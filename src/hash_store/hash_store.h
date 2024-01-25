// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef HASH_STORE_H
#define HASH_STORE_H

////////////////////////////////
//~ rjf: Cache Types

typedef struct HS_KeyNode HS_KeyNode;
struct HS_KeyNode
{
  HS_KeyNode *next;
  U128 key;
  U128 hash_history[2];
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
  
  // rjf: evictor thread
  OS_Handle evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static HS_TCTX *hs_tctx = 0;
global HS_Shared *hs_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U128 hs_hash_from_data(String8 data);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void hs_init(void);

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void hs_tctx_ensure_inited(void);

////////////////////////////////
//~ rjf: Cache Submission/Derefs

internal U128 hs_submit_data(U128 key, Arena **data_arena, String8 data);

////////////////////////////////
//~ rjf: Scoped Access

internal HS_Scope *hs_scope_open(void);
internal void hs_scope_close(HS_Scope *scope);
internal void hs_scope_touch_node__stripe_r_guarded(HS_Scope *scope, HS_Node *node);

////////////////////////////////
//~ rjf: Cache Lookups

internal U128 hs_hash_from_key(U128 key);
internal String8 hs_data_from_hash(HS_Scope *scope, U128 hash);

////////////////////////////////
//~ rjf: Evictor Thread

internal void hs_evictor_thread__entry_point(void *p);

#endif // HASH_STORE_H
