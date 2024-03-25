// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/ctrl.meta.c"

////////////////////////////////
//~ rjf: Basic Type Functions

internal U64
ctrl_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
ctrl_hash_from_machine_id_handle(CTRL_MachineID machine_id, DMN_Handle handle)
{
  U64 buf[] = {machine_id, handle.u64[0]};
  U64 hash = ctrl_hash_from_string(str8((U8 *)buf, sizeof(buf)));
  return hash;
}

internal CTRL_EventCause
ctrl_event_cause_from_dmn_event_kind(DMN_EventKind event_kind)
{
  CTRL_EventCause cause = CTRL_EventCause_Null;
  switch(event_kind)
  {
    default:{}break;
    case DMN_EventKind_Error:    {cause = CTRL_EventCause_Error;}break;
    case DMN_EventKind_Exception:{cause = CTRL_EventCause_InterruptedByException;}break;
    case DMN_EventKind_Trap:     {cause = CTRL_EventCause_InterruptedByTrap;}break;
    case DMN_EventKind_Halt:     {cause = CTRL_EventCause_InterruptedByHalt;}break;
  }
  return cause;
}

////////////////////////////////
//~ rjf: Machine/Handle Pair Type Functions

internal void
ctrl_machine_id_handle_pair_list_push(Arena *arena, CTRL_MachineIDHandlePairList *list, CTRL_MachineIDHandlePair *pair)
{
  CTRL_MachineIDHandlePairNode *n = push_array(arena, CTRL_MachineIDHandlePairNode, 1);
  MemoryCopyStruct(&n->v, pair);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal CTRL_MachineIDHandlePairList
ctrl_machine_id_handle_pair_list_copy(Arena *arena, CTRL_MachineIDHandlePairList *src)
{
  CTRL_MachineIDHandlePairList dst = {0};
  for(CTRL_MachineIDHandlePairNode *n = src->first; n != 0; n = n->next)
  {
    ctrl_machine_id_handle_pair_list_push(arena, &dst, &n->v);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Trap Type Functions

internal void
ctrl_trap_list_push(Arena *arena, CTRL_TrapList *list, CTRL_Trap *trap)
{
  CTRL_TrapNode *node = push_array(arena, CTRL_TrapNode, 1);
  MemoryCopyStruct(&node->v, trap);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal CTRL_TrapList
ctrl_trap_list_copy(Arena *arena, CTRL_TrapList *src)
{
  CTRL_TrapList dst = {0};
  for(CTRL_TrapNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    ctrl_trap_list_push(arena, &dst, &src_n->v);
  }
  return dst;
}

////////////////////////////////
//~ rjf: User Breakpoint Type Functions

internal void
ctrl_user_breakpoint_list_push(Arena *arena, CTRL_UserBreakpointList *list, CTRL_UserBreakpoint *bp)
{
  CTRL_UserBreakpointNode *n = push_array(arena, CTRL_UserBreakpointNode, 1);
  MemoryCopyStruct(&n->v, bp);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal CTRL_UserBreakpointList
ctrl_user_breakpoint_list_copy(Arena *arena, CTRL_UserBreakpointList *src)
{
  CTRL_UserBreakpointList dst = {0};
  for(CTRL_UserBreakpointNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    CTRL_UserBreakpoint dst_bp = zero_struct;
    MemoryCopyStruct(&dst_bp, &src_n->v);
    dst_bp.string = push_str8_copy(arena, src_n->v.string);
    dst_bp.condition = push_str8_copy(arena, src_n->v.condition);
    ctrl_user_breakpoint_list_push(arena, &dst, &dst_bp);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Message Type Functions

//- rjf: deep copying

internal void
ctrl_msg_deep_copy(Arena *arena, CTRL_Msg *dst, CTRL_Msg *src)
{
  MemoryCopyStruct(dst, src);
  dst->path                 = push_str8_copy(arena, src->path);
  dst->strings              = str8_list_copy(arena, &src->strings);
  dst->cmd_line_string_list = str8_list_copy(arena, &src->cmd_line_string_list);
  dst->env_string_list      = str8_list_copy(arena, &src->env_string_list);
  dst->traps                = ctrl_trap_list_copy(arena, &src->traps);
  dst->user_bps             = ctrl_user_breakpoint_list_copy(arena, &src->user_bps);
  dst->freeze_state_threads = ctrl_machine_id_handle_pair_list_copy(arena, &src->freeze_state_threads);
}

//- rjf: list building

internal CTRL_Msg *
ctrl_msg_list_push(Arena *arena, CTRL_MsgList *list)
{
  CTRL_MsgNode *n = push_array(arena, CTRL_MsgNode, 1);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  CTRL_Msg *msg = &n->v;
  return msg;
}

//- rjf: serialization

internal String8
ctrl_serialized_string_from_msg_list(Arena *arena, CTRL_MsgList *msgs)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List msgs_srlzed = {0};
  str8_serial_begin(scratch.arena, &msgs_srlzed);
  {
    // rjf: write message count
    str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msgs->count);
    
    // rjf: write all message data
    for(CTRL_MsgNode *msg_n = msgs->first; msg_n != 0; msg_n = msg_n->next)
    {
      CTRL_Msg *msg = &msg_n->v;
      
      // rjf: write flat parts
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->kind);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->msg_id);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->machine_id);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->entity);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->parent);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->entity_id);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->exit_code);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->env_inherit);
      str8_serial_push_array (scratch.arena, &msgs_srlzed, &msg->exception_code_filters[0], ArrayCount(msg->exception_code_filters));
      
      // rjf: write path string
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->path.size);
      str8_serial_push_data(scratch.arena, &msgs_srlzed, msg->path.str, msg->path.size);
      
      // rjf: write general string list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->strings.node_count);
      for(String8Node *n = msg->strings.first; n != 0; n = n->next)
      {
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &n->string.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, n->string.str, n->string.size);
      }
      
      // rjf: write command line string list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->cmd_line_string_list.node_count);
      for(String8Node *n = msg->cmd_line_string_list.first; n != 0; n = n->next)
      {
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &n->string.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, n->string.str, n->string.size);
      }
      
      // rjf: write environment string list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->env_string_list.node_count);
      for(String8Node *n = msg->env_string_list.first; n != 0; n = n->next)
      {
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &n->string.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, n->string.str, n->string.size);
      }
      
      // rjf: write trap list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->traps.count);
      for(CTRL_TrapNode *n = msg->traps.first; n != 0; n = n->next)
      {
        CTRL_Trap *trap = &n->v;
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &trap->flags);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &trap->vaddr);
      }
      
      // rjf: write user breakpoint list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->user_bps.count);
      for(CTRL_UserBreakpointNode *n = msg->user_bps.first; n != 0; n = n->next)
      {
        CTRL_UserBreakpoint *bp = &n->v;
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->kind);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->string.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, bp->string.str, bp->string.size);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->pt);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->u64);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->condition.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, bp->condition.str, bp->condition.size);
      }
      
      // rjf: write freeze state thread list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->freeze_state_threads.count);
      for(CTRL_MachineIDHandlePairNode *n = msg->freeze_state_threads.first; n != 0; n = n->next)
      {
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &n->v);
      }
      
      // rjf: write freeze state
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->freeze_state_is_frozen);
    }
  }
  String8 string = str8_serial_end(arena, &msgs_srlzed);
  scratch_end(scratch);
  return string;
}

internal CTRL_MsgList
ctrl_msg_list_from_serialized_string(Arena *arena, String8 string)
{
  CTRL_MsgList msgs = {0};
  {
    U64 read_off = 0;
    
    // rjf: read message count
    U64 msg_count = 0;
    read_off += str8_deserial_read_struct(string, read_off, &msg_count);
    
    // rjf: read data for all messages
    for(U64 msg_idx = 0; msg_idx < msg_count; msg_idx += 1)
    {
      // rjf: construct message
      CTRL_MsgNode *msg_node = push_array(arena, CTRL_MsgNode, 1);
      SLLQueuePush(msgs.first, msgs.last, msg_node);
      msgs.count += 1;
      CTRL_Msg *msg = &msg_node->v;
      
      // rjf: read flat data
      read_off += str8_deserial_read_struct(string, read_off, &msg->kind);
      read_off += str8_deserial_read_struct(string, read_off, &msg->msg_id);
      read_off += str8_deserial_read_struct(string, read_off, &msg->machine_id);
      read_off += str8_deserial_read_struct(string, read_off, &msg->entity);
      read_off += str8_deserial_read_struct(string, read_off, &msg->parent);
      read_off += str8_deserial_read_struct(string, read_off, &msg->entity_id);
      read_off += str8_deserial_read_struct(string, read_off, &msg->exit_code);
      read_off += str8_deserial_read_struct(string, read_off, &msg->env_inherit);
      read_off += str8_deserial_read_array (string, read_off, &msg->exception_code_filters[0], ArrayCount(msg->exception_code_filters));
      
      // rjf: read path string
      read_off += str8_deserial_read_struct(string, read_off, &msg->path.size);
      msg->path.str = push_array_no_zero(arena, U8, msg->path.size);
      read_off += str8_deserial_read(string, read_off, msg->path.str, msg->path.size, 1);
      
      // rjf: read general string list
      U64 string_list_string_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &string_list_string_count);
      for(U64 idx = 0; idx < string_list_string_count; idx += 1)
      {
        String8 str = {0};
        read_off += str8_deserial_read_struct(string, read_off, &str.size);
        str.str = push_array_no_zero(arena, U8, str.size);
        read_off += str8_deserial_read(string, read_off, str.str, str.size, 1);
        str8_list_push(arena, &msg->strings, str);
      }
      
      // rjf: read command line string list
      U64 cmd_line_string_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &cmd_line_string_count);
      for(U64 idx = 0; idx < cmd_line_string_count; idx += 1)
      {
        String8 cmd_line_str = {0};
        read_off += str8_deserial_read_struct(string, read_off, &cmd_line_str.size);
        cmd_line_str.str = push_array_no_zero(arena, U8, cmd_line_str.size);
        read_off += str8_deserial_read(string, read_off, cmd_line_str.str, cmd_line_str.size, 1);
        str8_list_push(arena, &msg->cmd_line_string_list, cmd_line_str);
      }
      
      // rjf: read environment string list
      U64 env_string_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &env_string_count);
      for(U64 idx = 0; idx < env_string_count; idx += 1)
      {
        String8 env_str = {0};
        read_off += str8_deserial_read_struct(string, read_off, &env_str.size);
        env_str.str = push_array_no_zero(arena, U8, env_str.size);
        read_off += str8_deserial_read(string, read_off, env_str.str, env_str.size, 1);
        str8_list_push(arena, &msg->env_string_list, env_str);
      }
      
      // rjf: read trap list
      U64 trap_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &trap_count);
      for(U64 idx = 0; idx < trap_count; idx += 1)
      {
        CTRL_TrapNode *n = push_array(arena, CTRL_TrapNode, 1);
        SLLQueuePush(msg->traps.first, msg->traps.last, n);
        msg->traps.count += 1;
        CTRL_Trap *trap = &n->v;
        read_off += str8_deserial_read_struct(string, read_off, &trap->flags);
        read_off += str8_deserial_read_struct(string, read_off, &trap->vaddr);
      }
      
      // rjf: read user breakpoint list
      U64 user_bp_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &user_bp_count);
      for(U64 idx = 0; idx < user_bp_count; idx += 1)
      {
        CTRL_UserBreakpointNode *n = push_array(arena, CTRL_UserBreakpointNode, 1);
        SLLQueuePush(msg->user_bps.first, msg->user_bps.last, n);
        msg->user_bps.count += 1;
        CTRL_UserBreakpoint *bp = &n->v;
        read_off += str8_deserial_read_struct(string, read_off, &bp->kind);
        read_off += str8_deserial_read_struct(string, read_off, &bp->string.size);
        bp->string.str = push_array_no_zero(arena, U8, bp->string.size);
        read_off += str8_deserial_read(string, read_off, bp->string.str, bp->string.size, 1);
        read_off += str8_deserial_read_struct(string, read_off, &bp->pt);
        read_off += str8_deserial_read_struct(string, read_off, &bp->u64);
        read_off += str8_deserial_read_struct(string, read_off, &bp->condition.size);
        bp->condition.str = push_array_no_zero(arena, U8, bp->condition.size);
        read_off += str8_deserial_read(string, read_off, bp->condition.str, bp->condition.size, 1);
      }
      
      // rjf: read freeze state thread list
      U64 frozen_thread_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &frozen_thread_count);
      for(U64 idx = 0; idx < frozen_thread_count; idx += 1)
      {
        CTRL_MachineIDHandlePair pair = {0};
        read_off += str8_deserial_read_struct(string, read_off, &pair);
        ctrl_machine_id_handle_pair_list_push(arena, &msg->freeze_state_threads, &pair);
      }
      
      // rjf: read freeze state
      read_off += str8_deserial_read_struct(string, read_off, &msg->freeze_state_is_frozen);
    }
  }
  return msgs;
}

////////////////////////////////
//~ rjf: Event Type Functions

//- rjf: list building

internal CTRL_Event *
ctrl_event_list_push(Arena *arena, CTRL_EventList *list)
{
  CTRL_EventNode *n = push_array(arena, CTRL_EventNode, 1);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  CTRL_Event *event = &n->v;
  return event;
}

internal void
ctrl_event_list_concat_in_place(CTRL_EventList *dst, CTRL_EventList *to_push)
{
  if(dst->last == 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  else if(to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
  }
  MemoryZeroStruct(to_push);
}

//- rjf: serialization

internal String8
ctrl_serialized_string_from_event(Arena *arena, CTRL_Event *event)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);;
  {
    str8_serial_push_struct(scratch.arena, &srl, &event->kind);
    str8_serial_push_struct(scratch.arena, &srl, &event->cause);
    str8_serial_push_struct(scratch.arena, &srl, &event->exception_kind);
    str8_serial_push_struct(scratch.arena, &srl, &event->msg_id);
    str8_serial_push_struct(scratch.arena, &srl, &event->machine_id);
    str8_serial_push_struct(scratch.arena, &srl, &event->entity);
    str8_serial_push_struct(scratch.arena, &srl, &event->parent);
    str8_serial_push_struct(scratch.arena, &srl, &event->arch);
    str8_serial_push_struct(scratch.arena, &srl, &event->u64_code);
    str8_serial_push_struct(scratch.arena, &srl, &event->entity_id);
    str8_serial_push_struct(scratch.arena, &srl, &event->vaddr_rng);
    str8_serial_push_struct(scratch.arena, &srl, &event->rip_vaddr);
    str8_serial_push_struct(scratch.arena, &srl, &event->stack_base);
    str8_serial_push_struct(scratch.arena, &srl, &event->tls_root);
    str8_serial_push_struct(scratch.arena, &srl, &event->exception_code);
    str8_serial_push_struct(scratch.arena, &srl, &event->string.size);
    str8_serial_push_data(scratch.arena, &srl, event->string.str, event->string.size);
  }
  String8 string = str8_serial_end(arena, &srl);
  scratch_end(scratch);
  return string;
}

internal CTRL_Event
ctrl_event_from_serialized_string(Arena *arena, String8 string)
{
  CTRL_Event event = zero_struct;
  {
    U64 read_off = 0;
    read_off += str8_deserial_read_struct(string, read_off, &event.kind);
    read_off += str8_deserial_read_struct(string, read_off, &event.cause);
    read_off += str8_deserial_read_struct(string, read_off, &event.exception_kind);
    read_off += str8_deserial_read_struct(string, read_off, &event.msg_id);
    read_off += str8_deserial_read_struct(string, read_off, &event.machine_id);
    read_off += str8_deserial_read_struct(string, read_off, &event.entity);
    read_off += str8_deserial_read_struct(string, read_off, &event.parent);
    read_off += str8_deserial_read_struct(string, read_off, &event.arch);
    read_off += str8_deserial_read_struct(string, read_off, &event.u64_code);
    read_off += str8_deserial_read_struct(string, read_off, &event.entity_id);
    read_off += str8_deserial_read_struct(string, read_off, &event.vaddr_rng);
    read_off += str8_deserial_read_struct(string, read_off, &event.rip_vaddr);
    read_off += str8_deserial_read_struct(string, read_off, &event.stack_base);
    read_off += str8_deserial_read_struct(string, read_off, &event.tls_root);
    read_off += str8_deserial_read_struct(string, read_off, &event.exception_code);
    read_off += str8_deserial_read_struct(string, read_off, &event.string.size);
    event.string.str = push_array_no_zero(arena, U8, event.string.size);
    read_off += str8_deserial_read(string, read_off, event.string.str, event.string.size, 1);
  }
  return event;
}

////////////////////////////////
//~ rjf: Entity Type Functions

//- rjf: cache creation/destruction

internal CTRL_EntityStore *
ctrl_entity_store_alloc(void)
{
  Arena *arena = arena_alloc();
  CTRL_EntityStore *store = push_array(arena, CTRL_EntityStore, 1);
  store->arena = arena;
  store->hash_slots_count = 1024;
  store->hash_slots = push_array(arena, CTRL_EntityHashSlot, store->hash_slots_count);
  return store;
}

internal void
ctrl_entity_store_release(CTRL_EntityStore *cache)
{
  arena_release(cache->arena);
}

internal U64
ctrl_name_bucket_idx_from_string_size(U64 size)
{
  U64 size_rounded = u64_up_to_pow2(size+1);
  size_rounded = ClampBot((1<<4), size_rounded);
  U64 bucket_idx = 0;
  switch(size_rounded)
  {
    case 1<<4: {bucket_idx = 0;}break;
    case 1<<5: {bucket_idx = 1;}break;
    case 1<<6: {bucket_idx = 2;}break;
    case 1<<7: {bucket_idx = 3;}break;
    case 1<<8: {bucket_idx = 4;}break;
    case 1<<9: {bucket_idx = 5;}break;
    case 1<<10:{bucket_idx = 6;}break;
    default:{bucket_idx = ArrayCount(((CTRL_EntityStore *)0)->free_string_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
ctrl_entity_string_alloc(CTRL_EntityStore *store, String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = ctrl_name_bucket_idx_from_string_size(string.size);
  CTRL_EntityStringChunkNode *node = store->free_string_chunks[bucket_idx];
  
  // rjf: pull from bucket free list
  if(node != 0)
  {
    if(bucket_idx == ArrayCount(store->free_string_chunks)-1)
    {
      node = 0;
      CTRL_EntityStringChunkNode *prev = 0;
      for(CTRL_EntityStringChunkNode *n = store->free_string_chunks[bucket_idx];
          n != 0;
          prev = n, n = n->next)
      {
        if(n->size >= string.size+1)
        {
          if(prev == 0)
          {
            store->free_string_chunks[bucket_idx] = n->next;
          }
          else
          {
            prev->next = n->next;
          }
          node = n;
          break;
        }
      }
    }
    else
    {
      SLLStackPop(store->free_string_chunks[bucket_idx]);
    }
  }
  
  // rjf: no found node -> allocate new
  if(node == 0)
  {
    U64 chunk_size = 0;
    if(bucket_idx < ArrayCount(store->free_string_chunks)-1)
    {
      chunk_size = 1<<(bucket_idx+4);
    }
    else
    {
      chunk_size = u64_up_to_pow2(string.size);
    }
    U8 *chunk_memory = push_array(store->arena, U8, chunk_size);
    node = (CTRL_EntityStringChunkNode *)chunk_memory;
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
ctrl_entity_string_release(CTRL_EntityStore *store, String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = ctrl_name_bucket_idx_from_string_size(string.size);
  CTRL_EntityStringChunkNode *node = (CTRL_EntityStringChunkNode *)string.str;
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(store->free_string_chunks[bucket_idx], node);
}

//- rjf: entity construction/deletion

internal CTRL_Entity *
ctrl_entity_alloc(CTRL_EntityStore *store, CTRL_Entity *parent, CTRL_EntityKind kind, Architecture arch, CTRL_MachineID machine_id, DMN_Handle handle)
{
  CTRL_Entity *entity = &ctrl_entity_nil;
  {
    // rjf: allocate
    entity = store->free;
    {
      if(entity != 0)
      {
        SLLStackPop(store->free);
      }
      else
      {
        entity = push_array_no_zero(store->arena, CTRL_Entity, 1);
      }
      MemoryZeroStruct(entity);
    }
    
    // rjf: fill
    {
      entity->kind        = kind;
      entity->arch        = arch;
      entity->machine_id  = machine_id;
      entity->handle      = handle;
      entity->parent      = parent;
      entity->next = entity->prev = entity->first = entity->last = &ctrl_entity_nil;
      if(parent != &ctrl_entity_nil)
      {
        DLLPushBack_NPZ(&ctrl_entity_nil, parent->first, parent->last, entity, next, prev);
      }
    }
    
    // rjf: insert into hash map
    {
      U64 hash = ctrl_hash_from_machine_id_handle(machine_id, handle);
      U64 slot_idx = hash%store->hash_slots_count;
      CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
      CTRL_EntityHashNode *node = 0;
      for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
      {
        if(n->entity->machine_id == machine_id && dmn_handle_match(n->entity->handle, handle))
        {
          node = n;
          break;
        }
      }
      if(node == 0)
      {
        node = store->hash_node_free;
        if(node != 0)
        {
          SLLStackPop(store->hash_node_free);
        }
        else
        {
          node = push_array_no_zero(store->arena, CTRL_EntityHashNode, 1);
        }
        MemoryZeroStruct(node);
        DLLPushBack(slot->first, slot->last, node);
        node->entity = entity;
      }
    }
  }
  return entity;
}

internal void
ctrl_entity_release(CTRL_EntityStore *store, CTRL_Entity *entity)
{
  // rjf: unhook root
  if(entity->parent != &ctrl_entity_nil)
  {
    DLLRemove_NPZ(&ctrl_entity_nil, entity->parent->first, entity->parent->last, entity, next, prev);
  }
  
  // rjf: walk every entity in this tree, free each
  if(entity != &ctrl_entity_nil)
  {
    Temp scratch = scratch_begin(0, 0);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      CTRL_Entity *e;
    };
    Task start_task = {0, entity};
    Task *first_task = &start_task;
    Task *last_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      for(CTRL_Entity *child = t->e->first; child != &ctrl_entity_nil; child = child->next)
      {
        Task *t = push_array(scratch.arena, Task, 1);
        t->e = child;
        SLLQueuePush(first_task, last_task, t);
      }
      
      // rjf: free entity
      SLLStackPush(store->free, t->e);
      
      // rjf: remove from hash map
      {
        U64 hash = ctrl_hash_from_machine_id_handle(t->e->machine_id, t->e->handle);
        U64 slot_idx = hash%store->hash_slots_count;
        CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
        CTRL_EntityHashNode *node = 0;
        for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
        {
          if(n->entity->machine_id == t->e->machine_id && dmn_handle_match(n->entity->handle, t->e->handle))
          {
            DLLRemove(slot->first, slot->last, n);
            SLLStackPush(store->hash_node_free, n);
            break;
          }
        }
      }
    }
    scratch_end(scratch);
  }
}

//- rjf: entity equipment

internal void
ctrl_entity_equip_string(CTRL_EntityStore *store, CTRL_Entity *entity, String8 string)
{
  if(entity->string.size != 0)
  {
    ctrl_entity_string_release(store, entity->string);
  }
  entity->string = ctrl_entity_string_alloc(store, string);
}

//- rjf: entity store lookups

internal CTRL_Entity *
ctrl_entity_from_machine_id_handle(CTRL_EntityStore *store, CTRL_MachineID machine_id, DMN_Handle handle)
{
  CTRL_Entity *entity = &ctrl_entity_nil;
  {
    U64 hash = ctrl_hash_from_machine_id_handle(machine_id, handle);
    U64 slot_idx = hash%store->hash_slots_count;
    CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
    CTRL_EntityHashNode *node = 0;
    for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
    {
      if(n->entity->machine_id == machine_id && dmn_handle_match(n->entity->handle, handle))
      {
        entity = n->entity;
        break;
      }
    }
  }
  return entity;
}

//- rjf: applying events to entity caches

internal void
ctrl_entity_store_apply_events(CTRL_EntityStore *store, CTRL_EventList *list)
{
  //- rjf: construct root-level entities
  if(!store->root)
  {
    CTRL_Entity *root = store->root = ctrl_entity_alloc(store, &ctrl_entity_nil, CTRL_EntityKind_Root, Architecture_Null, 0, dmn_handle_zero());
    CTRL_Entity *local_machine = ctrl_entity_alloc(store, root, CTRL_EntityKind_Machine, architecture_from_context(), CTRL_MachineID_Local, dmn_handle_zero());
    (void)local_machine;
  }
  
  //- rjf: scan events & construct entities
  for(CTRL_EventNode *n = list->first; n != 0; n = n->next)
  {
    CTRL_Event *event = &n->v;
    switch(event->kind)
    {
      //- rjf: processes
      case CTRL_EventKind_NewProc:
      {
        CTRL_Entity *machine = ctrl_entity_from_machine_id_handle(store, event->machine_id, dmn_handle_zero());
        CTRL_Entity *process = ctrl_entity_alloc(store, machine, CTRL_EntityKind_Process, event->arch, event->machine_id, event->entity);
      }break;
      case CTRL_EventKind_EndProc:
      {
        CTRL_Entity *process = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->entity);
        ctrl_entity_release(store, process);
      }break;
      
      //- rjf: threads
      case CTRL_EventKind_NewThread:
      {
        CTRL_Entity *process = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->parent);
        CTRL_Entity *thread = ctrl_entity_alloc(store, process, CTRL_EntityKind_Thread, event->arch, event->machine_id, event->entity);
      }break;
      case CTRL_EventKind_EndThread:
      {
        CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->entity);
        ctrl_entity_release(store, thread);
      }break;
      case CTRL_EventKind_ThreadName:
      {
        CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->entity);
        ctrl_entity_equip_string(store, thread, event->string);
      }break;
      
      //- rjf: modules
      case CTRL_EventKind_NewModule:
      {
        CTRL_Entity *process = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->parent);
        CTRL_Entity *module = ctrl_entity_alloc(store, process, CTRL_EntityKind_Module, event->arch, event->machine_id, event->entity);
        ctrl_entity_equip_string(store, module, event->string);
        module->vaddr_range = event->vaddr_rng;
      }break;
      case CTRL_EventKind_EndModule:
      {
        CTRL_Entity *module = ctrl_entity_from_machine_id_handle(store, event->machine_id, event->entity);
        ctrl_entity_release(store, module);
      }break;
    }
  }
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
ctrl_init(void)
{
  Arena *arena = arena_alloc();
  ctrl_state = push_array(arena, CTRL_State, 1);
  ctrl_state->arena = arena;
  for(Architecture arch = (Architecture)0; arch < Architecture_COUNT; arch = (Architecture)(arch+1))
  {
    String8 *reg_names = regs_reg_code_string_table_from_architecture(arch);
    U64 reg_count = regs_reg_code_count_from_architecture(arch);
    String8 *alias_names = regs_alias_code_string_table_from_architecture(arch);
    U64 alias_count = regs_alias_code_count_from_architecture(arch);
    ctrl_state->arch_string2reg_tables[arch] = eval_string2num_map_make(ctrl_state->arena, 256);
    ctrl_state->arch_string2alias_tables[arch] = eval_string2num_map_make(ctrl_state->arena, 256);
    for(U64 idx = 1; idx < reg_count; idx += 1)
    {
      eval_string2num_map_insert(ctrl_state->arena, &ctrl_state->arch_string2reg_tables[arch], reg_names[idx], idx);
    }
    for(U64 idx = 1; idx < alias_count; idx += 1)
    {
      eval_string2num_map_insert(ctrl_state->arena, &ctrl_state->arch_string2alias_tables[arch], alias_names[idx], idx);
    }
  }
  ctrl_state->process_memory_cache.slots_count = 256;
  ctrl_state->process_memory_cache.slots = push_array(arena, CTRL_ProcessMemoryCacheSlot, ctrl_state->process_memory_cache.slots_count);
  ctrl_state->process_memory_cache.stripes_count = os_logical_core_count();
  ctrl_state->process_memory_cache.stripes = push_array(arena, CTRL_ProcessMemoryCacheStripe, ctrl_state->process_memory_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->process_memory_cache.stripes_count; idx += 1)
  {
    ctrl_state->process_memory_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
    ctrl_state->process_memory_cache.stripes[idx].cv = os_condition_variable_alloc();
  }
  ctrl_state->thread_reg_cache.slots_count = 1024;
  ctrl_state->thread_reg_cache.slots = push_array(arena, CTRL_ThreadRegCacheSlot, ctrl_state->thread_reg_cache.slots_count);
  ctrl_state->thread_reg_cache.stripes_count = os_logical_core_count();
  ctrl_state->thread_reg_cache.stripes = push_array(arena, CTRL_ThreadRegCacheStripe, ctrl_state->thread_reg_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->thread_reg_cache.stripes_count; idx += 1)
  {
    ctrl_state->thread_reg_cache.stripes[idx].arena = arena_alloc();
    ctrl_state->thread_reg_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  ctrl_state->u2c_ring_size = KB(64);
  ctrl_state->u2c_ring_base = push_array_no_zero(arena, U8, ctrl_state->u2c_ring_size);
  ctrl_state->u2c_ring_mutex = os_mutex_alloc();
  ctrl_state->u2c_ring_cv = os_condition_variable_alloc();
  ctrl_state->c2u_ring_size = KB(64);
  ctrl_state->c2u_ring_base = push_array_no_zero(arena, U8, ctrl_state->c2u_ring_size);
  ctrl_state->c2u_ring_mutex = os_mutex_alloc();
  ctrl_state->c2u_ring_cv = os_condition_variable_alloc();
  ctrl_state->ctrl_thread_entity_store = ctrl_entity_store_alloc();
  ctrl_state->dmn_event_arena = arena_alloc();
  ctrl_state->user_entry_point_arena = arena_alloc();
  for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
  {
    if(ctrl_exception_code_kind_default_enable_table[k])
    {
      ctrl_state->exception_code_filters[k/64] |= 1ull<<(k%64);
    }
  }
  ctrl_state->u2ms_ring_size = KB(64);
  ctrl_state->u2ms_ring_base = push_array(arena, U8, ctrl_state->u2ms_ring_size);
  ctrl_state->u2ms_ring_mutex = os_mutex_alloc();
  ctrl_state->u2ms_ring_cv = os_condition_variable_alloc();
  ctrl_state->ctrl_thread = os_launch_thread(ctrl_thread__entry_point, 0, 0);
  ctrl_state->ms_thread_count = Clamp(1, os_logical_core_count()-1, 4);
  ctrl_state->ms_threads = push_array(arena, OS_Handle, ctrl_state->ms_thread_count);
  for(U64 idx = 0; idx < ctrl_state->ms_thread_count; idx += 1)
  {
    ctrl_state->ms_threads[idx] = os_launch_thread(ctrl_mem_stream_thread__entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Wakeup Callback Registration

internal void
ctrl_set_wakeup_hook(CTRL_WakeupFunctionType *wakeup_hook)
{
  ctrl_state->wakeup_hook = wakeup_hook;
}

////////////////////////////////
//~ rjf: Process Memory Functions

//- rjf: process memory cache interaction

internal U128
ctrl_hash_store_key_from_process_vaddr_range(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, B32 zero_terminated)
{
  U64 key_hash_data[] =
  {
    (U64)machine_id,
    (U64)process.u64[0],
    range.min,
    range.max,
    (U64)zero_terminated,
  };
  U128 key = hs_hash_from_data(str8((U8*)key_hash_data, sizeof(key_hash_data)));
  return key;
}

internal U128
ctrl_stored_hash_from_process_vaddr_range(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, B32 zero_terminated, B32 *out_is_stale, U64 endt_us)
{
  U128 result = {0};
  U64 size = dim_1u64(range);
  if(size != 0) for(;;)
  {
    CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
    U64 process_hash = ctrl_hash_from_string(str8_struct(&process));
    U64 process_slot_idx = process_hash%cache->slots_count;
    U64 process_stripe_idx = process_slot_idx%cache->stripes_count;
    CTRL_ProcessMemoryCacheSlot *process_slot = &cache->slots[process_slot_idx];
    CTRL_ProcessMemoryCacheStripe *process_stripe = &cache->stripes[process_stripe_idx];
    U64 range_hash = ctrl_hash_from_string(str8_struct(&range));
    
    //- rjf: try to read from cache
    B32 is_good = 0;
    B32 is_stale = 0;
    OS_MutexScopeR(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
            {
              result = range_n->hash;
              is_good = 1;
              is_stale = (range_n->mem_gen != dmn_mem_gen());
              goto read_cache__break_all;
            }
          }
        }
      }
      read_cache__break_all:;
    }
    
    //- rjf: not good -> create process cache node if necessary
    if(!is_good)
    {
      OS_MutexScopeW(process_stripe->rw_mutex)
      {
        B32 process_node_exists = 0;
        for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
        {
          if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
          {
            process_node_exists = 1;
            break;
          }
        }
        if(!process_node_exists)
        {
          Arena *node_arena = arena_alloc();
          CTRL_ProcessMemoryCacheNode *node = push_array(node_arena, CTRL_ProcessMemoryCacheNode, 1);
          node->arena = node_arena;
          node->machine_id = machine_id;
          node->process = process;
          node->range_hash_slots_count = 1024;
          node->range_hash_slots = push_array(node_arena, CTRL_ProcessMemoryRangeHashSlot, node->range_hash_slots_count);
          DLLPushBack(process_slot->first, process_slot->last, node);
        }
      }
    }
    
    //- rjf: not good -> create range node if necessary
    U64 last_time_requested_us = 0;
    if(!is_good)
    {
      OS_MutexScopeW(process_stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
        {
          if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
          {
            U64 range_slot_idx = range_hash%n->range_hash_slots_count;
            CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
            B32 range_node_exists = 0;
            for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
            {
              if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
              {
                last_time_requested_us = range_n->last_time_requested_us;
                range_node_exists = 1;
                break;
              }
            }
            if(!range_node_exists)
            {
              CTRL_ProcessMemoryRangeHashNode *range_n = push_array(n->arena, CTRL_ProcessMemoryRangeHashNode, 1);
              SLLQueuePush(range_slot->first, range_slot->last, range_n);
              range_n->vaddr_range = range;
              range_n->zero_terminated = zero_terminated;
              range_n->vaddr_range_clamped = range;
              {
                range_n->vaddr_range_clamped.max = Max(range_n->vaddr_range_clamped.max, range_n->vaddr_range_clamped.min);
                U64 max_size_cap = Min(max_U64-range_n->vaddr_range_clamped.min, GB(1));
                range_n->vaddr_range_clamped.max = Min(range_n->vaddr_range_clamped.max, range_n->vaddr_range_clamped.min+max_size_cap);
              }
              break;
            }
          }
        }
      }
    }
    
    //- rjf: not good, or is stale -> submit hash request
    if((!is_good || is_stale) && os_now_microseconds() >= last_time_requested_us+10000)
    {
      if(ctrl_u2ms_enqueue_req(machine_id, process, range, zero_terminated, endt_us)) OS_MutexScopeW(process_stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
        {
          if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
          {
            U64 range_slot_idx = range_hash%n->range_hash_slots_count;
            CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
            B32 range_node_exists = 0;
            for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
            {
              if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
              {
                range_n->last_time_requested_us = os_now_microseconds();
                break;
              }
            }
          }
        }
      }
    }
    
    //- rjf: out of time? -> exit
    if(os_now_microseconds() >= endt_us)
    {
      if(is_good && is_stale && out_is_stale)
      {
        out_is_stale[0] = 1;
      }
      break;
    }
    
    //- rjf: done? -> exit
    if(is_good && !is_stale)
    {
      break;
    }
  }
  return result;
}

//- rjf: process memory cache reading helpers

internal CTRL_ProcessMemorySlice
ctrl_query_cached_data_from_process_vaddr_range(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, U64 endt_us)
{
  CTRL_ProcessMemorySlice result = {0};
  if(range.max > range.min &&
     dim_1u64(range) <= MB(256) &&
     range.min <= 0x000FFFFFFFFFFFFFull &&
     range.max <= 0x000FFFFFFFFFFFFFull)
  {
    Temp scratch = scratch_begin(&arena, 1);
    HS_Scope *scope = hs_scope_open();
    CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
    
    //- rjf: unpack address range, prepare per-touched-page info
    U64 page_size = KB(4);
    Rng1U64 page_range = r1u64(AlignDownPow2(range.min, page_size), AlignPow2(range.max, page_size));
    U64 page_count = dim_1u64(page_range)/page_size;
    U128 *page_hashes = push_array(scratch.arena, U128, page_count);
    U128 *page_last_hashes = push_array(scratch.arena, U128, page_count);
    
    //- rjf: gather hashes & last-hashes for each page
    for(U64 page_idx = 0; page_idx < page_count; page_idx += 1)
    {
      U64 page_base_vaddr = page_range.min + page_idx*page_size;
      U128 page_key = ctrl_hash_store_key_from_process_vaddr_range(machine_id, process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0);
      B32 page_is_stale = 0;
      U128 page_hash = ctrl_stored_hash_from_process_vaddr_range(machine_id, process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0, &page_is_stale, endt_us);
      U128 page_last_hash = hs_hash_from_key(page_key, 1);
      result.stale = (result.stale || page_is_stale);
      page_hashes[page_idx] = page_hash;
      page_last_hashes[page_idx] = page_last_hash;
    }
    
    //- rjf: setup output buffers
    void *read_out = push_array(arena, U8, dim_1u64(range));
    U64 *byte_bad_flags = push_array(arena, U64, (dim_1u64(range)+63)/64);
    U64 *byte_changed_flags = push_array(arena, U64, (dim_1u64(range)+63)/64);
    
    //- rjf: iterate pages, fill output
    {
      U64 write_off = 0;
      for(U64 page_idx = 0; page_idx < page_count; page_idx += 1)
      {
        // rjf: read data for this page
        String8 data = hs_data_from_hash(scope, page_hashes[page_idx]);
        Rng1U64 data_vaddr_range = r1u64(page_range.min + page_idx*page_size, page_range.min + page_idx*page_size+data.size);
        
        // rjf: skip/chop bytes which are irrelevant for the actual requested read
        String8 in_range_data = data;
        if(page_idx == page_count-1 && data_vaddr_range.max > range.max)
        {
          in_range_data = str8_chop(in_range_data, data_vaddr_range.max-range.max);
        }
        if(page_idx == 0 && range.min > data_vaddr_range.min)
        {
          in_range_data = str8_skip(in_range_data, range.min-data_vaddr_range.min);
        }
        
        // rjf: write this chunk
        MemoryCopy((U8*)read_out+write_off, in_range_data.str, in_range_data.size);
        
        // rjf; if this page's data doesn't fill the entire range, mark
        // missing bytes as bad
        if(data.size < page_size)
        {
          for(U64 invalid_vaddr = data_vaddr_range.min+data.size;
              invalid_vaddr < data_vaddr_range.min + page_size;
              invalid_vaddr += 1)
          {
            if(contains_1u64(range, invalid_vaddr))
            {
              U64 idx_in_range = invalid_vaddr-range.min;
              byte_bad_flags[idx_in_range/64] |= (1ull<<(idx_in_range%64));
            }
          }
        }
        
        // rjf: if this page's hash & last_hash don't match, diff each byte &
        // fill out changed flags
        if(!u128_match(page_hashes[page_idx], page_last_hashes[page_idx]))
        {
          String8 last_data = hs_data_from_hash(scope, page_last_hashes[page_idx]);
          String8 in_range_last_data = last_data;
          if(page_idx == page_count-1 && data_vaddr_range.max > range.max)
          {
            in_range_last_data = str8_chop(in_range_last_data, data_vaddr_range.max-range.max);
          }
          if(page_idx == 0 && range.min > data_vaddr_range.min)
          {
            in_range_last_data = str8_skip(in_range_last_data, range.min-data_vaddr_range.min);
          }
          for(U64 idx = 0; idx < in_range_data.size; idx += 1)
          {
            U8 last_byte = idx < in_range_last_data.size ? in_range_last_data.str[idx] : 0;
            U8 now_byte  = idx < in_range_data.size ? in_range_data.str[idx] : 0;
            if(last_byte != now_byte)
            {
              U64 idx_in_read_out = write_off+idx;
              byte_changed_flags[idx_in_read_out/64] |= (1ull<<(idx_in_read_out%64));
            }
          }
        }
        
        // rjf: increment past this chunk
        write_off += in_range_data.size;
        if(data.size < page_size)
        {
          U64 missed_byte_count = page_size-data.size;
          write_off += missed_byte_count;
        }
      }
    }
    
    //- rjf: fill result
    result.data.str = (U8*)read_out;
    result.data.size = dim_1u64(range);
    result.byte_bad_flags = byte_bad_flags;
    result.byte_changed_flags = byte_changed_flags;
    if(byte_bad_flags != 0)
    {
      for(U64 idx = 0; idx < (dim_1u64(range)+63)/64; idx += 1)
      {
        result.any_byte_bad = result.any_byte_bad || !!result.byte_bad_flags[idx];
      }
    }
    if(byte_changed_flags != 0)
    {
      for(U64 idx = 0; idx < (dim_1u64(range)+63)/64; idx += 1)
      {
        result.any_byte_changed = result.any_byte_changed || !!result.byte_changed_flags[idx];
      }
    }
    
    hs_scope_close(scope);
    scratch_end(scratch);
  }
  return result;
}

internal CTRL_ProcessMemorySlice
ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, U64 vaddr, U64 limit, U64 endt_us)
{
  CTRL_ProcessMemorySlice result = ctrl_query_cached_data_from_process_vaddr_range(arena, machine_id, process, r1u64(vaddr, vaddr+limit), endt_us);
  for(U64 idx = 0; idx < result.data.size; idx += 1)
  {
    if(result.data.str[idx] == 0)
    {
      result.data.size = idx;
      break;
    }
  }
  return result;
}

//- rjf: process memory writing

internal B32
ctrl_process_write(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, void *src)
{
  ProfBeginFunction();
  B32 result = dmn_process_write(process, range, src);
  
  //- rjf: success -> wait for cache updates, for small regions - prefer relatively seamless
  // writes within calling frame's "view" of the memory, at the expense of a small amount of
  // time.
  if(result)
  {
    Temp scratch = scratch_begin(0, 0);
    U64 endt_us = os_now_microseconds()+5000;
    
    //- rjf: gather tasks for all affected cached regions
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      CTRL_MachineID machine_id;
      DMN_Handle process;
      Rng1U64 range;
    };
    Task *first_task = 0;
    Task *last_task = 0;
    CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
    for(U64 slot_idx = 0; slot_idx < cache->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%cache->stripes_count;
      CTRL_ProcessMemoryCacheSlot *slot = &cache->slots[slot_idx];
      CTRL_ProcessMemoryCacheStripe *stripe = &cache->stripes[stripe_idx];
      OS_MutexScopeW(stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *proc_n = slot->first; proc_n != 0; proc_n = proc_n->next)
        {
          for(U64 range_hash_idx = 0; range_hash_idx < proc_n->range_hash_slots_count; range_hash_idx += 1)
          {
            CTRL_ProcessMemoryRangeHashSlot *range_slot = &proc_n->range_hash_slots[range_hash_idx];
            for(CTRL_ProcessMemoryRangeHashNode *n = range_slot->first; n != 0; n = n->next)
            {
              Rng1U64 intersection_w_range = intersect_1u64(range, n->vaddr_range);
              if(dim_1u64(intersection_w_range) != 0 && dim_1u64(n->vaddr_range) <= KB(64))
              {
                Task *task = push_array(scratch.arena, Task, 1);
                task->machine_id = proc_n->machine_id;
                task->process = proc_n->process;
                task->range = n->vaddr_range;
                SLLQueuePush(first_task, last_task, task);
              }
            }
          }
        }
      }
    }
    
    //- rjf: for all tasks, wait for up-to-date results
    for(Task *task = first_task; task != 0; task = task->next)
    {
      Temp temp = temp_begin(scratch.arena);
      ctrl_query_cached_data_from_process_vaddr_range(temp.arena, task->machine_id, task->process, task->range, endt_us);
      temp_end(temp);
    }
    
    scratch_end(scratch);
  }
  
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Thread Register Functions

//- rjf: thread register cache reading

internal void *
ctrl_query_cached_reg_block_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_MachineID machine_id, DMN_Handle thread)
{
  CTRL_ThreadRegCache *cache = &ctrl_state->thread_reg_cache;
  CTRL_Entity *thread_entity = ctrl_entity_from_machine_id_handle(store, machine_id, thread);
  Architecture arch = thread_entity->arch;
  U64 reg_block_size = regs_block_size_from_architecture(arch);
  U64 hash = ctrl_hash_from_machine_id_handle(machine_id, thread);
  U64 slot_idx = hash%cache->slots_count;
  U64 stripe_idx = slot_idx%cache->stripes_count;
  CTRL_ThreadRegCacheSlot *slot = &cache->slots[slot_idx];
  CTRL_ThreadRegCacheStripe *stripe = &cache->stripes[stripe_idx];
  void *result = push_array(arena, U8, reg_block_size);
  OS_MutexScopeR(stripe->rw_mutex)
  {
    // rjf: find existing node
    CTRL_ThreadRegCacheNode *node = 0;
    for(CTRL_ThreadRegCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(n->machine_id == machine_id && dmn_handle_match(n->thread, thread))
      {
        node = n;
        break;
      }
    }
    
    // rjf: allocate existing node
    if(!node)
    {
      OS_MutexScopeRWPromote(stripe->rw_mutex)
      {
        for(CTRL_ThreadRegCacheNode *n = slot->first; n != 0; n = n->next)
        {
          if(n->machine_id == machine_id && dmn_handle_match(n->thread, thread))
          {
            node = n;
            break;
          }
        }
        if(!node)
        {
          node = push_array(stripe->arena, CTRL_ThreadRegCacheNode, 1);
          DLLPushBack(slot->first, slot->last, node);
          node->machine_id = machine_id;
          node->thread     = thread;
          node->block_size = reg_block_size;
          node->block      = push_array(stripe->arena, U8, reg_block_size);
        }
      }
      for(CTRL_ThreadRegCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(n->machine_id == machine_id && dmn_handle_match(n->thread, thread))
        {
          node = n;
          break;
        }
      }
    }
    
    // rjf: copy from node
    if(node)
    {
      U64 current_reg_gen = dmn_reg_gen();
      B32 need_stale = 1;
      if(node->reg_gen != current_reg_gen && dmn_thread_read_reg_block(thread, result))
      {
        need_stale = 0;
        node->reg_gen = current_reg_gen;
        MemoryCopy(node->block, result, reg_block_size);
      }
      if(need_stale)
      {
        MemoryCopy(result, node->block, reg_block_size);
      }
    }
  }
  return result;
}

internal U64
ctrl_query_cached_tls_root_vaddr_from_thread(CTRL_EntityStore *store, CTRL_MachineID machine_id, DMN_Handle thread)
{
  U64 result = dmn_tls_root_vaddr_from_thread(thread);
  return result;
}

internal U64
ctrl_query_cached_rip_from_thread(CTRL_EntityStore *store, CTRL_MachineID machine_id, DMN_Handle thread)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *thread_entity = ctrl_entity_from_machine_id_handle(store, machine_id, thread);
  Architecture arch = thread_entity->arch;
  void *block = ctrl_query_cached_reg_block_from_thread(scratch.arena, store, machine_id, thread);
  U64 result = regs_rip_from_arch_block(arch, block);
  scratch_end(scratch);
  return result;
}

//- rjf: thread register writing

internal B32
ctrl_thread_write_reg_block(CTRL_MachineID machine_id, DMN_Handle thread, void *block)
{
  B32 good = dmn_thread_write_reg_block(thread, block);
  return good;
}

////////////////////////////////
//~ rjf: Unwinding Functions

internal CTRL_Unwind
ctrl_unwind_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_MachineID machine_id, DMN_Handle thread, U64 endt_us)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  DBGI_Scope *scope = dbgi_scope_open();
  CTRL_Unwind unwind = {0};
  unwind.error = 1;
  
  //- rjf: unpack args
  CTRL_Entity *thread_entity = ctrl_entity_from_machine_id_handle(store, machine_id, thread);
  CTRL_Entity *process_entity = thread_entity->parent;
  Architecture arch = thread_entity->arch;
  U64 arch_reg_block_size = regs_block_size_from_architecture(arch);
  
  //- rjf: grab initial register block
  void *regs_block = push_array(scratch.arena, U8, arch_reg_block_size);
  B32 regs_block_good = dmn_thread_read_reg_block(thread, regs_block);
  
  //- rjf: grab initial memory view
  B32 stack_memview_good = 0;
  UNW_MemView stack_memview = {0};
  {
    U64 stack_base_unrounded = dmn_stack_base_vaddr_from_thread(thread);
    U64 stack_top_unrounded = regs_rsp_from_arch_block(arch, regs_block);
    U64 stack_base = AlignPow2(stack_base_unrounded, KB(4));
    U64 stack_top = AlignDownPow2(stack_top_unrounded, KB(4));
    U64 stack_size = stack_base - stack_top;
    if(stack_base >= stack_top)
    {
      U8 *stack_memory_base = push_array(scratch.arena, U8, stack_size);
      U64 actual_stack_bytes_read = dmn_process_read(process_entity->handle, r1u64(stack_top, stack_top+stack_size), stack_memory_base);
      String8 stack_memory = str8(stack_memory_base, actual_stack_bytes_read);
      if(stack_memory.size >= stack_size)
      {
        stack_memview_good = 1;
        stack_memview.data = stack_memory.str;
        stack_memview.addr_first = stack_top;
        stack_memview.addr_opl = stack_base;
      }
    }
  }
  
  //- rjf: loop & unwind
  UNW_MemView memview = stack_memview;
  if(regs_block_good && stack_memview_good)
  {
    unwind.error = 0;
    for(;;)
    {
      // rjf: regs -> rip*module
      U64 rip = regs_rip_from_arch_block(arch, regs_block);
      DMN_Handle module = {0};
      String8 module_name = {0};
      Rng1U64 module_vaddr_range = {0};
      for(CTRL_Entity *m = process_entity->first; m != &ctrl_entity_nil; m = m->next)
      {
        if(m->kind == CTRL_EntityKind_Module && contains_1u64(m->vaddr_range, rip))
        {
          module = m->handle;
          module_name = m->string;
          module_vaddr_range = m->vaddr_range;
          break;
        }
      }
      
      // rjf: cancel on 0 rip
      if(rip == 0)
      {
        break;
      }
      
      // rjf: module -> all the binary info
      String8 binary_full_path = module_name;
      DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_full_path, 0);
      String8 binary_data = str8((U8 *)dbgi->exe_base, dbgi->exe_props.size);
      
      // rjf: cancel on bad data
      if(binary_data.size == 0)
      {
        unwind.error = 1;
        break;
      }
      
      // rjf: valid step -> push frame
      CTRL_UnwindFrame *frame = push_array(arena, CTRL_UnwindFrame, 1);
      frame->rip = rip;
      frame->regs = push_array_no_zero(arena, U8, arch_reg_block_size);
      MemoryCopy(frame->regs, regs_block, arch_reg_block_size);
      DLLPushBack(unwind.first, unwind.last, frame);
      unwind.count += 1;
      
      // rjf: unwind one step
      UNW_Result unwind_step = {0};
      switch(arch)
      {
        default:{unwind_step.dead = 1;}break;
        case Architecture_x64:
        {
          unwind_step = unw_pe_x64(binary_data, &dbgi->pe, module_vaddr_range.min, &memview, (UNW_X64_Regs *)regs_block);
        }break;
      }
      
      // rjf: cancel on bad step
      if(unwind_step.dead != 0)
      {
        break;
      }
      if(unwind_step.missed_read != 0)
      {
        unwind.error = 1;
        break;
      }
      if(unwind_step.stack_pointer == 0)
      {
        break;
      }
    }
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return unwind;
}

////////////////////////////////
//~ rjf: Halting All Attached Processes

internal void
ctrl_halt(void)
{
  dmn_halt(0, 0);
}

////////////////////////////////
//~ rjf: Shared Accessor Functions

//- rjf: run generation counter

internal U64
ctrl_run_gen(void)
{
  U64 result = dmn_run_gen();
  return result;
}

internal U64
ctrl_mem_gen(void)
{
  U64 result = dmn_mem_gen();
  return result;
}

internal U64
ctrl_reg_gen(void)
{
  U64 result = dmn_reg_gen();
  return result;
}

//- rjf: name -> register/alias hash tables, for eval

internal EVAL_String2NumMap *
ctrl_string2reg_from_arch(Architecture arch)
{
  return &ctrl_state->arch_string2reg_tables[arch];
}

internal EVAL_String2NumMap *
ctrl_string2alias_from_arch(Architecture arch)
{
  return &ctrl_state->arch_string2alias_tables[arch];
}

////////////////////////////////
//~ rjf: Control-Thread Functions

//- rjf: user -> control thread communication

internal B32
ctrl_u2c_push_msgs(CTRL_MsgList *msgs, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  String8 msgs_srlzed_baked = ctrl_serialized_string_from_msg_list(scratch.arena, msgs);
  B32 good = 0;
  OS_MutexScope(ctrl_state->u2c_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (ctrl_state->u2c_ring_write_pos-ctrl_state->u2c_ring_read_pos);
    U64 available_size = ctrl_state->u2c_ring_size-unconsumed_size;
    if(available_size >= sizeof(U64) + msgs_srlzed_baked.size)
    {
      ctrl_state->u2c_ring_write_pos += ring_write_struct(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_write_pos, &msgs_srlzed_baked.size);
      ctrl_state->u2c_ring_write_pos += ring_write(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_write_pos, msgs_srlzed_baked.str, msgs_srlzed_baked.size);
      ctrl_state->u2c_ring_write_pos += 7;
      ctrl_state->u2c_ring_write_pos -= ctrl_state->u2c_ring_write_pos%8;
      good = 1;
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(ctrl_state->u2c_ring_cv, ctrl_state->u2c_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(ctrl_state->u2c_ring_cv);
  }
  scratch_end(scratch);
  return good;
}

internal CTRL_MsgList
ctrl_u2c_pop_msgs(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 msgs_srlzed_baked = {0};
  OS_MutexScope(ctrl_state->u2c_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (ctrl_state->u2c_ring_write_pos-ctrl_state->u2c_ring_read_pos);
    if(unconsumed_size >= sizeof(U64))
    {
      U64 size_to_decode = 0;
      ctrl_state->u2c_ring_read_pos += ring_read_struct(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_read_pos, &size_to_decode);
      msgs_srlzed_baked.size = size_to_decode;
      msgs_srlzed_baked.str = push_array_no_zero(scratch.arena, U8, msgs_srlzed_baked.size);
      ctrl_state->u2c_ring_read_pos += ring_read(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_read_pos, msgs_srlzed_baked.str, size_to_decode);
      ctrl_state->u2c_ring_read_pos += 7;
      ctrl_state->u2c_ring_read_pos -= ctrl_state->u2c_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(ctrl_state->u2c_ring_cv, ctrl_state->u2c_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ctrl_state->u2c_ring_cv);
  CTRL_MsgList msgs = ctrl_msg_list_from_serialized_string(arena, msgs_srlzed_baked);
  scratch_end(scratch);
  return msgs;
}

//- rjf: control -> user thread communication

internal void
ctrl_c2u_push_events(CTRL_EventList *events)
{
  if(events->count != 0) ProfScope("ctrl_c2u_push_events")
  {
    ctrl_entity_store_apply_events(ctrl_state->ctrl_thread_entity_store, events);
    for(CTRL_EventNode *n = events->first; n != 0; n = n ->next)
    {
      Temp scratch = scratch_begin(0, 0);
      String8 event_srlzed = ctrl_serialized_string_from_event(scratch.arena, &n->v);
      OS_MutexScope(ctrl_state->c2u_ring_mutex) for(;;)
      {
        U64 unconsumed_size = (ctrl_state->c2u_ring_write_pos-ctrl_state->c2u_ring_read_pos);
        U64 available_size = ctrl_state->c2u_ring_size-unconsumed_size;
        if(available_size >= sizeof(U64) + event_srlzed.size)
        {
          ctrl_state->c2u_ring_write_pos += ring_write_struct(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_write_pos, &event_srlzed.size);
          ctrl_state->c2u_ring_write_pos += ring_write(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_write_pos, event_srlzed.str, event_srlzed.size);
          ctrl_state->c2u_ring_write_pos += 7;
          ctrl_state->c2u_ring_write_pos -= ctrl_state->c2u_ring_write_pos%8;
          break;
        }
        os_condition_variable_wait(ctrl_state->c2u_ring_cv, ctrl_state->c2u_ring_mutex, os_now_microseconds()+100);
      }
      os_condition_variable_broadcast(ctrl_state->c2u_ring_cv);
      if(ctrl_state->wakeup_hook != 0)
      {
        ctrl_state->wakeup_hook();
      }
      scratch_end(scratch);
    }
  }
}

internal CTRL_EventList
ctrl_c2u_pop_events(Arena *arena)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_EventList events = {0};
  OS_MutexScope(ctrl_state->c2u_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (ctrl_state->c2u_ring_write_pos-ctrl_state->c2u_ring_read_pos);
    if(unconsumed_size >= sizeof(U64))
    {
      U64 size_to_decode = 0;
      ctrl_state->c2u_ring_read_pos += ring_read_struct(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_read_pos, &size_to_decode);
      String8 event_srlzed = {0};
      event_srlzed.size = size_to_decode;
      event_srlzed.str = push_array_no_zero(scratch.arena, U8, event_srlzed.size);
      ctrl_state->c2u_ring_read_pos += ring_read(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_read_pos, event_srlzed.str, event_srlzed.size);
      ctrl_state->c2u_ring_read_pos += 7;
      ctrl_state->c2u_ring_read_pos -= ctrl_state->c2u_ring_read_pos%8;
      CTRL_Event *new_event = ctrl_event_list_push(arena, &events);
      *new_event = ctrl_event_from_serialized_string(arena, event_srlzed);
    }
    else
    {
      break;
    }
  }
  os_condition_variable_broadcast(ctrl_state->c2u_ring_cv);
  scratch_end(scratch);
  ProfEnd();
  return events;
}

//- rjf: entry point

internal void
ctrl_thread__entry_point(void *p)
{
  ThreadNameF("[ctrl] thread");
  ProfBeginFunction();
  DMN_CtrlCtx *ctrl_ctx = dmn_ctrl_begin();
  
  //- rjf: loop
  Temp scratch = scratch_begin(0, 0);
  for(;;)
  {
    temp_end(scratch);
    
    //- rjf: get next messages
    CTRL_MsgList msgs = ctrl_u2c_pop_msgs(scratch.arena);
    
    //- rjf: enable stuck-thread-step behavior in all cases - can be silently enabled by launch_and_init for subsequent messages
    {
      ctrl_state->disable_stuck_thread_step = 0;
    }
    
    //- rjf: process messages
    DMN_CtrlExclusiveAccessScope
    {
      B32 done = 0;
      for(CTRL_MsgNode *msg_n = msgs.first; msg_n != 0 && done == 0; msg_n = msg_n->next)
      {
        CTRL_Msg *msg = &msg_n->v;
        MemoryCopyArray(ctrl_state->exception_code_filters, msg->exception_code_filters);
        switch(msg->kind)
        {
          case CTRL_MsgKind_Null:
          case CTRL_MsgKind_COUNT:{}break;
          
          //- rjf: target operations
          case CTRL_MsgKind_LaunchAndHandshake:{ctrl_thread__launch_and_handshake(ctrl_ctx, msg);}break;
          case CTRL_MsgKind_LaunchAndInit:     {ctrl_thread__launch_and_init     (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Attach:            {ctrl_thread__attach              (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Kill:              {ctrl_thread__kill                (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Detach:            {ctrl_thread__detach              (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Run:               {ctrl_thread__run                 (ctrl_ctx, msg); done = 1;}break;
          case CTRL_MsgKind_SingleStep:        {ctrl_thread__single_step         (ctrl_ctx, msg); done = 1;}break;
          
          //- rjf: configuration
          case CTRL_MsgKind_SetUserEntryPoints:
          {
            arena_clear(ctrl_state->user_entry_point_arena);
            MemoryZeroStruct(&ctrl_state->user_entry_points);
            for(String8Node *n = msg->strings.first; n != 0; n = n->next)
            {
              str8_list_push(ctrl_state->user_entry_point_arena, &ctrl_state->user_entry_points, n->string);
            }
          }break;
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

//- rjf: breakpoint resolution

internal void
ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, DMN_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  DBGI_Scope *scope = dbgi_scope_open();
  CTRL_Entity *module_entity = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, machine_id, module);
  String8 exe_path = module_entity->string;
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
  RDI_Parsed *rdi = &dbgi->rdi;
  U64 base_vaddr = module_entity->vaddr_range.min;
  for(CTRL_UserBreakpointNode *n = user_bps->first; n != 0; n = n->next)
  {
    CTRL_UserBreakpoint *bp = &n->v;
    switch(bp->kind)
    {
      default:{}break;
      
      //- rjf: file:line-based breakpoints
      case CTRL_UserBreakpointKind_FileNameAndLineColNumber:
      {
        // rjf: unpack & normalize
        TxtPt pt = bp->pt;
        String8 filename = bp->string;
        String8 filename_normalized = push_str8_copy(scratch.arena, filename);
        for(U64 idx = 0; idx < filename_normalized.size; idx += 1)
        {
          filename_normalized.str[idx] = char_to_lower(filename_normalized.str[idx]);
          filename_normalized.str[idx] = char_to_correct_slash(filename_normalized.str[idx]);
        }
        
        // rjf: filename -> src_id
        U32 src_id = 0;
        {
          RDI_NameMap *mapptr = rdi_name_map_from_kind(rdi, RDI_NameMapKind_NormalSourcePaths);
          if(mapptr != 0)
          {
            RDI_ParsedNameMap map = {0};
            rdi_name_map_parse(rdi, mapptr, &map);
            RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, filename_normalized.str, filename_normalized.size);
            if(node != 0)
            {
              U32 id_count = 0;
              U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
              if(id_count > 0)
              {
                src_id = ids[0];
              }
            }
          }
        }
        
        // rjf: src_id * pt -> push
        {
          RDI_SourceFile *src = rdi_element_from_idx(rdi, source_files, src_id);
          RDI_ParsedLineMap line_map = {0};
          rdi_line_map_from_source_file(rdi, src, &line_map);
          U32 voff_count = 0;
          U64 *voffs = rdi_line_voffs_from_num(&line_map, pt.line, &voff_count);
          for(U32 i = 0; i < voff_count; i += 1)
          {
            U64 vaddr = voffs[i] + base_vaddr;
            DMN_Trap trap = {process, vaddr, (U64)bp};
            dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
          }
        }
      }break;
      
      //- rjf: symbol:voff-based breakpoints
      case CTRL_UserBreakpointKind_SymbolNameAndOffset:
      {
        String8 symbol_name = bp->string;
        U64 voff = bp->u64;
        if(rdi != 0 && rdi->procedures != 0)
        {
          RDI_NameMap *mapptr = rdi_name_map_from_kind(rdi, RDI_NameMapKind_Procedures);
          if(mapptr != 0)
          {
            RDI_ParsedNameMap map = {0};
            rdi_name_map_parse(rdi, mapptr, &map);
            RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, symbol_name.str, symbol_name.size);
            if(node != 0)
            {
              U32 id_count = 0;
              U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
              for(U32 match_i = 0; match_i < id_count; match_i += 1)
              {
                U64 proc_voff = rdi_first_voff_from_proc(rdi, ids[match_i]);
                U64 proc_vaddr = proc_voff + base_vaddr;
                DMN_Trap trap = {process, proc_vaddr + voff, (U64)bp};
                dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
              }
            }
          }
        }
      }break;
    }
  }
  dbgi_scope_close(scope);
  scratch_end(scratch);
}

internal void
ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  for(CTRL_UserBreakpointNode *n = user_bps->first; n != 0; n = n->next)
  {
    CTRL_UserBreakpoint *bp = &n->v;
    if(bp->kind == CTRL_UserBreakpointKind_VirtualAddress)
    {
      DMN_Trap trap = {process, bp->u64, (U64)bp};
      dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
    }
  }
}

//- rjf: attached process running/event gathering

internal DMN_Event *
ctrl_thread__next_dmn_event(Arena *arena, DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg, DMN_RunCtrls *run_ctrls, CTRL_Spoof *spoof)
{
  ProfBeginFunction();
  DMN_Event *event = push_array(arena, DMN_Event, 1);
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: loop -> try to get event, run, repeat
  U64 spoof_old_ip_value = 0;
  ProfScope("loop -> try to get event, run, repeat") for(B32 got_event = 0; got_event == 0;)
  {
    //- rjf: get next event
    ProfScope("get next event")
    {
      // rjf: grab first event
      DMN_EventNode *next_event_node = ctrl_state->first_dmn_event_node;
      
      // rjf: determine if we should filter
      B32 should_filter_event = 0;
      if(next_event_node != 0)
      {
        DMN_Event *ev = &next_event_node->v;
        switch(ev->kind)
        {
          default:{}break;
          case DMN_EventKind_Exception:
          {
            // NOTE(rjf): first chance exceptions -> try ignoring
            should_filter_event = (ev->exception_repeated == 0 && (spoof == 0 || ev->instruction_pointer != spoof->new_ip_value));
            
            // rjf: exception code -> kind
            CTRL_ExceptionCodeKind code_kind = CTRL_ExceptionCodeKind_Null;
            if(should_filter_event)
            {
              for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
              {
                if(ctrl_exception_code_kind_code_table[k] == ev->code)
                {
                  code_kind = k;
                  break;
                }
              }
            }
            
            // rjf: exception code kind -> shouldn't stop? if so, do not filter
            if(should_filter_event)
            {
              B32 shouldnt_filter = !!(ctrl_state->exception_code_filters[code_kind/64] & (1ull<<(code_kind%64)));
              if(should_filter_event && shouldnt_filter)
              {
                should_filter_event = 0;
              }
            }
            
            // rjf: special case: be gracious with ASan modules or symbols if
            // they do their cute little 0xc0000005 exception trick...
            if(!should_filter_event && ev->code == 0xc0000005 &&
               (spoof == 0 || ev->instruction_pointer != spoof->new_ip_value))
            {
              DBGI_Scope *scope = dbgi_scope_open();
              CTRL_Entity *process = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, ev->process);
              CTRL_Entity *module = &ctrl_entity_nil;
              for(CTRL_Entity *child = process->first; child != &ctrl_entity_nil; child = child->next)
              {
                if(child->kind == CTRL_EntityKind_Module)
                {
                  module = child;
                  break;
                }
              }
              if(module != &ctrl_entity_nil)
              {
                // rjf: determine base address of asan shadow space
                U64 asan_shadow_base_vaddr = 0;
                B32 asan_shadow_variable_exists_but_is_zero = 0;
                String8 module_path = module->string;
                DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, module_path, max_U64);
                RDI_Parsed *rdi = &dbgi->rdi;
                RDI_NameMap *unparsed_map = rdi_name_map_from_kind(rdi, RDI_NameMapKind_GlobalVariables);
                if(rdi->global_variables != 0 && unparsed_map != 0)
                {
                  RDI_ParsedNameMap map = {0};
                  rdi_name_map_parse(rdi, unparsed_map, &map);
                  String8 name = str8_lit("__asan_shadow_memory_dynamic_address");
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  if(node != 0)
                  {
                    U32 id_count = 0;
                    U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                    if(id_count > 0)
                    {
                      RDI_GlobalVariable *global_var = rdi_element_from_idx(rdi, global_variables, ids[0]);
                      U64 global_var_voff = global_var->voff;
                      U64 global_var_vaddr = global_var->voff + module->vaddr_range.min;
                      Architecture arch = process->arch;
                      U64 addr_size = bit_size_from_arch(arch)/8;
                      dmn_process_read(ev->process, r1u64(global_var_vaddr, global_var_vaddr+addr_size), &asan_shadow_base_vaddr);
                      asan_shadow_variable_exists_but_is_zero = (asan_shadow_base_vaddr == 0);
                    }
                  }
                }
                
                // rjf: determine if this was a read/write to the shadow space
                B32 violation_in_shadow_space = 0;
                if(asan_shadow_base_vaddr != 0)
                {
                  U64 asan_shadow_space_size = TB(128)/8;
                  if(asan_shadow_base_vaddr <= ev->address && ev->address < asan_shadow_base_vaddr+asan_shadow_space_size)
                  {
                    violation_in_shadow_space = 1;
                  }
                }
                
                // rjf: filter event if this violation occurred in asan's shadow space
                if(violation_in_shadow_space || asan_shadow_variable_exists_but_is_zero)
                {
                  should_filter_event = 1;
                }
              }
              
              dbgi_scope_close(scope);
            }
          }break;
        }
      }
      
      // rjf: good event & unfiltered? -> pop from queue & grab as result
      if(next_event_node != 0 && !should_filter_event)
      {
        got_event = 1;
        SLLQueuePop(ctrl_state->first_dmn_event_node, ctrl_state->last_dmn_event_node);
        MemoryCopyStruct(event, &next_event_node->v);
        event->string = push_str8_copy(arena, event->string);
        run_ctrls->ignore_previous_exception = 1;
      }
      
      // rjf: good event but filtered? pop from queue
      if(next_event_node != 0 && should_filter_event)
      {
        SLLQueuePop(ctrl_state->first_dmn_event_node, ctrl_state->last_dmn_event_node);
        run_ctrls->ignore_previous_exception = 0;
      }
    }
    
    //- rjf: no event -> dmn_ctrl_run for a new one
    if(got_event == 0) ProfScope("no event -> dmn_ctrl_run for a new one")
    {
      // rjf: prep spoof
      B32 do_spoof = (spoof != 0 && dmn_handle_match(run_ctrls->single_step_thread, dmn_handle_zero()));
      U64 size_of_spoof = 0;
      if(do_spoof) ProfScope("prep spoof")
      {
        CTRL_Entity *spoof_process = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, spoof->process);
        Architecture arch = spoof_process->arch;
        size_of_spoof = bit_size_from_arch(arch)/8;
        dmn_process_read(spoof_process->handle, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
      }
      
      // rjf: set spoof
      if(do_spoof) ProfScope("set spoof")
      {
        dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof->new_ip_value);
      }
      
      // rjf: run for new events
      ProfScope("run for new events")
      {
        DMN_EventList events = dmn_ctrl_run(scratch.arena, ctrl_ctx, run_ctrls);
        for(DMN_EventNode *src_n = events.first; src_n != 0; src_n = src_n->next)
        {
          DMN_EventNode *dst_n = ctrl_state->free_dmn_event_node;
          if(dst_n != 0)
          {
            SLLStackPop(ctrl_state->free_dmn_event_node);
          }
          else
          {
            dst_n = push_array(ctrl_state->dmn_event_arena, DMN_EventNode, 1);
          }
          MemoryCopyStruct(&dst_n->v, &src_n->v);
          dst_n->v.string = push_str8_copy(ctrl_state->dmn_event_arena, dst_n->v.string);
          SLLQueuePush(ctrl_state->first_dmn_event_node, ctrl_state->last_dmn_event_node, dst_n);
        }
      }
      
      // rjf: unset spoof
      if(do_spoof) ProfScope("unset spoof")
      {
        dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
      }
    }
  }
  
  //- rjf: irrespective of what event came back, we should ALWAYS check the
  // spoof's thread and see if it hit the spoof address, because we may have
  // simply been sent other debug events first
  if(spoof != 0)
  {
    CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, spoof->thread);
    Architecture arch = thread->arch;
    void *regs_block = push_array(scratch.arena, U8, regs_block_size_from_architecture(arch));
    dmn_thread_read_reg_block(spoof->thread, regs_block);
    U64 spoof_thread_rip = regs_rip_from_arch_block(arch, regs_block);
    if(spoof_thread_rip == spoof->new_ip_value)
    {
      regs_arch_block_write_rip(arch, regs_block, spoof_old_ip_value);
      dmn_thread_write_reg_block(spoof->thread, regs_block);
    }
  }
  
  //- rjf: push ctrl events associated with this demon event
  CTRL_EventList evts = {0};
  ProfScope("push ctrl events associated with this demon event") switch(event->kind)
  {
    default:{}break;
    case DMN_EventKind_CreateProcess:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_NewProc;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->process;
      out_evt->arch       = event->arch;
      out_evt->entity_id  = event->code;
      ctrl_state->process_counter += 1;
    }break;
    case DMN_EventKind_CreateThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_NewThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->thread;
      out_evt->parent     = event->process;
      out_evt->arch       = event->arch;
      out_evt->entity_id  = event->code;
      out_evt->stack_base = dmn_stack_base_vaddr_from_thread(event->thread);
      out_evt->tls_root   = dmn_tls_root_vaddr_from_thread(event->thread);
      out_evt->rip_vaddr  = event->instruction_pointer;
      out_evt->string     = event->string;
    }break;
    case DMN_EventKind_LoadModule:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      String8 module_path = event->string;
      dbgi_binary_open(module_path);
      out_evt->kind       = CTRL_EventKind_NewModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->module;
      out_evt->parent     = event->process;
      out_evt->arch       = event->arch;
      out_evt->entity_id  = event->code;
      out_evt->vaddr_rng  = r1u64(event->address, event->address+event->size);
      out_evt->rip_vaddr  = event->address;
      out_evt->string     = module_path;
    }break;
    case DMN_EventKind_ExitProcess:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndProc;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->process;
      out_evt->u64_code   = event->code;
      ctrl_state->process_counter -= 1;
    }break;
    case DMN_EventKind_ExitThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->thread;
      out_evt->entity_id  = event->code;
    }break;
    case DMN_EventKind_UnloadModule:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      String8 module_path = event->string;
      dbgi_binary_close(module_path);
      out_evt->kind       = CTRL_EventKind_EndModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->module;
    }break;
    case DMN_EventKind_DebugString:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_DebugString;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->thread;
      out_evt->parent     = event->process;
      out_evt->string     = event->string;
    }break;
    case DMN_EventKind_SetThreadName:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_ThreadName;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Local;
      out_evt->entity     = event->thread;
      out_evt->parent     = event->process;
      out_evt->string     = event->string;
      out_evt->entity_id  = event->code;
    }break;
  }
  ctrl_c2u_push_events(&evts);
  
  //- rjf: clear process memory cache, if we've just started a lone process
  if(event->kind == DMN_EventKind_CreateProcess && ctrl_state->process_counter == 1)
  {
    CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
    for(U64 slot_idx = 0; slot_idx < cache->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%cache->stripes_count;
      CTRL_ProcessMemoryCacheSlot *slot = &cache->slots[slot_idx];
      CTRL_ProcessMemoryCacheStripe *stripe = &cache->stripes[stripe_idx];
      OS_MutexScopeW(stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          arena_clear(n->arena);
        }
      }
      MemoryZeroStruct(slot);
    }
  }
  
  //- rjf: out of queued up demon events -> clear event arena
  if(ctrl_state->first_dmn_event_node == 0)
  {
    ctrl_state->free_dmn_event_node = 0;
    arena_clear(ctrl_state->dmn_event_arena);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return(event);
}

//- rjf: eval helpers

internal B32
ctrl_eval_memory_read(void *u, void *out, U64 addr, U64 size)
{
  DMN_Handle process = *(DMN_Handle *)u;
  U64 read_size = dmn_process_read(process, r1u64(addr, addr+size), out);
  B32 result = (read_size == size);
  return result;
}

//- rjf: msg kind implementations

internal void
ctrl_thread__launch_and_handshake(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  
  //- rjf: launch
  OS_LaunchOptions opts = {0};
  {
    opts.cmd_line    = msg->cmd_line_string_list;
    opts.path        = msg->path;
    opts.env         = msg->env_string_list;
    opts.inherit_env = msg->env_inherit;
  }
  U32 id = dmn_ctrl_launch(ctrl_ctx, &opts);
  
  //- rjf: record start
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: run to handshake
  DMN_Event *stop_event = 0;
  if(id != 0)
  {
    // rjf: prep run controls
    DMN_Handle unfrozen_process = {0};
    DMN_RunCtrls run_ctrls = {0};
    {
      run_ctrls.run_entities_are_unfrozen = 1;
      run_ctrls.run_entities_are_processes = 1;
      run_ctrls.run_entities = &unfrozen_process;
      run_ctrls.run_entity_count = 0;
    }
    
    // rjf: run until handshake-signifying events
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_CreateProcess:
        {
          unfrozen_process = event->process;
          run_ctrls.run_entity_count = 1;
        }break;
        case DMN_EventKind_Error:
        case DMN_EventKind_Breakpoint:
        case DMN_EventKind_Exception:
        case DMN_EventKind_ExitProcess:
        case DMN_EventKind_HandshakeComplete:
        {
          done = 1;
          stop_event = event;
        }break;
      }
    }
  }
  
  //- rjf: record bad stop
  if(stop_event == 0 && id == 0)
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Error;
    event->cause = CTRL_EventCause_Error;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Stopped;
    if(stop_event != 0)
    {
      event->cause = ctrl_event_cause_from_dmn_event_kind(stop_event->kind);
      event->machine_id = CTRL_MachineID_Local;
      event->entity = stop_event->thread;
      event->parent = stop_event->process;
      event->exception_code = stop_event->code;
      event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
      event->rip_vaddr = stop_event->instruction_pointer;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: push request resolution event
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_LaunchAndHandshakeDone;
    evt->machine_id= CTRL_MachineID_Local;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = id;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__launch_and_init(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: launch
  //
  OS_LaunchOptions opts = {0};
  {
    opts.cmd_line    = msg->cmd_line_string_list;
    opts.path        = msg->path;
    opts.env         = msg->env_string_list;
    opts.inherit_env = msg->env_inherit;
  }
  U32 id = dmn_ctrl_launch(ctrl_ctx, &opts);
  
  //////////////////////////////
  //- rjf: record start
  //
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //////////////////////////////
  //- rjf: run to initialization (entry point)
  //
  DMN_Event *stop_event = 0;
  if(id != 0)
  {
    DMN_Handle unfrozen_process[8] = {0};
    DMN_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        
        //- rjf: new process -> freeze process
        case DMN_EventKind_CreateProcess:
        if(run_ctrls.run_entity_count < ArrayCount(unfrozen_process))
        {
          unfrozen_process[run_ctrls.run_entity_count] = event->process;
          run_ctrls.run_entities = &unfrozen_process[0];
          run_ctrls.run_entity_count += 1;
        }break;
        
        //- rjf: breakpoint -> if it's the entry point, we're done. otherwise, keep going
        case DMN_EventKind_Breakpoint:
        {
          for(DMN_TrapChunkNode *n = run_ctrls.traps.first; n != 0; n = n->next)
          {
            for(U64 idx = 0; idx < n->count; idx += 1)
            {
              if(n->v[idx].vaddr == event->instruction_pointer)
              {
                done = 1;
                stop_event = event;
                goto end_look_for_entry_match;
              }
            }
          }
          end_look_for_entry_match:;
        }break;
        
        //- rjf: exception -> done
        case DMN_EventKind_Exception:
        {
          done = 1;
          stop_event = event;
        }break;
        
        //- rjf: process ended? -> remove from unfrozen processes. zero processes -> done.
        case DMN_EventKind_ExitProcess:
        {
          for(U64 idx = 0; idx < run_ctrls.run_entity_count; idx += 1)
          {
            if(dmn_handle_match(run_ctrls.run_entities[idx], event->process) &&
               idx+1 < run_ctrls.run_entity_count)
            {
              MemoryCopy(run_ctrls.run_entities+idx, run_ctrls.run_entities+idx+1, sizeof(DMN_Handle)*(run_ctrls.run_entity_count-(idx+1)));
              break;
            }
          }
          if(run_ctrls.run_entity_count > 0)
          {
            run_ctrls.run_entity_count -= 1;
          }
          if(run_ctrls.run_entity_count == 0)
          {
            done = 1;
            stop_event = event;
          }
        }break;
        
        //- rjf: done with handshake -> ready to find entry point. search launched processes
        case DMN_EventKind_HandshakeComplete:
        {
          DBGI_Scope *scope = dbgi_scope_open();
          
          //- rjf: add traps for all possible entry points
          for(U64 process_idx = 0; process_idx < run_ctrls.run_entity_count; process_idx += 1)
          {
            //- rjf: unpack process & first module info
            CTRL_Entity *process = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, run_ctrls.run_entities[process_idx]);
            CTRL_Entity *module = &ctrl_entity_nil;
            for(CTRL_Entity *child = process->first; child != &ctrl_entity_nil; child = child->next)
            {
              if(child->kind == CTRL_EntityKind_Module)
              {
                module = child;
                break;
              }
            }
            U64 module_base_vaddr = module->vaddr_range.min;
            String8 exe_path = module->string;
            DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
            RDI_Parsed *rdi = &dbgi->rdi;
            RDI_NameMap *unparsed_map = rdi_name_map_from_kind(rdi, RDI_NameMapKind_Procedures);
            RDI_ParsedNameMap map = {0};
            rdi_name_map_parse(rdi, unparsed_map, &map);
            
            //- rjf: add trap for user-specified entry point, if specified
            B32 entries_found = 0;
            if(!entries_found)
            {
              for(String8Node *n = msg->strings.first; n != 0; n = n->next)
              {
                U32 procedure_id = 0;
                {
                  String8 name = n->string;
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  U32 id_count = 0;
                  U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                  if(id_count > 0)
                  {
                    procedure_id = ids[0];
                  }
                }
                U64 voff = rdi_first_voff_from_proc(rdi, procedure_id);
                if(voff != 0)
                {
                  entries_found = 1;
                  DMN_Trap trap = {run_ctrls.run_entities[process_idx], module_base_vaddr + voff};
                  dmn_trap_chunk_list_push(scratch.arena, &run_ctrls.traps, 256, &trap);
                }
              }
            }
            
            //- rjf: add traps for all custom user entry points
            if(!entries_found)
            {
              for(String8Node *n = ctrl_state->user_entry_points.first; n != 0; n = n->next)
              {
                U32 procedure_id = 0;
                {
                  String8 name = n->string;
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  U32 id_count = 0;
                  U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                  if(id_count > 0)
                  {
                    procedure_id = ids[0];
                  }
                }
                U64 voff = rdi_first_voff_from_proc(rdi, procedure_id);
                if(voff != 0)
                {
                  DMN_Trap trap = {run_ctrls.run_entities[process_idx], module_base_vaddr + voff};
                  dmn_trap_chunk_list_push(scratch.arena, &run_ctrls.traps, 256, &trap);
                  break;
                }
              }
            }
            
            //- rjf: add traps for all high-level entry points
            if(!entries_found)
            {
              String8 hi_entry_points[] =
              {
                str8_lit("WinMain"),
                str8_lit("wWinMain"),
                str8_lit("main"),
                str8_lit("wmain"),
              };
              for(U64 idx = 0; idx < ArrayCount(hi_entry_points); idx += 1)
              {
                U32 procedure_id = 0;
                {
                  String8 name = hi_entry_points[idx];
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  U32 id_count = 0;
                  U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                  if(id_count > 0)
                  {
                    procedure_id = ids[0];
                  }
                }
                U64 voff = rdi_first_voff_from_proc(rdi, procedure_id);
                if(voff != 0)
                {
                  entries_found = 1;
                  DMN_Trap trap = {run_ctrls.run_entities[process_idx], module_base_vaddr + voff};
                  dmn_trap_chunk_list_push(scratch.arena, &run_ctrls.traps, 256, &trap);
                }
              }
            }
            
            //- rjf: add trap for PE header entry
            if(!entries_found)
            {
              U64 voff = dbgi->pe.entry_point;
              if(voff != 0)
              {
                DMN_Trap trap = {run_ctrls.run_entities[process_idx], module_base_vaddr + voff};
                dmn_trap_chunk_list_push(scratch.arena, &run_ctrls.traps, 256, &trap);
              }
            }
            
            //- rjf: add traps for all low-level entry points
            if(!entries_found)
            {
              String8 lo_entry_points[] =
              {
                str8_lit("WinMainCRTStartup"),
                str8_lit("wWinMainCRTStartup"),
                str8_lit("mainCRTStartup"),
                str8_lit("wmainCRTStartup"),
              };
              for(U64 idx = 0; idx < ArrayCount(lo_entry_points); idx += 1)
              {
                U32 procedure_id = 0;
                {
                  String8 name = lo_entry_points[idx];
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  U32 id_count = 0;
                  U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                  if(id_count > 0)
                  {
                    procedure_id = ids[0];
                  }
                }
                U64 voff = rdi_first_voff_from_proc(rdi, procedure_id);
                if(voff != 0)
                {
                  entries_found = 1;
                  DMN_Trap trap = {run_ctrls.run_entities[process_idx], module_base_vaddr + voff};
                  dmn_trap_chunk_list_push(scratch.arena, &run_ctrls.traps, 256, &trap);
                }
              }
            }
            
            //- rjf: no entry point found -> done
            if(run_ctrls.traps.trap_count == 0)
            {
              done = 1;
              stop_event = event;
            }
          }
          
          dbgi_scope_close(scope);
        }break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: record bad stop
  //
  if(stop_event == 0 && id == 0)
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Error;
    event->cause = CTRL_EventCause_Error;
    ctrl_c2u_push_events(&evts);
  }
  
  //////////////////////////////
  //- rjf: record stop
  //
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind    = CTRL_EventKind_Stopped;
    event->msg_id  = msg->msg_id;
    if(stop_event != 0)
    {
      event->cause          = ctrl_event_cause_from_dmn_event_kind(stop_event->kind);
      event->machine_id     = CTRL_MachineID_Local;
      event->entity         = stop_event->thread;
      event->parent         = stop_event->process;
      event->exception_code = stop_event->code;
      event->vaddr_rng      = r1u64(stop_event->address, stop_event->address);
      event->rip_vaddr      = stop_event->instruction_pointer;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  //////////////////////////////
  //- rjf: push request resolution event
  //
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_LaunchAndInitDone;
    evt->machine_id= CTRL_MachineID_Local;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = id;
    ctrl_c2u_push_events(&evts);
  }
  
  //////////////////////////////
  //- rjf: disable 'step-over-stuck' behavior
  //
  {
    ctrl_state->disable_stuck_thread_step = 1;
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__attach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  
  //- rjf: attach
  B32 attach_successful = dmn_ctrl_attach(ctrl_ctx, msg->entity_id);
  
  //- rjf: run to handshake
  if(attach_successful)
  {
    DMN_Handle unfrozen_process = {0};
    DMN_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_CreateProcess:
        {
          unfrozen_process = event->process;
          run_ctrls.run_entities = &unfrozen_process;
          run_ctrls.run_entity_count = 1;
        }break;
        case DMN_EventKind_HandshakeComplete:
        {
          done = 1;
        }break;
      }
    }
  }
  
  //- rjf: push request resolution event
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_AttachDone;
    evt->machine_id= CTRL_MachineID_Local;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = !!attach_successful * msg->entity_id;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__kill(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DMN_Handle process = msg->entity;
  U32 exit_code = msg->exit_code;
  
  //- rjf: send kill
  B32 kill_worked = dmn_ctrl_kill(ctrl_ctx, process, exit_code);
  
  //- rjf: wait for process to be dead
  if(kill_worked)
  {
    DMN_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    run_ctrls.run_entities = &process;
    run_ctrls.run_entity_count = 1;
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      if(event->kind == DMN_EventKind_ExitProcess && dmn_handle_match(event->process, process))
      {
        done = 1;
      }
    }
  }
  
  //- rjf: push request resolution event
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_KillDone;
    evt->machine_id= CTRL_MachineID_Local;
    evt->msg_id    = msg->msg_id;
    if(kill_worked)
    {
      evt->entity = msg->entity;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__detach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DMN_Handle process = msg->entity;
  
  //- rjf: detach
  B32 detach_worked = dmn_ctrl_detach(ctrl_ctx, process);
  
  //- rjf: wait for process to be dead
  if(detach_worked)
  {
    DMN_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    run_ctrls.run_entities = &process;
    run_ctrls.run_entity_count = 1;
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      if(event->kind == DMN_EventKind_ExitProcess && dmn_handle_match(event->process, process))
      {
        done = 1;
      }
    }
  }
  
  //- rjf: push request resolution event
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_DetachDone;
    evt->machine_id= CTRL_MachineID_Local;
    evt->msg_id    = msg->msg_id;
    if(detach_worked)
    {
      evt->entity = msg->entity;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__run(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DMN_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  DMN_Handle target_thread = msg->entity;
  DMN_Handle target_process = msg->parent;
  U64 spoof_ip_vaddr = 911;
  
  //////////////////////////////
  //- rjf: gather all initial breakpoints
  //
  DMN_TrapChunkList user_traps = {0};
  for(CTRL_Entity *machine = ctrl_state->ctrl_thread_entity_store->root->first;
      machine != &ctrl_entity_nil;
      machine = machine->next)
  {
    if(machine->kind != CTRL_EntityKind_Machine) { continue; }
    for(CTRL_Entity *process = machine->first; process != &ctrl_entity_nil; process = process->next)
    {
      if(process->kind != CTRL_EntityKind_Process) { continue; }
      
      // rjf: resolve module-dependent user bps
      for(CTRL_Entity *module = process->first; module != &ctrl_entity_nil; module = module->next)
      {
        if(module->kind != CTRL_EntityKind_Module) { continue; }
        ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, machine->machine_id, process->handle, module->handle, &msg->user_bps, &user_traps);
      }
      
      // rjf: push virtual-address user breakpoints per-process
      ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, machine->machine_id, process->handle, &msg->user_bps, &user_traps);
    }
  }
  
  //////////////////////////////
  //- rjf: read initial stack-pointer-check value
  //
  // This MUST happen before any threads move, including single-stepping stuck
  // threads, because otherwise, their stack pointer may change, if single-stepping
  // causes e.g. entrance into a function via a call instruction.
  //
  U64 sp_check_value = dmn_rsp_from_thread(target_thread);
  
  //////////////////////////////
  //- rjf: single step "stuck threads"
  //
  // "Stuck threads" are threads that are already on a User BP and would hit
  // it immediately if resumed with all User BPs enabled. To get them "unstuck"
  // we just need to single step them to get them off their current instruction.
  //
  // This only applies to threads OTHER THAN the target thread. If the target
  // thread is on a user breakpoint, then we need to let trap net logic run,
  // which may include features put on a trap net trap at the same address as
  // the user breakpoint.
  //
  B32 target_thread_is_on_user_bp_and_trap_net_trap = 0;
  if(ctrl_state->disable_stuck_thread_step)
  {
    ctrl_state->disable_stuck_thread_step = 0;
  }
  else if(stop_event == 0 && !ctrl_state->disable_stuck_thread_step)
  {
    // rjf: gather stuck threads
    DMN_HandleList stuck_threads = {0};
    for(CTRL_Entity *machine = ctrl_state->ctrl_thread_entity_store->root->first;
        machine != &ctrl_entity_nil;
        machine = machine->next)
    {
      if(machine->kind != CTRL_EntityKind_Machine) { continue; }
      for(CTRL_Entity *process = machine->first; process != &ctrl_entity_nil; process = process->next)
      {
        if(process->kind != CTRL_EntityKind_Process) { continue; }
        for(CTRL_Entity *thread = process->first; thread != &ctrl_entity_nil; thread = thread->next)
        {
          U64 rip = dmn_rip_from_thread(thread->handle);
          
          // rjf: determine if thread is frozen
          B32 thread_is_frozen = !msg->freeze_state_is_frozen;
          for(CTRL_MachineIDHandlePairNode *n = msg->freeze_state_threads.first; n != 0; n = n->next)
          {
            if(dmn_handle_match(n->v.handle, thread->handle))
            {
              thread_is_frozen ^= 1;
              break;
            }
          }
          
          // rjf: not frozen? -> check if stuck & gather if so
          if(thread_is_frozen == 0)
          {
            for(DMN_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
            {
              B32 is_on_user_bp = 0;
              for(DMN_Trap *trap_ptr = n->v; trap_ptr < n->v+n->count; trap_ptr += 1)
              {
                if(dmn_handle_match(trap_ptr->process, process->handle) && trap_ptr->vaddr == rip)
                {
                  is_on_user_bp = 1;
                }
              }
              
              B32 is_on_net_trap = 0;
              for(CTRL_TrapNode *n = msg->traps.first; n != 0; n = n->next)
              {
                if(n->v.vaddr == rip)
                {
                  is_on_net_trap = 1;
                }
              }
              
              if(is_on_user_bp && (!is_on_net_trap || !dmn_handle_match(thread->handle, target_thread)))
              {
                dmn_handle_list_push(scratch.arena, &stuck_threads, thread->handle);
              }
              
              if(is_on_user_bp && is_on_net_trap && dmn_handle_match(thread->handle, target_thread))
              {
                target_thread_is_on_user_bp_and_trap_net_trap = 1;
              }
            }
          }
        }
      }
    }
    
    // rjf: actually step stuck threads
    for(DMN_HandleNode *node = stuck_threads.first;
        node != 0;
        node = node->next)
    {
      DMN_RunCtrls run_ctrls = {0};
      run_ctrls.single_step_thread = node->v;
      for(B32 done = 0; done == 0;)
      {
        DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
        switch(event->kind)
        {
          default:{}break;
          case DMN_EventKind_Error:      stop_cause = CTRL_EventCause_Error; goto stop;
          case DMN_EventKind_Exception:  stop_cause = CTRL_EventCause_InterruptedByException; goto stop;
          case DMN_EventKind_Trap:       stop_cause = CTRL_EventCause_InterruptedByTrap; goto stop;
          case DMN_EventKind_Halt:       stop_cause = CTRL_EventCause_InterruptedByHalt; goto stop;
          stop:;
          {
            stop_event = event;
            done = 1;
          }break;
          case DMN_EventKind_SingleStep:
          {
            done = 1;
          }break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: resolve trap net
  //
  DMN_TrapChunkList trap_net_traps = {0};
  for(CTRL_TrapNode *node = msg->traps.first;
      node != 0;
      node = node->next)
  {
    DMN_Trap trap = {target_process, node->v.vaddr};
    dmn_trap_chunk_list_push(scratch.arena, &trap_net_traps, 256, &trap);
  }
  
  //////////////////////////////
  //- rjf: join user breakpoints and trap net traps
  //
  DMN_TrapChunkList joined_traps = {0};
  {
    dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &user_traps);
    dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &trap_net_traps);
  }
  
  //////////////////////////////
  //- rjf: record start
  //
  if(stop_event == 0)
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //////////////////////////////
  //- rjf: run loop
  //
  if(stop_event == 0)
  {
    B32 spoof_mode = 0;
    CTRL_Spoof spoof = {0};
    for(;;)
    {
      //////////////////////////
      //- rjf: choose low level traps
      //
      DMN_TrapChunkList *trap_list = &joined_traps;
      if(spoof_mode)
      {
        trap_list = &user_traps;
      }
      
      //////////////////////////
      //- rjf: choose spoof
      //
      CTRL_Spoof *run_spoof = 0;
      if(spoof_mode)
      {
        run_spoof = &spoof;
      }
      
      //////////////////////////
      //- rjf: setup run controls
      //
      DMN_RunCtrls run_ctrls = {0};
      run_ctrls.ignore_previous_exception = 1;
      run_ctrls.run_entity_count = msg->freeze_state_threads.count;
      run_ctrls.run_entities     = push_array(scratch.arena, DMN_Handle, run_ctrls.run_entity_count);
      run_ctrls.run_entities_are_unfrozen = !msg->freeze_state_is_frozen;
      {
        U64 idx = 0;
        for(CTRL_MachineIDHandlePairNode *n = msg->freeze_state_threads.first; n != 0; n = n->next)
        {
          run_ctrls.run_entities[idx] = n->v.handle;
          idx += 1;
        }
      }
      run_ctrls.traps = *trap_list;
      
      //////////////////////////
      //- rjf: get next event
      //
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, run_spoof);
      
      //////////////////////////
      //- rjf: determine event handling
      //
      B32 hard_stop = 0;
      CTRL_EventCause hard_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
      B32 use_stepping_logic = 0;
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_Error:
        case DMN_EventKind_Halt:
        case DMN_EventKind_SingleStep:
        case DMN_EventKind_Trap:
        {
          hard_stop = 1;
        }break;
        case DMN_EventKind_Exception:
        case DMN_EventKind_Breakpoint:
        {
          use_stepping_logic = 1;
        }break;
        case DMN_EventKind_CreateProcess:
        {
          DMN_TrapChunkList new_traps = {0};
          ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, CTRL_MachineID_Local, event->process, &msg->user_bps, &new_traps);
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
        }break;
        case DMN_EventKind_LoadModule:
        {
          DMN_TrapChunkList new_traps = {0};
          ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, CTRL_MachineID_Local, event->process, event->module, &msg->user_bps, &new_traps);
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &user_traps, &new_traps);
        }break;
      }
      
      //////////////////////////
      //- rjf: unpack info about thread attached to event
      //
      CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, event->thread);
      Architecture arch = thread->arch;
      U64 thread_rip_vaddr = dmn_rsp_from_thread(event->thread);
      DMN_Handle module = {0};
      String8 module_name = {0};
      U64 module_base_vaddr = 0;
      U64 thread_rip_voff = 0;
      {
        CTRL_Entity *process = ctrl_entity_from_machine_id_handle(ctrl_state->ctrl_thread_entity_store, CTRL_MachineID_Local, event->process);
        for(CTRL_Entity *module = process->first; module != &ctrl_entity_nil; module = module->next)
        {
          if(module->kind == CTRL_EntityKind_Module && contains_1u64(module->vaddr_range, thread_rip_vaddr))
          {
            module_name = module->string;
            module_base_vaddr = module->vaddr_range.min;
            thread_rip_voff = thread_rip_vaddr - module->vaddr_range.min;
            break;
          }
        }
      }
      
      //////////////////////////
      //- rjf: stepping logic
      //
      //{
      
      //////////////////////////
      //- rjf: handle if hitting a spoof or baked in trap
      //
      B32 hit_spoof = 0;
      B32 exception_stop = 0;
      if(use_stepping_logic)
      {
        if(event->kind == DMN_EventKind_Exception)
        {
          // rjf: spoof check
          if(spoof_mode &&
             dmn_handle_match(target_process, event->process) &&
             dmn_handle_match(target_thread, event->thread) &&
             spoof.new_ip_value == event->instruction_pointer)
          {
            hit_spoof = 1;
          }
          
          // rjf: other exceptions cause stop
          if(!hit_spoof)
          {
            exception_stop = 1;
            use_stepping_logic = 0;
          }
        }
      }
      
      //- rjf: handle spoof hit
      if(hit_spoof)
      {
        // rjf: clear spoof mode
        spoof_mode = 0;
        MemoryZeroStruct(&spoof);
        
        // rjf: skip remainder of handling
        use_stepping_logic = 0;
      }
      
      //- rjf: for breakpoint events, gather bp info
      B32 hit_user_bp = 0;
      B32 hit_trap_net_bp = 0;
      B32 hit_conditional_bp_but_filtered = 0;
      CTRL_TrapFlags hit_trap_flags = 0;
      if(use_stepping_logic)
      {
        if(event->kind == DMN_EventKind_Breakpoint)
        {
          Temp temp = temp_begin(scratch.arena);
          String8List conditions = {0};
          
          // rjf: user breakpoints
          for(DMN_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
          {
            DMN_Trap *trap = n->v;
            DMN_Trap *opl = n->v + n->count;
            for(;trap < opl; trap += 1)
            {
              if(dmn_handle_match(trap->process, event->process) &&
                 trap->vaddr == event->instruction_pointer &&
                 (!dmn_handle_match(event->thread, target_thread) || !target_thread_is_on_user_bp_and_trap_net_trap))
              {
                CTRL_UserBreakpoint *user_bp = (CTRL_UserBreakpoint *)trap->id;
                hit_user_bp = 1;
                if(user_bp != 0 && user_bp->condition.size != 0)
                {
                  str8_list_push(temp.arena, &conditions, user_bp->condition);
                }
              }
            }
          }
          
          // rjf: evaluate hit stop conditions
          if(conditions.node_count != 0)
          {
            String8 exe_path = module_name;
            DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
            RDI_Parsed *rdi = &dbgi->rdi;
            for(String8Node *condition_n = conditions.first; condition_n != 0; condition_n = condition_n->next)
            {
              String8 string = condition_n->string;
              EVAL_ParseCtx parse_ctx = zero_struct;
              {
                parse_ctx.arch = arch;
                parse_ctx.ip_voff = thread_rip_voff;
                parse_ctx.rdi = rdi;
                parse_ctx.type_graph = tg_graph_begin(bit_size_from_arch(arch)/8, 256);
                parse_ctx.regs_map = ctrl_string2reg_from_arch(arch);
                parse_ctx.reg_alias_map = ctrl_string2alias_from_arch(arch);
                parse_ctx.locals_map = eval_push_locals_map_from_rdi_voff(temp.arena, rdi, thread_rip_voff);
                parse_ctx.member_map = eval_push_member_map_from_rdi_voff(temp.arena, rdi, thread_rip_voff);
              }
              EVAL_TokenArray tokens = eval_token_array_from_text(temp.arena, string);
              EVAL_ParseResult parse = eval_parse_expr_from_text_tokens(temp.arena, &parse_ctx, string, &tokens);
              EVAL_ErrorList errors = parse.errors;
              B32 parse_has_expr = (parse.expr != &eval_expr_nil);
              B32 parse_is_type = (parse_has_expr && parse.expr->kind == EVAL_ExprKind_TypeIdent);
              EVAL_IRTreeAndType ir_tree_and_type = {&eval_irtree_nil};
              if(parse_has_expr && errors.count == 0)
              {
                ir_tree_and_type = eval_irtree_and_type_from_expr(temp.arena, parse_ctx.type_graph, rdi, &eval_string2expr_map_nil, parse.expr, &errors);
              }
              EVAL_OpList op_list = {0};
              if(parse_has_expr && ir_tree_and_type.tree != &eval_irtree_nil)
              {
                eval_oplist_from_irtree(scratch.arena, ir_tree_and_type.tree, &op_list);
              }
              String8 bytecode = {0};
              if(parse_has_expr && parse_is_type == 0 && op_list.encoded_size != 0)
              {
                bytecode = eval_bytecode_from_oplist(scratch.arena, &op_list);
              }
              EVAL_Result eval = {0};
              if(bytecode.size != 0)
              {
                U64 module_base = module_base_vaddr;
                U64 tls_base = dmn_tls_root_vaddr_from_thread(event->thread);
                EVAL_Machine machine = {0};
                machine.u = &event->process;
                machine.arch = arch;
                machine.memory_read = ctrl_eval_memory_read;
                machine.reg_size = regs_block_size_from_architecture(arch);
                machine.reg_data = push_array(scratch.arena, U8, machine.reg_size);
                machine.module_base = &module_base;
                machine.tls_base = &tls_base;
                dmn_thread_read_reg_block(event->thread, machine.reg_data);
                eval = eval_interpret(&machine, bytecode);
              }
              if(eval.code == EVAL_ResultCode_Good && eval.value.u64 == 0)
              {
                hit_user_bp = 0;
                hit_conditional_bp_but_filtered = 1;
              }
              else
              {
                hit_user_bp = 1;
                hit_conditional_bp_but_filtered = 0;
                break;
              }
            }
          }
          
          // rjf: gather trap net hits
          if(!hit_user_bp && dmn_handle_match(event->process, target_process))
          {
            for(CTRL_TrapNode *node = msg->traps.first;
                node != 0;
                node = node->next)
            {
              if(node->v.vaddr == event->instruction_pointer)
              {
                hit_trap_net_bp = 1;
                hit_trap_flags |= node->v.flags;
              }
            }
          }
          
          temp_end(temp);
        }
      }
      
      //- rjf: hit conditional user bp but filtered -> single step
      B32 cond_bp_single_step_stop = 0;
      CTRL_EventCause cond_bp_single_step_stop_cause = CTRL_EventCause_Null;
      if(hit_conditional_bp_but_filtered)
      {
        DMN_RunCtrls single_step_ctrls = {0};
        single_step_ctrls.single_step_thread = event->thread;
        for(B32 single_step_done = 0; single_step_done == 0;)
        {
          DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
          switch(event->kind)
          {
            default:{}break;
            case DMN_EventKind_Error:
            case DMN_EventKind_Exception:
            case DMN_EventKind_Halt:
            case DMN_EventKind_Trap:
            {
              cond_bp_single_step_stop = 1;
              single_step_done = 1;
              use_stepping_logic = 0;
              cond_bp_single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
            }break;
            case DMN_EventKind_SingleStep:
            {
              single_step_done = 1;
              cond_bp_single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
            }break;
          }
        }
      }
      
      //- rjf: user breakpoints on *any thread* cause a stop
      B32 user_bp_stop = 0;
      if(use_stepping_logic && hit_user_bp)
      {
        user_bp_stop = 1;
        use_stepping_logic = 0;
      }
      
      //- rjf: trap net on off-target threads are ignored
      B32 step_past_trap_net = 0;
      if(use_stepping_logic && hit_trap_net_bp)
      {
        if(!dmn_handle_match(event->thread, target_thread))
        {
          step_past_trap_net = 1;
          use_stepping_logic = 0;
        }
      }
      
      //- rjf: trap net on on-target threads trigger trap net logic
      B32 use_trap_net_logic = 0;
      if(use_stepping_logic && hit_trap_net_bp)
      {
        if(dmn_handle_match(event->thread, target_thread))
        {
          use_trap_net_logic = 1;
        }
      }
      
      //- rjf: trap net logic: stack pointer check
      B32 stack_pointer_matches = 0;
      if(use_trap_net_logic)
      {
        U64 sp = dmn_rsp_from_thread(target_thread);
        stack_pointer_matches = (sp == sp_check_value);
      }
      
      //- rjf: trap net logic: single step after hit
      B32 single_step_stop = 0;
      CTRL_EventCause single_step_stop_cause = CTRL_EventCause_Null;
      if(use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_SingleStepAfterHit)
        {
          DMN_RunCtrls single_step_ctrls = {0};
          single_step_ctrls.single_step_thread = target_thread;
          for(B32 single_step_done = 0; single_step_done == 0;)
          {
            DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
            switch(event->kind)
            {
              default:{}break;
              case DMN_EventKind_Error:
              case DMN_EventKind_Exception:
              case DMN_EventKind_Halt:
              case DMN_EventKind_Trap:
              {
                single_step_stop = 1;
                single_step_done = 1;
                use_stepping_logic = 0;
                use_trap_net_logic = 0;
                single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
              }break;
              case DMN_EventKind_SingleStep:
              {
                single_step_done = 1;
                single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
              }break;
            }
          }
        }
      }
      
      //- rjf: trap net logic: begin spoof mode
      B32 begin_spoof_mode = 0;
      if(use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_BeginSpoofMode)
        {
          // rjf: setup spoof mode
          begin_spoof_mode = 1;
          U64 spoof_sp = dmn_rsp_from_thread(target_thread);
          spoof_mode = 1;
          spoof.process = target_process;
          spoof.thread  = target_thread;
          spoof.vaddr   = spoof_sp;
          spoof.new_ip_value = spoof_ip_vaddr;
        }
      }
      
      //- rjf: trap net logic: save stack pointer
      B32 save_stack_pointer = 0;
      if(use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_SaveStackPointer)
        {
          if(stack_pointer_matches)
          {
            save_stack_pointer = 1;
            sp_check_value = dmn_rsp_from_thread(target_thread);
          }
        }
      }
      
      //- rjf: trap net logic: end stepping
      B32 trap_net_stop = 0;
      if(use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_EndStepping)
        {
          if((hit_trap_flags & CTRL_TrapFlag_IgnoreStackPointerCheck) ||
             stack_pointer_matches)
          {
            trap_net_stop = 1;
            use_trap_net_logic = 0;
          }
        }
      }
      
      //}
      //
      //- rjf: stepping logic
      ////////////////////////////////
      
      //- rjf: handle step past trap net
      B32 step_past_trap_net_stop = 0;
      CTRL_EventCause step_past_trap_net_stop_cause = CTRL_EventCause_Null;
      if(step_past_trap_net)
      {
        DMN_RunCtrls single_step_ctrls = {0};
        single_step_ctrls.single_step_thread = event->thread;
        for(B32 single_step_done = 0; single_step_done == 0;)
        {
          DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
          switch(event->kind)
          {
            default:{}break;
            case DMN_EventKind_Error:
            case DMN_EventKind_Exception:
            case DMN_EventKind_Halt:
            case DMN_EventKind_Trap:
            {
              step_past_trap_net_stop = 1;
              single_step_done = 1;
              step_past_trap_net_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
            }break;
            case DMN_EventKind_SingleStep:
            {
              single_step_done = 1;
              step_past_trap_net_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
            }break;
          }
        }
      }
      
      //- rjf: loop exit condition
      CTRL_EventCause stage_stop_cause = CTRL_EventCause_Null;
      if(hard_stop)
      {
        stage_stop_cause = hard_stop_cause;
      }
      else if(cond_bp_single_step_stop)
      {
        stage_stop_cause = cond_bp_single_step_stop_cause;
      }
      else if(single_step_stop)
      {
        stage_stop_cause = single_step_stop_cause;
      }
      else if(step_past_trap_net_stop)
      {
        stage_stop_cause = step_past_trap_net_stop_cause;
      }
      else if(exception_stop)
      {
        stage_stop_cause = CTRL_EventCause_InterruptedByException;
      }
      else if(user_bp_stop)
      {
        stage_stop_cause = CTRL_EventCause_UserBreakpoint;
      }
      else if(trap_net_stop)
      {
        stage_stop_cause = CTRL_EventCause_Finished;
      }
      if(stage_stop_cause != CTRL_EventCause_Null)
      {
        stop_event = event;
        stop_cause = stage_stop_cause;
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: record stop
  //
  if(stop_event != 0)
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Stopped;
    event->cause = stop_cause;
    event->machine_id = CTRL_MachineID_Local;
    event->entity = stop_event->thread;
    event->parent = stop_event->process;
    event->exception_code = stop_event->code;
    event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
    event->rip_vaddr = stop_event->instruction_pointer;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__single_step(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  
  //- rjf: record start
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: single step
  DMN_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  {
    DMN_RunCtrls run_ctrls = {0};
    run_ctrls.single_step_thread = msg->entity;
    for(B32 done = 0; done == 0;)
    {
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_Error:      {stop_cause = CTRL_EventCause_Error;}goto end_single_step;
        case DMN_EventKind_Exception:  {stop_cause = CTRL_EventCause_InterruptedByException;}goto end_single_step;
        case DMN_EventKind_Halt:       {stop_cause = CTRL_EventCause_InterruptedByHalt;}goto end_single_step;
        case DMN_EventKind_Trap:       {stop_cause = CTRL_EventCause_InterruptedByTrap;}goto end_single_step;
        case DMN_EventKind_SingleStep: {stop_cause = CTRL_EventCause_Finished;}goto end_single_step;
        case DMN_EventKind_Breakpoint: {stop_cause = CTRL_EventCause_UserBreakpoint;}goto end_single_step;
        end_single_step:
        {
          stop_event = event;
          done = 1;
        }break;
      }
    }
  }
  
  //- rjf: record stop
  if(stop_event != 0)
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Stopped;
    event->cause = stop_cause;
    event->machine_id = CTRL_MachineID_Local;
    event->entity = stop_event->thread;
    event->parent = stop_event->process;
    event->exception_code = stop_event->code;
    event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
    event->rip_vaddr = stop_event->instruction_pointer;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Memory-Stream-Thread-Only Functions

//- rjf: user -> memory stream communication

internal B32
ctrl_u2ms_enqueue_req(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    U64 available_size = ctrl_state->u2ms_ring_size-unconsumed_size;
    if(available_size >= sizeof(machine_id)+sizeof(process)+sizeof(vaddr_range))
    {
      good = 1;
      ctrl_state->u2ms_ring_write_pos += ring_write_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_write_pos, &machine_id);
      ctrl_state->u2ms_ring_write_pos += ring_write_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_write_pos, &process);
      ctrl_state->u2ms_ring_write_pos += ring_write_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_write_pos, &vaddr_range);
      ctrl_state->u2ms_ring_write_pos += ring_write_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_write_pos, &zero_terminated);
      break;
    }
    if(os_now_microseconds() >= endt_us) {break;}
    os_condition_variable_wait(ctrl_state->u2ms_ring_cv, ctrl_state->u2ms_ring_mutex, endt_us);
  }
  os_condition_variable_broadcast(ctrl_state->u2ms_ring_cv);
  return good;
}

internal void
ctrl_u2ms_dequeue_req(CTRL_MachineID *out_machine_id, DMN_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated)
{
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    if(unconsumed_size >= sizeof(*out_machine_id)+sizeof(*out_process)+sizeof(*out_vaddr_range))
    {
      ctrl_state->u2ms_ring_read_pos += ring_read_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_read_pos, out_machine_id);
      ctrl_state->u2ms_ring_read_pos += ring_read_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_read_pos, out_process);
      ctrl_state->u2ms_ring_read_pos += ring_read_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_read_pos, out_vaddr_range);
      ctrl_state->u2ms_ring_read_pos += ring_read_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_read_pos, out_zero_terminated);
      break;
    }
    os_condition_variable_wait(ctrl_state->u2ms_ring_cv, ctrl_state->u2ms_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ctrl_state->u2ms_ring_cv);
}

//- rjf: entry point

internal void
ctrl_mem_stream_thread__entry_point(void *p)
{
  CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
  for(;;)
  {
    //- rjf: unpack next request
    CTRL_MachineID machine_id = 0;
    DMN_Handle process = {0};
    Rng1U64 vaddr_range = {0};
    B32 zero_terminated = 0;
    ctrl_u2ms_dequeue_req(&machine_id, &process, &vaddr_range, &zero_terminated);
    U128 key = ctrl_hash_store_key_from_process_vaddr_range(machine_id, process, vaddr_range, zero_terminated);
    
    //- rjf: unpack process memory cache key
    U64 process_hash = ctrl_hash_from_string(str8_struct(&process));
    U64 process_slot_idx = process_hash%cache->slots_count;
    U64 process_stripe_idx = process_slot_idx%cache->stripes_count;
    CTRL_ProcessMemoryCacheSlot *process_slot = &cache->slots[process_slot_idx];
    CTRL_ProcessMemoryCacheStripe *process_stripe = &cache->stripes[process_stripe_idx];
    
    //- rjf: unpack address range hash cache key
    U64 range_hash = ctrl_hash_from_string(str8_struct(&vaddr_range));
    
    //- rjf: take task
    B32 got_task = 0;
    U64 preexisting_mem_gen = 0;
    U128 preexisting_hash = {0};
    Rng1U64 vaddr_range_clamped = {0};
    OS_MutexScopeW(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &vaddr_range) && range_n->zero_terminated == zero_terminated)
            {
              got_task = !ins_atomic_u32_eval_cond_assign(&range_n->is_taken, 1, 0);
              preexisting_mem_gen = range_n->mem_gen;
              preexisting_hash = range_n->hash;
              vaddr_range_clamped = range_n->vaddr_range_clamped;
              goto take_task__break_all;
            }
          }
        }
      }
      take_task__break_all:;
    }
    
    //- rjf: task was taken -> read memory
    U64 range_size = 0;
    Arena *range_arena = 0;
    void *range_base = 0;
    U64 zero_terminated_size = 0;
    U64 mem_gen = dmn_mem_gen();
    if(got_task && mem_gen != preexisting_mem_gen)
    {
      range_size = dim_1u64(vaddr_range_clamped);
      U64 arena_size = AlignPow2(range_size + ARENA_HEADER_SIZE, os_page_size());
      range_arena = arena_alloc__sized(range_size+ARENA_HEADER_SIZE, range_size+ARENA_HEADER_SIZE);
      if(range_arena == 0)
      {
        range_size = 0;
      }
      else
      {
        range_base = push_array_no_zero(range_arena, U8, range_size);
        U64 bytes_read = dmn_process_read(process, vaddr_range_clamped, range_base);
        if(bytes_read == 0)
        {
          arena_release(range_arena);
          range_base = 0;
          range_size = 0;
          range_arena = 0;
        }
        else if(bytes_read < range_size)
        {
          MemoryZero((U8 *)range_base + bytes_read, range_size-bytes_read);
        }
        zero_terminated_size = range_size;
        if(zero_terminated)
        {
          for(U64 idx = 0; idx < bytes_read; idx += 1)
          {
            if(((U8 *)range_base)[idx] == 0)
            {
              zero_terminated_size = idx;
              break;
            }
          }
        }
      }
    }
    
    //- rjf: read successful -> submit to hash store
    U128 hash = {0};
    if(got_task && range_base != 0)
    {
      hash = hs_submit_data(key, &range_arena, str8((U8*)range_base, zero_terminated_size));
    }
    
    //- rjf: commit hash to cache
    if(got_task) OS_MutexScopeW(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(n->machine_id == machine_id && dmn_handle_match(n->process, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &vaddr_range) && range_n->zero_terminated == zero_terminated)
            {
              if(!u128_match(u128_zero(), hash))
              {
                range_n->hash = hash;
                range_n->mem_gen = mem_gen;
              }
              ins_atomic_u32_eval_assign(&range_n->is_taken, 0);
              goto commit__break_all;
            }
          }
        }
      }
      commit__break_all:;
    }
    
    //- rjf: broadcast changes
    os_condition_variable_broadcast(process_stripe->cv);
  }
}
