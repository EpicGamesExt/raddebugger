// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_CORE_H
#define RADDBG_CORE_H

////////////////////////////////
//~ rjf: Config IDs

typedef U64 RD_CfgID;

typedef struct RD_CfgIDNode RD_CfgIDNode;
struct RD_CfgIDNode
{
  RD_CfgIDNode *next;
  RD_CfgID v;
};

typedef struct RD_CfgIDList RD_CfgIDList;
struct RD_CfgIDList
{
  RD_CfgIDNode *first;
  RD_CfgIDNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Key Bindings

typedef struct RD_Binding RD_Binding;
struct RD_Binding
{
  OS_Key key;
  OS_Modifiers modifiers;
};

typedef struct RD_KeyMapNode RD_KeyMapNode;
struct RD_KeyMapNode
{
  RD_KeyMapNode *name_hash_next;
  RD_KeyMapNode *binding_hash_next;
  RD_CfgID cfg_id;
  String8 name;
  RD_Binding binding;
};

typedef struct RD_KeyMapNodePtr RD_KeyMapNodePtr;
struct RD_KeyMapNodePtr
{
  RD_KeyMapNodePtr *next;
  RD_KeyMapNode *v;
};

typedef struct RD_KeyMapNodePtrList RD_KeyMapNodePtrList;
struct RD_KeyMapNodePtrList
{
  RD_KeyMapNodePtr *first;
  RD_KeyMapNodePtr *last;
  U64 count;
};

typedef struct RD_KeyMapSlot RD_KeyMapSlot;
struct RD_KeyMapSlot
{
  RD_KeyMapNode *first;
  RD_KeyMapNode *last;
};

typedef struct RD_KeyMap RD_KeyMap;
struct RD_KeyMap
{
  U64 name_slots_count;
  RD_KeyMapSlot *name_slots;
  U64 binding_slots_count;
  RD_KeyMapSlot *binding_slots;
};

////////////////////////////////
//~ rjf: Evaluation Spaces

typedef U64 RD_EvalSpaceKind;
enum
{
  RD_EvalSpaceKind_CtrlEntity = E_SpaceKind_FirstUserDefined,
  RD_EvalSpaceKind_MetaQuery,
  RD_EvalSpaceKind_MetaCfg,
  RD_EvalSpaceKind_MetaCmd,
  RD_EvalSpaceKind_MetaTheme,
  RD_EvalSpaceKind_MetaCtrlEntity,
  RD_EvalSpaceKind_MetaUnattachedProcess,
};

////////////////////////////////
//~ rjf: View UI Hook Types

#define RD_VIEW_UI_FUNCTION_SIG(name) void name(E_Eval eval, Rng2F32 rect)
#define RD_VIEW_UI_FUNCTION_NAME(name) rd_view_ui__##name
#define RD_VIEW_UI_FUNCTION_DEF(name) internal RD_VIEW_UI_FUNCTION_SIG(RD_VIEW_UI_FUNCTION_NAME(name))
typedef RD_VIEW_UI_FUNCTION_SIG(RD_ViewUIFunctionType);

typedef struct RD_ViewUIRule RD_ViewUIRule;
struct RD_ViewUIRule
{
  String8 name;
  RD_ViewUIFunctionType *ui;
};

typedef struct RD_ViewUIRuleNode RD_ViewUIRuleNode;
struct RD_ViewUIRuleNode
{
  RD_ViewUIRuleNode *next;
  RD_ViewUIRule v;
};

typedef struct RD_ViewUIRuleSlot RD_ViewUIRuleSlot;
struct RD_ViewUIRuleSlot
{
  RD_ViewUIRuleNode *first;
  RD_ViewUIRuleNode *last;
};

typedef struct RD_ViewUIRuleMap RD_ViewUIRuleMap;
struct RD_ViewUIRuleMap
{
  RD_ViewUIRuleSlot *slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Drag/Drop Types

typedef enum RD_DragDropState
{
  RD_DragDropState_Null,
  RD_DragDropState_Dragging,
  RD_DragDropState_Dropping,
  RD_DragDropState_COUNT
}
RD_DragDropState;

////////////////////////////////
//~ rjf: Command Kind Types

typedef U32 RD_QueryFlags;
enum
{
  RD_QueryFlag_AllowFiles       = (1<<0),
  RD_QueryFlag_AllowFolders     = (1<<1),
  RD_QueryFlag_CodeInput        = (1<<2),
  RD_QueryFlag_KeepOldInput     = (1<<3),
  RD_QueryFlag_SelectOldInput   = (1<<4),
  RD_QueryFlag_Floating         = (1<<5),
  RD_QueryFlag_Required         = (1<<6),
};

typedef U32 RD_CmdKindFlags;
enum
{
  RD_CmdKindFlag_ListInUI      = (1<<0),
  RD_CmdKindFlag_ListInIPCDocs = (1<<1),
  RD_CmdKindFlag_ListInTab     = (1<<2),
  RD_CmdKindFlag_ListInTextPt  = (1<<3),
  RD_CmdKindFlag_ListInTextRng = (1<<4),
};

////////////////////////////////
//~ rjf: Autocompletion Cursor Info Type

typedef struct RD_AutocompCursorInfo RD_AutocompCursorInfo;
struct RD_AutocompCursorInfo
{
  String8 list_expr;
  String8 filter;
  Rng1U64 replaced_range;
  String8 callee_expr;
  MD_Node *arg_schema;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/raddbg.meta.h"

////////////////////////////////
//~ rjf: View State Types

typedef struct RD_ArenaExt RD_ArenaExt;
struct RD_ArenaExt
{
  RD_ArenaExt *next;
  Arena *arena;
};

typedef struct RD_ViewState RD_ViewState;
struct RD_ViewState
{
  // rjf: hash links & key
  RD_ViewState *hash_next;
  RD_ViewState *hash_prev;
  RD_CfgID cfg_id;
  
  // rjf: touch info
  U64 last_frame_index_touched;
  U64 last_frame_index_built;
  
  // rjf: loading indicator info
  F32 loading_t;
  F32 loading_t_target;
  U64 loading_progress_v;
  U64 loading_progress_v_target;
  
  // rjf: scroll position
  UI_ScrollPt2 scroll_pos;
  
  // rjf: eval visualization view state
  EV_View *ev_view;
  
  // rjf: view-lifetime allocation & user data extensions
  Arena *arena;
  U64 arena_reset_pos;
  RD_ArenaExt *first_arena_ext;
  RD_ArenaExt *last_arena_ext;
  void *user_data;
  
  // rjf: query state
  B32 query_is_open;
  TxtPt query_cursor;
  TxtPt query_mark;
  U8 query_buffer[KB(1)];
  U64 query_string_size;
  
  // rjf: contents are focused (disables query focus)
  B32 contents_are_focused;
};

typedef struct RD_ViewStateSlot RD_ViewStateSlot;
struct RD_ViewStateSlot
{
  RD_ViewState *first;
  RD_ViewState *last;
};

////////////////////////////////
//~ rjf: Vocabulary Map

typedef struct RD_VocabInfoMapNode RD_VocabInfoMapNode;
struct RD_VocabInfoMapNode
{
  RD_VocabInfoMapNode *single_next;
  RD_VocabInfoMapNode *plural_next;
  RD_VocabInfo v;
};

typedef struct RD_VocabInfoMapSlot RD_VocabInfoMapSlot;
struct RD_VocabInfoMapSlot
{
  RD_VocabInfoMapNode *first;
  RD_VocabInfoMapNode *last;
};

typedef struct RD_VocabInfoMap RD_VocabInfoMap;
struct RD_VocabInfoMap
{
  U64 single_slots_count;
  RD_VocabInfoMapSlot *single_slots;
  U64 plural_slots_count;
  RD_VocabInfoMapSlot *plural_slots;
};

////////////////////////////////
//~ rjf: Config Tree

typedef struct RD_Cfg RD_Cfg;
struct RD_Cfg
{
  RD_Cfg *first;
  RD_Cfg *last;
  RD_Cfg *next;
  RD_Cfg *prev;
  RD_Cfg *parent;
  RD_CfgID id;
  String8 string;
};

typedef struct RD_CfgNode RD_CfgNode;
struct RD_CfgNode
{
  RD_CfgNode *next;
  RD_CfgNode *prev;
  RD_Cfg *v;
};

typedef struct RD_CfgSlot RD_CfgSlot;
struct RD_CfgSlot
{
  RD_CfgNode *first;
  RD_CfgNode *last;
};

typedef struct RD_CfgList RD_CfgList;
struct RD_CfgList
{
  RD_CfgNode *first;
  RD_CfgNode *last;
  U64 count;
};

typedef struct RD_CfgArray RD_CfgArray;
struct RD_CfgArray
{
  RD_Cfg **v;
  U64 count;
};

typedef struct RD_CfgRec RD_CfgRec;
struct RD_CfgRec
{
  RD_Cfg *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Structured Locations, Parsed From Config Trees

typedef struct RD_Location RD_Location;
struct RD_Location
{
  String8 file_path;
  TxtPt pt;
  String8 expr;
};

////////////////////////////////
//~ rjf: Structured Panel Trees, Parsed From Config Trees

typedef struct RD_PanelNode RD_PanelNode;
struct RD_PanelNode
{
  // rjf: links data
  RD_PanelNode *first;
  RD_PanelNode *last;
  RD_PanelNode *next;
  RD_PanelNode *prev;
  RD_PanelNode *parent;
  U64 child_count;
  RD_Cfg *cfg;
  
  // rjf: split data
  Axis2 split_axis;
  F32 pct_of_parent;
  
  // rjf: tab params
  Side tab_side;
  
  // rjf: which tabs are attached
  RD_CfgList tabs;
  RD_Cfg *selected_tab;
};

typedef struct RD_PanelTree RD_PanelTree;
struct RD_PanelTree
{
  RD_PanelNode *root;
  RD_PanelNode *focused;
};

typedef struct RD_PanelNodeRec RD_PanelNodeRec;
struct RD_PanelNodeRec
{
  RD_PanelNode *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Command Types

typedef struct RD_Cmd RD_Cmd;
struct RD_Cmd
{
  String8 name;
  RD_Regs *regs;
};

typedef struct RD_CmdNode RD_CmdNode;
struct RD_CmdNode
{
  RD_CmdNode *next;
  RD_CmdNode *prev;
  RD_Cmd cmd;
};

typedef struct RD_CmdList RD_CmdList;
struct RD_CmdList
{
  RD_CmdNode *first;
  RD_CmdNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Context Register Types

typedef struct RD_RegsNode RD_RegsNode;
struct RD_RegsNode
{
  RD_RegsNode *next;
  RD_Regs v;
};

////////////////////////////////
//~ rjf: Structured Theme Types, Parsed From Config

typedef enum RD_FontSlot
{
  RD_FontSlot_Main,
  RD_FontSlot_Code,
  RD_FontSlot_Icons,
  RD_FontSlot_COUNT
}
RD_FontSlot;

////////////////////////////////
//~ rjf: Per-Window State

typedef struct RD_WindowState RD_WindowState;
struct RD_WindowState
{
  // rjf: links & metadata
  RD_WindowState *order_next;
  RD_WindowState *order_prev;
  RD_WindowState *hash_next;
  RD_WindowState *hash_prev;
  RD_CfgID cfg_id;
  U64 frames_alive;
  U64 last_frame_index_touched;
  
  // rjf: top-level info & handles
  Arena *arena;
  OS_Handle os;
  R_Handle r;
  UI_State *ui;
  F32 last_dpi;
  B32 window_temporarily_focused_ipc;
  B32 window_layout_reset;
  Rng2F32 last_window_rect;
  
  // rjf: theme (recomputed each frame)
  UI_Theme *theme;
  Vec4F32 theme_code_colors[RD_CodeColorSlot_COUNT];
  
  // rjf: font raster flags (recomputed each frame)
  FNT_RasterFlags font_slot_raster_flags[RD_FontSlot_COUNT];
  
  // rjf: dev interface state
  B32 dev_menu_is_open;
  
  // rjf: menu bar state
  B32 menu_bar_focused;
  B32 menu_bar_focused_on_press;
  B32 menu_bar_key_held;
  B32 menu_bar_focus_press_started;
  
  // rjf: drop-completion state
  Arena *drop_completion_arena;
  String8List drop_completion_paths;
  
  // rjf: query state
  B32 query_is_active;
  Arena *query_arena;
  RD_Regs *query_regs;
  RD_CfgID query_view_id;
  RD_CfgID query_last_view_id;
  
  // rjf: hover eval state
  B32 hover_eval_focused;
  Arena *hover_eval_arena;
  Vec2F32 hover_eval_spawn_pos;
  String8 hover_eval_string;
  U64 hover_eval_firstt_us;
  U64 hover_eval_lastt_us;
  
  // rjf: autocompletion state
  U64 autocomp_last_frame_index;
  Arena *autocomp_arena;
  RD_Regs *autocomp_regs;
  RD_AutocompCursorInfo autocomp_cursor_info;
  
  // rjf: error state
  U8 error_buffer[512];
  U64 error_string_size;
  F32 error_t;
  
  // rjf: per-frame ui events state
  UI_EventList ui_events;
  
  // rjf: per-frame drawing state
  DR_Bucket *draw_bucket;
};

typedef struct RD_WindowStateSlot RD_WindowStateSlot;
struct RD_WindowStateSlot
{
  RD_WindowState *first;
  RD_WindowState *last;
};

////////////////////////////////
//~ rjf: Main Per-Process Graphical State

read_only global U64 rd_name_bucket_chunk_sizes[] =
{
  16,
  64,
  256,
  1024,
  4096,
  16384,
  65536,
  0xffffffffffffffffull,
};

typedef struct RD_NameChunkNode RD_NameChunkNode;
struct RD_NameChunkNode
{
  RD_NameChunkNode *next;
  U64 size;
};

typedef struct RD_AmbiguousPathNode RD_AmbiguousPathNode;
struct RD_AmbiguousPathNode
{
  RD_AmbiguousPathNode *next;
  String8 name;
  String8List paths;
};

typedef struct RD_State RD_State;
struct RD_State
{
  // rjf: basics
  Arena *arena;
  B32 quit;
  B32 quit_after_success;
  S32 frame_depth;
  U64 frame_eval_memread_endt_us;
  
  // rjf: config bucket paths
  Arena *user_path_arena;
  String8 user_path;
  Arena *project_path_arena;
  String8 project_path;
  Arena *theme_path_arena;
  String8 theme_path;
  
  // rjf: unpacked settings (cached, because they need to be used
  // earlier than setting evaluation is legal in a frame)
  B32 alt_menu_bar_enabled;
  B32 use_default_stl_type_views;
  B32 use_default_ue_type_views;
  
  // rjf: animation rates
  F32 catchall_animation_rate;
  F32 menu_animation_rate;
  F32 menu_animation_rate__slow;
  F32 entity_alive_animation_rate;
  F32 rich_hover_animation_rate;
  F32 scrolling_animation_rate;
  F32 tooltip_animation_rate;
  
  // rjf: serialized config debug string keys
  U128 user_cfg_string_key;
  U128 project_cfg_string_key;
  U128 cmdln_cfg_string_key;
  U128 transient_cfg_string_key;
  
  // rjf: schema table
  MD_NodePtrList *schemas;
  
  // rjf: default theme table
  MD_Node *theme_preset_trees[RD_ThemePreset_COUNT];
  
  // rjf: vocab table
  RD_VocabInfoMap vocab_info_map;
  
  // rjf: log
  Log *log;
  String8 log_path;
  
  // rjf: frame history info
  U64 frame_index;
  Arena *frame_arenas[2];
  U64 frame_time_us_history[64];
  U64 num_frames_requested;
  F64 time_in_seconds;
  U64 time_in_us;
  
  // rjf: frame parameters
  F32 frame_dt;
  DI_Scope *frame_di_scope;
  
  // rjf: dbgi match store
  DI_MatchStore *match_store;
  
  // rjf: evaluation cache
  E_Cache *eval_cache;
  
  // rjf: ambiguous path table (constructed from-scratch each frame)
  U64 ambiguous_path_slots_count;
  RD_AmbiguousPathNode **ambiguous_path_slots;
  
  // rjf: key map (constructed from-scratch each frame)
  RD_KeyMap *key_map;
  
  // rjf: slot -> font tag map (constructed from-scratch each frame)
  FNT_Tag font_slot_table[RD_FontSlot_COUNT];
  
  // rjf: meta name -> eval type key map (constructed from-scratch each frame)
  E_String2TypeKeyMap *meta_name2type_map;
  
  // rjf: name -> view ui map (constructed from-scratch each frame)
  RD_ViewUIRuleMap *view_ui_rule_map;
  
  // rjf: registers stack
  RD_RegsNode base_regs;
  RD_RegsNode *top_regs;
  
  // rjf: autosave state
  F32 seconds_until_autosave;
  
  // rjf: commands
  Arena *cmds_arenas[2];
  RD_CmdList cmds[2];
  U64 cmds_gen;
  
  // rjf: popup state
  UI_Key popup_key;
  B32 popup_active;
  F32 popup_t;
  Arena *popup_arena;
  RD_CmdList popup_cmds;
  String8 popup_title;
  String8 popup_desc;
  
  // rjf: text editing mode state
  B32 text_edit_mode;
  
  // rjf: contextual hover info
  RD_Regs *hover_regs;
  RD_RegSlot hover_regs_slot;
  RD_Regs *next_hover_regs;
  RD_RegSlot next_hover_regs_slot;
  
  // rjf: icon texture
  R_Handle icon_texture;
  
  // rjf: fixed ui keys
  UI_Key drop_completion_key;
  UI_Key ctx_menu_key;
  
  // rjf: drag/drop state
  Arena *drag_drop_arena;
  RD_Regs *drag_drop_regs;
  RD_RegSlot drag_drop_regs_slot;
  RD_DragDropState drag_drop_state;
  
  // rjf: cfg state
  RD_NameChunkNode *free_name_chunks[ArrayCount(rd_name_bucket_chunk_sizes)];
  RD_Cfg *free_cfg;
  RD_Cfg *root_cfg;
  U64 cfg_id_slots_count;
  RD_CfgSlot *cfg_id_slots;
  RD_CfgNode *free_cfg_id_node;
  U64 cfg_id_gen;
  RD_CfgID cfg_last_accessed_id;
  RD_Cfg *cfg_last_accessed;
  U64 cfg_change_gen;
  
  // rjf: window state cache
  U64 window_state_slots_count;
  RD_WindowStateSlot *window_state_slots;
  RD_WindowState *free_window_state;
  RD_CfgID last_focused_window;
  RD_WindowState *first_window_state;
  RD_WindowState *last_window_state;
  RD_CfgID window_state_last_accessed_id;
  RD_WindowState *window_state_last_accessed;
  
  // rjf: view state cache
  U64 view_state_slots_count;
  RD_ViewStateSlot *view_state_slots;
  RD_ViewState *free_view_state;
  RD_CfgID view_state_last_accessed_id;
  RD_ViewState *view_state_last_accessed;
  
  // rjf: bind change
  Arena *bind_change_arena;
  B32 bind_change_active;
  RD_CfgID bind_change_binding_id;
  String8 bind_change_cmd_name;
};

////////////////////////////////
//~ rjf: Globals

read_only global RD_VocabInfo rd_nil_vocab_info = {0};

read_only global RD_Cfg rd_nil_cfg =
{
  &rd_nil_cfg,
  &rd_nil_cfg,
  &rd_nil_cfg,
  &rd_nil_cfg,
  &rd_nil_cfg,
};

read_only global RD_PanelNode rd_nil_panel_node =
{
  &rd_nil_panel_node,
  &rd_nil_panel_node,
  &rd_nil_panel_node,
  &rd_nil_panel_node,
  &rd_nil_panel_node,
  0,
  &rd_nil_cfg,
  .selected_tab = &rd_nil_cfg,
};

read_only global RD_CmdKindInfo rd_nil_cmd_kind_info = {0};

RD_VIEW_UI_FUNCTION_DEF(null);
read_only global RD_ViewUIRule rd_nil_view_ui_rule =
{
  {0},
  RD_VIEW_UI_FUNCTION_NAME(null),
};

read_only global RD_ViewState rd_nil_view_state =
{
  &rd_nil_view_state,
  &rd_nil_view_state,
};

read_only global RD_WindowState rd_nil_window_state =
{
  &rd_nil_window_state,
  &rd_nil_window_state,
  &rd_nil_window_state,
  &rd_nil_window_state,
};

global RD_State *rd_state = 0;
global RD_CfgID rd_last_drag_drop_panel = 0;
global RD_CfgID rd_last_drag_drop_prev_tab = 0;

////////////////////////////////
//~ rjf: Config ID Type Functions

internal void rd_cfg_id_list_push(Arena *arena, RD_CfgIDList *list, RD_CfgID id);
internal RD_CfgIDList rd_cfg_id_list_copy(Arena *arena, RD_CfgIDList *src);

////////////////////////////////
//~ rjf: Registers Type Functions

internal void rd_regs_copy_contents(Arena *arena, RD_Regs *dst, RD_Regs *src);
internal RD_Regs *rd_regs_copy(Arena *arena, RD_Regs *src);

////////////////////////////////
//~ rjf: Commands Type Functions

internal void rd_cmd_list_push_new(Arena *arena, RD_CmdList *cmds, String8 name, RD_Regs *regs);

////////////////////////////////
//~ rjf: View UI Rule Functions

internal RD_ViewUIRuleMap *rd_view_ui_rule_map_make(Arena *arena, U64 slots_count);
internal void rd_view_ui_rule_map_insert(Arena *arena, RD_ViewUIRuleMap *map, String8 string, RD_ViewUIFunctionType *ui);

internal RD_ViewUIRule *rd_view_ui_rule_from_string(String8 string);

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32 rd_drag_is_active(void);
internal void rd_drag_begin(RD_RegSlot slot);
internal B32 rd_drag_drop(void);
internal void rd_drag_kill(void);

internal void rd_set_hover_regs(RD_RegSlot slot);
internal RD_Regs *rd_get_hover_regs(void);

////////////////////////////////
//~ rjf: Name Allocation

internal U64 rd_name_bucket_num_from_string_size(U64 size);
internal String8 rd_name_alloc(String8 string);
internal void rd_name_release(String8 string);

////////////////////////////////
//~ rjf: Config Tree Functions

internal RD_Cfg *rd_cfg_alloc(void);
internal void rd_cfg_release(RD_Cfg *cfg);
internal void rd_cfg_release_all_children(RD_Cfg *cfg);
internal RD_Cfg *rd_cfg_from_id(RD_CfgID id);
internal RD_Cfg *rd_cfg_new(RD_Cfg *parent, String8 string);
internal RD_Cfg *rd_cfg_newf(RD_Cfg *parent, char *fmt, ...);
internal RD_Cfg *rd_cfg_new_replace(RD_Cfg *parent, String8 string);
internal RD_Cfg *rd_cfg_new_replacef(RD_Cfg *parent, char *fmt, ...);
internal RD_Cfg *rd_cfg_deep_copy(RD_Cfg *src_root);
internal void rd_cfg_equip_string(RD_Cfg *cfg, String8 string);
internal void rd_cfg_equip_stringf(RD_Cfg *cfg, char *fmt, ...);
internal void rd_cfg_insert_child(RD_Cfg *parent, RD_Cfg *prev_child, RD_Cfg *new_child);
internal void rd_cfg_unhook(RD_Cfg *parent, RD_Cfg *child);
internal RD_Cfg *rd_cfg_child_from_string(RD_Cfg *parent, String8 string);
internal RD_Cfg *rd_cfg_child_from_string_or_alloc(RD_Cfg *parent, String8 string);
internal RD_Cfg *rd_cfg_child_from_string_or_parent(RD_Cfg *parent, String8 string);
internal RD_CfgList rd_cfg_child_list_from_string(Arena *arena, RD_Cfg *parent, String8 string);
internal RD_CfgList rd_cfg_top_level_list_from_string(Arena *arena, String8 string);
internal RD_CfgArray rd_cfg_array_from_list(Arena *arena, RD_CfgList *list);
internal RD_CfgList rd_cfg_tree_list_from_string(Arena *arena, String8 root_path, String8 string);
internal String8 rd_string_from_cfg_tree(Arena *arena, String8 root_path, RD_Cfg *cfg);
internal RD_CfgRec rd_cfg_rec__depth_first(RD_Cfg *root, RD_Cfg *cfg);
internal void rd_cfg_list_push(Arena *arena, RD_CfgList *list, RD_Cfg *cfg);
internal void rd_cfg_list_push_front(Arena *arena, RD_CfgList *list, RD_Cfg *cfg);
#define rd_cfg_list_first(list) ((list)->count ? (list)->first->v : &rd_nil_cfg)
#define rd_cfg_list_last(list)  ((list)->count ? (list)->last->v  : &rd_nil_cfg)

internal RD_PanelTree rd_panel_tree_from_cfg(Arena *arena, RD_Cfg *cfg);
internal RD_PanelNodeRec rd_panel_node_rec__depth_first(RD_PanelNode *root, RD_PanelNode *panel, U64 sib_off, U64 child_off);
#define rd_panel_node_rec__depth_first_pre(root, p)     rd_panel_node_rec__depth_first((root), (p), OffsetOf(RD_PanelNode, next), OffsetOf(RD_PanelNode, first))
#define rd_panel_node_rec__depth_first_pre_rev(root, p) rd_panel_node_rec__depth_first((root), (p), OffsetOf(RD_PanelNode, prev), OffsetOf(RD_PanelNode, last))
internal RD_PanelNode *rd_panel_node_from_tree_cfg(RD_PanelNode *root, RD_Cfg *cfg);
internal Rng2F32 rd_target_rect_from_panel_node_child(Rng2F32 parent_rect, RD_PanelNode *parent, RD_PanelNode *panel);
internal Rng2F32 rd_target_rect_from_panel_node(Rng2F32 root_rect, RD_PanelNode *root, RD_PanelNode *panel);

internal B32 rd_cfg_is_project_filtered(RD_Cfg *cfg);
internal RD_KeyMapNodePtrList rd_key_map_node_ptr_list_from_name(Arena *arena, String8 string);
internal RD_KeyMapNodePtrList rd_key_map_node_ptr_list_from_binding(Arena *arena, RD_Binding binding);

internal Vec4F32 rd_hsva_from_cfg(RD_Cfg *cfg);
internal Vec4F32 rd_color_from_cfg(RD_Cfg *cfg);

internal B32 rd_disabled_from_cfg(RD_Cfg *cfg);
internal RD_Location rd_location_from_cfg(RD_Cfg *cfg);
internal String8 rd_label_from_cfg(RD_Cfg *cfg);
internal String8 rd_expr_from_cfg(RD_Cfg *cfg);
internal String8 rd_path_from_cfg(RD_Cfg *cfg);
internal D_Target rd_target_from_cfg(Arena *arena, RD_Cfg *cfg);

internal MD_NodePtrList rd_schemas_from_name(String8 name);
internal String8 rd_default_setting_from_names(String8 schema_name, String8 setting_name);

internal String8 rd_setting_from_name(String8 name);
internal B32 rd_setting_b32_from_name(String8 name);
internal U64 rd_setting_u64_from_name(String8 name);
internal F32 rd_setting_f32_from_name(String8 name);

internal RD_Cfg *rd_immediate_cfg_from_key(String8 string);
internal RD_Cfg *rd_immediate_cfg_from_keyf(char *fmt, ...);

internal String8 rd_mapped_from_file_path(Arena *arena, String8 file_path);
internal String8List rd_possible_overrides_from_file_path(Arena *arena, String8 file_path);

////////////////////////////////
//~ rjf: Control Entity Info Extraction

internal Vec4F32 rd_color_from_ctrl_entity(CTRL_Entity *entity);
internal String8 rd_name_from_ctrl_entity(Arena *arena, CTRL_Entity *entity);

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: cfg <-> eval space
internal RD_Cfg *rd_cfg_from_eval_space(E_Space space);
internal E_Space rd_eval_space_from_cfg(RD_Cfg *cfg);

//- rjf: ctrl entity <-> eval space
internal CTRL_Entity *rd_ctrl_entity_from_eval_space(E_Space space);
internal E_Space rd_eval_space_from_ctrl_entity(CTRL_Entity *entity, E_SpaceKind kind);

//- rjf: command name <-> eval space
internal String8 rd_cmd_name_from_eval(E_Eval eval);

//- rjf: eval space reads/writes
internal U64 rd_eval_space_gen(void *u, E_Space space);
internal B32 rd_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range);
internal B32 rd_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range);

//- rjf: asynchronous streamed reads -> hashes from spaces
internal U128 rd_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated);

//- rjf: space -> entire range
internal Rng1U64 rd_whole_range_from_eval_space(E_Space space);

////////////////////////////////
//~ rjf: Evaluation Visualization

//- rjf: writing values back to child processes
internal B32 rd_commit_eval_value_string(E_Eval dst_eval, String8 string);

//- rjf: eval <-> file path
internal String8 rd_file_path_from_eval(Arena *arena, E_Eval eval);
internal String8 rd_file_path_from_eval_string(Arena *arena, String8 string);
internal String8 rd_eval_string_from_file_path(Arena *arena, String8 string);

//- rjf: eval -> query
internal String8 rd_query_from_eval_string(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: View Functions

internal RD_Cfg *rd_view_from_eval(RD_Cfg *parent, E_Eval eval);
internal RD_ViewState *rd_view_state_from_cfg(RD_Cfg *cfg);
internal void rd_view_ui(Rng2F32 rect);

////////////////////////////////
//~ rjf: View Building API

//- rjf: view info extraction
internal Arena *rd_view_arena(void);
internal UI_ScrollPt2 rd_view_scroll_pos(void);
internal EV_View *rd_view_eval_view(void);
internal String8 rd_view_query_cmd(void);
internal String8 rd_view_query_input(void);
internal String8 rd_view_setting_from_name(String8 string);
internal E_Value rd_view_setting_value_from_name(String8 string);
internal B32 rd_view_setting_b32_from_name(String8 string);
internal U64 rd_view_setting_u64_from_name(String8 string);
internal F32 rd_view_setting_f32_from_name(String8 string);

//- rjf: evaluation & tag (a view's 'call') parameter extraction
internal TXT_LangKind rd_lang_kind_from_eval(E_Eval eval);
internal Arch rd_arch_from_eval(E_Eval eval);

//- rjf: pushing/attaching view resources
internal void *rd_view_state_by_size(U64 size);
#define rd_view_state(T) (T *)rd_view_state_by_size(sizeof(T))
internal Arena *rd_push_view_arena(void);

//- rjf: storing view-attached state
internal void rd_store_view_expr_string(String8 string);
internal void rd_store_view_loading_info(B32 is_loading, U64 progress_u64, U64 progress_u64_target);
internal void rd_store_view_scroll_pos(UI_ScrollPt2 pos);
internal void rd_store_view_param(String8 key, String8 value);
internal void rd_store_view_paramf(String8 key, char *fmt, ...);
#define rd_store_view_param_f32(key, f32) rd_store_view_paramf((key), "%ff", (f32))
#define rd_store_view_param_s64(key, s64) rd_store_view_paramf((key), "%I64d", (s64))
#define rd_store_view_param_u64(key, u64) rd_store_view_paramf((key), "0x%I64x", (u64))

////////////////////////////////
//~ rjf: Window Functions

internal String8 rd_push_window_title(Arena *arena);
internal RD_Cfg *rd_window_from_cfg(RD_Cfg *cfg);
internal RD_WindowState *rd_window_state_from_cfg(RD_Cfg *cfg);
internal RD_WindowState *rd_window_state_from_os_handle(OS_Handle os);
internal void rd_window_frame(void);

////////////////////////////////
//~ rjf: Eval Visualization

internal String8 rd_value_string_from_eval(Arena *arena, String8 filter, EV_StringParams *params, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval);

////////////////////////////////
//~ rjf: Hover Eval

internal void rd_set_hover_eval(Vec2F32 pos, String8 string);

////////////////////////////////
//~ rjf: Autocompletion Lister

internal void rd_set_autocomp_regs_(E_Eval dst_eval, RD_Regs *regs);
#define rd_set_autocomp_regs(dst_eval, ...) rd_set_autocomp_regs_((dst_eval), &(RD_Regs){rd_regs_lit_init_top __VA_ARGS__})

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: colors
internal MD_Node *rd_theme_tree_from_name(Arena *arena, HS_Scope *scope, String8 theme_name);
internal Vec4F32 rd_rgba_from_code_color_slot(RD_CodeColorSlot slot);
internal RD_CodeColorSlot rd_code_color_slot_from_txt_token_kind(TXT_TokenKind kind);
internal RD_CodeColorSlot rd_code_color_slot_from_txt_token_kind_lookup_string(TXT_TokenKind kind, String8 string);

//- rjf: fonts
internal F32 rd_font_size(void);
internal FNT_Tag rd_font_from_slot(RD_FontSlot slot);
internal FNT_RasterFlags rd_raster_flags_from_slot(RD_FontSlot slot);

////////////////////////////////
//~ rjf: Process Control Info Stringification

internal String8 rd_string_from_exception_code(U32 code);
internal DR_FStrList rd_stop_explanation_fstrs_from_ctrl_event(Arena *arena, CTRL_Event *event);

////////////////////////////////
//~ rjf: Vocab Info Lookups

internal RD_VocabInfo *rd_vocab_info_from_code_name(String8 code_name);
internal RD_VocabInfo *rd_vocab_info_from_code_name_plural(String8 code_name_plural);
#define rd_plural_from_code_name(code_name) (rd_vocab_info_from_code_name(code_name)->code_name_plural)
#define rd_display_from_code_name(code_name) (rd_vocab_info_from_code_name(code_name)->display_name)
#define rd_display_plural_from_code_name(code_name) (rd_vocab_info_from_code_name(code_name)->display_name_plural)
#define rd_icon_kind_from_code_name(code_name) (rd_vocab_info_from_code_name(code_name)->icon_kind)
#define rd_singular_from_code_name_plural(code_name_plural) (rd_vocab_info_from_code_name_plural(code_name_plural)->code_name)

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void rd_request_frame(void);

////////////////////////////////
//~ rjf: Main State Accessors

//- rjf: per-frame arena
internal Arena *rd_frame_arena(void);

////////////////////////////////
//~ rjf: Registers

#define rd_regs() (&rd_state->top_regs->v)
#define rd_base_regs() (&rd_state->base_regs.v)
internal RD_Regs *rd_push_regs_(RD_Regs *regs);
#define rd_push_regs(...) rd_push_regs_(&(RD_Regs){rd_regs_lit_init_top __VA_ARGS__})
internal RD_Regs *rd_pop_regs(void);
#define RD_RegsScope(...) DeferLoop(rd_push_regs(__VA_ARGS__), rd_pop_regs())
internal void rd_regs_fill_slot_from_string(RD_RegSlot slot, String8 string);

////////////////////////////////
//~ rjf: Commands

//- rjf: name -> info
internal RD_CmdKind rd_cmd_kind_from_string(String8 string);
internal RD_CmdKindInfo *rd_cmd_kind_info_from_string(String8 string);

//- rjf: pushing
internal void rd_push_cmd(String8 name, RD_Regs *regs);
#define rd_cmd(kind, ...) rd_push_cmd(rd_cmd_kind_info_table[kind].string, &(RD_Regs){rd_regs_lit_init_top __VA_ARGS__})

//- rjf: iterating
internal B32 rd_next_cmd(RD_Cmd **cmd);
internal B32 rd_next_view_cmd(RD_Cmd **cmd);

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void rd_init(CmdLine *cmdln);
internal void rd_frame(void);

#endif // RADDBG_CORE_H
