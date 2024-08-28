// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Frontend Entry Points

internal void
update_and_render(OS_Handle repaint_window_handle, void *user_data)
{
  ProfTick(0);
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: begin logging
  //
  if(main_thread_log == 0)
  {
    main_thread_log = log_alloc();
    String8 user_program_data_path = os_get_process_info()->user_program_data_path;
    String8 user_data_folder = push_str8f(scratch.arena, "%S/raddbg/logs", user_program_data_path);
    main_thread_log_path = push_str8f(d_state->arena, "%S/ui_thread.raddbg_log", user_data_folder);
    os_make_directory(user_data_folder);
    os_write_data_to_file_path(main_thread_log_path, str8_zero());
  }
  log_select(main_thread_log);
  log_scope_begin();
  
  //////////////////////////////
  //- rjf: tick cache layers
  //
  txt_user_clock_tick();
  dasm_user_clock_tick();
  geo_user_clock_tick();
  tex_user_clock_tick();
  
  //////////////////////////////
  //- rjf: pick target hz
  //
  // TODO(rjf): maximize target, given all windows and their monitors
  F32 target_hz = os_get_gfx_info()->default_refresh_rate;
  if(frame_time_us_history_idx > 32)
  {
    // rjf: calculate average frame time out of the last N
    U64 num_frames_in_history = Min(ArrayCount(frame_time_us_history), frame_time_us_history_idx);
    U64 frame_time_history_sum_us = 0;
    for(U64 idx = 0; idx < num_frames_in_history; idx += 1)
    {
      frame_time_history_sum_us += frame_time_us_history[idx];
    }
    U64 frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    
    // rjf: pick among a number of sensible targets to snap to, given how well
    // we've been performing
    F32 possible_alternate_hz_targets[] = {target_hz, 60.f, 120.f, 144.f, 240.f};
    F32 best_target_hz = target_hz;
    S64 best_target_hz_frame_time_us_diff = max_S64;
    for(U64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
    {
      F32 candidate = possible_alternate_hz_targets[idx];
      if(candidate <= target_hz)
      {
        U64 candidate_frame_time_us = 1000000/(U64)candidate;
        S64 frame_time_us_diff = (S64)frame_time_history_avg_us - (S64)candidate_frame_time_us;
        if(abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff)
        {
          best_target_hz = candidate;
          best_target_hz_frame_time_us_diff = frame_time_us_diff;
        }
      }
    }
    target_hz = best_target_hz;
  }
  
  //////////////////////////////
  //- rjf: target Hz -> delta time
  //
  F32 dt = 1.f/target_hz;
  
  //////////////////////////////
  //- rjf: get events from the OS
  //
  OS_EventList events = {0};
  if(os_handle_match(repaint_window_handle, os_handle_zero()))
  {
    events = os_get_events(scratch.arena, df_gfx_state->num_frames_requested == 0);
  }
  
  //////////////////////////////
  //- rjf: begin measuring actual per-frame work
  //
  U64 begin_time_us = os_now_microseconds();
  
  //////////////////////////////
  //- rjf: bind change
  //
  if(!df_gfx_state->confirm_active && df_gfx_state->bind_change_active)
  {
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Esc))
    {
      df_gfx_request_frame();
      df_gfx_state->bind_change_active = 0;
    }
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Delete))
    {
      df_gfx_request_frame();
      df_unbind_spec(df_gfx_state->bind_change_cmd_spec, df_gfx_state->bind_change_binding);
      df_gfx_state->bind_change_active = 0;
      d_cmd(d_cfg_src_write_cmd_kind_table[D_CfgSrc_User]);
    }
    for(OS_Event *event = events.first, *next = 0; event != 0; event = next)
    {
      if(event->kind == OS_EventKind_Press &&
         event->key != OS_Key_Esc &&
         event->key != OS_Key_Return &&
         event->key != OS_Key_Backspace &&
         event->key != OS_Key_Delete &&
         event->key != OS_Key_LeftMouseButton &&
         event->key != OS_Key_RightMouseButton &&
         event->key != OS_Key_MiddleMouseButton &&
         event->key != OS_Key_Ctrl &&
         event->key != OS_Key_Alt &&
         event->key != OS_Key_Shift)
      {
        df_gfx_state->bind_change_active = 0;
        DF_Binding binding = zero_struct;
        {
          binding.key = event->key;
          binding.flags = event->flags;
        }
        df_unbind_spec(df_gfx_state->bind_change_cmd_spec, df_gfx_state->bind_change_binding);
        df_bind_spec(df_gfx_state->bind_change_cmd_spec, binding);
        U32 codepoint = os_codepoint_from_event_flags_and_key(event->flags, event->key);
        os_text(&events, os_handle_zero(), codepoint);
        os_eat_event(&events, event);
        d_cmd(d_cfg_src_write_cmd_kind_table[D_CfgSrc_User]);
        df_gfx_request_frame();
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: consume events
  //
  B32 queue_drag_drop = 0;
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
    {
      next = event->next;
      DF_Window *window = df_window_from_os_handle(event->window);
      D_CmdParams params = window ? df_cmd_params_from_window(window) : df_cmd_params_from_gfx();
      B32 take = 0;
      B32 skip = 0;
      
      //- rjf: try drag-drop
      if(df_drag_is_active() && event->kind == OS_EventKind_Release && event->key == OS_Key_LeftMouseButton)
      {
        skip = 1;
        queue_drag_drop = 1;
      }
      
      //- rjf: try window close
      if(!take && event->kind == OS_EventKind_WindowClose && window != 0)
      {
        take = 1;
        d_cmd(D_CmdKind_CloseWindow, .window = df_handle_from_window(window));
      }
      
      //- rjf: try menu bar operations
      {
        if(!take && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
        {
          take = 1;
          df_gfx_request_frame();
          window->menu_bar_focused_on_press = window->menu_bar_focused;
          window->menu_bar_key_held = 1;
          window->menu_bar_focus_press_started = 1;
        }
        if(!take && event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
        {
          take = 1;
          df_gfx_request_frame();
          window->menu_bar_key_held = 0;
        }
        if(window->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
        {
          take = 1;
          df_gfx_request_frame();
          window->menu_bar_focused = 0;
        }
        else if(window->menu_bar_focus_press_started && !window->menu_bar_focused && event->kind == OS_EventKind_Release && event->flags == 0 && event->key == OS_Key_Alt && event->is_repeat == 0)
        {
          take = 1;
          df_gfx_request_frame();
          window->menu_bar_focused = !window->menu_bar_focused_on_press;
          window->menu_bar_focus_press_started = 0;
        }
        else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && window->menu_bar_focused && !ui_any_ctx_menu_is_open())
        {
          take = 1;
          df_gfx_request_frame();
          window->menu_bar_focused = 0;
        }
      }
      
      //- rjf: try hotkey presses
      if(!take && event->kind == OS_EventKind_Press)
      {
        DF_Binding binding = {event->key, event->flags};
        D_CmdSpecList spec_candidates = df_cmd_spec_list_from_binding(scratch.arena, binding);
        if(spec_candidates.first != 0 && !d_cmd_spec_is_nil(spec_candidates.first->spec))
        {
          D_CmdSpec *run_spec = d_cmd_spec_from_kind(D_CmdKind_RunCommand);
          D_CmdSpec *spec = spec_candidates.first->spec;
          if(run_spec != spec)
          {
            params.cmd_spec = spec;
          }
          U32 hit_char = os_codepoint_from_event_flags_and_key(event->flags, event->key);
          take = 1;
          d_push_cmd(run_spec, &params);
          if(event->flags & OS_EventFlag_Alt)
          {
            window->menu_bar_focus_press_started = 0;
          }
        }
        else if(OS_Key_F1 <= event->key && event->key <= OS_Key_F19)
        {
          window->menu_bar_focus_press_started = 0;
        }
        df_gfx_request_frame();
      }
      
      //- rjf: try text events
      if(!take && event->kind == OS_EventKind_Text)
      {
        String32 insertion32 = str32(&event->character, 1);
        String8 insertion8 = str8_from_32(scratch.arena, insertion32);
        D_CmdSpec *spec = d_cmd_spec_from_kind(D_CmdKind_InsertText);
        params.string = insertion8;
        d_push_cmd(spec, &params);
        df_gfx_request_frame();
        take = 1;
        if(event->flags & OS_EventFlag_Alt)
        {
          window->menu_bar_focus_press_started = 0;
        }
      }
      
      //- rjf: do fall-through
      if(!take)
      {
        take = 1;
        params.os_event = event;
        d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_OSEvent), &params);
      }
      
      //- rjf: take
      if(take && !skip)
      {
        os_eat_event(&events, event);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather root-level commands
  //
  D_CmdList cmds = d_gather_root_cmds(scratch.arena);
  
  //////////////////////////////
  //- rjf: begin frame
  //
  {
    d_begin_frame(scratch.arena, &cmds, dt);
    df_gfx_begin_frame(scratch.arena, &cmds);
  }
  
  //////////////////////////////
  //- rjf: queue drop for drag/drop
  //
  if(queue_drag_drop)
  {
    df_queue_drag_drop();
  }
  
  //////////////////////////////
  //- rjf: auto-focus moused-over windows while dragging
  //
  if(df_drag_is_active())
  {
    B32 over_focused_window = 0;
    {
      for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
      {
        Vec2F32 mouse = os_mouse_from_window(window->os);
        Rng2F32 rect = os_client_rect_from_window(window->os);
        if(os_window_is_focused(window->os) && contains_2f32(rect, mouse))
        {
          over_focused_window = 1;
          break;
        }
      }
    }
    if(!over_focused_window)
    {
      for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
      {
        Vec2F32 mouse = os_mouse_from_window(window->os);
        Rng2F32 rect = os_client_rect_from_window(window->os);
        if(!os_window_is_focused(window->os) && contains_2f32(rect, mouse))
        {
          os_window_focus(window->os);
          break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: update & render
  //
  {
    dr_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      B32 window_is_focused = os_window_is_focused(w->os);
      if(window_is_focused)
      {
        last_focused_window = df_handle_from_window(w);
      }
      d_push_interact_regs();
      d_interact_regs()->window = df_handle_from_window(w);
      df_window_update_and_render(scratch.arena, w, &cmds);
      D_InteractRegs *window_regs = d_pop_interact_regs();
      if(df_window_from_handle(last_focused_window) == w)
      {
        MemoryCopyStruct(d_interact_regs(), window_regs);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: end frontend frame, send signals, etc.
  //
  {
    df_gfx_end_frame();
    d_end_frame();
  }
  
  //////////////////////////////
  //- rjf: submit rendering to all windows
  //
  {
    r_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      r_window_begin_frame(w->os, w->r);
      dr_submit_bucket(w->os, w->r, w->draw_bucket);
      r_window_end_frame(w->os, w->r);
    }
    r_end_frame();
  }
  
  //////////////////////////////
  //- rjf: show windows after first frame
  //
  if(os_handle_match(repaint_window_handle, os_handle_zero()))
  {
    D_HandleList windows_to_show = {0};
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      if(w->frames_alive == 1)
      {
        d_handle_list_push(scratch.arena, &windows_to_show, df_handle_from_window(w));
      }
    }
    for(D_HandleNode *n = windows_to_show.first; n != 0; n = n->next)
    {
      DF_Window *window = df_window_from_handle(n->handle);
      os_window_first_paint(window->os);
    }
  }
  
  //////////////////////////////
  //- rjf: determine frame time, record into history
  //
  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  frame_time_us_history[frame_time_us_history_idx%ArrayCount(frame_time_us_history)] = frame_time_us;
  frame_time_us_history_idx += 1;
  
  //////////////////////////////
  //- rjf: end logging
  //
  {
    LogScopeResult log = log_scope_end(scratch.arena);
    os_append_data_to_file_path(main_thread_log_path, log.strings[LogMsgKind_Info]);
    if(log.strings[LogMsgKind_UserError].size != 0)
    {
      d_error(log.strings[LogMsgKind_UserError]);
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal CTRL_WAKEUP_FUNCTION_DEF(wakeup_hook_ctrl)
{
  os_send_wakeup_event();
}
