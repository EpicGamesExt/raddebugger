// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "arch/arch.c"
#include "arch/os/arch_os.c"
#if defined(DWARF_H)
# include "arch/dwarf/arch_dwarf.c"
#endif
#if defined(RDI_H)
# include "arch/rdi/arch_rdi.c"
#endif
#if defined(ELF_H)
# include "arch/elf/arch_elf.c"
#endif
