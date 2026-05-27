// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Includes

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

////////////////////////////////

internal U64
lnx_dmn_size_from_fd(int memory_fd, U64 cap)
{
  U8 temp[4096];
  size_t cursor = 0;
  while(cursor < cap)
  {
    ssize_t actual_read = LNX_RETRY_ON_EINTR(pread(memory_fd, temp, sizeof(temp), cursor));
    if(actual_read <= 0) { break; }
    cursor += (U64)actual_read;
  }
  return (U64)cursor;
}

internal U64
lnx_dmn_read(int memory_fd, Rng1U64 range, void *dst)
{
  size_t cursor = 0, size = dim_1u64(range);
  while(cursor < size)
  {
    size_t  to_read     = size - cursor;
    ssize_t actual_read = LNX_RETRY_ON_EINTR(pread(memory_fd, (U8 *)dst + cursor, to_read, range.min + cursor));
    if(actual_read < 0) { break; }
    if(actual_read == 0) { break; }
    cursor += actual_read;
  }
  return cursor;
}

internal B32
lnx_dmn_write(int memory_fd, Rng1U64 range, void *src)
{
  size_t cursor = 0, size = dim_1u64(range);
  while(cursor < size)
  {
    size_t to_write = size - cursor;
    ssize_t actual_write = LNX_RETRY_ON_EINTR(pwrite(memory_fd, (U8 *)src + cursor, to_write, range.min + cursor));
    if(actual_write <= 0) { break; }
    cursor += actual_write;
  }
  B32 is_written = (cursor == size);
  return is_written;
}

internal String8
lnx_dmn_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size)
{
  String8 result = {0};
  U64 string_size = 0;
  for(U64 vaddr = base_vaddr; string_size < cap_size; vaddr += 1, string_size += 1)
  {
    char byte = 0;
    if(LNX_RETRY_ON_EINTR(pread(memory_fd, &byte, sizeof(byte), vaddr) <= 0))
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
    LNX_RETRY_ON_EINTR(pread(memory_fd, buf, string_size, base_vaddr));
    buf[string_size] = '\0';
    result = str8((U8 *)buf, string_size);
  }
  return result;
}

internal String8
lnx_dmn_read_string(Arena *arena, int memory_fd, U64 vaddr)
{
  return lnx_dmn_read_string_capped(arena, memory_fd, vaddr, 4096);
}

internal
MACHINE_OP_MEM_READ(lnx_dmn_machine_op_mem_read)
{
  U64 read_size = lnx_dmn_read(*(int *)ud, r1u64(addr, addr + buffer_size), buffer);
  return read_size == buffer_size ? MachineOpResult_Ok : MachineOpResult_Fail;
}

internal int
lnx_dmn_ptrace_seize(pid_t pid)
{
  // TODO: PTRACE_O_TRACEVFORK | PTRACE_O_TRACEVFORKDONE | PTRACE_O_TRACEFORK
  return LNX_RETRY_ON_EINTR(ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL | PTRACE_O_TRACECLONE));
}

internal String8
lnx_dmn_exe_path_from_pid(Arena *arena, pid_t pid)
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
lnx_dmn_dl_path_from_pid(Arena *arena, pid_t pid, U64 auxv_base)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 dl_path = {0};
  
  int maps_fd = LNX_RETRY_ON_EINTR(open((char *)str8f(scratch.arena, "/proc/%d/maps", pid).str, O_RDONLY));
  if(maps_fd != -1)
  {
    // read entire /proc/pid/maps
    U64  maps_size = lnx_dmn_size_from_fd(maps_fd, MB(1));
    U8  *maps_ptr  = push_array(scratch.arena, U8, maps_size);
    U64  read_size = lnx_dmn_read(maps_fd, r1u64(0, maps_size), maps_ptr);
    
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
    
    LNX_RETRY_ON_EINTR(close(maps_fd));
  }
  else { Assert(0 && "failed to open DL fd"); }
  
  scratch_end(scratch);
  Assert(dl_path.size);
  return dl_path;
}

internal ELF_Hdr64
lnx_dmn_ehdr_from_pid(pid_t pid)
{
  Temp scratch = scratch_begin(0, 0);
  
  ELF_Hdr64 exe     = {0};
  B32       is_read = 0;
  
  char *exe_path = (char *)str8f(scratch.arena, "/proc/%d/exe", pid).str;
  int   exe_fd   = LNX_RETRY_ON_EINTR(open(exe_path, O_RDONLY));
  
  if(exe_fd >= 0)
  {
    is_read = elf_read_ehdr(lnx_dmn_machine_op_mem_read, &exe_fd, 0, &exe);
    LNX_RETRY_ON_EINTR(close(exe_fd));
  }
  
  Assert(is_read);
  scratch_end(scratch);
  return exe;
}

internal LNX_DMN_Auxv
lnx_dmn_auxv_from_pid(pid_t pid, ELF_Class elf_class)
{
  Temp scratch = scratch_begin(0, 0);
  LNX_DMN_Auxv result = {0};
  
  // rjf: open aux data
  String8 auxv_path = str8f(scratch.arena, "/proc/%d/auxv", pid);
  int auxv_fd = LNX_RETRY_ON_EINTR(open((char *)auxv_path.str, O_RDONLY));
  
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
    LNX_RETRY_ON_EINTR(close(auxv_fd));
  }
  
  scratch_end(scratch);
  return result;
}

internal LNX_DMN_Thread *
lnx_dmn_thread_from_pid(pid_t tid)
{
  return hash_table_search_u64_raw(lnx_dmn_state->tid_ht, tid);
}

internal LNX_DMN_Process *
lnx_dmn_process_from_pid(pid_t pid)
{
  return hash_table_search_u64_raw(lnx_dmn_state->pid_ht, pid);
}

////////////////////////////////
//~ rjf: Module Info Parsing

internal DMN_ModuleInfo *
lnx_dmn_module_info_from_process_module(Arena *arena, pid_t pid, int memory_fd, U64 base_vaddr, U64 name_vaddr, B32 is_main)
{
  DMN_ModuleInfo *info = push_array(arena, DMN_ModuleInfo, 1);
  Temp scratch = scratch_begin(&arena, 1);
  {
    //- rjf: read ELF signature
    U8 elf_sig_maybe[ELF_Identifier_Max] = {0};
    lnx_dmn_read(memory_fd, r1u64(base_vaddr, base_vaddr + sizeof(elf_sig_maybe)), elf_sig_maybe);
    
    //- rjf: unpack elf header info
    Arch arch = Arch_Null;
    U64 entry_point_voff = 0;
    ELF_Type elf_type = ELF_Type_None;
    U64 phdrs_off = 0;
    U64 phdrs_entry_size = 0;
    U64 phdrs_count = 0;
    U64 shdrs_off = 0;
    U64 shdrs_entry_size = 0;
    U64 shdrs_count = 0;
    U64 shdrs_strings_idx = 0;
    {
      // rjf: read ehdr
      ELF_Hdr64 ehdr = {0};
      switch(elf_sig_maybe[ELF_Identifier_Class])
      {
        default:
        case ELF_Class_None:
        {}break;
        case ELF_Class_32:
        {
          ELF_Hdr32 ehdr32 = {0};
          lnx_dmn_read_struct(memory_fd, base_vaddr, &ehdr32);
          MemoryCopy(ehdr.e_ident, ehdr32.e_ident, sizeof(ehdr.e_ident));
          ehdr.e_type        = ehdr32.e_type;
          ehdr.e_machine     = ehdr32.e_machine;
          ehdr.e_version     = ehdr32.e_version;
          ehdr.e_entry       = (U64)ehdr32.e_entry;
          ehdr.e_phoff       = (U64)ehdr32.e_phoff;
          ehdr.e_shoff       = (U64)ehdr32.e_shoff;
          ehdr.e_flags       = ehdr32.e_flags;
          ehdr.e_ehsize      = ehdr32.e_ehsize;
          ehdr.e_phentsize   = ehdr32.e_phentsize;
          ehdr.e_phnum       = ehdr32.e_phnum;
          ehdr.e_shentsize   = ehdr32.e_shentsize;
          ehdr.e_shnum       = ehdr32.e_shnum;
          ehdr.e_shstrndx    = ehdr32.e_shstrndx;
        }break;
        case ELF_Class_64:
        {
          lnx_dmn_read_struct(memory_fd, base_vaddr, &ehdr);
        }break;
      }
      
      // rjf: unpack ehdr
      arch               = arch_from_elf_machine(ehdr.e_machine);
      entry_point_voff   = ehdr.e_entry - base_vaddr;
      elf_type           = ehdr.e_type;
      phdrs_off          = ehdr.e_phoff;
      phdrs_entry_size   = ehdr.e_phentsize;
      phdrs_count        = ehdr.e_phnum;
      shdrs_off          = ehdr.e_shoff;
      shdrs_entry_size   = ehdr.e_shentsize;
      shdrs_count        = ehdr.e_shnum;
      shdrs_strings_idx  = ehdr.e_shstrndx;
    }
    
    //- rjf: decide on rebase for this module
    U64 module_rebase = 0;
    if(elf_type == ELF_Type_Dyn)
    {
      module_rebase = base_vaddr;
    }
    
    //- rjf: determine shdrs/phdrs addresses
    U64 phdrs_vaddr = module_rebase + phdrs_off;
    U64 phdrs_vaddr_opl = phdrs_vaddr + phdrs_count*phdrs_entry_size;
    U64 shdrs_vaddr = module_rebase + shdrs_off;
    
    //- rjf: scan phdrs, unpack info
    U64 image_vsize = 0;
    Rng1U64 eh_frame_header_vaddr_range = {0};
    {
      for EachIndex(phdr_idx, phdrs_count)
      {
        U64 phdr_vaddr = phdrs_vaddr + phdr_idx*phdrs_entry_size;
        ELF_Phdr64 phdr = {0};
        switch(elf_sig_maybe[ELF_Identifier_Class])
        {
          default:{}break;
          case ELF_Class_32:
          {
            ELF_Phdr32 phdr32 = {0};
            lnx_dmn_read_struct(memory_fd, phdr_vaddr, &phdr32);
            phdr.p_type   = phdr32.p_type;
            phdr.p_flags  = phdr32.p_flags;
            phdr.p_offset = (U64)phdr32.p_offset;
            phdr.p_vaddr  = (U64)phdr32.p_vaddr;
            phdr.p_paddr  = (U64)phdr32.p_paddr;
            phdr.p_filesz = (U64)phdr32.p_filesz;
            phdr.p_memsz  = (U64)phdr32.p_memsz;
            phdr.p_align  = (U64)phdr32.p_align;
          }break;
          case ELF_Class_64:
          {
            lnx_dmn_read_struct(memory_fd, phdr_vaddr, &phdr);
          }break;
        }
        if(phdr.p_type == ELF_PType_Load)
        {
          image_vsize = (module_rebase + phdr.p_vaddr + phdr.p_memsz) - base_vaddr;
        }
        if(phdr.p_type == ELF_PType_GnuEHFrame)
        {
          eh_frame_header_vaddr_range = r1u64(module_rebase + phdr.p_vaddr, module_rebase + phdr.p_vaddr + phdr.p_memsz);
        }
      }
    }
    
    //- rjf: scan shdrs, unpack info
    Rng1U64 raddbg_info_voff_range = {0};
    U64 raddbg_is_attached_marker_voff = 0;
    {
      // rjf: read all shdrs
      ELF_Shdr64 *shdrs = push_array(scratch.arena, ELF_Shdr64, shdrs_count);
      for EachIndex(shdr_idx, shdrs_count)
      {
        U64 shdr_vaddr = shdrs_vaddr + shdr_idx*shdrs_entry_size;
        switch(elf_sig_maybe[ELF_Identifier_Class])
        {
          case ELF_Class_32:
          {
            ELF_Shdr32 shdr32 = {0};
            lnx_dmn_read_struct(memory_fd, shdr_vaddr, &shdr32);
            shdrs[shdr_idx].sh_name     = shdr32.sh_name;
            shdrs[shdr_idx].sh_type     = shdr32.sh_type;
            shdrs[shdr_idx].sh_flags    = (U64)shdr32.sh_flags;
            shdrs[shdr_idx].sh_addr     = (U64)shdr32.sh_addr;
            shdrs[shdr_idx].sh_offset   = (U64)shdr32.sh_offset;
            shdrs[shdr_idx].sh_size     = (U64)shdr32.sh_size;
            shdrs[shdr_idx].sh_link     = shdr32.sh_link;
            shdrs[shdr_idx].sh_info     = shdr32.sh_info;
            shdrs[shdr_idx].sh_addralign= (U64)shdr32.sh_addralign;
            shdrs[shdr_idx].sh_entsize  = (U64)shdr32.sh_entsize;
          }break;
          case ELF_Class_64:
          {
            lnx_dmn_read_struct(memory_fd, shdr_vaddr, &shdrs[shdr_idx]);
          }break;
        }
      }
      
      // rjf: get string shdr
      ELF_Shdr64 *string_shdr = 0;
      if(shdrs_strings_idx < shdrs_count)
      {
        string_shdr = &shdrs[shdrs_strings_idx];
      }
      
      // rjf: scan shdrs, using names
      if(string_shdr != 0)
      {
        for EachIndex(shdr_idx, shdrs_count)
        {
          ELF_Shdr64 *shdr = &shdrs[shdr_idx];
          U64 name_vaddr = string_shdr->sh_addr + shdr->sh_name;
          String8 shdr_name = lnx_dmn_read_string(scratch.arena, memory_fd, name_vaddr);
          if(str8_match(shdr_name, s(".raddbg"), 0))
          {
            raddbg_info_voff_range = r1u64(shdr->sh_addr - base_vaddr, shdr->sh_addr - base_vaddr + shdr->sh_size);
          }
          if(str8_match(shdr_name, s(".rdbgia"), 0))
          {
            raddbg_is_attached_marker_voff = (shdr->sh_addr - base_vaddr);
          }
        }
      }
    }
    
    //- rjf: unpack tls info
    U64 tls_index = max_U64;
    U64 tls_offset = max_U64;
    if(is_main)
    {
      tls_index = 1;
      tls_offset = 0;
    }
    else if(lnx_dmn_state->is_tls_detected)
    {
      Rng1U64 tls_modid_range  = r1u64(lnx_dmn_state->tls_modid_desc.offset, lnx_dmn_state->tls_modid_desc.offset + lnx_dmn_state->tls_modid_desc.bit_size / 8);
      Rng1U64 tls_offset_range = r1u64(lnx_dmn_state->tls_offset_desc.offset, lnx_dmn_state->tls_offset_desc.offset + lnx_dmn_state->tls_offset_desc.bit_size / 8);
      tls_modid_range  = shift_1u64(tls_modid_range, base_vaddr);
      tls_offset_range = shift_1u64(tls_offset_range, base_vaddr);
      tls_modid_range.max = Min(tls_modid_range.max, tls_modid_range.min + sizeof(tls_index));
      tls_offset_range.max = Min(tls_offset_range.max, tls_offset_range.min + sizeof(tls_offset));
      lnx_dmn_read(memory_fd, tls_modid_range, &tls_index);
      lnx_dmn_read(memory_fd, tls_offset_range, &tls_offset);
    }
    
    //- rjf: read module path
    String8 module_path = lnx_dmn_read_string(arena, memory_fd, name_vaddr);
    
    //- rjf: decide on debug info key info
    String8 debug_info_path = module_path;
    Guid debug_info_guid = {0};
    U64 debug_info_timestamp = 0;
    U64 debug_info_age = 0;
    
    //- rjf: fill result
    info->arch                           = arch;
    info->vsize                          = image_vsize;
    info->entry_point_voff               = entry_point_voff;
    info->module_path                    = module_path;
    info->debug_info_path                = debug_info_path;
    info->debug_info_guid                = debug_info_guid;
    info->debug_info_timestamp           = debug_info_timestamp;
    info->debug_info_age                 = debug_info_age;
    info->tls_index                      = tls_index;
    info->tls_offset                     = tls_offset;
    info->eh_frame_header_vaddr_range    = eh_frame_header_vaddr_range;
    info->cfi_rebase                     = module_rebase;
    info->raddbg_info_voff_range         = raddbg_info_voff_range;
    info->raddbg_is_attached_marker_voff = raddbg_is_attached_marker_voff;
  }
  scratch_end(scratch);
  return info;
}

////////////////////////////////
//~ rjf: Trap Setting

internal LNX_DMN_ActiveTrap *
lnx_dmn_set_trap(Arena *arena, DMN_Trap *trap)
{
  ARCH_Info *arch = arch_info_from_arch(Arch_CURRENT);
  String8 trap_inst = arch->trap_instruction;
  U8 *swap_bytes = push_array(arena, U8, trap_inst.size);
  B32 good_read = dmn_process_read(trap->process, r1u64(trap->vaddr, trap->vaddr + trap_inst.size), swap_bytes);
  B32 good_write = 0;
  if(good_read)
  {
    good_write = dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr + trap_inst.size), trap_inst.str);
  }
  LNX_DMN_ActiveTrap *result = push_array(arena, LNX_DMN_ActiveTrap, 1);
  result->good = (good_read && good_write);
  result->trap = trap;
  result->swap_bytes = str8(swap_bytes, trap_inst.size);
  return result;
}

internal Rng1U64
lnx_dmn_compute_image_vrange(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum)
{ 
  Rng1U64 result = { .min = max_U64 };
  
  for(U64 ph_cursor = e_phaddr, ph_opl = (e_phaddr + e_phentsize * e_phnum); ph_cursor < ph_opl; ph_cursor += e_phentsize)
  {
    ELF_Phdr64 phdr = {0};
    if(elf_read_phdr(lnx_dmn_machine_op_mem_read, &memory_fd, ph_cursor, elf_class, &phdr) != MachineOpResult_Ok)
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

internal LNX_DMN_DynamicInfo
lnx_dmn_dynamic_info_from_memory(int memory_fd, ELF_Class elf_class, U64 rebase, U64 dynamic_vaddr)
{
  LNX_DMN_DynamicInfo dynamic_info = {0};
  for(U64 dynamic_cursor = dynamic_vaddr; ; dynamic_cursor += elf_dyn_size_from_class(elf_class))
  {
    // rjf: read next dyn entry
    ELF_Dyn64 dyn = {0};
    if(elf_read_dyn(lnx_dmn_machine_op_mem_read, &memory_fd, dynamic_cursor, elf_class, &dyn) != MachineOpResult_Ok) { Assert(0 && "unable to read dynamic"); }
    
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
lnx_dmn_find_dynamic_phdr(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum)
{
  U64 result = max_U64;
  
  for(U64 ph_cursor = e_phaddr, ph_opl = (e_phaddr + e_phentsize * e_phnum); ph_cursor < ph_opl; ph_cursor += e_phentsize)
  {
    ELF_Phdr64 phdr = {0};
    if(elf_read_phdr(lnx_dmn_machine_op_mem_read, &memory_fd, ph_cursor, elf_class, &phdr) != MachineOpResult_Ok)
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
lnx_dmn_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vbase, B32 is_rebased)
{
  Temp scratch = scratch_begin(0, 0);
  
  U64 rdebug_vaddr = 0;
  
  // load DL's header
  ELF_Hdr64 ehdr = {0};
  if(elf_read_ehdr(lnx_dmn_machine_op_mem_read, &memory_fd, loader_vbase, &ehdr) != MachineOpResult_Ok) { Assert(0 && "failed to read interp's header"); goto exit; }
  
  U64       rebase    = ehdr.e_type == ELF_Type_Dyn ? loader_vbase : 0;
  ELF_Class elf_class = ehdr.e_ident[ELF_Identifier_Class];
  
  // find dynamic program header
  U64 phdr_vaddr    = loader_vbase + ehdr.e_phoff;
  U64 dynamic_vaddr = lnx_dmn_find_dynamic_phdr(memory_fd, elf_class, rebase, phdr_vaddr, ehdr.e_phentsize, ehdr.e_phentsize);
  
  // extract necessary info out of dynamic program header
  U64                 dynamic_info_rebase = is_rebased ? 0 : rebase;
  LNX_DMN_DynamicInfo dynamic_info        = lnx_dmn_dynamic_info_from_memory(memory_fd, elf_class, dynamic_info_rebase, dynamic_vaddr);
  
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
    if(lnx_dmn_read(memory_fd, r1u64(dynamic_info.hash_vaddr, dynamic_info.hash_vaddr + hash_entry_size), &chain_count) == hash_entry_size)
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
      if(elf_read_symbol(lnx_dmn_machine_op_mem_read, &memory_fd, dynamic_info.symtab_vaddr + symbol_idx * dynamic_info.symtab_entry_size, elf_class, &symbol) != MachineOpResult_Ok)
      {
        Assert(0 && "failed to read symbol table");
        break;
      }
      
      Temp temp = temp_begin(scratch.arena);
      
      String8 symbol_name = {0};
      if(symbol.st_name < dynamic_info.strtab_size)
      {
        U64 cap = dynamic_info.strtab_size - symbol.st_name;
        symbol_name = lnx_dmn_read_string_capped(temp.arena, memory_fd, dynamic_info.strtab_vaddr + symbol.st_name, cap);
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

internal LNX_DMN_ProbeList
lnx_dmn_read_probes(Arena *arena, int fd, U64 offset, U64 image_base)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  LNX_DMN_ProbeList probes = {0};
  
  ELF_Hdr64 ehdr = {0};
  if(elf_read_ehdr(lnx_dmn_machine_op_mem_read, &fd, offset, &ehdr) != MachineOpResult_Ok) { goto exit; }
  
  U64        strtab_shdr_offset = offset + ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize;
  ELF_Shdr64 strtab_shdr        = {0};
  if(elf_read_shdr(lnx_dmn_machine_op_mem_read, &fd, strtab_shdr_offset, ehdr.e_ident[ELF_Identifier_Class], &strtab_shdr) != MachineOpResult_Ok) { goto exit; }
  
  B32 found_probes      = 0;
  B32 found_probes_base = 0;
  ELF_Shdr64 text_shdr         = {0};
  ELF_Shdr64 stapsdt_base_shdr = {0};
  ELF_Shdr64 stapsdt_shdr      = {0};
  for(U64 shdr_off = offset + ehdr.e_shoff, shdr_opl = shdr_off + ehdr.e_shentsize * ehdr.e_shnum;
      shdr_off < shdr_opl;
      shdr_off += ehdr.e_shentsize) {
    ELF_Shdr64 shdr = {0};
    if(elf_read_shdr(lnx_dmn_machine_op_mem_read, &fd, shdr_off, ehdr.e_ident[ELF_Identifier_Class], &shdr) != MachineOpResult_Ok) { goto exit; }
    
    if(shdr.sh_type == ELF_ShType_Note)
    {
      U64     name_offset = offset + strtab_shdr.sh_offset + shdr.sh_name;
      U64     name_cap    = offset + strtab_shdr.sh_offset + strtab_shdr.sh_size;
      String8 name        = lnx_dmn_read_string_capped(scratch.arena, fd, name_offset, name_cap);
      
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
      String8 name        = lnx_dmn_read_string_capped(scratch.arena, fd, name_offset, name_cap);
      
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
  U64      note_read_size = lnx_dmn_read(fd, note_range, raw_note);
  if(note_read_size != dim_1u64(note_range)) { goto exit; }
  
  Arch         arch = arch_from_elf_machine(ehdr.e_machine);
  ELF_NoteList note = elf_parse_note(scratch.arena, str8(raw_note, dim_1u64(note_range)), ehdr.e_ident[ELF_Identifier_Class], ehdr.e_machine);
  
  for EachNode(n, ELF_NoteNode, note.first)
  {
    ELF_Note *note = &n->v;
    if(!str8_match(note->owner, str8_lit("stapsdt"), 0)) { continue; }
    if(note->type != ELF_NoteType_STapSdt)               { continue; }
    
    LNX_DMN_Probe probe = {0};
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
    
    LNX_DMN_ProbeNode *n = push_array(arena, LNX_DMN_ProbeNode, 1);
    n->v = probe;
    SLLQueuePush(probes.first, probes.last, n);
    probes.count += 1;
  }
  
  exit:;
  scratch_end(scratch);
  return probes;
}

internal
STAP_MEMORY_READ(lnx_dmn_stap_memory_read)
{
  LNX_DMN_Process *process = raw_ctx;
  U64 bytes_read = lnx_dmn_read(process->fd, r1u64(addr, addr + read_size), buffer);
  return bytes_read == read_size;
}

internal LNX_DMN_Entity *
lnx_dmn_entity_alloc(LNX_DMN_EntityKind kind)
{
  LNX_DMN_Entity *entity = lnx_dmn_state->free_entity;
  if(entity != 0)
  {
    SLLStackPop(lnx_dmn_state->free_entity);
  }
  else
  {
    entity = push_array(lnx_dmn_state->entities_arena, LNX_DMN_Entity, 1);
    lnx_dmn_state->entities_count += 1;
  }
  U32 gen = entity->gen;
  entity->gen += 1;
  entity->kind = kind;
  return entity;
}

internal LNX_DMN_Process *
lnx_dmn_process_alloc(pid_t pid, LNX_DMN_ProcessState state, LNX_DMN_Process *parent_process, B32 debug_subprocesses, B32 is_cow)
{
  Temp scratch = scratch_begin(0, 0);
  
  LNX_DMN_Process *process = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_Process)->process;
  process->pid                = pid;
  process->fd                 = LNX_RETRY_ON_EINTR(open((char *)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR));
  process->state              = state;
  process->debug_subprocesses = debug_subprocesses;
  process->is_cow             = is_cow;
  process->parent_process     = parent_process;
  
  // update pending process tracker
  if(state != LNX_DMN_ProcessState_Normal)
  {
    lnx_dmn_state->process_pending_creation += 1;
  }
  
  // add process to the list
  DLLPushBack(lnx_dmn_state->first_process, lnx_dmn_state->last_process, process);
  lnx_dmn_state->process_count += 1;
  
  // push pid -> LNX_DMN_Process mapping
  hash_table_push_u64_raw(lnx_dmn_state->arena, lnx_dmn_state->pid_ht, pid, process);
  
  scratch_end(scratch);
  return process;
}

internal LNX_DMN_ProcessCtx *
lnx_dmn_process_ctx_alloc(LNX_DMN_Process *process, B32 is_rebased)
{
  LNX_DMN_ProcessCtx *ctx = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_ProcessCtx)->process_ctx;
  
  ELF_Hdr64     exe_ehdr     = lnx_dmn_ehdr_from_pid(process->pid);
  LNX_DMN_Auxv  auxv         = lnx_dmn_auxv_from_pid(process->pid, exe_ehdr.e_ident[ELF_Identifier_Class]);
  Arch          arch         = arch_from_elf_machine(exe_ehdr.e_machine);
  U64           rdebug_vaddr = lnx_dmn_rdebug_vaddr_from_memory(process->fd, auxv.base, is_rebased);
  U64           base_vaddr   = (auxv.phdr & ~(auxv.pagesz-1));
  U64           rebase       = exe_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  Rng1U64       image_vrange = lnx_dmn_compute_image_vrange(process->fd, exe_ehdr.e_ident[ELF_Identifier_Class], rebase, auxv.phdr, auxv.phent, auxv.phnum);
  Arena        *ctx_arena    = arena_alloc();
  
  ELF_Class dl_class;
  {
    ELF_Hdr64 ehdr = {0};
    if(elf_read_ehdr(lnx_dmn_machine_op_mem_read, &process->fd, auxv.base, &ehdr) != MachineOpResult_Ok) { Assert(0 && "failed to read interp's header"); }
    dl_class = ehdr.e_ident[ELF_Identifier_Class];
  }
  
  // query xsave layout
  U64             xcr0         = 0;
  U64             xsave_size   = 0;
  X64_XSaveLayout xsave_layout = {0};
  if(arch == Arch_x64)
  {
    X64_XSave xsave = {0};
    if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, process->pid, (void *)NT_X86_XSTATE, &(struct iovec){.iov_base = &xsave, .iov_len = sizeof(xsave) }) >= 0))
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
  LNX_DMN_Probe **known_probes = push_array(ctx_arena, LNX_DMN_Probe *, LNX_DMN_ProbeType_Count);
  {
    Temp scratch = scratch_begin(0, 0);
    
    String8 dl_path = lnx_dmn_dl_path_from_pid(scratch.arena, process->pid, auxv.base);
    int dl_fd = LNX_RETRY_ON_EINTR(open((char *)dl_path.str, O_RDONLY));
    
    LNX_DMN_ProbeList probes = {0};
    if(dl_fd >= 0)
    {
      probes = lnx_dmn_read_probes(ctx_arena, dl_fd, 0, auxv.base);
      LNX_RETRY_ON_EINTR(close(dl_fd));
    }
    
    for EachNode(n, LNX_DMN_ProbeNode, probes.first)
    {
      LNX_DMN_Probe *p = &n->v;
      if(str8_match(p->provider, str8_lit("rtld"), 0))
      {
#define X(_N,_A,_S) if(str8_match(p->name, str8_lit(_S), 0)) { AssertAlways(p->args.count == _A); known_probes[LNX_DMN_ProbeType_##_N] = p; continue ; }
        LNX_DMN_Probe_XList
#undef X
      }
    }
    
    scratch_end(scratch);
  }
  
  ctx = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_ProcessCtx)->process_ctx;
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
  LNX_DMN_Module *main_module = lnx_dmn_module_alloc(ctx, process->fd, base_vaddr, auxv.execfn, 1, 1);
  
  // glibc has a shortcut mapping for the main module
  hash_table_push_u64_raw(ctx->arena, ctx->loaded_modules_ht, 0, main_module);
  
  return ctx;
}

internal LNX_DMN_Thread *
lnx_dmn_thread_alloc(LNX_DMN_Process *process, LNX_DMN_ThreadState thread_state, pid_t tid)
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
    ARCH_Info *arch_info = arch_info_from_arch(process->ctx->arch);
    U64 reg_block_size = arch_info->reg_block_size;
    reg_block = push_array(process->ctx->arena, U8, reg_block_size);
  }
  
  LNX_DMN_Thread *thread = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_Thread)->thread;
  thread->tid       = tid;
  thread->state     = thread_state;
  thread->process   = process;
  thread->reg_block = reg_block;
  if(thread_state == LNX_DMN_ThreadState_Stopped)
  {
    thread->is_reg_block_dirty = !lnx_dmn_thread_read_reg_block(thread);
  }
  
  // add thread to the list
  DLLPushBack(process->first_thread, process->last_thread, thread);
  process->thread_count += 1;
  
  // push tid -> thread mapping
  hash_table_push_u64_raw(lnx_dmn_state->arena, lnx_dmn_state->tid_ht, thread->tid, thread);
  
  // update global thread counter
  if(thread_state == LNX_DMN_ThreadState_PendingCreation)
  {
    lnx_dmn_state->threads_pending_creation += 1;
  }
  
  return thread;
}

internal LNX_DMN_Module *
lnx_dmn_module_alloc(LNX_DMN_ProcessCtx *ctx, int memory_fd, U64 base_vaddr, U64 name_vaddr, U64 name_space_id, B32 is_main)
{
  LNX_DMN_Module *module = hash_table_search_u64_raw(ctx->loaded_modules_ht, base_vaddr);
  if(module) { goto exit; }
  
  // parse out module's ELF header
  ELF_Hdr64 module_ehdr = {0};
  if(elf_read_ehdr(lnx_dmn_machine_op_mem_read, &memory_fd, base_vaddr, &module_ehdr) != MachineOpResult_Ok) { goto exit; }
  
  // gather info about module
  U64     module_rebase     = module_ehdr.e_type == ELF_Type_Dyn ? base_vaddr : 0;
  U64     module_phdr_vaddr = module_rebase + module_ehdr.e_phoff;
  Rng1U64 module_vrange     = lnx_dmn_compute_image_vrange(memory_fd, module_ehdr.e_ident[ELF_Identifier_Class], module_rebase, module_phdr_vaddr, module_ehdr.e_phentsize, module_ehdr.e_phnum);
  
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
    if(lnx_dmn_state->is_tls_detected)
    {
      Rng1U64 tls_modid_range  = r1u64(lnx_dmn_state->tls_modid_desc.offset, lnx_dmn_state->tls_modid_desc.offset + lnx_dmn_state->tls_modid_desc.bit_size / 8);
      Rng1U64 tls_offset_range = r1u64(lnx_dmn_state->tls_offset_desc.offset, lnx_dmn_state->tls_offset_desc.offset + lnx_dmn_state->tls_offset_desc.bit_size / 8);
      tls_modid_range  = shift_1u64(tls_modid_range, base_vaddr);
      tls_offset_range = shift_1u64(tls_offset_range, base_vaddr);
      if(!lnx_dmn_read(memory_fd, tls_modid_range, &tls_index))   { Assert(0 && "failed to read TLS index");  }
      if(!lnx_dmn_read(memory_fd, tls_offset_range, &tls_offset)) { Assert(0 && "failed to read TLS offset"); }
    }
  }
  
  module = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_Module)->module;
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
lnx_dmn_entity_release(LNX_DMN_Entity *entity)
{
  U32 gen = entity->gen + 1;
  MemoryZeroStruct(entity);
  entity->gen = gen;
  SLLStackPush(lnx_dmn_state->free_entity, entity);
}

internal void
lnx_dmn_process_release(LNX_DMN_Process *process)
{
  // update global state
  AssertAlways(lnx_dmn_state->process_count > 0);
  DLLRemove(lnx_dmn_state->first_process, lnx_dmn_state->last_process, process);
  lnx_dmn_state->process_count -= 1;
  
  // update pending process tracker
  if(process->state != LNX_DMN_ProcessState_Normal)
  {
    Assert(lnx_dmn_state->process_pending_creation > 0);
    lnx_dmn_state->process_pending_creation -= 1;
  }
  
  // close memory handle
  if(LNX_RETRY_ON_EINTR(close(process->fd)) < 0) { Assert(0 && "failed to close memory descriptor"); }
  
  // remove pid mapping
  hash_table_purge_u64(lnx_dmn_state->pid_ht, process->pid);
  
  // release the context
  if(process->ctx)
  {
    lnx_dmn_process_ctx_release(process->ctx);
  }
  
  // release process entity
  lnx_dmn_entity_release((LNX_DMN_Entity *)process);
}

internal void
lnx_dmn_process_ctx_release(LNX_DMN_ProcessCtx *ctx)
{
  Assert(ctx->ref_count > 0);
  ctx->ref_count -= 1;
  
  if(ctx->ref_count == 0)
  {
    arena_release(ctx->arena);
    lnx_dmn_entity_release((LNX_DMN_Entity *)ctx);
  }
}

internal void
lnx_dmn_thread_release(LNX_DMN_Thread *thread)
{
  LNX_DMN_Process *process = thread->process;
  
  // purge tid mapping
  hash_table_purge_u64(lnx_dmn_state->tid_ht, thread->tid);
  
  // update global thread counter
  if(thread->state == LNX_DMN_ThreadState_PendingCreation)
  {
    AssertAlways(lnx_dmn_state->threads_pending_creation > 0);
    lnx_dmn_state->threads_pending_creation -= 1;
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
  
  lnx_dmn_entity_release((LNX_DMN_Entity *)thread);
}

internal void
lnx_dmn_module_release(LNX_DMN_ProcessCtx *ctx, LNX_DMN_Module *module)
{
  // remove module from the list
  Assert(ctx->module_count > 0);
  DLLRemove(ctx->first_module, ctx->last_module, module);
  ctx->module_count -= 1;
  
  // purge base addr -> module mapping
  hash_table_purge_u64(ctx->loaded_modules_ht, module->base_vaddr);
  
  lnx_dmn_entity_release((LNX_DMN_Entity *)module);
}

internal LNX_DMN_ProcessCtx *
lnx_dmn_process_ctx_clone(LNX_DMN_Process *new_owner, LNX_DMN_ProcessCtx *ctx)
{
  LNX_DMN_ProcessCtx *result = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_ProcessCtx)->process_ctx;
  
  result->arena             = arena_alloc();
  result->arch              = ctx->arch;
  result->rdebug_vaddr      = ctx->rdebug_vaddr;
  result->dl_class          = ctx->dl_class;
  result->loaded_modules_ht = hash_table_init(result->arena, ctx->loaded_modules_ht->cap);
  result->xcr0              = ctx->xcr0;
  result->xsave_size        = ctx->xsave_size;
  result->xsave_layout      = ctx->xsave_layout;
  
  // clone probes
  result->probes = push_array(result->arena, LNX_DMN_Probe *, LNX_DMN_ProbeType_Count);
  for EachIndex(probe_idx, LNX_DMN_ProbeType_Count)
  {
    LNX_DMN_Probe *dst = result->probes[probe_idx];
    LNX_DMN_Probe *src = ctx->probes[probe_idx];
    
    dst->provider    = str8_copy(result->arena, src->provider);
    dst->name        = str8_copy(result->arena, src->name);
    dst->args_string = str8_copy(result->arena, src->args_string);
    dst->args        = stap_arg_array_copy(result->arena, src->args);
    dst->pc          = src->pc;
    dst->semaphore   = src->semaphore;
  }
  
  // clone probe traps
  for EachNode(src, LNX_DMN_ActiveTrap, ctx->first_probe_trap)
  {
    DMN_Trap *src_trap = src->trap;
    DMN_Trap *dst_trap = push_array(result->arena, DMN_Trap, 1);
    dst_trap->process = lnx_dmn_handle_from_process(new_owner);
    dst_trap->vaddr   = src_trap->vaddr;
    dst_trap->id      = src_trap->id;
    dst_trap->flags   = src_trap->flags;
    dst_trap->size    = src_trap->size;
    
    LNX_DMN_ActiveTrap *dst = push_array(result->arena, LNX_DMN_ActiveTrap, 1);
    dst->trap       = dst_trap;
    dst->swap_bytes = str8_copy(result->arena, src->swap_bytes);
    
    SLLQueuePush(result->first_probe_trap, result->last_probe_trap, dst);
  }
  
  // clone modules
  for EachNode(module, LNX_DMN_Module, ctx->first_module)
  {
    lnx_dmn_module_clone(result, module);
  }
  
  return result;
}

internal LNX_DMN_Module *
lnx_dmn_module_clone(LNX_DMN_ProcessCtx *process_ctx, LNX_DMN_Module *module)
{
  LNX_DMN_Module *result = &lnx_dmn_entity_alloc(LNX_DMN_EntityKind_Module)->module;
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
lnx_dmn_handle_from_entity(LNX_DMN_Entity *entity)
{
  DMN_Handle handle = {0};
  U64 index = IntFromPtr(entity - lnx_dmn_state->entities_base);
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
lnx_dmn_handle_from_process(LNX_DMN_Process *process)
{
  return lnx_dmn_handle_from_entity((LNX_DMN_Entity *)process);
}

internal DMN_Handle
lnx_dmn_handle_from_process_ctx(LNX_DMN_ProcessCtx *process_ctx)
{
  return lnx_dmn_handle_from_entity((LNX_DMN_Entity *)process_ctx);
}

internal DMN_Handle
lnx_dmn_handle_from_thread(LNX_DMN_Thread *thread)
{
  return lnx_dmn_handle_from_entity((LNX_DMN_Entity *)thread);
}

internal DMN_Handle
lnx_dmn_handle_from_module(LNX_DMN_Module *module)
{
  return lnx_dmn_handle_from_entity((LNX_DMN_Entity *)module);
}

internal LNX_DMN_Entity *
lnx_dmn_entity_from_handle(DMN_Handle handle, LNX_DMN_EntityKind expected_kind)
{
  LNX_DMN_Entity *result = 0;
  U32 index = handle.u32[0];
  U32 gen   = handle.u32[1];
  if(index < lnx_dmn_state->entities_count && lnx_dmn_state->entities_base[index].gen == gen)
  {
    if(lnx_dmn_state->entities_base[index].kind == expected_kind)
    {
      result = &lnx_dmn_state->entities_base[index];
    }
  }
  return result;
}

internal LNX_DMN_Process *
lnx_dmn_process_from_handle(DMN_Handle process_handle)
{
  return (LNX_DMN_Process *)lnx_dmn_entity_from_handle(process_handle, LNX_DMN_EntityKind_Process);
}

internal LNX_DMN_ProcessCtx *
lnx_dmn_process_ctx_from_handle(DMN_Handle process_ctx_handle)
{
  return (LNX_DMN_ProcessCtx *)lnx_dmn_entity_from_handle(process_ctx_handle, LNX_DMN_EntityKind_ProcessCtx);
}

internal LNX_DMN_Thread *
lnx_dmn_thread_from_handle(DMN_Handle thread_handle)
{
  return (LNX_DMN_Thread *)lnx_dmn_entity_from_handle(thread_handle, LNX_DMN_EntityKind_Thread);
}

internal LNX_DMN_Module *
lnx_dmn_module_from_handle(DMN_Handle module_handle)
{
  return (LNX_DMN_Module *)lnx_dmn_entity_from_handle(module_handle, LNX_DMN_EntityKind_Module);
}

internal void
lnx_dmn_process_trap_probes(LNX_DMN_Process *process)
{
  for EachIndex(i, LNX_DMN_ProbeType_Count)
  {
    if(process->ctx->probes[i] == 0) { continue; }
    
    DMN_Trap *trap = push_array(process->ctx->arena, DMN_Trap, 1);
    trap->process = lnx_dmn_handle_from_process(process);
    trap->vaddr   = process->ctx->probes[i]->pc;
    trap->id      = i;
    
    LNX_DMN_ActiveTrap *active_trap = lnx_dmn_set_trap(process->ctx->arena, trap);
    SLLQueuePush(process->ctx->first_probe_trap, process->ctx->last_probe_trap, active_trap);
    
    if(BUILD_DEBUG && process->ctx->arch == Arch_x64)
    {
      Assert(active_trap->swap_bytes.size == 1 && active_trap->swap_bytes.str[0] == 0x90);
    }
  }
}

internal void
lnx_dmn_process_untrap_probes(LNX_DMN_Process *process)
{
  NotImplemented;
}

internal U64
lnx_dmn_thread_read_ip(LNX_DMN_Thread *thread)
{
  ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
  U64 result = arch_ip_from_reg_block(arch_info, thread->reg_block);
  return result;
}

internal U64
lnx_dmn_thread_read_sp(LNX_DMN_Thread *thread)
{
  ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
  U64 result = arch_sp_from_reg_block(arch_info, thread->reg_block);
  return result;
}

internal void
lnx_dmn_thread_write_ip(LNX_DMN_Thread *thread, U64 ip)
{
  ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
  arch_reg_block_write_ip(arch_info, thread->reg_block, ip);
  thread->is_reg_block_dirty = 1;
}

internal void
lnx_dmn_thread_write_sp(LNX_DMN_Thread *thread, U64 sp)
{
  ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
  arch_reg_block_write_sp(arch_info, thread->reg_block, sp);
  thread->is_reg_block_dirty = 1;
}

internal B32
lnx_dmn_thread_read_reg_block(LNX_DMN_Thread *thread)
{
  B32 is_reg_block_read = 0;
  if(thread->state == LNX_DMN_ThreadState_Stopped)
  {
    switch(thread->process->ctx->arch)
    {
      default:{}break;
      case Arch_x64:
      {
        LNX_DMN_ProcessCtx *process_ctx = thread->process->ctx;
        X64_RegBlock *dst = thread->reg_block;
        
        // general purpose registers
        {
          LNX_DMN_GprsX64 src;
          int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_len = sizeof(src), .iov_base = &src }));
          if(ptrace_result < 0) { goto exit;  }
          
          dst->r15     = src.r15;
          dst->r14     = src.r14;
          dst->r13     = src.r13;
          dst->r12     = src.r12;
          dst->rbp     = src.rbp;
          dst->rbx     = src.rbx;
          dst->r11     = src.r11;
          dst->r10     = src.r10;
          dst->r9      = src.r9;
          dst->r8      = src.r8;
          dst->rax     = src.rax;
          dst->rcx     = src.rcx;
          dst->rdx     = src.rdx;
          dst->rsi     = src.rsi;
          dst->rdi     = src.rdi;
          dst->rip     = src.rip;
          dst->cs      = src.cs;
          dst->rflags  = src.rflags;
          dst->rsp     = src.rsp;
          dst->ss      = src.ss;
          dst->fsbase  = src.fsbase;
          dst->gsbase  = src.gsbase;
          dst->ds      = src.ds;
          dst->es      = src.es;
          dst->fs      = src.fs;
          dst->gs      = src.gs;
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
            void *xsave_raw = push_array(scratch.arena, U8, process_ctx->xsave_size);
            int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_len = process_ctx->xsave_size, .iov_base = xsave_raw }));
            if(ptrace_result < 0) { goto exit; }
            xsave  = xsave_raw;
            fxsave = &xsave->fxsave;
          }
          
          // get fxsave
          if(fxsave == 0)
          {
            fxsave = push_array(scratch.arena, X64_FXSave, 1);
            int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETREGSET, thread->tid, (void *)NT_FPREGSET, &(struct iovec){ .iov_len = sizeof(*fxsave), .iov_base = fxsave }));
            if(ptrace_result < 0) { goto exit; }
            fxsave = 0;
          }
          
          // copy fxsave registers
          if(fxsave)
          {
            X64_FXSave *src = fxsave;
            
            // copy x87 registers
            dst->fcw = src->fcw;
            dst->fsw = src->fsw;
            dst->ftw = src->ftw;
            dst->fop = src->fop;
            dst->fip = src->fip;
            dst->fdp = src->fdp;
            dst->mxcsr = src->mxcsr;
            dst->mxcsr_mask = src->mxcsr_mask;
            for EachIndex(i, 8)
            {
              MemoryCopy(&dst->st0 + i, src->st_space + i, sizeof(U80));
            }
            
            // SSE registers are always available in x64
            {
              U128 *xmm_d = fxsave->xmm_space;
              U512 *zmm_s = &dst->zmm0;
              for EachIndex(i, 16)
              {
                MemoryCopy(&zmm_s[i], &xmm_d[i], sizeof(*xmm_d));
              }
            }
          }
          
          // copy xsave registers
          //
          // compact register layout is not supported (xsave->header.xcomp_bv == 0)
          //
          if(xsave && xsave->header.xcomp_bv == 0)
          {
            if(xsave->header.xstate_bv & X64_XStateComponentFlag_AVX &&
               process_ctx->xsave_layout.avx_offset + 16*sizeof(U128) <= process_ctx->xsave_size)
            {
              U128 *avx_s = (U128 *)((U8 *)xsave + process_ctx->xsave_layout.avx_offset);
              U512 *zmm_d = &dst->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&zmm_d[n].u8[16], &avx_s[n], sizeof(U128));
              }
            }
            
            if(xsave->header.xstate_bv & X64_XStateComponentFlag_OPMASK &&
               process_ctx->xsave_layout.opmask_offset + sizeof(U64) * 8 <= process_ctx->xsave_size)
            {
              U64 *kmask_s = (U64 *)((U8 *)xsave + process_ctx->xsave_layout.opmask_offset);
              U64 *kmask_d = &dst->k0;
              for EachIndex(n, 8)
              {
                MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(U64));
              }
            }
            
            if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM_H &&
               process_ctx->xsave_layout.zmm_h_offset + sizeof(U256) * 16 <= process_ctx->xsave_size)
            {
              U256 *avx512h_s = (U256 *)((U8 *)xsave + process_ctx->xsave_layout.zmm_h_offset);
              U512 *zmmh_d    = &dst->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&zmmh_d[n].u8[32], &avx512h_s[n], sizeof(U256));
              }
            }
            
            if(xsave->header.xstate_bv & X64_XStateComponentFlag_ZMM &&
               process_ctx->xsave_layout.zmm_offset + sizeof(U512) * 16 <= process_ctx->xsave_size)
            {
              U512 *avx512_s = (U512 *)((U8 *)xsave + process_ctx->xsave_layout.zmm_offset);
              U512 *zmm_d = &dst->zmm16;
              for EachIndex(n, 16)
              {
                MemoryCopy(&zmm_d[n], &avx512_s[n], sizeof(U512));
              }
            }
            
            if(xsave->header.xstate_bv & X64_XStateComponentFlag_CETU &&
               process_ctx->xsave_layout.cet_u_offset + sizeof(U64)*2 <= process_ctx->xsave_size)
            {
              U64 *cet_u = (U64 *)((U8 *)xsave + process_ctx->xsave_layout.cet_u_offset);
              dst->cetmsr = cet_u[0];
              dst->cetssp = cet_u[1];
            }
          }
          
          scratch_end(scratch);
        }
        
        // debug registers
        {
          U64 *dr_d = &dst->dr0;
          for EachIndex(n, 8)
          {
            if(n != 4 && n != 5)
            {
              U64 offset = OffsetOf(LNX_DMN_UserX64, u_debugreg[n]);
              errno = 0;
              long peek_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_PEEKUSER, thread->tid, PtrFromInt(offset), 0));
              if(errno != 0) { goto exit; }
              dr_d[n] = peek_result;
            }
          }
        }
        
        is_reg_block_read = 1;
      }break;
    }
  }
  exit:;
  return is_reg_block_read;
}

internal B32
lnx_dmn_thread_write_reg_block(LNX_DMN_Thread *thread)
{
  AssertAlways(thread->state == LNX_DMN_ThreadState_Stopped);
  
  B32 is_reg_block_written = 0;
  
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
    case Arch_x64:
    {
      LNX_DMN_ProcessCtx *process_ctx = thread->process->ctx;
      X64_RegBlock   *src         = thread->reg_block;
      
      // general purpose registers
      {
        LNX_DMN_GprsX64 dst;
        dst.r15      = src->r15;
        dst.r14      = src->r14;
        dst.r13      = src->r13;
        dst.r12      = src->r12;
        dst.rbp      = src->rbp;
        dst.rbx      = src->rbx;
        dst.r11      = src->r11;
        dst.r10      = src->r10;
        dst.r9       = src->r9;
        dst.r8       = src->r8;
        dst.rax      = src->rax;
        dst.rcx      = src->rcx;
        dst.rdx      = src->rdx;
        dst.rsi      = src->rsi;
        dst.rdi      = src->rdi;
        dst.orig_rax = thread->orig_rax;
        dst.rip      = src->rip;
        dst.cs       = src->cs;
        dst.rflags   = src->rflags;
        dst.rsp      = src->rsp;
        dst.ss       = src->ss;
        dst.fsbase   = src->fsbase;
        dst.gsbase   = src->gsbase;
        dst.ds       = src->ds;
        dst.es       = src->es;
        dst.fs       = src->fs;
        dst.gs       = src->gs;
        int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, thread->tid, (void *)NT_PRSTATUS, &(struct iovec){ .iov_base = &dst, .iov_len = sizeof(dst) }));
        if(ptrace_result < 0) { goto exit; }
      }
      
      // xsave
      {
        Temp scratch = scratch_begin(0, 0);
        
        X64_FXSave dst_fxsave = {0};
        {
          dst_fxsave.fcw        = src->fcw;
          dst_fxsave.fsw        = src->fsw;
          dst_fxsave.ftw        = src->ftw;
          dst_fxsave.fop        = src->fop;
          dst_fxsave.fip        = src->fip;
          dst_fxsave.fdp        = src->fdp;
          dst_fxsave.mxcsr      = src->mxcsr;
          dst_fxsave.mxcsr_mask = src->mxcsr_mask;
          
          U128 *st_d = (U128 *)dst_fxsave.st_space;
          U80  *st_s = &src->st0;
          for EachIndex(n, 8)
          {
            MemoryCopy(&st_d[n], &st_s[n], sizeof(U80));
          }
          
          U128 *xmm_d = (U128 *)dst_fxsave.xmm_space;
          U512 *xmm_s = &src->zmm0;
          for EachIndex(n, 16)
          {
            MemoryCopy(&xmm_d[n], &xmm_s[n], sizeof(U128));
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
            if(process_ctx->xsave_layout.avx_offset + sizeof(U128) * 16 <= process_ctx->xsave_size)
            {
              U128 *avx_d = (U128 *)(xsave_raw + process_ctx->xsave_layout.avx_offset);
              U512 *zmm_s = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx_d[n], &zmm_s[n].u8[16], sizeof(U128));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_AVX;
            }
          }
          
          if(process_ctx->xsave_layout.opmask_offset)
          {
            if(process_ctx->xsave_layout.opmask_offset + sizeof(U64) * 8 <= process_ctx->xsave_size)
            {
              U64 *kmask_d = (U64 *)(xsave_raw + process_ctx->xsave_layout.opmask_offset);
              U64 *kmask_s = &src->k0;
              for EachIndex(n, 8)
              {
                MemoryCopy(&kmask_d[n], &kmask_s[n], sizeof(U64));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_OPMASK;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.zmm_h_offset)
          {
            if(process_ctx->xsave_layout.zmm_h_offset + sizeof(U256) * 16 <= process_ctx->xsave_size)
            {
              U256 *avx512h_d = (U256 *)(xsave_raw + process_ctx->xsave_layout.zmm_h_offset);
              U512 *zmmh_s    = &src->zmm0;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512h_d[n], &zmmh_s[n].u8[32], sizeof(U256));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM_H;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.zmm_offset)
          {
            if(process_ctx->xsave_layout.zmm_offset + sizeof(U512) * 16 <= process_ctx->xsave_size)
            {
              U512 *avx512_d = (U512 *)(xsave_raw + process_ctx->xsave_layout.zmm_offset);
              U512 *zmm_s    = &src->zmm16;
              for EachIndex(n, 16)
              {
                MemoryCopy(&avx512_d[n], &zmm_s[n], sizeof(U512));
              }
              dst->header.xstate_bv |= X64_XStateComponentFlag_ZMM;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          if(process_ctx->xsave_layout.cet_u_offset)
          {
            if(process_ctx->xsave_layout.cet_u_offset + sizeof(U64) * 2 <= process_ctx->xsave_size)
            {
              U64 *cet_u = (U64 *)(xsave_raw + process_ctx->xsave_layout.cet_u_offset);
              cet_u[0] = src->cetmsr;
              cet_u[1] = src->cetssp;
              dst->header.xstate_bv |= X64_XStateComponentFlag_CETU;
            }
            else { Assert(0 && "invalid xsave size"); goto exit; }
          }
          
          // xsave
          Assert(dst->header.xcomp_bv == 0); // must always be zero
          int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_SETREGSET, thread->tid, (void *)NT_X86_XSTATE, &(struct iovec){ .iov_base = dst, .iov_len = process_ctx->xsave_size }));
          if(ptrace_result < 0) { goto exit; }
        }
        
        scratch_end(scratch);
      }
      
      // debug registers
      {
        src->dr7 |= (1 << 10);
        
        U64 *dr_s = &src->dr0;
        for EachIndex(n, 8)
        {
          if(n != 4 && n != 5)
          {
            U64 offset = OffsetOf(LNX_DMN_UserX64, u_debugreg[n]);
            int ptrace_result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_POKEUSER, thread->tid, PtrFromInt(offset), (void*)(uintptr_t)dr_s[n]));
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
lnx_dmn_set_single_step_flag(LNX_DMN_Thread *thread, B32 is_on)
{
  B32 is_flag_set = 0;
  switch(thread->process->ctx->arch)
  {
    case Arch_Null: {} break;
    case Arch_x64:
    {
      X64_RegBlock *reg_block = thread->reg_block;
      if(is_on) { reg_block->rflags |= X64_RFlag_Trap;  }
      else      { reg_block->rflags &= ~X64_RFlag_Trap; }
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
lnx_dmn_thread_ptr_list_push_node(LNX_DMN_ThreadPtrList *list, LNX_DMN_ThreadPtrNode *n)
{
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
}

internal LNX_DMN_ThreadPtrNode *
lnx_dmn_thread_ptr_list_push(Arena *arena, LNX_DMN_ThreadPtrList *list, LNX_DMN_Thread *v)
{
  LNX_DMN_ThreadPtrNode *n = push_array(arena, LNX_DMN_ThreadPtrNode, 1);
  n->v = v;
  lnx_dmn_thread_ptr_list_push_node(list, n);
  return n;
}

internal void
lnx_dmn_thread_ptr_list_remove(LNX_DMN_ThreadPtrList *list, LNX_DMN_ThreadPtrNode *n)
{
  DLLRemove(list->first, list->last, n);
  list->count -= 1;
}

internal LNX_DMN_ModulePtrNode *
lnx_dmn_module_ptr_list_push(Arena *arena, LNX_DMN_ModulePtrList *list, LNX_DMN_Module *v)
{
  LNX_DMN_ModulePtrNode *n = push_array(arena, LNX_DMN_ModulePtrNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal LNX_DMN_ProcessPtrNode *
lnx_dmn_process_ptr_list_push(Arena *arena, LNX_DMN_ProcessPtrList *list, LNX_DMN_Process *v)
{
  LNX_DMN_ProcessPtrNode *n = push_array(arena, LNX_DMN_ProcessPtrNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal void
lnx_dmn_push_event_create_process(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process)
{
  // push create process event
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind      = DMN_EventKind_CreateProcess;
  e->process   = lnx_dmn_handle_from_process(process);
  e->arch      = process->ctx->arch;
  e->code      = process->pid;
  e->tls_model = DMN_TlsModel_Gnu; // TODO: use dynamic linker path to figure out correct enum here
}

internal void
lnx_dmn_push_event_exit_process(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_ExitProcess;
  e->process = lnx_dmn_handle_from_process(process);
  e->code    = process->main_thread_exit_code;
}

internal void
lnx_dmn_push_event_create_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_CreateThread;
  e->process = lnx_dmn_handle_from_process(thread->process);
  e->thread  = lnx_dmn_handle_from_thread(thread);
  e->arch    = thread->process->ctx->arch;
  e->code    = thread->tid;
}

internal void
lnx_dmn_push_event_exit_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 exit_code)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_ExitThread;
  e->process = lnx_dmn_handle_from_process(thread->process);
  e->thread  = lnx_dmn_handle_from_thread(thread);
  e->code    = exit_code;
}

internal void
lnx_dmn_push_event_load_module(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, LNX_DMN_Module *module)
{
  // TODO: reporting this module breaks ctrl thread
  LNX_DMN_Process *process = thread->process;
  DMN_ModuleInfo *module_info = lnx_dmn_module_info_from_process_module(arena, process->pid, process->fd, module->base_vaddr, module->name_vaddr, module->is_main);
  String8 module_name = module_info->module_path;
  if(!str8_match(module_name, str8_lit("linux-vdso.so.1"), 0))
  {
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind             = DMN_EventKind_LoadModule;
    e->process          = lnx_dmn_handle_from_process(thread->process);
    e->thread           = lnx_dmn_handle_from_thread(thread);
    e->module           = lnx_dmn_handle_from_module(module);
    e->arch             = module_info->arch;
    e->address          = module->base_vaddr;
    e->size             = module->size;
    e->string           = module_name;
    e->module_info      = module_info;
  }
}

internal void
lnx_dmn_push_event_unload_module(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, LNX_DMN_Module *module)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_UnloadModule;
  e->process = lnx_dmn_handle_from_process(process);
  e->module  = lnx_dmn_handle_from_module(module);
}

internal void
lnx_dmn_push_event_handshake_complete(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind    = DMN_EventKind_HandshakeComplete;
  e->process = lnx_dmn_handle_from_process(process);
  e->thread  = lnx_dmn_handle_from_thread(process->first_thread);
  e->arch    = process->ctx->arch;
}

internal void
lnx_dmn_push_event_breakpoint(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 address)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_Breakpoint;
  e->process             = lnx_dmn_handle_from_process(thread->process);
  e->thread              = lnx_dmn_handle_from_thread(thread);
  e->instruction_pointer = address;
  e->address             = address;
}

internal void
lnx_dmn_push_event_single_step(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind                = DMN_EventKind_SingleStep;
  e->process             = lnx_dmn_handle_from_process(thread->process);
  e->thread              = lnx_dmn_handle_from_thread(thread);
  e->instruction_pointer = lnx_dmn_thread_read_ip(thread);
  e->address             = e->instruction_pointer;
}

internal void
lnx_dmn_push_event_exception(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 signo)
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
  e->process             = lnx_dmn_handle_from_process(thread->process);
  e->thread              = lnx_dmn_handle_from_thread(thread);
  e->instruction_pointer = lnx_dmn_thread_read_ip(thread);
  e->address             = e->instruction_pointer;
  e->code                = signo;
  e->exception_repeated  = signo < ArrayCount(is_repeatable) ? is_repeatable[signo] : 0;
  
  if(signo == SIGSEGV)
  {
    siginfo_t si = {0};
    LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, thread->tid, 0, &si));
    e->address = (U64)si.si_addr;
  }
}

internal void
lnx_dmn_push_event_halt(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind = DMN_EventKind_Halt;
}

internal void
lnx_dmn_push_event_not_attached(Arena *arena, DMN_EventList *events)
{
  DMN_Event *e = dmn_event_list_push(arena, events);
  e->kind       = DMN_EventKind_Error;
  e->error_kind = DMN_ErrorKind_NotAttached;
}

internal LNX_DMN_Thread *
lnx_dmn_event_create_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, pid_t tid)
{
  LNX_DMN_Thread *thread = lnx_dmn_thread_alloc(process, LNX_DMN_ThreadState_Stopped, tid);
  lnx_dmn_push_event_create_thread(arena, events, thread);
  return thread;
}

internal void
lnx_dmn_event_exit_thread(Arena *arena, DMN_EventList *events, pid_t tid, U64 exit_code)
{
  LNX_DMN_Thread  *thread  = lnx_dmn_thread_from_pid(tid);
  LNX_DMN_Process *process = thread->process;
  
  // store main thread's exit code
  if(thread->tid == thread->process->pid)
  {
    thread->process->main_thread_exit_code = exit_code;
  }
  
  // push exit event
  lnx_dmn_push_event_exit_thread(arena, events, thread, exit_code);
  
  // release entity
  lnx_dmn_thread_release(thread);
  
  // auto exit process on last thread
  if(process->thread_count == 0)
  {
    lnx_dmn_event_exit_process(arena, events, process->pid);
  }
}

internal LNX_DMN_Process *
lnx_dmn_event_create_process(Arena *arena, DMN_EventList *events, pid_t pid, LNX_DMN_Process *parent_process, LNX_DMN_CreateProcessFlags flags)
{
  LNX_DMN_Process *process = lnx_dmn_process_alloc(pid, LNX_DMN_ProcessState_Normal, parent_process, !!(flags & LNX_DMN_CreateProcessFlag_DebugSubprocesses), !!(flags & LNX_DMN_CreateProcessFlag_Cow));
  
  if(flags & LNX_DMN_CreateProcessFlag_ClonedMemory)
  {
    process->ctx = parent_process->ctx;
  }
  else
  {
    process->ctx = lnx_dmn_process_ctx_alloc(process, !!(flags & LNX_DMN_CreateProcessFlag_Rebased));
  }
  process->ctx->ref_count += 1;
  
  // install probes in a process that does not have a cloned memory
  if(!(flags & LNX_DMN_CreateProcessFlag_ClonedMemory))
  {
    lnx_dmn_process_trap_probes(process);
  }
  
  // create main thread
  lnx_dmn_thread_alloc(process, LNX_DMN_ThreadState_Stopped, pid);
  
  // push events
  lnx_dmn_push_event_create_process(arena, events, process);
  for EachNode(thread, LNX_DMN_Thread, process->first_thread)
  {
    lnx_dmn_push_event_create_thread(arena, events, thread);
  }
  for EachNode(module, LNX_DMN_Module, process->ctx->first_module)
  {
    lnx_dmn_push_event_load_module(arena, events, process->first_thread, module);
  }
  lnx_dmn_push_event_handshake_complete(arena, events, process);
  
  return process;
}

internal void
lnx_dmn_event_exit_process(Arena *arena, DMN_EventList *events, pid_t pid)
{
  LNX_DMN_Process *process = lnx_dmn_process_from_pid(pid);
  AssertAlways(process->thread_count == 0);
  
  // push module events
  for EachNode(module, LNX_DMN_Module, process->ctx->first_module)
  {
    lnx_dmn_push_event_unload_module(arena, events, process, module);
  }
  
  // push process exit event
  lnx_dmn_push_event_exit_process(arena, events, process);
  
  // release process
  lnx_dmn_process_release(process);
}

internal void
lnx_dmn_event_load_module(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 name_space_id, U64 new_link_map_vaddr)
{
  LNX_DMN_Process *process = thread->process;
  
  GNU_LinkMap64 map = {0};
  for(U64 map_vaddr = new_link_map_vaddr; map_vaddr != 0; map_vaddr = map.next_vaddr)
  {
    // read out new link map item
    if(gnu_read_link_map(lnx_dmn_machine_op_mem_read, &process->fd, map_vaddr, process->ctx->dl_class, &map) != MachineOpResult_Ok) { break; }
    
    // was module already loaded?
    LNX_DMN_Module *module = hash_table_search_u64_raw(process->ctx->loaded_modules_ht, map.addr_vaddr);
    if(module) { continue; }
    
    // clone process ctx
    if(process->is_cow)
    {
      process->is_cow = 0;
      process->ctx    = lnx_dmn_process_ctx_clone(process, process->ctx);
    }
    
    // alloc module
    module = lnx_dmn_module_alloc(process->ctx, process->fd, map.addr_vaddr, map.name_vaddr, name_space_id, 0);
    
    // push load module event
    lnx_dmn_push_event_load_module(arena, events, thread, module);
  }
}

internal void
lnx_dmn_event_unload_module(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, U64 rdebug_vaddr)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  LNX_DMN_ProcessCtx *ctx = process->ctx;
  
  // flag every module as inactive
  for EachNode(module, LNX_DMN_Module, ctx->first_module)
  {
    module->is_live = 0;
  }
  
  // mark live modules
  B32                is_64bit    = ctx->dl_class == ELF_Class_64;
  GNU_RDebugInfoList rdebug_list = gnu_parse_rdebug(scratch.arena, is_64bit, rdebug_vaddr, lnx_dmn_machine_op_mem_read, &process->fd);
  for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
  {
    GNU_LinkMapList link_map_list = gnu_parse_link_map_list(scratch.arena, is_64bit, rdebug_n->v.r_map, lnx_dmn_machine_op_mem_read, &process->fd);
    for EachNode(link_map_n, GNU_LinkMapNode, link_map_list.first)
    {
      LNX_DMN_Module *module = hash_table_search_u64_raw(ctx->loaded_modules_ht, link_map_n->v.addr_vaddr);
      module->is_live = 1;
    }
  }
  
  // collect unloaded modules
  LNX_DMN_ModulePtrList to_release = {0};
  for EachNode(module, LNX_DMN_Module, ctx->first_module)
  {
    if(!module->is_live)
    {
      lnx_dmn_module_ptr_list_push(scratch.arena, &to_release, module);
    }
  }
  
  // clone process context
  if(to_release.count > 0)
  {
    if(process->is_cow)
    {
      process->is_cow = 0;
      process->ctx    = lnx_dmn_process_ctx_clone(process, process->ctx);
    }
  }
  
  // push events and clean up unloaded modules
  for EachNode(module_n, LNX_DMN_ModulePtrNode, to_release.first)
  {
    lnx_dmn_push_event_unload_module(arena, events, process, module_n->v);
    lnx_dmn_module_release(process->ctx, module_n->v);
  }
  
  scratch_end(scratch);
}

internal void
lnx_dmn_event_breakpoint(Arena *arena, DMN_EventList *events, LNX_DMN_ActiveTrap *user_traps, pid_t tid)
{
  LNX_DMN_Thread  *thread  = lnx_dmn_thread_from_pid(tid);
  LNX_DMN_Process *process = thread->process;
  U64              ip      = lnx_dmn_thread_read_ip(thread);
  
  // is this user trap?
  LNX_DMN_ActiveTrap *hit_user_trap = 0;
  {
    DMN_Handle process_handle = lnx_dmn_handle_from_process(process);
    for EachNode(active_trap, LNX_DMN_ActiveTrap, user_traps)
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
  LNX_DMN_ProbeType probe_type = LNX_DMN_ProbeType_Null;
  if(hit_user_trap == 0)
  {
    for EachNode(active_trap, LNX_DMN_ActiveTrap, process->ctx->first_probe_trap)
    {
      if(active_trap->trap->vaddr == ip-1)
      {
        probe_type = active_trap->trap->id;
        break;
      }
    }
  }
  
  if(probe_type == LNX_DMN_ProbeType_InitComplete)
  {
    B32 is_init_completed = 0;
    
    LNX_DMN_Probe *probe = process->ctx->probes[LNX_DMN_ProbeType_InitComplete];
    U64 name_space_id = 0, rdebug_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &name_space_id)) { goto init_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &rdebug_addr))   { goto init_complete_exit; }
    
    GNU_RDebugInfo64 rdebug = {0};
    if(gnu_read_r_debug(lnx_dmn_machine_op_mem_read, &process->fd, rdebug_addr, process->ctx->arch, &rdebug) != MachineOpResult_Ok) { goto init_complete_exit; }
    if(rdebug.r_version < 1) { goto init_complete_exit; }
    
    lnx_dmn_event_load_module(arena, events, thread, name_space_id, rdebug.r_map);
    
    is_init_completed = 1;
    init_complete_exit:;
    AssertAlways(is_init_completed);
  }
  else if(probe_type == LNX_DMN_ProbeType_RelocComplete)
  {
    B32 is_reloc_completed = 0;
    
    LNX_DMN_Probe *probe = process->ctx->probes[LNX_DMN_ProbeType_RelocComplete];
    U64 name_space_id = 0, new_link_map_addr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &name_space_id))     { goto reloc_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[2], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &new_link_map_addr)) { goto reloc_complete_exit; }
    
    lnx_dmn_event_load_module(arena, events, thread, name_space_id, new_link_map_addr);
    
    is_reloc_completed = 1;
    reloc_complete_exit:;
    AssertAlways(is_reloc_completed);
  }
  else if(probe_type == LNX_DMN_ProbeType_UnmapComplete)
  {
    B32 is_unmap_completed = 0;
    
    LNX_DMN_Probe *probe = process->ctx->probes[LNX_DMN_ProbeType_UnmapComplete];
    U64 name_space_id = 0, rdebug_vaddr = 0;
    if(!stap_read_arg_u(probe->args.v[0], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &name_space_id)) { goto unmap_complete_exit; }
    if(!stap_read_arg_u(probe->args.v[1], process->ctx->arch, thread->reg_block, lnx_dmn_stap_memory_read, process, &rdebug_vaddr))  { goto unmap_complete_exit; }
    
    lnx_dmn_event_unload_module(arena, events, process, rdebug_vaddr);
    
    is_unmap_completed = 1;
    unmap_complete_exit:;
    AssertAlways(is_unmap_completed);
  }
  
  if(probe_type == LNX_DMN_ProbeType_Null)
  {
    // rollback IP on user traps
    if(hit_user_trap)
    {
      U64 ip = lnx_dmn_thread_read_ip(thread);
      lnx_dmn_thread_write_ip(thread, ip - 1);
    }
    
    DMN_Event *e = dmn_event_list_push(arena, events);
    e->kind                = DMN_EventKind_Breakpoint;
    e->process             = lnx_dmn_handle_from_process(process);
    e->thread              = lnx_dmn_handle_from_thread(thread);
    e->instruction_pointer = lnx_dmn_thread_read_ip(thread);
  }
}

internal void
lnx_dmn_event_data_breakpoint(Arena *arena, DMN_EventList *events, pid_t tid)
{
  LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(tid);
  
  B32 is_valid = 1;
  U64 address  = 0;
  switch(thread->process->ctx->arch)
  {
    default:{}break;
    case Arch_x64:
    {
      X64_RegBlock *regs_x64 = thread->reg_block;
      if(regs_x64->dr6 & X64_DebugStatusFlag_B0)
      {
        address = regs_x64->dr0;
      }
      else if(regs_x64->dr6 & X64_DebugStatusFlag_B1)
      {
        address = regs_x64->dr1;
      }
      else if(regs_x64->dr6 & X64_DebugStatusFlag_B2)
      {
        address = regs_x64->dr2;
      }
      else if(regs_x64->dr6 & X64_DebugStatusFlag_B3)
      {
        address = regs_x64->dr3;
      }
      else
      {
        is_valid = 0;
      }
    }break;
  }
  
  if(is_valid)
  {
    lnx_dmn_push_event_breakpoint(arena, events, thread, address);
  }
}

internal void
lnx_dmn_event_halt(Arena *arena, DMN_EventList *events)
{
  lnx_dmn_push_event_halt(arena, events);
}

internal void
lnx_dmn_event_single_step(Arena *arena, DMN_EventList *events, pid_t tid)
{
  LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(tid);
  
  // clear single step flag
  lnx_dmn_set_single_step_flag(thread, 0);
  
  // push event
  lnx_dmn_push_event_single_step(arena, events, thread);
}

internal void
lnx_dmn_event_exception(Arena *arena, DMN_EventList *events, pid_t tid, U64 signo)
{
  LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(tid);
  
  thread->pass_through_signal = 1;
  thread->pass_through_signo  = signo;
  
  lnx_dmn_push_event_exception(arena, events, thread, signo);
}

internal LNX_DMN_Process *
lnx_dmn_event_attach(Arena *arena, DMN_EventList *events, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // create process
  LNX_DMN_Process *process = lnx_dmn_event_create_process(arena, events, pid, 0, LNX_DMN_CreateProcessFlag_DebugSubprocesses|LNX_DMN_CreateProcessFlag_Rebased);
  
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
        lnx_dmn_event_create_thread(arena, events, process, tid);
      }
      
      LNX_RETRY_ON_EINTR(closedir(task_dirp));
    }
  }
  
  // extract modules from r_debug
  {
    B32                is_64bit      = process->ctx->dl_class == ELF_Class_64;
    GNU_RDebugInfoList rdebug_list   = gnu_parse_rdebug(scratch.arena, is_64bit, process->ctx->rdebug_vaddr, lnx_dmn_machine_op_mem_read, &process->fd);
    U64                name_space_id = 0;
    for EachNode(rdebug_n, GNU_RDebugInfoNode, rdebug_list.first)
    {
      lnx_dmn_event_load_module(arena, events, process->first_thread, name_space_id, rdebug_n->v.r_map);
      name_space_id += 1;
    }
  }
  
  // handshake complete
  lnx_dmn_push_event_handshake_complete(arena, events, process);
  
  scratch_end(scratch);
  return process;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  if(lnx_dmn_state == 0)
  {
    local_persist LNX_DMN_State state;
    lnx_dmn_state = &state;
    
    lnx_dmn_state->arena          = arena_alloc();
    lnx_dmn_state->access_mutex   = mutex_alloc();
    lnx_dmn_state->entities_arena = arena_alloc(.reserve_size = GB(32), .commit_size = KB(64), .flags = ArenaFlag_NoChain);
    lnx_dmn_state->entities_base  = push_array(lnx_dmn_state->entities_arena, LNX_DMN_Entity, 0);
    lnx_dmn_state->tid_ht         = hash_table_init(lnx_dmn_state->arena, 0x2000);
    lnx_dmn_state->pid_ht         = hash_table_init(lnx_dmn_state->arena, 0x400);
    lnx_dmn_state->halter_mutex   = mutex_alloc();
    lnx_dmn_entity_alloc(LNX_DMN_EntityKind_Null);
    
    // find offsets of TLS index and TLS offset in the link_map struct
    // 
    // TODO: assuming that target is using same libc version as debugger
    {
      LNX_DMN_DbDesc *tls_modid_desc  = dlsym(RTLD_DEFAULT, "_thread_db_link_map_l_tls_modid");
      LNX_DMN_DbDesc *tls_offset_desc = dlsym(RTLD_DEFAULT, "_thread_db_link_map_l_tls_offset");
      if(tls_modid_desc && tls_offset_desc)
      {
        if(tls_modid_desc->bit_size <= 64 && tls_offset_desc->bit_size <= 64)
        {
          lnx_dmn_state->tls_modid_desc  = *tls_modid_desc;
          lnx_dmn_state->tls_offset_desc = *tls_offset_desc;
          lnx_dmn_state->is_tls_detected = 1;
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
  lnx_dmn_ctrl_thread = 1;
  return ctx;
}

internal void
dmn_ctrl_exclusive_access_begin(void)
{
  MutexScope(lnx_dmn_state->access_mutex)
  {
    lnx_dmn_state->access_run_state = 1;
  }
}

internal void
dmn_ctrl_exclusive_access_end(void)
{
  MutexScope(lnx_dmn_state->access_mutex)
  {
    lnx_dmn_state->access_run_state = 0;
  }
}

internal U32
dmn_ctrl_launch(DMN_CtrlCtx *ctx, ProcessLaunchParams *params)
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
  U64    envc = lnx_state.default_env_count + params->env.node_count + 1;
  char **envp = push_array(scratch.arena, char *, envc);
  {
    // copy default environment
    MemoryCopyTyped(envp, lnx_state.default_env, lnx_state.default_env_count);
    
    // copy user environment
    U64 idx = lnx_state.default_env_count;
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
    if(LNX_RETRY_ON_EINTR(raise(SIGSTOP)) < 0) { goto child_exit; }
    
    // change work directory to tracee
    if(LNX_RETRY_ON_EINTR(chdir(work_dir_path)) < 0) { goto child_exit; }
    
    // replace process with target program
    if(LNX_RETRY_ON_EINTR(execve(argv[0], argv, envp)) < 0) { goto child_exit; }
    
    child_exit:;
    exit(0);
  }
  // parent process
  else if(pid > 0)
  {
    // try to seize child process
    if(lnx_dmn_ptrace_seize(pid) >= 0)
    {
      // tracee was successfully seized, create process
      lnx_dmn_process_alloc(pid, LNX_DMN_ProcessState_Launch, 0, params->debug_subprocesses, 0);
    }
    else
    {
      Assert(0 && "failed to ptrace child process");
      LNX_RETRY_ON_EINTR(kill(SIGKILL, pid));
      LNX_RETRY_ON_EINTR(waitpid(pid, 0, WNOHANG));
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
  if(lnx_dmn_ptrace_seize(pid) >= 0)
  {
    if(LNX_RETRY_ON_EINTR(kill(pid, SIGSTOP)) >= 0)
    {
      lnx_dmn_process_alloc(pid, LNX_DMN_ProcessState_Attach, 0, 1, 0);
      is_attached = 1;
    }
    else
    {
      LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, pid, 0, 0));
    }
  }
  return is_attached;
}

internal B32
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process_handle, U32 exit_code)
{
  B32 result = 0;
  mutex_take(lnx_dmn_state->halter_mutex);
  LNX_DMN_Process *process = lnx_dmn_process_from_handle(process_handle);
  if(process)
  {
    result = LNX_RETRY_ON_EINTR(kill(process->pid, SIGKILL)) >= 0;
  }
  mutex_drop(lnx_dmn_state->halter_mutex);
  return result;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process_handle)
{
  B32 result = 0;
  mutex_take(lnx_dmn_state->halter_mutex);
  LNX_DMN_Process *process = lnx_dmn_process_from_handle(process_handle);
  if(process)
  {
    result = LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, process->pid, 0, 0)) >= 0;
  }
  mutex_drop(lnx_dmn_state->halter_mutex);
  return result;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  Temp scratch = scratch_begin(&arena, 1);
  DMN_EventList events = {0};
  
  mutex_take(lnx_dmn_state->halter_mutex);
  
  // wait for signals from the running threads
  if(lnx_dmn_state->process_count > 0)
  {
    // write traps to memory
    LNX_DMN_ActiveTrap *active_trap_first = 0, *active_trap_last = 0;
    {
      HashTable *process_ht = hash_table_init(scratch.arena, lnx_dmn_state->process_count);
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
          LNX_DMN_ActiveTrap *is_set = hash_table_search_u64_raw(active_trap_ht, trap->vaddr);
          if(is_set) { continue; }
          
          // TODO: ctrl sends down traps for exited process
          LNX_DMN_Process *process = lnx_dmn_process_from_handle(trap->process);
          if(!process) { continue; }
          
          // trap instruction
          LNX_DMN_ActiveTrap *active_trap = lnx_dmn_set_trap(scratch.arena, trap);
          
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
      LNX_DMN_Thread *single_step_thread = lnx_dmn_thread_from_handle(ctrls->single_step_thread);
      if(single_step_thread)
      {
        lnx_dmn_set_single_step_flag(single_step_thread, 1);
      }
      else
      {
        Assert(0 && "invalid single_step_thread handle");
      }
    }
    
    // schedule threads to run
    LNX_DMN_ThreadPtrList running_threads = {0};
    {
      for EachNode(process, LNX_DMN_Process, lnx_dmn_state->first_process)
      {
        //- rjf: determine if this process is frozen
        B32 process_is_frozen = 0;
        if(ctrls->run_entities_are_processes)
        {
          for EachIndex(idx, ctrls->run_entity_count)
          {
            if(dmn_handle_match(ctrls->run_entities[idx], lnx_dmn_handle_from_process(process)))
            {
              process_is_frozen = 1;
              break;
            }
          }
        }
        
        for EachNode(thread, LNX_DMN_Thread, process->first_thread)
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
                if(dmn_handle_match(ctrls->run_entities[idx], lnx_dmn_handle_from_thread(thread)))
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
            is_frozen = !dmn_handle_match(lnx_dmn_handle_from_thread(thread), ctrls->single_step_thread);
          }
          
          // resume thread
          if(!is_frozen)
          {
            AssertAlways(thread->state == LNX_DMN_ThreadState_Stopped);
            
            // write registers
            if(thread->is_reg_block_dirty)
            {
              thread->is_reg_block_dirty = !lnx_dmn_thread_write_reg_block(thread);
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
            if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, thread->tid, 0, sig_code)) >= 0)
            {
              thread->state              = LNX_DMN_ThreadState_Running;
              thread->is_reg_block_dirty = 1;
              lnx_dmn_thread_ptr_list_push(scratch.arena, &running_threads, thread);
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
    for EachNode(n, LNX_DMN_ThreadPtrNode, running_threads.first)
    {
      hash_table_push_u64_raw(scratch.arena, running_threads_ht, n->v->tid, n);
    }
    
    B32                   is_halt_done    = 0;
    LNX_DMN_ThreadPtrList stopped_threads = {0};
    do
    {
      // wait for a signal
      int   status  = 0;
      pid_t wait_id = 0; 
      {
        mutex_drop(lnx_dmn_state->halter_mutex);
        for(;;)
        {
          wait_id = LNX_RETRY_ON_EINTR(waitpid(-1, &status, __WALL|__WNOTHREAD));
          if(wait_id == -1) { InvalidPath; } // TODO: graceful exit
          break;
        }
        mutex_take(lnx_dmn_state->halter_mutex);
      }
      
      // unpack status
      int wifexited   = WIFEXITED(status);
      int wifsignaled = WIFSIGNALED(status);
      int wifstopped  = WIFSTOPPED(status);
      int wstopsig    = WSTOPSIG(status);
      int event_code  = (status >> 16);
      
      // intercept initing processes
      {
        LNX_DMN_Process *process = lnx_dmn_process_from_pid(wait_id);
        
        if(process && process->state != LNX_DMN_ProcessState_Normal)
        {
          switch(process->state)
          {
            case LNX_DMN_ProcessState_Null:
            case LNX_DMN_ProcessState_Normal:
            {
              InvalidPath;
            } break;
            case LNX_DMN_ProcessState_Launch:
            {
              if(wstopsig == SIGSTOP)
              {
                if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_CONT, process->pid, 0, 0)) >= 0)
                {
                  process->state = LNX_DMN_ProcessState_WaitForExec;
                  goto wait_for_signal;
                }
                else { Assert(0 && "failed to resume tracee"); }
              }
              // else { Assert(0 && "unexpected signal"); }
            } break;
            case LNX_DMN_ProcessState_WaitForExec:
            {
              if(wstopsig == SIGTRAP && event_code == PTRACE_EVENT_EXEC)
              {
                LNX_DMN_CreateProcessFlags create_flags = process->debug_subprocesses ? LNX_DMN_CreateProcessFlag_DebugSubprocesses : 0;
                lnx_dmn_process_release(process);
                lnx_dmn_event_create_process(arena, &events, wait_id, 0, create_flags);
                goto wait_for_signal;
              }
              //else { Assert(0 && "unexpected signal"); }
            } break;
            case LNX_DMN_ProcessState_ExecFailedDoExit:
            {
              if(wifexited)
              {
                lnx_dmn_process_release(process);
                goto wait_for_signal;
              }
              else { Assert(0 && "unexpected signal"); }
            } break;
            case LNX_DMN_ProcessState_Attach:
            {
              if(wstopsig == SIGSTOP)
              {
                LNX_DMN_CreateProcessFlags create_flags = process->debug_subprocesses ? LNX_DMN_CreateProcessFlag_DebugSubprocesses : 0;
                lnx_dmn_process_release(process);
                lnx_dmn_event_create_process(arena, &events, wait_id, 0, create_flags);
                goto wait_for_signal;
              }
              else { Assert(0 && "unexpected signal"); }
            } break;
          }
          
          // shutdown process
          {
            if(LNX_RETRY_ON_EINTR(kill(wait_id, SIGKILL)) >= 0)
            {
              process->state = LNX_DMN_ProcessState_ExecFailedDoExit;
            }
            else
            {
              //Assert(0 && "failed to send kill signal to the tracee");
              lnx_dmn_process_release(process);
            }
          }
          
          wait_for_signal:;
          continue;
        }
      }
      
      if(wifstopped || wifsignaled || wifexited)
      {
        LNX_DMN_ThreadPtrNode *thread_n = hash_table_search_u64_raw(running_threads_ht, wait_id);
        if(thread_n)
        {
          LNX_DMN_Thread *thread = thread_n->v;
          AssertAlways(thread->state == LNX_DMN_ThreadState_Running);
          
          // remove mapping
          hash_table_purge_u64(running_threads_ht, thread->tid);
          
          // move thread to the stopped list
          lnx_dmn_thread_ptr_list_remove(&running_threads, thread_n);
          lnx_dmn_thread_ptr_list_push_node(&stopped_threads, thread_n);
          
          // update thread state
          if(wifstopped && !wifsignaled && !wifexited)
          {
            thread->state = LNX_DMN_ThreadState_Stopped;
          }
          else if(!wifstopped && (wifsignaled || wifexited))
          {
            thread->state = LNX_DMN_ThreadState_Exited;
          }
          else { InvalidPath; }
          
          // stop all other threads
          if(stopped_threads.count == 1)
          {
            for EachNode(n, LNX_DMN_ThreadPtrNode, running_threads.first)
            {
              if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_INTERRUPT, n->v->tid, 0, 0)) < 0) { Assert(0 && "failed to interrupt process"); }
            }
          }
        }
      }
      
      // normal child exit via _exit or exit()
      if(wifexited)
      {
        lnx_dmn_event_exit_thread(arena, &events, wait_id, WEXITSTATUS(status));
      }
      
      // exit because child did not handle a signal
      else if(wifsignaled)
      {
        lnx_dmn_event_exit_thread(arena, &events, wait_id, WTERMSIG(status));
      }
      // thread or group stop
      else if(wifstopped)
      {
        // read thread registers
        {
          LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(wait_id);
          if(thread && thread->state != LNX_DMN_ThreadState_PendingCreation)
          {
            thread->is_reg_block_dirty = !lnx_dmn_thread_read_reg_block(thread);
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
              if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, wait_id, 0, &siginfo)) >= 0)
              {
                switch(siginfo.si_code)
                {
                  case SI_KERNEL:   { lnx_dmn_event_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                  case TRAP_BRKPT:  { lnx_dmn_event_breakpoint(arena, &events, active_trap_first, wait_id); } break;
                  case TRAP_TRACE:  { lnx_dmn_event_single_step(arena, &events, wait_id);                   } break;
                  case TRAP_HWBKPT: { lnx_dmn_event_data_breakpoint(arena, &events, wait_id);               } break;
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
              if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &new_tid)) >= 0)
              {
                LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(wait_id);
                
                // create a new partially inited thread
                lnx_dmn_thread_alloc(thread->process, LNX_DMN_ThreadState_PendingCreation, new_tid);
              }
              else { Assert(0 && "failed to get new tid"); }
            }break;
            case PTRACE_EVENT_EXEC:
            {
              LNX_DMN_Thread  *thread  = lnx_dmn_thread_from_pid(wait_id);
              LNX_DMN_Process *process = thread->process;
              if(process->debug_subprocesses)
              {
                lnx_dmn_event_exit_thread(arena, &events, wait_id, 0);
                lnx_dmn_event_create_process(arena, &events, wait_id, 0, LNX_DMN_CreateProcessFlag_DebugSubprocesses);
              }
              else
              {
                if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_DETACH, wait_id, 0, 0)) >= 0)
                {
                  lnx_dmn_event_exit_thread(arena, &events, wait_id, 0);
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
              LNX_DMN_Thread *thread = lnx_dmn_thread_from_pid(wait_id);
              if(thread->state == LNX_DMN_ThreadState_PendingCreation)
              {
                LNX_DMN_Process *process = thread->process;
                lnx_dmn_thread_release(thread);
                thread = lnx_dmn_event_create_thread(arena, &events, process, wait_id);
              }
              else
              {
                AssertAlways(thread->state == LNX_DMN_ThreadState_Stopped);
              }
            }break;
            default: { Assert(0 && "unexpected ptrace code"); } break;
          }
        }
        else if(wstopsig == SIGSTOP)
        {
          siginfo_t siginfo = {0};
          if(LNX_RETRY_ON_EINTR(ptrace(PTRACE_GETSIGINFO, wait_id, 0, &siginfo)) >= 0)
          {
            // finish halting if this is our SIGSTOP
            if(siginfo.si_pid == lnx_dmn_state->halter_tid)
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
          lnx_dmn_event_exception(arena, &events, wait_id, wstopsig);
        }
      }
      else { Assert(0 && "unexpected stop code"); }
    } while(running_threads.count > 0 || lnx_dmn_state->process_pending_creation > 0 || lnx_dmn_state->threads_pending_creation > 0);
    
    // finalize halter state
    if(is_halt_done)
    {
      // push event
      lnx_dmn_event_halt(arena, &events);
      
      // reset state
      lnx_dmn_state->halter_tid     = 0;
      lnx_dmn_state->halt_code      = 0;
      lnx_dmn_state->halt_user_data = 0;
      lnx_dmn_state->is_halting     = 0;
    }
    
    // restore original instruction bytes
    for EachNode(active_trap, LNX_DMN_ActiveTrap, active_trap_first)
    {
      // skip process that exited during the wait
      LNX_DMN_Process *process = lnx_dmn_process_from_handle(active_trap->trap->process);
      if(!process) { continue; }
      
      if(!dmn_process_write(active_trap->trap->process, r1u64(active_trap->trap->vaddr, active_trap->trap->vaddr + active_trap->swap_bytes.size), active_trap->swap_bytes.str) &&
         active_trap->good)
      {
        // TODO(rjf): log an error here? - we wrote the trap successfully, but we did not remove it successfully,
        // implying a change in address mapping. this is maybe not even a bug, since the address is simply now
        // unmapped.
      }
    }
  }
  
  if(events.count == 0 && lnx_dmn_state->process_count == 0)
  {
    lnx_dmn_push_event_not_attached(arena, &events);
  }
  
  mutex_drop(lnx_dmn_state->halter_mutex);
  scratch_end(scratch);
  return events;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
  mutex_take(lnx_dmn_state->halter_mutex);
  if(lnx_dmn_state->process_count)
  {
    lnx_dmn_state->halter_tid     = gettid();
    lnx_dmn_state->halt_code      = code;
    lnx_dmn_state->halt_user_data = user_data;
    for EachNode(process, LNX_DMN_Process, lnx_dmn_state->first_process)
    {
      if(LNX_RETRY_ON_EINTR(kill(process->pid, SIGSTOP)) < 0) { Assert(0 && "failed to send SIGSTOP"); }
    }
  }
  mutex_drop(lnx_dmn_state->halter_mutex);
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Introspection Functions (Implemented Per-OS)

//- rjf: non-blocking-control-thread access barriers

internal B32
dmn_access_open(void)
{
  B32 result = 0;
  if(lnx_dmn_ctrl_thread)
  {
    result = 1;
  }
  else
  {
    mutex_take(lnx_dmn_state->access_mutex);
    result = !lnx_dmn_state->access_run_state;
  }
  return result;
}

internal void
dmn_access_close(void)
{
  if(!lnx_dmn_ctrl_thread)
  {
    mutex_drop(lnx_dmn_state->access_mutex);
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
dmn_process_memory_protect(DMN_Handle process, U64 vaddr, U64 size, AccessFlags flags)
{
}

internal U64
dmn_process_read(DMN_Handle process_handle, Rng1U64 range, void *dst)
{
  LNX_DMN_Process *process = lnx_dmn_process_from_handle(process_handle);
  U64 result = 0;
  if(process)
  {
    result = lnx_dmn_read(process->fd, range, dst);
  }
  return result;
}

internal B32
dmn_process_write(DMN_Handle process_handle, Rng1U64 range, void *src)
{
  LNX_DMN_Process *process = lnx_dmn_process_from_handle(process_handle);
  B32 result = 0;
  if(process)
  {
    result = lnx_dmn_write(process->fd, range, src);
  }
  return result;
}

//- rjf: threads

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
    LNX_DMN_Thread *thread = lnx_dmn_thread_from_handle(thread_handle);
    if(thread)
    {
      switch(thread->process->ctx->arch)
      {
        case Arch_Null: {} break;
        case Arch_x64:
        {
          X64_RegBlock *reg_block   = thread->reg_block;
          U64               dtv_pointer = 0;
          if(lnx_dmn_read_struct(thread->process->fd, reg_block->fsbase + 8, &dtv_pointer))
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
    LNX_DMN_Thread *thread = lnx_dmn_thread_from_handle(thread_handle);
    if(thread)
    {
      ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
      U64 reg_block_size = arch_info->reg_block_size;
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
    LNX_DMN_Thread *thread = lnx_dmn_thread_from_handle(thread_handle);
    if(thread)
    {
      ARCH_Info *arch_info = arch_info_from_arch(thread->process->ctx->arch);
      U64 reg_block_size = arch_info->reg_block_size;
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
    String8 name = lnx_dmn_exe_path_from_pid(arena, pid);
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
