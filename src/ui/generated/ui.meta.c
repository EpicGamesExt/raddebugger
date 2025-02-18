// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#if 0
#define UI_Parent(v) DeferLoop(ui_push_parent(v), ui_pop_parent())
#define UI_ChildLayoutAxis(v) DeferLoop(ui_push_child_layout_axis(v), ui_pop_child_layout_axis())
#define UI_FixedX(v) DeferLoop(ui_push_fixed_x(v), ui_pop_fixed_x())
#define UI_FixedY(v) DeferLoop(ui_push_fixed_y(v), ui_pop_fixed_y())
#define UI_FixedWidth(v) DeferLoop(ui_push_fixed_width(v), ui_pop_fixed_width())
#define UI_FixedHeight(v) DeferLoop(ui_push_fixed_height(v), ui_pop_fixed_height())
#define UI_PrefWidth(v) DeferLoop(ui_push_pref_width(v), ui_pop_pref_width())
#define UI_PrefHeight(v) DeferLoop(ui_push_pref_height(v), ui_pop_pref_height())
#define UI_PermissionFlags(v) DeferLoop(ui_push_permission_flags(v), ui_pop_permission_flags())
#define UI_Flags(v) DeferLoop(ui_push_flags(v), ui_pop_flags())
#define UI_FocusHot(v) DeferLoop(ui_push_focus_hot(v), ui_pop_focus_hot())
#define UI_FocusActive(v) DeferLoop(ui_push_focus_active(v), ui_pop_focus_active())
#define UI_FastpathCodepoint(v) DeferLoop(ui_push_fastpath_codepoint(v), ui_pop_fastpath_codepoint())
#define UI_GroupKey(v) DeferLoop(ui_push_group_key(v), ui_pop_group_key())
#define UI_Transparency(v) DeferLoop(ui_push_transparency(v), ui_pop_transparency())
#define UI_Palette(v) DeferLoop(ui_push_palette(v), ui_pop_palette())
#define UI_Tag(v) DeferLoop(ui_push_tag(v), ui_pop_tag())
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
#endif
internal UI_Box * ui_top_parent(void) { UI_StackTopImpl(ui_state, Parent, parent) }
internal Axis2 ui_top_child_layout_axis(void) { UI_StackTopImpl(ui_state, ChildLayoutAxis, child_layout_axis) }
internal F32 ui_top_fixed_x(void) { UI_StackTopImpl(ui_state, FixedX, fixed_x) }
internal F32 ui_top_fixed_y(void) { UI_StackTopImpl(ui_state, FixedY, fixed_y) }
internal F32 ui_top_fixed_width(void) { UI_StackTopImpl(ui_state, FixedWidth, fixed_width) }
internal F32 ui_top_fixed_height(void) { UI_StackTopImpl(ui_state, FixedHeight, fixed_height) }
internal UI_Size ui_top_pref_width(void) { UI_StackTopImpl(ui_state, PrefWidth, pref_width) }
internal UI_Size ui_top_pref_height(void) { UI_StackTopImpl(ui_state, PrefHeight, pref_height) }
internal UI_PermissionFlags ui_top_permission_flags(void) { UI_StackTopImpl(ui_state, PermissionFlags, permission_flags) }
internal UI_BoxFlags ui_top_flags(void) { UI_StackTopImpl(ui_state, Flags, flags) }
internal UI_FocusKind ui_top_focus_hot(void) { UI_StackTopImpl(ui_state, FocusHot, focus_hot) }
internal UI_FocusKind ui_top_focus_active(void) { UI_StackTopImpl(ui_state, FocusActive, focus_active) }
internal U32 ui_top_fastpath_codepoint(void) { UI_StackTopImpl(ui_state, FastpathCodepoint, fastpath_codepoint) }
internal UI_Key ui_top_group_key(void) { UI_StackTopImpl(ui_state, GroupKey, group_key) }
internal F32 ui_top_transparency(void) { UI_StackTopImpl(ui_state, Transparency, transparency) }
internal UI_Palette*     ui_top_palette(void) { UI_StackTopImpl(ui_state, Palette, palette) }
internal String8 ui_top_tag(void) { UI_StackTopImpl(ui_state, Tag, tag) }
internal F32 ui_top_squish(void) { UI_StackTopImpl(ui_state, Squish, squish) }
internal OS_Cursor ui_top_hover_cursor(void) { UI_StackTopImpl(ui_state, HoverCursor, hover_cursor) }
internal FNT_Tag ui_top_font(void) { UI_StackTopImpl(ui_state, Font, font) }
internal F32 ui_top_font_size(void) { UI_StackTopImpl(ui_state, FontSize, font_size) }
internal FNT_RasterFlags ui_top_text_raster_flags(void) { UI_StackTopImpl(ui_state, TextRasterFlags, text_raster_flags) }
internal F32 ui_top_tab_size(void) { UI_StackTopImpl(ui_state, TabSize, tab_size) }
internal F32 ui_top_corner_radius_00(void) { UI_StackTopImpl(ui_state, CornerRadius00, corner_radius_00) }
internal F32 ui_top_corner_radius_01(void) { UI_StackTopImpl(ui_state, CornerRadius01, corner_radius_01) }
internal F32 ui_top_corner_radius_10(void) { UI_StackTopImpl(ui_state, CornerRadius10, corner_radius_10) }
internal F32 ui_top_corner_radius_11(void) { UI_StackTopImpl(ui_state, CornerRadius11, corner_radius_11) }
internal F32 ui_top_blur_size(void) { UI_StackTopImpl(ui_state, BlurSize, blur_size) }
internal F32 ui_top_text_padding(void) { UI_StackTopImpl(ui_state, TextPadding, text_padding) }
internal UI_TextAlign ui_top_text_alignment(void) { UI_StackTopImpl(ui_state, TextAlignment, text_alignment) }
internal UI_Box * ui_bottom_parent(void) { UI_StackBottomImpl(ui_state, Parent, parent) }
internal Axis2 ui_bottom_child_layout_axis(void) { UI_StackBottomImpl(ui_state, ChildLayoutAxis, child_layout_axis) }
internal F32 ui_bottom_fixed_x(void) { UI_StackBottomImpl(ui_state, FixedX, fixed_x) }
internal F32 ui_bottom_fixed_y(void) { UI_StackBottomImpl(ui_state, FixedY, fixed_y) }
internal F32 ui_bottom_fixed_width(void) { UI_StackBottomImpl(ui_state, FixedWidth, fixed_width) }
internal F32 ui_bottom_fixed_height(void) { UI_StackBottomImpl(ui_state, FixedHeight, fixed_height) }
internal UI_Size ui_bottom_pref_width(void) { UI_StackBottomImpl(ui_state, PrefWidth, pref_width) }
internal UI_Size ui_bottom_pref_height(void) { UI_StackBottomImpl(ui_state, PrefHeight, pref_height) }
internal UI_PermissionFlags ui_bottom_permission_flags(void) { UI_StackBottomImpl(ui_state, PermissionFlags, permission_flags) }
internal UI_BoxFlags ui_bottom_flags(void) { UI_StackBottomImpl(ui_state, Flags, flags) }
internal UI_FocusKind ui_bottom_focus_hot(void) { UI_StackBottomImpl(ui_state, FocusHot, focus_hot) }
internal UI_FocusKind ui_bottom_focus_active(void) { UI_StackBottomImpl(ui_state, FocusActive, focus_active) }
internal U32 ui_bottom_fastpath_codepoint(void) { UI_StackBottomImpl(ui_state, FastpathCodepoint, fastpath_codepoint) }
internal UI_Key ui_bottom_group_key(void) { UI_StackBottomImpl(ui_state, GroupKey, group_key) }
internal F32 ui_bottom_transparency(void) { UI_StackBottomImpl(ui_state, Transparency, transparency) }
internal UI_Palette*     ui_bottom_palette(void) { UI_StackBottomImpl(ui_state, Palette, palette) }
internal String8 ui_bottom_tag(void) { UI_StackBottomImpl(ui_state, Tag, tag) }
internal F32 ui_bottom_squish(void) { UI_StackBottomImpl(ui_state, Squish, squish) }
internal OS_Cursor ui_bottom_hover_cursor(void) { UI_StackBottomImpl(ui_state, HoverCursor, hover_cursor) }
internal FNT_Tag ui_bottom_font(void) { UI_StackBottomImpl(ui_state, Font, font) }
internal F32 ui_bottom_font_size(void) { UI_StackBottomImpl(ui_state, FontSize, font_size) }
internal FNT_RasterFlags ui_bottom_text_raster_flags(void) { UI_StackBottomImpl(ui_state, TextRasterFlags, text_raster_flags) }
internal F32 ui_bottom_tab_size(void) { UI_StackBottomImpl(ui_state, TabSize, tab_size) }
internal F32 ui_bottom_corner_radius_00(void) { UI_StackBottomImpl(ui_state, CornerRadius00, corner_radius_00) }
internal F32 ui_bottom_corner_radius_01(void) { UI_StackBottomImpl(ui_state, CornerRadius01, corner_radius_01) }
internal F32 ui_bottom_corner_radius_10(void) { UI_StackBottomImpl(ui_state, CornerRadius10, corner_radius_10) }
internal F32 ui_bottom_corner_radius_11(void) { UI_StackBottomImpl(ui_state, CornerRadius11, corner_radius_11) }
internal F32 ui_bottom_blur_size(void) { UI_StackBottomImpl(ui_state, BlurSize, blur_size) }
internal F32 ui_bottom_text_padding(void) { UI_StackBottomImpl(ui_state, TextPadding, text_padding) }
internal UI_TextAlign ui_bottom_text_alignment(void) { UI_StackBottomImpl(ui_state, TextAlignment, text_alignment) }
internal UI_Box * ui_push_parent(UI_Box * v) { UI_StackPushImpl(ui_state, Parent, parent, UI_Box *, v) }
internal Axis2 ui_push_child_layout_axis(Axis2 v) { UI_StackPushImpl(ui_state, ChildLayoutAxis, child_layout_axis, Axis2, v) }
internal F32 ui_push_fixed_x(F32 v) { UI_StackPushImpl(ui_state, FixedX, fixed_x, F32, v) }
internal F32 ui_push_fixed_y(F32 v) { UI_StackPushImpl(ui_state, FixedY, fixed_y, F32, v) }
internal F32 ui_push_fixed_width(F32 v) { UI_StackPushImpl(ui_state, FixedWidth, fixed_width, F32, v) }
internal F32 ui_push_fixed_height(F32 v) { UI_StackPushImpl(ui_state, FixedHeight, fixed_height, F32, v) }
internal UI_Size ui_push_pref_width(UI_Size v) { UI_StackPushImpl(ui_state, PrefWidth, pref_width, UI_Size, v) }
internal UI_Size ui_push_pref_height(UI_Size v) { UI_StackPushImpl(ui_state, PrefHeight, pref_height, UI_Size, v) }
internal UI_PermissionFlags ui_push_permission_flags(UI_PermissionFlags v) { UI_StackPushImpl(ui_state, PermissionFlags, permission_flags, UI_PermissionFlags, v) }
internal UI_BoxFlags ui_push_flags(UI_BoxFlags v) { UI_StackPushImpl(ui_state, Flags, flags, UI_BoxFlags, v) }
internal UI_FocusKind ui_push_focus_hot(UI_FocusKind v) { UI_StackPushImpl(ui_state, FocusHot, focus_hot, UI_FocusKind, v) }
internal UI_FocusKind ui_push_focus_active(UI_FocusKind v) { UI_StackPushImpl(ui_state, FocusActive, focus_active, UI_FocusKind, v) }
internal U32 ui_push_fastpath_codepoint(U32 v) { UI_StackPushImpl(ui_state, FastpathCodepoint, fastpath_codepoint, U32, v) }
internal UI_Key ui_push_group_key(UI_Key v) { UI_StackPushImpl(ui_state, GroupKey, group_key, UI_Key, v) }
internal F32 ui_push_transparency(F32 v) { UI_StackPushImpl(ui_state, Transparency, transparency, F32, v) }
internal UI_Palette*     ui_push_palette(UI_Palette*     v) { UI_StackPushImpl(ui_state, Palette, palette, UI_Palette*    , v) }
internal String8 ui_push_tag(String8 v) { UI_StackPushImpl(ui_state, Tag, tag, String8, v) }
internal F32 ui_push_squish(F32 v) { UI_StackPushImpl(ui_state, Squish, squish, F32, v) }
internal OS_Cursor ui_push_hover_cursor(OS_Cursor v) { UI_StackPushImpl(ui_state, HoverCursor, hover_cursor, OS_Cursor, v) }
internal FNT_Tag ui_push_font(FNT_Tag v) { UI_StackPushImpl(ui_state, Font, font, FNT_Tag, v) }
internal F32 ui_push_font_size(F32 v) { UI_StackPushImpl(ui_state, FontSize, font_size, F32, v) }
internal FNT_RasterFlags ui_push_text_raster_flags(FNT_RasterFlags v) { UI_StackPushImpl(ui_state, TextRasterFlags, text_raster_flags, FNT_RasterFlags, v) }
internal F32 ui_push_tab_size(F32 v) { UI_StackPushImpl(ui_state, TabSize, tab_size, F32, v) }
internal F32 ui_push_corner_radius_00(F32 v) { UI_StackPushImpl(ui_state, CornerRadius00, corner_radius_00, F32, v) }
internal F32 ui_push_corner_radius_01(F32 v) { UI_StackPushImpl(ui_state, CornerRadius01, corner_radius_01, F32, v) }
internal F32 ui_push_corner_radius_10(F32 v) { UI_StackPushImpl(ui_state, CornerRadius10, corner_radius_10, F32, v) }
internal F32 ui_push_corner_radius_11(F32 v) { UI_StackPushImpl(ui_state, CornerRadius11, corner_radius_11, F32, v) }
internal F32 ui_push_blur_size(F32 v) { UI_StackPushImpl(ui_state, BlurSize, blur_size, F32, v) }
internal F32 ui_push_text_padding(F32 v) { UI_StackPushImpl(ui_state, TextPadding, text_padding, F32, v) }
internal UI_TextAlign ui_push_text_alignment(UI_TextAlign v) { UI_StackPushImpl(ui_state, TextAlignment, text_alignment, UI_TextAlign, v) }
internal UI_Box * ui_pop_parent(void) { UI_StackPopImpl(ui_state, Parent, parent) }
internal Axis2 ui_pop_child_layout_axis(void) { UI_StackPopImpl(ui_state, ChildLayoutAxis, child_layout_axis) }
internal F32 ui_pop_fixed_x(void) { UI_StackPopImpl(ui_state, FixedX, fixed_x) }
internal F32 ui_pop_fixed_y(void) { UI_StackPopImpl(ui_state, FixedY, fixed_y) }
internal F32 ui_pop_fixed_width(void) { UI_StackPopImpl(ui_state, FixedWidth, fixed_width) }
internal F32 ui_pop_fixed_height(void) { UI_StackPopImpl(ui_state, FixedHeight, fixed_height) }
internal UI_Size ui_pop_pref_width(void) { UI_StackPopImpl(ui_state, PrefWidth, pref_width) }
internal UI_Size ui_pop_pref_height(void) { UI_StackPopImpl(ui_state, PrefHeight, pref_height) }
internal UI_PermissionFlags ui_pop_permission_flags(void) { UI_StackPopImpl(ui_state, PermissionFlags, permission_flags) }
internal UI_BoxFlags ui_pop_flags(void) { UI_StackPopImpl(ui_state, Flags, flags) }
internal UI_FocusKind ui_pop_focus_hot(void) { UI_StackPopImpl(ui_state, FocusHot, focus_hot) }
internal UI_FocusKind ui_pop_focus_active(void) { UI_StackPopImpl(ui_state, FocusActive, focus_active) }
internal U32 ui_pop_fastpath_codepoint(void) { UI_StackPopImpl(ui_state, FastpathCodepoint, fastpath_codepoint) }
internal UI_Key ui_pop_group_key(void) { UI_StackPopImpl(ui_state, GroupKey, group_key) }
internal F32 ui_pop_transparency(void) { UI_StackPopImpl(ui_state, Transparency, transparency) }
internal UI_Palette*     ui_pop_palette(void) { UI_StackPopImpl(ui_state, Palette, palette) }
internal String8 ui_pop_tag(void) { UI_StackPopImpl(ui_state, Tag, tag) }
internal F32 ui_pop_squish(void) { UI_StackPopImpl(ui_state, Squish, squish) }
internal OS_Cursor ui_pop_hover_cursor(void) { UI_StackPopImpl(ui_state, HoverCursor, hover_cursor) }
internal FNT_Tag ui_pop_font(void) { UI_StackPopImpl(ui_state, Font, font) }
internal F32 ui_pop_font_size(void) { UI_StackPopImpl(ui_state, FontSize, font_size) }
internal FNT_RasterFlags ui_pop_text_raster_flags(void) { UI_StackPopImpl(ui_state, TextRasterFlags, text_raster_flags) }
internal F32 ui_pop_tab_size(void) { UI_StackPopImpl(ui_state, TabSize, tab_size) }
internal F32 ui_pop_corner_radius_00(void) { UI_StackPopImpl(ui_state, CornerRadius00, corner_radius_00) }
internal F32 ui_pop_corner_radius_01(void) { UI_StackPopImpl(ui_state, CornerRadius01, corner_radius_01) }
internal F32 ui_pop_corner_radius_10(void) { UI_StackPopImpl(ui_state, CornerRadius10, corner_radius_10) }
internal F32 ui_pop_corner_radius_11(void) { UI_StackPopImpl(ui_state, CornerRadius11, corner_radius_11) }
internal F32 ui_pop_blur_size(void) { UI_StackPopImpl(ui_state, BlurSize, blur_size) }
internal F32 ui_pop_text_padding(void) { UI_StackPopImpl(ui_state, TextPadding, text_padding) }
internal UI_TextAlign ui_pop_text_alignment(void) { UI_StackPopImpl(ui_state, TextAlignment, text_alignment) }
internal UI_Box * ui_set_next_parent(UI_Box * v) { UI_StackSetNextImpl(ui_state, Parent, parent, UI_Box *, v) }
internal Axis2 ui_set_next_child_layout_axis(Axis2 v) { UI_StackSetNextImpl(ui_state, ChildLayoutAxis, child_layout_axis, Axis2, v) }
internal F32 ui_set_next_fixed_x(F32 v) { UI_StackSetNextImpl(ui_state, FixedX, fixed_x, F32, v) }
internal F32 ui_set_next_fixed_y(F32 v) { UI_StackSetNextImpl(ui_state, FixedY, fixed_y, F32, v) }
internal F32 ui_set_next_fixed_width(F32 v) { UI_StackSetNextImpl(ui_state, FixedWidth, fixed_width, F32, v) }
internal F32 ui_set_next_fixed_height(F32 v) { UI_StackSetNextImpl(ui_state, FixedHeight, fixed_height, F32, v) }
internal UI_Size ui_set_next_pref_width(UI_Size v) { UI_StackSetNextImpl(ui_state, PrefWidth, pref_width, UI_Size, v) }
internal UI_Size ui_set_next_pref_height(UI_Size v) { UI_StackSetNextImpl(ui_state, PrefHeight, pref_height, UI_Size, v) }
internal UI_PermissionFlags ui_set_next_permission_flags(UI_PermissionFlags v) { UI_StackSetNextImpl(ui_state, PermissionFlags, permission_flags, UI_PermissionFlags, v) }
internal UI_BoxFlags ui_set_next_flags(UI_BoxFlags v) { UI_StackSetNextImpl(ui_state, Flags, flags, UI_BoxFlags, v) }
internal UI_FocusKind ui_set_next_focus_hot(UI_FocusKind v) { UI_StackSetNextImpl(ui_state, FocusHot, focus_hot, UI_FocusKind, v) }
internal UI_FocusKind ui_set_next_focus_active(UI_FocusKind v) { UI_StackSetNextImpl(ui_state, FocusActive, focus_active, UI_FocusKind, v) }
internal U32 ui_set_next_fastpath_codepoint(U32 v) { UI_StackSetNextImpl(ui_state, FastpathCodepoint, fastpath_codepoint, U32, v) }
internal UI_Key ui_set_next_group_key(UI_Key v) { UI_StackSetNextImpl(ui_state, GroupKey, group_key, UI_Key, v) }
internal F32 ui_set_next_transparency(F32 v) { UI_StackSetNextImpl(ui_state, Transparency, transparency, F32, v) }
internal UI_Palette*     ui_set_next_palette(UI_Palette*     v) { UI_StackSetNextImpl(ui_state, Palette, palette, UI_Palette*    , v) }
internal String8 ui_set_next_tag(String8 v) { UI_StackSetNextImpl(ui_state, Tag, tag, String8, v) }
internal F32 ui_set_next_squish(F32 v) { UI_StackSetNextImpl(ui_state, Squish, squish, F32, v) }
internal OS_Cursor ui_set_next_hover_cursor(OS_Cursor v) { UI_StackSetNextImpl(ui_state, HoverCursor, hover_cursor, OS_Cursor, v) }
internal FNT_Tag ui_set_next_font(FNT_Tag v) { UI_StackSetNextImpl(ui_state, Font, font, FNT_Tag, v) }
internal F32 ui_set_next_font_size(F32 v) { UI_StackSetNextImpl(ui_state, FontSize, font_size, F32, v) }
internal FNT_RasterFlags ui_set_next_text_raster_flags(FNT_RasterFlags v) { UI_StackSetNextImpl(ui_state, TextRasterFlags, text_raster_flags, FNT_RasterFlags, v) }
internal F32 ui_set_next_tab_size(F32 v) { UI_StackSetNextImpl(ui_state, TabSize, tab_size, F32, v) }
internal F32 ui_set_next_corner_radius_00(F32 v) { UI_StackSetNextImpl(ui_state, CornerRadius00, corner_radius_00, F32, v) }
internal F32 ui_set_next_corner_radius_01(F32 v) { UI_StackSetNextImpl(ui_state, CornerRadius01, corner_radius_01, F32, v) }
internal F32 ui_set_next_corner_radius_10(F32 v) { UI_StackSetNextImpl(ui_state, CornerRadius10, corner_radius_10, F32, v) }
internal F32 ui_set_next_corner_radius_11(F32 v) { UI_StackSetNextImpl(ui_state, CornerRadius11, corner_radius_11, F32, v) }
internal F32 ui_set_next_blur_size(F32 v) { UI_StackSetNextImpl(ui_state, BlurSize, blur_size, F32, v) }
internal F32 ui_set_next_text_padding(F32 v) { UI_StackSetNextImpl(ui_state, TextPadding, text_padding, F32, v) }
internal UI_TextAlign ui_set_next_text_alignment(UI_TextAlign v) { UI_StackSetNextImpl(ui_state, TextAlignment, text_alignment, UI_TextAlign, v) }
