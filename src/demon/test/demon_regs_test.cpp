// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "demon/demon_inc.h"
#include "syms_helpers/syms_internal_overrides.h"
#include "syms/syms_inc.h"
#include "syms_helpers/syms_helpers.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "demon/demon_inc.c"
#include "syms_helpers/syms_internal_overrides.c"
#include "syms/syms_inc.c"
#include "syms_helpers/syms_helpers.c"

internal SYMS_String8
file_load_func_for_syms(void *user, SYMS_Arena *arena, SYMS_String8 file_name){
  String8 data = os_read_file(arena, str8_from_syms(file_name));
  SYMS_String8 result = syms_from_str8(data);
  return(result);
}

int
main(int argument_count, char **arguments)
{
  os_init(argument_count, arguments);
  Temp scratch = scratch_begin(0, 0);
  
  // setup
  demon_init();
  
  // parse arguments
  String8 executable_file_name = {0};
  U64 bp_address = 0;
  
  {
    String8List command_line_arguments = os_get_command_line_arguments();
    CmdLine cmd_line = cmd_line_from_string_list(scratch.arena, command_line_arguments);
    if (cmd_line.inputs.first != 0){
      executable_file_name = cmd_line.inputs.first->string;
    }
    String8 bp_string = cmd_line_string(cmd_line, str8_lit("bp"));
    try_u64_from_str8_c_rules(bp_string, &bp_address);
  }
  
  // check parameters
  if (bp_address == 0 || executable_file_name.size == 0){
    printf("bad parameters\n");
    exit(0);
  }
  
  // demon launch
  OS_LaunchOptions launch_opts = {0};
  str8_list_push(scratch.arena, &launch_opts.cmd_line, executable_file_name);
  launch_opts.path = os_get_path(scratch.arena, OS_SystemPath_Current);
  U32 process_id = demon_launch_process(&launch_opts);
  if (process_id == 0){
    printf("could not launch: '%.*s'\n", str8_varg(executable_file_name));
    exit(0);
  }
  
  // demon loop
  {
    DEMON_Handle process = 0;
    DEMON_Handle thread = 0;
    
    B32 hit_bp = false;
    U64 single_step_counter = 0;
    
    U64 counter = 0;
    for (;;){
      Temp temp = temp_begin(scratch.arena);
      
      DEMON_RunCtrls run_controls = {0};
      DEMON_Trap traps[1];
      
      if (!hit_bp){
        if (process != 0){
          run_controls.trap_count = 1;
          run_controls.traps = traps;
          run_controls.traps[0].process = process;
          run_controls.traps[0].address = bp_address;
        }
      }
      else{
        run_controls.single_step_thread = thread;
      }
      
      DEMON_EventList events = demon_run(temp.arena, run_controls);
      
      for (DEMON_Event *event = events.first;
           event != 0;
           event = event->next, counter += 1){
        // update tracking state
        switch (event->kind){
          case DEMON_EventKind_CreateProcess:
          {
            process = event->process;
          }break;
          
          case DEMON_EventKind_ExitProcess:
          {
            if (event->process == process){
              process = 0;
            }
          }break;
          
          case DEMON_EventKind_CreateThread:
          {
            thread = event->thread;
          }break;
          
          case DEMON_EventKind_Breakpoint:
          {
            hit_bp = true;
          }break;
          
          case DEMON_EventKind_SingleStep:
          {
            single_step_counter += 1;
            
            SYMS_RegX64 regs1 = {0};
            demon_read_x64_regs(thread, &regs1);
            demon_write_x64_regs(thread, &regs1);
            SYMS_RegX64 regs2 = {0};
            demon_read_x64_regs(thread, &regs2);
            if (!MemoryMatchStruct(&regs1, &regs2)){
              printf("mismatch at single_step_counter=%llu\n", single_step_counter);
            }
            
            if (single_step_counter == 1000){
              goto end_loop;
            }
          }break;
          
          case DEMON_EventKind_NotAttached:
          {
            fprintf(stderr, "not attached - exiting\n");
            goto end_loop;
          }break;
          
          case DEMON_EventKind_NotInitialized:
          case DEMON_EventKind_UnexpectedFailure:
          {
            fprintf(stderr, "unexpected error - exiting\n");
            goto end_loop;
          }break;
        }
      }
      
      goto end_it;
      end_loop:
      temp_end(temp);
      goto loop_exit;
      end_it:;
    }
    
    loop_exit:;
  }
  
  printf("[done]\n");
  scratch_end(scratch);
}

