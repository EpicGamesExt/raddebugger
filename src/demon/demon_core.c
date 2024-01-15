// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
demon_init(void){
  demon_common_init();
  demon_os_init();
}

////////////////////////////////
//~ rjf: Basic Type Functions

//- rjf: stringizing

internal String8
demon_string_from_event_kind(DEMON_EventKind kind){
  String8 result = str8_lit("unknown");
  switch (kind){
    default: break;
    case DEMON_EventKind_Error:             result = str8_lit("Error"); break;
    case DEMON_EventKind_HandshakeComplete: result = str8_lit("HandshakeComplete"); break;
    case DEMON_EventKind_CreateProcess:     result = str8_lit("CreateProcess"); break;
    case DEMON_EventKind_ExitProcess:       result = str8_lit("ExitProcess"); break;
    case DEMON_EventKind_CreateThread:      result = str8_lit("CreateThread"); break;
    case DEMON_EventKind_ExitThread:        result = str8_lit("ExitThread"); break;
    case DEMON_EventKind_LoadModule:        result = str8_lit("LoadModule"); break;
    case DEMON_EventKind_UnloadModule:      result = str8_lit("UnloadModule"); break;
    case DEMON_EventKind_Breakpoint:        result = str8_lit("Breakpoint"); break;
    case DEMON_EventKind_Trap:              result = str8_lit("Trap"); break;
    case DEMON_EventKind_SingleStep:        result = str8_lit("SingleStep"); break;
    case DEMON_EventKind_Exception:         result = str8_lit("Exception"); break;
    case DEMON_EventKind_Halt:              result = str8_lit("Halt"); break;
    case DEMON_EventKind_Memory:            result = str8_lit("Memory"); break;
    case DEMON_EventKind_DebugString:       result = str8_lit("DebugString"); break;
    case DEMON_EventKind_SetThreadName:     result = str8_lit("SetThreadName"); break;
  }
  return(result);
}

internal String8
demon_string_from_memory_event_kind(DEMON_MemoryEventKind kind){
  String8 result = str8_lit("unknown");
  switch (kind){
    default: break;
    case DEMON_MemoryEventKind_Commit:   result = str8_lit("Commit"); break;
    case DEMON_MemoryEventKind_Reserve:  result = str8_lit("Reserve"); break;
    case DEMON_MemoryEventKind_Decommit: result = str8_lit("Decommit"); break;
    case DEMON_MemoryEventKind_Release:  result = str8_lit("Release"); break;
  }
  return(result);
}

internal String8
demon_string_from_exception_kind(DEMON_ExceptionKind kind){
  String8 result = str8_lit("unknown");
  switch (kind){
    default: break;
    case DEMON_ExceptionKind_MemoryRead:     result = str8_lit("MemoryRead"); break;
    case DEMON_ExceptionKind_MemoryWrite:    result = str8_lit("MemoryWrite"); break;
    case DEMON_ExceptionKind_MemoryExecute:  result = str8_lit("MemoryExecute"); break;
    case DEMON_ExceptionKind_CppThrow:       result = str8_lit("CppThrow"); break;
  }
  return(result);
}

internal void
demon_string_list_from_event(Arena *arena, String8List *out, DEMON_Event *event){
  B32 need_exception_info = (event->kind == DEMON_EventKind_Exception ||
                             event->kind == DEMON_EventKind_Breakpoint ||
                             event->kind == DEMON_EventKind_Halt ||
                             event->kind == DEMON_EventKind_SingleStep);
  
  // allen: kind
  String8 kind_string = demon_string_from_event_kind(event->kind);
  str8_list_pushf(arena, out, "%S: { (%i)", kind_string, event->kind);
  
  // rjf: basics
  {
    str8_list_pushf(arena, out, "  process: (%I64x)", event->process);
    str8_list_pushf(arena, out, "  thread: (%I64x)", event->thread);
    str8_list_pushf(arena, out, "  module: (%I64x)", event->module);
    str8_list_pushf(arena, out, "  address: (%I64x)", event->address, event->address);
    str8_list_pushf(arena, out, "  size: (0x%I64x, %I64u)", event->size, event->size);
  }
  
  // rjf: string
  if (event->string.size != 0){
    str8_list_pushf(arena, out, "  string: \"%S\"", event->string);
  }
  
  // rjf: exception info
  if (need_exception_info){
    str8_list_pushf(arena, out, "  code: (0x%x, %i)", event->code, event->code);
    str8_list_pushf(arena, out, "  flags: (0x%x, %i)", event->flags, event->flags);
    str8_list_pushf(arena, out, "  signo: (0x%x, %i)", event->signo, event->signo);
    str8_list_pushf(arena, out, "  sigcode: (0x%x, %i)", event->sigcode, event->sigcode);
  }
  
  // rjf: need error info
  if (event->kind == DEMON_EventKind_Error){
    str8_list_pushf(arena, out, "  error_kind: (0x%x, %i)", event->error_kind, event->error_kind);
  }
  
  // rjf: memory event kind info
  if (event->memory_kind != DEMON_MemoryEventKind_Null){
    String8 memory_kind_string = demon_string_from_memory_event_kind(event->memory_kind);
    str8_list_pushf(arena, out, "  memory_kind: (%S, %i)",
                    memory_kind_string, event->memory_kind);
  }
  
  // rjf: exception kind
  if (need_exception_info){
    String8 exception_kind_string = demon_string_from_exception_kind(event->exception_kind);
    str8_list_pushf(arena, out, "  exception_kind: (%S, %i)",
                    exception_kind_string, event->exception_kind);
  }
  
  // rjf: instruction ptr
  if (event->instruction_pointer != 0){
    str8_list_pushf(arena, out, "  instruction_pointer: (%I64x)", event->instruction_pointer);
  }
  
  // rjf: stack ptr
  if (event->stack_pointer != 0){
    str8_list_pushf(arena, out, "  stack_pointer: (%I64x)", event->stack_pointer);
  }
  
  str8_list_pushf(arena, out, "  user_data: (0x%I64x, %I64u)",
                  event->user_data, event->user_data);
  str8_list_pushf(arena, out, "}");
}

//- rjf: trap chunk lists

internal void
demon_trap_chunk_list_push(Arena *arena, DEMON_TrapChunkList *list, U64 cap, DEMON_Trap *trap)
{
  DEMON_TrapChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DEMON_TrapChunkNode, 1);
    node->cap = cap;
    node->v = push_array_no_zero(arena, DEMON_Trap, node->cap);
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], trap);
  node->count += 1;
  list->trap_count += 1;
}

internal void
demon_trap_chunk_list_concat_in_place(DEMON_TrapChunkList *dst, DEMON_TrapChunkList *to_push)
{
  if(dst->last == 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  else if(to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->node_count += to_push->node_count;
    dst->trap_count += to_push->trap_count;
  }
  MemoryZeroStruct(to_push);
}

internal void
demon_trap_chunk_list_concat_shallow_copy(Arena *arena, DEMON_TrapChunkList *dst, DEMON_TrapChunkList *to_push)
{
  for(DEMON_TrapChunkNode *src_n = to_push->first; src_n != 0; src_n = src_n->next)
  {
    DEMON_TrapChunkNode *dst_n = push_array(arena, DEMON_TrapChunkNode, 1);
    dst_n->v     = src_n->v;
    dst_n->cap   = src_n->cap;
    dst_n->count = src_n->count;
    SLLQueuePush(dst->first, dst->last, dst_n);
    dst->node_count += 1;
    dst->trap_count += dst_n->count;
  }
}

//- rjf: handle lists

internal void
demon_handle_list_push(Arena *arena, DEMON_HandleList *list, DEMON_Handle handle)
{
  DEMON_HandleNode *node = push_array(arena, DEMON_HandleNode, 1);
  SLLQueuePush(list->first, list->last, node);
  node->v = handle;
  list->count += 1;
}

internal DEMON_HandleArray
demon_handle_array_from_list(Arena *arena, DEMON_HandleList *list)
{
  DEMON_HandleArray array = {0};
  array.count = list->count;
  array.handles = push_array_no_zero(arena, DEMON_Handle, array.count);
  U64 idx = 0;
  for(DEMON_HandleNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    array.handles[idx] = n->v;
  }
  return array;
}

internal DEMON_HandleArray
demon_handle_array_copy(Arena *arena, DEMON_HandleArray *src)
{
  DEMON_HandleArray dst = {0};
  dst.count = src->count;
  dst.handles = push_array_no_zero(arena, DEMON_Handle, dst.count);
  MemoryCopy(dst.handles, src->handles, sizeof(DEMON_Handle)*dst.count);
  return dst;
}

////////////////////////////////
//~ rjf: Primary Thread & Exclusive Mode Controls

internal void
demon_primary_thread_begin(void){
  demon_primary_thread = 1;
}

internal void
demon_exclusive_mode_begin(void){
  Assert(demon_primary_thread);
  os_mutex_take(demon_state_mutex);
  demon_run_state = 1;
  os_mutex_drop(demon_state_mutex);
}

internal void
demon_exclusive_mode_end(void){
  Assert(demon_primary_thread);
  os_mutex_take(demon_state_mutex);
  demon_run_state = 0;
  os_mutex_drop(demon_state_mutex);
}

////////////////////////////////
//~ rjf: Running/Halting

internal DEMON_EventList
demon_run(Arena *arena, DEMON_RunCtrls *ctrls)
{
  Assert(demon_primary_thread);
  Temp scratch = scratch_begin(&arena, 1);
  
  // convert controls to os controls
  B32 full_conversion = 1;
  DEMON_OS_RunCtrls os_ctrls = {0};
  {
    // convert single_step_thread
    if (ctrls->single_step_thread != 0){
      DEMON_Entity *sst_entity = demon_ent_ptr_from_handle(ctrls->single_step_thread);
      if (sst_entity != 0 &&
          sst_entity->kind == DEMON_EntityKind_Thread){
        os_ctrls.single_step_thread = sst_entity;
      }
      else{
        full_conversion = 0;
        goto finish_conversion;
      }
    }
    
    // convert exception handling flag
    os_ctrls.ignore_previous_exception = ctrls->ignore_previous_exception;
    
    // convert fronzen threads
    os_ctrls.run_entities_are_unfrozen = ctrls->run_entities_are_unfrozen;
    os_ctrls.run_entities_are_processes = ctrls->run_entities_are_processes;
    os_ctrls.run_entity_count = ctrls->run_entity_count;
    os_ctrls.run_entities = push_array_no_zero(scratch.arena, DEMON_Entity*, ctrls->run_entity_count);
    {
      DEMON_EntityKind expected_entity_kind = DEMON_EntityKind_Thread;
      if (os_ctrls.run_entities_are_processes){
        expected_entity_kind = DEMON_EntityKind_Process;
      }
      
      DEMON_Handle *src = ctrls->run_entities;
      DEMON_Entity **dst = os_ctrls.run_entities;
      for (U64 i = 0; i < ctrls->run_entity_count; i += 1, src += 1, dst += 1){
        DEMON_Entity *frozen_thread = demon_ent_ptr_from_handle(*src);
        if (frozen_thread != 0 &&
            frozen_thread->kind == expected_entity_kind){
          *dst = frozen_thread;
        }
        else{
          full_conversion = 0;
          goto finish_conversion;
        }
      }
    }
    
    // convert traps
    os_ctrls.traps = push_array_no_zero(scratch.arena, DEMON_OS_Trap, ctrls->traps.trap_count);
    {
      DEMON_OS_Trap *dst = os_ctrls.traps;
      
      for (DEMON_TrapChunkNode *node = ctrls->traps.first;
           node != 0;
           node = node->next){
        DEMON_Trap *src = node->v;
        U64 node_trap_count = node->count;
        for (U64 i = 0; i < node_trap_count; i += 1, src += 1){
          if (src->process != 0){
            DEMON_Entity *trap_process = demon_ent_ptr_from_handle(src->process); 
            if (trap_process != 0 &&
                trap_process->kind == DEMON_EntityKind_Process){
              dst->process = trap_process;
              dst->address = src->address;
              dst += 1;
            }
            else{
              full_conversion = 0;
              goto finish_conversion;
            }
          }
        }
      }
      
      os_ctrls.trap_count = (U64)(dst - os_ctrls.traps);
    }
    
    finish_conversion:;
  }
  
  // call the OS implementation of run
  DEMON_EventList result = {0};
  if (full_conversion){
    result = demon_os_run(arena, &os_ctrls);
  }
  else{
    DEMON_Event *event = demon_push_event(arena, &result, DEMON_EventKind_Error);
    event->error_kind = DEMON_ErrorKind_InvalidHandle;
  }
  
  scratch_end(scratch);
  return(result);
}

internal void
demon_halt(U64 code, U64 user_data){
  demon_os_halt(code, user_data);
}

internal U64
demon_get_time_counter(void){
  return(demon_time);
}

////////////////////////////////
//~ rjf: Target Process Launching/Attaching/Killing/Detaching/Halting

internal U32
demon_launch_process(OS_LaunchOptions *options){
  Assert(demon_primary_thread);
  U32 result = demon_os_launch_process(options);
  return(result);
}

internal B32
demon_attach_process(U32 pid){
  Assert(demon_primary_thread);
  B32 result = demon_os_attach_process(pid);
  return(result);
}

internal B32
demon_kill_process(DEMON_Handle process, U32 exit_code){
  Assert(demon_primary_thread);
  B32 result = 0;
  DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
  if (entity != 0 &&
      entity->kind == DEMON_EntityKind_Process){
    result = demon_os_kill_process(entity, exit_code);
  }
  return(result);
}

internal B32
demon_detach_process(DEMON_Handle process){
  Assert(demon_primary_thread);
  B32 result = 0;
  DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
  if (entity != 0 &&
      entity->kind == DEMON_EntityKind_Process){
    result = demon_os_detach_process(entity);
  }
  return(result);
}

internal DEMON_Handle
demon_snapshot_thread(DEMON_Handle thread)
{
  DEMON_Handle result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      result = demon_os_create_snapshot(entity);
    }
    demon_access_end();
  }

  return(result);
}

internal void
demon_snapshot_release(DEMON_Handle snapshot)
{
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(snapshot);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Snapshot){
      demon_os_snapshot_release(entity);
    }
    demon_access_end();
  }
}

////////////////////////////////
//~ rjf: Entity Functions

//- rjf: basics

internal B32
demon_object_exists(DEMON_Handle object){
  B32 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(object);
    result = (entity != 0);
    demon_access_end();
  }
  return(result);
}

//- rjf: introspection

internal Architecture
demon_arch_from_object(DEMON_Handle object){
  Architecture result = Architecture_Null;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(object);
    if (entity != 0){
      result = (Architecture)entity->arch;
    }
    demon_access_end();
  }
  return(result);
}

internal U64
demon_base_vaddr_from_module(DEMON_Handle module){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(module);
    if (entity != 0 && entity->kind == DEMON_EntityKind_Module){
      result = entity->id;
    }
    demon_access_end();
  }
  return(result);
}

internal Rng1U64
demon_vaddr_range_from_module(DEMON_Handle module)
{
  Rng1U64 result = {0};
  if(demon_access_begin())
  {
    DEMON_Entity *entity = demon_ent_ptr_from_handle(module);
    if(entity != 0 && entity->kind == DEMON_EntityKind_Module)
    {
      result = r1u64(entity->id, entity->id+entity->addr_range_dim);
    }
    demon_access_end();
  }
  return(result);
}

internal String8
demon_full_path_from_module(Arena *arena, DEMON_Handle module){
  String8 result = {0};
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(module);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Module){
      result = demon_accel_full_path_from_module(arena, entity);
    }
    demon_access_end();
  }
  return(result);
}

internal U64
demon_stack_base_vaddr_from_thread(DEMON_Handle thread){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 && entity->kind == DEMON_EntityKind_Thread){
      result = demon_accel_stack_base_vaddr_from_thread(entity);
    }
    demon_access_end();
  }
  return(result);
}

internal U64
demon_tls_root_vaddr_from_thread(DEMON_Handle handle){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(handle);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      result = demon_accel_tls_root_vaddr_from_thread(entity);
    }
    demon_access_end();
  }
  return(result);
}

internal DEMON_HandleArray
demon_all_processes(Arena *arena){
  DEMON_HandleArray result = {0};
  
  if (demon_access_begin()){
    DEMON_Handle *handles = push_array_no_zero(arena, DEMON_Handle, demon_proc_count);
    DEMON_Handle *handle_opl = handles + demon_proc_count;
    DEMON_Handle *handle_ptr = handles;
    
    for (DEMON_Entity *process = demon_ent_root->first;
         process != 0 && handle_ptr < handle_opl;
         process = process->next){
      if (process->kind == DEMON_EntityKind_Process){
        *handle_ptr = demon_ent_handle_from_ptr(process);
        handle_ptr += 1;
      }
    }
    
    result.handles = handles;
    result.count = (U64)(handle_ptr - handles);
    
    U64 unused_count = demon_proc_count - result.count;
    arena_put_back(arena, sizeof(DEMON_Handle)*unused_count);
    demon_access_end();
  }
  
  return(result);
}

internal DEMON_HandleArray
demon_threads_from_process(Arena *arena, DEMON_Handle process){
  DEMON_HandleArray result = {0};
  
  if (demon_access_begin()){
    DEMON_Handle *handles = push_array_no_zero(arena, DEMON_Handle, demon_thread_count);
    DEMON_Handle *handle_opl = handles + demon_thread_count;
    DEMON_Handle *handle_ptr = handles;
    
    DEMON_Entity *process_ptr = demon_ent_ptr_from_handle(process);
    
    if (process_ptr != 0 && process_ptr->kind == DEMON_EntityKind_Process){
      for (DEMON_Entity *thread = process_ptr->first;
           thread != 0 && handle_ptr < handle_opl;
           thread = thread->next){
        if (thread->kind == DEMON_EntityKind_Thread){
          *handle_ptr = demon_ent_handle_from_ptr(thread);
          handle_ptr += 1;
        }
      }
    }
    
    result.handles = handles;
    result.count = (U64)(handle_ptr - handles);
    
    U64 unused_count = demon_thread_count - result.count;
    arena_put_back(arena, sizeof(DEMON_Handle)*unused_count);
    demon_access_end();
  }
  
  return(result);
}

internal DEMON_HandleArray
demon_modules_from_process(Arena *arena, DEMON_Handle process){
  DEMON_HandleArray result = {0};
  
  if (demon_access_begin()){
    DEMON_Handle *handles = push_array_no_zero(arena, DEMON_Handle, demon_module_count);
    DEMON_Handle *handle_opl = handles + demon_module_count;
    DEMON_Handle *handle_ptr = handles;
    
    DEMON_Entity *process_ptr = demon_ent_ptr_from_handle(process);
    
    if (process_ptr != 0 && process_ptr->kind == DEMON_EntityKind_Process){
      for (DEMON_Entity *module = process_ptr->first;
           module != 0 && handle_ptr < handle_opl;
           module = module->next){
        if (module->kind == DEMON_EntityKind_Module){
          *handle_ptr = demon_ent_handle_from_ptr(module);
          handle_ptr += 1;
        }
      }
    }
    
    result.handles = handles;
    result.count = (U64)(handle_ptr - handles);
    
    U64 unused_count = demon_module_count - result.count;
    arena_put_back(arena, sizeof(DEMON_Handle)*unused_count);
    demon_access_end();
  }
  
  return(result);
}

//- rjf: target process memory allocation/protection

internal U64
demon_reserve_memory(DEMON_Handle process, U64 size){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Process){
      result = demon_os_reserve_memory(entity, size);
    }
    demon_access_end();
  }
  return(result);
}

internal B32
demon_set_memory_protect_flags(DEMON_Handle process, U64 page_vaddr, U64 size, DEMON_MemoryProtectFlags flags){
  B32 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Process){
      demon_os_set_memory_protect_flags(entity, page_vaddr, size, flags);
      result = 1;
    }
    demon_access_end();
  }
  return(result);
}

internal B32
demon_release_memory(DEMON_Handle process, U64 vaddr, U64 size){
  B32 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Process){
      demon_os_release_memory(entity, vaddr, size);
      result = 1;
    }
    demon_access_end();
  }
  return(result);
}

//- rjf: target process memory reading/writing

internal U64
demon_read_memory(DEMON_Handle process, void *dst, U64 src_address, U64 size){
  U64 bytes_read = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
    if (entity != 0 &&
        (entity->kind == DEMON_EntityKind_Process || entity->kind == DEMON_EntityKind_Snapshot)){
      bytes_read = demon_os_read_memory(entity, dst, src_address, size);
    }
    demon_access_end();
  }
  return(bytes_read);
}

internal B32
demon_write_memory(DEMON_Handle process, U64 dst_address, void *src, U64 size){
  B32 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(process);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Process){
      result = demon_os_write_memory(entity, dst_address, src, size);
    }
    demon_access_end();
  }
  return(result);
}

#define READ_BLOCK_SIZE 4096

internal U64
demon_read_memory_amap_aligned(DEMON_Handle process, void *dst, U64 src_address, U64 size){
  // Algorithm:
  //   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //   ^                        ^                           ^
  //   MIN                      MAX                         SMAX
  //   [MIN,MAX) - range attempting to read
  //   [MAX,SMAX) - range not yet proven to be impossible to read
  
  Assert(src_address%READ_BLOCK_SIZE == 0);
  Assert(size%READ_BLOCK_SIZE == 0);
  
  U64 read_size = 0;
  U64 min = 0;
  U64 max = size;
  U64 smax = max;
  
  for (;;){
    if (max <= min){
      break;
    }
    
    // attempt to read range
    U64 attempt_size = max - min;
    B32 success = demon_read_memory(process, (U8*)dst + min, src_address + min, attempt_size);
    
    if (success){
      // increase successful read size
      read_size += attempt_size;
      // adjust range up
      min = max;
      max = smax;
    }
    else{
      // mark this point as too far
      smax = max - READ_BLOCK_SIZE;
      // bisect the range for the next read attempt
      U64 mid = (min + max)/2;
      U64 aligned_mid = AlignDownPow2(mid, READ_BLOCK_SIZE);
      max = aligned_mid;
    }
  }
  
  U64 result = read_size;
  return(result);
}

internal U64
demon_read_memory_amap(DEMON_Handle process, void *dst, U64 src_address, U64 size){
  U64 read_size = 0;
  
  if (demon_access_begin()){
    B32 done = 0;
    U64 read_opl = src_address + size;
    
    // pre-aligned part   --  [SRC,PRE_OPL)
    U64 src_block_opl = AlignPow2(src_address, READ_BLOCK_SIZE);
    U64 pre_opl = Min(src_block_opl, read_opl);
    if(src_address < pre_opl)
    {
      U64 attempt_size = pre_opl - src_address;
      if(!demon_read_memory(process, dst, src_address, attempt_size))
      {
        done = 1;
      }
      else
      {
        read_size += attempt_size;
      }
    }
    
    // aligned part       --  [PRE_OPL,POST_FIRST)
    U64 read_opl_block_base = AlignDownPow2(read_opl, READ_BLOCK_SIZE);
    U64 post_first = Max(read_opl_block_base, pre_opl);
    if (!done && pre_opl < post_first){
      U64 off = pre_opl - src_address;
      U64 attempt_size = post_first - pre_opl;
      U64 actual_size = demon_read_memory_amap_aligned(process, (U8*)dst + off,
                                                       pre_opl, attempt_size);
      read_size += actual_size;
      if (actual_size < attempt_size){
        done = 1;
      }
    }
    
    // post-aligned part  --  [POST_FIRST,READ_OPL)
    if (!done && post_first < read_opl){
      U64 off = post_first - src_address;
      U64 attempt_size = read_opl - post_first;
      if (!demon_read_memory(process, (U8*)dst + off, post_first, attempt_size)){
        done = 1;
      }
      else
      {
        read_size += attempt_size;
      }
    }
    
    demon_access_end();
  }
  
  U64 result = read_size;
  return(result);
}

#undef READ_BLOCK_SIZE

//- rjf: thread registers reading/writing

internal void*
demon_read_regs(DEMON_Handle thread){
  void *result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      result = demon_accel_read_regs(entity);
    }
    demon_access_end();
  }
  return(result);
}

internal B32
demon_write_regs(DEMON_Handle thread, void *data){
  B32 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      demon_accel_write_regs(entity, data);
      result = 1;
    }
    demon_access_end();
  }
  return(result);
}

internal U64
demon_read_ip(DEMON_Handle thread){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      void *regs = demon_accel_read_regs(entity);
      result = regs_rip_from_arch_block((Architecture)entity->arch, regs);
    }
    demon_access_end();
  }
  return(result);
}

internal U64
demon_read_sp(DEMON_Handle thread){
  U64 result = 0;
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      void *regs = demon_accel_read_regs(entity);
      result = regs_rsp_from_arch_block((Architecture)entity->arch, regs);
    }
    demon_access_end();
  }
  return(result);
}

internal void
demon_write_ip(DEMON_Handle thread, U64 ip){
  if (demon_access_begin()){
    DEMON_Entity *entity = demon_ent_ptr_from_handle(thread);
    if (entity != 0 &&
        entity->kind == DEMON_EntityKind_Thread){
      void *regs = demon_accel_read_regs(entity);
      regs_arch_block_write_rip((Architecture)entity->arch, regs, ip);
      demon_accel_write_regs(entity, regs);
    }
    demon_access_end();
  }
}

////////////////////////////////
//~ rjf: Process Listing

internal void
demon_proc_iter_begin(DEMON_ProcessIter *iter){
  demon_os_proc_iter_begin(iter);
}

internal B32
demon_proc_iter_next(Arena *arena, DEMON_ProcessIter *iter, DEMON_ProcessInfo *info_out){
  return(demon_os_proc_iter_next(arena, iter, info_out));
}

internal void
demon_proc_iter_end(DEMON_ProcessIter *iter){
  demon_os_proc_iter_end(iter);
}
