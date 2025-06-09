// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_H
#define RDI_FROM_PDB_H

////////////////////////////////
//~ rjf: Conversion Stage Inputs/Outputs

typedef struct P2R_ConvertParams P2R_ConvertParams;
struct P2R_ConvertParams
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

////////////////////////////////
//~ rjf: Initial PDB Information Extraction & Conversion Preparation Task Types

//- rjf: tpi hash parsing

typedef struct P2R_TPIHashParseIn P2R_TPIHashParseIn;
struct P2R_TPIHashParseIn
{
  PDB_Strtbl *strtbl;
  PDB_TpiParsed *tpi;
  String8 hash_data;
  String8 aux_data;
};

//- rjf: tpi leaves parsing

typedef struct P2R_TPILeafParseIn P2R_TPILeafParseIn;
struct P2R_TPILeafParseIn
{
  String8 leaf_data;
  CV_TypeId itype_first;
};

//- rjf: exe hashing

typedef struct P2R_EXEHashIn P2R_EXEHashIn;
struct P2R_EXEHashIn
{
  String8 exe_data;
};

//- rjf: symbol stream parsing

typedef struct P2R_SymbolStreamParseIn P2R_SymbolStreamParseIn;
struct P2R_SymbolStreamParseIn
{
  String8 data;
};

//- rjf: c13 line info stream parsing

typedef struct P2R_C13StreamParseIn P2R_C13StreamParseIn;
struct P2R_C13StreamParseIn
{
  String8 data;
  String8 strtbl;
  COFF_SectionHeaderArray coff_sections;
};

//- rjf: comp unit parsing

typedef struct P2R_CompUnitParseIn P2R_CompUnitParseIn;
struct P2R_CompUnitParseIn
{
  String8 data;
};

//- rjf: comp unit contribution table parsing

typedef struct P2R_CompUnitContributionsParseIn P2R_CompUnitContributionsParseIn;
struct P2R_CompUnitContributionsParseIn
{
  String8 data;
  COFF_SectionHeaderArray coff_sections;
};

//- rjf: comp unit contribution table bucketing by unit

typedef struct P2R_CompUnitContributionsBucketIn P2R_CompUnitContributionsBucketIn;
struct P2R_CompUnitContributionsBucketIn
{
  U64 comp_unit_count;
  PDB_CompUnitContributionArray contributions;
};

typedef struct P2R_CompUnitContributionsBucketOut P2R_CompUnitContributionsBucketOut;
struct P2R_CompUnitContributionsBucketOut
{
  RDIM_Rng1U64ChunkList *unit_ranges;
};

////////////////////////////////
//~ rjf: Conversion Data Structure & Task Types

//- rjf: link name map (voff -> string)

typedef struct P2R_LinkNameNode P2R_LinkNameNode;
struct P2R_LinkNameNode
{
  P2R_LinkNameNode *next;
  U64 voff;
  String8 name;
};

typedef struct P2R_LinkNameMap P2R_LinkNameMap;
struct P2R_LinkNameMap
{
  P2R_LinkNameNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 link_name_count;
};

//- rjf: normalized file path -> source file map

typedef struct P2R_SrcFileNode P2R_SrcFileNode;
struct P2R_SrcFileNode
{
  P2R_SrcFileNode *next;
  RDIM_SrcFile *src_file;
};

typedef struct P2R_SrcFileMap P2R_SrcFileMap;
struct P2R_SrcFileMap
{
  P2R_SrcFileNode **slots;
  U64 slots_count;
};

//- rjf: per-unit source files conversion tasks

typedef struct P2R_GatherUnitSrcFilesIn P2R_GatherUnitSrcFilesIn;
struct P2R_GatherUnitSrcFilesIn
{
  PDB_Strtbl *pdb_strtbl;
  COFF_SectionHeaderArray coff_sections;
  PDB_CompUnit *comp_unit;
  CV_SymParsed *comp_unit_syms;
  CV_C13Parsed *comp_unit_c13s;
};

typedef struct P2R_GatherUnitSrcFilesOut P2R_GatherUnitSrcFilesOut;
struct P2R_GatherUnitSrcFilesOut
{
  String8Array src_file_paths;
};

//- rjf: unit conversion tasks

typedef struct P2R_UnitConvertIn P2R_UnitConvertIn;
struct P2R_UnitConvertIn
{
  U64 comp_unit_idx;
  PDB_Strtbl *pdb_strtbl;
  COFF_SectionHeaderArray coff_sections;
  PDB_CompUnit *comp_unit;
  RDIM_Rng1U64ChunkList comp_unit_ranges;
  CV_SymParsed *comp_unit_syms;
  CV_C13Parsed *comp_unit_c13s;
  P2R_SrcFileMap *src_file_map;
};

typedef struct P2R_UnitConvertOut P2R_UnitConvertOut;
struct P2R_UnitConvertOut
{
  RDIM_UnitChunkList units;
  RDIM_LineTableChunkList line_tables;
  RDIM_LineTable *unit_first_inline_site_line_table;
};

//- rjf: src file sequence equipping task

typedef struct P2R_SrcFileSeqEquipIn P2R_SrcFileSeqEquipIn;
struct P2R_SrcFileSeqEquipIn
{
  RDIM_SrcFileChunkList src_files;
  RDIM_LineTableChunkList line_tables;
};

//- rjf: link name map building tasks

typedef struct P2R_LinkNameMapBuildIn P2R_LinkNameMapBuildIn;
struct P2R_LinkNameMapBuildIn
{
  CV_SymParsed *sym;
  COFF_SectionHeaderArray coff_sections;
  P2R_LinkNameMap *link_name_map;
};

//- rjf: type forward resolution map build

typedef struct P2R_ITypeFwdMapFillIn P2R_ITypeFwdMapFillIn;
struct P2R_ITypeFwdMapFillIn
{
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
};

//- rjf: itype chain build

typedef struct P2R_TypeIdChain P2R_TypeIdChain;
struct P2R_TypeIdChain
{
  P2R_TypeIdChain *next;
  CV_TypeId itype;
};

typedef struct P2R_ITypeChainBuildIn P2R_ITypeChainBuildIn;
struct P2R_ITypeChainBuildIn
{
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
  P2R_TypeIdChain **itype_chains;
};

//- rjf: udt conversion

typedef struct P2R_UDTConvertIn P2R_UDTConvertIn;
struct P2R_UDTConvertIn
{
  CV_LeafParsed *tpi_leaf;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  CV_TypeId *itype_fwd_map;
  RDIM_Type **itype_type_ptrs;
};

//- rjf: symbol stream conversion

typedef struct P2R_SymbolStreamConvertIn P2R_SymbolStreamConvertIn;
struct P2R_SymbolStreamConvertIn
{
  B32 parsing_global_stream;
  RDI_Arch arch;
  COFF_SectionHeaderArray coff_sections;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_LeafParsed *ipi_leaf;
  CV_SymParsed *sym;
  U64 sym_ranges_first;
  U64 sym_ranges_opl;
  CV_TypeId *itype_fwd_map;
  RDIM_Type **itype_type_ptrs;
  P2R_LinkNameMap *link_name_map;
  RDIM_LineTable *first_inline_site_line_table;
};

typedef struct P2R_SymbolStreamConvertOut P2R_SymbolStreamConvertOut;
struct P2R_SymbolStreamConvertOut
{
  RDIM_SymbolChunkList procedures;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_SymbolChunkList constants;
  RDIM_ScopeChunkList scopes;
  RDIM_InlineSiteChunkList inline_sites;
  RDIM_TypeChunkList typedefs;
};

////////////////////////////////
//~ rjf: Globals

global ASYNC_Root *p2r_async_root = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: Command Line -> Conversion Inputs

#if 0
internal P2R_ConvertParams *p2r_user2convert_from_cmdln(Arena *arena, CmdLine *cmdline);
#endif

////////////////////////////////
//~ rjf: COFF => RDI Canonical Conversions

internal RDI_BinarySectionFlags p2r_rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RDI Canonical Conversions

internal RDI_Arch     p2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegCode  p2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language p2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind p2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal RDI_RegCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal void p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Tasks

ASYNC_WORK_DEF(p2r_exe_hash_work);
ASYNC_WORK_DEF(p2r_tpi_hash_parse_work);
ASYNC_WORK_DEF(p2r_tpi_leaf_work);
ASYNC_WORK_DEF(p2r_symbol_stream_parse_work);
ASYNC_WORK_DEF(p2r_c13_stream_parse_work);
ASYNC_WORK_DEF(p2r_comp_unit_parse_work);
ASYNC_WORK_DEF(p2r_comp_unit_contributions_parse_work);
ASYNC_WORK_DEF(p2r_comp_unit_contributions_bucket_work);

////////////////////////////////
//~ rjf: Unit Source File Gathering Tasks

ASYNC_WORK_DEF(p2r_gather_unit_src_file_work);

////////////////////////////////
//~ rjf: Unit Conversion Tasks

ASYNC_WORK_DEF(p2r_unit_convert_work);

////////////////////////////////
//~ rjf: Source File Sequence Equipping Task

ASYNC_WORK_DEF(p2r_src_file_seq_equip_work);

////////////////////////////////
//~ rjf: Link Name Map Building Tasks

ASYNC_WORK_DEF(p2r_link_name_map_build_work);

////////////////////////////////
//~ rjf: Type Parsing/Conversion Tasks

ASYNC_WORK_DEF(p2r_itype_fwd_map_fill_work);
ASYNC_WORK_DEF(p2r_itype_chain_build_work);

////////////////////////////////
//~ rjf: UDT Conversion Tasks

ASYNC_WORK_DEF(p2r_udt_convert_work);

////////////////////////////////
//~ rjf: Symbol Stream Conversion Tasks

ASYNC_WORK_DEF(p2r_symbol_stream_convert_work);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams p2r_convert(Arena *arena, ASYNC_Root *async_root, P2R_ConvertParams *in);

////////////////////////////////

internal B32 p2r_has_symbol_ref(String8 msf_data, String8List symbol_list, MSF_RawStreamTable *st);
internal B32 p2r_has_file_ref(String8 msf_data, String8List file_list, MSF_RawStreamTable *st);
internal B32 p2r_has_symbol_or_file_ref(String8 msf_data, String8List symbol_list, String8List file_list);

#endif // RDI_FROM_PDB_H
