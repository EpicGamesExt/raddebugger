// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal TEX_Topology
tex_topology_make(Vec2S32 dim, R_Tex2DFormat fmt)
{
  TEX_Topology top = {0};
  top.dim.x = (S16)Clamp(0, dim.x, max_S32);
  top.dim.y = (S16)Clamp(0, dim.y, max_S32);
  top.fmt = fmt;
  return top;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
tex_init(void)
{
  Arena *arena = arena_alloc();
  tex_shared = push_array(arena, TEX_Shared, 1);
  tex_shared->arena = arena;
  tex_shared->slots_count = 1024;
  tex_shared->stripes_count = Min(tex_shared->slots_count, os_get_system_info()->logical_processor_count);
  tex_shared->slots = push_array(arena, TEX_Slot, tex_shared->slots_count);
  tex_shared->stripes = push_array(arena, TEX_Stripe, tex_shared->stripes_count);
  tex_shared->stripes_free_nodes = push_array(arena, TEX_Node *, tex_shared->stripes_count);
  for(U64 idx = 0; idx < tex_shared->stripes_count; idx += 1)
  {
    tex_shared->stripes[idx].arena = arena_alloc();
    tex_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    tex_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  tex_shared->u2x_ring_size = KB(64);
  tex_shared->u2x_ring_base = push_array_no_zero(arena, U8, tex_shared->u2x_ring_size);
  tex_shared->u2x_ring_cv = os_condition_variable_alloc();
  tex_shared->u2x_ring_mutex = os_mutex_alloc();
  tex_shared->evictor_thread = os_thread_launch(tex_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void
tex_tctx_ensure_inited(void)
{
  if(tex_tctx == 0)
  {
    Arena *arena = arena_alloc();
    tex_tctx = push_array(arena, TEX_TCTX, 1);
    tex_tctx->arena = arena;
  }
}

////////////////////////////////
//~ rjf: Scoped Access

internal TEX_Scope *
tex_scope_open(void)
{
  tex_tctx_ensure_inited();
  TEX_Scope *scope = tex_tctx->free_scope;
  if(scope)
  {
    SLLStackPop(tex_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(tex_tctx->arena, TEX_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
tex_scope_close(TEX_Scope *scope)
{
  for(TEX_Touch *touch = scope->top_touch, *next = 0; touch != 0; touch = next)
  {
    U128 hash = touch->hash;
    next = touch->next;
    U64 slot_idx = hash.u64[1]%tex_shared->slots_count;
    U64 stripe_idx = slot_idx%tex_shared->stripes_count;
    TEX_Slot *slot = &tex_shared->slots[slot_idx];
    TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TEX_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash) && MemoryMatchStruct(&touch->topology, &n->topology))
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
    SLLStackPush(tex_tctx->free_touch, touch);
  }
  SLLStackPush(tex_tctx->free_scope, scope);
}

internal void
tex_scope_touch_node__stripe_r_guarded(TEX_Scope *scope, TEX_Node *node)
{
  TEX_Touch *touch = tex_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, update_tick_idx());
  if(touch != 0)
  {
    SLLStackPop(tex_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(tex_tctx->arena, TEX_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->hash = node->hash;
  touch->topology = node->topology;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle
tex_texture_from_hash_topology(TEX_Scope *scope, U128 hash, TEX_Topology topology)
{
  R_Handle handle = {0};
  {
    U64 slot_idx = hash.u64[1]%tex_shared->slots_count;
    U64 stripe_idx = slot_idx%tex_shared->stripes_count;
    TEX_Slot *slot = &tex_shared->slots[slot_idx];
    TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
    B32 found = 0;
    B32 stale = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(TEX_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash) && MemoryMatchStruct(&topology, &n->topology))
        {
          handle = n->texture;
          found = !r_handle_match(r_handle_zero(), handle);
          tex_scope_touch_node__stripe_r_guarded(scope, n);
          break;
        }
      }
    }
    B32 node_is_new = 0;
    if(!found)
    {
      OS_MutexScopeW(stripe->rw_mutex)
      {
        TEX_Node *node = 0;
        for(TEX_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(hash, n->hash) && MemoryMatchStruct(&topology, &n->topology))
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = tex_shared->stripes_free_nodes[stripe_idx];
          if(node)
          {
            SLLStackPop(tex_shared->stripes_free_nodes[stripe_idx]);
          }
          else
          {
            node = push_array_no_zero(stripe->arena, TEX_Node, 1);
          }
          MemoryZeroStruct(node);
          DLLPushBack(slot->first, slot->last, node);
          node->hash = hash;
          MemoryCopyStruct(&node->topology, &topology);
          node_is_new = 1;
        }
      }
    }
    if(node_is_new)
    {
      tex_u2x_enqueue_req(hash, topology, max_U64);
      async_push_work(tex_xfer_work);
    }
  }
  return handle;
}

internal R_Handle
tex_texture_from_key_topology(TEX_Scope *scope, U128 key, TEX_Topology topology, U128 *hash_out)
{
  R_Handle handle = {0};
  for(U64 rewind_idx = 0; rewind_idx < HS_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
  {
    U128 hash = hs_hash_from_key(key, rewind_idx);
    handle = tex_texture_from_hash_topology(scope, hash, topology);
    if(!r_handle_match(handle, r_handle_zero()))
    {
      if(hash_out)
      {
        *hash_out = hash;
      }
      break;
    }
  }
  return handle;
}

////////////////////////////////
//~ rjf: Transfer Threads

internal B32
tex_u2x_enqueue_req(U128 hash, TEX_Topology top, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(tex_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = tex_shared->u2x_ring_write_pos-tex_shared->u2x_ring_read_pos;
    U64 available_size = tex_shared->u2x_ring_size-unconsumed_size;
    if(available_size >= sizeof(hash)+sizeof(top))
    {
      good = 1;
      tex_shared->u2x_ring_write_pos += ring_write_struct(tex_shared->u2x_ring_base, tex_shared->u2x_ring_size, tex_shared->u2x_ring_write_pos, &hash);
      tex_shared->u2x_ring_write_pos += ring_write_struct(tex_shared->u2x_ring_base, tex_shared->u2x_ring_size, tex_shared->u2x_ring_write_pos, &top);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(tex_shared->u2x_ring_cv, tex_shared->u2x_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(tex_shared->u2x_ring_cv);
  }
  return good;
}

internal void
tex_u2x_dequeue_req(U128 *hash_out, TEX_Topology *top_out)
{
  OS_MutexScope(tex_shared->u2x_ring_mutex) for(;;)
  {
    U64 unconsumed_size = tex_shared->u2x_ring_write_pos-tex_shared->u2x_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out)+sizeof(*top_out))
    {
      tex_shared->u2x_ring_read_pos += ring_read_struct(tex_shared->u2x_ring_base, tex_shared->u2x_ring_size, tex_shared->u2x_ring_read_pos, hash_out);
      tex_shared->u2x_ring_read_pos += ring_read_struct(tex_shared->u2x_ring_base, tex_shared->u2x_ring_size, tex_shared->u2x_ring_read_pos, top_out);
      break;
    }
    os_condition_variable_wait(tex_shared->u2x_ring_cv, tex_shared->u2x_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(tex_shared->u2x_ring_cv);
}

ASYNC_WORK_DEF(tex_xfer_work)
{
  HS_Scope *scope = hs_scope_open();
  
  //- rjf: decode
  U128 hash = {0};
  TEX_Topology top = {0};
  tex_u2x_dequeue_req(&hash, &top);
  
  //- rjf: unpack hash
  U64 slot_idx = hash.u64[1]%tex_shared->slots_count;
  U64 stripe_idx = slot_idx%tex_shared->stripes_count;
  TEX_Slot *slot = &tex_shared->slots[slot_idx];
  TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
  
  //- rjf: take task
  B32 got_task = 0;
  OS_MutexScopeR(stripe->rw_mutex)
  {
    for(TEX_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash) && MemoryMatchStruct(&top, &n->topology))
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
  
  //- rjf: data * topology -> texture
  R_Handle texture = {0};
  if(got_task && top.dim.x > 0 && top.dim.y > 0 && data.size >= (U64)top.dim.x*(U64)top.dim.y*(U64)r_tex2d_format_bytes_per_pixel_table[top.fmt])
  {
    texture = r_tex2d_alloc(R_ResourceKind_Static, v2s32(top.dim.x, top.dim.y), top.fmt, data.str);
  }
  
  //- rjf: commit results to cache
  if(got_task) OS_MutexScopeW(stripe->rw_mutex)
  {
    for(TEX_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash) && MemoryMatchStruct(&top, &n->topology))
      {
        n->texture = texture;
        ins_atomic_u32_eval_assign(&n->is_working, 0);
        ins_atomic_u64_inc_eval(&n->load_count);
        break;
      }
    }
  }
  
  hs_scope_close(scope);
  return 0;
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
tex_evictor_thread__entry_point(void *p)
{
  ThreadNameF("[tex] evictor thread");
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = update_tick_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < tex_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%tex_shared->stripes_count;
      TEX_Slot *slot = &tex_shared->slots[slot_idx];
      TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(TEX_Node *n = slot->first; n != 0; n = n->next)
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
        for(TEX_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            if(!r_handle_match(n->texture, r_handle_zero()))
            {
              r_tex2d_release(n->texture);
            }
            SLLStackPush(tex_shared->stripes_free_nodes[stripe_idx], n);
          }
        }
      }
      os_sleep_milliseconds(5);
    }
    os_sleep_milliseconds(1000);
  }
}
