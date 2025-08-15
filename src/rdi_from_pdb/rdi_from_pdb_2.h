// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_PDB_2_H
#define RDI_FROM_PDB_2_H

typedef struct P2R2_ConvertThreadParams P2R2_ConvertThreadParams;
struct P2R2_ConvertThreadParams
{
  Arena *arena;
  U64 lane_idx;
  U64 lane_count;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 input_pdb_name;
  String8 input_pdb_data;
  B32 deterministic;
};

typedef struct P2R2_Shared P2R2_Shared;
struct P2R2_Shared
{
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
};

global P2R2_Shared *p2r2_shared = 0;

internal void p2r2_convert_thread_entry_point(void *p);

#endif // RDI_FROM_PDB_2_H
