// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Includes

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

////////////////////////////////

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
  U64 read_size = dmn_lnx_read(*(int *)ud, r1u64(addr, addr + buffer_size), buffer);
  return read_size == buffer_size ? MachineOpResult_Ok : MachineOpResult_Fail;
}

internal int
dmn_lnx_ptrace_seize(pid_t pid)
{
  // TODO: PTRACE_O_TRACEVFORK | PTRACE_O_TRACEVFORKDONE | PTRACE_O_TRACEFORK
  return OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL | PTRACE_O_TRACECLONE));
}

internal String8
dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 exe_link_path = str8f(scratch.arena, "/proc/%d/exe", pid);
  String8List parts = {0};
  int readlink_result = 0;
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

internal String8
dmn_lnx_dl_path_from_pid(Arena *arena, pid_t pid, U64 auxv_base)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 dl_path = {0};
  
  int maps_fd = OS_LNX_RETRY_ON_EINTR(open((char *)str8f(scratch.arena, "/proc/%d/maps", pid).str, O_RDONLY));
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
    
    OS_LNX_RETRY_ON_EINTR(close(maps_fd));
  }
  else { Assert(0 && "failed to open DL fd"); }
  
  scratch_end(scratch);
  Assert(dl_path.size);
  return dl_path;
}

internal ELF_Hdr64
dmn_lnx_ehdr_from_pid(pid_t pid)
{
  Temp scratch = scratch_begin(0, 0);
  
  ELF_Hdr64 exe     = {0};
  B32       is_read = 0;
  
  char *exe_path = (char *)str8f(scratch.arena, "/proc/%d/exe", pid).str;
  int   exe_fd   = OS_LNX_RETRY_ON_EINTR(open(exe_path, O_RDONLY));
  
  if(exe_fd >= 0)
  {
    is_read = elf_read_ehdr(dmn_lnx_machine_op_mem_read, &exe_fd, 0, &exe);
    OS_LNX_RETRY_ON_EINTR(close(exe_fd));
  }
  
  Assert(is_read);
  scratch_end(scratch);
  return exe;
}

internal DMN_LNX_Auxv
dmn_lnx_auxv_from_pid(pid_t pid, ELF_Class elf_class)
{
  Temp scratch = scratch_begin(0, 0);
  DMN_LNX_Auxv result = {0};
  
  // rjf: open aux data
  String8 auxv_path = str8f(scratch.arena, "/proc/%d/auxv", pid);
  int auxv_fd = OS_LNX_RETRY_ON_EINTR(open((char *)auxv_path.str, O_RDONLY));
  
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
        case ELF_AuxType_Null:   goto brkloop; break;
        case ELF_AuxType_Base:   result.base   = auxv.a_val; break;
        case ELF_AuxType_Phnum:  result.phnum  = auxv.a_val; break;
        case ELF_AuxType_Phent:  result.phent  = auxv.a_val; break;
        case ELF_AuxType_Phdr:   result.phdr   = auxv.a_val; break;
        case ELF_AuxType_ExecFn: result.execfn = auxv.a_val; break;
        case ELF_AuxType_Pagesz: result.pagesz = auxv.a_val; break;
      }
    }
    brkloop:;
    OS_LNX_RETRY_ON_EINTR(close(auxv_fd));
  }
  
  scratch_end(scratch);
  return result;
}

internal DMN_LNX_Thread *
dmn_lnx_thread_from_pid(pid_t tid)
{
  return hash_table_search_u64_raw(dmn_lnx_state->tid_ht, tid);
}

internal DMN_LNX_Process *
dmn_lnx_process_from_pid(pid_t pid)
{
  return hash_table_search_u64_raw(dmn_lnx_state->pid_ht, pid);
}

internal Rng1U64
dmn_lnx_compute_image_vrange(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum)
{ 
  Rng1U64 result = { .min = max_U64 };
  
  for(U64 ph_cursor = e_phaddr, ph_opl = (e_phaddr + e_phentsize * e_phnum); ph_cursor < ph_opl; ph_cursor += e_phentsize)
  {
    ELF_Phdr64 phdr = {0};
    if(elf_read_phdr(dmn_lnx_machine_op_mem_read, &memory_fd, ph_cursor, elf_class, &phdr) != MachineOpResult_Ok)
    {
      Assert(0 && "unable to read a program header");
    }
    
    if(phdr.p_type  == ELF_PType_Load)
    {
      U64 min = rebase + phdr.p_vaddr;
      U64 max = rebase + phdr.p_vaddr + phdr.p_memsz;
      result.min = Min(result.min, min);
      result.max = Max(result.max, max);
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
    if(elf_read_dyn(dmn_lnx_machine_op_mem_read, &memory_fd, dynamic_cursor, elf_class, &dyn) != MachineOpResult_Ok) { Assert(0 && "unable to read dynamic"); }
    
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
dmn_lnx_find_dynamic_phdr(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum)
{
  U64 result = max_U64;
  
  for(U64 ph_cursor = e_phaddr, ph_opl = (e_phaddr + e_phentsize * e_phnum); ph_cursor < ph_opl; ph_cursor += e_phentsize)
  {
    ELF_Phdr64 phdr = {0};
    if(elf_read_phdr(dmn_lnx_machine_op_mem_read, &memory_fd, ph_cursor, elf_class, &phdr) != MachineOpResult_Ok)
    {
      Assert(0 && "unable to read a program header");
    }
    
    if(phdr.p_type == ELF_PType_Dynamic)
    {
      result = rebase + phdr.p_vaddr;
      break;
    }
  }
  
  return result;
}

internal U64
dmn_lnx_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vbase, B32 is_rebased)
{
  Temp scratch = scratch_begin(0, 0);
  
  U64 rdebug_vaddr = 0;
  
  // load DL's header
  ELF_Hdr64 ehdr = {0};
  if(elf_read_ehdr(dmn_lnx_machine_op_mem_read, &memory_fd, loader_vbase, &ehdr) != MachineOpResult_Ok) { Assert(0 && "failed to read interp's header"); goto exit; }
  
  U64       rebase    = ehdr.e_type == ELF_Type_Dyn ? loader_vbase : 0;
  ELF_Class elf_class = ehdr.e_ident[ELF_Identifier_Class];
  
  // find dynamic program header
  U64 phdr_vaddr    = loader_vbase + ehdr.e_phoff;
  U64 dynamic_vaddr = dmn_lnx_find_dynamic_phdr(memory_fd, elf_class, rebase, phdr_vaddr, ehdr.e_phentsize, ehdr.e_phentsize);
  
  // extract necessary info out of dynamic program header
  U64                 dynamic_info_rebase = is_rebased ? 0 : rebase;
  DMN_LNX_DynamicInfo dynamic_info        = dmn_lnx_dynamic_info_from_memory(memory_fd, elf_class, dynamic_info_rebase, dynamic_vaddr);
  
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
      if(elf_read_symbol(dmn_lnx_machine_op_mem_read, &memory_fd, dynamic_info.symtab_vaddr + symbol_idx * dynamic_info.symtab_entry_size, elf_class, &symbol) != MachineOpResult_Ok)
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

internal DMN_LNX_ProbeList
dmn_lnx_read_probes(Arena *arena, int fd, U64 offset, U64 image_base)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  DMN_LNX_ProbeList probes = {0};
  
  ELF_Hdr64 ehdr = {0};
  if(elf_read_ehdr(dmn_lnx_machine_op_mem_read, &fd, offset, &ehdr) != MachineOpResult_Ok) { goto exit; }
  
  U64        strtab_shdr_offset = offset + ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize;
  ELF_Shdr64 strtab_shdr        = {0};
  if(elf_read_shdr(dmn_lnx_machine_op_mem_read, &fd, strtab_shdr_offset, ehdr.e_ident[ELF_Identifier_Class], &strtab_shdr) != MachineOpResult_Ok) { goto exit; }
  
  B32 found_probes      = 0;
  B32 found_probes_base = 0;
  ELF_Shdr64 text_shdr         = {0};
  ELF_Shdr64 stapsdt_base_shdr = {0};
  ELF_Shdr64 stapsdt_shdr      = {0};
  for(U64 shdr_off = offset + ehdr.e_shoff, shdr_opl = shdr_off + ehdr.e_shentsize * ehdr.e_shnum;
      shdr_off < shdr_opl;
      shdr_off += ehdr.e_shentsize) {
    ELF_Shdr64 shdr = {0};
    if(elf_read_shdr(dmn_lnx_machine_op_mem_read, &fd, shdr_off, ehdr.e_ident[ELF_Identifier_Class], &shdr) != MachineOpResult_Ok) { goto exit; }
    
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

internal
STAP_MEMORY_READ(dmn_lnx_stap_memory_read)
{
  DMN_LNX_Process *process = raw_ctx;
  U64 bytes_read = dmn_lnx_read(process->fd, r1u64(addr, addr + read_size), buffer);
  return bytes_read == read_size;
}

internal DMN_LNX_Entity *
dmn_lnx_entity_alloc(DMN_LNX_EntityKind kind)
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
  entity->gen += 1;
  entity->kind = kind;
  return entity;
}

internal DMN_LNX_Process *
dmn_lnx_process_alloc(pid_t pid, DMN_LNX_ProcessState state, DMN_LNX_Process *parent_process, B32 debug_subprocesses, B32 is_cow)
{
  Temp scratch = scratch_begin(0, 0);
  
  DMN_LNX_Process *process = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_Process)->process;
  process->pid                = pid;
  process->fd                 = OS_LNX_RETRY_ON_EINTR(open((char *)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR));
  process->state              = state;
  process->debug_subprocesses = debug_subprocesses;
  process->is_cow             = is_cow;
  process->parent_process     = parent_process;
  
  // update pending process tracker
  if(state != DMN_LNX_ProcessState_Normal)
  {
    dmn_lnx_state->process_pending_creation += 1;
  }
  
  // add process to the list
  DLLPushBack(dmn_lnx_state->first_process, dmn_lnx_state->last_process, process);
  dmn_lnx_state->process_count += 1;
  
  // push pid -> DMN_LNX_Process mapping
  hash_table_push_u64_raw(dmn_lnx_state->arena, dmn_lnx_state->pid_ht, pid, process);
  
  scratch_end(scratch);
  return process;
}

internal DMN_LNX_ProcessCtx *
dmn_lnx_process_ctx_alloc(DMN_LNX_Process *process, B32 is_rebased)
{
  DMN_LNX_ProcessCtx *ctx = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_ProcessCtx)->process_ctx;
  
  ELF_Hdr64     exe_ehdr     = dmn_lnx_ehdr_from_pid(process->pid);
  DMN_LNX_Auxv  auxv         = dmn_lnx_auxv_from_pid(process->pid, exe_ehdr.e_ident[ELF_Identifier_Class]);
  Arch          arch         = arch_from_elf_machine(exe_ehdr.e_machine);
  U64           rdebug_vaddr = dmn_lnx_rdebug_vaddr_from_memory(process->fd, auxv.base, is_rebased);
  U64           base_vaddr   = (auxv.phdr & ~(auxv.pagesz-1));
  U64           rebase       = exe_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  Rng1U64       image_vrange = dmn_lnx_compute_image_vrange(process->fd, exe_ehdr.e_ident[ELF_Identifier_Class], rebase, auxv.phdr, auxv.phent, auxv.phnum);
  Arena        *ctx_arena    = arena_alloc();
  
  ELF_Class dl_class;
  {
    ELF_Hdr64 ehdr = {0};
    if(elf_read_ehdr(dmn_lnx_machine_op_mem_read, &process->fd, auxv.base, &ehdr) != MachineOpResult_Ok) { Assert(0 && "failed to read interp's header"); }
    dl_class = ehdr.e_ident[ELF_Identifier_Class];
  }
  
  // query xsave layout
  U64             xcr0         = 0;
  U64             xsave_size   = 0;
  X64_XSaveLayout xsave_layout = {0};
  if(arch == Arch_x64)
  {
    X64_XSave xsave = {0};
    if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, process->pid, (void *)NT_X86_XSTATE, &(struct iovec){.iov_base = &xsave, .iov_len = sizeof(xsave) }) >= 0))
    {
      // Linux stores xcr0 bits in fxstate padding,
      // see https://github.com/torvalds/linux/blob/6548d364a3e850326831799d7e3ea2d7bb97ba08/arch/x86/include/asm/user.h#L25
      xcr0         = *(U64 *)((U8 *)&xsave + 464);
      xsave_size   = x64_get_xsave_size();
      xsave_layout = x64_get_xsave_layout(xcr0);
    }
    else { Assert(0 && "failed to get xstate"); }
  }
  
  // gather probes
  DMN_LNX_Probe **known_probes = push_array(ctx_arena, DMN_LNX_Probe *, DMN_LNX_ProbeType_Count);
  {
    Temp scratch = scratch_begin(0, 0);
    
    String8 dl_path = dmn_lnx_dl_path_from_pid(scratch.arena, process->pid, auxv.base);
    int dl_fd = OS_LNX_RETRY_ON_EINTR(open((char *)dl_path.str, O_RDONLY));
    
    DMN_LNX_ProbeList probes = {0};
    if(dl_fd >= 0)
    {
      probes = dmn_lnx_read_probes(ctx_arena, dl_fd, 0, auxv.base);
      OS_LNX_RETRY_ON_EINTR(close(dl_fd));
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
    
    scratch_end(scratch);
  }
  
  ctx = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_ProcessCtx)->process_ctx;
  ctx->arena             = arena_alloc();
  ctx->arch              = arch;
  ctx->rdebug_vaddr      = rdebug_vaddr;
  ctx->dl_class          = dl_class;
  ctx->loaded_modules_ht = hash_table_init(ctx_arena, 0x1000);
  ctx->probes            = known_probes;
  ctx->xcr0              = xcr0;
  ctx->xsave_size        = Max(xsave_size, sizeof(X64_XSave));
  ctx->xsave_layout      = xsave_layout;
  
  // create main module
  DMN_LNX_Module *main_module = dmn_lnx_module_alloc(ctx, process->fd, base_vaddr, auxv.execfn, 1, 1);
  
  // glibc has a shortcut mapping for the main module
  hash_table_push_u64_raw(ctx->arena, ctx->loaded_modules_ht, 0, main_module);
  
  return ctx;
}

internal DMN_LNX_Thread *
dmn_lnx_thread_alloc(DMN_LNX_Process *process, DMN_LNX_ThreadState thread_state, pid_t tid)
{
  void *reg_block;
  if(process->ctx->free_reg_blocks.node_count)
  {
    String8Node *n = str8_list_pop_front(&process->ctx->free_reg_blocks);
    reg_block = n->string.str;
    str8_list_push_node(&process->ctx->free_reg_block_nodes, n);
  }
  else
  {
    U64 reg_block_size = regs_block_size_from_arch(process->ctx->arch);
    reg_block = push_array(process->ctx->arena, U8, reg_block_size);
  }
  
  DMN_LNX_Thread *thread = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_Thread)->thread;
  thread->tid       = tid;
  thread->state     = thread_state;
  thread->process   = process;
  thread->reg_block = reg_block;
  if(thread_state == DMN_LNX_ThreadState_Stopped)
  {
    thread->is_reg_block_dirty = !dmn_lnx_thread_read_reg_block(thread);
  }
  
  // add thread to the list
  DLLPushBack(process->first_thread, process->last_thread, thread);
  process->thread_count += 1;
  
  // push tid -> thread mapping
  hash_table_push_u64_raw(dmn_lnx_state->arena, dmn_lnx_state->tid_ht, thread->tid, thread);
  
  // update global thread counter
  if(thread_state == DMN_LNX_ThreadState_PendingCreation)
  {
    dmn_lnx_state->threads_pending_creation += 1;
  }
  
  return thread;
}

internal DMN_LNX_Module *
dmn_lnx_module_alloc(DMN_LNX_ProcessCtx *ctx, int memory_fd, U64 base_vaddr, U64 name_vaddr, U64 name_space_id, B32 is_main)
{
  DMN_LNX_Module *module = hash_table_search_u64_raw(ctx->loaded_modules_ht, base_vaddr);
  if(module) { goto exit; }
  
  // parse out module's ELF header
  ELF_Hdr64 module_ehdr = {0};
  if(elf_read_ehdr(dmn_lnx_machine_op_mem_read, &memory_fd, base_vaddr, &module_ehdr) != MachineOpResult_Ok) { goto exit; }
  
  // gather info about module
  U64     module_rebase     = module_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  U64     module_phdr_vaddr = module_rebase + module_ehdr.e_phoff;
  Rng1U64 module_vrange     = dmn_lnx_compute_image_vrange(memory_fd, module_ehdr.e_ident[ELF_Identifier_Class], module_rebase, module_phdr_vaddr, module_ehdr.e_phentsize, module_ehdr.e_phnum);
  
  // read TLS index and TLS offset
  U64 tls_index  = max_U64;
  U64 tls_offset = max_U64;
  if(is_main)
  {
    tls_index  = 1;
    tls_offset = 0;
  }
  else
  {
    if(dmn_lnx_state->is_tls_detected)
    {
      Rng1U64 tls_modid_range  = r1u64(dmn_lnx_state->tls_modid_desc.offset, dmn_lnx_state->tls_modid_desc.offset + dmn_lnx_state->tls_modid_desc.bit_size / 8);
      Rng1U64 tls_offset_range = r1u64(dmn_lnx_state->tls_offset_desc.offset, dmn_lnx_state->tls_offset_desc.offset + dmn_lnx_state->tls_offset_desc.bit_size / 8);
      tls_modid_range  = shift_1u64(tls_modid_range, base_vaddr);
      tls_offset_range = shift_1u64(tls_offset_range, base_vaddr);
      if(!dmn_lnx_read(memory_fd, tls_modid_range, &tls_index))   { Assert(0 && "failed to read TLS index");  }
      if(!dmn_lnx_read(memory_fd, tls_offset_range, &tls_offset)) { Assert(0 && "failed to read TLS offset"); }
    }
  }
  
  module = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_Module)->module;
  module->base_vaddr    = base_vaddr;
  module->name_vaddr    = name_vaddr;
  module->name_space_id = name_space_id;
  module->size          = dim_1u64(module_vrange);
  module->phvaddr       = base_vaddr + module_ehdr.e_phoff;
  module->phcount       = module_ehdr.e_phnum;
  module->phentsize     = module_ehdr.e_phentsize;
  module->tls_index     = tls_index;
  module->tls_offset    = tls_offset;
  module->is_main       = is_main;
  
  // add module to the list
  DLLPushBack(ctx->first_module, ctx->last_module, module);
  ctx->module_count += 1;
  
  // push base address -> module mapping
  hash_table_push_u64_raw(ctx->arena, ctx->loaded_modules_ht, base_vaddr, module);
  
  exit:;
  return module;
}

internal void
dmn_lnx_entity_release(DMN_LNX_Entity *entity)
{
  U32 gen = entity->gen + 1;
  MemoryZeroStruct(entity);
  entity->gen = gen;
  SLLStackPush(dmn_lnx_state->free_entity, entity);
}

internal void
dmn_lnx_process_release(DMN_LNX_Process *process)
{
  // update global state
  AssertAlways(dmn_lnx_state->process_count > 0);
  DLLRemove(dmn_lnx_state->first_process, dmn_lnx_state->last_process, process);
  dmn_lnx_state->process_count -= 1;
  
  // update pending process tracker
  if(process->state != DMN_LNX_ProcessState_Normal)
  {
    Assert(dmn_lnx_state->process_pending_creation > 0);
    dmn_lnx_state->process_pending_creation -= 1;
  }
  
  // close memory handle
  if(OS_LNX_RETRY_ON_EINTR(close(process->fd)) < 0) { Assert(0 && "failed to close memory descriptor"); }
  
  // remove pid mapping
  hash_table_purge_u64(dmn_lnx_state->pid_ht, process->pid);
  
  // release the context
  if(process->ctx)
  {
    dmn_lnx_process_ctx_release(process->ctx);
  }
  
  // release process entity
  dmn_lnx_entity_release((DMN_LNX_Entity *)process);
}

internal void
dmn_lnx_process_ctx_release(DMN_LNX_ProcessCtx *ctx)
{
  Assert(ctx->ref_count > 0);
  ctx->ref_count -= 1;
  
  if(ctx->ref_count == 0)
  {
    arena_release(ctx->arena);
    dmn_lnx_entity_release((DMN_LNX_Entity *)ctx);
  }
}

internal void
dmn_lnx_thread_release(DMN_LNX_Thread *thread)
{
  DMN_LNX_Process *process = thread->process;
  
  // purge tid mapping
  hash_table_purge_u64(dmn_lnx_state->tid_ht, thread->tid);
  
  // update global thread counter
  if(thread->state == DMN_LNX_ThreadState_PendingCreation)
  {
    AssertAlways(dmn_lnx_state->threads_pending_creation > 0);
    dmn_lnx_state->threads_pending_creation -= 1;
  }
  
  // remove thread from the list
  Assert(process->thread_count > 0);
  DLLRemove(process->first_thread, process->last_thread, thread);
  process->thread_count -= 1;
  
  // push reg block to the free list
  String8Node *reg_block_node;
  if(process->ctx->free_reg_block_nodes.node_count)
  {
    reg_block_node = str8_list_pop_front(&process->ctx->free_reg_block_nodes);
  }
  else
  {
    reg_block_node = push_array(process->ctx->arena, String8Node, 1);
  }
  reg_block_node->string = str8(thread->reg_block, 0);
  str8_list_push_node(&process->ctx->free_reg_blocks, reg_block_node);
  
  dmn_lnx_entity_release((DMN_LNX_Entity *)thread);
}

internal void
dmn_lnx_module_release(DMN_LNX_ProcessCtx *ctx, DMN_LNX_Module *module)
{
  // remove module from the list
  Assert(ctx->module_count > 0);
  DLLRemove(ctx->first_module, ctx->last_module, module);
  ctx->module_count -= 1;
  
  // purge base addr -> module mapping
  hash_table_purge_u64(ctx->loaded_modules_ht, module->base_vaddr);
  
  dmn_lnx_entity_release((DMN_LNX_Entity *)module);
}

internal DMN_LNX_ProcessCtx *
dmn_lnx_process_ctx_clone(DMN_LNX_Process *new_owner, DMN_LNX_ProcessCtx *ctx)
{
  DMN_LNX_ProcessCtx *result = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_ProcessCtx)->process_ctx;
  
  result->arena             = arena_alloc();
  result->arch              = ctx->arch;
  result->rdebug_vaddr      = ctx->rdebug_vaddr;
  result->dl_class          = ctx->dl_class;
  result->loaded_modules_ht = hash_table_init(result->arena, ctx->loaded_modules_ht->cap);
  result->xcr0              = ctx->xcr0;
  result->xsave_size        = ctx->xsave_size;
  result->xsave_layout      = ctx->xsave_layout;
  
  // clone probes
  result->probes = push_array(result->arena, DMN_LNX_Probe *, DMN_LNX_ProbeType_Count);
  for EachIndex(probe_idx, DMN_LNX_ProbeType_Count)
  {
    DMN_LNX_Probe *dst = result->probes[probe_idx];
    DMN_LNX_Probe *src = ctx->probes[probe_idx];
    
    dst->provider    = str8_copy(result->arena, src->provider);
    dst->name        = str8_copy(result->arena, src->name);
    dst->args_string = str8_copy(result->arena, src->args_string);
    dst->args        = stap_arg_array_copy(result->arena, src->args);
    dst->pc          = src->pc;
    dst->semaphore   = src->semaphore;
  }
  
  // clone probe traps
  for EachNode(src, DMN_ActiveTrap, ctx->first_probe_trap)
  {
    DMN_Trap *src_trap = src->trap;
    DMN_Trap *dst_trap = push_array(result->arena, DMN_Trap, 1);
    dst_trap->process = dmn_lnx_handle_from_process(new_owner);
    dst_trap->vaddr   = src_trap->vaddr;
    dst_trap->id      = src_trap->id;
    dst_trap->flags   = src_trap->flags;
    dst_trap->size    = src_trap->size;
    
    DMN_ActiveTrap *dst = push_array(result->arena, DMN_ActiveTrap, 1);
    dst->trap       = dst_trap;
    dst->swap_bytes = str8_copy(result->arena, src->swap_bytes);
    
    SLLQueuePush(result->first_probe_trap, result->last_probe_trap, dst);
  }
  
  // clone modules
  for EachNode(module, DMN_LNX_Module, ctx->first_module)
  {
    dmn_lnx_module_clone(result, module);
  }
  
  return result;
}

internal DMN_LNX_Module *
dmn_lnx_module_clone(DMN_LNX_ProcessCtx *process_ctx, DMN_LNX_Module *module)
{
  DMN_LNX_Module *result = &dmn_lnx_entity_alloc(DMN_LNX_EntityKind_Module)->module;
  *result = *module;
  result->next = result->prev = 0;
  
  // clone base addr mapping
  hash_table_push_u64_raw(process_ctx->arena, process_ctx->loaded_modules_ht, result->base_vaddr, result);
  
  // push module to the list
  DLLPushBack(process_ctx->first_module, process_ctx->last_module, module);
  process_ctx->module_count += 1;
  
  return result;
}

internal DMN_Handle
dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity)
{
  DMN_Handle handle = {0};
  U64 index = IntFromPtr(entity - dmn_lnx_state->entities_base);
  if(index <= max_U32)
  {
    handle.u32[0] = index;
    handle.u32[1] = entity->gen;
  }
  else
  {
    Assert(0 && "failed to make a handle for the entity");
  }
  return handle;
}

internal DMN_Handle
dmn_lnx_handle_from_process(DMN_LNX_Process *process)
{
  return dmn_lnx_handle_from_entity((DMN_LNX_Entity *)process);
}

internal DMN_Handle
dmn_lnx_handle_from_process_ctx(DMN_LNX_ProcessCtx *process_ctx)
{
  return dmn_lnx_handle_from_entity((DMN_LNX_Entity *)process_ctx);
}

internal DMN_Handle
dmn_lnx_handle_from_thread(DMN_LNX_Thread *thread)
{
  return dmn_lnx_handle_from_entity((DMN_LNX_Entity *)thread);
}

internal DMN_Handle
dmn_lnx_handle_from_module(DMN_LNX_Module *module)
{
  return dmn_lnx_handle_from_entity((DMN_LNX_Entity *)module);
}

internal DMN_LNX_Entity *
dmn_lnx_entity_from_handle(DMN_Handle handle, DMN_LNX_EntityKind expected_kind)
{
  DMN_LNX_Entity *result = 0;
  U32 index = handle.u32[0];
  U32 gen   = handle.u32[1];
  if(index < dmn_lnx_state->entities_count && dmn_lnx_state->entities_base[index].gen == gen)
  {
    if(dmn_lnx_state->entities_base[index].kind == expected_kind)
    {
      result = &dmn_lnx_state->entities_base[index];
    }
  }
  return result;
}

internal DMN_LNX_Process *
dmn_lnx_process_from_handle(DMN_Handle process_handle)
{
  return (DMN_LNX_Process *)dmn_lnx_entity_from_handle(process_handle, DMN_LNX_EntityKind_Process);
}

internal DMN_LNX_ProcessCtx *
dmn_lnx_process_ctx_from_handle(DMN_Handle process_ctx_handle)
{
  return (DMN_LNX_ProcessCtx *)dmn_lnx_entity_from_handle(process_ctx_handle, DMN_LNX_EntityKind_ProcessCtx);
}

internal DMN_LNX_Thread *
dmn_lnx_thread_from_handle(DMN_Handle thread_handle)
{
  return (DMN_LNX_Thread *)dmn_lnx_entity_from_handle(thread_handle, DMN_LNX_EntityKind_Thread);
}

internal DMN_LNX_Module *
dmn_lnx_module_from_handle(DMN_Handle module_handle)
{
  return (DMN_LNX_Module *)dmn_lnx_entity_from_handle(module_handle, DMN_LNX_EntityKind_Module);
}

internal void
dmn_lnx_process_trap_probes(DMN_LNX_Process *process)
{
  for EachIndex(i, DMN_LNX_ProbeType_Count)
  {
    if(process->ctx->probes[i] == 0) { continue; }
    
    DMN_Trap *trap = push_array(process->ctx->arena, DMN_Trap, 1);
    trap->process = dmn_lnx_handle_from_process(process);
    trap->vaddr   = process->ctx->probes[i]->pc;
    trap->id      = i;
    
    DMN_ActiveTrap *active_trap = dmn_set_trap(process->ctx->arena, trap);
    SLLQueuePush(process->ctx->first_probe_trap, process->ctx->last_probe_trap, active_trap);
    
    if(BUILD_DEBUG && process->ctx->arch == Arch_x64)
    {
      Assert(active_trap->swap_bytes.size == 1 && active_trap->swap_bytes.str[0] == 0x90);
    }
  }
}

internal void
dmn_lnx_process_untrap_probes(DMN_LNX_Process *process)
{
  NotImplemented;
}

internal U64
dmn_lnx_thread_read_ip(DMN_LNX_Thread *thread)
{
  return regs_rip_from_arch_block(thread->process->ctx->arch, thread->reg_block);
}

internal U64
dmn_lnx_thread_read_sp(DMN_LNX_Thread *thread)
{
  return regs_rsp_from_arch_block(thread->process->ctx->arch, thread->reg_block);
}

internal void
dmn_lnx_thread_write_ip(DMN_LNX_Thread *thread, U64 ip)
{
  regs_arch_block_write_rip(thread->process->ctx->arch, thread->reg_block, ip);
  thread->is_reg_block_dirty = 1;
}

internal void
dmn_lnx_thread_write_sp(DMN_LNX_Thread *thread, U64 sp)
{
  regs_arch_block_write_rsp(thread->process->ctx->arch, thread->reg_block, sp);
  thread->is_reg_block_dirty = 1;
}

internal B32
dmn_lnx_thread_read_reg_block(DMN_LNX_Thread *thread)
{
  AssertAlways(thread->state == DMN_LNX_ThreadState_Stopped);
  
  B32 is_reg_block_read = 0;
  
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
    case Arch_x64:
    {
      DMN_LNX_ProcessCtx *process_ctx = thread->process->ctx;
      REGS_RegBlockX64   *dst         = thread->reg_block;
      
      // general purpose registers
      {
        OS_LNX_GprsX64 src;
        int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_len = sizeof(src), .iov_base = &src }));
        if(ptrace_result < 0) { goto exit;  }
        
        dst->r15.u64     = src.r15;
        dst->r14.u64     = src.r14;
        dst->r13.u64     = src.r13;
        dst->r12.u64     = src.r12;
        dst->rbp.u64     = src.rbp;
        dst->rbx.u64     = src.rbx;
        dst->r11.u64     = src.r11;
        dst->r10.u64     = src.r10;
        dst->r9.u64      = src.r9;
        dst->r8.u64      = src.r8;
        dst->rax.u64     = src.rax;
        dst->rcx.u64     = src.rcx;
        dst->rdx.u64     = src.rdx;
        dst->rsi.u64     = src.rsi;
        dst->rdi.u64     = src.rdi;
        dst->rip.u64     = src.rip;
        dst->cs.u16      = src.cs;
        dst->rflags.u64  = src.rflags;
        dst->rsp.u64     = src.rsp;
        dst->ss.u16      = src.ss;
        dst->fsbase.u64  = src.fsbase;
        dst->gsbase.u64  = src.gsbase;
        dst->ds.u16      = src.ds;
        dst->es.u16      = src.es;
        dst->fs.u16      = src.fs;
        dst->gs.u16      = src.gs;
        thread->orig_rax = src.orig_rax;
      }
      
      // xsave
      {
        Temp scratch = scratch_begin(0, 0);
        
        X64_XSave  *xsave  = 0;
        X64_FXSave *fxsave = 0;
        
        // get xsave
        if(x64_is_xsave_supported())
        {
          void *xsave_raw     = push_array(scratch.arena, U8, process_ctx->xsave_size);
          int   ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_len = process_ctx->xsave_size, .iov_base = xsave_raw }));
          if(ptrace_result < 0) { goto exit; }
          
          xsave  = xsave_raw;
          fxsave = &xsave->fxsave;
        }
        
        // get fxsave
        if(fxsave == 0)
        {
          fxsave = push_array(scratch.arena, X64_FXSave, 1);
          int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_len = sizeof(*fxsave), .iov_base = fxsave }));
          if(ptrace_result < 0) { goto exit; }
          
          fxsave = 0;
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
            AssertAlways(process_ctx->xsave_layout.avx_offset + 16*sizeof(REGS_Reg128) <= process_ctx->xsave_size);
            REGS_Reg128 *avx_s = (REGS_Reg128 *)((U8 *)xsave + process_ctx->xsave_layout.avx_offset);
            REGS_Reg512 *zmm_d = &dst->zmm0;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmm_d[n].v[16], &avx_s[n], sizeof(REGS_Reg128));
            }
          }
          
          if(xsave->header.xstate_bv & X64_XStateComponentFlag_OPMASK)
          {
            AssertAlways(process_ctx->xsave_layout.opmask_offset + sizeof(REGS_Reg64) * 8 <= process_ctx->xsave_size);
            REGS_Reg64 *kmask_s = (REGS_Reg64 *)((U8 *)xsave + process_ctx->xsave_layout.opmask_offset);
            REGS_Reg64 *kmask_d = &dst->k0;
            for EachIndex(n, 8)
            {
              MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(REGS_Reg64));
            }
          }
          
          if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM_H)
          {
            AssertAlways(process_ctx->xsave_layout.zmm_h_offset + sizeof(REGS_Reg256) * 16 <= process_ctx->xsave_size);
            REGS_Reg256 *avx512h_s = (REGS_Reg256 *)((U8 *)xsave + process_ctx->xsave_layout.zmm_h_offset);
            REGS_Reg512 *zmmh_d    = &dst->zmm0;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmmh_d[n].v[32], &avx512h_s[n], sizeof(REGS_Reg256));
            }
          }
          
          if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM)
          {
            AssertAlways(process_ctx->xsave_layout.zmm_offset + sizeof(REGS_Reg512) * 16 <= process_ctx->xsave_size);
            REGS_Reg512 *avx512_s = (REGS_Reg512 *)((U8 *)xsave + process_ctx->xsave_layout.zmm_offset);
            REGS_Reg512 *zmm_d    = &dst->zmm16;
            for EachIndex(n, 16)
            {
              MemoryCopy(&zmm_d[n], &avx512_s[n], sizeof(REGS_Reg512));
            }
          }
          
          if(xsave->header.xstate_bv & X64_XStateComponentFlag_CETU)
          {
            AssertAlways(process_ctx->xsave_layout.cet_u_offset + sizeof(U64)*2 <= process_ctx->xsave_size);
            U64 *cet_u = (U64 *)((U8 *)xsave + process_ctx->xsave_layout.cet_u_offset);
            dst->cetmsr.u64 = cet_u[0];
            dst->cetssp.u64 = cet_u[1];
          }
        }
        
        scratch_end(scratch);
      }
      
      // debug registers
      {
        REGS_Reg64 *dr_d = &dst->dr0;
        for EachIndex(n, 8)
        {
          if(n != 4 && n != 5)
          {
            U64 offset = OffsetOf(OS_LNX_UserX64, u_debugreg[n]);
            errno = 0;
            long peek_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_PEEKUSER, thread->tid, PtrFromInt(offset), 0));
            if(errno != 0) { goto exit; }
            dr_d[n].u64 = peek_result;
          }
        }
      }
      
      is_reg_block_read = 1;
    } break;
    case Arch_x86:
    case Arch_arm64:
    case Arch_arm32: { NotImplemented; } break;
    default: { InvalidPath; } break;
  }
  
  exit:;
  return is_reg_block_read;
}

internal B32
dmn_lnx_thread_write_reg_block(DMN_LNX_Thread *thread)
{
  AssertAlways(thread->state == DMN_LNX_ThreadState_Stopped);
  
  B32 is_reg_block_written = 0;
  
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
    case Arch_x64:
    {
      DMN_LNX_ProcessCtx *process_ctx = thread->process->ctx;
      REGS_RegBlockX64   *src         = thread->reg_block;
      
      // general purpose registers
      {
        OS_LNX_GprsX64 dst;
        dst.r15      = src->r15.u64;
        dst.r14      = src->r14.u64;
        dst.r13      = src->r13.u64;
        dst.r12      = src->r12.u64;
        dst.rbp      = src->rbp.u64;
        dst.rbx      = src->rbx.u64;
        dst.r11      = src->r11.u64;
        dst.r10      = src->r10.u64;
        dst.r9       = src->r9.u64;
        dst.r8       = src->r8.u64;
        dst.rax      = src->rax.u64;
        dst.rcx      = src->rcx.u64;
        dst.rdx      = src->rdx.u64;
        dst.rsi      = src->rsi.u64;
        dst.rdi      = src->rdi.u64;
        dst.orig_rax = thread->orig_rax;
        dst.rip      = src->rip.u64;
        dst.cs       = src->cs.u16;
        dst.rflags   = src->rflags.u64;
        dst.rsp      = src->rsp.u64;
        dst.ss       = src->ss.u16;
        dst.fsbase   = src->fsbase.u64;
        dst.gsbase   = src->gsbase.u64;
        dst.ds       = src->ds.u16;
        dst.es       = src->es.u16;
        dst.fs       = src->fs.u16;
        dst.gs       = src->gs.u16;
        int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, thread->tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_base = &dst, .iov_len = sizeof(dst) }));
        if(ptrace_result < 0) { goto exit; }
      }
      
      // xsave
      {
        Temp scratch = scratch_begin(0, 0);
        
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
          U8        *xsave_raw = push_array(scratch.arena, U8, process_ctx->xsave_size);
          X64_XSave *dst       = (X64_XSave *)xsave_raw;
          dst->fxsave = dst_fxsave;
          dst->header.xstate_bv |= X64_XStateComponentFlag_FP;
          dst->header.xstate_bv |= X64_XStateComponentFlag_SSE;
          
          if(process_ctx->xsave_layout.avx_offset)
          {
            if(process_ctx->xsave_layout.avx_offset + sizeof(REGS_Reg128) * 16 <= process_ctx->xsave_size)
            {
              REGS_Reg128 *avx_d = (REGS_Reg128 *)(xsave_raw + process_ctx->xsave_layout.avx_offset);
              REGS_Reg512 *zmm_s = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx_d[n], &zmm_s[n].v[16], sizeof(REGS_Reg128));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_AVX;
            }
          }
          
          if(process_ctx->xsave_layout.opmask_offset)
          {
            if(process_ctx->xsave_layout.opmask_offset + sizeof(REGS_Reg64) * 8 <= process_ctx->xsave_size)
            {
              REGS_Reg64 *kmask_d = (REGS_Reg64 *)(xsave_raw + process_ctx->xsave_layout.opmask_offset);
              REGS_Reg64 *kmask_s = &src->k0;
              for EachIndex(n, 8)
              {
                MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(REGS_Reg64));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_OPMASK;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.zmm_h_offset)
          {
            if(process_ctx->xsave_layout.zmm_h_offset + sizeof(REGS_Reg256) * 16 <= process_ctx->xsave_size)
            {
              REGS_Reg256 *avx512h_d = (REGS_Reg256 *)(xsave_raw + process_ctx->xsave_layout.zmm_h_offset);
              REGS_Reg512 *zmmh_s    = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512h_d[n], &zmmh_s[n].v[32], sizeof(REGS_Reg256));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM_H;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.zmm_offset)
          {
            if(process_ctx->xsave_layout.zmm_offset + sizeof(REGS_Reg512) * 16 <= process_ctx->xsave_size)
            {
              REGS_Reg512 *avx512_d = (REGS_Reg512 *)(xsave_raw + process_ctx->xsave_layout.zmm_offset);
              REGS_Reg512 *zmm_s    = &src->zmm16;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512_d[n], &zmm_s[n], sizeof(REGS_Reg512));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.cet_u_offset)
          {
            if(process_ctx->xsave_layout.cet_u_offset + sizeof(REGS_Reg64) * 2 <= process_ctx->xsave_size)
            {
              REGS_Reg64 *cet_u = (REGS_Reg64 *)(xsave_raw + process_ctx->xsave_layout.cet_u_offset);
              cet_u[0] = src->cetmsr;
              cet_u[1] = src->cetssp;
              dst->header.xstate_bv |= X64_XStateComponentFlag_CETU;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          // xsave
          Assert(dst->header.xcomp_bv == 0); // must always be zero
          int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, thread->tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_base = dst, .iov_len = process_ctx->xsave_size }));
          if(ptrace_result < 0) { goto exit; }
        }
        
        scratch_end(scratch);
      }
      
      // debug registers
      {
        src->dr7.u64 |= (1 << 10);
        
        REGS_Reg64 *dr_s = &src->dr0;
        for EachIndex(n, 8)
        {
          if(n != 4 && n != 5)
          {
            U64 offset = OffsetOf(OS_LNX_UserX64, u_debugreg[n]);
            int ptrace_result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_POKEUSER, thread->tid, PtrFromInt(offset), (void*)(uintptr_t)dr_s[n].u64));
            if(ptrace_result < 0) { goto exit; }
          }
        }
      }
      
      is_reg_block_written = 1;
    } break;
    case Arch_arm64:
    case Arch_arm32:
    case Arch_x86: { NotImplemented; }break;
    default: { InvalidPath; } break;
  }
  
  exit:;
  return is_reg_block_written;
}

internal B32
dmn_lnx_set_single_step_flag(DMN_LNX_Thread *thread, B32 is_on)
{
  B32 is_flag_set = 0;
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
    case Arch_x64:
    {
      REGS_RegBlockX64 *reg_block = thread->reg_block;
      if(is_on) { reg_block->rflags.u64 |= X64_RFlag_Trap;  }
      else      { reg_block->rflags.u64 &= ~X64_RFlag_Trap; }
      thread->is_reg_block_dirty = 1;
      is_flag_set = 1;
    } break;
    case Arch_x86:
    case Arch_arm32:
    case Arch_arm64: { NotImplemented; }break;
    default: { InvalidPath; } break;
  }
  Assert(is_flag_set);
  return is_flag_set;
}

internal void
dmn_lnx_thread_ptr_list_push_node(DMN_LNX_ThreadPtrList *list, DMN_LNX_ThreadPtrNode *n)
{
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
}

internal DMN_LNX_ThreadPtrNode *
dmn_lnx_thread_ptr_list_push(Arena *arena, DMN_LNX_ThreadPtrList *list, DMN_LNX_Thread *v)
{
  DMN_LNX_ThreadPtrNode *n = push_array(arena, DMN_LNX_ThreadPtrNode, 1);
  n->v = v;
  dmn_lnx_thread_ptr_list_push_node(list, n);
  return n;
}

internal void
dmn_lnx_thread_ptr_list_remove(DMN_LNX_ThreadPtrList *list, DMN_LNX_ThreadPtrNode *n)
{
  DLLRemove(list->first, list->last, n);
  list->count -= 1;
}

internal DMN_LNX_ModulePtrNode *
dmn_lnx_module_ptr_list_push(Arena *arena, DMN_LNX_ModulePtrList *list, DMN_LNX_Module *v)
{
  DMN_LNX_ModulePtrNode *n = push_array(arena, DMN_LNX_ModulePtrNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal DMN_LNX_ProcessPtrNode *
dmn_lnx_process_ptr_list_push(Arena *arena, DMN_LNX_ProcessPtrList *list, DMN_LNX_Process *v)
{
  DMN_LNX_ProcessPtrNode *n = push_array(arena, DMN_LNX_ProcessPtrNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal void
dmn_lnx_push_event_create_process(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process)
{
  // push create process event
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind      = DMN_EventKind_CreateProcess;
  e->process   = dmn_lnx_handle_from_process(process);
  e->arch      = process->ctx->arch;
  e->code      = process->pid;
  e->tls_model = DMN_TlsModel_Gnu; // TODO: use dynamic linker path to figure out correct enum here
}

internal void
dmn_lnx_push_event_exit_process(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_ExitProcess;
  e->process = dmn_lnx_handle_from_process(process);
  e->code    = process->main_thread_exit_code;
}

internal void
dmn_lnx_push_event_create_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_CreateThread;
  e->process = dmn_lnx_handle_from_process(thread->process);
  e->thread  = dmn_lnx_handle_from_thread(thread);
  e->arch    = thread->process->ctx->arch;
  e->code    = thread->tid;
}

internal void
dmn_lnx_push_event_exit_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 exit_code)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_ExitThread;
  e->process = dmn_lnx_handle_from_process(thread->process);
  e->thread  = dmn_lnx_handle_from_thread(thread);
  e->code    = exit_code;
}

internal void
dmn_lnx_push_event_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, DMN_LNX_Module *module)
{
  // TODO: reporting this module breaks ctrl thread
  String8 module_name = dmn_lnx_read_string(arena, thread->process->fd, module->name_vaddr);
  if(!str8_match(module_name, str8_lit("linux-vdso.so.1"), 0))
  {
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind             = DMN_EventKind_LoadModule;
    e->process          = dmn_lnx_handle_from_process(thread->process);
    e->thread           = dmn_lnx_handle_from_thread(thread);
    e->module           = dmn_lnx_handle_from_module(module);
    e->arch             = thread->process->ctx->arch;
    e->address          = module->base_vaddr;
    e->size             = module->size;
    e->string           = module_name;
    e->elf_phdr_vrange  = r1u64(module->phvaddr, module->phvaddr + module->phentsize * module->phcount);
    e->elf_phdr_entsize = module->phentsize;
    e->tls_index        = module->tls_index;
    e->tls_offset       = module->tls_offset;
  }
}

internal void
dmn_lnx_push_event_unload_module(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, DMN_LNX_Module *module)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_UnloadModule;
  e->process = dmn_lnx_handle_from_process(process);
  e->module  = dmn_lnx_handle_from_module(module);
}

internal void
dmn_lnx_push_event_handshake_complete(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_HandshakeComplete;
  e->process = dmn_lnx_handle_from_process(process);
  e->thread  = dmn_lnx_handle_from_thread(process->first_thread);
  e->arch    = process->ctx->arch;
}

internal void
dmn_lnx_push_event_breakpoint(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 address)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_Breakpoint;
  e->process             = dmn_lnx_handle_from_process(thread->process);
  e->thread              = dmn_lnx_handle_from_thread(thread);
  e->instruction_pointer = address;
}

internal void
dmn_lnx_push_event_single_step(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_SingleStep;
  e->process             = dmn_lnx_handle_from_process(thread->process);
  e->thread              = dmn_lnx_handle_from_thread(thread);
  e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
}

internal void
dmn_lnx_push_event_exception(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 signo)
{
  local_persist B8 is_repeatable[] =
  {
    0, // null
    1, // 1 SIGHUP
    1, // 2 SIGINT
    1, // 3 SIGQUIT
    1, // 4 SIGTRAP
    1, // 5 SIGABRT
    1, // 6 SIGIOT
    1, // 7 SIGBUS
    1, // 8 SIGFPE
    1, // 9 SIGKILL
    1, // 10 SIGUSR1
    1, // 11 SIGSEGV
    1, // 12 SIGUSR2
    1, // 13 SIGPIPE
    1, // 14 SIGALRM
    1, // 15 SIGTERM
    1, // 16 SIGTKFLT
    0, // 17 SIGCHLD
    0, // 18 SIGCONT
    1, // 19 SIGSTOP
    1, // 20 SIGTSP
    1, // 21 SIGTTIN
    1, // 22 SIGTTOU
    0, // 23 SIGURG
    1, // 24 SIGXCPU
    1, // 25 SIGXFSZ
    1, // 26 SIGVTALRM
    1, // 27 SIGPROF
    0, // 28 SIGWINCH
    1, // 29 SIGIO
    1, // 30 SIGPWR
    1, // 31 SIGSYS
    1, // 32 SIGUNUSED
  };
  
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_Exception;
  e->process             = dmn_lnx_handle_from_process(thread->process);
  e->thread              = dmn_lnx_handle_from_thread(thread);
  e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
  e->signo               = signo;
  e->exception_repeated  = signo < ArrayCount(is_repeatable) ? is_repeatable[signo] : 0;
  
  if(signo == SIGSEGV)
  {
    siginfo_t si = {0};
    OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, thread->tid, 0, &si));
    e->address = (U64)si.si_addr;
  }
}

internal void
dmn_lnx_push_event_halt(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind = DMN_EventKind_Halt;
}

internal void
dmn_lnx_push_event_not_attached(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind       = DMN_EventKind_Error;
  e->error_kind = DMN_ErrorKind_NotAttached;
}

internal DMN_LNX_Thread *
dmn_lnx_event_create_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, pid_t tid)
{
  DMN_LNX_Thread *thread = dmn_lnx_thread_alloc(process, DMN_LNX_ThreadState_Stopped, tid);
  dmn_lnx_push_event_create_thread(arena, events, thread);
  return thread;
}

internal void
dmn_lnx_event_exit_thread(Arena *arena, DMN_EventList *events, pid_t tid, U64 exit_code)
{
  DMN_LNX_Thread  *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Process *process = thread->process;
  
  // store main thread's exit code
  if(thread->tid == thread->process->pid)
  {
    thread->process->main_thread_exit_code = exit_code;
  }
  
  // push exit event
  dmn_lnx_push_event_exit_thread(arena, events, thread, exit_code);
  
  // release entity
  dmn_lnx_thread_release(thread);
  
  // auto exit process on last thread
  if(process->thread_count == 0)
  {
    dmn_lnx_event_exit_process(arena, events, process->pid);
  }
}

internal DMN_LNX_Process *
dmn_lnx_event_create_process(Arena *arena, DMN_EventList *events, pid_t pid, DMN_LNX_Process *parent_process, DMN_LNX_CreateProcessFlags flags)
{
  DMN_LNX_Process *process = dmn_lnx_process_alloc(pid, DMN_LNX_ProcessState_Normal, parent_process, !!(flags & DMN_LNX_CreateProcessFlag_DebugSubprocesses), !!(flags & DMN_LNX_CreateProcessFlag_Cow));
  
  if(flags & DMN_LNX_CreateProcessFlag_ClonedMemory)
  {
    process->ctx = parent_process->ctx;
  }
  else
  {
    process->ctx = dmn_lnx_process_ctx_alloc(process, !!(flags & DMN_LNX_CreateProcessFlag_Rebased));
  }
  process->ctx->ref_count += 1;
  
  // install probes in a process that does not have a cloned memory
  if(!(flags & DMN_LNX_CreateProcessFlag_ClonedMemory))
  {
    dmn_lnx_process_trap_probes(process);
  }
  
  // create main thread
  dmn_lnx_thread_alloc(process, DMN_LNX_ThreadState_Stopped, pid);
  
  // push events
  dmn_lnx_push_event_create_process(arena, events, process);
  for EachNode(thread, DMN_LNX_Thread, process->first_thread)
  {
    dmn_lnx_push_event_create_thread(arena, events, thread);
  }
  for EachNode(module, DMN_LNX_Module, process->ctx->first_module)
  {
    dmn_lnx_push_event_load_module(arena, events, process->first_thread, module);
  }
  dmn_lnx_push_event_handshake_complete(arena, events, process);
  
  return process;
}

internal void
dmn_lnx_event_exit_process(Arena *arena, DMN_EventList *events, pid_t pid)
{
  DMN_LNX_Process *process = dmn_lnx_process_from_pid(pid);
  AssertAlways(process->thread_count == 0);
  
  // push module events
  for EachNode(module, DMN_LNX_Module, process->ctx->first_module)
  {
    dmn_lnx_push_event_unload_module(arena, events, process, module);
  }
  
  // push process exit event
  dmn_lnx_push_event_exit_process(arena, events, process);
  
  // release process
  dmn_lnx_process_release(process);
}

internal void
dmn_lnx_event_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 name_space_id, U64 new_link_map_vaddr)
{
  DMN_LNX_Process *process = thread->process;
  
  GNU_LinkMap64 map = {0};
  for(U64 map_vaddr = new_link_map_vaddr; map_vaddr != 0; map_vaddr = map.next_vaddr)
  {
    // read out new link map item
    if(gnu_read_link_map(dmn_lnx_machine_op_mem_read, &process->fd, map_vaddr, process->ctx->dl_class, &map) != MachineOpResult_Ok) { break; }
    
    // was module already loaded?
    DMN_LNX_Module *module = hash_table_search_u64_raw(process->ctx->loaded_modules_ht, map.addr_vaddr);
    if(module) { continue; }
    
    // clone process ctx
    if(process->is_cow)
    {
      process->is_cow = 0;
      process->ctx    = dmn_lnx_process_ctx_clone(process, process->ctx);
    }
    
    // alloc module
    module = dmn_lnx_module_alloc(process->ctx, process->fd, map.addr_vaddr, map.name_vaddr, name_space_id, 0);
    
    // push load module event
    dmn_lnx_push_event_load_module(arena, events, thread, module);
  }
}

internal void
dmn_lnx_event_unload_module(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, U64 rdebug_vaddr)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  DMN_LNX_ProcessCtx *ctx = process->ctx;
  
  // flag every module as inactive
  for EachNode(module, DMN_LNX_Module, ctx->first_module)
  {
    module->is_live = 0;
  }
  
  // mark live modules
  B32                is_64bit    = ctx->dl_class == ELF_Class_64;
  GNU_RDebugInfoList rdebug_list = gnu_parse_rdebug(scratch.arena, is_64bit, rdebug_vaddr, dmn_lnx_machine_op_mem_read, &process->fd);
  for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
  {
    GNU_LinkMapList link_map_list = gnu_parse_link_map_list(scratch.arena, is_64bit, rdebug_n->v.r_map, dmn_lnx_machine_op_mem_read, &process->fd);
    for EachNode(link_map_n, GNU_LinkMapNode, link_map_list.first)
    {
      DMN_LNX_Module *module = hash_table_search_u64_raw(ctx->loaded_modules_ht, link_map_n->v.addr_vaddr);
      module->is_live = 1;
    }
  }
  
  // collect unloaded modules
  DMN_LNX_ModulePtrList to_release = {0};
  for EachNode(module, DMN_LNX_Module, ctx->first_module)
  {
    if(!module->is_live)
    {
      dmn_lnx_module_ptr_list_push(scratch.arena, &to_release, module);
    }
  }
  
  // clone process context
  if(to_release.count > 0)
  {
    if(process->is_cow)
    {
      process->is_cow = 0;
      process->ctx    = dmn_lnx_process_ctx_clone(process, process->ctx);
    }
  }
  
  // push events and clean up unloaded modules
  for EachNode(module_n, DMN_LNX_ModulePtrNode, to_release.first)
  {
    dmn_lnx_push_event_unload_module(arena, events, process, module_n->v);
    dmn_lnx_module_release(process->ctx, module_n->v);
  }
  
  scratch_end(scratch);
}

internal void
dmn_lnx_event_breakpoint(Arena *arena, DMN_EventList *events, DMN_ActiveTrap *user_traps, pid_t tid)
{
  DMN_LNX_Thread  *thread  = dmn_lnx_thread_from_pid(tid);
  DMN_LNX_Process *process = thread->process;
  U64              ip      = dmn_lnx_thread_read_ip(thread);
  
  // is this user trap?
  DMN_ActiveTrap *hit_user_trap = 0;
  {
    DMN_Handle process_handle = dmn_lnx_handle_from_process(process);
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
    for EachNode(active_trap, DMN_ActiveTrap, process->ctx->first_probe_trap)
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
    
    DMN_LNX_Probe *probe = process->ctx->probes[DMN_LNX_ProbeType_InitComplete];
    U64 name_space_id = 0, rdebug_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id)) { goto init_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_addr))   { goto init_complete_exit; }
    
    GNU_RDebugInfo64 rdebug = {0};
    if(gnu_read_r_debug(dmn_lnx_machine_op_mem_read, &process->fd, rdebug_addr, process->ctx->arch, &rdebug) != MachineOpResult_Ok) { goto init_complete_exit; }
    if(rdebug.r_version < 1) { goto init_complete_exit; }
    
    dmn_lnx_event_load_module(arena, events, thread, name_space_id, rdebug.r_map);
    
    is_init_completed = 1;
    init_complete_exit:;
    AssertAlways(is_init_completed);
  }
  else if(probe_type == DMN_LNX_ProbeType_RelocComplete)
  {
    B32 is_reloc_completed = 0;
    
    DMN_LNX_Probe *probe = process->ctx->probes[DMN_LNX_ProbeType_RelocComplete];
    U64 name_space_id = 0, new_link_map_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id))     { goto reloc_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[2], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &new_link_map_addr)) { goto reloc_complete_exit; }
    
    dmn_lnx_event_load_module(arena, events, thread, name_space_id, new_link_map_addr);
    
    is_reloc_completed = 1;
    reloc_complete_exit:;
    AssertAlways(is_reloc_completed);
  }
  else if(probe_type == DMN_LNX_ProbeType_UnmapComplete)
  {
    B32 is_unmap_completed = 0;
    
    DMN_LNX_Probe *probe = process->ctx->probes[DMN_LNX_ProbeType_UnmapComplete];
    U64 name_space_id = 0, rdebug_vaddr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &name_space_id)) { goto unmap_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->ctx->arch, thread->reg_block, dmn_lnx_stap_memory_read, process, &rdebug_vaddr))  { goto unmap_complete_exit; }
    
    dmn_lnx_event_unload_module(arena, events, process, rdebug_vaddr);
    
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
    e->process             = dmn_lnx_handle_from_process(process);
    e->thread              = dmn_lnx_handle_from_thread(thread);
    e->instruction_pointer = dmn_lnx_thread_read_ip(thread);
  }
}

internal void
dmn_lnx_event_data_breakpoint(Arena *arena, DMN_EventList *events, pid_t tid)
{
  DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(tid);
  
  B32 is_valid = 1;
  U64 address  = 0;
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
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
    } break;
    case Arch_x86:
    case Arch_arm32:
    case Arch_arm64:
    { NotImplemented; } break;
    default: { InvalidPath; } break;
  }
  
  if(is_valid)
  {
    dmn_lnx_push_event_breakpoint(arena, events, thread, address);
  }
}

internal void
dmn_lnx_event_halt(Arena *arena, DMN_EventList *events)
{
  dmn_lnx_push_event_halt(arena, events);
}

internal void
dmn_lnx_event_single_step(Arena *arena, DMN_EventList *events, pid_t tid)
{
  DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(tid);
  
  // clear single step flag
  dmn_lnx_set_single_step_flag(thread, 0);
  
  // push event
  dmn_lnx_push_event_single_step(arena, events, thread);
}

internal void
dmn_lnx_event_exception(Arena *arena, DMN_EventList *events, pid_t tid, U64 signo)
{
  DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(tid);
  
  thread->pass_through_signal = 1;
  thread->pass_through_signo  = signo;
  
  dmn_lnx_push_event_exception(arena, events, thread, signo);
}

internal DMN_LNX_Process *
dmn_lnx_event_attach(Arena *arena, DMN_EventList *events, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // create process
  DMN_LNX_Process *process = dmn_lnx_event_create_process(arena, events, pid, 0, DMN_LNX_CreateProcessFlag_DebugSubprocesses|DMN_LNX_CreateProcessFlag_Rebased);
  
  // extract threads from /proc/pid/task
  {
    String8 task_path = str8f(scratch.arena, "/proc/%d/task", pid);
    DIR *task_dirp = opendir((char *)task_path.str);
    if(task_dirp)
    {
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
        dmn_lnx_event_create_thread(arena, events, process, tid);
      }
      
      OS_LNX_RETRY_ON_EINTR(closedir(task_dirp));
    }
  }
  
  // extract modules from r_debug
  {
    B32                is_64bit      = process->ctx->dl_class == ELF_Class_64;
    GNU_RDebugInfoList rdebug_list   = gnu_parse_rdebug(scratch.arena, is_64bit, process->ctx->rdebug_vaddr, dmn_lnx_machine_op_mem_read, &process->fd);
    U64                name_space_id = 0;
    for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
    {
      dmn_lnx_event_load_module(arena, events, process->first_thread, name_space_id, rdebug_n->v.r_map);
      name_space_id += 1;
    }
  }
  
  // handshake complete
  dmn_lnx_push_event_handshake_complete(arena, events, process);
  
  scratch_end(scratch);
  return process;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  if(dmn_lnx_state == 0)
  {
    local_persist DMN_LNX_State state;
    dmn_lnx_state = &state;
    
    dmn_lnx_state->arena          = arena_alloc();
    dmn_lnx_state->access_mutex   = mutex_alloc();
    dmn_lnx_state->entities_arena = arena_alloc(.reserve_size = GB(32), .commit_size = KB(64), .flags = ArenaFlag_NoChain);
    dmn_lnx_state->entities_base  = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 0);
    dmn_lnx_state->tid_ht         = hash_table_init(dmn_lnx_state->arena, 0x2000);
    dmn_lnx_state->pid_ht         = hash_table_init(dmn_lnx_state->arena, 0x400);
    dmn_lnx_state->halter_mutex   = mutex_alloc();
    dmn_lnx_entity_alloc(DMN_LNX_EntityKind_Null);
    
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
    for EachNode(n, String8Node, params->cmd_line.first)
    {
      argv[idx] = (char *)str8_copy(scratch.arena, n->string).str;
      idx += 1;
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
    for EachNode(n, String8Node, params->env.first)
    {
      envp[idx] = (char *)str8_copy(scratch.arena, n->string).str;
      idx += 1;
    }
  }
  
  // create zero-terminated work directory path
  char *work_dir_path = (char *)str8_copy(scratch.arena, params->path).str;
  
  // fork process
  pid_t pid = fork();
  
  // child process
  if(pid == 0)
  {
    // wait for seize
    if(OS_LNX_RETRY_ON_EINTR(raise(SIGSTOP)) < 0) { goto child_exit; }
    
    // change work directory to tracee
    if(OS_LNX_RETRY_ON_EINTR(chdir(work_dir_path)) < 0) { goto child_exit; }
    
    // replace process with target program
    if(OS_LNX_RETRY_ON_EINTR(execve(argv[0], argv, envp)) < 0) { goto child_exit; }
    
    child_exit:;
    exit(0);
  }
  // parent process
  else if(pid > 0)
  {
    // try to seize child process
    if(dmn_lnx_ptrace_seize(pid) >= 0)
    {
      // tracee was successfully seized, create process
      dmn_lnx_process_alloc(pid, DMN_LNX_ProcessState_Launch, 0, params->debug_subprocesses, 0);
    }
    else
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
      dmn_lnx_process_alloc(pid, DMN_LNX_ProcessState_Attach, 0, 1, 0);
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
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process_handle, U32 exit_code)
{
  B32 result = 0;
  mutex_take(dmn_lnx_state->halter_mutex);
  DMN_LNX_Process *process = dmn_lnx_process_from_handle(process_handle);
  if(process)
  {
    result = OS_LNX_RETRY_ON_EINTR(kill(process->pid, SIGKILL)) >= 0;
  }
  mutex_drop(dmn_lnx_state->halter_mutex);
  return result;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process_handle)
{
  B32 result = 0;
  mutex_take(dmn_lnx_state->halter_mutex);
  DMN_LNX_Process *process = dmn_lnx_process_from_handle(process_handle);
  if(process)
  {
    result = OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, process->pid, 0, 0)) >= 0;
  }
  mutex_drop(dmn_lnx_state->halter_mutex);
  return result;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  Temp scratch = scratch_begin(&arena, 1);
  DMN_EventList events = {0};
  
  mutex_take(dmn_lnx_state->halter_mutex);
  
  // wait for signals from the running threads
  if(dmn_lnx_state->process_count > 0)
  {
    // write traps to memory
    DMN_ActiveTrap *active_trap_first = 0, *active_trap_last = 0;
    {
      HashTable *process_ht = hash_table_init(scratch.arena, dmn_lnx_state->process_count);
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
          DMN_LNX_Process *process = dmn_lnx_process_from_handle(trap->process);
          if(!process) { continue; }
          
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
      DMN_LNX_Thread *single_step_thread = dmn_lnx_thread_from_handle(ctrls->single_step_thread);
      if(single_step_thread)
      {
        dmn_lnx_set_single_step_flag(single_step_thread, 1);
      }
      else
      {
        Assert(0 && "invalid single_step_thread handle");
      }
    }
    
    // schedule threads to run
    DMN_LNX_ThreadPtrList running_threads = {0};
    {
      for EachNode(process, DMN_LNX_Process, dmn_lnx_state->first_process)
      {
        //- rjf: determine if this process is frozen
        B32 process_is_frozen = 0;
        if(ctrls->run_entities_are_processes)
        {
          for EachIndex(idx, ctrls->run_entity_count)
          {
            if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_process(process)))
            {
              process_is_frozen = 1;
              break;
            }
          }
        }
        
        for EachNode(thread, DMN_LNX_Thread, process->first_thread)
        {
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
                if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_thread(thread)))
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
            is_frozen = !dmn_handle_match(dmn_lnx_handle_from_thread(thread), ctrls->single_step_thread);
          }
          
          // resume thread
          if(!is_frozen)
          {
            AssertAlways(thread->state == DMN_LNX_ThreadState_Stopped);
            
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
            if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, thread->tid, 0, sig_code)) >= 0)
            {
              thread->state              = DMN_LNX_ThreadState_Running;
              thread->is_reg_block_dirty = 1;
              dmn_lnx_thread_ptr_list_push(scratch.arena, &running_threads, thread);
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
    
    // hash running threads tids
    HashTable *running_threads_ht = hash_table_init(scratch.arena, running_threads.count * 2);
    for EachNode(n, DMN_LNX_ThreadPtrNode, running_threads.first)
    {
      hash_table_push_u64_raw(scratch.arena, running_threads_ht, n->v->tid, n);
    }
    
    B32                   is_halt_done    = 0;
    DMN_LNX_ThreadPtrList stopped_threads = {0};
    do
    {
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
      
      // intercept initing processes
      {
        DMN_LNX_Process *process = dmn_lnx_process_from_pid(wait_id);
        
        if(process && process->state != DMN_LNX_ProcessState_Normal)
        {
          switch(process->state)
          {
            case DMN_LNX_ProcessState_Null:
            case DMN_LNX_ProcessState_Normal:
            {
              InvalidPath;
            } break;
            case DMN_LNX_ProcessState_Launch:
            {
              if(wstopsig == SIGSTOP)
              {
                if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, process->pid, 0, 0)) >= 0)
                {
                  process->state = DMN_LNX_ProcessState_WaitForExec;
                  goto wait_for_signal;
                }
                else { Assert(0 && "failed to resume tracee"); }
              }
              // else { Assert(0 && "unexpected signal"); }
            } break;
            case DMN_LNX_ProcessState_WaitForExec:
            {
              if(wstopsig == SIGTRAP && event_code == PTRACE_EVENT_EXEC)
              {
                DMN_LNX_CreateProcessFlags create_flags = process->debug_subprocesses ? DMN_LNX_CreateProcessFlag_DebugSubprocesses : 0;
                dmn_lnx_process_release(process);
                dmn_lnx_event_create_process(arena, &events, wait_id, 0, create_flags);
                goto wait_for_signal;
              }
              //else { Assert(0 && "unexpected signal"); }
            } break;
            case DMN_LNX_ProcessState_ExecFailedDoExit:
            {
              if(wifexited)
              {
                dmn_lnx_process_release(process);
                goto wait_for_signal;
              }
              else { Assert(0 && "unexpected signal"); }
            } break;
            case DMN_LNX_ProcessState_Attach:
            {
              if(wstopsig == SIGSTOP)
              {
                DMN_LNX_CreateProcessFlags create_flags = process->debug_subprocesses ? DMN_LNX_CreateProcessFlag_DebugSubprocesses : 0;
                dmn_lnx_process_release(process);
                dmn_lnx_event_create_process(arena, &events, wait_id, 0, create_flags);
                goto wait_for_signal;
              }
              else { Assert(0 && "unexpected signal"); }
            } break;
          }
          
          // shutdown process
          {
            if(OS_LNX_RETRY_ON_EINTR(kill(wait_id, SIGKILL)) >= 0)
            {
              process->state = DMN_LNX_ProcessState_ExecFailedDoExit;
            }
            else
            {
              //Assert(0 && "failed to send kill signal to the tracee");
              dmn_lnx_process_release(process);
            }
          }
          
          wait_for_signal:;
          continue;
        }
      }
      
      if(wifstopped || wifsignaled || wifexited)
      {
        DMN_LNX_ThreadPtrNode *thread_n = hash_table_search_u64_raw(running_threads_ht, wait_id);
        if(thread_n)
        {
          DMN_LNX_Thread *thread = thread_n->v;
          AssertAlways(thread->state == DMN_LNX_ThreadState_Running);
          
          // remove mapping
          hash_table_purge_u64(running_threads_ht, thread->tid);
          
          // move thread to the stopped list
          dmn_lnx_thread_ptr_list_remove(&running_threads, thread_n);
          dmn_lnx_thread_ptr_list_push_node(&stopped_threads, thread_n);
          
          // update thread state
          if(wifstopped && !wifsignaled && !wifexited)
          {
            thread->state = DMN_LNX_ThreadState_Stopped;
          }
          else if(!wifstopped && (wifsignaled || wifexited))
          {
            thread->state = DMN_LNX_ThreadState_Exited;
          }
          else { InvalidPath; }
          
          // stop all other threads
          if(stopped_threads.count == 1)
          {
            for EachNode(n, DMN_LNX_ThreadPtrNode, running_threads.first)
            {
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_INTERRUPT, n->v->tid, 0, 0)) < 0) { Assert(0 && "failed to interrupt process"); }
            }
          }
        }
      }
      
      // normal child exit via _exit or exit()
      if(wifexited)
      {
        dmn_lnx_event_exit_thread(arena, &events, wait_id, WEXITSTATUS(status));
      }
      
      // exit because child did not handle a signal
      else if(wifsignaled)
      {
        dmn_lnx_event_exit_thread(arena, &events, wait_id, WTERMSIG(status));
      }
      // thread or group stop
      else if(wifstopped)
      {
        // read thread registers
        {
          DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(wait_id);
          if(thread && thread->state != DMN_LNX_ThreadState_PendingCreation)
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
                  case SI_KERNEL:   { dmn_lnx_event_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                  case TRAP_BRKPT:  { dmn_lnx_event_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                  case TRAP_TRACE:  { dmn_lnx_event_single_step(arena, &events, wait_id);                   } break;
                  case TRAP_HWBKPT: { dmn_lnx_event_data_breakpoint(arena, &events, wait_id);               } break;
                  case TRAP_BRANCH: { NotImplemented; }break;
                  case TRAP_UNK:    { NotImplemented; }break;
                  default: { InvalidPath; } break;
                }
              } else { Assert(0 && "failed to get signal info"); }
            }break;
            case PTRACE_EVENT_FORK:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_VFORK:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_VFORK_DONE:
            {
              NotImplemented;
            }break;
            case PTRACE_EVENT_CLONE:
            {
              // kernel stopped the parent just before scheduling the child to
              // give us a chance to prepare to trace it; next event for the child
              // will be a PTRACE_EVENT_STOP
              
              pid_t new_tid;
              if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &new_tid)) >= 0)
              {
                DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(wait_id);
                
                // create a new partially inited thread
                dmn_lnx_thread_alloc(thread->process, DMN_LNX_ThreadState_PendingCreation, new_tid);
              }
              else { Assert(0 && "failed to get new tid"); }
            }break;
            case PTRACE_EVENT_EXEC:
            {
              DMN_LNX_Thread  *thread  = dmn_lnx_thread_from_pid(wait_id);
              DMN_LNX_Process *process = thread->process;
              if(process->debug_subprocesses)
              {
                dmn_lnx_event_exit_thread(arena, &events, wait_id, 0);
                dmn_lnx_event_create_process(arena, &events, wait_id, 0, DMN_LNX_CreateProcessFlag_DebugSubprocesses);
              }
              else
              {
                if(OS_LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, wait_id, 0, 0)) >= 0)
                {
                  dmn_lnx_event_exit_thread(arena, &events, wait_id, 0);
                }
                else { Assert(0 && "failed to detach"); }
              }
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
              DMN_LNX_Thread *thread = dmn_lnx_thread_from_pid(wait_id);
              if(thread->state == DMN_LNX_ThreadState_PendingCreation)
              {
                DMN_LNX_Process *process = thread->process;
                dmn_lnx_thread_release(thread);
                thread = dmn_lnx_event_create_thread(arena, &events, process, wait_id);
              }
              else
              {
                AssertAlways(thread->state == DMN_LNX_ThreadState_Stopped);
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
          dmn_lnx_event_exception(arena, &events, wait_id, wstopsig);
        }
      }
      else { Assert(0 && "unexpected stop code"); }
    } while(running_threads.count > 0 || dmn_lnx_state->process_pending_creation > 0 || dmn_lnx_state->threads_pending_creation > 0);
    
    // finalize halter state
    if(is_halt_done)
    {
      // push event
      dmn_lnx_event_halt(arena, &events);
      
      // reset state
      dmn_lnx_state->halter_tid     = 0;
      dmn_lnx_state->halt_code      = 0;
      dmn_lnx_state->halt_user_data = 0;
      dmn_lnx_state->is_halting     = 0;
    }
    
    // restore original instruction bytes
    for EachNode(active_trap, DMN_ActiveTrap, active_trap_first)
    {
      // skip process that exited during the wait
      DMN_LNX_Process *process = dmn_lnx_process_from_handle(active_trap->trap->process);
      if(!process) { continue; }
      
      if(!dmn_process_write(active_trap->trap->process, r1u64(active_trap->trap->vaddr, active_trap->trap->vaddr + active_trap->swap_bytes.size), active_trap->swap_bytes.str))
      {
        Assert(0 && "failed to restore original instruction bytes");
      }
    }
  }
  
  if(events.count == 0 && dmn_lnx_state->process_count == 0)
  {
    dmn_lnx_push_event_not_attached(arena, &events);
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
  if(dmn_lnx_state->process_count)
  {
    dmn_lnx_state->halter_tid     = gettid();
    dmn_lnx_state->halt_code      = code;
    dmn_lnx_state->halt_user_data = user_data;
    for EachNode(process, DMN_LNX_Process, dmn_lnx_state->first_process)
    {
      if(OS_LNX_RETRY_ON_EINTR(kill(process->pid, SIGSTOP)) < 0) { Assert(0 && "failed to send SIGSTOP"); }
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
dmn_process_read(DMN_Handle process_handle, Rng1U64 range, void *dst)
{
  DMN_LNX_Process *process = dmn_lnx_process_from_handle(process_handle);
  U64 result = 0;
  if(process)
  {
    result = dmn_lnx_read(process->fd, range, dst);
  }
  return result;
}

internal B32
dmn_process_write(DMN_Handle process_handle, Rng1U64 range, void *src)
{
  DMN_LNX_Process *process = dmn_lnx_process_from_handle(process_handle);
  B32 result = 0;
  if(process)
  {
    result = dmn_lnx_write(process->fd, range, src);
  }
  return result;
}

//- rjf: threads

internal Arch
dmn_arch_from_thread(DMN_Handle thread_handle)
{
  Arch arch = Arch_Null;
  DMN_LNX_Thread *thread = dmn_lnx_thread_from_handle(thread_handle);
  if(thread)
  {
    arch = thread->process->ctx->arch;
  }
  return arch;
}

internal U64
dmn_stack_base_vaddr_from_thread(DMN_Handle thread_handle)
{
  return 0;
}

internal U64
dmn_tls_root_vaddr_from_thread(DMN_Handle thread_handle)
{
  U64 tls_root_vaddr = max_U64;
  DMN_AccessScope
  {
    DMN_LNX_Thread *thread = dmn_lnx_thread_from_handle(thread_handle);
    if(thread)
    {
      switch(thread->process->ctx->arch)
      {
        case Arch_Null: {} break;
        case Arch_x64:
        {
          REGS_RegBlockX64 *reg_block   = thread->reg_block;
          U64               dtv_pointer = 0;
          if(dmn_lnx_read_struct(thread->process->fd, reg_block->fsbase.u64 + 8, &dtv_pointer))
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
  }
  return tls_root_vaddr;
}

internal B32
dmn_thread_read_reg_block(DMN_Handle thread_handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Thread *thread = dmn_lnx_thread_from_handle(thread_handle);
    if(thread)
    {
      U64 reg_block_size = regs_block_size_from_arch(thread->process->ctx->arch);
      MemoryCopy(reg_block, thread->reg_block, reg_block_size);
    }
    result = 1;
  }
  return result;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle thread_handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Thread *thread = dmn_lnx_thread_from_handle(thread_handle);
    if(thread)
    {
      U64 reg_block_size = regs_block_size_from_arch(thread->process->ctx->arch);
      MemoryCopy(thread->reg_block, reg_block, reg_block_size);
      thread->is_reg_block_dirty = 1;
      result = 1;
    }
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

