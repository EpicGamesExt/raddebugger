// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_OS_H
#define DEMON_OS_H

// NOTE(allen):
// These are the functions that the OS backends actually implement.
// Demon objects go through a handle validation layer but it is a lot more
// convenient in the OS backends to implement these versions which take the
// already validated DEMON_Entity*. These are also more convenient to call from
// the backend layer, which lets us avoid converting back and forth between
// handles and pointers a lot.

////////////////////////////////
//~ NOTE(allen): Demon OS Run Control Types

typedef struct DEMON_OS_Trap DEMON_OS_Trap;
struct DEMON_OS_Trap
{
  DEMON_Entity *process;
  U64 address;
};

typedef struct DEMON_OS_RunCtrls DEMON_OS_RunCtrls;
struct DEMON_OS_RunCtrls
{
  DEMON_Entity *single_step_thread;
  B8 ignore_previous_exception;
  B8 run_entities_are_unfrozen;
  B8 run_entities_are_processes;
  DEMON_Entity **run_entities;
  U64 run_entity_count;
  DEMON_OS_Trap *traps;
  U64 trap_count;
};

////////////////////////////////
//~ rjf: @demon_os_hooks Main Layer Initialization

internal void demon_os_init(void);

////////////////////////////////
//~ rjf: @demon_os_hooks Running/Halting

internal DEMON_EventList demon_os_run(Arena *arena, DEMON_OS_RunCtrls *controls);
internal void demon_os_halt(U64 code, U64 user_data);

////////////////////////////////
//~ rjf: @demon_os_hooks Target Process Launching/Attaching/Killing/Detaching/Halting

internal U32 demon_os_launch_process(OS_LaunchOptions *options);
internal B32 demon_os_attach_process(U32 pid);
internal B32 demon_os_kill_process(DEMON_Entity *process, U32 exit_code);
internal B32 demon_os_detach_process(DEMON_Entity *process);

internal DEMON_Handle demon_os_create_snapshot(DEMON_Entity *thread);
internal void         demon_os_snapshot_release(DEMON_Entity *entity);

////////////////////////////////
//~ rjf: @demon_os_hooks Entity Functions

//- rjf: cleanup
internal void demon_os_entity_cleanup(DEMON_Entity *entity);

//- rjf: introspection
internal String8 demon_os_full_path_from_module(Arena *arena, DEMON_Entity *module);
internal U64 demon_os_stack_base_vaddr_from_thread(DEMON_Entity *thread);
internal U64 demon_os_tls_root_vaddr_from_thread(DEMON_Entity *thread);

//- rjf: target process memory allocation/protection
internal U64  demon_os_reserve_memory(DEMON_Entity *process, U64 size);
internal void demon_os_set_memory_protect_flags(DEMON_Entity *process, U64 page_vaddr, U64 size, DEMON_MemoryProtectFlags flags);
internal void demon_os_release_memory(DEMON_Entity *process, U64 vaddr, U64 size);

//- rjf: target process memory reading/writing
internal U64 demon_os_read_memory(DEMON_Entity *process, void *dst, U64 src_address, U64 size);
internal B32 demon_os_write_memory(DEMON_Entity *process, U64 dst_address, void *src, U64 size);
#define demon_os_read_struct(p,dst,src)  demon_os_read_memory((p), (dst), (src), sizeof(*(dst)))
#define demon_os_write_struct(p,dst,src) demon_os_write_memory((p), (dst), (src), sizeof(*(src)))

//- rjf: thread registers reading/writing
internal B32 demon_os_read_regs_x86(DEMON_Entity *thread, REGS_RegBlockX86 *dst);
internal B32 demon_os_write_regs_x86(DEMON_Entity *thread, REGS_RegBlockX86 *src);
internal B32 demon_os_read_regs_x64(DEMON_Entity *thread, REGS_RegBlockX64 *dst);
internal B32 demon_os_write_regs_x64(DEMON_Entity *thread, REGS_RegBlockX64 *src);

////////////////////////////////
//~ rjf: @demon_os_hooks Process Listing

internal void demon_os_proc_iter_begin(DEMON_ProcessIter *iter);
internal B32  demon_os_proc_iter_next(Arena *arena, DEMON_ProcessIter *iter, DEMON_ProcessInfo *info_out);
internal void demon_os_proc_iter_end(DEMON_ProcessIter *iter);

#endif //DEMON_OS_H
