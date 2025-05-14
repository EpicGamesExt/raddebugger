// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
rdi_arch_from_coff_machine(COFF_MachineType machine)
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
