// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
c2r_rdi_arch_from_coff_machine(COFF_MachineType machine)
{
  switch (machine) {
  case COFF_Machine_X86: return RDI_Arch_X86;
  case COFF_Machine_X64: return RDI_Arch_X64;

  case COFF_Machine_Unknown:
  case COFF_Machine_Am33:
  case COFF_Machine_Arm:
  case COFF_Machine_Arm64:
  case COFF_Machine_ArmNt:
  case COFF_Machine_Ebc:
  case COFF_Machine_Ia64:
  case COFF_Machine_M32R:
  case COFF_Machine_Mips16:
  case COFF_Machine_MipsFpu:
  case COFF_Machine_MipsFpu16:
  case COFF_Machine_PowerPc:
  case COFF_Machine_PowerPcFp:
  case COFF_Machine_R4000:
  case COFF_Machine_RiscV32:
  case COFF_Machine_RiscV64:
  case COFF_Machine_Sh3:
  case COFF_Machine_Sh3Dsp:
  case COFF_Machine_Sh4:
  case COFF_Machine_Sh5:
  case COFF_Machine_Thumb:
  case COFF_Machine_WceMipsV2:
    NotImplemented;
  default:
    return RDI_Arch_NULL;
  }
}

internal RDI_BinarySectionFlags
c2r_rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags)
{
  RDI_BinarySectionFlags result = 0;
  if(flags & COFF_SectionFlag_MemRead)
  {
    result |= RDI_BinarySectionFlag_Read;
  }
  if(flags & COFF_SectionFlag_MemWrite)
  {
    result |= RDI_BinarySectionFlag_Write;
  }
  if(flags & COFF_SectionFlag_MemExecute)
  {
    result |= RDI_BinarySectionFlag_Execute;
  }
  return(result);
}

internal RDIM_BinarySectionList
c2r_rdi_binary_sections_from_coff_sections(Arena *arena, String8 image_data, U64 string_table_off, U64 sectab_count, COFF_SectionHeader *sectab)
{
  ProfBeginFunction();

  RDIM_BinarySectionList binary_sections = {0};

  for (U64 isec = 0; isec < sectab_count; ++isec) {
    COFF_SectionHeader *coff_sec = &sectab[isec];
    RDIM_BinarySection *sec      = rdim_binary_section_list_push(arena, &binary_sections);

    sec->name       = coff_name_from_section_header(image_data, coff_sec, string_table_off);
    sec->flags      = c2r_rdi_binary_section_flags_from_coff_section_flags(coff_sec->flags);
    sec->voff_first = coff_sec->voff;
    sec->voff_opl   = coff_sec->voff + coff_sec->vsize;
    sec->foff_first = coff_sec->foff;
    sec->foff_opl   = coff_sec->foff + coff_sec->fsize;
  }

  ProfEnd();
  return binary_sections;
}

