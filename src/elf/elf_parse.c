// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal MachineOpResult
elf_load_virtual(MachineOp_MemRead *mem_read, int memory_fd, U64 base, B32 is_rebased, ELF_Bin *elf_out)
{
  ELF_Bin elf = { .addr_mode = ELF_BinAddrMode_Virtual, .mem_read = mem_read, .base = base, .is_rebased = is_rebased };
  MemoryCopy(&elf.mem_read_ud[0], &memory_fd, sizeof(memory_fd));

  MachineOpResult status = elf_read_ehdr(elf.mem_read, elf.mem_read_ud, elf.base, &elf.ehdr);
  if(status == MachineOpResult_Ok && elf_out) {
    *elf_out = elf;
  }
  return status;
}

internal MachineOpResult
elf_load_file(String8 file, ELF_Bin *elf_out)
{
  ELF_Bin elf = { .addr_mode = ELF_BinAddrMode_File, .mem_read = machine_op_read_string };
  MemoryCopy(&elf.mem_read_ud[0], &file, sizeof(file));

  B32 is_read = elf_read_ehdr_string(file, &elf.ehdr);
  if (is_read && elf_out) {
    *elf_out = elf;
  }
  return is_read ? MachineOpResult_Ok : MachineOpResult_Fail;
}

internal U64
elf_rebase_vaddr(ELF_Bin *elf, U64 vaddr)
{
  if (elf->ehdr.e_type == ELF_Type_Dyn) {
    return elf->base + vaddr;
  }
  return vaddr;
}

internal U64
elf_base_addr_from_bin(ELF_Bin *elf, ELF_Phdr64Array phdrs)
{
  U64 base_vaddr = 0;
  if (elf->ehdr.e_type != ELF_Type_Dyn) {
    for EachIndex(phdr_idx, phdrs.count) {
      ELF_Phdr64 *phdr = &phdrs.v[phdr_idx];
      if (phdr->p_type == ELF_PhdrType_Load && (base_vaddr == 0 || phdr->p_vaddr < base_vaddr)) {
        base_vaddr = phdr->p_vaddr;
      }
    }
  }
  return base_vaddr;
}

internal Rng1U64
elf_image_vrange_from_phdrs(ELF_Bin *elf, ELF_Phdr64Array phdrs)
{
  Rng1U64 result = { .min = max_U64 };
  for EachIndex(i, phdrs.count) {
    if (phdrs.v[i].p_type  == ELF_PhdrType_Load) {
      U64 min = elf_rebase_vaddr(elf, phdrs.v[i].p_vaddr);
      U64 max = elf_rebase_vaddr(elf, phdrs.v[i].p_vaddr) + phdrs.v[i].p_memsz;
      result.min = Min(result.min, min);
      result.max = Max(result.max, max);
    }
  }
  return result;
}

internal void
elf_rebase_phdr64_array(ELF_Phdr64Array *arr, U64 rebase)
{
  for EachIndex(i, arr->count) {
    arr->v[i].p_vaddr += rebase;
  }
}

internal void
elf_rebase_dyn64_array(ELF_Dyn64Array *arr, U64 rebase)
{
  for EachIndex(i, arr->count) {
    ELF_DynTagValueKind value_kind = elf_value_kind_from_dyn_tag(arr->v[i].tag);
    if (value_kind == ELF_DynTagValueKind_Address) {
      arr->v[i].val += rebase;
    }
  }
}

internal void
elf_rebase_phdr64(ELF_Phdr64 *v, U64 rebase)
{
  elf_rebase_phdr64_array(&(ELF_Phdr64Array){ 1, v }, rebase);
}

internal void
elf_rebase_dyn64(ELF_Dyn64 *v, U64 rebase)
{
  elf_rebase_dyn64_array(&(ELF_Dyn64Array){ 1, v }, rebase);
}

internal ELF_NoteList
elf_parse_note(Arena *arena, ELF_Hdr64 ehdr, String8 raw_note)
{
  ELF_NoteList result = {0};
  
  for (U64 cursor = 0; cursor < raw_note.size; ) {
    U32 owner_size;
    U64 owner_size_size = str8_deserial_read_struct(raw_note, cursor, &owner_size);
    if (owner_size_size == 0) { goto exit; }
    cursor += owner_size_size;
    
    U32 desc_size;
    U64 desc_size_size = str8_deserial_read_struct(raw_note, cursor, &desc_size);
    if (desc_size_size == 0) { goto exit; }
    cursor += desc_size_size;
    
    ELF_NoteType type;
    U64 type_size = str8_deserial_read_struct(raw_note, cursor, &type);
    if (type_size == 0) { goto exit; }
    cursor += type_size;
    
    if (cursor + owner_size > raw_note.size) { goto exit; }
    String8 owner = str8_cstring_capped(raw_note.str + cursor, raw_note.str + cursor + owner_size);
    cursor += owner_size;
    
    if (cursor + desc_size > raw_note.size) { goto exit; }
    String8 desc = str8_substr(raw_note, r1u64(cursor, cursor + desc_size));
    cursor += desc_size;
    cursor = AlignPow2(cursor, 4);
    
    ELF_NoteNode *n = push_array(arena, ELF_NoteNode, 1);
    n->v.owner = owner;
    n->v.desc  = desc;
    n->v.type  = type;
    
    SLLQueuePush(result.first, result.last, n);
    result.count += 1;
  }
  
  exit:;
  return result;
}

internal MachineOpResult
elf_parse_shdr_name(Arena *arena, ELF_Bin *elf, ELF_Shdr64 *shstr, ELF_Shdr64 *shdr, String8 *name_out)
{
  U64 shstr_vaddr = 0;
  U64 shstr_opl   = 0;
  if (elf->addr_mode == ELF_BinAddrMode_Virtual) {
    if (shstr->sh_addr) {
      shstr_vaddr = shstr->sh_addr;
      shstr_opl   = shstr->sh_addr + shstr->sh_size;
    }
  } else if (elf->addr_mode == ELF_BinAddrMode_File) {
    shstr_vaddr = shstr->sh_offset;
    shstr_opl   = shstr->sh_offset + shstr->sh_size;
  }

  String8         name   = {0};
  MachineOpResult status = machine_read_cstring_opl(arena, shstr_vaddr + shdr->sh_name, shstr_opl, elf->mem_read, elf->mem_read_ud, &name);
  if (name_out && status == MachineOpResult_Ok) {
    *name_out = name;
  }

  return status;
}

internal MachineOpResult
elf_parse_shdr_data(Arena *arena, ELF_Bin *elf, ELF_Shdr64 *shdr, String8 *data_out)
{
  U64 addr = 0;
  if (shdr->sh_flags & ELF_Shf_Alloc) {
    addr = shdr->sh_addr;
  } else {
    addr = shdr->sh_offset;
  }

  U8 *buffer = push_array(arena, U8, shdr->sh_size);
  MachineOpResult status = elf->mem_read(addr, buffer, shdr->sh_size, elf->mem_read_ud);

  if (status == MachineOpResult_Ok) {
    if (data_out) {
      *data_out = str8(buffer, shdr->sh_size);
    }
  } else {
    arena_pop(arena, shdr->sh_size);
  }

  return status;
}

internal MachineOpResult
elf_parse_gnu_debug_link(Arena *arena, ELF_Bin *elf, ELF_GnuDebugLink *debug_link_out)
{
  MachineOpResult op_result = MachineOpResult_Fail;
  ELF_GnuDebugLink result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  
  // find GNU Link section
  ELF_Shdr64 shdr = {0};
  op_result = elf_find_shdr_by_name(elf, str8_lit(".gnu_debuglink"), &shdr);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // locate section header
  U64 sh_vaddr     = 0;
  U64 sh_vaddr_opl = 0;
  if (elf->addr_mode == ELF_BinAddrMode_File) {
    sh_vaddr     = shdr.sh_offset;
    sh_vaddr_opl = shdr.sh_offset + shdr.sh_size;
  } else if (elf->addr_mode == ELF_BinAddrMode_Virtual) {
    if (shdr.sh_addr) {
      sh_vaddr     = elf_rebase_vaddr(elf, shdr.sh_addr);
      sh_vaddr_opl = elf_rebase_vaddr(elf, shdr.sh_addr) + shdr.sh_size;
    } else {
      // no in-memory sections
      goto exit;
    }
  }

  // read file path
  op_result = machine_read_cstring_opl(arena, sh_vaddr, sh_vaddr_opl, elf->mem_read, elf->mem_read_ud, &result.path);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;
  
  // read image checksum
  U64 checksum_off       = AlignPow2(result.path.size + 1, 4);
  U64 checksum_read_size = sh_vaddr + checksum_off <= sh_vaddr_opl ? sizeof(result.checksum) : 0;
  op_result = elf->mem_read(sh_vaddr + checksum_off, &result.checksum, checksum_read_size, elf->mem_read_ud);
  if(op_result != MachineOpResult_Ok) { goto exit; }

  if(debug_link_out != 0)
  {
    *debug_link_out = result;
  }
  
  exit:;
  scratch_end(scratch);
  return op_result;
}

internal MachineOpResult
elf_parse_shdrs(Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Shdr64Array *shdrs_out)
{
  Temp temp = temp_begin(arena);

  MachineOpResult parse_result = MachineOpResult_Ok;

  // clamp parse range
  Rng1U64 range_clamped = {0};
  range_clamped.min = Min(range.min, elf->ehdr.e_shnum);
  range_clamped.max = Min(range.max, elf->ehdr.e_shnum);
  
  // alloc section header array
  ELF_Shdr64Array result = {0};
  result.count = dim_1u64(range_clamped);
  result.v     = push_array_no_zero(arena, ELF_Shdr64, result.count);

  // TODO: do bulk read and then convert one-by-one
  for EachInRange(i, range_clamped) {
    // compute address of the section header
    U64 shdr_vaddr = elf->base + elf->ehdr.e_shoff + i*elf->ehdr.e_shentsize;

    // TODO: use elf class in the elf
    parse_result = elf_read_shdr(elf->mem_read, elf->mem_read_ud, shdr_vaddr, elf->ehdr.e_ident[ELF_Identifier_Class], &result.v[i]);
    if (parse_result != MachineOpResult_Ok) { break; }
  }
  
  if (parse_result == MachineOpResult_Ok) {
    *shdrs_out = result;
  } else {
    temp_end(temp);
  }

  return parse_result;
}

internal MachineOpResult
elf_parse_phdrs(Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Phdr64Array *phdrs_out)
{
  Temp temp = temp_begin(arena);

  MachineOpResult op = MachineOpResult_Fail;

  Rng1U64 range_clamped = {0};
  range_clamped.min = Min(range.min, elf->ehdr.e_phnum);
  range_clamped.max = Min(range.max, elf->ehdr.e_phnum);

  // phdr index -> address
  U64 ph_lo = elf->base + elf->ehdr.e_phoff + range_clamped.min * elf->ehdr.e_phentsize;

  // phdr index -> address
  U64 ph_hi = elf->base + elf->ehdr.e_phoff + range_clamped.max * elf->ehdr.e_phentsize;

  // alloc output array
  ELF_Phdr64Array result = {0};
  result.count = dim_1u64(range_clamped);
  result.v     = push_array_no_zero(arena, ELF_Phdr64, result.count);

  // read program header table
  U64             result_size = ph_hi - ph_lo;
  if      (elf->ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) { op = elf->mem_read(ph_lo, result.v, result_size, elf->mem_read_ud); }
  // TODO: convert to 64-bit
  else if (elf->ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_32) { NotImplemented; }

  if (!elf->is_rebased && elf->ehdr.e_type == ELF_Type_Dyn) {
    for EachIndex(i, result.count) {
      result.v[i].p_vaddr += elf->base;
    }
  }

  if (op == MachineOpResult_Ok) {
    *phdrs_out = result;
  } else {
    temp_end(temp);
  }
  return op;
}

internal MachineOpResult
elf_parse_dyns(Arena *arena, ELF_Bin *elf, Rng1U64 range, ELF_Phdr64 pt_dynamic, ELF_Dyn64Array *dyns_out)
{
  Temp temp = temp_begin(arena);

  Assert(pt_dynamic.p_type == ELF_PhdrType_Dynamic);

  MachineOpResult op = MachineOpResult_Fail;

  U64 dy_ent_size = elf_dyn_size_from_class(elf->ehdr.e_ident[ELF_Identifier_Class]);
  U64 dy_cap      = pt_dynamic.p_filesz / dy_ent_size;

  // clamp range
  Rng1U64 range_clamped = {0};
  range_clamped.min = Min(range.min, dy_cap);
  range_clamped.max = Min(range.max, dy_cap);

  // map range indices to dynamic entry offsets
  U64   dy_lo    = pt_dynamic.p_vaddr + range_clamped.min * dy_ent_size;
  U64   dy_hi    = pt_dynamic.p_vaddr + range_clamped.max * dy_ent_size;
  U64   dy_size  = dy_hi - dy_lo;
  U64   dy_count = dy_size / dy_ent_size;
  void *dy_ptr   = push_array(arena, U8, dy_size);

  // read dynamic tags
  if (elf->ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) {
    op = elf->mem_read(dy_lo, dy_ptr, dy_size, elf->mem_read_ud);
  }
  else if (elf->ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_32) {
    // TODO: 32-bit conversion
    NotImplemented;
  }

  if (op == MachineOpResult_Ok && dyns_out) {
    // scan forward until first null entry
    dyns_out->count = index_of_zero_element(dy_ptr, dy_count, dy_ent_size);
    dyns_out->v     = dy_ptr;

    if (!elf->is_rebased && elf->ehdr.e_type == ELF_Type_Dyn) {
      elf_rebase_dyn64_array(dyns_out, elf->base);
    }

    // release null entries
    U64 pop_count = dy_count - dyns_out->count;
    arena_pop(arena, pop_count * sizeof(dyns_out->v[0]));
  } else {
    temp_end(temp);
  }

  return op;
}

internal MachineOpResult
elf_find_first_phdr(ELF_Bin *elf, ELF_PhdrType phdr_type, ELF_Phdr64 *phdr_out)
{
  Temp scratch = scratch_begin(0, 0);
  MachineOpResult op_result = MachineOpResult_Fail;

  for EachIndex(i, elf->ehdr.e_phnum) {
    Temp temp = temp_begin(scratch.arena);

    ELF_Phdr64Array arr = {0};
    op_result = elf_parse_phdrs(temp.arena, elf, r1u64(i, i + 1), &arr);
    if (op_result != MachineOpResult_Ok) { break; }
    op_result = MachineOpResult_Fail;

    if (arr.count == 0) { break; }

    if (arr.v[0].p_type == phdr_type) {
      if (phdr_out) {
        *phdr_out = arr.v[0];
      }
      op_result = MachineOpResult_Ok;
      break;
    }

    temp_end(temp);
  }

  scratch_end(scratch);
  return op_result;
}

internal MachineOpResult
elf_find_shdr_by_name(ELF_Bin *elf, String8 name, ELF_Shdr64 *shdr_out)
{
  Temp scratch = scratch_begin(0,0);

  // read section headers
  ELF_Shdr64Array shdrs  = {0};
  MachineOpResult status = elf_parse_shdrs(scratch.arena, elf, r1u64(0, elf->ehdr.e_shnum), &shdrs);
  if (status == MachineOpResult_Ok) {
    status = MachineOpResult_Fail;

    // get the string table
    ELF_Shdr64 *shstr_shdr = &shdrs.v[elf->ehdr.e_shstrndx];

    // TODO: linear scan string table scan
    for EachIndex(shdr_idx, shdrs.count) {
      ELF_Shdr64 *shdr = &shdrs.v[shdr_idx];

      // skip corrupted name offsets
      if (shdr->sh_name >= shstr_shdr->sh_size) {
        continue;
      }

      // shstr_shdr -> name
      U64     shdr_name_vaddr = shstr_shdr->sh_offset + shdr->sh_name;
      U64     shdr_name_opl   = shstr_shdr->sh_offset + shstr_shdr->sh_size;
      String8 shdr_name       = {0};
      status = machine_read_cstring_opl(scratch.arena, shdr_name_vaddr, shdr_name_opl, elf->mem_read, elf->mem_read_ud, &shdr_name);
      if (status != MachineOpResult_Ok) { break; }

      // match? -> stop
      if (str8_match(shdr_name, name, 0)) {
        if (shdr_out != 0) {
          *shdr_out = *shdr;
        }
        status = MachineOpResult_Ok;
        break;
      }
    }
  }
  
  scratch_end(scratch);
  return status;
}

internal MachineOpResult
elf_find_symbol_entry_by_name(ELF_Bin *elf, String8 symbol_name, U64 *symbol_entry_vaddr_out)
{
  Temp scratch = scratch_begin(0,0);
  MachineOpResult op_result = MachineOpResult_Fail;

  U64 rebase = (elf->is_rebased && elf->ehdr.e_type == ELF_Type_Dyn) ? elf->base : 0;

  // find PT_DYNAMIC
  ELF_Phdr64 pt_dynamic = {0};
  op_result = elf_find_first_phdr(elf, ELF_PhdrType_Dynamic, &pt_dynamic);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // read dynamic tags
  ELF_Dyn64Array dyns = {0};
  op_result = elf_parse_dyns(scratch.arena, elf, r1u64(0, max_U16), pt_dynamic, &dyns);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // find symbol table tags
  ELF_Dyn64 *dt_hash_sysv = 0; // SysV hash table tag
  ELF_Dyn64 *dt_hash_gnu  = 0; // GNU hash table tag
  ELF_Dyn64 *dt_symtab    = 0; // symbol table address
  ELF_Dyn64 *dt_syment    = 0; // size of ELF symbol
  ELF_Dyn64 *dt_strtab    = 0; // string table address
  ELF_Dyn64 *dt_strsz     = 0; // string table size
  for EachIndex(i, dyns.count)
  {
    if      (dyns.v[i].tag == ELF_DynTag_Hash)     { dt_hash_sysv = &dyns.v[i]; }
    else if (dyns.v[i].tag == ELF_DynTag_GNU_Hash) { dt_hash_gnu  = &dyns.v[i]; }
    else if (dyns.v[i].tag == ELF_DynTag_Symtab)   { dt_symtab    = &dyns.v[i]; }
    else if (dyns.v[i].tag == ELF_DynTag_Syment)   { dt_syment    = &dyns.v[i]; }
    else if (dyns.v[i].tag == ELF_DynTag_Strtab)   { dt_strtab    = &dyns.v[i]; }
    else if (dyns.v[i].tag == ELF_DynTag_Strsz)    { dt_strsz     = &dyns.v[i]; }
  }

  // no symbol table tags? -> exit
  if(dt_symtab == 0 || dt_strtab == 0 || dt_strsz == 0) { goto exit; }

  // pick symbol size
  U64 syment_size = elf_sym_size_from_class(elf->ehdr.e_ident[ELF_Identifier_Class]);
  if(dt_syment != 0 && dt_syment->val != 0)
  {
    syment_size = dt_syment->val;
  }

  //
  // GNU variant
  //
  if(op_result != MachineOpResult_Ok && op_result != MachineOpResult_Maybe && dt_hash_gnu != 0)
  {
    typedef struct ELF_HashHeader_GNU64
    {
      U32 nbuckets;
      U32 symoffset;
      U32 bloom_size;
      U32 bloom_shift;
      // U64 bloom[bloom_size];
      // U32 buckets[nbuckets];
      // U32 chains[nchains];
    } ELF_HashHeader_GNU64;

    // read the header
    ELF_HashHeader_GNU64 header = {0};
    op_result = elf->mem_read(dt_hash_gnu->val, &header, sizeof(header), elf->mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    U32 hash            = elf_hash_gnu_from_string(symbol_name);
    U64 bloom_word_size = (elf->ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) ? 8 : 4;
    U64 buckets_vaddr   = dt_hash_gnu->val + sizeof(header) + header.bloom_size * bloom_word_size;
    U64 chains_vaddr    = buckets_vaddr + header.nbuckets * sizeof(U32);

    // is the hash table empty? -> skip
    if(header.nbuckets == 0 || header.bloom_size == 0) { goto skip_gnu; }

    // compute bloom filter word address
    U64 word_bit_count = bloom_word_size * 8;
    U64 bloom_idx        = (hash / word_bit_count) % header.bloom_size;
    U64 bloom_word_vaddr = dt_hash_gnu->val + sizeof(header) + bloom_idx * bloom_word_size;
    U64 bloom_word       = 0;

    // read bloom word
    op_result = elf->mem_read(bloom_word_vaddr, &bloom_word, bloom_word_size, elf->mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    // key is not in the filter? -> skip
    U64 bloom_mask = (1ull << (hash % word_bit_count)) | (1ull << ((hash >> header.bloom_shift) % word_bit_count));
    if((bloom_word & bloom_mask) != bloom_mask) { goto skip_gnu; }

    // read symbol index
    U64 symbol_idx_vaddr = buckets_vaddr + (hash % header.nbuckets) * sizeof(U32);
    U32 symbol_idx       = 0;
    op_result = elf->mem_read(symbol_idx_vaddr, &symbol_idx, sizeof(symbol_idx), elf->mem_read_ud);
    if (op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    if(symbol_idx == 0 || symbol_idx < header.symoffset) { goto skip_gnu; }

    // walk symbol chain
    for(U64 chain_idx = symbol_idx - header.symoffset;; chain_idx += 1, symbol_idx += 1)
    {
      // read chain hash
      U64 chain_hash_vaddr = chains_vaddr + chain_idx * sizeof(U32);
      U32 chain_hash       = 0;
      op_result = elf->mem_read(chain_hash_vaddr, &chain_hash, sizeof(chain_hash), elf->mem_read_ud);
      if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
      op_result = MachineOpResult_Fail;

      if((chain_hash | 1) == (hash | 1))
      {
        // read symbol
        U64       symbol_entry_vaddr = dt_symtab->val + symbol_idx * syment_size;
        ELF_Sym64 symbol             = {0};
        op_result = elf_read_symbol(elf->mem_read, elf->mem_read_ud, symbol_entry_vaddr, elf->ehdr.e_ident[ELF_Identifier_Class], &symbol);
        if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
        op_result = MachineOpResult_Fail;

        if(symbol.st_name != 0 && symbol.st_name < dt_strsz->val && symbol.st_shndx != ELF_SectionIndex_Undef)
        {
          U64     string_vaddr     = dt_strtab->val + symbol.st_name;
          U64     string_vaddr_opl = dt_strtab->val + dt_strsz->val;
          String8 string           = {0};
          op_result = machine_read_cstring_opl(scratch.arena, string_vaddr, string_vaddr_opl, elf->mem_read, elf->mem_read_ud, &string);

          // no string? -> skip
          if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
          op_result = MachineOpResult_Fail;

          if(str8_match(string, symbol_name, 0))
          {
            if(symbol_entry_vaddr_out)
            {
              *symbol_entry_vaddr_out = symbol_entry_vaddr;
            }
            op_result = MachineOpResult_Ok;
            goto exit;
          }
        }
      }

      // reached end of the chain marker? -> stop
      if(chain_hash & 1) { break; }
    }
  }
  skip_gnu:;

  //
  // SysV variant
  //
  if(op_result != MachineOpResult_Ok && op_result != MachineOpResult_Maybe && dt_hash_sysv != 0)
  {
    typedef struct ELF_HashHeader_SYSV
    {
      U32 nbuckets;
      U32 nchains;
      // U32 bucket[nbuckets];
      // U32 chains[nchains];
    } ELF_HashHeader_SYSV;

    // read the header
    ELF_HashHeader_SYSV header = {0};
    op_result = elf->mem_read(dt_hash_sysv->val, &header, sizeof(header), elf->mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
    op_result = MachineOpResult_Fail;

    // is the hash table empty? -> skip
    if(header.nbuckets == 0) { goto skip_sysv; }

    U32 hash          = elf_hash_sysv_from_string(symbol_name);
    U64 buckets_vaddr = dt_hash_sysv->val + sizeof(header);
    U64 chains_vaddr  = buckets_vaddr + header.nbuckets * sizeof(U32);

    // read symbol index
    U32 symbol_idx    = 0;
    op_result = elf->mem_read(buckets_vaddr + (hash % header.nbuckets) * sizeof(U32), &symbol_idx, sizeof(symbol_idx), elf->mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_sysv; }

    // walk symbol chain
    while(symbol_idx != 0 && symbol_idx < header.nchains)
    {
      // read symbol
      U64       symbol_entry_vaddr = dt_symtab->val + symbol_idx * syment_size;
      ELF_Sym64 symbol             = {0};
      op_result = elf_read_symbol(elf->mem_read, elf->mem_read_ud, symbol_entry_vaddr, elf->ehdr.e_ident[ELF_Identifier_Class], &symbol);
      if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
      op_result = MachineOpResult_Fail;

      if(symbol.st_name != 0 && symbol.st_name < dt_strsz->val && symbol.st_shndx != ELF_SectionIndex_Undef)
      {
        U64     string_vaddr     = dt_strtab->val + symbol.st_name;
        U64     string_vaddr_opl = dt_strtab->val + dt_strsz->val;
        String8 string           = {0};
        op_result = machine_read_cstring_opl(scratch.arena, string_vaddr, string_vaddr_opl, elf->mem_read, elf->mem_read_ud, &string);

        // no string? -> skip
        if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
        op_result = MachineOpResult_Fail;

        if(str8_match(string, symbol_name, 0))
        {
          if(symbol_entry_vaddr_out)
          {
            *symbol_entry_vaddr_out = symbol_entry_vaddr;
          }
          op_result = MachineOpResult_Ok;
          goto exit;
        }
      }

      // read next symbol index in the chain
      U64 next_symbol_idx_vaddr = chains_vaddr + symbol_idx * sizeof(U32);
      op_result = elf->mem_read(next_symbol_idx_vaddr, &symbol_idx, sizeof(symbol_idx), elf->mem_read_ud);
      if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
      op_result = MachineOpResult_Fail;
    }
  }
  skip_sysv:;

  exit:;
  scratch_end(scratch);
  return op_result;
}

internal MachineOpResult
elf_find_rdebug_vaddr(ELF_Bin *elf, U64 *rdebug_vaddr_out)
{
  MachineOpResult op_result = MachineOpResult_Fail;

  // look up _r_debug symbol table address
  U64 rdebug_symbol_entry_vaddr = 0;
  op_result = elf_find_symbol_entry_by_name(elf, str8_lit("_r_debug"), &rdebug_symbol_entry_vaddr);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // symbol address -> ELF symbol
  ELF_Sym64 rdebug_symbol = {0};
  op_result = elf_read_symbol(elf->mem_read, elf->mem_read_ud, rdebug_symbol_entry_vaddr, elf->ehdr.e_ident[ELF_Identifier_Class], &rdebug_symbol);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // is ELF symbol invalid? -> exit
  ELF_SymType symbol_type = ELF_ST_TYPE(rdebug_symbol.st_info);
  if(symbol_type != ELF_SymType_Object || rdebug_symbol.st_size == 0) { goto exit; }

  if (rdebug_vaddr_out) {
    // rebase address
    if (rdebug_symbol.st_shndx != ELF_SectionIndex_Abs && !elf->is_rebased && elf->ehdr.e_type == ELF_Type_Dyn) {
      *rdebug_vaddr_out = rdebug_symbol.st_value + elf->base;
    } else {
      *rdebug_vaddr_out = rdebug_symbol.st_value;
    }
  }

  op_result = MachineOpResult_Ok;

  exit:;
  return op_result;
}

