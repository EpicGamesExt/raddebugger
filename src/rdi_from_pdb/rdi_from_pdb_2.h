// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_2_H
#define RDI_FROM_PDB_2_H

typedef struct P2R2_ConvertThreadParams P2R2_ConvertThreadParams;
struct P2R2_ConvertThreadParams
{
  Arena *arena;
  LaneCtx lane_ctx;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 input_pdb_name;
  String8 input_pdb_data;
  B32 deterministic;
  RDIM_BakeParams *out_bake_params;
};

typedef struct P2R2_SymBlock P2R2_SymBlock;
struct P2R2_SymBlock
{
  P2R2_SymBlock *next;
  PDB_CompUnit *unit;
  CV_SymParsed *sym;
  CV_C13Parsed *c13;
  Rng1U64 sym_rec_range;
};

typedef struct P2R2_SymBlockList P2R2_SymBlockList;
struct P2R2_SymBlockList
{
  P2R2_SymBlock *first;
  P2R2_SymBlock *last;
};

typedef struct P2R2_UnitSubStartPtInfo P2R2_UnitSubStartPtInfo;
struct P2R2_UnitSubStartPtInfo
{
  CV_SymFrameproc last_frameproc;
  U64 last_proc_voff;
};

typedef struct P2R2_Shared P2R2_Shared;
struct P2R2_Shared
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
  PDB_CompUnitContributionArray *comp_unit_contributions;
  RDIM_Rng1U64ChunkList *unit_ranges;
  
  U64 sym_c13_unit_lane_counter;
  U64 all_syms_count;
  CV_SymParsed **all_syms; // [0] -> global; rest are unit nums
  CV_C13Parsed **all_c13s; // [0] -> blank (global); rest are unit nums
  
  U64 exe_voff_max;
  RDI_Arch arch;
  U64 symbol_count_prediction;
  
  P2R_LinkNameMap link_name_map;
  
  U64 total_sym_record_count;
  P2R2_SymBlockList *lane_sym_blocks;
  
  P2R2_UnitSubStartPtInfo *lane_unit_sub_start_pt_infos;
  
  String8Array *lane_file_paths;
  U64Array *lane_file_paths_hashes;
  
  U64 total_path_count;
  
  RDIM_SrcFileChunkList all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map;
  
  RDIM_UnitChunkList all_units;
  RDIM_LineTableChunkList *lanes_line_tables;
  RDIM_LineTable **lanes_first_inline_site_line_tables;
  
  RDIM_LineTableChunkList all_line_tables;
  
  CV_TypeId *itype_fwd_map;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  
  P2R_TypeIdChain **itype_chains;
  
  RDIM_Type **itype_type_ptrs;
  RDIM_Type **basic_type_ptrs;
  RDIM_TypeChunkList all_types__pre_typedefs;
  
  RDIM_UDTChunkList *lanes_udts;
  RDIM_UDTMemberChunkList *lanes_members;
  RDIM_UDTEnumValChunkList *lanes_enum_vals;
  
  RDIM_UDTChunkList all_udts;
  RDIM_UDTMemberChunkList all_members;
  RDIM_UDTEnumValChunkList all_enum_vals;
  
  RDIM_LocationChunkList *lanes_locations;
  RDIM_LocationCaseChunkList *lanes_location_cases;
  RDIM_SymbolChunkList *lanes_procedures;
  RDIM_SymbolChunkList *lanes_global_variables;
  RDIM_SymbolChunkList *lanes_thread_variables;
  RDIM_SymbolChunkList *lanes_constants;
  RDIM_ScopeChunkList *lanes_scopes;
  RDIM_InlineSiteChunkList *lanes_inline_sites;
  RDIM_TypeChunkList *lanes_typedefs;
  
  RDIM_LocationChunkList all_locations;
  RDIM_LocationCaseChunkList all_location_cases;
  RDIM_SymbolChunkList all_procedures;
  RDIM_SymbolChunkList all_global_variables;
  RDIM_SymbolChunkList all_thread_variables;
  RDIM_SymbolChunkList all_constants;
  RDIM_ScopeChunkList all_scopes;
  RDIM_InlineSiteChunkList all_inline_sites;
  RDIM_TypeChunkList all_types;
};

global P2R2_Shared *p2r2_shared = 0;

internal RDIM_LocationInfo p2r2_location_info_from_addr_reg_off(Arena *arena, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 extra_indirection);
internal void p2r2_location_over_lvar_addr_range(Arena *arena, RDIM_ScopeChunkList *scopes, RDIM_LocationSet *locset, RDIM_Location *location, CV_LvarAddrRange *range, COFF_SectionHeader *section, CV_LvarAddrGap *gaps, U64 gap_count);

internal RDIM_BakeParams p2r2_convert(Arena *arena, P2R_ConvertParams *params);

#endif // RDI_FROM_PDB_2_H
