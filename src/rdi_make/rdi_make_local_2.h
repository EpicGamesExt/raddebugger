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
  RDI_U64 line_table_take_counter;
  RDIM_LineTable **src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables;
  
  RDIM_SortKey **sorted_line_table_keys;
};

global RDIM2_Shared *rdim2_shared = 0;

internal RDIM_BakeResults rdim2_bake(Arena *arena, RDIM_BakeParams *params);

#endif // RDI_MAKE_LOCAL_2_H
