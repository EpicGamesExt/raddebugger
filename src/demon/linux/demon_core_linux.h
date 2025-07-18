// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_LINUX_H
#define DEMON_CORE_LINUX_H

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <elf.h>
#include <dirent.h>
#include <errno.h>

#define DMN_LNX_PTRACE_OPTIONS (PTRACE_O_TRACEEXIT|\
PTRACE_O_EXITKILL|\
PTRACE_O_TRACEFORK|\
PTRACE_O_TRACEVFORK|\
PTRACE_O_TRACECLONE)

typedef struct DMN_LNX_ProcessAux DMN_LNX_ProcessAux;
struct DMN_LNX_ProcessAux
{
  B32 filled;
  U64 phnum;
  U64 phent;
  U64 phdr;
  U64 execfn;
};

typedef struct DMN_LNX_PhdrInfo DMN_LNX_PhdrInfo;
struct DMN_LNX_PhdrInfo
{
  Rng1U64 range;
  U64 dynamic;
};

typedef struct DMN_LNX_ModuleInfo DMN_LNX_ModuleInfo;
struct DMN_LNX_ModuleInfo
{
  Rng1U64 vaddr_range;
  U64 name;
};

typedef struct DMN_LNX_ModuleInfoNode DMN_LNX_ModuleInfoNode;
struct DMN_LNX_ModuleInfoNode
{
  DMN_LNX_ModuleInfoNode *next;
  DMN_LNX_ModuleInfo v;
};

typedef struct DMN_LNX_ModuleInfoList DMN_LNX_ModuleInfoList;
struct DMN_LNX_ModuleInfoList
{
  DMN_LNX_ModuleInfoNode *first;
  DMN_LNX_ModuleInfoNode *last;
  U64 count;
};

typedef enum DMN_LNX_EntityKind
{
  DMN_LNX_EntityKind_Null,
  DMN_LNX_EntityKind_Root,
  DMN_LNX_EntityKind_Process,
  DMN_LNX_EntityKind_Thread,
  DMN_LNX_EntityKind_Module,
  DMN_LNX_EntityKind_COUNT
}
DMN_LNX_EntityKind;

typedef struct DMN_LNX_Entity DMN_LNX_Entity;
struct DMN_LNX_Entity
{
  DMN_LNX_Entity *first;
  DMN_LNX_Entity *last;
  DMN_LNX_Entity *next;
  DMN_LNX_Entity *prev;
  DMN_LNX_Entity *parent;
  DMN_LNX_EntityKind kind;
  U32 gen;
  Arch arch;
  U64 id;
  int fd;
};

typedef struct DMN_LNX_EntityNode DMN_LNX_EntityNode;
struct DMN_LNX_EntityNode
{
  DMN_LNX_EntityNode *next;
  DMN_LNX_Entity *v;
};

typedef struct DMN_LNX_State DMN_LNX_State;
struct DMN_LNX_State
{
  Arena *arena;
  
  // rjf: access locking mechanism
  OS_Handle access_mutex;
  B32 access_run_state;
  
  // rjf: deferred events
  Arena *deferred_events_arena;
  DMN_EventList deferred_events;
  
  // rjf: entity storage
  Arena *entities_arena;
  DMN_LNX_Entity *entities_base;
  U64 entities_count;
  DMN_LNX_Entity *free_entity;
};

read_only global DMN_LNX_Entity dmn_lnx_nil_entity = {&dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity};
global DMN_LNX_State *dmn_lnx_state = 0;
thread_static B32 dmn_lnx_ctrl_thread = 0;

////////////////////////////////
//~ rjf: Helpers

//- rjf: file descriptor memory reading/writing helpers
internal U64 dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst);
internal B32 dmn_lnx_write(int memory_fd, Rng1U64 range, void *src);
#define dmn_lnx_read_struct(fd, vaddr, ptr) dmn_lnx_read((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
#define dmn_lnx_write_struct(fd, vaddr, ptr) dmn_lnx_write((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
internal String8 dmn_lnx_read_string(Arena *arena, int memory_fd, U64 base_vaddr);

//- rjf: pid => info extraction
internal String8 dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid);
internal Arch dmn_lnx_arch_from_pid(pid_t pid);
internal DMN_LNX_ProcessAux dmn_lnx_aux_from_pid(pid_t pid, Arch arch);

//- rjf: phdr info extraction
internal DMN_LNX_PhdrInfo dmn_lnx_phdr_info_from_memory(int memory_fd, B32 is_32bit, U64 phvaddr, U64 phsize, U64 phcount);

//- rjf: process entity => info extraction
internal DMN_LNX_ModuleInfoList dmn_lnx_module_info_list_from_process(Arena *arena, DMN_LNX_Entity *process);

////////////////////////////////
//~ rjf: Entity Functions

internal DMN_LNX_Entity *dmn_lnx_entity_alloc(DMN_LNX_Entity *parent, DMN_LNX_EntityKind kind);
internal void dmn_lnx_entity_release(DMN_LNX_Entity *entity);
internal DMN_Handle dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity);
internal DMN_LNX_Entity *dmn_lnx_entity_from_handle(DMN_Handle handle);

#endif // DEMON_CORE_LINUX_H
