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
ctrl_hash_from_handle(CTRL_Handle handle)
{
  U64 buf[] = {handle.machine_id, handle.dmn_handle.u64[0]};
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

internal String8
ctrl_string_from_event_kind(CTRL_EventKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    default:{}break;
    case CTRL_EventKind_Null:                              { result = str8_lit("Null");}break;
    case CTRL_EventKind_Error:                             { result = str8_lit("Error");}break;
    case CTRL_EventKind_Started:                           { result = str8_lit("Started");}break;
    case CTRL_EventKind_Stopped:                           { result = str8_lit("Stopped");}break;
    case CTRL_EventKind_NewProc:                           { result = str8_lit("NewProc");}break;
    case CTRL_EventKind_NewThread:                         { result = str8_lit("NewThread");}break;
    case CTRL_EventKind_NewModule:                         { result = str8_lit("NewModule");}break;
    case CTRL_EventKind_EndProc:                           { result = str8_lit("EndProc");}break;
    case CTRL_EventKind_EndThread:                         { result = str8_lit("EndThread");}break;
    case CTRL_EventKind_EndModule:                         { result = str8_lit("EndModule");}break;
    case CTRL_EventKind_ModuleDebugInfoPathChange:         { result = str8_lit("ModuleDebugInfoPathChange");}break;
    case CTRL_EventKind_DebugString:                       { result = str8_lit("DebugString");}break;
    case CTRL_EventKind_ThreadName:                        { result = str8_lit("ThreadName");}break;
    case CTRL_EventKind_MemReserve:                        { result = str8_lit("MemReserve");}break;
    case CTRL_EventKind_MemCommit:                         { result = str8_lit("MemCommit");}break;
    case CTRL_EventKind_MemDecommit:                       { result = str8_lit("MemDecommit");}break;
    case CTRL_EventKind_MemRelease:                        { result = str8_lit("MemRelease");}break;
  }
  return result;
}

internal String8
ctrl_string_from_msg_kind(CTRL_MsgKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    default:{}break;
    case CTRL_MsgKind_Launch:                    {result = str8_lit("Launch");}break;
    case CTRL_MsgKind_Attach:                    {result = str8_lit("Attach");}break;
    case CTRL_MsgKind_Kill:                      {result = str8_lit("Kill");}break;
    case CTRL_MsgKind_KillAll:                   {result = str8_lit("KillAll");}break;
    case CTRL_MsgKind_Detach:                    {result = str8_lit("Detach");}break;
    case CTRL_MsgKind_Run:                       {result = str8_lit("Run");}break;
    case CTRL_MsgKind_SingleStep:                {result = str8_lit("SingleStep");}break;
    case CTRL_MsgKind_SetUserEntryPoints:        {result = str8_lit("SetUserEntryPoints");}break;
    case CTRL_MsgKind_SetModuleDebugInfoPath:    {result = str8_lit("SetModuleDebugInfoPath");}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Machine/Handle Pair Type Functions

internal CTRL_Handle
ctrl_handle_zero(void)
{
  CTRL_Handle handle = {0};
  return handle;
}

internal CTRL_Handle
ctrl_handle_make(CTRL_MachineID machine_id, DMN_Handle dmn_handle)
{
  CTRL_Handle handle = {machine_id, dmn_handle};
  return handle;
}

internal B32
ctrl_handle_match(CTRL_Handle a, CTRL_Handle b)
{
  B32 result = (a.machine_id == b.machine_id &&
                dmn_handle_match(a.dmn_handle, b.dmn_handle));
  return result;
}

internal void
ctrl_handle_list_push(Arena *arena, CTRL_HandleList *list, CTRL_Handle *pair)
{
  CTRL_HandleNode *n = push_array(arena, CTRL_HandleNode, 1);
  MemoryCopyStruct(&n->v, pair);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal CTRL_HandleList
ctrl_handle_list_copy(Arena *arena, CTRL_HandleList *src)
{
  CTRL_HandleList dst = {0};
  for(CTRL_HandleNode *n = src->first; n != 0; n = n->next)
  {
    ctrl_handle_list_push(arena, &dst, &n->v);
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
  dst->entry_points         = str8_list_copy(arena, &src->entry_points);
  dst->cmd_line_string_list = str8_list_copy(arena, &src->cmd_line_string_list);
  dst->env_string_list      = str8_list_copy(arena, &src->env_string_list);
  dst->traps                = ctrl_trap_list_copy(arena, &src->traps);
  dst->user_bps             = ctrl_user_breakpoint_list_copy(arena, &src->user_bps);
  dst->meta_evals           = *deep_copy_from_struct(arena, CTRL_MetaEvalArray, &src->meta_evals);
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

internal CTRL_MsgList
ctrl_msg_list_deep_copy(Arena *arena, CTRL_MsgList *src)
{
  CTRL_MsgList dst = {0};
  for(CTRL_MsgNode *n = src->first; n != 0; n = n->next)
  {
    CTRL_Msg *src_msg = &n->v;
    CTRL_Msg *dst_msg = ctrl_msg_list_push(arena, &dst);
    ctrl_msg_deep_copy(arena, dst_msg, src_msg);
  }
  return dst;
}

internal void
ctrl_msg_list_concat_in_place(CTRL_MsgList *dst, CTRL_MsgList *src)
{
  if(dst->last && src->first)
  {
    dst->last->next = src->first;
    dst->last = src->last;
    dst->count += src->count;
  }
  else if(src->first)
  {
    MemoryCopyStruct(dst, src);
  }
  MemoryZeroStruct(src);
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
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->run_flags);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->msg_id);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->entity);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->parent);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->entity_id);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->exit_code);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->env_inherit);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->debug_subprocesses);
      str8_serial_push_array (scratch.arena, &msgs_srlzed, &msg->exception_code_filters[0], ArrayCount(msg->exception_code_filters));
      
      // rjf: write path string
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->path.size);
      str8_serial_push_data(scratch.arena, &msgs_srlzed, msg->path.str, msg->path.size);
      
      // rjf: write entry point string list
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->entry_points.node_count);
      for(String8Node *n = msg->entry_points.first; n != 0; n = n->next)
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
      
      // rjf: write stdout/stderr/stdin paths
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->stdout_path.size);
      str8_serial_push_string(scratch.arena, &msgs_srlzed, msg->stdout_path);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->stderr_path.size);
      str8_serial_push_string(scratch.arena, &msgs_srlzed, msg->stderr_path);
      str8_serial_push_struct(scratch.arena, &msgs_srlzed, &msg->stdin_path.size);
      str8_serial_push_string(scratch.arena, &msgs_srlzed, msg->stdin_path);
      
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
      
      // rjf: write meta-eval-info array
      String8 meta_evals_srlzed = serialized_from_struct(scratch.arena, CTRL_MetaEvalArray, &msg->meta_evals);
      str8_serial_push_string(scratch.arena, &msgs_srlzed, meta_evals_srlzed);
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
      read_off += str8_deserial_read_struct(string, read_off, &msg->run_flags);
      read_off += str8_deserial_read_struct(string, read_off, &msg->msg_id);
      read_off += str8_deserial_read_struct(string, read_off, &msg->entity);
      read_off += str8_deserial_read_struct(string, read_off, &msg->parent);
      read_off += str8_deserial_read_struct(string, read_off, &msg->entity_id);
      read_off += str8_deserial_read_struct(string, read_off, &msg->exit_code);
      read_off += str8_deserial_read_struct(string, read_off, &msg->env_inherit);
      read_off += str8_deserial_read_struct(string, read_off, &msg->debug_subprocesses);
      read_off += str8_deserial_read_array (string, read_off, &msg->exception_code_filters[0], ArrayCount(msg->exception_code_filters));
      
      // rjf: read path string
      read_off += str8_deserial_read_struct(string, read_off, &msg->path.size);
      msg->path.str = push_array_no_zero(arena, U8, msg->path.size);
      read_off += str8_deserial_read(string, read_off, msg->path.str, msg->path.size, 1);
      
      // rjf: read entry point string list
      U64 entry_point_list_string_count = 0;
      read_off += str8_deserial_read_struct(string, read_off, &entry_point_list_string_count);
      for(U64 idx = 0; idx < entry_point_list_string_count; idx += 1)
      {
        String8 str = {0};
        read_off += str8_deserial_read_struct(string, read_off, &str.size);
        str.str = push_array_no_zero(arena, U8, str.size);
        read_off += str8_deserial_read(string, read_off, str.str, str.size, 1);
        str8_list_push(arena, &msg->entry_points, str);
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
      
      // rjf: read stdout/stderr/stdin paths
      read_off += str8_deserial_read_struct(string, read_off, &msg->stdout_path.size);
      msg->stdout_path.str = push_array(arena, U8, msg->stdout_path.size);
      read_off += str8_deserial_read(string, read_off, msg->stdout_path.str, msg->stdout_path.size, 1);
      read_off += str8_deserial_read_struct(string, read_off, &msg->stderr_path.size);
      msg->stderr_path.str = push_array(arena, U8, msg->stderr_path.size);
      read_off += str8_deserial_read(string, read_off, msg->stderr_path.str, msg->stderr_path.size, 1);
      read_off += str8_deserial_read_struct(string, read_off, &msg->stdin_path.size);
      msg->stdin_path.str = push_array(arena, U8, msg->stdin_path.size);
      read_off += str8_deserial_read(string, read_off, msg->stdin_path.str, msg->stdin_path.size, 1);
      
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
      
      // rjf: read meta-eval-info array
      String8 meta_evals_data = str8_skip(string, read_off);
      U64 meta_evals_size = 0;
      msg->meta_evals = *struct_from_serialized(arena, CTRL_MetaEvalArray, meta_evals_data, .advance_out = &meta_evals_size);
      read_off += meta_evals_size;
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
ctrl_serialized_string_from_event(Arena *arena, CTRL_Event *event, U64 max)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  {
    str8_serial_push_struct(scratch.arena, &srl, &event->kind);
    str8_serial_push_struct(scratch.arena, &srl, &event->cause);
    str8_serial_push_struct(scratch.arena, &srl, &event->exception_kind);
    str8_serial_push_struct(scratch.arena, &srl, &event->msg_id);
    str8_serial_push_struct(scratch.arena, &srl, &event->entity);
    str8_serial_push_struct(scratch.arena, &srl, &event->parent);
    str8_serial_push_struct(scratch.arena, &srl, &event->arch);
    str8_serial_push_struct(scratch.arena, &srl, &event->u64_code);
    str8_serial_push_struct(scratch.arena, &srl, &event->entity_id);
    str8_serial_push_struct(scratch.arena, &srl, &event->vaddr_rng);
    str8_serial_push_struct(scratch.arena, &srl, &event->rip_vaddr);
    str8_serial_push_struct(scratch.arena, &srl, &event->stack_base);
    str8_serial_push_struct(scratch.arena, &srl, &event->tls_root);
    str8_serial_push_struct(scratch.arena, &srl, &event->timestamp);
    str8_serial_push_struct(scratch.arena, &srl, &event->rgba);
    str8_serial_push_struct(scratch.arena, &srl, &event->exception_code);
    String8 string = event->string;
    string.size = Min(string.size, max-srl.total_size);
    str8_serial_push_struct(scratch.arena, &srl, &string.size);
    str8_serial_push_data(scratch.arena, &srl, string.str, string.size);
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
    read_off += str8_deserial_read_struct(string, read_off, &event.entity);
    read_off += str8_deserial_read_struct(string, read_off, &event.parent);
    read_off += str8_deserial_read_struct(string, read_off, &event.arch);
    read_off += str8_deserial_read_struct(string, read_off, &event.u64_code);
    read_off += str8_deserial_read_struct(string, read_off, &event.entity_id);
    read_off += str8_deserial_read_struct(string, read_off, &event.vaddr_rng);
    read_off += str8_deserial_read_struct(string, read_off, &event.rip_vaddr);
    read_off += str8_deserial_read_struct(string, read_off, &event.stack_base);
    read_off += str8_deserial_read_struct(string, read_off, &event.tls_root);
    read_off += str8_deserial_read_struct(string, read_off, &event.timestamp);
    read_off += str8_deserial_read_struct(string, read_off, &event.exception_code);
    read_off += str8_deserial_read_struct(string, read_off, &event.rgba);
    read_off += str8_deserial_read_struct(string, read_off, &event.string.size);
    event.string.str = push_array_no_zero(arena, U8, event.string.size);
    read_off += str8_deserial_read(string, read_off, event.string.str, event.string.size, 1);
  }
  return event;
}

////////////////////////////////
//~ rjf: Entity Type Functions

//- rjf: entity list data structures

internal void
ctrl_entity_list_push(Arena *arena, CTRL_EntityList *list, CTRL_Entity *entity)
{
  CTRL_EntityNode *n = push_array(arena, CTRL_EntityNode, 1);
  n->v = entity;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal CTRL_EntityList
ctrl_entity_list_from_handle_list(Arena *arena, CTRL_EntityStore *store, CTRL_HandleList *list)
{
  CTRL_EntityList result = {0};
  for(CTRL_HandleNode *n = list->first; n != 0; n = n->next)
  {
    CTRL_Entity *entity = ctrl_entity_from_handle(store, n->v);
    ctrl_entity_list_push(arena, &result, entity);
  }
  return result;
}

//- rjf: entity array data structure

internal CTRL_EntityArray
ctrl_entity_array_from_list(Arena *arena, CTRL_EntityList *list)
{
  CTRL_EntityArray result = {0};
  result.count = list->count;
  result.v = push_array_no_zero(arena, CTRL_Entity *, result.count);
  U64 idx = 0;
  for(CTRL_EntityNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->v;
  }
  return result;
}

//- rjf: cache creation/destruction

internal CTRL_EntityStore *
ctrl_entity_store_alloc(void)
{
  Arena *arena = arena_alloc();
  CTRL_EntityStore *store = push_array(arena, CTRL_EntityStore, 1);
  store->arena = arena;
  store->hash_slots_count = 1024;
  store->hash_slots = push_array(arena, CTRL_EntityHashSlot, store->hash_slots_count);
  for EachEnumVal(CTRL_EntityKind, k)
  {
    store->entity_kind_lists_arenas[k] = arena_alloc();
  }
  CTRL_Entity *root = store->root = ctrl_entity_alloc(store, &ctrl_entity_nil, CTRL_EntityKind_Root, Arch_Null, ctrl_handle_zero(), 0);
  CTRL_Entity *local_machine = ctrl_entity_alloc(store, root, CTRL_EntityKind_Machine, arch_from_context(), ctrl_handle_make(CTRL_MachineID_Local, dmn_handle_zero()), 0);
  Temp scratch = scratch_begin(0, 0);
  String8 local_machine_name = push_str8f(scratch.arena, "This PC (%S)", os_get_system_info()->machine_name);
  ctrl_entity_equip_string(store, local_machine, local_machine_name);
  scratch_end(scratch);
  return store;
}

internal void
ctrl_entity_store_release(CTRL_EntityStore *cache)
{
  arena_release(cache->arena);
}

//- rjf: string allocation/deletion

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
        if(n->size >= string.size)
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
    node->size = chunk_size;
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
ctrl_entity_alloc(CTRL_EntityStore *store, CTRL_Entity *parent, CTRL_EntityKind kind, Arch arch, CTRL_Handle handle, U64 id)
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
      entity->handle      = handle;
      entity->id          = id;
      entity->parent      = parent;
      entity->next = entity->prev = entity->first = entity->last = &ctrl_entity_nil;
      if(parent != &ctrl_entity_nil)
      {
        DLLPushBack_NPZ(&ctrl_entity_nil, parent->first, parent->last, entity, next, prev);
      }
    }
    
    // rjf: insert into hash map
    {
      U64 hash = ctrl_hash_from_handle(handle);
      U64 slot_idx = hash%store->hash_slots_count;
      CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
      CTRL_EntityHashNode *node = 0;
      for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->entity->handle, handle))
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
    
    // rjf: bump counters
    store->entity_kind_counts[kind] += 1;
    store->entity_kind_alloc_gens[kind] += 1;
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
        U64 hash = ctrl_hash_from_handle(t->e->handle);
        U64 slot_idx = hash%store->hash_slots_count;
        CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
        CTRL_EntityHashNode *node = 0;
        for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
        {
          if(ctrl_handle_match(n->entity->handle, t->e->handle))
          {
            DLLRemove(slot->first, slot->last, n);
            SLLStackPush(store->hash_node_free, n);
            break;
          }
        }
      }
      
      // rjf: dec counter
      store->entity_kind_counts[t->e->kind] -= 1;
      store->entity_kind_alloc_gens[t->e->kind] += 1;
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
ctrl_entity_from_handle(CTRL_EntityStore *store, CTRL_Handle handle)
{
  CTRL_Entity *entity = &ctrl_entity_nil;
  if(!ctrl_handle_match(handle, ctrl_handle_zero()))
  {
    U64 hash = ctrl_hash_from_handle(handle);
    U64 slot_idx = hash%store->hash_slots_count;
    CTRL_EntityHashSlot *slot = &store->hash_slots[slot_idx];
    CTRL_EntityHashNode *node = 0;
    for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->entity->handle, handle))
      {
        entity = n->entity;
        break;
      }
    }
  }
  return entity;
}

internal CTRL_Entity *
ctrl_entity_child_from_kind(CTRL_Entity *parent, CTRL_EntityKind kind)
{
  CTRL_Entity *result = &ctrl_entity_nil;
  for(CTRL_Entity *child = parent->first;
      child != &ctrl_entity_nil;
      child = child->next)
  {
    if(child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

internal CTRL_Entity *
ctrl_entity_ancestor_from_kind(CTRL_Entity *entity, CTRL_EntityKind kind)
{
  CTRL_Entity *result = &ctrl_entity_nil;
  for(CTRL_Entity *p = entity->parent; p != &ctrl_entity_nil; p = p->parent)
  {
    if(p->kind == kind)
    {
      result = p;
      break;
    }
  }
  return result;
}

internal CTRL_Entity *
ctrl_process_from_entity(CTRL_Entity *entity)
{
  CTRL_Entity *result = &ctrl_entity_nil;
  if(entity->kind == CTRL_EntityKind_Process)
  {
    result = entity;
  }
  else
  {
    result = ctrl_entity_ancestor_from_kind(entity, CTRL_EntityKind_Process);
  }
  return result;
}

internal CTRL_Entity *
ctrl_module_from_process_vaddr(CTRL_Entity *process, U64 vaddr)
{
  CTRL_Entity *result = &ctrl_entity_nil;
  for(CTRL_Entity *child = process->first;
      child != &ctrl_entity_nil;
      child = child->next)
  {
    if(child->kind == CTRL_EntityKind_Module && contains_1u64(child->vaddr_range, vaddr))
    {
      result = child;
      break;
    }
  }
  return result;
}

internal DI_Key
ctrl_dbgi_key_from_module(CTRL_Entity *module)
{
  CTRL_Entity *debug_info_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
  DI_Key dbgi_key = {debug_info_path->string, debug_info_path->timestamp};
  return dbgi_key;
}

internal CTRL_EntityList
ctrl_modules_from_dbgi_key(Arena *arena, CTRL_EntityStore *store, DI_Key *dbgi_key)
{
  CTRL_EntityList list = {0};
  CTRL_EntityList all_modules = ctrl_entity_list_from_kind(store, CTRL_EntityKind_Module);
  for(CTRL_EntityNode *n = all_modules.first; n != 0; n = n->next)
  {
    CTRL_Entity *module = n->v;
    DI_Key module_dbgi_key = ctrl_dbgi_key_from_module(module);
    if(di_key_match(&module_dbgi_key, dbgi_key))
    {
      ctrl_entity_list_push(arena, &list, module);
    }
  }
  return list;
}

internal CTRL_Entity *
ctrl_module_from_thread_candidates(CTRL_EntityStore *store, CTRL_Entity *thread, CTRL_EntityList *candidates)
{
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  U64 thread_rip_vaddr = ctrl_query_cached_rip_from_thread(store, thread->handle);
  CTRL_Entity *src_module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
  CTRL_Entity *module = &ctrl_entity_nil;
  for(CTRL_EntityNode *n = candidates->first; n != 0; n = n->next)
  {
    CTRL_Entity *candidate_module = n->v;
    CTRL_Entity *candidate_process = ctrl_entity_ancestor_from_kind(candidate_module, CTRL_EntityKind_Process);
    if(candidate_process == process)
    {
      module = candidate_module;
    }
    if(candidate_module == src_module)
    {
      break;
    }
  }
  return module;
}

internal CTRL_EntityList
ctrl_entity_list_from_kind(CTRL_EntityStore *store, CTRL_EntityKind kind)
{
  if(store->entity_kind_lists_gens[kind] != store->entity_kind_alloc_gens[kind])
  {
    arena_clear(store->entity_kind_lists_arenas[kind]);
    MemoryZeroStruct(&store->entity_kind_lists[kind]);
    for(CTRL_Entity *e = store->root;
        e != &ctrl_entity_nil;
        e = ctrl_entity_rec_depth_first_pre(e, store->root).next)
    {
      if(e->kind == kind)
      {
        ctrl_entity_list_push(store->entity_kind_lists_arenas[kind], &store->entity_kind_lists[kind], e);
      }
    }
    store->entity_kind_lists_gens[kind] = store->entity_kind_alloc_gens[kind];
  }
  return store->entity_kind_lists[kind];
}

internal U64
ctrl_vaddr_from_voff(CTRL_Entity *module, U64 voff)
{
  U64 result = voff + module->vaddr_range.min;
  return result;
}

internal U64
ctrl_voff_from_vaddr(CTRL_Entity *module, U64 vaddr)
{
  U64 result = vaddr - module->vaddr_range.min;
  return result;
}

internal Rng1U64
ctrl_vaddr_range_from_voff_range(CTRL_Entity *module, Rng1U64 voff_range)
{
  U64 dim = dim_1u64(voff_range);
  U64 min = ctrl_vaddr_from_voff(module, voff_range.min);
  Rng1U64 result = {min, min+dim};
  return result;
}

internal Rng1U64
ctrl_voff_range_from_vaddr_range(CTRL_Entity *module, Rng1U64 vaddr_range)
{
  U64 dim = dim_1u64(vaddr_range);
  U64 min = ctrl_voff_from_vaddr(module, vaddr_range.min);
  Rng1U64 result = {min, min+dim};
  return result;
}

internal B32
ctrl_entity_tree_is_frozen(CTRL_Entity *root)
{
  B32 is_frozen = 1;
  for(CTRL_Entity *e = root; e != &ctrl_entity_nil; e = ctrl_entity_rec_depth_first_pre(e, root).next)
  {
    if(e->kind == CTRL_EntityKind_Thread && !e->is_frozen)
    {
      is_frozen = 0;
      break;
    }
  }
  return is_frozen;
}

//- rjf: entity tree iteration

internal CTRL_EntityRec
ctrl_entity_rec_depth_first(CTRL_Entity *entity, CTRL_Entity *subtree_root, U64 sib_off, U64 child_off)
{
  CTRL_EntityRec result = {0};
  result.next = &ctrl_entity_nil;
  if((*MemberFromOffset(CTRL_Entity **, entity, child_off)) != &ctrl_entity_nil)
  {
    result.next = *MemberFromOffset(CTRL_Entity **, entity, child_off);
    result.push_count = 1;
  }
  else for(CTRL_Entity *parent = entity; parent != subtree_root && parent != &ctrl_entity_nil; parent = parent->parent)
  {
    if(parent != subtree_root && (*MemberFromOffset(CTRL_Entity **, parent, sib_off)) != &ctrl_entity_nil)
    {
      result.next = *MemberFromOffset(CTRL_Entity **, parent, sib_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

//- rjf: applying events to entity caches

internal void
ctrl_entity_store_apply_events(CTRL_EntityStore *store, CTRL_EventList *list)
{
  //- rjf: scan events & construct entities
  for(CTRL_EventNode *n = list->first; n != 0; n = n->next)
  {
    CTRL_Event *event = &n->v;
    switch(event->kind)
    {
      default:{}break;
      
      //- rjf: processes
      case CTRL_EventKind_NewProc:
      {
        CTRL_Entity *machine = ctrl_entity_from_handle(store, ctrl_handle_make(event->entity.machine_id, dmn_handle_zero()));
        CTRL_Entity *process = ctrl_entity_alloc(store, machine, CTRL_EntityKind_Process, event->arch, event->entity, (U64)event->entity_id);
      }break;
      case CTRL_EventKind_EndProc:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(store, event->entity);
        ctrl_entity_release(store, process);
        for(CTRL_Entity *entry = store->root->first, *next = &ctrl_entity_nil;
            entry != &ctrl_entity_nil;
            entry = next)
        {
          next = entry->next;
          if(entry->kind == CTRL_EntityKind_EntryPoint && entry->id == process->id)
          {
            ctrl_entity_release(store, entry);
          }
        }
      }break;
      
      //- rjf: threads
      case CTRL_EventKind_NewThread:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(store, event->parent);
        CTRL_Entity *thread = ctrl_entity_alloc(store, process, CTRL_EntityKind_Thread, event->arch, event->entity, (U64)event->entity_id);
        CTRL_Entity *first_thread = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Thread);
        if(first_thread == thread)
        {
          ctrl_entity_equip_string(store, thread, str8_lit("main_thread"));
        }
        CTRL_EntityList pending_thread_names = ctrl_entity_list_from_kind(store, CTRL_EntityKind_PendingThreadName);
        for(CTRL_EntityNode *n = pending_thread_names.first; n != 0; n = n->next)
        {
          if(n->v->id == event->entity_id)
          {
            ctrl_entity_equip_string(store, thread, n->v->string);
            ctrl_entity_release(store, n->v);
            break;
          }
        }
        thread->stack_base = event->stack_base;
        ctrl_query_cached_rip_from_thread(store, event->entity);
      }break;
      case CTRL_EventKind_EndThread:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(store, event->entity);
        ctrl_entity_release(store, thread);
      }break;
      case CTRL_EventKind_ThreadName:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(store, event->parent);
        CTRL_Entity *thread = ctrl_entity_from_handle(store, event->entity);
        if(thread != &ctrl_entity_nil)
        {
          ctrl_entity_equip_string(store, thread, event->string);
        }
        else
        {
          CTRL_Entity *pending_name = ctrl_entity_alloc(store, process, CTRL_EntityKind_PendingThreadName, Arch_Null, ctrl_handle_zero(), event->entity_id);
          ctrl_entity_equip_string(store, pending_name, event->string);
        }
      }break;
      case CTRL_EventKind_ThreadColor:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(store, event->entity);
        thread->rgba = event->rgba;
      }break;
      case CTRL_EventKind_ThreadFrozen:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(store, event->entity);
        thread->is_frozen = 1;
      }break;
      case CTRL_EventKind_ThreadThawed:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(store, event->entity);
        thread->is_frozen = 0;
      }break;
      
      //- rjf: modules
      case CTRL_EventKind_NewModule:
      {
        Temp scratch = scratch_begin(0, 0);
        CTRL_Entity *process = ctrl_entity_from_handle(store, event->parent);
        CTRL_Entity *module = ctrl_entity_alloc(store, process, CTRL_EntityKind_Module, event->arch, event->entity, event->vaddr_rng.min);
        ctrl_entity_equip_string(store, module, event->string);
        module->timestamp = event->timestamp;
        module->vaddr_range = event->vaddr_rng;
        CTRL_Entity *first_module = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Module);
        if(first_module == module)
        {
          ctrl_entity_equip_string(store, process, str8_skip_last_slash(event->string));
        }
        scratch_end(scratch);
      }break;
      case CTRL_EventKind_EndModule:
      {
        CTRL_Entity *module = ctrl_entity_from_handle(store, event->entity);
        ctrl_entity_release(store, module);
      }break;
      case CTRL_EventKind_ModuleDebugInfoPathChange:
      {
        CTRL_Entity *module = ctrl_entity_from_handle(store, event->entity);
        CTRL_Entity *debug_info_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
        if(debug_info_path == &ctrl_entity_nil)
        {
          debug_info_path = ctrl_entity_alloc(store, module, CTRL_EntityKind_DebugInfoPath, Arch_Null, ctrl_handle_zero(), 0);
        }
        ctrl_entity_equip_string(store, debug_info_path, event->string);
        debug_info_path->timestamp = event->timestamp;
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
  for(Arch arch = (Arch)0; arch < Arch_COUNT; arch = (Arch)(arch+1))
  {
    String8 *reg_names = regs_reg_code_string_table_from_arch(arch);
    U64 reg_count = regs_reg_code_count_from_arch(arch);
    String8 *alias_names = regs_alias_code_string_table_from_arch(arch);
    U64 alias_count = regs_alias_code_count_from_arch(arch);
    ctrl_state->arch_string2reg_tables[arch] = e_string2num_map_make(ctrl_state->arena, 256);
    ctrl_state->arch_string2alias_tables[arch] = e_string2num_map_make(ctrl_state->arena, 256);
    for(U64 idx = 1; idx < reg_count; idx += 1)
    {
      e_string2num_map_insert(ctrl_state->arena, &ctrl_state->arch_string2reg_tables[arch], reg_names[idx], idx);
    }
    for(U64 idx = 1; idx < alias_count; idx += 1)
    {
      e_string2num_map_insert(ctrl_state->arena, &ctrl_state->arch_string2alias_tables[arch], alias_names[idx], idx);
    }
  }
  ctrl_state->process_memory_cache.slots_count = 256;
  ctrl_state->process_memory_cache.slots = push_array(arena, CTRL_ProcessMemoryCacheSlot, ctrl_state->process_memory_cache.slots_count);
  ctrl_state->process_memory_cache.stripes_count = os_get_system_info()->logical_processor_count;
  ctrl_state->process_memory_cache.stripes = push_array(arena, CTRL_ProcessMemoryCacheStripe, ctrl_state->process_memory_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->process_memory_cache.stripes_count; idx += 1)
  {
    ctrl_state->process_memory_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
    ctrl_state->process_memory_cache.stripes[idx].cv = os_condition_variable_alloc();
  }
  ctrl_state->thread_reg_cache.slots_count = 1024;
  ctrl_state->thread_reg_cache.slots = push_array(arena, CTRL_ThreadRegCacheSlot, ctrl_state->thread_reg_cache.slots_count);
  ctrl_state->thread_reg_cache.stripes_count = os_get_system_info()->logical_processor_count;
  ctrl_state->thread_reg_cache.stripes = push_array(arena, CTRL_ThreadRegCacheStripe, ctrl_state->thread_reg_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->thread_reg_cache.stripes_count; idx += 1)
  {
    ctrl_state->thread_reg_cache.stripes[idx].arena = arena_alloc();
    ctrl_state->thread_reg_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  ctrl_state->module_image_info_cache.slots_count = 1024;
  ctrl_state->module_image_info_cache.slots = push_array(arena, CTRL_ModuleImageInfoCacheSlot, ctrl_state->module_image_info_cache.slots_count);
  ctrl_state->module_image_info_cache.stripes_count = os_get_system_info()->logical_processor_count;
  ctrl_state->module_image_info_cache.stripes = push_array(arena, CTRL_ModuleImageInfoCacheStripe, ctrl_state->module_image_info_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->module_image_info_cache.stripes_count; idx += 1)
  {
    ctrl_state->module_image_info_cache.stripes[idx].arena = arena_alloc();
    ctrl_state->module_image_info_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  ctrl_state->u2c_ring_size = KB(64);
  ctrl_state->u2c_ring_base = push_array_no_zero(arena, U8, ctrl_state->u2c_ring_size);
  ctrl_state->u2c_ring_mutex = os_mutex_alloc();
  ctrl_state->u2c_ring_cv = os_condition_variable_alloc();
  ctrl_state->c2u_ring_size = KB(64);
  ctrl_state->c2u_ring_max_string_size = ctrl_state->c2u_ring_size/2;
  ctrl_state->c2u_ring_base = push_array_no_zero(arena, U8, ctrl_state->c2u_ring_size);
  ctrl_state->c2u_ring_mutex = os_mutex_alloc();
  ctrl_state->c2u_ring_cv = os_condition_variable_alloc();
  {
    Temp scratch = scratch_begin(0, 0);
    String8 user_program_data_path = os_get_process_info()->user_program_data_path;
    String8 user_data_folder = push_str8f(scratch.arena, "%S/raddbg/logs", user_program_data_path);
    os_make_directory(user_data_folder);
    ctrl_state->ctrl_thread_log_path = push_str8f(ctrl_state->arena, "%S/ctrl_thread.raddbg_log", user_data_folder);
    os_write_data_to_file_path(ctrl_state->ctrl_thread_log_path, str8_zero());
    scratch_end(scratch);
  }
  ctrl_state->ctrl_thread_entity_store = ctrl_entity_store_alloc();
  ctrl_state->dmn_event_arena = arena_alloc();
  ctrl_state->user_entry_point_arena = arena_alloc();
  ctrl_state->user_meta_eval_arena = arena_alloc();
  ctrl_state->dbg_dir_arena = arena_alloc();
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
  ctrl_state->ctrl_thread_log = log_alloc();
  ctrl_state->ctrl_thread = os_thread_launch(ctrl_thread__entry_point, 0, 0);
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
ctrl_calc_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated)
{
  U64 key_hash_data[] =
  {
    (U64)process.machine_id,
    (U64)process.dmn_handle.u64[0],
    range.min,
    range.max,
    (U64)zero_terminated,
  };
  U128 key = hs_hash_from_data(str8((U8*)key_hash_data, sizeof(key_hash_data)));
  return key;
}

internal U128
ctrl_stored_hash_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated, B32 *out_is_stale, U64 endt_us)
{
  ProfBeginFunction();
  U128 result = {0};
  U64 size = dim_1u64(range);
  U64 pre_mem_gen = dmn_mem_gen();
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
    B32 is_stale = 1;
    OS_MutexScopeR(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->handle, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
            {
              result = range_n->hash;
              is_good = 1;
              is_stale = (range_n->mem_gen != pre_mem_gen);
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
          if(ctrl_handle_match(n->handle, process))
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
          node->handle = process;
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
          if(ctrl_handle_match(n->handle, process))
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
    if((!is_good || is_stale) && os_now_microseconds() >= last_time_requested_us+100000)
    {
      if(ctrl_u2ms_enqueue_req(process, range, zero_terminated, endt_us))
      {
        OS_MutexScopeW(process_stripe->rw_mutex)
        {
          for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
          {
            if(ctrl_handle_match(n->handle, process))
            {
              U64 range_slot_idx = range_hash%n->range_hash_slots_count;
              CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
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
        async_push_work(ctrl_mem_stream_work);
      }
    }
    
    //- rjf: out of time? -> exit
    if(os_now_microseconds() >= endt_us)
    {
      if(is_stale && out_is_stale)
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
  U64 post_mem_gen = dmn_mem_gen();
  if(post_mem_gen != pre_mem_gen && out_is_stale)
  {
    out_is_stale[0] = 1;
  }
  ProfEnd();
  return result;
}

//- rjf: bundled key/stream helper

internal U128
ctrl_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated)
{
  U128 key = ctrl_calc_hash_store_key_from_process_vaddr_range(process, range, zero_terminated);
  ctrl_stored_hash_from_process_vaddr_range(process, range, zero_terminated, 0, 0);
  return key;
}

//- rjf: process memory cache reading helpers

internal CTRL_ProcessMemorySlice
ctrl_query_cached_data_from_process_vaddr_range(Arena *arena, CTRL_Handle process, Rng1U64 range, U64 endt_us)
{
  ProfBeginFunction();
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
    ProfScope("gather hashes & last-hashes for each page")
    {
      for(U64 page_idx = 0; page_idx < page_count; page_idx += 1)
      {
        U64 page_base_vaddr = page_range.min + page_idx*page_size;
        U128 page_key = ctrl_calc_hash_store_key_from_process_vaddr_range(process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0);
        B32 page_is_stale = 0;
        U128 page_hash = ctrl_stored_hash_from_process_vaddr_range(process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0, &page_is_stale, endt_us);
        U128 page_last_hash = hs_hash_from_key(page_key, 1);
        result.stale = (result.stale || page_is_stale);
        page_hashes[page_idx] = page_hash;
        page_last_hashes[page_idx] = page_last_hash;
      }
    }
    
    //- rjf: setup output buffers
    void *read_out = push_array(arena, U8, dim_1u64(range));
    U64 *byte_bad_flags = push_array(arena, U64, (dim_1u64(range)+63)/64);
    U64 *byte_changed_flags = push_array(arena, U64, (dim_1u64(range)+63)/64);
    
    //- rjf: iterate pages, fill output
    ProfScope("iterate pages, fill output")
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
        if(data.size < page_size) ProfScope("mark missing bytes as bad")
        {
          Rng1U64 invalid_range = r1u64(data_vaddr_range.min+data.size, data_vaddr_range.min + page_size);
          Rng1U64 in_range_invalid_range = intersect_1u64(invalid_range, range);
          for(U64 invalid_vaddr = in_range_invalid_range.min;
              invalid_vaddr < in_range_invalid_range.max;
              invalid_vaddr += 1)
          {
            U64 idx_in_range = invalid_vaddr - range.min;
            byte_bad_flags[idx_in_range/64] |= (1ull<<(idx_in_range%64));
          }
        }
        
        // rjf: if this page's hash & last_hash don't match, diff each byte &
        // fill out changed flags
        if(!u128_match(page_hashes[page_idx], page_last_hashes[page_idx])) ProfScope("hashes don't match; diff each byte")
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
        U64 bytes_to_skip = page_size;
        if(page_idx == 0 && range.min > data_vaddr_range.min)
        {
          bytes_to_skip -= (range.min-data_vaddr_range.min);
        }
        write_off += bytes_to_skip;
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
  ProfEnd();
  return result;
}

internal CTRL_ProcessMemorySlice
ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(Arena *arena, CTRL_Handle process, U64 vaddr, U64 limit, U64 element_size, U64 endt_us)
{
  CTRL_ProcessMemorySlice result = ctrl_query_cached_data_from_process_vaddr_range(arena, process, r1u64(vaddr, vaddr+limit), endt_us);
  U64 element_count = result.data.size/element_size;
  for(U64 element_idx = 0; element_idx < element_count; element_idx += 1)
  {
    B32 element_is_zero = 1;
    for(U64 element_byte_idx = 0; element_byte_idx < element_size; element_byte_idx += 1)
    {
      if(result.data.str[element_idx*element_size + element_byte_idx] != 0)
      {
        element_is_zero = 0;
        break;
      }
    }
    if(element_is_zero)
    {
      result.data.size = element_idx*element_size;
      break;
    }
  }
  return result;
}

internal B32
ctrl_read_cached_process_memory(CTRL_Handle process, Rng1U64 range, B32 *is_stale_out, void *out, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  U64 needed_size = dim_1u64(range);
  CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process, range, endt_us);
  B32 good = (slice.data.size >= needed_size && !slice.any_byte_bad);
  if(good)
  {
    MemoryCopy(out, slice.data.str, needed_size);
  }
  if(slice.stale && is_stale_out)
  {
    *is_stale_out = 1;
  }
  scratch_end(scratch);
  return good;
}

//- rjf: process memory writing

internal B32
ctrl_process_write(CTRL_Handle process, Rng1U64 range, void *src)
{
  ProfBeginFunction();
  B32 result = dmn_process_write(process.dmn_handle, range, src);
  
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
      CTRL_Handle process;
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
                task->process = proc_n->handle;
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
      ctrl_query_cached_data_from_process_vaddr_range(temp.arena, task->process, task->range, endt_us);
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
ctrl_query_cached_reg_block_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_Handle handle)
{
  CTRL_ThreadRegCache *cache = &ctrl_state->thread_reg_cache;
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(store, handle);
  Arch arch = thread_entity->arch;
  U64 reg_block_size = regs_block_size_from_arch(arch);
  U64 hash = ctrl_hash_from_handle(handle);
  U64 slot_idx = hash%cache->slots_count;
  U64 stripe_idx = slot_idx%cache->stripes_count;
  CTRL_ThreadRegCacheSlot *slot = &cache->slots[slot_idx];
  CTRL_ThreadRegCacheStripe *stripe = &cache->stripes[stripe_idx];
  void *result = push_array(arena, U8, reg_block_size);
  OS_MutexScopeW(stripe->rw_mutex)
  {
    // rjf: find existing node
    CTRL_ThreadRegCacheNode *node = 0;
    for(CTRL_ThreadRegCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->handle, handle))
      {
        node = n;
        break;
      }
    }
    
    // rjf: allocate existing node
    if(!node)
    {
      node = push_array(stripe->arena, CTRL_ThreadRegCacheNode, 1);
      DLLPushBack(slot->first, slot->last, node);
      node->handle     = handle;
      node->block_size = reg_block_size;
      node->block      = push_array(stripe->arena, U8, reg_block_size);
    }
    
    // rjf: copy from node
    if(node)
    {
      U64 current_reg_gen = dmn_reg_gen();
      B32 need_stale = 1;
      if(node->reg_gen != current_reg_gen && dmn_thread_read_reg_block(handle.dmn_handle, result))
      {
        if(node != 0)
        {
          need_stale = 0;
          node->reg_gen = current_reg_gen;
          MemoryCopy(node->block, result, reg_block_size);
        }
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
ctrl_query_cached_tls_root_vaddr_from_thread(CTRL_EntityStore *store, CTRL_Handle handle)
{
  U64 result = dmn_tls_root_vaddr_from_thread(handle.dmn_handle);
  return result;
}

internal U64
ctrl_query_cached_rip_from_thread(CTRL_EntityStore *store, CTRL_Handle handle)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(store, handle);
  Arch arch = thread_entity->arch;
  void *block = ctrl_query_cached_reg_block_from_thread(scratch.arena, store, handle);
  U64 result = regs_rip_from_arch_block(arch, block);
  scratch_end(scratch);
  return result;
}

internal U64
ctrl_query_cached_rsp_from_thread(CTRL_EntityStore *store, CTRL_Handle handle)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(store, handle);
  Arch arch = thread_entity->arch;
  void *block = ctrl_query_cached_reg_block_from_thread(scratch.arena, store, handle);
  U64 result = regs_rsp_from_arch_block(arch, block);
  scratch_end(scratch);
  return result;
}

//- rjf: thread register writing

internal B32
ctrl_thread_write_reg_block(CTRL_Handle thread, void *block)
{
  B32 good = dmn_thread_write_reg_block(thread.dmn_handle, block);
  return good;
}

////////////////////////////////
//~ rjf: Module Image Info Functions

//- rjf: cache lookups

internal PE_IntelPdata *
ctrl_intel_pdata_from_module_voff(Arena *arena, CTRL_Handle module_handle, U64 voff)
{
  PE_IntelPdata *first_pdata = 0;
  {
    U64 hash = ctrl_hash_from_handle(module_handle);
    U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
    U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
    CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
    CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex) for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->module, module_handle))
      {
        PE_IntelPdata *pdatas = n->intel_pdatas;
        U64 pdatas_count = n->intel_pdatas_count;
        if(n->intel_pdatas_count != 0 && voff >= n->intel_pdatas[0].voff_first)
        {
          // NOTE(rjf):
          //
          // binary search:
          //  find max index s.t. pdata_array[index].voff_first <= voff
          //  we assume (i < j) -> (pdata_array[i].voff_first < pdata_array[j].voff_first)
          U64 index = pdatas_count;
          U64 min = 0;
          U64 opl = pdatas_count;
          for(;;)
          {
            U64 mid = (min + opl)/2;
            PE_IntelPdata *pdata = pdatas + mid;
            if(voff < pdata->voff_first)
            {
              opl = mid;
            }
            else if(pdata->voff_first < voff)
            {
              min = mid;
            }
            else
            {
              index = mid;
              break;
            }
            if(min + 1 >= opl)
            {
              index = min;
              break;
            }
          }
          
          // rjf: if we are in range fill result
          {
            PE_IntelPdata *pdata = pdatas + index;
            if(pdata->voff_first <= voff && voff < pdata->voff_one_past_last)
            {
              first_pdata = push_array(arena, PE_IntelPdata, 1);
              MemoryCopyStruct(first_pdata, pdata);
            }
          }
        }
        break;
      }
    }
  }
  return first_pdata;
}

internal U64
ctrl_entry_point_voff_from_module(CTRL_Handle module_handle)
{
  U64 result = 0;
  U64 hash = ctrl_hash_from_handle(module_handle);
  U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
  U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
  CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
  CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex) for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(ctrl_handle_match(n->module, module_handle))
    {
      result = n->entry_point_voff;
      break;
    }
  }
  return result;
}

internal Rng1U64
ctrl_tls_vaddr_range_from_module(CTRL_Handle module_handle)
{
  Rng1U64 result = {0};
  U64 hash = ctrl_hash_from_handle(module_handle);
  U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
  U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
  CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
  CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex) for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(ctrl_handle_match(n->module, module_handle))
    {
      result = n->tls_vaddr_range;
      break;
    }
  }
  return result;
}

internal String8
ctrl_initial_debug_info_path_from_module(Arena *arena, CTRL_Handle module_handle)
{
  String8 result = {0};
  U64 hash = ctrl_hash_from_handle(module_handle);
  U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
  U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
  CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
  CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex) for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(ctrl_handle_match(n->module, module_handle))
    {
      result = push_str8_copy(arena, n->initial_debug_info_path);
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Unwinding Functions

//- rjf: unwind deep copier

internal CTRL_Unwind
ctrl_unwind_deep_copy(Arena *arena, Arch arch, CTRL_Unwind *src)
{
  CTRL_Unwind dst = {0};
  {
    dst.flags = src->flags;
    dst.frames.count = src->frames.count;
    dst.frames.v = push_array(arena, CTRL_UnwindFrame, dst.frames.count);
    MemoryCopy(dst.frames.v, src->frames.v, sizeof(dst.frames.v[0])*dst.frames.count);
    U64 block_size = regs_block_size_from_arch(arch);
    for(U64 idx = 0; idx < dst.frames.count; idx += 1)
    {
      dst.frames.v[idx].regs = push_array_no_zero(arena, U8, block_size);
      MemoryCopy(dst.frames.v[idx].regs, src->frames.v[idx].regs, block_size);
    }
  }
  return dst;
}

//- rjf: [x64]

internal REGS_Reg64 *
ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 gpr_reg)
{
  local_persist REGS_Reg64 dummy = {0};
  REGS_Reg64 *result = &dummy;
  switch(gpr_reg)
  {
    case PE_UnwindGprRegX64_RAX:{result = &regs->rax;}break;
    case PE_UnwindGprRegX64_RCX:{result = &regs->rcx;}break;
    case PE_UnwindGprRegX64_RDX:{result = &regs->rdx;}break;
    case PE_UnwindGprRegX64_RBX:{result = &regs->rbx;}break;
    case PE_UnwindGprRegX64_RSP:{result = &regs->rsp;}break;
    case PE_UnwindGprRegX64_RBP:{result = &regs->rbp;}break;
    case PE_UnwindGprRegX64_RSI:{result = &regs->rsi;}break;
    case PE_UnwindGprRegX64_RDI:{result = &regs->rdi;}break;
    case PE_UnwindGprRegX64_R8 :{result = &regs->r8 ;}break;
    case PE_UnwindGprRegX64_R9 :{result = &regs->r9 ;}break;
    case PE_UnwindGprRegX64_R10:{result = &regs->r10;}break;
    case PE_UnwindGprRegX64_R11:{result = &regs->r11;}break;
    case PE_UnwindGprRegX64_R12:{result = &regs->r12;}break;
    case PE_UnwindGprRegX64_R13:{result = &regs->r13;}break;
    case PE_UnwindGprRegX64_R14:{result = &regs->r14;}break;
    case PE_UnwindGprRegX64_R15:{result = &regs->r15;}break;
  }
  return result;
}

internal CTRL_UnwindStepResult
ctrl_unwind_step__pe_x64(CTRL_EntityStore *store, CTRL_Handle process_handle, CTRL_Handle module_handle, REGS_RegBlockX64 *regs, U64 endt_us)
{
  B32 is_stale = 0;
  B32 is_good = 1;
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: unpack parameters
  //
  CTRL_Entity *module = ctrl_entity_from_handle(store, module_handle);
  CTRL_Entity *process = ctrl_entity_from_handle(store, process_handle);
  U64 rip_voff = regs->rip.u64 - module->vaddr_range.min;
  
  //////////////////////////////
  //- rjf: rip_voff -> first pdata
  //
  PE_IntelPdata *first_pdata = ctrl_intel_pdata_from_module_voff(scratch.arena, module_handle, rip_voff);
  
  //////////////////////////////
  //- rjf: pdata -> detect if in epilog
  //
  B32 has_pdata_and_in_epilog = 0;
  if(first_pdata) ProfScope("pdata -> detect if in epilog")
  {
    // NOTE(allen): There are restrictions placed on how an epilog is allowed
    // to be formed (https://docs.microsoft.com/en-us/cpp/build/prolog-and-epilog?view=msvc-160)
    // Here we interpret machine code directly according to the rules
    // given there to determine if the code we're looking at looks like an epilog.
    
    //- rjf: set up parsing state
    B32 is_epilog = 0;
    B32 keep_parsing = 1;
    U64 read_vaddr = regs->rip.u64;
    U64 read_vaddr_opl = read_vaddr + 256;
    
    //- rjf: check first instruction
    {
      B32 inst_good = 0;
      U8 inst[4] = {0};
      if(read_vaddr + sizeof(inst) <= read_vaddr_opl)
      {
        inst_good = ctrl_read_cached_process_memory(process_handle, r1u64(read_vaddr, read_vaddr+sizeof(inst)), &is_stale, inst, endt_us);
        inst_good = inst_good && !is_stale;
      }
      if(!inst_good)
      {
        keep_parsing = 0;
      }
      else if((inst[0] & 0xF8) == 0x48)
      {
        switch(inst[1])
        {
          // rjf: add $nnnn,%rsp
          case 0x81:
          {
            if(inst[0] == 0x48 && inst[2] == 0xC4)
            {
              read_vaddr += 7;
            }
            else
            {
              keep_parsing = 0;
            }
          }break;
          
          // rjf: add $n,%rsp
          case 0x83:
          {
            if(inst[0] == 0x48 && inst[2] == 0xC4)
            {
              read_vaddr += 4;
            }
            else
            {
              keep_parsing = 0;
            }
          }break;
          
          // rjf: lea n(reg),%rsp
          case 0x8D:
          {
            if((inst[0] & 0x06) == 0 &&
               ((inst[2] >> 3) & 0x07) == 0x04 &&
               (inst[2] & 0x07) != 0x04)
            {
              U8 imm_size = (inst[2] >> 6);
              
              // rjf: 1-byte immediate
              if(imm_size == 1)
              {
                read_vaddr += 4;
              }
              
              // rjf: 4-byte immediate
              else if(imm_size == 2)
              {
                read_vaddr += 7;
              }
              
              // rjf: other case
              else
              {
                keep_parsing = 0;
              }
            }
            else
            {
              keep_parsing = 0;
            }
          }break;
        }
      }
    }
    
    //- rjf: continue parsing instructions
    for(;keep_parsing;)
    {
      // rjf: read next instruction byte
      B32 inst_byte_good = 0;
      U8 inst_byte = 0;
      if(read_vaddr + sizeof(inst_byte) <= read_vaddr_opl)
      {
        inst_byte_good = ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &inst_byte, endt_us);
      }
      if(!inst_byte_good || is_stale)
      {
        keep_parsing = 0;
      }
      
      // rjf: when (... I don't know ...) rely on the next byte
      B32 check_inst_byte_good = inst_byte_good;
      U64 check_vaddr = read_vaddr;
      U8 check_inst_byte = inst_byte;
      if(inst_byte_good && (inst_byte & 0xF0) == 0x40)
      {
        check_vaddr = read_vaddr + 1;
        if(read_vaddr + sizeof(check_inst_byte) <= read_vaddr_opl)
        {
          check_inst_byte_good = ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &check_inst_byte, endt_us);
        }
        if(!check_inst_byte_good || is_stale)
        {
          keep_parsing = 0;
        }
      }
      
      // rjf: check instruction byte
      if(check_inst_byte_good)
      {
        switch(check_inst_byte)
        {
          // rjf: pop
          case 0x58:case 0x59:case 0x5A:case 0x5B:
          case 0x5C:case 0x5D:case 0x5E:case 0x5F:
          {
            read_vaddr = check_vaddr + 1;
          }break;
          
          // rjf: ret
          case 0xC2:
          case 0xC3:
          { 
            is_epilog = 1;
            keep_parsing = 0;
          }break;
          
          // rjf: jmp nnnn
          case 0xE9:
          {
            U64 imm_vaddr = check_vaddr + 1;
            S32 imm = 0;
            B32 imm_good = 0;
            if(read_vaddr + sizeof(imm) <= read_vaddr_opl)
            {
              imm_good = ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm, endt_us);
            }
            if(!imm_good || is_stale)
            {
              keep_parsing = 0;
            }
            if(imm_good)
            {
              U64 next_vaddr = (U64)(imm_vaddr + sizeof(imm) + imm);
              U64 next_voff = next_vaddr - module->vaddr_range.min; // TODO(rjf): verify that this offset is from module base vaddr, not section
              if(!(first_pdata->voff_first <= next_voff && next_voff < first_pdata->voff_one_past_last))
              {
                keep_parsing = 0;
              }
              else
              {
                read_vaddr = next_vaddr;
              }
            }
            // TODO(allen): why isn't this just the end of the epilog?
          }break;
          
          // rjf: rep; ret (for amd64 prediction bug)
          case 0xF3:
          {
            U8 next_inst_byte = 0;
            B32 next_inst_byte_good = 0;
            if(read_vaddr + sizeof(next_inst_byte) <= read_vaddr_opl)
            {
              next_inst_byte_good = ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &next_inst_byte, endt_us);
            }
            if(next_inst_byte_good)
            {
              is_epilog = (next_inst_byte == 0xC3);
            }
            keep_parsing = 0;
          }break;
          
          default:{keep_parsing = 0;}break;
        }
      }
    }
    has_pdata_and_in_epilog = is_epilog;
  }
  
  //////////////////////////////
  //- rjf: pdata & in epilog -> epilog unwind
  //
  if(first_pdata && has_pdata_and_in_epilog) ProfScope("pdata & in epilog -> epilog unwind")
  {
    U64 read_vaddr = regs->rip.u64;
    for(B32 keep_parsing = 1;keep_parsing != 0;)
    {
      //- rjf: assume no more parsing after this instruction
      keep_parsing = 0;
      
      //- rjf: read next instruction byte
      U8 inst_byte = 0;
      is_good = is_good && ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &inst_byte, endt_us);
      is_good = is_good && !is_stale;
      read_vaddr += 1;
      
      //- rjf: extract rex from instruction byte
      U8 rex = 0;
      if((inst_byte & 0xF0) == 0x40)
      {
        rex = inst_byte & 0xF; // rex prefix
        is_good = is_good && ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &inst_byte, endt_us);
        is_good = is_good && !is_stale;
        read_vaddr += 1;
      }
      
      //- rjf: parse remainder of instruction
      switch(inst_byte)
      {
        // rjf: pop
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        {
          // rjf: read value at rsp
          U64 sp = regs->rsp.u64;
          U64 value = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, sp, &is_stale, &value, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          
          // rjf: modify registers
          PE_UnwindGprRegX64 gpr_reg = (inst_byte - 0x58) + (rex & 1)*8;
          REGS_Reg64 *reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, gpr_reg);
          reg->u64 = value;
          regs->rsp.u64 = sp + 8;
          
          // rjf: not a final instruction, so keep mparsing
          keep_parsing = 1;
        }break;
        
        // rjf: add $nnnn,%rsp 
        case 0x81:
        {
          // rjf: skip one byte (we already know what it is in this scenario)
          read_vaddr += 1;
          
          // rjf: read the 4-byte immediate
          S32 imm = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          read_vaddr += 4;
          
          // rjf: update stack pointer
          regs->rsp.u64 = (U64)(regs->rsp.u64 + imm);
          
          // rjf: not a final instruction; keep parsing
          keep_parsing = 1;
        }break;
        
        // rjf: add $n,%rsp
        case 0x83:
        {
          // rjf: skip one byte (we already know what it is in this scenario)
          read_vaddr += 1;
          
          // rjf: read the 4-byte immediate
          S8 imm = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          read_vaddr += 1;
          
          // rjf: update stack pointer
          regs->rsp.u64 = (U64)(regs->rsp.u64 + imm);
          
          // rjf: not a final instruction; keep parsing
          keep_parsing = 1;
        }break;
        
        // rjf: lea imm8/imm32,$rsp
        case 0x8D:
        {
          // rjf: read source register
          U8 modrm = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &modrm, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          read_vaddr += 1;
          PE_UnwindGprRegX64 gpr_reg = (modrm & 7) + (rex & 1)*8;
          REGS_Reg64 *reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, gpr_reg);
          U64 reg_value = reg->u64;
          
          // rjf: read immediate
          S32 imm = 0;
          {
            // rjf: read 1-byte immediate
            if((modrm >> 6) == 1)
            {
              S8 imm8 = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm8, endt_us) ||
                 is_stale)
              {
                is_good = 0;
                break;
              }
              read_vaddr += 1;
              imm = (S32)imm8;
            }
            
            // rjf: read 4-byte immediate
            else
            {
              if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm, endt_us) ||
                 is_stale)
              {
                is_good = 0;
                break;
              }
              read_vaddr += 4;
            }
          }
          
          // rjf: update stack pointer
          regs->rsp.u64 = (U64)(reg_value + imm);
          
          // rjf: not a final instruction; keep parsing
          keep_parsing = 1;
        }break;
        
        // rjf: ret $nn
        case 0xC2:
        {
          // rjf: read new ip
          U64 sp = regs->rsp.u64;
          U64 new_ip = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, sp, &is_stale, &new_ip, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          
          // rjf: read 2-byte immediate & advance stack pointer
          U16 imm = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, read_vaddr, &is_stale, &imm, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          U64 new_sp = sp + 8 + imm;
          
          // rjf: commit registers
          regs->rip.u64 = new_ip;
          regs->rsp.u64 = new_sp;
        }break;
        
        // rjf: ret / rep; ret
        case 0xF3:
        {
          // Assert(!"Hit me!");
        }break;
        case 0xC3:
        {
          // rjf: read new ip
          U64 sp = regs->rsp.u64;
          U64 new_ip = 0;
          if(!ctrl_read_cached_process_memory_struct(process->handle, sp, &is_stale, &new_ip, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          
          // rjf: advance stack pointer
          U64 new_sp = sp + 8;
          
          // rjf: commit registers
          regs->rip.u64 = new_ip;
          regs->rsp.u64 = new_sp;
        }break;
        
        // rjf: jmp nnnn
        case 0xE9:
        {
          // Assert(!"Hit Me");
          // TODO(allen): general idea: read the immediate, move the ip, leave the sp, done
          // we don't have any cases to exercise this right now. no guess implementation!
        }break;
        
        // rjf: Sjmp n
        case 0xEB:
        {
          // Assert(!"Hit Me");
          // TODO(allen): general idea: read the immediate, move the ip, leave the sp, done
          // we don't have any cases to exercise this right now. no guess implementation!
        }break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: pdata & not in epilog -> xdata unwind
  //
  B32 xdata_unwind_did_machframe = 0;
  if(first_pdata && !has_pdata_and_in_epilog) ProfScope("pdata & not in epilog -> xdata unwind")
  {
    //- rjf: get frame reg
    B32 bad_frame_reg_info = 0;
    REGS_Reg64 *frame_reg = 0;
    U64 frame_off = 0;
    {
      U64 unwind_info_off = first_pdata->voff_unwind_info;
      PE_UnwindInfoX64 unwind_info = {0};
      if(!ctrl_read_cached_process_memory_struct(process->handle, module->vaddr_range.min+unwind_info_off, &is_stale, &unwind_info, endt_us) ||
         is_stale)
      {
        is_good = 0;
      }
      U32 frame_reg_id = PE_UNWIND_INFO_REG_FROM_FRAME(unwind_info.frame);
      U64 frame_off_val = PE_UNWIND_INFO_OFF_FROM_FRAME(unwind_info.frame);
      if(frame_reg_id != 0)
      {
        frame_reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, frame_reg_id);
        bad_frame_reg_info = (frame_reg == 0); // NOTE(rjf): frame_reg should never be 0 at this point, in valid exe
      }
      frame_off = frame_off_val;
    }
    
    //- rjf: iterate pdatas, apply opcodes
    PE_IntelPdata *last_pdata = 0;
    PE_IntelPdata *pdata = first_pdata;
    if(!bad_frame_reg_info) for(B32 keep_parsing = 1; keep_parsing && pdata != last_pdata;)
    {
      //- rjf: unpack unwind info & codes
      B32 good_unwind_info = 1;
      U64 unwind_info_off = pdata->voff_unwind_info;
      PE_UnwindInfoX64 unwind_info = {0};
      good_unwind_info = good_unwind_info && ctrl_read_cached_process_memory_struct(process->handle, module->vaddr_range.min+unwind_info_off, &is_stale, &unwind_info, endt_us);
      PE_UnwindCodeX64 *unwind_codes = push_array(scratch.arena, PE_UnwindCodeX64, unwind_info.codes_num);
      good_unwind_info = good_unwind_info && ctrl_read_cached_process_memory(process->handle, r1u64(module->vaddr_range.min+unwind_info_off+sizeof(unwind_info),
                                                                                                    module->vaddr_range.min+unwind_info_off+sizeof(unwind_info)+sizeof(PE_UnwindCodeX64)*unwind_info.codes_num),
                                                                             &is_stale, unwind_codes, endt_us);
      good_unwind_info = good_unwind_info && !is_stale;
      
      //- rjf: bad unwind info -> abort
      if(!good_unwind_info)
      {
        is_good = 0;
        break;
      }
      
      //- rjf: unpack frame base
      U64 frame_base = regs->rsp.u64;
      if(frame_reg != 0)
      {
        U64 raw_frame_base = frame_reg->u64;
        U64 adjusted_frame_base = raw_frame_base - frame_off*16;
        frame_base = adjusted_frame_base;
      }
      
      //- rjf: apply opcodes
      PE_UnwindCodeX64 *code_ptr = unwind_codes;
      PE_UnwindCodeX64 *code_opl = unwind_codes + unwind_info.codes_num;
      for(PE_UnwindCodeX64 *next_code_ptr = 0; code_ptr < code_opl; code_ptr = next_code_ptr)
      {
        // rjf: unpack opcode info
        U32 op_code = PE_UNWIND_OPCODE_FROM_FLAGS(code_ptr->flags);
        U32 op_info = PE_UNWIND_INFO_FROM_FLAGS(code_ptr->flags);
        U32 slot_count = pe_slot_count_from_unwind_op_code__x64(op_code);
        if(op_code == PE_UnwindOpCodeX64_ALLOC_LARGE && op_info == 1)
        {
          slot_count += 1;
        }
        
        // rjf: detect bad slot counts
        if(slot_count == 0 || code_ptr+slot_count > code_opl)
        {
          keep_parsing = 0;
          is_good = 0;
          break;
        }
        
        // rjf: set next op code pointer
        next_code_ptr = code_ptr + slot_count;
        
        // rjf: interpret this op code
        U64 code_voff = pdata->voff_first + code_ptr->off_in_prolog;
        if(code_voff <= rip_voff)
        {
          switch(op_code)
          {
            case PE_UnwindOpCodeX64_PUSH_NONVOL:
            {
              // rjf: read value from stack pointer
              U64 rsp = regs->rsp.u64;
              U64 value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, rsp, &is_stale, &value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: advance stack ptr
              U64 new_rsp = rsp + 8;
              
              // rjf: commit registers
              REGS_Reg64 *reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, op_info);
              reg->u64 = value;
              regs->rsp.u64 = new_rsp;
            }break;
            
            case PE_UnwindOpCodeX64_ALLOC_LARGE:
            {
              // rjf: read alloc size
              U64 size = 0;
              if(op_info == 0)
              {
                size = code_ptr[1].u16*8;
              }
              else if(op_info == 1)
              {
                size = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
              }
              else
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: advance stack pointer
              U64 rsp = regs->rsp.u64;
              U64 new_rsp = rsp + size;
              
              // rjf: advance stack pointer
              regs->rsp.u64 = new_rsp;
            }break;
            
            case PE_UnwindOpCodeX64_ALLOC_SMALL:
            {
              // rjf: advance stack pointer
              regs->rsp.u64 += op_info*8 + 8;
            }break;
            
            case PE_UnwindOpCodeX64_SET_FPREG:
            {
              // rjf: put stack pointer back to the frame base
              regs->rsp.u64 = frame_base;
            }break;
            
            case PE_UnwindOpCodeX64_SAVE_NONVOL:
            {
              // rjf: read value from frame base
              U64 off = code_ptr[1].u16*8;
              U64 addr = frame_base + off;
              U64 value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, addr, &is_stale, &value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit to register
              REGS_Reg64 *reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, op_info);
              reg->u64 = value;
            }break;
            
            case PE_UnwindOpCodeX64_SAVE_NONVOL_FAR:
            {
              // rjf: read value from frame base
              U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
              U64 addr = frame_base + off;
              U64 value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, addr, &is_stale, &value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit to register
              REGS_Reg64 *reg = ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(regs, op_info);
              reg->u64 = value;
            }break;
            
            case PE_UnwindOpCodeX64_EPILOG:
            {
              keep_parsing = 1;
            }break;
            
            case PE_UnwindOpCodeX64_SPARE_CODE:
            {
              // TODO(rjf): ???
              keep_parsing = 0;
              is_good = 0;
            }break;
            
            case PE_UnwindOpCodeX64_SAVE_XMM128:
            {
              // rjf: read new register values
              U8 buf[16];
              U64 off = code_ptr[1].u16*16;
              U64 addr = frame_base + off;
              if(!ctrl_read_cached_process_memory(process->handle, r1u64(addr, addr+sizeof(buf)), &is_stale, buf, endt_us))
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit to register
              void *xmm_reg = (&regs->zmm0) + op_info;
              MemoryCopy(xmm_reg, buf, sizeof(buf));
            }break;
            
            case PE_UnwindOpCodeX64_SAVE_XMM128_FAR:
            {
              // rjf: read new register values
              U8 buf[16];
              U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
              U64 addr = frame_base + off;
              if(!ctrl_read_cached_process_memory(process->handle, r1u64(addr, addr+16), &is_stale, buf, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit to register
              void *xmm_reg = (&regs->zmm0) + op_info;
              MemoryCopy(xmm_reg, buf, sizeof(buf));
            }break;
            
            case PE_UnwindOpCodeX64_PUSH_MACHFRAME:
            {
              // NOTE(rjf): this was found by stepping through kernel code after an exception was
              // thrown, encountered in the exception_stepping_tests (after the throw) in mule_main
              if(op_info > 1)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: read values
              U64 sp_og = regs->rsp.u64;
              U64 sp_adj = sp_og;
              if(op_info == 1)
              {
                sp_adj += 8;
              }
              U64 ip_value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, sp_adj, &is_stale, &ip_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_ip = sp_adj + 8;
              U16 ss_value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, sp_after_ip, &is_stale, &ss_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_ss = sp_after_ip + 8;
              U64 rflags_value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, sp_after_ss, &is_stale, &rflags_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_rflags = sp_after_ss + 8;
              U64 sp_value = 0;
              if(!ctrl_read_cached_process_memory_struct(process->handle, sp_after_rflags, &is_stale, &sp_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit registers
              regs->rip.u64 = ip_value;
              regs->ss.u16 = ss_value;
              regs->rflags.u64 = rflags_value;
              regs->rsp.u64 = sp_value;
              
              // rjf: mark machine frame
              xdata_unwind_did_machframe = 1;
            }break;
          }
        }
      }
      
      //- rjf: iterate to next pdata
      if(keep_parsing)
      {
        U32 flags = PE_UNWIND_INFO_FLAGS_FROM_HDR(unwind_info.header);
        if(!(flags & PE_UnwindInfoX64Flag_CHAINED))
        {
          break;
        }
        U64 code_count_rounded = AlignPow2(unwind_info.codes_num, sizeof(PE_UnwindCodeX64));
        U64 code_size = code_count_rounded*sizeof(PE_UnwindCodeX64);
        U64 chained_pdata_off = unwind_info_off + sizeof(PE_UnwindInfoX64) + code_size;
        last_pdata = pdata;
        pdata = push_array(scratch.arena, PE_IntelPdata, 1);
        if(!ctrl_read_cached_process_memory_struct(process->handle, module->vaddr_range.min+chained_pdata_off, &is_stale, pdata, endt_us) ||
           is_stale)
        {
          is_good = 0;
          break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: no pdata, or didn't do machframe in xdata unwind -> unwind by reading stack pointer
  //
  if(!first_pdata || (!has_pdata_and_in_epilog && !xdata_unwind_did_machframe)) ProfScope("no pdata, or didn't do machframe in xdata unwind -> unwind by reading stack pointer")
  {
    // rjf: read rip from stack pointer
    U64 rsp = regs->rsp.u64;
    U64 new_rip = 0;
    if(!ctrl_read_cached_process_memory_struct(process->handle, rsp, &is_stale, &new_rip, endt_us) ||
       is_stale)
    {
      is_good = 0;
    }
    
    // rjf: commit registers
    if(is_good)
    {
      U64 new_rsp = rsp + 8;
      regs->rip.u64 = new_rip;
      regs->rsp.u64 = new_rsp;
    }
  }
  
  //////////////////////////////
  //- rjf: fill & return
  //
  scratch_end(scratch);
  CTRL_UnwindStepResult result = {0};
  if(!is_good) {result.flags |= CTRL_UnwindFlag_Error;}
  if(is_stale) {result.flags |= CTRL_UnwindFlag_Stale;}
  return result;
}

internal PE_Arm64Pdata *
ctrl_arm64_pdata_from_module_voff(Arena *arena, CTRL_Handle module_handle, U64 voff)
{
  PE_Arm64Pdata *first_pdata = 0;
  {
    U64 hash = ctrl_hash_from_handle(module_handle);
    U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
    U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
    CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
    CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex) for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->module, module_handle))
      {
        PE_Arm64Pdata *pdatas = n->arm64_pdatas;
        U64 pdatas_count = n->arm64_pdatas_count;

        if(n->arm64_pdatas_count != 0 && voff >= n->arm64_pdatas[0].voff_first)
        {
          // NOTE(rjf):
          //
          // binary search:
          //  find max index s.t. pdata_array[index].voff_first <= voff
          //  we assume (i < j) -> (pdata_array[i].voff_first < pdata_array[j].voff_first)
          U64 index = pdatas_count;
          U64 min = 0;
          U64 opl = pdatas_count;
          for(;;)
          {
            U64 mid = (min + opl)/2;
            PE_Arm64Pdata *pdata = pdatas + mid;
            if(voff < pdata->voff_first)
            {
              opl = mid;
            }
            else if(pdata->voff_first < voff)
            {
              min = mid;
            }
            else
            {
              index = mid;
              break;
            }
            if(min + 1 >= opl)
            {
              index = min;
              break;
            }
          }
          
          // rjf: if we are in range fill result
          {
            PE_Arm64Pdata *pdata = pdatas + index;
            U32 pdata_flag = pdata->combined & 0x3;
            B32 is_good = 1;
            U32 function_size = max_U32;

            if (pdata_flag != 0)
            {
              //- antoniom: packed unwind data
              function_size = 4 * ((pdata->combined >> 2) & 0x7ff);
              is_good = pdata->voff_first <= voff && voff < (pdata->voff_first + function_size);
            }
            //- antoniom: xdata will be handled in its own path

            if(is_good)
            {
              first_pdata = push_array(arena, PE_Arm64Pdata, 1);
              MemoryCopyStruct(first_pdata, pdata);
            }
          }
        }
        break;
      }
    }
  }
  return first_pdata;
}

typedef struct PE_ParsedPackedUnwindDataArm64 PE_ParsedPackedUnwindDataArm64;
struct PE_ParsedPackedUnwindDataArm64
{
  U32 function_size;
  U32 regf;
  U32 regi;
  U32 h;
  U32 cr;
  U32 frame_size;
  U32 flags;
};

typedef struct PE_ParsedPackedXDataArm64 PE_ParsedPackedXDataArm64;
struct PE_ParsedPackedXDataArm64
{
  U32 function_size;
  U32 exception_data_present;
  U32 packed_epilog;
  U32 epilog_count;
  U32 code_words;
};

typedef struct PE_UnwindCodeArm64 PE_UnwindCodeArm64;
struct PE_UnwindCodeArm64
{
  PE_UnwindOpCodeArm64 op;
  U32 reg0;
  U32 reg1;
  U32 sp_off;
  U32 add_to_reg;
};


// TODO(antoniom): use list approach again
internal PE_UnwindCodeArm64
ctrl_unwind_code_from_packed_unwind_data__pe_arm64(U32 packed_unwind_data, S32 step)
{
  PE_UnwindCodeArm64 result = {0};

  PE_ParsedPackedUnwindDataArm64 parsed_data = {0};
  parsed_data.function_size = 4 * ((packed_unwind_data >> 2) & 0x7ff);
  parsed_data.regf = (packed_unwind_data >> 13) & 0x7;
  if(parsed_data.regf != 0)
  {
    parsed_data.regf += 1;
  }
  parsed_data.regi = (packed_unwind_data >> 16) & 0xf;
  parsed_data.h = (packed_unwind_data >> 20) & 0x1;
  parsed_data.cr = (packed_unwind_data >> 21) & 0x3;
  parsed_data.frame_size = 16 * ((packed_unwind_data >> 23) & 0x1ff);

  U32 int_size = parsed_data.regi * 8;
  U32 float_size = parsed_data.regf * 8;
  U32 save_size = ((int_size + float_size + (8 * 8 * parsed_data.h)) + 0xf) & ~0xf;
  U32 loc_size = parsed_data.frame_size - save_size;

  if(parsed_data.cr == 1)
  {
    int_size += 8;
  }

  if(step == 0)
  {
    result.op = PE_UnwindOpCodeArm64_end;
  }

  if(parsed_data.cr == 2)
  {
    step -= 1;
    if (step == 0)
    {
      result.op = PE_UnwindOpCodeArm64_pac_sign_lr;
    }
  }

  if(step > 0)
  {
    step -= parsed_data.regi & 1;
    if (step == 0)
    {
      if (parsed_data.regi == 1)
      {
        result.op = PE_UnwindOpCodeArm64_save_regp_x;
        result.sp_off = int_size;
      }
      else
      {
        result.op = PE_UnwindOpCodeArm64_save_reg;
        result.sp_off = int_size - 8;
      }

      result.reg0 = parsed_data.regi + OffsetOf(REGS_RegBlockARM64, x0);
      if ((parsed_data.regi & 1) && parsed_data.cr == 1)
      {
        result.op = PE_UnwindOpCodeArm64_save_regp;
        result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
        result.sp_off = int_size - 16;
      }
    }
  }

  if (step > 0)
  {
    step -= (parsed_data.regi / 2);
    if (step <= 0)
    {
      if (step < 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_regp;
        result.sp_off = 16 * -step;
        result.reg0 = 19 + (2 * -step) + OffsetOf(REGS_RegBlockARM64, x0);
        result.reg1 = 20 + (2 * -step) + OffsetOf(REGS_RegBlockARM64, x0);
      }
      else if (step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_regp_x;
        result.sp_off = int_size;
        result.reg0 = 19 + OffsetOf(REGS_RegBlockARM64, x0);;
        result.reg1 = 20 + OffsetOf(REGS_RegBlockARM64, x0);;
      }
    }
  }

  if(step > 0)
  {
    if(parsed_data.cr == 1 && (parsed_data.regi % 2 == 0))
    {
      step -= 1;
      if (step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_reg;
        result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);;
        result.sp_off = int_size - 8;
      }
    }
  }

  if(step > 0)
  {
    if(parsed_data.regf & 1)
    {
      step -= 1;
      if(step == 0)
      {
        if(parsed_data.regf == 1)
        {
          result.op = PE_UnwindOpCodeArm64_save_freg;
          result.sp_off = 8;
          result.reg0 = 8 + OffsetOf(REGS_RegBlockARM64, v0);
          if(parsed_data.regi == 0 && parsed_data.cr == 0)
          {
            result.op = PE_UnwindOpCodeArm64_save_freg_x;
          }
        }
        else
        {
          result.op = PE_UnwindOpCodeArm64_save_freg;
          result.sp_off = float_size - 8;
          result.reg0 = 8 + parsed_data.regf + OffsetOf(REGS_RegBlockARM64, v0);;
        }
      }
    }
  }

  if(step > 0)
  {
    step -= parsed_data.regf / 2;
    if (step == 0)
    {
      if(parsed_data.regi == 0 && parsed_data.cr == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_fregp_x;
      }
      else
      {
        result.op = PE_UnwindOpCodeArm64_save_fregp;
      }
      // TODO(antoniom): check
      result.sp_off = int_size + float_size;
      result.reg0 = 8 + (2 * -step) + OffsetOf(REGS_RegBlockARM64, v0);
      result.reg1 = 9 + (2 * -step) + OffsetOf(REGS_RegBlockARM64, v0);
    }
    else
    {
      result.op = PE_UnwindOpCodeArm64_save_fregp;
      result.sp_off = int_size + float_size - (16 * -step);
      result.reg0 = 8 + OffsetOf(REGS_RegBlockARM64, v0);
      result.reg1 = 9 + OffsetOf(REGS_RegBlockARM64, v0);
    }
  }

  if(step > 0)
  {
    if(parsed_data.h != 0)
    {
      step -= 4;
      if (step <= 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_reg;
        result.sp_off = int_size + float_size + (16 * -step);
        result.reg0 = 2 * -step + OffsetOf(REGS_RegBlockARM64, x0);
        result.reg1 = 1 + (2 * -step) + OffsetOf(REGS_RegBlockARM64, x0);
      }
    }
  }

  if((parsed_data.cr == 2 || parsed_data.cr == 3) && parsed_data.frame_size <= 512)
  {
    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_fplr_x;
        result.sp_off = loc_size;
        result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);
        result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_set_fp;
      }
    }
  }
  else if((parsed_data.cr == 2 || parsed_data.cr == 3) &&
          (512 < parsed_data.frame_size && parsed_data.frame_size <= 4080))
  {
    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = loc_size;
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_fplr_x;
        result.sp_off = 0;
        result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);
        result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_set_fp;
      }
    }
  }
  else if((parsed_data.cr == 2 || parsed_data.cr == 3) &&
          4080 < parsed_data.frame_size)
  {
    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = 4080;
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = loc_size - 4080;
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_save_fplr_x;
        result.sp_off = 0;
        result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);
        result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step)
      {
        result.op = PE_UnwindOpCodeArm64_set_fp;
      }
    }
  }
  else if((parsed_data.cr == 0 || parsed_data.cr == 1) &&
          parsed_data.frame_size <= 4080)
  {
    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = loc_size;
      }
    }
  }
  else if((parsed_data.cr == 0 || parsed_data.cr == 1) &&
          4080 < parsed_data.frame_size)
  {
    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = 4080;
      }
    }

    if(step > 0)
    {
      step -= 1;
      if(step == 0)
      {
        result.op = PE_UnwindOpCodeArm64_alloc_m;
        result.sp_off = loc_size - 4080;
      }
    }
  }

  return(result);
}

internal PE_UnwindCodeArm64
ctrl_unwind_code_from_xdata__pe_arm64(CTRL_Handle process_handle, U64 endt_us, U64 code_words, U64 xdata_voff, U64 unwind_off_start, S32 step, S32 max_step, B32 *out_is_good, B32 *out_is_stale)
{
  PE_UnwindCodeArm64 result = {0};
  B32 is_good = 1;
  B32 is_stale = 0;
  U64 unwind_off = unwind_off_start;
  B32 keep_parsing = (unwind_off - unwind_off_start) < (code_words * 4);
  S32 cur_step = 0;

  if(step < 0)
  {
    result.op = PE_UnwindOpCodeArm64_end;
    keep_parsing = 0;
  }

  while(keep_parsing)
  {
    MemoryZeroStruct(&result);

    U32 unwind_header = 0;
    is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(xdata_voff+unwind_off, xdata_voff+unwind_off+sizeof(U32)), &is_stale, &unwind_header, endt_us);
    is_good = is_good && !is_stale;

    U8 unwind_op = unwind_header & 0xff;
    U8 *unwind_header_u8s = (U8*)&unwind_header;

    if((unwind_op >> 5) == 0)
    {
      // 000xxxxx
      // epilog: add sp, sp, (x * 16)
      result.op = PE_UnwindOpCodeArm64_alloc_s;
      result.sp_off = (unwind_op & 0x1f) * 16;
      unwind_off += 1;
    }
    else if((unwind_op >> 5) == 1)
    {
      // 001zzzzz
      result.op = PE_UnwindOpCodeArm64_save_r19r20_x;
      result.sp_off = (unwind_op & 0x1f) * 8;
      result.reg0 = 19 + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = 20 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 1;
    }
    else if((unwind_op >> 6) == 1)
    {
      // 01zzzzzz
      result.op = PE_UnwindOpCodeArm64_save_fplr;
      result.sp_off = (unwind_op & 0x3f) * 8;
      result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 1;
    }
    else if((unwind_op >> 6) == 2)
    {
      // 10zzzzzz
      result.op = PE_UnwindOpCodeArm64_save_fplr_x;
      result.sp_off = ((unwind_op & 0x3f) + 1) * 8;
      result.reg0 = 29 + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 1;
    }
    else if((unwind_op >> 3) == 24)
    {
      // 11000xxx'xxxxxxxx
      U32 stack_size = ((unwind_op & 0x3) << 8) | unwind_header_u8s[1];
      result.op = PE_UnwindOpCodeArm64_alloc_m;
      result.sp_off = stack_size * 16;
      unwind_off += 2;
    }
    else if((unwind_op >> 3) == 29)
    {
      //- antoniom: custom stack cases for asm routines (__security_pop_cookie)
      unwind_off += 1;
    }
    else if((unwind_op >> 2) == 50)
    {
      // 110010xx'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x3) << 2) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_regp;
      result.sp_off = (unwind_header_u8s[1] & 0x3f) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = reg_off + 1 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 2;
    }
    else if((unwind_op >> 2) == 51)
    {
      // 110011xx'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x3) << 2) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_regp_x;
      result.sp_off = ((unwind_header_u8s[1] & 0x3f) + 1) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = reg_off + 1 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 2;
    }
    else if((unwind_op >> 2) == 52)
    {
      // 110100xx'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x3) << 2) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_reg;
      result.sp_off = (unwind_header_u8s[1] & 0x3f) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 2;
    }
    else if((unwind_op >> 1) == 106)
    {
      // 1101010x'xxxzzzzz
      U32 reg_off = ((unwind_op & 0x1) << 1) | ((unwind_header_u8s[1] >> 5) & 0x7);
      result.op = PE_UnwindOpCodeArm64_save_reg_x;
      result.sp_off = ((unwind_header_u8s[1] & 0x1f) + 1) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 2;
    }
    else if((unwind_op >> 1) == 107)
    {
      // 1101011x'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x1) << 1) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_lrpair;
      result.sp_off = ((unwind_header_u8s[1] & 0x3f) + 1) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, x0);
      result.reg1 = 30 + OffsetOf(REGS_RegBlockARM64, x0);
      unwind_off += 2;
    }
    else if((unwind_op >> 1) == 108)
    {
      // 1101100x'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x1) << 1) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_fregp;
      result.sp_off = (unwind_header_u8s[1] & 0x3f) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, v0);
      result.reg1 = reg_off + 1 + OffsetOf(REGS_RegBlockARM64, v0);
      unwind_off += 2;
    }
    else if((unwind_op >> 1) == 109)
    {
      // 1101101x'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x1) << 1) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_fregp_x;
      result.sp_off = ((unwind_header_u8s[1] & 0x3f) + 1) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, v0);
      result.reg1 = reg_off + 1 + OffsetOf(REGS_RegBlockARM64, v0);
      unwind_off += 2;
    }
    else if((unwind_op >> 1) == 110)
    {
      // 1101110x'xxzzzzzz
      U32 reg_off = ((unwind_op & 0x1) << 1) | ((unwind_header_u8s[1] >> 6) & 0x3);
      result.op = PE_UnwindOpCodeArm64_save_freg;
      result.sp_off = (unwind_header_u8s[1] & 0x3f) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, v0);
      unwind_off += 2;
    }
    else if(unwind_op == 222)
    {
      // save_freg_x
      // 11011110'xxxzzzzz:
      U32 reg_off = (unwind_header_u8s[1] >> 5) & 0x7;
      result.op = PE_UnwindOpCodeArm64_save_freg_x;
      result.sp_off = ((unwind_header_u8s[1] & 0x1f) + 1) * 8;
      result.reg0 = reg_off + OffsetOf(REGS_RegBlockARM64, v0); 
      unwind_off += 2;
    }
    else if(unwind_op == 224)
    {
      // alloc_l
      // 11100000'xxxxxxxx'xxxxxxxx'xxxxxxxx
      result.op = PE_UnwindOpCodeArm64_alloc_l;
      result.sp_off = (unwind_header & 0xffffff) * 16;
      unwind_off += 4;
    }
    else if(unwind_op == 225)
    {
      // 11100001
      result.op = PE_UnwindOpCodeArm64_set_fp;
      unwind_off += 1;
    }
    else if(unwind_op == 226)
    {
      // 11100010'xxxxxxxx
      result.op = PE_UnwindOpCodeArm64_add_fp;
      result.add_to_reg = unwind_header_u8s[1] * 8;
      unwind_off += 2;
    }
    else if(unwind_op == 227)
    {
      // 11100011
      //- antoniom: skip nops
      unwind_off += 1;
    }
    else if(unwind_op == 228)
    {
      // 11100100
      result.op = PE_UnwindOpCodeArm64_end;
      unwind_off += 1;
      keep_parsing = 0;
    }
    else if(unwind_op == 229)
    {
      // 11100101
      result.op = PE_UnwindOpCodeArm64_end_c;
      unwind_off += 1;
    }
    else if(unwind_op == 230)
    {
      // 11100110
      result.op = PE_UnwindOpCodeArm64_save_next;
      unwind_off += 1;
    }
    else if(unwind_op == 231)
    {
      // 11100111
      unwind_off += 1;
    }
    else if(unwind_op == 232)
    {
      // MSFT_OP_TRAP_FRAME
    }
    else if(unwind_op == 233)
    {
      // MSFT_OP_MACHINE_FRAME
    }
    else if(unwind_op == 234)
    {
      // MSFT_OP_CONTEXT
    }
    else if(unwind_op == 235)
    {
      // MSFT_OP_EC_CONTEXT
    }
    else if(unwind_op == 236)
    {
      // MSFT_OP_CLEAR_UNWOUND_TO_CALL
    }
    else if(unwind_op == 252)
    {
      // 11111100
      // qreg
      unwind_off += 1;
    }

    if (step == max_step)
    {
      keep_parsing = 0;
    }
    step += 1;
    keep_parsing = keep_parsing && is_good && ((unwind_off - unwind_off_start) < (code_words * 4));
  }

  *out_is_stale = is_stale;
  *out_is_good = is_good && !is_stale;

  return(result);
}

internal CTRL_UnwindStepResult
ctrl_unwind_step__pe_arm64(CTRL_EntityStore *store, CTRL_Handle process_handle, CTRL_Handle module_handle, REGS_RegBlockARM64 *regs, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);

  B32 is_stale = 0;
  B32 is_good = 1;

  //////////////////////////////
  //- antoniom: unpack parameters
  //
  CTRL_Entity *module = ctrl_entity_from_handle(store, module_handle);
  CTRL_Entity *process = ctrl_entity_from_handle(store, process_handle);
  U64 ip_voff = regs->pc.u64 - module->vaddr_range.min;

  PE_Arm64Pdata *first_pdata = ctrl_arm64_pdata_from_module_voff(scratch.arena, module_handle, ip_voff);

  U64 new_pc = 0;
  U64 new_sp = 0;

  U64 function_start_vaddr = first_pdata ? first_pdata->voff_first + module->vaddr_range.min : 0;
  U64 function_end_vaddr = 0;

  //- antoniom: find function end vaddr
  U32 combined_flag = first_pdata ? first_pdata->combined & 0x3 : 0;
  if(first_pdata && combined_flag != 0)
  {
    U32 packed_unwind_data = first_pdata->combined;
    function_end_vaddr = function_start_vaddr + (4 * ((packed_unwind_data >> 2) & 0x7FF));
  }
  else if(first_pdata)
  {
    U64 xdata_voff = first_pdata->combined + module->vaddr_range.min;
    U32 header = 0;
    is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, xdata_voff, &is_stale, &header, endt_us);
    is_good = is_good && !is_stale;
    function_end_vaddr = function_start_vaddr + (4 * (header & 0x3ffff));
  }

  //- antoniom: check to see if in epilog
  B32 has_pdata_and_in_epilog = 0;
  if(first_pdata)
  {
    B32 is_epilog = 0;
    B32 keep_parsing = 1;
    U64 read_vaddr = regs->pc.u64;

    //- antoniom: only read 16 instructions before the end of the function
    Rng1U64 epilog_vaddr_rng = r1u64(function_end_vaddr - 0x40, function_end_vaddr + 0x4);

    //- antoniom: Check to see if in epilog
    for(B32 keep_parsing = 1; keep_parsing;)
    {
      U32 inst = 0;
      if(contains_1u64(epilog_vaddr_rng, read_vaddr))
      {
        is_good = ctrl_read_cached_process_memory(process_handle, r1u64(read_vaddr, read_vaddr+sizeof(inst)), &is_stale, &inst, endt_us);
        is_good = is_good && !is_stale;
      }

      if(!is_good)
      {
        keep_parsing = 0;
      }
      else
      {
        if((inst & 0xFFC003FF) == 0x910003FF)
        {
          // add sp, sp, #imm
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFFC003FF) == 0x8B0003FF)
        {
          // add sp, sp, reg, lsl #imm
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if(inst == 0xD65F03C0)
        {
          // ret
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFC000000) == 0x94000000)
        {
          // bl (e.g. __security_pop_cookie)
          // go to next instruction
          read_vaddr += 4;
        }
        else if(inst == 0x910002BF)
        {
          // mov sp, x29
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFFC003E0) == 0xA9C003E0)
        {
          // ldp reg0, reg1, [sp, #imm]
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFFC003E0) == 0xA8C003E0)
        {
          // ldp reg0, reg1, [sp], #imm (post-indexed load)
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFFE007E0) == 0xF94007E0)
        {
          // ldr reg0, [sp, #imm]
          keep_parsing = 0;
          is_epilog = 1;
        }
        else if((inst & 0xFFE007E0) == 0xF84007E0)
        {
          // ldr reg0, [sp], #imm (post-indexed_load)
          keep_parsing = 0;
          is_epilog = 1;
        }
        else
        {
          // TODO(antoniom): fregs
          keep_parsing = 0;
        }
      }
    }
    has_pdata_and_in_epilog = is_epilog;
  }

  //- antoniom: pdata & in epilog -> epilog unwind
  if(first_pdata && has_pdata_and_in_epilog)
  {
    U64 read_vaddr = regs->pc.u64;
    new_sp = regs->x31.u64;
    new_pc = regs->pc.u64;

    for (B32 keep_parsing = 1; keep_parsing;)
    {
      U32 inst = 0;
      is_good = ctrl_read_cached_process_memory(process_handle, r1u64(read_vaddr, read_vaddr+sizeof(inst)), &is_stale, &inst, endt_us);
      is_good = is_good && !is_stale;
      read_vaddr += 4;

      if(is_good)
      {
        if((inst & 0xFFC003FF) == 0x910003FF)
        {
          // add sp, sp, #imm
          B32 shift = (inst >> 22) & 0x1;
          U32 imm = (inst >> 10) & 0xFFF;
          new_sp += shift ? (imm << 12) : imm;
        }
        else if((inst & 0xFFC003FF) == 0x8B0003FF)
        {
          // add sp, sp, lsl #imm
          U8 option = (inst >> 13) & 0x7;
          U32 shift_amount = (inst >> 10) & 0x3F;
          REGS_Reg64 *reg = &regs->x0 + ((inst >> 16) & 0x1F);
          new_sp += (reg->u64 << shift_amount);
        }
        else if(inst == 0xD65F03C0)
        {
          // ret
          U64 mask = ~0ULL;
          mask >>= 17;
          new_pc = regs->x30.u64 & mask;
          keep_parsing = 0;
        }
        else if((inst & 0xFC000000) == 0x94000000)
        {
          // bl (e.g. __security_pop_cookie)
          //- antoniom: : it's possible to encounter a security_pop_cookie, which does add sp, sp, #0x10
          S64 sign_extended_imm = ((S64)(inst & 0x3FFFFFF) << 38) >> 38;
          U64 branch_read_vaddr = read_vaddr + 4 * sign_extended_imm;
          U32 insts[16];
          is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(branch_read_vaddr, branch_read_vaddr + sizeof(insts)), &is_stale, insts, endt_us);
          is_good = is_good && !is_stale;
          if(is_good) for(U32 idx = 0; idx < 16; idx += 1)
          {
            // add sp, sp, 0x10 -> ret
            if(insts[idx] == 0x910043ff && idx < 15 && insts[idx+1] == 0xd65f03c0)
            {
              new_sp += 0x10;
              break;
            }
          }
          keep_parsing = is_good;
        }
        else if(inst == 0x910002BF)
        {
          // mov sp, x29
        }
        else if((inst & 0xFEC003E0) == 0xA9C003E0)
        {
          // ldp reg0, reg1, [sp,#imm]
          REGS_Reg64 *reg0 = &regs->x0 + (inst & 0x1F);
          REGS_Reg64 *reg1 = &regs->x0 + ((inst >> 10) & 0x1F);

          S64 imm = 8 * ((inst >> 15) & 0x7f);
          S64 sign_extended_imm = (imm << 58) >> 58;
          U64 reg_read_vaddr = (U64)((S64)new_sp + sign_extended_imm);

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg0->u64, endt_us);
          is_good = is_good && !is_stale;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr + 8, &is_stale, &reg1->u64, endt_us);
          is_good = is_good && !is_stale;
          keep_parsing = is_good;
        }
        else if((inst & 0xFEC003E0) == 0xA8C003E0)
        {
          // ldp reg0, reg1, [sp], #imm (post-indexed load)
          REGS_Reg64 *reg0 = &regs->x0 + (inst & 0x1F);
          REGS_Reg64 *reg1 = &regs->x0 + ((inst >> 10) & 0x1F);

          S64 imm = 8 * ((inst >> 15) & 0x7f);
          S64 sign_extended_imm = (imm << 58) >> 58;
          U64 reg_read_vaddr = new_sp;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg0->u64, endt_us);
          is_good = is_good && !is_stale;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr + 8, &is_stale, &reg1->u64, endt_us);
          is_good = is_good && !is_stale;

          new_sp += sign_extended_imm;
          keep_parsing = is_good;
        }
        else if((inst & 0xFFE007E0) == 0xF84007E0)
        {
          // ldr reg0, [sp,#imm]
          REGS_Reg64 *reg = &regs->x0 + (inst & 0x1F);

          // TODO(antoniom): sign-extended? even right?
          S64 imm = 8 * ((inst >> 10) & 0xFFF);
          U64 reg_read_vaddr = (U64)((S64)new_sp + imm);

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg->u64, endt_us);
          is_good = is_good && !is_stale;
          keep_parsing = is_good;
        }
        else if((inst & 0xFFE007E0) == 0xF84007E0)
        {
          // ldr reg0, [sp], #imm (post-indexed load)
          REGS_Reg64 *reg = &regs->x0 + (inst & 0x1F);

          // TODO(antoniom): sign-extended?
          S64 imm = 8 * ((inst >> 12) & 0x1FF);
          S64 sign_extended_imm = (imm << 54) >> 54;
          U64 reg_read_vaddr = new_sp;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg->u64, endt_us);
          is_good = is_good && !is_stale;

          new_sp += sign_extended_imm;
          keep_parsing = is_good;
        }
        else if(inst == 0xd50323ff)
        {
          keep_parsing = 1;
        }
        else
        {
          keep_parsing = 0;
        }
      }
    }
  }

  B32 has_pdata_and_in_prolog = 0;
  if(first_pdata)
  {
    B32 is_prolog = 0;
    U64 read_vaddr = regs->pc.u64;

    for(B32 keep_parsing = 1; keep_parsing;)
    {
      U32 inst = 0;

      if(contains_1u64(r1u64(function_start_vaddr, function_start_vaddr + 0x30), read_vaddr))
      {
        is_good = ctrl_read_cached_process_memory(process_handle, r1u64(read_vaddr, read_vaddr+sizeof(inst)), &is_stale, &inst, endt_us);
        is_good = is_good && !is_stale;
      }

      if(!is_good)
      {
        keep_parsing = 0;
      }
      else
      {
        if((inst & 0xFFC003FF) == 0xD10003FF)
        {
          // sub sp, sp, #imm
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFF80001F) == 0xD280000F)
        {
          // mov x15, #imm
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFF0003FF) == 0xCB0003FF)
        {
          // sub sp, sp, reg, lsl #imm
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFC000000) == 0x94000000)
        {
          // bl (e.g. __security_pop_cookie)
          // go to prev instruction
          read_vaddr -= 4;
        }
        else if(inst == 0x910003fd)
        {
          // mov x29, sp
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFFC003E0) == 0xA9000360)
        {
          // stp reg0, reg1, [sp,#imm]
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFFC003E0) == 0xA98003E0)
        {
          // stp reg0, reg1, [sp,#imm]! (pre-indexed load)
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if((inst & 0xFFC003E0) == 0xF90003E0)
        {
          // str reg0, [sp,#imm]
          keep_parsing = 0;
          U32 str_reg = inst & 0x1f;
          if(19 <= str_reg && str_reg <= 30)
          {
            is_prolog = 1;
          }
        }
        else if((inst & 0xFFE083E0) == 0xF82083E0)
        {
          // str reg0, [sp,#imm]! (pre-indexed_load)
          keep_parsing = 0;
          is_prolog = 1;
        }
        else if(inst == 0xd503237f)
        {
          // pacibsp
          keep_parsing = 0;
          is_prolog = 1;
        }
        else
        {
          // TODO(antoniom): fregs
          keep_parsing = 0;
        }
      }
    }
    has_pdata_and_in_prolog = is_prolog;
  }

  //- antoniom: pdata & in prolog -> prolog unwind
  //  need to "undo" previous instructions, so the execution stream
  //  starts at the previous instruction. This makes operations look
  //  like the epilog block operations
  if (first_pdata && has_pdata_and_in_prolog)
  {
    U64 read_vaddr = regs->pc.u64 - 4;
    new_sp = regs->x31.u64;
    new_pc = regs->pc.u64;

    for(B32 keep_parsing = 1; keep_parsing;)
    {
      U32 inst = 0;
      is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, read_vaddr, &is_stale, &inst, endt_us);
      is_good = is_good && !is_stale;

      if(is_good)
      {
        if((inst & 0xFFC003FF) == 0xD10003FF)
        {
          // sub sp, sp, #imm
          B32 shift = (inst >> 22) & 0x1;
          U32 imm = (inst >> 10) & 0xFFF;
          new_sp += shift ? (imm << 12) : imm;
        }
        else if((inst & 0xFF80001F) == 0xD280000F)
        {
          // mov x15, #imm
          //- antoniom: no need to do anything.
          //  If this is the first instruction we encouter,
          //  then it doesn't matter. If the sub sp, sp, x15, lsl #imm
          //  has been encountered, then the "functional" part of this
          //  operation has already occurred
        }
        else if((inst & 0xFF0003FF) == 0xCB0003FF)
        {
          // sub sp, sp, reg, lsl #imm
          U8 option = (inst >> 13) & 0x7;
          U32 shift_amount = (inst >> 10) & 0x3F;
          REGS_Reg64 *reg = &regs->x0 + ((inst >> 16) & 0x1F);
          new_sp += (reg->u64 << shift_amount);
        }
        else if((inst & 0xFC000000) == 0x94000000)
        {
          //- antoniom: basically for security_push_cookie.
          // Look for sub sp, sp, #0x10. We're undoing it, so we will add 0x10 to sp
          S64 sign_extended_imm = ((S64)(inst & 0x3FFFFFF) << 38) >> 38;
          U64 branch_read_vaddr = read_vaddr + 4 * sign_extended_imm;
          U32 branch_inst = 0;
          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, branch_read_vaddr, &is_stale, &branch_inst, endt_us);
          is_good = is_good && !is_stale;
          if(is_good && branch_inst == 0xd10043ff)
          {
            new_sp += 0x10;
          }
          keep_parsing = is_good;
        }
        else if(inst == 0x910003FD)
        {
          // mov x29, sp
        }
        else if((inst & 0xFFC003E0) == 0xA90003E0)
        {
          // stp reg0, reg1, [sp,#imm] -> do load
          REGS_Reg64 *reg0 = &regs->x0 + (inst & 0x1F);
          REGS_Reg64 *reg1 = &regs->x0 + ((inst >> 10) & 0x1F);

          //- antoniom: multiply by 8 after?
          S64 imm = 8 * ((inst >> 15) & 0x7f);
          S64 sign_extended_imm = (imm << 58) >> 58;
          U64 reg_read_vaddr = (U64)((S64)new_sp + sign_extended_imm);

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg0->u64, endt_us);
          is_good = is_good && !is_stale;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr + 8, &is_stale, &reg1->u64, endt_us);
          is_good = is_good && !is_stale;

          keep_parsing = is_good;
        }
        else if((inst & 0xFFC003E0) == 0xA98003E0)
        {
          // stp reg0, reg1, [sp], #imm (pre-indexed load) -> do post-index load
          REGS_Reg64 *reg0 = &regs->x0 + (inst & 0x1F);
          REGS_Reg64 *reg1 = &regs->x0 + ((inst >> 10) & 0x1F);

          S64 imm = 8 * ((inst >> 15) & 0x7f);
          S64 sign_extended_imm = (imm << 57) >> 57;
          sign_extended_imm *= -1;

          U64 reg_read_vaddr = new_sp;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg0->u64, endt_us);
          is_good = is_good && !is_stale;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr + 8, &is_stale, &reg1->u64, endt_us);
          is_good = is_good && !is_stale;

          new_sp += sign_extended_imm;
          keep_parsing = is_good;
        }
        else if((inst & 0xFFC003E0) == 0xF90003E0)
        {
          // str reg0, [sp,#imm] -> do load
          REGS_Reg64 *reg = &regs->x0 + (inst & 0x1F);

          S64 unsigned_imm = 8 * ((inst >> 10) & 0xFFF);
          U64 reg_read_vaddr = (U64)((S64)new_sp + unsigned_imm);

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg->u64, endt_us);
          is_good = is_good && !is_stale;

          keep_parsing = is_good;
        }
        else if((inst & 0xFFE083E0) == 0xF82083E0)
        {
          // str reg0, [sp], #imm (post-indexed load) -> do pre-indexed ldr
          REGS_Reg64 *reg = &regs->x0 + (inst & 0x1F);

          S64 imm = ((inst >> 10) & 0x1FF);
          S64 sign_extended_imm = (imm << 54) >> 54;
          U64 reg_read_vaddr = new_sp;

          is_good = is_good && ctrl_read_cached_process_memory_struct(process_handle, reg_read_vaddr, &is_stale, &reg->u64, endt_us);
          is_good = is_good && !is_stale;

          new_sp += sign_extended_imm;

          keep_parsing = is_good;
        }
        else
        {
          // TODO(antoniom): fregs
          keep_parsing = 0;
        }
      }
      keep_parsing = keep_parsing && (read_vaddr >= function_start_vaddr);
      read_vaddr -= 4;
    }

    //- antoniom: simulate ret
    U64 mask = ~0ULL;
    mask >>= 17;
    new_pc = regs->x30.u64 & mask;
  }

  //////////////////////////////
  //- antoniom: pdata & not in epilog/prolog -> xdata unwind
  //
  if(is_good && first_pdata && !has_pdata_and_in_epilog && !has_pdata_and_in_prolog)
  {
    B32 is_packed_unwind_data = 0;
    B32 is_xdata = 0;

    PE_Arm64Pdata *pdata = first_pdata;

    U64 xdata_code_words = 0;
    U64 xdata_code_words_ptr = 0;
    U64 xdata_step_count = 0;
    U64 xdata_voff = 0;
    U64 xdata_off = 0;

    U64 packed_unwind_count = 0;

    if(is_good && combined_flag != 0)
    {
      is_packed_unwind_data = 1;

      if (is_good)
      {
        U32 packed_unwind_data = pdata->combined;
        PE_ParsedPackedUnwindDataArm64 parsed_data = {0};
        parsed_data.function_size = 4 * ((packed_unwind_data >> 2) & 0x7FF);
        parsed_data.regf = (packed_unwind_data >> 13) & 0x7;
        if(parsed_data.regf != 0)
        {
          parsed_data.regf += 1;
        }
        parsed_data.regi = (packed_unwind_data >> 16) & 0xf;
        parsed_data.h = (packed_unwind_data >> 20) & 0x1;
        parsed_data.cr = (packed_unwind_data >> 21) & 0x3;
        parsed_data.frame_size =  16 * ((packed_unwind_data >> 23) & 0x1FF);

        if(parsed_data.cr == 2)
        {
          packed_unwind_count += 1;
        }

        U32 regi_count = (parsed_data.regi / 2) + (parsed_data.regi & 0x1);
        packed_unwind_count += regi_count;
        if(parsed_data.cr == 1 && (parsed_data.regi % 2 == 0))
        {
          packed_unwind_count += 1;
        }

        // TODO(antoniom): review
        U32 regf_count = (parsed_data.regf / 2) + (parsed_data.regf & 0x1);
        packed_unwind_count += regf_count;

        if(parsed_data.h != 0)
        {
          packed_unwind_count += 4;
        }

        if((parsed_data.cr == 2 || parsed_data.cr == 3) && parsed_data.frame_size <= 512)
        {
          packed_unwind_count += 2;
        }
        else if((parsed_data.cr == 2 || parsed_data.cr == 3) &&
                (512 < parsed_data.frame_size && parsed_data.frame_size <= 4080))
        {
          packed_unwind_count += 3;
        }
        else if((parsed_data.cr == 2 || parsed_data.cr == 3) &&
                4080 < parsed_data.frame_size)
        {
          packed_unwind_count += 4;
        }
        else if((parsed_data.cr == 0 || parsed_data.cr == 1) &&
                parsed_data.frame_size <= 4080)
        {
          packed_unwind_count += 1;
        }
        else if((parsed_data.cr == 0 || parsed_data.cr == 1) &&
                4080 < parsed_data.frame_size)
        {
          packed_unwind_count += 2;
        }
      }
    }
    else if (is_good)
    {
      is_xdata = 1;

      xdata_voff = pdata->combined + module->vaddr_range.min;
      xdata_off = 0;

      PE_ParsedPackedXDataArm64 parsed_data = {0};
      if(is_good)
      {
        U32 header = 0;
        is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(xdata_voff+xdata_off, xdata_voff+xdata_off+sizeof(U32)), &is_stale, &header, endt_us);
        is_good = is_good && !is_stale;

        parsed_data.function_size = 4 * (header & 0x3ffff);
        parsed_data.exception_data_present = (header >> 20) & 0x1;
        parsed_data.packed_epilog = (header >> 21) & 0x1;
        parsed_data.epilog_count = (header >> 22) & 0x1f;
        parsed_data.code_words = (header >> 27) & 0x1f;
      }

      is_good = is_good && (pdata->voff_first <= ip_voff && ip_voff < (pdata->voff_first + parsed_data.function_size));

      if(is_good && parsed_data.code_words == 0 && parsed_data.epilog_count == 0)
      {
        xdata_off += 4;

        U32 ext_header = 0;
        is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(xdata_voff+xdata_off, xdata_voff+xdata_off+sizeof(U32)), &is_stale, &ext_header, endt_us);
        is_good = is_good && !is_stale;

        if(is_good)
        {
          parsed_data.epilog_count = ext_header & 0xffff;
          parsed_data.code_words = (ext_header >> 16) & 0xff;
        }
      }

      xdata_off += 4;

      if(parsed_data.packed_epilog == 0)
      {
        //- antoniom: epilog scope
        U64 epilog_scope_ptr = xdata_voff + xdata_off;
        U32 min_epilog_offset = max_U32;
        U32 min_unwind_code_offset = max_U32;
        for(U32 epilog_scope_idx = 0; is_good && !is_stale && epilog_scope_idx < parsed_data.epilog_count; epilog_scope_idx += 1)
        {
          U32 epilog_scope_header = 0;
          is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(epilog_scope_ptr, epilog_scope_ptr+sizeof(U32)), &is_stale, &epilog_scope_header, endt_us);
          is_good = is_good && !is_stale;

          U32 epilog_offset = 4 * (epilog_scope_header & 0x3ffff);
          if(epilog_offset < min_epilog_offset)
          {
            min_epilog_offset = epilog_offset;
          }

          U32 unwind_code_offset = (epilog_scope_header >> 22) & 0x3ff;
          if(unwind_code_offset < min_unwind_code_offset)
          {
            min_unwind_code_offset = unwind_code_offset;
          }
        }

        if(min_epilog_offset < max_U32)
        {
          xdata_off += parsed_data.epilog_count * 4 + min_unwind_code_offset;
        }
      }

      xdata_code_words_ptr = xdata_voff + xdata_off;
      xdata_code_words = parsed_data.code_words;

      U64 unwind_off = xdata_off;
      B32 keep_parsing = (unwind_off - xdata_off) <= (4 * parsed_data.code_words);
      while(keep_parsing)
      {
        U32 unwind_header = 0;

        is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(xdata_voff+unwind_off, xdata_voff+unwind_off+sizeof(U32)), &is_stale, &unwind_header, endt_us);
        is_good = is_good && !is_stale;

        U8 unwind_op = unwind_header & 0xff;
        U8 *unwind_header_u8s = (U8*)&unwind_header;

        if((unwind_op >> 5) == 0)
        {
          // 000xxxxx
          // epilog: add sp, sp, (x * 16)
          unwind_off += 1;
        }
        else if((unwind_op >> 5) == 1)
        {
          // 001zzzzz
          unwind_off += 1;
        }
        else if((unwind_op >> 6) == 1)
        {
          // 01zzzzzz
          unwind_off += 1;
        }
        else if((unwind_op >> 6) == 2)
        {
          // 10zzzzzz
          unwind_off += 1;
        }
        else if((unwind_op >> 3) == 24)
        {
          // 11000xxx'xxxxxxxx
          unwind_off += 2;
        }
        else if((unwind_op >> 3) == 29)
        {
          // custom stack cases for asm routines
          unwind_off += 1;
        }
        else if((unwind_op >> 2) == 50)
        {
          // 110010xx'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 2) == 51)
        {
          // 110011xx'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 2) == 52)
        {
          // 110100xx'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 1) == 106)
        {
          // 1101010x'xxxzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 1) == 107)
        {
          // 1101011x'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 1) == 108)
        {
          // 1101100x'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 1) == 109)
        {
          // 1101101x'xxzzzzzz
          unwind_off += 2;
        }
        else if((unwind_op >> 1) == 110)
        {
          // 1101110x'xxzzzzzz
          unwind_off += 2;
        }
        else if(unwind_op == 222)
        {
          // save_freg_x
          // 11011110'xxxzzzzz:
          unwind_off += 2;
        }
        else if(unwind_op == 224)
        {
          // alloc_l
          // 11100000'xxxxxxxx'xxxxxxxx'xxxxxxxx
          unwind_off += 4;
        }
        else if(unwind_op == 225)
        {
          // 11100001
          unwind_off += 1;
        }
        else if(unwind_op == 226)
        {
          // 11100010'xxxxxxxx
          unwind_off += 2;
        }
        else if(unwind_op == 227)
        {
          // 11100011
          //- antoniom: skip nops
          unwind_off += 1;
        }
        else if(unwind_op == 228)
        {
          // 11100100
          unwind_off += 1;
          keep_parsing = 0;
        }
        else if(unwind_op == 229)
        {
          // 11100101
          unwind_off += 1;
        }
        else if(unwind_op == 230)
        {
          // 11100110
          unwind_off += 1;
        }
        else if(unwind_op == 231)
        {
          // 11100111
          unwind_off += 1;
        }
        else if(unwind_op == 232)
        {
          // MSFT_OP_TRAP_FRAME
        }
        else if(unwind_op == 233)
        {
          // MSFT_OP_MACHINE_FRAME
        }
        else if(unwind_op == 234)
        {
          // MSFT_OP_CONTEXT
        }
        else if(unwind_op == 235)
        {
          // MSFT_OP_EC_CONTEXT
        }
        else if(unwind_op == 236)
        {
          // MSFT_OP_CLEAR_UNWOUND_TO_CALL
        }
        else if(unwind_op == 252)
        {
          // 11111100
          // qreg
          unwind_off += 1;
        }

        xdata_step_count += 1;
        keep_parsing = keep_parsing && ((unwind_off - xdata_off) < (parsed_data.code_words * 4));
      }
    }

    is_good = is_good && !is_stale;
    if (is_good)
    {
      U32 step = 0;

      if(is_packed_unwind_data)
      {
        step = packed_unwind_count;
      }
      else if(is_xdata)
      {
        step = xdata_step_count;
      }

      new_sp = regs->x31.u64;

      if(is_good) for(B32 keep_processing = 1; keep_processing;)
      {
        PE_UnwindCodeArm64 unwind_code;

        if(is_good && is_packed_unwind_data)
        {
          unwind_code = ctrl_unwind_code_from_packed_unwind_data__pe_arm64(pdata->combined, step);
        }
        else if(is_good && is_xdata)
        {
          unwind_code = ctrl_unwind_code_from_xdata__pe_arm64(process_handle, endt_us, xdata_code_words, xdata_voff, xdata_off, step, xdata_step_count, &is_good, &is_stale);
        }

        keep_processing = is_good;
        switch(unwind_code.op)
        {
          default:{}break;

          case PE_UnwindOpCodeArm64_nop: {}break;

          case PE_UnwindOpCodeArm64_MSFT_OP_TRAP_FRAME:
          case PE_UnwindOpCodeArm64_MSFT_OP_MACHINE_FRAME:
          case PE_UnwindOpCodeArm64_MSFT_OP_CONTEXT:
          case PE_UnwindOpCodeArm64_MSFT_OP_EC_CONTEXT:
          case PE_UnwindOpCodeArm64_MSFT_OP_CLEAR_UNWOUND_TO_CALL:
          {
          }break;

          case PE_UnwindOpCodeArm64_end_c:
          {
            Assert(!"Wait until this is encountered");
          }break;

          case PE_UnwindOpCodeArm64_alloc_s:
          case PE_UnwindOpCodeArm64_alloc_m:
          case PE_UnwindOpCodeArm64_alloc_l:
          {
            new_sp += unwind_code.sp_off;
          }break;

          case PE_UnwindOpCodeArm64_pac_sign_lr:
          {
            //- antoniom: treating as no-op, aren't allowed to read "secret" registers
            // used for generating the hash
          }break;

          case PE_UnwindOpCodeArm64_save_r19r20_x:
          case PE_UnwindOpCodeArm64_save_fplr:
          case PE_UnwindOpCodeArm64_save_fplr_x:
          case PE_UnwindOpCodeArm64_save_regp:
          case PE_UnwindOpCodeArm64_save_regp_x:
          case PE_UnwindOpCodeArm64_save_reg:
          case PE_UnwindOpCodeArm64_save_reg_x:
          case PE_UnwindOpCodeArm64_save_lrpair:
          case PE_UnwindOpCodeArm64_save_fregp:
          case PE_UnwindOpCodeArm64_save_fregp_x:
          case PE_UnwindOpCodeArm64_save_freg:
          case PE_UnwindOpCodeArm64_save_freg_x:
          case PE_UnwindOpCodeArm64_save_qregp:
          case PE_UnwindOpCodeArm64_save_qregp_x:
          {
            // 0 is not a valid value for unwind.reg0/reg1
            REGS_Reg64 *first_reg = &regs->x0 + unwind_code.reg0;
            REGS_Reg64 *second_reg = (unwind_code.reg1) ? &regs->x0 + unwind_code.reg1 : 0;

            U64 first_reg_mask = ~0ULL;
            U64 second_reg_mask = ~0ULL;

            U64 sp = new_sp;
            U64 sp_off = unwind_code.sp_off;

            if(unwind_code.op == PE_UnwindOpCodeArm64_save_fplr || unwind_code.op == PE_UnwindOpCodeArm64_save_fplr_x)
            {
              //- antoniom: assume a user-mode module
              second_reg_mask >>= 17;
            }

            if(PE_UnwindOpCodeArm64_save_r19r20_x <= unwind_code.op && unwind_code.op <= PE_UnwindOpCodeArm64_save_freg_x)
            {
              sp_off = 0;
            }

            if(first_reg)
            {
              U64 first_reg_read = 0;
              is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(sp+sp_off, sp+sp_off+sizeof(first_reg_read)), &is_stale, &first_reg_read, endt_us);
              is_good = is_good && !is_stale;
              if(is_good)
              {
                first_reg->u64 = first_reg_read & first_reg_mask;
              }
              else
              {
                is_good = 0;
              }
            }

            if(second_reg)
            {
              U64 second_reg_read = 0;
              is_good = is_good && ctrl_read_cached_process_memory(process_handle, r1u64(sp+sp_off+8, sp+sp_off+8+sizeof(second_reg_read)), &is_stale, &second_reg_read, endt_us);
              is_good = is_good && !is_stale;
              if(is_good)
              {
                second_reg->u64 = second_reg_read & second_reg_mask;
              }
              else
              {
                is_good = 0;
              }
            }

            if(PE_UnwindOpCodeArm64_save_r19r20_x <= unwind_code.op && unwind_code.op <= PE_UnwindOpCodeArm64_save_freg_x)
            {
              new_sp += unwind_code.sp_off;
            }
          }break;

          case PE_UnwindOpCodeArm64_set_fp:
          {
            regs->x31.u64 = regs->x29.u64;
            new_sp = regs->x29.u64;
          }break;
          case PE_UnwindOpCodeArm64_add_fp:
          {
            Assert(!"Hit this");
            regs->x29.u64 = regs->x31.u64 + unwind_code.add_to_reg;
          }break;
          case PE_UnwindOpCodeArm64_end:
          {
            keep_processing = 0;
            U64 mask = ~0ULL;
            mask >>= 17;
            new_pc = regs->x30.u64 & mask;
          }break;
          // TODO(antoniom): entry_thunks contain q* saves
          case PE_UnwindOpCodeArm64_save_next:
          {
            // TODO(antoniom): need to keep track of Int/FP pair
            // TODO(antoniom): keep track and need to see ahead/behind depending on direction
          }break;
        }

        step -= 1;
      }
    }
  }

  if(is_good && !first_pdata)
  {
    U64 mask = ~0ULL;
    mask >>= 17;
    new_pc = regs->x30.u64 & mask;
    new_sp = regs->x31.u64;
  }

  CTRL_UnwindStepResult result = {0};

  if(!is_good) {result.flags |= CTRL_UnwindFlag_Error;}
  if(is_stale) {result.flags |= CTRL_UnwindFlag_Stale;}

  if(is_good && !is_stale)
  {
    regs->pc.u64 = new_pc;
    regs->x31.u64 = new_sp;
  }

  scratch_end(scratch);
  return result;
}

//- rjf: abstracted unwind step

internal CTRL_UnwindStepResult
ctrl_unwind_step(CTRL_EntityStore *store, CTRL_Handle process, CTRL_Handle module, Arch arch, void *reg_block, U64 endt_us)
{
  CTRL_UnwindStepResult result = {0};
  switch(arch)
  {
    default:{}break;
    case Arch_x64:
    {
      result = ctrl_unwind_step__pe_x64(store, process, module, (REGS_RegBlockX64 *)reg_block, endt_us);
    }break;
    case Arch_arm64:
    {
      result = ctrl_unwind_step__pe_arm64(store, process, module, (REGS_RegBlockARM64 *)reg_block, endt_us);
    }break;
  }
  return result;
}

//- rjf: abstracted full unwind

internal CTRL_Unwind
ctrl_unwind_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_Handle thread, U64 endt_us)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_Unwind unwind = {0};
  unwind.flags |= CTRL_UnwindFlag_Error;
  
  //- rjf: unpack args
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(store, thread);
  CTRL_Entity *process_entity = thread_entity->parent;
  Arch arch = thread_entity->arch;
  U64 arch_reg_block_size = regs_block_size_from_arch(arch);
  
  //- rjf: grab initial register block
  void *regs_block = ctrl_query_cached_reg_block_from_thread(scratch.arena, store, thread);
  B32 regs_block_good = (arch != Arch_Null && regs_block != 0);
  
  //- rjf: loop & unwind
  CTRL_UnwindFrameNode *first_frame_node = 0;
  CTRL_UnwindFrameNode *last_frame_node = 0;
  U64 frame_node_count = 0;
  if(regs_block_good)
  {
    unwind.flags = 0;
    for(;;)
    {
      // rjf: regs -> rip*module
      U64 rip = regs_rip_from_arch_block(arch, regs_block);
      CTRL_Handle module = {0};
      for(CTRL_Entity *m = process_entity->first; m != &ctrl_entity_nil; m = m->next)
      {
        if(m->kind == CTRL_EntityKind_Module && contains_1u64(m->vaddr_range, rip))
        {
          module = m->handle;
          break;
        }
      }
      
      // rjf: cancel on 0 rip
      if(rip == 0)
      {
        break;
      }
      
      // rjf: valid step -> push frame
      CTRL_UnwindFrameNode *frame_node = push_array(scratch.arena, CTRL_UnwindFrameNode, 1);
      CTRL_UnwindFrame *frame = &frame_node->v;
      frame->regs = push_array_no_zero(arena, U8, arch_reg_block_size);
      MemoryCopy(frame->regs, regs_block, arch_reg_block_size);
      DLLPushBack(first_frame_node, last_frame_node, frame_node);
      frame_node_count += 1;
      
      // rjf: unwind one step
      CTRL_UnwindStepResult step = ctrl_unwind_step(store, process_entity->handle, module, arch, regs_block, endt_us);
      unwind.flags |= step.flags;
      if(step.flags & CTRL_UnwindFlag_Error ||
         regs_rsp_from_arch_block(arch, regs_block) == 0 ||
         regs_rip_from_arch_block(arch, regs_block) == 0 ||
         regs_rip_from_arch_block(arch, regs_block) == rip)
      {
        break;
      }
    }
  }
  
  //- rjf: bake frames list into result array
  {
    unwind.frames.count = frame_node_count;
    unwind.frames.v = push_array(arena, CTRL_UnwindFrame, unwind.frames.count);
    U64 idx = 0;
    for(CTRL_UnwindFrameNode *n = first_frame_node; n != 0; n = n->next, idx += 1)
    {
      unwind.frames.v[idx] = n->v;
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return unwind;
}

////////////////////////////////
//~ rjf: Call Stack Building Functions

internal CTRL_CallStack
ctrl_call_stack_from_unwind(Arena *arena, DI_Scope *di_scope, CTRL_Entity *process, CTRL_Unwind *base_unwind)
{
  Arch arch = process->arch;
  CTRL_CallStack result = {0};
  result.concrete_frame_count = base_unwind->frames.count;
  result.total_frame_count = result.concrete_frame_count;
  result.frames = push_array(arena, CTRL_CallStackFrame, result.concrete_frame_count);
  for(U64 idx = 0; idx < result.concrete_frame_count; idx += 1)
  {
    CTRL_UnwindFrame *src = &base_unwind->frames.v[idx];
    CTRL_CallStackFrame *dst = &result.frames[idx];
    U64 rip_vaddr = regs_rip_from_arch_block(arch, src->regs);
    CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
    U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
    RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
    RDI_Scope *scope = rdi_scope_from_voff(rdi, rip_voff);
    
    // rjf: fill concrete frame info
    dst->regs = src->regs;
    dst->rdi = rdi;
    dst->procedure = rdi_element_from_name_idx(rdi, Procedures, scope->proc_idx);
    
    // rjf: push inline frames
    for(RDI_Scope *s = scope;
        s->inline_site_idx != 0;
        s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
    {
      RDI_InlineSite *site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
      CTRL_CallStackInlineFrame *inline_frame = push_array(arena, CTRL_CallStackInlineFrame, 1);
      DLLPushFront(dst->first_inline_frame, dst->last_inline_frame, inline_frame);
      inline_frame->inline_site = site;
      dst->inline_frame_count += 1;
      result.inline_frame_count += 1;
      result.total_frame_count += 1;
    }
  }
  return result;
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

internal E_String2NumMap *
ctrl_string2reg_from_arch(Arch arch)
{
  return &ctrl_state->arch_string2reg_tables[arch];
}

internal E_String2NumMap *
ctrl_string2alias_from_arch(Arch arch)
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
    U64 needed_size = sizeof(msgs_srlzed_baked.size) + msgs_srlzed_baked.size;
    if(available_size >= needed_size)
    {
      ctrl_state->u2c_ring_write_pos += ring_write_struct(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_write_pos, &msgs_srlzed_baked.size);
      ctrl_state->u2c_ring_write_pos += ring_write(ctrl_state->u2c_ring_base, ctrl_state->u2c_ring_size, ctrl_state->u2c_ring_write_pos, msgs_srlzed_baked.str, msgs_srlzed_baked.size);
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
      String8 event_srlzed = ctrl_serialized_string_from_event(scratch.arena, &n->v, ctrl_state->c2u_ring_size-sizeof(U64));
      OS_MutexScope(ctrl_state->c2u_ring_mutex) for(;;)
      {
        U64 unconsumed_size = (ctrl_state->c2u_ring_write_pos-ctrl_state->c2u_ring_read_pos);
        U64 available_size = ctrl_state->c2u_ring_size-unconsumed_size;
        U64 needed_size = sizeof(event_srlzed.size) + event_srlzed.size;
        if(available_size >= needed_size)
        {
          ctrl_state->c2u_ring_write_pos += ring_write_struct(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_write_pos, &event_srlzed.size);
          ctrl_state->c2u_ring_write_pos += ring_write(ctrl_state->c2u_ring_base, ctrl_state->c2u_ring_size, ctrl_state->c2u_ring_write_pos, event_srlzed.str, event_srlzed.size);
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
  log_select(ctrl_state->ctrl_thread_log);
  
  //- rjf: loop
  Temp scratch = scratch_begin(0, 0);
  for(;;)
  {
    temp_end(scratch);
    log_scope_begin();
    
    //- rjf: get next messages
    CTRL_MsgList msgs = ctrl_u2c_pop_msgs(scratch.arena);
    
    //- rjf: process messages
    DMN_CtrlExclusiveAccessScope
    {
      for(CTRL_MsgNode *msg_n = msgs.first; msg_n != 0; msg_n = msg_n->next)
      {
        CTRL_Msg *msg = &msg_n->v;
        {
          log_infof("user2ctrl_msg:{kind:\"%S\"}\n", ctrl_string_from_msg_kind(msg->kind));
        }
        
        //- rjf: unpack per-message parameterizations & store
        {
          MemoryCopyArray(ctrl_state->exception_code_filters, msg->exception_code_filters);
          arena_clear(ctrl_state->user_meta_eval_arena);
          ctrl_state->user_meta_evals = *deep_copy_from_struct(ctrl_state->user_meta_eval_arena, CTRL_MetaEvalArray, &msg->meta_evals);
        }
        
        //- rjf: process message
        switch(msg->kind)
        {
          case CTRL_MsgKind_Null:
          case CTRL_MsgKind_COUNT:{}break;
          
          //- rjf: target operations
          case CTRL_MsgKind_Launch:            {ctrl_thread__launch              (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Attach:            {ctrl_thread__attach              (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Kill:              {ctrl_thread__kill                (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_KillAll:           {ctrl_thread__kill_all            (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Detach:            {ctrl_thread__detach              (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_Run:               {ctrl_thread__run                 (ctrl_ctx, msg);}break;
          case CTRL_MsgKind_SingleStep:        {ctrl_thread__single_step         (ctrl_ctx, msg);}break;
          
          //- rjf: configuration
          case CTRL_MsgKind_SetUserEntryPoints:
          {
            arena_clear(ctrl_state->user_entry_point_arena);
            MemoryZeroStruct(&ctrl_state->user_entry_points);
            for(String8Node *n = msg->entry_points.first; n != 0; n = n->next)
            {
              str8_list_push(ctrl_state->user_entry_point_arena, &ctrl_state->user_entry_points, n->string);
            }
          }break;
          case CTRL_MsgKind_SetModuleDebugInfoPath:
          {
            String8 path = msg->path;
            CTRL_Entity *module = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, msg->entity);
            CTRL_Entity *debug_info_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
            DI_Key old_dbgi_key = {debug_info_path->string, debug_info_path->timestamp};
            di_close(&old_dbgi_key);
            ctrl_entity_equip_string(ctrl_state->ctrl_thread_entity_store, debug_info_path, path);
            U64 new_dbgi_timestamp = os_properties_from_file_path(path).modified;
            debug_info_path->timestamp = new_dbgi_timestamp;
            DI_Key new_dbgi_key = {debug_info_path->string, new_dbgi_timestamp};
            di_open(&new_dbgi_key);
            CTRL_EventList evts = {0};
            CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
            evt->kind       = CTRL_EventKind_ModuleDebugInfoPathChange;
            evt->entity     = msg->entity;
            evt->string     = path;
            evt->timestamp  = new_dbgi_timestamp;
            ctrl_c2u_push_events(&evts);
          }break;
          case CTRL_MsgKind_FreezeThread:
          {
            CTRL_EventList evts = {0};
            CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
            evt->kind       = CTRL_EventKind_ThreadFrozen;
            evt->entity     = msg->entity;
            ctrl_c2u_push_events(&evts);
          }break;
          case CTRL_MsgKind_ThawThread:
          {
            CTRL_EventList evts = {0};
            CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
            evt->kind       = CTRL_EventKind_ThreadThawed;
            evt->entity     = msg->entity;
            ctrl_c2u_push_events(&evts);
          }break;
        }
      }
    }
    
    //- rjf: gather & output logs
    LogScopeResult log = log_scope_end(scratch.arena);
    ctrl_thread__flush_info_log(log.strings[LogMsgKind_Info]);
    if(log.strings[LogMsgKind_UserError].size != 0)
    {
      CTRL_EventList evts = {0};
      CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
      evt->kind       = CTRL_EventKind_Error;
      evt->string     = log.strings[LogMsgKind_UserError];
      ctrl_c2u_push_events(&evts);
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

//- rjf: breakpoint resolution

internal void
ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, CTRL_Handle process, CTRL_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  if(user_bps->first == 0) { return; }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *di_scope = di_scope_open();
  CTRL_Entity *module_entity = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, module);
  CTRL_Entity *debug_info_path_entity = ctrl_entity_child_from_kind(module_entity, CTRL_EntityKind_DebugInfoPath);
  DI_Key dbgi_key = {debug_info_path_entity->string, debug_info_path_entity->timestamp};
  RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, max_U64);
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
          RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
          if(mapptr != 0)
          {
            RDI_ParsedNameMap map = {0};
            rdi_parsed_from_name_map(rdi, mapptr, &map);
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
          RDI_SourceFile *src = rdi_element_from_name_idx(rdi, SourceFiles, src_id);
          RDI_SourceLineMap *src_line_map = rdi_element_from_name_idx(rdi, SourceLineMaps, src->source_line_map_idx);
          RDI_ParsedSourceLineMap line_map = {0};
          rdi_parsed_from_source_line_map(rdi, src_line_map, &line_map);
          U32 voff_count = 0;
          U64 *voffs = rdi_line_voffs_from_num(&line_map, pt.line, &voff_count);
          for(U32 i = 0; i < voff_count; i += 1)
          {
            U64 vaddr = voffs[i] + base_vaddr;
            DMN_Trap trap = {process.dmn_handle, vaddr, (U64)bp};
            dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
          }
        }
      }break;
      
      //- rjf: symbol:voff-based breakpoints
      case CTRL_UserBreakpointKind_SymbolNameAndOffset:
      {
        String8 symbol_name = bp->string;
        U64 voff = bp->u64;
        RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Procedures);
        RDI_ParsedNameMap map = {0};
        rdi_parsed_from_name_map(rdi, mapptr, &map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, symbol_name.str, symbol_name.size);
        if(node != 0)
        {
          U32 id_count = 0;
          U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
          for(U32 match_i = 0; match_i < id_count; match_i += 1)
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, ids[match_i]);
            U64 proc_voff = rdi_first_voff_from_procedure(rdi, procedure);
            U64 proc_vaddr = proc_voff + base_vaddr;
            DMN_Trap trap = {process.dmn_handle, proc_vaddr + voff, (U64)bp};
            dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
          }
        }
      }break;
    }
  }
  di_scope_close(di_scope);
  scratch_end(scratch);
}

internal void
ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, CTRL_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  for(CTRL_UserBreakpointNode *n = user_bps->first; n != 0; n = n->next)
  {
    CTRL_UserBreakpoint *bp = &n->v;
    if(bp->kind == CTRL_UserBreakpointKind_VirtualAddress)
    {
      DMN_Trap trap = {process.dmn_handle, bp->u64, (U64)bp};
      dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
    }
  }
}

//- rjf: module lifetime open/close work

internal void
ctrl_thread__module_open(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range, String8 path)
{
  Temp scratch = scratch_begin(0, 0);

  //////////////////////////////
  //- rjf: parse module image info
  //
  Arena *arena = arena_alloc();
  PE_IntelPdata *intel_pdatas = 0;
  U64 intel_pdatas_count = 0;
  PE_Arm64Pdata *arm64_pdatas = 0;
  U64 arm64_pdatas_count = 0;
  U64 entry_point_voff = 0;
  Rng1U64 tls_vaddr_range = {0};
  U32 pdb_dbg_time = 0;
  U32 pdb_dbg_age = 0;
  Guid pdb_dbg_guid = {0};
  String8 pdb_dbg_path = str8_zero();
  U32 rdi_dbg_time = 0;
  Guid rdi_dbg_guid = {0};
  String8 rdi_dbg_path = str8_zero();
  ProfScope("unpack relevant PE info")
  {
    B32 is_valid = 1;
    
    //- rjf: read DOS header
    PE_DosHeader dos_header = {0};
    if(is_valid)
    {
      if(!dmn_process_read_struct(process.dmn_handle, vaddr_range.min, &dos_header) ||
         dos_header.magic != PE_DOS_MAGIC)
      {
        is_valid = 0;
      }
    }
    
    //- rjf: read PE magic
    U32 pe_magic = 0;
    if(is_valid)
    {
      if(!dmn_process_read_struct(process.dmn_handle, vaddr_range.min + dos_header.coff_file_offset, &pe_magic) ||
         pe_magic != PE_MAGIC)
      {
        is_valid = 0;
      }
    }
    
    //- rjf: read COFF header
    U64 coff_header_off = dos_header.coff_file_offset + sizeof(pe_magic);
    COFF_Header coff_header = {0};
    if(is_valid)
    {
      if(!dmn_process_read_struct(process.dmn_handle, vaddr_range.min + coff_header_off, &coff_header))
      {
        is_valid = 0;
      }
    }
    
    //- rjf: unpack range of optional extension header
    U32 opt_ext_size = coff_header.optional_header_size;
    Rng1U64 opt_ext_off_range = r1u64(coff_header_off + sizeof(coff_header),
                                      coff_header_off + sizeof(coff_header) + opt_ext_size);
    
    //- rjf: read optional header
    U16 optional_magic = 0;
    U64 image_base = 0;
    U64 entry_point = 0;
    U32 data_dir_count = 0;
    U64 virt_section_align = 0;
    U64 file_section_align = 0;
    Rng1U64 *data_dir_franges = 0;
    if(opt_ext_size > 0)
    {
      // rjf: read magic number
      U16 opt_ext_magic = 0;
      dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min, &opt_ext_magic);
      
      // rjf: read info
      U32 reported_data_dir_offset = 0;
      U32 reported_data_dir_count = 0;
      switch(opt_ext_magic)
      {
        case PE_PE32_MAGIC:
        {
          PE_OptionalHeader32 pe_optional = {0};
          dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min, &pe_optional);
          image_base = pe_optional.image_base;
          entry_point = pe_optional.entry_point_va;
          virt_section_align = pe_optional.section_alignment;
          file_section_align = pe_optional.file_alignment;
          reported_data_dir_offset = sizeof(pe_optional);
          reported_data_dir_count = pe_optional.data_dir_count;
        }break;
        case PE_PE32PLUS_MAGIC:
        {
          PE_OptionalHeader32Plus pe_optional = {0};
          dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min, &pe_optional);
          image_base = pe_optional.image_base;
          entry_point = pe_optional.entry_point_va;
          virt_section_align = pe_optional.section_alignment;
          file_section_align = pe_optional.file_alignment;
          reported_data_dir_offset = sizeof(pe_optional);
          reported_data_dir_count = pe_optional.data_dir_count;
        }break;
      }

      //- antoniom: find section count and section array offset
      U32 coff_header_off = dos_header.coff_file_offset + sizeof(pe_magic);
      U64 after_coff_header_off = coff_header_off + sizeof(coff_header);
      U64 after_optional_header_off = after_coff_header_off + opt_ext_size;

      Rng1U64 optional_range = {0};
      optional_range.min = ClampTop(after_coff_header_off, dim_1u64(vaddr_range));
      optional_range.max = ClampTop(after_optional_header_off, dim_1u64(vaddr_range));

      U64 section_array_offset = optional_range.max;
      COFF_SectionHeader *sec_array = (COFF_SectionHeader*)(vaddr_range.min + section_array_offset);
      U64 sec_array_raw_opl = section_array_offset + coff_header.section_count*sizeof(COFF_SectionHeader);
      U64 sec_array_opl = ClampTop(sec_array_raw_opl, dim_1u64(vaddr_range));
      U64 section_count = (sec_array_opl - section_array_offset)/sizeof(COFF_SectionHeader);

      COFF_SectionHeader *loaded_sec_array = push_array(scratch.arena, COFF_SectionHeader, section_count);
      dmn_process_read(process.dmn_handle, rng_1u64(vaddr_range.min+section_array_offset,vaddr_range.min+section_array_offset+sizeof(COFF_SectionHeader)*section_count), loaded_sec_array);

      // rjf: find number of data directories
      U32 data_dir_max = (opt_ext_size - reported_data_dir_offset) / sizeof(PE_DataDirectory);
      data_dir_count = ClampTop(reported_data_dir_count, data_dir_max);
      
      // antoniom: detect if ARM64EC binary
      COFF_MachineType machine_type = coff_header.machine;
      PE_LoadConfig64 load_config_header = {0};

      U64 pdata_voff = 0;
      U64 pdata_size = 0;
      B32 is_arm64ec = 0;

      if (PE_DataDirectoryIndex_LOAD_CONFIG < data_dir_count)
      {
        U64 dir_offset = vaddr_range.min + optional_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_LOAD_CONFIG;
        PE_DataDirectory data_dir = {0};
        dmn_process_read_struct(process.dmn_handle, dir_offset, &data_dir);

        switch(optional_magic)
        {
          case PE_PE32PLUS_MAGIC:
          {
            dmn_process_read_struct(process.dmn_handle, vaddr_range.min + data_dir.virt_off, &load_config_header);
          }break;
          case PE_PE32_MAGIC:
          {
            PE_LoadConfig32 load_config_header32 = {0};
            dmn_process_read_struct(process.dmn_handle, vaddr_range.min + data_dir.virt_off, &load_config_header32);
            load_config_header.size = load_config_header32.size;
            load_config_header.chpe_metadata_ptr = load_config_header32.chpe_metadata_ptr;
          }break;
        }
      }

      if (load_config_header.size >= OffsetOf(PE_LoadConfig64, chpe_metadata_ptr) && load_config_header.chpe_metadata_ptr != 0)
      {
        machine_type = COFF_MachineType_ARM64;

        PE_ARM64ECMetadata arm64ec_metadata = {0};
        dmn_process_read_struct(process.dmn_handle, load_config_header.chpe_metadata_ptr, &arm64ec_metadata);

        for(U32 section_idx = 0; section_idx < section_count; section_idx += 1)
        {
          COFF_SectionHeader *cur_sec = loaded_sec_array + section_idx;
          String8 cur_sec_name = str8_cstring_capped(cur_sec->name, cur_sec->name + sizeof(cur_sec->name));
          if(str8_match(cur_sec_name, str8_lit(".pdata"), 0))
          {
            pdata_voff = cur_sec->voff;
            pdata_size = cur_sec->vsize;
            is_arm64ec = 1;
            break;
          }
        }
        // TODO(antonio): get hybrid code ranges for arm64ec
      }

      // rjf: grab pdatas from exceptions section
      if(data_dir_count > PE_DataDirectoryIndex_EXCEPTIONS)
      {
        PE_DataDirectory dir = {0};
        dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_EXCEPTIONS, &dir);
        Rng1U64 pdatas_voff_range = r1u64((U64)dir.virt_off, (U64)dir.virt_off + (U64)dir.virt_size);
        if(is_arm64ec)
        {
          pdatas_voff_range = r1u64(pdata_voff, pdata_voff + pdata_size);
        }

        switch(machine_type)
        {
          case COFF_MachineType_X86:
          case COFF_MachineType_X64:
          {
            intel_pdatas_count = dim_1u64(pdatas_voff_range)/sizeof(PE_IntelPdata);
            intel_pdatas = push_array(arena, PE_IntelPdata, intel_pdatas_count);
            dmn_process_read(process.dmn_handle, r1u64(vaddr_range.min + pdatas_voff_range.min, vaddr_range.min + pdatas_voff_range.max), intel_pdatas);
          }break;

          case COFF_MachineType_ARM64:
          {
            arm64_pdatas_count = dim_1u64(pdatas_voff_range)/sizeof(PE_Arm64Pdata);
            arm64_pdatas = push_array(arena, PE_Arm64Pdata, arm64_pdatas_count);
            dmn_process_read(process.dmn_handle, r1u64(vaddr_range.min + pdatas_voff_range.min, vaddr_range.min + pdatas_voff_range.max), arm64_pdatas);
          }break;
        }
      }
      
      // rjf: extract tls header
      PE_TLSHeader64 tls_header = {0};
      if(data_dir_count > PE_DataDirectoryIndex_TLS)
      {
        PE_DataDirectory dir = {0};
        dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_TLS, &dir);
        Rng1U64 tls_voff_range = r1u64((U64)dir.virt_off, (U64)dir.virt_off + (U64)dir.virt_size);
        switch(machine_type)
        {
          default:{}break;
          case COFF_MachineType_X86:
          {
            PE_TLSHeader32 tls_header32 = {0};
            dmn_process_read_struct(process.dmn_handle, vaddr_range.min + tls_voff_range.min, &tls_header32);
            tls_header.raw_data_start    = (U64)tls_header32.raw_data_start;
            tls_header.raw_data_end      = (U64)tls_header32.raw_data_end;
            tls_header.index_address     = (U64)tls_header32.index_address;
            tls_header.callbacks_address = (U64)tls_header32.callbacks_address;
            tls_header.zero_fill_size    = (U64)tls_header32.zero_fill_size;
            tls_header.characteristics   = (U64)tls_header32.characteristics;
          }break;
          case COFF_MachineType_ARM64:
          case COFF_MachineType_X64:
          {
            dmn_process_read_struct(process.dmn_handle, vaddr_range.min + tls_voff_range.min, &tls_header);
          }break;
        }
      }
      
      // rjf: grab entry point vaddr
      entry_point_voff = entry_point;
      
      // rjf: calculate TLS vaddr range
      tls_vaddr_range = r1u64(tls_header.index_address, tls_header.index_address+sizeof(U32));
      
      // rjf: grab data about debug info
      if(data_dir_count > PE_DataDirectoryIndex_DEBUG)
      {
        // rjf: read data dir
        PE_DataDirectory dir = {0};
        dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_DEBUG, &dir);
        
        U64 dbg_dir_count = dir.virt_size / sizeof(PE_DebugDirectory);
        for(U64 dbg_dir_idx = 0; dbg_dir_idx < dbg_dir_count; dbg_dir_idx += 1)
        {
          // rjf: read debug directory
          U64 dir_addr = vaddr_range.min + dir.virt_off + dbg_dir_idx * sizeof(PE_DebugDirectory);
          PE_DebugDirectory dbg_data = {0};
          dmn_process_read_struct(process.dmn_handle, dir_addr, &dbg_data);
          
          // rjf: extract external file info from codeview header
          if(dbg_data.type == PE_DebugDirectoryType_CODEVIEW)
          {
            U32 cv_magic = 0;
            dmn_process_read_struct(process.dmn_handle, vaddr_range.min + dbg_data.voff, &cv_magic);
            switch(cv_magic)
            {
              default:break;
              case PE_CODEVIEW_PDB20_MAGIC:
              {
                PE_CvHeaderPDB20 cv;
                U64 read_size = dmn_process_read_struct(process.dmn_handle, vaddr_range.min+dbg_data.voff, &cv);
                if(read_size == sizeof(cv))
                {
                  pdb_dbg_time = cv.time_stamp;
                  pdb_dbg_age = cv.age;
                  pdb_dbg_path = dmn_process_read_cstring(arena, process.dmn_handle, vaddr_range.min + dbg_data.voff + sizeof(cv));
                }
              }break;
              case PE_CODEVIEW_PDB70_MAGIC:
              {
                PE_CvHeaderPDB70 cv;
                U64 read_size = dmn_process_read_struct(process.dmn_handle, vaddr_range.min + dbg_data.voff, &cv);
                if(read_size == sizeof(cv))
                {
                  pdb_dbg_guid = cv.guid;
                  pdb_dbg_age = cv.age;
                  pdb_dbg_path = dmn_process_read_cstring(arena, process.dmn_handle, vaddr_range.min + dbg_data.voff + sizeof(cv));
                }
              }break;
              case PE_CODEVIEW_RDI_MAGIC:
              {
                PE_CvHeaderRDI cv;
                U64 read_size = dmn_process_read_struct(process.dmn_handle, vaddr_range.min + dbg_data.voff, &cv);
                if(read_size == sizeof(cv))
                {
                  rdi_dbg_guid = cv.guid;
                  rdi_dbg_path = dmn_process_read_cstring(arena, process.dmn_handle, vaddr_range.min + dbg_data.voff + sizeof(cv));
                }
              }break;
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: pick default initial debug info path
  //
  String8 initial_debug_info_path = str8_zero();
  {
    Temp scratch = scratch_begin(0, 0);
    String8 exe_folder = str8_chop_last_slash(path);
    String8List dbg_path_candidates = {0};
    //
    //~ TODO(rjf): @linux_port PLEASE READ RYAN vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    //
    // TODO(rjf): trying "exe_folder/embedded_path" as the first option is only a valid
    // heuristic on Windows, because we know that two absolute paths concatted together
    // are necessarily invalid. however, on Linux, this is not the case - you could stitch
    // two paths together and get a third path that is completely valid. so, in that case,
    // we will need to infer if the path is relative, and then use either the embedded
    // path as-is, or the exe-relative-path accordingly, depending on that.
    //
    if(rdi_dbg_path.size != 0)
    {
      str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S/%S", exe_folder, rdi_dbg_path);
      str8_list_push(scratch.arena,  &dbg_path_candidates, rdi_dbg_path);
    }
    if(pdb_dbg_path.size != 0)
    {
      str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S/%S", exe_folder, pdb_dbg_path);
      str8_list_push(scratch.arena,  &dbg_path_candidates, pdb_dbg_path);
    }
    str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S.pdb", str8_chop_last_dot(path));
    str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S.pdb", path);
    str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S.rdi", str8_chop_last_dot(path));
    str8_list_pushf(scratch.arena, &dbg_path_candidates, "%S.rdi", path);
    for(String8Node *n = dbg_path_candidates.first; n != 0; n = n->next)
    {
      String8 candidate_path = n->string;
      FileProperties props = os_properties_from_file_path(candidate_path);
      if(props.modified != 0 && props.size != 0)
      {
        initial_debug_info_path = push_str8_copy(arena, candidate_path);
        break;
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: insert info into cache
  //
  {
    U64 hash = ctrl_hash_from_handle(module);
    U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
    U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
    CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
    CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
    OS_MutexScopeW(stripe->rw_mutex)
    {
      CTRL_ModuleImageInfoCacheNode *node = 0;
      for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->module, module))
        {
          node = n;
          break;
        }
      }
      if(!node)
      {
        node = push_array(arena, CTRL_ModuleImageInfoCacheNode, 1);
        DLLPushBack(slot->first, slot->last, node);
        node->module = module;
        node->arena = arena;
        node->intel_pdatas = intel_pdatas;
        node->intel_pdatas_count = intel_pdatas_count;
        node->arm64_pdatas = arm64_pdatas;
        node->arm64_pdatas_count = arm64_pdatas_count;
        node->entry_point_voff = entry_point_voff;
        node->initial_debug_info_path = initial_debug_info_path;
      }
    }
  }
  scratch_end(scratch);
}

internal void
ctrl_thread__module_close(CTRL_Handle module)
{
  //////////////////////////////
  //- rjf: evict module image info from cache
  //
  {
    U64 hash = ctrl_hash_from_handle(module);
    U64 slot_idx = hash%ctrl_state->module_image_info_cache.slots_count;
    U64 stripe_idx = slot_idx%ctrl_state->module_image_info_cache.stripes_count;
    CTRL_ModuleImageInfoCacheSlot *slot = &ctrl_state->module_image_info_cache.slots[slot_idx];
    CTRL_ModuleImageInfoCacheStripe *stripe = &ctrl_state->module_image_info_cache.stripes[stripe_idx];
    OS_MutexScopeW(stripe->rw_mutex)
    {
      CTRL_ModuleImageInfoCacheNode *node = 0;
      for(CTRL_ModuleImageInfoCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->module, module))
        {
          node = n;
          break;
        }
      }
      if(node)
      {
        DLLRemove(slot->first, slot->last, node);
        arena_release(node->arena);
      }
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
      
      // rjf: log event
      if(next_event_node != 0)
      {
        DMN_Event *ev = &next_event_node->v;
        LogInfoNamedBlockF("dmn_event")
        {
          log_infof("kind:           %S\n",       dmn_event_kind_string_table[ev->kind]);
          log_infof("exception_kind: %S\n",       dmn_exception_kind_string_table[ev->exception_kind]);
          log_infof("process:        [%I64u]\n",  ev->process.u64[0]);
          log_infof("thread:         [%I64u]\n",  ev->thread.u64[0]);
          log_infof("module:         [%I64u]\n",  ev->module.u64[0]);
          log_infof("arch:           %S\n",       string_from_arch(ev->arch));
          log_infof("address:        0x%I64x\n",  ev->address);
          log_infof("string:         \"%S\"\n",   ev->string);
          log_infof("ip_vaddr:       0x%I64x\n",  ev->instruction_pointer);
        }
      }
      
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
              DI_Scope *di_scope = di_scope_open();
              CTRL_Entity *process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, ev->process));
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
                CTRL_Entity *dbg_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
                DI_Key dbgi_key = {dbg_path->string, dbg_path->timestamp};
                RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, max_U64);
                RDI_NameMap *unparsed_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_GlobalVariables);
                {
                  RDI_ParsedNameMap map = {0};
                  rdi_parsed_from_name_map(rdi, unparsed_map, &map);
                  String8 name = str8_lit("__asan_shadow_memory_dynamic_address");
                  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                  if(node != 0)
                  {
                    U32 id_count = 0;
                    U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                    if(id_count > 0)
                    {
                      RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, ids[0]);
                      U64 global_var_voff = global_var->voff;
                      U64 global_var_vaddr = global_var->voff + module->vaddr_range.min;
                      Arch arch = process->arch;
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
              
              di_scope_close(di_scope);
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
        CTRL_Entity *spoof_process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, spoof->process));
        Arch arch = spoof_process->arch;
        switch(arch)
        {
          default: {}break;
          case Arch_x64:
          case Arch_x86:
          {
            size_of_spoof = bit_size_from_arch(arch)/8;
            dmn_process_read(spoof_process->handle.dmn_handle, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
          }break;
        }
      }
      
      // rjf: set spoof
      if(do_spoof) ProfScope("set spoof")
      {
        CTRL_Entity *spoof_process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, spoof->process));
        Arch arch = spoof_process->arch;
        switch(arch)
        {
          default: {}break;
          case Arch_x64:
          case Arch_x86:
          {
            dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof->new_ip_value);
          }break;
        }
      }
      
      // rjf: run for new events
      ProfScope("run for new events")
      {
        LogInfoNamedBlockF("dmn_ctrl_run")
        {
          log_infof("single_step_thread:         [0x%I64x]\n", run_ctrls->single_step_thread);
          log_infof("ignore_previous_exception:  %i\n", !!run_ctrls->ignore_previous_exception);
          log_infof("run_entities_are_unfrozen:  %i\n", !!run_ctrls->run_entities_are_unfrozen);
          log_infof("run_entities_are_processes: %i\n", !!run_ctrls->run_entities_are_processes);
          log_infof("run_entity_count:           %I64u\n", run_ctrls->run_entity_count);
          LogInfoNamedBlockF("run_entities") for(U64 idx = 0; idx < run_ctrls->run_entity_count; idx += 1)
          {
            log_infof("[0x%I64x]\n", run_ctrls->run_entities[idx]);
          }
          log_infof("trap_count:                 %I64u\n", run_ctrls->traps.trap_count);
          LogInfoNamedBlockF("traps") for(DMN_TrapChunkNode *n = run_ctrls->traps.first; n != 0; n = n->next)
          {
            for(U64 idx = 0; idx < n->count; idx += 1)
            {
              log_infof("{process:[0x%I64x], vaddr:0x%I64x, id:0x%I64x}\n", n->v[idx].process.u64[0], n->v[idx].vaddr, n->v[idx].id);
            }
          }
        }
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
        CTRL_Entity *spoof_process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, spoof->process));
        Arch arch = spoof_process->arch;
        switch(arch)
        {
          default: {}break;
          case Arch_x64:
          {
            dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
          }break;
        }
      }
    }
  }
  
  //- rjf: irrespective of what event came back, we should ALWAYS check the
  // spoof's thread and see if it hit the spoof address, because we may have
  // simply been sent other debug events first
  if(spoof != 0)
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, spoof->thread));
    Arch arch = thread->arch;
    void *regs_block = push_array(scratch.arena, U8, regs_block_size_from_arch(arch));
    dmn_thread_read_reg_block(spoof->thread, regs_block);
    U64 spoof_thread_rip = regs_rip_from_arch_block(arch, regs_block);
    if(spoof_thread_rip == spoof->new_ip_value)
    {
      regs_arch_block_write_rip(arch, regs_block, spoof_old_ip_value);
      ctrl_thread_write_reg_block(ctrl_handle_make(CTRL_MachineID_Local, spoof->thread), regs_block);
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
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->arch       = event->arch;
      out_evt->entity_id  = event->code;
      ctrl_state->process_counter += 1;
    }break;
    case DMN_EventKind_CreateThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_NewThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->arch       = event->arch;
      out_evt->entity_id  = event->code;
      out_evt->stack_base = dmn_stack_base_vaddr_from_thread(event->thread);
      out_evt->tls_root   = dmn_tls_root_vaddr_from_thread(event->thread);
      out_evt->rip_vaddr  = event->instruction_pointer;
      out_evt->string     = event->string;
    }break;
    case DMN_EventKind_LoadModule:
    {
      CTRL_Handle process_handle = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      CTRL_Handle module_handle = ctrl_handle_make(CTRL_MachineID_Local, event->module);
      CTRL_Event *out_evt1 = ctrl_event_list_push(scratch.arena, &evts);
      String8 module_path = path_normalized_from_string(scratch.arena, event->string);
      U64 exe_timestamp = os_properties_from_file_path(module_path).modified;
      ctrl_thread__module_open(process_handle, module_handle, r1u64(event->address, event->address+event->size), module_path);
      out_evt1->kind       = CTRL_EventKind_NewModule;
      out_evt1->msg_id     = msg->msg_id;
      out_evt1->entity     = module_handle;
      out_evt1->parent     = process_handle;
      out_evt1->arch       = event->arch;
      out_evt1->entity_id  = event->code;
      out_evt1->vaddr_rng  = r1u64(event->address, event->address+event->size);
      out_evt1->rip_vaddr  = event->address;
      out_evt1->timestamp  = exe_timestamp;
      out_evt1->string     = module_path;
      CTRL_Event *out_evt2 = ctrl_event_list_push(scratch.arena, &evts);
      String8 initial_debug_info_path = ctrl_initial_debug_info_path_from_module(scratch.arena, module_handle);
      U64 debug_info_timestamp = os_properties_from_file_path(initial_debug_info_path).modified;
      out_evt2->kind       = CTRL_EventKind_ModuleDebugInfoPathChange;
      out_evt2->msg_id     = msg->msg_id;
      out_evt2->entity     = module_handle;
      out_evt2->parent     = process_handle;
      out_evt2->timestamp  = debug_info_timestamp;
      out_evt2->string     = initial_debug_info_path;
      DI_Key initial_dbgi_key = {initial_debug_info_path, debug_info_timestamp};
      di_open(&initial_dbgi_key);
    }break;
    case DMN_EventKind_ExitProcess:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndProc;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->u64_code   = event->code;
      ctrl_state->process_counter -= 1;
    }break;
    case DMN_EventKind_ExitThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->entity_id  = event->code;
    }break;
    case DMN_EventKind_UnloadModule:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      CTRL_Handle module_handle = ctrl_handle_make(CTRL_MachineID_Local, event->module);
      String8 module_path = event->string;
      ctrl_thread__module_close(module_handle);
      out_evt->kind       = CTRL_EventKind_EndModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = module_handle;
      out_evt->string     = module_path;
      CTRL_Entity *module_ent = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, module_handle);
      CTRL_Entity *debug_info_path_ent = ctrl_entity_child_from_kind(module_ent, CTRL_EntityKind_DebugInfoPath);
      if(debug_info_path_ent != &ctrl_entity_nil)
      {
        DI_Key dbgi_key = {debug_info_path_ent->string, debug_info_path_ent->timestamp};
        di_close(&dbgi_key);
      }
    }break;
    case DMN_EventKind_DebugString:
    {
      U64 num_strings = (event->string.size + ctrl_state->c2u_ring_max_string_size-1) / ctrl_state->c2u_ring_max_string_size;
      for(U64 string_idx = 0; string_idx < num_strings; string_idx += 1)
      {
        CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
        out_evt->kind       = CTRL_EventKind_DebugString;
        out_evt->msg_id     = msg->msg_id;
        out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
        out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
        out_evt->string     = str8_substr(event->string, r1u64(string_idx*ctrl_state->c2u_ring_max_string_size, (string_idx+1)*ctrl_state->c2u_ring_max_string_size));
      }
    }break;
    case DMN_EventKind_SetThreadName:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_ThreadName;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->string     = event->string;
      out_evt->entity_id  = event->code;
    }break;
  }
  ctrl_c2u_push_events(&evts);
  
  //- rjf: if this is the first process in a session, clear the debug directory
  // cache state
  if(ctrl_state->process_counter == 1 && event->kind == DMN_EventKind_CreateProcess)
  {
    arena_clear(ctrl_state->dbg_dir_arena);
    ctrl_state->dbg_dir_root = push_array(ctrl_state->dbg_dir_arena, CTRL_DbgDirNode, 1);
  }
  
  //- rjf: when a new module is loaded, pre-emptively try to open all adjacent
  // debug infos. with debug events, we learn about loaded modules serially,
  // and we need to completely load debug info before continuing. for massive
  // projects, this is a problem, because completely loading debug info isn't a
  // trivial cost, and there are often 1000s of DLLs.
  //
  // an imperfect but usually reasonable heuristic is to look at adjacent
  // debug info files, in the same or under the directory as the initially
  // loaded, and pre-emptively convert all of them (which for us is the
  // heaviest part of debug info loading, if native RDI is not used).
  //
  // only do this on the first ever loaded module, *or* once we get beyond 256
  // modules (a very bad heuristic that may or may not inform us that we are
  // dealing with insane-town projects)
  //
  if(event->kind == DMN_EventKind_LoadModule &&
     (ctrl_state->ctrl_thread_entity_store->entity_kind_counts[CTRL_EntityKind_Module] > 256 ||
      ctrl_state->ctrl_thread_entity_store->entity_kind_counts[CTRL_EntityKind_Module] == 1))
  {
    U64 endt_us = os_now_microseconds() + 1000000;
    
    //- rjf: unpack event
    CTRL_Handle process_handle = ctrl_handle_make(CTRL_MachineID_Local, event->process);
    CTRL_Handle loaded_module_handle = ctrl_handle_make(CTRL_MachineID_Local, event->module);
    CTRL_Entity *process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, process_handle);
    CTRL_Entity *loaded_module = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, loaded_module_handle);
    
    //- rjf: for each module, use its full path as the start to a new limited recursive
    // directory search. cache each directory once traversed in the dbg_dir tree. if any
    // node is not cached, then scan it & pre-emptively convert debug info.
    ProfScope("pre-emptively load adjacent debug info for %.*s", str8_varg(loaded_module->string))
    {
      //- rjf: calculate seed path
      DI_Key loaded_di_key = ctrl_dbgi_key_from_module(loaded_module);
      String8 loaded_di_name = str8_skip_last_slash(loaded_di_key.path);
      String8 debug_info_ext = str8_skip_last_dot(loaded_di_key.path);
      String8 seed_folder_path = str8_chop_last_slash(loaded_di_key.path);
      if(seed_folder_path.size == 0)
      {
        String8 module_path = loaded_module->string;
        seed_folder_path = str8_chop_last_slash(module_path);
      }
      
      //- rjf: split seed path
      String8List seed_path_parts = str8_split_path(scratch.arena, seed_folder_path);
      
      //- rjf: find parent dir node for this module's debug info; build tree leading to this dir
      CTRL_DbgDirNode *parent_dir_node = ctrl_state->dbg_dir_root;
      for(String8Node *n = seed_path_parts.first; n != 0; n = n->next)
      {
        String8 name = n->string;
        CTRL_DbgDirNode *next_child = 0;
        for(CTRL_DbgDirNode *child = parent_dir_node->first; child != 0; child = child->next)
        {
          if(str8_match(child->name, name, StringMatchFlag_CaseInsensitive))
          {
            next_child = child;
            break;
          }
        }
        if(next_child == 0)
        {
          next_child = push_array(ctrl_state->dbg_dir_arena, CTRL_DbgDirNode, 1);
          DLLPushBack(parent_dir_node->first, parent_dir_node->last, next_child);
          next_child->parent = parent_dir_node;
          next_child->name = push_str8_copy(ctrl_state->dbg_dir_arena, name);
          parent_dir_node->child_count += 1;
        }
        parent_dir_node = next_child;
      }
      
      //- rjf: count modules
      {
        parent_dir_node->module_direct_count += 1;
      }
      
      //- rjf: iterate from dir node up its ancestor chain - do recursive
      // searches if this is an ancestor of loaded modules, it has not been
      // searched yet, but it has >4 child branches, meaning it looks like
      // project directory
      //
      DI_KeyList preemptively_loaded_keys = {0};
      for(CTRL_DbgDirNode *dir_node = parent_dir_node; dir_node != 0; dir_node = dir_node->parent)
      {
        if(dir_node->search_count == 0 && dir_node->module_direct_count >= 1)
        {
          //- rjf: form full path of this directory node
          String8List dir_node_path_parts = {0};
          for(CTRL_DbgDirNode *n = dir_node; n != 0; n = n->parent)
          {
            if(n->name.size != 0)
            {
              str8_list_push_front(scratch.arena, &dir_node_path_parts, n->name);
            }
          }
          String8 dir_node_path = str8_list_join(scratch.arena, &dir_node_path_parts, &(StringJoin){.sep = str8_lit("/")});
          
          //- rjf: iterate downwards from this directory recursively, locate
          // debug infos, and pre-emptively convert
          typedef struct Task Task;
          struct Task
          {
            Task *next;
            CTRL_DbgDirNode *node;
            String8 path;
          };
          Task start_task = {0, dir_node, dir_node_path};
          Task *first_task = &start_task;
          Task *last_task = first_task;
          U64 task_count = 0;
          for(Task *t = first_task; t != 0; t = t->next)
          {
            ProfBegin("search task %.*s", str8_varg(t->path));
            
            // rjf: increment search counter
            t->node->search_count += 1;
            
            // rjf: iterate this directory. if debug infos are encountered,
            // kick off pre-emptive conversion, and gather key. if folders
            // are encountered, then add them to the tree, and kick off a
            // sub-search if needed.
            OS_FileIter *it = os_file_iter_begin(scratch.arena, t->path, 0);
            U64 idx = 0;
            for(OS_FileInfo info = {0}; idx < 16384 && os_file_iter_next(scratch.arena, it, &info); idx += 1)
            {
              // rjf: folder -> do sub-search if not duplicative
              if(info.props.flags & FilePropertyFlag_IsFolder && task_count < 16384 && !str8_match(str8_prefix(info.name, 1), str8_lit("."), 0))
              {
                CTRL_DbgDirNode *existing_dir_child = 0;
                for(CTRL_DbgDirNode *child = t->node->first; child != 0; child = child->next)
                {
                  if(str8_match(child->name, info.name, StringMatchFlag_CaseInsensitive))
                  {
                    existing_dir_child = child;
                    break;
                  }
                }
                if(existing_dir_child == 0)
                {
                  existing_dir_child = push_array(ctrl_state->dbg_dir_arena, CTRL_DbgDirNode, 1);
                  DLLPushBack(t->node->first, t->node->last, existing_dir_child);
                  existing_dir_child->parent = t->node;
                  existing_dir_child->name = push_str8_copy(ctrl_state->dbg_dir_arena, info.name);
                  t->node->child_count += 1;
                }
                if(existing_dir_child->search_count == 0)
                {
                  Task *task = push_array(scratch.arena, Task, 1);
                  task->node = existing_dir_child;
                  task->path = push_str8f(scratch.arena, "%S/%S", t->path, info.name);
                  SLLQueuePush(first_task, last_task, task);
                  task_count += 1;
                }
              }
              
              // rjf: debug info file -> kick off open
              else if(preemptively_loaded_keys.count < 4096 &&
                      !(info.props.flags & FilePropertyFlag_IsFolder) &&
                      str8_match(str8_skip_last_dot(info.name), debug_info_ext, StringMatchFlag_CaseInsensitive) &&
                      !str8_match(loaded_di_name, info.name, StringMatchFlag_CaseInsensitive))
              {
                DI_Key key = {push_str8f(scratch.arena, "%S/%S", t->path, info.name), info.props.modified};
                di_open(&key);
                di_key_list_push(scratch.arena, &preemptively_loaded_keys, &key);
              }
            }
            os_file_iter_end(it);
            ProfEnd();
          }
        }
      }
      
      //- rjf: for each pre-emptively loaded key, wait for the initial
      // load task to be done
      for(DI_KeyNode *n = preemptively_loaded_keys.first; n != 0; n = n->next)
      {
        DI_Scope *di_scope = di_scope_open();
        RDI_Parsed *rdi = di_rdi_from_key(di_scope, &n->v, endt_us);
        di_scope_close(di_scope);
        di_close(&n->v);
      }
    }
  }
  
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
ctrl_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range)
{
  B32 result = 0;
  switch(space.kind)
  {
    default:{}break;
    
    //- rjf: intra-entity reads (process memory or thread registers)
    case CTRL_EvalSpaceKind_Entity:
    {
      CTRL_Entity *entity = (CTRL_Entity *)space.u64_0;
      switch(entity->kind)
      {
        default:{}break;
        case CTRL_EntityKind_Process:
        {
          U64 read_size = dmn_process_read(entity->handle.dmn_handle, range, out);
          result = (read_size == dim_1u64(range));
        }break;
        case CTRL_EntityKind_Thread:
        {
          Temp scratch = scratch_begin(0, 0);
          U64 regs_size = regs_block_size_from_arch(entity->arch);
          void *regs = ctrl_query_cached_reg_block_from_thread(scratch.arena, ctrl_state->ctrl_thread_entity_store, entity->handle);
          Rng1U64 legal_range = r1u64(0, regs_size);
          Rng1U64 read_range = intersect_1u64(legal_range, range);
          U64 read_size = dim_1u64(read_range);
          MemoryCopy(out, (U8 *)regs + read_range.min, read_size);
          result = (read_size == dim_1u64(range));
          scratch_end(scratch);
        }break;
      }
    }break;
    
    //- rjf: meta evaluations
    case CTRL_EvalSpaceKind_Meta:
    {
      Temp scratch = scratch_begin(0, 0);
      U64 meta_eval_idx = space.u64s[0];
      if(meta_eval_idx < ctrl_state->user_meta_evals.count)
      {
        CTRL_MetaEval *meval = &ctrl_state->user_meta_evals.v[meta_eval_idx];
        
        // rjf: copy meta evaluation to scratch arena, to form range of legal reads
        arena_push(scratch.arena, 0, 64);
        String8 meval_srlzed = serialized_from_struct(scratch.arena, CTRL_MetaEval, meval);
        U64 pos_min = arena_pos(scratch.arena);
        CTRL_MetaEval *meval_read = struct_from_serialized(scratch.arena, CTRL_MetaEval, meval_srlzed);
        U64 pos_opl = arena_pos(scratch.arena);
        
        // rjf: rebase all pointer values in meta evaluation to be relative to base pointer
        struct_rebase_ptrs(CTRL_MetaEval, meval_read, meval_read);
        
        // rjf: perform actual read
        Rng1U64 legal_range = r1u64(0, pos_opl-pos_min);
        if(contains_1u64(legal_range, range.min))
        {
          result = 1;
          U64 range_dim = dim_1u64(range);
          U64 bytes_to_read = Min(range_dim, (legal_range.max - range.min));
          MemoryCopy(out, ((U8 *)meval_read) + range.min, bytes_to_read);
          if(bytes_to_read < range_dim)
          {
            MemoryZero((U8 *)out + bytes_to_read, range_dim - bytes_to_read);
          }
        }
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

//- rjf: log flusher

internal void
ctrl_thread__flush_info_log(String8 string)
{
  os_append_data_to_file_path(ctrl_state->ctrl_thread_log_path, string);
}

internal void
ctrl_thread__end_and_flush_info_log(void)
{
  Temp scratch = scratch_begin(0, 0);
  LogScopeResult log = log_scope_end(scratch.arena);
  ctrl_thread__flush_info_log(log.strings[LogMsgKind_Info]);
  scratch_end(scratch);
}

//- rjf: msg kind implementations

internal void
ctrl_thread__launch(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  //- rjf: obtain stdout/stderr/stdin handles
  OS_Handle stdout_handle = {0};
  OS_Handle stderr_handle = {0};
  OS_Handle stdin_handle  = {0};
  if(msg->stdout_path.size != 0)
  {
    OS_Handle f = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Read, msg->stdout_path);
    os_file_close(f);
    stdout_handle = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Append|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, msg->stdout_path);
  }
  if(msg->stderr_path.size != 0)
  {
    OS_Handle f = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Read, msg->stderr_path);
    os_file_close(f);
    stderr_handle = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Append|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, msg->stderr_path);
  }
  if(msg->stdin_path.size != 0)
  {
    stdin_handle = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, msg->stdin_path);
  }
  
  //- rjf: launch
  OS_ProcessLaunchParams params = {0};
  {
    params.cmd_line           = msg->cmd_line_string_list;
    params.path               = msg->path;
    params.env                = msg->env_string_list;
    params.inherit_env        = msg->env_inherit;
    params.debug_subprocesses = msg->debug_subprocesses;
    params.stdout_file        = stdout_handle;
    params.stderr_file        = stderr_handle;
    params.stdin_file         = stdin_handle;
  }
  U32 id = dmn_ctrl_launch(ctrl_ctx, &params);
  
  //- rjf: close stdout/stderr/stdin files
  os_file_close(stdout_handle);
  os_file_close(stderr_handle);
  os_file_close(stdin_handle);
  
  //- rjf: record (id -> entry points), so that we know custom entry points for this PID
  for(String8Node *n = msg->entry_points.first; n != 0; n = n->next)
  {
    String8 string = n->string;
    CTRL_Entity *entry = ctrl_entity_alloc(ctrl_state->ctrl_thread_entity_store, ctrl_state->ctrl_thread_entity_store->root, CTRL_EntityKind_EntryPoint, Arch_Null, ctrl_handle_zero(), (U64)id);
    ctrl_entity_equip_string(ctrl_state->ctrl_thread_entity_store, entry, string);
  }
}

internal void
ctrl_thread__attach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
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
        case DMN_EventKind_Halt:
        case DMN_EventKind_Exception:
        case DMN_EventKind_Error:
        case DMN_EventKind_HandshakeComplete:
        {
          done = 1;
        }break;
      }
    }
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind       = CTRL_EventKind_Stopped;
    event->cause      = CTRL_EventCause_Finished;
    event->msg_id     = msg->msg_id;
    event->entity_id = !!attach_successful * msg->entity_id;
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__kill(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DMN_Handle process = msg->entity.dmn_handle;
  U32 exit_code = msg->exit_code;
  
  //- rjf: send kill
  B32 kill_worked = dmn_ctrl_kill(ctrl_ctx, process, exit_code);
  
  //- rjf: wait for process to be dead
  CTRL_EventCause cause = CTRL_EventCause_Finished;
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
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_ExitProcess:
        if(dmn_handle_match(event->process, process))
        {
          done = 1;
        }break;
        case DMN_EventKind_Error:{done = 1; cause = CTRL_EventCause_Error;}break;
        case DMN_EventKind_Halt: {done = 1; cause = CTRL_EventCause_InterruptedByHalt;}break;
      }
    }
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind       = CTRL_EventKind_Stopped;
    event->cause      = cause;
    event->msg_id     = msg->msg_id;
    if(kill_worked)
    {
      event->entity = msg->entity;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__kill_all(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  U32 exit_code = msg->exit_code;
  
  //- rjf: gather all currently existing processes
  CTRL_EntityList initial_processes = ctrl_entity_list_from_kind(ctrl_state->ctrl_thread_entity_store, CTRL_EntityKind_Process);
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    Task *prev;
    CTRL_Entity *process;
  };
  Task *first_task = 0;
  Task *last_task = 0;
  for(CTRL_EntityNode *n = initial_processes.first; n != 0; n = n->next)
  {
    Task *t = push_array(scratch.arena, Task, 1);
    t->process = n->v;
    DLLPushBack(first_task, last_task, t);
  }
  
  //- rjf: kill processes as needed, wait for all processes to be dead
  CTRL_EventCause cause = CTRL_EventCause_Finished;
  if(first_task != 0)
  {
    DMN_RunCtrls run_ctrls = {0};
    for(B32 done = 0; !done;)
    {
      // rjf: kill remaining processes
      for(Task *t = first_task, *next = 0; t != 0; t = next)
      {
        next = t->next;
        B32 kill_worked = dmn_ctrl_kill(ctrl_ctx, t->process->handle.dmn_handle, exit_code);
        if(kill_worked)
        {
          DLLRemove(first_task, last_task, t);
        }
      }
      
      // rjf: get next event
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      
      // rjf: process event
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_CreateProcess:
        {
          CTRL_Entity *new_process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, event->process));
          Task *t = push_array(scratch.arena, Task, 1);
          t->process = new_process;
          DLLPushBack(first_task, last_task, t);
        }break;
        case DMN_EventKind_Error:{done = 1; cause = CTRL_EventCause_Error;}break;
        case DMN_EventKind_Halt: {done = 1; cause = CTRL_EventCause_InterruptedByHalt;}break;
      }
      
      // rjf: end if all processes are gone
      CTRL_EntityList processes = ctrl_entity_list_from_kind(ctrl_state->ctrl_thread_entity_store, CTRL_EntityKind_Process);
      if(processes.count == 0)
      {
        done = 1;
      }
    }
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind       = CTRL_EventKind_Stopped;
    event->cause      = cause;
    event->msg_id     = msg->msg_id;
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__detach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DMN_Handle process = msg->entity.dmn_handle;
  
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
      if(event->kind == DMN_EventKind_Halt)
      {
        done = 1;
      }
    }
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind       = CTRL_EventKind_Stopped;
    event->cause      = CTRL_EventCause_Finished;
    event->msg_id     = msg->msg_id;
    if(detach_worked)
    {
      event->entity = msg->entity;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__run(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DMN_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  CTRL_Handle target_thread = msg->entity;
  CTRL_Handle target_process = msg->parent;
  CTRL_Entity *target_process_entity = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, target_process);
  //- antoniom: ARM64 requires that instructions be 32-bit aligned
  U64 spoof_ip_vaddr = 912;
  log_infof("ctrl_thread__run:\n{\n");
  
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
        ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, process->handle, module->handle, &msg->user_bps, &user_traps);
      }
      
      // rjf: push virtual-address user breakpoints per-process
      ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, process->handle, &msg->user_bps, &user_traps);
    }
  }
  
  //////////////////////////////
  //- rjf: read initial stack-pointer-check value
  //
  // This MUST happen before any threads move, including single-stepping stuck
  // threads, because otherwise, their stack pointer may change, if single-stepping
  // causes e.g. entrance into a function via a call instruction.
  //
  U64 sp_check_value = dmn_rsp_from_thread(target_thread.dmn_handle);
  log_infof("sp_check_value := 0x%I64x\n", sp_check_value);
  
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
  if(stop_event == 0)
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
          U64 rip = dmn_rip_from_thread(thread->handle.dmn_handle);
          
          // rjf: determine if thread is frozen
          B32 thread_is_frozen = thread->is_frozen;
          
          // rjf: not frozen? -> check if stuck & gather if so
          if(!thread_is_frozen)
          {
            for(DMN_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
            {
              B32 is_on_user_bp = 0;
              for(DMN_Trap *trap_ptr = n->v; trap_ptr < n->v+n->count; trap_ptr += 1)
              {
                if(dmn_handle_match(trap_ptr->process, process->handle.dmn_handle) && trap_ptr->vaddr == rip)
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
              
              if(is_on_user_bp && (!is_on_net_trap || !dmn_handle_match(thread->handle.dmn_handle, target_thread.dmn_handle)))
              {
                dmn_handle_list_push(scratch.arena, &stuck_threads, thread->handle.dmn_handle);
              }
              
              if(is_on_user_bp && is_on_net_trap && dmn_handle_match(thread->handle.dmn_handle, target_thread.dmn_handle))
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
      DMN_Handle thread = node->v;
      U64 thread_pre_rip = dmn_rip_from_thread(thread);
      U64 thread_post_rip = thread_pre_rip;
      for(B32 done = 0; !done;)
      {
        log_infof("single_step_stuck_thread([0x%I64x])\n", thread.u64[0]);
        DMN_RunCtrls run_ctrls = {0};
        run_ctrls.run_entities_are_unfrozen = 1;
        run_ctrls.run_entities = &thread;
        run_ctrls.run_entity_count = 1;
        if(thread_post_rip == thread_pre_rip)
        {
          run_ctrls.single_step_thread = thread;
        }
        DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
        thread_post_rip = dmn_rip_from_thread(thread);
        switch(event->kind)
        {
          default:{}break;
          case DMN_EventKind_ExitThread:
          if(dmn_handle_match(event->thread, thread))
          {
            stop_cause = CTRL_EventCause_Error;
            goto stop;
          }break;
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
            done = dmn_handle_match(node->v, event->thread);
          }break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather frozen threads
  //
  CTRL_EntityList frozen_threads = {0};
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
        if(thread->is_frozen)
        {
          ctrl_entity_list_push(scratch.arena, &frozen_threads, thread);
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
    DMN_Trap trap = {target_process.dmn_handle, node->v.vaddr};
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
    DMN_TrapChunkList entry_traps = {0};
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
      run_ctrls.run_entity_count = frozen_threads.count;
      run_ctrls.run_entities     = push_array(scratch.arena, DMN_Handle, run_ctrls.run_entity_count);
      run_ctrls.run_entities_are_unfrozen = 0;
      {
        U64 idx = 0;
        for(CTRL_EntityNode *n = frozen_threads.first; n != 0; n = n->next)
        {
          run_ctrls.run_entities[idx] = n->v->handle.dmn_handle;
          idx += 1;
        }
      }
      run_ctrls.traps = *trap_list;
      
      //////////////////////////
      //- rjf: get next run-related event
      //
      log_infof("get_next_event:\n{\n");
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, run_spoof);
      log_infof("}\n\n");
      
      //////////////////////////
      //- rjf: determine event handling
      //
      B32 launch_done_first_module = 0;
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
          log_infof("step_rule: unexpected -> hard_stop\n");
        }break;
        case DMN_EventKind_Exception:
        case DMN_EventKind_Breakpoint:
        {
          use_stepping_logic = 1;
          log_infof("step_rule: exception/breakpoint -> stepping_logic\n");
        }break;
        case DMN_EventKind_CreateProcess:
        {
          DMN_TrapChunkList new_traps = {0};
          ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, ctrl_handle_make(CTRL_MachineID_Local, event->process), &msg->user_bps, &new_traps);
          log_infof("step_rule: create_process -> resolve traps\n");
          log_infof("new_traps:\n{\n");
          for(DMN_TrapChunkNode *n = new_traps.first; n != 0; n = n->next)
          {
            for(U64 idx = 0; idx < n->count; idx += 1)
            {
              DMN_Trap *trap = &n->v[idx];
              log_infof("{process:[0x%I64x], vaddr:0x%I64x}\n", trap->process.u64[0], trap->vaddr);
            }
          }
          log_infof("}\n\n");
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &user_traps, &new_traps);
        }break;
        case DMN_EventKind_LoadModule:
        {
          DMN_TrapChunkList new_traps = {0};
          ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, ctrl_handle_make(CTRL_MachineID_Local, event->process), ctrl_handle_make(CTRL_MachineID_Local, event->module), &msg->user_bps, &new_traps);
          log_infof("step_rule: load_module -> resolve traps\n");
          log_infof("new_traps:\n{\n");
          for(DMN_TrapChunkNode *n = new_traps.first; n != 0; n = n->next)
          {
            for(U64 idx = 0; idx < n->count; idx += 1)
            {
              DMN_Trap *trap = &n->v[idx];
              log_infof("{process:[0x%I64x], vaddr:0x%I64x}\n", trap->process.u64[0], trap->vaddr);
            }
          }
          log_infof("}\n\n");
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
          dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &user_traps, &new_traps);
        }break;
      }
      
      //////////////////////////
      //- rjf: on launches, detect entry points, place traps
      //
      if(msg->run_flags & CTRL_RunFlag_StopOnEntryPoint && !launch_done_first_module && event->kind == DMN_EventKind_HandshakeComplete)
      {
        launch_done_first_module = 1;
        DI_Scope *di_scope = di_scope_open();
        
        //- rjf: unpack process/module info
        CTRL_Entity *process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, event->process));
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
        CTRL_Entity *dbg_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
        DI_Key dbgi_key = {dbg_path->string, dbg_path->timestamp};
        RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, max_U64);
        RDI_NameMap *unparsed_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Procedures);
        RDI_ParsedNameMap map = {0};
        rdi_parsed_from_name_map(rdi, unparsed_map, &map);
        
        //- rjf: add traps for user-specified entry points on this message, if specified
        B32 entries_found = 0;
        if(!entries_found)
        {
          for(String8Node *n = msg->entry_points.first; n != 0; n = n->next)
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
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_id);
            U64 voff = rdi_first_voff_from_procedure(rdi, procedure);
            if(voff != 0)
            {
              entries_found = 1;
              DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
              dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
            }
          }
        }
        
        //- rjf: add traps for PID-correllated entry points
        if(!entries_found)
        {
          for(CTRL_Entity *e = ctrl_state->ctrl_thread_entity_store->root->first; e != &ctrl_entity_nil; e = e->next)
          {
            if(e->id == process->id)
            {
              U32 procedure_id = 0;
              {
                String8 name = e->string;
                RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, name.str, name.size);
                U32 id_count = 0;
                U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
                if(id_count > 0)
                {
                  procedure_id = ids[0];
                }
              }
              RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_id);
              U64 voff = rdi_first_voff_from_procedure(rdi, procedure);
              if(voff != 0)
              {
                entries_found = 1;
                DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
                dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
              }
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
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_id);
            U64 voff = rdi_first_voff_from_procedure(rdi, procedure);
            if(voff != 0)
            {
              DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
              dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
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
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_id);
            U64 voff = rdi_first_voff_from_procedure(rdi, procedure);
            if(voff != 0)
            {
              entries_found = 1;
              DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
              dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
            }
          }
        }
        
        //- rjf: add trap for PE header entry
        if(!entries_found)
        {
          U64 voff = ctrl_entry_point_voff_from_module(module->handle);
          if(voff != 0)
          {
            DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
            dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
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
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_id);
            U64 voff = rdi_first_voff_from_procedure(rdi, procedure);
            if(voff != 0)
            {
              entries_found = 1;
              DMN_Trap trap = {process->handle.dmn_handle, module_base_vaddr + voff};
              dmn_trap_chunk_list_push(scratch.arena, &entry_traps, 256, &trap);
            }
          }
        }
        
        //- rjf: no entry point found -> done
        if(entry_traps.trap_count == 0)
        {
          hard_stop = 1;
        }
        
        //- rjf: found entry points -> add to joined traps
        dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &entry_traps);
        
        di_scope_close(di_scope);
      }
      
      //////////////////////////
      //- rjf: unpack info about thread attached to event
      //
      CTRL_Entity *thread = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, event->thread));
      CTRL_Entity *process = ctrl_entity_from_handle(ctrl_state->ctrl_thread_entity_store, ctrl_handle_make(CTRL_MachineID_Local, event->process));
      Arch arch = thread->arch;
      U64 thread_rip_vaddr = dmn_rip_from_thread(event->thread);
      CTRL_Entity *module = &ctrl_entity_nil;
      {
        for(CTRL_Entity *m = process->first; m != &ctrl_entity_nil; m = m->next)
        {
          if(m->kind == CTRL_EntityKind_Module && contains_1u64(m->vaddr_range, thread_rip_vaddr))
          {
            module = m;
            break;
          }
        }
      }
      
      //////////////////////////
      //- rjf: extract module-dependent info
      //
      U64 thread_rip_voff = thread_rip_vaddr - module->vaddr_range.min;
      
      //////////////////////////
      //- rjf: stepping logic
      //
      //{
      
      //////////////////////////
      //- rjf: handle if hitting a spoof
      //
      B32 exception_stop = 0;
      B32 hit_spoof = 0;
      if(!hard_stop && use_stepping_logic && event->kind == DMN_EventKind_Exception)
      {
        if(spoof_mode &&
           dmn_handle_match(target_process.dmn_handle, event->process) &&
           dmn_handle_match(target_thread.dmn_handle, event->thread) &&
           spoof.new_ip_value == event->address)
        {
          hit_spoof = 1;
          log_infof("hit_spoof\n");
        }
        else
        {
          exception_stop = 1;
          use_stepping_logic = 0;
        }
      }
      
      //- rjf: handle spoof hit
      if(hit_spoof)
      {
        log_infof("exit_spoof_mode\n");
        
        // rjf: clear spoof mode
        spoof_mode = 0;
        MemoryZeroStruct(&spoof);
        
        // rjf: skip remainder of handling
        use_stepping_logic = 0;
      }
      
      //- rjf: for breakpoint events, gather bp info
      B32 hit_entry = 0;
      B32 hit_user_bp = 0;
      B32 hit_trap_net_bp = 0;
      B32 hit_conditional_bp_but_filtered = 0;
      CTRL_TrapFlags hit_trap_flags = 0;
      if(!hard_stop && use_stepping_logic && event->kind == DMN_EventKind_Breakpoint)
        ProfScope("for breakpoint events, gather bp info")
      {
        Temp temp = temp_begin(scratch.arena);
        String8List conditions = {0};
        
        // rjf: entry breakpoints
        for(DMN_TrapChunkNode *n = entry_traps.first; n != 0; n = n->next)
        {
          DMN_Trap *trap = n->v;
          DMN_Trap *opl = n->v + n->count;
          for(;trap < opl; trap += 1)
          {
            if(dmn_handle_match(trap->process, event->process) && trap->vaddr == event->instruction_pointer)
            {
              hit_entry = 1;
            }
          }
        }
        
        // rjf: user breakpoints
        for(DMN_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
        {
          DMN_Trap *trap = n->v;
          DMN_Trap *opl = n->v + n->count;
          for(;trap < opl; trap += 1)
          {
            if(dmn_handle_match(trap->process, event->process) &&
               trap->vaddr == event->instruction_pointer &&
               (!dmn_handle_match(event->thread, target_thread.dmn_handle) || !target_thread_is_on_user_bp_and_trap_net_trap))
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
        if(conditions.node_count != 0) ProfScope("evaluate hit stop conditions")
        {
          DI_Scope *di_scope = di_scope_open();
          
          // rjf: gather evaluation modules
          U64 eval_modules_count = Max(1, ctrl_state->ctrl_thread_entity_store->entity_kind_counts[CTRL_EntityKind_Module]);
          E_Module *eval_modules = push_array(temp.arena, E_Module, eval_modules_count);
          E_Module *eval_modules_primary = &eval_modules[0];
          eval_modules_primary->rdi = &di_rdi_parsed_nil;
          eval_modules_primary->vaddr_range = r1u64(0, max_U64);
          {
            U64 eval_module_idx = 0;
            for(CTRL_Entity *machine = ctrl_state->ctrl_thread_entity_store->root->first;
                machine != &ctrl_entity_nil;
                machine = machine->next)
            {
              if(machine->kind != CTRL_EntityKind_Machine) { continue; }
              for(CTRL_Entity *process = machine->first;
                  process != &ctrl_entity_nil;
                  process = process->next)
              {
                if(process->kind != CTRL_EntityKind_Process) { continue; }
                for(CTRL_Entity *mod = process->first;
                    mod != &ctrl_entity_nil;
                    mod = mod->next)
                {
                  if(mod->kind != CTRL_EntityKind_Module) { continue; }
                  CTRL_Entity *dbg_path = ctrl_entity_child_from_kind(mod, CTRL_EntityKind_DebugInfoPath);
                  DI_Key dbgi_key = {dbg_path->string, dbg_path->timestamp};
                  eval_modules[eval_module_idx].arch        = arch;
                  eval_modules[eval_module_idx].rdi         = di_rdi_from_key(di_scope, &dbgi_key, max_U64);
                  eval_modules[eval_module_idx].vaddr_range = mod->vaddr_range;
                  eval_modules[eval_module_idx].space       = e_space_make(CTRL_EvalSpaceKind_Entity);
                  eval_modules[eval_module_idx].space.u64_0 = (U64)process;
                  if(mod == module)
                  {
                    eval_modules_primary = &eval_modules[eval_module_idx];
                  }
                  eval_module_idx += 1;
                }
              }
            }
          }
          
          // rjf: loop through all conditions, check all
          for(String8Node *condition_n = conditions.first; condition_n != 0; condition_n = condition_n->next)
          {
            // rjf: build eval type context
            E_TypeCtx type_ctx = zero_struct;
            {
              E_TypeCtx *ctx = &type_ctx;
              ctx->ip_vaddr          = thread_rip_vaddr;
              ctx->ip_voff           = thread_rip_voff;
              ctx->modules           = eval_modules;
              ctx->modules_count     = eval_modules_count;
              ctx->primary_module    = eval_modules_primary;
            }
            e_select_type_ctx(&type_ctx);
            
            // rjf: build eval parse context
            E_ParseCtx parse_ctx = zero_struct;
            ProfScope("build eval parse context")
            {
              E_ParseCtx *ctx = &parse_ctx;
              ctx->ip_vaddr          = thread_rip_vaddr;
              ctx->ip_voff           = thread_rip_voff;
              ctx->ip_thread_space   = e_space_make(CTRL_EvalSpaceKind_Entity);
              ctx->ip_thread_space.u64_0 = (U64)thread;
              ctx->modules           = eval_modules;
              ctx->modules_count     = eval_modules_count;
              ctx->primary_module    = eval_modules_primary;
              ctx->regs_map      = ctrl_string2reg_from_arch(arch);
              ctx->reg_alias_map = ctrl_string2alias_from_arch(arch);
              ctx->locals_map    = e_push_locals_map_from_rdi_voff(temp.arena, eval_modules_primary->rdi, thread_rip_voff);
              ctx->member_map    = e_push_member_map_from_rdi_voff(temp.arena, eval_modules_primary->rdi, thread_rip_voff);
            }
            e_select_parse_ctx(&parse_ctx);
            
            // rjf: build eval IR context
            E_IRCtx ir_ctx = zero_struct;
            {
              E_IRCtx *ctx = &ir_ctx;
              ctx->macro_map     = push_array(temp.arena, E_String2ExprMap, 1);
              ctx->macro_map[0]  = e_string2expr_map_make(temp.arena, 512);
              E_TypeKey meval_type_key = e_type_key_cons_base(type(CTRL_MetaEval));
              for EachIndex(idx, ctrl_state->user_meta_evals.count)
              {
                E_Space space = e_space_make(CTRL_EvalSpaceKind_Meta);
                E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
                expr->space    = space;
                expr->mode     = E_Mode_Offset;
                expr->type_key = meval_type_key;
                e_string2expr_map_insert(temp.arena, ctx->macro_map, ctrl_state->user_meta_evals.v[idx].label, expr);
              }
            }
            e_select_ir_ctx(&ir_ctx);
            
            // rjf: build eval interpretation context
            E_InterpretCtx interpret_ctx = zero_struct;
            {
              E_InterpretCtx *ctx = &interpret_ctx;
              ctx->space_rw_user_data = ctrl_state->ctrl_thread_entity_store;
              ctx->space_read    = ctrl_eval_space_read;
              ctx->primary_space = eval_modules_primary->space;
              ctx->reg_arch      = eval_modules_primary->arch;
              ctx->reg_space     = e_space_make(CTRL_EvalSpaceKind_Entity);
              ctx->reg_space.u64_0 = (U64)thread;
              ctx->module_base   = push_array(temp.arena, U64, 1);
              ctx->module_base[0]= module->vaddr_range.min;
              ctx->tls_base      = push_array(temp.arena, U64, 1);
            }
            e_select_interpret_ctx(&interpret_ctx);
            
            // rjf: evaluate
            E_Eval eval = zero_struct;
            ProfScope("evaluate expression")
            {
              eval = e_eval_from_string(temp.arena, condition_n->string);
            }
            
            // rjf: interpret evaluation
            if(eval.code == E_InterpretationCode_Good && eval.value.u64 == 0)
            {
              hit_user_bp = 0;
              hit_conditional_bp_but_filtered = 1;
              log_infof("conditional_breakpoint_hit: 'condition eval'd to 0, and so filtered'\n");
            }
            else
            {
              hit_user_bp = 1;
              hit_conditional_bp_but_filtered = 0;
              log_infof("conditional_breakpoint_hit: 'conditional eval'd to nonzero, hit'\n");
              break;
            }
          }
          di_scope_close(di_scope);
        }
        
        // rjf: gather trap net hits
        ProfScope("gather trap net hits")
        {
          if(!hit_user_bp && dmn_handle_match(event->process, target_process.dmn_handle))
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
        }
        
        log_infof("user_breakpoint_hit: %i\n", hit_user_bp);
        log_infof("entry_point_hit: %i\n", hit_entry);
        temp_end(temp);
      }
      
      //- rjf: hit conditional user bp but filtered -> single step
      B32 cond_bp_single_step_stop = 0;
      CTRL_EventCause cond_bp_single_step_stop_cause = CTRL_EventCause_Null;
      if(hit_conditional_bp_but_filtered) LogInfoNamedBlockF("conditional_bp_hit_single_step")
      {
        DMN_Handle thread = event->thread;
        U64 thread_pre_rip = dmn_rip_from_thread(thread);
        U64 thread_post_rip = thread_pre_rip;
        for(B32 single_step_done = 0; !single_step_done;)
        {
          DMN_RunCtrls single_step_ctrls = {0};
          single_step_ctrls.run_entities_are_unfrozen = 1;
          single_step_ctrls.run_entities = &thread;
          single_step_ctrls.run_entity_count = 1;
          if(thread_post_rip == thread_pre_rip)
          {
            single_step_ctrls.single_step_thread = thread;
          }
          DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
          thread_post_rip = dmn_rip_from_thread(thread);
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
              single_step_done = dmn_handle_match(event->thread, thread);
              cond_bp_single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
            }break;
          }
        }
      }
      
      //- rjf: hit entry points on *any thread* cause a stop, if this msg says as such
      B32 entry_stop = 0;
      if(msg->run_flags & CTRL_RunFlag_StopOnEntryPoint && hit_entry)
      {
        entry_stop = 1;
        use_stepping_logic = 0;
      }
      
      //- rjf: user breakpoints on *any thread* cause a stop
      B32 user_bp_stop = 0;
      if(!hard_stop && use_stepping_logic && hit_user_bp)
      {
        user_bp_stop = 1;
        use_stepping_logic = 0;
      }
      
      //- rjf: trap net on off-target threads are ignored
      B32 step_past_trap_net = 0;
      if(!hard_stop && use_stepping_logic && hit_trap_net_bp)
      {
        if(!dmn_handle_match(event->thread, target_thread.dmn_handle))
        {
          step_past_trap_net = 1;
          use_stepping_logic = 0;
        }
      }
      
      //- rjf: trap net on on-target threads trigger trap net logic
      B32 use_trap_net_logic = 0;
      if(!hard_stop && use_stepping_logic && hit_trap_net_bp)
      {
        if(dmn_handle_match(event->thread, target_thread.dmn_handle))
        {
          use_trap_net_logic = 1;
        }
      }
      
      //- rjf: trap net logic: stack pointer check
      B32 stack_pointer_matches = 0;
      if(use_trap_net_logic)
      {
        U64 sp = dmn_rsp_from_thread(target_thread.dmn_handle);
        stack_pointer_matches = (sp == sp_check_value);
      }
      
      //- rjf: trap net logic: single step after hit
      B32 single_step_stop = 0;
      CTRL_EventCause single_step_stop_cause = CTRL_EventCause_Null;
      if(!hard_stop && use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_SingleStepAfterHit) LogInfoNamedBlockF("trap_net__single_step_after_hit")
        {
          U64 thread_pre_rip = dmn_rip_from_thread(target_thread.dmn_handle);
          U64 thread_post_rip = thread_pre_rip;
          for(B32 single_step_done = 0; single_step_done == 0;)
          {
            DMN_RunCtrls single_step_ctrls = {0};
            single_step_ctrls.run_entities_are_unfrozen = 1;
            single_step_ctrls.run_entities = &target_thread.dmn_handle;
            single_step_ctrls.run_entity_count = 1;
            if(thread_post_rip == thread_pre_rip)
            {
              single_step_ctrls.single_step_thread = target_thread.dmn_handle;
            }
            DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
            thread_post_rip = dmn_rip_from_thread(target_thread.dmn_handle);
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
                single_step_done = dmn_handle_match(event->thread, target_thread.dmn_handle);
                single_step_stop_cause = ctrl_event_cause_from_dmn_event_kind(event->kind);
              }break;
            }
          }
        }
      }
      
      //- rjf: trap net logic: begin spoof mode
      B32 begin_spoof_mode = 0;
      if(!hard_stop && use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_BeginSpoofMode) LogInfoNamedBlockF("trap_net__begin_spoof_mode")
        {
          // rjf: setup spoof mode
          begin_spoof_mode = 1;
          spoof_mode = 1;
          spoof.process = target_process.dmn_handle;
          spoof.thread  = target_thread.dmn_handle;
          spoof.new_ip_value = spoof_ip_vaddr;

          switch(arch)
          {
            default: {}break;
            case Arch_x64:
            case Arch_x86:
            {
              U64 spoof_sp = dmn_rsp_from_thread(target_thread.dmn_handle);
              spoof.vaddr = spoof_sp;
            }break;
          }

          log_infof("spoof:{process:[0x%I64x], thread:[0x%I64x], vaddr:0x%I64x, new_ip_value:0x%I64x}\n", spoof.process.u64[0], spoof.thread.u64[0], spoof.vaddr, spoof.new_ip_value);
        }
      }
      
      //- rjf: trap net logic: save stack pointer
      B32 save_stack_pointer = 0;
      if(!hard_stop && use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_SaveStackPointer)
        {
          if(stack_pointer_matches) LogInfoNamedBlockF("trap_net__save_sp")
          {
            save_stack_pointer = 1;
            sp_check_value = dmn_rsp_from_thread(target_thread.dmn_handle);
            log_infof("sp_check_value = 0x%I64x\n", sp_check_value);
          }
        }
      }
      
      //- rjf: trap net logic: end stepping
      B32 trap_net_stop = 0;
      if(!hard_stop && use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_EndStepping) LogInfoNamedBlockF("trap_net__end_step")
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
      if(step_past_trap_net) LogInfoNamedBlockF("trap_net__single_step_past_trap_net")
      {
        DMN_Handle thread = event->thread;
        U64 thread_pre_rip = dmn_rip_from_thread(thread);
        U64 thread_post_rip = thread_pre_rip;
        for(B32 single_step_done = 0; single_step_done == 0;)
        {
          DMN_RunCtrls single_step_ctrls = {0};
          single_step_ctrls.run_entities_are_unfrozen = 1;
          single_step_ctrls.run_entities = &thread;
          single_step_ctrls.run_entity_count = 1;
          if(thread_post_rip == thread_pre_rip)
          {
            single_step_ctrls.single_step_thread = thread;
          }
          DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &single_step_ctrls, 0);
          thread_post_rip = dmn_rip_from_thread(thread);
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
              single_step_done = dmn_handle_match(event->thread, thread);
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
      else if(entry_stop)
      {
        stage_stop_cause = CTRL_EventCause_EntryPoint;
      }
      else if(trap_net_stop)
      {
        stage_stop_cause = CTRL_EventCause_Finished;
      }
      log_infof("stop_cause: %i\n", stage_stop_cause);
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
    event->entity = ctrl_handle_make(CTRL_MachineID_Local, stop_event->thread);
    event->parent = ctrl_handle_make(CTRL_MachineID_Local, stop_event->process);
    event->exception_code = stop_event->code;
    event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
    event->rip_vaddr = stop_event->instruction_pointer;
    ctrl_c2u_push_events(&evts);
  }
  
  log_infof("}\n\n");
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__single_step(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: record start
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: single step
  DMN_Handle thread = msg->entity.dmn_handle;
  B32 thread_is_valid = !dmn_handle_match(thread, dmn_handle_zero());
  DMN_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  if(thread_is_valid)
  {
    U64 thread_pre_rip = dmn_rip_from_thread(thread);
    U64 thread_post_rip = thread_pre_rip;
    for(B32 done = 0; done == 0;)
    {
      DMN_RunCtrls run_ctrls = {0};
      run_ctrls.run_entities_are_unfrozen = 1;
      run_ctrls.run_entities = &thread;
      run_ctrls.run_entity_count = 1;
      if(thread_post_rip == thread_pre_rip)
      {
        run_ctrls.single_step_thread = msg->entity.dmn_handle;
      }
      DMN_Event *event = ctrl_thread__next_dmn_event(scratch.arena, ctrl_ctx, msg, &run_ctrls, 0);
      thread_post_rip = dmn_rip_from_thread(msg->entity.dmn_handle);
      switch(event->kind)
      {
        default:{}break;
        case DMN_EventKind_Error:      {stop_cause = CTRL_EventCause_Error;}goto end_single_step;
        case DMN_EventKind_Exception:  {stop_cause = CTRL_EventCause_InterruptedByException;}goto end_single_step;
        case DMN_EventKind_Halt:       {stop_cause = CTRL_EventCause_InterruptedByHalt;}goto end_single_step;
        case DMN_EventKind_Trap:       {stop_cause = CTRL_EventCause_InterruptedByTrap;}goto end_single_step;
        case DMN_EventKind_Breakpoint: {stop_cause = CTRL_EventCause_UserBreakpoint;}goto end_single_step;
        case DMN_EventKind_SingleStep: {stop_cause = CTRL_EventCause_Finished;}goto end_single_step;
        end_single_step:
        {
          stop_event = event;
          done = 1;
        }break;
      }
    }
  }
  
  //- rjf: record stop
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Stopped;
    event->cause = stop_cause;
    if(stop_event != 0)
    {
      event->entity = ctrl_handle_make(CTRL_MachineID_Local, stop_event->thread);
      event->parent = ctrl_handle_make(CTRL_MachineID_Local, stop_event->process);
      event->exception_code = stop_event->code;
      event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
      event->rip_vaddr = stop_event->instruction_pointer;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Memory-Stream-Thread-Only Functions

//- rjf: user -> memory stream communication

internal B32
ctrl_u2ms_enqueue_req(CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    U64 available_size = ctrl_state->u2ms_ring_size-unconsumed_size;
    if(available_size >= sizeof(process)+sizeof(vaddr_range)+sizeof(zero_terminated))
    {
      good = 1;
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
ctrl_u2ms_dequeue_req(CTRL_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated)
{
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    if(unconsumed_size >= sizeof(*out_process)+sizeof(*out_vaddr_range)+sizeof(*out_zero_terminated))
    {
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

ASYNC_WORK_DEF(ctrl_mem_stream_work)
{
  ProfBeginFunction();
  CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
  //- rjf: unpack next request
  CTRL_Handle process = {0};
  Rng1U64 vaddr_range = {0};
  B32 zero_terminated = 0;
  ctrl_u2ms_dequeue_req(&process, &vaddr_range, &zero_terminated);
  U128 key = ctrl_calc_hash_store_key_from_process_vaddr_range(process, vaddr_range, zero_terminated);
  ProfBegin("memory stream request");
  
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
      if(ctrl_handle_match(n->handle, process))
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
  U64 pre_read_mem_gen = dmn_mem_gen();
  U64 post_read_mem_gen = 0;
  if(got_task && pre_read_mem_gen != preexisting_mem_gen)
  {
    range_size = dim_1u64(vaddr_range_clamped);
    U64 page_size = os_get_system_info()->page_size;
    U64 arena_size = AlignPow2(range_size + ARENA_HEADER_SIZE, page_size);
    range_arena = arena_alloc(.reserve_size = range_size+ARENA_HEADER_SIZE, .commit_size = range_size+ARENA_HEADER_SIZE);
    if(range_arena == 0)
    {
      range_size = 0;
    }
    else
    {
      range_base = push_array_no_zero(range_arena, U8, range_size);
      U64 bytes_read = 0;
      U64 retry_count = 0;
      U64 retry_limit = range_size > page_size ? 64 : 0;
      for(Rng1U64 vaddr_range_clamped_retry = vaddr_range_clamped;
          retry_count <= retry_limit;
          retry_count += 1)
      {
        bytes_read = dmn_process_read(process.dmn_handle, vaddr_range_clamped_retry, range_base);
        if(bytes_read == 0 && vaddr_range_clamped_retry.max > vaddr_range_clamped_retry.min)
        {
          U64 diff = (vaddr_range_clamped_retry.max-vaddr_range_clamped_retry.min)/2;
          vaddr_range_clamped_retry.max -= diff;
          vaddr_range_clamped_retry.max = AlignDownPow2(vaddr_range_clamped_retry.max, page_size);
          if(diff == 0)
          {
            break;
          }
        }
        else
        {
          break;
        }
      }
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
    post_read_mem_gen = dmn_mem_gen();
  }
  
  //- rjf: read successful -> submit to hash store
  U128 hash = {0};
  if(got_task && range_base != 0 && pre_read_mem_gen == post_read_mem_gen)
  {
    hash = hs_submit_data(key, &range_arena, str8((U8*)range_base, zero_terminated_size));
  }
  else if(range_arena != 0)
  {
    arena_release(range_arena);
  }
  
  //- rjf: commit hash to cache
  if(got_task) OS_MutexScopeW(process_stripe->rw_mutex)
  {
    for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->handle, process))
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
              range_n->mem_gen = post_read_mem_gen;
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
  ProfEnd();
  ProfEnd();
  return 0;
}
