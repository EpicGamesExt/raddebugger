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
  AccessPt access_pt;
  U64 working_count;
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

typedef struct TEX_Request TEX_Request;
struct TEX_Request
{
  U128 hash;
  TEX_Topology top;
};

typedef struct TEX_RequestNode TEX_RequestNode;
struct TEX_RequestNode
{
  TEX_RequestNode *next;
  TEX_Request v;
};

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
  
  // rjf: requests
  Mutex req_mutex;
  Arena *req_arena;
  TEX_RequestNode *first_req;
  TEX_RequestNode *last_req;
  U64 req_count;
  U64 lane_req_take_counter;
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
//~ rjf: Tick

internal void tex_tick(void);

#endif // TEXTURE_CACHE_H
