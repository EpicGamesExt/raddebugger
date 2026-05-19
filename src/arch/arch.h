// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef ARCH_H
#define ARCH_H

////////////////////////////////
//~ rjf: Abstraction Backend Info

typedef U8 ARCH_RegCode;

typedef struct ARCH_Info ARCH_Info;
struct ARCH_Info
{
  U16 reg_block_size;
  ARCH_RegCode instruction_pointer_reg_code;
  ARCH_RegCode stack_pointer_reg_code;
  U16 reg_code_count;
  String8 trap_instruction;
  Rng1U16 *reg_code_rng_table;
  String8 *reg_code_name_table;
  U8 *reg_code_base_table;
  B8 *reg_code_is_vector_table;
};

////////////////////////////////
//~ rjf: Globals

global read_only Rng1U16 arch_reg_code_rng_nil = {0};
global read_only String8 arch_reg_code_name_nil = {0};
global read_only U8 arch_reg_code_u8_nil = 0;
global read_only B8 arch_reg_code_b8_nil = 0;
global read_only ARCH_Info arch_info_nil = {0, 0, 0, 0, {0}, &arch_reg_code_rng_nil, &arch_reg_code_name_nil, &arch_reg_code_u8_nil, &arch_reg_code_b8_nil};

////////////////////////////////
//~ rjf: Abstracted Architecture Functions

internal ARCH_Info *arch_info_from_arch(Arch arch);
internal ARCH_RegCode arch_reg_code_from_name(ARCH_Info *arch_info, String8 name);
internal ARCH_RegCode arch_reg_code_from_rdi(Arch arch, RDI_RegCode code);
internal ARCH_RegCode arch_reg_code_from_dw(Arch arch, DW_Reg code);
internal B32 arch_reg_block_read_range(ARCH_Info *arch_info, void *block, Rng1U16 range, void *dst);
internal B32 arch_reg_block_write_range(ARCH_Info *arch_info, void *block, Rng1U16 range, void *src);
internal U64 arch_ip_from_reg_block(ARCH_Info *arch_info, void *block);
internal U64 arch_sp_from_reg_block(ARCH_Info *arch_info, void *block);
internal B32 arch_reg_block_write_ip(ARCH_Info *arch_info, void *block, U64 ip);
internal B32 arch_reg_block_write_sp(ARCH_Info *arch_info, void *block, U64 sp);

#endif // REGS_H
