// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef REGS_DWARF_H
#define REGS_DWARF_H

internal REGS_RegCode reg_code_from_dw_reg_x64(DW_Reg reg);
internal REGS_RegCode reg_code_from_dw_reg(Arch arch, DW_Reg reg);

internal MACHINE_OP_REG_READ(regs_read_dwarf_x64);
internal MACHINE_OP_REG_WRITE(regs_write_dwarf_x64);

#endif // REGS_DWARF_H

