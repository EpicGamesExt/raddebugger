// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADCON_ELF_H
#define RADCON_ELF_H

internal RDIM_BinarySectionList e2r_rdi_binary_sections_from_elf_section_table(Arena *arena, ELF_Shdr64Array shdrs);
internal RDIM_TopLevelInfo      e2r_make_rdim_top_level_info(String8 image_name, RDI_Arch arch, ELF_Shdr64Array shdrs);

#endif // RADCON_ELF_H
