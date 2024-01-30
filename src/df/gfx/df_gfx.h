// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DF_GFX_H
#define DF_GFX_H

////////////////////////////////
//~ rjf: Basic Types

typedef struct DF_PathQuery DF_PathQuery;
struct DF_PathQuery
{
  String8 prefix;
  String8 path;
  String8 search;
};

////////////////////////////////
//~ rjf: Binding Types

typedef struct DF_Binding DF_Binding;
struct DF_Binding
{
  OS_Key key;
  OS_EventFlags flags;
};

typedef struct DF_BindingNode DF_BindingNode;
struct DF_BindingNode
{
  DF_BindingNode *next;
  DF_Binding binding;
};

typedef struct DF_BindingList DF_BindingList;
struct DF_BindingList
{
  DF_BindingNode *first;
  DF_BindingNode *last;
  U64 count;
};

typedef struct DF_StringBindingPair DF_StringBindingPair;
struct DF_StringBindingPair
{
  String8 string;
  DF_Binding binding;
};

////////////////////////////////
//~ rjf: Text Searching Types

typedef struct DF_TextSearchMatch DF_TextSearchMatch;
struct DF_TextSearchMatch
{
  TxtPt pt;
};

typedef struct DF_TextSearchMatchChunkNode DF_TextSearchMatchChunkNode;
struct DF_TextSearchMatchChunkNode
{
  DF_TextSearchMatchChunkNode *next;
  DF_TextSearchMatch *v;
  U64 count;
  U64 cap;
};

typedef struct DF_TextSearchMatchChunkList DF_TextSearchMatchChunkList;
struct DF_TextSearchMatchChunkList
{
  DF_TextSearchMatchChunkNode *first;
  DF_TextSearchMatchChunkNode *last;
  U64 node_count;
  U64 total_count;
};

typedef struct DF_TextSearchMatchArray DF_TextSearchMatchArray;
struct DF_TextSearchMatchArray
{
  DF_TextSearchMatch *v;
  U64 count;
};

typedef struct DF_TextSearchCacheNode DF_TextSearchCacheNode;
struct DF_TextSearchCacheNode
{
  // rjf: links
  DF_TextSearchCacheNode *next;
  DF_TextSearchCacheNode *prev;
  
  // rjf: allocation
  Arena *arena;
  
  // rjf: search parameters
  U128 hash;
  String8 needle;
  DF_TextSliceFlags flags;
  TxtPt start_pt;
  
  // rjf: search results
  B32 good;
  DF_TextSearchMatchChunkList search_matches;
  
  // rjf: last time touched
  U64 last_time_touched_us;
};

typedef struct DF_TextSearchCacheSlot DF_TextSearchCacheSlot;
struct DF_TextSearchCacheSlot
{
  DF_TextSearchCacheNode *first;
  DF_TextSearchCacheNode *last;
};

////////////////////////////////
//~ rjf: Key Map Types

typedef struct DF_KeyMapNode DF_KeyMapNode;
struct DF_KeyMapNode
{
  DF_KeyMapNode *hash_next;
  DF_KeyMapNode *hash_prev;
  DF_CmdSpec *spec;
  DF_Binding binding;
};

typedef struct DF_KeyMapSlot DF_KeyMapSlot;
struct DF_KeyMapSlot
{
  DF_KeyMapNode *first;
  DF_KeyMapNode *last;
};

////////////////////////////////
//~ rjf: View Functions

struct DF_View;
struct DF_Panel;
struct DF_Window;

#define DF_VIEW_SETUP_FUNCTION_SIG(name) void name(struct DF_View *view, DF_CfgNode *cfg_root)
#define DF_VIEW_SETUP_FUNCTION_NAME(name) df_view_setup_##name
#define DF_VIEW_SETUP_FUNCTION_DEF(name) internal DF_VIEW_SETUP_FUNCTION_SIG(DF_VIEW_SETUP_FUNCTION_NAME(name))
typedef DF_VIEW_SETUP_FUNCTION_SIG(DF_ViewSetupFunctionType);

#define DF_VIEW_STRING_FROM_STATE_FUNCTION_SIG(name) String8 name(Arena *arena, struct DF_View *view)
#define DF_VIEW_STRING_FROM_STATE_FUNCTION_NAME(name) df_view_string_from_state_##name
#define DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(name) internal DF_VIEW_STRING_FROM_STATE_FUNCTION_SIG(DF_VIEW_STRING_FROM_STATE_FUNCTION_NAME(name))
typedef DF_VIEW_STRING_FROM_STATE_FUNCTION_SIG(DF_ViewStringFromStateFunctionType);

#define DF_VIEW_CMD_FUNCTION_SIG(name) void name(struct DF_Window *ws, struct DF_Panel *panel, struct DF_View *view, struct DF_CmdList *cmds)
#define DF_VIEW_CMD_FUNCTION_NAME(name) df_view_cmds_##name
#define DF_VIEW_CMD_FUNCTION_DEF(name) internal DF_VIEW_CMD_FUNCTION_SIG(DF_VIEW_CMD_FUNCTION_NAME(name))
typedef DF_VIEW_CMD_FUNCTION_SIG(DF_ViewCmdFunctionType);

#define DF_VIEW_UI_FUNCTION_SIG(name) void name(struct DF_Window *ws, struct DF_Panel *panel, struct DF_View *view, Rng2F32 rect)
#define DF_VIEW_UI_FUNCTION_NAME(name) df_view_ui_##name
#define DF_VIEW_UI_FUNCTION_DEF(name) internal DF_VIEW_UI_FUNCTION_SIG(DF_VIEW_UI_FUNCTION_NAME(name))
typedef DF_VIEW_UI_FUNCTION_SIG(DF_ViewUIFunctionType);

////////////////////////////////
//~ rjf: View Specification Types

typedef U32 DF_ViewSpecFlags;
enum
{
  DF_ViewSpecFlag_ParameterizedByEntity   = (1<<0),
  DF_ViewSpecFlag_CanSerialize            = (1<<1),
  DF_ViewSpecFlag_CanSerializeEntityPath  = (1<<2),
};

typedef struct DF_ViewSpecInfo DF_ViewSpecInfo;
struct DF_ViewSpecInfo
{
  DF_ViewSpecFlags flags;
  String8 name;
  String8 display_string;
  DF_NameKind name_kind;
  DF_IconKind icon_kind;
  DF_ViewSetupFunctionType *setup_hook;
  DF_ViewStringFromStateFunctionType *string_from_state_hook;
  DF_ViewCmdFunctionType *cmd_hook;
  DF_ViewUIFunctionType *ui_hook;
};

typedef struct DF_ViewSpec DF_ViewSpec;
struct DF_ViewSpec
{
  DF_ViewSpec *hash_next;
  DF_ViewSpecInfo info;
};

typedef struct DF_ViewSpecInfoArray DF_ViewSpecInfoArray;
struct DF_ViewSpecInfoArray
{
  DF_ViewSpecInfo *v;
  U64 count;
};

typedef struct DF_CmdParamSlotViewSpecRuleNode DF_CmdParamSlotViewSpecRuleNode;
struct DF_CmdParamSlotViewSpecRuleNode
{
  DF_CmdParamSlotViewSpecRuleNode *next;
  DF_ViewSpec *view_spec;
  DF_CmdSpec *cmd_spec;
};

typedef struct DF_CmdParamSlotViewSpecRuleList DF_CmdParamSlotViewSpecRuleList;
struct DF_CmdParamSlotViewSpecRuleList
{
  DF_CmdParamSlotViewSpecRuleNode *first;
  DF_CmdParamSlotViewSpecRuleNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: View Types

typedef struct DF_ArenaExt DF_ArenaExt;
struct DF_ArenaExt
{
  DF_ArenaExt *next;
  Arena *arena;
};

typedef struct DF_View DF_View;
struct DF_View
{
  // rjf: ownership links ('owners' can have lists of views)
  DF_View *next;
  DF_View *prev;
  
  // rjf: allocation info
  U64 generation;
  
  // rjf: loading animation state
  F32 loading_t;
  F32 loading_t_target;
  U64 loading_progress_v;
  U64 loading_progress_v_target;
  
  // rjf: update flash animation state
  F32 flash_t;
  
  // rjf: view state
  UI_ScrollPt2 scroll_pos;
  
  // rjf: ctrl context overrides
  DF_CtrlCtx ctrl_ctx_overrides;
  
  // rjf: allocation & user data extensions
  Arena *arena;
  DF_ArenaExt *first_arena_ext;
  DF_ArenaExt *last_arena_ext;
  void *user_data;
  
  // rjf: view kind info
  DF_ViewSpec *spec;
  DF_Handle entity;
  
  // rjf: query -> params data
  TxtPt query_cursor;
  TxtPt query_mark;
  U8 query_buffer[1024];
  U64 query_string_size;
};

////////////////////////////////
//~ rjf: Panel Types

typedef struct DF_Panel DF_Panel;
struct DF_Panel
{
  // rjf: tree links/data
  DF_Panel *first;
  DF_Panel *last;
  DF_Panel *next;
  DF_Panel *prev;
  DF_Panel *parent;
  U64 child_count;
  
  // rjf: allocation data
  U64 generation;
  
  // rjf: split data
  Axis2 split_axis;
  Vec2F32 off_pct_of_parent;
  Vec2F32 off_pct_of_parent_target;
  Vec2F32 size_pct_of_parent;
  Vec2F32 size_pct_of_parent_target;
  
  // rjf: tab params
  Side tab_side;
  
  // rjf: stable view stacks (tabs)
  DF_View *first_tab_view;
  DF_View *last_tab_view;
  U64 tab_view_count;
  DF_Handle selected_tab_view;
};

typedef struct DF_PanelRec DF_PanelRec;
struct DF_PanelRec
{
  DF_Panel *next;
  int push_count;
  int pop_count;
};

////////////////////////////////
//~ rjf: Drag/Drop Types

typedef enum DF_DragDropState
{
  DF_DragDropState_Null,
  DF_DragDropState_Dragging,
  DF_DragDropState_Dropping,
  DF_DragDropState_COUNT
}
DF_DragDropState;

typedef struct DF_DragDropPayload DF_DragDropPayload;
struct DF_DragDropPayload
{
  UI_Key key;
  DF_Handle panel;
  DF_Handle view;
  DF_Handle entity;
  TxtPt text_point;
};

////////////////////////////////
//~ rjf: View Rule Spec Types

typedef U32 DF_GfxViewRuleSpecInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  DF_GfxViewRuleSpecInfoFlag_VizRowProd     = (1<<0),
  DF_GfxViewRuleSpecInfoFlag_LineStringize  = (1<<1),
  DF_GfxViewRuleSpecInfoFlag_RowUI          = (1<<2),
  DF_GfxViewRuleSpecInfoFlag_BlockUI        = (1<<3),
};

#define DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(name) void name(void)
#define DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_NAME(name) df_gfx_view_rule_viz_row_prod__##name
#define DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(name) internal DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_NAME(name))

#define DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(name) void name(void)
#define DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_NAME(name) df_gfx_view_rule_line_stringize__##name
#define DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(name) internal DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_NAME(name))

#define DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_SIG(name) void name(DF_ExpandKey key, DF_Eval eval, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, struct DF_CfgNode *cfg)
#define DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_NAME(name) df_gfx_view_rule_row_ui__##name
#define DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(name) DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_SIG(DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_NAME(name))

#define DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_SIG(name) void name(struct DF_Window *ws, DF_ExpandKey key, DF_Eval eval, DBGI_Scope *dbgi_scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, struct DF_CfgNode *cfg, Vec2F32 dim)
#define DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_NAME(name) df_gfx_view_rule_block_ui__##name
#define DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(name) DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_SIG(DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_NAME(name))

typedef DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(DF_GfxViewRuleVizRowProdHookFunctionType);
typedef DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(DF_GfxViewRuleLineStringizeHookFunctionType);
typedef DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_SIG(DF_GfxViewRuleRowUIFunctionType);
typedef DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_SIG(DF_GfxViewRuleBlockUIFunctionType);

typedef struct DF_GfxViewRuleSpecInfo DF_GfxViewRuleSpecInfo;
struct DF_GfxViewRuleSpecInfo
{
  String8 string;
  DF_GfxViewRuleSpecInfoFlags flags;
  DF_GfxViewRuleVizRowProdHookFunctionType *viz_row_prod;
  DF_GfxViewRuleLineStringizeHookFunctionType *line_stringize;
  DF_GfxViewRuleRowUIFunctionType *row_ui;
  DF_GfxViewRuleBlockUIFunctionType *block_ui;
};

typedef struct DF_GfxViewRuleSpecInfoArray DF_GfxViewRuleSpecInfoArray;
struct DF_GfxViewRuleSpecInfoArray
{
  DF_GfxViewRuleSpecInfo *v;
  U64 count;
};

typedef struct DF_GfxViewRuleSpec DF_GfxViewRuleSpec;
struct DF_GfxViewRuleSpec
{
  DF_GfxViewRuleSpec *hash_next;
  DF_GfxViewRuleSpecInfo info;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/df_gfx.meta.h"

////////////////////////////////
//~ rjf: Theme Types

typedef struct DF_Theme DF_Theme;
struct DF_Theme
{
  Vec4F32 colors[DF_ThemeColor_COUNT];
};

typedef enum DF_FontSlot
{
  DF_FontSlot_Main,
  DF_FontSlot_Code,
  DF_FontSlot_Icons,
  DF_FontSlot_COUNT
}
DF_FontSlot;

////////////////////////////////
//~ rjf: UI Helper & Widget Types

//- rjf: line edits

typedef U32 DF_LineEditFlags;
enum
{
  DF_LineEditFlag_Expander            = (1<<0),
  DF_LineEditFlag_ExpanderSpace       = (1<<1),
  DF_LineEditFlag_ExpanderPlaceholder = (1<<2),
  DF_LineEditFlag_DisableEdit         = (1<<3),
  DF_LineEditFlag_CodeContents        = (1<<4),
  DF_LineEditFlag_Border              = (1<<5),
  DF_LineEditFlag_NoBackground        = (1<<6),
  DF_LineEditFlag_PreferDisplayString = (1<<7),
  DF_LineEditFlag_DisplayStringIsCode = (1<<8),
};

//- rjf: code viewing/editing widgets

typedef U32 DF_CodeSliceFlags;
enum
{
  DF_CodeSliceFlag_Margin   = (1<<0),
  DF_CodeSliceFlag_LineNums = (1<<1),
};

typedef struct DF_CodeSliceParams DF_CodeSliceParams;
struct DF_CodeSliceParams
{
  // rjf: content
  DF_CodeSliceFlags flags;
  Rng1S64 line_num_range;
  String8 *line_text;
  Rng1U64 *line_ranges;
  TXTI_TokenArray *line_tokens;
  DF_EntityList *line_bps;
  DF_EntityList *line_ips;
  DF_EntityList *line_pins;
  DF_TextLineDasm2SrcInfoList *line_dasm2src;
  DF_TextLineSrc2DasmInfoList *line_src2dasm;
  DF_EntityList relevant_binaries;
  
  // rjf: visual parameters
  F_Tag font;
  F32 font_size;
  String8 search_query;
  F32 line_height_px;
  F32 margin_width_px;
  F32 line_num_width_px;
  F32 line_text_max_width_px;
  DF_EntityList flash_ranges;
};

typedef struct DF_CodeSliceSignal DF_CodeSliceSignal;
struct DF_CodeSliceSignal
{
  UI_Signal base;
  TxtPt mouse_pt;
  TxtRng mouse_expr_rng;
  Vec2F32 mouse_expr_baseline_pos;
  S64 clicked_margin_line_num;
  DF_Entity *dropped_entity;
  S64 dropped_entity_line_num;
  TxtRng copy_range;
  B32 toggle_cursor_watch;
  S64 set_next_statement_line_num;
  S64 run_to_line_num;
  S64 goto_disasm_line_num;
  S64 goto_src_line_num;
};

////////////////////////////////
//~ rjf: Auto-Complete Lister Types

typedef U32 DF_AutoCompListerFlags;
enum
{
  DF_AutoCompListerFlag_Locals    = (1<<0),
  DF_AutoCompListerFlag_Registers = (1<<1),
  DF_AutoCompListerFlag_ViewRules = (1<<2),
};

typedef struct DF_AutoCompListerItem DF_AutoCompListerItem;
struct DF_AutoCompListerItem
{
  String8 string;
  String8 kind_string;
  FuzzyMatchRangeList matches;
};

typedef struct DF_AutoCompListerItemChunkNode DF_AutoCompListerItemChunkNode;
struct DF_AutoCompListerItemChunkNode
{
  DF_AutoCompListerItemChunkNode *next;
  DF_AutoCompListerItem *v;
  U64 count;
  U64 cap;
};

typedef struct DF_AutoCompListerItemChunkList DF_AutoCompListerItemChunkList;
struct DF_AutoCompListerItemChunkList
{
  DF_AutoCompListerItemChunkNode *first;
  DF_AutoCompListerItemChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct DF_AutoCompListerItemArray DF_AutoCompListerItemArray;
struct DF_AutoCompListerItemArray
{
  DF_AutoCompListerItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Per-Window State

typedef struct DF_Window DF_Window;
struct DF_Window
{
  // rjf: links & metadata
  DF_Window *next;
  DF_Window *prev;
  U64 gen;
  U64 frames_alive;
  DF_CfgSrc cfg_src;
  
  // rjf: top-level info & handles
  Arena *arena;
  OS_Handle os;
  R_Handle r;
  UI_State *ui;
  F32 code_font_size_delta;
  F32 main_font_size_delta;
  
  // rjf: view state delta history
  DF_StateDeltaHistory *view_state_hist;
  
  // rjf: context menu info
  B32 dev_menu_is_open;
  B32 menu_bar_focused;
  B32 menu_bar_focused_on_press;
  B32 menu_bar_key_held;
  B32 menu_bar_focus_press_started;
  UI_Key drop_completion_ctx_menu_key;
  DF_Handle drop_completion_entity;
  DF_Handle drop_completion_panel;
  UI_Key entity_ctx_menu_key;
  DF_Handle entity_ctx_menu_entity;
  U8 entity_ctx_menu_input_buffer[1024];
  U64 entity_ctx_menu_input_size;
  TxtPt entity_ctx_menu_input_cursor;
  TxtPt entity_ctx_menu_input_mark;
  UI_Key tab_ctx_menu_key;
  DF_Handle tab_ctx_menu_view;
  
  // rjf: autocomplete lister state
  U64 autocomp_last_frame_idx;
  B32 autocomp_force_closed;
  UI_Key autocomp_root_key;
  DF_CtrlCtx autocomp_ctrl_ctx;
  DF_AutoCompListerFlags autocomp_lister_flags;
  U8 autocomp_lister_query_buffer[1024];
  U64 autocomp_lister_query_size;
  F32 autocomp_open_t;
  F32 autocomp_num_visible_rows_t;
  S64 autocomp_cursor_num;
  
  // rjf: query view stack
  Arena *query_cmd_arena;
  DF_CmdSpec *query_cmd_spec;
  DF_CmdParams query_cmd_params;
  DF_View *query_view_stack_top;
  B32 query_view_selected;
  F32 query_view_selected_t;
  F32 query_view_t;
  
  // rjf: hover eval stable state
  TxtPt hover_eval_txt_cursor;
  TxtPt hover_eval_txt_mark;
  U8 hover_eval_txt_buffer[1024];
  U64 hover_eval_txt_size;
  Arena *hover_eval_arena;
  Vec2F32 hover_eval_spawn_pos;
  String8 hover_eval_string;
  
  // rjf: hover eval timer
  U64 hover_eval_first_frame_idx;
  U64 hover_eval_last_frame_idx;
  
  // rjf: hover eval params
  DF_CtrlCtx hover_eval_ctrl_ctx;
  DF_Handle hover_eval_file;
  TxtPt hover_eval_file_pt;
  U64 hover_eval_vaddr;
  F32 hover_eval_open_t;
  F32 hover_eval_num_visible_rows_t;
  
  // rjf: error state
  U8 error_buffer[512];
  U64 error_string_size;
  F32 error_t;
  
  // rjf: context overrides
  DF_CtrlCtx ctrl_ctx_overrides;
  
  // rjf: panel state
  DF_Panel *root_panel;
  DF_Panel *free_panel;
  DF_Panel *focused_panel;
  
  // rjf: per-frame drawing state
  D_Bucket *draw_bucket;
};

////////////////////////////////
//~ rjf: View Rule Block State Types

typedef struct DF_ViewRuleBlockArenaExt DF_ViewRuleBlockArenaExt;
struct DF_ViewRuleBlockArenaExt
{
  DF_ViewRuleBlockArenaExt *next;
  Arena *arena;
};

typedef struct DF_ViewRuleBlockNode DF_ViewRuleBlockNode;
struct DF_ViewRuleBlockNode
{
  DF_ViewRuleBlockNode *next;
  DF_ExpandKey key;
  DF_ViewRuleBlockArenaExt *first_arena_ext;
  DF_ViewRuleBlockArenaExt *last_arena_ext;
  Arena *user_state_arena;
  void *user_state;
  U64 user_state_size;
};

typedef struct DF_ViewRuleBlockSlot DF_ViewRuleBlockSlot;
struct DF_ViewRuleBlockSlot
{
  DF_ViewRuleBlockNode *first;
  DF_ViewRuleBlockNode *last;
};

////////////////////////////////
//~ rjf: Main Per-Process Graphical State

typedef struct DF_String2ViewNode DF_String2ViewNode;
struct DF_String2ViewNode
{
  DF_String2ViewNode *hash_next;
  String8 string;
  String8 view_name;
};

typedef struct DF_String2ViewSlot DF_String2ViewSlot;
struct DF_String2ViewSlot
{
  DF_String2ViewNode *first;
  DF_String2ViewNode *last;
};

typedef struct DF_GfxState DF_GfxState;
struct DF_GfxState
{
  // rjf: arenas
  Arena *arena;
  
  // rjf: frame request state
  U64 num_frames_requested;
  
  // rjf: history cache
  DF_StateDeltaHistory *hist;
  
  // rjf: key map table
  Arena *key_map_arena;
  U64 key_map_table_size;
  DF_KeyMapSlot *key_map_table;
  DF_KeyMapNode *free_key_map_node;
  U64 key_map_total_count;
  
  // rjf: bind change
  B32 bind_change_active;
  DF_CmdSpec *bind_change_cmd_spec;
  DF_Binding bind_change_binding;
  
  // rjf: confirmation popup state
  UI_Key confirm_key;
  B32 confirm_active;
  F32 confirm_t;
  Arena *confirm_arena;
  DF_CmdList confirm_cmds;
  String8 confirm_title;
  String8 confirm_msg;
  
  // rjf: string search state
  Arena *string_search_arena;
  String8 string_search_string;
  
  // rjf: view specs
  U64 view_spec_table_size;
  DF_ViewSpec **view_spec_table;
  
  // rjf: view rule specs
  U64 view_rule_spec_table_size;
  DF_GfxViewRuleSpec **view_rule_spec_table;
  
  // rjf: view rule block state
  U64 view_rule_block_slots_count;
  DF_ViewRuleBlockSlot *view_rule_block_slots;
  DF_ViewRuleBlockNode *free_view_rule_block_node;
  
  // rjf: cmd param slot -> view spec rule table
  DF_CmdParamSlotViewSpecRuleList cmd_param_slot_view_spec_table[DF_CmdParamSlot_COUNT];
  
  // rjf: windows
  OS_WindowRepaintFunctionType *repaint_hook;
  DF_Window *first_window;
  DF_Window *last_window;
  DF_Window *free_window;
  U64 window_count;
  B32 last_window_queued_save;
  
  // rjf: view state
  DF_View *free_view;
  U64 free_view_count;
  U64 allocated_view_count;
  
  // rjf: drag/drop state machine
  DF_DragDropState drag_drop_state;
  
  // rjf: hover line info correllation state
  DF_Handle hover_line_binary;
  U64 hover_line_voff;
  B32 hover_line_set_this_frame;
  
  // rjf: running theme state
  DF_Theme cfg_theme_target;
  DF_Theme cfg_theme;
  Arena *cfg_main_font_path_arena;
  Arena *cfg_code_font_path_arena;
  String8 cfg_main_font_path;
  String8 cfg_code_font_path;
  F_Tag cfg_font_tags[DF_FontSlot_COUNT];
};

////////////////////////////////
//~ rjf: Globals

read_only global DF_ViewSpec df_g_nil_view_spec =
{
  &df_g_nil_view_spec,
  {
    0,
    {0},
    {0},
    DF_NameKind_Null,
    DF_IconKind_Null,
    DF_VIEW_SETUP_FUNCTION_NAME(Null),
    DF_VIEW_STRING_FROM_STATE_FUNCTION_NAME(Null),
    DF_VIEW_CMD_FUNCTION_NAME(Null),
    DF_VIEW_UI_FUNCTION_NAME(Null),
  },
};

read_only global DF_GfxViewRuleSpec df_g_nil_gfx_view_rule_spec =
{
  &df_g_nil_gfx_view_rule_spec,
};

read_only global DF_View df_g_nil_view =
{
  &df_g_nil_view,
  &df_g_nil_view,
  0,
  0,
  0,
  0,
  0,
  0,
  {0},
  {0},
  0,
  0,
  0,
  0,
  &df_g_nil_view_spec,
  {0},
};

read_only global DF_Panel df_g_nil_panel =
{
  &df_g_nil_panel,
  &df_g_nil_panel,
  &df_g_nil_panel,
  &df_g_nil_panel,
  &df_g_nil_panel,
  0,
};

global DF_GfxState *df_gfx_state = 0;
global DF_DragDropPayload df_g_drag_drop_payload = {0};

////////////////////////////////
//~ rjf: Basic Helpers

internal DF_PathQuery df_path_query_from_string(String8 string);

////////////////////////////////
//~ rjf: View Type Functions

internal B32 df_view_is_nil(DF_View *view);
internal DF_Handle df_handle_from_view(DF_View *view);
internal DF_View *df_view_from_handle(DF_Handle handle);

////////////////////////////////
//~ rjf: View Spec Type Functions

internal DF_GfxViewKind df_gfx_view_kind_from_string(String8 string);

////////////////////////////////
//~ rjf: Panel Type Functions

//- rjf: basic type functions
internal B32 df_panel_is_nil(DF_Panel *panel);
internal DF_Handle df_handle_from_panel(DF_Panel *panel);
internal DF_Panel *df_panel_from_handle(DF_Handle handle);
internal UI_Key df_ui_key_from_panel(DF_Panel *panel);

//- rjf: panel tree mutation notification
internal void df_panel_notify_mutation(DF_Window *window, DF_Panel *panel);

//- rjf: tree construction
internal void df_panel_insert(DF_Panel *parent, DF_Panel *prev_child, DF_Panel *new_child);
internal void df_panel_remove(DF_Panel *parent, DF_Panel *child);

//- rjf: tree walk
internal DF_PanelRec df_panel_rec_df(DF_Panel *panel, U64 sib_off, U64 child_off);
#define df_panel_rec_df_pre(panel) df_panel_rec_df(panel, OffsetOf(DF_Panel, next), OffsetOf(DF_Panel, first))
#define df_panel_rec_df_post(panel) df_panel_rec_df(panel, OffsetOf(DF_Panel, prev), OffsetOf(DF_Panel, last))

//- rjf: panel -> rect calculations
internal Rng2F32 df_rect_from_panel_child(Rng2F32 parent_rect, DF_Panel *parent, DF_Panel *panel);
internal Rng2F32 df_rect_from_panel(Rng2F32 root_rect, DF_Panel *root, DF_Panel *panel);

//- rjf: view ownership insertion/removal
internal void df_panel_insert_tab_view(DF_Panel *panel, DF_View *prev_view, DF_View *view);
internal void df_panel_remove_tab_view(DF_Panel *panel, DF_View *view);

//- rjf: icons & display strings
internal String8 df_display_string_from_view(Arena *arena, DF_CtrlCtx ctrl_ctx, DF_View *view);
internal DF_IconKind df_icon_kind_from_view(DF_View *view);

////////////////////////////////
//~ rjf: Window Type Functions

internal DF_Handle df_handle_from_window(DF_Window *window);
internal DF_Window *df_window_from_handle(DF_Handle handle);

////////////////////////////////
//~ rjf: Control Context

internal DF_CtrlCtx df_ctrl_ctx_from_window(DF_Window *ws);
internal DF_CtrlCtx df_ctrl_ctx_from_view(DF_Window *ws, DF_View *view);

////////////////////////////////
//~ rjf: Command Parameters From Context

internal DF_CmdParams df_cmd_params_from_gfx(void);
internal B32 df_prefer_dasm_from_window(DF_Window *window);
internal DF_CmdParams df_cmd_params_from_window(DF_Window *window);
internal DF_CmdParams df_cmd_params_from_panel(DF_Window *window, DF_Panel *panel);
internal DF_CmdParams df_cmd_params_from_view(DF_Window *window, DF_Panel *panel, DF_View *view);
internal DF_CmdParams df_cmd_params_copy(Arena *arena, DF_CmdParams *src);

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32 df_drag_is_active(void);
internal void df_drag_begin(DF_DragDropPayload *payload);
internal B32 df_drag_drop(DF_DragDropPayload *out_payload);
internal void df_drag_kill(void);
internal void df_queue_drag_drop(void);

internal void df_set_hovered_line_info(DF_Entity *binary, U64 voff);
internal DF_Entity *df_get_hovered_line_info_binary(void);
internal U64 df_get_hovered_line_info_voff(void);

////////////////////////////////
//~ rjf: View Spec State Functions

internal void df_register_view_specs(DF_ViewSpecInfoArray specs);
internal DF_ViewSpec *df_view_spec_from_string(String8 string);
internal DF_ViewSpec *df_view_spec_from_gfx_view_kind(DF_GfxViewKind gfx_view_kind);
internal DF_ViewSpec *df_view_spec_from_cmd_param_slot_spec(DF_CmdParamSlot slot, DF_CmdSpec *cmd_spec);

////////////////////////////////
//~ rjf: View Rule Spec State Functions

internal void df_register_gfx_view_rule_specs(DF_GfxViewRuleSpecInfoArray specs);
internal DF_GfxViewRuleSpec *df_gfx_view_rule_spec_from_string(String8 string);

////////////////////////////////
//~ rjf: View State Functions

internal DF_View *df_view_alloc(void);
internal void df_view_release(DF_View *view);
internal void df_view_equip_spec(DF_View *view, DF_ViewSpec *spec, DF_Entity *entity, String8 default_query, DF_CfgNode *cfg_root);
internal void df_view_equip_loading_info(DF_View *view, B32 is_loading, U64 progress_v, U64 progress_target);
internal void df_view_clear_user_state(DF_View *view);
internal void *df_view_get_or_push_user_state(DF_View *view, U64 size);
internal Arena *df_view_push_arena_ext(DF_View *view);
#define df_view_user_state(view, type) (type *)df_view_get_or_push_user_state((view), sizeof(type))

////////////////////////////////
//~ rjf: View Rule Instance State Functions

internal void *df_view_rule_block_get_or_push_user_state(DF_ExpandKey key, U64 size);
#define df_view_rule_block_user_state(key, type) (type *)df_view_rule_block_get_or_push_user_state(key, sizeof(type))
internal Arena *df_view_rule_block_push_arena_ext(DF_ExpandKey key);

////////////////////////////////
//~ rjf: Panel State Functions

internal DF_Panel *df_panel_alloc(DF_Window *ws);
internal void df_panel_release(DF_Window *ws, DF_Panel *panel);
internal void df_panel_release_all_views(DF_Panel *panel);

////////////////////////////////
//~ rjf: Window State Functions

internal DF_Window *df_window_open(Vec2F32 size, OS_Handle preferred_monitor, DF_CfgSrc cfg_src);

internal DF_Window *df_window_from_os_handle(OS_Handle os);

internal void df_window_update_and_render(Arena *arena, OS_EventList *events, DF_Window *ws, DF_CmdList *cmds);

////////////////////////////////
//~ rjf: Eval Viz

internal String8List df_single_line_eval_value_strings_from_eval(Arena *arena, DF_EvalVizStringFlags flags, TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, U32 default_radix, F_Tag font, F32 font_size, F32 max_size, S32 depth, DF_Eval eval, DF_CfgTable *cfg_table);
internal DF_EvalVizWindowedRowList df_eval_viz_windowed_row_list_from_viz_block_list(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, U32 default_radix, F_Tag font, F32 font_size, Rng1S64 visible_range, DF_EvalVizBlockList *blocks);

////////////////////////////////
//~ rjf: Hover Eval

internal void df_set_hover_eval(DF_Window *ws, Vec2F32 pos, DF_CtrlCtx ctrl_ctx, DF_Entity *file, TxtPt pt, U64 vaddr, String8 string);

////////////////////////////////
//~ rjf: Auto-Complete Lister

internal void df_autocomp_lister_item_chunk_list_push(Arena *arena, DF_AutoCompListerItemChunkList *list, U64 cap, DF_AutoCompListerItem *item);
internal DF_AutoCompListerItemArray df_autocomp_lister_item_array_from_chunk_list(Arena *arena, DF_AutoCompListerItemChunkList *list);
internal int df_autocomp_lister_item_qsort_compare(DF_AutoCompListerItem *a, DF_AutoCompListerItem *b);
internal void df_autocomp_lister_item_array_sort__in_place(DF_AutoCompListerItemArray *array);

internal void df_set_autocomp_lister_query(DF_Window *ws, UI_Key root_key, DF_CtrlCtx ctrl_ctx, DF_AutoCompListerFlags flags, String8 query);

////////////////////////////////
//~ rjf: Search Strings

internal void df_set_search_string(String8 string);
internal String8 df_push_search_string(Arena *arena);

////////////////////////////////
//~ rjf: Text Searching

internal void df_text_search_match_chunk_list_push(Arena *arena, DF_TextSearchMatchChunkList *list, U64 cap, DF_TextSearchMatch *match);
internal DF_TextSearchMatchArray df_text_search_match_array_from_chunk_list(Arena *arena, DF_TextSearchMatchChunkList *chunks);
internal U64 df_text_search_little_hash_from_hash(U128 hash);
internal void df_text_search_thread_entry_point(void *p);
internal int df_text_search_match_array_qsort_compare(TxtPt *a, TxtPt *b);
internal void df_text_search_match_array_sort_in_place(DF_TextSearchMatchArray *array);
internal DF_TextSearchMatch df_text_search_match_array_find_nearest__linear_scan(DF_TextSearchMatchArray *array, TxtPt pt, Side side);

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: keybindings
internal OS_Key df_os_key_from_cfg_string(String8 string);
internal void df_clear_bindings(void);
internal DF_BindingList df_bindings_from_spec(Arena *arena, DF_CmdSpec *spec);
internal void df_bind_spec(DF_CmdSpec *spec, DF_Binding binding);
internal void df_unbind_spec(DF_CmdSpec *spec, DF_Binding binding);
internal DF_CmdSpecList df_cmd_spec_list_from_binding(Arena *arena, DF_Binding binding);
internal DF_CmdSpecList df_cmd_spec_list_from_event_flags(Arena *arena, OS_EventFlags flags);

//- rjf: colors
internal Vec4F32 df_rgba_from_theme_color(DF_ThemeColor color);
internal DF_ThemeColor df_theme_color_from_txti_token_kind(TXTI_TokenKind kind);

//- rjf: fonts/sizes
internal F_Tag df_font_from_slot(DF_FontSlot slot);
internal F32 df_font_size_from_slot(DF_Window *ws, DF_FontSlot slot);

//- rjf: config serialization
internal String8List df_cfg_strings_from_gfx(Arena *arena, String8 root_path, DF_CfgSrc source);

////////////////////////////////
//~ rjf: UI Helpers

internal void df_box_equip_fuzzy_match_range_list_vis(UI_Box *box, FuzzyMatchRangeList range_list);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Buttons

internal void df_cmd_binding_button(DF_CmdSpec *spec);
internal UI_Signal df_menu_bar_button(String8 string);
internal UI_Signal df_cmd_spec_button(DF_CmdSpec *spec);
internal void df_cmd_list_menu_buttons(DF_Window *ws, U64 count, DF_CoreCmdKind *cmds, U32 *fastpath_codepoints);
internal UI_Signal df_icon_button(DF_IconKind kind, String8 string);
internal UI_Signal df_icon_buttonf(DF_IconKind kind, char *fmt, ...);
internal void df_entity_tooltips(DF_Entity *entity);
internal void df_entity_desc_button(DF_Window *ws, DF_Entity *entity);
internal void df_entity_src_loc_button(DF_Window *ws, DF_Entity *entity, TxtPt point);

////////////////////////////////
//~ rjf: UI Widgets: Text View

internal UI_BOX_CUSTOM_DRAW(df_thread_box_draw_extensions);
internal UI_BOX_CUSTOM_DRAW(df_bp_box_draw_extensions);
internal DF_CodeSliceSignal df_code_slice(DF_Window *ws, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, String8 string);
internal DF_CodeSliceSignal df_code_slicef(DF_Window *ws, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, char *fmt, ...);

internal B32 df_do_txti_controls(TXTI_Handle handle, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column);
internal B32 df_do_dasm_controls(DASM_Handle handle, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Labels

internal UI_Signal df_error_label(String8 string);
internal B32 df_help_label(String8 string);
internal D_FancyStringList df_fancy_string_list_from_code_string(Arena *arena, F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);
internal void df_code_label(F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);

////////////////////////////////
//~ rjf: UI Widgets: Line Edit

internal UI_Signal df_line_edit(DF_LineEditFlags flags, S32 depth, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, String8 string);
internal UI_Signal df_line_editf(DF_LineEditFlags flags, S32 depth, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, char *fmt, ...);

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void df_gfx_request_frame(void);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void df_gfx_init(OS_WindowRepaintFunctionType *window_repaint_entry_point, DF_StateDeltaHistory *hist);
internal void df_gfx_begin_frame(Arena *arena, DF_CmdList *cmds);
internal void df_gfx_end_frame(void);

#endif // DF_GFX_H
