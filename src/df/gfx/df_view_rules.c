// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: "default"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  ////////////////////////////
  //- rjf: unpack expression type info
  //
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = e_type_unwrap(irtree.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  
  ////////////////////////////
  //- rjf: do struct/union/class member block generation
  //
  if((type_kind == E_TypeKind_Struct ||
      type_kind == E_TypeKind_Union ||
      type_kind == E_TypeKind_Class) ||
     (e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                   direct_type_kind == E_TypeKind_Union ||
                                                   direct_type_kind == E_TypeKind_Class)))
  {
    // rjf: type -> filtered data members
    E_MemberArray data_members = e_type_data_members_from_key(arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    E_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(arena, data_members, cfg_table);
    
    // rjf: build blocks for all members, split by sub-expansions
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Members, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth);
    {
      last_vb->expr             = expr;
      last_vb->cfg_table        = cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, filtered_data_members.count);
      last_vb->members          = filtered_data_members;
    }
    for(DF_ExpandNode *child = expand_node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      E_Expr *child_expr = df_expr_from_eval_viz_block_index(arena, last_vb, child_idx);
      if(child_idx >= last_vb->semantic_idx_range.max)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = df_eval_viz_block_split_and_continue(arena, out, last_vb, child_idx);
      
      // rjf: build child config table
      DF_CfgTable *child_cfg_table = cfg_table;
      {
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, child->key);
        if(view_rule_string.size != 0)
        {
          child_cfg_table = push_array(arena, DF_CfgTable, 1);
          *child_cfg_table = df_cfg_table_from_inheritance(arena, cfg_table);
          df_cfg_table_push_unparsed_string(arena, child_cfg_table, view_rule_string, DF_CfgSrc_User);
        }
      }
      
      // rjf: recurse for child
      df_append_expr_eval_viz_blocks__rec(arena, eval_view, key, child->key, str8_zero(), child_expr, child_cfg_table, depth, out);
    }
    df_eval_viz_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do enum member block generation
  //
  // (just a single block for all enum members; enum members can never be expanded)
  //
  else if(type_kind == E_TypeKind_Enum ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Enum))
  {
    E_Type *type = e_type_from_key(arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_EnumMembers, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth);
    {
      last_vb->expr             = expr;
      last_vb->cfg_table        = cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, type->count);
      last_vb->enum_vals.v      = type->enum_vals;
      last_vb->enum_vals.count  = type->count;
    }
    df_eval_viz_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do array element block generation
  //
  else if(type_kind == E_TypeKind_Array ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Array))
  {
    // rjf: unpack array type info
    E_Type *array_type = e_type_from_key(scratch.arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    U64 array_count = array_type->count;
    B32 need_extra_deref = e_type_kind_is_pointer_or_ref(type_kind);
    
    // rjf: build blocks for all elements, split by sub-expansions
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Elements, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth);
    {
      last_vb->expr             = need_extra_deref ? e_expr_ref_deref(arena, expr) : expr;
      last_vb->cfg_table        = cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, array_count);
    }
    for(DF_ExpandNode *child = expand_node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      E_Expr *child_expr = df_expr_from_eval_viz_block_index(arena, last_vb, child_idx);
      if(child_idx >= last_vb->semantic_idx_range.max)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = df_eval_viz_block_split_and_continue(arena, out, last_vb, child_idx);
      
      // rjf: build child config table
      DF_CfgTable *child_cfg_table = cfg_table;
      {
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, child->key);
        if(view_rule_string.size != 0)
        {
          child_cfg_table = push_array(arena, DF_CfgTable, 1);
          *child_cfg_table = df_cfg_table_from_inheritance(arena, cfg_table);
          df_cfg_table_push_unparsed_string(arena, child_cfg_table, view_rule_string, DF_CfgSrc_User);
        }
      }
      
      // rjf: recurse for child
      df_append_expr_eval_viz_blocks__rec(arena, eval_view, key, child->key, str8_zero(), child_expr, child_cfg_table, depth, out);
    }
    df_eval_viz_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do pointer-to-pointer block generation
  //
  else if(e_type_kind_is_pointer_or_ref(type_kind) && e_type_kind_is_pointer_or_ref(direct_type_kind))
  {
    // rjf: compute key
    DF_ExpandKey child_key = df_expand_key_make(df_hash_from_expand_key(key), 1);
    
    // rjf: build child config table
    DF_CfgTable *child_cfg_table = cfg_table;
    {
      String8 view_rule_string = df_eval_view_rule_from_key(eval_view, child_key);
      if(view_rule_string.size != 0)
      {
        child_cfg_table = push_array(arena, DF_CfgTable, 1);
        *child_cfg_table = df_cfg_table_from_inheritance(arena, cfg_table);
        df_cfg_table_push_unparsed_string(arena, child_cfg_table, view_rule_string, DF_CfgSrc_User);
      }
    }
    
    // rjf: recurse for child
    E_Expr *child_expr = e_expr_ref_deref(arena, expr);
    df_append_expr_eval_viz_blocks__rec(arena, eval_view, key, child_key, str8_zero(), child_expr, child_cfg_table, depth, out);
  }
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "array"

DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(array)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  if(e_type_kind_is_pointer_or_ref(type_kind))
  {
    E_Value count = df_value_from_params(params);
    E_TypeKey element_type_key = e_type_ptee_from_key(type_key);
    E_TypeKey array_type_key = e_type_key_cons_array(element_type_key, count.u64);
    E_TypeKey ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key);
    expr = e_expr_ref_cast(arena, ptr_type_key, expr);
  }
  scratch_end(scratch);
  return expr;
}

////////////////////////////////
//~ rjf: "slice"

DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(slice)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKind type_kind = e_type_kind_from_key(irtree.type_key);
  if(type_kind == E_TypeKind_Struct || type_kind == E_TypeKind_Class)
  {
    // rjf: unpack members
    E_MemberArray members = e_type_data_members_from_key(scratch.arena, irtree.type_key);
    
    // rjf: choose base pointer & count members
    E_Member *base_ptr_member = 0;
    E_Member *count_member = 0;
    for(U64 idx = 0; idx < members.count; idx += 1)
    {
      E_Member *member = &members.v[idx];
      E_TypeKey member_type = e_type_unwrap(member->type_key);
      E_TypeKind member_type_kind = e_type_kind_from_key(member_type);
      if(count_member == 0 && e_type_kind_is_integer(member_type_kind))
      {
        count_member = member;
      }
      if(base_ptr_member == 0 && e_type_kind_is_pointer_or_ref(member_type_kind))
      {
        base_ptr_member = &members.v[idx];
      }
      if(count_member != 0 && base_ptr_member != 0)
      {
        break;
      }
    }
    
    // rjf: evaluate count member, determine count
    U64 count = 0;
    if(count_member != 0)
    {
      E_Expr *count_member_expr = e_expr_ref_member_access(scratch.arena, expr, count_member->name);
      E_Eval count_member_eval = e_eval_from_expr(scratch.arena, count_member_expr);
      E_Eval count_member_value_eval = e_value_eval_from_eval(count_member_eval);
      count = count_member_value_eval.value.u64;
    }
    
    // rjf: generate new struct slice type
    E_TypeKey slice_type_key = zero_struct;
    if(base_ptr_member != 0 && count_member != 0)
    {
      String8 struct_name = e_type_string_from_key(scratch.arena, irtree.type_key);
      E_TypeKey element_type_key = e_type_ptee_from_key(base_ptr_member->type_key);
      E_TypeKey array_type_key = e_type_key_cons_array(element_type_key, count);
      E_TypeKey sized_base_ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key);
      E_MemberList slice_type_members = {0};
      e_member_list_push(scratch.arena, &slice_type_members, count_member);
      e_member_list_push(scratch.arena, &slice_type_members, &(E_Member){.kind = E_MemberKind_DataField, .type_key = sized_base_ptr_type_key, .name = base_ptr_member->name, .off = base_ptr_member->off});
      E_MemberArray slice_type_members_array = e_member_array_from_list(scratch.arena, &slice_type_members);
      slice_type_key = e_type_key_cons(.arch = e_type_state->ctx->primary_module->arch,
                                       .kind = E_TypeKind_Struct,
                                       .name = struct_name,
                                       .members = slice_type_members_array.v,
                                       .count = slice_type_members_array.count);
    }
    
    // rjf: generate new expression tree - addr of struct, cast-to-ptr, deref
    if(base_ptr_member != 0 && count_member != 0)
    {
      expr = e_expr_ref_addr(arena, expr);
      expr = e_expr_ref_cast(arena, e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, slice_type_key), expr);
      expr = e_expr_ref_deref(arena, expr);
    }
  }
  scratch_end(scratch);
  return expr;
}

////////////////////////////////
//~ rjf: "list"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(list){}
DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(list){}

////////////////////////////////
//~ rjf: "bswap"

DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(bswap)
{
  expr = e_expr_ref_bswap(arena, expr);
  return expr;
}

////////////////////////////////
//~ rjf: "cast"

DF_CORE_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(cast)
{
  E_TypeKey type_key = df_type_key_from_params(params);
  expr = e_expr_ref_cast(arena, type_key, expr);
  return expr;
}

////////////////////////////////
//~ rjf: "dec"

DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(dec){}

////////////////////////////////
//~ rjf: "bin"

DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(bin){}

////////////////////////////////
//~ rjf: "oct"

DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(oct){}

////////////////////////////////
//~ rjf: "hex"

DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(hex){}

////////////////////////////////
//~ rjf: "only"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(only){}
DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(only){}
DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(only){}

////////////////////////////////
//~ rjf: "omit"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(omit){}
DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(omit){}
DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(omit){}

////////////////////////////////
//~ rjf: "no_addr"

DF_GFX_VIEW_RULE_LINE_STRINGIZE_FUNCTION_DEF(no_addr){}

////////////////////////////////
//~ rjf: "checkbox"

DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(checkbox)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_string(scratch.arena, string);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  if(ui_clicked(df_icon_buttonf(ws, value_eval.value.u64 == 0 ? DF_IconKind_CheckHollow : DF_IconKind_CheckFilled, 0, "###check")))
  {
    df_commit_eval_value_string(eval, value_eval.value.u64 == 0 ? str8_lit("1") : str8_lit("0"));
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "rgba"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(rgba)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(rgba)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_string(scratch.arena, string);
  Vec4F32 rgba = df_rgba_from_eval_params(eval, params);
  Vec4F32 hsva = hsva_from_rgba(rgba);
  
  //- rjf: build text box
  UI_Box *text_box = &ui_g_nil_box;
  UI_WidthFill DF_Font(ws, DF_FontSlot_Code)
  {
    text_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
    D_FancyStringList fancy_strings = {0};
    {
      D_FancyString open_paren = {ui_top_font(), str8_lit("("), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      D_FancyString comma = {ui_top_font(), str8_lit(", "), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      D_FancyString r_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.x), v4f32(1.f, 0.25f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString g_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.y), v4f32(0.25f, 1.f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString b_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.z), v4f32(0.25f, 0.25f, 1.f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString a_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.w), v4f32(1.f,   1.f,   1.f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString clse_paren = {ui_top_font(), str8_lit(")"), ui_top_palette()->text, ui_top_font_size(), 0, 0};
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &open_paren);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &r_fstr);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &g_fstr);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &b_fstr);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &comma);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &a_fstr);
      d_fancy_string_list_push(scratch.arena, &fancy_strings, &clse_paren);
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

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(text)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "disasm"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(disasm)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "memory"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(memory)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 16);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "graph"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(graph)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "bitmap"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(bitmap)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}

////////////////////////////////
//~ rjf: "geo3d"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(geo3d)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->string             = string;
  vb->expr               = expr;
  vb->visual_idx_range   = r1u64(0, 16);
  vb->semantic_idx_range = r1u64(0, 1);
  vb->cfg_table          = cfg_table;
  df_eval_viz_block_end(out, vb);
}
