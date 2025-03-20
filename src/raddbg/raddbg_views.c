// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Code Views

internal void
rd_code_view_init(RD_CodeViewState *cv)
{
  ProfBeginFunction();
  if(cv->initialized == 0)
  {
    cv->initialized = 1;
    cv->preferred_column = 1;
    cv->find_text_arena = rd_push_view_arena();
    cv->center_cursor = 1;
    rd_store_view_loading_info(1, 0, 0);
  }
  ProfEnd();
}

internal RD_CodeViewBuildResult
rd_code_view_build(Arena *arena, RD_CodeViewState *cv, RD_CodeViewBuildFlags flags, Rng2F32 rect, String8 text_data, TXT_TextInfo *text_info, DASM_LineArray *dasm_lines, Rng1U64 dasm_vaddr_range, DI_Key dasm_dbgi_key)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////
  //- rjf: unpack state
  //
  UI_ScrollPt2 scroll_pos = rd_view_scroll_pos();
  
  //////////////////////////////
  //- rjf: extract invariants
  //
  FNT_Tag code_font = rd_font_from_slot(RD_FontSlot_Code);
  F32 code_font_size = rd_font_size_from_slot(RD_FontSlot_Code);
  F32 code_tab_size = fnt_column_size_from_tag_size(code_font, code_font_size)*rd_setting_u64_from_name(str8_lit("tab_width"));
  FNT_Metrics code_font_metrics = fnt_metrics_from_tag_size(code_font, code_font_size);
  F32 code_line_height = ceil_f32(fnt_line_height_from_metrics(&code_font_metrics) * 1.5f);
  F32 big_glyph_advance = fnt_dim_from_tag_size_string(code_font, code_font_size, 0, 0, str8_lit("H")).x;
  Vec2F32 panel_box_dim = dim_2f32(rect);
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 code_area_dim = v2f32(panel_box_dim.x - scroll_bar_dim, panel_box_dim.y - scroll_bar_dim);
  S64 num_possible_visible_lines = (S64)(code_area_dim.y/code_line_height)+1;
  CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  B32 do_line_numbers = rd_setting_b32_from_name(str8_lit("show_line_numbers"));
  
  //////////////////////////////
  //- rjf: unpack information about the viewed source file, if any
  //
  String8 file_path = rd_regs()->file_path;
  String8List file_path_possible_overrides = rd_possible_overrides_from_file_path(scratch.arena, file_path);
  
  //////////////////////////////
  //- rjf: process commands
  //
  for(RD_Cmd *cmd = 0; rd_next_view_cmd(&cmd);)
  {
    RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
    switch(kind)
    {
      default: break;
      case RD_CmdKind_GoToLine:
      {
        cv->goto_line_num = cmd->regs->cursor.line;
      }break;
      case RD_CmdKind_CenterCursor:
      {
        cv->center_cursor = 1;
      }break;
      case RD_CmdKind_ContainCursor:
      {
        cv->contain_cursor = 1;
      }break;
      case RD_CmdKind_Search:
      {
        arena_clear(cv->find_text_arena);
        cv->find_text_fwd = push_str8_copy(cv->find_text_arena, cmd->regs->string);
      }break;
      case RD_CmdKind_SearchBackwards:
      {
        arena_clear(cv->find_text_arena);
        cv->find_text_bwd = push_str8_copy(cv->find_text_arena, cmd->regs->string);
      }break;
      case RD_CmdKind_FindNext:
      {
        String8 string = rd_view_query_input();
        arena_clear(cv->find_text_arena);
        cv->find_text_fwd = push_str8_copy(cv->find_text_arena, string);
      }break;
      case RD_CmdKind_FindPrev:
      {
        String8 string = rd_view_query_input();
        arena_clear(cv->find_text_arena);
        cv->find_text_bwd = push_str8_copy(cv->find_text_arena, string);
      }break;
      case RD_CmdKind_ToggleWatchExpressionAtMouse:
      {
        cv->watch_expr_at_mouse = 1;
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: determine visible line range / count
  //
  Rng1S64 visible_line_num_range = r1s64(scroll_pos.y.idx + (S64)(scroll_pos.y.off) + 1 - !!(scroll_pos.y.off < 0),
                                         scroll_pos.y.idx + (S64)(scroll_pos.y.off) + 1 + num_possible_visible_lines);
  Rng1S64 target_visible_line_num_range = r1s64(scroll_pos.y.idx + 1,
                                                scroll_pos.y.idx + 1 + num_possible_visible_lines);
  U64 visible_line_count = 0;
  {
    visible_line_num_range.min = Clamp(1, visible_line_num_range.min, (S64)text_info->lines_count);
    visible_line_num_range.max = Clamp(1, visible_line_num_range.max, (S64)text_info->lines_count);
    visible_line_num_range.min = Max(1, visible_line_num_range.min);
    visible_line_num_range.max = Max(1, visible_line_num_range.max);
    target_visible_line_num_range.min = Clamp(1, target_visible_line_num_range.min, (S64)text_info->lines_count);
    target_visible_line_num_range.max = Clamp(1, target_visible_line_num_range.max, (S64)text_info->lines_count);
    target_visible_line_num_range.min = Max(1, target_visible_line_num_range.min);
    target_visible_line_num_range.max = Max(1, target_visible_line_num_range.max);
    visible_line_count = (U64)dim_1s64(visible_line_num_range)+1;
  }
  
  //////////////////////////////
  //- rjf: calculate scroll bounds
  //
  S64 line_size_x = 0;
  Rng1S64 scroll_idx_rng[Axis2_COUNT] = {0};
  {
    line_size_x = (text_info->lines_max_size*big_glyph_advance*3)/2;
    line_size_x = ClampBot(line_size_x, (S64)big_glyph_advance*120);
    line_size_x = ClampBot(line_size_x, (S64)code_area_dim.x);
    scroll_idx_rng[Axis2_X] = r1s64(0, line_size_x-(S64)code_area_dim.x);
    scroll_idx_rng[Axis2_Y] = r1s64(0, (S64)text_info->lines_count-1);
  }
  
  //////////////////////////////
  //- rjf: calculate line-range-dependent info
  //
  F32 line_num_width_px = 0;
  if(do_line_numbers)
  {
    line_num_width_px = floor_f32(big_glyph_advance * (log10(visible_line_num_range.max) + 3));
  }
  F32 priority_margin_width_px = 0;
  F32 catchall_margin_width_px = 0;
  if(flags & RD_CodeViewBuildFlag_Margins)
  {
    priority_margin_width_px = floor_f32(big_glyph_advance*3.5f);
    catchall_margin_width_px = floor_f32(big_glyph_advance*3.5f);
  }
  TXT_LineTokensSlice slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, text_info, text_data, visible_line_num_range);
  
  //////////////////////////////
  //- rjf: selection on single line, no query? -> set search text
  //
  if(rd_regs()->cursor.line == rd_regs()->mark.line)
  {
    RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
    RD_ViewState *vs = rd_view_state_from_cfg(view);
    if(!vs->query_is_selected)
    {
      RD_Cfg *query = rd_cfg_child_from_string_or_alloc(view, str8_lit("query"));
      RD_Cfg *input = rd_cfg_child_from_string_or_alloc(query, str8_lit("input"));
      String8 text = txt_string_from_info_data_txt_rng(text_info, text_data, txt_rng(rd_regs()->cursor, rd_regs()->mark));
      if(text.size < 256)
      {
        rd_cfg_new_replace(input, text);
      }
      else
      {
        rd_cfg_new_replace(input, str8_zero());
      }
    }
  }
  
  //////////////////////////////
  //- rjf: get active search query
  //
  String8 search_query = rd_view_query_input();
  B32 search_query_is_active = 0;
  
  //////////////////////////////
  //- rjf: prepare code slice info bundle, for the viewable region of text
  //
  RD_CodeSliceParams code_slice_params = {0};
  {
    // rjf: fill basics
    code_slice_params.flags = RD_CodeSliceFlag_Clickable;
    if(do_line_numbers)
    {
      code_slice_params.flags |= RD_CodeSliceFlag_LineNums;
    }
    if(flags & RD_CodeViewBuildFlag_Margins)
    {
      code_slice_params.flags |= RD_CodeSliceFlag_PriorityMargin|RD_CodeSliceFlag_CatchallMargin;
    }
    code_slice_params.line_num_range            = visible_line_num_range;
    code_slice_params.line_text                 = push_array(scratch.arena, String8, visible_line_count);
    code_slice_params.line_ranges               = push_array(scratch.arena, Rng1U64, visible_line_count);
    code_slice_params.line_tokens               = push_array(scratch.arena, TXT_TokenArray, visible_line_count);
    code_slice_params.line_bps                  = push_array(scratch.arena, RD_CfgList, visible_line_count);
    code_slice_params.line_ips                  = push_array(scratch.arena, CTRL_EntityList, visible_line_count);
    code_slice_params.line_pins                 = push_array(scratch.arena, RD_CfgList, visible_line_count);
    code_slice_params.line_vaddrs               = push_array(scratch.arena, U64, visible_line_count);
    code_slice_params.line_infos                = push_array(scratch.arena, D_LineList, visible_line_count);
    code_slice_params.font                      = code_font;
    code_slice_params.font_size                 = code_font_size;
    code_slice_params.tab_size                  = code_tab_size;
    code_slice_params.line_height_px            = code_line_height;
    code_slice_params.search_query              = search_query;
    code_slice_params.priority_margin_width_px  = priority_margin_width_px;
    code_slice_params.catchall_margin_width_px  = catchall_margin_width_px;
    code_slice_params.line_num_width_px         = line_num_width_px;
    code_slice_params.line_text_max_width_px    = (F32)line_size_x;
    code_slice_params.margin_float_off_px       = scroll_pos.x.idx + floor_f32(scroll_pos.x.off);
    
    // rjf: fill text info
    {
      S64 line_num = visible_line_num_range.min;
      U64 line_idx = visible_line_num_range.min-1;
      for(U64 visible_line_idx = 0;
          visible_line_idx < visible_line_count && line_idx < text_info->lines_count;
          visible_line_idx += 1, line_idx += 1, line_num += 1)
      {
        code_slice_params.line_text[visible_line_idx]   = str8_substr(text_data, text_info->lines_ranges[line_idx]);
        code_slice_params.line_ranges[visible_line_idx] = text_info->lines_ranges[line_idx];
        code_slice_params.line_tokens[visible_line_idx] = slice.line_tokens[visible_line_idx];
      }
    }
    
    // rjf: find visible breakpoints for source code
    if(!dasm_lines) ProfScope("find visible breakpoints for source code")
    {
      RD_CfgList bps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
      for(RD_CfgNode *n = bps.first; n != 0; n = n->next)
      {
        RD_Cfg *bp = n->v;
        RD_Location loc = rd_location_from_cfg(bp);
        if(visible_line_num_range.min <= loc.pt.line && loc.pt.line <= visible_line_num_range.max)
        {
          for(String8Node *override_n = file_path_possible_overrides.first;
              override_n != 0;
              override_n = override_n->next)
          {
            if(path_match_normalized(loc.file_path, override_n->string))
            {
              U64 slice_line_idx = (U64)(loc.pt.line-visible_line_num_range.min);
              rd_cfg_list_push(scratch.arena, &code_slice_params.line_bps[slice_line_idx], bp);
              break;
            }
          }
        }
      }
    }
    
    // rjf: find live threads mapping to source code
    if(!dasm_lines) ProfScope("find live threads mapping to this file")
    {
      CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
      CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
      for(CTRL_EntityNode *thread_n = threads.first; thread_n != 0; thread_n = thread_n->next)
      {
        CTRL_Entity *thread = thread_n->v;
        CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
        U64 unwind_count = (thread == selected_thread) ? rd_regs()->unwind_count : 0;
        U64 inline_depth = (thread == selected_thread) ? rd_regs()->inline_depth : 0;
        U64 rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_count);
        U64 last_inst_on_unwound_rip_vaddr = rip_vaddr - !!unwind_count;
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, last_inst_on_unwound_rip_vaddr);
        U64 rip_voff = ctrl_voff_from_vaddr(module, last_inst_on_unwound_rip_vaddr);
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
        D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff);
        for(D_LineNode *n = lines.first; n != 0; n = n->next)
        {
          if(visible_line_num_range.min <= n->v.pt.line && n->v.pt.line <= visible_line_num_range.max)
          {
            for(String8Node *override_n = file_path_possible_overrides.first;
                override_n != 0;
                override_n = override_n->next)
            {
              if(path_match_normalized(n->v.file_path, override_n->string))
              {
                U64 slice_line_idx = n->v.pt.line-visible_line_num_range.min;
                ctrl_entity_list_push(scratch.arena, &code_slice_params.line_ips[slice_line_idx], thread);
                break;
              }
            }
          }
        }
      }
    }
    
    // rjf: find visible watch pins for source code
    if(!dasm_lines) ProfScope("find visible watch pins for source code")
    {
      RD_CfgList wps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch_pin"));
      for(RD_CfgNode *n = wps.first; n != 0; n = n->next)
      {
        RD_Cfg *wp = n->v;
        RD_Location loc = rd_location_from_cfg(wp);
        if(visible_line_num_range.min <= loc.pt.line && loc.pt.line <= visible_line_num_range.max)
        {
          for(String8Node *override_n = file_path_possible_overrides.first;
              override_n != 0;
              override_n = override_n->next)
          {
            if(path_match_normalized(loc.file_path, override_n->string))
            {
              U64 slice_line_idx = (loc.pt.line-visible_line_num_range.min);
              rd_cfg_list_push(scratch.arena, &code_slice_params.line_pins[slice_line_idx], wp);
              break;
            }
          }
        }
      }
    }
    
    // rjf: find all src -> dasm info
    if(!dasm_lines) ProfScope("find all src -> dasm info for source code")
    {
      String8 file_path = rd_regs()->file_path;
      CTRL_Entity *module = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->module);
      DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
      D_LineListArray lines_array = d_lines_array_from_dbgi_key_file_path_line_range(scratch.arena, dbgi_key, file_path, visible_line_num_range);
      if(lines_array.count != 0)
      {
        MemoryCopy(code_slice_params.line_infos, lines_array.v, sizeof(D_LineList)*lines_array.count);
      }
      code_slice_params.relevant_dbgi_keys = lines_array.dbgi_keys;
    }
    
    // rjf: find live threads mapping to disasm
    if(dasm_lines) ProfScope("find live threads mapping to this disassembly")
    {
      CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
      CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
      for(CTRL_EntityNode *thread_n = threads.first; thread_n != 0; thread_n = thread_n->next)
      {
        CTRL_Entity *thread = thread_n->v;
        U64 unwind_count = (thread == selected_thread) ? rd_regs()->unwind_count : 0;
        U64 rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_count);
        if(ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process) == process && contains_1u64(dasm_vaddr_range, rip_vaddr))
        {
          U64 rip_off = rip_vaddr - dasm_vaddr_range.min;
          S64 line_num = dasm_line_array_idx_from_code_off__linear_scan(dasm_lines, rip_off)+1;
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            ctrl_entity_list_push(scratch.arena, &code_slice_params.line_ips[slice_line_idx], thread);
          }
        }
      }
    }
    
    // rjf: find breakpoints mapping to this disasm
    if(dasm_lines) ProfScope("find breakpoints mapping to this disassembly")
    {
      RD_CfgList bps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
      for(RD_CfgNode *n = bps.first; n != 0; n = n->next)
      {
        RD_Cfg *bp = n->v;
        RD_Location loc = rd_location_from_cfg(bp);
        E_Value loc_value = e_value_from_string(loc.expr);
        if(contains_1u64(dasm_vaddr_range, loc_value.u64))
        {
          U64 off = loc_value.u64 - dasm_vaddr_range.min;
          U64 idx = dasm_line_array_idx_from_code_off__linear_scan(dasm_lines, off);
          S64 line_num = (S64)idx+1;
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            rd_cfg_list_push(scratch.arena, &code_slice_params.line_bps[slice_line_idx], bp);
          }
        }
      }
    }
    
    // rjf: find watch pins mapping to this disasm
    if(dasm_lines) ProfScope("find watch pins mapping to this disassembly")
    {
      RD_CfgList wps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch_pin"));
      for(RD_CfgNode *n = wps.first; n != 0; n = n->next)
      {
        RD_Cfg *wp = n->v;
        RD_Location loc = rd_location_from_cfg(wp);
        E_Value loc_value = e_value_from_string(loc.expr);
        if(contains_1u64(dasm_vaddr_range, loc_value.u64))
        {
          U64 off = loc_value.u64 - dasm_vaddr_range.min;
          U64 idx = dasm_line_array_idx_from_code_off__linear_scan(dasm_lines, off);
          S64 line_num = (S64)idx+1;
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            rd_cfg_list_push(scratch.arena, &code_slice_params.line_pins[slice_line_idx], wp);
          }
        }
      }
    }
    
    // rjf: fill dasm -> src info
    if(dasm_lines)
    {
      CTRL_Entity *module = ctrl_module_from_process_vaddr(process, dasm_vaddr_range.min);
      DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
      for(S64 line_num = visible_line_num_range.min; line_num < visible_line_num_range.max; line_num += 1)
      {
        U64 vaddr = dasm_vaddr_range.min + dasm_line_array_code_off_from_idx(dasm_lines, line_num-1);
        U64 voff = ctrl_voff_from_vaddr(module, vaddr);
        U64 slice_idx = line_num-visible_line_num_range.min;
        code_slice_params.line_vaddrs[slice_idx] = vaddr;
        code_slice_params.line_infos[slice_idx] = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, voff);
      }
    }
    
    // rjf: add dasm dbgi key to relevant dbgis
    if(dasm_lines != 0)
    {
      di_key_list_push(scratch.arena, &code_slice_params.relevant_dbgi_keys, &dasm_dbgi_key);
    }
  }
  
  //////////////////////////////
  //- rjf: build container
  //
  UI_Box *container_box = &ui_nil_box;
  {
    ui_set_next_pref_width(ui_px(code_area_dim.x, 1));
    ui_set_next_pref_height(ui_px(code_area_dim.y, 1));
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                              UI_BoxFlag_Scroll|
                                              UI_BoxFlag_AllowOverflowX|
                                              UI_BoxFlag_AllowOverflowY,
                                              "###code_area");
  }
  
  //////////////////////////////
  //- rjf: cancelled search query -> center cursor
  //
  if(!search_query_is_active && cv->drifted_for_search)
  {
    cv->drifted_for_search = 0;
    cv->center_cursor = 1;
  }
  
  //////////////////////////////
  //- rjf: do searching operations
  //
  {
    //- rjf: find text (forward)
    if(cv->find_text_fwd.size != 0)
    {
      B32 found = 0;
      B32 first = 1;
      S64 line_num_start = rd_regs()->cursor.line;
      S64 line_num_last = (S64)text_info->lines_count;
      for(S64 line_num = line_num_start; 1 <= line_num && line_num <= line_num_last; first = 0)
      {
        // rjf: gather line info
        String8 line_string = str8_substr(text_data, text_info->lines_ranges[line_num-1]);
        U64 search_start = 0;
        if(rd_regs()->cursor.line == line_num && first)
        {
          search_start = rd_regs()->cursor.column;
        }
        
        // rjf: search string
        U64 needle_pos = str8_find_needle(line_string, search_start, cv->find_text_fwd, StringMatchFlag_CaseInsensitive);
        if(needle_pos < line_string.size)
        {
          rd_regs()->mark.line = line_num;
          rd_regs()->mark.column = needle_pos+1;
          rd_regs()->cursor = rd_regs()->mark;
          rd_regs()->cursor.column += cv->find_text_fwd.size;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num += 1;
        if(line_num > line_num_last)
        {
          line_num = 1;
        }
      }
      cv->center_cursor = found;
      if(found == 0)
      {
        log_user_errorf("Could not find `%S`", cv->find_text_fwd);
      }
    }
    
    //- rjf: find text (backward)
    if(cv->find_text_bwd.size != 0)
    {
      B32 found = 0;
      B32 first = 1;
      TxtRng rng = txt_rng(rd_regs()->cursor, rd_regs()->mark);
      S64 line_num_start = rng.min.line;
      S64 line_num_last = (S64)text_info->lines_count;
      for(S64 line_num = line_num_start; 1 <= line_num && line_num <= line_num_last; first = 0)
      {
        // rjf: gather line info
        String8 line_string = str8_substr(text_data, text_info->lines_ranges[line_num-1]);
        if(rng.min.line == line_num && first)
        {
          line_string = str8_prefix(line_string, rng.min.column-1);
        }
        
        // rjf: search string
        U64 next_needle_pos = line_string.size;
        for(U64 needle_pos = 0; needle_pos < line_string.size;)
        {
          needle_pos = str8_find_needle(line_string, needle_pos, cv->find_text_bwd, StringMatchFlag_CaseInsensitive);
          if(needle_pos < line_string.size)
          {
            next_needle_pos = needle_pos;
            needle_pos += 1;
          }
        }
        if(next_needle_pos < line_string.size)
        {
          rd_regs()->mark.line = line_num;
          rd_regs()->mark.column = next_needle_pos+1;
          rd_regs()->cursor = rd_regs()->mark;
          rd_regs()->cursor.column += cv->find_text_bwd.size;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor line
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num -= 1;
        if(line_num == 0)
        {
          line_num = line_num_last;
        }
      }
      cv->center_cursor = found;
      if(found == 0)
      {
        log_user_errorf("Could not find `%S`", cv->find_text_bwd);
      }
    }
    
    MemoryZeroStruct(&cv->find_text_fwd);
    MemoryZeroStruct(&cv->find_text_bwd);
    arena_clear(cv->find_text_arena);
  }
  
  //////////////////////////////
  //- rjf: do goto line
  //
  if(cv->goto_line_num != 0 && text_info->lines_count != 0)
  {
    S64 line_num = cv->goto_line_num;
    cv->goto_line_num = 0;
    line_num = Clamp(1, line_num, text_info->lines_count);
    rd_regs()->cursor = rd_regs()->mark = txt_pt(line_num, 1);
    cv->center_cursor = !cv->contain_cursor || (line_num < target_visible_line_num_range.min+4 || target_visible_line_num_range.max-4 < line_num);
  }
  
  //////////////////////////////
  //- rjf: do keyboard interaction
  //
  B32 snap[Axis2_COUNT] = {0};
  UI_Focus(UI_FocusKind_On)
  {
    if(ui_is_focus_active() && visible_line_num_range.max >= visible_line_num_range.min)
    {
      snap[Axis2_X] = snap[Axis2_Y] = rd_do_txt_controls(text_info, text_data, ClampBot(num_possible_visible_lines, 10) - 10, &rd_regs()->cursor, &rd_regs()->mark, &cv->preferred_column);
    }
  }
  
  //////////////////////////////
  //- rjf: build container contents
  //
  UI_Parent(container_box)
  {
    //- rjf: build fractional space
    container_box->view_off.x = container_box->view_off_target.x = scroll_pos.x.idx + scroll_pos.x.off;
    container_box->view_off.y = container_box->view_off_target.y = code_line_height*mod_f32(scroll_pos.y.off, 1.f) + code_line_height*(scroll_pos.y.off < 0) - code_line_height*(scroll_pos.y.off == -1.f && scroll_pos.y.idx == 1);
    
    //- rjf: build code slice
    RD_CodeSliceSignal sig = {0};
    UI_Focus(UI_FocusKind_On)
    {
      sig = rd_code_slicef(&code_slice_params, &rd_regs()->cursor, &rd_regs()->mark, &cv->preferred_column, "code_slice");
    }
    
    //- rjf: press code slice? -> focus panel
    if(ui_pressed(sig.base))
    {
      rd_cmd(RD_CmdKind_FocusPanel);
    }
    
    //- rjf: dragging & outside region? -> contain cursor
    if(ui_dragging(sig.base) && sig.base.event_flags == 0)
    {
      if(!contains_2f32(sig.base.box->rect, ui_mouse()))
      {
        cv->contain_cursor = 1;
      }
      else
      {
        snap[Axis2_X] = 1;
      }
    }
    
    //- rjf: ctrl+pressed? -> go to name
    if(ui_pressed(sig.base) && sig.base.event_flags & OS_Modifier_Ctrl)
    {
      ui_kill_action();
      rd_cmd(RD_CmdKind_GoToName, .string = txt_string_from_info_data_txt_rng(text_info, text_data, sig.mouse_expr_rng));
    }
    
    //- rjf: watch expr at mouse
    if(cv->watch_expr_at_mouse)
    {
      cv->watch_expr_at_mouse = 0;
      rd_cmd(RD_CmdKind_ToggleWatchExpression, .string = txt_string_from_info_data_txt_rng(text_info, text_data, sig.mouse_expr_rng));
    }
  }
  
  //////////////////////////////
  //- rjf: apply post-build view snapping rules
  //
  if(text_info->lines_count != 0)
  {
    TxtPt cursor = rd_regs()->cursor;
    B32 cursor_in_range = (1 <= cursor.line && cursor.line <= text_info->lines_count);
    
    // rjf: contain => snap
    if(cv->contain_cursor && text_info->lines_count != 0)
    {
      cv->contain_cursor = 0;
      snap[Axis2_X] = 1;
      snap[Axis2_Y] = 1;
    }
    
    // rjf: center cursor
    if(cv->center_cursor && text_info->lines_count != 0)
    {
      cv->center_cursor = 0;
      if(cursor_in_range)
      {
        String8 cursor_line = str8_substr(text_data, text_info->lines_ranges[cursor.line-1]);
        F32 cursor_advance = fnt_dim_from_tag_size_string(code_font, code_font_size, 0, code_tab_size, str8_prefix(cursor_line, cursor.column-1)).x;
        
        // rjf: scroll x
        {
          S64 new_idx = (S64)(cursor_advance - code_area_dim.x/2);
          new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
          ui_scroll_pt_target_idx(&scroll_pos.x, new_idx);
          snap[Axis2_X] = 0;
        }
        
        // rjf: scroll y
        {
          S64 new_idx = (cursor.line-1) - num_possible_visible_lines/2 + 2;
          new_idx = Clamp(scroll_idx_rng[Axis2_Y].min, new_idx, scroll_idx_rng[Axis2_Y].max);
          ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
          snap[Axis2_Y] = 0;
        }
      }
    }
    
    // rjf: snap in X
    if(snap[Axis2_X] && cursor_in_range)
    {
      String8 cursor_line = str8_substr(text_data, text_info->lines_ranges[cursor.line-1]);
      S64 cursor_off = (S64)(fnt_dim_from_tag_size_string(code_font, code_font_size, 0, code_tab_size, str8_prefix(cursor_line, cursor.column-1)).x + priority_margin_width_px + catchall_margin_width_px + line_num_width_px);
      Rng1S64 visible_pixel_range =
      {
        scroll_pos.x.idx,
        scroll_pos.x.idx + (S64)code_area_dim.x,
      };
      Rng1S64 cursor_pixel_range =
      {
        cursor_off - (S64)(big_glyph_advance*4) - (S64)(priority_margin_width_px + catchall_margin_width_px + line_num_width_px),
        cursor_off + (S64)(big_glyph_advance*4),
      };
      S64 min_delta = Min(0, cursor_pixel_range.min - visible_pixel_range.min);
      S64 max_delta = Max(0, cursor_pixel_range.max - visible_pixel_range.max);
      S64 new_idx = scroll_pos.x.idx+min_delta+max_delta;
      new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
      ui_scroll_pt_target_idx(&scroll_pos.x, new_idx);
    }
    
    // rjf: snap in Y
    if(snap[Axis2_Y])
    {
      Rng1S64 cursor_visibility_range = r1s64(cursor.line-4, cursor.line+4);
      cursor_visibility_range.min = ClampBot(0, cursor_visibility_range.min);
      cursor_visibility_range.max = ClampBot(0, cursor_visibility_range.max);
      S64 min_delta = Min(0, cursor_visibility_range.min-(target_visible_line_num_range.min));
      S64 max_delta = Max(0, cursor_visibility_range.max-(target_visible_line_num_range.min+num_possible_visible_lines));
      S64 new_idx = scroll_pos.y.idx+min_delta+max_delta;
      new_idx = Clamp(0, new_idx, (S64)text_info->lines_count-1);
      ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build horizontal scroll bar
  //
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y);
    ui_set_next_fixed_width(panel_box_dim.x - scroll_bar_dim);
    ui_set_next_fixed_height(scroll_bar_dim);
    {
      scroll_pos.x = ui_scroll_bar(Axis2_X,
                                   ui_px(scroll_bar_dim, 1.f),
                                   scroll_pos.x,
                                   scroll_idx_rng[Axis2_X],
                                   (S64)code_area_dim.x);
    }
  }
  
  //////////////////////////////
  //- rjf: build vertical scroll bar
  //
  {
    ui_set_next_fixed_x(code_area_dim.x);
    ui_set_next_fixed_y(0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(panel_box_dim.y - scroll_bar_dim);
    {
      scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                   ui_px(scroll_bar_dim, 1.f),
                                   scroll_pos.y,
                                   scroll_idx_rng[Axis2_Y],
                                   num_possible_visible_lines);
    }
  }
  
  //////////////////////////////
  //- rjf: top-level container interaction (scrolling)
  //
  if(text_info->lines_count != 0)
  {
    UI_Signal sig = ui_signal_from_box(container_box);
    if(sig.scroll.x != 0)
    {
      S64 new_idx = scroll_pos.x.idx+sig.scroll.x*big_glyph_advance;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_X], new_idx);
      ui_scroll_pt_target_idx(&scroll_pos.x, new_idx);
    }
    if(sig.scroll.y != 0)
    {
      S64 new_idx = scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_Y], new_idx);
      ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
    }
    ui_scroll_pt_clamp_idx(&scroll_pos.x, scroll_idx_rng[Axis2_X]);
    ui_scroll_pt_clamp_idx(&scroll_pos.y, scroll_idx_rng[Axis2_Y]);
    if(ui_mouse_over(sig))
    {
      for(UI_Event *evt = 0; ui_next_event(&evt);)
      {
        if(evt->kind == UI_EventKind_Scroll && evt->modifiers & OS_Modifier_Ctrl)
        {
          ui_eat_event(evt);
          if(evt->delta_2f32.y < 0)
          {
            rd_cmd(RD_CmdKind_IncCodeFontScale);
          }
          else if(evt->delta_2f32.y > 0)
          {
            rd_cmd(RD_CmdKind_DecCodeFontScale);
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build result
  //
  RD_CodeViewBuildResult result = {0};
  {
    for(DI_KeyNode *n = code_slice_params.relevant_dbgi_keys.first; n != 0; n = n->next)
    {
      DI_Key copy = di_key_copy(arena, &n->v);
      di_key_list_push(arena, &result.dbgi_keys, &copy);
    }
  }
  
  //////////////////////////////
  //- rjf: store state
  //
  rd_store_view_scroll_pos(scroll_pos);
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Watch Views

//- rjf: cell list building

internal U64
rd_id_from_watch_cell(RD_WatchCell *cell)
{
  U64 result = 5381;
  result = e_hash_from_string(result, str8_struct(&cell->kind));
  if(cell->kind != RD_WatchCellKind_Expr)
  {
    result = e_hash_from_string(result, str8_struct(&cell->eval.irtree.mode));
    result = e_hash_from_string(result, str8_struct(&cell->index));
    result = e_hash_from_string(result, str8_struct(&cell->default_pct));
  }
  return result;
}

internal RD_WatchCell *
rd_watch_cell_list_push(Arena *arena, RD_WatchCellList *list)
{
  RD_WatchCell *cell = push_array(arena, RD_WatchCell, 1);
  cell->index = list->count;
  SLLQueuePush(list->first, list->last, cell);
  list->count += 1;
  return cell;
}

internal RD_WatchCell *
rd_watch_cell_list_push_new_(Arena *arena, RD_WatchCellList *list, RD_WatchCell *params)
{
  RD_WatchCell *cell = rd_watch_cell_list_push(arena, list);
  U64 index = cell->index;
  MemoryCopyStruct(cell, params);
  cell->index = index;
  if(cell->pct == 0)
  {
    cell->pct = cell->default_pct;
  }
  cell->next = 0;
  return cell;
}

//- rjf: watch view points <-> table coordinates

internal B32
rd_watch_pt_match(RD_WatchPt a, RD_WatchPt b)
{
  return (ev_key_match(a.parent_key, b.parent_key) &&
          ev_key_match(a.key, b.key) &&
          a.cell_id == b.cell_id);
}

internal RD_WatchPt
rd_watch_pt_from_tbl(EV_BlockRangeList *block_ranges, Vec2S64 tbl)
{
  RD_WatchPt pt = zero_struct;
  {
    Temp scratch = scratch_begin(0, 0);
    EV_Row *row = ev_row_from_num(scratch.arena, rd_view_eval_view(), rd_view_query_input(), block_ranges, (U64)tbl.y);
    RD_WatchRowInfo row_info = rd_watch_row_info_from_row(scratch.arena, row);
    {
      S64 x = 0;
      for(RD_WatchCell *cell = row_info.cells.first; cell != 0; cell = cell->next, x += 1)
      {
        if(x == tbl.x)
        {
          pt.cell_id = rd_id_from_watch_cell(cell);
          break;
        }
      }
    }
    pt.key         = row->key;
    pt.parent_key  = row->block->key;
    scratch_end(scratch);
  }
  return pt;
}

internal Vec2S64
rd_tbl_from_watch_pt(EV_BlockRangeList *block_ranges, RD_WatchPt pt)
{
  Vec2S64 tbl = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    U64 num = ev_num_from_key(block_ranges, pt.key);
    EV_Row *row = ev_row_from_num(scratch.arena, rd_view_eval_view(), rd_view_query_input(), block_ranges, num);
    RD_WatchRowInfo row_info = rd_watch_row_info_from_row(scratch.arena, row);
    tbl.x = 0;
    {
      S64 x = 0;
      for(RD_WatchCell *cell = row_info.cells.first; cell != 0; cell = cell->next, x += 1)
      {
        U64 cell_id = rd_id_from_watch_cell(cell);
        if(cell_id == pt.cell_id)
        {
          tbl.x = x;
          break;
        }
      }
    }
    tbl.y = (S64)num;
    scratch_end(scratch);
  }
  return tbl;
}

//- rjf: row -> info

internal RD_WatchRowInfo
rd_watch_row_info_from_row(Arena *arena, EV_Row *row)
{
  RD_WatchRowInfo info =
  {
    .module           = &ctrl_entity_nil,
    .can_expand       = ev_row_is_expandable(row),
    .group_cfg_parent = &rd_nil_cfg,
    .group_cfg_child  = &rd_nil_cfg,
    .group_entity     = &ctrl_entity_nil,
    .callstack_thread = &ctrl_entity_nil,
    .view_ui_rule     = &rd_nil_view_ui_rule,
    .view_ui_tag      = &e_expr_nil,
  };
  {
    Temp scratch = scratch_begin(&arena, 1);
    DI_Scope *di_scope = di_scope_open();
    
    // rjf: unpack key & block
    EV_Block *block = row->block;
    EV_Key key = row->key;
    E_IRTreeAndType parent_irtree = e_irtree_and_type_from_expr(scratch.arena, block->expr);
    E_Type *parent_type = e_type_from_key__cached(parent_irtree.type_key);
    E_Eval block_eval = e_eval_from_expr(scratch.arena, row->block->expr);
    E_TypeKey block_type_key = block_eval.irtree.type_key;
    E_TypeKind block_type_kind = e_type_kind_from_key(block_type_key);
    E_Type *block_type = e_type_from_key__cached(block_type_key);
    
    // rjf: fill row's eval
    info.eval = e_eval_from_expr(arena, row->expr);
    
    // rjf: determine if row's expression is editable
    if(block_type->flags & E_TypeFlag_EditableChildren || row->expr == &e_expr_nil)
    {
      info.expr_is_editable = 1;
    }
    
    // rjf: determine row's module
    CTRL_Entity *row_ctrl_entity = rd_ctrl_entity_from_eval_space(info.eval.space);
    CTRL_Entity *row_module = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->module);
    if(info.eval.space.kind == RD_EvalSpaceKind_CtrlEntity)
    {
      switch(row_ctrl_entity->kind)
      {
        default:
        case CTRL_EntityKind_Process:
        if(info.eval.irtree.mode == E_Mode_Offset)
        {
          info.module = ctrl_module_from_process_vaddr(row_ctrl_entity, info.eval.value.u64);
        }break;
        case CTRL_EntityKind_Thread:
        if(info.eval.irtree.mode == E_Mode_Value)
        {
          CTRL_Entity *process = ctrl_process_from_entity(row_ctrl_entity);
          info.module = ctrl_module_from_process_vaddr(process, d_query_cached_rip_from_thread(row_ctrl_entity));
        }break;
      }
    }
    
    // rjf: determine call stack info
    if(block_eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity && str8_match(str8_lit("call_stack"), block_type->name, 0))
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(block_eval.space);
      if(entity->kind == CTRL_EntityKind_Thread)
      {
        info.callstack_thread = entity;
        U64 frame_num = block->lookup_rule->num_from_id(key.child_id, block->lookup_rule_user_data);
        CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
        CTRL_CallStack call_stack = ctrl_call_stack_from_unwind(scratch.arena, di_scope, ctrl_process_from_entity(entity), &unwind);
        if(1 <= frame_num && frame_num <= call_stack.count)
        {
          CTRL_CallStackFrame *f = &call_stack.frames[frame_num-1];
          info.callstack_unwind_index = f->unwind_count;
          info.callstack_inline_depth = f->inline_depth;
          info.callstack_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
        }
      }
    }
    
    // rjf: determine ctrl entity
    if(block_type_kind == E_TypeKind_Set && (block_eval.space.kind == RD_EvalSpaceKind_MetaQuery ||
                                             block_eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity))
    {
      info.group_entity = rd_ctrl_entity_from_eval_space(info.eval.space);
    }
    
    // rjf: determine cfg group name / parent
    if(block_type_kind == E_TypeKind_Set && (block_eval.space.kind == RD_EvalSpaceKind_MetaQuery ||
                                             block_eval.space.kind == RD_EvalSpaceKind_MetaCfg))
    {
      info.group_cfg_parent = rd_cfg_from_eval_space(block_eval.space);
    }
    
    // rjf: determine group cfg name
    if(block_type_kind == E_TypeKind_Set)
    {
      String8 singular_name = rd_singular_from_code_name_plural(block_type->name);
      if(singular_name.size != 0)
      {
        info.group_cfg_name = singular_name;
      }
      else
      {
        info.group_cfg_name = block_type->name;
      }
    }
    
    // rjf: determine row's group cfg
    if(info.group_cfg_name.size != 0)
    {
      RD_CfgID id = row->key.child_id;
      info.group_cfg_child = rd_cfg_from_id(id);
    }
    
    // rjf: determine cfgs/entities that this row is evaluating
    RD_Cfg *evalled_cfg = rd_cfg_from_eval_space(info.eval.space);
    CTRL_Entity *evalled_entity = (info.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity ? rd_ctrl_entity_from_eval_space(info.eval.space) : &ctrl_entity_nil);
    
    // rjf: determine if this cfg/entity evaluation is top-level - e.g. if we
    // are evaluating a cfg tree, or some descendant of it
    B32 is_top_level = 0;
    if(evalled_cfg != &rd_nil_cfg)
    {
      E_TypeKey top_level_type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, evalled_cfg->string);
      is_top_level = (info.eval.value.u64 == 0 && e_type_key_match(top_level_type_key, info.eval.irtree.type_key));
    }
    if(evalled_entity != &ctrl_entity_nil)
    {
      String8 top_level_name = ctrl_entity_kind_code_name_table[evalled_entity->kind];
      E_TypeKey top_level_type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, top_level_name);
      is_top_level = (info.eval.value.u64 == 0 && e_type_key_match(top_level_type_key, info.eval.irtree.type_key));
    }
    
    // rjf: determine view ui rule
    info.view_ui_rule = rd_view_ui_rule_from_string(row->block->expand_rule->string);
    if(info.view_ui_rule != &rd_nil_view_ui_rule)
    {
      info.view_ui_tag = row->block->expand_tag;
    }
    
    // rjf: fill row's cells
    {
      if(0){}
      
      // rjf: folder / file rows
      else if(info.eval.space.kind == E_SpaceKind_FileSystem)
      {
        E_Type *type = e_type_from_key__cached(info.eval.irtree.type_key);
        if(type->kind == E_TypeKind_Set)
        {
          String8 file_path = e_string_from_id(info.eval.value.u64);
          DR_FStrList fstrs = rd_title_fstrs_from_file_path(arena, file_path);
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr,
                                      .flags = RD_WatchCellFlag_Button|RD_WatchCellFlag_IsNonCode,
                                      .pct = 1.f,
                                      .fstrs = fstrs);
          if(str8_match(type->name, str8_lit("file"), 0))
          {
            info.can_expand = 0;
          }
        }
        else
        {
          info.cell_style_key = str8_lit("expr_and_eval");
          RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
          RD_Cfg *style = rd_cfg_child_from_string(view, info.cell_style_key);
          RD_Cfg *w_cfg = style->first;
          F32 next_pct = 0;
#define take_pct() (next_pct = (F32)f64_from_str8(w_cfg->string), w_cfg = w_cfg->next, next_pct)
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .default_pct = 0.35f, .pct = take_pct());
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .default_pct = 0.65f, .pct = take_pct());
#undef take_pct
        }
      }
      
      // rjf: singular button for unattached processes
      else if(info.eval.space.kind == RD_EvalSpaceKind_MetaUnattachedProcess)
      {
        E_Type *type = e_type_from_key__cached(info.eval.irtree.type_key);
        if(str8_match(type->name, str8_lit("unattached_process"), 0))
        {
          U64 pid = info.eval.value.u128.u64[0];
          String8 name = e_string_from_id(info.eval.value.u128.u64[1]);
          DR_FStrParams params = {rd_font_from_slot(RD_FontSlot_Main), rd_raster_flags_from_slot(RD_FontSlot_Main), ui_color_from_name(str8_lit("text")), ui_top_font_size()};
          DR_FStrList fstrs = {0};
          UI_TagF("weak")
          {
            dr_fstrs_push_new(arena, &fstrs, &params,
                              rd_icon_kind_text_table[RD_IconKind_Scheduler],
                              .font = rd_font_from_slot(RD_FontSlot_Icons),
                              .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons),
                              .color = ui_color_from_name(str8_lit("text")));
          }
          dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
          dr_fstrs_push_new(arena, &fstrs, &params, push_str8f(arena, "(PID: %I64u)", pid));
          dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
          dr_fstrs_push_new(arena, &fstrs, &params, name);
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = RD_WatchCellFlag_Button, .pct = 1.f, .fstrs = fstrs);
        }
      }
      
      // rjf: lister rows
      else if(rd_cfg_child_from_string(rd_cfg_from_id(rd_regs()->view), str8_lit("lister")) != &rd_nil_cfg)
      {
        info.can_expand = 0;
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = RD_WatchCellFlag_Button, .pct = 1.f);
      }
      
      // rjf: top-level cfg rows
      else if(is_top_level && evalled_cfg != &rd_nil_cfg)
      {
        RD_Cfg *cfg = evalled_cfg;
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = RD_WatchCellFlag_Button, .pct = 1.f, .fstrs = rd_title_fstrs_from_cfg(arena, cfg));
        MD_Node *schema = rd_schema_from_name(arena, cfg->string);
        MD_Node *cmds_root = md_tag_from_string(schema, str8_lit("commands"), 0);
        for MD_EachNode(cmd, cmds_root->first)
        {
          String8 cmd_name = cmd->string;
          RD_CmdKind cmd_kind = rd_cmd_kind_from_string(cmd_name);
          switch(cmd_kind)
          {
            default:{}break;
            case RD_CmdKind_EnableCfg:
            {
              B32 is_disabled = rd_disabled_from_cfg(cfg);
              if(!is_disabled)
              {
                cmd_kind = RD_CmdKind_DisableCfg;
              }
            }break;
            case RD_CmdKind_DisableCfg:
            {
              B32 is_disabled = rd_disabled_from_cfg(cfg);
              if(is_disabled)
              {
                cmd_kind = RD_CmdKind_EnableCfg;
              }
            }break;
            case RD_CmdKind_SelectCfg:
            {
              B32 is_disabled = rd_disabled_from_cfg(cfg);
              if(!is_disabled)
              {
                cmd_kind = RD_CmdKind_DeselectCfg;
              }
            }break;
            case RD_CmdKind_DeselectCfg:
            {
              B32 is_disabled = rd_disabled_from_cfg(cfg);
              if(is_disabled)
              {
                cmd_kind = RD_CmdKind_SelectCfg;
              }
            }break;
          }
          if(cmd_kind != RD_CmdKind_Null)
          {
            String8 cmd_name = rd_cmd_kind_info_table[cmd_kind].string;
            rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .flags = RD_WatchCellFlag_ActivateWithSingleClick|RD_WatchCellFlag_Button, .px = floor_f32(ui_top_font_size()*4.f), .string = push_str8f(arena, "query:commands.%S", cmd_name));
          }
        }
      }
      
      // rjf: top-level entity rows
      else if(is_top_level && evalled_entity != &ctrl_entity_nil)
      {
        CTRL_Entity *entity = evalled_entity;
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = RD_WatchCellFlag_Button, .pct = 1.f, .fstrs = rd_title_fstrs_from_ctrl_entity(arena, entity, 1));
        if(entity->kind == CTRL_EntityKind_Machine ||
           entity->kind == CTRL_EntityKind_Process ||
           entity->kind == CTRL_EntityKind_Thread)
        {
          RD_CmdKind cmd_kind = RD_CmdKind_FreezeEntity;
          if(ctrl_entity_tree_is_frozen(entity))
          {
            cmd_kind = RD_CmdKind_ThawEntity;
          }
          String8 cmd_name = rd_cmd_kind_info_table[cmd_kind].string;
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .flags = RD_WatchCellFlag_ActivateWithSingleClick|RD_WatchCellFlag_Button, .px = floor_f32(ui_top_font_size()*4.f), .string = push_str8f(arena, "query:commands.%S", cmd_name));
        }
        if(entity->kind == CTRL_EntityKind_Thread)
        {
          RD_CmdKind cmd_kind = RD_CmdKind_SelectEntity;
          if(ctrl_handle_match(entity->handle, rd_base_regs()->thread))
          {
            cmd_kind = RD_CmdKind_DeselectEntity;
          }
          String8 cmd_name = rd_cmd_kind_info_table[cmd_kind].string;
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .flags = RD_WatchCellFlag_ActivateWithSingleClick|RD_WatchCellFlag_Button, .px = floor_f32(ui_top_font_size()*4.f), .string = push_str8f(arena, "query:commands.%S", cmd_name));
        }
      }
      
      // rjf: singular row for queries
      else if(info.eval.space.kind == RD_EvalSpaceKind_MetaQuery)
      {
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .pct = 1.f);
      }
      
      // rjf: singular button for commands
      else if(info.eval.space.kind == RD_EvalSpaceKind_MetaCmd)
      {
        E_Type *type = e_type_from_key__cached(info.eval.irtree.type_key);
        if(type->kind == E_TypeKind_Set)
        {
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = 0, .pct = 1.f);
        }
        else
        {
          String8 cmd_name = e_string_from_id(e_value_eval_from_eval(info.eval).value.u64);
          RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(cmd_name);
          rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .flags = RD_WatchCellFlag_Button|RD_WatchCellFlag_ActivateWithSingleClick, .pct = 1.f, .fstrs = rd_title_fstrs_from_code_name(arena, cmd_kind_info->string));
        }
      }
      
      // rjf: singular cell for view ui
      else if(info.view_ui_rule != &rd_nil_view_ui_rule)
      {
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_ViewUI, .pct = 1.f);
      }
      
      // rjf: for 'add-new' rows in meta-cfg evaluation spaces, only do expr
      else if(info.eval.exprs.last == &e_expr_nil && info.group_cfg_name.size != 0 && info.group_cfg_child == &rd_nil_cfg)
      {
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .pct = 1.f);
      }
      
      // rjf: for meta-cfg evaluation spaces, only do expr/value
      else if(info.eval.space.kind == RD_EvalSpaceKind_MetaCfg ||
              info.eval.space.kind == RD_EvalSpaceKind_MetaCmd ||
              info.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity ||
              info.eval.space.kind == E_SpaceKind_File)
      {
        info.cell_style_key = str8_lit("expr_and_eval");
        RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
        RD_Cfg *style = rd_cfg_child_from_string(view, info.cell_style_key);
        RD_Cfg *w_cfg = style->first;
        F32 next_pct = 0;
#define take_pct() (next_pct = (F32)f64_from_str8(w_cfg->string), w_cfg = w_cfg->next, next_pct)
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr, .default_pct = 0.35f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .default_pct = 0.65f, .pct = take_pct());
#undef take_pct
      }
      
      // rjf: procedures collections get only expr/value/view-rule
      else if(block_type->kind == E_TypeKind_Set && str8_match(block_type->name, str8_lit("procedures"), 0))
      {
        info.cell_style_key = str8_lit("expr_value_viewrule");
        RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
        RD_Cfg *style = rd_cfg_child_from_string(view, info.cell_style_key);
        RD_Cfg *w_cfg = style->first;
        F32 next_pct = 0;
#define take_pct() (next_pct = (F32)f64_from_str8(w_cfg->string), w_cfg = w_cfg->next, next_pct)
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr,                                               .default_pct = 0.65f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .string = str8_lit("(U64)($expr) => hex"),    .default_pct = 0.20f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Tag,                                                .default_pct = 0.15f, .pct = take_pct());
#undef take_pct
      }
      
      // rjf: callstack frames
      else if(info.callstack_thread != &ctrl_entity_nil)
      {
        info.cell_style_key = str8_lit("call_stack_frame");
        CTRL_Entity *process = ctrl_process_from_entity(info.callstack_thread);
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, info.callstack_vaddr);
        E_Space space = rd_eval_space_from_ctrl_entity(module, RD_EvalSpaceKind_MetaCtrlEntity);
        E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafOffset, 0);
        expr->space    = space;
        expr->mode     = E_Mode_Offset;
        expr->type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, str8_lit("module"));
        E_Eval module_eval = e_eval_from_expr(arena, expr);
        RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
        RD_Cfg *style = rd_cfg_child_from_string(view, info.cell_style_key);
        RD_Cfg *w_cfg = style->first;
        F32 next_pct = 0;
#define take_pct() (next_pct = (F32)f64_from_str8(w_cfg->string), w_cfg = w_cfg->next, next_pct)
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_CallStackFrame,                                    .default_pct = 0.05f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval,                                              .default_pct = 0.55f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .string = str8_lit("(U64)($expr) => hex"),   .default_pct = 0.20f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .eval = module_eval,                         .default_pct = 0.20f, .pct = take_pct());
#undef take_pct
      }
      
      // rjf: default cells
      else
      {
        info.cell_style_key = str8_lit("normal");
        RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
        RD_Cfg *style = rd_cfg_child_from_string(view, info.cell_style_key);
        RD_Cfg *w_cfg = style->first;
        F32 next_pct = 0;
#define take_pct() (next_pct = (F32)f64_from_str8(w_cfg->string), w_cfg = w_cfg->next, next_pct)
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Expr,                                        .default_pct = 0.25f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval,                                        .default_pct = 0.35f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Eval, .string = str8_lit("typeof($expr)"),   .default_pct = 0.15f, .pct = take_pct());
        rd_watch_cell_list_push_new(arena, &info.cells, RD_WatchCellKind_Tag,                                         .default_pct = 0.25f, .pct = take_pct());
#undef take_pct
      }
    }
    
    di_scope_close(di_scope);
    scratch_end(scratch);
  }
  return info;
}

//- rjf: row * cell -> string

internal RD_WatchRowCellInfo
rd_info_from_watch_row_cell(Arena *arena, EV_Row *row, EV_StringFlags string_flags, RD_WatchRowInfo *row_info, RD_WatchCell *cell, FNT_Tag font, F32 font_size, F32 max_size_px)
{
  RD_WatchRowCellInfo result = {0};
  
  //- rjf: fill basics/defaults
  result.view_ui_rule = &rd_nil_view_ui_rule;
  result.view_ui_tag = &e_expr_nil;
  result.fstrs = cell->fstrs;
  result.flags = cell->flags;
  result.cfg = &rd_nil_cfg;
  result.entity = &ctrl_entity_nil;
  
  //- rjf: do per-kind fills
  switch(cell->kind)
  {
    default:{}break;
    
    //- rjf: expression cells -> if no string attached to row itself, form one from the
    // expression tree.
    case RD_WatchCellKind_Expr:
    {
      if(row_info->expr_is_editable)
      {
        result.flags |= RD_WatchCellFlag_CanEdit;
      }
      result.eval = (cell->eval.irtree.mode != E_Mode_Null ? cell->eval : e_eval_from_expr(arena, row->expr));
      result.string = row->string;
      if(result.string.size == 0)
      {
        E_Expr *notable_expr = row->expr;
        for(B32 good = 0; !good;)
        {
          switch(notable_expr->kind)
          {
            default:{good = 1;}break;
            case E_ExprKind_Address:
            case E_ExprKind_Deref:
            case E_ExprKind_Cast:
            {
              notable_expr = notable_expr->last;
            }break;
            case E_ExprKind_Ref:
            {
              notable_expr = notable_expr->ref;
            }break;
          }
        }
        switch(notable_expr->kind)
        {
          default:
          {
            result.string = e_string_from_expr(arena, notable_expr);
          }break;
          case E_ExprKind_ArrayIndex:
          {
            result.string = push_str8f(arena, "[%S]", e_string_from_expr(arena, notable_expr->last));
          }break;
          case E_ExprKind_MemberAccess:
          {
            Temp scratch = scratch_begin(&arena, 1);
            E_Member member = result.eval.irtree.member;
            String8 member_name = member.name;
            if(member.inheritance_key_chain.count != 0)
            {
              String8List strings = {0};
              for(E_TypeKeyNode *n = member.inheritance_key_chain.first; n != 0; n = n->next)
              {
                String8 base_class_name = e_type_string_from_key(scratch.arena, n->v);
                str8_list_push(scratch.arena, &strings, base_class_name);
              }
              result.inheritance_tooltip = str8_list_join(arena, &strings, &(StringJoin){.sep = str8_lit_comp("::")});
            }
            B32 is_non_code = 0;
            String8 string = push_str8f(arena, ".%S", member_name);
            if(result.eval.space.kind == RD_EvalSpaceKind_MetaCfg ||
               result.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity ||
               result.eval.space.kind == E_SpaceKind_File ||
               result.eval.space.kind == E_SpaceKind_FileSystem)
            {
              String8 fancy_name = rd_display_from_code_name(member_name);
              if(fancy_name.size != 0)
              {
                string = fancy_name;
                is_non_code = 1;
              }
            }
            result.flags |= (!!is_non_code * RD_WatchCellFlag_IsNonCode);
            result.string = string;
            scratch_end(scratch);
          }break;
        }
      }
    }break;
    
    //- rjf: evaluation cells -> wrap expression if needed, evaluate, & stringize
    case RD_WatchCellKind_Eval:
    {
      Temp scratch = scratch_begin(&arena, 1);
      
      //- rjf: use cell's wrap string to wrap row's expression
      String8 wrap_string = cell->string;
      E_Expr *root_expr = row->expr;
      if(wrap_string.size != 0)
      {
        E_Expr *wrap_expr = e_parse_expr_from_text(scratch.arena, wrap_string).exprs.last;
        root_expr = wrap_expr;
        typedef struct Task Task;
        struct Task
        {
          Task *next;
          E_Expr *parent;
          E_Expr *expr;
        };
        Task start_task = {0, &e_expr_nil, wrap_expr};
        Task *first_task = &start_task;
        Task *last_task = first_task;
        for(Task *t = first_task; t != 0; t = t->next)
        {
          if(t->expr->kind == E_ExprKind_LeafIdent && str8_match(t->expr->string, str8_lit("$expr"), 0))
          {
            E_Expr *original_expr_ref = e_expr_ref(scratch.arena, row->expr);
            if(t->parent != &e_expr_nil)
            {
              e_expr_insert_child(t->parent, t->expr, original_expr_ref);
              e_expr_remove_child(t->parent, t->expr);
            }
            else
            {
              root_expr = original_expr_ref;
            }
          }
          else for(E_Expr *child = t->expr->first; child != &e_expr_nil; child = child->next)
          {
            Task *task = push_array(scratch.arena, Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->parent = t->expr;
            task->expr = child;
          }
        }
      }
      
      //- rjf: evaluate wrapped expression
      result.eval     = (cell->eval.irtree.mode != E_Mode_Null ? cell->eval : e_eval_from_expr(arena, root_expr));
      
      //- rjf: determine default radix
      U32 default_radix = 10;
      if(result.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity &&
         rd_ctrl_entity_from_eval_space(result.eval.space)->kind == CTRL_EntityKind_Module)
      {
        default_radix = 16;
      }
      
      //- rjf: generate strings/flags based on that expression & fill
      result.string   = rd_value_string_from_eval(arena, rd_view_query_input(), string_flags, default_radix, font, font_size, max_size_px, result.eval);
      result.flags   |= !!(ev_type_key_is_editable(result.eval.irtree.type_key) && result.eval.irtree.mode == E_Mode_Offset) * RD_WatchCellFlag_CanEdit;
      E_Type *type = e_type_from_key__cached(result.eval.irtree.type_key);
      if(type->flags & (E_TypeFlag_IsPlainText|E_TypeFlag_IsPathText))
      {
        result.flags |= RD_WatchCellFlag_IsNonCode;
      }
      
      scratch_end(scratch);
    }break;
    
    //- rjf: tag cells -> look up attached tag
    case RD_WatchCellKind_Tag:
    {
      EV_View *ev_view = rd_view_eval_view();
      result.string = ev_view_rule_from_key(ev_view, row->key);
      result.flags |= RD_WatchCellFlag_CanEdit;
    }break;
    
    //- rjf: view ui cells
    case RD_WatchCellKind_ViewUI:
    {
      result.eval = (cell->eval.irtree.mode != E_Mode_Null ? cell->eval : e_eval_from_expr(arena, row->expr));
      result.view_ui_rule = row_info->view_ui_rule;
      result.view_ui_tag = row_info->view_ui_tag;
    }break;
  }
  
  //- rjf: adjust style based on evaluation
  switch(cell->kind)
  {
    default:{}break;
    case RD_WatchCellKind_Expr:
    {
      if(result.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity &&
         result.eval.value.u64 == 0)
      {
        CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(result.eval.space);
        E_TypeKey cfg_type = e_string2typekey_map_lookup(rd_state->meta_name2type_map, ctrl_entity_kind_code_name_table[entity->kind]);
        if(e_type_key_match(cfg_type, result.eval.irtree.type_key))
        {
          result.entity = entity;
        }
      }
      else if(result.eval.space.kind == RD_EvalSpaceKind_MetaCfg &&
              result.eval.value.u64 == 0)
      {
        RD_Cfg *cfg = rd_cfg_from_eval_space(result.eval.space);
        E_TypeKey cfg_type = e_string2typekey_map_lookup(rd_state->meta_name2type_map, cfg->string);
        if(e_type_key_match(cfg_type, result.eval.irtree.type_key))
        {
          result.cfg = cfg;
          result.fstrs = rd_title_fstrs_from_cfg(arena, cfg);
        }
      }
      else if(result.eval.space.kind == RD_EvalSpaceKind_MetaCmd)
      {
        result.cmd_name = e_string_from_id(result.eval.value.u64);
      }
      else if(result.eval.space.kind == E_SpaceKind_FileSystem)
      {
        E_Type *type = e_type_from_key__cached(result.eval.irtree.type_key);
        if(type->kind == E_TypeKind_Set)
        {
          String8 file_path = e_string_from_id(result.eval.value.u64);
          result.fstrs = rd_title_fstrs_from_file_path(arena, file_path);
          result.flags |= RD_WatchCellFlag_Button;
        }
      }
    }break;
    case RD_WatchCellKind_Eval:
    {
      if(result.eval.msgs.max_kind != E_MsgKind_Null)
      {
        Temp scratch = scratch_begin(&arena, 1);
        result.flags |= RD_WatchCellFlag_IsErrored|RD_WatchCellFlag_IsNonCode;
        String8List error_strings = {0};
        for(E_Msg *msg = result.eval.msgs.first; msg != 0; msg = msg->next)
        {
          str8_list_push(scratch.arena, &error_strings, msg->text);
          if(msg->next)
          {
            str8_list_pushf(scratch.arena, &error_strings, " ");
          }
        }
        result.string = str8_list_join(arena, &error_strings, 0);
        scratch_end(scratch);
      }
      else if(result.eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity &&
              result.eval.value.u64 == 0)
      {
        CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(result.eval.space);
        E_TypeKey cfg_type = e_string2typekey_map_lookup(rd_state->meta_name2type_map, ctrl_entity_kind_code_name_table[entity->kind]);
        if(e_type_key_match(cfg_type, result.eval.irtree.type_key))
        {
          result.fstrs = rd_title_fstrs_from_ctrl_entity(arena, entity, 1);
          result.flags |= RD_WatchCellFlag_Button;
          result.entity = entity;
        }
      }
      else if(result.eval.space.kind == RD_EvalSpaceKind_MetaCfg &&
              result.eval.value.u64 == 0)
      {
        RD_Cfg *cfg = rd_cfg_from_eval_space(result.eval.space);
        E_TypeKey cfg_type = e_string2typekey_map_lookup(rd_state->meta_name2type_map, cfg->string);
        if(e_type_key_match(cfg_type, result.eval.irtree.type_key))
        {
          result.fstrs = rd_title_fstrs_from_cfg(arena, cfg);
          result.flags |= RD_WatchCellFlag_Button;
          result.cfg = cfg;
        }
      }
      else if(result.eval.space.kind == RD_EvalSpaceKind_MetaCmd)
      {
        String8 cmd_name = e_string_from_id(result.eval.value.u64);
        if(cell->px != 0) UI_TagF("weak")
        {
          DR_FStrParams params = {rd_font_from_slot(RD_FontSlot_Icons), rd_raster_flags_from_slot(RD_FontSlot_Icons), ui_color_from_name(str8_lit("text")), ui_top_font_size()};
          dr_fstrs_push_new(arena, &result.fstrs, &params, rd_icon_kind_text_table[rd_icon_kind_from_code_name(cmd_name)]);
        }
        else
        {
          result.fstrs = rd_title_fstrs_from_code_name(arena, cmd_name);
        }
        result.flags |= RD_WatchCellFlag_Button;
        result.cmd_name = cmd_name;
      }
      else if(result.eval.space.kind == E_SpaceKind_FileSystem)
      {
        E_Type *type = e_type_from_key__cached(result.eval.irtree.type_key);
        if(type->kind == E_TypeKind_Set)
        {
          String8 file_path = e_string_from_id(result.eval.value.u64);
          result.fstrs = rd_title_fstrs_from_file_path(arena, file_path);
          result.flags |= RD_WatchCellFlag_Button;
        }
      }
    }break;
  }
  
  return result;
}

//- rjf: table coordinates -> text edit state

internal RD_WatchViewTextEditState *
rd_watch_view_text_edit_state_from_pt(RD_WatchViewState *wv, RD_WatchPt pt)
{
  RD_WatchViewTextEditState *result = &wv->dummy_text_edit_state;
  if(wv->text_edit_state_slots_count != 0 && wv->text_editing != 0)
  {
    U64 hash = ev_hash_from_key(pt.key);
    U64 slot_idx = hash%wv->text_edit_state_slots_count;
    for(RD_WatchViewTextEditState *s = wv->text_edit_state_slots[slot_idx]; s != 0; s = s->pt_hash_next)
    {
      if(rd_watch_pt_match(pt, s->pt))
      {
        result = s;
        break;
      }
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: null @view_hook_impl

RD_VIEW_UI_FUNCTION_DEF(null) {}

////////////////////////////////
//~ rjf: text @view_hook_impl

EV_EXPAND_RULE_INFO_FUNCTION_DEF(text)
{
  EV_ExpandInfo info = {0};
  info.row_count = 8;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(text)
{
  RD_CodeViewState *cv = rd_view_state(RD_CodeViewState);
  rd_code_view_init(cv);
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  
  //////////////////////////////
  //- rjf: set up invariants
  //
  F32 bottom_bar_height = ui_top_font_size()*2.f;
  Rng2F32 code_area_rect = r2f32p(rect.x0, rect.y0, rect.x1, rect.y1 - bottom_bar_height);
  Rng2F32 bottom_bar_rect = r2f32p(rect.x0, rect.y1 - bottom_bar_height, rect.x1, rect.y1);
  
  //////////////////////////////
  //- rjf: process code-file commands
  //
  ProfScope("process code-file commands") for(RD_Cmd *cmd = 0; rd_next_view_cmd(&cmd);)
  {
    RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
    switch(kind)
    {
      default:{}break;
      
      // rjf: override file picking
      case RD_CmdKind_PickFile:
      {
        String8 src = rd_regs()->file_path;
        String8 dst = cmd->regs->file_path;
        if(src.size != 0 && dst.size != 0)
        {
          // rjf: record src -> dst mapping
          rd_cmd(RD_CmdKind_SetFileReplacementPath, .string = src, .file_path = dst);
          
          // rjf: switch this view to viewing replacement file
          rd_store_view_expr_string(rd_eval_string_from_file_path(scratch.arena, dst));
        }
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: unpack parameterization info
  //
  ProfBegin("unpack parameterization info");
  rd_regs()->file_path     = rd_file_path_from_eval(rd_frame_arena(), eval);
  rd_regs()->vaddr         = 0;
  rd_regs()->prefer_disasm = 0;
  rd_regs()->cursor.line   = rd_view_cfg_value_from_string(str8_lit("cursor_line")).s64;
  rd_regs()->cursor.column = rd_view_cfg_value_from_string(str8_lit("cursor_column")).s64;
  rd_regs()->mark.line     = rd_view_cfg_value_from_string(str8_lit("mark_line")).s64;
  rd_regs()->mark.column   = rd_view_cfg_value_from_string(str8_lit("mark_column")).s64;
  if(rd_regs()->cursor.line == 0)   { rd_regs()->cursor.line = 1; }
  if(rd_regs()->cursor.column == 0) { rd_regs()->cursor.column = 1; }
  if(rd_regs()->mark.line == 0)     { rd_regs()->mark.line = 1; }
  if(rd_regs()->mark.column == 0)   { rd_regs()->mark.column = 1; }
  Rng1U64 range = rd_range_from_eval_tag(eval, tag);
  rd_regs()->text_key = rd_key_from_eval_space_range(eval.space, range, 1);
  rd_regs()->lang_kind = rd_lang_kind_from_eval_tag(eval, tag);
  U128 hash = {0};
  TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, rd_regs()->text_key, rd_regs()->lang_kind, &hash);
  String8 data = hs_data_from_hash(hs_scope, hash);
  B32 file_is_missing = (rd_regs()->file_path.size != 0 && os_properties_from_file_path(rd_regs()->file_path).modified == 0);
  B32 key_has_data = !u128_match(hash, u128_zero()) && info.lines_count;
  ProfEnd();
  
  //////////////////////////////
  //- rjf: build missing file interface
  //
  if(file_is_missing)
  {
    UI_WidthFill UI_HeightFill UI_Column UI_Padding(ui_pct(1, 0))
    {
      Temp scratch = scratch_begin(0, 0);
      UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(1, 1))
        UI_TagF("weak")
      {
        RD_Font(RD_FontSlot_Icons) ui_label(rd_icon_kind_text_table[RD_IconKind_WarningBig]);
        ui_labelf("Could not find \"%S\".", rd_regs()->file_path);
      }
      UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_CornerRadius(ui_top_font_size()/3)
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_Focus(UI_FocusKind_On)
        UI_TextAlignment(UI_TextAlign_Center)
        UI_TagF("pop")
        if(ui_clicked(ui_buttonf("Find alternative...")))
      {
        rd_cmd(RD_CmdKind_RunCommand, .cmd_name = rd_cmd_kind_info_table[RD_CmdKind_PickFile].string);
      }
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: code is not missing, but not ready -> equip loading info to this view
  //
  if(!file_is_missing && info.lines_count == 0 && eval.msgs.max_kind == E_MsgKind_Null)
  {
    rd_store_view_loading_info(1, info.bytes_processed, info.bytes_to_process);
  }
  
  //////////////////////////////
  //- rjf: build code contents
  //
  DI_KeyList dbgi_keys = {0};
  if(!file_is_missing)
  {
    RD_CodeViewBuildFlags flags = RD_CodeViewBuildFlag_All;
    if(rd_regs()->file_path.size == 0)
    {
      flags &= ~RD_CodeViewBuildFlag_Margins;
    }
    RD_CodeViewBuildResult result = rd_code_view_build(scratch.arena, cv, flags, code_area_rect, data, &info, 0, r1u64(0, 0), di_key_zero());
    dbgi_keys = result.dbgi_keys;
  }
  
  //////////////////////////////
  //- rjf: unpack cursor info
  //
  if(rd_regs()->file_path.size != 0)
  {
    CTRL_Entity *module = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->module);
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
    rd_regs()->lines = d_lines_from_dbgi_key_file_path_line_num(rd_frame_arena(), dbgi_key, rd_regs()->file_path, rd_regs()->cursor.line);
  }
  
  //////////////////////////////
  //- rjf: determine if file is out-of-date
  //
  B32 file_is_out_of_date = 0;
  String8 out_of_date_dbgi_name = {0};
  {
    U64 file_timestamp = fs_properties_from_path(rd_regs()->file_path).modified;
    if(file_timestamp != 0)
    {
      for(DI_KeyNode *n = dbgi_keys.first; n != 0; n = n->next)
      {
        DI_Key key = n->v;
        if(key.min_timestamp < file_timestamp && key.min_timestamp != 0 && key.path.size != 0)
        {
          file_is_out_of_date = 1;
          out_of_date_dbgi_name = str8_skip_last_slash(key.path);
          break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build bottom bar
  //
  if(!file_is_missing && key_has_data)
  {
    ui_set_next_rect(shift_2f32(bottom_bar_rect, scale_2f32(rect.p0, -1.f)));
    ui_set_next_flags(UI_BoxFlag_DrawBackground);
    UI_Row
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TagF("weak")
    {
      if(file_is_out_of_date) UI_TagF("bad_pop")
      {
        UI_Box *box = &ui_nil_box;
        RD_Font(RD_FontSlot_Icons)
        {
          box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, "%S###file_ood_warning", rd_icon_kind_text_table[RD_IconKind_WarningBig]);
        }
        UI_Signal sig = ui_signal_from_box(box);
        if(ui_hovering(sig)) UI_Tooltip
        {
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(1, 1))
          {
            ui_labelf("This file has changed since ", out_of_date_dbgi_name);
            ui_label(out_of_date_dbgi_name);
            ui_labelf(" was produced.");
          }
        }
      }
      RD_Font(RD_FontSlot_Code)
      {
        if(rd_regs()->file_path.size != 0)
        {
          ui_label(rd_regs()->file_path);
          ui_spacer(ui_em(1.5f, 1));
        }
        ui_labelf("Line: %I64d, Column: %I64d", rd_regs()->cursor.line, rd_regs()->cursor.column);
        ui_spacer(ui_pct(1, 0));
        ui_labelf("(read only)");
        ui_labelf("%s",
                  info.line_end_kind == TXT_LineEndKind_LF   ? "lf" :
                  info.line_end_kind == TXT_LineEndKind_CRLF ? "crlf" :
                  "bin");
      }
    }
  }
  
  //////////////////////////////
  //- rjf: store params
  //
  rd_store_view_param_s64(str8_lit("cursor_line"), rd_regs()->cursor.line);
  rd_store_view_param_s64(str8_lit("cursor_column"), rd_regs()->cursor.column);
  rd_store_view_param_s64(str8_lit("mark_line"), rd_regs()->mark.line);
  rd_store_view_param_s64(str8_lit("mark_column"), rd_regs()->mark.column);
  
  txt_scope_close(txt_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: disasm @view_hook_impl

typedef struct RD_DisasmViewState RD_DisasmViewState;
struct RD_DisasmViewState
{
  B32 initialized;
  TxtPt cursor;
  TxtPt mark;
  CTRL_Handle temp_look_process;
  U64 temp_look_vaddr;
  U64 temp_look_run_gen;
  U64 goto_vaddr;
  RD_CodeViewState cv;
};

EV_EXPAND_RULE_INFO_FUNCTION_DEF(disasm)
{
  EV_ExpandInfo info = {0};
  info.row_count = 8;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(disasm)
{
  RD_DisasmViewState *dv = rd_view_state(RD_DisasmViewState);
  if(dv->initialized == 0)
  {
    dv->initialized = 1;
    dv->cursor = txt_pt(1, 1);
    dv->mark = txt_pt(1, 1);
    rd_code_view_init(&dv->cv);
  }
  RD_CodeViewState *cv = &dv->cv;
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DASM_Scope *dasm_scope = dasm_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  
  //////////////////////////////
  //- rjf: if disassembly views are not parameterized by anything, they
  // automatically snap to the selected thread's RIP, OR the "temp look
  // address" (commanded by go-to-disasm or go-to-address), rounded down to the
  // nearest 16K boundary
  //
  B32 auto_selected = 0;
  E_Space auto_space = {0};
  if(eval.exprs.last == &e_expr_nil)
  {
    if(dv->temp_look_vaddr != 0 && dv->temp_look_run_gen == ctrl_run_gen())
    {
      auto_selected = 1;
      auto_space = rd_eval_space_from_ctrl_entity(ctrl_entity_from_handle(d_state->ctrl_entity_store, dv->temp_look_process), RD_EvalSpaceKind_CtrlEntity);
      eval = e_eval_from_stringf(scratch.arena, "(0x%I64x & (~(0x4000 - 1)))", dv->temp_look_vaddr);
    }
    else
    {
      auto_selected = 1;
      auto_space = rd_eval_space_from_ctrl_entity(ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->process), RD_EvalSpaceKind_CtrlEntity);
      eval = e_eval_from_stringf(scratch.arena, "(rip.u64 & (~(0x4000 - 1)))");
    }
  }
  
  //////////////////////////////
  //- rjf: set up invariants
  //
  F32 bottom_bar_height = ui_top_font_size()*2.f;
  Rng2F32 code_area_rect = r2f32p(rect.x0, rect.y0, rect.x1, rect.y1 - bottom_bar_height);
  Rng2F32 bottom_bar_rect = r2f32p(rect.x0, rect.y1 - bottom_bar_height, rect.x1, rect.y1);
  rd_regs()->file_path = str8_zero();
  rd_regs()->cursor = dv->cursor;
  rd_regs()->mark = dv->mark;
  
  //////////////////////////////
  //- rjf: process disassembly-specific commands
  //
  for(RD_Cmd *cmd = 0; rd_next_view_cmd(&cmd);)
  {
    RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
    switch(kind)
    {
      default: break;
      case RD_CmdKind_GoToAddress:
      {
        dv->temp_look_process = cmd->regs->process;
        dv->temp_look_vaddr   = cmd->regs->vaddr;
        dv->temp_look_run_gen = ctrl_run_gen();
        dv->goto_vaddr        = cmd->regs->vaddr;
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: unpack parameterization info
  //
  E_Space space = eval.space;
  if(auto_selected)
  {
    space = auto_space;
  }
  Rng1U64 range = rd_range_from_eval_tag(eval, tag);
  Arch arch = rd_arch_from_eval_tag(eval, tag);
  CTRL_Entity *space_entity = rd_ctrl_entity_from_eval_space(space);
  CTRL_Entity *dasm_module = &ctrl_entity_nil;
  DI_Key dbgi_key = {0};
  U64 base_vaddr = 0;
  switch(space_entity->kind)
  {
    default:{}break;
    case CTRL_EntityKind_Process:
    {
      if(arch == Arch_Null) { arch = space_entity->arch; }
      dasm_module = ctrl_module_from_process_vaddr(space_entity, range.min);
      dbgi_key    = ctrl_dbgi_key_from_module(dasm_module);
      base_vaddr  = dasm_module->vaddr_range.min;
    }break;
  }
  DASM_StyleFlags style_flags = 0;
  DASM_Syntax syntax = DASM_Syntax_Intel;
  {
    if(rd_setting_b32_from_name(str8_lit("show_addresses")))
    {
      style_flags |= DASM_StyleFlag_Addresses;
    }
    if(rd_setting_b32_from_name(str8_lit("show_code_bytes")))
    {
      style_flags |= DASM_StyleFlag_CodeBytes;
    }
    if(rd_setting_b32_from_name(str8_lit("show_source_lines")))
    {
      style_flags |= DASM_StyleFlag_SourceFilesNames;
      style_flags |= DASM_StyleFlag_SourceLines;
    }
    if(rd_setting_b32_from_name(str8_lit("show_symbol_names")))
    {
      style_flags |= DASM_StyleFlag_SymbolNames;
    }
  }
  U128 dasm_key = rd_key_from_eval_space_range(space, range, 0);
  U128 dasm_data_hash = {0};
  DASM_Params dasm_params = {0};
  {
    dasm_params.vaddr       = range.min;
    dasm_params.arch        = arch;
    dasm_params.style_flags = style_flags;
    dasm_params.syntax      = syntax;
    dasm_params.base_vaddr  = base_vaddr;
    dasm_params.dbgi_key    = dbgi_key;
  }
  DASM_Info dasm_info = dasm_info_from_key_params(dasm_scope, dasm_key, &dasm_params, &dasm_data_hash);
  rd_regs()->text_key = dasm_info.text_key;
  rd_regs()->lang_kind = txt_lang_kind_from_arch(arch);
  U128 dasm_text_hash = {0};
  TXT_TextInfo dasm_text_info = txt_text_info_from_key_lang(txt_scope, rd_regs()->text_key, rd_regs()->lang_kind, &dasm_text_hash);
  String8 dasm_text_data = hs_data_from_hash(hs_scope, dasm_text_hash);
  B32 has_disasm = (dasm_info.lines.count != 0 && dasm_text_info.lines_count != 0);
  B32 is_loading = (!has_disasm && dim_1u64(range) != 0 && eval.msgs.max_kind == E_MsgKind_Null && (space.kind != RD_EvalSpaceKind_CtrlEntity || space_entity != &ctrl_entity_nil));
  
  //////////////////////////////
  //- rjf: is loading -> equip view with loading information
  //
  if(is_loading && !d_ctrl_targets_running())
  {
    rd_store_view_loading_info(is_loading, 0, 0);
  }
  
  //////////////////////////////
  //- rjf: do goto vaddr
  //
  if(!is_loading && has_disasm && dv->goto_vaddr != 0 && contains_1u64(range, dv->goto_vaddr))
  {
    U64 vaddr = dv->goto_vaddr;
    U64 line_idx = dasm_line_array_idx_from_code_off__linear_scan(&dasm_info.lines, vaddr-range.min);
    if(line_idx < dasm_info.lines.count)
    {
      S64 line_num = (S64)(line_idx+1);
      dv->goto_vaddr = 0;
      cv->goto_line_num = line_num;
    }
  }
  
  //////////////////////////////
  //- rjf: build code contents
  //
  if(!is_loading && has_disasm)
  {
    rd_code_view_build(scratch.arena, cv, RD_CodeViewBuildFlag_All, code_area_rect, dasm_text_data, &dasm_text_info, &dasm_info.lines, range, dbgi_key);
  }
  
  //////////////////////////////
  //- rjf: unpack cursor info
  //
  if(!is_loading && has_disasm)
  {
    U64 off = dasm_line_array_code_off_from_idx(&dasm_info.lines, rd_regs()->cursor.line-1);
    rd_regs()->prefer_disasm = 1;
    rd_regs()->vaddr = range.min+off;
    rd_regs()->vaddr_range = r1u64(range.min+off, range.min+off);
    rd_regs()->voff_range = ctrl_voff_range_from_vaddr_range(dasm_module, rd_regs()->vaddr_range);
    rd_regs()->lines = d_lines_from_dbgi_key_voff(rd_frame_arena(), &dbgi_key, rd_regs()->voff_range.min);
  }
  
  //////////////////////////////
  //- rjf: build bottom bar
  //
  if(!is_loading && has_disasm)
  {
    ui_set_next_rect(shift_2f32(bottom_bar_rect, scale_2f32(rect.p0, -1.f)));
    ui_set_next_flags(UI_BoxFlag_DrawBackground);
    UI_Row
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TagF("weak")
      RD_Font(RD_FontSlot_Code)
    {
      U64 cursor_vaddr = (1 <= rd_regs()->cursor.line && rd_regs()->cursor.line <= dasm_info.lines.count) ? (range.min+dasm_info.lines.v[rd_regs()->cursor.line-1].code_off) : 0;
      if(dasm_module != &ctrl_entity_nil)
      {
        ui_labelf("%S", path_normalized_from_string(scratch.arena, dasm_module->string));
        ui_spacer(ui_em(1.5f, 1));
      }
      ui_labelf("Address: 0x%I64x, Line: %I64d, Column: %I64d", cursor_vaddr, rd_regs()->cursor.line, rd_regs()->cursor.column);
      ui_spacer(ui_pct(1, 0));
      ui_labelf("(read only)");
      ui_labelf("bin");
    }
  }
  
  //////////////////////////////
  //- rjf: commit storage
  //
  dv->cursor = rd_regs()->cursor;
  dv->mark = rd_regs()->mark;
  
  txt_scope_close(txt_scope);
  dasm_scope_close(dasm_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: memory @view_hook_impl

typedef struct RD_MemoryViewState RD_MemoryViewState;
struct RD_MemoryViewState
{
  B32 center_cursor;
  B32 contain_cursor;
};

EV_EXPAND_RULE_INFO_FUNCTION_DEF(memory)
{
  EV_ExpandInfo info = {0};
  info.row_count = 16;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(memory)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  RD_MemoryViewState *mv = rd_view_state(RD_MemoryViewState);
  
  //////////////////////////////
  //- rjf: unpack parameterization info
  //
  Rng1U64 space_range = rd_range_from_eval_tag(eval, tag);
  if(eval.space.kind == 0)
  {
    eval.space = rd_eval_space_from_ctrl_entity(ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->process), RD_EvalSpaceKind_CtrlEntity);
    space_range = rd_whole_range_from_eval_space(eval.space);
    if(dim_1u64(space_range) == 0)
    {
      space_range = r1u64(0, 0x7FFFFFFFFFFFull);
    }
  }
  U64 cursor          = rd_view_cfg_value_from_string(str8_lit("cursor_vaddr")).u64;
  U64 mark            = rd_view_cfg_value_from_string(str8_lit("mark_vaddr")).u64;
  U64 bytes_per_cell  = rd_view_cfg_value_from_string(str8_lit("bytes_per_cell")).u64;
  U64 num_columns     = rd_view_cfg_value_from_string(str8_lit("num_columns")).u64;
  if(num_columns == 0)
  {
    num_columns = 16;
  }
  num_columns = ClampBot(1, num_columns);
  bytes_per_cell = ClampBot(1, bytes_per_cell);
  UI_ScrollPt2 scroll_pos = rd_view_scroll_pos();
  Vec4F32 selection_color = ui_color_from_name(str8_lit("selection"));
  Vec4F32 border_color = ui_color_from_name(str8_lit("border"));
  
  //////////////////////////////
  //- rjf: process commands
  //
  for(RD_Cmd *cmd = 0; rd_next_view_cmd(&cmd);)
  {
    RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
    switch(kind)
    {
      default: break;
      case RD_CmdKind_CenterCursor:
      {
        mv->center_cursor = 1;
      }break;
      case RD_CmdKind_ContainCursor:
      {
        mv->contain_cursor = 1;
      }break;
      case RD_CmdKind_GoToAddress:
      {
        cursor = mark = cmd->regs->vaddr;
        mv->center_cursor = 1;
      }break;
      case RD_CmdKind_SetColumns:
      {
        // TODO(rjf)
      }break;
    }
  }
  
  //////////////////////////////
  //- rjf: unpack visual params
  //
  FNT_Tag font = rd_font_from_slot(RD_FontSlot_Code);
  FNT_RasterFlags font_raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Code);
  F32 font_size = rd_font_size_from_slot(RD_FontSlot_Code);
  F32 big_glyph_advance = fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("H")).x;
  F32 row_height_px = floor_f32(font_size*2.f);
  F32 cell_width_px = floor_f32(font_size*2.f * bytes_per_cell);
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 panel_dim = dim_2f32(rect);
  F32 footer_dim = font_size*10.f;
  Rng2F32 header_rect = r2f32p(0, 0, panel_dim.x, row_height_px);
  Rng2F32 footer_rect = r2f32p(0, panel_dim.y-footer_dim, panel_dim.x, panel_dim.y);
  Rng2F32 content_rect = r2f32p(0, row_height_px, panel_dim.x-scroll_bar_dim, footer_rect.y0);
  
  //////////////////////////////
  //- rjf: determine legal scroll range
  //
  Rng1S64 scroll_idx_rng = r1s64(0, dim_1u64(space_range)/num_columns);
  
  //////////////////////////////
  //- rjf: determine info about visible range of rows
  //
  Rng1S64 viz_range_rows = {0};
  Rng1U64 viz_range_bytes = {0};
  S64 num_possible_visible_rows = 0;
  {
    num_possible_visible_rows = dim_2f32(content_rect).y/row_height_px;
    viz_range_rows.min = scroll_pos.y.idx + (S64)scroll_pos.y.off - !!(scroll_pos.y.off<0);
    viz_range_rows.max = scroll_pos.y.idx + (S64)scroll_pos.y.off + num_possible_visible_rows,
    viz_range_rows.min = clamp_1s64(scroll_idx_rng, viz_range_rows.min);
    viz_range_rows.max = clamp_1s64(scroll_idx_rng, viz_range_rows.max);
    viz_range_bytes.min = space_range.min + viz_range_rows.min*num_columns;
    viz_range_bytes.max = space_range.min + (viz_range_rows.max+1)*num_columns+1;
    if(viz_range_bytes.min > viz_range_bytes.max)
    {
      Swap(U64, viz_range_bytes.min, viz_range_bytes.max);
    }
    viz_range_bytes = intersect_1u64(space_range, viz_range_bytes);
  }
  
  //////////////////////////////
  //- rjf: take keyboard controls
  //
  UI_Focus(UI_FocusKind_On) if(ui_is_focus_active())
  {
    U64 next_cursor = cursor;
    U64 next_mark = mark;
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      Vec2S64 cell_delta = {0};
      switch(evt->delta_unit)
      {
        default:{}break;
        case UI_EventDeltaUnit_Char:
        {
          cell_delta.x = (S64)evt->delta_2s32.x;
          cell_delta.y = (S64)evt->delta_2s32.y;
        }break;
        case UI_EventDeltaUnit_Word:
        case UI_EventDeltaUnit_Page:
        {
          if(evt->delta_2s32.x < 0)
          {
            cell_delta.x = -(S64)(cursor%num_columns);
          }
          else if(evt->delta_2s32.x > 0)
          {
            cell_delta.x = (num_columns-1) - (S64)(cursor%num_columns);
          }
          if(evt->delta_2s32.y < 0)
          {
            cell_delta.y = -4;
          }
          else if(evt->delta_2s32.y > 0)
          {
            cell_delta.y = +4;
          }
        }break;
      }
      B32 good_action = 0;
      if(evt->delta_2s32.x != 0 || evt->delta_2s32.y != 0)
      {
        good_action = 1;
      }
      if(good_action && evt->flags & UI_EventFlag_ZeroDeltaOnSelect && cursor != mark)
      {
        MemoryZeroStruct(&cell_delta);
      }
      if(good_action)
      {
        cell_delta.x = ClampBot(cell_delta.x, (S64)-next_cursor);
        cell_delta.y = ClampBot(cell_delta.y, (S64)-(next_cursor/num_columns));
        next_cursor += cell_delta.x;
        next_cursor += cell_delta.y*num_columns;
      }
      if(good_action && evt->flags & UI_EventFlag_PickSelectSide && cursor != mark)
      {
        if(evt->delta_2s32.x < 0 || evt->delta_2s32.y < 0)
        {
          next_cursor = Min(cursor, mark);
        }
        else
        {
          next_cursor = Max(cursor, mark);
        }
      }
      if(good_action && !(evt->flags & UI_EventFlag_KeepMark))
      {
        next_mark = next_cursor;
      }
      if(good_action)
      {
        mv->contain_cursor = 1;
        ui_eat_event(evt);
      }
    }
    cursor = next_cursor;
    mark = next_mark;
  }
  
  //////////////////////////////
  //- rjf: clamp cursor
  //
  {
    Rng1U64 cursor_valid_rng = space_range;
    if(cursor_valid_rng.max != 0)
    {
      cursor_valid_rng.max -= 1;
    }
    cursor = clamp_1u64(cursor_valid_rng, cursor);
    mark = clamp_1u64(cursor_valid_rng, mark);
  }
  
  //////////////////////////////
  //- rjf: center cursor
  //
  if(mv->center_cursor)
  {
    mv->center_cursor = 0;
    S64 cursor_row_idx = cursor/num_columns;
    S64 new_idx = (cursor_row_idx-num_possible_visible_rows/2+1);
    new_idx = clamp_1s64(scroll_idx_rng, new_idx);
    ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
  }
  
  //////////////////////////////
  //- rjf: contain cursor
  //
  if(mv->contain_cursor)
  {
    mv->contain_cursor = 0;
    S64 cursor_row_idx = cursor/num_columns;
    Rng1S64 cursor_viz_range = r1s64(clamp_1s64(scroll_idx_rng, cursor_row_idx-2), clamp_1s64(scroll_idx_rng, cursor_row_idx+3));
    S64 min_delta = Min(0, cursor_viz_range.min-viz_range_rows.min);
    S64 max_delta = Max(0, cursor_viz_range.max-viz_range_rows.max);
    S64 new_idx = scroll_pos.y.idx+min_delta+max_delta;
    new_idx = clamp_1s64(scroll_idx_rng, new_idx);
    ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
  }
  
  //////////////////////////////
  //- rjf: produce fancy string runs for all possible byte values in all cells
  //
  DR_FStrList byte_fstrs[256] = {0};
  {
    Vec4F32 full_color = ui_color_from_name(str8_lit("text"));
    Vec4F32 zero_color = full_color;
    UI_TagF("weak") zero_color = ui_color_from_name(str8_lit("text"));
    for(U64 idx = 0; idx < ArrayCount(byte_fstrs); idx += 1)
    {
      U8 byte = (U8)idx;
      F32 pct = (byte/255.f);
      Vec4F32 text_color = mix_4f32(zero_color, full_color, pct);
      if(byte == 0)
      {
        text_color.w *= 0.5f;
      }
      DR_FStr fstr = {push_str8f(scratch.arena, "%02x", byte), {font, font_raster_flags, text_color, font_size, 0, 0}};
      dr_fstrs_push(scratch.arena, &byte_fstrs[idx], &fstr);
    }
  }
  
  //////////////////////////////
  //- rjf: grab windowed memory
  //
  U64 visible_memory_size = dim_1u64(viz_range_bytes);
  U8 *visible_memory = push_array(scratch.arena, U8, visible_memory_size);
  {
    e_space_read(eval.space, visible_memory, viz_range_bytes);
  }
  
  //////////////////////////////
  //- rjf: grab annotations for windowed range of memory
  //
  typedef struct Annotation Annotation;
  struct Annotation
  {
    Annotation *next;
    String8 name_string;
    String8 kind_string;
    String8 type_string;
    Vec4F32 color;
    Rng1U64 vaddr_range;
  };
  typedef struct AnnotationList AnnotationList;
  struct AnnotationList
  {
    Annotation *first;
    Annotation *last;
  };
  AnnotationList *visible_memory_annotations = push_array(scratch.arena, AnnotationList, visible_memory_size);
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
    CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(thread);
    
    //- rjf: fill unwind frame annotations
    if(unwind.frames.count != 0) UI_Tag(str8_lit("weak"))
    {
      U64 last_stack_top = regs_rsp_from_arch_block(thread->arch, unwind.frames.v[0].regs);
      for(U64 idx = 1; idx < unwind.frames.count; idx += 1)
      {
        CTRL_UnwindFrame *f = &unwind.frames.v[idx];
        U64 f_stack_top = regs_rsp_from_arch_block(thread->arch, f->regs);
        Rng1U64 frame_vaddr_range = r1u64(last_stack_top, f_stack_top);
        Rng1U64 frame_vaddr_range_in_viz = intersect_1u64(frame_vaddr_range, viz_range_bytes);
        last_stack_top = f_stack_top;
        if(dim_1u64(frame_vaddr_range_in_viz) != 0)
        {
          U64 f_rip = regs_rip_from_arch_block(thread->arch, f->regs);
          CTRL_Entity *module = ctrl_module_from_process_vaddr(process, f_rip);
          DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
          U64 rip_voff = ctrl_voff_from_vaddr(module, f_rip);
          String8 symbol_name = d_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff, 0, 1);
          Annotation *annotation = push_array(scratch.arena, Annotation, 1);
          annotation->name_string = symbol_name.size != 0 ? symbol_name : str8_lit("[external code]");
          annotation->kind_string = str8_lit("Call Stack Frame");
          annotation->color = symbol_name.size != 0 ? ui_color_from_name(str8_lit("code_symbol")) : ui_color_from_name(str8_lit("text"));
          annotation->vaddr_range = frame_vaddr_range;
          for(U64 vaddr = frame_vaddr_range_in_viz.min; vaddr < frame_vaddr_range_in_viz.max; vaddr += 1)
          {
            U64 visible_byte_idx = vaddr - viz_range_bytes.min;
            SLLQueuePush(visible_memory_annotations[visible_byte_idx].first, visible_memory_annotations[visible_byte_idx].last, annotation);
          }
        }
      }
    }
    
    //- rjf: fill selected thread stack range annotation
    if(unwind.frames.count > 0)
    {
      U64 stack_base_vaddr = thread->stack_base;
      U64 stack_top_vaddr = regs_rsp_from_arch_block(thread->arch, unwind.frames.v[0].regs);
      Rng1U64 stack_vaddr_range = r1u64(stack_base_vaddr, stack_top_vaddr);
      Rng1U64 stack_vaddr_range_in_viz = intersect_1u64(stack_vaddr_range, viz_range_bytes);
      if(dim_1u64(stack_vaddr_range_in_viz) != 0)
      {
        Annotation *annotation = push_array(scratch.arena, Annotation, 1);
        annotation->name_string = thread->string.size ? thread->string : push_str8f(scratch.arena, "TID: %I64u", thread->id);
        annotation->kind_string = str8_lit("Stack");
        annotation->color = rd_color_from_ctrl_entity(thread);
        annotation->vaddr_range = stack_vaddr_range;
        for(U64 vaddr = stack_vaddr_range_in_viz.min; vaddr < stack_vaddr_range_in_viz.max; vaddr += 1)
        {
          U64 visible_byte_idx = vaddr - viz_range_bytes.min;
          SLLQueuePush(visible_memory_annotations[visible_byte_idx].first, visible_memory_annotations[visible_byte_idx].last, annotation);
        }
      }
    }
    
    //- rjf: fill local variable annotations
    {
      DI_Scope *scope = di_scope_open();
      Vec4F32 color_gen_table[] =
      {
        ui_color_from_name(str8_lit("code_local")),
      };
      U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, rd_regs()->unwind_count);
      for(E_String2NumMapNode *n = e_parse_state->ctx->locals_map->first; n != 0; n = n->order_next)
      {
        String8 local_name = n->string;
        E_Eval local_eval = e_eval_from_string(scratch.arena, local_name);
        if(local_eval.irtree.mode == E_Mode_Offset)
        {
          E_TypeKind local_eval_type_kind = e_type_kind_from_key(local_eval.irtree.type_key);
          U64 local_eval_type_size = e_type_byte_size_from_key(local_eval.irtree.type_key);
          Rng1U64 vaddr_rng = r1u64(local_eval.value.u64, local_eval.value.u64+local_eval_type_size);
          Rng1U64 vaddr_rng_in_visible = intersect_1u64(viz_range_bytes, vaddr_rng);
          if(vaddr_rng_in_visible.max != vaddr_rng_in_visible.min)
          {
            Annotation *annotation = push_array(scratch.arena, Annotation, 1);
            {
              annotation->name_string = push_str8_copy(scratch.arena, local_name);
              annotation->kind_string = str8_lit("Local");
              annotation->type_string = e_type_string_from_key(scratch.arena, local_eval.irtree.type_key);
              annotation->color = color_gen_table[(vaddr_rng.min/8)%ArrayCount(color_gen_table)];
              annotation->vaddr_range = vaddr_rng;
            }
            for(U64 vaddr = vaddr_rng_in_visible.min; vaddr < vaddr_rng_in_visible.max; vaddr += 1)
            {
              SLLQueuePushFront(visible_memory_annotations[vaddr-viz_range_bytes.min].first, visible_memory_annotations[vaddr-viz_range_bytes.min].last, annotation);
            }
          }
        }
      }
      di_scope_close(scope);
    }
  }
  
  //////////////////////////////
  //- rjf: build main container
  //
  UI_Box *container_box = &ui_nil_box;
  {
    Vec2F32 dim = dim_2f32(rect);
    ui_set_next_fixed_width(dim.x);
    ui_set_next_fixed_height(dim.y);
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(0, "memory_view_container");
  }
  
  //////////////////////////////
  //- rjf: build header
  //
  UI_Box *header_box = &ui_nil_box;
  UI_Parent(container_box)
  {
    UI_WidthFill UI_PrefHeight(ui_px(row_height_px, 1.f)) UI_Row
      header_box = ui_build_box_from_stringf(UI_BoxFlag_DrawSideBottom, "table_header");
    UI_Parent(header_box)
      RD_Font(RD_FontSlot_Code)
      UI_FontSize(font_size)
      UI_TagF("weak")
    {
      UI_PrefWidth(ui_px(big_glyph_advance*20.f, 1.f)) ui_labelf("Address");
      UI_PrefWidth(ui_px(cell_width_px, 1.f))
        UI_TextAlignment(UI_TextAlign_Center)
      {
        Rng1U64 col_selection_rng = r1u64(cursor%num_columns, mark%num_columns);
        for(U64 row_off = 0; row_off < num_columns*bytes_per_cell; row_off += bytes_per_cell)
        {
          ui_labelf("%I64X", row_off);
        }
      }
      ui_spacer(ui_px(big_glyph_advance*1.5f, 1.f));
      UI_WidthFill ui_labelf("ASCII");
    }
  }
  
  //////////////////////////////
  //- rjf: build scroll bar
  //
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(content_rect.x1);
    ui_set_next_fixed_y(content_rect.y0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(dim_2f32(content_rect).y);
    {
      scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                   ui_px(scroll_bar_dim, 1.f),
                                   scroll_pos.y,
                                   scroll_idx_rng,
                                   num_possible_visible_rows);
    }
  }
  
  //////////////////////////////
  //- rjf: build scrollable box
  //
  UI_Box *scrollable_box = &ui_nil_box;
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(content_rect.x0);
    ui_set_next_fixed_y(content_rect.y0);
    ui_set_next_fixed_width(dim_2f32(content_rect).x);
    ui_set_next_fixed_height(dim_2f32(content_rect).y);
    ui_set_next_child_layout_axis(Axis2_Y);
    scrollable_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                               UI_BoxFlag_Scroll|
                                               UI_BoxFlag_AllowOverflowX|
                                               UI_BoxFlag_AllowOverflowY,
                                               "scrollable_box");
    container_box->view_off.x = container_box->view_off_target.x = scroll_pos.x.idx + scroll_pos.x.off;
    scrollable_box->view_off.y = scrollable_box->view_off_target.y = floor_f32(row_height_px*mod_f32(scroll_pos.y.off, 1.f) + row_height_px*(scroll_pos.y.off < 0));
  }
  
  //////////////////////////////
  //- rjf: build row container/overlay
  //
  UI_Box *row_container_box = &ui_nil_box;
  UI_Box *row_overlay_box = &ui_nil_box;
  UI_Parent(scrollable_box) UI_WidthFill UI_HeightFill
  {
    ui_set_next_child_layout_axis(Axis2_Y);
    ui_set_next_hover_cursor(OS_Cursor_IBar);
    row_container_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "row_container");
    UI_Parent(row_container_box)
    {
      row_overlay_box = ui_build_box_from_stringf(UI_BoxFlag_Floating, "row_overlay");
    }
  }
  
  //////////////////////////////
  //- rjf: interact with row container
  //
  U64 mouse_hover_byte_num = 0;
  {
    UI_Signal sig = ui_signal_from_box(row_container_box);
    
    // rjf: calculate hovered byte
    if(ui_hovering(sig) || ui_dragging(sig))
    {
      Vec2F32 mouse_rel = sub_2f32(ui_mouse(), row_container_box->rect.p0);
      U64 row_idx = ClampBot(0, mouse_rel.y) / row_height_px;
      
      // rjf: try from cells
      if(mouse_hover_byte_num == 0)
      {
        U64 col_idx = ClampBot(mouse_rel.x-big_glyph_advance*20.f, 0)/cell_width_px;
        if(col_idx < num_columns)
        {
          mouse_hover_byte_num = viz_range_bytes.min + row_idx*num_columns + col_idx + 1;
        }
      }
      
      // rjf: try from ascii
      if(mouse_hover_byte_num == 0)
      {
        U64 col_idx = ClampBot(mouse_rel.x - (big_glyph_advance*20.f + cell_width_px*num_columns + big_glyph_advance*1.5f), 0)/big_glyph_advance;
        col_idx = ClampTop(col_idx, num_columns-1);
        mouse_hover_byte_num = viz_range_bytes.min + row_idx*num_columns + col_idx + 1;
      }
      
      mouse_hover_byte_num = Clamp(1, mouse_hover_byte_num, 0x7FFFFFFFFFFFull+1);
    }
    
    // rjf: press -> focus panel
    if(ui_pressed(sig))
    {
      rd_cmd(RD_CmdKind_FocusPanel);
    }
    
    // rjf: click & drag -> select
    if(ui_dragging(sig) && mouse_hover_byte_num != 0)
    {
      if(!contains_2f32(sig.box->rect, ui_mouse()))
      {
        mv->contain_cursor = 1;
      }
      cursor = mouse_hover_byte_num-1;
      cursor = clamp_1u64(space_range, cursor);
      if(ui_pressed(sig))
      {
        mark = cursor;
      }
    }
    
    // rjf: ctrl+scroll -> change font size
    if(ui_hovering(sig))
    {
      for(UI_Event *evt = 0; ui_next_event(&evt);)
      {
        if(evt->kind == UI_EventKind_Scroll && evt->modifiers & OS_Modifier_Ctrl)
        {
          ui_eat_event(evt);
          if(evt->delta_2f32.y < 0)
          {
            rd_cmd(RD_CmdKind_IncCodeFontScale);
          }
          else if(evt->delta_2f32.y > 0)
          {
            rd_cmd(RD_CmdKind_DecCodeFontScale);
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build rows
  //
  UI_Parent(row_container_box) RD_Font(RD_FontSlot_Code) UI_FontSize(font_size)
  {
    Rng1U64 selection = r1u64(cursor, mark);
    U8 *row_ascii_buffer = push_array(scratch.arena, U8, num_columns);
    UI_WidthFill UI_PrefHeight(ui_px(row_height_px, 1.f))
      for(S64 row_idx = viz_range_rows.min; row_idx <= viz_range_rows.max; row_idx += 1)
    {
      Rng1U64 row_range_bytes = r1u64(space_range.min + row_idx*num_columns,
                                      space_range.min + (row_idx+1)*num_columns);
      if(row_range_bytes.min >= space_range.max)
      {
        break;
      }
      B32 row_is_boundary = 0;
      if(row_range_bytes.min%64 == 0)
      {
        row_is_boundary = 1;
        ui_set_next_tag(str8_lit("pop"));
      }
      UI_Box *row = ui_build_box_from_stringf(UI_BoxFlag_DrawSideTop*!!row_is_boundary, "row_%I64x", row_range_bytes.min);
      UI_Parent(row)
      {
        UI_PrefWidth(ui_px(big_glyph_advance*20.f, 1.f))
        {
          if(!(selection.max >= row_range_bytes.min && selection.min < row_range_bytes.max))
          {
            ui_set_next_tag(str8_lit("weak"));
          }
          ui_labelf("0x%016I64X", row_range_bytes.min);
        }
        UI_PrefWidth(ui_px(cell_width_px, 1.f))
          UI_TextAlignment(UI_TextAlign_Center)
          UI_CornerRadius(0)
        {
          for(U64 col_idx = 0; col_idx < num_columns; col_idx += 1)
          {
            // rjf: unpack information about this slot
            U64 visible_byte_idx = (row_idx-viz_range_rows.min)*num_columns + col_idx;
            U64 global_byte_idx = viz_range_bytes.min+visible_byte_idx;
            U64 global_byte_num = global_byte_idx+1;
            
            // rjf: build space, if this cell is out-of-range
            if(global_byte_idx >= viz_range_bytes.max)
            {
              ui_build_box_from_key(0, ui_key_zero());
            }
            
            // rjf: build actual cell
            else
            {
              // rjf: unpack byte info
              U8 byte_value = visible_memory[visible_byte_idx];
              Annotation *annotation = visible_memory_annotations[visible_byte_idx].first;
              
              // rjf: unpack visual cell info
              UI_BoxFlags cell_flags = 0;
              Vec4F32 cell_bg_rgba = {0};
              if(global_byte_num == mouse_hover_byte_num)
              {
                cell_flags |= UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight;
              }
              if(annotation != 0)
              {
                cell_flags |= UI_BoxFlag_DrawBackground;
                cell_bg_rgba = annotation->color;
                if(contains_1u64(annotation->vaddr_range, mouse_hover_byte_num-1))
                {
                  cell_bg_rgba.w *= 0.15f;
                }
                else
                {
                  cell_bg_rgba.w *= 0.08f;
                }
              }
              if(selection.min <= global_byte_idx && global_byte_idx <= selection.max)
              {
                cell_flags |= UI_BoxFlag_DrawBackground;
                cell_bg_rgba = selection_color;
                cell_bg_rgba.w *= 0.2f;
              }
              
              // rjf: build
              ui_set_next_background_color(cell_bg_rgba);
              UI_Box *cell_box = ui_build_box_from_key(UI_BoxFlag_DrawText|cell_flags, ui_key_zero());
              ui_box_equip_display_fstrs(cell_box, &byte_fstrs[byte_value]);
              {
                F32 off = 0;
                for(Annotation *a = annotation; a != 0; a = a->next)
                {
                  if(global_byte_idx == a->vaddr_range.min) UI_Parent(row_overlay_box)
                  {
                    ui_set_next_background_color(annotation->color);
                    ui_set_next_fixed_x(big_glyph_advance*20.f + col_idx*cell_width_px + -cell_width_px/8.f + off);
                    ui_set_next_fixed_y((row_idx-viz_range_rows.min)*row_height_px + -cell_width_px/8.f);
                    ui_set_next_fixed_width(cell_width_px/4.f);
                    ui_set_next_fixed_height(cell_width_px/4.f);
                    ui_set_next_corner_radius_00(cell_width_px/8.f);
                    ui_set_next_corner_radius_01(cell_width_px/8.f);
                    ui_set_next_corner_radius_10(cell_width_px/8.f);
                    ui_set_next_corner_radius_11(cell_width_px/8.f);
                    ui_build_box_from_key(UI_BoxFlag_Floating|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, ui_key_zero());
                    off += cell_width_px/8.f + cell_width_px/16.f;
                  }
                }
              }
              if(annotation != 0 && mouse_hover_byte_num == global_byte_num) UI_Tooltip UI_FontSize(ui_top_font_size()) UI_PrefHeight(ui_px(ui_top_font_size()*1.75f, 1.f))
              {
                for(Annotation *a = annotation; a != 0; a = a->next)
                {
                  UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(10, 1))
                  {
                    RD_Font(RD_FontSlot_Code) ui_label(a->name_string);
                    RD_Font(RD_FontSlot_Main) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_label(a->kind_string);
                  }
                  if(a->type_string.size != 0)
                  {
                    rd_code_label(1.f, 1, ui_color_from_name(str8_lit("code_type")), a->type_string);
                  }
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_label(str8_from_memory_size(scratch.arena, dim_1u64(a->vaddr_range)));
                  if(a->next != 0)
                  {
                    ui_spacer(ui_em(1.5f, 1.f));
                  }
                }
              }
            }
          }
        }
        ui_spacer(ui_px(big_glyph_advance*1.5f, 1.f));
        UI_WidthFill
        {
          MemoryZero(row_ascii_buffer, num_columns);
          U64 num_bytes_this_row = 0;
          for(U64 col_idx = 0; col_idx < num_columns; col_idx += 1)
          {
            U64 visible_byte_idx = (row_idx-viz_range_rows.min)*num_columns + col_idx;
            if(visible_byte_idx < visible_memory_size)
            {
              U8 byte_value = visible_memory[visible_byte_idx];
              row_ascii_buffer[col_idx] = byte_value;
              if(byte_value <= 32 || 127 < byte_value)
              {
                row_ascii_buffer[col_idx] = '.';
              }
              num_bytes_this_row += 1;
            }
          }
          String8 ascii_text = str8(row_ascii_buffer, num_bytes_this_row);
          UI_Box *ascii_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S###ascii_row_%I64x", ascii_text, row_range_bytes.min);
          if(selection.max >= row_range_bytes.min && selection.min < row_range_bytes.max)
          {
            Rng1U64 selection_in_row = intersect_1u64(row_range_bytes, selection);
            DR_Bucket *bucket = dr_bucket_make();
            DR_BucketScope(bucket)
            {
              Vec2F32 text_pos = ui_box_text_position(ascii_box);
              Vec4F32 color = selection_color;
              color.w *= 0.2f;
              dr_rect(r2f32p(text_pos.x + fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_prefix(ascii_text, selection_in_row.min+0-row_range_bytes.min)).x - font_size/8.f,
                             ascii_box->rect.y0,
                             text_pos.x + fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_prefix(ascii_text, selection_in_row.max+1-row_range_bytes.min)).x + font_size/4.f,
                             ascii_box->rect.y1),
                      color,
                      0, 0, 1.f);
            }
            ui_box_equip_draw_bucket(ascii_box, bucket);
          }
          if(mouse_hover_byte_num != 0 && contains_1u64(row_range_bytes, mouse_hover_byte_num-1))
          {
            DR_Bucket *bucket = dr_bucket_make();
            DR_BucketScope(bucket)
            {
              Vec2F32 text_pos = ui_box_text_position(ascii_box);
              Vec4F32 color = border_color;
              dr_rect(r2f32p(text_pos.x + fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_prefix(ascii_text, mouse_hover_byte_num-1-row_range_bytes.min)).x - font_size/8.f,
                             ascii_box->rect.y0,
                             text_pos.x + fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_prefix(ascii_text, mouse_hover_byte_num+0-row_range_bytes.min)).x + font_size/4.f,
                             ascii_box->rect.y1),
                      color,
                      1.f, 3.f, 1.f);
            }
            ui_box_equip_draw_bucket(ascii_box, bucket);
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build footer
  //
  UI_Box *footer_box = &ui_nil_box;
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(footer_rect.x0);
    ui_set_next_fixed_y(footer_rect.y0);
    ui_set_next_fixed_width(dim_2f32(footer_rect).x);
    ui_set_next_fixed_height(dim_2f32(footer_rect).y);
    footer_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, "footer");
    UI_Parent(footer_box) RD_Font(RD_FontSlot_Code) UI_FontSize(font_size)
    {
      UI_PrefWidth(ui_em(7.5f, 1.f)) UI_HeightFill UI_Column UI_TagF("weak")
        UI_PrefHeight(ui_px(row_height_px, 0.f))
      {
        ui_labelf("Address:");
        ui_labelf("U8:");
        ui_labelf("U16:");
        ui_labelf("U32:");
        ui_labelf("U64:");
      }
      UI_PrefWidth(ui_em(45.f, 1.f)) UI_HeightFill UI_Column
        UI_PrefHeight(ui_px(row_height_px, 0.f))
      {
        B32 cursor_in_range = (viz_range_bytes.min <= cursor && cursor+8 <= viz_range_bytes.max);
        ui_labelf("%016I64X", cursor);
        if(cursor_in_range)
        {
          U64 as_u8  = 0;
          U64 as_u16 = 0;
          U64 as_u32 = 0;
          U64 as_u64 = 0;
          U64 cursor_off = cursor-viz_range_bytes.min;
          as_u8  = (U64)*(U8 *)(visible_memory + cursor_off);
          as_u16 = (U64)*(U16*)(visible_memory + cursor_off);
          as_u32 = (U64)*(U32*)(visible_memory + cursor_off);
          as_u64 = (U64)*(U64*)(visible_memory + cursor_off);
          ui_labelf("%02X (%I64u)",  as_u8,  as_u8);
          ui_labelf("%04X (%I64u)",  as_u16, as_u16);
          ui_labelf("%08X (%I64u)",  as_u32, as_u32);
          ui_labelf("%016I64X (%I64u)", as_u64, as_u64);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: scroll
  //
  {
    UI_Signal sig = ui_signal_from_box(scrollable_box);
    if(sig.scroll.y != 0)
    {
      S64 new_idx = scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng, new_idx);
      ui_scroll_pt_target_idx(&scroll_pos.y, new_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: save parameters
  //
  rd_store_view_param_u64(str8_lit("cursor_vaddr"), cursor);
  rd_store_view_param_u64(str8_lit("mark_vaddr"), mark);
  rd_store_view_param_u64(str8_lit("bytes_per_cell"), bytes_per_cell);
  rd_store_view_param_u64(str8_lit("num_columns"), num_columns);
  rd_store_view_scroll_pos(scroll_pos);
  
  hs_scope_close(hs_scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: "graph"

EV_EXPAND_RULE_INFO_FUNCTION_DEF(graph)
{
  EV_ExpandInfo info = {0};
  info.row_count = 8;
  info.single_item = 1;
  return info;
}

////////////////////////////////
//~ rjf: bitmap @view_hook_impl

typedef struct RD_BitmapBoxDrawData RD_BitmapBoxDrawData;
struct RD_BitmapBoxDrawData
{
  Rng2F32 src;
  R_Handle texture;
  F32 loaded_t;
  B32 hovered;
  Vec2S32 mouse_px;
  F32 ui_per_bmp_px;
};

typedef struct RD_BitmapCanvasBoxDrawData RD_BitmapCanvasBoxDrawData;
struct RD_BitmapCanvasBoxDrawData
{
  Vec2F32 view_center_pos;
  F32 zoom;
};

internal Vec2F32
rd_bitmap_screen_from_canvas_pos(Vec2F32 view_center_pos, F32 zoom, Rng2F32 rect, Vec2F32 cvs)
{
  Vec2F32 scr =
  {
    (rect.x0+rect.x1)/2 + (cvs.x - view_center_pos.x) * zoom,
    (rect.y0+rect.y1)/2 + (cvs.y - view_center_pos.y) * zoom,
  };
  return scr;
}

internal Rng2F32
rd_bitmap_screen_from_canvas_rect(Vec2F32 view_center_pos, F32 zoom, Rng2F32 rect, Rng2F32 cvs)
{
  Rng2F32 scr = r2f32(rd_bitmap_screen_from_canvas_pos(view_center_pos, zoom, rect, cvs.p0), rd_bitmap_screen_from_canvas_pos(view_center_pos, zoom, rect, cvs.p1));
  return scr;
}

internal Vec2F32
rd_bitmap_canvas_from_screen_pos(Vec2F32 view_center_pos, F32 zoom, Rng2F32 rect, Vec2F32 scr)
{
  Vec2F32 cvs =
  {
    (scr.x - (rect.x0+rect.x1)/2) / zoom + view_center_pos.x,
    (scr.y - (rect.y0+rect.y1)/2) / zoom + view_center_pos.y,
  };
  return cvs;
}

internal Rng2F32
rd_bitmap_canvas_from_screen_rect(Vec2F32 view_center_pos, F32 zoom, Rng2F32 rect, Rng2F32 scr)
{
  Rng2F32 cvs = r2f32(rd_bitmap_canvas_from_screen_pos(view_center_pos, zoom, rect, scr.p0), rd_bitmap_canvas_from_screen_pos(view_center_pos, zoom, rect, scr.p1));
  return cvs;
}

internal UI_BOX_CUSTOM_DRAW(rd_bitmap_view_canvas_box_draw)
{
  RD_BitmapCanvasBoxDrawData *draw_data = (RD_BitmapCanvasBoxDrawData *)user_data;
  Rng2F32 rect_scrn = box->rect;
  Rng2F32 rect_cvs = rd_bitmap_canvas_from_screen_rect(draw_data->view_center_pos, draw_data->zoom, rect_scrn, rect_scrn);
  F32 grid_cell_size_cvs = box->font_size*10.f;
  F32 grid_line_thickness_px = Max(2.f, box->font_size*0.1f);
  Vec4F32 grid_line_color = {0};
  UI_TagF("weak")
  {
    grid_line_color = ui_color_from_name(str8_lit("text"));
  }
  for EachEnumVal(Axis2, axis)
  {
    for(F32 v = rect_cvs.p0.v[axis] - mod_f32(rect_cvs.p0.v[axis], grid_cell_size_cvs);
        v < rect_cvs.p1.v[axis];
        v += grid_cell_size_cvs)
    {
      Vec2F32 p_cvs = {0};
      p_cvs.v[axis] = v;
      Vec2F32 p_scr = rd_bitmap_screen_from_canvas_pos(draw_data->view_center_pos, draw_data->zoom, rect_scrn, p_cvs);
      Rng2F32 rect = {0};
      rect.p0.v[axis] = p_scr.v[axis] - grid_line_thickness_px/2;
      rect.p1.v[axis] = p_scr.v[axis] + grid_line_thickness_px/2;
      rect.p0.v[axis2_flip(axis)] = box->rect.p0.v[axis2_flip(axis)];
      rect.p1.v[axis2_flip(axis)] = box->rect.p1.v[axis2_flip(axis)];
      dr_rect(rect, grid_line_color, 0, 0, 1.f);
    }
  }
}

EV_EXPAND_RULE_INFO_FUNCTION_DEF(bitmap)
{
  EV_ExpandInfo info = {0};
  info.row_count = 8;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(bitmap)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  TEX_Scope *tex_scope = tex_scope_open();
  
  //////////////////////////////
  //- rjf: evaluate expression
  //
  Vec2S32 dim = rd_dim2s32_from_eval_tag(eval, tag);
  R_Tex2DFormat fmt = rd_tex2dformat_from_eval_tag(eval, tag);
  U64 base_offset = rd_base_offset_from_eval(eval);
  U64 expected_size = dim.x*dim.y*r_tex2d_format_bytes_per_pixel_table[fmt];
  Rng1U64 offset_range = r1u64(base_offset, base_offset + expected_size);
  
  //////////////////////////////
  //- rjf: unpack params
  //
  F32 zoom = rd_view_cfg_value_from_string(str8_lit("zoom")).f32;
  Vec2F32 view_center_pos =
  {
    rd_view_cfg_value_from_string(str8_lit("x")).f32,
    rd_view_cfg_value_from_string(str8_lit("y")).f32,
  };
  if(zoom == 0)
  {
    F32 available_dim_y = dim_2f32(rect).y;
    F32 image_dim_y = (F32)dim.y;
    if(image_dim_y != 0)
    {
      zoom = (available_dim_y / image_dim_y) * 0.8f;
    }
    else
    {
      zoom = 1.f;
    }
  }
  
  //////////////////////////////
  //- rjf: map expression artifacts -> texture
  //
  U128 texture_key = rd_key_from_eval_space_range(eval.space, offset_range, 0);
  TEX_Topology topology = tex_topology_make(dim, fmt);
  U128 data_hash = {0};
  R_Handle texture = tex_texture_from_key_topology(tex_scope, texture_key, topology, &data_hash);
  String8 data = hs_data_from_hash(hs_scope, data_hash);
  
  //////////////////////////////
  //- rjf: equip loading info
  //
  if(offset_range.max != offset_range.min &&
     eval.msgs.max_kind == E_MsgKind_Null &&
     (u128_match(data_hash, u128_zero()) ||
      r_handle_match(texture, r_handle_zero()) ||
      data.size == 0))
  {
    rd_store_view_loading_info(1, 0, 0);
  }
  
  //////////////////////////////
  //- rjf: build canvas box
  //
  UI_Box *canvas_box = &ui_nil_box;
  Vec2F32 canvas_dim = dim_2f32(rect);
  Rng2F32 canvas_rect = r2f32p(0, 0, canvas_dim.x, canvas_dim.y);
  UI_Rect(canvas_rect)
  {
    canvas_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_Clickable|UI_BoxFlag_Scroll, "bmp_canvas");
  }
  
  //////////////////////////////
  //- rjf: canvas dragging
  //
  UI_Signal canvas_sig = ui_signal_from_box(canvas_box);
  {
    if(ui_dragging(canvas_sig))
    {
      if(ui_pressed(canvas_sig))
      {
        rd_cmd(RD_CmdKind_FocusPanel);
        ui_store_drag_struct(&view_center_pos);
      }
      Vec2F32 start_view_center_pos = *ui_get_drag_struct(Vec2F32);
      Vec2F32 drag_delta_scr = ui_drag_delta();
      Vec2F32 drag_delta_cvs = scale_2f32(drag_delta_scr, 1.f/zoom);
      Vec2F32 new_view_center_pos = sub_2f32(start_view_center_pos, drag_delta_cvs);
      view_center_pos = new_view_center_pos;
    }
    if(canvas_sig.scroll.y != 0)
    {
      F32 new_zoom = zoom - zoom*canvas_sig.scroll.y/10.f;
      new_zoom = Clamp(1.f/256.f, new_zoom, 256.f);
      Vec2F32 mouse_scr_pre = sub_2f32(ui_mouse(), rect.p0);
      Vec2F32 mouse_cvs = rd_bitmap_canvas_from_screen_pos(view_center_pos, zoom, canvas_rect, mouse_scr_pre);
      zoom = new_zoom;
      Vec2F32 mouse_scr_pst = rd_bitmap_screen_from_canvas_pos(view_center_pos, zoom, canvas_rect, mouse_cvs);
      Vec2F32 drift_scr = sub_2f32(mouse_scr_pst, mouse_scr_pre);
      view_center_pos = add_2f32(view_center_pos, scale_2f32(drift_scr, 1.f/new_zoom));
    }
    if(ui_double_clicked(canvas_sig))
    {
      ui_kill_action();
      MemoryZeroStruct(&view_center_pos);
      zoom = 1.f;
    }
  }
  
  //////////////////////////////
  //- rjf: equip canvas draw info
  //
  {
    RD_BitmapCanvasBoxDrawData *draw_data = push_array(ui_build_arena(), RD_BitmapCanvasBoxDrawData, 1);
    draw_data->view_center_pos = view_center_pos;
    draw_data->zoom = zoom;
    ui_box_equip_custom_draw(canvas_box, rd_bitmap_view_canvas_box_draw, draw_data);
  }
  
  //////////////////////////////
  //- rjf: calculate image coordinates
  //
  Rng2F32 img_rect_cvs = r2f32p(-topology.dim.x/2, -topology.dim.y/2, +topology.dim.x/2, +topology.dim.y/2);
  Rng2F32 img_rect_scr = rd_bitmap_screen_from_canvas_rect(view_center_pos, zoom, canvas_rect, img_rect_cvs);
  
  //////////////////////////////
  //- rjf: image-region canvas interaction
  //
  Vec2S32 mouse_bmp = {-1, -1};
  if(ui_hovering(canvas_sig) && !ui_dragging(canvas_sig))
  {
    Vec2F32 mouse_scr = sub_2f32(ui_mouse(), rect.p0);
    Vec2F32 mouse_cvs = rd_bitmap_canvas_from_screen_pos(view_center_pos, zoom, canvas_rect, mouse_scr);
    if(contains_2f32(img_rect_cvs, mouse_cvs))
    {
      mouse_bmp = v2s32((S32)(mouse_cvs.x-img_rect_cvs.x0), (S32)(mouse_cvs.y-img_rect_cvs.y0));
      S64 off_px = mouse_bmp.y*topology.dim.x + mouse_bmp.x;
      S64 off_bytes = off_px*r_tex2d_format_bytes_per_pixel_table[topology.fmt];
      if(0 <= off_bytes && off_bytes+r_tex2d_format_bytes_per_pixel_table[topology.fmt] <= data.size &&
         r_tex2d_format_bytes_per_pixel_table[topology.fmt] != 0)
      {
        B32 color_is_good = 1;
        Vec4F32 color = {0};
        switch(topology.fmt)
        {
          default:{color_is_good = 0;}break;
          case R_Tex2DFormat_R8:     {color = v4f32(((U8 *)(data.str+off_bytes))[0]/255.f, 0, 0, 1);}break;
          case R_Tex2DFormat_RG8:    {color = v4f32(((U8 *)(data.str+off_bytes))[0]/255.f, ((U8 *)(data.str+off_bytes))[1]/255.f, 0, 1);}break;
          case R_Tex2DFormat_RGBA8:  {color = v4f32(((U8 *)(data.str+off_bytes))[0]/255.f, ((U8 *)(data.str+off_bytes))[1]/255.f, ((U8 *)(data.str+off_bytes))[2]/255.f, ((U8 *)(data.str+off_bytes))[3]/255.f);}break;
          case R_Tex2DFormat_BGRA8:  {color = v4f32(((U8 *)(data.str+off_bytes))[3]/255.f, ((U8 *)(data.str+off_bytes))[2]/255.f, ((U8 *)(data.str+off_bytes))[1]/255.f, ((U8 *)(data.str+off_bytes))[0]/255.f);}break;
          case R_Tex2DFormat_R16:    {color = v4f32(((U16 *)(data.str+off_bytes))[0]/(F32)max_U16, 0, 0, 1);}break;
          case R_Tex2DFormat_RGBA16: {color = v4f32(((U16 *)(data.str+off_bytes))[0]/(F32)max_U16, ((U16 *)(data.str+off_bytes))[1]/(F32)max_U16, ((U16 *)(data.str+off_bytes))[2]/(F32)max_U16, ((U16 *)(data.str+off_bytes))[3]/(F32)max_U16);}break;
          case R_Tex2DFormat_R32:    {color = v4f32(((F32 *)(data.str+off_bytes))[0], 0, 0, 1);}break;
          case R_Tex2DFormat_RG32:   {color = v4f32(((F32 *)(data.str+off_bytes))[0], ((F32 *)(data.str+off_bytes))[1], 0, 1);}break;
          case R_Tex2DFormat_RGBA32: {color = v4f32(((F32 *)(data.str+off_bytes))[0], ((F32 *)(data.str+off_bytes))[1], ((F32 *)(data.str+off_bytes))[2], ((F32 *)(data.str+off_bytes))[3]);}break;
        }
        if(color_is_good)
        {
          Vec4F32 hsva = hsva_from_rgba(color);
          ui_do_color_tooltip_hsva(hsva);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build image
  //
  UI_Parent(canvas_box)
  {
    if(0 <= mouse_bmp.x && mouse_bmp.x < dim.x &&
       0 <= mouse_bmp.x && mouse_bmp.x < dim.y)
    {
      F32 pixel_size_scr = 1.f*zoom;
      Rng2F32 indicator_rect_scr = r2f32p(img_rect_scr.x0 + mouse_bmp.x*pixel_size_scr,
                                          img_rect_scr.y0 + mouse_bmp.y*pixel_size_scr,
                                          img_rect_scr.x0 + (mouse_bmp.x+1)*pixel_size_scr,
                                          img_rect_scr.y0 + (mouse_bmp.y+1)*pixel_size_scr);
      UI_Rect(indicator_rect_scr)
      {
        ui_build_box_from_key(UI_BoxFlag_DrawBorder|UI_BoxFlag_Floating, ui_key_zero());
      }
    }
    UI_Rect(img_rect_scr) UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_Floating)
    {
      ui_image(texture, R_Tex2DSampleKind_Nearest, r2f32p(0, 0, (F32)dim.x, (F32)dim.y), v4f32(1, 1, 1, 1), 0, str8_lit("bmp_image"));
    }
  }
  
  //////////////////////////////
  //- rjf: store params
  //
  rd_store_view_param_f32(str8_lit("zoom"), zoom);
  rd_store_view_param_f32(str8_lit("x"), view_center_pos.x);
  rd_store_view_param_f32(str8_lit("y"), view_center_pos.y);
  
  hs_scope_close(hs_scope);
  tex_scope_close(tex_scope);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: "checkbox"

RD_VIEW_UI_FUNCTION_DEF(checkbox)
{
  E_Eval value_eval = e_value_eval_from_eval(eval);
  if(ui_clicked(rd_icon_buttonf(value_eval.value.u64 == 0 ? RD_IconKind_CheckHollow : RD_IconKind_CheckFilled, 0, "###check")))
  {
    rd_commit_eval_value_string(eval, value_eval.value.u64 == 0 ? str8_lit("1") : str8_lit("0"), 0);
  }
}

////////////////////////////////
//~ rjf: color_rgba @view_hook_impl

internal Vec4F32
rd_rgba_from_eval_params(E_Eval eval, MD_Node *params)
{
  Vec4F32 rgba = {0};
  {
    E_Eval value_eval = e_value_eval_from_eval(eval);
    E_TypeKey type_key = eval.irtree.type_key;
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    U64 type_size = e_type_byte_size_from_key(type_key);
    if(16 <= type_size)
    {
      e_space_read(eval.space, &rgba, r1u64(eval.value.u64, eval.value.u64 + 16));
    }
    else if(4 <= type_size)
    {
      U32 hex_val = value_eval.value.u32;
      rgba = rgba_from_u32(hex_val);
    }
  }
  return rgba;
}

EV_EXPAND_RULE_INFO_FUNCTION_DEF(color_rgba)
{
  EV_ExpandInfo info = {0};
  info.row_count = 8;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(color_rgba)
{
  Temp scratch = scratch_begin(0, 0);
  Vec2F32 dim = dim_2f32(rect);
  F32 padding = ui_top_font_size()*3.f;
  Vec4F32 rgba = {0}; // TODO(rjf): @cfg rd_rgba_from_eval_params(eval, params);
  Vec4F32 hsva = hsva_from_rgba(rgba);
  
  //- rjf: too small -> just show components
  if(dim.y <= ui_top_font_size()*8.f)
  {
    //- rjf: build text box
    UI_Box *text_box = &ui_nil_box;
    UI_WidthFill RD_Font(RD_FontSlot_Code)
    {
      text_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
      DR_FStrList fstrs = {0};
      {
        DR_FStrParams params = {ui_top_font(), ui_top_text_raster_flags(), ui_color_from_name(str8_lit("text")), ui_top_font_size()};
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, str8_lit("("));
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, push_str8f(scratch.arena, "%.2f", rgba.x), .color = v4f32(1.f, 0.25f, 0.25f, 1.f), .underline_thickness = 4.f);
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, str8_lit(", "));
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, push_str8f(scratch.arena, "%.2f", rgba.y), .color = v4f32(0.25f, 1.f, 0.25f, 1.f), .underline_thickness = 4.f);
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, str8_lit(", "));
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, push_str8f(scratch.arena, "%.2f", rgba.z), .color = v4f32(0.25f, 0.25f, 1.f, 1.f), .underline_thickness = 4.f);
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, str8_lit(", "));
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, push_str8f(scratch.arena, "%.2f", rgba.w), .color = v4f32(1.f,   1.f,   1.f, 1.f), .underline_thickness = 4.f);
        dr_fstrs_push_new(scratch.arena, &fstrs, &params, str8_lit(")"));
      }
      ui_box_equip_display_fstrs(text_box, &fstrs);
    }
    
    //- rjf: build color box
    UI_Box *color_box = &ui_nil_box;
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
  }
  
  //- rjf: large enough -> full color picker
  else
  {
    UI_WidthFill UI_HeightFill UI_Column UI_Padding(ui_px(padding, 1.f)) UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_HeightFill
    {
      UI_PrefWidth(ui_px(dim.y - padding*2, 1.f))
      {
        UI_Signal sv_sig = ui_sat_val_pickerf(hsva.x, &hsva.y, &hsva.z, "sat_val_picker");
      }
      UI_PrefWidth(ui_em(3.f, 1.f))
      {
        UI_Signal h_sig  = ui_hue_pickerf(&hsva.x, hsva.y, hsva.z, "hue_picker");
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Column UI_PrefWidth(ui_text_dim(10, 1)) UI_PrefHeight(ui_em(2.f, 0.f)) RD_Font(RD_FontSlot_Code)
        UI_TagF("weak")
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
      UI_PrefWidth(ui_children_sum(1)) UI_Column UI_PrefWidth(ui_text_dim(10, 1)) UI_PrefHeight(ui_em(2.f, 0.f)) RD_Font(RD_FontSlot_Code)
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
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: geo3d @view_hook_impl

typedef struct RD_Geo3DViewState RD_Geo3DViewState;
struct RD_Geo3DViewState
{
  F32 yaw;
  F32 pitch;
  F32 zoom;
};

typedef struct RD_Geo3DBoxDrawData RD_Geo3DBoxDrawData;
struct RD_Geo3DBoxDrawData
{
  F32 yaw;
  F32 pitch;
  F32 zoom;
  R_Handle vertex_buffer;
  R_Handle index_buffer;
};

internal UI_BOX_CUSTOM_DRAW(rd_geo3d_box_draw)
{
  RD_Geo3DBoxDrawData *draw_data = (RD_Geo3DBoxDrawData *)user_data;
  
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
  Vec3F32 eye = v3f32(draw_data->zoom*cos_f32(draw_data->yaw)*sin_f32(draw_data->pitch),
                      draw_data->zoom*sin_f32(draw_data->yaw)*sin_f32(draw_data->pitch),
                      draw_data->zoom*cos_f32(draw_data->pitch));
  
  // rjf: mesh
  Vec2F32 box_dim = dim_2f32(box->rect);
  R_PassParams_Geo3D *pass = dr_geo3d_begin(box->rect,
                                            make_look_at_4x4f32(eye, target, v3f32(0, 0, 1)),
                                            make_perspective_4x4f32(0.25f, box_dim.x/box_dim.y, 0.1f, 500.f));
  pass->clip = clip;
  dr_mesh(draw_data->vertex_buffer, draw_data->index_buffer, R_GeoTopologyKind_Triangles, R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB, r_handle_zero(), mat_4x4f32(1.f));
}

EV_EXPAND_RULE_INFO_FUNCTION_DEF(geo3d)
{
  EV_ExpandInfo info = {0};
  info.row_count = 16;
  info.single_item = 1;
  return info;
}

RD_VIEW_UI_FUNCTION_DEF(geo3d)
{
  Temp scratch = scratch_begin(0, 0);
  GEO_Scope *geo_scope = geo_scope_open();
  RD_Geo3DViewState *state = rd_view_state(RD_Geo3DViewState);
  
  //////////////////////////////
  //- rjf: unpack parameters
  //
  U64 count        = rd_value_from_eval_tag_key(eval, tag, str8_lit("count")).u64;
  U64 vtx_base_off = rd_value_from_eval_tag_key(eval, tag, str8_lit("vtx")).u64;
  U64 vtx_size     = rd_value_from_eval_tag_key(eval, tag, str8_lit("vtx_size")).u64;
  F32 yaw_target   = rd_view_cfg_value_from_string(str8_lit("yaw")).f32;
  F32 pitch_target = rd_view_cfg_value_from_string(str8_lit("pitch")).f32;
  F32 zoom_target  = rd_view_cfg_value_from_string(str8_lit("zoom")).f32;
  
  //////////////////////////////
  //- rjf: evaluate & unpack expression
  //
  U64 base_offset = rd_base_offset_from_eval(eval);
  Rng1U64 idxs_range = r1u64(base_offset, base_offset+count*sizeof(U32));
  Rng1U64 vtxs_range = r1u64(vtx_base_off, vtx_base_off+vtx_size);
  U128 idxs_key = rd_key_from_eval_space_range(eval.space, idxs_range, 0);
  U128 vtxs_key = rd_key_from_eval_space_range(eval.space, vtxs_range, 0);
  R_Handle idxs_buffer = geo_buffer_from_key(geo_scope, idxs_key);
  R_Handle vtxs_buffer = geo_buffer_from_key(geo_scope, vtxs_key);
  
  //////////////////////////////
  //- rjf: equip loading info
  //
  if(eval.msgs.max_kind == E_MsgKind_Null &&
     (r_handle_match(idxs_buffer, r_handle_zero()) ||
      r_handle_match(vtxs_buffer, r_handle_zero())))
  {
    rd_store_view_loading_info(1, 0, 0);
  }
  
  //////////////////////////////
  //- rjf: do first-time camera initialization, if needed
  //
  if(zoom_target == 0)
  {
    yaw_target   = -0.125f;
    pitch_target = -0.125f;
    zoom_target  = 3.5f;
  }
  
  //////////////////////////////
  //- rjf: animate camera
  //
  {
    F32 fast_rate = 1 - pow_f32(2, (-60.f * rd_state->frame_dt));
    F32 slow_rate = 1 - pow_f32(2, (-30.f * rd_state->frame_dt));
    state->zoom  += (zoom_target - state->zoom) * slow_rate;
    state->yaw   += (yaw_target - state->yaw) * fast_rate;
    state->pitch += (pitch_target - state->pitch) * fast_rate;
    if(abs_f32(state->zoom  - zoom_target)  > 0.001f ||
       abs_f32(state->yaw   - yaw_target)   > 0.001f ||
       abs_f32(state->pitch - pitch_target) > 0.001f)
    {
      rd_request_frame();
    }
  }
  
  //////////////////////////////
  //- rjf: build
  //
  if(count != 0 && !r_handle_match(idxs_buffer, r_handle_zero()) && !r_handle_match(vtxs_buffer, r_handle_zero()))
  {
    Vec2F32 dim = dim_2f32(rect);
    UI_Box *box = &ui_nil_box;
    UI_FixedSize(dim)
    {
      box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable|UI_BoxFlag_Scroll, "geo_box");
    }
    UI_Signal sig = ui_signal_from_box(box);
    if(ui_dragging(sig))
    {
      if(ui_pressed(sig))
      {
        rd_cmd(RD_CmdKind_FocusPanel);
        Vec2F32 data = v2f32(yaw_target, pitch_target);
        ui_store_drag_struct(&data);
      }
      Vec2F32 drag_delta      = ui_drag_delta();
      Vec2F32 drag_start_data = *ui_get_drag_struct(Vec2F32);
      yaw_target   = drag_start_data.x + drag_delta.x/dim.x;
      pitch_target = drag_start_data.y + drag_delta.y/dim.y;
    }
    zoom_target += sig.scroll.y;
    zoom_target = Clamp(0.1f, zoom_target, 100.f);
    pitch_target = Clamp(-0.49f, pitch_target, -0.01f);
    RD_Geo3DBoxDrawData *draw_data = push_array(ui_build_arena(), RD_Geo3DBoxDrawData, 1);
    draw_data->yaw   = state->yaw;
    draw_data->pitch = state->pitch;
    draw_data->zoom  = state->zoom;
    draw_data->vertex_buffer  = vtxs_buffer;
    draw_data->index_buffer   = idxs_buffer;
    ui_box_equip_custom_draw(box, rd_geo3d_box_draw, draw_data);
  }
  
  //////////////////////////////
  //- rjf: commit parameters
  //
  rd_store_view_param_f32(str8_lit("yaw"),   yaw_target);
  rd_store_view_param_f32(str8_lit("pitch"), pitch_target);
  rd_store_view_param_f32(str8_lit("zoom"),  zoom_target);
  
  geo_scope_close(geo_scope);
  scratch_end(scratch);
}
