// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: post-0.9.20 TODO notes
//
//- urgent fixes
// [ ] (use msvc assert as an example) show fastfail exception info (code, name, etc.) - comes from ExceptionInformation @fastfail
// [ ] stepping w/ spoofs & shadow stack enabled - writing spoof will send a stack buffer overrun event @shadow_stack_step
// [ ] hardware breakpoints regression (global eval in ctrl)
// [ ] native filesystem dialog, resizing raddbg window -> crash!
// [ ] stdout/stderr path target setting is now busted >:(
// [ ] target ui entry point should override built-in entry point
// [ ] list of all tabs in palette
// [ ] u64 + (ptr - ptr) seems to produce unexpected results - double check with C rules?
//
//- flow notes
// [ ] "skip breakpoint, run to source", when stopped at a non-source location
// [ ] adjust menu bar rendering when not focused
// [ ] treat int 0x29 similarly to int3
// [ ] auto_step, launching terminal, terminal steals focus from debugger...
//
//- memory view
// [ ] have smaller visible range than entire memory space, within some bounds (e.g. 64KB)
// [ ] dynamically expand memory space, based on scrolling
// [ ] fix clicking through occluded panels etc.
// [ ] disambiguate . character in ASCII columns
// [ ] fix type intepretations of cursor in bottom pane
//
//- watch improvements
// [ ] *ALL* expressions in watch windows need to be editable.
//
//- cfg improvements
// [ ] config hot-reloading, using cfg wins
// [ ] undo/redo, using cfg wins
// [ ] back/forward, using cfg wins
//  [ ]  mouse back button should make view to go back after I double clicked
//       on function to open it
// [ ] expand %environment_variables% in target environment strings - is there
//     a way we can defer to the underlying shell in a non-horrible way...?
//
//- stepping or breakpoint oddness/fixes
// [ ] halting during a spoof-ridden step leaves the spoofs in place!!!
//     (repro via LOTS of code on one line & halting)
// [ ] stepping-onto a line with a conditional breakpoint, which fails, causes a
// single step over the first instruction of that line, even if the thread
// would've stopped at the first instruction due to the step, were that bp not
// there.
// [ ] breakpoints in optimized code? maybe early-terminating bp resolution loop? @bpmiss
//      - actually this seems to be potentially because of incomplete src-line-map info...
// [ ] Mohit-reported breakpoint not hitting - may be similar thing to @bpmiss
//
//- ui improvements
// [ ] we probably want to disable pop/pull out for transient things, e.g. theme color cfgs
//     (actually, just kill the tabs on load if they refer to transient things)
// [ ] universal ctx menu address/watch options; e.g. watch -> memory; watch -> add watch
// [ ] rich hover coverage; bitmap <-> geo <-> memory <-> disassembly <-> text; etc.
// [ ] tooltip coverage pass (row commands, etc.)
// [ ] visualize all breakpoints everywhere - source view should show up in
//     disasm, disasm should show up in source view, function should show up in
//     both, etc.
//  [ ] ** Function breakpoints should show up in the source listing. Without
//      them being visible, it is confusing when you run and you stop there,
//      because you're like "wait why did it stop" and then you later remember
//      that's because there was a function breakpoint there.
// [ ] (reported by forrest) 'set-next-statement' -> prioritize current
//     module/symbol, in cases where one line maps to many voffs
// [ ] "Browse..." buttons should adopt a more relevant starting search path,
//     if possible
//  [ ] (since browse buttons are currently gone i should just add them backin
//       while respecting this old todo)
// [ ] For the Scheduler window, it would be nice if you could dim or
//     folderize threads that are not your threads - eg., if a thread doesn't
//     have any resolved stack pointers in your executable code, then you can
//     ignore it when you are focusing on your own code. I don't know what the
//     best way to detect this is, other than by walking the call stack... one
//     way might be to just have a way to separate threads you've named from
//     threads you haven't? Or, there could even be a debugger-specific API
//     that you use to tag them. Just some way that would make it easier to
//     focus on your own threads.
// [ ] use backslashes on windows by default, forward slashes elsewhere (?)
// [ ] For theme editing, when you hove the mouse over a theme color entry and
//     it highlights that entry, it might help to temporarily change that
//     color to white (or the inverse of the background color, or whatever) so
//     that the user can see what things on the screen use that theme color.
// [ ] The hex format for color values in the config file was a real
//     mindbender. It's prefixed with "0x", so I was assuming it was either
//     Windows Big Endian (0xAARRGGBB) or Mac Little Endian (0xAABBGGRR). To
//     my surprise, it was neither - it was actually web format (RRGGBBAA),
//     which I was not expecting because that is normally written with a
//     number sign (#AARRGGBB) not an 0x.
// [ ] It'd be nice to have a "goto byte" option for source views, for jumping
//     to error messages that are byte-based instead of line-based.
//  [ ] Clicking on either side of a scroll bar is idiosyncratic. Normally,
//      that is "page up" / "page down", but here it is "smooth scroll upward"
//      / "smooth scroll downward" for some reason?
//  [ ]  Alt+8 to switch to disassembly would be nice (regardless on which
//       panel was previous, don't want to use ctrl+, multiple times)
//       Alt+8 for disasm and Alt+6 for memory view are shortcuts I often use
//       in VS
//  [ ]  default font size is too small for me - not only source code, but
//       menus/tab/watch names (which don't resize). Maybe you could query
//       Windows for initial font size?
// [ ] globally disable/configure default view rule-like things (string
//     viz for u8s in particular)
// [ ] fancy string runs can include "weakness" information for text truncation
//     ... can prioritize certain parts of strings to be truncated before
//     others. would be good for e.g. the middle of a path
//
//- visualizer improvements
// [ ] disasm starting address - need to use debug info for more correct results...
// [ ] multidimensional `array`
// [ ] 2-vector, 3-vector, quaternion
// [ ] audio waveform views
//
//- eval improvements
// [ ] maybe add extra caching layer to process memory querying? we pay a pretty
//     heavy cost even to just read 8 bytes...
// [ ] serializing eval view maps (?)
// [ ] EVAL LOOKUP RULES -> currently going 0 -> rdis_count, but we need
//  to prioritize the primary rdi
// [ ] wide transforms
//  [ ] sum
//  [ ] plot
//  [ ] max view rule
//  [ ] min view rule
//  [ ] histogram view rule
//  [ ] diffs?
//  [ ] ** One very nice feature of RemedyBG that I use all the time is the
//      ability to put "$err, hr" into the watch window, which will just show
//      the value of GetLastError() as a string. This is super useful for
//      debugging, so you don't have to litter your own code with it.
//      (NOTE(rjf): NtQueryInformationThread)
// [ ] C++ virtual inheritance member visualization
// [ ] smart scopes - expression operators for "grab me the first type X"
// [ ] "pinning" watch expressions, to attach it to a particular scope/evaluation context
//
//- control improvements
// [ ] debug info overrides (both path-based AND module-based)
// [ ] symbol server
// [ ] can it ignore stepping into _RTC_CheckStackVars generated functions?
// [ ] jump table thunks, on code w/o /INCREMENTAL:NO
// [ ] investigate /DEBUG:FASTLINK - can we somehow alert that we do not
//     support it?
// [ ] just-in-time debugging
// [ ] step-out-of-loop
//
//- late-conversion performance improvements
// [ ] live++ investigations - ctrl+alt+f11 in UE?
//
//- short-to-medium term future features
// [ ] search-in-all-files
//  [ ] automatically snap to search matches when searching source files
// [ ] memory view
//  [ ] memory view mutation controls
//  [ ] memory view user-made annotations
//  [ ] memory view searching
// [ ] disasm view
//  [ ] visualize jump destinations in disasm
//
//- longer-term future features
// [ ] long-term future notes from martins
// [ ] core dump saving/loading
// [ ] parallel call stacks view
// [ ] parallel watch view
// [ ] mixed native/interpreted/jit debugging
//     - it seems python has a top-level linked list of interpreter states,
//       which should allow the debugger to map native callstacks to python
//       code
//
//- code cleanup
// [ ] eliminate explicit font parameters in the various ui paths (e.g.
//     code slice params)
// [ ] font cache eviction (both for font tags, closing fp handles, and
//     rasterizations)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "The RAD Debugger"
#define OS_FEATURE_GRAPHICAL 1

#define DMN_INIT_MANUAL 1
#define CTRL_INIT_MANUAL 1
#define OS_GFX_INIT_MANUAL 1
#define FP_INIT_MANUAL 1
#define R_INIT_MANUAL 1
#define FNT_INIT_MANUAL 1
#define D_INIT_MANUAL 1
#define RD_INIT_MANUAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "linker/hash_table.h"
#include "os/os_inc.h"
#include "artifact_cache/artifact_cache.h"
#include "rdi/rdi_local.h"
#include "rdi_make/rdi_make_local.h"
#include "mdesk/mdesk.h"
#include "config/config_inc.h"
#include "content/content.h"
#include "file_stream/file_stream.h"
#include "text/text.h"
#include "mutable_text/mutable_text.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "eh/eh_frame.h"
#include "dwarf/dwarf_inc.h"
#include "rdi_from_coff/rdi_from_coff.h"
#include "rdi_from_elf/rdi_from_elf.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "rdi_from_dwarf/rdi_from_dwarf.h"
#include "radbin/radbin.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "dbg_info/dbg_info.h"
#include "disasm/disasm.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "eval_visualization/eval_visualization_inc.h"
#include "ctrl/ctrl_inc.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "dbg_engine/dbg_engine_inc.h"
#include "raddbg/raddbg_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "linker/hash_table.c"
#include "os/os_inc.c"
#include "artifact_cache/artifact_cache.c"
#include "rdi/rdi_local.c"
#include "rdi_make/rdi_make_local.c"
#include "mdesk/mdesk.c"
#include "config/config_inc.c"
#include "content/content.c"
#include "file_stream/file_stream.c"
#include "text/text.c"
#include "mutable_text/mutable_text.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "eh/eh_frame.c"
#include "dwarf/dwarf_inc.c"
#include "rdi_from_coff/rdi_from_coff.c"
#include "rdi_from_elf/rdi_from_elf.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "rdi_from_dwarf/rdi_from_dwarf.c"
#include "radbin/radbin.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "dbg_info/dbg_info.c"
#include "disasm/disasm.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "eval_visualization/eval_visualization_inc.c"
#include "ctrl/ctrl_inc.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "dbg_engine/dbg_engine_inc.c"
#include "raddbg/raddbg_inc.c"

////////////////////////////////
//~ rjf: Top-Level Execution Types

typedef enum ExecMode
{
  ExecMode_Normal,
  ExecMode_IPCSender,
  ExecMode_BinaryUtility,
  ExecMode_Help,
}
ExecMode;

typedef struct IPCInfo IPCInfo;
struct IPCInfo
{
  U64 msg_size;
};

////////////////////////////////
//~ rjf: Globals

//- rjf: IPC resources
#define IPC_SHARED_MEMORY_BUFFER_SIZE MB(4)
StaticAssert(IPC_SHARED_MEMORY_BUFFER_SIZE > sizeof(IPCInfo), ipc_buffer_size_requirement);
global Semaphore ipc_sender2main_signal_semaphore = {0};
global Semaphore ipc_sender2main_lock_semaphore = {0};
global U8 *ipc_sender2main_shared_memory_base = 0;
global Semaphore ipc_main2sender_signal_semaphore = {0};
global Semaphore ipc_main2sender_lock_semaphore = {0};
global U8 *ipc_main2sender_shared_memory_base = 0;
global U8  ipc_s2m_ring_buffer[MB(4)] = {0};
global U64 ipc_s2m_ring_write_pos = 0;
global U64 ipc_s2m_ring_read_pos = 0;
global Mutex ipc_s2m_ring_mutex = {0};
global CondVar ipc_s2m_ring_cv = {0};

////////////////////////////////
//~ rjf: IPC Signaler Thread

internal void
ipc_signaler_thread__entry_point(void *p)
{
  ThreadNameF("rd_ipc_signaler_thread");
  for(;;)
  {
    if(os_semaphore_take(ipc_sender2main_signal_semaphore, max_U64))
    {
      if(os_semaphore_take(ipc_sender2main_lock_semaphore, max_U64))
      {
        IPCInfo *ipc_info = (IPCInfo *)ipc_sender2main_shared_memory_base;
        String8 msg = str8((U8 *)(ipc_info+1), ipc_info->msg_size);
        msg.size = Min(msg.size, IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo));
        MutexScope(ipc_s2m_ring_mutex) for(;;)
        {
          U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
          U64 available_size = (sizeof(ipc_s2m_ring_buffer) - unconsumed_size);
          if(available_size >= sizeof(U64)+sizeof(msg.size))
          {
            ipc_s2m_ring_write_pos += ring_write_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, &msg.size);
            ipc_s2m_ring_write_pos += ring_write(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, msg.str, msg.size);
            break;
          }
          cond_var_wait(ipc_s2m_ring_cv, ipc_s2m_ring_mutex, max_U64);
        }
        cond_var_broadcast(ipc_s2m_ring_cv);
        os_send_wakeup_event();
        ipc_info->msg_size = 0;
        os_semaphore_drop(ipc_sender2main_lock_semaphore);
      }
    }
  }
}

////////////////////////////////
//~ rjf: Ctrl -> Main Thread Wakeup Hook

internal CTRL_WAKEUP_FUNCTION_DEF(wakeup_hook_ctrl)
{
  os_send_wakeup_event();
}

////////////////////////////////
//~ rjf: Per-Frame Entry Point

internal B32
frame(void)
{
  rd_frame();
  return rd_state->quit;
}

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmd_line)
{
  Temp scratch = scratch_begin(0, 0);
  
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
    else if(cmd_line_has_flag(cmd_line, str8_lit("bin")))
    {
      exec_mode = ExecMode_BinaryUtility;
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
  
  //- rjf: dispatch to top-level codepath based on execution mode
  switch(exec_mode)
  {
    //- rjf: normal execution
    default:
    case ExecMode_Normal:
    {
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
      
      //- rjf: manual layer initialization
      {
        dmn_init();
        ctrl_init();
        os_gfx_init();
        fp_init();
        r_init(cmd_line);
        fnt_init();
        d_init();
        rd_init(cmd_line);
        ctrl_set_wakeup_hook(wakeup_hook_ctrl);
      }
      
      //- rjf: set up shared resources for ipc to this instance; launch IPC signaler thread
      {
        Temp scratch = scratch_begin(0, 0);
        U32 instance_pid = os_get_process_info()->pid;
        
        // rjf: set up cross-process sender -> main ring buffer
        String8 ipc_sender2main_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_shared_memory_%i_", instance_pid);
        String8 ipc_sender2main_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_signal_semaphore_%i_", instance_pid);
        String8 ipc_sender2main_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_lock_semaphore_%i_", instance_pid);
        OS_Handle ipc_sender2main_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_sender2main_shared_memory_name);
        ipc_sender2main_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_sender2main_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
        ipc_sender2main_signal_semaphore = semaphore_alloc(0, 1, ipc_sender2main_signal_semaphore_name);
        ipc_sender2main_lock_semaphore = semaphore_alloc(1, 1, ipc_sender2main_lock_semaphore_name);
        
        // rjf: set up cross-process main -> sender ring buffer
        String8 ipc_main2sender_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_shared_memory_%i_", instance_pid);
        String8 ipc_main2sender_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_signal_semaphore_%i_", instance_pid);
        String8 ipc_main2sender_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_lock_semaphore_%i_", instance_pid);
        OS_Handle ipc_main2sender_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_main2sender_shared_memory_name);
        ipc_main2sender_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_main2sender_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
        ipc_main2sender_signal_semaphore = semaphore_alloc(0, 1, ipc_main2sender_signal_semaphore_name);
        ipc_main2sender_lock_semaphore = semaphore_alloc(1, 1, ipc_main2sender_lock_semaphore_name);
        
        // rjf: set up ipc-receiver -> main thread ring buffer; launch signaler thread
        ipc_s2m_ring_mutex = mutex_alloc();
        ipc_s2m_ring_cv = cond_var_alloc();
        IPCInfo *ipc_info = (IPCInfo *)ipc_sender2main_shared_memory_base;
        if(ipc_sender2main_shared_memory_base != 0)
        {
          MemoryZeroStruct(ipc_info);
          thread_launch(ipc_signaler_thread__entry_point, 0);
        }
        
        scratch_end(scratch);
      }
      
      //- rjf: main application loop
      {
        for(B32 quit = 0; !quit;)
        {
          //- rjf: consume IPC messages, dispatch UI commands
          B32 ipc_command_frame = 0;
          {
            Temp scratch = scratch_begin(0, 0);
            B32 consumed = 0;
            String8 msg = {0};
            MutexScope(ipc_s2m_ring_mutex)
            {
              U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
              if(unconsumed_size >= sizeof(U64))
              {
                consumed = 1;
                ipc_command_frame = 1;
                ipc_s2m_ring_read_pos += ring_read_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, &msg.size);
                msg.size = Min(msg.size, unconsumed_size);
                msg.str = push_array(scratch.arena, U8, msg.size);
                ipc_s2m_ring_read_pos += ring_read(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, msg.str, msg.size);
              }
            }
            if(consumed)
            {
              cond_var_broadcast(ipc_s2m_ring_cv);
            }
            if(msg.size != 0)
            {
              log_infof("ipc_msg: \"%S\"", msg);
              RD_WindowState *dst_ws = rd_state->first_window_state;
              for(RD_WindowState *ws = dst_ws; ws != &rd_nil_window_state; ws = ws->order_next)
              {
                if(os_window_is_focused(ws->os))
                {
                  dst_ws = ws;
                  break;
                }
              }
              if(dst_ws != &rd_nil_window_state)
              {
                dst_ws->window_temporarily_focused_ipc = 1;
                RD_RegsScope()
                {
                  if(dst_ws->cfg_id != rd_regs()->window)
                  {
                    Temp scratch = scratch_begin(0, 0);
                    CFG_PanelTree panel_tree = cfg_panel_tree_from_cfg(scratch.arena, cfg_node_from_id(dst_ws->cfg_id));
                    rd_regs()->window = dst_ws->cfg_id;
                    rd_regs()->panel  = panel_tree.focused->cfg->id;
                    rd_regs()->tab    = panel_tree.focused->selected_tab->id;
                    rd_regs()->view   = panel_tree.focused->selected_tab->id;
                    scratch_end(scratch);
                  }
                  rd_cmd(RD_CmdKind_RunExternalDriverTextCommand, .string = msg);
                  rd_request_frame();
                }
              }
            }
            scratch_end(scratch);
          }
          
          //- rjf: update
          quit = update();
          
          //- rjf: auto run
          if(auto_run)
          {
            auto_run = 0;
            rd_cmd(RD_CmdKind_Run);
          }
          
          //- rjf: auto step
          if(auto_step)
          {
            auto_step = 0;
            rd_cmd(RD_CmdKind_StepInto);
          }
          
          //- rjf: jit attach
          if(jit_attach)
          {
            jit_attach = 0;
            rd_cmd(RD_CmdKind_Attach, .pid = jit_pid);
          }
          
          //- rjf: gather command outputs & write them
          if(ipc_command_frame)
          {
            if(ipc_main2sender_shared_memory_base != 0 &&
               os_semaphore_take(ipc_main2sender_lock_semaphore, os_now_microseconds()+5000000))
            {
              IPCInfo *ipc_info = (IPCInfo *)ipc_main2sender_shared_memory_base;
              U8 *buffer = (U8 *)(ipc_info+1);
              U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
              StringJoin join = {str8_lit(""), str8_lit("\0"), str8_lit("")};
              String8 msg = str8_list_join(scratch.arena, &rd_state->cmd_outputs, &join);
              ipc_info->msg_size = Min(buffer_max, msg.size);
              MemoryCopy(buffer, msg.str, ipc_info->msg_size);
              os_semaphore_drop(ipc_main2sender_signal_semaphore);
              os_semaphore_drop(ipc_main2sender_lock_semaphore);
            }
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
        U32 this_pid = os_get_process_info()->pid;
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
      String8 ipc_sender2main_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_shared_memory_%i_", dst_pid);
      String8 ipc_sender2main_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_signal_semaphore_%i_", dst_pid);
      String8 ipc_sender2main_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_sender2main_lock_semaphore_%i_", dst_pid);
      OS_Handle ipc_sender2main_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_sender2main_shared_memory_name);
      ipc_sender2main_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_sender2main_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      ipc_sender2main_signal_semaphore = os_semaphore_alloc(0, 1, ipc_sender2main_signal_semaphore_name);
      ipc_sender2main_lock_semaphore = os_semaphore_alloc(1, 1, ipc_sender2main_lock_semaphore_name);
      String8 ipc_main2sender_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_shared_memory_%i_", dst_pid);
      String8 ipc_main2sender_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_signal_semaphore_%i_", dst_pid);
      String8 ipc_main2sender_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_main2sender_lock_semaphore_%i_", dst_pid);
      OS_Handle ipc_main2sender_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_main2sender_shared_memory_name);
      ipc_main2sender_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_main2sender_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      ipc_main2sender_signal_semaphore = os_semaphore_alloc(0, 1, ipc_main2sender_signal_semaphore_name);
      ipc_main2sender_lock_semaphore = os_semaphore_alloc(1, 1, ipc_main2sender_lock_semaphore_name);
      
      //- rjf: got resources -> write message
      B32 wrote_message = 0;
      if(dst_pid != 0 &&
         ipc_sender2main_shared_memory_base != 0 &&
         os_semaphore_take(ipc_sender2main_lock_semaphore, max_U64))
      {
        wrote_message = 1;
        IPCInfo *ipc_info = (IPCInfo *)ipc_sender2main_shared_memory_base;
        U8 *buffer = (U8 *)(ipc_info+1);
        U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
        String8List parts = {0};
        {
          for EachIndex(idx, cmd_line->argc-1)
          {
            str8_list_push(scratch.arena, &parts, str8_cstring(cmd_line->argv[idx+1]));
          }
        }
        StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
        String8 msg = str8_list_join(scratch.arena, &parts, &join);
        ipc_info->msg_size = Min(buffer_max, msg.size);
        MemoryCopy(buffer, msg.str, ipc_info->msg_size);
        os_semaphore_drop(ipc_sender2main_signal_semaphore);
        os_semaphore_drop(ipc_sender2main_lock_semaphore);
      }
      
      //- rjf: wrote message -> wait for outputs, read outputs
      String8List outputs = {0};
      if(wrote_message &&
         ipc_main2sender_shared_memory_base != 0 &&
         os_semaphore_take(ipc_main2sender_signal_semaphore, os_now_microseconds()+10000000))
      {
        if(os_semaphore_take(ipc_main2sender_lock_semaphore, max_U64))
        {
          IPCInfo *ipc_info = (IPCInfo *)ipc_main2sender_shared_memory_base;
          String8 msg = str8((U8 *)(ipc_info+1), ipc_info->msg_size);
          msg.size = Min(msg.size, IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo));
          U8 split_char = 0;
          outputs = str8_split(scratch.arena, msg, &split_char, 1, 0);
          os_semaphore_drop(ipc_main2sender_lock_semaphore);
        }
      }
      
      //- rjf: write outputs to stdout
      for(String8Node *n = outputs.first; n != 0; n = n->next)
      {
        fwrite(n->string.str, 1, n->string.size, stdout);
      }
      fflush(stdout);
      
      scratch_end(scratch);
    }break;
    
    //- rjf: built-in binary utility mode
    case ExecMode_BinaryUtility:
    {
      rb_entry_point(cmd_line);
      di_signal_completion();
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
                                    "This will step into all active targets after the debugger initially starts.\n\n"
                                    "--auto_run\n"
                                    "This will run all active targets after the debugger initially starts.\n\n"
                                    "--quit_after_success (or -q)\n"
                                    "This will close the debugger automatically after all processes exit, if they all exited successfully (with code 0), and ran with no interruptions.\n\n"
                                    "--ipc <command>\n"
                                    "This will launch the debugger in the non-graphical IPC mode, which is used to communicate with another running instance of the debugger. The debugger instance will launch, send the specified command, then immediately terminate. This may be used by editors or other programs to control the debugger.\n\n"));
    }break;
  }
  
  scratch_end(scratch);
}
