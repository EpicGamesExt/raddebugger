// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0x4d9ae3ff

////////////////////////////////
//~ rjf: Generated Code

#include "dbg_engine/generated/dbg_engine.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

#if !defined(XXH_IMPLEMENTATION)
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif

internal U64
d_hash_from_seed_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal U64
d_hash_from_string(String8 string)
{
  return d_hash_from_seed_string(5381, string);
}

internal U64
d_hash_from_seed_string__case_insensitive(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + char_to_lower(string.str[i]);
  }
  return result;
}

internal U64
d_hash_from_string__case_insensitive(String8 string)
{
  return d_hash_from_seed_string__case_insensitive(5381, string);
}

////////////////////////////////
//~ rjf: Breakpoints

internal D_BreakpointArray
d_breakpoint_array_copy(Arena *arena, D_BreakpointArray *src)
{
  D_BreakpointArray dst = {0};
  dst.count = src->count;
  dst.v = push_array(arena, D_Breakpoint, dst.count);
  MemoryCopy(dst.v, src->v, sizeof(dst.v[0])*dst.count);
  for(U64 idx = 0; idx < dst.count; idx += 1)
  {
    dst.v[idx].file_path   = push_str8_copy(arena, dst.v[idx].file_path);
    dst.v[idx].vaddr_expr  = push_str8_copy(arena, dst.v[idx].vaddr_expr);
    dst.v[idx].condition   = push_str8_copy(arena, dst.v[idx].condition);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Path Map Application

internal String8List
d_possible_path_overrides_from_maps_path(Arena *arena, D_PathMapArray *path_maps, String8 file_path)
{
  // NOTE(rjf): This path, given some target file path, scans all file path map
  // overrides, and collects the set of file paths which could've redirected
  // to the target file path given the set of file path maps.
  //
  // For example, if I have a rule saying D:/devel/ maps to C:/devel/, and I
  // feed in C:/devel/foo/bar.txt, then this path will construct
  // D:/devel/foo/bar.txt, as a possible option.
  //
  // It will also preserve C:/devel/foo/bar.txt in the resultant list, so that
  // overrideless files still work through this path, and both redirected
  // files and non-redirected files can go through the same path.
  //
  String8List result = {0};
  str8_list_push(arena, &result, file_path);
  Temp scratch = scratch_begin(&arena, 1);
  PathStyle pth_style = PathStyle_Relative;
  String8List pth_parts = path_normalized_list_from_string(scratch.arena, file_path, &pth_style);
  {
    for(U64 idx = 0; idx < path_maps->count; idx += 1)
    {
      //- rjf: unpack link
      D_PathMap *map = &path_maps->v[idx];
      PathStyle src_style = PathStyle_Relative;
      PathStyle dst_style = PathStyle_Relative;
      String8List src_parts = path_normalized_list_from_string(scratch.arena, map->src, &src_style);
      String8List dst_parts = path_normalized_list_from_string(scratch.arena, map->dst, &dst_style);
      
      //- rjf: determine if this link can possibly redirect to the target file path
      B32 dst_redirects_to_pth = 0;
      String8Node *non_redirected_pth_first = 0;
      if(dst_style == pth_style && dst_parts.first != 0 && pth_parts.first != 0)
      {
        dst_redirects_to_pth = 1;
        String8Node *dst_n = dst_parts.first;
        String8Node *pth_n = pth_parts.first;
        for(;dst_n != 0 && pth_n != 0; dst_n = dst_n->next, pth_n = pth_n->next)
        {
          if(!str8_match(dst_n->string, pth_n->string, StringMatchFlag_CaseInsensitive))
          {
            dst_redirects_to_pth = 0;
            break;
          }
          non_redirected_pth_first = pth_n->next;
        }
      }
      
      //- rjf: if this link can redirect to this path via `src` -> `dst`, compute
      // possible full source path, by taking `src` and appending non-redirected
      // suffix (which did not show up in `dst`)
      if(dst_redirects_to_pth)
      {
        String8List candidate_parts = src_parts;
        for(String8Node *p = non_redirected_pth_first; p != 0; p = p->next)
        {
          str8_list_push(scratch.arena, &candidate_parts, p->string);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 candidate_path = str8_list_join(arena, &candidate_parts, &join);
        str8_list_push(arena, &result, candidate_path);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal D_LineList
d_line_list_copy(Arena *arena, D_LineList *list)
{
  D_LineList dst = {0};
  for(D_LineNode *src_n = list->first; src_n != 0; src_n = src_n->next)
  {
    D_LineNode *dst_n = push_array(arena, D_LineNode, 1);
    MemoryCopyStruct(dst_n, src_n);
    dst_n->v.file_path = push_str8_copy(arena, dst_n->v.file_path);
    dst_n->v.dbgi_key = di_key_copy(arena, &src_n->v.dbgi_key);
    SLLQueuePush(dst.first, dst.last, dst_n);
    dst.count += 1;
  }
  return dst;
}

////////////////////////////////
//~ rjf: Command Type Functions

//- rjf: command parameters

internal D_CmdParams
d_cmd_params_copy(Arena *arena, D_CmdParams *src)
{
  D_CmdParams dst = {0};
  MemoryCopyStruct(&dst, src);
  dst.file_path = push_str8_copy(arena, dst.file_path);
  dst.targets.v = push_array(arena, D_Target, dst.targets.count);
  MemoryCopy(dst.targets.v, src->targets.v, sizeof(D_Target)*dst.targets.count);
  for(U64 idx = 0; idx < dst.targets.count; idx += 1)
  {
    D_Target *target = &dst.targets.v[idx];
    target->exe = push_str8_copy(arena, target->exe);
    target->args = push_str8_copy(arena, target->args);
    target->working_directory = push_str8_copy(arena, target->working_directory);
    target->custom_entry_point_name = push_str8_copy(arena, target->custom_entry_point_name);
    target->env = str8_list_copy(arena, &target->env);
  }
  return dst;
}

//- rjf: command lists

internal void
d_cmd_list_push_new(Arena *arena, D_CmdList *cmds, D_CmdKind kind, D_CmdParams *params)
{
  D_CmdNode *n = push_array(arena, D_CmdNode, 1);
  n->cmd.kind = kind;
  n->cmd.params = d_cmd_params_copy(arena, params);
  DLLPushBack(cmds->first, cmds->last, n);
  cmds->count += 1;
}

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

// NOTE(rjf): Stepping Algorithm Overview (2024/01/17)
//
// The basic idea behind all stepping algorithms in the debugger are setting up
// a "trap net". A "trap net" is just a collection of high-level traps that are
// meant to "catch" a thread after letting it run. This trap net is submitted
// when the debugger frontend sends a "run" command (it is just empty if doing
// an actual 'run' or 'continue'). The debugger control thread then uses this
// trap net to program a state machine, to appropriately respond to a variety
// of debug events which it is passed from the OS.
//
// These are "high-level traps" because they can have specific behavioral info
// attached to them. These are encoded via the `CTRL_TrapFlags` type, which
// allow expression of the following behaviors:
//
//  - end-stepping: when this trap is hit, it will end the stepping operation,
//      and the target will not continue.
//  - ignore-stack-pointer-check: when a trap in the trap net is hit, it will
//      by-default be ignored if the thread's stack pointer has changed. this
//      flag disables that behavior, for when the stack pointer is expected to
//      change (e.g. step-out).
//  - single-step-after-hit: when a trap with this flag is hit, the debugger
//      will immediately single-step the thread which hit it.
//  - save-stack-pointer: when a trap with this flag is hit, it will rewrite
//      the stack pointer which is used to compare against, when deciding
//      whether or not to filter a trap (based on stack pointer changes).
//  - begin-spoof-mode: this enables "spoof mode". "spoof mode" is a special
//      mode that disables the trap net entirely, and lets the thread run
//      freely - but it catches the thread not with a trap, but a false return
//      address. the debugger will overwrite a specific return address on the
//      stack. this address will be overwritten with an address which does NOT
//      point to a valid page, such that when the thread returns out of a
//      particular call frame, the debugger will receive a debug event, at
//      which point it can move the thread back to the correct return address,
//      and resume with the trap net enabled. this is used in "step over"
//      operations, because it avoids target <-> debugger "roundtrips" (e.g.
//      target being stopped, debugger being called with debug events, then
//      target resumes when debugger's control thread is done running) for
//      recursions. (it doesn't make a difference with non-recursive calls,
//      but the debugger can't detect the difference).
//
// Each stepping command prepares its trap net differently.
//
// --- Instruction Step Into --------------------------------------------------
// In this case, no trap net is prepared, and only a low-level single-step is
// performed.
//
// --- Instruction Step Over --------------------------------------------------
// To build a trap net for an instruction-level step-over, the next instruction
// at the thread's current instruction pointer is decoded. If it is a call
// instruction, or if it is a repeating instruction, then a trap with the
// 'end-stepping' behavior is placed at the instruction immediately following
// the 'call' instruction.
//
// --- Line Step Into ---------------------------------------------------------
// For a source-line step-into, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> if can decode call destination address, place
//     "end-stepping | ignore-stack-pointer-check" trap at destination. if
//     can't, "end-stepping | single-step-after | ignore-stack-pointer-check"
//     trap at call.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping | ignore-
//     stack-pointer-check" trap at destination. if can't, "end-stepping |
//     single-step-after | ignore-stack-pointer-check" trap at jmp. if jump
//     stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
//
// --- Line Step Over ---------------------------------------------------------
// For a source-line step-over, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> place "single-step-after | begin-spoof-mode" trap at
//     call instruction.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping" trap at
//     destination. if can't, "end-stepping | single-step-after" trap at jmp.
//     if jump stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
// - for any instructions which may change the stack pointer, traps are placed
//     at them with the "save-stack-pointer | single-step-after" behaviors.

internal CTRL_TrapList
d_trap_net_from_thread__step_over_inst(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => unpacked info
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_rip_from_thread(&d_state->ctrl_entity_store->ctx, thread->handle);
  
  // rjf: ip => machine code
  String8 machine_code = {0};
  {
    Rng1U64 rng = r1u64(ip_vaddr, ip_vaddr+max_instruction_size_from_arch(arch));
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
  }
  
  // rjf: build traps if machine code was read successfully
  if(machine_code.size != 0)
  {
    // rjf: decode instruction
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, ip_vaddr, machine_code, DASM_Syntax_Intel);
    
    // rjf: call => run until call returns
    if(inst.flags & DASM_InstFlag_Call || inst.flags & DASM_InstFlag_Repeats)
    {
      CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, ip_vaddr+inst.size};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal CTRL_TrapList
d_trap_net_from_thread__step_over_line(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  log_infof("step_over_line:\n{\n");
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_rip_from_thread(&d_state->ctrl_entity_store->ctx, thread->handle);
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, ip_vaddr);
  DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
  log_infof("ip_vaddr: 0x%I64x\n", ip_vaddr);
  log_infof("dbgi_key: {%S, 0x%I64x}\n", dbgi_key.path, dbgi_key.min_timestamp);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = ctrl_voff_from_vaddr(module, ip_vaddr);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = ctrl_vaddr_range_from_voff_range(module, line_voff_rng);
      log_infof("line: {%S:%I64i}\n", lines.first->v.file_path, lines.first->v.pt.line);
    }
    log_infof("voff_range: {0x%I64x, 0x%I64x}\n", line_voff_rng.min, line_voff_rng.max);
    log_infof("vaddr_range: {0x%I64x, 0x%I64x}\n", line_vaddr_rng.min, line_vaddr_rng.max);
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = ctrl_voff_from_vaddr(module, line_vaddr_rng.max);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = ctrl_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  B32 good_machine_code = 0;
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, line_vaddr_rng, os_now_microseconds()+50000);
    machine_code = machine_code_slice.data;
    good_machine_code = (machine_code.size == dim_1u64(line_vaddr_rng) && !machine_code_slice.any_byte_bad);
    LogInfoNamedBlockF("machine_code_slice")
    {
      log_infof("stale: %i\n", machine_code_slice.stale);
      log_infof("any_byte_bad: %i\n", machine_code_slice.any_byte_bad);
      log_infof("any_byte_changed: %i\n", machine_code_slice.any_byte_changed);
      log_infof("bytes:\n[\n");
      for(U64 idx = 0; idx < machine_code_slice.data.size; idx += 1)
      {
        log_infof("0x%x,", machine_code_slice.data.str[idx]);
        if(idx%16 == 15 || idx+1 == machine_code_slice.data.size)
        {
          log_infof("\n");
        }
      }
      log_infof("]\n");
    }
  }
  
  // rjf: machine code => ctrl flow analysis
  DASM_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_machine_code)
  {
    ctrl_flow_info = dasm_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                              DASM_InstFlag_Call|
                                                              DASM_InstFlag_Branch|
                                                              DASM_InstFlag_UnconditionalJump|
                                                              DASM_InstFlag_ChangesStackPointer|
                                                              DASM_InstFlag_Return,
                                                              arch,
                                                              line_vaddr_rng.min,
                                                              machine_code);
    LogInfoNamedBlockF("ctrl_flow_info")
    {
      LogInfoNamedBlockF("exit_points") for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
      {
        log_infof("{vaddr:0x%I64x, jump_dest_vaddr:0x%I64x, inst_flags:%x}\n", n->v.vaddr, n->v.jump_dest_vaddr, n->v.inst_flags);
      }
    }
  }
  
  // rjf: push traps for all exit points
  if(good_machine_code) for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DASM_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
      
    }
    
    // rjf: call => place spoof at return spot in stack, single-step after hitting
    else if(point->inst_flags & DASM_InstFlag_Call)
    {
      flags |= (CTRL_TrapFlag_BeginSpoofMode|CTRL_TrapFlag_SingleStepAfterHit);
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info && good_machine_code)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  // rjf: log
  LogInfoNamedBlockF("traps") for(CTRL_TrapNode *n = result.first; n != 0; n = n->next)
  {
    log_infof("{flags:0x%x, vaddr:0x%I64x}\n", n->v.flags, n->v.vaddr);
  }
  
  scratch_end(scratch);
  log_infof("}\n\n");
  return result;
}

internal CTRL_TrapList
d_trap_net_from_thread__step_into_line(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_rip_from_thread(&d_state->ctrl_entity_store->ctx, thread->handle);
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, ip_vaddr);
  DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = ctrl_voff_from_vaddr(module, ip_vaddr);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = ctrl_vaddr_range_from_voff_range(module, line_voff_rng);
    }
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = ctrl_voff_from_vaddr(module, line_vaddr_rng.max);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = ctrl_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  B32 good_machine_code = 0;
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, line_vaddr_rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
    good_machine_code = (machine_code.size == dim_1u64(line_vaddr_rng) && !machine_code_slice.any_byte_bad);
  }
  
  // rjf: machine code => ctrl flow analysis
  DASM_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_machine_code)
  {
    ctrl_flow_info = dasm_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                              DASM_InstFlag_Call|
                                                              DASM_InstFlag_Branch|
                                                              DASM_InstFlag_UnconditionalJump|
                                                              DASM_InstFlag_ChangesStackPointer|
                                                              DASM_InstFlag_Return,
                                                              arch,
                                                              line_vaddr_rng.min,
                                                              machine_code);
  }
  
  // rjf: determine last 
  DASM_CtrlFlowPoint *last_call_point = 0;
  if(good_machine_code) for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    if(n->v.inst_flags & DASM_InstFlag_Call)
    {
      last_call_point = &n->v;
    }
  }
  
  // rjf: push traps for all exit points
  if(good_machine_code) for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DASM_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: if this is not the last call instruction in the control flow,
    // and if we have no line info for this address, then do not add.
    if(point != last_call_point &&
       point->inst_flags & DASM_InstFlag_Call &&
       point->jump_dest_vaddr != 0)
    {
      U64 jump_dest_vaddr = point->jump_dest_vaddr;
      CTRL_Entity *jump_dest_module = ctrl_module_from_process_vaddr(process, jump_dest_vaddr);
      U64 jump_dest_voff = ctrl_voff_from_vaddr(jump_dest_module, jump_dest_vaddr);
      DI_Key jump_dest_dbgi_key = ctrl_dbgi_key_from_module(jump_dest_module);
      D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &jump_dest_dbgi_key, jump_dest_voff);
      if(lines.count == 0)
      {
        add = 0;
      }
    }
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Call|
                            DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info && good_machine_code)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: symbol -> voff lookups

internal U64
d_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DI_Scope *scope = di_scope_open();
  U64 result = 0;
  {
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 1, 0);
    RDI_NameMapKind name_map_kinds[] =
    {
      RDI_NameMapKind_GlobalVariables,
      RDI_NameMapKind_Procedures,
    };
    if(rdi != &rdi_parsed_nil)
    {
      for(U64 name_map_kind_idx = 0;
          name_map_kind_idx < ArrayCount(name_map_kinds);
          name_map_kind_idx += 1)
      {
        RDI_NameMapKind name_map_kind = name_map_kinds[name_map_kind_idx];
        RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, name_map_kind);
        RDI_ParsedNameMap parsed_name_map = {0};
        rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, symbol_name.str, symbol_name.size);
        
        // rjf: node -> num
        U64 entity_num = 0;
        if(node != 0)
        {
          switch(node->match_count)
          {
            case 1:
            {
              entity_num = node->match_idx_or_idx_run_first + 1;
            }break;
            default:
            {
              U32 num = 0;
              U32 *run = rdi_matches_from_map_node(rdi, node, &num);
              if(num != 0)
              {
                entity_num = run[0]+1;
              }
            }break;
          }
        }
        
        // rjf: num -> voff
        U64 voff = 0;
        if(entity_num != 0) switch(name_map_kind)
        {
          default:{}break;
          case RDI_NameMapKind_GlobalVariables:
          {
            RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, entity_num-1);
            voff = global_var->voff;
          }break;
          case RDI_NameMapKind_Procedures:
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, entity_num-1);
            RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
            voff = *rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
          }break;
        }
        
        // rjf: nonzero voff -> break
        if(voff != 0)
        {
          result = voff;
          break;
        }
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return result;
}

//- rjf: voff -> line info

internal D_LineList
d_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 1, 0);
  D_LineList result = {0};
  {
    //- rjf: gather line tables
    typedef struct LineTableNode LineTableNode;
    struct LineTableNode
    {
      LineTableNode *next;
      RDI_ParsedLineTable parsed_line_table;
    };
    LineTableNode start_line_table = {0};
    RDI_Unit *unit = rdi_unit_from_voff(rdi, voff);
    RDI_LineTable *unit_line_table = rdi_line_table_from_unit(rdi, unit);
    rdi_parsed_from_line_table(rdi, unit_line_table, &start_line_table.parsed_line_table);
    LineTableNode *top_line_table = 0;
    RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
    {
      for(RDI_Scope *s = scope;
          s->inline_site_idx != 0;
          s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
      {
        RDI_InlineSite *inline_site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
        if(inline_site->line_table_idx != 0)
        {
          LineTableNode *n = push_array(scratch.arena, LineTableNode, 1);
          SLLStackPush(top_line_table, n);
          RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, inline_site->line_table_idx);
          rdi_parsed_from_line_table(rdi, line_table, &n->parsed_line_table);
        }
      }
    }
    SLLStackPush(top_line_table, &start_line_table);
    
    //- rjf: gather lines in each line table
    Rng1U64 shallowest_voff_range = {0};
    for(LineTableNode *line_table_n = top_line_table; line_table_n != 0; line_table_n = line_table_n->next)
    {
      RDI_ParsedLineTable parsed_line_table = line_table_n->parsed_line_table;
      U64 line_info_idx = rdi_line_info_idx_from_voff(&parsed_line_table, voff);
      if(line_info_idx < parsed_line_table.count)
      {
        RDI_Line *line = &parsed_line_table.lines[line_info_idx];
        RDI_Column *column = (line_info_idx < parsed_line_table.col_count) ? &parsed_line_table.cols[line_info_idx] : 0;
        RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
        String8List path_parts = {0};
        for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, file->file_path_node_idx);
            fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
            fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node))
        {
          String8 path_part = {0};
          path_part.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &path_part.size);
          str8_list_push_front(scratch.arena, &path_parts, path_part);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 file_normalized_full_path = str8_list_join(arena, &path_parts, &join);
        D_LineNode *n = push_array(arena, D_LineNode, 1);
        SLLQueuePush(result.first, result.last, n);
        result.count += 1;
        if(line->file_idx != 0 && file_normalized_full_path.size != 0)
        {
          n->v.file_path = file_normalized_full_path;
        }
        n->v.pt = txt_pt(line->line_num, column ? column->col_first : 1);
        n->v.voff_range = r1u64(parsed_line_table.voffs[line_info_idx], parsed_line_table.voffs[line_info_idx+1]);
        n->v.dbgi_key = *dbgi_key;
        if(line_table_n == top_line_table)
        {
          shallowest_voff_range = n->v.voff_range;
        }
      }
    }
    
    //- rjf: clamp all lines from all tables by shallowest (most unwound) range
    for(D_LineNode *n = result.first; n != 0; n = n->next)
    {
      n->v.voff_range = intersect_1u64(n->v.voff_range, shallowest_voff_range);
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return result;
}

//- rjf: file:line -> line info

// TODO(rjf): this depends on file path maps, needs to move

internal D_LineListArray
d_lines_array_from_dbgi_key_file_path_line_range(Arena *arena, DI_Key dbgi_key, String8 file_path, Rng1S64 line_num_range)
{
  D_LineListArray array = {0};
  {
    array.count = dim_1s64(line_num_range)+1;
    array.v = push_array(arena, D_LineList, array.count);
    di_key_list_push(arena, &array.dbgi_keys, &dbgi_key);
  }
  Temp scratch = scratch_begin(&arena, 1);
  U64 *lines_num_voffs = push_array(scratch.arena, U64, array.count);
  DI_Scope *scope = di_scope_open();
  String8List overrides = rd_possible_overrides_from_file_path(scratch.arena, file_path);
  for(String8Node *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    String8 file_path = override_n->string;
    String8 file_path_normalized = lower_from_str8(scratch.arena, path_normalized_from_string(scratch.arena, file_path));
    
    // rjf: binary -> rdi
    RDI_Parsed *rdi = di_rdi_from_key(scope, &dbgi_key, 1, 0);
    
    // rjf: file_path_normalized * rdi -> src_id
    B32 good_src_id = 0;
    U32 src_id = 0;
    if(rdi != &rdi_parsed_nil) ProfScope("file_path_normalized * rdi -> src_id")
    {
      RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
      RDI_ParsedNameMap map = {0};
      rdi_parsed_from_name_map(rdi, mapptr, &map);
      RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, file_path_normalized.str, file_path_normalized.size);
      if(node != 0)
      {
        U32 id_count = 0;
        U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
        if(id_count > 0)
        {
          good_src_id = 1;
          src_id = ids[0];
        }
      }
    }
    
    // rjf: good src-id -> look up line info for visible range
    if(good_src_id) ProfScope("good src-id -> look up line info for visible range")
    {
      RDI_SourceFile *src = rdi_element_from_name_idx(rdi, SourceFiles, src_id);
      RDI_SourceLineMap *src_line_map = rdi_element_from_name_idx(rdi, SourceLineMaps, src->source_line_map_idx);
      RDI_ParsedSourceLineMap line_map = {0};
      rdi_parsed_from_source_line_map(rdi, src_line_map, &line_map);
      U64 line_idx = 0;
      for(S64 line_num = line_num_range.min;
          line_num <= line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        D_LineList *list = &array.v[line_idx];
        U32 voff_count = 0;
        U64 *voffs = rdi_line_voffs_from_num(&line_map, u32_from_u64_saturate((U64)line_num), &voff_count);
        if(lines_num_voffs[line_idx] < 8) ProfScope("iterate voffs (%i)", voff_count) for(U64 idx = 0; idx < voff_count; idx += 1)
        {
          U64 base_voff = voffs[idx];
          U64 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, base_voff);
          RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
          RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
          RDI_ParsedLineTable unit_line_info = {0};
          rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
          U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, base_voff);
          if(unit_line_info.voffs != 0)
          {
            Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
            S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
            D_LineNode *n = push_array(arena, D_LineNode, 1);
            n->v.voff_range = range;
            n->v.pt.line = (S64)actual_line;
            n->v.pt.column = 1;
            n->v.dbgi_key = dbgi_key;
            SLLQueuePush(list->first, list->last, n);
            list->count += 1;
            lines_num_voffs[line_idx] += 1;
            if(lines_num_voffs[line_idx] >= 8)
            {
              break;
            }
          }
        }
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return array;
}

internal D_LineListArray
d_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range)
{
  D_LineListArray array = {0};
  {
    array.count = dim_1s64(line_num_range)+1;
    array.v = push_array(arena, D_LineList, array.count);
  }
  Temp scratch = scratch_begin(&arena, 1);
  U64 *lines_num_voffs = push_array(scratch.arena, U64, array.count);
  DI_Scope *scope = di_scope_open();
  DI_KeyList dbgi_keys = d_push_active_dbgi_key_list(scratch.arena);
  String8List overrides = rd_possible_overrides_from_file_path(scratch.arena, file_path);
  for(String8Node *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    String8 file_path = override_n->string;
    String8 file_path_normalized = lower_from_str8(scratch.arena, file_path);
    for(DI_KeyNode *dbgi_key_n = dbgi_keys.first;
        dbgi_key_n != 0;
        dbgi_key_n = dbgi_key_n->next)
    {
      // rjf: binary -> rdi
      DI_Key key = dbgi_key_n->v;
      RDI_Parsed *rdi = di_rdi_from_key(scope, &key, 1, 0);
      
      // rjf: file_path_normalized * rdi -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
      if(rdi != &rdi_parsed_nil) ProfScope("file_path_normalized * rdi -> src_id")
      {
        RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
        RDI_ParsedNameMap map = {0};
        rdi_parsed_from_name_map(rdi, mapptr, &map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, file_path_normalized.str, file_path_normalized.size);
        if(node != 0)
        {
          U32 id_count = 0;
          U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
          if(id_count > 0)
          {
            good_src_id = 1;
            src_id = ids[0];
          }
        }
      }
      
      // rjf: good src-id -> look up line info for visible range
      if(good_src_id) ProfScope("good src-id -> look up line info for visible range")
      {
        RDI_SourceFile *src = rdi_element_from_name_idx(rdi, SourceFiles, src_id);
        RDI_SourceLineMap *src_line_map = rdi_element_from_name_idx(rdi, SourceLineMaps, src->source_line_map_idx);
        RDI_ParsedSourceLineMap line_map = {0};
        rdi_parsed_from_source_line_map(rdi, src_line_map, &line_map);
        U64 line_idx = 0;
        for(S64 line_num = line_num_range.min;
            line_num <= line_num_range.max;
            line_num += 1, line_idx += 1)
        {
          D_LineList *list = &array.v[line_idx];
          U32 voff_count = 0;
          U64 *voffs = rdi_line_voffs_from_num(&line_map, u32_from_u64_saturate((U64)line_num), &voff_count);
          if(lines_num_voffs[line_idx] < 8) ProfScope("iterate voffs (%i)", voff_count) for(U64 idx = 0; idx < voff_count; idx += 1)
          {
            U64 base_voff = voffs[idx];
            U64 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, base_voff);
            RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
            RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
            RDI_ParsedLineTable unit_line_info = {0};
            rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
            U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, base_voff);
            if(unit_line_info.voffs != 0)
            {
              Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
              S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
              D_LineNode *n = push_array(arena, D_LineNode, 1);
              n->v.voff_range = range;
              n->v.pt.line = (S64)actual_line;
              n->v.pt.column = 1;
              n->v.dbgi_key = key;
              SLLQueuePush(list->first, list->last, n);
              list->count += 1;
              lines_num_voffs[line_idx] += 1;
              if(lines_num_voffs[line_idx] >= 8)
              {
                break;
              }
            }
          }
        }
      }
      
      // rjf: good src id -> push to relevant dbgi keys
      if(good_src_id)
      {
        di_key_list_push(arena, &array.dbgi_keys, &key);
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return array;
}

internal D_LineList
d_lines_from_dbgi_key_file_path_line_num(Arena *arena, DI_Key dbgi_key, String8 file_path, S64 line_num)
{
  D_LineListArray array = d_lines_array_from_dbgi_key_file_path_line_range(arena, dbgi_key, file_path, r1s64(line_num, line_num+1));
  D_LineList list = {0};
  if(array.count != 0)
  {
    list = array.v[0];
  }
  return list;
}

internal D_LineList
d_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num)
{
  D_LineListArray array = d_lines_array_from_file_path_line_range(arena, file_path, r1s64(line_num, line_num+1));
  D_LineList list = {0};
  if(array.count != 0)
  {
    list = array.v[0];
  }
  return list;
}

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal U64
d_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  ProfBeginFunction();
  U64 base_vaddr = 0;
  Temp scratch = scratch_begin(0, 0);
  if(!d_ctrl_targets_running())
  {
    //- rjf: unpack module info
    CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
    Rng1U64 tls_vaddr_range = ctrl_tls_vaddr_range_from_module(module->handle);
    U64 addr_size = bit_size_from_arch(process->arch)/8;
    
    //- rjf: read module's TLS index
    U64 tls_index = 0;
    if(addr_size != 0)
    {
      CTRL_ProcessMemorySlice tls_index_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, tls_vaddr_range, 0);
      if(tls_index_slice.data.size >= addr_size)
      {
        tls_index = *(U64 *)tls_index_slice.data.str;
      }
    }
    
    //- rjf: PE path
    if(addr_size != 0)
    {
      U64 thread_info_addr = root_vaddr;
      U64 tls_addr_off = tls_index*addr_size;
      U64 tls_addr_array = 0;
      CTRL_ProcessMemorySlice tls_addr_array_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, r1u64(thread_info_addr, thread_info_addr+addr_size), 0);
      String8 tls_addr_array_data = tls_addr_array_slice.data;
      if(tls_addr_array_data.size >= 8)
      {
        MemoryCopy(&tls_addr_array, tls_addr_array_data.str, sizeof(U64));
      }
      CTRL_ProcessMemorySlice result_slice = ctrl_process_memory_slice_from_vaddr_range(scratch.arena, process->handle, r1u64(tls_addr_array + tls_addr_off, tls_addr_array + tls_addr_off + addr_size), 0);
      String8 result_data = result_slice.data;
      if(result_data.size >= 8)
      {
        MemoryCopy(&base_vaddr, result_data.str, sizeof(U64));
      }
    }
    
    //- rjf: non-PE path (not implemented)
#if 0
    if(!bin_is_pe)
    {
      // TODO(rjf): not supported. old code from the prototype that Nick had sketched out:
      // TODO(nick): This code works only if the linked c runtime library is glibc.
      // Implement CRT detection here.
      
      U64 dtv_addr = UINT64_MAX;
      demon_read_memory(process->demon_handle, &dtv_addr, thread_info_addr, addr_size);
      
      /*
        union delta_thread_vector
        {
          size_t counter;
          struct
          {
            void *value;
            void *to_free;
          } pointer;
        };
      */
      
      U64 dtv_size = 16;
      U64 dtv_count = 0;
      demon_read_memory(process->demon_handle, &dtv_count, dtv_addr - dtv_size, addr_size);
      
      if (tls_index > 0 && tls_index < dtv_count)
      {
        demon_read_memory(process->demon_handle, &result, dtv_addr + dtv_size*tls_index, addr_size);
      }
    }
#endif
  }
  scratch_end(scratch);
  ProfEnd();
  return base_vaddr;
}

////////////////////////////////
//~ rjf: Target Controls

//- rjf: stopped info from the control thread

internal CTRL_Event
d_ctrl_last_stop_event(void)
{
  return d_state->ctrl_last_stop_event;
}

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data

internal U64
d_frame_index(void)
{
  return d_state->frame_index;
}

//- rjf: control state

internal D_RunKind
d_ctrl_last_run_kind(void)
{
  return d_state->ctrl_last_run_kind;
}

internal U64
d_ctrl_last_run_frame_idx(void)
{
  return d_state->ctrl_last_run_frame_idx;
}

internal B32
d_ctrl_targets_running(void)
{
  return d_state->ctrl_is_running;
}

//- rjf: active entity based queries

internal DI_KeyList
d_push_active_dbgi_key_list(Arena *arena)
{
  DI_KeyList dbgis = {0};
  CTRL_EntityArray modules = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Module);
  for EachIndex(idx, modules.count)
  {
    CTRL_Entity *module = modules.v[idx];
    DI_Key key = ctrl_dbgi_key_from_module(module);
    di_key_list_push(arena, &dbgis, &key);
  }
  return dbgis;
}

//- rjf: per-run caches

internal U64
d_query_cached_rip_from_thread(CTRL_Entity *thread)
{
  U64 result = d_query_cached_rip_from_thread_unwind(thread, 0);
  return result;
}

internal U64
d_query_cached_rip_from_thread_unwind(CTRL_Entity *thread, U64 unwind_count)
{
  U64 result = 0;
  if(unwind_count == 0)
  {
    result = ctrl_rip_from_thread(&d_state->ctrl_entity_store->ctx, thread->handle);
  }
  else
  {
    CTRL_Scope *ctrl_scope = ctrl_scope_open();
    CTRL_CallStack callstack = ctrl_call_stack_from_thread(ctrl_scope, &d_state->ctrl_entity_store->ctx, thread, 1, 0);
    if(callstack.concrete_frames_count != 0)
    {
      result = regs_rip_from_arch_block(thread->arch, callstack.concrete_frames[unwind_count%callstack.concrete_frames_count]->regs);
    }
    ctrl_scope_close(ctrl_scope);
  }
  return result;
}

internal U64
d_query_cached_tls_base_vaddr_from_process_root_rip(CTRL_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  U64 result = 0;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->tls_base_caches); cache_idx += 1)
  {
    D_RunTLSBaseCache *cache = &d_state->tls_base_caches[(d_state->tls_base_cache_gen+cache_idx)%ArrayCount(d_state->tls_base_caches)];
    if(cache_idx == 0 && cache->slots_count == 0)
    {
      cache->slots_count = 256;
      cache->slots = push_array(cache->arena, D_RunTLSBaseCacheSlot, cache->slots_count);
    }
    else if(cache->slots_count == 0)
    {
      break;
    }
    CTRL_Handle handle = process->handle;
    U64 hash = d_hash_from_seed_string(d_hash_from_string(str8_struct(&handle)), str8_struct(&rip_vaddr));
    U64 slot_idx = hash%cache->slots_count;
    D_RunTLSBaseCacheSlot *slot = &cache->slots[slot_idx];
    D_RunTLSBaseCacheNode *node = 0;
    for(D_RunTLSBaseCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(ctrl_handle_match(n->process, handle) && n->root_vaddr == root_vaddr && n->rip_vaddr == rip_vaddr)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      U64 tls_base_vaddr = d_tls_base_vaddr_from_process_root_rip(process, root_vaddr, rip_vaddr);
      if(tls_base_vaddr != 0)
      {
        node = push_array(cache->arena, D_RunTLSBaseCacheNode, 1);
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
        node->process = handle;
        node->root_vaddr = root_vaddr;
        node->rip_vaddr = rip_vaddr;
        node->tls_base_vaddr = tls_base_vaddr;
      }
    }
    if(node != 0 && node->tls_base_vaddr != 0)
    {
      result = node->tls_base_vaddr;
      break;
    }
  }
  return result;
}

internal E_String2NumMap *
d_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->locals_caches); cache_idx += 1)
  {
    D_RunLocalsCache *cache = &d_state->locals_caches[(d_state->locals_cache_gen+cache_idx)%ArrayCount(d_state->locals_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, D_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    D_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    D_RunLocalsCacheNode *node = 0;
    for(D_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 1, 0);
      E_String2NumMap *map = e_push_locals_map_from_rdi_voff(cache->arena, rdi, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, D_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

internal E_String2NumMap *
d_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->member_caches); cache_idx += 1)
  {
    D_RunLocalsCache *cache = &d_state->member_caches[(d_state->member_cache_gen+cache_idx)%ArrayCount(d_state->member_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, D_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    D_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    D_RunLocalsCacheNode *node = 0;
    for(D_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 1, 0);
      E_String2NumMap *map = e_push_member_map_from_rdi_voff(cache->arena, rdi, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, D_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

//- rjf: top-level command dispatch

internal void
d_push_cmd(D_CmdKind kind, D_CmdParams *params)
{
  d_cmd_list_push_new(d_state->cmds_arena, &d_state->cmds, kind, params);
}

//- rjf: command iteration

internal B32
d_next_cmd(D_Cmd **cmd)
{
  D_CmdNode *start_node = d_state->cmds.first;
  if(cmd[0] != 0)
  {
    start_node = CastFromMember(D_CmdNode, cmd, cmd[0]);
    start_node = start_node->next;
  }
  cmd[0] = 0;
  if(start_node != 0)
  {
    cmd[0] = &start_node->cmd;
  }
  return !!cmd[0];
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

#if !defined(XXH_IMPLEMENTATION)
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif

internal void
d_init(void)
{
  Arena *arena = arena_alloc();
  d_state = push_array(arena, D_State, 1);
  d_state->arena = arena;
  d_state->cmds_arena = arena_alloc();
  d_state->output_log_key = hs_key_make(hs_root_alloc(), hs_id_make(0, 0));
  hs_submit_data(d_state->output_log_key, 0, str8_zero());
  d_state->ctrl_entity_store = ctrl_entity_ctx_rw_store_alloc();
  d_state->ctrl_stop_arena = arena_alloc();
  d_state->ctrl_msg_arena = arena_alloc();
  
  // rjf: set up caches
  for(U64 idx = 0; idx < ArrayCount(d_state->tls_base_caches); idx += 1)
  {
    d_state->tls_base_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(d_state->locals_caches); idx += 1)
  {
    d_state->locals_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(d_state->member_caches); idx += 1)
  {
    d_state->member_caches[idx].arena = arena_alloc();
  }
  
  // rjf: set up run state
  d_state->ctrl_last_run_arena = arena_alloc();
}

internal D_EventList
d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints, D_PathMapArray *path_maps, U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64])
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  D_EventList result = {0};
  d_state->frame_index += 1;
  
  //////////////////////////////
  //- rjf: sync with ctrl thread
  //
  ProfScope("sync with ctrl thread")
  {
    //- rjf: grab next reggen/memgen
    U64 new_mem_gen = ctrl_mem_gen();
    U64 new_reg_gen = ctrl_reg_gen();
    
    //- rjf: consume & process events
    CTRL_EventList events = ctrl_c2u_pop_events(scratch.arena);
    ctrl_entity_store_apply_events(d_state->ctrl_entity_store, &events);
    for(CTRL_EventNode *event_n = events.first;
        event_n != 0;
        event_n = event_n->next)
    {
      CTRL_Event *event = &event_n->v;
      log_infof("ctrl_event:\n{\n");
      log_infof("kind: \"%S\"\n", ctrl_string_from_event_kind(event->kind));
      log_infof("entity_id: %u\n", event->entity_id);
      switch(event->kind)
      {
        default:{}break;
        
        //- rjf: errors
        
        case CTRL_EventKind_Error:
        {
          log_user_error(event->string);
        }break;
        
        //- rjf: starts/stops
        
        case CTRL_EventKind_Started:
        {
          d_state->ctrl_is_running = 1;
          d_state->ctrl_thread_run_state = 1;
        }break;
        
        case CTRL_EventKind_Stopped:
        {
          B32 should_snap = !(d_state->ctrl_soft_halt_issued);
          d_state->ctrl_is_running = 0;
          d_state->ctrl_thread_run_state = 0;
          d_state->ctrl_soft_halt_issued = 0;
          
          // rjf: exception or unexpected trap -> push error
          if(event->cause == CTRL_EventCause_InterruptedByException ||
             event->cause == CTRL_EventCause_InterruptedByTrap)
          {
            log_user_error(str8_zero());
          }
          
          // rjf: gather stop info
          {
            arena_clear(d_state->ctrl_stop_arena);
            MemoryCopyStruct(&d_state->ctrl_last_stop_event, event);
            d_state->ctrl_last_stop_event.string = push_str8_copy(d_state->ctrl_stop_arena, d_state->ctrl_last_stop_event.string);
          }
          
          // rjf: push stop event to caller, if this is not a soft-halt
          if(should_snap)
          {
            CTRL_Entity *thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, event->entity);
            D_EventCause cause = D_EventCause_Null;
            switch(event->cause)
            {
              default:{}break;
              case CTRL_EventCause_InterruptedByHalt:
              {
                if(should_snap)
                {
                  cause = D_EventCause_Halt;
                }
                else
                {
                  cause = D_EventCause_SoftHalt;
                }
              }break;
              case CTRL_EventCause_UserBreakpoint:    {cause = D_EventCause_UserBreakpoint;}break;
            }
            D_EventNode *n = push_array(arena, D_EventNode, 1);
            SLLQueuePush(result.first, result.last, n);
            result.count += 1;
            D_Event *evt = &n->v;
            evt->kind = D_EventKind_Stop;
            evt->cause  = cause;
            evt->thread = thread->kind == CTRL_EntityKind_Thread ? thread->handle : ctrl_handle_zero();
            evt->vaddr  = event->rip_vaddr;
            evt->id     = event->u64_code;
          }
        }break;
        
        //- rjf: entity creation/deletion
        
        case CTRL_EventKind_NewProc:
        {
          // rjf: the first process? -> clear session output
          CTRL_EntityArray existing_processes = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Process);
          if(existing_processes.count == 1)
          {
            MTX_Op op = {r1u64(0, 0xffffffffffffffffull), str8_lit("[new session]\n")};
            mtx_push_op(d_state->output_log_key, op);
          }
        }break;
        
        case CTRL_EventKind_EndProc:
        {
          D_EventNode *n = push_array(arena, D_EventNode, 1);
          SLLQueuePush(result.first, result.last, n);
          result.count += 1;
          D_Event *evt = &n->v;
          evt->kind = D_EventKind_ProcessEnd;
          evt->code = event->u64_code;
        }break;
        
        //- rjf: debug strings
        
        case CTRL_EventKind_DebugString:
        {
          MTX_Op op = {r1u64(max_U64, max_U64), event->string};
          mtx_push_op(d_state->output_log_key, op);
        }break;
        
        //- rjf: memory
        
        case CTRL_EventKind_MemReserve:{}break;
        case CTRL_EventKind_MemCommit:{}break;
        case CTRL_EventKind_MemDecommit:{}break;
        case CTRL_EventKind_MemRelease:{}break;
      }
      log_infof("}\n\n");
    }
    
    //- rjf: clear tls base cache
    if((d_state->tls_base_cache_reggen_idx != new_reg_gen ||
        d_state->tls_base_cache_memgen_idx != new_mem_gen) &&
       !d_ctrl_targets_running())
    {
      d_state->tls_base_cache_gen += 1;
      D_RunTLSBaseCache *cache = &d_state->tls_base_caches[d_state->tls_base_cache_gen%ArrayCount(d_state->tls_base_caches)];
      arena_clear(cache->arena);
      cache->slots_count = 0;
      cache->slots = 0;
      d_state->tls_base_cache_reggen_idx = new_reg_gen;
      d_state->tls_base_cache_memgen_idx = new_mem_gen;
    }
    
    //- rjf: clear locals cache
    if(d_state->locals_cache_reggen_idx != new_reg_gen &&
       !d_ctrl_targets_running())
    {
      d_state->locals_cache_gen += 1;
      D_RunLocalsCache *cache = &d_state->locals_caches[d_state->locals_cache_gen%ArrayCount(d_state->locals_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      d_state->locals_cache_reggen_idx = new_reg_gen;
    }
    
    //- rjf: clear members cache
    if(d_state->member_cache_reggen_idx != new_reg_gen &&
       !d_ctrl_targets_running())
    {
      d_state->member_cache_gen += 1;
      D_RunLocalsCache *cache = &d_state->member_caches[d_state->member_cache_gen%ArrayCount(d_state->member_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      d_state->member_cache_reggen_idx = new_reg_gen;
    }
  }
  
  //////////////////////////////
  //- rjf: hash ctrl parameterization state
  //
  U128 ctrl_param_state_hash = {0};
  {
    // rjf: build data strings of all param data
    String8List strings = {0};
    {
      CTRL_EntityArray threads = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Thread);
      for EachIndex(idx, threads.count)
      {
        CTRL_Entity *thread = threads.v[idx];
        if(thread->is_frozen)
        {
          str8_list_push(scratch.arena, &strings, str8_struct(&thread->id));
          str8_list_push(scratch.arena, &strings, str8_struct(&thread->is_frozen));
        }
      }
      for(U64 idx = 0; idx < breakpoints->count; idx += 1)
      {
        D_Breakpoint *bp = &breakpoints->v[idx];
        str8_list_push(scratch.arena, &strings, bp->file_path);
        str8_list_push(scratch.arena, &strings, str8_struct(&bp->pt));
        str8_list_push(scratch.arena, &strings, bp->vaddr_expr);
        str8_list_push(scratch.arena, &strings, bp->condition);
      }
    }
    
    // rjf: join & hash to produce result
    String8 string = str8_list_join(scratch.arena, &strings, 0);
    XXH128_hash_t hash = XXH3_128bits(string.str, string.size);
    MemoryCopy(&ctrl_param_state_hash, &hash, sizeof(ctrl_param_state_hash));
  }
  
  //////////////////////////////
  //- rjf: if ctrl thread is running, and our ctrl parameterization
  // state hash has changed since the last run, we should soft-
  // halt-refresh to inform the ctrl thread about the updated
  // state
  //
  if(d_ctrl_targets_running() && !u128_match(ctrl_param_state_hash, d_state->ctrl_last_run_param_state_hash))
  {
    d_cmd(D_CmdKind_SoftHaltRefresh);
  }
  
  //////////////////////////////
  //- rjf: process top-level commands
  //
  CTRL_MsgList ctrl_msgs = {0};
  ProfScope("process top-level commands")
  {
    D_BreakpointArray run_extra_bps = {0};
    for(D_Cmd *cmd = 0; d_next_cmd(&cmd);)
    {
      // rjf: unpack command
      D_CmdParams *params = &cmd->params;
      
      // rjf: prep ctrl running arguments
      B32 need_run = 0;
      D_RunKind run_kind = D_RunKind_Run;
      CTRL_Entity *run_thread = &ctrl_entity_nil;
      CTRL_RunFlags run_flags = 0;
      CTRL_TrapList run_traps = {0};
      
      // rjf: process command
      switch(cmd->kind)
      {
        default:{}break;
        
        //- rjf: low-level target control operations
        case D_CmdKind_LaunchAndRun:
        case D_CmdKind_LaunchAndStepInto:
        {
          // rjf: get list of targets to launch
          D_TargetArray *targets_to_launch = &params->targets;
          
          // rjf: no targets => assume all active targets
          if(targets_to_launch->count == 0)
          {
            targets_to_launch = targets;
          }
          
          // rjf: launch
          if(targets_to_launch->count != 0)
          {
            for(U64 idx = 0; idx < targets_to_launch->count; idx += 1)
            {
              // rjf: unpack target
              D_Target *target = &targets_to_launch->v[idx];
              String8 exe                     = str8_skip_chop_whitespace(target->exe);
              String8 args                    = str8_skip_chop_whitespace(target->args);
              String8 working_directory       = str8_skip_chop_whitespace(target->working_directory);
              String8 custom_entry_point_name = str8_skip_chop_whitespace(target->custom_entry_point_name);
              String8 stdout_path             = str8_skip_chop_whitespace(target->stdout_path);
              String8 stderr_path             = str8_skip_chop_whitespace(target->stderr_path);
              String8 stdin_path              = str8_skip_chop_whitespace(target->stdin_path);
              String8List env                 = target->env;
              if(working_directory.size == 0)
              {
                working_directory = os_get_current_path(scratch.arena);
              }
              
              // rjf: build launch options
              String8List cmdln_strings = {0};
              {
                str8_list_push(scratch.arena, &cmdln_strings, exe);
                {
                  U64 start_split_idx = 0;
                  B32 quoted = 0;
                  for(U64 idx = 0; idx <= args.size; idx += 1)
                  {
                    U8 byte = idx < args.size ? args.str[idx] : 0;
                    if(byte == '"')
                    {
                      quoted ^= 1;
                    }
                    B32 splitter_found = (!quoted && (byte == 0 || char_is_space(byte)));
                    if(splitter_found)
                    {
                      String8 string = str8_substr(args, r1u64(start_split_idx, idx));
                      if(string.size > 0)
                      {
                        str8_list_push(scratch.arena, &cmdln_strings, string);
                      }
                      start_split_idx = idx+1;
                    }
                  }
                }
              }
              
              // rjf: push message to launch
              {
                CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
                msg->kind = CTRL_MsgKind_Launch;
                msg->path = working_directory;
                msg->cmd_line_string_list = cmdln_strings;
                msg->stdout_path = stdout_path;
                msg->stderr_path = stderr_path;
                msg->stdin_path  = stdin_path;
                msg->debug_subprocesses = target->debug_subprocesses;
                msg->env_inherit = 1;
                MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
                str8_list_push(scratch.arena, &msg->entry_points, custom_entry_point_name);
                msg->env_string_list = env;
              }
            }
            
            // rjf: run
            need_run = 1;
            run_kind = D_RunKind_Run;
            run_thread = &ctrl_entity_nil;
            run_flags = (cmd->kind == D_CmdKind_LaunchAndStepInto) ? CTRL_RunFlag_StopOnEntryPoint : 0;
          }
          
          // rjf: no targets -> error
          if(targets_to_launch->count == 0)
          {
            log_user_error(str8_lit("No active targets exist; cannot launch. You must select a target first."));
          }
        }break;
        case D_CmdKind_Kill:
        {
          CTRL_Entity *process = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->process);
          if(process == &ctrl_entity_nil)
          {
            log_user_error(str8_lit("Cannot kill; no process was specified."));
          }
          else
          {
            CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
            msg->kind = CTRL_MsgKind_Kill;
            msg->exit_code = 1;
            msg->entity = process->handle;
            MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
          }
        }break;
        case D_CmdKind_KillAll:
        {
          CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
          msg->kind = CTRL_MsgKind_KillAll;
          msg->exit_code = 1;
          MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
        }break;
        case D_CmdKind_Detach:
        {
          CTRL_Entity *process = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->process);
          if(process == &ctrl_entity_nil)
          {
            log_user_error(str8_lit("Cannot detach; no process specified."));
          }
          else
          {
            CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
            msg->kind   = CTRL_MsgKind_Detach;
            msg->entity = process->handle;
            MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
          }
        }break;
        case D_CmdKind_Continue:
        {
          B32 good_to_run = 0;
          CTRL_EntityArray threads = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Thread);
          if(threads.count > 0)
          {
            for EachIndex(idx, threads.count)
            {
              CTRL_Entity *thread = threads.v[idx];
              if(!thread->is_frozen)
              {
                good_to_run = 1;
                break;
              }
            }
            if(good_to_run)
            {
              need_run = 1;
              run_kind = D_RunKind_Run;
              run_thread = &ctrl_entity_nil;
            }
            else
            {
              log_user_error(str8_lit("Cannot run with all threads frozen."));
            }
          }
        }break;
        case D_CmdKind_StepIntoInst:
        case D_CmdKind_StepOverInst:
        case D_CmdKind_StepIntoLine:
        case D_CmdKind_StepOverLine:
        case D_CmdKind_StepOut:
        {
          CTRL_Entity *thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->thread);
          if(thread == &ctrl_entity_nil)
          {
            log_user_error(str8_lit("Must have a selected thread to step."));
          }
          else if(d_ctrl_targets_running())
          {
            if(d_ctrl_last_run_kind() == D_RunKind_Run)
            {
              log_user_error(str8_lit("Must halt before stepping."));
            }
          }
          else if(thread->is_frozen)
          {
            log_user_error(str8_lit("Must thaw selected thread before stepping."));
          }
          else
          {
            B32 good = 1;
            CTRL_TrapList traps = {0};
            switch(cmd->kind)
            {
              default: break;
              case D_CmdKind_StepIntoInst: {}break;
              case D_CmdKind_StepOverInst: {traps = d_trap_net_from_thread__step_over_inst(scratch.arena, thread);}break;
              case D_CmdKind_StepIntoLine: {traps = d_trap_net_from_thread__step_into_line(scratch.arena, thread);}break;
              case D_CmdKind_StepOverLine: {traps = d_trap_net_from_thread__step_over_line(scratch.arena, thread);}break;
              case D_CmdKind_StepOut:
              {
                CTRL_Scope *ctrl_scope = ctrl_scope_open();
                
                // rjf: thread => call stack
                CTRL_CallStack callstack = ctrl_call_stack_from_thread(ctrl_scope, &d_state->ctrl_entity_store->ctx, thread, 1, os_now_microseconds()+10000);
                
                // rjf: use first unwind frame to generate trap
                if(callstack.concrete_frames_count > 1)
                {
                  U64 vaddr = regs_rip_from_arch_block(thread->arch, callstack.concrete_frames[1]->regs);
                  CTRL_Trap trap = {CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck, vaddr};
                  ctrl_trap_list_push(scratch.arena, &traps, &trap);
                }
                else
                {
                  log_user_error(str8_lit("Could not find the return address of the current callstack frame successfully."));
                  good = 0;
                }
                
                ctrl_scope_close(ctrl_scope);
              }break;
            }
            if(good && traps.count != 0)
            {
              need_run   = 1;
              run_kind   = D_RunKind_Step;
              run_thread = thread;
              run_flags  = 0;
              run_traps  = traps;
            }
            if(good && traps.count == 0)
            {
              need_run   = 1;
              run_kind   = D_RunKind_SingleStep;
              run_thread = thread;
              run_flags  = 0;
              run_traps  = traps;
            }
          }
        }break;
        case D_CmdKind_Halt:
        if(d_ctrl_targets_running())
        {
          ctrl_halt();
        }break;
        case D_CmdKind_SoftHaltRefresh:
        if(d_ctrl_targets_running())
        {
          need_run   = 1;
          run_kind   = d_state->ctrl_last_run_kind;
          run_thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, d_state->ctrl_last_run_thread_handle);
          run_flags  = d_state->ctrl_last_run_flags;
          run_traps  = d_state->ctrl_last_run_traps;
        }break;
        case D_CmdKind_SetThreadIP:
        {
          CTRL_Entity *thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->thread);
          U64 vaddr = params->vaddr;
          void *block = ctrl_reg_block_from_thread(scratch.arena, &d_state->ctrl_entity_store->ctx, thread->handle);
          regs_arch_block_write_rip(thread->arch, block, vaddr);
          B32 result = ctrl_thread_write_reg_block(thread->handle, block);
          (void)result;
        }break;
        
        //- rjf: high-level composite target control operations
        case D_CmdKind_RunToLine:
        {
          run_extra_bps.count = 1;
          run_extra_bps.v = push_array(scratch.arena, D_Breakpoint, 1);
          if(params->file_path.size != 0)
          {
            run_extra_bps.v[0].file_path = params->file_path;
            run_extra_bps.v[0].pt        = params->cursor;
          }
          else if(params->vaddr != 0)
          {
            run_extra_bps.v[0].vaddr_expr = push_str8f(scratch.arena, "0x%I64x", params->vaddr);
          }
          d_cmd(D_CmdKind_Run);
        }break;
        case D_CmdKind_Run:
        {
          CTRL_EntityArray processes = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Process);
          if(processes.count != 0)
          {
            d_cmd(D_CmdKind_Continue);
          }
          else if(!d_ctrl_targets_running())
          {
            d_cmd(D_CmdKind_LaunchAndRun);
          }
        }break;
        case D_CmdKind_Restart:
        {
          CTRL_EntityArray processes = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Process);
          if(processes.count != 0)
          {
            d_cmd(D_CmdKind_KillAll);
          }
          d_cmd(D_CmdKind_LaunchAndRun);
        }break;
        case D_CmdKind_StepInto:
        case D_CmdKind_StepOver:
        {
          CTRL_EntityArray processes = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Process);
          if(processes.count != 0)
          {
            D_CmdKind step_cmd_kind = (cmd->kind == D_CmdKind_StepInto
                                       ? D_CmdKind_StepIntoLine
                                       : D_CmdKind_StepOverLine);
            B32 prefer_disasm = params->prefer_disasm;
            if(prefer_disasm)
            {
              step_cmd_kind = (cmd->kind == D_CmdKind_StepInto
                               ? D_CmdKind_StepIntoInst
                               : D_CmdKind_StepOverInst);
            }
            d_cmd(step_cmd_kind, .thread = params->thread);
          }
          else if(!d_ctrl_targets_running())
          {
            d_cmd(D_CmdKind_LaunchAndStepInto, .targets = *targets);
          }
        }break;
        
        //- rjf: debug control context management operations
        case D_CmdKind_FreezeThread:
        case D_CmdKind_ThawThread:
        case D_CmdKind_FreezeProcess:
        case D_CmdKind_ThawProcess:
        case D_CmdKind_FreezeMachine:
        case D_CmdKind_ThawMachine:
        {
          D_CmdKind disptch_kind = ((cmd->kind == D_CmdKind_FreezeThread ||
                                     cmd->kind == D_CmdKind_FreezeProcess ||
                                     cmd->kind == D_CmdKind_FreezeMachine)
                                    ? D_CmdKind_FreezeEntity
                                    : D_CmdKind_ThawEntity);
          d_push_cmd(disptch_kind, params);
        }break;
        case D_CmdKind_FreezeLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          d_cmd(D_CmdKind_FreezeMachine, .entity = ctrl_handle_make(machine_id, dmn_handle_zero()));
        }break;
        case D_CmdKind_ThawLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          d_cmd(D_CmdKind_ThawMachine, .entity = ctrl_handle_make(machine_id, dmn_handle_zero()));
        }break;
        case D_CmdKind_FreezeEntity:
        case D_CmdKind_ThawEntity:
        {
          B32 should_freeze = (cmd->kind == D_CmdKind_FreezeEntity);
          CTRL_Entity *root = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->entity);
          for(CTRL_Entity *e = root; e != &ctrl_entity_nil; e = ctrl_entity_rec_depth_first_pre(e, root).next)
          {
            if(e->kind == CTRL_EntityKind_Thread)
            {
              e->is_frozen = should_freeze;
              CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
              msg->kind   = (should_freeze ? CTRL_MsgKind_FreezeThread : CTRL_MsgKind_ThawThread);
              msg->entity = e->handle;
            }
          }
          if(d_ctrl_targets_running())
          {
            need_run   = 1;
            run_kind   = d_state->ctrl_last_run_kind;
            run_thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, d_state->ctrl_last_run_thread_handle);
            run_flags  = d_state->ctrl_last_run_flags;
            run_traps  = d_state->ctrl_last_run_traps;
          }
        }break;
        
        //- rjf: entity decoration
        case D_CmdKind_SetEntityColor:
        {
          CTRL_Entity *entity = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->entity);
          entity->rgba = params->rgba;
        }break;
        case D_CmdKind_SetEntityName:
        {
          CTRL_Entity *entity = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, params->entity);
          ctrl_entity_equip_string(d_state->ctrl_entity_store, entity, params->string);
        }break;
        
        //- rjf: attaching
        case D_CmdKind_Attach:
        {
          U32 pid = params->pid;
          if(pid != 0)
          {
            CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
            msg->kind      = CTRL_MsgKind_Attach;
            msg->entity_id = pid;
            MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
          }
        }break;
      }
      
      // rjf: do run if needed
      if(need_run)
      {
        // rjf: compute hash of all run-parameterization entities, store
        {
          d_state->ctrl_last_run_param_state_hash = ctrl_param_state_hash;
        }
        
        // rjf: push & fill run message
        CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
        {
          CTRL_Entity *process = ctrl_entity_ancestor_from_kind(run_thread, CTRL_EntityKind_Process);
          msg->kind = (run_kind == D_RunKind_Run || run_kind == D_RunKind_Step) ? CTRL_MsgKind_Run : CTRL_MsgKind_SingleStep;
          msg->run_flags  = run_flags;
          msg->entity     = run_thread->handle;
          msg->parent     = process->handle;
          MemoryCopyArray(msg->exception_code_filters, exception_code_filters);
          MemoryCopyStruct(&msg->traps, &run_traps);
          D_BreakpointArray *bp_batches[] =
          {
            breakpoints,
            &run_extra_bps,
          };
          for(U64 batch_idx = 0; batch_idx < ArrayCount(bp_batches); batch_idx += 1)
          {
            D_BreakpointArray *batch_breakpoints = bp_batches[batch_idx];
            for(U64 idx = 0; idx < batch_breakpoints->count; idx += 1)
            {
              // rjf: unpack user breakpoint entity
              D_Breakpoint *bp = &batch_breakpoints->v[idx];
              
              // rjf: d -> ctrl flags
              CTRL_UserBreakpointFlags ctrl_bp_flags = 0;
              if(bp->flags & D_BreakpointFlag_BreakOnWrite)    { ctrl_bp_flags |= CTRL_UserBreakpointFlag_BreakOnWrite; }
              if(bp->flags & D_BreakpointFlag_BreakOnRead)     { ctrl_bp_flags |= CTRL_UserBreakpointFlag_BreakOnRead; }
              if(bp->flags & D_BreakpointFlag_BreakOnExecute)  { ctrl_bp_flags |= CTRL_UserBreakpointFlag_BreakOnExecute; }
              
              // rjf: textual location -> add breakpoints for all possible override locations
              if(bp->file_path.size != 0 && bp->pt.line != 0)
              {
                String8List overrides = d_possible_path_overrides_from_maps_path(scratch.arena, path_maps, bp->file_path);
                for(String8Node *n = overrides.first; n != 0; n = n->next)
                {
                  CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_FileNameAndLineColNumber};
                  ctrl_user_bp.flags     = ctrl_bp_flags;
                  ctrl_user_bp.id        = bp->id;
                  ctrl_user_bp.string    = n->string;
                  ctrl_user_bp.pt        = bp->pt;
                  ctrl_user_bp.condition = bp->condition;
                  ctrl_user_bp.size      = bp->size;
                  ctrl_user_breakpoint_list_push(scratch.arena, &msg->user_bps, &ctrl_user_bp);
                }
              }
              
              // rjf: virtual address expression -> add expression breakpoint
              else if(bp->vaddr_expr.size != 0)
              {
                CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_Expression};
                ctrl_user_bp.flags     = ctrl_bp_flags;
                ctrl_user_bp.id        = bp->id;
                ctrl_user_bp.string    = bp->vaddr_expr;
                ctrl_user_bp.condition = bp->condition;
                ctrl_user_bp.size      = bp->size;
                ctrl_user_breakpoint_list_push(scratch.arena, &msg->user_bps, &ctrl_user_bp);
              }
            }
          }
        }
        
        // rjf: copy run traps to scratch (needed, if run_traps can be `d_state->ctrl_last_run_traps`)
        CTRL_TrapList run_traps_copy = ctrl_trap_list_copy(scratch.arena, &run_traps);
        D_BreakpointArray run_extra_bps_copy = d_breakpoint_array_copy(scratch.arena, &run_extra_bps);
        
        // rjf: store last run info
        arena_clear(d_state->ctrl_last_run_arena);
        d_state->ctrl_last_run_kind              = run_kind;
        d_state->ctrl_last_run_frame_idx         = d_frame_index();
        d_state->ctrl_last_run_thread_handle     = run_thread->handle;
        d_state->ctrl_last_run_flags             = run_flags;
        d_state->ctrl_last_run_traps             = ctrl_trap_list_copy(d_state->ctrl_last_run_arena, &run_traps_copy);
        d_state->ctrl_last_run_extra_bps         = d_breakpoint_array_copy(d_state->ctrl_last_run_arena, &run_extra_bps_copy);
        d_state->ctrl_is_running                 = 1;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: clear command batch
  //
  {
    arena_clear(d_state->cmds_arena);
    MemoryZeroStruct(&d_state->cmds);
  }
  
  //////////////////////////////
  //- rjf: push new control messages to queue - try to send queue to control,
  // clear queue if successful (if not, we'll just keep them around until
  // the next tick)
  //
  {
    CTRL_MsgList msgs_copy = ctrl_msg_list_deep_copy(d_state->ctrl_msg_arena, &ctrl_msgs);
    ctrl_msg_list_concat_in_place(&d_state->ctrl_msgs, &msgs_copy);
    if(d_state->ctrl_msgs.count != 0)
    {
      if(!d_state->ctrl_soft_halt_issued && d_state->ctrl_thread_run_state)
      {
        d_state->ctrl_soft_halt_issued = 1;
        ctrl_halt();
      }
      if(ctrl_u2c_push_msgs(&d_state->ctrl_msgs, os_now_microseconds()+100))
      {
        MemoryZeroStruct(&d_state->ctrl_msgs);
        arena_clear(d_state->ctrl_msg_arena);
      }
    }
  }
  
  ProfEnd();
  scratch_end(scratch);
  return result;
}
