// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_FRONTEND_CORE_H
#define DBG_FRONTEND_CORE_H

////////////////////////////////
//~ rjf: Config Slots

typedef enum DF_CfgSlot
{
  DF_CfgSlot_User,
  DF_CfgSlot_Project,
  DF_CfgSlot_CommandLine,
  DF_CfgSlot_COUNT
}
DF_CfgSlot;

////////////////////////////////
//~ rjf: Handle Types

typedef struct DF_Handle DF_Handle;
struct DF_Handle
{
  U64 u64[4];
};

typedef struct DF_HandleNode DF_HandleNode;
struct DF_HandleNode
{
  DF_HandleNode *next;
  DF_Handle v;
};

typedef struct DF_HandleList DF_HandleList;
struct DF_HandleList
{
  DF_HandleNode *first;
  DF_HandleNode *last;
  U64 count;
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
//~ rjf: Key Map Types

typedef struct DF_KeyMapNode DF_KeyMapNode;
struct DF_KeyMapNode
{
  DF_KeyMapNode *hash_next;
  DF_KeyMapNode *hash_prev;
  D_CmdSpec *spec;
  DF_Binding binding;
};

typedef struct DF_KeyMapSlot DF_KeyMapSlot;
struct DF_KeyMapSlot
{
  DF_KeyMapNode *first;
  DF_KeyMapNode *last;
};

////////////////////////////////
//~ rjf: Setting Types

typedef struct DF_SettingVal DF_SettingVal;
struct DF_SettingVal
{
  B32 set;
  S32 s32;
};

////////////////////////////////
//~ rjf: View Hook Function Types

typedef struct DF_View DF_View;

#define DF_VIEW_SETUP_FUNCTION_SIG(name) void name(void)
#define DF_VIEW_SETUP_FUNCTION_NAME(name) df_view_setup_##name
#define DF_VIEW_SETUP_FUNCTION_DEF(name) internal DF_VIEW_SETUP_FUNCTION_SIG(DF_VIEW_SETUP_FUNCTION_NAME(name))
typedef DF_VIEW_SETUP_FUNCTION_SIG(DF_ViewSetupFunctionType);

#define DF_VIEW_CMD_FUNCTION_SIG(name) void name(void)
#define DF_VIEW_CMD_FUNCTION_NAME(name) df_view_cmds_##name
#define DF_VIEW_CMD_FUNCTION_DEF(name) internal DF_VIEW_CMD_FUNCTION_SIG(DF_VIEW_CMD_FUNCTION_NAME(name))
typedef DF_VIEW_CMD_FUNCTION_SIG(DF_ViewCmdFunctionType);

#define DF_VIEW_UI_FUNCTION_SIG(name) void name(Rng2F32 rect)
#define DF_VIEW_UI_FUNCTION_NAME(name) df_view_ui_##name
#define DF_VIEW_UI_FUNCTION_DEF(name) internal DF_VIEW_UI_FUNCTION_SIG(DF_VIEW_UI_FUNCTION_NAME(name))
typedef DF_VIEW_UI_FUNCTION_SIG(DF_ViewUIFunctionType);

////////////////////////////////
//~ rjf: View Specification Types

typedef U32 DF_ViewSpecFlags;
enum
{
  DF_ViewSpecFlag_ParameterizedByEntity      = (1<<0),
  DF_ViewSpecFlag_ProjectSpecific            = (1<<1),
  DF_ViewSpecFlag_CanSerialize               = (1<<2),
  DF_ViewSpecFlag_CanFilter                  = (1<<3),
  DF_ViewSpecFlag_FilterIsCode               = (1<<4),
  DF_ViewSpecFlag_TypingAutomaticallyFilters = (1<<5),
};

typedef struct DF_ViewSpecInfo DF_ViewSpecInfo;
struct DF_ViewSpecInfo
{
  DF_ViewSpecFlags flags;
  String8 name;
  String8 display_string;
  U32 icon_kind;
  DF_ViewSetupFunctionType *setup_hook;
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

typedef struct D_CmdParamSlotViewSpecRuleNode D_CmdParamSlotViewSpecRuleNode;
struct D_CmdParamSlotViewSpecRuleNode
{
  D_CmdParamSlotViewSpecRuleNode *next;
  DF_ViewSpec *view_spec;
  D_CmdSpec *cmd_spec;
};

typedef struct D_CmdParamSlotViewSpecRuleList D_CmdParamSlotViewSpecRuleList;
struct D_CmdParamSlotViewSpecRuleList
{
  D_CmdParamSlotViewSpecRuleNode *first;
  D_CmdParamSlotViewSpecRuleNode *last;
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

typedef struct DF_TransientViewNode DF_TransientViewNode;
struct DF_TransientViewNode
{
  DF_TransientViewNode *next;
  DF_TransientViewNode *prev;
  D_ExpandKey key;
  DF_View *view;
  Arena *initial_params_arena;
  MD_Node *initial_params;
  U64 first_frame_index_touched;
  U64 last_frame_index_touched;
};

typedef struct DF_TransientViewSlot DF_TransientViewSlot;
struct DF_TransientViewSlot
{
  DF_TransientViewNode *first;
  DF_TransientViewNode *last;
};

typedef struct DF_View DF_View;
struct DF_View
{
  // rjf: allocation links (for iterating all views)
  DF_View *alloc_next;
  DF_View *alloc_prev;
  
  // rjf: ownership links ('owners' can have lists of views)
  DF_View *order_next;
  DF_View *order_prev;
  
  // rjf: cfg tree handle
  DF_Handle handle;
  
  // rjf: transient view children
  DF_View *first_transient;
  DF_View *last_transient;
  
  // rjf: view specification info
  DF_ViewSpec *spec;
  
  // rjf: allocation info
  U64 generation;
  
  // rjf: loading animation state
  F32 loading_t;
  F32 loading_t_target;
  U64 loading_progress_v;
  U64 loading_progress_v_target;
  
  // rjf: view project (for project-specific/filtered views)
  Arena *project_path_arena;
  String8 project_path;
  
  // rjf: view state
  UI_ScrollPt2 scroll_pos;
  
  // rjf: view-lifetime allocation & user data extensions
  Arena *arena;
  DF_ArenaExt *first_arena_ext;
  DF_ArenaExt *last_arena_ext;
  U64 transient_view_slots_count;
  DF_TransientViewSlot *transient_view_slots;
  DF_TransientViewNode *free_transient_view_node;
  void *user_data;
  
  // rjf: filter mode
  B32 is_filtering;
  F32 is_filtering_t;
  
  // rjf: params tree state
  Arena *params_arenas[2];
  MD_Node *params_roots[2];
  U64 params_write_gen;
  U64 params_read_gen;
  
  // rjf: text query state
  TxtPt query_cursor;
  TxtPt query_mark;
  U64 query_string_size;
  U8 query_buffer[KB(4)];
};

typedef struct DF_ViewNode DF_ViewNode;
struct DF_ViewNode
{
  DF_ViewNode *next;
  DF_ViewNode *prev;
  DF_View v;
};

typedef struct DF_ViewSlot DF_ViewSlot;
struct DF_ViewSlot
{
  DF_ViewNode *first;
  DF_ViewNode *last;
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
  F32 pct_of_parent;
  
  // rjf: animated rectangle data
  Rng2F32 animated_rect_pct;
  
  // rjf: tab params
  Side tab_side;
  
  // rjf: stable views (tabs)
  DF_View *first_tab_view;
  DF_View *last_tab_view;
  U64 tab_view_count;
  D_Handle selected_tab_view;
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
  UI_Key key__;
  DF_Handle cfg_tree;
  D_Handle entity;
  TxtPt text_point;
};

////////////////////////////////
//~ rjf: Rich Hover Types

typedef struct DF_RichHoverInfo DF_RichHoverInfo;
struct DF_RichHoverInfo
{
  D_Handle process;
  Rng1U64 vaddr_range;
  D_Handle module;
  Rng1U64 voff_range;
  DI_Key dbgi_key;
};

////////////////////////////////
//~ rjf: View Rule Spec Types

typedef U32 DF_ViewRuleSpecInfoFlags; // NOTE(rjf): see @view_rule_info
enum
{
  DF_ViewRuleSpecInfoFlag_VizRowProd     = (1<<0),
  DF_ViewRuleSpecInfoFlag_LineStringize  = (1<<1),
  DF_ViewRuleSpecInfoFlag_RowUI          = (1<<2),
  DF_ViewRuleSpecInfoFlag_ViewUI         = (1<<3),
};

#define DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(name) void name(void)
#define DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_NAME(name) df_view_rule_viz_row_prod__##name
#define DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(name) internal DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_NAME(name))

#define DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(name) void name(void)
#define DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_NAME(name) df_view_rule_line_stringize__##name
#define DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(name) internal DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_NAME(name))

#define DF_VIEW_RULE_ROW_UI_FUNCTION_SIG(name) void name(D_ExpandKey key, MD_Node *params, String8 string)
#define DF_VIEW_RULE_ROW_UI_FUNCTION_NAME(name) df_view_rule_row_ui__##name
#define DF_VIEW_RULE_ROW_UI_FUNCTION_DEF(name) DF_VIEW_RULE_ROW_UI_FUNCTION_SIG(DF_VIEW_RULE_ROW_UI_FUNCTION_NAME(name))

typedef DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_SIG(DF_ViewRuleVizRowProdHookFunctionType);
typedef DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_SIG(DF_ViewRuleLineStringizeHookFunctionType);
typedef DF_VIEW_RULE_ROW_UI_FUNCTION_SIG(DF_ViewRuleRowUIFunctionType);

typedef struct DF_ViewRuleSpecInfo DF_ViewRuleSpecInfo;
struct DF_ViewRuleSpecInfo
{
  String8 string;
  DF_ViewRuleSpecInfoFlags flags;
  DF_ViewRuleVizRowProdHookFunctionType *viz_row_prod;
  DF_ViewRuleLineStringizeHookFunctionType *line_stringize;
  DF_ViewRuleRowUIFunctionType *row_ui;
};

typedef struct DF_ViewRuleSpecInfoArray DF_ViewRuleSpecInfoArray;
struct DF_ViewRuleSpecInfoArray
{
  DF_ViewRuleSpecInfo *v;
  U64 count;
};

typedef struct DF_ViewRuleSpec DF_ViewRuleSpec;
struct DF_ViewRuleSpec
{
  DF_ViewRuleSpec *hash_next;
  DF_ViewRuleSpecInfo info;
};

////////////////////////////////
//~ rjf: Message Kind Metadata Types

typedef U32 DF_MsgQueryFlags;
enum
{
  DF_MsgQueryFlag_AllowFiles       = (1<<0),
  DF_MsgQueryFlag_AllowFolders     = (1<<1),
  DF_MsgQueryFlag_CodeInput        = (1<<2),
  DF_MsgQueryFlag_KeepOldInput     = (1<<3),
  DF_MsgQueryFlag_SelectOldInput   = (1<<4),
  DF_MsgQueryFlag_Required         = (1<<5),
};

typedef struct DF_MsgQuery DF_MsgQuery;
struct DF_MsgQuery
{
  DF_MsgQueryFlags flags;
  D_RegSlot slot;
  D_EntityKind entity_kind;
};

typedef U32 DF_MsgKindFlags;
enum
{
  DF_MsgKindFlag_ListInUI      = (1<<0),
  DF_MsgKindFlag_ListInIPCDocs = (1<<1),
};

typedef struct DF_MsgKindInfo DF_MsgKindInfo;
struct DF_MsgKindInfo
{
  String8 name_lower;
  String8 display_name;
  String8 description;
  String8 search_tags;
  DF_MsgKindFlags flags;
  DF_MsgQuery query;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_frontend.meta.h"

////////////////////////////////
//~ rjf: Message Types

typedef struct DF_Msg DF_Msg;
struct DF_Msg
{
  DF_MsgKind kind;
  DF_Regs *regs;
};

typedef struct DF_MsgNode DF_MsgNode;
struct DF_MsgNode
{
  DF_MsgNode *next;
  DF_Msg v;
};

typedef struct DF_MsgList DF_MsgList;
struct DF_MsgList
{
  DF_MsgNode *first;
  DF_MsgNode *last;
  U64 count;
};

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

typedef enum DF_PaletteCode
{
  DF_PaletteCode_Base,
  DF_PaletteCode_MenuBar,
  DF_PaletteCode_Floating,
  DF_PaletteCode_ImplicitButton,
  DF_PaletteCode_PlainButton,
  DF_PaletteCode_PositivePopButton,
  DF_PaletteCode_NegativePopButton,
  DF_PaletteCode_NeutralPopButton,
  DF_PaletteCode_ScrollBarButton,
  DF_PaletteCode_Tab,
  DF_PaletteCode_TabInactive,
  DF_PaletteCode_DropSiteOverlay,
  DF_PaletteCode_COUNT
}
DF_PaletteCode;

////////////////////////////////
//~ rjf: Auto-Complete Lister Types

typedef U32 DF_AutoCompListerFlags;
enum
{
  DF_AutoCompListerFlag_Locals        = (1<<0),
  DF_AutoCompListerFlag_Registers     = (1<<1),
  DF_AutoCompListerFlag_ViewRules     = (1<<2),
  DF_AutoCompListerFlag_ViewRuleParams= (1<<3),
  DF_AutoCompListerFlag_Members       = (1<<4),
  DF_AutoCompListerFlag_Languages     = (1<<5),
  DF_AutoCompListerFlag_Architectures = (1<<6),
  DF_AutoCompListerFlag_Tex2DFormats  = (1<<7),
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

typedef struct DF_AutoCompListerParams DF_AutoCompListerParams;
struct DF_AutoCompListerParams
{
  DF_AutoCompListerFlags flags;
  String8List strings;
};

////////////////////////////////
//~ rjf: Window State Cache

typedef struct DF_Window DF_Window;
struct DF_Window
{
  // rjf: links & metadata
  DF_Window *next;
  DF_Window *prev;
  DF_Handle handle;
  U64 first_frame_touched;
  U64 last_frame_touched;
  U64 gen;
  U64 frames_alive;
  D_CfgSrc cfg_src;
  
  // rjf: top-level info & handles
  Arena *arena;
  OS_Handle os;
  R_Handle r;
  UI_State *ui;
  F32 last_dpi;
  B32 window_temporarily_focused_ipc;
  
  // rjf: config/settings
  DF_SettingVal setting_vals[DF_SettingCode_COUNT];
  UI_Palette cfg_palettes[DF_PaletteCode_COUNT]; // derivative from theme
  
  // rjf: dev interface state
  B32 dev_menu_is_open;
  
  // rjf: menu bar state
  B32 menu_bar_focused;
  B32 menu_bar_focused_on_press;
  B32 menu_bar_key_held;
  B32 menu_bar_focus_press_started;
  
  // rjf: code context menu state
  Arena *code_ctx_menu_arena;
  String8 code_ctx_menu_file_path;
  U128 code_ctx_menu_text_key;
  TXT_LangKind code_ctx_menu_lang_kind;
  TxtRng code_ctx_menu_range;
  U64 code_ctx_menu_vaddr;
  D_LineList code_ctx_menu_lines;
  
  // rjf: entity context menu state
  D_Handle entity_ctx_menu_entity;
  U8 entity_ctx_menu_input_buffer[1024];
  U64 entity_ctx_menu_input_size;
  TxtPt entity_ctx_menu_input_cursor;
  TxtPt entity_ctx_menu_input_mark;
  
  // rjf: tab context menu state
  DF_Handle tab_ctx_menu_view;
  U8 tab_ctx_menu_input_buffer[1024];
  U64 tab_ctx_menu_input_size;
  TxtPt tab_ctx_menu_input_cursor;
  TxtPt tab_ctx_menu_input_mark;
  
  // rjf: autocomplete lister state
  U64 autocomp_last_frame_idx;
  B32 autocomp_force_closed;
  B32 autocomp_query_dirty;
  UI_Key autocomp_root_key;
  Arena *autocomp_lister_params_arena;
  DF_AutoCompListerParams autocomp_lister_params;
  U64 autocomp_cursor_off;
  U8 autocomp_lister_query_buffer[1024];
  U64 autocomp_lister_query_size;
  F32 autocomp_open_t;
  F32 autocomp_num_visible_rows_t;
  S64 autocomp_cursor_num;
  
  // rjf: query view stack
  Arena *query_msg_arena;
  DF_MsgKind query_msg_kind;
  DF_MsgQuery query_msg_query;
  U64 query_msg_regs_mask[(D_RegSlot_COUNT+63)/64];
  D_Regs *query_msg_regs;
  Arena *query_cmd_arena;
  D_CmdSpec *query_cmd_spec;
  U64 query_cmd_params_mask[(D_CmdParamSlot_COUNT + 63) / 64];
  D_CmdParams query_cmd_params;
  DF_View *query_view_stack_top;
  B32 query_view_selected;
  F32 query_view_selected_t;
  F32 query_view_t;
  
  // rjf: hover eval stable state
  B32 hover_eval_focused;
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
  String8 hover_eval_file_path;
  TxtPt hover_eval_file_pt;
  U64 hover_eval_vaddr;
  F32 hover_eval_open_t;
  F32 hover_eval_num_visible_rows_t;
  
  // rjf: error state
  U8 error_buffer[512];
  U64 error_string_size;
  F32 error_t;
  
  // rjf: panel state
  DF_Panel *root_panel;
  DF_Panel *free_panel;
  DF_Panel *focused_panel;
  
  // rjf: per-frame drawing state
  DR_Bucket *draw_bucket;
};

typedef struct DF_WindowNode DF_WindowNode;
struct DF_WindowNode
{
  DF_WindowNode *next;
  DF_WindowNode *prev;
  DF_Window v;
};

typedef struct DF_WindowSlot DF_WindowSlot;
struct DF_WindowSlot
{
  DF_WindowNode *first;
  DF_WindowNode *last;
};

////////////////////////////////
//~ rjf: Context Register Types

typedef struct DF_RegsNode DF_RegsNode;
struct DF_RegsNode
{
  DF_RegsNode *next;
  DF_Regs v;
};

////////////////////////////////
//~ rjf: Main Per-Process Graphical State

typedef struct DF_State DF_State;
struct DF_State
{
  Arena *arena;
  B32 quit;
  U64 frame_index;
  
  // rjf: frame arenas
  Arena *frame_arenas[2];
  
  // rjf: icon texture
  R_Handle icon_texture;
  
  // rjf: frame time history
  U64 frame_time_us_history[64];
  
  // rjf: frame request state
  U64 num_frames_requested;
  
  // rjf: config trees
  Arena *cfg_root_arena;
  MD_Node *cfg_root;
  Arena *cfg_slot_arenas[DF_CfgSlot_COUNT];
  U64 cfg_slot_gens[DF_CfgSlot_COUNT];
  MD_Node *cfg_slot_roots[DF_CfgSlot_COUNT];
  
  // rjf: interaction registers
  DF_RegsNode base_regs;
  DF_RegsNode *top_regs;
  
  // rjf: messages
  Arena *msgs_arena;
  DF_MsgList msgs;
  
  // rjf: window state cache
  U64 window_slots_count;
  DF_WindowSlot *window_slots;
  
  // rjf: view state cache
  U64 view_slots_count;
  DF_ViewSlot *view_slots;
  
  // rjf: key map table
  Arena *key_map_arena;
  U64 key_map_table_size;
  DF_KeyMapSlot *key_map_table;
  DF_KeyMapNode *free_key_map_node;
  U64 key_map_total_count;
  
  // rjf: bind change
  B32 bind_change_active;
  D_CmdSpec *bind_change_cmd_spec;
  DF_Binding bind_change_binding;
  
  // rjf: top-level context menu keys
  UI_Key code_ctx_menu_key;
  UI_Key entity_ctx_menu_key;
  UI_Key tab_ctx_menu_key;
  
  // rjf: confirmation popup state
  UI_Key confirm_key;
  B32 confirm_active;
  F32 confirm_t;
  Arena *confirm_arena;
  DF_Msg confirm_msg;
  String8 confirm_title;
  String8 confirm_desc;
  
  // rjf: string search state
  Arena *string_search_arena;
  String8 string_search_string;
  
  // rjf: view specs
  U64 view_spec_table_size;
  DF_ViewSpec **view_spec_table;
  
  // rjf: view rule specs
  U64 view_rule_spec_table_size;
  DF_ViewRuleSpec **view_rule_spec_table;
  
  // rjf: cmd param slot -> view spec rule table
  D_CmdParamSlotViewSpecRuleList cmd_param_slot_view_spec_table[D_CmdParamSlot_COUNT];
  
  // rjf: windows
  DF_Window *first_window;
  DF_Window *last_window;
  DF_Window *free_window;
  U64 window_count;
  B32 last_window_queued_save;
  D_Handle last_focused_window;
  
  // rjf: view state
  DF_View *first_view;
  DF_View *last_view;
  DF_View *free_view;
  U64 free_view_count;
  U64 allocated_view_count;
  
  // rjf: drag/drop state machine
  DF_DragDropState drag_drop_state;
  
  // rjf: rich hover info
  Arena *rich_hover_info_next_arena;
  Arena *rich_hover_info_current_arena;
  DF_RichHoverInfo rich_hover_info_next;
  DF_RichHoverInfo rich_hover_info_current;
  
  // rjf: running theme state
  DF_Theme cfg_theme_target;
  DF_Theme cfg_theme;
  Arena *cfg_main_font_path_arena;
  Arena *cfg_code_font_path_arena;
  String8 cfg_main_font_path;
  String8 cfg_code_font_path;
  FNT_Tag cfg_font_tags[DF_FontSlot_COUNT]; // derivative from font paths
  
  // rjf: global settings
  DF_SettingVal cfg_setting_vals[D_CfgSrc_COUNT][DF_SettingCode_COUNT];
};

////////////////////////////////
//~ rjf: Globals

read_only global DF_ViewSpec df_nil_view_spec =
{
  &df_nil_view_spec,
  {
    0,
    {0},
    {0},
    DF_IconKind_Null,
    DF_VIEW_SETUP_FUNCTION_NAME(null),
    DF_VIEW_CMD_FUNCTION_NAME(null),
    DF_VIEW_UI_FUNCTION_NAME(null),
  },
};

read_only global DF_ViewRuleSpec df_nil_view_rule_spec =
{
  &df_nil_view_rule_spec,
};

read_only global DF_Window df_nil_window = {0};

read_only global DF_View df_nil_view =
{
  &df_nil_view,
  &df_nil_view,
  &df_nil_view,
  &df_nil_view,
  {0},
  &df_nil_view,
  &df_nil_view,
  &df_nil_view_spec,
};

read_only global DF_Panel df_nil_panel =
{
  &df_nil_panel,
  &df_nil_panel,
  &df_nil_panel,
  &df_nil_panel,
  &df_nil_panel,
};

global DF_State *df_state = 0;
global DF_DragDropPayload df_drag_drop_payload = {0};
global D_Handle df_last_drag_drop_panel = {0};
global D_Handle df_last_drag_drop_prev_tab = {0};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 df_hash_from_seed_string(U64 seed, String8 string);
internal U64 df_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Handle Type Functions

internal DF_Handle df_handle_zero(void);
internal B32 df_handle_match(DF_Handle a, DF_Handle b);
internal void df_handle_list_push(Arena *arena, DF_HandleList *list, DF_Handle v);
internal DF_HandleList df_handle_list_copy(Arena *arena, DF_HandleList *src);

////////////////////////////////
//~ rjf: Register Type Functions

internal void df_regs_copy_contents(Arena *arena, DF_Regs *dst, DF_Regs *src);
internal DF_Regs *df_regs_copy(Arena *arena, DF_Regs *src);

////////////////////////////////
//~ rjf: View Type Functions

internal B32 df_view_is_nil(DF_View *view);
internal B32 df_view_is_project_filtered(DF_View *view);
internal D_Handle df_handle_from_view(DF_View *view);
internal DF_View *df_view_from_handle(D_Handle handle);

////////////////////////////////
//~ rjf: View Spec Type Functions

internal DF_ViewKind df_view_kind_from_string(String8 string);

////////////////////////////////
//~ rjf: Panel Type Functions

//- rjf: basic type functions
internal B32 df_panel_is_nil(DF_Panel *panel);
internal D_Handle df_handle_from_panel(DF_Panel *panel);
internal DF_Panel *df_panel_from_handle(D_Handle handle);
internal UI_Key df_ui_key_from_panel(DF_Panel *panel);

//- rjf: tree construction
internal void df_panel_insert(DF_Panel *parent, DF_Panel *prev_child, DF_Panel *new_child);
internal void df_panel_remove(DF_Panel *parent, DF_Panel *child);

//- rjf: tree walk
internal DF_PanelRec df_panel_rec_df(DF_Panel *panel, U64 sib_off, U64 child_off);
#define df_panel_rec_df_pre(panel) df_panel_rec_df(panel, OffsetOf(DF_Panel, next), OffsetOf(DF_Panel, first))
#define df_panel_rec_df_post(panel) df_panel_rec_df(panel, OffsetOf(DF_Panel, prev), OffsetOf(DF_Panel, last))

//- rjf: panel -> rect calculations
internal Rng2F32 df_target_rect_from_panel_child(Rng2F32 parent_rect, DF_Panel *parent, DF_Panel *panel);
internal Rng2F32 df_target_rect_from_panel(Rng2F32 root_rect, DF_Panel *root, DF_Panel *panel);

//- rjf: view ownership insertion/removal
internal void df_panel_insert_tab_view(DF_Panel *panel, DF_View *prev_view, DF_View *view);
internal void df_panel_remove_tab_view(DF_Panel *panel, DF_View *view);
internal DF_View *df_selected_tab_from_panel(DF_Panel *panel);

//- rjf: icons & display strings
internal DF_IconKind df_icon_kind_from_view(DF_View *view);
internal DR_FancyStringList df_title_fstrs_from_view_spec_query(Arena *arena, DF_ViewSpec *spec, String8 query, Vec4F32 primary_color, Vec4F32 secondary_color, F32 size);

////////////////////////////////
//~ rjf: Window Type Functions

internal D_Handle df_handle_from_window(DF_Window *window);
internal DF_Window *df_window_from_handle(D_Handle handle);

////////////////////////////////
//~ rjf: Command Parameters From Context

internal D_CmdParams df_cmd_params_from_gfx(void);
internal B32 df_prefer_dasm_from_window(DF_Window *window);
#if 0 // TODO(rjf): @msgs
internal D_CmdParams df_cmd_params_from_window(DF_Window *window);
internal D_CmdParams df_cmd_params_from_panel(DF_Window *window, DF_Panel *panel);
internal D_CmdParams df_cmd_params_from_view(DF_Window *window, DF_Panel *panel, DF_View *view);
#endif
internal D_CmdParams df_cmd_params_copy(Arena *arena, D_CmdParams *src);

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32 df_drag_is_active(void);
internal void df_drag_begin(DF_DragDropPayload *payload);
internal B32 df_drag_drop(DF_DragDropPayload *out_payload);
internal void df_drag_kill(void);
internal void df_queue_drag_drop(void);

internal void df_set_rich_hover_info(DF_RichHoverInfo *info);
internal DF_RichHoverInfo df_get_rich_hover_info(void);

////////////////////////////////
//~ rjf: View Spec State Functions

internal void df_register_view_specs(DF_ViewSpecInfoArray specs);
internal DF_ViewSpec *df_view_spec_from_string(String8 string);
internal DF_ViewSpec *df_view_spec_from_kind(DF_ViewKind kind);
internal DF_ViewSpec *df_view_spec_from_cmd_param_slot_spec(D_CmdParamSlot slot, D_CmdSpec *cmd_spec);

////////////////////////////////
//~ rjf: View Rule Spec State Functions

internal void df_register_view_rule_specs(DF_ViewRuleSpecInfoArray specs);
internal DF_ViewRuleSpec *df_view_rule_spec_from_string(String8 string);

////////////////////////////////
//~ rjf: View State Functions

//- rjf: cfg tree -> view
internal DF_View *df_view_from_cfg_tree(MD_Node *view_cfg);

//- rjf: allocation/releasing
internal DF_View *df_view_alloc(void);
internal void df_view_release(DF_View *view);

//- rjf: equipment
internal void df_view_equip_spec(DF_View *view, DF_ViewSpec *spec, String8 query, MD_Node *params);
internal void df_view_equip_query(DF_View *view, String8 query);
internal void df_view_equip_loading_info(DF_View *view, B32 is_loading, U64 progress_v, U64 progress_target);

//- rjf: user state extensions
internal void *df_view_get_or_push_user_state(DF_View *view, U64 size);
internal Arena *df_view_push_arena_ext(DF_View *view);
#define df_view_user_state(view, type) (type *)df_view_get_or_push_user_state((view), sizeof(type))

//- rjf: param saving
internal void df_view_store_param(DF_View *view, String8 key, String8 value);
internal void df_view_store_paramf(DF_View *view, String8 key, char *fmt, ...);
#define df_view_store_param_f32(view, key, f32) df_view_store_paramf((view), (key), "%ff", (f32))
#define df_view_store_param_s64(view, key, s64) df_view_store_paramf((view), (key), "%I64d", (s64))
#define df_view_store_param_u64(view, key, u64) df_view_store_paramf((view), (key), "0x%I64x", (u64))

////////////////////////////////
//~ rjf: Expand-Keyed Transient View Functions

internal DF_TransientViewNode *df_transient_view_node_from_expand_key(DF_View *owner_view, D_ExpandKey key);

////////////////////////////////
//~ rjf: Panel State Functions

internal DF_Panel *df_panel_alloc(DF_Window *ws);
internal void df_panel_release(DF_Window *ws, DF_Panel *panel);
internal void df_panel_release_all_views(DF_Panel *panel);

////////////////////////////////
//~ rjf: Window State Functions

internal DF_Window *df_window_from_cfg_tree(MD_Node *window_cfg);

internal DF_Window *df_window_open(Vec2F32 size, OS_Handle preferred_monitor, D_CfgSrc cfg_src);

internal DF_Window *df_window_from_os_handle(OS_Handle os);

internal void df_window_frame(Arena *arena, MD_Node *window_cfg);

////////////////////////////////
//~ rjf: Eval Viz

internal F32 df_append_value_strings_from_eval(Arena *arena, D_EvalVizStringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, S32 depth, E_Eval eval, E_Member *member, D_CfgTable *cfg_table, String8List *out);
internal String8 df_value_string_from_eval(Arena *arena, D_EvalVizStringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval, E_Member *member, D_CfgTable *cfg_table);

////////////////////////////////
//~ rjf: Hover Eval

internal void df_set_hover_eval(Vec2F32 pos, String8 file_path, TxtPt pt, U64 vaddr, String8 string);

////////////////////////////////
//~ rjf: Auto-Complete Lister

internal void df_autocomp_lister_item_chunk_list_push(Arena *arena, DF_AutoCompListerItemChunkList *list, U64 cap, DF_AutoCompListerItem *item);
internal DF_AutoCompListerItemArray df_autocomp_lister_item_array_from_chunk_list(Arena *arena, DF_AutoCompListerItemChunkList *list);
internal int df_autocomp_lister_item_qsort_compare(DF_AutoCompListerItem *a, DF_AutoCompListerItem *b);
internal void df_autocomp_lister_item_array_sort__in_place(DF_AutoCompListerItemArray *array);

internal String8 df_autocomp_query_word_from_input_string_off(String8 input, U64 cursor_off);
internal DF_AutoCompListerParams df_view_rule_autocomp_lister_params_from_input_cursor(Arena *arena, String8 string, U64 cursor_off);
internal void df_set_autocomp_lister_query(UI_Key root_key, DF_AutoCompListerParams *params, String8 input, U64 cursor_off);

////////////////////////////////
//~ rjf: Search Strings

internal void df_set_search_string(String8 string);
internal String8 df_push_search_string(Arena *arena);

////////////////////////////////
//~ rjf: Main State Accessors

internal Arena *df_frame_arena(void);

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: handle <-> cfg tree
internal DF_Handle df_handle_from_cfg_tree(MD_Node *cfg);
internal MD_Node *df_cfg_tree_from_handle(DF_Handle handle);

//- rjf: string <-> cfg tree
internal MD_Node *df_cfg_tree_from_key(String8 string);
internal String8 df_key_from_cfg_tree(Arena *arena, MD_Node *node);

//- rjf: config tree mutations
internal DF_CfgSlot df_cfg_slot_from_tree(MD_Node *node);
internal MD_Node *df_cfg_tree_store(MD_Node *parent, MD_Node *replace_node, String8 string);
internal MD_Node *df_cfg_tree_storef(MD_Node *parent, MD_Node *replace_node, char *fmt, ...);
internal void df_cfg_tree_set_string(MD_Node *node, String8 string);
internal void df_cfg_tree_set_stringf(MD_Node *node, char *fmt, ...);
#define df_cfg_tree_set_key(parent, key, val) df_cfg_tree_store((parent), md_child_from_string((parent), (key), 0), (val))
#define df_cfg_tree_set_keyf(parent, key, fmt, ...) df_cfg_tree_storef((parent), md_child_from_string((parent), (key), 0), (fmt), __VA_ARGS__)
#define df_cfg_tree_remove(node) df_cfg_tree_store((node)->parent, (node), str8_zero())

//- rjf: config tree lookups
internal Rng2F32 df_target_rect_from_panel_child_cfg(Rng2F32 parent_rect, Axis2 parent_split_axis, MD_Node *panel);
internal Rng2F32 df_target_rect_from_panel_cfg(Rng2F32 root_rect, MD_Node *panel);

//- rjf: keybindings
internal void df_clear_bindings(void);
internal DF_BindingList df_bindings_from_spec(Arena *arena, D_CmdSpec *spec);
internal void df_bind_spec(D_CmdSpec *spec, DF_Binding binding);
internal void df_unbind_spec(D_CmdSpec *spec, DF_Binding binding);
internal D_CmdSpecList df_cmd_spec_list_from_binding(Arena *arena, DF_Binding binding);
internal D_CmdSpecList df_cmd_spec_list_from_event_flags(Arena *arena, OS_EventFlags flags);

//- rjf: colors
internal Vec4F32 df_rgba_from_theme_color(DF_ThemeColor color);
internal DF_ThemeColor df_theme_color_from_txt_token_kind(TXT_TokenKind kind);

//- rjf: code -> palette
internal UI_Palette *df_palette_from_code(DF_PaletteCode code);

//- rjf: fonts/sizes
internal FNT_Tag df_font_from_slot(DF_FontSlot slot);
internal F32 df_font_size_from_slot(DF_FontSlot slot);
internal FNT_RasterFlags df_raster_flags_from_slot(DF_FontSlot slot);

//- rjf: settings
internal DF_SettingVal df_setting_val_from_code(DF_SettingCode code);

//- rjf: config serialization
#if 0 // TODO(rjf): @msgs
internal int df_qsort_compare__cfg_string_bindings(DF_StringBindingPair *a, DF_StringBindingPair *b);
internal String8List df_cfg_strings_from_state(Arena *arena, String8 root_path, D_CfgSrc source);
#endif

////////////////////////////////
//~ rjf: Process Control Info Stringification

internal String8 df_string_from_exception_code(U32 code);
internal String8 df_stop_explanation_string_icon_from_ctrl_event(Arena *arena, CTRL_Event *event, DF_IconKind *icon_out);

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void df_request_frame(void);

////////////////////////////////
//~ rjf: Registers Functions

internal DF_Regs *df_regs(void);
internal DF_Regs *df_base_regs(void);
internal DF_Regs *df_push_regs(void);
internal DF_Regs *df_pop_regs(void);
#define DF_RegsScope DeferLoop(df_push_regs(), df_pop_regs())

////////////////////////////////
//~ rjf: Message Functions

//- rjf: string -> msg kind
internal DF_MsgKind df_msg_kind_from_string(String8 string);

//- rjf: register setting helpers
internal void df_regs_set_window(MD_Node *cfg_tree);
internal void df_regs_set_panel(MD_Node *cfg_tree);
internal void df_regs_set_view(MD_Node *cfg_tree);
internal void df_regs_set_from_query_slot_string(D_RegSlot slot, String8 string);

//- rjf: message pushing
internal void df_msg_(DF_MsgKind kind, DF_Regs *regs);
#define df_msg(kind, ...) df_msg_((kind),\
&(DF_Regs)\
{\
df_regs_lit_init_top \
__VA_ARGS__\
})

//- rjf: message iteration
internal B32 df_next_msg(DF_Msg **msg);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void df_init(CmdLine *cmdln);
internal void df_frame(void);

#endif // DBG_FRONTEND_CORE_H
