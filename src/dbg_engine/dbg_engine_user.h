// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_ENGINE_USER_H
#define DBG_ENGINE_USER_H

////////////////////////////////
//~ rjf: Line Info Types

typedef struct D_Line D_Line;
struct D_Line
{
  String8 file_path;
  TxtPt pt;
  Rng1U64 voff_range;
  DI_Key dbgi_key;
};

typedef struct D_LineNode D_LineNode;
struct D_LineNode
{
  D_LineNode *next;
  D_Line v;
};

typedef struct D_LineList D_LineList;
struct D_LineList
{
  D_LineNode *first;
  D_LineNode *last;
  U64 count;
};

typedef struct D_LineListArray D_LineListArray;
struct D_LineListArray
{
  D_LineList *v;
  U64 count;
  DI_KeyList dbgi_keys;
};

////////////////////////////////
//~ rjf: Debug Engine Control Communication Types

typedef enum D_RunKind
{
  D_RunKind_Run,
  D_RunKind_SingleStep,
  D_RunKind_Step,
  D_RunKind_COUNT
}
D_RunKind;

////////////////////////////////
//~ rjf: Command Types

typedef struct D_CmdParams D_CmdParams;
struct D_CmdParams
{
  D_Handle machine;
  D_Handle process;
  D_Handle thread;
  D_Handle entity;
  String8 string;
  String8 file_path;
  TxtPt cursor;
  U64 vaddr;
  B32 prefer_disasm;
  U32 pid;
  U32 rgba;
  D_TargetArray targets;
  U64 retry_idx;
};

typedef struct D_Cmd D_Cmd;
struct D_Cmd
{
  D_CmdKind kind;
  D_CmdParams params;
};

typedef struct D_CmdNode D_CmdNode;
struct D_CmdNode
{
  D_CmdNode *next;
  D_CmdNode *prev;
  D_Cmd cmd;
};

typedef struct D_CmdList D_CmdList;
struct D_CmdList
{
  D_CmdNode *first;
  D_CmdNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Main State Caches

//- rjf: per-run tls-base-vaddr cache

typedef struct D_RunTLSBaseCacheNode D_RunTLSBaseCacheNode;
struct D_RunTLSBaseCacheNode
{
  D_RunTLSBaseCacheNode *hash_next;
  D_Handle process;
  U64 root_vaddr;
  U64 rip_vaddr;
  U64 tls_base_vaddr;
};

typedef struct D_RunTLSBaseCacheSlot D_RunTLSBaseCacheSlot;
struct D_RunTLSBaseCacheSlot
{
  D_RunTLSBaseCacheNode *first;
  D_RunTLSBaseCacheNode *last;
};

typedef struct D_RunTLSBaseCache D_RunTLSBaseCache;
struct D_RunTLSBaseCache
{
  Arena *arena;
  U64 slots_count;
  D_RunTLSBaseCacheSlot *slots;
};

//- rjf: per-run locals cache

typedef struct D_RunLocalsCacheNode D_RunLocalsCacheNode;
struct D_RunLocalsCacheNode
{
  D_RunLocalsCacheNode *hash_next;
  DI_Key dbgi_key;
  U64 voff;
  E_String2NumMap *locals_map;
};

typedef struct D_RunLocalsCacheSlot D_RunLocalsCacheSlot;
struct D_RunLocalsCacheSlot
{
  D_RunLocalsCacheNode *first;
  D_RunLocalsCacheNode *last;
};

typedef struct D_RunLocalsCache D_RunLocalsCache;
struct D_RunLocalsCache
{
  Arena *arena;
  U64 table_size;
  D_RunLocalsCacheSlot *table;
};

////////////////////////////////
//~ rjf: User -> Ctrl Message Types

typedef enum D_MsgKind
{
  D_MsgKind_Null,
  D_MsgKind_Launch,
  D_MsgKind_Attach,
  D_MsgKind_Kill,
  D_MsgKind_KillAll,
  D_MsgKind_Detach,
  D_MsgKind_Run,
  D_MsgKind_SingleStep,
  D_MsgKind_SetUserEntryPoints,
  D_MsgKind_SetModuleDebugInfoPath,
  D_MsgKind_FreezeThread,
  D_MsgKind_ThawThread,
  D_MsgKind_COUNT,
}
D_MsgKind;

typedef U32 D_RunFlags;
enum
{
  D_RunFlag_StopOnEntryPoint = (1<<0),
};

typedef struct D_Msg D_Msg;
struct D_Msg
{
  D_MsgKind kind;
  D_RunFlags run_flags;
  D_MsgID msg_id;
  D_Handle entity;
  D_Handle parent;
  U32 entity_id;
  U32 exit_code;
  B32 env_inherit;
  B32 debug_subprocesses;
  U64 exception_code_filters[(D_ExceptionCodeKind_COUNT+63)/64];
  String8 path;
  String8List entry_points;
  String8List cmd_line_string_list;
  String8List env_string_list;
  String8 stdout_path;
  String8 stderr_path;
  String8 stdin_path;
  D_TrapList traps;
  CTRL_UserBreakpointList user_bps;
};

typedef struct D_MsgNode D_MsgNode;
struct D_MsgNode
{
  D_MsgNode *next;
  D_Msg v;
};

typedef struct D_MsgList D_MsgList;
struct D_MsgList
{
  D_MsgNode *first;
  D_MsgNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Ctrl -> User Event Types

typedef enum D_EventKind
{
  D_EventKind_Null,
  D_EventKind_Error,
  
  //- rjf: starts/stops
  D_EventKind_Started,
  D_EventKind_Stopped,
  
  //- rjf: entity creation/deletion
  D_EventKind_NewProc,
  D_EventKind_NewThread,
  D_EventKind_NewModule,
  D_EventKind_EndProc,
  D_EventKind_EndThread,
  D_EventKind_EndModule,
  
  //- rjf: thread freeze state changes
  D_EventKind_ThreadFrozen,
  D_EventKind_ThreadThawed,
  
  //- rjf: debug info changes
  D_EventKind_ModuleDebugInfoPathChange,
  
  //- rjf: debug strings / decorations / markup
  D_EventKind_DebugString,
  D_EventKind_ThreadName,
  D_EventKind_ThreadColor,
  D_EventKind_SetBreakpoint,
  D_EventKind_UnsetBreakpoint,
  D_EventKind_SetVAddrRangeNote,
  
  //- rjf: memory
  D_EventKind_MemReserve,
  D_EventKind_MemCommit,
  D_EventKind_MemDecommit,
  D_EventKind_MemRelease,
  
  D_EventKind_COUNT
}
D_EventKind;

typedef enum D_EventCause
{
  D_EventCause_Null,
  D_EventCause_Error,
  D_EventCause_Finished,
  D_EventCause_EntryPoint,
  D_EventCause_UserBreakpoint,
  D_EventCause_InterruptedByTrap,
  D_EventCause_InterruptedByException,
  D_EventCause_InterruptedByHalt,
  D_EventCause_COUNT
}
D_EventCause;

typedef enum D_ExceptionKind
{
  D_ExceptionKind_Null,
  D_ExceptionKind_MemoryRead,
  D_ExceptionKind_MemoryWrite,
  D_ExceptionKind_MemoryExecute,
  D_ExceptionKind_CppThrow,
  D_ExceptionKind_COUNT
}
D_ExceptionKind;

typedef struct D_Event D_Event;
struct D_Event
{
  D_EventKind kind;
  D_EventCause cause;
  D_ExceptionKind exception_kind;
  D_MsgID msg_id;
  D_Handle entity;
  D_Handle parent;
  Arch arch;
  U64 u64_code;
  U32 entity_id;
  Rng1U64 vaddr_rng;
  U64 rip_vaddr;
  U64 stack_base;
  U64 tls_root;
  U64 tls_index;
  U64 tls_offset;
  U64 timestamp;
  U32 exception_code;
  U32 rgba;
  D_BreakpointFlags bp_flags;
  String8 string;
  OperatingSystem target_os;
  D_TlsModel tls_model;
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
//~ rjf: Main State Types

typedef struct D_UserState D_UserState;
struct D_UserState
{
  // rjf: top-level state
  Arena *arena;
  U64 frame_index;
  
  // rjf: commands
  Arena *cmds_arena;
  D_CmdList cmds;
  
  // rjf: output log key
  C_Key output_log_key;
  
  // rjf: per-run caches
  U64 tls_base_cache_reggen_idx;
  U64 tls_base_cache_memgen_idx;
  D_RunTLSBaseCache tls_base_caches[2];
  U64 tls_base_cache_gen;
  U64 locals_cache_reggen_idx;
  D_RunLocalsCache locals_caches[2];
  U64 locals_cache_gen;
  U64 member_cache_reggen_idx;
  D_RunLocalsCache member_caches[2];
  U64 member_cache_gen;
  
  // rjf: user -> ctrl driving state
  Arena *ctrl_last_run_arena;
  D_RunKind ctrl_last_run_kind;
  U64 ctrl_last_run_frame_idx;
  D_Handle ctrl_last_run_thread_handle;
  D_RunFlags ctrl_last_run_flags;
  D_TrapList ctrl_last_run_traps;
  D_BreakpointArray ctrl_last_run_extra_bps;
  U128 ctrl_last_run_param_state_hash;
  B32 ctrl_is_running;
  B32 ctrl_thread_run_state;
  B32 ctrl_soft_halt_issued;
  Arena *ctrl_msg_arena;
  D_MsgList ctrl_msgs;
  
  // rjf: ctrl -> user reading state
  D_EntityCtxRWStore *ctrl_entity_store;
  Arena *ctrl_stop_arena;
  D_Event ctrl_last_stop_event;
};

////////////////////////////////
//~ rjf: Globals

global D_UserState *d_user_state = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 d_hash_from_seed_string(U64 seed, String8 string);
internal U64 d_hash_from_string(String8 string);
internal U64 d_hash_from_seed_string__case_insensitive(U64 seed, String8 string);
internal U64 d_hash_from_string__case_insensitive(String8 string);

////////////////////////////////
//~ rjf: Breakpoints

internal D_BreakpointArray d_breakpoint_array_copy(Arena *arena, D_BreakpointArray *src);

////////////////////////////////
//~ rjf: Path Map Application

internal String8List d_possible_path_overrides_from_maps_path(Arena *arena, D_PathMapArray *path_maps, String8 file_path);

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal D_LineList d_line_list_copy(Arena *arena, D_LineList *list);

////////////////////////////////
//~ rjf: Command Type Functions

//- rjf: command parameters
internal D_CmdParams d_cmd_params_copy(Arena *arena, D_CmdParams *src);

//- rjf: command lists
internal void d_cmd_list_push_new(Arena *arena, D_CmdList *cmds, D_CmdKind kind, D_CmdParams *params);

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

internal D_TrapNet d_trap_net_from_thread__step_over_inst(Arena *arena, D_Entity *thread);
internal D_TrapNet d_trap_net_from_thread__step_over_line(Arena *arena, D_Entity *thread);
internal D_TrapNet d_trap_net_from_thread__step_into_line(Arena *arena, D_Entity *thread);

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: voff -> line info
internal D_LineList d_lines_from_dbgi_key_voff(Arena *arena, DI_Key dbgi_key, U64 voff);

//- rjf: file:line -> line info
// TODO(rjf): this depends on file path maps, needs to move
// TODO(rjf): need to clean this up & dedup
internal D_LineListArray d_lines_array_from_dbgi_key_file_path_line_range(Arena *arena, DI_Key dbgi_key, String8 file_path, Rng1S64 line_num_range);
internal D_LineListArray d_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range);
internal D_LineList d_lines_from_dbgi_key_file_path_line_num(Arena *arena, DI_Key dbgi_key, String8 file_path, S64 line_num);
internal D_LineList d_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num);

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal U64 d_tls_base_vaddr_from_process_root_rip(D_Entity *process, U64 root_vaddr, U64 rip_vaddr);

////////////////////////////////
//~ rjf: Target Controls

//- rjf: stopped info from the control thread
internal D_Event d_ctrl_last_stop_event(void);

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data
internal U64 d_frame_index(void);

//- rjf: control state
internal D_RunKind d_ctrl_last_run_kind(void);
internal U64 d_ctrl_last_run_frame_idx(void);
internal B32 d_ctrl_targets_running(void);

//- rjf: active entity based queries
internal DI_KeyList d_push_active_dbgi_key_list(Arena *arena);

//- rjf: per-run caches
internal U64 d_query_cached_rip_from_thread(D_Entity *thread);
internal U64 d_query_cached_rip_from_thread_unwind(D_Entity *thread, U64 unwind_count);
internal U64 d_query_cached_cfa_from_thread_unwind(D_Entity *thread, U64 unwind_count);
internal U64 d_query_cached_tls_base_vaddr_from_process_root_rip(D_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal E_String2NumMap *d_query_cached_locals_map_from_dbgi_key_voff(DI_Key dbgi_key, U64 voff);
internal E_String2NumMap *d_query_cached_member_map_from_dbgi_key_voff(DI_Key dbgi_key, U64 voff);

//- rjf: top-level command dispatch
internal void d_push_cmd(D_CmdKind kind, D_CmdParams *params);
#define d_cmd(kind, ...) d_push_cmd((kind), &(D_CmdParams){.thread = {0}, __VA_ARGS__})

//- rjf: command iteration
internal B32 d_next_cmd(D_Cmd **cmd);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal D_EventList d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints, D_PathMapArray *path_maps, U64 exception_code_filters[(D_ExceptionCodeKind_COUNT+63)/64]);

#endif // DBG_ENGINE_USER_H
