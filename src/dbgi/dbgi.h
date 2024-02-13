// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBGI_H
#define DBGI_H

////////////////////////////////
//~ rjf: Info Bundle Types

typedef struct DBGI_Parse DBGI_Parse;
struct DBGI_Parse
{
  U64 gen;
  Arena *arena;
  void *exe_base;
  FileProperties exe_props;
  String8 dbg_path;
  void *dbg_base;
  FileProperties dbg_props;
  PE_BinInfo pe;
  RDI_Parsed rdi;
};

////////////////////////////////
//~ rjf: Exe -> Debug Forced Override Cache Types

typedef struct DBGI_ForceNode DBGI_ForceNode;
struct DBGI_ForceNode
{
  DBGI_ForceNode *next;
  String8 exe_path;
  U64 dbg_path_cap;
  U64 dbg_path_size;
  U8 *dbg_path_base;
};

typedef struct DBGI_ForceSlot DBGI_ForceSlot;
struct DBGI_ForceSlot
{
  DBGI_ForceNode *first;
  DBGI_ForceNode *last;
};

typedef struct DBGI_ForceStripe DBGI_ForceStripe;
struct DBGI_ForceStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Binary Cache State Types

typedef U32 DBGI_BinaryFlags;
enum
{
  DBGI_BinaryFlag_ParseInFlight = (1<<0),
};

typedef struct DBGI_Binary DBGI_Binary;
struct DBGI_Binary
{
  // rjf: links & metadata
  DBGI_Binary *next;
  String8 exe_path;
  U64 refcount;
  U64 scope_touch_count;
  U64 last_time_enqueued_for_parse_us;
  DBGI_BinaryFlags flags;
  U64 gen;
  
  // rjf: exe handles
  OS_Handle exe_file;
  OS_Handle exe_file_map;
  
  // rjf: debug handles
  OS_Handle dbg_file;
  OS_Handle dbg_file_map;
  
  // rjf: analysis results
  DBGI_Parse parse;
};

typedef struct DBGI_BinarySlot DBGI_BinarySlot;
struct DBGI_BinarySlot
{
  DBGI_Binary *first;
  DBGI_Binary *last;
};

typedef struct DBGI_BinaryStripe DBGI_BinaryStripe;
struct DBGI_BinaryStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Fuzzy Search Cache Types

typedef enum DBGI_FuzzySearchTarget
{
  DBGI_FuzzySearchTarget_Procedures,
  DBGI_FuzzySearchTarget_GlobalVariables,
  DBGI_FuzzySearchTarget_ThreadVariables,
  DBGI_FuzzySearchTarget_UDTs,
  DBGI_FuzzySearchTarget_COUNT
}
DBGI_FuzzySearchTarget;

typedef struct DBGI_FuzzySearchItem DBGI_FuzzySearchItem;
struct DBGI_FuzzySearchItem
{
  U64 idx;
  U64 missed_size;
  FuzzyMatchRangeList match_ranges;
};

typedef struct DBGI_FuzzySearchItemChunk DBGI_FuzzySearchItemChunk;
struct DBGI_FuzzySearchItemChunk
{
  DBGI_FuzzySearchItemChunk *next;
  DBGI_FuzzySearchItem *v;
  U64 count;
  U64 cap;
};

typedef struct DBGI_FuzzySearchItemChunkList DBGI_FuzzySearchItemChunkList;
struct DBGI_FuzzySearchItemChunkList
{
  DBGI_FuzzySearchItemChunk *first;
  DBGI_FuzzySearchItemChunk *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct DBGI_FuzzySearchItemArray DBGI_FuzzySearchItemArray;
struct DBGI_FuzzySearchItemArray
{
  DBGI_FuzzySearchItem *v;
  U64 count;
};

typedef struct DBGI_FuzzySearchBucket DBGI_FuzzySearchBucket;
struct DBGI_FuzzySearchBucket
{
  Arena *arena;
  String8 exe_path;
  String8 query;
  DBGI_FuzzySearchTarget target;
};

typedef struct DBGI_FuzzySearchNode DBGI_FuzzySearchNode;
struct DBGI_FuzzySearchNode
{
  DBGI_FuzzySearchNode *next;
  U128 key;
  U64 scope_touch_count;
  U64 last_time_submitted_us;
  DBGI_FuzzySearchBucket buckets[3];
  U64 gen;
  U64 submit_gen;
  DBGI_FuzzySearchItemArray gen_items;
};

typedef struct DBGI_FuzzySearchSlot DBGI_FuzzySearchSlot;
struct DBGI_FuzzySearchSlot
{
  DBGI_FuzzySearchNode *first;
  DBGI_FuzzySearchNode *last;
};

typedef struct DBGI_FuzzySearchStripe DBGI_FuzzySearchStripe;
struct DBGI_FuzzySearchStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

typedef struct DBGI_FuzzySearchThread DBGI_FuzzySearchThread;
struct DBGI_FuzzySearchThread
{
  OS_Handle thread;
  OS_Handle u2f_ring_mutex;
  OS_Handle u2f_ring_cv;
  U64 u2f_ring_size;
  U8 *u2f_ring_base;
  U64 u2f_ring_write_pos;
  U64 u2f_ring_read_pos;
};

////////////////////////////////
//~ rjf: Weak Access Scope Types

typedef struct DBGI_TouchedBinary DBGI_TouchedBinary;
struct DBGI_TouchedBinary
{
  DBGI_TouchedBinary *next;
  DBGI_Binary *binary;
};

typedef struct DBGI_TouchedFuzzySearch DBGI_TouchedFuzzySearch;
struct DBGI_TouchedFuzzySearch
{
  DBGI_TouchedFuzzySearch *next;
  DBGI_FuzzySearchNode *node;
};

typedef struct DBGI_Scope DBGI_Scope;
struct DBGI_Scope
{
  DBGI_Scope *next;
  DBGI_TouchedBinary *first_tb;
  DBGI_TouchedBinary *last_tb;
  DBGI_TouchedFuzzySearch *first_tfs;
  DBGI_TouchedFuzzySearch *last_tfs;
};

typedef struct DBGI_ThreadCtx DBGI_ThreadCtx;
struct DBGI_ThreadCtx
{
  Arena *arena;
  DBGI_Scope *free_scope;
  DBGI_TouchedBinary *free_tb;
  DBGI_TouchedFuzzySearch *free_tfs;
};

////////////////////////////////
//~ rjf: Event Types

typedef enum DBGI_EventKind
{
  DBGI_EventKind_Null,
  DBGI_EventKind_ConversionStarted,
  DBGI_EventKind_ConversionEnded,
  DBGI_EventKind_ConversionFailureUnsupportedFormat,
  DBGI_EventKind_COUNT
}
DBGI_EventKind;

typedef struct DBGI_Event DBGI_Event;
struct DBGI_Event
{
  DBGI_EventKind kind;
  String8 string;
};

typedef struct DBGI_EventNode DBGI_EventNode;
struct DBGI_EventNode
{
  DBGI_EventNode *next;
  DBGI_Event v;
};

typedef struct DBGI_EventList DBGI_EventList;
struct DBGI_EventList
{
  DBGI_EventNode *first;
  DBGI_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Cross-Thread Shared State

typedef struct DBGI_Shared DBGI_Shared;
struct DBGI_Shared
{
  // rjf: arena
  Arena *arena;
  
  // rjf: forced override table
  U64 force_slots_count;
  U64 force_stripes_count;
  DBGI_ForceSlot *force_slots;
  DBGI_ForceStripe *force_stripes;
  
  // rjf: binary table
  U64 binary_slots_count;
  U64 binary_stripes_count;
  DBGI_BinarySlot *binary_slots;
  DBGI_BinaryStripe *binary_stripes;
  
  // rjf: fuzzy search cache table
  U64 fuzzy_search_slots_count;
  U64 fuzzy_search_stripes_count;
  DBGI_FuzzySearchSlot *fuzzy_search_slots;
  DBGI_FuzzySearchStripe *fuzzy_search_stripes;
  
  // rjf: user -> parse ring
  OS_Handle u2p_ring_mutex;
  OS_Handle u2p_ring_cv;
  U64 u2p_ring_size;
  U8 *u2p_ring_base;
  U64 u2p_ring_write_pos;
  U64 u2p_ring_read_pos;
  
  // rjf: parse -> user event ring
  OS_Handle p2u_ring_mutex;
  OS_Handle p2u_ring_cv;
  U64 p2u_ring_size;
  U8 *p2u_ring_base;
  U64 p2u_ring_write_pos;
  U64 p2u_ring_read_pos;
  
  // rjf: threads
  U64 parse_thread_count;
  OS_Handle *parse_threads;
  U64 fuzzy_thread_count;
  DBGI_FuzzySearchThread *fuzzy_threads;
};

////////////////////////////////
//~ rjf: Globals

global DBGI_Shared *dbgi_shared = 0;
thread_static DBGI_ThreadCtx *dbgi_tctx = 0;
global DBGI_Parse dbgi_parse_nil =
{
  0,
  0,
  0,
  {0},
  {0},
  0,
  {0},
  {0},
  {
    0,
    0,
    0,
    0,
    {0},
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    &rdi_binary_section_nil, 1,
    &rdi_file_path_node_nil, 1,
    &rdi_source_file_nil, 1,
    &rdi_unit_nil, 1,
    &rdi_vmap_entry_nil, 1,
    &rdi_type_node_nil, 1,
    &rdi_udt_nil, 1,
    &rdi_member_nil, 1,
    &rdi_enum_member_nil, 1,
    &rdi_global_variable_nil, 1,
    &rdi_vmap_entry_nil, 1,
    &rdi_thread_variable_nil, 1,
    &rdi_procedure_nil, 1,
    &rdi_scope_nil, 1,
    &rdi_voff_nil, 1,
    &rdi_vmap_entry_nil, 1,
    &rdi_local_nil, 1,
    &rdi_location_block_nil, 1,
    0, 0,
    0, 0,
  },
};

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dbgi_init(void);

////////////////////////////////
//~ rjf: Thread-Context Idempotent Initialization

internal void dbgi_ensure_tctx_inited(void);

////////////////////////////////
//~ rjf: Helpers

internal U64 dbgi_hash_from_string(String8 string);
internal U64 dbgi_fuzzy_item_num_from_array_element_idx__linear_search(DBGI_FuzzySearchItemArray *array, U64 element_idx);
internal String8 dbgi_fuzzy_item_string_from_rdi_target_element_idx(RDI_Parsed *rdi, DBGI_FuzzySearchTarget target, U64 element_idx);

////////////////////////////////
//~ rjf: Forced Override Cache Functions

internal void dbgi_force_exe_path_dbg_path(String8 exe_path, String8 dbg_path);
internal String8 dbgi_forced_dbg_path_from_exe_path(Arena *arena, String8 exe_path);

////////////////////////////////
//~ rjf: Scope Functions

internal DBGI_Scope *dbgi_scope_open(void);
internal void dbgi_scope_close(DBGI_Scope *scope);
internal void dbgi_scope_touch_binary__stripe_mutex_r_guarded(DBGI_Scope *scope, DBGI_Binary *binary);
internal void dbgi_scope_touch_fuzzy_search__stripe_mutex_r_guarded(DBGI_Scope *scope, DBGI_FuzzySearchNode *node);

////////////////////////////////
//~ rjf: Binary Cache Functions

internal void dbgi_binary_open(String8 exe_path);
internal void dbgi_binary_close(String8 exe_path);
internal DBGI_Parse *dbgi_parse_from_exe_path(DBGI_Scope *scope, String8 exe_path, U64 endt_us);

////////////////////////////////
//~ rjf: Fuzzy Search Cache Functions

internal DBGI_FuzzySearchItemArray dbgi_fuzzy_search_items_from_key_exe_query(DBGI_Scope *scope, U128 key, String8 exe_path, String8 query, DBGI_FuzzySearchTarget target, U64 endt_us, B32 *stale_out);

////////////////////////////////
//~ rjf: Parse Threads

internal B32 dbgi_u2p_enqueue_exe_path(String8 exe_path, U64 endt_us);
internal String8 dbgi_u2p_dequeue_exe_path(Arena *arena);

internal void dbgi_p2u_push_event(DBGI_Event *event);
internal DBGI_EventList dbgi_p2u_pop_events(Arena *arena, U64 endt_us);

internal void dbgi_parse_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Fuzzy Searching Threads

internal B32 dbgi_u2f_enqueue_req(U128 key, U64 endt_us);
internal void dbgi_u2f_dequeue_req(Arena *arena, DBGI_FuzzySearchThread *thread, U128 *key_out);

internal int dbgi_qsort_compare_fuzzy_search_items(DBGI_FuzzySearchItem *a, DBGI_FuzzySearchItem *b);

internal void dbgi_fuzzy_thread__entry_point(void *p);

#endif //DBGI_H
