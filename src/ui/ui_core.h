// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef UI_H
#define UI_H

////////////////////////////////
//~ rjf: Icon Info

typedef enum UI_IconKind
{
  UI_IconKind_Null,
  UI_IconKind_RightArrow,
  UI_IconKind_DownArrow,
  UI_IconKind_LeftArrow,
  UI_IconKind_UpArrow,
  UI_IconKind_RightCaret,
  UI_IconKind_DownCaret,
  UI_IconKind_LeftCaret,
  UI_IconKind_UpCaret,
  UI_IconKind_CheckHollow,
  UI_IconKind_CheckFilled,
  UI_IconKind_COUNT
}
UI_IconKind;

typedef struct UI_IconInfo UI_IconInfo;
struct UI_IconInfo
{
  FNT_Tag icon_font;
  String8 icon_kind_text_map[UI_IconKind_COUNT];
};

////////////////////////////////
//~ rjf: Mouse Button Kinds

typedef enum UI_MouseButtonKind
{
  UI_MouseButtonKind_Left,
  UI_MouseButtonKind_Middle,
  UI_MouseButtonKind_Right,
  UI_MouseButtonKind_COUNT
}
UI_MouseButtonKind;

////////////////////////////////
//~ rjf: Codepath Permissions

typedef U32 UI_PermissionFlags;
enum
{
  UI_PermissionFlag_ClicksLeft       = (1<<0),
  UI_PermissionFlag_ClicksMiddle     = (1<<1),
  UI_PermissionFlag_ClicksRight      = (1<<2),
  UI_PermissionFlag_ScrollX          = (1<<3),
  UI_PermissionFlag_ScrollY          = (1<<4),
  UI_PermissionFlag_KeyboardPrimary  = (1<<5),
  UI_PermissionFlag_KeyboardSecondary= (1<<6),
  UI_PermissionFlag_Text             = (1<<7),
  
  //- rjf bundles
  UI_PermissionFlag_Keyboard = (UI_PermissionFlag_KeyboardPrimary|UI_PermissionFlag_KeyboardSecondary),
  UI_PermissionFlag_Clicks = (UI_PermissionFlag_ClicksLeft|UI_PermissionFlag_ClicksMiddle|UI_PermissionFlag_ClicksRight),
  UI_PermissionFlag_All = 0xffffffff,
};

////////////////////////////////
//~ rjf: Focus Types

typedef enum UI_FocusKind
{
  UI_FocusKind_Null,
  UI_FocusKind_Off,
  UI_FocusKind_On,
  UI_FocusKind_Root,
  UI_FocusKind_COUNT
}
UI_FocusKind;

////////////////////////////////
//~ rjf: Events

// TODO(rjf): clean all this up

typedef enum UI_EventKind
{
  UI_EventKind_Null,
  UI_EventKind_Press,
  UI_EventKind_Release,
  UI_EventKind_Text,
  UI_EventKind_Navigate,
  UI_EventKind_Edit,
  UI_EventKind_MouseMove,
  UI_EventKind_Scroll,
  UI_EventKind_AutocompleteHint,
  UI_EventKind_FileDrop,
  UI_EventKind_COUNT
}
UI_EventKind;

typedef enum UI_EventActionSlot
{
  UI_EventActionSlot_Null,
  UI_EventActionSlot_Accept,
  UI_EventActionSlot_Cancel,
  UI_EventActionSlot_Edit,
  UI_EventActionSlot_COUNT
}
UI_EventActionSlot;

typedef U32 UI_EventFlags;
enum
{
  UI_EventFlag_KeepMark            = (1<<0),
  UI_EventFlag_Delete              = (1<<1),
  UI_EventFlag_Copy                = (1<<2),
  UI_EventFlag_Paste               = (1<<3),
  UI_EventFlag_ZeroDeltaOnSelect   = (1<<4),
  UI_EventFlag_PickSelectSide      = (1<<5),
  UI_EventFlag_CapAtLine           = (1<<6),
  UI_EventFlag_ExplicitDirectional = (1<<7),
  UI_EventFlag_Reorder             = (1<<8),
  UI_EventFlag_Secondary           = (1<<9),
};

typedef enum UI_EventDeltaUnit
{
  UI_EventDeltaUnit_Null,
  UI_EventDeltaUnit_Char,
  UI_EventDeltaUnit_Word,
  UI_EventDeltaUnit_Line,
  UI_EventDeltaUnit_Page,
  UI_EventDeltaUnit_Whole,
  UI_EventDeltaUnit_COUNT
}
UI_EventDeltaUnit;

typedef struct UI_Event UI_Event;
struct UI_Event
{
  UI_EventKind kind;
  UI_EventActionSlot slot;
  UI_EventFlags flags;
  UI_EventDeltaUnit delta_unit;
  OS_Key key;
  OS_Modifiers modifiers;
  String8 string;
  String8List paths;
  Vec2F32 pos;
  Vec2F32 delta_2f32;
  Vec2S32 delta_2s32;
  U64 timestamp_us;
};

typedef struct UI_EventNode UI_EventNode;
struct UI_EventNode
{
  UI_EventNode *next;
  UI_EventNode *prev;
  UI_Event v;
};

typedef struct UI_EventList UI_EventList;
struct UI_EventList
{
  UI_EventNode *first;
  UI_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Textual Operations

typedef U32 UI_TxtOpFlags;
enum
{
  UI_TxtOpFlag_Invalid = (1<<0),
  UI_TxtOpFlag_Copy    = (1<<1),
};

typedef struct UI_TxtOp UI_TxtOp;
struct UI_TxtOp
{
  UI_TxtOpFlags flags;
  String8 replace;
  String8 copy;
  TxtRng range;
  TxtPt cursor;
  TxtPt mark;
};

////////////////////////////////
//~ rjf: Keys

typedef struct UI_Key UI_Key;
struct UI_Key
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Sizes

typedef enum UI_SizeKind
{
  UI_SizeKind_Null,
  UI_SizeKind_Pixels,      // size is computed via a preferred pixel value
  UI_SizeKind_TextContent, // size is computed via the dimensions of box's rendered string
  UI_SizeKind_ParentPct,   // size is computed via a well-determined parent or grandparent size
  UI_SizeKind_ChildrenSum, // size is computed via summing well-determined sizes of children
}
UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size
{
  UI_SizeKind kind;
  F32 value;
  F32 strictness;
};

////////////////////////////////
//~ rjf: Themes

typedef struct UI_ThemePattern UI_ThemePattern;
struct UI_ThemePattern
{
  String8Array tags;
  Vec4F32 linear;
};

typedef struct UI_Theme UI_Theme;
struct UI_Theme
{
  UI_ThemePattern *patterns;
  U64 patterns_count;
};

////////////////////////////////
//~ rjf: Animation Info

typedef U32 UI_AnimationInfoFlags;
enum
{
  UI_AnimationInfoFlag_HotAnimations          = (1<<0),
  UI_AnimationInfoFlag_ActiveAnimations       = (1<<1),
  UI_AnimationInfoFlag_FocusAnimations        = (1<<2),
  UI_AnimationInfoFlag_TooltipAnimations      = (1<<3),
  UI_AnimationInfoFlag_ContextMenuAnimations  = (1<<4),
  UI_AnimationInfoFlag_ScrollingAnimations    = (1<<5),
  UI_AnimationInfoFlag_All = 0xffffffff,
};

typedef struct UI_AnimationInfo UI_AnimationInfo;
struct UI_AnimationInfo
{
  UI_AnimationInfoFlags flags;
};

////////////////////////////////
//~ rjf: Scroll Positions

typedef struct UI_ScrollPt UI_ScrollPt;
struct UI_ScrollPt
{
  S64 idx;
  F32 off;
};

typedef union UI_ScrollPt2 UI_ScrollPt2;
union UI_ScrollPt2
{
  UI_ScrollPt v[2];
  struct
  {
    UI_ScrollPt x;
    UI_ScrollPt y;
  };
};

////////////////////////////////
//~ rjf: Box Types

typedef enum UI_TextAlign
{
  UI_TextAlign_Left,
  UI_TextAlign_Center,
  UI_TextAlign_Right,
  UI_TextAlign_COUNT
}
UI_TextAlign;

struct UI_Box;
#define UI_BOX_CUSTOM_DRAW(name) void name(struct UI_Box *box, void *user_data)
typedef UI_BOX_CUSTOM_DRAW(UI_BoxCustomDrawFunctionType);

typedef U64 UI_BoxFlags;
//{
//- rjf: interaction
# define UI_BoxFlag_MouseClickable            (UI_BoxFlags)(1ull<<0)
# define UI_BoxFlag_KeyboardClickable         (UI_BoxFlags)(1ull<<1)
# define UI_BoxFlag_DropSite                  (UI_BoxFlags)(1ull<<2)
# define UI_BoxFlag_ClickToFocus              (UI_BoxFlags)(1ull<<3)
# define UI_BoxFlag_Scroll                    (UI_BoxFlags)(1ull<<4)
# define UI_BoxFlag_ViewScrollX               (UI_BoxFlags)(1ull<<5)
# define UI_BoxFlag_ViewScrollY               (UI_BoxFlags)(1ull<<6)
# define UI_BoxFlag_ViewClampX                (UI_BoxFlags)(1ull<<7)
# define UI_BoxFlag_ViewClampY                (UI_BoxFlags)(1ull<<8)
# define UI_BoxFlag_FocusHot                  (UI_BoxFlags)(1ull<<9)
# define UI_BoxFlag_FocusActive               (UI_BoxFlags)(1ull<<10)
# define UI_BoxFlag_FocusHotDisabled          (UI_BoxFlags)(1ull<<11)
# define UI_BoxFlag_FocusActiveDisabled       (UI_BoxFlags)(1ull<<12)
# define UI_BoxFlag_DefaultFocusNavX          (UI_BoxFlags)(1ull<<13)
# define UI_BoxFlag_DefaultFocusNavY          (UI_BoxFlags)(1ull<<14)
# define UI_BoxFlag_DefaultFocusEdit          (UI_BoxFlags)(1ull<<15)
# define UI_BoxFlag_FocusNavSkip              (UI_BoxFlags)(1ull<<16)
# define UI_BoxFlag_DisableTruncatedHover     (UI_BoxFlags)(1ull<<17)
# define UI_BoxFlag_Disabled                  (UI_BoxFlags)(1ull<<18)

//- rjf: layout
# define UI_BoxFlag_FloatingX                 (UI_BoxFlags)(1ull<<19)
# define UI_BoxFlag_FloatingY                 (UI_BoxFlags)(1ull<<20)
# define UI_BoxFlag_FixedWidth                (UI_BoxFlags)(1ull<<21)
# define UI_BoxFlag_FixedHeight               (UI_BoxFlags)(1ull<<22)
# define UI_BoxFlag_AllowOverflowX            (UI_BoxFlags)(1ull<<23)
# define UI_BoxFlag_AllowOverflowY            (UI_BoxFlags)(1ull<<24)
# define UI_BoxFlag_SkipViewOffX              (UI_BoxFlags)(1ull<<25)
# define UI_BoxFlag_SkipViewOffY              (UI_BoxFlags)(1ull<<26)

//- rjf: appearance / animation
# define UI_BoxFlag_DrawDropShadow            (UI_BoxFlags)(1ull<<27)
# define UI_BoxFlag_DrawBackgroundBlur        (UI_BoxFlags)(1ull<<28)
# define UI_BoxFlag_DrawBackground            (UI_BoxFlags)(1ull<<29)
# define UI_BoxFlag_DrawBorder                (UI_BoxFlags)(1ull<<30)
# define UI_BoxFlag_DrawSideTop               (UI_BoxFlags)(1ull<<31)
# define UI_BoxFlag_DrawSideBottom            (UI_BoxFlags)(1ull<<32)
# define UI_BoxFlag_DrawSideLeft              (UI_BoxFlags)(1ull<<33)
# define UI_BoxFlag_DrawSideRight             (UI_BoxFlags)(1ull<<34)
# define UI_BoxFlag_DrawText                  (UI_BoxFlags)(1ull<<35)
# define UI_BoxFlag_DrawTextFastpathCodepoint (UI_BoxFlags)(1ull<<36)
# define UI_BoxFlag_DrawTextWeak              (UI_BoxFlags)(1ull<<37)
# define UI_BoxFlag_DrawHotEffects            (UI_BoxFlags)(1ull<<38)
# define UI_BoxFlag_DrawActiveEffects         (UI_BoxFlags)(1ull<<39)
# define UI_BoxFlag_DrawOverlay               (UI_BoxFlags)(1ull<<40)
# define UI_BoxFlag_DrawBucket                (UI_BoxFlags)(1ull<<41)
# define UI_BoxFlag_Clip                      (UI_BoxFlags)(1ull<<42)
# define UI_BoxFlag_AnimatePosX               (UI_BoxFlags)(1ull<<43)
# define UI_BoxFlag_AnimatePosY               (UI_BoxFlags)(1ull<<44)
# define UI_BoxFlag_DisableTextTrunc          (UI_BoxFlags)(1ull<<45)
# define UI_BoxFlag_DisableIDString           (UI_BoxFlags)(1ull<<46)
# define UI_BoxFlag_DisableFocusBorder        (UI_BoxFlags)(1ull<<47)
# define UI_BoxFlag_DisableFocusOverlay       (UI_BoxFlags)(1ull<<48)
# define UI_BoxFlag_HasDisplayString          (UI_BoxFlags)(1ull<<49)
# define UI_BoxFlag_HasFuzzyMatchRanges       (UI_BoxFlags)(1ull<<50)
# define UI_BoxFlag_RoundChildrenByParent     (UI_BoxFlags)(1ull<<51)
# define UI_BoxFlag_SquishAnchored            (UI_BoxFlags)(1ull<<52)

//- rjf: debug
# define UI_BoxFlag_Debug                     (UI_BoxFlags)(1ull<<53)

//- rjf: bundles
# define UI_BoxFlag_Clickable           (UI_BoxFlag_MouseClickable|UI_BoxFlag_KeyboardClickable)
# define UI_BoxFlag_DefaultFocusNav     (UI_BoxFlag_DefaultFocusNavX|UI_BoxFlag_DefaultFocusNavY|UI_BoxFlag_DefaultFocusEdit)
# define UI_BoxFlag_Floating            (UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY)
# define UI_BoxFlag_FixedSize           (UI_BoxFlag_FixedWidth|UI_BoxFlag_FixedHeight)
# define UI_BoxFlag_AllowOverflow       (UI_BoxFlag_AllowOverflowX|UI_BoxFlag_AllowOverflowY)
# define UI_BoxFlag_AnimatePos          (UI_BoxFlag_AnimatePosX|UI_BoxFlag_AnimatePosY)
# define UI_BoxFlag_ViewScroll          (UI_BoxFlag_ViewScrollX|UI_BoxFlag_ViewScrollY)
# define UI_BoxFlag_ViewClamp           (UI_BoxFlag_ViewClampX|UI_BoxFlag_ViewClampY)
# define UI_BoxFlag_DisableFocusEffects (UI_BoxFlag_DisableFocusBorder|UI_BoxFlag_DisableFocusOverlay)
//}

typedef struct UI_Box UI_Box;
struct UI_Box
{
  //- rjf: persistent links
  UI_Box *hash_next;
  UI_Box *hash_prev;
  
  //- rjf: per-build links/data
  UI_Box *first;
  UI_Box *last;
  UI_Box *next;
  UI_Box *prev;
  UI_Box *parent;
  U64 child_count;
  
  //- rjf: per-build equipment
  UI_Key key;
  UI_BoxFlags flags;
  UI_Key tags_key;
  String8 string;
  UI_TextAlign text_align;
  Vec2F32 fixed_position;
  Vec2F32 fixed_size;
  Vec2F32 min_size;
  UI_Size pref_size[Axis2_COUNT];
  Axis2 child_layout_axis;
  OS_Cursor hover_cursor;
  U32 fastpath_codepoint;
  UI_Key group_key;
  DR_Bucket *draw_bucket;
  UI_BoxCustomDrawFunctionType *custom_draw;
  void *custom_draw_user_data;
  Vec4F32 background_color;
  Vec4F32 text_color;
  FNT_Tag font;
  F32 font_size;
  F32 tab_size;
  FNT_RasterFlags text_raster_flags;
  F32 corner_radii[Corner_COUNT];
  F32 blur_size;
  F32 transparency;
  F32 squish;
  F32 text_padding;
  
  //- rjf: per-build artifacts
  DR_FStrList display_fstrs;
  DR_FRunList display_fruns;
  Rng2F32 rect;
  Vec2F32 fixed_position_animated;
  Vec2F32 position_delta;
  FuzzyMatchRangeList fuzzy_match_ranges;
  
  //- rjf: persistent data
  U64 first_touched_build_index;
  U64 last_touched_build_index;
  U64 first_disabled_build_index;
  F32 hot_t;
  F32 active_t;
  F32 disabled_t;
  F32 focus_hot_t;
  F32 focus_active_t;
  F32 focus_active_disabled_t;
  Vec2F32 view_off;
  Vec2F32 view_off_target;
  Vec2F32 view_bounds;
  UI_Key default_nav_focus_hot_key;
  UI_Key default_nav_focus_active_key;
  UI_Key default_nav_focus_next_hot_key;
  UI_Key default_nav_focus_next_active_key;
};

typedef struct UI_BoxRec UI_BoxRec;
struct UI_BoxRec
{
  UI_Box *next;
  S32 push_count;
  S32 pop_count;
};

typedef struct UI_BoxNode UI_BoxNode;
struct UI_BoxNode
{
  UI_BoxNode *next;
  UI_Box *box;
};

typedef struct UI_BoxList UI_BoxList;
struct UI_BoxList
{
  UI_BoxNode *first;
  UI_BoxNode *last;
  U64 count;
};

typedef U32 UI_SignalFlags;
enum
{
  // rjf: mouse press -> box was pressed while hovering
  UI_SignalFlag_LeftPressed         = (1<<0),
  UI_SignalFlag_MiddlePressed       = (1<<1),
  UI_SignalFlag_RightPressed        = (1<<2),
  
  // rjf: dragging -> box was previously pressed, user is still holding button
  UI_SignalFlag_LeftDragging        = (1<<3),
  UI_SignalFlag_MiddleDragging      = (1<<4),
  UI_SignalFlag_RightDragging       = (1<<5),
  
  // rjf: double-dragging -> box was previously double-clicked, user is still holding button
  UI_SignalFlag_LeftDoubleDragging  = (1<<6),
  UI_SignalFlag_MiddleDoubleDragging= (1<<7),
  UI_SignalFlag_RightDoubleDragging = (1<<8),
  
  // rjf: triple-dragging -> box was previously triple-clicked, user is still holding button
  UI_SignalFlag_LeftTripleDragging  = (1<<9),
  UI_SignalFlag_MiddleTripleDragging= (1<<10),
  UI_SignalFlag_RightTripleDragging = (1<<11),
  
  // rjf: released -> box was previously pressed & user released, in or out of bounds
  UI_SignalFlag_LeftReleased        = (1<<12),
  UI_SignalFlag_MiddleReleased      = (1<<13),
  UI_SignalFlag_RightReleased       = (1<<14),
  
  // rjf: clicked -> box was previously pressed & user released, in bounds
  UI_SignalFlag_LeftClicked         = (1<<15),
  UI_SignalFlag_MiddleClicked       = (1<<16),
  UI_SignalFlag_RightClicked        = (1<<17),
  
  // rjf: double clicked -> box was previously clicked, pressed again
  UI_SignalFlag_LeftDoubleClicked   = (1<<18),
  UI_SignalFlag_MiddleDoubleClicked = (1<<19),
  UI_SignalFlag_RightDoubleClicked  = (1<<20),
  
  // rjf: triple clicked -> box was previously clicked twice, pressed again
  UI_SignalFlag_LeftTripleClicked   = (1<<21),
  UI_SignalFlag_MiddleTripleClicked = (1<<22),
  UI_SignalFlag_RightTripleClicked  = (1<<23),
  
  // rjf: keyboard pressed -> box had focus, user activated via their keyboard
  UI_SignalFlag_KeyboardPressed     = (1<<24),
  
  // rjf: passive mouse info
  UI_SignalFlag_Hovering            = (1<<25), // hovering specifically this box
  UI_SignalFlag_MouseOver           = (1<<26), // mouse is over, but may be occluded
  
  // rjf: committing state changes via user interaction
  UI_SignalFlag_Commit              = (1<<27),
  
  // rjf: high-level combos
  UI_SignalFlag_Pressed = UI_SignalFlag_LeftPressed|UI_SignalFlag_KeyboardPressed,
  UI_SignalFlag_Released = UI_SignalFlag_LeftReleased,
  UI_SignalFlag_Clicked = UI_SignalFlag_LeftClicked|UI_SignalFlag_KeyboardPressed,
  UI_SignalFlag_DoubleClicked = UI_SignalFlag_LeftDoubleClicked,
  UI_SignalFlag_TripleClicked = UI_SignalFlag_LeftTripleClicked,
  UI_SignalFlag_Dragging = UI_SignalFlag_LeftDragging,
};

typedef struct UI_Signal UI_Signal;
struct UI_Signal
{
  UI_Box *box;
  OS_Modifiers event_flags;
  Vec2S16 scroll;
  UI_SignalFlags f;
};

#define ui_pressed(s)        !!((s).f&UI_SignalFlag_Pressed)
#define ui_clicked(s)        !!((s).f&UI_SignalFlag_Clicked)
#define ui_released(s)       !!((s).f&UI_SignalFlag_Released)
#define ui_double_clicked(s) !!((s).f&UI_SignalFlag_DoubleClicked)
#define ui_triple_clicked(s) !!((s).f&UI_SignalFlag_TripleClicked)
#define ui_middle_clicked(s) !!((s).f&UI_SignalFlag_MiddleClicked)
#define ui_right_clicked(s)  !!((s).f&UI_SignalFlag_RightClicked)
#define ui_dragging(s)       !!((s).f&UI_SignalFlag_Dragging)
#define ui_hovering(s)       !!((s).f&UI_SignalFlag_Hovering)
#define ui_mouse_over(s)     !!((s).f&UI_SignalFlag_MouseOver)
#define ui_committed(s)      !!((s).f&UI_SignalFlag_Commit)

typedef struct UI_Nav UI_Nav;
struct UI_Nav
{
  B32 moved;
  Vec2S64 new_p;
};

////////////////////////////////
//~ rjf: Animation State Types

typedef struct UI_AnimParams UI_AnimParams;
struct UI_AnimParams
{
  F32 initial;
  F32 target;
  F32 rate;
  F32 epsilon;
  B32 reset;
};

typedef struct UI_AnimNode UI_AnimNode;
struct UI_AnimNode
{
  UI_AnimNode *slot_next;
  UI_AnimNode *slot_prev;
  UI_AnimNode *lru_next;
  UI_AnimNode *lru_prev;
  U64 first_touched_build_index;
  U64 last_touched_build_index;
  UI_Key key;
  UI_AnimParams params;
  F32 current;
};

typedef struct UI_AnimSlot UI_AnimSlot;
struct UI_AnimSlot
{
  UI_AnimNode *first;
  UI_AnimNode *last;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/ui.meta.h"

////////////////////////////////
//~ rjf: State Types

//- rjf: cache for mapping 64-bit key -> array of tags

typedef struct UI_TagsCacheNode UI_TagsCacheNode;
struct UI_TagsCacheNode
{
  UI_TagsCacheNode *next;
  UI_Key key;
  String8Array tags;
};

typedef struct UI_TagsCacheSlot UI_TagsCacheSlot;
struct UI_TagsCacheSlot
{
  UI_TagsCacheNode *first;
  UI_TagsCacheNode *last;
};

typedef struct UI_TagsKeyStackNode UI_TagsKeyStackNode;
struct UI_TagsKeyStackNode
{
  UI_TagsKeyStackNode *next;
  UI_Key key;
};

//- rjf: cache for mapping 64-bit key * string -> theme pattern

typedef struct UI_ThemePatternCacheNode UI_ThemePatternCacheNode;
struct UI_ThemePatternCacheNode
{
  UI_ThemePatternCacheNode *next;
  UI_Key key;
  UI_ThemePattern *pattern;
};

typedef struct UI_ThemePatternCacheSlot UI_ThemePatternCacheSlot;
struct UI_ThemePatternCacheSlot
{
  UI_ThemePatternCacheNode *first;
  UI_ThemePatternCacheNode *last;
};

//- rjf: cache for mapping 64-bit key -> box

typedef struct UI_BoxHashSlot UI_BoxHashSlot;
struct UI_BoxHashSlot
{
  UI_Box *hash_first;
  UI_Box *hash_last;
};

//- rjf: main state bundle

typedef struct UI_State UI_State;
struct UI_State
{
  //- rjf: main arena
  Arena *arena;
  
  //- rjf: fixed keys
  UI_Key external_key;
  
  //- rjf: build arenas
  Arena *build_arenas[2];
  U64 build_index;
  
  //- rjf: box cache
  UI_Box *first_free_box;
  U64 box_table_size;
  UI_BoxHashSlot *box_table;
  
  //- rjf: anim cache
  UI_AnimNode *free_anim_node;
  UI_AnimNode *lru_anim_node;
  UI_AnimNode *mru_anim_node;
  U64 anim_slots_count;
  UI_AnimSlot *anim_slots;
  
  //- rjf: build state machine state
  B32 is_in_open_ctx_menu;
  B32 tooltip_can_overflow_window;
  UI_Key tooltip_anchor_key;
  String8Array current_gen_tags;
  U64 current_gen_tags_gen;
  UI_TagsKeyStackNode *tags_key_stack_top;
  UI_TagsKeyStackNode *tags_key_stack_free;
  U64 tags_cache_slots_count;
  UI_TagsCacheSlot *tags_cache_slots;
  U64 theme_pattern_cache_slots_count;
  UI_ThemePatternCacheSlot *theme_pattern_cache_slots;
  
  //- rjf: build phase output
  UI_Box *root;
  UI_Box *tooltip_root;
  UI_Box *ctx_menu_root;
  UI_Key default_nav_root_key;
  U64 build_box_count;
  U64 last_build_box_count;
  B32 ctx_menu_touched_this_frame;
  B32 is_animating;
  
  //- rjf: build parameters
  UI_IconInfo icon_info;
  UI_Theme *theme;
  UI_AnimationInfo animation_info;
  OS_Handle window;
  UI_EventList *events;
  Vec2F32 mouse;
  F32 animation_dt;
  F32 default_animation_rate;
  
  //- rjf: user interaction state
  UI_Key hot_box_key;
  UI_Key active_box_key[UI_MouseButtonKind_COUNT];
  UI_Key drop_hot_box_key;
  UI_Key clipboard_copy_key;
  U64 press_timestamp_history_us[UI_MouseButtonKind_COUNT][3];
  UI_Key press_key_history[UI_MouseButtonKind_COUNT][3];
  Vec2F32 press_pos_history[UI_MouseButtonKind_COUNT][3];
  Vec2F32 drag_start_mouse;
  Arena *drag_state_arena;
  String8 drag_state_data;
  Arena *string_hover_arena;
  String8 string_hover_string;
  F32 string_hover_size;
  DR_FStrList string_hover_fstrs;
  U64 string_hover_begin_us;
  U64 string_hover_build_index;
  U64 last_time_mousemoved_us;
  
  //- rjf: tooltip state
  F32 tooltip_open_t;
  B32 tooltip_open;
  
  //- rjf: context menu state
  UI_Key ctx_menu_anchor_key;
  UI_Key next_ctx_menu_anchor_key;
  Vec2F32 ctx_menu_anchor_box_last_pos;
  Vec2F32 ctx_menu_anchor_off;
  B32 ctx_menu_open;
  B32 next_ctx_menu_open;
  F32 ctx_menu_open_t;
  UI_Key ctx_menu_key;
  B32 ctx_menu_changed;
  
  //- rjf: build phase stacks
  UI_DeclStackNils;
  UI_DeclStacks;
};

////////////////////////////////
//~ rjf: Basic Type Functions

internal U64     ui_hash_from_string(U64 seed, String8 string);
internal String8 ui_hash_part_from_key_string(String8 string);
internal String8 ui_display_part_from_key_string(String8 string);
internal UI_Key  ui_key_zero(void);
internal UI_Key  ui_key_make(U64 v);
internal UI_Key  ui_key_from_string(UI_Key seed_key, String8 string);
internal UI_Key  ui_key_from_stringf(UI_Key seed_key, char *fmt, ...);
internal B32     ui_key_match(UI_Key a, UI_Key b);

////////////////////////////////
//~ rjf: Event Type Functions

internal UI_EventNode *ui_event_list_push(Arena *arena, UI_EventList *list, UI_Event *v);
internal void ui_eat_event_node(UI_EventList *list, UI_EventNode *node);

////////////////////////////////
//~ rjf: Text Operation Functions

internal B32 ui_char_is_scan_boundary(U8 c);
internal S64 ui_scanned_column_from_column(String8 string, S64 start_column, Side side);
internal UI_TxtOp ui_single_line_txt_op_from_event(Arena *arena, UI_Event *event, String8 string, TxtPt cursor, TxtPt mark);
internal String8 ui_push_string_replace_range(Arena *arena, String8 string, Rng1S64 range, String8 replace);

////////////////////////////////
//~ rjf: Size Type Functions

internal UI_Size ui_size(UI_SizeKind kind, F32 value, F32 strictness);
#define ui_px(value, strictness)         ui_size(UI_SizeKind_Pixels, value, strictness)
#define ui_em(value, strictness)         ui_size(UI_SizeKind_Pixels, (value) * ui_top_font_size(), strictness)
#define ui_text_dim(padding, strictness) ui_size(UI_SizeKind_TextContent, padding, strictness)
#define ui_pct(value, strictness)        ui_size(UI_SizeKind_ParentPct, value, strictness)
#define ui_children_sum(strictness)      ui_size(UI_SizeKind_ChildrenSum, 0.f, strictness)

////////////////////////////////
//~ rjf: Scroll Point Type Functions

internal UI_ScrollPt ui_scroll_pt(S64 idx, F32 off);
internal void ui_scroll_pt_target_idx(UI_ScrollPt *v, S64 idx);
internal void ui_scroll_pt_clamp_idx(UI_ScrollPt *v, Rng1S64 range);

////////////////////////////////
//~ rjf: Box Type Functions

read_only global UI_Box ui_nil_box =
{
  &ui_nil_box,
  &ui_nil_box,
  &ui_nil_box,
  &ui_nil_box,
  &ui_nil_box,
  &ui_nil_box,
  &ui_nil_box,
};
internal B32 ui_box_is_nil(UI_Box *box);
internal UI_BoxRec ui_box_rec_df(UI_Box *box, UI_Box *root, U64 sib_member_off, U64 child_member_off);
#define ui_box_rec_df_pre(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, next), OffsetOf(UI_Box, first))
#define ui_box_rec_df_post(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, prev), OffsetOf(UI_Box, last))
internal void ui_box_list_push(Arena *arena, UI_BoxList *list, UI_Box *box);

////////////////////////////////
//~ rjf: State Allocating / Selection

internal UI_State *ui_state_alloc(void);
internal void      ui_state_release(UI_State *state);
internal UI_Box *  ui_root_from_state(UI_State *state);
internal B32       ui_animating_from_state(UI_State *state);
internal void      ui_select_state(UI_State *state);
internal UI_State *ui_get_selected_state(void);

////////////////////////////////
//~ rjf: Implicit State Accessors/Mutators

//- rjf: per-frame info
internal Arena *           ui_build_arena(void);
internal OS_Handle         ui_window(void);
internal Vec2F32           ui_mouse(void);
internal FNT_Tag             ui_icon_font(void);
internal String8           ui_icon_string_from_kind(UI_IconKind icon_kind);
internal F32               ui_dt(void);

//- rjf: event pumping
internal B32 ui_next_event(UI_Event **ev);
internal void ui_eat_event(UI_Event *ev);

//- rjf: event consumption helpers
internal B32 ui_key_press(OS_Modifiers mods, OS_Key key);
internal B32 ui_key_release(OS_Modifiers mods, OS_Key key);
internal B32 ui_text(U32 character);
internal B32 ui_slot_press(UI_EventActionSlot slot);

//- rjf: drag data
internal Vec2F32           ui_drag_start_mouse(void);
internal Vec2F32           ui_drag_delta(void);
internal void              ui_store_drag_data(String8 string);
internal String8           ui_get_drag_data(U64 min_required_size);
#define ui_store_drag_struct(ptr) ui_store_drag_data(str8_struct(ptr))
#define ui_get_drag_struct(type) ((type *)ui_get_drag_data(sizeof(type)).str)

//- rjf: hovered string info
internal B32                ui_string_hover_active(void);
internal DR_FStrList ui_string_hover_fstrs(Arena *arena);

//- rjf: interaction keys
internal UI_Key            ui_hot_key(void);
internal UI_Key            ui_active_key(UI_MouseButtonKind button_kind);
internal UI_Key            ui_drop_hot_key(void);

//- rjf: controls over interaction
internal void              ui_kill_action(void);

//- rjf: box cache lookup
internal UI_Box *          ui_box_from_key(UI_Key key);

////////////////////////////////
//~ rjf: Top-Level Building API

internal void ui_begin_build(OS_Handle window, UI_EventList *events, UI_IconInfo *icon_info, UI_Theme *theme, UI_AnimationInfo *animation_info, F32 real_dt, F32 animation_dt);
internal void ui_end_build(void);
internal void ui_calc_sizes_standalone__in_place(UI_Box *root, Axis2 axis);
internal void ui_calc_sizes_upwards_dependent__in_place(UI_Box *root, Axis2 axis);
internal void ui_calc_sizes_downwards_dependent__in_place(UI_Box *root, Axis2 axis);
internal void ui_layout_enforce_constraints__in_place(UI_Box *root, Axis2 axis);
internal void ui_layout_position__in_place(UI_Box *root, Axis2 axis);
internal void ui_layout_root(UI_Box *root, Axis2 axis);

////////////////////////////////
//~ rjf: Box Tree Building API

//- rjf: spacers
internal UI_Signal         ui_spacer(UI_Size size);

//- rjf: tooltips
internal void              ui_tooltip_begin_base(void);
internal void              ui_tooltip_end_base(void);
internal void              ui_tooltip_begin(void);
internal void              ui_tooltip_end(void);

//- rjf: context menus
internal void              ui_ctx_menu_open(UI_Key key, UI_Key anchor_box_key, Vec2F32 anchor_off);
internal void              ui_ctx_menu_close(void);
internal B32               ui_begin_ctx_menu(UI_Key key);
internal void              ui_end_ctx_menu(void);
internal B32               ui_ctx_menu_is_open(UI_Key key);
internal B32               ui_any_ctx_menu_is_open(void);

//- rjf: focus tree coloring
internal B32               ui_is_focus_hot(void);
internal B32               ui_is_focus_active(void);

//- rjf: implicit auto-managed tree-based focus state
internal B32               ui_is_key_auto_focus_active(UI_Key key);
internal B32               ui_is_key_auto_focus_hot(UI_Key key);
internal void              ui_set_auto_focus_active_key(UI_Key key);
internal void              ui_set_auto_focus_hot_key(UI_Key key);

//- rjf: current style tags key
internal UI_Key            ui_top_tags_key(void);

//- rjf: theme color lookups
internal Vec4F32           ui_color_from_name(String8 name);
internal Vec4F32           ui_color_from_tags_key_name(UI_Key key, String8 name);

//- rjf: box node construction
internal UI_Box *          ui_build_box_from_key(UI_BoxFlags flags, UI_Key key);
internal UI_Key            ui_active_seed_key(void);
internal UI_Box *          ui_build_box_from_string(UI_BoxFlags flags, String8 string);
internal UI_Box *          ui_build_box_from_stringf(UI_BoxFlags flags, char *fmt, ...);

//- rjf: box node equipment
internal inline void       ui_box_equip_display_string(UI_Box *box, String8 string);
internal inline void       ui_box_equip_display_fstrs(UI_Box *box, DR_FStrList *strings);
internal inline void       ui_box_equip_fuzzy_match_ranges(UI_Box *box, FuzzyMatchRangeList *matches);
internal inline void       ui_box_equip_draw_bucket(UI_Box *box, DR_Bucket *bucket);
internal inline void       ui_box_equip_custom_draw(UI_Box *box, UI_BoxCustomDrawFunctionType *custom_draw, void *user_data);

//- rjf: box accessors / queries
internal String8           ui_box_display_string(UI_Box *box);
internal Vec2F32           ui_box_text_position(UI_Box *box);
internal U64               ui_box_char_pos_from_xy(UI_Box *box, Vec2F32 xy);

////////////////////////////////
//~ rjf: User Interaction

internal UI_Signal ui_signal_from_box(UI_Box *box);

////////////////////////////////
//~ rjf: Animation Cache Interaction API

read_only global UI_AnimNode ui_nil_anim_node =
{
  &ui_nil_anim_node,
  &ui_nil_anim_node,
};

internal F32 ui_anim_(UI_Key key, UI_AnimParams *params);
#define ui_anim(key, target_val, ...) ui_anim_((key), &(UI_AnimParams){.target = (target_val), .rate = (ui_state->default_animation_rate), __VA_ARGS__})

////////////////////////////////
//~ rjf: Stacks

internal void ui__push_tags_key_from_appended_string(String8 string);
internal void ui__pop_tags_key(void);

//- rjf: base
internal UI_Box *                   ui_top_parent(void);
internal Axis2                      ui_top_child_layout_axis(void);
internal F32                        ui_top_fixed_x(void);
internal F32                        ui_top_fixed_y(void);
internal F32                        ui_top_fixed_width(void);
internal F32                        ui_top_fixed_height(void);
internal UI_Size                    ui_top_pref_width(void);
internal UI_Size                    ui_top_pref_height(void);
internal UI_PermissionFlags         ui_top_permission_flags(void);
internal UI_BoxFlags                ui_top_flags(void);
internal UI_BoxFlags                ui_top_omit_flags(void);
internal UI_FocusKind               ui_top_focus_hot(void);
internal UI_FocusKind               ui_top_focus_active(void);
internal U32                        ui_top_fastpath_codepoint(void);
internal UI_Key                     ui_top_group_key(void);
internal F32                        ui_top_transparency(void);
internal String8                    ui_top_tag(void);
internal Vec4F32                    ui_top_background_color(void);
internal Vec4F32                    ui_top_text_color(void);
internal F32                        ui_top_squish(void);
internal OS_Cursor                  ui_top_hover_cursor(void);
internal FNT_Tag                    ui_top_font(void);
internal F32                        ui_top_font_size(void);
internal FNT_RasterFlags            ui_top_text_raster_flags(void);
internal F32                        ui_top_tab_size(void);
internal F32                        ui_top_corner_radius_00(void);
internal F32                        ui_top_corner_radius_01(void);
internal F32                        ui_top_corner_radius_10(void);
internal F32                        ui_top_corner_radius_11(void);
internal F32                        ui_top_blur_size(void);
internal F32                        ui_top_text_padding(void);
internal UI_TextAlign               ui_top_text_alignment(void);
internal UI_Box *                   ui_bottom_parent(void);
internal Axis2                      ui_bottom_child_layout_axis(void);
internal F32                        ui_bottom_fixed_x(void);
internal F32                        ui_bottom_fixed_y(void);
internal F32                        ui_bottom_fixed_width(void);
internal F32                        ui_bottom_fixed_height(void);
internal UI_Size                    ui_bottom_pref_width(void);
internal UI_Size                    ui_bottom_pref_height(void);
internal UI_PermissionFlags         ui_bottom_permission_flags(void);
internal UI_BoxFlags                ui_bottom_flags(void);
internal UI_BoxFlags                ui_bottom_omit_flags(void);
internal UI_FocusKind               ui_bottom_focus_hot(void);
internal UI_FocusKind               ui_bottom_focus_active(void);
internal U32                        ui_bottom_fastpath_codepoint(void);
internal UI_Key                     ui_bottom_group_key(void);
internal F32                        ui_bottom_transparency(void);
internal String8                    ui_bottom_tag(void);
internal Vec4F32                    ui_bottom_background_color(void);
internal Vec4F32                    ui_bottom_text_color(void);
internal F32                        ui_bottom_squish(void);
internal OS_Cursor                  ui_bottom_hover_cursor(void);
internal FNT_Tag                    ui_bottom_font(void);
internal F32                        ui_bottom_font_size(void);
internal FNT_RasterFlags            ui_bottom_text_raster_flags(void);
internal F32                        ui_bottom_tab_size(void);
internal F32                        ui_bottom_corner_radius_00(void);
internal F32                        ui_bottom_corner_radius_01(void);
internal F32                        ui_bottom_corner_radius_10(void);
internal F32                        ui_bottom_corner_radius_11(void);
internal F32                        ui_bottom_blur_size(void);
internal F32                        ui_bottom_text_padding(void);
internal UI_TextAlign               ui_bottom_text_alignment(void);
internal UI_Box *                   ui_push_parent(UI_Box * v);
internal Axis2                      ui_push_child_layout_axis(Axis2 v);
internal F32                        ui_push_fixed_x(F32 v);
internal F32                        ui_push_fixed_y(F32 v);
internal F32                        ui_push_fixed_width(F32 v);
internal F32                        ui_push_fixed_height(F32 v);
internal UI_Size                    ui_push_pref_width(UI_Size v);
internal UI_Size                    ui_push_pref_height(UI_Size v);
internal UI_PermissionFlags         ui_push_permission_flags(UI_PermissionFlags v);
internal UI_BoxFlags                ui_push_flags(UI_BoxFlags v);
internal UI_BoxFlags                ui_push_omit_flags(UI_BoxFlags v);
internal UI_FocusKind               ui_push_focus_hot(UI_FocusKind v);
internal UI_FocusKind               ui_push_focus_active(UI_FocusKind v);
internal U32                        ui_push_fastpath_codepoint(U32 v);
internal UI_Key                     ui_push_group_key(UI_Key v);
internal F32                        ui_push_transparency(F32 v);
internal String8                    ui_push_tag(String8 v);
internal Vec4F32                    ui_push_background_color(Vec4F32 v);
internal Vec4F32                    ui_push_text_color(Vec4F32 v);
internal F32                        ui_push_squish(F32 v);
internal OS_Cursor                  ui_push_hover_cursor(OS_Cursor v);
internal FNT_Tag                    ui_push_font(FNT_Tag v);
internal F32                        ui_push_font_size(F32 v);
internal FNT_RasterFlags            ui_push_text_raster_flags(FNT_RasterFlags v);
internal F32                        ui_push_tab_size(F32 v);
internal F32                        ui_push_corner_radius_00(F32 v);
internal F32                        ui_push_corner_radius_01(F32 v);
internal F32                        ui_push_corner_radius_10(F32 v);
internal F32                        ui_push_corner_radius_11(F32 v);
internal F32                        ui_push_blur_size(F32 v);
internal F32                        ui_push_text_padding(F32 v);
internal UI_TextAlign               ui_push_text_alignment(UI_TextAlign v);
internal UI_Box *                   ui_pop_parent(void);
internal Axis2                      ui_pop_child_layout_axis(void);
internal F32                        ui_pop_fixed_x(void);
internal F32                        ui_pop_fixed_y(void);
internal F32                        ui_pop_fixed_width(void);
internal F32                        ui_pop_fixed_height(void);
internal UI_Size                    ui_pop_pref_width(void);
internal UI_Size                    ui_pop_pref_height(void);
internal UI_PermissionFlags         ui_pop_permission_flags(void);
internal UI_BoxFlags                ui_pop_flags(void);
internal UI_BoxFlags                ui_pop_omit_flags(void);
internal UI_FocusKind               ui_pop_focus_hot(void);
internal UI_FocusKind               ui_pop_focus_active(void);
internal U32                        ui_pop_fastpath_codepoint(void);
internal UI_Key                     ui_pop_group_key(void);
internal F32                        ui_pop_transparency(void);
internal String8                    ui_pop_tag(void);
internal Vec4F32                    ui_pop_background_color(void);
internal Vec4F32                    ui_pop_text_color(void);
internal F32                        ui_pop_squish(void);
internal OS_Cursor                  ui_pop_hover_cursor(void);
internal FNT_Tag                    ui_pop_font(void);
internal F32                        ui_pop_font_size(void);
internal FNT_RasterFlags            ui_pop_text_raster_flags(void);
internal F32                        ui_pop_tab_size(void);
internal F32                        ui_pop_corner_radius_00(void);
internal F32                        ui_pop_corner_radius_01(void);
internal F32                        ui_pop_corner_radius_10(void);
internal F32                        ui_pop_corner_radius_11(void);
internal F32                        ui_pop_blur_size(void);
internal F32                        ui_pop_text_padding(void);
internal UI_TextAlign               ui_pop_text_alignment(void);
internal UI_Box *                   ui_set_next_parent(UI_Box * v);
internal Axis2                      ui_set_next_child_layout_axis(Axis2 v);
internal F32                        ui_set_next_fixed_x(F32 v);
internal F32                        ui_set_next_fixed_y(F32 v);
internal F32                        ui_set_next_fixed_width(F32 v);
internal F32                        ui_set_next_fixed_height(F32 v);
internal UI_Size                    ui_set_next_pref_width(UI_Size v);
internal UI_Size                    ui_set_next_pref_height(UI_Size v);
internal UI_PermissionFlags         ui_set_next_permission_flags(UI_PermissionFlags v);
internal UI_BoxFlags                ui_set_next_flags(UI_BoxFlags v);
internal UI_BoxFlags                ui_set_next_omit_flags(UI_BoxFlags v);
internal UI_FocusKind               ui_set_next_focus_hot(UI_FocusKind v);
internal UI_FocusKind               ui_set_next_focus_active(UI_FocusKind v);
internal U32                        ui_set_next_fastpath_codepoint(U32 v);
internal UI_Key                     ui_set_next_group_key(UI_Key v);
internal F32                        ui_set_next_transparency(F32 v);
internal String8                    ui_set_next_tag(String8 v);
internal Vec4F32                    ui_set_next_background_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_text_color(Vec4F32 v);
internal F32                        ui_set_next_squish(F32 v);
internal OS_Cursor                  ui_set_next_hover_cursor(OS_Cursor v);
internal FNT_Tag                    ui_set_next_font(FNT_Tag v);
internal F32                        ui_set_next_font_size(F32 v);
internal FNT_RasterFlags            ui_set_next_text_raster_flags(FNT_RasterFlags v);
internal F32                        ui_set_next_tab_size(F32 v);
internal F32                        ui_set_next_corner_radius_00(F32 v);
internal F32                        ui_set_next_corner_radius_01(F32 v);
internal F32                        ui_set_next_corner_radius_10(F32 v);
internal F32                        ui_set_next_corner_radius_11(F32 v);
internal F32                        ui_set_next_blur_size(F32 v);
internal F32                        ui_set_next_text_padding(F32 v);
internal UI_TextAlign               ui_set_next_text_alignment(UI_TextAlign v);

//- rjf: helpers
internal Rng2F32  ui_push_rect(Rng2F32 rect);
internal Rng2F32  ui_pop_rect(void);
internal void     ui_set_next_rect(Rng2F32 rect);
internal UI_Size  ui_push_pref_size(Axis2 axis, UI_Size v);
internal UI_Size  ui_pop_pref_size(Axis2 axis);
internal UI_Size  ui_set_next_pref_size(Axis2 axis, UI_Size v);
internal void     ui_push_corner_radius(F32 v);
internal void     ui_pop_corner_radius(void);
internal void     ui_push_tagf(char *fmt, ...);
internal F32      ui_top_px_height(void);

////////////////////////////////
//~ rjf: Macro Loop Wrappers

//- rjf: stacks (base)
#define UI_Parent(v) DeferLoop(ui_push_parent(v), ui_pop_parent())
#define UI_ChildLayoutAxis(v) DeferLoop(ui_push_child_layout_axis(v), ui_pop_child_layout_axis())
#define UI_FixedX(v) DeferLoop(ui_push_fixed_x(v), ui_pop_fixed_x())
#define UI_FixedY(v) DeferLoop(ui_push_fixed_y(v), ui_pop_fixed_y())
#define UI_FixedWidth(v) DeferLoop(ui_push_fixed_width(v), ui_pop_fixed_width())
#define UI_FixedHeight(v) DeferLoop(ui_push_fixed_height(v), ui_pop_fixed_height())
#define UI_PrefWidth(v) DeferLoop(ui_push_pref_width(v), ui_pop_pref_width())
#define UI_PrefHeight(v) DeferLoop(ui_push_pref_height(v), ui_pop_pref_height())
#define UI_MinWidth(v) DeferLoop(ui_push_min_width(v), ui_pop_min_width())
#define UI_MinHeight(v) DeferLoop(ui_push_min_height(v), ui_pop_min_height())
#define UI_PermissionFlags(v) DeferLoop(ui_push_permission_flags(v), ui_pop_permission_flags())
#define UI_Flags(v) DeferLoop(ui_push_flags(v), ui_pop_flags())
#define UI_OmitFlags(v) DeferLoop(ui_push_omit_flags(v), ui_pop_omit_flags())
#define UI_FocusHot(v) DeferLoop(ui_push_focus_hot(v), ui_pop_focus_hot())
#define UI_FocusActive(v) DeferLoop(ui_push_focus_active(v), ui_pop_focus_active())
#define UI_FastpathCodepoint(v) DeferLoop(ui_push_fastpath_codepoint(v), ui_pop_fastpath_codepoint())
#define UI_GroupKey(v) DeferLoop(ui_push_group_key(v), ui_pop_group_key())
#define UI_Transparency(v) DeferLoop(ui_push_transparency(v), ui_pop_transparency())
#define UI_Tag(v) DeferLoop(ui_push_tag(v), ui_pop_tag())
#define UI_BackgroundColor(v) DeferLoop(ui_push_background_color(v), ui_pop_background_color())
#define UI_TextColor(v) DeferLoop(ui_push_text_color(v), ui_pop_text_color())
#define UI_Squish(v) DeferLoop(ui_push_squish(v), ui_pop_squish())
#define UI_HoverCursor(v) DeferLoop(ui_push_hover_cursor(v), ui_pop_hover_cursor())
#define UI_Font(v) DeferLoop(ui_push_font(v), ui_pop_font())
#define UI_FontSize(v) DeferLoop(ui_push_font_size(v), ui_pop_font_size())
#define UI_TextRasterFlags(v) DeferLoop(ui_push_text_raster_flags(v), ui_pop_text_raster_flags())
#define UI_TabSize(v) DeferLoop(ui_push_tab_size(v), ui_pop_tab_size())
#define UI_CornerRadius00(v) DeferLoop(ui_push_corner_radius_00(v), ui_pop_corner_radius_00())
#define UI_CornerRadius01(v) DeferLoop(ui_push_corner_radius_01(v), ui_pop_corner_radius_01())
#define UI_CornerRadius10(v) DeferLoop(ui_push_corner_radius_10(v), ui_pop_corner_radius_10())
#define UI_CornerRadius11(v) DeferLoop(ui_push_corner_radius_11(v), ui_pop_corner_radius_11())
#define UI_BlurSize(v) DeferLoop(ui_push_blur_size(v), ui_pop_blur_size())
#define UI_TextPadding(v) DeferLoop(ui_push_text_padding(v), ui_pop_text_padding())
#define UI_TextAlignment(v) DeferLoop(ui_push_text_alignment(v), ui_pop_text_alignment())

//- rjf: stacks (compositions)
#define UI_FixedPos(v)       DeferLoop((ui_push_fixed_x((v).x), ui_push_fixed_y((v).y)), (ui_pop_fixed_x(), ui_pop_fixed_y()))
#define UI_FixedSize(v)      DeferLoop((ui_push_fixed_width((v).x), ui_push_fixed_height((v).y)), (ui_pop_fixed_width(), ui_pop_fixed_height()))
#define UI_WidthFill         UI_PrefWidth(ui_pct(1.f, 0.f))
#define UI_HeightFill        UI_PrefHeight(ui_pct(1.f, 0.f))
#define UI_Rect(r)           DeferLoop(ui_push_rect(r), ui_pop_rect())
#define UI_PrefSize(axis, v) DeferLoop(ui_push_pref_size((axis), (v)), ui_pop_pref_size(axis))
#define UI_CornerRadius(v)   DeferLoop(ui_push_corner_radius(v), ui_pop_corner_radius())
#define UI_Focus(kind)       DeferLoop((ui_push_focus_hot(kind), ui_push_focus_active(kind)), (ui_pop_focus_hot(), ui_pop_focus_active()))
#define UI_FlagsAdd(v)       DeferLoop(ui_push_flags(ui_top_flags()|(v)), ui_pop_flags())
#define UI_TagF(...)         DeferLoop(ui_push_tagf(__VA_ARGS__), ui_pop_tag())

//- rjf: tooltip
#define UI_TooltipBase DeferLoop(ui_tooltip_begin_base(), ui_tooltip_end_base())
#define UI_Tooltip DeferLoop(ui_tooltip_begin(), ui_tooltip_end())

//- rjf: context menu
#define UI_CtxMenu(key) DeferLoopChecked(ui_begin_ctx_menu(key), ui_end_ctx_menu())

//- rjf: debug
#define UI_Debug UI_FlagsAdd(UI_BoxFlag_Debug)

#endif // UI_H
