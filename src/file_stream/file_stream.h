// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FILE_STREAM_H
#define FILE_STREAM_H

////////////////////////////////
//~ rjf: Path Cache

typedef struct FS_Node FS_Node;
struct FS_Node
{
  FS_Node *next;
  String8 path;
  U64 gen;
  U64 last_modified_timestamp;
  U64 size;
};

typedef struct FS_Slot FS_Slot;
struct FS_Slot
{
  FS_Node *first;
  FS_Node *last;
};

////////////////////////////////
//~ rjf: Shared State Bundle

typedef struct FS_Shared FS_Shared;
struct FS_Shared
{
  Arena *arena;
  U64 change_gen;
  U64 slots_count;
  FS_Slot *slots;
  StripeArray stripes;
};

////////////////////////////////
//~ rjf: Globals

global FS_Shared *fs_shared = 0;

////////////////////////////////
//~ rjf: Top-Level API

internal void fs_init(void);

////////////////////////////////
//~ rjf: Change Generation

internal U64 fs_change_gen(void);

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Accessing API

internal AC_Artifact fs_artifact_create(String8 key, U64 gen, B32 *cancel_signal, B32 *retry_out);
internal void fs_artifact_destroy(AC_Artifact artifact);

internal C_Key fs_key_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
internal U128 fs_hash_from_path_range(String8 path, Rng1U64 range, U64 endt_us);
#define fs_key_from_path(path, endt_us) fs_key_from_path_range((path), r1u64(0, max_U64), (endt_us))
#define fs_hash_from_path(path, endt_us) fs_hash_from_path_range((path), r1u64(0, max_U64), (endt_us))

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void fs_async_tick(void);

#endif // FILE_STREAM_H
