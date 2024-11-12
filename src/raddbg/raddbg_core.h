// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_CORE_H
#define RADDBG_CORE_H

////////////////////////////////
//~ rjf: Handles

typedef struct RD_Handle RD_Handle;
struct RD_Handle
{
  U64 u64[2];
};

typedef struct RD_HandleNode RD_HandleNode;
struct RD_HandleNode
{
  RD_HandleNode *next;
  RD_HandleNode *prev;
  RD_Handle handle;
};

typedef struct RD_HandleList RD_HandleList;
struct RD_HandleList
{
  RD_HandleNode *first;
  RD_HandleNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Evaluation Spaces

typedef U64 RD_EvalSpaceKind;
enum
{
  RD_EvalSpaceKind_CtrlEntity = E_SpaceKind_FirstUserDefined,
  RD_EvalSpaceKind_MetaEntity,
  RD_EvalSpaceKind_MetaCtrlEntity,
  RD_EvalSpaceKind_MetaCollection,
};

////////////////////////////////
//~ rjf: Entity Kind Flags

typedef U32 RD_EntityKindFlags;
enum
{
  //- rjf: allowed operations
  RD_EntityKindFlag_CanDelete                = (1<<0),
  RD_EntityKindFlag_CanFreeze                = (1<<1),
  RD_EntityKindFlag_CanEdit                  = (1<<2),
  RD_EntityKindFlag_CanRename                = (1<<3),
  RD_EntityKindFlag_CanEnable                = (1<<4),
  RD_EntityKindFlag_CanCondition             = (1<<5),
  RD_EntityKindFlag_CanDuplicate             = (1<<6),
  
  //- rjf: name categorization
  RD_EntityKindFlag_NameIsCode               = (1<<7),
  RD_EntityKindFlag_NameIsPath               = (1<<8),
  
  //- rjf: lifetime categorization
  RD_EntityKindFlag_UserDefinedLifetime      = (1<<9),
  
  //- rjf: serialization
  RD_EntityKindFlag_IsSerializedToConfig     = (1<<10),
};

////////////////////////////////
//~ rjf: Entity Flags

typedef U32 RD_EntityFlags;
enum
{
  //- rjf: allocationless, simple equipment
  RD_EntityFlag_HasTextPoint      = (1<<0),
  RD_EntityFlag_HasEntityHandle   = (1<<2),
  RD_EntityFlag_HasU64            = (1<<4),
  RD_EntityFlag_HasColor          = (1<<6),
  RD_EntityFlag_DiesOnRunStop     = (1<<8),
  
  //- rjf: ctrl entity equipment
  RD_EntityFlag_HasCtrlHandle     = (1<<9),
  RD_EntityFlag_HasArch           = (1<<10),
  RD_EntityFlag_HasCtrlID         = (1<<11),
  RD_EntityFlag_HasStackBase      = (1<<12),
  RD_EntityFlag_HasTLSRoot        = (1<<13),
  RD_EntityFlag_HasVAddrRng       = (1<<14),
  RD_EntityFlag_HasVAddr          = (1<<15),
  
  //- rjf: file properties
  RD_EntityFlag_IsFolder          = (1<<16),
  RD_EntityFlag_IsMissing         = (1<<17),
  
  //- rjf: deletion
  RD_EntityFlag_MarkedForDeletion = (1<<31),
};

////////////////////////////////
//~ rjf: Binding Types

typedef struct RD_Binding RD_Binding;
struct RD_Binding
{
  OS_Key key;
  OS_Modifiers modifiers;
};

typedef struct RD_BindingNode RD_BindingNode;
struct RD_BindingNode
{
  RD_BindingNode *next;
  RD_Binding binding;
};

typedef struct RD_BindingList RD_BindingList;
struct RD_BindingList
{
  RD_BindingNode *first;
  RD_BindingNode *last;
  U64 count;
};

typedef struct RD_StringBindingPair RD_StringBindingPair;
struct RD_StringBindingPair
{
  String8 string;
  RD_Binding binding;
};

////////////////////////////////
//~ rjf: Key Map Types

typedef struct RD_KeyMapNode RD_KeyMapNode;
struct RD_KeyMapNode
{
  RD_KeyMapNode *hash_next;
  RD_KeyMapNode *hash_prev;
  String8 name;
  RD_Binding binding;
};

typedef struct RD_KeyMapSlot RD_KeyMapSlot;
struct RD_KeyMapSlot
{
  RD_KeyMapNode *first;
  RD_KeyMapNode *last;
};

////////////////////////////////
//~ rjf: Setting Types

typedef struct RD_SettingVal RD_SettingVal;
struct RD_SettingVal
{
  B32 set;
  S32 s32;
};

////////////////////////////////
//~ rjf: View Rule Info Types

typedef U32 RD_ViewRuleInfoFlags;
enum
{
  RD_ViewRuleInfoFlag_ShowInDocs                 = (1<<0),
  RD_ViewRuleInfoFlag_CanFilter                  = (1<<1),
  RD_ViewRuleInfoFlag_FilterIsCode               = (1<<2),
  RD_ViewRuleInfoFlag_TypingAutomaticallyFilters = (1<<3),
  RD_ViewRuleInfoFlag_CanUseInWatchTable         = (1<<4),
  RD_ViewRuleInfoFlag_CanFillValueCell           = (1<<5),
  RD_ViewRuleInfoFlag_CanExpand                  = (1<<6),
  RD_ViewRuleInfoFlag_ProjectFiltered            = (1<<7),
};

#define RD_VIEW_RULE_UI_FUNCTION_SIG(name) void name(String8 string, MD_Node *params, Rng2F32 rect)
#define RD_VIEW_RULE_UI_FUNCTION_NAME(name) rd_view_rule_ui_##name
#define RD_VIEW_RULE_UI_FUNCTION_DEF(name) internal RD_VIEW_RULE_UI_FUNCTION_SIG(RD_VIEW_RULE_UI_FUNCTION_NAME(name))
typedef RD_VIEW_RULE_UI_FUNCTION_SIG(RD_ViewRuleUIFunctionType);

////////////////////////////////
//~ rjf: View Types

typedef struct RD_View RD_View;

typedef struct RD_ArenaExt RD_ArenaExt;
struct RD_ArenaExt
{
  RD_ArenaExt *next;
  Arena *arena;
};

typedef struct RD_TransientViewNode RD_TransientViewNode;
struct RD_TransientViewNode
{
  RD_TransientViewNode *next;
  RD_TransientViewNode *prev;
  EV_Key key;
  RD_View *view;
  Arena *initial_params_arena;
  MD_Node *initial_params;
  U64 first_frame_index_touched;
  U64 last_frame_index_touched;
};

typedef struct RD_TransientViewSlot RD_TransientViewSlot;
struct RD_TransientViewSlot
{
  RD_TransientViewNode *first;
  RD_TransientViewNode *last;
};

typedef struct RD_View RD_View;
struct RD_View
{
  // rjf: allocation links (for iterating all views)
  RD_View *alloc_next;
  RD_View *alloc_prev;
  
  // rjf: ownership links ('owners' can have lists of views)
  RD_View *order_next;
  RD_View *order_prev;
  
  // rjf: transient view children
  RD_View *first_transient;
  RD_View *last_transient;
  
  // rjf: view specification info
  struct RD_ViewRuleInfo *spec;
  
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
  RD_ArenaExt *first_arena_ext;
  RD_ArenaExt *last_arena_ext;
  U64 transient_view_slots_count;
  RD_TransientViewSlot *transient_view_slots;
  RD_TransientViewNode *free_transient_view_node;
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

////////////////////////////////
//~ rjf: Panel Types

typedef struct RD_Panel RD_Panel;
struct RD_Panel
{
  // rjf: tree links/data
  RD_Panel *first;
  RD_Panel *last;
  RD_Panel *next;
  RD_Panel *prev;
  RD_Panel *parent;
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
  RD_View *first_tab_view;
  RD_View *last_tab_view;
  U64 tab_view_count;
  RD_Handle selected_tab_view;
};

typedef struct RD_PanelRec RD_PanelRec;
struct RD_PanelRec
{
  RD_Panel *next;
  int push_count;
  int pop_count;
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
  RD_QueryFlag_Required         = (1<<5),
};

typedef U32 RD_CmdKindFlags;
enum
{
  RD_CmdKindFlag_ListInUI      = (1<<0),
  RD_CmdKindFlag_ListInIPCDocs = (1<<1),
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/raddbg.meta.h"

////////////////////////////////
//~ rjf: Config Types

typedef struct RD_CfgTree RD_CfgTree;
struct RD_CfgTree
{
  RD_CfgTree *next;
  RD_CfgSrc source;
  MD_Node *root;
};

typedef struct RD_CfgVal RD_CfgVal;
struct RD_CfgVal
{
  RD_CfgVal *hash_next;
  RD_CfgVal *linear_next;
  RD_CfgTree *first;
  RD_CfgTree *last;
  U64 insertion_stamp;
  String8 string;
};

typedef struct RD_CfgSlot RD_CfgSlot;
struct RD_CfgSlot
{
  RD_CfgVal *first;
};

typedef struct RD_CfgTable RD_CfgTable;
struct RD_CfgTable
{
  U64 slot_count;
  RD_CfgSlot *slots;
  U64 insertion_stamp_counter;
  RD_CfgVal *first_val;
  RD_CfgVal *last_val;
};

////////////////////////////////
//~ rjf: Entity Types

typedef U64 RD_EntityID;

typedef struct RD_Entity RD_Entity;
struct RD_Entity
{
  // rjf: tree links
  RD_Entity *first;
  RD_Entity *last;
  RD_Entity *next;
  RD_Entity *prev;
  RD_Entity *parent;
  
  // rjf: metadata
  RD_EntityKind kind;
  RD_EntityFlags flags;
  RD_EntityID id;
  U64 gen;
  U64 alloc_time_us;
  
  // rjf: basic equipment
  TxtPt text_point;
  RD_Handle entity_handle;
  B32 disabled;
  U64 u64;
  Vec4F32 color_hsva;
  RD_CfgSrc cfg_src;
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

typedef struct RD_EntityNode RD_EntityNode;
struct RD_EntityNode
{
  RD_EntityNode *next;
  RD_Entity *entity;
};

typedef struct RD_EntityList RD_EntityList;
struct RD_EntityList
{
  RD_EntityNode *first;
  RD_EntityNode *last;
  U64 count;
};

typedef struct RD_EntityArray RD_EntityArray;
struct RD_EntityArray
{
  RD_Entity **v;
  U64 count;
};

typedef struct RD_EntityRec RD_EntityRec;
struct RD_EntityRec
{
  RD_Entity *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ rjf: Entity Evaluation Types

typedef struct RD_EntityEval RD_EntityEval;
struct RD_EntityEval
{
  B64 enabled;
  U64 hit_count;
  U64 label_off;
  U64 location_off;
  U64 condition_off;
};

////////////////////////////////
//~ rjf: Entity Fuzzy Listing Types

typedef struct RD_EntityFuzzyItem RD_EntityFuzzyItem;
struct RD_EntityFuzzyItem
{
  RD_Entity *entity;
  FuzzyMatchRangeList matches;
};

typedef struct RD_EntityFuzzyItemArray RD_EntityFuzzyItemArray;
struct RD_EntityFuzzyItemArray
{
  RD_EntityFuzzyItem *v;
  U64 count;
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
//~ rjf: Theme Types

typedef struct RD_Theme RD_Theme;
struct RD_Theme
{
  Vec4F32 colors[RD_ThemeColor_COUNT];
};

typedef enum RD_FontSlot
{
  RD_FontSlot_Main,
  RD_FontSlot_Code,
  RD_FontSlot_Icons,
  RD_FontSlot_COUNT
}
RD_FontSlot;

typedef enum RD_PaletteCode
{
  RD_PaletteCode_Base,
  RD_PaletteCode_MenuBar,
  RD_PaletteCode_Floating,
  RD_PaletteCode_ImplicitButton,
  RD_PaletteCode_PlainButton,
  RD_PaletteCode_PositivePopButton,
  RD_PaletteCode_NegativePopButton,
  RD_PaletteCode_NeutralPopButton,
  RD_PaletteCode_ScrollBarButton,
  RD_PaletteCode_Tab,
  RD_PaletteCode_TabInactive,
  RD_PaletteCode_DropSiteOverlay,
  RD_PaletteCode_COUNT
}
RD_PaletteCode;

////////////////////////////////
//~ rjf: Auto-Complete Lister Types

typedef U32 RD_AutoCompListerFlags;
enum
{
  RD_AutoCompListerFlag_Locals        = (1<<0),
  RD_AutoCompListerFlag_Registers     = (1<<1),
  RD_AutoCompListerFlag_ViewRules     = (1<<2),
  RD_AutoCompListerFlag_ViewRuleParams= (1<<3),
  RD_AutoCompListerFlag_Members       = (1<<4),
  RD_AutoCompListerFlag_Globals       = (1<<5),
  RD_AutoCompListerFlag_ThreadLocals  = (1<<6),
  RD_AutoCompListerFlag_Procedures    = (1<<7),
  RD_AutoCompListerFlag_Types         = (1<<8),
  RD_AutoCompListerFlag_Languages     = (1<<9),
  RD_AutoCompListerFlag_Architectures = (1<<10),
  RD_AutoCompListerFlag_Tex2DFormats  = (1<<11),
  RD_AutoCompListerFlag_Files         = (1<<12),
};

typedef struct RD_AutoCompListerItem RD_AutoCompListerItem;
struct RD_AutoCompListerItem
{
  String8 string;
  String8 kind_string;
  FuzzyMatchRangeList matches;
  U64 group;
  B32 is_non_code;
};

typedef struct RD_AutoCompListerItemChunkNode RD_AutoCompListerItemChunkNode;
struct RD_AutoCompListerItemChunkNode
{
  RD_AutoCompListerItemChunkNode *next;
  RD_AutoCompListerItem *v;
  U64 count;
  U64 cap;
};

typedef struct RD_AutoCompListerItemChunkList RD_AutoCompListerItemChunkList;
struct RD_AutoCompListerItemChunkList
{
  RD_AutoCompListerItemChunkNode *first;
  RD_AutoCompListerItemChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct RD_AutoCompListerItemArray RD_AutoCompListerItemArray;
struct RD_AutoCompListerItemArray
{
  RD_AutoCompListerItem *v;
  U64 count;
};

typedef struct RD_AutoCompListerParams RD_AutoCompListerParams;
struct RD_AutoCompListerParams
{
  RD_AutoCompListerFlags flags;
  String8List strings;
};

////////////////////////////////
//~ rjf: Per-Window State

typedef struct RD_Window RD_Window;
struct RD_Window
{
  // rjf: links & metadata
  RD_Window *next;
  RD_Window *prev;
  U64 gen;
  U64 frames_alive;
  RD_CfgSrc cfg_src;
  
  // rjf: top-level info & handles
  Arena *arena;
  OS_Handle os;
  R_Handle r;
  UI_State *ui;
  F32 last_dpi;
  B32 window_temporarily_focused_ipc;
  
  // rjf: config/settings
  RD_SettingVal setting_vals[RD_SettingCode_COUNT];
  UI_Palette cfg_palettes[RD_PaletteCode_COUNT]; // derivative from theme
  
  // rjf: dev interface state
  B32 dev_menu_is_open;
  
  // rjf: menu bar state
  B32 menu_bar_focused;
  B32 menu_bar_focused_on_press;
  B32 menu_bar_key_held;
  B32 menu_bar_focus_press_started;
  
  // rjf: context menu state
  Arena *ctx_menu_arena;
  RD_Regs *ctx_menu_regs;
  RD_RegSlot ctx_menu_regs_slot;
  U8 *ctx_menu_input_buffer;
  U64 ctx_menu_input_buffer_size;
  U64 ctx_menu_input_string_size;
  TxtPt ctx_menu_input_cursor;
  TxtPt ctx_menu_input_mark;
  
  // rjf: drop-completion state
  Arena *drop_completion_arena;
  String8List drop_completion_paths;
  
  // rjf: autocomplete lister state
  U64 autocomp_last_frame_idx;
  B32 autocomp_input_dirty;
  UI_Key autocomp_root_key;
  Arena *autocomp_lister_params_arena;
  RD_AutoCompListerParams autocomp_lister_params;
  U64 autocomp_cursor_off;
  U8 autocomp_lister_input_buffer[1024];
  U64 autocomp_lister_input_size;
  F32 autocomp_open_t;
  F32 autocomp_num_visible_rows_t;
  S64 autocomp_cursor_num;
  
  // rjf: query view stack
  Arena *query_cmd_arena;
  String8 query_cmd_name;
  RD_Regs *query_cmd_regs;
  U64 query_cmd_regs_mask[(RD_RegSlot_COUNT + 63) / 64];
  RD_View *query_view_stack_top;
  B32 query_view_selected;
  F32 query_view_selected_t;
  F32 query_view_t;
  
  // rjf: hover eval state
  B32 hover_eval_focused;
  TxtPt hover_eval_txt_cursor;
  TxtPt hover_eval_txt_mark;
  U8 hover_eval_txt_buffer[1024];
  U64 hover_eval_txt_size;
  Arena *hover_eval_arena;
  Vec2F32 hover_eval_spawn_pos;
  String8 hover_eval_string;
  U64 hover_eval_first_frame_idx;
  U64 hover_eval_last_frame_idx;
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
  RD_Panel *root_panel;
  RD_Panel *free_panel;
  RD_Panel *focused_panel;
  
  // rjf: per-frame ui events state
  UI_EventList ui_events;
  
  // rjf: per-frame drawing state
  DR_Bucket *draw_bucket;
};

////////////////////////////////
//~ rjf: Eval Visualization View Cache Types

typedef struct RD_EvalVizViewCacheNode RD_EvalVizViewCacheNode;
struct RD_EvalVizViewCacheNode
{
  RD_EvalVizViewCacheNode *next;
  RD_EvalVizViewCacheNode *prev;
  U64 key;
  EV_View *v;
};

typedef struct RD_EvalVizViewCacheSlot RD_EvalVizViewCacheSlot;
struct RD_EvalVizViewCacheSlot
{
  RD_EvalVizViewCacheNode *first;
  RD_EvalVizViewCacheNode *last;
};

////////////////////////////////
//~ rjf: Meta Evaluation Cache Types

typedef struct RD_CtrlEntityMetaEvalCacheNode RD_CtrlEntityMetaEvalCacheNode;
struct RD_CtrlEntityMetaEvalCacheNode
{
  RD_CtrlEntityMetaEvalCacheNode *next;
  CTRL_Handle handle;
  CTRL_MetaEval *meval;
  Rng1U64 range;
};

typedef struct RD_CtrlEntityMetaEvalCacheSlot RD_CtrlEntityMetaEvalCacheSlot;
struct RD_CtrlEntityMetaEvalCacheSlot
{
  RD_CtrlEntityMetaEvalCacheNode *first;
  RD_CtrlEntityMetaEvalCacheNode *last;
};

////////////////////////////////
//~ rjf: Main Per-Process Graphical State

typedef struct RD_NameChunkNode RD_NameChunkNode;
struct RD_NameChunkNode
{
  RD_NameChunkNode *next;
  U64 size;
};

typedef struct RD_EntityListCache RD_EntityListCache;
struct RD_EntityListCache
{
  Arena *arena;
  U64 alloc_gen;
  RD_EntityList list;
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
  
  // rjf: log
  Log *log;
  String8 log_path;
  
  // rjf: frame history info
  U64 frame_index;
  Arena *frame_arenas[2];
  U64 frame_time_us_history[64];
  U64 num_frames_requested;
  F64 time_in_seconds;
  
  // rjf: frame parameters
  F32 frame_dt;
  DI_Scope *frame_di_scope;
  
  // rjf: dbgi match store
  DI_MatchStore *match_store;
  
  // rjf: ambiguous path table
  U64 ambiguous_path_slots_count;
  RD_AmbiguousPathNode **ambiguous_path_slots;
  
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
  
  // rjf: string search state
  Arena *string_search_arena;
  String8 string_search_string;
  
  // rjf: eval visualization view cache
  U64 eval_viz_view_cache_slots_count;
  RD_EvalVizViewCacheSlot *eval_viz_view_cache_slots;
  RD_EvalVizViewCacheNode *eval_viz_view_cache_node_free;
  
  // rjf: ctrl entity meta eval cache
  U64 ctrl_entity_meval_cache_slots_count;
  RD_CtrlEntityMetaEvalCacheSlot *ctrl_entity_meval_cache_slots;
  
  // rjf: contextual hover info
  RD_Regs *hover_regs;
  RD_RegSlot hover_regs_slot;
  RD_Regs *next_hover_regs;
  RD_RegSlot next_hover_regs_slot;
  
  // rjf: icon texture
  R_Handle icon_texture;
  
  // rjf: current path
  Arena *current_path_arena;
  String8 current_path;
  
  // rjf: fixed ui keys
  UI_Key drop_completion_key;
  UI_Key ctx_menu_key;
  
  // rjf: drag/drop state
  Arena *drag_drop_arena;
  RD_Regs *drag_drop_regs;
  RD_RegSlot drag_drop_regs_slot;
  RD_DragDropState drag_drop_state;
  
  //-
  // TODO(rjf): TO BE ELIMINATED OR REPLACED VVVVVVVVVVVVVVVV
  //-
  
  // rjf: entity state
  RD_NameChunkNode *free_name_chunks[8];
  Arena *entities_arena;
  RD_Entity *entities_base;
  U64 entities_count;
  U64 entities_id_gen;
  RD_Entity *entities_root;
  RD_Entity *entities_free[2]; // [0] -> normal lifetime, not user defined; [1] -> user defined lifetime (& thus undoable)
  U64 entities_free_count;
  U64 entities_active_count;
  
  // rjf: entity query caches
  U64 kind_alloc_gens[RD_EntityKind_COUNT];
  RD_EntityListCache kind_caches[RD_EntityKind_COUNT];
  
  // rjf: key map table
  Arena *key_map_arena;
  U64 key_map_table_size;
  RD_KeyMapSlot *key_map_table;
  RD_KeyMapNode *free_key_map_node;
  U64 key_map_total_count;
  
  // rjf: bind change
  Arena *bind_change_arena;
  B32 bind_change_active;
  String8 bind_change_cmd_name;
  RD_Binding bind_change_binding;
  
  // rjf: windows
  RD_Window *first_window;
  RD_Window *last_window;
  RD_Window *free_window;
  U64 window_count;
  B32 last_window_queued_save;
  RD_Handle last_focused_window;
  
  // rjf: view state
  RD_View *first_view;
  RD_View *last_view;
  RD_View *free_view;
  U64 free_view_count;
  U64 allocated_view_count;
  
  // rjf: config reading state
  Arena *cfg_path_arenas[RD_CfgSrc_COUNT];
  String8 cfg_paths[RD_CfgSrc_COUNT];
  U64 cfg_cached_timestamp[RD_CfgSrc_COUNT];
  Arena *cfg_arena;
  RD_CfgTable cfg_table;
  U64 ctrl_exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64];
  
  // rjf: running theme state
  RD_Theme cfg_theme_target;
  RD_Theme cfg_theme;
  Arena *cfg_main_font_path_arena;
  Arena *cfg_code_font_path_arena;
  String8 cfg_main_font_path;
  String8 cfg_code_font_path;
  FNT_Tag cfg_font_tags[RD_FontSlot_COUNT]; // derivative from font paths
  
  // rjf: global settings
  RD_SettingVal cfg_setting_vals[RD_CfgSrc_COUNT][RD_SettingCode_COUNT];
  
  //-
  // TODO(rjf): TO BE ELIMINATED OR REPLACED ^^^^^^^^^^^^^^^^^^
  //-
};

////////////////////////////////
//~ rjf: Globals

read_only global RD_CfgTree d_nil_cfg_tree = {&d_nil_cfg_tree, RD_CfgSrc_User, &md_nil_node};
read_only global RD_CfgVal d_nil_cfg_val = {&d_nil_cfg_val, &d_nil_cfg_val, &d_nil_cfg_tree, &d_nil_cfg_tree};

read_only global RD_Entity d_nil_entity =
{
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
  &d_nil_entity,
};

read_only global RD_CmdKindInfo rd_nil_cmd_kind_info = {0};

read_only global RD_ViewRuleInfo rd_nil_view_rule_info =
{
  {0},
  {0},
  {0},
  {0},
  RD_IconKind_Null,
  0,
  EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_NAME(nil),
  RD_VIEW_RULE_UI_FUNCTION_NAME(null)
};

read_only global RD_View rd_nil_view =
{
  &rd_nil_view,
  &rd_nil_view,
  &rd_nil_view,
  &rd_nil_view,
  &rd_nil_view,
  &rd_nil_view,
  &rd_nil_view_rule_info,
};

read_only global RD_Panel rd_nil_panel =
{
  &rd_nil_panel,
  &rd_nil_panel,
  &rd_nil_panel,
  &rd_nil_panel,
  &rd_nil_panel,
};

global RD_State *rd_state = 0;
global RD_Handle rd_last_drag_drop_panel = {0};
global RD_Handle rd_last_drag_drop_prev_tab = {0};

////////////////////////////////
//~ rjf: Handle Type Pure Functions

internal RD_Handle rd_handle_zero(void);
internal B32 rd_handle_match(RD_Handle a, RD_Handle b);
internal void rd_handle_list_push_node(RD_HandleList *list, RD_HandleNode *node);
internal void rd_handle_list_push(Arena *arena, RD_HandleList *list, RD_Handle handle);
internal RD_HandleList rd_handle_list_copy(Arena *arena, RD_HandleList list);

////////////////////////////////
//~ rjf: Config Type Pure Functions

internal void rd_cfg_table_push_unparsed_string(Arena *arena, RD_CfgTable *table, String8 string, RD_CfgSrc source);
internal RD_CfgVal *rd_cfg_val_from_string(RD_CfgTable *table, String8 string);

////////////////////////////////
//~ rjf: Registers Type Functions

internal void rd_regs_copy_contents(Arena *arena, RD_Regs *dst, RD_Regs *src);
internal RD_Regs *rd_regs_copy(Arena *arena, RD_Regs *src);

////////////////////////////////
//~ rjf: Commands Type Functions

internal void rd_cmd_list_push_new(Arena *arena, RD_CmdList *cmds, String8 name, RD_Regs *regs);

////////////////////////////////
//~ rjf: Entity Type Pure Functions

//- rjf: nil
internal B32 rd_entity_is_nil(RD_Entity *entity);
#define rd_require_entity_nonnil(entity, if_nil_stmts) do{if(rd_entity_is_nil(entity)){if_nil_stmts;}}while(0)

//- rjf: handle <-> entity conversions
internal U64 rd_index_from_entity(RD_Entity *entity);
internal RD_Handle rd_handle_from_entity(RD_Entity *entity);
internal RD_Entity *rd_entity_from_handle(RD_Handle handle);
internal RD_HandleList rd_handle_list_from_entity_list(Arena *arena, RD_EntityList entities);

//- rjf: entity recursion iterators
internal RD_EntityRec rd_entity_rec_depth_first(RD_Entity *entity, RD_Entity *subtree_root, U64 sib_off, U64 child_off);
#define rd_entity_rec_depth_first_pre(entity, subtree_root)  rd_entity_rec_depth_first((entity), (subtree_root), OffsetOf(RD_Entity, next), OffsetOf(RD_Entity, first))
#define rd_entity_rec_depth_first_post(entity, subtree_root) rd_entity_rec_depth_first((entity), (subtree_root), OffsetOf(RD_Entity, prev), OffsetOf(RD_Entity, last))

//- rjf: ancestor/child introspection
internal RD_Entity *rd_entity_child_from_kind(RD_Entity *entity, RD_EntityKind kind);
internal RD_Entity *rd_entity_ancestor_from_kind(RD_Entity *entity, RD_EntityKind kind);
internal RD_EntityList rd_push_entity_child_list_with_kind(Arena *arena, RD_Entity *entity, RD_EntityKind kind);
internal RD_Entity *rd_entity_child_from_string_and_kind(RD_Entity *parent, String8 string, RD_EntityKind kind);

//- rjf: entity list building
internal void rd_entity_list_push(Arena *arena, RD_EntityList *list, RD_Entity *entity);
internal RD_EntityArray rd_entity_array_from_list(Arena *arena, RD_EntityList *list);
#define rd_first_entity_from_list(list) ((list)->first != 0 ? (list)->first->entity : &d_nil_entity)

//- rjf: entity fuzzy list building
internal RD_EntityFuzzyItemArray rd_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, RD_EntityList *list, String8 needle);
internal RD_EntityFuzzyItemArray rd_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, RD_EntityArray *array, String8 needle);

//- rjf: full path building, from file/folder entities
internal String8 rd_full_path_from_entity(Arena *arena, RD_Entity *entity);

//- rjf: display string entities, for referencing entities in ui
internal String8 rd_display_string_from_entity(Arena *arena, RD_Entity *entity);

//- rjf: extra search tag strings for fuzzy filtering entities
internal String8 rd_search_tags_from_entity(Arena *arena, RD_Entity *entity);

//- rjf: entity -> color operations
internal Vec4F32 rd_hsva_from_entity(RD_Entity *entity);
internal Vec4F32 rd_rgba_from_entity(RD_Entity *entity);

//- rjf: entity -> expansion tree keys
internal EV_Key rd_ev_key_from_entity(RD_Entity *entity);
internal EV_Key rd_parent_ev_key_from_entity(RD_Entity *entity);

//- rjf: entity -> evaluation
internal RD_EntityEval *rd_eval_from_entity(Arena *arena, RD_Entity *entity);

////////////////////////////////
//~ rjf: View Type Functions

internal B32 rd_view_is_nil(RD_View *view);
internal B32 rd_view_is_project_filtered(RD_View *view);
internal RD_Handle rd_handle_from_view(RD_View *view);
internal RD_View *rd_view_from_handle(RD_Handle handle);

////////////////////////////////
//~ rjf: View Spec Type Functions

internal RD_ViewRuleKind rd_view_rule_kind_from_string(String8 string);
internal RD_ViewRuleInfo *rd_view_rule_info_from_kind(RD_ViewRuleKind kind);
internal RD_ViewRuleInfo *rd_view_rule_info_from_string(String8 string);

////////////////////////////////
//~ rjf: Panel Type Functions

//- rjf: basic type functions
internal B32 rd_panel_is_nil(RD_Panel *panel);
internal RD_Handle rd_handle_from_panel(RD_Panel *panel);
internal RD_Panel *rd_panel_from_handle(RD_Handle handle);
internal UI_Key rd_ui_key_from_panel(RD_Panel *panel);

//- rjf: tree construction
internal void rd_panel_insert(RD_Panel *parent, RD_Panel *prev_child, RD_Panel *new_child);
internal void rd_panel_remove(RD_Panel *parent, RD_Panel *child);

//- rjf: tree walk
internal RD_PanelRec rd_panel_rec_depth_first(RD_Panel *panel, U64 sib_off, U64 child_off);
#define rd_panel_rec_depth_first_pre(panel) rd_panel_rec_depth_first(panel, OffsetOf(RD_Panel, next), OffsetOf(RD_Panel, first))
#define rd_panel_rec_depth_first_pre_rev(panel) rd_panel_rec_depth_first(panel, OffsetOf(RD_Panel, prev), OffsetOf(RD_Panel, last))

//- rjf: panel -> rect calculations
internal Rng2F32 rd_target_rect_from_panel_child(Rng2F32 parent_rect, RD_Panel *parent, RD_Panel *panel);
internal Rng2F32 rd_target_rect_from_panel(Rng2F32 root_rect, RD_Panel *root, RD_Panel *panel);

//- rjf: view ownership insertion/removal
internal void rd_panel_insert_tab_view(RD_Panel *panel, RD_View *prev_view, RD_View *view);
internal void rd_panel_remove_tab_view(RD_Panel *panel, RD_View *view);
internal RD_View *rd_selected_tab_from_panel(RD_Panel *panel);

//- rjf: icons & display strings
internal RD_IconKind rd_icon_kind_from_view(RD_View *view);
internal DR_FancyStringList rd_title_fstrs_from_view(Arena *arena, RD_View *view, Vec4F32 primary_color, Vec4F32 secondary_color, F32 size);

////////////////////////////////
//~ rjf: Window Type Functions

internal RD_Handle rd_handle_from_window(RD_Window *window);
internal RD_Window *rd_window_from_handle(RD_Handle handle);

////////////////////////////////
//~ rjf: Command Parameters From Context

internal B32 rd_prefer_dasm_from_window(RD_Window *window);

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32 rd_drag_is_active(void);
internal void rd_drag_begin(RD_RegSlot slot);
internal B32 rd_drag_drop(void);
internal void rd_drag_kill(void);

internal void rd_set_hover_regs(RD_RegSlot slot);
internal RD_Regs *rd_get_hover_regs(void);

internal void rd_open_ctx_menu(UI_Key anchor_box_key, Vec2F32 anchor_box_off, RD_RegSlot slot);

////////////////////////////////
//~ rjf: Name Allocation

internal U64 rd_name_bucket_idx_from_string_size(U64 size);
internal String8 rd_name_alloc(String8 string);
internal void rd_name_release(String8 string);

////////////////////////////////
//~ rjf: Entity Stateful Functions

//- rjf: entity allocation + tree forming
internal RD_Entity *rd_entity_alloc(RD_Entity *parent, RD_EntityKind kind);
internal void rd_entity_mark_for_deletion(RD_Entity *entity);
internal void rd_entity_release(RD_Entity *entity);
internal void rd_entity_change_parent(RD_Entity *entity, RD_Entity *old_parent, RD_Entity *new_parent, RD_Entity *prev_child);
internal RD_Entity *rd_entity_child_from_kind_or_alloc(RD_Entity *entity, RD_EntityKind kind);

//- rjf: entity simple equipment
internal void rd_entity_equip_txt_pt(RD_Entity *entity, TxtPt point);
internal void rd_entity_equip_entity_handle(RD_Entity *entity, RD_Handle handle);
internal void rd_entity_equip_disabled(RD_Entity *entity, B32 b32);
internal void rd_entity_equip_u64(RD_Entity *entity, U64 u64);
internal void rd_entity_equip_color_rgba(RD_Entity *entity, Vec4F32 rgba);
internal void rd_entity_equip_color_hsva(RD_Entity *entity, Vec4F32 hsva);
internal void rd_entity_equip_cfg_src(RD_Entity *entity, RD_CfgSrc cfg_src);
internal void rd_entity_equip_timestamp(RD_Entity *entity, U64 timestamp);

//- rjf: control layer correllation equipment
internal void rd_entity_equip_ctrl_handle(RD_Entity *entity, CTRL_Handle handle);
internal void rd_entity_equip_arch(RD_Entity *entity, Arch arch);
internal void rd_entity_equip_ctrl_id(RD_Entity *entity, U32 id);
internal void rd_entity_equip_stack_base(RD_Entity *entity, U64 stack_base);
internal void rd_entity_equip_vaddr_rng(RD_Entity *entity, Rng1U64 range);
internal void rd_entity_equip_vaddr(RD_Entity *entity, U64 vaddr);

//- rjf: name equipment
internal void rd_entity_equip_name(RD_Entity *entity, String8 name);

//- rjf: file path map override lookups
internal String8 rd_mapped_from_file_path(Arena *arena, String8 file_path);
internal String8List rd_possible_overrides_from_file_path(Arena *arena, String8 file_path);

//- rjf: top-level state queries
internal RD_Entity *rd_entity_root(void);
internal RD_EntityList rd_push_entity_list_with_kind(Arena *arena, RD_EntityKind kind);
internal RD_Entity *rd_entity_from_id(RD_EntityID id);
internal RD_Entity *rd_machine_entity_from_machine_id(CTRL_MachineID machine_id);
internal RD_Entity *rd_entity_from_ctrl_handle(CTRL_Handle handle);
internal RD_Entity *rd_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id);
internal RD_Entity *rd_entity_from_name_and_kind(String8 string, RD_EntityKind kind);

////////////////////////////////
//~ rjf: Frontend Entity Info Extraction

internal D_Target rd_d_target_from_entity(RD_Entity *entity);
internal DR_FancyStringList rd_title_fstrs_from_entity(Arena *arena, RD_Entity *entity, Vec4F32 secondary_color, F32 size);

////////////////////////////////
//~ rjf: Control Entity Info Extraction

internal Vec4F32 rd_rgba_from_ctrl_entity(CTRL_Entity *entity);
internal String8 rd_name_from_ctrl_entity(Arena *arena, CTRL_Entity *entity);
internal DR_FancyStringList rd_title_fstrs_from_ctrl_entity(Arena *arena, CTRL_Entity *entity, Vec4F32 secondary_color, F32 size, B32 include_extras);

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: entity <-> eval space
internal RD_Entity *rd_entity_from_eval_space(E_Space space);
internal E_Space rd_eval_space_from_entity(RD_Entity *entity);

//- rjf: ctrl entity <-> eval space
internal CTRL_Entity *rd_ctrl_entity_from_eval_space(E_Space space);
internal E_Space rd_eval_space_from_ctrl_entity(CTRL_Entity *entity, E_SpaceKind kind);

//- rjf: entity -> meta eval
internal CTRL_MetaEval *rd_ctrl_meta_eval_from_entity(Arena *arena, RD_Entity *entity);
internal CTRL_MetaEval *rd_ctrl_meta_eval_from_ctrl_entity(Arena *arena, CTRL_Entity *entity);

//- rjf: eval space reads/writes
internal B32 rd_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range);
internal B32 rd_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range);

//- rjf: asynchronous streamed reads -> hashes from spaces
internal U128 rd_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated);

//- rjf: space -> entire range
internal Rng1U64 rd_whole_range_from_eval_space(E_Space space);

////////////////////////////////
//~ rjf: Evaluation Visualization

//- rjf: writing values back to child processes
internal B32 rd_commit_eval_value_string(E_Eval dst_eval, String8 string, B32 string_needs_unescaping);

//- rjf: eval / view rule params tree info extraction
internal U64 rd_base_offset_from_eval(E_Eval eval);
internal E_Value rd_value_from_params_key(MD_Node *params, String8 key);
internal Rng1U64 rd_range_from_eval_params(E_Eval eval, MD_Node *params);
internal TXT_LangKind rd_lang_kind_from_eval_params(E_Eval eval, MD_Node *params);
internal Arch rd_arch_from_eval_params(E_Eval eval, MD_Node *params);
internal Vec2S32 rd_dim2s32_from_eval_params(E_Eval eval, MD_Node *params);
internal R_Tex2DFormat rd_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params);

//- rjf: eval <-> file path
internal String8 rd_file_path_from_eval_string(Arena *arena, String8 string);
internal String8 rd_eval_string_from_file_path(Arena *arena, String8 string);

////////////////////////////////
//~ rjf: View State Functions

//- rjf: allocation/releasing
internal RD_View *rd_view_alloc(void);
internal void rd_view_release(RD_View *view);

//- rjf: equipment
internal void rd_view_equip_spec(RD_View *view, RD_ViewRuleInfo *spec, String8 query, MD_Node *params);
internal void rd_view_equip_query(RD_View *view, String8 query);
internal void rd_view_equip_loading_info(RD_View *view, B32 is_loading, U64 progress_v, U64 progress_target);

//- rjf: user state extensions
internal void *rd_view_get_or_push_user_state(RD_View *view, U64 size);
internal Arena *rd_view_push_arena_ext(RD_View *view);
#define rd_view_user_state(view, type) (type *)rd_view_get_or_push_user_state((view), sizeof(type))

//- rjf: param saving
internal void rd_view_store_param(RD_View *view, String8 key, String8 value);
internal void rd_view_store_paramf(RD_View *view, String8 key, char *fmt, ...);
#define rd_view_store_param_f32(view, key, f32) rd_view_store_paramf((view), (key), "%ff", (f32))
#define rd_view_store_param_s64(view, key, s64) rd_view_store_paramf((view), (key), "%I64d", (s64))
#define rd_view_store_param_u64(view, key, u64) rd_view_store_paramf((view), (key), "0x%I64x", (u64))

////////////////////////////////
//~ rjf: View Building API

//- rjf: view info extraction
internal Arena *rd_view_arena(void);
internal UI_ScrollPt2 rd_view_scroll_pos(void);
internal String8 rd_view_expr_string(void);
internal String8 rd_view_filter(void);

//- rjf: pushing/attaching view resources
internal void *rd_view_state_by_size(U64 size);
#define rd_view_state(T) (T *)rd_view_state_by_size(sizeof(T))
internal Arena *rd_push_view_arena(void);

//- rjf: storing view-attached state
internal void rd_store_view_expr_string(String8 string);
internal void rd_store_view_filter(String8 string);
internal void rd_store_view_loading_info(B32 is_loading, U64 progress_u64, U64 progress_u64_target);
internal void rd_store_view_scroll_pos(UI_ScrollPt2 pos);
internal void rd_store_view_param(String8 key, String8 value);
internal void rd_store_view_paramf(String8 key, char *fmt, ...);
#define rd_store_view_param_f32(key, f32) rd_store_view_paramf((key), "%ff", (f32))
#define rd_store_view_param_s64(key, s64) rd_store_view_paramf((key), "%I64d", (s64))
#define rd_store_view_param_u64(key, u64) rd_store_view_paramf((key), "0x%I64x", (u64))

////////////////////////////////
//~ rjf: Expand-Keyed Transient View Functions

internal RD_TransientViewNode *rd_transient_view_node_from_ev_key(RD_View *owner_view, EV_Key key);

////////////////////////////////
//~ rjf: Panel State Functions

internal RD_Panel *rd_panel_alloc(RD_Window *ws);
internal void rd_panel_release(RD_Window *ws, RD_Panel *panel);
internal void rd_panel_release_all_views(RD_Panel *panel);

////////////////////////////////
//~ rjf: Window State Functions

internal RD_Window *rd_window_open(Vec2F32 size, OS_Handle preferred_monitor, RD_CfgSrc cfg_src);

internal RD_Window *rd_window_from_os_handle(OS_Handle os);

internal void rd_window_frame(RD_Window *ws);

////////////////////////////////
//~ rjf: Eval Visualization

internal EV_ExpandInfo      rd_ev_view_rule_expr_expand_info__meta_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, RD_EntityKind kind);
internal EV_ExpandRangeInfo rd_ev_view_rule_expr_expand_range_info__meta_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, RD_EntityKind kind, B32 add_new_at_top);
internal U64                rd_ev_view_rule_expr_id_from_num__meta_entities(U64 num, void *user_data, RD_EntityKind kind, B32 add_new_at_top);
internal U64                rd_ev_view_rule_expr_num_from_id__meta_entities(U64 id, void *user_data, RD_EntityKind kind, B32 add_new_at_top);

internal EV_ExpandInfo      rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, CTRL_EntityKind kind);
internal EV_ExpandRangeInfo rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, CTRL_EntityKind kind);
internal U64                rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(U64 num, void *user_data, CTRL_EntityKind kind);
internal U64                rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(U64 id, void *user_data, CTRL_EntityKind kind);

internal EV_ExpandInfo      rd_ev_view_rule_expr_expand_info__debug_info_tables(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, RDI_SectionKind section);
internal EV_ExpandRangeInfo rd_ev_view_rule_expr_expand_range_info__debug_info_tables(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, RDI_SectionKind section);
internal U64                rd_ev_view_rule_expr_id_from_num__debug_info_tables(U64 num, void *user_data, RDI_SectionKind section);
internal U64                rd_ev_view_rule_expr_num_from_id__debug_info_tables(U64 id, void *user_data, RDI_SectionKind section);

internal EV_View *rd_ev_view_from_key(U64 key);
internal F32 rd_append_value_strings_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, S32 depth, E_Eval eval, E_Member *member, EV_ViewRuleList *view_rules, String8List *out);
internal String8 rd_value_string_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval, E_Member *member, EV_ViewRuleList *view_rules);

////////////////////////////////
//~ rjf: Hover Eval

internal void rd_set_hover_eval(Vec2F32 pos, String8 file_path, TxtPt pt, U64 vaddr, String8 string);

////////////////////////////////
//~ rjf: Auto-Complete Lister

internal void rd_autocomp_lister_item_chunk_list_push(Arena *arena, RD_AutoCompListerItemChunkList *list, U64 cap, RD_AutoCompListerItem *item);
internal RD_AutoCompListerItemArray rd_autocomp_lister_item_array_from_chunk_list(Arena *arena, RD_AutoCompListerItemChunkList *list);
internal int rd_autocomp_lister_item_qsort_compare(RD_AutoCompListerItem *a, RD_AutoCompListerItem *b);
internal void rd_autocomp_lister_item_array_sort__in_place(RD_AutoCompListerItemArray *array);

internal String8 rd_autocomp_query_word_from_input_string_off(String8 input, U64 cursor_off);
internal String8 rd_autocomp_query_path_from_input_string_off(String8 input, U64 cursor_off);
internal RD_AutoCompListerParams rd_view_rule_autocomp_lister_params_from_input_cursor(Arena *arena, String8 string, U64 cursor_off);
internal void rd_set_autocomp_lister_query(UI_Key root_key, RD_AutoCompListerParams *params, String8 input, U64 cursor_off);

////////////////////////////////
//~ rjf: Search Strings

internal void rd_set_search_string(String8 string);
internal String8 rd_push_search_string(Arena *arena);

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: keybindings
internal OS_Key rd_os_key_from_cfg_string(String8 string);
internal void rd_clear_bindings(void);
internal RD_BindingList rd_bindings_from_name(Arena *arena, String8 name);
internal void rd_bind_name(String8 name, RD_Binding binding);
internal void rd_unbind_name(String8 name, RD_Binding binding);
internal String8List rd_cmd_name_list_from_binding(Arena *arena, RD_Binding binding);

//- rjf: colors
internal Vec4F32 rd_rgba_from_theme_color(RD_ThemeColor color);
internal RD_ThemeColor rd_theme_color_from_txt_token_kind(TXT_TokenKind kind);
internal RD_ThemeColor rd_theme_color_from_txt_token_kind_lookup_string(TXT_TokenKind kind, String8 string);

//- rjf: code -> palette
internal UI_Palette *rd_palette_from_code(RD_PaletteCode code);

//- rjf: fonts/sizes
internal FNT_Tag rd_font_from_slot(RD_FontSlot slot);
internal F32 rd_font_size_from_slot(RD_FontSlot slot);
internal FNT_RasterFlags rd_raster_flags_from_slot(RD_FontSlot slot);

//- rjf: settings
internal RD_SettingVal rd_setting_val_from_code(RD_SettingCode code);

//- rjf: config serialization
internal int rd_qsort_compare__cfg_string_bindings(RD_StringBindingPair *a, RD_StringBindingPair *b);
internal String8List rd_cfg_strings_from_gfx(Arena *arena, String8 root_path, RD_CfgSrc source);

////////////////////////////////
//~ rjf: Process Control Info Stringification

internal String8 rd_string_from_exception_code(U32 code);
internal String8 rd_stop_explanation_string_icon_from_ctrl_event(Arena *arena, CTRL_Event *event, RD_IconKind *icon_out);

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void rd_request_frame(void);

////////////////////////////////
//~ rjf: Main State Accessors

//- rjf: per-frame arena
internal Arena *rd_frame_arena(void);

//- rjf: config paths
internal String8 rd_cfg_path_from_src(RD_CfgSrc src);

//- rjf: entity cache queries
internal RD_EntityList rd_query_cached_entity_list_with_kind(RD_EntityKind kind);
internal RD_EntityList rd_push_active_target_list(Arena *arena);
internal RD_Entity *rd_entity_from_ev_key_and_kind(EV_Key key, RD_EntityKind kind);

//- rjf: config state
internal RD_CfgTable *rd_cfg_table(void);

////////////////////////////////
//~ rjf: Registers

internal RD_Regs *rd_regs(void);
internal RD_Regs *rd_base_regs(void);
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

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void rd_init(CmdLine *cmdln);
internal void rd_frame(void);

#endif // RADDBG_CORE_H
