// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_FRONTEND_WIDGETS_H
#define DBG_FRONTEND_WIDGETS_H

////////////////////////////////
//~ rjf: Line Edit Types

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

////////////////////////////////
//~ rjf: Code Slice Types

typedef U32 DF_CodeSliceFlags;
enum
{
  DF_CodeSliceFlag_Clickable         = (1<<0),
  DF_CodeSliceFlag_PriorityMargin    = (1<<1),
  DF_CodeSliceFlag_CatchallMargin    = (1<<2),
  DF_CodeSliceFlag_LineNums          = (1<<3),
};

typedef struct DF_CodeSliceParams DF_CodeSliceParams;
struct DF_CodeSliceParams
{
  // rjf: content
  DF_CodeSliceFlags flags;
  Rng1S64 line_num_range;
  String8 *line_text;
  Rng1U64 *line_ranges;
  TXT_TokenArray *line_tokens;
  D_EntityList *line_bps;
  D_EntityList *line_ips;
  D_EntityList *line_pins;
  U64 *line_vaddrs;
  D_LineList *line_infos;
  DI_KeyList relevant_dbgi_keys;
  
  // rjf: visual parameters
  FNT_Tag font;
  F32 font_size;
  F32 tab_size;
  String8 search_query;
  F32 line_height_px;
  F32 priority_margin_width_px;
  F32 catchall_margin_width_px;
  F32 line_num_width_px;
  F32 line_text_max_width_px;
  F32 margin_float_off_px;
};

typedef struct DF_CodeSliceSignal DF_CodeSliceSignal;
struct DF_CodeSliceSignal
{
  UI_Signal base;
  TxtPt mouse_pt;
  TxtRng mouse_expr_rng;
};

////////////////////////////////
//~ rjf: UI Building Helpers

#define DF_Palette(code) UI_Palette(df_palette_from_code(code))
#define DF_Font(slot) UI_Font(df_font_from_slot(slot)) UI_TextRasterFlags(df_raster_flags_from_slot((slot)))

////////////////////////////////
//~ rjf: UI Widgets: Loading Overlay

internal void df_loading_overlay(Rng2F32 rect, F32 loading_t, U64 progress_v, U64 progress_v_target);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Buttons

internal void df_cmd_binding_buttons(String8 name);
internal UI_Signal df_menu_bar_button(String8 string);
internal UI_Signal df_cmd_spec_button(String8 name);
internal void df_cmd_list_menu_buttons(U64 count, String8 *cmd_names, U32 *fastpath_codepoints);
internal UI_Signal df_icon_button(DF_IconKind kind, FuzzyMatchRangeList *matches, String8 string);
internal UI_Signal df_icon_buttonf(DF_IconKind kind, FuzzyMatchRangeList *matches, char *fmt, ...);
internal void df_entity_tooltips(D_Entity *entity);
internal UI_Signal df_entity_desc_button(D_Entity *entity, FuzzyMatchRangeList *name_matches, String8 fuzzy_query, B32 is_implicit);
internal void df_src_loc_button(String8 file_path, TxtPt point);

////////////////////////////////
//~ rjf: UI Widgets: Text View

internal UI_BOX_CUSTOM_DRAW(df_thread_box_draw_extensions);
internal UI_BOX_CUSTOM_DRAW(df_bp_box_draw_extensions);
internal DF_CodeSliceSignal df_code_slice(DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, String8 string);
internal DF_CodeSliceSignal df_code_slicef(DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, char *fmt, ...);

internal B32 df_do_txt_controls(TXT_TextInfo *info, String8 data, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Labels

internal UI_Signal df_label(String8 string);
internal UI_Signal df_error_label(String8 string);
internal B32 df_help_label(String8 string);
internal DR_FancyStringList df_fancy_string_list_from_code_string(Arena *arena, F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);
internal UI_Box *df_code_label(F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);

////////////////////////////////
//~ rjf: UI Widgets: Line Edit

internal UI_Signal df_line_edit(DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, String8 string);
internal UI_Signal df_line_editf(DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, char *fmt, ...);

#endif // DBG_FRONTEND_WIDGETS_H
