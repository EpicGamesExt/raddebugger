// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef REGS_RDI_H
#define REGS_RDI_H

internal RDI_RegisterCode regs_rdi_code_from_arch_reg_code(Architecture arch, REGS_RegCode code);
internal REGS_RegCode regs_reg_code_from_arch_rdi_code(Architecture arch, RDI_RegisterCode reg);

#endif //REGS_RDI_H
