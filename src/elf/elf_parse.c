// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- rjf: top-level binary parsing

internal ELF_Bin
elf_bin_from_data(Arena *arena, String8 data)
{
  ELF_Bin bin = {0};
  if(str8_match(str8_prefix(data, elf_magic_string.size), elf_magic_string, 0) &&
     data.size >= ELF_Identifier_Max)
  {
    //- rjf: parse sig/header
    U8 sig[ELF_Identifier_Max] = {0};
    str8_deserial_read(data, 0, &sig[0], sizeof(sig), 1);
    switch(sig[ELF_Identifier_Class])
    {
      default:
      case ELF_Class_None:{}break;
      case ELF_Class_32:
      {
        ELF_Hdr32 hdr32 = {0};
        U64 hdr_size = str8_deserial_read_struct(data, 0, &hdr32);
        if(hdr_size == sizeof(hdr32))
        {
          bin.hdr = elf_hdr64_from_hdr32(hdr32);
          U64 shstr_off = hdr32.e_shoff + hdr32.e_shentsize*hdr32.e_shstrndx;
          ELF_Shdr32 shdr = {0};
          U64 shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);
          if(shdr_size == sizeof(shdr))
          {
            bin.sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
          }
        }
      }break;
      case ELF_Class_64:
      {
        ELF_Hdr64 hdr64 = {0};
        U64 hdr_size = str8_deserial_read_struct(data, 0, &hdr64);
        if(hdr_size == sizeof(hdr64))
        {
          bin.hdr = hdr64;
          U64 shstr_off = hdr64.e_shoff + hdr64.e_shentsize*hdr64.e_shstrndx;
          ELF_Shdr64 shdr = {0};
          U64 shdr_size = str8_deserial_read_struct(data, shstr_off, &shdr);
          if(shdr_size == sizeof(shdr))
          {
            bin.sh_name_range = rng_1u64(shdr.sh_offset, shdr.sh_offset + shdr.sh_size);
          }
        }
      }break;
    }
    
    //- rjf: gather all shdrs
    {
      ELF_Hdr64 *hdr = &bin.hdr;
      bin.shdrs.count = hdr->e_shnum;
      bin.shdrs.v = push_array(arena, ELF_Shdr64, hdr->e_shnum);
      Rng1U64 shdr_range = rng_1u64(hdr->e_shoff, hdr->e_shoff + hdr->e_shentsize*hdr->e_shnum);
      String8 shdr_data = str8_substr(data, shdr_range);
      for EachIndex(shdr_idx, hdr->e_shnum)
      {
        switch(hdr->e_ident[ELF_Identifier_Class])
        {
          default:
          case ELF_Class_None:
          {}break;
          case ELF_Class_32:
          {
            ELF_Shdr32 shdr32 = {0};
            str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr32), &shdr32);
            bin.shdrs.v[shdr_idx] = elf_shdr64_from_shdr32(shdr32);
          }break;
          case ELF_Class_64:
          {
            str8_deserial_read_struct(shdr_data, shdr_idx * sizeof(ELF_Shdr64), &bin.shdrs.v[shdr_idx]);
          }break;
        }
      }
    }
    
    //- rjf: gather all phdrs
    {
      ELF_Hdr64 *hdr = &bin.hdr;
      bin.phdrs.count = hdr->e_phnum;
      bin.phdrs.v = push_array(arena, ELF_Phdr64, hdr->e_phnum);
      Rng1U64 phdr_range = rng_1u64(hdr->e_phoff, hdr->e_phoff + hdr->e_phentsize*hdr->e_phnum);
      String8 phdr_data = str8_substr(data, phdr_range);
      for EachIndex(phdr_idx, hdr->e_phnum)
      {
        switch(hdr->e_ident[ELF_Identifier_Class])
        {
          default:
          case ELF_Class_None:
          {}break;
          case ELF_Class_32:
          {
            ELF_Phdr32 phdr32 = {0};
            str8_deserial_read_struct(phdr_data, phdr_idx * sizeof(ELF_Phdr32), &phdr32);
            bin.phdrs.v[phdr_idx] = elf_phdr64_from_phdr32(phdr32);
          }break;
          case ELF_Class_64:
          {
            str8_deserial_read_struct(phdr_data, phdr_idx * sizeof(ELF_Phdr64), &bin.phdrs.v[phdr_idx]);
          }break;
        }
      }
    }
  }
  return bin;
}

//- rjf: extra bin info extraction

internal String8
elf_name_from_shdr64(String8 data, ELF_Bin *bin, ELF_Shdr64 *shdr)
{
  String8 sh_names = str8_substr(data, bin->sh_name_range);
  String8 name = {0};
  str8_deserial_read_cstr(sh_names, shdr->sh_name, &name);
  return name;
}

internal U64
elf_base_addr_from_bin(ELF_Bin *bin)
{
  U64 base_vaddr = 0;
  if(bin->hdr.e_type != ELF_Type_Dyn)
  {
    for EachIndex(phdr_idx, bin->phdrs.count)
    {
      ELF_Phdr64 *phdr = &bin->phdrs.v[phdr_idx];
      if(phdr->p_type == ELF_PhdrType_Load &&
         (base_vaddr == 0 || phdr->p_vaddr < base_vaddr))
      {
        base_vaddr = phdr->p_vaddr;
      }
    }
  }
  return base_vaddr;
}

internal ELF_GnuDebugLink
elf_gnu_debug_link_from_bin(String8 raw_data, ELF_Bin *bin)
{
  ELF_GnuDebugLink result = {0};
  for EachIndex(idx, bin->shdrs.count)
  {
    ELF_Shdr64 *shdr = &bin->shdrs.v[idx];
    String8 name = elf_name_from_shdr64(raw_data, bin, shdr);
    if(str8_match(name, str8_lit(".gnu_debuglink"), 0))
    {
      Rng1U64 raw_data_range = rng_1u64(shdr->sh_offset, shdr->sh_offset + shdr->sh_size);
      String8 data = str8_substr(raw_data, raw_data_range);
      String8 path = {0};
      U32 checksum = 0;
      {
        U64 cursor = 0;
        cursor += str8_deserial_read_cstr(data, cursor, &path);
        cursor = AlignPow2(cursor, 4);
        cursor += str8_deserial_read_struct(data, cursor, &checksum);
      }
      result.path = path;
      result.checksum = checksum;
      break;
    }
  }
  return result;
}

internal ELF_NoteList
elf_parse_note(Arena *arena, String8 raw_note, ELF_Class elf_class, ELF_MachineKind e_machine)
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
elf_read_phdrs(Arena *arena, ELF_Hdr64 ehdr, U64 base, Rng1U64 range, MachineOp_MemRead *mem_read, void *mem_read_ud, ELF_Phdr64Array *phdrs_out)
{
  Temp temp = temp_begin(arena);

  MachineOpResult op = MachineOpResult_Fail;

  Rng1U64 range_clamped = {0};
  range_clamped.min = Min(range.min, ehdr.e_phnum);
  range_clamped.max = Min(range.max, ehdr.e_phnum);

  // phdr index -> address
  U64 ph_lo = base + ehdr.e_phoff + range_clamped.min * ehdr.e_phentsize;

  // phdr index -> address
  U64 ph_hi = base + ehdr.e_phoff + range_clamped.max * ehdr.e_phentsize;

  // alloc output array
  ELF_Phdr64Array result = {0};
  result.count = dim_1u64(range_clamped);
  result.v     = push_array_no_zero(arena, ELF_Phdr64, result.count);

  // read program header table
  U64             result_size = ph_hi - ph_lo;
  if      (ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) { op = mem_read(ph_lo, result.v, result_size, mem_read_ud); }
  // TODO: convert to 64-bit
  else if (ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_32) { NotImplemented; }

  if (op == MachineOpResult_Ok) {
    *phdrs_out = result;
  } else {
    temp_end(temp);
  }
  return op;
}

internal MachineOpResult
elf_find_first_phdr(ELF_Hdr64          ehdr,
                    U64                base,
                    MachineOp_MemRead *mem_read,
                    void              *mem_read_ud,
                    ELF_PhdrType       phdr_type,
                    ELF_Phdr64        *phdr_out)
{
  Temp scratch = scratch_begin(0, 0);
  MachineOpResult op_result = MachineOpResult_Fail;

  for EachIndex(i, ehdr.e_phnum) {
    Temp temp = temp_begin(scratch.arena);

    ELF_Phdr64Array arr = {0};
    op_result = elf_read_phdrs(temp.arena, ehdr, base, r1u64(i, i + 1), mem_read, mem_read_ud, &arr);
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
elf_read_dyn_tags(Arena             *arena,
                  ELF_Hdr64          ehdr,
                  ELF_Phdr64         pt_dynamic,
                  Rng1U64            range,
                  MachineOp_MemRead *mem_read,
                  void              *mem_read_ud,
                  ELF_Dyn64Array    *dyns_out)
{
  Temp temp = temp_begin(arena);

  Assert(pt_dynamic.p_type == ELF_PhdrType_Dynamic);

  MachineOpResult op = MachineOpResult_Fail;

  U64 dy_ent_size = elf_dyn_size_from_class(ehdr.e_ident[ELF_Identifier_Class]);
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
  if (ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) {
    op = mem_read(dy_lo, dy_ptr, dy_size, mem_read_ud);
  }
  else if (ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_32) {
    // TODO: 32-bit conversion
    NotImplemented;
  }

  if (op == MachineOpResult_Ok && dyns_out) {
    // scan forward until first null entry
    dyns_out->count = index_of_zero_element(dy_ptr, dy_count, dy_ent_size);
    dyns_out->v     = dy_ptr;

    // release null entries
    U64 pop_count = dy_count - dyns_out->count;
    arena_pop(arena, pop_count * sizeof(dyns_out->v[0]));
  } else {
    temp_end(temp);
  }

  return op;
}

internal void
elf_rebase_phdr64_array(ELF_Phdr64Array *arr, U64 rebase)
{
  for EachIndex(i, arr->count) {
    arr->v[i].p_vaddr += rebase;
  }
}

internal void
elf_rebase_phdr64(ELF_Phdr64 *v, U64 rebase)
{
  elf_rebase_phdr64_array(&(ELF_Phdr64Array){ 1, v }, rebase);
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
elf_rebase_dyn64(ELF_Dyn64 *v, U64 rebase)
{
  elf_rebase_dyn64_array(&(ELF_Dyn64Array){ 1, v }, rebase);
}

internal MachineOpResult
elf_symbol_entry_vaddr_from_memory(ELF_Hdr64          ehdr,
                                   U64                base,
                                   B32                is_rebased,
                                   MachineOp_MemRead *mem_read,
                                   void              *mem_read_ud,
                                   String8            symbol_name,
                                   U64               *symbol_entry_vaddr_out)
{
  Temp scratch = scratch_begin(0,0);
  MachineOpResult op_result = MachineOpResult_Fail;

  U64 rebase = (is_rebased && ehdr.e_type == ELF_Type_Dyn) ? base : 0;

  // find PT_DYNAMIC
  ELF_Phdr64 pt_dynamic = {0};
  op_result = elf_find_first_phdr(ehdr, base, mem_read, mem_read_ud, ELF_PhdrType_Dynamic, &pt_dynamic);
  if (op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  if(is_rebased)
  {
    elf_rebase_phdr64(&pt_dynamic, rebase);
  }

  // read dynamic tags
  ELF_Dyn64Array dyns = {0};
  op_result = elf_read_dyn_tags(scratch.arena, ehdr, pt_dynamic, r1u64(0, max_U16), mem_read, mem_read_ud, &dyns);
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

  // rebase tags
  elf_rebase_dyn64(dt_symtab, rebase);
  elf_rebase_dyn64(dt_strtab, rebase);
  if(dt_hash_sysv != 0) { elf_rebase_dyn64(dt_hash_sysv, rebase); }
  if(dt_hash_gnu  != 0) { elf_rebase_dyn64(dt_hash_gnu,  rebase); }

  // pick symbol size
  U64 syment_size = elf_sym_size_from_class(ehdr.e_ident[ELF_Identifier_Class]);
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
    op_result = mem_read(dt_hash_gnu->val, &header, sizeof(header), mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    U32 hash            = elf_hash_gnu_from_string(symbol_name);
    U64 bloom_word_size = (ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64) ? 8 : 4;
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
    op_result = mem_read(bloom_word_vaddr, &bloom_word, bloom_word_size, mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    // key is not in the filter? -> skip
    U64 bloom_mask = (1ull << (hash % word_bit_count)) | (1ull << ((hash >> header.bloom_shift) % word_bit_count));
    if((bloom_word & bloom_mask) != bloom_mask) { goto skip_gnu; }

    // read symbol index
    U64 symbol_idx_vaddr = buckets_vaddr + (hash % header.nbuckets) * sizeof(U32);
    U32 symbol_idx       = 0;
    op_result = mem_read(symbol_idx_vaddr, &symbol_idx, sizeof(symbol_idx), mem_read_ud);
    if (op_result != MachineOpResult_Ok) { goto skip_gnu; }
    op_result = MachineOpResult_Fail;

    if(symbol_idx == 0 || symbol_idx < header.symoffset) { goto skip_gnu; }

    // walk symbol chain
    for(U64 chain_idx = symbol_idx - header.symoffset;; chain_idx += 1, symbol_idx += 1)
    {
      // read chain hash
      U64 chain_hash_vaddr = chains_vaddr + chain_idx * sizeof(U32);
      U32 chain_hash       = 0;
      op_result = mem_read(chain_hash_vaddr, &chain_hash, sizeof(chain_hash), mem_read_ud);
      if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
      op_result = MachineOpResult_Fail;

      if((chain_hash | 1) == (hash | 1))
      {
        // read symbol
        U64       symbol_entry_vaddr = dt_symtab->val + symbol_idx * syment_size;
        ELF_Sym64 symbol             = {0};
        op_result = elf_read_symbol(mem_read, mem_read_ud, symbol_entry_vaddr, ehdr.e_ident[ELF_Identifier_Class], &symbol);
        if(op_result != MachineOpResult_Ok) { goto skip_gnu; }
        op_result = MachineOpResult_Fail;

        if(symbol.st_name != 0 && symbol.st_name < dt_strsz->val && symbol.st_shndx != ELF_SectionIndex_Undef)
        {
          U64     string_vaddr     = dt_strtab->val + symbol.st_name;
          U64     string_vaddr_opl = dt_strtab->val + dt_strsz->val;
          String8 string           = {0};
          op_result = machine_read_cstring_capped(scratch.arena, string_vaddr, string_vaddr_opl, mem_read, mem_read_ud, &string);

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
    op_result = mem_read(dt_hash_sysv->val, &header, sizeof(header), mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
    op_result = MachineOpResult_Fail;

    // is the hash table empty? -> skip
    if(header.nbuckets == 0) { goto skip_sysv; }

    U32 hash          = elf_hash_sysv_from_string(symbol_name);
    U64 buckets_vaddr = dt_hash_sysv->val + sizeof(header);
    U64 chains_vaddr  = buckets_vaddr + header.nbuckets * sizeof(U32);

    // read symbol index
    U32 symbol_idx    = 0;
    op_result = mem_read(buckets_vaddr + (hash % header.nbuckets) * sizeof(U32), &symbol_idx, sizeof(symbol_idx), mem_read_ud);
    if(op_result != MachineOpResult_Ok) { goto skip_sysv; }

    // walk symbol chain
    while(symbol_idx != 0 && symbol_idx < header.nchains)
    {
      // read symbol
      U64       symbol_entry_vaddr = dt_symtab->val + symbol_idx * syment_size;
      ELF_Sym64 symbol             = {0};
      op_result = elf_read_symbol(mem_read, mem_read_ud, symbol_entry_vaddr, ehdr.e_ident[ELF_Identifier_Class], &symbol);
      if(op_result != MachineOpResult_Ok) { goto skip_sysv; }
      op_result = MachineOpResult_Fail;

      if(symbol.st_name != 0 && symbol.st_name < dt_strsz->val && symbol.st_shndx != ELF_SectionIndex_Undef)
      {
        U64     string_vaddr     = dt_strtab->val + symbol.st_name;
        U64     string_vaddr_opl = dt_strtab->val + dt_strsz->val;
        String8 string           = {0};
        op_result = machine_read_cstring_capped(scratch.arena, string_vaddr, string_vaddr_opl, mem_read, mem_read_ud, &string);

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
      op_result = mem_read(next_symbol_idx_vaddr, &symbol_idx, sizeof(symbol_idx), mem_read_ud);
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
elf_find_rdebug_vaddr(U64 loader_vbase, B32 is_rebased, MachineOp_MemRead *mem_read, void *mem_read_ud, U64 *rdebug_vaddr_out)
{
  MachineOpResult op_result = MachineOpResult_Fail;

  // load DL header
  ELF_Hdr64 ehdr = {0};
  op_result = elf_read_ehdr(mem_read, mem_read_ud, loader_vbase, &ehdr);
  if(op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // look up _r_debug symbol table address
  U64 rdebug_symbol_entry_vaddr = 0;
  op_result = elf_symbol_entry_vaddr_from_memory(ehdr, loader_vbase, is_rebased, mem_read, mem_read_ud, str8_lit("_r_debug"), &rdebug_symbol_entry_vaddr);
  if(op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // symbol address -> ELF symbol
  ELF_Sym64 rdebug_symbol = {0};
  op_result = elf_read_symbol(mem_read, mem_read_ud, rdebug_symbol_entry_vaddr, ehdr.e_ident[ELF_Identifier_Class], &rdebug_symbol);
  if(op_result != MachineOpResult_Ok) { goto exit; }
  op_result = MachineOpResult_Fail;

  // is ELF symbol invalid? -> exit
  ELF_SymType symbol_type = ELF_ST_TYPE(rdebug_symbol.st_info);
  if(symbol_type != ELF_SymType_Object || rdebug_symbol.st_size == 0) { goto exit; }

  if(rdebug_vaddr_out)
  {
    // rebase address
    if(rdebug_symbol.st_shndx != ELF_SectionIndex_Abs && is_rebased && ehdr.e_type == ELF_Type_Dyn)
    {
      *rdebug_vaddr_out = rdebug_symbol.st_value + loader_vbase;
    }
    else
    {
      *rdebug_vaddr_out = rdebug_symbol.st_value;
    }
  }

  op_result = MachineOpResult_Ok;

  exit:;
  return op_result;
}
