// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "arch/arch.c"
#if defined(DWARF_H)
# include "arch/dwarf/arch_dwarf.c"
#endif
#if defined(RDI_H)
# include "arch/rdi/arch_rdi.c"
#endif
