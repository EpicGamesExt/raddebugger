// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_WIDGETS_H
#define RADDBG_WIDGETS_H

////////////////////////////////
//~ rjf: Cell Types

typedef U32 RD_CellFlags;
enum
{
  //- rjf: expander
  RD_CellFlag_Expander            = (1<<0),
  RD_CellFlag_ExpanderSpace       = (1<<1),
  RD_CellFlag_ExpanderPlaceholder = (1<<2),
  
  //- rjf: toggle switch extension
  RD_CellFlag_ToggleSwitch        = (1<<3),
  
  //- rjf: slider extension
  RD_CellFlag_Slider              = (1<<4),
  
  //- rjf: behavior
  RD_CellFlag_DisableEdit         = (1<<5),
  RD_CellFlag_KeyboardClickable   = (1<<6),
  RD_CellFlag_SingleClickActivate = (1<<7),
  
  //- rjf: contents description
  RD_CellFlag_CodeContents        = (1<<8),
  
  //- rjf: appearance
  RD_CellFlag_Border              = (1<<9),
  RD_CellFlag_NoBackground        = (1<<10),
  RD_CellFlag_Button              = (1<<11),
  RD_CellFlag_PreferDisplayString = (1<<12),
  RD_CellFlag_DisplayStringIsCode = (1<<13),
};

typedef struct RD_CellParams RD_CellParams;
struct RD_CellParams
{
  //- rjf: catachall parameters
  RD_CellFlags flags;
  S32 depth;
  FuzzyMatchRangeList *fuzzy_matches;
  String8 pre_edit_value;
  DR_FStrList fstrs;
  
  //- rjf: expander r/w info
  B32 *expanded_out;
  
  //- rjf: toggle-switch r/w info
  B32 *toggled_out;
  
  //- rjf: slider info r/w info
  Rng1U64 slider_value_range;
  U64 *slider_value_out;
  
  //- rjf: text editing r/w info
  TxtPt *cursor;
  TxtPt *mark;
  U8 *edit_buffer;
  U64 edit_buffer_size;
  U64 *edit_string_size_out;
};

////////////////////////////////
//~ rjf: Code Slice Types

typedef U32 RD_CodeSliceFlags;
enum
{
  RD_CodeSliceFlag_Clickable         = (1<<0),
  RD_CodeSliceFlag_PriorityMargin    = (1<<1),
  RD_CodeSliceFlag_CatchallMargin    = (1<<2),
  RD_CodeSliceFlag_LineNums          = (1<<3),
};

typedef struct RD_CodeSliceParams RD_CodeSliceParams;
struct RD_CodeSliceParams
{
  // rjf: content
  RD_CodeSliceFlags flags;
  Rng1S64 line_num_range;
  String8 *line_text;
  Rng1U64 *line_ranges;
  TXT_TokenArray *line_tokens;
  RD_CfgList *line_bps;
  CTRL_EntityList *line_ips;
  RD_CfgList *line_pins;
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

typedef struct RD_CodeSliceSignal RD_CodeSliceSignal;
struct RD_CodeSliceSignal
{
  UI_Signal base;
  TxtPt mouse_pt;
  TxtRng mouse_expr_rng;
};

////////////////////////////////
//~ rjf: UI Building Helpers

#define RD_Palette(code) UI_Palette(rd_palette_from_code(code))
#define RD_Font(slot) UI_Font(rd_font_from_slot(slot)) UI_TextRasterFlags(rd_raster_flags_from_slot((slot)))

////////////////////////////////
//~ rjf: UI Widgets: Fancy Title Strings

internal DR_FStrList rd_title_fstrs_from_cfg(Arena *arena, RD_Cfg *cfg);
internal DR_FStrList rd_title_fstrs_from_ctrl_entity(Arena *arena, CTRL_Entity *entity, B32 include_extras);
internal DR_FStrList rd_title_fstrs_from_code_name(Arena *arena, String8 code_name);
internal DR_FStrList rd_title_fstrs_from_file_path(Arena *arena, String8 file_path);

////////////////////////////////
//~ rjf: UI Widgets: Loading Overlay

internal void rd_loading_overlay(Rng2F32 rect, F32 loading_t, U64 progress_v, U64 progress_v_target);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Buttons

internal void rd_cmd_binding_buttons(String8 name);
internal UI_Signal rd_menu_bar_button(String8 string);
internal UI_Signal rd_cmd_spec_button(String8 name);
internal void rd_cmd_list_menu_buttons(U64 count, String8 *cmd_names, U32 *fastpath_codepoints);
internal UI_Signal rd_icon_button(RD_IconKind kind, FuzzyMatchRangeList *matches, String8 string);
internal UI_Signal rd_icon_buttonf(RD_IconKind kind, FuzzyMatchRangeList *matches, char *fmt, ...);

////////////////////////////////
//~ rjf: UI Widgets: Text View

internal UI_BOX_CUSTOM_DRAW(rd_thread_box_draw_extensions);
internal UI_BOX_CUSTOM_DRAW(rd_bp_box_draw_extensions);
internal RD_CodeSliceSignal rd_code_slice(RD_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, String8 string);
internal RD_CodeSliceSignal rd_code_slicef(RD_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, char *fmt, ...);

internal B32 rd_do_txt_controls(TXT_TextInfo *info, String8 data, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column);

////////////////////////////////
//~ rjf: UI Widgets: Fancy Labels

internal UI_Signal rd_label(String8 string);
internal UI_Signal rd_error_label(String8 string);
internal B32 rd_help_label(String8 string);
internal DR_FStrList rd_fstrs_from_code_string(Arena *arena, F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);
internal UI_Box *rd_code_label(F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string);

////////////////////////////////
//~ rjf: UI Widgets: Line Edit

internal UI_Signal rd_cell(RD_CellParams *params, String8 string);
internal UI_Signal rd_cellf(RD_CellParams *params, char *fmt, ...);

#endif // RADDBG_WIDGETS_H
