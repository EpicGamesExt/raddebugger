// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
di_hash_from_string(String8 string, StringMatchFlags match_flags)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + ((match_flags & StringMatchFlag_CaseInsensitive) ? char_to_lower(string.str[i]) : string.str[i]);
  }
  return result;
}

internal U64
di_hash_from_key(DI_Key *k)
{
  U64 hash = di_hash_from_string(k->path, StringMatchFlag_CaseInsensitive);
  return hash;
}

internal DI_Key
di_key_zero(void)
{
  DI_Key key = {0};
  return key;
}

internal B32
di_key_match(DI_Key *a, DI_Key *b)
{
  return (str8_match(a->path, b->path, StringMatchFlag_CaseInsensitive) && a->min_timestamp == b->min_timestamp);
}

internal DI_Key
di_key_copy(Arena *arena, DI_Key *src)
{
  DI_Key dst = {0};
  MemoryCopyStruct(&dst, src);
  dst.path = push_str8_copy(arena, src->path);
  return dst;
}

internal DI_Key
di_normalized_key_from_key(Arena *arena, DI_Key *src)
{
  DI_Key dst = {path_normalized_from_string(arena, src->path), src->min_timestamp};
  return dst;
}

internal void
di_key_list_push(Arena *arena, DI_KeyList *list, DI_Key *key)
{
  DI_KeyNode *n = push_array(arena, DI_KeyNode, 1);
  MemoryCopyStruct(&n->v, key);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal DI_KeyArray
di_key_array_from_list(Arena *arena, DI_KeyList *list)
{
  DI_KeyArray array = {0};
  array.count = list->count;
  array.v = push_array_no_zero(arena, DI_Key, array.count);
  U64 idx = 0;
  for(DI_KeyNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    MemoryCopyStruct(&array.v[idx], &n->v);
  }
  return array;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
di_init(void)
{
  Arena *arena = arena_alloc();
  di_shared = push_array(arena, DI_Shared, 1);
  di_shared->arena = arena;
  di_shared->slots_count = 1024;
  di_shared->slots = push_array(arena, DI_Slot, di_shared->slots_count);
  di_shared->stripes_count = Min(di_shared->slots_count, os_get_system_info()->logical_processor_count);
  di_shared->stripes = push_array(arena, DI_Stripe, di_shared->stripes_count);
  for(U64 idx = 0; idx < di_shared->stripes_count; idx += 1)
  {
    di_shared->stripes[idx].arena = arena_alloc();
    di_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    di_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  di_shared->u2p_ring_mutex = os_mutex_alloc();
  di_shared->u2p_ring_cv = os_condition_variable_alloc();
  di_shared->u2p_ring_size = KB(64);
  di_shared->u2p_ring_base = push_array_no_zero(arena, U8, di_shared->u2p_ring_size);
  di_shared->p2u_ring_mutex = os_mutex_alloc();
  di_shared->p2u_ring_cv = os_condition_variable_alloc();
  di_shared->p2u_ring_size = KB(64);
  di_shared->p2u_ring_base = push_array_no_zero(arena, U8, di_shared->p2u_ring_size);
}

////////////////////////////////
//~ rjf: Scope Functions

internal DI_Scope *
di_scope_open(void)
{
  if(di_tctx == 0)
  {
    Arena *arena = arena_alloc();
    di_tctx = push_array(arena, DI_TCTX, 1);
    di_tctx->arena = arena;
  }
  DI_Scope *scope = di_tctx->free_scope;
  if(scope != 0)
  {
    SLLStackPop(di_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(di_tctx->arena, DI_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
di_scope_close(DI_Scope *scope)
{
  for(DI_Touch *t = scope->first_touch, *next = 0; t != 0; t = next)
  {
    next = t->next;
    if(t->node != 0)
    {
      ins_atomic_u64_dec_eval(&t->node->touch_count);
    }
    SLLStackPush(di_tctx->free_touch, t);
  }
  SLLStackPush(di_tctx->free_scope, scope);
}

internal void
di_scope_touch_node__stripe_mutex_r_guarded(DI_Scope *scope, DI_Node *node)
{
  if(node != 0)
  {
    ins_atomic_u64_inc_eval(&node->touch_count);
  }
  DI_Touch *touch = di_tctx->free_touch;
  if(touch != 0)
  {
    SLLStackPop(di_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(di_tctx->arena, DI_Touch, 1);
  }
  MemoryZeroStruct(touch);
  SLLQueuePush(scope->first_touch, scope->last_touch, touch);
  touch->node = node;
}

////////////////////////////////
//~ rjf: Per-Slot Functions

internal DI_Node *
di_node_from_key_slot__stripe_mutex_r_guarded(DI_Slot *slot, DI_Key *key)
{
  DI_Node *node = 0;
  StringMatchFlags match_flags = path_match_flags_from_os(operating_system_from_context());
  U64 most_recent_timestamp = max_U64;
  for(DI_Node *n = slot->first; n != 0; n = n->next)
  {
    if(str8_match(n->key.path, key->path, match_flags) &&
       key->min_timestamp <= n->key.min_timestamp &&
       (n->key.min_timestamp - key->min_timestamp) <= most_recent_timestamp)
    {
      node = n;
      most_recent_timestamp = (n->key.min_timestamp - key->min_timestamp);
    }
  }
  return node;
}

////////////////////////////////
//~ rjf: Per-Stripe Functions

internal U64
di_string_bucket_idx_from_string_size(U64 size)
{
  U64 size_rounded = u64_up_to_pow2(size+1);
  size_rounded = ClampBot((1<<4), size_rounded);
  U64 bucket_idx = 0;
  switch(size_rounded)
  {
    case 1<<4: {bucket_idx = 0;}break;
    case 1<<5: {bucket_idx = 1;}break;
    case 1<<6: {bucket_idx = 2;}break;
    case 1<<7: {bucket_idx = 3;}break;
    case 1<<8: {bucket_idx = 4;}break;
    case 1<<9: {bucket_idx = 5;}break;
    case 1<<10:{bucket_idx = 6;}break;
    default:{bucket_idx = ArrayCount(((DI_Stripe *)0)->free_string_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
di_string_alloc__stripe_mutex_w_guarded(DI_Stripe *stripe, String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = di_string_bucket_idx_from_string_size(string.size);
  DI_StringChunkNode *node = stripe->free_string_chunks[bucket_idx];
  
  // rjf: pull from bucket free list
  if(node != 0)
  {
    if(bucket_idx == ArrayCount(stripe->free_string_chunks)-1)
    {
      node = 0;
      DI_StringChunkNode *prev = 0;
      for(DI_StringChunkNode *n = stripe->free_string_chunks[bucket_idx];
          n != 0;
          prev = n, n = n->next)
      {
        if(n->size >= string.size+1)
        {
          if(prev == 0)
          {
            stripe->free_string_chunks[bucket_idx] = n->next;
          }
          else
          {
            prev->next = n->next;
          }
          node = n;
          break;
        }
      }
    }
    else
    {
      SLLStackPop(stripe->free_string_chunks[bucket_idx]);
    }
  }
  
  // rjf: no found node -> allocate new
  if(node == 0)
  {
    U64 chunk_size = 0;
    if(bucket_idx < ArrayCount(stripe->free_string_chunks)-1)
    {
      chunk_size = 1<<(bucket_idx+4);
    }
    else
    {
      chunk_size = u64_up_to_pow2(string.size);
    }
    U8 *chunk_memory = push_array(stripe->arena, U8, chunk_size);
    node = (DI_StringChunkNode *)chunk_memory;
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
di_string_release__stripe_mutex_w_guarded(DI_Stripe *stripe, String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = di_string_bucket_idx_from_string_size(string.size);
  DI_StringChunkNode *node = (DI_StringChunkNode *)string.str;
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(stripe->free_string_chunks[bucket_idx], node);
}

////////////////////////////////
//~ rjf: Key Opening/Closing

internal void
di_open(DI_Key *key)
{
  Temp scratch = scratch_begin(0, 0);
  if(key->path.size != 0)
  {
    DI_Key key_normalized = di_normalized_key_from_key(scratch.arena, key);
    U64 hash = di_hash_from_key(&key_normalized);
    U64 slot_idx = hash%di_shared->slots_count;
    U64 stripe_idx = slot_idx%di_shared->stripes_count;
    DI_Slot *slot = &di_shared->slots[slot_idx];
    DI_Stripe *stripe = &di_shared->stripes[stripe_idx];
    log_infof("open_debug_info: {\"%S\", 0x%I64x}\n", key_normalized.path, key_normalized.min_timestamp);
    OS_MutexScopeW(stripe->rw_mutex)
    {
      //- rjf: find existing node
      DI_Node *node = di_node_from_key_slot__stripe_mutex_r_guarded(slot, &key_normalized);
      
      //- rjf: allocate node if none exists; insert into slot
      if(node == 0)
      {
        U64 current_timestamp = os_properties_from_file_path(key_normalized.path).modified;
        if(current_timestamp == 0)
        {
          current_timestamp = key_normalized.min_timestamp;
        }
        node = stripe->free_node;
        if(node != 0)
        {
          SLLStackPop(stripe->free_node);
        }
        else
        {
          node = push_array_no_zero(stripe->arena, DI_Node, 1);
        }
        MemoryZeroStruct(node);
        DLLPushBack(slot->first, slot->last, node);
        String8 path_stored = di_string_alloc__stripe_mutex_w_guarded(stripe, key_normalized.path);
        node->key.path = path_stored;
        node->key.min_timestamp = current_timestamp;
      }
      
      //- rjf: increment node reference count
      if(node != 0)
      {
        node->ref_count += 1;
        if(node->ref_count == 1)
        {
          di_u2p_enqueue_key(&key_normalized, max_U64);
        }
      }
    }
  }
  scratch_end(scratch);
}

internal void
di_close(DI_Key *key)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  if(key->path.size != 0)
  {
    DI_Key key_normalized = di_normalized_key_from_key(scratch.arena, key);
    U64 hash = di_hash_from_key(&key_normalized);
    U64 slot_idx = hash%di_shared->slots_count;
    U64 stripe_idx = slot_idx%di_shared->stripes_count;
    DI_Slot *slot = &di_shared->slots[slot_idx];
    DI_Stripe *stripe = &di_shared->stripes[stripe_idx];
    log_infof("close_debug_info: {\"%S\", 0x%I64x}\n", key_normalized.path, key_normalized.min_timestamp);
    OS_MutexScopeW(stripe->rw_mutex)
    {
      //- rjf: find existing node
      DI_Node *node = di_node_from_key_slot__stripe_mutex_r_guarded(slot, &key_normalized);
      
      //- rjf: node exists -> decrement reference count; release
      if(node != 0)
      {
        node->ref_count -= 1;
        if(node->ref_count == 0) for(;;)
        {
          //- rjf: wait for touch count to go to 0
          if(ins_atomic_u64_eval(&node->touch_count) != 0)
          {
            os_rw_mutex_drop_w(stripe->rw_mutex);
            for(U64 start_t = os_now_microseconds(); os_now_microseconds() <= start_t + 250;);
            os_rw_mutex_take_w(stripe->rw_mutex);
          }
          
          //- rjf: release
          if(node->ref_count == 0 && ins_atomic_u64_eval(&node->touch_count) == 0)
          {
            di_string_release__stripe_mutex_w_guarded(stripe, node->key.path);
            if(node->file_base != 0)
            {
              os_file_map_view_close(node->file_map, node->file_base, r1u64(0, node->file_props.size));
            }
            if(!os_handle_match(node->file_map, os_handle_zero()))
            {
              os_file_map_close(node->file_map);
            }
            if(!os_handle_match(node->file, os_handle_zero()))
            {
              os_file_close(node->file);
            }
            if(node->arena != 0)
            {
              arena_release(node->arena);
            }
            DLLRemove(slot->first, slot->last, node);
            SLLStackPush(stripe->free_node, node);
            break;
          }
        }
      }
    }
  }
  ProfEnd();
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal RDI_Parsed *
di_rdi_from_key(DI_Scope *scope, DI_Key *key, U64 endt_us)
{
  RDI_Parsed *result = &di_rdi_parsed_nil;
  if(key->path.size != 0)
  {
    Temp scratch = scratch_begin(0, 0);
    DI_Key key_normalized = di_normalized_key_from_key(scratch.arena, key);
    U64 hash = di_hash_from_key(&key_normalized);
    U64 slot_idx = hash%di_shared->slots_count;
    U64 stripe_idx = slot_idx%di_shared->stripes_count;
    DI_Slot *slot = &di_shared->slots[slot_idx];
    DI_Stripe *stripe = &di_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      //- rjf: find existing node
      DI_Node *node = di_node_from_key_slot__stripe_mutex_r_guarded(slot, &key_normalized);
      
      //- rjf: no node? this path is not opened
      if(node == 0)
      {
        break;
      }
      
      //- rjf: node refcount == 0? this node is being destroyed
      if(node->ref_count == 0)
      {
        break;
      }
      
      //- rjf: parse done -> touch, grab result
      if(node != 0 && node->parse_done)
      {
        di_scope_touch_node__stripe_mutex_r_guarded(scope, node);
        result = &node->rdi;
        break;
      }
      
      //- rjf: parse not done, not working -> ask for parse
      if(node != 0 &&
         !node->parse_done &&
         ins_atomic_u64_eval(&node->request_count) == ins_atomic_u64_eval(&node->completion_count) &&
         async_push_work(di_parse_work, .endt_us = endt_us, .completion_counter = &node->completion_count))
      {
        di_u2p_enqueue_key(&key_normalized, max_U64);
        ins_atomic_u64_inc_eval(&node->request_count);
      }
      
      //- rjf: time expired -> break
      if(os_now_microseconds() >= endt_us)
      {
        break;
      }
      
      //- rjf: wait on this stripe
      {
        os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
      }
    }
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: Parse Threads

internal B32
di_u2p_enqueue_key(DI_Key *key, U64 endt_us)
{
  B32 sent = 0;
  OS_MutexScope(di_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = di_shared->u2p_ring_write_pos - di_shared->u2p_ring_read_pos;
    U64 available_size = di_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(key->path.size) + key->path.size + sizeof(key->min_timestamp))
    {
      di_shared->u2p_ring_write_pos += ring_write_struct(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_write_pos, &key->path.size);
      di_shared->u2p_ring_write_pos += ring_write(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_write_pos, key->path.str, key->path.size);
      di_shared->u2p_ring_write_pos += ring_write_struct(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_write_pos, &key->min_timestamp);
      di_shared->u2p_ring_write_pos += 7;
      di_shared->u2p_ring_write_pos -= di_shared->u2p_ring_write_pos%8;
      sent = 1;
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(di_shared->u2p_ring_cv, di_shared->u2p_ring_mutex, endt_us);
  }
  if(sent)
  {
    os_condition_variable_broadcast(di_shared->u2p_ring_cv);
  }
  return sent;
}

internal void
di_u2p_dequeue_key(Arena *arena, DI_Key *out_key)
{
  OS_MutexScope(di_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = di_shared->u2p_ring_write_pos - di_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(out_key->path.size) + sizeof(out_key->min_timestamp))
    {
      di_shared->u2p_ring_read_pos += ring_read_struct(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_read_pos, &out_key->path.size);
      out_key->path.str = push_array(arena, U8, out_key->path.size);
      di_shared->u2p_ring_read_pos += ring_read(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_read_pos, out_key->path.str, out_key->path.size);
      di_shared->u2p_ring_read_pos += ring_read_struct(di_shared->u2p_ring_base, di_shared->u2p_ring_size, di_shared->u2p_ring_read_pos, &out_key->min_timestamp);
      di_shared->u2p_ring_read_pos += 7;
      di_shared->u2p_ring_read_pos -= di_shared->u2p_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(di_shared->u2p_ring_cv, di_shared->u2p_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(di_shared->u2p_ring_cv);
}

internal void
di_p2u_push_event(DI_Event *event)
{
  OS_MutexScope(di_shared->p2u_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (di_shared->p2u_ring_write_pos-di_shared->p2u_ring_read_pos);
    U64 available_size = di_shared->p2u_ring_size-unconsumed_size;
    U64 needed_size = sizeof(DI_EventKind) + sizeof(U64) + event->string.size;
    if(available_size >= needed_size)
    {
      di_shared->p2u_ring_write_pos += ring_write_struct(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_write_pos, &event->kind);
      di_shared->p2u_ring_write_pos += ring_write_struct(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_write_pos, &event->string.size);
      di_shared->p2u_ring_write_pos += ring_write(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_write_pos, event->string.str, event->string.size);
      di_shared->p2u_ring_write_pos += 7;
      di_shared->p2u_ring_write_pos -= di_shared->p2u_ring_write_pos%8;
      break;
    }
    os_condition_variable_wait(di_shared->p2u_ring_cv, di_shared->p2u_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(di_shared->p2u_ring_cv);
}

internal DI_EventList
di_p2u_pop_events(Arena *arena, U64 endt_us)
{
  DI_EventList events = {0};
  OS_MutexScope(di_shared->p2u_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (di_shared->p2u_ring_write_pos-di_shared->p2u_ring_read_pos);
    if(unconsumed_size >= sizeof(DI_EventKind) + sizeof(U64))
    {
      DI_EventNode *n = push_array(arena, DI_EventNode, 1);
      SLLQueuePush(events.first, events.last, n);
      events.count += 1;
      di_shared->p2u_ring_read_pos += ring_read_struct(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_read_pos, &n->v.kind);
      di_shared->p2u_ring_read_pos += ring_read_struct(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_read_pos, &n->v.string.size);
      n->v.string.str = push_array_no_zero(arena, U8, n->v.string.size);
      di_shared->p2u_ring_read_pos += ring_read(di_shared->p2u_ring_base, di_shared->p2u_ring_size, di_shared->p2u_ring_read_pos, n->v.string.str, n->v.string.size);
      di_shared->p2u_ring_read_pos += 7;
      di_shared->p2u_ring_read_pos -= di_shared->p2u_ring_read_pos%8;
    }
    else if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(di_shared->p2u_ring_cv, di_shared->p2u_ring_mutex, endt_us);
  }
  os_condition_variable_broadcast(di_shared->p2u_ring_cv);
  return events;
}

ASYNC_WORK_DEF(di_parse_work)
{
  Temp scratch = scratch_begin(0, 0);
  
  ////////////////////////////
  //- rjf: grab next key
  //
  DI_Key key = {0};
  di_u2p_dequeue_key(scratch.arena, &key);
  String8 og_path = key.path;
  U64 min_timestamp = key.min_timestamp;
  
  ////////////////////////////
  //- rjf: unpack key
  //
  U64 hash = di_hash_from_string(og_path, StringMatchFlag_CaseInsensitive);
  U64 slot_idx = hash%di_shared->slots_count;
  U64 stripe_idx = slot_idx%di_shared->stripes_count;
  DI_Slot *slot = &di_shared->slots[slot_idx];
  DI_Stripe *stripe = &di_shared->stripes[stripe_idx];
  
  ////////////////////////////
  //- rjf: take task
  //
  B32 got_task = 0;
  OS_MutexScopeR(stripe->rw_mutex)
  {
    DI_Node *node = di_node_from_key_slot__stripe_mutex_r_guarded(slot, &key);
    if(node != 0)
    {
      got_task = !ins_atomic_u64_eval_cond_assign(&node->is_working, 1, 0);
    }
  }
  
  ////////////////////////////
  //- rjf: got task -> open O.G. file (may or may not be RDI)
  //
  B32 og_format_is_known = 0;
  B32 og_is_pe     = 0;
  B32 og_is_pdb    = 0;
  B32 og_is_elf    = 0;
  B32 og_is_rdi    = 0;
  FileProperties og_props = {0};
  if(got_task) ProfScope("analyze %.*s", str8_varg(og_path))
  {
    OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, og_path);
    OS_Handle file_map = os_file_map_open(OS_AccessFlag_Read, file);
    FileProperties props = og_props = os_properties_from_file(file);
    void *base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, props.size));
    String8 data = str8((U8 *)base, props.size);
    if(!og_format_is_known)
    {
      String8 msf20_magic = str8_lit("Microsoft C/C++ program database 2.00\r\n\x1aJG\0\0");
      String8 msf70_magic = str8_lit("Microsoft C/C++ MSF 7.00\r\n\032DS\0\0");
      String8 msfxx_magic = str8_lit("Microsoft C/C++");
      if((data.size >= msf20_magic.size && str8_match(data, msf20_magic, StringMatchFlag_RightSideSloppy)) ||
         (data.size >= msf70_magic.size && str8_match(data, msf70_magic, StringMatchFlag_RightSideSloppy)) ||
         (data.size >= msfxx_magic.size && str8_match(data, msfxx_magic, StringMatchFlag_RightSideSloppy)))
      {
        og_format_is_known = 1;
        og_is_pdb = 1;
      }
    }
    if(!og_format_is_known)
    {
      if(data.size >= 8 && *(U64 *)data.str == RDI_MAGIC_CONSTANT)
      {
        og_format_is_known = 1;
        og_is_rdi = 1;
      }
    }
    if(!og_format_is_known)
    {
      if(data.size >= 4 &&
         data.str[0] == 0x7f &&
         data.str[1] == 'E' &&
         data.str[2] == 'L' &&
         data.str[3] == 'F')
      {
        og_format_is_known = 1;
        og_is_elf = 1;
      }
    }
    if(!og_format_is_known)
    {
      if(data.size >= 2 && *(U16 *)data.str == 0x5a4d)
      {
        og_format_is_known = 1;
        og_is_pe = 1;
      }
    }
    os_file_map_view_close(file_map, base, r1u64(0, props.size));
    os_file_map_close(file_map);
    os_file_close(file);
  }
  
  ////////////////////////////
  //- rjf: given O.G. path & analysis, determine RDI path
  //
  String8 rdi_path = {0};
  if(got_task)
  {
    if(og_is_rdi)
    {
      rdi_path = og_path;
    }
    else if(og_format_is_known && og_is_pdb)
    {
      rdi_path = push_str8f(scratch.arena, "%S.rdi", str8_chop_last_dot(og_path));
    }
  }
  
  ////////////////////////////
  //- rjf: check if rdi file is up-to-date
  //
  B32 rdi_file_is_up_to_date = 0;
  if(got_task)
  {
    if(rdi_path.size != 0) ProfScope("check %.*s is up-to-date", str8_varg(rdi_path))
    {
      FileProperties props = os_properties_from_file_path(rdi_path);
      rdi_file_is_up_to_date = (props.modified > og_props.modified);
    }
  }
  
  ////////////////////////////
  //- rjf: if raddbg file is up to date based on timestamp, check the
  // encoding generation number & size, to see if we need to regenerate it
  // regardless
  //
  if(got_task && rdi_file_is_up_to_date) ProfScope("check %.*s version matches our's", str8_varg(rdi_path))
  {
    OS_Handle file = {0};
    OS_Handle file_map = {0};
    FileProperties file_props = {0};
    void *file_base = 0;
    file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, rdi_path);
    file_map = os_file_map_open(OS_AccessFlag_Read, file);
    file_props = os_properties_from_file(file);
    file_base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, file_props.size));
    if(sizeof(RDI_Header) <= file_props.size)
    {
      RDI_Header *header = (RDI_Header*)file_base;
      if(header->encoding_version != RDI_ENCODING_VERSION)
      {
        rdi_file_is_up_to_date = 0;
      }
    }
    else
    {
      rdi_file_is_up_to_date = 0;
    }
    os_file_map_view_close(file_map, file_base, r1u64(0, file_props.size));
    os_file_map_close(file_map);
    os_file_close(file);
  }
  
  ////////////////////////////
  //- rjf: heuristically choose compression settings
  //
  B32 should_compress = 0;
#if 0
  if(og_dbg_props.size > MB(64))
  {
    should_compress = 1;
  }
#endif
  
  ////////////////////////////
  //- rjf: rdi file not up-to-date? we need to generate it
  //
  if(got_task && !rdi_file_is_up_to_date) ProfScope("generate %.*s", str8_varg(rdi_path))
  {
    if(og_is_pdb)
    {
      //- rjf: push conversion task begin event
      {
        DI_Event event = {DI_EventKind_ConversionStarted};
        event.string = rdi_path;
        di_p2u_push_event(&event);
      }
      
      //- rjf: kick off process
      OS_Handle process = {0};
      {
        OS_ProcessLaunchParams params = {0};
        params.path = os_get_process_info()->binary_path;
        params.inherit_env = 1;
        params.consoleless = 1;
        str8_list_pushf(scratch.arena, &params.cmd_line, "raddbg");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--convert");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--quiet");
        if(should_compress)
        {
          str8_list_pushf(scratch.arena, &params.cmd_line, "--compress");
        }
        // str8_list_pushf(scratch.arena, &params.cmd_line, "--capture");
        str8_list_pushf(scratch.arena, &params.cmd_line, "--pdb:%S", og_path);
        str8_list_pushf(scratch.arena, &params.cmd_line, "--out:%S", rdi_path);
        process = os_process_launch(&params);
      }
      
      //- rjf: wait for process to complete
      {
        U64 start_wait_t = os_now_microseconds();
        for(;;)
        {
          B32 wait_done = os_process_join(process, os_now_microseconds()+1000);
          if(wait_done)
          {
            rdi_file_is_up_to_date = 1;
            break;
          }
        }
      }
      
      //- rjf: push conversion task end event
      {
        DI_Event event = {DI_EventKind_ConversionEnded};
        event.string = rdi_path;
        di_p2u_push_event(&event);
      }
    }
    else
    {
      // NOTE(rjf): we cannot convert from this O.G. debug info format right now.
      //- rjf: push conversion task failure event
      {
        DI_Event event = {DI_EventKind_ConversionFailureUnsupportedFormat};
        event.string = rdi_path;
        di_p2u_push_event(&event);
      }
    }
  }
  
  ////////////////////////////
  //- rjf: got task -> open file
  //
  OS_Handle file = {0};
  OS_Handle file_map = {0};
  FileProperties file_props = {0};
  void *file_base = 0;
  if(got_task)
  {
    file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead|OS_AccessFlag_ShareWrite, rdi_path);
    file_map = os_file_map_open(OS_AccessFlag_Read, file);
    file_props = os_properties_from_file(file);
    file_base = os_file_map_view_open(file_map, OS_AccessFlag_Read, r1u64(0, file_props.size));
  }
  
  ////////////////////////////
  //- rjf: do initial parse of rdi
  //
  RDI_Parsed rdi_parsed_maybe_compressed = di_rdi_parsed_nil;
  if(got_task)
  {
    RDI_ParseStatus parse_status = rdi_parse((U8 *)file_base, file_props.size, &rdi_parsed_maybe_compressed);
    (void)parse_status;
  }
  
  ////////////////////////////
  //- rjf: decompress & re-parse, if necessary
  //
  Arena *rdi_parsed_arena = 0;
  RDI_Parsed rdi_parsed = rdi_parsed_maybe_compressed;
  if(got_task)
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
  
  ////////////////////////////
  //- rjf: commit parsed info to cache
  //
  if(got_task) OS_MutexScopeW(stripe->rw_mutex)
  {
    DI_Node *node = di_node_from_key_slot__stripe_mutex_r_guarded(slot, &key);
    if(node != 0)
    {
      node->is_working = 0;
      node->file = file;
      node->file_map = file_map;
      node->file_base = file_base;
      node->file_props = file_props;
      node->arena = rdi_parsed_arena;
      node->rdi = rdi_parsed;
      node->parse_done = 1;
    }
  }
  os_condition_variable_broadcast(stripe->cv);
  
  scratch_end(scratch);
  return 0;
}
