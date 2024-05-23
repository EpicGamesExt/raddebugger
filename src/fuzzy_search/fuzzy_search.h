// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FUZZY_SEARCH_H
#define FUZZY_SEARCH_H

////////////////////////////////
//~ rjf: Result Types

typedef struct FZY_Item FZY_Item;
struct FZY_Item
{
  U64 idx; // indexes into whole space of parameter tables. [rdis[0] element count) [rdis[1] element count) ... [rdis[n] element count)
  U64 missed_size;
  FuzzyMatchRangeList match_ranges;
};

typedef struct FZY_ItemChunk FZY_ItemChunk;
struct FZY_ItemChunk
{
  FZY_ItemChunk *next;
  FZY_Item *v;
  U64 count;
  U64 cap;
};

typedef struct FZY_ItemChunkList FZY_ItemChunkList;
struct FZY_ItemChunkList
{
  FZY_ItemChunk *first;
  FZY_ItemChunk *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct FZY_ItemArray FZY_ItemArray;
struct FZY_ItemArray
{
  FZY_Item *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Search Parameter Types

typedef enum FZY_Target
{
  FZY_Target_Procedures,
  FZY_Target_GlobalVariables,
  FZY_Target_ThreadVariables,
  FZY_Target_UDTs,
  FZY_Target_COUNT
}
FZY_Target;

typedef struct FZY_DbgiKey FZY_DbgiKey;
struct FZY_DbgiKey
{
  String8 path;
  U64 timestamp;
};

typedef struct FZY_DbgiKeyNode FZY_DbgiKeyNode;
struct FZY_DbgiKeyNode
{
  FZY_DbgiKeyNode *next;
  FZY_DbgiKey v;
};

typedef struct FZY_DbgiKeyList FZY_DbgiKeyList;
struct FZY_DbgiKeyList
{
  FZY_DbgiKeyNode *first;
  FZY_DbgiKeyNode *last;
  U64 count;
};

typedef struct FZY_DbgiKeyArray FZY_DbgiKeyArray;
struct FZY_DbgiKeyArray
{
  FZY_DbgiKey *v;
  U64 count;
};

typedef struct FZY_Params FZY_Params;
struct FZY_Params
{
  FZY_Target target;
  FZY_DbgiKeyArray dbgi_keys;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct FZY_Bucket FZY_Bucket;
struct FZY_Bucket
{
  Arena *arena;
  String8 query;
  FZY_Params params;
  U64 params_hash;
};

typedef struct FZY_Node FZY_Node;
struct FZY_Node
{
  FZY_Node *next;
  U128 key;
  U64 touch_count;
  U64 last_time_submitted_us;
  FZY_Bucket buckets[3];
  U64 gen;
  U64 submit_gen;
  FZY_ItemArray gen_items;
};

typedef struct FZY_Slot FZY_Slot;
struct FZY_Slot
{
  FZY_Node *first;
  FZY_Node *last;
};

typedef struct FZY_Stripe FZY_Stripe;
struct FZY_Stripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access Types

typedef struct FZY_Touch FZY_Touch;
struct FZY_Touch
{
  FZY_Touch *next;
  FZY_Node *node;
};

typedef struct FZY_Scope FZY_Scope;
struct FZY_Scope
{
  FZY_Scope *next;
  FZY_Touch *first_touch;
  FZY_Touch *last_touch;
};

typedef struct FZY_TCTX FZY_TCTX;
struct FZY_TCTX
{
  Arena *arena;
  FZY_Scope *free_scope;
  FZY_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State Types

typedef struct FZY_Thread FZY_Thread;
struct FZY_Thread
{
  OS_Handle thread;
  OS_Handle u2f_ring_mutex;
  OS_Handle u2f_ring_cv;
  U64 u2f_ring_size;
  U8 *u2f_ring_base;
  U64 u2f_ring_write_pos;
  U64 u2f_ring_read_pos;
};

typedef struct FZY_Shared FZY_Shared;
struct FZY_Shared
{
  Arena *arena;
  
  // rjf: search artifact cache table
  U64 slots_count;
  U64 stripes_count;
  FZY_Slot *slots;
  FZY_Stripe *stripes;
  
  // rjf: threads
  U64 thread_count;
  FZY_Thread *threads;
};

////////////////////////////////
//~ rjf: Globals

global FZY_Shared *fzy_shared = 0;
thread_static FZY_TCTX *fzy_tctx = 0;

////////////////////////////////
//~ rjf: Helpers

internal U64 fzy_hash_from_string(U64 seed, String8 string);
internal U64 fzy_hash_from_params(FZY_Params *params);
internal U64 fzy_item_num_from_array_element_idx__linear_search(FZY_ItemArray *array, U64 element_idx);
internal String8 fzy_item_string_from_rdi_target_element_idx(RDI_Parsed *rdi, FZY_Target target, U64 element_idx);
internal void fzy_dbgi_key_list_push(Arena *arena, FZY_DbgiKeyList *list, FZY_DbgiKey key);
internal FZY_DbgiKeyArray fzy_dbgi_key_array_from_list(Arena *arena, FZY_DbgiKeyList *list);
internal FZY_Params fzy_params_copy(Arena *arena, FZY_Params *src);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void fzy_init(void);

////////////////////////////////
//~ rjf: Scope Functions

internal FZY_Scope *fzy_scope_open(void);
internal void fzy_scope_close(FZY_Scope *scope);
internal void fzy_scope_touch_node__stripe_mutex_r_guarded(FZY_Scope *scope, FZY_Node *node);

////////////////////////////////
//~ rjf: Cache Lookup Functions

internal FZY_ItemArray fzy_items_from_key_params_query(FZY_Scope *scope, U128 key, FZY_Params *params, String8 query, U64 endt_us, B32 *stale_out);

////////////////////////////////
//~ rjf: Searcher Threads

internal B32 fzy_u2s_enqueue_req(U128 key, U64 endt_us);
internal void fzy_u2s_dequeue_req(Arena *arena, FZY_Thread *thread, U128 *key_out);

internal int fzy_qsort_compare_items(FZY_Item *a, FZY_Item *b);

internal void fzy_search_thread__entry_point(void *p);

#endif // FUZZY_SEARCH_H
