// Copyright (c) Epic Games Tools
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
c_little_hash_from_data(String8 data)
{
  U64 result = XXH3_64bits(data.str, data.size);
  return result;
}

internal U128
c_hash_from_data(String8 data)
{
  U128 u128 = {0};
  XXH128_hash_t hash = XXH3_128bits(data.str, data.size);
  MemoryCopy(&u128, &hash, sizeof(u128));
  return u128;
}

internal C_ID
c_id_make(U64 u64_0, U64 u64_1)
{
  C_ID id;
  id.u128[0].u64[0] = u64_0;
  id.u128[0].u64[1] = u64_1;
  return id;
}

internal B32
c_id_match(C_ID a, C_ID b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal C_Key
c_key_make(C_Root root, C_ID id)
{
  C_Key key = {root, 0, id};
  return key;
}

internal B32
c_key_match(C_Key a, C_Key b)
{
  return (MemoryMatchStruct(&a.root, &b.root) && c_id_match(a.id, b.id));
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
c_init(void)
{
  Arena *arena = arena_alloc();
  c_shared = push_array(arena, C_Shared, 1);
  c_shared->arena = arena;
  c_shared->blob_slots_count = 16384;
  c_shared->blob_stripes_count = Min(c_shared->blob_slots_count, os_get_system_info()->logical_processor_count);
  c_shared->blob_slots = push_array(arena, C_BlobSlot, c_shared->blob_slots_count);
  c_shared->blob_stripes = push_array(arena, C_Stripe, c_shared->blob_stripes_count);
  c_shared->blob_stripes_free_nodes = push_array(arena, C_BlobNode *, c_shared->blob_stripes_count);
  for(U64 idx = 0; idx < c_shared->blob_stripes_count; idx += 1)
  {
    C_Stripe *stripe = &c_shared->blob_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = rw_mutex_alloc();
    stripe->cv = cond_var_alloc();
  }
  c_shared->key_slots_count = 4096;
  c_shared->key_stripes_count = Min(c_shared->key_slots_count, os_get_system_info()->logical_processor_count);
  c_shared->key_slots = push_array(arena, C_KeySlot, c_shared->key_slots_count);
  c_shared->key_stripes = push_array(arena, C_Stripe, c_shared->key_stripes_count);
  c_shared->key_stripes_free_nodes = push_array(arena, C_KeyNode *, c_shared->key_stripes_count);
  for(U64 idx = 0; idx < c_shared->key_stripes_count; idx += 1)
  {
    C_Stripe *stripe = &c_shared->key_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = rw_mutex_alloc();
    stripe->cv = cond_var_alloc();
  }
  c_shared->root_slots_count = 4096;
  c_shared->root_stripes_count = Min(c_shared->root_slots_count, os_get_system_info()->logical_processor_count);
  c_shared->root_slots = push_array(arena, C_RootSlot, c_shared->root_slots_count);
  c_shared->root_stripes = push_array(arena, C_Stripe, c_shared->root_stripes_count);
  c_shared->root_stripes_free_nodes = push_array(arena, C_RootNode *, c_shared->root_stripes_count);
  for(U64 idx = 0; idx < c_shared->root_stripes_count; idx += 1)
  {
    C_Stripe *stripe = &c_shared->root_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = rw_mutex_alloc();
    stripe->cv = cond_var_alloc();
  }
}

////////////////////////////////
//~ rjf: Root Allocation/Deallocation

internal C_Root
c_root_alloc(void)
{
  C_Root root = {0};
  root.u64[0] = ins_atomic_u64_inc_eval(&c_shared->root_id_gen);
  U64 slot_idx = root.u64[0]%c_shared->root_slots_count;
  U64 stripe_idx = slot_idx%c_shared->root_stripes_count;
  C_RootSlot *slot = &c_shared->root_slots[slot_idx];
  C_Stripe *stripe = &c_shared->root_stripes[stripe_idx];
  MutexScopeW(stripe->rw_mutex)
  {
    C_RootNode *node = c_shared->root_stripes_free_nodes[stripe_idx];
    if(node != 0)
    {
      SLLStackPop(c_shared->root_stripes_free_nodes[stripe_idx]);
    }
    else
    {
      node = push_array(stripe->arena, C_RootNode, 1);
    }
    DLLPushBack(slot->first, slot->last, node);
    node->root = root;
    node->arena = arena_alloc();
  }
  return root;
}

internal void
c_root_release(C_Root root)
{
  //- rjf: unpack root
  U64 slot_idx = root.u64[0]%c_shared->root_slots_count;
  U64 stripe_idx = slot_idx%c_shared->root_stripes_count;
  C_RootSlot *slot = &c_shared->root_slots[slot_idx];
  C_Stripe *stripe = &c_shared->root_stripes[stripe_idx];
  
  //- rjf: release root node, grab its arena / ID list
  Arena *root_arena = 0;
  C_RootIDChunkList root_ids = {0};
  MutexScopeW(stripe->rw_mutex)
  {
    for(C_RootNode *n = slot->first; n != 0; n = n->next)
    {
      if(MemoryMatchStruct(&root, &n->root))
      {
        DLLRemove(slot->first, slot->last, n);
        root_arena = n->arena;
        root_ids = n->ids;
        SLLStackPush(c_shared->root_stripes_free_nodes[stripe_idx], n);
        break;
      }
    }
  }
  
  //- rjf: release all IDs
  for(C_RootIDChunkNode *id_chunk_n = root_ids.first; id_chunk_n != 0; id_chunk_n = id_chunk_n->next)
  {
    for EachIndex(chunk_idx, id_chunk_n->count)
    {
      C_ID id = id_chunk_n->v[chunk_idx];
      C_Key key = c_key_make(root, id);
      U64 key_hash = c_little_hash_from_data(str8_struct(&key));
      U64 key_slot_idx = key_hash%c_shared->key_slots_count;
      U64 key_stripe_idx = key_slot_idx%c_shared->key_stripes_count;
      C_KeySlot *key_slot = &c_shared->key_slots[key_slot_idx];
      C_Stripe *key_stripe = &c_shared->key_stripes[key_stripe_idx];
      MutexScopeW(key_stripe->rw_mutex)
      {
        for(C_KeyNode *n = key_slot->first; n != 0; n = n->next)
        {
          if(c_key_match(n->key, key))
          {
            // rjf: release reference to all hashes
            for(U64 history_idx = 0; history_idx < C_KEY_HASH_HISTORY_STRONG_REF_COUNT && history_idx < n->hash_history_gen; history_idx += 1)
            {
              U128 hash = n->hash_history[(n->hash_history_gen+history_idx)%ArrayCount(n->hash_history)];
              U64 hash_slot_idx = hash.u64[1]%c_shared->blob_slots_count;
              U64 hash_stripe_idx = hash_slot_idx%c_shared->blob_stripes_count;
              C_BlobSlot *hash_slot = &c_shared->blob_slots[hash_slot_idx];
              C_Stripe *hash_stripe = &c_shared->blob_stripes[hash_stripe_idx];
              MutexScopeR(hash_stripe->rw_mutex)
              {
                for(C_BlobNode *n = hash_slot->first; n != 0; n = n->next)
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
            SLLStackPush(c_shared->key_stripes_free_nodes[key_stripe_idx], n);
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
c_submit_data(C_Key key, Arena **data_arena, String8 data)
{
  U64 key_hash = c_little_hash_from_data(str8_struct(&key));
  U64 key_slot_idx = key_hash%c_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%c_shared->key_stripes_count;
  C_KeySlot *key_slot = &c_shared->key_slots[key_slot_idx];
  C_Stripe *key_stripe = &c_shared->key_stripes[key_stripe_idx];
  U128 hash = c_hash_from_data(data);
  U64 slot_idx = hash.u64[1]%c_shared->blob_slots_count;
  U64 stripe_idx = slot_idx%c_shared->blob_stripes_count;
  C_BlobSlot *slot = &c_shared->blob_slots[slot_idx];
  C_Stripe *stripe = &c_shared->blob_stripes[stripe_idx];
  
  //- rjf: commit data to cache - if already there, just bump key refcount
  ProfScope("commit data to cache - if already there, just bump key refcount") MutexScopeW(stripe->rw_mutex)
  {
    C_BlobNode *existing_node = 0;
    for(C_BlobNode *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash))
      {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0)
    {
      C_BlobNode *node = c_shared->blob_stripes_free_nodes[stripe_idx];
      if(node)
      {
        SLLStackPop(c_shared->blob_stripes_free_nodes[stripe_idx]);
      }
      else
      {
        node = push_array(stripe->arena, C_BlobNode, 1);
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
  ProfScope("commit this hash to key cache") MutexScopeW(key_stripe->rw_mutex)
  {
    // rjf: find existing key
    B32 key_is_new = 0;
    C_KeyNode *key_node = 0;
    for(C_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(c_key_match(n->key, key))
      {
        key_node = n;
        break;
      }
    }
    
    // rjf: create key node if it doesn't exist
    if(!key_node)
    {
      key_is_new = 1;
      key_node = c_shared->key_stripes_free_nodes[key_stripe_idx];
      if(key_node)
      {
        SLLStackPop(c_shared->key_stripes_free_nodes[key_stripe_idx]);
      }
      else
      {
        key_node = push_array(key_stripe->arena, C_KeyNode, 1);
      }
      key_node->key = key;
      DLLPushBack(key_slot->first, key_slot->last, key_node);
    }
    
    // rjf: push hash into key's history
    if(key_node)
    {
      if(key_node->hash_history_gen >= C_KEY_HASH_HISTORY_STRONG_REF_COUNT)
      {
        key_expired_hash = key_node->hash_history[(key_node->hash_history_gen-C_KEY_HASH_HISTORY_STRONG_REF_COUNT)%ArrayCount(key_node->hash_history)];
      }
      key_node->hash_history[key_node->hash_history_gen%ArrayCount(key_node->hash_history)] = hash;
      key_node->hash_history_gen += 1;
    }
    
    // rjf: key is new -> add this key to the associated root
    if(key_is_new)
    {
      U64 root_hash = c_little_hash_from_data(str8_struct(&key.root));
      U64 root_slot_idx = root_hash%c_shared->root_slots_count;
      U64 root_stripe_idx = root_slot_idx%c_shared->root_stripes_count;
      C_RootSlot *root_slot = &c_shared->root_slots[root_slot_idx];
      C_Stripe *root_stripe = &c_shared->root_stripes[root_stripe_idx];
      MutexScopeW(root_stripe->rw_mutex)
      {
        for(C_RootNode *n = root_slot->first; n != 0; n = n->next)
        {
          if(MemoryMatchStruct(&n->root, &key.root))
          {
            C_RootIDChunkNode *chunk = n->ids.last;
            if(chunk == 0 || chunk->count >= chunk->cap)
            {
              chunk = push_array(n->arena, C_RootIDChunkNode, 1);
              SLLQueuePush(n->ids.first, n->ids.last, chunk);
              n->ids.chunk_count += 1;
              chunk->cap = 1024;
              chunk->v = push_array_no_zero(n->arena, C_ID, chunk->cap);
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
    U64 old_hash_slot_idx = key_expired_hash.u64[1]%c_shared->blob_slots_count;
    U64 old_hash_stripe_idx = old_hash_slot_idx%c_shared->blob_stripes_count;
    C_BlobSlot *old_hash_slot = &c_shared->blob_slots[old_hash_slot_idx];
    C_Stripe *old_hash_stripe = &c_shared->blob_stripes[old_hash_stripe_idx];
    MutexScopeR(old_hash_stripe->rw_mutex)
    {
      for(C_BlobNode *n = old_hash_slot->first; n != 0; n = n->next)
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
//~ rjf: Downstream Accesses

internal void
c_hash_downstream_inc(U128 hash)
{
  U64 slot_idx = hash.u64[1]%c_shared->blob_slots_count;
  U64 stripe_idx = slot_idx%c_shared->blob_stripes_count;
  C_BlobSlot *slot = &c_shared->blob_slots[slot_idx];
  C_Stripe *stripe = &c_shared->blob_stripes[stripe_idx];
  MutexScopeR(stripe->rw_mutex)
  {
    for(C_BlobNode *n = slot->first; n != 0; n = n->next)
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
c_hash_downstream_dec(U128 hash)
{
  U64 slot_idx = hash.u64[1]%c_shared->blob_slots_count;
  U64 stripe_idx = slot_idx%c_shared->blob_stripes_count;
  C_BlobSlot *slot = &c_shared->blob_slots[slot_idx];
  C_Stripe *stripe = &c_shared->blob_stripes[stripe_idx];
  MutexScopeR(stripe->rw_mutex)
  {
    for(C_BlobNode *n = slot->first; n != 0; n = n->next)
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
c_hash_from_key(C_Key key, U64 rewind_count)
{
  U128 result = {0};
  U64 key_hash = c_little_hash_from_data(str8_struct(&key));
  U64 key_slot_idx = key_hash%c_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%c_shared->key_stripes_count;
  C_KeySlot *key_slot = &c_shared->key_slots[key_slot_idx];
  C_Stripe *key_stripe = &c_shared->key_stripes[key_stripe_idx];
  MutexScopeR(key_stripe->rw_mutex)
  {
    for(C_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(c_key_match(n->key, key) && n->hash_history_gen > 0 && n->hash_history_gen-1 >= rewind_count)
      {
        result = n->hash_history[(n->hash_history_gen-1-rewind_count)%ArrayCount(n->hash_history)];
        break;
      }
    }
  }
  return result;
}

internal String8
c_data_from_hash(Access *access, U128 hash)
{
  ProfBeginFunction();
  String8 result = {0};
  U64 slot_idx = hash.u64[1]%c_shared->blob_slots_count;
  U64 stripe_idx = slot_idx%c_shared->blob_stripes_count;
  C_BlobSlot *slot = &c_shared->blob_slots[slot_idx];
  C_Stripe *stripe = &c_shared->blob_stripes[stripe_idx];
  MutexScopeR(stripe->rw_mutex)
  {
    for(C_BlobNode *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash))
      {
        result = n->data;
        access_touch(access, &n->scope_ref_count, stripe->cv);
        break;
      }
    }
  }
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Tick

internal void
c_tick(void)
{
  ProfBeginFunction();
  Rng1U64 range = lane_range(c_shared->blob_slots_count);
  for EachInRange(slot_idx, range)
  {
    U64 stripe_idx = slot_idx%c_shared->blob_stripes_count;
    C_BlobSlot *slot = &c_shared->blob_slots[slot_idx];
    C_Stripe *stripe = &c_shared->blob_stripes[stripe_idx];
    B32 slot_has_work = 0;
    MutexScopeR(stripe->rw_mutex)
    {
      for(C_BlobNode *n = slot->first; n != 0; n = n->next)
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
    if(slot_has_work) MutexScopeW(stripe->rw_mutex)
    {
      for(C_BlobNode *n = slot->first, *next = 0; n != 0; n = next)
      {
        next = n->next;
        U64 key_ref_count = ins_atomic_u64_eval(&n->key_ref_count);
        U64 scope_ref_count = ins_atomic_u64_eval(&n->scope_ref_count);
        U64 downstream_ref_count = ins_atomic_u64_eval(&n->downstream_ref_count);
        if(key_ref_count == 0 && scope_ref_count == 0 && downstream_ref_count == 0)
        {
          DLLRemove(slot->first, slot->last, n);
          SLLStackPush(c_shared->blob_stripes_free_nodes[stripe_idx], n);
          if(n->arena != 0)
          {
            arena_release(n->arena);
          }
        }
      }
    }
  }
  ProfEnd();
}
