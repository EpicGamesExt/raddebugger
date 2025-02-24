// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
rdi_arch_from_coff_machine(COFF_MachineType machine)
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
rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags)
{
  RDI_BinarySectionFlags result = 0;
  if (flags & COFF_SectionFlag_MemRead) {
    result |= RDI_BinarySectionFlag_Read;
  }
  if (flags & COFF_SectionFlag_MemWrite) {
    result |= RDI_BinarySectionFlag_Write;
  }
  if (flags & COFF_SectionFlag_MemExecute) {
    result |= RDI_BinarySectionFlag_Execute;
  }
  return result;
}
