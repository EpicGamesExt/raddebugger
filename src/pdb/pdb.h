// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PDB_H
#define PDB_H

// https://github.com/microsoft/microsoft-pdb/tree/master/PDB

////////////////////////////////
//~ PDB Format Types

typedef U32 PDB_Version;
enum{
  PDB_Version_VC2      = 19941610,
  PDB_Version_VC4      = 19950623,
  PDB_Version_VC41     = 19950814,
  PDB_Version_VC50     = 19960307,
  PDB_Version_VC98     = 19970604,
  PDB_Version_VC70_DEP = 19990604,
  PDB_Version_VC70     = 20000404,
  PDB_Version_VC80     = 20030901,
  PDB_Version_VC110    = 20091201,
  PDB_Version_VC140    = 20140508
};

typedef U16 PDB_ModIndex;
typedef U32 PDB_StringIndex;

typedef enum PDB_FixedStream{
  PDB_FixedStream_PdbInfo = 1,
  PDB_FixedStream_Tpi = 2,
  PDB_FixedStream_Dbi = 3,
  PDB_FixedStream_Ipi = 4
} PDB_FixedStream;

typedef enum PDB_NamedStream{
  PDB_NamedStream_HEADER_BLOCK,
  PDB_NamedStream_STRTABLE,
  PDB_NamedStream_LINK_INFO,
  PDB_NamedStream_COUNT
} PDB_NamedStream;

typedef struct PDB_InfoHeader{
  PDB_Version version;
  U32 time;
  U32 age;
} PDB_InfoHeader;

enum{
  PDB_StrtblHeader_MAGIC = 0xEFFEEFFE
};

typedef struct PDB_StrtblHeader{
  U32 magic;
  U32 version;
} PDB_StrtblHeader;

////////////////////////////////
//~ PDB Format DBI Types

typedef U32 PDB_DbiStream;
enum{
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
enum{
  PDB_DbiHeaderSignature_V1 = 0xFFFFFFFF
};

typedef U32 PDB_DbiVersion;
enum{
  PDB_DbiVersion_41  =   930803,
  PDB_DbiVersion_50  = 19960307,
  PDB_DbiVersion_60  = 19970606,
  PDB_DbiVersion_70  = 19990903,
  PDB_DbiVersion_110 = 20091201,
};

typedef U16 PDB_DbiBuildNumber;
#define PDB_DbiBuildNumberNewFormatFlag 0x8000
#define PDB_DbiBuildNumberMinor(bn) ((bn)&0xFF)
#define PDB_DbiBuildNumberMajor(bn) (((bn) >> 8)&0x7F)
#define PDB_DbiBuildNumberNewFormat(bn) (!!((bn)&PDB_DbiBuildNumberNewFormatFlag))
#define PDB_DbiBuildNumber(maj, min) \
(PDB_DbiBuildNumberNewFormatFlag | ((min)&0xFF) | (((maj)&0x7F) << 16))

typedef U16 PDB_DbiHeaderFlags;
enum{
  PDB_DbiHeaderFlag_Incremental = 0x1,
  PDB_DbiHeaderFlag_Stripped    = 0x2,
  PDB_DbiHeaderFlag_CTypes      = 0x4
};

typedef struct PDB_DbiHeader{
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

//  (this is not "literally" defined by the format - but helpful to have)
typedef enum PDB_DbiRange{
  PDB_DbiRange_ModuleInfo,
  PDB_DbiRange_SecCon,
  PDB_DbiRange_SecMap,
  PDB_DbiRange_FileInfo,
  PDB_DbiRange_TSM,
  PDB_DbiRange_EcInfo,
  PDB_DbiRange_DbgHeader,
  PDB_DbiRange_COUNT
} PDB_DbiRange;

// "ModuleInfo" DBI range

typedef U32 PDB_DbiSectionContribVersion;
#define PDB_DbiSectionContribVersion_1 (0xeffe0000u + 19970605u)
#define PDB_DbiSectionContribVersion_2 (0xeffe0000u + 20140516u)

typedef struct PDB_DbiSectionContrib40{
  CV_SectionIndex sec;
  U32 sec_off;
  U32 size;
  U32 flags;
  PDB_ModIndex mod;
} PDB_DbiSectionContrib40;

typedef struct PDB_DbiSectionContrib{
  PDB_DbiSectionContrib40 base;
  U32 data_crc;
  U32 reloc_crc;
} PDB_DbiSectionContrib;

typedef struct PDB_DbiSectionContrib2{
  PDB_DbiSectionContrib40 base;
  U32 data_crc;
  U32 reloc_crc;
  U32 sec_coff;
} PDB_DbiSectionContrib2;

typedef struct PDB_DbiCompUnitHeader{
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

//  (this is not "literally" defined by the format - but helpful to have)
typedef enum{
  PDB_DbiCompUnitRange_Symbols,
  PDB_DbiCompUnitRange_C11,
  PDB_DbiCompUnitRange_C13,
  PDB_DbiCompUnitRange_COUNT
} PDB_DbiCompUnitRange;

////////////////////////////////
//~ PDB Format TPI Types

typedef U32 PDB_TpiVersion;
enum{
  PDB_TpiVersion_INTV_VC2 = 920924,
  PDB_TpiVersion_IMPV40 = 19950410,
  PDB_TpiVersion_IMPV41 = 19951122,
  PDB_TpiVersion_IMPV50_INTERIM = 19960307,
  PDB_TpiVersion_IMPV50 = 19961031,
  PDB_TpiVersion_IMPV70 = 19990903,
  PDB_TpiVersion_IMPV80 = 20040203,
};

typedef struct PDB_TpiHeader{
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
  U32 hash_vals_off;
  U32 hash_vals_size;
  U32 itype_off;
  U32 itype_size;
  U32 hash_adj_off;
  U32 hash_adj_size;
} PDB_TpiHeader;

typedef struct PDB_TpiOffHint{
  CV_TypeId itype;
  U32 off;
} PDB_TpiOffHint;


////////////////////////////////
//~ PDB Format GSI Types

typedef U32 PDB_GsiSignature;
enum{
  PDB_GsiSignature_Basic = 0xffffffff,
};

typedef U32 PDB_GsiVersion;
enum{
  PDB_GsiVersion_V70 = 0xeffe0000 + 19990810,
};

typedef struct PDB_GsiHeader{
  PDB_GsiSignature signature;
  PDB_GsiVersion version;
  U32 hr_len;
  U32 num_buckets;
} PDB_GsiHeader;

typedef struct PDB_GsiHashRecord{
  U32 symbol_off;
  U32 cref;
} PDB_GsiHashRecord;

typedef struct PDB_PsiHeader{
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
//~ PDB Parser Types

typedef struct PDB_InfoNode{
  struct PDB_InfoNode *next;
  String8 string;
  MSF_StreamNumber sn;
} PDB_InfoNode;

typedef struct PDB_Info{
  PDB_InfoNode *first;
  PDB_InfoNode *last;
  COFF_Guid auth_guid;
} PDB_Info;

typedef struct PDB_NamedStreamTable{
  MSF_StreamNumber sn[PDB_NamedStream_COUNT];
} PDB_NamedStreamTable;

typedef struct PDB_Strtbl{
  String8 data;
  U32 bucket_count;
  U32 strblock_min;
  U32 strblock_max;
  U32 buckets_min;
  U32 buckets_max;
} PDB_Strtbl;

typedef struct PDB_DbiParsed{
  String8 data;
  COFF_MachineType machine_type;
  MSF_StreamNumber gsi_sn;
  MSF_StreamNumber psi_sn;
  MSF_StreamNumber sym_sn;
  
  U64 range_off[(U64)(PDB_DbiRange_COUNT) + 1];
  MSF_StreamNumber dbg_streams[PDB_DbiStream_COUNT];
} PDB_DbiParsed;

typedef struct PDB_TpiParsed{
  String8 data;
  
  // leaf info
  U64 leaf_first;
  U64 leaf_opl;
  U32 itype_first;
  U32 itype_opl;
  
  // hash info
  MSF_StreamNumber hash_sn;
  MSF_StreamNumber hash_sn_aux;
  U32 hash_key_size;
  U32 hash_bucket_count;
  U32 hash_vals_off;
  U32 hash_vals_size;
  U32 itype_off;
  U32 itype_size;
  U32 hash_adj_off;
  U32 hash_adj_size;
  
} PDB_TpiParsed;

typedef struct PDB_TpiHashBlock{
  struct PDB_TpiHashBlock *next;
  U32 local_count;
  CV_TypeId itypes[13]; // 13 = (64 - 12)/4
} PDB_TpiHashBlock;

typedef struct PDB_TpiHashParsed{
  String8 data;
  String8 aux_data;
  
  PDB_TpiHashBlock **buckets;
  U32 bucket_count;
  U32 bucket_mask;
} PDB_TpiHashParsed;

typedef struct PDB_GsiBucket{
  U32 *offs;
  U64 count;
} PDB_GsiBucket;

typedef struct PDB_GsiParsed{
  PDB_GsiBucket buckets[4096];
} PDB_GsiParsed;

typedef struct PDB_CompUnit{
  MSF_StreamNumber sn;
  U32 range_off[(U32)(PDB_DbiCompUnitRange_COUNT) + 1];
  
  String8 obj_name;
  String8 group_name;
} PDB_CompUnit;

typedef struct PDB_CoffSectionArray{
  COFF_SectionHeader *sections;
  U64 count;
} PDB_CoffSectionArray;

typedef struct PDB_CompUnitNode{
  struct PDB_CompUnitNode *next;
  PDB_CompUnit unit;
} PDB_CompUnitNode;

typedef struct PDB_CompUnitArray{
  PDB_CompUnit **units;
  U64 count;
} PDB_CompUnitArray;

typedef struct PDB_CompUnitContribution{
  U32 mod;
  U64 voff_first;
  U64 voff_opl;
} PDB_CompUnitContribution;

typedef struct PDB_CompUnitContributionArray{
  PDB_CompUnitContribution *contributions;
  U64 count;
} PDB_CompUnitContributionArray;

////////////////////////////////
//~ PDB Parser Functions

internal PDB_Info*            pdb_info_from_data(Arena *arena, String8 pdb_info_data);
internal PDB_NamedStreamTable*pdb_named_stream_table_from_info(Arena *arena, PDB_Info *info);
internal PDB_Strtbl*          pdb_strtbl_from_data(Arena *arena, String8 strtbl_data);

internal PDB_DbiParsed*       pdb_dbi_from_data(Arena *arena, String8 dbi_data);
internal PDB_TpiParsed*       pdb_tpi_from_data(Arena *arena, String8 tpi_data);
internal PDB_TpiHashParsed*   pdb_tpi_hash_from_data(Arena *arena,
                                                     PDB_Strtbl *strtbl,
                                                     PDB_TpiParsed *tpi,
                                                     String8 tpi_hash_data,
                                                     String8 tpi_hash_aux_data);
internal PDB_GsiParsed*       pdb_gsi_from_data(Arena *arena, String8 gsi_data);

internal PDB_CoffSectionArray*pdb_coff_section_array_from_data(Arena *arena,
                                                               String8 section_data);

internal PDB_CompUnitArray*   pdb_comp_unit_array_from_data(Arena *arena,
                                                            String8 module_info_data);

internal PDB_CompUnitContributionArray*
pdb_comp_unit_contribution_array_from_data(Arena *arena, String8 seccontrib_data,
                                           PDB_CoffSectionArray *sections);

////////////////////////////////
//~ PDB Definition Functions

internal U32                  pdb_string_hash1(String8 string);

////////////////////////////////
//~ PDB Dbi Functions

internal String8              pdb_data_from_dbi_range(PDB_DbiParsed *dbi, PDB_DbiRange range);
internal String8              pdb_data_from_unit_range(MSF_Parsed *msf, PDB_CompUnit *unit,
                                                       PDB_DbiCompUnitRange range);

////////////////////////////////
//~ PDB Tpi Functions

internal String8              pdb_leaf_data_from_tpi(PDB_TpiParsed *tpi);

internal CV_TypeIdArray       pdb_tpi_itypes_from_name(Arena *arena,
                                                       PDB_TpiHashParsed *tpi_hash,
                                                       CV_LeafParsed *tpi_leaf,
                                                       String8 name,
                                                       B32 compare_unique_name,
                                                       U32 output_cap);

internal CV_TypeId            pdb_tpi_first_itype_from_name(PDB_TpiHashParsed *tpi_hash,
                                                            CV_LeafParsed *tpi_leaf,
                                                            String8 name,
                                                            B32 compare_unique_name);

////////////////////////////////
//~ PDB Strtbl Functions

internal String8              pdb_strtbl_string_from_off(PDB_Strtbl *strtbl, U32 off);
internal String8              pdb_strtbl_string_from_index(PDB_Strtbl *strtbl,
                                                           PDB_StringIndex idx);

#endif // PDB_H
