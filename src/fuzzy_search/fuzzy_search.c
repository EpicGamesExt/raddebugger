// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal U64
fzy_hash_from_string(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
fzy_hash_from_params(FZY_Params *params)
{
  U64 hash = 5381;
  hash = fzy_hash_from_string(hash, str8_struct(&params->target));
  for(U64 idx = 0; idx < params->dbgi_keys.count; idx += 1)
  {
    hash = fzy_hash_from_string(hash, str8_struct(&params->dbgi_keys.v[idx].min_timestamp));
    hash = fzy_hash_from_string(hash, params->dbgi_keys.v[idx].path);
  }
  return hash;
}

internal U64
fzy_item_num_from_array_element_idx__linear_search(FZY_ItemArray *array, U64 element_idx)
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
fzy_item_string_from_rdi_target_element_idx(RDI_Parsed *rdi, FZY_Target target, U64 element_idx)
{
  String8 result = {0};
  switch(target)
  {
    // NOTE(rjf): no default - warn if we miss a case
    case FZY_Target_Procedures:
    {
      RDI_Procedure *proc = rdi_element_from_name_idx(rdi, Procedures, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, proc->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case FZY_Target_GlobalVariables:
    {
      RDI_GlobalVariable *gvar = rdi_element_from_name_idx(rdi, GlobalVariables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, gvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case FZY_Target_ThreadVariables:
    {
      RDI_ThreadVariable *tvar = rdi_element_from_name_idx(rdi, ThreadVariables, element_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, tvar->name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case FZY_Target_UDTs:
    {
      RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, element_idx);
      RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, type_node->user_defined.name_string_idx, &name_size);
      result = str8(name_base, name_size);
    }break;
    case FZY_Target_COUNT:{}break;
  }
  return result;
}

internal FZY_Params
fzy_params_copy(Arena *arena, FZY_Params *src)
{
  FZY_Params dst = zero_struct;
  MemoryCopyStruct(&dst, src);
  dst.dbgi_keys.v = push_array(arena, DI_Key, dst.dbgi_keys.count);
  MemoryCopy(dst.dbgi_keys.v, src->dbgi_keys.v, sizeof(DI_Key)*src->dbgi_keys.count);
  for(U64 idx = 0; idx < dst.dbgi_keys.count; idx += 1)
  {
    dst.dbgi_keys.v[idx].path = push_str8_copy(arena, dst.dbgi_keys.v[idx].path);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
fzy_init(void)
{
  Arena *arena = arena_alloc();
  fzy_shared = push_array(arena, FZY_Shared, 1);
  fzy_shared->arena = arena;
  fzy_shared->slots_count = 256;
  fzy_shared->stripes_count = os_get_system_info()->logical_processor_count;
  fzy_shared->slots = push_array(arena, FZY_Slot, fzy_shared->slots_count);
  fzy_shared->stripes = push_array(arena, FZY_Stripe, fzy_shared->stripes_count);
  for(U64 idx = 0; idx < fzy_shared->stripes_count; idx += 1)
  {
    fzy_shared->stripes[idx].arena = arena_alloc();
    fzy_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    fzy_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  fzy_shared->thread_count = Min(os_get_system_info()->logical_processor_count, 2);
  fzy_shared->threads = push_array(arena, FZY_Thread, fzy_shared->thread_count);
  for(U64 idx = 0; idx < fzy_shared->thread_count; idx += 1)
  {
    fzy_shared->threads[idx].u2f_ring_mutex = os_mutex_alloc();
    fzy_shared->threads[idx].u2f_ring_cv = os_condition_variable_alloc();
    fzy_shared->threads[idx].u2f_ring_size = KB(64);
    fzy_shared->threads[idx].u2f_ring_base = push_array_no_zero(arena, U8, fzy_shared->threads[idx].u2f_ring_size);
    fzy_shared->threads[idx].thread = os_thread_launch(fzy_search_thread__entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Scope Functions

internal FZY_Scope *
fzy_scope_open(void)
{
  if(fzy_tctx == 0)
  {
    Arena *arena = arena_alloc();
    fzy_tctx = push_array(arena, FZY_TCTX, 1);
    fzy_tctx->arena = arena;
  }
  FZY_Scope *scope = fzy_tctx->free_scope;
  if(scope != 0)
  {
    SLLStackPop(fzy_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(fzy_tctx->arena, FZY_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
fzy_scope_close(FZY_Scope *scope)
{
  for(FZY_Touch *t = scope->first_touch, *next = 0; t != 0; t = next)
  {
    next = t->next;
    SLLStackPush(fzy_tctx->free_touch, t);
    if(t->node != 0)
    {
      ins_atomic_u64_dec_eval(&t->node->touch_count);
    }
  }
  SLLStackPush(fzy_tctx->free_scope, scope);
}

internal void
fzy_scope_touch_node__stripe_mutex_r_guarded(FZY_Scope *scope, FZY_Node *node)
{
  if(node != 0)
  {
    ins_atomic_u64_inc_eval(&node->touch_count);
  }
  FZY_Touch *touch = fzy_tctx->free_touch;
  if(touch != 0)
  {
    SLLStackPop(fzy_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(fzy_tctx->arena, FZY_Touch, 1);
  }
  MemoryZeroStruct(touch);
  SLLQueuePush(scope->first_touch, scope->last_touch, touch);
  touch->node = node;
}

////////////////////////////////
//~ rjf: Cache Lookup Functions

internal FZY_ItemArray
fzy_items_from_key_params_query(FZY_Scope *scope, U128 key, FZY_Params *params, String8 query, U64 endt_us, B32 *stale_out)
{
  Temp scratch = scratch_begin(0, 0);
  FZY_ItemArray items = {0};
  
  //- rjf: hash parameters
  U64 params_hash = fzy_hash_from_params(params);
  
  //- rjf: unpack key
  U64 slot_idx = key.u64[1]%fzy_shared->slots_count;
  U64 stripe_idx = slot_idx%fzy_shared->stripes_count;
  FZY_Slot *slot = &fzy_shared->slots[slot_idx];
  FZY_Stripe *stripe = &fzy_shared->stripes[stripe_idx];
  
  //- rjf: query and/or request
  OS_MutexScopeR(stripe->rw_mutex) for(;;)
  {
    // rjf: map key -> node
    FZY_Node *node = 0;
    for(FZY_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->key, key))
      {
        node = n;
        break;
      }
    }
    
    // rjf: no node? -> allocate
    if(node == 0) OS_MutexScopeRWPromote(stripe->rw_mutex)
    {
      node = push_array(stripe->arena, FZY_Node, 1);
      SLLQueuePush(slot->first, slot->last, node);
      node->key = key;
      for(U64 idx = 0; idx < ArrayCount(node->buckets); idx += 1)
      {
        node->buckets[idx].arena = arena_alloc();
      }
    }
    
    // rjf: try to grab last valid results for this key/query; determine if stale
    B32 stale = 1;
    if(params_hash == node->buckets[node->gen%ArrayCount(node->buckets)].params_hash &&
       node->gen != 0)
    {
      fzy_scope_touch_node__stripe_mutex_r_guarded(scope, node);
      items = node->gen_items;
      stale = !str8_match(query, node->buckets[node->gen%ArrayCount(node->buckets)].query, 0);
      if(stale_out != 0)
      {
        *stale_out = stale;
      }
    }
    
    // rjf: if stale -> request again
    if(stale) OS_MutexScopeRWPromote(stripe->rw_mutex)
    {
      if(node->gen <= node->submit_gen && node->submit_gen < node->gen + ArrayCount(node->buckets)-1)
      {
        node->submit_gen += 1;
        arena_clear(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena);
        node->buckets[node->submit_gen%ArrayCount(node->buckets)].query = push_str8_copy(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena, query);
        node->buckets[node->submit_gen%ArrayCount(node->buckets)].params = fzy_params_copy(node->buckets[node->submit_gen%ArrayCount(node->buckets)].arena, params);
        node->buckets[node->submit_gen%ArrayCount(node->buckets)].params_hash = params_hash;
      }
      if((node->submit_gen > node->gen+1 || os_now_microseconds() >= node->last_time_submitted_us+100000) &&
         fzy_u2s_enqueue_req(key, endt_us))
      {
        node->last_time_submitted_us = os_now_microseconds();
      }
    }
    
    // rjf: not stale, or timeout -> break
    if(!stale || os_now_microseconds() >= endt_us)
    {
      break;
    }
    
    // rjf: no results, but have time to wait -> wait
    os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
  }
  
  scratch_end(scratch);
  return items;
}

////////////////////////////////
//~ rjf: Searcher Threads

internal B32
fzy_u2s_enqueue_req(U128 key, U64 endt_us)
{
  B32 sent = 0;
  FZY_Thread *thread = &fzy_shared->threads[key.u64[1]%fzy_shared->thread_count];
  OS_MutexScope(thread->u2f_ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->u2f_ring_write_pos - thread->u2f_ring_read_pos;
    U64 available_size = thread->u2f_ring_size - unconsumed_size;
    if(available_size >= sizeof(U128))
    {
      sent = 1;
      thread->u2f_ring_write_pos += ring_write_struct(thread->u2f_ring_base, thread->u2f_ring_size, thread->u2f_ring_write_pos, &key);
      break;
    }
    os_condition_variable_wait(thread->u2f_ring_cv, thread->u2f_ring_mutex, endt_us);
  }
  if(sent)
  {
    os_condition_variable_broadcast(thread->u2f_ring_cv);
  }
  return sent;
}

internal void
fzy_u2s_dequeue_req(Arena *arena, FZY_Thread *thread, U128 *key_out)
{
  OS_MutexScope(thread->u2f_ring_mutex) for(;;)
  {
    U64 unconsumed_size = thread->u2f_ring_write_pos - thread->u2f_ring_read_pos;
    if(unconsumed_size >= sizeof(U128))
    {
      thread->u2f_ring_read_pos += ring_read_struct(thread->u2f_ring_base, thread->u2f_ring_size, thread->u2f_ring_read_pos, key_out);
      break;
    }
    os_condition_variable_wait(thread->u2f_ring_cv, thread->u2f_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(thread->u2f_ring_cv);
}

internal int
fzy_qsort_compare_items(FZY_Item *a, FZY_Item *b)
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
fzy_search_thread__entry_point(void *p)
{
  ThreadNameF("[fzy] searcher #%I64u", (U64)p);
  FZY_Thread *thread = &fzy_shared->threads[(U64)p];
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    DI_Scope *di_scope = di_scope_open();
    
    ////////////////////////////
    //- rjf: dequeue next request
    //
    U128 key = {0};
    fzy_u2s_dequeue_req(scratch.arena, thread, &key);
    U64 slot_idx = key.u64[1]%fzy_shared->slots_count;
    U64 stripe_idx = slot_idx%fzy_shared->stripes_count;
    FZY_Slot *slot = &fzy_shared->slots[slot_idx];
    FZY_Stripe *stripe = &fzy_shared->stripes[stripe_idx];
    
    ////////////////////////////
    //- rjf: grab next exe_path/query for this key
    //
    B32 task_is_good = 0;
    Arena *task_arena = 0;
    String8 query = {0};
    FZY_Params params = {FZY_Target_Procedures};
    U64 initial_submit_gen = 0;
    OS_MutexScopeW(stripe->rw_mutex)
    {
      for(FZY_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->key, key))
        {
          FZY_Bucket *bucket = &n->buckets[n->submit_gen%ArrayCount(n->buckets)];
          task_is_good = 1;
          initial_submit_gen = n->submit_gen;
          task_arena         = bucket->arena;
          query              = bucket->query;
          params             = bucket->params;
          break;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: params -> look up all rdis
    //
    U64 rdis_count = params.dbgi_keys.count;
    RDI_Parsed **rdis = push_array(scratch.arena, RDI_Parsed *, rdis_count);
    if(task_is_good)
    {
      for(U64 idx = 0; idx < rdis_count; idx += 1)
      {
        rdis[idx] = di_rdi_from_key(di_scope, &params.dbgi_keys.v[idx], max_U64);
      }
    }
    
    ////////////////////////////
    //- rjf: search target -> info about search space
    //
    RDI_SectionKind section_kind = RDI_SectionKind_NULL;
    U64 element_name_idx_off = 0;
    if(task_is_good)
    {
      switch(params.target)
      {
        // NOTE(rjf): no default!
        case FZY_Target_COUNT:{}break;
        case FZY_Target_Procedures:
        {
          section_kind = RDI_SectionKind_Procedures;
          element_name_idx_off = OffsetOf(RDI_Procedure, name_string_idx);
        }break;
        case FZY_Target_GlobalVariables:
        {
          section_kind = RDI_SectionKind_GlobalVariables;
          element_name_idx_off = OffsetOf(RDI_GlobalVariable, name_string_idx);
        }break;
        case FZY_Target_ThreadVariables:
        {
          section_kind = RDI_SectionKind_ThreadVariables;
          element_name_idx_off = OffsetOf(RDI_ThreadVariable, name_string_idx);
        }break;
        case FZY_Target_UDTs:
        {
          section_kind = RDI_SectionKind_UDTs;
        }break;
      }
    }
    
    ////////////////////////////
    //- rjf: rdis * query * params -> item list
    //
    FZY_ItemChunkList items_list = {0};
    if(task_is_good)
    {
      U64 base_idx = 0;
      for(U64 rdi_idx = 0; rdi_idx < rdis_count; rdi_idx += 1)
      {
        RDI_Parsed *rdi = rdis[rdi_idx];
        U64 element_count = 0;
        void *table_base = rdi_section_raw_table_from_kind(rdi, section_kind, &element_count);
        U64 element_size = rdi_section_element_size_table[section_kind];
        for(U64 idx = 1; task_is_good && idx < element_count; idx += 1)
        {
          void *element = (U8 *)table_base + element_size*idx;
          U32 *name_idx_ptr = (U32 *)((U8 *)element + element_name_idx_off);
          if(params.target == FZY_Target_UDTs)
          {
            RDI_UDT *udt = (RDI_UDT *)element;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
            name_idx_ptr = &type_node->user_defined.name_string_idx;
          }
          U32 name_idx = *name_idx_ptr;
          U64 name_size = 0;
          U8 *name_base = rdi_string_from_idx(rdi, name_idx, &name_size);
          String8 name = str8(name_base, name_size);
          if(name.size == 0) { continue; }
          FuzzyMatchRangeList matches = fuzzy_match_find(task_arena, query, name);
          if(matches.count == matches.needle_part_count)
          {
            FZY_ItemChunk *chunk = items_list.last;
            if(chunk == 0 || chunk->count >= chunk->cap)
            {
              chunk = push_array(scratch.arena, FZY_ItemChunk, 1);
              chunk->cap = 1024;
              chunk->count = 0;
              chunk->v = push_array_no_zero(scratch.arena, FZY_Item, chunk->cap);
              SLLQueuePush(items_list.first, items_list.last, chunk);
              items_list.chunk_count += 1;
            }
            chunk->v[chunk->count].idx = base_idx + idx;
            chunk->v[chunk->count].match_ranges = matches;
            chunk->v[chunk->count].missed_size = (name_size > matches.total_dim) ? (name_size-matches.total_dim) : 0;
            chunk->count += 1;
            items_list.total_count += 1;
          }
          if(idx%100 == 99) OS_MutexScopeR(stripe->rw_mutex)
          {
            for(FZY_Node *n = slot->first; n != 0; n = n->next)
            {
              if(u128_match(n->key, key) && n->submit_gen > initial_submit_gen)
              {
                task_is_good = 0;
                break;
              }
            }
          }
        }
        base_idx += element_count;
      }
    }
    
    //- rjf: item list -> item array
    FZY_ItemArray items = {0};
    if(task_is_good)
    {
      items.count = items_list.total_count;
      items.v = push_array_no_zero(task_arena, FZY_Item, items.count);
      U64 idx = 0;
      for(FZY_ItemChunk *chunk = items_list.first; chunk != 0; chunk = chunk->next)
      {
        MemoryCopy(items.v+idx, chunk->v, sizeof(FZY_Item)*chunk->count);
        idx += chunk->count;
      }
    }
    
    //- rjf: sort item array
    if(items.count != 0 && query.size != 0)
    {
      quick_sort(items.v, items.count, sizeof(FZY_Item), fzy_qsort_compare_items);
    }
    
    //- rjf: commit to cache - busyloop on scope touches
    if(task_is_good)
    {
      for(B32 done = 0; !done;)
      {
        B32 found = 0;
        OS_MutexScopeW(stripe->rw_mutex) for(FZY_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(n->key, key))
          {
            if(n->touch_count == 0)
            {
              n->gen = initial_submit_gen;
              n->gen_items = items;
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
