// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO(allen): run controls: ignore_previous_exception

////////////////////////////////
//~ allen: Elf Parsing Code

#include "syms/syms_elf_inc.c"

////////////////////////////////
//~ rjf: Globals

global B32 demon_lnx_already_has_halt_injection = false;
global U64 demon_lnx_halt_code = 0;
global U64 demon_lnx_halt_user_data = 0;

global B32 demon_lnx_new_process_pending = false;

global Arena *demon_lnx_event_arena = 0;
global DEMON_EventList demon_lnx_queued_events = {0};

global U32 demon_lnx_ptrace_options = (PTRACE_O_TRACEEXIT|
                                       PTRACE_O_EXITKILL|
                                       PTRACE_O_TRACEFORK|
                                       PTRACE_O_TRACEVFORK|
                                       PTRACE_O_TRACECLONE);

////////////////////////////////
//~ rjf: Helpers

internal DEMON_LNX_ThreadExt*
demon_lnx_thread_ext(DEMON_Entity *entity){
  DEMON_LNX_ThreadExt *result = (DEMON_LNX_ThreadExt*)&entity->ext;
  return(result);
}

internal B32
demon_lnx_attach_pid(Arena *arena, pid_t pid, DEMON_LNX_AttachNode **new_node){
  B32 result = false;
  
  int attach_result = ptrace(PTRACE_ATTACH, pid, 0, 0);
  if (attach_result == -1){
    // TODO(allen): attach denied
  }
  else{
    // return a new attachment node as soon as the ptrace exists. we use these nodes
    // for cleanup on failure *and* for initializing on success. either way we need
    // to see all new attachments whether or not they fully initialized correctly.
    DEMON_LNX_AttachNode *proc_attachment = push_array_no_zero(arena, DEMON_LNX_AttachNode, 1);
    proc_attachment->next = 0;
    proc_attachment->pid = pid;
    *new_node = proc_attachment;
    
    int status = 0;
    pid_t wait_id = waitpid(pid, &status, __WALL);
    // NOTE(allen): if wait_id != pid we don't know what that means; study that case before
    // deciding how error handling around it works.
    if (wait_id == pid){
      int setoptions_result = ptrace(PTRACE_SETOPTIONS, pid, 0, PtrFromInt(demon_lnx_ptrace_options));
      if (setoptions_result == -1){
        // TODO(allen): setup failed
      }
      else{
        result = true;
      }
    }
  }
  
  return(result);
}

internal String8
demon_lnx_executable_path_from_pid(Arena *arena, pid_t pid){
  // get symbolic path
  Temp scratch = scratch_begin(&arena, 1);
  String8 exe_symbol_path = push_str8f(scratch.arena, "/proc/%d/exe", pid);
  
  // try to read the link for a bit
  Temp restore_point = temp_begin(arena);
  B32 got_final_result = false;
  U8 *buffer = 0;
  int size = 0;
  S64 cap = PATH_MAX;
  for (S64 r = 0; r < 4; cap *= 2, r += 1){
    temp_end(restore_point);
    buffer = push_array_no_zero(arena, U8, cap);
    size = readlink((char*)exe_symbol_path.str, (char*)buffer, cap);
    if (size < cap){
      got_final_result = true;
      break;
    }
  }
  
  // finalize result
  String8 result = {0};
  if (!got_final_result || size == -1){
    temp_end(restore_point);
  }
  else{
    arena_pop(arena, (cap - size - 1));
    result = str8(buffer, size + 1);
  }
  
  scratch_end(scratch);
  return(result);
}

internal int
demon_lnx_open_memory_fd_for_pid(pid_t pid){
  Temp scratch = scratch_begin(0, 0);
  String8 memory_path = push_str8f(scratch.arena, "/proc/%i/mem", pid);
  int result = open((char*)memory_path.str, O_RDWR|O_CLOEXEC);
  scratch_end(scratch);
  return(result);
}

internal Arch
demon_lnx_arch_from_pid(pid_t pid){
  Temp scratch = scratch_begin(0, 0);
  Arch result = Arch_Null;
  
  // exe path
  String8 exe_path = demon_lnx_executable_path_from_pid(scratch.arena, pid);
  
  // handle to exe
  int exe_fd = -1;
  if (exe_path.size != 0){
    exe_fd = open((char*)exe_path.str, O_RDONLY|O_CLOEXEC);
  }
  
  // elf identification
  B32 is_elf = false;
  U8 e_ident[SYMS_ElfIdentifier_NIDENT] = {0};
  if (exe_fd >= 0){
    if (pread(exe_fd, e_ident, sizeof(e_ident), 0) == sizeof(e_ident)){
      is_elf = (e_ident[SYMS_ElfIdentifier_MAG0] == 0x7f && 
                e_ident[SYMS_ElfIdentifier_MAG1] == 'E'  &&
                e_ident[SYMS_ElfIdentifier_MAG2] == 'L'  && 
                e_ident[SYMS_ElfIdentifier_MAG3] == 'F');
    }
  }
  
  // elf class
  U8 elf_class = 0;
  if (is_elf){
    elf_class = e_ident[SYMS_ElfIdentifier_CLASS];
  }
  
  // exe header data
  SYMS_ElfEhdr64 ehdr = {0};
  switch (elf_class){
    case 1:
    {
      SYMS_ElfEhdr32 ehdr32 = {0};
      if (pread(exe_fd, &ehdr32, sizeof(ehdr32), 0) == sizeof(ehdr32)){
        ehdr = syms_elf_ehdr64_from_ehdr32(ehdr32);
      }
    }break;
    
    case 2:
    {
      pread(exe_fd, &ehdr, sizeof(ehdr), 0);
    }break;
  }
  
  // determine machine type
  switch (ehdr.e_machine){
    case SYMS_ElfMachineKind_386:
    {
      result = Arch_x86;
    }break;
    
    case SYMS_ElfMachineKind_ARM:
    {
      result = Arch_arm32;
    }break;
    
    case SYMS_ElfMachineKind_X86_64:
    {
      result = Arch_x64;
    }break;
    
    case SYMS_ElfMachineKind_AARCH64:
    {
      result = Arch_arm64;
    }break;
  }
  
  scratch_end(scratch);
  return(result);
}

internal DEMON_LNX_ProcessAux
demon_lnx_aux_from_pid(pid_t pid, Arch arch){
  DEMON_LNX_ProcessAux result = {0};
  B32 addr_32bit = (arch == Arch_x86 || arch == Arch_arm32);
  
  // open aux data
  Temp scratch = scratch_begin(0, 0);
  String8 auxv_symbol_path = push_str8f(scratch.arena, "/proc/%d/auxv", pid);
  int aux_fd = open((char*)auxv_symbol_path.str, O_RDONLY|O_CLOEXEC);
  
  // scan aux data
  if (aux_fd >= 0){
    for (;;){
      result.filled = true;
      
      // read next aux
      U64 type = 0;
      U64 val = 0;
      if (addr_32bit){
        SYMS_ElfAuxv32 aux;
        if (read(aux_fd, &aux, sizeof(aux)) != sizeof(aux)){
          goto brkloop;
        }
        type = aux.a_type;
        val = aux.a_val;
      }
      else{
        SYMS_ElfAuxv64 aux;
        if (read(aux_fd, &aux, sizeof(aux)) != sizeof(aux)){
          goto brkloop;
        }
        type = aux.a_type;
        val = aux.a_val;
      }
      
      // place value in result
      switch (type){
        default:break;
        case SYMS_ElfAuxType_NULL:         goto brkloop; break;
        case SYMS_ElfAuxType_PHNUM:        result.phnum  = val; break;
        case SYMS_ElfAuxType_PHENT:        result.phent  = val; break;
        case SYMS_ElfAuxType_PHDR:         result.phdr   = val; break;
        case SYMS_ElfAuxType_EXECFN:       result.execfn = val; break;
      }
    }
    brkloop:;
    
    close(aux_fd);
  }
  
  scratch_end(scratch);
  return(result);
}

internal DEMON_LNX_PhdrInfo
demon_lnx_phdr_info_from_memory(int memory_fd, B32 is_32bit, U64 phvaddr, U64 phentsize, U64 phcount){
  DEMON_LNX_PhdrInfo result = {0};
  result.range.min = max_U64;
  
  // how much phdr will we read?
  U64 phdr_size_expected = (is_32bit?sizeof(SYMS_ElfPhdr32):sizeof(SYMS_ElfPhdr64));
  U64 phdr_stride = (phentsize?phentsize:phdr_size_expected);
  U64 phdr_read_size = ClampTop(phdr_stride, phdr_size_expected);
  
  // scan table
  U64 va = phvaddr;
  for (U64 i = 0; i < phcount; i += 1, va += phdr_stride){
    
    // get type and range
    SYMS_ElfPKind p_type = 0;
    U64 p_vaddr = 0;
    U64 p_memsz = 0;
    
    if (is_32bit){
      SYMS_ElfPhdr32 phdr32 = {0};
      demon_lnx_read_memory(memory_fd, &phdr32, va, phdr_read_size);
      p_type = phdr32.p_type;
      p_vaddr = phdr32.p_vaddr;
      p_memsz = phdr32.p_memsz;
    }
    else{
      SYMS_ElfPhdr64 phdr64 = {0};
      demon_lnx_read_memory(memory_fd, &phdr64, va, phdr_read_size);
      p_type = phdr64.p_type;
      p_vaddr = phdr64.p_vaddr;
      p_memsz = phdr64.p_memsz;
    }
    
    // save useful info
    switch (p_type){
      case SYMS_ElfPKind_Dynamic:
      {
        result.dynamic = p_vaddr;
      }break;
      case SYMS_ElfPKind_Load:
      {
        U64 min = p_vaddr;
        U64 max = p_vaddr + p_memsz;
        result.range.min = Min(result.range.min, min);
        result.range.max = Max(result.range.max, max);
      }break;
    }
  }
  
  return(result);
}

internal DEMON_LNX_ModuleNode*
demon_lnx_module_list_from_process(Arena *arena, DEMON_Entity *process){
  Arch arch = (Arch)process->arch;
  B32 is_32bit = (arch == Arch_x86 || arch == Arch_arm32);
  int memory_fd = (int)process->ext_u64;
  
  // aux from pid
  DEMON_LNX_ProcessAux aux = demon_lnx_aux_from_pid((pid_t)process->id, arch);
  
  // extract info from program headers
  DEMON_LNX_PhdrInfo phdr_info = demon_lnx_phdr_info_from_memory(memory_fd, is_32bit,
                                                                 aux.phdr, aux.phent, aux.phnum);
  
  // linkmap first from memory space & dyn address
  U64 first_linkmap_va = 0;
  if (phdr_info.dynamic != 0){
    U64 off = phdr_info.dynamic;
    for (;;){
      SYMS_ElfDyn64 dyn = {0};
      if (is_32bit){
        SYMS_ElfDyn32 dyn32 = {0};
        demon_lnx_read_memory(memory_fd, &dyn32, off, sizeof(dyn32));
        dyn.tag = dyn32.tag;
        dyn.val = dyn32.val;
        off += sizeof(dyn32);
      }
      else{
        demon_lnx_read_memory(memory_fd, &dyn, off, sizeof(dyn));
        off += sizeof(dyn);
      }
      
      if (dyn.tag == SYMS_ElfDynTag_NULL){
        break;
      }
      
      if (dyn.tag == SYMS_ElfDynTag_PLTGOT){
        // True for x86 and x64
        //  vas[0] virtual address of .dynamic
        //  vas[2] callback for resolving function address of relocation and if successful jumps to it.
        // 
        // Code that sets up PLTGOT is in glibc/sysdeps/x86_64/dl_machine.h -> elf_machine_runtime_setup
        U64 vas_off = dyn.val;
        U64 vas[3] = {0};
        demon_lnx_read_memory(memory_fd, vas, vas_off, sizeof(vas));
        first_linkmap_va = vas[1];
        break;
      }
    }
  }
  
  // setup output list
  DEMON_LNX_ModuleNode *first = 0;
  DEMON_LNX_ModuleNode *last = 0;
  
  // main module
  {
    DEMON_LNX_ModuleNode *node = push_array(arena, DEMON_LNX_ModuleNode, 1);
    SLLQueuePush(first, last, node);
    node->vaddr = phdr_info.range.min;
    node->size = phdr_info.range.max - phdr_info.range.min;
    node->name = aux.execfn;
  }
  
  // iterate link maps
  if (first_linkmap_va != 0){
    U64 linkmap_va = first_linkmap_va;
    
    for (;;){
      SYMS_ElfLinkMap64 linkmap = {0};
      if (is_32bit){
        // TOOD(nick): endian awarness
        SYMS_ElfLinkMap32 linkmap32 = {0};
        demon_lnx_read_memory(memory_fd, &linkmap32, linkmap_va, sizeof(linkmap32));
        linkmap.base = linkmap32.base;
        linkmap.name = linkmap32.name;
        linkmap.ld   = linkmap32.ld;
        linkmap.next = linkmap32.next;
      }
      else{
        demon_lnx_read_memory(memory_fd, &linkmap, linkmap_va, sizeof(linkmap));
      }
      
      if (linkmap.base != 0){
        // find phdrs for this module
        SYMS_U64 phvaddr = 0;
        SYMS_U64 phentsize = 0;
        SYMS_U64 phcount = 0;
        
        if (is_32bit){
          SYMS_ElfEhdr32 ehdr = {0};
          demon_lnx_read_memory(memory_fd, &ehdr, linkmap.base, sizeof(ehdr));
          phvaddr = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount = ehdr.e_phnum;
        }
        else{
          SYMS_ElfEhdr64 ehdr = {0};
          demon_lnx_read_memory(memory_fd, &ehdr, linkmap.base, sizeof(ehdr));
          phvaddr = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount = ehdr.e_phnum;
        }
        
        // extract info from phdrs
        DEMON_LNX_PhdrInfo module_phdr_info = demon_lnx_phdr_info_from_memory(memory_fd, is_32bit,
                                                                              phvaddr, phentsize, phcount);
        
        // save module node
        DEMON_LNX_ModuleNode *node = push_array(arena, DEMON_LNX_ModuleNode, 1);
        SLLQueuePush(first, last, node);
        node->vaddr = linkmap.base;
        node->size = module_phdr_info.range.max - module_phdr_info.range.min;
        node->name = linkmap.name;
      }
      
      linkmap_va = linkmap.next;
      if (linkmap_va == 0){
        break;
      }
    }
  }
  
  return(first);
}

internal U64
demon_lnx_read_memory(int memory_fd, void *dst, U64 src, U64 size){
  U64 bytes_read = 0;
  U8 *ptr = (U8*)dst;
  U8 *opl = ptr + size;
  U64 cursor = src;
  for (;ptr < opl;){
    size_t to_read = (size_t)(opl - ptr);
    ssize_t actual_read = pread(memory_fd, ptr, to_read, cursor);
    if (actual_read == -1){
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
    bytes_read += actual_read;
  }
  return(bytes_read);
}

internal B32
demon_lnx_write_memory(int memory_fd, U64 dst, void *src, U64 size){
  B32 result = true;
  U8 *ptr = (U8*)src;
  U8 *opl = ptr + size;
  U64 cursor = dst;
  for (;ptr < opl;){
    size_t to_write = (size_t)(opl - ptr);
    ssize_t actual_write = pwrite(memory_fd, ptr, to_write, cursor);
    if (actual_write == -1){
      result = false;
      break;
    }
    ptr += actual_write;
    cursor += actual_write;
  }
  return(result);
}

internal String8
demon_lnx_read_memory_str(Arena *arena, int memory_fd, U64 address){
  // TODO(allen): this could be done better with a demon_lnx_read_memory
  // that returns a read amount instead of a success/fail.
  
  // scan piece by piece
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  U64 max_cap = 256;
  U64 cap = max_cap;
  U64 read_p = address;
  for (;;){
    U8 *block = push_array(scratch.arena, U8, cap);
    for (;cap > 0;){
      if (demon_lnx_read_memory(memory_fd, block, read_p, cap)){
        break;
      }
      cap /= 2;
    }
    read_p += cap;
    
    U64 block_opl = 0;
    for (;block_opl < cap; block_opl += 1){
      if (block[block_opl] == 0){
        break;
      }
    }
    
    if (block_opl > 0){
      str8_list_push(scratch.arena, &list, str8(block, block_opl));
    }
    
    if (block_opl < cap || cap == 0){
      break;
    }
  }
  
  // assemble results
  String8 result = str8_list_join(arena, &list, 0);
  scratch_end(scratch);
  return(result);
}

internal void
demon_lnx_regs_x64_from_usr_regs_x64(SYMS_RegX64 *dst, DEMON_LNX_UserRegsX64 *src){
  dst->rax.u64 = src->rax;
  dst->rcx.u64 = src->rcx;
  dst->rdx.u64 = src->rdx;
  dst->rbx.u64 = src->rbx;
  dst->rsp.u64 = src->rsp;
  dst->rbp.u64 = src->rbp;
  dst->rsi.u64 = src->rsi;
  dst->rdi.u64 = src->rdi;
  dst->r8.u64  = src->r8;
  dst->r9.u64  = src->r9;
  dst->r10.u64 = src->r10;
  dst->r11.u64 = src->r11;
  dst->r12.u64 = src->r12;
  dst->r13.u64 = src->r13;
  dst->r14.u64 = src->r14;
  dst->r15.u64 = src->r15;
  dst->cs.u16  = src->cs;
  dst->ds.u16  = src->ds;
  dst->es.u16  = src->es;
  dst->fs.u16  = src->fs;
  dst->gs.u16  = src->gs;
  dst->ss.u16  = src->ss;
  dst->fsbase.u64 = src->fsbase;
  dst->gsbase.u64 = src->gsbase;
  dst->rip.u64    = src->rip;
  dst->rflags.u64 = src->rflags;
}

internal void
demon_lnx_usr_regs_x64_from_regs_x64(DEMON_LNX_UserRegsX64 *dst, SYMS_RegX64 *src){
  dst->rax = src->rax.u64;
  dst->rcx = src->rcx.u64;
  dst->rdx = src->rdx.u64;
  dst->rbx = src->rbx.u64;
  dst->rsp = src->rsp.u64;
  dst->rbp = src->rbp.u64;
  dst->rsi = src->rsi.u64;
  dst->rdi = src->rdi.u64;
  dst->r8  = src->r8.u64;
  dst->r9  = src->r9.u64;
  dst->r10 = src->r10.u64;
  dst->r11 = src->r11.u64;
  dst->r12 = src->r12.u64;
  dst->r13 = src->r13.u64;
  dst->r14 = src->r14.u64;
  dst->r15 = src->r15.u64;
  dst->cs  = src->cs.u16;
  dst->ds  = src->ds.u16;
  dst->es  = src->es.u16;
  dst->fs  = src->fs.u16;
  dst->gs  = src->gs.u16;
  dst->ss  = src->ss.u16;
  dst->fsbase = src->fsbase.u64;
  dst->gsbase = src->gsbase.u64;
  dst->rip    = src->rip.u64;
  dst->rflags = src->rflags.u64;
}

////////////////////////////////

internal String8
demon_lnx_read_int_string(Arena *arena, int fd, int radix){
  String8 integer = str8(0,0);
  
  int to_read = 0;
  int to_seek = 0;
  for (;;){
    char b = 0;
    if (read(fd, &b, sizeof(b)) == 0){
      break;
    }
    to_seek += 1;
    if ( ! char_is_digit(b, radix)){
      break;
    }
    to_read += 1;
  }
  
  if (lseek(fd, -to_seek, SEEK_CUR) != -1) {
    char *buf = push_array_no_zero(arena, char, to_read + 1);
    read(fd, buf, to_read);
    buf[to_read] = '\0';
    integer = str8((U8*)buf, (U64)to_read);
  }
  
  return(integer);
}

internal U64
demon_lnx_read_u64(int fd, int radix){
  Temp scratch = scratch_begin(0, 0);
  String8 integer = demon_lnx_read_int_string(scratch.arena, fd, radix);
  U64 result = u64_from_str8(integer, radix);
  scratch_end(scratch);
  return(result);
}

internal S64
demon_lnx_read_s64(int fd, int radix){
  Temp scratch = scratch_begin(0, 0);
  String8 integer = demon_lnx_read_int_string(scratch.arena, fd, radix);
  S64 result = s64_from_str8(integer, radix);
  scratch_end(scratch);
  return(result);
}

internal B32
demon_lnx_read_expect(int fd, char expect){
  char got = 0;
  read(fd, &got, sizeof(got));
  B32 result = (got == expect);
  if (!result){
    lseek(fd, -1, SEEK_CUR);
  }
  return(result);
}

internal int
demon_lnx_read_whitespace(int fd){
  int whitespace_size = 0;
  for (;;){
    if (!demon_lnx_read_expect(fd, ' ')){
      if (!demon_lnx_read_expect(fd, '\t')){
        break;
      }
    }
    whitespace_size += 1;
  }
  return whitespace_size;
}

internal String8
demon_lnx_read_string(Arena *arena, int fd){
  String8 result = str8(0,0);
  
  int to_read = 0;
  int to_seek = 0;
  for (;;){
    char b = 0;
    if (read(fd, &b, sizeof(b)) == 0) {
      break;
    }
    to_seek += 1;
    if (b == '\0' || b == '\n'){
      break;
    }
    to_read += 1;
  }
  
  if (to_seek > 0 && lseek(fd, -to_seek, SEEK_CUR) != -1){
    char *buf = push_array_no_zero(arena, char, to_read + 1);
    read(fd, buf, to_read);
    buf[to_read] = '\0';
    result = str8((U8*)buf, to_read);
  }
  
  return(result);
}

internal int
demon_lnx_open_maps(pid_t pid){
  Temp scratch = scratch_begin(0, 0);
  String8 path = push_str8f(scratch.arena, "/proc/%d/maps", pid);
  int maps = open((char*)path.str, O_RDONLY|O_CLOEXEC);
  scratch_end(scratch);
  return(maps);
}

internal B32
demon_lnx_next_map(Arena *arena, int maps, DEMON_LNX_MapsEntry *entry_out){
  B32 is_parsed = false;
  MemoryZeroStruct(entry_out);
  do{
    U64 address_lo = 0;
    U64 address_hi = 0;
    DEMON_LNX_PermFlags perms = 0;
    U64 offset = 0;
    U64 dev_major = 0;
    U64 dev_minor = 0;
    U64 inode = 0;
    String8 pathname = str8(0,0);
    
    // address range
    address_lo = demon_lnx_read_u64(maps, 16);
    if (!demon_lnx_read_expect(maps, '-')){
      break;
    }
    address_hi = demon_lnx_read_u64(maps, 16);
    if (demon_lnx_read_whitespace(maps) == 0){
      break;
    }
    
    // permission flags
    char b;
    if (read(maps, &b, sizeof(b)) == 0){
      break;
    }
    if (b=='r'){
      perms |= DEMON_LNX_PermFlags_Read; 
    }
    if (read(maps, &b, sizeof(b)) == 0){
      break;
    }
    if (b=='w'){
      perms |= DEMON_LNX_PermFlags_Write;
    }
    if (read(maps, &b, sizeof(b)) == 0){
      break;
    }
    if (b=='x'){
      perms |= DEMON_LNX_PermFlags_Exec;
    }
    if (read(maps, &b, sizeof(b)) == 0){
      break;
    }
    if (b == 'p'){
      perms |= DEMON_LNX_PermFlags_Private;
    }
    if (demon_lnx_read_whitespace(maps) == 0){
      break;
    }
    
    // offset
    offset = demon_lnx_read_u64(maps, 16);
    if (demon_lnx_read_whitespace(maps) == 0){
      break;
    }
    
    // dev
    dev_major = demon_lnx_read_u64(maps, 10);
    if (!demon_lnx_read_expect(maps, ':')){
      break;
    }
    dev_minor = demon_lnx_read_u64(maps, 10);
    if (demon_lnx_read_whitespace(maps) == 0){
      break;
    }
    
    // inode
    inode = demon_lnx_read_u64(maps, 10);
    if (demon_lnx_read_whitespace(maps) == 10){
      break;
    }
    
    // pathname
    pathname = demon_lnx_read_string(arena, maps);
    
    // emit entry if en
    b = 0;
    read(maps, &b, sizeof(b));
    if (b != '\n' && b != '\0') {
      break;
    }
    
    // fill result
    entry_out->address_lo = address_lo;
    entry_out->address_hi = address_hi;
    entry_out->perms      = perms;
    entry_out->offset     = offset;
    entry_out->dev_major  = (U32)dev_major;
    entry_out->dev_minor  = (U32)dev_minor;
    entry_out->inode      = inode;
    entry_out->pathname   = pathname;
    entry_out->type       = DEMON_LNX_MapsEntryType_Null;
    entry_out->stack_tid  = 0;
    
    if (str8_match(pathname, str8_lit("/"), StringMatchFlag_RightSideSloppy)){
      entry_out->type = DEMON_LNX_MapsEntryType_Path;
    } else if (str8_match(pathname, str8_lit("[heap]"), 0)){
      entry_out->type = DEMON_LNX_MapsEntryType_Heap;
    } else if (str8_match(pathname, str8_lit("[stack]"), 0)){
      entry_out->type = DEMON_LNX_MapsEntryType_Stack;
    } else if (str8_match(pathname, str8_lit("[stack:"), StringMatchFlag_RightSideSloppy)){
      entry_out->type = DEMON_LNX_MapsEntryType_Stack;
      String8 tid = str8_substr(pathname, r1u64(7, pathname.size - 8));
      entry_out->stack_tid = (pid_t)u64_from_str8(tid, 10);
    }
    
    is_parsed = true;
  }while(0);
  return(is_parsed);
}

////////////////////////////////
//~ rjf: @demon_os_hooks Main Layer Initialization

internal void
demon_os_init(void){
  demon_lnx_event_arena = arena_alloc();
}

////////////////////////////////
//~ rjf: @demon_os_hooks Running/Halting

internal DEMON_EventList
demon_os_run(Arena *arena, DEMON_OS_RunCtrls *controls){
  DEMON_EventList result = {0};
  
  if (demon_ent_root == 0){
    demon_push_event(arena, &result, DEMON_EventKind_NotInitialized);
  }
  else if (demon_ent_root->first == 0 && !demon_lnx_new_process_pending){
    demon_push_event(arena, &result, DEMON_EventKind_NotAttached);
  }
  else{
    Temp scratch = scratch_begin(&arena, 1);
    
    // use queued events if there are any
    if (demon_lnx_queued_events.first != 0){
      // copy event queue
      for (DEMON_Event *node = demon_lnx_queued_events.first;
           node != 0;
           node = node->next){
        DEMON_Event *copy = push_array_no_zero(arena, DEMON_Event, 1);
        MemoryCopyStruct(copy, node);
        SLLQueuePush(result.first, result.last, copy);
      }
      result.count = demon_lnx_queued_events.count;
      
      // zero stored queue
      MemoryZeroStruct(&demon_lnx_queued_events);
      arena_clear(demon_lnx_event_arena);
    }
    
    // get the single step thread (if any)
    DEMON_Entity *single_step_thread = controls->single_step_thread;
    
    // do setup
    B32 did_setup = false;
    U8 *trap_swap_bytes = 0;
    
    if (result.first == 0){
      // TODO(allen): per-Arch implementation of single steps
      // set single step bit
      if (single_step_thread != 0){
        switch (single_step_thread->arch){
          case Arch_x86:
          {
            // TODO(allen): possibly buggy
            SYMS_RegX86 regs = {0};
            demon_os_read_regs_x86(single_step_thread, &regs);
            regs.eflags.u32 |= 0x100;
            demon_os_write_regs_x86(single_step_thread, &regs);
          }break;
          
          case Arch_x64:
          {
            // TODO(allen): possibly buggy
            SYMS_RegX64 regs = {0};
            demon_os_read_regs_x64(single_step_thread, &regs);
            regs.rflags.u64 |= 0x100;
            demon_os_write_regs_x64(single_step_thread, &regs);
          }break;
        }
      }
      
      // TODO(allen): per-Arch implementation of traps
      trap_swap_bytes = push_array_no_zero(scratch.arena, U8, controls->trap_count);
      
      {
        DEMON_OS_Trap *trap = controls->traps;
        for (U64 i = 0; i < controls->trap_count; i += 1, trap += 1){
          if (demon_os_read_memory(trap->process, trap_swap_bytes + i, trap->address, 1)){
            U8 int3 = 0xCC;
            demon_os_write_memory(trap->process, trap->address, &int3, 1);
          }
          else{
            trap_swap_bytes[i] = 0xCC;
          }
        }
      }
      
      did_setup = true;
    }
    
    // do run
    B32 did_run = false;
    if (did_setup){
      // continue non-frozen threads
      DEMON_LNX_EntityNode *resume_threads = 0;
      for (DEMON_Entity *process = demon_ent_root->first;
           process != 0;
           process = process->next){
        if (process->kind == DEMON_EntityKind_Process){
          
          // determine if this process is frozen
          B32 process_is_frozen = false;
          if (controls->run_entities_are_processes){
            for (U64 i = 0; i < controls->run_entity_count; i += 1){
              if (controls->run_entities[i] == process){
                process_is_frozen = true;
                break;
              }
            }
          }
          
          for (DEMON_Entity *thread = process->first;
               thread != 0;
               thread = thread->next){
            if (thread->kind == DEMON_EntityKind_Thread){
              // determine if this thread is frozen
              B32 is_frozen = false;
              
              if (controls->single_step_thread != 0 &&
                  controls->single_step_thread != thread){
                is_frozen = true;
              }
              else{
                
                if (controls->run_entities_are_processes){
                  is_frozen = process_is_frozen;
                }
                else{
                  for (U64 i = 0; i < controls->run_entity_count; i += 1){
                    if (controls->run_entities[i] == thread){
                      is_frozen = true;
                      break;
                    }
                  }
                }
                
                if (controls->run_entities_are_unfrozen){
                  is_frozen = !is_frozen;
                }
              }
              
              // continue if not frozen
              if (!is_frozen){
                errno = 0;
                ptrace(PTRACE_CONT, (pid_t)thread->id, 0, 0);
                DEMON_LNX_EntityNode *thread_node = push_array_no_zero(scratch.arena, DEMON_LNX_EntityNode, 1);
                SLLStackPush(resume_threads, thread_node);
                thread_node->entity = thread;
              }
            }
          }
        }
      }
      
      // get next stop
      wait_for_stop:
      B32 did_dummy_stop = false;
      int status = 0;
      pid_t wait_id = waitpid(-1, &status, __WALL);
      
      // increment demon time
      demon_time += 1;
      
      // handle devent
      DEMON_Entity *thread = demon_ent_map_entity_from_id(DEMON_EntityKind_Thread, wait_id);
      if (thread == 0){
        if (wait_id >= 0){
          // TODO(allen): this isn't a great situation! From what I can tell there's no
          // options that I am super happy with for going from unknown tid -> pid.
          // We can parse it out of /proc/<tid>/status; but I don't want to do that until
          // I'm forced to, because it seems like this shouldn't happen if the ptrace
          // API works correctly and we don't have any bugs in our demon entity system.
        }
      }
      else{
        B32 thread_exit = false;
        U64 exit_code = 0;
        
        DEMON_Entity *process = thread->parent;
        // NOTE(allen): hitting this assert should never ever be possible, if our entities
        // are wired up correctly. it doesn't matter what ptrace or waitpid are doing.
        Assert(process != 0);
        
        // read register info
        U64 instruction_pointer = 0;
        union{ SYMS_RegX86 x86; SYMS_RegX64 x64; } regs = {0};
        
        switch (thread->arch){
          case Arch_x86:
          {
            demon_os_read_regs_x86(thread, &regs.x86);
            instruction_pointer = regs.x86.eip.u32;
          }break;
          
          case Arch_x64:
          {
            demon_os_read_regs_x64(thread, &regs.x64);
            instruction_pointer = regs.x64.rip.u64;
          }break;
        }
        
        // check stop status
        if (WIFEXITED(status)){
          thread_exit = true;
        }
        if (WIFSIGNALED(status)){
          exit_code = WTERMSIG(status);
          thread_exit = true;
        }
        
        // extra event list
        DEMON_EventList stop_events = {0};
        
        if (WIFSTOPPED(status)){
          switch (WSTOPSIG(status)){
            case SIGTRAP:
            {
              switch (status >> 8){
                case (SIGTRAP | (PTRACE_EVENT_EXIT << 8)):
                {
                  // TODO(allen): (not sure actually, study this part)
                  thread_exit = true;
                }break;
                
                case (SIGTRAP | (PTRACE_EVENT_CLONE << 8)):
                {
                  // new thread coming
                  unsigned long new_tid = 0;
                  int get_message_result = ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &new_tid);
                  if (get_message_result == -1){
                    // TODO(allen): this isn't right, time to give up on getting this process.
                    // this will likely lead to getting unrecognized wait_id s later. So we need
                    // this stuff in the log to make sense of it still.
                  }
                  else{
                    // thread entity
                    DEMON_Entity *new_thread = demon_ent_new(process, DEMON_EntityKind_Thread, new_tid);
                    demon_thread_count += 1;
                    DEMON_LNX_ThreadExt *thread_ext = demon_lnx_thread_ext(new_thread);
                    thread_ext->expecting_dummy_sigstop = true;
                    
                    // thread event
                    DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_CreateThread);
                    e->process = demon_ent_handle_from_ptr(process);
                    e->thread = demon_ent_handle_from_ptr(new_thread);
                  }
                }break;
                
                case (SIGTRAP | (PTRACE_EVENT_FORK << 8)):
                case (SIGTRAP | (PTRACE_EVENT_VFORK << 8)):
                {
                  // new process coming
                  unsigned long new_pid = 0;
                  int get_message_result = ptrace(PTRACE_GETEVENTMSG, wait_id, 0, &new_pid);
                  if (get_message_result == -1){
                    // TODO(allen): this isn't right, time to give up on getting this process.
                    // this will likely lead to getting unrecognized wait_id s later. So we need
                    // this stuff in the log to make sense of it still.
                  }
                  else{
                    Arch arch = demon_lnx_arch_from_pid(new_pid);
                    
                    // process entity
                    DEMON_Entity *new_process = demon_ent_new(demon_ent_root, DEMON_EntityKind_Process, new_pid);
                    new_process->arch = arch;
                    new_process->ext_u64 = demon_lnx_open_memory_fd_for_pid(new_pid);
                    
                    demon_lnx_new_process_pending = false;
                    
                    // thread entity
                    DEMON_Entity *new_thread = demon_ent_new(new_process, DEMON_EntityKind_Thread, new_pid);
                    demon_thread_count += 1;
                    DEMON_LNX_ThreadExt *thread_ext = demon_lnx_thread_ext(new_thread);
                    thread_ext->expecting_dummy_sigstop = true;
                    
                    // process event
                    {
                      DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_CreateProcess);
                      e->process = demon_ent_handle_from_ptr(new_process);
                    }
                    
                    // thread event
                    {
                      DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_CreateThread);
                      e->process = demon_ent_handle_from_ptr(new_process);
                      e->thread = demon_ent_handle_from_ptr(new_thread);
                    }
                  }
                }break;
                
                default:
                {
                  // check single step
                  DEMON_EventKind e_kind = DEMON_EventKind_Trap;
                  if (thread == single_step_thread){
                    e_kind = DEMON_EventKind_SingleStep;
                  }
                  
                  // check bp
                  if (e_kind == DEMON_EventKind_Trap){
                    DEMON_OS_Trap *trap = controls->traps;
                    for (U64 i = 0; i < controls->trap_count; i += 1, trap += 1){
                      if (trap->process == process && trap->address == instruction_pointer - 1){
                        e_kind = DEMON_EventKind_Breakpoint;
                        break;
                      }
                    }
                  }
                  
                  // adjust ip after breakpoint
                  if (e_kind == DEMON_EventKind_Breakpoint){
                    // TODO(allen): possibly buggy
                    switch (thread->arch){
                      case Arch_x86:
                      {
                        instruction_pointer -= 1;
                        regs.x86.eip.u32 = instruction_pointer;
                        demon_os_write_regs_x86(thread, &regs.x86);
                      }break;
                      
                      case Arch_x64:
                      {
                        instruction_pointer -= 1;
                        regs.x64.rip.u64 = instruction_pointer;
                        demon_os_write_regs_x64(thread, &regs.x64);
                      }break;
                    }
                  }
                  
                  // event
                  DEMON_Event *e = demon_push_event(arena, &stop_events, e_kind);
                  e->process = demon_ent_handle_from_ptr(process);
                  e->thread = demon_ent_handle_from_ptr(thread);
                  e->instruction_pointer = instruction_pointer;
                }break;
              }
            }break;
            
            case SIGSTOP:
            {
              // TODO(allen): we need to figure out how we want to tell apart:
              //  SIGSTOP All-Stop, SIGSTOP Halt, SIGSTOP "User"
              // what we're doing right now == big-time race conditions
              
              DEMON_LNX_ThreadExt *thread_ext = demon_lnx_thread_ext(thread);
              
              if (thread_ext->expecting_dummy_sigstop){
                thread_ext->expecting_dummy_sigstop = false;
                did_dummy_stop = true;
              }
              else if (demon_lnx_already_has_halt_injection){
                DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_Halt);
                e->process = demon_ent_handle_from_ptr(process);
                e->thread = demon_ent_handle_from_ptr(thread);
                e->instruction_pointer = instruction_pointer;
              }
              else{
                // TODO(allen): a signal we don't want to mess with (except to record that it happened maybe)
                // we should "hand it back"
              }
            }break;
            
            default:
            {
#if 0
              // these are a little special. the program cannot continue after these
              // unless the user first does something to change the state (move the IP, change a variable, w/e)
              case SIGABRT:case SIGFPE:case SIGSEGV:
#endif
              
              // event
              DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_Exception);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
              e->instruction_pointer = instruction_pointer;
              e->signo = WSTOPSIG(status);
            }break;
          }
        }
        
        // entity cleanup
        if (thread_exit){
          if (thread->id == process->id){
            // generate events for threads & modules
            for (DEMON_Entity *entity = process->first;
                 entity != 0;
                 entity = entity->next){
              if (entity->kind == DEMON_EntityKind_Thread){
                DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_ExitThread);
                e->process = demon_ent_handle_from_ptr(process);
                e->thread = demon_ent_handle_from_ptr(entity);
              }
              else{
                DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_UnloadModule);
                e->process = demon_ent_handle_from_ptr(process);
                e->module = demon_ent_handle_from_ptr(entity);
              }
            }
            
            // exit event
            DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_ExitProcess);
            e->process = demon_ent_handle_from_ptr(process);
            e->code = exit_code;
            
            // free entity
            demon_ent_release_root_and_children(process);
          }
          else{
            // exit event
            DEMON_Event *e = demon_push_event(arena, &stop_events, DEMON_EventKind_ExitThread);
            e->process = demon_ent_handle_from_ptr(process);
            e->thread = demon_ent_handle_from_ptr(thread);
            e->code = exit_code;
            
            // free entity
            demon_ent_release_root_and_children(thread);
          }
        }
        
        // update all module lists (for each process ...)
        DEMON_EventList module_change_events = {0};
        
        for (DEMON_Entity *proc_node = demon_ent_root->first;
             proc_node != 0;
             proc_node = proc_node->next){
          DEMON_LNX_ModuleNode *first_module = demon_lnx_module_list_from_process(scratch.arena, proc_node);
          
          DEMON_LNX_EntityNode *first_unloaded = 0;
          DEMON_LNX_EntityNode *last_unloaded = 0;
          
          // compute the delta (mark known modules, save list of unloaded modules)
          for (DEMON_Entity *entity = proc_node->first;
               entity != 0;
               entity = entity->next){
            if (entity->kind == DEMON_EntityKind_Module){
              U64 base = entity->id;
              U64 name = entity->ext_u64;
              B32 still_exists = false;
              for (DEMON_LNX_ModuleNode *module_node = first_module;
                   module_node != 0;
                   module_node = module_node->next){
                if (module_node->vaddr == base && module_node->name == name){
                  module_node->already_known = true;
                  still_exists = true;
                  break;
                }
              }
              if (!still_exists){
                DEMON_LNX_EntityNode *node = push_array_no_zero(scratch.arena, DEMON_LNX_EntityNode, 1);
                SLLQueuePush(first_unloaded, last_unloaded, node);
                node->entity = entity;
              }
            }
          }
          
          // handle unloads
          for (DEMON_LNX_EntityNode *unloaded_node = first_unloaded;
               unloaded_node != 0;
               unloaded_node = unloaded_node->next){
            DEMON_Entity *module = unloaded_node->entity;
            
            // event
            {
              DEMON_Event *e = demon_push_event(arena, &module_change_events, DEMON_EventKind_UnloadModule);
              e->process = demon_ent_handle_from_ptr(proc_node);
              e->module = demon_ent_handle_from_ptr(module);
            }
            
            // free entity
            demon_ent_release_root_and_children(module);
          }
          
          // handle loads
          for (DEMON_LNX_ModuleNode *module_node = first_module;
               module_node != 0;
               module_node = module_node->next){
            if (!module_node->already_known){
              // entity
              DEMON_Entity *module = demon_ent_new(proc_node, DEMON_EntityKind_Module, module_node->vaddr);
              demon_module_count += 1;
              module->ext_u64 = module_node->name;
              
              // event
              {
                DEMON_Event *e = demon_push_event(arena, &module_change_events, DEMON_EventKind_LoadModule);
                e->process = demon_ent_handle_from_ptr(proc_node);
                e->module = demon_ent_handle_from_ptr(module);
                e->address = module_node->vaddr;
                e->size = module_node->size;
              }
            }
          }
        }
        
        // concat the events list (with module changes first)
        result.count = module_change_events.count + stop_events.count;
        result.first = module_change_events.first;
        result.last = module_change_events.last;
        if (stop_events.first != 0){
          if (result.first != 0){
            result.last->next = stop_events.first;
            result.last = stop_events.last;
          }
          else{
            result.first = stop_events.first;
            result.last = stop_events.last;
          }
        }
      }
      
      // do we have a reason to keep going?
      B32 skip_this_stop = false;
      if (did_dummy_stop && result.count == 0){
        skip_this_stop = true;
      }
      
      // ignore this stop, resume and wait again
      if (skip_this_stop){
        if (wait_id != 0){
          ptrace(PTRACE_CONT, (pid_t)wait_id, 0, 0);
        }
        goto wait_for_stop;
      }
      
      // stop all running threads
      for (DEMON_LNX_EntityNode *node = resume_threads;
           node != 0;
           node = node->next){
        DEMON_Entity *thread = node->entity;
        pid_t thread_id = (pid_t)thread->id;
        if (thread_id != wait_id){
          union sigval sv = {0};
          sigqueue(thread_id, SIGSTOP, sv);
          
          DEMON_LNX_ThreadExt *thread_ext = demon_lnx_thread_ext(thread);
          thread_ext->expecting_dummy_sigstop = true;
        }
      }
      
      did_run = true;
    }
    
    // cleanup
    if (did_run){
      // TODO(allen): per-Arch
      // unset traps
      {
        DEMON_OS_Trap *trap = controls->traps;
        for (U64 i = 0; i < controls->trap_count; i += 1, trap += 1){
          U8 og_byte = trap_swap_bytes[i];
          if (og_byte != 0xCC){
            demon_os_write_memory(trap->process, trap->address, &og_byte, 1);
          }
        }
      }
      
      // TODO(allen): per-Arch
      // unset single step bit
      //  the single step bit is automatically unset whenever we single step
      //  but if *something else* happened, it will still be there ready to
      //  confound us later; so here we're just being sure it's taken out.
      if (single_step_thread != 0){
        // TODO(allen): possibly buggy
        switch (single_step_thread->arch){
          case Arch_x86:
          {
            SYMS_RegX86 regs = {0};
            demon_os_read_regs_x86(single_step_thread, &regs);
            regs.eflags.u32 &= ~0x100;
            demon_os_write_regs_x86(single_step_thread, &regs);
          }break;
          
          case Arch_x64:
          {
            SYMS_RegX64 regs = {0};
            demon_os_read_regs_x64(single_step_thread, &regs);
            regs.rflags.u64 &= ~0x100;
            demon_os_write_regs_x64(single_step_thread, &regs);
          }break;
        }
      }
    }
    
    scratch_end(scratch);
  }
  
  return(result);
}

internal void
demon_os_halt(U64 code, U64 user_data){
  if (demon_ent_root != 0 && !demon_lnx_already_has_halt_injection){
    DEMON_Entity *process = demon_ent_root->first;
    if (process != 0){
      demon_lnx_already_has_halt_injection = true;
      demon_lnx_halt_code = code;
      demon_lnx_halt_user_data = user_data;
      union sigval sv = {0};
      if (sigqueue(process->id, SIGSTOP, sv) == -1){
        demon_lnx_already_has_halt_injection = false;
      }
    }
  }
}

// NOTE(allen): siginfo hint from old code:
#if 0
{
  switch (siginfo.si_code){
    // SI_KERNEL (hit int3; 0xCC)
    case 0x80:
    {
      // TODO(allen): breakpoint event
    }break;
    
    // TRAP_UNK, TRAP_HWBKPT, TRAP_BRKPT, TRAP_TRACE
    case 0x5: case 0x4: case 0x1: case 0x2:
    {
      // TODO(allen): breakpoint event (?)
    }break;
    
    case 0x3: case 0x0:
    {
      // TODO(allen): do nothing I guess?
    }break;
  }
}
#endif

////////////////////////////////
//~ rjf: @demon_os_hooks Target Process Launching/Attaching/Killing/Detaching/Halting

internal U32
demon_os_launch_process(OS_LaunchOptions *options){
  U32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  
  // arrange options
  char *binary = 0;
  char **args = 0;
  if (options->cmd_line.node_count > 0){
    args = push_array_no_zero(scratch.arena, char*, options->cmd_line.node_count + 1);
    char **arg_ptr = args;
    for (String8Node *node = options->cmd_line.first;
         node != 0;
         node = node->next, arg_ptr += 1){
      String8 string = push_str8_copy(scratch.arena, node->string);
      *arg_ptr = (char*)string.str;
    }
    *arg_ptr = 0;
    binary = args[0];
  }
  
  char *path = 0;
  {
    String8 string = push_str8_copy(scratch.arena, options->path);
    path = (char*)string.str;
  }
  
  char **env = 0;
  if (options->env.node_count > 0){
    env = push_array_no_zero(scratch.arena, char*, options->env.node_count + 1);
    char **env_ptr = env;
    for (String8Node *node = options->env.first;
         node != 0;
         node = node->next, env_ptr += 1){
      String8 string = push_str8_copy(scratch.arena, node->string);
      *env_ptr = (char*)string.str;
    }
    *env_ptr = 0;
  }
  
  // fork
  if (binary != 0){
    pid_t pid = fork();
    if (pid == -1){
      // TODO(allen): fork error
    }
    else if (pid == 0){
      // NOTE(allen): child process
      int ptrace_result = ptrace(PTRACE_TRACEME, 0, 0, 0);
      if (ptrace_result != -1){
        int chdir_result = chdir(path);
        if (chdir_result != -1){
          execve(binary, args, env);
        }
      }
      // failed to init fully; abort so the parent can clean up the child
      abort();
    }
    else{
      // NOTE(allen): parent process
      
      // wait for child
      int status = 0;
      pid_t wait_id = waitpid(pid, &status, __WALL);
      
      // determine child launch status
      enum{
        LaunchCode_Null,
        LaunchCode_FailBeforePtrace,
        LaunchCode_FailAfterPtrace,
        LaunchCode_Success,
      };
      U32 launch_result = LaunchCode_Null;
      // NOTE(allen): if wait_id != pid we don't know what that means; study that case before
      // deciding how error handling around it works.
      if (wait_id == pid){
        if (WIFSTOPPED(status)){
          if (WSTOPSIG(status) == SIGTRAP){
            launch_result = LaunchCode_Success;
          }
          else{
            launch_result = LaunchCode_FailAfterPtrace;
          }
        }
        else{
          launch_result = LaunchCode_FailBeforePtrace;
        }
      }
      
      // handle launch result
      switch (launch_result){
        default:
        {
          // TODO(allen): error that we do not understand
        }break;
        
        case LaunchCode_FailBeforePtrace:
        {
          // TODO(allen): child ptrace init failed
        }break;
        
        case LaunchCode_FailAfterPtrace:
        {
          // need to specifically pull the exit status out of the child
          // or it will sit around as a zombie forever since it is ptraced.
          B32 cleanup_good = false;
          int detach_result = ptrace(PTRACE_DETACH, pid, 0, (void*)SIGCONT);
          if (detach_result != -1){
            int status_cleanup = 0;
            pid_t wait_id_cleanup = waitpid(pid, &status_cleanup, __WALL);
            if (wait_id_cleanup == pid){
              cleanup_good = true;
            }
          }
          if (cleanup_good){
            // TODO(allen): child init failed
          }
          else{
            // TODO(allen): child init failed; something went wrong and a process may have leaked
          }
        }break;
        
        case LaunchCode_Success:
        {
          int setoptions_result = ptrace(PTRACE_SETOPTIONS, pid, 0, PtrFromInt(demon_lnx_ptrace_options));
          if (setoptions_result == -1){
            // TODO(allen): ptrace setup failed; need to kill the child and clean it up
          }
          else{
            result = pid;
            
            Arch arch = demon_lnx_arch_from_pid(pid);
            
            // process entity
            DEMON_Entity *process = demon_ent_new(demon_ent_root, DEMON_EntityKind_Process, pid);
            demon_proc_count += 1;
            process->arch = arch;
            process->ext_u64 = demon_lnx_open_memory_fd_for_pid(pid);
            
            // thread entity
            DEMON_Entity *thread = demon_ent_new(process, DEMON_EntityKind_Thread, pid);
            demon_thread_count += 1;
            
            // process event
            {
              DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                                DEMON_EventKind_CreateProcess);
              e->process = demon_ent_handle_from_ptr(process);
            }
            
            // thread event
            {
              DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                                DEMON_EventKind_CreateThread);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
            }
            
            // get module list
            DEMON_LNX_ModuleNode *module_list = demon_lnx_module_list_from_process(scratch.arena, process);
            
            // for each module ...
            for (DEMON_LNX_ModuleNode *node = module_list;
                 node != 0;
                 node = node->next){
              // module entity
              DEMON_Entity *module = demon_ent_new(process, DEMON_EntityKind_Module, node->vaddr);
              demon_module_count += 1;
              module->ext_u64 = node->name;
              
              // event
              {
                DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                                  DEMON_EventKind_LoadModule);
                e->process = demon_ent_handle_from_ptr(process);
                e->module = demon_ent_handle_from_ptr(module);
                e->address = node->vaddr;
                e->size = node->size;
              }
            }
            
            // handshake event
            {
              DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                                DEMON_EventKind_HandshakeComplete);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
            }
          }
        }break;
      }
    }
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
demon_os_attach_process(U32 pid){
  B32 result = false;
  
  Temp scratch = scratch_begin(0, 0);
  DEMON_LNX_AttachNode *attachments = 0;
  DEMON_LNX_AttachNode *the_process = 0;
  
  // TODO(allen): double check that this logic only lets us
  // "attach" when pid is the id of the main thread of a process.
  
  // attach this process
  B32 attached_proc = false;
  if (kill(pid, 0) == -1){
    // TODO(allen): process does not exist
  }
  else{
    attached_proc = demon_lnx_attach_pid(scratch.arena, pid, &the_process);
    if (the_process != 0){
      SLLStackPush(attachments, the_process);
    }
  }
  
  // open thread list
  if (attached_proc){
    String8 threads_path = push_str8f(scratch.arena, "/proc/%d/task", pid);
    DIR *proc_dir = opendir((char*)threads_path.str);
    if (proc_dir == 0){
      // TODO(allen): could not read proc threads somehow; no good!
    }
    else{
      
      // attach all threads
      B32 attached_all_threads = true;
      for (;;){
        struct dirent *entry = readdir(proc_dir);
        if (entry == 0){
          break;
        }
        
        String8 name = str8_cstring(entry->d_name);
        if (str8_is_integer(name, 10)){
          pid_t tid = u64_from_str8(name, 10);
          if (tid != pid){
            DEMON_LNX_AttachNode *new_attachment = 0;
            B32 attached_this_thread = demon_lnx_attach_pid(scratch.arena, tid, &new_attachment);
            if (new_attachment != 0){
              SLLStackPush(attachments, new_attachment);
            }
            if (!attached_this_thread){
              attached_all_threads = false;
              break;
            }
          }
        }
      }
      closedir(proc_dir);
      
      if (attached_all_threads){
        result = true;
      }
    }
  }
  
  // initialize new entities on success
  if (result){
    Arch arch = demon_lnx_arch_from_pid(the_process->pid);
    
    // process entity
    DEMON_Entity *process = demon_ent_new(demon_ent_root, DEMON_EntityKind_Process, the_process->pid);
    demon_proc_count += 1;
    process->arch = arch;
    process->ext_u64 = demon_lnx_open_memory_fd_for_pid(the_process->pid);
    
    // process event
    {
      DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                        DEMON_EventKind_CreateProcess);
      e->process = demon_ent_handle_from_ptr(process);
    }
    
    // TODO(allen): happens on windows here?
    
    for (DEMON_LNX_AttachNode *node = attachments;
         node != 0;
         node = node->next){
      DEMON_Entity *thread = demon_ent_new(process, DEMON_EntityKind_Thread, node->pid);
      demon_thread_count += 1;
      
      // thread event
      {
        DEMON_Event *e = demon_push_event(demon_lnx_event_arena, &demon_lnx_queued_events,
                                          DEMON_EventKind_CreateThread);
        e->process = demon_ent_handle_from_ptr(process);
        e->thread = demon_ent_handle_from_ptr(thread);
      }
    }
    
    // TODO(allen): sync modules in process
  }
  
  // cleanup on failure
  else{
    for (DEMON_LNX_AttachNode *node = attachments;
         node != 0;
         node = node->next){
      ptrace(PTRACE_DETACH, node->pid, 0, (void*)SIGCONT);
    }
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
demon_os_kill_process(DEMON_Entity *process, U32 exit_code){
  B32 result = false;
  if (process != 0){
    if (kill(process->id, SIGKILL) != -1){
      result = true;
    }
  }
  return(result);
}

internal B32
demon_os_detach_process(DEMON_Entity *process){
  B32 result = false;
  if (process != 0){
    int detach_result = ptrace(PTRACE_DETACH, process->id, 0, 0);
    result = (detach_result != -1);
  }
  return(0);
}

////////////////////////////////
//~ rjf: @demon_os_hooks Entity Functions

//- rjf: cleanup

internal void
demon_os_entity_cleanup(DEMON_Entity *entity)
{
  // NOTE(rjf): no-op
}

//- rjf: introspection

internal String8
demon_os_full_path_from_module(Arena *arena, DEMON_Entity *module){
  DEMON_Entity *process = module->parent;
  int memory_fd = (int)process->ext_u64;
  U64 name_va = module->ext_u64;
  String8 result = demon_lnx_read_memory_str(arena, memory_fd, name_va);
  return(result);
}

internal U64
demon_os_stack_base_vaddr_from_thread(DEMON_Entity *thread){
  Temp scratch = scratch_begin(0, 0);
  
  U64 stack_base = 0;
  
  DEMON_Entity *process = thread->parent;
  
  // id for main thread is zero
  B32 is_main_thread = (thread->id == process->id);
  pid_t match_tid = is_main_thread ? 0 : thread->id;
  
  // open /proc/$pid/maps
  int maps = demon_lnx_open_maps(process->id);
  
  // look for entry with stack markings and matching thread id
  for (;;){
    DEMON_LNX_MapsEntry e;
    Temp temp = temp_begin(scratch.arena);
    if (!demon_lnx_next_map(temp.arena, maps, &e)){
      break;
    }
    if (e.type == DEMON_LNX_MapsEntryType_Stack && e.stack_tid == match_tid){
      stack_base = e.address_lo;
      break;
    }
    temp_end(temp);
  }
  
  scratch_end(scratch);
  return(stack_base);
}

internal U64
demon_os_tls_root_vaddr_from_thread(DEMON_Entity *thread){
  U64 result = 0;
  switch (thread->arch){
    case Arch_x64:
    case Arch_x86:
    {
      U32 fsbase = 0;
      pid_t tid = (pid_t)thread->id;
      if (ptrace(PT_GETFSBASE, tid, (void*)&fsbase, 0) != -1){
        result = (U64)fsbase;
      }
      if (thread->arch == Arch_x64){
        result += 8;
      }
      else{
        result += 4;
      }
    }break;
  }
  return(result);
}

//- rjf: target process memory allocation/protection

internal U64
demon_os_reserve_memory(DEMON_Entity *process, U64 size){
  U64 result = 0;
  NotImplemented;
  return(result);
}

internal void
demon_os_set_memory_protect_flags(DEMON_Entity *process, U64 page_vaddr, U64 size, DEMON_MemoryProtectFlags flags){
  NotImplemented;
}

internal void
demon_os_release_memory(DEMON_Entity *process, U64 vaddr, U64 size){
  NotImplemented;
}

//- rjf: target process memory reading/writing

internal U64
demon_os_read_memory(DEMON_Entity *process, void *dst, U64 src_address, U64 size){
  int memory_fd = (int)process->ext_u64;
  U64 result = demon_lnx_read_memory(memory_fd, dst, src_address, size);
  return(result);
}

internal B32
demon_os_write_memory(DEMON_Entity *process, U64 dst_address, void *src, U64 size){
  int memory_fd = (int)process->ext_u64;
  B32 result = demon_lnx_write_memory(memory_fd, dst_address, src, size);
  return(result);
}

//- rjf: thread registers reading/writing

internal B32
demon_os_read_regs_x86(DEMON_Entity *thread, SYMS_RegX86 *dst){
  B32 result = false;
  NotImplemented;
  return(result);
}

internal B32
demon_os_write_regs_x86(DEMON_Entity *thread, SYMS_RegX86 *src){
  B32 result = false;
  NotImplemented;
  return(result);
}

internal B32
demon_os_read_regs_x64(DEMON_Entity *thread, SYMS_RegX64 *dst){
  pid_t tid = (pid_t)thread->id;
  
  // gpr
  B32 got_gpr = false;
  DEMON_LNX_UserX64 ctx = {0};
  struct iovec iov_gpr = {0};
  iov_gpr.iov_len = sizeof(ctx);
  iov_gpr.iov_base = &ctx;
  if (ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &iov_gpr) != -1){
    demon_lnx_regs_x64_from_usr_regs_x64(dst, &ctx.regs);
    got_gpr = true;
  }
  
  // fpr
  B32 got_fpr = false;
  if (got_gpr){
    B32 got_xsave = false;
    {
      U8 xsave_buffer[KB(4)];
      struct iovec iov_xsave = {0};
      iov_xsave.iov_len = sizeof(xsave_buffer);
      iov_xsave.iov_base = xsave_buffer;
      if (ptrace(PTRACE_GETREGSET, tid, (void*)NT_X86_XSTATE, &iov_xsave) != -1){
        SYMS_XSave *xsave = (SYMS_XSave*)xsave_buffer;
        syms_x64_regs__set_full_regs_from_xsave_legacy(dst, &xsave->legacy);
        
        // TODO(allen): this is a lie; ymm can technically move around
        // we need some more low-level-assembly-fu to do this hardcore.
        B32 has_ymm_registers = ((xsave->header.xstate_bv & 4) != 0);
        if (has_ymm_registers){
          syms_x64_regs__set_full_regs_from_xsave_avx_extension(dst, xsave->ymmh);
        }
        
        got_xsave = true;
      }
    }
    
    B32 got_fxsave = false;
    if (!got_xsave){
      SYMS_XSaveLegacy fxsave = {0};
      struct iovec iov_fxsave = {0};
      iov_fxsave.iov_len = sizeof(fxsave);
      iov_fxsave.iov_base = &fxsave;
      if (ptrace(PTRACE_GETREGSET, tid, (void*)NT_FPREGSET, &iov_fxsave) != -1){
        syms_x64_regs__set_full_regs_from_xsave_legacy(dst, &fxsave);
        got_fxsave = true;
      }
    }
    
    if (got_xsave || got_fxsave){
      got_fpr = true;
    }
  }
  
  // debug
  B32 got_debug = false;
  if (got_fpr){
    got_debug = true;
    SYMS_Reg32 *dr_d = &dst->dr0;
    for (U32 i = 0; i < 8; i += 1, dr_d += 1){
      if (i != 4 && i != 5){
        U64 offset = OffsetOf(DEMON_LNX_UserX64, u_debugreg[i]);
        errno = 0;
        int peek_result = ptrace(PTRACE_PEEKUSER, tid, PtrFromInt(offset), 0);
        if (errno == 0){
          dr_d->u32 = (U32)peek_result;
        }
        else{
          got_debug = false;
        }
      }
    }
  }
  
  // got everything
  B32 result = got_debug;
  return(result);
}

internal B32
demon_os_write_regs_x64(DEMON_Entity *thread, SYMS_RegX64 *src){
  pid_t tid = (pid_t)thread->id;
  
  // gpr
  DEMON_LNX_UserX64 ctx = {0};
  demon_lnx_usr_regs_x64_from_regs_x64(&ctx.regs, src);
  
  struct iovec iov_gpr = {0};
  iov_gpr.iov_base = &ctx;
  iov_gpr.iov_len = sizeof(ctx);
  int gpr_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &iov_gpr);
  B32 gpr_success = (gpr_result != -1);
  
  // fpr
  int xsave_result = 0;
  int fxsave_result = 0;
  
  {
    U8 xsave_buffer[KB(4)] = {0};
    SYMS_XSave *xsave = (SYMS_XSave*)xsave_buffer;
    syms_x64_regs__set_xsave_legacy_from_full_regs(&xsave->legacy, src);
    
    xsave->header.xstate_bv = 7;
    
    // TODO(allen): this is a lie; ymm can technically move around
    // we need some more low-level-assembly-fu to do this hardcore.
    syms_x64_regs__set_xsave_avx_extension_from_full_regs(xsave->ymmh, src);
    
    {
      struct iovec iov_xsave = {0};
      iov_xsave.iov_base = &xsave;
      iov_xsave.iov_len = sizeof(xsave);
      xsave_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_X86_XSTATE, &iov_xsave);
    }
    
    if (xsave_result == -1){
      struct iovec iov_fxsave = {0};
      iov_fxsave.iov_base = &xsave->legacy;
      iov_fxsave.iov_len = sizeof(xsave->legacy);
      fxsave_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_FPREGSET, &iov_fxsave);
    }
  }
  
  B32 fpr_success = (xsave_result != -1 || fxsave_result != -1);
  
  // debug
  B32 dr_success = true;
  {
    SYMS_Reg32 *dr_s = &src->dr0;
    for (U32 i = 0; i < 8; i += 1, dr_s += 1){
      if (i != 4 && i != 5){
        U64 offset = OffsetOf(DEMON_LNX_UserX64, u_debugreg[i]);
        errno = 0;
        int poke_result = ptrace(PTRACE_POKEUSER, tid, PtrFromInt(offset), dr_s->u32);
        if (poke_result == -1){
          dr_success = false;
        }
      }
    }
  }
  
  // assemble result
  B32 result = (gpr_success && fpr_success && dr_success);
  
  return(result);
}

////////////////////////////////
//~ rjf: @demon_os_hooks Process Listing

internal void
demon_os_proc_iter_begin(DEMON_ProcessIter *iter){
  DIR *dir = opendir("/proc");
  MemoryZeroStruct(iter);
  iter->v[0] = IntFromPtr(dir);
}

internal B32
demon_os_proc_iter_next(Arena *arena, DEMON_ProcessIter *iter, DEMON_ProcessInfo *info_out){
  // scan for a process id
  B32 got_pid = false;
  String8 pid_string = {0};
  
  DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
  if (dir != 0 && iter->v[1] == 0){
    for (;;){
      struct dirent *d = readdir(dir);
      if (d == 0){
        break;
      }
      
      // check file name is integer
      String8 file_name = str8_cstring((char*)d->d_name);
      B32 is_integer = str8_is_integer(file_name, 10);
      
      // break on integers (which represent processes)
      if (is_integer){
        got_pid = true;
        pid_string = file_name;
        break;
      }
    }
  }
  
  // mark iterator dead if nothing found
  if (!got_pid){
    iter->v[1] = 1;
  }
  
  // if got process id convert pid -> process info
  B32 result = false;
  if (got_pid){
    // determine the name we will report
    pid_t pid = u64_from_str8(pid_string, 10);
    String8 name = demon_lnx_executable_path_from_pid(arena, pid);
    if (name.size == 0){
      name = str8_lit("<name-not-resolved>");
    }
    
    // finish conversion
    info_out->name = name;
    info_out->pid = pid;
    result = true;
  }
  
  return(result);
}

internal void
demon_os_proc_iter_end(DEMON_ProcessIter *iter){
  DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
  if (dir != 0){
    closedir(dir);
  }
  MemoryZeroStruct(iter);
}
