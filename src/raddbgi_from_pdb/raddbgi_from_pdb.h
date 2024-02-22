// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_FROM_PDB_H
#define RADDBGI_FROM_PDB_H

////////////////////////////////
//~ rjf: Conversion Stage Inputs/Outputs

typedef struct P2R_User2Convert P2R_User2Convert;
struct P2R_User2Convert
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 output_name;
  struct
  {
    B8 input;
    B8 output;
    B8 parsing;
    B8 converting;
  } hide_errors;
  B8 dump;
  B8 dump__first;
  B8 dump_coff_sections;
  B8 dump_msf;
  B8 dump_sym;
  B8 dump_tpi_hash;
  B8 dump_leaf;
  B8 dump_c13;
  B8 dump_contributions;
  B8 dump_table_diagnostics;
  B8 dump__last;
  String8List errors;
};

typedef struct P2R_Convert2Bake P2R_Convert2Bake;
struct P2R_Convert2Bake
{
  RDIM_BakeParams bake_params;
};

typedef struct P2R_Bake2Serialize P2R_Bake2Serialize;
struct P2R_Bake2Serialize
{
  RDIM_BakeSectionList sections;
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
  PDB_Strtbl *strtbl;
  PDB_CoffSectionArray *coff_sections;
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
  PDB_CoffSectionArray *coff_sections;
};

////////////////////////////////
//~ rjf: Conversion Data Structure Types

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

//- rjf: link name map building tasks

typedef struct P2R_LinkNameMapBuildIn P2R_LinkNameMapBuildIn;
struct P2R_LinkNameMapBuildIn
{
  CV_SymParsed *sym;
  PDB_CoffSectionArray *coff_sections;
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

//- rjf: symbol stream conversion

typedef struct P2R_SymbolStreamConvertIn P2R_SymbolStreamConvertIn;
struct P2R_SymbolStreamConvertIn
{
  RDI_Arch arch;
  PDB_CoffSectionArray *coff_sections;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  CV_SymParsed *sym;
  U64 sym_ranges_first;
  U64 sym_ranges_opl;
  CV_TypeId *itype_fwd_map;
  RDIM_Type **itype_type_ptrs;
  P2R_LinkNameMap *link_name_map;
};

typedef struct P2R_SymbolStreamConvertOut P2R_SymbolStreamConvertOut;
struct P2R_SymbolStreamConvertOut
{
  RDIM_SymbolChunkList procedures;
  RDIM_SymbolChunkList global_variables;
  RDIM_SymbolChunkList thread_variables;
  RDIM_ScopeChunkList scopes;
};

////////////////////////////////
//~ rjf: Baking Task Types

typedef struct P2R_BuildBakeStringMapIn P2R_BuildBakeStringMapIn;
struct P2R_BuildBakeStringMapIn
{
  RDIM_BakePathTree *path_tree;
  RDIM_BakeParams *params;
};

typedef struct P2R_BuildBakeNameMapIn P2R_BuildBakeNameMapIn;
struct P2R_BuildBakeNameMapIn
{
  RDI_NameMapKind k;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeUnitsIn P2R_BakeUnitsIn;
struct P2R_BakeUnitsIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakePathTree *path_tree;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeUnitVMapIn P2R_BakeUnitVMapIn;
struct P2R_BakeUnitVMapIn
{
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeSrcFilesIn P2R_BakeSrcFilesIn;
struct P2R_BakeSrcFilesIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakePathTree *path_tree;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeUDTsIn P2R_BakeUDTsIn;
struct P2R_BakeUDTsIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeGlobalVariablesIn P2R_BakeGlobalVariablesIn;
struct P2R_BakeGlobalVariablesIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeGlobalVMapIn P2R_BakeGlobalVMapIn;
struct P2R_BakeGlobalVMapIn
{
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeThreadVariablesIn P2R_BakeThreadVariablesIn;
struct P2R_BakeThreadVariablesIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeProceduresIn P2R_BakeProceduresIn;
struct P2R_BakeProceduresIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeScopesIn P2R_BakeScopesIn;
struct P2R_BakeScopesIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeScopeVMapIn P2R_BakeScopeVMapIn;
struct P2R_BakeScopeVMapIn
{
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeFilePathsIn P2R_BakeFilePathsIn;
struct P2R_BakeFilePathsIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakePathTree *path_tree;
};

typedef struct P2R_BakeStringsIn P2R_BakeStringsIn;
struct P2R_BakeStringsIn
{
  RDIM_BakeStringMap *strings;
};

typedef struct P2R_BakeTypeNodesIn P2R_BakeTypeNodesIn;
struct P2R_BakeTypeNodesIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeIdxRunMap *idx_runs;
  RDIM_BakeParams *params;
};

typedef struct P2R_BakeNameMapIn P2R_BakeNameMapIn;
struct P2R_BakeNameMapIn
{
  RDIM_BakeStringMap *strings;
  RDIM_BakeIdxRunMap *idx_runs;
  RDIM_BakeParams *params;
  RDI_NameMapKind kind;
  RDIM_BakeNameMap *map;
};

typedef struct P2R_BakeIdxRunsIn P2R_BakeIdxRunsIn;
struct P2R_BakeIdxRunsIn
{
  RDIM_BakeIdxRunMap *idx_runs;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

////////////////////////////////
//~ rjf: Command Line -> Conversion Inputs

internal P2R_User2Convert *p2r_user2convert_from_cmdln(Arena *arena, CmdLine *cmdline);

////////////////////////////////
//~ rjf: COFF => RADDBGI Canonical Conversions

internal RDI_BinarySectionFlags rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RADDBGI Canonical Conversions

internal RDI_Arch         p2r_rdi_arch_from_cv_arch(CV_Arch arch);
internal RDI_RegisterCode p2r_rdi_reg_code_from_cv_reg_code(RDI_Arch arch, CV_Reg reg_code);
internal RDI_Language     p2r_rdi_language_from_cv_language(CV_Language language);
internal RDI_TypeKind     p2r_rdi_type_kind_from_cv_basic_type(CV_BasicType basic_type);

////////////////////////////////
//~ rjf: Location Info Building Helpers

internal RDIM_Location *p2r_location_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegisterCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal CV_EncodedFramePtrReg p2r_cv_encoded_fp_reg_from_frameproc(CV_SymFrameproc *frameproc, B32 param_base);
internal RDI_RegisterCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal void p2r_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Initial Parsing & Preparation Pass Tasks

internal void *p2r_exe_hash_task__entry_point(Arena *arena, void *p);
internal void *p2r_tpi_hash_parse_task__entry_point(Arena *arena, void *p);
internal void *p2r_tpi_leaf_parse_task__entry_point(Arena *arena, void *p);
internal void *p2r_symbol_stream_parse_task__entry_point(Arena *arena, void *p);
internal void *p2r_c13_stream_parse_task__entry_point(Arena *arena, void *p);
internal void *p2r_comp_unit_parse_task__entry_point(Arena *arena, void *p);
internal void *p2r_comp_unit_contributions_parse_task__entry_point(Arena *arena, void *p);

////////////////////////////////
//~ rjf: Link Name Map Building Task

internal void *p2r_link_name_map_build_task__entry_point(Arena *arena, void *p);

////////////////////////////////
//~ rjf: Type Parsing/Conversion Tasks

internal void *p2r_itype_fwd_map_fill_task__entry_point(Arena *arena, void *p);
internal void *p2r_itype_chain_build_task__entry_point(Arena *arena, void *p);

////////////////////////////////
//~ rjf: Symbol Stream Conversion Tasks

internal void *p2r_symbol_stream_convert_task__entry_point(Arena *arena, void *p);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal P2R_Convert2Bake *p2r_convert(Arena *arena, P2R_User2Convert *in);

////////////////////////////////
//~ rjf: Baking Stage Tasks

//- rjf: pass 1: interner/deduper map builds
internal void *p2r_build_bake_string_map_task__entry_point(Arena *arena, void *p);
internal void *p2r_build_bake_name_map_task__entry_point(Arena *arena, void *p);

//- rjf: pass 2: string-map-dependent debug info stream builds
internal void *p2r_bake_units_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_unit_vmap_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_src_files_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_udts_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_global_variables_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_global_vmap_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_thread_variables_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_procedures_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_scopes_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_scope_vmap_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_file_paths_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_strings_task__entry_point(Arena *arena, void *p);

//- rjf: pass 3: idx-run-map-dependent debug info stream builds
internal void *p2r_bake_type_nodes_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_name_map_task__entry_point(Arena *arena, void *p);
internal void *p2r_bake_idx_runs_task__entry_point(Arena *arena, void *p);

////////////////////////////////
//~ rjf: Top-Level Baking Entry Point

internal P2R_Bake2Serialize *p2r_bake(Arena *arena, P2R_Convert2Bake *in);

#endif // RADDBGI_FROM_PDB_H
