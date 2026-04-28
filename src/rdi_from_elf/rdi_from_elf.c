// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDIM_BinarySectionList
e2r_rdi_binary_sections_from_elf_section_table(Arena *arena, String8 data, ELF_Bin *bin, ELF_Shdr64Array *shdrs)
{
  RDIM_BinarySectionList result = {0};
  for EachIndex(idx, shdrs->count)
  {
    // rjf: unpack section
    ELF_Shdr64 *src_section = &shdrs->v[idx];
    String8 name = elf_name_from_shdr64(data, bin, src_section);
    U64 voff_first = src_section->sh_addr;
    U64 voff_opl   = voff_first + src_section->sh_size;
    U64 foff_first = src_section->sh_offset;
    U64 foff_opl   = foff_first + src_section->sh_size;
    
    // rjf: map flags -> rdi
    RDI_BinarySectionFlags flags = RDI_BinarySectionFlag_Read;
    if(src_section->sh_flags & ELF_Shf_Write)     { flags |= RDI_BinarySectionFlag_Write; }
    if(src_section->sh_flags & ELF_Shf_ExecInstr) { flags |= RDI_BinarySectionFlag_Execute; }
    
    // rjf: make rdi section
    RDIM_BinarySection *dst_section = rdim_binary_section_list_push(arena, &result);
    dst_section->name       = name;
    dst_section->flags      = flags;
    dst_section->voff_first = voff_first;
    dst_section->voff_opl   = voff_opl;
    dst_section->foff_first = foff_first;
    dst_section->foff_opl   = foff_opl;
  }
  return result;
}
