// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FILE_STREAM_H
#define FILE_STREAM_H

////////////////////////////////
//~ rjf: Per-Path Info Cache Types

typedef struct FS_Node FS_Node;
struct FS_Node
{
  FS_Node *next;
  String8 path;
  U64 timestamp;
  U64 last_time_requested_us;
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
  
  // rjf: streamer threads
  U64 streamer_count;
  OS_Handle *streamers;
  
  // rjf: change detector threads
  OS_Handle detector_thread;
};

////////////////////////////////
//~ rjf: Globals

global FS_Shared *fs_shared = 0;

////////////////////////////////
//~ rjf: Top-Level API

internal void fs_init(void);

////////////////////////////////
//~ rjf: Cache Interaction

internal U128 fs_hash_from_path(String8 path, U64 endt_us);
internal U128 fs_key_from_path(String8 path);

////////////////////////////////
//~ rjf: Streamer Threads

internal B32 fs_u2s_enqueue_path(String8 path, U64 endt_us);
internal String8 fs_u2s_dequeue_path(Arena *arena);

internal void fs_streamer_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Change Detector Thread

internal void fs_detector_thread__entry_point(void *p);

#endif // FILE_STREAM_H
