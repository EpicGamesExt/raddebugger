// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

internal UI_Box * ui_push_parent(UI_Box * v) {return UI_StackPush(parent, v);}
internal Axis2 ui_push_child_layout_axis(Axis2 v) {return UI_StackPush(child_layout_axis, v);}
internal F32 ui_push_fixed_x(F32 v) {return UI_StackPush(fixed_x, v);}
internal F32 ui_push_fixed_y(F32 v) {return UI_StackPush(fixed_y, v);}
internal F32 ui_push_fixed_width(F32 v) {return UI_StackPush(fixed_width, v);}
internal F32 ui_push_fixed_height(F32 v) {return UI_StackPush(fixed_height, v);}
internal UI_Size ui_push_pref_width(UI_Size v) {return UI_StackPush(pref_width, v);}
internal UI_Size ui_push_pref_height(UI_Size v) {return UI_StackPush(pref_height, v);}
internal UI_BoxFlags ui_push_flags(UI_BoxFlags v) {return UI_StackPush(flags, v);}
internal U32 ui_push_fastpath_codepoint(U32 v) {return UI_StackPush(fastpath_codepoint, v);}
internal Vec4F32 ui_push_background_color(Vec4F32 v) {return UI_StackPush(background_color, v);}
internal Vec4F32 ui_push_text_color(Vec4F32 v) {return UI_StackPush(text_color, v);}
internal Vec4F32 ui_push_border_color(Vec4F32 v) {return UI_StackPush(border_color, v);}
internal Vec4F32 ui_push_overlay_color(Vec4F32 v) {return UI_StackPush(overlay_color, v);}
internal Vec4F32 ui_push_text_select_color(Vec4F32 v) {return UI_StackPush(text_select_color, v);}
internal Vec4F32 ui_push_text_cursor_color(Vec4F32 v) {return UI_StackPush(text_cursor_color, v);}
internal OS_Cursor ui_push_hover_cursor(OS_Cursor v) {return UI_StackPush(hover_cursor, v);}
internal F_Tag ui_push_font(F_Tag v) {return UI_StackPush(font, v);}
internal F32 ui_push_font_size(F32 v) {return UI_StackPush(font_size, v);}
internal F32 ui_push_corner_radius_00(F32 v) {return UI_StackPush(corner_radius_00, v);}
internal F32 ui_push_corner_radius_01(F32 v) {return UI_StackPush(corner_radius_01, v);}
internal F32 ui_push_corner_radius_10(F32 v) {return UI_StackPush(corner_radius_10, v);}
internal F32 ui_push_corner_radius_11(F32 v) {return UI_StackPush(corner_radius_11, v);}
internal F32 ui_push_blur_size(F32 v) {return UI_StackPush(blur_size, v);}
internal F32 ui_push_text_padding(F32 v) {return UI_StackPush(text_padding, v);}
internal UI_TextAlign ui_push_text_alignment(UI_TextAlign v) {return UI_StackPush(text_alignment, v);}
internal UI_Box * ui_pop_parent(void) {UI_Box * popped; return UI_StackPop(parent, popped);}
internal Axis2 ui_pop_child_layout_axis(void) {Axis2 popped; return UI_StackPop(child_layout_axis, popped);}
internal F32 ui_pop_fixed_x(void) {F32 popped; return UI_StackPop(fixed_x, popped);}
internal F32 ui_pop_fixed_y(void) {F32 popped; return UI_StackPop(fixed_y, popped);}
internal F32 ui_pop_fixed_width(void) {F32 popped; return UI_StackPop(fixed_width, popped);}
internal F32 ui_pop_fixed_height(void) {F32 popped; return UI_StackPop(fixed_height, popped);}
internal UI_Size ui_pop_pref_width(void) {UI_Size popped; return UI_StackPop(pref_width, popped);}
internal UI_Size ui_pop_pref_height(void) {UI_Size popped; return UI_StackPop(pref_height, popped);}
internal UI_BoxFlags ui_pop_flags(void) {UI_BoxFlags popped; return UI_StackPop(flags, popped);}
internal U32 ui_pop_fastpath_codepoint(void) {U32 popped; return UI_StackPop(fastpath_codepoint, popped);}
internal Vec4F32 ui_pop_background_color(void) {Vec4F32 popped; return UI_StackPop(background_color, popped);}
internal Vec4F32 ui_pop_text_color(void) {Vec4F32 popped; return UI_StackPop(text_color, popped);}
internal Vec4F32 ui_pop_border_color(void) {Vec4F32 popped; return UI_StackPop(border_color, popped);}
internal Vec4F32 ui_pop_overlay_color(void) {Vec4F32 popped; return UI_StackPop(overlay_color, popped);}
internal Vec4F32 ui_pop_text_select_color(void) {Vec4F32 popped; return UI_StackPop(text_select_color, popped);}
internal Vec4F32 ui_pop_text_cursor_color(void) {Vec4F32 popped; return UI_StackPop(text_cursor_color, popped);}
internal OS_Cursor ui_pop_hover_cursor(void) {OS_Cursor popped; return UI_StackPop(hover_cursor, popped);}
internal F_Tag ui_pop_font(void) {F_Tag popped; return UI_StackPop(font, popped);}
internal F32 ui_pop_font_size(void) {F32 popped; return UI_StackPop(font_size, popped);}
internal F32 ui_pop_corner_radius_00(void) {F32 popped; return UI_StackPop(corner_radius_00, popped);}
internal F32 ui_pop_corner_radius_01(void) {F32 popped; return UI_StackPop(corner_radius_01, popped);}
internal F32 ui_pop_corner_radius_10(void) {F32 popped; return UI_StackPop(corner_radius_10, popped);}
internal F32 ui_pop_corner_radius_11(void) {F32 popped; return UI_StackPop(corner_radius_11, popped);}
internal F32 ui_pop_blur_size(void) {F32 popped; return UI_StackPop(blur_size, popped);}
internal F32 ui_pop_text_padding(void) {F32 popped; return UI_StackPop(text_padding, popped);}
internal UI_TextAlign ui_pop_text_alignment(void) {UI_TextAlign popped; return UI_StackPop(text_alignment, popped);}
internal UI_Box * ui_top_parent(void) {return UI_StackTop(parent);}
internal Axis2 ui_top_child_layout_axis(void) {return UI_StackTop(child_layout_axis);}
internal F32 ui_top_fixed_x(void) {return UI_StackTop(fixed_x);}
internal F32 ui_top_fixed_y(void) {return UI_StackTop(fixed_y);}
internal F32 ui_top_fixed_width(void) {return UI_StackTop(fixed_width);}
internal F32 ui_top_fixed_height(void) {return UI_StackTop(fixed_height);}
internal UI_Size ui_top_pref_width(void) {return UI_StackTop(pref_width);}
internal UI_Size ui_top_pref_height(void) {return UI_StackTop(pref_height);}
internal UI_BoxFlags ui_top_flags(void) {return UI_StackTop(flags);}
internal U32 ui_top_fastpath_codepoint(void) {return UI_StackTop(fastpath_codepoint);}
internal Vec4F32 ui_top_background_color(void) {return UI_StackTop(background_color);}
internal Vec4F32 ui_top_text_color(void) {return UI_StackTop(text_color);}
internal Vec4F32 ui_top_border_color(void) {return UI_StackTop(border_color);}
internal Vec4F32 ui_top_overlay_color(void) {return UI_StackTop(overlay_color);}
internal Vec4F32 ui_top_text_select_color(void) {return UI_StackTop(text_select_color);}
internal Vec4F32 ui_top_text_cursor_color(void) {return UI_StackTop(text_cursor_color);}
internal OS_Cursor ui_top_hover_cursor(void) {return UI_StackTop(hover_cursor);}
internal F_Tag ui_top_font(void) {return UI_StackTop(font);}
internal F32 ui_top_font_size(void) {return UI_StackTop(font_size);}
internal F32 ui_top_corner_radius_00(void) {return UI_StackTop(corner_radius_00);}
internal F32 ui_top_corner_radius_01(void) {return UI_StackTop(corner_radius_01);}
internal F32 ui_top_corner_radius_10(void) {return UI_StackTop(corner_radius_10);}
internal F32 ui_top_corner_radius_11(void) {return UI_StackTop(corner_radius_11);}
internal F32 ui_top_blur_size(void) {return UI_StackTop(blur_size);}
internal F32 ui_top_text_padding(void) {return UI_StackTop(text_padding);}
internal UI_TextAlign ui_top_text_alignment(void) {return UI_StackTop(text_alignment);}
internal UI_Box * ui_bottom_parent(void) {return UI_StackBottom(parent);}
internal Axis2 ui_bottom_child_layout_axis(void) {return UI_StackBottom(child_layout_axis);}
internal F32 ui_bottom_fixed_x(void) {return UI_StackBottom(fixed_x);}
internal F32 ui_bottom_fixed_y(void) {return UI_StackBottom(fixed_y);}
internal F32 ui_bottom_fixed_width(void) {return UI_StackBottom(fixed_width);}
internal F32 ui_bottom_fixed_height(void) {return UI_StackBottom(fixed_height);}
internal UI_Size ui_bottom_pref_width(void) {return UI_StackBottom(pref_width);}
internal UI_Size ui_bottom_pref_height(void) {return UI_StackBottom(pref_height);}
internal UI_BoxFlags ui_bottom_flags(void) {return UI_StackBottom(flags);}
internal U32 ui_bottom_fastpath_codepoint(void) {return UI_StackBottom(fastpath_codepoint);}
internal Vec4F32 ui_bottom_background_color(void) {return UI_StackBottom(background_color);}
internal Vec4F32 ui_bottom_text_color(void) {return UI_StackBottom(text_color);}
internal Vec4F32 ui_bottom_border_color(void) {return UI_StackBottom(border_color);}
internal Vec4F32 ui_bottom_overlay_color(void) {return UI_StackBottom(overlay_color);}
internal Vec4F32 ui_bottom_text_select_color(void) {return UI_StackBottom(text_select_color);}
internal Vec4F32 ui_bottom_text_cursor_color(void) {return UI_StackBottom(text_cursor_color);}
internal OS_Cursor ui_bottom_hover_cursor(void) {return UI_StackBottom(hover_cursor);}
internal F_Tag ui_bottom_font(void) {return UI_StackBottom(font);}
internal F32 ui_bottom_font_size(void) {return UI_StackBottom(font_size);}
internal F32 ui_bottom_corner_radius_00(void) {return UI_StackBottom(corner_radius_00);}
internal F32 ui_bottom_corner_radius_01(void) {return UI_StackBottom(corner_radius_01);}
internal F32 ui_bottom_corner_radius_10(void) {return UI_StackBottom(corner_radius_10);}
internal F32 ui_bottom_corner_radius_11(void) {return UI_StackBottom(corner_radius_11);}
internal F32 ui_bottom_blur_size(void) {return UI_StackBottom(blur_size);}
internal F32 ui_bottom_text_padding(void) {return UI_StackBottom(text_padding);}
internal UI_TextAlign ui_bottom_text_alignment(void) {return UI_StackBottom(text_alignment);}
internal UI_Box * ui_set_next_parent(UI_Box * v) {return UI_StackSetNext(parent, v);}
internal Axis2 ui_set_next_child_layout_axis(Axis2 v) {return UI_StackSetNext(child_layout_axis, v);}
internal F32 ui_set_next_fixed_x(F32 v) {return UI_StackSetNext(fixed_x, v);}
internal F32 ui_set_next_fixed_y(F32 v) {return UI_StackSetNext(fixed_y, v);}
internal F32 ui_set_next_fixed_width(F32 v) {return UI_StackSetNext(fixed_width, v);}
internal F32 ui_set_next_fixed_height(F32 v) {return UI_StackSetNext(fixed_height, v);}
internal UI_Size ui_set_next_pref_width(UI_Size v) {return UI_StackSetNext(pref_width, v);}
internal UI_Size ui_set_next_pref_height(UI_Size v) {return UI_StackSetNext(pref_height, v);}
internal UI_BoxFlags ui_set_next_flags(UI_BoxFlags v) {return UI_StackSetNext(flags, v);}
internal U32 ui_set_next_fastpath_codepoint(U32 v) {return UI_StackSetNext(fastpath_codepoint, v);}
internal Vec4F32 ui_set_next_background_color(Vec4F32 v) {return UI_StackSetNext(background_color, v);}
internal Vec4F32 ui_set_next_text_color(Vec4F32 v) {return UI_StackSetNext(text_color, v);}
internal Vec4F32 ui_set_next_border_color(Vec4F32 v) {return UI_StackSetNext(border_color, v);}
internal Vec4F32 ui_set_next_overlay_color(Vec4F32 v) {return UI_StackSetNext(overlay_color, v);}
internal Vec4F32 ui_set_next_text_select_color(Vec4F32 v) {return UI_StackSetNext(text_select_color, v);}
internal Vec4F32 ui_set_next_text_cursor_color(Vec4F32 v) {return UI_StackSetNext(text_cursor_color, v);}
internal OS_Cursor ui_set_next_hover_cursor(OS_Cursor v) {return UI_StackSetNext(hover_cursor, v);}
internal F_Tag ui_set_next_font(F_Tag v) {return UI_StackSetNext(font, v);}
internal F32 ui_set_next_font_size(F32 v) {return UI_StackSetNext(font_size, v);}
internal F32 ui_set_next_corner_radius_00(F32 v) {return UI_StackSetNext(corner_radius_00, v);}
internal F32 ui_set_next_corner_radius_01(F32 v) {return UI_StackSetNext(corner_radius_01, v);}
internal F32 ui_set_next_corner_radius_10(F32 v) {return UI_StackSetNext(corner_radius_10, v);}
internal F32 ui_set_next_corner_radius_11(F32 v) {return UI_StackSetNext(corner_radius_11, v);}
internal F32 ui_set_next_blur_size(F32 v) {return UI_StackSetNext(blur_size, v);}
internal F32 ui_set_next_text_padding(F32 v) {return UI_StackSetNext(text_padding, v);}
internal UI_TextAlign ui_set_next_text_alignment(UI_TextAlign v) {return UI_StackSetNext(text_alignment, v);}
