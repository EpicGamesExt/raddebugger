// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_DWARF_2_H
#define RDI_FROM_DWARF_2_H

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
  RDIM_SubsetFlags subset_flags;
  B32 deterministic;
};

////////////////////////////////
//~ rjf: Main Conversion Entry Point (New)

internal RDIM_BakeParams d2r2_convert(Arena *arena, D2R2_ConvertParams *params);

#endif // RDI_FROM_DWARF_2_H
