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
di2_init(void)
{
  Arena *arena = arena_alloc();
  di2_shared = push_array(arena, DI2_Shared, 1);
  di2_shared->arena = arena;
  di2_shared->key_slots_count = 4096;
  di2_shared->key_slots = push_array(arena, DI2_KeySlot, di2_shared->key_slots_count);
  di2_shared->key_stripes = stripe_array_alloc(arena);
  di2_shared->key_path_slots_count = 4096;
  di2_shared->key_path_slots = push_array(arena, DI2_KeySlot, di2_shared->key_path_slots_count);
  di2_shared->key_path_stripes = stripe_array_alloc(arena);
  di2_shared->slots_count = 4096;
  di2_shared->slots = push_array(arena, DI2_Slot, di2_shared->slots_count);
  di2_shared->stripes = stripe_array_alloc(arena);
  for EachElement(idx, di2_shared->req_batches)
  {
    di2_shared->req_batches[idx].mutex = mutex_alloc();
    di2_shared->req_batches[idx].arena = arena_alloc();
  }
}

////////////////////////////////
//~ rjf: Path * Timestamp Cache Submission & Lookup

internal DI2_Key
di2_key_from_path_timestamp(String8 path, U64 min_timestamp)
{
  //- rjf: unpack key
  U64 hash = u64_hash_from_str8(path);
  U64 slot_idx = hash%di2_shared->key_slots_count;
  DI2_KeySlot *slot = &di2_shared->key_slots[slot_idx];
  Stripe *stripe = stripe_from_slot_idx(&di2_shared->key_stripes, slot_idx);
  
  //- rjf: look up key, create if needed
  DI2_Key key = {0};
  for(B32 write_mode = 0; write_mode <= 1; write_mode += 1)
  {
    // rjf: look up node, with this write mode, to find existing key computation
    B32 found = 0;
    RWMutexScope(stripe->rw_mutex, write_mode)
    {
      DI2_KeyNode *node = 0;
      for(DI2_KeyNode *n = slot->first; n != 0; n = n->next)
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
          node = push_array(stripe->arena, DI2_KeyNode, 1);
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
              U8 msf20_magic_maybe[sizeof(msf_msf20_magic)] = {0};
              os_file_read(file, r1u64(0, sizeof(msf20_magic_maybe)), msf20_magic_maybe);
              if(MemoryMatch(msf20_magic_maybe, msf_msf20_magic, sizeof(msf20_magic_maybe)))
              {
                is_pdb = 1;
              }
            }
            if(!is_pdb)
            {
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
        U64 key_slot_idx = key_hash%di2_shared->key_path_slots_count;
        DI2_KeySlot *key_slot = &di2_shared->key_path_slots[key_slot_idx];
        Stripe *key_stripe = stripe_from_slot_idx(&di2_shared->key_path_stripes, key_slot_idx);
        RWMutexScope(key_stripe->rw_mutex, 1)
        {
          DI2_KeyNode *node = 0;
          for EachNode(n, DI2_KeyNode, key_slot->first)
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
              node = push_array(key_stripe->arena, DI2_KeyNode, 1);
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
      U64 key_slot_idx = key_hash%di2_shared->key_slots_count;
      DI2_KeySlot *key_slot = &di2_shared->key_slots[key_slot_idx];
      Stripe *key_stripe = stripe_from_slot_idx(&di2_shared->key_stripes, key_slot_idx);
      
      //- rjf: get key's O.G. path
      String8 og_path = {0};
      U64 og_min_timestamp = 0;
      RWMutexScope(key_stripe->rw_mutex, 0)
      {
        for(DI2_KeyNode *n = key_slot->first; n != 0; n = n->next)
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
          else if(og_size <= MB(512)) {thread_count = max_thread_count/2;}
          else {thread_count = max_thread_count;}
        }
        thread_count = Max(1, thread_count);
        t->thread_count = thread_count;
      }
      
      //- rjf: launch conversion processes
      if(!og_is_rdi && rdi_is_stale && t->thread_count != 0 && t->status != DI2_LoadTaskStatus_Active)
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
        di2_shared->conversion_process_count -= 1;
        di2_shared->conversion_thread_count -= t->thread_count;
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
      U64 parse_task_idx = ins_atomic_u64_inc_eval(parse_task_take_counter_ptr);
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
