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
  
  String8Array *unit_src_file_paths;
  U64Array *unit_src_file_paths_hashes;
  U64 total_path_count;
  
  RDIM_SrcFileChunkList all_src_files__sequenceless;
  P2R_SrcFileMap src_file_map;
  
  RDIM_UnitChunkList all_units;
  RDIM_LineTableChunkList *units_line_tables;
  RDIM_LineTable **units_first_inline_site_line_tables;
  
  RDIM_LineTableChunkList all_line_tables;
};

global P2R2_Shared *p2r2_shared = 0;

internal void p2r2_convert_thread_entry_point(void *p);
internal RDIM_BakeParams p2r2_convert(Arena **thread_arenas, U64 thread_count, P2R_ConvertParams *in);

#endif // RDI_FROM_PDB_2_H
