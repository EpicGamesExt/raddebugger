// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_DEMON_H
#define LINUX_DEMON_H

////////////////////////////////
//~ rjf: Linux Includes

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <elf.h>

////////////////////////////////
//~ rjf: Generated Code

#include "generated/linux_demon.meta.h"

////////////////////////////////
//~ rjf: Register Layouts
//
// These are defined in <sys/user.h>, but only for one architecture at a time

typedef struct LNX_DMN_GprsX64 LNX_DMN_GprsX64;
struct LNX_DMN_GprsX64
{
  U64 r15;
  U64 r14;
  U64 r13;
  U64 r12;
  U64 rbp;
  U64 rbx;
  U64 r11;
  U64 r10;
  U64 r9;
  U64 r8;
  U64 rax;
  U64 rcx;
  U64 rdx;
  U64 rsi;
  U64 rdi;
  U64 orig_rax;
  U64 rip;
  U64 cs;
  U64 rflags;
  U64 rsp;
  U64 ss;
  U64 fsbase;
  U64 gsbase;
  U64 ds;
  U64 es;
  U64 fs;
  U64 gs;
};

typedef struct LNX_DMN_UserX64 LNX_DMN_UserX64;
struct LNX_DMN_UserX64
{
  LNX_DMN_GprsX64 regs;
  S32 u_fpvalid;
  U32 _pad0;
  X64_FXSave i387;
  U64 u_tsize;
  U64 u_dsize;
  U64 u_ssize;
  U64 start_code;
  U64 start_stack;
  U64 signal;
  U32 reserved;
  U32 _pad1;
  U64 u_ar0;
  U64 u_fpstate;
  U64 magic;
  U8  u_comm[32];
  U64 u_debugreg[8];
};

StaticAssert(sizeof(LNX_DMN_UserX64) == 912, lnx_user_x64_size_check);

////////////////////////////////
//~ rjf: TLS Descriptions

typedef struct LNX_DMN_DbDesc LNX_DMN_DbDesc;
struct LNX_DMN_DbDesc
{
  U32 bit_size;
  U32 count;
  U32 offset;
};

////////////////////////////////
//~ rjf: SDT (Statically-Defined Tracing) Probes

typedef struct LNX_DMN_Probe LNX_DMN_Probe;
struct LNX_DMN_Probe
{
  String8 provider;
  String8 name;
  String8 args_string;
  STAP_ArgArray args;
  U64 pc;
  U64 semaphore;
};

typedef struct LNX_DMN_ProbeNode LNX_DMN_ProbeNode;
struct LNX_DMN_ProbeNode
{
  LNX_DMN_ProbeNode *next;
  LNX_DMN_Probe v;
};

typedef struct LNX_DMN_ProbeList LNX_DMN_ProbeList;
struct LNX_DMN_ProbeList
{
  LNX_DMN_ProbeNode *first;
  LNX_DMN_ProbeNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Process Info

typedef struct LNX_DMN_Auxv LNX_DMN_Auxv;
struct LNX_DMN_Auxv
{
  U64 base;
  U64 phnum;
  U64 phent;
  U64 phdr;
  U64 execfn;
  U64 pagesz;
};

typedef struct LNX_DMN_DynamicInfo LNX_DMN_DynamicInfo;
struct LNX_DMN_DynamicInfo
{
  U64 hash_vaddr;
  U64 gnu_hash_vaddr;
  U64 strtab_vaddr;
  U64 strtab_size;
  U64 symtab_vaddr;
  U64 symtab_entry_size;
};

////////////////////////////////
//~ rjf: Entities

typedef enum LNX_DMN_EntityKind
{
  LNX_DMN_EntityKind_Null,
  LNX_DMN_EntityKind_Process,
  LNX_DMN_EntityKind_ProcessCtx,
  LNX_DMN_EntityKind_Thread,
  LNX_DMN_EntityKind_Module,
}
LNX_DMN_EntityKind;

typedef enum LNX_DMN_ThreadState
{
  LNX_DMN_ThreadState_Null,
  LNX_DMN_ThreadState_Running,
  LNX_DMN_ThreadState_Stopped,
  LNX_DMN_ThreadState_Exited,
  LNX_DMN_ThreadState_PendingCreation,
}
LNX_DMN_ThreadState;

typedef struct LNX_DMN_Thread LNX_DMN_Thread;
struct LNX_DMN_Thread
{
  LNX_DMN_Thread *next;
  LNX_DMN_Thread *prev;
  LNX_DMN_Thread *tid_next;
  LNX_DMN_Thread *tid_prev;
  pid_t tid;
  LNX_DMN_ThreadState state;
  struct LNX_DMN_Process *process;
  void *reg_block;
  B32 is_reg_block_dirty;
  B32 pass_through_signal;
  U64 pass_through_signo;
  U64 orig_rax;
  U64 dtv_base_vaddr;
};

typedef struct LNX_DMN_ThreadSlot LNX_DMN_ThreadSlot;
struct LNX_DMN_ThreadSlot
{
  LNX_DMN_Thread *first;
  LNX_DMN_Thread *last;
};

typedef struct LNX_DMN_Module LNX_DMN_Module;
struct LNX_DMN_Module
{
  LNX_DMN_Module *order_next;
  LNX_DMN_Module *order_prev;
  LNX_DMN_Module *hash_next;
  LNX_DMN_Module *hash_prev;
  U64 name_vaddr;
  U64 base_vaddr;
  U64 name_space_id;
  U64 size;
  U64 tls_index;
  U64 tls_offset;
  B8 is_live;
  B8 is_main;
};

typedef struct LNX_DMN_ModuleSlot LNX_DMN_ModuleSlot;
struct LNX_DMN_ModuleSlot
{
  LNX_DMN_Module *first;
  LNX_DMN_Module *last;
};

typedef enum LNX_DMN_ProcessState
{
  LNX_DMN_ProcessState_Null,
  LNX_DMN_ProcessState_Launch,
  LNX_DMN_ProcessState_Attach,
  LNX_DMN_ProcessState_WaitForExec,
  LNX_DMN_ProcessState_ExecFailedDoExit,
  LNX_DMN_ProcessState_Normal,
}
LNX_DMN_ProcessState;

typedef enum LNX_DMN_CreateProcessFlags
{
  LNX_DMN_CreateProcessFlag_DebugSubprocesses = (1 << 0),
  LNX_DMN_CreateProcessFlag_Rebased           = (1 << 1),
  LNX_DMN_CreateProcessFlag_Cow               = (1 << 2),
  LNX_DMN_CreateProcessFlag_ClonedMemory      = (1 << 3),
}
LNX_DMN_CreateProcessFlags;

typedef struct LNX_DMN_Process LNX_DMN_Process;
struct LNX_DMN_Process
{
  LNX_DMN_Process *next;
  LNX_DMN_Process *prev;
  LNX_DMN_Process *pid_next;
  LNX_DMN_Process *pid_prev;
  LNX_DMN_Process *parent_process;
  struct LNX_DMN_ProcessCtx *ctx;
  pid_t pid;
  int fd;
  LNX_DMN_ProcessState state;
  B32 debug_subprocesses;
  B32 is_cow;
  B32 vfork_with_spoof;
  U64 thread_count;
  LNX_DMN_Thread *first_thread;
  LNX_DMN_Thread *last_thread;
  U64 main_thread_exit_code;
};

typedef struct LNX_DMN_ProcessSlot LNX_DMN_ProcessSlot;
struct LNX_DMN_ProcessSlot
{
  LNX_DMN_Process *first;
  LNX_DMN_Process *last;
};

typedef struct LNX_DMN_ProcessPtrNode LNX_DMN_ProcessPtrNode;
struct LNX_DMN_ProcessPtrNode
{
  LNX_DMN_ProcessPtrNode *next;
  LNX_DMN_Process *v;
};

typedef struct LNX_DMN_ProcessPtrList LNX_DMN_ProcessPtrList;
struct LNX_DMN_ProcessPtrList
{
  LNX_DMN_ProcessPtrNode *first;
  LNX_DMN_ProcessPtrNode *last;
  U64 count;
};

typedef struct LNX_DMN_ActiveTrap LNX_DMN_ActiveTrap;
struct LNX_DMN_ActiveTrap
{
  LNX_DMN_ActiveTrap *next;
  B32 good;
  DMN_Trap *trap;
  String8 swap_bytes;
};

typedef struct LNX_DMN_ProcessCtx LNX_DMN_ProcessCtx;
struct LNX_DMN_ProcessCtx
{
  Arena *arena;
  Arch arch;
  U64 rdebug_vaddr;
  ELF_Class dl_class;
  LNX_DMN_Probe **probes;
  LNX_DMN_ActiveTrap *first_probe_trap;
  LNX_DMN_ActiveTrap *last_probe_trap;
  LNX_DMN_Module *first_module;
  LNX_DMN_Module *last_module;
  U64 module_count;
  U64 module_slots_count;
  LNX_DMN_ModuleSlot *module_slots;
  U64 ref_count;
  String8List free_reg_blocks;
  String8List free_reg_block_nodes;
  
  // x64
  U64 xcr0;
  U64 xsave_size;
  X64_XSaveLayout xsave_layout;
};

typedef struct LNX_DMN_Entity LNX_DMN_Entity;
struct LNX_DMN_Entity
{
  union
  {
    LNX_DMN_Process process;
    LNX_DMN_ProcessCtx process_ctx;
    LNX_DMN_Thread thread;
    LNX_DMN_Module module;
    LNX_DMN_Entity *next;
  };
  U32 gen;
  LNX_DMN_EntityKind kind;
};

typedef struct LNX_DMN_EntityNode LNX_DMN_EntityNode;
struct LNX_DMN_EntityNode
{
  LNX_DMN_EntityNode *next;
  LNX_DMN_Entity *v;
};

typedef struct LNX_DMN_EntityList LNX_DMN_EntityList;
struct LNX_DMN_EntityList
{
  LNX_DMN_EntityNode *first;
  LNX_DMN_EntityNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Top-Level State Bundle

typedef struct LNX_DMN_State LNX_DMN_State;
struct LNX_DMN_State
{
  Arena *arena;
  
  // rjf: access locking mechanism
  Mutex access_mutex;
  B32 access_run_state;
  
  // rjf: main entity storage
  Arena *entities_arena;
  LNX_DMN_Entity *entities_base;
  LNX_DMN_Entity *free_entity;
  U64 entities_count;
  
  // rjf: id -> entity tables
  LNX_DMN_ProcessSlot *process_from_pid_slots;
  U64 process_from_pid_slots_count;
  LNX_DMN_ThreadSlot *thread_from_tid_slots;
  U64 thread_from_tid_slots_count;
  
  // rjf: process entity list
  LNX_DMN_Process *first_process;
  LNX_DMN_Process *last_process;
  U64 process_count;
  
  // rjf: pending process/thread counts
  U64 process_pending_creation;
  U64 threads_pending_creation;
  
  // rjf: halting state
  Mutex halter_mutex;
  pid_t halter_tid;
  U64 halt_code;
  U64 halt_user_data;
  B32 is_halting;
  
  // rjf: TLS
  B32 is_tls_detected;
  LNX_DMN_DbDesc tls_modid_desc;
  LNX_DMN_DbDesc tls_offset_desc;
};

////////////////////////////////
//~ rjf: Globals

global LNX_DMN_State *lnx_dmn_state = 0;
thread_static B32 lnx_dmn_ctrl_thread = 0;

////////////////////////////////
//~ rjf: Memory R/W Helpers

internal U64 lnx_dmn_read(int memory_fd, Rng1U64 range, void *dst);
internal B32 lnx_dmn_write(int memory_fd, Rng1U64 range, void *src);
internal String8 lnx_dmn_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size);
internal String8 lnx_dmn_read_string(Arena *arena, int memory_fd, U64 base_vaddr);
#define lnx_dmn_read_struct(fd, vaddr, ptr)  lnx_dmn_read((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
#define lnx_dmn_write_struct(fd, vaddr, ptr) lnx_dmn_write((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))

////////////////////////////////
//~ rjf: Module Info Parsing

internal DMN_ModuleInfo *lnx_dmn_module_info_from_process_module(Arena *arena, pid_t pid, int memory_fd, U64 base_vaddr, U64 link_map_vaddr, B32 is_main);

////////////////////////////////
//~ rjf: Trap Setting

internal LNX_DMN_ActiveTrap *lnx_dmn_set_trap(Arena *arena, DMN_Trap *trap);

////////////////////////////////
//~ rjf: ELF/GNU Parsing

internal Rng1U64 lnx_dmn_compute_image_vrange(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum);

////////////////////////////////
//~ rjf: Entity Functions

//- rjf: base allocation / deallocation
internal LNX_DMN_Entity *lnx_dmn_entity_alloc(LNX_DMN_EntityKind kind);
internal void lnx_dmn_entity_release(LNX_DMN_Entity *entity);

//- rjf: specialized allocation / deallocation helpers
internal LNX_DMN_Process *lnx_dmn_process_alloc(pid_t pid, LNX_DMN_ProcessState state, LNX_DMN_Process *parent_process, B32 debug_subprocess, B32 is_cow);
internal LNX_DMN_Module *lnx_dmn_module_alloc(LNX_DMN_ProcessCtx *ctx, int memory_fd, U64 base_vaddr, U64 name_vaddr, U64 name_space_id, B32 is_main);
internal void lnx_dmn_module_release(LNX_DMN_ProcessCtx *ctx, LNX_DMN_Module *module);

//- rjf: context cloning
internal LNX_DMN_ProcessCtx * lnx_dmn_process_ctx_clone(LNX_DMN_Process *process, LNX_DMN_ProcessCtx *ctx);

//- rjf: entity <-> handle
internal DMN_Handle lnx_dmn_handle_from_entity(LNX_DMN_Entity *entity);
internal DMN_Handle lnx_dmn_handle_from_process(LNX_DMN_Process *process);
internal DMN_Handle lnx_dmn_handle_from_thread(LNX_DMN_Thread *thread);
internal DMN_Handle lnx_dmn_handle_from_module(LNX_DMN_Module *module);
internal LNX_DMN_Entity *lnx_dmn_entity_from_handle(DMN_Handle handle, LNX_DMN_EntityKind expected_kind);
internal LNX_DMN_Process *lnx_dmn_process_from_handle(DMN_Handle process_handle);
internal LNX_DMN_Thread *lnx_dmn_thread_from_handle(DMN_Handle thread_handle);
internal LNX_DMN_Module *lnx_dmn_module_from_handle(DMN_Handle module_handle);

//- rjf: entity <-> pid
internal LNX_DMN_Thread *lnx_dmn_thread_from_pid(pid_t pid);
internal LNX_DMN_Process *lnx_dmn_process_from_pid(pid_t pid);

////////////////////////////////
//~ rjf: Thread Helpers

internal U64  lnx_dmn_ip_from_thread(LNX_DMN_Thread *thread);
internal void lnx_dmn_thread_write_ip(LNX_DMN_Thread *thread, U64 ip);
internal B32  lnx_dmn_thread_read_reg_block(LNX_DMN_Thread *thread);
internal B32  lnx_dmn_thread_write_reg_block(LNX_DMN_Thread *thread);
internal B32  lnx_dmn_set_single_step_flag(LNX_DMN_Thread *thread, B32 is_on);
internal U64  lnx_dmn_tls_root_vaddr_from_reg_block(int fd, Arch arch, void *reg_block);

////////////////////////////////
//~ Debug Event Pushers

internal void lnx_dmn_push_event_load_module(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, LNX_DMN_Module *module);
internal void lnx_dmn_push_event_unload_module(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, LNX_DMN_Module *module);

#endif // LINUX_DEMON_H
