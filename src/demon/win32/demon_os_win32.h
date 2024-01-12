// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DEMON_OS_WIN32_H
#define DEMON_OS_WIN32_H

////////////////////////////////
//~ NOTE(allen): Win32 Demon Headers Negotation

// windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

////////////////////////////////
//~ NOTE(allen): Win32 Demon Types

//- entities

// Demon Win32 Entity Extensions
//  Process: ext points to independently allocated DEMON_W32_Ext
//  Thread : ext points to independently allocated DEMON_W32_Ext
//  Module : ext set to HANDLE

typedef union DEMON_W32_Ext DEMON_W32_Ext;
union DEMON_W32_Ext
{
  DEMON_W32_Ext *next;
  struct{
    HANDLE handle;
    U64 injection_address;
    B32 did_first_bp;
  } proc;
  struct{
    HANDLE handle;
    U64 thread_local_base;
    U64 last_name_hash;
    U64 name_gather_time_us;
  } thread;
  struct{
    HANDLE handle;
    U64 address_of_name_pointer;
    B32 is_main;
    B32 name_is_unicode;
  } module;
};

//- helpers

typedef struct DEMON_W32_InjectedBreak DEMON_W32_InjectedBreak;
struct DEMON_W32_InjectedBreak
{
  U64 code;
  U64 user_data;
};
#define DEMON_W32_INJECTED_CODE_SIZE 32

typedef struct DEMON_W32_ImageInfo DEMON_W32_ImageInfo;
struct DEMON_W32_ImageInfo
{
  Architecture arch;
  U32 size;
};

typedef struct DEMON_W32_EntityNode DEMON_W32_EntityNode;
struct DEMON_W32_EntityNode
{
  DEMON_W32_EntityNode *next;
  DEMON_Entity *entity;
};

typedef HRESULT GetThreadDescriptionFunctionType(HANDLE hThread, WCHAR **ppszThreadDescription);

////////////////////////////////
//~ NOTE(allen): Win32 Demon Exceptions

#define DEMON_W32_EXCEPTION_BREAKPOINT               0x80000003u
#define DEMON_W32_EXCEPTION_SINGLE_STEP              0x80000004u
#define DEMON_W32_EXCEPTION_LONG_JUMP                0x80000026u
#define DEMON_W32_EXCEPTION_ACCESS_VIOLATION         0xC0000005u
#define DEMON_W32_EXCEPTION_ARRAY_BOUNDS_EXCEEDED    0xC000008Cu
#define DEMON_W32_EXCEPTION_DATA_TYPE_MISALIGNMENT   0x80000002u
#define DEMON_W32_EXCEPTION_GUARD_PAGE_VIOLATION     0x80000001u
#define DEMON_W32_EXCEPTION_FLT_DENORMAL_OPERAND     0xC000008Du
#define DEMON_W32_EXCEPTION_FLT_DEVIDE_BY_ZERO       0xC000008Eu
#define DEMON_W32_EXCEPTION_FLT_INEXACT_RESULT       0xC000008Fu
#define DEMON_W32_EXCEPTION_FLT_INVALID_OPERATION    0xC0000090u
#define DEMON_W32_EXCEPTION_FLT_OVERFLOW             0xC0000091u
#define DEMON_W32_EXCEPTION_FLT_STACK_CHECK          0xC0000092u
#define DEMON_W32_EXCEPTION_FLT_UNDERFLOW            0xC0000093u
#define DEMON_W32_EXCEPTION_INT_DIVIDE_BY_ZERO       0xC0000094u
#define DEMON_W32_EXCEPTION_INT_OVERFLOW             0xC0000095u
#define DEMON_W32_EXCEPTION_PRIVILEGED_INSTRUCTION   0xC0000096u
#define DEMON_W32_EXCEPTION_ILLEGAL_INSTRUCTION      0xC000001Du
#define DEMON_W32_EXCEPTION_IN_PAGE_ERROR            0xC0000006u
#define DEMON_W32_EXCEPTION_INVALID_DISPOSITION      0xC0000026u
#define DEMON_W32_EXCEPTION_NONCONTINUABLE           0xC0000025u
#define DEMON_W32_EXCEPTION_STACK_OVERFLOW           0xC00000FDu
#define DEMON_W32_EXCEPTION_INVALID_HANDLE           0xC0000008u
#define DEMON_W32_EXCEPTION_UNWIND_CONSOLIDATE       0x80000029u
#define DEMON_W32_EXCEPTION_DLL_NOT_FOUND            0xC0000135u
#define DEMON_W32_EXCEPTION_ORDINAL_NOT_FOUND        0xC0000138u
#define DEMON_W32_EXCEPTION_ENTRY_POINT_NOT_FOUND    0xC0000139u
#define DEMON_W32_EXCEPTION_DLL_INIT_FAILED          0xC0000142u
#define DEMON_W32_EXCEPTION_CONTROL_C_EXIT           0xC000013Au
#define DEMON_W32_EXCEPTION_FLT_MULTIPLE_FAULTS      0xC00002B4u
#define DEMON_W32_EXCEPTION_FLT_MULTIPLE_TRAPS       0xC00002B5u
#define DEMON_W32_EXCEPTION_NAT_CONSUMPTION          0xC00002C9u
#define DEMON_W32_EXCEPTION_HEAP_CORRUPTION          0xC0000374u
#define DEMON_W32_EXCEPTION_STACK_BUFFER_OVERRUN     0xC0000409u
#define DEMON_W32_EXCEPTION_INVALID_CRUNTIME_PARAM   0xC0000417u
#define DEMON_W32_EXCEPTION_ASSERT_FAILURE           0xC0000420u
#define DEMON_W32_EXCEPTION_NO_MEMORY                0xC0000017u
#define DEMON_W32_EXCEPTION_THROW                    0xE06D7363u
#define DEMON_W32_EXCEPTION_SET_THREAD_NAME          0x406d1388u

////////////////////////////////
//~ NOTE(allen): Win32 Demon Register API Codes

#define DEMON_W32_CTX_X86       0x00010000
#define DEMON_W32_CTX_X64       0x00100000

#define DEMON_W32_CTX_INTEL_CONTROL       0x0001
#define DEMON_W32_CTX_INTEL_INTEGER       0x0002
#define DEMON_W32_CTX_INTEL_SEGMENTS      0x0004
#define DEMON_W32_CTX_INTEL_FLOATS        0x0008
#define DEMON_W32_CTX_INTEL_DEBUG         0x0010
#define DEMON_W32_CTX_INTEL_EXTENDED      0x0020
#define DEMON_W32_CTX_INTEL_XSTATE        0x0040

#define DEMON_W32_CTX_X86_ALL (DEMON_W32_CTX_X86 | \
DEMON_W32_CTX_INTEL_CONTROL | DEMON_W32_CTX_INTEL_INTEGER | \
DEMON_W32_CTX_INTEL_SEGMENTS | DEMON_W32_CTX_INTEL_DEBUG | \
DEMON_W32_CTX_INTEL_EXTENDED)
#define DEMON_W32_CTX_X64_ALL (DEMON_W32_CTX_X64 | \
DEMON_W32_CTX_INTEL_CONTROL | DEMON_W32_CTX_INTEL_INTEGER | \
DEMON_W32_CTX_INTEL_SEGMENTS | DEMON_W32_CTX_INTEL_FLOATS | \
DEMON_W32_CTX_INTEL_DEBUG)

////////////////////////////////
//~ rjf: DOS Header Types

// this is the "MZ" as a 16-bit short
#define DEMON_DOS_MAGIC 0x5a4d

#pragma pack(push,1)
typedef struct DEMON_DosHeader DEMON_DosHeader;
struct DEMON_DosHeader
{
  U16 magic;
  U16 last_page_size;
  U16 page_count;
  U16 reloc_count;
  U16 paragraph_header_size;
  U16 min_paragraph;
  U16 max_paragraph;
  U16 init_ss;
  U16 init_sp;
  U16 checksum;
  U16 init_ip;
  U16 init_cs;
  U16 reloc_table_file_off;
  U16 overlay_number;
  U16 reserved[4];
  U16 oem_id;
  U16 oem_info;
  U16 reserved2[10];
  U32 coff_file_offset;
};
#pragma pack(pop)

////////////////////////////////
//~ rjf: Coff Header Types

#define DEMON_PE_MAGIC   0x00004550u

typedef U16 DEMON_CoffMachineType;
enum{
  DEMON_CoffMachineType_UNKNOWN = 0x0,
  DEMON_CoffMachineType_X86 = 0x14c,
  DEMON_CoffMachineType_X64 = 0x8664,
  DEMON_CoffMachineType_ARM33 = 0x1d3,
  DEMON_CoffMachineType_ARM = 0x1c0,
  DEMON_CoffMachineType_ARM64 = 0xaa64,
  DEMON_CoffMachineType_ARMNT = 0x1c4,
  DEMON_CoffMachineType_EBC = 0xebc,
  DEMON_CoffMachineType_IA64 = 0x200,
  DEMON_CoffMachineType_M32R = 0x9041,
  DEMON_CoffMachineType_MIPS16 = 0x266,
  DEMON_CoffMachineType_MIPSFPU = 0x366,
  DEMON_CoffMachineType_MIPSFPU16 = 0x466,
  DEMON_CoffMachineType_POWERPC = 0x1f0,
  DEMON_CoffMachineType_POWERPCFP = 0x1f1,
  DEMON_CoffMachineType_R4000 = 0x166,
  DEMON_CoffMachineType_RISCV32 = 0x5032,
  DEMON_CoffMachineType_RISCV64 = 0x5064,
  DEMON_CoffMachineType_RISCV128 = 0x5128,
  DEMON_CoffMachineType_SH3 = 0x1a2,
  DEMON_CoffMachineType_SH3DSP = 0x1a3,
  DEMON_CoffMachineType_SH4 = 0x1a6,
  DEMON_CoffMachineType_SH5 = 0x1a8,
  DEMON_CoffMachineType_THUMB = 0x1c2,
  DEMON_CoffMachineType_WCEMIPSV2 = 0x169,
  DEMON_CoffMachineType_COUNT = 25
};

typedef U16 DEMON_CoffFlags;
enum{
  DEMON_CoffFlag_RELOC_STRIPPED = (1 << 0),
  DEMON_CoffFlag_EXECUTABLE_IMAGE = (1 << 1),
  DEMON_CoffFlag_LINE_NUMS_STRIPPED = (1 << 2),
  DEMON_CoffFlag_SYM_STRIPPED = (1 << 3),
  DEMON_CoffFlag_RESERVED_0 = (1 << 4),
  DEMON_CoffFlag_LARGE_ADDRESS_AWARE = (1 << 5),
  DEMON_CoffFlag_RESERVED_1 = (1 << 6),
  DEMON_CoffFlag_RESERVED_2 = (1 << 7),
  DEMON_CoffFlag_32BIT_MACHINE = (1 << 8),
  DEMON_CoffFlag_DEBUG_STRIPPED = (1 << 9),
  DEMON_CoffFlag_REMOVABLE_RUN_FROM_SWAP = (1 << 10),
  DEMON_CoffFlag_NET_RUN_FROM_SWAP = (1 << 11),
  DEMON_CoffFlag_SYSTEM = (1 << 12),
  DEMON_CoffFlag_DLL = (1 << 13),
  DEMON_CoffFlag_UP_SYSTEM_ONLY = (1 << 14),
  DEMON_CoffFlag_BYTES_RESERVED_HI = (1 << 15),
};

#pragma pack(push,1)
typedef struct DEMON_CoffHeader DEMON_CoffHeader;
struct DEMON_CoffHeader
{
  DEMON_CoffMachineType machine;
  U16 section_count;
  U32 time_date_stamp;
  //  TODO: rename to "unix_timestamp"
  U32 pointer_to_symbol_table;
  U32 number_of_symbols;
  //  TODO: rename to "symbol_count"
  U16 size_of_optional_header;
  //  TODO: rename to "optional_header_size"
  DEMON_CoffFlags flags;
};
#pragma pack(pop)

////////////////////////////////
//~ rjf: PE Header Types

#pragma pack(push, 1)

typedef U16 DEMON_PeWindowsSubsystem;
enum{
  DEMON_PeWindowsSubsystem_UNKNOWN = 0,
  DEMON_PeWindowsSubsystem_NATIVE = 1,
  DEMON_PeWindowsSubsystem_WINDOWS_GUI = 2,
  DEMON_PeWindowsSubsystem_WINDOWS_CUI = 3,
  DEMON_PeWindowsSubsystem_OS2_CUI = 5,
  DEMON_PeWindowsSubsystem_POSIX_CUI = 7,
  DEMON_PeWindowsSubsystem_NATIVE_WINDOWS = 8,
  DEMON_PeWindowsSubsystem_WINDOWS_CE_GUI = 9,
  DEMON_PeWindowsSubsystem_EFI_APPLICATION = 10,
  DEMON_PeWindowsSubsystem_EFI_BOOT_SERVICE_DRIVER = 11,
  DEMON_PeWindowsSubsystem_EFI_RUNTIME_DRIVER = 12,
  DEMON_PeWindowsSubsystem_EFI_ROM = 13,
  DEMON_PeWindowsSubsystem_XBOX = 14,
  DEMON_PeWindowsSubsystem_WINDOWS_BOOT_APPLICATION = 16,
  DEMON_PeWindowsSubsystem_COUNT = 14
};

typedef U16 DEMON_DllCharacteristics;
enum{
  DEMON_DllCharacteristic_HIGH_ENTROPY_VA = (1 << 5),
  DEMON_DllCharacteristic_DYNAMIC_BASE = (1 << 6),
  DEMON_DllCharacteristic_FORCE_INTEGRITY = (1 << 7),
  DEMON_DllCharacteristic_NX_COMPAT = (1 << 8),
  DEMON_DllCharacteristic_NO_ISOLATION = (1 << 9),
  DEMON_DllCharacteristic_NO_SEH = (1 << 10),
  DEMON_DllCharacteristic_NO_BIND = (1 << 11),
  DEMON_DllCharacteristic_APPCONTAINER = (1 << 12),
  DEMON_DllCharacteristic_WDM_DRIVER = (1 << 13),
  DEMON_DllCharacteristic_GUARD_CF = (1 << 14),
  DEMON_DllCharacteristic_TERMINAL_SERVER_AWARE = (1 << 15),
};

typedef struct DEMON_PeOptionalHeader32 DEMON_PeOptionalHeader32;
struct DEMON_PeOptionalHeader32
{
  U16 magic;
  U8 major_linker_version;
  U8 minor_linker_version;
  U32 sizeof_code;
  U32 sizeof_inited_data;
  U32 sizeof_uninited_data;
  U32 entry_point_va;
  U32 code_base;
  U32 data_base;
  U32 image_base;
  U32 section_alignment;
  U32 file_alignment;
  U16 major_os_ver;
  U16 minor_os_ver;
  U16 major_img_ver;
  U16 minor_img_ver;
  U16 major_subsystem_ver;
  U16 minor_subsystem_ver;
  U32 win32_version_value;
  U32 sizeof_image;
  U32 sizeof_headers;
  U32 check_sum;
  DEMON_PeWindowsSubsystem subsystem;
  DEMON_DllCharacteristics dll_characteristics;
  U32 sizeof_stack_reserve;
  U32 sizeof_stack_commit;
  U32 sizeof_heap_reserve;
  U32 sizeof_heap_commit;
  U32 loader_flags;
  U32 data_dir_count;
};

typedef struct DEMON_PeOptionalHeader32Plus DEMON_PeOptionalHeader32Plus;
struct DEMON_PeOptionalHeader32Plus
{
  U16 magic;
  U8 major_linker_version;
  U8 minor_linker_version;
  U32 sizeof_code;
  U32 sizeof_inited_data;
  U32 sizeof_uninited_data;
  U32 entry_point_va;
  U32 code_base;
  U64 image_base;
  U32 section_alignment;
  U32 file_alignment;
  U16 major_os_ver;
  U16 minor_os_ver;
  U16 major_img_ver;
  U16 minor_img_ver;
  U16 major_subsystem_ver;
  U16 minor_subsystem_ver;
  U32 win32_version_value;
  U32 sizeof_image;
  U32 sizeof_headers;
  U32 check_sum;
  DEMON_PeWindowsSubsystem subsystem;
  DEMON_DllCharacteristics dll_characteristics;
  U64 sizeof_stack_reserve;
  U64 sizeof_stack_commit;
  U64 sizeof_heap_reserve;
  U64 sizeof_heap_commit;
  U32 loader_flags;
  U32 data_dir_count;
};

#pragma pack(pop)

////////////////////////////////
//~ rjf: Helpers

internal U64 demon_w32_hash_from_string(String8 string);
internal DEMON_W32_Ext* demon_w32_ext_alloc(void);
internal DEMON_W32_Ext* demon_w32_ext(DEMON_Entity *entity);

internal U64 demon_w32_read_memory(HANDLE process_handle, void *dst, U64 src_address, U64 size);
internal B32 demon_w32_write_memory(HANDLE process_handle, U64 dst_address, void *src, U64 size);
internal String8 demon_w32_read_memory_str(Arena *arena, HANDLE process_handle, U64 address);
internal String16 demon_w32_read_memory_str16(Arena *arena, HANDLE process_handle, U64 address);

#define demon_w32_read_struct(h,dst,src)  demon_w32_read_memory((h), (dst), (src), sizeof(*(dst)))

internal DEMON_W32_ImageInfo demon_w32_image_info_from_base(HANDLE process_handle, U64 base);
internal DWORD demon_w32_inject_thread(DEMON_Entity *process, U64 start_address);

internal U16 demon_w32_real_tag_word_from_xsave(XSAVE_FORMAT *fxsave);
internal U16 demon_w32_xsave_tag_word_from_real_tag_word(U16 ftw);

internal DWORD demon_w32_win32_from_memory_protect_flags(DEMON_MemoryProtectFlags flags);

////////////////////////////////
//~ rjf: Experiments

internal void demon_w32_peak_at_tls(DEMON_Handle handle);

#endif //DEMON_OS_WIN32_H
