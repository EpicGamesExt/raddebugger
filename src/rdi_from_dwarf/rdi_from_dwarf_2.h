// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_2_H
#define RDI_FROM_DWARF_2_H

////////////////////////////////
//~ rjf: Helper Types

typedef struct D2R2_UniqueTypeTagNode D2R2_UniqueTypeTagNode;
struct D2R2_UniqueTypeTagNode
{
  D2R2_UniqueTypeTagNode *next;
  U64 hash;
  U64 info_off;
  U64 order_idx;
};

////////////////////////////////
//~ rjf: Conversion Stage Inputs (New)

typedef struct D2R2_ConvertParams D2R2_ConvertParams;
struct D2R2_ConvertParams
{
  String8 exe_name;
  String8 exe_data;
  Arch arch;
  U64 base_vaddr;
  RDIM_BinarySectionList binary_sections;
  DW_Raw raw;
  PathStyle path_style;
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

////////////////////////////////
//~ rjf: Helpers

internal int d2r2_unique_type_tag_node_is_less_than(D2R2_UniqueTypeTagNode **l, D2R2_UniqueTypeTagNode **r);

////////////////////////////////
//~ rjf: Main Conversion Entry Point (New)

internal RDIM_BakeParams d2r2_convert(Arena *arena, D2R2_ConvertParams *params);

#endif // RDI_FROM_DWARF_2_H
