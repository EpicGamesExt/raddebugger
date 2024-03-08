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
//~ rjf: Machine/Handle Pair Types

typedef struct CTRL_MachineIDHandlePair CTRL_MachineIDHandlePair;
struct CTRL_MachineIDHandlePair
{
  CTRL_MachineID machine_id;
  DMN_Handle handle;
};

typedef struct CTRL_MachineIDHandlePairNode CTRL_MachineIDHandlePairNode;
struct CTRL_MachineIDHandlePairNode
{
  CTRL_MachineIDHandlePairNode *next;
  CTRL_MachineIDHandlePair v;
};

typedef struct CTRL_MachineIDHandlePairList CTRL_MachineIDHandlePairList;
struct CTRL_MachineIDHandlePairList
{
  CTRL_MachineIDHandlePairNode *first;
  CTRL_MachineIDHandlePairNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Entity Types

typedef enum CTRL_EntityKind
{
  CTRL_EntityKind_Null,
  CTRL_EntityKind_Root,
  CTRL_EntityKind_Machine,
  CTRL_EntityKind_Process,
  CTRL_EntityKind_Thread,
  CTRL_EntityKind_Module,
  CTRL_EntityKind_COUNT
}
CTRL_EntityKind;

typedef struct CTRL_Entity CTRL_Entity;
struct CTRL_Entity
{
  CTRL_Entity *first;
  CTRL_Entity *last;
  CTRL_Entity *next;
  CTRL_Entity *prev;
  CTRL_Entity *parent;
  CTRL_EntityKind kind;
  Architecture arch;
  CTRL_MachineID machine_id;
  DMN_Handle handle;
  Rng1U64 vaddr_range;
  String8 name;
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

////////////////////////////////
//~ rjf: Unwind Types

typedef struct CTRL_UnwindFrame CTRL_UnwindFrame;
struct CTRL_UnwindFrame
{
  CTRL_UnwindFrame *next;
  CTRL_UnwindFrame *prev;
  U64 rip;
  void *regs;
};

typedef struct CTRL_Unwind CTRL_Unwind;
struct CTRL_Unwind
{
  CTRL_UnwindFrame *first;
  CTRL_UnwindFrame *last;
  U64 count;
  B32 error;
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
//~ rjf: Generated Code

#include "generated/ctrl.meta.h"

////////////////////////////////
//~ rjf: Message Types

typedef enum CTRL_MsgKind
{
  CTRL_MsgKind_Null,
  CTRL_MsgKind_LaunchAndHandshake,
  CTRL_MsgKind_LaunchAndInit,
  CTRL_MsgKind_Attach,
  CTRL_MsgKind_Kill,
  CTRL_MsgKind_Detach,
  CTRL_MsgKind_Run,
  CTRL_MsgKind_SingleStep,
  CTRL_MsgKind_SetUserEntryPoints,
  CTRL_MsgKind_COUNT,
}
CTRL_MsgKind;

typedef struct CTRL_Msg CTRL_Msg;
struct CTRL_Msg
{
  CTRL_MsgKind kind;
  CTRL_MsgID msg_id;
  CTRL_MachineID machine_id;
  DMN_Handle entity;
  DMN_Handle parent;
  U32 entity_id;
  U32 exit_code;
  B32 env_inherit;
  U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  String8 path;
  String8List strings;
  String8List cmd_line_string_list;
  String8List env_string_list;
  CTRL_TrapList traps;
  CTRL_UserBreakpointList user_bps;
  CTRL_MachineIDHandlePairList freeze_state_threads; // NOTE(rjf): can be frozen or unfrozen, depending on `freeze_state_is_frozen`
  B32 freeze_state_is_frozen;
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
  
  //- rjf: debug strings
  CTRL_EventKind_DebugString,
  CTRL_EventKind_ThreadName,
  
  //- rjf: memory
  CTRL_EventKind_MemReserve,
  CTRL_EventKind_MemCommit,
  CTRL_EventKind_MemDecommit,
  CTRL_EventKind_MemRelease,
  
  //- rjf: ctrl requests
  CTRL_EventKind_LaunchAndHandshakeDone,
  CTRL_EventKind_LaunchAndInitDone,
  CTRL_EventKind_AttachDone,
  CTRL_EventKind_KillDone,
  CTRL_EventKind_DetachDone,
  
  CTRL_EventKind_COUNT
}
CTRL_EventKind;

typedef enum CTRL_EventCause
{
  CTRL_EventCause_Null,
  CTRL_EventCause_Error,
  CTRL_EventCause_Finished,
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
  CTRL_MachineID machine_id;
  DMN_Handle entity;
  DMN_Handle parent;
  Architecture arch;
  U64 u64_code;
  U32 entity_id;
  Rng1U64 vaddr_rng;
  U64 rip_vaddr;
  U64 stack_base;
  U64 tls_root;
  U32 exception_code;
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
  U64 memgen_idx;
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
  CTRL_MachineID machine_id;
  DMN_Handle process;
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
};

////////////////////////////////
//~ rjf: Thread Register Cache Types

typedef struct CTRL_ThreadRegCacheNode CTRL_ThreadRegCacheNode;
struct CTRL_ThreadRegCacheNode
{
  CTRL_ThreadRegCacheNode *next;
  CTRL_ThreadRegCacheNode *prev;
  CTRL_MachineID machine_id;
  DMN_Handle thread;
  U128 regs_hash;
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
  U64 run_idx;
  U64 memgen_idx;
  U64 reggen_idx;
  
  // rjf: name -> register/alias hash tables for eval
  EVAL_String2NumMap arch_string2reg_tables[Architecture_COUNT];
  EVAL_String2NumMap arch_string2alias_tables[Architecture_COUNT];
  
  // rjf: caches
  CTRL_ProcessMemoryCache process_memory_cache;
  CTRL_ThreadRegCache thread_reg_cache;
  
  // rjf: user -> ctrl msg ring buffer
  U64 u2c_ring_size;
  U8 *u2c_ring_base;
  U64 u2c_ring_write_pos;
  U64 u2c_ring_read_pos;
  OS_Handle u2c_ring_mutex;
  OS_Handle u2c_ring_cv;
  
  // rjf: ctrl -> user event ring buffer
  U64 c2u_ring_size;
  U8 *c2u_ring_base;
  U64 c2u_ring_write_pos;
  U64 c2u_ring_read_pos;
  OS_Handle c2u_ring_mutex;
  OS_Handle c2u_ring_cv;
  
  // rjf: ctrl thread state
  OS_Handle ctrl_thread;
  Arena *ctrl_entity_arena;
  CTRL_Entity *ctrl_entity_root;
  CTRL_Entity *ctrl_entity_free;
  CTRL_EntityHashSlot *ctrl_entity_hash_slots;
  CTRL_EntityHashNode *ctrl_entity_hash_node_free;
  U64 ctrl_entity_hash_slots_count;
  Arena *dmn_event_arena;
  DMN_EventNode *first_dmn_event_node;
  DMN_EventNode *last_dmn_event_node;
  DMN_EventNode *free_dmn_event_node;
  Arena *user_entry_point_arena;
  String8List user_entry_points;
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
//~ rjf: Basic Type Functions

internal U64 ctrl_hash_from_string(String8 string);
internal U64 ctrl_hash_from_machine_id_handle(CTRL_MachineID machine_id, DMN_Handle handle);
internal CTRL_EventCause ctrl_event_cause_from_dmn_event_kind(DMN_EventKind event_kind);
internal B32 ctrl_handle_match(DMN_Handle a, DMN_Handle b);

////////////////////////////////
//~ rjf: Machine/Handle Pair Type Functions

internal void ctrl_machine_id_handle_pair_list_push(Arena *arena, CTRL_MachineIDHandlePairList *list, CTRL_MachineIDHandlePair *pair);
internal CTRL_MachineIDHandlePairList ctrl_machine_id_handle_pair_list_copy(Arena *arena, CTRL_MachineIDHandlePairList *src);

////////////////////////////////
//~ rjf: Trap Type Functions

internal void ctrl_trap_list_push(Arena *arena, CTRL_TrapList *list, CTRL_Trap *trap);
internal CTRL_TrapList ctrl_trap_list_copy(Arena *arena, CTRL_TrapList *src);

////////////////////////////////
//~ rjf: User Breakpoint Type Functions

internal void ctrl_user_breakpoint_list_push(Arena *arena, CTRL_UserBreakpointList *list, CTRL_UserBreakpoint *bp);
internal CTRL_UserBreakpointList ctrl_user_breakpoint_list_copy(Arena *arena, CTRL_UserBreakpointList *src);
internal void ctrl_append_resolved_module_user_bp_traps(Arena *arena, DMN_Handle process, DMN_Handle module, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);
internal void ctrl_append_resolved_process_user_bp_traps(Arena *arena, DMN_Handle process, CTRL_UserBreakpointList *user_bps, DMN_TrapChunkList *traps_out);

////////////////////////////////
//~ rjf: Message Type Functions

//- rjf: deep copying
internal void ctrl_msg_deep_copy(Arena *arena, CTRL_Msg *dst, CTRL_Msg *src);

//- rjf: list building
internal CTRL_Msg *ctrl_msg_list_push(Arena *arena, CTRL_MsgList *list);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_msg_list(Arena *arena, CTRL_MsgList *msgs);
internal CTRL_MsgList ctrl_msg_list_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Event Type Functions

//- rjf: list building
internal CTRL_Event *ctrl_event_list_push(Arena *arena, CTRL_EventList *list);
internal void ctrl_event_list_concat_in_place(CTRL_EventList *dst, CTRL_EventList *to_push);

//- rjf: serialization
internal String8 ctrl_serialized_string_from_event(Arena *arena, CTRL_Event *event);
internal CTRL_Event ctrl_event_from_serialized_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void ctrl_init(void);

////////////////////////////////
//~ rjf: Wakeup Callback Registration

internal void ctrl_set_wakeup_hook(CTRL_WakeupFunctionType *wakeup_hook);

////////////////////////////////
//~ rjf: Process Memory Functions

//- rjf: process memory cache interaction
internal U128 ctrl_hash_store_key_from_process_vaddr_range(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, B32 zero_terminated);
internal U128 ctrl_stored_hash_from_process_vaddr_range(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, B32 zero_terminated, U64 endt_us);

//- rjf: process memory cache reading helpers
internal CTRL_ProcessMemorySlice ctrl_query_cached_data_from_process_vaddr_range(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, U64 endt_us);
internal CTRL_ProcessMemorySlice ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(Arena *arena, CTRL_MachineID machine_id, DMN_Handle process, U64 vaddr, U64 limit, U64 endt_us);

//- rjf: process memory writing
internal B32 ctrl_process_write(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 range, void *src);

////////////////////////////////
//~ rjf: Thread Register Functions

//- rjf: thread register cache reading
internal void *ctrl_query_cached_reg_block_from_thread(Arena *arena, CTRL_MachineID machine_id, DMN_Handle thread, U64 endt_us);
internal U64 ctrl_query_cached_tls_root_vaddr_from_thread(CTRL_MachineID machine_id, DMN_Handle thread);

//- rjf: thread register writing
internal B32 ctrl_thread_write_reg_block(CTRL_MachineID machine_id, DMN_Handle thread, void *block);

////////////////////////////////
//~ rjf: Unwinding Functions

internal CTRL_Unwind ctrl_unwind_from_thread(Arena *arena, CTRL_MachineID machine_id, DMN_Handle thread);

////////////////////////////////
//~ rjf: Halting All Attached Processes

internal void ctrl_halt(void);

////////////////////////////////
//~ rjf: Shared Accessor Functions

//- rjf: run indices
internal U64 ctrl_run_idx(void);
internal U64 ctrl_memgen_idx(void);
internal U64 ctrl_reggen_idx(void);

//- rjf: name -> register/alias hash tables, for eval
internal EVAL_String2NumMap *ctrl_string2reg_from_arch(Architecture arch);
internal EVAL_String2NumMap *ctrl_string2alias_from_arch(Architecture arch);

////////////////////////////////
//~ rjf: Control-Thread Functions

//- rjf: user -> control thread communication
internal B32 ctrl_u2c_push_msgs(CTRL_MsgList *msgs, U64 endt_us);
internal CTRL_MsgList ctrl_u2c_pop_msgs(Arena *arena);

//- rjf: control -> user thread communication
internal void ctrl_c2u_push_events(CTRL_EventList *events);
internal CTRL_EventList ctrl_c2u_pop_events(Arena *arena);

//- rjf: entity tree construction
internal CTRL_Entity *ctrl_thread__entity_alloc(CTRL_Entity *parent, CTRL_EntityKind kind, CTRL_MachineID machine_id, DMN_Handle handle);
internal void ctrl_thread__entity_release(CTRL_Entity *entity);
internal CTRL_Entity *ctrl_thread__entity_from_machine_id_handle(CTRL_MachineID machine_id, DMN_Handle handle);

//- rjf: entry point
internal void ctrl_thread__entry_point(void *p);

//- rjf: attached process running/event gathering
internal DMN_Event *ctrl_thread__next_dmn_event(Arena *arena, CTRL_Msg *msg, DMN_RunCtrls *run_ctrls, CTRL_Spoof *spoof);

//- rjf: eval helpers
internal B32 ctrl_eval_memory_read(void *u, void *out, U64 addr, U64 size);

//- rjf: msg kind implementations
internal void ctrl_thread__launch_and_handshake(CTRL_Msg *msg);
internal void ctrl_thread__launch_and_init(CTRL_Msg *msg);
internal void ctrl_thread__attach(CTRL_Msg *msg);
internal void ctrl_thread__kill(CTRL_Msg *msg);
internal void ctrl_thread__detach(CTRL_Msg *msg);
internal void ctrl_thread__run(CTRL_Msg *msg);
internal void ctrl_thread__single_step(CTRL_Msg *msg);

////////////////////////////////
//~ rjf: Memory-Stream Thread Functions

//- rjf: user -> memory stream communication
internal B32 ctrl_u2ms_enqueue_req(CTRL_MachineID machine_id, DMN_Handle process, Rng1U64 vaddr_range, B32 zero_terminated, U64 endt_us);
internal void ctrl_u2ms_dequeue_req(CTRL_MachineID *out_machine_id, DMN_Handle *out_process, Rng1U64 *out_vaddr_range, B32 *out_zero_terminated);

//- rjf: entry point
internal void ctrl_mem_stream_thread__entry_point(void *p);

#endif // CTRL_CORE_H
