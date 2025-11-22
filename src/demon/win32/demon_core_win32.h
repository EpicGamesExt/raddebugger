// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_CORE_WIN32_H
#define DEMON_CORE_WIN32_H

////////////////////////////////
//~ rjf: Windows Includes

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

////////////////////////////////
//~ rjf: Win32 Exception Codes

#define DMN_W32_EXCEPTION_BREAKPOINT                     0x80000003u
#define DMN_W32_EXCEPTION_SINGLE_STEP                    0x80000004u
#define DMN_W32_EXCEPTION_LONG_JUMP                      0x80000026u
#define DMN_W32_EXCEPTION_ACCESS_VIOLATION               0xC0000005u
#define DMN_W32_EXCEPTION_ARRAY_BOUNDS_EXCEEDED          0xC000008Cu
#define DMN_W32_EXCEPTION_DATA_TYPE_MISALIGNMENT         0x80000002u
#define DMN_W32_EXCEPTION_GUARD_PAGE_VIOLATION           0x80000001u
#define DMN_W32_EXCEPTION_FLT_DENORMAL_OPERAND           0xC000008Du
#define DMN_W32_EXCEPTION_FLT_DEVIDE_BY_ZERO             0xC000008Eu
#define DMN_W32_EXCEPTION_FLT_INEXACT_RESULT             0xC000008Fu
#define DMN_W32_EXCEPTION_FLT_INVALID_OPERATION          0xC0000090u
#define DMN_W32_EXCEPTION_FLT_OVERFLOW                   0xC0000091u
#define DMN_W32_EXCEPTION_FLT_STACK_CHECK                0xC0000092u
#define DMN_W32_EXCEPTION_FLT_UNDERFLOW                  0xC0000093u
#define DMN_W32_EXCEPTION_INT_DIVIDE_BY_ZERO             0xC0000094u
#define DMN_W32_EXCEPTION_INT_OVERFLOW                   0xC0000095u
#define DMN_W32_EXCEPTION_PRIVILEGED_INSTRUCTION         0xC0000096u
#define DMN_W32_EXCEPTION_ILLEGAL_INSTRUCTION            0xC000001Du
#define DMN_W32_EXCEPTION_IN_PAGE_ERROR                  0xC0000006u
#define DMN_W32_EXCEPTION_INVALID_DISPOSITION            0xC0000026u
#define DMN_W32_EXCEPTION_NONCONTINUABLE                 0xC0000025u
#define DMN_W32_EXCEPTION_STACK_OVERFLOW                 0xC00000FDu
#define DMN_W32_EXCEPTION_INVALID_HANDLE                 0xC0000008u
#define DMN_W32_EXCEPTION_UNWIND_CONSOLIDATE             0x80000029u
#define DMN_W32_EXCEPTION_DLL_NOT_FOUND                  0xC0000135u
#define DMN_W32_EXCEPTION_ORDINAL_NOT_FOUND              0xC0000138u
#define DMN_W32_EXCEPTION_ENTRY_POINT_NOT_FOUND          0xC0000139u
#define DMN_W32_EXCEPTION_DLL_INIT_FAILED                0xC0000142u
#define DMN_W32_EXCEPTION_CONTROL_C_EXIT                 0xC000013Au
#define DMN_W32_EXCEPTION_FLT_MULTIPLE_FAULTS            0xC00002B4u
#define DMN_W32_EXCEPTION_FLT_MULTIPLE_TRAPS             0xC00002B5u
#define DMN_W32_EXCEPTION_NAT_CONSUMPTION                0xC00002C9u
#define DMN_W32_EXCEPTION_HEAP_CORRUPTION                0xC0000374u
#define DMN_W32_EXCEPTION_STACK_BUFFER_OVERRUN           0xC0000409u
#define DMN_W32_EXCEPTION_INVALID_CRUNTIME_PARAM         0xC0000417u
#define DMN_W32_EXCEPTION_ASSERT_FAILURE                 0xC0000420u
#define DMN_W32_EXCEPTION_NO_MEMORY                      0xC0000017u
#define DMN_W32_EXCEPTION_THROW                          0xE06D7363u
#define DMN_W32_EXCEPTION_SET_THREAD_NAME                0x406d1388u
#define DMN_w32_EXCEPTION_CLRDBG_NOTIFICATION            0x04242420u
#define DMN_w32_EXCEPTION_CLR                            0xE0434352u
#define DMN_W32_EXCEPTION_RADDBG_SET_THREAD_COLOR        0x00524144u
#define DMN_W32_EXCEPTION_RADDBG_SET_BREAKPOINT          0x00524145u
#define DMN_W32_EXCEPTION_RADDBG_SET_VADDR_RANGE_NOTE    0x00524156u

////////////////////////////////
//~ rjf: Win32 Exception ExceptionInformation Codes
//
// used as a subcode, apparently in all cases, for DMN_W32_EXCEPTION_STACK_BUFFER_OVERRUN.
// need to somehow pipe this through & interpret it correctly in outer layers... @fastfail

#define DMN_W32_FAST_FAIL_LEGACY_GS_VIOLATION               0
#define DMN_W32_FAST_FAIL_VTGUARD_CHECK_FAILURE             1
#define DMN_W32_FAST_FAIL_STACK_COOKIE_CHECK_FAILURE        2
#define DMN_W32_FAST_FAIL_CORRUPT_LIST_ENTRY                3
#define DMN_W32_FAST_FAIL_INCORRECT_STACK                   4
#define DMN_W32_FAST_FAIL_INVALID_ARG                       5
#define DMN_W32_FAST_FAIL_GS_COOKIE_INIT                    6
#define DMN_W32_FAST_FAIL_FATAL_APP_EXIT                    7
#define DMN_W32_FAST_FAIL_RANGE_CHECK_FAILURE               8
#define DMN_W32_FAST_FAIL_UNSAFE_REGISTRY_ACCESS            9
#define DMN_W32_FAST_FAIL_GUARD_ICALL_CHECK_FAILURE         10
#define DMN_W32_FAST_FAIL_GUARD_WRITE_CHECK_FAILURE         11
#define DMN_W32_FAST_FAIL_INVALID_FIBER_SWITCH              12
#define DMN_W32_FAST_FAIL_INVALID_SET_OF_CONTEXT            13
#define DMN_W32_FAST_FAIL_INVALID_REFERENCE_COUNT           14
#define DMN_W32_FAST_FAIL_INVALID_JUMP_BUFFER               18
#define DMN_W32_FAST_FAIL_MRDATA_MODIFIED                   19
#define DMN_W32_FAST_FAIL_CERTIFICATION_FAILURE             20
#define DMN_W32_FAST_FAIL_INVALID_EXCEPTION_CHAIN           21
#define DMN_W32_FAST_FAIL_CRYPTO_LIBRARY                    22
#define DMN_W32_FAST_FAIL_INVALID_CALL_IN_DLL_CALLOUT       23
#define DMN_W32_FAST_FAIL_INVALID_IMAGE_BASE                24
#define DMN_W32_FAST_FAIL_DLOAD_PROTECTION_FAILURE          25
#define DMN_W32_FAST_FAIL_UNSAFE_EXTENSION_CALL             26
#define DMN_W32_FAST_FAIL_DEPRECATED_SERVICE_INVOKED        27
#define DMN_W32_FAST_FAIL_INVALID_BUFFER_ACCESS             28
#define DMN_W32_FAST_FAIL_INVALID_BALANCED_TREE             29
#define DMN_W32_FAST_FAIL_INVALID_NEXT_THREAD               30
#define DMN_W32_FAST_FAIL_GUARD_ICALL_CHECK_SUPPRESSED      31         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_APCS_DISABLED                     32
#define DMN_W32_FAST_FAIL_INVALID_IDLE_STATE                33
#define DMN_W32_FAST_FAIL_MRDATA_PROTECTION_FAILURE         34
#define DMN_W32_FAST_FAIL_UNEXPECTED_HEAP_EXCEPTION         35
#define DMN_W32_FAST_FAIL_INVALID_LOCK_STATE                36
#define DMN_W32_FAST_FAIL_GUARD_JUMPTABLE                   37         // Known to compiler, must retain value 37
#define DMN_W32_FAST_FAIL_INVALID_LONGJUMP_TARGET           38
#define DMN_W32_FAST_FAIL_INVALID_DISPATCH_CONTEXT          39
#define DMN_W32_FAST_FAIL_INVALID_THREAD                    40
#define DMN_W32_FAST_FAIL_INVALID_SYSCALL_NUMBER            41         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_INVALID_FILE_OPERATION            42         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_LPAC_ACCESS_DENIED                43         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_GUARD_SS_FAILURE                  44
#define DMN_W32_FAST_FAIL_LOADER_CONTINUITY_FAILURE         45         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_GUARD_EXPORT_SUPPRESSION_FAILURE  46
#define DMN_W32_FAST_FAIL_INVALID_CONTROL_STACK             47
#define DMN_W32_FAST_FAIL_SET_CONTEXT_DENIED                48
#define DMN_W32_FAST_FAIL_INVALID_IAT                       49
#define DMN_W32_FAST_FAIL_HEAP_METADATA_CORRUPTION          50
#define DMN_W32_FAST_FAIL_PAYLOAD_RESTRICTION_VIOLATION     51
#define DMN_W32_FAST_FAIL_LOW_LABEL_ACCESS_DENIED           52         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_ENCLAVE_CALL_FAILURE              53
#define DMN_W32_FAST_FAIL_UNHANDLED_LSS_EXCEPTON            54
#define DMN_W32_FAST_FAIL_ADMINLESS_ACCESS_DENIED           55         // Telemetry, nonfatal
#define DMN_W32_FAST_FAIL_UNEXPECTED_CALL                   56
#define DMN_W32_FAST_FAIL_CONTROL_INVALID_RETURN_ADDRESS    57
#define DMN_W32_FAST_FAIL_UNEXPECTED_HOST_BEHAVIOR          58
#define DMN_W32_FAST_FAIL_FLAGS_CORRUPTION                  59
#define DMN_W32_FAST_FAIL_VEH_CORRUPTION                    60
#define DMN_W32_FAST_FAIL_ETW_CORRUPTION                    61
#define DMN_W32_FAST_FAIL_RIO_ABORT                         62
#define DMN_W32_FAST_FAIL_INVALID_PFN                       63
#define DMN_W32_FAST_FAIL_GUARD_ICALL_CHECK_FAILURE_XFG     64
#define DMN_W32_FAST_FAIL_CAST_GUARD                        65         // Known to compiler, must retain value 65
#define DMN_W32_FAST_FAIL_HOST_VISIBILITY_CHANGE            66
#define DMN_W32_FAST_FAIL_KERNEL_CET_SHADOW_STACK_ASSIST    67
#define DMN_W32_FAST_FAIL_PATCH_CALLBACK_FAILED             68
#define DMN_W32_FAST_FAIL_NTDLL_PATCH_FAILED                69
#define DMN_W32_FAST_FAIL_INVALID_FLS_DATA                  70
#define DMN_W32_FAST_FAIL_INVALID_FAST_FAIL_CODE            0xFFFFFFFF

////////////////////////////////
//~ rjf: Win32 Register Codes

#define DMN_W32_CTX_X86       0x00010000
#define DMN_W32_CTX_X64       0x00100000

#define DMN_W32_CTX_INTEL_CONTROL       0x0001    // segss, rsp, segcs, rip, and rflags
#define DMN_W32_CTX_INTEL_INTEGER       0x0002    // rax, rcx, rdx, rbx, rbp, rsi, rdi, and r8-r15
#define DMN_W32_CTX_INTEL_SEGMENTS      0x0004    // segds, seges, segfs, and seggs
#define DMN_W32_CTX_INTEL_FLOATS        0x0008    // xmm0-xmm15
#define DMN_W32_CTX_INTEL_DEBUG         0x0010    // dr0-dr3 and dr6-dr7
#define DMN_W32_CTX_INTEL_EXTENDED      0x0020
#define DMN_W32_CTX_INTEL_XSTATE        0x0040

#define DMN_W32_CTX_X86_ALL (DMN_W32_CTX_X86 | \
DMN_W32_CTX_INTEL_CONTROL | DMN_W32_CTX_INTEL_INTEGER | \
DMN_W32_CTX_INTEL_SEGMENTS | DMN_W32_CTX_INTEL_DEBUG | \
DMN_W32_CTX_INTEL_EXTENDED)
#define DMN_W32_CTX_X64_ALL (DMN_W32_CTX_X64 | \
DMN_W32_CTX_INTEL_CONTROL | DMN_W32_CTX_INTEL_INTEGER | \
DMN_W32_CTX_INTEL_SEGMENTS | DMN_W32_CTX_INTEL_FLOATS | \
DMN_W32_CTX_INTEL_DEBUG)

////////////////////////////////
//~ rjf: Per-Entity State

typedef enum DMN_W32_EntityKind
{
  DMN_W32_EntityKind_Null,
  DMN_W32_EntityKind_Root,
  DMN_W32_EntityKind_Process,
  DMN_W32_EntityKind_Thread,
  DMN_W32_EntityKind_Module,
  DMN_W32_EntityKind_COUNT
}
DMN_W32_EntityKind;

typedef struct DMN_W32_Entity DMN_W32_Entity;
struct DMN_W32_Entity
{
  DMN_W32_Entity *first;
  DMN_W32_Entity *last;
  DMN_W32_Entity *next;
  DMN_W32_Entity *prev;
  DMN_W32_Entity *parent;
  DMN_W32_EntityKind kind;
  U32 gen;
  U64 id;
  HANDLE handle;
  Arch arch;
  union
  {
    struct
    {
      U64 injection_address;
      B32 did_first_bp;
    }
    proc;
    struct
    {
      U64 thread_local_base;
      U64 last_name_hash;
      U64 name_gather_time_us;
    }
    thread;
    struct
    {
      Rng1U64 vaddr_range;
      U64 address_of_name_pointer;
      B32 is_main;
      B32 name_is_unicode;
    }
    module;
  };
};

typedef struct DMN_W32_EntityNode DMN_W32_EntityNode;
struct DMN_W32_EntityNode
{
  DMN_W32_EntityNode *next;
  DMN_W32_Entity *v;
};

typedef struct DMN_W32_EntityIDHashNode DMN_W32_EntityIDHashNode;
struct DMN_W32_EntityIDHashNode
{
  DMN_W32_EntityIDHashNode *next;
  DMN_W32_EntityIDHashNode *prev;
  U64 id;
  DMN_W32_Entity *entity;
};

typedef struct DMN_W32_EntityIDHashSlot DMN_W32_EntityIDHashSlot;
struct DMN_W32_EntityIDHashSlot
{
  DMN_W32_EntityIDHashNode *first;
  DMN_W32_EntityIDHashNode *last;
};

////////////////////////////////
//~ rjf: Injection Types

typedef struct DMN_W32_InjectedBreak DMN_W32_InjectedBreak;
struct DMN_W32_InjectedBreak
{
  U64 code;
  U64 user_data;
};

#define DMN_W32_INJECTED_CODE_SIZE 32

////////////////////////////////
//~ rjf: Image Info Types

typedef struct DMN_W32_ImageInfo DMN_W32_ImageInfo;
struct DMN_W32_ImageInfo
{
  Arch arch;
  U32 size;
  U64 tls_index;
};

////////////////////////////////
//~ rjf: Dynamically-Loaded Win32 Function Types

typedef HRESULT DMN_W32_GetThreadDescriptionFunctionType(HANDLE hThread, WCHAR **ppszThreadDescription);

////////////////////////////////
//~ rjf: Shared State Bundle

typedef struct DMN_W32_Shared DMN_W32_Shared;
struct DMN_W32_Shared
{
  // rjf: top-level info
  Arena *arena;
  String8List env_strings;
  
  // rjf: access locking mechanism
  Mutex access_mutex;
  B32 access_run_state;
  
  // rjf: detaching info
  Arena *detach_arena;
  DMN_HandleList detach_processes;
  
  // rjf: entity state
  Arena *entities_arena;
  DMN_W32_Entity *entities_base;
  DMN_W32_Entity *entities_first_free;
  U64 entities_count;
  DMN_W32_EntityIDHashSlot *entities_id_hash_slots;
  U64 entities_id_hash_slots_count;
  DMN_W32_EntityIDHashNode *entities_id_hash_node_free;
  
  // rjf: launch state
  B32 new_process_pending;
  
  // rjf: run results
  B32 resume_needed;
  U32 resume_pid;
  U32 resume_tid;
  B32 exception_not_handled;
  
  // rjf: halting info
  DMN_Handle halter_process;
  U32 halter_tid;
};

////////////////////////////////
//~ rjf: Globals

global DMN_W32_Shared *dmn_w32_shared = 0;
global DMN_W32_Entity dmn_w32_entity_nil = {&dmn_w32_entity_nil, &dmn_w32_entity_nil, &dmn_w32_entity_nil, &dmn_w32_entity_nil, &dmn_w32_entity_nil};
global DMN_W32_GetThreadDescriptionFunctionType *dmn_w32_GetThreadDescription = 0;
thread_static B32 dmn_w32_ctrl_thread = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 dmn_w32_hash_from_string(String8 string);
internal U64 dmn_w32_hash_from_id(U64 id);

////////////////////////////////
//~ rjf: Entity Helpers

//- rjf: entity <-> handle
internal DMN_Handle dmn_w32_handle_from_entity(DMN_W32_Entity *entity);
internal DMN_W32_Entity *dmn_w32_entity_from_handle(DMN_Handle handle);

//- rjf: entity allocation/deallocation
internal DMN_W32_Entity *dmn_w32_entity_alloc(DMN_W32_Entity *parent, DMN_W32_EntityKind kind, U64 id);
internal void dmn_w32_entity_release(DMN_W32_Entity *entity);

//- rjf: kind*id -> entity
internal DMN_W32_Entity *dmn_w32_entity_from_kind_id(DMN_W32_EntityKind kind, U64 id);

////////////////////////////////
//~ rjf: Module Info Extraction

internal String8 dmn_w32_full_path_from_module(Arena *arena, DMN_W32_Entity *module);

////////////////////////////////
//~ rjf: Win32-Level Process/Thread Reads/Writes

//- rjf: processes
internal U64 dmn_w32_process_read(HANDLE process, Rng1U64 range, void *dst);
internal B32 dmn_w32_process_write(HANDLE process, Rng1U64 range, void *src);
internal String8 dmn_w32_read_memory_str(Arena *arena, HANDLE process_handle, U64 address);
internal String16 dmn_w32_read_memory_str16(Arena *arena, HANDLE process_handle, U64 address);
#define dmn_w32_process_read_struct(process, vaddr, ptr) dmn_w32_process_read((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)
#define dmn_w32_process_write_struct(process, vaddr, ptr) dmn_w32_process_write((process), r1u64((vaddr), (vaddr)+(sizeof(*ptr))), ptr)
internal DMN_W32_ImageInfo dmn_w32_image_info_from_process_base_vaddr(HANDLE process, U64 base_vaddr);

//- rjf: threads
internal U16 dmn_w32_real_tag_word_from_xsave(XSAVE_FORMAT *fxsave);
internal U16 dmn_w32_xsave_tag_word_from_real_tag_word(U16 ftw);
internal B32 dmn_w32_thread_read_reg_block(Arch arch, HANDLE thread, void *reg_block);
internal B32 dmn_w32_thread_write_reg_block(Arch arch, HANDLE thread, void *reg_block);

//- rjf: remote thread injection
internal DWORD dmn_w32_inject_thread(HANDLE process, U64 start_address);

#endif // DEMON_CORE_WIN32_H
