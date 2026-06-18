// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_CODEVIEW_H
#define RDI_FROM_CODEVIEW_H

////////////////////////////////
//~ rjf: Conversion Parameters

typedef struct CV2R_CompUnit CV2R_CompUnit;
struct CV2R_CompUnit
{
  String8 obj_name;
  String8 group_name;
  RDIM_Rng1U64ChunkList ranges;
  CV_SymParsed *sym;
  CV_C13Parsed *c13;
};

typedef struct CV2R_Section CV2R_Section;
struct CV2R_Section
{
  String8 name;
  RDI_BinarySectionFlags flags;
  U64 voff;
  U64 vsize;
  U64 foff;
  U64 fsize;
};

typedef struct CV2R_StringTable CV2R_StringTable;
struct CV2R_StringTable
{
  String8 data;
  U32 bucket_count;
  U32 strblock_min;
  U32 strblock_max;
  U32 buckets_min;
  U32 buckets_max;
};

typedef struct CV2R_TPIHashBlock CV2R_TPIHashBlock;
struct CV2R_TPIHashBlock
{
  CV2R_TPIHashBlock *next;
  U32 local_count;
  CV_TypeId itypes[13]; // 13 = (64 - 12)/4
};

typedef struct CV2R_TPIHash CV2R_TPIHash;
struct CV2R_TPIHash
{
  String8 data;
  CV2R_TPIHashBlock **buckets;
  U32 bucket_count;
  U32 bucket_mask;
};

typedef struct CV2R_CompUnitContribution CV2R_CompUnitContribution;
struct CV2R_CompUnitContribution
{
  U32 mod;
  U64 voff_first;
  U64 voff_opl;
};

typedef struct CV2R_ConvertParams CV2R_ConvertParams;
struct CV2R_ConvertParams
{
  // rjf: info extracted from either linker / pdb
  String8 exe_name;
  String8 exe_data;
  Guid guid;
  U64 comp_units_count;
  CV2R_CompUnit *comp_units;
  U64 comp_unit_contributions_count;
  CV2R_CompUnitContribution *comp_unit_contributions;
  U64 sections_count;
  CV2R_Section *sections;
  CV2R_StringTable *strtbl;
  CV_LeafParsed *tpi_leaf;
  CV_LeafParsed *ipi_leaf;
  CV2R_TPIHash *tpi_hash;
  CV2R_TPIHash *ipi_hash;
  
  // rjf: generation params
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

////////////////////////////////
//~ rjf: Conversion Helper Types

//- rjf: link name map (voff -> string)

typedef struct CV2R_LinkNameNode CV2R_LinkNameNode;
struct CV2R_LinkNameNode
{
  CV2R_LinkNameNode *next;
  U64 voff;
  String8 name;
  U64 referencing_symbol_count;
};

typedef struct CV2R_LinkNameMap CV2R_LinkNameMap;
struct CV2R_LinkNameMap
{
  CV2R_LinkNameNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 link_name_count;
};

//- rjf: deduplicated namespace map type

typedef struct CV2R_NamespaceNode CV2R_NamespaceNode;
struct CV2R_NamespaceNode
{
  CV2R_NamespaceNode *next;
  String8 string;
  B32 corresponds_to_scope;
  RDIM_Scope *scope;
  RDIM_Type *type;
  RDIM_Namespace *ns;
};

//- rjf: normalized file path -> source file map

typedef struct CV2R_SrcFileStub CV2R_SrcFileStub;
struct CV2R_SrcFileStub
{
  String8 file_path;
  CV_C13ChecksumKind checksum_kind;
  String8 checksum;
};

typedef struct CV2R_SrcFileStubArray CV2R_SrcFileStubArray;
struct CV2R_SrcFileStubArray
{
  CV2R_SrcFileStub *v;
  U64 count;
};

typedef struct CV2R_SrcFileStubNode CV2R_SrcFileStubNode;
struct CV2R_SrcFileStubNode
{
  CV2R_SrcFileStubNode *next;
  CV2R_SrcFileStub v;
};

typedef struct CV2R_SrcFileNode CV2R_SrcFileNode;
struct CV2R_SrcFileNode
{
  CV2R_SrcFileNode *next;
  RDIM_SrcFile *src_file;
};

typedef struct CV2R_SrcFileMap CV2R_SrcFileMap;
struct CV2R_SrcFileMap
{
  CV2R_SrcFileNode **slots;
  U64 slots_count;
};

//- rjf: itype chains

typedef struct CV2R_TypeIdChain CV2R_TypeIdChain;
struct CV2R_TypeIdChain
{
  CV2R_TypeIdChain *next;
  CV_TypeId itype;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 cv2r_end_of_cplusplus_container_name(String8 str);
internal U64 cv2r_hash_from_voff(U64 voff);
internal int cv2r_namespace_node_is_before(void *raw_a, void *raw_b);
internal U64 cv2r_voff_from_soff(CV2R_Section *sections, U64 sections_count, U64 sec_idx, U64 soff);

////////////////////////////////
//~ rjf: String Table Functions

internal String8 cv2r_string_from_off(CV2R_StringTable *strtbl, U64 off);

////////////////////////////////
//~ rjf: Compilation Unit Contribution Functions

internal CV2R_CompUnitContribution *cv2r_comp_unit_contribution_from_voff__binary_search(CV2R_CompUnitContribution *contributions, U64 contributions_count, U64 voff);

////////////////////////////////
//~ rjf: TPI Hash Table Functions

internal U32 cv2r_tpi_hash_from_data(String8 string);
internal CV_TypeIdArray cv2r_itypes_from_name(Arena *arena, CV2R_TPIHash *tpi_hash, CV_LeafParsed *leaf, String8 name, B32 compare_unique_name, U32 output_cap);
internal CV_TypeId cv2r_first_itype_from_name(CV2R_TPIHash *tpi_hash, CV_LeafParsed *tpi_leaf, String8 name, B32 compare_unique_name);

////////////////////////////////
//~ rjf: CodeView => RDI Canonical Conversions

internal RDI_Arch         cv2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegCode      cv2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language     cv2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind     cv2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);
internal RDI_ChecksumKind cv2r_rdi_from_cv_c13_checksum_kind(CV_C13ChecksumKind k);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDI_RegCode cv2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal RDIM_Location cv2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal void cv2r_location_case_list_push_over_lvar_addr_range(Arena *arena, RDIM_LocationCaseList *loc_cases, RDIM_Location loc, Rng1U64 voff_range, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams cv2r_convert(Arena *arena, CV2R_ConvertParams *params);

#endif // RDI_FROM_CODEVIEW_H
