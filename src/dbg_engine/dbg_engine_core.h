// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_ENGINE_CORE_H
#define DBG_ENGINE_CORE_H

////////////////////////////////
//~ rjf: ID Types

typedef U64 CTRL_MsgID;
typedef U64 CTRL_MachineID;

#define CTRL_MachineID_Local (1)

////////////////////////////////
//~ rjf: Entity Handle Types

typedef struct CTRL_Handle CTRL_Handle;
struct CTRL_Handle
{
  CTRL_MachineID machine_id;
  DMN_Handle dmn_handle;
};

typedef struct CTRL_HandleNode CTRL_HandleNode;
struct CTRL_HandleNode
{
  CTRL_HandleNode *next;
  CTRL_Handle v;
};

typedef struct CTRL_HandleList CTRL_HandleList;
struct CTRL_HandleList
{
  CTRL_HandleNode *first;
  CTRL_HandleNode *last;
  U64 count;
};

typedef struct CTRL_HandleArray CTRL_HandleArray;
struct CTRL_HandleArray
{
  CTRL_Handle *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_engine.meta.h"

////////////////////////////////
//~ rjf: User Breakpoint Types

typedef U32 CTRL_UserBreakpointFlags;
enum
{
  CTRL_UserBreakpointFlag_BreakOnWrite   = (1<<0),
  CTRL_UserBreakpointFlag_BreakOnRead    = (1<<1),
  CTRL_UserBreakpointFlag_BreakOnExecute = (1<<2),
};

typedef enum CTRL_UserBreakpointKind
{
  CTRL_UserBreakpointKind_Null,
  CTRL_UserBreakpointKind_FileNameAndLineColNumber,
  CTRL_UserBreakpointKind_Expression,
  CTRL_UserBreakpointKind_COUNT
}
CTRL_UserBreakpointKind;

typedef struct CTRL_UserBreakpoint CTRL_UserBreakpoint;
struct CTRL_UserBreakpoint
{
  CTRL_UserBreakpointKind kind;
  CTRL_UserBreakpointFlags flags;
  U64 id;
  String8 string;
  TxtPt pt;
  U64 size;
  String8 condition;
};

typedef struct CTRL_UserBreakpointNode CTRL_UserBreakpointNode;
struct CTRL_UserBreakpointNode
{
  CTRL_UserBreakpointNode *next;
  CTRL_UserBreakpoint v;
};

typedef struct CTRL_UserBreakpointList CTRL_UserBreakpointList;
struct CTRL_UserBreakpointList
{
  CTRL_UserBreakpointNode *first;
  CTRL_UserBreakpointNode *last;
  U64 count;
};

////////////////////////////////
//~ Dynamic Linker Types

typedef U32 CTRL_TlsModel;
enum
{
  CTRL_TlsModel_Null,
  CTRL_TlsModel_WinodwsNt,
  CTRL_TlsModel_Gnu
};

////////////////////////////////
//~ rjf: Entity Types

typedef struct CTRL_Entity CTRL_Entity;
struct CTRL_Entity
{
  CTRL_Entity *first;
  CTRL_Entity *last;
  CTRL_Entity *next;
  CTRL_Entity *prev;
  CTRL_Entity *parent;
  CTRL_EntityKind kind;
  Arch arch;
  B32 is_frozen;
  B32 is_soloed;
  U32 rgba;
  CTRL_Handle handle;
  U64 id;
  Rng1U64 vaddr_range;
  U64 stack_base;
  U64 timestamp;
  CTRL_UserBreakpointFlags bp_flags;
  String8 string;
  CTRL_TlsModel tls_model;
  U64 tls_index;
  U64 tls_offset;
  OperatingSystem target_os;
};

typedef struct CTRL_EntityNode CTRL_EntityNode;
struct CTRL_EntityNode
{
  CTRL_EntityNode *next;
  CTRL_Entity *v;
};

typedef struct CTRL_EntityList CTRL_EntityList;
struct CTRL_EntityList
{
  CTRL_EntityNode *first;
  CTRL_EntityNode *last;
  U64 count;
};

typedef struct CTRL_EntityArray CTRL_EntityArray;
struct CTRL_EntityArray
{
  CTRL_Entity **v;
  U64 count;
};

typedef struct CTRL_EntityRec CTRL_EntityRec;
struct CTRL_EntityRec
{
  CTRL_Entity *next;
  S32 push_count;
  S64 pop_count;
};

typedef struct CTRL_EntityHashNode CTRL_EntityHashNode;
struct CTRL_EntityHashNode
{
  CTRL_EntityHashNode *next;
  CTRL_EntityHashNode *prev;
  CTRL_Entity *entity;
};

typedef struct CTRL_EntityHashSlot CTRL_EntityHashSlot;
struct CTRL_EntityHashSlot
{
  CTRL_EntityHashNode *first;
  CTRL_EntityHashNode *last;
};

typedef struct CTRL_EntityStringChunkNode CTRL_EntityStringChunkNode;
struct CTRL_EntityStringChunkNode
{
  CTRL_EntityStringChunkNode *next;
  U64 size;
};

read_only global U64 ctrl_entity_string_bucket_chunk_sizes[] =
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

typedef struct CTRL_EntityCtx CTRL_EntityCtx;
struct CTRL_EntityCtx
{
  CTRL_Entity *root;
  U64 hash_slots_count;
  CTRL_EntityHashSlot *hash_slots;
  U64 entity_kind_counts[CTRL_EntityKind_COUNT];
  U64 entity_kind_alloc_gens[CTRL_EntityKind_COUNT];
};

typedef struct CTRL_EntityCtxRWStore CTRL_EntityCtxRWStore;
struct CTRL_EntityCtxRWStore
{
  Arena *arena;
  CTRL_EntityCtx ctx;
  CTRL_Entity *free;
  CTRL_EntityHashNode *hash_node_free;
  CTRL_EntityStringChunkNode *free_string_chunks[ArrayCount(ctrl_entity_string_bucket_chunk_sizes)];
};

typedef struct CTRL_EntityCtxLookupAccel CTRL_EntityCtxLookupAccel;
struct CTRL_EntityCtxLookupAccel
{
  Arena *arena;
  Arena *entity_kind_arrays_arenas[CTRL_EntityKind_COUNT];
  CTRL_EntityArray entity_kind_arrays[CTRL_EntityKind_COUNT];
  U64 entity_kind_arrays_gens[CTRL_EntityKind_COUNT];
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
//~ rjf: Tick Output Types

typedef enum D_EventKind
{
  D_EventKind_Null,
  D_EventKind_ModuleLoad,
  D_EventKind_ProcessEnd,
  D_EventKind_Stop,
  D_EventKind_COUNT
}
D_EventKind;

typedef enum D_EventCause
{
  D_EventCause_Null,
  D_EventCause_UserBreakpoint,
  D_EventCause_Halt,
  D_EventCause_SoftHalt,
  D_EventCause_COUNT
}
D_EventCause;

typedef struct D_Event D_Event;
struct D_Event
{
  D_EventKind kind;
  D_EventCause cause;
  CTRL_Handle module;
  CTRL_Handle thread;
  U64 vaddr;
  U64 code;
  U64 id;
};

typedef struct D_EventNode D_EventNode;
struct D_EventNode
{
  D_EventNode *next;
  D_Event v;
};

typedef struct D_EventList D_EventList;
struct D_EventList
{
  D_EventNode *first;
  D_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Trap Types

typedef U32 CTRL_TrapFlags;
enum
{
  CTRL_TrapFlag_IgnoreStackPointerCheck = (1<<0),
  CTRL_TrapFlag_SingleStepAfterHit      = (1<<1),
  CTRL_TrapFlag_SaveStackPointer        = (1<<2),
  CTRL_TrapFlag_BeginSpoofMode          = (1<<3),
  CTRL_TrapFlag_EndStepping             = (1<<4),
};

typedef struct CTRL_Trap CTRL_Trap;
struct CTRL_Trap
{
  CTRL_TrapFlags flags;
  U64 vaddr;
};

typedef struct CTRL_TrapNode CTRL_TrapNode;
struct CTRL_TrapNode
{
  CTRL_TrapNode *next;
  CTRL_Trap v;
};

typedef struct CTRL_TrapList CTRL_TrapList;
struct CTRL_TrapList
{
  CTRL_TrapNode *first;
  CTRL_TrapNode *last;
  U64 count;
};

typedef struct CTRL_Spoof CTRL_Spoof;
struct CTRL_Spoof
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
  CTRL_TrapList traps;
  B32 good_line_info;
  B32 good_read;
};

////////////////////////////////
//~ rjf: Layer Initialization

internal void d_init(void);

#endif // DBG_ENGINE_CORE_H
