// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_ENGINE_CORE_H
#define DBG_ENGINE_CORE_H

////////////////////////////////
//~ rjf: ID Types

typedef U64 D_MsgID;
typedef U64 D_MachineID;

#define D_MachineID_Local (1)

////////////////////////////////
//~ rjf: Entity Handle Types

typedef struct D_Handle D_Handle;
struct D_Handle
{
  D_MachineID machine_id;
  DMN_Handle dmn_handle;
};

typedef struct D_HandleNode D_HandleNode;
struct D_HandleNode
{
  D_HandleNode *next;
  D_Handle v;
};

typedef struct D_HandleList D_HandleList;
struct D_HandleList
{
  D_HandleNode *first;
  D_HandleNode *last;
  U64 count;
};

typedef struct D_HandleArray D_HandleArray;
struct D_HandleArray
{
  D_Handle *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_engine.meta.h"

////////////////////////////////
//~ rjf: User Breakpoint Types

typedef U32 D_BreakpointFlags;
enum
{
  D_BreakpointFlag_BreakOnWrite   = (1<<0),
  D_BreakpointFlag_BreakOnRead    = (1<<1),
  D_BreakpointFlag_BreakOnExecute = (1<<2),
};

typedef struct D_Breakpoint D_Breakpoint;
struct D_Breakpoint
{
  D_BreakpointFlags flags;
  U64 id;
  String8 file_path;
  TxtPt pt;
  String8 vaddr_expr;
  String8 condition;
  U64 size;
};

typedef struct D_BreakpointArray D_BreakpointArray;
struct D_BreakpointArray
{
  D_Breakpoint *v;
  U64 count;
};

typedef struct D_BreakpointNode D_BreakpointNode;
struct D_BreakpointNode
{
  D_BreakpointNode *next;
  D_Breakpoint v;
};

typedef struct D_BreakpointList D_BreakpointList;
struct D_BreakpointList
{
  D_BreakpointNode *first;
  D_BreakpointNode *last;
  U64 count;
};

////////////////////////////////
//~ Dynamic Linker Types

typedef U32 D_TlsModel;
enum
{
  D_TlsModel_Null,
  D_TlsModel_WinodwsNt,
  D_TlsModel_Gnu
};

////////////////////////////////
//~ rjf: Entity Types

typedef struct D_Entity D_Entity;
struct D_Entity
{
  D_Entity *first;
  D_Entity *last;
  D_Entity *next;
  D_Entity *prev;
  D_Entity *parent;
  D_EntityKind kind;
  Arch arch;
  B32 is_frozen;
  B32 is_soloed;
  U32 rgba;
  D_Handle handle;
  U64 id;
  Rng1U64 vaddr_range;
  U64 stack_base;
  U64 timestamp;
  D_BreakpointFlags bp_flags;
  String8 string;
  D_TlsModel tls_model;
  U64 tls_index;
  U64 tls_offset;
  OperatingSystem target_os;
};

typedef struct D_EntityNode D_EntityNode;
struct D_EntityNode
{
  D_EntityNode *next;
  D_Entity *v;
};

typedef struct D_EntityList D_EntityList;
struct D_EntityList
{
  D_EntityNode *first;
  D_EntityNode *last;
  U64 count;
};

typedef struct D_EntityArray D_EntityArray;
struct D_EntityArray
{
  D_Entity **v;
  U64 count;
};

typedef struct D_EntityRec D_EntityRec;
struct D_EntityRec
{
  D_Entity *next;
  S32 push_count;
  S64 pop_count;
};

typedef struct D_EntityHashNode D_EntityHashNode;
struct D_EntityHashNode
{
  D_EntityHashNode *next;
  D_EntityHashNode *prev;
  D_Entity *entity;
};

typedef struct D_EntityHashSlot D_EntityHashSlot;
struct D_EntityHashSlot
{
  D_EntityHashNode *first;
  D_EntityHashNode *last;
};

typedef struct D_EntityStringChunkNode D_EntityStringChunkNode;
struct D_EntityStringChunkNode
{
  D_EntityStringChunkNode *next;
  U64 size;
};

read_only global U64 d_entity_string_bucket_chunk_sizes[] =
{
  16,
  64,
  256,
  1024,
  4096,
  16384,
  65536,
  0xffffffffffffffffull,
};

typedef struct D_EntityCtx D_EntityCtx;
struct D_EntityCtx
{
  D_Entity *root;
  U64 hash_slots_count;
  D_EntityHashSlot *hash_slots;
  U64 entity_kind_counts[D_EntityKind_COUNT];
  U64 entity_kind_alloc_gens[D_EntityKind_COUNT];
};

typedef struct D_EntityCtxRWStore D_EntityCtxRWStore;
struct D_EntityCtxRWStore
{
  Arena *arena;
  D_EntityCtx ctx;
  D_Entity *free;
  D_EntityHashNode *hash_node_free;
  D_EntityStringChunkNode *free_string_chunks[ArrayCount(d_entity_string_bucket_chunk_sizes)];
};

typedef struct D_EntityCtxLookupAccel D_EntityCtxLookupAccel;
struct D_EntityCtxLookupAccel
{
  Arena *arena;
  Arena *entity_kind_arrays_arenas[D_EntityKind_COUNT];
  D_EntityArray entity_kind_arrays[D_EntityKind_COUNT];
  U64 entity_kind_arrays_gens[D_EntityKind_COUNT];
};

////////////////////////////////
//~ rjf: Tick Input Types

typedef struct D_Target D_Target;
struct D_Target
{
  String8 exe;
  String8 args;
  String8 working_directory;
  String8 custom_entry_point_name;
  String8 stdout_path;
  String8 stderr_path;
  String8 stdin_path;
  B32 debug_subprocesses;
  String8List env;
};

typedef struct D_TargetArray D_TargetArray;
struct D_TargetArray
{
  D_Target *v;
  U64 count;
};

typedef struct D_PathMap D_PathMap;
struct D_PathMap
{
  String8 src;
  String8 dst;
};

typedef struct D_PathMapArray D_PathMapArray;
struct D_PathMapArray
{
  D_PathMap *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Trap Types

typedef U32 D_TrapFlags;
enum
{
  D_TrapFlag_IgnoreStackPointerCheck = (1<<0),
  D_TrapFlag_SingleStepAfterHit      = (1<<1),
  D_TrapFlag_SaveStackPointer        = (1<<2),
  D_TrapFlag_BeginSpoofMode          = (1<<3),
  D_TrapFlag_EndStepping             = (1<<4),
};

typedef struct D_Trap D_Trap;
struct D_Trap
{
  D_TrapFlags flags;
  U64 vaddr;
};

typedef struct D_TrapNode D_TrapNode;
struct D_TrapNode
{
  D_TrapNode *next;
  D_Trap v;
};

typedef struct D_TrapList D_TrapList;
struct D_TrapList
{
  D_TrapNode *first;
  D_TrapNode *last;
  U64 count;
};

typedef struct D_Spoof D_Spoof;
struct D_Spoof
{
  DMN_Handle process;
  DMN_Handle thread;
  U64 vaddr;
  U64 new_ip_value;
};

////////////////////////////////
//~ rjf: Trap Nets

typedef struct D_TrapNet D_TrapNet;
struct D_TrapNet
{
  D_TrapList traps;
  B32 good_line_info;
  B32 good_read;
};

////////////////////////////////
//~ rjf: Layer Initialization

internal void d_init(void);

#endif // DBG_ENGINE_CORE_H
