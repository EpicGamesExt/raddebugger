// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_ENGINE_CORE_H
#define DBG_ENGINE_CORE_H

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
  CTRL_Handle thread;
  U64 vaddr;
  U64 code;
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
//~ rjf: Generated Code

#include "dbg_engine/generated/dbg_engine.meta.h"

////////////////////////////////
//~ rjf: View Rules

typedef U32 D_ViewRuleSpecInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  D_ViewRuleSpecInfoFlag_Inherited      = (1<<0),
  D_ViewRuleSpecInfoFlag_Expandable     = (1<<1),
  D_ViewRuleSpecInfoFlag_ExprResolution = (1<<2),
  D_ViewRuleSpecInfoFlag_VizBlockProd   = (1<<3),
};

typedef struct D_ViewRuleSpecInfo D_ViewRuleSpecInfo;
struct D_ViewRuleSpecInfo
{
  String8 string;
  String8 display_string;
  String8 schema;
  String8 description;
  D_ViewRuleSpecInfoFlags flags;
};

typedef struct D_ViewRuleSpecInfoArray D_ViewRuleSpecInfoArray;
struct D_ViewRuleSpecInfoArray
{
  D_ViewRuleSpecInfo *v;
  U64 count;
};

typedef struct D_ViewRuleSpec D_ViewRuleSpec;
struct D_ViewRuleSpec
{
  D_ViewRuleSpec *hash_next;
  D_ViewRuleSpecInfo info;
};

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

//- rjf: per-thread unwind cache

typedef struct D_UnwindCacheNode D_UnwindCacheNode;
struct D_UnwindCacheNode
{
  D_UnwindCacheNode *next;
  D_UnwindCacheNode *prev;
  U64 reggen;
  U64 memgen;
  Arena *arena;
  CTRL_Handle thread;
  CTRL_Unwind unwind;
};

typedef struct D_UnwindCacheSlot D_UnwindCacheSlot;
struct D_UnwindCacheSlot
{
  D_UnwindCacheNode *first;
  D_UnwindCacheNode *last;
};

typedef struct D_UnwindCache D_UnwindCache;
struct D_UnwindCache
{
  U64 slots_count;
  D_UnwindCacheSlot *slots;
  D_UnwindCacheNode *free_node;
};

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
//~ rjf: Main State Types

typedef struct D_State D_State;
struct D_State
{
  // rjf: top-level state
  Arena *arena;
  U64 frame_index;
  
  // rjf: commands
  Arena *cmds_arena;
  D_CmdList cmds;
  
  // rjf: output log key
  U128 output_log_key;
  
  // rjf: per-run caches
  D_UnwindCache unwind_cache;
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
  
  // rjf: view rule specification table
  U64 view_rule_spec_table_size;
  D_ViewRuleSpec **view_rule_spec_table;
  
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
  CTRL_EntityStore *ctrl_entity_store;
  Arena *ctrl_stop_arena;
  CTRL_Event ctrl_last_stop_event;
};

////////////////////////////////
//~ rjf: Globals

read_only global D_ViewRuleSpec d_nil_core_view_rule_spec = {0};
global D_State *d_state = 0;

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
//~ rjf: View Rule Spec Stateful Functions

internal void d_register_view_rule_specs(D_ViewRuleSpecInfoArray specs);
internal D_ViewRuleSpec *d_view_rule_spec_from_string(String8 string);

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

internal CTRL_TrapList d_trap_net_from_thread__step_over_inst(Arena *arena, CTRL_Entity *thread);
internal CTRL_TrapList d_trap_net_from_thread__step_over_line(Arena *arena, CTRL_Entity *thread);
internal CTRL_TrapList d_trap_net_from_thread__step_into_line(Arena *arena, CTRL_Entity *thread);

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: voff|vaddr -> symbol lookups
internal String8 d_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff, U64 depth, B32 decorated);
internal String8 d_symbol_name_from_process_vaddr(Arena *arena, CTRL_Entity *process, U64 vaddr, U64 depth, B32 decorated);

//- rjf: symbol -> voff lookups
internal U64 d_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name);
internal U64 d_type_num_from_dbgi_key_name(DI_Key *dbgi_key, String8 name);

//- rjf: voff -> line info
internal D_LineList d_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff);

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
internal CTRL_Unwind d_query_cached_unwind_from_thread(CTRL_Entity *thread);
internal U64 d_query_cached_rip_from_thread(CTRL_Entity *thread);
internal U64 d_query_cached_rip_from_thread_unwind(CTRL_Entity *thread, U64 unwind_count);
internal U64 d_query_cached_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal E_String2NumMap *d_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff);
internal E_String2NumMap *d_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff);

//- rjf: top-level command dispatch
internal void d_push_cmd(D_CmdKind kind, D_CmdParams *params);
#define d_cmd(kind, ...) d_push_cmd((kind), &(D_CmdParams){.thread = {0}, __VA_ARGS__})

//- rjf: command iteration
internal B32 d_next_cmd(D_Cmd **cmd);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void d_init(void);
internal D_EventList d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints, D_PathMapArray *path_maps, U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64]);

#endif // DBG_ENGINE_CORE_H
