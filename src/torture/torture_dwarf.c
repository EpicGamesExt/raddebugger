// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Dwarf"

internal U64
t_dw_test_uleb128(U64 v, U64 expected_length)
{
  Temp scratch = scratch_begin(0, 0);
  B32 is_ok = 0;

  U64 v0 = v;
  String8 e = dw_write_uleb128(scratch.arena, v0);
  if (!(expected_length == e.size)) { goto exit; }

  U64 v1;
  U64 bytes_read = str8_deserial_read_uleb128(e, 0, &v1);
  if (!(bytes_read == e.size)) { goto exit; }
  if (!(v0 == v1)) { goto exit; }

  is_ok = 1;
exit:;
  scratch_end(scratch);
  return is_ok;
}

internal U64
t_dw_test_sleb128(U64 v, U64 expected_length)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = 0;

  U64 v0 = v;
  String8 e = dw_write_sleb128(scratch.arena, v0);
  if (!(expected_length == e.size)) { goto exit; }

  U64 v1;
  U64 bytes_read = str8_deserial_read_sleb128(e, 0, &v1);
  if (!(bytes_read == e.size)) { goto exit; }
  if (!(v0 == v1)) { goto exit; }

  is_ok = 1;
exit:;
  scratch_end(scratch);
  return is_ok;
}

T_BeginTest(test_leb128)
{
  T_Ok(t_dw_test_uleb128(0, 1));
  T_Ok(t_dw_test_sleb128(0, 1));
  T_Ok(t_dw_test_sleb128(-1, 1));

  T_Ok(t_dw_test_uleb128(max_U64, 10));
  T_Ok(t_dw_test_sleb128(min_S64, 10));
  T_Ok(t_dw_test_sleb128(max_S64, 10));

  for EachIndex(i, 64) {
    T_Ok(t_dw_test_uleb128((1ull << i), 1 + (i / 7)));
  }
  for EachIndex(i, 64) {
    T_Ok(t_dw_test_sleb128((1ull << i), 1 + (i + 1) / 7));
  }
}
T_EndTest;

T_BeginTest(value_in_register)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_Reg3 - DW_ExprOp_Reg0);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 0xc0ffee;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  // compile a simple program which reads the value from register 3
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Reg3) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);
  
  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_U64);
  T_Ok(expr_value.u64 == value);
}
T_EndTest;

T_BeginTest(value_in_x_register)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_RegX64_FsBase);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 0xc0ffee;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  // compile a simple program which reads the value from register 3
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(RegX), DW_ExprEnc_ULEB128(DW_RegX64_FsBase) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_U64);
  T_Ok(expr_value.u64 == value);
}
T_EndTest;

T_BeginTest(address_of_value)
{
  // compile a simple program which reads address
  U64 addr = 0xdeadbeef;
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Addr), DW_ExprEnc_U64(addr) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, 0, 0, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == addr);
}
T_EndTest;

T_BeginTest(register_relative_variable)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg11 - DW_ExprOp_BReg0);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 1;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(BReg11), DW_ExprEnc_SLEB128(44) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == (1 + 44));
}
T_EndTest;

T_BeginTest(frame_relative_variable)
{
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(FBReg), DW_ExprEnc_SLEB128(-50) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  U64               frame_base = 123;
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, frame_base, 0, 0, max_U64, expr, 0, 0, 0, 0, &expr_value);

  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == frame_base -50);
}
T_EndTest;

internal
MACHINE_OP_MEM_READ(t_machine_op_mem_read)
{
  MemoryCopy(buffer, PtrFromInt(addr), buffer_size);
  return MachineOpResult_Ok;
}

T_BeginTest(call_by_reference)
{
  U8 *memory = push_array(scratch.arena, U8, 128);
  U64 value = 0xc0ffee;
  MemoryCopy(memory + 32, &value, sizeof(value));

  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, 58); // fsbase
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 memory_ptr = IntFromPtr(memory);
  MemoryCopy((U8 *)&regs + reg_range.min, &memory_ptr, sizeof(memory_ptr));

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(BRegX), DW_ExprEnc_ULEB128(58), DW_ExprEnc_SLEB128(32), DW_ExprEnc_Op(Deref) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value = { 0 };
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, t_machine_op_mem_read, 0, &expr_value);

  T_Ok(expr_value.type == DW_ExprValueType_Generic);
  T_Ok(expr_value.generic.size == sizeof(U64));
  T_Ok(*(U64 *)expr_value.generic.str == value);
}
T_EndTest;

T_BeginTest(plus_uconst)
{
  U64 struct_addr = 0x123;
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Addr), DW_ExprEnc_Addr(struct_addr), DW_ExprEnc_Op(PlusUConst), DW_ExprEnc_ULEB128(4) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, 0, 0, t_machine_op_mem_read, 0, &expr_value);

  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == 0x123 + 4);
}
T_EndTest;

#if 0
T_BeginTest(reg_split_spill)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  {
    REGS_RegCode reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg3 - DW_ExprOp_BReg0);
    Rng1U64      reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
    U64          value     = 0xaaaa;
    MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));
  }
  {
    REGS_RegCode reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg10 - DW_ExprOp_BReg0);
    Rng1U64      reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
    U64          value     = 0xbbbb;
    MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));
  }

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Reg3), DW_ExprEnc_Op(Piece), DW_ExprEnc_ULEB128(4), DW_ExprEnc_Op(Reg10), DW_ExprEnc_Op(Piece), DW_ExprEnc_ULEB128(2) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);
}
T_EndTest;
#endif

#undef T_Group

