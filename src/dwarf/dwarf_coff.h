// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_COFF_H
#define DWARF_COFF_H

internal DW_Input dw_input_from_coff_section_table(Arena *arena, String8 raw_image, String8 string_table, U64 section_count, COFF_SectionHeader *section_table);

#endif // DWARF_COFF_H

