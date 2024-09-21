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
//~ rjf: Meta Evaluation Types

//- rjf: meta evaluation callstack
typedef struct CTRL_MetaEvalFrame CTRL_MetaEvalFrame;
struct CTRL_MetaEvalFrame
{
  U64 vaddr;
};
struct_members(CTRL_MetaEvalFrame)
{
  member_lit_comp(CTRL_MetaEvalFrame, type(U64), vaddr),
};
struct_type(CTRL_MetaEvalFrame);
typedef struct CTRL_MetaEvalFrameArray CTRL_MetaEvalFrameArray;
struct CTRL_MetaEvalFrameArray
{
  U64 count;
  CTRL_MetaEvalFrame *v;
};
ptr_type(CTRL_MetaEvalFrameArray__v_ptr_type, type(CTRL_MetaEvalFrame), .count_delimiter_name = str8_lit_comp("count"));
struct_members(CTRL_MetaEvalFrameArray)
{
  member_lit_comp(CTRL_MetaEvalFrameArray, type(U64), count),
  {str8_lit_comp("v"), &CTRL_MetaEvalFrameArray__v_ptr_type, OffsetOf(CTRL_MetaEvalFrameArray, v)},
};
struct_type(CTRL_MetaEvalFrameArray);

//- rjf: meta evaluation instance
typedef struct CTRL_MetaEval CTRL_MetaEval;
struct CTRL_MetaEval
{
#define CTRL_MetaEval_MemberXList \
X(B32, enabled)\
X(B32, frozen)\
X(U64, hit_count)\
X(U64, id)\
X(Rng1U64, vaddr_range)\
X(U32, color)\
X(String8, label)\
X(String8, exe)\
X(String8, dbg)\
X(String8, args)\
X(String8, working_directory)\
X(String8, entry_point)\
X(String8, location)\
X(String8, condition)\
X(CTRL_MetaEvalFrameArray, callstack)
#define X(T, name) T name;
  CTRL_MetaEval_MemberXList
#undef X
};
struct_members(CTRL_MetaEval)
{
#define X(T, name) member_lit_comp(CTRL_MetaEval, type(T), name),
  CTRL_MetaEval_MemberXList
#undef X
};
struct_type(CTRL_MetaEval);

//- rjf: meta evaluation array
typedef struct CTRL_MetaEvalArray CTRL_MetaEvalArray;
struct CTRL_MetaEvalArray
{
  CTRL_MetaEval *v;
  U64 count;
};
ptr_type(CTRL_MetaEvalArray__v_ptr_type, type(CTRL_MetaEval), .count_delimiter_name = str8_lit_comp("count"));
struct_members(CTRL_MetaEvalArray)
{
  {str8_lit_comp("v"), &CTRL_MetaEvalArray__v_ptr_type, OffsetOf(CTRL_MetaEvalArray, v)},
  member_lit_comp(CTRL_MetaEvalArray, type(U64), count),
};
struct_type(CTRL_MetaEvalArray);

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
  U32 rgba;
  CTRL_Handle handle;
  U64 id;
  Rng1U64 vaddr_range;
  U64 stack_base;
  U64 timestamp;
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

typedef struct CTRL_EntityStore CTRL_EntityStore;
struct CTRL_EntityStore
{
  Arena *arena;
  CTRL_Entity *root;
  CTRL_Entity *free;
  CTRL_EntityHashSlot *hash_slots;
  CTRL_EntityHashNode *hash_node_free;
  U64 hash_slots_count;
  CTRL_EntityStringChunkNode *free_string_chunks[8];
  U64 entity_kind_counts[CTRL_EntityKind_COUNT];
  Arena *entity_kind_lists_arenas[CTRL_EntityKind_COUNT];
  U64 entity_kind_lists_gens[CTRL_EntityKind_COUNT];
  U64 entity_kind_alloc_gens[CTRL_EntityKind_COUNT];
  CTRL_EntityList entity_kind_lists[CTRL_EntityKind_COUNT];
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
//~ rjf: User Breakpoint Types

typedef enum CTRL_UserBreakpointKind
{
  CTRL_UserBreakpointKind_Null,
  CTRL_UserBreakpointKind_FileNameAndLineColNumber,
  CTRL_UserBreakpointKind_SymbolNameAndOffset,
  CTRL_UserBreakpointKind_VirtualAddress,
  CTRL_UserBreakpointKind_COUNT
}
CTRL_UserBreakpointKind;

typedef struct CTRL_UserBreakpoint CTRL_UserBreakpoint;
struct CTRL_UserBreakpoint
{
  CTRL_UserBreakpointKind kind;
  String8 string;
  TxtPt pt;
  U64 u64;
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
  U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  String8 path;
  String8List entry_points;
  String8List cmd_line_string_list;
  String8List env_string_list;
  CTRL_TrapList traps;
  CTRL_UserBreakpointList user_bps;
  CTRL_MetaEvalArray meta_evals;
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
  
  //- rjf: debug strings / decorations
  CTRL_EventKind_DebugString,
  CTRL_EventKind_ThreadName,
  CTRL_EventKind_ThreadColor,
  
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
  Rng1U64 vaddr_range;
  B32 zero_terminated;
  Rng1U64 vaddr_range_clamped;
  U128 hash;
  U64 mem_gen;
  U64 last_time_requested_us;
  B32 is_taken;
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
  CTRL_EntityStore *ctrl_thread_entity_store;
  Arena *dmn_event_arena;
  DMN_EventNode *first_dmn_event_node;
  DMN_EventNode *last_dmn_event_node;
  DMN_EventNode *free_dmn_event_node;
  Arena *user_entry_point_arena;
  String8List user_entry_points;
  Arena *user_meta_eval_arena;
  CTRL_MetaEvalArray *user_meta_evals;
  U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  U64 process_counter;
  
  // rjf: user -> memstream ring buffer
  U64 u2ms_ring_size;
  U8 *u2ms_ring_base;
  U64 u2ms_ring_write_pos;
  U64 u2ms_ring_read_pos;
  OS_Handle u2ms_ring_mutex;
  OS_Handle u2ms_ring_cv;
  
  // rjf: memory stream threads
  U64 ms_thread_count;
  OS_Handle *ms_threads;
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

////////////////////////////////
//~ rjf: Logging Markup

#define CTRL_CtrlThreadLogScope DeferLoop(log_scope_begin(), ctrl_thread__end_and_flush_info_log())

////////////////////////////////
//~ rjf: Basic Type Functions

internal U64 ctrl_hash_from_string(String8 string);
internal U64 ctrl_hash_from_handle(CTRL_Handle handle);
internal CTRL_EventCause ctrl_event_cause_from_dmn_event_kind(DMN_EventKind event_kind);
internal String8 ctrl_string_from_event_kind(CTRL_EventKind kind);
internal String8 ctrl_string_from_msg_kind(CTRL_MsgKind kind);

////////////////////////////////
//~ rjf: Handle Type Functions

internal CTRL_Handle ctrl_handle_zero(void);
internal CTRL_Handle ctrl_handle_make(CTRL_MachineID machine_id, DMN_Handle dmn_handle);
internal B32 ctrl_handle_match(CTRL_Handle a, CTRL_Handle b);
internal void ctrl_handle_list_push(Arena *arena, CTRL_HandleList *list, CTRL_Handle *pair);
internal CTRL_HandleList ctrl_handle_list_copy(Arena *arena, CTRL_HandleList *src);

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
internal CTRL_EntityList ctrl_entity_list_from_handle_list(Arena *arena, CTRL_EntityStore *store, CTRL_HandleList *list);
#define ctrl_entity_list_first(list) ((list)->first ? (list)->first->v : &ctrl_entity_nil)

//- rjf: cache creation/destruction
internal CTRL_EntityStore *ctrl_entity_store_alloc(void);
internal void ctrl_entity_store_release(CTRL_EntityStore *store);

//- rjf: string allocation/deletion
internal U64 ctrl_name_bucket_idx_from_string_size(U64 size);
internal String8 ctrl_entity_string_alloc(CTRL_EntityStore *store, String8 string);
internal void ctrl_entity_string_release(CTRL_EntityStore *store, String8 string);

//- rjf: entity construction/deletion
internal CTRL_Entity *ctrl_entity_alloc(CTRL_EntityStore *store, CTRL_Entity *parent, CTRL_EntityKind kind, Arch arch, CTRL_Handle handle, U64 id);
internal void ctrl_entity_release(CTRL_EntityStore *store, CTRL_Entity *entity);

//- rjf: entity equipment
internal void ctrl_entity_equip_string(CTRL_EntityStore *store, CTRL_Entity *entity, String8 string);

//- rjf: entity store lookups
internal CTRL_Entity *ctrl_entity_from_handle(CTRL_EntityStore *store, CTRL_Handle handle);
internal CTRL_Entity *ctrl_entity_child_from_kind(CTRL_Entity *parent, CTRL_EntityKind kind);
internal CTRL_Entity *ctrl_entity_ancestor_from_kind(CTRL_Entity *entity, CTRL_EntityKind kind);
internal CTRL_Entity *ctrl_module_from_process_vaddr(CTRL_Entity *process, U64 vaddr);
internal DI_Key ctrl_dbgi_key_from_module(CTRL_Entity *module);
internal CTRL_EntityList ctrl_modules_from_dbgi_key(Arena *arena, CTRL_EntityStore *store, DI_Key *dbgi_key);
internal CTRL_Entity *ctrl_module_from_thread_candidates(CTRL_EntityStore *store, CTRL_Entity *thread, CTRL_EntityList *candidates);
internal CTRL_EntityList ctrl_entity_list_from_kind(CTRL_EntityStore *store, CTRL_EntityKind kind);
internal U64 ctrl_vaddr_from_voff(CTRL_Entity *module, U64 voff);
internal U64 ctrl_voff_from_vaddr(CTRL_Entity *module, U64 vaddr);
internal Rng1U64 ctrl_vaddr_range_from_voff_range(CTRL_Entity *module, Rng1U64 voff_range);
internal Rng1U64 ctrl_voff_range_from_vaddr_range(CTRL_Entity *module, Rng1U64 vaddr_range);
internal B32 ctrl_entity_tree_is_frozen(CTRL_Entity *root);

//- rjf: entity tree iteration
internal CTRL_EntityRec ctrl_entity_rec_depth_first(CTRL_Entity *entity, CTRL_Entity *subtree_root, U64 sib_off, U64 child_off);
#define ctrl_entity_rec_depth_first_pre(entity, subtree_root)  ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(CTRL_Entity, next), OffsetOf(CTRL_Entity, first))
#define ctrl_entity_rec_depth_first_post(entity, subtree_root) ctrl_entity_rec_depth_first((entity), (subtree_root), OffsetOf(CTRL_Entity, prev), OffsetOf(CTRL_Entity, last))

//- rjf: applying events to entity caches
internal void ctrl_entity_store_apply_events(CTRL_EntityStore *store, CTRL_EventList *list);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void ctrl_init(void);

////////////////////////////////
//~ rjf: Wakeup Callback Registration

internal void ctrl_set_wakeup_hook(CTRL_WakeupFunctionType *wakeup_hook);

////////////////////////////////
//~ rjf: Process Memory Functions

//- rjf: process memory cache interaction
internal U128 ctrl_calc_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated);
internal U128 ctrl_stored_hash_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated, B32 *out_is_stale, U64 endt_us);

//- rjf: bundled key/stream helper
internal U128 ctrl_hash_store_key_from_process_vaddr_range(CTRL_Handle process, Rng1U64 range, B32 zero_terminated);

//- rjf: process memory cache reading helpers
internal CTRL_ProcessMemorySlice ctrl_query_cached_data_from_process_vaddr_range(Arena *arena, CTRL_Handle process, Rng1U64 range, U64 endt_us);
internal CTRL_ProcessMemorySlice ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(Arena *arena, CTRL_Handle process, U64 vaddr, U64 limit, U64 element_size, U64 endt_us);
internal B32 ctrl_read_cached_process_memory(CTRL_Handle process, Rng1U64 range, B32 *is_stale_out, void *out, U64 endt_us);
#define ctrl_read_cached_process_memory_struct(process, vaddr, is_stale_out, ptr, endt_us) ctrl_read_cached_process_memory((process), r1u64((vaddr), (vaddr)+(sizeof(*(ptr)))), (is_stale_out), (ptr), (endt_us))

//- rjf: process memory writing
internal B32 ctrl_process_write(CTRL_Handle process, Rng1U64 range, void *src);

////////////////////////////////
//~ rjf: Thread Register Functions

//- rjf: thread register cache reading
internal void *ctrl_query_cached_reg_block_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_Handle handle);
internal U64 ctrl_query_cached_tls_root_vaddr_from_thread(CTRL_EntityStore *store, CTRL_Handle handle);
internal U64 ctrl_query_cached_rip_from_thread(CTRL_EntityStore *store, CTRL_Handle handle);
internal U64 ctrl_query_cached_rsp_from_thread(CTRL_EntityStore *store, CTRL_Handle handle);

//- rjf: thread register writing
internal B32 ctrl_thread_write_reg_block(CTRL_Handle thread, void *block);

////////////////////////////////
//~ rjf: Module Image Info Functions

//- rjf: cache lookups
internal PE_IntelPdata *ctrl_intel_pdata_from_module_voff(Arena *arena, CTRL_Handle module_handle, U64 voff);
internal U64 ctrl_entry_point_voff_from_module(CTRL_Handle module_handle);
internal Rng1U64 ctrl_tls_vaddr_range_from_module(CTRL_Handle module_handle);
internal String8 ctrl_initial_debug_info_path_from_module(Arena *arena, CTRL_Handle module_handle);

////////////////////////////////
//~ rjf: Unwinding Functions

//- rjf: unwind deep copier
internal CTRL_Unwind ctrl_unwind_deep_copy(Arena *arena, Arch arch, CTRL_Unwind *src);

//- rjf: [x64]
internal REGS_Reg64 *ctrl_unwind_reg_from_pe_gpr_reg__pe_x64(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 gpr_reg);
internal CTRL_UnwindStepResult ctrl_unwind_step__pe_x64(CTRL_EntityStore *store, CTRL_Handle process_handle, CTRL_Handle module_handle, REGS_RegBlockX64 *regs, U64 endt_us);

//- rjf: abstracted unwind step
internal CTRL_UnwindStepResult ctrl_unwind_step(CTRL_EntityStore *store, CTRL_Handle process, CTRL_Handle module, Arch arch, void *reg_block, U64 endt_us);

//- rjf: abstracted full unwind
internal CTRL_Unwind ctrl_unwind_from_thread(Arena *arena, CTRL_EntityStore *store, CTRL_Handle thread, U64 endt_us);

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
internal void ctrl_thread__append_resolved_module_user_bp_traps(Arena *arena, CTRL_Handle process, CTRL_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_thread__append_resolved_process_user_bp_traps(Arena *arena, CTRL_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);

//- rjf: module lifetime open/close work
internal void ctrl_thread__module_open(CTRL_Handle process, CTRL_Handle module, Rng1U64 vaddr_range, String8 path);
internal void ctrl_thread__module_close(CTRL_Handle module);

//- rjf: attached process running/event gathering
internal DMN_Event *ctrl_thread__next_dmn_event(Arena *arena, DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg, DMN_RunCtrls *run_ctrls, CTRL_Spoof *spoof);

//- rjf: eval helpers
internal B32 ctrl_eval_space_read(void *u, E_Space space, void *out, Rng1U64 vaddr_range);

//- rjf: log flusher
internal void ctrl_thread__flush_info_log(String8 string);
internal void ctrl_thread__end_and_flush_info_log(void);

//- rjf: msg kind implementations
internal void ctrl_thread__launch(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__attach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__kill(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__detach(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__run(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);
internal void ctrl_thread__single_step(DMN_CtrlCtx *ctrl_ctx, CTRL_Msg *msg);

////////////////////////////////
//~ rjf: Memory-Stream Thread Functions

//- rjf: user -> memory stream communication
internal B32 ctrl_u2ms_enqueue_req(CTRL_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us);
internal void ctrl_u2ms_dequeue_req(CTRL_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated);

//- rjf: entry point
internal void ctrl_mem_stream_thread__entry_point(void *p);

#endif // CTRL_CORE_H
