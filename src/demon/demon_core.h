// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_H
#define DEMON_CORE_H

////////////////////////////////
//~ rjf: Control-Thread-Only Context
//
// An instance of this struct must ONLY be returned by dmn_ctrl_begin, and only
// used by the thread which called it. All APIs which can ONLY run on the
// control thread, which blocks to control & receive events, will take this
// parameter. All other APIs can be called from any thread.

typedef struct DMN_CtrlCtx DMN_CtrlCtx;
struct DMN_CtrlCtx
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Handle Types

typedef union DMN_Handle DMN_Handle;
union DMN_Handle
{
  U32 u32[2];
  U64 u64[1];
};

typedef struct DMN_HandleNode DMN_HandleNode;
struct DMN_HandleNode
{
  DMN_HandleNode *next;
  DMN_Handle v;
};

typedef struct DMN_HandleList DMN_HandleList;
struct DMN_HandleList
{
  DMN_HandleNode *first;
  DMN_HandleNode *last;
  U64 count;
};

typedef struct DMN_HandleArray DMN_HandleArray;
struct DMN_HandleArray
{
  DMN_Handle *handles;
  U64 count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/demon.meta.h"

////////////////////////////////
//~ rjf: Event Types

typedef struct DMN_Event DMN_Event;
struct DMN_Event
{
  DMN_EventKind kind;
  DMN_ErrorKind error_kind;
  DMN_MemoryEventKind memory_kind;
  DMN_ExceptionKind exception_kind;
  DMN_Handle process;
  DMN_Handle thread;
  DMN_Handle module;
  Arch arch;
  U64 address;
  U64 size;
  String8 string;
  U32 code; // code gives pid & tid on CreateProcess and CreateThread (respectfully)
  U32 flags;
  S32 signo;
  S32 sigcode;
  U64 instruction_pointer;
  U64 stack_pointer;
  U64 user_data;
  B32 exception_repeated;
};

typedef struct DMN_EventNode DMN_EventNode;
struct DMN_EventNode
{
  DMN_EventNode *next;
  DMN_Event v;
};

typedef struct DMN_EventList DMN_EventList;
struct DMN_EventList
{
  DMN_EventNode *first;
  DMN_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Run Control Types

typedef U32 DMN_TrapFlags;
enum
{
  DMN_TrapFlag_BreakOnWrite   = (1<<0),
  DMN_TrapFlag_BreakOnRead    = (1<<1),
  DMN_TrapFlag_BreakOnExecute = (1<<2),
};

typedef struct DMN_Trap DMN_Trap;
struct DMN_Trap
{
  DMN_Handle process;
  U64 vaddr;
  U64 id;
  DMN_TrapFlags flags;
  U32 length;
};

typedef struct DMN_TrapChunkNode DMN_TrapChunkNode;
struct DMN_TrapChunkNode
{
  DMN_TrapChunkNode *next;
  DMN_Trap *v;
  U64 cap;
  U64 count;
};

typedef struct DMN_TrapChunkList DMN_TrapChunkList;
struct DMN_TrapChunkList
{
  DMN_TrapChunkNode *first;
  DMN_TrapChunkNode *last;
  U64 node_count;
  U64 trap_count;
};

typedef struct DMN_RunCtrls DMN_RunCtrls;
struct DMN_RunCtrls
{
  DMN_Handle single_step_thread;
  B8 ignore_previous_exception;
  B8 run_entities_are_unfrozen;
  B8 run_entities_are_processes;
  DMN_Handle *run_entities;
  U64 run_entity_count;
  DMN_TrapChunkList traps;
};

////////////////////////////////
//~ rjf: System Process Listing Types

typedef struct DMN_ProcessIter DMN_ProcessIter;
struct DMN_ProcessIter
{
  U64 v[2];
};

typedef struct DMN_ProcessInfo DMN_ProcessInfo;
struct DMN_ProcessInfo
{
  String8 name;
  U32 pid;
};

////////////////////////////////
//~ rjf: Basic Type Functions (Helpers, Implemented Once)

//- rjf: handles
internal DMN_Handle dmn_handle_zero(void);
internal B32 dmn_handle_match(DMN_Handle a, DMN_Handle b);

//- rjf: trap chunk lists
internal void dmn_trap_chunk_list_push(Arena *arena, DMN_TrapChunkList *list, U64 cap, DMN_Trap *trap);
internal void dmn_trap_chunk_list_concat_in_place(DMN_TrapChunkList *dst, DMN_TrapChunkList *to_push);
internal void dmn_trap_chunk_list_concat_shallow_copy(Arena *arena, DMN_TrapChunkList *dst, DMN_TrapChunkList *to_push);

//- rjf: handle lists
internal void dmn_handle_list_push(Arena *arena, DMN_HandleList *list, DMN_Handle handle);
internal DMN_HandleArray dmn_handle_array_from_list(Arena *arena, DMN_HandleList *list);
internal DMN_HandleArray dmn_handle_array_copy(Arena *arena, DMN_HandleArray *src);

//- rjf: event list building
internal DMN_Event *dmn_event_list_push(Arena *arena, DMN_EventList *list);

////////////////////////////////
//~ rjf: Thread Reading Helper Functions (Helpers, Implemented Once)

internal U64 dmn_rip_from_thread(DMN_Handle thread);
internal U64 dmn_rsp_from_thread(DMN_Handle thread);

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void dmn_init(void);

////////////////////////////////
//~ rjf: @dmn_os_hooks Blocking Control Thread Operations (Implemented Per-OS)

internal DMN_CtrlCtx *dmn_ctrl_begin(void);
internal void dmn_ctrl_exclusive_access_begin(void);
internal void dmn_ctrl_exclusive_access_end(void);
#define DMN_CtrlExclusiveAccessScope DeferLoop(dmn_ctrl_exclusive_access_begin(), dmn_ctrl_exclusive_access_end())
internal U32 dmn_ctrl_launch(DMN_CtrlCtx *ctx, OS_ProcessLaunchParams *params);
internal B32 dmn_ctrl_attach(DMN_CtrlCtx *ctx, U32 pid);
internal B32 dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process, U32 exit_code);
internal B32 dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process);
internal DMN_EventList dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls);

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void dmn_halt(U64 code, U64 user_data);

////////////////////////////////
//~ rjf: @dmn_os_hooks Introspection Functions (Implemented Per-OS)

//- rjf: run/memory/register counters
internal U64 dmn_run_gen(void);
internal U64 dmn_mem_gen(void);
internal U64 dmn_reg_gen(void);

//- rjf: non-blocking-control-thread access barriers
internal B32 dmn_access_open(void);
internal void dmn_access_close(void);
#define DMN_AccessScope DeferLoopChecked(dmn_access_open(), dmn_access_close())

//- rjf: processes
internal U64 dmn_process_memory_reserve(DMN_Handle process, U64 vaddr, U64 size);
internal void dmn_process_memory_commit(DMN_Handle process, U64 vaddr, U64 size);
internal void dmn_process_memory_decommit(DMN_Handle process, U64 vaddr, U64 size);
internal void dmn_process_memory_release(DMN_Handle process, U64 vaddr, U64 size);
internal void dmn_process_memory_protect(DMN_Handle process, U64 vaddr, U64 size, OS_AccessFlags flags);
internal U64 dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst);
internal B32 dmn_process_write(DMN_Handle process, Rng1U64 range, void *src);
#define dmn_process_read_struct(process, vaddr, ptr) dmn_process_read((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)
#define dmn_process_write_struct(process, vaddr, ptr) dmn_process_write((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)
internal String8 dmn_process_read_cstring(Arena *arena, DMN_Handle process, U64 addr);

//- rjf: threads
internal Arch dmn_arch_from_thread(DMN_Handle handle);
internal U64 dmn_stack_base_vaddr_from_thread(DMN_Handle handle);
internal U64 dmn_tls_root_vaddr_from_thread(DMN_Handle handle);
internal B32 dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block);
internal B32 dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block);

//- rjf: system process listing
internal void dmn_process_iter_begin(DMN_ProcessIter *iter);
internal B32  dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out);
internal void dmn_process_iter_end(DMN_ProcessIter *iter);

#endif // DEMON_CORE_H
