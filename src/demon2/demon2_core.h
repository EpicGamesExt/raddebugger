// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON2_CORE_H
#define DEMON2_CORE_H

////////////////////////////////
//~ rjf: Handle Types

typedef struct DMN_Handle DMN_Handle;
struct DMN_Handle
{
  U32 u32[2];
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

#include "generated/demon2.meta.h"

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

typedef struct DMN_Trap DMN_Trap;
struct DMN_Trap
{
  DMN_Handle process;
  U64 vaddr;
  U64 id;
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
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void dmn_init(void);

////////////////////////////////
//~ rjf: @dmn_os_hooks Running/Halting (Implemented Per-OS)

internal DMN_EventList dmn_run(Arena *arena, DMN_RunCtrls *ctrls);
internal void dmn_halt(U64 code, U64 user_data);

////////////////////////////////
//~ rjf: @dmn_os_hooks Process Launching/Attaching/Killing/Detaching (Implemented Per-OS)

internal U32 dmn_launch_process(OS_LaunchOptions *options);
internal B32 dmn_attach_process(U32 pid);
internal B32 dmn_kill_process(DMN_Handle process, U32 exit_code);
internal B32 dmn_detach_process(DMN_Handle process);

////////////////////////////////
//~ rjf: @dmn_os_hooks Entities (Implemented Per-OS)

//- rjf: basic entity info extraction
internal Architecture dmn_arch_from_handle(DMN_Handle handle);
internal String8      dmn_string_from_handle(Arena *arena, DMN_Handle handle);

//- rjf: processes
internal U64          dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst);
internal B32          dmn_process_write(DMN_Handle process, Rng1U64 range, void *src);
#define dmn_process_read_struct(process, vaddr, ptr) dmn_process_read((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)
#define dmn_process_write_struct(process, vaddr, ptr) dmn_process_write((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)

//- rjf: modules
internal Rng1U64      dmn_vaddr_range_from_module(DMN_Handle handle);

//- rjf: threads
internal U64          dmn_stack_base_vaddr_from_thread(DMN_Handle handle);
internal U64          dmn_tls_root_vaddr_from_thread(DMN_Handle handle);
internal B32          dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block);
internal B32          dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block);

////////////////////////////////
//~ rjf: @dmn_os_hooks System Process Listing (Implemented Per-OS)

internal void dmn_process_iter_begin(DMN_ProcessIter *iter);
internal B32  dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out);
internal void dmn_process_iter_end(DMN_ProcessIter *iter);

#endif // DEMON2_CORE_H
