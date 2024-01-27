// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
ctrl_init(CTRL_WakeupFunctionType *wakeup_hook)
{
  Arena *arena = arena_alloc();
  ctrl_state = push_array(arena, CTRL_State, 1);
  ctrl_state->arena = arena;
  ctrl_state->wakeup_hook = wakeup_hook;
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
  ctrl_state->process_memory_cache.stripes_count = 8;
  ctrl_state->process_memory_cache.stripes = push_array(arena, CTRL_ProcessMemoryCacheStripe, ctrl_state->process_memory_cache.stripes_count);
  for(U64 idx = 0; idx < ctrl_state->process_memory_cache.stripes_count; idx += 1)
  {
    ctrl_state->process_memory_cache.stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  ctrl_state->u2c_ring_size = KB(64);
  ctrl_state->u2c_ring_base = push_array_no_zero(arena, U8, ctrl_state->u2c_ring_size);
  ctrl_state->u2c_ring_mutex = os_mutex_alloc();
  ctrl_state->u2c_ring_cv = os_condition_variable_alloc();
  ctrl_state->c2u_ring_size = KB(64);
  ctrl_state->c2u_ring_base = push_array_no_zero(arena, U8, ctrl_state->c2u_ring_size);
  ctrl_state->c2u_ring_mutex = os_mutex_alloc();
  ctrl_state->c2u_ring_cv = os_condition_variable_alloc();
  ctrl_state->demon_event_arena = arena_alloc();
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
  ctrl_state->ms_thread_count = Min(4, os_logical_core_count()-1);
  ctrl_state->ms_threads = push_array(arena, OS_Handle, ctrl_state->ms_thread_count);
  for(U64 idx = 0; idx < ctrl_state->ms_thread_count; idx += 1)
  {
    ctrl_state->ms_threads[idx] = os_launch_thread(ctrl_mem_stream_thread__entry_point, (void *)idx, 0);
  }
}

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

internal CTRL_EventCause
ctrl_event_cause_from_demon_event_kind(DEMON_EventKind event_kind)
{
  CTRL_EventCause cause = CTRL_EventCause_Null;
  switch(event_kind)
  {
    default:{}break;
    case DEMON_EventKind_Error:    {cause = CTRL_EventCause_Error;}break;
    case DEMON_EventKind_Exception:{cause = CTRL_EventCause_InterruptedByException;}break;
    case DEMON_EventKind_Trap:     {cause = CTRL_EventCause_InterruptedByTrap;}break;
    case DEMON_EventKind_Halt:     {cause = CTRL_EventCause_InterruptedByHalt;}break;
  }
  return cause;
}

internal B32
ctrl_handle_match(CTRL_Handle a, CTRL_Handle b)
{
  return MemoryMatchStruct(&a, &b);
}

////////////////////////////////
//~ rjf: Ctrl <-> Demon Handle Translation Functions

internal DEMON_Handle
ctrl_demon_handle_from_ctrl(CTRL_Handle h)
{
  DEMON_Handle result = h.u64[0];
  return result;
}

internal CTRL_Handle
ctrl_handle_from_demon(DEMON_Handle h)
{
  CTRL_Handle result = {h};
  return result;
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

internal void
ctrl_append_resolved_module_user_bp_traps(Arena *arena, DEMON_Handle process, DEMON_Handle module, CTRL_UserBreakpointList *user_bps, DEMON_TrapChunkList *traps_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  DBGI_Scope *scope = dbgi_scope_open();
  String8 exe_path = demon_full_path_from_module(scratch.arena, module);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
  RADDBG_Parsed *rdbg = &dbgi->rdbg;
  U64 base_vaddr = demon_base_vaddr_from_module(module);
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
          RADDBG_NameMap *mapptr = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_NormalSourcePaths);
          if(mapptr != 0)
          {
            RADDBG_ParsedNameMap map = {0};
            raddbg_name_map_parse(rdbg, mapptr, &map);
            RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, filename_normalized.str, filename_normalized.size);
            if(node != 0)
            {
              U32 id_count = 0;
              U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
              if(id_count > 0)
              {
                src_id = ids[0];
              }
            }
          }
        }
        
        // rjf: src_id * pt -> push
        if(0 < src_id && src_id < rdbg->source_file_count)
        {
          RADDBG_SourceFile *src = rdbg->source_files + src_id;
          RADDBG_ParsedLineMap line_map = {0};
          raddbg_line_map_from_source_file(rdbg, src, &line_map);
          U32 voff_count = 0;
          U64 *voffs = raddbg_line_voffs_from_num(&line_map, pt.line, &voff_count);
          for(U32 i = 0; i < voff_count; i += 1)
          {
            U64 vaddr = voffs[i] + base_vaddr;
            DEMON_Trap trap = {process, vaddr, (U64)bp};
            demon_trap_chunk_list_push(arena, traps_out, 256, &trap);
          }
        }
      }break;
      
      //- rjf: symbol:voff-based breakpoints
      case CTRL_UserBreakpointKind_SymbolNameAndOffset:
      {
        String8 symbol_name = bp->string;
        U64 voff = bp->u64;
        if(rdbg != 0 && rdbg->procedures != 0)
        {
          RADDBG_NameMap *mapptr = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_Procedures);
          if(mapptr != 0)
          {
            RADDBG_ParsedNameMap map = {0};
            raddbg_name_map_parse(rdbg, mapptr, &map);
            RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, symbol_name.str, symbol_name.size);
            if(node != 0)
            {
              U32 id_count = 0;
              U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
              for(U32 match_i = 0; match_i < id_count; match_i += 1)
              {
                U64 proc_voff = raddbg_first_voff_from_proc(rdbg, ids[match_i]);
                U64 proc_vaddr = proc_voff + base_vaddr;
                DEMON_Trap trap = {process, proc_vaddr + voff, (U64)bp};
                demon_trap_chunk_list_push(arena, traps_out, 256, &trap);
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
ctrl_append_resolved_process_user_bp_traps(Arena *arena, DEMON_Handle process, CTRL_UserBreakpointList *user_bps, DEMON_TrapChunkList *traps_out)
{
  for(CTRL_UserBreakpointNode *n = user_bps->first; n != 0; n = n->next)
  {
    CTRL_UserBreakpoint *bp = &n->v;
    if(bp->kind == CTRL_UserBreakpointKind_VirtualAddress)
    {
      DEMON_Trap trap = {process, bp->u64, (U64)bp};
      demon_trap_chunk_list_push(arena, traps_out, 256, &trap);
    }
  }
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
//~ rjf: Shared Functions

//- rjf: run index

internal U64
ctrl_run_idx(void)
{
  U64 result = ins_atomic_u64_eval(&ctrl_state->run_idx);
  return result;
}

internal U64
ctrl_memgen_idx(void)
{
  U64 result = ins_atomic_u64_eval(&ctrl_state->memgen_idx);
  return result;
}

internal U64
ctrl_reggen_idx(void)
{
  U64 result = ins_atomic_u64_eval(&ctrl_state->reggen_idx);
  return result;
}

//- rjf: halt everything

internal void
ctrl_halt(void)
{
  demon_halt(0, 0);
}

//- rjf: entity introspection

internal U32
ctrl_id_from_machine_entity(CTRL_MachineID id, DEMON_Handle handle)
{
  U32 result = 0;
  return result;
}

//- rjf: exe -> dbg path mapping

internal String8
ctrl_inferred_og_dbg_path_from_exe_path(Arena *arena, String8 exe_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 result = {0};
  {
    // TODO(rjf): do preliminary header parse of EXE if possible, look for connected debug info path
  }
  scratch_end(scratch);
  return result;
}

internal String8
ctrl_forced_og_dbg_path_from_exe_path(Arena *arena, String8 exe_path)
{
  String8 result = {0};
  // TODO(rjf)
  return result;
}

internal String8
ctrl_natural_og_dbg_path_from_exe_path(Arena *arena, String8 exe_path)
{
  String8 exe_extension = str8_skip_last_dot(exe_path);
  String8 dbg_extension = {0};
  if(str8_match(exe_extension, str8_lit("exe"), 0)) {dbg_extension = str8_lit("pdb");}
  if(str8_match(exe_extension, str8_lit("elf"), 0)) {dbg_extension = str8_lit("debug");}
  String8 result = {0};
  if(dbg_extension.size != 0)
  {
    result = push_str8f(arena, "%S.%S", str8_chop_last_dot(exe_path), dbg_extension);
  }
  return result;
}

internal String8
ctrl_og_dbg_path_from_exe_path(Arena *arena, String8 exe_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 forced_og_dbg_path = ctrl_forced_og_dbg_path_from_exe_path(scratch.arena, exe_path);
  String8 inferred_og_dbg_path = ctrl_inferred_og_dbg_path_from_exe_path(scratch.arena, exe_path);
  String8 natural_og_dbg_path = ctrl_natural_og_dbg_path_from_exe_path(scratch.arena, exe_path);
  String8 og_dbg_path = forced_og_dbg_path.size?forced_og_dbg_path : inferred_og_dbg_path.size?inferred_og_dbg_path : natural_og_dbg_path;
  og_dbg_path = push_str8_copy(arena, og_dbg_path);
  scratch_end(scratch);
  return og_dbg_path;
}

//- rjf: handle -> arch

internal Architecture
ctrl_arch_from_handle(CTRL_MachineID machine, CTRL_Handle handle)
{
  return demon_arch_from_object(ctrl_demon_handle_from_ctrl(handle));
}

//- rjf: process memory reading/writing

internal U64
ctrl_process_read(CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 range, void *dst)
{
  U64 actual_bytes_read = demon_read_memory(ctrl_demon_handle_from_ctrl(process), dst, range.min, dim_1u64(range));
  return actual_bytes_read;
}

internal B32
ctrl_process_write(CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 range, void *src)
{
  ProfBeginFunction();
  U64 size = dim_1u64(range);
  B32 result = demon_write_memory(ctrl_demon_handle_from_ctrl(process), range.min, src, size);
  if(result)
  {
    ins_atomic_u64_inc_eval(&ctrl_state->memgen_idx);
  }
  ProfEnd();
  return result;
}

//- rjf: process memory cache interaction

internal U128
ctrl_hash_store_key_from_process_vaddr_range(CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 range, B32 zero_terminated)
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
ctrl_stored_hash_from_process_vaddr_range(CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 range, B32 zero_terminated)
{
  U128 result = {0};
  U64 size = dim_1u64(range);
  if(size != 0)
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
        if(n->machine_id == machine_id && ctrl_handle_match(n->process, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
            {
              result = range_n->hash;
              is_good = 1;
              is_stale = range_n->memgen_idx < ctrl_memgen_idx();
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
          if(n->machine_id == machine_id && ctrl_handle_match(n->process, process))
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
    if(!is_good)
    {
      OS_MutexScopeW(process_stripe->rw_mutex)
      {
        for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
        {
          if(n->machine_id == machine_id && ctrl_handle_match(n->process, process))
          {
            U64 range_slot_idx = range_hash%n->range_hash_slots_count;
            CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
            B32 range_node_exists = 0;
            for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
            {
              if(MemoryMatchStruct(&range_n->vaddr_range, &range) && range_n->zero_terminated == zero_terminated)
              {
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
              break;
            }
          }
        }
      }
    }
    
    //- rjf: not good, or is stale -> submit hash request
    if(!is_good || is_stale)
    {
      ctrl_u2ms_enqueue_req(machine_id, process, range, zero_terminated, 0);
    }
  }
  return result;
}

//- rjf: process memory cache reading helpers

internal CTRL_ProcessMemorySlice
ctrl_query_cached_data_from_process_vaddr_range(Arena *arena, CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 range)
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
      U128 page_hash = ctrl_stored_hash_from_process_vaddr_range(machine_id, process, r1u64(page_base_vaddr, page_base_vaddr+page_size), 0);
      U128 page_last_hash = hs_hash_from_key(page_key, 1);
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
    
    hs_scope_close(scope);
    scratch_end(scratch);
  }
  return result;
}

internal CTRL_ProcessMemorySlice
ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(Arena *arena, CTRL_MachineID machine_id, CTRL_Handle process, U64 vaddr, U64 limit, U64 endt_us)
{
  CTRL_ProcessMemorySlice result = ctrl_query_cached_data_from_process_vaddr_range(arena, machine_id, process, r1u64(vaddr, vaddr+limit));
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

//- rjf: register reading/writing

internal void *
ctrl_reg_block_from_thread(CTRL_MachineID machine_id, CTRL_Handle thread)
{
  void *regs = demon_read_regs(ctrl_demon_handle_from_ctrl(thread));
  return regs;
}

internal B32
ctrl_thread_write_reg_block(CTRL_MachineID machine_id, CTRL_Handle thread, void *block)
{
  B32 good = demon_write_regs(ctrl_demon_handle_from_ctrl(thread), block);
  ins_atomic_u64_inc_eval(&ctrl_state->reggen_idx);
  return good;
}

internal U64
ctrl_rip_from_thread(CTRL_MachineID machine_id, CTRL_Handle thread)
{
  U64 result = 0;
  void *regs = ctrl_reg_block_from_thread(machine_id, thread);
  if(regs != 0)
  {
    Architecture arch = demon_arch_from_object(ctrl_demon_handle_from_ctrl(thread));
    result = regs_rip_from_arch_block(arch, regs);
  }
  return result;
}

internal B32
ctrl_thread_write_rip(CTRL_MachineID machine_id, CTRL_Handle thread, U64 rip)
{
  B32 result = 0;
  void *regs = ctrl_reg_block_from_thread(machine_id, thread);
  if(regs != 0)
  {
    Architecture arch = demon_arch_from_object(ctrl_demon_handle_from_ctrl(thread));
    regs_arch_block_write_rip(arch, regs, rip);
    ctrl_thread_write_reg_block(machine_id, thread, regs);
    result = 1;
  }
  return result;
}

internal U64
ctrl_tls_root_vaddr_from_thread(CTRL_MachineID machine_id, CTRL_Handle thread)
{
  U64 result = 0;
  DEMON_Handle demon_handle = ctrl_demon_handle_from_ctrl(thread);
  result = demon_tls_root_vaddr_from_thread(demon_handle);
  return result;
}

//- rjf: process * vaddr -> module

internal CTRL_Handle
ctrl_module_from_process_vaddr(CTRL_MachineID machine_id, CTRL_Handle process, U64 vaddr)
{
  CTRL_Handle handle = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    DEMON_HandleArray modules = demon_modules_from_process(scratch.arena, ctrl_demon_handle_from_ctrl(process));
    for(U64 idx = 0; idx < modules.count; idx += 1)
    {
      DEMON_Handle m = modules.handles[idx];
      Rng1U64 m_vaddr_rng = demon_vaddr_range_from_module(m);
      if(contains_1u64(m_vaddr_rng, vaddr))
      {
        handle = ctrl_handle_from_demon(m);
        break;
      }
    }
    scratch_end(scratch);
  }
  return handle;
}

//- rjf: unwinding

internal CTRL_Unwind
ctrl_unwind_from_process_thread(Arena *arena, CTRL_MachineID machine_id, CTRL_Handle process, CTRL_Handle thread)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  DBGI_Scope *scope = dbgi_scope_open();
  Architecture arch = demon_arch_from_object(ctrl_demon_handle_from_ctrl(thread));
  U64 arch_reg_block_size = regs_block_size_from_architecture(arch);
  CTRL_Unwind unwind = {0};
  unwind.error = 1;
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:
    {
      // rjf: grab initial register block
      void *regs_block = push_array(scratch.arena, U8, arch_reg_block_size);
      B32 regs_block_good = 0;
      {
        void *regs_raw = ctrl_reg_block_from_thread(machine_id, thread);
        if(regs_raw != 0)
        {
          MemoryCopy(regs_block, regs_raw, arch_reg_block_size);
          regs_block_good = 1;
        }
      }
      
      // rjf: grab initial memory view
      B32 stack_memview_good = 0;
      UNW_MemView stack_memview = {0};
      if(regs_block_good)
      {
        U64 stack_base_unrounded = demon_stack_base_vaddr_from_thread(ctrl_demon_handle_from_ctrl(thread));
        U64 stack_top_unrounded = regs_rsp_from_arch_block(arch, regs_block);
        U64 stack_base = AlignPow2(stack_base_unrounded, KB(4));
        U64 stack_top = AlignDownPow2(stack_top_unrounded, KB(4));
        U64 stack_size = stack_base - stack_top;
        if(stack_base >= stack_top)
        {
          String8 stack_memory = {0};
          stack_memory.str = push_array_no_zero(scratch.arena, U8, stack_size);
          stack_memory.size = ctrl_process_read(machine_id, process, r1u64(stack_top, stack_top+stack_size), stack_memory.str);
          if(stack_memory.size != 0)
          {
            stack_memview_good = 1;
            stack_memview.data = stack_memory.str;
            stack_memview.addr_first = stack_top;
            stack_memview.addr_opl = stack_base;
          }
        }
      }
      
      // rjf: loop & unwind
      UNW_MemView memview = stack_memview;
      if(stack_memview_good) for(;;)
      {
        unwind.error = 0;
        
        // rjf: regs -> rip*module*binary
        U64 rip = regs_rip_from_arch_block(arch, regs_block);
        CTRL_Handle module = ctrl_module_from_process_vaddr(machine_id, process, rip);
        
        // rjf: cancel on 0 rip
        if(rip == 0)
        {
          break;
        }
        
        // rjf: binary -> all the binary info
        String8 binary_full_path = demon_full_path_from_module(scratch.arena, ctrl_demon_handle_from_ctrl(module));
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
        SLLQueuePush(unwind.first, unwind.last, frame);
        unwind.count += 1;
        
        // rjf: unwind one step
        UNW_Result unwind_step = unw_pe_x64(binary_data, &dbgi->pe, demon_vaddr_range_from_module(ctrl_demon_handle_from_ctrl(module)).min, &memview, (UNW_X64_Regs *)regs_block);
        
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
    }break;
  }
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return unwind;
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
//~ rjf: User -> Ctrl Communication

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

////////////////////////////////
//~ rjf: Ctrl -> User Communication

internal void
ctrl_c2u_push_events(CTRL_EventList *events)
{
  if(events->count != 0) ProfScope("ctrl_c2u_push_events")
  {
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

////////////////////////////////
//~ rjf: User -> Memory Stream Communication

internal B32
ctrl_u2ms_enqueue_req(CTRL_MachineID machine_id, CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us)
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
ctrl_u2ms_dequeue_req(CTRL_MachineID *out_machine_id, CTRL_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated)
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

////////////////////////////////
//~ rjf: Control-Thread-Only Functions

//- rjf: entry point

internal void
ctrl_thread__entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  ThreadName("[ctrl] thread");
  ProfBeginFunction();
  demon_primary_thread_begin();
  Temp scratch = scratch_begin(0, 0);
  for(;;)
  {
    temp_end(scratch);
    
    //- rjf: get next messages
    CTRL_MsgList msgs = ctrl_u2c_pop_msgs(scratch.arena);
    
    //- rjf: process messages
    {
      demon_exclusive_mode_begin();
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
          case CTRL_MsgKind_LaunchAndHandshake:{ctrl_thread__launch_and_handshake(msg);}break;
          case CTRL_MsgKind_LaunchAndInit:     {ctrl_thread__launch_and_init     (msg);}break;
          case CTRL_MsgKind_Attach:            {ctrl_thread__attach              (msg);}break;
          case CTRL_MsgKind_Kill:              {ctrl_thread__kill                (msg);}break;
          case CTRL_MsgKind_Detach:            {ctrl_thread__detach              (msg);}break;
          case CTRL_MsgKind_Run:               {ctrl_thread__run                 (msg); done = 1;}break;
          case CTRL_MsgKind_SingleStep:        {ctrl_thread__single_step         (msg); done = 1;}break;
          
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
      demon_exclusive_mode_end();
    }
  }
  scratch_end(scratch);
  ProfEnd();
}

//- rjf: attached process running/event gathering

internal DEMON_Event *
ctrl_thread__next_demon_event(Arena *arena, CTRL_Msg *msg, DEMON_RunCtrls *run_ctrls, CTRL_Spoof *spoof)
{
  ProfBeginFunction();
  DEMON_Event *event = push_array(arena, DEMON_Event, 1);
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: loop -> try to get event, run, repeat
  U64 spoof_old_ip_value = 0;
  ProfScope("loop -> try to get event, run, repeat") for(B32 got_event = 0; got_event == 0;)
  {
    //- rjf: get next event
    ProfScope("get next event")
    {
      // rjf: grab first event
      DEMON_EventNode *next_event_node = ctrl_state->first_demon_event_node;
      
      // rjf: determine if we should filter
      B32 should_filter_event = 0;
      if(next_event_node != 0)
      {
        DEMON_Event *ev = &next_event_node->v;
        switch(ev->kind)
        {
          default:{}break;
          case DEMON_EventKind_Exception:
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
              DEMON_HandleArray modules = demon_modules_from_process(scratch.arena, ev->process);
              if(modules.count != 0)
              {
                // rjf: determine base address of asan shadow space
                U64 asan_shadow_base_vaddr = 0;
                B32 asan_shadow_variable_exists_but_is_zero = 0;
                CTRL_Handle module = ctrl_handle_from_demon(modules.handles[0]);
                String8 module_path = demon_full_path_from_module(scratch.arena, ctrl_demon_handle_from_ctrl(module));
                DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, module_path, max_U64);
                RADDBG_Parsed *rdbg = &dbgi->rdbg;
                RADDBG_NameMap *unparsed_map = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_GlobalVariables);
                if(rdbg->global_variables != 0 && unparsed_map != 0)
                {
                  RADDBG_ParsedNameMap map = {0};
                  raddbg_name_map_parse(rdbg, unparsed_map, &map);
                  String8 name = str8_lit("__asan_shadow_memory_dynamic_address");
                  RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, name.str, name.size);
                  if(node != 0)
                  {
                    U32 id_count = 0;
                    U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
                    if(id_count > 0 && 0 < ids[0] && ids[0] < rdbg->global_variable_count)
                    {
                      RADDBG_GlobalVariable *global_var = &rdbg->global_variables[ids[0]];
                      U64 global_var_voff = global_var->voff;
                      U64 global_var_vaddr = global_var->voff + demon_base_vaddr_from_module(ctrl_demon_handle_from_ctrl(module));
                      Architecture arch = demon_arch_from_object(ev->thread);
                      U64 addr_size = bit_size_from_arch(arch)/8;
                      ctrl_process_read(CTRL_MachineID_Client, ctrl_handle_from_demon(ev->process), r1u64(global_var_vaddr, global_var_vaddr+addr_size), &asan_shadow_base_vaddr);
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
        SLLQueuePop(ctrl_state->first_demon_event_node, ctrl_state->last_demon_event_node);
        MemoryCopyStruct(event, &next_event_node->v);
        event->string = push_str8_copy(arena, event->string);
        run_ctrls->ignore_previous_exception = 1;
      }
      
      // rjf: good event but filtered? pop from queue
      if(next_event_node != 0 && should_filter_event)
      {
        SLLQueuePop(ctrl_state->first_demon_event_node, ctrl_state->last_demon_event_node);
        run_ctrls->ignore_previous_exception = 0;
      }
    }
    
    //- rjf: no event -> demon_run for a new one
    if(got_event == 0) ProfScope("no event -> demon_run for a new one")
    {
      // rjf: prep spoof
      B32 do_spoof = (spoof != 0 && run_ctrls->single_step_thread == 0);
      U64 size_of_spoof = 0;
      if(do_spoof) ProfScope("prep spoof")
      {
        Architecture arch = demon_arch_from_object(ctrl_demon_handle_from_ctrl(spoof->process));
        demon_read_memory(ctrl_demon_handle_from_ctrl(spoof->process), &spoof_old_ip_value, spoof->vaddr, sizeof(spoof_old_ip_value));
        size_of_spoof = bit_size_from_arch(arch)/8;
      }
      
      // rjf: set spoof
      if(do_spoof) ProfScope("set spoof")
      {
        demon_write_memory(ctrl_demon_handle_from_ctrl(spoof->process), spoof->vaddr, &spoof->new_ip_value, size_of_spoof);
      }
      
      // rjf: run for new events
      ProfScope("run for new events")
      {
        DEMON_EventList events = demon_run(scratch.arena, run_ctrls);
        for(DEMON_EventNode *src_n = events.first; src_n != 0; src_n = src_n->next)
        {
          DEMON_EventNode *dst_n = ctrl_state->free_demon_event_node;
          if(dst_n != 0)
          {
            SLLStackPop(ctrl_state->free_demon_event_node);
          }
          else
          {
            dst_n = push_array(ctrl_state->demon_event_arena, DEMON_EventNode, 1);
          }
          MemoryCopyStruct(&dst_n->v, &src_n->v);
          dst_n->v.string = push_str8_copy(ctrl_state->demon_event_arena, dst_n->v.string);
          SLLQueuePush(ctrl_state->first_demon_event_node, ctrl_state->last_demon_event_node, dst_n);
        }
      }
      
      // rjf: unset spoof
      if(do_spoof) ProfScope("unset spoof")
      {
        demon_write_memory(ctrl_demon_handle_from_ctrl(spoof->process), spoof->vaddr, &spoof_old_ip_value, size_of_spoof);
      }
      
      // rjf: inc generation counters
      {
        ins_atomic_u64_inc_eval(&ctrl_state->run_idx);
        ins_atomic_u64_inc_eval(&ctrl_state->memgen_idx);
        ins_atomic_u64_inc_eval(&ctrl_state->reggen_idx);
      }
    }
  }
  
  //- rjf: irrespective of what event came back, we should ALWAYS check the
  // spoof's thread and see if it hit the spoof address, because we may have
  // simply been sent other debug events first
  if(spoof != 0)
  {
    U64 spoof_thread_rip = demon_read_ip(ctrl_demon_handle_from_ctrl(spoof->thread));
    if(spoof_thread_rip == spoof->new_ip_value)
    {
      demon_write_ip(ctrl_demon_handle_from_ctrl(spoof->thread), spoof_old_ip_value);
    }
  }
  
  //- rjf: push ctrl events associated with this demon event
  CTRL_EventList evts = {0};
  ProfScope("push ctrl events associated with this demon event") switch(event->kind)
  {
    default:{}break;
    case DEMON_EventKind_CreateProcess:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_NewProc;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->process);
      out_evt->arch       = demon_arch_from_object(event->process);
      out_evt->entity_id  = event->code;
      ctrl_state->process_counter += 1;
    }break;
    case DEMON_EventKind_CreateThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_NewThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->thread);
      out_evt->parent     = ctrl_handle_from_demon(event->process);
      out_evt->arch       = demon_arch_from_object(event->process);
      out_evt->entity_id  = event->code;
      out_evt->stack_base = demon_stack_base_vaddr_from_thread(event->thread);
      out_evt->tls_root   = demon_tls_root_vaddr_from_thread(event->thread);
      out_evt->rip_vaddr  = event->instruction_pointer;
      out_evt->string     = event->string;
    }break;
    case DEMON_EventKind_LoadModule:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      String8 module_path = event->string;
      dbgi_binary_open(module_path);
      out_evt->kind       = CTRL_EventKind_NewModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->module);
      out_evt->parent     = ctrl_handle_from_demon(event->process);
      out_evt->arch       = demon_arch_from_object(event->module);
      out_evt->entity_id  = event->code;
      out_evt->vaddr_rng  = r1u64(event->address, event->address+event->size);
      out_evt->rip_vaddr  = demon_base_vaddr_from_module(event->module);
      out_evt->string     = module_path;
    }break;
    case DEMON_EventKind_ExitProcess:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndProc;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->process);
      out_evt->u64_code   = event->code;
      ctrl_state->process_counter -= 1;
    }break;
    case DEMON_EventKind_ExitThread:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_EndThread;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->thread);
      out_evt->entity_id  = event->code;
    }break;
    case DEMON_EventKind_UnloadModule:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      String8 module_path = event->string;
      dbgi_binary_close(module_path);
      out_evt->kind       = CTRL_EventKind_EndModule;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->module);
    }break;
    case DEMON_EventKind_DebugString:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_DebugString;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->thread);
      out_evt->parent     = ctrl_handle_from_demon(event->process);
      out_evt->string     = event->string;
    }break;
    case DEMON_EventKind_SetThreadName:
    {
      CTRL_Event *out_evt = ctrl_event_list_push(scratch.arena, &evts);
      out_evt->kind       = CTRL_EventKind_ThreadName;
      out_evt->msg_id     = msg->msg_id;
      out_evt->machine_id = CTRL_MachineID_Client;
      out_evt->entity     = ctrl_handle_from_demon(event->thread);
      out_evt->parent     = ctrl_handle_from_demon(event->process);
      out_evt->string     = event->string;
    }break;
  }
  ctrl_c2u_push_events(&evts);
  
  //- rjf: clear process memory cache, if we've just started a lone process
  if(event->kind == DEMON_EventKind_CreateProcess && ctrl_state->process_counter == 1)
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
  if(ctrl_state->first_demon_event_node == 0)
  {
    ctrl_state->free_demon_event_node = 0;
    arena_clear(ctrl_state->demon_event_arena);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return(event);
}

//- rjf: eval helpers

internal B32
ctrl_eval_memory_read(void *u, void *out, U64 addr, U64 size)
{
  DEMON_Handle process = (DEMON_Handle)u;
  U64 read_size = demon_read_memory(process, out, addr, size);
  B32 result = (read_size == size);
  return result;
}

//- rjf: msg kind implementations

internal void
ctrl_thread__launch_and_handshake(CTRL_Msg *msg)
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
  U32 id = demon_launch_process(&opts);
  
  //- rjf: record start
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: run to handshake
  DEMON_Event *stop_event = 0;
  if(id != 0)
  {
    // rjf: prep run controls
    DEMON_Handle unfrozen_process = 0;
    DEMON_RunCtrls run_ctrls = {0};
    {
      run_ctrls.run_entities_are_unfrozen = 1;
      run_ctrls.run_entities_are_processes = 1;
      run_ctrls.run_entities = &unfrozen_process;
      run_ctrls.run_entity_count = 0;
    }
    
    // rjf: run until handshake-signifying events
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DEMON_EventKind_CreateProcess:
        {
          unfrozen_process = event->process;
          run_ctrls.run_entity_count = 1;
        }break;
        case DEMON_EventKind_Error:
        case DEMON_EventKind_Breakpoint:
        case DEMON_EventKind_Exception:
        case DEMON_EventKind_ExitProcess:
        case DEMON_EventKind_HandshakeComplete:
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
      event->cause = ctrl_event_cause_from_demon_event_kind(stop_event->kind);
      event->machine_id = CTRL_MachineID_Client;
      event->entity = ctrl_handle_from_demon(stop_event->thread);
      event->parent = ctrl_handle_from_demon(stop_event->process);
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
    evt->machine_id= CTRL_MachineID_Client;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = id;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__launch_and_init(CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: launch
  OS_LaunchOptions opts = {0};
  {
    opts.cmd_line    = msg->cmd_line_string_list;
    opts.path        = msg->path;
    opts.env         = msg->env_string_list;
    opts.inherit_env = msg->env_inherit;
  }
  U32 id = demon_launch_process(&opts);
  
  //- rjf: record start
  {
    CTRL_EventList evts = {0};
    CTRL_Event *event = ctrl_event_list_push(scratch.arena, &evts);
    event->kind = CTRL_EventKind_Started;
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: run to initialization (entry point)
  DEMON_Event *stop_event = 0;
  if(id != 0)
  {
    DEMON_Handle unfrozen_process[8] = {0};
    DEMON_TrapChunkList demon_traps = {0};
    U64 entry_vaddr = 0;
    DEMON_Handle entry_vaddr_proc = 0;
    DEMON_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        
        // rjf: new process -> freeze process
        case DEMON_EventKind_CreateProcess:
        if(run_ctrls.run_entity_count < ArrayCount(unfrozen_process))
        {
          unfrozen_process[run_ctrls.run_entity_count] = event->process;
          run_ctrls.run_entities = &unfrozen_process[0];
          run_ctrls.run_entity_count += 1;
        }break;
        
        // rjf: breakpoint -> if it's the entry point, we're done. otherwise, keep going
        case DEMON_EventKind_Breakpoint:
        if(event->instruction_pointer == entry_vaddr)
        {
          done = 1;
          stop_event = event;
        }break;
        
        // rjf: exception -> done
        case DEMON_EventKind_Exception:
        {
          done = 1;
          stop_event = event;
        }break;
        
        // rjf: process ended? -> remove from unfrozen processes. zero processes -> done.
        case DEMON_EventKind_ExitProcess:
        {
          for(U64 idx = 0; idx < run_ctrls.run_entity_count; idx += 1)
          {
            if(run_ctrls.run_entities[idx] == event->process &&
               idx+1 < run_ctrls.run_entity_count)
            {
              MemoryCopy(run_ctrls.run_entities+idx, run_ctrls.run_entities+idx+1, sizeof(DEMON_Handle)*(run_ctrls.run_entity_count-(idx+1)));
              break;
            }
          }
          run_ctrls.run_entity_count -= 1;
          if(run_ctrls.run_entity_count == 0)
          {
            done = 1;
            stop_event = event;
          }
        }break;
        
        // rjf: done with handshake -> ready to find entry point. search launched processes
        case DEMON_EventKind_HandshakeComplete:
        {
          DBGI_Scope *scope = dbgi_scope_open();
          
          // rjf: find entry point vaddr
          for(U64 process_idx = 0; process_idx < run_ctrls.run_entity_count; process_idx += 1)
          {
            DEMON_HandleArray modules = demon_modules_from_process(scratch.arena, run_ctrls.run_entities[process_idx]);
            if(modules.count == 0) { continue; }
            DEMON_Handle module = modules.handles[0];
            String8 exe_path = demon_full_path_from_module(scratch.arena, module);
            DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
            RADDBG_Parsed *rdbg = &dbgi->rdbg;
            RADDBG_NameMap *unparsed_map = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_Procedures);
            if(rdbg->procedures != 0 && unparsed_map != 0)
            {
              // rjf: grab parsed name map
              RADDBG_ParsedNameMap map = {0};
              raddbg_name_map_parse(rdbg, unparsed_map, &map);
              
              // rjf: look up binary's built-in entry point name
              String8 builtin_entry_point_name = {0};
              if(rdbg->scope_vmap != 0 && rdbg->procedures != 0)
              {
                U64 builtin_entry_point_voff = dbgi->pe.entry_point;
                U64 scope_idx = raddbg_vmap_idx_from_voff(rdbg->scope_vmap, rdbg->scope_vmap_count, builtin_entry_point_voff);
                RADDBG_Scope *scope = &rdbg->scopes[scope_idx];
                U64 proc_idx = scope->proc_idx;
                RADDBG_Procedure *procedure = &rdbg->procedures[proc_idx];
                U64 name_size = 0;
                U8 *name_ptr = raddbg_string_from_idx(rdbg, procedure->name_string_idx, &name_size);
                builtin_entry_point_name = str8(name_ptr, name_size);
              }
              
              // rjf: grab entry point symbol names we might want to run to
              String8 default_entry_points[] =
              {
                str8_lit("WinMain"),
                str8_lit("wWinMain"),
                str8_lit("main"),
                str8_lit("wmain"),
                str8_lit("WinMainCRTStartup"),
                str8_lit("wWinMainCRTStartup"),
                str8_lit("mainCRTStartup"),
                str8_lit("wmainCRTStartup"),
              };
              
              // rjf: determine if built-in entry point is not one of the defaults, and thus
              // specified by user
              B32 builtin_entry_point_is_special = 0;
              if(builtin_entry_point_name.size != 0)
              {
                builtin_entry_point_is_special = 1;
                for(U64 idx = 0; idx < ArrayCount(default_entry_points); idx += 1)
                {
                  if(str8_match(default_entry_points[idx], builtin_entry_point_name, 0))
                  {
                    builtin_entry_point_is_special = 0;
                    break;
                  }
                }
              }
              
              // rjf: builtin entry point is unique -> use entry point voff
              U64 voff = 0;
              if(voff == 0 && builtin_entry_point_is_special)
              {
                voff = dbgi->pe.entry_point;
              }
              
              // rjf: find voff for one of the custom entry points attached to this msg
              if(voff == 0)
              {
                for(String8Node *n = msg->strings.first; n != 0; n = n->next)
                {
                  U32 procedure_id = 0;
                  {
                    String8 name = n->string;
                    RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, name.str, name.size);
                    if(node != 0)
                    {
                      U32 id_count = 0;
                      U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
                      if(id_count > 0)
                      {
                        procedure_id = ids[0];
                      }
                    }
                  }
                  if(procedure_id != 0)
                  {
                    voff = raddbg_first_voff_from_proc(rdbg, procedure_id);
                    break;
                  }
                }
              }
              
              // rjf: find voff for one of the user's custom entry points
              if(voff == 0)
              {
                for(String8Node *n = ctrl_state->user_entry_points.first; n != 0; n = n->next)
                {
                  U32 procedure_id = 0;
                  {
                    String8 name = n->string;
                    RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, name.str, name.size);
                    if(node != 0)
                    {
                      U32 id_count = 0;
                      U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
                      if(id_count > 0)
                      {
                        procedure_id = ids[0];
                      }
                    }
                  }
                  if(procedure_id != 0)
                  {
                    voff = raddbg_first_voff_from_proc(rdbg, procedure_id);
                    break;
                  }
                }
              }
              
              // rjf: find voff for one of the default entry points
              if(voff == 0)
              {
                for(U64 idx = 0; voff == 0 && idx < ArrayCount(default_entry_points); idx += 1)
                {
                  U32 procedure_id = 0;
                  {
                    String8 name = default_entry_points[idx];
                    RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, name.str, name.size);
                    if(node != 0)
                    {
                      U32 id_count = 0;
                      U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
                      if(id_count > 0)
                      {
                        procedure_id = ids[0];
                      }
                    }
                  }
                  if(procedure_id != 0)
                  {
                    voff = raddbg_first_voff_from_proc(rdbg, procedure_id);
                    break;
                  }
                }
              }
              
              // rjf: nonzero voff? => store
              if(voff != 0)
              {
                U64 base_vaddr = demon_base_vaddr_from_module(module);
                entry_vaddr = base_vaddr + voff;
                entry_vaddr_proc = run_ctrls.run_entities[process_idx];
              }
            }
            
            // rjf: found entry point -> insert into trap controls
            if(entry_vaddr != 0)
            {
              DEMON_Trap trap = {entry_vaddr_proc, entry_vaddr};
              demon_trap_chunk_list_push(scratch.arena, &demon_traps, 256, &trap);
              run_ctrls.traps = demon_traps;
            }
            
            // rjf: no entry point found -> done
            else
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
    event->kind    = CTRL_EventKind_Stopped;
    event->msg_id  = msg->msg_id;
    if(stop_event != 0)
    {
      event->cause          = ctrl_event_cause_from_demon_event_kind(stop_event->kind);
      event->machine_id     = CTRL_MachineID_Client;
      event->entity         = ctrl_handle_from_demon(stop_event->thread);
      event->parent         = ctrl_handle_from_demon(stop_event->process);
      event->exception_code = stop_event->code;
      event->vaddr_rng      = r1u64(stop_event->address, stop_event->address);
      event->rip_vaddr      = stop_event->instruction_pointer;
    }
    ctrl_c2u_push_events(&evts);
  }
  
  //- rjf: push request resolution event
  {
    CTRL_EventList evts = {0};
    CTRL_Event *evt = ctrl_event_list_push(scratch.arena, &evts);
    evt->kind      = CTRL_EventKind_LaunchAndInitDone;
    evt->machine_id= CTRL_MachineID_Client;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = id;
    ctrl_c2u_push_events(&evts);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__attach(CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  
  //- rjf: attach
  B32 attach_successful = demon_attach_process(msg->entity_id);
  
  //- rjf: run to handshake
  if(attach_successful)
  {
    DEMON_Handle unfrozen_process = 0;
    DEMON_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DEMON_EventKind_CreateProcess:
        {
          unfrozen_process = event->process;
          run_ctrls.run_entities = &unfrozen_process;
          run_ctrls.run_entity_count = 1;
        }break;
        case DEMON_EventKind_HandshakeComplete:
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
    evt->machine_id= CTRL_MachineID_Client;
    evt->msg_id    = msg->msg_id;
    evt->entity_id = !!attach_successful * msg->entity_id;
    ctrl_c2u_push_events(&evts);
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ctrl_thread__kill(CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DEMON_Handle process = ctrl_demon_handle_from_ctrl(msg->entity);
  U32 exit_code = msg->exit_code;
  
  //- rjf: send kill
  B32 kill_worked = demon_kill_process(process, exit_code);
  
  //- rjf: wait for process to be dead
  if(demon_object_exists(process) && kill_worked)
  {
    DEMON_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    run_ctrls.run_entities = &process;
    run_ctrls.run_entity_count = 1;
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      if(event->kind == DEMON_EventKind_ExitProcess && event->process == process)
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
    evt->machine_id= CTRL_MachineID_Client;
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
ctrl_thread__detach(CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DEMON_Handle process = ctrl_demon_handle_from_ctrl(msg->entity);
  
  //- rjf: detach
  B32 detach_worked = demon_detach_process(process);
  
  //- rjf: wait for process to be dead
  if(demon_object_exists(process) && detach_worked)
  {
    DEMON_RunCtrls run_ctrls = {0};
    run_ctrls.run_entities_are_unfrozen = 1;
    run_ctrls.run_entities_are_processes = 1;
    run_ctrls.run_entities = &process;
    run_ctrls.run_entity_count = 1;
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      if(event->kind == DEMON_EventKind_ExitProcess && event->process == process)
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
    evt->machine_id= CTRL_MachineID_Client;
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
ctrl_thread__run(CTRL_Msg *msg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DEMON_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  DEMON_Handle target_thread = ctrl_demon_handle_from_ctrl(msg->entity);
  DEMON_Handle target_process = ctrl_demon_handle_from_ctrl(msg->parent);
  U64 spoof_ip_vaddr = 911;
  
  //////////////////////////////
  //- rjf: gather processes
  //
  DEMON_HandleArray processes = demon_all_processes(scratch.arena);
  
  //////////////////////////////
  //- rjf: gather all initial breakpoints
  //
  DEMON_TrapChunkList user_traps = {0};
  {
    // rjf: resolve module-dependent user bps
    for(U64 process_idx = 0; process_idx < processes.count; process_idx += 1)
    {
      DEMON_Handle process = processes.handles[process_idx];
      DEMON_HandleArray modules = demon_modules_from_process(scratch.arena, process);
      for(U64 module_idx = 0; module_idx < modules.count; module_idx += 1)
      {
        DEMON_Handle module = modules.handles[module_idx];
        ctrl_append_resolved_module_user_bp_traps(scratch.arena, process, module, &msg->user_bps, &user_traps);
      }
    }
    
    // rjf: push virtual-address user breakpoints per-process
    for(U64 process_idx = 0; process_idx < processes.count; process_idx += 1)
    {
      ctrl_append_resolved_process_user_bp_traps(scratch.arena, processes.handles[process_idx], &msg->user_bps, &user_traps);
    }
  }
  
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
    DEMON_HandleList stuck_threads = {0};
    for(U64 i = 0; i < processes.count; i += 1)
    {
      DEMON_Handle process = processes.handles[i];
      DEMON_HandleArray threads = demon_threads_from_process(scratch.arena, process);
      for(U64 j = 0; j < threads.count; j += 1)
      {
        DEMON_Handle thread = threads.handles[j];
        U64 rip = demon_read_ip(thread);
        
        // rjf: determine if thread is frozen
        B32 thread_is_frozen = !msg->freeze_state_is_frozen;
        for(CTRL_MachineIDHandlePairNode *n = msg->freeze_state_threads.first; n != 0; n = n->next)
        {
          if(ctrl_demon_handle_from_ctrl(n->v.handle) == thread)
          {
            thread_is_frozen ^= 1;
            break;
          }
        }
        
        // rjf: not frozen? -> check if stuck & gather if so
        if(thread_is_frozen == 0)
        {
          for(DEMON_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
          {
            B32 is_on_user_bp = 0;
            for(DEMON_Trap *trap_ptr = n->v; trap_ptr < n->v+n->count; trap_ptr += 1)
            {
              if(trap_ptr->process == process && trap_ptr->address == rip)
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
            
            if(is_on_user_bp && (!is_on_net_trap || thread != target_thread))
            {
              demon_handle_list_push(scratch.arena, &stuck_threads, thread);
            }
            
            if(is_on_user_bp && is_on_net_trap && thread == target_thread)
            {
              target_thread_is_on_user_bp_and_trap_net_trap = 1;
            }
          }
        }
      }
    }
    
    // rjf: actually step stuck threads
    for(DEMON_HandleNode *node = stuck_threads.first;
        node != 0;
        node = node->next)
    {
      DEMON_RunCtrls run_ctrls = {0};
      run_ctrls.single_step_thread = node->v;
      for(B32 done = 0; done == 0;)
      {
        DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
        switch(event->kind)
        {
          default:{}break;
          case DEMON_EventKind_Error:      stop_cause = CTRL_EventCause_Error; goto stop;
          case DEMON_EventKind_Exception:  stop_cause = CTRL_EventCause_InterruptedByException; goto stop;
          case DEMON_EventKind_Trap:       stop_cause = CTRL_EventCause_InterruptedByTrap; goto stop;
          case DEMON_EventKind_Halt:       stop_cause = CTRL_EventCause_InterruptedByHalt; goto stop;
          stop:;
          {
            stop_event = event;
            done = 1;
          }break;
          case DEMON_EventKind_SingleStep:
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
  DEMON_TrapChunkList trap_net_traps = {0};
  for(CTRL_TrapNode *node = msg->traps.first;
      node != 0;
      node = node->next)
  {
    DEMON_Trap trap = {target_process, node->v.vaddr};
    demon_trap_chunk_list_push(scratch.arena, &trap_net_traps, 256, &trap);
  }
  
  //////////////////////////////
  //- rjf: join user breakpoints and trap net traps
  //
  DEMON_TrapChunkList joined_traps = {0};
  {
    demon_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &user_traps);
    demon_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &trap_net_traps);
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
    U64 sp_check_value = demon_read_sp(target_thread);
    B32 spoof_mode = 0;
    CTRL_Spoof spoof = {0};
    for(;;)
    {
      //////////////////////////
      //- rjf: choose low level traps
      //
      DEMON_TrapChunkList *trap_list = &joined_traps;
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
      DEMON_RunCtrls run_ctrls = {0};
      run_ctrls.ignore_previous_exception = 1;
      run_ctrls.run_entity_count = msg->freeze_state_threads.count;
      run_ctrls.run_entities     = push_array(scratch.arena, DEMON_Handle, run_ctrls.run_entity_count);
      run_ctrls.run_entities_are_unfrozen = !msg->freeze_state_is_frozen;
      {
        U64 idx = 0;
        for(CTRL_MachineIDHandlePairNode *n = msg->freeze_state_threads.first; n != 0; n = n->next)
        {
          run_ctrls.run_entities[idx] = ctrl_demon_handle_from_ctrl(n->v.handle);
          idx += 1;
        }
      }
      run_ctrls.traps = *trap_list;
      
      //////////////////////////
      //- rjf: get next event
      //
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, run_spoof);
      
      //////////////////////////
      //- rjf: determine event handling
      //
      B32 hard_stop = 0;
      CTRL_EventCause hard_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
      B32 use_stepping_logic = 0;
      switch(event->kind)
      {
        default:{}break;
        case DEMON_EventKind_Error:
        case DEMON_EventKind_Halt:
        case DEMON_EventKind_SingleStep:
        case DEMON_EventKind_Trap:
        {
          hard_stop = 1;
        }break;
        case DEMON_EventKind_Exception:
        case DEMON_EventKind_Breakpoint:
        {
          use_stepping_logic = 1;
        }break;
        case DEMON_EventKind_CreateProcess:
        {
          DEMON_TrapChunkList new_traps = {0};
          ctrl_append_resolved_process_user_bp_traps(scratch.arena, event->process, &msg->user_bps, &new_traps);
          demon_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
        }break;
        case DEMON_EventKind_LoadModule:
        {
          DEMON_TrapChunkList new_traps = {0};
          ctrl_append_resolved_module_user_bp_traps(scratch.arena, event->process, event->module, &msg->user_bps, &new_traps);
          demon_trap_chunk_list_concat_shallow_copy(scratch.arena, &joined_traps, &new_traps);
          demon_trap_chunk_list_concat_shallow_copy(scratch.arena, &user_traps, &new_traps);
        }break;
      }
      
      //////////////////////////
      //- rjf: unpack info about thread attached to event
      //
      Architecture arch = demon_arch_from_object(event->thread);
      U64 reg_size = regs_block_size_from_architecture(arch);
      void *thread_regs_block = demon_read_regs(event->thread);
      U64 thread_rip_vaddr = regs_rip_from_arch_block(arch, thread_regs_block);
      DEMON_Handle module = 0;
      U64 thread_rip_voff = 0;
      {
        Temp temp = temp_begin(scratch.arena);
        DEMON_HandleArray modules = demon_modules_from_process(temp.arena, event->process);
        if(modules.count != 0)
        {
          for(U64 idx = 0; idx < modules.count; idx += 1)
          {
            Rng1U64 vaddr_range = demon_vaddr_range_from_module(modules.handles[idx]);
            if(contains_1u64(vaddr_range, thread_rip_vaddr))
            {
              module = modules.handles[idx];
              thread_rip_voff = thread_rip_vaddr - vaddr_range.min;
              break;
            }
          }
        }
        temp_end(temp);
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
        if(event->kind == DEMON_EventKind_Exception)
        {
          // rjf: spoof check
          if(spoof_mode &&
             target_process == event->process &&
             target_thread == event->thread &&
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
        if(event->kind == DEMON_EventKind_Breakpoint)
        {
          Temp temp = temp_begin(scratch.arena);
          String8List conditions = {0};
          
          // rjf: user breakpoints
          for(DEMON_TrapChunkNode *n = user_traps.first; n != 0; n = n->next)
          {
            DEMON_Trap *trap = n->v;
            DEMON_Trap *opl = n->v + n->count;
            for(;trap < opl; trap += 1)
            {
              if(trap->process == event->process &&
                 trap->address == event->instruction_pointer &&
                 (event->thread != target_thread || !target_thread_is_on_user_bp_and_trap_net_trap))
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
            String8 exe_path = demon_full_path_from_module(temp.arena, module);
            DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
            RADDBG_Parsed *rdbg = &dbgi->rdbg;
            for(String8Node *condition_n = conditions.first; condition_n != 0; condition_n = condition_n->next)
            {
              String8 string = condition_n->string;
              EVAL_ParseCtx parse_ctx = zero_struct;
              {
                parse_ctx.arch = arch;
                parse_ctx.ip_voff = thread_rip_voff;
                parse_ctx.rdbg = rdbg;
                parse_ctx.type_graph = tg_graph_begin(bit_size_from_arch(arch)/8, 256);
                parse_ctx.regs_map = ctrl_string2reg_from_arch(arch);
                parse_ctx.reg_alias_map = ctrl_string2alias_from_arch(arch);
                parse_ctx.locals_map = eval_push_locals_map_from_raddbg_voff(temp.arena, rdbg, thread_rip_voff);
                parse_ctx.member_map = eval_push_member_map_from_raddbg_voff(temp.arena, rdbg, thread_rip_voff);
              }
              EVAL_TokenArray tokens = eval_token_array_from_text(temp.arena, string);
              EVAL_ParseResult parse = eval_parse_expr_from_text_tokens(temp.arena, &parse_ctx, string, &tokens);
              EVAL_ErrorList errors = parse.errors;
              B32 parse_has_expr = (parse.expr != &eval_expr_nil);
              B32 parse_is_type = (parse_has_expr && parse.expr->kind == EVAL_ExprKind_TypeIdent);
              EVAL_IRTreeAndType ir_tree_and_type = {&eval_irtree_nil};
              if(parse_has_expr && errors.count == 0)
              {
                ir_tree_and_type = eval_irtree_and_type_from_expr(temp.arena, parse_ctx.type_graph, rdbg, parse.expr, &errors);
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
                U64 module_base = demon_base_vaddr_from_module(module);
                U64 tls_base = 0; // TODO(rjf)
                EVAL_Machine machine = {0};
                machine.u = (void *)event->process;
                machine.arch = arch;
                machine.memory_read = ctrl_eval_memory_read;
                machine.reg_data = thread_regs_block;
                machine.reg_size = reg_size;
                machine.module_base = &module_base;
                machine.tls_base = &tls_base;
                eval = eval_interpret(&machine, bytecode);
              }
              if(eval.bad_eval == 0 && eval.value.u64 == 0)
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
          if(!hit_user_bp && event->process == target_process)
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
        DEMON_RunCtrls single_step_ctrls = {0};
        single_step_ctrls.single_step_thread = event->thread;
        for(B32 single_step_done = 0; single_step_done == 0;)
        {
          DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &single_step_ctrls, 0);
          switch(event->kind)
          {
            default:{}break;
            case DEMON_EventKind_Error:
            case DEMON_EventKind_Exception:
            case DEMON_EventKind_Halt:
            case DEMON_EventKind_Trap:
            {
              cond_bp_single_step_stop = 1;
              single_step_done = 1;
              use_stepping_logic = 0;
              cond_bp_single_step_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
            }break;
            case DEMON_EventKind_SingleStep:
            {
              single_step_done = 1;
              cond_bp_single_step_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
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
        if(event->thread != target_thread)
        {
          step_past_trap_net = 1;
          use_stepping_logic = 0;
        }
      }
      
      //- rjf: trap net on on-target threads trigger trap net logic
      B32 use_trap_net_logic = 0;
      if(use_stepping_logic && hit_trap_net_bp)
      {
        if(event->thread == target_thread)
        {
          use_trap_net_logic = 1;
        }
      }
      
      //- rjf: trap net logic: stack pointer check
      B32 stack_pointer_matches = 0;
      if(use_trap_net_logic)
      {
        U64 sp = demon_read_sp(target_thread);
        stack_pointer_matches = (sp == sp_check_value);
      }
      
      //- rjf: trap net logic: single step after hit
      B32 single_step_stop = 0;
      CTRL_EventCause single_step_stop_cause = CTRL_EventCause_Null;
      if(use_trap_net_logic)
      {
        if(hit_trap_flags & CTRL_TrapFlag_SingleStepAfterHit)
        {
          DEMON_RunCtrls single_step_ctrls = {0};
          single_step_ctrls.single_step_thread = target_thread;
          for(B32 single_step_done = 0; single_step_done == 0;)
          {
            DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &single_step_ctrls, 0);
            switch(event->kind)
            {
              default:{}break;
              case DEMON_EventKind_Error:
              case DEMON_EventKind_Exception:
              case DEMON_EventKind_Halt:
              case DEMON_EventKind_Trap:
              {
                single_step_stop = 1;
                single_step_done = 1;
                use_stepping_logic = 0;
                use_trap_net_logic = 0;
                single_step_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
              }break;
              case DEMON_EventKind_SingleStep:
              {
                single_step_done = 1;
                single_step_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
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
          U64 spoof_sp = demon_read_sp(target_thread);
          spoof_mode = 1;
          spoof.process = ctrl_handle_from_demon(target_process);
          spoof.thread  = ctrl_handle_from_demon(target_thread);
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
            sp_check_value = demon_read_sp(target_thread);
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
        DEMON_RunCtrls single_step_ctrls = {0};
        single_step_ctrls.single_step_thread = event->thread;
        for(B32 single_step_done = 0; single_step_done == 0;)
        {
          DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &single_step_ctrls, 0);
          switch(event->kind)
          {
            default:{}break;
            case DEMON_EventKind_Error:
            case DEMON_EventKind_Exception:
            case DEMON_EventKind_Halt:
            case DEMON_EventKind_Trap:
            {
              step_past_trap_net_stop = 1;
              single_step_done = 1;
              step_past_trap_net_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
            }break;
            case DEMON_EventKind_SingleStep:
            {
              single_step_done = 1;
              step_past_trap_net_stop_cause = ctrl_event_cause_from_demon_event_kind(event->kind);
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
    event->machine_id = CTRL_MachineID_Client;
    event->entity = ctrl_handle_from_demon(stop_event->thread);
    event->parent = ctrl_handle_from_demon(stop_event->process);
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
ctrl_thread__single_step(CTRL_Msg *msg)
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
  DEMON_Event *stop_event = 0;
  CTRL_EventCause stop_cause = CTRL_EventCause_Null;
  {
    DEMON_RunCtrls run_ctrls = {0};
    run_ctrls.single_step_thread = ctrl_demon_handle_from_ctrl(msg->entity);
    for(B32 done = 0; done == 0;)
    {
      DEMON_Event *event = ctrl_thread__next_demon_event(scratch.arena, msg, &run_ctrls, 0);
      switch(event->kind)
      {
        default:{}break;
        case DEMON_EventKind_Error:      {stop_cause = CTRL_EventCause_Error;}goto end_single_step;
        case DEMON_EventKind_Exception:  {stop_cause = CTRL_EventCause_InterruptedByException;}goto end_single_step;
        case DEMON_EventKind_Halt:       {stop_cause = CTRL_EventCause_InterruptedByHalt;}goto end_single_step;
        case DEMON_EventKind_Trap:       {stop_cause = CTRL_EventCause_InterruptedByTrap;}goto end_single_step;
        case DEMON_EventKind_SingleStep: {stop_cause = CTRL_EventCause_Finished;}goto end_single_step;
        case DEMON_EventKind_Breakpoint: {stop_cause = CTRL_EventCause_UserBreakpoint;}goto end_single_step;
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
    event->machine_id = CTRL_MachineID_Client;
    event->entity = ctrl_handle_from_demon(stop_event->thread);
    event->parent = ctrl_handle_from_demon(stop_event->process);
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

//- rjf: entry point

internal void
ctrl_mem_stream_thread__entry_point(void *p)
{
  TCTX tctx_ = {0};
  tctx_init_and_equip(&tctx_);
  CTRL_ProcessMemoryCache *cache = &ctrl_state->process_memory_cache;
  for(;;)
  {
    //- rjf: unpack next request
    CTRL_MachineID machine_id = 0;
    CTRL_Handle process = {0};
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
    U64 preexisting_memgen_idx = 0;
    U128 preexisting_hash = {0};
    OS_MutexScopeW(process_stripe->rw_mutex)
    {
      for(CTRL_ProcessMemoryCacheNode *n = process_slot->first; n != 0; n = n->next)
      {
        if(n->machine_id == machine_id && ctrl_handle_match(n->process, process))
        {
          U64 range_slot_idx = range_hash%n->range_hash_slots_count;
          CTRL_ProcessMemoryRangeHashSlot *range_slot = &n->range_hash_slots[range_slot_idx];
          for(CTRL_ProcessMemoryRangeHashNode *range_n = range_slot->first; range_n != 0; range_n = range_n->next)
          {
            if(MemoryMatchStruct(&range_n->vaddr_range, &vaddr_range) && range_n->zero_terminated == zero_terminated)
            {
              got_task = !ins_atomic_u32_eval_cond_assign(&range_n->is_taken, 1, 0);
              preexisting_memgen_idx = range_n->memgen_idx;
              preexisting_hash = range_n->hash;
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
    U64 memgen_idx = ctrl_memgen_idx();
    if(got_task && memgen_idx != preexisting_memgen_idx)
    {
      range_size = dim_1u64(vaddr_range);
      U64 arena_size = AlignPow2(range_size + ARENA_HEADER_SIZE, KB(64));
      range_arena = arena_alloc__sized(range_size+ARENA_HEADER_SIZE, range_size+ARENA_HEADER_SIZE);
      range_base = push_array_no_zero(range_arena, U8, range_size);
      U64 bytes_read = ctrl_process_read(machine_id, process, vaddr_range, range_base);
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
        if(n->machine_id == machine_id && ctrl_handle_match(n->process, process))
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
                range_n->memgen_idx = memgen_idx;
              }
              ins_atomic_u32_eval_assign(&range_n->is_taken, 0);
              goto commit__break_all;
            }
          }
        }
      }
      commit__break_all:;
    }
  }
}
