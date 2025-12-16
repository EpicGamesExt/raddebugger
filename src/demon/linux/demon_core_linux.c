// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal DMN_LNX_EntityNode *
dmn_lnx_entity_list_push(Arena *arena, DMN_LNX_EntityList *list, DMN_LNX_Entity *v)
{
  DMN_LNX_EntityNode *n = push_array(arena, DMN_LNX_EntityNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal void
dmn_lnx_process_launch_list_push_node(DMN_LNX_ProcessLaunchList *list, DMN_LNX_ProcessLaunch *n)
{
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
}

internal DMN_LNX_ProcessLaunch *
dmn_lnx_process_launch_list_push(Arena *arena, DMN_LNX_ProcessLaunchList *list, pid_t pid)
{
  DMN_LNX_ProcessLaunch *n = push_array(arena, DMN_LNX_ProcessLaunch, 1);
  n->pid = pid;
  dmn_lnx_process_launch_list_push_node(list, n);
  return n;
}

internal void
dmn_lnx_process_launch_list_remove(DMN_LNX_ProcessLaunchList *list, DMN_LNX_ProcessLaunch *n)
{
  Assert(list->count > 0);
  DLLRemove(list->first, list->last, n);
  list->count -= 1;
}

//- rjf: file descriptor memory reading/writing helpers

internal U64
dmn_lnx_size_from_fd(int memory_fd, U64 cap)
{
  U8 temp[4096];
  size_t cursor = 0;
  while(cursor < cap)
  {
    ssize_t actual_read = OS_LNX_RETRY_ON_EINTR(pread(memory_fd, temp, sizeof(temp), cursor));
    if(actual_read <= 0) { break; }
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
    ssize_t actual_read = OS_LNX_RETRY_ON_EINTR(pread(memory_fd, (U8 *)dst + cursor, to_read, range.min + cursor));
    if(actual_read < 0) { break; }
    if(actual_read == 0) { break; }
    cursor += actual_read;
  }
  return cursor;
}

internal B32
dmn_lnx_write(int memory_fd, Rng1U64 range, void *src)
{
  size_t cursor = 0, size = dim_1u64(range);
  while(cursor < size)
  {
    size_t to_write = size - cursor;
    ssize_t actual_write = OS_LNX_RETRY_ON_EINTR(pwrite(memory_fd, (U8 *)src + cursor, to_write, range.min + cursor));
    if(actual_write <= 0) { break; }
    cursor += actual_write;
  }
  B32 is_written = (cursor == size);
  return is_written;
}

internal String8
dmn_lnx_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size)
{
  String8 result = {0};
  U64 string_size = 0;
  for(U64 vaddr = base_vaddr; string_size < cap_size; vaddr += 1, string_size += 1)
  {
    char byte = 0;
    if(OS_LNX_RETRY_ON_EINTR(pread(memory_fd, &byte, sizeof(byte), vaddr) <= 0))
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
    OS_LNX_RETRY_ON_EINTR(pread(memory_fd, buf, string_size, base_vaddr));
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

internal
MACHINE_OP_MEM_READ(dmn_lnx_machine_op_mem_read)
{
  return dmn_lnx_read(((DMN_LNX_Entity *)ud)->fd, r1u64(addr, addr + buffer_size), buffer);
}

internal int
dmn_lnx_ptrace_seize(pid_t pid)
{
  // ensure tracer ops are issued on thread that sizes processes
  if(dmn_lnx_state->tracer_tid == 0) { dmn_lnx_state->tracer_tid = gettid(); }
  AssertAlways(dmn_lnx_state->tracer_tid == gettid());

  // TODO: PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEVFORKDONE
  return OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL | PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE));
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
  
  String8     exe_link_path   = str8f(scratch.arena, "/proc/%d/exe", pid);
  String8List parts           = {0};
  int         readlink_result = 0;
  for(S64 r = 0, cap = PATH_MAX; r < 4; cap *= 2, r += 1)
  {
    U8 *buffer = push_array(arena, U8, cap);
    readlink_result = readlink((char *)exe_link_path.str, (char *)buffer, cap);

    if(readlink_result < 0)
    {
      break;
    }

    str8_list_push(scratch.arena, &parts, str8(buffer, readlink_result));

    if(readlink_result < cap)
    {
      break;
    }
  }
  
  String8 result = str8_list_join(arena, &parts, 0);
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
    int exe_fd = OS_LNX_RETRY_ON_EINTR(open((char *)exe_path.str, O_RDONLY));
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
  int     auxv_fd   = OS_LNX_RETRY_ON_EINTR(open((char*)auxv_path.str, O_RDONLY));
  
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
        if(read(auxv_fd, &auxv32, sizeof(auxv32)) != sizeof(auxv32)) { goto brkloop; }
        auxv = elf_auxv64_from_auxv32(auxv32);
      }break;
      case ELF_Class_64:
      {
        if(read(auxv_fd, &auxv, sizeof(auxv)) != sizeof(auxv)) { goto brkloop; }
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
dmn_lnx_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vbase, B32 is_rebased)
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
  U64 dynamic_info_rebase = is_rebased ? 0 : rebase;
  DMN_LNX_DynamicInfo dynamic_info = dmn_lnx_dynamic_info_from_memory(memory_fd, elf_class, dynamic_info_rebase, dynamic_vaddr);

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

internal String8
dmn_lnx_dl_path_from_pid(Arena *arena, pid_t pid, U64 auxv_base)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 dl_path = {0};

  int maps_fd = open((char *)str8f(scratch.arena, "/proc/%d/maps", pid).str, O_RDONLY);
  if(maps_fd != -1)
  {
    // read entire /proc/pid/maps
    U64  maps_size = dmn_lnx_size_from_fd(maps_fd, MB(1));
    U8  *maps_ptr  = push_array(scratch.arena, U8, maps_size);
    U64  read_size = dmn_lnx_read(maps_fd, r1u64(0, maps_size), maps_ptr);

      // split map file on lines
    String8List lines = str8_split_by_string_chars(scratch.arena, str8(maps_ptr, maps_size), str8_lit("\n"), 0);

    // scan each line until a virtual mapping whose low part matches the DL base address is found
    for EachNode(n, String8Node, lines.first)
    {
      String8 line = n->string;

      // split the string while respecting escape sequences
      String8List parts = {0};
      for EachIndex(cursor, line.size)
      {
        if(parts.node_count > 5)    { break; }
        if(line.str[cursor] == ' ') { continue; }

        // scan forward to the closing delimiter
        U64 token_start = cursor;
        for(; cursor < line.size; cursor += 1)
        {
          if(line.str[cursor] == '\\')
          {
            cursor += 1;
            continue;
          }
          if(line.str[cursor] == ' ') { break; }
        }

        // push sub-string to the list
        str8_list_push(scratch.arena, &parts, str8_substr(line, r1u64(token_start, cursor)));
      }

      // was line parsed correctly?
      if(parts.node_count < 5) { Assert(0 && "failed to parse map line"); continue; }

      // parse map virtual range
      String8List vaddr_list = str8_split_by_string_chars(scratch.arena, parts.first->string, str8_lit("-"), 0);
      if(vaddr_list.node_count != 2) { Assert(0 && "failed to parse virtual range portion of map line"); continue; }

      // does the low part match DL base address?
      U64 lo_vaddr = u64_from_str8(vaddr_list.first->string, 16);
      if(lo_vaddr == auxv_base)
      {
        dl_path = parts.node_count == 5 ? str8_zero() : parts.last->string;
        dl_path = push_str8_copy(arena, dl_path);
        break;
      }
    }

    close(maps_fd);
  }
  else { Assert(0 && "failed to open DL fd"); }

  scratch_end(scratch);
  Assert(dl_path.size);
  return dl_path;
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
  MemoryCopyStruct(entity, dmn_lnx_nil_entity);
  entity->gen += 1;
  if(parent != dmn_lnx_nil_entity)
  {
    DLLPushBack_NPZ(dmn_lnx_nil_entity, parent->first, parent->last, entity, next, prev);
    entity->parent = parent;
  }
  entity->kind = kind;
  return entity;
}

internal void
dmn_lnx_entity_release(DMN_LNX_Entity *entity)
{
  if(entity->parent != dmn_lnx_nil_entity)
  {
    DLLRemove_NPZ(dmn_lnx_nil_entity, entity->parent->first, entity->parent->last, entity, next, prev);
    entity->parent = dmn_lnx_nil_entity;
  }
  {
    Temp scratch = scratch_begin(0, 0);
    for EachNode(t, DMN_LNX_EntityNode, &(DMN_LNX_EntityNode) { .v = entity })
    {
      // put children nodes on the task list
      for(DMN_LNX_Entity *child = t->v->first; child != dmn_lnx_nil_entity; child = child->next)
      {
        DMN_LNX_EntityNode *task = push_array(scratch.arena, DMN_LNX_EntityNode, 1);
        task->next = t->next;
        t->next = task;
        task->v = child;
      }

      // free entity
      U64 gen = t->v->gen + 1;
      MemoryZeroStruct(t->v);
      t->v->gen = gen;
      SLLStackPush(dmn_lnx_state->free_entity, t->v);
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
  DMN_LNX_Entity *result = dmn_lnx_nil_entity;
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
  DMN_LNX_Entity *thread = hash_table_search_u64_raw(dmn_lnx_state->tid_ht, pid);
  if(thread == 0)
  {
    thread = dmn_lnx_nil_entity;
  }
  return thread;
}

internal DMN_LNX_Entity *
dmn_lnx_process_from_pid(pid_t pid)
{
  DMN_LNX_Entity *process = hash_table_search_u64_raw(dmn_lnx_state->pid_ht, pid);
  if(process == 0)
  {
    process = dmn_lnx_nil_entity;
  }
  return process;
}

internal U64
dmn_lnx_thread_read_ip(DMN_LNX_Entity *thread)
{
  return regs_rip_from_arch_block(thread->arch, thread->reg_block);
}

internal U64
dmn_lnx_thread_read_sp(DMN_LNX_Entity *thread)
{
  return regs_rsp_from_arch_block(thread->arch, thread->reg_block);
}

internal void
dmn_lnx_thread_write_ip(DMN_LNX_Entity *thread, U64 ip)
{
  regs_arch_block_write_rip(thread->arch, thread->reg_block, ip);
  thread->is_reg_block_dirty = 1;
}

internal void
dmn_lnx_thread_write_sp(DMN_LNX_Entity *thread, U64 sp)
{
  regs_arch_block_write_rsp(thread->arch, thread->reg_block, sp);
  thread->is_reg_block_dirty = 1;
}

internal B32
dmn_lnx_thread_read_reg_block(DMN_LNX_Entity *thread)
{
  AssertAlways(dmn_lnx_state->tracer_tid == gettid());

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
      REGS_RegBlockX64 *dst     = thread->reg_block;
      
      //- rjf: read GPR
      B32 got_gpr = 0;
      {
        DMN_LNX_UserX64 ctx = {0};
        int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_len = sizeof(ctx), .iov_base = &ctx }));
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
          int   ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_len = process->xsave_size, .iov_base = xsave_raw }));
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
          int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_len = sizeof(*fxsave), .iov_base = fxsave }));
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
          dst->ftw            = src->ftw;
          dst->fop.u16        = src->fop;
          dst->fip.u64        = src->fip;
          dst->fdp.u64        = src->fdp;
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
          // compact register layout is not supported
          AssertAlways(xsave->header.xcomp_bv == 0);

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
            long peek_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_PEEKUSER, tid, PtrFromInt(offset), 0));
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
dmn_lnx_thread_write_reg_block(DMN_LNX_Entity *thread)
{
  AssertAlways(dmn_lnx_state->tracer_tid == thread->parent->tracer_tid);

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
      REGS_RegBlockX64 *src     = thread->reg_block;
      
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
        did_gpr = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_base = &dst, .iov_len = sizeof(dst) }) >= 0);
      }
      
      B32 did_fpr = 0;
      if(did_gpr)
      {
        Temp scratch = scratch_begin(0, 0);

        int xsave_result  = -1;
        int fxsave_result = -1;

        X64_FXSave dst_fxsave = {0};
        {
          dst_fxsave.fcw        = src->fcw.u16;
          dst_fxsave.fsw        = src->fsw.u16;
          dst_fxsave.ftw        = src->ftw;
          dst_fxsave.fop        = src->fop.u16;
          dst_fxsave.fip        = src->fip.u64;
          dst_fxsave.fdp        = src->fdp.u64;
          dst_fxsave.mxcsr      = src->mxcsr.u32;
          dst_fxsave.mxcsr_mask = src->mxcsr_mask.u32;

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
          U8        *xsave_raw = push_array(scratch.arena, U8, process->xsave_size);
          X64_XSave *dst       = (X64_XSave *)xsave_raw;
          dst->fxsave = dst_fxsave;
          dst->header.xstate_bv |= X64_XStateComponentFlag_FP;
          dst->header.xstate_bv |= X64_XStateComponentFlag_SSE;

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
              dst->header.xstate_bv |= X64_XStateComponentFlag_AVX;
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
              dst->header.xstate_bv |= X64_XStateComponentFlag_OPMASK;
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
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM_H;
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
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM;
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
              dst->header.xstate_bv |= X64_XStateComponentFlag_CETU;
            }
            else { Assert(0 && "invalid xsave size"); }
          }

          // xsave
          Assert(dst->header.xcomp_bv == 0); // must always be zero
          xsave_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_base = dst, .iov_len = process->xsave_size }));
          Assert(xsave_result >= 0);
        }

        // fallback to fxsave
        if(xsave_result < 0)
        {
          fxsave_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_base = &dst_fxsave, sizeof(dst_fxsave) }));
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
            int poke_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_POKEUSER, tid, PtrFromInt(offset), (void*)(uintptr_t)dr_s[n].u64));
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
    if(is_on) { reg_block->rflags.u64 |= X64_RFlag_Trap;  }
    else      { reg_block->rflags.u64 &= ~X64_RFlag_Trap; }
    thread->is_reg_block_dirty = 1;
    is_flag_set = 1;
  }break;
  case Arch_x86:
  case Arch_arm32:
  case Arch_arm64:
  {
    NotImplemented;
  }break;
  }
  Assert(is_flag_set);
  return is_flag_set;
}

////////////////////////////////

internal void
dmn_lnx_handle_not_attached(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind       = DMN_EventKind_Error;
  e->error_kind = DMN_ErrorKind_NotAttached;
}

internal DMN_LNX_Entity *
dmn_lnx_handle_create_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, pid_t tid)
{
  DMN_LNX_Entity *thread = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Thread);
  thread->id                 = tid;
  thread->arch               = process->arch;
  thread->reg_block          = push_array(process->arena, U8, regs_block_size_from_arch(process->arch));
  thread->thread_state       = DMN_LNX_ThreadState_Stopped;
  thread->is_reg_block_dirty = !dmn_lnx_thread_read_reg_block(thread);
  hash_table_push_u64_raw(dmn_lnx_state->arena, dmn_lnx_state->tid_ht, thread->id, thread);

  if(!thread->is_reg_block_dirty)
  {
    switch(thread->arch)
    {
    case Arch_Null: break;
    case Arch_x64: thread->thread_local_base = ((REGS_RegBlockX64 *)thread->reg_block)->fsbase.u64; break;
    case Arch_x86:
    case Arch_arm32:
    case Arch_arm64: { NotImplemented; } break;
    default: { InvalidPath; } break;
    }
  }

  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_CreateThread;
  e->process = dmn_lnx_handle_from_entity(process);
  e->thread  = dmn_lnx_handle_from_entity(thread);
  e->arch    = thread->arch;
  e->code    = thread->id;

  process->thread_count += 1;

  return thread;
}

internal DMN_LNX_Entity *
dmn_lnx_handle_create_process(Arena *arena, DMN_EventList *events, B32 debug_subprocesses, B32 is_rebased, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);

  DMN_LNX_Entity *process = 0;

  ELF_Hdr64           exe_ehdr         = dmn_lnx_ehdr_from_pid(pid);
  int                 memory_fd        = open((char*)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR);
  DMN_LNX_ProcessAuxv auxv             = dmn_lnx_auxv_from_pid(pid, exe_ehdr.e_ident[ELF_Identifier_Class]);
  Arch                arch             = arch_from_elf_machine(exe_ehdr.e_machine);
  U64                 rdebug_vaddr     = dmn_lnx_rdebug_vaddr_from_memory(memory_fd, auxv.base, is_rebased);
  U64                 rdebug_brk_vaddr = rdebug_vaddr + gnu_r_brk_offset_from_arch(arch);
  U64                 base_vaddr       = (auxv.phdr & ~(auxv.pagesz-1));
  U64                 rebase           = exe_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  DMN_LNX_PhdrInfo    phdr_info        = dmn_lnx_phdr_info_from_memory(memory_fd, exe_ehdr.e_ident[ELF_Identifier_Class], rebase, auxv.phdr, auxv.phent, auxv.phnum);
  Arena              *process_arena    = arena_alloc();

  ELF_Class dl_class;
  {
    ELF_Hdr64 ehdr = {0};
    if(!dmn_lnx_read_ehdr(memory_fd, auxv.base, &ehdr)) { Assert(0 && "failed to read interp's header"); }
    dl_class = ehdr.e_ident[ELF_Identifier_Class];
  }

  //
  // query xsave layout
  //
  U64             xcr0         = 0;
  U64             xsave_size   = 0;
  X64_XSaveLayout xsave_layout = {0};
  if(arch == Arch_x64)
  {
    X64_XSave xsave = {0};
    if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, pid, (void*)NT_X86_XSTATE, &(struct iovec){.iov_base = &xsave, .iov_len = sizeof(xsave) }) >= 0))
    {
      // Linux stores xcr0 bits in fxstate padding,
      // see https://github.com/torvalds/linux/blob/6548d364a3e850326831799d7e3ea2d7bb97ba08/arch/x86/include/asm/user.h#L25
      xcr0         = *(U64 *)((U8 *)&xsave + 464);
      xsave_size   = x64_get_xsave_size();
      xsave_layout = x64_get_xsave_layout(xcr0);
    }
    else { Assert(0 && "failed to get xstate"); }
  }

  //
  // gather probes
  //
  DMN_LNX_Probe **known_probes = push_array(process_arena, DMN_LNX_Probe *, DMN_LNX_ProbeType_Count);
  {
    String8 dl_path = dmn_lnx_dl_path_from_pid(scratch.arena, pid, auxv.base);
    int     dl_fd   = open((char *)dl_path.str, O_RDONLY);

    DMN_LNX_ProbeList probes = {0};
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

  //
  // init process
  //
  process = dmn_lnx_entity_alloc(dmn_lnx_state->entities_base, DMN_LNX_EntityKind_Process);
  process->id                            = pid;
  process->arch                          = arch;
  process->fd                            = memory_fd;
  process->tracer_tid                    = gettid();
  process->debug_subprocesses            = debug_subprocesses;
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
  hash_table_push_u64_raw(dmn_lnx_state->arena, dmn_lnx_state->pid_ht, pid, process);

  // trap probes
  for EachIndex(i, DMN_LNX_ProbeType_Count)
  {
    if(known_probes[i] == 0) { continue; }

    DMN_Trap *trap = push_array(process_arena, DMN_Trap, 1);
    trap->process = dmn_lnx_handle_from_entity(process);
    trap->vaddr   = known_probes[i]->pc;
    trap->id      = i;

    DMN_ActiveTrap *active_trap = dmn_set_trap(process_arena, trap);
    SLLQueuePush(process->first_probe_trap, process->last_probe_trap, active_trap);

    if(BUILD_DEBUG && arch == Arch_x64)
    {
      Assert(active_trap->swap_bytes.size == 1 && active_trap->swap_bytes.str[0] == 0x90);
    }
  }

  // push create process event
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind      = DMN_EventKind_CreateProcess;
  e->process   = dmn_lnx_handle_from_entity(process);
  e->arch      = process->arch;
  e->code      = pid;
  e->tls_model = DMN_TlsModel_Gnu; // TODO: use dynamic linker path to figure out correct enum here

  //
  // init main thread
  //
  DMN_LNX_Entity *thread = dmn_lnx_handle_create_thread(arena, events, process, pid);

  //
  // init main module
  //
  {
    DMN_LNX_Entity *module = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
    module->id                = auxv.execfn;
    module->base_vaddr        = base_vaddr;
    module->module_name_vaddr = auxv.execfn;
    module->is_main           = 1;

    hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, 0, module);
    hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, base_vaddr, module);

    // push load module event
    {
      DMN_Event *e = dmn_event_list_push(arena, events);
      e->kind             = DMN_EventKind_LoadModule;
      e->process          = dmn_lnx_handle_from_entity(process);
      e->thread           = dmn_lnx_handle_from_entity(thread);
      e->module           = dmn_lnx_handle_from_entity(module);
      e->arch             = process->arch;
      e->address          = base_vaddr;
      e->size             = dim_1u64(phdr_info.range);
      e->string           = dmn_lnx_read_string(arena, process->fd, auxv.execfn);
      e->elf_phdr_vrange  = r1u64(auxv.phdr, auxv.phdr + auxv.phent * auxv.phnum);
      e->elf_phdr_entsize = auxv.phent;
      e->tls_index        = 1;
      e->tls_offset       = 0;
    }
  }

  //
  // handshake complete
  //
  {
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind    = DMN_EventKind_HandshakeComplete;
    e->process = dmn_lnx_handle_from_entity(process);
    e->thread  = dmn_lnx_handle_from_entity(thread);
    e->arch    = process->arch;
  }

  //
  // update global state
  //
  dmn_lnx_state->active_process_count += 1;

  scratch_end(scratch);
  return process;
}

internal void
dmn_lnx_handle_exit_process(Arena *arena, DMN_EventList *events, pid_t pid)
{
  DMN_LNX_Entity *process = dmn_lnx_process_from_pid(pid);

  // rjf: generate unload-module events
  for(DMN_LNX_Entity *child = process->first; child != dmn_lnx_nil_entity; child = child->next)
  {
    if(child->kind == DMN_LNX_EntityKind_Module)
    {
      DMN_Event *e = dmn_event_list_push(arena, events);
      e->kind    = DMN_EventKind_UnloadModule;
      e->process = dmn_lnx_handle_from_entity(process);
      e->module  = dmn_lnx_handle_from_entity(child);
      // TODO(rjf): e->string = ...;
    }
  }

  // rjf: generate exit process event
  {
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind    = DMN_EventKind_ExitProcess;
    e->process = dmn_lnx_handle_from_entity(process);
    e->code    = process->main_thread_exit_code;
  }

  // free process
  if(close(process->fd) < 0) { Assert(0 && "failed to close memory descriptor"); }
  arena_release(process->arena);

  // rjf: eliminate entity tree
  hash_table_purge_u64(dmn_lnx_state->pid_ht, process->id);
  dmn_lnx_entity_release(process);

  //
  // update global state
  //
  AssertAlways(dmn_lnx_state->active_process_count > 0);
  dmn_lnx_state->active_process_count -= 1;
}

internal void
dmn_lnx_handle_exit_thread(Arena *arena, DMN_EventList *events, pid_t tid, U64 exit_code)
{
  DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Entity *process = thread->parent;

  // store main thread's exit code
  if(thread->id == process->id)
  {
    process->main_thread_exit_code = exit_code;
  }

  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_ExitThread;
  e->process = dmn_lnx_handle_from_entity(process);
  e->thread  = dmn_lnx_handle_from_entity(thread);
  hash_table_purge_u64(dmn_lnx_state->tid_ht, thread->id);
  dmn_lnx_entity_release(thread);
  process->thread_count -= 1;

  if(process->thread_count == 0)
  {
    dmn_lnx_handle_exit_process(arena, events, process->id);
  }
}

internal DMN_LNX_Entity *
dmn_lnx_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, U64 name_space_id, U64 module_name_vaddr, U64 base_vaddr)
{
  DMN_LNX_Entity *module = 0;

  // parse out module's ELF header
  ELF_Hdr64 module_ehdr = {0};
  if(!dmn_lnx_read_ehdr(process->fd, base_vaddr, &module_ehdr)) { goto exit; }

  // gather info about module
  U64              module_rebase     = module_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  U64              module_phdr_vaddr = module_rebase + module_ehdr.e_phoff;
  DMN_LNX_PhdrInfo module_phdr_info  = dmn_lnx_phdr_info_from_memory(process->fd, module_ehdr.e_ident[ELF_Identifier_Class], module_rebase, module_phdr_vaddr, module_ehdr.e_phentsize, module_ehdr.e_phnum);
  String8          module_name       = dmn_lnx_read_string(process->arena, process->fd, module_name_vaddr);

  // read TLS index and TLS offset
  U64 tls_index  = max_U64;
  U64 tls_offset = max_U64;
  if(dmn_lnx_state->is_tls_detected)
  {
    Rng1U64 tls_modid_range  = r1u64(dmn_lnx_state->tls_modid_desc.offset, dmn_lnx_state->tls_modid_desc.offset + dmn_lnx_state->tls_modid_desc.bit_size / 8);
    Rng1U64 tls_offset_range = r1u64(dmn_lnx_state->tls_offset_desc.offset, dmn_lnx_state->tls_offset_desc.offset + dmn_lnx_state->tls_offset_desc.bit_size / 8);
    tls_modid_range  = shift_1u64(tls_modid_range, base_vaddr);
    tls_offset_range = shift_1u64(tls_offset_range, base_vaddr);
    if(!dmn_lnx_read(process->fd, tls_modid_range, &tls_index))   { Assert(0 && "failed to read TLS index");  }
    if(!dmn_lnx_read(process->fd, tls_offset_range, &tls_offset)) { Assert(0 && "failed to read TLS offset"); }
  }

  // fill out module
  module = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
  module->id                = module_name_vaddr;
  module->base_vaddr        = base_vaddr;
  module->module_name_vaddr = module_name_vaddr;

  if(str8_match(module_name, str8_lit("linux-vdso.so.1"), 0)) { goto exit; }

  // push load event
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind             = DMN_EventKind_LoadModule;
  e->process          = dmn_lnx_handle_from_entity(process);
  e->module           = dmn_lnx_handle_from_entity(module);
  e->arch             = arch_from_elf_machine(module_ehdr.e_machine);
  e->address          = base_vaddr;
  e->size             = dim_1u64(module_phdr_info.range);
  e->string           = module_name;
  e->elf_phdr_vrange  = r1u64(module_phdr_vaddr, module_phdr_vaddr + module_ehdr.e_phentsize * module_ehdr.e_phnum);
  e->elf_phdr_entsize = module_ehdr.e_phentsize;
  e->tls_index        = tls_index;
  e->tls_offset       = tls_offset;

  exit:;
  return module;
}

internal void
dmn_lnx_handle_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, U64 name_space_id, U64 new_link_map_vaddr)
{
  GNU_LinkMap64 map = {0};
  for(U64 map_vaddr = new_link_map_vaddr; map_vaddr != 0; map_vaddr = map.next_vaddr)
  {
    // read out new link map item
    if(!dmn_lnx_read_linkmap(process->fd, map_vaddr, process->dl_class, &map)) { break; }

    // was module with this base already registered?
    DMN_LNX_Entity *module = hash_table_search_u64_raw(process->loaded_modules_ht, map.addr_vaddr);
    if(module == 0)
    {
      // load module
      module = dmn_lnx_load_module(arena, events, process, name_space_id, map.name_vaddr, map.addr_vaddr);

      // create mapping for base -> module
      hash_table_push_u64_raw(process->arena, process->loaded_modules_ht, map.addr_vaddr, module);
    }
  }
}

internal void
dmn_lnx_hanlde_unload_module(Arena *arena, DMN_EventList *events, DMN_LNX_Entity *process, U64 name_space_id, U64 rdebug_vaddr)
{
  Temp scratch = scratch_begin(&arena, 1);

  // flag every module as inactive
  for(DMN_LNX_Entity *module = process->first; module != dmn_lnx_nil_entity; module = module->next)
  {
    if(module->kind != DMN_LNX_EntityKind_Module) { continue; }
    module->is_live = 0;
  }

  // mark live modules
  B32 is_64bit = process->dl_class == ELF_Class_64;
  GNU_RDebugInfoList rdebug_list = gnu_parse_rdebug(scratch.arena, is_64bit, rdebug_vaddr, dmn_lnx_machine_op_mem_read, process);
  for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first) {
    GNU_LinkMapList link_map_list = gnu_parse_link_map_list(scratch.arena, is_64bit, rdebug_n->v.r_map, dmn_lnx_machine_op_mem_read, process);
    for EachNode(link_map_n, GNU_LinkMapNode, link_map_list.first) {
      DMN_LNX_Entity *module = hash_table_search_u64_raw(process->loaded_modules_ht, link_map_n->v.addr_vaddr);
      module->is_live = 1;
    }
  }

  // collect unloaded modules
  DMN_HandleList to_release = {0};
  for(DMN_LNX_Entity *module = process->first; module != dmn_lnx_nil_entity; module = module->next)
  {
    if(module->kind != DMN_LNX_EntityKind_Module) { continue; }
    if(module->is_live)                           { continue; }
    dmn_handle_list_push(scratch.arena, &to_release, dmn_lnx_handle_from_entity(module));
  }

  // push events and clean up internal structures
  for EachNode(n, DMN_HandleNode, to_release.first)
  {
    DMN_LNX_Entity *module = dmn_lnx_entity_from_handle(n->v);

    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind    = DMN_EventKind_UnloadModule;
    e->process = dmn_lnx_handle_from_entity(process);
    e->module  = dmn_lnx_handle_from_entity(module);
    e->string  = dmn_lnx_read_string(arena, process->fd, module->id);

    hash_table_purge_u64(process->loaded_modules_ht, module->base_vaddr);

    dmn_lnx_entity_release(module);
  }

  scratch_end(scratch);
}

internal void
dmn_lnx_handle_breakpoint(Arena *arena, DMN_EventList *events, DMN_ActiveTrap *user_traps, pid_t tid)
{
  DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Entity *process = thread->parent;
  U64             ip      = dmn_lnx_thread_read_ip(thread);

  // is this user trap?
  DMN_ActiveTrap *hit_user_trap = 0;
  {
    DMN_Handle process_handle = dmn_lnx_handle_from_entity(process);
    for EachNode(active_trap, DMN_ActiveTrap, user_traps)
    {
      if(MemoryCompare(&active_trap->trap->process, &process_handle, sizeof(DMN_Handle)) == 0)
      {
        if(active_trap->trap->vaddr == ip-1)
        {
          hit_user_trap = active_trap;
          break;
        }
      }
    }
  }

    // is this a probe trap?
  DMN_LNX_ProbeType probe_type = DMN_LNX_ProbeType_Null;
  if(hit_user_trap == 0)
  {
    for EachNode(active_trap, DMN_ActiveTrap, process->first_probe_trap)
    {
      if(active_trap->trap->vaddr == ip-1)
      {
        probe_type = active_trap->trap->id;
        break;
      }
    }
  }

  if(probe_type == DMN_LNX_ProbeType_InitComplete)
  {
    B32 is_init_completed = 0;

    DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_InitComplete];
    U64 name_space_id = 0, rdebug_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id)) { goto init_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_addr))   { goto init_complete_exit; }

    GNU_RDebugInfo64 rdebug = {0};
    if(!dmn_lnx_read_r_debug(process->fd, rdebug_addr, process->arch, &rdebug)) { goto init_complete_exit; }
    if(rdebug.r_version < 1)                                                    { goto init_complete_exit; }

    dmn_lnx_handle_load_module(arena, events, process, name_space_id, rdebug.r_map);

    is_init_completed = 1;
    init_complete_exit:;
    AssertAlways(is_init_completed);
  }
  else if(probe_type == DMN_LNX_ProbeType_RelocComplete)
  {
    B32 is_reloc_completed = 0;

    DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_RelocComplete];
    U64 name_space_id = 0, new_link_map_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id))     { goto reloc_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[2], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &new_link_map_addr)) { goto reloc_complete_exit; }

    dmn_lnx_handle_load_module(arena, events, process, name_space_id, new_link_map_addr);

    is_reloc_completed = 1;
    reloc_complete_exit:;
    AssertAlways(is_reloc_completed);
  }
  else if(probe_type == DMN_LNX_ProbeType_UnmapComplete)
  {
    B32 is_unmap_completed = 0;

    DMN_LNX_Probe *probe = process->probes[DMN_LNX_ProbeType_UnmapComplete];
    U64 name_space_id = 0, rdebug_vaddr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id)) { goto unmap_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_vaddr))  { goto unmap_complete_exit; }

    dmn_lnx_hanlde_unload_module(arena, events, process, name_space_id, rdebug_vaddr);

    is_unmap_completed = 1;
    unmap_complete_exit:;
    AssertAlways(is_unmap_completed);
  }

  if(probe_type == DMN_LNX_ProbeType_Null)
  {
    // rollback IP on user traps
    if(hit_user_trap)
    {
      U64 ip = dmn_lnx_thread_read_ip(thread);
      dmn_lnx_thread_write_ip(thread, ip - 1);
    }

    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind                = DMN_EventKind_Breakpoint;
    e->process             = dmn_lnx_handle_from_entity(process);
    e->thread              = dmn_lnx_handle_from_entity(thread);
    e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
  }
}

internal void
dmn_lnx_handle_data_breakpoint(Arena *arena, DMN_EventList *events, pid_t tid)
{
  DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Entity *process = thread->parent;

  B32 is_valid = 1;
  U64 address  = 0;
  switch(thread->arch)
  {
    case Arch_Null: break;
    case Arch_x64:
    {
      REGS_RegBlockX64 *regs_x64 = thread->reg_block;
      if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B0)
      {
        address = regs_x64->dr0.u64;
      }
      else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B1)
      {
        address = regs_x64->dr1.u64;
      }
      else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B2)
      {
        address = regs_x64->dr2.u64;
      }
      else if(regs_x64->dr6.u64 & X64_DebugStatusFlag_B3)
      {
        address = regs_x64->dr3.u64;
      }
      else
      {
        is_valid = 0;
      }
    }break;
    case Arch_x86:
    case Arch_arm32:
    case Arch_arm64:
      { NotImplemented; }break;
    default: { InvalidPath; } break;
  }

  if(is_valid)
  {
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind                = DMN_EventKind_Breakpoint;
    e->process             = dmn_lnx_handle_from_entity(process);
    e->thread              = dmn_lnx_handle_from_entity(thread);
    e->instruction_pointer = address;
  }
}

internal void
dmn_lnx_handle_halt(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind = DMN_EventKind_Halt;
}

internal void
dmn_lnx_handle_single_step(Arena *arena, DMN_EventList *events, pid_t tid)
{
  DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Entity *process = thread->parent;

  // clear single step flag
  dmn_lnx_set_single_step_flag(thread, 0);

  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_SingleStep;
  e->process             = dmn_lnx_handle_from_entity(process);
  e->thread              = dmn_lnx_handle_from_entity(thread);
  e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
}

internal void
dmn_lnx_handle_exception(Arena *arena, DMN_EventList *events, pid_t tid, U64 signo)
{
  // TODO(rjf): possible cases:
  // SIGABRT
  // SIGFPE
  // SIGSEGV
  // SIGILL

  DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Entity *process = thread->parent;

  thread->pass_through_signal = 1;
  thread->pass_through_signo  = signo;

  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_Exception;
  e->process             = dmn_lnx_handle_from_entity(process);
  e->thread              = dmn_lnx_handle_from_entity(thread);
  e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
  e->signo               = signo;

  if(signo == SIGSEGV)
  {
    siginfo_t si = {0};
    OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, tid, 0, &si));
    e->address = (U64)si.si_addr;
  }
}

internal DMN_LNX_Entity *
dmn_lnx_handle_attach(Arena *arena, DMN_EventList *events, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);

  DMN_LNX_Entity *process = 0;

  String8 task_path = push_str8f(scratch.arena, "/proc/%d/task", pid);
  DIR    *task_dirp = opendir((char *)task_path.str);
  if(task_dirp)
  {
    // create process
    process = dmn_lnx_handle_create_process(arena, events, 1, 1, pid);

    // extract threads from /proc/pid/task
    for(;;)
    {
      struct dirent *dirent = readdir(task_dirp);
      if(dirent == 0) { break; }

      String8 tid_str = str8_cstring_capped(dirent->d_name, dirent->d_name + NAME_MAX);
      if(str8_match(tid_str, str8_lit(".."), 0) || str8_match(tid_str, str8_lit("."), 0)) { continue; }
      U64     tid_64  = u64_from_str8(tid_str, 10);
      pid_t   tid     = (pid_t)tid_64;
      AssertAlways(tid == tid_64);

      if(tid == pid) { continue; } // main thread was created during create process sequence
      dmn_lnx_handle_create_thread(arena, events, process, tid);
    }

    // extract modules from r_debug
    B32                is_64bit      = process->dl_class == ELF_Class_64;
    GNU_RDebugInfoList rdebug_list   = gnu_parse_rdebug(scratch.arena, is_64bit, process->rdebug_vaddr, dmn_lnx_machine_op_mem_read, process);
    U64                name_space_id = 0;
    for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
    {
      dmn_lnx_handle_load_module(arena, events, process, name_space_id, rdebug_n->v.r_map);
      name_space_id += 1;
    }

    OS_LNX_RETRY_ON_EINTR(closedir(task_dirp));
  }

  //
  // handshake complete
  //
  {
    DMN_LNX_Entity *main_thread = dmn_lnx_thread_from_pid(pid);

    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind    = DMN_EventKind_HandshakeComplete;
    e->process = dmn_lnx_handle_from_entity(process);
    e->thread  = dmn_lnx_handle_from_entity(main_thread);
    e->arch    = process->arch;
  }

  scratch_end(scratch);
  return process;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  local_persist B32 was_inited;
  if(!was_inited)
  {
    was_inited = 1;

    local_persist DMN_LNX_State state;
    dmn_lnx_state = &state;

    dmn_lnx_state->arena          = arena_alloc();
    dmn_lnx_state->access_mutex   = mutex_alloc();
    dmn_lnx_state->entities_arena = arena_alloc(.reserve_size = GB(32), .commit_size = KB(64), .flags = ArenaFlag_NoChain);
    dmn_lnx_state->entities_base  = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 0);
    dmn_lnx_state->tid_ht         = hash_table_init(dmn_lnx_state->arena, 0x2000);
    dmn_lnx_state->pid_ht         = hash_table_init(dmn_lnx_state->arena, 0x400);
    dmn_lnx_state->halter_mutex   = mutex_alloc();
    dmn_lnx_entity_alloc(dmn_lnx_nil_entity, DMN_LNX_EntityKind_Root);

    // find offsets of TLS index and TLS offset in the link_map struct
    // 
    // TODO: assuming that target is using same libc version as debugger
    {
      DMN_LNX_DbDesc *tls_modid_desc  = dlsym(RTLD_DEFAULT, "_thread_db_link_map_l_tls_modid");
      DMN_LNX_DbDesc *tls_offset_desc = dlsym(RTLD_DEFAULT, "_thread_db_link_map_l_tls_offset");
      if(tls_modid_desc && tls_offset_desc)
      {
        if(tls_modid_desc->bit_size <= 64 && tls_offset_desc->bit_size <= 64)
        {
          dmn_lnx_state->tls_modid_desc  = *tls_modid_desc;
          dmn_lnx_state->tls_offset_desc = *tls_offset_desc;
          dmn_lnx_state->is_tls_detected = 1;
        }
        else { Assert(0 && "invalid TLS desc"); }
      }
    }
  }
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

  // setup target command line 
  U64    argc = params->cmd_line.node_count + 1;
  char **argv = push_array(scratch.arena, char *, argc);
  {
    U64 idx = 0;
    for(String8Node *n = params->cmd_line.first; n != 0; n = n->next, idx += 1)
    {
      argv[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
    }
  }
   
  // setup target environment
  U64    envc = os_lnx_state.default_env_count + params->env.node_count + 1;
  char **envp = push_array(scratch.arena, char *, envc);
  {
    // copy default environment
    MemoryCopyTyped(envp, os_lnx_state.default_env, os_lnx_state.default_env_count);

    // copy user environment
    U64 idx = os_lnx_state.default_env_count;
    for(String8Node *n = params->env.first; n != 0; n = n->next, idx += 1)
    {
      envp[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
    }
  }

  // create zero-terminated work directory path
  char *work_dir_path = (char *)push_str8_copy(scratch.arena, params->path).str;

  // fork process
  pid_t pid = fork();

  // child process
  if(pid == 0)
  {
    // change work directory to tracee
    if(OS_LNX_RETRY_ON_EINTR(chdir(work_dir_path)) < 0) { goto child_exit; }

    // replace process with target program
    if(OS_LNX_RETRY_ON_EINTR(execve(argv[0], argv, envp)) < 0) { goto child_exit; }

    child_exit:;
    abort();
  }
  // parent process
  else if(pid > 0)
  {
    B32 failed_to_seize = 1;

    // try to seize child process
    if(dmn_lnx_ptrace_seize(pid) < 0) { goto parent_exit; }

    // alloc pending process node
    DMN_LNX_ProcessLaunch *pending_proc = dmn_lnx_state->free_pids.first;
    if(pending_proc) { dmn_lnx_process_launch_list_remove(&dmn_lnx_state->free_pids, pending_proc); }
    else             { pending_proc = push_array(dmn_lnx_state->arena, DMN_LNX_ProcessLaunch, 1);   }

    // add pending proc node to the list
    *pending_proc = (DMN_LNX_ProcessLaunch){ params->debug_subprocesses, pid, DMN_LNX_ProcessLaunchState_Exec };
    dmn_lnx_process_launch_list_push_node(&dmn_lnx_state->pending_procs, pending_proc);

    // tracee was successfully seized
    failed_to_seize = 0;

    parent_exit:;
    if(failed_to_seize)
    {
      Assert(0 && "failed to ptrace child process");
      OS_LNX_RETRY_ON_EINTR(kill(SIGKILL, pid));
      OS_LNX_RETRY_ON_EINTR(waitpid(pid, 0, WNOHANG));
      pid = 0;
    }
  }
  
  scratch_end(scratch);
  return pid;
}

internal B32
dmn_ctrl_attach(DMN_CtrlCtx *ctx, U32 pid)
{
  B32 is_attached = 0;

  if(dmn_lnx_ptrace_seize(pid) >= 0)
  {
    if(OS_LNX_RETRY_ON_EINTR(kill(pid, SIGSTOP)) >= 0)
    {
      DMN_LNX_ProcessLaunch *pending_proc = dmn_lnx_state->free_pids.first;
      if(pending_proc) { dmn_lnx_process_launch_list_remove(&dmn_lnx_state->free_pids, pending_proc); }
      else             { pending_proc = push_array(dmn_lnx_state->arena, DMN_LNX_ProcessLaunch, 1);   }

      *pending_proc = (DMN_LNX_ProcessLaunch){ 1, pid, DMN_LNX_ProcessLaunchState_Attach };
      dmn_lnx_process_launch_list_push_node(&dmn_lnx_state->pending_procs, pending_proc);

      is_attached = 1;
    }
    else
    {
      OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, pid, 0, 0));
    }
  }

  return is_attached;
}

internal B32
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process, U32 exit_code)
{
  B32 result = 0;
  DMN_LNX_Entity *process_entity = dmn_lnx_entity_from_handle(process);
  if(process_entity != dmn_lnx_nil_entity &&
     OS_LNX_RETRY_ON_EINTR(kill(process_entity->id, SIGKILL)) != -1)
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
  if(process_entity != dmn_lnx_nil_entity &&
     OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, process_entity->id, 0, 0)) != -1)
  {
    result = 1;
  }
  return result;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  Assert(gettid() == dmn_lnx_state->tracer_tid);
  Temp scratch = scratch_begin(&arena, 1);
  DMN_EventList events = {0};

  mutex_take(dmn_lnx_state->halter_mutex);
  
  // write traps to memory
  DMN_ActiveTrap *active_trap_first = 0, *active_trap_last = 0;
  {
    HashTable *process_ht = hash_table_init(scratch.arena, dmn_lnx_state->active_process_count);
    for EachNode(n, DMN_TrapChunkNode, ctrls->traps.first)
    {
      for EachIndex(n_idx, n->count)
      {
        // skip hardware breakpoints
        DMN_Trap *trap = n->v+n_idx;
        if(trap->flags) { continue; }

        HashTable *active_trap_ht = hash_table_search_u64_raw(process_ht, trap->process.u64[0]);
        if(active_trap_ht == 0)
        {
          active_trap_ht = hash_table_init(scratch.arena, ctrls->traps.trap_count);
          hash_table_push_u64_raw(scratch.arena, process_ht, trap->process.u64[0], active_trap_ht);
        }

        // TODO: ctrl sends down duplicate traps
        DMN_ActiveTrap *is_set = hash_table_search_u64_raw(active_trap_ht, trap->vaddr);
        if(is_set) { continue; }

        // TODO: ctrl sends down traps for exited process
        DMN_LNX_Entity *process = dmn_lnx_entity_from_handle(trap->process);
        if(process == dmn_lnx_nil_entity) { continue; }

        // trap instruction
        DMN_ActiveTrap *active_trap = dmn_set_trap(scratch.arena, trap);

        // add trap to the active list
        SLLQueuePush(active_trap_first, active_trap_last, active_trap);

        // add (address -> trap)
        hash_table_push_u64_raw(scratch.arena, active_trap_ht, trap->vaddr, active_trap);
      }
    }
  }

  // enable single stepping
  if(!dmn_handle_match(ctrls->single_step_thread, dmn_handle_zero()))
  {
    DMN_LNX_Entity *single_step_thread = dmn_lnx_entity_from_handle(ctrls->single_step_thread);
    dmn_lnx_set_single_step_flag(single_step_thread, 1);
  }

  // schedule threads to run
  DMN_LNX_EntityList running_threads = {0};
  {
    for(DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first; process != dmn_lnx_nil_entity; process = process->next)
    {
      if(process->kind != DMN_LNX_EntityKind_Process) { continue; }
      
      //- rjf: determine if this process is frozen
      B32 process_is_frozen = 0;
      if(ctrls->run_entities_are_processes)
      {
        for EachIndex(idx, ctrls->run_entity_count)
        {
          if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(process)))
          {
            process_is_frozen = 1;
            break;
          }
        }
      }

      if(!process_is_frozen && process->thread_count == 0)
      {
        if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, (pid_t)process->id, 0, 0)) < 0) { InvalidPath; }
        break;
      }

      for(DMN_LNX_Entity *thread = process->first; thread != dmn_lnx_nil_entity; thread = thread->next)
      {
        if(thread->kind != DMN_LNX_EntityKind_Thread) { continue; }

        //- rjf: determine if this thread is frozen
        B32 is_frozen = 0;

        // rjf: not single-stepping? determine based on run controls freezing info
        if(dmn_handle_match(dmn_handle_zero(), ctrls->single_step_thread))
        {
          if(ctrls->run_entities_are_processes)
          {
            is_frozen = process_is_frozen;
          }
          else 
          {
            for EachIndex(idx, ctrls->run_entity_count)
            {
              if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(thread)))
              {
                is_frozen = 1;
                break;
              }
            }
          }
          if(ctrls->run_entities_are_unfrozen)
          {
            is_frozen ^= 1;
          }
        }
        // rjf: single-step? freeze if not the single-step thread.
        else
        {
          is_frozen = !dmn_handle_match(dmn_lnx_handle_from_entity(thread), ctrls->single_step_thread);
        }

        // resume thread
        if(!is_frozen)
        {
          AssertAlways(thread->thread_state == DMN_LNX_ThreadState_Stopped);

          // write registers
          if(thread->is_reg_block_dirty)
          {
            thread->is_reg_block_dirty = !dmn_lnx_thread_write_reg_block(thread);
          }

          // pass signal to the child process
          void *sig_code = 0;
          if(thread->pass_through_signal)
          {
            thread->pass_through_signal = 0;
            if(!ctrls->ignore_previous_exception)
            {
              sig_code = (void *)(uintptr_t)thread->pass_through_signo;
            }
          }

          // resume thread
          if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, (pid_t)thread->id, 0, sig_code)) >= 0)
          {
            thread->thread_state       = DMN_LNX_ThreadState_Running;
            thread->is_reg_block_dirty = 1;
            dmn_lnx_entity_list_push(scratch.arena, &running_threads, thread);
          }
          else
          {
            if(errno == ESRCH)
            {
              // Race note: In multithreaded programs, ptrace(PTRACE_CONT) may fail if a running thread calls
              // exit_group while we are resuming threads. The kernel begins tearing down the thread group so
              // the target might not be in a ptrace-soppped state anymore. Handle this by ignoring, on waitpid
              // we will get chance to report exit event.
              //
              // TODO: unfortunatley, kernel also sends us ESRCH whenever ptrace(PTRACE_CONT) is sent to a tracee
              // that is not in a ptace-stop, so how can we tell these errors apart?
            }
            else
            {
              InvalidPath;
            }
          }
        }
      }
    }
  }

  // wait for signals from the running threads
  {
    B32 is_halt_done = 0;
    for(U64 stopped_threads = 0;;)
    {
      // do not wait if there are no pending or active processes
      if(dmn_lnx_state->pending_procs.count == 0 && dmn_lnx_state->active_process_count == 0)
      {
        dmn_lnx_handle_not_attached(arena, &events);
        break;
      }

      // wait for a signal
      int   status  = 0;
      pid_t wait_id = 0; 
      {
        mutex_drop(dmn_lnx_state->halter_mutex);
        for(;;)
        {
          wait_id = OS_LNX_RETRY_ON_EINTR(waitpid(-1, &status, __WALL|__WNOTHREAD));
          if(wait_id == -1) { InvalidPath; } // TODO: graceful exit
          break;
        }
        mutex_take(dmn_lnx_state->halter_mutex);
      }

      // unpack status
      int wifexited   = WIFEXITED(status);
      int wifsignaled = WIFSIGNALED(status);
      int wifstopped  = WIFSTOPPED(status);
      int wstopsig    = WSTOPSIG(status);
      int event_code  = (status >> 16);

      // intercept signals meant for the process launch sequence
      {
        // pid -> pending process launch
        DMN_LNX_ProcessLaunch *pending_proc = 0;
        for EachNode(n, DMN_LNX_ProcessLaunch, dmn_lnx_state->pending_procs.first) { if(n->pid == wait_id) { pending_proc = n; break; } }

        if(pending_proc)
        {
          // process unexpectedly exited
          if(wifexited) { pending_proc->state = DMN_LNX_ProcessLaunchState_Exit; }

          if(pending_proc->state == DMN_LNX_ProcessLaunchState_Exec)
          {
            if(wstopsig == SIGTRAP && event_code == PTRACE_EVENT_EXEC)
            {
              // move pending process node to the free list
              dmn_lnx_process_launch_list_remove(&dmn_lnx_state->pending_procs, pending_proc);
              dmn_lnx_process_launch_list_push_node(&dmn_lnx_state->free_pids, pending_proc);

              // push create process events
              DMN_LNX_Entity *process = dmn_lnx_handle_create_process(arena, &events, pending_proc->debug_subprocesses, 0, wait_id);

              // override event code so the main code path does not create another process
              event_code = PTRACE_EVENT_STOP;
            }
            else
            {
              // shutdown process on abnormal init
              OS_LNX_RETRY_ON_EINTR(kill(wait_id, SIGKILL));
              pending_proc->state = DMN_LNX_ProcessLaunchState_Exit;
              continue;
            }
          }
          else if(pending_proc->state == DMN_LNX_ProcessLaunchState_Attach)
          {
            // move pending process node to the free list
            dmn_lnx_process_launch_list_remove(&dmn_lnx_state->pending_procs, pending_proc);
            dmn_lnx_process_launch_list_push_node(&dmn_lnx_state->free_pids, pending_proc);

            dmn_lnx_handle_attach(arena, &events, wait_id);

            // override event code
            event_code = PTRACE_EVENT_STOP;
          }
          else if(pending_proc->state == DMN_LNX_ProcessLaunchState_Exit)
          {
            // move pending process node to the free list
            dmn_lnx_process_launch_list_remove(&dmn_lnx_state->pending_procs, pending_proc);
            dmn_lnx_process_launch_list_push_node(&dmn_lnx_state->free_pids, pending_proc);
            continue;
          }
        }
      }

      if(wifstopped || wifsignaled || wifexited)
      {
        DMN_LNX_Entity *thread = dmn_lnx_thread_from_pid(wait_id);

        // kernel may send multiple stop signals for the same thread,
        // so count first stop signal and ignore subsequent signals
        B32 is_first_thread_to_stop = 0;
        if(thread->thread_state == DMN_LNX_ThreadState_Running)
        {
          is_first_thread_to_stop = (stopped_threads == 0);
          stopped_threads += 1;
        }

        // update thread state
        if(thread != dmn_lnx_nil_entity)
        {
          if(wifstopped && !wifsignaled && !wifexited)
          {
            thread->thread_state = DMN_LNX_ThreadState_Stopped;
          }
          else if(!wifstopped && (wifsignaled || wifexited))
          {
            thread->thread_state = DMN_LNX_ThreadState_Exited;
          }
          else { InvalidPath; }
        }

        // stop all other threads
        if(is_first_thread_to_stop)
        {
          for EachNode(thread, DMN_LNX_EntityNode, running_threads.first)
          {
            if(thread->v->thread_state == DMN_LNX_ThreadState_Running)
            {
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_INTERRUPT, thread->v->id, 0, 0)) < 0) { Assert(0 && "failed to interrupt process"); }
            }
          }
        }
      }

      // normal child exit via _exit or exit()
      if(wifexited)
      {
        dmn_lnx_handle_exit_thread(arena, &events, wait_id, WEXITSTATUS(status));
      }

      // exit because child did not handle a signal
      else if(wifsignaled)
      {
        dmn_lnx_handle_exit_thread(arena, &events, wait_id, WTERMSIG(status));
      }
      // thread or group stop
      else if(wifstopped)
      {
        // read thread registers
        {
          DMN_LNX_Entity *thread = dmn_lnx_thread_from_pid(wait_id);
          if(thread != dmn_lnx_nil_entity)
          {
            thread->is_reg_block_dirty = !dmn_lnx_thread_read_reg_block(thread);
            Assert(!thread->is_reg_block_dirty);
          }
        }

        if(wstopsig == SIGTRAP)
        {
          switch(event_code)
          {
            case 0:
            {
              // translate signal code to DEMON event
              siginfo_t siginfo = {0};
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, wait_id, 0, &siginfo)) >= 0)
              {
                switch(siginfo.si_code)
                {
                case SI_KERNEL:   { dmn_lnx_handle_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                case TRAP_BRKPT:  { dmn_lnx_handle_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                case TRAP_TRACE:  { dmn_lnx_handle_single_step(arena, &events, wait_id);                   } break;
                case TRAP_HWBKPT: { dmn_lnx_handle_data_breakpoint(arena, &events, wait_id);               } break;
                case TRAP_BRANCH: { NotImplemented; }break;
                case TRAP_UNK:    { NotImplemented; }break;
                default: { InvalidPath; } break;
                }
              } else { Assert(0 && "failed to get signal info"); }
            }break;
            case PTRACE_EVENT_FORK:
            {
              // grab child pid
              pid_t child_pid;
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &child_pid)) < 0) { AssertAlways(0 && "failed to grab child pid"); }

              // grab parent process
              DMN_LNX_Entity *parent_thread  = dmn_lnx_thread_from_pid(wait_id);
              DMN_LNX_Entity *parent_process = parent_thread->parent;

              // clean up in the child process instrumentation inherited during fork
              {
                DMN_Handle parent_process_handle = dmn_lnx_handle_from_entity(parent_process);
                DMN_Handle parent_thread_handle  = dmn_lnx_handle_from_entity(parent_thread);
                
                // at this point kernel cloned memory pages and writes in the child process trigger COW
                Temp temp = temp_begin(scratch.arena);
                int child_memory_fd = open((char*)str8f(temp.arena, "/proc/%d/mem", child_pid).str, O_RDWR);

                // if debugger is stepping over a line with fork we have to undo the return address spoof
                if(MemoryCompare(&parent_process_handle, &ctrls->spoof.process, sizeof(DMN_Handle)) == 0 &&
                    MemoryCompare(&parent_thread_handle, &ctrls->spoof.thread, sizeof(DMN_Handle)) == 0)
                {
                  B32 was_spoof_removed = dmn_lnx_write(child_memory_fd, r1u64(ctrls->spoof.vaddr, ctrls->spoof.vaddr + ctrls->spoof.size), &ctrls->spoof.old_ip);
                  AssertAlways(was_spoof_removed);
                }

                // remove probe traps from the child process
                for EachNode(n, DMN_ActiveTrap, parent_process->first_probe_trap)
                {
                  B32 is_written = dmn_lnx_write(child_memory_fd, r1u64(n->trap->vaddr, n->trap->vaddr + n->swap_bytes.size), n->swap_bytes.str);
                  AssertAlways(is_written);
                }

                // remove traps from the child process
                for EachNode(n, DMN_ActiveTrap, active_trap_first)
                {
                  if(MemoryCompare(&n->trap->process, &parent_process_handle, sizeof(DMN_Handle)) == 0)
                  {
                    B32 is_written = dmn_lnx_write(child_memory_fd, r1u64(n->trap->vaddr, n->trap->vaddr + n->swap_bytes.size), n->swap_bytes.str);
                    AssertAlways(is_written);
                  }
                }

                close(child_memory_fd);
                temp_end(temp);
              }

              if(parent_process->debug_subprocesses)
              {
                // create child process
                DMN_LNX_Entity *child_process = dmn_lnx_handle_create_process(arena, &events, 1, 1, child_pid);

                // copy modules from the parent process
                for(DMN_LNX_Entity *mod = parent_process->first; mod != dmn_lnx_nil_entity; mod = mod->next)
                {
                  if(mod->kind != DMN_LNX_EntityKind_Module) { continue; }
                  if(mod->is_main)                           { continue; }
                  dmn_lnx_load_module(arena, &events, child_process, mod->name_space_id, mod->module_name_vaddr, mod->base_vaddr);
                }
              }
              else
              {
                // user asked to detach from the child process
                if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, child_pid, 0, 0) < 0)) { Assert(0 && "unsuccessful detach"); }
              }
            }break;
            case PTRACE_EVENT_VFORK:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_CLONE:
            {
              pid_t new_pid;
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &new_pid)) >= 0)
              {
                DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(wait_id);
                DMN_LNX_Entity *process = thread->parent;
                dmn_lnx_handle_create_thread(arena, &events, process, new_pid);
              }
              else { Assert(0 && "failed to get new tid"); }
            }break;
            case PTRACE_EVENT_EXEC:
            {
              DMN_LNX_Entity *thread  = dmn_lnx_thread_from_pid(wait_id);
              DMN_LNX_Entity *process = thread->parent;
              if(process->debug_subprocesses)
              {
                dmn_lnx_handle_exit_thread(arena, &events, wait_id, 0);
                DMN_LNX_Entity *process = dmn_lnx_handle_create_process(arena, &events, 1, 0, wait_id);
              }
              else
              {
                if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, wait_id, 0, 0)) >= 0)
                {
                  dmn_lnx_handle_exit_thread(arena, &events, wait_id, 0);
                }
                else { Assert(0 && "failed to detach"); }
              }
            }break;
            case PTRACE_EVENT_VFORK_DONE:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_EXIT:
            {
              AssertAlways(0 && "exit tracing is disabled, this event should not be delivered");
            }break;
            case PTRACE_EVENT_SECCOMP:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_STOP:
            {
              DMN_LNX_Entity *thread = dmn_lnx_thread_from_pid(wait_id);
              DMN_LNX_Entity *process = thread->parent;
              if(process->expect_user_interrupt)
              {
                process->expect_user_interrupt = 0;
                dmn_lnx_handle_halt(arena, &events);
              }
            }break;
            default: { Assert(0 && "unexpected ptrace code"); } break;
          }
        }
        else if(wstopsig == SIGSTOP)
        {
          siginfo_t siginfo = {0};
          if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, wait_id, 0, &siginfo)) >= 0)
          {
            // finish halting if this is our SIGSTOP
            if(siginfo.si_pid == dmn_lnx_state->halter_tid)
            {
              is_halt_done = 1;
            }
            else
            {
              // TODO: handle SIGSTOP
            }
          }
        }
        else
        {
          dmn_lnx_handle_exception(arena, &events, wait_id, wstopsig);
        }
      }
      else { Assert(0 && "unexpected stop code"); }

      // do not wait if all threads are stopped and there are no launching processes
      if(stopped_threads >= running_threads.count && dmn_lnx_state->pending_procs.count == 0)
      {
        break;
      }
    }

    // finalize halter state
    if(is_halt_done)
    {
      // push event
      dmn_lnx_handle_halt(arena, &events);

      // reset state
      dmn_lnx_state->halter_tid     = 0;
      dmn_lnx_state->halt_code      = 0;
      dmn_lnx_state->halt_user_data = 0;
      dmn_lnx_state->is_halting     = 0;
    }
  }

  // restore original instruction bytes
  for EachNode(active_trap, DMN_ActiveTrap, active_trap_first)
  {
    // skip process that exited during the wait
    DMN_LNX_Entity *process = dmn_lnx_entity_from_handle(active_trap->trap->process);
    if(process == dmn_lnx_nil_entity) { continue; }

    if(!dmn_process_write(active_trap->trap->process, r1u64(active_trap->trap->vaddr, active_trap->trap->vaddr + active_trap->swap_bytes.size), active_trap->swap_bytes.str))
    {
      Assert(0 && "failed to restore original instruction bytes");
    }
  }
  
  mutex_drop(dmn_lnx_state->halter_mutex);
  scratch_end(scratch);
  return events;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
  mutex_take(dmn_lnx_state->halter_mutex);
  if(dmn_lnx_state->active_process_count)
  {
    dmn_lnx_state->halter_tid     = gettid();
    dmn_lnx_state->halt_code      = code;
    dmn_lnx_state->halt_user_data = user_data;
    for (DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first; process != dmn_lnx_nil_entity; process = process->next)
    {
      if(process->kind != DMN_LNX_EntityKind_Process) { continue; }
      if(OS_LNX_RETRY_ON_EINTR(kill(process->id, SIGSTOP)) < 0) { Assert(0 && "failed to send SIGSTOP"); }
    }
  }
  mutex_drop(dmn_lnx_state->halter_mutex);
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
  U64 tls_root_vaddr = max_U64;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
    switch (thread->arch)
    {
      case Arch_Null: {} break;
      case Arch_x64:
      {
        DMN_LNX_Entity   *process   = thread->parent;
        REGS_RegBlockX64 *reg_block = thread->reg_block;
        U64 dtv_pointer = 0;
        if(dmn_lnx_read_struct(process->fd, reg_block->fsbase.u64 + 8, &dtv_pointer))
        {
          tls_root_vaddr = dtv_pointer;
        }
      } break;
      case Arch_x86:
      case Arch_arm32:
      case Arch_arm64:
      {
        NotImplemented;
      } break;
      default: { InvalidPath; } break;
    }
  }
  return tls_root_vaddr;
}

internal B32
dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
    if(thread != dmn_lnx_nil_entity)
    {
      U64 reg_block_size = regs_block_size_from_arch(thread->arch);
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
    if(thread == dmn_lnx_nil_entity)
    {
      MemoryZero(reg_block, reg_block_size);
    }
    else
    {
      MemoryCopy(thread->reg_block, reg_block, reg_block_size);
      thread->is_reg_block_dirty = 1;
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
