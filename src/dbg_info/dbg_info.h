// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_INFO_H
#define DBG_INFO_H

////////////////////////////////
//~ rjf: Unique Debug Info Key

typedef struct DI_Key DI_Key;
struct DI_Key
{
  U64 u64[2];
};

typedef struct DI_KeyNode DI_KeyNode;
struct DI_KeyNode
{
  DI_KeyNode *next;
  DI_Key v;
};

typedef struct DI_KeyList DI_KeyList;
struct DI_KeyList
{
  DI_KeyNode *first;
  DI_KeyNode *last;
  U64 count;
};

typedef struct DI_KeyArray DI_KeyArray;
struct DI_KeyArray
{
  DI_Key *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Debug Info Path / Timestamp => Key Cache Types

typedef struct DI_KeyPathNode DI_KeyPathNode;
struct DI_KeyPathNode
{
  DI_KeyPathNode *next;
  DI_KeyPathNode *prev;
  String8 path;
  U64 min_timestamp;
  DI_Key key;
};

typedef struct DI_KeySlot DI_KeySlot;
struct DI_KeySlot
{
  DI_KeyPathNode *first;
  DI_KeyPathNode *last;
};

////////////////////////////////
//~ rjf: Debug Info Cache Types

typedef struct DI_Node DI_Node;
struct DI_Node
{
  // rjf: links
  DI_Node *next;
  DI_Node *prev;
  
  // rjf: key
  DI_Key key;
  
  // rjf: value
  OS_Handle file;
  OS_Handle file_map;
  void *file_base;
  FileProperties file_props;
  Arena *arena;
  RDI_Parsed rdi;
  
  // rjf: metadata
  AccessPt access_pt;
  U64 refcount;
  U64 batch_request_counts[2];
  U64 working_count;
  U64 completion_count;
};

typedef struct DI_Slot DI_Slot;
struct DI_Slot
{
  DI_Node *first;
  DI_Node *last;
};

////////////////////////////////
//~ rjf: Requests

typedef struct DI_Request DI_Request;
struct DI_Request
{
  DI_Key key;
};

typedef struct DI_RequestNode DI_RequestNode;
struct DI_RequestNode
{
  DI_RequestNode *next;
  DI_Request v;
};

typedef struct DI_RequestBatch DI_RequestBatch;
struct DI_RequestBatch
{
  Mutex mutex;
  Arena *arena;
  DI_RequestNode *first;
  DI_RequestNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Load Tasks

typedef enum DI_LoadTaskStatus
{
  DI_LoadTaskStatus_Null,
  DI_LoadTaskStatus_Active,
  DI_LoadTaskStatus_Done,
}
DI_LoadTaskStatus;

typedef struct DI_LoadTask DI_LoadTask;
struct DI_LoadTask
{
  DI_LoadTask *next;
  DI_LoadTask *prev;
  
  DI_Key key;
  DI_LoadTaskStatus status;
  
  B32 og_analyzed;
  B32 og_is_rdi;
  U64 og_size;
  
  B32 rdi_analyzed;
  B32 rdi_is_stale;
  
  U64 thread_count;
  OS_Handle process;
};

typedef struct DI_LoadCompletion DI_LoadCompletion;
struct DI_LoadCompletion
{
  DI_LoadCompletion *next;
  U64 code;
};

////////////////////////////////
//~ rjf: Search Types

typedef struct DI_SearchItem DI_SearchItem;
struct DI_SearchItem
{
  U64 idx;
  DI_Key key;
  U64 missed_size;
  FuzzyMatchRangeList match_ranges;
};

typedef struct DI_SearchItemChunk DI_SearchItemChunk;
struct DI_SearchItemChunk
{
  DI_SearchItemChunk *next;
  U64 base_idx;
  DI_SearchItem *v;
  U64 count;
  U64 cap;
};

typedef struct DI_SearchItemChunkList DI_SearchItemChunkList;
struct DI_SearchItemChunkList
{
  DI_SearchItemChunk *first;
  DI_SearchItemChunk *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct DI_SearchItemArray DI_SearchItemArray;
struct DI_SearchItemArray
{
  DI_SearchItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Match Types

typedef struct DI_Match DI_Match;
struct DI_Match
{
  DI_Key key;
  RDI_SectionKind section_kind;
  U32 idx;
};

////////////////////////////////
//~ rjf: Events

typedef enum DI_EventKind
{
  DI_EventKind_Null,
  DI_EventKind_ConversionStarted,
  DI_EventKind_ConversionEnded,
  DI_EventKind_COUNT
}
DI_EventKind;

typedef struct DI_Event DI_Event;
struct DI_Event
{
  DI_EventKind kind;
  String8 string;
};

typedef struct DI_EventNode DI_EventNode;
struct DI_EventNode
{
  DI_EventNode *next;
  DI_Event v;
};

typedef struct DI_EventList DI_EventList;
struct DI_EventList
{
  DI_EventNode *first;
  DI_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct DI_Shared DI_Shared;
struct DI_Shared
{
  Arena *arena;
  U64 load_gen;
  U64 load_count;
  
  // rjf: key -> path cache
  U64 key2path_slots_count;
  DI_KeySlot *key2path_slots;
  StripeArray key2path_stripes;
  
  // rjf: path -> key cache
  U64 path2key_slots_count;
  DI_KeySlot *path2key_slots;
  StripeArray path2key_stripes;
  
  // rjf: debug info cache
  U64 slots_count;
  DI_Slot *slots;
  StripeArray stripes;
  
  // rjf: requests
  DI_RequestBatch req_batches[2]; // [0] -> high priority, [1] -> low priority
  
  // rjf: conversion tasks
  DI_LoadTask *first_load_task;
  DI_LoadTask *last_load_task;
  DI_LoadTask *free_load_task;
  U64 conversion_process_count;
  U64 conversion_thread_count;
  
  // rjf: conversion completion receiving thread
  U64 conversion_completion_code;
  String8 conversion_completion_lock_semaphore_name;
  String8 conversion_completion_signal_semaphore_name;
  String8 conversion_completion_shared_memory_name;
  Semaphore conversion_completion_lock_semaphore;
  Semaphore conversion_completion_signal_semaphore;
  OS_Handle conversion_completion_shared_memory;
  U64 *conversion_completion_shared_memory_base;
  Thread conversion_completion_signal_receiver_thread;
  
  // rjf: completion batch
  Mutex completion_mutex;
  Arena *completion_arena;
  DI_LoadCompletion *first_completion;
  DI_LoadCompletion *last_completion;
  
  // rjf: events
  Mutex event_mutex;
  Arena *event_arena;
  DI_EventList events;
};

////////////////////////////////
//~ rjf: Globals

global DI_Shared *di_shared = 0;

////////////////////////////////
//~ rjf: Helpers

internal DI_Key di_key_zero(void);
internal B32 di_key_match(DI_Key a, DI_Key b);
internal void di_key_list_push(Arena *arena, DI_KeyList *list, DI_Key key);
internal DI_KeyArray di_key_array_from_list(Arena *arena, DI_KeyList *list);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void di_init(CmdLine *cmdline);

////////////////////////////////
//~ rjf: Path * Timestamp Cache Submission & Lookup

internal DI_Key di_key_from_path_timestamp(String8 path, U64 min_timestamp);

////////////////////////////////
//~ rjf: Debug Info Opening / Closing

internal void di_open(DI_Key key);
internal void di_close(DI_Key key);

////////////////////////////////
//~ rjf: Debug Info Lookups

internal U64 di_load_gen(void);
internal U64 di_load_count(void);
internal DI_KeyArray di_push_all_loaded_keys(Arena *arena);
internal RDI_Parsed *di_rdi_from_key(Access *access, DI_Key key, B32 high_priority, U64 endt_us);

////////////////////////////////
//~ rjf: Events

internal DI_EventList di_get_events(Arena *arena);

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void di_async_tick(void);

////////////////////////////////
//~ rjf: Conversion Completion Signal Receiver Thread

internal void di_signal_completion(void);
internal void di_conversion_completion_signal_receiver_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Search Artifact Cache Hooks / Lookups

internal AC_Artifact di_search_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out);
internal void di_search_artifact_destroy(AC_Artifact artifact);
internal DI_SearchItemArray di_search_item_array_from_target_query(Access *access, RDI_SectionKind target, String8 query, U64 endt_us, B32 *stale_out);

////////////////////////////////
//~ rjf: Match Artifact Cache Hooks / Lookups

internal AC_Artifact di_match_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out);
internal DI_Match di_match_from_string(String8 string, U64 index, DI_Key preferred_dbgi_key, U64 endt_us);

#endif // DBG_INFO_H
