// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Top-Level API

internal void
fs_init(void)
{
  Arena *arena = arena_alloc();
  fs_shared = push_array(arena, FS_Shared, 1);
  fs_shared->arena = arena;
  fs_shared->slots_count = 1024;
  fs_shared->stripes_count = 64;
  fs_shared->slots = push_array(arena, FS_Slot, fs_shared->slots_count);
  fs_shared->stripes = push_array(arena, FS_Stripe, fs_shared->stripes_count);
  for(U64 idx = 0; idx < fs_shared->stripes_count; idx += 1)
  {
    fs_shared->stripes[idx].arena = arena_alloc();
    fs_shared->stripes[idx].cv = os_condition_variable_alloc();
    fs_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  fs_shared->u2s_ring_size = KB(64);
  fs_shared->u2s_ring_base = push_array_no_zero(arena, U8, fs_shared->u2s_ring_size);
  fs_shared->u2s_ring_cv = os_condition_variable_alloc();
  fs_shared->u2s_ring_mutex = os_mutex_alloc();
  fs_shared->streamer_count = Min(4, os_logical_core_count()-1);
  fs_shared->streamers = push_array(arena, OS_Handle, 1);
  for(U64 idx = 0; idx < fs_shared->streamer_count; idx += 1)
  {
    fs_shared->streamers[idx] = os_launch_thread(fs_streamer_thread__entry_point, &idx, 0);
  }
}

////////////////////////////////
//~ rjf: Cache Interaction

internal U128
fs_hash_from_path(String8 path, U64 rewind_count, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  path = path_normalized_from_string(scratch.arena, path);
  U128 path_key = hs_hash_from_data(path);
  U128 result = hs_hash_from_key(path_key, rewind_count);
  if(u128_match(result, u128_zero()))
  {
    U64 slot_idx = path_key.u64[0]%fs_shared->slots_count;
    U64 stripe_idx = slot_idx%fs_shared->stripes_count;
    FS_Slot *slot = &fs_shared->slots[slot_idx];
    FS_Stripe *stripe = &fs_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      FS_Node *node = 0;
      for(FS_Node *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(path, n->path, 0))
        {
          node = n;
          break;
        }
      }
      if(node == 0) OS_MutexScopeRWPromote(stripe->rw_mutex)
      {
        node = push_array(stripe->arena, FS_Node, 1);
        SLLQueuePush(slot->first, slot->last, node);
        node->path = push_str8_copy(stripe->arena, path);
      }
      if(os_now_microseconds() >= ins_atomic_u64_eval(&node->last_time_requested_us)+1000000 &&
         fs_u2s_enqueue_path(path, endt_us))
      {
        ins_atomic_u64_eval_assign(&node->last_time_requested_us, os_now_microseconds());
      }
      result = hs_hash_from_key(path_key, rewind_count);
      if(u128_match(result, u128_zero()) && os_now_microseconds() <= endt_us)
      {
        os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
      }
      else
      {
        break;
      }
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Streamer Threads

internal B32
fs_u2s_enqueue_path(String8 path, U64 endt_us)
{
  B32 result = 0;
  path.size = Min(path.size, fs_shared->u2s_ring_size);
  OS_MutexScope(fs_shared->u2s_ring_mutex) for(;;)
  {
    U64 unconsumed_size = fs_shared->u2s_ring_write_pos - fs_shared->u2s_ring_read_pos;
    U64 available_size = fs_shared->u2s_ring_size - unconsumed_size;
    if(available_size >= sizeof(U64) + path.size)
    {
      result = 1;
      fs_shared->u2s_ring_write_pos += ring_write_struct(fs_shared->u2s_ring_base, fs_shared->u2s_ring_size, fs_shared->u2s_ring_write_pos, &path.size);
      fs_shared->u2s_ring_write_pos += ring_write(fs_shared->u2s_ring_base, fs_shared->u2s_ring_size, fs_shared->u2s_ring_write_pos, path.str, path.size);
      fs_shared->u2s_ring_write_pos += 7;
      fs_shared->u2s_ring_write_pos -= fs_shared->u2s_ring_write_pos%8;
      break;
    }
    os_condition_variable_wait(fs_shared->u2s_ring_cv, fs_shared->u2s_ring_mutex, endt_us);
  }
  if(result)
  {
    os_condition_variable_broadcast(fs_shared->u2s_ring_cv);
  }
  return result;
}

internal String8
fs_u2s_dequeue_path(Arena *arena)
{
  String8 path = {0};
  OS_MutexScope(fs_shared->u2s_ring_mutex) for(;;)
  {
    U64 unconsumed_size = fs_shared->u2s_ring_write_pos - fs_shared->u2s_ring_read_pos;
    if(unconsumed_size >= sizeof(U64))
    {
      fs_shared->u2s_ring_read_pos += ring_write_struct(fs_shared->u2s_ring_base, fs_shared->u2s_ring_size, fs_shared->u2s_ring_read_pos, &path.size);
      path.str = push_array(arena, U8, path.size);
      fs_shared->u2s_ring_read_pos += ring_write(fs_shared->u2s_ring_base, fs_shared->u2s_ring_size, fs_shared->u2s_ring_read_pos, path.str, path.size);
      fs_shared->u2s_ring_read_pos += 7;
      fs_shared->u2s_ring_read_pos -= fs_shared->u2s_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(fs_shared->u2s_ring_cv, fs_shared->u2s_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(fs_shared->u2s_ring_cv);
  return path;
}

internal void
fs_streamer_thread__entry_point(void *p)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  ThreadName("[fs] streamer #%I64u", (U64)p);
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    String8 path = fs_u2s_dequeue_path(scratch.arena);
    FileProperties pre_props = os_properties_from_file_path(path);
    OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, path);
    U64 data_arena_size = pre_props.size+ARENA_HEADER_SIZE;
    data_arena_size += KB(4)-1;
    data_arena_size -= data_arena_size%KB(4);
    Arena *data_arena = arena_alloc__sized(data_arena_size, data_arena_size);
    String8 data = os_string_from_file_range(data_arena, file, r1u64(0, pre_props.size));
    os_file_close(file);
    FileProperties post_props = os_properties_from_file_path(path);
    if(pre_props.modified != post_props.modified)
    {
      arena_release(data_arena);
      MemoryZeroStruct(&data);
    }
    else
    {
      U128 key = hs_hash_from_data(path);
      hs_submit_data(key, &data_arena, data);
      U64 slot_idx = key.u64[0]%fs_shared->slots_count;
      U64 stripe_idx = slot_idx%fs_shared->stripes_count;
      FS_Slot *slot = &fs_shared->slots[slot_idx];
      FS_Stripe *stripe = &fs_shared->stripes[stripe_idx];
      OS_MutexScopeW(stripe->rw_mutex)
      {
        FS_Node *node = 0;
        for(FS_Node *n = slot->first; n != 0; n = n->next)
        {
          if(str8_match(n->path, path, 0))
          {
            node = n;
            break;
          }
        }
        if(node != 0)
        {
          node->timestamp = post_props.modified;
        }
      }
      os_condition_variable_broadcast(stripe->cv);
    }
    scratch_end(scratch);
  }
}
