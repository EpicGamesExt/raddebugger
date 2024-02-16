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
  
  //- rjf: tick cache layers
  txt_user_clock_tick();
  geo_user_clock_tick();
  tex_user_clock_tick();
  
  //- rjf: pick target hz
  // TODO(rjf): maximize target, given all windows and their monitors
  F32 target_hz = os_default_refresh_rate();
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
  
  //- rjf: target Hz -> delta time
  F32 dt = 1.f/target_hz;
  
  //- rjf: last frame before sleep -> disable txti change detection
  if(df_gfx_state->num_frames_requested == 0)
  {
    txti_set_external_change_detection_enabled(0);
  }
  
  //- rjf: get events from the OS
  OS_EventList events = {0};
  if(os_handle_match(repaint_window_handle, os_handle_zero()))
  {
    events = os_get_events(scratch.arena, df_gfx_state->num_frames_requested == 0);
  }
  
  //- rjf: enable txti change detection
  txti_set_external_change_detection_enabled(1);
  
  //- rjf: begin measuring actual per-frame work
  U64 begin_time_us = os_now_microseconds();
  
  //- rjf: bind change
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
      DF_CmdParams p = df_cmd_params_from_gfx();
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_write_cmd_kind_table[DF_CfgSrc_User]));
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
        DF_CmdParams p = df_cmd_params_from_gfx();
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_write_cmd_kind_table[DF_CfgSrc_User]));
        df_gfx_request_frame();
        break;
      }
    }
  }
  
  //- rjf: take hotkeys
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
    {
      next = event->next;
      DF_Window *window = df_window_from_os_handle(event->window);
      DF_CmdParams params = window ? df_cmd_params_from_window(window) : df_cmd_params_from_gfx();
      if(event->kind == OS_EventKind_Press)
      {
        DF_Binding binding = {event->key, event->flags};
        DF_CmdSpecList spec_candidates = df_cmd_spec_list_from_binding(scratch.arena, binding);
        if(spec_candidates.first != 0 && !df_cmd_spec_is_nil(spec_candidates.first->spec))
        {
          DF_CmdSpec *run_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand);
          DF_CmdSpec *spec = spec_candidates.first->spec;
          if(run_spec != spec)
          {
            params.cmd_spec = spec;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
          }
          U32 hit_char = os_codepoint_from_event_flags_and_key(event->flags, event->key);
          os_eat_event(&events, event);
          df_push_cmd__root(&params, run_spec);
          if(event->flags & OS_EventFlag_Alt)
          {
            window->menu_bar_focus_press_started = 0;
          }
        }
        df_gfx_request_frame();
      }
      else if(event->kind == OS_EventKind_Text)
      {
        String32 insertion32 = str32(&event->character, 1);
        String8 insertion8 = str8_from_32(scratch.arena, insertion32);
        DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_InsertText);
        params.string = insertion8;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, spec);
        df_gfx_request_frame();
        os_eat_event(&events, event);
        if(event->flags & OS_EventFlag_Alt)
        {
          window->menu_bar_focus_press_started = 0;
        }
      }
    }
  }
  
  //- rjf: menu bar focus
  {
    for(OS_Event *event = events.first, *next = 0; event != 0; event = next)
    {
      next = event->next;
      DF_Window *ws = df_window_from_os_handle(event->window);
      if(ws == 0)
      {
        continue;
      }
      B32 take = 0;
      if(event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
      {
        take = 1;
        df_gfx_request_frame();
        ws->menu_bar_focused_on_press = ws->menu_bar_focused;
        ws->menu_bar_key_held = 1;
        ws->menu_bar_focus_press_started = 1;
      }
      if(event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
      {
        take = 1;
        df_gfx_request_frame();
        ws->menu_bar_key_held = 0;
      }
      if(ws->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
      {
        take = 1;
        df_gfx_request_frame();
        ws->menu_bar_focused = 0;
      }
      else if(ws->menu_bar_focus_press_started && !ws->menu_bar_focused && event->kind == OS_EventKind_Release && event->flags == 0 && event->key == OS_Key_Alt && event->is_repeat == 0)
      {
        take = 1;
        df_gfx_request_frame();
        ws->menu_bar_focused = !ws->menu_bar_focused_on_press;
        ws->menu_bar_focus_press_started = 0;
      }
      else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && ws->menu_bar_focused && !ui_any_ctx_menu_is_open())
      {
        take = 1;
        df_gfx_request_frame();
        ws->menu_bar_focused = 0;
      }
      if(take)
      {
        os_eat_event(&events, event);
      }
    }
  }
  
  //- rjf: gather root-level commands
  DF_CmdList cmds = df_core_gather_root_cmds(scratch.arena);
  
  //- rjf: begin frame
  df_core_begin_frame(scratch.arena, &cmds, dt);
  df_gfx_begin_frame(scratch.arena, &cmds);
  
  //- rjf: queue drop for drag/drop
  if(df_drag_is_active())
  {
    for(OS_Event *event = events.first; event != 0; event = event->next)
    {
      if(event->kind == OS_EventKind_Release && event->key == OS_Key_LeftMouseButton)
      {
        df_queue_drag_drop();
        break;
      }
    }
  }
  
  //- rjf: auto-focus moused-over windows while dragging
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
  
  //- rjf: update & render
  {
    d_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      df_window_update_and_render(scratch.arena, &events, w, &cmds);
    }
  }
  
  //- rjf: end frontend frame, send signals, etc.
  df_gfx_end_frame();
  df_core_end_frame();
  
  //- rjf: submit rendering to all windows
  {
    r_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      r_window_begin_frame(w->os, w->r);
      d_submit_bucket(w->os, w->r, w->draw_bucket);
      r_window_end_frame(w->os, w->r);
    }
    r_end_frame();
  }
  
  //- rjf: take window closing events
  for(OS_Event *e = events.first, *next = 0; e; e = next)
  {
    next = e->next;
    if(e->kind == OS_EventKind_WindowClose)
    {
      for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
      {
        if(os_handle_match(w->os, e->window))
        {
          DF_CmdParams params = df_cmd_params_from_window(w);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
          break;
        }
      }
      os_eat_event(&events, e);
    }
  }
  
  //- rjf: determine frame time, record into history
  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  frame_time_us_history[frame_time_us_history_idx%ArrayCount(frame_time_us_history)] = frame_time_us;
  frame_time_us_history_idx += 1;
  
  scratch_end(scratch);
  ProfEnd();
}

internal CTRL_WAKEUP_FUNCTION_DEF(wakeup_hook)
{
  os_send_wakeup_event();
}

internal void
entry_point(int argc, char **argv)
{
  Temp scratch = scratch_begin(0, 0);
#if PROFILE_TELEMETRY
  local_persist U8 tm_data[MB(64)];
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(1024);
  tmInitialize(sizeof(tm_data), (char *)tm_data);
#endif
  ThreadName("[main]");
  
  //- rjf: initialize basic dependencies
  os_init(argc, argv);
  
  //- rjf: parse command line arguments
  CmdLine cmdln = cmd_line_from_string_list(scratch.arena, os_get_command_line_arguments());
  ExecMode exec_mode = ExecMode_Normal;
  String8 user_cfg_path = str8_lit("");
  String8 profile_cfg_path = str8_lit("");
  B32 capture = 0;
  B32 auto_run = 0;
  B32 auto_step = 0;
  B32 jit_attach = 0;
  U64 jit_pid = 0;
  U64 jit_code = 0;
  U64 jit_addr = 0;
  {
    if(cmd_line_has_flag(&cmdln, str8_lit("ipc")))
    {
      exec_mode = ExecMode_IPCSender;
    }
    else if(cmd_line_has_flag(&cmdln, str8_lit("convert")))
    {
      exec_mode = ExecMode_Converter;
    }
    else if(cmd_line_has_flag(&cmdln, str8_lit("?")) ||
            cmd_line_has_flag(&cmdln, str8_lit("help")))
    {
      exec_mode = ExecMode_Help;
    }
    user_cfg_path = cmd_line_string(&cmdln, str8_lit("user"));
    profile_cfg_path = cmd_line_string(&cmdln, str8_lit("profile"));
    capture = cmd_line_has_flag(&cmdln, str8_lit("capture"));
    auto_run = cmd_line_has_flag(&cmdln, str8_lit("auto_run"));
    auto_step = cmd_line_has_flag(&cmdln, str8_lit("auto_step"));
    String8 jit_pid_string = {0};
    String8 jit_code_string = {0};
    String8 jit_addr_string = {0};
    jit_pid_string = cmd_line_string(&cmdln, str8_lit("jit_pid"));
    jit_code_string = cmd_line_string(&cmdln, str8_lit("jit_code"));
    jit_addr_string = cmd_line_string(&cmdln, str8_lit("jit_addr"));
    try_u64_from_str8_c_rules(jit_pid_string, &jit_pid);
    try_u64_from_str8_c_rules(jit_code_string, &jit_code);
    try_u64_from_str8_c_rules(jit_addr_string, &jit_addr);
    jit_attach = (jit_addr != 0);
  }
  
  //- rjf: auto-start capture
  if(capture)
  {
    ProfBeginCapture("raddbg");
  }
  
  //- rjf: set default user/profile paths
  {
    String8 user_program_data_path = os_string_from_system_path(scratch.arena, OS_SystemPath_UserProgramData);
    String8 user_data_folder = push_str8f(scratch.arena, "%S/%S", user_program_data_path, str8_lit("raddbg"));
    os_make_directory(user_data_folder);
    if(user_cfg_path.size == 0)
    {
      user_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_user", user_data_folder);
    }
    if(profile_cfg_path.size == 0)
    {
      profile_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_profile", user_data_folder);
    }
  }
  
  //- rjf: dispatch to top-level codepath based on execution mode
  switch(exec_mode)
  {
    //- rjf: normal execution
    default:
    case ExecMode_Normal:
    {
      //- rjf: set up shared memory for ipc
      OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
      void *ipc_shared_memory_base = os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      OS_Handle ipc_semaphore = os_semaphore_alloc(1, 1, ipc_semaphore_name);
      IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
      ipc_info->msg_size = 0;
      
      //- rjf: initialize stuff we depend on
      {
        hs_init();
        fs_init();
        txt_init();
        dbgi_init();
        txti_init();
        demon_init();
        ctrl_init(wakeup_hook);
        dasm_init();
        os_graphical_init();
        fp_init();
        r_init(&cmdln);
        tex_init();
        geo_init();
        f_init();
        DF_StateDeltaHistory *hist = df_state_delta_history_alloc();
        df_core_init(user_cfg_path, profile_cfg_path, hist);
        df_gfx_init(update_and_render, hist);
        os_set_cursor(OS_Cursor_Pointer);
      }
      
      //- rjf: setup initial target from command line args
      {
        String8List args = cmdln.inputs;
        if(args.node_count > 0 && args.first->string.size != 0)
        {
          Temp scratch = scratch_begin(0, 0);
          DF_Entity *target = df_entity_alloc(0, df_entity_root(), DF_EntityKind_Target);
          df_entity_equip_b32(target, 1);
          df_entity_equip_cfg_src(target, DF_CfgSrc_CommandLine);
          String8List passthrough_args_list = {0};
          for(String8Node *n = args.first->next; n != 0; n = n->next)
          {
            str8_list_push(scratch.arena, &passthrough_args_list, n->string);
          }
          
          // rjf: equip exe
          if(args.first->string.size != 0)
          {
            DF_Entity *exe = df_entity_alloc(0, target, DF_EntityKind_Executable);
            df_entity_equip_name(0, exe, args.first->string);
          }
          
          // rjf: equip path
          String8 path_part_of_arg = str8_chop_last_slash(args.first->string);
          if(path_part_of_arg.size != 0)
          {
            String8 path = push_str8f(scratch.arena, "%S/", path_part_of_arg);
            DF_Entity *execution_path = df_entity_alloc(0, target, DF_EntityKind_ExecutionPath);
            df_entity_equip_name(0, execution_path, path);
          }
          
          // rjf: equip args
          StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
          String8 args_str = str8_list_join(scratch.arena, &passthrough_args_list, &join);
          if(args_str.size != 0)
          {
            DF_Entity *args_entity = df_entity_alloc(0, target, DF_EntityKind_Arguments);
            df_entity_equip_name(0, args_entity, args_str);
          }
          scratch_end(scratch);
        }
      }
      
      //- rjf: main application loop
      {
        for(;;)
        {
          //- rjf: get IPC messages & dispatch ui commands from them
          {
            if(os_semaphore_take(ipc_semaphore, max_U64))
            {
              if(ipc_info->msg_size != 0)
              {
                U8 *buffer = (U8 *)(ipc_info+1);
                U64 msg_size = ipc_info->msg_size;
                String8 cmd_string = str8(buffer, msg_size);
                ipc_info->msg_size = 0;
                DF_Window *dst_window = df_gfx_state->first_window;
                for(DF_Window *window = dst_window; window != 0; window = window->next)
                {
                  if(os_window_is_focused(window->os))
                  {
                    dst_window = window;
                    break;
                  }
                }
                if(dst_window != 0)
                {
                  Temp scratch = scratch_begin(0, 0);
                  String8 cmd_spec_string = df_cmd_name_part_from_string(cmd_string);
                  DF_CmdSpec *cmd_spec = df_cmd_spec_from_string(cmd_spec_string);
                  if(!df_cmd_spec_is_nil(cmd_spec))
                  {
                    DF_CmdParams params = df_cmd_params_from_gfx();
                    DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(dst_window);
                    String8 error = df_cmd_params_apply_spec_query(scratch.arena, &ctrl_ctx, &params, cmd_spec, df_cmd_arg_part_from_string(cmd_string));
                    if(error.size == 0)
                    {
                      df_push_cmd__root(&params, cmd_spec);
                    }
                    else
                    {
                      DF_CmdParams params = df_cmd_params_from_window(dst_window);
                      params.string = error;
                      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                    }
                  }
                  scratch_end(scratch);
                }
              }
              os_semaphore_drop(ipc_semaphore);
            }
          }
          
          //- rjf: update & render frame
          OS_Handle repaint_window = {0};
          update_and_render(repaint_window, 0);
          
          //- rjf: auto run
          if(auto_run)
          {
            auto_run = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
          }
          
          //- rjf: auto step
          if(auto_step)
          {
            auto_step = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_StepInto));
          }
          
          //- rjf: jit attach
          if(jit_attach)
          {
            jit_attach = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_ID);
            params.id = jit_pid;
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Attach));
          }
          
          //- rjf: quit if no windows are left
          if(df_gfx_state->first_window == 0)
          {
            break;
          }
        }
      }
      
    }break;
    
    //- rjf: inter-process communication message sender
    case ExecMode_IPCSender:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: grab ipc shared memory
      OS_Handle ipc_shared_memory = os_shared_memory_open(ipc_shared_memory_name);
      void *ipc_shared_memory_base = os_shared_memory_view_open(ipc_shared_memory, r1u64(0, MB(16)));
      if(ipc_shared_memory_base != 0)
      {
        OS_Handle ipc_semaphore = os_semaphore_open(ipc_semaphore_name);
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        if(os_semaphore_take(ipc_semaphore, os_now_microseconds() + Million(6)))
        {
          U8 *buffer = (U8 *)(ipc_info+1);
          U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
          StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
          String8 msg = str8_list_join(scratch.arena, &cmdln.inputs, &join);
          ipc_info->msg_size = Min(buffer_max, msg.size);
          MemoryCopy(buffer, msg.str, ipc_info->msg_size);
          os_semaphore_drop(ipc_semaphore);
        }
      }
      
      scratch_end(scratch);
    }break;
    
    //- rjf: built-in pdb/dwarf -> raddbg converter mode
    case ExecMode_Converter:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: parse arguments
      P2R_ConvertIn *convert_in = p2r_convert_in_from_cmd_line(scratch.arena, &cmdln);
      
      //- rjf: open output file
      String8 output_name = push_str8_copy(scratch.arena, convert_in->output_name);
      OS_Handle out_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_name);
      B32 out_file_is_good = !os_handle_match(out_file, os_handle_zero());
      
      //- rjf: convert
      P2R_ConvertOut *convert_out = 0;
      if(out_file_is_good)
      {
        convert_out = p2r_convert(scratch.arena, convert_in);
      }
      
      //- rjf: bake
      String8List bake_strings = {0};
      if(convert_out != 0 && convert_in->output_name.size > 0)
      {
        RDIM_BakeParams bake_params = {0};
        {
          bake_params.top_level_info   = convert_out->top_level_info;
          bake_params.binary_sections  = convert_out->binary_sections;
          bake_params.units            = convert_out->units;
          bake_params.types            = convert_out->types;
          bake_params.udts             = convert_out->udts;
          bake_params.global_variables = convert_out->global_variables;
          bake_params.thread_variables = convert_out->thread_variables;
          bake_params.procedures       = convert_out->procedures;
          bake_params.scopes           = convert_out->scopes;
        }
        bake_strings = rdim_bake(scratch.arena, &bake_params);
      }
      
      //- rjf: write
      if(out_file_is_good)
      {
        U64 off = 0;
        for(String8Node *n = bake_strings.first; n != 0; n = n->next)
        {
          os_file_write(out_file, r1u64(off, off+n->string.size), n->string.str);
          off += n->string.size;
        }
      }
      
      //- rjf: close output file
      os_file_close(out_file);
      
      scratch_end(scratch);
    }break;
    
    //- rjf: help message box
    case ExecMode_Help:
    {
      os_graphical_message(0,
                           str8_lit("The RAD Debugger - Help"),
                           str8_lit("The following options may be used when starting the RAD Debugger from the command line:\n\n"
                                    "--user:<path>\n"
                                    "Use to specify the location of a user file which should be used. User files are used to store settings for users, including window and panel setups, path mapping, and visual settings. If this file does not exist, it will be created as necessary. This file will be autosaved as user-related changes are made.\n\n"
                                    "--profile:<path>\n"
                                    "Use to specify the location of a profile file which should be used. Profile files are used to store settings for users and projects. If this file does not exist, it will be created as necessary. This file will be autosaved as profile-related changes are made.\n\n"
                                    "--auto_step\n"
                                    "This will step into all targets after the debugger initially starts.\n\n"
                                    "--auto_run\n"
                                    "This will run all targets after the debugger initially starts.\n\n"
                                    "--ipc <command>\n"
                                    "This will launch the debugger in the non-graphical IPC mode, which is used to communicate with another running instance of the debugger. The debugger instance will launch, send the specified command, then immediately terminate. This may be used by editors or other programs to control the debugger.\n\n"));
    }break;
  }
  
  scratch_end(scratch);
}
