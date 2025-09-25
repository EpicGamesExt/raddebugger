// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xfffa00ff

////////////////////////////////
//~ rjf: Top-Level API

internal void
fs_init(void)
{
  Arena *arena = arena_alloc();
  fs_shared = push_array(arena, FS_Shared, 1);
  fs_shared->arena = arena;
  fs_shared->change_gen = 1;
  fs_shared->slots_count = 1024;
  fs_shared->slots = push_array(arena, FS_Slot, fs_shared->slots_count);
  fs_shared->stripes = stripe_array_alloc(arena);
}

////////////////////////////////
//~ rjf: Change Generation

internal U64
fs_change_gen(void)
{
  return ins_atomic_u64_eval(&fs_shared->change_gen);
}

////////////////////////////////
//~ rjf: Cache Interaction

internal AC_Artifact
fs_artifact_create(String8 key, B32 *retry_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: unpack key
  String8 path = {0};
  Rng1U64 range = {0};
  {
    U64 key_read_off = 0;
    key_read_off += str8_deserial_read_struct(key, key_read_off, &path.size);
    path.str = push_array(scratch.arena, U8, path.size);
    key_read_off += str8_deserial_read(key, key_read_off, path.str, path.size, 1);
    key_read_off += str8_deserial_read_struct(key, key_read_off, &range);
  }
  
  //- rjf: measure file properties *before* read
  B32 file_is_good = 0;
  FileProperties pre_props = {0};
  if(lane_idx() == 0)
  {
    pre_props = os_properties_from_file_path(path);
    file_is_good = (pre_props.modified != 0);
  }
  lane_sync_u64(&file_is_good, 0);
  
  //- rjf: setup output data
  Arena *data_arena = 0;
  U64 data_buffer_size = 0;
  U8 *data_buffer = 0;
  if(file_is_good)
  {
    if(lane_idx() == 0)
    {
      U64 range_size = dim_1u64(range);
      U64 read_size = Min(pre_props.size - range.min, range_size);
      U64 data_arena_size = read_size+ARENA_HEADER_SIZE;
      data_arena_size += KB(4)-1;
      data_arena_size -= data_arena_size%KB(4);
      data_arena = arena_alloc(.reserve_size = data_arena_size, .commit_size = data_arena_size);
      data_buffer_size = read_size;
      data_buffer = push_array_no_zero(data_arena, U8, data_buffer_size);
    }
    lane_sync_u64(&data_buffer, 0);
    lane_sync_u64(&data_buffer_size, 0);
  }
  
  //- rjf: open file
  OS_Handle file = {0};
  if(file_is_good)
  {
    if(lane_idx() == 0)
    {
      file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, path);
    }
    lane_sync_u64(&file, 0);
  }
  B32 file_handle_is_valid = !os_handle_match(os_handle_zero(), file);
  
  //- rjf: do read
  U64 total_bytes_read = 0;
  if(file_handle_is_valid)
  {
    U64 *total_bytes_read_ptr = 0;
    if(lane_idx() == 0)
    {
      total_bytes_read_ptr = &total_bytes_read;
    }
    lane_sync_u64(&total_bytes_read_ptr, 0);
    ProfScope("read \"%.*s\" [0x%I64x, 0x%I64x)", str8_varg(path), range.min, range.max)
    {
      Rng1U64 lane_read_range = lane_range(data_buffer_size);
      U64 bytes_read = os_file_read(file, shift_1u64(lane_read_range, range.min), data_buffer + lane_read_range.min);
      ins_atomic_u64_add_eval(total_bytes_read_ptr, bytes_read);
    }
    lane_sync();
    lane_sync_u64(&total_bytes_read, 0);
  }
  
  //- rjf: close file
  if(file_handle_is_valid)
  {
    if(lane_idx() == 0)
    {
      os_file_close(file);
    }
  }
  
  //- rjf: measure file properties *after* read
  FileProperties post_props = {0};
  if(lane_idx() == 0)
  {
    post_props = os_properties_from_file_path(path);
  }
  
  //- rjf: form content key
  C_Key content_key = {0};
  {
    content_key.id.u128[0] = u128_hash_from_str8(key);
  }
  
  //- rjf: abort if modification timestamps or sizes differ - we did not successfully read the file;
  //       otherwise submit data
  B32 read_good = 0;
  if(file_is_good)
  {
    if(lane_idx() == 0)
    {
      read_good = (pre_props.modified == post_props.modified &&
                   pre_props.size == post_props.size &&
                   data_buffer_size == total_bytes_read &&
                   (file_handle_is_valid || pre_props.flags & FilePropertyFlag_IsFolder));
      if(!read_good)
      {
        retry_out[0] = 1;
        ProfScope("abort")
        {
          arena_release(data_arena);
          MemoryZeroStruct(&content_key);
        }
      }
      else
      {
        ProfScope("submit")
        {
          c_submit_data(content_key, &data_arena, str8(data_buffer, data_buffer_size));
        }
      }
    }
    lane_sync();
  }
  
  //- rjf: if the read was good, record this path's timestamp in this layer's path info cache
  U64 path_hash = u64_hash_from_str8(path);
  if(lane_idx() == 0 && read_good)
  {
    U64 slot_idx = path_hash%fs_shared->slots_count;
    FS_Slot *slot = &fs_shared->slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&fs_shared->stripes, slot_idx);
    RWMutexScope(stripe->rw_mutex, 1)
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
      if(node == 0)
      {
        node = stripe->free;
        if(node)
        {
          stripe->free = node->next;
        }
        else
        {
          node = push_array_no_zero(stripe->arena, FS_Node, 1);
        }
        MemoryZeroStruct(node);
        node->path = str8_copy(stripe->arena, path);
        SLLQueuePush(slot->first, slot->last, node);
      }
      node->last_modified_timestamp = pre_props.modified;
      node->size = pre_props.size;
    }
  }
  lane_sync();
  
  //- rjf: bundle content key as artifact
  AC_Artifact artifact = {0};
  StaticAssert(sizeof(content_key) == sizeof(artifact), artifact_key_size_check);
  MemoryCopyStruct(&artifact, &content_key);
  
  scratch_end(scratch);
  ProfEnd();
  return artifact;
}

internal void
fs_artifact_destroy(AC_Artifact artifact)
{
  C_Key key = {0};
  MemoryCopyStruct(&key, &artifact);
  c_close_key(key);
}

internal C_Key
fs_key_from_path_range(String8 path, Rng1U64 range, U64 endt_us)
{
  C_Key result = {0};
  Temp scratch = scratch_begin(0, 0);
  Access *access = access_open();
  {
    String8List key_parts = {0};
    str8_list_push(scratch.arena, &key_parts, str8_struct(&path.size));
    str8_list_push(scratch.arena, &key_parts, path);
    str8_list_push(scratch.arena, &key_parts, str8_struct(&range));
    String8 key = str8_list_join(scratch.arena, &key_parts, 0);
    
    //- rjf: find generation number for this key
    U64 gen = 0;
    {
      U64 hash = u64_hash_from_str8(path);
      U64 slot_idx = hash%fs_shared->slots_count;
      FS_Slot *slot = &fs_shared->slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&fs_shared->stripes, slot_idx);
      RWMutexScope(stripe->rw_mutex, 0)
      {
        for(FS_Node *n = slot->first; n != 0; n = n->next)
        {
          if(str8_match(path, n->path, 0))
          {
            gen = n->gen;
            break;
          }
        }
      }
    }
    
    //- rjf: map to artifact
    AC_Artifact artifact = ac_artifact_from_key(access, key, fs_artifact_create, fs_artifact_destroy, endt_us, .gen = gen, .flags = AC_Flag_Wide);
    MemoryCopyStruct(&result, &artifact);
  }
  access_close(access);
  scratch_end(scratch);
  return result;
}

internal U128
fs_hash_from_path_range(String8 path, Rng1U64 range, U64 endt_us)
{
  U128 hash = {0};
  {
    C_Key key = fs_key_from_path_range(path, range, endt_us);
    for EachIndex(rewind_idx, C_KEY_HASH_HISTORY_COUNT)
    {
      hash = c_hash_from_key(key, rewind_idx);
      if(!u128_match(hash, u128_zero()))
      {
        break;
      }
    }
  }
  return hash;
}

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void
fs_async_tick(void)
{
  ProfBeginFunction();
  
  //- rjf: detect changed timestamps for active paths
  {
    Rng1U64 range = lane_range(fs_shared->slots_count);
    for EachInRange(slot_idx, range)
    {
      FS_Slot *slot = &fs_shared->slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&fs_shared->stripes, slot_idx);
      for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
      {
        B32 found_work = 0;
        RWMutexScope(stripe->rw_mutex, write_mode)
        {
          for(FS_Node *n = slot->first; n != 0; n = n->next)
          {
            FileProperties props = os_properties_from_file_path(n->path);
            if(props.modified != n->last_modified_timestamp)
            {
              found_work = 1;
              if(write_mode)
              {
                n->gen += 1;
                ins_atomic_u64_inc_eval(&fs_shared->change_gen);
              }
            }
          }
        }
        if(!found_work)
        {
          break;
        }
      }
    }
  }
  
  ProfEnd();
}
