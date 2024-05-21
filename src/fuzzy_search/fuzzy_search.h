// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FUZZY_SEARCH_H
#define FUZZY_SEARCH_H

////////////////////////////////
//~ rjf: Fuzzy Search Types

typedef enum FZY_Target
{
  FZY_Target_Procedures,
  FZY_Target_GlobalVariables,
  FZY_Target_ThreadVariables,
  FZY_Target_UDTs,
  FZY_Target_COUNT
}
FZY_Target;

typedef struct FZY_Params FZY_Params;
struct FZY_Params
{
  FZY_Target target;
};

typedef struct FZY_Item FZY_Item;
struct FZY_Item
{
  U64 idx;
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

typedef struct FZY_Bucket FZY_Bucket;
struct FZY_Bucket
{
  Arena *arena;
  String8 query;
  FZY_Target target;
};

typedef struct FZY_Node FZY_Node;
struct FZY_Node
{
  FZY_Node *next;
  U128 key;
  U64 scope_touch_count;
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
//~ rjf: Helpers

internal U64 fzy_hash_from_string(String8 string, StringMatchFlags match_flags);
internal U64 fzy_item_num_from_array_element_idx__linear_search(FZY_ItemArray *array, U64 element_idx);

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
