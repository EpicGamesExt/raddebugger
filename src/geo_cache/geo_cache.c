// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
geo_init(void)
{
  Arena *arena = arena_alloc();
  geo_shared = push_array(arena, GEO_Shared, 1);
  geo_shared->arena = arena;
  geo_shared->slots_count = 1024;
  geo_shared->stripes_count = 64;
  geo_shared->slots = push_array(arena, GEO_Slot, geo_shared->slots_count);
  geo_shared->stripes = push_array(arena, GEO_Stripe, geo_shared->stripes_count);
  geo_shared->stripes_free_nodes = push_array(arena, GEO_Node *, geo_shared->stripes_count);
  for(U64 idx = 0; idx < geo_shared->stripes_count; idx += 1)
  {
    geo_shared->stripes[idx].arena = arena_alloc();
    geo_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    geo_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  geo_shared->fallback_slots_count = 1024;
  geo_shared->fallback_stripes_count = 64;
  geo_shared->fallback_slots = push_array(arena, GEO_KeyFallbackSlot, geo_shared->fallback_slots_count);
  geo_shared->fallback_stripes = push_array(arena, GEO_Stripe, geo_shared->fallback_stripes_count);
  for(U64 idx = 0; idx < geo_shared->fallback_stripes_count; idx += 1)
  {
    geo_shared->fallback_stripes[idx].arena = arena_alloc();
    geo_shared->fallback_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    geo_shared->fallback_stripes[idx].cv = os_condition_variable_alloc();
  }
  geo_shared->u2x_ring_size = KB(64);
  geo_shared->u2x_ring_base = push_array_no_zero(arena, U8, geo_shared->u2x_ring_size);
  geo_shared->u2x_ring_cv = os_condition_variable_alloc();
  geo_shared->u2x_ring_mutex = os_mutex_alloc();
  geo_shared->xfer_thread_count = Clamp(1, os_logical_core_count()-1, 4);
  geo_shared->xfer_threads = push_array(arena, OS_Handle, geo_shared->xfer_thread_count);
  for(U64 idx = 0; idx < geo_shared->xfer_thread_count; idx += 1)
  {
    geo_shared->xfer_threads[idx] = os_launch_thread(geo_xfer_thread__entry_point, (void *)idx, 0);
  }
  geo_shared->evictor_thread = os_launch_thread(geo_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void
geo_tctx_ensure_inited(void)
{
  if(geo_tctx == 0)
  {
    Arena *arena = arena_alloc();
    geo_tctx = push_array(arena, GEO_TCTX, 1);
    geo_tctx->arena = arena;
  }
}

////////////////////////////////
//~ rjf: User Clock

internal void
geo_user_clock_tick(void)
{
  ins_atomic_u64_inc_eval(&geo_shared->user_clock_idx);
}

internal U64
geo_user_clock_idx(void)
{
  return ins_atomic_u64_eval(&geo_shared->user_clock_idx);
}

////////////////////////////////
//~ rjf: Scoped Access

internal GEO_Scope *
geo_scope_open(void)
{
  geo_tctx_ensure_inited();
  GEO_Scope *scope = geo_tctx->free_scope;
  if(scope)
  {
    SLLStackPop(geo_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(geo_tctx->arena, GEO_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
geo_scope_close(GEO_Scope *scope)
{
  for(GEO_Touch *touch = scope->top_touch, *next = 0; touch != 0; touch = next)
  {
    U128 hash = touch->hash;
    next = touch->next;
    U64 slot_idx = hash.u64[1]%geo_shared->slots_count;
    U64 stripe_idx = slot_idx%geo_shared->stripes_count;
    GEO_Slot *slot = &geo_shared->slots[slot_idx];
    GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(GEO_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash))
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
    SLLStackPush(geo_tctx->free_touch, touch);
  }
  SLLStackPush(geo_tctx->free_scope, scope);
}

internal void
geo_scope_touch_node__stripe_r_guarded(GEO_Scope *scope, GEO_Node *node)
{
  GEO_Touch *touch = geo_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, geo_user_clock_idx());
  if(touch != 0)
  {
    SLLStackPop(geo_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(geo_tctx->arena, GEO_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->hash = node->hash;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle
geo_buffer_from_key_hash(GEO_Scope *scope, U128 key, U128 hash)
{
  R_Handle handle = {0};
  if(!u128_match(hash, u128_zero()))
  {
    U64 slot_idx = hash.u64[1]%geo_shared->slots_count;
    U64 stripe_idx = slot_idx%geo_shared->stripes_count;
    GEO_Slot *slot = &geo_shared->slots[slot_idx];
    GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
    B32 found = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(GEO_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash))
        {
          handle = n->buffer;
          found = !r_handle_match(r_handle_zero(), handle);
          geo_scope_touch_node__stripe_r_guarded(scope, n);
          break;
        }
      }
    }
    B32 node_is_new = 0;
    if(!found)
    {
      OS_MutexScopeW(stripe->rw_mutex)
      {
        GEO_Node *node = 0;
        for(GEO_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(hash, n->hash))
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = geo_shared->stripes_free_nodes[stripe_idx];
          if(node)
          {
            SLLStackPop(geo_shared->stripes_free_nodes[stripe_idx]);
          }
          else
          {
            node = push_array_no_zero(stripe->arena, GEO_Node, 1);
          }
          MemoryZeroStruct(node);
          DLLPushBack(slot->first, slot->last, node);
          node->hash = hash;
          node_is_new = 1;
        }
      }
    }
    if(node_is_new)
    {
      geo_u2x_enqueue_req(key, hash, max_U64);
    }
    if(r_handle_match(handle, r_handle_zero()))
    {
      U128 fallback_hash = {0};
      U64 fallback_slot_idx = key.u64[1]%geo_shared->fallback_slots_count;
      U64 fallback_stripe_idx = fallback_slot_idx%geo_shared->fallback_stripes_count;
      GEO_KeyFallbackSlot *fallback_slot = &geo_shared->fallback_slots[fallback_slot_idx];
      GEO_Stripe *fallback_stripe = &geo_shared->fallback_stripes[fallback_stripe_idx];
      OS_MutexScopeR(fallback_stripe->rw_mutex) for(GEO_KeyFallbackNode *n = fallback_slot->first; n != 0; n = n->next)
      {
        if(u128_match(key, n->key))
        {
          fallback_hash = n->hash;
          break;
        }
      }
      if(!u128_match(fallback_hash, u128_zero()))
      {
        U64 retry_slot_idx = fallback_hash.u64[1]%geo_shared->slots_count;
        U64 retry_stripe_idx = retry_slot_idx%geo_shared->stripes_count;
        GEO_Slot *retry_slot = &geo_shared->slots[retry_slot_idx];
        GEO_Stripe *retry_stripe = &geo_shared->stripes[retry_stripe_idx];
        OS_MutexScopeR(retry_stripe->rw_mutex)
        {
          for(GEO_Node *n = retry_slot->first; n != 0; n = n->next)
          {
            if(u128_match(fallback_hash, n->hash))
            {
              handle = n->buffer;
              geo_scope_touch_node__stripe_r_guarded(scope, n);
              break;
            }
          }
        }
      }
    }
  }
  return handle;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
geo_u2x_enqueue_req(U128 key, U128 hash, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(geo_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = geo_shared->u2x_ring_write_pos-geo_shared->u2x_ring_read_pos;
    U64 available_size = geo_shared->u2x_ring_size-unconsumed_size;
    if(available_size >= sizeof(key)+sizeof(hash))
    {
      good = 1;
      geo_shared->u2x_ring_write_pos += ring_write_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_write_pos, &key);
      geo_shared->u2x_ring_write_pos += ring_write_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_write_pos, &hash);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(geo_shared->u2x_ring_cv, geo_shared->u2x_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(geo_shared->u2x_ring_cv);
  }
  return good;
}

internal void
geo_u2x_dequeue_req(U128 *key_out, U128 *hash_out)
{
  OS_MutexScope(geo_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = geo_shared->u2x_ring_write_pos-geo_shared->u2x_ring_read_pos;
    if(unconsumed_size >= sizeof(*key_out)+sizeof(*hash_out))
    {
      geo_shared->u2x_ring_read_pos += ring_read_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_read_pos, key_out);
      geo_shared->u2x_ring_read_pos += ring_read_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_read_pos, hash_out);
      break;
    }
    os_condition_variable_wait(geo_shared->u2x_ring_cv, geo_shared->u2x_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(geo_shared->u2x_ring_cv);
}

internal void
geo_xfer_thread__entry_point(void *p)
{
  for(;;)
  {
    HS_Scope *scope = hs_scope_open();
    
    //- rjf: decode
    U128 key = {0};
    U128 hash = {0};
    geo_u2x_dequeue_req(&key, &hash);
    
    //- rjf: unpack hash
    U64 slot_idx = hash.u64[1]%geo_shared->slots_count;
    U64 stripe_idx = slot_idx%geo_shared->stripes_count;
    GEO_Slot *slot = &geo_shared->slots[slot_idx];
    GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
    
    //- rjf: take task
    B32 got_task = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(GEO_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash))
        {
          got_task = !ins_atomic_u32_eval_cond_assign(&n->is_working, 1, 0);
          break;
        }
      }
    }
    
    //- rjf: hash -> data
    String8 data = {0};
    if(got_task)
    {
      data = hs_data_from_hash(scope, hash);
    }
    
    //- rjf: data -> buffer
    R_Handle buffer = {0};
    if(got_task && data.size != 0)
    {
      buffer = r_buffer_alloc(R_BufferKind_Static, data.size, data.str);
    }
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(GEO_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash))
        {
          n->buffer = buffer;
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    //- rjf: commit this key/hash pair to fallback cache
    if(got_task && !u128_match(key, u128_zero()) && !u128_match(hash, u128_zero()))
    {
      U64 fallback_slot_idx = key.u64[1]%geo_shared->fallback_slots_count;
      U64 fallback_stripe_idx = fallback_slot_idx%geo_shared->fallback_stripes_count;
      GEO_KeyFallbackSlot *fallback_slot = &geo_shared->fallback_slots[fallback_slot_idx];
      GEO_Stripe *fallback_stripe = &geo_shared->fallback_stripes[fallback_stripe_idx];
      OS_MutexScopeW(fallback_stripe->rw_mutex)
      {
        GEO_KeyFallbackNode *node = 0;
        for(GEO_KeyFallbackNode *n = fallback_slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key))
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = push_array(fallback_stripe->arena, GEO_KeyFallbackNode, 1);
          SLLQueuePush(fallback_slot->first, fallback_slot->last, node);
        }
        node->key = key;
        node->hash = hash;
      }
    }
    
    hs_scope_close(scope);
  }
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
geo_evictor_thread__entry_point(void *p)
{
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = geo_user_clock_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < geo_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%geo_shared->stripes_count;
      GEO_Slot *slot = &geo_shared->slots[slot_idx];
      GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(GEO_Node *n = slot->first; n != 0; n = n->next)
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
        for(GEO_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            if(!r_handle_match(n->buffer, r_handle_zero()))
            {
              r_buffer_release(n->buffer);
            }
            SLLStackPush(geo_shared->stripes_free_nodes[stripe_idx], n);
          }
        }
      }
      os_sleep_milliseconds(5);
    }
    os_sleep_milliseconds(1000);
  }
}
