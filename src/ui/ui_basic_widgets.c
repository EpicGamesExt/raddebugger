// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Widgets

internal void
ui_divider(UI_Size size)
{
  UI_Box *parent = ui_top_parent();
  ui_set_next_pref_size(parent->child_layout_axis, size);
  ui_set_next_child_layout_axis(parent->child_layout_axis);
  UI_Box *box = ui_build_box_from_key(0, ui_key_zero());
  UI_Parent(box) UI_PrefSize(parent->child_layout_axis, ui_pct(1, 0))
  {
    ui_build_box_from_key(UI_BoxFlag_DrawSideBottom, ui_key_zero());
    ui_build_box_from_key(0, ui_key_zero());
  }
}

internal UI_Signal
ui_label(String8 string)
{
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_DrawText, str8_zero());
  ui_box_equip_display_string(box, string);
  UI_Signal interact = ui_signal_from_box(box);
  return interact;
}

internal UI_Signal
ui_labelf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal result = ui_label(string);
  scratch_end(scratch);
  return result;
}

internal void
ui_label_multiline(F32 max, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  ui_set_next_child_layout_axis(Axis2_Y);
  ui_set_next_pref_height(ui_children_sum(1));
  UI_Box *box = ui_build_box_from_key(0, ui_key_zero());
  String8List lines = fnt_wrapped_string_lines_from_font_size_string_max(scratch.arena, ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), string, max);
  for(String8Node *n = lines.first; n != 0; n = n->next)
  {
    ui_label(n->string);
  }
  scratch_end(scratch);
}

internal void
ui_label_multilinef(F32 max, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  ui_label_multiline(max, string);
  scratch_end(scratch);
}

internal UI_Signal
ui_button(String8 string)
{
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|
                                         UI_BoxFlag_DrawBackground|
                                         UI_BoxFlag_DrawBorder|
                                         UI_BoxFlag_DrawText|
                                         UI_BoxFlag_DrawHotEffects|
                                         UI_BoxFlag_DrawActiveEffects,
                                         string);
  UI_Signal interact = ui_signal_from_box(box);
  return interact;
}

internal UI_Signal
ui_buttonf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal result = ui_button(string);
  scratch_end(scratch);
  return result;
}

internal UI_Signal
ui_hover_label(String8 string)
{
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, string);
  UI_Signal interact = ui_signal_from_box(box);
  if(ui_hovering(interact))
  {
    box->flags |= UI_BoxFlag_DrawBorder;
  }
  return interact;
}

internal UI_Signal
ui_hover_labelf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_hover_label(string);
  scratch_end(scratch);
  return sig;
}

typedef struct UI_LineEditDrawData UI_LineEditDrawData;
struct UI_LineEditDrawData
{
  String8 edited_string;
  TxtPt cursor;
  TxtPt mark;
};

internal UI_BOX_CUSTOM_DRAW(ui_line_edit_draw)
{
  UI_LineEditDrawData *draw_data = (UI_LineEditDrawData *)user_data;
  FNT_Tag font = box->font;
  F32 font_size = box->font_size;
  F32 tab_size = box->tab_size;
  Vec4F32 cursor_color = ui_color_from_tags_key_name(box->tags_key, str8_lit("cursor"));
  cursor_color.w *= box->parent->parent->focus_active_t;
  Vec4F32 select_color = ui_color_from_tags_key_name(box->tags_key, str8_lit("selection"));
  select_color.w *= 0.1f*(box->parent->parent->focus_active_t*0.2f + 0.8f);
  Vec2F32 text_position = ui_box_text_position(box);
  String8 edited_string = draw_data->edited_string;
  TxtPt cursor = draw_data->cursor;
  TxtPt mark = draw_data->mark;
  F32 cursor_pixel_off = fnt_dim_from_tag_size_string(font, font_size, 0, tab_size, str8_prefix(edited_string, cursor.column-1)).x;
  F32 mark_pixel_off   = fnt_dim_from_tag_size_string(font, font_size, 0, tab_size, str8_prefix(edited_string, mark.column-1)).x;
  F32 cursor_thickness = ClampBot(4.f, font_size/6.f);
  Rng2F32 cursor_rect =
  {
    text_position.x + cursor_pixel_off - cursor_thickness*0.50f,
    box->rect.y0+ui_top_font_size()*0.5f,
    text_position.x + cursor_pixel_off + cursor_thickness*0.50f,
    box->rect.y1-ui_top_font_size()*0.5f,
  };
  Rng2F32 mark_rect =
  {
    text_position.x + mark_pixel_off - cursor_thickness*0.50f,
    box->rect.y0+ui_top_font_size()*0.5f,
    text_position.x + mark_pixel_off + cursor_thickness*0.50f,
    box->rect.y1-ui_top_font_size()*0.5f,
  };
  Rng2F32 select_rect = union_2f32(cursor_rect, mark_rect);
  dr_rect(select_rect, select_color, font_size/2.f, 0, 1.f);
  dr_rect(cursor_rect, cursor_color, 0.f, 0, 1.f);
}

internal UI_Signal
ui_line_edit(TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, String8 pre_edit_value, String8 string)
{
  //- rjf: make key
  UI_Key key = ui_key_from_string(ui_active_seed_key(), string);
  
  //- rjf: calculate focus
  B32 is_auto_focus_hot = ui_is_key_auto_focus_hot(key);
  B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
  ui_push_focus_hot(is_auto_focus_hot ? UI_FocusKind_On : UI_FocusKind_Null);
  ui_push_focus_active(is_auto_focus_active ? UI_FocusKind_On : UI_FocusKind_Null);
  B32 is_focus_hot    = ui_is_focus_hot();
  B32 is_focus_active = ui_is_focus_active();
  B32 is_focus_hot_disabled = (!is_focus_hot && ui_top_focus_hot() == UI_FocusKind_On);
  B32 is_focus_active_disabled = (!is_focus_active && ui_top_focus_active() == UI_FocusKind_On);
  
  //- rjf: build top-level box
  ui_set_next_hover_cursor(is_focus_active ? OS_Cursor_IBar : OS_Cursor_Pointer);
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|
                                      UI_BoxFlag_DrawBorder|
                                      UI_BoxFlag_MouseClickable|
                                      UI_BoxFlag_ClickToFocus|
                                      ((is_auto_focus_hot || is_auto_focus_active)*UI_BoxFlag_KeyboardClickable)|
                                      UI_BoxFlag_DrawHotEffects|
                                      (is_focus_active || is_focus_active_disabled)*(UI_BoxFlag_Clip|UI_BoxFlag_AllowOverflowX|UI_BoxFlag_ViewClamp),
                                      key);
  
  //- rjf: take navigation actions for editing
  B32 changes_made = 0;
  if(is_focus_active)
  {
    Temp scratch = scratch_begin(0, 0);
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      
      // rjf: do not consume anything that doesn't fit a single-line's operations
      if((evt->kind != UI_EventKind_Edit && evt->kind != UI_EventKind_Navigate && evt->kind != UI_EventKind_Text) || evt->delta_2s32.y != 0)
      {
        continue;
      }
      
      // rjf: map this action to an op
      UI_TxtOp op = ui_single_line_txt_op_from_event(scratch.arena, evt, edit_string, *cursor, *mark);
      
      // rjf: perform replace range
      if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
      {
        String8 new_string = ui_push_string_replace_range(scratch.arena, edit_string, r1s64(op.range.min.column, op.range.max.column), op.replace);
        new_string.size = Min(edit_buffer_size, new_string.size);
        MemoryCopy(edit_buffer, new_string.str, new_string.size);
        edit_string_size_out[0] = new_string.size;
      }
      
      // rjf: perform copy
      if(op.flags & UI_TxtOpFlag_Copy)
      {
        os_set_clipboard_text(op.copy);
      }
      
      // rjf: commit op's changed cursor & mark to caller-provided state
      *cursor = op.cursor;
      *mark = op.mark;
      
      // rjf: consume event
      {
        ui_eat_event(evt);
        changes_made = 1;
      }
    }
    scratch_end(scratch);
  }
  
  //- rjf: build contents
  TxtPt mouse_pt = {0};
  F32 cursor_off = 0;
  UI_Parent(box)
  {
    String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
    if(!is_focus_active && !is_focus_active_disabled)
    {
      String8 display_string = ui_display_part_from_key_string(string);
      if(pre_edit_value.size != 0)
      {
        display_string = pre_edit_value;
      }
      ui_label(display_string);
    }
    else
    {
      F32 total_text_width = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), edit_string).x;
      ui_set_next_pref_width(ui_px(total_text_width+ui_top_font_size()*5, 1.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      ui_box_equip_display_string(editstr_box, edit_string);
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), str8_prefix(edit_string, cursor->column-1)).x;
    }
  }
  
  //- rjf: interact
  UI_Signal sig = ui_signal_from_box(box);
  if(!is_focus_active && sig.f&(UI_SignalFlag_DoubleClicked|UI_SignalFlag_KeyboardPressed))
  {
    String8 edit_string = pre_edit_value;
    edit_string.size = Min(edit_buffer_size, pre_edit_value.size);
    MemoryCopy(edit_buffer, edit_string.str, edit_string.size);
    edit_string_size_out[0] = edit_string.size;
    ui_set_auto_focus_active_key(key);
    ui_kill_action();
    *cursor = txt_pt(1, edit_string.size+1);
    *mark = txt_pt(1, 1);
  }
  if(is_focus_active && sig.f&UI_SignalFlag_KeyboardPressed)
  {
    ui_set_auto_focus_active_key(ui_key_zero());
    sig.f |= UI_SignalFlag_Commit;
  }
  if(is_focus_active && ui_dragging(sig))
  {
    if(ui_pressed(sig))
    {
      *mark = mouse_pt;
    }
    *cursor = mouse_pt;
  }
  
  //- rjf: focus cursor
  {
    Rng1F32 cursor_range_px  = r1f32(cursor_off-ui_top_font_size()*2.f, cursor_off+ui_top_font_size()*2.f);
    Rng1F32 visible_range_px = r1f32(box->view_off_target.x, box->view_off_target.x + dim_2f32(box->rect).x);
    cursor_range_px.min = ClampBot(0, cursor_range_px.min);
    cursor_range_px.max = ClampBot(0, cursor_range_px.max);
    F32 min_delta = cursor_range_px.min-visible_range_px.min;
    F32 max_delta = cursor_range_px.max-visible_range_px.max;
    min_delta = Min(min_delta, 0);
    max_delta = Max(max_delta, 0);
    box->view_off_target.x += min_delta;
    box->view_off_target.x += max_delta;
  }
  
  //- rjf: pop focus
  ui_pop_focus_hot();
  ui_pop_focus_active();
  
  return sig;
}

internal UI_Signal
ui_line_editf(TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, String8 pre_edit_value, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal result = ui_line_edit(cursor, mark, edit_buffer, edit_buffer_size, edit_string_size_out, pre_edit_value, string);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Images

typedef struct UI_ImageDrawData UI_ImageDrawData;
struct UI_ImageDrawData
{
  R_Handle texture;
  R_Tex2DSampleKind sample_kind;
  Rng2F32 region;
  Vec4F32 tint;
  F32 blur;
};

internal UI_BOX_CUSTOM_DRAW(ui_image_draw)
{
  UI_ImageDrawData *draw_data = (UI_ImageDrawData *)user_data;
  if(r_handle_match(draw_data->texture, r_handle_zero()))
  {
    R_Rect2DInst *inst = dr_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
    MemoryCopyArray(inst->corner_radii, box->corner_radii);
  }
  else DR_Tex2DSampleKindScope(draw_data->sample_kind)
  {
    R_Rect2DInst *inst = dr_img(box->rect, draw_data->region, draw_data->texture, draw_data->tint, 0, 0, 0);
    MemoryCopyArray(inst->corner_radii, box->corner_radii);
  }
  if(draw_data->blur > 0.01f)
  {
    Rng2F32 clip = box->rect;
    for(UI_Box *b = box->parent; !ui_box_is_nil(b); b = b->parent)
    {
      if(b->flags & UI_BoxFlag_Clip)
      {
        clip = intersect_2f32(b->rect, clip);
      }
    }
    R_PassParams_Blur *blur = dr_blur(intersect_2f32(clip, box->rect), draw_data->blur, 0);
    MemoryCopyArray(blur->corner_radii, box->corner_radii);
  }
}

internal UI_Signal
ui_image(R_Handle texture, R_Tex2DSampleKind sample_kind, Rng2F32 region, Vec4F32 tint, F32 blur, String8 string)
{
  UI_Box *box = ui_build_box_from_string(0, string);
  UI_ImageDrawData *draw_data = push_array(ui_build_arena(), UI_ImageDrawData, 1);
  draw_data->texture = texture;
  draw_data->sample_kind = sample_kind;
  draw_data->region = region;
  draw_data->tint = tint;
  draw_data->blur = blur;
  ui_box_equip_custom_draw(box, ui_image_draw, draw_data);
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal UI_Signal
ui_imagef(R_Handle texture, R_Tex2DSampleKind sample_kind, Rng2F32 region, Vec4F32 tint, F32 blur, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal result = ui_image(texture, sample_kind, region, tint, blur, string);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Special Buttons

internal UI_Signal
ui_expander(B32 is_expanded, String8 string)
{
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_text_alignment(UI_TextAlign_Center);
  ui_set_next_font(ui_icon_font());
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, string);
  ui_box_equip_display_string(box, is_expanded ? str8_lit("v") : str8_lit(">"));
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal UI_Signal
ui_expanderf(B32 is_expanded, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_expander(is_expanded, string);
  scratch_end(scratch);
  return sig;
}

internal UI_Signal
ui_sort_header(B32 sorting, B32 ascending, String8 string)
{
  ui_set_next_child_layout_axis(Axis2_X);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, string);
  ui_push_parent(box);
  
  // rjf: make icon
  if(sorting)
  {
    ui_set_next_pref_width(ui_em(1.8f, 1.f));
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_font(ui_icon_font());
    UI_Box *icon = ui_build_box_from_string(UI_BoxFlag_DrawText, str8_lit(""));
    ui_box_equip_display_string(icon, ascending ? str8_lit("^") : str8_lit("v"));
  }
  
  // rjf: make text
  {
    ui_label(string);
  }
  
  ui_pop_parent();
  UI_Signal interact = ui_signal_from_box(box);
  return interact;
}

internal UI_Signal
ui_sort_headerf(B32 sorting, B32 ascending, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_sort_header(sorting, ascending, string);
  scratch_end(scratch);
  return sig;
}

////////////////////////////////
//~ rjf: Color Pickers

//- rjf: tooltips

internal void
ui_do_color_tooltip_hsv(Vec3F32 hsv)
{
  Vec3F32 rgb = rgb_from_hsv(hsv);
  UI_Tooltip UI_Padding(ui_em(2.f, 1.f))
  {
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_em(6.f, 1.f)) UI_Row UI_Padding(ui_pct(1, 0))
    {
      UI_BackgroundColor(v4f32(rgb.x, rgb.y, rgb.z, 1.f))
        UI_CornerRadius(4.f)
        UI_PrefWidth(ui_em(6.f, 1.f)) UI_PrefHeight(ui_em(6.f, 1.f))
        ui_build_box_from_string(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, str8_lit(""));
    }
    ui_spacer(ui_em(0.3f, 1.f));
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
    {
      ui_labelf("Hex: #%02x%02x%02x", (U8)(rgb.x*255.f), (U8)(rgb.y*255.f), (U8)(rgb.z*255.f));
    }
    ui_spacer(ui_em(0.3f, 1.f));
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_children_sum(1)) UI_Row
    {
      UI_WidthFill UI_Column UI_PrefHeight(ui_em(1.8f, 1.f))
      {
        ui_labelf("Red: %.2f", rgb.x);
        ui_labelf("Green: %.2f", rgb.y);
        ui_labelf("Blue: %.2f", rgb.z);
      }
      UI_WidthFill UI_Column UI_PrefHeight(ui_em(1.8f, 1.f))
      {
        ui_labelf("Hue: %.2f", hsv.x);
        ui_labelf("Sat: %.2f", hsv.y);
        ui_labelf("Val: %.2f", hsv.z);
      }
    }
  }
}

internal void
ui_do_color_tooltip_hsva(Vec4F32 hsva)
{
  Vec3F32 hsv = v3f32(hsva.x, hsva.y, hsva.z);
  Vec3F32 rgb = rgb_from_hsv(hsv);
  Vec4F32 rgba = v4f32(rgb.x, rgb.y, rgb.z, hsva.w);
  UI_Tooltip UI_Padding(ui_em(2.f, 1.f))
  {
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_em(6.f, 1.f)) UI_Row UI_Padding(ui_pct(1, 0))
    {
      UI_BackgroundColor(rgba)
        UI_CornerRadius(4.f)
        UI_PrefWidth(ui_em(6.f, 1.f)) UI_PrefHeight(ui_em(6.f, 1.f))
        ui_build_box_from_string(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, str8_lit(""));
    }
    ui_spacer(ui_em(0.3f, 1.f));
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
    {
      ui_labelf("Hex: #%02x%02x%02x%02x", (U8)(rgba.x*255.f), (U8)(rgba.y*255.f), (U8)(rgba.z*255.f), (U8)(rgba.w*255.f));
    }
    ui_spacer(ui_em(0.3f, 1.f));
    UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_children_sum(1)) UI_Row
    {
      UI_WidthFill UI_Column UI_PrefHeight(ui_em(1.8f, 1.f))
      {
        ui_labelf("Red: %.2f", rgba.x);
        ui_labelf("Green: %.2f", rgba.y);
        ui_labelf("Blue: %.2f", rgba.z);
        ui_labelf("Alpha: %.2f", rgba.w);
      }
      UI_WidthFill UI_Column UI_PrefHeight(ui_em(1.8f, 1.f))
      {
        ui_labelf("Hue: %.2f", hsva.x);
        ui_labelf("Sat: %.2f", hsva.y);
        ui_labelf("Val: %.2f", hsva.z);
        ui_labelf("Alpha: %.2f", hsva.w);
      }
    }
  }
}

//- rjf: saturation/value picker

typedef struct UI_SatValDrawData UI_SatValDrawData;
struct UI_SatValDrawData
{
  F32 hue;
  F32 sat;
  F32 val;
};

internal UI_BOX_CUSTOM_DRAW(ui_sat_val_picker_draw)
{
  UI_SatValDrawData *data = (UI_SatValDrawData *)user_data;
  
  // rjf: hue => rgb
  Vec3F32 hue_rgb = rgb_from_hsv(v3f32(data->hue, 1, 1));
  
  // rjf: white -> rgb background
  {
    R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, -1.f), v4f32(hue_rgb.x, hue_rgb.y, hue_rgb.z, 1), 4.f, 0, 1.f);
    inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(1, 1, 1, 1);
  }
  
  // rjf: black gradient overlay
  {
    R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, -1.f), v4f32(0, 0, 0, 0), 4.f, 0, 1.f);
    inst->colors[Corner_01] = v4f32(0, 0, 0, 1);
    inst->colors[Corner_11] = v4f32(0, 0, 0, 1);
  }
  
  // rjf: indicator
  {
    Vec2F32 box_rect_dim = dim_2f32(box->rect);
    Vec2F32 center = v2f32(box->rect.x0 + data->sat*box_rect_dim.x, box->rect.y0 + (1-data->val)*box_rect_dim.y);
    F32 half_size = box->font_size * (0.5f + box->active_t*0.2f);
    Rng2F32 rect = r2f32p(center.x - half_size,
                          center.y - half_size,
                          center.x + half_size,
                          center.y + half_size);
    dr_rect(rect, v4f32(1, 1, 1, 1), half_size/2, 2.f, 1.f);
  }
}

internal UI_Signal
ui_sat_val_picker(F32 hue, F32 *out_sat, F32 *out_val, String8 string)
{
  // rjf: build & interact
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable, string);
  UI_SatValDrawData *user = push_array(ui_build_arena(), UI_SatValDrawData, 1);
  ui_box_equip_custom_draw(box, ui_sat_val_picker_draw, user);
  UI_Signal sig = ui_signal_from_box(box);
  
  // rjf: click+draw behavior
  if(ui_dragging(sig))
  {
    Vec2F32 dim = dim_2f32(box->rect);
    *out_sat = (ui_mouse().x - box->rect.x0) / dim.x;
    *out_val = 1 - (ui_mouse().y - box->rect.y0) / dim.y;
    *out_sat = Clamp(0, *out_sat, 1);
    *out_val = Clamp(0, *out_val, 1);
    ui_do_color_tooltip_hsv(v3f32(hue, *out_sat, *out_val));
    if(ui_pressed(sig))
    {
      Vec2F32 data = v2f32(*out_sat, *out_val);
      ui_store_drag_struct(&data);
    }
    if(ui_slot_press(UI_EventActionSlot_Cancel))
    {
      Vec2F32 data = *ui_get_drag_struct(Vec2F32);
      *out_sat = data.x;
      *out_val = data.y;
      ui_kill_action();
    }
  }
  
  // rjf: fill draw data
  {
    user->hue = hue;
    user->sat = *out_sat;
    user->val = *out_val;
  }
  
  return sig;
}

internal UI_Signal
ui_sat_val_pickerf(F32 hue, F32 *out_sat, F32 *out_val, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_sat_val_picker(hue, out_sat, out_val, string);
  scratch_end(scratch);
  return sig;
}

//- rjf: hue picker

typedef struct UI_HueDrawData UI_HueDrawData;
struct UI_HueDrawData
{
  F32 hue;
  F32 sat;
  F32 val;
};

internal UI_BOX_CUSTOM_DRAW(ui_hue_picker_draw)
{
  UI_HueDrawData *data = (UI_HueDrawData *)user_data;
  Vec2F32 dim = dim_2f32(box->rect);
  F32 segment_dim = floor_f32(dim.y/6.f);
  Rng2F32 hue_cycle_rect = box->rect;
  Vec2F32 hue_cycle_center = center_2f32(hue_cycle_rect);
  hue_cycle_rect.x0 += (hue_cycle_center.x - hue_cycle_rect.x0) * 0.3f;
  hue_cycle_rect.x1 += (hue_cycle_center.x - hue_cycle_rect.x1) * 0.3f;
  Rng2F32 rect = r2f32p(hue_cycle_rect.x0,
                        hue_cycle_rect.y0,
                        hue_cycle_rect.x1,
                        hue_cycle_rect.y0 + segment_dim);
  for(int seg = 0; seg < 6; seg += 1)
  {
    F32 hue0 = (F32)(seg)/6;
    F32 hue1 = (F32)(seg+1)/6;
    Vec3F32 rgb0 = rgb_from_hsv(v3f32(hue0, 1, 1));
    Vec3F32 rgb1 = rgb_from_hsv(v3f32(hue1, 1, 1));
    Vec4F32 rgba0 = v4f32(rgb0.x, rgb0.y, rgb0.z, 1);
    Vec4F32 rgba1 = v4f32(rgb1.x, rgb1.y, rgb1.z, 1);
    R_Rect2DInst *inst = dr_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 0.f);
    inst->colors[Corner_00] = rgba0;
    inst->colors[Corner_01] = rgba1;
    inst->colors[Corner_10] = rgba0;
    inst->colors[Corner_11] = rgba1;
    rect.y0 += segment_dim;
    rect.y1 += segment_dim;
  }
  
  // rjf: indicator
  {
    Vec2F32 box_rect_dim = dim_2f32(box->rect);
    Vec2F32 center = v2f32((box->rect.x0+box->rect.x1)/2, box->rect.y0 + data->hue*box_rect_dim.y);
    F32 half_size = box->font_size * (0.5f + box->active_t*0.2f);
    Rng2F32 rect = r2f32p(center.x - half_size,
                          center.y - 2.f,
                          center.x + half_size,
                          center.y + 2.f);
    dr_rect(rect, v4f32(1, 1, 1, 1), half_size/2, 2.f, 1.f);
  }
}

internal UI_Signal
ui_hue_picker(F32 *out_hue, F32 sat, F32 val, String8 string)
{
  // rjf: build & interact
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable, string);
  UI_HueDrawData *user = push_array(ui_build_arena(), UI_HueDrawData, 1);
  ui_box_equip_custom_draw(box, ui_hue_picker_draw, user);
  UI_Signal sig = ui_signal_from_box(box);
  
  // rjf: click+draw behavior
  if(ui_dragging(sig))
  {
    Vec2F32 dim = dim_2f32(box->rect);
    *out_hue = (ui_mouse().y - box->rect.y0) / dim.y;
    *out_hue = Clamp(0, *out_hue, 1);
    ui_do_color_tooltip_hsv(v3f32(*out_hue, sat, val));
    if(ui_pressed(sig))
    {
      ui_store_drag_struct(out_hue);
    }
    if(ui_slot_press(UI_EventActionSlot_Cancel))
    {
      *out_hue = *ui_get_drag_struct(F32);
      ui_kill_action();
    }
  }
  
  // rjf: fill draw data
  {
    user->hue = *out_hue;
    user->sat = sat;
    user->val = val;
  }
  
  return sig;
}

internal UI_Signal
ui_hue_pickerf(F32 *out_hue, F32 sat, F32 val, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_hue_picker(out_hue, sat, val, string);
  scratch_end(scratch);
  return sig;
}

//- rjf: alpha picker

typedef struct UI_AlphaDrawData UI_AlphaDrawData;
struct UI_AlphaDrawData
{
  F32 alpha;
};

internal UI_BOX_CUSTOM_DRAW(ui_alpha_picker_draw)
{
  UI_AlphaDrawData *data = (UI_AlphaDrawData *)user_data;
  Vec2F32 dim = dim_2f32(box->rect);
  
  // rjf: build gradient
  {
    Rng2F32 rect = box->rect;
    Vec2F32 center = center_2f32(rect);
    rect.x0 += (center.x - rect.x0) * 0.3f;
    rect.x1 += (center.x - rect.x1) * 0.3f;
    R_Rect2DInst *inst = dr_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 0);
    inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(1, 1, 1, 1);
  }
  
  // rjf: indicator
  {
    Vec2F32 box_rect_dim = dim_2f32(box->rect);
    Vec2F32 center = v2f32((box->rect.x0+box->rect.x1)/2, box->rect.y0 + (1-data->alpha)*box_rect_dim.y);
    F32 half_size = box->font_size * (0.5f + box->active_t*0.2f);
    Rng2F32 rect = r2f32p(center.x - half_size,
                          center.y - 2.f,
                          center.x + half_size,
                          center.y + 2.f);
    dr_rect(rect, v4f32(1, 1, 1, 1), half_size/2, 2.f, 1.f);
  }
}

internal UI_Signal
ui_alpha_picker(F32 *out_alpha, String8 string)
{
  // rjf: build & interact
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable, string);
  UI_AlphaDrawData *user = push_array(ui_build_arena(), UI_AlphaDrawData, 1);
  ui_box_equip_custom_draw(box, ui_alpha_picker_draw, user);
  UI_Signal sig = ui_signal_from_box(box);
  
  // rjf: click+draw behavior
  if(ui_dragging(sig))
  {
    Vec2F32 dim = dim_2f32(box->rect);
    F32 drag_pct = (ui_mouse().y - box->rect.y0) / dim.y; 
    drag_pct = Clamp(0, drag_pct, 1);
    *out_alpha = 1-drag_pct;
    if(ui_pressed(sig))
    {
      ui_store_drag_struct(out_alpha);
    }
    if(ui_slot_press(UI_EventActionSlot_Cancel))
    {
      *out_alpha = *ui_get_drag_struct(F32);
      ui_kill_action();
    }
  }
  
  // rjf: fill draw data
  {
    user->alpha = *out_alpha;
  }
  
  return sig;
}

internal UI_Signal
ui_alpha_pickerf(F32 *out_alpha, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = ui_alpha_picker(out_alpha, string);
  scratch_end(scratch);
  return sig;
}

////////////////////////////////
//~ rjf: Simple Layout Widgets

internal UI_Box *ui_row_begin(void)    { return ui_named_row_begin(str8_lit("")); }
internal UI_Signal ui_row_end(void)    { return ui_named_row_end(); }
internal UI_Box *ui_column_begin(void) { return ui_named_column_begin(str8_lit("")); }
internal UI_Signal ui_column_end(void) { return ui_named_column_end(); }

internal UI_Box *
ui_named_row_begin(String8 string)
{
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_string(0, string);
  ui_push_parent(box);
  return box;
}

internal UI_Signal
ui_named_row_end(void)
{
  UI_Box *box = ui_pop_parent();
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal UI_Box *
ui_named_column_begin(String8 string)
{
  ui_set_next_child_layout_axis(Axis2_Y);
  UI_Box *box = ui_build_box_from_string(0, string);
  ui_push_parent(box);
  return box;
}

internal UI_Signal
ui_named_column_end(void)
{
  UI_Box *box = ui_pop_parent();
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

////////////////////////////////
//~ rjf: Floating Panes

internal UI_Box *
ui_pane_begin(Rng2F32 rect, String8 string)
{
  ui_push_rect(rect);
  ui_set_next_child_layout_axis(Axis2_Y);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|UI_BoxFlag_Clip|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, string);
  ui_pop_rect();
  ui_push_parent(box);
  ui_push_pref_width(ui_pct(1, 0));
  return box;
}

internal UI_Box *
ui_pane_beginf(Rng2F32 rect, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Box *box = ui_pane_begin(rect, string);
  scratch_end(scratch);
  return box;
}

internal UI_Signal
ui_pane_end(void)
{
  ui_pop_pref_width();
  UI_Box *box = ui_pop_parent();
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

////////////////////////////////
//~ rjf: Tables

thread_static U64 ui_ts_col_pct_count = 0;
thread_static F32 *ui_ts_col_pcts_stable = 0;
thread_static U64 ui_ts_vector_idx = 0;
thread_static U64 ui_ts_cell_idx = 0;

internal void
ui_table_begin(U64 column_pct_count, F32 **column_pcts, String8 string)
{
  //- rjf: store off persistent, user-provided column info
  ui_ts_col_pct_count = column_pct_count;
  
  //- rjf: build main table parent
  ui_set_next_pref_height(ui_children_sum(1));
  ui_set_next_child_layout_axis(Axis2_Y);
  UI_Box *table = ui_build_box_from_string(0, string);
  ui_push_parent(table);
  
  //- rjf: build column boundaries
  F32 x_off = (ui_ts_col_pct_count > 0 ? *column_pcts[0] : 0) * dim_2f32(table->rect).x;
  for(U64 column_idx = 1; column_idx < ui_ts_col_pct_count; column_idx += 1)
  {
    // rjf: build base rectangle
    Rng2F32 rect = {0};
    {
      rect.x0 = x_off-3.f;
      rect.y0 = 0;
      rect.x1 = x_off+3.f;
      rect.y1 = dim_2f32(table->rect).y;
      x_off += *column_pcts[column_idx] * dim_2f32(table->rect).x;
    }
    
    // rjf: make column boundary widget
    UI_Rect(rect)
    {
      ui_set_next_hover_cursor(OS_Cursor_LeftRight);
      UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%S_boundary_%I64u", table->string, column_idx);
      
      F32 *left_pct_ptr  = column_idx < ui_ts_col_pct_count ? column_pcts[column_idx-1] : 0;
      F32 *right_pct_ptr = column_idx < ui_ts_col_pct_count ? column_pcts[column_idx] : 0;
      
      // rjf: boundary dragging
      UI_Signal interact = ui_signal_from_box(box);
      if(ui_dragging(interact))
      {
        if(ui_pressed(interact))
        {
          Vec2F32 v = v2f32(*left_pct_ptr, *right_pct_ptr);
          ui_store_drag_struct(&v);
        }
        
        // rjf: calculate how much space we're dividing amongst the columns that
        // the user can resize
        F32 adjustable_table_dim = 0;
        if(table->child_layout_axis == Axis2_Y)
        {
          adjustable_table_dim = dim_2f32(table->rect).x;
        }
        else
        {
          U64 child_idx = 0;
          for(UI_Box *v = table->first; !ui_box_is_nil(v); v = v->next, child_idx += 1)
          {
            U64 column_idx = (child_idx+1);
            if(column_idx < ui_ts_col_pct_count)
            {
              adjustable_table_dim += dim_2f32(v->rect).x;
            }
            else
            {
              break;
            }
          }
        }
        
        // rjf: calculate diff
        F32 min_size = 30.f;
        F32 left_pct__before     = ui_get_drag_struct(Vec2F32)->x;
        F32 left_pixels__before  = left_pct__before * adjustable_table_dim;
        F32 left_pixels__after   = left_pixels__before + ui_drag_delta().x;
        
        // rjf: clamp left side
        if(left_pixels__after < min_size)
        {
          left_pixels__after = min_size;
        }
        
        // rjf: calculate right side
        F32 left_pct__after      = left_pixels__after / adjustable_table_dim;
        F32 pct_delta            = left_pct__after - left_pct__before;
        F32 right_pct__before    = ui_get_drag_struct(Vec2F32)->y;
        F32 right_pct__after     = right_pct__before - pct_delta;
        F32 right_pixels__after  = right_pct__after * adjustable_table_dim;
        
        // rjf: clamp right side & back-solve
        if(right_pixels__after < min_size)
        {
          right_pixels__after = min_size;
          right_pct__after = right_pixels__after/adjustable_table_dim;
          pct_delta = -(right_pct__after-right_pct__before);
          left_pct__after = left_pct__before+pct_delta;
        }
        
        // rjf: commit new percentages
        *left_pct_ptr = left_pct__after;
        *right_pct_ptr = right_pct__after;
      }
    }
  }
  
  //- rjf: form stable pcts
  ui_ts_col_pcts_stable = push_array(ui_build_arena(), F32, ui_ts_col_pct_count);
  for(U64 idx = 0; idx < column_pct_count; idx += 1)
  {
    ui_ts_col_pcts_stable[idx] = *column_pcts[idx];
  }
  
  ui_ts_vector_idx = 0;
}

internal void
ui_table_beginf(U64 column_pct_count, F32 **column_pcts, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  ui_table_begin(column_pct_count, column_pcts, string);
  scratch_end(scratch);
}

internal void
ui_table_end(void)
{
  ui_pop_parent();
}

internal UI_Box *
ui_named_table_vector_begin(String8 string)
{
  ui_set_next_pref_width(ui_pct(1, 0));
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *vector = ui_build_box_from_string(UI_BoxFlag_DrawSideBottom, string);
  ui_ts_vector_idx += 1;
  ui_ts_cell_idx = 0;
  ui_push_parent(vector);
  return vector;
}

internal UI_Box *
ui_named_table_vector_beginf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Box *vector = ui_named_table_vector_begin(string);
  scratch_end(scratch);
  return vector;
}

internal UI_Box *
ui_table_vector_begin(void)
{
  UI_Box *table = ui_top_parent();
  UI_Box *vector = ui_named_table_vector_beginf("###tbl_vec_%p_%I64u", table, ui_ts_vector_idx);
  return vector;
}

internal UI_Signal
ui_table_vector_end(void)
{
  UI_Box *box = ui_pop_parent();
  return ui_signal_from_box(box);
}

internal UI_Box *
ui_table_cell_begin(void)
{
  U64 column_idx = ui_ts_cell_idx;
  F32 width_pct = column_idx < ui_ts_col_pct_count ? ui_ts_col_pcts_stable[column_idx] : 1.f;
  return ui_table_cell_sized_begin(ui_pct(width_pct, 0));
}

internal UI_Signal
ui_table_cell_end(void)
{
  UI_Box *cell = ui_pop_parent();
  return ui_signal_from_box(cell);
}

internal UI_Box *
ui_table_cell_sized_begin(UI_Size size)
{
  UI_Box *vector = ui_top_parent();
  U64 column_idx = ui_ts_cell_idx;
  ui_ts_cell_idx += 1;
  ui_set_next_pref_width(size);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *cell = ui_build_box_from_stringf((column_idx > 0 ? UI_BoxFlag_DrawSideLeft : 0), "###tbl_cell_%p_%I64u", vector, ui_ts_cell_idx);
  ui_push_parent(cell);
  return cell;
}

////////////////////////////////
//~ rjf: Scroll Regions

internal void
ui_scroll_list_row_block_chunk_list_push(Arena *arena, UI_ScrollListRowBlockChunkList *list, U64 cap, UI_ScrollListRowBlock *block)
{
  UI_ScrollListRowBlockChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = push_array(arena, UI_ScrollListRowBlockChunkNode, 1);
    n->cap = cap;
    n->v = push_array_no_zero(arena, UI_ScrollListRowBlock, n->cap);
    SLLQueuePush(list->first, list->last, n);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&n->v[n->count], block);
  n->count += 1;
  list->total_count += 1;
}

internal UI_ScrollListRowBlockArray
ui_scroll_list_row_block_array_from_chunk_list(Arena *arena, UI_ScrollListRowBlockChunkList *list)
{
  UI_ScrollListRowBlockArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, UI_ScrollListRowBlock, array.count);
  U64 idx = 0;
  for(UI_ScrollListRowBlockChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(n->v[0])*n->count);
    idx += n->count;
  }
  return array;
}

internal U64
ui_scroll_list_row_from_item(UI_ScrollListRowBlockArray *blocks, U64 item)
{
  U64 result = 0;
  {
    U64 row_idx = 0;
    U64 item_idx = 0;
    for(U64 block_idx = 0; block_idx < blocks->count; block_idx += 1)
    {
      UI_ScrollListRowBlock *block = &blocks->v[block_idx];
      U64 next_row_idx = row_idx + block->row_count;
      U64 next_item_idx= item_idx+ block->item_count;
      if(item_idx <= item && item < next_item_idx)
      {
        U64 item_off_rows = (item-item_idx) * (block->row_count/block->item_count);
        result = row_idx + item_off_rows;
        break;
      }
      row_idx = next_row_idx;
      item_idx = next_item_idx;
    }
  }
  return result;
}

internal U64
ui_scroll_list_item_from_row(UI_ScrollListRowBlockArray *blocks, U64 row)
{
  U64 result = 0;
  {
    U64 row_idx = 0;
    U64 item_idx = 0;
    for(U64 block_idx = 0; block_idx < blocks->count; block_idx += 1)
    {
      UI_ScrollListRowBlock *block = &blocks->v[block_idx];
      U64 next_row_idx = row_idx + block->row_count;
      U64 next_item_idx= item_idx+ block->item_count;
      if(row_idx <= row && row < next_row_idx)
      {
        result = item_idx;
        break;
      }
      row_idx = next_row_idx;
      item_idx = next_item_idx;
    }
  }
  return result;
}

internal UI_ScrollPt
ui_scroll_bar(Axis2 axis, UI_Size off_axis_size, UI_ScrollPt pt, Rng1S64 idx_range, S64 view_num_indices)
{
  ui_push_tag(str8_lit("scroll_bar"));
  ui_push_font_size(ui_bottom_font_size()*0.65f);
  
  //- rjf: unpack
  S64 idx_range_dim = Max(dim_1s64(idx_range), 1);
  
  //- rjf: produce extra flags for cases in which scrolling is disabled
  UI_BoxFlags disabled_flags = 0;
  if(idx_range.min == idx_range.max)
  {
    disabled_flags |= UI_BoxFlag_Disabled;
  }
  
  //- rjf: build main container
  ui_set_next_pref_size(axis2_flip(axis), off_axis_size);
  ui_set_next_child_layout_axis(axis);
  UI_Box *container_box = ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
  
  //- rjf: build scroll-min button
  UI_Signal min_scroll_sig = {0};
  UI_Parent(container_box)
    UI_PrefSize(axis, off_axis_size)
    UI_Flags(UI_BoxFlag_DrawBorder|disabled_flags)
    UI_TextAlignment(UI_TextAlign_Center)
    UI_Font(ui_icon_font())
  {
    String8 arrow_string = ui_icon_string_from_kind(axis == Axis2_X ? UI_IconKind_LeftArrow : UI_IconKind_UpArrow);
    min_scroll_sig = ui_buttonf("%S##_min_scroll_%i", arrow_string, axis);
  }
  
  //- rjf: main scroller area
  UI_Signal space_before_sig = {0};
  UI_Signal space_after_sig = {0};
  UI_Signal scroller_sig = {0};
  UI_Box *scroll_area_box = &ui_nil_box;
  UI_Box *scroller_box = &ui_nil_box;
  UI_Parent(container_box)
  {
    ui_set_next_pref_size(axis, ui_pct(1, 0));
    ui_set_next_child_layout_axis(axis);
    scroll_area_box = ui_build_box_from_stringf(0, "##_scroll_area_%i", axis);
    UI_Parent(scroll_area_box)
    {
      // rjf: space before
      if(idx_range.max != idx_range.min)
      {
        ui_set_next_pref_size(axis, ui_pct((F32)((F64)(pt.idx-idx_range.min)/(F64)idx_range_dim), 0));
        UI_Box *space_before_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "##scroll_area_before");
        space_before_sig = ui_signal_from_box(space_before_box);
      }
      
      // rjf: scroller
      UI_Flags(disabled_flags) UI_PrefSize(axis, ui_pct(Clamp(0.05f, (F32)((F64)Max(view_num_indices, 1)/(F64)idx_range_dim), 1.f), 0.f))
      {
        scroller_sig = ui_buttonf("##_scroller_%i", axis);
        scroller_box = scroller_sig.box;
      }
      
      // rjf: space after
      if(idx_range.max != idx_range.min)
      {
        ui_set_next_pref_size(axis, ui_pct(1.f - (F32)((F64)(pt.idx-idx_range.min)/(F64)idx_range_dim), 0));
        UI_Box *space_after_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "##scroll_area_after");
        space_after_sig = ui_signal_from_box(space_after_box);
      }
    }
  }
  
  //- rjf: build scroll-max button
  UI_Signal max_scroll_sig = {0};
  UI_Parent(container_box)
    UI_PrefSize(axis, off_axis_size)
    UI_Flags(UI_BoxFlag_DrawBorder|disabled_flags)
    UI_TextAlignment(UI_TextAlign_Center)
    UI_Font(ui_icon_font())
  {
    String8 arrow_string = ui_icon_string_from_kind(axis == Axis2_X ? UI_IconKind_RightArrow : UI_IconKind_DownArrow);
    max_scroll_sig = ui_buttonf("%S##_max_scroll_%i", arrow_string, axis);
  }
  
  //- rjf: pt * signals -> new pt
  UI_ScrollPt new_pt = pt;
  {
    typedef struct UI_ScrollBarDragData UI_ScrollBarDragData;
    struct UI_ScrollBarDragData
    {
      UI_ScrollPt start_pt;
      F32 scroll_space_px;
    };
    if(ui_dragging(scroller_sig))
    {
      if(ui_pressed(scroller_sig))
      {
        UI_ScrollBarDragData drag_data = {pt, (floor_f32(dim_2f32(scroll_area_box->rect).v[axis])-floor_f32(dim_2f32(scroller_box->rect).v[axis]))};
        ui_store_drag_struct(&drag_data);
      }
      UI_ScrollBarDragData *drag_data = ui_get_drag_struct(UI_ScrollBarDragData);
      UI_ScrollPt original_pt = drag_data->start_pt;
      F32 drag_delta = ui_drag_delta().v[axis];
      F32 drag_pct = drag_delta / drag_data->scroll_space_px;
      S64 new_idx = original_pt.idx + drag_pct*idx_range_dim;
      new_idx = Clamp(idx_range.min, new_idx, idx_range.max);
      ui_scroll_pt_target_idx(&new_pt, new_idx);
      new_pt.off = 0;
    }
    if(ui_dragging(min_scroll_sig) || ui_dragging(space_before_sig))
    {
      S64 new_idx = new_pt.idx-1;
      new_idx = Clamp(idx_range.min, new_idx, idx_range.max);
      ui_scroll_pt_target_idx(&new_pt, new_idx);
    }
    if(ui_dragging(max_scroll_sig) || ui_dragging(space_after_sig))
    {
      S64 new_idx = new_pt.idx+1;
      new_idx = Clamp(idx_range.min, new_idx, idx_range.max);
      ui_scroll_pt_target_idx(&new_pt, new_idx);
    }
  }
  
  ui_pop_font_size();
  ui_pop_tag();
  return new_pt;
}

thread_static UI_ScrollPt *ui_scroll_list_scroll_pt_ptr = 0;
thread_static F32 ui_scroll_list_scroll_bar_dim_px = 0;
thread_static Vec2F32 ui_scroll_list_dim_px = {0};
thread_static Rng1S64 ui_scroll_list_scroll_idx_rng = {0};

internal void
ui_scroll_list_begin(UI_ScrollListParams *params, UI_ScrollPt *scroll_pt, Vec2S64 *cursor_out, Vec2S64 *mark_out, Rng1S64 *visible_row_range_out, UI_ScrollListSignal *signal_out)
{
  //- rjf: unpack arguments
  Rng1S64 scroll_row_idx_range = r1s64(params->item_range.min, ClampBot(params->item_range.min, params->item_range.max-1));
  S64 num_possible_visible_rows = (S64)(params->dim_px.y/params->row_height_px);
  
  //- rjf: do keyboard navigation
  B32 moved = 0;
  if(params->flags & UI_ScrollListFlag_Nav && cursor_out != 0 && ui_is_focus_active())
  {
    Vec2S64 cursor = *cursor_out;
    Vec2S64 mark = mark_out ? *mark_out : cursor;
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if((evt->delta_2s32.x == 0 && evt->delta_2s32.y == 0) ||
         evt->flags & UI_EventFlag_Delete)
      {
        continue;
      }
      ui_eat_event(evt);
      moved = 1;
      switch(evt->delta_unit)
      {
        default:{moved = 0;}break;
        case UI_EventDeltaUnit_Char:
        {
          for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis+1))
          {
            cursor.v[axis] += evt->delta_2s32.v[axis];
            if(cursor.v[axis] < params->cursor_range.min.v[axis])
            {
              cursor.v[axis] = params->cursor_range.max.v[axis];
            }
            if(cursor.v[axis] > params->cursor_range.max.v[axis])
            {
              cursor.v[axis] = params->cursor_range.min.v[axis];
            }
            cursor.v[axis] = clamp_1s64(r1s64(params->cursor_range.min.v[axis], params->cursor_range.max.v[axis]), cursor.v[axis]);
          }
        }break;
        case UI_EventDeltaUnit_Word:
        case UI_EventDeltaUnit_Line:
        case UI_EventDeltaUnit_Page:
        {
          cursor.x  = (evt->delta_2s32.x>0 ? params->cursor_range.max.x : evt->delta_2s32.x<0 ? params->cursor_range.min.x + !!params->cursor_min_is_empty_selection[Axis2_X] : cursor.x);
          cursor.y += ((evt->delta_2s32.y>0 ? +(num_possible_visible_rows-3) : evt->delta_2s32.y<0 ? -(num_possible_visible_rows-3) : 0));
          cursor.y = clamp_1s64(r1s64(params->cursor_range.min.y + !!params->cursor_min_is_empty_selection[Axis2_Y], params->cursor_range.max.y), cursor.y);
        }break;
        case UI_EventDeltaUnit_Whole:
        {
          for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis+1))
          {
            cursor.v[axis] = (evt->delta_2s32.v[axis]>0 ? params->cursor_range.max.v[axis] : evt->delta_2s32.v[axis]<0 ? params->cursor_range.min.v[axis] + !!params->cursor_min_is_empty_selection[axis] : cursor.v[axis]);
          }
        }break;
      }
      if(!(evt->flags & UI_EventFlag_KeepMark))
      {
        mark = cursor;
      }
    }
    if(moved)
    {
      *cursor_out = cursor;
      if(mark_out)
      {
        *mark_out = mark;
      }
    }
  }
  
  //- rjf: moved -> snap
  if(params->flags & UI_ScrollListFlag_Snap && moved)
  {
    S64 cursor_item_idx = cursor_out->y-1;
    if(params->item_range.min <= cursor_item_idx && cursor_item_idx <= params->item_range.max)
    {
      //- rjf: compute visible row range
      Rng1S64 visible_row_range = r1s64(scroll_pt->idx + 0 - !!(scroll_pt->off < 0),
                                        scroll_pt->idx + 0 + num_possible_visible_rows + 1);
      
      //- rjf: compute cursor row range from cursor item
      Rng1S64 cursor_visibility_row_range = {0};
      if(params->row_blocks.count == 0)
      {
        cursor_visibility_row_range = r1s64(cursor_item_idx-1, cursor_item_idx+3);
      }
      else
      {
        cursor_visibility_row_range.min = (S64)ui_scroll_list_row_from_item(&params->row_blocks, (U64)cursor_item_idx);
        cursor_visibility_row_range.max = cursor_visibility_row_range.min + 4;
      }
      
      //- rjf: compute deltas & apply
      S64 min_delta = Min(0, cursor_visibility_row_range.min-visible_row_range.min);
      S64 max_delta = Max(0, cursor_visibility_row_range.max-visible_row_range.max);
      S64 new_idx = scroll_pt->idx+min_delta+max_delta;
      new_idx = clamp_1s64(scroll_row_idx_range, new_idx);
      ui_scroll_pt_target_idx(scroll_pt, new_idx);
    }
  }
  
  //- rjf: output signal
  if(signal_out != 0)
  {
    signal_out->cursor_moved = moved;
  }
  
  //- rjf: determine ranges & limits
  Rng1S64 visible_row_range = r1s64(scroll_pt->idx + (S64)(scroll_pt->off) + 0 - !!(scroll_pt->off < 0),
                                    scroll_pt->idx + (S64)(scroll_pt->off) + 0 + num_possible_visible_rows + 1);
  visible_row_range.min = clamp_1s64(params->item_range, visible_row_range.min);
  visible_row_range.max = clamp_1s64(params->item_range, visible_row_range.max);
  *visible_row_range_out = visible_row_range;
  
  //- rjf: store thread-locals
  ui_scroll_list_scroll_bar_dim_px = ui_top_font_size()*1.5f;
  ui_scroll_list_scroll_pt_ptr = scroll_pt;
  ui_scroll_list_dim_px = params->dim_px;
  ui_scroll_list_scroll_idx_rng = scroll_row_idx_range;
  
  //- rjf: build top-level container
  UI_Box *container_box = &ui_nil_box;
  UI_FixedWidth(params->dim_px.x) UI_FixedHeight(params->dim_px.y) UI_ChildLayoutAxis(Axis2_X)
  {
    container_box = ui_build_box_from_key(0, ui_key_zero());
  }
  
  //- rjf: build scrollable container
  UI_Box *scrollable_container_box = &ui_nil_box;
  UI_Parent(container_box) UI_ChildLayoutAxis(Axis2_Y) UI_FixedWidth(params->dim_px.x-ui_scroll_list_scroll_bar_dim_px) UI_FixedHeight(params->dim_px.y)
  {
    scrollable_container_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_Scroll, "###sp");
    scrollable_container_box->view_off.y = scrollable_container_box->view_off_target.y = params->row_height_px*mod_f32(scroll_pt->off, 1.f) + params->row_height_px*(scroll_pt->off < 0) - params->row_height_px*(scroll_pt->off == -1.f && scroll_pt->idx == 1);
  }
  
  //- rjf: build vertical scroll bar
  UI_Parent(container_box) UI_Focus(UI_FocusKind_Null)
  {
    ui_set_next_fixed_width(ui_scroll_list_scroll_bar_dim_px);
    ui_set_next_fixed_height(ui_scroll_list_dim_px.y);
    *ui_scroll_list_scroll_pt_ptr = ui_scroll_bar(Axis2_Y,
                                                  ui_px(ui_scroll_list_scroll_bar_dim_px, 1.f),
                                                  *ui_scroll_list_scroll_pt_ptr,
                                                  scroll_row_idx_range,
                                                  num_possible_visible_rows);
  }
  
  //- rjf: begin scrollable region
  ui_push_parent(container_box);
  ui_push_parent(scrollable_container_box);
  ui_push_pref_height(ui_px(params->row_height_px, 1.f));
}

internal void
ui_scroll_list_end(void)
{
  ui_pop_pref_height();
  UI_Box *scrollable_container_box = ui_pop_parent();
  UI_Box *container_box = ui_pop_parent();
  
  //- rjf: scroll
  {
    UI_Signal sig = ui_signal_from_box(scrollable_container_box);
    if(sig.scroll.y != 0)
    {
      S64 new_idx = ui_scroll_list_scroll_pt_ptr->idx + sig.scroll.y;
      new_idx = clamp_1s64(ui_scroll_list_scroll_idx_rng, new_idx);
      ui_scroll_pt_target_idx(ui_scroll_list_scroll_pt_ptr, new_idx);
    }
    ui_scroll_pt_clamp_idx(ui_scroll_list_scroll_pt_ptr, ui_scroll_list_scroll_idx_rng);
  }
}
