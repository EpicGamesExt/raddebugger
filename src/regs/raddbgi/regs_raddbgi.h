// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef REGS_RADDBGI_H
#define REGS_RADDBGI_H

internal RADDBGI_RegisterCode regs_raddbgi_code_from_arch_reg_code(Architecture arch, REGS_RegCode code);
internal REGS_RegCode regs_reg_code_from_arch_raddbgi_code(Architecture arch, RADDBGI_RegisterCode reg);

#endif //REGS_RADDBGI_H
