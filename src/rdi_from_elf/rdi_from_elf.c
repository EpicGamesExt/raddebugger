// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_BinarySectionFlags
e2r_rdi_binary_section_flags_from_elf(ELF_SectionFlags f)
{
  RDI_BinarySectionFlags result = RDI_BinarySectionFlag_Read;
  if(f & ELF_Shf_Write)
  {
    result |= RDI_BinarySectionFlag_Write;
  }
  if(f & ELF_Shf_ExecInstr)
  {
    result |= RDI_BinarySectionFlag_Execute;
  }
  return result;
}

internal RDIM_BinarySectionList
e2r_rdi_binary_sections_from_elf_section_table(Arena *arena, String8 data, ELF_Bin *bin, ELF_Shdr64Array shdrs)
{
  RDIM_BinarySectionList result = {0};
  U64 base_vaddr = elf_base_addr_from_bin(bin);
  for EachIndex(idx, shdrs.count)
  {
    ELF_Shdr64 *src = &shdrs.v[idx];
    RDIM_BinarySection *dst = rdim_binary_section_list_push(arena, &result);
    dst->name       = elf_name_from_shdr64(data, bin, src);
    dst->flags      = e2r_rdi_binary_section_flags_from_elf(src->sh_flags);
    dst->voff_first = src->sh_addr - base_vaddr;
    dst->voff_opl   = dst->voff_first + src->sh_size;
    dst->foff_first = src->sh_offset;
    dst->foff_opl   = dst->foff_first + src->sh_size;
  }
  return result;
}
