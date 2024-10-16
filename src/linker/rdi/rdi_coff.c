// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDI_Arch
rdi_arch_from_coff_machine(COFF_MachineType machine)
{
  switch (machine) {
  case COFF_MachineType_X86: return RDI_Arch_X86;
  case COFF_MachineType_X64: return RDI_Arch_X64;

  case COFF_MachineType_UNKNOWN:
  case COFF_MachineType_AM33:
  case COFF_MachineType_ARM:
  case COFF_MachineType_ARM64:
  case COFF_MachineType_ARMNT:
  case COFF_MachineType_EBC:
  case COFF_MachineType_IA64:
  case COFF_MachineType_M32R:
  case COFF_MachineType_MIPS16:
  case COFF_MachineType_MIPSFPU:
  case COFF_MachineType_MIPSFPU16:
  case COFF_MachineType_POWERPC:
  case COFF_MachineType_POWERPCFP:
  case COFF_MachineType_R4000:
  case COFF_MachineType_RISCV32:
  case COFF_MachineType_RISCV64:
  case COFF_MachineType_SH3:
  case COFF_MachineType_SH3DSP:
  case COFF_MachineType_SH4:
  case COFF_MachineType_SH5:
  case COFF_MachineType_THUMB:
  case COFF_MachineType_WCEMIPSV2:
    NotImplemented;
  default:
    return RDI_Arch_NULL;
  }
}

internal RDI_BinarySectionFlags
rdi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags)
{
  RDI_BinarySectionFlags result = 0;
  if (flags & COFF_SectionFlag_MEM_READ) {
    result |= RDI_BinarySectionFlag_Read;
  }
  if (flags & COFF_SectionFlag_MEM_WRITE) {
    result |= RDI_BinarySectionFlag_Write;
  }
  if (flags & COFF_SectionFlag_MEM_EXECUTE) {
    result |= RDI_BinarySectionFlag_Execute;
  }
  return result;
}
