// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xe34cd4ff

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
    tex_shared->stripes[idx].rw_mutex = rw_mutex_alloc();
    tex_shared->stripes[idx].cv = cond_var_alloc();
  }
  tex_shared->req_mutex = mutex_alloc();
  tex_shared->req_arena = arena_alloc();
}

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle
tex_texture_from_hash_topology(Access *access, U128 hash, TEX_Topology topology)
{
  R_Handle handle = {0};
  {
    //- rjf: unpack hash
    U64 slot_idx = hash.u64[1]%tex_shared->slots_count;
    U64 stripe_idx = slot_idx%tex_shared->stripes_count;
    TEX_Slot *slot = &tex_shared->slots[slot_idx];
    TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
    
    //- rjf: get results, request if needed
    for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
    {
      B32 got_node = 0;
      RWMutexScope(stripe->rw_mutex, write_mode)
      {
        // rjf: get node
        TEX_Node *node = 0;
        for(TEX_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(hash, n->hash) && MemoryMatchStruct(&topology, &n->topology))
          {
            node = n;
            got_node = 1;
            break;
          }
        }
        
        // rjf: no node? -> create & request
        if(write_mode && !node)
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
          MutexScope(tex_shared->req_mutex)
          {
            TEX_RequestNode *n = push_array(tex_shared->req_arena, TEX_RequestNode, 1);
            SLLQueuePush(tex_shared->first_req, tex_shared->last_req, n);
            n->v.hash = hash;
            n->v.top  = topology;
            tex_shared->req_count += 1;
          }
        }
        
        // rjf: node? -> grab & access
        if(!write_mode && node)
        {
          handle = node->texture;
          access_touch(access, &node->access_pt, stripe->cv);
        }
      }
      if(got_node)
      {
        break;
      }
    }
  }
  return handle;
}

internal R_Handle
tex_texture_from_key_topology(Access *access, C_Key key, TEX_Topology topology, U128 *hash_out)
{
  R_Handle handle = {0};
  for(U64 rewind_idx = 0; rewind_idx < C_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
  {
    U128 hash = c_hash_from_key(key, rewind_idx);
    handle = tex_texture_from_hash_topology(access, hash, topology);
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
//~ rjf: Tick

internal void
tex_tick(void)
{
  if(ins_atomic_u64_eval(&tex_shared) == 0) { return; }
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: do eviction pass
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = update_tick_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    Rng1U64 range = lane_range(tex_shared->slots_count);
    for EachInRange(slot_idx, range)
    {
      U64 stripe_idx = slot_idx%tex_shared->stripes_count;
      TEX_Slot *slot = &tex_shared->slots[slot_idx];
      TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
      for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
      {
        B32 slot_has_work = 0;
        RWMutexScope(stripe->rw_mutex, write_mode)
        {
          for(TEX_Node *n = slot->first; n != 0; n = n->next)
          {
            if(access_pt_is_expired(&n->access_pt) &&
               n->load_count != 0 &&
               n->working_count == 0)
            {
              slot_has_work = 1;
              if(!write_mode)
              {
                break;
              }
              else
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
        }
        if(!slot_has_work)
        {
          break;
        }
      }
    }
  }
  
  //- rjf: gather all requests
  local_persist TEX_Request *reqs = 0;
  local_persist U64 reqs_count = 0;
  if(lane_idx() == 0) MutexScope(tex_shared->req_mutex)
  {
    reqs_count = tex_shared->req_count;
    reqs = push_array(scratch.arena, TEX_Request, reqs_count);
    U64 idx = 0;
    for EachNode(r, TEX_RequestNode, tex_shared->first_req)
    {
      MemoryCopyStruct(&reqs[idx], &r->v);
      idx += 1;
    }
    arena_clear(tex_shared->req_arena);
    tex_shared->first_req = tex_shared->last_req = 0;
    tex_shared->req_count = 0;
    tex_shared->lane_req_take_counter = 0;
  }
  lane_sync();
  
  //- rjf: do requests
  for(;;)
  {
    //- rjf: get next request
    U64 req_num = ins_atomic_u64_inc_eval(&tex_shared->lane_req_take_counter);
    if(req_num < 1 || reqs_count < req_num)
    {
      break;
    }
    U64 req_idx = req_num-1;
    U128 hash = reqs[req_idx].hash;
    TEX_Topology top = reqs[req_idx].top;
    Access *access = access_open();
    
    //- rjf: unpack request
    U64 slot_idx = hash.u64[1]%tex_shared->slots_count;
    U64 stripe_idx = slot_idx%tex_shared->stripes_count;
    TEX_Slot *slot = &tex_shared->slots[slot_idx];
    TEX_Stripe *stripe = &tex_shared->stripes[stripe_idx];
    String8 data = c_data_from_hash(access, hash);
    
    //- rjf: create texture
    R_Handle texture = {0};
    if(top.dim.x > 0 && top.dim.y > 0 && data.size >= (U64)top.dim.x*(U64)top.dim.y*(U64)r_tex2d_format_bytes_per_pixel_table[top.fmt])
    {
      texture = r_tex2d_alloc(R_ResourceKind_Static, v2s32(top.dim.x, top.dim.y), top.fmt, data.str);
    }
    
    //- rjf: commit results to cache
    RWMutexScope(stripe->rw_mutex, 1)
    {
      for(TEX_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) && MemoryMatchStruct(&top, &n->topology))
        {
          n->texture = texture;
          ins_atomic_u64_dec_eval(&n->working_count);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    access_close(access);
  }
  
  scratch_end(scratch);
  ProfEnd();
}
