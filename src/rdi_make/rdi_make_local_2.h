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
  RDI_U64 line_tables_count;
  RDI_U64 line_table_block_take_counter;
  RDIM_LineTable **src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables;
  
  RDIM_SortKey **sorted_line_table_keys;
  
  RDI_U64 final_line_tables_count;
  RDI_U64 final_line_voffs_count;
  RDI_U64 final_lines_count;
  RDI_U64 final_cols_count;
  RDI_LineTable *final_line_tables;
  RDI_U64 *final_line_voffs;
  RDI_Line *final_lines;
  RDI_Column *final_cols;
  
  RDIM_BakeStringMapTopology bake_string_map_topology;
  RDIM_BakeStringMapLoose **lane_bake_string_maps__loose;
  RDIM_BakeStringMapLoose *bake_string_map__loose;
  RDIM_BakeStringMapTight bake_strings;
};

global RDIM2_Shared *rdim2_shared = 0;

internal RDIM_BakeResults rdim2_bake(Arena *arena, RDIM_BakeParams *params);

#endif // RDI_MAKE_LOCAL_2_H
