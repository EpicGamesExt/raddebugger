// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ARTIFACT_CACHE_H
#define ARTIFACT_CACHE_H

////////////////////////////////
//~ rjf: Artifact Handle Type

typedef struct AC_Artifact AC_Artifact;
struct AC_Artifact
{
  U64 u64[4];
};

////////////////////////////////
//~ rjf: Artifact Computation Function Types

typedef AC_Artifact AC_CreateFunctionType(String8 key, B32 *retry_out);
typedef void AC_DestroyFunctionType(AC_Artifact artifact);

typedef struct AC_ArtifactParams AC_ArtifactParams;
struct AC_ArtifactParams
{
  AC_CreateFunctionType *create;
  AC_DestroyFunctionType *destroy;
  U64 slots_count;
  U64 gen;
  B32 wait_for_fresh;
  B32 *stale_out;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct AC_Request AC_Request;
struct AC_Request
{
  String8 key;
  U64 gen;
  AC_CreateFunctionType *create;
};

typedef struct AC_RequestNode AC_RequestNode;
struct AC_RequestNode
{
  AC_RequestNode *next;
  AC_Request v;
};

typedef struct AC_Node AC_Node;
struct AC_Node
{
  AC_Node *next;
  AC_Node *prev;
  
  // rjf: key/gen/value
  String8 key;
  U64 gen;
  AC_Artifact val;
  
  // rjf: metadata
  AccessPt access_pt;
  U64 working_count;
  U64 completion_count;
};

typedef struct AC_Slot AC_Slot;
struct AC_Slot
{
  AC_Node *first;
  AC_Node *last;
};

typedef struct AC_Cache AC_Cache;
struct AC_Cache
{
  // rjf: link / key for cache-cache
  AC_Cache *next;
  AC_CreateFunctionType *create;
  AC_DestroyFunctionType *destroy;
  
  // rjf: artifact cache
  U64 slots_count;
  AC_Slot *slots;
  StripeArray stripes;
};

typedef struct AC_Shared AC_Shared;
struct AC_Shared
{
  Arena *arena;
  
  // rjf: cache cache
  U64 cache_slots_count;
  AC_Cache **cache_slots;
  StripeArray cache_stripes;
  
  // rjf: requests
  Mutex req_mutex;
  Arena *req_arena;
  AC_RequestNode *first_req;
  AC_RequestNode *last_req;
  U64 req_count;
};

////////////////////////////////
//~ rjf: Globals

global AC_Shared *ac_shared = 0;

////////////////////////////////
//~ rjf: Layer Initialization

internal void ac_init(void);

////////////////////////////////
//~ rjf: Cache Lookups

internal AC_Artifact ac_artifact_from_key_(Access *access, String8 key, AC_ArtifactParams *params, U64 endt_us);
#define ac_artifact_from_key(access, key, create_fn, destroy_fn, endt_us, ...) ac_artifact_from_key_((access), (key), &(AC_ArtifactParams){.create = (create_fn), .destroy = (destroy_fn), __VA_ARGS__}, (endt_us))

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void ac_async_tick(void);

#endif // ARTIFACT_CACHE_H
