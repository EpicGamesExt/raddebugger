// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

//- rjf: file descriptor memory reading/writing helpers

internal U64
dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst)
{
  U64 bytes_read = 0;
  U8 *ptr = (U8 *)dst;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    size_t to_read = (size_t)(opl - ptr);
    ssize_t actual_read = pread(memory_fd, ptr, to_read, cursor);
    if(actual_read == -1)
    {
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
    bytes_read += actual_read;
  }
  return bytes_read;
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

internal Arch
dmn_lnx_arch_from_pid(pid_t pid)
{
  Arch result = Arch_Null;
  {
    Temp scratch = scratch_begin(0, 0);
    String8 exe_path = dmn_lnx_exe_path_from_pid(scratch.arena, pid);
    
    // rjf: unpack exe handle
    int exe_fd = -1;
    if(exe_path.size != 0)
    {
      exe_fd = open((char*)exe_path.str, O_RDONLY);
    }
    
    // rjf: unpack elf identifier
    U8 e_ident[ELF_Identifier_Max] = {0};
    B32 is_elf = 0;
    U8 elf_class = 0;
    if(exe_fd >= 0 &&
       pread(exe_fd, e_ident, sizeof(e_ident), 0) == sizeof(e_ident))
    {
      is_elf = (e_ident[ELF_Identifier_Mag0] == 0x7f && 
                e_ident[ELF_Identifier_Mag1] == 'E'  &&
                e_ident[ELF_Identifier_Mag2] == 'L'  && 
                e_ident[ELF_Identifier_Mag3] == 'F');
      elf_class = e_ident[ELF_Identifier_Class];
    }
    
    // rjf: read elf header
    ELF_Hdr64 hdr = {0};
    switch(elf_class)
    {
      case 1:
      {
        ELF_Hdr32 hdr32 = {0};
        if(pread(exe_fd, &hdr32, sizeof(hdr32), 0) == sizeof(hdr32))
        {
          hdr = elf_hdr64_from_hdr32(hdr32);
        }
      }break;
      case 2:
      {
        pread(exe_fd, &hdr, sizeof(hdr), 0);
      }break;
    }
    
    // rjf: determine arch from elf machine kind
    result = arch_from_elf_machine(hdr.e_machine);
    
    scratch_end(scratch);
  }
  return result;
}

internal DMN_LNX_ProcessAux
dmn_lnx_aux_from_pid(pid_t pid, Arch arch)
{
  Temp scratch = scratch_begin(0, 0);
  DMN_LNX_ProcessAux result = {0};
  
  // rjf: open aux data
  String8 auxv_path = push_str8f(scratch.arena, "/proc/%d/auxv", pid);
  int aux_fd = open((char*)auxv_path.str, O_RDONLY);
  
  // rjf: scan aux data
  if(aux_fd >= 0)
  {
    B32 addr_32bit = (arch == Arch_x86 || arch == Arch_arm32);
    for(;;)
    {
      result.filled = 1;
      
      // rjf: read next aux
      U64 type = 0;
      U64 val = 0;
      if(addr_32bit)
      {
        ELF_Auxv32 aux = {0};
        if(read(aux_fd, &aux, sizeof(aux)) != sizeof(aux))
        {
          goto brkloop;
        }
        type = aux.a_type;
        val  = aux.a_val;
      }
      else
      {
        ELF_Auxv64 aux = {0};
        if(read(aux_fd, &aux, sizeof(aux)) != sizeof(aux))
        {
          goto brkloop;
        }
        type = aux.a_type;
        val  = aux.a_val;
      }
      
      // rjf: fill result
      switch(type)
      {
        default:{}break;
        case ELF_AuxType_Null:         goto brkloop; break;
        case ELF_AuxType_Phnum:        result.phnum  = val; break;
        case ELF_AuxType_Phent:        result.phent  = val; break;
        case ELF_AuxType_Phdr:         result.phdr   = val; break;
        case ELF_AuxType_ExecFn:       result.execfn = val; break;
      }
    }
    brkloop:;
    close(aux_fd);
  }
  
  scratch_end(scratch);
  return result;
}

//- rjf: phdr info extraction

internal DMN_LNX_PhdrInfo
dmn_lnx_phdr_info_from_memory(int memory_fd, B32 is_32bit, U64 phvaddr, U64 phsize, U64 phcount)
{
  DMN_LNX_PhdrInfo result = {0};
  
  // rjf: determine how much phdr we'll read
  U64 phdr_size_expected = (is_32bit ? sizeof(ELF_Phdr32) : sizeof(ELF_Phdr64));
  U64 phdr_stride = (phsize ? phsize : phdr_size_expected);
  U64 phdr_read_size = ClampTop(phsize, phdr_size_expected);
  
  // rjf: scan table
  U64 va = phvaddr;
  for(U64 i = 0; i < phcount; i += 1, va += phdr_stride)
  {
    // rjf: read type and range
    ELF_PType p_type = 0;
    U64 p_vaddr = 0;
    U64 p_memsz = 0;
    if(is_32bit)
    {
      ELF_Phdr32 phdr32 = {0};
      dmn_lnx_read_struct(memory_fd, va, &phdr32);
      p_type = phdr32.p_type;
      p_vaddr = phdr32.p_vaddr;
      p_memsz = phdr32.p_memsz;
    }
    else
    {
      ELF_Phdr64 phdr64 = {0};
      dmn_lnx_read_struct(memory_fd, va, &phdr64);
      p_type = phdr64.p_type;
      p_vaddr = phdr64.p_vaddr;
      p_memsz = phdr64.p_memsz;
    }
    
    // rjf: save
    switch(p_type)
    {
      case ELF_PType_Dynamic:
      {
        result.dynamic = p_vaddr;
      }break;
      case ELF_PType_Load:
      {
        U64 min = p_vaddr;
        U64 max = p_vaddr + p_memsz;
        result.range.min = Min(result.range.min, min);
        result.range.max = Max(result.range.max, max);
      }break;
    }
  }
  
  return result;
}

//- rjf: process entity => info extraction

internal DMN_LNX_ModuleInfoList
dmn_lnx_module_info_list_from_process(Arena *arena, DMN_LNX_Entity *process)
{
  Arch arch = process->arch;
  B32 is_32bit = (arch == Arch_x86 || arch == Arch_arm32);
  int memory_fd = (int)process->fd;
  
  //- rjf: pid => aux
  DMN_LNX_ProcessAux aux = dmn_lnx_aux_from_pid((pid_t)process->id, arch);
  
  //- rjf: memory => phdr info
  DMN_LNX_PhdrInfo phdr_info = dmn_lnx_phdr_info_from_memory(memory_fd, is_32bit,
                                                             aux.phdr, aux.phent, aux.phnum);
  
  //- rjf: memory space & vaddr => linkmap first
  U64 first_linkmap_vaddr = 0;
  if(phdr_info.dynamic != 0)
  {
    U64 off = phdr_info.dynamic;
    for(;;)
    {
      // rjf: read next dyn entry
      ELF_Dyn64 dyn = {0};
      if(is_32bit)
      {
        ELF_Dyn32 dyn32 = {0};
        dmn_lnx_read_struct(memory_fd, off, &dyn32);
        dyn.tag = dyn32.tag;
        dyn.val = dyn32.val;
        off += sizeof(dyn32);
      }
      else
      {
        dmn_lnx_read_struct(memory_fd, off, &dyn);
        off += sizeof(dyn);
      }
      
      // rjf: break on zero
      if(dyn.tag == ELF_DynTag_Null)
      {
        break;
      }
      
      // rjf: pltgot => grab first linkmap address
      if(dyn.tag == ELF_DynTag_PltGot)
      {
        // True for x86 and x64
        //  vas[0] virtual address of .dynamic
        //  vas[2] callback for resolving function address of relocation and if successful jumps to it.
        // 
        // Code that sets up PLTGOT is in glibc/sysdeps/x86_64/dl_machine.h -> elf_machine_runtime_setup
        //
        U64 vas_off = dyn.val;
        U64 vas[3] = {0};
        dmn_lnx_read(memory_fd, r1u64(vas_off, vas_off+sizeof(vas)), vas);
        first_linkmap_vaddr = vas[1];
        break;
      }
    }
  }
  
  //- rjf: push main module
  DMN_LNX_ModuleInfoList list = {0};
  {
    DMN_LNX_ModuleInfoNode *n = push_array(arena, DMN_LNX_ModuleInfoNode, 1);
    SLLQueuePush(list.first, list.last, n);
    list.count += 1;
    n->v.vaddr_range = phdr_info.range;
    n->v.name = aux.execfn;
  }
  
  //- rjf: iterate link maps
  if(first_linkmap_vaddr != 0)
  {
    U64 linkmap_vaddr = first_linkmap_vaddr;
    for(;linkmap_vaddr != 0;)
    {
      // rjf: read next linkmap entry
      ELF_LinkMap64 linkmap = {0};
      if(is_32bit)
      {
        // TODO(rjf): endianness
        ELF_LinkMap32 linkmap32 = {0};
        dmn_lnx_read_struct(memory_fd, linkmap_vaddr, &linkmap32);
        linkmap.base = linkmap32.base;
        linkmap.name = linkmap32.name;
        linkmap.ld   = linkmap32.ld;
        linkmap.next = linkmap32.next;
      }
      else
      {
        dmn_lnx_read_struct(memory_fd, linkmap_vaddr, &linkmap);
      }
      
      // rjf: push module for next link map
      if(linkmap.base != 0)
      {
        // rjf: find phdr info for this module
        U64 phvaddr = 0;
        U64 phentsize = 0;
        U64 phcount = 0;
        if(is_32bit)
        {
          ELF_Hdr32 ehdr = {0};
          dmn_lnx_read_struct(memory_fd, linkmap.base, &ehdr);
          phvaddr   = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount   = ehdr.e_phnum;
        }
        else
        {
          ELF_Hdr64 ehdr = {0};
          dmn_lnx_read_struct(memory_fd, linkmap.base, &ehdr);
          phvaddr   = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount   = ehdr.e_phnum;
        }
        
        // rjf: extract info from module's phdrs
        DMN_LNX_PhdrInfo module_phdr_info = dmn_lnx_phdr_info_from_memory(memory_fd, is_32bit, phvaddr, phentsize, phcount);
        
        // rjf: push
        DMN_LNX_ModuleInfoNode *n = push_array(arena, DMN_LNX_ModuleInfoNode, 1);
        SLLQueuePush(list.first, list.last, n);
        list.count += 1;
        n->v.vaddr_range = r1u64(linkmap.base, linkmap.base + dim_1u64(module_phdr_info.range));
        n->v.name = linkmap.name;
      }
      
      // rjf: iterate
      linkmap_vaddr = linkmap.next;
    }
  }
  
  return list;
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
  SLLStackPush(dmn_lnx_state->free_entity, entity);
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
  dmn_lnx_state->root_entity = dmn_lnx_entity_alloc(&dmn_lnx_nil_entity, DMN_LNX_EntityKind_Root);
  dmn_lnx_state->access_mutex = os_mutex_alloc();
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
  OS_MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 1;
  }
}

internal void
dmn_ctrl_exclusive_access_end(void)
{
  OS_MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 1;
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
    pid_t pid = 0;
    int ptrace_result = 0;
    int chdir_result = 0;
    int setoptions_result = 0;
    B32 error__need_child_kill = 0;
    
    //- rjf: fork
    pid = fork();
    if(pid == -1) { goto error; }
    
    //- rjf: child process -> execute actual target
    if(pid == 0)
    {
      ptrace_result = ptrace(PTRACE_TRACEME, 0, 0, 0);
      if(ptrace_result == -1) { goto error; }
      chdir_result = chdir(path);
      if(chdir_result == -1) { goto error; }
      execve(argv[0], argv, env);
      abort();
    }
    
    //- rjf: parent process
    if(pid != 0)
    {
      //- rjf: wait for child
      int status = 0;
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
          
          //- rjf: build initial process/thread/modules entities
          DMN_LNX_Entity *process = &dmn_lnx_nil_entity;
          DMN_LNX_Entity *main_thread = &dmn_lnx_nil_entity;
          {
            // rjf: build process
            process = dmn_lnx_entity_alloc(dmn_lnx_state->root_entity, DMN_LNX_EntityKind_Process);
            process->arch = dmn_lnx_arch_from_pid(pid);
            process->id = pid;
            process->fd = open((char*)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR);
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_CreateProcess;
              e->process = dmn_lnx_handle_from_entity(process);
            }
            
            // rjf: build thread
            {
              DMN_LNX_Entity *thread = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Thread);
              thread->id = pid;
              thread->arch = process->arch;
              {
                DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
                e->kind    = DMN_EventKind_CreateThread;
                e->process = dmn_lnx_handle_from_entity(process);
                e->thread  = dmn_lnx_handle_from_entity(thread);
              }
              main_thread = thread;
            }
            
            // rjf: gather all process module infos
            DMN_LNX_ModuleInfoList module_infos = dmn_lnx_module_info_list_from_process(scratch.arena, process);
            for(DMN_LNX_ModuleInfoNode *n = module_infos.first; n != 0; n = n->next)
            {
              DMN_LNX_Entity *module = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
              module->id = n->v.name;
              {
                DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
                e->kind    = DMN_EventKind_LoadModule;
                e->process = dmn_lnx_handle_from_entity(process);
                e->thread  = dmn_lnx_handle_from_entity(main_thread);
                e->module  = dmn_lnx_handle_from_entity(module);
                e->address = n->v.vaddr_range.min;
                e->size    = dim_1u64(n->v.vaddr_range);
              }
            }
            
            // rjf: handshake event
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_HandshakeComplete;
              e->process = dmn_lnx_handle_from_entity(process);
              e->thread  = dmn_lnx_handle_from_entity(main_thread);
            }
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
  return 0;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process)
{
  return 0;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  DMN_EventList evts = {0};
  return evts;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
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
    os_mutex_take(dmn_lnx_state->access_mutex);
    result = !dmn_lnx_state->access_run_state;
  }
  return result;
}

internal void
dmn_access_close(void)
{
  if(!dmn_lnx_ctrl_thread)
  {
    os_mutex_drop(dmn_lnx_state->access_mutex);
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
  return 0;
}

internal B32
dmn_process_write(DMN_Handle process, Rng1U64 range, void *src)
{
  return 0;
}

//- rjf: threads

internal Arch
dmn_arch_from_thread(DMN_Handle handle)
{
  return Arch_Null;
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
  return 0;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block)
{
  return 0;
}

//- rjf: system process listing

internal void
dmn_process_iter_begin(DMN_ProcessIter *iter)
{
}

internal B32
dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out)
{
  return 0;
}

internal void
dmn_process_iter_end(DMN_ProcessIter *iter)
{
}
