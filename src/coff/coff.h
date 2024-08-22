// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef COFF_H
#define COFF_H

typedef U32 COFF_TimeStamp;

////////////////////////////////
//~ rjf: Coff Format Types

#pragma pack(push,1)

typedef struct COFF_Guid COFF_Guid;
struct COFF_Guid
{
  U32 data1;
  U16 data2;
  U16 data3;
  U32 data4;
  U32 data5;
};

typedef U16 COFF_Flags;
enum
{
  COFF_Flag_RELOC_STRIPPED             = (1 << 0),
  COFF_Flag_EXECUTABLE_IMAGE           = (1 << 1),
  COFF_Flag_LINE_NUMS_STRIPPED         = (1 << 2),
  COFF_Flag_SYM_STRIPPED               = (1 << 3),
  COFF_Flag_RESERVED_0                 = (1 << 4),
  COFF_Flag_LARGE_ADDRESS_AWARE        = (1 << 5),
  COFF_Flag_RESERVED_1                 = (1 << 6),
  COFF_Flag_RESERVED_2                 = (1 << 7),
  COFF_Flag_32BIT_MACHINE              = (1 << 8),
  COFF_Flag_DEBUG_STRIPPED             = (1 << 9),
  COFF_Flag_REMOVABLE_RUN_FROM_SWAP    = (1 << 10),
  COFF_Flag_NET_RUN_FROM_SWAP          = (1 << 11),
  COFF_Flag_SYSTEM                     = (1 << 12),
  COFF_Flag_DLL                        = (1 << 13),
  COFF_Flag_UP_SYSTEM_ONLY             = (1 << 14),
  COFF_Flag_BYTES_RESERVED_HI          = (1 << 15),
};

typedef U16 COFF_MachineType;
enum
{
  COFF_MachineType_UNKNOWN             = 0x0,
  COFF_MachineType_X86                 = 0x14c,
  COFF_MachineType_X64                 = 0x8664,
  COFF_MachineType_AM33                = 0x1d3,
  COFF_MachineType_ARM                 = 0x1c0,
  COFF_MachineType_ARM64               = 0xaa64,
  COFF_MachineType_ARMNT               = 0x1c4,
  COFF_MachineType_EBC                 = 0xebc,
  COFF_MachineType_IA64                = 0x200,
  COFF_MachineType_M32R                = 0x9041,
  COFF_MachineType_MIPS16              = 0x266,
  COFF_MachineType_MIPSFPU             = 0x366,
  COFF_MachineType_MIPSFPU16           = 0x466,
  COFF_MachineType_POWERPC             = 0x1f0,
  COFF_MachineType_POWERPCFP           = 0x1f1,
  COFF_MachineType_R4000               = 0x166,
  COFF_MachineType_RISCV32             = 0x5032,
  COFF_MachineType_RISCV64             = 0x5064,
  COFF_MachineType_RISCV128            = 0x5128,
  COFF_MachineType_SH3                 = 0x1a2,
  COFF_MachineType_SH3DSP              = 0x1a3,
  COFF_MachineType_SH4                 = 0x1a6,
  COFF_MachineType_SH5                 = 0x1a8,
  COFF_MachineType_THUMB               = 0x1c2,
  COFF_MachineType_WCEMIPSV2           = 0x169,
  COFF_MachineType_COUNT               = 25
};

typedef struct COFF_Header COFF_Header;
struct COFF_Header
{
  COFF_MachineType machine;
  U16 section_count;
  COFF_TimeStamp time_stamp;
  U32 symbol_table_foff;
  U32 symbol_count;
  U16 optional_header_size;
  COFF_Flags flags;
};

typedef U32 COFF_SectionAlign;
enum
{
  COFF_SectionAlign_1BYTES             = 0x1,
  COFF_SectionAlign_2BYTES             = 0x2,
  COFF_SectionAlign_4BYTES             = 0x3,
  COFF_SectionAlign_8BYTES             = 0x4,
  COFF_SectionAlign_16BYTES            = 0x5,
  COFF_SectionAlign_32BYTES            = 0x6,
  COFF_SectionAlign_64BYTES            = 0x7,
  COFF_SectionAlign_128BYTES           = 0x8,
  COFF_SectionAlign_256BYTES           = 0x9,
  COFF_SectionAlign_512BYTES           = 0xA,
  COFF_SectionAlign_1024BYTES          = 0xB,
  COFF_SectionAlign_2048BYTES          = 0xC,
  COFF_SectionAlign_4096BYTES          = 0xD,
  COFF_SectionAlign_8192BYTES          = 0xE,
  COFF_SectionAlign_COUNT              = 14
};

typedef U32 COFF_SectionFlags;
enum
{
  COFF_SectionFlag_TYPE_NO_PAD              = (1 << 3),
  COFF_SectionFlag_CNT_CODE                 = (1 << 5),
  COFF_SectionFlag_CNT_INITIALIZED_DATA     = (1 << 6),
  COFF_SectionFlag_CNT_UNINITIALIZED_DATA   = (1 << 7),
  COFF_SectionFlag_LNK_OTHER                = (1 << 8),
  COFF_SectionFlag_LNK_INFO                 = (1 << 9),
  COFF_SectionFlag_LNK_REMOVE               = (1 << 11),
  COFF_SectionFlag_LNK_COMDAT               = (1 << 12),
  COFF_SectionFlag_GPREL                    = (1 << 15),
  COFF_SectionFlag_MEM_16BIT                = (1 << 17),
  COFF_SectionFlag_MEM_LOCKED               = (1 << 18),
  COFF_SectionFlag_MEM_PRELOAD              = (1 << 19),
  COFF_SectionFlag_ALIGN_SHIFT = 20, COFF_SectionFlag_ALIGN_MASK = 0xf,
  COFF_SectionFlag_LNK_NRELOC_OVFL          = (1 << 24),
  COFF_SectionFlag_MEM_DISCARDABLE          = (1 << 25),
  COFF_SectionFlag_MEM_NOT_CACHED           = (1 << 26),
  COFF_SectionFlag_MEM_NOT_PAGED            = (1 << 27),
  COFF_SectionFlag_MEM_SHARED               = (1 << 28),
  COFF_SectionFlag_MEM_EXECUTE              = (1 << 29),
  COFF_SectionFlag_MEM_READ                 = (1 << 30),
  COFF_SectionFlag_MEM_WRITE                = (1 << 31),
};
#define COFF_SectionFlags_Extract_ALIGN(f) (COFF_SectionAlign)(((f) >> COFF_SectionFlag_ALIGN_SHIFT) & COFF_SectionFlag_ALIGN_MASK)
#define COFF_SectionFlags_LNK_FLAGS    ((COFF_SectionFlag_ALIGN_MASK << COFF_SectionFlag_ALIGN_SHIFT) | COFF_SectionFlag_LNK_COMDAT | COFF_SectionFlag_LNK_INFO | COFF_SectionFlag_LNK_OTHER | COFF_SectionFlag_LNK_REMOVE | COFF_SectionFlag_LNK_NRELOC_OVFL)

typedef struct COFF_SectionHeader COFF_SectionHeader;
struct COFF_SectionHeader
{
  U8 name[8];
  U32 vsize;
  U32 voff;
  U32 fsize;
  U32 foff;
  U32 relocs_foff;
  U32 lines_foff;
  U16 reloc_count;
  U16 line_count;
  COFF_SectionFlags flags;
};

typedef U16 COFF_RelocTypeX64;
enum
{
  COFF_RelocTypeX64_ABS                = 0x0,
  COFF_RelocTypeX64_ADDR64             = 0x1,
  COFF_RelocTypeX64_ADDR32             = 0x2,
  COFF_RelocTypeX64_ADDR32NB           = 0x3,
  // NB => No Base
  COFF_RelocTypeX64_REL32              = 0x4,
  COFF_RelocTypeX64_REL32_1            = 0x5,
  COFF_RelocTypeX64_REL32_2            = 0x6,
  COFF_RelocTypeX64_REL32_3            = 0x7,
  COFF_RelocTypeX64_REL32_4            = 0x8,
  COFF_RelocTypeX64_REL32_5            = 0x9,
  COFF_RelocTypeX64_SECTION            = 0xA,
  COFF_RelocTypeX64_SECREL             = 0xB,
  COFF_RelocTypeX64_SECREL7            = 0xC,
  // TODO(nick): MSDN doesn't specify size for CLR token
  COFF_RelocTypeX64_TOKEN              = 0xD,
  COFF_RelocTypeX64_SREL32             = 0xE,
  // TODO(nick): MSDN doesn't specify size for PAIR
  COFF_RelocTypeX64_PAIR               = 0xF,
  COFF_RelocTypeX64_SSPAN32            = 0x10,
  COFF_RelocTypeX64_COUNT              = 17
};

typedef U16 COFF_RelocTypeX86;
enum
{
  COFF_RelocTypeX86_ABS                     = 0x0,
  //  relocation is ignored
  COFF_RelocTypeX86_DIR16                   = 0x1,
  //  no support
  COFF_RelocTypeX86_REL16                   = 0x2,
  //  no support
  COFF_RelocTypeX86_UNKNOWN0                = 0x3,
  COFF_RelocTypeX86_UNKNOWN2                = 0x4,
  COFF_RelocTypeX86_UNKNOWN3                = 0x5,
  COFF_RelocTypeX86_DIR32                   = 0x6,
  //  32-bit virtual address
  COFF_RelocTypeX86_DIR32NB                 = 0x7,
  //  32-bit virtual offset
  COFF_RelocTypeX86_SEG12                   = 0x9,
  //  no support
  COFF_RelocTypeX86_SECTION                 = 0xA,
  //  16-bit section index, used for debug info purposes
  COFF_RelocTypeX86_SECREL                  = 0xB,
  //  32-bit offset from start of a section
  COFF_RelocTypeX86_TOKEN                   = 0xC,
  //  CLR token? (for managed languages)
  COFF_RelocTypeX86_SECREL7                 = 0xD,
  //  7-bit offset from the base of the section that contains the target.
  COFF_RelocTypeX86_UNKNOWN4                = 0xE,
  COFF_RelocTypeX86_UNKNOWN5                = 0xF,
  COFF_RelocTypeX86_UNKNOWN6                = 0x10,
  COFF_RelocTypeX86_UNKNOWN7                = 0x11,
  COFF_RelocTypeX86_UNKNOWN8                = 0x12,
  COFF_RelocTypeX86_UNKNOWN9                = 0x13,
  COFF_RelocTypeX86_REL32                   = 0x14,
  COFF_RelocTypeX86_COUNT                   = 20
};

typedef U16 COFF_RelocTypeARM;
enum
{
  COFF_RelocTypeARM_ABS                     = 0x0,
  COFF_RelocTypeARM_ADDR32                  = 0x1,
  COFF_RelocTypeARM_ADDR32NB                = 0x2,
  COFF_RelocTypeARM_BRANCH24                = 0x3,
  COFF_RelocTypeARM_BRANCH11                = 0x4,
  COFF_RelocTypeARM_UNKNOWN1                = 0x5,
  COFF_RelocTypeARM_UNKNOWN2                = 0x6,
  COFF_RelocTypeARM_UNKNOWN3                = 0x7,
  COFF_RelocTypeARM_UNKNOWN4                = 0x8,
  COFF_RelocTypeARM_UNKNOWN5                = 0x9,
  COFF_RelocTypeARM_REL32                   = 0xA,
  COFF_RelocTypeARM_SECTION                 = 0xE,
  COFF_RelocTypeARM_SECREL                  = 0xF,
  COFF_RelocTypeARM_MOV32                   = 0x10,
  COFF_RelocTypeARM_THUMB_MOV32             = 0x11,
  COFF_RelocTypeARM_THUMB_BRANCH20          = 0x12,
  COFF_RelocTypeARM_UNUSED                  = 0x13,
  COFF_RelocTypeARM_THUMB_BRANCH24          = 0x14,
  COFF_RelocTypeARM_THUMB_BLX23             = 0x15,
  COFF_RelocTypeARM_PAIR                    = 0x16,
  COFF_RelocTypeARM_COUNT                   = 20
};

typedef U16 COFF_RelocTypeARM64;
enum
{
  COFF_RelocTypeARM64_ABS                   = 0x0,
  COFF_RelocTypeARM64_ADDR32                = 0x1,
  COFF_RelocTypeARM64_ADDR32NB              = 0x2,
  COFF_RelocTypeARM64_BRANCH26              = 0x3,
  COFF_RelocTypeARM64_PAGEBASE_REL21        = 0x4,
  COFF_RelocTypeARM64_REL21                 = 0x5,
  COFF_RelocTypeARM64_PAGEOFFSET_12A        = 0x6,
  COFF_RelocTypeARM64_SECREL                = 0x8,
  COFF_RelocTypeARM64_SECREL_LOW12A         = 0x9,
  COFF_RelocTypeARM64_SECREL_HIGH12A        = 0xA,
  COFF_RelocTypeARM64_SECREL_LOW12L         = 0xB,
  COFF_RelocTypeARM64_TOKEN                 = 0xC,
  COFF_RelocTypeARM64_SECTION               = 0xD,
  COFF_RelocTypeARM64_ADDR64                = 0xE,
  COFF_RelocTypeARM64_BRANCH19              = 0xF,
  COFF_RelocTypeARM64_BRANCH14              = 0x10,
  COFF_RelocTypeARM64_REL32                 = 0x11,
  COFF_RelocTypeARM64_COUNT                 = 17
};

typedef U8 COFF_SymType;
enum
{
  COFF_SymType_NULL,
  COFF_SymType_VOID,
  COFF_SymType_CHAR,
  COFF_SymType_SHORT,
  COFF_SymType_INT,
  COFF_SymType_LONG,
  COFF_SymType_FLOAT,
  COFF_SymType_DOUBLE,
  COFF_SymType_STRUCT,
  COFF_SymType_UNION,
  COFF_SymType_ENUM,
  COFF_SymType_MOE,
  //  member of enumeration
  COFF_SymType_BYTE,
  COFF_SymType_WORD,
  COFF_SymType_UINT,
  COFF_SymType_DWORD,
  COFF_SymType_COUNT = 16
};

typedef U8 COFF_SymStorageClass;
enum
{
  COFF_SymStorageClass_END_OF_FUNCTION      = 0xff,
  COFF_SymStorageClass_NULL                 = 0,
  COFF_SymStorageClass_AUTOMATIC            = 1,
  COFF_SymStorageClass_EXTERNAL             = 2,
  COFF_SymStorageClass_STATIC               = 3,
  COFF_SymStorageClass_REGISTER             = 4,
  COFF_SymStorageClass_EXTERNAL_DEF         = 5,
  COFF_SymStorageClass_LABEL                = 6,
  COFF_SymStorageClass_UNDEFINED_LABEL      = 7,
  COFF_SymStorageClass_MEMBER_OF_STRUCT     = 8,
  COFF_SymStorageClass_ARGUMENT             = 9,
  COFF_SymStorageClass_STRUCT_TAG           = 10,
  COFF_SymStorageClass_MEMBER_OF_UNION      = 11,
  COFF_SymStorageClass_UNION_TAG            = 12,
  COFF_SymStorageClass_TYPE_DEFINITION      = 13,
  COFF_SymStorageClass_UNDEFINED_STATIC     = 14,
  COFF_SymStorageClass_ENUM_TAG             = 15,
  COFF_SymStorageClass_MEMBER_OF_ENUM       = 16,
  COFF_SymStorageClass_REGISTER_PARAM       = 17,
  COFF_SymStorageClass_BIT_FIELD            = 18,
  COFF_SymStorageClass_BLOCK                = 100,
  COFF_SymStorageClass_FUNCTION             = 101,
  COFF_SymStorageClass_END_OF_STRUCT        = 102,
  COFF_SymStorageClass_FILE                 = 103,
  COFF_SymStorageClass_SECTION              = 104,
  COFF_SymStorageClass_WEAK_EXTERNAL        = 105,
  COFF_SymStorageClass_CLR_TOKEN            = 107,
  COFF_SymStorageClass_COUNT                = 27
};

typedef U16 COFF_SymSecNumber;
enum
{
  COFF_SymSecNumber_NUMBER_UNDEFINED        = 0,
  COFF_SymSecNumber_ABSOLUTE                = 0xffff,
  COFF_SymSecNumber_DEBUG                   = 0xfffe,
  COFF_SymSecNumber_COUNT                   = 3
};

typedef U8 COFF_SymDType;
enum
{
  COFF_SymDType_NULL                        = 0,
  COFF_SymDType_PTR                         = 16,
  COFF_SymDType_FUNC                        = 32,
  COFF_SymDType_ARRAY                       = 48,
  COFF_SymDType_COUNT                       = 4
};

typedef U32 COFF_WeakExtType;
enum
{
  COFF_WeakExtType_NOLIBRARY                = 1,
  COFF_WeakExtType_SEARCH_LIBRARY           = 2,
  COFF_WeakExtType_SEARCH_ALIAS             = 3,
  COFF_WeakExtType_COUNT                    = 3
};

typedef U32 COFF_ImportHeaderType;
enum
{
  COFF_ImportHeaderType_CODE                = 0,
  COFF_ImportHeaderType_DATA                = 1,
  COFF_ImportHeaderType_CONST               = 2,
  COFF_ImportHeaderType_COUNT               = 3
};

typedef U32 COFF_ImportHeaderNameType;
enum
{
  COFF_ImportHeaderNameType_ORDINAL         = 0,
  COFF_ImportHeaderNameType_NAME            = 1,
  COFF_ImportHeaderNameType_NAME_NOPREFIX   = 2,
  COFF_ImportHeaderNameType_UNDECORATE      = 3,
  COFF_ImportHeaderNameType_COUNT           = 4
};

#define COFF_IMPORT_HEADER_TYPE_MASK  0x03
#define COFF_IMPORT_HEADER_TYPE_SHIFT 0
#define COFF_IMPORT_HEADER_NAME_TYPE_MASK  0x1c
#define COFF_IMPORT_HEADER_NAME_TYPE_SHIFT 2
#define COFF_IMPORT_HEADER_GET_TYPE(x)      (((x) & COFF_IMPORT_HEADER_TYPE_MASK) >> COFF_IMPORT_HEADER_TYPE_SHIFT)
#define COFF_IMPORT_HEADER_GET_NAME_TYPE(x) (((x) & COFF_IMPORT_HEADER_NAME_TYPE_MASK) >> COFF_IMPORT_HEADER_NAME_TYPE_SHIFT)
typedef struct COFF_ImportHeader
{
  U16 sig1;
  U16 sig2;
  U16 version;
  U16 machine;
  COFF_TimeStamp time_stamp;
  U32 data_size;
  U16 hint;
  U16 type;
  U16 name_type;
  //  type : 2
  //  name type : 3
  //  reserved : 11
  //U16 flags;
  String8 func_name;
  String8 dll_name;
} COFF_ImportHeader;

typedef U8 COFF_ComdatSelectType;
enum
{
  COFF_ComdatSelectType_NULL                = 0,
  //  Only one symbol is allowed to be in global symbol table, otherwise multiply defintion error is thrown.
  COFF_ComdatSelectType_NODUPLICATES        = 1,
  //  Select any symbol, even if there are multiple definitions. (we default to first declaration)
  COFF_ComdatSelectType_ANY                 = 2,
  //  Sections that symbols reference must match in size, otherwise multiply definition error is thrown.
  COFF_ComdatSelectType_SAME_SIZE           = 3,
  //  Sections that symbols reference must have identical checksums, otherwise multiply defintion error is thrown.
  COFF_ComdatSelectType_EXACT_MATCH         = 4,
  //  Symbols with associative type form a chain of sections are related to each other. (next link is indicated in COFF_SecDef in 'number')
  COFF_ComdatSelectType_ASSOCIATIVE         = 5,
  //  Linker selects section with largest size.
  COFF_ComdatSelectType_LARGEST             = 6,
  COFF_ComdatSelectType_COUNT               = 7
};

#define COFF_MIN_BIG_OBJ_VERSION 2

global U8 coff_big_obj_magic[] =
{
  0xC7,0xA1,0xBA,0xD1,0xEE,0xBA,0xA9,0x4B,
  0xAF,0x20,0xFA,0xF6,0x6A,0xA4,0xDC,0xB8,
};

typedef struct COFF_HeaderBigObj COFF_HeaderBigObj;
struct COFF_HeaderBigObj
{
  U16 sig1; // COFF_MachineType_UNKNOWN
  U16 sig2; // U16_MAX
  U16 version;
  U16 machine;
  U32 time_stamp;
  U8 magic[16];
  U32 unused[4];
  U32 section_count;
  U32 pointer_to_symbol_table;
  U32 number_of_symbols;
};

// Special values for section number field in coff symbol
#define COFF_SYMBOL_UNDEFINED_SECTION 0
#define COFF_SYMBOL_ABS_SECTION      ((U32)-1)
#define COFF_SYMBOL_DEBUG_SECTION    ((U32)-2)
#define COFF_SYMBOL_ABS_SECTION_16   ((U16)-1)
#define COFF_SYMBOL_DEBUG_SECTION_16 ((U16)-2)

typedef union COFF_SymbolName COFF_SymbolName;
union COFF_SymbolName
{
  U8 short_name[8];
  struct
  {
    // if this field is filled with zeroes we have a long name,
    // which means name is stored in the string table 
    // and we need to use the offset to look it up...
    U32 zeroes;
    U32 string_table_offset;
  }long_name;
};

typedef struct COFF_Symbol16 COFF_Symbol16;
struct COFF_Symbol16
{
  COFF_SymbolName name;
  U32 value;
  U16 section_number;
  union
  {
    struct
    {
      COFF_SymDType msb;
      COFF_SymType  lsb;
    }u;
    U16 v;
  }type;
  COFF_SymStorageClass storage_class;
  U8 aux_symbol_count;
};

typedef struct COFF_Symbol32 COFF_Symbol32;
struct COFF_Symbol32
{
  COFF_SymbolName name;
  U32 value;
  U32 section_number;
  union
  {
    struct
    {
      COFF_SymDType msb;
      COFF_SymType  lsb;
    }u;
    U16 v;
  }type;
  COFF_SymStorageClass storage_class;
  U8 aux_symbol_count;
};

// Auxilary symbols are allocated with fixed size so that symbol table could be maintaned as array of regular size.
#define COFF_AUX_SYMBOL_SIZE 18

// storage class: FUNCTION
typedef struct COFF_SymbolFunc
{
  U8 unused[4];
  U16 ln;
  U8 unused2[2];
  U32 ptr_to_next_func;
  U8 unused3[2];
} COFF_SymbolFunc;

// storage class: WEAK_EXTERNAL
typedef struct COFF_SymbolWeakExt
{
  U32 tag_index;
  U32 characteristics;
  U8 unused[10];
} COFF_SymbolWeakExt;

typedef struct COFF_SymbolFile
{
  char name[18];
} COFF_SymbolFile;

// provides information about section to which symbol refers to.
// storage class: STATIC
typedef struct COFF_SymbolSecDef
{
  U32 length;
  U16 number_of_relocations;
  U16 number_of_ln;
  U32 check_sum;
  U16 number; // one-based section index
  U8 selection;
  U8 unused[3];
} COFF_SymbolSecDef;

// specifies how section data should be modified when placed in the image file.
typedef struct COFF_Reloc COFF_Reloc;
struct COFF_Reloc
{
  U32 apply_off; // section relative offset where relocation is placed
  U32 isymbol;   // zero based index into coff symbol table
  U16 type;      // relocation type that depends on the arch
};

#pragma pack(pop)

// feature flags in absolute symbol @feat.00
enum
{
  COFF_FeatFlag_HAS_SAFE_SEH  = (1 << 0),  // /safeseh
  COFF_FeatFlag_UNKNOWN_4     = (1 << 4),
  COFF_FeatFlag_GUARD_STACK   = (1 << 8),  // /GS
  COFF_FeatFlag_SDL           = (1 << 9),  // /sdl
  COFF_FeatFlag_GUARD_CF      = (1 << 11), // /guard:cf
  COFF_FeatFlag_GUARD_EH_CONT = (1 << 14), // /guard:ehcont
  COFF_FeatFlag_NO_RTTI       = (1 << 17), // /GR-
  COFF_FeatFlag_KERNEL        = (1 << 30), // /kernel
};
typedef U32 COFF_FeatFlags;

////////////////////////////////

#define COFF_RES_ALIGN 4u

typedef struct COFF_ResourceHeaderPrefix
{
  U32 data_size;
  U32 header_size;
} COFF_ResourceHeaderPrefix;

typedef U16 COFF_ResourceMemoryFlags;
enum
{
  COFF_ResourceMemoryFlag_MOVEABLE    = 0x10,
  COFF_ResourceMemoryFlag_PURE        = 0x20,
  COFF_ResourceMemoryFlag_PRELOAD     = 0x40,
  COFF_ResourceMemoryFlag_DISCARDABLE = 0x1000,
};

typedef enum
{
  COFF_ResourceIDType_NULL,
  COFF_ResourceIDType_NUMBER,
  COFF_ResourceIDType_STRING,
  COFF_ResourceIDType_COUNT
} COFF_ResourceIDType;

typedef struct COFF_ResourceID_16
{
  COFF_ResourceIDType type;
  union
  {
    U16 number;
    String16 string;
  } u;
} COFF_ResourceID_16;

typedef struct COFF_ResourceID
{
  COFF_ResourceIDType type;
  union
  {
    U16 number;
    String8 string;
  } u;
} COFF_ResourceID;

typedef struct COFF_Resource
{
  COFF_ResourceID type;
  COFF_ResourceID name;
  U16 language_id;
  U32 data_version;
  U32 version;
  COFF_ResourceMemoryFlags memory_flags;
  String8 data;
} COFF_Resource;

typedef struct COFF_ResourceDataEntry
{
  U32 data_voff;
  U32 data_size;
  U32 code_page;
  U32 reserved;
} COFF_ResourceDataEntry;

typedef struct COFF_ResourceDirTable
{
  U32 characteristics;
  COFF_TimeStamp time_stamp;
  U16 major_version;
  U16 minor_version;
  U16 name_entry_count;
  U16 id_entry_count;
} COFF_ResourceDirTable;

#define COFF_RESOURCE_SUB_DIR_FLAG (1u << 31u)
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

// !<arch>\n
#define COFF_ARCHIVE_SIG 0x0A3E686372613C21ULL
// !<thin>\n
#define COFF_THIN_ARCHIVE_SIG 0xA3E6E6968743C21ULL

#define COFF_ARCHIVE_MAX_SHORT_NAME_SIZE 15

#define COFF_ARCHIVE_ALIGN 2

typedef struct COFF_ArchiveMemberHeader
{
  String8 name;       // padded to 16 bytes with spaces
  U32 date;           // unix time
  U32 user_id;        // unix artifact that does not have meaning on windows
  U32 group_id;       // unix artifact that does not have meaning on windows
  String8 mode;       // octal representation the members file mode
  U32 size;           // size of the member data, not including header
  B32 is_end_correct; // set to true if found correct signature after header
} COFF_ArchiveMemberHeader;

////////////////////////////////
// Helpers

enum
{
  COFF_DataType_NULL,
  COFF_DataType_BIG_OBJ,
  COFF_DataType_OBJ,
  COFF_DataType_IMPORT,
};
typedef U32 COFF_DataType;

typedef struct COFF_HeaderInfo
{
  COFF_MachineType machine;
  U64 section_array_off;
  U64 section_count_no_null;
  U64 string_table_off;
  U64 symbol_size;
  U64 symbol_off;
  U64 symbol_count;
} COFF_HeaderInfo;

enum
{
  // symbol has section and offset.
  COFF_SymbolValueInterp_REGULAR,
  
  // symbol is overridable
  COFF_SymbolValueInterp_WEAK,
  
  // symbol doesn't have a reference section.
  COFF_SymbolValueInterp_UNDEFINED,
  
  // symbol has no section but still has size.
  COFF_SymbolValueInterp_COMMON,
  
  // symbol has an absolute (non-relocatable) value and is not an address.
  COFF_SymbolValueInterp_ABS,
  
  // symbol is used to provide general type of debugging information.
  COFF_SymbolValueInterp_DEBUG
};
typedef U32 COFF_SymbolValueInterpType;

typedef struct COFF_Symbol16Node
{
  struct COFF_Symbol16Node *next;
  COFF_Symbol16 data;
} COFF_Symbol16Node;

typedef struct COFF_Symbol16List
{
  U64 count;
  COFF_Symbol16Node *first;
  COFF_Symbol16Node *last;
} COFF_Symbol16List;

typedef struct COFF_Symbol32Array
{
  U64 count;
  COFF_Symbol32 *v;
} COFF_Symbol32Array;

typedef struct COFF_RelocNode
{
  struct COFF_RelocNode *next;
  COFF_Reloc data;
} COFF_RelocNode;

typedef struct COFF_RelocList
{
  U64 count;
  COFF_RelocNode *first;
  COFF_RelocNode *last;
} COFF_RelocList;

typedef struct COFF_RelocArray
{
  U64 count;
  COFF_Reloc *v;
} COFF_RelocArray;

typedef struct COFF_RelocInfo
{
  U64 array_off;
  U64 count;
} COFF_RelocInfo;

typedef struct COFF_ResourceNode
{
  struct COFF_ResourceNode *next;
  COFF_Resource data;
} COFF_ResourceNode;

typedef struct COFF_ResourceList
{
  U64 count;
  COFF_ResourceNode *first;
  COFF_ResourceNode *last;
} COFF_ResourceList;

////////////////////////////////

typedef struct COFF_ArchiveMember
{
  COFF_ArchiveMemberHeader header;
  U64 offset;
  String8 data;
} COFF_ArchiveMember;

typedef struct COFF_ArchiveFirstMember
{
  U32 symbol_count;
  String8 member_offsets;
  String8 string_table;
} COFF_ArchiveFirstMember;

typedef struct COFF_ArchiveSecondMember
{
  U32 member_count;
  U32 symbol_count;
  String8 member_offsets;
  String8 symbol_indices;
  String8 string_table;
} COFF_ArchiveSecondMember;

typedef struct COFF_ArchiveMemberNode
{
  struct COFF_ArchiveMemberNode *next;
  COFF_ArchiveMember data;
} COFF_ArchiveMemberNode;

typedef struct COFF_ArchiveMemberList
{
  U64 count;
  COFF_ArchiveMemberNode *first;
  COFF_ArchiveMemberNode *last;
} COFF_ArchiveMemberList;

typedef enum
{
  COFF_Archive_Null,
  COFF_Archive_Regular,
  COFF_Archive_Thin
} COFF_ArchiveType;

typedef struct COFF_ArchiveParse
{
  COFF_ArchiveFirstMember first_member;
  COFF_ArchiveSecondMember second_member;
  String8 long_names;
} COFF_ArchiveParse;

////////////////////////////////
//~ rjf: Globals

read_only global COFF_SectionHeader coff_section_header_nil = {0};

////////////////////////////////
//~ rjf: Helper Functions

internal B32                        coff_is_big_obj(String8 data);
internal B32                        coff_is_obj(String8 data);
internal COFF_HeaderInfo            coff_header_info_from_data(String8 data);
internal U64                        coff_align_size_from_section_flags(COFF_SectionFlags flags);
internal COFF_SymbolValueInterpType coff_interp_symbol(COFF_Symbol32 *symbol);

internal U64                 coff_foff_from_voff(COFF_SectionHeader *sections, U64 section_count, U64 voff);
internal COFF_SectionHeader *coff_section_header_from_num(String8 data, U64 section_headers_off, U64 n);
internal String8             coff_section_header_get_name(COFF_SectionHeader *header, String8 coff_data, U64 string_table_base);
internal void                coff_parse_section_name(String8 full_name, String8 *name_out, String8 *postfix_out);

internal String8             coff_read_symbol_name(String8 data, U64 string_table_base_offset, COFF_SymbolName *name);
internal void                coff_symbol32_from_coff_symbol16(COFF_Symbol32 *sym32, COFF_Symbol16 *sym16);
internal COFF_Symbol32Array  coff_symbol_array_from_data_16(Arena *arena, String8 data, U64 symbol_array_off, U64 symbol_count);
internal COFF_Symbol32Array  coff_symbol_array_from_data_32(Arena *arena, String8 data, U64 symbol_array_off, U64 symbol_count);
internal COFF_Symbol32Array  coff_symbol_array_from_data(Arena *arena, String8 data, U64 symbol_off, U64 symbol_count, U64 symbol_size);
internal COFF_Symbol16Node * coff_symbol16_list_push(Arena *arena, COFF_Symbol16List *list, COFF_Symbol16 symbol);
internal COFF_RelocInfo      coff_reloc_info_from_section_header(String8 data, COFF_SectionHeader *header);

internal U64     coff_word_size_from_machine(COFF_MachineType machine);
internal String8 coff_make_import_lookup(Arena *arena, U16 hint, String8 name);
internal U32     coff_make_ordinal_32(U16 hint);
internal U64     coff_make_ordinal_64(U16 hint);

internal B32               coff_resource_id_is_equal(COFF_ResourceID a, COFF_ResourceID b);
internal COFF_ResourceID   coff_resource_id_copy(Arena *arena, COFF_ResourceID id);
internal COFF_ResourceID   coff_convert_resource_id(Arena *arena, COFF_ResourceID_16 *id_16);
internal U64               coff_read_resource_id(String8 res, U64 off, COFF_ResourceID_16 *id_out);
internal U64               coff_read_resource(String8 data, U64 off, Arena *arena, COFF_Resource *res_out);
internal COFF_ResourceList coff_resource_list_from_data(Arena *arena, String8 data);

internal COFF_DataType      coff_data_type_from_data(String8 data);
internal B32                coff_is_import(String8 data);
internal B32                coff_is_archive(String8 data);
internal B32                coff_is_thin_archive(String8 data);
internal U64                coff_read_archive_member_header(String8 data, U64 offset, COFF_ArchiveMemberHeader *header_out);
internal COFF_ArchiveMember coff_read_archive_member(String8 data, U64 offset);
internal U64                coff_read_archive_import(String8 data, U64 offset, COFF_ImportHeader *header_out);
internal String8            coff_read_archive_long_name(String8 long_names, String8 name);
internal COFF_ArchiveMember coff_archive_member_from_data(String8 data);
internal U64                coff_archive_member_iter_init(String8 data);
internal B32                coff_archive_member_iter_next(String8 data, U64 *offset, COFF_ArchiveMember *member_out);
internal COFF_ArchiveParse  coff_archive_parse_from_member_list(COFF_ArchiveMemberList list);
internal COFF_ArchiveParse  coff_archive_from_data(Arena *arena, String8 data);
internal U64                coff_thin_archive_member_iter_init(String8 data);
internal B32                coff_thin_archive_member_iter_next(String8 data, U64 *offset, COFF_ArchiveMember *member_out);
internal COFF_ArchiveParse  coff_thin_archive_from_data(Arena *arena, String8 data);
internal COFF_ArchiveType   coff_archive_type_from_data(String8 data);
internal COFF_ArchiveParse  coff_archive_parse_from_data(Arena *arena, String8 data);

internal String8 coff_string_from_comdat_select_type(COFF_ComdatSelectType select);
internal String8 coff_string_from_machine_type(COFF_MachineType machine);
internal String8 coff_string_from_section_flags(Arena *arena, COFF_SectionFlags flags);

#endif //COFF_H
