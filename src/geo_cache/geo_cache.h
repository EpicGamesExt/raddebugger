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
  U64 load_count;
  AccessPt access_pt;
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

global GEO_Shared *geo_shared = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void geo_init(void);

////////////////////////////////
//~ rjf: Cache Lookups

internal R_Handle geo_buffer_from_hash(Access *access, U128 hash);
internal R_Handle geo_buffer_from_key(Access *access, C_Key key);

////////////////////////////////
//~ rjf: Transfer Threads

internal B32 geo_u2x_enqueue_req(U128 hash, U64 endt_us);
internal void geo_u2x_dequeue_req(U128 *hash_out);
ASYNC_WORK_DEF(geo_xfer_work);

////////////////////////////////
//~ rjf: Evictor Threads

internal void geo_evictor_thread__entry_point(void *p);

#endif //GEO_CACHE_H
