// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_UNWIND_H
#define DWARF_UNWIND_H

typedef enum DW_CFI_RegisterRule
{
  DW_CFI_RegisterRule_Undefined,
  DW_CFI_RegisterRule_SameValue,
  DW_CFI_RegisterRule_Offset,
  DW_CFI_RegisterRule_ValOffset,
  DW_CFI_RegisterRule_Register,
  DW_CFI_RegisterRule_Expression,
  DW_CFI_RegisterRule_ValExpression,
  DW_CFI_RegisterRule_Architectural
} DW_CFI_RegisterRule;

typedef enum DW_CFA_Rule
{
  DW_CFA_Rule_Null,
  DW_CFA_Rule_RegOff,
  DW_CFA_Rule_Expression
} DW_CFA_Rule;

typedef struct DW_CFA
{
  DW_CFA_Rule rule;
  union {
    struct {
      DW_Reg reg;
      S64    off;
    };
    String8 expr;
  };
} DW_CFA;

typedef struct DW_CFI_Register
{
  DW_CFI_RegisterRule rule;
  union {
    S64     n;
    String8 expr;
  };
} DW_CFI_Register;

typedef struct DW_CFI_Row
{
  DW_CFA           cfa;
  DW_CFI_Register *regs;
  struct DW_CFI_Row *next;
} DW_CFI_Row;

typedef struct DW_CFI_Unwind
{
  DW_CFA_InstList  insts;
  DW_CIE          *cie;
  DW_FDE          *fde;
  DW_CFI_Row      *initial_row;
  DW_CFI_Row      *row;
  DW_CFA_InstNode *curr_inst;
  DW_CFI_Row      *free_rows;
  U64              pc;
  Arch             arch;
  U64              reg_count;
} DW_CFI_Unwind;

////////////////////////////////

internal DW_CFI_Row * dw_make_cfi_row(Arena *arena, U64 reg_count);
internal DW_CFI_Row * dw_copy_cfi_row(Arena *arena, DW_CFI_Row *row, U64 reg_count);

internal DW_CFI_Unwind * dw_cfi_unwind_init(Arena *arena, Arch arch, DW_CIE *cie, DW_FDE *fde, DW_DecodePtr *decode_ptr_func, void *decode_ptr_ud);
internal B32             dw_cfi_next_row(Arena *arena, DW_CFI_Unwind *uw);

internal DW_CFI_Row *    dw_cfi_row_from_pc(Arena *arena, Arch arch, struct DW_CIE *cie, struct DW_FDE *fde, DW_DecodePtr *decode_ptr_func, void *decode_ptr_ctx, U64 pc);
internal DW_UnwindStatus dw_cfi_apply_register_rules(Arch arch, U64 cfa, DW_CFI_Row *row, DW_MemRead *mem_read_func, void *mem_read_ud, DW_RegRead *reg_read_func,  void *reg_read_ud, DW_RegWrite *reg_write_func, void *reg_write_ud);

#endif // DWARF_UNWIND_H

