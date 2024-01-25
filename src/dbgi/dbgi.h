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
  RADDBG_Parsed rdbg;
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
//~ rjf: Weak Access Scope Types

typedef struct DBGI_TouchedBinary DBGI_TouchedBinary;
struct DBGI_TouchedBinary
{
  DBGI_TouchedBinary *next;
  DBGI_Binary *binary;
};

typedef struct DBGI_Scope DBGI_Scope;
struct DBGI_Scope
{
  DBGI_Scope *next;
  DBGI_TouchedBinary *first_tb;
  DBGI_TouchedBinary *last_tb;
};

typedef struct DBGI_ThreadCtx DBGI_ThreadCtx;
struct DBGI_ThreadCtx
{
  Arena *arena;
  DBGI_Scope *free_scope;
  DBGI_TouchedBinary *free_tb;
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

global DBGI_Shared *dbgi_shared = 0;
thread_static DBGI_ThreadCtx *dbgi_tctx = 0;
global DBGI_Parse dbgi_parse_nil = {0};

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dbgi_init(void);

////////////////////////////////
//~ rjf: Thread-Context Idempotent Initialization

internal void dbgi_ensure_tctx_inited(void);

////////////////////////////////
//~ rjf: Helpers

internal U64 dbgi_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Forced Override Cache Functions

internal void dbgi_force_exe_path_dbg_path(String8 exe_path, String8 dbg_path);
internal String8 dbgi_forced_dbg_path_from_exe_path(Arena *arena, String8 exe_path);

////////////////////////////////
//~ rjf: Scope Functions

internal DBGI_Scope *dbgi_scope_open(void);
internal void dbgi_scope_close(DBGI_Scope *scope);
internal void dbgi_scope_touch_binary__stripe_mutex_r_guarded(DBGI_Scope *scope, DBGI_Binary *binary);

////////////////////////////////
//~ rjf: Binary Cache Functions

internal void dbgi_binary_open(String8 exe_path);
internal void dbgi_binary_close(String8 exe_path);
internal DBGI_Parse *dbgi_parse_from_exe_path(DBGI_Scope *scope, String8 exe_path, U64 endt_us);

////////////////////////////////
//~ rjf: Parse Threads

internal B32 dbgi_u2p_enqueue_exe_path(String8 exe_path, U64 endt_us);
internal String8 dbgi_u2p_dequeue_exe_path(Arena *arena);

internal void dbgi_p2u_push_event(DBGI_Event *event);
internal DBGI_EventList dbgi_p2u_pop_events(Arena *arena, U64 endt_us);

internal void dbgi_parse_thread_entry_point(void *p);

#endif //DBGI_H
