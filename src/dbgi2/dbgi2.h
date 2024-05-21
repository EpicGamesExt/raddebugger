// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DI_H
#define DI_H

////////////////////////////////
//~ rjf: Event Types

typedef enum DI_EventKind
{
  DI_EventKind_Null,
  DI_EventKind_ConversionStarted,
  DI_EventKind_ConversionEnded,
  DI_EventKind_ConversionFailureUnsupportedFormat,
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
//~ rjf: Cache Types

typedef struct DI_StringChunkNode DI_StringChunkNode;
struct DI_StringChunkNode
{
  DI_StringChunkNode *next;
  U64 size;
};

typedef struct DI_Node DI_Node;
struct DI_Node
{
  // rjf: links
  DI_Node *next;
  DI_Node *prev;
  
  // rjf: metadata
  U64 ref_count;
  U64 touch_count;
  U64 is_working;
  U64 last_time_requested_us;
  
  // rjf: key
  String8 path;
  U64 min_timestamp;
  
  // rjf: file handles
  OS_Handle file;
  OS_Handle file_map;
  void *file_base;
  FileProperties file_props;
  
  // rjf: parse artifacts
  Arena *arena;
  RDI_Parsed rdi;
  B32 parse_done;
};

typedef struct DI_Slot DI_Slot;
struct DI_Slot
{
  DI_Node *first;
  DI_Node *last;
};

typedef struct DI_Stripe DI_Stripe;
struct DI_Stripe
{
  Arena *arena;
  DI_Node *free_node;
  DI_StringChunkNode *free_string_chunks[8];
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access Types

typedef struct DI_Touch DI_Touch;
struct DI_Touch
{
  DI_Touch *next;
  DI_Node *node;
};

typedef struct DI_Scope DI_Scope;
struct DI_Scope
{
  DI_Scope *next;
  DI_Touch *first_touch;
  DI_Touch *last_touch;
};

typedef struct DI_TCTX DI_TCTX;
struct DI_TCTX
{
  Arena *arena;
  DI_Scope *free_scope;
  DI_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State Types

typedef struct DI_Shared DI_Shared;
struct DI_Shared
{
  Arena *arena;
  
  // rjf: node cache
  U64 slots_count;
  DI_Slot *slots;
  U64 stripes_count;
  DI_Stripe *stripes;
  
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
};

////////////////////////////////
//~ rjf: Globals

global DI_Shared *di_shared = 0;
thread_static DI_TCTX *di_tctx = 0;
global RDI_Parsed di_rdi_parsed_nil =
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
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 di_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void di_init(void);

////////////////////////////////
//~ rjf: Scope Functions

internal DI_Scope *di_scope_open(void);
internal void di_scope_close(DI_Scope *scope);
internal void di_scope_touch_node__stripe_mutex_r_guarded(DI_Scope *scope, DI_Node *node);

////////////////////////////////
//~ rjf: Per-Slot Functions

internal DI_Node *di_node_from_path_min_timestamp_slot__stripe_mutex_r_guarded(DI_Slot *slot, String8 path, U64 min_timestamp);

////////////////////////////////
//~ rjf: Per-Stripe Functions

internal U64 di_string_bucket_idx_from_string_size(U64 size);
internal String8 di_string_alloc__stripe_mutex_w_guarded(DI_Stripe *stripe, String8 string);
internal void di_string_release__stripe_mutex_w_guarded(DI_Stripe *stripe, String8 string);

////////////////////////////////
//~ rjf: Key Opening/Closing

internal void di_open(String8 path, U64 min_timestamp);
internal void di_close(String8 path, U64 min_timestamp);

////////////////////////////////
//~ rjf: Cache Lookups

internal RDI_Parsed *di_rdi_from_path_min_timestamp(DI_Scope *scope, String8 path, U64 min_timestamp, U64 endt_us);

////////////////////////////////
//~ rjf: Parse Threads

internal B32 di_u2p_enqueue_key(String8 path, U64 min_timestamp, U64 endt_us);
internal void di_u2p_dequeue_key(Arena *arena, String8 *out_path, U64 *out_min_timestamp);

internal void di_p2u_push_event(DI_Event *event);
internal DI_EventList di_p2u_pop_events(Arena *arena, U64 endt_us);

internal void di_parse_thread__entry_point(void *p);

#endif // DI_H
