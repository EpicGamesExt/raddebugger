// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_FROM_ELF_H
#define RDI_FROM_ELF_H

internal RDI_BinarySectionFlags e2r_rdi_binary_section_flags_from_elf(ELF_SectionFlags f);
internal RDIM_BinarySectionList e2r_rdi_binary_sections_from_elf_section_table(Arena *arena, String8 data, ELF_Bin *bin, ELF_Shdr64Array shdrs);

#endif // RDI_FROM_ELF_H
