// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FILE_STREAM_H
#define FILE_STREAM_H

////////////////////////////////
//~ rjf: Per-Path Info Cache Types

typedef struct FS_RangeNode FS_RangeNode;
struct FS_RangeNode
{
  FS_RangeNode *next;
  HS_ID id;
  U64 working_count;
};

typedef struct FS_RangeSlot FS_RangeSlot;
struct FS_RangeSlot
{
  FS_RangeNode *first;
  FS_RangeNode *last;
};

typedef struct FS_Node FS_Node;
struct FS_Node
{
  FS_Node *next;
  
  // rjf: file metadata
  String8 path;
  FileProperties props;
  
  // rjf: hash store root
  HS_Root root;
  
  // rjf: sub-table of per-requested-file-range info
  U64 slots_count;
  FS_RangeSlot *slots;
};

typedef struct FS_Slot FS_Slot;
struct FS_Slot
{
  FS_Node *first;
  FS_Node *last;
};

typedef struct FS_Stripe FS_Stripe;
struct FS_Stripe
{
  Arena *arena;
  CondVar cv;
  RWMutex rw_mutex;
};

////////////////////////////////
//~ rjf: Shared State Bundle

typedef struct FS_Request FS_Request;
struct FS_Request
{
  FS_Request *next;
  HS_Key key;
  String8 path;
  Rng1U64 range;
};

typedef struct FS_RequestNode FS_RequestNode;
struct FS_RequestNode
{
  FS_RequestNode *next;
  FS_Request v;
};

typedef struct FS_Shared FS_Shared;
struct FS_Shared
{
  Arena *arena;
  U64 change_gen;
  
  // rjf: path info cache
  U64 slots_count;
  U64 stripes_count;
  FS_Slot *slots;
  FS_Stripe *stripes;
  
  // rjf: requests
  Mutex req_mutex;
  Arena *req_arena;
  FS_RequestNode *first_req;
  FS_RequestNode *last_req;
  U64 req_count;
};

////////////////////////////////
//~ rjf: Globals

global FS_Shared *fs_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 fs_little_hash_from_string(String8 string);
internal U128 fs_big_hash_from_string_range(String8 string, Rng1U64 range);

////////////////////////////////
//~ rjf: Top-Level API

internal void fs_init(void);

////////////////////////////////
//~ rjf: Change Generation

internal U64 fs_change_gen(void);

////////////////////////////////
//~ rjf: Cache Interaction

internal HS_Key fs_key_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
internal U128 fs_hash_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
internal FileProperties fs_properties_from_path(String8 path);

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void fs_async_tick(void);

#endif // FILE_STREAM_H
