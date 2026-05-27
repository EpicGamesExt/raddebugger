// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_DEMON_H
#define LINUX_DEMON_H

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
//~ TLS

typedef struct LNX_DMN_DbDesc
{
  U32 bit_size;
  U32 count;
  U32 offset;
} LNX_DMN_DbDesc;

////////////////////////////////
//~ SDT Probes

typedef struct LNX_DMN_Probe
{
  String8       provider;
  String8       name;
  String8       args_string;
  STAP_ArgArray args;
  U64           pc;
  U64           semaphore;
} LNX_DMN_Probe;

typedef struct LNX_DMN_ProbeNode
{
  LNX_DMN_Probe v;
  struct LNX_DMN_ProbeNode *next;
} LNX_DMN_ProbeNode;

typedef struct LNX_DMN_ProbeList
{
  U64                count;
  LNX_DMN_ProbeNode *first;
  LNX_DMN_ProbeNode *last;
} LNX_DMN_ProbeList;

#define LNX_DMN_Probe_XList             \
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
  LNX_DMN_ProbeType_Null,
#define X(_N,...) LNX_DMN_ProbeType_##_N,
  LNX_DMN_Probe_XList
#undef X
  LNX_DMN_ProbeType_Count,
} LNX_DMN_ProbeType;

////////////////////////////////
//~ Process Info

typedef struct LNX_DMN_Auxv
{
  U64 base;
  U64 phnum;
  U64 phent;
  U64 phdr;
  U64 execfn;
  U64 pagesz;
} LNX_DMN_Auxv;

typedef struct LNX_DMN_DynamicInfo
{
  U64 hash_vaddr;
  U64 gnu_hash_vaddr;
  U64 strtab_vaddr;
  U64 strtab_size;
  U64 symtab_vaddr;
  U64 symtab_entry_size;
} LNX_DMN_DynamicInfo;

////////////////////////////////
//~ Entities

typedef enum LNX_DMN_EntityKind
{
  LNX_DMN_EntityKind_Null,
  LNX_DMN_EntityKind_Process,
  LNX_DMN_EntityKind_ProcessCtx,
  LNX_DMN_EntityKind_Thread,
  LNX_DMN_EntityKind_Module,
} LNX_DMN_EntityKind;

typedef enum LNX_DMN_ThreadState
{
  LNX_DMN_ThreadState_Null,
  LNX_DMN_ThreadState_Running,
  LNX_DMN_ThreadState_Stopped,
  LNX_DMN_ThreadState_Exited,
  LNX_DMN_ThreadState_PendingCreation,
} LNX_DMN_ThreadState;

typedef struct LNX_DMN_Thread
{
  pid_t                   tid;
  LNX_DMN_ThreadState     state;
  struct LNX_DMN_Process *process;
  void                   *reg_block;
  B32                     is_reg_block_dirty;
  B32                     pass_through_signal;
  U64                     pass_through_signo;
  U64                     orig_rax;
  
  struct LNX_DMN_Thread *next;
  struct LNX_DMN_Thread *prev;
} LNX_DMN_Thread;

typedef struct LNX_DMN_ThreadPtrNode
{
  LNX_DMN_Thread *v;
  struct LNX_DMN_ThreadPtrNode *next;
  struct LNX_DMN_ThreadPtrNode *prev;
} LNX_DMN_ThreadPtrNode;

typedef struct LNX_DMN_ThreadPtrList
{
  U64                    count;
  LNX_DMN_ThreadPtrNode *first;
  LNX_DMN_ThreadPtrNode *last;
} LNX_DMN_ThreadPtrList;

typedef struct LNX_DMN_Module
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
  
  struct LNX_DMN_Module *next;
  struct LNX_DMN_Module *prev;
} LNX_DMN_Module;

typedef struct LNX_DMN_ModulePtrNode
{
  LNX_DMN_Module *v;
  struct LNX_DMN_ModulePtrNode *next;
} LNX_DMN_ModulePtrNode;

typedef struct LNX_DMN_ModulePtrList
{
  U64                    count;
  LNX_DMN_ModulePtrNode *first;
  LNX_DMN_ModulePtrNode *last;
} LNX_DMN_ModulePtrList;

typedef enum
{
  LNX_DMN_ProcessState_Null,
  LNX_DMN_ProcessState_Launch,
  LNX_DMN_ProcessState_Attach,
  LNX_DMN_ProcessState_WaitForExec,
  LNX_DMN_ProcessState_ExecFailedDoExit,
  LNX_DMN_ProcessState_Normal,
} LNX_DMN_ProcessState;

typedef enum
{
  LNX_DMN_CreateProcessFlag_DebugSubprocesses = (1 << 0),
  LNX_DMN_CreateProcessFlag_Rebased           = (1 << 1),
  LNX_DMN_CreateProcessFlag_Cow               = (1 << 2),
  LNX_DMN_CreateProcessFlag_ClonedMemory      = (1 << 3),
} LNX_DMN_CreateProcessFlags;

typedef struct LNX_DMN_Process
{
  pid_t                      pid;
  int                        fd;
  LNX_DMN_ProcessState       state;
  B32                        debug_subprocesses;
  B32                        is_cow;
  B32                        vfork_with_spoof;
  U64                        thread_count;
  LNX_DMN_Thread            *first_thread;
  LNX_DMN_Thread            *last_thread;
  U64                        main_thread_exit_code;
  struct LNX_DMN_Process    *parent_process;
  struct LNX_DMN_ProcessCtx *ctx;
  
  
  struct LNX_DMN_Process *next;
  struct LNX_DMN_Process *prev;
} LNX_DMN_Process;

typedef struct LNX_DMN_ProcessPtrNode
{
  LNX_DMN_Process *v;
  struct LNX_DMN_ProcessPtrNode *next;
} LNX_DMN_ProcessPtrNode;

typedef struct LNX_DMN_ProcessPtrList
{
  U64                     count;
  LNX_DMN_ProcessPtrNode *first;
  LNX_DMN_ProcessPtrNode *last;
} LNX_DMN_ProcessPtrList;

typedef struct LNX_DMN_ActiveTrap LNX_DMN_ActiveTrap;
struct LNX_DMN_ActiveTrap
{
  LNX_DMN_ActiveTrap *next;
  B32 good;
  DMN_Trap *trap;
  String8 swap_bytes;
};

typedef struct LNX_DMN_ProcessCtx
{
  Arena                 *arena;
  Arch                   arch;
  U64                    rdebug_vaddr;
  ELF_Class              dl_class;
  HashTable             *loaded_modules_ht;
  LNX_DMN_Probe        **probes;
  LNX_DMN_ActiveTrap        *first_probe_trap;
  LNX_DMN_ActiveTrap        *last_probe_trap;
  LNX_DMN_Module        *first_module;
  LNX_DMN_Module        *last_module;
  U64                    module_count;
  U64                    ref_count;
  
  String8List free_reg_blocks;
  String8List free_reg_block_nodes;
  
  // x64
  U64             xcr0;
  U64             xsave_size;
  X64_XSaveLayout xsave_layout;
} LNX_DMN_ProcessCtx;

typedef struct LNX_DMN_Entity
{
  union
  {
    LNX_DMN_Process    process;
    LNX_DMN_ProcessCtx process_ctx;
    LNX_DMN_Thread     thread;
    LNX_DMN_Module     module;
    struct
    {
      struct LNX_DMN_Entity *next;
    };
  };
  U32                gen;
  LNX_DMN_EntityKind kind;
} LNX_DMN_Entity;

typedef struct LNX_DMN_EntityNode
{
  LNX_DMN_Entity *v;
  struct LNX_DMN_EntityNode *next;
} LNX_DMN_EntityNode;

typedef struct LNX_DMN_EntityList
{
  U64                 count;
  LNX_DMN_EntityNode *first;
  LNX_DMN_EntityNode *last;
} LNX_DMN_EntityList;

////////////////////////////////
//~ Global State

typedef struct LNX_DMN_State
{
  Arena *arena;
  
  // rjf: access locking mechanism
  Mutex access_mutex;
  B32   access_run_state;
  
  // rjf: entity storage
  Arena          *entities_arena;
  LNX_DMN_Entity *entities_base;
  LNX_DMN_Entity *free_entity;
  U64             entities_count;
  
  HashTable *tid_ht; // thread id -> thread entity
  HashTable *pid_ht; // process id -> process entity
  
  // process tracking
  U64              process_count;
  LNX_DMN_Process *first_process;
  LNX_DMN_Process *last_process;
  
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
  LNX_DMN_DbDesc tls_modid_desc;
  LNX_DMN_DbDesc tls_offset_desc;
} LNX_DMN_State;

////////////////////////////////
//~ Globals

global B32            lnx_dmn_ctrl_thread;
global LNX_DMN_State *lnx_dmn_state;

////////////////////////////////
//~ Memory R/W

internal U64     lnx_dmn_read(int memory_fd, Rng1U64 range, void *dst);
internal B32     lnx_dmn_write(int memory_fd, Rng1U64 range, void *src);
internal String8 lnx_dmn_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size);
internal String8 lnx_dmn_read_string(Arena *arena, int memory_fd, U64 base_vaddr);
#define lnx_dmn_read_struct(fd, vaddr, ptr)  lnx_dmn_read((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
#define lnx_dmn_write_struct(fd, vaddr, ptr) lnx_dmn_write((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))

////////////////////////////////
//~ rjf: Trap Setting

internal LNX_DMN_ActiveTrap *lnx_dmn_set_trap(Arena *arena, DMN_Trap *trap);

////////////////////////////////
//~ ELF/GNU info

internal Rng1U64             lnx_dmn_compute_image_vrange(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum);
internal LNX_DMN_DynamicInfo lnx_dmn_dynamic_info_from_memory(int memory_fd, ELF_Class elf_Class, U64 rebase, U64 dynamic_vaddr);
internal U64                 lnx_dmn_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vaddr, B32 is_rebased);

////////////////////////////////
//~ Process Info

internal String8           lnx_dmn_exe_path_from_pid(Arena *arena, pid_t pid);
internal String8           lnx_dmn_dl_path_from_pid(Arena *arena, pid_t pid, U64 auxv_base);
internal ELF_Hdr64         lnx_dmn_ehdr_from_pid(pid_t pid);
internal LNX_DMN_Auxv      lnx_dmn_auxv_from_pid(pid_t pid, ELF_Class elf_class);
internal LNX_DMN_Thread *  lnx_dmn_thread_from_pid(pid_t pid);
internal LNX_DMN_Process * lnx_dmn_process_from_pid(pid_t pid);

////////////////////////////////
//~ rjf: Module Info Parsing

internal DMN_ModuleInfo *lnx_dmn_module_info_from_process_module(Arena *arena, pid_t pid, int memory_fd, U64 base_vaddr, U64 link_map_vaddr, B32 is_main);

////////////////////////////////
//~ Entity

// alloc
internal LNX_DMN_Entity *     lnx_dmn_entity_alloc(LNX_DMN_EntityKind kind);
internal LNX_DMN_Process *    lnx_dmn_process_alloc(pid_t pid, LNX_DMN_ProcessState state, LNX_DMN_Process *parent_process, B32 debug_subprocess, B32 is_cow);
internal LNX_DMN_ProcessCtx * lnx_dmn_process_ctx_alloc(LNX_DMN_Process *process, B32 is_rebased);
internal LNX_DMN_Thread *     lnx_dmn_thread_alloc(LNX_DMN_Process *process, LNX_DMN_ThreadState thread_state, pid_t tid);
internal LNX_DMN_Module *     lnx_dmn_module_alloc(LNX_DMN_ProcessCtx *ctx, int memory_fd, U64 base_vaddr, U64 name_vaddr, U64 name_space_id, B32 is_main);

// release
internal void lnx_dmn_entity_release(LNX_DMN_Entity *entity);
internal void lnx_dmn_process_release(LNX_DMN_Process *process);
internal void lnx_dmn_process_ctx_release(LNX_DMN_ProcessCtx *process_ctx);
internal void lnx_dmn_thread_release(LNX_DMN_Thread *thread);
internal void lnx_dmn_module_release(LNX_DMN_ProcessCtx *ctx, LNX_DMN_Module *module);

// clone
internal LNX_DMN_ProcessCtx * lnx_dmn_process_ctx_clone(LNX_DMN_Process *process, LNX_DMN_ProcessCtx *ctx);
internal LNX_DMN_Module *     lnx_dmn_module_clone(LNX_DMN_ProcessCtx *process_ctx, LNX_DMN_Module *module);

// entity -> handle
internal DMN_Handle lnx_dmn_handle_from_entity(LNX_DMN_Entity *entity);
internal DMN_Handle lnx_dmn_handle_from_process(LNX_DMN_Process *process);
internal DMN_Handle lnx_dmn_handle_from_process_ctx(LNX_DMN_ProcessCtx *process_ctx);
internal DMN_Handle lnx_dmn_handle_from_thread(LNX_DMN_Thread *thread);
internal DMN_Handle lnx_dmn_handle_from_module(LNX_DMN_Module *module);

// handle -> entity
internal LNX_DMN_Entity *     lnx_dmn_entity_from_handle(DMN_Handle handle, LNX_DMN_EntityKind expected_kind);
internal LNX_DMN_Process *    lnx_dmn_process_from_handle(DMN_Handle process_handle);
internal LNX_DMN_ProcessCtx * lnx_dmn_process_ctx_from_handle(DMN_Handle process_ctx_handle);
internal LNX_DMN_Thread *     lnx_dmn_thread_from_handle(DMN_Handle thread_handle);
internal LNX_DMN_Module *     lnx_dmn_module_from_handle(DMN_Handle module_handle);

////////////////////////////////
//~ Process Helpers

internal void lnx_dmn_process_trap_probes(LNX_DMN_Process *process);
internal void lnx_dmn_process_untrap_probes(LNX_DMN_Process *process);

////////////////////////////////
//~ Thread Helpers

internal U64  lnx_dmn_thread_read_ip(LNX_DMN_Thread *thread);
internal U64  lnx_dmn_thread_read_sp(LNX_DMN_Thread *thread);
internal void lnx_dmn_thread_write_ip(LNX_DMN_Thread *thread, U64 ip);
internal void lnx_dmn_thread_write_sp(LNX_DMN_Thread *thread, U64 sp);
internal B32  lnx_dmn_thread_read_reg_block(LNX_DMN_Thread *thread);
internal B32  lnx_dmn_thread_write_reg_block(LNX_DMN_Thread *thread);
internal B32  lnx_dmn_set_single_step_flag(LNX_DMN_Thread *thread, B32 is_on);

////////////////////////////////
//~ List Helpers

internal void                     lnx_dmn_thread_ptr_list_push_node(LNX_DMN_ThreadPtrList *list, LNX_DMN_ThreadPtrNode *n);
internal LNX_DMN_ThreadPtrNode *  lnx_dmn_thread_ptr_list_push(Arena *arena, LNX_DMN_ThreadPtrList *list, LNX_DMN_Thread *v);
internal LNX_DMN_ModulePtrNode *  lnx_dmn_module_ptr_list_push(Arena *arena, LNX_DMN_ModulePtrList *list, LNX_DMN_Module *v);
internal LNX_DMN_ProcessPtrNode * lnx_dmn_process_ptr_list_push(Arena *arena, LNX_DMN_ProcessPtrList *list, LNX_DMN_Process *v);

////////////////////////////////
//~ Debug Event Pushers

internal void lnx_dmn_push_event_create_process(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process);
internal void lnx_dmn_push_event_exit_process(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process);
internal void lnx_dmn_push_event_create_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread);
internal void lnx_dmn_push_event_exit_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 exit_code);
internal void lnx_dmn_push_event_load_module(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, LNX_DMN_Module *module);
internal void lnx_dmn_push_event_unload_module(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, LNX_DMN_Module *module);
internal void lnx_dmn_push_event_handshake_complete(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process);
internal void lnx_dmn_push_event_breakpoint(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 address);
internal void lnx_dmn_push_event_single_step(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread);
internal void lnx_dmn_push_event_exception(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 signo);
internal void lnx_dmn_push_event_halt(Arena *arena, DMN_EventList *events);
internal void lnx_dmn_push_event_not_attached(Arena *arena, DMN_EventList *events);

////////////////////////////////
//~ Debug Event

internal LNX_DMN_Thread *  lnx_dmn_event_create_thread(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, pid_t tid);
internal void              lnx_dmn_event_exit_thread(Arena *arena, DMN_EventList *events, pid_t tid, U64 exit_code);
internal LNX_DMN_Process * lnx_dmn_event_create_process(Arena *arena, DMN_EventList *events, pid_t pid, LNX_DMN_Process *parent_process, LNX_DMN_CreateProcessFlags flags);
internal void              lnx_dmn_event_exit_process(Arena *arena, DMN_EventList *events, pid_t pid);
internal void              lnx_dmn_event_load_module(Arena *arena, DMN_EventList *events, LNX_DMN_Thread *thread, U64 name_space_id, U64 new_link_map_vaddr);
internal void              lnx_dmn_event_unload_module(Arena *arena, DMN_EventList *events, LNX_DMN_Process *process, U64 rdebug_vaddr);
internal void              lnx_dmn_event_breakpoint(Arena *arena, DMN_EventList *events, LNX_DMN_ActiveTrap *user_traps, pid_t tid);
internal void              lnx_dmn_event_data_breakpoint(Arena *arena, DMN_EventList *events, pid_t tid);
internal void              lnx_dmn_event_halt(Arena *arena, DMN_EventList *events);
internal void              lnx_dmn_event_single_step(Arena *arena, DMN_EventList *events, pid_t tid);
internal void              lnx_dmn_event_exception(Arena *arena, DMN_EventList *events, pid_t tid, U64 signo);
internal LNX_DMN_Process * lnx_dmn_event_attach(Arena *arena, DMN_EventList *events, pid_t pid);

#endif // LINUX_DEMON_H
