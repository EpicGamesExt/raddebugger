// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ARCH_INC_H
#define ARCH_INC_H

#include "arch/arch.h"
#include "arch/os/arch_os.h"
#if defined(DWARF_H)
# include "arch/dwarf/arch_dwarf.h"
#endif
#if defined(RDI_H)
# include "arch/rdi/arch_rdi.h"
#endif

#endif // ARCH_INC_H
