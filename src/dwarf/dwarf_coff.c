// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal DW_SectionArray dw_sections_from_coff_section_table(Arena *arena, String8 raw_image, U64 string_table_off, U64 section_count, COFF_SectionHeader *sections);

