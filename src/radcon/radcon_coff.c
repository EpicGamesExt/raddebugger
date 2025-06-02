// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
c2r_rdi_arch_from_coff_machine(COFF_MachineType machine)
{
  switch (machine) {
    case COFF_MachineType_X86: return RDI_Arch_X86;
    case COFF_MachineType_X64: return RDI_Arch_X64;
    
    case COFF_MachineType_Unknown:
    case COFF_MachineType_Am33:
    case COFF_MachineType_Arm:
    case COFF_MachineType_Arm64:
    case COFF_MachineType_ArmNt:
    case COFF_MachineType_Ebc:
    case COFF_MachineType_Ia64:
    case COFF_MachineType_M32R:
    case COFF_MachineType_Mips16:
    case COFF_MachineType_MipsFpu:
    case COFF_MachineType_MipsFpu16:
    case COFF_MachineType_PowerPc:
    case COFF_MachineType_PowerPcFp:
    case COFF_MachineType_R4000:
    case COFF_MachineType_RiscV32:
    case COFF_MachineType_RiscV64:
    case COFF_MachineType_Sh3:
    case COFF_MachineType_Sh3Dsp:
    case COFF_MachineType_Sh4:
    case COFF_MachineType_Sh5:
    case COFF_MachineType_Thumb:
    case COFF_MachineType_WceMipsV2:
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
c2r_rdi_binary_sections_from_coff_sections(Arena *arena, String8 image_data, String8 string_table, U64 sectab_count, COFF_SectionHeader *sectab)
{
  ProfBeginFunction();
  
  RDIM_BinarySectionList binary_sections = {0};
  
  for (U64 isec = 0; isec < sectab_count; ++isec) {
    COFF_SectionHeader *coff_sec = &sectab[isec];
    RDIM_BinarySection *sec      = rdim_binary_section_list_push(arena, &binary_sections);
    
    sec->name       = coff_name_from_section_header(string_table, coff_sec);
    sec->flags      = c2r_rdi_binary_section_flags_from_coff_section_flags(coff_sec->flags);
    sec->voff_first = coff_sec->voff;
    sec->voff_opl   = coff_sec->voff + coff_sec->vsize;
    sec->foff_first = coff_sec->foff;
    sec->foff_opl   = coff_sec->foff + coff_sec->fsize;
  }
  
  ProfEnd();
  return binary_sections;
}

