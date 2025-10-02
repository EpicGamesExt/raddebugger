// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_MAKE_LOCAL_H
#define RDI_MAKE_LOCAL_H

//- rjf: base layer memory ops
#define RDIM_MEMSET_OVERRIDE
#define RDIM_MEMCPY_OVERRIDE
#define rdim_memset MemorySet
#define rdim_memcpy MemoryCopy

//- rjf: base layer string overrides
#define RDI_STRING8_OVERRIDE
#define RDIM_String8            String8
#define RDIM_String8_BaseMember str
#define RDIM_String8_SizeMember size
#define RDI_STRING8LIST_OVERRIDE
#define RDIM_String8Node                 String8Node
#define RDIM_String8Node_NextPtrMember   next
#define RDIM_String8Node_StringMember    string
#define RDIM_String8List                 String8List
#define RDIM_String8List_FirstMember     first
#define RDIM_String8List_LastMember      last
#define RDIM_String8List_NodeCountMember node_count
#define RDIM_String8List_TotalSizeMember total_size

//- rjf: base layer arena overrides
#define RDIM_ARENA_OVERRIDE
#define RDIM_Arena Arena
#define rdim_arena_alloc     arena_alloc
#define rdim_arena_release   arena_release
#define rdim_arena_pos       arena_pos
#define rdim_arena_push      arena_push
#define rdim_arena_pop_to    arena_pop_to

//- rjf: base layer scratch arena overrides
#define RDIM_SCRATCH_OVERRIDE
#define RDIM_Temp Temp
#define rdim_temp_arena(t)   ((t).arena)
#define rdim_scratch_begin   scratch_begin
#define rdim_scratch_end     scratch_end

//- rjf: base layer profiling markup overrides
#define RDIM_ProfBegin(...) ProfBeginDynamic(__VA_ARGS__)
#define RDIM_ProfEnd(...) ProfEnd()

//- rjf: main library
#include "lib_rdi_make/rdi_make.h"

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

typedef struct RDIM_Shared RDIM_Shared;
struct RDIM_Shared
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
  RDIM_BakeNameMap **lane_bake_name_maps[RDI_NameMapKind_COUNT];
  RDIM_BakeNameMap *bake_name_maps[RDI_NameMapKind_COUNT];
  
  RDIM_BakeIdxRunMapTopology bake_idx_run_map_topology;
  RDIM_BakeIdxRunMapLoose **lane_bake_idx_run_maps__loose;
  RDIM_BakeIdxRunMapLoose *bake_idx_run_map__loose;
  RDIM_BakeIdxRunMap bake_idx_runs;
  
  RDIM_StringBakeResult baked_strings;
  
  RDIM_IndexRunBakeResult baked_idx_runs;
  
  RDI_U64 *lane_name_map_node_counts[RDI_NameMapKind_COUNT];
  RDI_U64 *lane_name_map_node_offs[RDI_NameMapKind_COUNT];
  RDI_U64 name_map_node_counts[RDI_NameMapKind_COUNT];
  RDI_U64 total_name_map_node_count;
  RDIM_TopLevelNameMapBakeResult baked_top_level_name_maps;
  RDIM_NameMapBakeResult baked_name_maps;
  
  RDIM_BakeSrcLineMap *bake_src_line_maps;
  
  RDI_U64 bake_src_line_map_take_counter;
  RDIM_SortKey **bake_src_line_map_keys;
  
  RDI_U64 *lane_chunk_src_file_num_counts; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_voff_counts; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_map_counts; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_num_offs; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_voff_offs; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_map_offs; // [lane_count * src_file_chunk_count]
  RDI_U64 total_src_map_line_count;
  RDI_U64 total_src_map_voff_count;
  
  RDIM_SrcFileBakeResult baked_src_files;
  
  RDI_U64 *lane_chunk_src_file_checksum_counts; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_checksum_sizes; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_checksum_off_offs; // [lane_count * src_file_chunk_count]
  RDI_U64 *lane_chunk_src_file_checksum_data_offs; // [lane_count * src_file_chunk_count]
  U64 total_checksum_count;
  U64 total_checksum_size;
  
  RDIM_ChecksumBakeResult baked_checksums;
  
  RDI_U64 *member_chunk_lane_counts; // [lane_count * udt_chunk_count]
  RDI_U64 *member_chunk_lane_offs; // [lane_count * udt_chunk_count]
  RDI_U64 *enum_val_chunk_lane_counts; // [lane_count * udt_chunk_count]
  RDI_U64 *enum_val_chunk_lane_offs; // [lane_count * udt_chunk_count]
  
  RDIM_UDTBakeResult baked_udts;
  
  RDI_U64 *location_case_chunk_lane_counts; // [lane_count * (scope_chunk_count + procedure_chunk_count)
  RDI_U64 *location_case_chunk_lane_offs; // [lane_count * (scope_chunk_count + procedure_chunk_count)
  RDI_U64 total_location_case_count;
  
  RDIM_LocationBlockBakeResult baked_location_blocks;
  
  RDIM_LocationBakeResult baked_locations;
  
  RDI_U64 *scope_local_chunk_lane_counts; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_local_chunk_lane_offs; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_voff_chunk_lane_counts; // [lane_count * scope_chunk_count]
  RDI_U64 *scope_voff_chunk_lane_offs; // [lane_count * scope_chunk_count]
  
  RDIM_ScopeBakeResult baked_scopes;
  
  RDIM_ProcedureBakeResult baked_procedures;
  
  RDI_U64 *constant_data_chunk_lane_counts; // [lane_count * constant_chunk_count]
  RDI_U64 *constant_data_chunk_lane_offs; // [lane_count * constant_chunk_count]
  
  RDIM_ConstantsBakeResult baked_constants;
  
  RDIM_UnitBakeResult baked_units;
  RDIM_TypeNodeBakeResult baked_type_nodes;
  RDIM_GlobalVariableBakeResult baked_global_variables;
  RDIM_ThreadVariableBakeResult baked_thread_variables;
  RDIM_InlineSiteBakeResult baked_inline_sites;
  
  RDIM_BakePathNode **baked_file_path_src_nodes;
  RDIM_FilePathBakeResult baked_file_paths;
  
  RDIM_TopLevelInfoBakeResult baked_top_level_info;
  RDIM_BinarySectionBakeResult baked_binary_sections;
};

global RDIM_Shared *rdim_shared = 0;

internal RDIM_DataModel rdim_data_model_from_os_arch(OperatingSystem os, RDI_Arch arch);
internal RDIM_TopLevelInfo rdim_make_top_level_info(String8 image_name, Arch arch, U64 exe_hash, RDIM_BinarySectionList sections);
internal RDIM_BakeResults rdim_bake(Arena *arena, RDIM_BakeParams *params);
internal RDIM_SerializedSectionBundle rdim_compress(Arena *arena, RDIM_SerializedSectionBundle *in);

#endif // RDI_MAKE_LOCAL_H
