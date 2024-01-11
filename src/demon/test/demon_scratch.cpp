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
  demon_init();
  
  String8Node node[2];
  
#define TARGET_EXE "C:\\devel\\projects\\debugger\\build\\mule_unwind_20210511_clang11_lldlink.exe"
  
  OS_LaunchOptions options = {0};
#if OS_WINDOWS
  str8_list_push(&options.cmd_line, &node[0], str8_lit(TARGET_EXE));
  options.path = str8_lit("C:\\devel\\projects\\debugger\\build\\");
#else
  str8_list_push(&options.cmd_line, &node[0], str8_lit("/home/allenw/projects_copy/debugger/build/mule_main"));
  options.path = str8_lit("/home/allenw/projects_copy/debugger/build/");
#endif
  U32 process_id = demon_launch_process(&options);
  if (process_id == 0){
    printf("Could not launch process\n");
    exit(1);
  }
  
#if OS_WINDOWS
  U64 bp_addr = 0x140001134;
#else
  U64 bp_addr = 0x400918;
#endif
  
  DEMON_Handle process = 0;
  DEMON_Handle thread = 0;
  DEMON_Handle module = 0;
  U64 module_base = 0;
  SYMS_Group *group = 0;
  
  B32 hit_bp = false;
  U64 counter = 0;
  
  for (;;){
    DEMON_RunCtrls run_controls = {0};
    DEMON_Trap trap_memory[2];
    
    DEMON_Trap *trap_ptr = trap_memory;
    if (process != 0 && !hit_bp){
      trap_ptr->process = process;
      trap_ptr->address = bp_addr;
      trap_ptr += 1;
    }
    run_controls.traps = trap_memory;
    run_controls.trap_count = (U64)(trap_ptr - trap_memory);
    
    Temp scratch = scratch_begin(0, 0);
    DEMON_EventList events = demon_run(scratch.arena, run_controls);
    
    for (DEMON_Event *event = events.first;
         event != 0;
         event = event->next){
      printf("STEP[%05llx] -- ", counter);
      counter += 1;
      
      switch (event->kind){
        case DEMON_EventKind_NotInitialized:
        {
          printf("Not Initialized\n");
          exit(1);
        }break;
        
        case DEMON_EventKind_NotAttached:
        {
          printf("Not Attached\n");
          exit(1);
        }break;
        
        case DEMON_EventKind_UnexpectedFailure:
        {
          printf("Unexpected Failure\n");
          exit(1);
        }break;
        
        case DEMON_EventKind_CreateProcess:
        {
          printf("Create Process\n");
          if (process == 0){
            process = event->process;
          }
        }break;
        
        case DEMON_EventKind_CreateThread:
        {
          printf("Create Thread\n");
          if (thread == 0){
            thread = event->thread;
          }
        }break;
        
        case DEMON_EventKind_LoadModule:
        {
          Temp temp = temp_begin(scratch.arena);
          String8 file_name = demon_full_path_from_module(scratch.arena, event->module);
          printf("Load Module: %.*s\n", str8_varg(file_name));
          if (module == 0 && str8_match(file_name, str8_lit(TARGET_EXE), 0)){
            module = event->module;
            module_base = event->address;
            
            // setup syms group
            group = syms_group_alloc();
            
            SYMS_FileLoadCtx ctx = {0};
            ctx.file_load_func = file_load_func_for_syms;
            
            SYMS_String8List file_names = {0};
            syms_string_list_push(group->arena, &file_names, syms_from_str8(file_name));
            
            SYMS_FileInfOptions opts = {0};
            
            SYMS_FileInfResult inf_result = syms_file_inf_infer_from_file_list(group->arena, ctx, file_names, &opts);
            syms_group_init(group, &inf_result.data_parsed);
          }
          temp_end(temp);
        }break;
        
        case DEMON_EventKind_ExitProcess:
        {
          printf("Exit Process\n");
          exit(0);
        }break;
        
        case DEMON_EventKind_ExitThread:
        {
          printf("Exit Thread\n");
        }break;
        
        case DEMON_EventKind_UnloadModule:
        {
          printf("Unload Module\n");
        }break;
        
        case DEMON_EventKind_Breakpoint:
        {
          Architecture arch = demon_arch_from_object(event->process);
          U64 ip = event->instruction_pointer;
          printf("Breakpoint: %llx\n", ip);
          
          hit_bp = true;
          
          //- unwind
          
          // setup bin
          SYMS_String8 bin_data = group->bin_data;
          SYMS_BinAccel *generic_bin = group->bin;
          SYMS_PeBinAccel *pe_bin = 0;
          if (generic_bin->format == SYMS_FileFormat_PE){
            pe_bin = (SYMS_PeBinAccel*)generic_bin;
          }
          
          if (pe_bin != 0){
            // read regs
            SYMS_RegX64 regs = {0};
            demon_read_x64_regs(event->thread, &regs);
            
            // read stack
            SYMS_U64 sp = regs.rsp.u64;
            SYMS_U64 sp_rounded_down = sp&~(KB(4) - 1);
            
            SYMS_String8 stack_memory = {0};
            stack_memory.size = KB(8);
            stack_memory.str = push_array_no_zero(scratch.arena, U8, stack_memory.size);
            
            SYMS_U64 stack_memory_addr = sp_rounded_down;
            stack_memory.size = demon_read_memory_amap(event->process, stack_memory.str,
                                                       stack_memory_addr, stack_memory.size);
            
            // unwind loop
            U64 counter = 1;
            for (;; counter += 1){
              printf("%02llu: ip=%llx; sp=%llx\n", counter, regs.rip.u64, regs.rsp.u64);
              SYMS_MemoryView memview = syms_memory_view_make(stack_memory, stack_memory_addr);
              SYMS_UnwindResult unwind_result = syms_unwind_pe_x64(bin_data, pe_bin, module_base, &memview, &regs);
              if (unwind_result.dead){
                break;
              }
            }
          }
        }break;
        
        case DEMON_EventKind_Trap:
        {
          Architecture arch = demon_arch_from_object(event->process);
          U64 ip = event->instruction_pointer;
          printf("Trap: %llx\n", ip);
        }break;
        
        case DEMON_EventKind_SingleStep:
        {
          printf("Single Step: %llx\n", event->instruction_pointer);
        }break;
        
        case DEMON_EventKind_Exception:
        {
          printf("Exception: %llx\n", event->instruction_pointer);
        }break;
        
        case DEMON_EventKind_Halt:
        {
          printf("Halt\n");
        }break;
        
        case DEMON_EventKind_Memory:
        {
          printf("Memory\n");
        }break;
        
        default:
        {
          printf("Unhandled Event\n");
          exit(1);
        }break;
      }
    }
    scratch_end(scratch);
  }
  
  printf("Done\n");
}

