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

////////////////////////////////
//~ rjf: Register Layouts
//
// These are defined in <sys/user.h>, but only for one architecture at a time

#pragma pack(push, 1)

typedef struct DMN_LNX_UserRegsX64 DMN_LNX_UserRegsX64;
struct DMN_LNX_UserRegsX64
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

typedef struct DMN_LNX_XSaveLegacy DMN_LNX_XSaveLegacy;
struct DMN_LNX_XSaveLegacy
{
  U16 fcw;
  U16 fsw;
  U16 ftw;
  U16 fop;
  union
  {
    struct
    {
      U64 fip;
      U64 fdp;
    } b64;
    struct
    {
      U32 fip;
      U16 fcs, _pad0;
      U32 fdp;
      U16 fds, _pad1;
    } b32;
  };
  U32 mxcsr;
  U32 mxcsr_mask;
  U128 st_space;
  U256 xmm_space;
  U8 padding[96];
};

typedef struct DMN_LNX_XSaveHeader DMN_LNX_XSaveHeader;
struct DMN_LNX_XSaveHeader
{
  U64 xstate_bv;
  U64 xcomp_bv;
  U8 reserved[48];
};

// TODO(rjf):
//
// this one is hacked; ymmh is not gauranteed to be at a fixed location
// and there can be more after that. Requires CPUID to be totally compliant to the standard.
// See intel's manual on the xsave format for more info.
//
typedef struct DMN_LNX_XSave DMN_LNX_XSave;
struct DMN_LNX_XSave
{
  DMN_LNX_XSaveLegacy legacy;
  DMN_LNX_XSaveHeader header;
  U8 ymmh[256];
};

typedef struct DMN_LNX_UserX64 DMN_LNX_UserX64;
struct DMN_LNX_UserX64
{
  DMN_LNX_UserRegsX64 regs;
  S32 u_fpvalid, _pad0;
  DMN_LNX_XSaveLegacy i387;
  U64 u_tsize, u_dsize, u_ssize, start_code, start_stack;
  U64 signal;
  S32 reserved, _pad1;
  U64 u_ar0, u_fpstate;
  U64 magic;
  U8  u_comm[32];
  U64 u_debugreg[8];
};

typedef struct DMN_LNX_UserRegsX86 DMN_LNX_UserRegsX86;
struct DMN_LNX_UserRegsX86
{
  U32 ebx;
	U32 ecx;
	U32 edx;
	U32 esi;
	U32 edi;
	U32 ebp;
	U32 eax;
	U32 ds;
	U32 es;
	U32 fs;
	U32 gs;
	U32 orig_eax;
	U32 eip;
	U32 cs;
	U32 eflags;
	U32 sp;
	U32 ss;
};

// NOTE(rjf): (32-Bit Protected Mode Format)
typedef struct DMN_LNX_FSave DMN_LNX_FSave;
struct DMN_LNX_FSave
{
  // control registers
  U16 fcw;
  U16 _pad0;
  U16 fsw;
  U16 _pad1;
  U16 ftw;
  U16 _pad2;
  U32 fip;
  U16 fips;
  U16 fop;
  U32 fdp;
  U16 fds;
  U16 _pad3;
  
  // data registers
  U8 st[80];
};

typedef struct DMN_LNX_UserX86 DMN_LNX_UserX86;
struct DMN_LNX_UserX86
{
  DMN_LNX_UserRegsX86 regs;
  S32 u_fpvalid;
  DMN_LNX_FSave i387;
  U32 u_tsize, u_dsize, u_ssize, start_code, start_stack;
  S32 signal, reserved;
  U32 u_ar0, u_fpstate;
  U32 magic;
  U8  u_comm[32];
  U32 u_debugreg[8];
};

#pragma pack(pop)

////////////////////////////////
//~ rjf: Process Info Extraction Types

typedef struct DMN_LNX_ProcessAux DMN_LNX_ProcessAux;
struct DMN_LNX_ProcessAux
{
  B32 filled;
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
  int fd;
  B32 expecting_dummy_sigstop;
};

typedef struct DMN_LNX_EntityNode DMN_LNX_EntityNode;
struct DMN_LNX_EntityNode
{
  DMN_LNX_EntityNode *next;
  DMN_LNX_Entity *v;
};

////////////////////////////////
//~ rjf: Main State Bundle

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
  
  // rjf: halting mechanism
  B32 has_halt_injection;
  U64 halt_code;
  U64 halt_user_data;
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
internal DMN_LNX_Entity *dmn_lnx_thread_from_pid(pid_t pid);
internal B32 dmn_lnx_thread_read_reg_block(DMN_LNX_Entity *thread, void *reg_block);
internal B32 dmn_lnx_thread_write_reg_block(DMN_LNX_Entity *thread, void *reg_block);

#endif // DEMON_CORE_LINUX_H
