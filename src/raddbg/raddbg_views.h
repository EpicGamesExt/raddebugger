// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_VIEWS_H
#define RADDBG_VIEWS_H

////////////////////////////////
//~ rjf: Code View Types

typedef U32 RD_CodeViewBuildFlags;
enum
{
  RD_CodeViewBuildFlag_Margins = (1<<0),
  RD_CodeViewBuildFlag_All     = 0xffffffff,
};

typedef struct RD_CodeViewState RD_CodeViewState;
struct RD_CodeViewState
{
  // rjf: stable state
  B32 initialized;
  S64 preferred_column;
  B32 drifted_for_search;
  
  // rjf: per-frame command info
  S64 goto_line_num;
  B32 center_cursor;
  B32 contain_cursor;
  B32 watch_expr_at_mouse;
  Arena *find_text_arena;
  String8 find_text_fwd;
  String8 find_text_bwd;
};

typedef struct RD_CodeViewBuildResult RD_CodeViewBuildResult;
struct RD_CodeViewBuildResult
{
  DI_KeyList dbgi_keys;
};

////////////////////////////////
//~ rjf: Watch View Types

typedef enum RD_WatchCellKind
{
  RD_WatchCellKind_String, // plain text
  RD_WatchCellKind_Expr,   // strings for forming expression, under some key; `expression` for expression, `view_rule` for view rule
  RD_WatchCellKind_Eval,   // an evaluation of the expression, with some optional modification - e.g. `$expr.some_member`, or `typeof($expr)~
}
RD_WatchCellKind;

typedef struct RD_WatchCell RD_WatchCell;
struct RD_WatchCell
{
  RD_WatchCell *next;
  RD_WatchCellKind kind;
  String8 string;
  F32 pct;
};

typedef struct RD_WatchCellList RD_WatchCellList;
struct RD_WatchCellList
{
  RD_WatchCell *first;
  RD_WatchCell *last;
  U64 count;
};

typedef struct RD_WatchRowInfo RD_WatchRowInfo;
struct RD_WatchRowInfo
{
  E_Eval eval;
  CTRL_Entity *module;
  String8 group_key;
  RD_Cfg *group_cfg;
  CTRL_Entity *group_entity;
  CTRL_Entity *callstack_thread;
  U64 callstack_unwind_index;
  U64 callstack_inline_depth;
  RD_WatchCellList cells;
};

typedef struct RD_WatchRowCellInfo RD_WatchRowCellInfo;
struct RD_WatchRowCellInfo
{
  E_Eval eval;
  String8 string;
  B32 can_edit;
  B32 is_errored;
  String8 error_tooltip;
  String8 inheritance_tooltip;
  RD_ViewRuleUIFunctionType *ui;
  MD_Node *ui_params;
};

typedef enum RD_WatchViewColumnKind
{
  RD_WatchViewColumnKind_Expr,
  RD_WatchViewColumnKind_Value,
  RD_WatchViewColumnKind_Type,
  RD_WatchViewColumnKind_ViewRule,
  RD_WatchViewColumnKind_Member,
  RD_WatchViewColumnKind_CallStackFrame,
  RD_WatchViewColumnKind_CallStackFrameSelection,
  RD_WatchViewColumnKind_Module,
  RD_WatchViewColumnKind_COUNT
}
RD_WatchViewColumnKind;

typedef struct RD_WatchViewColumnParams RD_WatchViewColumnParams;
struct RD_WatchViewColumnParams
{
  String8 string;
  String8 display_string;
  String8 view_rule;
  B32 is_non_code;
  B32 dequote_string;
  B32 rangify_braces;
};

typedef struct RD_WatchViewColumn RD_WatchViewColumn;
struct RD_WatchViewColumn
{
  RD_WatchViewColumn *next;
  RD_WatchViewColumn *prev;
  RD_WatchViewColumnKind kind;
  F32 pct;
  U8 string_buffer[1024];
  U64 string_size;
  U8 display_string_buffer[1024];
  U64 display_string_size;
  U8 view_rule_buffer[1024];
  U64 view_rule_size;
  B32 is_non_code;
  B32 dequote_string;
  B32 rangify_braces;
};

typedef struct RD_WatchViewRowCtrl RD_WatchViewRowCtrl;
struct RD_WatchViewRowCtrl
{
  RD_EntityKind entity_kind;
  CTRL_EntityKind ctrl_entity_kind;
  RD_CmdKind kind;
};

typedef enum RD_WatchViewRowKind
{
  RD_WatchViewRowKind_Normal,
  RD_WatchViewRowKind_Header,
  RD_WatchViewRowKind_Canvas,
  RD_WatchViewRowKind_PrettyEntityControls,
}
RD_WatchViewRowKind;

typedef struct RD_WatchPt RD_WatchPt;
struct RD_WatchPt
{
  EV_Key parent_key;
  EV_Key key;
  U64 cell_id;
};

typedef struct RD_WatchViewRowInfo RD_WatchViewRowInfo;
struct RD_WatchViewRowInfo
{
  RD_EntityKind collection_entity_kind;
  RD_Entity *collection_entity;
  CTRL_EntityKind collection_ctrl_entity_kind;
  CTRL_Entity *collection_ctrl_entity;
  CTRL_Entity *callstack_thread;
  U64 callstack_unwind_index;
  U64 callstack_inline_depth;
};

typedef struct RD_WatchViewTextEditState RD_WatchViewTextEditState;
struct RD_WatchViewTextEditState
{
  RD_WatchViewTextEditState *pt_hash_next;
  RD_WatchPt pt;
  TxtPt cursor;
  TxtPt mark;
  U8 input_buffer[1024];
  U64 input_size;
  U8 initial_buffer[1024];
  U64 initial_size;
};

typedef struct RD_WatchViewState RD_WatchViewState;
struct RD_WatchViewState
{
  B32 initialized;
  
  // rjf: column state
  Arena *column_arena;
  RD_WatchViewColumn *first_column;
  RD_WatchViewColumn *last_column;
  RD_WatchViewColumn *free_column;
  U64 column_count;
  
  // rjf; table cursor state
  RD_WatchPt cursor;
  RD_WatchPt mark;
  RD_WatchPt next_cursor;
  RD_WatchPt next_mark;
  
  // rjf: text input state
  Arena *text_edit_arena;
  U64 text_edit_state_slots_count;
  RD_WatchViewTextEditState dummy_text_edit_state;
  RD_WatchViewTextEditState **text_edit_state_slots;
  B32 text_editing;
};

////////////////////////////////
//~ rjf: Code View Functions

internal void rd_code_view_init(RD_CodeViewState *cv);
internal RD_CodeViewBuildResult rd_code_view_build(Arena *arena, RD_CodeViewState *cv, RD_CodeViewBuildFlags flags, Rng2F32 rect, String8 text_data, TXT_TextInfo *text_info, DASM_LineArray *dasm_lines, Rng1U64 dasm_vaddr_range, DI_Key dasm_dbgi_key);

////////////////////////////////
//~ rjf: Watch View Functions

//- rjf: cell list building
internal U64 rd_id_from_watch_cell(RD_WatchCell *cell);
internal RD_WatchCell *rd_watch_cell_list_push(Arena *arena, RD_WatchCellList *list);
internal RD_WatchCell *rd_watch_cell_list_push_new_(Arena *arena, RD_WatchCellList *list, RD_WatchCell *params);
#define rd_watch_cell_list_push_new(arena, list, kind_, ...) rd_watch_cell_list_push_new_((arena), (list), &(RD_WatchCell){.kind = (kind_), __VA_ARGS__})

//- rjf: index -> column
internal RD_WatchViewColumn *rd_watch_view_column_from_x(RD_WatchViewState *wv, S64 index);

//- rjf: watch view points <-> table coordinates
internal B32 rd_watch_pt_match(RD_WatchPt a, RD_WatchPt b);
internal RD_WatchPt rd_watch_pt_from_tbl(EV_BlockRangeList *block_ranges, Vec2S64 tbl);
internal Vec2S64 rd_tbl_from_watch_pt(EV_BlockRangeList *block_ranges, RD_WatchPt pt);

//- rjf: row -> info
internal RD_WatchRowInfo rd_watch_row_info_from_row(Arena *arena, EV_Row *row);

//- rjf: row -> context info
internal RD_WatchViewRowInfo rd_watch_view_row_info_from_row(EV_Row *row);

//- rjf: row * cell -> info
internal RD_WatchRowCellInfo rd_info_from_watch_row_cell(Arena *arena, EV_Row *row, RD_WatchRowInfo *row_info, RD_WatchCell *cell, FNT_Tag font, F32 font_size, F32 max_size_px);

//- rjf: row/column -> exprs / strings
internal E_Expr *rd_expr_from_watch_view_row_column(Arena *arena, EV_Row *row, RD_WatchViewColumn *col);
internal String8 rd_string_from_eval_viz_row_column(Arena *arena, EV_Row *row, RD_WatchViewColumn *col, EV_StringFlags string_flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size_px);

//- rjf: table coordinates -> text edit state
internal RD_WatchViewTextEditState *rd_watch_view_text_edit_state_from_pt(RD_WatchViewState *wv, RD_WatchPt pt);

//- rjf: watch view column state mutation
internal RD_WatchViewColumn *rd_watch_view_column_alloc_(RD_WatchViewState *wv, RD_WatchViewColumnKind kind, F32 pct, RD_WatchViewColumnParams *params);
#define rd_watch_view_column_alloc(wv, kind, pct, ...) rd_watch_view_column_alloc_((wv), (kind), (pct), &(RD_WatchViewColumnParams){.string = str8_zero(), __VA_ARGS__})
internal void rd_watch_view_column_release(RD_WatchViewState *wv, RD_WatchViewColumn *col);

//- rjf: watch view main hooks
internal void rd_watch_view_init(RD_WatchViewState *ewv);
internal void rd_watch_view_build(RD_WatchViewState *ewv, String8 root_expr, String8 root_view_rule, B32 modifiable, U32 default_radix, Rng2F32 rect);

#endif // RADDBG_VIEWS_H
