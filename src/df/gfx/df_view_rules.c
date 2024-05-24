// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: "array"

DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_DEF(array)
{
  Temp scratch = scratch_begin(&arena, 1);
  TG_Key type_key = eval.type_key;
  TG_Kind type_kind = tg_kind_from_key(type_key);
  if(type_kind == TG_Kind_Ptr || type_kind == TG_Kind_LRef || type_kind == TG_Kind_RRef)
  {
    DF_CfgNode *array_node = val->last;
    if(array_node != &df_g_nil_cfg_node)
    {
      // rjf: determine array size
      U64 array_size = 0;
      {
        String8List array_size_expr_strs = {0};
        for(DF_CfgNode *child = array_node->first; child != &df_g_nil_cfg_node; child = child->next)
        {
          str8_list_push(scratch.arena, &array_size_expr_strs, child->string);
        }
        String8 array_size_expr = str8_list_join(scratch.arena, &array_size_expr_strs, 0);
        DF_Eval array_size_eval = df_eval_from_string(arena, di_scope, ctrl_ctx, parse_ctx, macro_map, array_size_expr);
        DF_Eval array_size_eval_value = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, array_size_eval);
        eval_error_list_concat_in_place(&eval.errors, &array_size_eval.errors);
        array_size = array_size_eval_value.imm_u64;
      }
      
      // rjf: apply array size to type
      TG_Key pointee = tg_ptee_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, type_key);
      TG_Key array_type = tg_cons_type_make(parse_ctx->type_graph, TG_Kind_Array, pointee, array_size);
      eval.type_key = tg_cons_type_make(parse_ctx->type_graph, TG_Kind_Ptr, array_type, 0);
    }
  }
  scratch_end(scratch);
  return eval;
}

////////////////////////////////
//~ rjf: "list"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(list){}
DF_GFX_VIEW_RULE_VIZ_ROW_PROD_FUNCTION_DEF(list){}

////////////////////////////////
//~ rjf: "bswap"

DF_CORE_VIEW_RULE_EVAL_RESOLUTION_FUNCTION_DEF(bswap)
{
  Temp scratch = scratch_begin(&arena, 1);
  TG_Key type_key = eval.type_key;
  TG_Kind type_kind = tg_kind_from_key(type_key);
  U64 type_size_bytes = tg_byte_size_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, type_key);
  if(TG_Kind_Char8 <= type_kind && type_kind <= TG_Kind_S256 &&
     (type_size_bytes == 2 ||
      type_size_bytes == 4 ||
      type_size_bytes == 8))
  {
    DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
    if(value_eval.mode == EVAL_EvalMode_Value)
    {
      switch(type_size_bytes)
      {
        default:{}break;
        case 2:{U16 v = (U16)value_eval.imm_u64; v = bswap_u16(v); value_eval.imm_u64 = (U64)v;}break;
        case 4:{U32 v = (U32)value_eval.imm_u64; v = bswap_u32(v); value_eval.imm_u64 = (U64)v;}break;
        case 8:{U64 v =      value_eval.imm_u64; v = bswap_u64(v); value_eval.imm_u64 =      v;}break;
      }
    }
    eval = value_eval;
  }
  scratch_end(scratch);
  return eval;
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
//~ rjf: "rgba"

internal Vec4F32
df_vr_rgba_from_eval(DF_Eval eval, TG_Graph *graph, RDI_Parsed *raddbg, DF_Entity *process)
{
  Vec4F32 rgba = {0};
  Temp scratch = scratch_begin(0, 0);
  TG_Key type_key = eval.type_key;
  TG_Kind type_kind = tg_kind_from_key(type_key);
  switch(type_kind)
  {
    default:{}break;
    
    // rjf: extract r/g/b/a bytes from u32
    case TG_Kind_U32:
    case TG_Kind_S32:
    {
      U32 hex_val = (U32)eval.imm_u64;
      rgba = rgba_from_u32(hex_val);
    }break;
    
    // rjf: extract r/g/b/a values from array
    case TG_Kind_Array:
    if(eval.mode == EVAL_EvalMode_Addr)
    {
      U64 array_total_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, type_key);
      U64 array_total_size_capped = ClampTop(array_total_size, 64);
      Rng1U64 array_memory_vaddr_rng = r1u64(eval.offset, eval.offset + array_total_size_capped);
      CTRL_ProcessMemorySlice array_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, array_memory_vaddr_rng, 0);
      String8 array_memory = array_slice.data;
      TG_Key element_type_key = tg_unwrapped_direct_from_graph_rdi_key(graph, raddbg, type_key);
      TG_Kind element_type_kind = tg_kind_from_key(element_type_key);
      U64 element_type_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, element_type_key);
      for(U64 element_idx = 0; element_idx < 4; element_idx += 1)
      {
        U64 offset = element_idx*element_type_size;
        if(offset >= array_memory.size)
        {
          break;
        }
        switch(element_type_kind)
        {
          default:{}break;
          case TG_Kind_U8:
          {
            U8 byte = array_memory.str[offset];
            rgba.v[element_idx] = byte/255.f;
          }break;
          case TG_Kind_F32:
          {
            rgba.v[element_idx] = *(F32 *)(array_memory.str+offset);
          }break;
        }
      }
    }break;
    
    // rjf: extract r/g/b/a values from struct
    case TG_Kind_Struct:
    case TG_Kind_Class:
    case TG_Kind_Union:
    if(eval.mode == EVAL_EvalMode_Addr)
    {
      U64 struct_total_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, type_key);
      U64 struct_total_size_capped = ClampTop(struct_total_size, 64);
      Rng1U64 struct_memory_vaddr_rng = r1u64(eval.offset, eval.offset + struct_total_size_capped);
      CTRL_ProcessMemorySlice struct_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, struct_memory_vaddr_rng, 0);
      String8 struct_memory = struct_slice.data;
      TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, graph, raddbg, type_key);
      for(U64 element_idx = 0, member_idx = 0;
          element_idx < 4 && member_idx < type->count;
          member_idx += 1)
      {
        TG_Member *member = &type->members[member_idx];
        TG_Key member_type_key = member->type_key;
        TG_Kind member_type_kind = tg_kind_from_key(member_type_key);
        B32 member_is_component = 1;
        switch(member_type_kind)
        {
          default:{member_is_component = 0;}break;
          case TG_Kind_U8:
          {
            rgba.v[element_idx] = struct_memory.str[member->off]/255.f;
          }break;
          case TG_Kind_F32:
          {
            rgba.v[element_idx] = *(F32 *)(struct_memory.str + member->off);
          }break;
        }
        if(member_is_component)
        {
          element_idx += 1;
        }
      }
    }break;
  }
  scratch_end(scratch);
  return rgba;
}

internal void
df_vr_eval_commit_rgba(DF_Eval eval, TG_Graph *graph, RDI_Parsed *raddbg, DF_CtrlCtx *ctrl_ctx, Vec4F32 rgba)
{
  TG_Key type_key = eval.type_key;
  TG_Kind type_kind = tg_kind_from_key(type_key);
  switch(type_kind)
  {
    default:{}break;
    
    // rjf: extract r/g/b/a bytes from u32
    case TG_Kind_U32:
    case TG_Kind_S32:
    {
      U32 val = u32_from_rgba(rgba);
      DF_Eval src_eval = eval;
      src_eval.mode = EVAL_EvalMode_Value;
      src_eval.imm_u64 = (U64)val;
      df_commit_eval_value(graph, raddbg, ctrl_ctx, eval, src_eval);
    }break;
    
#if 0
    // rjf: extract r/g/b/a values from array
    case TG_Kind_Array:
    if(eval.mode == EVAL_EvalMode_Addr)
    {
      U64 array_total_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, type_key);
      U64 array_total_size_capped = ClampTop(array_total_size, 64);
      Rng1U64 array_memory_vaddr_rng = r1u64(eval.offset, eval.offset + array_total_size_capped);
      String8 array_memory = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, array_memory_vaddr_rng);
      TG_Key element_type_key = tg_unwrapped_direct_from_graph_rdi_key(graph, raddbg, type_key);
      TG_Kind element_type_kind = tg_kind_from_key(element_type_key);
      U64 element_type_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, element_type_key);
      for(U64 element_idx = 0; element_idx < 4; element_idx += 1)
      {
        U64 offset = element_idx*element_type_size;
        if(offset >= array_memory.size)
        {
          break;
        }
        switch(element_type_kind)
        {
          default:{}break;
          case TG_Kind_U8:
          {
            U8 byte = array_memory.str[offset];
            rgba.v[element_idx] = byte/255.f;
          }break;
          case TG_Kind_F32:
          {
            rgba.v[element_idx] = *(F32 *)(array_memory.str+offset);
          }break;
        }
      }
    }break;
    
    // rjf: extract r/g/b/a values from struct
    case TG_Kind_Struct:
    case TG_Kind_Class:
    case TG_Kind_Union:
    if(eval.mode == EVAL_EvalMode_Addr)
    {
      U64 struct_total_size = tg_byte_size_from_graph_rdi_key(graph, raddbg, type_key);
      U64 struct_total_size_capped = ClampTop(struct_total_size, 64);
      Rng1U64 struct_memory_vaddr_rng = r1u64(eval.offset, eval.offset + struct_total_size_capped);
      String8 struct_memory = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, struct_memory_vaddr_rng);
      TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, graph, raddbg, type_key);
      for(U64 element_idx = 0, member_idx = 0;
          element_idx < 4 && member_idx < type->count;
          member_idx += 1)
      {
        TG_Member *member = &type->members[member_idx];
        TG_Key member_type_key = member->type_key;
        TG_Kind member_type_kind = tg_kind_from_key(member_type_key);
        B32 member_is_component = 1;
        switch(member_type_kind)
        {
          default:{member_is_component = 0;}break;
          case TG_Kind_U8:
          {
            rgba.v[element_idx] = struct_memory.str[member->off]/255.f;
          }break;
          case TG_Kind_F32:
          {
            rgba.v[element_idx] = *(F32 *)(struct_memory.str + member->off);
          }break;
        }
        if(member_is_component)
        {
          element_idx += 1;
        }
      }
    }break;
#endif
  }
}

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(rgba)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(rgba)
{
  Temp scratch = scratch_begin(0, 0);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  
  //- rjf: grab hsva
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  Vec4F32 rgba = df_vr_rgba_from_eval(value_eval, parse_ctx->type_graph, parse_ctx->rdi, process);
  Vec4F32 hsva = hsva_from_rgba(rgba);
  
  //- rjf: build text box
  UI_Box *text_box = &ui_g_nil_box;
  UI_WidthFill UI_Font(df_font_from_slot(DF_FontSlot_Code))
  {
    text_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
    D_FancyStringList fancy_strings = {0};
    {
      D_FancyString open_paren = {ui_top_font(), str8_lit("("), ui_top_text_color(), ui_top_font_size(), 0, 0};
      D_FancyString comma = {ui_top_font(), str8_lit(", "), ui_top_text_color(), ui_top_font_size(), 0, 0};
      D_FancyString r_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.x), v4f32(1.f, 0.25f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString g_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.y), v4f32(0.25f, 1.f, 0.25f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString b_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.z), v4f32(0.25f, 0.25f, 1.f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString a_fstr = {ui_top_font(), push_str8f(scratch.arena, "%.2f", rgba.w), v4f32(1.f,   1.f,   1.f, 1.f), ui_top_font_size(), 4.f, 0};
      D_FancyString clse_paren = {ui_top_font(), str8_lit(")"), ui_top_text_color(), ui_top_font_size(), 0, 0};
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
    ui_box_equip_display_fancy_strings(text_box, 0, &fancy_strings);
  }
  
  //- rjf: build color box
  UI_Box *color_box = &ui_g_nil_box;
  UI_PrefWidth(ui_em(1.875f, 1.f)) UI_ChildLayoutAxis(Axis2_Y)
  {
    color_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "color_box");
    UI_Parent(color_box) UI_PrefHeight(ui_em(1.875f, 1.f)) UI_Padding(ui_pct(1, 0))
    {
      UI_BackgroundColor(rgba) UI_CornerRadius(ui_top_font_size()*0.5f)
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

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(rgba)
{
  Temp scratch = scratch_begin(0, 0);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_VR_RGBAState *state = df_view_rule_block_user_state(key, DF_VR_RGBAState);
  
  //- rjf: grab hsva
  Vec4F32 rgba = {0};
  Vec4F32 hsva = {0};
  {
    if(state->memgen_idx >= ctrl_mem_gen())
    {
      hsva = state->hsva;
      rgba = rgba_from_hsva(hsva);
    }
    else
    {
      DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
      rgba = df_vr_rgba_from_eval(value_eval, parse_ctx->type_graph, parse_ctx->rdi, process);
      state->hsva = hsva = hsva_from_rgba(rgba);
      state->memgen_idx = ctrl_mem_gen();
    }
  }
  Vec4F32 initial_hsva = hsva;
  
  //- rjf: build color picker
  B32 commit = 0;
  UI_Padding(ui_pct(1.f, 0.f))
  {
    UI_PrefWidth(ui_px(dim.y, 1.f))
    {
      UI_Signal sv_sig = ui_sat_val_pickerf(hsva.x, &hsva.y, &hsva.z, "sat_val_picker");
      commit = commit || ui_released(sv_sig);
    }
    UI_PrefWidth(ui_em(3.f, 1.f))
    {
      UI_Signal h_sig  = ui_hue_pickerf(&hsva.x, hsva.y, hsva.z, "hue_picker");
      commit = commit || ui_released(h_sig);
    }
    UI_PrefWidth(ui_children_sum(1)) UI_Column UI_PrefWidth(ui_text_dim(10, 1)) UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      ui_labelf("Hex");
      ui_labelf("R");
      ui_labelf("G");
      ui_labelf("B");
      ui_labelf("H");
      ui_labelf("S");
      ui_labelf("V");
      ui_labelf("A");
    }
    UI_PrefWidth(ui_children_sum(1)) UI_Column UI_PrefWidth(ui_text_dim(10, 1)) UI_Font(df_font_from_slot(DF_FontSlot_Code))
    {
      String8 hex_string = hex_string_from_rgba_4f32(scratch.arena, rgba);
      ui_label(hex_string);
      ui_labelf("%.2f", rgba.x);
      ui_labelf("%.2f", rgba.y);
      ui_labelf("%.2f", rgba.z);
      ui_labelf("%.2f", hsva.x);
      ui_labelf("%.2f", hsva.y);
      ui_labelf("%.2f", hsva.z);
      ui_labelf("%.2f", rgba.w);
    }
  }
  
  //- rjf: commit edited hsva back
  if(commit)
  {
    Vec4F32 rgba = rgba_from_hsva(hsva);
    df_vr_eval_commit_rgba(eval, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, rgba);
    state->memgen_idx = ctrl_mem_gen();
  }
  
  //- rjf: commit possible edited value to state
  state->hsva = hsva;
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "text"

internal DF_TxtTopologyInfo
df_vr_txt_topology_info_from_cfg(DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  DF_TxtTopologyInfo result = zero_struct;
  {
    StringJoin join = {0};
    join.sep = str8_lit(" ");
    DF_CfgNode *size_cfg = df_cfg_node_child_from_string(cfg, str8_lit("size"), 0);
    DF_CfgNode *lang_cfg = df_cfg_node_child_from_string(cfg, str8_lit("lang"), 0);
    String8List size_expr_strs = {0};
    String8 lang_string = {0};
    for(DF_CfgNode *child = size_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &size_expr_strs, child->string);
    }
    lang_string = lang_cfg->first->string;
    String8 size_expr = str8_list_join(scratch.arena, &size_expr_strs, &join);
    DF_Eval size_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, size_expr);
    DF_Eval size_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, size_eval);
    result.lang = txt_lang_kind_from_extension(lang_string);
    result.size_cap = size_val_eval.imm_u64;
  }
  scratch_end(scratch);
  return result;
}

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(text)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(text)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  
  //////////////////////////////
  //- rjf: get & initialize state
  //
  DF_VR_TextState *state = df_view_rule_block_user_state(key, DF_VR_TextState);
  if(!state->initialized)
  {
    state->initialized = 1;
    state->cursor = state->mark = txt_pt(1, 1);
  }
  
  //////////////////////////////
  //- rjf: unpack evaluation / view rule params
  //
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_TxtTopologyInfo top = df_vr_txt_topology_info_from_cfg(di_scope, ctrl_ctx, parse_ctx, macro_map, cfg);
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  Rng1U64 vaddr_range = r1u64(base_vaddr, base_vaddr + (top.size_cap ? top.size_cap : 2048));
  
  //////////////////////////////
  //- rjf: evaluation info -> text visualization info
  //
  String8 data = {0};
  TXT_TextInfo info = {0};
  TXT_LineTokensSlice line_tokens_slice = {0};
  {
    U128 text_hash = {0};
    U128 text_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, vaddr_range, 1);
    info = txt_text_info_from_key_lang(txt_scope, text_key, top.lang, &text_hash);
    data = hs_data_from_hash(hs_scope, text_hash);
    line_tokens_slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, &info, data, r1s64(1, info.lines_count));
  }
  
  //////////////////////////////
  //- rjf: info -> code slice info
  //
  DF_CodeSliceParams code_slice_params = {0};
  {
    code_slice_params.flags = DF_CodeSliceFlag_LineNums;
    code_slice_params.line_num_range = r1s64(1, info.lines_count);
    code_slice_params.line_text = push_array(scratch.arena, String8, info.lines_count);
    code_slice_params.line_ranges = push_array(scratch.arena, Rng1U64, info.lines_count);
    code_slice_params.line_tokens = push_array(scratch.arena, TXT_TokenArray, info.lines_count);
    code_slice_params.line_bps = push_array(scratch.arena, DF_EntityList, info.lines_count);
    code_slice_params.line_ips = push_array(scratch.arena, DF_EntityList, info.lines_count);
    code_slice_params.line_pins = push_array(scratch.arena, DF_EntityList, info.lines_count);
    code_slice_params.line_dasm2src = push_array(scratch.arena, DF_TextLineDasm2SrcInfoList, info.lines_count);
    code_slice_params.line_src2dasm = push_array(scratch.arena, DF_TextLineSrc2DasmInfoList, info.lines_count);
    for(U64 line_idx = 0; line_idx < info.lines_count; line_idx += 1)
    {
      code_slice_params.line_text[line_idx] = str8_substr(data, info.lines_ranges[line_idx]);
      code_slice_params.line_ranges[line_idx] = info.lines_ranges[line_idx];
      code_slice_params.line_tokens[line_idx] = line_tokens_slice.line_tokens[line_idx];
    }
    code_slice_params.font = df_font_from_slot(DF_FontSlot_Code);
    code_slice_params.font_size = ui_top_font_size();
    code_slice_params.tab_size = f_column_size_from_tag_size(code_slice_params.font, code_slice_params.font_size)*4.f;
    code_slice_params.line_height_px = ui_top_font_size()*1.5f;
    code_slice_params.margin_width_px = 0;
    code_slice_params.line_num_width_px = ui_top_font_size()*5.f;
    code_slice_params.line_text_max_width_px = ui_top_font_size()*2.f*info.lines_max_size;
  }
  
  //////////////////////////////
  //- rjf: build UI
  //
  if(info.lines_count != 0)
  {
    //- rjf: build top-level container
    UI_Box *container = &ui_g_nil_box;
    UI_PrefWidth(ui_px(dim.x, 1.f)) UI_PrefHeight(ui_px(dim.y, 1.f))
    {
      container = ui_build_box_from_stringf(UI_BoxFlag_AllowOverflow|UI_BoxFlag_Clip, "###text_container");
    }
    
    //- rjf: build code slice
    UI_WidthFill UI_HeightFill UI_Parent(container)
    {
      DF_CodeSliceSignal slice_sig = df_code_slice(ws, ctrl_ctx, parse_ctx, &code_slice_params, &state->cursor, &state->mark, &state->preferred_column, str8_lit("###slice"));
    }
  }
  
  txt_scope_close(txt_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}

DF_VIEW_SETUP_FUNCTION_DEF(text) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(text) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(text) {}
DF_VIEW_UI_FUNCTION_DEF(text)
{
}

////////////////////////////////
//~ rjf: "disasm"

internal DF_DisasmTopologyInfo
df_vr_disasm_topology_info_from_cfg(DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  DF_DisasmTopologyInfo result = zero_struct;
  {
    StringJoin join = {0};
    join.sep = str8_lit(" ");
    DF_CfgNode *size_cfg = df_cfg_node_child_from_string(cfg, str8_lit("size"), 0);
    DF_CfgNode *arch_cfg = df_cfg_node_child_from_string(cfg, str8_lit("arch"), 0);
    String8List size_expr_strs = {0};
    String8 arch_string = {0};
    for(DF_CfgNode *child = size_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &size_expr_strs, child->string);
    }
    arch_string = arch_cfg->first->string;
    String8 size_expr = str8_list_join(scratch.arena, &size_expr_strs, &join);
    DF_Eval size_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, size_expr);
    DF_Eval size_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, size_eval);
    if(str8_match(arch_string, str8_lit("x64"), StringMatchFlag_CaseInsensitive))
    {
      result.arch = Architecture_x64;
    }
    result.size_cap = size_val_eval.imm_u64;
  }
  scratch_end(scratch);
  return result;
}

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(disasm)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(disasm)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  DASM_Scope *dasm_scope = dasm_scope_open();
  DF_VR_DisasmState *state = df_view_rule_block_user_state(key, DF_VR_DisasmState);
  if(!state->initialized)
  {
    state->initialized = 1;
    state->cursor = state->mark = txt_pt(1, 1);
  }
  if(state->last_open_frame_idx+1 < df_frame_index())
  {
    state->loaded_t = 0;
  }
  state->last_open_frame_idx = df_frame_index();
  {
    //- rjf: unpack params
    DF_DisasmTopologyInfo top = df_vr_disasm_topology_info_from_cfg(di_scope, ctrl_ctx, parse_ctx, macro_map, cfg);
    
    //- rjf: resolve to address value & range
    DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
    U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
    Rng1U64 vaddr_range = r1u64(base_vaddr, base_vaddr + (top.size_cap ? top.size_cap : 2048));
    
    //- rjf: unpack thread/process of eval
    DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
    DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
    
    //- rjf: unpack key for this region in memory
    U128 dasm_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, vaddr_range, 0);
    
    //- rjf: key -> parsed text info
    U128 data_hash = {0};
    DASM_Params dasm_params = {0};
    {
      dasm_params.vaddr = vaddr_range.min;
      dasm_params.arch = top.arch;
      dasm_params.style_flags = DASM_StyleFlag_Addresses;
      dasm_params.syntax = DASM_Syntax_Intel;
      dasm_params.base_vaddr = 0;
    }
    DASM_Info dasm_info = dasm_info_from_key_params(dasm_scope, dasm_key, &dasm_params, &data_hash);
    String8 dasm_text_data = {0};
    TXT_TextInfo dasm_text_info = {0};
    for(U64 rewind_idx = 0; rewind_idx < 2; rewind_idx += 1)
    {
      U128 dasm_text_hash = hs_hash_from_key(dasm_info.text_key, rewind_idx);
      dasm_text_data = hs_data_from_hash(hs_scope, dasm_text_hash);
      dasm_text_info = txt_text_info_from_hash_lang(txt_scope, dasm_text_hash, TXT_LangKind_DisasmX64Intel);
      if(dasm_text_info.lines_count != 0)
      {
        break;
      }
    }
    TXT_LineTokensSlice line_tokens_slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, &dasm_text_info, dasm_text_data, r1s64(1, dasm_info.insts.count));
    
    //- rjf: info -> code slice info
    DF_CodeSliceParams code_slice_params = {0};
    {
      code_slice_params.flags = DF_CodeSliceFlag_LineNums;
      code_slice_params.line_num_range = r1s64(1, dasm_text_info.lines_count);
      code_slice_params.line_text = push_array(scratch.arena, String8, dasm_text_info.lines_count);
      code_slice_params.line_ranges = push_array(scratch.arena, Rng1U64, dasm_text_info.lines_count);
      code_slice_params.line_tokens = push_array(scratch.arena, TXT_TokenArray, dasm_text_info.lines_count);
      code_slice_params.line_bps = push_array(scratch.arena, DF_EntityList, dasm_text_info.lines_count);
      code_slice_params.line_ips = push_array(scratch.arena, DF_EntityList, dasm_text_info.lines_count);
      code_slice_params.line_pins = push_array(scratch.arena, DF_EntityList, dasm_text_info.lines_count);
      code_slice_params.line_dasm2src = push_array(scratch.arena, DF_TextLineDasm2SrcInfoList, dasm_text_info.lines_count);
      code_slice_params.line_src2dasm = push_array(scratch.arena, DF_TextLineSrc2DasmInfoList, dasm_text_info.lines_count);
      for(U64 line_idx = 0; line_idx < dasm_text_info.lines_count; line_idx += 1)
      {
        code_slice_params.line_text[line_idx] = str8_substr(dasm_text_data, dasm_info.insts.v[line_idx].text_range);
        code_slice_params.line_ranges[line_idx] = dasm_info.insts.v[line_idx].text_range;
        code_slice_params.line_tokens[line_idx] = line_tokens_slice.line_tokens[line_idx];
      }
      code_slice_params.font = df_font_from_slot(DF_FontSlot_Code);
      code_slice_params.font_size = ui_top_font_size();
      code_slice_params.tab_size = f_column_size_from_tag_size(code_slice_params.font, code_slice_params.font_size)*4.f;
      code_slice_params.line_height_px = ui_top_font_size()*1.5f;
      code_slice_params.margin_width_px = 0;
      code_slice_params.line_num_width_px = ui_top_font_size()*5.f;
      code_slice_params.line_text_max_width_px = ui_top_font_size()*2.f*dasm_text_info.lines_max_size;
    }
    
    //- rjf: build code slice
    if(dasm_info.insts.count != 0 && dasm_text_info.lines_count != 0)
      UI_Padding(ui_pct(1, 0)) UI_PrefWidth(ui_px(dasm_text_info.lines_max_size*ui_top_font_size()*1.2f, 1.f)) UI_Column UI_Padding(ui_pct(1, 0))
    {
      DF_CodeSliceSignal sig = df_code_slice(ws, ctrl_ctx, parse_ctx, &code_slice_params, &state->cursor, &state->mark, &state->preferred_column, str8_lit("###code_slice"));
    }
  }
  dasm_scope_close(dasm_scope);
  txt_scope_close(txt_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}

DF_VIEW_SETUP_FUNCTION_DEF(disasm) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(disasm) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(disasm) {}
DF_VIEW_UI_FUNCTION_DEF(disasm)
{
}

////////////////////////////////
//~ rjf: "graph"

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(graph)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(graph)
{
}

DF_VIEW_SETUP_FUNCTION_DEF(graph) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(graph) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(graph) {}
DF_VIEW_UI_FUNCTION_DEF(graph)
{
}

////////////////////////////////
//~ rjf: "bitmap"

internal Vec2F32
df_bitmap_view_state__screen_from_canvas_pos(DF_BitmapViewState *bvs, Rng2F32 rect, Vec2F32 cvs)
{
  Vec2F32 scr =
  {
    (rect.x0+rect.x1)/2 + (cvs.x - bvs->view_center_pos.x) * bvs->zoom,
    (rect.y0+rect.y1)/2 + (cvs.y - bvs->view_center_pos.y) * bvs->zoom,
  };
  return scr;
}

internal Rng2F32
df_bitmap_view_state__screen_from_canvas_rect(DF_BitmapViewState *bvs, Rng2F32 rect, Rng2F32 cvs)
{
  Rng2F32 scr = r2f32(df_bitmap_view_state__screen_from_canvas_pos(bvs, rect, cvs.p0), df_bitmap_view_state__screen_from_canvas_pos(bvs, rect, cvs.p1));
  return scr;
}

internal Vec2F32
df_bitmap_view_state__canvas_from_screen_pos(DF_BitmapViewState *bvs, Rng2F32 rect, Vec2F32 scr)
{
  Vec2F32 cvs =
  {
    (scr.x - (rect.x0+rect.x1)/2) / bvs->zoom + bvs->view_center_pos.x,
    (scr.y - (rect.y0+rect.y1)/2) / bvs->zoom + bvs->view_center_pos.y,
  };
  return cvs;
}

internal Rng2F32
df_bitmap_view_state__canvas_from_screen_rect(DF_BitmapViewState *bvs, Rng2F32 rect, Rng2F32 scr)
{
  Rng2F32 cvs = r2f32(df_bitmap_view_state__canvas_from_screen_pos(bvs, rect, scr.p0), df_bitmap_view_state__canvas_from_screen_pos(bvs, rect, scr.p1));
  return cvs;
}

internal DF_BitmapTopologyInfo
df_vr_bitmap_topology_info_from_cfg(DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  DF_BitmapTopologyInfo info = {0};
  {
    info.fmt = R_Tex2DFormat_RGBA8;
  }
  {
    DF_CfgNode *width_cfg  = df_cfg_node_child_from_string(cfg, str8_lit("w"), 0);
    DF_CfgNode *height_cfg = df_cfg_node_child_from_string(cfg, str8_lit("h"), 0);
    DF_CfgNode *fmt_cfg  = df_cfg_node_child_from_string(cfg, str8_lit("fmt"), 0);
    String8List width_expr_strs = {0};
    String8List height_expr_strs = {0};
    for(DF_CfgNode *child = width_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &width_expr_strs, child->string);
    }
    for(DF_CfgNode *child = height_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &height_expr_strs, child->string);
    }
    String8 width_expr = str8_list_join(scratch.arena, &width_expr_strs, 0);
    String8 height_expr = str8_list_join(scratch.arena, &height_expr_strs, 0);
    String8 fmt_string = fmt_cfg->first->string;
    DF_Eval width_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, width_expr);
    DF_Eval width_eval_value = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, width_eval);
    info.width = width_eval_value.imm_u64;
    DF_Eval height_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, height_expr);
    DF_Eval height_eval_value = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, height_eval);
    info.height = height_eval_value.imm_u64;
    if(fmt_string.size != 0)
    {
      for(R_Tex2DFormat fmt = (R_Tex2DFormat)0; fmt < R_Tex2DFormat_COUNT; fmt = (R_Tex2DFormat)(fmt+1))
      {
        if(str8_match(r_tex2d_format_display_string_table[fmt], fmt_string, StringMatchFlag_CaseInsensitive))
        {
          info.fmt = fmt;
          break;
        }
      }
    }
  }
  scratch_end(scratch);
  return info;
}

internal UI_BOX_CUSTOM_DRAW(df_vr_bitmap_box_draw)
{
  DF_VR_BitmapBoxDrawData *draw_data = (DF_VR_BitmapBoxDrawData *)user_data;
  Vec4F32 bg_color = box->background_color;
  d_img(box->rect, draw_data->src, draw_data->texture, v4f32(1, 1, 1, 1), 0, 0, 0);
  if(draw_data->loaded_t < 0.98f)
  {
    Rng2F32 clip = box->rect;
    for(UI_Box *b = box->parent; !ui_box_is_nil(b); b = b->parent)
    {
      if(b->flags & UI_BoxFlag_Clip)
      {
        clip = intersect_2f32(b->rect, clip);
      }
    }
    d_blur(intersect_2f32(clip, box->rect), 10.f-9.f*draw_data->loaded_t, 0);
  }
  if(r_handle_match(draw_data->texture, r_handle_zero()))
  {
    d_rect(box->rect, v4f32(0, 0, 0, 1), 0, 0, 0);
  }
  d_rect(box->rect, v4f32(bg_color.x*bg_color.w, bg_color.y*bg_color.w, bg_color.z*bg_color.w, 1.f-draw_data->loaded_t), 0, 0, 0);
  if(draw_data->hovered)
  {
    Vec4F32 indicator_color = df_rgba_from_theme_color(DF_ThemeColor_PlainBorder);
    indicator_color.w = 1.f;
    d_rect(pad_2f32(r2f32p(box->rect.x0 + draw_data->mouse_px.x*draw_data->ui_per_bmp_px,
                           box->rect.y0 + draw_data->mouse_px.y*draw_data->ui_per_bmp_px,
                           box->rect.x0 + draw_data->mouse_px.x*draw_data->ui_per_bmp_px + draw_data->ui_per_bmp_px,
                           box->rect.y0 + draw_data->mouse_px.y*draw_data->ui_per_bmp_px + draw_data->ui_per_bmp_px),
                    3.f),
           indicator_color, 3.f, 4.f, 1.f);
  }
}

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(bitmap)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 8);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(bitmap)
{
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  DF_BitmapTopologyInfo topology = df_vr_bitmap_topology_info_from_cfg(scope, ctrl_ctx, parse_ctx, macro_map, cfg);
  U64 expected_size = topology.width*topology.height*r_tex2d_format_bytes_per_pixel_table[topology.fmt];
  UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    ui_labelf("0x%I64x -> Bitmap (%I64u x %I64u)", base_vaddr, topology.width, topology.height);
}

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(bitmap)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  TEX_Scope *tex_scope = tex_scope_open();
  DF_VR_BitmapState *state = df_view_rule_block_user_state(key, DF_VR_BitmapState);
  if(state->last_open_frame_idx+1 < df_frame_index())
  {
    state->loaded_t = 0;
  }
  state->last_open_frame_idx = df_frame_index();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  
  //////////////////////////////
  //- rjf: evaluate expression
  //
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  DF_BitmapTopologyInfo topology_info = df_vr_bitmap_topology_info_from_cfg(di_scope, ctrl_ctx, parse_ctx, macro_map, cfg);
  U64 expected_size = topology_info.width*topology_info.height*r_tex2d_format_bytes_per_pixel_table[topology_info.fmt];
  Rng1U64 vaddr_range = r1u64(base_vaddr, base_vaddr+expected_size);
  
  //////////////////////////////
  //- rjf: map expression artifacts -> texture
  //
  U128 texture_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, vaddr_range, 0);
  TEX_Topology topology = tex_topology_make(v2s32((S32)topology_info.width, (S32)topology_info.height), topology_info.fmt);
  R_Handle texture = tex_texture_from_key_topology(tex_scope, texture_key, topology);
  
  //////////////////////////////
  //- rjf: animate
  //
  if(expected_size != 0)
  {
    if(r_handle_match(r_handle_zero(), texture))
    {
      df_gfx_request_frame();
      state->loaded_t = 0;
    }
    else
    {
      F32 rate = 1 - pow_f32(2, (-15.f * df_dt()));
      state->loaded_t += (1.f - state->loaded_t) * rate;
      if(state->loaded_t < 0.99f)
      {
        df_gfx_request_frame();
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build preview
  //
  if(expected_size != 0)
  {
    F32 img_dim = dim.y - ui_top_font_size()*2.f;
    UI_Padding(ui_pct(1.f, 0.f))
      UI_PrefWidth(ui_px(img_dim*((F32)topology_info.width/(F32)topology_info.height), 1.f))
      UI_Column UI_Padding(ui_pct(1.f, 0.f))
      UI_PrefHeight(ui_px(img_dim, 1.f))
    {
      ui_image(texture, R_Tex2DSampleKind_Nearest, r2f32(v2f32(0, 0), v2f32((F32)topology_info.width, (F32)topology_info.height)), v4f32(1, 1, 1, state->loaded_t), 10.f*(1-state->loaded_t), str8_lit("image_box"));
    }
  }
  
  tex_scope_close(tex_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}

DF_VIEW_SETUP_FUNCTION_DEF(bitmap)
{
  DF_BitmapViewState *bvs = df_view_user_state(view, DF_BitmapViewState);
  DI_Scope *di_scope = di_scope_open();
  DF_CfgNode *view_center_cfg = df_cfg_node_child_from_string(cfg_root, str8_lit("view_center"), StringMatchFlag_CaseInsensitive);
  DF_CfgNode *zoom_cfg = df_cfg_node_child_from_string(cfg_root, str8_lit("zoom"), StringMatchFlag_CaseInsensitive);
  DF_CfgNode *bitmap_cfg = df_cfg_node_child_from_string(cfg_root, str8_lit("bitmap"), StringMatchFlag_CaseInsensitive);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  U64 thread_unwind_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(di_scope, process, thread_unwind_rip_vaddr);
  bvs->view_center_pos.x = (F32)f64_from_str8(bitmap_cfg->first->string);
  bvs->view_center_pos.y = (F32)f64_from_str8(bitmap_cfg->first->next->string);
  bvs->zoom = (F32)f64_from_str8(zoom_cfg->first->string);
  bvs->top = df_vr_bitmap_topology_info_from_cfg(di_scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, bitmap_cfg);
  if(bvs->zoom == 0)
  {
    bvs->zoom = 1.f;
  }
  di_scope_close(di_scope);
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(bitmap)
{
  DF_BitmapViewState *bvs = df_view_user_state(view, DF_BitmapViewState);
  String8 result = push_str8f(arena, "view_center:(%.2f %.2f) zoom:(%.2f) bitmap:(w:%I64u, h:%I64u, fmt:%S)",
                              bvs->view_center_pos.x,
                              bvs->view_center_pos.y,
                              bvs->zoom,
                              bvs->top.width,
                              bvs->top.height,
                              r_tex2d_format_display_string_table[bvs->top.fmt]);
  return result;
}

DF_VIEW_CMD_FUNCTION_DEF(bitmap)
{
}

internal UI_BOX_CUSTOM_DRAW(df_bitmap_view_canvas_box_draw)
{
  DF_BitmapViewState *bvs = (DF_BitmapViewState *)user_data;
  Rng2F32 rect_scrn = box->rect;
  Rng2F32 rect_cvs = df_bitmap_view_state__canvas_from_screen_rect(bvs, rect_scrn, rect_scrn);
  F32 grid_cell_size_cvs = box->font_size*10.f;
  F32 grid_line_thickness_px = Max(2.f, box->font_size*0.1f);
  Vec4F32 grid_line_color = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
  for(EachEnumVal(Axis2, axis))
  {
    for(F32 v = rect_cvs.p0.v[axis] - mod_f32(rect_cvs.p0.v[axis], grid_cell_size_cvs);
        v < rect_cvs.p1.v[axis];
        v += grid_cell_size_cvs)
    {
      Vec2F32 p_cvs = {0};
      p_cvs.v[axis] = v;
      Vec2F32 p_scr = df_bitmap_view_state__screen_from_canvas_pos(bvs, rect_scrn, p_cvs);
      Rng2F32 rect = {0};
      rect.p0.v[axis] = p_scr.v[axis] - grid_line_thickness_px/2;
      rect.p1.v[axis] = p_scr.v[axis] + grid_line_thickness_px/2;
      rect.p0.v[axis2_flip(axis)] = box->rect.p0.v[axis2_flip(axis)];
      rect.p1.v[axis2_flip(axis)] = box->rect.p1.v[axis2_flip(axis)];
      d_rect(rect, grid_line_color, 0, 0, 1.f);
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(bitmap)
{
  DF_BitmapViewState *bvs = df_view_user_state(view, DF_BitmapViewState);
  Temp scratch = scratch_begin(0, 0);
  DI_Scope *di_scope = di_scope_open();
  HS_Scope *hs_scope = hs_scope_open();
  TEX_Scope *tex_scope = tex_scope_open();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  U64 thread_unwind_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(di_scope, process, thread_unwind_rip_vaddr);
  
  //////////////////////////////
  //- rjf: evaluate expression
  //
  String8 expr = str8(view->query_buffer, view->query_string_size);
  DF_Eval eval = df_eval_from_string(scratch.arena, di_scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, expr);
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx.type_graph, parse_ctx.rdi, &ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  U64 expected_size = bvs->top.width*bvs->top.height*r_tex2d_format_bytes_per_pixel_table[bvs->top.fmt];
  Rng1U64 vaddr_range = r1u64(base_vaddr, base_vaddr+expected_size);
  
  //////////////////////////////
  //- rjf: map expression artifacts -> texture
  //
  U128 texture_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, vaddr_range, 0);
  TEX_Topology topology = tex_topology_make(v2s32((S32)bvs->top.width, (S32)bvs->top.height), bvs->top.fmt);
  R_Handle texture = tex_texture_from_key_topology(tex_scope, texture_key, topology);
  
  //////////////////////////////
  //- rjf: build canvas box
  //
  UI_Box *canvas_box = &ui_g_nil_box;
  Vec2F32 canvas_dim = dim_2f32(rect);
  Rng2F32 canvas_rect = r2f32p(0, 0, canvas_dim.x, canvas_dim.y);
  UI_Rect(canvas_rect)
  {
    canvas_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_Clickable|UI_BoxFlag_Scroll, "bmp_canvas_%p", view);
    ui_box_equip_custom_draw(canvas_box, df_bitmap_view_canvas_box_draw, bvs);
  }
  
  //////////////////////////////
  //- rjf: canvas box interaction
  //
  {
    UI_Signal canvas_sig = ui_signal_from_box(canvas_box);
    if(ui_dragging(canvas_sig))
    {
      if(ui_pressed(canvas_sig))
      {
        DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
        ui_store_drag_struct(&bvs->view_center_pos);
      }
      Vec2F32 start_view_center_pos = *ui_get_drag_struct(Vec2F32);
      Vec2F32 drag_delta_scr = ui_drag_delta();
      Vec2F32 drag_delta_cvs = scale_2f32(drag_delta_scr, 1.f/bvs->zoom);
      Vec2F32 new_view_center_pos = sub_2f32(start_view_center_pos, drag_delta_cvs);
      bvs->view_center_pos = new_view_center_pos;
    }
    if(canvas_sig.scroll.y != 0)
    {
      F32 new_zoom = bvs->zoom - bvs->zoom*canvas_sig.scroll.y/10.f;
      new_zoom = Clamp(1.f/256.f, new_zoom, 256.f);
      Vec2F32 mouse_scr_pre = sub_2f32(ui_mouse(), rect.p0);
      Vec2F32 mouse_cvs = df_bitmap_view_state__canvas_from_screen_pos(bvs, canvas_rect, mouse_scr_pre);
      bvs->zoom = new_zoom;
      Vec2F32 mouse_scr_pst = df_bitmap_view_state__screen_from_canvas_pos(bvs, canvas_rect, mouse_cvs);
      Vec2F32 drift_scr = sub_2f32(mouse_scr_pst, mouse_scr_pre);
      bvs->view_center_pos = add_2f32(bvs->view_center_pos, scale_2f32(drift_scr, 1.f/new_zoom));
    }
    if(ui_double_clicked(canvas_sig))
    {
      ui_kill_action();
      MemoryZeroStruct(&bvs->view_center_pos);
      bvs->zoom = 1.f;
    }
  }
  
  //////////////////////////////
  //- rjf: build image
  //
  UI_Parent(canvas_box)
  {
    Rng2F32 img_rect_cvs = r2f32p(-topology.dim.x/2, -topology.dim.y/2, +topology.dim.x/2, +topology.dim.y/2);
    Rng2F32 img_rect_scr = df_bitmap_view_state__screen_from_canvas_rect(bvs, canvas_rect, img_rect_cvs);
    UI_Rect(img_rect_scr) UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_Floating)
    {
      ui_image(texture, R_Tex2DSampleKind_Nearest, r2f32p(0, 0, (F32)bvs->top.width, (F32)bvs->top.height), v4f32(1, 1, 1, 1), 0, str8_lit("bmp_image"));
    }
  }
  
  hs_scope_close(hs_scope);
  tex_scope_close(tex_scope);
  di_scope_close(di_scope);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "geo"

internal DF_GeoTopologyInfo
df_vr_geo_topology_info_from_cfg(DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_CfgNode *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  DF_GeoTopologyInfo result = {0};
  {
    StringJoin join = {0};
    join.sep = str8_lit(" ");
    DF_CfgNode *count_cfg         = df_cfg_node_child_from_string(cfg, str8_lit("count"), 0);
    DF_CfgNode *vertices_base_cfg = df_cfg_node_child_from_string(cfg, str8_lit("vertices_base"), 0);
    DF_CfgNode *vertices_size_cfg = df_cfg_node_child_from_string(cfg, str8_lit("vertices_size"), 0);
    String8List count_expr_strs = {0};
    String8List vertices_base_expr_strs = {0};
    String8List vertices_size_expr_strs = {0};
    for(DF_CfgNode *child = count_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &count_expr_strs, child->string);
    }
    for(DF_CfgNode *child = vertices_base_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &vertices_base_expr_strs, child->string);
    }
    for(DF_CfgNode *child = vertices_size_cfg->first; child != &df_g_nil_cfg_node; child = child->next)
    {
      str8_list_push(scratch.arena, &vertices_size_expr_strs, child->string);
    }
    String8 count_expr = str8_list_join(scratch.arena, &count_expr_strs, &join);
    String8 vertices_base_expr = str8_list_join(scratch.arena, &vertices_base_expr_strs, &join);
    String8 vertices_size_expr = str8_list_join(scratch.arena, &vertices_size_expr_strs, &join);
    DF_Eval count_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, count_expr);
    DF_Eval vertices_base_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, vertices_base_expr);
    DF_Eval vertices_size_eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, macro_map, vertices_size_expr);
    DF_Eval count_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, count_eval);
    DF_Eval vertices_base_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, vertices_base_eval);
    DF_Eval vertices_size_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, vertices_size_eval);
    U64 vertices_base_vaddr = vertices_base_val_eval.imm_u64 ? vertices_base_val_eval.imm_u64 : vertices_base_val_eval.offset;
    result.index_count = count_val_eval.imm_u64;
    result.vertices_vaddr_range = r1u64(vertices_base_vaddr, vertices_base_vaddr+vertices_size_val_eval.imm_u64);
  }
  scratch_end(scratch);
  return result;
}

internal UI_BOX_CUSTOM_DRAW(df_vr_geo_box_draw)
{
  DF_VR_GeoBoxDrawData *draw_data = (DF_VR_GeoBoxDrawData *)user_data;
  DF_VR_GeoState *state = df_view_rule_block_user_state(draw_data->key, DF_VR_GeoState);
  Vec4F32 bg_color = box->background_color;
  
  // rjf: get clip
  Rng2F32 clip = box->rect;
  for(UI_Box *b = box->parent; !ui_box_is_nil(b); b = b->parent)
  {
    if(b->flags & UI_BoxFlag_Clip)
    {
      clip = intersect_2f32(b->rect, clip);
    }
  }
  
  // rjf: calculate eye/target
  Vec3F32 target = {0};
  Vec3F32 eye = v3f32(state->zoom*cos_f32(state->yaw)*sin_f32(state->pitch),
                      state->zoom*sin_f32(state->yaw)*sin_f32(state->pitch),
                      state->zoom*cos_f32(state->pitch));
  
  // rjf: mesh
  Vec2F32 box_dim = dim_2f32(box->rect);
  R_PassParams_Geo3D *pass = d_geo3d_begin(box->rect,
                                           make_look_at_4x4f32(eye, target, v3f32(0, 0, 1)),
                                           make_perspective_4x4f32(0.25f, box_dim.x/box_dim.y, 0.1f, 500.f));
  pass->clip = clip;
  d_mesh(draw_data->vertex_buffer, draw_data->index_buffer, R_GeoTopologyKind_Triangles, R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB, r_handle_zero(), mat_4x4f32(1.f));
  
  // rjf: blur
  if(draw_data->loaded_t < 0.98f)
  {
    d_blur(intersect_2f32(clip, box->rect), 10.f-9.f*draw_data->loaded_t, 0);
  }
}

DF_CORE_VIEW_RULE_VIZ_BLOCK_PROD_FUNCTION_DEF(geo)
{
  DF_EvalVizBlock *vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Canvas, key, df_expand_key_make(df_hash_from_expand_key(key), 1), depth);
  vb->eval = eval;
  vb->string = string;
  vb->cfg_table = *cfg_table;
  vb->visual_idx_range = r1u64(0, 16);
  vb->semantic_idx_range = r1u64(0, 1);
  df_eval_viz_block_end(out, vb);
}

DF_GFX_VIEW_RULE_ROW_UI_FUNCTION_DEF(geo)
{
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    ui_labelf("0x%I64x -> Geometry", base_vaddr);
}

DF_GFX_VIEW_RULE_BLOCK_UI_FUNCTION_DEF(geo)
{
  Temp scratch = scratch_begin(0, 0);
  GEO_Scope *geo_scope = geo_scope_open();
  DF_VR_GeoState *state = df_view_rule_block_user_state(key, DF_VR_GeoState);
  if(!state->initialized)
  {
    state->initialized = 1;
    state->zoom_target = 3.5f;
    state->yaw = state->yaw_target = -0.125f;
    state->pitch = state->pitch_target = -0.125f;
  }
  if(state->last_open_frame_idx+1 < df_frame_index())
  {
    state->loaded_t = 0;
  }
  state->last_open_frame_idx = df_frame_index();
  
  //- rjf: resolve to address value
  DF_Eval value_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  U64 base_vaddr = value_eval.imm_u64 ? value_eval.imm_u64 : value_eval.offset;
  
  //- rjf: extract extra geo topology info from view rule
  DF_GeoTopologyInfo top = df_vr_geo_topology_info_from_cfg(di_scope, ctrl_ctx, parse_ctx, macro_map, cfg);
  Rng1U64 index_buffer_vaddr_range = r1u64(base_vaddr, base_vaddr+top.index_count*sizeof(U32));
  Rng1U64 vertex_buffer_vaddr_range = top.vertices_vaddr_range;
  
  //- rjf: unpack thread/process of eval
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  
  //- rjf: obtain keys for index buffer & vertex buffer memory
  U128 index_buffer_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, index_buffer_vaddr_range, 0);
  U128 vertex_buffer_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, vertex_buffer_vaddr_range, 0);
  
  //- rjf: get gpu buffers
  R_Handle index_buffer = geo_buffer_from_key(geo_scope, index_buffer_key);
  R_Handle vertex_buffer = geo_buffer_from_key(geo_scope, vertex_buffer_key);
  
  //- rjf: build preview
  F32 rate = 1 - pow_f32(2, (-15.f * df_dt()));
  if(top.index_count != 0)
  {
    UI_Padding(ui_pct(1.f, 0.f))
      UI_PrefWidth(ui_px(dim.y, 1.f))
      UI_Column UI_Padding(ui_pct(1.f, 0.f))
      UI_PrefHeight(ui_px(dim.y, 1.f))
    {
      UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable, "geo_box");
      UI_Signal sig = ui_signal_from_box(box);
      if(ui_dragging(sig))
      {
        if(ui_pressed(sig))
        {
          Vec2F32 data = v2f32(state->yaw_target, state->pitch_target);
          ui_store_drag_struct(&data);
        }
        Vec2F32 drag_delta = ui_drag_delta();
        Vec2F32 drag_start_data = *ui_get_drag_struct(Vec2F32);
        state->yaw_target = drag_start_data.x + drag_delta.x/dim_2f32(box->rect).x;
        state->pitch_target = drag_start_data.y + drag_delta.y/dim_2f32(box->rect).y;
      }
      state->zoom += (state->zoom_target - state->zoom) * rate;
      state->yaw += (state->yaw_target - state->yaw) * rate;
      state->pitch += (state->pitch_target - state->pitch) * rate;
      if(abs_f32(state->zoom-state->zoom_target) > 0.001f ||
         abs_f32(state->yaw-state->yaw_target) > 0.001f ||
         abs_f32(state->pitch-state->pitch_target) > 0.001f)
      {
        df_gfx_request_frame();
      }
      DF_VR_GeoBoxDrawData *draw_data = push_array(ui_build_arena(), DF_VR_GeoBoxDrawData, 1);
      draw_data->key = key;
      draw_data->vertex_buffer = vertex_buffer;
      draw_data->index_buffer = index_buffer;
      draw_data->loaded_t = state->loaded_t;
      ui_box_equip_custom_draw(box, df_vr_geo_box_draw, draw_data);
      if(r_handle_match(r_handle_zero(), vertex_buffer))
      {
        df_gfx_request_frame();
        state->loaded_t = 0;
      }
      else
      {
        state->loaded_t += (1.f - state->loaded_t) * rate;
        if(state->loaded_t < 0.99f)
        {
          df_gfx_request_frame();
        }
      }
    }
  }
  
  geo_scope_close(geo_scope);
  scratch_end(scratch);
}

DF_VIEW_SETUP_FUNCTION_DEF(geo) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(geo) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(geo) {}
DF_VIEW_UI_FUNCTION_DEF(geo)
{
}
