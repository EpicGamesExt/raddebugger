// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0x684123ff

////////////////////////////////
//~ rjf: Basic Helpers

#if !defined(XXH_IMPLEMENTATION)
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif

internal U64
hs_little_hash_from_data(String8 data)
{
  U64 result = XXH3_64bits(data.str, data.size);
  return result;
}

internal U128
hs_hash_from_data(String8 data)
{
  U128 u128 = {0};
  XXH128_hash_t hash = XXH3_128bits(data.str, data.size);
  MemoryCopy(&u128, &hash, sizeof(u128));
  return u128;
}

internal HS_ID
hs_id_make(U64 u64_0, U64 u64_1)
{
  HS_ID id;
  id.u128[0].u64[0] = u64_0;
  id.u128[0].u64[1] = u64_1;
  return id;
}

internal B32
hs_id_match(HS_ID a, HS_ID b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal HS_Key
hs_key_make(HS_Root root, HS_ID id)
{
  HS_Key key = {root, 0, id};
  return key;
}

internal B32
hs_key_match(HS_Key a, HS_Key b)
{
  return (MemoryMatchStruct(&a.root, &b.root) && hs_id_match(a.id, b.id));
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
hs_init(void)
{
  Arena *arena = arena_alloc();
  hs_shared = push_array(arena, HS_Shared, 1);
  hs_shared->arena = arena;
  hs_shared->slots_count = 4096;
  hs_shared->stripes_count = Min(hs_shared->slots_count, os_get_system_info()->logical_processor_count);
  hs_shared->slots = push_array(arena, HS_Slot, hs_shared->slots_count);
  hs_shared->stripes = push_array(arena, HS_Stripe, hs_shared->stripes_count);
  hs_shared->stripes_free_nodes = push_array(arena, HS_Node *, hs_shared->stripes_count);
  for(U64 idx = 0; idx < hs_shared->stripes_count; idx += 1)
  {
    HS_Stripe *stripe = &hs_shared->stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = os_rw_mutex_alloc();
    stripe->cv = os_condition_variable_alloc();
  }
  hs_shared->key_slots_count = 4096;
  hs_shared->key_stripes_count = Min(hs_shared->key_slots_count, os_get_system_info()->logical_processor_count);
  hs_shared->key_slots = push_array(arena, HS_KeySlot, hs_shared->key_slots_count);
  hs_shared->key_stripes = push_array(arena, HS_Stripe, hs_shared->key_stripes_count);
  hs_shared->key_stripes_free_nodes = push_array(arena, HS_KeyNode *, hs_shared->key_stripes_count);
  for(U64 idx = 0; idx < hs_shared->key_stripes_count; idx += 1)
  {
    HS_Stripe *stripe = &hs_shared->key_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = os_rw_mutex_alloc();
    stripe->cv = os_condition_variable_alloc();
  }
  hs_shared->root_slots_count = 4096;
  hs_shared->root_stripes_count = Min(hs_shared->root_slots_count, os_get_system_info()->logical_processor_count);
  hs_shared->root_slots = push_array(arena, HS_RootSlot, hs_shared->root_slots_count);
  hs_shared->root_stripes = push_array(arena, HS_Stripe, hs_shared->root_stripes_count);
  hs_shared->root_stripes_free_nodes = push_array(arena, HS_RootNode *, hs_shared->root_stripes_count);
  for(U64 idx = 0; idx < hs_shared->root_stripes_count; idx += 1)
  {
    HS_Stripe *stripe = &hs_shared->root_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = os_rw_mutex_alloc();
    stripe->cv = os_condition_variable_alloc();
  }
  hs_shared->evictor_thread = os_thread_launch(hs_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Root Allocation/Deallocation

internal HS_Root
hs_root_alloc(void)
{
  HS_Root root = {0};
  root.u64[0] = ins_atomic_u64_inc_eval(&hs_shared->root_id_gen);
  U64 slot_idx = root.u64[1]%hs_shared->root_slots_count;
  U64 stripe_idx = slot_idx%hs_shared->root_stripes_count;
  HS_RootSlot *slot = &hs_shared->root_slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->root_stripes[stripe_idx];
  OS_MutexScopeW(stripe->rw_mutex)
  {
    HS_RootNode *node = hs_shared->root_stripes_free_nodes[stripe_idx];
    if(node != 0)
    {
      SLLStackPop(hs_shared->root_stripes_free_nodes[stripe_idx]);
    }
    else
    {
      node = push_array(stripe->arena, HS_RootNode, 1);
    }
    DLLPushBack(slot->first, slot->last, node);
    node->root = root;
    node->arena = arena_alloc();
  }
  return root;
}

internal void
hs_root_release(HS_Root root)
{
  //- rjf: unpack root
  U64 slot_idx = root.u64[1]%hs_shared->root_slots_count;
  U64 stripe_idx = slot_idx%hs_shared->root_stripes_count;
  HS_RootSlot *slot = &hs_shared->root_slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->root_stripes[stripe_idx];
  
  //- rjf: release root node, grab its arena / ID list
  Arena *root_arena = 0;
  HS_RootIDChunkList root_ids = {0};
  OS_MutexScopeW(stripe->rw_mutex)
  {
    for(HS_RootNode *n = slot->first; n != 0; n = n->next)
    {
      if(MemoryMatchStruct(&root, &n->root))
      {
        DLLRemove(slot->first, slot->last, n);
        root_arena = n->arena;
        root_ids = n->ids;
        SLLStackPush(hs_shared->root_stripes_free_nodes[stripe_idx], n);
        break;
      }
    }
  }
  
  //- rjf: release all IDs
  for(HS_RootIDChunkNode *id_chunk_n = root_ids.first; id_chunk_n != 0; id_chunk_n = id_chunk_n->next)
  {
    for EachIndex(chunk_idx, id_chunk_n->count)
    {
      HS_ID id = id_chunk_n->v[chunk_idx];
      HS_Key key = hs_key_make(root, id);
      U64 key_hash = hs_little_hash_from_data(str8_struct(&key));
      U64 key_slot_idx = key_hash%hs_shared->key_slots_count;
      U64 key_stripe_idx = key_slot_idx%hs_shared->key_stripes_count;
      HS_KeySlot *key_slot = &hs_shared->key_slots[key_slot_idx];
      HS_Stripe *key_stripe = &hs_shared->key_stripes[key_stripe_idx];
      OS_MutexScopeW(key_stripe->rw_mutex)
      {
        for(HS_KeyNode *n = key_slot->first; n != 0; n = n->next)
        {
          if(hs_key_match(n->key, key))
          {
            // rjf: release reference to all hashes
            for(U64 history_idx = 0; history_idx < HS_KEY_HASH_HISTORY_STRONG_REF_COUNT && history_idx < n->hash_history_gen; history_idx += 1)
            {
              U128 hash = n->hash_history[(n->hash_history_gen+history_idx)%ArrayCount(n->hash_history)];
              U64 hash_slot_idx = hash.u64[1]%hs_shared->slots_count;
              U64 hash_stripe_idx = hash_slot_idx%hs_shared->stripes_count;
              HS_Slot *hash_slot = &hs_shared->slots[hash_slot_idx];
              HS_Stripe *hash_stripe = &hs_shared->stripes[hash_stripe_idx];
              OS_MutexScopeR(hash_stripe->rw_mutex)
              {
                for(HS_Node *n = hash_slot->first; n != 0; n = n->next)
                {
                  if(u128_match(n->hash, hash))
                  {
                    ins_atomic_u64_dec_eval(&n->key_ref_count);
                    break;
                  }
                }
              }
            }
            
            // rjf: release key node
            DLLRemove(key_slot->first, key_slot->last, n);
            SLLStackPush(hs_shared->key_stripes_free_nodes[key_stripe_idx], n);
            break;
          }
        }
      }
    }
  }
}

////////////////////////////////
//~ rjf: Cache Submission

internal U128
hs_submit_data(HS_Key key, Arena **data_arena, String8 data)
{
  U64 key_hash = hs_little_hash_from_data(str8_struct(&key));
  U64 key_slot_idx = key_hash%hs_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%hs_shared->key_stripes_count;
  HS_KeySlot *key_slot = &hs_shared->key_slots[key_slot_idx];
  HS_Stripe *key_stripe = &hs_shared->key_stripes[key_stripe_idx];
  U128 hash = hs_hash_from_data(data);
  U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
  U64 stripe_idx = slot_idx%hs_shared->stripes_count;
  HS_Slot *slot = &hs_shared->slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
  
  //- rjf: commit data to cache - if already there, just bump key refcount
  ProfScope("commit data to cache - if already there, just bump key refcount") OS_MutexScopeW(stripe->rw_mutex)
  {
    HS_Node *existing_node = 0;
    for(HS_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash))
      {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0)
    {
      HS_Node *node = hs_shared->stripes_free_nodes[stripe_idx];
      if(node)
      {
        SLLStackPop(hs_shared->stripes_free_nodes[stripe_idx]);
      }
      else
      {
        node = push_array(stripe->arena, HS_Node, 1);
      }
      node->hash = hash;
      if(data_arena != 0)
      {
        node->arena = *data_arena;
      }
      node->data = data;
      node->scope_ref_count = 0;
      node->key_ref_count = 1;
      DLLPushBack(slot->first, slot->last, node);
    }
    else
    {
      existing_node->key_ref_count += 1;
      if(data_arena != 0)
      {
        arena_release(*data_arena);
      }
    }
    if(data_arena != 0)
    {
      *data_arena = 0;
    }
  }
  
  //- rjf: commit this hash to key cache
  U128 key_expired_hash = {0};
  ProfScope("commit this hash to key cache") OS_MutexScopeW(key_stripe->rw_mutex)
  {
    // rjf: find existing key
    B32 key_is_new = 0;
    HS_KeyNode *key_node = 0;
    for(HS_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(hs_key_match(n->key, key))
      {
        key_node = n;
        break;
      }
    }
    
    // rjf: create key node if it doesn't exist
    if(!key_node)
    {
      key_is_new = 1;
      key_node = hs_shared->key_stripes_free_nodes[key_stripe_idx];
      if(key_node)
      {
        SLLStackPop(hs_shared->key_stripes_free_nodes[key_stripe_idx]);
      }
      else
      {
        key_node = push_array(key_stripe->arena, HS_KeyNode, 1);
      }
      key_node->key = key;
      DLLPushBack(key_slot->first, key_slot->last, key_node);
    }
    
    // rjf: push hash into key's history
    if(key_node)
    {
      if(key_node->hash_history_gen >= HS_KEY_HASH_HISTORY_STRONG_REF_COUNT)
      {
        key_expired_hash = key_node->hash_history[(key_node->hash_history_gen-HS_KEY_HASH_HISTORY_STRONG_REF_COUNT)%ArrayCount(key_node->hash_history)];
      }
      key_node->hash_history[key_node->hash_history_gen%ArrayCount(key_node->hash_history)] = hash;
      key_node->hash_history_gen += 1;
    }
    
    // rjf: key is new -> add this key to the associated root
    if(key_is_new)
    {
      U64 root_hash = hs_little_hash_from_data(str8_struct(&key.root));
      U64 root_slot_idx = root_hash%hs_shared->root_slots_count;
      U64 root_stripe_idx = root_slot_idx%hs_shared->root_stripes_count;
      HS_RootSlot *root_slot = &hs_shared->root_slots[root_slot_idx];
      HS_Stripe *root_stripe = &hs_shared->root_stripes[root_stripe_idx];
      OS_MutexScopeW(root_stripe->rw_mutex)
      {
        for(HS_RootNode *n = root_slot->first; n != 0; n = n->next)
        {
          if(MemoryMatchStruct(&n->root, &key.root))
          {
            HS_RootIDChunkNode *chunk = n->ids.last;
            if(chunk == 0 || chunk->count >= chunk->cap)
            {
              chunk = push_array(n->arena, HS_RootIDChunkNode, 1);
              SLLQueuePush(n->ids.first, n->ids.last, chunk);
              n->ids.chunk_count += 1;
              chunk->cap = 1024;
              chunk->v = push_array_no_zero(n->arena, HS_ID, chunk->cap);
            }
            chunk->v[chunk->count] = key.id;
            chunk->count += 1;
            n->ids.total_count += 1;
            break;
          }
        }
      }
    }
  }
  
  //- rjf: decrement key ref count of expired hash
  ProfScope("decrement key ref count of expired hash")
    if(!u128_match(key_expired_hash, u128_zero()))
  {
    U64 old_hash_slot_idx = key_expired_hash.u64[1]%hs_shared->slots_count;
    U64 old_hash_stripe_idx = old_hash_slot_idx%hs_shared->stripes_count;
    HS_Slot *old_hash_slot = &hs_shared->slots[old_hash_slot_idx];
    HS_Stripe *old_hash_stripe = &hs_shared->stripes[old_hash_stripe_idx];
    OS_MutexScopeR(old_hash_stripe->rw_mutex)
    {
      for(HS_Node *n = old_hash_slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, key_expired_hash))
        {
          ins_atomic_u64_dec_eval(&n->key_ref_count);
          break;
        }
      }
    }
  }
  
  return hash;
}

////////////////////////////////
//~ rjf: Scoped Access

internal HS_Scope *
hs_scope_open(void)
{
  if(hs_tctx == 0)
  {
    Arena *arena = arena_alloc();
    hs_tctx = push_array(arena, HS_TCTX, 1);
    hs_tctx->arena = arena;
  }
  HS_Scope *scope = hs_tctx->free_scope;
  if(scope)
  {
    SLLStackPop(hs_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(hs_tctx->arena, HS_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
hs_scope_close(HS_Scope *scope)
{
  for(HS_Touch *touch = scope->top_touch, *next = 0; touch != 0; touch = next)
  {
    U128 hash = touch->hash;
    next = touch->next;
    U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
    U64 stripe_idx = slot_idx%hs_shared->stripes_count;
    HS_Slot *slot = &hs_shared->slots[slot_idx];
    HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(HS_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash))
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
    SLLStackPush(hs_tctx->free_touch, touch);
  }
  SLLStackPush(hs_tctx->free_scope, scope);
}

internal void
hs_scope_touch_node__stripe_r_guarded(HS_Scope *scope, HS_Node *node)
{
  HS_Touch *touch = hs_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  if(touch != 0)
  {
    SLLStackPop(hs_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(hs_tctx->arena, HS_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->hash = node->hash;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Downstream Accesses

internal void
hs_hash_downstream_inc(U128 hash)
{
  U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
  U64 stripe_idx = slot_idx%hs_shared->stripes_count;
  HS_Slot *slot = &hs_shared->slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    for(HS_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(hash, n->hash))
      {
        ins_atomic_u64_inc_eval(&n->downstream_ref_count);
        break;
      }
    }
  }
}

internal void
hs_hash_downstream_dec(U128 hash)
{
  U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
  U64 stripe_idx = slot_idx%hs_shared->stripes_count;
  HS_Slot *slot = &hs_shared->slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    for(HS_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(hash, n->hash))
      {
        ins_atomic_u64_dec_eval(&n->downstream_ref_count);
        break;
      }
    }
  }
}

////////////////////////////////
//~ rjf: Cache Lookup

internal U128
hs_hash_from_key(HS_Key key, U64 rewind_count)
{
  U128 result = {0};
  U64 key_hash = hs_little_hash_from_data(str8_struct(&key));
  U64 key_slot_idx = key_hash%hs_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%hs_shared->key_stripes_count;
  HS_KeySlot *key_slot = &hs_shared->key_slots[key_slot_idx];
  HS_Stripe *key_stripe = &hs_shared->key_stripes[key_stripe_idx];
  OS_MutexScopeR(key_stripe->rw_mutex)
  {
    for(HS_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(hs_key_match(n->key, key) && n->hash_history_gen > 0 && n->hash_history_gen-1 >= rewind_count)
      {
        result = n->hash_history[(n->hash_history_gen-1-rewind_count)%ArrayCount(n->hash_history)];
        break;
      }
    }
  }
  return result;
}

internal String8
hs_data_from_hash(HS_Scope *scope, U128 hash)
{
  ProfBeginFunction();
  String8 result = {0};
  U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
  U64 stripe_idx = slot_idx%hs_shared->stripes_count;
  HS_Slot *slot = &hs_shared->slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
  OS_MutexScopeR(stripe->rw_mutex)
  {
    for(HS_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash))
      {
        result = n->data;
        hs_scope_touch_node__stripe_r_guarded(scope, n);
        break;
      }
    }
  }
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Evictor Thread

internal void
hs_evictor_thread__entry_point(void *p)
{
  ThreadNameF("[hs] evictor thread");
  for(;;)
  {
    for(U64 slot_idx = 0; slot_idx < hs_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%hs_shared->stripes_count;
      HS_Slot *slot = &hs_shared->slots[slot_idx];
      HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(HS_Node *n = slot->first; n != 0; n = n->next)
        {
          U64 key_ref_count = ins_atomic_u64_eval(&n->key_ref_count);
          U64 scope_ref_count = ins_atomic_u64_eval(&n->scope_ref_count);
          U64 downstream_ref_count = ins_atomic_u64_eval(&n->downstream_ref_count);
          if(key_ref_count == 0 && scope_ref_count == 0 && downstream_ref_count == 0)
          {
            slot_has_work = 1;
            break;
          }
        }
      }
      if(slot_has_work) OS_MutexScopeW(stripe->rw_mutex)
      {
        for(HS_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          U64 key_ref_count = ins_atomic_u64_eval(&n->key_ref_count);
          U64 scope_ref_count = ins_atomic_u64_eval(&n->scope_ref_count);
          U64 downstream_ref_count = ins_atomic_u64_eval(&n->downstream_ref_count);
          if(key_ref_count == 0 && scope_ref_count == 0 && downstream_ref_count == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            SLLStackPush(hs_shared->stripes_free_nodes[stripe_idx], n);
            if(n->arena != 0)
            {
              arena_release(n->arena);
            }
          }
        }
      }
    }
    os_sleep_milliseconds(1000);
  }
}
