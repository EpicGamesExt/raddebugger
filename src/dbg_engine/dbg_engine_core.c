// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_engine.meta.c"

////////////////////////////////
//~ rjf: @eval2 Bundled Evaluation Path

internal D_Eval
d_eval_from_string(Arena *arena, String8 string)
{
  D_Eval eval = {0};
  {
    E2_MsgList msgs = {0};
    
    //- rjf: string -> expr
    E2_Expr *expr = &e2_expr_nil;
    {
      E2_ParseState state = {0};
      B32 identifier_is_type = 0;
      for(;;)
      {
        E2_Parse parse = e2_parse_from_string(arena, &state, identifier_is_type, E2_LangKind_CLike, string);
        identifier_is_type = 0;
        expr = parse.expr;
        e2_msg_list_concat_in_place(&msgs, &parse.msgs);
        if(e2_parse_status_is_terminal(parse.status))
        {
          break;
        }
      }
    }
    
    //- rjf: expr -> irtree
    E2_IRNode *irtree = &e2_irnode_nil;
    {
      E2_IRNode *resolve_result = &e2_irnode_nil;
      E2_Val compile_time_eval_result = {0};
      E2_CompileState state = {0};
      for(;;)
      {
        E2_Compile compile = e2_compile_from_expr(arena, &state, resolve_result, compile_time_eval_result, expr);
        resolve_result = &e2_irnode_nil;
        MemoryZeroStruct(&compile_time_eval_result);
        irtree = compile.irtree;
        e2_msg_list_concat_in_place(&msgs, &compile.msgs);
        switch(compile.status)
        {
          default:{}break;
          case E2_CompileStatus_MissedIdentifierResolution:
          {
            resolve_result = e2_irnode_const_u64_or_smaller(arena, 123);
          }break;
        }
        if(e2_compile_status_is_terminal(compile.status))
        {
          break;
        }
      }
    }
    
    //- rjf: ir tree -> bytecode
    String8 bytecode = e2_bytecode_from_irnode(arena, irtree);
    
    //- rjf: bytecode -> value
    E2_Val val = {0};
    {
      E2_InterpState state = {0};
      E2_SpaceMap space_map = {0};
      for(;;)
      {
        E2_Interp interp = e2_interp_from_bytecode(arena, &state, &space_map, bytecode);
        val = interp.val;
        e2_msg_list_concat_in_place(&msgs, &interp.msgs);
        if(e2_interp_status_is_terminal(interp.status))
        {
          break;
        }
      }
    }
    
    //- rjf: fill
    eval.string   = string;
    eval.expr     = expr;
    eval.irtree   = irtree;
    eval.bytecode = bytecode;
    eval.val      = val;
    eval.msgs     = msgs;
  }
  return eval;
}

////////////////////////////////
//~ rjf: Layer Initialization

internal void
d_init(void)
{
  //- rjf: set up ctrl state
  {
    Arena *arena = arena_alloc();
    d_ctrl_state = push_array(arena, D_CtrlState, 1);
    d_ctrl_state->arena = arena;
    for EachEnumVal(Arch, arch)
    {
      ARCH_Info *arch_info = arch_info_from_arch(arch);
      U64 reg_count = arch_info->reg_code_count;
      String8 *reg_names = arch_info->reg_code_name_table;
      d_ctrl_state->arch_string2reg_tables[arch] = e_string2num_map_make(d_ctrl_state->arena, 256);
      for(U64 idx = 1; idx < reg_count; idx += 1)
      {
        e_string2num_map_insert(d_ctrl_state->arena, &d_ctrl_state->arch_string2reg_tables[arch], reg_names[idx], idx);
      }
    }
    d_ctrl_state->thread_reg_cache.slots_count = 1024;
    d_ctrl_state->thread_reg_cache.slots = push_array(arena, D_ThreadRegCacheSlot, d_ctrl_state->thread_reg_cache.slots_count);
    d_ctrl_state->thread_reg_cache.stripes_count = get_system_info()->logical_processor_count;
    d_ctrl_state->thread_reg_cache.stripes = push_array(arena, D_ThreadRegCacheStripe, d_ctrl_state->thread_reg_cache.stripes_count);
    for(U64 idx = 0; idx < d_ctrl_state->thread_reg_cache.stripes_count; idx += 1)
    {
      d_ctrl_state->thread_reg_cache.stripes[idx].arena = arena_alloc();
      d_ctrl_state->thread_reg_cache.stripes[idx].rw_mutex = rw_mutex_alloc();
    }
    d_ctrl_state->module_info_cache.slots_count = 1024;
    d_ctrl_state->module_info_cache.slots = push_array(arena, D_ModuleInfoCacheSlot, d_ctrl_state->module_info_cache.slots_count);
    d_ctrl_state->module_info_cache.stripes = stripe_array_alloc(arena);
    d_ctrl_state->u2c_ring_size = KB(64);
    d_ctrl_state->u2c_ring_base = push_array_no_zero(arena, U8, d_ctrl_state->u2c_ring_size);
    d_ctrl_state->u2c_ring_mutex = mutex_alloc();
    d_ctrl_state->u2c_ring_cv = cond_var_alloc();
    d_ctrl_state->c2u_ring_size = KB(64);
    d_ctrl_state->c2u_ring_max_string_size = d_ctrl_state->c2u_ring_size/2;
    d_ctrl_state->c2u_ring_base = push_array_no_zero(arena, U8, d_ctrl_state->c2u_ring_size);
    d_ctrl_state->c2u_ring_mutex = mutex_alloc();
    d_ctrl_state->c2u_ring_cv = cond_var_alloc();
    {
      d_ctrl_state->ctrl_thread_log_path = push_str8f(d_ctrl_state->arena, "%S/ctrl_thread.raddbg_log", g_logs_folder);
      write_data_to_file_path(d_ctrl_state->ctrl_thread_log_path, str8_zero());
    }
    d_ctrl_state->ctrl_thread_entity_ctx_rw_mutex = rw_mutex_alloc();
    d_ctrl_state->ctrl_thread_entity_store = d_entity_ctx_rw_store_alloc();
    d_ctrl_state->ctrl_thread_eval_cache = e_cache_alloc();
    d_ctrl_state->ctrl_thread_msg_process_arena = arena_alloc();
    d_ctrl_state->dmn_event_arena = arena_alloc();
    d_ctrl_state->user_entry_point_arena = arena_alloc();
    d_ctrl_state->dbg_dir_arena = arena_alloc();
    for(D_ExceptionCodeKind k = (D_ExceptionCodeKind)0; k < D_ExceptionCodeKind_COUNT; k = (D_ExceptionCodeKind)(k+1))
    {
      if(d_exception_code_kind_default_enable_table[k])
      {
        d_ctrl_state->exception_code_filters[k/64] |= 1ull<<(k%64);
      }
    }
    d_ctrl_state->ctrl_thread_log = log_alloc();
    d_ctrl_state->ctrl_thread = thread_launch(d_ctrl_thread__entry_point, 0);
    d_ctrl_state->dump_cache.slots_count = 64;
    d_ctrl_state->dump_cache.slots = push_array(arena, D_DumpSlot, d_ctrl_state->dump_cache.slots_count);
    d_ctrl_state->dump_cache.stripes = stripe_array_alloc(arena);
  }
  
  //- rjf: set up user state
  {
    Arena *arena = arena_alloc();
    d_user_state = push_array(arena, D_UserState, 1);
    d_user_state->arena = arena;
    d_user_state->cmds_arena = arena_alloc();
    d_user_state->output_log_root = c_root_alloc();
    d_user_state->ctrl_entity_store = d_entity_ctx_rw_store_alloc();
    d_user_state->ctrl_stop_arena = arena_alloc();
    d_user_state->ctrl_msg_arena = arena_alloc();
    
    // rjf: set up caches
    for(U64 idx = 0; idx < ArrayCount(d_user_state->tls_base_caches); idx += 1)
    {
      d_user_state->tls_base_caches[idx].arena = arena_alloc();
    }
    for(U64 idx = 0; idx < ArrayCount(d_user_state->locals_caches); idx += 1)
    {
      d_user_state->locals_caches[idx].arena = arena_alloc();
    }
    for(U64 idx = 0; idx < ArrayCount(d_user_state->member_caches); idx += 1)
    {
      d_user_state->member_caches[idx].arena = arena_alloc();
    }
    
    // rjf: set up run state
    d_user_state->ctrl_last_run_arena = arena_alloc();
  }
  
  //- rjf: select user state's entity context
  d_select_entity_ctx(&d_user_state->ctrl_entity_store->ctx);
}
