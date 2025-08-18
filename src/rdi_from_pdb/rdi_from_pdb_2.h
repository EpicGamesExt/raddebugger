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
};

typedef struct P2R2_UnitSymBlock P2R2_UnitSymBlock;
struct P2R2_UnitSymBlock
{
  P2R2_UnitSymBlock *next;
  U64 unit_idx;
  Rng1U64 unit_rec_range;
};

typedef struct P2R2_UnitSymBlockList P2R2_UnitSymBlockList;
struct P2R2_UnitSymBlockList
{
  P2R2_UnitSymBlock *first;
  P2R2_UnitSymBlock *last;
};

typedef struct P2R2_Shared P2R2_Shared;
struct P2R2_Shared
{
  MSF_RawStreamTable *msf_raw_stream_table;
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
  CV_SymParsed *sym;
  PDB_CompUnitArray *comp_units;
  PDB_CompUnitContributionArray *comp_unit_contributions;
  RDIM_Rng1U64ChunkList *unit_ranges;
  
  CV_SymParsed **sym_for_unit;
  CV_C13Parsed **c13_for_unit;
  
  U64 exe_voff_max;
  RDI_Arch arch;
  
  U64 total_sym_record_count;
  P2R2_UnitSymBlockList *lane_sym_blocks;
  
  String8Array *lane_file_paths;
  U64Array *lane_file_paths_hashes;
  
  U64 total_path_count;
  
  RDIM_SrcFileChunkList all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map;
  
  RDIM_UnitChunkList all_units;
  RDIM_LineTable **units_first_inline_site_line_tables;
  RDIM_LineTableChunkList *lanes_line_tables;
  
  RDIM_LineTableChunkList all_line_tables;
  
  CV_TypeId *itype_fwd_map;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  
  P2R_TypeIdChain **itype_chains;
  
  RDIM_Type **itype_type_ptrs;
  RDIM_Type **basic_type_ptrs;
  RDIM_TypeChunkList all_types;
  
  RDIM_UDTChunkList *lanes_udts;
  
  RDIM_SymbolChunkList *lanes_procedures;
  RDIM_SymbolChunkList *lanes_global_variables;
  RDIM_SymbolChunkList *lanes_thread_variables;
  RDIM_SymbolChunkList *lanes_constants;
  RDIM_ScopeChunkList *lanes_scopes;
  RDIM_InlineSiteChunkList *lanes_inline_sites;
  RDIM_TypeChunkList *lanes_typedefs;
  
  RDIM_SymbolChunkList all_procedures;
  RDIM_SymbolChunkList all_global_variables;
  RDIM_SymbolChunkList all_thread_variables;
  RDIM_SymbolChunkList all_constants;
  RDIM_ScopeChunkList all_scopes;
  RDIM_InlineSiteChunkList all_inline_sites;
};

global P2R2_Shared *p2r2_shared = 0;

internal RDIM_BakeParams p2r2_convert(Arena **thread_arenas, U64 thread_count, P2R_ConvertParams *in);
internal void p2r2_convert_thread_entry_point(void *p);

#endif // RDI_FROM_PDB_2_H
