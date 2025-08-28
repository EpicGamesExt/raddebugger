// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_MAKE_LOCAL_2_H
#define RDI_MAKE_LOCAL_2_H

//- rjf: unsorted joined line table info

typedef struct RDIM_UnsortedJoinedLineTable RDIM_UnsortedJoinedLineTable;
struct RDIM_UnsortedJoinedLineTable
{
  RDI_U64 line_count;
  RDI_U64 seq_count;
  RDI_U64 key_count;
  RDIM_SortKey *line_keys;
  RDIM_LineRec *line_recs;
};

//- rjf: shared state bundle

typedef struct RDIM2_Shared RDIM2_Shared;
struct RDIM2_Shared
{
  RDIM_BakePathTree *path_tree;
  
  RDI_U64 line_tables_count;
  RDI_U64 line_table_block_take_counter;
  RDIM_LineTable **src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables;
  
  RDIM_SortKey **sorted_line_table_keys;
  
  RDIM_LineTableBakeResult baked_line_tables;
  
  RDIM_BakeStringMapTopology bake_string_map_topology;
  RDIM_BakeStringMapLoose **lane_bake_string_maps__loose;
  RDIM_BakeStringMapLoose *bake_string_map__loose;
  RDIM_BakeStringMapTight bake_strings;
  
  RDIM_BakeNameMapTopology bake_name_map_topology[RDI_NameMapKind_COUNT];
  RDIM_BakeNameMap2 **lane_bake_name_maps[RDI_NameMapKind_COUNT];
  RDIM_BakeNameMap2 *bake_name_maps[RDI_NameMapKind_COUNT];
  
  RDIM_BakeIdxRunMapTopology bake_idx_run_map_topology;
  RDIM_BakeIdxRunMapLoose **lane_bake_idx_run_maps__loose;
  RDIM_BakeIdxRunMapLoose *bake_idx_run_map__loose;
  RDIM_BakeIdxRunMap2 *bake_idx_runs;
  
  RDIM_StringBakeResult baked_strings;
  RDIM_IndexRunBakeResult baked_idx_runs;
  
  RDIM_UnitBakeResult baked_units;
  RDIM_SrcFileBakeResult baked_src_files;
  RDIM_TypeNodeBakeResult baked_type_nodes;
  RDIM_UDTBakeResult baked_udts;
  RDIM_LocationBakeResult baked_locations;
  RDIM_GlobalVariableBakeResult baked_global_variables;
  RDIM_ThreadVariableBakeResult baked_thread_variables;
  RDIM_ConstantsBakeResult baked_constants;
  RDIM_ProcedureBakeResult baked_procedures;
  RDIM_InlineSiteBakeResult baked_inline_sites;
  
  RDIM_TopLevelInfoBakeResult baked_top_level_info;
  RDIM_BinarySectionBakeResult baked_binary_sections;
  RDIM_UnitVMapBakeResult baked_unit_vmap;
  RDIM_ScopeVMapBakeResult baked_scope_vmap;
  RDIM_GlobalVMapBakeResult baked_global_vmap;
};

global RDIM2_Shared *rdim2_shared = 0;

internal RDIM_BakeResults rdim2_bake(Arena *arena, RDIM_BakeParams *params);

#endif // RDI_MAKE_LOCAL_2_H
