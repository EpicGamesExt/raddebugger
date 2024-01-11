// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

#if !defined(BLAKE2_H)
#define HAVE_SSE2
#include "third_party/blake2/blake2.h"
#include "third_party/blake2/blake2b.c"
#endif

internal U128
hs_hash_from_data(String8 data)
{
  U128 u128 = {0};
  blake2b((U8 *)&u128.u64[0], sizeof(u128), data.str, data.size, 0, 0);
  return u128;
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
  hs_shared->stripes_count = 64;
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
  hs_shared->key_stripes_count = 64;
  hs_shared->key_slots = push_array(arena, HS_KeySlot, hs_shared->key_slots_count);
  hs_shared->key_stripes = push_array(arena, HS_Stripe, hs_shared->key_stripes_count);
  for(U64 idx = 0; idx < hs_shared->key_stripes_count; idx += 1)
  {
    HS_Stripe *stripe = &hs_shared->key_stripes[idx];
    stripe->arena = arena_alloc();
    stripe->rw_mutex = os_rw_mutex_alloc();
    stripe->cv = os_condition_variable_alloc();
  }
  hs_shared->evictor_thread = os_launch_thread(hs_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void
hs_tctx_ensure_inited(void)
{
  if(hs_tctx == 0)
  {
    Arena *arena = arena_alloc();
    hs_tctx = push_array(arena, HS_TCTX, 1);
    hs_tctx->arena = arena;
  }
}

////////////////////////////////
//~ rjf: Cache Submission

internal U128
hs_submit_data(U128 key, Arena **data_arena, String8 data)
{
  U64 key_slot_idx = key.u64[1]%hs_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%hs_shared->key_stripes_count;
  HS_KeySlot *key_slot = &hs_shared->key_slots[key_slot_idx];
  HS_Stripe *key_stripe = &hs_shared->key_stripes[key_stripe_idx];
  U128 hash = hs_hash_from_data(data);
  U64 slot_idx = hash.u64[1]%hs_shared->slots_count;
  U64 stripe_idx = slot_idx%hs_shared->stripes_count;
  HS_Slot *slot = &hs_shared->slots[slot_idx];
  HS_Stripe *stripe = &hs_shared->stripes[stripe_idx];
  
  //- rjf: commit data to cache - if already there, just bump key refcount
  OS_MutexScopeW(stripe->rw_mutex)
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
      node->arena = *data_arena;
      node->data = data;
      node->scope_ref_count = 0;
      node->key_ref_count = 1;
      DLLPushBack(slot->first, slot->last, node);
    }
    else
    {
      existing_node->key_ref_count += 1;
      arena_release(*data_arena);
    }
    *data_arena = 0;
  }
  
  //- rjf: commit this hash to key cache
  U128 key_old_hash = {0};
  OS_MutexScopeW(key_stripe->rw_mutex)
  {
    HS_KeyNode *key_node = 0;
    for(HS_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->key, key))
      {
        key_node = n;
        break;
      }
    }
    if(!key_node)
    {
      key_node = push_array(key_stripe->arena, HS_KeyNode, 1);
      key_node->key = key;
      SLLQueuePush(key_slot->first, key_slot->last, key_node);
    }
    if(key_node)
    {
      key_old_hash = key_node->hash;
      key_node->hash = hash;
    }
  }
  
  //- rjf: if this key was correllated with an old hash, dec key ref count of old hash
  if(!u128_match(key_old_hash, u128_zero()))
  {
    U64 old_hash_slot_idx = key_old_hash.u64[1]%hs_shared->slots_count;
    U64 old_hash_stripe_idx = old_hash_slot_idx%hs_shared->stripes_count;
    HS_Slot *old_hash_slot = &hs_shared->slots[old_hash_slot_idx];
    HS_Stripe *old_hash_stripe = &hs_shared->stripes[old_hash_stripe_idx];
    OS_MutexScopeR(old_hash_stripe->rw_mutex)
    {
      for(HS_Node *n = old_hash_slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, key_old_hash))
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
  hs_tctx_ensure_inited();
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
//~ rjf: Cache Lookup

internal U128
hs_hash_from_key(U128 key)
{
  U128 result = {0};
  U64 key_slot_idx = key.u64[1]%hs_shared->key_slots_count;
  U64 key_stripe_idx = key_slot_idx%hs_shared->key_stripes_count;
  HS_KeySlot *key_slot = &hs_shared->key_slots[key_slot_idx];
  HS_Stripe *key_stripe = &hs_shared->key_stripes[key_stripe_idx];
  OS_MutexScopeR(key_stripe->rw_mutex)
  {
    for(HS_KeyNode *n = key_slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->key, key))
      {
        result = n->hash;
        break;
      }
    }
  }
  return result;
}

internal String8
hs_data_from_hash(HS_Scope *scope, U128 hash)
{
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
  return result;
}

////////////////////////////////
//~ rjf: Evictor Thread

internal void
hs_evictor_thread__entry_point(void *p)
{
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
          if(key_ref_count == 0 && scope_ref_count == 0)
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
          if(key_ref_count == 0 && scope_ref_count == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            SLLStackPush(hs_shared->stripes_free_nodes[stripe_idx], n);
            arena_release(n->arena);
          }
        }
      }
    }
    os_sleep_milliseconds(1000);
  }
}
