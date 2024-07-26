// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

////////////////////////////////
//~ rjf: Texture Topology

typedef struct TEX_Topology TEX_Topology;
struct TEX_Topology
{
  Vec2S16 dim;
  R_Tex2DFormat fmt;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct TEX_Node TEX_Node;
struct TEX_Node
{
  TEX_Node *next;
  TEX_Node *prev;
  U128 hash;
  TEX_Topology topology;
  R_Handle texture;
  B32 is_working;
  U64 scope_ref_count;
  U64 last_time_touched_us;
  U64 last_user_clock_idx_touched;
  U64 load_count;
};

typedef struct TEX_Slot TEX_Slot;
struct TEX_Slot
{
  TEX_Node *first;
  TEX_Node *last;
};

typedef struct TEX_Stripe TEX_Stripe;
struct TEX_Stripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access

typedef struct TEX_Touch TEX_Touch;
struct TEX_Touch
{
  TEX_Touch *next;
  U128 hash;
  TEX_Topology topology;
};

typedef struct TEX_Scope TEX_Scope;
struct TEX_Scope
{
  TEX_Scope *next;
  TEX_Touch *top_touch;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct TEX_TCTX TEX_TCTX;
struct TEX_TCTX
{
  Arena *arena;
  TEX_Scope *free_scope;
  TEX_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct TEX_Shared TEX_Shared;
struct TEX_Shared
{
  Arena *arena;
  
  // rjf: user clock
  U64 user_clock_idx;
  
  // rjf: cache
  U64 slots_count;
  U64 stripes_count;
  TEX_Slot *slots;
  TEX_Stripe *stripes;
  TEX_Node **stripes_free_nodes;
  
  // rjf: user -> xfer thread
  U64 u2x_ring_size;
  U8 *u2x_ring_base;
  U64 u2x_ring_write_pos;
  U64 u2x_ring_read_pos;
  OS_Handle u2x_ring_cv;
  OS_Handle u2x_ring_mutex;
  
  // rjf: transfer threads
  U64 xfer_thread_count;
  OS_Handle *xfer_threads;
  
  // rjf: evictor thread
  OS_Handle evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static TEX_TCTX *tex_tctx = 0;
global TEX_Shared *tex_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal TEX_Topology tex_topology_make(Vec2S32 dim, R_Tex2DFormat fmt);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void tex_init(void);

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void tex_tctx_ensure_inited(void);

////////////////////////////////
//~ rjf: User Clock

internal void tex_user_clock_tick(void);
internal U64 tex_user_clock_idx(void);

////////////////////////////////
//~ rjf: Scoped Access

internal TEX_Scope *tex_scope_open(void);
internal void tex_scope_close(TEX_Scope *scope);
internal void tex_scope_touch_node__stripe_r_guarded(TEX_Scope *scope, TEX_Node *node);

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle tex_texture_from_hash_topology(TEX_Scope *scope, U128 hash, TEX_Topology topology);
internal R_Handle tex_texture_from_key_topology(TEX_Scope *scope, U128 key, TEX_Topology topology, U128 *hash_out);

////////////////////////////////
//~ rjf: Transfer Threads

internal B32 tex_u2x_enqueue_req(U128 hash, TEX_Topology top, U64 endt_us);
internal void tex_u2x_dequeue_req(U128 *hash_out, TEX_Topology *top_out);
internal void tex_xfer_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Evictor Threads

internal void tex_evictor_thread__entry_point(void *p);

#endif //TEXTURE_CACHE_H
