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

internal void *
ac_artifact_from_key(Access *access, String8 key, U64 gen, AC_CreateFunctionType *create, AC_DestroyFunctionType *destroy, U64 slots_count)
{
  //- rjf: create function -> cache
  AC_Cache *cache = 0;
  {
    U64 cache_hash = u64_hash_from_str8(str8_struct(&create));
    U64 cache_slot_idx = cache_hash%ac_shared->cache_slots_count;
    Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
    for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
    {
      RWMutexScope(cache_stripe->rw_mutex, write_mode)
      {
        for(AC_Cache *c = ac_shared->cache_slots[cache_slot_idx]; c != 0; c = c->next)
        {
          if(c->create == create)
          {
            cache = c;
            break;
          }
        }
        if(write_mode && cache == 0)
        {
          cache = push_array(cache_stripe->arena, AC_Cache, 1);
          SLLStackPush(ac_shared->cache_slots[cache_slot_idx], cache);
          cache->create = create;
          cache->destroy = destroy;
          cache->slots_count = slots_count;
          cache->slots = push_array(cache_stripe->arena, AC_Slot, slots_count);
          cache->stripes = stripe_array_alloc(cache_stripe->arena);
        }
      }
      if(cache != 0)
      {
        break;
      }
    }
  }
  
  //- rjf: cache * key -> artifact
  void *artifact = 0;
  {
    U64 hash = u64_hash_from_str8(key);
    U64 slot_idx = hash%cache->slots_count;
    AC_Slot *slot = &cache->slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
    for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
    {
      B32 found = 0;
      B32 need_request = 0;
      RWMutexScope(stripe->rw_mutex, write_mode)
      {
        // rjf: look up node
        for(AC_Node *n = slot->first; n != 0; n = n->next)
        {
          if(str8_match(n->key, key, 0))
          {
            found = 1;
            artifact = n->val;
            access_touch(access, &n->access_pt, stripe->cv);
            if(n->gen != gen)
            {
              B32 got_task = (ins_atomic_u64_eval_cond_assign(&n->working_count, 1, 0) == 0);
              need_request = got_task;
            }
            break;
          }
        }
        
        // rjf: no node? -> create
        if(write_mode && !found)
        {
          need_request = 1;
          AC_Node *node = stripe->free;
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
      }
      
      // rjf: need request -> push
      if(need_request)
      {
        MutexScope(ac_shared->req_mutex)
        {
          AC_RequestNode *n = push_array(ac_shared->req_arena, AC_RequestNode, 1);
          SLLQueuePush(ac_shared->first_req, ac_shared->last_req, n);
          ac_shared->req_count += 1;
          n->v.key = str8_copy(ac_shared->req_arena, key);
          n->v.gen = gen;
          n->v.create = create;
        }
        cond_var_broadcast(async_tick_start_cond_var);
      }
      
      // rjf: found node -> break
      if(found)
      {
        break;
      }
    }
  }
  
  return artifact;
}

////////////////////////////////
//~ rjf: Tick

internal void
ac_tick(void)
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
    void *val = reqs[idx].create(reqs[idx].key, &retry);
    
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
          }
        }
      }
    }
  }
  lane_sync();
  
  scratch_end(scratch);
}
