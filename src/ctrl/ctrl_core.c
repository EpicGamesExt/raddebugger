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

internal CTRL_ExceptionKind
ctrl_exception_kind_from_dmn(DMN_ExceptionKind kind)
{
  CTRL_ExceptionKind result = CTRL_ExceptionKind_Null;
  switch(kind)
  {
    default:{}break;
    case DMN_ExceptionKind_MemoryRead:    {result = CTRL_ExceptionKind_MemoryRead;}break;
    case DMN_ExceptionKind_MemoryWrite:   {result = CTRL_ExceptionKind_MemoryWrite;}break;
    case DMN_ExceptionKind_MemoryExecute: {result = CTRL_ExceptionKind_MemoryExecute;}break;
    case DMN_ExceptionKind_CppThrow:      {result = CTRL_ExceptionKind_CppThrow;}break;
  }
  return result;
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

internal CTRL_EntityKind
ctrl_entity_kind_from_string(String8 string)
{
  CTRL_EntityKind result = CTRL_EntityKind_Null;
  for EachNonZeroEnumVal(CTRL_EntityKind, k)
  {
    if(str8_match(ctrl_entity_kind_code_name_table[k], string, 0))
    {
      result = k;
      break;
    }
  }
  return result;
}

internal DMN_TrapFlags
ctrl_dmn_trap_flags_from_user_breakpoint_flags(CTRL_UserBreakpointFlags flags)
{
  DMN_TrapFlags result = 0;
  if(flags & CTRL_UserBreakpointFlag_BreakOnWrite)    { result |= DMN_TrapFlag_BreakOnWrite; }
  if(flags & CTRL_UserBreakpointFlag_BreakOnRead)     { result |= DMN_TrapFlag_BreakOnRead; }
  if(flags & CTRL_UserBreakpointFlag_BreakOnExecute)  { result |= DMN_TrapFlag_BreakOnExecute; }
  return result;
}

internal CTRL_UserBreakpointFlags
ctrl_user_breakpoint_flags_from_dmn_trap_flags(DMN_TrapFlags flags)
{
  CTRL_UserBreakpointFlags result = 0;
  if(flags & DMN_TrapFlag_BreakOnWrite)    { result |= CTRL_UserBreakpointFlag_BreakOnWrite; }
  if(flags & DMN_TrapFlag_BreakOnRead)     { result |= CTRL_UserBreakpointFlag_BreakOnRead; }
  if(flags & DMN_TrapFlag_BreakOnExecute)  { result |= CTRL_UserBreakpointFlag_BreakOnExecute; }
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

internal String8
ctrl_string_from_handle(Arena *arena, CTRL_Handle handle)
{
  String8 result = push_str8f(arena, "$%I64x$%I64x", handle.machine_id, handle.dmn_handle.u64[0]);
  return result;
}

internal CTRL_Handle
ctrl_handle_from_string(String8 string)
{
  CTRL_Handle handle = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    U8 split = '$';
    String8List parts = str8_split(scratch.arena, string, &split, 1, 0);
    if(parts.first && parts.first->next)
    {
      CTRL_MachineID machine_id = u64_from_str8(parts.first->string, 16);
      DMN_Handle dmn_handle = {0};
      dmn_handle.u64[0] = u64_from_str8(parts.first->next->string, 16);
      handle.machine_id = machine_id;
      handle.dmn_handle = dmn_handle;
    }
    scratch_end(scratch);
  }
  return handle;
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
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->flags);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->id);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->string.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, bp->string.str, bp->string.size);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->pt);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->size);
        str8_serial_push_struct(scratch.arena, &msgs_srlzed, &bp->condition.size);
        str8_serial_push_data(scratch.arena, &msgs_srlzed, bp->condition.str, bp->condition.size);
      }
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
        read_off += str8_deserial_read_struct(string, read_off, &bp->flags);
        read_off += str8_deserial_read_struct(string, read_off, &bp->id);
        read_off += str8_deserial_read_struct(string, read_off, &bp->string.size);
        bp->string.str = push_array_no_zero(arena, U8, bp->string.size);
        read_off += str8_deserial_read(string, read_off, bp->string.str, bp->string.size, 1);
        read_off += str8_deserial_read_struct(string, read_off, &bp->pt);
        read_off += str8_deserial_read_struct(string, read_off, &bp->size);
        read_off += str8_deserial_read_struct(string, read_off, &bp->condition.size);
        bp->condition.str = push_array_no_zero(arena, U8, bp->condition.size);
        read_off += str8_deserial_read(string, read_off, bp->condition.str, bp->condition.size, 1);
      }
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
    str8_serial_push_struct(scratch.arena, &srl, &event->exception_code);
    str8_serial_push_struct(scratch.arena, &srl, &event->rgba);
    str8_serial_push_struct(scratch.arena, &srl, &event->bp_flags);
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
    read_off += str8_deserial_read_struct(string, read_off, &event.bp_flags);
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
ctrl_entity_list_from_handle_list(Arena *arena, CTRL_EntityCtx *ctx, CTRL_HandleList *list)
{
  CTRL_EntityList result = {0};
  for(CTRL_HandleNode *n = list->first; n != 0; n = n->next)
  {
    CTRL_Entity *entity = ctrl_entity_from_handle(ctx, n->v);
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

//- rjf: entity context (entity group read-only) functions

internal CTRL_Entity *
ctrl_entity_from_handle(CTRL_EntityCtx *ctx, CTRL_Handle handle)
{
  CTRL_Entity *entity = &ctrl_entity_nil;
  if(!ctrl_handle_match(handle, ctrl_handle_zero()))
  {
    U64 hash = ctrl_hash_from_handle(handle);
    U64 slot_idx = hash%ctx->hash_slots_count;
    CTRL_EntityHashSlot *slot = &ctx->hash_slots[slot_idx];
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

internal CTRL_Entity *
ctrl_module_from_thread_candidates(CTRL_EntityCtx *ctx, CTRL_Entity *thread, CTRL_EntityList *candidates)
{
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  U64 thread_rip_vaddr = ctrl_rip_from_thread(ctx, thread->handle);
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

//- rjf: entity ctx r/w store state functions

internal CTRL_EntityCtxRWStore *
ctrl_entity_ctx_rw_store_alloc(void)
{
  Arena *arena = arena_alloc();
  CTRL_EntityCtxRWStore *store = push_array(arena, CTRL_EntityCtxRWStore, 1);
  store->arena = arena;
  store->ctx.hash_slots_count = 1024;
  store->ctx.hash_slots = push_array(arena, CTRL_EntityHashSlot, store->ctx.hash_slots_count);
  CTRL_Entity *root = store->ctx.root = ctrl_entity_alloc(store, &ctrl_entity_nil, CTRL_EntityKind_Root, Arch_Null, ctrl_handle_zero(), 0);
  CTRL_Entity *local_machine = ctrl_entity_alloc(store, root, CTRL_EntityKind_Machine, arch_from_context(), ctrl_handle_make(CTRL_MachineID_Local, dmn_handle_zero()), 0);
  Temp scratch = scratch_begin(0, 0);
  String8 local_machine_name = push_str8f(scratch.arena, "This PC (%S)", os_get_system_info()->machine_name);
  ctrl_entity_equip_string(store, local_machine, local_machine_name);
  scratch_end(scratch);
  return store;
}

internal void
ctrl_entity_ctx_rw_store_release(CTRL_EntityCtxRWStore *store)
{
  arena_release(store->arena);
}

//- rjf: string allocation/deletion

internal U64
ctrl_name_bucket_num_from_string_size(U64 size)
{
  U64 bucket_num = 0;
  if(size > 0)
  {
    for EachElement(idx, ctrl_entity_string_bucket_chunk_sizes)
    {
      if(size <= ctrl_entity_string_bucket_chunk_sizes[idx])
      {
        bucket_num = idx+1;
        break;
      }
    }
  }
  return bucket_num;
}

internal String8
ctrl_entity_string_alloc(CTRL_EntityCtxRWStore *store, String8 string)
{
  //- rjf: allocate node
  CTRL_EntityStringChunkNode *node = 0;
  {
    U64 bucket_num = ctrl_name_bucket_num_from_string_size(string.size);
    if(bucket_num == ArrayCount(ctrl_entity_string_bucket_chunk_sizes))
    {
      CTRL_EntityStringChunkNode *best_node = 0;
      CTRL_EntityStringChunkNode *best_node_prev = 0;
      U64 best_node_size = max_U64;
      {
        for(CTRL_EntityStringChunkNode *n = store->free_string_chunks[bucket_num-1], *prev = 0; n != 0; (prev = n, n = n->next))
        {
          if(n->size >= string.size && n->size < best_node_size)
          {
            best_node = n;
            best_node_prev = prev;
            best_node_size = n->size;
          }
        }
      }
      if(best_node != 0)
      {
        node = best_node;
        if(best_node_prev)
        {
          best_node_prev->next = best_node->next;
        }
        else
        {
          store->free_string_chunks[bucket_num-1] = best_node->next;
        }
      }
      else
      {
        U64 chunk_size = u64_up_to_pow2(string.size);
        node = (CTRL_EntityStringChunkNode *)push_array(store->arena, U8, chunk_size);
      }
    }
    else if(bucket_num != 0)
    {
      node = store->free_string_chunks[bucket_num-1];
      if(node != 0)
      {
        SLLStackPop(store->free_string_chunks[bucket_num-1]);
      }
      else
      {
        node = (CTRL_EntityStringChunkNode *)push_array(store->arena, U8, ctrl_entity_string_bucket_chunk_sizes[bucket_num-1]);
      }
    }
  }
  
  //- rjf: fill node
  String8 result = {0};
  if(node != 0)
  {
    result.str = (U8 *)node;
    result.size = string.size;
    MemoryCopy(result.str, string.str, result.size);
  }
  return result;
}

internal void
ctrl_entity_string_release(CTRL_EntityCtxRWStore *store, String8 string)
{
  U64 bucket_num = ctrl_name_bucket_num_from_string_size(string.size);
  if(1 <= bucket_num && bucket_num <= ArrayCount(rd_name_bucket_chunk_sizes))
  {
    U64 bucket_idx = bucket_num-1;
    CTRL_EntityStringChunkNode *node = (CTRL_EntityStringChunkNode *)string.str;
    SLLStackPush(store->free_string_chunks[bucket_idx], node);
    node->size = u64_up_to_pow2(string.size);
  }
}

//- rjf: entity construction/deletion

internal CTRL_Entity *
ctrl_entity_alloc(CTRL_EntityCtxRWStore *store, CTRL_Entity *parent, CTRL_EntityKind kind, Arch arch, CTRL_Handle handle, U64 id)
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
      U64 slot_idx = hash%store->ctx.hash_slots_count;
      CTRL_EntityHashSlot *slot = &store->ctx.hash_slots[slot_idx];
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
    store->ctx.entity_kind_counts[kind] += 1;
    store->ctx.entity_kind_alloc_gens[kind] += 1;
  }
  return entity;
}

internal void
ctrl_entity_release(CTRL_EntityCtxRWStore *store, CTRL_Entity *entity)
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
        U64 slot_idx = hash%store->ctx.hash_slots_count;
        CTRL_EntityHashSlot *slot = &store->ctx.hash_slots[slot_idx];
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
      store->ctx.entity_kind_counts[t->e->kind] -= 1;
      store->ctx.entity_kind_alloc_gens[t->e->kind] += 1;
    }
    scratch_end(scratch);
  }
}

//- rjf: entity equipment

internal void
ctrl_entity_equip_string(CTRL_EntityCtxRWStore *store, CTRL_Entity *entity, String8 string)
{
  if(entity->string.size != 0)
  {
    ctrl_entity_string_release(store, entity->string);
  }
  entity->string = ctrl_entity_string_alloc(store, string);
}

//- rjf: accelerated entity context lookups

internal CTRL_EntityCtxLookupAccel *
ctrl_thread_entity_ctx_lookup_accel(void)
{
  if(ctrl_entity_ctx_lookup_accel == 0)
  {
    Arena *arena = arena_alloc();
    ctrl_entity_ctx_lookup_accel = push_array(arena, CTRL_EntityCtxLookupAccel, 1);
    ctrl_entity_ctx_lookup_accel->arena = arena;
    for EachEnumVal(CTRL_EntityKind, k)
    {
      ctrl_entity_ctx_lookup_accel->entity_kind_arrays_arenas[k] = arena_alloc();
    }
  }
  return ctrl_entity_ctx_lookup_accel;
}

internal CTRL_EntityArray
ctrl_entity_array_from_kind(CTRL_EntityCtx *ctx, CTRL_EntityKind kind)
{
  CTRL_EntityCtxLookupAccel *accel = ctrl_thread_entity_ctx_lookup_accel();
  if(accel->entity_kind_arrays_gens[kind] != ctx->entity_kind_alloc_gens[kind])
  {
    Temp scratch = scratch_begin(0, 0);
    CTRL_EntityList entities = {0};
    for(CTRL_Entity *e = ctx->root;
        e != &ctrl_entity_nil;
        e = ctrl_entity_rec_depth_first_pre(e, ctx->root).next)
    {
      if(e->kind == kind)
      {
        ctrl_entity_list_push(scratch.arena, &entities, e);
      }
    }
    accel->entity_kind_arrays_gens[kind] = ctx->entity_kind_alloc_gens[kind];
    arena_clear(accel->entity_kind_arrays_arenas[kind]);
    accel->entity_kind_arrays[kind] = ctrl_entity_array_from_list(accel->entity_kind_arrays_arenas[kind], &entities);
    scratch_end(scratch);
  }
  return accel->entity_kind_arrays[kind];
}

internal CTRL_EntityList
ctrl_modules_from_dbgi_key(Arena *arena, CTRL_EntityCtx *ctx, DI_Key *dbgi_key)
{
  CTRL_EntityList list = {0};
  CTRL_EntityArray all_modules = ctrl_entity_array_from_kind(ctx, CTRL_EntityKind_Module);
  for EachIndex(idx, all_modules.count)
  {
    CTRL_Entity *module = all_modules.v[idx];
    DI_Key module_dbgi_key = ctrl_dbgi_key_from_module(module);
    if(di_key_match(&module_dbgi_key, dbgi_key))
    {
      ctrl_entity_list_push(arena, &list, module);
    }
  }
  return list;
}

internal CTRL_Entity *
ctrl_thread_from_id(CTRL_EntityCtx *ctx, U64 id)
{
  CTRL_Entity *thread = &ctrl_entity_nil;
  CTRL_EntityArray threads = ctrl_entity_array_from_kind(ctx, CTRL_EntityKind_Thread);
  for EachIndex(idx, threads.count)
  {
    if(threads.v[idx]->id == id)
    {
      thread = threads.v[idx];
    }
  }
  return thread;
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
ctrl_entity_store_apply_events(CTRL_EntityCtxRWStore *store, CTRL_EventList *list)
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
        CTRL_Entity *machine = ctrl_entity_from_handle(&store->ctx, ctrl_handle_make(event->entity.machine_id, dmn_handle_zero()));
        CTRL_Entity *process = ctrl_entity_alloc(store, machine, CTRL_EntityKind_Process, event->arch, event->entity, (U64)event->entity_id);
      }break;
      case CTRL_EventKind_EndProc:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->entity);
        ctrl_entity_release(store, process);
        for(CTRL_Entity *entry = store->ctx.root->first, *next = &ctrl_entity_nil;
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
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
        CTRL_Entity *thread = ctrl_entity_alloc(store, process, CTRL_EntityKind_Thread, event->arch, event->entity, (U64)event->entity_id);
        CTRL_Entity *first_thread = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Thread);
        if(first_thread == thread)
        {
          ctrl_entity_equip_string(store, thread, str8_lit("main_thread"));
        }
        CTRL_EntityArray pending_thread_names = ctrl_entity_array_from_kind(&store->ctx, CTRL_EntityKind_PendingThreadName);
        for EachIndex(idx, pending_thread_names.count)
        {
          CTRL_Entity *entity = pending_thread_names.v[idx];
          if(entity->id == event->entity_id)
          {
            ctrl_entity_equip_string(store, thread, entity->string);
            ctrl_entity_release(store, entity);
            break;
          }
        }
        CTRL_EntityArray pending_thread_colors = ctrl_entity_array_from_kind(&store->ctx, CTRL_EntityKind_PendingThreadColor);
        for EachIndex(idx, pending_thread_colors.count)
        {
          CTRL_Entity *entity = pending_thread_colors.v[idx];
          if(entity->id == event->entity_id)
          {
            thread->rgba = entity->rgba;
            ctrl_entity_release(store, entity);
            break;
          }
        }
        thread->stack_base = event->stack_base;
        ctrl_rip_from_thread(&store->ctx, event->entity);
      }break;
      case CTRL_EventKind_EndThread:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(&store->ctx, event->entity);
        ctrl_entity_release(store, thread);
      }break;
      case CTRL_EventKind_ThreadName:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
        CTRL_Entity *thread = &ctrl_entity_nil;
        if(event->entity_id == 0)
        {
          thread = ctrl_entity_from_handle(&store->ctx, event->entity);
        }
        else
        {
          thread = ctrl_thread_from_id(&store->ctx, event->entity_id);
        }
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
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
        CTRL_Entity *thread = &ctrl_entity_nil;
        if(event->entity_id == 0)
        {
          thread = ctrl_entity_from_handle(&store->ctx, event->entity);
        }
        else
        {
          thread = ctrl_thread_from_id(&store->ctx, event->entity_id);
        }
        if(thread != &ctrl_entity_nil)
        {
          thread->rgba = event->rgba;
        }
        else
        {
          CTRL_Entity *pending = ctrl_entity_alloc(store, process, CTRL_EntityKind_PendingThreadColor, Arch_Null, ctrl_handle_zero(), event->entity_id);
          pending->rgba = event->rgba;
        }
      }break;
      case CTRL_EventKind_ThreadFrozen:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(&store->ctx, event->entity);
        thread->is_frozen = 1;
      }break;
      case CTRL_EventKind_ThreadThawed:
      {
        CTRL_Entity *thread = ctrl_entity_from_handle(&store->ctx, event->entity);
        thread->is_frozen = 0;
      }break;
      
      //- rjf: modules
      case CTRL_EventKind_NewModule:
      {
        Temp scratch = scratch_begin(0, 0);
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
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
        CTRL_Entity *module = ctrl_entity_from_handle(&store->ctx, event->entity);
        ctrl_entity_release(store, module);
      }break;
      case CTRL_EventKind_ModuleDebugInfoPathChange:
      {
        CTRL_Entity *module = ctrl_entity_from_handle(&store->ctx, event->entity);
        CTRL_Entity *debug_info_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
        if(debug_info_path == &ctrl_entity_nil)
        {
          debug_info_path = ctrl_entity_alloc(store, module, CTRL_EntityKind_DebugInfoPath, Arch_Null, ctrl_handle_zero(), 0);
        }
        ctrl_entity_equip_string(store, debug_info_path, event->string);
        debug_info_path->timestamp = event->timestamp;
      }break;
      
      //- rjf: dynamic, program-created breakpoints
      case CTRL_EventKind_SetBreakpoint:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
        CTRL_Entity *bp = ctrl_entity_alloc(store, process, CTRL_EntityKind_Breakpoint, Arch_Null, ctrl_handle_zero(), 0);
        bp->vaddr_range = event->vaddr_rng;
        bp->bp_flags = event->bp_flags;
      }break;
      case CTRL_EventKind_UnsetBreakpoint:
      {
        CTRL_Entity *process = ctrl_entity_from_handle(&store->ctx, event->parent);
        for(CTRL_Entity *child = process->first; child != &ctrl_entity_nil; child = child->next)
        {
          if(child->kind == CTRL_EntityKind_Breakpoint &&
             child->vaddr_range.min == event->vaddr_rng.min &&
             child->vaddr_range.max == event->vaddr_rng.max &&
             child->bp_flags == event->bp_flags)
          {
            ctrl_entity_release(store, child);
            break;
          }
        }
      }break;
    }
  }
}

////////////////////////////////
//~ rjf: Cache Accessing Scopes

internal CTRL_Scope *
ctrl_scope_open(void)
{
  if(ctrl_tctx == 0)
  {
    Arena *arena = arena_alloc();
    ctrl_tctx = push_array(arena, CTRL_TCTX, 1);
    ctrl_tctx->arena = arena;
  }
  CTRL_Scope *scope = ctrl_tctx->free_scope;
  if(scope != 0)
  {
    SLLStackPop(ctrl_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(ctrl_tctx->arena, CTRL_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
ctrl_scope_close(CTRL_Scope *scope)
{
  for(CTRL_ScopeCallStackTouch *t = scope->first_call_stack_touch, *next = 0; t != 0; t = next)
  {
    next = t->next;
    ins_atomic_u64_dec_eval(&t->node->scope_touch_count);
    os_condition_variable_broadcast(t->stripe->cv);
    SLLStackPush(ctrl_tctx->free_call_stack_touch, t);
  }
  SLLStackPush(ctrl_tctx->free_scope, scope);
}

internal void
ctrl_scope_touch_call_stack_node__stripe_r_guarded(CTRL_Scope *scope, CTRL_CallStackCacheStripe *stripe, CTRL_CallStackCacheNode *node)
{
  ins_atomic_u64_inc_eval(&node->scope_touch_count);
  CTRL_ScopeCallStackTouch *touch = ctrl_tctx->free_call_stack_touch;
  if(touch != 0)
  {
    SLLStackPop(ctrl_tctx->free_call_stack_touch);
  }
  else
  {
    touch = push_array(ctrl_tctx->arena, CTRL_ScopeCallStackTouch, 1);
  }
  SLLQueuePush(scope->first_call_stack_touch, scope->last_call_stack_touch, touch);
  touch->stripe = stripe;
  touch->node = node;
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
  ctrl_state->call_stack_cache.slots_count = 1024;
  ctrl_state->call_stack_cache.slots = push_array(arena, CTRL_CallStackCacheSlot, ctrl_state->call_stack_cache.slots_count);
  ctrl_state->call_stack_cache.stripes_count = os_get_system_info()->logical_processor_count;
  ctrl_state->call_stack_cache.stripes = push_array(arena, CTRL_CallStackCacheStripe, ctrl_state->call_stack_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->call_stack_cache.stripes_count; idx += 1)
  {
    ctrl_state->call_stack_cache.stripes[idx].arena = arena_alloc();
    ctrl_state->call_stack_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
    ctrl_state->call_stack_cache.stripes[idx].cv = os_condition_variable_alloc();
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
  ctrl_state->ctrl_thread_entity_ctx_rw_mutex = os_rw_mutex_alloc();
  ctrl_state->ctrl_thread_entity_store = ctrl_entity_ctx_rw_store_alloc();
  ctrl_state->ctrl_thread_eval_cache = e_cache_alloc();
  ctrl_state->dmn_event_arena = arena_alloc();
  ctrl_state->user_entry_point_arena = arena_alloc();
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
  ctrl_state->u2csb_ring_size = KB(64);
  ctrl_state->u2csb_ring_base = push_array(arena, U8, ctrl_state->u2csb_ring_size);
  ctrl_state->u2csb_ring_mutex = os_mutex_alloc();
  ctrl_state->u2csb_ring_cv = os_condition_variable_alloc();
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

//- rjf: process memory cache key reading

internal HS_Key
ctrl_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us, B32 *out_is_stale)
{
  CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
  
  //- rjf: unpack process key
  U64 process_hash = ctrl_hash_from_handle(process);
  U64 process_slot_idx = process_hash%cache->slots_count;
  U64 process_stripe_idx = process_slot_idx%cache->stripes_count;
  CTRL_ProcessMemoryCacheSlot *process_slot = &cache->slots[process_slot_idx];
  CTRL_ProcessMemoryCacheStripe *process_stripe = &cache->stripes[process_stripe_idx];
  
  //- rjf: get the hash store root for this process; construct process node if it
  // doesn't exist
  HS_Root root = {0};
  {
    B32 node_found = 0;
    OS_MutexScopeR(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->handle, process))
        {
          node_found = 1;
          root = n->root;
          break;
        }
      }
    }
    if(!node_found) OS_MutexScopeW(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->handle, process))
        {
          node_found = 1;
          root = n->root;
          break;
        }
      }
      if(!node_found)
      {
        Arena *node_arena = arena_alloc();
        CTRL_ProcessMemoryCacheNode *node = push_array(node_arena, CTRL_ProcessMemoryCacheNode, 1);
        DLLPushBack(process_slot->first, process_slot->last, node);
        node->arena = node_arena;
        node->handle = process;
        node->root = hs_root_alloc();
        node->range_hash_slots_count = 1024;
        node->range_hash_slots = push_array(node_arena, CTRL_ProcessMemoryRangeHashSlot, node->range_hash_slots_count);
        root = node->root;
      }
    }
  }
  
  //- rjf: form ID for this process memory query
  HS_ID id = {0};
  {
    id.u128[0].u64[0] = vaddr_range.min & 0x00ffffffffffffffull;
    id.u128[0].u64[1] = vaddr_range.max & 0x00ffffffffffffffull;
    if(zero_terminated)
    {
      id.u128[0].u64[0] |= (1ull << 63);
    }
  }
  U64 range_hash = hs_little_hash_from_data(str8_struct(&id));
  
  //- rjf: form full key
  HS_Key key = hs_key_make(root, id);
  
  //- rjf: loop: try to look for current results, request if not there, wait if we can, repeat until we can't
  U64 mem_gen = ctrl_mem_gen();
  B32 key_is_stale = 0;
  B32 requested = 0;
  for(;;)
  {
    //- rjf: step 1: [read-only] try to look for current results for key's ID
    B32 id_exists = 0;
    B32 id_stale = 0;
    B32 id_working = 0;
    OS_MutexScopeR(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *process_n = process_slot->first; process_n != 0; process_n = process_n->next)
      {
        if(ctrl_handle_match(process_n->handle, process))
        {
          U64 range_slot_idx = range_hash%process_n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &process_n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *n = range_slot->first; n != 0; n = n->next)
          {
            if(hs_id_match(n->id, id))
            {
              id_exists = 1;
              id_stale = (n->mem_gen < mem_gen);
              id_working = (ins_atomic_u64_eval(&n->working_count) != 0);
              goto end_fast_lookup;
            }
          }
        }
      }
      end_fast_lookup:;
    }
    key_is_stale = id_stale;
    
    //- rjf: step 2: if the ID exists and is not stale, then we're done;
    // the hash store contains the most up-to-date representation of the
    // process memory for this key.
    if(id_exists && !id_stale)
    {
      break;
    }
    
    //- rjf: step 3: if the ID does not exist in the process' cache, then we
    // need to build a node for it. if that, or if the ID is stale, then also
    // request that that range is streamed.
    if(!id_exists || (id_exists && id_stale && !id_working))
    {
      B32 node_needs_stream = 0;
      U64 *node_working_count = 0;
      OS_MutexScopeW(process_stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *process_n = process_slot->first; process_n != 0; process_n = process_n->next)
        {
          if(ctrl_handle_match(process_n->handle, process))
          {
            U64 range_slot_idx = range_hash%process_n->range_hash_slots_count;
            CTRL_ProcessMemoryRangeHashSlot *range_slot = &process_n->range_hash_slots[range_slot_idx];
            CTRL_ProcessMemoryRangeHashNode *range_n = 0;
            for(CTRL_ProcessMemoryRangeHashNode *n = range_slot->first; n != 0; n = n->next)
            {
              if(hs_id_match(n->id, id))
              {
                range_n = n;
                break;
              }
            }
            if(range_n == 0)
            {
              range_n = push_array(process_n->arena, CTRL_ProcessMemoryRangeHashNode, 1);
              SLLQueuePush(range_slot->first, range_slot->last, range_n);
              range_n->vaddr_range = vaddr_range;
              range_n->zero_terminated = zero_terminated;
              range_n->id = id;
              ins_atomic_u64_inc_eval(&range_n->working_count);
              node_needs_stream = 1;
            }
            else
            {
              node_needs_stream = (range_n->mem_gen < mem_gen);
              if(node_needs_stream)
              {
                ins_atomic_u64_inc_eval(&range_n->working_count);
              }
            }
            node_working_count = &range_n->working_count;
            break;
          }
        }
      }
      if(node_needs_stream)
      {
        if(ctrl_u2ms_enqueue_req(key, process, vaddr_range, zero_terminated, endt_us))
        {
          async_push_work(ctrl_mem_stream_work, .working_counter = node_working_count);
          requested = 1;
        }
        else OS_MutexScopeR(process_stripe->rw_mutex)
        {
          for(CTRL_ProcessMemoryCacheNode *process_n = process_slot->first; process_n != 0; process_n = process_n->next)
          {
            if(ctrl_handle_match(process_n->handle, process))
            {
              U64 range_slot_idx = range_hash%process_n->range_hash_slots_count;
              CTRL_ProcessMemoryRangeHashSlot *range_slot = &process_n->range_hash_slots[range_slot_idx];
              for(CTRL_ProcessMemoryRangeHashNode *n = range_slot->first; n != 0; n = n->next)
              {
                if(hs_id_match(n->id, id))
                {
                  ins_atomic_u64_dec_eval(&n->working_count);
                  goto end_fail_work;
                }
              }
            }
          }
          end_fail_work:;
        }
      }
    }
    
    //- rjf: step 4: if we have no time to wait, then abort; if we submitted a
    // request, but the work is done and we have no results, then abort;
    // otherwise, wait on this process' stripe
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    else if(!id_working && requested)
    {
      break;
    }
    else OS_MutexScopeR(process_stripe->rw_mutex)
    {
      os_condition_variable_wait_rw_r(process_stripe->cv, process_stripe->rw_mutex, endt_us);
    }
  }
  if(out_is_stale)
  {
    *out_is_stale = key_is_stale;
  }
  return key;
}

//- rjf: process memory cache reading helpers

internal CTRL_ProcessMemorySlice
ctrl_process_memory_slice_from_vaddr_range(Arena *arena, CTRL_Handle process, Rng1U64 range, U64 endt_us)
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
        B32 page_is_stale = 0;
        HS_Key page_key = ctrl_key_from_process_vaddr_range(process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0, endt_us, &page_is_stale);
        U128 page_hash = hs_hash_from_key(page_key, 0);
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

internal B32
ctrl_process_memory_read(CTRL_Handle process, Rng1U64 range, B32 *is_stale_out, void *out, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  U64 needed_size = dim_1u64(range);
  CTRL_ProcessMemorySlice slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process, range, endt_us);
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
      ctrl_process_memory_slice_from_vaddr_range(temp.arena, task->process, task->range, endt_us);
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
ctrl_reg_block_from_thread(Arena *arena, CTRL_EntityCtx *ctx, CTRL_Handle handle)
{
  CTRL_ThreadRegCache *cache = &ctrl_state->thread_reg_cache;
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(ctx, handle);
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
ctrl_tls_root_vaddr_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle)
{
  U64 result = dmn_tls_root_vaddr_from_thread(handle.dmn_handle);
  return result;
}

internal U64
ctrl_rip_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(ctx, handle);
  Arch arch = thread_entity->arch;
  void *block = ctrl_reg_block_from_thread(scratch.arena, ctx, handle);
  U64 result = regs_rip_from_arch_block(arch, block);
  scratch_end(scratch);
  return result;
}

internal U64
ctrl_rsp_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(ctx, handle);
  Arch arch = thread_entity->arch;
  void *block = ctrl_reg_block_from_thread(scratch.arena, ctx, handle);
  U64 result = regs_rsp_from_arch_block(arch, block);
  scratch_end(scratch);
  return result;
}

//- rjf: thread register writing

internal B32
ctrl_thread_write_reg_block(CTRL_Handle thread, void *block)
{
  // TODO(rjf): @callstacks immediately reflect this in the call stack cache
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
        PE_IntelPdata *pdatas = n->pdatas;
        U64 pdatas_count = n->pdatas_count;
        if(n->pdatas_count != 0 && voff >= n->pdatas[0].voff_first)
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

internal String8
ctrl_raddbg_data_from_module(Arena *arena, CTRL_Handle module_handle)
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
      result = push_str8_copy(arena, n->raddbg_data);
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
ctrl_unwind_step__pe_x64(CTRL_Handle process_handle, CTRL_Handle module_handle, U64 module_base_vaddr, REGS_RegBlockX64 *regs, U64 endt_us)
{
  B32 is_stale = 0;
  B32 is_good = 1;
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: unpack parameters
  //
  U64 rip_voff = regs->rip.u64 - module_base_vaddr;
  
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
        inst_good = ctrl_process_memory_read(process_handle, r1u64(read_vaddr, read_vaddr+sizeof(inst)), &is_stale, inst, endt_us);
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
        inst_byte_good = ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &inst_byte, endt_us);
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
          check_inst_byte_good = ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &check_inst_byte, endt_us);
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
              imm_good = ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm, endt_us);
            }
            if(!imm_good || is_stale)
            {
              keep_parsing = 0;
            }
            if(imm_good)
            {
              U64 next_vaddr = (U64)(imm_vaddr + sizeof(imm) + imm);
              U64 next_voff = next_vaddr - module_base_vaddr; // TODO(rjf): verify that this offset is from module base vaddr, not section
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
              next_inst_byte_good = ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &next_inst_byte, endt_us);
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
      is_good = is_good && ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &inst_byte, endt_us);
      is_good = is_good && !is_stale;
      read_vaddr += 1;
      
      //- rjf: extract rex from instruction byte
      U8 rex = 0;
      if((inst_byte & 0xF0) == 0x40)
      {
        rex = inst_byte & 0xF; // rex prefix
        is_good = is_good && ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &inst_byte, endt_us);
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
          if(!ctrl_process_memory_read_struct(process_handle, sp, &is_stale, &value, endt_us) ||
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
          if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm, endt_us) ||
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
          if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm, endt_us) ||
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
          if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &modrm, endt_us) ||
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
              if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm8, endt_us) ||
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
              if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm, endt_us) ||
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
          if(!ctrl_process_memory_read_struct(process_handle, sp, &is_stale, &new_ip, endt_us) ||
             is_stale)
          {
            is_good = 0;
            break;
          }
          
          // rjf: read 2-byte immediate & advance stack pointer
          U16 imm = 0;
          if(!ctrl_process_memory_read_struct(process_handle, read_vaddr, &is_stale, &imm, endt_us) ||
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
          if(!ctrl_process_memory_read_struct(process_handle, sp, &is_stale, &new_ip, endt_us) ||
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
      PE_UnwindInfo unwind_info = {0};
      if(!ctrl_process_memory_read_struct(process_handle, module_base_vaddr+unwind_info_off, &is_stale, &unwind_info, endt_us) ||
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
      PE_UnwindInfo unwind_info = {0};
      good_unwind_info = good_unwind_info && ctrl_process_memory_read_struct(process_handle, module_base_vaddr+unwind_info_off, &is_stale, &unwind_info, endt_us);
      PE_UnwindCode *unwind_codes = push_array(scratch.arena, PE_UnwindCode, unwind_info.codes_num);
      good_unwind_info = good_unwind_info && ctrl_process_memory_read(process_handle, r1u64(module_base_vaddr+unwind_info_off+sizeof(unwind_info),
                                                                                            module_base_vaddr+unwind_info_off+sizeof(unwind_info)+sizeof(PE_UnwindCode)*unwind_info.codes_num),
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
      PE_UnwindCode *code_ptr = unwind_codes;
      PE_UnwindCode *code_opl = unwind_codes + unwind_info.codes_num;
      for(PE_UnwindCode *next_code_ptr = 0; code_ptr < code_opl; code_ptr = next_code_ptr)
      {
        // rjf: unpack opcode info
        U32 op_code = PE_UNWIND_OPCODE_FROM_FLAGS(code_ptr->flags);
        U32 op_info = PE_UNWIND_INFO_FROM_FLAGS(code_ptr->flags);
        U32 slot_count = pe_slot_count_from_unwind_op_code(op_code);
        if(op_code == PE_UnwindOpCode_ALLOC_LARGE && op_info == 1)
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
            case PE_UnwindOpCode_PUSH_NONVOL:
            {
              // rjf: read value from stack pointer
              U64 rsp = regs->rsp.u64;
              U64 value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, rsp, &is_stale, &value, endt_us) ||
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
            
            case PE_UnwindOpCode_ALLOC_LARGE:
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
            
            case PE_UnwindOpCode_ALLOC_SMALL:
            {
              // rjf: advance stack pointer
              regs->rsp.u64 += op_info*8 + 8;
            }break;
            
            case PE_UnwindOpCode_SET_FPREG:
            {
              // rjf: put stack pointer back to the frame base
              regs->rsp.u64 = frame_base;
            }break;
            
            case PE_UnwindOpCode_SAVE_NONVOL:
            {
              // rjf: read value from frame base
              U64 off = code_ptr[1].u16*8;
              U64 addr = frame_base + off;
              U64 value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, addr, &is_stale, &value, endt_us) ||
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
            
            case PE_UnwindOpCode_SAVE_NONVOL_FAR:
            {
              // rjf: read value from frame base
              U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
              U64 addr = frame_base + off;
              U64 value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, addr, &is_stale, &value, endt_us) ||
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
            
            case PE_UnwindOpCode_EPILOG:
            {
              keep_parsing = 1;
            }break;
            
            case PE_UnwindOpCode_SPARE_CODE:
            {
              // TODO(rjf): ???
              keep_parsing = 0;
              is_good = 0;
            }break;
            
            case PE_UnwindOpCode_SAVE_XMM128:
            {
              // rjf: read new register values
              U8 buf[16];
              U64 off = code_ptr[1].u16*16;
              U64 addr = frame_base + off;
              if(!ctrl_process_memory_read(process_handle, r1u64(addr, addr+sizeof(buf)), &is_stale, buf, endt_us))
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              
              // rjf: commit to register
              void *xmm_reg = (&regs->zmm0) + op_info;
              MemoryCopy(xmm_reg, buf, sizeof(buf));
            }break;
            
            case PE_UnwindOpCode_SAVE_XMM128_FAR:
            {
              // rjf: read new register values
              U8 buf[16];
              U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
              U64 addr = frame_base + off;
              if(!ctrl_process_memory_read(process_handle, r1u64(addr, addr+16), &is_stale, buf, endt_us) ||
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
            
            case PE_UnwindOpCode_PUSH_MACHFRAME:
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
              if(!ctrl_process_memory_read_struct(process_handle, sp_adj, &is_stale, &ip_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_ip = sp_adj + 8;
              U16 ss_value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, sp_after_ip, &is_stale, &ss_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_ss = sp_after_ip + 8;
              U64 rflags_value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, sp_after_ss, &is_stale, &rflags_value, endt_us) ||
                 is_stale)
              {
                keep_parsing = 0;
                is_good = 0;
                break;
              }
              U64 sp_after_rflags = sp_after_ss + 8;
              U64 sp_value = 0;
              if(!ctrl_process_memory_read_struct(process_handle, sp_after_rflags, &is_stale, &sp_value, endt_us) ||
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
        if(!(flags & PE_UnwindInfoFlag_CHAINED))
        {
          break;
        }
        U64 code_count_rounded = AlignPow2(unwind_info.codes_num, sizeof(PE_UnwindCode));
        U64 code_size = code_count_rounded*sizeof(PE_UnwindCode);
        U64 chained_pdata_off = unwind_info_off + sizeof(PE_UnwindInfo) + code_size;
        last_pdata = pdata;
        pdata = push_array(scratch.arena, PE_IntelPdata, 1);
        if(!ctrl_process_memory_read_struct(process_handle, module_base_vaddr+chained_pdata_off, &is_stale, pdata, endt_us) ||
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
    if(!ctrl_process_memory_read_struct(process_handle, rsp, &is_stale, &new_rip, endt_us) ||
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

//- rjf: abstracted unwind step

internal CTRL_UnwindStepResult
ctrl_unwind_step(CTRL_Handle process, CTRL_Handle module, U64 module_base_vaddr, Arch arch, void *reg_block, U64 endt_us)
{
  CTRL_UnwindStepResult result = {0};
  switch(arch)
  {
    default:{}break;
    case Arch_x64:
    {
      result = ctrl_unwind_step__pe_x64(process, module, module_base_vaddr, (REGS_RegBlockX64 *)reg_block, endt_us);
    }break;
  }
  return result;
}

//- rjf: abstracted full unwind

internal CTRL_Unwind
ctrl_unwind_from_thread(Arena *arena, CTRL_EntityCtx *ctx, CTRL_Handle thread, U64 endt_us)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_Unwind unwind = {0};
  unwind.flags |= CTRL_UnwindFlag_Error;
  
  //- rjf: unpack args
  CTRL_Entity *thread_entity = ctrl_entity_from_handle(ctx, thread);
  CTRL_Entity *process_entity = thread_entity->parent;
  Arch arch = thread_entity->arch;
  U64 arch_reg_block_size = regs_block_size_from_arch(arch);
  
  //- rjf: grab initial register block
  void *regs_block = ctrl_reg_block_from_thread(scratch.arena, ctx, thread);
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
      U64 rsp = regs_rsp_from_arch_block(arch, regs_block);
      CTRL_Entity *module = &ctrl_entity_nil;
      for(CTRL_Entity *m = process_entity->first; m != &ctrl_entity_nil; m = m->next)
      {
        if(m->kind == CTRL_EntityKind_Module && contains_1u64(m->vaddr_range, rip))
        {
          module = m;
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
      CTRL_UnwindStepResult step = ctrl_unwind_step(process_entity->handle, module->handle, module->vaddr_range.min, arch, regs_block, endt_us);
      unwind.flags |= step.flags;
      if(step.flags & CTRL_UnwindFlag_Error ||
         regs_rsp_from_arch_block(arch, regs_block) == 0 ||
         regs_rip_from_arch_block(arch, regs_block) == 0 ||
         (regs_rsp_from_arch_block(arch, regs_block) == rsp &&
          regs_rip_from_arch_block(arch, regs_block) == rip))
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
ctrl_call_stack_from_unwind(Arena *arena, CTRL_Entity *process, CTRL_Unwind *base_unwind)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *di_scope = di_scope_open();
  Arch arch = process->arch;
  CTRL_CallStack result = {0};
  {
    typedef struct FrameNode FrameNode;
    struct FrameNode
    {
      FrameNode *next;
      CTRL_CallStackFrame v;
    };
    
    //- rjf: gather all frames
    FrameNode *first_frame = 0;
    FrameNode *last_frame = 0;
    U64 frame_count = 0;
    for(U64 base_frame_idx = 0; base_frame_idx < base_unwind->frames.count; base_frame_idx += 1)
    {
      // rjf: unpack
      CTRL_UnwindFrame *src = &base_unwind->frames.v[base_frame_idx];
      U64 rip_vaddr = regs_rip_from_arch_block(arch, src->regs);
      CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
      U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
      DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
      RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
      RDI_Scope *scope = rdi_scope_from_voff(rdi, rip_voff);
      
      // rjf: build inline frames (minus parent & inline depth)
      FrameNode *first_inline_frame = 0;
      FrameNode *last_inline_frame = 0;
      U64 inline_frame_count = 0;
      for(RDI_Scope *s = scope;
          s->inline_site_idx != 0;
          s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
      {
        FrameNode *dst_inline = push_array(scratch.arena, FrameNode, 1);
        if(first_inline_frame == 0)
        {
          first_inline_frame = dst_inline;
        }
        last_inline_frame = dst_inline;
        SLLQueuePush(first_frame, last_frame, dst_inline);
        dst_inline->v.unwind_count = base_frame_idx;
        dst_inline->v.regs         = src->regs;
        frame_count += 1;
        inline_frame_count += 1;
      }
      
      // rjf: build concrete frame
      FrameNode *dst_base = push_array(scratch.arena, FrameNode, 1);
      SLLQueuePush(first_frame, last_frame, dst_base);
      dst_base->v.unwind_count = base_frame_idx;
      dst_base->v.regs         = src->regs;
      frame_count += 1;
      
      // rjf: hook up inline frames to point to concrete frame, and to account for inline depth
      U64 inline_frame_idx = 0;
      for(FrameNode *inline_frame = first_inline_frame; inline_frame != 0; inline_frame = inline_frame->next, inline_frame_idx += 1)
      {
        inline_frame->v.inline_depth = inline_frame_count - inline_frame_idx;
        if(inline_frame == last_inline_frame)
        {
          break;
        }
      }
    }
    
    //- rjf: package
    result.frames_count = frame_count; 
    result.frames = push_array(arena, CTRL_CallStackFrame, result.frames_count);
    result.concrete_frames_count = base_unwind->frames.count;
    result.concrete_frames = push_array(arena, CTRL_CallStackFrame *, result.concrete_frames_count);
    {
      U64 idx = 0;
      U64 concrete_idx = 0;
      for(FrameNode *n = first_frame; n != 0; n = n->next, idx += 1)
      {
        MemoryCopyStruct(&result.frames[idx], &n->v);
        if(n->v.inline_depth == 0 && concrete_idx < result.concrete_frames_count)
        {
          result.concrete_frames[concrete_idx] = &result.frames[idx];
          concrete_idx += 1;
        }
      }
    }
  }
  di_scope_close(di_scope);
  scratch_end(scratch);
  return result;
}

internal CTRL_CallStackFrame *
ctrl_call_stack_frame_from_unwind_and_inline_depth(CTRL_CallStack *call_stack, U64 unwind_count, U64 inline_depth)
{
  CTRL_CallStackFrame *f = 0;
  {
    U64 base_frame_idx = 0;
    for(U64 idx = 0; idx < call_stack->frames_count; idx += 1)
    {
      if(call_stack->frames[idx].inline_depth == 0)
      {
        if(base_frame_idx == unwind_count)
        {
          f = &call_stack->frames[idx];
          break;
        }
        base_frame_idx += 1;
      }
    }
    if(f != 0 && call_stack->frames + inline_depth < f)
    {
      f -= inline_depth;
    }
  }
  return f;
}

////////////////////////////////
//~ rjf: Call Stack Cache Functions

internal CTRL_CallStack
ctrl_call_stack_from_thread(CTRL_Scope *scope, CTRL_EntityCtx *entity_ctx, CTRL_Entity *thread, B32 high_priority, U64 endt_us)
{
  CTRL_CallStack call_stack = {0};
  CTRL_CallStackCache *cache = &ctrl_state->call_stack_cache;
  
  //////////////////////////////
  //- rjf: unpack thread
  //
  CTRL_Handle handle = thread->handle;
  U64 hash = ctrl_hash_from_handle(handle);
  U64 slot_idx = hash%cache->slots_count;
  U64 stripe_idx = slot_idx%cache->stripes_count;
  CTRL_CallStackCacheSlot *slot = &cache->slots[slot_idx];
  CTRL_CallStackCacheStripe *stripe = &cache->stripes[stripe_idx];
  U64 reg_gen = ctrl_reg_gen();
  U64 mem_gen = ctrl_mem_gen();
  
  //////////////////////////////
  //- rjf: loop: try to grab cached call stack; request; wait
  //
  B32 can_request = !ins_atomic_u64_eval(&ctrl_state->ctrl_thread_run_state);
  B32 did_request = 0;
  OS_MutexScopeR(stripe->rw_mutex) for(;;)
  {
    ////////////////////////////
    //- rjf: try to grab cached
    //
    B32 is_good = 0;
    B32 is_stale = 1;
    B32 is_working = 0;
    CTRL_CallStackCacheNode *node = 0;
    {
      for(CTRL_CallStackCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->thread, handle))
        {
          node = n;
          is_good    = 1;
          is_stale   = (reg_gen > n->reg_gen || mem_gen > n->mem_gen);
          is_working = (n->working_count > 0);
          call_stack = n->call_stack;
          ctrl_scope_touch_call_stack_node__stripe_r_guarded(scope, stripe, n);
          break;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: create node if needed
    //
    if(!is_good) OS_MutexScopeRWPromote(stripe->rw_mutex)
    {
      node = 0;
      for(CTRL_CallStackCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->thread, handle))
        {
          node = n;
          break;
        }
      }
      if(node == 0)
      {
        node = push_array(stripe->arena, CTRL_CallStackCacheNode, 1);
        DLLPushBack(slot->first, slot->last, node);
        node->thread = thread->handle;
      }
    }
    
    ////////////////////////////
    //- rjf: request if needed
    //
    if(can_request && node != 0 && !is_working && is_stale)
    {
      if(ctrl_u2csb_enqueue_req(thread->handle, endt_us))
      {
        did_request = 1;
        is_working = 1;
        ins_atomic_u64_inc_eval(&node->working_count);
        async_push_work(ctrl_call_stack_build_work, .priority = high_priority ? ASYNC_Priority_High : ASYNC_Priority_Low);
      }
    }
    
    ////////////////////////////
    //- rjf: good, or timeout? -> exit
    //
    if(!can_request || !is_stale || os_now_microseconds() >= endt_us)
    {
      break;
    }
    
    ////////////////////////////
    //- rjf: time to wait for new result? -> wait
    //
    if(did_request && !is_working)
    {
      break;
    }
    else if(did_request)
    {
      os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  return call_stack;
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
    OS_MutexScopeW(ctrl_state->ctrl_thread_entity_ctx_rw_mutex)
    {
      ctrl_entity_store_apply_events(ctrl_state->ctrl_thread_entity_store, events);
    }
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
      ins_atomic_u64_eval_assign(&ctrl_state->ctrl_thread_run_state, 1);
      for(CTRL_MsgNode *msg_n = msgs.first; msg_n != 0; msg_n = msg_n->next)
      {
        CTRL_Msg *msg = &msg_n->v;
        {
          log_infof("user2ctrl_msg:{kind:\"%S\"}\n", ctrl_string_from_msg_kind(msg->kind));
        }
        
        //- rjf: unpack per-message parameterizations & store
        {
          MemoryCopyArray(ctrl_state->exception_code_filters, msg->exception_code_filters);
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
            CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
            String8 path = msg->path;
            CTRL_Entity *module = ctrl_entity_from_handle(entity_ctx, msg->entity);
            CTRL_Entity *debug_info_path = ctrl_entity_child_from_kind(module, CTRL_EntityKind_DebugInfoPath);
            DI_Key old_dbgi_key = {debug_info_path->string, debug_info_path->timestamp};
            di_close(&old_dbgi_key);
            OS_MutexScopeW(ctrl_state->ctrl_thread_entity_ctx_rw_mutex) ctrl_entity_equip_string(ctrl_state->ctrl_thread_entity_store, debug_info_path, path);
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
      ins_atomic_u64_eval_assign(&ctrl_state->ctrl_thread_run_state, 0);
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
ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, CTRL_EvalScope *eval_scope, CTRL_Handle process, CTRL_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  if(user_bps->first == 0) { return; }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *di_scope = eval_scope->di_scope;
  CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
  CTRL_Entity *module_entity = ctrl_entity_from_handle(entity_ctx, module);
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
      
      //- rjf: expression-based breakpoints
      case CTRL_UserBreakpointKind_Expression:
      {
        String8 expr = bp->string;
        E_Value value = e_value_from_string(expr);
        if(value.u64 != 0)
        {
          DMN_Trap trap = {process.dmn_handle, value.u64, (U64)bp};
          trap.flags = ctrl_dmn_trap_flags_from_user_breakpoint_flags(bp->flags);
          trap.size = bp->size;
          dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
        }
      }break;
    }
  }
  scratch_end(scratch);
}

internal void
ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, CTRL_EvalScope *eval_scope, CTRL_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out)
{
  for(CTRL_UserBreakpointNode *n = user_bps->first; n != 0; n = n->next)
  {
    CTRL_UserBreakpoint *bp = &n->v;
    if(bp->kind == CTRL_UserBreakpointKind_Expression)
    {
      String8 expr = bp->string;
      E_Value value = e_value_from_string(expr);
      if(value.u64 != 0)
      {
        DMN_Trap trap = {process.dmn_handle, value.u64, (U64)bp};
        trap.flags = ctrl_dmn_trap_flags_from_user_breakpoint_flags(bp->flags);
        trap.size = bp->size;
        dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
      }
    }
  }
}

internal void
ctrl_thread__append_program_defined_bp_traps(Arena *arena, CTRL_Entity *bp, DMN_TrapChunkList *traps_out)
{
  CTRL_Entity *process = bp->parent;
  DMN_Trap trap =
  {
    .process = process->handle.dmn_handle,
    .vaddr = bp->vaddr_range.min,
    .id = ((U64)bp|bit64),
    .flags = ctrl_dmn_trap_flags_from_user_breakpoint_flags(bp->bp_flags),
    .size = (U32)dim_1u64(bp->vaddr_range),
  };
  dmn_trap_chunk_list_push(arena, traps_out, 256, &trap);
}

//- rjf: module lifetime open/close work

internal void
ctrl_thread__module_open(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range, String8 path)
{
  //////////////////////////////
  //- rjf: parse module image info
  //
  Arena *arena = arena_alloc();
  PE_IntelPdata *pdatas = 0;
  U64 pdatas_count = 0;
  U64 entry_point_voff = 0;
  Rng1U64 tls_vaddr_range = {0};
  U32 pdb_dbg_time = 0;
  U32 pdb_dbg_age = 0;
  Guid pdb_dbg_guid = {0};
  String8 pdb_dbg_path = str8_zero();
  U32 rdi_dbg_time = 0;
  Guid rdi_dbg_guid = {0};
  String8 rdi_dbg_path = str8_zero();
  String8 raddbg_data = str8_zero();
  Rng1U64 raddbg_section_voff_range = r1u64(0, 0);
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
    U64 file_header_off = dos_header.coff_file_offset + sizeof(pe_magic);
    COFF_FileHeader file_header = {0};
    if(is_valid)
    {
      if(!dmn_process_read_struct(process.dmn_handle, vaddr_range.min + file_header_off, &file_header))
      {
        is_valid = 0;
      }
    }
    
    //- rjf: unpack range of optional extension header
    U32 opt_ext_size = file_header.optional_header_size;
    Rng1U64 opt_ext_off_range = r1u64(file_header_off + sizeof(COFF_FileHeader),
                                      file_header_off + sizeof(COFF_FileHeader) + opt_ext_size);
    
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
      
      // rjf: find number of data directories
      U32 data_dir_max = (opt_ext_size - reported_data_dir_offset) / sizeof(PE_DataDirectory);
      data_dir_count = ClampTop(reported_data_dir_count, data_dir_max);
      
      // rjf: grab pdatas from exceptions section
      if(data_dir_count > PE_DataDirectoryIndex_EXCEPTIONS)
      {
        PE_DataDirectory dir = {0};
        dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_EXCEPTIONS, &dir);
        Rng1U64 pdatas_voff_range = r1u64((U64)dir.virt_off, (U64)dir.virt_off + (U64)dir.virt_size);
        pdatas_count = dim_1u64(pdatas_voff_range)/sizeof(PE_IntelPdata);
        pdatas = push_array(arena, PE_IntelPdata, pdatas_count);
        dmn_process_read(process.dmn_handle, r1u64(vaddr_range.min + pdatas_voff_range.min, vaddr_range.min + pdatas_voff_range.max), pdatas);
      }
      
      // rjf: extract tls header
      PE_TLSHeader64 tls_header = {0};
      if(data_dir_count > PE_DataDirectoryIndex_TLS)
      {
        PE_DataDirectory dir = {0};
        dmn_process_read_struct(process.dmn_handle, vaddr_range.min + opt_ext_off_range.min + reported_data_dir_offset + sizeof(PE_DataDirectory)*PE_DataDirectoryIndex_TLS, &dir);
        Rng1U64 tls_voff_range = r1u64((U64)dir.virt_off, (U64)dir.virt_off + (U64)dir.virt_size);
        switch(file_header.machine)
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
      
      // rjf: extract copy of module's raddbg data
      {
        Temp scratch = scratch_begin(0, 0);
        U64 sec_array_off = opt_ext_off_range.max;
        U64 sec_count = file_header.section_count;
        COFF_SectionHeader *sec = push_array(scratch.arena, COFF_SectionHeader, sec_count);
        dmn_process_read(process.dmn_handle, r1u64(vaddr_range.min + sec_array_off, vaddr_range.min + sec_array_off + sec_count*sizeof(COFF_SectionHeader)), sec);
        for EachIndex(idx, sec_count)
        {
          String8 section_name = str8_cstring((char *)sec[idx].name);
          if(str8_match(section_name, str8_lit(".raddbg"), 0))
          {
            raddbg_section_voff_range.min = sec[idx].voff;
            raddbg_section_voff_range.max = sec[idx].voff + sec[idx].vsize;
          }
        }
        raddbg_data.size = dim_1u64(raddbg_section_voff_range);
        raddbg_data.str = push_array(arena, U8, raddbg_data.size);
        dmn_process_read(process.dmn_handle, r1u64(vaddr_range.min + raddbg_section_voff_range.min,
                                                   vaddr_range.min + raddbg_section_voff_range.max), raddbg_data.str);
        scratch_end(scratch);
      }
      
      // rjf: if we have a raddbg section, mark the first byte as 1, to signify attachment
      if(raddbg_section_voff_range.max != raddbg_section_voff_range.min)
      {
        U8 new_value = 1;
        dmn_process_write_struct(process.dmn_handle, vaddr_range.min + raddbg_section_voff_range.min, &new_value);
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
        initial_debug_info_path = push_str8_copy(arena, path_normalized_from_string(scratch.arena, candidate_path));
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
        node->pdatas = pdatas;
        node->pdatas_count = pdatas_count;
        node->entry_point_voff = entry_point_voff;
        node->initial_debug_info_path = initial_debug_info_path;
        node->raddbg_section_voff_range = raddbg_section_voff_range;
        node->raddbg_data = raddbg_data;
      }
    }
  }
}

internal void
ctrl_thread__module_close(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range)
{
  //////////////////////////////
  //- rjf: evict module image info from cache
  //
  Rng1U64 raddbg_section_voff_range = {0};
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
        raddbg_section_voff_range = node->raddbg_section_voff_range;
        DLLRemove(slot->first, slot->last, node);
        arena_release(node->arena);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: write 0 into first byte of raddbg data section, to signify detachment
  //
  if(raddbg_section_voff_range.max != raddbg_section_voff_range.min)
  {
    U8 new_value = 0;
    dmn_process_write_struct(process.dmn_handle, vaddr_range.min + raddbg_section_voff_range.min, &new_value);
  }
}

//- rjf: attached process running/event gathering

internal DMN_Event *
ctrl_thread__next_dmn_event(Arena *arena, DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg, DMN_RunCtrls *run_ctrls, CTRL_Spoof *spoof)
{
  ProfBeginFunction();
  DMN_Event *event = push_array(arena, DMN_Event, 1);
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
  
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
              CTRL_Entity *process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, ev->process));
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
        CTRL_Entity *spoof_process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, spoof->process));
        Arch arch = spoof_process->arch;
        size_of_spoof = bit_size_from_arch(arch)/8;
        dmn_process_read(spoof_process->handle.dmn_handle, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
      }
      
      // rjf: set spoof
      if(do_spoof) ProfScope("set spoof")
      {
        dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof->new_ip_value);
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
        dmn_process_write(spoof->process, r1u64(spoof->vaddr, spoof->vaddr+size_of_spoof), &spoof_old_ip_value);
      }
    }
  }
  
  //- rjf: irrespective of what event came back, we should ALWAYS check the
  // spoof's thread and see if it hit the spoof address, because we may have
  // simply been sent other debug events first
  if(spoof != 0)
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, spoof->thread));
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
      CTRL_Entity *module_ent = ctrl_entity_from_handle(entity_ctx, module_handle);
      CTRL_Entity *process_ent = ctrl_process_from_entity(module_ent);
      String8 module_path = event->string;
      ctrl_thread__module_close(process_ent->handle, module_handle, module_ent->vaddr_range);
      out_evt->kind       = CTRL_EventKind_EndModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = module_handle;
      out_evt->string     = module_path;
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
    case DMN_EventKind_SetThreadColor:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_ThreadColor;
      out_evt->msg_id     = msg->msg_id;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->entity_id  = event->code;
      out_evt->rgba       = event->user_data;
    }break;
    case DMN_EventKind_SetBreakpoint:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_SetBreakpoint;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->vaddr_rng  = r1u64(event->address, event->address+event->size);
      out_evt->bp_flags   = ctrl_user_breakpoint_flags_from_dmn_trap_flags(event->flags);
    }break;
    case DMN_EventKind_UnsetBreakpoint:
    {
      // TODO(rjf): this needs to be reflected in the resolved trap list too!!!!!!!!
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_UnsetBreakpoint;
      out_evt->entity     = ctrl_handle_make(CTRL_MachineID_Local, event->thread);
      out_evt->parent     = ctrl_handle_make(CTRL_MachineID_Local, event->process);
      out_evt->vaddr_rng  = r1u64(event->address, event->address+event->size);
      out_evt->bp_flags   = ctrl_user_breakpoint_flags_from_dmn_trap_flags(event->flags);
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
     (entity_ctx->entity_kind_counts[CTRL_EntityKind_Module] > 256 ||
      entity_ctx->entity_kind_counts[CTRL_EntityKind_Module] == 1))
  {
    U64 endt_us = os_now_microseconds() + 1000000;
    
    //- rjf: unpack event
    CTRL_Handle process_handle = ctrl_handle_make(CTRL_MachineID_Local, event->process);
    CTRL_Handle loaded_module_handle = ctrl_handle_make(CTRL_MachineID_Local, event->module);
    CTRL_Entity *process = ctrl_entity_from_handle(entity_ctx, process_handle);
    CTRL_Entity *loaded_module = ctrl_entity_from_handle(entity_ctx, loaded_module_handle);
    
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
          CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
          U64 regs_size = regs_block_size_from_arch(entity->arch);
          void *regs = ctrl_reg_block_from_thread(scratch.arena, entity_ctx, entity->handle);
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
      
    }break;
  }
  return result;
}

//- rjf: control thread eval scopes

internal CTRL_EvalScope *
ctrl_thread__eval_scope_begin(Arena *arena, CTRL_Entity *thread)
{
  CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
  CTRL_EvalScope *scope = push_array(arena, CTRL_EvalScope, 1);
  scope->di_scope = di_scope_open();
  
  //////////////////////////////
  //- rjf: unpack thread
  //
  Arch arch = thread->arch;
  U64 thread_rip_vaddr = dmn_rip_from_thread(thread->handle.dmn_handle);
  CTRL_Entity *process = ctrl_process_from_entity(thread);
  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
  U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
  
  //////////////////////////////
  //- rjf: gather evaluation modules
  //
  U64 eval_modules_count = Max(1, entity_ctx->entity_kind_counts[CTRL_EntityKind_Module]);
  E_Module *eval_modules = push_array(arena, E_Module, eval_modules_count);
  E_Module *eval_modules_primary = &eval_modules[0];
  eval_modules_primary->rdi = &rdi_parsed_nil;
  eval_modules_primary->vaddr_range = r1u64(0, max_U64);
  {
    U64 eval_module_idx = 0;
    for(CTRL_Entity *machine = entity_ctx->root->first;
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
          eval_modules[eval_module_idx].rdi         = di_rdi_from_key(scope->di_scope, &dbgi_key, max_U64);
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
  
  //////////////////////////////
  //- rjf: select evaluation cache
  //
  e_select_cache(ctrl_state->ctrl_thread_eval_cache);
  
  //////////////////////////////
  //- rjf: build base evaluation context
  //
  {
    E_BaseCtx *ctx = &scope->base_ctx;
    
    //- rjf: fill instruction pointer info
    ctx->thread_ip_vaddr     = thread_rip_vaddr;
    ctx->thread_ip_voff      = thread_rip_voff;
    ctx->thread_arch         = thread->arch;
    ctx->thread_reg_space = e_space_make(CTRL_EvalSpaceKind_Entity);
    ctx->thread_reg_space.u64_0 = (U64)thread;
    
    //- rjf: fill modules
    ctx->modules        = eval_modules;
    ctx->modules_count  = eval_modules_count;
    ctx->primary_module = eval_modules_primary;
    
    //- rjf: fill space hooks
    ctx->space_read  = ctrl_eval_space_read;
  }
  e_select_base_ctx(&scope->base_ctx);
  
  //////////////////////////////
  //- rjf: build IR evaluation context
  //
  {
    E_IRCtx *ctx = &scope->ir_ctx;
    ctx->regs_map      = ctrl_string2reg_from_arch(arch);
    ctx->reg_alias_map = ctrl_string2alias_from_arch(arch);
    ctx->locals_map    = e_push_locals_map_from_rdi_voff(arena, eval_modules_primary->rdi, thread_rip_voff);
    ctx->member_map    = e_push_member_map_from_rdi_voff(arena, eval_modules_primary->rdi, thread_rip_voff);
    ctx->macro_map     = push_array(arena, E_String2ExprMap, 1);
    ctx->macro_map[0]  = e_string2expr_map_make(arena, 512);
    ctx->auto_hook_map = push_array(arena, E_AutoHookMap, 1);
    ctx->auto_hook_map[0] = e_auto_hook_map_make(arena, 512);
  }
  e_select_ir_ctx(&scope->ir_ctx);
  
  //////////////////////////////
  //- rjf: build eval interpretation context
  //
  {
    E_InterpretCtx *ctx = &scope->interpret_ctx;
    ctx->space_read    = ctrl_eval_space_read;
    ctx->primary_space = eval_modules_primary->space;
    ctx->reg_arch      = eval_modules_primary->arch;
    ctx->reg_space     = e_space_make(CTRL_EvalSpaceKind_Entity);
    ctx->reg_space.u64_0 = (U64)thread;
    ctx->module_base   = push_array(arena, U64, 1);
    ctx->module_base[0]= module->vaddr_range.min;
    ctx->frame_base    = push_array(arena, U64, 1);
    // TODO(rjf): need to compute this out here somehow... ctx->frame_base[0] = ;
    ctx->tls_base      = push_array(arena, U64, 1);
  }
  e_select_interpret_ctx(&scope->interpret_ctx, eval_modules_primary->rdi, thread_rip_voff);
  
  return scope;
}

internal void
ctrl_thread__eval_scope_end(CTRL_EvalScope *scope)
{
  di_scope_close(scope->di_scope);
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
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: produce full stdout/stderr/stdin paths
  String8 stdout_path = path_absolute_dst_from_relative_dst_src(scratch.arena, msg->stdout_path, msg->path);
  String8 stdin_path  = path_absolute_dst_from_relative_dst_src(scratch.arena, msg->stdin_path, msg->path);
  String8 stderr_path = path_absolute_dst_from_relative_dst_src(scratch.arena, msg->stderr_path, msg->path);
  
  //- rjf: obtain stdout/stderr/stdin handles
  OS_Handle stdout_handle = {0};
  OS_Handle stderr_handle = {0};
  OS_Handle stdin_handle  = {0};
  if(stdout_path.size != 0)
  {
    OS_Handle f = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Read, stdout_path);
    os_file_close(f);
    stdout_handle = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Append|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, msg->stdout_path);
  }
  if(stderr_path.size != 0)
  {
    OS_Handle f = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Read, stderr_path);
    os_file_close(f);
    stderr_handle = os_file_open(OS_AccessFlag_Write|OS_AccessFlag_Append|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, msg->stderr_path);
  }
  if(stdin_path.size != 0)
  {
    stdin_handle = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite|OS_AccessFlag_Inherited, stdin_path);
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
  CTRL_EntityCtxRWStore *entity_ctx_rw_store = ctrl_state->ctrl_thread_entity_store;
  OS_MutexScopeW(ctrl_state->ctrl_thread_entity_ctx_rw_mutex)
  {
    for(String8Node *n = msg->entry_points.first; n != 0; n = n->next)
    {
      String8 string = n->string;
      CTRL_Entity *entry = ctrl_entity_alloc(entity_ctx_rw_store, entity_ctx_rw_store->ctx.root, CTRL_EntityKind_EntryPoint, Arch_Null, ctrl_handle_zero(), (U64)id);
      ctrl_entity_equip_string(entity_ctx_rw_store, entry, string);
    }
  }
  
  scratch_end(scratch);
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
  CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
  
  //- rjf: gather all currently existing processes
  CTRL_EntityArray initial_processes = ctrl_entity_array_from_kind(entity_ctx, CTRL_EntityKind_Process);
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    Task *prev;
    CTRL_Entity *process;
  };
  Task *first_task = 0;
  Task *last_task = 0;
  for EachIndex(idx, initial_processes.count)
  {
    CTRL_Entity *entity = initial_processes.v[idx];
    Task *t = push_array(scratch.arena, Task, 1);
    t->process = entity;
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
          CTRL_Entity *new_process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->process));
          Task *t = push_array(scratch.arena, Task, 1);
          t->process = new_process;
          DLLPushBack(first_task, last_task, t);
        }break;
        case DMN_EventKind_Error:{done = 1; cause = CTRL_EventCause_Error;}break;
        case DMN_EventKind_Halt: {done = 1; cause = CTRL_EventCause_InterruptedByHalt;}break;
      }
      
      // rjf: end if all processes are gone
      CTRL_EntityArray processes = ctrl_entity_array_from_kind(entity_ctx, CTRL_EntityKind_Process);
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
  CTRL_EntityCtx *entity_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
  CTRL_Handle target_thread = msg->entity;
  CTRL_Handle target_process = msg->parent;
  CTRL_Entity *target_process_entity = ctrl_entity_from_handle(entity_ctx, target_process);
  U64 spoof_ip_vaddr = 911;
  log_infof("ctrl_thread__run:\n{\n");
  
  //////////////////////////////
  //- rjf: gather all initial breakpoints
  //
  DMN_TrapChunkList user_traps = {0};
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(entity_ctx, target_thread);
    CTRL_EvalScope *eval_scope = ctrl_thread__eval_scope_begin(scratch.arena, thread);
    for(CTRL_Entity *machine = entity_ctx->root->first;
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
          ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, eval_scope, process->handle, module->handle, &msg->user_bps, &user_traps);
        }
        
        // rjf: push process-declared breakpoins
        for(CTRL_Entity *bp = process->first; bp != &ctrl_entity_nil; bp = bp->next)
        {
          if(bp->kind != CTRL_EntityKind_Breakpoint) { continue; }
          ctrl_thread__append_program_defined_bp_traps(scratch.arena, bp, &user_traps);
        }
        
        // rjf: push virtual-address user breakpoints per-process
        ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, eval_scope, process->handle, &msg->user_bps, &user_traps);
      }
    }
    ctrl_thread__eval_scope_end(eval_scope);
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
    for(CTRL_Entity *machine = entity_ctx->root->first;
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
  for(CTRL_Entity *machine = entity_ctx->root->first;
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
          CTRL_EvalScope *eval_scope = ctrl_thread__eval_scope_begin(scratch.arena, &ctrl_entity_nil);
          {
            DMN_TrapChunkList new_traps = {0};
            ctrl_thread__append_resolved_process_user_bp_traps(scratch.arena, eval_scope, ctrl_handle_make(CTRL_MachineID_Local, event->process), &msg->user_bps, &new_traps);
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
          }
          ctrl_thread__eval_scope_end(eval_scope);
        }break;
        case DMN_EventKind_LoadModule:
        {
          CTRL_Entity *thread = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->thread));
          CTRL_EvalScope *eval_scope = ctrl_thread__eval_scope_begin(scratch.arena, thread);
          {
            DMN_TrapChunkList new_traps = {0};
            ctrl_thread__append_resolved_module_user_bp_traps(scratch.arena, eval_scope, ctrl_handle_make(CTRL_MachineID_Local, event->process), ctrl_handle_make(CTRL_MachineID_Local, event->module), &msg->user_bps, &new_traps);
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
          }
          ctrl_thread__eval_scope_end(eval_scope);
        }break;
        case DMN_EventKind_SetBreakpoint:
        {
          CTRL_Entity *bp = &ctrl_entity_nil;
          {
            CTRL_Entity *process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->process));
            for(CTRL_Entity *child = process->first; child != &ctrl_entity_nil; child = child->next)
            {
              if(child->kind == CTRL_EntityKind_Breakpoint &&
                 child->vaddr_range.min == event->address &&
                 child->vaddr_range.max == event->address + event->size &&
                 child->bp_flags == ctrl_user_breakpoint_flags_from_dmn_trap_flags(event->flags))
              {
                bp = child;
                break;
              }
            }
          }
          if(bp != &ctrl_entity_nil)
          {
            DMN_TrapChunkList new_traps = {0};
            ctrl_thread__append_program_defined_bp_traps(scratch.arena, bp, &new_traps);
            dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
            dmn_trap_chunk_list_concat_shallow_copy(scratch.arena, &user_traps, &new_traps);
          }
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
        CTRL_Entity *process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->process));
        CTRL_Entity *module = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Module);
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
        
        //- rjf: add traps for module-baked entry points, if specified
        if(!entries_found)
        {
          String8 raddbg_data = ctrl_raddbg_data_from_module(scratch.arena, module->handle);
          U8 split_char = 0;
          String8List raddbg_data_text_parts = str8_split(scratch.arena, raddbg_data, &split_char, 1, 0);
          for(String8Node *text_n = raddbg_data_text_parts.first; text_n != 0; text_n = text_n->next)
          {
            String8 text = text_n->string;
            MD_Node *root = md_tree_from_string(scratch.arena, text);
            if(str8_match(root->first->string, str8_lit("entry_point"), 0))
            {
              String8 name = root->first->first->string;
              U32 procedure_id = 0;
              {
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
        
        //- rjf: add traps for PID-correllated entry points
        if(!entries_found)
        {
          for(CTRL_Entity *e = entity_ctx->root->first; e != &ctrl_entity_nil; e = e->next)
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
      CTRL_Entity *thread = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->thread));
      CTRL_Entity *process = ctrl_entity_from_handle(entity_ctx, ctrl_handle_make(CTRL_MachineID_Local, event->process));
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
        {
          if(event->user_data != 0)
          {
            hit_user_bp = 1;
          }
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
                if(user_bp != 0 && !(trap->id & bit64) && user_bp->condition.size != 0)
                {
                  str8_list_push(temp.arena, &conditions, user_bp->condition);
                }
              }
            }
          }
        }
        
        // rjf: evaluate hit stop conditions
        if(conditions.node_count != 0) ProfScope("evaluate hit stop conditions")
        {
          CTRL_EvalScope *eval_scope = ctrl_thread__eval_scope_begin(temp.arena, thread);
          for(String8Node *condition_n = conditions.first; condition_n != 0; condition_n = condition_n->next)
          {
            // rjf: evaluate
            E_Eval eval = zero_struct;
            ProfScope("evaluate expression")
            {
              eval = e_eval_from_string(condition_n->string);
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
          ctrl_thread__eval_scope_end(eval_scope);
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
          U64 spoof_sp = dmn_rsp_from_thread(target_thread.dmn_handle);
          spoof_mode = 1;
          spoof.process = target_process.dmn_handle;
          spoof.thread  = target_thread.dmn_handle;
          spoof.vaddr   = spoof_sp;
          spoof.new_ip_value = spoof_ip_vaddr;
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
    event->exception_kind = ctrl_exception_kind_from_dmn(stop_event->exception_kind);
    event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
    event->rip_vaddr = stop_event->instruction_pointer;
    if(stop_cause == CTRL_EventCause_UserBreakpoint && stop_event->user_data != 0)
    {
      if(!(stop_event->user_data & bit64))
      {
        CTRL_UserBreakpoint *user_bp = (CTRL_UserBreakpoint *)stop_event->user_data;
        event->u64_code = user_bp->id;
      }
    }
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
      event->exception_kind = ctrl_exception_kind_from_dmn(stop_event->exception_kind);
      event->vaddr_rng = r1u64(stop_event->address, stop_event->address);
      event->rip_vaddr = stop_event->instruction_pointer;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Asynchronous Memory Streaming Functions

//- rjf: user -> memory stream communication

internal B32
ctrl_u2ms_enqueue_req(HS_Key key, CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    U64 available_size = ctrl_state->u2ms_ring_size-unconsumed_size;
    if(available_size >= sizeof(key)+sizeof(process)+sizeof(vaddr_range)+sizeof(zero_terminated))
    {
      good = 1;
      ctrl_state->u2ms_ring_write_pos += ring_write_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_write_pos, &key);
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
ctrl_u2ms_dequeue_req(HS_Key *out_key, CTRL_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated)
{
  OS_MutexScope(ctrl_state->u2ms_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2ms_ring_write_pos-ctrl_state->u2ms_ring_read_pos;
    if(unconsumed_size >= sizeof(*out_key)+sizeof(*out_process)+sizeof(*out_vaddr_range)+sizeof(*out_zero_terminated))
    {
      ctrl_state->u2ms_ring_read_pos += ring_read_struct(ctrl_state->u2ms_ring_base, ctrl_state->u2ms_ring_size, ctrl_state->u2ms_ring_read_pos, out_key);
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
  HS_Key key = {0};
  CTRL_Handle process = {0};
  Rng1U64 vaddr_range = {0};
  B32 zero_terminated = 0;
  ctrl_u2ms_dequeue_req(&key, &process, &vaddr_range, &zero_terminated);
  ProfBegin("memory stream request");
  
  //- rjf: unpack process key
  U64 process_hash = ctrl_hash_from_handle(process);
  U64 process_slot_idx = process_hash%cache->slots_count;
  U64 process_stripe_idx = process_slot_idx%cache->stripes_count;
  CTRL_ProcessMemoryCacheSlot *process_slot = &cache->slots[process_slot_idx];
  CTRL_ProcessMemoryCacheStripe *process_stripe = &cache->stripes[process_stripe_idx];
  
  //- rjf: unpack address range hash cache key
  U64 range_hash = hs_little_hash_from_data(str8_struct(&key.id));
  
  //- rjf: clamp vaddr range
  Rng1U64 vaddr_range_clamped = vaddr_range;
  {
    vaddr_range_clamped.max = Max(vaddr_range_clamped.max, vaddr_range_clamped.min);
    U64 max_size_cap = Min(max_U64-vaddr_range_clamped.min, GB(1));
    vaddr_range_clamped.max = Min(vaddr_range_clamped.max, vaddr_range_clamped.min+max_size_cap);
  }
  
  //- rjf: task was taken -> read memory
  U64 range_size = 0;
  Arena *range_arena = 0;
  void *range_base = 0;
  U64 zero_terminated_size = 0;
  U64 pre_read_mem_gen = ctrl_mem_gen();
  U64 post_read_mem_gen = 0;
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
  if(range_base != 0 && pre_read_mem_gen == post_read_mem_gen)
  {
    hash = hs_submit_data(key, &range_arena, str8((U8*)range_base, zero_terminated_size));
  }
  else if(range_arena != 0)
  {
    arena_release(range_arena);
  }
  
  //- rjf: commit new info to cache
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
          if(hs_id_match(range_n->id, key.id))
          {
            if(!u128_match(u128_zero(), hash))
            {
              range_n->mem_gen = post_read_mem_gen;
            }
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

////////////////////////////////
//~ rjf: Asynchronous Unwinding Functions

//- rjf: user -> memory stream communication

internal B32
ctrl_u2csb_enqueue_req(CTRL_Handle thread, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(ctrl_state->u2csb_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2csb_ring_write_pos - ctrl_state->u2csb_ring_read_pos;
    U64 available_size = ctrl_state->u2csb_ring_size - unconsumed_size;
    if(available_size >= sizeof(thread))
    {
      good = 1;
      ctrl_state->u2csb_ring_write_pos += ring_write_struct(ctrl_state->u2csb_ring_base, ctrl_state->u2csb_ring_size, ctrl_state->u2csb_ring_write_pos, &thread);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(ctrl_state->u2csb_ring_cv, ctrl_state->u2csb_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(ctrl_state->u2csb_ring_cv);
  }
  return good;
}

internal void
ctrl_u2csb_dequeue_req(CTRL_Handle *out_thread)
{
  OS_MutexScope(ctrl_state->u2csb_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ctrl_state->u2csb_ring_write_pos - ctrl_state->u2csb_ring_read_pos;
    if(unconsumed_size >= sizeof(*out_thread))
    {
      ctrl_state->u2csb_ring_read_pos += ring_read_struct(ctrl_state->u2csb_ring_base, ctrl_state->u2csb_ring_size, ctrl_state->u2csb_ring_read_pos, out_thread);
      break;
    }
    os_condition_variable_wait(ctrl_state->u2csb_ring_cv, ctrl_state->u2csb_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ctrl_state->u2csb_ring_cv);
}

//- rjf: entry point

ASYNC_WORK_DEF(ctrl_call_stack_build_work)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_CallStackCache *cache = &ctrl_state->call_stack_cache;
  
  //- rjf: get next request & unpack
  CTRL_Handle thread_handle = {0};
  ctrl_u2csb_dequeue_req(&thread_handle);
  U64 hash = ctrl_hash_from_handle(thread_handle);
  U64 slot_idx = hash%cache->slots_count;
  U64 stripe_idx = hash%cache->stripes_count;
  CTRL_CallStackCacheSlot *slot = &cache->slots[slot_idx];
  CTRL_CallStackCacheStripe *stripe = &cache->stripes[stripe_idx];
  
  //- rjf: produce mini entity context for just this process
  CTRL_EntityCtx *entity_ctx = push_array(scratch.arena, CTRL_EntityCtx, 1);
  OS_MutexScopeR(ctrl_state->ctrl_thread_entity_ctx_rw_mutex)
  {
    CTRL_EntityCtx *src_ctx = &ctrl_state->ctrl_thread_entity_store->ctx;
    CTRL_EntityCtx *dst_ctx = entity_ctx;
    {
      dst_ctx->root = &ctrl_entity_nil;
      dst_ctx->hash_slots_count = 1024;
      dst_ctx->hash_slots = push_array(scratch.arena, CTRL_EntityHashSlot, dst_ctx->hash_slots_count);
      MemoryCopyArray(dst_ctx->entity_kind_counts, src_ctx->entity_kind_counts);
      MemoryCopyArray(dst_ctx->entity_kind_alloc_gens, src_ctx->entity_kind_alloc_gens);
    }
    CTRL_Entity *src_thread = ctrl_entity_from_handle(src_ctx, thread_handle);
    CTRL_Entity *src_process = ctrl_process_from_entity(src_thread);
    {
      CTRL_EntityRec rec = {0};
      CTRL_Entity *dst_parent = &ctrl_entity_nil;
      for(CTRL_Entity *src_e = src_process; src_e != &ctrl_entity_nil; src_e = rec.next)
      {
        rec = ctrl_entity_rec_depth_first_pre(src_e, src_process);
        
        // rjf: copy this entity
        CTRL_Entity *dst_e = push_array(scratch.arena, CTRL_Entity, 1);
        {
          dst_e->first = dst_e->last = dst_e->next = dst_e->prev = &ctrl_entity_nil;
          dst_e->parent           = dst_parent;
          dst_e->kind             = src_e->kind;
          dst_e->arch             = src_e->arch;
          dst_e->is_frozen        = src_e->is_frozen;
          dst_e->is_soloed        = src_e->is_soloed;
          dst_e->rgba             = src_e->rgba;
          dst_e->handle           = src_e->handle;
          dst_e->id               = src_e->id;
          dst_e->vaddr_range      = src_e->vaddr_range;
          dst_e->stack_base       = src_e->stack_base;
          dst_e->timestamp        = src_e->timestamp;
          dst_e->bp_flags         = src_e->bp_flags;
          dst_e->string           = push_str8_copy(scratch.arena, src_e->string);
        }
        if(dst_parent == &ctrl_entity_nil)
        {
          dst_ctx->root = dst_e;
        }
        else
        {
          DLLPushBack_NPZ(&ctrl_entity_nil, dst_parent->first, dst_parent->last, dst_e, next, prev);
        }
        
        // rjf: insert into hash map
        {
          U64 hash = ctrl_hash_from_handle(dst_e->handle);
          U64 slot_idx = hash%dst_ctx->hash_slots_count;
          CTRL_EntityHashSlot *slot = &dst_ctx->hash_slots[slot_idx];
          CTRL_EntityHashNode *node = 0;
          for(CTRL_EntityHashNode *n = slot->first; n != 0; n = n->next)
          {
            if(ctrl_handle_match(n->entity->handle, dst_e->handle))
            {
              node = n;
              break;
            }
          }
          if(node == 0)
          {
            node = push_array(scratch.arena, CTRL_EntityHashNode, 1);
            MemoryZeroStruct(node);
            DLLPushBack(slot->first, slot->last, node);
            node->entity = dst_e;
          }
        }
        
        // rjf: push/pop
        if(rec.push_count)
        {
          dst_parent = dst_e;
        }
        else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
        {
          dst_parent = dst_parent->parent;
        }
      }
    }
  }
  
  //- rjf: do task
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(entity_ctx, thread_handle);
    CTRL_Entity *process = ctrl_process_from_entity(thread);
    
    //- rjf: compute unwind to find list of all concrete frames, then
    // call stack, to determine list of all concrete & inline frames
    Arena *arena = arena_alloc();
    U64 pre_reg_gen = 0;
    U64 post_reg_gen = 0;
    U64 pre_mem_gen = 0;
    U64 post_mem_gen = 0;
    CTRL_Unwind unwind = {0};
    CTRL_CallStack call_stack = {0};
    {
      pre_reg_gen = ctrl_reg_gen();
      pre_mem_gen = ctrl_mem_gen();
      unwind = ctrl_unwind_from_thread(arena, entity_ctx, thread_handle, os_now_microseconds()+5000);
      call_stack = ctrl_call_stack_from_unwind(arena, process, &unwind);
      post_reg_gen = ctrl_reg_gen();
      post_mem_gen = ctrl_mem_gen();
    }
    
    //- rjf: store new results in cache
    Arena *last_arena = arena;
    if(pre_reg_gen == post_reg_gen &&
       pre_mem_gen == post_mem_gen)
    {
      B32 found = 0;
      B32 committed = 0;
      OS_MutexScopeW(stripe->rw_mutex) for(;;)
      {
        // rjf: try to find node & commit
        for(CTRL_CallStackCacheNode *n = slot->first; n != 0; n = n->next)
        {
          if(ctrl_handle_match(n->thread, thread_handle))
          {
            found = 1;
            if(n->scope_touch_count == 0)
            {
              committed = 1;
              if(unwind.flags == 0 || call_stack.frames_count >= n->call_stack.frames_count)
              {
                last_arena = n->arena;
                n->arena = arena;
                n->call_stack = call_stack;
              }
              if(unwind.flags == 0)
              {
                n->reg_gen = pre_reg_gen;
                n->mem_gen = pre_mem_gen;
              }
            }
            break;
          }
        }
        
        // rjf: not found, or committed? -> abort
        if(!found || committed)
        {
          break;
        }
        
        // rjf: found, not committed? -> wait & retry
        if(found && !committed)
        {
          os_condition_variable_wait_rw_w(stripe->cv, stripe->rw_mutex, os_now_microseconds()+10);
        }
      }
    }
    
    //- rjf: release last results
    if(last_arena != 0)
    {
      arena_release(last_arena);
    }
    
    //- rjf: mark work as done
    OS_MutexScopeW(stripe->rw_mutex) for(CTRL_CallStackCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->thread, thread_handle))
      {
        ins_atomic_u64_dec_eval(&n->working_count);
        break;
      }
    }
    
    //- rjf: broadcast update
    os_condition_variable_broadcast(stripe->cv);
  }
  
  scratch_end(scratch);
  return 0;
}
