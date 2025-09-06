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
//~ rjf: Shared Conversion State

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

//- rjf: itype chains

typedef struct P2R_TypeIdChain P2R_TypeIdChain;
struct P2R_TypeIdChain
{
  P2R_TypeIdChain *next;
  CV_TypeId itype;
};

//- rjf: main state bundle

typedef struct P2R_Shared P2R_Shared;
struct P2R_Shared
{
  MSF_RawStreamTable *msf_raw_stream_table;
  U64 msf_stream_lane_counter;
  MSF_Parsed *msf;
  
  PDB_Info *pdb_info;
  PDB_NamedStreamTable *named_streams;
  
  PDB_Strtbl *strtbl;
  String8 raw_strtbl;
  PDB_DbiParsed *dbi;
  PDB_TpiParsed *tpi;
  PDB_TpiParsed *ipi;
  
  COFF_SectionHeaderArray coff_sections;
  PDB_GsiParsed *gsi;
  PDB_GsiParsed *psi_gsi_part;
  
  U64 exe_hash;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  PDB_TpiHashParsed *ipi_hash;
  CV_LeafParsed *ipi_leaf;
  PDB_CompUnitArray *comp_units;
  PDB_CompUnitContributionArray comp_unit_contributions;
  RDIM_Rng1U64ChunkList *unit_ranges;
  
  U64 sym_c13_unit_lane_counter;
  U64 all_syms_count;
  CV_SymParsed **all_syms; // [0] -> global; rest are unit nums
  CV_C13Parsed **all_c13s; // [0] -> blank (global); rest are unit nums
  
  U64 exe_voff_max;
  RDI_Arch arch;
  U64 symbol_count_prediction;
  
  P2R_LinkNameMap link_name_map;
  
  U64 sym_lane_take_counter;
  
  String8Array *unit_file_paths;
  U64Array *unit_file_paths_hashes;
  
  U64 total_path_count;
  
  RDIM_SrcFileChunkList all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map;
  
  RDIM_UnitChunkList all_units;
  RDIM_LineTableChunkList *units_line_tables;
  RDIM_LineTable **units_first_inline_site_line_tables;
  
  RDIM_LineTableChunkList all_line_tables;
  
  CV_TypeId *itype_fwd_map;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  
  P2R_TypeIdChain **itype_chains;
  
  RDIM_Type **itype_type_ptrs;
  RDIM_Type **basic_type_ptrs;
  RDIM_TypeChunkList all_types__pre_typedefs;
  
  RDIM_UDTChunkList *lanes_udts;
  
  RDIM_UDTChunkList all_udts;
  
  RDIM_LocationChunkList *syms_locations;
  RDIM_SymbolChunkList *syms_procedures;
  RDIM_SymbolChunkList *syms_global_variables;
  RDIM_SymbolChunkList *syms_thread_variables;
  RDIM_SymbolChunkList *syms_constants;
  RDIM_ScopeChunkList *syms_scopes;
  RDIM_InlineSiteChunkList *syms_inline_sites;
  RDIM_TypeChunkList *syms_typedefs;
  
  RDIM_LocationChunkList all_locations;
  RDIM_SymbolChunkList all_procedures;
  RDIM_SymbolChunkList all_global_variables;
  RDIM_SymbolChunkList all_thread_variables;
  RDIM_SymbolChunkList all_constants;
  RDIM_ScopeChunkList all_scopes;
  RDIM_InlineSiteChunkList all_inline_sites;
  RDIM_TypeChunkList all_types;
};

////////////////////////////////
//~ rjf: Globals

global P2R_Shared *p2r_shared = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 p2r_end_of_cplusplus_container_name(String8 str);
internal U64 p2r_hash_from_voff(U64 voff);

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

internal RDI_RegCode p2r_reg_code_from_arch_encoded_fp_reg(RDI_Arch arch, CV_EncodedFramePtrReg encoded_reg);
internal RDIM_LocationInfo p2r_location_info_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal void p2r_local_push_location_cases_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_Local *local, RDIM_Location *loc, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal RDIM_BakeParams p2r_convert(Arena *arena, P2R_ConvertParams *params);

#endif // RDI_FROM_PDB_H
