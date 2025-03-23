// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_COFF_H
#define DWARF_COFF_H

internal DW_Input dw_input_from_coff_section_table(Arena *arena, String8 raw_image, U64 string_table_off, U64 section_count, COFF_SectionHeader *sections);

#endif // DWARF_COFF_H

