// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal String8
w32_smsv_dbg_name_from_local_path(String8 path)
{
  String8 result = str8_skip_last_slash(path);
  return result;
}

internal String8
w32_smsv_unique_identifier_from_local_path(String8 path)
{
  U64 local_cache_path_pos = str8_find_needle(path, 0, w32_smsv_state->symbol_cache_path, StringMatchFlag_SlashInsensitive);
  U64 first_slash_pos = str8_find_needle(path, local_cache_path_pos, s("/"), StringMatchFlag_SlashInsensitive);
  String8 cache_relative_path = str8_skip(path, first_slash_pos);
  U64 second_slash_pos = str8_find_needle(cache_relative_path, 0, s("/"), StringMatchFlag_SlashInsensitive);
  String8 name_root = str8_skip(cache_relative_path, second_slash_pos+1);
  U64 third_slash_pos = str8_find_needle(name_root, 0, s("/"), 0);
  String8 name = str8_skip(name_root, third_slash_pos+1);
  U64 fourth_slash_pos = str8_find_needle(name, 0, s("/"), StringMatchFlag_SlashInsensitive);
  String8 unique_identifier = str8_skip(name, fourth_slash_pos+1);
  U64 last_slash_pos = str8_find_needle(unique_identifier, 0, s("/"), StringMatchFlag_SlashInsensitive);
  unique_identifier = str8_prefix(unique_identifier, last_slash_pos);
  return unique_identifier;
}

////////////////////////////////
//~ rjf: Implementation

internal void
smsv_init(void)
{
  Arena *arena = arena_alloc();
  w32_smsv_state = push_array(arena, W32_SMSV_State, 1);
  w32_smsv_state->arena = arena;
  
  // rjf: get symbol server configuration from environment
  {
    // rjf: extract parameterizations from _NT_SYMBOL_PATH
    {
      // rjf: get rules string from environment
      String8 rules = {0};
      for EachNode(n, String8Node, get_process_info()->environment.first)
      {
        String8 string = n->string;
        if(str8_match(string, s("_NT_SYMBOL_PATH="), StringMatchFlag_RightSideSloppy))
        {
          rules = str8_skip(string, 16);
        }
      }
      
      // rjf: parse rules string
      {
        Temp scratch = scratch_begin(0, 0);
        if(str8_match(rules, s("srv*"), StringMatchFlag_RightSideSloppy))
        {
          String8List parts = str8_split(scratch.arena, rules, (U8 *)"*", 1, 0);
          for EachNode(n, String8Node, parts.first)
          {
            PathStyle style = path_style_from_str8(n->string);
            if(w32_smsv_state->symbol_cache_path.size != 0 && style == PathStyle_WindowsAbsolute)
            {
              w32_smsv_state->symbol_cache_path = n->string;
            }
            else if(str8_match(n->string, s("https://"), StringMatchFlag_RightSideSloppy) ||
                    str8_match(n->string, s("http://"), StringMatchFlag_RightSideSloppy))
            {
              str8_list_push(arena, &w32_smsv_state->symbol_server_urls, n->string);
            }
            else if(str8_match(n->string, s("\\\\"), StringMatchFlag_RightSideSloppy))
            {
              // TODO(rjf): network share; currently unsupported
            }
          }
        }
        else
        {
          w32_smsv_state->symbol_cache_path = str8_skip_chop_whitespace(rules);
        }
        scratch_end(scratch);
      }
    }
    
    // rjf: always fall back to public MS server
    {
      String8 public_ms_server = s("https://msdl.microsoft.com/download/symbols");
      B32 public_ms_server_already_present = 0;
      for EachNode(n, String8Node, w32_smsv_state->symbol_server_urls.first)
      {
        if(str8_match(public_ms_server, n->string, 0))
        {
          public_ms_server_already_present = 1;
          break;
        }
      }
      if(!public_ms_server_already_present)
      {
        str8_list_push(arena, &w32_smsv_state->symbol_server_urls, public_ms_server);
      }
    }
    
    // rjf fall back to using C:/SymbolCache (or wherever windows is located)
    if(w32_smsv_state->symbol_cache_path.size == 0)
    {
      Temp scratch = scratch_begin(0, 0);
      U64 size = KB(32);
      U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
      if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_SYSTEM, 0, 0, (WCHAR*)buffer)))
      {
        String8 system32 = str8_from_16(scratch.arena, str16_cstring(buffer));
        String8 containing_drive = str8_prefix(system32, str8_find_needle(system32, 0, s("/"), StringMatchFlag_SlashInsensitive));
        w32_smsv_state->symbol_cache_path = str8f(arena, "%S\\SymbolCache", containing_drive);
      }
      scratch_end(scratch);
    }
  }
  
  // rjf: set up task cache
  {
    w32_smsv_state->task_slots_count = 256;
    w32_smsv_state->task_slots = push_array(arena, W32_SMSV_TaskSlot, w32_smsv_state->task_slots_count);
    w32_smsv_state->task_stripes = stripe_array_alloc(arena);
  }
  
  // rjf: set up rings
  w32_smsv_state->http_response_ring = guarded_ring_alloc(arena, KB(256));
  w32_smsv_state->new_task_ring = guarded_ring_alloc(arena, KB(8));
  w32_smsv_state->retry_arena = arena_alloc();
}

internal void
smsv_async_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: pop new tasks from new task ring, gather them
  W32_SMSV_TaskNode *first_request_task = 0;
  W32_SMSV_TaskNode *last_request_task = 0;
  U64 request_task_count = 0;
  if(lane_idx() == 0)
  {
    for(B32 done = 0;!done;)
    {
      // rjf: get the next task ID to start
      U64 task_id = 0;
      {
        RingGuard g = guarded_ring_open(w32_smsv_state->new_task_ring);
        if(!guarded_ring_try_read_struct(&g, &task_id))
        {
          done = 1;
        }
        guarded_ring_close(&g);
      }
      
      // rjf: gather
      if(!done)
      {
        W32_SMSV_Task *task = (W32_SMSV_Task *)task_id;
        W32_SMSV_TaskNode *n = push_array(scratch.arena, W32_SMSV_TaskNode, 1);
        n->task = task;
        SLLQueuePush(first_request_task, last_request_task, n);
        request_task_count += 1;
      }
    }
  }
  lane_sync();
  
  //- rjf: gather tasks from the retry batch
  if(lane_idx() == 0)
  {
    for(W32_SMSV_TaskNode *t = w32_smsv_state->first_retry_request; t != 0; t = t->next)
    {
      W32_SMSV_TaskNode *n = push_array(scratch.arena, W32_SMSV_TaskNode, 1);
      n->task = t->task;
      SLLQueuePush(first_request_task, last_request_task, n);
      request_task_count += 1;
    }
    arena_clear(w32_smsv_state->retry_arena);
    w32_smsv_state->first_retry_request = w32_smsv_state->last_retry_request = 0;
  }
  lane_sync();
  
  //- rjf: pop responses -> for any responses that suggest that a particular debug info
  // is not present on the server, we need to generate new request nodes for those tasks,
  // to fall back to subsequent server URLS.
  W32_SMSV_TaskNode *first_finish_task = 0;
  W32_SMSV_TaskNode *last_finish_task = 0;
  U64 finish_task_count = 0;
  if(lane_idx() == 0)
  {
    for(HTTP_Response r = {0}; http_pop_response(scratch.arena, w32_smsv_state->http_response_ring, &r, 0);)
    {
      W32_SMSV_Task *task = (W32_SMSV_Task *)r.id;
      if(task != 0)
      {
        if(r.body.size != 0)
        {
          str8_list_push(task->arena, &task->file_pieces, str8_copy(task->arena, r.body));
        }
        if(!r.has_more && task->status != W32_SMSV_TaskStatus_DoneDownloading)
        {
          task->status = W32_SMSV_TaskStatus_DoneDownloading;
          W32_SMSV_TaskNode *n = push_array(scratch.arena, W32_SMSV_TaskNode, 1);
          n->task = task;
          SLLQueuePush(first_finish_task, last_finish_task, n);
          finish_task_count += 1;
        }
      }
    }
  }
  lane_sync();
  
  //- rjf: flatten all request tasks
  W32_SMSV_Task **request_tasks = 0;
  if(lane_idx() == 0)
  {
    request_tasks = push_array(scratch.arena, W32_SMSV_Task *, request_task_count);
    {
      U64 idx = 0;
      for EachNode(n, W32_SMSV_TaskNode, first_request_task)
      {
        request_tasks[idx] = n->task;
        idx += 1;
      }
    }
  }
  lane_sync_u64(&request_tasks, 0);
  
  //- rjf: do all request tasks
  {
    Rng1U64 range = lane_range(request_task_count);
    for EachInRange(idx, range)
    {
      W32_SMSV_Task *task = request_tasks[idx];
      
      // rjf: adjust the server URL node that we're using for this task
      if(task->status == W32_SMSV_TaskStatus_Failed || task->status == W32_SMSV_TaskStatus_Null)
      {
        if(task->current_server_url_node == 0)
        {
          task->current_server_url_node = w32_smsv_state->symbol_server_urls.first;
        }
        else
        {
          task->current_server_url_node = task->current_server_url_node->next;
        }
      }
      
      // rjf: get server URL
      String8 server_url = {0};
      if(task->current_server_url_node != 0)
      {
        server_url = task->current_server_url_node->string;
      }
      
      // rjf: kick off request if we have a server URL
      if(server_url.size != 0)
      {
        task->status = W32_SMSV_TaskStatus_Requested;
        String8 dbg_name = w32_smsv_dbg_name_from_local_path(task->local_path);
        String8 debug_info_unique_identifier = w32_smsv_unique_identifier_from_local_path(task->local_path);
        HTTP_RequestParams params =
        {
          .id = (U64)task,
          .method = HTTP_Method_Get,
          .url = str8f(scratch.arena, "%S/%S/%S/%S", server_url, dbg_name, debug_info_unique_identifier, dbg_name),
        };
        if(!http_push_request(w32_smsv_state->http_response_ring, &params, 0))
        {
          W32_SMSV_TaskNode *n = push_array(w32_smsv_state->retry_arena, W32_SMSV_TaskNode, 1);
          SLLQueuePush(w32_smsv_state->first_retry_request, w32_smsv_state->last_retry_request, n);
          n->task = task;
          ins_atomic_u32_eval_assign(&async_loop_again, 1);
          cond_var_broadcast(async_tick_start_cond_var);
        }
      }
      
      // rjf: if we don't have a server URL, mark this task as failed
      else
      {
        task->status = W32_SMSV_TaskStatus_Failed;
        W32_SMSV_TaskNode *n = push_array(scratch.arena, W32_SMSV_TaskNode, 1);
        n->task = task;
        SLLQueuePush(first_finish_task, last_finish_task, n);
        finish_task_count += 1;
      }
    }
  }
  
  //- rjf: flatten all finish tasks
  W32_SMSV_Task **finish_tasks = 0;
  if(lane_idx() == 0)
  {
    finish_tasks = push_array(scratch.arena, W32_SMSV_Task *, finish_task_count);
    {
      U64 idx = 0;
      for EachNode(n, W32_SMSV_TaskNode, first_finish_task)
      {
        finish_tasks[idx] = n->task;
        idx += 1;
      }
    }
  }
  lane_sync_u64(&finish_tasks, 0);
  
  //- rjf: do all finish tasks
  {
    Rng1U64 range = lane_range(finish_task_count);
    for EachInRange(idx, range)
    {
      W32_SMSV_Task *task = finish_tasks[idx];
      
      // rjf: successful task -> write to local path
      if(task->status != W32_SMSV_TaskStatus_Failed)
      {
        String8 directory = str8_chop_last_slash(task->local_path);
        for(U64 slash_pos = 0; slash_pos <= directory.size;)
        {
          slash_pos = str8_find_needle(directory, slash_pos+1, s("/"), StringMatchFlag_SlashInsensitive);
          String8 root_dir = str8_substr(directory, r1u64(0, slash_pos));
          make_directory(root_dir);
          if(slash_pos == directory.size)
          {
            break;
          }
        }
        write_data_list_to_file_path(task->local_path, task->file_pieces);
        if(task->current_server_url_node != 0)
        {
          String8 origin_path = str8f(scratch.arena, "%S/origin.txt", str8_chop_last_slash(task->local_path));
          write_data_to_file_path(origin_path, task->current_server_url_node->string);
        }
      }
      
      // rjf: evict task from cache
      U64 hash = u64_hash_from_str8(task->local_path);
      U64 slot_idx = hash_index64(hash, w32_smsv_state->task_slots_count);
      W32_SMSV_TaskSlot *slot = &w32_smsv_state->task_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&w32_smsv_state->task_stripes, slot_idx);
      RWMutexScope(stripe->rw_mutex, 1)
      {
        DLLRemove(slot->first, slot->last, task);
        arena_release(task->arena);
      }
    }
  }
  
  scratch_end(scratch);
}

internal String8
smsv_cache_path(void)
{
  return w32_smsv_state->symbol_cache_path;
}

internal String8
smsv_local_path_from_key(Arena *arena, String8 dbg_name, Guid guid, U64 age)
{
  String8 result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    // rjf: form unique identifier based on guid
    String8 debug_info_unique_identifier = {0};
    {
      debug_info_unique_identifier = str8f(scratch.arena, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%I64x",
                                           guid.data1, guid.data2, guid.data3, guid.data4[0], guid.data4[1], guid.data4[2], guid.data4[3], guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7], age);
    }
    
    // rjf: form full path
    if(debug_info_unique_identifier.size != 0)
    {
      String8 symbol_cache_path = w32_smsv_state->symbol_cache_path;
      String8 path = str8f(scratch.arena, "%S/%S/%S/%S", symbol_cache_path, dbg_name, debug_info_unique_identifier, dbg_name);
      result = path_normalized_from_string(arena, path);
    }
  }
  scratch_end(scratch);
  return result;
}

internal void
smsv_fill_local_path(String8 path)
{
  // rjf: determine if we already have this debug info stored locally
  B32 already_cached_locally = (properties_from_file_path(path).modified != 0);
  
  // rjf: if not cached: record (local path -> download task) mapping
  B32 task_is_new = 0;
  U64 task_id = 0;
  if(!already_cached_locally)
  {
    U64 hash = u64_hash_from_str8(path);
    U64 slot_idx = hash_index64(hash, w32_smsv_state->task_slots_count);
    W32_SMSV_TaskSlot *slot = &w32_smsv_state->task_slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&w32_smsv_state->task_stripes, slot_idx);
    for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
    {
      B32 already_exists = 0;
      RWMutexScope(stripe->rw_mutex, write_mode)
      {
        W32_SMSV_Task *node = 0;
        for(W32_SMSV_Task *n = slot->first; n != 0; n = n->next)
        {
          if(path_match_normalized(n->local_path, path))
          {
            already_exists = 1;
            node = n;
            break;
          }
        }
        if(node == 0 && write_mode)
        {
          Arena *arena = arena_alloc();
          node = push_array(arena, W32_SMSV_Task, 1);
          node->arena = arena;
          node->local_path = path_normalized_from_string(arena, path);
          DLLPushBack(slot->first, slot->last, node);
          task_is_new = 1;
          task_id = (U64)node;
        }
      }
      if(already_exists)
      {
        break;
      }
    }
  }
  
  // rjf: if the task is new -> push request to start task
  if(task_is_new)
  {
    RingGuard g = guarded_ring_open(w32_smsv_state->new_task_ring);
    guarded_ring_write_struct_or_wait(&g, &task_id, max_U64);
    guarded_ring_close(&g);
    ins_atomic_u32_eval_assign(&async_loop_again, 1);
    cond_var_broadcast(async_tick_start_cond_var);
  }
}

internal SMSV_Status
smsv_status_from_local_path(String8 path)
{
  SMSV_Status status = SMSV_Status_Null;
  {
    Temp scratch = scratch_begin(0, 0);
    String8 path_normalized = path_normalized_from_string(scratch.arena, path);
    U64 hash = u64_hash_from_str8(path_normalized);
    U64 slot_idx = hash_index64(hash, w32_smsv_state->task_slots_count);
    W32_SMSV_TaskSlot *slot = &w32_smsv_state->task_slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&w32_smsv_state->task_stripes, slot_idx);
    RWMutexScope(stripe->rw_mutex, 0)
    {
      for(W32_SMSV_Task *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->local_path, path, 0))
        {
          status = SMSV_Status_Pending;
          break;
        }
      }
    }
    scratch_end(scratch);
  }
  return status;
}
