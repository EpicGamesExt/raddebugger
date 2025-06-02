// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef COFF_H
#define COFF_H

////////////////////////////////
//~ rjf: COFF Format Types

read_only global U8 g_coff_big_header_magic[] =
{
  0xc7, 0xa1, 0xba, 0xd1, 0xee, 0xba, 0xa9, 0x4b, 0xaf, 0x20, 0xfa, 0xf6, 0x6a, 0xa4, 0xdc, 0xb8
};
read_only global U8 g_coff_res_magic[] =
{
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
read_only global U8 g_coff_archive_sig[8]      = "!<arch>\n";
read_only global U8 g_coff_thin_archive_sig[8] = "!<thin>\n";

#pragma pack(push, 1)

typedef U32 COFF_TimeStamp;

typedef U16 COFF_FileHeaderFlags;
enum
{
  COFF_FileHeaderFlag_RelocStripped        = (1 << 0),
  COFF_FileHeaderFlag_ExecutableImage      = (1 << 1),
  COFF_FileHeaderFlag_LineNumbersStripped  = (1 << 2),
  COFF_FileHeaderFlag_SymbolsStripped      = (1 << 3),
  COFF_FileHeaderFlag_Reserved0            = (1 << 4),
  COFF_FileHeaderFlag_LargeAddressAware    = (1 << 5),
  COFF_FileHeaderFlag_Reserved1            = (1 << 6),
  COFF_FileHeaderFlag_Reserved2            = (1 << 7),
  COFF_FileHeaderFlag_32BitMachine         = (1 << 8),
  COFF_FileHeaderFlag_DebugStripped        = (1 << 9),
  COFF_FileHeaderFlag_RemovableRunFromSwap = (1 << 10),
  COFF_FileHeaderFlag_NetRunFromSwap       = (1 << 11),
  COFF_FileHeaderFlag_System               = (1 << 12),
  COFF_FileHeaderFlag_Dll                  = (1 << 13),
  COFF_FileHeaderFlag_UpSystemOnly         = (1 << 14),
  COFF_FileHeaderFlag_BytesReservedHi      = (1 << 15)
};

typedef U16 COFF_MachineType;
enum
{
  COFF_MachineType_Unknown    = 0x0,
  COFF_MachineType_X86        = 0x14c,
  COFF_MachineType_X64        = 0x8664,
  COFF_MachineType_Am33       = 0x1d3,
  COFF_MachineType_Arm        = 0x1c0,
  COFF_MachineType_Arm64      = 0xaa64,
  COFF_MachineType_ArmNt      = 0x1c4,
  COFF_MachineType_Ebc        = 0xebc,
  COFF_MachineType_Ia64       = 0x200,
  COFF_MachineType_M32R       = 0x9041,
  COFF_MachineType_Mips16     = 0x266,
  COFF_MachineType_MipsFpu    = 0x366,
  COFF_MachineType_MipsFpu16  = 0x466,
  COFF_MachineType_PowerPc    = 0x1f0,
  COFF_MachineType_PowerPcFp  = 0x1f1,
  COFF_MachineType_R4000      = 0x166,
  COFF_MachineType_RiscV32    = 0x5032,
  COFF_MachineType_RiscV64    = 0x5064,
  COFF_MachineType_RiscV128   = 0x5128,
  COFF_MachineType_Sh3        = 0x1a2,
  COFF_MachineType_Sh3Dsp     = 0x1a3,
  COFF_MachineType_Sh4        = 0x1a6,
  COFF_MachineType_Sh5        = 0x1a8,
  COFF_MachineType_Thumb      = 0x1c2,
  COFF_MachineType_WceMipsV2  = 0x169
};

typedef struct COFF_FileHeader
{
  COFF_MachineType     machine;
  U16                  section_count;
  COFF_TimeStamp       time_stamp;
  U32                  symbol_table_foff;
  U32                  symbol_count;
  U16                  optional_header_size;
  COFF_FileHeaderFlags flags;
} COFF_FileHeader;

typedef struct COFF_BigObjHeader
{
  U16              sig1;              // COFF_MachineType_Unknown
  U16              sig2;              // max_U16
  U16              version;           // 2
  COFF_MachineType machine;
  COFF_TimeStamp   time_stamp;
  U8               magic[16];         // g_coff_big_header_magic
  U8               unused[16];
  U32              section_count;
  U32              symbol_table_foff;
  U32              symbol_count;
} COFF_BigObjHeader;

typedef U32 COFF_SectionAlign;
enum
{
  COFF_SectionAlign_1Bytes    = 0x1,
  COFF_SectionAlign_2Bytes    = 0x2,
  COFF_SectionAlign_4Bytes    = 0x3,
  COFF_SectionAlign_8Bytes    = 0x4,
  COFF_SectionAlign_16Bytes   = 0x5,
  COFF_SectionAlign_32Bytes   = 0x6,
  COFF_SectionAlign_64Bytes   = 0x7,
  COFF_SectionAlign_128Bytes  = 0x8,
  COFF_SectionAlign_256Bytes  = 0x9,
  COFF_SectionAlign_512Bytes  = 0xa,
  COFF_SectionAlign_1024Bytes = 0xb,
  COFF_SectionAlign_2048Bytes = 0xc,
  COFF_SectionAlign_4096Bytes = 0xd,
  COFF_SectionAlign_8192Bytes = 0xe
};

typedef U32 COFF_SectionFlags;
enum
{
  COFF_SectionFlag_TypeNoPad            = (1 << 3),
  COFF_SectionFlag_CntCode              = (1 << 5),
  COFF_SectionFlag_CntInitializedData   = (1 << 6),
  COFF_SectionFlag_CntUninitializedData = (1 << 7),
  COFF_SectionFlag_LnkOther             = (1 << 8),
  COFF_SectionFlag_LnkInfo              = (1 << 9),
  COFF_SectionFlag_LnkRemove            = (1 << 11),
  COFF_SectionFlag_LnkCOMDAT            = (1 << 12),
  COFF_SectionFlag_GpRel                = (1 << 15),
  COFF_SectionFlag_Mem16Bit             = (1 << 17),
  COFF_SectionFlag_MemLocked            = (1 << 18),
  COFF_SectionFlag_MemPreload           = (1 << 19),
  COFF_SectionFlag_AlignShift           = 20,
  COFF_SectionFlag_AlignMask            = 0xf,
  COFF_SectionFlag_LnkNRelocOvfl        = (1 << 24),
  COFF_SectionFlag_MemDiscardable       = (1 << 25),
  COFF_SectionFlag_MemNotCached         = (1 << 26),
  COFF_SectionFlag_MemNotPaged          = (1 << 27),
  COFF_SectionFlag_MemShared            = (1 << 28),
  COFF_SectionFlag_MemExecute           = (1 << 29),
  COFF_SectionFlag_MemRead              = (1 << 30),
  COFF_SectionFlag_MemWrite             = (1 << 31)
};
#define COFF_SectionFlags_ExtractAlign(f) (COFF_SectionAlign)(((f) >> COFF_SectionFlag_AlignShift) & COFF_SectionFlag_AlignMask)
#define COFF_SectionFlags_LnkFlags        ((COFF_SectionFlag_AlignMask << COFF_SectionFlag_AlignShift) | COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_LnkInfo | COFF_SectionFlag_LnkOther | COFF_SectionFlag_LnkRemove | COFF_SectionFlag_LnkNRelocOvfl)

typedef struct COFF_SectionHeader
{
  U8                name[8];
  U32               vsize;
  U32               voff;
  U32               fsize;
  U32               foff;
  U32               relocs_foff;
  U32               lines_foff;
  U16               reloc_count;
  U16               line_count;
  COFF_SectionFlags flags;
} COFF_SectionHeader;

////////////////////////////////

typedef U8 COFF_SymType;
enum
{
  COFF_SymType_Null,
  COFF_SymType_Void,
  COFF_SymType_Char,
  COFF_SymType_Short,
  COFF_SymType_Int,
  COFF_SymType_Long,
  COFF_SymType_Float,
  COFF_SymType_Double,
  COFF_SymType_Struct,
  COFF_SymType_Union,
  COFF_SymType_Enum,
  COFF_SymType_MemberOfEnumeration,
  COFF_SymType_Byte,
  COFF_SymType_Word,
  COFF_SymType_UInt,
  COFF_SymType_DWord
};

typedef U8 COFF_SymStorageClass;
enum
{
  COFF_SymStorageClass_Null            = 0x00,
  COFF_SymStorageClass_Automatic       = 0x01,
  COFF_SymStorageClass_External        = 0x02,
  COFF_SymStorageClass_Static          = 0x03,
  COFF_SymStorageClass_Register        = 0x04,
  COFF_SymStorageClass_ExternalDef     = 0x05,
  COFF_SymStorageClass_Label           = 0x06,
  COFF_SymStorageClass_UndefinedLabel  = 0x07,
  COFF_SymStorageClass_MemberOfStruct  = 0x08,
  COFF_SymStorageClass_Argument        = 0x09,
  COFF_SymStorageClass_StructTag       = 0x0a,
  COFF_SymStorageClass_MemberOfUnion   = 0x0b,
  COFF_SymStorageClass_UnionTag        = 0x0c,
  COFF_SymStorageClass_TypeDefinition  = 0x0d,
  COFF_SymStorageClass_UndefinedStatic = 0x0e,
  COFF_SymStorageClass_EnumTag         = 0x0f,
  COFF_SymStorageClass_MemberOfEnum    = 0x10,
  COFF_SymStorageClass_RegisterParam   = 0x11,
  COFF_SymStorageClass_BitField        = 0x12,
  COFF_SymStorageClass_Block           = 0x64,
  COFF_SymStorageClass_Function        = 0x65,
  COFF_SymStorageClass_EndOfStruct     = 0x66,
  COFF_SymStorageClass_File            = 0x67,
  COFF_SymStorageClass_Section         = 0x68,
  COFF_SymStorageClass_WeakExternal    = 0x69,
  COFF_SymStorageClass_CLRToken        = 0x6b,
  COFF_SymStorageClass_EndOfFunction   = 0xff
};

typedef U8 COFF_SymDType;
enum
{
  COFF_SymDType_Null  = 0x00,
  COFF_SymDType_Ptr   = 0x10,
  COFF_SymDType_Func  = 0x20,
  COFF_SymDType_Array = 0x30
};

// Special values for section number field in coff symbol
#define COFF_Symbol_UndefinedSection 0
#define COFF_Symbol_AbsSection32     ((U32)-1)
#define COFF_Symbol_DebugSection32   ((U32)-2)
#define COFF_Symbol_AbsSection16     ((U16)-1)
#define COFF_Symbol_DebugSection16   ((U16)-2)

typedef union COFF_SymbolName
{
  U8 short_name[8];
  struct {
    // if this field is filled with zeroes we have a long name,
    // which means name is stored in the string table
    // and we need to use the offset to look it up...
    U32 zeroes;
    U32 string_table_offset;
  } long_name;
} COFF_SymbolName;

#define COFF_SymbolType_IsFunc(x) ((x).u.lsb == COFF_SymType_Null && (x).u.msb == COFF_SymDType_Func)

typedef union COFF_SymbolType
{
  struct {
    COFF_SymDType msb;
    COFF_SymType lsb;
  } u;
  U16 v;
} COFF_SymbolType;

typedef struct COFF_Symbol16
{
  COFF_SymbolName      name;
  U32                  value;
  U16                  section_number;
  COFF_SymbolType      type;
  COFF_SymStorageClass storage_class;
  U8                   aux_symbol_count;
} COFF_Symbol16;

typedef struct COFF_Symbol32
{
  COFF_SymbolName      name;
  U32                  value;
  U32                  section_number;
  COFF_SymbolType      type;
  COFF_SymStorageClass storage_class;
  U8                   aux_symbol_count;
} COFF_Symbol32;

// Auxilary symbols are allocated with fixed size so that symbol table could be maintaned as array of regular size.
#define COFF_AuxSymbolSize 18

typedef U32 COFF_WeakExtType;
enum
{
  COFF_WeakExt_NoLibrary     = 1,
  COFF_WeakExt_SearchLibrary = 2,
  COFF_WeakExt_SearchAlias   = 3
};

// storage class: External
typedef struct COFF_SymbolFuncDef
{
  U32 tag_index;
  U32 total_size;
  U32 ptr_to_ln;
  U32 ptr_to_next_func;
  U8  unused[2];
} COFF_SymbolFuncDef;

// storage class: Function
typedef struct COFF_SymbolFunc
{
  U8  unused[4];
  U16 ln;
  U8  unused2[2];
  U32 ptr_to_next_func;
  U8  unused3[2];
} COFF_SymbolFunc;

// storage class: WeakExternal
typedef struct COFF_SymbolWeakExt
{
  U32              tag_index;
  COFF_WeakExtType characteristics;
  U8               unused[10];
} COFF_SymbolWeakExt;

typedef struct COFF_SymbolFile 
{
  U8 name[18];
} COFF_SymbolFile;

typedef U8 COFF_ComdatSelectType;
enum
{
  COFF_ComdatSelect_Null         = 0, 
  COFF_ComdatSelect_NoDuplicates = 1, // Only one symbol is allowed to be in global symbol table, otherwise multiply defintion error is thrown.
  COFF_ComdatSelect_Any          = 2, // Select any symbol, even if there are multiple definitions. (we default to first declaration)
  COFF_ComdatSelect_SameSize     = 3, // Sections that symbols reference must match in size, otherwise multiply definition error is thrown.
  COFF_ComdatSelect_ExactMatch   = 4, // Sections that symbols reference must have identical checksums, otherwise multiply defintion error is thrown.
  COFF_ComdatSelect_Associative  = 5, // Symbols with associative type form a chain of sections are related to each other. (next link is indicated in COFF_SecDef in 'number')
  COFF_ComdatSelect_Largest      = 6  // Linker selects section with largest size.
};

// provides information about section to which symbol refers to.
// storage class: Static
typedef struct COFF_SymbolSecDef
{
  U32                   length;
  U16                   number_of_relocations;
  U16                   number_of_ln;
  U32                   check_sum;
  U16                   number_lo;  // low 16 bits of one-based section index
  COFF_ComdatSelectType selection;
  U8                    unused;
  U16                   number_hi;
} COFF_SymbolSecDef;

////////////////////////////////

typedef U16 COFF_RelocType;

typedef COFF_RelocType COFF_Reloc_X64;
enum
{
  COFF_Reloc_X64_Abs      = 0x0,
  COFF_Reloc_X64_Addr64   = 0x1,
  COFF_Reloc_X64_Addr32   = 0x2,
  COFF_Reloc_X64_Addr32Nb = 0x3,  // NB => No Base
  COFF_Reloc_X64_Rel32    = 0x4,
  COFF_Reloc_X64_Rel32_1  = 0x5,
  COFF_Reloc_X64_Rel32_2  = 0x6,
  COFF_Reloc_X64_Rel32_3  = 0x7,
  COFF_Reloc_X64_Rel32_4  = 0x8,
  COFF_Reloc_X64_Rel32_5  = 0x9,
  COFF_Reloc_X64_Section  = 0xA,
  COFF_Reloc_X64_SecRel   = 0xB,
  COFF_Reloc_X64_SecRel7  = 0xC,  // TODO(nick): MSDN doesn't specify size for CLR token
  COFF_Reloc_X64_Token    = 0xD,
  COFF_Reloc_X64_SRel32   = 0xE,  // TODO(nick): MSDN doesn't specify size for PAIR
  COFF_Reloc_X64_Pair     = 0xF,
  COFF_Reloc_X64_SSpan32  = 0x10
};

typedef COFF_RelocType COFF_Reloc_X86;
enum
{
  COFF_Reloc_X86_Abs      = 0x0,  //  relocation is ignored
  COFF_Reloc_X86_Dir16    = 0x1,  //  no support
  COFF_Reloc_X86_Rel16    = 0x2,  //  no support
  COFF_Reloc_X86_Unknown0 = 0x3,
  COFF_Reloc_X86_Unknown2 = 0x4,
  COFF_Reloc_X86_Unknown3 = 0x5,
  COFF_Reloc_X86_Dir32    = 0x6,  //  32-bit virtual address
  COFF_Reloc_X86_Dir32Nb  = 0x7,  //  32-bit virtual offset
  COFF_Reloc_X86_Seg12    = 0x9,  //  no support
  COFF_Reloc_X86_Section  = 0xa,  //  16-bit section index, used for debug info purposes
  COFF_Reloc_X86_SecRel   = 0xb,  //  32-bit offset from start of a section
  COFF_Reloc_X86_Token    = 0xc,  //  CLR token? (for managed languages)
  COFF_Reloc_X86_SecRel7  = 0xd,  //  7-bit offset from the base of the section that contains the target.
  COFF_Reloc_X86_Unknown4 = 0xe,
  COFF_Reloc_X86_Unknown5 = 0xf,
  COFF_Reloc_X86_Unknown6 = 0x10,
  COFF_Reloc_X86_Unknown7 = 0x11,
  COFF_Reloc_X86_Unknown8 = 0x12,
  COFF_Reloc_X86_Unknown9 = 0x13,
  COFF_Reloc_X86_Rel32    = 0x14
};

typedef COFF_RelocType COFF_Reloc_Arm;
enum
{
  COFF_Reloc_Arm_Abs           = 0x0,
  COFF_Reloc_Arm_Addr32        = 0x1,
  COFF_Reloc_Arm_Addr32Nb      = 0x2,
  COFF_Reloc_Arm_Branch24      = 0x3,
  COFF_Reloc_Arm_Branch11      = 0x4,
  COFF_Reloc_Arm_Unknown1      = 0x5,
  COFF_Reloc_Arm_Unknown2      = 0x6,
  COFF_Reloc_Arm_Unknown3      = 0x7,
  COFF_Reloc_Arm_Unknown4      = 0x8,
  COFF_Reloc_Arm_Unknown5      = 0x9,
  COFF_Reloc_Arm_Rel32         = 0xa,
  COFF_Reloc_Arm_Section       = 0xe,
  COFF_Reloc_Arm_SecRel        = 0xf,
  COFF_Reloc_Arm_Mov32         = 0x10,
  COFF_Reloc_Arm_ThumbMov32    = 0x11,
  COFF_Reloc_Arm_ThumbBranch20 = 0x12,
  COFF_Reloc_Arm_Unused        = 0x13,
  COFF_Reloc_Arm_ThumbBranch24 = 0x14,
  COFF_Reloc_Arm_ThumbBlx23    = 0x15,
  COFF_Reloc_Arm_Pair          = 0x16
};

typedef COFF_RelocType COFF_Reloc_Arm64;
enum
{
  COFF_Reloc_Arm64_Abs           = 0x0,
  COFF_Reloc_Arm64_Addr32        = 0x1,
  COFF_Reloc_Arm64_Addr32Nb      = 0x2,
  COFF_Reloc_Arm64_Branch26      = 0x3,
  COFF_Reloc_Arm64_PageBaseRel21 = 0x4,
  COFF_Reloc_Arm64_Rel21         = 0x5,
  COFF_Reloc_Arm64_PageOffset12a = 0x6,
  COFF_Reloc_Arm64_SecRel        = 0x8,
  COFF_Reloc_Arm64_SecRelLow12a  = 0x9,
  COFF_Reloc_Arm64_SecRelHigh12a = 0xa,
  COFF_Reloc_Arm64_SecRelLow12l  = 0xb,
  COFF_Reloc_Arm64_Token         = 0xc,
  COFF_Reloc_Arm64_Section       = 0xd,
  COFF_Reloc_Arm64_Addr64        = 0xe,
  COFF_Reloc_Arm64_Branch19      = 0xf,
  COFF_Reloc_Arm64_Branch14      = 0x10,
  COFF_Reloc_Arm64_Rel32         = 0x11
};

typedef struct COFF_Reloc
{
  U32            apply_off; // section relative offset where relocation is placed
  U32            isymbol;   // zero based index into coff symbol table
  COFF_RelocType type;      // relocation type that depends on the arch
} COFF_Reloc;

////////////////////////////////

#define COFF_ResourceAlign 4u

typedef struct COFF_ResourceHeaderPrefix
{
  U32 data_size;
  U32 header_size;
} COFF_ResourceHeaderPrefix;

typedef U16 COFF_ResourceMemoryFlags;
enum
{
  COFF_ResourceMemoryFlag_Moveable    = 0x10,
  COFF_ResourceMemoryFlag_Pure        = 0x20,
  COFF_ResourceMemoryFlag_PreLoad     = 0x40,
  COFF_ResourceMemoryFlag_Discardable = 0x1000
};

typedef struct COFF_ResourceDataEntry
{
  U32 data_voff;
  U32 data_size;
  U32 code_page;
  U32 reserved;
} COFF_ResourceDataEntry;

typedef struct COFF_ResourceDirTable
{
  U32            characteristics;
  COFF_TimeStamp time_stamp;
  U16            major_version;
  U16            minor_version;
  U16            name_entry_count;
  U16            id_entry_count;
} COFF_ResourceDirTable;

#define COFF_Resource_SubDirFlag (1u << 31u)
typedef struct COFF_ResourceDirEntry
{
  union {
    U32 offset;
    U32 id;
  } name;
  union {
    U32 data_entry_offset;
    U32 sub_dir_offset;
  } id;
} COFF_ResourceDirEntry;

////////////////////////////////

#define COFF_Archive_MemberAlign      2
#define COFF_Archive_MaxShortNameSize 15

typedef struct COFF_ArchiveMemberHeader
{
  U8 name[16];
  U8 date[12];
  U8 user_id[6];
  U8 group_id[6];
  U8 mode[8];
  U8 size[10];
  U8 end[2];
} COFF_ArchiveMemberHeader;

#define COFF_ImportType_Invalid max_U16
typedef U16 COFF_ImportType;
enum
{
  COFF_ImportHeader_Code  = 0,
  COFF_ImportHeader_Data  = 1,
  COFF_ImportHeader_Const = 2
};

typedef U32 COFF_ImportByType;
enum
{
  COFF_ImportBy_Ordinal      = 0,
  COFF_ImportBy_Name         = 1,
  COFF_ImportBy_NameNoPrefix = 2,
  COFF_ImportBy_Undecorate   = 3
};

typedef U16 COFF_ImportHeaderFlags;
enum
{
  COFF_ImportHeader_TypeShift = 0,
  COFF_ImportHeader_TypeMask  = 3,
  COFF_ImportHeader_ImportByShift = 2,
  COFF_ImportHeader_ImportByMask  = 3,
};
#define COFF_ImportHeader_ExtractType(x)     (((x) >> COFF_ImportHeader_TypeShift    ) & COFF_ImportHeader_TypeMask    )
#define COFF_ImportHeader_ExtractImportBy(x) (((x) >> COFF_ImportHeader_ImportByShift) & COFF_ImportHeader_ImportByMask)

typedef struct COFF_ImportHeader
{
  U16                    sig1;     // COFF_MachineType_Unknown
  U16                    sig2;     // max_U16
  U16                    version;  // 0
  COFF_MachineType       machine;
  COFF_TimeStamp         time_stamp;
  U32                    data_size;
  U16                    hint_or_ordinal;
  COFF_ImportHeaderFlags flags;
  // char *func_name;
  // char *dll_name;
} COFF_ImportHeader;

#pragma pack(pop)

////////////////////////////////
// Section

internal U64               coff_align_size_from_section_flags(COFF_SectionFlags flags);
internal COFF_SectionFlags coff_section_flag_from_align_size (U64 align);

internal String8 coff_name_from_section_header(String8 string_table, COFF_SectionHeader *header);
internal void    coff_parse_section_name      (String8 full_name, String8 *name_out, String8 *postfix_out);

////////////////////////////////
// Symbol

internal String8 coff_read_symbol_name(String8 string_table, COFF_SymbolName *name);

////////////////////////////////
// Reloc

internal U64 coff_apply_size_from_reloc_x64(COFF_Reloc_X64 x);
internal U64 coff_apply_size_from_reloc_x86(COFF_Reloc_X86 x);

////////////////////////////////
// Import

internal U32 coff_make_ordinal32(U16 hint);
internal U64 coff_make_ordinal64(U16 hint);

internal String8 coff_make_import_lookup           (Arena *arena, U16 hint, String8 name);
internal String8 coff_make_import_header_by_name   (Arena *arena, String8 dll_name, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 name, U16 hint, COFF_ImportType type);
internal String8 coff_make_import_header_by_ordinal(Arena *arena, String8 dll_name, COFF_MachineType machine, COFF_TimeStamp time_stamp, U16 ordinal, COFF_ImportType type);

////////////////////////////////
// Misc

internal U64 coff_word_size_from_machine       (COFF_MachineType machine);
internal U64 coff_default_exe_base_from_machine(COFF_MachineType machine);
internal U64 coff_default_dll_base_from_machine(COFF_MachineType machine);

internal Arch arch_from_coff_machine(COFF_MachineType machine);
internal U64  coff_foff_from_voff(COFF_SectionHeader *sections, U64 section_count, U64 voff);

#endif // COFF_H
