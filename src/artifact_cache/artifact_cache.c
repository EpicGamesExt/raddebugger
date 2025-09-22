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
}

////////////////////////////////
//~ rjf: Cache Lookups

internal void *
ac_artifact_from_key(Access *access, String8 key, AC_CreateFunctionType *create, AC_DestroyFunctionType *destroy, U64 slots_count)
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
      RWMutexScope(stripe->rw_mutex, write_mode)
      {
        for(AC_Node *n = slot->first; n != 0; n = n->next)
        {
          if(str8_match(n->key, key, 0))
          {
            found = 1;
            artifact = n->val;
            access_touch(access, &n->access_pt, stripe->cv);
            break;
          }
        }
        if(write_mode && !found)
        {
          AC_Node *node = stripe->free;
          if(node)
          {
            stripe->free = node->next;
          }
          else
          {
            node = push_array(stripe->arena, AC_Node, 1);
            DLLPushBack(slot->first, slot->last, node);
            // TODO(rjf): string allocator for keys
            node->key = str8_copy(stripe->arena, key);
            node->working_count = 1;
          }
        }
      }
      if(found)
      {
        break;
      }
      else if(write_mode)
      {
        MutexScope(ac_shared->req_mutex)
        {
          AC_RequestNode *n = push_array(ac_shared->req_arena, AC_RequestNode, 1);
          SLLQueuePush(ac_shared->first_req, ac_shared->last_req, n);
          ac_shared->req_count += 1;
          n->v.key = str8_copy(ac_shared->req_arena, key);
          n->v.create = create;
        }
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
    reqs[idx].create(reqs[idx].key);
  }
  lane_sync();
  
  scratch_end(scratch);
}
