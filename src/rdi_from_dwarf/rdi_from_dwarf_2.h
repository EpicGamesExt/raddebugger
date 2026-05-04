// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_2_H
#define RDI_FROM_DWARF_2_H

////////////////////////////////
//~ rjf: Helper Types

typedef enum D2R2_UniqueTagKind
{
  D2R2_UniqueTagKind_Type,
  D2R2_UniqueTagKind_Namespace,
  D2R2_UniqueTagKind_COUNT
}
D2R2_UniqueTagKind;

typedef struct D2R2_UniqueTagNode D2R2_UniqueTagNode;
struct D2R2_UniqueTagNode
{
  D2R2_UniqueTagNode *next;
  D2R2_UniqueTagKind kind;
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

internal int d2r2_unique_tag_node_is_less_than(D2R2_UniqueTagNode **l, D2R2_UniqueTagNode **r);

////////////////////////////////
//~ rjf: Main Conversion Entry Point (New)

internal RDIM_BakeParams d2r2_convert(Arena *arena, D2R2_ConvertParams *params);

#endif // RDI_FROM_DWARF_2_H
