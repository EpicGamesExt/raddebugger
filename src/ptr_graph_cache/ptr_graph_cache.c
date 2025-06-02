// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
ptg_init(void)
{
  Arena *arena = arena_alloc();
  ptg_shared = push_array(arena, PTG_Shared, 1);
  ptg_shared->arena = arena;
  ptg_shared->slots_count = 1024;
  ptg_shared->stripes_count = Min(ptg_shared->slots_count, os_get_system_info()->logical_processor_count);
  ptg_shared->slots = push_array(arena, PTG_GraphSlot, ptg_shared->slots_count);
  ptg_shared->stripes = push_array(arena, PTG_GraphStripe, ptg_shared->stripes_count);
  for(U64 idx = 0; idx < ptg_shared->stripes_count; idx += 1)
  {
    ptg_shared->stripes[idx].arena = arena_alloc();
    ptg_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    ptg_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  ptg_shared->u2b_ring_size = KB(64);
  ptg_shared->u2b_ring_base = push_array_no_zero(arena, U8, ptg_shared->u2b_ring_size);
  ptg_shared->u2b_ring_cv = os_condition_variable_alloc();
  ptg_shared->u2b_ring_mutex = os_mutex_alloc();
  ptg_shared->builder_thread_count = Clamp(1, os_get_system_info()->logical_processor_count-1, 4);
  ptg_shared->builder_threads = push_array(arena, OS_Handle, ptg_shared->builder_thread_count);
  for(U64 idx = 0; idx < ptg_shared->builder_thread_count; idx += 1)
  {
    ptg_shared->builder_threads[idx] = os_thread_launch(ptg_builder_thread__entry_point, (void *)idx, 0);
  }
  ptg_shared->evictor_thread = os_thread_launch(ptg_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: User Clock

internal void
ptg_user_clock_tick(void)
{
  ins_atomic_u64_inc_eval(&ptg_shared->user_clock_idx);
}

internal U64
ptg_user_clock_idx(void)
{
  return ins_atomic_u64_eval(&ptg_shared->user_clock_idx);
}

////////////////////////////////
//~ rjf: Scoped Access

internal PTG_Scope *
ptg_scope_open(void)
{
  if(ptg_tctx == 0)
  {
    Arena *arena = arena_alloc();
    ptg_tctx = push_array(arena, PTG_TCTX, 1);
    ptg_tctx->arena = arena;
  }
  PTG_Scope *scope = ptg_tctx->free_scope;
  if(scope)
  {
    SLLStackPop(ptg_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(ptg_tctx->arena, PTG_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
ptg_scope_close(PTG_Scope *scope)
{
  for(PTG_Touch *touch = scope->top_touch, *next = 0; touch != 0; touch = next)
  {
    next = touch->next;
    ins_atomic_u64_dec_eval(&touch->node->scope_ref_count);
    SLLStackPush(ptg_tctx->free_touch, touch);
  }
  SLLStackPush(ptg_tctx->free_scope, scope);
}

internal void
ptg_scope_touch_node__stripe_r_guarded(PTG_Scope *scope, PTG_GraphNode *node)
{
  PTG_Touch *touch = ptg_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, ptg_user_clock_idx());
  if(touch != 0)
  {
    SLLStackPop(ptg_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(ptg_tctx->arena, PTG_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->node = node;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal PTG_Graph *
ptg_graph_from_key(PTG_Scope *scope, PTG_Key *key)
{
  PTG_Graph *g = 0;
  return g;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
ptg_u2b_enqueue_req(PTG_Key *key, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(ptg_shared->u2b_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ptg_shared->u2b_ring_write_pos-ptg_shared->u2b_ring_read_pos;
    U64 available_size = ptg_shared->u2b_ring_size-unconsumed_size;
    if(available_size >= sizeof(key))
    {
      good = 1;
      ptg_shared->u2b_ring_write_pos += ring_write_struct(ptg_shared->u2b_ring_base, ptg_shared->u2b_ring_size, ptg_shared->u2b_ring_write_pos, &key);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(ptg_shared->u2b_ring_cv, ptg_shared->u2b_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(ptg_shared->u2b_ring_cv);
  }
  return good;
}

internal void
ptg_u2b_dequeue_req(PTG_Key *key_out)
{
  OS_MutexScope(ptg_shared->u2b_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ptg_shared->u2b_ring_write_pos-ptg_shared->u2b_ring_read_pos;
    if(unconsumed_size >= sizeof(*key_out))
    {
      ptg_shared->u2b_ring_read_pos += ring_read_struct(ptg_shared->u2b_ring_base, ptg_shared->u2b_ring_size, ptg_shared->u2b_ring_read_pos, key_out);
      break;
    }
    os_condition_variable_wait(ptg_shared->u2b_ring_cv, ptg_shared->u2b_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ptg_shared->u2b_ring_cv);
}

internal void
ptg_builder_thread__entry_point(void *p)
{
  for(;;)
  {
    HS_Scope *scope = hs_scope_open();
    
    //- rjf: get next key
    PTG_Key key = {0};
    ptg_u2b_dequeue_req(&key);
    
    //- rjf: unpack hash
    U64 slot_idx = key.root_hash.u64[1]%ptg_shared->slots_count;
    U64 stripe_idx = slot_idx%ptg_shared->stripes_count;
    PTG_GraphSlot *slot = &ptg_shared->slots[slot_idx];
    PTG_GraphStripe *stripe = &ptg_shared->stripes[stripe_idx];
    
    //- rjf: take task
    B32 got_task = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(PTG_GraphNode *n = slot->first; n != 0; n = n->next)
      {
        if(MemoryMatchStruct(&n->key, &key))
        {
          got_task = !ins_atomic_u32_eval_cond_assign(&n->is_working, 1, 0);
          break;
        }
      }
    }
    
    //- rjf: do task
    if(got_task)
    {
      
    }
    
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(PTG_GraphNode *n = slot->first; n != 0; n = n->next)
      {
        if(MemoryMatchStruct(&n->key, &key))
        {
          
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    hs_scope_close(scope);
  }
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
ptg_evictor_thread__entry_point(void *p)
{
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = ptg_user_clock_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < ptg_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%ptg_shared->stripes_count;
      PTG_GraphSlot *slot = &ptg_shared->slots[slot_idx];
      PTG_GraphStripe *stripe = &ptg_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(PTG_GraphNode *n = slot->first; n != 0; n = n->next)
        {
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            slot_has_work = 1;
            break;
          }
        }
      }
      if(slot_has_work) OS_MutexScopeW(stripe->rw_mutex)
      {
        for(PTG_GraphNode *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            arena_clear(n->arena);
            SLLStackPush(stripe->free_node, n);
          }
        }
      }
      os_sleep_milliseconds(5);
    }
    os_sleep_milliseconds(1000);
  }
}
