// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
di_hash_from_seed_string(U64 seed, String8 string, StringMatchFlags match_flags)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + ((match_flags & StringMatchFlag_CaseInsensitive) ? char_to_lower(string.str[i]) : string.str[i]);
  }
  return result;
}

internal U64
di_hash_from_string(String8 string, StringMatchFlags match_flags)
{
  U64 hash = di_hash_from_seed_string(5381, string, match_flags);
  return hash;
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

internal DI_KeyArray
di_key_array_copy(Arena *arena, DI_KeyArray *src)
{
  DI_KeyArray dst = {0};
  dst.count = src->count;
  dst.v = push_array(arena, DI_Key, dst.count);
  for EachIndex(idx, dst.count)
  {
    dst.v[idx] = di_key_copy(arena, &src->v[idx]);
  }
  return dst;
}

internal DI_SearchParams
di_search_params_copy(Arena *arena, DI_SearchParams *src)
{
  DI_SearchParams dst = {0};
  MemoryCopyStruct(&dst, src);
  dst.dbgi_keys = di_key_array_copy(arena, &dst.dbgi_keys);
  return dst;
}

internal U64
di_hash_from_search_params(DI_SearchParams *params)
{
  U64 hash = 5381;
  hash = di_hash_from_seed_string(hash, str8_struct(&params->target), 0);
  for(U64 idx = 0; idx < params->dbgi_keys.count; idx += 1)
  {
    hash = di_hash_from_seed_string(hash, str8_struct(&params->dbgi_keys.v[idx].min_timestamp), 0);
    hash = di_hash_from_seed_string(hash, params->dbgi_keys.v[idx].path, StringMatchFlag_CaseInsensitive);
  }
  return hash;
}

internal void
di_search_item_chunk_list_concat_in_place(DI_SearchItemChunkList *dst, DI_SearchItemChunkList *to_push)
{
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

internal U64
di_search_item_num_from_array_element_idx__linear_search(DI_SearchItemArray *array, U64 element_idx)
{
  U64 fuzzy_item_num = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    if(array->v[idx].idx == element_idx)
    {
      fuzzy_item_num = idx+1;
      break;
    }
  }
  return fuzzy_item_num;
}

internal String8
di_search_item_string_from_rdi_target_element_idx(RDI_Parsed *rdi, RDI_SectionKind target, U64 element_idx)
{
  String8 result = {0};
  switch(target)
  {
    default:{}break;
    case RDI_SectionKind_Procedures:
    {
      RDI_Procedure *proc = rdi_element_from_name_idx(rdi, Procedures, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, proc->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case RDI_SectionKind_GlobalVariables:
    {
      RDI_GlobalVariable *gvar = rdi_element_from_name_idx(rdi, GlobalVariables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, gvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case RDI_SectionKind_ThreadVariables:
    {
      RDI_ThreadVariable *tvar = rdi_element_from_name_idx(rdi, ThreadVariables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, tvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case RDI_SectionKind_UDTs:
    {
      RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, element_idx);
      RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, type_node->user_defined.name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
  }
  return result;
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
  di_shared->search_slots_count = 512;
  di_shared->search_slots = push_array(arena, DI_SearchSlot, di_shared->search_slots_count);
  di_shared->search_stripes_count = Min(di_shared->search_slots_count, os_get_system_info()->logical_processor_count);
  di_shared->search_stripes = push_array(arena, DI_SearchStripe, di_shared->search_stripes_count);
  for(U64 idx = 0; idx < di_shared->search_stripes_count; idx += 1)
  {
    di_shared->search_stripes[idx].arena = arena_alloc();
    di_shared->search_stripes[idx].rw_mutex = os_rw_mutex_alloc();
    di_shared->search_stripes[idx].cv = os_condition_variable_alloc();
  }
  di_shared->u2p_ring_mutex = os_mutex_alloc();
  di_shared->u2p_ring_cv = os_condition_variable_alloc();
  di_shared->u2p_ring_size = KB(64);
  di_shared->u2p_ring_base = push_array_no_zero(arena, U8, di_shared->u2p_ring_size);
  di_shared->p2u_ring_mutex = os_mutex_alloc();
  di_shared->p2u_ring_cv = os_condition_variable_alloc();
  di_shared->p2u_ring_size = KB(64);
  di_shared->p2u_ring_base = push_array_no_zero(arena, U8, di_shared->p2u_ring_size);
  di_shared->search_threads_count = 1;
  di_shared->search_threads = push_array(arena, DI_SearchThread, di_shared->search_threads_count);
  for EachIndex(idx, di_shared->search_threads_count)
  {
    di_shared->search_threads[idx].ring_mutex = os_mutex_alloc();
    di_shared->search_threads[idx].ring_cv    = os_condition_variable_alloc();
    di_shared->search_threads[idx].ring_size  = KB(64);
    di_shared->search_threads[idx].ring_base  = push_array_no_zero(arena, U8, di_shared->search_threads[idx].ring_size);
    di_shared->search_threads[idx].thread = os_thread_launch(di_search_thread__entry_point, (void *)idx, 0);
  }
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
    if(t->search_node != 0)
    {
      ins_atomic_u64_dec_eval(&t->search_node->scope_refcount);
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

internal void
di_scope_touch_search_node__stripe_mutex_r_guarded(DI_Scope *scope, DI_SearchNode *node)
{
  if(node != 0)
  {
    ins_atomic_u64_inc_eval(&node->scope_refcount);
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
  touch->search_node = node;
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
          async_push_work(di_parse_work);
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
//~ rjf: Debug Info Cache Lookups

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
         di_u2p_enqueue_key(&key_normalized, endt_us))
      {
        ins_atomic_u64_inc_eval(&node->request_count);
        async_push_work(di_parse_work, .completion_counter = &node->completion_count);
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
//~ rjf: Search Cache Lookups

internal DI_SearchItemArray
di_search_items_from_key_params_query(DI_Scope *scope, U128 key, DI_SearchParams *params, String8 query, U64 endt_us, B32 *stale_out)
{
  DI_SearchItemArray items = {0};
  {
    U64 params_hash = di_hash_from_search_params(params);
    U64               slot_idx   = key.u64[0]%di_shared->search_slots_count;
    U64               stripe_idx = slot_idx%di_shared->search_stripes_count;
    DI_SearchSlot *   slot       = &di_shared->search_slots[slot_idx];
    DI_SearchStripe * stripe     = &di_shared->search_stripes[stripe_idx];
    OS_MutexScopeW(stripe->rw_mutex) for(;;)
    {
      // rjf: map key -> node
      DI_SearchNode *node = 0;
      for(DI_SearchNode *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->key, key))
        {
          node = n;
          break;
        }
      }
      
      // rjf: no node? -> allocate
      if(node == 0)
      {
        node = push_array(stripe->arena, DI_SearchNode, 1);
        SLLQueuePush(slot->first, slot->last, node);
        node->key = key;
        for(U64 idx = 0; idx < ArrayCount(node->buckets); idx += 1)
        {
          node->buckets[idx].arena = arena_alloc();
        }
      }
      
      // rjf: record update idx info
      ins_atomic_u64_eval_assign(&node->last_update_tick_idx, update_tick_idx());
      
      // rjf: try to grab last valid results for this key/query; determine if stale
      B32 stale = 1;
      if(params_hash == node->buckets[node->bucket_read_gen%ArrayCount(node->buckets)].params_hash &&
         node->bucket_read_gen != 0)
      {
        di_scope_touch_search_node__stripe_mutex_r_guarded(scope, node);
        items = node->items;
        stale = !str8_match(query, node->buckets[node->bucket_read_gen%ArrayCount(node->buckets)].query, 0);
      }
      if(stale_out != 0)
      {
        *stale_out = stale;
      }
      
      // rjf: if stale -> request again
      if(stale && node->bucket_read_gen <= node->bucket_write_gen && node->bucket_write_gen < node->bucket_read_gen + ArrayCount(node->buckets)-1)
      {
        node->bucket_write_gen += 1;
        if(node->bucket_write_gen >= node->bucket_items_gen + ArrayCount(node->buckets))
        {
          MemoryZeroStruct(&node->items);
          MemoryZeroStruct(&items);
        }
        U64 new_bucket_idx = node->bucket_write_gen%ArrayCount(node->buckets);
        arena_clear(node->buckets[new_bucket_idx].arena);
        node->buckets[new_bucket_idx].query = push_str8_copy(node->buckets[new_bucket_idx].arena, query);
        node->buckets[new_bucket_idx].params = di_search_params_copy(node->buckets[new_bucket_idx].arena, params);
        node->buckets[new_bucket_idx].params_hash = params_hash;
        di_u2s_enqueue_req(key, endt_us);
      }
      
      // rjf: not stale, or timeout -> break
      if(!stale || os_now_microseconds() >= endt_us)
      {
        break;
      }
      
      // rjf: no results, but have time to wait -> wait
      os_condition_variable_wait_rw_w(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  return items;
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
  ProfBeginFunction();
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
  ProfEnd();
  return 0;
}

////////////////////////////////
//~ rjf: Search Threads

internal B32
di_u2s_enqueue_req(U128 key, U64 endt_us)
{
  B32 result = 0;
  U64 thread_idx = key.u64[0]%di_shared->search_threads_count;
  DI_SearchThread *thread = &di_shared->search_threads[thread_idx];
  OS_MutexScope(thread->ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->ring_write_pos - thread->ring_read_pos;
    U64 available_size = thread->ring_size - unconsumed_size;
    if(available_size >= sizeof(key))
    {
      result = 1;
      thread->ring_write_pos += ring_write_struct(thread->ring_base, thread->ring_size, thread->ring_write_pos, &key);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(thread->ring_cv, thread->ring_mutex, endt_us);
  }
  if(result)
  {
    os_condition_variable_broadcast(thread->ring_cv);
  }
  return result;
}

internal U128
di_u2s_dequeue_req(U64 thread_idx)
{
  U128 key = {0};
  DI_SearchThread *thread = &di_shared->search_threads[thread_idx];
  OS_MutexScope(thread->ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->ring_write_pos - thread->ring_read_pos;
    if(unconsumed_size >= sizeof(key))
    {
      thread->ring_read_pos += ring_read_struct(thread->ring_base, thread->ring_size, thread->ring_read_pos, &key);
      break;
    }
    os_condition_variable_wait(thread->ring_cv, thread->ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(thread->ring_cv);
  return key;
}

typedef struct DI_SearchWorkIn DI_SearchWorkIn;
struct DI_SearchWorkIn
{
  U128 key;
  U64 initial_bucket_write_gen;
  Arena **work_thread_arenas;
  RDI_Parsed *rdi;
  RDI_SectionKind section_kind;
  Rng1U64 element_range;
  String8 query;
  U64 dbgi_idx;
};
typedef struct DI_SearchWorkOut DI_SearchWorkOut;
struct DI_SearchWorkOut
{
  B32 cancelled;
  DI_SearchItemChunkList items;
};
ASYNC_WORK_DEF(di_search_work)
{
  ProfBeginFunction();
  
  //- rjf: unpack parameters
  DI_SearchWorkIn *in = (DI_SearchWorkIn *)input;
  if(in->work_thread_arenas[thread_idx] == 0)
  {
    in->work_thread_arenas[thread_idx] = arena_alloc();
  }
  Arena *arena = in->work_thread_arenas[thread_idx];
  U128 key = in->key;
  U64               slot_idx   = key.u64[0]%di_shared->search_slots_count;
  U64               stripe_idx = slot_idx%di_shared->search_stripes_count;
  DI_SearchSlot *   slot       = &di_shared->search_slots[slot_idx];
  DI_SearchStripe * stripe     = &di_shared->search_stripes[stripe_idx];
  
  //- rjf: setup output
  DI_SearchWorkOut *out = push_array(arena, DI_SearchWorkOut, 1);
  
  //- rjf: unpack table info
  U64 element_count = 0;
  void *table_base = rdi_section_raw_table_from_kind(in->rdi, in->section_kind, &element_count);
  U64 element_size = rdi_section_element_size_table[in->section_kind];
  
  //- rjf: determine name string index offset, depending on table kind
  U64 element_name_idx_off = 0;
  switch(in->section_kind)
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
  }
  
  //- rjf: loop through table, gather matches
  B32 cancelled = 0;
  for(U64 idx = in->element_range.min; (idx < in->element_range.max && idx < element_count); idx += 1)
  {
    //- rjf: every so often, check the key's write gen - if it has been bumped, then cancel
    if(idx%100 == 0)
    {
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(DI_SearchNode *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key) && n->bucket_write_gen != in->initial_bucket_write_gen)
          {
            cancelled = 1;
            break;
          }
        }
      }
    }
    if(cancelled)
    {
      break;
    }
    
    //- rjf: get element, map to string; if empty, continue to next element
    void *element = (U8 *)table_base + element_size*idx;
    U32 *name_idx_ptr = (U32 *)((U8 *)element + element_name_idx_off);
    if(in->section_kind == RDI_SectionKind_UDTs)
    {
      RDI_UDT *udt = (RDI_UDT *)element;
      RDI_TypeNode *type_node = rdi_element_from_name_idx(in->rdi, TypeNodes, udt->self_type_idx);
      name_idx_ptr = &type_node->user_defined.name_string_idx;
    }
    U32 name_idx = *name_idx_ptr;
    U64 name_size = 0;
    U8 *name_base = rdi_string_from_idx(in->rdi, name_idx, &name_size);
    String8 name = str8(name_base, name_size);
    if(name.size == 0) { continue; }
    
    //- rjf: fuzzy match against query
    FuzzyMatchRangeList matches = fuzzy_match_find(arena, in->query, name);
    
    //- rjf: collect
    if(matches.count == matches.needle_part_count)
    {
      DI_SearchItemChunk *chunk = out->items.last;
      if(chunk == 0 || chunk->count >= chunk->cap)
      {
        chunk = push_array(arena, DI_SearchItemChunk, 1);
        chunk->cap = 1024;
        chunk->count = 0;
        chunk->v = push_array_no_zero(arena, DI_SearchItem, chunk->cap);
        SLLQueuePush(out->items.first, out->items.last, chunk);
        out->items.chunk_count += 1;
      }
      chunk->v[chunk->count].idx          = idx;
      chunk->v[chunk->count].dbgi_idx     = in->dbgi_idx;
      chunk->v[chunk->count].match_ranges = matches;
      chunk->v[chunk->count].missed_size  = (name_size > matches.total_dim) ? (name_size-matches.total_dim) : 0;
      chunk->count += 1;
      out->items.total_count += 1;
    }
  }
  out->cancelled = cancelled;
  ProfEnd();
  return out;
}

internal int
di_qsort_compare_search_items(DI_SearchItem *a, DI_SearchItem *b)
{
  int result = 0;
  if(a->match_ranges.count > b->match_ranges.count)
  {
    result = -1;
  }
  else if(a->match_ranges.count < b->match_ranges.count)
  {
    result = +1;
  }
  else if(a->missed_size < b->missed_size)
  {
    result = -1;
  }
  else if(a->missed_size > b->missed_size)
  {
    result = +1;
  }
  return result;
}

internal void
di_search_thread__entry_point(void *p)
{
  U64 thread_idx = (U64)p;
  ThreadNameF("[di] search thread #%I64u", thread_idx);
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    DI_Scope *di_scope = di_scope_open();
    
    //- rjf: get next key, unpack
    U128 key = di_u2s_dequeue_req(thread_idx);
    U64               slot_idx   = key.u64[0]%di_shared->search_slots_count;
    U64               stripe_idx = slot_idx%di_shared->search_stripes_count;
    DI_SearchSlot *   slot       = &di_shared->search_slots[slot_idx];
    DI_SearchStripe * stripe     = &di_shared->search_stripes[stripe_idx];
    
    //- rjf: map key -> output arena & search parameters
    Arena *arena = 0;
    String8 query = {0};
    DI_SearchParams params = {0};
    U64 initial_bucket_write_gen = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(DI_SearchNode *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->key, key))
        {
          U64 bucket_idx = n->bucket_write_gen%ArrayCount(n->buckets);
          arena  = n->buckets[bucket_idx].arena;
          query  = push_str8_copy(scratch.arena, n->buckets[bucket_idx].query);
          params = di_search_params_copy(scratch.arena, &n->buckets[bucket_idx].params);
          initial_bucket_write_gen = n->bucket_write_gen;
          break;
        }
      }
    }
    
    //- rjf: get all rdis
    U64 rdis_count = params.dbgi_keys.count;
    RDI_Parsed **rdis = push_array(scratch.arena, RDI_Parsed *, rdis_count);
    for EachIndex(idx, rdis_count)
    {
      rdis[idx] = di_rdi_from_key(di_scope, &params.dbgi_keys.v[idx], max_U64);
    }
    
    //- rjf: kick off search tasks
    ASYNC_TaskList tasks = {0};
    Arena **work_thread_arenas = 0;
    if(arena != 0)
    {
      U64 elements_per_task = 16384;
      work_thread_arenas = push_array(arena, Arena *, async_thread_count());
      for EachIndex(idx, rdis_count)
      {
        RDI_Parsed *rdi = rdis[idx];
        U64 element_count_in_this_rdi = 0;
        rdi_section_raw_table_from_kind(rdi, params.target, &element_count_in_this_rdi);
        U64 tasks_per_this_rdi = (element_count_in_this_rdi+elements_per_task-1)/elements_per_task;
        for(U64 task_in_this_rdi_idx = 0; task_in_this_rdi_idx < tasks_per_this_rdi; task_in_this_rdi_idx += 1)
        {
          DI_SearchWorkIn *in = push_array(scratch.arena, DI_SearchWorkIn, 1);
          in->key                      = key;
          in->initial_bucket_write_gen = initial_bucket_write_gen;
          in->work_thread_arenas       = work_thread_arenas;
          in->rdi                      = rdi;
          in->section_kind             = params.target;
          in->element_range            = r1u64(task_in_this_rdi_idx*elements_per_task, (task_in_this_rdi_idx+1)*elements_per_task);
          in->element_range.max        = ClampTop(in->element_range.max, element_count_in_this_rdi);
          in->query                    = query;
          in->dbgi_idx                 = idx;
          async_task_list_push(scratch.arena, &tasks, async_task_launch(scratch.arena, di_search_work, .input = in));
        }
      }
    }
    
    //- rjf: join tasks, form final list
    B32 cancelled = 0;
    DI_SearchItemChunkList items_list = {0};
    for(ASYNC_TaskNode *n = tasks.first; n != 0; n = n->next)
    {
      DI_SearchWorkOut *out = async_task_join_struct(n->v, DI_SearchWorkOut);
      di_search_item_chunk_list_concat_in_place(&items_list, &out->items);
      cancelled = (cancelled || out->cancelled);
    }
    
    //- rjf: list -> array
    DI_SearchItemArray items = {0};
    if(!cancelled)
    {
      items.count = items_list.total_count;
      items.v = push_array(arena, DI_SearchItem, items.count);
      U64 off = 0;
      for(DI_SearchItemChunk *chunk = items_list.first; chunk != 0; chunk = chunk->next)
      {
        MemoryCopy(items.v + off, chunk->v, sizeof(chunk->v[0])*chunk->count);
        for EachIndex(idx, chunk->count)
        {
          items.v[off + idx].match_ranges = fuzzy_match_range_list_copy(arena, &items.v[off + idx].match_ranges);
        }
        off += chunk->count;
      }
    }
    
    //- rjf: release all search work artifact arenas
    if(work_thread_arenas != 0)
    {
      for EachIndex(idx, async_thread_count())
      {
        if(work_thread_arenas[idx] != 0)
        {
          arena_release(work_thread_arenas[idx]);
        }
      }
    }
    
    //- rjf: array -> sorted array
    if(items.count != 0 && query.size != 0)
    {
      quick_sort(items.v, items.count, sizeof(DI_SearchItem), di_qsort_compare_search_items);
    }
    
    //- rjf: commit to cache - busyloop on scope touches
    if(arena != 0)
    {
      for(B32 done = 0; !done;)
      {
        B32 found = 0;
        OS_MutexScopeW(stripe->rw_mutex) for(DI_SearchNode *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key))
          {
            if(n->scope_refcount == 0)
            {
              n->bucket_read_gen += 1;
              if(!cancelled)
              {
                n->items = items;
                n->bucket_items_gen = initial_bucket_write_gen;
              }
              done = 1;
            }
            found = 1;
            break;
          }
        }
        if(!found)
        {
          break;
        }
      }
    }
    
    di_scope_close(di_scope);
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Match Store

internal DI_MatchStore *
di_match_store_alloc(void)
{
  Arena *arena = arena_alloc();
  DI_MatchStore *store = push_array(arena, DI_MatchStore, 1);
  store->arena                  = arena;
  for EachElement(idx, store->gen_arenas)
  {
    store->gen_arenas[idx] = arena_alloc();
  }
  store->params_arena           = arena_alloc();
  store->params_rw_mutex        = os_rw_mutex_alloc();
  store->match_name_slots_count = 4096;
  store->match_name_slots       = push_array(arena, DI_MatchNameSlot, store->match_name_slots_count);
  store->u2m_ring_cv            = os_condition_variable_alloc();
  store->u2m_ring_mutex         = os_mutex_alloc();
  store->u2m_ring_size          = KB(2);
  store->u2m_ring_base          = push_array_no_zero(arena, U8, store->u2m_ring_size);
  store->m2u_ring_cv            = os_condition_variable_alloc();
  store->m2u_ring_mutex         = os_mutex_alloc();
  store->m2u_ring_size          = KB(2);
  store->m2u_ring_base          = push_array_no_zero(arena, U8, store->m2u_ring_size);
  return store;
}

internal void
di_match_store_begin(DI_MatchStore *store, DI_KeyArray keys)
{
  store->gen += 1;
  arena_clear(store->gen_arenas[store->gen%ArrayCount(store->gen_arenas)]);
  
  // rjf: hash parameters
  U64 params_hash = 5381;
  for EachIndex(idx, keys.count)
  {
    params_hash = di_hash_from_seed_string(params_hash, str8_struct(&keys.v[idx].min_timestamp), 0);
    params_hash = di_hash_from_seed_string(params_hash, keys.v[idx].path, StringMatchFlag_CaseInsensitive);
  }
  
  // rjf: store parameters if needed
  if(store->params_hash != params_hash) OS_MutexScopeW(store->params_rw_mutex)
  {
    arena_clear(store->params_arena);
    store->params_hash = params_hash;
    store->params_keys = di_key_array_copy(store->params_arena, &keys);
  }
  
  // rjf: prune least recently used matches
  {
    for(DI_MatchNameNode *node = store->last_lru_match_name, *prev = 0; node != 0; node = prev)
    {
      prev = node->prev;
      if(node->last_gen_touched+64 < store->gen)
      {
        U64 slot_idx = node->hash%store->match_name_slots_count;
        DI_MatchNameSlot *slot = &store->match_name_slots[slot_idx];
        DLLRemove_NP(store->first_lru_match_name, store->last_lru_match_name, node, lru_next, lru_prev);
        DLLRemove(slot->first, slot->last, node);
        SLLStackPush(store->first_free_match_name, node);
        store->active_match_name_nodes_count -= 1;
      }
      else
      {
        break;
      }
    }
  }
}

internal RDI_SectionKind
di_match_store_section_kind_from_name(DI_MatchStore *store, String8 name, U64 endt_us)
{
  RDI_SectionKind result = 0;
  {
    // rjf: unpack name
    U64 hash = di_hash_from_string(name, 0);
    U64 slot_idx = hash%store->match_name_slots_count;
    DI_MatchNameSlot *slot = &store->match_name_slots[slot_idx];
    
    // rjf: get name's node, if it exists
    DI_MatchNameNode *node = 0;
    for(DI_MatchNameNode *n = slot->first; n != 0; n = n->next)
    {
      if(n->hash == hash && str8_match(n->name, name, 0))
      {
        node = n;
        break;
      }
    }
    
    // rjf: if node does not exist, create
    if(node == 0)
    {
      node = store->first_free_match_name;
      if(node)
      {
        SLLStackPop(store->first_free_match_name);
      }
      else
      {
        node = push_array_no_zero(store->arena, DI_MatchNameNode, 1);
      }
      MemoryZeroStruct(node);
      node->hash = hash;
      DLLPushBack(slot->first, slot->last, node);
      node->first_gen_touched = store->gen;
      DLLInsert_NP(store->first_lru_match_name, store->last_lru_match_name, (DI_MatchNameNode *)0, node, lru_next, lru_prev);
      store->active_match_name_nodes_count += 1;
    }
    
    // rjf: touch node for this gen
    node->last_gen_touched = store->gen;
    node->name = push_str8_copy(store->gen_arenas[store->gen%ArrayCount(store->gen_arenas)], name);
    DLLRemove_NP(store->first_lru_match_name, store->last_lru_match_name, node, lru_next, lru_prev);
    DLLInsert_NP(store->first_lru_match_name, store->last_lru_match_name, (DI_MatchNameNode *)0, node, lru_next, lru_prev);
    
    // rjf: if this node is new, request it for the given parameters
    if(node->req_params_hash != store->params_hash)
    {
      B32 sent = 0;
      OS_MutexScope(store->u2m_ring_mutex) for(;;)
      {
        U64 unconsumed_size = store->u2m_ring_write_pos - store->u2m_ring_read_pos;
        U64 available_size = store->u2m_ring_size - unconsumed_size;
        if(available_size >= sizeof(U64) + name.size)
        {
          store->u2m_ring_write_pos += ring_write_struct(store->u2m_ring_base, store->u2m_ring_size, store->u2m_ring_write_pos, &name.size);
          store->u2m_ring_write_pos +=        ring_write(store->u2m_ring_base, store->u2m_ring_size, store->u2m_ring_write_pos, name.str, name.size);
          store->u2m_ring_write_pos += 7;
          store->u2m_ring_write_pos -= store->u2m_ring_write_pos%8;
          sent = 1;
          break;
        }
        if(os_now_microseconds() >= endt_us)
        {
          break;
        }
        os_condition_variable_wait(store->u2m_ring_cv, store->u2m_ring_mutex, endt_us);
      }
      if(sent)
      {
        os_condition_variable_broadcast(store->u2m_ring_cv);
        async_push_work(di_match_work, .input = store);
        node->req_params_hash = store->params_hash;
      }
    }
    
    // rjf: if this node's state is stale, consume results from match work & store them
    if(node->req_params_hash != node->cmp_params_hash)
    {
      for(B32 done = 0; !done;)
      {
        Temp scratch = scratch_begin(0, 0);
        
        // rjf: get next result
        String8 result_name = {0};
        U64 result_params_hash = 0;
        U64 result_dbgi_idx = 0;
        RDI_SectionKind result_section_kind = RDI_SectionKind_NULL;
        U32 result_idx = 0;
        OS_MutexScope(store->m2u_ring_mutex) for(;;)
        {
          U64 unconsumed_size = store->m2u_ring_write_pos - store->m2u_ring_read_pos;
          if(unconsumed_size >= sizeof(U64)*4)
          {
            store->m2u_ring_read_pos += ring_read_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, &result_name.size);
            result_name.str = push_array(scratch.arena, U8, result_name.size);
            store->m2u_ring_read_pos += ring_read(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, result_name.str, result_name.size);
            store->m2u_ring_read_pos += 7;
            store->m2u_ring_read_pos -= store->m2u_ring_read_pos%8;
            store->m2u_ring_read_pos += ring_read_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, &result_params_hash);
            store->m2u_ring_read_pos += ring_read_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, &result_dbgi_idx);
            store->m2u_ring_read_pos += ring_read_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, &result_section_kind);
            store->m2u_ring_read_pos += ring_read_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_read_pos, &result_idx);
            break;
          }
          if(os_now_microseconds() >= endt_us)
          {
            break;
          }
          os_condition_variable_wait(store->m2u_ring_cv, store->m2u_ring_mutex, endt_us);
        }
        os_condition_variable_broadcast(store->m2u_ring_cv);
        
        // rjf: store result
        U64 result_hash = di_hash_from_string(result_name, 0);
        U64 result_slot_idx = result_hash%store->match_name_slots_count;
        DI_MatchNameSlot *result_slot = &store->match_name_slots[slot_idx];
        for(DI_MatchNameNode *n = result_slot->first; n != 0; n = n->next)
        {
          if(n->hash == result_hash && str8_match(result_name, n->name, 0))
          {
            n->cmp_params_hash = result_params_hash;
            n->section_kind = result_section_kind;
            break;
          }
        }
        
        // rjf: we're done if we got the hash we were looking for
        if(result_hash == hash && result_params_hash == store->params_hash)
        {
          done = 1;
        }
        
        // rjf: we're done if we're out of time
        if(os_now_microseconds() >= endt_us)
        {
          done = 1;
        }
        
        scratch_end(scratch);
      }
    }
    
    // rjf: return node present info
    result = node->section_kind;
  }
  return result;
}

ASYNC_WORK_DEF(di_match_work)
{
  Temp scratch = scratch_begin(0, 0);
  DI_MatchStore *store = (DI_MatchStore *)input;
  {
    //- rjf: get next name
    String8 name = {0};
    OS_MutexScope(store->u2m_ring_mutex) for(;;)
    {
      U64 unconsumed_size = store->u2m_ring_write_pos - store->u2m_ring_read_pos;
      if(unconsumed_size >= sizeof(U64))
      {
        store->u2m_ring_read_pos += ring_read_struct(store->u2m_ring_base, store->u2m_ring_size, store->u2m_ring_read_pos, &name.size);
        name.str = push_array(scratch.arena, U8, name.size);
        store->u2m_ring_read_pos += ring_read(store->u2m_ring_base, store->u2m_ring_size, store->u2m_ring_read_pos, name.str, name.size);
        store->u2m_ring_read_pos += 7;
        store->u2m_ring_read_pos -= store->u2m_ring_read_pos%8;
        break;
      }
      os_condition_variable_wait(store->u2m_ring_cv, store->u2m_ring_mutex, max_U64);
    }
    os_condition_variable_broadcast(store->u2m_ring_cv);
    
    //- rjf: read parameters
    U64 params_hash = 0;
    DI_KeyArray params_keys = {0};
    OS_MutexScopeR(store->params_rw_mutex)
    {
      params_keys = di_key_array_copy(scratch.arena, &store->params_keys);
      params_hash = store->params_hash;
    }
    
    //- rjf: do match
    RDI_NameMapKind name_map_kinds[] =
    {
      RDI_NameMapKind_GlobalVariables,
      RDI_NameMapKind_ThreadVariables,
      RDI_NameMapKind_Procedures,
      RDI_NameMapKind_Types,
    };
    RDI_SectionKind name_map_section_kinds[] =
    {
      RDI_SectionKind_GlobalVariables,
      RDI_SectionKind_ThreadVariables,
      RDI_SectionKind_Procedures,
      RDI_SectionKind_TypeNodes,
    };
    for EachIndex(dbgi_idx, params_keys.count)
    {
      DI_Scope *di_scope = di_scope_open();
      DI_Key key = params_keys.v[dbgi_idx];
      RDI_Parsed *rdi = di_rdi_from_key(di_scope, &key, max_U64);
      for EachElement(name_map_kind_idx, name_map_kinds)
      {
        RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, name_map_kinds[name_map_kind_idx]);
        RDI_ParsedNameMap parsed_name_map = {0};
        rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, name.str, name.size);
        U32 num = 0;
        U32 *run = rdi_matches_from_map_node(rdi, node, &num);
        for(U32 run_idx = 0; run_idx < num; run_idx += 1)
        {
          OS_MutexScope(store->m2u_ring_mutex) for(;;)
          {
            U64 unconsumed_size = store->m2u_ring_write_pos - store->m2u_ring_read_pos;
            U64 available_size  = store->m2u_ring_size - unconsumed_size;
            U64 needed_size     = 0;
            needed_size += sizeof(U64);
            needed_size += name.size;
            needed_size += 7;
            needed_size -= needed_size%8;
            needed_size += sizeof(U64);
            needed_size += sizeof(U64);
            needed_size += sizeof(RDI_SectionKind);
            needed_size += sizeof(U32);
            if(available_size >= needed_size)
            {
              store->m2u_ring_write_pos += ring_write_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, &name.size);
              store->m2u_ring_write_pos += ring_write(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, name.str, name.size);
              store->m2u_ring_write_pos += 7;
              store->m2u_ring_write_pos -= store->m2u_ring_write_pos%8;
              store->m2u_ring_write_pos += ring_write_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, &params_hash);
              store->m2u_ring_write_pos += ring_write_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, &dbgi_idx);
              store->m2u_ring_write_pos += ring_write_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, &name_map_section_kinds[name_map_kind_idx]);
              store->m2u_ring_write_pos += ring_write_struct(store->m2u_ring_base, store->m2u_ring_size, store->m2u_ring_write_pos, &run[run_idx]);
              break;
            }
            os_condition_variable_wait(store->m2u_ring_cv, store->m2u_ring_mutex, max_U64);
          }
          os_condition_variable_broadcast(store->m2u_ring_cv);
        }
      }
      di_scope_close(di_scope);
    }
  }
  scratch_end(scratch);
  return 0;
}
