// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef REGS_RADDBG_H
#define REGS_RADDBG_H

internal RADDBG_RegisterCode regs_raddbg_code_from_arch_reg_code(Architecture arch, REGS_RegCode code);
internal REGS_RegCode regs_reg_code_from_arch_raddbg_code(Architecture arch, RADDBG_RegisterCode reg);

#endif //REGS_RADDBG_H
