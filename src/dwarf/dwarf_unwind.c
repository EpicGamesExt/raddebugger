// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal DW_CFA_Row *
dw_make_cfa_row(Arena *arena, U64 reg_count)
{
  DW_CFA_Row *row = push_array(arena, DW_CFA_Row, 1);
  row->regs = push_array(arena, DW_CFI_Register, reg_count);
  for EachIndex(reg_idx, reg_count) {
    row->regs[reg_idx].rule = DW_CFI_RegisterRule_SameValue;
  }
  return row;
}

internal DW_CFA_Row *
dw_copy_cfa_row(Arena *arena, U64 reg_count, DW_CFA_Row *row)
{
  DW_CFA_Row *new_row = dw_make_cfa_row(arena, reg_count);
  new_row->cfa = row->cfa;
  MemoryCopyTyped(new_row->regs, row->regs, reg_count);
  return new_row;
}

internal DW_CFI_Unwind *
dw_cfi_unwind_init(Arena        *arena,
                   Arch          arch,
                   DW_CIE       *cie,
                   DW_FDE       *fde,
                   DW_DecodePtr *decode_ptr_func,
                   void         *decode_ptr_ud)
{
  Temp scratch = scratch_begin(&arena, 1);

  DW_CFI_Unwind *uw = push_array(arena, DW_CFI_Unwind, 1);
  uw->insts     = dw_parse_cfa_inst_list(arena, fde->insts, cie->code_align_factor, cie->data_align_factor, decode_ptr_func, decode_ptr_ud);
  uw->cie       = cie;
  uw->fde       = fde;
  uw->reg_count = dw_reg_count_from_arch(arch);
  uw->pc        = fde->pc_range.min;
  uw->arch      = arch;

  // setup initial register rules
  DW_CFA_InstList initial_insts = dw_parse_cfa_inst_list(scratch.arena, cie->insts, cie->code_align_factor, cie->data_align_factor, decode_ptr_func, decode_ptr_ud);
  uw->row       = dw_make_cfa_row(arena, uw->reg_count);
  uw->curr_inst = initial_insts.first;
  dw_cfi_next_row(arena, uw);

  // make first row from initial rules
  uw->initial_row = uw->row;
  uw->row         = dw_copy_cfa_row(arena, uw->reg_count, uw->initial_row);
  uw->curr_inst   = uw->insts.first;

  scratch_end(scratch);
  return uw;
}

internal B32
dw_cfi_next_row(Arena *arena, DW_CFI_Unwind *uw)
{
  B32 is_row_valid = 0;

  while (uw->curr_inst) {
    DW_CFA_Inst *inst = &uw->curr_inst->v;

    // validate register operands
    DW_CFA_OperandType *operand_types = dw_operand_types_from_cfa_op(inst->opcode);
    U64                operand_count = dw_operand_count_from_cfa_opcode(inst->opcode);
    for EachIndex(operand_idx, operand_count) {
      if (operand_types[operand_idx] == DW_CFA_OperandType_Register) {
        if (inst->operands[operand_idx].u64 >= uw->reg_count) {
          goto exit;
        }
      }
    }

    switch (inst->opcode) {
    // Row Creation Instructions
    case DW_CFA_SetLoc: {
      uw->pc = inst->operands[0].u64;
    } break;
    case DW_CFA_AdvanceLoc:
    case DW_CFA_AdvanceLoc1:
    case DW_CFA_AdvanceLoc2:
    case DW_CFA_AdvanceLoc4: {
      uw->pc += inst->operands[0].u64;
    } break;

    // CFA Definition Instructions
    case DW_CFA_DefCfa:
    case DW_CFA_DefCfaSf: {
      U64 reg = inst->operands[0].u64;
      S64 off = inst->operands[1].s64;
      uw->row->cfa.rule = DW_CFA_Rule_RegOff;
      uw->row->cfa.reg  = reg;
      uw->row->cfa.off  = off;
    } break;
    case DW_CFA_DefCfaRegister: {
      // TODO: report error: this operation is valid only if the current CFA rule is defined to register+offset
      if (uw->row->cfa.rule != DW_CFA_Rule_RegOff) { goto exit; }
      U64 reg = inst->operands[0].u64;
      uw->row->cfa.reg = reg;
    } break;
    case DW_CFA_DefCfaOffset:
    case DW_CFA_DefCfaOffsetSf: {
      // TODO: report error: this operation is valid only if the current CFA rule is defined to register+offset
      if (uw->row->cfa.rule != DW_CFA_Rule_RegOff) { goto exit; }
      uw->row->cfa.off = inst->operands[0].s64;
    } break;
    case DW_CFA_DefCfaExpr: {
      uw->row->cfa.rule = DW_CFA_Rule_Expression;
      uw->row->cfa.expr = inst->operands[0].block;
    } break;

    // Register Rule Instructions
    case DW_CFA_Undefined: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_Undefined;
    } break;
    case DW_CFA_SameValue: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_SameValue;
    } break;
    case DW_CFA_Offset:
    case DW_CFA_OffsetExt:
    case DW_CFA_OffsetExtSf: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_Offset;
      uw->row->regs[reg].n    = inst->operands[1].s64;
    } break;
    case DW_CFA_ValOffset:
    case DW_CFA_ValOffsetSf: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_ValOffset;
      uw->row->regs[reg].n    = inst->operands[1].s64;
    } break;
    case DW_CFA_Register: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_Register;
      uw->row->regs[reg].n    = inst->operands[1].s64;
    } break;
    case DW_CFA_Expr: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_Expression;
      uw->row->regs[reg].expr = inst->operands[1].block;
    } break;
    case DW_CFA_ValExpr: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg].rule = DW_CFI_RegisterRule_ValExpression;
      uw->row->regs[reg].expr = inst->operands[1].block;
    } break;
    case DW_CFA_Restore:
    case DW_CFA_RestoreExt: {
      U64 reg = inst->operands[0].u64;
      uw->row->regs[reg] = uw->initial_row->regs[reg];
    } break;
      
    // Row State Instructions
    case DW_CFA_RememberState: {
      DW_CFA_Row *new_row = dw_copy_cfa_row(arena, uw->reg_count, uw->row);
      SLLStackPush(uw->row, new_row);
    } break;
    case DW_CFA_RestoreState: {
      if (uw->row == 0) { goto exit; } // TODO: report error: unbalanced number of pushes and pops
      DW_CFA_Row *free_row = uw->row;
      SLLStackPop(uw->row);
      SLLStackPush(uw->free_rows, free_row);
    } break;

    case DW_CFA_Nop: {} break;

    default: { NotImplemented; } break; // TODO: report error: unknown CFA opcode
    }

    is_row_valid = 1;

    uw->curr_inst = uw->curr_inst->next;
    if (uw->curr_inst) {
      if (dw_is_new_row_cfa_opcode(uw->curr_inst->v.opcode)) { break; }
    }
  }

exit:;
  return is_row_valid;
}

internal DW_UnwindStatus
dw_cfi_apply_register_rules(DW_CFI_Unwind *uw,
                            DW_MemRead    *mem_read_func,  void *mem_read_ud,
                            DW_RegRead    *reg_read_func,  void *reg_read_ud,
                            DW_RegWrite   *reg_write_func, void *reg_write_ud)
{
  Temp scratch = scratch_begin(0,0);
  DW_UnwindStatus unwind_status = DW_UnwindStatus_Ok;

  DW_CFA_Row *row = uw->row;

  // establish CFA
  U64 cfa = 0;
  switch (row->cfa.rule) {
  case DW_CFA_Rule_Null: break;
  case DW_CFA_Rule_RegOff: {
    // TODO: report error (invalid register read)
    U64 cfa_reg_value = 0;
    U64 reg_size = dw_reg_size_from_code(uw->arch, row->cfa.reg);
    AssertAlways(reg_size <= sizeof(cfa_reg_value));
    unwind_status = reg_read_func(row->cfa.reg, &cfa_reg_value, reg_size, reg_read_ud);
    if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }
    cfa = cfa_reg_value + row->cfa.off;
  } break;
  case DW_CFA_Rule_Expression: {
    // TODO: evaluate expression
  } break;
  }

  U64   max_reg_size = dw_reg_max_size_from_arch(uw->arch);
  void *reg_buffer   = push_array(scratch.arena, U8, max_reg_size);

  for EachIndex(reg_idx, uw->reg_count) {
    DW_CFI_Register *reg = &uw->row->regs[reg_idx];
    switch (reg->rule) {
    case DW_CFI_RegisterRule_Undefined: {
      // TODO: ???
      Assert(!"undefined");
    } break;
    case DW_CFI_RegisterRule_SameValue: {} break;
    case DW_CFI_RegisterRule_Offset: {
      // read register value from memory
      U64 addr     = cfa + reg->n;
      U64 reg_size = dw_reg_size_from_code(uw->arch, reg_idx);
      unwind_status = mem_read_func(addr, reg_size, reg_buffer, mem_read_ud);
      if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }

      // write register value to the thread context
      unwind_status = reg_write_func(reg_idx, reg_buffer, reg_size, reg_write_ud);
      if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }
    } break;
    case DW_CFI_RegisterRule_ValOffset: {
      // compute register value
      U64 reg_value = cfa + reg->n;

      // write register value to the thread context
      U64 reg_size = dw_reg_size_from_code(uw->arch, reg_idx);
      unwind_status = reg_write_func(reg_idx, &reg_value, reg_size, reg_write_ud);
      if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }
    } break;
    case DW_CFI_RegisterRule_Register: {
      // read register value from another register
      U64 reg_size = dw_reg_size_from_code(uw->arch, reg_idx);
      unwind_status = reg_read_func(reg->n, reg_buffer, reg_size, reg_read_ud);
      if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }

      // write register value to the thread context
      unwind_status = reg_write_func(reg_idx, reg_buffer, reg_size, reg_write_ud);
      if (unwind_status != DW_UnwindStatus_Ok) { goto exit; }
    } break;
    case DW_CFI_RegisterRule_Expression: {
      // TODO: evaluate expression
      NotImplemented;
    } break;
    case DW_CFI_RegisterRule_ValExpression: {
      // TODO: evaluate expression
      NotImplemented;
    } break;
    case DW_CFI_RegisterRule_Architectural: {
      NotImplemented;
    } break;
    }
  }
  
exit:;
  scratch_end(scratch);
  return unwind_status;
}

