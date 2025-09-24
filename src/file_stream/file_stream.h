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
  C_ID id;
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
  C_Root root;
  
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
  C_Key key;
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
  
  // rjf: request take counter
  U64 lane_req_take_counter;
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

internal AC_Artifact fs_artifact_create(String8 key, B32 *retry_out);
internal void fs_artifact_destroy(AC_Artifact artifact);

internal C_Key fs_key_from_path_range_new(String8 path, Rng1U64 range, U64 endt_us);
#define fs_key_from_path(path, endt_us) fs_key_from_path_range_new((path), r1u64(0, max_U64), (endt_us))

internal C_Key fs_key_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
internal U128 fs_hash_from_path_range(String8 path, Rng1U64 range, U64 endt_us);

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void fs_async_tick(void);

#endif // FILE_STREAM_H
