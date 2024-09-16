// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef UI_META_H
#define UI_META_H

typedef struct UI_ParentNode UI_ParentNode; struct UI_ParentNode{UI_ParentNode *next; UI_Box * v;};
typedef struct UI_ChildLayoutAxisNode UI_ChildLayoutAxisNode; struct UI_ChildLayoutAxisNode{UI_ChildLayoutAxisNode *next; Axis2 v;};
typedef struct UI_FixedXNode UI_FixedXNode; struct UI_FixedXNode{UI_FixedXNode *next; F32 v;};
typedef struct UI_FixedYNode UI_FixedYNode; struct UI_FixedYNode{UI_FixedYNode *next; F32 v;};
typedef struct UI_FixedWidthNode UI_FixedWidthNode; struct UI_FixedWidthNode{UI_FixedWidthNode *next; F32 v;};
typedef struct UI_FixedHeightNode UI_FixedHeightNode; struct UI_FixedHeightNode{UI_FixedHeightNode *next; F32 v;};
typedef struct UI_PrefWidthNode UI_PrefWidthNode; struct UI_PrefWidthNode{UI_PrefWidthNode *next; UI_Size v;};
typedef struct UI_PrefHeightNode UI_PrefHeightNode; struct UI_PrefHeightNode{UI_PrefHeightNode *next; UI_Size v;};
typedef struct UI_PermissionFlagsNode UI_PermissionFlagsNode; struct UI_PermissionFlagsNode{UI_PermissionFlagsNode *next; UI_PermissionFlags v;};
typedef struct UI_FlagsNode UI_FlagsNode; struct UI_FlagsNode{UI_FlagsNode *next; UI_BoxFlags v;};
typedef struct UI_FocusHotNode UI_FocusHotNode; struct UI_FocusHotNode{UI_FocusHotNode *next; UI_FocusKind v;};
typedef struct UI_FocusActiveNode UI_FocusActiveNode; struct UI_FocusActiveNode{UI_FocusActiveNode *next; UI_FocusKind v;};
typedef struct UI_FastpathCodepointNode UI_FastpathCodepointNode; struct UI_FastpathCodepointNode{UI_FastpathCodepointNode *next; U32 v;};
typedef struct UI_GroupKeyNode UI_GroupKeyNode; struct UI_GroupKeyNode{UI_GroupKeyNode *next; UI_Key v;};
typedef struct UI_TransparencyNode UI_TransparencyNode; struct UI_TransparencyNode{UI_TransparencyNode *next; F32 v;};
typedef struct UI_PaletteNode UI_PaletteNode; struct UI_PaletteNode{UI_PaletteNode *next; UI_Palette*     v;};
typedef struct UI_SquishNode UI_SquishNode; struct UI_SquishNode{UI_SquishNode *next; F32 v;};
typedef struct UI_HoverCursorNode UI_HoverCursorNode; struct UI_HoverCursorNode{UI_HoverCursorNode *next; OS_Cursor v;};
typedef struct UI_FontNode UI_FontNode; struct UI_FontNode{UI_FontNode *next; FNT_Tag v;};
typedef struct UI_FontSizeNode UI_FontSizeNode; struct UI_FontSizeNode{UI_FontSizeNode *next; F32 v;};
typedef struct UI_TextRasterFlagsNode UI_TextRasterFlagsNode; struct UI_TextRasterFlagsNode{UI_TextRasterFlagsNode *next; FNT_RasterFlags v;};
typedef struct UI_TabSizeNode UI_TabSizeNode; struct UI_TabSizeNode{UI_TabSizeNode *next; F32 v;};
typedef struct UI_CornerRadius00Node UI_CornerRadius00Node; struct UI_CornerRadius00Node{UI_CornerRadius00Node *next; F32 v;};
typedef struct UI_CornerRadius01Node UI_CornerRadius01Node; struct UI_CornerRadius01Node{UI_CornerRadius01Node *next; F32 v;};
typedef struct UI_CornerRadius10Node UI_CornerRadius10Node; struct UI_CornerRadius10Node{UI_CornerRadius10Node *next; F32 v;};
typedef struct UI_CornerRadius11Node UI_CornerRadius11Node; struct UI_CornerRadius11Node{UI_CornerRadius11Node *next; F32 v;};
typedef struct UI_BlurSizeNode UI_BlurSizeNode; struct UI_BlurSizeNode{UI_BlurSizeNode *next; F32 v;};
typedef struct UI_TextPaddingNode UI_TextPaddingNode; struct UI_TextPaddingNode{UI_TextPaddingNode *next; F32 v;};
typedef struct UI_TextAlignmentNode UI_TextAlignmentNode; struct UI_TextAlignmentNode{UI_TextAlignmentNode *next; UI_TextAlign v;};
#define UI_DeclStackNils \
struct\
{\
UI_ParentNode parent_nil_stack_top;\
UI_ChildLayoutAxisNode child_layout_axis_nil_stack_top;\
UI_FixedXNode fixed_x_nil_stack_top;\
UI_FixedYNode fixed_y_nil_stack_top;\
UI_FixedWidthNode fixed_width_nil_stack_top;\
UI_FixedHeightNode fixed_height_nil_stack_top;\
UI_PrefWidthNode pref_width_nil_stack_top;\
UI_PrefHeightNode pref_height_nil_stack_top;\
UI_PermissionFlagsNode permission_flags_nil_stack_top;\
UI_FlagsNode flags_nil_stack_top;\
UI_FocusHotNode focus_hot_nil_stack_top;\
UI_FocusActiveNode focus_active_nil_stack_top;\
UI_FastpathCodepointNode fastpath_codepoint_nil_stack_top;\
UI_GroupKeyNode group_key_nil_stack_top;\
UI_TransparencyNode transparency_nil_stack_top;\
UI_PaletteNode palette_nil_stack_top;\
UI_SquishNode squish_nil_stack_top;\
UI_HoverCursorNode hover_cursor_nil_stack_top;\
UI_FontNode font_nil_stack_top;\
UI_FontSizeNode font_size_nil_stack_top;\
UI_TextRasterFlagsNode text_raster_flags_nil_stack_top;\
UI_TabSizeNode tab_size_nil_stack_top;\
UI_CornerRadius00Node corner_radius_00_nil_stack_top;\
UI_CornerRadius01Node corner_radius_01_nil_stack_top;\
UI_CornerRadius10Node corner_radius_10_nil_stack_top;\
UI_CornerRadius11Node corner_radius_11_nil_stack_top;\
UI_BlurSizeNode blur_size_nil_stack_top;\
UI_TextPaddingNode text_padding_nil_stack_top;\
UI_TextAlignmentNode text_alignment_nil_stack_top;\
}
#define UI_InitStackNils(state) \
state->parent_nil_stack_top.v = &ui_nil_box;\
state->child_layout_axis_nil_stack_top.v = Axis2_X;\
state->fixed_x_nil_stack_top.v = 0;\
state->fixed_y_nil_stack_top.v = 0;\
state->fixed_width_nil_stack_top.v = 0;\
state->fixed_height_nil_stack_top.v = 0;\
state->pref_width_nil_stack_top.v = ui_px(250.f, 1.f);\
state->pref_height_nil_stack_top.v = ui_px(30.f, 1.f);\
state->permission_flags_nil_stack_top.v = UI_PermissionFlag_All;\
state->flags_nil_stack_top.v = 0;\
state->focus_hot_nil_stack_top.v = UI_FocusKind_Null;\
state->focus_active_nil_stack_top.v = UI_FocusKind_Null;\
state->fastpath_codepoint_nil_stack_top.v = 0;\
state->group_key_nil_stack_top.v = ui_key_zero();\
state->transparency_nil_stack_top.v = 0;\
state->palette_nil_stack_top.v = &ui_g_nil_palette;\
state->squish_nil_stack_top.v = 0;\
state->hover_cursor_nil_stack_top.v = OS_Cursor_Pointer;\
state->font_nil_stack_top.v = fnt_tag_zero();\
state->font_size_nil_stack_top.v = 24.f;\
state->text_raster_flags_nil_stack_top.v = FNT_RasterFlag_Hinted;\
state->tab_size_nil_stack_top.v = 24.f*4.f;\
state->corner_radius_00_nil_stack_top.v = 0;\
state->corner_radius_01_nil_stack_top.v = 0;\
state->corner_radius_10_nil_stack_top.v = 0;\
state->corner_radius_11_nil_stack_top.v = 0;\
state->blur_size_nil_stack_top.v = 0;\
state->text_padding_nil_stack_top.v = 0;\
state->text_alignment_nil_stack_top.v = UI_TextAlign_Left;\

#define UI_DeclStacks \
struct\
{\
struct { UI_ParentNode *top; UI_Box * bottom_val; UI_ParentNode *free; B32 auto_pop; } parent_stack;\
struct { UI_ChildLayoutAxisNode *top; Axis2 bottom_val; UI_ChildLayoutAxisNode *free; B32 auto_pop; } child_layout_axis_stack;\
struct { UI_FixedXNode *top; F32 bottom_val; UI_FixedXNode *free; B32 auto_pop; } fixed_x_stack;\
struct { UI_FixedYNode *top; F32 bottom_val; UI_FixedYNode *free; B32 auto_pop; } fixed_y_stack;\
struct { UI_FixedWidthNode *top; F32 bottom_val; UI_FixedWidthNode *free; B32 auto_pop; } fixed_width_stack;\
struct { UI_FixedHeightNode *top; F32 bottom_val; UI_FixedHeightNode *free; B32 auto_pop; } fixed_height_stack;\
struct { UI_PrefWidthNode *top; UI_Size bottom_val; UI_PrefWidthNode *free; B32 auto_pop; } pref_width_stack;\
struct { UI_PrefHeightNode *top; UI_Size bottom_val; UI_PrefHeightNode *free; B32 auto_pop; } pref_height_stack;\
struct { UI_PermissionFlagsNode *top; UI_PermissionFlags bottom_val; UI_PermissionFlagsNode *free; B32 auto_pop; } permission_flags_stack;\
struct { UI_FlagsNode *top; UI_BoxFlags bottom_val; UI_FlagsNode *free; B32 auto_pop; } flags_stack;\
struct { UI_FocusHotNode *top; UI_FocusKind bottom_val; UI_FocusHotNode *free; B32 auto_pop; } focus_hot_stack;\
struct { UI_FocusActiveNode *top; UI_FocusKind bottom_val; UI_FocusActiveNode *free; B32 auto_pop; } focus_active_stack;\
struct { UI_FastpathCodepointNode *top; U32 bottom_val; UI_FastpathCodepointNode *free; B32 auto_pop; } fastpath_codepoint_stack;\
struct { UI_GroupKeyNode *top; UI_Key bottom_val; UI_GroupKeyNode *free; B32 auto_pop; } group_key_stack;\
struct { UI_TransparencyNode *top; F32 bottom_val; UI_TransparencyNode *free; B32 auto_pop; } transparency_stack;\
struct { UI_PaletteNode *top; UI_Palette*     bottom_val; UI_PaletteNode *free; B32 auto_pop; } palette_stack;\
struct { UI_SquishNode *top; F32 bottom_val; UI_SquishNode *free; B32 auto_pop; } squish_stack;\
struct { UI_HoverCursorNode *top; OS_Cursor bottom_val; UI_HoverCursorNode *free; B32 auto_pop; } hover_cursor_stack;\
struct { UI_FontNode *top; FNT_Tag bottom_val; UI_FontNode *free; B32 auto_pop; } font_stack;\
struct { UI_FontSizeNode *top; F32 bottom_val; UI_FontSizeNode *free; B32 auto_pop; } font_size_stack;\
struct { UI_TextRasterFlagsNode *top; FNT_RasterFlags bottom_val; UI_TextRasterFlagsNode *free; B32 auto_pop; } text_raster_flags_stack;\
struct { UI_TabSizeNode *top; F32 bottom_val; UI_TabSizeNode *free; B32 auto_pop; } tab_size_stack;\
struct { UI_CornerRadius00Node *top; F32 bottom_val; UI_CornerRadius00Node *free; B32 auto_pop; } corner_radius_00_stack;\
struct { UI_CornerRadius01Node *top; F32 bottom_val; UI_CornerRadius01Node *free; B32 auto_pop; } corner_radius_01_stack;\
struct { UI_CornerRadius10Node *top; F32 bottom_val; UI_CornerRadius10Node *free; B32 auto_pop; } corner_radius_10_stack;\
struct { UI_CornerRadius11Node *top; F32 bottom_val; UI_CornerRadius11Node *free; B32 auto_pop; } corner_radius_11_stack;\
struct { UI_BlurSizeNode *top; F32 bottom_val; UI_BlurSizeNode *free; B32 auto_pop; } blur_size_stack;\
struct { UI_TextPaddingNode *top; F32 bottom_val; UI_TextPaddingNode *free; B32 auto_pop; } text_padding_stack;\
struct { UI_TextAlignmentNode *top; UI_TextAlign bottom_val; UI_TextAlignmentNode *free; B32 auto_pop; } text_alignment_stack;\
}
#define UI_InitStacks(state) \
state->parent_stack.top = &state->parent_nil_stack_top; state->parent_stack.bottom_val = &ui_nil_box; state->parent_stack.free = 0; state->parent_stack.auto_pop = 0;\
state->child_layout_axis_stack.top = &state->child_layout_axis_nil_stack_top; state->child_layout_axis_stack.bottom_val = Axis2_X; state->child_layout_axis_stack.free = 0; state->child_layout_axis_stack.auto_pop = 0;\
state->fixed_x_stack.top = &state->fixed_x_nil_stack_top; state->fixed_x_stack.bottom_val = 0; state->fixed_x_stack.free = 0; state->fixed_x_stack.auto_pop = 0;\
state->fixed_y_stack.top = &state->fixed_y_nil_stack_top; state->fixed_y_stack.bottom_val = 0; state->fixed_y_stack.free = 0; state->fixed_y_stack.auto_pop = 0;\
state->fixed_width_stack.top = &state->fixed_width_nil_stack_top; state->fixed_width_stack.bottom_val = 0; state->fixed_width_stack.free = 0; state->fixed_width_stack.auto_pop = 0;\
state->fixed_height_stack.top = &state->fixed_height_nil_stack_top; state->fixed_height_stack.bottom_val = 0; state->fixed_height_stack.free = 0; state->fixed_height_stack.auto_pop = 0;\
state->pref_width_stack.top = &state->pref_width_nil_stack_top; state->pref_width_stack.bottom_val = ui_px(250.f, 1.f); state->pref_width_stack.free = 0; state->pref_width_stack.auto_pop = 0;\
state->pref_height_stack.top = &state->pref_height_nil_stack_top; state->pref_height_stack.bottom_val = ui_px(30.f, 1.f); state->pref_height_stack.free = 0; state->pref_height_stack.auto_pop = 0;\
state->permission_flags_stack.top = &state->permission_flags_nil_stack_top; state->permission_flags_stack.bottom_val = UI_PermissionFlag_All; state->permission_flags_stack.free = 0; state->permission_flags_stack.auto_pop = 0;\
state->flags_stack.top = &state->flags_nil_stack_top; state->flags_stack.bottom_val = 0; state->flags_stack.free = 0; state->flags_stack.auto_pop = 0;\
state->focus_hot_stack.top = &state->focus_hot_nil_stack_top; state->focus_hot_stack.bottom_val = UI_FocusKind_Null; state->focus_hot_stack.free = 0; state->focus_hot_stack.auto_pop = 0;\
state->focus_active_stack.top = &state->focus_active_nil_stack_top; state->focus_active_stack.bottom_val = UI_FocusKind_Null; state->focus_active_stack.free = 0; state->focus_active_stack.auto_pop = 0;\
state->fastpath_codepoint_stack.top = &state->fastpath_codepoint_nil_stack_top; state->fastpath_codepoint_stack.bottom_val = 0; state->fastpath_codepoint_stack.free = 0; state->fastpath_codepoint_stack.auto_pop = 0;\
state->group_key_stack.top = &state->group_key_nil_stack_top; state->group_key_stack.bottom_val = ui_key_zero(); state->group_key_stack.free = 0; state->group_key_stack.auto_pop = 0;\
state->transparency_stack.top = &state->transparency_nil_stack_top; state->transparency_stack.bottom_val = 0; state->transparency_stack.free = 0; state->transparency_stack.auto_pop = 0;\
state->palette_stack.top = &state->palette_nil_stack_top; state->palette_stack.bottom_val = &ui_g_nil_palette; state->palette_stack.free = 0; state->palette_stack.auto_pop = 0;\
state->squish_stack.top = &state->squish_nil_stack_top; state->squish_stack.bottom_val = 0; state->squish_stack.free = 0; state->squish_stack.auto_pop = 0;\
state->hover_cursor_stack.top = &state->hover_cursor_nil_stack_top; state->hover_cursor_stack.bottom_val = OS_Cursor_Pointer; state->hover_cursor_stack.free = 0; state->hover_cursor_stack.auto_pop = 0;\
state->font_stack.top = &state->font_nil_stack_top; state->font_stack.bottom_val = fnt_tag_zero(); state->font_stack.free = 0; state->font_stack.auto_pop = 0;\
state->font_size_stack.top = &state->font_size_nil_stack_top; state->font_size_stack.bottom_val = 24.f; state->font_size_stack.free = 0; state->font_size_stack.auto_pop = 0;\
state->text_raster_flags_stack.top = &state->text_raster_flags_nil_stack_top; state->text_raster_flags_stack.bottom_val = FNT_RasterFlag_Hinted; state->text_raster_flags_stack.free = 0; state->text_raster_flags_stack.auto_pop = 0;\
state->tab_size_stack.top = &state->tab_size_nil_stack_top; state->tab_size_stack.bottom_val = 24.f*4.f; state->tab_size_stack.free = 0; state->tab_size_stack.auto_pop = 0;\
state->corner_radius_00_stack.top = &state->corner_radius_00_nil_stack_top; state->corner_radius_00_stack.bottom_val = 0; state->corner_radius_00_stack.free = 0; state->corner_radius_00_stack.auto_pop = 0;\
state->corner_radius_01_stack.top = &state->corner_radius_01_nil_stack_top; state->corner_radius_01_stack.bottom_val = 0; state->corner_radius_01_stack.free = 0; state->corner_radius_01_stack.auto_pop = 0;\
state->corner_radius_10_stack.top = &state->corner_radius_10_nil_stack_top; state->corner_radius_10_stack.bottom_val = 0; state->corner_radius_10_stack.free = 0; state->corner_radius_10_stack.auto_pop = 0;\
state->corner_radius_11_stack.top = &state->corner_radius_11_nil_stack_top; state->corner_radius_11_stack.bottom_val = 0; state->corner_radius_11_stack.free = 0; state->corner_radius_11_stack.auto_pop = 0;\
state->blur_size_stack.top = &state->blur_size_nil_stack_top; state->blur_size_stack.bottom_val = 0; state->blur_size_stack.free = 0; state->blur_size_stack.auto_pop = 0;\
state->text_padding_stack.top = &state->text_padding_nil_stack_top; state->text_padding_stack.bottom_val = 0; state->text_padding_stack.free = 0; state->text_padding_stack.auto_pop = 0;\
state->text_alignment_stack.top = &state->text_alignment_nil_stack_top; state->text_alignment_stack.bottom_val = UI_TextAlign_Left; state->text_alignment_stack.free = 0; state->text_alignment_stack.auto_pop = 0;\

#define UI_AutoPopStacks(state) \
if(state->parent_stack.auto_pop) { ui_pop_parent(); state->parent_stack.auto_pop = 0; }\
if(state->child_layout_axis_stack.auto_pop) { ui_pop_child_layout_axis(); state->child_layout_axis_stack.auto_pop = 0; }\
if(state->fixed_x_stack.auto_pop) { ui_pop_fixed_x(); state->fixed_x_stack.auto_pop = 0; }\
if(state->fixed_y_stack.auto_pop) { ui_pop_fixed_y(); state->fixed_y_stack.auto_pop = 0; }\
if(state->fixed_width_stack.auto_pop) { ui_pop_fixed_width(); state->fixed_width_stack.auto_pop = 0; }\
if(state->fixed_height_stack.auto_pop) { ui_pop_fixed_height(); state->fixed_height_stack.auto_pop = 0; }\
if(state->pref_width_stack.auto_pop) { ui_pop_pref_width(); state->pref_width_stack.auto_pop = 0; }\
if(state->pref_height_stack.auto_pop) { ui_pop_pref_height(); state->pref_height_stack.auto_pop = 0; }\
if(state->permission_flags_stack.auto_pop) { ui_pop_permission_flags(); state->permission_flags_stack.auto_pop = 0; }\
if(state->flags_stack.auto_pop) { ui_pop_flags(); state->flags_stack.auto_pop = 0; }\
if(state->focus_hot_stack.auto_pop) { ui_pop_focus_hot(); state->focus_hot_stack.auto_pop = 0; }\
if(state->focus_active_stack.auto_pop) { ui_pop_focus_active(); state->focus_active_stack.auto_pop = 0; }\
if(state->fastpath_codepoint_stack.auto_pop) { ui_pop_fastpath_codepoint(); state->fastpath_codepoint_stack.auto_pop = 0; }\
if(state->group_key_stack.auto_pop) { ui_pop_group_key(); state->group_key_stack.auto_pop = 0; }\
if(state->transparency_stack.auto_pop) { ui_pop_transparency(); state->transparency_stack.auto_pop = 0; }\
if(state->palette_stack.auto_pop) { ui_pop_palette(); state->palette_stack.auto_pop = 0; }\
if(state->squish_stack.auto_pop) { ui_pop_squish(); state->squish_stack.auto_pop = 0; }\
if(state->hover_cursor_stack.auto_pop) { ui_pop_hover_cursor(); state->hover_cursor_stack.auto_pop = 0; }\
if(state->font_stack.auto_pop) { ui_pop_font(); state->font_stack.auto_pop = 0; }\
if(state->font_size_stack.auto_pop) { ui_pop_font_size(); state->font_size_stack.auto_pop = 0; }\
if(state->text_raster_flags_stack.auto_pop) { ui_pop_text_raster_flags(); state->text_raster_flags_stack.auto_pop = 0; }\
if(state->tab_size_stack.auto_pop) { ui_pop_tab_size(); state->tab_size_stack.auto_pop = 0; }\
if(state->corner_radius_00_stack.auto_pop) { ui_pop_corner_radius_00(); state->corner_radius_00_stack.auto_pop = 0; }\
if(state->corner_radius_01_stack.auto_pop) { ui_pop_corner_radius_01(); state->corner_radius_01_stack.auto_pop = 0; }\
if(state->corner_radius_10_stack.auto_pop) { ui_pop_corner_radius_10(); state->corner_radius_10_stack.auto_pop = 0; }\
if(state->corner_radius_11_stack.auto_pop) { ui_pop_corner_radius_11(); state->corner_radius_11_stack.auto_pop = 0; }\
if(state->blur_size_stack.auto_pop) { ui_pop_blur_size(); state->blur_size_stack.auto_pop = 0; }\
if(state->text_padding_stack.auto_pop) { ui_pop_text_padding(); state->text_padding_stack.auto_pop = 0; }\
if(state->text_alignment_stack.auto_pop) { ui_pop_text_alignment(); state->text_alignment_stack.auto_pop = 0; }\

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
internal UI_FocusKind               ui_top_focus_hot(void);
internal UI_FocusKind               ui_top_focus_active(void);
internal U32                        ui_top_fastpath_codepoint(void);
internal UI_Key                     ui_top_group_key(void);
internal F32                        ui_top_transparency(void);
internal UI_Palette*                ui_top_palette(void);
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
internal UI_FocusKind               ui_bottom_focus_hot(void);
internal UI_FocusKind               ui_bottom_focus_active(void);
internal U32                        ui_bottom_fastpath_codepoint(void);
internal UI_Key                     ui_bottom_group_key(void);
internal F32                        ui_bottom_transparency(void);
internal UI_Palette*                ui_bottom_palette(void);
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
internal UI_FocusKind               ui_push_focus_hot(UI_FocusKind v);
internal UI_FocusKind               ui_push_focus_active(UI_FocusKind v);
internal U32                        ui_push_fastpath_codepoint(U32 v);
internal UI_Key                     ui_push_group_key(UI_Key v);
internal F32                        ui_push_transparency(F32 v);
internal UI_Palette*                ui_push_palette(UI_Palette*     v);
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
internal UI_FocusKind               ui_pop_focus_hot(void);
internal UI_FocusKind               ui_pop_focus_active(void);
internal U32                        ui_pop_fastpath_codepoint(void);
internal UI_Key                     ui_pop_group_key(void);
internal F32                        ui_pop_transparency(void);
internal UI_Palette*                ui_pop_palette(void);
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
internal UI_FocusKind               ui_set_next_focus_hot(UI_FocusKind v);
internal UI_FocusKind               ui_set_next_focus_active(UI_FocusKind v);
internal U32                        ui_set_next_fastpath_codepoint(U32 v);
internal UI_Key                     ui_set_next_group_key(UI_Key v);
internal F32                        ui_set_next_transparency(F32 v);
internal UI_Palette*                ui_set_next_palette(UI_Palette*     v);
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
#endif // UI_META_H
