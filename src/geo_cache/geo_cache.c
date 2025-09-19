// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xe34cd4ff

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
geo_init(void)
{
  Arena *arena = arena_alloc();
  geo_shared = push_array(arena, GEO_Shared, 1);
  geo_shared->arena = arena;
  geo_shared->slots_count = 1024;
  geo_shared->stripes_count = Min(geo_shared->slots_count, os_get_system_info()->logical_processor_count);
  geo_shared->slots = push_array(arena, GEO_Slot, geo_shared->slots_count);
  geo_shared->stripes = push_array(arena, GEO_Stripe, geo_shared->stripes_count);
  geo_shared->stripes_free_nodes = push_array(arena, GEO_Node *, geo_shared->stripes_count);
  for(U64 idx = 0; idx < geo_shared->stripes_count; idx += 1)
  {
    geo_shared->stripes[idx].arena = arena_alloc();
    geo_shared->stripes[idx].rw_mutex = rw_mutex_alloc();
    geo_shared->stripes[idx].cv = cond_var_alloc();
  }
  geo_shared->u2x_ring_size = KB(64);
  geo_shared->u2x_ring_base = push_array_no_zero(arena, U8, geo_shared->u2x_ring_size);
  geo_shared->u2x_ring_cv = cond_var_alloc();
  geo_shared->u2x_ring_mutex = mutex_alloc();
  geo_shared->evictor_thread = thread_launch(geo_evictor_thread__entry_point, 0);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle
geo_buffer_from_hash(Access *access, U128 hash)
{
  R_Handle handle = {0};
  if(!u128_match(hash, u128_zero()))
  {
    U64 slot_idx = hash.u64[1]%geo_shared->slots_count;
    U64 stripe_idx = slot_idx%geo_shared->stripes_count;
    GEO_Slot *slot = &geo_shared->slots[slot_idx];
    GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
    B32 found = 0;
    MutexScopeR(stripe->rw_mutex)
    {
      for(GEO_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash))
        {
          handle = n->buffer;
          found = !r_handle_match(r_handle_zero(), handle);
          access_touch(access, &n->access_pt, stripe->cv);
          break;
        }
      }
    }
    B32 node_is_new = 0;
    if(!found)
    {
      MutexScopeW(stripe->rw_mutex)
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
      geo_u2x_enqueue_req(hash, max_U64);
      async_push_work(geo_xfer_work);
    }
  }
  return handle;
}

internal R_Handle
geo_buffer_from_key(Access *access, C_Key key)
{
  R_Handle handle = {0};
  for(U64 rewind_idx = 0; rewind_idx < C_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
  {
    U128 hash = c_hash_from_key(key, rewind_idx);
    handle = geo_buffer_from_hash(access, hash);
    if(!r_handle_match(handle, r_handle_zero()))
    {
      break;
    }
  }
  return handle;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
geo_u2x_enqueue_req(U128 hash, U64 endt_us)
{
  B32 good = 0;
  MutexScope(geo_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = geo_shared->u2x_ring_write_pos-geo_shared->u2x_ring_read_pos;
    U64 available_size = geo_shared->u2x_ring_size-unconsumed_size;
    if(available_size >= sizeof(hash))
    {
      good = 1;
      geo_shared->u2x_ring_write_pos += ring_write_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_write_pos, &hash);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    cond_var_wait(geo_shared->u2x_ring_cv, geo_shared->u2x_ring_mutex, endt_us);
  }
  if(good)
  {
    cond_var_broadcast(geo_shared->u2x_ring_cv);
  }
  return good;
}

internal void
geo_u2x_dequeue_req(U128 *hash_out)
{
  MutexScope(geo_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = geo_shared->u2x_ring_write_pos-geo_shared->u2x_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out))
    {
      geo_shared->u2x_ring_read_pos += ring_read_struct(geo_shared->u2x_ring_base, geo_shared->u2x_ring_size, geo_shared->u2x_ring_read_pos, hash_out);
      break;
    }
    cond_var_wait(geo_shared->u2x_ring_cv, geo_shared->u2x_ring_mutex, max_U64);
  }
  cond_var_broadcast(geo_shared->u2x_ring_cv);
}

ASYNC_WORK_DEF(geo_xfer_work)
{
  ProfBeginFunction();
  Access *access = access_open();
  
  //- rjf: decode
  U128 hash = {0};
  geo_u2x_dequeue_req(&hash);
  
  //- rjf: unpack hash
  U64 slot_idx = hash.u64[1]%geo_shared->slots_count;
  U64 stripe_idx = slot_idx%geo_shared->stripes_count;
  GEO_Slot *slot = &geo_shared->slots[slot_idx];
  GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
  
  //- rjf: take task
  B32 got_task = 0;
  MutexScopeR(stripe->rw_mutex)
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
    data = c_data_from_hash(access, hash);
  }
  
  //- rjf: data -> buffer
  R_Handle buffer = {0};
  if(got_task && data.size != 0)
  {
    buffer = r_buffer_alloc(R_ResourceKind_Static, data.size, data.str);
  }
  
  //- rjf: commit results to cache
  if(got_task) MutexScopeW(stripe->rw_mutex)
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
  
  access_close(access);
  ProfEnd();
  return 0;
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
geo_evictor_thread__entry_point(void *p)
{
  ThreadNameF("geo_evictor_thread");
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = update_tick_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < geo_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%geo_shared->stripes_count;
      GEO_Slot *slot = &geo_shared->slots[slot_idx];
      GEO_Stripe *stripe = &geo_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      MutexScopeR(stripe->rw_mutex)
      {
        for(GEO_Node *n = slot->first; n != 0; n = n->next)
        {
          if(access_pt_is_expired(&n->access_pt) &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            slot_has_work = 1;
            break;
          }
        }
      }
      if(slot_has_work) MutexScopeW(stripe->rw_mutex)
      {
        for(GEO_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(access_pt_is_expired(&n->access_pt) &&
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
