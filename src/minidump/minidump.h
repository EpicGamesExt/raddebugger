// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef MINIDUMP_H
#define MINIDUMP_H

#pragma pack(push, 1)

typedef enum MDMP_DumpKind
{
  MDMP_DumpKind_Normal                                     = 0x00000000,
  MDMP_DumpKind_WithDataSegs                               = 0x00000001,
  MDMP_DumpKind_WithFullMemory                             = 0x00000002,
  MDMP_DumpKind_WithHandleData                             = 0x00000004,
  MDMP_DumpKind_FilterMemory                               = 0x00000008,
  MDMP_DumpKind_ScanMemory                                 = 0x00000010,
  MDMP_DumpKind_WithUnloadedModules                        = 0x00000020,
  MDMP_DumpKind_WithIndirectlyReferencedMemory             = 0x00000040,
  MDMP_DumpKind_FilterModulePaths                          = 0x00000080,
  MDMP_DumpKind_WithProcessThreadData                      = 0x00000100,
  MDMP_DumpKind_WithPrivateReadWriteMemory                 = 0x00000200,
  MDMP_DumpKind_WithoutOptionalData                        = 0x00000400,
  MDMP_DumpKind_WithFullMemoryInfo                         = 0x00000800,
  MDMP_DumpKind_WithThreadInfo                             = 0x00001000,
  MDMP_DumpKind_WithCodeSegs                               = 0x00002000,
  MDMP_DumpKind_WithoutAuxiliaryState                      = 0x00004000,
  MDMP_DumpKind_WithFullAuxiliaryState                     = 0x00008000,
  MDMP_DumpKind_WithPrivateWriteCopyMemory                 = 0x00010000,
  MDMP_DumpKind_IgnoreInaccessibleMemory                   = 0x00020000,
  MDMP_DumpKind_WithTokenInformation                       = 0x00040000,
  MDMP_DumpKind_WithModuleHeaders                          = 0x00080000,
  MDMP_DumpKind_FilterTriage                               = 0x00100000,
  MDMP_DumpKind_WithAvxXStateContext                       = 0x00200000,
  MDMP_DumpKind_WithIptTrace                               = 0x00400000,
  MDMP_DumpKind_ScanInaccessiblePartialPages               = 0x00800000,
  MDMP_DumpKind_FilterWriteCombinedMemory,
  MDMP_DumpKind_ValidTypeFlags                             = 0x01ffffff,
  MDMP_DumpKind_NoIgnoreInaccessibleMemory,
  MDMP_DumpKind_ValidTypeFlagsEx
}
MDMP_DumpKind;

typedef enum MDMP_StreamKind
{
  MDMP_StreamKind_Unused                             = 0,
  MDMP_StreamKind_Reserved0                          = 1,
  MDMP_StreamKind_Reserved1                          = 2,
  MDMP_StreamKind_ThreadList                         = 3,
  MDMP_StreamKind_ModuleList                         = 4,
  MDMP_StreamKind_MemoryList                         = 5,
  MDMP_StreamKind_Exception                          = 6,
  MDMP_StreamKind_SystemInfo                         = 7,
  MDMP_StreamKind_ThreadExList                       = 8,
  MDMP_StreamKind_Memory64List                       = 9,
  MDMP_StreamKind_CommentA                           = 10,
  MDMP_StreamKind_CommentW                           = 11,
  MDMP_StreamKind_HandleData                         = 12,
  MDMP_StreamKind_FunctionTable                      = 13,
  MDMP_StreamKind_UnloadedModuleList                 = 14,
  MDMP_StreamKind_MiscInfo                           = 15,
  MDMP_StreamKind_MemoryInfoList                     = 16,
  MDMP_StreamKind_ThreadInfoList                     = 17,
  MDMP_StreamKind_HandleOperationList                = 18,
  MDMP_StreamKind_Token                              = 19,
  MDMP_StreamKind_JavaScriptData                     = 20,
  MDMP_StreamKind_SystemMemoryInfo                   = 21,
  MDMP_StreamKind_ProcessVmCounters                  = 22,
  MDMP_StreamKind_IptTrace                           = 23,
  MDMP_StreamKind_ThreadNames                        = 24,
  MDMP_StreamKind_ceNull                             = 0x8000,
  MDMP_StreamKind_ceSystemInfo                       = 0x8001,
  MDMP_StreamKind_ceException                        = 0x8002,
  MDMP_StreamKind_ceModuleList                       = 0x8003,
  MDMP_StreamKind_ceProcessList                      = 0x8004,
  MDMP_StreamKind_ceThreadList                       = 0x8005,
  MDMP_StreamKind_ceThreadContextList                = 0x8006,
  MDMP_StreamKind_ceThreadCallStackList              = 0x8007,
  MDMP_StreamKind_ceMemoryVirtualList                = 0x8008,
  MDMP_StreamKind_ceMemoryPhysicalList               = 0x8009,
  MDMP_StreamKind_ceBucketParameters                 = 0x800A,
  MDMP_StreamKind_ceProcessModuleMap                 = 0x800B,
  MDMP_StreamKind_ceDiagnosisList                    = 0x800C,
  MDMP_StreamKind_LastReserved                       = 0xffff
}
MDMP_StreamKind;

#define MDMP_MAGIC 0x504d444d

typedef struct MDMP_Header MDMP_Header;
struct MDMP_Header
{
  U32 magic;
  U32 version;
  U32 number_of_streams;
  U32 stream_directory_foff;
  U32 checksum;
  U32 reserved_or_time_date_stamp;
  U64 flags; // MDMP_DumpKind
};

typedef struct MDMP_LocationDescriptor32 MDMP_LocationDescriptor32;
struct MDMP_LocationDescriptor32
{
  U32 data_size;
  U32 foff;
};

typedef struct MDMP_LocationDescriptor64 MDMP_LocationDescriptor64;
struct MDMP_LocationDescriptor64
{
  U64 data_size;
  U64 foff;
};

typedef struct MDMP_Directory MDMP_Directory;
struct MDMP_Directory
{
  U32 stream_kind;
  MDMP_LocationDescriptor32 location;
};

typedef struct MDMP_MemoryDescriptor32 MDMP_MemoryDescriptor32;
struct MDMP_MemoryDescriptor32
{
  U64 start_of_memory_range;
  MDMP_LocationDescriptor32 memory_location;
};

typedef struct MDMP_MemoryDescriptor64 MDMP_MemoryDescriptor64;
struct MDMP_MemoryDescriptor64
{
  U64 start_of_memory_range;
  U64 size;
};

typedef struct MDMP_Thread MDMP_Thread;
struct MDMP_Thread
{
  U32 id;
  U32 suspend_count;
  U32 priority_class;
  U32 priority;
  U64 teb;
  MDMP_MemoryDescriptor32 stack;
  MDMP_LocationDescriptor32 thread_context;
};

typedef struct MDMP_FixedFileInfo MDMP_FixedFileInfo;
struct MDMP_FixedFileInfo
{
  U32 signature;
  U32 struct_version;
  U32 file_version_ms;
  U32 file_version_ls;
  U32 product_version_ms;
  U32 product_version_ls;
  U32 file_flags_mask;
  U32 file_flags;
  U32 file_os;
  U32 file_type;
  U32 file_subtype;
  U32 file_date_ms;
  U32 file_date_ls;
};

typedef struct MDMP_Module MDMP_Module;
struct MDMP_Module
{
  U64 image_base_vaddr;
  U32 image_size;
  U32 checksum;
  U32 time_date_stamp;
  U32 module_name_foff;
  MDMP_FixedFileInfo version_info;
  MDMP_LocationDescriptor32 cv_record;
  MDMP_LocationDescriptor32 misc_record;
  U64 reserved_0;
  U64 reserved_1;
};

typedef struct MDMP_Exception MDMP_Exception;
struct MDMP_Exception
{
  U32 exception_code;
  U32 exception_flags;
  U64 exception_record;
  U64 exception_addr;
  U32 number_parameters;
  U32 unused_alignment;
  U64 exception_information[15];
};

typedef struct MDMP_ExceptionStream MDMP_ExceptionStream;
struct MDMP_ExceptionStream
{
  U32 thread_id;
  U32 padding;
  MDMP_Exception exception;
  MDMP_LocationDescriptor32 thread_context;
};

typedef U16 MDMP_Arch;
typedef enum MDMP_ArchEnum
{
  MDMP_Arch_x86  = 0,
  MDMP_Arch_Arm  = 5,
  MDMP_Arch_IA64 = 6,
  MDMP_Arch_x64  = 9,
  MDMP_Arch_Unknown = 0xffff,
}
MDMP_ArchEnum;

typedef struct MDMP_SystemInfo MDMP_SystemInfo;
struct MDMP_SystemInfo
{
  MDMP_Arch processor_architecture;
  U16 processor_level;
  U16 processor_revision;
  U8 number_of_processors;
  U8 product_type;
  U32 major_version;
  U32 minor_version;
  U32 build_number;
  U32 platform_id;
  U32 csd_version_rva;
  U16 suite_mask;
  U16 padding;
  // CPU_INFORMATION Cpu; (architecture dependent)
};

#pragma pack(pop)

#endif // MINIDUMP_H
