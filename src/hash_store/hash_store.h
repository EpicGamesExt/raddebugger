// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef HASH_STORE_H
#define HASH_STORE_H

////////////////////////////////
//~ NOTE(rjf): Hash Store Notes (2025/05/18)
//
// The hash store is a general-purpose data cache. It offers three layers of
// caching: (a) content (hash of data), (b) key (unique identity correllated
// with history of hashes), and (c) root (bucket for many keys, manually
// allocated / deallocated).
//
//  (a) The "content" level of cache access is a simply hash(data) -> data
//      mapping. This bypasses all identity/key/root mechanisms and provides a
//      way to just talk about unique (and deduplicated) blobs of data.
//
//  (b) The "key" level of cache access is used to encode a history of hashes
//      for some unique "identity", where the "identity" is a concept managed
//      by the user. One example of an identity would be a particular address
//      range inside of some process to which the debugger is attached. Another
//      might be a range inside of some file.
//
//  (c) The "root" level is to provide a top-level allocation/deallocation
//      mechanism for a large set of keys. It also provides an extra level of
//      key uniqueness. For instance, each process to which the debugger is
//      attached might have its own root, and each key might correspond to a
//      particular address range within that process. This way, when the
//      process ends, all of its keys can be easily destroyed using a single
//      deallocation of the root.
//
// The way this might be generally used inside of the debugger would be that
// some evaluation - let's say it's some variable `x` - is mapped (via debug
// info) to some address range. If `x` is a `char[4096]`, then it might map
// to some address range [&x, &x + 4096). This, together with the process
// within which `x` is evaluated, forms both a `root` (for the process) and
// a `key` (for the address range). Some asynchronous memory streaming system
// can then, together with the root and key, read memory for that range, then
// submit that data to the hash store, correllating with the root and key
// combo.

////////////////////////////////
//~ rjf: Key Types

typedef struct HS_Root HS_Root;
struct HS_Root
{
  U64 u64[1];
};

typedef struct HS_ID HS_ID;
struct HS_ID
{
  U128 u128[1];
};

typedef struct HS_Key HS_Key;
struct HS_Key
{
  HS_Root root;
  U64 _padding_;
  HS_ID id;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct HS_RootIDChunkNode HS_RootIDChunkNode;
struct HS_RootIDChunkNode
{
  HS_RootIDChunkNode *next;
  U128 *v;
  U64 count;
  U64 cap;
};

typedef struct HS_RootIDChunkList HS_RootIDChunkList;
struct HS_RootIDChunkList
{
  HS_RootIDChunkNode *first;
  HS_RootIDChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct HS_RootNode HS_RootNode;
struct HS_RootNode
{
  HS_RootNode *next;
  HS_RootNode *prev;
  Arena *arena;
  HS_Root root;
  HS_RootIDChunkList ids;
};

typedef struct HS_RootSlot HS_RootSlot;
struct HS_RootSlot
{
  HS_RootNode *first;
  HS_RootNode *last;
};

#define HS_KEY_HASH_HISTORY_COUNT 64
#define HS_KEY_HASH_HISTORY_STRONG_REF_COUNT 2

typedef struct HS_KeyNode HS_KeyNode;
struct HS_KeyNode
{
  HS_KeyNode *next;
  HS_KeyNode *prev;
  HS_Key key;
  U128 hash_history[HS_KEY_HASH_HISTORY_COUNT];
  U64 hash_history_gen;
};

typedef struct HS_KeySlot HS_KeySlot;
struct HS_KeySlot
{
  HS_KeyNode *first;
  HS_KeyNode *last;
};

typedef struct HS_Node HS_Node;
struct HS_Node
{
  HS_Node *next;
  HS_Node *prev;
  U128 hash;
  Arena *arena;
  String8 data;
  U64 scope_ref_count;
  U64 key_ref_count;
  U64 downstream_ref_count;
};

typedef struct HS_Slot HS_Slot;
struct HS_Slot
{
  HS_Node *first;
  HS_Node *last;
};

typedef struct HS_Stripe HS_Stripe;
struct HS_Stripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access

typedef struct HS_Touch HS_Touch;
struct HS_Touch
{
  HS_Touch *next;
  U128 hash;
};

typedef struct HS_Scope HS_Scope;
struct HS_Scope
{
  HS_Scope *next;
  HS_Touch *top_touch;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct HS_TCTX HS_TCTX;
struct HS_TCTX
{
  Arena *arena;
  HS_Scope *free_scope;
  HS_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct HS_Shared HS_Shared;
struct HS_Shared
{
  Arena *arena;
  
  // rjf: main data cache
  U64 slots_count;
  U64 stripes_count;
  HS_Slot *slots;
  HS_Stripe *stripes;
  HS_Node **stripes_free_nodes;
  
  // rjf: key cache
  U64 key_slots_count;
  U64 key_stripes_count;
  HS_KeySlot *key_slots;
  HS_Stripe *key_stripes;
  HS_KeyNode **key_stripes_free_nodes;
  
  // rjf: root cache
  U64 root_slots_count;
  U64 root_stripes_count;
  HS_RootSlot *root_slots;
  HS_Stripe *root_stripes;
  HS_RootNode **root_stripes_free_nodes;
  U64 root_id_gen;
  
  // rjf: evictor thread
  OS_Handle evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static HS_TCTX *hs_tctx = 0;
global HS_Shared *hs_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 hs_little_hash_from_data(String8 data);
internal U128 hs_hash_from_data(String8 data);
internal HS_ID hs_id_make(U64 u64_0, U64 u64_1);
internal B32 hs_id_match(HS_ID a, HS_ID b);
internal HS_Key hs_key_make(HS_Root root, HS_ID id);
internal B32 hs_key_match(HS_Key a, HS_Key b);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void hs_init(void);

////////////////////////////////
//~ rjf: Root Allocation/Deallocation

internal HS_Root hs_root_alloc(void);
internal void hs_root_release(HS_Root root);

////////////////////////////////
//~ rjf: Cache Submission

internal U128 hs_submit_data(HS_Key key, Arena **data_arena, String8 data);

////////////////////////////////
//~ rjf: Scoped Access

internal HS_Scope *hs_scope_open(void);
internal void hs_scope_close(HS_Scope *scope);
internal void hs_scope_touch_node__stripe_r_guarded(HS_Scope *scope, HS_Node *node);

////////////////////////////////
//~ rjf: Key Closing

internal void hs_key_close(HS_Key key);

////////////////////////////////
//~ rjf: Downstream Accesses

internal void hs_hash_downstream_inc(U128 hash);
internal void hs_hash_downstream_dec(U128 hash);

////////////////////////////////
//~ rjf: Cache Lookups

internal U128 hs_hash_from_key(HS_Key key, U64 rewind_count);
internal String8 hs_data_from_hash(HS_Scope *scope, U128 hash);

////////////////////////////////
//~ rjf: Evictor Thread

internal void hs_evictor_thread__entry_point(void *p);

#endif // HASH_STORE_H
