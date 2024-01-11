// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef UI_META_H
#define UI_META_H

#define UI_StackDecls struct{\
struct { UI_Box * active; UI_Box * v[64]; U64 count; B32 auto_pop; } parent;\
struct { Axis2 active; Axis2 v[64]; U64 count; B32 auto_pop; } child_layout_axis;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } fixed_x;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } fixed_y;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } fixed_width;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } fixed_height;\
struct { UI_Size active; UI_Size v[64]; U64 count; B32 auto_pop; } pref_width;\
struct { UI_Size active; UI_Size v[64]; U64 count; B32 auto_pop; } pref_height;\
struct { UI_BoxFlags active; UI_BoxFlags v[64]; U64 count; B32 auto_pop; } flags;\
struct { U32 active; U32 v[64]; U64 count; B32 auto_pop; } fastpath_codepoint;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } background_color;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } text_color;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } border_color;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } overlay_color;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } text_select_color;\
struct { Vec4F32 active; Vec4F32 v[64]; U64 count; B32 auto_pop; } text_cursor_color;\
struct { OS_Cursor active; OS_Cursor v[64]; U64 count; B32 auto_pop; } hover_cursor;\
struct { F_Tag active; F_Tag v[64]; U64 count; B32 auto_pop; } font;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } font_size;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } corner_radius_00;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } corner_radius_01;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } corner_radius_10;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } corner_radius_11;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } blur_size;\
struct { F32 active; F32 v[64]; U64 count; B32 auto_pop; } text_padding;\
struct { UI_TextAlign active; UI_TextAlign v[64]; U64 count; B32 auto_pop; } text_alignment;\
}
#define UI_ZeroAllStacks(ui_state) do{\
MemoryZeroStruct(&ui_state->parent);\
MemoryZeroStruct(&ui_state->child_layout_axis);\
MemoryZeroStruct(&ui_state->fixed_x);\
MemoryZeroStruct(&ui_state->fixed_y);\
MemoryZeroStruct(&ui_state->fixed_width);\
MemoryZeroStruct(&ui_state->fixed_height);\
MemoryZeroStruct(&ui_state->pref_width);\
MemoryZeroStruct(&ui_state->pref_height);\
MemoryZeroStruct(&ui_state->flags);\
MemoryZeroStruct(&ui_state->fastpath_codepoint);\
MemoryZeroStruct(&ui_state->background_color);\
MemoryZeroStruct(&ui_state->text_color);\
MemoryZeroStruct(&ui_state->border_color);\
MemoryZeroStruct(&ui_state->overlay_color);\
MemoryZeroStruct(&ui_state->text_select_color);\
MemoryZeroStruct(&ui_state->text_cursor_color);\
MemoryZeroStruct(&ui_state->hover_cursor);\
MemoryZeroStruct(&ui_state->font);\
MemoryZeroStruct(&ui_state->font_size);\
MemoryZeroStruct(&ui_state->corner_radius_00);\
MemoryZeroStruct(&ui_state->corner_radius_01);\
MemoryZeroStruct(&ui_state->corner_radius_10);\
MemoryZeroStruct(&ui_state->corner_radius_11);\
MemoryZeroStruct(&ui_state->blur_size);\
MemoryZeroStruct(&ui_state->text_padding);\
MemoryZeroStruct(&ui_state->text_alignment);\
} while(0)
#define UI_AutoPopAllStacks(ui_state) do{\
if(ui_state->parent.auto_pop) {ui_state->parent.auto_pop = 0; ui_pop_parent();}\
if(ui_state->child_layout_axis.auto_pop) {ui_state->child_layout_axis.auto_pop = 0; ui_pop_child_layout_axis();}\
if(ui_state->fixed_x.auto_pop) {ui_state->fixed_x.auto_pop = 0; ui_pop_fixed_x();}\
if(ui_state->fixed_y.auto_pop) {ui_state->fixed_y.auto_pop = 0; ui_pop_fixed_y();}\
if(ui_state->fixed_width.auto_pop) {ui_state->fixed_width.auto_pop = 0; ui_pop_fixed_width();}\
if(ui_state->fixed_height.auto_pop) {ui_state->fixed_height.auto_pop = 0; ui_pop_fixed_height();}\
if(ui_state->pref_width.auto_pop) {ui_state->pref_width.auto_pop = 0; ui_pop_pref_width();}\
if(ui_state->pref_height.auto_pop) {ui_state->pref_height.auto_pop = 0; ui_pop_pref_height();}\
if(ui_state->flags.auto_pop) {ui_state->flags.auto_pop = 0; ui_pop_flags();}\
if(ui_state->fastpath_codepoint.auto_pop) {ui_state->fastpath_codepoint.auto_pop = 0; ui_pop_fastpath_codepoint();}\
if(ui_state->background_color.auto_pop) {ui_state->background_color.auto_pop = 0; ui_pop_background_color();}\
if(ui_state->text_color.auto_pop) {ui_state->text_color.auto_pop = 0; ui_pop_text_color();}\
if(ui_state->border_color.auto_pop) {ui_state->border_color.auto_pop = 0; ui_pop_border_color();}\
if(ui_state->overlay_color.auto_pop) {ui_state->overlay_color.auto_pop = 0; ui_pop_overlay_color();}\
if(ui_state->text_select_color.auto_pop) {ui_state->text_select_color.auto_pop = 0; ui_pop_text_select_color();}\
if(ui_state->text_cursor_color.auto_pop) {ui_state->text_cursor_color.auto_pop = 0; ui_pop_text_cursor_color();}\
if(ui_state->hover_cursor.auto_pop) {ui_state->hover_cursor.auto_pop = 0; ui_pop_hover_cursor();}\
if(ui_state->font.auto_pop) {ui_state->font.auto_pop = 0; ui_pop_font();}\
if(ui_state->font_size.auto_pop) {ui_state->font_size.auto_pop = 0; ui_pop_font_size();}\
if(ui_state->corner_radius_00.auto_pop) {ui_state->corner_radius_00.auto_pop = 0; ui_pop_corner_radius_00();}\
if(ui_state->corner_radius_01.auto_pop) {ui_state->corner_radius_01.auto_pop = 0; ui_pop_corner_radius_01();}\
if(ui_state->corner_radius_10.auto_pop) {ui_state->corner_radius_10.auto_pop = 0; ui_pop_corner_radius_10();}\
if(ui_state->corner_radius_11.auto_pop) {ui_state->corner_radius_11.auto_pop = 0; ui_pop_corner_radius_11();}\
if(ui_state->blur_size.auto_pop) {ui_state->blur_size.auto_pop = 0; ui_pop_blur_size();}\
if(ui_state->text_padding.auto_pop) {ui_state->text_padding.auto_pop = 0; ui_pop_text_padding();}\
if(ui_state->text_alignment.auto_pop) {ui_state->text_alignment.auto_pop = 0; ui_pop_text_alignment();}\
} while(0)
internal UI_Box *                   ui_push_parent(UI_Box * v);
internal Axis2                      ui_push_child_layout_axis(Axis2 v);
internal F32                        ui_push_fixed_x(F32 v);
internal F32                        ui_push_fixed_y(F32 v);
internal F32                        ui_push_fixed_width(F32 v);
internal F32                        ui_push_fixed_height(F32 v);
internal UI_Size                    ui_push_pref_width(UI_Size v);
internal UI_Size                    ui_push_pref_height(UI_Size v);
internal UI_BoxFlags                ui_push_flags(UI_BoxFlags v);
internal U32                        ui_push_fastpath_codepoint(U32 v);
internal Vec4F32                    ui_push_background_color(Vec4F32 v);
internal Vec4F32                    ui_push_text_color(Vec4F32 v);
internal Vec4F32                    ui_push_border_color(Vec4F32 v);
internal Vec4F32                    ui_push_overlay_color(Vec4F32 v);
internal Vec4F32                    ui_push_text_select_color(Vec4F32 v);
internal Vec4F32                    ui_push_text_cursor_color(Vec4F32 v);
internal OS_Cursor                  ui_push_hover_cursor(OS_Cursor v);
internal F_Tag                      ui_push_font(F_Tag v);
internal F32                        ui_push_font_size(F32 v);
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
internal UI_BoxFlags                ui_pop_flags(void);
internal U32                        ui_pop_fastpath_codepoint(void);
internal Vec4F32                    ui_pop_background_color(void);
internal Vec4F32                    ui_pop_text_color(void);
internal Vec4F32                    ui_pop_border_color(void);
internal Vec4F32                    ui_pop_overlay_color(void);
internal Vec4F32                    ui_pop_text_select_color(void);
internal Vec4F32                    ui_pop_text_cursor_color(void);
internal OS_Cursor                  ui_pop_hover_cursor(void);
internal F_Tag                      ui_pop_font(void);
internal F32                        ui_pop_font_size(void);
internal F32                        ui_pop_corner_radius_00(void);
internal F32                        ui_pop_corner_radius_01(void);
internal F32                        ui_pop_corner_radius_10(void);
internal F32                        ui_pop_corner_radius_11(void);
internal F32                        ui_pop_blur_size(void);
internal F32                        ui_pop_text_padding(void);
internal UI_TextAlign               ui_pop_text_alignment(void);
internal UI_Box *                   ui_top_parent(void);
internal Axis2                      ui_top_child_layout_axis(void);
internal F32                        ui_top_fixed_x(void);
internal F32                        ui_top_fixed_y(void);
internal F32                        ui_top_fixed_width(void);
internal F32                        ui_top_fixed_height(void);
internal UI_Size                    ui_top_pref_width(void);
internal UI_Size                    ui_top_pref_height(void);
internal UI_BoxFlags                ui_top_flags(void);
internal U32                        ui_top_fastpath_codepoint(void);
internal Vec4F32                    ui_top_background_color(void);
internal Vec4F32                    ui_top_text_color(void);
internal Vec4F32                    ui_top_border_color(void);
internal Vec4F32                    ui_top_overlay_color(void);
internal Vec4F32                    ui_top_text_select_color(void);
internal Vec4F32                    ui_top_text_cursor_color(void);
internal OS_Cursor                  ui_top_hover_cursor(void);
internal F_Tag                      ui_top_font(void);
internal F32                        ui_top_font_size(void);
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
internal UI_BoxFlags                ui_bottom_flags(void);
internal U32                        ui_bottom_fastpath_codepoint(void);
internal Vec4F32                    ui_bottom_background_color(void);
internal Vec4F32                    ui_bottom_text_color(void);
internal Vec4F32                    ui_bottom_border_color(void);
internal Vec4F32                    ui_bottom_overlay_color(void);
internal Vec4F32                    ui_bottom_text_select_color(void);
internal Vec4F32                    ui_bottom_text_cursor_color(void);
internal OS_Cursor                  ui_bottom_hover_cursor(void);
internal F_Tag                      ui_bottom_font(void);
internal F32                        ui_bottom_font_size(void);
internal F32                        ui_bottom_corner_radius_00(void);
internal F32                        ui_bottom_corner_radius_01(void);
internal F32                        ui_bottom_corner_radius_10(void);
internal F32                        ui_bottom_corner_radius_11(void);
internal F32                        ui_bottom_blur_size(void);
internal F32                        ui_bottom_text_padding(void);
internal UI_TextAlign               ui_bottom_text_alignment(void);
internal UI_Box *                   ui_set_next_parent(UI_Box * v);
internal Axis2                      ui_set_next_child_layout_axis(Axis2 v);
internal F32                        ui_set_next_fixed_x(F32 v);
internal F32                        ui_set_next_fixed_y(F32 v);
internal F32                        ui_set_next_fixed_width(F32 v);
internal F32                        ui_set_next_fixed_height(F32 v);
internal UI_Size                    ui_set_next_pref_width(UI_Size v);
internal UI_Size                    ui_set_next_pref_height(UI_Size v);
internal UI_BoxFlags                ui_set_next_flags(UI_BoxFlags v);
internal U32                        ui_set_next_fastpath_codepoint(U32 v);
internal Vec4F32                    ui_set_next_background_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_text_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_border_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_overlay_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_text_select_color(Vec4F32 v);
internal Vec4F32                    ui_set_next_text_cursor_color(Vec4F32 v);
internal OS_Cursor                  ui_set_next_hover_cursor(OS_Cursor v);
internal F_Tag                      ui_set_next_font(F_Tag v);
internal F32                        ui_set_next_font_size(F32 v);
internal F32                        ui_set_next_corner_radius_00(F32 v);
internal F32                        ui_set_next_corner_radius_01(F32 v);
internal F32                        ui_set_next_corner_radius_10(F32 v);
internal F32                        ui_set_next_corner_radius_11(F32 v);
internal F32                        ui_set_next_blur_size(F32 v);
internal F32                        ui_set_next_text_padding(F32 v);
internal UI_TextAlign               ui_set_next_text_alignment(UI_TextAlign v);
#if 0
#define UI_Parent(v)                DeferLoop(ui_push_parent(v), ui_pop_parent())
#define UI_ChildLayoutAxis(v)       DeferLoop(ui_push_child_layout_axis(v), ui_pop_child_layout_axis())
#define UI_FixedX(v)                DeferLoop(ui_push_fixed_x(v), ui_pop_fixed_x())
#define UI_FixedY(v)                DeferLoop(ui_push_fixed_y(v), ui_pop_fixed_y())
#define UI_FixedWidth(v)            DeferLoop(ui_push_fixed_width(v), ui_pop_fixed_width())
#define UI_FixedHeight(v)           DeferLoop(ui_push_fixed_height(v), ui_pop_fixed_height())
#define UI_PrefWidth(v)             DeferLoop(ui_push_pref_width(v), ui_pop_pref_width())
#define UI_PrefHeight(v)            DeferLoop(ui_push_pref_height(v), ui_pop_pref_height())
#define UI_Flags(v)                 DeferLoop(ui_push_flags(v), ui_pop_flags())
#define UI_FastpathCodepoint(v)     DeferLoop(ui_push_fastpath_codepoint(v), ui_pop_fastpath_codepoint())
#define UI_BackgroundColor(v)       DeferLoop(ui_push_background_color(v), ui_pop_background_color())
#define UI_TextColor(v)             DeferLoop(ui_push_text_color(v), ui_pop_text_color())
#define UI_BorderColor(v)           DeferLoop(ui_push_border_color(v), ui_pop_border_color())
#define UI_OverlayColor(v)          DeferLoop(ui_push_overlay_color(v), ui_pop_overlay_color())
#define UI_TextSelectColor(v)       DeferLoop(ui_push_text_select_color(v), ui_pop_text_select_color())
#define UI_TextCursorColor(v)       DeferLoop(ui_push_text_cursor_color(v), ui_pop_text_cursor_color())
#define UI_HoverCursor(v)           DeferLoop(ui_push_hover_cursor(v), ui_pop_hover_cursor())
#define UI_Font(v)                  DeferLoop(ui_push_font(v), ui_pop_font())
#define UI_FontSize(v)              DeferLoop(ui_push_font_size(v), ui_pop_font_size())
#define UI_CornerRadius00(v)        DeferLoop(ui_push_corner_radius_00(v), ui_pop_corner_radius_00())
#define UI_CornerRadius01(v)        DeferLoop(ui_push_corner_radius_01(v), ui_pop_corner_radius_01())
#define UI_CornerRadius10(v)        DeferLoop(ui_push_corner_radius_10(v), ui_pop_corner_radius_10())
#define UI_CornerRadius11(v)        DeferLoop(ui_push_corner_radius_11(v), ui_pop_corner_radius_11())
#define UI_BlurSize(v)              DeferLoop(ui_push_blur_size(v), ui_pop_blur_size())
#define UI_TextPadding(v)           DeferLoop(ui_push_text_padding(v), ui_pop_text_padding())
#define UI_TextAlignment(v)         DeferLoop(ui_push_text_alignment(v), ui_pop_text_alignment())
#endif

#endif // UI_META_H
