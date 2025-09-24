// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Layer Initialization

internal void
ac_init(void)
{
  Arena *arena = arena_alloc();
  ac_shared = push_array(arena, AC_Shared, 1);
  ac_shared->arena = arena;
  ac_shared->cache_slots_count = 256;
  ac_shared->cache_slots = push_array(arena, AC_Cache *, ac_shared->cache_slots_count);
  ac_shared->cache_stripes = stripe_array_alloc(arena);
  ac_shared->req_mutex = mutex_alloc();
  ac_shared->req_arena = arena_alloc();
}

////////////////////////////////
//~ rjf: Cache Lookups

internal AC_Artifact
ac_artifact_from_key_(Access *access, String8 key, AC_ArtifactParams *params, U64 endt_us)
{
  //- rjf: create function -> cache
  AC_Cache *cache = 0;
  {
    U64 cache_hash = u64_hash_from_str8(str8_struct(&params->create));
    U64 cache_slot_idx = cache_hash%ac_shared->cache_slots_count;
    Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
    for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
    {
      RWMutexScope(cache_stripe->rw_mutex, write_mode)
      {
        for(AC_Cache *c = ac_shared->cache_slots[cache_slot_idx]; c != 0; c = c->next)
        {
          if(c->create == params->create)
          {
            cache = c;
            break;
          }
        }
        if(write_mode && cache == 0)
        {
          cache = push_array(cache_stripe->arena, AC_Cache, 1);
          SLLStackPush(ac_shared->cache_slots[cache_slot_idx], cache);
          cache->create = params->create;
          cache->destroy = params->destroy;
          cache->slots_count = Max(256, params->slots_count);
          cache->slots = push_array(cache_stripe->arena, AC_Slot, cache->slots_count);
          cache->stripes = stripe_array_alloc(cache_stripe->arena);
        }
      }
      if(cache != 0)
      {
        break;
      }
    }
  }
  
  //- rjf: unpack key
  U64 hash = u64_hash_from_str8(key);
  U64 slot_idx = hash%cache->slots_count;
  AC_Slot *slot = &cache->slots[slot_idx];
  Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
  
  //- rjf: cache * key -> existing artifact
  B32 artifact_is_stale = 0;
  B32 got_artifact = 0;
  B32 need_request = 0;
  AC_Artifact artifact = {0};
  RWMutexScope(stripe->rw_mutex, 0)
  {
    for(AC_Node *n = slot->first; n != 0; n = n->next)
    {
      if(str8_match(n->key, key, 0))
      {
        if(ins_atomic_u64_eval(&n->completion_count) != 0 && (n->gen == params->gen || !params->wait_for_fresh))
        {
          got_artifact = 1;
          artifact_is_stale = (n->gen == params->gen);
          artifact = n->val;
          access_touch(access, &n->access_pt, stripe->cv);
        }
        if(n->gen != params->gen)
        {
          B32 got_task = (ins_atomic_u64_eval_cond_assign(&n->working_count, 1, 0) == 0);
          need_request = got_task;
        }
        break;
      }
    }
  }
  
  //- rjf: didn't get artifact we want? -> fall back to slow path
  if(!got_artifact || need_request)
  {
    RWMutexScope(stripe->rw_mutex, 1) for(;;)
    {
      B32 out_of_time = (os_now_microseconds() >= endt_us);
      
      // rjf: find node in cache
      AC_Node *node = 0;
      for(AC_Node *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->key, key, 0))
        {
          node = n;
          break;
        }
      }
      
      // rjf: no node? -> create
      if(node == 0)
      {
        need_request = 1;
        node = stripe->free;
        if(node)
        {
          stripe->free = node->next;
        }
        else
        {
          node = push_array_no_zero(stripe->arena, AC_Node, 1);
        }
        MemoryZeroStruct(node);
        DLLPushBack(slot->first, slot->last, node);
        // TODO(rjf): string allocator for keys
        node->key = str8_copy(stripe->arena, key);
        node->working_count = 1;
      }
      
      // rjf: request
      if(need_request)
      {
        need_request = 0;
        MutexScope(ac_shared->req_mutex)
        {
          AC_RequestNode *n = push_array(ac_shared->req_arena, AC_RequestNode, 1);
          SLLQueuePush(ac_shared->first_req, ac_shared->last_req, n);
          ac_shared->req_count += 1;
          n->v.key = str8_copy(ac_shared->req_arena, key);
          n->v.gen = params->gen;
          n->v.create = params->create;
        }
        cond_var_broadcast(async_tick_start_cond_var);
        ins_atomic_u32_eval_assign(&async_loop_again, 1);
      }
      
      // rjf: get value from node, if possible
      if(!got_artifact && ins_atomic_u64_eval(&node->completion_count) != 0 && ((node->gen == params->gen) || !params->wait_for_fresh || out_of_time))
      {
        got_artifact = 1;
        artifact_is_stale = (node->gen == params->gen);
        artifact = node->val;
        access_touch(access, &node->access_pt, stripe->cv);
      }
      
      // rjf: abort if needed
      if(out_of_time || got_artifact || is_async_thread)
      {
        break;
      }
      
      // rjf: wait for results
      cond_var_wait_rw(stripe->cv, stripe->rw_mutex, 1, endt_us);
    }
  }
  
  //- rjf: report staleness
  if(params->stale_out)
  {
    params->stale_out[0] = artifact_is_stale;
  }
  
  return artifact;
}

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void
ac_async_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: do eviction pass across all caches
  for EachIndex(cache_slot_idx, ac_shared->cache_slots_count)
  {
    Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
    RWMutexScope(cache_stripe->rw_mutex, 0)
    {
      for EachNode(cache, AC_Cache, ac_shared->cache_slots[cache_slot_idx])
      {
        Rng1U64 slot_range = lane_range(cache->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          AC_Slot *slot = &cache->slots[slot_idx];
          Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
          for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
          {
            B32 slot_has_work = 0;
            RWMutexScope(stripe->rw_mutex, write_mode)
            {
              for(AC_Node *n = slot->first, *next = 0; n != 0; n = next)
              {
                next = n->next;
                if(access_pt_is_expired(&n->access_pt) && ins_atomic_u64_eval(&n->working_count) == 0)
                {
                  slot_has_work = 1;
                  if(!write_mode)
                  {
                    break;
                  }
                  else
                  {
                    DLLRemove(slot->first, slot->last, n);
                    n->next = (AC_Node *)stripe->free;
                    stripe->free = n;
                    if(cache->destroy)
                    {
                      cache->destroy(n->val);
                    }
                  }
                }
              }
            }
            if(!slot_has_work)
            {
              break;
            }
          }
        }
      }
    }
  }
  
  //- rjf: gather all requests
  local_persist AC_Request *reqs = 0;
  local_persist U64 reqs_count = 0;
  if(lane_idx() == 0) MutexScope(ac_shared->req_mutex)
  {
    reqs_count = ac_shared->req_count;
    reqs = push_array(scratch.arena, AC_Request, reqs_count);
    U64 idx = 0;
    for EachNode(r, AC_RequestNode, ac_shared->first_req)
    {
      MemoryCopyStruct(&reqs[idx], &r->v);
      reqs[idx].key = str8_copy(scratch.arena, reqs[idx].key);
      idx += 1;
    }
    arena_clear(ac_shared->req_arena);
    ac_shared->first_req = ac_shared->last_req = 0;
    ac_shared->req_count = 0;
  }
  lane_sync();
  
  //- rjf: do all requests on all lanes
  for EachIndex(idx, reqs_count)
  {
    lane_sync();
    
    // rjf: compute val
    B32 retry = 0;
    AC_Artifact val = reqs[idx].create(reqs[idx].key, &retry);
    
    // rjf: retry? -> resubmit request
    if(retry && lane_idx() == 0)
    {
      MutexScope(ac_shared->req_mutex)
      {
        AC_RequestNode *n = push_array(ac_shared->req_arena, AC_RequestNode, 1);
        SLLQueuePush(ac_shared->first_req, ac_shared->last_req, n);
        ac_shared->req_count += 1;
        MemoryCopyStruct(&n->v, &reqs[idx]);
        n->v.key = str8_copy(ac_shared->req_arena, n->v.key);
      }
      ins_atomic_u32_eval_assign(&async_loop_again, 1);
    }
    
    // rjf: create function -> cache
    AC_Cache *cache = 0;
    if(!retry)
    {
      U64 cache_hash = u64_hash_from_str8(str8_struct(&reqs[idx].create));
      U64 cache_slot_idx = cache_hash%ac_shared->cache_slots_count;
      Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
      RWMutexScope(cache_stripe->rw_mutex, 0)
      {
        for(AC_Cache *c = ac_shared->cache_slots[cache_slot_idx]; c != 0; c = c->next)
        {
          if(c->create == reqs[idx].create)
          {
            cache = c;
            break;
          }
        }
      }
    }
    
    // rjf: write value into cache
    if(!retry && lane_idx() == 0)
    {
      U64 hash = u64_hash_from_str8(reqs[idx].key);
      U64 slot_idx = hash%cache->slots_count;
      AC_Slot *slot = &cache->slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
      RWMutexScope(stripe->rw_mutex, 1)
      {
        for(AC_Node *n = slot->first; n != 0; n = n->next)
        {
          if(str8_match(n->key, reqs[idx].key, 0))
          {
            n->gen = reqs[idx].gen;
            n->val = val;
            ins_atomic_u64_dec_eval(&n->working_count);
            ins_atomic_u64_inc_eval(&n->completion_count);
          }
        }
      }
      cond_var_broadcast(stripe->cv);
    }
  }
  lane_sync();
  
  scratch_end(scratch);
}
