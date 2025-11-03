// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

//- rjf: file descriptor memory reading/writing helpers

internal U64
dmn_lnx_size_from_fd(int memory_fd, U64 cap)
{
  U8 temp[4096];
  size_t cursor = 0;
  while(cursor < cap)
  {
    ssize_t actual_read = pread(memory_fd, temp, sizeof(temp), cursor);
    if(actual_read < 0)
    {
      if(errno == EINTR) { continue; }
      break;
    }
    if(actual_read == 0) { break; }
    cursor += (U64)actual_read;
  }
  return (U64)cursor;
}

internal U64
dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst)
{
  size_t cursor = 0, size = dim_1u64(range);
  while(cursor < size)
  {
    size_t  to_read     = size - cursor;
    ssize_t actual_read = pread(memory_fd, (U8 *)dst + cursor, to_read, range.min + cursor);
    if(actual_read < 0)
    {
      if(errno == EINTR) { continue; }
      break;
    }
    if(actual_read == 0) { break; }
    cursor += actual_read;
  }
  return (U64)cursor;
}

internal B32
dmn_lnx_write(int memory_fd, Rng1U64 range, void *src)
{
  B32 result = 1;
  U8 *ptr = (U8 *)src;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    size_t to_write = (size_t)(opl - ptr);
    ssize_t actual_write = pwrite(memory_fd, ptr, to_write, cursor);
    if(actual_write == -1)
    {
      result = 0;
      break;
    }
    ptr += actual_write;
    cursor += actual_write;
  }
  return result;
}

internal String8
dmn_lnx_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size)
{
  String8 result = {0};
  U64 string_size = 0;
  for(U64 vaddr = base_vaddr; string_size < cap_size; vaddr += 1, string_size += 1)
  {
    char byte = 0;
    if(pread(memory_fd, &byte, sizeof(byte), vaddr) == 0)
    {
      break;
    }
    if(byte == '\0' || byte == '\n')
    {
      break;
    }
  }
  if(string_size != 0)
  {
    char *buf = push_array_no_zero(arena, char, string_size+1);
    pread(memory_fd, buf, string_size, base_vaddr);
    buf[string_size] = '\0';
    result = str8((U8 *)buf, string_size);
  }
  return result;
}

internal String8
dmn_lnx_read_string(Arena *arena, int memory_fd, U64 vaddr)
{
  return dmn_lnx_read_string_capped(arena, memory_fd, vaddr, 4096);
}

////////////////////////////////
//~ Runtime Struct Helpers

internal B32
dmn_lnx_read_ehdr(int memory_fd, U64 addr, ELF_Hdr64 *ehdr_out)
{
  B32 is_read = 0;
  U8 e_ident[ELF_Identifier_Max] = {0};
  U64 e_ident_size = dmn_lnx_read(memory_fd, r1u64(addr, addr + sizeof(e_ident)), &e_ident);
  if(e_ident_size == sizeof(e_ident))
  {
    if(str8_match(str8_prefix(str8_array_fixed(e_ident), elf_magic_string.size), elf_magic_string, 0))
    {
      switch(e_ident[ELF_Identifier_Class])
      {
      default:{InvalidPath;}break;
      case ELF_Class_None: {}break;
      case ELF_Class_32:
      {
        ELF_Hdr32 ehdr32 = {0};
        if(dmn_lnx_read_struct(memory_fd, addr, &ehdr32))
        {
          *ehdr_out = elf_hdr64_from_hdr32(ehdr32);
          is_read = 1;
        }
      }break;
      case ELF_Class_64:
      {
        is_read = dmn_lnx_read_struct(memory_fd, addr, ehdr_out);
      }break;
      }
    }
  }
  return is_read;
}

internal B32
dmn_lnx_read_phdr(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Phdr64 *phdr_out)
{
  B32 is_read = 0;
  switch (elf_class)
  {
  case ELF_Class_None: break;
  case ELF_Class_32: 
  {
    ELF_Phdr32 phdr32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &phdr32))
    {
      *phdr_out = elf_phdr64_from_phdr32(phdr32);
      is_read = 1;
    }
  }break;
  case ELF_Class_64:
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, phdr_out);
  }break;
  default:{NotImplemented;}break;
  }
  return is_read;
}

internal B32
dmn_lnx_read_shdr(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Shdr64 *shdr_out)
{ 
  B32 is_read = 0;
  switch (elf_class)
  {
  case ELF_Class_None: break;
  case ELF_Class_32: 
  {
    ELF_Shdr32 shdr32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &shdr32))
    {
      *shdr_out = elf_shdr64_from_shdr32(shdr32);
      is_read = 1;
    }
  }break;
  case ELF_Class_64:
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, shdr_out);
  }break;
  default:{NotImplemented;}break;
  }
  return is_read;
}

internal B32
dmn_lnx_read_linkmap(int memory_fd, U64 addr, ELF_Class elf_class, GNU_LinkMap64 *linkmap_out)
{
  B32 is_read = 0;
  switch(elf_class) 
  {
  case ELF_Class_None: {}break;
  case ELF_Class_32:
  {
    // TODO(rjf): endianness
    GNU_LinkMap32 linkmap32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &linkmap32))
    {
      *linkmap_out = gnu_linkmap64_from_linkmap32(linkmap32);
      is_read = 1;
    }
  }break;
  case ELF_Class_64:
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, linkmap_out);
  }break;
  default:{NotImplemented;}break;
  }
  return is_read;
}

internal B32
dmn_lnx_read_dynamic(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Dyn64 *dyn_out)
{
  B32 is_read = 0;
  switch(elf_class)
  {
  case ELF_Class_None:{}break;
  case ELF_Class_32:
  {
    ELF_Dyn32 dyn32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &dyn32))
    {
      *dyn_out = elf_dyn64_from_dyn32(dyn32);
      is_read = 1;
    }
  }break;
  case ELF_Class_64:
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, dyn_out);
  }break;
  default:{NotImplemented;}break;
  }
  return is_read;
}

internal B32
dmn_lnx_read_symbol(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Sym64 *symbol_out)
{
  B32 is_read = 0;
  switch(elf_class)
  {
  case ELF_Class_None:{}break;
  case ELF_Class_32:
  {
    ELF_Sym32 symbol32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &symbol32))
    {
      *symbol_out = elf_sym64_from_sym32(symbol32);
      is_read = 1;
    }
  }break;
  case ELF_Class_64:
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, symbol_out);
  }break;
  default:{NotImplemented;}break;
  }
  return is_read;
}

internal B32
dmn_lnx_read_r_debug(int memory_fd, U64 addr, Arch arch, GNU_RDebugInfo64 *rdebug_out)
{
  B32 is_read = 0;
  switch(gnu_rdebug_info_size_from_arch(arch))
  {
  case 0: {} break;
  case sizeof(GNU_RDebugInfo32): {
    GNU_RDebugInfo32 rdebug32 = {0};
    if(dmn_lnx_read_struct(memory_fd, addr, &rdebug32))
    {
      *rdebug_out = gnu_rdebug_info64_from_rdebug_info32(rdebug32);
      is_read = 1;
    }
  }break;
  case sizeof(GNU_RDebugInfo64):
  {
    is_read = dmn_lnx_read_struct(memory_fd, addr, rdebug_out);
  }break;
  default:{InvalidPath;}break;
  }
  Assert(is_read);
  return is_read;
}

//- rjf: pid => info extraction

internal String8
dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: get exe link path
  String8 exe_link_path = str8f(scratch.arena, "/proc/%d/exe", pid);
  
  //- rjf: read the link
  Temp restore_point = temp_begin(arena);
  B32 good = 0;
  U8 *buffer = 0;
  int readlink_result = 0;
  S64 cap = PATH_MAX;
  for(S64 r = 0; r < 4; cap *= 2, r += 1)
  {
    temp_end(restore_point);
    buffer = push_array_no_zero(arena, U8, cap);
    readlink_result = readlink((char *)exe_link_path.str, (char *)buffer, cap);
    if(readlink_result < cap)
    {
      good = 1;
      break;
    }
  }
  
  //- rjf: package result
  String8 result = {0};
  if(!good || readlink_result == -1)
  {
    temp_end(restore_point);
  }
  else
  {
    arena_pop(arena, (cap - readlink_result - 1));
    result = str8(buffer, readlink_result + 1);
  }
  
  scratch_end(scratch);
  return result;
}

internal ELF_Hdr64
dmn_lnx_ehdr_from_pid(pid_t pid)
{
  Temp scratch = scratch_begin(0, 0);
  B32       is_read  = 0;
  ELF_Hdr64 exe      = {0};
  String8   exe_path = dmn_lnx_exe_path_from_pid(scratch.arena, pid);
  if(exe_path.size != 0)
  {
    int exe_fd = open((char *)exe_path.str, O_RDONLY);
    if(exe_fd != -1)
    {
      is_read = dmn_lnx_read_ehdr(exe_fd, 0, &exe);
      close(exe_fd);
    }   
  }
  Assert(is_read);
  scratch_end(scratch);
  return exe;
}

internal DMN_LNX_ProcessAuxv
dmn_lnx_auxv_from_pid(pid_t pid, ELF_Class elf_class)
{
  Temp scratch = scratch_begin(0, 0);
  DMN_LNX_ProcessAuxv result = {0};
  
  // rjf: open aux data
  String8 auxv_path = push_str8f(scratch.arena, "/proc/%d/auxv", pid);
  int     auxv_fd   = open((char*)auxv_path.str, O_RDONLY);
  
  // rjf: scan aux data
  if(auxv_fd >= 0)
  {
    for(;;)
    {
      // rjf: read next aux
      ELF_Auxv64 auxv = {0};
      switch(elf_class)
      {
      case ELF_Class_None:{}break;
      case ELF_Class_32:
      {
        ELF_Auxv32 auxv32 = {0};
        if(read(auxv_fd, &auxv32, sizeof(auxv32)) != sizeof(auxv32))
        {
          goto brkloop;
        }
        auxv = elf_auxv64_from_auxv32(auxv32);
      }break;
      case ELF_Class_64:
      {
        if(read(auxv_fd, &auxv, sizeof(auxv)) != sizeof(auxv))
        {
          goto brkloop;
        }
      }break;
      default:{NotImplemented;}break;
      }
      
      // rjf: fill result
      switch(auxv.a_type)
      {
        default:{}break;
        case ELF_AuxType_Null:         goto brkloop; break;
        case ELF_AuxType_Base:         result.base   = auxv.a_val; break;
        case ELF_AuxType_Phnum:        result.phnum  = auxv.a_val; break;
        case ELF_AuxType_Phent:        result.phent  = auxv.a_val; break;
        case ELF_AuxType_Phdr:         result.phdr   = auxv.a_val; break;
        case ELF_AuxType_ExecFn:       result.execfn = auxv.a_val; break;
        case ELF_AuxType_Pagesz:       result.pagesz = auxv.a_val; break;
      }
    }
    brkloop:;
    close(auxv_fd);
  }
  
  scratch_end(scratch);
  return result;
}

internal DMN_LNX_PhdrInfo
dmn_lnx_phdr_info_from_memory(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum)
{
  DMN_LNX_PhdrInfo result = { .range.min = max_U64 };
  
  // rjf: scan table
  for(U64 ph_cursor = e_phaddr, ph_opl = (e_phaddr + e_phentsize * e_phnum);
      ph_cursor < ph_opl;
      ph_cursor += e_phentsize)
  {
    ELF_Phdr64 phdr = {0};
    if(!dmn_lnx_read_phdr(memory_fd, ph_cursor, elf_class, &phdr))
    {
      Assert(0 && "unable to read a program header");
    }

    // rjf: save
    switch(phdr.p_type)
    {
      default:{}break;
      case ELF_PType_Dynamic:
      {
        result.dynamic = rebase + phdr.p_vaddr;
      }break;
      case ELF_PType_Load:
      {
        U64 min = rebase + phdr.p_vaddr;
        U64 max = rebase + phdr.p_vaddr + phdr.p_memsz;
        result.range.min = Min(result.range.min, min);
        result.range.max = Max(result.range.max, max);
      }break;
    }
  }
  
  return result;
}

internal DMN_LNX_DynamicInfo
dmn_lnx_dynamic_info_from_memory(int memory_fd, ELF_Class elf_class, U64 rebase, U64 dynamic_vaddr)
{
  DMN_LNX_DynamicInfo dynamic_info = {0};
  for(U64 dynamic_cursor = dynamic_vaddr; ; dynamic_cursor += elf_dyn_size_from_class(elf_class))
  {
    // rjf: read next dyn entry
    ELF_Dyn64 dyn = {0};
    if(!dmn_lnx_read_dynamic(memory_fd, dynamic_cursor, elf_class, &dyn)) { Assert(0 && "unable to read dynamic"); }

    // rjf: break on zero
    if(dyn.tag == ELF_DynTag_Null) { break; }

    // extract reuiqred values out of dynamic section
    if(dyn.tag == ELF_DynTag_Strtab)
    {
      dynamic_info.strtab_vaddr = rebase + dyn.val;
    }
    else if(dyn.tag == ELF_DynTag_Strsz)
    {
      dynamic_info.strtab_size = dyn.val;
    }
    else if(dyn.tag == ELF_DynTag_Symtab)
    {
      dynamic_info.symtab_vaddr = rebase + dyn.val;
    }
    else if(dyn.tag == ELF_DynTag_Syment)
    {
      dynamic_info.symtab_entry_size = dyn.val;
    }
    else if(dyn.tag == ELF_DynTag_Hash)
    {
      dynamic_info.hash_vaddr = rebase + dyn.val;
    }
    else if(dyn.tag == ELF_DynTag_GNU_Hash)
    {
      dynamic_info.gnu_hash_vaddr = rebase + dyn.val;
    }
  }
  return dynamic_info;
}

internal U64
dmn_lnx_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vbase)
{
  Temp scratch = scratch_begin(0, 0);

  U64 rdebug_vaddr = 0;

  // load DL's header
  ELF_Hdr64 ehdr = {0};
  if(!dmn_lnx_read_ehdr(memory_fd, loader_vbase, &ehdr)) { Assert(0 && "failed to read interp's header"); goto exit; }

  U64       rebase    = ehdr.e_type == ELF_Type_Dyn ? loader_vbase : 0;
  ELF_Class elf_class = ehdr.e_ident[ELF_Identifier_Class];

  // find dynamic program header
  U64 dynamic_vaddr = max_U64;
  for EachIndex(phdr_idx, ehdr.e_phnum)
  {
    U64 phdr_vaddr = loader_vbase + ehdr.e_phoff + phdr_idx * ehdr.e_phentsize;

    ELF_Phdr64 phdr = {0};
    if(!dmn_lnx_read_phdr(memory_fd, phdr_vaddr, elf_class, &phdr)) { Assert(0 && "failed to read program header"); goto exit; }

    if(phdr.p_type == ELF_PType_Dynamic)
    {
      dynamic_vaddr = rebase + phdr.p_offset;
      break;
    }
  }

  // extract necessary info out of dynamic program header
  DMN_LNX_DynamicInfo dynamic_info = dmn_lnx_dynamic_info_from_memory(memory_fd, elf_class, rebase, dynamic_vaddr);

  // extract symbol table count from available options
  U64 symbol_count = 0;
  if(dynamic_info.hash_vaddr)
  {
    U64 hash_entry_size = 4;
    if(elf_class == ELF_Class_64 && (ehdr.e_machine == ELF_MachineKind_ALPHA || ehdr.e_machine == ELF_MachineKind_S390 || ehdr.e_machine == ELF_MachineKind_S390_OLD))
    {
      hash_entry_size = 8;
    }

    U64 chain_count = 0;
    if(dmn_lnx_read(memory_fd, r1u64(dynamic_info.hash_vaddr, dynamic_info.hash_vaddr + hash_entry_size), &chain_count) == hash_entry_size)
    {
      symbol_count = chain_count;
    }
    else
    {
      Assert(0 && "failed to read hash table's chain count out of HASH");
    }
  }
  else
  {
    // TODO: extract count from GNU_HASH
    NotImplemented;
  }

  // scan symbol table for the rendezvous symbol
  if(dynamic_info.symtab_vaddr && dynamic_info.symtab_entry_size && symbol_count)
  {
    for EachIndex(symbol_idx, symbol_count)
    {
      ELF_Sym64 symbol = {0};
      if(!dmn_lnx_read_symbol(memory_fd, dynamic_info.symtab_vaddr + symbol_idx * dynamic_info.symtab_entry_size, elf_class, &symbol))
      {
        Assert(0 && "failed to read symbol table");
        break;
      }

      Temp temp = temp_begin(scratch.arena);

      String8 symbol_name = {0};
      if(symbol.st_name < dynamic_info.strtab_size)
      {
        U64 cap = dynamic_info.strtab_size - symbol.st_name;
        symbol_name = dmn_lnx_read_string_capped(temp.arena, memory_fd, dynamic_info.strtab_vaddr + symbol.st_name, cap);
      }

      if(str8_match(symbol_name, str8_lit("_r_debug"), 0))
      {
        ELF_SymType symbol_type = ELF_ST_TYPE(symbol.st_info);
        if(symbol_type == ELF_SymType_Object && symbol.st_size > 0)
        {
          rdebug_vaddr = rebase + symbol.st_value;
          break;
        }
      }

      temp_end(temp);
    }
  }
  
exit:;
  scratch_end(scratch);
  return rdebug_vaddr;
}

////////////////////////////////
//~ SDT Probes

internal DMN_LNX_ProbeList
dmn_lnx_read_probes(Arena *arena, int fd, U64 offset, U64 image_base)
{
  Temp scratch = scratch_begin(&arena, 1);

  DMN_LNX_ProbeList probes = {0};

  ELF_Hdr64 ehdr = {0};
  if(!dmn_lnx_read_ehdr(fd, offset, &ehdr)) { goto exit; }

  U64        strtab_shdr_offset = offset + ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize;
  ELF_Shdr64 strtab_shdr        = {0};
  if(!dmn_lnx_read_shdr(fd, strtab_shdr_offset, ehdr.e_ident[ELF_Identifier_Class], &strtab_shdr)) { goto exit; }

  B32 found_probes      = 0;
  B32 found_probes_base = 0;
  ELF_Shdr64 text_shdr         = {0};
  ELF_Shdr64 stapsdt_base_shdr = {0};
  ELF_Shdr64 stapsdt_shdr      = {0};
  for(U64 shdr_off = offset + ehdr.e_shoff, shdr_opl = shdr_off + ehdr.e_shentsize * ehdr.e_shnum;
       shdr_off < shdr_opl;
       shdr_off += ehdr.e_shentsize) {
    ELF_Shdr64 shdr = {0};
    if(!dmn_lnx_read_shdr(fd, shdr_off, ehdr.e_ident[ELF_Identifier_Class], &shdr)) { goto exit; }

    if(shdr.sh_type == ELF_ShType_Note)
    {
      U64     name_offset = offset + strtab_shdr.sh_offset + shdr.sh_name;
      U64     name_cap    = offset + strtab_shdr.sh_offset + strtab_shdr.sh_size;
      String8 name        = dmn_lnx_read_string_capped(scratch.arena, fd, name_offset, name_cap);

      if(str8_match(name, str8_lit(".note.stapsdt"), 0))
      {
        stapsdt_shdr = shdr;
        found_probes = 1;
      }
    }
    else if(shdr.sh_type == ELF_ShType_ProgBits)
    {
      U64     name_offset = offset + strtab_shdr.sh_offset + shdr.sh_name;
      U64     name_cap    = offset + strtab_shdr.sh_offset + strtab_shdr.sh_size;
      String8 name        = dmn_lnx_read_string_capped(scratch.arena, fd, name_offset, name_cap);

      if(str8_match(name, str8_lit(".stapsdt.base"), 0))
      {
        stapsdt_base_shdr = shdr;
        found_probes_base = 1;
      } else if(str8_match(name, str8_lit(".text"), 0))
      {
        text_shdr = shdr;
      }
    }

    if(found_probes && found_probes_base) { break; }
  }

  if(!found_probes || !found_probes_base) { goto exit; }

  U64 probes_base = stapsdt_base_shdr.sh_addr;
  
  Rng1U64  note_range     = shift_1u64(r1u64(stapsdt_shdr.sh_offset, stapsdt_shdr.sh_offset + stapsdt_shdr.sh_size), offset);
  void    *raw_note       = push_array(arena, U8, stapsdt_shdr.sh_size);
  U64      note_read_size = dmn_lnx_read(fd, note_range, raw_note);
  if(note_read_size != dim_1u64(note_range)) { goto exit; }

  Arch         arch = arch_from_elf_machine(ehdr.e_machine);
  ELF_NoteList note = elf_parse_note(scratch.arena, str8(raw_note, dim_1u64(note_range)), ehdr.e_ident[ELF_Identifier_Class], ehdr.e_machine);

  for EachNode(n, ELF_NoteNode, note.first)
  {
    ELF_Note *note = &n->v;
    if(!str8_match(note->owner, str8_lit("stapsdt"), 0)) { continue; }
    if(note->type != ELF_NoteType_STapSdt)               { continue; }

    DMN_LNX_Probe probe = {0};
    {
      U64 cursor    = 0;
      U64 addr_size = ehdr.e_ident[ELF_Identifier_Class] == ELF_Class_64 ? 8 : 4;

      U64 pc = 0;
      U64 pc_size = str8_deserial_read(note->desc, cursor, &pc, addr_size, addr_size);
      if (pc_size == 0) { goto exit; }
      cursor += pc_size;

      U64 base_addr = 0;
      U64 base_addr_size = str8_deserial_read(note->desc, cursor, &base_addr, addr_size, addr_size);
      if (base_addr_size == 0) { goto exit; }
      cursor += base_addr_size;

      U64 semaphore = 0;
      U64 semaphore_size = str8_deserial_read(note->desc, cursor, &semaphore, addr_size, addr_size);
      if (semaphore_size == 0) { goto exit; }
      cursor += semaphore_size;

      String8 provider = str8_cstring_capped(note->desc.str + cursor, note->desc.str + note->desc.size);
      cursor += provider.size + 1;
      if (cursor > note->desc.size) { goto exit; }

      String8 name = str8_cstring_capped(note->desc.str + cursor, note->desc.str + note->desc.size);
      cursor += name.size + 1;
      if (cursor > note->desc.size) { goto exit; }

      String8 args = str8_cstring_capped(note->desc.str + cursor, note->desc.str + note->desc.size);
      cursor += args.size + 1;
      if (cursor > note->desc.size) { goto exit; }

      U64 probe_rebase = image_base + (base_addr - probes_base);

      probe.provider  = provider;
      probe.name      = name;
      probe.args      = stap_arg_array_from_string(arena, arch, args);
      probe.pc        = pc + probe_rebase;
      probe.semaphore = semaphore ? semaphore + probe_rebase : 0;
    }

    DMN_LNX_ProbeNode *n = push_array(arena, DMN_LNX_ProbeNode, 1);
    n->v = probe;
    SLLQueuePush(probes.first, probes.last, n);
    probes.count += 1;
  }

exit:;
  scratch_end(scratch);
  return probes;
}

////////////////////////////////
//~ STAP

internal
STAP_MEMORY_READ(dmn_lnx_stap_memory_read)
{
  DMN_LNX_Entity *process = raw_ctx;
  U64 bytes_read = dmn_lnx_read(process->fd, r1u64(addr, addr + read_size), buffer);
  return bytes_read == read_size;
}

////////////////////////////////
//~ rjf: Entity Functions

internal DMN_LNX_Entity *
dmn_lnx_entity_alloc(DMN_LNX_Entity *parent, DMN_LNX_EntityKind kind)
{
  DMN_LNX_Entity *entity = dmn_lnx_state->free_entity;
  if(entity != 0)
  {
    SLLStackPop(dmn_lnx_state->free_entity);
  }
  else
  {
    entity = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 1);
    dmn_lnx_state->entities_count += 1;
  }
  U32 gen = entity->gen;
  MemoryCopyStruct(entity, &dmn_lnx_nil_entity);
  entity->gen += 1;
  if(parent != &dmn_lnx_nil_entity)
  {
    DLLPushBack_NPZ(&dmn_lnx_nil_entity, parent->first, parent->last, entity, next, prev);
    entity->parent = parent;
  }
  entity->kind = kind;
  return entity;
}

internal void
dmn_lnx_entity_release(DMN_LNX_Entity *entity)
{
  if(entity->parent != &dmn_lnx_nil_entity)
  {
    DLLRemove_NPZ(&dmn_lnx_nil_entity, entity->parent->first, entity->parent->last, entity, next, prev);
    entity->parent = &dmn_lnx_nil_entity;
  }
  {
    Temp scratch = scratch_begin(0, 0);
    DMN_LNX_EntityNode start_task = {0, entity};
    DMN_LNX_EntityNode *first_task = &start_task;
    for(DMN_LNX_EntityNode *t = first_task; t != 0; t = t->next)
    {
      SLLStackPush(dmn_lnx_state->free_entity, t->v);
      for(DMN_LNX_Entity *child = t->v->first; child != &dmn_lnx_nil_entity; child = child->next)
      {
        DMN_LNX_EntityNode *task = push_array(scratch.arena, DMN_LNX_EntityNode, 1);
        task->next = t->next;
        t->next = task;
        task->v = child;
      }
    }
    scratch_end(scratch);
  }
}

internal DMN_Handle
dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity)
{
  DMN_Handle handle = {0};
  U64 index = (U64)(entity - dmn_lnx_state->entities_base);
  if(index <= 0xffffffffu)
  {
    handle.u32[0] = index;
    handle.u32[1] = entity->gen;
  }
  return handle;
}

internal DMN_LNX_Entity *
dmn_lnx_entity_from_handle(DMN_Handle handle)
{
  DMN_LNX_Entity *result = &dmn_lnx_nil_entity;
  U64 index = (U64)handle.u32[0];
  if(index < dmn_lnx_state->entities_count &&
     dmn_lnx_state->entities_base[index].gen == handle.u32[1])
  {
    result = &dmn_lnx_state->entities_base[index];
  }
  return result;
}

internal DMN_LNX_Entity *
dmn_lnx_thread_from_pid(pid_t pid)
{
  DMN_LNX_Entity *result = &dmn_lnx_nil_entity;
  if(pid != 0)
  {
    for EachIndex(idx, dmn_lnx_state->entities_count)
    {
      if(dmn_lnx_state->entities_base[idx].kind == DMN_LNX_EntityKind_Thread && (pid_t)dmn_lnx_state->entities_base[idx].id == pid)
      {
        result = &dmn_lnx_state->entities_base[idx];
        break;
      }
    }
  }
  return result;
}

internal U64
dmn_lnx_thread_read_ip(DMN_LNX_Entity *thread)
{
  U64 ip = 0;
  if(thread->reg_block)
  {
    ip = regs_rip_from_arch_block(thread->arch, thread->reg_block);
  }
  Assert(ip);
  return ip;
}

internal U64
dmn_lnx_thread_read_sp(DMN_LNX_Entity *thread)
{
  U64 sp = 0;
  if(thread->reg_block)
  {
    sp = regs_rsp_from_arch_block(thread->arch, thread->reg_block);
  }
  Assert(sp);
  return sp;
}

internal B32
dmn_lnx_thread_write_ip(DMN_LNX_Entity *thread, U64 ip)
{
  B32 is_ip_written = 0;
  if(thread->reg_block)
  {
    REGS_RegBlockX64 *reg_block = thread->reg_block;
    regs_arch_block_write_rip(thread->arch, reg_block, ip);
    is_ip_written = 1;
  }
  Assert(is_ip_written);
  return is_ip_written;
}

internal B32
dmn_lnx_thread_write_sp(DMN_LNX_Entity *thread, U64 sp)
{
  B32 is_sp_written = 0;
  if(thread->reg_block)
  {
    REGS_RegBlockX64 *reg_block = thread->reg_block;
    regs_arch_block_write_rsp(thread->arch, reg_block, sp);
    is_sp_written = 1;
  }
  Assert(is_sp_written);
  return is_sp_written;
}

internal B32
dmn_lnx_thread_read_reg_block(DMN_LNX_Entity *thread, void *reg_block)
{
  AssertAlways(gettid() == thread->parent->tracer_tid);

  B32 result = 0;
  switch(thread->arch)
  {
    case Arch_Null:
    case Arch_COUNT:{}break;
    case Arch_x86:
    case Arch_arm64:
    case Arch_arm32:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: [x64]
    //
    case Arch_x64:
    {
      DMN_LNX_Entity   *process = thread->parent;
      pid_t             tid     = (pid_t)thread->id;
      REGS_RegBlockX64 *dst     = reg_block;
      
      //- rjf: read GPR
      B32 got_gpr = 0;
      {
        DMN_LNX_UserX64 ctx = {0};
        int ptrace_result = ptrace(PTRACE_GETREGSET, tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_len = sizeof(ctx), .iov_base = &ctx });
        if(ptrace_result != -1)
        {
          got_gpr = 1;
          DMN_LNX_UserX64 *src = &ctx;
          dst->rax.u64    = src->regs.rax;
          dst->rcx.u64    = src->regs.rcx;
          dst->rdx.u64    = src->regs.rdx;
          dst->rbx.u64    = src->regs.rbx;
          dst->rsp.u64    = src->regs.rsp;
          dst->rbp.u64    = src->regs.rbp;
          dst->rsi.u64    = src->regs.rsi;
          dst->rdi.u64    = src->regs.rdi;
          dst->r8.u64     = src->regs.r8;
          dst->r9.u64     = src->regs.r9;
          dst->r10.u64    = src->regs.r10;
          dst->r11.u64    = src->regs.r11;
          dst->r12.u64    = src->regs.r12;
          dst->r13.u64    = src->regs.r13;
          dst->r14.u64    = src->regs.r14;
          dst->r15.u64    = src->regs.r15;
          dst->cs.u16     = src->regs.cs;
          dst->ds.u16     = src->regs.ds;
          dst->es.u16     = src->regs.es;
          dst->fs.u16     = src->regs.fs;
          dst->gs.u16     = src->regs.gs;
          dst->ss.u16     = src->regs.ss;
          dst->fsbase.u64 = src->regs.fsbase;
          dst->gsbase.u64 = src->regs.gsbase;
          dst->rip.u64    = src->regs.rip;
          dst->rflags.u64 = src->regs.rflags;
        }
        else { Assert(0 && "failed to get gprs"); }
      }
      
      //- rjf: read FPR
      B32 got_fpr = 0;
      if(got_gpr)
      {
        Temp scratch = scratch_begin(0, 0);

        X64_XSave  *xsave  = 0;
        X64_FXSave *fxsave = 0;
        
        // get xsave
        if(x64_is_xsave_supported())
        {
          void *xsave_raw     = push_array(scratch.arena, U8, process->xsave_size);
          int   ptrace_result = ptrace(PTRACE_GETREGSET, tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_len = process->xsave_size, .iov_base = xsave_raw });
          if(ptrace_result != -1)
          {
            xsave  = xsave_raw;
            fxsave = &xsave->fxsave;
          }
          else { Assert(0 && "failed to get xsave"); }
        }
        
        // get fxsave
        if (fxsave == 0)
        {
          fxsave = push_array(scratch.arena, X64_FXSave, 1);
          int ptrace_result = ptrace(PTRACE_GETREGSET, tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_len = sizeof(*fxsave), .iov_base = fxsave });
          if(ptrace_result != -1)
          {
            fxsave = 0;
          }
          else { Assert(0 && "failed to get fxsave"); }
        }
        
        // copy fxsave registers
        if(fxsave)
        {
          X64_FXSave *src = fxsave;

          // copy x87 registers
          dst->fcw.u16        = src->fcw;
          dst->fsw.u16        = src->fsw;
          dst->ftw.u16        = x64_xsave_tag_word_from_real_tag_word(src->ftw);
          dst->fop.u16        = src->fop;
          dst->fip.u64        = src->b64.fip;
          dst->fdp.u64        = src->b64.fdp;
          dst->mxcsr.u32      = src->mxcsr;
          dst->mxcsr_mask.u32 = src->mxcsr_mask;
          for EachIndex(i, 8)
          {
            MemoryCopy(&dst->st0 + i, src->st_space + i, sizeof(REGS_Reg80));
          }

          // SSE registers are always available in x64
          {
            U128        *xmm_d = fxsave->xmm_space;
            REGS_Reg512 *zmm_s = &dst->zmm0;
            for EachIndex(i, 16)
            {
              MemoryCopy(&zmm_s[i], &xmm_d[i], sizeof(*xmm_d));
            }
          }
        }

        // copy xsave registers
        if(xsave)
        {
          if(xsave->header.xstate_bv & X64_XStateComponentFlag_AVX)
          {
            AssertAlways(process->xsave_layout.avx_offset + 16*sizeof(REGS_Reg128) <= process->xsave_size);
            REGS_Reg128 *avx_s = (REGS_Reg128 *)((U8 *)xsave + process->xsave_layout.avx_offset);
            REGS_Reg512 *zmm_d = &dst->zmm0;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmm_d[n].v[16], &avx_s[n], sizeof(REGS_Reg128));
            }
          }

          if(xsave->header.xstate_bv & X64_XStateComponentFlag_OPMASK)
          {
            AssertAlways(process->xsave_layout.opmask_offset + sizeof(REGS_Reg64) * 8 <= process->xsave_size);
            REGS_Reg64 *kmask_s = (REGS_Reg64 *)((U8 *)xsave + process->xsave_layout.opmask_offset);
            REGS_Reg64 *kmask_d = &dst->k0;
            for EachIndex(n, 8)
            {
              MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(REGS_Reg64));
            }
          }

          if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM_H)
          {
            AssertAlways(process->xsave_layout.zmm_h_offset + sizeof(REGS_Reg256) * 16 <= process->xsave_size);
            REGS_Reg256 *avx512h_s = (REGS_Reg256 *)((U8 *)xsave + process->xsave_layout.zmm_h_offset);
            REGS_Reg512 *zmmh_d    = &dst->zmm0;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmmh_d[n].v[32], &avx512h_s[n], sizeof(REGS_Reg256));
            }
          }

          if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM)
          {
            AssertAlways(process->xsave_layout.zmm_offset + sizeof(REGS_Reg512) * 16 <= process->xsave_size);
            REGS_Reg512 *avx512_s = (REGS_Reg512 *)((U8 *)xsave + process->xsave_layout.zmm_offset);
            REGS_Reg512 *zmm_d    = &dst->zmm16;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmm_d[n], &avx512_s[n], sizeof(REGS_Reg512));
            }
          }

          if(xsave->header.xstate_bv & X64_XStateComponentFlag_CETU)
          {
            AssertAlways(process->xsave_layout.cet_u_offset + sizeof(U64)*2 <= process->xsave_size);
            U64 *cet_u = (U64 *)((U8 *)xsave + process->xsave_layout.cet_u_offset);
            dst->cetmsr.u64 = cet_u[0];
            dst->cetssp.u64 = cet_u[1];
          }
        }
        
        got_fpr = (xsave || fxsave);
        scratch_end(scratch);
      }
      
      //- rjf: read debug registers
      B32 got_debug = 0;
      if(got_fpr)
      {
        got_debug = 1;
        REGS_Reg64 *dr_d = &dst->dr0;
        for EachIndex(n, 8)
        {
          if(n != 4 && n != 5)
          {
            U64 offset = OffsetOf(DMN_LNX_UserX64, u_debugreg[n]);
            errno = 0;
            long peek_result = ptrace(PTRACE_PEEKUSER, tid, PtrFromInt(offset), 0);
            if(errno == 0)
            {
              dr_d[n].u64 = (U64)peek_result;
            }
            else
            {
              got_debug = 0;
            }
          }
        }
      }
      
      result = got_debug;
    }break;
  }
  return result;
}

internal B32
dmn_lnx_thread_write_reg_block(DMN_LNX_Entity *thread, void *reg_block)
{
  AssertAlways(gettid() == thread->parent->tracer_tid);

  B32 result = 0;
  switch(thread->arch)
  {
    case Arch_Null:
    case Arch_COUNT:{}break;
    case Arch_arm64:
    case Arch_arm32:
    case Arch_x86:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: [x64]
    //
    case Arch_x64:
    {
      DMN_LNX_Entity   *process = thread->parent;
      pid_t             tid     = (pid_t)thread->id;
      REGS_RegBlockX64 *src     = reg_block;
      
      //- rjf: write GPR
      B32 did_gpr = 0;
      {
        DMN_LNX_UserX64 dst = {0};
        dst.regs.rax    = src->rax.u64;
        dst.regs.rcx    = src->rcx.u64;
        dst.regs.rdx    = src->rdx.u64;
        dst.regs.rbx    = src->rbx.u64;
        dst.regs.rsp    = src->rsp.u64;
        dst.regs.rbp    = src->rbp.u64;
        dst.regs.rsi    = src->rsi.u64;
        dst.regs.rdi    = src->rdi.u64;
        dst.regs.r8     = src->r8.u64;
        dst.regs.r9     = src->r9.u64;
        dst.regs.r10    = src->r10.u64;
        dst.regs.r11    = src->r11.u64;
        dst.regs.r12    = src->r12.u64;
        dst.regs.r13    = src->r13.u64;
        dst.regs.r14    = src->r14.u64;
        dst.regs.r15    = src->r15.u64;
        dst.regs.cs     = src->cs.u16;
        dst.regs.ds     = src->ds.u16;
        dst.regs.es     = src->es.u16;
        dst.regs.fs     = src->fs.u16;
        dst.regs.gs     = src->gs.u16;
        dst.regs.ss     = src->ss.u16;
        dst.regs.fsbase = src->fsbase.u64;
        dst.regs.gsbase = src->gsbase.u64;
        dst.regs.rip    = src->rip.u64;
        dst.regs.rflags = src->rflags.u64;
        did_gpr = ptrace(PTRACE_SETREGSET, tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_base = &dst, .iov_len = sizeof(dst) }) >= 0;
      }
      
      B32 did_fpr = 0;
      if(did_gpr)
      {
        Temp scratch = scratch_begin(0, 0);

        int xsave_result  = -1;
        int fxsave_result = -1;

        X64_FXSave dst_fxsave = {0};
        {
          dst_fxsave.fcw          = src->fcw.u16;
          dst_fxsave.fsw          = src->fsw.u16;
          dst_fxsave.ftw          = src->ftw.u16;
          dst_fxsave.fop          = src->fop.u16;
          dst_fxsave.b64.fip      = src->fip.u64;
          dst_fxsave.b64.fdp      = src->fdp.u64;
          dst_fxsave.mxcsr        = src->mxcsr.u32;
          dst_fxsave.mxcsr_mask   = src->mxcsr_mask.u32;

          REGS_Reg128 *st_d = (REGS_Reg128 *)dst_fxsave.st_space;
          REGS_Reg80  *st_s = &src->st0;
          for EachIndex(n, 8)
          {
            MemoryCopy(&st_d[n], &st_s[n], sizeof(REGS_Reg80));
          }

          REGS_Reg128 *xmm_d = (REGS_Reg128 *)dst_fxsave.xmm_space;
          REGS_Reg512 *xmm_s = &src->zmm0;
          for EachIndex(n, 16)
          {
            MemoryCopy(&xmm_d[n], &xmm_s[n], sizeof(REGS_Reg128));
          }
        }

        if(x64_is_xsave_supported())
        {
          U8  *xsave_raw = push_array(scratch.arena, U8, process->xsave_size);
          int  xsave_get = ptrace(PTRACE_GETREGSET, tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_base = xsave_raw, .iov_len = process->xsave_size });
          AssertAlways(xsave_get >= 0);

          X64_XSave *dst = (X64_XSave *)xsave_raw;
          dst->fxsave = dst_fxsave;

          if(process->xsave_layout.avx_offset)
          {
            if(process->xsave_layout.avx_offset + sizeof(REGS_Reg128) * 16 <= process->xsave_size)
            {
              REGS_Reg128 *avx_d = (REGS_Reg128 *)(xsave_raw + process->xsave_layout.avx_offset);
              REGS_Reg512 *zmm_s = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx_d[n], &zmm_s[n].v[16], sizeof(REGS_Reg128));
              }
            }
          }

          if(process->xsave_layout.opmask_offset)
          {
            if(process->xsave_layout.opmask_offset + sizeof(REGS_Reg64) * 8 <= process->xsave_size)
            {
              REGS_Reg64 *kmask_d = (REGS_Reg64 *)(xsave_raw + process->xsave_layout.opmask_offset);
              REGS_Reg64 *kmask_s = &src->k0;
              for EachIndex(n, 8)
              {
                MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(REGS_Reg64));
              }
            }
            else { Assert(0 && "invalid xsave size"); }
          }

          if(process->xsave_layout.zmm_h_offset)
          {
            if(process->xsave_layout.zmm_h_offset + sizeof(REGS_Reg256) * 16 <= process->xsave_size)
            {
              REGS_Reg256 *avx512h_d = (REGS_Reg256 *)(xsave_raw + process->xsave_layout.zmm_h_offset);
              REGS_Reg512 *zmmh_s    = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512h_d[n], &zmmh_s[n].v[32], sizeof(REGS_Reg256));
              }
            }
            else { Assert(0 && "invalid xsave size"); }
          }

          if(process->xsave_layout.zmm_offset)
          {
            if(process->xsave_layout.zmm_offset + sizeof(REGS_Reg512) * 16 <= process->xsave_size)
            {
              REGS_Reg512 *avx512_d = (REGS_Reg512 *)(xsave_raw + process->xsave_layout.zmm_offset);
              REGS_Reg512 *zmm_s    = &src->zmm16;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512_d[n], &zmm_s[n], sizeof(REGS_Reg512));
              }
            }
            else { Assert(0 && "invalid xsave size"); }
          }

          if(process->xsave_layout.cet_u_offset)
          {
            if(process->xsave_layout.cet_u_offset + sizeof(REGS_Reg64) * 2 <= process->xsave_size)
            {
              REGS_Reg64 *cet_u = (REGS_Reg64 *)(xsave_raw + process->xsave_layout.cet_u_offset);
              cet_u[0] = src->cetmsr;
              cet_u[1] = src->cetssp;
            }
            else { Assert(0 && "invalid xsave size"); }
          }

          // xsave
          xsave_result = ptrace(PTRACE_SETREGSET, tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_base = dst, .iov_len = process->xsave_size });
          Assert(xsave_result >= 0);
        }

        // fallback to fxsave
        if(xsave_result < 0)
        {
          fxsave_result = ptrace(PTRACE_SETREGSET, tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_base = &dst_fxsave, sizeof(dst_fxsave) });
          Assert(fxsave_result >= 0);
        }

        // rjf: good finish requires xsave or fxsave
        did_fpr = (xsave_result >= 0 || fxsave_result >= 0);

        scratch_end(scratch);
      }
      
      //- rjf: write debug registers
      B32 did_dbg = 0;
      if(did_fpr)
      {
        did_dbg = 1;

        src->dr7.u64 |= (1 << 10);

        REGS_Reg64 *dr_s = &src->dr0;
        for EachIndex(n, 8)
        {
          if(n != 4 && n != 5)
          {
            U64 offset = OffsetOf(DMN_LNX_UserX64, u_debugreg[n]);
            int poke_result = ptrace(PTRACE_POKEUSER, tid, PtrFromInt(offset), dr_s[n].u64);
            if(poke_result < 0)
            {
              did_dbg = 0;
              break;
            }
          }
        }
      }
      
      result = (did_dbg);
    }break;
  }
  return result;
}

internal B32
dmn_lnx_set_single_step_flag(DMN_LNX_Entity *thread, B32 is_on)
{
  B32 is_flag_set = 0;
  switch(thread->arch)
  {
  case Arch_COUNT:
  case Arch_Null: {} break;
  case Arch_x64:
  {
    REGS_RegBlockX64 *reg_block = thread->reg_block;
    if(is_on)
    {
      reg_block->rflags.u64 |= X64_RFlag_Trap;
    }
    else
    {
      reg_block->rflags.u64 &= ~X64_RFlag_Trap;
    }
    is_flag_set = 1;
  }break;
  case Arch_x86:
  case Arch_arm32:
  case Arch_arm64:
  {
    NotImplemented;
  }break;
  }
  return is_flag_set;
}

internal void
dmn_lnx_process_loaded_modules(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, U64 name_space_id, U64 new_link_map_vaddr)
{
  GNU_LinkMap64 map = {0};
  for(U64 map_vaddr = new_link_map_vaddr; map_vaddr != 0; map_vaddr = map.next_vaddr)
  {
    // read out new link map item
    if(!dmn_lnx_read_linkmap(process->fd, map_vaddr, process->dl_class, &map)) { goto exit; }

    // was module with this base already registered?
    DMN_LNX_Entity *module = hash_table_search_u64_raw(process->loaded_modules_ht, map.addr_vaddr);
    if(module) { continue; }

    // parse out module's ELF header
    ELF_Hdr64 module_ehdr = {0};
    if(!dmn_lnx_read_ehdr(process->fd, map.addr_vaddr, &module_ehdr)) { goto exit; }

    // gather info about module
    U64              module_rebase     = module_ehdr.e_type == ELF_Type_Dyn ? map.addr_vaddr : 0;
    U64              module_phdr_vaddr = module_rebase + module_ehdr.e_phoff;
    DMN_LNX_PhdrInfo module_phdr_info  = dmn_lnx_phdr_info_from_memory(process->fd, module_ehdr.e_ident[ELF_Identifier_Class], module_rebase, module_phdr_vaddr, module_ehdr.e_phentsize, module_ehdr.e_phnum);
    String8          module_name       = dmn_lnx_read_string(process->arena, process->fd, map.name_vaddr);

    // fill out module
    module             = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
    module->id         = map.name_vaddr;
    module->base_vaddr = map.addr_vaddr;

    // push load event
    if(!str8_match(module_name, str8_lit("linux-vdso.so.1"), 0))
    {
      DMN_Event *e = dmn_event_list_push(arena, events);
      e->kind             = DMN_EventKind_LoadModule;
      e->process          = dmn_lnx_handle_from_entity(process);
      e->module           = dmn_lnx_handle_from_entity(module);
      e->arch             = arch_from_elf_machine(module_ehdr.e_machine);
      e->address          = map.addr_vaddr;
      e->size             = dim_1u64(module_phdr_info.range);
      e->string           = module_name;
      e->elf_phdr_vrange  = r1u64(module_phdr_vaddr, module_phdr_vaddr + module_ehdr.e_phentsize * module_ehdr.e_phnum);
      e->elf_phdr_entsize = module_ehdr.e_phentsize;
    }

    // create mapping for base -> module
    hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, map.addr_vaddr, module);
  }

  exit:;
}

internal void
dmn_lnx_process_unloaded_modules(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, U64 name_space_id, U64 rdebug_vaddr)
{
  Temp scratch = scratch_begin(&arena, 1);
  B32 is_unmap_complete_finished = 0;

  GNU_RDebugInfo64 rdebug = {0};
  if(!dmn_lnx_read_r_debug(process->fd, rdebug_vaddr, process->arch, &rdebug)) { goto exit; }
  if(rdebug.r_version != 1) { goto exit; }

  // flag every module as inactive
  for(DMN_LNX_Entity *module = process->first; module != &dmn_lnx_nil_entity; module = module->next)
  {
    if(module->kind != DMN_LNX_EntityKind_Module) {continue;}
    module->is_live = 0;
  }

  // loop over modules in the link map and mark live modules
  GNU_LinkMap64 map = {0};
  for(U64 map_vaddr = rdebug.r_map; map_vaddr != 0; map_vaddr = map.next_vaddr)
  {
    if(dmn_lnx_read_linkmap(process->fd, map_vaddr, process->dl_class, &map))
    {
      DMN_LNX_Entity *module = hash_table_search_u64_raw(process->loaded_modules_ht, map.addr_vaddr);
      if(module)
      {
        module->is_live = 1;
      }
      else { Assert(0 && "unknown module is being unloaded"); }
    }
    else { Assert(0 && "unable to read Link Map"); }
  }

  // unload inactive modules
  DMN_HandleList to_release = {0};
  for(DMN_LNX_Entity *module = process->first; module != &dmn_lnx_nil_entity; module = module->next)
  {
    if(module->kind != DMN_LNX_EntityKind_Module) {continue;}
    if(module->is_live)                           {continue;}
    dmn_handle_list_push(scratch.arena, &to_release, dmn_lnx_handle_from_entity(module));
  }

  // push events and clean up internal structures
  for EachNode(n, DMN_HandleNode, to_release.first)
  {
    DMN_LNX_Entity *module = dmn_lnx_entity_from_handle(n->v);

    DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
    e->kind    = DMN_EventKind_UnloadModule;
    e->process = dmn_lnx_handle_from_entity(process);
    e->module  = dmn_lnx_handle_from_entity(module);
    e->string  = dmn_lnx_read_string(arena, process->fd, module->id);

    hash_table_purge_u64(process->loaded_modules_ht, module->base_vaddr);

    dmn_lnx_entity_release(module);
  }

  is_unmap_complete_finished = 1;
exit:;
  Assert(is_unmap_complete_finished);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  Arena *arena = arena_alloc();
  dmn_lnx_state = push_array(arena, DMN_LNX_State, 1);
  dmn_lnx_state->arena = arena;
  dmn_lnx_state->deferred_events_arena = arena_alloc();
  dmn_lnx_state->entities_arena = arena_alloc(.reserve_size = GB(32), .commit_size = KB(64), .flags = ArenaFlag_NoChain);
  dmn_lnx_state->entities_base = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 0);
  dmn_lnx_entity_alloc(&dmn_lnx_nil_entity, DMN_LNX_EntityKind_Root);
  dmn_lnx_state->access_mutex = mutex_alloc();
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Blocking Control Thread Operations (Implemented Per-OS)

internal DMN_CtrlCtx *
dmn_ctrl_begin(void)
{
  DMN_CtrlCtx *ctx = (DMN_CtrlCtx *)1;
  dmn_lnx_ctrl_thread = 1;
  return ctx;
}

internal void
dmn_ctrl_exclusive_access_begin(void)
{
  MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 1;
  }
}

internal void
dmn_ctrl_exclusive_access_end(void)
{
  MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 0;
  }
}

internal U32
dmn_ctrl_launch(DMN_CtrlCtx *ctx, OS_ProcessLaunchParams *params)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: unpack command line
  char **argv = 0;
  int argc = 0;
  {
    argc = (int)(params->cmd_line.node_count);
    argv = push_array(scratch.arena, char *, argc+1);
    {
      U64 idx = 0;
      for(String8Node *n = params->cmd_line.first; n != 0; n = n->next, idx += 1)
      {
        argv[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
      }
    }
  }
  
  //- rjf: unpack path
  char *path = (char *)push_str8_copy(scratch.arena, params->path).str;
  
  //- rjf: unpack environment
  char **env = 0;
  {
    env = push_array(scratch.arena, char *, params->env.node_count+1);
    {
      U64 idx = 0;
      for(String8Node *n = params->env.first; n != 0; n = n->next, idx += 1)
      {
        env[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
      }
    }
  }
  
  //- rjf: create & set up new process
  if(argv != 0 && argv[0] != 0)
  {
    pid_t pid                    = 0;
    int   ptrace_result          = 0;
    int   chdir_result           = 0;
    int   setoptions_result      = 0;
    B32   error__need_child_kill = 0;
    
    //- rjf: fork
    pid = fork();
    if(pid == -1) { goto error; }
    
    //- rjf: child process -> execute actual target
    if(pid == 0)
    {
      // turn the thread into a tracee
      ptrace_result = ptrace(PTRACE_TRACEME, 0, 0, 0);
      if(ptrace_result == -1) { goto error; }

      // set current working directory to tracee
      chdir_result = chdir(path);
      if(chdir_result == -1) { goto error; }

      // replace process with target
      execve(argv[0], argv, env);

      // execve failed -- exit
      abort();
    }
    
    //- rjf: parent process
    if(pid != 0)
    {
      //- rjf: wait for child
      int   status  = 0;
      pid_t wait_id = waitpid(pid, &status, __WALL);
      if(wait_id != pid)
      {
        // NOTE(rjf): we do not know what this means - needs study if this actually arises.
        goto error;
      }
      
      //- rjf: determine child launch status
      typedef enum LaunchStatus
      {
        LaunchStatus_Null,
        LaunchStatus_FailBeforePtrace,
        LaunchStatus_FailAfterPtrace,
        LaunchStatus_Success,
      }
      LaunchStatus;
      LaunchStatus launch_status = LaunchStatus_Null;
      {
        B32 wifstopped = WIFSTOPPED(status);
        int wstopsig = WSTOPSIG(status);
        if(0){}
        else if(wifstopped && wstopsig == SIGTRAP) { launch_status = LaunchStatus_Success; }
        else if(wifstopped && wstopsig != SIGTRAP) { launch_status = LaunchStatus_FailAfterPtrace; }
        else                                       { launch_status = LaunchStatus_FailBeforePtrace; }
      }

      //- rjf: respond to launch status appropriately
      switch(launch_status)
      {
        //- rjf: no understood handling path
        default:{}break;
        
        //- rjf: failure, after ptrace => we need to explicitly obtain the
        // result code & exit the process, otherwise it will become a zombie,
        // since it is ptrace'd.
        case LaunchStatus_FailAfterPtrace:
        {
          B32 cleanup_good = 0;
          int detach_result = ptrace(PTRACE_DETACH, pid, 0, (void*)SIGCONT);
          if(detach_result != -1)
          {
            int status_cleanup = 0;
            pid_t wait_id_cleanup = waitpid(pid, &status_cleanup, __WALL);
            if(wait_id_cleanup == pid)
            {
              cleanup_good = 1;
            }
          }
          if(cleanup_good)
          {
            // TODO(rjf): child initialization failed, but we at least cleaned it up.
          }
          else
          {
            // TODO(rjf): child initialization failed, *and* we couldn't clean it up, so we've created
            // yet-another zombie.
          }
        }break;
        
        //- rjf: successful launch
        case LaunchStatus_Success:
        {
          setoptions_result = ptrace(PTRACE_SETOPTIONS, pid, 0, PtrFromInt(DMN_LNX_PTRACE_OPTIONS));
          if(setoptions_result == -1) { error__need_child_kill = 1; goto error; }

          ELF_Hdr64           exe_ehdr         = dmn_lnx_ehdr_from_pid(pid);
          int                 memory_fd        = open((char*)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR);
          DMN_LNX_ProcessAuxv auxv             = dmn_lnx_auxv_from_pid(pid, exe_ehdr.e_ident[ELF_Identifier_Class]);
          Arch                arch             = arch_from_elf_machine(exe_ehdr.e_machine);
          U64                 rdebug_vaddr     = dmn_lnx_rdebug_vaddr_from_memory(memory_fd, auxv.base);
          U64                 rdebug_brk_vaddr = rdebug_vaddr + gnu_r_brk_offset_from_arch(arch);

          ELF_Class dl_class;
          {
            ELF_Hdr64 ehdr = {0};
            if(!dmn_lnx_read_ehdr(memory_fd, auxv.base, &ehdr)) { Assert(0 && "failed to read interp's header"); }
            dl_class = ehdr.e_ident[ELF_Identifier_Class];
          }

          U64             xcr0         = 0;
          U64             xsave_size   = 0;
          X64_XSaveLayout xsave_layout = {0};
          if(arch == Arch_x64)
          {
            X64_XSave xsave = {0};
            if(ptrace(PTRACE_GETREGSET, pid, (void*)NT_X86_XSTATE, &(struct iovec){.iov_base = &xsave, .iov_len = sizeof(xsave) }) >= 0)
            {
              // Linux stores xcr0 bits in fxstate padding,
              // see https://github.com/torvalds/linux/blob/6548d364a3e850326831799d7e3ea2d7bb97ba08/arch/x86/include/asm/user.h#L25
              xcr0         = *(U64 *)((U8 *)&xsave + 464);
              xsave_size   = x64_get_xsave_size();
              xsave_layout = x64_get_xsave_layout(xcr0);
            }
            else
            {
              Assert(0 && "failed to get xstate");
            }
          }

          String8 dl_path = {0};
          {
            int maps_fd = open((char *)str8f(scratch.arena, "/proc/%d/maps", pid).str, O_RDONLY);
            if(maps_fd != -1)
            {
              struct stat st = {0};
              if(fstat(maps_fd, &st) != -1)
              {
                U64  maps_size = dmn_lnx_size_from_fd(maps_fd, MB(1));
                U8  *maps_ptr  = push_array(scratch.arena, U8, maps_size);
                U64  read_size = dmn_lnx_read(maps_fd, r1u64(0, maps_size), maps_ptr);
                if(read_size == maps_size)
                {
                  String8 maps = str8(maps_ptr, maps_size);

                  String8List parts = {0};
                  {
                    for(U64 cursor = 0, part_off = 0; cursor < maps.size; cursor += 1)
                    {
                      if(maps.str[cursor] == '\\')
                      {
                        cursor += 1;
                        continue;
                      }
                      if(maps.str[cursor] == ' ' || maps.str[cursor] == '\n' || cursor + 1 >= maps.size)
                      {
                        String8 p = str8_substr(maps, r1u64(part_off, cursor));
                        if(p.size > 0)
                        {
                          str8_list_push(scratch.arena, &parts, p);
                        }
                        part_off = cursor + 1;
                      }
                    }
                  }

                  for(String8Node *n = parts.first; n != 0; )
                  {
                    String8 vrange_str = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8 perms_str = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8 offset_str = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8 dev_str = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8 inode_str = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8 path = n->string;
                    n = n->next;
                    if(n == 0) { break; }

                    String8List vaddr_list = str8_split_by_string_chars(scratch.arena, vrange_str, str8_lit("-"), 0);
                    if(vaddr_list.node_count != 2) { break; }

                    U64 lo_vaddr = u64_from_str8(vaddr_list.first->string, 16);
                    if(lo_vaddr == auxv.base)
                    {
                      dl_path = push_str8_copy(scratch.arena, path);
                      break;
                    }
                  }
                }
              }
              close(maps_fd);
            }
          }
          AssertAlways(dl_path.size);

          // alloc arena for the process
          Arena *process_arena = arena_alloc();

          DMN_LNX_Probe **known_probes = push_array(process_arena, DMN_LNX_Probe *, DMN_LNX_ProbeType_Count);
          {
            DMN_LNX_ProbeList probes = {0};
            int dl_fd = open((char *)dl_path.str, O_RDONLY);
            if(dl_fd >= 0)
            {
              probes = dmn_lnx_read_probes(process_arena, dl_fd, 0, auxv.base);
              close(dl_fd);
            }

            for EachNode(n, DMN_LNX_ProbeNode, probes.first)
            {
              DMN_LNX_Probe *p = &n->v;
              if(str8_match(p->provider, str8_lit("rtld"), 0))
              {
                #define X(_N,_A,_S) if(str8_match(p->name, str8_lit(_S), 0)) { AssertAlways(p->args.count == _A); known_probes[DMN_LNX_ProbeType_##_N] = p; continue ; }
                DMN_LNX_Probe_XList
                #undef X
              }
            }
          }

          // install DL probes
          U64 probe_vaddrs[DMN_LNX_ProbeType_Count] = {0};
          for EachIndex(i, DMN_LNX_ProbeType_Count)
          {
            if(known_probes[i] == 0) { continue; }

            U8 og_byte = 0;
            if(!dmn_lnx_read_struct(memory_fd, known_probes[i]->pc, &og_byte)) { Assert(0 && "failed to read original byte"); }
            Assert(og_byte == 0x90);

            U8 trap = 0xcc;
            if(!dmn_lnx_write_struct(memory_fd, known_probes[i]->pc, &trap)) { Assert(0 && "failed to install probe"); }

            probe_vaddrs[i] = known_probes[i]->pc;
          }

          // make process entity & push create event
          DMN_LNX_Entity *process = &dmn_lnx_nil_entity;
          {
            process = dmn_lnx_entity_alloc(dmn_lnx_state->entities_base, DMN_LNX_EntityKind_Process);
            process->arch                          = arch;
            process->id                            = pid;
            process->fd                            = memory_fd;
            process->tracer_tid                    = gettid();
            process->rdebug_vaddr                  = rdebug_vaddr;
            process->rdebug_brk_vaddr              = rdebug_brk_vaddr;
            process->expect_rdebug_data_breakpoint = rdebug_vaddr != 0;
            process->dl_class                      = dl_class;
            process->arena                         = process_arena;
            process->loaded_modules_ht             = hash_table_init(process_arena, 0x1000);
            process->probes                        = known_probes;
            process->xcr0                          = xcr0;
            process->xsave_size                    = Max(xsave_size, sizeof(X64_XSave));
            process->xsave_layout                  = xsave_layout;
            MemoryCopyTyped(&process->probe_vaddrs[0], &probe_vaddrs[0], DMN_LNX_ProbeType_Count);
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_CreateProcess;
              e->process = dmn_lnx_handle_from_entity(process);
              e->arch    = process->arch;
              e->code    = pid;
            }
          }

          // make thread entity & push create event
          DMN_LNX_Entity *main_thread = &dmn_lnx_nil_entity;
          {
            main_thread = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Thread);
            main_thread->id        = pid;
            main_thread->arch      = process->arch;
            main_thread->reg_block = push_array(process->arena, U8, regs_block_size_from_arch(process->arch));
            dmn_lnx_thread_read_reg_block(main_thread, main_thread->reg_block);
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_CreateThread;
              e->process = dmn_lnx_handle_from_entity(process);
              e->thread  = dmn_lnx_handle_from_entity(main_thread);
              e->arch    = main_thread->arch;
              e->code    = main_thread->id;
            }
          }

          // make main module & push load module event
          {
            U64              base_vaddr = (auxv.phdr & ~(auxv.pagesz-1));
            U64              rebase     = exe_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
            DMN_LNX_PhdrInfo phdr_info  = dmn_lnx_phdr_info_from_memory(memory_fd, exe_ehdr.e_ident[ELF_Identifier_Class], rebase, auxv.phdr, auxv.phent, auxv.phnum);

            DMN_LNX_Entity *module = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
            module->id         = auxv.execfn;
            module->base_vaddr = base_vaddr;

            DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
            e->kind             = DMN_EventKind_LoadModule;
            e->process          = dmn_lnx_handle_from_entity(process);
            e->thread           = dmn_lnx_handle_from_entity(main_thread);
            e->module           = dmn_lnx_handle_from_entity(module);
            e->arch             = process->arch;
            e->address          = base_vaddr;
            e->size             = dim_1u64(phdr_info.range);
            e->string           = dmn_lnx_read_string(dmn_lnx_state->deferred_events_arena, process->fd, auxv.execfn);
            e->elf_phdr_vrange  = r1u64(auxv.phdr, auxv.phdr + auxv.phent * auxv.phnum);
            e->elf_phdr_entsize = auxv.phent;

            hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, 0, module);
            hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, base_vaddr, module);
          }

          // rjf: handshake event
          {
            DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
            e->kind    = DMN_EventKind_HandshakeComplete;
            e->process = dmn_lnx_handle_from_entity(process);
            e->thread  = dmn_lnx_handle_from_entity(main_thread);
            e->arch    = process->arch;
          }
        }break;
      }
    }
    
    //- rjf: error case
    goto success;
    error:;
    {
      if(error__need_child_kill)
      {
        // TODO(rjf)
      }
    }
    
    //- rjf: success
    success:;
  }
  
  scratch_end(scratch);
  return 0;
}

internal B32
dmn_ctrl_attach(DMN_CtrlCtx *ctx, U32 pid)
{
  return 0;
}

internal B32
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process, U32 exit_code)
{
  B32 result = 0;
  DMN_LNX_Entity *process_entity = dmn_lnx_entity_from_handle(process);
  if(process_entity != &dmn_lnx_nil_entity &&
     kill(process_entity->id, SIGKILL) != -1)
  {
    result = 1;
  }
  return result;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process)
{
  B32 result = 0;
  DMN_LNX_Entity *process_entity = dmn_lnx_entity_from_handle(process);
  if(process_entity != &dmn_lnx_nil_entity &&
     ptrace(PTRACE_DETACH, process_entity->id, 0, 0) != -1)
  {
    result = 1;
  }
  return result;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  Temp scratch = scratch_begin(&arena, 1);
  DMN_EventList evts = {0};
  
  ////////////////////////////
  //- rjf: push any deferred events
  //
  {
    for EachNode(n, DMN_EventNode, dmn_lnx_state->deferred_events.first)
    {
      DMN_Event *e_src = &n->v;
      DMN_Event *e_dst = dmn_event_list_push(arena, &evts);
      MemoryCopyStruct(e_dst, e_src);
      e_dst->string = str8_copy(arena, e_dst->string);
    }
    MemoryZeroStruct(&dmn_lnx_state->deferred_events);
    arena_clear(dmn_lnx_state->deferred_events_arena);
  }
  
  ////////////////////////////
  //- rjf: no processes, no output events -> not attached
  //
  if(evts.count == 0 && dmn_lnx_state->entities_base->first == &dmn_lnx_nil_entity)
  {
    DMN_Event *e = dmn_event_list_push(arena, &evts);
    e->kind       = DMN_EventKind_Error;
    e->error_kind = DMN_ErrorKind_NotAttached;
  }
  
  ////////////////////////////
  //- rjf: determine if we need to wait for new events
  //
  B32 need_wait_on_events = (evts.count == 0);
  
  ////////////////////////////
  //- rjf: write all traps into memory
  //
  DMN_ActiveTrap *active_trap_first = 0, *active_trap_last = 0;
  {
    ProfScope("write all traps into memory")
    {
      HashTable *ht = hash_table_init(scratch.arena, ctrls->traps.trap_count);
      for EachNode(n, DMN_TrapChunkNode, ctrls->traps.first)
      {
        for EachIndex(n_idx, n->count)
        {
          DMN_Trap *trap = n->v+n_idx;

          if(trap->flags == 0)
          {
            DMN_ActiveTrap *is_set = hash_table_search_u64_raw(ht, trap->vaddr);
            if(is_set) {continue;}

            U8 swap_byte = 0;
            if(dmn_process_read(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &swap_byte) > 0)
            {
              U8 int3 = 0xCC;
              if(dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &int3))
              {
                DMN_ActiveTrap *active_trap = push_array(scratch.arena, DMN_ActiveTrap, 1);
                active_trap->trap      = trap;
                active_trap->swap_byte = swap_byte;
                SLLQueuePush(active_trap_first, active_trap_last, active_trap);

                hash_table_push_u64_raw(scratch.arena, ht, trap->vaddr, active_trap);
              } else { Assert(0 && "failed to write trap"); }
            } else { Assert(0 && "failed to read original byte"); }
          }
        }
      }
    }
  }

  ////////////////////////////
  //- rjf: gather all threads which we should run
  //
  DMN_LNX_EntityNode *first_run_thread = 0, *last_run_thread = 0;
  if(need_wait_on_events) ProfScope("gather all threads which we should run")
  {
    //- rjf: scan all processes
    for(DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first;
        process != &dmn_lnx_nil_entity;
        process = process->next)
    {
      if(process->kind != DMN_LNX_EntityKind_Process) {continue;}
      
      //- rjf: determine if this process is frozen
      B32 process_is_frozen = 0;
      if(ctrls->run_entities_are_processes)
      {
        for(U64 idx = 0; idx < ctrls->run_entity_count; idx += 1)
        {
          if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(process)))
          {
            process_is_frozen = 1;
            break;
          }
        }
      }
      
      //- rjf: scan all threads in this process
      for(DMN_LNX_Entity *thread = process->first;
          thread != &dmn_lnx_nil_entity;
          thread = thread->next)
      {
        if(thread->kind != DMN_LNX_EntityKind_Thread) {continue;}
        
        //- rjf: determine if this thread is frozen
        B32 is_frozen = 0;
        {
          // rjf: single-step? freeze if not the single-step thread.
          if(!dmn_handle_match(dmn_handle_zero(), ctrls->single_step_thread))
          {
            is_frozen = !dmn_handle_match(dmn_lnx_handle_from_entity(thread), ctrls->single_step_thread);
          }
          
          // rjf: not single-stepping? determine based on run controls freezing info
          else
          {
            if(ctrls->run_entities_are_processes)
            {
              is_frozen = process_is_frozen;
            }
            else for(U64 idx = 0; idx < ctrls->run_entity_count; idx += 1)
            {
              if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(thread)))
              {
                is_frozen = 1;
                break;
              }
            }
            if(ctrls->run_entities_are_unfrozen)
            {
              is_frozen ^= 1;
            }
          }
        }
        
        //- rjf: disregard all other rules if this is the halter thread
        // TODO(rjf): halting - here is what we do on windows...
#if 0
        if(dmn_w32_shared->halter_tid == thread->id)
        {
          is_frozen = 0;
        }
#endif
        
        //- rjf: add to list
        if(!is_frozen)
        {
          DMN_LNX_EntityNode *n = push_array(scratch.arena, DMN_LNX_EntityNode, 1);
          n->v = thread;
          SLLQueuePush(first_run_thread, last_run_thread, n);
        }
      }
    }
  }

  // enable single stepping
  DMN_LNX_Entity *single_step_thread = dmn_lnx_entity_from_handle(ctrls->single_step_thread);
  if(single_step_thread != &dmn_lnx_nil_entity)
  {
    dmn_lnx_set_single_step_flag(single_step_thread, 1);
  }

  // update registers
  for EachNode(n, DMN_LNX_EntityNode, first_run_thread)
  {
    DMN_LNX_Entity *thread = n->v;
    if(thread->reg_block)
    {
      if (!dmn_lnx_thread_write_reg_block(thread, thread->reg_block)) { Assert(0 && "failed to write thread's registers"); }
    }
  }

  ////////////////////////////
  //- rjf: resume all threads we need to run
  //
  DMN_LNX_EntityNode *first_ran_thread = 0, *last_ran_thread = 0;
  for EachNode(n, DMN_LNX_EntityNode, first_run_thread)
  {
    DMN_LNX_Entity *thread = n->v;

    void *sig_code = 0;
    if(dmn_lnx_state->last_event_kind == DMN_EventKind_Exception && dmn_lnx_state->last_stop_pid == thread->id)
    {
      sig_code = (void *)(uintptr_t)dmn_lnx_state->last_sig_code;
    }
    if (ptrace(PTRACE_CONT, (pid_t)thread->id, 0, (void *)sig_code) < 0) { Assert(0 && "failed to resume a thread"); }

    DMN_LNX_EntityNode *n2 = push_array_no_zero(scratch.arena, DMN_LNX_EntityNode, 1);
    SLLQueuePush(first_ran_thread, last_ran_thread, n2);
    n2->v = thread;
  }
  
  ////////////////////////////
  //- rjf: loop: wait for next stop, produce debug events
  //
  for(B32 done = !need_wait_on_events; !done;)
  {
    //- rjf: wait for next event
    int   status  = 0;
    pid_t wait_id = waitpid(-1, &status, __WALL|__WNOTHREAD);
    if(status == -1 && errno == EINTR) {continue;} // wait interrupted, try again
    if(status == -1) {InvalidPath;} // TODO: graceful exit

    //- rjf: unpack event
    int             wifexited              = WIFEXITED(status);
    int             wifsignaled            = WIFSIGNALED(status);
    int             wifstopped             = WIFSTOPPED(status);
    int             wstopsig               = WSTOPSIG(status);
    int             ptrace_event_code      = (status>>16);
    DMN_LNX_Entity *thread                 = dmn_lnx_thread_from_pid(wait_id);
    DMN_LNX_Entity *process                = thread->parent;
    B32             thread_is_process_root = (thread->id == process->id);

    // update thread registers
    if(thread != &dmn_lnx_nil_entity)
    {
      if (!dmn_lnx_thread_read_reg_block(thread, thread->reg_block)) { Assert(0 && "failed to update thread's registers"); }
    }
    
    DMN_EventKind  e_kind        = DMN_EventKind_Null;
    U64            exit_code     = max_U64;
    U64            address       = 0;
    DMN_Trap      *hit_user_trap = 0;

    //- rjf: WIFEXITED(status) -> thread exit
    if(wifexited)
    {
      e_kind = DMN_EventKind_ExitThread;
    }
    
    //- rjf: WIFEXITED(status) -> thread exit w/ exit code
    else if(wifsignaled)
    {
      e_kind    = DMN_EventKind_ExitThread;
      exit_code = WTERMSIG(status);
    }
    
    //- rjf: SIGTRAP:PTRACE_EVENT_EXIT
    else if(wifstopped && wstopsig == SIGTRAP && ptrace_event_code == PTRACE_EVENT_EXIT)
    {
      // TODO(rjf): verify
      e_kind = DMN_EventKind_ExitThread;
    }
    
    //- rjf: SIGTRAP:PTRACE_EVENT_CLONE
    else if(wifstopped && wstopsig == SIGTRAP && ptrace_event_code == PTRACE_EVENT_CLONE)
    {
      // TODO(rjf)
    }
    
    //- rjf: SIGTRAP:PTRACE_EVENT_FORK, or SIGTRAP:PTRACE_EVENT_VFORK
    else if(wifstopped && wstopsig == SIGTRAP &&
            (ptrace_event_code == PTRACE_EVENT_FORK ||
             ptrace_event_code == PTRACE_EVENT_VFORK))
    {
    }
    
    //- rjf: SIGTRAP
    else if(wifstopped && wstopsig == SIGTRAP)
    {
      // translate signal code to DEMON event kind
      siginfo_t siginfo = {0};
      if(ptrace(PTRACE_GETSIGINFO, wait_id, 0, &siginfo) < 0) { Assert(0 && "failed to get signal info"); }
      switch(siginfo.si_code)
      {
        case DMN_LNX_SigTrapCode_Brkpt:
        {
          e_kind = DMN_EventKind_Breakpoint;
        }break;
        case DMN_LNX_SigTrapCode_Trace:
        {
          e_kind = DMN_EventKind_SingleStep;
        }break;
        case DMN_LNX_SigTrapCode_HwBkpt:
        {
          if(thread->arch == Arch_Null) { } 
          else if(thread->arch == Arch_x64)
          {
            REGS_RegBlockX64 *regs_x64 = thread->reg_block;
            if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B0)
            {
              address = regs_x64->dr0.u64;
              e_kind = DMN_EventKind_Breakpoint;
            }
            else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B1)
            {
              address = regs_x64->dr1.u64;
              e_kind = DMN_EventKind_Breakpoint;
            }
            else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B2)
            {
              address = regs_x64->dr2.u64;
              e_kind = DMN_EventKind_Breakpoint;
            }
            else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B3)
            {
              address = regs_x64->dr3.u64;
              e_kind = DMN_EventKind_Breakpoint;
            }
          }
          else
          {
            NotImplemented;
          }
        }break;
        case SI_KERNEL:
        {
          e_kind = DMN_EventKind_Breakpoint;
        }break;
        case DMN_LNX_SigTrapCode_Unk:  {NotImplemented;}break;
        case DMN_LNX_SigTrapCode_Perf: {NotImplemented;}break;
        default: {InvalidPath;} break;
      }
    }
    
    //- rjf: WSTOPSIG(status) is SIGSTOP
    else if(wifstopped && wstopsig == SIGSTOP)
    {
      //
      // TODO(rjf): how do we tell the following apart?:
      // - SIGSTOP All-Stop
      // - SIGSTOP Halt
      // - SIGSTOP "User"
      //
      // we are currently just assuming that, if we've queried a SIGSTOP to halt, then
      // the first one that comes back is our "dummy" sigstop. this is likely not
      // necessarily true.
      //
      if(thread->expecting_dummy_sigstop)
      {
        thread->expecting_dummy_sigstop = 0;
        done = 0;
      }
      else if(dmn_lnx_state->has_halt_injection)
      {
        e_kind = DMN_EventKind_Halt;
      }
      else
      {
        // TODO(rjf): study this case; old notes:
        //
        // a signal we don't want to mess with (except to record that it
        // happened maybe) we should "hand it back"
      }
    }
    
    //- rjf: WSTOPSIG(status) is an unrecoverable exception (unless user does something to fix state first)
    else if(wifstopped)
    {
      e_kind = DMN_EventKind_Exception;
    }
    else
    {
      Assert(0 && "unexpected stop code");
    }

    dmn_lnx_state->last_event_kind = e_kind;
    dmn_lnx_state->last_stop_pid   = wait_id;
    dmn_lnx_state->last_sig_code   = wstopsig;
    done = 1;

    if(e_kind == DMN_EventKind_Breakpoint)
    {
      U64 ip = dmn_lnx_thread_read_ip(thread);
      for EachNode(active_trap, DMN_ActiveTrap, active_trap_first)
      {
        if(active_trap->trap->vaddr == ip-1)
        {
          hit_user_trap = active_trap->trap;
          break;
        }
      }
    }

    // is this a probe trap?
    if(e_kind == DMN_EventKind_Breakpoint)
    {
      // find which probe was triggered
      U64 ip = dmn_lnx_thread_read_ip(thread);
      DMN_LNX_ProbeType probe_type = DMN_LNX_ProbeType_Null;
      for EachIndex(i, ArrayCount(process->probe_vaddrs))
      {
        if(process->probe_vaddrs[i] == ip-1)
        {
          probe_type = i;
          break;
        }
      }

      if(probe_type == DMN_LNX_ProbeType_InitComplete)
      {
        U64 name_space_id = 0, rdebug_addr = 0;
        DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_InitComplete];
        if(stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id))
        {
          if(stap_read_arg_u(probe->args.v[1], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_addr))
          {
            GNU_RDebugInfo64 rdebug = {0};
            if(dmn_lnx_read_r_debug(process->fd, rdebug_addr, process->arch, &rdebug))
            {
              if(rdebug.r_version == 1)
              {
                dmn_lnx_process_loaded_modules(arena, &evts, process, name_space_id, rdebug.r_map);
              }
              else { Assert(0 && "unexpected version number"); }
            }
            else { Assert(0 && "failed to read rdebug"); }
          }
          else { Assert(0 && "failed to parse second argument"); }
        }
        else { Assert(0 && "failed to parse first argument"); }
      }
      else if(probe_type == DMN_LNX_ProbeType_RelocComplete)
      {
        U64 name_space_id = 0, new_link_map_addr = 0;
        DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_RelocComplete];
        if(stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id))
        {
          if(stap_read_arg_u(probe->args.v[2], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &new_link_map_addr))
          {
            dmn_lnx_process_loaded_modules(arena, &evts, process, name_space_id, new_link_map_addr);
          }
          else { Assert(0 && "failed to parse third argument"); }
        }
        else { Assert(0 && "failed to parse first argument"); }
      }
      else if(probe_type == DMN_LNX_ProbeType_UnmapComplete)
      {
        // read probe's arguments
        U64 name_space_id = 0, rdebug_vaddr = 0;
        DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_UnmapComplete];
        if(stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id))
        {
          if(stap_read_arg_u(probe->args.v[1], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_vaddr))
          {
            dmn_lnx_process_unloaded_modules(arena, &evts, process, name_space_id, rdebug_vaddr);
          }
          else { Assert(0 && "failed to read second argument"); }
        }
        else { Assert(0 && "failed to read first argument"); }
      }

      if(probe_type != DMN_LNX_ProbeType_Null) {break;}
    }

    // rollback IP on user traps
    if(hit_user_trap)
    {
      U64 ip = dmn_lnx_thread_read_ip(thread);
      dmn_lnx_thread_write_ip(thread, ip - 1);
    }

    switch(e_kind)
    {
      case DMN_EventKind_COUNT:
      case DMN_EventKind_Null: break;
      case DMN_EventKind_Error:
      case DMN_EventKind_HandshakeComplete:
      case DMN_EventKind_LoadModule:
      case DMN_EventKind_UnloadModule:
        {InvalidPath;}break;
      case DMN_EventKind_Trap:
      case DMN_EventKind_Memory:
      case DMN_EventKind_SetThreadName:
      case DMN_EventKind_SetThreadColor:
      case DMN_EventKind_SetBreakpoint:
      case DMN_EventKind_UnsetBreakpoint:
      case DMN_EventKind_SetVAddrRangeNote:
      case DMN_EventKind_DebugString:
      {
        NotImplemented;
      }break;
      case DMN_EventKind_SingleStep:
      {
        // clear single step flag
        dmn_lnx_set_single_step_flag(thread, 0);

        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind                = e_kind;
        e->process             = dmn_lnx_handle_from_entity(process);
        e->thread              = dmn_lnx_handle_from_entity(thread);
        e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
      }break;
      case DMN_EventKind_Breakpoint:
      {
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind                = e_kind;
        e->process             = dmn_lnx_handle_from_entity(process);
        e->thread              = dmn_lnx_handle_from_entity(thread);
        e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
      }break;
      case DMN_EventKind_Halt:
      {
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind    = DMN_EventKind_Halt;
        e->process = dmn_lnx_handle_from_entity(process);
        e->thread  = dmn_lnx_handle_from_entity(thread);
      }break;
      case DMN_EventKind_Exception:
      {
        // TODO(rjf): possible cases:
        // SIGABRT
        // SIGFPE
        // SIGSEGV
        // SIGILL
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind                = DMN_EventKind_Exception;
        e->process             = dmn_lnx_handle_from_entity(process);
        e->thread              = dmn_lnx_handle_from_entity(thread);
        e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
        e->signo               = wstopsig;
      }break;
      case DMN_EventKind_CreateProcess:
      {
        NotImplemented;
      }break;
      case DMN_EventKind_ExitProcess:
      {
        // rjf: generate exit-thread / unload-module events
        for(DMN_LNX_Entity *child = process->first; child != &dmn_lnx_nil_entity; child = child->next)
        {
          switch(child->kind)
          {
            default:{}break;
            case DMN_LNX_EntityKind_Thread:
            {
              DMN_Event *e = dmn_event_list_push(arena, &evts);
              e->kind    = DMN_EventKind_ExitThread;
              e->process = dmn_lnx_handle_from_entity(process);
              e->thread  = dmn_lnx_handle_from_entity(child);
            }break;
            case DMN_LNX_EntityKind_Module:
            {
              DMN_Event *e = dmn_event_list_push(arena, &evts);
              e->kind    = DMN_EventKind_UnloadModule;
              e->process = dmn_lnx_handle_from_entity(process);
              e->module  = dmn_lnx_handle_from_entity(child);
              // TODO(rjf): e->string = ...;
            }break;
          }
        }
        
        // rjf: generate exit process event
        {
          DMN_Event *e = dmn_event_list_push(arena, &evts);
          e->kind    = DMN_EventKind_ExitProcess;
          e->process = dmn_lnx_handle_from_entity(process);
          e->code    = exit_code;
        }
        
        // rjf: eliminate entity tree
        dmn_lnx_entity_release(process);
      }break;
      case DMN_EventKind_CreateThread:
      {
        NotImplemented;
      }break;
      case DMN_EventKind_ExitThread:
      {
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind    = DMN_EventKind_ExitThread;
        e->process = dmn_lnx_handle_from_entity(process);
        e->thread  = dmn_lnx_handle_from_entity(thread);
        dmn_lnx_entity_release(thread);
      }break;
    }
  }
  
  ////////////////////////////
  //- rjf: stop all threads
  //
  for EachNode(n, DMN_LNX_EntityNode, first_ran_thread)
  {
    DMN_LNX_Entity *thread = n->v;
    pid_t thread_id = (pid_t)thread->id;
    if(thread_id != dmn_lnx_state->last_stop_pid)
    {
      union sigval sv = {0};
      sigqueue(thread_id, SIGSTOP, sv);
      thread->expecting_dummy_sigstop = 1;
    }
  }
  
  //////////////////////////
  //- rjf: restore original memory at trap locations
  //
  ProfScope("restore original memory at trap locations")
  {
    for EachNode(active_trap, DMN_ActiveTrap, active_trap_first)
    {
      if(!dmn_process_write_struct(active_trap->trap->process, active_trap->trap->vaddr, &active_trap->swap_byte))
      {
        Assert(0 && "failed to swap back original byte");
      }
    }
  }
  
  scratch_end(scratch);
  return evts;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
  if(!dmn_lnx_state->has_halt_injection)
  {
    DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first;
    if(process != &dmn_lnx_nil_entity)
    {
      union sigval sv = {0};
      if(sigqueue(process->id, SIGSTOP, sv) != -1)
      {
        dmn_lnx_state->has_halt_injection = 1;
        dmn_lnx_state->halt_code          = code;
        dmn_lnx_state->halt_user_data     = user_data;
      }
    }
  }
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Introspection Functions (Implemented Per-OS)

//- rjf: non-blocking-control-thread access barriers

internal B32
dmn_access_open(void)
{
  B32 result = 0;
  if(dmn_lnx_ctrl_thread)
  {
    result = 1;
  }
  else
  {
    mutex_take(dmn_lnx_state->access_mutex);
    result = !dmn_lnx_state->access_run_state;
  }
  return result;
}

internal void
dmn_access_close(void)
{
  if(!dmn_lnx_ctrl_thread)
  {
    mutex_drop(dmn_lnx_state->access_mutex);
  }
}

//- rjf: processes

internal U64
dmn_process_memory_reserve(DMN_Handle process, U64 vaddr, U64 size)
{
  return 0;
}

internal void
dmn_process_memory_commit(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_decommit(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_release(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_protect(DMN_Handle process, U64 vaddr, U64 size, OS_AccessFlags flags)
{
}

internal U64
dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst)
{
  DMN_LNX_Entity *entity = dmn_lnx_entity_from_handle(process);
  U64 result = dmn_lnx_read(entity->fd, range, dst);
  return result;
}

internal B32
dmn_process_write(DMN_Handle process, Rng1U64 range, void *src)
{
  DMN_LNX_Entity *entity = dmn_lnx_entity_from_handle(process);
  B32 result = dmn_lnx_write(entity->fd, range, src);
  return result;
}

//- rjf: threads

internal Arch
dmn_arch_from_thread(DMN_Handle handle)
{
  DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
  return thread->arch;
}

internal U64
dmn_stack_base_vaddr_from_thread(DMN_Handle handle)
{
  return 0;
}

internal U64
dmn_tls_root_vaddr_from_thread(DMN_Handle handle)
{
  return 0;
}

internal B32
dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread         = dmn_lnx_entity_from_handle(handle);
    U64             reg_block_size = regs_block_size_from_arch(thread->arch);
    if(thread == &dmn_lnx_nil_entity)
    {
      MemoryZero(reg_block, reg_block_size);
    }
    else
    {
      MemoryCopy(reg_block, thread->reg_block, reg_block_size);
    }
    result = 1;
  }
  return result;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread         = dmn_lnx_entity_from_handle(handle);
    U64             reg_block_size = regs_block_size_from_arch(thread->arch);
    if(thread == &dmn_lnx_nil_entity)
    {
      MemoryZero(reg_block, reg_block_size);
    }
    else
    {
      MemoryCopy(thread->reg_block, reg_block, reg_block_size);
    }
    result = 1;
  }
  return result;
}

//- rjf: system process listing

internal void
dmn_process_iter_begin(DMN_ProcessIter *iter)
{
  DIR *dir = opendir("/proc");
  MemoryZeroStruct(iter);
  iter->v[0] = IntFromPtr(dir);
}

internal B32
dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out)
{
  // rjf: scan for the next process ID in the directory
  B32 got_pid = 0;
  String8 pid_string = {0};
  {
    DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
    if(dir != 0 && iter->v[1] == 0)
    {
      for(;;)
      {
        // rjf: get next entry
        struct dirent *d = readdir(dir);
        if(d == 0)
        {
          break;
        }
        
        // rjf: check file name is integer
        String8 file_name = str8_cstring((char*)d->d_name);
        B32 is_integer = str8_is_integer(file_name, 10);
        
        // rjf: break on integers (which represent processes)
        if(is_integer)
        {
          got_pid = 1;
          pid_string = file_name;
          break;
        }
      }
    }
  }
  
  // rjf: if we found a process id, map id => info
  B32 result = 0;
  if(got_pid)
  {
    pid_t pid = u64_from_str8(pid_string, 10);
    String8 name = dmn_lnx_exe_path_from_pid(arena, pid);
    if(name.size == 0)
    {
      name = str8_lit("(unknown process)");
    }
    info_out->name = name;
    info_out->pid = pid;
    result = 1;
  }
  
  return result;
}

internal void
dmn_process_iter_end(DMN_ProcessIter *iter)
{
  DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
  if(dir != 0)
  {
    closedir(dir);
  }
  MemoryZeroStruct(iter);
}
