// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
dbgi_init(void)
{
  Arena *arena = arena_alloc();
  dbgi_shared = push_array(arena, DBGI_Shared, 1);
  dbgi_shared->arena = arena;
  dbgi_shared->force_slots_count = 1024;
  dbgi_shared->force_stripes_count = 64;
  dbgi_shared->force_slots = push_array(arena, DBGI_ForceSlot, dbgi_shared->force_slots_count);
  dbgi_shared->force_stripes = push_array(arena, DBGI_ForceStripe, dbgi_shared->force_stripes_count);
  for(U64 idx = 0; idx < dbgi_shared->force_stripes_count; idx += 1)
  {
    dbgi_shared->force_stripes[idx].arena = arena_alloc();
    dbgi_shared->force_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    dbgi_shared->force_stripes[idx].cv = os_condition_variable_alloc();
  }
  dbgi_shared->binary_slots_count = 1024;
  dbgi_shared->binary_stripes_count = 64;
  dbgi_shared->binary_slots = push_array(arena, DBGI_BinarySlot, dbgi_shared->binary_slots_count);
  dbgi_shared->binary_stripes = push_array(arena, DBGI_BinaryStripe, dbgi_shared->binary_stripes_count);
  for(U64 idx = 0; idx < dbgi_shared->binary_stripes_count; idx += 1)
  {
    dbgi_shared->binary_stripes[idx].arena = arena_alloc();
    dbgi_shared->binary_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    dbgi_shared->binary_stripes[idx].cv = os_condition_variable_alloc();
  }
  dbgi_shared->fuzzy_search_slots_count = 64;
  dbgi_shared->fuzzy_search_stripes_count = 8;
  dbgi_shared->fuzzy_search_slots = push_array(arena, DBGI_FuzzySearchSlot, dbgi_shared->fuzzy_search_slots_count);
  dbgi_shared->fuzzy_search_stripes = push_array(arena, DBGI_FuzzySearchStripe, dbgi_shared->fuzzy_search_stripes_count);
  for(U64 idx = 0; idx < dbgi_shared->fuzzy_search_stripes_count; idx += 1)
  {
    dbgi_shared->fuzzy_search_stripes[idx].arena = arena_alloc();
    dbgi_shared->fuzzy_search_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    dbgi_shared->fuzzy_search_stripes[idx].cv = os_condition_variable_alloc();
  }
  dbgi_shared->u2p_ring_mutex = os_mutex_alloc();
  dbgi_shared->u2p_ring_cv = os_condition_variable_alloc();
  dbgi_shared->u2p_ring_size = KB(64);
  dbgi_shared->u2p_ring_base = push_array_no_zero(arena, U8, dbgi_shared->u2p_ring_size);
  dbgi_shared->p2u_ring_mutex = os_mutex_alloc();
  dbgi_shared->p2u_ring_cv = os_condition_variable_alloc();
  dbgi_shared->p2u_ring_size = KB(64);
  dbgi_shared->p2u_ring_base = push_array_no_zero(arena, U8, dbgi_shared->p2u_ring_size);
  dbgi_shared->parse_thread_count = Max(os_logical_core_count()-1, 1);
  dbgi_shared->parse_threads = push_array(arena, OS_Handle, dbgi_shared->parse_thread_count);
  for(U64 idx = 0; idx < dbgi_shared->parse_thread_count; idx += 1)
  {
    dbgi_shared->parse_threads[idx] = os_launch_thread(dbgi_parse_thread_entry_point, (void *)idx, 0);
  }
  dbgi_shared->fuzzy_thread_count = Clamp(1, os_logical_core_count()-1, 1);
  dbgi_shared->fuzzy_threads = push_array(arena, DBGI_FuzzySearchThread, dbgi_shared->fuzzy_thread_count);
  for(U64 idx = 0; idx < dbgi_shared->fuzzy_thread_count; idx += 1)
  {
    DBGI_FuzzySearchThread *thread = &dbgi_shared->fuzzy_threads[idx];
    thread->u2f_ring_mutex = os_mutex_alloc();
    thread->u2f_ring_cv = os_condition_variable_alloc();
    thread->u2f_ring_size = KB(64);
    thread->u2f_ring_base = push_array_no_zero(dbgi_shared->arena, U8, thread->u2f_ring_size);
    thread->thread = os_launch_thread(dbgi_fuzzy_thread__entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Thread-Context Idempotent Initialization

internal void
dbgi_ensure_tctx_inited(void)
{
  if(dbgi_tctx == 0)
  {
    Arena *arena = arena_alloc();
    dbgi_tctx = push_array(arena, DBGI_ThreadCtx, 1);
    dbgi_tctx->arena = arena;
  }
}

////////////////////////////////
//~ rjf: Helpers

internal U64
dbgi_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
dbgi_fuzzy_item_num_from_array_element_idx__linear_search(DBGI_FuzzySearchItemArray *array, U64 element_idx)
{
  U64 fuzzy_item_num = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    if(array->v[idx].idx == element_idx)
    {
      fuzzy_item_num = idx+1;
      break;
    }
  }
  return fuzzy_item_num;
}

internal String8
dbgi_fuzzy_item_string_from_rdbg_target_element_idx(RDI_Parsed *rdbg, DBGI_FuzzySearchTarget target, U64 element_idx)
{
  String8 result = {0};
  switch(target)
  {
    // NOTE(rjf): no default - warn if we miss a case
    case DBGI_FuzzySearchTarget_Procedures:
    {
      RDI_Procedure *proc = rdi_element_from_idx(rdbg, procedures, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdbg, proc->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case DBGI_FuzzySearchTarget_GlobalVariables:
    {
      RDI_GlobalVariable *gvar = rdi_element_from_idx(rdbg, global_variables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdbg, gvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case DBGI_FuzzySearchTarget_ThreadVariables:
    {
      RDI_ThreadVariable *tvar = rdi_element_from_idx(rdbg, thread_variables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdbg, tvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case DBGI_FuzzySearchTarget_UDTs:
    {
      RDI_UDT *udt = rdi_element_from_idx(rdbg, udts, element_idx);
      RDI_TypeNode *type_node = rdi_element_from_idx(rdbg, type_nodes, udt->self_type_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdbg, type_node->user_defined.name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case DBGI_FuzzySearchTarget_COUNT:{}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Forced Override Cache Functions

internal void
dbgi_force_exe_path_dbg_path(String8 exe_path, String8 dbg_path)
{
  U64 hash = dbgi_hash_from_string(exe_path);
  U64 slot_idx = hash%dbgi_shared->force_slots_count;
  U64 stripe_idx = slot_idx%dbgi_shared->force_stripes_count;
  DBGI_ForceSlot *slot = &dbgi_shared->force_slots[slot_idx];
  DBGI_ForceStripe *stripe = &dbgi_shared->force_stripes[stripe_idx];
  OS_MutexScopeW(stripe->rw_mutex)
  {
    DBGI_ForceNode *node = 0;
    for(DBGI_ForceNode *n = slot->first; n != 0; n = n->next)
    {
      if(str8_match(n->exe_path, exe_path, 0))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = push_array(stripe->arena, DBGI_ForceNode, 1);
      SLLQueuePush(slot->first, slot->first, node);
      node->exe_path = push_str8_copy(stripe->arena, exe_path);
      node->dbg_path_cap = 1024;
      node->dbg_path_base = push_array_no_zero(stripe->arena, U8, node->dbg_path_cap);
    }
    String8 dbg_path_clamped = dbg_path;
    dbg_path_clamped.size = Min(dbg_path_clamped.size, node->dbg_path_cap);
    MemoryCopy(node->dbg_path_base, dbg_path_clamped.str, dbg_path_clamped.size);
    node->dbg_path_size = dbg_path_clamped.size;
  }
}

internal String8
dbgi_forced_dbg_path_from_exe_path(Arena *arena, String8 exe_path)
{
  String8 result = {0};
  U64 hash = dbgi_hash_from_string(exe_path);
  U64 slot_idx = hash%dbgi_shared->force_slots_count;
  U64 stripe_idx = slot_idx%dbgi_shared->force_stripes_count;
  DBGI_ForceSlot *slot = &dbgi_shared->force_slots[slot_idx];
  DBGI_ForceStripe *stripe = &dbgi_shared->force_stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    DBGI_ForceNode *node = 0;
    for(DBGI_ForceNode *n = slot->first; n != 0; n = n->next)
    {
      if(str8_match(exe_path, n->exe_path, 0))
      {
        node = n;
        break;
      }
    }
    if(node != 0)
    {
      String8 dbg_path = str8(node->dbg_path_base, node->dbg_path_size);
      result = push_str8_copy(arena, dbg_path);
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Scope Functions

internal DBGI_Scope *
dbgi_scope_open(void)
{
  dbgi_ensure_tctx_inited();
  DBGI_Scope *scope = dbgi_tctx->free_scope;
  if(scope != 0)
  {
    SLLStackPop(dbgi_tctx->free_scope);
    MemoryZeroStruct(scope);
  }
  else
  {
    scope = push_array(dbgi_tctx->arena, DBGI_Scope, 1);
  }
  return scope;
}

internal void
dbgi_scope_close(DBGI_Scope *scope)
{
  for(DBGI_TouchedBinary *tb = scope->first_tb, *next = 0; tb != 0; tb = next)
  {
    next = tb->next;
    ins_atomic_u64_dec_eval(&tb->binary->scope_touch_count);
    SLLStackPush(dbgi_tctx->free_tb, tb);
  }
  for(DBGI_TouchedFuzzySearch *tfs = scope->first_tfs, *next = 0; tfs != 0; tfs = next)
  {
    next = tfs->next;
    ins_atomic_u64_dec_eval(&tfs->node->scope_touch_count);
    SLLStackPush(dbgi_tctx->free_tfs, tfs);
  }
  SLLStackPush(dbgi_tctx->free_scope, scope);
}

internal void
dbgi_scope_touch_binary__stripe_mutex_r_guarded(DBGI_Scope *scope, DBGI_Binary *binary)
{
  DBGI_TouchedBinary *tb = dbgi_tctx->free_tb;
  ins_atomic_u64_inc_eval(&binary->scope_touch_count);
  if(tb != 0)
  {
    SLLStackPop(dbgi_tctx->free_tb);
  }
  else
  {
    tb = push_array(dbgi_tctx->arena, DBGI_TouchedBinary, 1);
  }
  tb->binary = binary;
  SLLQueuePush(scope->first_tb, scope->last_tb, tb);
}

internal void
dbgi_scope_touch_fuzzy_search__stripe_mutex_r_guarded(DBGI_Scope *scope, DBGI_FuzzySearchNode *node)
{
  DBGI_TouchedFuzzySearch *tfs = dbgi_tctx->free_tfs;
  ins_atomic_u64_inc_eval(&node->scope_touch_count);
  if(tfs != 0)
  {
    SLLStackPop(dbgi_tctx->free_tfs);
  }
  else
  {
    tfs = push_array(dbgi_tctx->arena, DBGI_TouchedFuzzySearch, 1);
  }
  tfs->node = node;
  SLLQueuePush(scope->first_tfs, scope->last_tfs, tfs);
}

////////////////////////////////
//~ rjf: Binary Cache Functions

internal void
dbgi_binary_open(String8 exe_path)
{
  Temp scratch = scratch_begin(0, 0);
  exe_path = path_normalized_from_string(scratch.arena, exe_path);
  U64 hash = dbgi_hash_from_string(exe_path);
  U64 slot_idx = hash%dbgi_shared->binary_slots_count;
  U64 stripe_idx = slot_idx%dbgi_shared->binary_stripes_count;
  DBGI_BinarySlot *slot = &dbgi_shared->binary_slots[slot_idx];
  DBGI_BinaryStripe *stripe = &dbgi_shared->binary_stripes[stripe_idx];
  B32 is_new = 0;
  OS_MutexScopeW(stripe->rw_mutex)
  {
    DBGI_Binary *binary = 0;
    for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
    {
      if(str8_match(bin->exe_path, exe_path, 0))
      {
        binary = bin;
        break;
      }
    }
    if(binary == 0)
    {
      binary = push_array(stripe->arena, DBGI_Binary, 1);
      SLLQueuePush(slot->first, slot->last, binary);
      binary->exe_path = push_str8_copy(stripe->arena, exe_path);
      binary->gen += 1;
    }
    binary->refcount += 1;
    is_new = (binary->refcount == 1);
  }
  if(is_new)
  {
    dbgi_u2p_enqueue_exe_path(exe_path, 0);
  }
  scratch_end(scratch);
}

internal void
dbgi_binary_close(String8 exe_path)
{
  Temp scratch = scratch_begin(0, 0);
  exe_path = path_normalized_from_string(scratch.arena, exe_path);
  U64 hash = dbgi_hash_from_string(exe_path);
  U64 slot_idx = hash%dbgi_shared->binary_slots_count;
  U64 stripe_idx = slot_idx%dbgi_shared->binary_stripes_count;
  DBGI_BinarySlot *slot = &dbgi_shared->binary_slots[slot_idx];
  DBGI_BinaryStripe *stripe = &dbgi_shared->binary_stripes[stripe_idx];
  OS_MutexScopeW(stripe->rw_mutex)
  {
    DBGI_Binary *binary = 0;
    for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
    {
      if(str8_match(bin->exe_path, exe_path, 0))
      {
        binary = bin;
        break;
      }
    }
    B32 need_deletion = 0;
    if(binary != 0 && binary->refcount>0)
    {
      binary->refcount -= 1;
      need_deletion = (binary->refcount == 0);
    }
    if(need_deletion) for(;;)
    {
      os_rw_mutex_drop_w(stripe->rw_mutex);
      for(U64 start_t = os_now_microseconds();
          os_now_microseconds() <= start_t + 250;);
      os_rw_mutex_take_w(stripe->rw_mutex);
      if(binary->refcount == 0 && ins_atomic_u64_eval(&binary->scope_touch_count) == 0)
      {
        if(binary->parse.arena != 0) { arena_release(binary->parse.arena); }
        if(binary->parse.exe_base != 0) { os_file_map_view_close(binary->exe_file_map, binary->parse.exe_base); }
        if(!os_handle_match(os_handle_zero(), binary->exe_file_map)) { os_file_map_close(binary->exe_file_map); }
        if(!os_handle_match(os_handle_zero(), binary->exe_file)) { os_file_close(binary->exe_file); }
        if(binary->parse.dbg_base != 0) { os_file_map_view_close(binary->dbg_file_map, binary->parse.dbg_base); }
        if(!os_handle_match(os_handle_zero(), binary->dbg_file_map)) { os_file_map_close(binary->dbg_file_map); }
        if(!os_handle_match(os_handle_zero(), binary->dbg_file)) { os_file_close(binary->dbg_file); }
        binary->exe_file_map = binary->exe_file = os_handle_zero();
        binary->dbg_file_map = binary->dbg_file = os_handle_zero();
        MemoryZeroStruct(&binary->parse);
        binary->last_time_enqueued_for_parse_us = 0;
        binary->gen = 1;
        break;
      }
    }
  }
  scratch_end(scratch);
}

internal DBGI_Parse *
dbgi_parse_from_exe_path(DBGI_Scope *scope, String8 exe_path, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  exe_path = path_normalized_from_string(scratch.arena, exe_path);
  DBGI_Parse *parse = &dbgi_parse_nil;
  if(exe_path.size != 0)
  {
    U64 hash = dbgi_hash_from_string(exe_path);
    U64 slot_idx = hash%dbgi_shared->binary_slots_count;
    U64 stripe_idx = slot_idx%dbgi_shared->binary_stripes_count;
    DBGI_BinarySlot *slot = &dbgi_shared->binary_slots[slot_idx];
    DBGI_BinaryStripe *stripe = &dbgi_shared->binary_stripes[stripe_idx];
    B32 sent = 0;
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      DBGI_Binary *binary = 0;
      for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
      {
        if(str8_match(bin->exe_path, exe_path, 0))
        {
          binary = bin;
          break;
        }
      }
      if(binary != 0 && !(binary->flags & DBGI_BinaryFlag_ParseInFlight))
      {
        if(binary->parse.gen == binary->gen)
        {
          dbgi_scope_touch_binary__stripe_mutex_r_guarded(scope, binary);
          parse = &binary->parse;
          break;
        }
        else if(!sent &&
                ins_atomic_u64_eval(&binary->last_time_enqueued_for_parse_us) == 0 &&
                dbgi_u2p_enqueue_exe_path(exe_path, endt_us))
        {
          sent = 1;
          ins_atomic_u64_eval_assign(&binary->last_time_enqueued_for_parse_us, os_now_microseconds());
        }
      }
      if(os_now_microseconds() >= endt_us)
      {
        break;
      }
      os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  scratch_end(scratch);
  return parse;
}

////////////////////////////////
//~ rjf: Fuzzy Search Cache Functions

internal DBGI_FuzzySearchItemArray
dbgi_fuzzy_search_items_from_key_exe_query(DBGI_Scope *scope, U128 key, String8 exe_path, String8 query, DBGI_FuzzySearchTarget target, U64 endt_us, B32 *stale_out)
{
  Temp scratch = scratch_begin(0, 0);
  DBGI_FuzzySearchItemArray items = {0};
  exe_path = path_normalized_from_string(scratch.arena, exe_path);
  {
    //- rjf: unpack key
    U64 slot_idx = key.u64[1]%dbgi_shared->fuzzy_search_slots_count;
    U64 stripe_idx = slot_idx%dbgi_shared->fuzzy_search_stripes_count;
    DBGI_FuzzySearchSlot *slot = &dbgi_shared->fuzzy_search_slots[slot_idx];
    DBGI_FuzzySearchStripe *stripe = &dbgi_shared->fuzzy_search_stripes[stripe_idx];
    
    //- rjf: query and/or request
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      // rjf: map key -> node
      DBGI_FuzzySearchNode *node = 0;
      for(DBGI_FuzzySearchNode *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->key, key))
        {
          node = n;
          break;
        }
      }
      
      // rjf: no node? -> allocate
      if(node == 0) OS_MutexScopeRWPromote(stripe->rw_mutex)
      {
        node = push_array(stripe->arena, DBGI_FuzzySearchNode, 1);
        SLLQueuePush(slot->first, slot->last, node);
        node->key = key;
        for(U64 idx = 0; idx < ArrayCount(node->buckets); idx += 1)
        {
          node->buckets[idx].arena = arena_alloc();
        }
      }
      
      // rjf: try to grab last valid results for this key/query; determine if stale
      B32 stale = 1;
      if(str8_match(exe_path, node->buckets[node->gen%ArrayCount(node->buckets)].exe_path, 0) &&
         target == node->buckets[node->gen%ArrayCount(node->buckets)].target &&
         node->gen != 0)
      {
        dbgi_scope_touch_fuzzy_search__stripe_mutex_r_guarded(scope, node);
        items = node->gen_items;
        stale = !str8_match(query, node->buckets[node->gen%ArrayCount(node->buckets)].query, 0);
        *stale_out = stale;
      }
      
      // rjf: if stale -> request again
      if(stale) OS_MutexScopeRWPromote(stripe->rw_mutex)
      {
        if(node->gen <= node->submit_gen && node->submit_gen < node->gen + ArrayCount(node->buckets)-1)
        {
          node->submit_gen += 1;
          arena_clear(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena);
          node->buckets[node->submit_gen%ArrayCount(node->buckets)].exe_path = push_str8_copy(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena, exe_path);
          node->buckets[node->submit_gen%ArrayCount(node->buckets)].query = push_str8_copy(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena, query);
          node->buckets[node->submit_gen%ArrayCount(node->buckets)].target = target;
        }
        if((node->submit_gen > node->gen+1 || os_now_microseconds() >= node->last_time_submitted_us+100000) &&
           dbgi_u2f_enqueue_req(key, endt_us))
        {
          node->last_time_submitted_us = os_now_microseconds();
        }
      }
      
      // rjf: not stale, or timeout -> break
      if(!stale || os_now_microseconds() >= endt_us)
      {
        break;
      }
      
      // rjf: no results, but have time to wait -> wait
      os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  scratch_end(scratch);
  return items;
}

////////////////////////////////
//~ rjf: Analysis Threads

internal B32
dbgi_u2p_enqueue_exe_path(String8 exe_path, U64 endt_us)
{
  B32 sent = 0;
  OS_MutexScope(dbgi_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dbgi_shared->u2p_ring_write_pos-dbgi_shared->u2p_ring_read_pos);
    U64 available_size = dbgi_shared->u2p_ring_size-unconsumed_size;
    U64 needed_size = sizeof(U64)+exe_path.size;
    needed_size += 7;
    needed_size -= needed_size%8;
    if(available_size >= needed_size)
    {
      dbgi_shared->u2p_ring_write_pos += ring_write_struct(dbgi_shared->u2p_ring_base, dbgi_shared->u2p_ring_size, dbgi_shared->u2p_ring_write_pos, &exe_path.size);
      dbgi_shared->u2p_ring_write_pos += ring_write(dbgi_shared->u2p_ring_base, dbgi_shared->u2p_ring_size, dbgi_shared->u2p_ring_write_pos, exe_path.str, exe_path.size);
      dbgi_shared->u2p_ring_write_pos += 7;
      dbgi_shared->u2p_ring_write_pos -= dbgi_shared->u2p_ring_write_pos%8;
      sent = 1;
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(dbgi_shared->u2p_ring_cv, dbgi_shared->u2p_ring_mutex, endt_us);
  }
  os_condition_variable_broadcast(dbgi_shared->u2p_ring_cv);
  return sent;
}

internal String8
dbgi_u2p_dequeue_exe_path(Arena *arena)
{
  String8 result = {0};
  OS_MutexScope(dbgi_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dbgi_shared->u2p_ring_write_pos-dbgi_shared->u2p_ring_read_pos);
    if(unconsumed_size != 0)
    {
      dbgi_shared->u2p_ring_read_pos += ring_read_struct(dbgi_shared->u2p_ring_base, dbgi_shared->u2p_ring_size, dbgi_shared->u2p_ring_read_pos, &result.size);
      result.str = push_array_no_zero(arena, U8, result.size);
      dbgi_shared->u2p_ring_read_pos += ring_read(dbgi_shared->u2p_ring_base, dbgi_shared->u2p_ring_size, dbgi_shared->u2p_ring_read_pos, result.str, result.size);
      dbgi_shared->u2p_ring_read_pos += 7;
      dbgi_shared->u2p_ring_read_pos -= dbgi_shared->u2p_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(dbgi_shared->u2p_ring_cv, dbgi_shared->u2p_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(dbgi_shared->u2p_ring_cv);
  return result;
}

internal void
dbgi_p2u_push_event(DBGI_Event *event)
{
  OS_MutexScope(dbgi_shared->p2u_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dbgi_shared->p2u_ring_write_pos-dbgi_shared->p2u_ring_read_pos);
    U64 available_size = dbgi_shared->p2u_ring_size-unconsumed_size;
    U64 needed_size = sizeof(DBGI_EventKind) + sizeof(U64) + event->string.size;
    if(available_size >= needed_size)
    {
      dbgi_shared->p2u_ring_write_pos += ring_write_struct(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_write_pos, &event->kind);
      dbgi_shared->p2u_ring_write_pos += ring_write_struct(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_write_pos, &event->string.size);
      dbgi_shared->p2u_ring_write_pos += ring_write(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_write_pos, event->string.str, event->string.size);
      dbgi_shared->p2u_ring_write_pos += 7;
      dbgi_shared->p2u_ring_write_pos -= dbgi_shared->p2u_ring_write_pos%8;
      break;
    }
    os_condition_variable_wait(dbgi_shared->p2u_ring_cv, dbgi_shared->p2u_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(dbgi_shared->p2u_ring_cv);
}

internal DBGI_EventList
dbgi_p2u_pop_events(Arena *arena, U64 endt_us)
{
  DBGI_EventList events = {0};
  OS_MutexScope(dbgi_shared->p2u_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dbgi_shared->p2u_ring_write_pos-dbgi_shared->p2u_ring_read_pos);
    if(unconsumed_size >= sizeof(DBGI_EventKind) + sizeof(U64))
    {
      DBGI_EventNode *n = push_array(arena, DBGI_EventNode, 1);
      SLLQueuePush(events.first, events.last, n);
      events.count += 1;
      dbgi_shared->p2u_ring_read_pos += ring_read_struct(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_read_pos, &n->v.kind);
      dbgi_shared->p2u_ring_read_pos += ring_read_struct(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_read_pos, &n->v.string.size);
      n->v.string.str = push_array_no_zero(arena, U8, n->v.string.size);
      dbgi_shared->p2u_ring_read_pos += ring_read(dbgi_shared->p2u_ring_base, dbgi_shared->p2u_ring_size, dbgi_shared->p2u_ring_read_pos, n->v.string.str, n->v.string.size);
      dbgi_shared->p2u_ring_read_pos += 7;
      dbgi_shared->p2u_ring_read_pos -= dbgi_shared->p2u_ring_read_pos%8;
    }
    else if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(dbgi_shared->p2u_ring_cv, dbgi_shared->p2u_ring_mutex, endt_us);
  }
  os_condition_variable_broadcast(dbgi_shared->p2u_ring_cv);
  return events;
}

internal void
dbgi_parse_thread_entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  ProfThreadName("[dbgi] parse #%I64U", (U64)p);
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: grab next path & unpack
    String8 exe_path = dbgi_u2p_dequeue_exe_path(scratch.arena);
    ProfBegin("begin task for \"%.*s\"", str8_varg(exe_path));
    U64 hash = dbgi_hash_from_string(exe_path);
    U64 slot_idx = hash%dbgi_shared->binary_slots_count;
    U64 stripe_idx = slot_idx%dbgi_shared->binary_stripes_count;
    DBGI_BinarySlot *slot = &dbgi_shared->binary_slots[slot_idx];
    DBGI_BinaryStripe *stripe = &dbgi_shared->binary_stripes[stripe_idx];
    
    //- rjf: determine if binary's analysis work is taken by another thread.
    // if not, take it
    B32 task_is_taken_by_other_thread = 0;
    OS_MutexScopeW(stripe->rw_mutex)
    {
      DBGI_Binary *binary = 0;
      for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
      {
        if(str8_match(bin->exe_path, exe_path, 0))
        {
          binary = bin;
          break;
        }
      }
      if(binary == 0 || binary->flags&DBGI_BinaryFlag_ParseInFlight || binary->refcount == 0)
      {
        task_is_taken_by_other_thread = 1;
      }
      else if(binary != 0)
      {
        binary->flags |= DBGI_BinaryFlag_ParseInFlight;
      }
    }
    
    //- rjf: is the work taken? -> abort
    B32 do_task = 1;
    if(task_is_taken_by_other_thread)
    {
      do_task = 0;
    }
    
    //- rjf: open exe file & map into address space
    OS_Handle exe_file = {0};
    FileProperties exe_file_props = {0};
    OS_Handle exe_file_map = {0};
    void *exe_file_base = 0;
    if(do_task)
    {
      exe_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, exe_path);
      exe_file_props = os_properties_from_file(exe_file);
      exe_file_map = os_file_map_open(OS_AccessFlag_Read, exe_file);
      exe_file_base = os_file_map_view_open(exe_file_map, OS_AccessFlag_Read, r1u64(0, exe_file_props.size));
    }
    
    //- rjf: parse exe file info
    Arena *parse_arena = 0;
    PE_BinInfo exe_pe_info = {0};
    String8 exe_dbg_path_embedded_absolute = {0};
    String8 exe_dbg_path_embedded_relative = {0};
    if(do_task)
    {
      parse_arena = arena_alloc();
      if(exe_file_props.size >= 2 && *(U16 *)exe_file_base == PE_DOS_MAGIC)
      {
        String8 exe_data = str8((U8 *)exe_file_base, exe_file_props.size);
        exe_pe_info = pe_bin_info_from_data(parse_arena, exe_data);
        exe_dbg_path_embedded_absolute = str8_cstring_capped((char *)exe_data.str+exe_pe_info.dbg_path_off, (char *)exe_data.str+exe_pe_info.dbg_path_off+Min(exe_data.size-exe_pe_info.dbg_path_off, 4096));
        String8 exe_folder = str8_chop_last_slash(exe_path);
        exe_dbg_path_embedded_relative = push_str8f(scratch.arena, "%S/%S", exe_folder, exe_dbg_path_embedded_absolute);
      }
    }
    
    //- rjf: determine O.G. (may or may not be RADDBG) dbg path
    String8 og_dbg_path = {0};
    if(do_task) ProfScope("determine O.G. dbg path")
    {
      String8 forced_og_dbg_path = dbgi_forced_dbg_path_from_exe_path(scratch.arena, exe_path);
      if(forced_og_dbg_path.size != 0)
      {
        og_dbg_path = forced_og_dbg_path;
      }
      else
      {
        String8 possible_og_dbg_paths[] =
        {
          /* inferred (treated as absolute): */ exe_dbg_path_embedded_absolute,
          /* inferred (treated as relative): */ exe_dbg_path_embedded_relative,
          /* "foo.exe" -> "foo.pdb"          */ push_str8f(scratch.arena, "%S.pdb", str8_chop_last_dot(exe_path)),
          /* "foo.exe" -> "foo.exe.pdb"      */ push_str8f(scratch.arena, "%S.pdb", exe_path),
        };
        for(U64 idx = 0; idx < ArrayCount(possible_og_dbg_paths); idx += 1)
        {
          FileProperties props = os_properties_from_file_path(possible_og_dbg_paths[idx]);
          if(props.modified != 0 && props.size != 0)
          {
            og_dbg_path = possible_og_dbg_paths[idx];
            break;
          }
        }
      }
    }
    
    //- rjf: analyze O.G. dbg file
    B32 og_dbg_format_is_known = 0;
    B32 og_dbg_is_pe     = 0;
    B32 og_dbg_is_pdb    = 0;
    B32 og_dbg_is_elf    = 0;
    B32 og_dbg_is_raddbg = 0;
    FileProperties og_dbg_props = {0};
    if(do_task) ProfScope("analyze O.G. dbg file")
    {
      OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, og_dbg_path);
      OS_Handle file_map = os_file_map_open(OS_AccessFlag_Read, file);
      FileProperties props = og_dbg_props = os_properties_from_file(file);
      void *base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, props.size));
      String8 data = str8((U8 *)base, props.size);
      if(!og_dbg_format_is_known)
      {
        String8 msf20_magic = str8_lit("Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0");
        String8 msf70_magic = str8_lit("Microsoft C/C++ MSF 7.00\r\n\032DS\0\0");
        String8 msfxx_magic = str8_lit("Microsoft C/C++");
        if((data.size >= msf20_magic.size && str8_match(data, msf20_magic, StringMatchFlag_RightSideSloppy)) ||
           (data.size >= msf70_magic.size && str8_match(data, msf70_magic, StringMatchFlag_RightSideSloppy)) ||
           (data.size >= msfxx_magic.size && str8_match(data, msfxx_magic, StringMatchFlag_RightSideSloppy)))
        {
          og_dbg_format_is_known = 1;
          og_dbg_is_pdb = 1;
        }
      }
      if(!og_dbg_format_is_known)
      {
        if(data.size >= 8 && *(U64 *)data.str == RDI_MAGIC_CONSTANT)
        {
          og_dbg_format_is_known = 1;
          og_dbg_is_raddbg = 1;
        }
      }
      if(!og_dbg_format_is_known)
      {
        if(data.size >= 4 &&
           data.str[0] == 0x7f &&
           data.str[1] == 'E' &&
           data.str[2] == 'L' &&
           data.str[3] == 'F')
        {
          og_dbg_format_is_known = 1;
          og_dbg_is_elf = 1;
        }
      }
      if(!og_dbg_format_is_known)
      {
        if(data.size >= 2 && *(U16 *)data.str == PE_DOS_MAGIC)
        {
          og_dbg_format_is_known = 1;
          og_dbg_is_pe = 1;
        }
      }
      os_file_map_view_close(file_map, base);
      os_file_map_close(file_map);
      os_file_close(file);
    }
    
    //- rjf: given O.G. path & analysis, determine RADDBG file path
    String8 raddbgi_path = {0};
    if(do_task)
    {
      if(og_dbg_is_raddbg)
      {
        raddbgi_path = og_dbg_path;
      }
      else if(og_dbg_format_is_known && og_dbg_is_pdb)
      {
        raddbgi_path = push_str8f(scratch.arena, "%S.raddbgi", str8_chop_last_dot(og_dbg_path));
      }
    }
    
    //- rjf: check if raddbg file is up-to-date
    B32 raddbgi_file_is_up_to_date = 0;
    if(do_task)
    {
      if(raddbgi_path.size != 0)
      {
        FileProperties props = os_properties_from_file_path(raddbgi_path);
        raddbgi_file_is_up_to_date = (props.modified > og_dbg_props.modified);
      }
    }
    
    //- rjf: if raddbg file is up to date based on timestamp, check the
    // encoding generation number, to see if we need to regenerate it
    // regardless
    if(do_task && raddbgi_file_is_up_to_date)
    {
      OS_Handle file = {0};
      OS_Handle file_map = {0};
      FileProperties file_props = {0};
      void *file_base = 0;
      file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, raddbgi_path);
      file_map = os_file_map_open(OS_AccessFlag_Read, file);
      file_props = os_properties_from_file(file);
      file_base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, file_props.size));
      if(sizeof(RDI_Header) <= file_props.size)
      {
        RDI_Header *header = (RDI_Header*)file_base;
        if(header->encoding_version != RDI_ENCODING_VERSION)
        {
          raddbgi_file_is_up_to_date = 0;
        }
      }
      else
      {
        raddbgi_file_is_up_to_date = 0;
      }
      os_file_map_view_close(file_map, file_base);
      os_file_map_close(file_map);
      os_file_close(file);
    }
    
    //- rjf: raddbg file not up-to-date? we need to generate it
    if(do_task)
    {
      if(!raddbgi_file_is_up_to_date) ProfScope("generate raddbg file")
      {
        if(og_dbg_is_pdb)
        {
          // rjf: push conversion task begin event
          {
            DBGI_Event event = {DBGI_EventKind_ConversionStarted};
            event.string = raddbgi_path;
            dbgi_p2u_push_event(&event);
          }
          
          // rjf: kick off process
          OS_Handle process = {0};
          {
            OS_LaunchOptions opts = {0};
            opts.path = os_string_from_system_path(scratch.arena, OS_SystemPath_Binary);
            opts.inherit_env = 1;
            opts.consoleless = 1;
            str8_list_pushf(scratch.arena, &opts.cmd_line, "raddbg");
            str8_list_pushf(scratch.arena, &opts.cmd_line, "--convert");
            str8_list_pushf(scratch.arena, &opts.cmd_line, "--quiet");
            //str8_list_pushf(scratch.arena, &opts.cmd_line, "--capture");
            str8_list_pushf(scratch.arena, &opts.cmd_line, "--exe:%S", exe_path);
            str8_list_pushf(scratch.arena, &opts.cmd_line, "--pdb:%S", og_dbg_path);
            str8_list_pushf(scratch.arena, &opts.cmd_line, "--out:%S", raddbgi_path);
            os_launch_process(&opts, &process);
          }
          
          // rjf: wait for process to complete
          {
            U64 start_wait_t = os_now_microseconds();
            for(;;)
            {
              B32 wait_done = os_process_wait(process, os_now_microseconds()+1000);
              if(wait_done)
              {
                raddbgi_file_is_up_to_date = 1;
                break;
              }
              if(os_now_microseconds()-start_wait_t > 10000000 && og_dbg_props.size < MB(64))
              {
                // os_graphical_message(1, str8_lit("RADDBG INTERNAL DEVELOPMENT MESSAGE"), str8_lit("this is taking a while... indicative of something that seemed like a bug that Jeff hit before. attach with debugger now & see where the callstack is?"));
              }
            }
          }
          
          // rjf: push conversion task end event
          {
            DBGI_Event event = {DBGI_EventKind_ConversionEnded};
            event.string = raddbgi_path;
            dbgi_p2u_push_event(&event);
          }
        }
        else
        {
          // NOTE(rjf): we cannot convert from this O.G. debug info format right now.
          // rjf: push conversion task failure event
          {
            DBGI_Event event = {DBGI_EventKind_ConversionFailureUnsupportedFormat};
            event.string = raddbgi_path;
            dbgi_p2u_push_event(&event);
          }
        }
      }
    }
    
    //- rjf: open raddbg file & gather info
    OS_Handle raddbgi_file = {0};
    OS_Handle raddbgi_file_map = {0};
    FileProperties raddbgi_file_props = {0};
    void *raddbgi_file_base = 0;
    if(do_task && raddbgi_file_is_up_to_date)
    {
      raddbgi_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, raddbgi_path);
      raddbgi_file_map = os_file_map_open(OS_AccessFlag_Read, raddbgi_file);
      raddbgi_file_props = os_properties_from_file(raddbgi_file);
      raddbgi_file_base = os_file_map_view_open(raddbgi_file_map, OS_AccessFlag_Read, r1u64(0, raddbgi_file_props.size));
    }
    
    //- rjf: cache write, step 0: busy-loop-wait for all scope touches to be done
    if(do_task) ProfScope("cache write, step 0: busy-loop-wait for all scope touches to be done")
    {
      for(B32 done = 0; done == 0;)
      {
        OS_MutexScopeR(stripe->rw_mutex) for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
        {
          if(str8_match(bin->exe_path, exe_path, 0) &&
             bin->scope_touch_count == 0)
          {
            done = 1;
            break;
          }
        }
      }
    }
    
    //- rjf: cache write, step 1: check if refcount is still nonzero, &
    // either EXE or raddbg file is new. if so, clear all old results &
    // store new top-level info
    B32 binary_refcount_is_zero = 0;
    B32 raddbgi_or_exe_file_is_updated = 0;
    if(do_task) ProfScope("cache write, step 1: check if raddbg is new & clear")
    {
      OS_MutexScopeW(stripe->rw_mutex) for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
      {
        if(str8_match(bin->exe_path, exe_path, 0))
        {
          if(bin->refcount == 0)
          {
            binary_refcount_is_zero = 1;
            break;
          }
          if(bin->parse.dbg_props.modified != raddbgi_file_props.modified ||
             bin->parse.exe_props.modified != exe_file_props.modified)
          {
            raddbgi_or_exe_file_is_updated = 1;
            
            // rjf: clean up old stuff
            if(bin->parse.arena != 0) { arena_release(bin->parse.arena); }
            if(bin->parse.exe_base != 0) {os_file_map_view_close(bin->exe_file_map, bin->parse.exe_base);}
            if(!os_handle_match(os_handle_zero(), bin->exe_file_map)) {os_file_map_close(bin->exe_file_map);}
            if(!os_handle_match(os_handle_zero(), bin->exe_file)) {os_file_close(bin->exe_file);}
            if(bin->parse.dbg_base != 0) {os_file_map_view_close(bin->dbg_file_map, bin->parse.dbg_base);}
            if(!os_handle_match(os_handle_zero(), bin->dbg_file_map)) {os_file_map_close(bin->dbg_file_map);}
            if(!os_handle_match(os_handle_zero(), bin->dbg_file)) {os_file_close(bin->dbg_file);}
            MemoryZeroStruct(&bin->parse);
            bin->last_time_enqueued_for_parse_us = 0;
            
            // rjf: store new handles & props
            bin->exe_file = exe_file;
            bin->exe_file_map = exe_file_map;
            bin->parse.exe_base = exe_file_base;
            bin->parse.exe_props = exe_file_props;
            bin->dbg_file = raddbgi_file;
            bin->dbg_file_map = raddbgi_file_map;
            bin->parse.dbg_base = raddbgi_file_base;
            bin->parse.dbg_props = raddbgi_file_props;
            bin->gen += 1;
          }
          break;
        }
      }
    }
    
    //- rjf: raddbg file or exe is not new? cache can stay unmodified, close
    // handles & skip to end.
    if(do_task) if((!raddbgi_or_exe_file_is_updated && raddbgi_file_is_up_to_date) || binary_refcount_is_zero)
    {
      os_file_map_view_close(raddbgi_file_map, raddbgi_file_base);
      os_file_map_close(raddbgi_file_map);
      os_file_close(raddbgi_file);
      os_file_map_view_close(exe_file_map, exe_file_base);
      os_file_map_close(exe_file_map);
      os_file_close(exe_file);
      arena_release(parse_arena);
      do_task = 0;
    }
    
    //- rjf: parse raddbg info
    RDI_Parsed rdi_parsed = dbgi_parse_nil.rdbg;
    U64 arch_addr_size = 8;
    if(do_task)
    {
      RDI_ParseStatus parse_status = rdi_parse((U8 *)raddbgi_file_base, raddbgi_file_props.size, &rdi_parsed);
      if(rdi_parsed.top_level_info != 0)
      {
        arch_addr_size = rdi_addr_size_from_arch(rdi_parsed.top_level_info->architecture);
      }
    }
    
    //- rjf: cache write, step 2: store parse artifacts
    B32 parse_store_good = 0;
    if(do_task) ProfScope("cache write, step 2: store parse")
    {
      OS_MutexScopeW(stripe->rw_mutex) for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
      {
        if(str8_match(bin->exe_path, exe_path, 0))
        {
          String8 dbg_path = og_dbg_path;
          if(dbg_path.size == 0)
          {
            dbg_path = exe_dbg_path_embedded_absolute;
          }
          if(dbg_path.size == 0)
          {
            dbg_path = exe_dbg_path_embedded_relative;
          }
          if(dbg_path.size == 0)
          {
            dbg_path = push_str8f(scratch.arena, "%S.pdb", str8_chop_last_dot(exe_path));
          }
          parse_store_good = 1;
          bin->parse.arena = parse_arena;
          bin->parse.dbg_path = push_str8_copy(parse_arena, dbg_path);
          MemoryCopyStruct(&bin->parse.pe, &exe_pe_info);
          MemoryCopyStruct(&bin->parse.rdbg, &rdi_parsed);
          bin->parse.gen = bin->gen;
          break;
        }
      }
    }
    
    //- rjf: bad parse store? abort
    if(do_task && !parse_store_good)
    {
      arena_release(parse_arena);
    }
    
    //- rjf: cache write, step 3: mark binary work as complete
    if(!task_is_taken_by_other_thread) ProfScope("cache write, step 4: mark binary work as complete")
    {
      OS_MutexScopeW(stripe->rw_mutex) for(DBGI_Binary *bin = slot->first; bin != 0; bin = bin->next)
      {
        if(str8_match(bin->exe_path, exe_path, 0))
        {
          bin->flags &= ~DBGI_BinaryFlag_ParseInFlight;
          break;
        }
      }
      os_condition_variable_broadcast(stripe->cv);
    }
    
    ProfEnd();
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Fuzzy Searching Threads

internal B32
dbgi_u2f_enqueue_req(U128 key, U64 endt_us)
{
  B32 sent = 0;
  DBGI_FuzzySearchThread *thread = &dbgi_shared->fuzzy_threads[key.u64[1]%dbgi_shared->fuzzy_thread_count];
  OS_MutexScope(thread->u2f_ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->u2f_ring_write_pos - thread->u2f_ring_read_pos;
    U64 available_size = thread->u2f_ring_size - unconsumed_size;
    if(available_size >= sizeof(U128))
    {
      sent = 1;
      thread->u2f_ring_write_pos += ring_write_struct(thread->u2f_ring_base, thread->u2f_ring_size, thread->u2f_ring_write_pos, &key);
      break;
    }
    os_condition_variable_wait(thread->u2f_ring_cv, thread->u2f_ring_mutex, endt_us);
  }
  if(sent)
  {
    os_condition_variable_broadcast(thread->u2f_ring_cv);
  }
  return sent;
}

internal void
dbgi_u2f_dequeue_req(Arena *arena, DBGI_FuzzySearchThread *thread, U128 *key_out)
{
  OS_MutexScope(thread->u2f_ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->u2f_ring_write_pos - thread->u2f_ring_read_pos;
    if(unconsumed_size >= sizeof(U128))
    {
      thread->u2f_ring_read_pos += ring_read_struct(thread->u2f_ring_base, thread->u2f_ring_size, thread->u2f_ring_read_pos, key_out);
      break;
    }
    os_condition_variable_wait(thread->u2f_ring_cv, thread->u2f_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(thread->u2f_ring_cv);
}

internal int
dbgi_qsort_compare_fuzzy_search_items(DBGI_FuzzySearchItem *a, DBGI_FuzzySearchItem *b)
{
  int result = 0;
  if(a->match_ranges.count > b->match_ranges.count)
  {
    result = -1;
  }
  else if(a->match_ranges.count < b->match_ranges.count)
  {
    result = +1;
  }
  else if(a->missed_size < b->missed_size)
  {
    result = -1;
  }
  else if(a->missed_size > b->missed_size)
  {
    result = +1;
  }
  return result;
}

internal void
dbgi_fuzzy_thread__entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  ProfThreadName("[dbgi] fuzzy search #%I64U", (U64)p);
  DBGI_FuzzySearchThread *thread = &dbgi_shared->fuzzy_threads[(U64)p];
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    DBGI_Scope *scope = dbgi_scope_open();
    
    //- rjf: dequeue next request
    U128 key = {0};
    dbgi_u2f_dequeue_req(scratch.arena, thread, &key);
    U64 slot_idx = key.u64[1]%dbgi_shared->fuzzy_search_slots_count;
    U64 stripe_idx = slot_idx%dbgi_shared->fuzzy_search_stripes_count;
    DBGI_FuzzySearchSlot *slot = &dbgi_shared->fuzzy_search_slots[slot_idx];
    DBGI_FuzzySearchStripe *stripe = &dbgi_shared->fuzzy_search_stripes[stripe_idx];
    
    //- rjf: grab next exe_path/query for this key
    B32 task_is_good = 0;
    Arena *task_arena = 0;
    String8 exe_path = {0};
    String8 query = {0};
    DBGI_FuzzySearchTarget target = DBGI_FuzzySearchTarget_Procedures;
    U64 initial_submit_gen = 0;
    OS_MutexScopeW(stripe->rw_mutex)
    {
      for(DBGI_FuzzySearchNode *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->key, key))
        {
          task_is_good = 1;
          initial_submit_gen = n->submit_gen;
          task_arena = n->buckets[n->submit_gen%ArrayCount(n->buckets)].arena;
          exe_path = n->buckets[n->submit_gen%ArrayCount(n->buckets)].exe_path;
          query = n->buckets[n->submit_gen%ArrayCount(n->buckets)].query;
          target = n->buckets[n->submit_gen%ArrayCount(n->buckets)].target;
          break;
        }
      }
    }
    
    //- rjf: exe_path -> dbgi_parse, raddbg
    DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, max_U64);
    RDI_Parsed *rdbg = &dbgi->rdbg;
    
    //- rjf: rdbg * query -> item list
    U64 table_ptr_off = 0;
    U64 element_name_idx_off = 0;
    U64 element_count = 0;
    U64 element_size = 0;
    switch(target)
    {
      // NOTE(rjf): no default!
      case DBGI_FuzzySearchTarget_COUNT:{}break;
      case DBGI_FuzzySearchTarget_Procedures:
      {
        table_ptr_off = OffsetOf(RDI_Parsed, procedures);
        element_name_idx_off = OffsetOf(RDI_Procedure, name_string_idx);
        element_count = rdbg->procedures_count;
        element_size = sizeof(RDI_Procedure);
      }break;
      case DBGI_FuzzySearchTarget_GlobalVariables:
      {
        table_ptr_off = OffsetOf(RDI_Parsed, global_variables);
        element_name_idx_off = OffsetOf(RDI_GlobalVariable, name_string_idx);
        element_count = rdbg->global_variables_count;
        element_size = sizeof(RDI_GlobalVariable);
      }break;
      case DBGI_FuzzySearchTarget_ThreadVariables:
      {
        table_ptr_off = OffsetOf(RDI_Parsed, thread_variables);
        element_name_idx_off = OffsetOf(RDI_ThreadVariable, name_string_idx);
        element_count = rdbg->thread_variables_count;
        element_size = sizeof(RDI_ThreadVariable);
      }break;
      case DBGI_FuzzySearchTarget_UDTs:
      {
        table_ptr_off = OffsetOf(RDI_Parsed, udts);
        element_count = rdbg->udts_count;
        element_size = sizeof(RDI_UDT);
      }break;
    }
    DBGI_FuzzySearchItemChunkList items_list = {0};
    if(task_is_good)
    {
      void *table_base = (U8*)rdbg + table_ptr_off;
      for(U64 idx = 1; task_is_good && idx < element_count; idx += 1)
      {
        void *element = (U8 *)(*(void **)table_base) + element_size*idx;
        U32 *name_idx_ptr = (U32 *)((U8 *)element + element_name_idx_off);
        if(target == DBGI_FuzzySearchTarget_UDTs)
        {
          RDI_UDT *udt = (RDI_UDT *)element;
          RDI_TypeNode *type_node = rdi_element_from_idx(rdbg, type_nodes, udt->self_type_idx);
          name_idx_ptr = &type_node->user_defined.name_string_idx;
        }
        U32 name_idx = *name_idx_ptr;
        U64 name_size = 0;
        U8 *name_base = rdi_string_from_idx(rdbg, name_idx, &name_size);
        String8 name = str8(name_base, name_size);
        if(name.size == 0) { continue; }
        FuzzyMatchRangeList matches = fuzzy_match_find(task_arena, query, name);
        if(matches.count == matches.needle_part_count)
        {
          DBGI_FuzzySearchItemChunk *chunk = items_list.last;
          if(chunk == 0 || chunk->count >= chunk->cap)
          {
            chunk = push_array(scratch.arena, DBGI_FuzzySearchItemChunk, 1);
            chunk->cap = 512;
            chunk->count = 0;
            chunk->v = push_array_no_zero(scratch.arena, DBGI_FuzzySearchItem, chunk->cap);
            SLLQueuePush(items_list.first, items_list.last, chunk);
            items_list.chunk_count += 1;
          }
          chunk->v[chunk->count].idx = idx;
          chunk->v[chunk->count].match_ranges = matches;
          chunk->v[chunk->count].missed_size = (name_size > matches.total_dim) ? (name_size-matches.total_dim) : 0;
          chunk->count += 1;
          items_list.total_count += 1;
        }
        if(idx%100 == 99) OS_MutexScopeR(stripe->rw_mutex)
        {
          for(DBGI_FuzzySearchNode *n = slot->first; n != 0; n = n->next)
          {
            if(u128_match(n->key, key) && n->submit_gen > initial_submit_gen)
            {
              task_is_good = 0;
              break;
            }
          }
        }
      }
    }
    
    //- rjf: item list -> item array
    DBGI_FuzzySearchItemArray items = {0};
    if(task_is_good)
    {
      items.count = items_list.total_count;
      items.v = push_array_no_zero(task_arena, DBGI_FuzzySearchItem, items.count);
      U64 idx = 0;
      for(DBGI_FuzzySearchItemChunk *chunk = items_list.first; chunk != 0; chunk = chunk->next)
      {
        MemoryCopy(items.v+idx, chunk->v, sizeof(DBGI_FuzzySearchItem)*chunk->count);
        idx += chunk->count;
      }
    }
    
    //- rjf: sort item array
    if(items.count != 0 && query.size != 0)
    {
      qsort(items.v, items.count, sizeof(DBGI_FuzzySearchItem), (int (*)(const void *, const void *))dbgi_qsort_compare_fuzzy_search_items);
    }
    
    //- rjf: commit to cache - busyloop on scope touches
    if(task_is_good)
    {
      for(B32 done = 0; !done;)
      {
        B32 found = 0;
        OS_MutexScopeW(stripe->rw_mutex) for(DBGI_FuzzySearchNode *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key))
          {
            if(n->scope_touch_count == 0)
            {
              n->gen = initial_submit_gen;
              n->gen_items = items;
              done = 1;
            }
            found = 1;
            break;
          }
        }
        if(!found)
        {
          break;
        }
      }
    }
    
    dbgi_scope_close(scope);
    scratch_end(scratch);
  }
}
