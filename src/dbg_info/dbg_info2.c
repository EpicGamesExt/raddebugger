// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal DI2_Key
di2_key_zero(void)
{
  DI2_Key key = {0};
  return key;
}

internal B32
di2_key_match(DI2_Key a, DI2_Key b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
di2_init(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  di2_shared = push_array(arena, DI2_Shared, 1);
  di2_shared->arena = arena;
  di2_shared->key2path_slots_count = 4096;
  di2_shared->key2path_slots = push_array(arena, DI2_KeySlot, di2_shared->key2path_slots_count);
  di2_shared->key2path_stripes = stripe_array_alloc(arena);
  di2_shared->path2key_slots_count = 4096;
  di2_shared->path2key_slots = push_array(arena, DI2_KeySlot, di2_shared->path2key_slots_count);
  di2_shared->path2key_stripes = stripe_array_alloc(arena);
  di2_shared->slots_count = 4096;
  di2_shared->slots = push_array(arena, DI2_Slot, di2_shared->slots_count);
  di2_shared->stripes = stripe_array_alloc(arena);
  for EachElement(idx, di2_shared->req_batches)
  {
    di2_shared->req_batches[idx].mutex = mutex_alloc();
    di2_shared->req_batches[idx].arena = arena_alloc();
  }
  U64 signal_pid = 0;
  String8 signal_pid_string = cmd_line_string(cmdline, str8_lit("signal_pid"));
  B32 has_parent = 1;
  if(!try_u64_from_str8_c_rules(signal_pid_string, &signal_pid))
  {
    has_parent = 0;
    signal_pid = os_get_process_info()->pid;
  }
  di2_shared->conversion_completion_signal_semaphore_name = str8f(arena, "conversion_completion_signal_pid_%I64u", signal_pid);
  if(has_parent)
  {
    di2_shared->conversion_completion_signal_semaphore = semaphore_open(di2_shared->conversion_completion_signal_semaphore_name);
  }
  else
  {
    di2_shared->conversion_completion_signal_semaphore = semaphore_alloc(0, 65536, di2_shared->conversion_completion_signal_semaphore_name);
    di2_shared->conversion_completion_signal_receiver_thread = thread_launch(di2_conversion_completion_signal_receiver_thread_entry_point, 0);
  }
}

////////////////////////////////
//~ rjf: Path * Timestamp Cache Submission & Lookup

internal DI2_Key
di2_key_from_path_timestamp(String8 path, U64 min_timestamp)
{
  //- rjf: unpack key
  U64 hash = u64_hash_from_str8(path);
  U64 slot_idx = hash%di2_shared->path2key_slots_count;
  DI2_KeySlot *slot = &di2_shared->path2key_slots[slot_idx];
  Stripe *stripe = stripe_from_slot_idx(&di2_shared->path2key_stripes, slot_idx);
  
  //- rjf: look up key, create if needed
  DI2_Key key = {0};
  for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
  {
    // rjf: look up node, with this write mode, to find existing key computation
    B32 found = 0;
    RWMutexScope(stripe->rw_mutex, write_mode)
    {
      DI2_KeyPathNode *node = 0;
      for(DI2_KeyPathNode *n = slot->first; n != 0; n = n->next)
      {
        if(str8_match(n->path, path, 0) && min_timestamp <= n->min_timestamp)
        {
          found = 1;
          node = n;
          key = node->key;
          break;
        }
      }
      if(!found && write_mode)
      {
        node = stripe->free;
        if(node)
        {
          stripe->free = node->next;
        }
        else
        {
          node = push_array(stripe->arena, DI2_KeyPathNode, 1);
        }
        node->path = str8_copy(stripe->arena, path);
        node->min_timestamp = min_timestamp;
        node->key = key;
        DLLPushBack(slot->first, slot->last, node);
      }
    }
    
    // rjf: found the key? abort
    if(found)
    {
      break;
    }
    
    // rjf: didn't find the key on our read lookup? compute the key before entering
    // write mode
    if(!found && !write_mode)
    {
      B32 made_key = 0;
      
      //- rjf: try to make key from file's contents
      if(!made_key)
      {
        OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, path);
        FileProperties props = os_properties_from_file(file);
        if(min_timestamp <= props.modified)
        {
          //- rjf: PDB magic => use GUID for key
          if(!made_key)
          {
            B32 is_pdb = 0;
            if(!is_pdb)
            {
              read_only local_persist char msf_msf20_magic[] = "Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0";
              U8 msf20_magic_maybe[sizeof(msf_msf20_magic)] = {0};
              os_file_read(file, r1u64(0, sizeof(msf20_magic_maybe)), msf20_magic_maybe);
              if(MemoryMatch(msf20_magic_maybe, msf_msf20_magic, sizeof(msf20_magic_maybe)))
              {
                is_pdb = 1;
              }
            }
            if(!is_pdb)
            {
              read_only local_persist char msf_msf70_magic[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0";
              U8 msf70_magic_maybe[sizeof(msf_msf70_magic)] = {0};
              os_file_read(file, r1u64(0, sizeof(msf70_magic_maybe)), msf70_magic_maybe);
              if(MemoryMatch(msf70_magic_maybe, msf_msf70_magic, sizeof(msf70_magic_maybe)))
              {
                is_pdb = 1;
              }
            }
            if(is_pdb)
            {
              // TODO(rjf)
            }
          }
        }
        os_file_close(file);
      }
      
      //- rjf: fallback: hash from path/timestamp
      if(!made_key)
      {
        made_key = 1;
        U128 hash = u128_hash_from_seed_str8(min_timestamp, path);
        MemoryCopy(&key, &hash, Min(sizeof(hash), sizeof(key)));
      }
      
      //- rjf: made key -> store in (key -> path/timestamp) table
      if(made_key)
      {
        U64 key_hash = u64_hash_from_str8(str8_struct(&key));
        U64 key_slot_idx = key_hash%di2_shared->key2path_slots_count;
        DI2_KeySlot *key_slot = &di2_shared->key2path_slots[key_slot_idx];
        Stripe *key_stripe = stripe_from_slot_idx(&di2_shared->key2path_stripes, key_slot_idx);
        RWMutexScope(key_stripe->rw_mutex, 1)
        {
          DI2_KeyPathNode *node = 0;
          for EachNode(n, DI2_KeyPathNode, key_slot->first)
          {
            if(di2_key_match(n->key, key))
            {
              node = n;
              break;
            }
          }
          if(node == 0)
          {
            node = key_stripe->free;
            if(node != 0)
            {
              key_stripe->free = node->next;
            }
            else
            {
              node = push_array(key_stripe->arena, DI2_KeyPathNode, 1);
            }
            DLLPushBack(key_slot->first, key_slot->last, node);
            node->path = str8_copy(key_stripe->arena, path);
            node->min_timestamp = min_timestamp;
            node->key = key;
          }
        }
      }
    }
  }
  
  return key;
}

////////////////////////////////
//~ rjf: Debug Info Opening / Closing

internal void
di2_open(DI2_Key key)
{
  //- rjf: unpack key
  U64 hash = u64_hash_from_str8(str8_struct(&key));
  U64 slot_idx = hash%di2_shared->slots_count;
  DI2_Slot *slot = &di2_shared->slots[slot_idx];
  Stripe *stripe = stripe_from_slot_idx(&di2_shared->stripes, slot_idx);
  
  //- rjf: bump this key's node's refcount; create if needed
  B32 node_is_new = 0;
  RWMutexScope(stripe->rw_mutex, 1)
  {
    DI2_Node *node = 0;
    for(DI2_Node *n = slot->first; n != 0; n = n->next)
    {
      if(di2_key_match(n->key, key) && ins_atomic_u64_eval(&n->completion_count) > 0)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node_is_new = 1;
      node = stripe->free;
      if(node)
      {
        stripe->free = node->next;
      }
      else
      {
        node = push_array_no_zero(stripe->arena, DI2_Node, 1);
      }
      MemoryZeroStruct(node);
      DLLPushBack(slot->first, slot->last, node);
      node->key = key;
      node->batch_request_counts[1] = 1;
    }
    node->refcount += 1;
  }
  
  //- rjf: if new, submit low-priority request to load this key
  if(node_is_new)
  {
    DI2_RequestBatch *batch = &di2_shared->req_batches[1];
    MutexScope(batch->mutex)
    {
      DI2_RequestNode *n = push_array(batch->arena, DI2_RequestNode, 1);
      SLLQueuePush(batch->first, batch->last, n);
      n->v.key = key;
      batch->count += 1;
    }
    cond_var_broadcast(async_tick_start_cond_var);
  }
}

internal void
di2_close(DI2_Key key)
{
  //- rjf: unpack key
  U64 hash = u64_hash_from_str8(str8_struct(&key));
  U64 slot_idx = hash%di2_shared->slots_count;
  DI2_Slot *slot = &di2_shared->slots[slot_idx];
  Stripe *stripe = stripe_from_slot_idx(&di2_shared->stripes, slot_idx);
  
  //- rjf: decrement this key's node's refcount; remove if needed
  B32 node_released = 0;
  OS_Handle file = {0};
  OS_Handle file_map = {0};
  FileProperties file_props = {0};
  void *file_base = 0;
  Arena *arena = 0;
  RWMutexScope(stripe->rw_mutex, 1)
  {
    DI2_Node *node = 0;
    for(DI2_Node *n = slot->first; n != 0; n = n->next)
    {
      if(di2_key_match(n->key, key) && ins_atomic_u64_eval(&n->completion_count) > 0)
      {
        node = n;
        break;
      }
    }
    if(node)
    {
      node->refcount -= 1;
      if(node->refcount == 0)
      {
        for(;;)
        {
          if(access_pt_is_expired(&node->access_pt, .time = 0, .update_idxs = 0))
          {
            node_released = 1;
            DLLRemove(slot->first, slot->last, node);
            node->next = stripe->free;
            stripe->free = node;
            file = node->file;
            file_map = node->file_map;
            file_props = node->file_props;
            arena = node->arena;
            break;
          }
          cond_var_wait_rw(stripe->cv, stripe->rw_mutex, 1, max_U64);
        }
      }
    }
  }
  
  //- rjf: release node's resources if needed
  if(node_released)
  {
    arena_release(arena);
    os_file_map_view_close(file_map, file_base, r1u64(0, file_props.size));
    os_file_map_close(file_map);
    os_file_close(file);
  }
}

////////////////////////////////
//~ rjf: Debug Info Lookups

internal U64
di2_load_gen(void)
{
  U64 result = ins_atomic_u64_eval(&di2_shared->load_gen);
  return result;
}

internal DI2_KeyArray
di2_push_all_loaded_keys(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI2_KeyList list = {0};
  {
    for EachIndex(slot_idx, di2_shared->key2path_slots_count)
    {
      DI2_KeySlot *slot = &di2_shared->key2path_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&di2_shared->key2path_stripes, slot_idx);
      RWMutexScope(stripe->rw_mutex, 0)
      {
        for(DI2_KeyPathNode *n = slot->first; n != 0; n = n->next)
        {
          DI2_KeyNode *dst_n = push_array(scratch.arena, DI2_KeyNode, 1);
          SLLQueuePush(list.first, list.last, dst_n);
          list.count += 1;
          dst_n->v = n->key;
        }
      }
    }
  }
  DI2_KeyArray array = {0};
  array.count = list.count;
  array.v = push_array(arena, DI2_Key, array.count);
  {
    U64 idx = 0;
    for EachNode(n, DI2_KeyNode, list.first)
    {
      array.v[idx] = n->v;
      idx += 1;
    }
  }
  scratch_end(scratch);
  return array;
}

internal RDI_Parsed *
di2_rdi_from_key(Access *access, DI2_Key key, B32 high_priority, U64 endt_us)
{
  RDI_Parsed *rdi = &rdi_parsed_nil;
  {
    U64 hash = u64_hash_from_str8(str8_struct(&key));
    U64 slot_idx = hash%di2_shared->slots_count;
    DI2_Slot *slot = &di2_shared->slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&di2_shared->stripes, slot_idx);
    RWMutexScope(stripe->rw_mutex, 0) for(;;)
    {
      // rjf: try to grab current results
      B32 found = 0;
      B32 need_hi_request = 0;
      B32 grabbed = 0;
      for(DI2_Node *n = slot->first; n != 0; n = n->next)
      {
        if(di2_key_match(n->key, key))
        {
          found = 1;
          if(high_priority && ins_atomic_u64_eval_cond_assign(&n->batch_request_counts[0], 1, 0) == 0)
          {
            need_hi_request = 1;
          }
          if(ins_atomic_u64_eval(&n->completion_count) > 0)
          {
            grabbed = 1;
            rdi = &n->rdi;
            access_touch(access, &n->access_pt, stripe->cv);
          }
          break;
        }
      }
      
      // rjf: push high-priority request if needed
      if(need_hi_request)
      {
        DI2_RequestBatch *batch = &di2_shared->req_batches[0];
        MutexScope(batch->mutex)
        {
          DI2_RequestNode *n = push_array(batch->arena, DI2_RequestNode, 1);
          SLLQueuePush(batch->first, batch->last, n);
          n->v.key = key;
          batch->count += 1;
        }
        cond_var_broadcast(async_tick_start_cond_var);
      }
      
      // rjf: found current results, or out-of-time? abort
      if(grabbed || os_now_microseconds() >= endt_us)
      {
        break;
      }
      
      // rjf: wait on stripe change
      cond_var_wait_rw(stripe->cv, stripe->rw_mutex, 0, endt_us);
    }
  }
  return rdi;
}

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void
di2_async_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: do single-lane update: pop requests, update tasks, gather RDI paths to parse wide
  //
  typedef struct ParseTask ParseTask;
  struct ParseTask
  {
    DI2_Key key;
    String8 rdi_path;
  };
  ParseTask *parse_tasks = 0;
  U64 parse_tasks_count = 0;
  if(lane_idx() == 0)
  {
    typedef struct ParseTaskNode ParseTaskNode;
    struct ParseTaskNode
    {
      ParseTaskNode *next;
      ParseTask v;
    };
    ParseTaskNode *first_parse_task = 0;
    ParseTaskNode *last_parse_task = 0;
    
    ////////////////////////////
    //- rjf: pop all requests, high priority first
    //
    DI2_RequestNode *first_req = 0;
    DI2_RequestNode *last_req = 0;
    for EachElement(idx, di2_shared->req_batches)
    {
      DI2_RequestBatch *b = &di2_shared->req_batches[idx];
      MutexScope(b->mutex)
      {
        for EachNode(n, DI2_RequestNode, b->first)
        {
          DI2_RequestNode *n_copy = push_array(scratch.arena, DI2_RequestNode, 1);
          MemoryCopyStruct(&n_copy->v, &n->v);
          SLLQueuePush(first_req, last_req, n_copy);
        }
      }
      arena_clear(b->arena);
      b->first = b->last = 0;
      b->count = 0;
    }
    
    ////////////////////////////
    //- rjf: generate load tasks for all unique requests
    //
    for EachNode(n, DI2_RequestNode, first_req)
    {
      // rjf: unpack request
      DI2_Key key = n->v.key;
      
      // rjf: determine if this request is a duplicate
      B32 request_is_duplicate = 1;
      {
        U64 hash = u64_hash_from_str8(str8_struct(&key));
        U64 slot_idx = hash%di2_shared->slots_count;
        DI2_Slot *slot = &di2_shared->slots[slot_idx];
        Stripe *stripe = stripe_from_slot_idx(&di2_shared->stripes, slot_idx);
        RWMutexScope(stripe->rw_mutex, 0)
        {
          for(DI2_Node *n = slot->first; n != 0; n = n->next)
          {
            if(di2_key_match(n->key, key))
            {
              request_is_duplicate = (ins_atomic_u64_eval_cond_assign(&n->working_count, 1, 0) != 0);
              break;
            }
          }
        }
      }
      
      // rjf: if not a duplicate, create new task
      if(!request_is_duplicate)
      {
        DI2_LoadTask *t = di2_shared->free_load_task;
        if(t)
        {
          SLLStackPop(di2_shared->free_load_task);
        }
        else
        {
          t = push_array_no_zero(di2_shared->arena, DI2_LoadTask, 1);
        }
        MemoryZeroStruct(t);
        DLLPushBack(di2_shared->first_load_task, di2_shared->last_load_task, t);
        t->key = key;
      }
    }
    
    ////////////////////////////
    //- rjf: update tasks: configure, launch if we can, & retire if we can
    //
    for(DI2_LoadTask *t = di2_shared->first_load_task, *next = 0; t != 0; t = next)
    {
      next = t->next;
      
      //- rjf: unpack key
      DI2_Key key = t->key;
      U64 key_hash = u64_hash_from_str8(str8_struct(&key));
      U64 key_slot_idx = key_hash%di2_shared->key2path_slots_count;
      DI2_KeySlot *key_slot = &di2_shared->key2path_slots[key_slot_idx];
      Stripe *key_stripe = stripe_from_slot_idx(&di2_shared->key2path_stripes, key_slot_idx);
      
      //- rjf: get key's O.G. path
      String8 og_path = {0};
      U64 og_min_timestamp = 0;
      RWMutexScope(key_stripe->rw_mutex, 0)
      {
        for(DI2_KeyPathNode *n = key_slot->first; n != 0; n = n->next)
        {
          if(di2_key_match(n->key, key))
          {
            og_path = str8_copy(scratch.arena, n->path);
            og_min_timestamp = n->min_timestamp;
            break;
          }
        }
      }
      
      //- rjf: analyze O.G. debug info
      if(!t->og_analyzed)
      {
        t->og_analyzed = 1;
        OS_Handle file = os_file_open(OS_AccessFlag_ShareRead|OS_AccessFlag_Read, og_path);
        FileProperties props = os_properties_from_file(file);
        t->og_size = props.size;
        U64 rdi_magic_maybe = 0;
        if(os_file_read_struct(file, 0, &rdi_magic_maybe) == 8 &&
           rdi_magic_maybe == RDI_MAGIC_CONSTANT)
        {
          t->og_is_rdi = 1;
        }
        os_file_close(file);
      }
      U64 og_size = t->og_size;
      B32 og_is_rdi = t->og_is_rdi;
      
      //- rjf: compute key's RDI path
      String8 rdi_path = {0};
      {
        if(og_is_rdi)
        {
          rdi_path = og_path;
        }
        else
        {
          rdi_path = str8f(scratch.arena, "%S.rdi", str8_chop_last_dot(og_path));
        }
      }
      
      //- rjf: determine if RDI is stale
      if(!t->rdi_analyzed)
      {
        t->rdi_analyzed = 1;
        OS_Handle file = os_file_open(OS_AccessFlag_ShareRead|OS_AccessFlag_Read, rdi_path);
        FileProperties props = os_properties_from_file(file);
        if(props.modified < og_min_timestamp)
        {
          t->rdi_is_stale = 1;
        }
        else
        {
          t->rdi_is_stale = 1;
          RDI_Header header = {0};
          if(os_file_read_struct(file, 0, &header) == sizeof(header))
          {
            t->rdi_is_stale = (header.encoding_version != RDI_ENCODING_VERSION);
          }
        }
        os_file_close(file);
      }
      B32 rdi_is_stale = t->rdi_is_stale;
      
      //- rjf: calculate thread counts for conversion processes
      if(!og_is_rdi && rdi_is_stale && t->thread_count == 0)
      {
        U64 thread_count = 1;
        U64 max_thread_count = os_get_system_info()->logical_processor_count;
        {
          if(0){}
          else if(og_size <= MB(4))   {thread_count = 1;}
          else if(og_size <= MB(256)) {thread_count = max_thread_count/4;}
          else if(og_size <= MB(512)) {thread_count = max_thread_count/3;}
          else if(og_size <= GB(1)) {thread_count = max_thread_count/2;}
          else {thread_count = max_thread_count;}
        }
        thread_count = Max(1, thread_count);
        t->thread_count = thread_count;
      }
      
      //- rjf: determine if there are threads available
      B32 threads_available = 0;
      {
        U64 max_threads = os_get_system_info()->logical_processor_count*8;
        U64 current_threads = di2_shared->conversion_thread_count;
        U64 needed_threads = (current_threads + t->thread_count);
        threads_available = (max_threads >= needed_threads);
      }
      
      //- rjf: launch conversion processes
      if(threads_available && !og_is_rdi && rdi_is_stale && t->thread_count != 0 && t->status != DI2_LoadTaskStatus_Active)
      {
        B32 should_compress = 0;
        OS_ProcessLaunchParams params = {0};
        params.path = os_get_process_info()->binary_path;
        params.inherit_env = 1;
        params.consoleless = 1;
        str8_list_pushf(scratch.arena, &params.cmd_line, "raddbg");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--bin");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--quiet");
        if(should_compress)
        {
          str8_list_pushf(scratch.arena, &params.cmd_line, "--compress");
        }
        // str8_list_pushf(scratch.arena, &params.cmd_line, "--capture");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--rdi");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--out:%S", rdi_path);
        str8_list_pushf(scratch.arena, &params.cmd_line, "--thread_count:%I64u", t->thread_count);
        str8_list_pushf(scratch.arena, &params.cmd_line, "--signal_pid:%I64u", (U64)os_get_process_info()->pid);
        str8_list_pushf(scratch.arena, &params.cmd_line, "%S", og_path);
        t->process = os_process_launch(&params);
        t->status = DI2_LoadTaskStatus_Active;
        di2_shared->conversion_process_count += 1;
        di2_shared->conversion_thread_count += t->thread_count;
      }
      
      //- rjf: if active & process has completed, mark as done
      {
        U64 exit_code = 0;
        if(t->status == DI2_LoadTaskStatus_Active && os_process_join(t->process, 0, &exit_code))
        {
          t->status = DI2_LoadTaskStatus_Done;
          di2_shared->conversion_process_count -= 1;
          di2_shared->conversion_thread_count -= t->thread_count;
        }
      }
      
      //- rjf: if the RDI for this task is not stale, then we're already done - mark this
      // task as done & prepped for storing into the cache
      if(!rdi_is_stale)
      {
        t->status = DI2_LoadTaskStatus_Done;
      }
      
      //- rjf: if task is done, retire & recycle task; gather path to load
      if(t->status == DI2_LoadTaskStatus_Done)
      {
        DLLRemove(di2_shared->first_load_task, di2_shared->last_load_task, t);
        SLLStackPush(di2_shared->free_load_task, t);
        ParseTaskNode *n = push_array(scratch.arena, ParseTaskNode, 1);
        n->v.key = key;
        n->v.rdi_path = rdi_path;
        SLLQueuePush(first_parse_task, last_parse_task, n);
        parse_tasks_count += 1;
      }
    }
    
    ////////////////////////////
    //- rjf: join all parse tasks
    //
    parse_tasks = push_array(scratch.arena, ParseTask, parse_tasks_count);
    {
      U64 idx = 0;
      for EachNode(n, ParseTaskNode, first_parse_task)
      {
        parse_tasks[idx] = n->v;
        idx += 1;
      }
    }
  }
  lane_sync_u64(&parse_tasks, 0);
  lane_sync_u64(&parse_tasks_count, 0);
  lane_sync();
  
  //////////////////////////////
  //- rjf: do wide load of all prepped RDIs
  //
  U64 parse_task_take_counter = 0;
  U64 *parse_task_take_counter_ptr = 0;
  if(lane_idx() == 0)
  {
    parse_task_take_counter_ptr = &parse_task_take_counter;
  }
  lane_sync_u64(&parse_task_take_counter_ptr, 0);
  {
    for(;;)
    {
      //- rjf: take next task
      U64 parse_task_idx = ins_atomic_u64_inc_eval(parse_task_take_counter_ptr) - 1;
      if(parse_task_idx >= parse_tasks_count)
      {
        break;
      }
      
      //- rjf: unpack task
      DI2_Key key = parse_tasks[parse_task_idx].key;
      String8 rdi_path = parse_tasks[parse_task_idx].rdi_path;
      ProfBegin("parse %.*s", str8_varg(rdi_path));
      
      //- rjf: open file
      OS_Handle file = {0};
      OS_Handle file_map = {0};
      FileProperties file_props = {0};
      void *file_base = 0;
      {
        file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, rdi_path);
        file_map = os_file_map_open(OS_AccessFlag_Read, file);
        file_props = os_properties_from_file(file);
        file_base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, file_props.size));
      }
      
      //- rjf: do initial parse of rdi
      RDI_Parsed rdi_parsed_maybe_compressed = rdi_parsed_nil;
      {
        RDI_ParseStatus parse_status = rdi_parse((U8 *)file_base, file_props.size, &rdi_parsed_maybe_compressed);
        (void)parse_status;
      }
      
      //- rjf: decompress & re-parse, if necessary
      Arena *rdi_parsed_arena = 0;
      RDI_Parsed rdi_parsed = rdi_parsed_maybe_compressed;
      {
        U64 decompressed_size = rdi_decompressed_size_from_parsed(&rdi_parsed_maybe_compressed);
        if(decompressed_size > file_props.size)
        {
          rdi_parsed_arena = arena_alloc();
          U8 *decompressed_data = push_array_no_zero(rdi_parsed_arena, U8, decompressed_size);
          rdi_decompress_parsed(decompressed_data, decompressed_size, &rdi_parsed_maybe_compressed);
          RDI_ParseStatus parse_status = rdi_parse(decompressed_data, decompressed_size, &rdi_parsed);
          (void)parse_status;
        }
      }
      
      //- rjf: commit parsed info to cache
      {
        U64 hash = u64_hash_from_str8(str8_struct(&key));
        U64 slot_idx = hash%di2_shared->slots_count;
        DI2_Slot *slot = &di2_shared->slots[slot_idx];
        Stripe *stripe = stripe_from_slot_idx(&di2_shared->stripes, slot_idx);
        RWMutexScope(stripe->rw_mutex, 1)
        {
          DI2_Node *node = 0;
          for(DI2_Node *n = slot->first; n != 0; n = n->next)
          {
            if(di2_key_match(n->key, key))
            {
              node = n;
              break;
            }
          }
          if(node)
          {
            node->file = file;
            node->file_map = file_map;
            node->file_props = file_props;
            node->file_base = file_base;
            node->arena = rdi_parsed_arena;
            MemoryCopyStruct(&node->rdi, &rdi_parsed);
            node->completion_count += 1;
            node->working_count -= 1;
            ins_atomic_u64_inc_eval(&di2_shared->load_gen);
          }
          else
          {
            if(rdi_parsed_arena != 0)
            {
              arena_release(rdi_parsed_arena);
            }
            os_file_map_view_close(file_map, file_base, r1u64(0, file_props.size));
            os_file_map_close(file_map);
            os_file_close(file);
          }
        }
        cond_var_broadcast(stripe->cv);
      }
      
      ProfEnd();
    }
  }
  lane_sync();
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Conversion Completion Signal Receiver Thread

internal void
di2_signal_completion(void)
{
  semaphore_drop(di2_shared->conversion_completion_signal_semaphore);
}

internal void
di2_conversion_completion_signal_receiver_thread_entry_point(void *p)
{
  ThreadNameF("di2_conversion_completion_signal_receiver_thread");
  for(;;)
  {
    if(semaphore_take(di2_shared->conversion_completion_signal_semaphore, max_U64))
    {
      cond_var_broadcast(async_tick_start_cond_var);
    }
  }
}

////////////////////////////////
//~ rjf: Search Artifact Cache Hooks / Lookups

internal AC_Artifact
di2_search_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out)
{
  ProfBeginFunction();
  Access *access = access_open();
  Temp scratch = scratch_begin(0, 0);
  AC_Artifact artifact = {0};
  {
    //- rjf: unpack key
    RDI_SectionKind section_kind = RDI_SectionKind_NULL;
    String8 query = {0};
    {
      U64 key_read_off = 0;
      key_read_off += str8_deserial_read_struct(key, key_read_off, &section_kind);
      key_read_off += str8_deserial_read_struct(key, key_read_off, &query.size);
      query.str = push_array(scratch.arena, U8, query.size);
      key_read_off += str8_deserial_read(key, key_read_off, query.str, query.size, 1);
    }
    
    //- rjf: gather all debug info keys we'll search on
    DI2_KeyArray keys = {0};
    ProfScope("gather all debug info keys we'll search on")
    {
      if(lane_idx() == 0)
      {
        keys = di2_push_all_loaded_keys(scratch.arena);
      }
      lane_sync_u64(&keys.v, 0);
      lane_sync_u64(&keys.count, 0);
    }
    
    //- rjf: map all debug info keys -> RDIs
    RDI_Parsed **rdis = 0;
    ProfScope("map all debug info keys -> RDIs")
    {
      if(lane_idx() == 0)
      {
        rdis = push_array(scratch.arena, RDI_Parsed *, keys.count);
      }
      lane_sync_u64(&rdis, 0);
      {
        Rng1U64 range = lane_range(keys.count);
        for EachInRange(idx, range)
        {
          rdis[idx] = di2_rdi_from_key(access, keys.v[idx], 0, 0);
        }
      }
    }
    lane_sync();
    
    //- rjf: do wide search on all lanes
    Arena *arena = arena_alloc();
    DI2_SearchItemChunkList *lanes_items = 0;
    ProfScope("do wide search on all lanes")
    {
      if(lane_idx() == 0)
      {
        lanes_items = push_array(scratch.arena, DI2_SearchItemChunkList, lane_count());
      }
      lane_sync_u64(&lanes_items, 0);
      {
        DI2_SearchItemChunkList *lane_items = &lanes_items[lane_idx()];
        for EachIndex(rdi_idx, keys.count)
        {
          DI2_Key key = keys.v[rdi_idx];
          RDI_Parsed *rdi = rdis[rdi_idx];
          
          // rjf: unpack table info
          U64 element_count = 0;
          void *table_base = rdi_section_raw_table_from_kind(rdi, section_kind, &element_count);
          U64 element_size = rdi_section_element_size_table[section_kind];
          
          // rjf: determine name string index offset, depending on table kind
          U64 element_name_idx_off = 0;
          switch(section_kind)
          {
            default:{}break;
            case RDI_SectionKind_Procedures:
            {
              element_name_idx_off = OffsetOf(RDI_Procedure, name_string_idx);
            }break;
            case RDI_SectionKind_GlobalVariables:
            {
              element_name_idx_off = OffsetOf(RDI_GlobalVariable, name_string_idx);
            }break;
            case RDI_SectionKind_ThreadVariables:
            {
              element_name_idx_off = OffsetOf(RDI_ThreadVariable, name_string_idx);
            }break;
            case RDI_SectionKind_UDTs:
            {
              // NOTE(rjf): name must be determined from self_type_idx
            }break;
            case RDI_SectionKind_SourceFiles:
            {
              // NOTE(rjf): name must be determined from file path node chain
            }break;
          }
          
          Rng1U64 range = lane_range(element_count);
          for EachInRange(idx, range)
          {
            //- rjf: every so often, check if we need to cancel, and cancel
            {
              // TODO(rjf)
            }
            
            //- rjf: get element, map to string; if empty, continue to next element
            void *element = (U8 *)table_base + element_size*idx;
            String8 name = {0};
            switch(section_kind)
            {
              case RDI_SectionKind_UDTs:
              {
                RDI_UDT *udt = (RDI_UDT *)element;
                RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
                name.str = rdi_string_from_idx(rdi, type_node->user_defined.name_string_idx, &name.size);
                name = str8_copy(arena, name);
              }break;
              case RDI_SectionKind_SourceFiles:
              {
                Temp scratch = scratch_begin(&arena, 1);
                RDI_SourceFile *file = (RDI_SourceFile *)element;
                String8List path_parts = {0};
                for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, file->file_path_node_idx);
                    fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
                    fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node))
                {
                  String8 path_part = {0};
                  path_part.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &path_part.size);
                  str8_list_push_front(scratch.arena, &path_parts, path_part);
                }
                StringJoin join = {0};
                join.sep = str8_lit("/");
                name = str8_list_join(arena, &path_parts, &join);
                scratch_end(scratch);
              }break;
              default:
              {
                U32 name_idx = *(U32 *)((U8 *)element + element_name_idx_off);
                U64 name_size = 0;
                U8 *name_base = rdi_string_from_idx(rdi, name_idx, &name_size);
                name = str8(name_base, name_size);
              }break;
            }
            if(name.size == 0) { continue; }
            
            //- rjf: fuzzy match against query
            FuzzyMatchRangeList matches = fuzzy_match_find(arena, query, name);
            
            //- rjf: collect
            if(matches.count == matches.needle_part_count)
            {
              DI2_SearchItemChunk *chunk = lane_items->last;
              if(chunk == 0 || chunk->count >= chunk->cap)
              {
                chunk = push_array(scratch.arena, DI2_SearchItemChunk, 1);
                chunk->base_idx = lane_items->total_count;
                chunk->cap = 1024;
                chunk->count = 0;
                chunk->v = push_array_no_zero(scratch.arena, DI2_SearchItem, chunk->cap);
                SLLQueuePush(lane_items->first, lane_items->last, chunk);
                lane_items->chunk_count += 1;
              }
              chunk->v[chunk->count].idx          = idx;
              chunk->v[chunk->count].key          = key;
              chunk->v[chunk->count].match_ranges = matches;
              chunk->v[chunk->count].missed_size  = (name.size > matches.total_dim) ? (name.size-matches.total_dim) : 0;
              chunk->count += 1;
              lane_items->total_count += 1;
            }
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: join all lane chunk lists
    DI2_SearchItemChunkList *all_items = &lanes_items[0];
    if(lane_idx() == 0) ProfScope("join all lane chunk lists")
    {
      for(U64 lidx = 1; lidx < lane_count(); lidx += 1)
      {
        DI2_SearchItemChunkList *dst = all_items;
        DI2_SearchItemChunkList *to_push = &lanes_items[lidx];
        for EachNode(n, DI2_SearchItemChunk, to_push->first)
        {
          n->base_idx += dst->total_count;
        }
        if(dst->first && to_push->first)
        {
          dst->last->next = to_push->first;
          dst->last = to_push->last;
          dst->chunk_count += to_push->chunk_count;
          dst->total_count += to_push->total_count;
        }
        else if(dst->first == 0)
        {
          MemoryCopyStruct(dst, to_push);
        }
        MemoryZeroStruct(to_push);
      }
    }
    lane_sync();
    
    //- rjf: produce sort records
    typedef struct SortRecord SortRecord;
    struct SortRecord
    {
      U64 key;
      DI2_SearchItem *item;
    };
    U64 sort_records_count = all_items->total_count;
    SortRecord *sort_records = 0;
    SortRecord *sort_records__swap = 0;
    ProfScope("produce sort records")
    {
      if(lane_idx() == 0)
      {
        sort_records = push_array(scratch.arena, SortRecord, sort_records_count);
      }
      if(lane_idx() == lane_from_task_idx(1))
      {
        sort_records__swap = push_array(scratch.arena, SortRecord, sort_records_count);
      }
      lane_sync_u64(&sort_records, 0);
      lane_sync_u64(&sort_records__swap, lane_from_task_idx(1));
      for EachNode(n, DI2_SearchItemChunk, all_items->first)
      {
        Rng1U64 range = lane_range(n->count);
        U64 dst_idx = n->base_idx + range.min;
        for EachInRange(n_idx, range)
        {
          DI2_SearchItem *item = &n->v[n_idx];
          sort_records[dst_idx].item = item;
          sort_records[dst_idx].key = (((item->missed_size & 0xffffffffull) << 32) | (u64_hash_from_seed_str8(item->idx, str8_struct(&key)) & 0xffffffffull));
          dst_idx += 1;
        }
      }
    }
    lane_sync();
    
    //- rjf: sort records
    ProfScope("sort records")
    {
      //- rjf: set up common data
      U64 bits_per_digit = 8;
      U64 digits_count = 64 / bits_per_digit;
      U64 num_possible_values_per_digit = 1 << bits_per_digit;
      U32 **lanes_digit_counts = 0;
      U32 **lanes_digit_offsets = 0;
      if(lane_idx() == 0)
      {
        lanes_digit_counts = push_array(scratch.arena, U32 *, lane_count());
        lanes_digit_offsets = push_array(scratch.arena, U32 *, lane_count());
      }
      lane_sync_u64(&lanes_digit_counts, 0);
      lane_sync_u64(&lanes_digit_offsets, 0);
      
      //- rjf: set up this lane
      lanes_digit_counts[lane_idx()] = push_array(scratch.arena, U32, num_possible_values_per_digit);
      lanes_digit_offsets[lane_idx()] = push_array(scratch.arena, U32, num_possible_values_per_digit);
      SortRecord *src = sort_records;
      SortRecord *dst = sort_records__swap;
      U64 count = sort_records_count;
      
      //- rjf: do all per-digit sorts
      for EachIndex(digit_idx, digits_count)
      {
        // rjf: count digit value occurrences per-lane
        {
          U32 *digit_counts = lanes_digit_counts[lane_idx()];
          MemoryZero(digit_counts, sizeof(digit_counts[0])*num_possible_values_per_digit);
          Rng1U64 range = lane_range(count);
          for EachInRange(idx, range)
          {
            SortRecord *r = &src[idx];
            U16 digit_value = (U16)(U8)(r->key >> (digit_idx*bits_per_digit));
            digit_counts[digit_value] += 1;
          }
        }
        lane_sync();
        
        // rjf: compute thread * digit value *relative* offset table
        {
          Rng1U64 range = lane_range(num_possible_values_per_digit);
          for EachInRange(value_idx, range)
          {
            U64 layout_off = 0;
            for EachIndex(lane_idx, lane_count())
            {
              lanes_digit_offsets[lane_idx][value_idx] = layout_off;
              layout_off += lanes_digit_counts[lane_idx][value_idx];
            }
          }
        }
        lane_sync();
        
        // rjf: convert relative offsets -> absolute offsets
        if(lane_idx() == 0)
        {
          U64 last_off = 0;
          U64 num_of_nonzero_digit = 0;
          for EachIndex(value_idx, num_possible_values_per_digit)
          {
            for EachIndex(lane_idx, lane_count())
            {
              lanes_digit_offsets[lane_idx][value_idx] += last_off;
            }
            last_off = lanes_digit_offsets[lane_count()-1][value_idx] + lanes_digit_counts[lane_count()-1][value_idx];
          }
          // NOTE(rjf): required that: (last_off == element_count)
        }
        lane_sync();
        
        // rjf: move
        {
          U32 *lane_digit_offsets = lanes_digit_offsets[lane_idx()];
          Rng1U64 range = lane_range(count);
          for EachInRange(idx, range)
          {
            SortRecord *src_r = &src[idx];
            U16 digit_value = (U16)(U8)(src_r->key >> (digit_idx*bits_per_digit));
            U64 dst_off = lane_digit_offsets[digit_value];
            lane_digit_offsets[digit_value] += 1;
            MemoryCopyStruct(&dst[dst_off], src_r);
          }
        }
        lane_sync();
        
        // rjf: swap
        {
          SortRecord *swap = src;
          src = dst;
          dst = swap;
        }
      }
    }
    lane_sync();
    
    //- rjf: produce final array
    DI2_SearchItemArray items = {0};
    ProfScope("produce final array")
    {
      if(lane_idx() == 0)
      {
        items.count = all_items->total_count;
        items.v = push_array(arena, DI2_SearchItem, items.count);
      }
      lane_sync_u64(&items.count, 0);
      lane_sync_u64(&items.v, 0);
      Rng1U64 range = lane_range(sort_records_count);
      for EachInRange(idx, range)
      {
        SortRecord *record = &sort_records[idx];
        DI2_SearchItem *dst_item = &items.v[idx];
        MemoryCopyStruct(dst_item, record->item);
      }
    }
    lane_sync();
    
    //- rjf: bundle as artifact
    artifact.u64[0] = (U64)arena;
    artifact.u64[1] = (U64)items.v;
    artifact.u64[2] = items.count;
  }
  scratch_end(scratch);
  access_close(access);
  ProfEnd();
  return artifact;
}

internal void
di2_search_artifact_destroy(AC_Artifact artifact)
{
  Arena *arena = (Arena *)artifact.u64[0];
  if(arena != 0)
  {
    arena_release(arena);
  }
}

internal DI2_SearchItemArray
di2_search_item_array_from_target_query(Access *access, RDI_SectionKind target, String8 query, U64 endt_us)
{
  DI2_SearchItemArray result = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: form key
    String8List key_parts = {0};
    str8_list_push(scratch.arena, &key_parts, str8_struct(&target));
    str8_list_push(scratch.arena, &key_parts, str8_struct(&query.size));
    str8_list_push(scratch.arena, &key_parts, query);
    String8 key = str8_list_join(scratch.arena, &key_parts, 0);
    
    // rjf: get artifact
    AC_Artifact artifact = ac_artifact_from_key(access, key, di2_search_artifact_create, di2_search_artifact_destroy, endt_us, .gen = di2_load_gen(), .flags = AC_Flag_Wide);
    
    // rjf: unpack artifact
    result.v = (DI2_SearchItem *)artifact.u64[1];
    result.count = artifact.u64[2];
    
    scratch_end(scratch);
  }
  return result;
}
