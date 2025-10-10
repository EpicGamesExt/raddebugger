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

typedef struct DW_CFA_Row
{
  DW_CFA          cfa;
  DW_CFI_Register *regs;
  struct DW_CFA_Row *next;
} DW_CFA_Row;

typedef struct DW_CFI_Unwind
{
  DW_CFA_InstList  insts;
  DW_CIE          *cie;
  DW_FDE          *fde;
  DW_CFA_Row      *initial_row;
  DW_CFA_Row      *row;
  DW_CFA_InstNode *curr_inst;
  DW_CFA_Row      *free_rows;
  DW_Reg           reg_count;
  U64              pc;
  Arch             arch;
} DW_CFI_Unwind;

typedef enum {
  DW_UnwindStatus_Ok,
  DW_UnwindStatus_Fail,
  DW_UnwindStatus_Maybe
} DW_UnwindStatus;

#define DW_REG_READ(name) DW_UnwindStatus name(DW_Reg reg_id, void *buffer, U64 buffer_max, void *ud)
typedef DW_REG_READ(DW_RegRead);

#define DW_REG_WRITE(name) DW_UnwindStatus name(DW_Reg reg_id, void *value, U64 value_size, void *ud)
typedef DW_REG_WRITE(DW_RegWrite);

#define DW_MEM_READ(name) DW_UnwindStatus name(U64 addr, U64 size, void *buffer, void *ud)
typedef DW_MEM_READ(DW_MemRead);

////////////////////////////////

internal DW_CFA_Row * dw_make_cfa_row(Arena *arena, U64 reg_count);
internal DW_CFA_Row * dw_copy_cfa_row(Arena *arena, U64 reg_count, DW_CFA_Row *row);

internal DW_CFI_Unwind * dw_cfi_unwind_init(Arena *arena, Arch arch, DW_CIE *cie, DW_FDE *fde, DW_DecodePtr *decode_ptr_func, void *decode_ptr_ud);
internal B32             dw_cfi_next_row(Arena *arena, DW_CFI_Unwind *uw);
internal DW_UnwindStatus dw_cfi_apply_register_rules(DW_CFI_Unwind *uw, DW_MemRead *mem_read_func, void *mem_read_ud, DW_RegRead *reg_read_func,  void *reg_read_ud, DW_RegWrite *reg_write_func, void *reg_write_ud);

#endif // DWARF_UNWIND_H

