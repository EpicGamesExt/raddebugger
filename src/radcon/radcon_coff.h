// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADCON_COFF_H
#define RADCON_COFF_H

internal RDI_Arch               c2r_rdi_arch_from_coff_machine(COFF_MachineType machine);
internal RDI_BinarySectionFlags c2r_rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);
internal RDIM_BinarySectionList c2r_rdi_binary_sections_from_coff_sections(Arena *arena, String8 image_data, String8 string_table, U64 sectab_count, COFF_SectionHeader *sectab);

#endif // RADCON_COFF_H

