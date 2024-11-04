// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBGI_SEARCH_H
#define DBGI_SEARCH_H

////////////////////////////////
//~ rjf: Result Types

typedef struct DIS_Item DIS_Item;
struct DIS_Item
{
  U64 idx; // indexes into whole space of parameter tables. [rdis[0] element count) [rdis[1] element count) ... [rdis[n] element count)
  U64 missed_size;
  FuzzyMatchRangeList match_ranges;
};

typedef struct DIS_ItemChunk DIS_ItemChunk;
struct DIS_ItemChunk
{
  DIS_ItemChunk *next;
  DIS_Item *v;
  U64 count;
  U64 cap;
};

typedef struct DIS_ItemChunkList DIS_ItemChunkList;
struct DIS_ItemChunkList
{
  DIS_ItemChunk *first;
  DIS_ItemChunk *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct DIS_ItemArray DIS_ItemArray;
struct DIS_ItemArray
{
  DIS_Item *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Search Parameter Types

typedef struct DIS_Params DIS_Params;
struct DIS_Params
{
  RDI_SectionKind target;
  DI_KeyArray dbgi_keys;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct DIS_Bucket DIS_Bucket;
struct DIS_Bucket
{
  Arena *arena;
  String8 query;
  DIS_Params params;
  U64 params_hash;
};

typedef struct DIS_Node DIS_Node;
struct DIS_Node
{
  DIS_Node *next;
  U128 key;
  U64 touch_count;
  U64 last_time_submitted_us;
  DIS_Bucket buckets[3];
  U64 gen;
  U64 submit_gen;
  DIS_ItemArray gen_items;
};

typedef struct DIS_Slot DIS_Slot;
struct DIS_Slot
{
  DIS_Node *first;
  DIS_Node *last;
};

typedef struct DIS_Stripe DIS_Stripe;
struct DIS_Stripe
{
  Arena *arena;
  OS_Handle r_mutex;
  OS_Handle w_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access Types

typedef struct DIS_Touch DIS_Touch;
struct DIS_Touch
{
  DIS_Touch *next;
  DIS_Node *node;
};

typedef struct DIS_Scope DIS_Scope;
struct DIS_Scope
{
  DIS_Scope *next;
  DIS_Touch *first_touch;
  DIS_Touch *last_touch;
};

typedef struct DIS_TCTX DIS_TCTX;
struct DIS_TCTX
{
  Arena *arena;
  DIS_Scope *free_scope;
  DIS_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State Types

typedef struct DIS_Thread DIS_Thread;
struct DIS_Thread
{
  OS_Handle thread;
  OS_Handle u2f_ring_mutex;
  OS_Handle u2f_ring_cv;
  U64 u2f_ring_size;
  U8 *u2f_ring_base;
  U64 u2f_ring_write_pos;
  U64 u2f_ring_read_pos;
};

typedef struct DIS_Shared DIS_Shared;
struct DIS_Shared
{
  Arena *arena;
  
  // rjf: search artifact cache table
  U64 slots_count;
  U64 stripes_count;
  DIS_Slot *slots;
  DIS_Stripe *stripes;
  
  // rjf: threads
  U64 thread_count;
  DIS_Thread *threads;
};

////////////////////////////////
//~ rjf: Globals

global DIS_Shared *dis_shared = 0;
thread_static DIS_TCTX *dis_tctx = 0;

////////////////////////////////
//~ rjf: Helpers

internal U64 dis_hash_from_string(U64 seed, String8 string);
internal U64 dis_hash_from_params(DIS_Params *params);
internal U64 dis_item_num_from_array_element_idx__linear_search(DIS_ItemArray *array, U64 element_idx);
internal String8 dis_item_string_from_rdi_target_element_idx(RDI_Parsed *rdi, RDI_SectionKind target, U64 element_idx);
internal DIS_Params dis_params_copy(Arena *arena, DIS_Params *src);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dis_init(void);

////////////////////////////////
//~ rjf: Scope Functions

internal DIS_Scope *dis_scope_open(void);
internal void dis_scope_close(DIS_Scope *scope);
internal void dis_scope_touch_node__stripe_mutex_r_guarded(DIS_Scope *scope, DIS_Node *node);

////////////////////////////////
//~ rjf: Cache Lookup Functions

internal DIS_ItemArray dis_items_from_key_params_query(DIS_Scope *scope, U128 key, DIS_Params *params, String8 query, U64 endt_us, B32 *stale_out);

////////////////////////////////
//~ rjf: Searcher Threads

internal B32 dis_u2s_enqueue_req(U128 key, U64 endt_us);
internal void dis_u2s_dequeue_req(Arena *arena, DIS_Thread *thread, U128 *key_out);

internal int dis_qsort_compare_items(DIS_Item *a, DIS_Item *b);

internal void dis_search_thread__entry_point(void *p);

#endif // DBGI_SEARCH_H
