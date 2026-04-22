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
  CTRL_Handle machine;
  CTRL_Handle process;
  CTRL_Handle thread;
  CTRL_Handle entity;
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
  CTRL_Handle process;
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
//~ rjf: Ctrl -> User Event Types

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
  CTRL_EventKind_SetVAddrRangeNote,
  
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
  U64 tls_index;
  U64 tls_offset;
  U64 timestamp;
  U32 exception_code;
  U32 rgba;
  CTRL_UserBreakpointFlags bp_flags;
  String8 string;
  OperatingSystem target_os;
  CTRL_TlsModel tls_model;
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
  CTRL_Handle ctrl_last_run_thread_handle;
  CTRL_RunFlags ctrl_last_run_flags;
  CTRL_TrapList ctrl_last_run_traps;
  D_BreakpointArray ctrl_last_run_extra_bps;
  U128 ctrl_last_run_param_state_hash;
  B32 ctrl_is_running;
  B32 ctrl_thread_run_state;
  B32 ctrl_soft_halt_issued;
  Arena *ctrl_msg_arena;
  CTRL_MsgList ctrl_msgs;
  
  // rjf: ctrl -> user reading state
  CTRL_EntityCtxRWStore *ctrl_entity_store;
  Arena *ctrl_stop_arena;
  CTRL_Event ctrl_last_stop_event;
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

internal D_TrapNet d_trap_net_from_thread__step_over_inst(Arena *arena, CTRL_Entity *thread);
internal D_TrapNet d_trap_net_from_thread__step_over_line(Arena *arena, CTRL_Entity *thread);
internal D_TrapNet d_trap_net_from_thread__step_into_line(Arena *arena, CTRL_Entity *thread);

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

internal U64 d_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr);

////////////////////////////////
//~ rjf: Target Controls

//- rjf: stopped info from the control thread
internal CTRL_Event d_ctrl_last_stop_event(void);

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
internal U64 d_query_cached_rip_from_thread(CTRL_Entity *thread);
internal U64 d_query_cached_rip_from_thread_unwind(CTRL_Entity *thread, U64 unwind_count);
internal U64 d_query_cached_cfa_from_thread_unwind(CTRL_Entity *thread, U64 unwind_count);
internal U64 d_query_cached_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal E_String2NumMap *d_query_cached_locals_map_from_dbgi_key_voff(DI_Key dbgi_key, U64 voff);
internal E_String2NumMap *d_query_cached_member_map_from_dbgi_key_voff(DI_Key dbgi_key, U64 voff);

//- rjf: top-level command dispatch
internal void d_push_cmd(D_CmdKind kind, D_CmdParams *params);
#define d_cmd(kind, ...) d_push_cmd((kind), &(D_CmdParams){.thread = {0}, __VA_ARGS__})

//- rjf: command iteration
internal B32 d_next_cmd(D_Cmd **cmd);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal D_EventList d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints, D_PathMapArray *path_maps, U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64]);

#endif // DBG_ENGINE_USER_H
