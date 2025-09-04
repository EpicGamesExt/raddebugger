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
  RDI_U64 scope_vmap_count;
  RDIM_SortKey *scope_vmap_keys;
  RDIM_SortKey *scope_vmap_keys__swap;
  RDIM_VMapMarker *scope_vmap_markers;
  RDI_U64 unit_vmap_count;
  RDIM_SortKey *unit_vmap_keys;
  RDIM_SortKey *unit_vmap_keys__swap;
  RDIM_VMapMarker *unit_vmap_markers;
  RDI_U64 global_vmap_count;
  RDIM_SortKey *global_vmap_keys;
  RDIM_SortKey *global_vmap_keys__swap;
  RDIM_VMapMarker *global_vmap_markers;
  U32 **lane_digit_counts;
  U32 **lane_digit_offsets;
  
  RDIM_ScopeVMapBakeResult baked_scope_vmap;
  RDIM_UnitVMapBakeResult baked_unit_vmap;
  RDIM_GlobalVMapBakeResult baked_global_vmap;
  
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
  RDIM_BakeIdxRunMap2 bake_idx_runs;
  
  RDIM_StringBakeResult baked_strings;
  
  RDIM_IndexRunBakeResult baked_idx_runs;
  
  RDI_U64 *lane_name_map_node_counts[RDI_NameMapKind_COUNT];
  RDI_U64 *lane_name_map_node_offs[RDI_NameMapKind_COUNT];
  RDI_U64 name_map_node_counts[RDI_NameMapKind_COUNT];
  RDI_U64 total_name_map_node_count;
  RDIM_TopLevelNameMapBakeResult baked_top_level_name_maps;
  RDIM_NameMapBakeResult baked_name_maps;
  
  RDI_U64 *lane_src_line_map_counts;
  RDI_U64 *lane_src_line_map_offs;
  RDIM_SrcFileBakeResult baked_src_files;
  
  RDI_U64 *scope_local_chunk_lane_counts; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_local_chunk_lane_offs; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_voff_chunk_lane_counts; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_voff_chunk_lane_offs; // [lane_count * scope_chunk_count]
  
  RDIM_ScopeBakeResult baked_scopes;
  
  RDI_U64 *member_chunk_lane_counts; // [lane_count * udt_chunk_count]
  RDI_U64 *member_chunk_lane_offs; // [lane_count * udt_chunk_count]
  RDI_U64 *enum_val_chunk_lane_counts; // [lane_count * udt_chunk_count]
  RDI_U64 *enum_val_chunk_lane_offs; // [lane_count * udt_chunk_count]
  
  RDIM_UDTBakeResult baked_udts;
  
  RDI_U64 *location_case_chunk_lane_counts; // [lane_count * (scope_chunk_count + procedure_chunk_count)
  RDI_U64 *location_case_chunk_lane_offs; // [lane_count * (scope_chunk_count + procedure_chunk_count)
  RDI_U64 total_location_case_count;
  
  RDIM_LocationBlockBakeResult baked_location_blocks;
  
  RDIM_UnitBakeResult baked_units;
  RDIM_TypeNodeBakeResult baked_type_nodes;
  RDIM_LocationBakeResult baked_locations;
  RDIM_GlobalVariableBakeResult baked_global_variables;
  RDIM_ThreadVariableBakeResult baked_thread_variables;
  RDIM_ConstantsBakeResult baked_constants;
  RDIM_ProcedureBakeResult baked_procedures;
  RDIM_InlineSiteBakeResult baked_inline_sites;
  
  RDIM_BakePathNode **baked_file_path_src_nodes;
  RDIM_FilePathBakeResult baked_file_paths;
  
  RDIM_TopLevelInfoBakeResult baked_top_level_info;
  RDIM_BinarySectionBakeResult baked_binary_sections;
};

global RDIM2_Shared *rdim2_shared = 0;

internal RDIM_BakeResults rdim2_bake(Arena *arena, RDIM_BakeParams *params);

#endif // RDI_MAKE_LOCAL_2_H
