// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_LINUX_H
#define DEMON_CORE_LINUX_H

////////////////////////////////
//~ TLS

typedef struct DMN_LNX_DbDesc
{
  U32 bit_size;
  U32 count;
  U32 offset;
} DMN_LNX_DbDesc;

////////////////////////////////
//~ SDT Probes

typedef struct DMN_LNX_Probe
{
  String8       provider;
  String8       name;
  String8       args_string;
  STAP_ArgArray args;
  U64           pc;
  U64           semaphore;
} DMN_LNX_Probe;

typedef struct DMN_LNX_ProbeNode
{
  DMN_LNX_Probe v;
  struct DMN_LNX_ProbeNode *next;
} DMN_LNX_ProbeNode;

typedef struct DMN_LNX_ProbeList
{
  U64                count;
  DMN_LNX_ProbeNode *first;
  DMN_LNX_ProbeNode *last;
} DMN_LNX_ProbeList;

#define DMN_LNX_Probe_XList             \
  X(InitStart,     2, "init_start")     \
  X(InitComplete,  2, "init_complete")  \
  X(RelocStart,    2, "reloc_start")    \
  X(RelocComplete, 3, "reloc_complete") \
  X(MapStart,      2, "map_start")      \
  X(MapComplete,   3, "map_complete")   \
  X(UnmapStart,    2, "unmap_start")    \
  X(UnmapComplete, 2, "unmap_complete") \
  X(LongJmp,       3, "longjmp")        \
  X(LongJmpTarget, 3, "longjmp_target") \
  X(SetJmp,        3, "setjmp")

typedef enum
{
  DMN_LNX_ProbeType_Null,
#define X(_N,...) DMN_LNX_ProbeType_##_N,
  DMN_LNX_Probe_XList
#undef X
  DMN_LNX_ProbeType_Count,
} DMN_LNX_ProbeType;

////////////////////////////////
//~ Process Info

typedef struct DMN_LNX_Auxv
{
  U64 base;
  U64 phnum;
  U64 phent;
  U64 phdr;
  U64 execfn;
  U64 pagesz;
} DMN_LNX_Auxv;

typedef struct DMN_LNX_DynamicInfo
{
  U64 hash_vaddr;
  U64 gnu_hash_vaddr;
  U64 strtab_vaddr;
  U64 strtab_size;
  U64 symtab_vaddr;
  U64 symtab_entry_size;
} DMN_LNX_DynamicInfo;

////////////////////////////////
//~ Entities

typedef enum DMN_LNX_EntityKind
{
  DMN_LNX_EntityKind_Null,
  DMN_LNX_EntityKind_Process,
  DMN_LNX_EntityKind_ProcessCtx,
  DMN_LNX_EntityKind_Thread,
  DMN_LNX_EntityKind_Module,
} DMN_LNX_EntityKind;

typedef enum DMN_LNX_ThreadState
{
  DMN_LNX_ThreadState_Null,
  DMN_LNX_ThreadState_Running,
  DMN_LNX_ThreadState_Stopped,
  DMN_LNX_ThreadState_Exited,
  DMN_LNX_ThreadState_PendingCreation,
} DMN_LNX_ThreadState;

typedef struct DMN_LNX_Thread
{
  pid_t                   tid;
  DMN_LNX_ThreadState     state;
  struct DMN_LNX_Process *process;
  void                   *reg_block;
  B32                     is_reg_block_dirty;
  B32                     pass_through_signal;
  U64                     pass_through_signo;
  U64                     orig_rax;

  struct DMN_LNX_Thread *next;
  struct DMN_LNX_Thread *prev;
} DMN_LNX_Thread;

typedef struct DMN_LNX_ThreadPtrNode
{
  DMN_LNX_Thread *v;
  struct DMN_LNX_ThreadPtrNode *next;
  struct DMN_LNX_ThreadPtrNode *prev;
} DMN_LNX_ThreadPtrNode;

typedef struct DMN_LNX_ThreadPtrList
{
  U64                    count;
  DMN_LNX_ThreadPtrNode *first;
  DMN_LNX_ThreadPtrNode *last;
} DMN_LNX_ThreadPtrList;

typedef struct DMN_LNX_Module
{
  U64 name_vaddr;
  U64 base_vaddr;
  U64 name_space_id;
  U64 size;
  U64 phvaddr;
  U64 phentsize;
  U64 phcount;
  U64 tls_index;
  U64 tls_offset;
  B8  is_live;
  B8  is_main;

  struct DMN_LNX_Module *next;
  struct DMN_LNX_Module *prev;
} DMN_LNX_Module;

typedef struct DMN_LNX_ModulePtrNode
{
  DMN_LNX_Module *v;
  struct DMN_LNX_ModulePtrNode *next;
} DMN_LNX_ModulePtrNode;

typedef struct DMN_LNX_ModulePtrList
{
  U64                    count;
  DMN_LNX_ModulePtrNode *first;
  DMN_LNX_ModulePtrNode *last;
} DMN_LNX_ModulePtrList;

typedef enum
{
  DMN_LNX_ProcessState_Null,
  DMN_LNX_ProcessState_Launch,
  DMN_LNX_ProcessState_Attach,
  DMN_LNX_ProcessState_WaitForExec,
  DMN_LNX_ProcessState_ExecFailedDoExit,
  DMN_LNX_ProcessState_Normal,
} DMN_LNX_ProcessState;

typedef enum
{
  DMN_LNX_CreateProcessFlag_DebugSubprocesses = (1 << 0),
  DMN_LNX_CreateProcessFlag_Rebased           = (1 << 1),
  DMN_LNX_CreateProcessFlag_Cow               = (1 << 2),
  DMN_LNX_CreateProcessFlag_ClonedMemory      = (1 << 3),
} DMN_LNX_CreateProcessFlags;

typedef struct DMN_LNX_Process
{
  pid_t                      pid;
  int                        fd;
  DMN_LNX_ProcessState       state;
  B32                        debug_subprocesses;
  B32                        is_cow;
  B32                        vfork_with_spoof;
  U64                        thread_count;
  DMN_LNX_Thread            *first_thread;
  DMN_LNX_Thread            *last_thread;
  U64                        main_thread_exit_code;
  struct DMN_LNX_Process    *parent_process;
  struct DMN_LNX_ProcessCtx *ctx;


  struct DMN_LNX_Process *next;
  struct DMN_LNX_Process *prev;
} DMN_LNX_Process;

typedef struct DMN_LNX_ProcessPtrNode
{
  DMN_LNX_Process *v;
  struct DMN_LNX_ProcessPtrNode *next;
} DMN_LNX_ProcessPtrNode;

typedef struct DMN_LNX_ProcessPtrList
{
  U64                     count;
  DMN_LNX_ProcessPtrNode *first;
  DMN_LNX_ProcessPtrNode *last;
} DMN_LNX_ProcessPtrList;

typedef struct DMN_LNX_ProcessCtx
{
  Arena                 *arena;
  Arch                   arch;
  U64                    rdebug_vaddr;
  ELF_Class              dl_class;
  HashTable             *loaded_modules_ht;
  DMN_LNX_Probe        **probes;
  DMN_ActiveTrap        *first_probe_trap;
  DMN_ActiveTrap        *last_probe_trap;
  DMN_LNX_Module        *first_module;
  DMN_LNX_Module        *last_module;
  U64                    module_count;
  U64                    ref_count;

  String8List free_reg_blocks;
  String8List free_reg_block_nodes;

  // x64
  U64             xcr0;
  U64             xsave_size;
  X64_XSaveLayout xsave_layout;
} DMN_LNX_ProcessCtx;

typedef struct DMN_LNX_Entity
{
  union
  {
    DMN_LNX_Process    process;
    DMN_LNX_ProcessCtx process_ctx;
    DMN_LNX_Thread     thread;
    DMN_LNX_Module     module;
    struct
    {
      struct DMN_LNX_Entity *next;
    };
  };
  U32                gen;
  DMN_LNX_EntityKind kind;
} DMN_LNX_Entity;

typedef struct DMN_LNX_EntityNode
{
  DMN_LNX_Entity *v;
  struct DMN_LNX_EntityNode *next;
} DMN_LNX_EntityNode;

typedef struct DMN_LNX_EntityList
{
  U64                 count;
  DMN_LNX_EntityNode *first;
  DMN_LNX_EntityNode *last;
} DMN_LNX_EntityList;

////////////////////////////////
//~ Global State

typedef struct DMN_LNX_State
{
  Arena *arena;

  // rjf: access locking mechanism
  Mutex access_mutex;
  B32   access_run_state;

  // rjf: entity storage
  Arena          *entities_arena;
  DMN_LNX_Entity *entities_base;
  DMN_LNX_Entity *free_entity;
  U64             entities_count;

  HashTable *tid_ht; // thread id -> thread entity
  HashTable *pid_ht; // process id -> process entity

  // process tracking
  U64              process_count;
  DMN_LNX_Process *first_process;
  DMN_LNX_Process *last_process;

  // process/thread creation tracking
  U64 process_pending_creation;
  U64 threads_pending_creation;

  // halter
  Mutex halter_mutex;
  pid_t halter_tid;
  U64   halt_code;
  U64   halt_user_data;
  B32   is_halting;

  // TLS
  B32            is_tls_detected;
  DMN_LNX_DbDesc tls_modid_desc;
  DMN_LNX_DbDesc tls_offset_desc;
} DMN_LNX_State;

////////////////////////////////
//~ Globals

global B32            dmn_lnx_ctrl_thread;
global DMN_LNX_State *dmn_lnx_state;

////////////////////////////////
//~ Memory R/W

internal U64     dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst);
internal B32     dmn_lnx_write(int memory_fd, Rng1U64 range, void *src);
internal String8 dmn_lnx_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size);
internal String8 dmn_lnx_read_string(Arena *arena, int memory_fd, U64 base_vaddr);
#define dmn_lnx_read_struct(fd, vaddr, ptr)  dmn_lnx_read((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
#define dmn_lnx_write_struct(fd, vaddr, ptr) dmn_lnx_write((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))

////////////////////////////////
//~ ELF/GNU info

internal Rng1U64             dmn_lnx_compute_image_vrange(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum);
internal DMN_LNX_DynamicInfo dmn_lnx_dynamic_info_from_memory(int memory_fd, ELF_Class elf_Class, U64 rebase, U64 dynamic_vaddr);
internal U64                 dmn_lnx_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vaddr, B32 is_rebased);

////////////////////////////////
//~ Process Info

internal String8           dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid);
internal String8           dmn_lnx_dl_path_from_pid(Arena *arena, pid_t pid, U64 auxv_base);
internal ELF_Hdr64         dmn_lnx_ehdr_from_pid(pid_t pid);
internal DMN_LNX_Auxv      dmn_lnx_auxv_from_pid(pid_t pid, ELF_Class elf_class);
internal DMN_LNX_Thread *  dmn_lnx_thread_from_pid(pid_t pid);
internal DMN_LNX_Process * dmn_lnx_process_from_pid(pid_t pid);

////////////////////////////////
//~ Entity

// alloc
internal DMN_LNX_Entity *     dmn_lnx_entity_alloc(DMN_LNX_EntityKind kind);
internal DMN_LNX_Process *    dmn_lnx_process_alloc(pid_t pid, DMN_LNX_ProcessState state, DMN_LNX_Process *parent_process, B32 debug_subprocess, B32 is_cow);
internal DMN_LNX_ProcessCtx * dmn_lnx_process_ctx_alloc(DMN_LNX_Process *process, B32 is_rebased);
internal DMN_LNX_Thread *     dmn_lnx_thread_alloc(DMN_LNX_Process *process, DMN_LNX_ThreadState thread_state, pid_t tid);
internal DMN_LNX_Module *     dmn_lnx_module_alloc(DMN_LNX_ProcessCtx *ctx, int memory_fd, U64 base_vaddr, U64 name_vaddr, U64 name_space_id, B32 is_main);

// release
internal void dmn_lnx_entity_release(DMN_LNX_Entity *entity);
internal void dmn_lnx_process_release(DMN_LNX_Process *process);
internal void dmn_lnx_process_ctx_release(DMN_LNX_ProcessCtx *process_ctx);
internal void dmn_lnx_thread_release(DMN_LNX_Thread *thread);
internal void dmn_lnx_module_release(DMN_LNX_ProcessCtx *ctx, DMN_LNX_Module *module);

// clone
internal DMN_LNX_ProcessCtx * dmn_lnx_process_ctx_clone(DMN_LNX_Process *process, DMN_LNX_ProcessCtx *ctx);
internal DMN_LNX_Module *     dmn_lnx_module_clone(DMN_LNX_ProcessCtx *process_ctx, DMN_LNX_Module *module);

// entity -> handle
internal DMN_Handle dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity);
internal DMN_Handle dmn_lnx_handle_from_process(DMN_LNX_Process *process);
internal DMN_Handle dmn_lnx_handle_from_process_ctx(DMN_LNX_ProcessCtx *process_ctx);
internal DMN_Handle dmn_lnx_handle_from_thread(DMN_LNX_Thread *thread);
internal DMN_Handle dmn_lnx_handle_from_module(DMN_LNX_Module *module);

// handle -> entity
internal DMN_LNX_Entity *     dmn_lnx_entity_from_handle(DMN_Handle handle, DMN_LNX_EntityKind expected_kind);
internal DMN_LNX_Process *    dmn_lnx_process_from_handle(DMN_Handle process_handle);
internal DMN_LNX_ProcessCtx * dmn_lnx_process_ctx_from_handle(DMN_Handle process_ctx_handle);
internal DMN_LNX_Thread *     dmn_lnx_thread_from_handle(DMN_Handle thread_handle);
internal DMN_LNX_Module *     dmn_lnx_module_from_handle(DMN_Handle module_handle);

////////////////////////////////
//~ Process Helpers

internal void dmn_lnx_process_trap_probes(DMN_LNX_Process *process);
internal void dmn_lnx_process_untrap_probes(DMN_LNX_Process *process);

////////////////////////////////
//~ Thread Helpers

internal U64  dmn_lnx_thread_read_ip(DMN_LNX_Thread *thread);
internal U64  dmn_lnx_thread_read_sp(DMN_LNX_Thread *thread);
internal void dmn_lnx_thread_write_ip(DMN_LNX_Thread *thread, U64 ip);
internal void dmn_lnx_thread_write_sp(DMN_LNX_Thread *thread, U64 sp);
internal B32  dmn_lnx_thread_read_reg_block(DMN_LNX_Thread *thread);
internal B32  dmn_lnx_thread_write_reg_block(DMN_LNX_Thread *thread);
internal B32  dmn_lnx_set_single_step_flag(DMN_LNX_Thread *thread, B32 is_on);

////////////////////////////////
//~ List Helpers

internal void                     dmn_lnx_thread_ptr_list_push_node(DMN_LNX_ThreadPtrList *list, DMN_LNX_ThreadPtrNode *n);
internal DMN_LNX_ThreadPtrNode *  dmn_lnx_thread_ptr_list_push(Arena *arena, DMN_LNX_ThreadPtrList *list, DMN_LNX_Thread *v);
internal DMN_LNX_ModulePtrNode *  dmn_lnx_module_ptr_list_push(Arena *arena, DMN_LNX_ModulePtrList *list, DMN_LNX_Module *v);
internal DMN_LNX_ProcessPtrNode * dmn_lnx_process_ptr_list_push(Arena *arena, DMN_LNX_ProcessPtrList *list, DMN_LNX_Process *v);

////////////////////////////////
//~ Debug Event Pushers

internal void dmn_lnx_push_event_create_process(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process);
internal void dmn_lnx_push_event_exit_process(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process);
internal void dmn_lnx_push_event_create_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread);
internal void dmn_lnx_push_event_exit_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 exit_code);
internal void dmn_lnx_push_event_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, DMN_LNX_Module *module);
internal void dmn_lnx_push_event_unload_module(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, DMN_LNX_Module *module);
internal void dmn_lnx_push_event_handshake_complete(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process);
internal void dmn_lnx_push_event_breakpoint(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 address);
internal void dmn_lnx_push_event_single_step(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread);
internal void dmn_lnx_push_event_exception(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 signo);
internal void dmn_lnx_push_event_halt(Arena *arena, DMN_EventList *events);
internal void dmn_lnx_push_event_not_attached(Arena *arena, DMN_EventList *events);

////////////////////////////////
//~ Debug Event

internal DMN_LNX_Thread *  dmn_lnx_event_create_thread(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, pid_t tid);
internal void              dmn_lnx_event_exit_thread(Arena *arena, DMN_EventList *events, pid_t tid, U64 exit_code);
internal DMN_LNX_Process * dmn_lnx_event_create_process(Arena *arena, DMN_EventList *events, pid_t pid, DMN_LNX_Process *parent_process, DMN_LNX_CreateProcessFlags flags);
internal void              dmn_lnx_event_exit_process(Arena *arena, DMN_EventList *events, pid_t pid);
internal void              dmn_lnx_event_load_module(Arena *arena, DMN_EventList *events, DMN_LNX_Thread *thread, U64 name_space_id, U64 new_link_map_vaddr);
internal void              dmn_lnx_event_unload_module(Arena *arena, DMN_EventList *events, DMN_LNX_Process *process, U64 rdebug_vaddr);
internal void              dmn_lnx_event_breakpoint(Arena *arena, DMN_EventList *events, DMN_ActiveTrap *user_traps, pid_t tid);
internal void              dmn_lnx_event_data_breakpoint(Arena *arena, DMN_EventList *events, pid_t tid);
internal void              dmn_lnx_event_halt(Arena *arena, DMN_EventList *events);
internal void              dmn_lnx_event_single_step(Arena *arena, DMN_EventList *events, pid_t tid);
internal void              dmn_lnx_event_exception(Arena *arena, DMN_EventList *events, pid_t tid, U64 signo);
internal DMN_LNX_Process * dmn_lnx_event_attach(Arena *arena, DMN_EventList *events, pid_t pid);

#endif // DEMON_CORE_LINUX_H

