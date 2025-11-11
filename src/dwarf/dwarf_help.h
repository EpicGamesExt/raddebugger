// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_HELP_H
#define DWARF_HELP_H

typedef struct DW_CallFrameInfo
{
  U64 cie_count;
  U64 fde_count;
  DW_CIE *cie;
  DW_FDE *fde;
} DW_CallFrameInfo;

internal DW_CallFrameInfo dw_call_frame_info_from_data(Arena *arena, Arch arch, U64 rebase, String8 debug_frame);

#endif // DWARF_HELP_H
