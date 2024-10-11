// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: UI Widgets: Loading Overlay

internal void
rd_loading_overlay(Rng2F32 rect, F32 loading_t, U64 progress_v, U64 progress_v_target)
{
  if(loading_t >= 0.001f)
  {
    // rjf: set up dimensions
    F32 edge_padding = 30.f;
    F32 width = ui_top_font_size() * 10;
    F32 height = ui_top_font_size() * 1.f;
    F32 min_thickness = ui_top_font_size()/2;
    F32 trail = ui_top_font_size() * 4;
    F32 t = pow_f32(sin_f32((F32)rd_state->time_in_seconds / 1.8f), 2.f);
    F64 v = 1.f - abs_f32(0.5f - t);
    
    // rjf: colors
    Vec4F32 bg_color = rd_rgba_from_theme_color(RD_ThemeColor_BaseBackground);
    Vec4F32 bd_color = rd_rgba_from_theme_color(RD_ThemeColor_FloatingBorder);
    Vec4F32 hl_color = rd_rgba_from_theme_color(RD_ThemeColor_TextNeutral);
    bg_color.w *= loading_t;
    bd_color.w *= loading_t;
    hl_color.w *= loading_t;
    
    // rjf: grab animation params
    F32 bg_work_indicator_t = 1.f;
    
    // rjf: build indicator
    UI_CornerRadius(height/3.f)
    {
      // rjf: rects
      Rng2F32 indicator_region_rect =
        r2f32p((rect.x0 + rect.x1)/2 - width/2  - rect.x0,
               (rect.y0 + rect.y1)/2 - height/2 - rect.y0,
               (rect.x0 + rect.x1)/2 + width/2  - rect.x0,
               (rect.y0 + rect.y1)/2 + height/2 - rect.y0);
      Rng2F32 indicator_rect =
        r2f32p(indicator_region_rect.x0 + width*t - min_thickness/2 - trail*v,
               indicator_region_rect.y0,
               indicator_region_rect.x0 + width*t + min_thickness/2 + trail*v,
               indicator_region_rect.y1);
      indicator_rect.x0 = Clamp(indicator_region_rect.x0, indicator_rect.x0, indicator_region_rect.x1);
      indicator_rect.x1 = Clamp(indicator_region_rect.x0, indicator_rect.x1, indicator_region_rect.x1);
      indicator_rect = pad_2f32(indicator_rect, -1.f);
      
      // rjf: does the view have loading *progress* info? -> draw extra progress layer
      if(progress_v != progress_v_target)
      {
        F64 pct_done_f64 = ((F64)progress_v/(F64)progress_v_target);
        F32 pct_done = (F32)pct_done_f64;
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = v4f32(1, 1, 1, 0.2f*loading_t)));
        ui_set_next_fixed_x(indicator_region_rect.x0);
        ui_set_next_fixed_y(indicator_region_rect.y0);
        ui_set_next_fixed_width(dim_2f32(indicator_region_rect).x*pct_done);
        ui_set_next_fixed_height(dim_2f32(indicator_region_rect).y);
        ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
      }
      
      // rjf: fill
      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = hl_color));
      ui_set_next_fixed_x(indicator_rect.x0);
      ui_set_next_fixed_y(indicator_rect.y0);
      ui_set_next_fixed_width(dim_2f32(indicator_rect).x);
      ui_set_next_fixed_height(dim_2f32(indicator_rect).y);
      ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
      
      // rjf: animated bar
      ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = bd_color, .background = bg_color));
      ui_set_next_fixed_x(indicator_region_rect.x0);
      ui_set_next_fixed_y(indicator_region_rect.y0);
      ui_set_next_fixed_width(dim_2f32(indicator_region_rect).x);
      ui_set_next_fixed_height(dim_2f32(indicator_region_rect).y);
      UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY|UI_BoxFlag_Clickable, "bg_system_status");
      UI_Signal sig = ui_signal_from_box(box);
    }
    
    // rjf: build background
    UI_WidthFill UI_HeightFill
    {
      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = bg_color));
      ui_set_next_blur_size(10.f*loading_t);
      ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
    }
  }
}

////////////////////////////////
//~ rjf: UI Widgets: Fancy Buttons

internal void
rd_cmd_binding_buttons(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  RD_BindingList bindings = rd_bindings_from_name(scratch.arena, name);
  
  //- rjf: build buttons for each binding
  for(RD_BindingNode *n = bindings.first; n != 0; n = n->next)
  {
    RD_Binding binding = n->binding;
    B32 rebinding_active_for_this_binding = (rd_state->bind_change_active &&
                                             str8_match(rd_state->bind_change_cmd_name, name, 0) &&
                                             rd_state->bind_change_binding.key == binding.key &&
                                             rd_state->bind_change_binding.modifiers == binding.modifiers);
    
    //- rjf: grab all conflicts
    String8List specs_with_binding = rd_cmd_name_list_from_binding(scratch.arena, binding);
    B32 has_conflicts = 0;
    for(String8Node *n = specs_with_binding.first; n != 0; n = n->next)
    {
      if(!str8_match(n->string, name, 0))
      {
        has_conflicts = 1;
        break;
      }
    }
    
    //- rjf: form binding string
    String8 keybinding_str = {0};
    {
      if(binding.key != OS_Key_Null)
      {
        String8List mods = os_string_list_from_modifiers(scratch.arena, binding.modifiers);
        String8 key = os_g_key_display_string_table[binding.key];
        str8_list_push(scratch.arena, &mods, key);
        StringJoin join = {0};
        join.sep = str8_lit(" + ");
        keybinding_str = str8_list_join(scratch.arena, &mods, &join);
      }
      else
      {
        keybinding_str = str8_lit("- no binding -");
      }
    }
    
    //- rjf: form color palette
    UI_Palette *palette = ui_top_palette();
    if(has_conflicts || rebinding_active_for_this_binding)
    {
      palette = push_array(ui_build_arena(), UI_Palette, 1);
      MemoryCopyStruct(palette, ui_top_palette());
      if(has_conflicts)
      {
        palette->colors[UI_ColorCode_Text] = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative);
        palette->colors[UI_ColorCode_TextWeak] = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative);
      }
      if(rebinding_active_for_this_binding)
      {
        palette->colors[UI_ColorCode_Border] = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
        palette->colors[UI_ColorCode_Background] = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
        palette->colors[UI_ColorCode_Background].w *= 0.25f;
      }
    }
    
    //- rjf: build box
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_palette(palette);
    ui_set_next_group_key(ui_key_zero());
    ui_set_next_pref_width(ui_text_dim(ui_top_font_size()*1.f, 1));
    UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                            UI_BoxFlag_Clickable|
                                            UI_BoxFlag_DrawActiveEffects|
                                            UI_BoxFlag_DrawHotEffects|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DrawBackground,
                                            "%S###bind_btn_%S_%x_%x", keybinding_str, name, binding.key, binding.modifiers);
    
    //- rjf: interaction
    UI_Signal sig = ui_signal_from_box(box);
    {
      // rjf: click => toggle activity
      if(!rd_state->bind_change_active && ui_clicked(sig))
      {
        if((binding.key == OS_Key_Esc || binding.key == OS_Key_Delete) && binding.modifiers == 0)
        {
          log_user_error(str8_lit("Cannot rebind; this command uses a reserved keybinding."));
        }
        else
        {
          arena_clear(rd_state->bind_change_arena);
          rd_state->bind_change_active = 1;
          rd_state->bind_change_cmd_name = push_str8_copy(rd_state->bind_change_arena, name);
          rd_state->bind_change_binding = binding;
        }
      }
      else if(rd_state->bind_change_active && ui_clicked(sig))
      {
        rd_state->bind_change_active = 0;
      }
      
      // rjf: hover w/ conflicts => show conflicts
      if(ui_hovering(sig) && has_conflicts) UI_Tooltip
      {
        UI_PrefWidth(ui_children_sum(1)) rd_error_label(str8_lit("This binding conflicts with those for:"));
        for(String8Node *n = specs_with_binding.first; n != 0; n = n->next)
        {
          if(!str8_match(n->string, name, 0))
          {
            RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(n->string);
            ui_labelf("%S", info->display_name);
          }
        }
      }
    }
    
    //- rjf: delete button
    if(rebinding_active_for_this_binding)
      UI_PrefWidth(ui_em(2.5f, 1.f))
      UI_Palette(ui_build_palette(ui_top_palette(),
                                  .background = rd_rgba_from_theme_color(RD_ThemeColor_NegativePopButtonBackground),
                                  .border = rd_rgba_from_theme_color(RD_ThemeColor_NegativePopButtonBorder),
                                  .text = rd_rgba_from_theme_color(RD_ThemeColor_Text)))
    {
      ui_set_next_group_key(ui_key_zero());
      UI_Signal sig = rd_icon_button(RD_IconKind_X, 0, str8_lit("###delete_binding"));
      if(ui_clicked(sig))
      {
        rd_unbind_name(name, binding);
        rd_state->bind_change_active = 0;
      }
    }
    
    //- rjf: space
    ui_spacer(ui_em(1.f, 1.f));
  }
  
  //- rjf: build "add new binding" button
  RD_Font(RD_FontSlot_Icons)
  {
    UI_Palette *palette = ui_top_palette();
    B32 adding_new_binding = (rd_state->bind_change_active &&
                              str8_match(rd_state->bind_change_cmd_name, name, 0) &&
                              rd_state->bind_change_binding.key == OS_Key_Null &&
                              rd_state->bind_change_binding.modifiers == 0);
    if(adding_new_binding)
    {
      palette = ui_build_palette(ui_top_palette());
      palette->colors[UI_ColorCode_Border] = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
      palette->colors[UI_ColorCode_Background] = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
      palette->colors[UI_ColorCode_Background].w *= 0.25f;
    }
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_group_key(ui_key_zero());
    ui_set_next_pref_width(ui_text_dim(ui_top_font_size()*1.f, 1));
    ui_set_next_palette(palette);
    UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                            UI_BoxFlag_Clickable|
                                            UI_BoxFlag_DrawActiveEffects|
                                            UI_BoxFlag_DrawHotEffects|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DrawBackground,
                                            "%S###add_binding", rd_icon_kind_text_table[RD_IconKind_Add]);
    UI_Signal sig = ui_signal_from_box(box);
    if(ui_clicked(sig))
    {
      if(!rd_state->bind_change_active && ui_clicked(sig))
      {
        arena_clear(rd_state->bind_change_arena);
        rd_state->bind_change_active = 1;
        rd_state->bind_change_cmd_name = push_str8_copy(rd_state->bind_change_arena, name);
        MemoryZeroStruct(&rd_state->bind_change_binding);
      }
      else if(rd_state->bind_change_active && ui_clicked(sig))
      {
        rd_state->bind_change_active = 0;
      }
    }
  }
  
  scratch_end(scratch);
}

internal UI_Signal
rd_menu_bar_button(String8 string)
{
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable|UI_BoxFlag_DrawHotEffects, string);
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal UI_Signal
rd_cmd_spec_button(String8 name)
{
  RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(name);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects|
                                          UI_BoxFlag_Clickable,
                                          "###cmd_%p", info);
  UI_Parent(box) UI_HeightFill UI_Padding(ui_em(1.f, 1.f))
  {
    RD_IconKind canonical_icon = info->icon_kind;
    if(canonical_icon != RD_IconKind_Null)
    {
      RD_Font(RD_FontSlot_Icons)
        UI_PrefWidth(ui_em(2.f, 1.f))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      {
        ui_label(rd_icon_kind_text_table[canonical_icon]);
      }
    }
    UI_PrefWidth(ui_text_dim(10, 1.f))
    {
      UI_Flags(UI_BoxFlag_DrawTextFastpathCodepoint)
        UI_FastpathCodepoint(box->fastpath_codepoint)
        ui_label(info->display_name);
      ui_spacer(ui_pct(1, 0));
      ui_set_next_flags(UI_BoxFlag_Clickable);
      ui_set_next_group_key(ui_key_zero());
      UI_PrefWidth(ui_children_sum(1))
        UI_FontSize(ui_top_font_size()*0.95f) UI_HeightFill
        UI_NamedRow(str8_lit("###bindings"))
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
        UI_FastpathCodepoint(0)
      {
        rd_cmd_binding_buttons(name);
      }
    }
  }
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal void
rd_cmd_list_menu_buttons(U64 count, String8 *cmd_names, U32 *fastpath_codepoints)
{
  Temp scratch = scratch_begin(0, 0);
  for(U64 idx = 0; idx < count; idx += 1)
  {
    ui_set_next_fastpath_codepoint(fastpath_codepoints[idx]);
    UI_Signal sig = rd_cmd_spec_button(cmd_names[idx]);
    if(ui_clicked(sig))
    {
      rd_cmd(RD_CmdKind_RunCommand, .cmd_name = cmd_names[idx]);
      ui_ctx_menu_close();
      RD_Window *window = rd_window_from_handle(rd_regs()->window);
      window->menu_bar_focused = 0;
    }
  }
  scratch_end(scratch);
}

internal UI_Signal
rd_icon_button(RD_IconKind kind, FuzzyMatchRangeList *matches, String8 string)
{
  String8 display_string = ui_display_part_from_key_string(string);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|
                                         UI_BoxFlag_DrawBorder|
                                         UI_BoxFlag_DrawBackground|
                                         UI_BoxFlag_DrawHotEffects|
                                         UI_BoxFlag_DrawActiveEffects,
                                         string);
  UI_Parent(box)
  {
    if(display_string.size == 0)
    {
      ui_spacer(ui_pct(1, 0));
    }
    else
    {
      ui_spacer(ui_em(1.f, 1.f));
    }
    UI_TextAlignment(UI_TextAlign_Center)
      RD_Font(RD_FontSlot_Icons)
      UI_PrefWidth(ui_em(2.f, 1.f))
      UI_PrefHeight(ui_pct(1, 0))
      UI_FlagsAdd(UI_BoxFlag_DisableTextTrunc|UI_BoxFlag_DrawTextWeak)
      ui_label(rd_icon_kind_text_table[kind]);
    if(display_string.size != 0)
    {
      UI_PrefWidth(ui_pct(1.f, 0.f))
      {
        UI_Box *box = ui_label(display_string).box;
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
    }
    if(display_string.size == 0)
    {
      ui_spacer(ui_pct(1, 0));
    }
    else
    {
      ui_spacer(ui_em(1.f, 1.f));
    }
  }
  UI_Signal result = ui_signal_from_box(box);
  return result;
}

internal UI_Signal
rd_icon_buttonf(RD_IconKind kind, FuzzyMatchRangeList *matches, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = rd_icon_button(kind, matches, string);
  scratch_end(scratch);
  return sig;
}

internal void
rd_entity_tooltips(RD_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  RD_Palette(RD_PaletteCode_Floating) switch(entity->kind)
  {
    default:{}break;
    case RD_EntityKind_File:
    UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      String8 full_path = rd_full_path_from_entity(scratch.arena, entity);
      ui_label(full_path);
    }break;
    case RD_EntityKind_Thread: UI_Flags(0)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      String8 display_string = rd_display_string_from_entity(scratch.arena, entity);
      CTRL_Entity *entity_ctrl = ctrl_entity_from_handle(d_state->ctrl_entity_store, entity->ctrl_handle);
      U64 rip_vaddr = d_query_cached_rip_from_thread(entity_ctrl);
      Arch arch = entity_ctrl->arch;
      String8 arch_str = string_from_arch(arch);
      U32 pid_or_tid = entity->ctrl_id;
      if(display_string.size != 0) UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        if(entity->flags & RD_EntityFlag_HasColor)
        {
          ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_entity(entity)));
        }
        UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_string);
      }
      {
        CTRL_Event stop_event = d_ctrl_last_stop_event();
        RD_Entity *stopper_thread = rd_entity_from_ctrl_handle(stop_event.entity);
        if(stopper_thread == entity)
        {
          ui_spacer(ui_em(1.5f, 1.f));
          RD_IconKind icon_kind = RD_IconKind_Null;
          String8 explanation = rd_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon_kind);
          if(explanation.size != 0)
          {
            UI_Palette *palette = ui_top_palette();
            if(stop_event.cause == CTRL_EventCause_Error ||
               stop_event.cause == CTRL_EventCause_InterruptedByException ||
               stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
               stop_event.cause == CTRL_EventCause_UserBreakpoint)
            {
              palette = ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative));
            }
            UI_PrefWidth(ui_children_sum(1)) UI_Row UI_Palette(palette)
            {
              UI_PrefWidth(ui_em(1.5f, 1.f))
                RD_Font(RD_FontSlot_Icons)
                ui_label(rd_icon_kind_text_table[icon_kind]);
              UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            }
          }
        }
      }
      ui_spacer(ui_em(1.5f, 1.f));
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("TID: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("%i", pid_or_tid);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Arch: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_label(arch_str);
      }
      ui_spacer(ui_em(1.5f, 1.f));
      DI_Scope *di_scope = di_scope_open();
      CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity_ctrl, CTRL_EntityKind_Process);
      CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(entity_ctrl);
      CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
      for(U64 idx = 0; idx < rich_unwind.concrete_frame_count; idx += 1)
      {
        CTRL_CallStackFrame *f = &rich_unwind.frames[idx];
        RDI_Parsed *rdi = f->rdi;
        RDI_Procedure *procedure = f->procedure;
        U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
        String8 module_name = module == &ctrl_entity_nil ? str8_lit("???") : str8_skip_last_slash(module->string);
        
        // rjf: inline frames
        for(CTRL_CallStackInlineFrame *fin = f->last_inline_frame; fin != 0; fin = fin->prev)
          UI_PrefWidth(ui_children_sum(1)) UI_Row
        {
          String8 name = {0};
          name.str = rdi_string_from_idx(rdi, fin->inline_site->name_string_idx, &name.size);
          RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("0x%I64x", rip_vaddr);
          RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_label(str8_lit("[inlined]"));
          if(name.size != 0)
          {
            RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
            {
              rd_code_label(1.f, 0, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
            }
          }
          else
          {
            RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
          }
        }
        
        // rjf: concrete frame
        UI_PrefWidth(ui_children_sum(1)) UI_Row
        {
          String8 name = {0};
          name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
          RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("0x%I64x", rip_vaddr);
          if(name.size != 0)
          {
            RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
            {
              rd_code_label(1.f, 0, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
            }
          }
          else
          {
            RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
          }
        }
      }
      di_scope_close(di_scope);
    }break;
    case RD_EntityKind_Breakpoint: UI_Flags(0)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & RD_EntityFlag_HasColor)
      {
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_entity(entity)));
      }
      String8 display_string = rd_display_string_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_string);
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        String8 stop_condition = rd_entity_child_from_kind(entity, RD_EntityKind_Condition)->string;
        if(stop_condition.size == 0)
        {
          stop_condition = str8_lit("true");
        }
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Stop Condition: ");
        UI_PrefWidth(ui_text_dim(10, 1)) RD_Font(RD_FontSlot_Code) rd_code_label(1.f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), stop_condition);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        U64 hit_count = entity->u64;
        String8 hit_count_text = str8_from_u64(scratch.arena, hit_count, 10, 0, 0);
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Hit Count: ");
        UI_PrefWidth(ui_text_dim(10, 1)) RD_Font(RD_FontSlot_Code) rd_code_label(1.f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), hit_count_text);
      }
    }break;
    case RD_EntityKind_WatchPin:
    RD_Font(RD_FontSlot_Code)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & RD_EntityFlag_HasColor)
      {
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_entity(entity)));
      }
      String8 display_string = rd_display_string_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_text_dim(10, 1)) rd_code_label(1.f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), display_string);
    }break;
  }
  scratch_end(scratch);
}

internal UI_Signal
rd_entity_desc_button(RD_Entity *entity, FuzzyMatchRangeList *name_matches, String8 fuzzy_query, B32 is_implicit)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  UI_Palette *palette = ui_top_palette();
  if(entity->kind == RD_EntityKind_Thread)
  {
    CTRL_Event stop_event = d_ctrl_last_stop_event();
    CTRL_Entity *entity_ctrl = ctrl_entity_from_handle(d_state->ctrl_entity_store, entity->ctrl_handle);
    CTRL_Entity *stopped_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, stop_event.entity);
    CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
    if(selected_thread == entity_ctrl)
    {
      palette = rd_palette_from_code(RD_PaletteCode_NeutralPopButton);
    }
    if(stopped_thread == entity_ctrl &&
       (stop_event.cause == CTRL_EventCause_UserBreakpoint ||
        stop_event.cause == CTRL_EventCause_InterruptedByException ||
        stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
        stop_event.cause == CTRL_EventCause_InterruptedByHalt))
    {
      palette = rd_palette_from_code(RD_PaletteCode_NegativePopButton);
    }
  }
  if(entity->cfg_src == RD_CfgSrc_CommandLine)
  {
    palette = rd_palette_from_code(RD_PaletteCode_NeutralPopButton);
  }
  else if(entity->kind == RD_EntityKind_Target && !entity->disabled)
  {
    palette = rd_palette_from_code(RD_PaletteCode_NeutralPopButton);
  }
  ui_set_next_palette(palette);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                          (!is_implicit*UI_BoxFlag_DrawBorder)|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects,
                                          "entity_ref_button_%p", entity);
  
  //- rjf: build contents
  UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 0)) UI_Padding(ui_em(1.f, 1.f))
  {
    RD_EntityKindFlags kind_flags = rd_entity_kind_flags_table[entity->kind];
    RD_IconKind icon = rd_entity_kind_icon_kind_table[entity->kind];
    Vec4F32 entity_color = palette->colors[UI_ColorCode_Text];
    Vec4F32 entity_color_weak = palette->colors[UI_ColorCode_TextWeak];
    if(entity->flags & RD_EntityFlag_HasColor)
    {
      entity_color = rd_rgba_from_entity(entity);
      entity_color_weak = entity_color;
      entity_color_weak.w *= 0.5f;
    }
    UI_TextAlignment(UI_TextAlign_Center)
      RD_Font(RD_FontSlot_Icons)
      UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
      UI_PrefWidth(ui_em(1.875f, 1.f))
      UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      ui_label(rd_icon_kind_text_table[icon]);
    if(entity->cfg_src == RD_CfgSrc_CommandLine)
    {
      UI_TextAlignment(UI_TextAlign_Center)
        UI_PrefWidth(ui_em(1.875f, 1.f))
      {
        UI_Box *info_box = &ui_nil_box;
        RD_Font(RD_FontSlot_Icons)
          UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
        {
          info_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DrawTextWeak|UI_BoxFlag_Clickable, "%S###%p_temp_info", rd_icon_kind_text_table[RD_IconKind_Info], entity);
        }
        UI_Signal info_sig = ui_signal_from_box(info_box);
        if(ui_hovering(info_sig)) UI_Tooltip
        {
          ui_labelf("Specified via command line; not saved in project.");
        }
      }
    }
    String8 label = rd_display_string_from_entity(scratch.arena, entity);
    UI_Palette(ui_build_palette(ui_top_palette(), .text = entity_color))
      RD_Font(kind_flags&RD_EntityKindFlag_NameIsCode ? RD_FontSlot_Code : RD_FontSlot_Main)
      UI_Flags((entity->kind == RD_EntityKind_Thread ||
                entity->kind == RD_EntityKind_Breakpoint ||
                entity->kind == RD_EntityKind_WatchPin)
               ? UI_BoxFlag_DisableTruncatedHover
               : 0)
    {
      UI_Signal label_sig = ui_label(label);
      if(name_matches != 0)
      {
        ui_box_equip_fuzzy_match_ranges(label_sig.box, name_matches);
      }
    }
    if(entity->kind == RD_EntityKind_Target) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_FontSize(ui_top_font_size()*0.95f)
    {
      RD_Entity *args = rd_entity_child_from_kind(entity, RD_EntityKind_Arguments);
      ui_label(args->string);
    }
    if(kind_flags & RD_EntityKindFlag_CanEnable && entity->disabled) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_FontSize(ui_top_font_size()*0.95f) UI_HeightFill
    {
      ui_label(str8_lit("(Disabled)"));
    }
    if(entity->kind == RD_EntityKind_Thread)
      UI_FontSize(ui_top_font_size()*0.75f)
      RD_Font(RD_FontSlot_Code)
      UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol)))
      UI_Flags(UI_BoxFlag_DisableTruncatedHover)
    {
      CTRL_Entity *entity_ctrl = ctrl_entity_from_handle(d_state->ctrl_entity_store, entity->ctrl_handle);
      CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity_ctrl);
      CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity_ctrl, CTRL_EntityKind_Process);
      U64 idx = 0;
      U64 limit = 3;
      ui_spacer(ui_em(1.f, 1.f));
      for(U64 num = unwind.frames.count; num > 0; num -= 1)
      {
        CTRL_UnwindFrame *f = &unwind.frames.v[num-1];
        U64 rip_vaddr = regs_rip_from_arch_block(entity_ctrl->arch, f->regs);
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
        U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
        String8 procedure_name = d_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff, 0);
        if(procedure_name.size != 0)
        {
          FuzzyMatchRangeList fuzzy_matches = {0};
          if(fuzzy_query.size != 0)
          {
            fuzzy_matches = fuzzy_match_find(scratch.arena, fuzzy_query, procedure_name);
          }
          if(idx != 0)
          {
            UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(2.f, 1.f)) ui_label(str8_lit(">"));
          }
          UI_PrefWidth(ui_text_dim(10.f, 0.f))
          {
            UI_Box *label_box = ui_label(procedure_name).box;
            ui_box_equip_fuzzy_match_ranges(label_box, &fuzzy_matches);
          }
          idx += 1;
          if(idx == limit)
          {
            UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10.f, 1.f)) ui_label(str8_lit("> ..."));
          }
        }
      }
    }
  }
  
  //- rjf: do interaction on main box
  UI_Signal sig = ui_signal_from_box(box);
  {
    if(ui_hovering(sig) && !rd_drag_is_active())
    {
      rd_entity_tooltips(entity);
    }
    
    // rjf: click => fastpath for this entity
    if(ui_clicked(sig))
    {
      // TODO(rjf): rd_cmd(RD_CmdKind_EntityRefFastPath, .entity = rd_handle_from_entity(entity));
    }
    
    // rjf: right-click => context menu for this entity
    else if(ui_right_clicked(sig))
    {
      // RD_Handle handle = rd_handle_from_entity(entity);
      // RD_Window *window = rd_window_from_handle(rd_regs()->window);
      // ui_ctx_menu_open(rd_state->entity_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0));
      // window->entity_ctx_menu_entity = handle;
    }
    
    // rjf: drag+drop
    else if(ui_dragging(sig) && !contains_2f32(box->rect, ui_mouse()))
    {
      RD_RegsScope(.entity = rd_handle_from_entity(entity)) rd_drag_begin(RD_RegSlot_Entity);
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return sig;
}

internal void
rd_src_loc_button(String8 file_path, TxtPt point)
{
  Temp scratch = scratch_begin(0, 0);
  String8 filename = str8_skip_last_slash(file_path);
  
  // rjf: build main box
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                          UI_BoxFlag_DrawBorder|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects,
                                          "file_loc_button_%S", file_path);
  UI_Signal sig = ui_signal_from_box(box);
  
  // rjf: build contents
  UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 0))
  {
    RD_IconKind icon = RD_IconKind_FileOutline;
    UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      UI_TextAlignment(UI_TextAlign_Center)
      RD_Font(RD_FontSlot_Icons)
      UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
      ui_label(rd_icon_kind_text_table[icon]);
    ui_labelf("%S:%I64d:%I64d", filename, point.line, point.column);
  }
  
  // rjf: click => find code location
  if(ui_clicked(sig))
  {
    rd_cmd(RD_CmdKind_FindCodeLocation, .file_path = file_path, .cursor = point);
  }
  
  // rjf: hover => show full path
  else if(ui_hovering(sig) && !ui_dragging(sig)) UI_Tooltip
  {
    ui_labelf("%S:%I64d:%I64d", file_path, point.line, point.column);
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: UI Widgets: Text View

typedef struct RD_ThreadBoxDrawExtData RD_ThreadBoxDrawExtData;
struct RD_ThreadBoxDrawExtData
{
  Vec4F32 thread_color;
  F32 progress_t;
  F32 alive_t;
  F32 hover_t;
  B32 is_selected;
  B32 is_frozen;
  B32 do_lines;
  B32 do_glow;
};

internal UI_BOX_CUSTOM_DRAW(rd_thread_box_draw_extensions)
{
  RD_ThreadBoxDrawExtData *u = (RD_ThreadBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  if(u->do_lines)
  {
    R_Rect2DInst *inst = dr_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                        box->parent->rect.y0 - box->font_size*0.125f,
                                        box->parent->parent->parent->rect.x0 + box->font_size*260*u->alive_t,
                                        box->parent->rect.y0 + box->font_size*0.125f),
                                 v4f32(u->thread_color.x, u->thread_color.y, u->thread_color.z, 0),
                                 0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = u->thread_color;
  }
  
  // rjf: draw 'progress bar', showing thread's progress through the line's address range
  if(u->progress_t > 0)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.4f;
    dr_rect(r2f32p(box->rect.x0,
                   box->rect.y0,
                   box->rect.x1,
                   box->rect.y0 + (box->rect.y1-box->rect.y0)*u->progress_t),
            weak_thread_color,
            0, 0, 1);
  }
  
  // rjf: draw rich hover fill
  if(u->hover_t > 0.001f)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.5f*u->hover_t;
    R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0,
                                        box->parent->rect.y0,
                                        box->rect.x0 + ui_top_font_size()*22.f*u->hover_t,
                                        box->parent->rect.y1),
                                 v4f32(0, 0, 0, 0),
                                 0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: draw slight fill on selected thread
  if(u->is_selected && u->do_glow)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0,
                                        box->parent->rect.y0,
                                        box->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                        box->parent->rect.y1),
                                 v4f32(0, 0, 0, 0),
                                 0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: locked icon on frozen threads
  if(u->is_frozen)
  {
    F32 lock_icon_off = ui_top_font_size()*0.2f;
    Vec4F32 lock_icon_color = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative);
    dr_text(rd_font_from_slot(RD_FontSlot_Icons),
            box->font_size, 0, 0, FNT_RasterFlag_Smooth,
            v2f32((box->rect.x0 + box->rect.x1)/2 + lock_icon_off/2,
                  box->rect.y0 + lock_icon_off/2),
            lock_icon_color,
            rd_icon_kind_text_table[RD_IconKind_Locked]);
  }
}

typedef struct RD_BreakpointBoxDrawExtData RD_BreakpointBoxDrawExtData;
struct RD_BreakpointBoxDrawExtData
{
  Vec4F32 color;
  F32 alive_t;
  F32 hover_t;
  F32 remap_px_delta;
  B32 do_lines;
  B32 do_glow;
};

internal UI_BOX_CUSTOM_DRAW(rd_bp_box_draw_extensions)
{
  RD_BreakpointBoxDrawExtData *u = (RD_BreakpointBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  if(u->do_lines)
  {
    R_Rect2DInst *inst = dr_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                        box->parent->rect.y0 - box->font_size*0.125f,
                                        box->parent->parent->parent->rect.x0 + ui_top_font_size()*250.f*u->alive_t,
                                        box->parent->rect.y0 + box->font_size*0.125f),
                                 v4f32(u->color.x, u->color.y, u->color.z, 0),
                                 0, 0, 1.f);
    inst->colors[Corner_00] = inst->colors[Corner_01] = u->color;
  }
  
  // rjf: draw rich hover fill
  if(u->hover_t > 0.001f)
  {
    Vec4F32 weak_color = u->color;
    weak_color.w *= 0.5f*u->hover_t;
    R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0,
                                        box->parent->rect.y0,
                                        box->rect.x0 + ui_top_font_size()*22.f*u->hover_t,
                                        box->parent->rect.y1),
                                 v4f32(0, 0, 0, 0),
                                 0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_color;
  }
  
  // rjf: draw slight fill
  if(u->do_glow)
  {
    Vec4F32 weak_thread_color = u->color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0,
                                        box->parent->rect.y0,
                                        box->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                        box->parent->rect.y1),
                                 v4f32(0, 0, 0, 0),
                                 0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: draw remaps
  if(u->remap_px_delta != 0)
  {
    F32 remap_px_delta = u->remap_px_delta;
    F32 circle_advance = fnt_dim_from_tag_size_string(box->font, box->font_size, 0, 0, rd_icon_kind_text_table[RD_IconKind_CircleFilled]).x;
    Vec2F32 bp_text_pos = ui_box_text_position(box);
    Vec2F32 bp_center = v2f32(bp_text_pos.x + circle_advance/2, bp_text_pos.y);
    FNT_Metrics icon_font_metrics = fnt_metrics_from_tag_size(box->font, box->font_size);
    F32 icon_font_line_height = fnt_line_height_from_metrics(&icon_font_metrics);
    F32 remap_bar_thickness = 0.3f*ui_top_font_size();
    Vec4F32 remap_color = u->color;
    remap_color.w *= 0.3f;
    R_Rect2DInst *inst = dr_rect(r2f32p(bp_center.x - remap_bar_thickness,
                                        bp_center.y + ClampTop(remap_px_delta, 0) + remap_bar_thickness,
                                        bp_center.x + remap_bar_thickness,
                                        bp_center.y + ClampBot(remap_px_delta, 0) - remap_bar_thickness),
                                 remap_color, 2.f, 0, 1.f);
    dr_text(box->font, box->font_size, 0, 0, FNT_RasterFlag_Smooth,
            v2f32(bp_text_pos.x,
                  bp_center.y + remap_px_delta),
            remap_color,
            rd_icon_kind_text_table[RD_IconKind_CircleFilled]);
  }
}

internal RD_CodeSliceSignal
rd_code_slice(RD_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, String8 string)
{
  RD_CodeSliceSignal result = {0};
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
  CTRL_Entity *selected_thread_process = ctrl_entity_ancestor_from_kind(selected_thread, CTRL_EntityKind_Process);
  U64 selected_thread_rip_unwind_vaddr = d_query_cached_rip_from_thread_unwind(selected_thread, rd_regs()->unwind_count);
  CTRL_Entity *selected_thread_module = ctrl_module_from_process_vaddr(selected_thread_process, selected_thread_rip_unwind_vaddr);
  F32 selected_thread_alive_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###selected_thread_alive_t_%p", selected_thread), 1.f);
  F32 selected_thread_module_alive_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###selected_thread_module_alive_t_%p", selected_thread_module), 1.f);
  F32 selected_thread_arch_alive_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###selected_thread_arch_alive_t_%i", selected_thread->arch), 1.f);
  CTRL_Event stop_event = d_ctrl_last_stop_event();
  CTRL_Entity *stopper_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, stop_event.entity);
  B32 is_focused = ui_is_focus_active();
  B32 ctrlified = (os_get_modifiers() & OS_Modifier_Ctrl);
  Vec4F32 code_line_bgs[] =
  {
    rd_rgba_from_theme_color(RD_ThemeColor_LineInfoBackground0),
    rd_rgba_from_theme_color(RD_ThemeColor_LineInfoBackground1),
    rd_rgba_from_theme_color(RD_ThemeColor_LineInfoBackground2),
    rd_rgba_from_theme_color(RD_ThemeColor_LineInfoBackground3),
  };
  UI_Palette *margin_palette = rd_palette_from_code(RD_PaletteCode_Floating);
  UI_Palette *margin_contents_palette = ui_build_palette(rd_palette_from_code(RD_PaletteCode_Floating));
  margin_contents_palette->background = v4f32(0, 0, 0, 0);
  F32 line_num_padding_px = ui_top_font_size()*1.f;
  F32 entity_alive_t_rate = (1 - pow_f32(2, (-30.f * rd_state->frame_dt)));
  F32 entity_hover_t_rate = rd_setting_val_from_code(RD_SettingCode_HoverAnimations).s32 ? (1 - pow_f32(2, (-20.f * rd_state->frame_dt))) : 1.f;
  
  //////////////////////////////
  //- rjf: build top-level container
  //
  UI_Box *top_container_box = &ui_nil_box;
  Rng2F32 clipped_top_container_rect = {0};
  {
    ui_set_next_child_layout_axis(Axis2_X);
    ui_set_next_pref_width(ui_px(params->line_text_max_width_px, 1));
    ui_set_next_pref_height(ui_children_sum(1));
    top_container_box = ui_build_box_from_string(UI_BoxFlag_DisableFocusEffects|UI_BoxFlag_DrawBorder, string);
    clipped_top_container_rect = top_container_box->rect;
    for(UI_Box *b = top_container_box; !ui_box_is_nil(b); b = b->parent)
    {
      if(b->flags & UI_BoxFlag_Clip)
      {
        clipped_top_container_rect = intersect_2f32(b->rect, clipped_top_container_rect);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build per-line background colors
  //
  Vec4F32 *line_bg_colors = push_array(scratch.arena, Vec4F32, dim_1s64(params->line_num_range)+1);
  {
    //- rjf: color line with stopper-thread red
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      CTRL_EntityList threads = params->line_ips[line_idx];
      for(CTRL_EntityNode *n = threads.first; n != 0; n = n->next)
      {
        if(n->v == stopper_thread && (stop_event.cause == CTRL_EventCause_InterruptedByTrap || stop_event.cause == CTRL_EventCause_InterruptedByException))
        {
          line_bg_colors[line_idx] = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlayError);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build priority margin
  //
  UI_Box *priority_margin_container_box = &ui_nil_box;
  if(params->flags & RD_CodeSliceFlag_PriorityMargin) UI_Focus(UI_FocusKind_Off) UI_Parent(top_container_box) UI_Palette(margin_palette) ProfScope("build priority margins")
  {
    if(params->margin_float_off_px != 0)
    {
      ui_set_next_pref_width(ui_px(params->priority_margin_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_build_box_from_key(0, ui_key_zero());
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px));
    }
    ui_set_next_pref_width(ui_px(params->priority_margin_width_px, 1));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_child_layout_axis(Axis2_Y);
    priority_margin_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable), str8_lit("priority_margin_container"));
    UI_Parent(priority_margin_container_box) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Palette(margin_contents_palette)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        CTRL_EntityList line_ips  = params->line_ips[line_idx];
        ui_set_next_hover_cursor(OS_Cursor_HandPoint);
        UI_Box *line_margin_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, "line_margin_%I64x", line_num);
        UI_Parent(line_margin_box)
        {
          //- rjf: build margin thread ip ui
          for(CTRL_EntityNode *n = line_ips.first; n != 0; n = n->next)
          {
            // rjf: unpack thread
            CTRL_Entity *thread = n->v;
            if(thread != selected_thread)
            {
              continue;
            }
            U64 unwind_count = (thread == selected_thread) ? rd_regs()->unwind_count : 0;
            U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_count);
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
            DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
            U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
            
            // rjf: thread info => color
            Vec4F32 color = rd_rgba_from_ctrl_entity(thread);
            {
              if(unwind_count != 0)
              {
                color = rd_rgba_from_theme_color(RD_ThemeColor_ThreadUnwound);
              }
              else if(thread == stopper_thread &&
                      (stop_event.cause == CTRL_EventCause_InterruptedByHalt ||
                       stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
                       stop_event.cause == CTRL_EventCause_InterruptedByException))
              {
                color = rd_rgba_from_theme_color(RD_ThemeColor_ThreadError);
              }
              if(d_ctrl_targets_running() && d_ctrl_last_run_frame_idx() < d_frame_index())
              {
                color.w *= 0.5f;
              }
              if(thread != selected_thread)
              {
                color.w *= 0.8f;
              }
            }
            
            // rjf: build thread box
            ui_set_next_hover_cursor(OS_Cursor_UpDownLeftRight);
            ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
            ui_set_next_font_size(params->font_size);
            ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
            ui_set_next_pref_width(ui_pct(1, 0));
            ui_set_next_pref_height(ui_pct(1, 0));
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Key thread_box_key = ui_key_from_stringf(top_container_box->key, "###ip_%I64x_%p", line_num, thread);
            UI_Box *thread_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|
                                                       UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|
                                                       UI_BoxFlag_DrawText,
                                                       thread_box_key);
            ui_box_equip_display_string(thread_box, rd_icon_kind_text_table[RD_IconKind_RightArrow]);
            UI_Signal thread_sig = ui_signal_from_box(thread_box);
            
            // rjf: custom draw
            {
              RD_Regs *hover_regs = rd_get_hover_regs();
              B32 is_hovering = (ctrl_handle_match(hover_regs->thread, thread->handle) &&
                                 rd_state->hover_regs_slot == RD_RegSlot_Thread);
              RD_ThreadBoxDrawExtData *u = push_array(ui_build_arena(), RD_ThreadBoxDrawExtData, 1);
              u->thread_color = color;
              u->alive_t      = ui_anim(ui_key_from_stringf(top_container_box->key, "###thread_alive_t_%p", thread), 1.f, .rate = entity_alive_t_rate);
              u->hover_t      = ui_anim(ui_key_from_stringf(top_container_box->key, "###thread_hover_t_%p", thread), (F32)!!is_hovering, .rate = entity_hover_t_rate);
              u->is_selected  = (thread == selected_thread);
              u->is_frozen    = !!thread->is_frozen;
              u->do_lines     = rd_setting_val_from_code(RD_SettingCode_ThreadLines).s32;
              u->do_glow      = rd_setting_val_from_code(RD_SettingCode_ThreadGlow).s32;
              ui_box_equip_custom_draw(thread_box, rd_thread_box_draw_extensions, u);
              
              // rjf: fill out progress t (progress into range of current line's
              // voff range)
              if(params->line_infos[line_idx].first != 0)
              {
                D_LineList *lines = &params->line_infos[line_idx];
                D_Line *line = 0;
                for(D_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  if(di_key_match(&n->v.dbgi_key, &dbgi_key))
                  {
                    line = &n->v;
                    break;
                  }
                }
                if(line != 0)
                {
                  Rng1U64 line_voff_rng = line->voff_range;
                  Vec4F32 weak_thread_color = color;
                  weak_thread_color.w *= 0.4f;
                  F32 progress_t = (line_voff_rng.max != line_voff_rng.min) ? ((F32)(thread_rip_voff - line_voff_rng.min) / (F32)(line_voff_rng.max - line_voff_rng.min)) : 0;
                  progress_t = Clamp(0, progress_t, 1);
                  u->progress_t = progress_t;
                }
              }
            }
            
            // rjf: interactions 
            if(ui_hovering(thread_sig) && !rd_drag_is_active())
            {
              RD_RegsScope(.thread = thread->handle) rd_set_hover_regs(RD_RegSlot_Thread);
            }
            if(ui_right_clicked(thread_sig))
            {
              RD_RegsScope(.thread = thread->handle) rd_open_ctx_menu(thread_box->key, v2f32(0, thread_box->rect.y1-thread_box->rect.y0), RD_RegSlot_Thread);
            }
            if(ui_dragging(thread_sig) && !contains_2f32(thread_box->rect, ui_mouse()))
            {
              RD_RegsScope(.thread = thread->handle) rd_drag_begin(RD_RegSlot_Thread);
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build catchall margin
  //
  UI_Box *catchall_margin_container_box = &ui_nil_box;
  if(params->flags & RD_CodeSliceFlag_CatchallMargin) UI_Focus(UI_FocusKind_Off) UI_Palette(margin_palette) UI_Parent(top_container_box) ProfScope("build catchall margins")
  {
    if(params->margin_float_off_px != 0)
    {
      ui_set_next_pref_width(ui_px(params->catchall_margin_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_build_box_from_key(0, ui_key_zero());
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px + params->priority_margin_width_px));
    }
    ui_set_next_pref_width(ui_px(params->catchall_margin_width_px, 1));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_child_layout_axis(Axis2_Y);
    catchall_margin_container_box = ui_build_box_from_string(UI_BoxFlag_DrawSideLeft|UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable), str8_lit("catchall_margin_container"));
    UI_Parent(catchall_margin_container_box) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Palette(margin_contents_palette)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        CTRL_EntityList line_ips  = params->line_ips[line_idx];
        RD_EntityList line_bps  = params->line_bps[line_idx];
        RD_EntityList line_pins = params->line_pins[line_idx];
        ui_set_next_hover_cursor(OS_Cursor_HandPoint);
        UI_Box *line_margin_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, "line_margin_%I64x", line_num);
        UI_Parent(line_margin_box)
        {
          //- rjf: build margin thread ip ui
          for(CTRL_EntityNode *n = line_ips.first; n != 0; n = n->next)
          {
            // rjf: unpack thread
            CTRL_Entity *thread = n->v;
            if(thread == selected_thread)
            {
              continue;
            }
            U64 unwind_count = (thread == selected_thread) ? rd_regs()->unwind_count : 0;
            U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_count);
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
            DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
            U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
            
            // rjf: thread info => color
            Vec4F32 color = rd_rgba_from_ctrl_entity(thread);
            {
              if(unwind_count != 0)
              {
                color = rd_rgba_from_theme_color(RD_ThemeColor_ThreadUnwound);
              }
              else if(thread == stopper_thread &&
                      (stop_event.cause == CTRL_EventCause_InterruptedByHalt ||
                       stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
                       stop_event.cause == CTRL_EventCause_InterruptedByException))
              {
                color = rd_rgba_from_theme_color(RD_ThemeColor_ThreadError);
              }
              if(d_ctrl_targets_running() && d_ctrl_last_run_frame_idx() < d_frame_index())
              {
                color.w *= 0.5f;
              }
              if(thread != selected_thread)
              {
                color.w *= 0.8f;
              }
            }
            
            // rjf: build thread box
            ui_set_next_hover_cursor(OS_Cursor_UpDownLeftRight);
            ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
            ui_set_next_font_size(params->font_size);
            ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
            ui_set_next_pref_width(ui_pct(1, 0));
            ui_set_next_pref_height(ui_pct(1, 0));
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Key thread_box_key = ui_key_from_stringf(top_container_box->key, "###ip_%I64x_catchall_%p", line_num, thread);
            UI_Box *thread_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|
                                                       UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|
                                                       UI_BoxFlag_DrawText,
                                                       thread_box_key);
            ui_box_equip_display_string(thread_box, rd_icon_kind_text_table[RD_IconKind_RightArrow]);
            UI_Signal thread_sig = ui_signal_from_box(thread_box);
            
            // rjf: custom draw
            {
              RD_Regs *hover_regs = rd_get_hover_regs();
              B32 is_hovering = (ctrl_handle_match(hover_regs->thread, thread->handle) &&
                                 rd_state->hover_regs_slot == RD_RegSlot_Thread);
              RD_ThreadBoxDrawExtData *u = push_array(ui_build_arena(), RD_ThreadBoxDrawExtData, 1);
              u->thread_color = color;
              u->alive_t      = ui_anim(ui_key_from_stringf(top_container_box->key, "###thread_alive_t_%p", thread), 1.f, .rate = entity_alive_t_rate);
              u->hover_t      = ui_anim(ui_key_from_stringf(top_container_box->key, "###thread_hover_t_%p", thread), (F32)!!is_hovering, .rate = entity_hover_t_rate);
              u->is_selected  = (thread == selected_thread);
              u->is_frozen    = !!thread->is_frozen;
              ui_box_equip_custom_draw(thread_box, rd_thread_box_draw_extensions, u);
              
              // rjf: fill out progress t (progress into range of current line's
              // voff range)
              if(params->line_vaddrs[line_idx] == 0 && params->line_infos[line_idx].first != 0)
              {
                D_LineList *lines = &params->line_infos[line_idx];
                D_Line *line = 0;
                for(D_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  if(di_key_match(&n->v.dbgi_key, &dbgi_key))
                  {
                    line = &n->v;
                    break;
                  }
                }
                if(line != 0)
                {
                  Rng1U64 line_voff_rng = line->voff_range;
                  Vec4F32 weak_thread_color = color;
                  weak_thread_color.w *= 0.4f;
                  F32 progress_t = (line_voff_rng.max != line_voff_rng.min) ? ((F32)(thread_rip_voff - line_voff_rng.min) / (F32)(line_voff_rng.max - line_voff_rng.min)) : 0;
                  progress_t = Clamp(0, progress_t, 1);
                  u->progress_t = progress_t;
                }
              }
            }
            
            // rjf: interactions
            if(ui_hovering(thread_sig) && !rd_drag_is_active())
            {
              RD_RegsScope(.thread = thread->handle) rd_set_hover_regs(RD_RegSlot_Thread);
            }
            if(ui_right_clicked(thread_sig))
            {
              RD_RegsScope(.thread = thread->handle) rd_open_ctx_menu(thread_box->key, v2f32(0, thread_box->rect.y1-thread_box->rect.y0), RD_RegSlot_Thread);
            }
            if(ui_dragging(thread_sig) && !contains_2f32(thread_box->rect, ui_mouse()))
            {
              RD_RegsScope(.thread = thread->handle) rd_drag_begin(RD_RegSlot_Thread);
            }
            if(ui_double_clicked(thread_sig))
            {
              rd_cmd(RD_CmdKind_SelectThread, .thread = thread->handle);
              ui_kill_action();
            }
          }
          
          //- rjf: build margin breakpoint ui
          for(RD_EntityNode *n = line_bps.first; n != 0; n = n->next)
          {
            RD_Entity *bp = n->entity;
            Vec4F32 bp_color = rd_rgba_from_theme_color(RD_ThemeColor_Breakpoint);
            if(bp->flags & RD_EntityFlag_HasColor)
            {
              bp_color = rd_rgba_from_entity(bp);
            }
            if(bp->disabled)
            {
              bp_color = v4f32(bp_color.x * 0.6f, bp_color.y * 0.6f, bp_color.z * 0.6f, bp_color.w * 0.6f);
            }
            
            // rjf: prep custom rendering data
            RD_BreakpointBoxDrawExtData *bp_draw = push_array(ui_build_arena(), RD_BreakpointBoxDrawExtData, 1);
            {
              RD_Regs *hover_regs = rd_get_hover_regs();
              B32 is_hovering = (rd_entity_from_handle(hover_regs->entity) == bp && rd_state->hover_regs_slot == RD_RegSlot_Entity);
              bp_draw->color    = bp_color;
              bp_draw->alive_t  = ui_anim(ui_key_from_stringf(ui_key_zero(), "bp_alive_t_%p", bp), 1.f, .rate = entity_alive_t_rate);
              bp_draw->hover_t  = ui_anim(ui_key_from_stringf(ui_key_zero(), "bp_hover_t_%p", bp), (F32)!!is_hovering, .rate = entity_hover_t_rate);
              bp_draw->do_lines = rd_setting_val_from_code(RD_SettingCode_BreakpointLines).s32;
              bp_draw->do_glow  = rd_setting_val_from_code(RD_SettingCode_BreakpointGlow).s32;
              if(params->line_vaddrs[line_idx] == 0)
              {
                D_LineList *lines = &params->line_infos[line_idx];
                for(D_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  S64 remap_line = n->v.pt.line;
                  if(remap_line != line_num)
                  {
                    bp_draw->remap_px_delta = (remap_line - line_num) * params->line_height_px;
                    break;
                  }
                }
              }
            }
            
            // rjf: build box for breakpoint
            ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = bp_color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *bp_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                       UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|
                                                       UI_BoxFlag_DisableTextTrunc,
                                                       "%S##bp_%p",
                                                       rd_icon_kind_text_table[RD_IconKind_CircleFilled],
                                                       bp);
            ui_box_equip_custom_draw(bp_box, rd_bp_box_draw_extensions, bp_draw);
            UI_Signal bp_sig = ui_signal_from_box(bp_box);
            
            // rjf: bp hovering
            if(ui_hovering(bp_sig) && !rd_drag_is_active())
            {
              RD_RegsScope(.entity = rd_handle_from_entity(bp)) rd_set_hover_regs(RD_RegSlot_Entity);
            }
            
            // rjf: shift+click => enable breakpoint
            if(ui_clicked(bp_sig) && bp_sig.event_flags & OS_Modifier_Shift)
            {
              rd_cmd(bp->disabled ? RD_CmdKind_EnableEntity : RD_CmdKind_DisableEntity, .entity = rd_handle_from_entity(bp));
            }
            
            // rjf: click => remove breakpoint
            if(ui_clicked(bp_sig) && bp_sig.event_flags == 0)
            {
              rd_cmd(RD_CmdKind_RemoveEntity, .entity = rd_handle_from_entity(bp));
            }
            
            // rjf: drag start
            if(ui_dragging(bp_sig) && !contains_2f32(bp_box->rect, ui_mouse()))
            {
              RD_RegsScope(.entity = rd_handle_from_entity(bp)) rd_drag_begin(RD_RegSlot_Entity);
            }
            
            // rjf: bp right-click menu
            if(ui_right_clicked(bp_sig))
            {
              RD_RegsScope(.entity = rd_handle_from_entity(bp)) rd_open_ctx_menu(bp_box->key, v2f32(0, bp_box->rect.y1-bp_box->rect.y0), RD_RegSlot_Entity);
            }
          }
          
          //- rjf: build margin watch pin ui
          for(RD_EntityNode *n = line_pins.first; n != 0; n = n->next)
          {
            RD_Entity *pin = n->entity;
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Text);
            if(pin->flags & RD_EntityFlag_HasColor)
            {
              color = rd_rgba_from_entity(pin);
            }
            
            // rjf: build box for watch
            ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *pin_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                        UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|
                                                        UI_BoxFlag_DisableTextTrunc,
                                                        "%S##watch_%p",
                                                        rd_icon_kind_text_table[RD_IconKind_Pin],
                                                        pin);
            UI_Signal pin_sig = ui_signal_from_box(pin_box);
            
            // rjf: watch hovering
            if(ui_hovering(pin_sig) && !rd_drag_is_active())
            {
              RD_RegsScope(.entity = rd_handle_from_entity(pin)) rd_set_hover_regs(RD_RegSlot_Entity);
            }
            
            // rjf: click => remove pin
            if(ui_clicked(pin_sig))
            {
              rd_cmd(RD_CmdKind_RemoveEntity, .entity = rd_handle_from_entity(pin));
            }
            
            // rjf: drag start
            if(ui_dragging(pin_sig) && !contains_2f32(pin_box->rect, ui_mouse()))
            {
              RD_RegsScope(.entity = rd_handle_from_entity(pin)) rd_drag_begin(RD_RegSlot_Entity);
            }
            
            // rjf: watch right-click menu
            if(ui_right_clicked(pin_sig))
            {
              RD_RegsScope(.entity = rd_handle_from_entity(pin)) rd_open_ctx_menu(pin_box->key, v2f32(0, pin_box->rect.y1-pin_box->rect.y0), RD_RegSlot_Entity);
            }
          }
        }
        
        // rjf: empty margin interaction
        UI_Signal line_margin_sig = ui_signal_from_box(line_margin_box);
        if(ui_clicked(line_margin_sig))
        {
          rd_cmd(RD_CmdKind_AddBreakpoint,
                 .file_path  = params->line_vaddrs[line_idx] ? str8_zero() : rd_regs()->file_path,
                 .cursor     = params->line_vaddrs[line_idx] ? txt_pt(0, 0) : txt_pt(line_num, 1),
                 .vaddr      = params->line_vaddrs[line_idx]);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build line numbers
  //
  if(params->flags & RD_CodeSliceFlag_LineNums) UI_Parent(top_container_box) ProfScope("build line numbers") UI_Focus(UI_FocusKind_Off)
  {
    TxtRng select_rng = txt_rng(*cursor, *mark);
    Vec4F32 active_color = rd_rgba_from_theme_color(RD_ThemeColor_CodeLineNumbersSelected);
    Vec4F32 inactive_color = rd_rgba_from_theme_color(RD_ThemeColor_CodeLineNumbers);
    ui_set_next_fixed_x(floor_f32(params->margin_float_off_px + params->priority_margin_width_px + params->catchall_margin_width_px));
    ui_set_next_pref_width(ui_px(params->line_num_width_px, 1.f));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_flags(UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight);
    UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      RD_Font(RD_FontSlot_Code)
      UI_FontSize(params->font_size)
      UI_CornerRadius(0)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        Vec4F32 text_color = (select_rng.min.line <= line_num && line_num <= select_rng.max.line) ? active_color : inactive_color;
        Vec4F32 bg_color = v4f32(0, 0, 0, 0);
        
        // rjf: line info on this line -> adjust bg color to visualize
        B32 has_line_info = 0;
        {
          U64 best_stamp = 0;
          S64 line_info_line_num = 0;
          F32 line_info_t = 0;
          D_LineList *lines = &params->line_infos[line_idx];
          for(D_LineNode *n = lines->first; n != 0; n = n->next)
          {
            if(n->v.dbgi_key.min_timestamp >= best_stamp)
            {
              has_line_info = (n->v.pt.line == line_num || params->line_vaddrs[line_idx] != 0);
              line_info_line_num = n->v.pt.line;
              best_stamp = n->v.dbgi_key.min_timestamp;
              line_info_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "dbgi_alive_t_%S", n->v.dbgi_key.path), 1.f);
            }
          }
          if(has_line_info)
          {
            Vec4F32 color = code_line_bgs[line_info_line_num % ArrayCount(code_line_bgs)];
            color.w *= line_info_t;
            bg_color = color;
          }
        }
        
        // rjf: build line num box
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = text_color, .background = bg_color));
        ui_build_box_from_stringf(UI_BoxFlag_DrawText|(UI_BoxFlag_DrawBackground*!!has_line_info), "%I64u##line_num", line_num);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build background for line numbers & margins
  //
  {
    UI_Parent(top_container_box) RD_Palette(RD_PaletteCode_Floating)
    {
      ui_set_next_pref_width(ui_px(params->priority_margin_width_px + params->catchall_margin_width_px + params->line_num_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px));
      ui_build_box_from_key(UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, ui_key_zero());
    }
  }
  
  //////////////////////////////
  //- rjf: build main text container box, for mouse interaction on both lines & line numbers
  //
  UI_Box *text_container_box = &ui_nil_box;
  UI_Parent(top_container_box) UI_Focus(UI_FocusKind_Off)
  {
    ui_set_next_hover_cursor(ctrlified ? OS_Cursor_HandPoint : OS_Cursor_IBar);
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    text_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable), str8_lit("text_container"));
  }
  
  //////////////////////////////
  //- rjf: determine starting offset for each at line, at which we can begin placing extra info to the right
  //
  F32 *line_extras_off = push_array(scratch.arena, F32, dim_1s64(params->line_num_range)+1);
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      String8 line_text = params->line_text[line_idx];
      F32 line_text_dim = fnt_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, line_text).x + params->line_num_width_px;
      line_extras_off[line_idx] = Max(line_text_dim, params->font_size*50);
    }
  }
  
  //////////////////////////////
  //- rjf: produce per-line extra annotation containers
  //
  UI_Box **line_extras_boxes = push_array(scratch.arena, UI_Box *, dim_1s64(params->line_num_range)+1);
  UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Parent(text_container_box) UI_Focus(UI_FocusKind_Off)
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      ui_set_next_fixed_x(line_extras_off[line_idx]);
      ui_set_next_fixed_y(line_idx*params->line_height_px);
      line_extras_boxes[line_idx] = ui_build_box_from_stringf(0, "###extras_%I64x", line_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build exception annotations
  //
  UI_Focus(UI_FocusKind_Off)
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      CTRL_EntityList threads = params->line_ips[line_idx];
      for(CTRL_EntityNode *n = threads.first; n != 0; n = n->next)
      {
        CTRL_Entity *thread = n->v;
        if(thread == stopper_thread &&
           (stop_event.cause == CTRL_EventCause_InterruptedByException ||
            stop_event.cause == CTRL_EventCause_InterruptedByTrap))
        {
          RD_IconKind icon = RD_IconKind_WarningBig;
          String8 explanation = rd_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon);
          UI_Parent(line_extras_boxes[line_idx]) UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_px(params->line_height_px, 1.f))
            UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative)))
          {
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder, "###exception_info");
            UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 1))
            {
              RD_Font(RD_FontSlot_Icons) ui_label(rd_icon_kind_text_table[RD_IconKind_WarningBig]);
              ui_label(explanation);
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build watch pin annotations
  //
  UI_Focus(UI_FocusKind_Off)
  {
    DI_Scope *scope = di_scope_open();
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      RD_EntityList pins = params->line_pins[line_idx];
      if(pins.count != 0) UI_Parent(line_extras_boxes[line_idx])
        RD_Font(RD_FontSlot_Code)
        UI_FontSize(params->font_size)
        UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      {
        for(RD_EntityNode *n = pins.first; n != 0; n = n->next)
        {
          RD_Entity *pin = n->entity;
          String8 pin_expr = pin->string;
          E_Eval eval = e_eval_from_string(scratch.arena, pin_expr);
          String8 eval_string = {0};
          if(!e_type_key_match(e_type_key_zero(), eval.type_key))
          {
            EV_ViewRuleList view_rules = {0};
            eval_string = rd_value_string_from_eval(scratch.arena, EV_StringFlag_ReadOnlyDisplayRules, 10, params->font, params->font_size, params->font_size*60.f, eval, 0, &view_rules);
          }
          ui_spacer(ui_em(1.5f, 1.f));
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Key pin_box_key = ui_key_from_stringf(ui_key_zero(), "###pin_%p", pin);
          UI_Box *pin_box = ui_build_box_from_key(UI_BoxFlag_AnimatePos|
                                                  UI_BoxFlag_Clickable*!!(params->flags & RD_CodeSliceFlag_Clickable)|
                                                  UI_BoxFlag_DrawHotEffects|
                                                  UI_BoxFlag_DrawBorder, pin_box_key);
          UI_Parent(pin_box) UI_PrefWidth(ui_text_dim(10, 1))
          {
            Vec4F32 pin_color = rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault);
            if(pin->flags & RD_EntityFlag_HasColor)
            {
              pin_color = rd_rgba_from_entity(pin);
            }
            UI_PrefWidth(ui_em(1.5f, 1.f))
              RD_Font(RD_FontSlot_Icons)
              UI_Palette(ui_build_palette(ui_top_palette(), .text = pin_color))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_Flags(UI_BoxFlag_DisableTextTrunc)
            {
              UI_Signal sig = ui_buttonf("%S###pin_nub", rd_icon_kind_text_table[RD_IconKind_Pin]);
              if(ui_dragging(sig) && !contains_2f32(sig.box->rect, ui_mouse()))
              {
                RD_RegsScope(.entity = rd_handle_from_entity(pin)) rd_drag_begin(RD_RegSlot_Entity);
              }
              if(ui_right_clicked(sig))
              {
                RD_RegsScope(.entity = rd_handle_from_entity(pin)) rd_open_ctx_menu(sig.box->key, v2f32(0, sig.box->rect.y1-sig.box->rect.y0), RD_RegSlot_Entity);
              }
            }
            rd_code_label(0.8f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), pin_expr);
            rd_code_label(0.6f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), eval_string);
          }
          UI_Signal pin_sig = ui_signal_from_box(pin_box);
          if(ui_key_match(pin_box_key, ui_hot_key()))
          {
            rd_set_hover_eval(v2f32(pin_box->rect.x0, pin_box->rect.y1-2.f), str8_zero(), txt_pt(1, 1), 0, pin_expr);
          }
        }
      }
    }
    di_scope_close(scope);
  }
  
  //////////////////////////////
  //- rjf: mouse -> text coordinates
  //
  TxtPt mouse_pt = {0};
  ProfScope("mouse -> text coordinates")
  {
    Vec2F32 mouse = ui_mouse();
    
    // rjf: mouse y => index
    U64 mouse_y_line_idx = (U64)((mouse.y - text_container_box->rect.y0) / params->line_height_px);
    
    // rjf: index => line num
    S64 line_num = (params->line_num_range.min + mouse_y_line_idx);
    String8 line_string = (params->line_num_range.min <= line_num && line_num <= params->line_num_range.max) ? (params->line_text[mouse_y_line_idx]) : str8_zero();
    
    // rjf: mouse x * string => column
    S64 column = fnt_char_pos_from_tag_size_string_p(params->font, params->font_size, 0, params->tab_size, line_string, mouse.x-text_container_box->rect.x0-params->line_num_width_px-line_num_padding_px)+1;
    
    // rjf: bundle
    mouse_pt = txt_pt(line_num, column);
    
    // rjf: clamp
    if(dim_1s64(params->line_num_range) > 0)
    {
      U64 last_line_size = params->line_text[dim_1s64(params->line_num_range)-1].size;
      TxtRng legal_pt_rng = txt_rng(txt_pt(params->line_num_range.min, 1),
                                    txt_pt(params->line_num_range.max, last_line_size+1));
      if(txt_pt_less_than(mouse_pt, legal_pt_rng.min))
      {
        mouse_pt = legal_pt_rng.min;
      }
      if(txt_pt_less_than(legal_pt_rng.max, mouse_pt))
      {
        mouse_pt = legal_pt_rng.max;
      }
    }
    else
    {
      mouse_pt = txt_pt(1, 1);
    }
    result.mouse_pt = mouse_pt;
  }
  
  //////////////////////////////
  //- rjf: mouse point -> mouse token range, mouse line range
  //
  TxtRng mouse_token_rng = txt_rng(mouse_pt, mouse_pt);
  TxtRng mouse_line_rng = txt_rng(mouse_pt, mouse_pt);
  if(contains_1s64(params->line_num_range, mouse_pt.line))
  {
    TXT_TokenArray *line_tokens = &params->line_tokens[mouse_pt.line-params->line_num_range.min];
    Rng1U64 line_range = params->line_ranges[mouse_pt.line-params->line_num_range.min];
    U64 mouse_pt_off = (mouse_pt.column-1) + line_range.min;
    for(U64 line_token_idx = 0; line_token_idx < line_tokens->count; line_token_idx += 1)
    {
      TXT_Token *line_token = &line_tokens->v[line_token_idx];
      if(contains_1u64(line_token->range, mouse_pt_off))
      {
        mouse_token_rng = txt_rng(txt_pt(mouse_pt.line, 1+line_token->range.min-line_range.min), txt_pt(mouse_pt.line, 1+line_token->range.max-line_range.min));
        break;
      }
    }
    mouse_line_rng = txt_rng(txt_pt(mouse_pt.line, 1), txt_pt(mouse_pt.line, 1+line_range.max));
  }
  
  //////////////////////////////
  //- rjf: interact with margin box & text box
  //
  UI_Signal priority_margin_container_sig = ui_signal_from_box(priority_margin_container_box);
  UI_Signal catchall_margin_container_sig = ui_signal_from_box(catchall_margin_container_box);
  UI_Signal text_container_sig = ui_signal_from_box(text_container_box);
  B32 line_drag_drop = 0;
  RD_Entity *line_drag_entity = &d_nil_entity;
  CTRL_Entity *line_drag_ctrl_entity = &ctrl_entity_nil;
  Vec4F32 line_drag_drop_color = rd_rgba_from_theme_color(RD_ThemeColor_DropSiteOverlay);
  {
    //- rjf: determine mouse drag range
    TxtRng mouse_drag_rng = txt_rng(mouse_pt, mouse_pt);
    if(text_container_sig.f & UI_SignalFlag_LeftTripleDragging)
    {
      mouse_drag_rng = mouse_line_rng;
    }
    else if(text_container_sig.f & UI_SignalFlag_LeftDoubleDragging)
    {
      mouse_drag_rng = mouse_token_rng;
    }
    
    //- rjf: clicking/dragging over the text container
    if(!ctrlified && ui_dragging(text_container_sig))
    {
      if(mouse_pt.line == 0)
      {
        mouse_pt.column = 1;
        if(ui_mouse().y <= top_container_box->rect.y0)
        {
          mouse_pt.line = params->line_num_range.min - 2;
        }
        else if(ui_mouse().y >= top_container_box->rect.y1)
        {
          mouse_pt.line = params->line_num_range.max + 2;
        }
      }
      if(ui_pressed(text_container_sig))
      {
        *cursor = mouse_drag_rng.max;
        *mark = mouse_drag_rng.min;
      }
      if(txt_pt_less_than(mouse_pt, *mark))
      {
        *cursor = mouse_drag_rng.min;
      }
      else
      {
        *cursor = mouse_drag_rng.max;
      }
      *preferred_column = cursor->column;
    }
    
    //- rjf: right-click => code context menu
    if(ui_right_clicked(text_container_sig))
    {
      if(txt_pt_match(*cursor, *mark))
      {
        *cursor = *mark = mouse_pt;
      }
      U64 vaddr = 0;
      D_LineList lines = {0};
      if(params->line_num_range.min <= cursor->line && cursor->line < params->line_num_range.max)
      {
        vaddr = params->line_vaddrs[cursor->line - params->line_num_range.min];
        lines = params->line_infos[cursor->line - params->line_num_range.min];
      }
      RD_RegsScope(.cursor = *cursor,
                   .mark   = *mark,
                   .vaddr  = vaddr,
                   .lines  = lines)
      {
        rd_open_ctx_menu(ui_key_zero(), sub_2f32(ui_mouse(), v2f32(2, 2)), RD_RegSlot_Cursor);
      }
    }
    
    //- rjf: dragging threads, breakpoints, or watch pins over this slice ->
    // drop target
    if(rd_drag_is_active() && contains_2f32(clipped_top_container_rect, ui_mouse()))
    {
      CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_state->drag_drop_regs->thread);
      RD_Entity *entity = rd_entity_from_handle(rd_state->drag_drop_regs->entity);
      if(rd_state->drag_drop_regs_slot == RD_RegSlot_Entity &&
         (entity->kind == RD_EntityKind_WatchPin ||
          entity->kind == RD_EntityKind_Breakpoint))
      {
        line_drag_drop = 1;
        line_drag_entity = entity;
        if(entity->flags & RD_EntityFlag_HasColor)
        {
          line_drag_drop_color = rd_rgba_from_entity(entity);
          line_drag_drop_color.w *= 0.5f;
        }
      }
      if(rd_state->drag_drop_regs_slot == RD_RegSlot_Thread)
      {
        line_drag_drop = 1;
        line_drag_ctrl_entity = thread;
        line_drag_drop_color = rd_rgba_from_ctrl_entity(thread);
        line_drag_drop_color.w *= 0.5f;
      }
    }
    
    //- rjf: drop target is dropped -> process
    {
      if(!rd_entity_is_nil(line_drag_entity) && rd_drag_drop() && contains_1s64(params->line_num_range, mouse_pt.line))
      {
        RD_Entity *dropped_entity = line_drag_entity;
        S64 line_num = mouse_pt.line;
        U64 line_idx = line_num - params->line_num_range.min;
        U64 line_vaddr = params->line_vaddrs[line_idx];
        rd_cmd(RD_CmdKind_RelocateEntity,
               .entity     = rd_handle_from_entity(dropped_entity),
               .file_path  = line_vaddr == 0 ? rd_regs()->file_path : str8_zero(),
               .cursor     = line_vaddr == 0 ? txt_pt(line_num, 1) : txt_pt(0, 0),
               .vaddr      = line_vaddr);
      }
      if(line_drag_ctrl_entity != &ctrl_entity_nil && rd_drag_drop() && contains_1s64(params->line_num_range, mouse_pt.line))
      {
        S64 line_num = mouse_pt.line;
        U64 line_idx = line_num - params->line_num_range.min;
        U64 line_vaddr = params->line_vaddrs[line_idx];
        CTRL_Entity *thread = line_drag_ctrl_entity;
        U64 new_rip_vaddr = line_vaddr;
        if(params->line_vaddrs[line_idx] == 0)
        {
          D_LineList *lines = &params->line_infos[line_idx];
          for(D_LineNode *n = lines->first; n != 0; n = n->next)
          {
            CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
            CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
            if(module != &ctrl_entity_nil)
            {
              new_rip_vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
              break;
            }
          }
        }
        rd_cmd(RD_CmdKind_SetThreadIP, .thread = thread->handle, .vaddr = new_rip_vaddr);
      }
    }
    
    //- rjf: commit text container signal to main output
    result.base = text_container_sig;
  }
  
  //////////////////////////////
  //- rjf: mouse -> expression range info
  //
  TxtRng mouse_expr_rng = {0};
  Vec2F32 mouse_expr_baseline_pos = {0};
  String8 mouse_expr = {0};
  B32 mouse_expr_is_explicit = 0;
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line)) ProfScope("mouse -> expression range")
  {
    TxtRng selected_rng = txt_rng(*cursor, *mark);
    if(!txt_pt_match(*cursor, *mark) && cursor->line == mark->line &&
       ((txt_pt_less_than(selected_rng.min, mouse_pt) || txt_pt_match(selected_rng.min, mouse_pt)) &&
        txt_pt_less_than(mouse_pt, selected_rng.max)))
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      F32 expr_hoff_px = params->line_num_width_px + fnt_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, str8_prefix(line_text, selected_rng.min.column-1)).x;
      result.mouse_expr_rng = mouse_expr_rng = selected_rng;
      mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                      text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
      mouse_expr = str8_substr(line_text, r1u64(selected_rng.min.column-1, selected_rng.max.column-1));
      mouse_expr_is_explicit = 1;
    }
    else
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      TXT_TokenArray line_tokens = params->line_tokens[line_slice_idx];
      Rng1U64 line_range = params->line_ranges[line_slice_idx];
      U64 mouse_pt_off = line_range.min + (mouse_pt.column-1);
      Rng1U64 expr_off_rng = txt_expr_off_range_from_line_off_range_string_tokens(mouse_pt_off, line_range, line_text, &line_tokens);
      if(expr_off_rng.max != expr_off_rng.min)
      {
        F32 expr_hoff_px = params->line_num_width_px + fnt_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, str8_prefix(line_text, expr_off_rng.min-line_range.min)).x;
        result.mouse_expr_rng = mouse_expr_rng = txt_rng(txt_pt(mouse_pt.line, 1+(expr_off_rng.min-line_range.min)), txt_pt(mouse_pt.line, 1+(expr_off_rng.max-line_range.min)));
        mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                        text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
        mouse_expr = str8_substr(line_text, r1u64(expr_off_rng.min-line_range.min, expr_off_rng.max-line_range.min));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: mouse -> set global frontend hovered line info
  //
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line) && (ui_mouse().x - text_container_box->rect.x0 < params->line_num_width_px + line_num_padding_px))
  {
    U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
    D_LineList *lines = &params->line_infos[line_slice_idx];
    if(lines->first != 0 && (params->line_vaddrs[line_slice_idx] != 0 || lines->first->v.pt.line == mouse_pt.line))
    {
      RD_RegsScope(.process     = selected_thread_process->handle,
                   .vaddr_range = ctrl_vaddr_range_from_voff_range(selected_thread_module, lines->first->v.voff_range),
                   .module      = selected_thread_module->handle,
                   .dbgi_key    = lines->first->v.dbgi_key,
                   .voff_range  = lines->first->v.voff_range)
      {
        rd_set_hover_regs(RD_RegSlot_Null);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: hover eval
  //
  if(!ui_dragging(text_container_sig) && text_container_sig.event_flags == 0 && mouse_expr.size != 0)
  {
    E_Eval eval = e_eval_from_string(scratch.arena, mouse_expr);
    if(eval.msgs.max_kind == E_MsgKind_Null && (eval.mode != E_Mode_Null || mouse_expr_is_explicit))
    {
      U64 line_vaddr = 0;
      if(contains_1s64(params->line_num_range, mouse_pt.line))
      {
        U64 line_idx = mouse_pt.line-params->line_num_range.min;
        line_vaddr = params->line_vaddrs[line_idx];
      }
      rd_set_hover_eval(mouse_expr_baseline_pos, rd_regs()->file_path, mouse_pt, line_vaddr, mouse_expr);
    }
  }
  
  //////////////////////////////
  //- rjf: dragging/dropping which applies to lines over this slice -> visualize
  //
  if(line_drag_drop && contains_2f32(clipped_top_container_rect, ui_mouse()))
  {
    DR_Bucket *bucket = dr_bucket_make();
    D_BucketScope(bucket)
    {
      Vec4F32 color = line_drag_drop_color;
      Rng2F32 drop_line_rect = r2f32p(top_container_box->rect.x0,
                                      top_container_box->rect.y0 + (mouse_pt.line - params->line_num_range.min) * params->line_height_px,
                                      top_container_box->rect.x1,
                                      top_container_box->rect.y0 + (mouse_pt.line - params->line_num_range.min + 1) * params->line_height_px);
      R_Rect2DInst *inst = dr_rect(pad_2f32(drop_line_rect, 8.f), color, 0, 0, 4.f);
      inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(color.x, color.y, color.z, 0);
    }
    ui_box_equip_draw_bucket(text_container_box, bucket);
  }
  
  //////////////////////////////
  //- rjf: (cursor*mark*list(flash_range)) -> list(text_range*color)
  //
  typedef struct TxtRngColorPairNode TxtRngColorPairNode;
  struct TxtRngColorPairNode
  {
    TxtRngColorPairNode *next;
    TxtRng rng;
    Vec4F32 color;
  };
  TxtRngColorPairNode *first_txt_rng_color_pair = 0;
  TxtRngColorPairNode *last_txt_rng_color_pair = 0;
  {
    // rjf: push initial for cursor/mark
    {
      TxtRngColorPairNode *n = push_array(scratch.arena, TxtRngColorPairNode, 1);
      n->rng = txt_rng(*cursor, *mark);
      n->color = ui_top_palette()->colors[UI_ColorCode_Selection];
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
    
    // rjf: push for ctrlified mouse expr
    if(ctrlified && !txt_pt_match(result.mouse_expr_rng.max, result.mouse_expr_rng.min))
    {
      TxtRngColorPairNode *n = push_array(scratch.arena, TxtRngColorPairNode, 1);
      n->rng = result.mouse_expr_rng;
      n->color = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
  }
  
  //////////////////////////////
  //- rjf: build line numbers region (line number interaction should be basically identical to lines)
  //
  if(params->flags & RD_CodeSliceFlag_LineNums) UI_Parent(text_container_box) ProfScope("build line number interaction box") UI_Focus(UI_FocusKind_Off)
  {
    ui_set_next_pref_width(ui_px(params->line_num_width_px, 1.f));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_build_box_from_key(0, ui_key_zero());
  }
  
  //////////////////////////////
  //- rjf: build line text
  //
  UI_Parent(text_container_box) ProfScope("build line text") UI_Focus(UI_FocusKind_Off)
  {
    RD_Regs *hover_regs = rd_get_hover_regs();
    Rng1U64 hover_voff_range = hover_regs->voff_range;
    if(hover_voff_range.min == 0 && hover_voff_range.max == 0)
    {
      CTRL_Entity *module = ctrl_entity_from_handle(d_state->ctrl_entity_store, hover_regs->module);
      hover_voff_range = ctrl_voff_range_from_vaddr_range(module, hover_regs->vaddr_range);
    }
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    UI_WidthFill
      UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      RD_Font(RD_FontSlot_Code)
      UI_FontSize(params->font_size)
      UI_CornerRadius(0)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max; line_num += 1, line_idx += 1)
      {
        String8 line_string = params->line_text[line_idx];
        Rng1U64 line_range = params->line_ranges[line_idx];
        TXT_TokenArray *line_tokens = &params->line_tokens[line_idx];
        ui_set_next_text_padding(line_num_padding_px);
        UI_Key line_key = ui_key_from_stringf(top_container_box->key, "ln_%I64x", line_num);
        Vec4F32 line_bg_color = line_bg_colors[line_idx];
        if(line_bg_color.w != 0)
        {
          ui_set_next_flags(UI_BoxFlag_DrawBackground);
          ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = line_bg_color));
        }
        ui_set_next_tab_size(params->tab_size);
        UI_Box *line_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|UI_BoxFlag_DrawText|UI_BoxFlag_DisableIDString, line_key);
        DR_Bucket *line_bucket = dr_bucket_make();
        dr_push_bucket(line_bucket);
        
        // rjf: string * tokens -> fancy string list
        DR_FancyStringList line_fancy_strings = {0};
        {
          if(line_tokens->count == 0)
          {
            DR_FancyString fstr =
            {
              params->font,
              line_string,
              rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault),
              params->font_size,
              0,
              0,
            };
            dr_fancy_string_list_push(scratch.arena, &line_fancy_strings, &fstr);
          }
          else
          {
            TXT_Token *line_tokens_first = line_tokens->v;
            TXT_Token *line_tokens_opl = line_tokens->v + line_tokens->count;
            for(TXT_Token *token = line_tokens_first; token < line_tokens_opl; token += 1)
            {
              // rjf: token -> token string
              String8 token_string = {0};
              {
                Rng1U64 token_range = r1u64(0, line_string.size);
                if(token->range.min > line_range.min)
                {
                  token_range.min += token->range.min-line_range.min;
                }
                if(token->range.max < line_range.max)
                {
                  token_range.max = token->range.max-line_range.min;
                }
                token_string = str8_substr(line_string, token_range);
              }
              
              // rjf: token -> token color
              Vec4F32 token_color = rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault);
              {
                RD_ThemeColor new_color_kind = rd_theme_color_from_txt_token_kind(token->kind);
                F32 mix_t = 1.f;
                if(token->kind == TXT_TokenKind_Identifier || token->kind == TXT_TokenKind_Keyword)
                {
                  B32 mapped_special = 0;
                  for(DI_KeyNode *n = params->relevant_dbgi_keys.first; n != 0; n = n->next)
                  {
                    DI_Key dbgi_key = n->v;
                    UI_Key dbgi_anim_key = ui_key_from_stringf(ui_key_zero(), "###dbgi_alive_t_%S", dbgi_key.path);
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, token_string);
                      if(voff != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = RD_ThemeColor_CodeSymbol;
                        mix_t = ui_anim(dbgi_anim_key, 1.f);
                      }
                    }
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 type_num = d_type_num_from_dbgi_key_name(&dbgi_key, token_string);
                      if(type_num != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = RD_ThemeColor_CodeType;
                        mix_t = ui_anim(dbgi_anim_key, 1.f);
                      }
                    }
                    break;
                  }
                  if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                  {
                    U64 local_num = e_num_from_string(e_parse_ctx->locals_map, token_string);
                    if(local_num != 0)
                    {
                      mapped_special = 1;
                      new_color_kind = RD_ThemeColor_CodeLocal;
                      mix_t = selected_thread_module_alive_t;
                    }
                  }
                  if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                  {
                    U64 member_num = e_num_from_string(e_parse_ctx->member_map, token_string);
                    if(member_num != 0)
                    {
                      mapped_special = 1;
                      new_color_kind = RD_ThemeColor_CodeLocal;
                      mix_t = selected_thread_module_alive_t;
                    }
                  }
                  if(!mapped_special)
                  {
                    U64 reg_num = e_num_from_string(e_parse_ctx->regs_map, token_string);
                    if(reg_num != 0)
                    {
                      mapped_special = 1;
                      new_color_kind = RD_ThemeColor_CodeRegister;
                      mix_t = selected_thread_arch_alive_t;
                    }
                  }
                  if(!mapped_special)
                  {
                    U64 alias_num = e_num_from_string(e_parse_ctx->reg_alias_map, token_string);
                    if(alias_num != 0)
                    {
                      mapped_special = 1;
                      new_color_kind = RD_ThemeColor_CodeRegister;
                      mix_t = selected_thread_arch_alive_t;
                    }
                  }
                }
                if(new_color_kind != RD_ThemeColor_Null)
                {
                  Vec4F32 t_color = rd_rgba_from_theme_color(new_color_kind);
                  token_color.x += (t_color.x - token_color.x) * mix_t;
                  token_color.y += (t_color.y - token_color.y) * mix_t;
                  token_color.z += (t_color.z - token_color.z) * mix_t;
                  token_color.w += (t_color.w - token_color.w) * mix_t;
                }
              }
              
              // rjf: push fancy string
              DR_FancyString fstr =
              {
                params->font,
                token_string,
                token_color,
                params->font_size,
                0,
                0,
              };
              dr_fancy_string_list_push(scratch.arena, &line_fancy_strings, &fstr);
            }
          }
        }
        
        // rjf: equip fancy strings to line box
        ui_box_equip_display_fancy_strings(line_box, &line_fancy_strings);
        
        // rjf: extra rendering for strings that are currently being searched for
        if(params->search_query.size != 0)
        {
          for(U64 needle_pos = 0; needle_pos < line_string.size;)
          {
            needle_pos = str8_find_needle(line_string, needle_pos, params->search_query, StringMatchFlag_CaseInsensitive);
            if(needle_pos < line_string.size)
            {
              Rng1U64 match_range = r1u64(needle_pos, needle_pos+params->search_query.size);
              Rng1F32 match_column_pixel_off_range =
              {
                fnt_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, match_range.min)).x,
                fnt_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, match_range.max)).x,
              };
              Rng2F32 match_rect =
              {
                line_box->rect.x0+line_num_padding_px+match_column_pixel_off_range.min,
                line_box->rect.y0,
                line_box->rect.x0+line_num_padding_px+match_column_pixel_off_range.max+2.f,
                line_box->rect.y1,
              };
              Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
              if(cursor->line == line_num && needle_pos+1 <= cursor->column && cursor->column < needle_pos+params->search_query.size+1)
              {
                color.x += (1.f - color.x) * 0.5f;
                color.y += (1.f - color.y) * 0.5f;
                color.z += (1.f - color.z) * 0.5f;
                color.w += (1.f - color.w) * 0.5f;
              }
              if(!is_focused)
              {
                color.w *= 0.5f;
              }
              dr_rect(match_rect, color, 4.f, 0, 1.f);
              needle_pos += 1;
            }
          }
        }
        
        // rjf: extra rendering for list(text_range*color)
        {
          U64 prev_line_size = (line_idx > 0) ? params->line_text[line_idx-1].size : 0;
          U64 next_line_size = (line_idx+1 < dim_1s64(params->line_num_range)) ? params->line_text[line_idx+1].size : 0;
          for(TxtRngColorPairNode *n = first_txt_rng_color_pair; n != 0; n = n->next)
          {
            TxtRng select_range = n->rng;
            TxtRng line_range = txt_rng(txt_pt(line_num, 1), txt_pt(line_num, line_string.size+1));
            TxtRng select_range_in_line = txt_rng_intersect(select_range, line_range);
            if(!txt_pt_match(select_range_in_line.min, select_range_in_line.max) &&
               txt_pt_less_than(select_range_in_line.min, select_range_in_line.max))
            {
              TxtRng prev_line_range = txt_rng(txt_pt(line_num-1, 1), txt_pt(line_num-1, prev_line_size+1));
              TxtRng next_line_range = txt_rng(txt_pt(line_num+1, 1), txt_pt(line_num+1, next_line_size+1));
              TxtRng select_range_in_prev_line = txt_rng_intersect(prev_line_range, select_range);
              TxtRng select_range_in_next_line = txt_rng_intersect(next_line_range, select_range);
              B32 prev_line_good = (!txt_pt_match(select_range_in_prev_line.min, select_range_in_prev_line.max) &&
                                    txt_pt_less_than(select_range_in_prev_line.min, select_range_in_prev_line.max));
              B32 next_line_good = (!txt_pt_match(select_range_in_next_line.min, select_range_in_next_line.max) &&
                                    txt_pt_less_than(select_range_in_next_line.min, select_range_in_next_line.max));
              Rng1S64 select_column_range_in_line =
              {
                (select_range.min.line == line_num) ? select_range.min.column : 1,
                (select_range.max.line == line_num) ? select_range.max.column : (S64)(line_string.size+1),
              };
              Rng1F32 select_column_pixel_off_range =
              {
                fnt_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, select_column_range_in_line.min-1)).x,
                fnt_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, select_column_range_in_line.max-1)).x,
              };
              Rng2F32 select_rect =
              {
                line_box->rect.x0+line_num_padding_px+select_column_pixel_off_range.min-2.f,
                floor_f32(line_box->rect.y0) - 1.f,
                line_box->rect.x0+line_num_padding_px+select_column_pixel_off_range.max+2.f,
                ceil_f32(line_box->rect.y1) + 1.f,
              };
              Vec4F32 color = n->color;
              if(!is_focused)
              {
                color.w *= 0.5f;
              }
              F32 rounded_radius = params->font_size*0.4f;
              R_Rect2DInst *inst = dr_rect(select_rect, color, rounded_radius, 0, 1);
              inst->corner_radii[Corner_00] = !prev_line_good || select_range_in_prev_line.min.column > select_range_in_line.min.column ? rounded_radius : 0.f;
              inst->corner_radii[Corner_10] = (!prev_line_good || select_range_in_line.max.column > select_range_in_prev_line.max.column || select_range_in_line.max.column < select_range_in_prev_line.min.column) ? rounded_radius : 0.f;
              inst->corner_radii[Corner_01] = (!next_line_good || select_range_in_next_line.min.column > select_range_in_line.min.column || select_range_in_next_line.max.column < select_range_in_line.min.column) ? rounded_radius : 0.f;
              inst->corner_radii[Corner_11] = !next_line_good || select_range_in_line.max.column > select_range_in_next_line.max.column ? rounded_radius : 0.f;
            }
          }
        }
        
        // rjf: extra rendering for cursor position
        if(cursor->line == line_num)
        {
          S64 column = cursor->column;
          Vec2F32 advance = fnt_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, column-1));
          F32 cursor_off_pixels = advance.x;
          F32 cursor_thickness = ClampBot(4.f, line_box->font_size/6.f);
          Rng2F32 cursor_rect =
          {
            ui_box_text_position(line_box).x+cursor_off_pixels-cursor_thickness/2.f,
            line_box->rect.y0-params->font_size*0.25f,
            ui_box_text_position(line_box).x+cursor_off_pixels+cursor_thickness/2.f,
            line_box->rect.y1+params->font_size*0.25f,
          };
          dr_rect(cursor_rect, rd_rgba_from_theme_color(is_focused ? RD_ThemeColor_Cursor : RD_ThemeColor_CursorInactive), 1.f, 0, 1.f);
        }
        
        // rjf: extra rendering for lines with line-info that match the hovered
        {
          B32 matches = 0;
          S64 line_info_line_num = 0;
          D_LineList *lines = &params->line_infos[line_idx];
          for(D_LineNode *n = lines->first; n != 0; n = n->next)
          {
            if((n->v.pt.line == line_num || params->line_vaddrs[line_idx] != 0) &&
               ((di_key_match(&n->v.dbgi_key, &hover_regs->dbgi_key) &&
                 n->v.voff_range.min <= hover_voff_range.min && hover_voff_range.min < n->v.voff_range.max) ||
                (params->line_vaddrs[line_idx] == hover_regs->vaddr_range.min && hover_regs->vaddr_range.min != 0)))
            {
              matches = 1;
              line_info_line_num = n->v.pt.line;
              break;
            }
          }
          
          // rjf: matches => highlight background
          if(matches)
          {
            Vec4F32 highlight_color = code_line_bgs[line_info_line_num % ArrayCount(code_line_bgs)];
            highlight_color.w *= 0.25f;
            dr_rect(line_box->rect, highlight_color, 0, 0, 0);
          }
        }
        
        // rjf: equip bucket
        if(line_bucket->passes.count != 0)
        {
          ui_box_equip_draw_bucket(line_box, line_bucket);
        }
        
        dr_pop_bucket();
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal RD_CodeSliceSignal
rd_code_slicef(RD_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  RD_CodeSliceSignal sig = rd_code_slice(params, cursor, mark, preferred_column, string);
  va_end(args);
  scratch_end(scratch);
  return sig;
}

internal B32
rd_do_txt_controls(TXT_TextInfo *info, String8 data, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column)
{
  Temp scratch = scratch_begin(0, 0);
  B32 change = 0;
  for(UI_Event *evt = 0; ui_next_event(&evt);)
  {
    if(evt->kind != UI_EventKind_Navigate && evt->kind != UI_EventKind_Edit)
    {
      continue;
    }
    B32 taken = 0;
    String8 line = txt_string_from_info_data_line_num(info, data, cursor->line);
    UI_TxtOp single_line_op = ui_single_line_txt_op_from_event(scratch.arena, evt, line, *cursor, *mark);
    
    //- rjf: invalid single-line op or endpoint units => try multiline
    if(evt->delta_unit == UI_EventDeltaUnit_Whole || single_line_op.flags & UI_TxtOpFlag_Invalid)
    {
      U64 line_count = info->lines_count;
      String8 prev_line = txt_string_from_info_data_line_num(info, data, cursor->line-1);
      String8 next_line = txt_string_from_info_data_line_num(info, data, cursor->line+1);
      Vec2S32 delta = evt->delta_2s32;
      
      //- rjf: wrap lines right
      if(evt->delta_unit != UI_EventDeltaUnit_Whole && delta.x > 0 && cursor->column == line.size+1 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = 1;
        *preferred_column = 1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: wrap lines left
      if(evt->delta_unit != UI_EventDeltaUnit_Whole && delta.x < 0 && cursor->column == 1 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = prev_line.size+1;
        *preferred_column = prev_line.size+1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (plain)
      if(evt->delta_unit == UI_EventDeltaUnit_Char && delta.y > 0 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = Min(*preferred_column, next_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (plain)
      if(evt->delta_unit == UI_EventDeltaUnit_Char && delta.y < 0 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = Min(*preferred_column, prev_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (chunk)
      if(evt->delta_unit == UI_EventDeltaUnit_Word && delta.y > 0 && cursor->line+1 <= line_count)
      {
        for(S64 line_num = cursor->line+1; line_num <= line_count; line_num += 1)
        {
          String8 line = txt_string_from_info_data_line_num(info, data, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == line_count)
          {
            cursor->line = line_num;
            cursor->column = line_size+1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (chunk)
      if(evt->delta_unit == UI_EventDeltaUnit_Word && delta.y < 0 && cursor->line-1 >= 1)
      {
        for(S64 line_num = cursor->line-1; line_num > 0; line_num -= 1)
        {
          String8 line = txt_string_from_info_data_line_num(info, data, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == 1)
          {
            cursor->line = line_num;
            cursor->column = 1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (page)
      if(evt->delta_unit == UI_EventDeltaUnit_Page && delta.y > 0)
      {
        cursor->line += line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (page)
      if(evt->delta_unit == UI_EventDeltaUnit_Page && delta.y < 0)
      {
        cursor->line -= line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (+)
      if(evt->delta_unit == UI_EventDeltaUnit_Whole && (delta.y > 0 || delta.x > 0))
      {
        *cursor = txt_pt(line_count, info->lines_count ? dim_1u64(info->lines_ranges[info->lines_count-1])+1 : 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (-)
      if(evt->delta_unit == UI_EventDeltaUnit_Whole && (delta.y < 0 || delta.x < 0))
      {
        *cursor = txt_pt(1, 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
      if(!(evt->flags & UI_EventFlag_KeepMark))
      {
        *mark = *cursor;
      }
    }
    
    //- rjf: valid single-line op => do single-line op
    else
    {
      *cursor = single_line_op.cursor;
      *mark = single_line_op.mark;
      *preferred_column = cursor->column;
      change = 1;
      taken = 1;
    }
    
    //- rjf: copy
    if(evt->flags & UI_EventFlag_Copy)
    {
      String8 text = txt_string_from_info_data_txt_rng(info, data, txt_rng(*cursor, *mark));
      os_set_clipboard_text(text);
      taken = 1;
    }
    
    //- rjf: consume
    if(taken)
    {
      ui_eat_event(evt);
    }
  }
  
  scratch_end(scratch);
  return change;
}

////////////////////////////////
//~ rjf: UI Widgets: Fancy Labels

internal UI_Signal
rd_label(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  typedef U32 StringPartFlags;
  enum
  {
    StringPartFlag_Code      = (1<<0),
    StringPartFlag_Underline = (1<<1),
    StringPartFlag_Bright    = (1<<2),
  };
  typedef struct StringPart StringPart;
  struct StringPart
  {
    StringPart *next;
    StringPartFlags flags;
    String8 string;
  };
  StringPart *first_part = 0;
  StringPart *last_part = 0;
  U64 active_part_start_idx = 0;
  StringPartFlags active_part_flags = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(idx == string.size)
    {
      StringPart *p = push_array(scratch.arena, StringPart, 1);
      p->flags = active_part_flags;
      p->string = str8_substr(string, r1u64(active_part_start_idx, idx));
      SLLQueuePush(first_part, last_part, p);
    }
    else if(string.str[idx] == '`')
    {
      StringPart *p = push_array(scratch.arena, StringPart, 1);
      p->flags = active_part_flags;
      p->string = str8_substr(string, r1u64(active_part_start_idx, idx));
      SLLQueuePush(first_part, last_part, p);
      active_part_start_idx = idx+1;
      active_part_flags ^= StringPartFlag_Code;
    }
  }
  DR_FancyStringList fstrs = {0};
  for(StringPart *p = first_part; p != 0; p = p->next)
  {
    DR_FancyString fstr = {0};
    {
      fstr.font   = ui_top_font();
      fstr.string = p->string;
      fstr.color  = ui_top_palette()->colors[UI_ColorCode_Text];
      fstr.size   = ui_top_font_size();
      if(p->flags & StringPartFlag_Code)
      {
        fstr.font = rd_font_from_slot(RD_FontSlot_Code);
        fstr.color = rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault);
      }
    }
    dr_fancy_string_list_push(scratch.arena, &fstrs, &fstr);
  }
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
  ui_box_equip_display_fancy_strings(box, &fstrs);
  UI_Signal sig = ui_signal_from_box(box);
  scratch_end(scratch);
  return sig;
}

internal UI_Signal
rd_error_label(String8 string)
{
  UI_Box *box = ui_build_box_from_key(0, ui_key_zero());
  UI_Signal sig = ui_signal_from_box(box);
  UI_Parent(box) UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative), .text_weak = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative)))
  {
    ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
    ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
    UI_PrefWidth(ui_em(2.25f, 1.f)) ui_label(rd_icon_kind_text_table[RD_IconKind_WarningBig]);
    UI_PrefWidth(ui_text_dim(10, 0)) rd_label(string);
  }
  return sig;
}

internal B32
rd_help_label(String8 string)
{
  B32 result = 0;
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%S_help_label", string);
  UI_Signal sig = ui_signal_from_box(box);
  UI_Parent(box)
  {
    UI_PrefWidth(ui_pct(1, 0)) ui_label(string);
    if(ui_hovering(sig)) UI_PrefWidth(ui_em(2.25f, 1))
    {
      result = 1;
      ui_set_next_font(rd_font_from_slot(RD_FontSlot_Icons));
      ui_set_next_text_raster_flags(FNT_RasterFlag_Smooth);
      ui_set_next_text_alignment(UI_TextAlign_Center);
      UI_Box *help_hoverer = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawHotEffects, "###help_hoverer_%S", string);
      ui_box_equip_display_string(help_hoverer, rd_icon_kind_text_table[RD_IconKind_QuestionMark]);
      if(!contains_2f32(help_hoverer->rect, ui_mouse()))
      {
        result = 0;
      }
    }
  }
  return result;
}

internal DR_FancyStringList
rd_fancy_string_list_from_code_string(Arena *arena, F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  DR_FancyStringList fancy_strings = {0};
  TXT_TokenArray tokens = txt_token_array_from_string__c_cpp(scratch.arena, 0, string);
  TXT_Token *tokens_opl = tokens.v+tokens.count;
  S32 indirection_counter = 0;
  indirection_size_change = 0;
  for(TXT_Token *token = tokens.v; token < tokens_opl; token += 1)
  {
    RD_ThemeColor token_color = rd_theme_color_from_txt_token_kind(token->kind);
    Vec4F32 token_color_rgba = rd_rgba_from_theme_color(token_color);
    token_color_rgba.w *= alpha;
    String8 token_string = str8_substr(string, token->range);
    if(str8_match(token_string, str8_lit("{"), 0)) { indirection_counter += 1; }
    if(str8_match(token_string, str8_lit("["), 0)) { indirection_counter += 1; }
    indirection_counter = ClampBot(0, indirection_counter);
    switch(token->kind)
    {
      default:
      {
        DR_FancyString fancy_string =
        {
          ui_top_font(),
          token_string,
          token_color_rgba,
          ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f)),
        };
        dr_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
      }break;
      case TXT_TokenKind_Identifier:
      {
        E_TypeKey type = e_leaf_type_from_name(token_string);
        Vec4F32 color = base_color;
        if(!e_type_key_match(e_type_key_zero(), type))
        {
          color = rd_rgba_from_theme_color(RD_ThemeColor_CodeType);
        }
        else
        {
          CTRL_Entity *module = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->module);
          DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
          U64 symbol_voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, token_string);
          if(symbol_voff != 0)
          {
            color = rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol);
          }
        }
        DR_FancyString fancy_string =
        {
          ui_top_font(),
          token_string,
          color,
          ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f)),
        };
        dr_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
      }break;
      case TXT_TokenKind_Numeric:
      {
        Vec4F32 token_color_rgba_alt = rd_rgba_from_theme_color(RD_ThemeColor_CodeNumericAltDigitGroup);
        token_color_rgba_alt.w *= alpha;
        F32 font_size = ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f));
        
        // rjf: unpack string
        U32 base = 10;
        U64 prefix_skip = 0;
        U64 digit_group_size = 3;
        if(str8_match(str8_prefix(token_string, 2), str8_lit("0x"), StringMatchFlag_CaseInsensitive))
        {
          base = 16;
          prefix_skip = 2;
          digit_group_size = 4;
        }
        else if(str8_match(str8_prefix(token_string, 2), str8_lit("0b"), StringMatchFlag_CaseInsensitive))
        {
          base = 2;
          prefix_skip = 2;
          digit_group_size = 8;
        }
        else if(str8_match(str8_prefix(token_string, 2), str8_lit("0o"), StringMatchFlag_CaseInsensitive))
        {
          base = 8;
          prefix_skip = 2;
          digit_group_size = 2;
        }
        
        // rjf: grab string parts
        U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
        String8 prefix = str8_prefix(token_string, prefix_skip);
        String8 whole = str8_substr(token_string, r1u64(prefix_skip, dot_pos));
        String8 decimal = str8_skip(token_string, dot_pos);
        
        // rjf: determine # of digits
        U64 num_digits = 0;
        for(U64 idx = 0; idx < whole.size; idx += 1)
        {
          num_digits += char_is_digit(whole.str[idx], base);
        }
        
        // rjf: push prefix
        {
          DR_FancyString fancy_string =
          {
            ui_top_font(),
            prefix,
            token_color_rgba,
            font_size,
          };
          dr_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
        }
        
        // rjf: push digit groups
        {
          B32 odd = 0;
          U64 start_idx = 0;
          U64 num_digits_passed = digit_group_size - num_digits%digit_group_size;
          for(U64 idx = 0; idx <= whole.size; idx += 1)
          {
            U8 byte = idx < whole.size ? whole.str[idx] : 0;
            if(num_digits_passed >= digit_group_size || idx == whole.size)
            {
              num_digits_passed = 0;
              if(start_idx < idx)
              {
                DR_FancyString fancy_string =
                {
                  ui_top_font(),
                  str8_substr(whole, r1u64(start_idx, idx)),
                  odd ? token_color_rgba_alt : token_color_rgba,
                  font_size,
                };
                dr_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
                start_idx = idx;
                odd ^= 1;
              }
            }
            if(char_is_digit(byte, base))
            {
              num_digits_passed += 1;
            }
          }
        }
        
        // rjf: push decimal
        {
          DR_FancyString fancy_string =
          {
            ui_top_font(),
            decimal,
            token_color_rgba,
            font_size,
          };
          dr_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
        }
        
      }break;
    }
    if(str8_match(token_string, str8_lit("}"), 0)) { indirection_counter -= 1; }
    if(str8_match(token_string, str8_lit("]"), 0)) { indirection_counter -= 1; }
    indirection_counter = ClampBot(0, indirection_counter);
  }
  scratch_end(scratch);
  return fancy_strings;
}

internal UI_Box *
rd_code_label(F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  DR_FancyStringList fancy_strings = rd_fancy_string_list_from_code_string(scratch.arena, alpha, indirection_size_change, base_color, string);
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
  ui_box_equip_display_fancy_strings(box, &fancy_strings);
  scratch_end(scratch);
  return box;
}

////////////////////////////////
//~ rjf: UI Widgets: Line Edit

internal UI_Signal
rd_line_edit(RD_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, String8 string)
{
  //- rjf: unpack visual metrics
  F32 expander_size_px = ui_top_font_size()*2.f;
  
  //- rjf: make key
  UI_Key key = ui_key_from_string(ui_active_seed_key(), string);
  
  //- rjf: calculate & push focus
  B32 is_auto_focus_hot = ui_is_key_auto_focus_hot(key);
  B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
  if(is_auto_focus_hot) { ui_push_focus_hot(UI_FocusKind_On); }
  if(is_auto_focus_active) { ui_push_focus_active(UI_FocusKind_On); }
  B32 is_focus_hot    = ui_is_focus_hot();
  B32 is_focus_active = ui_is_focus_active();
  B32 is_focus_hot_disabled = (!is_focus_hot && ui_top_focus_hot() == UI_FocusKind_On);
  B32 is_focus_active_disabled = (!is_focus_active && ui_top_focus_active() == UI_FocusKind_On);
  
  //- rjf: build top-level box
  if(is_focus_active || is_focus_active_disabled)
  {
    ui_set_next_hover_cursor(OS_Cursor_IBar);
  }
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                      (!!(flags & RD_LineEditFlag_KeyboardClickable)*UI_BoxFlag_KeyboardClickable)|
                                      UI_BoxFlag_ClickToFocus|
                                      UI_BoxFlag_DrawHotEffects|
                                      (!(flags & RD_LineEditFlag_NoBackground)*UI_BoxFlag_DrawBackground)|
                                      (!!(flags & RD_LineEditFlag_Border)*UI_BoxFlag_DrawBorder)|
                                      ((is_auto_focus_hot || is_auto_focus_active)*UI_BoxFlag_KeyboardClickable)|
                                      (is_focus_active || is_focus_active_disabled)*(UI_BoxFlag_Clip),
                                      key);
  
  //- rjf: build indent
  UI_Parent(box) for(S32 idx = 0; idx < depth; idx += 1)
  {
    ui_set_next_flags(UI_BoxFlag_DrawSideLeft);
    ui_spacer(ui_em(1.f, 1.f));
  }
  
  //- rjf: build expander
  if(flags & RD_LineEditFlag_Expander) UI_PrefWidth(ui_px(expander_size_px, 1.f)) UI_Parent(box)
    UI_Flags(UI_BoxFlag_DrawSideLeft)
    UI_Focus(UI_FocusKind_Off)
  {
    UI_Signal expander_sig = ui_expanderf(*expanded_out, "expander");
    if(ui_pressed(expander_sig))
    {
      *expanded_out ^= 1;
    }
  }
  
  //- rjf: build expander placeholder
  else if(flags & RD_LineEditFlag_ExpanderPlaceholder) UI_Parent(box) UI_PrefWidth(ui_px(expander_size_px, 1.f)) UI_Focus(UI_FocusKind_Off)
  {
    UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      UI_Flags(UI_BoxFlag_DrawSideLeft)
      RD_Font(RD_FontSlot_Icons)
      UI_TextAlignment(UI_TextAlign_Center)
      ui_label(rd_icon_kind_text_table[RD_IconKind_Dot]);
  }
  
  //- rjf: build expander space
  else if(flags & RD_LineEditFlag_ExpanderSpace) UI_Parent(box) UI_Focus(UI_FocusKind_Off)
  {
    UI_Flags(UI_BoxFlag_DrawSideLeft) ui_spacer(ui_px(expander_size_px, 1.f));
  }
  
  //- rjf: build scrollable container box
  UI_Box *scrollable_box = &ui_nil_box;
  UI_Parent(box) UI_PrefWidth(ui_children_sum(0))
  {
    scrollable_box = ui_build_box_from_stringf(is_focus_active*(UI_BoxFlag_AllowOverflowX), "scroll_box_%p", edit_buffer);
  }
  
  //- rjf: do non-textual edits (delete, copy, cut)
  B32 commit = 0;
  if(!is_focus_active && is_focus_hot)
  {
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if(evt->flags & UI_EventFlag_Copy)
      {
        os_set_clipboard_text(pre_edit_value);
      }
      if(evt->flags & UI_EventFlag_Delete)
      {
        commit = 1;
        edit_string_size_out[0] = 0;
      }
    }
  }
  
  //- rjf: get signal
  UI_Signal sig = ui_signal_from_box(box);
  if(commit)
  {
    sig.f |= UI_SignalFlag_Commit;
  }
  
  //- rjf: do start/end editing interaction
  B32 focus_started = 0;
  if(!is_focus_active)
  {
    B32 start_editing_via_sig = (ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed);
    B32 start_editing_via_typing = 0;
    if(is_focus_hot)
    {
      for(UI_Event *evt = 0; ui_next_event(&evt);)
      {
        if(evt->string.size != 0 || evt->flags & UI_EventFlag_Paste)
        {
          start_editing_via_typing = 1;
          break;
        }
      }
    }
    if(is_focus_hot && ui_slot_press(UI_EventActionSlot_Edit))
    {
      start_editing_via_typing = 1;
    }
    if(start_editing_via_sig || start_editing_via_typing)
    {
      String8 edit_string = pre_edit_value;
      edit_string.size = Min(edit_buffer_size, pre_edit_value.size);
      MemoryCopy(edit_buffer, edit_string.str, edit_string.size);
      edit_string_size_out[0] = edit_string.size;
      ui_set_auto_focus_active_key(key);
      ui_kill_action();
      *cursor = txt_pt(1, edit_string.size+1);
      *mark = txt_pt(1, 1);
      focus_started = 1;
    }
  }
  else if(is_focus_active && sig.f&UI_SignalFlag_KeyboardPressed)
  {
    ui_set_auto_focus_active_key(ui_key_zero());
    sig.f |= UI_SignalFlag_Commit;
  }
  
  //- rjf: determine autocompletion string
  String8 autocomplete_hint_string = {0};
  {
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if(evt->kind == UI_EventKind_AutocompleteHint)
      {
        autocomplete_hint_string = evt->string;
      }
    }
  }
  
  //- rjf: take navigation actions for editing
  B32 changes_made = 0;
  if(!(flags & RD_LineEditFlag_DisableEdit) && (is_focus_active || focus_started))
  {
    Temp scratch = scratch_begin(0, 0);
    rd_state->text_edit_mode = 1;
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
      
      // rjf: any valid op & autocomplete hint? -> perform autocomplete first, then re-compute op
      if(autocomplete_hint_string.size != 0)
      {
        String8 word_query = rd_autocomp_query_word_from_input_string_off(edit_string, cursor->column-1);
        U64 word_off = (U64)(word_query.str - edit_string.str);
        String8 new_string = ui_push_string_replace_range(scratch.arena, edit_string, r1s64(word_off+1, word_off+1+word_query.size), autocomplete_hint_string);
        new_string.size = Min(edit_buffer_size, new_string.size);
        MemoryCopy(edit_buffer, new_string.str, new_string.size);
        edit_string_size_out[0] = new_string.size;
        *cursor = *mark = txt_pt(1, word_off+1+autocomplete_hint_string.size);
        edit_string = str8(edit_buffer, edit_string_size_out[0]);
        op = ui_single_line_txt_op_from_event(scratch.arena, evt, edit_string, *cursor, *mark);
        MemoryZeroStruct(&autocomplete_hint_string);
      }
      
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
  
  //- rjf: build scrolled contents
  TxtPt mouse_pt = {0};
  F32 cursor_off = 0;
  UI_Parent(scrollable_box)
  {
    if(!is_focus_active && !is_focus_active_disabled && flags & RD_LineEditFlag_CodeContents)
    {
      String8 display_string = ui_display_part_from_key_string(string);
      if(!(flags & RD_LineEditFlag_PreferDisplayString) && pre_edit_value.size != 0)
      {
        display_string = pre_edit_value;
        UI_Box *box = rd_code_label(1.f, 1, ui_top_palette()->text, display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else if(flags & RD_LineEditFlag_DisplayStringIsCode)
      {
        UI_Box *box = rd_code_label(1.f, 1, ui_top_palette()->text, display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else
      {
        ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
        UI_Box *box = ui_label(display_string).box;
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
    }
    else if(!is_focus_active && !is_focus_active_disabled && !(flags & RD_LineEditFlag_CodeContents))
    {
      String8 display_string = ui_display_part_from_key_string(string);
      if(!(flags & RD_LineEditFlag_PreferDisplayString) && pre_edit_value.size != 0)
      {
        display_string = pre_edit_value;
      }
      else
      {
        ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
      }
      UI_Box *box = ui_label(display_string).box;
      if(matches != 0)
      {
        ui_box_equip_fuzzy_match_ranges(box, matches);
      }
    }
    else if((is_focus_active || is_focus_active_disabled) && flags & RD_LineEditFlag_CodeContents)
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      Temp scratch = scratch_begin(0, 0);
      F32 total_text_width = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (RD_LineEditFlag_Expander|RD_LineEditFlag_ExpanderSpace|RD_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      DR_FancyStringList code_fancy_strings = rd_fancy_string_list_from_code_string(scratch.arena, 1.f, 0, ui_top_palette()->text, edit_string);
      if(autocomplete_hint_string.size != 0)
      {
        String8 query_word = rd_autocomp_query_word_from_input_string_off(edit_string, cursor->column-1);
        String8 autocomplete_append_string = str8_skip(autocomplete_hint_string, query_word.size);
        U64 off = 0;
        U64 cursor_off = cursor->column-1;
        DR_FancyStringNode *prev_n = 0;
        for(DR_FancyStringNode *n = code_fancy_strings.first; n != 0; n = n->next)
        {
          if(off <= cursor_off && cursor_off <= off+n->v.string.size)
          {
            prev_n = n;
            break;
          }
          off += n->v.string.size;
        }
        {
          DR_FancyStringNode *autocomp_fstr_n = push_array(scratch.arena, DR_FancyStringNode, 1);
          DR_FancyString *fstr = &autocomp_fstr_n->v;
          fstr->font = ui_top_font();
          fstr->string = autocomplete_append_string;
          fstr->color = ui_top_palette()->text;
          fstr->color.w *= 0.5f;
          fstr->size = ui_top_font_size();
          autocomp_fstr_n->next = prev_n ? prev_n->next : 0;
          if(prev_n != 0)
          {
            prev_n->next = autocomp_fstr_n;
          }
          if(prev_n == 0)
          {
            code_fancy_strings.first = code_fancy_strings.last = autocomp_fstr_n;
          }
          if(prev_n != 0 && prev_n->next == 0)
          {
            code_fancy_strings.last = autocomp_fstr_n;
          }
          code_fancy_strings.node_count += 1;
          code_fancy_strings.total_size += autocomplete_hint_string.size;
          if(prev_n != 0 && cursor_off - off < prev_n->v.string.size)
          {
            String8 full_string = prev_n->v.string;
            U64 chop_amt = full_string.size - (cursor_off - off);
            prev_n->v.string = str8_chop(full_string, chop_amt);
            code_fancy_strings.total_size -= chop_amt;
            if(chop_amt != 0)
            {
              String8 post_cursor = str8_skip(full_string, cursor_off - off);
              DR_FancyStringNode *post_fstr_n = push_array(scratch.arena, DR_FancyStringNode, 1);
              DR_FancyString *post_fstr = &post_fstr_n->v;
              MemoryCopyStruct(post_fstr, &prev_n->v);
              post_fstr->string   = post_cursor;
              if(autocomp_fstr_n->next == 0)
              {
                code_fancy_strings.last = post_fstr_n;
              }
              post_fstr_n->next = autocomp_fstr_n->next;
              autocomp_fstr_n->next = post_fstr_n;
              code_fancy_strings.node_count += 1;
              code_fancy_strings.total_size += post_cursor.size;
            }
          }
        }
      }
      ui_box_equip_display_fancy_strings(editstr_box, &code_fancy_strings);
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), str8_prefix(edit_string, cursor->column-1)).x;
      scratch_end(scratch);
    }
    else if((is_focus_active || is_focus_active_disabled) && !(flags & RD_LineEditFlag_CodeContents))
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      F32 total_text_width = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (RD_LineEditFlag_Expander|RD_LineEditFlag_ExpanderSpace|RD_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
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
  
  //- rjf: click+drag
  if(is_focus_active && ui_dragging(sig))
  {
    if(ui_pressed(sig))
    {
      *mark = mouse_pt;
    }
    *cursor = mouse_pt;
  }
  if(!is_focus_active && is_focus_active_disabled && ui_pressed(sig))
  {
    *cursor = *mark = mouse_pt;
  }
  
  //- rjf: focus cursor
  {
    F32 visible_dim_px = dim_2f32(box->rect).x - expander_size_px - ui_top_font_size()*depth;
    if(visible_dim_px > 0)
    {
      Rng1F32 cursor_range_px  = r1f32(cursor_off-ui_top_font_size()*2.f, cursor_off+ui_top_font_size()*2.f);
      Rng1F32 visible_range_px = r1f32(scrollable_box->view_off_target.x, scrollable_box->view_off_target.x + visible_dim_px);
      cursor_range_px.min = ClampBot(0, cursor_range_px.min);
      cursor_range_px.max = ClampBot(0, cursor_range_px.max);
      F32 min_delta = cursor_range_px.min-visible_range_px.min;
      F32 max_delta = cursor_range_px.max-visible_range_px.max;
      min_delta = Min(min_delta, 0);
      max_delta = Max(max_delta, 0);
      scrollable_box->view_off_target.x += min_delta;
      scrollable_box->view_off_target.x += max_delta;
    }
    if(!is_focus_active && !is_focus_active_disabled)
    {
      scrollable_box->view_off_target.x = scrollable_box->view_off.x = 0;
    }
  }
  
  //- rjf: pop focus
  if(is_auto_focus_hot) { ui_pop_focus_hot(); }
  if(is_auto_focus_active) { ui_pop_focus_active(); }
  
  return sig;
}

internal UI_Signal
rd_line_editf(RD_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = rd_line_edit(flags, depth, matches, cursor, mark, edit_buffer, edit_buffer_size, edit_string_size_out, expanded_out, pre_edit_value, string);
  scratch_end(scratch);
  return sig;
}
