// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_FRONTEND_VIEWS_H
#define DBG_FRONTEND_VIEWS_H

////////////////////////////////
//~ rjf: Code View Types

typedef U32 DF_CodeViewFlags;
enum
{
  DF_CodeViewFlag_StickToBottom = (1<<0),
};

typedef U32 DF_CodeViewBuildFlags;
enum
{
  DF_CodeViewBuildFlag_Margins = (1<<0),
  DF_CodeViewBuildFlag_All     = 0xffffffff,
};

typedef struct DF_CodeViewState DF_CodeViewState;
struct DF_CodeViewState
{
  // rjf: stable state
  B32 initialized;
  S64 preferred_column;
  B32 drifted_for_search;
  DF_CodeViewFlags flags;
  
  // rjf: per-frame command info
  S64 goto_line_num;
  B32 center_cursor;
  B32 contain_cursor;
  B32 watch_expr_at_mouse;
  Arena *find_text_arena;
  String8 find_text_fwd;
  String8 find_text_bwd;
};

typedef struct DF_CodeViewBuildResult DF_CodeViewBuildResult;
struct DF_CodeViewBuildResult
{
  DI_KeyList dbgi_keys;
};

////////////////////////////////
//~ rjf: Watch View Types

typedef enum DF_WatchViewColumnKind
{
  DF_WatchViewColumnKind_Expr,
  DF_WatchViewColumnKind_Value,
  DF_WatchViewColumnKind_Type,
  DF_WatchViewColumnKind_ViewRule,
  DF_WatchViewColumnKind_Module,
  DF_WatchViewColumnKind_FrameSelection,
  DF_WatchViewColumnKind_Member,
  DF_WatchViewColumnKind_COUNT
}
DF_WatchViewColumnKind;

typedef struct DF_WatchViewColumnParams DF_WatchViewColumnParams;
struct DF_WatchViewColumnParams
{
  String8 string;
  String8 display_string;
  String8 view_rule;
  B32 is_non_code;
  B32 dequote_string;
};

typedef struct DF_WatchViewColumn DF_WatchViewColumn;
struct DF_WatchViewColumn
{
  DF_WatchViewColumn *next;
  DF_WatchViewColumn *prev;
  DF_WatchViewColumnKind kind;
  F32 pct;
  U8 string_buffer[1024];
  U64 string_size;
  U8 display_string_buffer[1024];
  U64 display_string_size;
  U8 view_rule_buffer[1024];
  U64 view_rule_size;
  B32 is_non_code;
  B32 dequote_string;
};

typedef enum DF_WatchViewFillKind
{
  DF_WatchViewFillKind_Watch,
  DF_WatchViewFillKind_Breakpoints,
  DF_WatchViewFillKind_WatchPins,
  DF_WatchViewFillKind_CallStack,
  DF_WatchViewFillKind_Registers,
  DF_WatchViewFillKind_Locals,
  DF_WatchViewFillKind_Globals,
  DF_WatchViewFillKind_ThreadLocals,
  DF_WatchViewFillKind_Types,
  DF_WatchViewFillKind_Procedures,
  DF_WatchViewFillKind_COUNT
}
DF_WatchViewFillKind;

typedef struct DF_WatchViewPoint DF_WatchViewPoint;
struct DF_WatchViewPoint
{
  S64 x;
  EV_Key parent_key;
  EV_Key key;
};

typedef struct DF_WatchViewTextEditState DF_WatchViewTextEditState;
struct DF_WatchViewTextEditState
{
  DF_WatchViewTextEditState *pt_hash_next;
  DF_WatchViewPoint pt;
  TxtPt cursor;
  TxtPt mark;
  U8 input_buffer[1024];
  U64 input_size;
  U8 initial_buffer[1024];
  U64 initial_size;
};

typedef struct DF_WatchViewState DF_WatchViewState;
struct DF_WatchViewState
{
  B32 initialized;
  
  // rjf: fill kinds (way that the contents of the watch view are computed)
  DF_WatchViewFillKind fill_kind;
  
  // rjf: column state
  Arena *column_arena;
  DF_WatchViewColumn *first_column;
  DF_WatchViewColumn *last_column;
  DF_WatchViewColumn *free_column;
  U64 column_count;
  
  // rjf; table cursor state
  DF_WatchViewPoint cursor;
  DF_WatchViewPoint mark;
  DF_WatchViewPoint next_cursor;
  DF_WatchViewPoint next_mark;
  
  // rjf: text input state
  Arena *text_edit_arena;
  U64 text_edit_state_slots_count;
  DF_WatchViewTextEditState dummy_text_edit_state;
  DF_WatchViewTextEditState **text_edit_state_slots;
  B32 text_editing;
};

////////////////////////////////
//~ rjf: Code View Functions

internal void df_code_view_init(DF_CodeViewState *cv, DF_View *view);
internal void df_code_view_cmds(DF_View *view, DF_CodeViewState *cv, String8 text_data, TXT_TextInfo *text_info, DASM_LineArray *dasm_lines, Rng1U64 dasm_vaddr_range, DI_Key dasm_dbgi_key);
internal DF_CodeViewBuildResult df_code_view_build(Arena *arena, DF_View *view, DF_CodeViewState *cv, DF_CodeViewBuildFlags flags, Rng2F32 rect, String8 text_data, TXT_TextInfo *text_info, DASM_LineArray *dasm_lines, Rng1U64 dasm_vaddr_range, DI_Key dasm_dbgi_key);

////////////////////////////////
//~ rjf: Watch View Functions

//- rjf: index -> column
internal DF_WatchViewColumn *df_watch_view_column_from_x(DF_WatchViewState *wv, S64 index);

//- rjf: watch view points <-> table coordinates
internal B32 df_watch_view_point_match(DF_WatchViewPoint a, DF_WatchViewPoint b);
internal DF_WatchViewPoint df_watch_view_point_from_tbl(EV_BlockList *blocks, Vec2S64 tbl);
internal Vec2S64 df_tbl_from_watch_view_point(EV_BlockList *blocks, DF_WatchViewPoint pt);

//- rjf: table coordinates -> strings
internal String8 df_string_from_eval_viz_row_column(Arena *arena, EV_View *ev, EV_Row *row, DF_WatchViewColumn *col, B32 editable, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size_px);

//- rjf: table coordinates -> text edit state
internal DF_WatchViewTextEditState *df_watch_view_text_edit_state_from_pt(DF_WatchViewState *wv, DF_WatchViewPoint pt);

//- rjf: watch view column state mutation
internal DF_WatchViewColumn *df_watch_view_column_alloc_(DF_WatchViewState *wv, DF_WatchViewColumnKind kind, F32 pct, DF_WatchViewColumnParams *params);
#define df_watch_view_column_alloc(wv, kind, pct, ...) df_watch_view_column_alloc_((wv), (kind), (pct), &(DF_WatchViewColumnParams){.string = str8_zero(), __VA_ARGS__})
internal void df_watch_view_column_release(DF_WatchViewState *wv, DF_WatchViewColumn *col);

//- rjf: watch view main hooks
internal void df_watch_view_init(DF_WatchViewState *ewv, DF_View *view, DF_WatchViewFillKind fill_kind);
internal void df_watch_view_build(DF_View *view, DF_WatchViewState *ewv, B32 modifiable, U32 default_radix, Rng2F32 rect);

#endif // DBG_FRONTEND_VIEWS_H
