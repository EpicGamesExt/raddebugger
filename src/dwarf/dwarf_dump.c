// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Stringification Helpers

internal String8
dw_string_from_reg_off(Arena *arena, Arch arch, U64 reg_idx, S64 reg_off)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 reg_str = dw_string_from_register(scratch.arena, arch, reg_idx);
  String8 result;
  if (reg_off != 0) {
    result = push_str8f(arena, "%S%+lld", reg_str, reg_off);
  } else {
    result = reg_str;
  }
  scratch_end(scratch);
  return result;
}

internal String8List
dw_string_list_from_expression(Arena *arena, String8 raw_data, U64 cu_base, U64 addr_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List result = {0};
  for (U64 cursor = 0; cursor < raw_data.size; ) {
    U8 op = 0;
    cursor += str8_deserial_read_struct(raw_data, cursor, &op);
    
    String8 op_value   = str8_zero();
    U64     size_param = 0;
    B32     is_signed  = 0;
    switch (op) {
      case DW_ExprOp_Lit0:  case DW_ExprOp_Lit1:  case DW_ExprOp_Lit2:
      case DW_ExprOp_Lit3:  case DW_ExprOp_Lit4:  case DW_ExprOp_Lit5:
      case DW_ExprOp_Lit6:  case DW_ExprOp_Lit7:  case DW_ExprOp_Lit8:
      case DW_ExprOp_Lit9:  case DW_ExprOp_Lit10: case DW_ExprOp_Lit11:
      case DW_ExprOp_Lit12: case DW_ExprOp_Lit13: case DW_ExprOp_Lit14:
      case DW_ExprOp_Lit15: case DW_ExprOp_Lit16: case DW_ExprOp_Lit17:
      case DW_ExprOp_Lit18: case DW_ExprOp_Lit19: case DW_ExprOp_Lit20:
      case DW_ExprOp_Lit21: case DW_ExprOp_Lit22: case DW_ExprOp_Lit23:
      case DW_ExprOp_Lit24: case DW_ExprOp_Lit25: case DW_ExprOp_Lit26:
      case DW_ExprOp_Lit27: case DW_ExprOp_Lit28: case DW_ExprOp_Lit29:
      case DW_ExprOp_Lit30: case DW_ExprOp_Lit31: {
        U64 x = op - DW_ExprOp_Lit0;
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      
      case DW_ExprOp_Const1U:size_param = 1;                goto const_n;
      case DW_ExprOp_Const2U:size_param = 2;                goto const_n;
      case DW_ExprOp_Const4U:size_param = 4;                goto const_n;
      case DW_ExprOp_Const8U:size_param = 8;                goto const_n;
      case DW_ExprOp_Const1S:size_param = 1; is_signed = 1; goto const_n;
      case DW_ExprOp_Const2S:size_param = 2; is_signed = 1; goto const_n;
      case DW_ExprOp_Const4S:size_param = 4; is_signed = 1; goto const_n;
      case DW_ExprOp_Const8S:size_param = 8; is_signed = 1; goto const_n;
      const_n:
      {
        if (is_signed) {
          S64 x = 0;
          cursor += str8_deserial_read(raw_data, cursor, &x, size_param, 1);
          x = extend_sign64(x, size_param);
          op_value = push_str8f(scratch.arena, "%lld", x);
        } else {
          U64 x = 0;
          cursor += str8_deserial_read(raw_data, cursor, &x, size_param, 1);
          op_value = push_str8f(scratch.arena, "%llu", x);
        }
      } break;
      
      case DW_ExprOp_Addr: {
        U64 addr = 0;
        cursor += str8_deserial_read(raw_data, cursor, &addr, addr_size, 1);
        op_value = push_str8f(scratch.arena, "%#llx", addr);
      } break;
      
      case DW_ExprOp_ConstU: {
        U64 x = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      
      case DW_ExprOp_ConstS: {
        S64 x = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%lld", x);
      } break;
      
      case DW_ExprOp_Reg0:  case DW_ExprOp_Reg1:  case DW_ExprOp_Reg2:
      case DW_ExprOp_Reg3:  case DW_ExprOp_Reg4:  case DW_ExprOp_Reg5:
      case DW_ExprOp_Reg6:  case DW_ExprOp_Reg7:  case DW_ExprOp_Reg8:
      case DW_ExprOp_Reg9:  case DW_ExprOp_Reg10: case DW_ExprOp_Reg11:
      case DW_ExprOp_Reg12: case DW_ExprOp_Reg13: case DW_ExprOp_Reg14:
      case DW_ExprOp_Reg15: case DW_ExprOp_Reg16: case DW_ExprOp_Reg17:
      case DW_ExprOp_Reg18: case DW_ExprOp_Reg19: case DW_ExprOp_Reg20:
      case DW_ExprOp_Reg21: case DW_ExprOp_Reg22: case DW_ExprOp_Reg23:
      case DW_ExprOp_Reg24: case DW_ExprOp_Reg25: case DW_ExprOp_Reg26:
      case DW_ExprOp_Reg27: case DW_ExprOp_Reg28: case DW_ExprOp_Reg29:
      case DW_ExprOp_Reg30: case DW_ExprOp_Reg31: {
        U64 reg_idx = op - DW_ExprOp_Reg0;
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, 0);
      } break;
      
      case DW_ExprOp_RegX: {
        U64 reg_idx = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, 0);
      } break;
      
      case DW_ExprOp_ImplicitValue: {
        U64 value_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &value_size);
        Rng1U64 value_range = rng_1u64(cursor, cursor + value_size);
        String8 value_data  = str8_substr(raw_data, value_range);
        cursor += value_size;
        String8List value_strings = numeric_str8_list_from_data(scratch.arena, 16, value_data, 1);
        op_value = str8_list_join(scratch.arena, &value_strings, &(StringJoin){.pre = str8_lit("{ "), .sep = str8_lit(", "), .post = str8_lit(" }")});
      } break;
      
      case DW_ExprOp_Piece: {
        U64 size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &size);
        op_value = push_str8f(scratch.arena, "%u", size);
      } break;
      
      case DW_ExprOp_BitPiece: {
        U64 bit_size = 0, bit_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &bit_size);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &bit_off);
        op_value = push_str8f(scratch.arena, "bit size %llu, bit offset %llu", bit_size, bit_off);
      } break;
      
      case DW_ExprOp_Pick: {
        U8 stack_idx = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &stack_idx);
        op_value = push_str8f(scratch.arena, "stack index %u", stack_idx);
      } break;
      
      case DW_ExprOp_PlusUConst: {
        U64 addend = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &addend);
        op_value = push_str8f(arena, "addend %llu", addend);
      } break;
      
      case DW_ExprOp_Skip: {
        S16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%+d bytes", x);
      } break;
      
      case DW_ExprOp_Bra: {
        S16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%+d", x);
      } break;
      
      case DW_ExprOp_BReg0:  case DW_ExprOp_BReg1:  case DW_ExprOp_BReg2: 
      case DW_ExprOp_BReg3:  case DW_ExprOp_BReg4:  case DW_ExprOp_BReg5: 
      case DW_ExprOp_BReg6:  case DW_ExprOp_BReg7:  case DW_ExprOp_BReg8:  
      case DW_ExprOp_BReg9:  case DW_ExprOp_BReg10: case DW_ExprOp_BReg11: 
      case DW_ExprOp_BReg12: case DW_ExprOp_BReg13: case DW_ExprOp_BReg14: 
      case DW_ExprOp_BReg15: case DW_ExprOp_BReg16: case DW_ExprOp_BReg17: 
      case DW_ExprOp_BReg18: case DW_ExprOp_BReg19: case DW_ExprOp_BReg20: 
      case DW_ExprOp_BReg21: case DW_ExprOp_BReg22: case DW_ExprOp_BReg23: 
      case DW_ExprOp_BReg24: case DW_ExprOp_BReg25: case DW_ExprOp_BReg26: 
      case DW_ExprOp_BReg27: case DW_ExprOp_BReg28: case DW_ExprOp_BReg29: 
      case DW_ExprOp_BReg30: case DW_ExprOp_BReg31: {
        U64 reg_idx = op - DW_ExprOp_BReg0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off);
      } break;
      
      case DW_ExprOp_FBReg: {
        S64 reg_off = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = push_str8f(scratch.arena, "offset %lld", reg_off);
      } break;
      
      case DW_ExprOp_BRegX: {
        U64 reg_idx = 0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off);
      } break;
      
      case DW_ExprOp_XDerefSize:
      case DW_ExprOp_DerefSize: {
        U8 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%u", x);
      } break;
      
      case DW_ExprOp_Call2: {
        U16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%u", x);
      } break;
      case DW_ExprOp_Call4: {
        U32 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(arena, "%u", x);
      } break;
      case DW_ExprOp_CallRef: {
        U64 x = 0;
        cursor += str8_deserial_read_dwarf_uint(raw_data, cursor, format, &x);
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      case DW_ExprOp_ImplicitPointer:
      case DW_ExprOp_GNU_ImplicitPointer: {
        U64 info_off = 0;
        cursor += str8_deserial_read_dwarf_uint(raw_data, cursor, format, &info_off);
        S64 ptr = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &ptr);
        
        op_value = push_str8f(scratch.arena, ".debug_info+%#llx, ptr %llx", info_off, ptr);
      } break;
      case DW_ExprOp_Convert:
      case DW_ExprOp_GNU_Convert: {
        U64 type_cu_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "TypeCuOff %#llx", cu_base + type_cu_off);
      } break;
      case DW_ExprOp_GNU_ParameterRef: {
        // TODO: always 4 bytes?
        U32 cu_off = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &cu_off);
        op_value = push_str8f(scratch.arena, "CuOff %#x", cu_base + cu_off);
      } break;
      case DW_ExprOp_DerefType:
      case DW_ExprOp_GNU_DerefType: {
        U8  deref_size  = 0;
        U64 type_cu_off = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &deref_size);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "%#x, TypeCuOff %#llx", deref_size, cu_base + type_cu_off);
      } break;
      case DW_ExprOp_ConstType:
      case DW_ExprOp_GNU_ConstType: {
        U64 type_cu_off      = 0;
        U8  const_value_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        cursor += str8_deserial_read_struct(raw_data, cursor, &const_value_size);
        Rng1U64 const_value_range = rng_1u64(cursor, cursor + const_value_size);
        String8 const_value_data  = str8_substr(raw_data, const_value_range);
        String8List const_value_strings = numeric_str8_list_from_data(scratch.arena, 16, const_value_data, 1);
        String8 const_value_str = str8_list_join(scratch.arena, &const_value_strings, &(StringJoin){.sep = str8_lit(", ")});
        op_value = push_str8f(scratch.arena, "TypeCuOff %#llx, Const Value { %S }", cu_base + type_cu_off, const_value_str);
        cursor += const_value_size;
      } break;
      case DW_ExprOp_RegvalType:
      case DW_ExprOp_GNU_RegvalType: {
        U64 reg_idx = 0, type_cu_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "%S, TypeCuOff %#llx", dw_string_from_register(scratch.arena, arch, reg_idx), cu_base + type_cu_off);
      } break;
      case DW_ExprOp_EntryValue:
      case DW_ExprOp_GNU_EntryValue: {
        U64 block_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &block_size);
        Rng1U64     block_range = rng_1u64(cursor, cursor + block_size);
        String8     block_data  = str8_substr(raw_data, block_range);
        String8List block_expr  = dw_string_list_from_expression(scratch.arena, block_data, cu_base, addr_size, arch, ver, ext, format);
        op_value = str8_list_join(scratch.arena, &block_expr, &(StringJoin){.pre = str8_lit("{ "), .sep = str8_lit(","), .post = str8_lit(" }")});
        cursor += block_size;
      } break;
      case DW_ExprOp_Addrx: {
        U64 addr = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &addr);
        op_value = push_str8f(scratch.arena, "%#llx", addr);
      } break;
      
      case DW_ExprOp_CallFrameCfa:
      case DW_ExprOp_FormTlsAddress:
      case DW_ExprOp_PushObjectAddress:
      case DW_ExprOp_Nop:
      case DW_ExprOp_Eq:
      case DW_ExprOp_Ge:
      case DW_ExprOp_Gt:
      case DW_ExprOp_Le:
      case DW_ExprOp_Lt:
      case DW_ExprOp_Ne:
      case DW_ExprOp_Shl:
      case DW_ExprOp_Shr:
      case DW_ExprOp_Shra:
      case DW_ExprOp_Xor:
      case DW_ExprOp_XDeref:
      case DW_ExprOp_Abs:
      case DW_ExprOp_And:
      case DW_ExprOp_Div:
      case DW_ExprOp_Minus:
      case DW_ExprOp_Mod:
      case DW_ExprOp_Mul:
      case DW_ExprOp_Neg:
      case DW_ExprOp_Not:
      case DW_ExprOp_Or:
      case DW_ExprOp_Plus:
      case DW_ExprOp_Rot:
      case DW_ExprOp_Swap:
      case DW_ExprOp_Deref:
      case DW_ExprOp_Dup:
      case DW_ExprOp_Drop:
      case DW_ExprOp_Over:
      case DW_ExprOp_StackValue: {
        // no operands
      } break;
    }
    
    String8 opcode_str = dw_string_from_expr_op(scratch.arena, ver, ext, op);
    if (op_value.size == 0) {
      str8_list_pushf(arena, &result, "DW_OP_%S", opcode_str);
    } else {
      str8_list_pushf(arena, &result, "DW_OP_%S = %S", opcode_str, op_value);
    }
  }
  scratch_end(scratch);
  return result;
}

internal String8
dw_string_from_expression(Arena *arena, String8 expr, U64 cu_base, U64 addr_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = dw_string_list_from_expression(scratch.arena, expr, cu_base, addr_size, arch, ver, ext, format);
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  return result;
}

internal String8List
dw_string_list_from_cfi_program(Arena              *arena,
                                U64                 cu_base,
                                Arch                arch,
                                DW_Version          ver,
                                DW_Ext              ext,
                                DW_Format           format,
                                U64                 pc_begin,
                                DW_UnpackedCIE     *cie,
                                DW_ReadCfiPtrFunc  *read_cfi_ptr_func,
                                void               *read_cfi_ptr_ud,
                                String8             program)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};

  U64 pc = pc_begin;
  for (U64 cursor = 0; cursor < program.size; ) {
    DW_CFA opcode = 0;
    cursor += str8_deserial_read_struct(program, cursor, &opcode);

    U64 operand = 0;
    if ((opcode & DW_CFAMask_OpcodeHi) != 0) {
      operand = opcode & DW_CFAMask_Operand;
      opcode  = opcode & DW_CFAMask_OpcodeHi;
    }

    String8 operand_str = str8_lit("???");
    switch (opcode) {
    case DW_CFA_Nop: { operand_str = str8_zero(); } break;
    case DW_CFA_SetLoc: {
      U64 address = 0;
      cursor += read_cfi_ptr_func(program, cursor, read_cfi_ptr_ud, &address);
      operand_str = str8f(arena, "0x%X", address);
    } break;
    case DW_CFA_AdvanceLoc1: {
      U8 delta = 0;
      cursor += str8_deserial_read_struct(program, cursor, &delta);
      delta *= cie->code_align_factor;
      pc += delta;
      operand_str = str8f(arena, "%+u; PC 0x%I64x", delta, pc);
    } break;
    case DW_CFA_AdvanceLoc2: {
      U16 delta = 0;
      cursor += str8_deserial_read_struct(program, cursor, &delta);
      delta *= cie->code_align_factor;
      pc += delta;
      operand_str = str8f(arena, "%+u; PC 0x%I64x", delta, pc);
    } break;
    case DW_CFA_AdvanceLoc4: {
      U32 delta = 0;
      cursor += str8_deserial_read_struct(program, cursor, &delta);
      delta *= cie->code_align_factor;
      pc += delta;
      operand_str = str8f(arena, "%+u; PC 0x%I64x", delta, pc);
    } break;
    case DW_CFA_OffsetExt: {
      U64 reg = 0, offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      operand_str = str8f(arena, "%S", dw_string_from_reg_off(scratch.arena, arch, reg, (S64)offset * cie->data_align_factor));
    } break;
    case DW_CFA_RestoreExt: { operand_str = str8_zero(); } break;
    case DW_CFA_Undefined: {
      U64 reg = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      operand_str = str8f(arena, "%I64u", reg);
    } break;
    case DW_CFA_SameValue: {
      U64 reg = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      operand_str = str8f(arena, "%S", dw_string_from_reg_off(scratch.arena, arch, reg, 0));
    } break;
    case DW_CFA_Register: {
      U64 reg = 0, offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      operand_str = str8f(arena, "%S", dw_string_from_reg_off(scratch.arena, arch, reg, (S64)offset));
    } break;
    case DW_CFA_RememberState: { operand_str = str8_zero(); } break;
    case DW_CFA_RestoreState:  { operand_str = str8_zero(); } break;
    case DW_CFA_DefCfa: {
      U64 reg = 0, offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      operand_str = str8f(arena, "%S", dw_string_from_reg_off(scratch.arena, arch, reg, offset));
    } break;
    case DW_CFA_DefCfaRegister: {
      U64 reg = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      operand_str = str8f(arena, "%S", dw_string_from_reg_off(scratch.arena, arch, reg, 0));
    } break;
    case DW_CFA_DefCfaOffset: {
      U64 offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      operand_str = str8f(arena, "+%I64u", offset);
    } break;
    case DW_CFA_DefCfaExpr: {
      U64 expr_size = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &expr_size);
      String8 expr = str8_substr(program, rng_1u64(cursor, cursor + expr_size));
      String8 expr_str = dw_string_from_expression(scratch.arena, expr, cu_base, cie->address_size, arch, ver, ext, format);
      operand_str = str8f(arena, "%S", expr_str);
    } break;
    case DW_CFA_Expr: {
      U64 reg = 0, expr_size = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_uleb128(program, cursor, &expr_size);
      String8 expr     = str8_substr(program, rng_1u64(cursor, cursor + expr_size));
      String8 reg_str  = dw_string_from_reg_off(scratch.arena, arch, reg, 0);
      String8 expr_str = dw_string_from_expression(scratch.arena, expr, cu_base, cie->address_size, arch, ver, ext, format);
      operand_str = str8f(arena, "%S expression %S", reg_str, expr_str);
    } break;
    case DW_CFA_OffsetExtSf: {
      U64 reg = 0; S64 offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_sleb128(program, cursor, &offset);
      String8 reg_str = dw_string_from_reg_off(scratch.arena, arch, reg, offset * cie->data_align_factor);
      operand_str = str8f(arena, "%S", reg_str);
    } break;
    case DW_CFA_DefCfaSf: {
      U64 reg = 0; S64 offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &reg);
      cursor += str8_deserial_read_sleb128(program, cursor, &offset);
      String8 reg_str = dw_string_from_reg_off(scratch.arena, arch, reg, offset * cie->data_align_factor);
      operand_str = str8f(arena, "%S", reg_str);
    } break;
    case DW_CFA_ValOffset: {
      U64 val = 0, offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &val);
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      operand_str = str8f(arena, "value 0x%llx, offset %+I64d", val, offset);
    } break;
    case DW_CFA_ValOffsetSf: {
      U64 val = 0; S64 offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &val);
      cursor += str8_deserial_read_sleb128(program, cursor, &offset);
      operand_str = str8f(arena, "value %llu, offset %+I64d", val, offset);
    } break;
    case DW_CFA_ValExpr: {
      U64 val = 0; U64 expr_size = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &val);
      cursor += str8_deserial_read_uleb128(program, cursor, &expr_size);
      String8 expr = str8_substr(program, rng_1u64(cursor, cursor + expr_size));
      String8 expr_str = dw_string_from_expression(scratch.arena, expr, cu_base, cie->address_size, arch, ver, ext, format);
      operand_str = str8f(arena, "value +%I64u, expression %S", val, expr_str);
    } break;
    case DW_CFA_AdvanceLoc: {
      U64 delta = operand * cie->code_align_factor;
      pc += delta;
      operand_str = str8f(arena, "+%I64u; PC 0x%I64x", delta, pc);
    } break;
    case DW_CFA_Offset: {
      U64 offset = 0;
      cursor += str8_deserial_read_uleb128(program, cursor, &offset);
      S64 v = (S64)offset * cie->data_align_factor;
      operand_str = dw_string_from_reg_off(scratch.arena, arch, operand, v);
    } break;
    case DW_CFA_Restore: {
      String8 reg_str = dw_string_from_reg_off(scratch.arena, arch, operand, 0);
      operand_str = str8f(arena, "%S", reg_str);
    } break;
    default: { NotImplemented; } break;
    }

    str8_list_pushf(arena, &list, "DW_CFA_%S: %S", dw_string_from_cfa_opcode(opcode), operand_str);
  }

  scratch_end(scratch);
  return list;
}

internal String8
dw_single_line_string_from_expression(Arena *arena, String8 raw_data, U64 cu_base, U64 address_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp        scratch    = scratch_begin(&arena, 1);
  String8List list       = dw_string_list_from_expression(scratch.arena, raw_data, cu_base, address_size, arch, ver, ext, format);
  String8     expression = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return expression;
}

#if 0

internal String8
dw_string_from_eh_ptr_enc(Arena *arena, EH_PtrEnc enc)
{
  U8 type = enc & EH_PtrEnc_TypeMask;
  String8 type_str = str8_lit("NULL");
  switch (type) {
    case EH_PtrEnc_Ptr:     type_str = str8_lit("PTR");      break;
    case EH_PtrEnc_ULEB128: type_str = str8_lit("ULEB128");  break;
    case EH_PtrEnc_UData2:  type_str = str8_lit("UDATA2");   break;
    case EH_PtrEnc_UData4:  type_str = str8_lit("UDATA4");   break;
    case EH_PtrEnc_UData8:  type_str = str8_lit("UDATA8");   break;
    case EH_PtrEnc_Signed:  type_str = str8_lit("SIGNED");   break;
    case EH_PtrEnc_SLEB128: type_str = str8_lit("SLEB128");  break;
    case EH_PtrEnc_SData2:  type_str = str8_lit("SDATA2");   break;
    case EH_PtrEnc_SData4:  type_str = str8_lit("SDATA4");   break;
    case EH_PtrEnc_SData8:  type_str = str8_lit("SDATA8");   break;
  }
  U8 modifier = enc & EH_PtrEnc_ModifyMask;
  String8 modifier_str = str8_lit("NULL");
  switch (modifier) {
    case EH_PtrEnc_PcRel:   modifier_str = str8_lit("PCREL");   break;
    case EH_PtrEnc_TextRel: modifier_str = str8_lit("TEXTREL"); break;
    case EH_PtrEnc_DataRel: modifier_str = str8_lit("DATAREL"); break;
    case EH_PtrEnc_FuncRel: modifier_str = str8_lit("FUNCREL"); break;
  }
  String8 indir_str = str8_lit("");
  if (enc & EH_PtrEnc_Indirect) {
    indir_str = str8_lit("(INDIRECT)");
  }
  return push_str8f(arena, "Type: %S, Modifier: %S %S", type_str, modifier_str, indir_str);
}

internal void
dw_print_eh_frame(Arena *arena, String8List *out, String8 indent, String8 raw_eh_frame, Arch arch, DW_Version ver, DW_Ext ext, EH_PtrCtx *ptr_ctx)
{
  Temp scratch = scratch_begin(&arena, 1);
  DW_UnpackedCIE cie = {0};
  
  for (U64 cursor = 0; cursor < raw_eh_frame.size; ) {
    U64 header_offset = cursor;
    
    U64 length = 0; // doesn't include bytes for size
    cursor += dw_based_range_read_length(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), cursor, &length);
    
    if (length == 0) {
      break; // encountered exit marker
    }
    
    U64 entry_start = cursor;
    U64 entry_end   = cursor + length;
    
    U32 entry_id = 0; // always 4-bytes, even when length is encoded as 64-bit integer
    cursor += str8_deserial_read_struct(raw_eh_frame, cursor, &entry_id);
    
    // TODO: fix the freaking EH_PtrEnc_PCREL encoding.
    // it assumes "frame_base" points to the first byte of .eh_frame
    // but here base is start of ELF and we use range to select .eh_frame
    // bytes to read, which breaks parsing.
    String8 raw_frame = str8_substr(raw_eh_frame, rng_1u64(cursor, cursor + length - sizeof(entry_id)));
    Rng1U64 cfi_range = rng_1u64(0,0);
    
    // CIE
    if (entry_id == 0) {
      dw_unwind_parse_cie_x64(raw_frame.str, rng_1u64(0,raw_frame.size), ptr_ctx, 0, &cie);
      cfi_range = cie.cfi_range;
      
      rd_printf("CIE @ 0x%X, Length %u", header_offset, length);
      rd_indent();
      rd_printf("LSDA Encoding:           %S",    dw_string_from_eh_ptr_enc(scratch.arena, cie.lsda_encoding));
      rd_printf("Address Encoding:        %S",    dw_string_from_eh_ptr_enc(scratch.arena, cie.addr_encoding));
      rd_printf("Augmentation:            %S",    cie.augmentation);
      rd_printf("Code Align Factor:       %llu",  cie.code_align_factor);
      rd_printf("Data Align Factor:       %lld",  cie.data_align_factor);
      rd_printf("Return Address Register: %u",    cie.ret_addr_reg);
      rd_printf("Handler IP:              %#llx", cie.handler_ip);
      rd_unindent();
    }
    // FDE
    else {
      DW_UnpackedFDE fde = {0};
      dw_unwind_parse_fde_x64(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), ptr_ctx, &cie, 0, &fde);
      cfi_range = fde.cfi_range;
      
      // calc parent CIE offset
      AssertAlways(entry_start >= entry_id);
      U64 cie_offset = entry_start - entry_id; NotImplemented; // TODO: syms_safe_sub_u64(range.min + entry_start, entry_id);
      
      rd_printf("FDE @ %#llx, Length %u, Parent CIE @ %#llx", header_offset, length, cie_offset);
      rd_indent();
      rd_printf("IP Range: %#llx-%#llx", fde.ip_voff_range.min, fde.ip_voff_range.max);
      rd_printf("LSDA IP:  %#llx",       fde.lsda_ip);
      rd_unindent();
    }
    
    // print CFI program
    rd_printf("CFI Program:");
    rd_indent();
    
    DW_Format format  = DW_FormatFromSize(length);
    String8   raw_cfi = str8_substr(raw_eh_frame, cfi_range);
    dw_string_from_cfi_program(scratch.arena, out, indent, raw_cfi, &cie, ptr_ctx, arch, ver, ext, format);
    
    rd_unindent();
    rd_newline();
    
    // advance to next entry
    cursor = entry_end;
  }
  
  scratch_end(scratch);
}

internal void
dw_print_debug_loc(Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ExecutableImageKind image_type, B32 relaxed)
{
#if 0
  DW_Section info = input->sec[DW_Section_Info];
  DW_Section loc  = input->sec[DW_Section_Loc];
  
  if (loc.data.size == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", input->sec[DW_Section_Loc].name);
  rd_indent();
  
  // TODO: warn about overlaps in ranges
  
  Rng1U64List cu_range_list = dw_comp_unit_ranges_from_info(scratch.arena, info);
  
  // parse debug_info for attributes with LOCLIST and store .debug_loc offsets 
  U64List    *loc_lists     = push_array(scratch.arena, U64List,    cu_range_list.count);
  U64        *address_sizes = push_array(scratch.arena, U64,        cu_range_list.count);
  U64        *address_bases = push_array(scratch.arena, U64,        cu_range_list.count);
  U64        *cu_bases      = push_array(scratch.arena, U64,        cu_range_list.count);
  DW_Version *ver_arr       = push_array(scratch.arena, DW_Version, cu_range_list.count);
  DW_Ext     *ext_arr       = push_array(scratch.arena, DW_Ext,     cu_range_list.count);
  
  U64 comp_idx = 0;
  for (Rng1U64Node *cu_range_n = cu_range_list.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++comp_idx) {
    Temp comp_temp = temp_begin(arena);
    
    Rng1U64     cu_range = cu_range_n->v;
    DW_CompUnit cu       = dw_comp_unit_from_info_off(comp_temp.arena, input, cu_range.min, relaxed);
    
    // store info about comp unit
    address_sizes[comp_idx] = cu.address_size;
    address_bases[comp_idx] = cu.base_addr;
    ver_arr[comp_idx]       = cu.version;
    cu_bases[comp_idx]      = cu_range_n->v.min;
    
    // parse tags
    for (U64 info_off = cu.tags_range.min; info_off < cu.tags_range.max; /* empty */) {
      Temp tag_temp = temp_begin(scratch.arena);
      
      DW_Tag tag = dw_tag_from_info_offset_cu(tag_temp.arena, input, &cu, ext_arr[comp_idx], info_off);
      
      // parse attribs
      for (DW_AttribNode *attrib_node = tag.attribs.first; attrib_node != 0; attrib_node = attrib_node->next) {
        DW_Attrib *attrib         = &attrib_node->v;
        B32        is_sect_offset = attrib->value_class == DW_AttribClass_LocListPtr || (attrib->value_class == DW_AttribClass_LocList && attrib->form_kind == DW_Form_SecOffset);
        B32        is_sect_index  = attrib->value_class == DW_AttribClass_LocList && attrib->form_kind == DW_Form_LocListx;
        if (is_sect_offset) {
          u64_list_push(scratch.arena, &loc_lists[comp_idx], attrib->value.v[0]);
        } else if (is_sect_index) {
          // TODO: support for section indexing
        }
      }
      
      // advance to next tag
      info_off = tag.next_info_off;
      
      temp_end(tag_temp);
    }
    
    temp_end(comp_temp);
  }
  
  void    *base  = dw_base_from_sec(input, DW_Section_Loc);
  Rng1U64  range = dw_range_from_sec(input, DW_Section_Loc);
  
  rd_printf(".debug_loc");
  rd_indent();
  rd_printf("%-8s %-8s %-8s %s", "Offset", "Min", "Max", "Expression");
  for (U32 comp_idx = 0; comp_idx < cu_range_list.count; ++comp_idx) {
    Temp locs_temp = temp_begin(scratch.arena);
    
    DW_Version ver = ver_arr[comp_idx];
    DW_Ext     ext = ext_arr[comp_idx];
    
    U64Array locs = u64_array_from_list(locs_temp.arena, &loc_lists[comp_idx]);
    u64_array_sort(locs.count, locs.v);
    
    U64Array locs_set      = remove_duplicates_u64_array(locs_temp.arena, locs);
    U64      address_size  = address_sizes[comp_idx];
    U64      base_selector = (address_size == 8) ? max_U64 : max_U32;
    
    for (U64 loc_idx = 0; loc_idx < locs_set.count; ++loc_idx) {
      U64 base_address = address_bases[comp_idx];
      for (U64 cursor = locs_set.v[loc_idx]; cursor < dim_1u64(range); /* empty */) {
        Temp range_temp = temp_begin(arena);
        
        String8List list = {0};
        
        // offset
        str8_list_pushf(range_temp.arena, &list, "%08llx", cursor);
        
        // parse entry
        U64 v0 = 0, v1 = 0;
        cursor += dw_based_range_read(base, range, cursor, address_size, &v0);
        cursor += dw_based_range_read(base, range, cursor, address_size, &v1);
        
        B32 is_list_end = v0 == 0 && v1 == 0;
        if (is_list_end) {
          str8_list_pushf(range_temp.arena, &list, "<LIST END>");
        } else if (v0 == base_selector) {
          base_address = v1;
        } else {
          U16 expr_size = 0;
          cursor += dw_based_range_read_struct(base, range, cursor, &expr_size);
          Rng1U64 expr_range = rng_1u64(range.min+cursor, range.min+cursor+expr_size);
          cursor += expr_size;
          
          // format dwarf expression
          B32     is_dwarf64 = (address_size == 8);
          String8 raw_expr   = str8((U8*)base+expr_range.min, dim_1u64(expr_range));
          String8 expression = dw_single_line_string_from_expression(range_temp.arena, raw_expr, cu_bases[comp_idx], address_size, arch, ver, ext, input->sec[DW_Section_Loc].mode);
          
          // push entry
          U64 min = base_address + v0;
          U64 max = base_address + v1;
          str8_list_pushf(range_temp.arena, &list, "%08llx %08llx %S", min, max, expression);
        }
        
        // print entry
        String8 print = str8_list_join(range_temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", print);
        
        // cleanup temp
        temp_end(range_temp);
        
        // exit check
        if (is_list_end) {
          break;
        }
      }
    }
    
    temp_end(locs_temp);
  }
  rd_unindent();
  
  rd_unindent();
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_ranges(Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ExecutableImageKind image_type, B32 relaxed)
{
  NotImplemented;
#if 0
  DW_Section  ranges = input->sec[DW_Section_Ranges];
  void       *base   = dw_base_from_sec(input, DW_Section_Ranges);
  Rng1U64     range  = dw_range_from_sec(input, DW_Section_Ranges);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  Rng1U64List  cu_range_list = dw_comp_unit_ranges_from_info(scratch.arena, sections->v[DW_Section_Info]);
  
  // parse debug_info for attributes with LOCLIST and store .debug_loc offsets 
  U64List *loc_lists     = push_array(scratch.arena, U64List, cu_range_list.count);
  U64     *address_sizes = push_array(scratch.arena, U64,     cu_range_list.count);
  U64     *address_bases = push_array(scratch.arena, U64,     cu_range_list.count);
  
  {
    U64 comp_idx = 0;
    for (Rng1U64Node *cu_range_n = cu_range_list.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++comp_idx) {
      Rng1U64     cu_range = cu_range_n->v;
      DW_CompUnit cu       = dw_comp_unit_from_info_offset(scratch.arena, sections, cu_range.min, relaxed);
      
      // store info about comp unit
      address_sizes[comp_idx] = cu.address_size;
      address_bases[comp_idx] = cu.base_addr;
      
      // parse tags
      for (U64 info_off = cu.tags_range.min; info_off < cu.tags_range.max; /* empty */) {
        DW_Tag tag = dw_tag_from_info_offset_cu(scratch.arena, sections, &cu, info_off);
        
        // parse attribs
        for (DW_AttribNode *attrib_node = tag.attribs.first; attrib_node != 0; attrib_node = attrib_node->next) {
          DW_Attrib *attrib         = &attrib_node->v;
          B32        is_sect_offset = attrib->value_class == DW_AttribClass_RngListPtr || (attrib->value_class == DW_AttribClass_RngList && attrib->form_kind == DW_Form_SecOffset);
          B32        is_sect_index  = attrib->value_class == DW_AttribClass_RngList && attrib->form_kind == DW_Form_RngListx;
          if (is_sect_offset) {
            u64_list_push(scratch.arena, &loc_lists[comp_idx], attrib->value.v[0]);
          } else if (is_sect_index) {
            // TODO: support for section indexing
          }
        }
        
        info_off = tag.next_info_off;
      }
    }
  }
  
  rd_printf("# %S", sections->v[DW_Section_Ranges].name);
  rd_indent();
  rd_printf("%-8s %-8s %-8s", "Offset", "Min", "Max");
  for (U32 comp_idx = 0; comp_idx < cu_range_list.count; ++comp_idx) {
    U64Array locs = u64_array_from_list(scratch.arena, &loc_lists[comp_idx]);
    u64_array_sort(locs.count, locs.v);
    U64Array locs_set      = remove_duplicates_u64_array(scratch.arena, locs);
    U64      address_size  = address_sizes[comp_idx];
    U64      base_selector = (address_size == 8) ? max_U64 : max_U32;
    
    for (U64 loc_idx = 0; loc_idx < locs_set.count; ++loc_idx) {
      U64 base_address = address_bases[comp_idx];
      for (U64 cursor = locs_set.v[loc_idx]; cursor < dim_1u64(range); /* empty */) {
        Temp range_temp = temp_begin(scratch.arena);
        
        String8List list = {0};
        
        // offset
        str8_list_pushf(range_temp.arena, &list, "%08llx", cursor);
        
        // parse entry
        U64 v0 = 0, v1 = 0;
        cursor += dw_based_range_read(base, range, cursor, address_size, &v0);
        cursor += dw_based_range_read(base, range, cursor, address_size, &v1);
        
        B32 is_list_end = v0 == 0 && v1 == 0;
        if (is_list_end) {
          str8_list_pushf(range_temp.arena, &list, "<LIST END>");
        } else if (v0 == base_selector) {
          base_address = v1;
        } else {
          // push entry
          U64 min = base_address + v0;
          U64 max = base_address + v1;
          str8_list_pushf(range_temp.arena, &list, "%08llx %08llx", min, max);
        }
        
        // print entry
        String8 print = str8_list_join(range_temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", print);
        
        temp_end(range_temp);
        
        // exit check
        if (is_list_end) {
          break;
        }
      }
    }
  }
#endif
}

internal void
dw_print_debug_aranges(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_ARanges);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_ARanges);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_ARanges].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64        unit_length           = 0;
    DW_Version version               = 0;
    U64        debug_info_offset     = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    
    B32 is_dwarf64 = unit_length >= max_U32;
    U64 int_size   = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    cursor += dw_based_range_read(base, range, cursor, int_size, &debug_info_offset);
    
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    rd_printf("Unit length:           %llu",  unit_length);
    rd_printf("Version:               %u",    version);
    rd_printf("Debug info offset:     %#llx", debug_info_offset);
    rd_printf("Address size:          %u",    address_size);
    rd_printf("Segment selector size: %u",    segment_selector_size);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 2 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Range");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      U64 segment_selector = 0;
      U64 address          = 0;
      U64 length           = 0;
      cursor += dw_based_range_read(base, range, cursor, segment_selector_size, &segment_selector);
      cursor += dw_based_range_read(base, range, cursor, address_size, &address);
      cursor += dw_based_range_read(base, range, cursor, address_size, &length);
      
      if (segment_selector == 0 && address == 0 && length == 0) {
        str8_list_pushf(temp.arena, &list, "<LIST END>");
      } else {
        if (segment_selector != 0) {
          str8_list_pushf(temp.arena, &list, "%02llu:", segment_selector);
        }
        str8_list_pushf(temp.arena, &list, "%llx-%llx", address, address+length);
      }
      
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ") });
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_addr(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_Addr);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_Addr);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_Addr].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64        unit_length           = 0;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    
    U64 unit_offset = cursor;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    
    U64 unit_opl = cursor + unit_length;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Address");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08X", cursor);
      
      U64 segment_selector = 0;
      U64 address          = 0;
      cursor += dw_based_range_read(base, range, cursor, segment_selector_size, &segment_selector);
      cursor += dw_based_range_read(base, range, cursor, address_size, &address);
      
      if (segment_selector == 0 && address == 0) {
        str8_list_pushf(temp.arena, &list, "<LIST END>");
      } else {
        if (segment_selector != 0) {
          str8_list_pushf(temp.arena, &list, "%02u:", segment_selector);
        }
        str8_list_pushf(temp.arena, &list, "%llx", address);
      }
      
      String8 print = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal U64
dw_based_range_read_address(void *base, Rng1U64 range, U64 offset, Rng1U64Array segment_ranges, U8 segment_selector_size, U8 address_size, U64 *address_out)
{
  U64 read_offset = offset;
  
  // read segment
  U64 segment_selector = 0;
  read_offset += dw_based_range_read(base, range, read_offset, segment_selector_size, &segment_selector);
  
  // read address
  U64 address = 0;
  read_offset += dw_based_range_read(base, range, read_offset, address_size, &address);
  
  // apply segment offset
  B32 is_address_segment_relative = segment_selector_size > 0;
  if (is_address_segment_relative) {
    if (segment_selector < segment_ranges.count) {
      address += segment_ranges.v[segment_selector].min;
    } else {
      Assert(!"invalid segment selector");
    }
  }
  
  U64 read_size = (read_offset - offset);
  return read_size;
}

internal void
dw_print_debug_loclists(Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_virtual_ranges, Arch arch)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_LocLists);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_LocLists);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_LocLists].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    
    U64        unit_opl              = cursor + unit_length;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    U32        offset_entry_count    = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &offset_entry_count);
    
    U64 past_header_offset = cursor;
    B32 is_dwarf64         = unit_length > max_U32;
    U64 offset_size        = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    rd_printf("Offset entry count:    %u", offset_entry_count);
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    if (offset_entry_count > 0) {
      rd_printf("Offsets:");
      rd_indent();
      rd_printf("%-8s %-8s", "Index", "Offset");
      for (U64 offset_idx = 0; offset_idx < offset_entry_count; ++offset_idx) {
        U64 offset = 0;
        cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
        rd_printf("%-8llu %llx", offset_idx, offset+past_header_offset);
      }
      rd_unindent();
    }
    
    rd_printf("Locations:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Location");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      U8 kind = 0;
      cursor += dw_based_range_read_struct(base, range, cursor, &kind);
      str8_list_pushf(temp.arena, &list, "DW_LLE_%S", dw_string_from_loc_list_entry_kind(temp.arena, kind));
      
      B32 has_loc_desc = 0;
      switch (kind) {
        case DW_LocListEntryKind_EndOfList:
        break;
        case DW_LocListEntryKind_DefaultLocation: {
          has_loc_desc = 1;
        } break;
        case DW_LocListEntryKind_BaseAddress: {
          U64 base_address = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &base_address);
          str8_list_pushf(temp.arena, &list, "%llx", base_address);
        } break;
        case DW_LocListEntryKind_StartLength: {
          U64 start  = 0;
          U64 length = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, length);
        } break;
        case DW_LocListEntryKind_StartEnd: {
          U64 start = 0;
          U64 end   = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &end);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, end);
        } break;
        case DW_LocListEntryKind_BaseAddressX: {
          U64 base_addressx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &base_addressx);
          str8_list_pushf(temp.arena, &list, "%llx", base_addressx);
        } break;
        case DW_LocListEntryKind_StartXEndX: {
          U64 startx = 0;
          U64 endx   = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &endx);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, endx);
        } break;
        case DW_LocListEntryKind_OffsetPair: {
          U64 a = 0;
          U64 b = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &a);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &b);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", a, b);
          
          U8 expr_length = 0;
          cursor += dw_based_range_read_struct(base, range, cursor, &expr_length); 
          
          String8 raw_expr = str8((U8*)base+cursor, expr_length);
          cursor += expr_length;
          
          // TODO: we need actual cu base to format expression correctly
          NotImplemented;
          String8 expression = dw_single_line_string_from_expression(temp.arena, raw_expr, 0, address_size, arch, version, DW_Ext_Null, is_dwarf64);
          str8_list_pushf(temp.arena, &list, "(%S)", expression);
        } break;
        case DW_LocListEntryKind_StartXLength: {
          U64 startx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          U64 length = 0;
          if (version < DW_Version_5) {
            // pre-standard length
            cursor += dw_based_range_read(base, range, cursor, sizeof(U32), &length);
          } else {
            cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          }
        } break;
      }
      
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_rnglists(Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_ranges)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_RngLists);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_RngLists);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_RngLists].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64        unit_opl              = cursor + unit_length;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    U32        offset_entry_count    = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &offset_entry_count);
    
    U64 past_header_offset = cursor;
    B32 is_dwarf64         = unit_length > max_U32;
    U64 offset_size        = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    rd_printf("Offset entry count:    %u", offset_entry_count);
    
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    if (offset_entry_count > 0) {
      rd_printf("Offsets:");
      rd_indent();
      rd_printf("%-8s %-8s", "Index", "Offset");
      for (U64 offset_idx = 0; offset_idx < offset_entry_count; ++offset_idx) {
        U64 offset = 0;
        cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
        rd_printf("%-8llu %llx", offset_idx, offset+past_header_offset);
      }
      rd_unindent();
    }
    
    rd_printf("Ranges:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Range");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(scratch.arena);
      
      String8List list = {0};
      
      // offset
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      // opcode mnemonic
      U8 kind = 0;
      cursor += dw_based_range_read_struct(base, range, cursor, &kind);
      str8_list_pushf(temp.arena, &list, "DW_RLE_%S", dw_string_from_rng_list_entry_kind(temp.arena, kind));
      
      // operand
      switch (kind) {
        case DW_RngListEntryKind_EndOfList: {
          // empty
        } break;
        case DW_RngListEntryKind_BaseAddressX:  {
          U64 base_addressx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &base_addressx);
          str8_list_pushf(temp.arena, &list, "%llx", base_addressx);
        } break;
        case DW_RngListEntryKind_BaseAddress: {
          U64 base_address = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &base_address);
          str8_list_pushf(temp.arena, &list, "%llx", base_address);
        } break;
        case DW_RngListEntryKind_OffsetPair: {
          U64 min = 0;
          U64 max = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &min);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &max);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", min, max);
        } break;
        case DW_RngListEntryKind_StartxLength: {
          U64 startx = 0;
          U64 length = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, length);
        } break;
        case DW_RngListEntryKind_StartxEndx: {
          U64 startx = 0;
          U64 endx   = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &endx);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, endx);
        } break;
        case DW_RngListEntryKind_StartEnd: {
          U64 start = 0;
          U64 end = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &end);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, end);
        } break;
        case DW_RngListEntryKind_StartLength: {
          U64 start = 0;
          U64 length = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, length);
        } break;
      }
      
      // output row
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_format_string_table(Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_SectionKind sec)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, sec);
  Rng1U64  range = dw_range_from_sec(sections, sec);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[sec].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    
    DW_Version version = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 2");
    }
    
    B32 is_dwarf64      = unit_length > max_U32;
    U32 sec_offset_size = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    U64 debug_info_offset = 0, debug_info_length = 0;
    cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &debug_info_offset);
    cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &debug_info_length);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:           %u",            version);
    rd_printf("Debug info offset: %#llx",         debug_info_offset);
    rd_printf("Debug info length: %#llx",         debug_info_length);
    
    rd_printf("Entries:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "String");
    for (; cursor < unit_opl; ) {
      U64 info_offset = 0;
      cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &info_offset);
      String8 string = dw_based_range_read_string(base, range, cursor);
      cursor += (string.size + 1);
      
      rd_printf("%08llx %S", info_offset, string);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_pubnames(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  dw_format_string_table(arena, out, indent, input, DW_Section_PubNames);
}

internal void
dw_print_debug_pubtypes(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  dw_format_string_table(arena, out, indent, input, DW_Section_PubTypes);
}

internal void
dw_print_debug_line_str(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_LineStr);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_LineStr);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_LineStr].name);
  rd_indent();
  
  rd_printf("%-8s %-8s", "Offset", "String");
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 offset = cursor;
    String8 string = dw_based_range_read_string(base, range, cursor);
    cursor += (string.size + 1);
    rd_printf("%08llX %S", offset, string);
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_str_offsets(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_StrOffsets);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_StrOffsets);
  
  void    *debug_str_base  = dw_base_from_sec(sections, DW_Section_Str);
  Rng1U64  debug_str_range = dw_range_from_sec(sections, DW_Section_Str);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_StrOffsets].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    
    DW_Version version = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    U16 padding = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &padding);
    if (padding != 0) {
      rd_warningf("unexpected padding byte");
    }
    
    B32 is_dwarf64  = unit_length > max_U32;
    U32 offset_size = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llX, length %lld", unit_offset, unit_length);
    rd_printf("Version: %d", version);
    rd_printf("Padding: %d", padding);
    rd_indent();
    rd_printf("%-8s %-8s", "@", "Offset");
    for (; cursor < unit_opl; ) {
      U64 read_at = cursor;
      U64 offset  = 0;
      cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
      rd_printf("%08llx %08llx", read_at, offset);
      if (dim_1u64(debug_str_range) > 0) {
        String8 string = dw_based_range_read_string(debug_str_base, debug_str_range, offset);
        rd_printf(" %S", string);
      }
      rd_newline();
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

#endif

////////////////////////////////
//~ rjf: Dump Entry Point

internal String8
dw_string_from_attrib_value(Arena *arena, DW_Input *input, Arch arch, DW_CompUnit *unit, DW_LineVMHeader *line_vm, DW_Attrib *attrib)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List attrib_fmt = {0};
  
  // rjf: log attrib's value based on vlass
  DW_AttribClass value_class = dw_value_class_from_attrib(unit, attrib);
  switch(value_class)
  {
    default:                       {str8_list_pushf(scratch.arena, &attrib_fmt, "`unknown value class`");}break;
    case DW_AttribClass_Undefined: {str8_list_pushf(scratch.arena, &attrib_fmt, "`undefined value class`");}break;
    case DW_AttribClass_Address:   {str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x", dw_address_from_attrib(input, unit, attrib));}break;
    case DW_AttribClass_Const:     {str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x", dw_const_u64_from_attrib(input, unit, attrib));}break;
    case DW_AttribClass_Block:
    {
      String8 block = dw_block_from_attrib(input, unit, attrib);
      String8List block_strs = numeric_str8_list_from_data(scratch.arena, 16, block, 1);
      String8 block_str = str8_list_join(scratch.arena, &block_strs, &(StringJoin){.sep = str8_lit(", ")});
      str8_list_pushf(scratch.arena, &attrib_fmt, "{ %S }", block_str);
    }break;
    case DW_AttribClass_ExprLoc:
    {
      String8 exprloc = dw_exprloc_from_attrib(input, unit, attrib);
      String8 exprloc_str = dw_single_line_string_from_expression(scratch.arena, exprloc, unit->info_range.min, unit->address_size, arch, unit->version, unit->ext, unit->format);
      str8_list_pushf(scratch.arena, &attrib_fmt, "{ %S }", exprloc_str);
    }break;
    case DW_AttribClass_Flag:
    {
      B32 flag = dw_flag_from_attrib(input, unit, attrib);
      str8_list_pushf(scratch.arena, &attrib_fmt, "%llu (%s)", flag, flag == 0 ? "false" : "true");
    }break;
    case DW_AttribClass_LinePtr:
    case DW_AttribClass_LocListPtr:
    case DW_AttribClass_MacPtr:
    case DW_AttribClass_RngListPtr:
    case DW_AttribClass_RngList:
    case DW_AttribClass_StrOffsetsPtr:
    case DW_AttribClass_AddrPtr:
    {
      if(attrib->form_kind == DW_Form_SecOffset)
      {
        str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x", attrib->form.sec_offset);
      }
      else
      {
        str8_list_pushf(scratch.arena, &attrib_fmt, "`unexpected form kind %S`", dw_string_from_form_kind(scratch.arena, unit->version, attrib->form_kind));
      }
    }break;
    case DW_AttribClass_Reference:
    {
      if(attrib->form_kind == DW_Form_Ref1 ||
         attrib->form_kind == DW_Form_Ref2 ||
         attrib->form_kind == DW_Form_Ref4 ||
         attrib->form_kind == DW_Form_Ref8 ||
         attrib->form_kind == DW_Form_RefUData)
      {
        U64 info_off = unit->info_range.min + attrib->form.ref;
        str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x", info_off);
        if(!contains_1u64(unit->info_range, info_off))
        {
          str8_list_pushf(scratch.arena, &attrib_fmt, ": `(out of this unit's bounds)`");
        }
      }
      else
      {
        str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x", attrib->form.ref);
      }
    }break;
    case DW_AttribClass_String:
    {
      if(attrib->form_kind == DW_Form_Strp || attrib->form_kind == DW_Form_LineStrp || attrib->form_kind == DW_Form_StrpSup)
      {
        str8_list_pushf(scratch.arena, &attrib_fmt, "0x%I64x ", attrib->form.sec_offset);
      }
      String8 string = dw_string_from_attrib(input, unit, attrib);
      str8_list_pushf(scratch.arena, &attrib_fmt, "\"%S\"", string);
    }break;
  }
  
  // rjf: extend attrib's value with enum info
  {
    String8 enum_info = {0};
    switch(attrib->attrib_kind)
    {
      case DW_AttribKind_Language:
      {
        DW_Language lang = dw_const_u64_from_attrib(input, unit, attrib);
        enum_info = dw_string_from_language(scratch.arena, lang);
      }break;
      case DW_AttribKind_DeclFile:
      {
        U64          file_idx = dw_const_u64_from_attrib(input, unit, attrib);
        DW_LineFile *file     = dw_file_from_attrib(unit, line_vm, attrib);
        if(file != 0)
        {
          enum_info = dw_path_from_file(scratch.arena, line_vm, file);
        }
      }break;
      case DW_AttribKind_DeclLine:
      {
        enum_info = str8f(scratch.arena, "%I64u", dw_const_u64_from_attrib(input, unit, attrib));
      }break;
      case DW_AttribKind_Inline:
      {
        DW_InlKind inl = dw_const_u64_from_attrib(input, unit, attrib);
        enum_info = dw_string_from_inl(scratch.arena, inl);
      }break;
      case DW_AttribKind_Accessibility:
      {
        DW_AccessKind access = dw_const_u64_from_attrib(input, unit, attrib);
        enum_info = dw_string_from_access_kind(scratch.arena, access);
      }break;
      case DW_AttribKind_CallingConvention:
      {
        DW_CallingConventionKind calling_convetion = dw_const_u64_from_attrib(input, unit, attrib);
        enum_info = dw_string_from_calling_convetion(scratch.arena, calling_convetion);
      }break;
      case DW_AttribKind_Encoding:
      {
        DW_ATE encoding = dw_const_u64_from_attrib(input, unit, attrib);
        enum_info = dw_string_from_attrib_type_encoding(scratch.arena, encoding);
      }break;
    }

    if(enum_info.size)
    {
      str8_list_pushf(scratch.arena, &attrib_fmt, " `%S`", enum_info);
    }
  }
  
  String8 result = str8_list_join(arena, &attrib_fmt, 0);

  scratch_end(scratch);
  return result;
}

internal
DW_CIE_FROM_OFFSET_FUNC(dwarf_dump_cie_from_offset)
{
  return hash_table_search_u64_raw(ud, offset);
}

internal String8List
dw_dump_list_from_sections(Arena              *arena,
                           DW_Input           *input,
                           Arch                arch,
                           DW_DumpSubsetFlags  subset_flags)
{
  String8List strings = {0};
  String8 indent = str8_lit("                                                                                                                                ");
#define dump(str)  str8_list_push(arena, &strings, (str))
#define dumpf(...) str8_list_pushf(arena, &strings, __VA_ARGS__)
#define DumpSubset(name) if(subset_flags & DW_DumpSubsetFlag_##name) DeferLoop(dumpf("// %S\n\n", dw_name_title_from_dump_subset_table[DW_DumpSubset_##name]), dump(str8_lit("\n")))
  Temp scratch = scratch_begin(&arena, 1);
  Rng1U64Array segment_vranges = {0};
  DW_ListUnitInput lu_input = dw_list_unit_input_from_input(scratch.arena, input);
  B32 relaxed  = 1;

  DW_CompUnit *cu_arr;
  {
    DW_ListUnitInput  lu_input      = dw_list_unit_input_from_input(scratch.arena, input);
    Rng1U64List       cu_range_list = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_Info].data);
    Rng1U64Array      cu_ranges     = rng1u64_array_from_list(scratch.arena, &cu_range_list);
    cu_arr = push_array(scratch.arena, DW_CompUnit, cu_ranges.count);
    for EachIndex(cu_idx, cu_ranges.count)
    {
      cu_arr[cu_idx] = dw_cu_from_info_off(scratch.arena, input, lu_input, cu_ranges.v[cu_idx].min, relaxed);
    }
  }
  
  //////////////////////////////
  //- rjf: dump .debug_info
  //
  DumpSubset(DebugInfo)
  { Rng1U64List  unit_ranges_list = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_Info].data);
    Rng1U64Array unit_ranges = rng1u64_array_from_list(scratch.arena, &unit_ranges_list);
    for EachIndex(unit_idx, unit_ranges.count)
    {
      Temp unit_temp = temp_begin(scratch.arena);
      
      //- rjf: unpack unit
      Rng1U64 unit_range = unit_ranges.v[unit_idx];
      DW_CompUnit *unit  = &cu_arr[unit_idx];
      String8 unit_dir  = dw_string_from_tag_attrib_kind(input, unit, unit->tag, DW_AttribKind_CompDir );
      String8 unit_name = dw_string_from_tag_attrib_kind(input, unit, unit->tag, DW_AttribKind_Name    );
      String8 stmt_list = dw_line_ptr_from_tag_attrib_kind(input, unit, unit->tag, DW_AttribKind_StmtList);
      DW_LineVMHeader line_vm   = {0};
      dw_read_line_vm_header(unit_temp.arena, stmt_list, 0, input, unit_dir, unit_name, unit->address_size, unit->str_offsets_lu, &line_vm);
      
      //- rjf: log top-level unit info
      dumpf("unit: // compile_unit[%I64u]\n{\n", unit_idx);
      dumpf("  version:         %u\n",        unit->version);
      dumpf("  address_size:    %I64u\n",     unit->address_size);
      dumpf("  abbrev_off:      0x%I64x\n",   unit->abbrev_off);
      dumpf("  info_range:      [0x%I64x, 0x%I64x) // (%M)\n", unit->info_range.min, unit->info_range.max, dim_1u64(unit->info_range));
      
      //- rjf: log tags
      S64 tag_depth = 0;
      U64 tag_idx = 0;
      for(U64 info_off = unit->first_tag_info_off; info_off < unit->info_range.max; tag_idx += 1)
      {
        Temp tag_temp = temp_begin(scratch.arena);
        
        // rjf: unpack tag
        String8 tag_indent = str8_prefix(indent, (tag_depth+1)*2);
        U64 tag_info_off = info_off;
        DW_Tag tag = {0};
        info_off += dw_read_tag_cu(tag_temp.arena, input, unit, tag_info_off, &tag);
        
        // rjf: log top-level tag info
        dumpf("%Stag: // info_off: 0x%I64x, abbrev_id: %I64u, compile_unit[%I64u].tag[%I64u]\n%S{\n", tag_indent, tag_info_off, tag.abbrev_id, unit_idx, tag_idx, tag_indent);
        dumpf("%S  kind: %S\n", tag_indent, dw_string_from_tag_kind(tag_temp.arena, tag.kind));
        
        // log attribs
        {
          // compute columns' max widths
          U64 attrib_name_max_size = 0, form_kind_max_size = 0, value_max_size = 0;
          for EachNode(attrib_n, DW_AttribNode, tag.attribs.first)
          {
            Temp attrib_temp = temp_begin(tag_temp.arena);
            attrib_name_max_size = Max(attrib_name_max_size, dw_string_from_attrib_kind(attrib_temp.arena, unit->version, unit->ext, attrib_n->v.attrib_kind).size);
            form_kind_max_size   = Max(form_kind_max_size, dw_string_from_form_kind(attrib_temp.arena, unit->version, attrib_n->v.form_kind).size);
            value_max_size       = Max(value_max_size, dw_string_from_attrib_value(attrib_temp.arena, input, arch, unit, &line_vm, &attrib_n->v).size);
            temp_end(attrib_temp);
          }
          value_max_size = Min(120, value_max_size);

          // log
          for EachNode(attrib_n, DW_AttribNode, tag.attribs.first)
          {
            DW_Attrib *attrib = &attrib_n->v;

            Temp attrib_temp = temp_begin(tag_temp.arena);

            String8 attrib_kind_str = dw_string_from_attrib_kind(attrib_temp.arena, unit->version, unit->ext, attrib->attrib_kind);
            String8 form_kind_str   = dw_string_from_form_kind(attrib_temp.arena, unit->version, attrib->form_kind);
            String8 value_str       = dw_string_from_attrib_value(attrib_temp.arena, input, arch, unit, &line_vm, attrib);

            dumpf("%S  attrib: { kind: %S, %.*sform_kind: %S, %.*svalue: %S, %.*s} // info_off: 0x%I64x\n",
                tag_indent,
                attrib_kind_str, attrib_name_max_size - attrib_kind_str.size, indent.str,
                form_kind_str,   form_kind_max_size - form_kind_str.size, indent.str,
                value_str,       value_str.size < value_max_size ? value_max_size - value_str.size: 0, indent.str,
                attrib->info_off, unit_idx, tag_idx);

            temp_end(attrib_temp);
          }
        }
        
        // rjf: log tag closes
        if(!tag.has_children || tag.abbrev_id == 0)
        {
          dumpf("%S}\n", tag_indent);
        }
        
        // rjf: indent/unindent
        if(tag.has_children)
        {
          tag_depth += 1;
        }
        if(!tag.abbrev_id)
        {
          tag_depth -= 1;
        }
        
        temp_end(tag_temp);
      }
      temp_end(unit_temp);
      dumpf("} // compile_unit[/%u]\n\n", unit_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: dump .debug_abbrev
  //
  DumpSubset(DebugAbbrev)
  {
    Temp scratch = scratch_begin(&arena, 1);
    DW_Section abbrev = input->sec[DW_Section_Abbrev];
    S64 depth = 0;
    U64 idx = 0;
    for(U64 cursor = 0; cursor < abbrev.data.size; idx += 1)
    {
      // rjf: read id & advance
      U64 id_off = cursor;
      U64 id = 0;
      cursor += str8_deserial_read_uleb128(abbrev.data, cursor, &id);
      if(id == 0) { continue; }
      
      // rjf: unpack abbrev
      U64 tag = 0;
      U8 has_children = 0;
      cursor += str8_deserial_read_uleb128(abbrev.data, cursor, &tag);
      cursor += str8_deserial_read_struct(abbrev.data, cursor, &has_children);
      
      // rjf: log abbrev
      Temp temp = temp_begin(scratch.arena);
      dumpf("abbrev: // abbrev[%I64u]\n{\n", idx);
      dumpf("  offset:       0x%I64x\n", id_off);
      dumpf("  id:           %I64u\n", id);
      dumpf("  tag_kind:     %S\n", dw_string_from_tag_kind(temp.arena, tag));
      dumpf("  has_children: %s\n", has_children ? "true" : "false");
      for(;;)
      {
        U64 attrib_off = cursor;
        U64 attrib_id = 0, form_id = 0;
        cursor += str8_deserial_read_uleb128(abbrev.data, cursor, &attrib_id);
        cursor += str8_deserial_read_uleb128(abbrev.data, cursor, &form_id);
        if(attrib_id == 0) { break; }
        String8 attrib_str = dw_string_from_attrib_kind(temp.arena, DW_Version_Last, DW_Ext_All, attrib_id);
        String8 form_str   = dw_string_from_form_kind(temp.arena, DW_Version_Last, form_id);
        dumpf("  attrib: { offset: 0x%I64x, kind: %S, form_kind: %S }\n", attrib_off, attrib_str, form_str);
      }
      dumpf("}\n");
      temp_end(temp);
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_line
  //
  DumpSubset(DebugLine)
  {
    Rng1U64List  unit_ranges_list = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_Line].data);
    Rng1U64Array unit_ranges = rng1u64_array_from_list(scratch.arena, &unit_ranges_list);
    for EachIndex(unit_idx, unit_ranges.count)
    {
      Temp unit_temp = temp_begin(scratch.arena);
      
      // rjf: unpack unit
      String8      raw_lines    = str8_substr(input->sec[DW_Section_Line].data, unit_ranges.v[unit_idx]);
      DW_CompUnit *cu           = &cu_arr[unit_idx];
      String8      cu_stmt_list = dw_line_ptr_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_StmtList);
      String8      cu_dir       = dw_string_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_CompDir);
      String8      cu_name      = dw_string_from_tag_attrib_kind(input, cu, cu->tag, DW_AttribKind_Name);
      DW_LineVMHeader line_vm        = {0};
      U64             line_vm_size   = dw_read_line_vm_header(unit_temp.arena, raw_lines, 0, input, cu_dir, cu_name, cu->address_size, cu->str_offsets_lu, &line_vm);
      if(line_vm_size == 0)
      {
        continue;
      }
      
      // rjf: begin logging line table
      dumpf("line_table: // line_table[%I64u]\n{\n", unit_idx);
      
      // rjf: log line table header
      DeferLoop(dumpf("  header:\n  {\n"), dumpf("  }\n\n"))
      {
        String8List opcode_length_strings = numeric_str8_list_from_data(unit_temp.arena, 16, str8(line_vm.opcode_lens, line_vm.num_opcode_lens), 1);
        String8 opcode_lengths_string = str8_list_join(arena, &opcode_length_strings, &(StringJoin){.sep = str8_lit(", ")});
        dumpf("    version:                 %u\n",        line_vm.version              );
        dumpf("    line_table_off:          0x%I64x\n",   line_vm.unit_range.min       );
        dumpf("    line_table_size:         %I64u\n",     dim_1u64(line_vm.unit_range) );
        dumpf("    address_size:            %u\n",        line_vm.address_size         );
        dumpf("    segment_selector_size:   %u\n",        line_vm.segment_selector_size);
        dumpf("    header_length:           %I64u\n",     line_vm.header_length        );
        dumpf("    min_instruction_length:  %u\n",        line_vm.min_inst_len         );
        dumpf("    max_ops_for_instruction: %u\n",        line_vm.max_ops_for_inst     );
        dumpf("    default_is_stmt:         %u\n",        line_vm.default_is_stmt      );
        dumpf("    line_base:               %d\n",        line_vm.line_base            );
        dumpf("    line_range:              %u\n",        line_vm.line_range           );
        dumpf("    opcode_base:             %u\n",        line_vm.opcode_base          );
        dumpf("    opcode_lengths:          %S\n",        opcode_lengths_string        );
      }
      
      // rjf: log directory table
      DeferLoop(dumpf("  directory_table:\n  {\n"), dumpf("  }\n\n"))
      {
        dumpf("    // %-4s %-8s\n", "no.", "name");
        for EachIndex(dir_idx, line_vm.dir_table.count)
        {
          dumpf("    {  %-4llu %S  }\n", dir_idx, line_vm.dir_table.v[dir_idx]);
        }
      }
      
      // rjf: log file table
      DeferLoop(dumpf("  file_table:\n  {\n"), dumpf("  }\n\n"))
      {
        dumpf("  // %-4s %-8s %-8s %-33s %-8s %-8s\n", "no.", "dir_idx", "time", "md5", "size", "name");
        for EachIndex(file_idx, line_vm.file_table.count)
        {
          DW_LineFile *file = &line_vm.file_table.v[file_idx];
          dumpf("  {  %-4llu %-8llu %-8llu %016llx-%016llx %-8llu %S  }\n",
                file_idx,
                file->dir_idx,
                file->modify_time,
                file->md5_digest.u64[1],
                file->md5_digest.u64[0],
                file->file_size,
                file->file_name);
        }
      }
      
      // rjf: log opcodes
      DeferLoop(dumpf("  opcodes:\n  {\n"), dumpf("  }\n\n"))
      {
        String8        opcodes    = str8_skip(raw_lines, line_vm_size);
        B32            end_of_seq = 0;
        DW_LineVMState vm_state   = {0};
        dw_line_vm_reset(&vm_state, line_vm.default_is_stmt);
        for(U64 cursor = 0; cursor < opcodes.size;)
        {
          Temp opcode_temp = temp_begin(unit_temp.arena);
          String8List opcode_fmt = {0};
          
          // opcode offset
          str8_list_pushf(opcode_temp.arena, &opcode_fmt, "[%08llx]", cursor);
          
          // parse opcode
          U8 opcode = 0;
          cursor += str8_deserial_read_struct(opcodes, cursor, &opcode);
          
          // push opcode id
          String8 opcode_str = dw_string_from_std_opcode(opcode_temp.arena, opcode);
          str8_list_push(arena, &opcode_fmt, opcode_str);
          
          // format operands
          switch(opcode)
          {
            default:
            {
              if(opcode >= line_vm.opcode_base)
              {
                U32 adjusted_opcode = 0;
                U32 op_advance      = 0;
                S32 line_advance    = 0;
                U64 addr_advance    = 0;
                if(line_vm.line_range > 0 && line_vm.max_ops_for_inst > 0)
                {
                  adjusted_opcode = (U32)(opcode - line_vm.opcode_base);
                  op_advance      = adjusted_opcode / line_vm.line_range;
                  line_advance    = (S32)line_vm.line_base + ((S32)adjusted_opcode) % (S32)line_vm.line_range;
                  addr_advance    = line_vm.min_inst_len * ((vm_state.op_index+op_advance) / line_vm.max_ops_for_inst);
                }
                vm_state.address        += addr_advance;
                vm_state.op_index        = (vm_state.op_index + op_advance) % line_vm.max_ops_for_inst;
                vm_state.line            = (U32)((S32)vm_state.line + line_advance);
                vm_state.basic_block     = 0;
                vm_state.prologue_end    = 0;
                vm_state.epilogue_begin  = 0;
                vm_state.discriminator   = 0;
                end_of_seq = 0;
                str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance line by %d, advance address by %lld", line_advance, addr_advance);
              }
              else
              {
                if(opcode > 0 && opcode <= line_vm.num_opcode_lens)
                {
                  str8_list_pushf(opcode_temp.arena, &opcode_fmt, "skip operands:");
                  U64 num_operands = line_vm.opcode_lens[opcode - 1];
                  for(U8 i = 0; i < num_operands; i += 1)
                  {
                    U64 operand = 0;
                    cursor += str8_deserial_read_uleb128(opcodes, cursor, &operand);
                    str8_list_pushf(opcode_temp.arena, &opcode_fmt, " %llx", operand);
                  }
                }
              }
            }break;
            case DW_StdOpcode_Copy:
            {
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "Line = %u, Column = %u, Address = %#llx", vm_state.line, vm_state.column, vm_state.address);
              end_of_seq = 0;
              vm_state.discriminator   = 0;
              vm_state.basic_block     = 0;
              vm_state.prologue_end    = 0;
              vm_state.epilogue_begin  = 0;
            }break;
            case DW_StdOpcode_AdvancePc:
            {
              U64 advance = 0;
              cursor += str8_deserial_read_uleb128(opcodes, cursor, &advance);
              dw_line_vm_advance(&vm_state, advance, line_vm.min_inst_len, line_vm.max_ops_for_inst);
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance %#llx ; current address %#llx", advance, vm_state.address);
            }break;
            case DW_StdOpcode_AdvanceLine:
            {
              S64 advance = 0;
              cursor += str8_deserial_read_sleb128(opcodes, cursor, &advance);
              vm_state.line += advance;
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance %lld ; current line %u", advance, vm_state.line);
            }break;
            case DW_StdOpcode_SetFile:
            {
              U64 file_idx = 0;
              cursor += str8_deserial_read_uleb128(opcodes, cursor, &file_idx);
              vm_state.file_index = file_idx;
              String8 path = dw_path_from_file_idx(opcode_temp.arena, &line_vm, file_idx);
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu \"%S\"", file_idx, path);
            }break;
            case DW_StdOpcode_SetColumn:
            {
              U64 column = 0;
              cursor += str8_deserial_read_uleb128(opcodes, cursor, &column);
              vm_state.column = column;
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", column);
            }break;
            case DW_StdOpcode_NegateStmt:
            {
              vm_state.is_stmt = !vm_state.is_stmt;
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "is_stmt = %u", vm_state.is_stmt);
            }break;
            case DW_StdOpcode_SetBasicBlock:
            {
              vm_state.basic_block = 1;
            }break;
            case DW_StdOpcode_ConstAddPc:
            {
              U64 advance = (0xffu - line_vm.opcode_base)/line_vm.line_range;
              dw_line_vm_advance(&vm_state, advance, line_vm.min_inst_len, line_vm.max_ops_for_inst);
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%lld ; address %#llx", advance, vm_state.address);
            }break;
            case DW_StdOpcode_FixedAdvancePc:
            {
              U64 operand = 0;
              cursor += str8_deserial_read_struct(opcodes, cursor, &operand);
              vm_state.address += operand;
              vm_state.op_index = 0;
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", operand);
            }break;
            case DW_StdOpcode_SetPrologueEnd:
            {
              vm_state.prologue_end = 1;
            }break;
            case DW_StdOpcode_SetEpilogueBegin:
            {
              vm_state.epilogue_begin = 1;
            }break;
            case DW_StdOpcode_SetIsa:
            {
              U64 v = 0;
              cursor += str8_deserial_read_uleb128(opcodes, cursor, &v);
              vm_state.isa = v;
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", v);
            }break;
            case DW_StdOpcode_ExtendedOpcode:
            {
              U64 length = 0;
              U8 ext_opcode = 0;
              cursor += str8_deserial_read_uleb128(opcodes, cursor, &length);
              U64 opcode_end = cursor + length;
              cursor += str8_deserial_read_struct(opcodes, cursor, &ext_opcode);
              String8 ext_opcode_str = dw_string_from_ext_opcode(opcode_temp.arena, ext_opcode);
              //str8_list_pushf(opcode_temp.arena, &opcode_fmt, "length: %u", length);
              str8_list_push(opcode_temp.arena, &opcode_fmt, ext_opcode_str);
              switch(ext_opcode)
              {
                case DW_ExtOpcode_EndSequence:
                {
                  vm_state.end_sequence = 1;
                  dw_line_vm_reset(&vm_state, line_vm.default_is_stmt);
                  end_of_seq = 1;
                }break;
                case DW_ExtOpcode_SetAddress:
                {
                  U64 address = 0;
                  cursor += str8_deserial_read(opcodes, cursor, &address, line_vm.address_size, line_vm.address_size);
                  vm_state.address    = address;
                  vm_state.op_index   = 0;
                  str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%#llx", address);
                }break;
                case DW_ExtOpcode_DefineFile:
                {
                  String8 file_name = {0};
                  cursor += str8_deserial_read_cstr(opcodes, cursor, &file_name);
                  U64 dir_idx = 0, modify_time = 0, file_size = 0;
                  cursor += str8_deserial_read_uleb128(opcodes, cursor, &dir_idx);
                  cursor += str8_deserial_read_uleb128(opcodes, cursor, &modify_time);
                  cursor += str8_deserial_read_uleb128(opcodes, cursor, &file_size);
                  str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%S Dir: %llu, Time: %llu, Size: %llu", file_name, dir_idx, modify_time, file_size);
                }break;
                case DW_ExtOpcode_SetDiscriminator:
                {
                  U64 v = 0;
                  cursor += str8_deserial_read_uleb128(opcodes, cursor, &v);
                  vm_state.discriminator = v;
                  str8_list_pushf(arena, &opcode_fmt, "%llu", v);
                }break;
              }
              cursor = opcode_end;
            }break;
          }
          String8 string = str8_list_join(opcode_temp.arena, &opcode_fmt, &(StringJoin){.sep=str8_lit(" ")});
          dumpf("%S\n", string);
          temp_end(opcode_temp);
        }
      }
      temp_end(unit_temp);
    }
  }
  
  //////////////////////////////
  //- rjf: dump .debug_str
  //
  DumpSubset(DebugStr) DeferLoop(dumpf("strings:\n{\n"), dumpf("}\n\n"))
  {
    String8 data = input->sec[DW_Section_Str].data;
    for(U64 cursor = 0, read_size = 0; cursor < data.size; cursor += read_size)
    {
      String8 string = {0};
      read_size = str8_deserial_read_cstr(data, cursor, &string);
      dumpf("  { 0x%08I64x  %llu  \"%S\" }\n", cursor, string.size, string);
    }
  }

  //////////////////////////////
  //~ dump .debug_frame
  DumpSubset(DebugFrame)
  {
    HashTable *cie_ht = hash_table_init(scratch.arena, 0x2000);
    String8   debug_frame = input->sec[DW_Section_Frame].data;
    U64       addr_size   = byte_size_from_arch(arch);

    // make offset -> CIE hash table
    for (U64 cursor = 0, desc_size; cursor < debug_frame.size; cursor += desc_size) {
      DW_DescriptorEntry desc = {0};
      desc_size = dw_parse_descriptor_entry_header(debug_frame, cursor, &desc);
      if (desc.type == DW_DescriptorEntryType_CIE) {
        String8         raw_cie     = str8_substr(debug_frame, desc.entry_range);
        U64             restore_pos = arena_pos(scratch.arena);
        DW_UnpackedCIE *cie         = push_array(scratch.arena, DW_UnpackedCIE, 1);
        if (dw_unpack_cie(raw_cie, desc.format, arch, cie)) {
          hash_table_push_u64_raw(scratch.arena, cie_ht, cursor, cie);
        } else {
          arena_pop_to(arena, restore_pos);
        }
      }
    }
    
    for (U64 cursor = 0, desc_size; cursor < debug_frame.size; cursor += desc_size) {
      DW_DescriptorEntry desc = {0};
      desc_size = dw_parse_descriptor_entry_header(debug_frame, cursor, &desc);
      String8 raw_desc  = str8_substr(debug_frame, desc.entry_range);
      switch (desc.type) {
      case DW_DescriptorEntryType_Null: {} break;
      case DW_DescriptorEntryType_CIE: {
        DW_UnpackedCIE cie = {0};
        if (dw_unpack_cie(raw_desc, desc.format, arch, &cie)) {
          String8List init_insts_str_list = dw_string_list_from_cfi_program(scratch.arena, 0, arch, DW_Version_5, DW_Ext_All, cie.format, 0, &cie, dw_read_cfi_ptr, &cie, cie.init_insts);

          dumpf("CIE: // entry range: %r\n", desc.entry_range);
          dumpf("{\n");
          dumpf("  Format:          %S\n",    dw_string_from_format(desc.format));
          dumpf("  Version:         %u\n",    cie.version);
          dumpf("  Aug string:      %S\n",    cie.aug_string.size ? cie.aug_string : str8_lit("None"));
          dumpf("  Code align:      %I64u\n", cie.code_align_factor);
          dumpf("  Data align:      %I64d\n", cie.data_align_factor);
          dumpf("  Return addr reg: %S\n",    dw_string_from_register(scratch.arena, arch, cie.ret_addr_reg));
          if (cie.version > DW_Version_3) {
            dumpf("  Address size:          %u\n", cie.address_size);
            dumpf("  Segment selector size: %u\n", cie.segment_selector_size);
          }
          dumpf("  Initial Insturction:\n");
          dumpf("  {\n");
          for EachNode(n, String8Node, init_insts_str_list.first) { dumpf("    %S\n", n->string); }
          dumpf("  }\n");
          dumpf("}\n");
        } else {
          dumpf("ERROR: unable to parse CIE @ %I64x\n", desc.entry_range.min);
        }
      } break;
      case DW_DescriptorEntryType_FDE: {
        DW_UnpackedFDE fde = {0};
        if (dw_unpack_fde(raw_desc, desc.format, dwarf_dump_cie_from_offset, cie_ht, &fde)) {
          DW_UnpackedCIE *cie = hash_table_search_u64_raw(cie_ht, fde.cie_pointer);
          String8List insts_str_list = dw_string_list_from_cfi_program(scratch.arena, 0, arch, DW_Version_5, DW_Ext_All, fde.format, fde.pc_range.min, cie, dw_read_cfi_ptr, cie, fde.insts);

          dumpf("FDE: // entry range: %r\n", desc.entry_range, dw_string_from_format(fde.format), fde.cie_pointer, fde.pc_range);
          dumpf("{\n");
          {
            dumpf("  Format:      %S\n",      dw_string_from_format(fde.format));
            dumpf("  CIE pointer: 0x%I64x\n", fde.cie_pointer);
            dumpf("  PC range:    %r\n",      fde.pc_range);
            dumpf("  Instructions:\n");
            dumpf("  {\n");
            for EachNode(n, String8Node, insts_str_list.first) { dumpf("    %S\n", n->string); }
            dumpf("  }\n");
          }
          dumpf("}\n");
        } else {
          dumpf("ERROR: unable to parse FDE @ %I64x\n", desc.entry_range.min);
        }
      } break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump .debug_loc
  //
  DumpSubset(DebugLoc)
  {
    // dw_print_debug_loc(arena, out, indent, input, arch, image_type, relaxed);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_ranges
  //
  DumpSubset(DebugRanges)
  {
    // dw_print_debug_ranges(arena, out, indent, input, arch, image_type, relaxed);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_aranges
  //
  DumpSubset(DebugARanges)
  {
    // dw_print_debug_aranges(arena, out, indent, input);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_addr
  //
  DumpSubset(DebugAddr)
  {
    // dw_print_debug_addr(arena, out, indent, input);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_loclists
  //
  DumpSubset(DebugLocLists)
  {
    // dw_print_debug_loclists(arena, out, indent, input, segment_vranges, arch);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_rnglists
  //
  DumpSubset(DebugRngLists)
  {
    // dw_print_debug_rnglists(arena, out, indent, input, segment_vranges);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_pubnames
  //
  DumpSubset(DebugPubNames)
  {
    // dw_print_debug_pubnames(arena, out, indent, input);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_pubtypes
  //
  DumpSubset(DebugPubTypes)
  {
    // dw_print_debug_pubtypes(arena, out, indent, input);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_linestr
  //
  DumpSubset(DebugLineStr)
  {
    // dw_print_debug_line_str(arena, out, indent, input);
  }
  
  //////////////////////////////
  //- rjf: dump .debug_stroffs
  //
  DumpSubset(DebugStrOffsets)
  {
    // dw_print_debug_str_offsets(arena, out, indent, input);
  }
  
  scratch_end(scratch);
#undef DumpSubset
#undef dumpf
#undef dump
  return strings;
}
