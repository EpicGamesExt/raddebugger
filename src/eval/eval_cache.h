// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_CACHE_H
#define EVAL_CACHE_H

////////////////////////////////
//~ rjf: Cache Key Type

typedef struct E_Key E_Key;
struct E_Key
{
  U64 u64;
};

////////////////////////////////
//~ rjf: Cache Types

typedef U32 E_CacheBundleFlags;
enum
{
  E_CacheBundleFlag_Parse     = (1<<0),
  E_CacheBundleFlag_IRTree    = (1<<1),
  E_CacheBundleFlag_Bytecode  = (1<<2),
  E_CacheBundleFlag_Interpret = (1<<3),
};

typedef struct E_CacheBundle E_CacheBundle;
struct E_CacheBundle
{
  E_CacheBundleFlags flags;
  E_Key parent_key;
  String8 string;
  E_Parse parse;
  E_IRTreeAndType irtree;
  String8 bytecode;
  E_Interpretation interpretation;
};

typedef struct E_CacheNode E_CacheNode;
struct E_CacheNode
{
  E_CacheNode *string_next;
  E_CacheNode *key_next;
  E_Key key;
  E_CacheBundle bundle;
};

typedef struct E_CacheLookup E_CacheLookup;
struct E_CacheLookup
{
  E_CacheNode *node;
  U64 hash;
};

typedef struct E_CacheSlot E_CacheSlot;
struct E_CacheSlot
{
  E_CacheNode *first;
  E_CacheNode *last;
};

typedef struct E_CacheParentNode E_CacheParentNode;
struct E_CacheParentNode
{
  E_CacheParentNode *next;
  E_Key key;
};

typedef struct E_Cache E_Cache;
struct E_Cache
{
  Arena *arena;
  U64 arena_eval_start_pos;
  U64 key_id_gen;
  U64 key_slots_count;
  E_CacheSlot *key_slots;
  U64 string_slots_count;
  E_CacheSlot *string_slots;
  E_CacheParentNode *top_parent_node;
  E_CacheParentNode *free_parent_node;
};

////////////////////////////////
//~ rjf: Globals

thread_static E_Cache *e_cache = 0;
read_only global E_CacheBundle e_cache_bundle_nil = {0, {0}, {0}, {{0}, 0, &e_expr_nil, &e_expr_nil}, {&e_irnode_nil}};

////////////////////////////////
//~ rjf: Basic Key Helpers

internal B32 e_key_match(E_Key a, E_Key b);

////////////////////////////////
//~ rjf: Cache Initialization (Required For All Subsequent APIs)

internal void e_cache_eval_begin(void);

////////////////////////////////
//~ rjf: Cache Accessing Functions
//
// The cache uses a unique keying mechanism to refer to some evaluation at
// many layers of analysis.
//
//                                  key
//         ________________________________________________
//        /            /             |                     \
//     text ->   expression   ->  ir tree and type  ->  interpretation result
//
// Each one of these calls refers to one stage in this pipeline. The cache will
// only compute what is needed on-demand. If you ask for the full evaluation,
// which is a bundle of artifacts at all layers of analysis, then all stages
// will be computed.
//
// One wrinkle here is that the IR tree generation stage is implicitly
// parameterized by the "overridden" IR tree - this is to enable "parent
// expressions", e.g. `$.x`, or simply `x` assuming `foo` has such a member,
// in the context of some struct `foo` evaluates to the same thing as `foo.x`.
// So even though the primary API shape is based around singular keys, the
// "parent key stack" also implicitly parameterizes all of these (partly
// because it is not relevant in 99% of cases).

//- rjf: parent key stack
internal E_Key e_parent_key_push(E_Key key);
internal E_Key e_parent_key_pop(void);

//- rjf: key construction
internal E_Key e_key_from_string(String8 string);
internal E_Key e_key_from_stringf(char *fmt, ...);

//- rjf: base key -> bundle helper
internal E_CacheBundle *e_cache_bundle_from_key(E_Key key);

//- rjf: bundle -> pipeline stage outputs
internal E_Parse e_parse_from_bundle(E_CacheBundle *bundle);
internal E_IRTreeAndType e_irtree_from_bundle(E_CacheBundle *bundle);
internal String8 e_bytecode_from_bundle(E_CacheBundle *bundle);
internal E_Interpretation e_interpretation_from_bundle(E_CacheBundle *bundle);
#define e_parse_from_key(key) e_parse_from_bundle(e_cache_bundle_from_key(key))
#define e_irtree_from_key(key) e_irtree_from_bundle(e_cache_bundle_from_key(key))
#define e_bytecode_from_key(key) e_bytecode_from_bundle(e_cache_bundle_from_key(key))
#define e_interpretation_from_key(key) e_interpretation_from_bundle(e_cache_bundle_from_key(key))

//- rjf: comprehensive bundle
internal E_Eval e_eval_from_bundle(E_CacheBundle *bundle);
#define e_eval_from_key(key) e_eval_from_bundle(e_cache_bundle_from_key(key))

//- rjf: string-based helpers
// TODO(rjf): (replace the old bundle APIs here)

////////////////////////////////
//~ rjf: Key Extension Functions

internal E_Key e_key_wrap(E_Key key, String8 string);
internal E_Key e_key_wrapf(E_Key key, char *fmt, ...);

#endif // EVAL_CACHE_H
