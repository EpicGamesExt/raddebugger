// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_ELF_H
#define DWARF_ELF_H

internal B32 dw_is_dwarf_present_from_elf_bin(String8 raw_image, ELF_Bin *bin);
internal DW_Raw dw_raw_from_elf_bin(Arena *arena, String8 raw_image, ELF_Bin *bin);

#endif // DWARF_ELF_H
