// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)



////////////////////////////////
//~  @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  dmn_lnx_arena = arena_alloc();
  dmn_lnx = push_array(dmn_lnx_arena, DMN_LNX_Shared, 1);
  dmn_lnx->arena = dmn_lnx_arena;
  dmn_lnx->mutex_access = os_mutex_alloc();
}

////////////////////////////////
//~  @dmn_os_hooks Blocking Control Thread Operations (Implemented Per-OS)

internal DMN_CtrlCtx
*dmn_ctrl_begin(void)
{
  // NOTE: Boolean return, just says the context is valid
  DMN_CtrlCtx* ctx = (DMN_CtrlCtx* )1;
  dmn_lnx_ctrl_thread = 1;
  return ctx;
}

internal void
dmn_ctrl_exclusive_access_begin(void)
{
  os_mutex_take(dmn_lnx->mutex_access);
}

internal void
dmn_ctrl_exclusive_access_end(void)
{
  os_mutex_drop(dmn_lnx->mutex_access);
}

internal U32
dmn_ctrl_launch(DMN_CtrlCtx *ctx, OS_LaunchOptions *options)
{
  NotImplemented;
  U32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  AssertAlways(options->inherit_env); // NOTE(mallchad): Figure out no-inherit later
  OS_Handle process = {0};
  B32 success = 0;

  DMN_AccessScope
  {
    success = os_launch_process(options, &process);
  }
  /* NOTE(mallchad): I don't believe there is a neccessary Linux equivilent to AllocConsole.
     If you wanted to redirect stdin/stdout here would probably be the best place */
  if (success) { result = *process.u64; }
  scratch_end(scratch);

  return result;
}

/* TODO: This behaviour is governed by kernel.yama.ptrace_scope kernel parameter
   This information should probably be relayed to the user in future. */
internal B32
dmn_ctrl_attach(DMN_CtrlCtx *ctx, U32 pid)
{
  S32 error = ptrace(PTRACE_ATTACH, pid);
  return (error == 0);
}

internal B32
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process, U32 exit_code)
{
  NotImplemented;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process)
{
  NotImplemented;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  NotImplemented;
}

////////////////////////////////
//~  @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
  NotImplemented;
}

////////////////////////////////
//~  @dmn_os_hooks Introspection Functions (Implemented Per-OS)

//-  run/memory/register counters
internal U64
dmn_run_gen(void)
{
  return ins_atomic_u64_eval(&dmn_lnx->counter_run);
}

internal U64
dmn_mem_gen(void)
{
  return ins_atomic_u64_eval(&dmn_lnx->counter_mem);
}

internal U64
dmn_reg_gen(void)
{
  return ins_atomic_u64_eval(&dmn_lnx->counter_reg);
}

//-  non-blocking-control-thread access barriers

internal B32
dmn_access_open(void)
{
  // TODO(mallchad): I feel like this needs to be improved but I don't know how
  if (dmn_lnx_ctrl_thread)
  { }
  else
  { os_mutex_take(dmn_lnx->mutex_access); }
  return 1;
}
internal void
dmn_access_close(void)
{
  if (dmn_lnx_ctrl_thread == 0)
  { os_mutex_drop(dmn_lnx->mutex_access); }
}

//-  processes
internal U64
dmn_process_memory_reserve(DMN_Handle process, U64 vaddr, U64 size)
{
  NotImplemented;
}

internal void
dmn_process_memory_commit(DMN_Handle process, U64 vaddr, U64 size)
{
  NotImplemented;
}

internal void
dmn_process_memory_decommit(DMN_Handle process, U64 vaddr, U64 size)
{
  NotImplemented;
}

internal void
dmn_process_memory_release(DMN_Handle process, U64 vaddr, U64 size)
{
  NotImplemented;
}

internal void
dmn_process_memory_protect(DMN_Handle process, U64 vaddr, U64 size, OS_AccessFlags flags)
{
  NotImplemented;
}

internal U64
dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst)
{
  NotImplemented;
}

internal B32
dmn_process_write(DMN_Handle process, Rng1U64 range, void *src)
{
  NotImplemented;
}

//-  threads
internal Architecture dmn_arch_from_thread(DMN_Handle handle)
{
  NotImplemented;
}
internal U64 dmn_stack_base_vaddr_from_thread(DMN_Handle handle)
{
  NotImplemented;
}
internal U64 dmn_tls_root_vaddr_from_thread(DMN_Handle handle)
{
  NotImplemented;
}
internal B32
dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block)
{
  NotImplemented;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block)
{
  NotImplemented;
}

//-  system process listing
internal void
dmn_process_iter_begin(DMN_ProcessIter *iter)
{
  NotImplemented;
}

internal B32
dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out)
{
  NotImplemented;
}

internal void
dmn_process_iter_end(DMN_ProcessIter *iter)
{
  NotImplemented;
}
