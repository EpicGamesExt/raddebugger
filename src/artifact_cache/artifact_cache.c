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
  for EachElement(idx, ac_shared->req_batches)
  {
    ac_shared->req_batches[idx].mutex = mutex_alloc();
    ac_shared->req_batches[idx].arena = arena_alloc();
  }
}

////////////////////////////////
//~ rjf: Cache Lookups

internal AC_Artifact
ac_artifact_from_key_(Access *access, String8 key, AC_ArtifactParams *params, U64 endt_us)
{
  ProfBeginFunction();
  AC_RequestBatch *req_batch = &ac_shared->req_batches[params->flags & AC_Flag_HighPriority ? 0 : 1];
  
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
        ins_atomic_u64_eval_assign(&n->last_requested_gen, params->gen);
        B32 is_stale = (n->last_completed_gen != params->gen);
        if(ins_atomic_u64_eval(&n->completion_count) != 0 && (!is_stale || !(params->flags & AC_Flag_WaitForFresh)))
        {
          got_artifact = 1;
          artifact_is_stale = is_stale;
          artifact = n->val;
          access_touch(access, &n->access_pt, stripe->cv);
        }
        if(is_stale)
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
        node->evict_threshold_us = params->evict_threshold_us;
        node->access_pt.last_time_touched_us = os_now_microseconds();
        node->access_pt.last_update_idx_touched = update_tick_idx();
      }
      
      // rjf: request
      if(need_request)
      {
        need_request = 0;
        MutexScope(req_batch->mutex)
        {
          AC_RequestNode *n = push_array(req_batch->arena, AC_RequestNode, 1);
          if(params->flags & AC_Flag_Wide)
          {
            SLLQueuePush(req_batch->first_wide, req_batch->last_wide, n);
            req_batch->wide_count += 1;
          }
          else
          {
            SLLQueuePush(req_batch->first_thin, req_batch->last_thin, n);
            req_batch->thin_count += 1;
          }
          n->v.key = str8_copy(req_batch->arena, key);
          n->v.gen = params->gen;
          n->v.create = params->create;
          n->v.cancel_signal = params->cancel_signal;
        }
        cond_var_broadcast(async_tick_start_cond_var);
        ins_atomic_u32_eval_assign(&async_loop_again, 1);
        if(params->flags & AC_Flag_HighPriority)
        {
          ins_atomic_u32_eval_assign(&async_loop_again_high_priority, 1);
        }
      }
      
      // rjf: get value from node, if possible
      if(!got_artifact && ins_atomic_u64_eval(&node->completion_count) != 0 && ((node->last_completed_gen == params->gen) || !(params->flags & AC_Flag_WaitForFresh) || out_of_time))
      {
        got_artifact = 1;
        artifact_is_stale = (node->last_completed_gen == params->gen);
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
  
  ProfEnd();
  return artifact;
}

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void
ac_async_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: do eviction pass across all caches
  //
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
                if(access_pt_is_expired(&n->access_pt, .time = n->evict_threshold_us) && ins_atomic_u64_eval(&n->working_count) == 0)
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
  
  //////////////////////////////
  //- rjf: gather requests
  //
  typedef struct RequestBatchTask RequestBatchTask;
  struct RequestBatchTask
  {
    AC_Request *wide;
    U64 wide_count;
    AC_Request *thin;
    U64 thin_count;
  };
  RequestBatchTask *tasks = 0;
  U64 tasks_count = 0;
  if(lane_idx() == 0)
  {
    tasks_count = 2;
    tasks = push_array(scratch.arena, RequestBatchTask, tasks_count);
    for EachElement(task_idx, ac_shared->req_batches)
    {
      AC_RequestBatch *batch = &ac_shared->req_batches[task_idx];
      MutexScope(batch->mutex)
      {
        tasks[task_idx].wide_count = batch->wide_count;
        tasks[task_idx].thin_count = batch->thin_count;
        tasks[task_idx].wide = push_array(scratch.arena, AC_Request, tasks[task_idx].wide_count);
        tasks[task_idx].thin = push_array(scratch.arena, AC_Request, tasks[task_idx].thin_count);
        {
          U64 idx = 0;
          for EachNode(n, AC_RequestNode, batch->first_wide)
          {
            MemoryCopyStruct(&tasks[task_idx].wide[idx], &n->v);
            tasks[task_idx].wide[idx].key = str8_copy(scratch.arena, tasks[task_idx].wide[idx].key);
            idx += 1;
          }
        }
        {
          U64 idx = 0;
          for EachNode(n, AC_RequestNode, batch->first_thin)
          {
            MemoryCopyStruct(&tasks[task_idx].thin[idx], &n->v);
            tasks[task_idx].thin[idx].key = str8_copy(scratch.arena, tasks[task_idx].thin[idx].key);
            idx += 1;
          }
        }
        arena_clear(batch->arena);
        batch->first_wide = batch->last_wide = batch->first_thin = batch->last_thin = 0;
        batch->wide_count = batch->thin_count = 0;
      }
    }
  }
  lane_sync_u64(&tasks, 0);
  lane_sync_u64(&tasks_count, 0);
  lane_sync();
  
  //////////////////////////////
  //- rjf: do all requests
  //
  for EachIndex(task_idx, tasks_count)
  {
    lane_sync();
    RequestBatchTask *task = &tasks[task_idx];
    
    //- rjf: set up cancellation signal
    U64 cancelled = 0;
    U64 *cancelled_ptr = 0;
    if(lane_idx() == 0)
    {
      cancelled_ptr = &cancelled;
    }
    lane_sync_u64(&cancelled_ptr, 0);
    
    //- rjf: do all wide requests for this priority
    U64 done_wide_count = 0;
    ProfScope("wide requests (p%I64u)", task_idx)
    {
      for EachIndex(idx, task->wide_count)
      {
        lane_sync();
        AC_Request *r = &task->wide[idx];
        
        // rjf: any new higher priority tasks? -> cancel
        if(lane_idx() == 0)
        {
          if(task_idx == 1 && idx != 0 && ins_atomic_u32_eval(&async_loop_again_high_priority))
          {
            ins_atomic_u64_eval_assign(cancelled_ptr, 1);
          }
        }
        lane_sync();
        
        // rjf: cancelled? -> exit
        if(ins_atomic_u32_eval(cancelled_ptr))
        {
          break;
        }
        
        // rjf: compute val
        B32 retry = 0;
        AC_Artifact val = r->create(r->key, r->cancel_signal, &retry);
        
        // rjf: retry? -> resubmit request
        if(retry && lane_idx() == 0)
        {
          AC_RequestBatch *batch = &ac_shared->req_batches[task_idx];
          MutexScope(batch->mutex)
          {
            AC_RequestNode *n = push_array(batch->arena, AC_RequestNode, 1);
            SLLQueuePush(batch->first_wide, batch->last_wide, n);
            batch->wide_count += 1;
            MemoryCopyStruct(&n->v, r);
            n->v.key = str8_copy(batch->arena, n->v.key);
          }
          ins_atomic_u32_eval_assign(&async_loop_again, 1);
        }
        
        // rjf: create function -> cache
        AC_Cache *cache = 0;
        if(!retry && lane_idx() == 0)
        {
          U64 cache_hash = u64_hash_from_str8(str8_struct(&r->create));
          U64 cache_slot_idx = cache_hash%ac_shared->cache_slots_count;
          Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
          RWMutexScope(cache_stripe->rw_mutex, 0)
          {
            for(AC_Cache *c = ac_shared->cache_slots[cache_slot_idx]; c != 0; c = c->next)
            {
              if(c->create == r->create)
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
          U64 hash = u64_hash_from_str8(r->key);
          U64 slot_idx = hash%cache->slots_count;
          AC_Slot *slot = &cache->slots[slot_idx];
          Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
          RWMutexScope(stripe->rw_mutex, 1)
          {
            for(AC_Node *n = slot->first; n != 0; n = n->next)
            {
              if(str8_match(n->key, r->key, 0))
              {
                n->last_completed_gen = r->gen;
                n->val = val;
                ins_atomic_u64_dec_eval(&n->working_count);
                ins_atomic_u64_inc_eval(&n->completion_count);
              }
            }
          }
          cond_var_broadcast(stripe->cv);
        }
        
        // rjf: increment count
        lane_sync();
        done_wide_count += 1;
      }
    }
    lane_sync();
    
    //- rjf: do all thin requests for this priority
    U64 done_thin_count = 0;
    ProfScope("thin requests (p%I64u)", task_idx)
    {
      U64 req_take_counter = 0;
      U64 *req_take_counter_ptr = 0;
      if(lane_idx() == 0)
      {
        req_take_counter_ptr = &req_take_counter;
      }
      lane_sync_u64(&req_take_counter_ptr, 0);
      for(;;)
      {
        // rjf: any new higher priority tasks? -> cancel
        if(task_idx == 1 && ins_atomic_u64_eval(req_take_counter_ptr) >= task->thin_count/2 &&
           ins_atomic_u32_eval(&async_loop_again_high_priority))
        {
          ins_atomic_u64_eval_assign(cancelled_ptr, 1);
        }
        
        // rjf: cancelled? -> exit
        if(ins_atomic_u64_eval(cancelled_ptr))
        {
          break;
        }
        
        // rjf: take next task
        U64 req_idx = ins_atomic_u64_inc_eval(req_take_counter_ptr) - 1;
        if(req_idx >= task->thin_count) { break; }
        AC_Request *r = &task->thin[req_idx];
        
        // rjf: compute val
        B32 retry = 0;
        AC_Artifact val = r->create(r->key, r->cancel_signal, &retry);
        
        // rjf: retry? -> resubmit request
        if(retry)
        {
          AC_RequestBatch *batch = &ac_shared->req_batches[task_idx];
          MutexScope(batch->mutex)
          {
            AC_RequestNode *n = push_array(batch->arena, AC_RequestNode, 1);
            SLLQueuePush(batch->first_thin, batch->last_thin, n);
            batch->thin_count += 1;
            MemoryCopyStruct(&n->v, r);
            n->v.key = str8_copy(batch->arena, n->v.key);
          }
          ins_atomic_u32_eval_assign(&async_loop_again, 1);
        }
        
        // rjf: create function -> cache
        AC_Cache *cache = 0;
        if(!retry)
        {
          U64 cache_hash = u64_hash_from_str8(str8_struct(&r->create));
          U64 cache_slot_idx = cache_hash%ac_shared->cache_slots_count;
          Stripe *cache_stripe = stripe_from_slot_idx(&ac_shared->cache_stripes, cache_slot_idx);
          RWMutexScope(cache_stripe->rw_mutex, 0)
          {
            for(AC_Cache *c = ac_shared->cache_slots[cache_slot_idx]; c != 0; c = c->next)
            {
              if(c->create == r->create)
              {
                cache = c;
                break;
              }
            }
          }
        }
        
        // rjf: write value into cache
        if(!retry)
        {
          U64 hash = u64_hash_from_str8(r->key);
          U64 slot_idx = hash%cache->slots_count;
          AC_Slot *slot = &cache->slots[slot_idx];
          Stripe *stripe = stripe_from_slot_idx(&cache->stripes, slot_idx);
          RWMutexScope(stripe->rw_mutex, 1)
          {
            for(AC_Node *n = slot->first; n != 0; n = n->next)
            {
              if(str8_match(n->key, r->key, 0))
              {
                n->last_completed_gen = r->gen;
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
      done_thin_count = ins_atomic_u64_eval(req_take_counter_ptr);
      lane_sync();
    }
    
    //- rjf: cancelled early, unfinished tasks? -> defer to next tick
    if(lane_idx() == 0 && task_idx > 0)
    {
      B32 need_another_try = (done_wide_count < task->wide_count || done_thin_count < task->thin_count);
      AC_RequestBatch *batch = &ac_shared->req_batches[task_idx];
      MutexScope(batch->mutex)
      {
        // rjf: push leftover wide tasks
        for(U64 idx = done_wide_count; idx < task->wide_count; idx += 1)
        {
          AC_Request *r = &task->wide[idx];
          AC_RequestNode *n = push_array(batch->arena, AC_RequestNode, 1);
          SLLQueuePush(batch->first_wide, batch->last_wide, n);
          batch->wide_count += 1;
          MemoryCopyStruct(&n->v, r);
          n->v.key = str8_copy(batch->arena, n->v.key);
        }
        
        // rjf: push leftover thin tasks
        for(U64 idx = done_thin_count; idx < task->thin_count; idx += 1)
        {
          AC_Request *r = &task->thin[idx];
          AC_RequestNode *n = push_array(batch->arena, AC_RequestNode, 1);
          SLLQueuePush(batch->first_thin, batch->last_thin, n);
          batch->thin_count += 1;
          MemoryCopyStruct(&n->v, r);
          n->v.key = str8_copy(batch->arena, n->v.key);
        }
      }
      if(need_another_try)
      {
        ins_atomic_u32_eval_assign(&async_loop_again, 1);
      }
    }
    lane_sync();
  }
  lane_sync();
  
  scratch_end(scratch);
}
