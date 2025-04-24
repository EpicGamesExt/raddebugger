// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Key Helpers

internal B32
e_key_match(E_Key a, E_Key b)
{
  B32 result = (a.u64 == b.u64);
  return result;
}

////////////////////////////////
//~ rjf: Cache Initialization (Required For All Subsequent APIs)

internal void
e_cache_eval_begin(void)
{
  if(e_cache == 0)
  {
    Arena *arena = arena_alloc();
    e_cache = push_array(arena, E_Cache, 1);
    e_cache->arena = arena;
    e_cache->arena_eval_start_pos = arena_pos(arena);
  }
  arena_pop_to(e_cache->arena, e_cache->arena_eval_start_pos);
  e_cache->key_id_gen = 0;
  e_cache->key_slots_count = 4096;
  e_cache->key_slots = push_array(e_cache->arena, E_CacheSlot, e_cache->key_slots_count);
  e_cache->string_slots_count = 4096;
  e_cache->string_slots = push_array(e_cache->arena, E_CacheSlot, e_cache->string_slots_count);
  e_cache->free_parent_node = 0;
  e_cache->top_parent_node = 0;
}

////////////////////////////////
//~ rjf: Cache Accessing Functions

//- rjf: parent key stack

internal E_Key
e_parent_key_push(E_Key key)
{
  E_Key top = {0};
  if(e_cache->top_parent_node != 0)
  {
    top = e_cache->top_parent_node->key;
  }
  E_CacheParentNode *n = e_cache->free_parent_node;
  if(n != 0)
  {
    SLLStackPop(e_cache->free_parent_node);
  }
  else
  {
    n = push_array(e_cache->arena, E_CacheParentNode, 1);
  }
  SLLStackPush(e_cache->top_parent_node, n);
  n->key = key;
  return top;
}

internal E_Key
e_parent_key_pop(void)
{
  E_CacheParentNode *n = e_cache->top_parent_node;
  SLLStackPop(e_cache->top_parent_node);
  SLLStackPush(e_cache->free_parent_node, n);
  E_Key popped = n->key;
  return popped;
}

//- rjf: key construction

internal E_Key
e_key_from_string(String8 string)
{
  E_Key parent_key = {0};
  if(e_cache->top_parent_node)
  {
    parent_key = e_cache->top_parent_node->key;
  }
  U64 hash = e_hash_from_string(parent_key.u64, string);
  U64 slot_idx = hash%e_cache->string_slots_count;
  E_CacheSlot *slot = &e_cache->string_slots[slot_idx];
  E_CacheNode *node = 0;
  for(E_CacheNode *n = slot->first; n != 0; n = n->string_next)
  {
    if(e_key_match(parent_key, n->bundle.parent_key) && str8_match(n->bundle.string, string, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    e_cache->key_id_gen += 1;
    E_Key key = {e_cache->key_id_gen};
    U64 key_hash = e_hash_from_string(5381, str8_struct(&key));
    U64 key_slot_idx = key_hash%e_cache->key_slots_count;
    E_CacheSlot *key_slot = &e_cache->key_slots[key_slot_idx];
    node = push_array(e_cache->arena, E_CacheNode, 1);
    node->key = key;
    SLLQueuePush_N(slot->first, slot->last, node, string_next);
    SLLQueuePush_N(key_slot->first, key_slot->last, node, key_next);
    node->bundle.parent_key = parent_key;
    node->bundle.string = push_str8_copy(e_cache->arena, string);
  }
  return node->key;
}

internal E_Key
e_key_from_stringf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Key result = e_key_from_string(string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

//- rjf: base key -> node helper

internal E_CacheBundle *
e_cache_bundle_from_key(E_Key key)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&key));
  U64 slot_idx = hash%e_cache->key_slots_count;
  E_CacheSlot *slot = &e_cache->key_slots[slot_idx];
  E_CacheNode *node = 0;
  for(E_CacheNode *n = slot->first; n != 0; n = n->key_next)
  {
    if(e_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  E_CacheBundle *bundle = &e_cache_bundle_nil;
  if(node != 0)
  {
    bundle = &node->bundle;
  }
  return bundle;
}

//- rjf: bundle -> pipeline stage outputs

internal E_Parse
e_parse_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Parse))
  {
    bundle->flags |= E_CacheBundleFlag_Parse;
    bundle->parse = e_push_parse_from_string(e_cache->arena, bundle->string);
  }
  E_Parse parse = bundle->parse;
  return parse;
}

internal E_IRTreeAndType
e_irtree_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_IRTree))
  {
    bundle->flags |= E_CacheBundleFlag_IRTree;
    E_IRTreeAndType overridden = e_irtree_from_key(bundle->parent_key);
    E_IRTreeAndType *prev_overridden = e_ir_state->overridden_irtree;
    e_ir_state->overridden_irtree = &overridden;
    E_Parse parse = e_parse_from_bundle(bundle);
    bundle->irtree = e_push_irtree_and_type_from_expr(e_cache->arena, parse.expr);
    e_ir_state->overridden_irtree = prev_overridden;
  }
  E_IRTreeAndType result = bundle->irtree;
  return result;
}

internal String8
e_bytecode_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Bytecode))
  {
    bundle->flags |= E_CacheBundleFlag_Bytecode;
    Temp scratch = scratch_begin(0, 0);
    E_IRTreeAndType irtree = e_irtree_from_bundle(bundle);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree.root);
    bundle->bytecode = e_bytecode_from_oplist(e_cache->arena, &oplist);
    scratch_end(scratch);
  }
  String8 result = bundle->bytecode;
  return result;
}

internal E_Interpretation
e_interpretation_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Interpret))
  {
    bundle->flags |= E_CacheBundleFlag_Interpret;
    String8 bytecode = e_bytecode_from_bundle(bundle);
    E_Interpretation interpret = e_interpret(bytecode);
    bundle->interpretation = interpret;
  }
  E_Interpretation interpret = bundle->interpretation;
  return interpret;
}

//- rjf: comprehensive bundle

internal E_Eval
e_eval_from_bundle(E_CacheBundle *bundle)
{
  E_Eval eval =
  {
    .expr = e_parse_from_bundle(bundle).expr,
    .irtree = e_irtree_from_bundle(bundle),
    .bytecode = e_bytecode_from_bundle(bundle),
  };
  E_Interpretation interpretation = e_interpretation_from_bundle(bundle);
  eval.code = interpretation.code;
  eval.value = interpretation.value;
  eval.space = interpretation.space;
  return eval;
}

//- rjf: string-based helpers
// TODO(rjf): (replace the old bundle APIs here)

////////////////////////////////
//~ rjf: Key Extension Functions

internal E_Key
e_key_wrap(E_Key key, String8 string)
{
  e_parent_key_push(key);
  E_Key result = e_key_from_string(string);
  e_parent_key_pop();
  return result;
}

internal E_Key
e_key_wrapf(E_Key key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Key result = e_key_wrap(key, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}
