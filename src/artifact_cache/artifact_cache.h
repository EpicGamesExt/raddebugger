// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ARTIFACT_CACHE_H
#define ARTIFACT_CACHE_H

////////////////////////////////
//~ rjf: Artifact Computation Function Types

typedef void *AC_CreateFunctionType(String8 key);
typedef void AC_DestroyFunctionType(void *artifact);

////////////////////////////////
//~ rjf: Cache Types

typedef struct AC_Request AC_Request;
struct AC_Request
{
  String8 key;
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
  
  // rjf: key/value
  String8 key;
  void *val;
  
  // rjf: metadata
  AccessPt access_pt;
  U64 working_count;
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

internal void *ac_artifact_from_key(Access *access, String8 key, AC_CreateFunctionType *create, AC_DestroyFunctionType *destroy, U64 slots_count);

////////////////////////////////
//~ rjf: Tick

internal void ac_tick(void);

#endif // ARTIFACT_CACHE_H
