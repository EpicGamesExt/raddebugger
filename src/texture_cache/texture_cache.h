// Copyright (c) Epic Games Tools
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
  RWMutex rw_mutex;
  CondVar cv;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct TEX_Shared TEX_Shared;
struct TEX_Shared
{
  Arena *arena;
  
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
  CondVar u2x_ring_cv;
  Mutex u2x_ring_mutex;
  
  // rjf: evictor thread
  Thread evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

global TEX_Shared *tex_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal TEX_Topology tex_topology_make(Vec2S32 dim, R_Tex2DFormat fmt);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void tex_init(void);

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle tex_texture_from_hash_topology(Access *access, U128 hash, TEX_Topology topology);
internal R_Handle tex_texture_from_key_topology(Access *access, C_Key key, TEX_Topology topology, U128 *hash_out);

////////////////////////////////
//~ rjf: Transfer Threads

internal B32 tex_u2x_enqueue_req(U128 hash, TEX_Topology top, U64 endt_us);
internal void tex_u2x_dequeue_req(U128 *hash_out, TEX_Topology *top_out);
ASYNC_WORK_DEF(tex_xfer_work);

////////////////////////////////
//~ rjf: Evictor Threads

internal void tex_evictor_thread__entry_point(void *p);

#endif //TEXTURE_CACHE_H
