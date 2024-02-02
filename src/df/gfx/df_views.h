// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEBUG_FRONTEND_VIEWS_H
#define DEBUG_FRONTEND_VIEWS_H

////////////////////////////////
//~ rjf: FileSystem @view_types

typedef enum DF_FileSortKind
{
  DF_FileSortKind_Null,
  DF_FileSortKind_Filename,
  DF_FileSortKind_LastModified,
  DF_FileSortKind_Size,
  DF_FileSortKind_COUNT
}
DF_FileSortKind;

typedef struct DF_FileInfo DF_FileInfo;
struct DF_FileInfo
{
  String8 filename;
  FileProperties props;
  FuzzyMatchRangeList match_ranges;
};

typedef struct DF_FileInfoNode DF_FileInfoNode;
struct DF_FileInfoNode
{
  DF_FileInfoNode *next;
  DF_FileInfo file_info;
};

typedef struct DF_FileSystemViewPathState DF_FileSystemViewPathState;
struct DF_FileSystemViewPathState
{
  DF_FileSystemViewPathState *hash_next;
  String8 normalized_path;
  Vec2S64 cursor;
};

typedef struct DF_FileSystemViewState DF_FileSystemViewState;
struct DF_FileSystemViewState
{
  B32 initialized;
  U64 path_state_table_size;
  DF_FileSystemViewPathState **path_state_table;
  DF_FileSortKind sort_kind;
  Side sort_side;
  Arena *cached_files_arena;
  String8 cached_files_path;
  DF_FileSortKind cached_files_sort_kind;
  Side cached_files_sort_side;
  U64 cached_file_count;
  DF_FileInfo *cached_files;
  F32 col_pcts[3];
};

////////////////////////////////
//~ rjf: Commands @view_types

typedef struct DF_CmdListerItem DF_CmdListerItem;
struct DF_CmdListerItem
{
  DF_CmdSpec *cmd_spec;
  U64 registrar_idx;
  U64 ordering_idx;
  FuzzyMatchRangeList name_match_ranges;
  FuzzyMatchRangeList desc_match_ranges;
  FuzzyMatchRangeList tags_match_ranges;
};

typedef struct DF_CmdListerItemNode DF_CmdListerItemNode;
struct DF_CmdListerItemNode
{
  DF_CmdListerItemNode *next;
  DF_CmdListerItem item;
};

typedef struct DF_CmdListerItemList DF_CmdListerItemList;
struct DF_CmdListerItemList
{
  DF_CmdListerItemNode *first;
  DF_CmdListerItemNode *last;
  U64 count;
};

typedef struct DF_CmdListerItemArray DF_CmdListerItemArray;
struct DF_CmdListerItemArray
{
  DF_CmdListerItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: PendingEntity @view_types

typedef struct DF_PendingEntityViewState DF_PendingEntityViewState;
struct DF_PendingEntityViewState
{
  Arena *deferred_cmd_arena;
  DF_CmdList deferred_cmds;
  DF_Handle pick_file_override_target;
  Arena *complete_cfg_arena;
  DF_CfgNode *complete_cfg_root;
};

////////////////////////////////
//~ rjf: EntityLister @view_types

typedef struct DF_EntityListerItem DF_EntityListerItem;
struct DF_EntityListerItem
{
  DF_Entity *entity;
  FuzzyMatchRangeList name_match_ranges;
};

typedef struct DF_EntityListerItemNode DF_EntityListerItemNode;
struct DF_EntityListerItemNode
{
  DF_EntityListerItemNode *next;
  DF_EntityListerItem item;
};

typedef struct DF_EntityListerItemList DF_EntityListerItemList;
struct DF_EntityListerItemList
{
  DF_EntityListerItemNode *first;
  DF_EntityListerItemNode *last;
  U64 count;
};

typedef struct DF_EntityListerItemArray DF_EntityListerItemArray;
struct DF_EntityListerItemArray
{
  DF_EntityListerItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: SystemProcesses @view_types

typedef struct DF_ProcessInfo DF_ProcessInfo;
struct DF_ProcessInfo
{
  DEMON_ProcessInfo info;
  B32 is_attached;
  FuzzyMatchRangeList attached_match_ranges;
  FuzzyMatchRangeList name_match_ranges;
  FuzzyMatchRangeList pid_match_ranges;
};

typedef struct DF_ProcessInfoNode DF_ProcessInfoNode;
struct DF_ProcessInfoNode
{
  DF_ProcessInfoNode *next;
  DF_ProcessInfo info;
};

typedef struct DF_ProcessInfoList DF_ProcessInfoList;
struct DF_ProcessInfoList
{
  DF_ProcessInfoNode *first;
  DF_ProcessInfoNode *last;
  U64 count;
};

typedef struct DF_ProcessInfoArray DF_ProcessInfoArray;
struct DF_ProcessInfoArray
{
  DF_ProcessInfo *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Breakpoint @view_types

typedef struct DF_BreakpointViewState DF_BreakpointViewState;
struct DF_BreakpointViewState
{
  B32 initialized;
  Vec2S32 selected_p;
  F32 key_pct;
  F32 val_pct;
};

////////////////////////////////
//~ rjf: Target @view_types

typedef struct DF_TargetViewState DF_TargetViewState;
struct DF_TargetViewState
{
  B32 initialized;
  
  // rjf: pick file kind
  DF_EntityKind pick_dst_kind;
  
  // rjf: selection cursor
  Vec2S64 cursor;
  
  // rjf: text input state
  TxtPt input_cursor;
  TxtPt input_mark;
  U8 input_buffer[1024];
  U64 input_size;
  B32 input_editing;
  
  // rjf: table column pcts
  F32 key_pct;
  F32 value_pct;
};

////////////////////////////////
//~ rjf: FilePathMap @view_types

typedef struct DF_FilePathMapViewState DF_FilePathMapViewState;
struct DF_FilePathMapViewState
{
  B32 initialized;
  Vec2S64 cursor;
  TxtPt input_cursor;
  TxtPt input_mark;
  U8 input_buffer[1024];
  U64 input_size;
  B32 input_editing;
  DF_Handle pick_file_dst_map;
  Side pick_file_dst_side;
  F32 src_column_pct;
  F32 dst_column_pct;
};

////////////////////////////////
//~ rjf: Modules @view_types

typedef struct DF_ModulesViewState DF_ModulesViewState;
struct DF_ModulesViewState
{
  B32 initialized;
  DF_Handle selected_entity;
  S64 selected_column;
  B32 txt_editing;
  TxtPt txt_cursor;
  TxtPt txt_mark;
  U8 txt_buffer[1024];
  U64 txt_size;
  DF_Handle pick_file_dst_entity;
  F32 idx_col_pct;
  F32 desc_col_pct;
  F32 range_col_pct;
  F32 dbg_col_pct;
};

////////////////////////////////
//~ rjf: Watch, Locals, Registers @view_types

typedef struct DF_EvalRoot DF_EvalRoot;
struct DF_EvalRoot
{
  DF_EvalRoot *next;
  DF_EvalRoot *prev;
  U64 expr_buffer_string_size;
  U64 expr_buffer_cap;
  U8 *expr_buffer;
};

typedef enum DF_EvalWatchViewColumnKind
{
  DF_EvalWatchViewColumnKind_Expr,
  DF_EvalWatchViewColumnKind_Value,
  DF_EvalWatchViewColumnKind_Type,
  DF_EvalWatchViewColumnKind_ViewRule,
  DF_EvalWatchViewColumnKind_COUNT
}
DF_EvalWatchViewColumnKind;

typedef enum DF_EvalWatchViewFillKind
{
  DF_EvalWatchViewFillKind_Mutable,
  DF_EvalWatchViewFillKind_Registers,
  DF_EvalWatchViewFillKind_Locals,
  DF_EvalWatchViewFillKind_Globals,
  DF_EvalWatchViewFillKind_ThreadLocals,
  DF_EvalWatchViewFillKind_Types,
  DF_EvalWatchViewFillKind_COUNT
}
DF_EvalWatchViewFillKind;

typedef struct DF_EvalWatchViewState DF_EvalWatchViewState;
struct DF_EvalWatchViewState
{
  B32 initialized;
  
  // rjf: fill kind (way that the contents of the watch view are computed)
  DF_EvalWatchViewFillKind fill_kind;
  
  // rjf; selection state
  DF_EvalWatchViewColumnKind selected_column;
  DF_ExpandKey selected_parent_key;
  DF_ExpandKey selected_key;
  
  // rjf: text input state
  TxtPt input_cursor;
  TxtPt input_mark;
  U8 input_buffer[1024];
  U64 input_size;
  B32 input_editing;
  
  // rjf: table column width state
  F32 expr_column_pct;
  F32 value_column_pct;
  F32 type_column_pct;
  F32 view_rule_column_pct;
  
  // rjf: mutable fill-kind root expression state
  DF_EvalRoot *first_root;
  DF_EvalRoot *last_root;
  DF_EvalRoot *first_free_root;
  U64 root_count;
};

////////////////////////////////
//~ rjf: Code @view_types

typedef struct DF_CodeViewState DF_CodeViewState;
struct DF_CodeViewState
{
  // rjf: stable state
  B32 initialized;
  TxtPt cursor;
  TxtPt mark;
  S64 preferred_column;
  B32 drifted_for_search;
  DF_Handle pick_file_override_target;
  
  // rjf: per-frame command info
  S64 goto_line_num;
  B32 center_cursor;
  B32 contain_cursor;
  Arena *find_text_arena;
  String8 find_text_fwd;
  String8 find_text_bwd;
};

////////////////////////////////
//~ rjf: Disassembly @view_types

typedef struct DF_DisasmViewState DF_DisasmViewState;
struct DF_DisasmViewState
{
  // rjf: stable state
  B32 initialized;
  DF_Handle process;
  U64 base_vaddr;
  TxtPt cursor;
  TxtPt mark;
  S64 preferred_column;
  B32 drifted_for_search;
  
  // rjf: per-frame command info
  S64 goto_line_num;
  U64 goto_vaddr;
  B32 center_cursor;
  B32 contain_cursor;
  Arena *find_text_arena;
  String8 find_text_fwd;
  String8 find_text_bwd;
};

////////////////////////////////
//~ rjf: Memory @view_types

typedef struct DF_MemoryViewState DF_MemoryViewState;
struct DF_MemoryViewState
{
  B32 initialized;
  
  // rjf: last-viewed-memory cache
  Arena *last_viewed_memory_cache_arena;
  U8 *last_viewed_memory_cache_buffer;
  Rng1U64 last_viewed_memory_cache_range;
  U64 last_viewed_memory_cache_memgen_idx;
  
  // rjf: control state
  U64 cursor;
  U64 mark;
  
  // rjf: organization state
  U64 num_columns;
  U64 bytes_per_cell;
  
  // rjf: command pass-through data
  B32 center_cursor;
  B32 contain_cursor;
};

////////////////////////////////
//~ rjf: Quick Sort Comparisons

internal int df_qsort_compare_file_info__default(DF_FileInfo *a, DF_FileInfo *b);
internal int df_qsort_compare_file_info__default_filtered(DF_FileInfo *a, DF_FileInfo *b);
internal int df_qsort_compare_file_info__filename(DF_FileInfo *a, DF_FileInfo *b);
internal int df_qsort_compare_file_info__last_modified(DF_FileInfo *a, DF_FileInfo *b);
internal int df_qsort_compare_file_info__size(DF_FileInfo *a, DF_FileInfo *b);
internal int df_qsort_compare_process_info(DF_ProcessInfo *a, DF_ProcessInfo *b);
internal int df_qsort_compare_cmd_lister__strength(DF_CmdListerItem *a, DF_CmdListerItem *b);
internal int df_qsort_compare_entity_lister__strength(DF_EntityListerItem *a, DF_EntityListerItem *b);

////////////////////////////////
//~ rjf: Command Lister

internal DF_CmdListerItemList df_cmd_lister_item_list_from_needle(Arena *arena, String8 needle);
internal DF_CmdListerItemArray df_cmd_lister_item_array_from_list(Arena *arena, DF_CmdListerItemList list);
internal void df_cmd_lister_item_array_sort_by_strength__in_place(DF_CmdListerItemArray array);

////////////////////////////////
//~ rjf: System Process Lister

internal DF_ProcessInfoList df_process_info_list_from_query(Arena *arena, String8 query);
internal DF_ProcessInfoArray df_process_info_array_from_list(Arena *arena, DF_ProcessInfoList list);
internal void df_process_info_array_sort_by_strength__in_place(DF_ProcessInfoArray array);

////////////////////////////////
//~ rjf: Entity Lister

internal DF_EntityListerItemList df_entity_lister_item_list_from_needle(Arena *arena, DF_EntityKind kind, DF_EntityFlags omit_flags, String8 needle);
internal DF_EntityListerItemArray df_entity_lister_item_array_from_list(Arena *arena, DF_EntityListerItemList list);
internal void df_entity_lister_item_array_sort_by_strength__in_place(DF_EntityListerItemArray array);

////////////////////////////////
//~ rjf: Disassembly View

internal TXTI_TokenArray df_txti_token_array_from_dasm_arch_string(Arena *arena, Architecture arch, String8 string);

////////////////////////////////
//~ rjf: Eval/Watch Views

//- rjf: eval watch view instance -> eval view key
internal DF_EvalViewKey df_eval_view_key_from_eval_watch_view(DF_EvalWatchViewState *ewv);

//- rjf: root allocation/deallocation/mutation
internal DF_EvalRoot *  df_eval_root_alloc(DF_View *view, DF_EvalWatchViewState *ews);
internal void           df_eval_root_release(DF_EvalWatchViewState *ews, DF_EvalRoot *root);
internal void           df_eval_root_equip_string(DF_EvalRoot *root, String8 string);
internal DF_EvalRoot *  df_eval_root_from_string(DF_EvalWatchViewState *ews, String8 string);
internal DF_EvalRoot *  df_eval_root_from_expand_key(DF_EvalWatchViewState *ews, DF_EvalView *eval_view, DF_ExpandKey expand_key);
internal String8        df_string_from_eval_root(DF_EvalRoot *root);

//- rjf: windowed watch tree visualization
internal DF_EvalVizBlockList df_eval_viz_block_list_from_watch_view_state(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_View *view, DF_EvalWatchViewState *ews);

//- rjf: eval/watch views main hooks
internal void df_eval_watch_view_init(DF_EvalWatchViewState *ewv, DF_View *view, DF_EvalWatchViewFillKind fill_kind);
internal void df_eval_watch_view_cmds(DF_Window *ws, DF_Panel *panel, DF_View *view, DF_EvalWatchViewState *ewv, DF_CmdList *cmds);
internal void df_eval_watch_view_build(DF_Window *ws, DF_Panel *panel, DF_View *view, DF_EvalWatchViewState *ewv, B32 modifiable, U32 default_radix, Rng2F32 rect);

#endif // DEBUG_FRONTEND_VIEWS_H
