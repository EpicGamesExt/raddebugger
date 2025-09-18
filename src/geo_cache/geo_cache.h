// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef GEO_CACHE_H
#define GEO_CACHE_H

////////////////////////////////
//~ rjf: Cache Types

typedef struct GEO_Node GEO_Node;
struct GEO_Node
{
  GEO_Node *next;
  GEO_Node *prev;
  U128 hash;
  R_Handle buffer;
  B32 is_working;
  U64 scope_ref_count;
  U64 last_time_touched_us;
  U64 last_user_clock_idx_touched;
  U64 load_count;
};

typedef struct GEO_Slot GEO_Slot;
struct GEO_Slot
{
  GEO_Node *first;
  GEO_Node *last;
};

typedef struct GEO_Stripe GEO_Stripe;
struct GEO_Stripe
{
  Arena *arena;
  RWMutex rw_mutex;
  CondVar cv;
};

////////////////////////////////
//~ rjf: Scoped Access

typedef struct GEO_Touch GEO_Touch;
struct GEO_Touch
{
  GEO_Touch *next;
  U128 hash;
};

typedef struct GEO_Scope GEO_Scope;
struct GEO_Scope
{
  GEO_Scope *next;
  GEO_Touch *top_touch;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct GEO_TCTX GEO_TCTX;
struct GEO_TCTX
{
  Arena *arena;
  GEO_Scope *free_scope;
  GEO_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct GEO_Shared GEO_Shared;
struct GEO_Shared
{
  Arena *arena;
  
  // rjf: cache
  U64 slots_count;
  U64 stripes_count;
  GEO_Slot *slots;
  GEO_Stripe *stripes;
  GEO_Node **stripes_free_nodes;
  
  // rjf: user -> xfer thread
  U64 u2x_ring_size;
  U8 *u2x_ring_base;
  U64 u2x_ring_write_pos;
  U64 u2x_ring_read_pos;
  CondVar u2x_ring_cv;
  Mutex u2x_ring_mutex;
  
  // rjf: evictor thread
  Thread evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static GEO_TCTX *geo_tctx = 0;
global GEO_Shared *geo_shared = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void geo_init(void);

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void geo_tctx_ensure_inited(void);

////////////////////////////////
//~ rjf: Scoped Access

internal GEO_Scope *geo_scope_open(void);
internal void geo_scope_close(GEO_Scope *scope);
internal void geo_scope_touch_node__stripe_r_guarded(GEO_Scope *scope, GEO_Node *node);

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle geo_buffer_from_hash(GEO_Scope *scope, U128 hash);
internal R_Handle geo_buffer_from_key(GEO_Scope *scope, C_Key key);

////////////////////////////////
//~ rjf: Transfer Threads

internal B32 geo_u2x_enqueue_req(U128 hash, U64 endt_us);
internal void geo_u2x_dequeue_req(U128 *hash_out);
ASYNC_WORK_DEF(geo_xfer_work);

////////////////////////////////
//~ rjf: Evictor Threads

internal void geo_evictor_thread__entry_point(void *p);

#endif //GEO_CACHE_H
