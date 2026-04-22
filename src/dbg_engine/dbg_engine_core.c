// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_engine.meta.c"

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
    for(Arch arch = (Arch)0; arch < Arch_COUNT; arch = (Arch)(arch+1))
    {
      String8 *reg_names = regs_reg_code_string_table_from_arch(arch);
      U64 reg_count = regs_reg_code_count_from_arch(arch);
      String8 *alias_names = regs_alias_code_string_table_from_arch(arch);
      U64 alias_count = regs_alias_code_count_from_arch(arch);
      d_ctrl_state->arch_string2reg_tables[arch] = e_string2num_map_make(d_ctrl_state->arena, 256);
      d_ctrl_state->arch_string2alias_tables[arch] = e_string2num_map_make(d_ctrl_state->arena, 256);
      for(U64 idx = 1; idx < reg_count; idx += 1)
      {
        e_string2num_map_insert(d_ctrl_state->arena, &d_ctrl_state->arch_string2reg_tables[arch], reg_names[idx], idx);
      }
      for(U64 idx = 1; idx < alias_count; idx += 1)
      {
        e_string2num_map_insert(d_ctrl_state->arena, &d_ctrl_state->arch_string2alias_tables[arch], alias_names[idx], idx);
      }
    }
    d_ctrl_state->thread_reg_cache.slots_count = 1024;
    d_ctrl_state->thread_reg_cache.slots = push_array(arena, D_ThreadRegCacheSlot, d_ctrl_state->thread_reg_cache.slots_count);
    d_ctrl_state->thread_reg_cache.stripes_count = os_get_system_info()->logical_processor_count;
    d_ctrl_state->thread_reg_cache.stripes = push_array(arena, D_ThreadRegCacheStripe, d_ctrl_state->thread_reg_cache.stripes_count);
    for(U64 idx = 0; idx < d_ctrl_state->thread_reg_cache.stripes_count; idx += 1)
    {
      d_ctrl_state->thread_reg_cache.stripes[idx].arena = arena_alloc();
      d_ctrl_state->thread_reg_cache.stripes[idx].rw_mutex = rw_mutex_alloc();
    }
    d_ctrl_state->module_image_info_cache.slots_count = 1024;
    d_ctrl_state->module_image_info_cache.slots = push_array(arena, D_ModuleImageInfoCacheSlot, d_ctrl_state->module_image_info_cache.slots_count);
    d_ctrl_state->module_image_info_cache.stripes_count = os_get_system_info()->logical_processor_count;
    d_ctrl_state->module_image_info_cache.stripes = push_array(arena, D_ModuleImageInfoCacheStripe, d_ctrl_state->module_image_info_cache.stripes_count);
    for(U64 idx = 0; idx < d_ctrl_state->module_image_info_cache.stripes_count; idx += 1)
    {
      d_ctrl_state->module_image_info_cache.stripes[idx].arena = arena_alloc();
      d_ctrl_state->module_image_info_cache.stripes[idx].rw_mutex = rw_mutex_alloc();
    }
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
      Temp scratch = scratch_begin(0, 0);
      String8 user_program_data_path = os_get_process_info()->user_program_data_path;
      String8 user_data_folder = push_str8f(scratch.arena, "%S/raddbg/logs", user_program_data_path);
      os_make_directory(user_data_folder);
      d_ctrl_state->ctrl_thread_log_path = push_str8f(d_ctrl_state->arena, "%S/ctrl_thread.raddbg_log", user_data_folder);
      os_write_data_to_file_path(d_ctrl_state->ctrl_thread_log_path, str8_zero());
      scratch_end(scratch);
    }
    d_ctrl_state->ctrl_thread_entity_ctx_rw_mutex = rw_mutex_alloc();
    d_ctrl_state->ctrl_thread_entity_store = ctrl_entity_ctx_rw_store_alloc();
    d_ctrl_state->ctrl_thread_eval_cache = e_cache_alloc();
    d_ctrl_state->ctrl_thread_msg_process_arena = arena_alloc();
    d_ctrl_state->dmn_event_arena = arena_alloc();
    d_ctrl_state->user_entry_point_arena = arena_alloc();
    d_ctrl_state->dbg_dir_arena = arena_alloc();
    for(D_ExceptionCodeKind k = (D_ExceptionCodeKind)0; k < D_ExceptionCodeKind_COUNT; k = (D_ExceptionCodeKind)(k+1))
    {
      if(ctrl_exception_code_kind_default_enable_table[k])
      {
        d_ctrl_state->exception_code_filters[k/64] |= 1ull<<(k%64);
      }
    }
    d_ctrl_state->ctrl_thread_log = log_alloc();
    d_ctrl_state->ctrl_thread = thread_launch(ctrl_thread__entry_point, 0);
  }
  
  //- rjf: set up user state
  {
    Arena *arena = arena_alloc();
    d_user_state = push_array(arena, D_UserState, 1);
    d_user_state->arena = arena;
    d_user_state->cmds_arena = arena_alloc();
    d_user_state->output_log_key = c_key_make(c_root_alloc(), c_id_make(0, 0));
    c_submit_data(d_user_state->output_log_key, 0, str8_zero());
    d_user_state->ctrl_entity_store = ctrl_entity_ctx_rw_store_alloc();
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
}
