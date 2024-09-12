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
  String8List env;
};

typedef struct D_TargetArray D_TargetArray;
struct D_TargetArray
{
  D_Target *v;
  U64 count;
};

typedef struct D_Breakpoint D_Breakpoint;
struct D_Breakpoint
{
  String8 file_path;
  TxtPt pt;
  String8 symbol_name;
  U64 vaddr;
  String8 condition;
};

typedef struct D_BreakpointArray D_BreakpointArray;
struct D_BreakpointArray
{
  D_Breakpoint *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Tick Output Types

typedef enum D_EventKind
{
  D_EventKind_Null,
  D_EventKind_Stop,
  D_EventKind_COUNT
}
D_EventKind;

typedef enum D_EventCause
{
  D_EventCause_Null,
  D_EventCause_UserBreakpoint,
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
//~ rjf: Handles

typedef struct D_Handle D_Handle;
struct D_Handle
{
  U64 u64[2];
};

typedef struct D_HandleNode D_HandleNode;
struct D_HandleNode
{
  D_HandleNode *next;
  D_HandleNode *prev;
  D_Handle handle;
};

typedef struct D_HandleList D_HandleList;
struct D_HandleList
{
  D_HandleNode *first;
  D_HandleNode *last;
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
//~ rjf: Entity Kind Flags

typedef U32 D_EntityKindFlags;
enum
{
  //- rjf: allowed operations
  D_EntityKindFlag_CanDelete                = (1<<0),
  D_EntityKindFlag_CanFreeze                = (1<<1),
  D_EntityKindFlag_CanEdit                  = (1<<2),
  D_EntityKindFlag_CanRename                = (1<<3),
  D_EntityKindFlag_CanEnable                = (1<<4),
  D_EntityKindFlag_CanCondition             = (1<<5),
  D_EntityKindFlag_CanDuplicate             = (1<<6),
  
  //- rjf: name categorization
  D_EntityKindFlag_NameIsCode               = (1<<7),
  D_EntityKindFlag_NameIsPath               = (1<<8),
  
  //- rjf: lifetime categorization
  D_EntityKindFlag_UserDefinedLifetime      = (1<<9),
  
  //- rjf: serialization
  D_EntityKindFlag_IsSerializedToConfig     = (1<<10),
};

////////////////////////////////
//~ rjf: Entity Filesystem Lookup Flags

typedef U32 D_EntityFromPathFlags;
enum
{
  D_EntityFromPathFlag_AllowOverrides = (1<<0),
  D_EntityFromPathFlag_OpenAsNeeded   = (1<<1),
  D_EntityFromPathFlag_OpenMissing    = (1<<2),
  D_EntityFromPathFlag_All = 0xffffffff,
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
//~ rjf: Config Types

typedef struct D_CfgTree D_CfgTree;
struct D_CfgTree
{
  D_CfgTree *next;
  D_CfgSrc source;
  MD_Node *root;
};

typedef struct D_CfgVal D_CfgVal;
struct D_CfgVal
{
  D_CfgVal *hash_next;
  D_CfgVal *linear_next;
  D_CfgTree *first;
  D_CfgTree *last;
  U64 insertion_stamp;
  String8 string;
};

typedef struct D_CfgSlot D_CfgSlot;
struct D_CfgSlot
{
  D_CfgVal *first;
};

typedef struct D_CfgTable D_CfgTable;
struct D_CfgTable
{
  U64 slot_count;
  D_CfgSlot *slots;
  U64 insertion_stamp_counter;
  D_CfgVal *first_val;
  D_CfgVal *last_val;
};

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
//~ rjf: Entity Types

typedef U32 D_EntityFlags;
enum
{
  //- rjf: allocationless, simple equipment
  D_EntityFlag_HasTextPoint      = (1<<0),
  D_EntityFlag_HasEntityHandle   = (1<<2),
  D_EntityFlag_HasU64            = (1<<4),
  D_EntityFlag_HasColor          = (1<<6),
  D_EntityFlag_DiesOnRunStop     = (1<<8),
  
  //- rjf: ctrl entity equipment
  D_EntityFlag_HasCtrlHandle     = (1<<9),
  D_EntityFlag_HasArch           = (1<<10),
  D_EntityFlag_HasCtrlID         = (1<<11),
  D_EntityFlag_HasStackBase      = (1<<12),
  D_EntityFlag_HasTLSRoot        = (1<<13),
  D_EntityFlag_HasVAddrRng       = (1<<14),
  D_EntityFlag_HasVAddr          = (1<<15),
  
  //- rjf: file properties
  D_EntityFlag_IsFolder          = (1<<16),
  D_EntityFlag_IsMissing         = (1<<17),
  
  //- rjf: deletion
  D_EntityFlag_MarkedForDeletion = (1<<31),
};

typedef U64 D_EntityID;

typedef struct D_Entity D_Entity;
struct D_Entity
{
  // rjf: tree links
  D_Entity *first;
  D_Entity *last;
  D_Entity *next;
  D_Entity *prev;
  D_Entity *parent;
  
  // rjf: metadata
  D_EntityKind kind;
  D_EntityFlags flags;
  D_EntityID id;
  U64 gen;
  U64 alloc_time_us;
  F32 alive_t;
  
  // rjf: basic equipment
  TxtPt text_point;
  D_Handle entity_handle;
  B32 disabled;
  U64 u64;
  Vec4F32 color_hsva;
  D_CfgSrc cfg_src;
  U64 timestamp;
  
  // rjf: ctrl equipment
  CTRL_Handle ctrl_handle;
  Arch arch;
  U32 ctrl_id;
  U64 stack_base;
  Rng1U64 vaddr_rng;
  U64 vaddr;
  
  // rjf: string equipment
  String8 string;
  
  // rjf: parameter tree
  Arena *params_arena;
  MD_Node *params_root;
};

typedef struct D_EntityNode D_EntityNode;
struct D_EntityNode
{
  D_EntityNode *next;
  D_Entity *entity;
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
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Entity Evaluation Types

typedef struct D_EntityEval D_EntityEval;
struct D_EntityEval
{
  B64 enabled;
  U64 hit_count;
  U64 label_off;
  U64 location_off;
  U64 condition_off;
};

////////////////////////////////
//~ rjf: Entity Fuzzy Listing Types

typedef struct D_EntityFuzzyItem D_EntityFuzzyItem;
struct D_EntityFuzzyItem
{
  D_Entity *entity;
  FuzzyMatchRangeList matches;
};

typedef struct D_EntityFuzzyItemArray D_EntityFuzzyItemArray;
struct D_EntityFuzzyItemArray
{
  D_EntityFuzzyItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Rich (Including Inline) Unwind Types

typedef struct D_UnwindInlineFrame D_UnwindInlineFrame;
struct D_UnwindInlineFrame
{
  D_UnwindInlineFrame *next;
  D_UnwindInlineFrame *prev;
  RDI_InlineSite *inline_site;
};

typedef struct D_UnwindFrame D_UnwindFrame;
struct D_UnwindFrame
{
  D_UnwindInlineFrame *first_inline_frame;
  D_UnwindInlineFrame *last_inline_frame;
  U64 inline_frame_count;
  void *regs;
  RDI_Parsed *rdi;
  RDI_Procedure *procedure;
};

typedef struct D_UnwindFrameArray D_UnwindFrameArray;
struct D_UnwindFrameArray
{
  D_UnwindFrame *v;
  U64 concrete_frame_count;
  U64 inline_frame_count;
  U64 total_frame_count;
};

typedef struct D_Unwind D_Unwind;
struct D_Unwind
{
  D_UnwindFrameArray frames;
};

////////////////////////////////
//~ rjf: Context Register Types

typedef struct D_RegsNode D_RegsNode;
struct D_RegsNode
{
  D_RegsNode *next;
  D_Regs v;
};

////////////////////////////////
//~ rjf: Command Specification Types

#if 0
typedef U32 D_CmdQueryFlags;
enum
{
  D_CmdQueryFlag_AllowFiles       = (1<<0),
  D_CmdQueryFlag_AllowFolders     = (1<<1),
  D_CmdQueryFlag_CodeInput        = (1<<2),
  D_CmdQueryFlag_KeepOldInput     = (1<<3),
  D_CmdQueryFlag_SelectOldInput   = (1<<4),
  D_CmdQueryFlag_Required         = (1<<5),
};

typedef struct D_CmdQuery D_CmdQuery;
struct D_CmdQuery
{
  D_CmdParamSlot slot;
  D_EntityKind entity_kind;
  D_CmdQueryFlags flags;
};

typedef U32 D_CmdSpecFlags;
enum
{
  D_CmdSpecFlag_ListInUI      = (1<<0),
  D_CmdSpecFlag_ListInIPCDocs = (1<<1),
};

typedef struct D_CmdSpecInfo D_CmdSpecInfo;
struct D_CmdSpecInfo
{
  String8 string;
  String8 description;
  String8 search_tags;
  String8 display_name;
  D_CmdSpecFlags flags;
  D_CmdQuery query;
};

typedef struct D_CmdSpec D_CmdSpec;
struct D_CmdSpec
{
  D_CmdSpec *hash_next;
  D_CmdSpecInfo info;
  U64 registrar_index;
  U64 ordering_index;
  U64 run_count;
};

typedef struct D_CmdSpecNode D_CmdSpecNode;
struct D_CmdSpecNode
{
  D_CmdSpecNode *next;
  D_CmdSpec *spec;
};

typedef struct D_CmdSpecList D_CmdSpecList;
struct D_CmdSpecList
{
  D_CmdSpecNode *first;
  D_CmdSpecNode *last;
  U64 count;
};

typedef struct D_CmdSpecArray D_CmdSpecArray;
struct D_CmdSpecArray
{
  D_CmdSpec **v;
  U64 count;
};

typedef struct D_CmdSpecInfoArray D_CmdSpecInfoArray;
struct D_CmdSpecInfoArray
{
  D_CmdSpecInfo *v;
  U64 count;
};
#endif

////////////////////////////////
//~ rjf: Command Types

typedef struct D_CmdParams D_CmdParams;
struct D_CmdParams
{
  CTRL_Handle machine;
  CTRL_Handle thread;
  CTRL_Handle entity;
  CTRL_HandleList processes;
  String8 file_path;
  TxtPt cursor;
  U64 vaddr;
  B32 prefer_disasm;
  U32 pid;
  D_TargetArray targets;
  //String8 string;
  //String8 file_path;
  //MD_Node * params_tree;
  //U64 vaddr;
  //U64 voff;
  //U64 index;
  //U64 id;
  //B32 prefer_dasm;
  //B32 force_confirm;
  //Dir2 dir2;
  //U64 unwind_index;
  //U64 inline_depth;
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

//- rjf: per-entity-kind state cache

typedef struct D_EntityListCache D_EntityListCache;
struct D_EntityListCache
{
  Arena *arena;
  U64 alloc_gen;
  D_EntityList list;
};

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

//- rjf: name allocator types

typedef struct D_NameChunkNode D_NameChunkNode;
struct D_NameChunkNode
{
  D_NameChunkNode *next;
  U64 size;
};

//- rjf: core bundle state type

typedef struct D_State D_State;
struct D_State
{
  // rjf: top-level state
  Arena *arena;
  U64 frame_index;
  U64 frame_eval_memread_endt_us;
  
  // rjf: frame info
  Arena *frame_arenas[2];
  
  // rjf: interaction registers
  D_RegsNode base_regs;
  D_RegsNode *top_regs;
  
  // rjf: commands
  Arena *cmds_arena;
  D_CmdList cmds;
  
  // rjf: output log key
  U128 output_log_key;
  
  // rjf: name allocator
  D_NameChunkNode *free_name_chunks[8];
  
  // rjf: entity state
  Arena *entities_arena;
  D_Entity *entities_base;
  U64 entities_count;
  U64 entities_id_gen;
  D_Entity *entities_root;
  D_Entity *entities_free[2]; // [0] -> normal lifetime, not user defined; [1] -> user defined lifetime (& thus undoable)
  U64 entities_free_count;
  U64 entities_active_count;
  
  // rjf: entity query caches
  U64 kind_alloc_gens[D_EntityKind_COUNT];
  D_EntityListCache kind_caches[D_EntityKind_COUNT];
  
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
  U128 ctrl_last_run_param_state_hash;
  B32 ctrl_is_running;
  B32 ctrl_soft_halt_issued;
  U64 ctrl_exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
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
read_only global D_CfgTree d_nil_cfg_tree = {&d_nil_cfg_tree, D_CfgSrc_User, &md_nil_node};
read_only global D_CfgVal d_nil_cfg_val = {&d_nil_cfg_val, &d_nil_cfg_val, &d_nil_cfg_tree, &d_nil_cfg_tree};
read_only global D_Entity d_nil_entity =
{
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
};

global D_State *d_state = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 d_hash_from_seed_string(U64 seed, String8 string);
internal U64 d_hash_from_string(String8 string);
internal U64 d_hash_from_seed_string__case_insensitive(U64 seed, String8 string);
internal U64 d_hash_from_string__case_insensitive(String8 string);

////////////////////////////////
//~ rjf: Handle Type Pure Functions

internal D_Handle d_handle_zero(void);
internal B32 d_handle_match(D_Handle a, D_Handle b);
internal void d_handle_list_push_node(D_HandleList *list, D_HandleNode *node);
internal void d_handle_list_push(Arena *arena, D_HandleList *list, D_Handle handle);
internal D_HandleList d_handle_list_copy(Arena *arena, D_HandleList list);

////////////////////////////////
//~ rjf: Registers Type Pure Functions

internal void d_regs_copy_contents(Arena *arena, D_Regs *dst, D_Regs *src);
internal D_Regs *d_regs_copy(Arena *arena, D_Regs *src);

////////////////////////////////
//~ rjf: Config Type Pure Functions

internal void d_cfg_table_push_unparsed_string(Arena *arena, D_CfgTable *table, String8 string, D_CfgSrc source);
internal D_CfgTable d_cfg_table_from_inheritance(Arena *arena, D_CfgTable *src);
internal D_CfgVal *d_cfg_val_from_string(D_CfgTable *table, String8 string);

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal D_LineList d_line_list_copy(Arena *arena, D_LineList *list);

////////////////////////////////
//~ rjf: Command Type Pure Functions

//- rjf: command parameter bundles
#if 0 // TODO(rjf): @msgs
internal D_CmdParams d_cmd_params_zero(void);
internal String8 d_cmd_params_apply_spec_query(Arena *arena, D_CmdParams *params, D_CmdSpec *spec, String8 query);
#endif

//- rjf: command parameters
internal D_CmdParams d_cmd_params_copy(Arena *arena, D_CmdParams *src);

//- rjf: command lists
internal void d_cmd_list_push_new(Arena *arena, D_CmdList *cmds, D_CmdKind kind, D_CmdParams *params);

////////////////////////////////
//~ rjf: Entity Type Pure Functions

//- rjf: nil
internal B32 d_entity_is_nil(D_Entity *entity);
#define d_require_entity_nonnil(entity, if_nil_stmts) do{if(d_entity_is_nil(entity)){if_nil_stmts;}}while(0)

//- rjf: handle <-> entity conversions
internal U64 d_index_from_entity(D_Entity *entity);
internal D_Handle d_handle_from_entity(D_Entity *entity);
internal D_Entity *d_entity_from_handle(D_Handle handle);
internal D_EntityList d_entity_list_from_handle_list(Arena *arena, D_HandleList handles);
internal D_HandleList d_handle_list_from_entity_list(Arena *arena, D_EntityList entities);

//- rjf: entity recursion iterators
internal D_EntityRec d_entity_rec_depth_first(D_Entity *entity, D_Entity *subtree_root, U64 sib_off, U64 child_off);
#define d_entity_rec_depth_first_pre(entity, subtree_root)  d_entity_rec_depth_first((entity), (subtree_root), OffsetOf(D_Entity, next), OffsetOf(D_Entity, first))
#define d_entity_rec_depth_first_post(entity, subtree_root) d_entity_rec_depth_first((entity), (subtree_root), OffsetOf(D_Entity, prev), OffsetOf(D_Entity, last))

//- rjf: ancestor/child introspection
internal D_Entity *d_entity_child_from_kind(D_Entity *entity, D_EntityKind kind);
internal D_Entity *d_entity_ancestor_from_kind(D_Entity *entity, D_EntityKind kind);
internal D_EntityList d_push_entity_child_list_with_kind(Arena *arena, D_Entity *entity, D_EntityKind kind);
internal D_Entity *d_entity_child_from_string_and_kind(D_Entity *parent, String8 string, D_EntityKind kind);

//- rjf: entity list building
internal void d_entity_list_push(Arena *arena, D_EntityList *list, D_Entity *entity);
internal D_EntityArray d_entity_array_from_list(Arena *arena, D_EntityList *list);
#define d_first_entity_from_list(list) ((list)->first != 0 ? (list)->first->entity : &d_nil_entity)

//- rjf: entity fuzzy list building
internal D_EntityFuzzyItemArray d_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, D_EntityList *list, String8 needle);
internal D_EntityFuzzyItemArray d_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, D_EntityArray *array, String8 needle);

//- rjf: full path building, from file/folder entities
internal String8 d_full_path_from_entity(Arena *arena, D_Entity *entity);

//- rjf: display string entities, for referencing entities in ui
internal String8 d_display_string_from_entity(Arena *arena, D_Entity *entity);

//- rjf: extra search tag strings for fuzzy filtering entities
internal String8 d_search_tags_from_entity(Arena *arena, D_Entity *entity);

//- rjf: entity -> color operations
internal Vec4F32 d_hsva_from_entity(D_Entity *entity);
internal Vec4F32 d_rgba_from_entity(D_Entity *entity);

//- rjf: entity -> expansion tree keys
internal EV_Key d_ev_key_from_entity(D_Entity *entity);
internal EV_Key d_parent_ev_key_from_entity(D_Entity *entity);

//- rjf: entity -> evaluation
internal D_EntityEval *d_eval_from_entity(Arena *arena, D_Entity *entity);

////////////////////////////////
//~ rjf: Name Allocation

internal U64 d_name_bucket_idx_from_string_size(U64 size);
internal String8 d_name_alloc(String8 string);
internal void d_name_release(String8 string);

////////////////////////////////
//~ rjf: Entity Stateful Functions

//- rjf: entity allocation + tree forming
internal D_Entity *d_entity_alloc(D_Entity *parent, D_EntityKind kind);
internal void d_entity_mark_for_deletion(D_Entity *entity);
internal void d_entity_release(D_Entity *entity);
internal void d_entity_change_parent(D_Entity *entity, D_Entity *old_parent, D_Entity *new_parent, D_Entity *prev_child);

//- rjf: entity simple equipment
internal void d_entity_equip_txt_pt(D_Entity *entity, TxtPt point);
internal void d_entity_equip_entity_handle(D_Entity *entity, D_Handle handle);
internal void d_entity_equip_disabled(D_Entity *entity, B32 b32);
internal void d_entity_equip_u64(D_Entity *entity, U64 u64);
internal void d_entity_equip_color_rgba(D_Entity *entity, Vec4F32 rgba);
internal void d_entity_equip_color_hsva(D_Entity *entity, Vec4F32 hsva);
internal void d_entity_equip_cfg_src(D_Entity *entity, D_CfgSrc cfg_src);
internal void d_entity_equip_timestamp(D_Entity *entity, U64 timestamp);

//- rjf: control layer correllation equipment
internal void d_entity_equip_ctrl_handle(D_Entity *entity, CTRL_Handle handle);
internal void d_entity_equip_arch(D_Entity *entity, Arch arch);
internal void d_entity_equip_ctrl_id(D_Entity *entity, U32 id);
internal void d_entity_equip_stack_base(D_Entity *entity, U64 stack_base);
internal void d_entity_equip_vaddr_rng(D_Entity *entity, Rng1U64 range);
internal void d_entity_equip_vaddr(D_Entity *entity, U64 vaddr);

//- rjf: name equipment
internal void d_entity_equip_name(D_Entity *entity, String8 name);
internal void d_entity_equip_namef(D_Entity *entity, char *fmt, ...);

//- rjf: file path map override lookups
internal String8List d_possible_overrides_from_file_path(Arena *arena, String8 file_path);

//- rjf: top-level state queries
internal D_Entity *d_entity_root(void);
internal D_EntityList d_push_entity_list_with_kind(Arena *arena, D_EntityKind kind);
internal D_Entity *d_entity_from_id(D_EntityID id);
internal D_Entity *d_machine_entity_from_machine_id(CTRL_MachineID machine_id);
internal D_Entity *d_entity_from_ctrl_handle(CTRL_Handle handle);
internal D_Entity *d_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id);
internal D_Entity *d_entity_from_name_and_kind(String8 string, D_EntityKind kind);

////////////////////////////////
//~ rjf: Command Stateful Functions

#if 0 // TODO(rjf): @msgs
internal void d_register_cmd_specs(D_CmdSpecInfoArray specs);
internal D_CmdSpec *d_cmd_spec_from_string(String8 string);
internal D_CmdSpec *d_cmd_spec_from_kind(D_CmdKind kind);
internal void d_cmd_spec_counter_inc(D_CmdSpec *spec);
internal D_CmdSpecList d_push_cmd_spec_list(Arena *arena);
#endif

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
//~ rjf: Modules & Debug Info Mappings

//- rjf: module <=> debug info keys
internal DI_Key d_dbgi_key_from_module(D_Entity *module);
internal D_EntityList d_modules_from_dbgi_key(Arena *arena, DI_Key *dbgi_key);

//- rjf: voff <=> vaddr
internal U64 d_base_vaddr_from_module(D_Entity *module);
internal U64 d_voff_from_vaddr(D_Entity *module, U64 vaddr);
internal U64 d_vaddr_from_voff(D_Entity *module, U64 voff);
internal Rng1U64 d_voff_range_from_vaddr_range(D_Entity *module, Rng1U64 vaddr_rng);
internal Rng1U64 d_vaddr_range_from_voff_range(D_Entity *module, Rng1U64 voff_rng);

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: voff|vaddr -> symbol lookups
internal String8 d_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff, B32 decorated);
internal String8 d_symbol_name_from_process_vaddr(Arena *arena, CTRL_Entity *process, U64 vaddr, B32 decorated);

//- rjf: symbol -> voff lookups
internal U64 d_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name);
internal U64 d_type_num_from_dbgi_key_name(DI_Key *dbgi_key, String8 name);

//- rjf: voff -> line info
internal D_LineList d_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff);

//- rjf: file:line -> line info
internal D_LineListArray d_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range);
internal D_LineList d_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num);

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal D_Entity *d_module_from_process_vaddr(D_Entity *process, U64 vaddr);
#if 0 // TODO(rjf): @msgs
internal D_Entity *d_module_from_thread(D_Entity *thread);
#endif
internal U64 d_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr);
internal Arch d_arch_from_entity(D_Entity *entity);
internal E_String2NumMap *d_push_locals_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff);
internal E_String2NumMap *d_push_member_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff);
#if 0 // TODO(rjf): @msgs
internal D_Entity *d_module_from_thread_candidates(D_Entity *thread, D_EntityList *candidates);
#endif
internal D_Unwind d_unwind_from_ctrl_unwind(Arena *arena, DI_Scope *di_scope, CTRL_Entity *process, CTRL_Unwind *base_unwind);

////////////////////////////////
//~ rjf: Target Controls

//- rjf: stopped info from the control thread
internal CTRL_Event d_ctrl_last_stop_event(void);

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: ctrl entity <-> eval space
internal CTRL_Entity *d_ctrl_entity_from_eval_space(E_Space space);
internal E_Space d_eval_space_from_ctrl_entity(CTRL_Entity *entity);

//- rjf: entity <-> eval space
internal D_Entity *d_entity_from_eval_space(E_Space space);
internal E_Space d_eval_space_from_entity(D_Entity *entity);

//- rjf: eval space reads/writes
internal B32 d_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range);
internal B32 d_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range);

//- rjf: asynchronous streamed reads -> hashes from spaces
internal U128 d_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated);

//- rjf: space -> entire range
internal Rng1U64 d_whole_range_from_eval_space(E_Space space);

////////////////////////////////
//~ rjf: Evaluation Visualization

//- rjf: writing values back to child processes
internal B32 d_commit_eval_value_string(E_Eval dst_eval, String8 string);

//- rjf: eval / view rule params tree info extraction
internal U64 d_base_offset_from_eval(E_Eval eval);
internal E_Value d_value_from_params_key(MD_Node *params, String8 key);
internal Rng1U64 d_range_from_eval_params(E_Eval eval, MD_Node *params);
internal TXT_LangKind d_lang_kind_from_eval_params(E_Eval eval, MD_Node *params);
internal Arch d_arch_from_eval_params(E_Eval eval, MD_Node *params);
internal Vec2S32 d_dim2s32_from_eval_params(E_Eval eval, MD_Node *params);
internal R_Tex2DFormat d_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params);

//- rjf: eval <-> entity
internal D_Entity *d_entity_from_eval_string(String8 string);
internal String8 d_eval_string_from_entity(Arena *arena, D_Entity *entity);

//- rjf: eval <-> file path
internal String8 d_file_path_from_eval_string(Arena *arena, String8 string);
internal String8 d_eval_string_from_file_path(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data
internal U64 d_frame_index(void);
internal Arena *d_frame_arena(void);

//- rjf: registers
internal D_Regs *d_regs(void);
internal D_Regs *d_base_regs(void);
internal D_Regs *d_push_regs(void);
internal D_Regs *d_pop_regs(void);
#define D_RegsScope DeferLoop(d_push_regs(), d_pop_regs())

//- rjf: control state
internal D_RunKind d_ctrl_last_run_kind(void);
internal U64 d_ctrl_last_run_frame_idx(void);
internal B32 d_ctrl_targets_running(void);

//- rjf: config serialization
internal String8 d_cfg_escaped_from_raw_string(Arena *arena, String8 string);
internal String8 d_cfg_raw_from_escaped_string(Arena *arena, String8 string);
internal String8List d_cfg_strings_from_core(Arena *arena, String8 root_path, D_CfgSrc source);

//- rjf: entity kind cache
internal D_EntityList d_query_cached_entity_list_with_kind(D_EntityKind kind);

//- rjf: active entity based queries
internal DI_KeyList d_push_active_dbgi_key_list(Arena *arena);
internal D_EntityList d_push_active_target_list(Arena *arena);

//- rjf: expand key based entity queries
internal D_Entity *d_entity_from_ev_key_and_kind(EV_Key key, D_EntityKind kind);

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
internal D_EventList d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints);

#endif // DBG_ENGINE_CORE_H
