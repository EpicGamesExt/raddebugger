// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: "list"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(list){}
DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(list){}

////////////////////////////////
//~ rjf: "dec"

DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(dec){}

////////////////////////////////
//~ rjf: "bin"

DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(bin){}

////////////////////////////////
//~ rjf: "oct"

DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(oct){}

////////////////////////////////
//~ rjf: "hex"

DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(hex){}

////////////////////////////////
//~ rjf: "only"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(only){}
DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(only){}
DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(only){}

////////////////////////////////
//~ rjf: "omit"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(omit){}
DF_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(omit){}
DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(omit){}

////////////////////////////////
//~ rjf: "no_addr"

DF_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(no_addr){}

////////////////////////////////
//~ rjf: "checkbox"

DF_VIEW_RULE_ROW_UI_FUNCTION_DEF(checkbox)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_string(scratch.arena, string);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  if(ui_clicked(df_icon_buttonf(value_eval.value.u64 == 0 ? DF_IconKind_CheckHollow : DF_IconKind_CheckFilled, 0, "###check")))
  {
    d_commit_eval_value_string(eval, value_eval.value.u64 == 0 ? str8_lit("1") : str8_lit("0"));
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "rgba"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(color_rgba)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

DF_VIEW_RULE_ROW_UI_FUNCTION_DEF(color_rgba)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_string(scratch.arena, string);
  Vec4F32 rgba = df_rgba_from_eval_params(eval, params);
  Vec4F32 hsva = hsva_from_rgba(rgba);
  
  //- rjf: build text box
  UI_Box *text_box = &ui_g_nil_box;
  UI_WidthFill DF_Font(DF_FontSlot_Code)
  {
    text_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
    DR_FancyStringList fancy_strings = {0};
    {
      DR_FancyString open_paren = {ui_top_font(), str8_lit("("), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      DR_FancyString comma = {ui_top_font(), str8_lit(", "), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      DR_FancyString r_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.x), v4f32(1.f, 0.25f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      DR_FancyString g_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.y), v4f32(0.25f, 1.f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      DR_FancyString b_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.z), v4f32(0.25f, 0.25f, 1.f, 1.f), ui_top_font_size(), 4.f, 0};
      DR_FancyString a_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.w), v4f32(1.f,   1.f,   1.f, 1.f), ui_top_font_size(), 4.f, 0};
      DR_FancyString clse_paren = {ui_top_font(), str8_lit(")"), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &open_paren);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &r_fstr);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &g_fstr);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &b_fstr);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &a_fstr);
      dr_fancy_string_list_push(scratch.arena, &fancy_strings, &clse_paren);
    }
    ui_box_equip_display_fancy_strings(text_box, &fancy_strings);
  }
  
  //- rjf: build color box
  UI_Box *color_box = &ui_g_nil_box;
  UI_PrefWidth(ui_em(1.875f, 1.f)) UI_ChildLayoutAxis(Axis2_Y)
  {
    color_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "color_box");
    UI_Parent(color_box) UI_PrefHeight(ui_em(1.875f, 1.f)) UI_Padding(ui_pct(1, 0))
    {
      UI_Palette(ui_build_palette(ui_top_palette(), .background = rgba)) UI_CornerRadius(ui_top_font_size()*0.5f)
        ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
    }
  }
  
  //- rjf: space
  ui_spacer(ui_em(0.375f, 1.f));
  
  //- rjf: hover color box -> show components
  UI_Signal sig = ui_signal_from_box(color_box);
  if(ui_hovering(sig))
  {
    ui_do_color_tooltip_hsva(hsva);
  }
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "text"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(text)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "disasm"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(disasm)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "memory"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(memory)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 16);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "graph"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(graph)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "bitmap"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(bitmap)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "geo3d"

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(geo3d)
{
  EV_Block *vb = ev_block_begin(arena, EV_BlockKind_Canvas, key, ev_key_make(ev_hash_from_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 16);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->view_rules         = view_rules;
  ev_block_end(out, vb);
}
