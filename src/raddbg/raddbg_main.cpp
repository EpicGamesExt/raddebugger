// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 10
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "The RAD Debugger"
#define OS_FEATURE_GRAPHICAL 1

#define R_INIT_MANUAL 1
#define TEX_INIT_MANUAL 1
#define GEO_INIT_MANUAL 1
#define F_INIT_MANUAL 1
#define DF_INIT_MANUAL 1
#define DF_GFX_INIT_MANUAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"
#include "lib_rdi_format/rdi_format_parse.h"
#include "lib_rdi_format/rdi_format_parse.c"
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "ico/ico.h"
#include "rdi_make_local/rdi_make_local.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "file_stream/file_stream.h"
#include "text_cache/text_cache.h"
#include "path/path.h"
#include "txti/txti.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "type_graph/type_graph.h"
#include "dbgi/dbgi.h"
#include "dasm_cache/dasm_cache.h"
#include "fuzzy_search/fuzzy_search.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "ctrl/ctrl_inc.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "texture_cache/texture_cache.h"
#include "geo_cache/geo_cache.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "df/df_inc.h"
#include "raddbg.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "ico/ico.c"
#include "rdi_make_local/rdi_make_local.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "file_stream/file_stream.c"
#include "text_cache/text_cache.c"
#include "path/path.c"
#include "txti/txti.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "type_graph/type_graph.c"
#include "dbgi/dbgi.c"
#include "dasm_cache/dasm_cache.c"
#include "fuzzy_search/fuzzy_search.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "ctrl/ctrl_inc.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "texture_cache/texture_cache.c"
#include "geo_cache/geo_cache.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "df/df_inc.c"
#include "raddbg.c"

////////////////////////////////
//~ rjf: IPC Signaler Thread

internal void
ipc_signaler_thread__entry_point(void *p)
{
  for(;;)
  {
    if(os_semaphore_take(ipc_signal_semaphore, max_U64))
    {
      if(os_semaphore_take(ipc_lock_semaphore, max_U64))
      {
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        String8 msg = str8((U8 *)(ipc_info+1), ipc_info->msg_size);
        msg.size = Min(msg.size, IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo));
        OS_MutexScope(ipc_s2m_ring_mutex) for(;;)
        {
          U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
          U64 available_size = (sizeof(ipc_s2m_ring_buffer) - unconsumed_size);
          if(available_size >= sizeof(U64)+sizeof(msg.size))
          {
            ipc_s2m_ring_write_pos += ring_write_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, &msg.size);
            ipc_s2m_ring_write_pos += ring_write(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, msg.str, msg.size);
            break;
          }
          os_condition_variable_wait(ipc_s2m_ring_cv, ipc_s2m_ring_mutex, max_U64);
        }
        os_condition_variable_broadcast(ipc_s2m_ring_cv);
        os_send_wakeup_event();
        ipc_info->msg_size = 0;
        os_semaphore_drop(ipc_lock_semaphore);
      }
    }
  }
}

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmd_line)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: windows -> turn off output handles, as we need to control those for target processes
#if OS_WINDOWS
  HANDLE output_handles[3] =
  {
    GetStdHandle(STD_INPUT_HANDLE),
    GetStdHandle(STD_OUTPUT_HANDLE),
    GetStdHandle(STD_ERROR_HANDLE),
  };
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    B32 duplicate = 0;
    for(U64 idx2 = 0; idx2 < idx; idx2 += 1)
    {
      if(output_handles[idx2] == output_handles[idx])
      {
        duplicate = 1;
        break;
      }
    }
    if(duplicate)
    {
      output_handles[idx] = 0;
    }
  }
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    if(output_handles[idx] != 0)
    {
      CloseHandle(output_handles[idx]);
    }
  }
  SetStdHandle(STD_INPUT_HANDLE, 0);
  SetStdHandle(STD_OUTPUT_HANDLE, 0);
  SetStdHandle(STD_ERROR_HANDLE, 0);
#endif
  
  //- rjf: unpack command line arguments
  ExecMode exec_mode = ExecMode_Normal;
  B32 auto_run = 0;
  B32 auto_step = 0;
  B32 jit_attach = 0;
  U64 jit_pid = 0;
  U64 jit_code = 0;
  U64 jit_addr = 0;
  {
    if(cmd_line_has_flag(cmd_line, str8_lit("ipc")))
    {
      exec_mode = ExecMode_IPCSender;
    }
    else if(cmd_line_has_flag(cmd_line, str8_lit("convert")))
    {
      exec_mode = ExecMode_Converter;
    }
    else if(cmd_line_has_flag(cmd_line, str8_lit("?")) ||
            cmd_line_has_flag(cmd_line, str8_lit("help")))
    {
      exec_mode = ExecMode_Help;
    }
    auto_run = cmd_line_has_flag(cmd_line, str8_lit("auto_run"));
    auto_step = cmd_line_has_flag(cmd_line, str8_lit("auto_step"));
    String8 jit_pid_string = cmd_line_string(cmd_line, str8_lit("jit_pid"));
    String8 jit_code_string = cmd_line_string(cmd_line, str8_lit("jit_code"));
    String8 jit_addr_string = cmd_line_string(cmd_line, str8_lit("jit_addr"));
    try_u64_from_str8_c_rules(jit_pid_string, &jit_pid);
    try_u64_from_str8_c_rules(jit_code_string, &jit_code);
    try_u64_from_str8_c_rules(jit_addr_string, &jit_addr);
    jit_attach = (jit_addr != 0);
  }
  
  //- rjf: set up layers
  ctrl_set_wakeup_hook(wakeup_hook_ctrl);
  
  //- rjf: dispatch to top-level codepath based on execution mode
  switch(exec_mode)
  {
    //- rjf: normal execution
    default:
    case ExecMode_Normal:
    {
      //- rjf: manual layer initialization
      {
        r_init(cmd_line);
        tex_init();
        geo_init();
        f_init();
        DF_StateDeltaHistory *hist = df_state_delta_history_alloc();
        df_core_init(cmd_line, hist);
        df_gfx_init(update_and_render, df_state_delta_history());
      }
      
      //- rjf: setup initial target from command line args
      {
        String8List args = cmd_line->inputs;
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
          
          // rjf: get current path
          String8 current_path = os_string_from_system_path(scratch.arena, OS_SystemPath_Current);
          
          // rjf: equip exe
          if(args.first->string.size != 0)
          {
            String8 exe_name = args.first->string;
            DF_Entity *exe = df_entity_alloc(0, target, DF_EntityKind_Executable);
            PathStyle style = path_style_from_str8(exe_name);
            if(style == PathStyle_Relative)
            {
              exe_name = push_str8f(scratch.arena, "%S/%S", current_path, exe_name);
              exe_name = path_normalized_from_string(scratch.arena, exe_name);
            }
            df_entity_equip_name(0, exe, exe_name);
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
      
      //- rjf: set up shared resources for ipc to this instance; launch IPC signaler thread
      {
        Temp scratch = scratch_begin(0, 0);
        U32 instance_pid = os_get_pid();
        String8 ipc_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_shared_memory_%i_", instance_pid);
        String8 ipc_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_signal_semaphore_%i_", instance_pid);
        String8 ipc_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_lock_semaphore_%i_", instance_pid);
        OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
        ipc_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
        ipc_signal_semaphore = os_semaphore_alloc(0, 1, ipc_signal_semaphore_name);
        ipc_lock_semaphore = os_semaphore_alloc(1, 1, ipc_lock_semaphore_name);
        ipc_s2m_ring_mutex = os_mutex_alloc();
        ipc_s2m_ring_cv = os_condition_variable_alloc();
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        MemoryZeroStruct(ipc_info);
        os_launch_thread(ipc_signaler_thread__entry_point, 0, 0);
        scratch_end(scratch);
      }
      
      //- rjf: main application loop
      {
        for(;;)
        {
          //- rjf: consume IPC messages, dispatch UI commands
          {
            Temp scratch = scratch_begin(0, 0);
            B32 consumed = 0;
            String8 msg = {0};
            OS_MutexScope(ipc_s2m_ring_mutex)
            {
              U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
              if(unconsumed_size >= sizeof(U64))
              {
                consumed = 1;
                ipc_s2m_ring_read_pos += ring_read_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, &msg.size);
                msg.size = Min(msg.size, unconsumed_size);
                msg.str = push_array(scratch.arena, U8, msg.size);
                ipc_s2m_ring_read_pos += ring_read(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, msg.str, msg.size);
              }
            }
            if(consumed)
            {
              os_condition_variable_broadcast(ipc_s2m_ring_cv);
            }
            if(msg.size != 0)
            {
              log_infof("IPC message received: \"%S\"", msg);
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
                dst_window->window_temporarily_focused_ipc = 1;
                String8 cmd_spec_string = df_cmd_name_part_from_string(msg);
                DF_CmdSpec *cmd_spec = df_cmd_spec_from_string(cmd_spec_string);
                if(!df_cmd_spec_is_nil(cmd_spec))
                {
                  DF_CmdParams params = df_cmd_params_from_window(dst_window);
                  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(dst_window);
                  String8 error = df_cmd_params_apply_spec_query(scratch.arena, &ctrl_ctx, &params, cmd_spec, df_cmd_arg_part_from_string(msg));
                  if(error.size == 0)
                  {
                    df_push_cmd__root(&params, cmd_spec);
                    df_gfx_request_frame();
                  }
                  else
                  {
                    DF_CmdParams params = df_cmd_params_from_window(dst_window);
                    params.string = error;
                    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                    df_gfx_request_frame();
                  }
                }
                else
                {
                  DF_CmdParams params = df_cmd_params_from_window(dst_window);
                  params.string = push_str8f(scratch.arena, "\"%S\" is not a command.", cmd_spec_string);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                  df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                  df_gfx_request_frame();
                }
              }
            }
            scratch_end(scratch);
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
      
      //- rjf: grab explicit PID argument
      U32 dst_pid = 0;
      if(cmd_line_has_argument(cmd_line, str8_lit("pid")))
      {
        String8 dst_pid_string = cmd_line_string(cmd_line, str8_lit("pid"));
        U64 dst_pid_u64 = 0;
        if(dst_pid_string.size != 0 &&
           try_u64_from_str8_c_rules(dst_pid_string, &dst_pid_u64))
        {
          dst_pid = (U32)dst_pid_u64;
        }
      }
      
      //- rjf: no explicit PID? -> find PID to send message to, by looking for other raddbg instances
      if(dst_pid == 0)
      {
        U32 this_pid = os_get_pid();
        DMN_ProcessIter it = {0};
        dmn_process_iter_begin(&it);
        for(DMN_ProcessInfo info = {0}; dmn_process_iter_next(scratch.arena, &it, &info);)
        {
          if(str8_match(str8_skip_last_slash(str8_chop_last_dot(cmd_line->exe_name)), str8_skip_last_slash(str8_chop_last_dot(info.name)), StringMatchFlag_CaseInsensitive) &&
             this_pid != info.pid)
          {
            dst_pid = info.pid;
            break;
          }
        }
        dmn_process_iter_end(&it);
      }
      
      //- rjf: grab destination instance's shared memory resources
      String8 ipc_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_shared_memory_%i_", dst_pid);
      String8 ipc_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_signal_semaphore_%i_", dst_pid);
      String8 ipc_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_lock_semaphore_%i_", dst_pid);
      OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
      ipc_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      ipc_signal_semaphore = os_semaphore_alloc(0, 1, ipc_signal_semaphore_name);
      ipc_lock_semaphore = os_semaphore_alloc(1, 1, ipc_lock_semaphore_name);
      
      //- rjf: got resources -> write message
      if(ipc_shared_memory_base != 0 &&
         os_semaphore_take(ipc_lock_semaphore, max_U64))
      {
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        U8 *buffer = (U8 *)(ipc_info+1);
        U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
        StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
        String8 msg = str8_list_join(scratch.arena, &cmd_line->inputs, &join);
        ipc_info->msg_size = Min(buffer_max, msg.size);
        MemoryCopy(buffer, msg.str, ipc_info->msg_size);
        os_semaphore_drop(ipc_signal_semaphore);
        os_semaphore_drop(ipc_lock_semaphore);
      }
      
      scratch_end(scratch);
    }break;
    
    //- rjf: built-in pdb/dwarf -> raddbg converter mode
    case ExecMode_Converter:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: parse arguments
      P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(scratch.arena, cmd_line);
      
      //- rjf: open output file
      String8 output_name = push_str8_copy(scratch.arena, user2convert->output_name);
      OS_Handle out_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_name);
      B32 out_file_is_good = !os_handle_match(out_file, os_handle_zero());
      
      //- rjf: convert
      P2R_Convert2Bake *convert2bake = 0;
      if(out_file_is_good)
      {
        convert2bake = p2r_convert(scratch.arena, user2convert);
      }
      
      //- rjf: bake
      P2R_Bake2Serialize *bake2srlz = 0;
      ProfScope("bake")
      {
        bake2srlz = p2r_bake(scratch.arena, convert2bake);
      }
      
      //- rjf: compress
      P2R_Bake2Serialize *bake2srlz_compressed = bake2srlz;
      if(cmd_line_has_flag(cmd_line, str8_lit("compress"))) ProfScope("compress")
      {
        bake2srlz_compressed = p2r_compress(scratch.arena, bake2srlz);
      }
      
      //- rjf: serialize
      String8List serialize_out = rdim_serialized_strings_from_params_bake_section_list(scratch.arena, &convert2bake->bake_params, &bake2srlz_compressed->sections);
      
      //- rjf: write
      if(out_file_is_good)
      {
        U64 off = 0;
        for(String8Node *n = serialize_out.first; n != 0; n = n->next)
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
                                    "--project:<path>\n"
                                    "Use to specify the location of a project file which should be used. Project files are used to store settings for users and projects. If this file does not exist, it will be created as necessary. This file will be autosaved as project-related changes are made.\n\n"
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
