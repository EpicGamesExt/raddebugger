// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CTRL_CORE_H
#define CTRL_CORE_H

////////////////////////////////
//~ rjf: ID Types

typedef U64 CTRL_MsgID;
typedef U64 CTRL_MachineID;

#define CTRL_MachineID_Local (1)

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

////////////////////////////////
//~ rjf: Generated Code

#include "generated/ctrl.meta.h"

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
//~ rjf: Unwind Types

typedef U32 CTRL_UnwindFlags;
enum
{
  CTRL_UnwindFlag_Error = (1<<0),
  CTRL_UnwindFlag_Stale = (1<<1),
};

typedef struct CTRL_UnwindStepResult CTRL_UnwindStepResult;
struct CTRL_UnwindStepResult
{
  CTRL_UnwindFlags flags;
};

typedef struct CTRL_UnwindFrame CTRL_UnwindFrame;
struct CTRL_UnwindFrame
{
  void *regs;
};

typedef struct CTRL_UnwindFrameNode CTRL_UnwindFrameNode;
struct CTRL_UnwindFrameNode
{
  CTRL_UnwindFrameNode *next;
  CTRL_UnwindFrameNode *prev;
  CTRL_UnwindFrame v;
};

typedef struct CTRL_UnwindFrameArray CTRL_UnwindFrameArray;
struct CTRL_UnwindFrameArray
{
  CTRL_UnwindFrame *v;
  U64 count;
};

typedef struct CTRL_Unwind CTRL_Unwind;
struct CTRL_Unwind
{
  CTRL_UnwindFrameArray frames;
  CTRL_UnwindFlags flags;
};

////////////////////////////////
//~ rjf: Call Stack Types

typedef struct CTRL_CallStackFrame CTRL_CallStackFrame;
struct CTRL_CallStackFrame
{
  U64 unwind_count;
  U64 inline_depth;
  void *regs;
};

typedef struct CTRL_CallStack CTRL_CallStack;
struct CTRL_CallStack
{
  CTRL_CallStackFrame *frames;
  U64 frames_count;
  CTRL_CallStackFrame **concrete_frames;
  U64 concrete_frames_count;
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
//~ rjf: Evaluation Spaces

typedef U64 CTRL_EvalSpaceKind;
enum
{
  CTRL_EvalSpaceKind_Entity = E_SpaceKind_FirstUserDefined,
  CTRL_EvalSpaceKind_Meta,
};

////////////////////////////////
//~ rjf: Message Types

typedef enum CTRL_MsgKind
{
  CTRL_MsgKind_Null,
  CTRL_MsgKind_Launch,
  CTRL_MsgKind_Attach,
  CTRL_MsgKind_Kill,
  CTRL_MsgKind_KillAll,
  CTRL_MsgKind_Detach,
  CTRL_MsgKind_Run,
  CTRL_MsgKind_SingleStep,
  CTRL_MsgKind_SetUserEntryPoints,
  CTRL_MsgKind_SetModuleDebugInfoPath,
  CTRL_MsgKind_FreezeThread,
  CTRL_MsgKind_ThawThread,
  CTRL_MsgKind_COUNT,
}
CTRL_MsgKind;

typedef U32 CTRL_RunFlags;
enum
{
  CTRL_RunFlag_StopOnEntryPoint = (1<<0),
};

typedef struct CTRL_Msg CTRL_Msg;
struct CTRL_Msg
{
  CTRL_MsgKind kind;
  CTRL_RunFlags run_flags;
  CTRL_MsgID msg_id;
  CTRL_Handle entity;
  CTRL_Handle parent;
  U32 entity_id;
  U32 exit_code;
  B32 env_inherit;
  B32 debug_subprocesses;
  U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  String8 path;
  String8List entry_points;
  String8List cmd_line_string_list;
  String8List env_string_list;
  String8 stdout_path;
  String8 stderr_path;
  String8 stdin_path;
  CTRL_TrapList traps;
  CTRL_UserBreakpointList user_bps;
};

typedef struct CTRL_MsgNode CTRL_MsgNode;
struct CTRL_MsgNode
{
  CTRL_MsgNode *next;
  CTRL_Msg v;
};

typedef struct CTRL_MsgList CTRL_MsgList;
struct CTRL_MsgList
{
  CTRL_MsgNode *first;
  CTRL_MsgNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Event Types

typedef enum CTRL_EventKind
{
  CTRL_EventKind_Null,
  CTRL_EventKind_Error,
  
  //- rjf: starts/stops
  CTRL_EventKind_Started,
  CTRL_EventKind_Stopped,
  
  //- rjf: entity creation/deletion
  CTRL_EventKind_NewProc,
  CTRL_EventKind_NewThread,
  CTRL_EventKind_NewModule,
  CTRL_EventKind_EndProc,
  CTRL_EventKind_EndThread,
  CTRL_EventKind_EndModule,
  
  //- rjf: thread freeze state changes
  CTRL_EventKind_ThreadFrozen,
  CTRL_EventKind_ThreadThawed,
  
  //- rjf: debug info changes
  CTRL_EventKind_ModuleDebugInfoPathChange,
  
  //- rjf: debug strings / decorations / markup
  CTRL_EventKind_DebugString,
  CTRL_EventKind_ThreadName,
  CTRL_EventKind_ThreadColor,
  CTRL_EventKind_SetBreakpoint,
  CTRL_EventKind_UnsetBreakpoint,
  
  //- rjf: memory
  CTRL_EventKind_MemReserve,
  CTRL_EventKind_MemCommit,
  CTRL_EventKind_MemDecommit,
  CTRL_EventKind_MemRelease,
  
  CTRL_EventKind_COUNT
}
CTRL_EventKind;

typedef enum CTRL_EventCause
{
  CTRL_EventCause_Null,
  CTRL_EventCause_Error,
  CTRL_EventCause_Finished,
  CTRL_EventCause_EntryPoint,
  CTRL_EventCause_UserBreakpoint,
  CTRL_EventCause_InterruptedByTrap,
  CTRL_EventCause_InterruptedByException,
  CTRL_EventCause_InterruptedByHalt,
  CTRL_EventCause_COUNT
}
CTRL_EventCause;

typedef enum CTRL_ExceptionKind
{
  CTRL_ExceptionKind_Null,
  CTRL_ExceptionKind_MemoryRead,
  CTRL_ExceptionKind_MemoryWrite,
  CTRL_ExceptionKind_MemoryExecute,
  CTRL_ExceptionKind_CppThrow,
  CTRL_ExceptionKind_COUNT
}
CTRL_ExceptionKind;

typedef struct CTRL_Event CTRL_Event;
struct CTRL_Event
{
  CTRL_EventKind kind;
  CTRL_EventCause cause;
  CTRL_ExceptionKind exception_kind;
  CTRL_MsgID msg_id;
  CTRL_Handle entity;
  CTRL_Handle parent;
  Arch arch;
  U64 u64_code;
  U32 entity_id;
  Rng1U64 vaddr_rng;
  U64 rip_vaddr;
  U64 stack_base;
  U64 tls_root;
  U64 timestamp;
  U32 exception_code;
  U32 rgba;
  CTRL_UserBreakpointFlags bp_flags;
  String8 string;
};

typedef struct CTRL_EventNode CTRL_EventNode;
struct CTRL_EventNode
{
  CTRL_EventNode *next;
  CTRL_Event v;
};

typedef struct CTRL_EventList CTRL_EventList;
struct CTRL_EventList
{
  CTRL_EventNode *first;
  CTRL_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Process Memory Cache Types

typedef struct CTRL_ProcessMemoryRangeHashNode CTRL_ProcessMemoryRangeHashNode;
struct CTRL_ProcessMemoryRangeHashNode
{
  CTRL_ProcessMemoryRangeHashNode *next;
  
  // rjf: key
  Rng1U64 vaddr_range;
  B32 zero_terminated;
  HS_ID id;
  
  // rjf: staleness info
  U64 mem_gen;
  
  // rjf: metadata
  U64 working_count;
  U64 last_time_requested_us;
  U64 last_user_clock_idx_touched;
};

typedef struct CTRL_ProcessMemoryRangeHashSlot CTRL_ProcessMemoryRangeHashSlot;
struct CTRL_ProcessMemoryRangeHashSlot
{
  CTRL_ProcessMemoryRangeHashNode *first;
  CTRL_ProcessMemoryRangeHashNode *last;
};

typedef struct CTRL_ProcessMemoryCacheNode CTRL_ProcessMemoryCacheNode;
struct CTRL_ProcessMemoryCacheNode
{
  CTRL_ProcessMemoryCacheNode *next;
  CTRL_ProcessMemoryCacheNode *prev;
  Arena *arena;
  CTRL_Handle handle;
  HS_Root root;
  U64 range_hash_slots_count;
  CTRL_ProcessMemoryRangeHashSlot *range_hash_slots;
};

typedef struct CTRL_ProcessMemoryCacheSlot CTRL_ProcessMemoryCacheSlot;
struct CTRL_ProcessMemoryCacheSlot
{
  CTRL_ProcessMemoryCacheNode *first;
  CTRL_ProcessMemoryCacheNode *last;
};

typedef struct CTRL_ProcessMemoryCacheStripe CTRL_ProcessMemoryCacheStripe;
struct CTRL_ProcessMemoryCacheStripe
{
  OS_Handle rw_mutex;
  OS_Handle cv;
};

typedef struct CTRL_ProcessMemoryCache CTRL_ProcessMemoryCache;
struct CTRL_ProcessMemoryCache
{
  U64 slots_count;
  CTRL_ProcessMemoryCacheSlot *slots;
  U64 stripes_count;
  CTRL_ProcessMemoryCacheStripe *stripes;
};

typedef struct CTRL_ProcessMemorySlice CTRL_ProcessMemorySlice;
struct CTRL_ProcessMemorySlice
{
  String8 data;
  U64 *byte_bad_flags;
  U64 *byte_changed_flags;
  B32 stale;
  B32 any_byte_bad;
  B32 any_byte_changed;
};

////////////////////////////////
//~ rjf: Thread Register Cache Types

typedef struct CTRL_ThreadRegCacheNode CTRL_ThreadRegCacheNode;
struct CTRL_ThreadRegCacheNode
{
  CTRL_ThreadRegCacheNode *next;
  CTRL_ThreadRegCacheNode *prev;
  CTRL_Handle handle;
  U64 block_size;
  void *block;
  U64 reg_gen;
};

typedef struct CTRL_ThreadRegCacheSlot CTRL_ThreadRegCacheSlot;
struct CTRL_ThreadRegCacheSlot
{
  CTRL_ThreadRegCacheNode *first;
  CTRL_ThreadRegCacheNode *last;
};

typedef struct CTRL_ThreadRegCacheStripe CTRL_ThreadRegCacheStripe;
struct CTRL_ThreadRegCacheStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
};

typedef struct CTRL_ThreadRegCache CTRL_ThreadRegCache;
struct CTRL_ThreadRegCache
{
  U64 slots_count;
  CTRL_ThreadRegCacheSlot *slots;
  U64 stripes_count;
  CTRL_ThreadRegCacheStripe *stripes;
};

////////////////////////////////
//~ rjf: Call Stack Cache Types

typedef struct CTRL_CallStackCacheNode CTRL_CallStackCacheNode;
struct CTRL_CallStackCacheNode
{
  CTRL_CallStackCacheNode *next;
  CTRL_CallStackCacheNode *prev;
  
  // rjf: key
  CTRL_Handle thread;
  U64 reg_gen;
  U64 mem_gen;
  
  // rjf: counters
  U64 scope_touch_count;
  U64 working_count;
  
  // rjf: value
  Arena *arena;
  CTRL_CallStack call_stack;
};

typedef struct CTRL_CallStackCacheSlot CTRL_CallStackCacheSlot;
struct CTRL_CallStackCacheSlot
{
  CTRL_CallStackCacheNode *first;
  CTRL_CallStackCacheNode *last;
};

typedef struct CTRL_CallStackCacheStripe CTRL_CallStackCacheStripe;
struct CTRL_CallStackCacheStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

typedef struct CTRL_CallStackCache CTRL_CallStackCache;
struct CTRL_CallStackCache
{
  U64 slots_count;
  CTRL_CallStackCacheSlot *slots;
  U64 stripes_count;
  CTRL_CallStackCacheStripe *stripes;
};

////////////////////////////////
//~ rjf: Module Image Info Cache Types

typedef struct CTRL_ModuleImageInfoCacheNode CTRL_ModuleImageInfoCacheNode;
struct CTRL_ModuleImageInfoCacheNode
{
  CTRL_ModuleImageInfoCacheNode *next;
  CTRL_ModuleImageInfoCacheNode *prev;
  CTRL_Handle module;
  Arena *arena;
  PE_IntelPdata *pdatas;
  U64 pdatas_count;
  U64 entry_point_voff;
  Rng1U64 tls_vaddr_range;
  String8 initial_debug_info_path;
  Rng1U64 raddbg_section_voff_range;
  String8 raddbg_data;
};

typedef struct CTRL_ModuleImageInfoCacheSlot CTRL_ModuleImageInfoCacheSlot;
struct CTRL_ModuleImageInfoCacheSlot
{
  CTRL_ModuleImageInfoCacheNode *first;
  CTRL_ModuleImageInfoCacheNode *last;
};

typedef struct CTRL_ModuleImageInfoCacheStripe CTRL_ModuleImageInfoCacheStripe;
struct CTRL_ModuleImageInfoCacheStripe
{
  Arena *arena;
  OS_Handle rw_mutex;
};

typedef struct CTRL_ModuleImageInfoCache CTRL_ModuleImageInfoCache;
struct CTRL_ModuleImageInfoCache
{
  U64 slots_count;
  CTRL_ModuleImageInfoCacheSlot *slots;
  U64 stripes_count;
  CTRL_ModuleImageInfoCacheStripe *stripes;
};

////////////////////////////////
//~ rjf: Touched Debug Info Directory Cache

typedef struct CTRL_DbgDirNode CTRL_DbgDirNode;
struct CTRL_DbgDirNode
{
  CTRL_DbgDirNode *first;
  CTRL_DbgDirNode *last;
  CTRL_DbgDirNode *next;
  CTRL_DbgDirNode *prev;
  CTRL_DbgDirNode *parent;
  String8 name;
  U64 search_count;
  U64 child_count;
  U64 module_direct_count;
};

////////////////////////////////
//~ rjf: Control Thread Evaluation Scopes

typedef struct CTRL_EvalScope CTRL_EvalScope;
struct CTRL_EvalScope
{
  DI_Scope *di_scope;
  E_BaseCtx base_ctx;
  E_IRCtx ir_ctx;
  E_InterpretCtx interpret_ctx;
};

////////////////////////////////
//~ rjf: Control Cache Accessing Scopes

typedef struct CTRL_ScopeCallStackTouch CTRL_ScopeCallStackTouch;
struct CTRL_ScopeCallStackTouch
{
  CTRL_ScopeCallStackTouch *next;
  CTRL_CallStackCacheStripe *stripe;
  CTRL_CallStackCacheNode *node;
};

typedef struct CTRL_Scope CTRL_Scope;
struct CTRL_Scope
{
  CTRL_Scope *next;
  CTRL_ScopeCallStackTouch *first_call_stack_touch;
  CTRL_ScopeCallStackTouch *last_call_stack_touch;
};

typedef struct CTRL_TCTX CTRL_TCTX;
struct CTRL_TCTX
{
  Arena *arena;
  CTRL_Scope *free_scope;
  CTRL_ScopeCallStackTouch *free_call_stack_touch;
};

////////////////////////////////
//~ rjf: Wakeup Hook Function Types

#define CTRL_WAKEUP_FUNCTION_DEF(name) void name(void)
typedef CTRL_WAKEUP_FUNCTION_DEF(CTRL_WakeupFunctionType);

////////////////////////////////
//~ rjf: Main State Types

typedef struct CTRL_State CTRL_State;
struct CTRL_State
{
  Arena *arena;
  CTRL_WakeupFunctionType *wakeup_hook;
  
  // rjf: name -> register/alias hash tables for eval
  E_String2NumMap arch_string2reg_tables[Arch_COUNT];
  E_String2NumMap arch_string2alias_tables[Arch_COUNT];
  
  // rjf: caches
  CTRL_ProcessMemoryCache process_memory_cache;
  CTRL_ThreadRegCache thread_reg_cache;
  CTRL_CallStackCache call_stack_cache;
  CTRL_ModuleImageInfoCache module_image_info_cache;
  
  // rjf: user -> ctrl msg ring buffer
  U64 u2c_ring_size;
  U8 *u2c_ring_base;
  U64 u2c_ring_write_pos;
  U64 u2c_ring_read_pos;
  OS_Handle u2c_ring_mutex;
  OS_Handle u2c_ring_cv;
  
  // rjf: ctrl -> user event ring buffer
  U64 c2u_ring_size;
  U64 c2u_ring_max_string_size;
  U8 *c2u_ring_base;
  U64 c2u_ring_write_pos;
  U64 c2u_ring_read_pos;
  OS_Handle c2u_ring_mutex;
  OS_Handle c2u_ring_cv;
  
  // rjf: ctrl thread state
  String8 ctrl_thread_log_path;
  OS_Handle ctrl_thread;
  Log *ctrl_thread_log;
  OS_Handle ctrl_thread_entity_ctx_rw_mutex;
  CTRL_EntityCtxRWStore *ctrl_thread_entity_store;
  E_Cache *ctrl_thread_eval_cache;
  Arena *dmn_event_arena;
  DMN_EventNode *first_dmn_event_node;
  DMN_EventNode *last_dmn_event_node;
  DMN_EventNode *free_dmn_event_node;
  Arena *user_entry_point_arena;
  String8List user_entry_points;
  U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  U64 process_counter;
  Arena *dbg_dir_arena;
  CTRL_DbgDirNode *dbg_dir_root;
  
  // rjf: user -> memstream ring buffer
  U64 u2ms_ring_size;
  U8 *u2ms_ring_base;
  U64 u2ms_ring_write_pos;
  U64 u2ms_ring_read_pos;
  OS_Handle u2ms_ring_mutex;
  OS_Handle u2ms_ring_cv;
  
  // rjf: user -> call stack builder ring buffer
  U64 u2csb_ring_size;
  U8 *u2csb_ring_base;
  U64 u2csb_ring_write_pos;
  U64 u2csb_ring_read_pos;
  OS_Handle u2csb_ring_mutex;
  OS_Handle u2csb_ring_cv;
};

////////////////////////////////
//~ rjf: Globals

global CTRL_State *ctrl_state = 0;
read_only global CTRL_Entity ctrl_entity_nil =
{
  &ctrl_entity_nil,
  &ctrl_entity_nil,
  &ctrl_entity_nil,
  &ctrl_entity_nil,
  &ctrl_entity_nil,
};
thread_static CTRL_TCTX *ctrl_tctx = 0;
thread_static CTRL_EntityCtxLookupAccel *ctrl_entity_ctx_lookup_accel = 0;

////////////////////////////////
//~ rjf: Logging Markup

#define CTRL_CtrlThreadLogScope DeferLoop(log_scope_begin(), ctrl_thread__end_and_flush_info_log())

////////////////////////////////
//~ rjf: Basic Type Functions

internal U64 ctrl_hash_from_string(String8 string);
internal U64 ctrl_hash_from_handle(CTRL_Handle handle);
internal CTRL_EventCause ctrl_event_cause_from_dmn_event_kind(DMN_EventKind event_kind);
internal CTRL_ExceptionKind ctrl_exception_kind_from_dmn(DMN_ExceptionKind kind);
internal String8 ctrl_string_from_event_kind(CTRL_EventKind kind);
internal String8 ctrl_string_from_msg_kind(CTRL_MsgKind kind);
internal CTRL_EntityKind ctrl_entity_kind_from_string(String8 string);
internal DMN_TrapFlags ctrl_dmn_trap_flags_from_user_breakpoint_flags(CTRL_UserBreakpointFlags flags);
internal CTRL_UserBreakpointFlags ctrl_user_breakpoint_flags_from_dmn_trap_flags(DMN_TrapFlags flags);

////////////////////////////////
//~ rjf: Handle Type Functions

internal CTRL_Handle ctrl_handle_zero(void);
internal CTRL_Handle ctrl_handle_make(CTRL_MachineID machine_id, DMN_Handle dmn_handle);
internal B32 ctrl_handle_match(CTRL_Handle a, CTRL_Handle b);
internal void ctrl_handle_list_push(Arena *arena, CTRL_HandleList *list, CTRL_Handle *pair);
internal CTRL_HandleList ctrl_handle_list_copy(Arena *arena, CTRL_HandleList *src);
internal String8 ctrl_string_from_handle(Arena *arena, CTRL_Handle handle);
internal CTRL_Handle ctrl_handle_from_string(String8 string);

////////////////////////////////
//~ rjf: Trap Type Functions

internal void ctrl_trap_list_push(Arena *arena, CTRL_TrapList *list, CTRL_Trap *trap);
internal CTRL_TrapList ctrl_trap_list_copy(Arena *arena, CTRL_TrapList *src);

////////////////////////////////
//~ rjf: User Breakpoint Type Functions

internal void ctrl_user_breakpoint_list_push(Arena *arena, CTRL_UserBreakpointList *list, CTRL_UserBreakpoint *bp);
internal CTRL_UserBreakpointList ctrl_user_breakpoint_list_copy(Arena *arena, CTRL_UserBreakpointList *src);

////////////////////////////////
//~ rjf: Message Type Functions

//- rjf: deep copying
internal void ctrl_msg_deep_copy(Arena *arena, CTRL_Msg *dst, CTRL_Msg *src);

//- rjf: list building
internal CTRL_Msg *ctrl_msg_list_push(Arena *arena, CTRL_MsgList *list);
internal CTRL_MsgList ctrl_msg_list_deep_copy(Arena *arena, CTRL_MsgList *src);
internal void ctrl_msg_list_concat_in_place(CTRL_MsgList *dst, CTRL_MsgList *src);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_msg_list(Arena *arena, CTRL_MsgList *msgs);
internal CTRL_MsgList ctrl_msg_list_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Event Type Functions

//- rjf: list building
internal CTRL_Event *ctrl_event_list_push(Arena *arena, CTRL_EventList *list);
internal void ctrl_event_list_concat_in_place(CTRL_EventList *dst, CTRL_EventList *to_push);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_event(Arena *arena, CTRL_Event *event, U64 max);
internal CTRL_Event ctrl_event_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Entity Type Functions

//- rjf: entity list data structures
internal void ctrl_entity_list_push(Arena *arena, CTRL_EntityList *list, CTRL_Entity *entity);
internal CTRL_EntityList ctrl_entity_list_from_handle_list(Arena *arena, CTRL_EntityCtx *ctx, CTRL_HandleList *list);
#define ctrl_entity_list_first(list) ((list)->first ? (list)->first->v : &ctrl_entity_nil)

//- rjf: entity array data structure
internal CTRL_EntityArray ctrl_entity_array_from_list(Arena *arena, CTRL_EntityList *list);
#define ctrl_entity_array_first(array) ((array)->count != 0 ? (array)->v[0] : &ctrl_entity_nil)

//- rjf: entity context (entity group read-only) functions
internal CTRL_Entity *ctrl_entity_from_handle(CTRL_EntityCtx *ctx, CTRL_Handle handle);
internal CTRL_Entity *ctrl_entity_child_from_kind(CTRL_Entity *parent, CTRL_EntityKind kind);
internal CTRL_Entity *ctrl_entity_ancestor_from_kind(CTRL_Entity *entity, CTRL_EntityKind kind);
internal CTRL_Entity *ctrl_process_from_entity(CTRL_Entity *entity);
internal CTRL_Entity *ctrl_module_from_process_vaddr(CTRL_Entity *process, U64 vaddr);
internal DI_Key ctrl_dbgi_key_from_module(CTRL_Entity *module);
internal CTRL_Entity *ctrl_module_from_thread_candidates(CTRL_EntityCtx *ctx, CTRL_Entity *thread, CTRL_EntityList *candidates);
internal U64 ctrl_vaddr_from_voff(CTRL_Entity *module, U64 voff);
internal U64 ctrl_voff_from_vaddr(CTRL_Entity *module, U64 vaddr);
internal Rng1U64 ctrl_vaddr_range_from_voff_range(CTRL_Entity *module, Rng1U64 voff_range);
internal Rng1U64 ctrl_voff_range_from_vaddr_range(CTRL_Entity *module, Rng1U64 vaddr_range);
internal B32 ctrl_entity_tree_is_frozen(CTRL_Entity *root);

//- rjf: entity tree iteration
internal CTRL_EntityRec ctrl_entity_rec_depth_first(CTRL_Entity *entity, CTRL_Entity *subtree_root, U64 sib_off, U64 child_off);
#define ctrl_entity_rec_depth_first_pre(entity, subtree_root)  ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(CTRL_Entity, next), OffsetOf(CTRL_Entity, first))
#define ctrl_entity_rec_depth_first_post(entity, subtree_root) ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(CTRL_Entity, prev), OffsetOf(CTRL_Entity, last))

//- rjf: entity ctx r/w store state functions
internal CTRL_EntityCtxRWStore *ctrl_entity_ctx_rw_store_alloc(void);
internal void ctrl_entity_ctx_rw_store_release(CTRL_EntityCtxRWStore *store);

//- rjf: string allocation/deletion
internal U64 ctrl_name_bucket_num_from_string_size(U64 size);
internal String8 ctrl_entity_string_alloc(CTRL_EntityCtxRWStore *store, String8 string);
internal void ctrl_entity_string_release(CTRL_EntityCtxRWStore *store, String8 string);

//- rjf: entity construction/deletion
internal CTRL_Entity *ctrl_entity_alloc(CTRL_EntityCtxRWStore *store, CTRL_Entity *parent, CTRL_EntityKind kind, Arch arch, CTRL_Handle handle, U64 id);
internal void ctrl_entity_release(CTRL_EntityCtxRWStore *store, CTRL_Entity *entity);

//- rjf: entity equipment
internal void ctrl_entity_equip_string(CTRL_EntityCtxRWStore *store, CTRL_Entity *entity, String8 string);

//- rjf: accelerated entity context lookups
internal CTRL_EntityCtxLookupAccel *ctrl_thread_entity_ctx_lookup_accel(void);
internal CTRL_EntityArray ctrl_entity_array_from_kind(CTRL_EntityCtx *ctx, CTRL_EntityKind kind);
internal CTRL_EntityList ctrl_modules_from_dbgi_key(Arena *arena, CTRL_EntityCtx *ctx, DI_Key *dbgi_key);
internal CTRL_Entity *ctrl_thread_from_id(CTRL_EntityCtx *ctx, U64 id);

//- rjf: applying events to entity caches
internal void ctrl_entity_store_apply_events(CTRL_EntityCtxRWStore *store, CTRL_EventList *list);

////////////////////////////////
//~ rjf: Cache Accessing Scopes

internal CTRL_Scope *ctrl_scope_open(void);
internal void ctrl_scope_close(CTRL_Scope *scope);
internal void ctrl_scope_touch_call_stack_node__stripe_r_guarded(CTRL_Scope *scope, CTRL_CallStackCacheStripe *stripe, CTRL_CallStackCacheNode *node);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void ctrl_init(void);

////////////////////////////////
//~ rjf: Wakeup Callback Registration

internal void ctrl_set_wakeup_hook(CTRL_WakeupFunctionType *wakeup_hook);

////////////////////////////////
//~ rjf: Process Memory Functions

//- rjf: process memory cache key reading
internal HS_Key ctrl_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us, B32 *out_is_stale);

//- rjf: process memory cache interaction
#if 0 // TODO(rjf): @hs
internal U128 ctrl_calc_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated);
internal U128 ctrl_stored_hash_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated, B32 *out_is_stale, U64 endt_us);
#endif

//- rjf: bundled key/stream helper
#if 0 // TODO(rjf): @hs
internal U128 ctrl_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated);
#endif

//- rjf: process memory cache reading helpers
internal CTRL_ProcessMemorySlice ctrl_process_memory_slice_from_vaddr_range(Arena *arena, CTRL_Handle process, Rng1U64 range, U64 endt_us);
internal B32 ctrl_process_memory_read(CTRL_Handle process, Rng1U64 range, B32 *is_stale_out, void *out, U64 endt_us);
#define ctrl_process_memory_read_struct(process, vaddr, is_stale_out, ptr, endt_us) ctrl_process_memory_read((process), r1u64((vaddr), (vaddr)+(sizeof(*(ptr)))), (is_stale_out), (ptr), (endt_us))

//- rjf: process memory writing
internal B32 ctrl_process_write(CTRL_Handle process, Rng1U64 range, void *src);

////////////////////////////////
//~ rjf: Thread Register Functions

//- rjf: thread register cache reading
internal void *ctrl_reg_block_from_thread(Arena *arena, CTRL_EntityCtx *ctx, CTRL_Handle handle);
internal U64 ctrl_tls_root_vaddr_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle);
internal U64 ctrl_rip_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle);
internal U64 ctrl_rsp_from_thread(CTRL_EntityCtx *ctx, CTRL_Handle handle);

//- rjf: thread register writing
internal B32 ctrl_thread_write_reg_block(CTRL_Handle thread, void *block);

////////////////////////////////
//~ rjf: Module Image Info Functions

//- rjf: cache lookups
internal PE_IntelPdata *ctrl_intel_pdata_from_module_voff(Arena *arena, CTRL_Handle module_handle, U64 voff);
internal U64 ctrl_entry_point_voff_from_module(CTRL_Handle module_handle);
internal Rng1U64 ctrl_tls_vaddr_range_from_module(CTRL_Handle module_handle);
internal String8 ctrl_initial_debug_info_path_from_module(Arena *arena, CTRL_Handle module_handle);
internal String8 ctrl_raddbg_data_from_module(Arena *arena, CTRL_Handle module_handle);

////////////////////////////////
//~ rjf: Unwinding Functions

//- rjf: unwind deep copier
internal CTRL_Unwind ctrl_unwind_deep_copy(Arena *arena, Arch arch, CTRL_Unwind *src);

//- rjf: [x64]
internal REGS_Reg64 *ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 gpr_reg);
internal CTRL_UnwindStepResult ctrl_unwind_step__pe_x64(CTRL_Handle process_handle, CTRL_Handle module_handle, U64 module_base_vaddr, REGS_RegBlockX64 *regs, U64 endt_us);

//- rjf: abstracted unwind step
internal CTRL_UnwindStepResult ctrl_unwind_step(CTRL_Handle process, CTRL_Handle module, U64 module_base_vaddr, Arch arch, void *reg_block, U64 endt_us);

//- rjf: abstracted full unwind
internal CTRL_Unwind ctrl_unwind_from_thread(Arena *arena, CTRL_EntityCtx *ctx, CTRL_Handle thread, U64 endt_us);

////////////////////////////////
//~ rjf: Call Stack Building Functions

internal CTRL_CallStack ctrl_call_stack_from_unwind(Arena *arena, CTRL_Entity *process, CTRL_Unwind *base_unwind);
internal CTRL_CallStackFrame *ctrl_call_stack_frame_from_unwind_and_inline_depth(CTRL_CallStack *call_stack, U64 unwind_count, U64 inline_depth);

////////////////////////////////
//~ rjf: Call Stack Cache Functions

internal CTRL_CallStack ctrl_call_stack_from_thread(CTRL_Scope *scope, CTRL_EntityCtx *entity_ctx, CTRL_Entity *thread, B32 high_priority, U64 endt_us);

////////////////////////////////
//~ rjf: Halting All Attached Processes

internal void ctrl_halt(void);

////////////////////////////////
//~ rjf: Shared Accessor Functions

//- rjf: generation counters
internal U64 ctrl_run_gen(void);
internal U64 ctrl_mem_gen(void);
internal U64 ctrl_reg_gen(void);

//- rjf: name -> register/alias hash tables, for eval
internal E_String2NumMap *ctrl_string2reg_from_arch(Arch arch);
internal E_String2NumMap *ctrl_string2alias_from_arch(Arch arch);

////////////////////////////////
//~ rjf: Control-Thread Functions

//- rjf: user -> control thread communication
internal B32 ctrl_u2c_push_msgs(CTRL_MsgList *msgs, U64 endt_us);
internal CTRL_MsgList ctrl_u2c_pop_msgs(Arena *arena);

//- rjf: control -> user thread communication
internal void ctrl_c2u_push_events(CTRL_EventList *events);
internal CTRL_EventList ctrl_c2u_pop_events(Arena *arena);

//- rjf: entry point
internal void ctrl_thread__entry_point(void *p);

//- rjf: breakpoint resolution
internal void ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, CTRL_EvalScope *eval_scope, CTRL_Handle process, CTRL_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, CTRL_EvalScope *eval_scope, CTRL_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_thread__append_program_defined_bp_traps(Arena *arena, CTRL_Entity *bp, DMN_TrapChunkList *traps_out);

//- rjf: module lifetime open/close work
internal void ctrl_thread__module_open(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range, String8 path);
internal void ctrl_thread__module_close(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range);

//- rjf: attached process running/event gathering
internal DMN_Event *ctrl_thread__next_dmn_event(Arena *arena, DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg, DMN_RunCtrls *run_ctrls, CTRL_Spoof *spoof);

//- rjf: eval helpers
internal B32 ctrl_eval_space_read(void *u, E_Space space, void *out, Rng1U64 vaddr_range);

//- rjf: control thread eval scopes
internal CTRL_EvalScope *ctrl_thread__eval_scope_begin(Arena *arena, CTRL_Entity *thread);
internal void ctrl_thread__eval_scope_end(CTRL_EvalScope *scope);

//- rjf: log flusher
internal void ctrl_thread__flush_info_log(String8 string);
internal void ctrl_thread__end_and_flush_info_log(void);

//- rjf: msg kind implementations
internal void ctrl_thread__launch(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__attach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__kill(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__kill_all(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__detach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__run(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__single_step(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);

////////////////////////////////
//~ rjf: Asynchronous Memory Streaming Functions

//- rjf: user -> memory stream communication
internal B32 ctrl_u2ms_enqueue_req(HS_Key key, CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us);
internal void ctrl_u2ms_dequeue_req(HS_Key *out_key, CTRL_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated);

//- rjf: entry point
ASYNC_WORK_DEF(ctrl_mem_stream_work);

////////////////////////////////
//~ rjf: Asynchronous Call Stack Building Functions

//- rjf: user -> memory stream communication
internal B32 ctrl_u2csb_enqueue_req(CTRL_Handle thread, U64 endt_us);
internal void ctrl_u2csb_dequeue_req(CTRL_Handle *out_thread);

//- rjf: entry point
ASYNC_WORK_DEF(ctrl_call_stack_build_work);

#endif // CTRL_CORE_H
