// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_H
#define DEMON_CORE_H

////////////////////////////////
//~ allen: Demon Low Level Entities

typedef U64 DEMON_Handle;

typedef struct DEMON_HandleNode DEMON_HandleNode;
struct DEMON_HandleNode
{
  DEMON_HandleNode *next;
  DEMON_Handle v;
};

typedef struct DEMON_HandleList DEMON_HandleList;
struct DEMON_HandleList
{
  DEMON_HandleNode *first;
  DEMON_HandleNode *last;
  U64 count;
};

typedef struct DEMON_HandleArray DEMON_HandleArray;
struct DEMON_HandleArray
{
  DEMON_Handle *handles;
  U64 count;
};

////////////////////////////////
//~ rjf: Memory Protection Flags

typedef U32 DEMON_MemoryProtectFlags;
enum{
  DEMON_MemoryProtectFlag_Read    = (1<<0),
  DEMON_MemoryProtectFlag_Write   = (1<<1),
  DEMON_MemoryProtectFlag_Execute = (1<<2),
};

////////////////////////////////
//~ allen: Demon Event Types

typedef enum DEMON_EventKind
{
  DEMON_EventKind_Null,
  DEMON_EventKind_Error,
  DEMON_EventKind_HandshakeComplete,
  DEMON_EventKind_CreateProcess,
  DEMON_EventKind_ExitProcess,
  DEMON_EventKind_CreateThread,
  DEMON_EventKind_ExitThread,
  DEMON_EventKind_LoadModule,
  DEMON_EventKind_UnloadModule,
  DEMON_EventKind_Breakpoint,
  DEMON_EventKind_Trap,
  DEMON_EventKind_SingleStep,
  DEMON_EventKind_Exception,
  DEMON_EventKind_Halt,
  DEMON_EventKind_Memory,
  DEMON_EventKind_DebugString,
  DEMON_EventKind_SetThreadName,
  DEMON_EventKind_COUNT
}
DEMON_EventKind;

typedef enum DEMON_ErrorKind
{
  DEMON_ErrorKind_Null,
  DEMON_ErrorKind_NotInitialized,
  DEMON_ErrorKind_NotAttached,
  DEMON_ErrorKind_UnexpectedFailure,
  DEMON_ErrorKind_InvalidHandle,
}
DEMON_ErrorKind;

typedef enum DEMON_MemoryEventKind
{
  DEMON_MemoryEventKind_Null,
  DEMON_MemoryEventKind_Commit,
  DEMON_MemoryEventKind_Reserve,
  DEMON_MemoryEventKind_Decommit,
  DEMON_MemoryEventKind_Release,
  DEMON_MemoryEventKind_COUNT
}
DEMON_MemoryEventKind;

typedef enum DEMON_ExceptionKind
{
  DEMON_ExceptionKind_Null,
  DEMON_ExceptionKind_MemoryRead,
  DEMON_ExceptionKind_MemoryWrite,
  DEMON_ExceptionKind_MemoryExecute,
  DEMON_ExceptionKind_CppThrow,
  DEMON_ExceptionKind_COUNT
}
DEMON_ExceptionKind;

typedef struct DEMON_Event DEMON_Event;
struct DEMON_Event
{
  // TODO(allen): condense
  DEMON_EventKind kind;
  DEMON_ErrorKind error_kind;
  DEMON_MemoryEventKind memory_kind;
  DEMON_ExceptionKind exception_kind;
  DEMON_Handle process;
  DEMON_Handle thread;
  DEMON_Handle module;
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

typedef struct DEMON_EventNode DEMON_EventNode;
struct DEMON_EventNode
{
  DEMON_EventNode *next;
  DEMON_Event v;
};

typedef struct DEMON_EventList DEMON_EventList;
struct DEMON_EventList
{
  DEMON_EventNode *first;
  DEMON_EventNode *last;
  U64 count;
};

////////////////////////////////
//~ allen: Demon Run Control Types

typedef struct DEMON_Trap DEMON_Trap;
struct DEMON_Trap
{
  DEMON_Handle process;
  U64 address;
  U64 id;
};

typedef struct DEMON_TrapChunkNode DEMON_TrapChunkNode;
struct DEMON_TrapChunkNode
{
  DEMON_TrapChunkNode *next;
  DEMON_Trap *v;
  U64 cap;
  U64 count;
};

typedef struct DEMON_TrapChunkList DEMON_TrapChunkList;
struct DEMON_TrapChunkList
{
  DEMON_TrapChunkNode *first;
  DEMON_TrapChunkNode *last;
  U64 node_count;
  U64 trap_count;
};

typedef struct DEMON_RunCtrls DEMON_RunCtrls;
struct DEMON_RunCtrls
{
  DEMON_Handle single_step_thread;
  B8 ignore_previous_exception;
  B8 run_entities_are_unfrozen;
  B8 run_entities_are_processes;
  DEMON_Handle *run_entities;
  U64 run_entity_count;
  DEMON_TrapChunkList traps;
};

////////////////////////////////
//~ allen: Demon Process Listing

typedef struct DEMON_ProcessIter DEMON_ProcessIter;
struct DEMON_ProcessIter
{
  U64 v[2];
};

typedef struct DEMON_ProcessInfo DEMON_ProcessInfo;
struct DEMON_ProcessInfo
{
  String8 name;
  U32 pid;
};

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void demon_init(void);

////////////////////////////////
//~ rjf: Basic Type Functions

//- rjf: stringizing
internal String8 demon_string_from_event_kind(DEMON_EventKind kind);
internal String8 demon_string_from_memory_event_kind(DEMON_MemoryEventKind kind);
internal String8 demon_string_from_exception_kind(DEMON_ExceptionKind kind);
internal void    demon_string_list_from_event(Arena *arena, String8List *out, DEMON_Event *event);

//- rjf: trap chunk lists
internal void demon_trap_chunk_list_push(Arena *arena, DEMON_TrapChunkList *list, U64 cap, DEMON_Trap *trap);
internal void demon_trap_chunk_list_concat_in_place(DEMON_TrapChunkList *dst, DEMON_TrapChunkList *to_push);
internal void demon_trap_chunk_list_concat_shallow_copy(Arena *arena, DEMON_TrapChunkList *dst, DEMON_TrapChunkList *to_push);

//- rjf: handle lists
internal void demon_handle_list_push(Arena *arena, DEMON_HandleList *list, DEMON_Handle handle);
internal DEMON_HandleArray demon_handle_array_from_list(Arena *arena, DEMON_HandleList *list);
internal DEMON_HandleArray demon_handle_array_copy(Arena *arena, DEMON_HandleArray *src);

////////////////////////////////
//~ rjf: Primary Thread & Exclusive Mode Controls

internal void demon_primary_thread_begin(void);
internal void demon_exclusive_mode_begin(void);
internal void demon_exclusive_mode_end(void);

////////////////////////////////
//~ rjf: Running/Halting

internal DEMON_EventList demon_run(Arena *arena, DEMON_RunCtrls *ctrls);
internal void demon_halt(U64 code, U64 user_data);
internal U64 demon_get_time_counter(void);

////////////////////////////////
//~ rjf: Target Process Launching/Attaching/Killing/Detaching/Halting

internal U32 demon_launch_process(OS_LaunchOptions *options);
internal B32 demon_attach_process(U32 pid);
internal B32 demon_kill_process(DEMON_Handle process, U32 exit_code);
internal B32 demon_detach_process(DEMON_Handle process);
internal DEMON_Handle demon_snapshot_thread(DEMON_Handle process);
internal void demon_snapshot_release(DEMON_Handle snapshot);

////////////////////////////////
//~ rjf: Entity Functions

//- rjf: basics
internal B32 demon_object_exists(DEMON_Handle object);

//- rjf: introspection
internal Architecture      demon_arch_from_object(DEMON_Handle object);
internal U64               demon_base_vaddr_from_module(DEMON_Handle module);
internal Rng1U64           demon_vaddr_range_from_module(DEMON_Handle module);
internal String8           demon_full_path_from_module(Arena *arena, DEMON_Handle module);
internal U64               demon_stack_base_vaddr_from_thread(DEMON_Handle thread);
internal U64               demon_tls_root_vaddr_from_thread(DEMON_Handle thread);
internal DEMON_HandleArray demon_all_processes(Arena *arena);
internal DEMON_HandleArray demon_threads_from_process(Arena *arena, DEMON_Handle process);
internal DEMON_HandleArray demon_modules_from_process(Arena *arena, DEMON_Handle process);

//- rjf: target process memory allocation/protection
internal U64 demon_reserve_memory(DEMON_Handle process, U64 size);
internal B32 demon_set_memory_protect_flags(DEMON_Handle process, U64 page_vaddr, U64 size, DEMON_MemoryProtectFlags flags);
internal B32 demon_release_memory(DEMON_Handle process, U64 vaddr, U64 size);

//- rjf: target process memory reading/writing
internal U64 demon_read_memory(DEMON_Handle process, void *dst, U64 src_address, U64 size);
internal B32 demon_write_memory(DEMON_Handle process, U64 dst_address, void *src, U64 size);
internal U64 demon_read_memory_amap_aligned(DEMON_Handle process, void *dst, U64 src_address, U64 size);
internal U64 demon_read_memory_amap(DEMON_Handle process, void *dst, U64 src_address, U64 size);

//- rjf: thread registers reading/writing
// IMPORTANT(allen): This API is _trusting_ you. You should never modify the data pointed
// at by that void pointer! It is pointing to the internal cache of the registers, so it
// will become invalid after a call to demon_write_regs, or demon_run. Use it to read
// what you need and be done ASAP and we can avoid an extra copy baked into the API.
internal void *demon_read_regs(DEMON_Handle thread);
internal B32   demon_write_regs(DEMON_Handle thread, void *data);
// TODO(allen): These might be a bad idea when we try to extend to ARM
// They make sense for x86/x64 abstraction, which often needs identical
// code paths except for these parts. Revisit this when ARM is integrated.
internal U64  demon_read_ip(DEMON_Handle thread);
internal U64  demon_read_sp(DEMON_Handle thread);
internal void demon_write_ip(DEMON_Handle thread, U64 ip);

////////////////////////////////
//~ rjf: Process Listing

internal void demon_proc_iter_begin(DEMON_ProcessIter *iter);
internal B32  demon_proc_iter_next(Arena *arena, DEMON_ProcessIter *iter, DEMON_ProcessInfo *info_out);
internal void demon_proc_iter_end(DEMON_ProcessIter *iter);

#endif //DEMON_CORE_H
