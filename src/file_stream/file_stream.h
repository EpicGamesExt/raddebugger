// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FILE_STREAM_H
#define FILE_STREAM_H

////////////////////////////////
//~ rjf: Per-Path Info Cache Types

typedef struct FS_RangeNode FS_RangeNode;
struct FS_RangeNode
{
  FS_RangeNode *next;
  Rng1U64 range;
  U64 request_count;
  U64 completion_count;
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
  U64 timestamp;
  U64 size;
  
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
  OS_Handle cv;
  OS_Handle rw_mutex;
};

////////////////////////////////
//~ rjf: Shared State Bundle

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
  
  // rjf: user -> streamer ring buffer
  U64 u2s_ring_size;
  U8 *u2s_ring_base;
  U64 u2s_ring_write_pos;
  U64 u2s_ring_read_pos;
  OS_Handle u2s_ring_cv;
  OS_Handle u2s_ring_mutex;
  
  // rjf: change detector threads
  OS_Handle detector_thread;
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

internal U128 fs_hash_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
internal U128 fs_key_from_path_range(String8 path, Rng1U64 range);

internal U64 fs_timestamp_from_path(String8 path);
internal U64 fs_size_from_path(String8 path);

////////////////////////////////
//~ rjf: Streaming Work

internal B32 fs_u2s_enqueue_req(Rng1U64 range, String8 path, U64 endt_us);
internal void fs_u2s_dequeue_req(Arena *arena, Rng1U64 *range_out, String8 *path_out);
ASYNC_WORK_DEF(fs_stream_work);

////////////////////////////////
//~ rjf: Change Detector Thread

internal void fs_detector_thread__entry_point(void *p);

#endif // FILE_STREAM_H
