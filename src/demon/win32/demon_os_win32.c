// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals

global GetThreadDescriptionFunctionType *demon_w32_GetThreadDescription = 0;

global B32   demon_w32_resume_needed = 0;
global DWORD demon_w32_resume_pid = 0;
global DWORD demon_w32_resume_tid = 0;

global B32   demon_w32_exception_not_handled = 0;
global DEMON_Entity* demon_w32_halter_process = 0;
global DWORD demon_w32_halter_thread_id = 0;

global B32   demon_w32_new_process_pending = 0;

global Arena       *demon_w32_ext_arena = 0 ;
global DEMON_W32_Ext *demon_w32_proc_ext_free = 0;

global Arena          *demon_w32_detach_proc_arena = 0;
global DEMON_EntityNode *demon_w32_first_detached_proc = 0;
global DEMON_EntityNode *demon_w32_last_detached_proc = 0;

global String8List demon_w32_environment = {0};

////////////////////////////////
//~ rjf: Helpers

internal U64
demon_w32_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal DEMON_W32_Ext*
demon_w32_ext_alloc(void){
  DEMON_W32_Ext *result = demon_w32_proc_ext_free;
  if (result != 0){
    SLLStackPop(demon_w32_proc_ext_free);
  }
  else{
    result = push_array_no_zero(demon_w32_ext_arena, DEMON_W32_Ext, 1);
  }
  MemoryZeroStruct(result);
  return(result);
}

internal DEMON_W32_Ext*
demon_w32_ext(DEMON_Entity *entity){
  DEMON_W32_Ext *result = (DEMON_W32_Ext*)entity->ext;
  return(result);
}

internal U64
demon_w32_read_memory(HANDLE process_handle, void *dst, U64 src_address, U64 size){
  U64 bytes_read = 0;
  U8 *ptr = (U8*)dst;
  U8 *opl = ptr + size;
  U64 cursor = src_address;
  for (;ptr < opl;){
    SIZE_T to_read = (SIZE_T)(opl - ptr);
    SIZE_T actual_read = 0;
    if (!ReadProcessMemory(process_handle, (LPCVOID)cursor, ptr, to_read, &actual_read)){
      bytes_read += actual_read;
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
    bytes_read += actual_read;
  }
  return bytes_read;
}

internal B32
demon_w32_write_memory(HANDLE process_handle, U64 dst_address, void *src, U64 size){
  B32 result = 1;
  U8 *ptr = (U8*)src;
  U8 *opl = ptr + size;
  U64 cursor = dst_address;
  for (;ptr < opl;){
    SIZE_T to_write = (SIZE_T)(opl - ptr);
    SIZE_T actual_write = 0;
    if (!WriteProcessMemory(process_handle, (LPVOID)cursor, ptr, to_write, &actual_write)){
      result = 0;
      break;
    }
    ptr += actual_write;
    cursor += actual_write;
  }
  return(result);
}

internal String8
demon_w32_read_memory_str(Arena *arena, HANDLE process_handle, U64 address){
  // TODO(allen): this could be done better with a demon_w32_read_memory
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
      if (demon_w32_read_memory(process_handle, block, read_p, cap)){
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

internal String16
demon_w32_read_memory_str16(Arena *arena, HANDLE process_handle, U64 address){
  // TODO(allen): this could be done better with a demon_w32_read_memory
  // that returns a read amount instead of a success/fail.
  
  // scan piece by piece
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  U64 max_cap = 256;
  U64 cap = max_cap;
  U64 read_p = address;
  for (;;){
    U8 *block = push_array(scratch.arena, U8, cap);
    for (;cap > 1;){
      if (demon_w32_read_memory(process_handle, block, read_p, cap)){
        break;
      }
      cap /= 2;
    }
    read_p += cap;
    
    U16 *block16 = (U16*)block;
    (void)block16;
    U64 block_opl = 0;
    for (;block_opl < cap; block_opl += 2){
      if (*(U16*)(block + block_opl) == 0){
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
  String8 joined = str8_list_join(arena, &list, 0);
  String16 result = {(U16*)joined.str, joined.size/2};
  scratch_end(scratch);
  return(result);
}

internal DEMON_W32_ImageInfo
demon_w32_image_info_from_base(HANDLE process_handle, U64 base){
  // find pe offset in dos header
  U32 pe_offset = 0;
  
  {
    U64 dos_magic_off = base;
    U16 dos_magic = 0;
    demon_w32_read_struct(process_handle, &dos_magic, dos_magic_off);
    if (dos_magic == DEMON_DOS_MAGIC){
      U64 pe_offset_off = base + OffsetOf(DEMON_DosHeader, coff_file_offset);
      demon_w32_read_struct(process_handle, &pe_offset, pe_offset_off);
    }
  }
  
  // get coff header
  B32 got_coff_header = 0;
  U64 coff_header_off = 0;
  DEMON_CoffHeader coff_header = {0};
  
  if (pe_offset > 0){
    U64 pe_magic_off = base + pe_offset;
    U32 pe_magic = 0;
    demon_w32_read_struct(process_handle, &pe_magic, pe_magic_off);
    if (pe_magic == DEMON_PE_MAGIC){
      coff_header_off = pe_magic_off + sizeof(pe_magic);
      if (demon_w32_read_struct(process_handle, &coff_header, coff_header_off)){
        got_coff_header = 1;
      }
    }
  }
  
  // get arch and size
  DEMON_W32_ImageInfo result = zero_struct;
  if (got_coff_header){
    U64 optional_size_off = 0;
    
    Architecture arch = Architecture_Null;
    switch (coff_header.machine){
      case DEMON_CoffMachineType_X86:
      {
        arch = Architecture_x86;
        optional_size_off = OffsetOf(DEMON_PeOptionalHeader32, sizeof_image);
      }break;
      
      case DEMON_CoffMachineType_X64:
      {
        arch = Architecture_x64;
        optional_size_off = OffsetOf(DEMON_PeOptionalHeader32Plus, sizeof_image);
      }break;
      
      default:
      {}break;
    }
    
    if (arch != Architecture_Null){
      U64 optional_off = coff_header_off + sizeof(coff_header);
      U32 size = 0;
      if (demon_w32_read_struct(process_handle, &size, optional_off + optional_size_off)){
        result.arch = arch;
        result.size = size;
      }
    }
  }
  
  return(result);
}

internal DWORD
demon_w32_inject_thread(DEMON_Entity *process, U64 start_address){
  LPTHREAD_START_ROUTINE start = (LPTHREAD_START_ROUTINE)start_address;
  DEMON_Entity *entity = process;
  DEMON_W32_Ext *process_ext = demon_w32_ext(entity);
  DWORD thread_id = 0;
  HANDLE thread = CreateRemoteThread(process_ext->proc.handle, 0, 0, start, 0, 0, &thread_id);
  if (thread != 0){
    CloseHandle(thread);
  }
  return(thread_id);
}

internal U16
demon_w32_real_tag_word_from_xsave(XSAVE_FORMAT *fxsave){
  U16 result = 0;
  U32 top = (fxsave->StatusWord >> 11) & 7;
  for (U32 fpr = 0; fpr < 8; fpr += 1){
    U32 tag = 3;
    if (fxsave->TagWord & (1 << fpr)){
      U32 st = (fpr - top)&7;
      
      REGS_Reg80 *fp = (REGS_Reg80*)&fxsave->FloatRegisters[st*16];
      U16 exponent = fp->sign1_exp15 & bitmask15;
      U64 integer_part  = fp->int1_frac63 >> 63;
      U64 fraction_part = fp->int1_frac63 & bitmask63;
      
      // tag: 0 - normal; 1 - zero; 2 - special
      tag = 2;
      if (exponent == 0){
        if (integer_part == 0 && fraction_part == 0){
          tag = 1;
        }
      }
      else if (exponent != bitmask15 && integer_part != 0){
        tag = 0;
      }
    }
    result |= tag << (2 * fpr);
  }
  return(result);
}

internal U16
demon_w32_xsave_tag_word_from_real_tag_word(U16 ftw){
  U16 compact = 0;
  for (U32 fpr = 0; fpr < 8; fpr++){
    U32 tag = (ftw >> (fpr * 2)) & 3;
    if (tag != 3){
      compact |= (1 << fpr);
    }
  }
  return(compact);
}

internal DWORD
demon_w32_win32_from_memory_protect_flags(DEMON_MemoryProtectFlags flags){
  DWORD result = 0;
  switch (flags){
    default:
    case DEMON_MemoryProtectFlag_Read|DEMON_MemoryProtectFlag_Write:
    case DEMON_MemoryProtectFlag_Write:
    {
      result = PAGE_READWRITE;
    }break;
    case DEMON_MemoryProtectFlag_Read|DEMON_MemoryProtectFlag_Write|DEMON_MemoryProtectFlag_Execute:
    {
      result = PAGE_EXECUTE_READWRITE;
    }break;
    case DEMON_MemoryProtectFlag_Execute:
    {
      result = PAGE_EXECUTE;
    }break;
    case DEMON_MemoryProtectFlag_Read:
    {
      result = PAGE_READONLY;
    }break;
  }
  return(result);
}

////////////////////////////////
//~ rjf: Experiments

internal void
demon_w32_peak_at_tls(DEMON_Handle thread_handle){
  DEMON_Entity *thread = demon_ent_ptr_from_handle(thread_handle);
  if (thread != 0 && thread->kind == DEMON_EntityKind_Thread){
    
    DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
    U64 tlb = thread_ext->thread.thread_local_base;
    
    U8 buffer[0x1000];
    demon_os_read_memory(thread->parent, buffer, tlb, 0x1000);
    
    int x = 0;
    (void)x;
  }
}

////////////////////////////////
//~ rjf: @demon_os_hooks Main Layer Initialization

internal void
demon_os_init(void){
  demon_w32_ext_arena = arena_alloc();
  demon_w32_detach_proc_arena = arena_alloc();
  
  // rjf: load Windows 10+ GetThreadDescription API
  {
    demon_w32_GetThreadDescription = (GetThreadDescriptionFunctionType *)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "GetThreadDescription");
  }
  
  // rjf: setup environment variables
  {
    CHAR *this_proc_env = GetEnvironmentStrings();
    U64 start_idx = 0;
    for(U64 idx = 0;; idx += 1)
    {
      if(this_proc_env[idx] == 0)
      {
        if(start_idx == idx)
        {
          break;
        }
        else
        {
          String8 string = str8((U8 *)this_proc_env + start_idx, idx - start_idx);
          str8_list_push(demon_w32_ext_arena, &demon_w32_environment, string);
          start_idx = idx+1;
        }
      }
    }
  }
}

////////////////////////////////
//~ rjf: @demon_os_hooks Running/Halting

internal DEMON_EventList
demon_os_run(Arena *arena, DEMON_OS_RunCtrls *ctrls){
  DEMON_EventList result = {0};
  
  if (demon_ent_root == 0){
    DEMON_Event *event = demon_push_event(arena, &result, DEMON_EventKind_Error);
    event->error_kind = DEMON_ErrorKind_NotInitialized;
  }
  else if (demon_ent_root->first == 0 && !demon_w32_new_process_pending){
    DEMON_Event *event = demon_push_event(arena, &result, DEMON_EventKind_Error);
    event->error_kind = DEMON_ErrorKind_NotAttached;
  }
  else if(demon_w32_first_detached_proc != 0)
  {
    for(DEMON_EntityNode *n = demon_w32_first_detached_proc; n != 0; n = n->next)
    {
      DEMON_Entity *process = n->entity;
      
      // rjf: push exit thread events
      for(DEMON_Entity *child = process->first; child != 0; child = child->next)
      {
        if(child->kind == DEMON_EntityKind_Thread)
        {
          DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_ExitThread);
          e->process = demon_ent_handle_from_ptr(process);
          e->thread = demon_ent_handle_from_ptr(child);
        }
      }
      
      // rjf: push unload module events
      for(DEMON_Entity *child = process->first; child != 0; child = child->next)
      {
        if(child->kind == DEMON_EntityKind_Module)
        {
          DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_UnloadModule);
          e->process = demon_ent_handle_from_ptr(process);
          e->module = demon_ent_handle_from_ptr(child);
          e->string = demon_os_full_path_from_module(arena, child);
        }
      }
      
      // rjf: push exit process event
      {
        DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_ExitProcess);
        e->process = demon_ent_handle_from_ptr(process);
        e->code = 0;
      }
      
      // rjf: free process
      demon_ent_release_root_and_children(process);
    }
    demon_w32_first_detached_proc = 0;
    demon_w32_last_detached_proc = 0;
    arena_clear(demon_w32_detach_proc_arena);
  }
  else{
    Temp scratch = scratch_begin(&arena, 1);
    
    // get the single step thread (if any)
    DEMON_Entity *single_step_thread = ctrls->single_step_thread;
    
    // TODO(allen): dedup per architecture?
    // set single step bit
    if (single_step_thread != 0){
      // TODO(allen): possibly buggy
      switch(single_step_thread->arch)
      {
        default:{NotImplemented;}break;
        case Architecture_x86:
        {
          REGS_RegBlockX86 regs = {0};
          demon_os_read_regs_x86(single_step_thread, &regs);
          regs.eflags.u32 |= 0x100;
          demon_os_write_regs_x86(single_step_thread, &regs);
        }break;
        
        case Architecture_x64:
        {
          REGS_RegBlockX64 regs = {0};
          demon_os_read_regs_x64(single_step_thread, &regs);
          regs.rflags.u64 |= 0x100;
          demon_os_write_regs_x64(single_step_thread, &regs);
        }break;
      }
    }
    
    // TODO(allen): per-Architecture implementation of traps
    // set traps
    U8 *trap_swap_bytes = push_array_no_zero(scratch.arena, U8, ctrls->trap_count);
    
    {
      DEMON_OS_Trap *trap = ctrls->traps;
      for (U64 i = 0; i < ctrls->trap_count; i += 1, trap += 1){
        if (demon_os_read_memory(trap->process, trap_swap_bytes + i, trap->address, 1)){
          U8 int3 = 0xCC;
          demon_os_write_memory(trap->process, trap->address, &int3, 1);
        }
        else{
          trap_swap_bytes[i] = 0xCC;
        }
      }
    }
    
    // determine how to resume from the last event
    DWORD resume_code = DBG_CONTINUE;
    if (demon_w32_exception_not_handled){
      if (!ctrls->ignore_previous_exception){
        resume_code = DBG_EXCEPTION_NOT_HANDLED;
      }
    }
    demon_w32_exception_not_handled = 0;
    
    // list threads that run this time
    DEMON_W32_EntityNode *first_run_thread = 0;
    DEMON_W32_EntityNode *last_run_thread = 0;
    
    for(DEMON_Entity *process = demon_ent_root->first;
        process != 0;
        process = process->next){
      if (process->kind == DEMON_EntityKind_Process){
        
        // determine if this process is frozen
        B32 process_is_frozen = 0;
        if (ctrls->run_entities_are_processes){
          for (U64 i = 0; i < ctrls->run_entity_count; i += 1){
            if (ctrls->run_entities[i] == process){
              process_is_frozen = 1;
              break;
            }
          }
        }
        
        for (DEMON_Entity *thread = process->first;
             thread != 0;
             thread = thread->next){
          if (thread->kind == DEMON_EntityKind_Thread){
            // determine if this thread is frozen
            B32 is_frozen = 0;
            
            if (ctrls->single_step_thread != 0 &&
                ctrls->single_step_thread != thread){
              is_frozen = 1;
            }
            else{
              
              if (ctrls->run_entities_are_processes){
                is_frozen = process_is_frozen;
              }
              else{
                for (U64 i = 0; i < ctrls->run_entity_count; i += 1){
                  if (ctrls->run_entities[i] == thread){
                    is_frozen = 1;
                    break;
                  }
                }
              }
              
              if (ctrls->run_entities_are_unfrozen){
                is_frozen = !is_frozen;
              }
            }
            
            // rjf: disregard all other rules if this is the halter thread
            if(demon_w32_halter_thread_id == thread->id)
            {
              is_frozen = 0;
            }
            
            // add this thread to the list
            if (!is_frozen){
              DEMON_W32_EntityNode *node = push_array_no_zero(scratch.arena, DEMON_W32_EntityNode, 1);
              SLLQueuePush(first_run_thread, last_run_thread, node);
              node->entity = thread;
            }
          }
        }
      }
    }
    
    // prep suspension state of threads that will be allowed to run
    for(DEMON_W32_EntityNode *node = first_run_thread;
        node != 0;
        node = node->next)
    {
      DEMON_Entity *thread = node->entity;
      DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
      DWORD resume_result = ResumeThread(thread_ext->thread.handle);
      if(resume_result == max_U32)
      {
        // TODO(allen): Error. Unknown cause (do GetLastError, FromatMessage)
      }
      else
      {
        DWORD desired_counter = 0;
        DWORD current_counter = resume_result - 1;
        if(current_counter != desired_counter)
        {
          // NOTE(rjf): Warning. The user has manually suspended this thread,
          // so even though from Demon's perspective it thinks this thread
          // should run, it will not, because the user has manually called
          // SuspendThread or used CREATE_SUSPENDED or whatever.
        }
      }
    }
    
    // rjf: if run threads are marked as having reported an explicit trap
    // on their last run, shift their RIPs past that trap instruction, so
    // that they may continue
    for(DEMON_W32_EntityNode *node = first_run_thread;
        node != 0;
        node = node->next)
    {
      DEMON_Entity *thread = node->entity;
      DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
      if(thread_ext->thread.last_run_reported_trap)
      {
        Temp temp = temp_begin(scratch.arena);
        U64 regs_block_size = regs_block_size_from_architecture(thread->arch);
        void *regs_block = push_array(temp.arena, U8, regs_block_size);
        B32 good = demon_os_read_regs(thread, regs_block);
        U64 pre_rip = regs_rip_from_arch_block(thread->arch, regs_block);
        if(good && pre_rip == thread_ext->thread.last_run_reported_trap_pre_rip)
        {
          regs_arch_block_write_rip(thread->arch, regs_block, thread_ext->thread.last_run_reported_trap_post_rip);
          demon_os_write_regs(thread, regs_block);
        }
        temp_end(temp);
        thread_ext->thread.last_run_reported_trap = 0;
        thread_ext->thread.last_run_reported_trap_post_rip = 0;
      }
    }
    
    // send last saved continue signal
    B32 good_state = 1;
    if (demon_w32_resume_needed != 0){
      if (!ContinueDebugEvent(demon_w32_resume_pid, demon_w32_resume_tid, resume_code)){
        good_state = 0;
      }
      
      demon_w32_resume_needed = 0;
      demon_w32_resume_pid = 0;
      demon_w32_resume_tid = 0;
    }
    
    // wait for a new event from targets
    DEBUG_EVENT evt = {0};
    B32 got_new_event = 0;
    if (good_state){
      got_new_event = WaitForDebugEvent(&evt, INFINITE);
    }
    if (got_new_event){
      demon_w32_resume_needed = 1;
      demon_w32_resume_pid = evt.dwProcessId;
      demon_w32_resume_tid = evt.dwThreadId;
    }
    
    // increment demon time
    demon_time += 1;
    
    // reset all threads to paused state
    if (got_new_event){
      for (DEMON_W32_EntityNode *node = first_run_thread;
           node != 0;
           node = node->next){
        DEMON_Entity *thread = node->entity;
        DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
        DWORD suspend_result = SuspendThread(thread_ext->thread.handle);
        if (suspend_result == max_U32){
          // TODO(allen): Error. Unknown cause (do GetLastError, FromatMessage)
          // NOTE(allen): This can happen when the event is EXIT_THREAD_DEBUG_EVENT
          // or EXIT_PROCESS_DEBUG_EVENT. After such an event SuspendThread
          // gives error code 5 (access denied). This has no adverse effects, but if
          // want to start reporting errors we should take care to avoid calling
          // SuspendThread in that case.
        }
        else{
          DWORD desired_counter = 1;
          DWORD current_counter = suspend_result + 1;
          if (current_counter != desired_counter){
            // NOTE(rjf): Warning. We've suspended to something higher than 1.
            // In this case, it means the user probably created the thread in
            // a suspended state, or they called SuspendThread.
          }
        }
      }
    }
    
    // rjf: process event
    if (got_new_event){
      got_new_event = 1;
      
      switch (evt.dwDebugEventCode){
        case CREATE_PROCESS_DEBUG_EVENT:
        {
          // pull outs
          HANDLE process_handle = evt.u.CreateProcessInfo.hProcess;
          HANDLE thread_handle = evt.u.CreateProcessInfo.hThread;
          U64 module_base = (U64)evt.u.CreateProcessInfo.lpBaseOfImage;
          DEMON_W32_ImageInfo image_info = demon_w32_image_info_from_base(process_handle, module_base);
          
          // init process entity
          DEMON_Entity *process = demon_ent_new(demon_ent_root, DEMON_EntityKind_Process, evt.dwProcessId);
          demon_proc_count += 1;
          process->arch = image_info.arch;
          DEMON_W32_Ext *process_ext = demon_w32_ext_alloc();
          process->ext = process_ext;
          process_ext->proc.handle = process_handle;
          
          demon_w32_new_process_pending = 0;
          
          // init new associated entities
          DEMON_Entity *thread = demon_ent_new(process, DEMON_EntityKind_Thread, evt.dwThreadId);
          demon_thread_count += 1;
          DEMON_W32_Ext *thread_ext = demon_w32_ext_alloc();
          thread->ext = thread_ext;
          thread_ext->thread.handle = thread_handle;
          thread_ext->thread.thread_local_base = (U64)evt.u.CreateProcessInfo.lpThreadLocalBase;
          SuspendThread(thread_ext->thread.handle);
          
          DEMON_Entity *module = demon_ent_new(process, DEMON_EntityKind_Module, module_base);
          demon_module_count += 1;
          module->addr_range_dim = image_info.size;
          DEMON_W32_Ext *module_ext = demon_w32_ext_alloc();
          module->ext = module_ext;
          module_ext->module.handle = evt.u.CreateProcessInfo.hFile;
          module_ext->module.address_of_name_pointer = (U64)evt.u.CreateProcessInfo.lpImageName;
          module_ext->module.is_main = 1;
          module_ext->module.name_is_unicode = (evt.u.CreateProcessInfo.fUnicode != 0);
          
          // injection memory
          {
            U8 injection_code[DEMON_W32_INJECTED_CODE_SIZE];
            injection_code[0] = 0xC3;
            for (U64 i = 1; i < DEMON_W32_INJECTED_CODE_SIZE; i += 1){
              injection_code[i] = 0xCC;
            }
            
            U64 injection_size = DEMON_W32_INJECTED_CODE_SIZE + sizeof(DEMON_W32_InjectedBreak);
            U64 injection_address = (U64)VirtualAllocEx(process_ext->proc.handle, 0, injection_size,
                                                        MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE);
            demon_os_write_memory(process, injection_address, injection_code, sizeof(injection_code));
            process_ext->proc.injection_address = injection_address;
          }
          
          // generate events
          {
            DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_CreateProcess);
            e->process = demon_ent_handle_from_ptr(process);
            e->code = evt.dwProcessId;
          }
          
          {
            DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_CreateThread);
            e->process = demon_ent_handle_from_ptr(process);
            e->thread = demon_ent_handle_from_ptr(thread);
            e->code = evt.dwThreadId;
          }
          
          {
            DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_LoadModule);
            e->process = demon_ent_handle_from_ptr(process);
            e->module = demon_ent_handle_from_ptr(module);
            e->address = module_base;
            e->size = image_info.size;
            e->string = demon_os_full_path_from_module(arena, module);
          }
        }break;
        
        case EXIT_PROCESS_DEBUG_EVENT:
        {
          // get process entity
          DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
          
          if (process != 0){
            // update halter process pointer
            if (process == demon_w32_halter_process){
              demon_w32_halter_process = 0;
            }
            
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
                e->string = demon_os_full_path_from_module(arena, entity);
              }
            }
            
            // generate event for self
            DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_ExitProcess);
            e->process = demon_ent_handle_from_ptr(process);
            e->code = evt.u.ExitProcess.dwExitCode;
            
            // free entity
            demon_ent_release_root_and_children(process);
            
            // rjf: detach
            {
              DWORD pid = evt.dwProcessId;
              DebugActiveProcessStop(pid);
            }
          }
        }break;
        
        case CREATE_THREAD_DEBUG_EVENT:
        {
          // get process entity
          DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
          
          if (process != 0){
            // init new entity
            DEMON_Entity *thread = demon_ent_new(process, DEMON_EntityKind_Thread, evt.dwThreadId);
            demon_thread_count += 1;
            DEMON_W32_Ext *thread_ext = demon_w32_ext_alloc();
            thread->ext = thread_ext;
            thread_ext->thread.handle = evt.u.CreateThread.hThread;
            thread_ext->thread.thread_local_base = (U64)evt.u.CreateThread.lpThreadLocalBase;
            DWORD sus_result = SuspendThread(thread_ext->thread.handle);
            (void)sus_result;
            
            // rjf: unpack thread name
            String8 thread_name = {0};
            if(demon_w32_GetThreadDescription != 0)
            {
              WCHAR *thread_name_w = 0;
              HRESULT hr = demon_w32_GetThreadDescription(evt.u.CreateThread.hThread, &thread_name_w);
              if(SUCCEEDED(hr))
              {
                thread_name = str8_from_16(arena, str16_cstring((U16 *)thread_name_w));
                LocalFree(thread_name_w);
              }
            }
            
            // rjf: determine if this is the halter thread
            B32 is_halter = (evt.dwThreadId == demon_w32_halter_thread_id);
            
            // generate event for non-halters
            if (is_halter == 0){
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_CreateThread);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
              e->code = evt.dwThreadId;
              e->string = thread_name;
            }
          }
        }break;
        
        case EXIT_THREAD_DEBUG_EVENT:
        {
          // get thread and process entity
          DEMON_Entity *thread = demon_ent_map_entity_from_id(DEMON_EntityKind_Thread, evt.dwThreadId);
          
          if (thread != 0){
            DEMON_Entity *process = thread->parent;
            
            // rjf: determine if this is the halter thread
            B32 is_halter = (evt.dwThreadId == demon_w32_halter_thread_id);
            
            // rjf: generate a halt event if we're the halter
            if (is_halter){
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_Halt);
              demon_w32_halter_process = 0;
              (void)e;
            }
            
            // rjf: generate normal thread exit event on non-halter-thread exist
            if (!is_halter){
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_ExitThread);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
              e->code = evt.u.ExitThread.dwExitCode;
            }
            
            // free entity
            demon_ent_release_root_and_children(thread);
          }
        }break;
        
        case LOAD_DLL_DEBUG_EVENT:
        {
          // get process entity
          DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
          
          if (process != 0){
            // get image info
            DEMON_W32_Ext *process_ext = demon_w32_ext(process);
            HANDLE process_handle = process_ext->proc.handle;
            U64 module_base = (U64)evt.u.LoadDll.lpBaseOfDll;
            DEMON_W32_ImageInfo image_info = demon_w32_image_info_from_base(process_handle, module_base);
            
            // init new entity
            DEMON_Entity *module = demon_ent_new(process, DEMON_EntityKind_Module, module_base);
            module->addr_range_dim = image_info.size;
            demon_module_count += 1;
            DEMON_W32_Ext *module_ext = demon_w32_ext_alloc();
            module->ext = module_ext;
            module_ext->module.handle = evt.u.LoadDll.hFile;
            module_ext->module.address_of_name_pointer = (U64)evt.u.LoadDll.lpImageName;
            module_ext->module.is_main = 0;
            module_ext->module.name_is_unicode = (evt.u.LoadDll.fUnicode != 0);
            
            // generate events
            {
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_LoadModule);
              e->process = demon_ent_handle_from_ptr(process);
              e->module = demon_ent_handle_from_ptr(module);
              e->address = module_base;
              e->size = image_info.size;
              e->string = demon_os_full_path_from_module(arena, module);
            }
          }
        }break;
        
        case UNLOAD_DLL_DEBUG_EVENT:
        {
          // get module and process entity
          U64 module_base = (U64)evt.u.UnloadDll.lpBaseOfDll;
          DEMON_Entity *module  = demon_ent_map_entity_from_id(DEMON_EntityKind_Module, module_base);
          
          if (module != 0){
            DEMON_Entity *process = module->parent;
            
            // generate events
            {
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_UnloadModule);
              e->process = demon_ent_handle_from_ptr(process);
              e->module = demon_ent_handle_from_ptr(module);
              e->string = demon_os_full_path_from_module(arena, module);
            }
            
            // free entity
            demon_ent_release_root_and_children(module);
          }
        }break;
        
        case EXCEPTION_DEBUG_EVENT:
        {
          // NOTE(rjf): Notes on multithreaded breakpoint events
          // (2021/11/1):
          //
          // When many threads are simultaneously running, multiple threads
          // may hit a trap "at the same time". When this happens there will be
          // multiple events in an internal queue that we cannot see. If there
          // is another event in the queue we will not see it until we call
          // ContinueDebugEvent again, in a subsequent call to demon_os_run.
          //
          // When we get a trap event, the instruction pointer stored
          // in the event will have the address of the int 3 instruction that
          // was hit. Our RIP register, however, will be one byte past that.
          // So, to get the behavior we want, we need to set the RIP register
          // back to the address of the int 3.
          //
          // To deal with the fact that we may get breakpoint events later that
          // were actually from this run what we do is:
          //
          // #1. If we get a trap event, and it corresponds to a user submitted
          //     trap, then we treat it is a breakpoint event.
          // #2. If we get a trap event, and it does NOT correspond to a user
          //     trap in this call:
          //   #A. If the actual unmodified instruction byte is NOT an int 3,
          //       then this is a queued event from a previous run that is no
          //       longer applicable and we skip it.
          //   #B. If the actual unmodified instruction is an int 3, then this
          //       becomes a trap event and we do not reset RIP.
          
          // get thread and process entity
          DEMON_Entity *thread = demon_ent_map_entity_from_id(DEMON_EntityKind_Thread, evt.dwThreadId);
          
          if (thread != 0){
            DEMON_Entity *process = thread->parent;
            Assert(process->kind == DEMON_EntityKind_Process);
            
            DEMON_W32_Ext *process_ext = demon_w32_ext(process);
            
            // TODO(allen): the exception record has two forms, one for 32 bit and one for 64 bit mode.
            
            EXCEPTION_DEBUG_INFO *edi = &evt.u.Exception;
            EXCEPTION_RECORD *exception = &edi->ExceptionRecord;
            U64 instruction_pointer = (U64)exception->ExceptionAddress;
            
            // check if first BP
            B32 first_bp = 0;
            if (exception->ExceptionCode == DEMON_W32_EXCEPTION_BREAKPOINT &&
                !process_ext->proc.did_first_bp){
              process_ext->proc.did_first_bp = 1;
              first_bp = 1;
            }
            
            // rjf: check if trap
            B32 is_trap = (!first_bp &&
                           (exception->ExceptionCode == DEMON_W32_EXCEPTION_BREAKPOINT ||
                            exception->ExceptionCode == DEMON_W32_EXCEPTION_STACK_BUFFER_OVERRUN));
            
            // rjf: check if this trap is currently registered
            B32 hit_user_trap = 0;
            if (is_trap){
              DEMON_OS_Trap *trap = ctrls->traps;
              for (U64 i = 0; i < ctrls->trap_count; i += 1, trap += 1){
                if (trap->process == process && trap->address == instruction_pointer){
                  hit_user_trap = 1;
                  break;
                }
              }
            }
            
            // rjf: check if trap is explicit in the actual code memory
            B32 hit_explicit_trap = 0;
            if (is_trap && !hit_user_trap){
              U8 instruction_byte = 0;
              if (demon_os_read_memory(process, &instruction_byte, instruction_pointer, 1)){
                // TODO(rjf): x86/x64 specific check
                // TODO(rjf): do we need to check to make sure the instruction
                // pointer has not changed?
                hit_explicit_trap = (instruction_byte == 0xCC ||
                                     instruction_byte == 0xCD);
              }
            }
            
            // rjf: determine whether to roll back instruction pointer
            B32 rollback = (is_trap);
            
            // rjf: roll back
            U64 post_trap_rip = 0;
            if(rollback)
            {
              Temp temp = temp_begin(scratch.arena);
              U64 regs_block_size = regs_block_size_from_architecture(thread->arch);
              void *regs_block = push_array(scratch.arena, U8, regs_block_size);
              if(demon_os_read_regs(thread, regs_block))
              {
                post_trap_rip = regs_rip_from_arch_block(thread->arch, regs_block);
                regs_arch_block_write_rip(thread->arch, regs_block, instruction_pointer);
                demon_os_write_regs(thread, regs_block);
              }
              temp_end(temp);
            }
            
            // allen: if this is not a user trap or explicit trap it's a previous trap
            B32 hit_previous_trap = (is_trap && !hit_user_trap && !hit_explicit_trap);
            
            // determine whether to skip this event
            B32 skip_event = (hit_previous_trap);
            
            // emit event
            if(!skip_event)
            {
              // rjf: fill top-level info
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_Exception);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
              e->code = exception->ExceptionCode;
              e->flags = exception->ExceptionFlags;
              e->instruction_pointer = (U64)exception->ExceptionAddress;
              
              // rjf: explicit trap -> mark this thread as having reported this trap
              if(hit_explicit_trap)
              {
                DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
                thread_ext->thread.last_run_reported_trap = 1;
                thread_ext->thread.last_run_reported_trap_pre_rip = instruction_pointer;
                thread_ext->thread.last_run_reported_trap_post_rip = post_trap_rip;
              }
              
              // rjf: fill by exception code
              switch (exception->ExceptionCode){
                case DEMON_W32_EXCEPTION_BREAKPOINT:
                {
                  // rjf: determine event kind
                  DEMON_EventKind report_event_kind = DEMON_EventKind_Trap;
                  if (first_bp){
                    report_event_kind = DEMON_EventKind_HandshakeComplete;
                  }
                  else if (hit_user_trap){
                    report_event_kind = DEMON_EventKind_Breakpoint;
                  }
                  
                  // set event kind
                  e->kind = report_event_kind;
                }break;
                
                case DEMON_W32_EXCEPTION_STACK_BUFFER_OVERRUN:
                {
                  e->kind = DEMON_EventKind_Trap;
                }break;
                
                case DEMON_W32_EXCEPTION_SINGLE_STEP:
                {
                  e->kind = DEMON_EventKind_SingleStep;
                }break;
                
                case DEMON_W32_EXCEPTION_THROW:
                {
                  U64 exception_sp = 0;
                  U64 exception_ip = 0;
                  if (exception->NumberParameters >= 3){
                    exception_sp = (U64)exception->ExceptionInformation[1];
                    exception_ip = (U64)exception->ExceptionInformation[2];
                  }
                  e->stack_pointer = exception_sp;
                  e->exception_kind = DEMON_ExceptionKind_CppThrow;
                  e->exception_repeated = (edi->dwFirstChance == 0);
                  demon_w32_exception_not_handled = (edi->dwFirstChance != 0);
                }break;
                
                case DEMON_W32_EXCEPTION_ACCESS_VIOLATION:
                case DEMON_W32_EXCEPTION_IN_PAGE_ERROR:
                {
                  U64 exception_address = 0;
                  DEMON_ExceptionKind exception_kind = DEMON_ExceptionKind_Null;
                  if (exception->NumberParameters >= 2){
                    switch (exception->ExceptionInformation[0]){
                      case 0: exception_kind = DEMON_ExceptionKind_MemoryRead;    break;
                      case 1: exception_kind = DEMON_ExceptionKind_MemoryWrite;   break;
                      case 8: exception_kind = DEMON_ExceptionKind_MemoryExecute; break;
                    }
                    exception_address = exception->ExceptionInformation[1];
                  }
                  
                  e->address = exception_address;
                  e->exception_kind = exception_kind;
                  e->exception_repeated = (edi->dwFirstChance == 0);
                  demon_w32_exception_not_handled = (edi->dwFirstChance != 0);
                }break;
                
                case DEMON_W32_EXCEPTION_SET_THREAD_NAME:
                {
                  if(exception->NumberParameters >= 2)
                  {
                    U64 thread_name_address = exception->ExceptionInformation[1];
                    DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
                    String8List thread_name_strings = {0};
                    {
                      U64 read_addr = thread_name_address;
                      U64 total_string_size = 0;
                      for(;total_string_size < KB(4);)
                      {
                        U8 *buffer = push_array_no_zero(scratch.arena, U8, 256);
                        B32 good_read = demon_os_read_memory(process, buffer, read_addr, 256);
                        if(good_read)
                        {
                          U64 size = 256;
                          for(U64 idx = 0; idx < 256; idx += 1)
                          {
                            if(buffer[idx] == 0)
                            {
                              size = idx;
                              break;
                            }
                          }
                          String8 string_part = str8(buffer, size);
                          str8_list_push(scratch.arena, &thread_name_strings, string_part);
                          total_string_size += size;
                          read_addr += size;
                          if(size < 256)
                          {
                            break;
                          }
                        }
                      }
                    }
                    e->kind = DEMON_EventKind_SetThreadName;
                    e->string = str8_list_join(arena, &thread_name_strings, 0);
                    if(exception->NumberParameters > 2)
                    {
                      e->code = exception->ExceptionInformation[2];
                    }
                  }
                }break;
                
                default:
                {
                  demon_w32_exception_not_handled = (edi->dwFirstChance != 0);
                }break;
              }
            }
          }
        }break;
        
        case OUTPUT_DEBUG_STRING_EVENT:
        {
          // get process entity
          DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
          DEMON_Entity *thread = demon_ent_map_entity_from_id(DEMON_EntityKind_Thread, evt.dwThreadId);
          
          U64 string_address = (U64)evt.u.DebugString.lpDebugStringData;
          U64 string_size = (U64)evt.u.DebugString.nDebugStringLength;
          
          // TODO(allen): is the string in UTF-8 or UTF-16?
          
          U8 *buffer = push_array_no_zero(arena, U8, string_size + 1);
          demon_os_read_memory(process, buffer, string_address, string_size);
          buffer[string_size] = 0;
          
          DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_DebugString);
          e->process = demon_ent_handle_from_ptr(process);
          e->thread = demon_ent_handle_from_ptr(thread);
          e->string = str8(buffer, string_size);
          if(string_size != 0 && buffer[string_size-1] == 0)
          {
            e->string.size -= 1;
          }
        }break;
        
        case RIP_EVENT:
        {
          DEMON_Entity *process = demon_ent_map_entity_from_id(DEMON_EntityKind_Process, evt.dwProcessId);
          DEMON_Entity *thread = demon_ent_map_entity_from_id(DEMON_EntityKind_Thread, evt.dwThreadId);
          
          // TODO(allen): this is not the right way to handle this event
          DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_Exception);
          e->process = demon_ent_handle_from_ptr(process);
          e->thread = demon_ent_handle_from_ptr(thread);
          e->flags   = 0;
          e->address = 0;
        }break;
        
        default:
        {
          got_new_event = 0;
        }break;
      }
    }
    
    //- rjf: gather new thread-names
    if(demon_w32_GetThreadDescription != 0)
    {
      for(DEMON_Entity *process = demon_ent_root->first;
          process != 0;
          process = process->next)
      {
        if(process->kind != DEMON_EntityKind_Process) { continue; }
        for(DEMON_Entity *thread = process->first;
            thread != 0;
            thread = thread->next)
        {
          if(thread->kind != DEMON_EntityKind_Thread) { continue; }
          DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
          if(thread_ext->thread.last_name_hash == 0 ||
             thread_ext->thread.name_gather_time_us+1000000 <= os_now_microseconds())
          {
            String8 name = {0};
            {
              WCHAR *thread_name_w = 0;
              HRESULT hr = demon_w32_GetThreadDescription(thread_ext->thread.handle, &thread_name_w);
              if(SUCCEEDED(hr))
              {
                name = str8_from_16(scratch.arena, str16_cstring((U16 *)thread_name_w));
                LocalFree(thread_name_w);
              }
            }
            U64 name_hash = demon_w32_hash_from_string(name);
            if(name.size != 0 && name_hash != thread_ext->thread.last_name_hash)
            {
              DEMON_Event *e = demon_push_event(arena, &result, DEMON_EventKind_SetThreadName);
              e->process = demon_ent_handle_from_ptr(process);
              e->thread = demon_ent_handle_from_ptr(thread);
              e->string = push_str8_copy(arena, name);
            }
            thread_ext->thread.name_gather_time_us = os_now_microseconds();
            thread_ext->thread.last_name_hash = name_hash;
          }
        }
      }
    }
    
    // TODO(allen): handle errors? if (!got_new_event) ? if (!good_state) ?
    
    // TODO(allen): per-Architecture
    // unset traps
    {
      DEMON_OS_Trap *trap = ctrls->traps;
      for (U64 i = 0; i < ctrls->trap_count; i += 1, trap += 1){
        U8 og_byte = trap_swap_bytes[i];
        if (og_byte != 0xCC){
          demon_os_write_memory(trap->process, trap->address, &og_byte, 1);
        }
      }
    }
    
    // TODO(allen): per-Architecture
    // unset single step bit
    //  the single step bit is automatically unset whenever we single step
    //  but if *something else* happened, it will still be there ready to
    //  confound us later; so here we're just being sure it's taken out.
    if (single_step_thread != 0){
      // TODO(allen): possibly buggy
      switch(single_step_thread->arch)
      {
        default:{NotImplemented;}break;
        case Architecture_x86:
        {
          REGS_RegBlockX86 regs = {0};
          demon_os_read_regs_x86(single_step_thread, &regs);
          regs.eflags.u32 &= ~0x100;
          demon_os_write_regs_x86(single_step_thread, &regs);
        }break;
        
        case Architecture_x64:
        {
          REGS_RegBlockX64 regs = {0};
          demon_os_read_regs_x64(single_step_thread, &regs);
          regs.rflags.u64 &= ~0x100;
          demon_os_write_regs_x64(single_step_thread, &regs);
        }break;
      }
    }
    
    scratch_end(scratch);
  }
  
  return(result);
}

internal void
demon_os_halt(U64 code, U64 user_data){
  if (demon_ent_root != 0 && demon_w32_halter_process == 0){
    DEMON_Entity *process = demon_ent_root->first;
    if (process != 0){
      DEMON_W32_Ext *process_ext = demon_w32_ext(process);
      
      demon_w32_halter_process = process;
      DEMON_W32_InjectedBreak injection = {code, user_data};
      U64 data_injection_address = process_ext->proc.injection_address + DEMON_W32_INJECTED_CODE_SIZE;
      demon_os_write_struct(process, data_injection_address, &injection);
      demon_w32_halter_thread_id = demon_w32_inject_thread(process, process_ext->proc.injection_address);
    }
  }
}

////////////////////////////////
//~ rjf: @demon_os_hooks Target Process Launching/Attaching/Killing/Detaching/Halting

internal U32
demon_os_launch_process(OS_LaunchOptions *options){
  Temp scratch = scratch_begin(0, 0);
  
  // TODO(allen): maybe a good command line escaper/parser function pair?
  String8 cmd = {0};
  if(options->cmd_line.first != 0)
  {
    String8List args = {0};
    String8 exe_path = options->cmd_line.first->string;
    str8_list_pushf(scratch.arena, &args, "\"%S\"", exe_path);
    for(String8Node *n = options->cmd_line.first->next; n != 0; n = n->next)
    {
      str8_list_push(scratch.arena, &args, n->string);
    }
    StringJoin join_params = {0};
    join_params.sep = str8_lit(" ");
    cmd = str8_list_join(scratch.arena, &args, &join_params);
  }
  
  StringJoin join_params2 = {0};
  join_params2.sep = str8_lit("\0");
  join_params2.post = str8_lit("\0");
  String8List all_opts = options->env;
  if(options->inherit_env != 0)
  {
    MemoryZeroStruct(&all_opts);
    for(String8Node *n = options->env.first; n != 0; n = n->next)
    {
      str8_list_push(scratch.arena, &all_opts, n->string);
    }
    for(String8Node *n = demon_w32_environment.first; n != 0; n = n->next)
    {
      str8_list_push(scratch.arena, &all_opts, n->string);
    }
  }
  String8 env = str8_list_join(scratch.arena, &all_opts, &join_params2);
  
  String16 cmd16 = str16_from_8(scratch.arena, cmd);
  String16 dir16 = str16_from_8(scratch.arena, options->path);
  String16 env16 = str16_from_8(scratch.arena, env);
  
  U32 result = 0;
  
  //- rjf: launch
  DWORD access_flags = CREATE_UNICODE_ENVIRONMENT|DEBUG_PROCESS;
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {0};
  AllocConsole();
  if (CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, 1, access_flags, (WCHAR*)env16.str, (WCHAR*)dir16.str,
                     &startup_info, &process_info)){
    // check if we are 32-bit app, and just close it immediately
    BOOL is_wow = 0;
    IsWow64Process(process_info.hProcess, &is_wow);
    if ( is_wow ){
      MessageBox(0,"Sorry, The RAD Debugger only debugs 64-bit applications currently.","Process error",MB_OK|MB_ICONSTOP);
      DebugActiveProcessStop(process_info.dwProcessId);
      TerminateProcess(process_info.hProcess,0xffffffff);
      CloseHandle(process_info.hProcess);
      CloseHandle(process_info.hThread);
    }
    else{
      CloseHandle(process_info.hProcess);
      CloseHandle(process_info.hThread);
      result = process_info.dwProcessId;
      demon_w32_new_process_pending = 1;
    }
  }
  else{
    MessageBox(0,"Error starting process.","Process error",MB_OK|MB_ICONSTOP);
  }
  FreeConsole();
  
  //- rjf: eliminate all handles which have stuck around from the AllocConsole
  {
    SetStdHandle(STD_INPUT_HANDLE, 0);
    SetStdHandle(STD_OUTPUT_HANDLE, 0);
    SetStdHandle(STD_ERROR_HANDLE, 0);
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
demon_os_attach_process(U32 pid){
  B32 result = 0;
  if (DebugActiveProcess((DWORD)pid)){
    result = 1;
    demon_w32_new_process_pending = 1;
  }
  return(result);
}

internal B32
demon_os_kill_process(DEMON_Entity *process, U32 exit_code){
  B32 result = 0;
  DEMON_W32_Ext *ext = demon_w32_ext(process);
  if (TerminateProcess(ext->proc.handle, exit_code)){
    result = 1;
  }
  return(result);
}

internal B32
demon_os_detach_process(DEMON_Entity *process){
  B32 result = 0;
  
  // rjf: resume threads
  for(DEMON_Entity *child = process->first; child != 0; child = child->next)
  {
    if(child->kind == DEMON_EntityKind_Thread)
    {
      DEMON_W32_Ext *thread_ext = demon_w32_ext(child);
      DWORD resume_result = ResumeThread(thread_ext->thread.handle);
    }
  }
  
  // rjf: detach
  {
    DWORD pid = (DWORD)process->id;
    if(DebugActiveProcessStop(pid))
    {
      result = 1;
    }
  }
  
  // rjf: push into list
  if(result != 0)
  {
    DEMON_EntityNode *n = push_array(demon_w32_detach_proc_arena, DEMON_EntityNode, 1);
    n->entity = process;
    SLLQueuePush(demon_w32_first_detached_proc, demon_w32_last_detached_proc, n);
  }
  
  return(result);
}

////////////////////////////////
//~ rjf: @demon_os_hooks Entity Functions

//- rjf: cleanup

internal void
demon_os_entity_cleanup(DEMON_Entity *entity)
{
  if (entity->kind == DEMON_EntityKind_Process){
    DEMON_W32_Ext *ext = demon_w32_ext(entity);
    SLLStackPush(demon_w32_proc_ext_free, ext);
  }
  else if(entity->kind == DEMON_EntityKind_Module)
  {
    DEMON_W32_Ext *ext = demon_w32_ext(entity);
    CloseHandle(ext->module.handle);
  }
}

//- rjf: introspection

internal String8
demon_os_full_path_from_module(Arena *arena, DEMON_Entity *module){
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // object -> handle
  DEMON_W32_Ext *module_ext = (DEMON_W32_Ext*)module->ext;
  HANDLE handle = module_ext->module.handle;
  
  String16 path16 = {0};
  String8 path8 = {0};
  
  // handle -> full path
  if (handle != 0){
    DWORD cap16 = GetFinalPathNameByHandleW(handle, 0, 0, VOLUME_NAME_DOS);
    U16 *buffer16 = push_array_no_zero(scratch.arena, U16, cap16);
    DWORD size16 = GetFinalPathNameByHandleW(handle, (WCHAR*)buffer16, cap16, VOLUME_NAME_DOS);
    path16 = str16(buffer16, size16);
  }
  
  // fallback (main module only): process -> full path
  if (path16.size == 0 && module_ext->module.is_main){
    // process handle
    DEMON_Entity *process = module->parent;
    DEMON_W32_Ext *process_ext = (DEMON_W32_Ext*)process->ext;
    HANDLE process_handle = process_ext->proc.handle;
    
    DWORD size = KB(4);
    U16 *buf = push_array_no_zero(scratch.arena, U16, size);
    if (QueryFullProcessImageNameW(process_handle, 0, (WCHAR*)buf, &size)){
      path16 = str16(buf, size);
    }
  }
  
  // fallback (any module - no gaurantee): address_of_name -> full path
  if (path16.size == 0 && module_ext->module.address_of_name_pointer != 0){
    // process handle
    DEMON_Entity *process = module->parent;
    DEMON_W32_Ext *process_ext = (DEMON_W32_Ext*)process->ext;
    HANDLE process_handle = process_ext->proc.handle;
    
    // TODO(allen): address size independence
    U64 ptr_size = 8;
    
    U64 name_pointer = 0;
    if (demon_w32_read_memory(process_handle, &name_pointer, module_ext->module.address_of_name_pointer, ptr_size)){
      if (name_pointer != 0){
        if (module_ext->module.name_is_unicode){
          path16 = demon_w32_read_memory_str16(scratch.arena, process_handle, name_pointer);
        }
        else{
          path8 = demon_w32_read_memory_str(scratch.arena, process_handle, name_pointer);
        }
      }
    }
  }
  
  // finalize the result
  String8 result = {0};
  
  if (path16.size > 0){
    // skip the extended path thing if necessary
    if (path16.size >= 4 &&
        path16.str[0] == L'\\' &&
        path16.str[1] == L'\\' &&
        path16.str[2] == L'?' &&
        path16.str[3] == L'\\'){
      path16.size -= 4;
      path16.str += 4;
    }
    
    // convert result
    result = str8_from_16(arena, path16);
  }
  else{
    // skip the extended path thing if necessary
    if (path8.size >= 4 &&
        path8.str[0] == L'\\' &&
        path8.str[1] == L'\\' &&
        path8.str[2] == L'?' &&
        path8.str[3] == L'\\'){
      path8.size -= 4;
      path8.str += 4;
    }
    
    // copy the result
    result = push_str8_copy(arena, path8);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return(result);
}

internal U64
demon_os_stack_base_vaddr_from_thread(DEMON_Entity *thread)
{
  DEMON_Entity *process = thread->parent;
  DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
  U64 tlb = thread_ext->thread.thread_local_base;
  U64 result = 0;
  switch (thread->arch)
  {
    default:{NotImplemented;}break;
    case Architecture_x64:
    {
      U64 stack_base_addr = tlb + 0x8;
      demon_os_read_memory(process, &result, stack_base_addr, 8);
    }break;
    case Architecture_x86:
    {
      U64 stack_base_addr = tlb + 0x4;
      demon_os_read_memory(process, &result, stack_base_addr, 4);
    }break;
  }
  return(result);
}

internal U64
demon_os_tls_root_vaddr_from_thread(DEMON_Entity *thread)
{
  DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
  U64 result = thread_ext->thread.thread_local_base;
  switch(thread->arch)
  {
    default:{NotImplemented;}break;
    case Architecture_x64:
    {
      result += 88;
    }break;
    case Architecture_x86:
    {
      result += 44;
    }break;
  }
  return(result);
}

//- rjf: target process memory allocation/protection

internal U64
demon_os_reserve_memory(DEMON_Entity *process, U64 size){
  DEMON_W32_Ext *ext = demon_w32_ext(process);
  void *ptr = VirtualAllocEx(ext->proc.handle, 0, size, MEM_RESERVE, PAGE_NOACCESS);
  U64 result = (U64)ptr;
  return(result);
}

internal void
demon_os_set_memory_protect_flags(DEMON_Entity *process, U64 page_vaddr, U64 size, DEMON_MemoryProtectFlags flags){
  DEMON_W32_Ext *ext = demon_w32_ext(process);
  DWORD w32_flags = demon_w32_win32_from_memory_protect_flags(flags);
  DWORD old_flags = 0;
  DWORD alloc_type = (flags == 0 ? MEM_DECOMMIT : MEM_COMMIT);
  VirtualAllocEx(ext->proc.handle, (void *)page_vaddr, size, alloc_type, w32_flags);
  (void)old_flags;
}

internal void
demon_os_release_memory(DEMON_Entity *process, U64 vaddr, U64 size){
  DEMON_W32_Ext *ext = demon_w32_ext(process);
  VirtualFreeEx(ext->proc.handle, (void *)vaddr, 0, MEM_RELEASE);
}

//- rjf: target process memory reading/writing

internal U64
demon_os_read_memory(DEMON_Entity *process, void *dst, U64 src_address, U64 size){
  DEMON_W32_Ext *process_ext = demon_w32_ext(process);
  HANDLE handle = process_ext->proc.handle;
  U64 result = demon_w32_read_memory(handle, dst, src_address, size);
  return(result);
}

internal B32
demon_os_write_memory(DEMON_Entity *process, U64 dst_address, void *src, U64 size){
  DEMON_W32_Ext *process_ext = demon_w32_ext(process);
  HANDLE handle = process_ext->proc.handle;
  B32 result = demon_w32_write_memory(handle, dst_address, src, size);
  return(result);
}

//- rjf: thread registers reading/writing

internal B32
demon_os_read_regs_x86(DEMON_Entity *thread, REGS_RegBlockX86 *dst){
  B32 result = 0;
  
  // NOTE(allen): Get Thread Context
  WOW64_CONTEXT ctx = {0};
  ctx.ContextFlags = DEMON_W32_CTX_X86_ALL;
  DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
  HANDLE handle = thread_ext->thread.handle;
  if (Wow64GetThreadContext(handle, (WOW64_CONTEXT *)&ctx)){
    result = 1;
    
    // NOTE(allen): Convert WOW64_CONTEXT -> REGS_RegBlockX86
    dst->eax.u32 = ctx.Eax;
    dst->ebx.u32 = ctx.Ebx;
    dst->ecx.u32 = ctx.Ecx;
    dst->edx.u32 = ctx.Edx;
    dst->esi.u32 = ctx.Esi;
    dst->edi.u32 = ctx.Edi;
    dst->esp.u32 = ctx.Esp;
    dst->ebp.u32 = ctx.Ebp;
    dst->eip.u32 = ctx.Eip;
    dst->cs.u16 = ctx.SegCs;
    dst->ds.u16 = ctx.SegDs;
    dst->es.u16 = ctx.SegEs;
    dst->fs.u16 = ctx.SegFs;
    dst->gs.u16 = ctx.SegGs;
    dst->ss.u16 = ctx.SegSs;
    dst->dr0.u32 = ctx.Dr0;
    dst->dr1.u32 = ctx.Dr1;
    dst->dr2.u32 = ctx.Dr2;
    dst->dr3.u32 = ctx.Dr3;
    dst->dr6.u32 = ctx.Dr6;
    dst->dr7.u32 = ctx.Dr7;
    
    // NOTE(allen): This bit is "supposed to always be 1" I guess.
    // TODO(allen): Not sure what this is all about but I haven't investigated it yet.
    // This might be totally not necessary or something.
    dst->eflags.u32 = ctx.EFlags | 0x2;
    
    XSAVE_FORMAT *fxsave = (XSAVE_FORMAT*)ctx.ExtendedRegisters;
    dst->fcw.u16 = fxsave->ControlWord;
    dst->fsw.u16 = fxsave->StatusWord;
    dst->ftw.u16 = demon_w32_real_tag_word_from_xsave(fxsave);
    dst->fop.u16 = fxsave->ErrorOpcode;
    dst->fip.u32 = fxsave->ErrorOffset;
    dst->fcs.u16 = fxsave->ErrorSelector;
    dst->fdp.u32 = fxsave->DataOffset;
    dst->fds.u16 = fxsave->DataSelector;
    dst->mxcsr.u32 = fxsave->MxCsr;
    dst->mxcsr_mask.u32 = fxsave->MxCsr_Mask;
    
    M128A *float_s = fxsave->FloatRegisters;
    REGS_Reg80 *float_d = &dst->fpr0;
    for (U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1){
      MemoryCopy(float_d, float_s, sizeof(*float_d));
    }
    
    M128A *xmm_s = fxsave->XmmRegisters;
    REGS_Reg256 *xmm_d = &dst->ymm0;
    for (U32 n = 0; n < 8; n += 1, xmm_s += 1, xmm_d += 1){
      MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_s));
    }
    
    // read FS/GS base
    WOW64_LDT_ENTRY ldt = {0};
    
    if (Wow64GetThreadSelectorEntry(handle, ctx.SegFs, &ldt)){
      U32 base = (ldt.BaseLow) | (ldt.HighWord.Bytes.BaseMid << 16) | (ldt.HighWord.Bytes.BaseHi << 24);
      dst->fsbase.u32 = base;
    }
    if (Wow64GetThreadSelectorEntry(handle, ctx.SegGs, &ldt)){
      U32 base = (ldt.BaseLow) | (ldt.HighWord.Bytes.BaseMid << 16) | (ldt.HighWord.Bytes.BaseHi << 24);
      dst->gsbase.u32 = base;
    }
  }
  
  return(result);
}

internal B32
demon_os_write_regs_x86(DEMON_Entity *thread, REGS_RegBlockX86 *src){
  // NOTE(allen): Convert REGS_RegBlockX86 -> WOW64_CONTEXT
  WOW64_CONTEXT ctx = {0};
  ctx.ContextFlags = DEMON_W32_CTX_X86_ALL;
  ctx.Eax = src->eax.u32;
  ctx.Ebx = src->ebx.u32;
  ctx.Ecx = src->ecx.u32;
  ctx.Edx = src->edx.u32;
  ctx.Esi = src->esi.u32;
  ctx.Edi = src->edi.u32;
  ctx.Esp = src->esp.u32;
  ctx.Ebp = src->ebp.u32;
  ctx.Eip = src->eip.u32;
  ctx.SegCs = src->cs.u16;
  ctx.SegDs = src->ds.u16;
  ctx.SegEs = src->es.u16;
  ctx.SegFs = src->fs.u16;
  ctx.SegGs = src->gs.u16;
  ctx.SegSs = src->ss.u16;
  ctx.Dr0 = src->dr0.u32;
  ctx.Dr1 = src->dr1.u32;
  ctx.Dr2 = src->dr2.u32;
  ctx.Dr3 = src->dr3.u32;
  ctx.Dr6 = src->dr6.u32;
  ctx.Dr7 = src->dr7.u32;
  ctx.EFlags = src->eflags.u32;
  
  XSAVE_FORMAT *fxsave = (XSAVE_FORMAT*)ctx.ExtendedRegisters;
  fxsave->ControlWord = src->fcw.u16;
  fxsave->StatusWord = src->fsw.u16;
  fxsave->TagWord = demon_w32_xsave_tag_word_from_real_tag_word(src->ftw.u16);
  fxsave->ErrorOpcode = src->fop.u16;
  fxsave->ErrorSelector = src->fcs.u16;
  fxsave->DataSelector = src->fds.u16;
  fxsave->ErrorOffset = src->fip.u32;
  fxsave->DataOffset = src->fdp.u32;
  fxsave->MxCsr = src->mxcsr.u32 & src->mxcsr_mask.u32;
  fxsave->MxCsr_Mask = src->mxcsr_mask.u32;
  
  M128A *float_d = fxsave->FloatRegisters;
  REGS_Reg80 *float_s = &src->fpr0;
  for (U32 n = 0;
       n < 8;
       n += 1, float_s += 1, float_d += 1){
    MemoryCopy(float_d, float_s, 10);
  }
  
  M128A *xmm_d = fxsave->XmmRegisters;
  REGS_Reg256 *xmm_s = &src->ymm0;
  for (U32 n = 0;
       n < 8;
       n += 1, xmm_d += 1, xmm_s += 1){
    MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_d));
  }
  
  // set thread context
  DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
  HANDLE handle = thread_ext->thread.handle;
  B32 result = 0;
  if (Wow64SetThreadContext(handle, &ctx)){
    result = 1;
  }
  
  return(result);
}

internal B32
demon_os_read_regs_x64(DEMON_Entity *thread, REGS_RegBlockX64 *dst){
  Temp scratch = scratch_begin(0, 0);
  
  // NOTE(allen): Check available features
  U32 feature_mask = GetEnabledXStateFeatures();
  B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
  
  // NOTE(allen): Setup the context
  CONTEXT *ctx = 0;
  U32 ctx_flags = DEMON_W32_CTX_X64_ALL;
  if (avx_enabled){
    ctx_flags |= DEMON_W32_CTX_INTEL_XSTATE;
  }
  DWORD size = 0;
  InitializeContext(0, ctx_flags, 0, &size);
  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
    void *ctx_memory = push_array(scratch.arena, U8, size);
    if (!InitializeContext(ctx_memory, ctx_flags, &ctx, &size)){
      ctx = 0;
    }
  }
  
  B32 avx_available = 0;
  
  if (ctx != 0){
    // NOTE(allen): Finish Context Setup
    if (avx_enabled){
      SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
    }
    
    // NOTE(allen): Determine what features are available on this particular ctx
    // TODO(allen): Experiment carefully with this nonsense.
    // Does avx_enabled = avx_available in all circumstances or not?
    DWORD64 xstate_flags = 0;
    if (GetXStateFeaturesMask(ctx, &xstate_flags)){
      if (xstate_flags & XSTATE_MASK_AVX){
        avx_available = 1;
      }
    }
  }
  
  // get thread context
  DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
  HANDLE thread_handle = thread_ext->thread.handle;
  if (!GetThreadContext(thread_handle, ctx)){
    ctx = 0;
  }
  
  B32 result = 0;
  if (ctx != 0){
    result = 1;
    
    // NOTE(allen): Convert CONTEXT -> REGS_RegBlockX64
    dst->rax.u64 = ctx->Rax;
    dst->rcx.u64 = ctx->Rcx;
    dst->rdx.u64 = ctx->Rdx;
    dst->rbx.u64 = ctx->Rbx;
    dst->rsp.u64 = ctx->Rsp;
    dst->rbp.u64 = ctx->Rbp;
    dst->rsi.u64 = ctx->Rsi;
    dst->rdi.u64 = ctx->Rdi;
    dst->r8.u64  = ctx->R8;
    dst->r9.u64  = ctx->R9;
    dst->r10.u64 = ctx->R10;
    dst->r11.u64 = ctx->R11;
    dst->r12.u64 = ctx->R12;
    dst->r13.u64 = ctx->R13;
    dst->r14.u64 = ctx->R14;
    dst->r15.u64 = ctx->R15;
    dst->rip.u64 = ctx->Rip;
    dst->cs.u16  = ctx->SegCs;
    dst->ds.u16  = ctx->SegDs;
    dst->es.u16  = ctx->SegEs;
    dst->fs.u16  = ctx->SegFs;
    dst->gs.u16  = ctx->SegGs;
    dst->ss.u16  = ctx->SegSs;
    dst->dr0.u32 = ctx->Dr0;
    dst->dr1.u32 = ctx->Dr1;
    dst->dr2.u32 = ctx->Dr2;
    dst->dr3.u32 = ctx->Dr3;
    dst->dr6.u32 = ctx->Dr6;
    dst->dr7.u32 = ctx->Dr7;
    
    // NOTE(allen): This bit is "supposed to always be 1" I guess.
    // TODO(allen): Not sure what this is all about but I haven't investigated it yet.
    // This might be totally not necessary or something.
    dst->rflags.u64 = ctx->EFlags | 0x2;
    
    XSAVE_FORMAT *xsave = &ctx->FltSave;
    dst->fcw.u16 = xsave->ControlWord;
    dst->fsw.u16 = xsave->StatusWord;
    dst->ftw.u16 = demon_w32_real_tag_word_from_xsave(xsave);
    dst->fop.u16 = xsave->ErrorOpcode;
    dst->fcs.u16 = xsave->ErrorSelector;
    dst->fds.u16 = xsave->DataSelector;
    dst->fip.u32 = xsave->ErrorOffset;
    dst->fdp.u32 = xsave->DataOffset;
    dst->mxcsr.u32 = xsave->MxCsr;
    dst->mxcsr_mask.u32 = xsave->MxCsr_Mask;
    
    M128A *float_s = xsave->FloatRegisters;
    REGS_Reg80 *float_d = &dst->fpr0;
    for (U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1){
      MemoryCopy(float_d, float_s, sizeof(*float_d));
    }
    
    if (!avx_available){
      M128A *xmm_s = xsave->XmmRegisters;
      REGS_Reg256 *xmm_d = &dst->ymm0;
      for (U32 n = 0; n < 16; n += 1, xmm_s += 1, xmm_d += 1){
        MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_s));
      }
    }
    
    if (avx_available){
      DWORD part0_length = 0;
      M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
      DWORD part1_length = 0;
      M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
      Assert(part0_length == part1_length);
      
      DWORD count = part0_length/sizeof(part0[0]);
      count = ClampTop(count, 16);
      REGS_Reg256 *ymm_d = &dst->ymm0;
      for (DWORD i = 0;
           i < count;
           i += 1, part0 += 1, part1 += 1, ymm_d += 1){
        // TODO(allen): Are we writing these out in the right order? Seems weird right?
        ymm_d->u64[3] = part0->Low;
        ymm_d->u64[2] = part0->High;
        ymm_d->u64[1] = part1->Low;
        ymm_d->u64[0] = part1->High;
      }
    }
    
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
demon_os_write_regs_x64(DEMON_Entity *thread, REGS_RegBlockX64 *src){
  Temp scratch = scratch_begin(0, 0);
  
  // NOTE(allen): Check available features
  U32 feature_mask = GetEnabledXStateFeatures();
  B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
  
  // NOTE(allen): Setup the context
  CONTEXT *ctx = 0;
  U32 ctx_flags = DEMON_W32_CTX_X64_ALL;
  if (avx_enabled){
    ctx_flags |= DEMON_W32_CTX_INTEL_XSTATE;
  }
  DWORD size = 0;
  InitializeContext(0, ctx_flags, 0, &size);
  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
    void *ctx_memory = push_array(scratch.arena, U8, size);
    if (!InitializeContext(ctx_memory, ctx_flags, &ctx, &size)){
      ctx = 0;
    }
  }
  
  B32 avx_available = 0;
  
  if (ctx != 0){
    // NOTE(allen): Finish Context Setup
    if (avx_enabled){
      SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
    }
    
    // NOTE(allen): Determine what features are available on this particular ctx
    // TODO(allen): Experiment carefully with this nonsense.
    // Does avx_enabled = avx_available in all circumstances or not?
    DWORD64 xstate_flags = 0;
    if (GetXStateFeaturesMask(ctx, &xstate_flags)){
      if (xstate_flags & XSTATE_MASK_AVX){
        avx_available = 1;
      }
    }
  }
  
  B32 result = 0;
  if (ctx != 0){
    // NOTE(allen): Convert REGS_RegBlockX64 -> CONTEXT
    ctx->ContextFlags = ctx_flags;
    
    ctx->MxCsr = src->mxcsr.u32 & src->mxcsr_mask.u32;
    
    ctx->Rax = src->rax.u64;
    ctx->Rcx = src->rcx.u64;
    ctx->Rdx = src->rdx.u64;
    ctx->Rbx = src->rbx.u64;
    ctx->Rsp = src->rsp.u64;
    ctx->Rbp = src->rbp.u64;
    ctx->Rsi = src->rsi.u64;
    ctx->Rdi = src->rdi.u64;
    ctx->R8  = src->r8.u64;
    ctx->R9  = src->r9.u64;
    ctx->R10 = src->r10.u64;
    ctx->R11 = src->r11.u64;
    ctx->R12 = src->r12.u64;
    ctx->R13 = src->r13.u64;
    ctx->R14 = src->r14.u64;
    ctx->R15 = src->r15.u64;
    ctx->Rip = src->rip.u64;
    ctx->SegCs = src->cs.u16;
    ctx->SegDs = src->ds.u16;
    ctx->SegEs = src->es.u16;
    ctx->SegFs = src->fs.u16;
    ctx->SegGs = src->gs.u16;
    ctx->SegSs = src->ss.u16;
    ctx->Dr0 = src->dr0.u32;
    ctx->Dr1 = src->dr1.u32;
    ctx->Dr2 = src->dr2.u32;
    ctx->Dr3 = src->dr3.u32;
    ctx->Dr6 = src->dr6.u32;
    ctx->Dr7 = src->dr7.u32;
    
    ctx->EFlags = src->rflags.u64;
    
    XSAVE_FORMAT *fxsave = &ctx->FltSave;
    fxsave->ControlWord = src->fcw.u16;
    fxsave->StatusWord = src->fsw.u16;
    fxsave->TagWord = demon_w32_xsave_tag_word_from_real_tag_word(src->ftw.u16);
    fxsave->ErrorOpcode = src->fop.u16;
    fxsave->ErrorSelector = src->fcs.u16;
    fxsave->DataSelector = src->fds.u16;
    fxsave->ErrorOffset = src->fip.u32;
    fxsave->DataOffset = src->fdp.u32;
    
    M128A *float_d = fxsave->FloatRegisters;
    REGS_Reg80 *float_s = &src->fpr0;
    for (U32 n = 0;
         n < 8;
         n += 1, float_s += 1, float_d += 1){
      MemoryCopy(float_d, float_s, 10);
    }
    
    if (!avx_available){
      M128A *xmm_d = fxsave->XmmRegisters;
      REGS_Reg256 *xmm_s = &src->ymm0;
      for (U32 n = 0;
           n < 8;
           n += 1, xmm_d += 1, xmm_s += 1){
        MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_d));
      }
    }
    
    if (avx_available){
      DWORD part0_length = 0;
      M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
      DWORD part1_length = 0;
      M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
      Assert(part0_length == part1_length);
      
      DWORD count = part0_length/sizeof(part0[0]);
      count = ClampTop(count, 16);
      REGS_Reg256 *ymm_d = &src->ymm0;
      for (DWORD i = 0;
           i < count;
           i += 1, part0 += 1, part1 += 1, ymm_d += 1){
        // TODO(allen): Are we writing these out in the right order? Seems weird right?
        part0->Low  = ymm_d->u64[3];
        part0->High = ymm_d->u64[2];
        part1->Low  = ymm_d->u64[1];
        part1->High = ymm_d->u64[0];
      }
    }
    
    //- set thread context
    DEMON_W32_Ext *thread_ext = demon_w32_ext(thread);
    HANDLE thread_handle = thread_ext->thread.handle;
    if (SetThreadContext(thread_handle, ctx)){
      result = 1;
    }
  }
  
  scratch_end(scratch);
  return(result);
}

////////////////////////////////
//~ rjf: @demon_os_hooks Process Listing

internal void
demon_os_proc_iter_begin(DEMON_ProcessIter *iter){
  MemoryZeroStruct(iter);
  iter->v[0] = (U64)CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
}

internal B32
demon_os_proc_iter_next(Arena *arena, DEMON_ProcessIter *iter, DEMON_ProcessInfo *info_out){
  // get the next process entry
  B32 result = 0;
  PROCESSENTRY32W process_entry = {sizeof(process_entry)};
  HANDLE snapshot = (HANDLE)iter->v[0];
  if (iter->v[1] == 0){
    if (Process32FirstW(snapshot, &process_entry)){
      result = 1;
    }
  }
  else{
    if (Process32NextW(snapshot, &process_entry)){
      result = 1;
    }
  }
  
  // increment counter
  iter->v[1] += 1;
  
  // convert to process info
  if (result){
    info_out->name = str8_from_16(arena, str16_cstring((U16*)process_entry.szExeFile));
    info_out->pid = (U32)process_entry.th32ProcessID;
  }
  
  return(result);
}

internal void
demon_os_proc_iter_end(DEMON_ProcessIter *iter){
  CloseHandle((HANDLE)iter->v[0]);
  MemoryZeroStruct(iter);
}
