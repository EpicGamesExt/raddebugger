// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal Arch
arch_from_elf_machine(ELF_MachineKind e_machine)
{
  Arch arch = Arch_Null;
  switch (e_machine) {
  case ELF_MachineKind_None:    arch = Arch_Null;  break;
  case ELF_MachineKind_AARCH64: arch = Arch_arm32; break;
  case ELF_MachineKind_ARM:     arch = Arch_arm32; break;
  case ELF_MachineKind_386:     arch = Arch_x86;   break;
  case ELF_MachineKind_X86_64:  arch = Arch_x64;   break;
  default: NotImplemented; break;
  }
  return arch;
}

