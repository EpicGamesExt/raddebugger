// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_LINUX_H
#define DEMON_CORE_LINUX_H

////////////////////////////////
//~ rjf: Includes

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <elf.h>
#include <dirent.h>
#include <errno.h>

////////////////////////////////
//~ rjf: ptrace options

#define DMN_LNX_PTRACE_OPTIONS (PTRACE_O_TRACEEXIT|\
PTRACE_O_EXITKILL|\
PTRACE_O_TRACEFORK|\
PTRACE_O_TRACEVFORK|\
PTRACE_O_TRACECLONE)

enum
{
  DMN_LNX_SigTrapCode_Brkpt  = 1,
  DMN_LNX_SigTrapCode_Trace  = 2,
  DMN_LNX_SigTrapCode_Branch = 3,
  DMN_LNX_SigTrapCode_HwBkpt = 4,
  DMN_LNX_SigTrapCode_Unk    = 5,
  DMN_LNX_SigTrapCode_Perf   = 6
} DMN_LNX_SigTrapCode;

////////////////////////////////
//~ rjf: Register Layouts
//
// These are defined in <sys/user.h>, but only for one architecture at a time

typedef struct DMN_LNX_UserX64 DMN_LNX_UserX64;
struct DMN_LNX_UserX64
{
  struct
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
  } regs;
  S32 u_fpvalid, _pad0;
  X64_FXSave i387;
  U64 u_tsize, u_dsize, u_ssize, start_code, start_stack;
  U64 signal;
  S32 reserved, _pad1;
  U64 u_ar0, u_fpstate;
  U64 magic;
  U8  u_comm[32];
  U64 u_debugreg[8];
};
StaticAssert(sizeof(DMN_LNX_UserX64) == 912, g_dmn_lnx_user_x64_size_check);

////////////////////////////////
//~ SDT Probes

typedef struct DMN_LNX_Probe DMN_LNX_Probe;
struct DMN_LNX_Probe
{
  String8       provider;
  String8       name;
  String8       args_string;
  STAP_ArgArray args;
  U64           pc;
  U64           semaphore;
};

typedef struct DMN_LNX_ProbeNode DMN_LNX_ProbeNode;
struct DMN_LNX_ProbeNode
{
  DMN_LNX_Probe v;
  DMN_LNX_ProbeNode *next;
};

typedef struct DMN_LNX_ProbeList DMN_LNX_ProbeList;
struct DMN_LNX_ProbeList
{
  U64                count;
  DMN_LNX_ProbeNode *first;
  DMN_LNX_ProbeNode *last;
};

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
//~ rjf: Process Info Extraction Types

typedef struct DMN_LNX_ProcessAuxv DMN_LNX_ProcessAuxv;
struct DMN_LNX_ProcessAuxv
{
  U64 base;
  U64 phnum;
  U64 phent;
  U64 phdr;
  U64 execfn;
  U64 pagesz;
};

typedef struct DMN_LNX_PhdrInfo DMN_LNX_PhdrInfo;
struct DMN_LNX_PhdrInfo
{
  Rng1U64 range;
  U64 dynamic;
};

typedef struct DMN_LNX_DynamicInfo DMN_LNX_DynamicInfo;
struct DMN_LNX_DynamicInfo
{
  U64 hash_vaddr;
  U64 gnu_hash_vaddr;
  U64 strtab_vaddr;
  U64 strtab_size;
  U64 symtab_vaddr;
  U64 symtab_entry_size;
};

////////////////////////////////
//~ rjf: Entity Types

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

  // process
  Arena *arena;
  int fd;
  pid_t tracer_tid;
  B32 expect_rdebug_data_breakpoint;
  U64 rdebug_vaddr;
  U64 rdebug_brk_vaddr;
  ELF_Class dl_class;
  HashTable *loaded_modules_ht;
  DMN_LNX_Probe **probes;
  U64 probe_vaddrs[DMN_LNX_ProbeType_Count];

  // process x64
  U64 xcr0;
  U64 xsave_size;
  X64_XSaveLayout xsave_layout;

  // thread
  B32 expecting_dummy_sigstop;
  void *reg_block;

  // module
  U64 base_vaddr;
  U64 phvaddr;
  U64 phentsize;
  U64 phcount;
  B8  is_live;
};

typedef struct DMN_LNX_EntityNode DMN_LNX_EntityNode;
struct DMN_LNX_EntityNode
{
  DMN_LNX_EntityNode *next;
  DMN_LNX_Entity *v;
};

typedef struct DMN_LNX_EntityList DMN_LNX_EntityList;
struct DMN_LNX_EntityList
{
  U64 count;
  DMN_LNX_EntityNode *first;
  DMN_LNX_EntityNode *last;
};

////////////////////////////////
//~ rjf: Main State Bundle

typedef struct DMN_LNX_State DMN_LNX_State;
struct DMN_LNX_State
{
  Arena *arena;
  
  // rjf: access locking mechanism
  Mutex access_mutex;
  B32 access_run_state;
  
  // rjf: deferred events
  Arena *deferred_events_arena;
  DMN_EventList deferred_events;
  
  // rjf: entity storage
  Arena *entities_arena;
  DMN_LNX_Entity *entities_base;
  U64 entities_count;
  DMN_LNX_Entity *free_entity;
  
  // rjf: halting mechanism
  B32 has_halt_injection;
  U64 halt_code;
  U64 halt_user_data;

  DMN_EventKind last_event_kind;
  pid_t last_stop_pid;
  int last_sig_code;
};

read_only global DMN_LNX_Entity dmn_lnx_nil_entity = {&dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity, &dmn_lnx_nil_entity};
global DMN_LNX_State *dmn_lnx_state = 0;
thread_static B32 dmn_lnx_ctrl_thread = 0;

////////////////////////////////
//~ rjf: Helpers

internal DMN_LNX_EntityNode * dmn_lnx_entity_list_push(Arena *arena, DMN_LNX_EntityList *list, DMN_LNX_Entity *v);

//- rjf: file descriptor memory reading/writing helpers
internal U64 dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst);
internal B32 dmn_lnx_write(int memory_fd, Rng1U64 range, void *src);
#define dmn_lnx_read_struct(fd, vaddr, ptr) dmn_lnx_read((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
#define dmn_lnx_write_struct(fd, vaddr, ptr) dmn_lnx_write((fd), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
internal String8 dmn_lnx_read_string_capped(Arena *arena, int memory_fd, U64 base_vaddr, U64 cap_size);
internal String8 dmn_lnx_read_string(Arena *arena, int memory_fd, U64 base_vaddr);

////////////////////////////////
//~ Runtime Struct Helpers

internal B32 dmn_lnx_read_ehdr(int memory_fd, U64 addr, ELF_Hdr64 *ehdr_out);
internal B32 dmn_lnx_read_phdr(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Phdr64 *phdr_out);
internal B32 dmn_lnx_read_shdr(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Shdr64 *shdr_out);
internal B32 dmn_lnx_read_linkmap(int memory_fd, U64 addr, ELF_Class elf_class, GNU_LinkMap64 *link_map_out);
internal B32 dmn_lnx_read_dynamic(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Dyn64 *dyn_out);
internal B32 dmn_lnx_read_symbol(int memory_fd, U64 addr, ELF_Class elf_class, ELF_Sym64 *symbol_out);
internal B32 dmn_lnx_read_r_debug(int memory_fd, U64 addr, Arch arch, GNU_RDebugInfo64 *rdebug_out);

//- rjf: pid => info extraction
internal String8             dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid);
internal ELF_Hdr64           dmn_lnx_ehdr_from_pid(pid_t pid);
internal DMN_LNX_ProcessAuxv dmn_lnx_auxv_from_pid(pid_t pid, ELF_Class elf_class);

//- ELF/GNU info from memory
internal DMN_LNX_PhdrInfo       dmn_lnx_phdr_info_from_memory(int memory_fd, ELF_Class elf_class, U64 rebase, U64 e_phaddr, U64 e_phentsize, U64 e_phnum);
internal DMN_LNX_DynamicInfo    dmn_lnx_dynamic_info_from_memory(int memory_fd, ELF_Class elf_Class, U64 rebase, U64 dynamic_vaddr);
internal U64                    dmn_lnx_rdebug_vaddr_from_memory(int memory_fd, U64 loader_vaddr);

////////////////////////////////
//~ rjf: Entity Functions

internal DMN_LNX_Entity *dmn_lnx_entity_alloc(DMN_LNX_Entity *parent, DMN_LNX_EntityKind kind);
internal void            dmn_lnx_entity_release(DMN_LNX_Entity *entity);
internal DMN_Handle      dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity);
internal DMN_LNX_Entity *dmn_lnx_entity_from_handle(DMN_Handle handle);
internal DMN_LNX_Entity *dmn_lnx_thread_from_pid(pid_t pid);

internal U64 dmn_lnx_thread_read_ip(DMN_LNX_Entity *thread);
internal U64 dmn_lnx_thread_read_sp(DMN_LNX_Entity *thread);
internal B32 dmn_lnx_thread_write_ip(DMN_LNX_Entity *thread, U64 ip);
internal B32 dmn_lnx_thread_write_sp(DMN_LNX_Entity *thread, U64 sp);
internal B32 dmn_lnx_thread_read_reg_block(DMN_LNX_Entity *thread, void *reg_block);
internal B32 dmn_lnx_thread_write_reg_block(DMN_LNX_Entity *thread, void *reg_block);

////////////////////////////////

internal B32 dmn_lnx_set_single_step_flag(DMN_LNX_Entity *thread, B32 is_on);

#endif // DEMON_CORE_LINUX_H
