// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PDB_H
#define PDB_H

// https://github.com/microsoft/microsoft-pdb/tree/master/PDB

////////////////////////////////
//~ PDB String Table Types

#define PDB_INVALID_STRING_INDEX max_U32
typedef U32 PDB_StringIndex;
typedef U32 PDB_StringOffset;

enum
{
  PDB_StringTableHeader_MAGIC = 0xEFFEEFFE
};

enum
{
  PDB_StringTableHeader_Version1       = 1,
  PDB_StringTableHeader_CurrentVersion = PDB_StringTableHeader_Version1
};

typedef struct PDB_StringTableHeader
{
  U32 magic;
  U32 version;
} PDB_StringTableHeader;

////////////////////////////////

typedef enum PDB_FixedStream
{
  PDB_FixedStream_Info = 1,
  PDB_FixedStream_Tpi  = 2,
  PDB_FixedStream_Dbi  = 3,
  PDB_FixedStream_Ipi  = 4
} PDB_FixedStream;

////////////////////////////////
//~ PDB Info Types

typedef U32 PDB_InfoVersion;
enum{
  PDB_InfoVersion_VC2      = 19941610,
  PDB_InfoVersion_VC4      = 19950623,
  PDB_InfoVersion_VC41     = 19950814,
  PDB_InfoVersion_VC50     = 19960307,
  PDB_InfoVersion_VC98     = 19970604,
  PDB_InfoVersion_VC70_DEP = 19990604,
  PDB_InfoVersion_VC70     = 20000404,
  PDB_InfoVersion_VC80     = 20030901,
  PDB_InfoVersion_VC110    = 20091201,
  PDB_InfoVersion_VC140    = 20140508
};

// referenced in PDB1::loadPdbStream
enum
{
  PDB_FeatureSig_NULL               = 0,
  PDB_FeatureSig_VC110              = PDB_InfoVersion_VC110,
  PDB_FeatureSig_VC140              = PDB_InfoVersion_VC140,
  PDB_FeatureSig_NO_TYPE_MERGE      = 0x4D544F4E,
  PDB_FeatureSig_MINIMAL_DEBUG_INFO = 0x494E494D,
};
typedef U32 PDB_FeatureSig;

enum
{
  PDB_FeatureFlag_HAS_ID_STREAM    = (1 << 0),
  PDB_FeatureFlag_NO_TYPE_MERGE    = (1 << 1),
  PDB_FeatureFlag_MINIMAL_DBG_INFO = (1 << 2),
};
typedef U32 PDB_FeatureFlags;

#pragma pack(push,1)
typedef struct PDB_InfoHeaderV70
{
  PDB_InfoVersion    version;
  COFF_TimeStamp     time_stamp;
  U32                age;
  OS_Guid            guid;
  // PDB_HashTable   named_stream_hash_table
  // PDB_FeatureFlag features[*]
} PDB_InfoHeaderV70;

#pragma pack(pop)
StaticAssert(sizeof(PDB_InfoHeaderV70) == 28, pdb_info_header_v70_size_check);

#define PDB_SRC_HEADER_BLOCK_STREAM_NAME str8_lit("/src/headerblock")
#define PDB_LINK_INFO_STREAM_NAME        str8_lit("/LinkInfo")
#define PDB_NAMES_STREAM_NAME            str8_lit("/names")

////////////////////////////////
// SRC Header Block

#define PDB_SRC_HEADER_BLOCK_MAGIC_V1 19980827

typedef struct PDB_SrcHeaderBlockHeader
{
  U32 version;
  U32 stream_size;
  U64 file_time;
  U32 age;
  U8  pad[44];
} PDB_SrcHeaderBlockHeader;

enum
{
  PDB_SrcComp_NULL,
  PDB_SrcComp_RUN_LENGTH_RECORD,
  PDB_SrcComp_HUFFMAN,
  PDB_SrcComp_LZ,
  PDB_SrcComp_DOTNET
};
typedef U8 PDB_SrcCompType;

enum
{
  PDB_SrcHeaderBlockEntryFlag_IS_VIRTUAL = (1 << 0)
};
typedef U8 PDB_SrcHeaderFlags;

// (PDB/include/pdb.h: SrcHeaderOut)
typedef struct PDB_SrcHeaderBlockEntry
{
  U32                size;
  U32                version;
  U32                file_crc;
  U32                file_size;
  PDB_StringOffset   file_path;
  PDB_StringOffset   obj;
  PDB_StringOffset   virt_path;
  PDB_SrcCompType    comp;
  PDB_SrcHeaderFlags flags;
  U8                 pad[2];
  U8                 reserved[8];
} PDB_SrcHeaderBlockEntry;

////////////////////////////////
//~ PDB Format DBI Types

typedef U32 PDB_DbiStream;
enum
{
  PDB_DbiStream_FPO,
  PDB_DbiStream_EXCEPTION,
  PDB_DbiStream_FIXUP,
  PDB_DbiStream_OMAP_TO_SRC,
  PDB_DbiStream_OMAP_FROM_SRC,
  PDB_DbiStream_SECTION_HEADER,
  PDB_DbiStream_TOKEN_RDI_MAP,
  PDB_DbiStream_XDATA,
  PDB_DbiStream_PDATA,
  PDB_DbiStream_NEW_FPO,
  PDB_DbiStream_SECTION_HEADER_ORIG,
  PDB_DbiStream_COUNT
};

typedef U32 PDB_DbiHeaderSignature;
enum
{
  PDB_DbiHeaderSignature_V1 = 0xFFFFFFFF
};

typedef U32 PDB_DbiVersion;
enum
{
  PDB_DbiVersion_41  =   930803,
  PDB_DbiVersion_50  = 19960307,
  PDB_DbiVersion_60  = 19970606,
  PDB_DbiVersion_70  = 19990903,
  PDB_DbiVersion_110 = 20091201,
};

typedef U16 PDB_DbiBuildNumber;
#define PDB_DbiBuildNumberNewFormatFlag 0x8000
#define PDB_DbiBuildNumberMinor(bn)     ((bn)&0xFF)
#define PDB_DbiBuildNumberMajor(bn)     (((bn) >> 8)&0x7F)
#define PDB_DbiBuildNumberNewFormat(bn) (!!((bn)&PDB_DbiBuildNumberNewFormatFlag))
#define PDB_DbiMakeBuildNumber(maj, min) (PDB_DbiBuildNumber)(PDB_DbiBuildNumberNewFormatFlag | ((min)&0xFF) | (((maj)&0x7F) << 16))

typedef U16 PDB_DbiHeaderFlags;
enum
{
  PDB_DbiHeaderFlag_Incremental = 0x1,
  PDB_DbiHeaderFlag_Stripped    = 0x2,
  PDB_DbiHeaderFlag_CTypes      = 0x4
};

typedef struct PDB_DbiHeader
{
  PDB_DbiHeaderSignature sig;
  PDB_DbiVersion version;
  U32 age;
  MSF_StreamNumber gsi_sn;
  PDB_DbiBuildNumber build_number;
  
  MSF_StreamNumber psi_sn;
  U16 pdb_version;
  
  MSF_StreamNumber sym_sn;
  U16 pdb_version2;
  
  U32 module_info_size;
  U32 sec_con_size;
  U32 sec_map_size;
  U32 file_info_size;
  
  U32 tsm_size;
  U32 mfc_index;
  U32 dbg_header_size;
  U32 ec_info_size;
  
  PDB_DbiHeaderFlags flags;
  COFF_MachineType machine;
  
  U32 reserved;
} PDB_DbiHeader;

// "ModuleInfo" DBI range

typedef U32 PDB_DbiSectionContribVersion;
#define PDB_DbiSectionContribVersion_1 (0xeffe0000u + 19970605u)
#define PDB_DbiSectionContribVersion_2 (0xeffe0000u + 20140516u)

typedef struct PDB_DbiSectionContrib40
{
  CV_SectionIndex sec;
  U16 pad0;
  U32 sec_off;
  U32 size;
  U32 flags;
  CV_ModIndex mod;
  U16 pad1;
} PDB_DbiSectionContrib40;

typedef struct PDB_DbiSectionContrib
{
  PDB_DbiSectionContrib40 base;
  U32 data_crc;
  U32 reloc_crc;
} PDB_DbiSectionContrib;

typedef struct PDB_DbiSectionContrib2
{
  PDB_DbiSectionContrib40 base;
  U32 data_crc;
  U32 reloc_crc;
  U32 sec_coff;
} PDB_DbiSectionContrib2;

typedef struct PDB_DbiCompUnitHeader
{
  U32 unused;
  PDB_DbiSectionContrib contribution;
  U16 flags; // unknown
  
  MSF_StreamNumber sn;
  U32 symbols_size;
  U32 c11_lines_size;
  U32 c13_lines_size;
  
  U16 num_contrib_files;
  U16 unused2;
  U32 file_names_offset;
  
  PDB_StringIndex src_file;
  PDB_StringIndex pdb_file;
  
  // U8[] module_name (null terminated)
  // U8[] obj_name (null terminated)
} PDB_DbiCompUnitHeader;

////////////////////////////////

enum
{
  PDB_DbiOMF_NONE          = 0,
  PDB_DbiOMF_READ          = (1 << 0),
  PDB_DbiOMF_WRITE         = (1 << 1),
  PDB_DbiOMF_EXEC          = (1 << 2),
  PDB_DbiOMF_IS_32BIT_ADDR = (1 << 3), // Descritor is 32-bit address
  PDB_DbiOMF_IS_SELECTOR   = (1 << 8), // Frame is a selector
  PDB_DbiOMF_IS_ABS_ADDR   = (1 << 9), // Frame is absolute address
  PDB_DbiOMF_IS_GROUP      = (1 << 10) // Descriptor is a group
};
typedef U16 PDB_DbiOMF;

typedef struct PDB_DbiSecMapEntry
{
  PDB_DbiOMF flags;
  U16        ovl;
  U16        group;
  U16        frame;
  U16        sec_name;
  U16        class_name;
  U32        offset;
  U32        sec_size;
} PDB_DbiSecMapEntry;

typedef struct PDB_DbiSecMapHeader
{
  U16 section_count;
  U16 segment_count;
} PDB_DbiSecMapHeader;

////////////////////////////////
//~ PDB Format TPI/IPI Types

typedef U32 PDB_TpiVersion;
enum
{
  PDB_TpiVersion_INTV_VC2       = 920924,
  PDB_TpiVersion_IMPV40         = 19950410,
  PDB_TpiVersion_IMPV41         = 19951122,
  PDB_TpiVersion_IMPV50_INTERIM = 19960307,
  PDB_TpiVersion_IMPV50         = 19961031,
  PDB_TpiVersion_IMPV70         = 19990903,
  PDB_TpiVersion_IMPV80         = 20040203,
};

enum
{
  PDB_TYPE_SERVER_HASH_BUCKET_COUNT_V7   = 0x1000,
  PDB_TYPE_SERVER_HASH_BUCKET_COUNT_V8   = 0x3FFF,
  PDB_TYPE_SERVER_HASH_BUCKET_COUNT_INIT = 0x1000,
  PDB_TYPE_SERVER_HASH_BUCKET_COUNT_MAX  = 0x40000,
  
  PDB_TYPE_SERVER_HASH_BUCKET_COUNT_CURRENT = PDB_TYPE_SERVER_HASH_BUCKET_COUNT_V8,
};

#define PDB_TYPE_OFFSET_MAX  max_U32
typedef U32 PDB_TypeOffset;

typedef struct PDB_TpiOffHint
{
  CV_TypeId itype;
  PDB_TypeOffset off;
} PDB_TpiOffHint;

typedef struct PDB_OffsetSize
{
  U32 off;
  U32 size;
} PDB_OffsetSize;

typedef struct PDB_TpiHeader
{
  //   (HDR)
  PDB_TpiVersion version;
  U32 header_size;
  U32 ti_lo;
  U32 ti_hi;
  U32 leaf_data_size;
  
  //   (PdbTpiHash)
  MSF_StreamNumber hash_sn;
  MSF_StreamNumber hash_sn_aux;
  U32 hash_key_size;
  U32 hash_bucket_count;
  PDB_OffsetSize hash_vals;
  PDB_OffsetSize itype_offs;
  PDB_OffsetSize hash_adj;
} PDB_TpiHeader;


////////////////////////////////
//~ PDB Format GSI Types

typedef U32 PDB_GsiSignature;
enum
{
  PDB_GsiSignature_Basic = 0xffffffff,
};

typedef U32 PDB_GsiVersion;
enum
{
  PDB_GsiVersion_V70 = 0xeffe0000 + 19990810,
};

typedef struct PDB_GsiHeader
{
  PDB_GsiSignature signature;
  PDB_GsiVersion version;
  U32 hash_record_arr_size;
  U32 bucket_data_size;
} PDB_GsiHeader;

typedef struct PDB_GsiHashRecord
{
  U32 symbol_off;
  U32 cref;
} PDB_GsiHashRecord;

typedef struct PDB_GsiHashRecordOffsetCalc
{
  U32 next;
  U32 off;
  U32 cref;
} PDB_GsiHashRecordOffsetCalc;

typedef struct PDB_PsiHeader
{
  U32 sym_hash_size;
  U32 addr_map_size;
  U32 thunk_count;
  U32 thunk_size;
  CV_SectionIndex isec_thunk_table;
  U16 padding;
  U32 sec_thunk_table_off;
  U32 sec_count;
} PDB_PsiHeader;

////////////////////////////////

internal U32 pdb_hash_v1(String8 string);

#endif // PDB_H
