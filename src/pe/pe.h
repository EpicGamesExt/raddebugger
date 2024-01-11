// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

/* date = September 6th 2023 8:54 am */

#ifndef PE_H
#define PE_H

////////////////////////////////
//~ rjf: PE Format Types

#pragma pack(push,1)

typedef struct PE_DosHeader PE_DosHeader;
struct PE_DosHeader
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

typedef U16 PE_WindowsSubsystem;
enum
{
  PE_WindowsSubsystem_UNKNOWN                  = 0,
  PE_WindowsSubsystem_NATIVE                   = 1,
  PE_WindowsSubsystem_WINDOWS_GUI              = 2,
  PE_WindowsSubsystem_WINDOWS_CUI              = 3,
  PE_WindowsSubsystem_OS2_CUI                  = 5,
  PE_WindowsSubsystem_POSIX_CUI                = 7,
  PE_WindowsSubsystem_NATIVE_WINDOWS           = 8,
  PE_WindowsSubsystem_WINDOWS_CE_GUI           = 9,
  PE_WindowsSubsystem_EFI_APPLICATION          = 10,
  PE_WindowsSubsystem_EFI_BOOT_SERVICE_DRIVER  = 11,
  PE_WindowsSubsystem_EFI_RUNTIME_DRIVER       = 12,
  PE_WindowsSubsystem_EFI_ROM                  = 13,
  PE_WindowsSubsystem_XBOX                     = 14,
  PE_WindowsSubsystem_WINDOWS_BOOT_APPLICATION = 16,
  PE_WindowsSubsystem_COUNT                    = 14
};

typedef U16 PE_ImageFileCharacteristics;
enum
{
  PE_ImageFileCharacteristic_STRIPPED                      = (1 << 0),
  PE_ImageFileCharacteristic_EXE                           = (1 << 1),
  PE_ImageFileCharacteristic_NUMS_STRIPPED                 = (1 << 2),
  PE_ImageFileCharacteristic_PE_STRIPPED                   = (1 << 3),
  PE_ImageFileCharacteristic_AGGRESIVE_WS_TRIM             = (1 << 4),
  PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE           = (1 << 5),
  PE_ImageFileCharacteristic_UNUSED1                       = (1 << 6),
  PE_ImageFileCharacteristic_BYTES_RESERVED_LO             = (1 << 7),
  PE_ImageFileCharacteristic_32BIT_MACHINE                 = (1 << 8),
  PE_ImageFileCharacteristic_DEBUG_STRIPPED                = (1 << 9),
  PE_ImageFileCharacteristic_FILE_REMOVABLE_RUN_FROM_SWAP  = (1 << 10),
  PE_ImageFileCharacteristic_NET_RUN_FROM_SWAP             = (1 << 11),
  PE_ImageFileCharacteristic_FILE_SYSTEM                   = (1 << 12),
  PE_ImageFileCharacteristic_FILE_DLL                      = (1 << 13),
  PE_ImageFileCharacteristic_FILE_UP_SYSTEM_ONLY           = (1 << 14),
  PE_ImageFileCharacteristic_BYTES_RESERVED_HI             = (1 << 15),
};

typedef U16 PE_DllCharacteristics;
enum
{
  PE_DllCharacteristic_HIGH_ENTROPY_VA       = (1 << 5),
  PE_DllCharacteristic_DYNAMIC_BASE          = (1 << 6),
  PE_DllCharacteristic_FORCE_INTEGRITY       = (1 << 7),
  PE_DllCharacteristic_NX_COMPAT             = (1 << 8),
  PE_DllCharacteristic_NO_ISOLATION          = (1 << 9),
  PE_DllCharacteristic_NO_SEH                = (1 << 10),
  PE_DllCharacteristic_NO_BIND               = (1 << 11),
  PE_DllCharacteristic_APPCONTAINER          = (1 << 12),
  PE_DllCharacteristic_WDM_DRIVER            = (1 << 13),
  PE_DllCharacteristic_GUARD_CF              = (1 << 14),
  PE_DllCharacteristic_TERMINAL_SERVER_AWARE = (1 << 15),
};

typedef struct PE_OptionalHeader32 PE_OptionalHeader32;
struct PE_OptionalHeader32
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
  PE_WindowsSubsystem subsystem;
  PE_DllCharacteristics dll_characteristics;
  U32 sizeof_stack_reserve;
  U32 sizeof_stack_commit;
  U32 sizeof_heap_reserve;
  U32 sizeof_heap_commit;
  U32 loader_flags;
  U32 data_dir_count;
};

typedef struct PE_OptionalHeader32Plus PE_OptionalHeader32Plus;
struct PE_OptionalHeader32Plus
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
  PE_WindowsSubsystem subsystem;
  PE_DllCharacteristics dll_characteristics;
  U64 sizeof_stack_reserve;
  U64 sizeof_stack_commit;
  U64 sizeof_heap_reserve;
  U64 sizeof_heap_commit;
  U32 loader_flags;
  U32 data_dir_count;
};

typedef enum PE_DataDirectoryIndex
{
  PE_DataDirectoryIndex_EXPORT,
  PE_DataDirectoryIndex_IMPORT,
  PE_DataDirectoryIndex_RESOURCES,
  PE_DataDirectoryIndex_EXCEPTIONS,
  PE_DataDirectoryIndex_CERT,
  PE_DataDirectoryIndex_BASE_RELOC,
  PE_DataDirectoryIndex_DEBUG,
  PE_DataDirectoryIndex_ARCH,
  PE_DataDirectoryIndex_GLOBAL_PTR,
  PE_DataDirectoryIndex_TLS,
  PE_DataDirectoryIndex_LOAD_CONFIG,
  PE_DataDirectoryIndex_BOUND_IMPORT,
  PE_DataDirectoryIndex_IMPORT_ADDR,
  PE_DataDirectoryIndex_DELAY_IMPORT,
  PE_DataDirectoryIndex_COM_DESCRIPTOR,
  PE_DataDirectoryIndex_RESERVED,
  PE_DataDirectoryIndex_COUNT = 16
}
PE_DataDirectoryIndex;

typedef struct PE_DataDirectory PE_DataDirectory;
struct PE_DataDirectory
{
  U32 virt_off;
  U32 virt_size;
};

typedef U32 PE_DebugDirectoryType;
enum
{
  PE_DebugDirectoryType_UNKNOWN               = 0,
  PE_DebugDirectoryType_COFF                  = 1,
  PE_DebugDirectoryType_CODEVIEW              = 2,
  PE_DebugDirectoryType_FPO                   = 3,
  PE_DebugDirectoryType_MISC                  = 4,
  PE_DebugDirectoryType_EXCEPTION             = 5,
  PE_DebugDirectoryType_FIXUP                 = 6,
  PE_DebugDirectoryType_OMAP_TO_SRC           = 7,
  PE_DebugDirectoryType_OMAP_FROM_SRC         = 8,
  PE_DebugDirectoryType_BORLAND               = 9,
  PE_DebugDirectoryType_RESERVED10            = 10,
  PE_DebugDirectoryType_CLSID                 = 11,
  PE_DebugDirectoryType_VC_FEATURE            = 12,
  PE_DebugDirectoryType_POGO                  = 13,
  PE_DebugDirectoryType_ILTCG                 = 14,
  PE_DebugDirectoryType_MPX                   = 15,
  PE_DebugDirectoryType_REPRO                 = 16,
  PE_DebugDirectoryType_EX_DLLCHARACTERISTICS = 20,
  PE_DebugDirectoryType_COUNT                 = 18
};

typedef U8 PE_FPOFlags;
enum
{
  PE_FPOFlags_HAS_SEH    = 0x800,
  PE_FPOFlags_USE_BP_REG = 0x1000,
  PE_FPOFlags_RESERVED   = 0x2000,
  PE_FPOFlags_COUNT      = 3
};

typedef U16 PE_FPOEncoded;
enum
{
  PE_FPOEncoded_PROLOG_SIZE_SHIFT     = 0,  PE_FPOEncoded_PROLOG_SIZE_MASK     = 0xff,
  PE_FPOEncoded_SAVED_REGS_SIZE_SHIFT = 8,  PE_FPOEncoded_SAVED_REGS_SIZE_MASK = 0x7,
  PE_FPOEncoded_FLAGS_SHIFT           = 11, PE_FPOEncoded_FLAGS_MASK           = 0x7,
  PE_FPOEncoded_FRAME_TYPE_SHIFT      = 14, PE_FPOEncoded_FRAME_TYPE_MASK      = 0x3,
};
#define PE_FPOEncoded_Extract_PROLOG_SIZE(f)     (U8)(((f) >> PE_FPOEncoded_PROLOG_SIZE_SHIFT) & PE_FPOEncoded_PROLOG_SIZE_MASK)
#define PE_FPOEncoded_Extract_SAVED_REGS_SIZE(f) (U8)(((f) >> PE_FPOEncoded_SAVED_REGS_SIZE_SHIFT) & PE_FPOEncoded_SAVED_REGS_SIZE_MASK)
#define PE_FPOEncoded_Extract_FLAGS(f)      (U8)(((f) >> PE_FPOEncoded_FLAGS_SHIFT) & PE_FPOEncoded_FLAGS_MASK)
#define PE_FPOEncoded_Extract_FRAME_TYPE(f) (U8)(((f) >> PE_FPOEncoded_FRAME_TYPE_SHIFT) & PE_FPOEncoded_FRAME_TYPE_MASK)

typedef U8 PE_FPOType;
enum
{
  PE_FPOType_FPO = 0,
  PE_FPOType_TRAP = 1,
  PE_FPOType_TSS = 2,
  PE_FPOType_NOFPO = 3,
  PE_FPOType_COUNT = 4
};

typedef U32 PE_DebugMiscType;
enum
{
  PE_DebugMiscType_NULL,
  PE_DebugMiscType_EXE_NAME,
  PE_DebugMiscType_COUNT = 2
};

typedef struct PE_DebugDirectory PE_DebugDirectory;
struct PE_DebugDirectory
{
  U32 characteristics;
  COFF_TimeStamp time_stamp;
  U16 major_ver;
  U16 minor_ver;
  PE_DebugDirectoryType type;
  U32 size;
  U32 voff;
  U32 foff;
};

typedef U32 PE_GlobalFlags;
enum
{
  PE_GlobalFlags_STOP_ON_EXCEPTION          = (1 << 0),
  PE_GlobalFlags_SHOW_LDR_SNAPS             = (1 << 1),
  PE_GlobalFlags_DEBUG_INITIAL_COMMAND      = (1 << 2),
  PE_GlobalFlags_STOP_ON_HUNG_GUI           = (1 << 3),
  PE_GlobalFlags_HEAP_ENABLE_TAIL_CHECK     = (1 << 4),
  PE_GlobalFlags_HEAP_ENABLE_FREE_CHECK     = (1 << 5),
  PE_GlobalFlags_HEAP_VALIDATE_PARAMETERS   = (1 << 6),
  PE_GlobalFlags_HEAP_VALIDATE_ALL          = (1 << 7),
  PE_GlobalFlags_APPLICATION_VERIFIER       = (1 << 8),
  PE_GlobalFlags_POOL_ENABLE_TAGGING        = (1 << 10),
  PE_GlobalFlags_HEAP_ENABLE_TAGGING        = (1 << 11),
  PE_GlobalFlags_STACK_TRACE_DB             = (1 << 12),
  PE_GlobalFlags_KERNEL_STACK_TRACE_DB      = (1 << 13),
  PE_GlobalFlags_MAINTAIN_OBJECT_TYPELIST   = (1 << 14),
  PE_GlobalFlags_HEAP_ENABLE_TAG_BY_DLL     = (1 << 15),
  PE_GlobalFlags_DISABLE_STACK_EXTENSION    = (1 << 16),
  PE_GlobalFlags_ENABLE_CSRDEBUG            = (1 << 17),
  PE_GlobalFlags_ENABLE_KDEBUG_SYMBOL_LOAD 	= (1 << 18),
  PE_GlobalFlags_DISABLE_PAGE_KERNEL_STACKS = (1 << 19),
  PE_GlobalFlags_ENABLE_SYSTEM_CRIT_BREAKS  = (1 << 20),
  PE_GlobalFlags_HEAP_DISABLE_COALESCING    = (1 << 21),
  PE_GlobalFlags_ENABLE_CLOSE_EXCEPTIONS    = (1 << 22),
  PE_GlobalFlags_ENABLE_EXCEPTION_LOGGING   = (1 << 23),
  PE_GlobalFlags_ENABLE_HANDLE_TYPE_TAGGING = (1 << 24),
  PE_GlobalFlags_HEAP_PAGE_ALLOCS           = (1 << 25),
  PE_GlobalFlags_DEBUG_INITIAL_COMMAND_EX 	= (1 << 26),
  PE_GlobalFlags_DISABLE_DBGPRINT           = (1 << 27),
  PE_GlobalFlags_CRITSEC_EVENT_CREATION     = (1 << 28),
  PE_GlobalFlags_LDR_TOP_DOWN               = (1 << 29),
  PE_GlobalFlags_ENABLE_HANDLE_EXCEPTIONS   = (1 << 30),
  PE_GlobalFlags_DISABLE_PROTDLLS           = (1 << 31),
};

typedef U32 PE_LoadConfigGuardFlags;
enum
{
  PE_LoadConfigGuardFlags_CF_INSTRUMENTED                    = (1 << 8),
  PE_LoadConfigGuardFlags_CFW_INSTRUMENTED                   = (1 << 9),
  PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_PRESENT          = (1 << 10),
  PE_LoadConfigGuardFlags_SECURITY_COOKIE_UNUSED             = (1 << 11),
  PE_LoadConfigGuardFlags_PROTECT_DELAYLOAD_IAT              = (1 << 12),
  PE_LoadConfigGuardFlags_DELAYLOAD_IAT_IN_ITS_OWN_SECTION   = (1 << 13),
  PE_LoadConfigGuardFlags_CF_EXPORT_SUPPRESSION_INFO_PRESENT = (1 << 14),
  PE_LoadConfigGuardFlags_CF_ENABLE_EXPORT_SUPPRESSION       = (1 << 15),
  PE_LoadConfigGuardFlags_CF_LONGJUMP_TABLE_PRESENT          = (1 << 16),
  PE_LoadConfigGuardFlags_EH_CONTINUATION_TABLE_PRESENT      = (1 << 22),
  PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_SIZE_SHIFT       = 28, PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_SIZE_MASK = 0xf,
};
#define PE_LoadConfigGuardFlags_Extract_CF_FUNCTION_TABLE_SIZE(f) (U32)(((f) >> PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_SIZE_SHIFT) & PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_SIZE_MASK)

// this is the "MZ" as a 16-bit short
#define PE_DOS_MAGIC      0x5a4d
#define PE_MAGIC          0x00004550u
#define PE_PE32_MAGIC     0x010bu
#define PE_PE32PLUS_MAGIC 0x020bu

typedef struct PE_MipsPdata PE_MipsPdata;
struct PE_MipsPdata
{
  U32 voff_first;
  U32 voff_one_past_last;
  U32 voff_exception_handler;
  U32 voff_exception_handler_data;
  U32 voff_one_past_prolog;
};

typedef struct PE_ArmPdata PE_ArmPdata;
struct PE_ArmPdata
{
  U32 voff_first;
  // NOTE(allen):
  // bits    | meaning
  // [0:7]   | prolog length
  // [8:29]  | function length
  // [30:30] | instructions_are_32bits (otherwise they are 16 bits)
  // [31:31] | has_exception_handler
  U32 combined;
};

typedef struct PE_IntelPdata PE_IntelPdata;
struct PE_IntelPdata
{
  U32 voff_first;
  U32 voff_one_past_last;
  U32 voff_unwind_info;
};

#define PE_CODEVIEW_PDB20_MAGIC 0x3031424e
#define PE_CODEVIEW_PDB70_MAGIC 0x53445352

typedef struct PE_CvHeaderPDB20 PE_CvHeaderPDB20;
struct PE_CvHeaderPDB20
{
  U32 magic;
  U32 offset;
  U32 time;
  U32 age;
  // file name packed after struct
};

typedef struct PE_CvHeaderPDB70 PE_CvHeaderPDB70;
struct PE_CvHeaderPDB70
{
  U32 magic;
  OS_Guid guid;
  U32 age;
  // file name packed after struct
};

typedef struct PE_ImportEntry PE_ImportEntry;
struct PE_ImportEntry
{
  U32 lookup_table_voff;
  COFF_TimeStamp time_stamp;
  U32 forwarder_chain;
  U32 name_voff;
  U32 import_addr_table_voff;
};

typedef struct PE_DelayedImportEntry PE_DelayedImportEntry;
struct PE_DelayedImportEntry
{
  // According to PE/COFF spec this field is unused and should be set zero,
  // but when I compile mule with MSVC 2019 this is set to 1.
  U32 attributes;
  U32 name_voff;                       // Name of the DLL
  U32 module_handle_voff;              // Place where module handle from LoadLibrary is stored
  U32 iat_voff;
  U32 name_table_voff;                 // Array of hint/name or oridnals
  U32 bound_table_voff;                // (Optional) Points to an array of PE_ThunkData
  U32 unload_table_voff;               // (Optional) Copy of iat_voff
  //  0 not bound
  // -1 if bound and real timestamp located in bounded import directory
  // Otherwise time when dll was bound
  COFF_TimeStamp time_stamp; 
};

typedef struct PE_ExportTable PE_ExportTable;
struct PE_ExportTable
{
  U32 flags;                       // must be zero
  COFF_TimeStamp time_stamp;       // time and date when export table was created
  U16 major_ver;                   // table version, user can change major and minor version
  U16 minor_ver; 
  U32 name_voff;                   // ASCII name of the dll
  U32 ordinal_base;                // Starting oridnal number
  U32 export_address_table_count;
  U32 name_pointer_table_count;
  U32 export_address_table_voff;
  U32 name_pointer_table_voff;
  U32 ordinal_table_voff;
};

typedef struct PE_TLSHeader32 PE_TLSHeader32;
struct PE_TLSHeader32
{
  U32 raw_data_start;    // Range of initialized data that is copied for each thread from the image.
  U32 raw_data_end;      // (Typically points to .tls section)
  U32 index_address;     // Address where image loader places TLS slot index.
  U32 callbacks_address; // Zero terminated list of callbacks used for initializing data with constructors.
  U32 zero_fill_size;    // Amount of memory to fill with zeroes in TLS.
  U32 characteristics;   // COFF_SectionFlags but only align flags are used.
};

typedef struct PE_TLSHeader64 PE_TLSHeader64;
struct PE_TLSHeader64
{
  U64 raw_data_start;    // Range of initialized data that is copied for each thread from the image.
  U64 raw_data_end;      // (Typically points to .tls section)
  U64 index_address;     // Address where image loader places TLS slot index.
  U64 callbacks_address; // Zero terminated list of callbacks used for initializing data with constructors.
  U32 zero_fill_size;    // Amount of memory to fill with zeroes in TLS.
  U32 characteristics;   // COFF_SectionFlags but only align flags are used.
};

#pragma pack(pop)

////////////////////////////////
//~ rjf: Parsed Info Types

typedef struct PE_BinInfo PE_BinInfo;
struct PE_BinInfo
{
  U64 image_base;
  U64 entry_point;
  B32 is_pe32;
  U64 virt_section_align;
  U64 file_section_align;
  U64 section_array_off;
  U64 section_count;
  U64 symbol_array_off;
  U64 symbol_count;
  U64 string_table_off;
  U64 dbg_path_off;
  U64 dbg_path_size;
  OS_Guid dbg_guid;
  U32 dbg_age;
  U32 dbg_time;
  Architecture arch;
  Rng1U64 *data_dir_franges;
  U32 data_dir_count;
  PE_TLSHeader64 tls_header;
};

////////////////////////////////
//~ rjf: Parsed Info Extraction Helper Types

typedef U16 PE_BaseRelocKind;
enum
{
  PE_BaseRelocKind_ABSOLUTE            = 0, // No reallocation is applied. Can be used as padding.
  PE_BaseRelocKind_HIGH                = 1,
  PE_BaseRelocKind_LOW                 = 2,
  PE_BaseRelocKind_HIGHLOW             = 3,
  PE_BaseRelocKind_HIGHADJ             = 4,
  PE_BaseRelocKind_MIPS_JMPADDR        = 5,
  PE_BaseRelocKind_ARM_MOV32           = 5,
  PE_BaseRelocKind_RISCV_HIGH20        = 5,
  // 6 is reserved
  PE_BaseRelocKind_THUMB_MOV32         = 7,
  PE_BaseRelocKind_RISCV_LOW12I        = 7,
  PE_BaseRelocKind_RISCV_LOW12S        = 8,
  PE_BaseRelocKind_LOONGARCH32_MARK_LA = 8,
  PE_BaseRelocKind_LOONGARCH64_MARK_LA = 8,
  PE_BaseRelocKind_MIPS_JMPADDR16      = 9,
  PE_BaseRelocKind_DIR64               = 10,
};
#define PE_BaseRelocOffsetFromEntry(x) ((x) & 0x1fff)
#define PE_BaseRelocKindFromEntry(x)   (((x) >> 12) & 0xf)
#define PE_BaseRelocMake(k, off)       ((((U16)(k) & 0xf) << 12) | (U16)((off) & 0x1fff))

typedef struct PE_BaseRelocBlock PE_BaseRelocBlock;
struct PE_BaseRelocBlock
{
  U64 page_virt_off;
  U64 entry_count;
  U16 *entries;
};

typedef struct PE_BaseRelocBlockNode PE_BaseRelocBlockNode;
struct PE_BaseRelocBlockNode
{
  PE_BaseRelocBlockNode *next;
  PE_BaseRelocBlock v;
};

typedef struct PE_BaseRelocBlockList PE_BaseRelocBlockList;
struct PE_BaseRelocBlockList
{
  PE_BaseRelocBlockNode *first;
  PE_BaseRelocBlockNode *last;
  U64 count;
};

////////////////////////////////
//~ Resources

#define PE_RES_ALIGN 4u

read_only U8 PE_RES_MAGIC[] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

enum
{
  PE_Resource_CURSOR       = 0x1,
  PE_Resource_BITMAP       = 0x2,
  PE_Resource_ICON         = 0x3,
  PE_Resource_MENU         = 0x4,
  PE_Resource_DIALOG       = 0x5,
  PE_Resource_STRING       = 0x6,
  PE_Resource_FONTDIR      = 0x7,
  PE_Resource_FONT         = 0x8,
  PE_Resource_ACCELERATOR  = 0x9,
  PE_Resource_RCDATA       = 0xA,
  PE_Resource_MESSAGETABLE = 0xB,
  PE_Resource_GROUP_CURSOR = 0xC,
  PE_Resource_GROUP_ICON   = 0xE,
  PE_Resource_VERSION      = 0x10,
  PE_Resource_DLGINCLUDE   = 0x11,
  PE_Resource_PLUGPLAY     = 0x13,
  PE_Resource_VXD          = 0x14,
  PE_Resource_ANICURSOR    = 0x15,
  PE_Resource_ANIICON      = 0x16,
  PE_Resource_HTML         = 0x17,
  PE_Resource_MANIFEST     = 0x18,
  PE_Resource_BITMAP_NEW   = 0x2002,
  PE_Resource_MENU_NEW     = 0x2004,
  PE_Resource_DIALOG_NEW   = 0x2005,
};
typedef U32 PE_ResourceType;

#pragma pack(push, 1)
typedef struct PE_ResourceHeader
{
  COFF_ResourceHeaderPrefix prefix;
  U16 type;
  U16 pad0;
  U16 name;
  U16 pad1;
  U32 data_version;
  COFF_ResourceMemoryFlags memory_flags;
  U16 language_id;
  U32 version;
  U32 characteristics;
} PE_ResourceHeader;
#pragma pack(pop)

typedef enum
{
  PE_ResData_NULL,
  PE_ResData_DIR,
  PE_ResData_COFF_LEAF,
  PE_ResData_COFF_RESOURCE,
} PE_ResDataType;

typedef struct PE_Resource
{
  COFF_ResourceID id;
  PE_ResDataType type;
  union {
    COFF_ResourceDataEntry leaf;
    struct PE_ResourceDir *dir;
    struct {
      COFF_ResourceID type;
      U32 data_version;
      U32 version;
      COFF_ResourceMemoryFlags memory_flags;
      String8 data; 
    } coff_res;
  } u;
} PE_Resource;

typedef struct PE_ResourceNode
{
  struct PE_ResourceNode *next;
  PE_Resource data;
} PE_ResourceNode;

typedef struct PE_ResourceList
{
  U64 count;
  PE_ResourceNode *first;
  PE_ResourceNode *last;
} PE_ResourceList;

typedef struct PE_ResourceArray
{
  U64 count;
  PE_Resource *v;
} PE_ResourceArray;

typedef struct PE_ResourceDir
{
  U32 characteristics;
  COFF_TimeStamp time_stamp;
  U16 major_version;
  U16 minor_version;
  PE_ResourceList named_list;
  PE_ResourceList id_list;
} PE_ResourceDir;

////////////////////////////////
//~ rjf: Parser Functions

internal PE_BinInfo pe_bin_info_from_data(Arena *arena, String8 data);

////////////////////////////////
//~ rjf: Helpers

internal U64 pe_intel_pdata_off_from_voff__binary_search(String8 data, PE_BinInfo *bin, U64 voff);
internal void *pe_ptr_from_voff(String8 data, PE_BinInfo *bin, U64 voff);
internal U64 pe_section_num_from_voff(String8 data, PE_BinInfo *bin, U64 voff);
internal void *pe_ptr_from_section_num(String8 data, PE_BinInfo *bin, U64 n);
internal U64 pe_foff_from_voff(String8 data, PE_BinInfo *bin, U64 voff);
internal PE_BaseRelocBlockList pe_base_reloc_block_list_from_bin(Arena *arena, String8 data, PE_BinInfo *bin);
internal Rng1U64 pe_tls_rng_from_bin_base_vaddr(String8 data, PE_BinInfo *bin, U64 base_vaddr);

////////////////////////////////
//~ Resource Helpers

internal B32               pe_is_res(String8 data);
internal void              pe_resource_dir_push_res_file(Arena *arena, PE_ResourceDir *root_dir, String8 res_file);
internal PE_ResourceNode * pe_resource_dir_push_dir_node(Arena *arena, PE_ResourceDir *dir, COFF_ResourceID id, U32 characteristics, COFF_TimeStamp time_stamp, U16 major_version, U16 minor_version);
internal PE_ResourceNode * pe_resource_dir_push_entry_node(Arena *arena, PE_ResourceDir *dir, COFF_ResourceID id, COFF_ResourceID type, U32 data_version, U32 version, COFF_ResourceMemoryFlags memory_flags, String8 data);
internal PE_Resource *     pe_resource_dir_push_entry(Arena *arena, PE_ResourceDir *dir, COFF_ResourceID id, COFF_ResourceID type, U32 data_version, U32 version, COFF_ResourceMemoryFlags memory_flags, String8 data);
internal PE_Resource *     pe_resource_dir_push_dir(Arena *arena, PE_ResourceDir *dir, COFF_ResourceID id, U32 characteristics, COFF_TimeStamp time_stamp, U16 major_version, U16 minor_version);
internal PE_ResourceNode * pe_resource_dir_search_node(PE_ResourceDir *dir, COFF_ResourceID id);
internal PE_Resource *     pe_resource_dir_search(PE_ResourceDir *dir, COFF_ResourceID id);
internal PE_ResourceArray  pe_resource_list_to_array(Arena *arena, PE_ResourceList *list);
internal PE_ResourceDir *  pe_resource_table_from_directory_data(Arena *arena, String8 data);

////////////////////////////////

internal String8 pe_get_dos_program(void);

////////////////////////////////

internal String8 pe_string_from_subsystem(PE_WindowsSubsystem subsystem);

#endif //PE_H
