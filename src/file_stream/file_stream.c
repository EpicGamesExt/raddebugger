// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xfffa00ff

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
fs_little_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U128
fs_big_hash_from_string_range(String8 string, Rng1U64 range)
{
  Temp scratch = scratch_begin(0, 0);
  U64 buffer_size = string.size + sizeof(U64)*2;
  U8 *buffer = push_array_no_zero(scratch.arena, U8, buffer_size);
  MemoryCopy(buffer, string.str, string.size);
  MemoryCopy(buffer + string.size, &range.min, sizeof(range.min));
  MemoryCopy(buffer + string.size + sizeof(range.min), &range.max, sizeof(range.max));
  U128 hash = c_hash_from_data(str8(buffer, buffer_size));
  scratch_end(scratch);
  return hash;
}

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
  fs_shared->stripes_count = os_get_system_info()->logical_processor_count;
  fs_shared->slots = push_array(arena, FS_Slot, fs_shared->slots_count);
  fs_shared->stripes = push_array(arena, FS_Stripe, fs_shared->stripes_count);
  for(U64 idx = 0; idx < fs_shared->stripes_count; idx += 1)
  {
    fs_shared->stripes[idx].arena = arena_alloc();
    fs_shared->stripes[idx].cv = cond_var_alloc();
    fs_shared->stripes[idx].rw_mutex = rw_mutex_alloc();
  }
  fs_shared->req_mutex = mutex_alloc();
  fs_shared->req_arena = arena_alloc();
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
  FileProperties pre_props = {0};
  if(lane_idx() == 0)
  {
    pre_props = os_properties_from_file_path(path);
  }
  
  //- rjf: setup output data
  Arena *data_arena = 0;
  U64 data_buffer_size = 0;
  U8 *data_buffer = 0;
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
  
  //- rjf: open file
  OS_Handle file = {0};
  if(lane_idx() == 0)
  {
    file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, path);
  }
  lane_sync_u64(&file, 0);
  B32 file_handle_is_valid = !os_handle_match(os_handle_zero(), file);
  
  //- rjf: do read
  U64 total_bytes_read = 0;
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
  
  //- rjf: close file
  if(lane_idx() == 0)
  {
    os_file_close(file);
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
  if(lane_idx() == 0)
  {
    B32 read_good = (pre_props.modified == post_props.modified &&
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
fs_key_from_path_range_new(String8 path, Rng1U64 range, U64 endt_us)
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
    AC_Artifact artifact = ac_artifact_from_key(access, key, fs_artifact_create, fs_artifact_destroy, endt_us);
    MemoryCopyStruct(&result, &artifact);
  }
  access_close(access);
  scratch_end(scratch);
  return result;
}

internal C_Key
fs_key_from_path_range(String8 path, Rng1U64 range, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: unpack args
  path = path_normalized_from_string(scratch.arena, path);
  U64 path_little_hash = fs_little_hash_from_string(path);
  U64 path_slot_idx = path_little_hash%fs_shared->slots_count;
  U64 path_stripe_idx = path_slot_idx%fs_shared->stripes_count;
  FS_Slot *path_slot = &fs_shared->slots[path_slot_idx];
  FS_Stripe *path_stripe = &fs_shared->stripes[path_stripe_idx];
  
  //- rjf: get root for this path - on 1st try (read mode), try to read, on 2nd try (write mode), create node
  C_Root root = {0};
  for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
  {
    B32 node_found = 0;
    RWMutexScope(path_stripe->rw_mutex, write_mode)
    {
      for(FS_Node *n = path_slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->path, path, 0))
        {
          node_found = 1;
          root = n->root;
          break;
        }
      }
      if(write_mode && !node_found)
      {
        FS_Node *node = push_array(path_stripe->arena, FS_Node, 1);
        SLLQueuePush(path_slot->first, path_slot->last, node);
        node->path = push_str8_copy(path_stripe->arena, path);
        node->root = c_root_alloc();
        node->slots_count = 64;
        node->slots = push_array(path_stripe->arena, FS_RangeSlot, node->slots_count);
        root = node->root;
      }
    }
    if(node_found)
    {
      break;
    }
  }
  
  //- rjf: build a key for this path/range combo
  C_Key key = c_key_make(root, c_id_make(range.min, range.max));
  
  //- rjf: if the most recent hash for this key is zero, then try to submit a new
  // request to pull it in.
  if(u128_match(c_hash_from_key(key, 0), u128_zero()))
  {
    // rjf: loop: request, check for results, return until we can't
    RWMutexScope(path_stripe->rw_mutex, 1) for(;;)
    {
      // rjf: path -> node
      FS_Node *node = 0;
      for(FS_Node *n = path_slot->first; n != 0; n = n->next)
      {
        if(str8_match(path, n->path, 0))
        {
          node = n;
          break;
        }
      }
      
      // rjf: no node? -> weird case, node should've been made at this point.
      if(node == 0)
      {
        break;
      }
      
      // rjf: range -> node
      U64 range_hash = fs_little_hash_from_string(str8_struct(&key.id));
      U64 range_slot_idx = range_hash%node->slots_count;
      FS_RangeSlot *range_slot = &node->slots[range_slot_idx];
      FS_RangeNode *range_node = 0;
      for(FS_RangeNode *n = range_slot->first; n != 0; n = n->next)
      {
        if(c_id_match(n->id, key.id))
        {
          range_node = n;
          break;
        }
      }
      
      // rjf: range node does not exist? create & store
      if(range_node == 0)
      {
        range_node = push_array(path_stripe->arena, FS_RangeNode, 1);
        SLLQueuePush(range_slot->first, range_slot->last, range_node);
        range_node->id = key.id;
      }
      
      // rjf: push request
      if(range_node->working_count == 0)
      {
        range_node->working_count += 1;
        MutexScope(fs_shared->req_mutex)
        {
          FS_RequestNode *n = push_array(fs_shared->req_arena, FS_RequestNode, 1);
          SLLQueuePush(fs_shared->first_req, fs_shared->last_req, n);
          fs_shared->req_count += 1;
          n->v.key = key;
          n->v.path = str8_copy(fs_shared->req_arena, path);
          n->v.range = range;
        }
        cond_var_broadcast(async_tick_start_cond_var);
      }
      
      // rjf: have time to wait? -> wait on this stripe; otherwise exit
      B32 have_results = !u128_match(c_hash_from_key(key, 0), u128_zero());
      if(!have_results && os_now_microseconds() < endt_us)
      {
        cond_var_wait_rw(path_stripe->cv, path_stripe->rw_mutex, 1, endt_us);
      }
      else
      {
        break;
      }
    }
  }
  
  scratch_end(scratch);
  return key;
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
#if 0
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: do detection pass
  {
    U64 slots_per_stripe = fs_shared->slots_count/fs_shared->stripes_count;
    Rng1U64 range = lane_range(fs_shared->stripes_count);
    for EachInRange(stripe_idx, range)
    {
      FS_Stripe *stripe = &fs_shared->stripes[stripe_idx];
      MutexScopeR(stripe->rw_mutex) for(U64 slot_in_stripe_idx = 0; slot_in_stripe_idx < slots_per_stripe; slot_in_stripe_idx += 1)
      {
        U64 slot_idx = stripe_idx*slots_per_stripe + slot_in_stripe_idx;
        FS_Slot *slot = &fs_shared->slots[slot_idx];
        for(FS_Node *n = slot->first; n != 0; n = n->next)
        {
          FileProperties props = os_properties_from_file_path(n->path);
          if(props.modified != n->props.modified)
          {
            for(U64 range_slot_idx = 0; range_slot_idx < n->slots_count; range_slot_idx += 1)
            {
              for(FS_RangeNode *range_n = n->slots[range_slot_idx].first;
                  range_n != 0;
                  range_n = range_n->next)
              {
                C_Key key = c_key_make(n->root, range_n->id);
                if(ins_atomic_u64_eval(&range_n->working_count) == 0)
                {
                  ins_atomic_u64_inc_eval(&range_n->working_count);
                  MutexScope(fs_shared->req_mutex)
                  {
                    FS_RequestNode *req_n = push_array(fs_shared->req_arena, FS_RequestNode, 1);
                    SLLQueuePush(fs_shared->first_req, fs_shared->last_req, req_n);
                    fs_shared->req_count += 1;
                    req_n->v.key = key;
                    req_n->v.path = str8_copy(fs_shared->req_arena, n->path);
                    req_n->v.range = range;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  
  //- rjf: gather all requests
  local_persist FS_Request *reqs = 0;
  local_persist U64 reqs_count = 0;
  if(lane_idx() == 0) MutexScope(fs_shared->req_mutex)
  {
    reqs_count = fs_shared->req_count;
    reqs = push_array(scratch.arena, FS_Request, reqs_count);
    U64 idx = 0;
    for EachNode(r, FS_RequestNode, fs_shared->first_req)
    {
      MemoryCopyStruct(&reqs[idx], &r->v);
      reqs[idx].path = str8_copy(scratch.arena, reqs[idx].path);
      idx += 1;
    }
    arena_clear(fs_shared->req_arena);
    fs_shared->first_req = fs_shared->last_req = 0;
    fs_shared->req_count = 0;
    fs_shared->lane_req_take_counter = 0;
  }
  lane_sync();
  
  //- rjf: do requests
  for(;;)
  {
    //- rjf: unpack
    U64 req_num = ins_atomic_u64_inc_eval(&fs_shared->lane_req_take_counter);
    if(req_num < 1 || reqs_count < req_num)
    {
      break;
    }
    U64 req_idx = req_num-1;
    FS_Request *r = &reqs[req_idx];
    C_Key key = r->key;
    String8 path = r->path;
    Rng1U64 range = r->range;
    U64 path_hash = fs_little_hash_from_string(path);
    U64 path_slot_idx = path_hash%fs_shared->slots_count;
    U64 path_stripe_idx = path_slot_idx%fs_shared->stripes_count;
    FS_Slot *path_slot = &fs_shared->slots[path_slot_idx];
    FS_Stripe *path_stripe = &fs_shared->stripes[path_stripe_idx];
    
    //- rjf: load
    ProfBegin("load \"%.*s\"", str8_varg(path));
    FileProperties pre_props = os_properties_from_file_path(path);
    U64 range_size = dim_1u64(range);
    U64 read_size = Min(pre_props.size - range.min, range_size);
    OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, path);
    B32 file_handle_is_valid = !os_handle_match(os_handle_zero(), file);
    U64 data_arena_size = read_size+ARENA_HEADER_SIZE;
    data_arena_size += KB(4)-1;
    data_arena_size -= data_arena_size%KB(4);
    ProfBegin("allocate");
    Arena *data_arena = arena_alloc(.reserve_size = data_arena_size, .commit_size = data_arena_size);
    ProfEnd();
    ProfBegin("read");
    String8 data = os_string_from_file_range(data_arena, file, r1u64(range.min, range.min+read_size));
    ProfEnd();
    os_file_close(file);
    FileProperties post_props = os_properties_from_file_path(path);
    
    //- rjf: abort if modification timestamps or sizes differ - we did not successfully read the file
    B32 read_good = (pre_props.modified == post_props.modified &&
                     pre_props.size == post_props.size &&
                     read_size == data.size &&
                     (file_handle_is_valid || pre_props.flags & FilePropertyFlag_IsFolder));
    if(!read_good)
    {
      ProfScope("abort")
      {
        arena_release(data_arena);
        MemoryZeroStruct(&data);
        data_arena = 0;
      }
    }
    
    //- rjf: submit
    else
    {
      ProfScope("submit")
      {
        c_submit_data(key, &data_arena, data);
      }
    }
    
    //- rjf: commit info to cache
    ProfScope("commit to cache") MutexScopeW(path_stripe->rw_mutex)
    {
      FS_Node *node = 0;
      for(FS_Node *n = path_slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->path, path, 0))
        {
          node = n;
          break;
        }
      }
      if(node != 0 && read_good)
      {
        if(node->props.modified != 0)
        {
          ins_atomic_u64_inc_eval(&fs_shared->change_gen);
        }
        node->props = post_props;
      }
    }
    cond_var_broadcast(path_stripe->cv);
  }
  lane_sync();
  
  scratch_end(scratch);
  ProfEnd();
#endif
}
