// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

static D2R_Shared g_d2r_shared;

////////////////////////////////

internal B32
rdim_is_eval_bytecode_static(RDIM_EvalBytecode bc)
{
  B32 is_static = 1;
  RDI_EvalOp dynamic_ops[] = { RDI_EvalOp_MemRead, RDI_EvalOp_RegRead, RDI_EvalOp_RegReadDyn, RDI_EvalOp_CFA };
  for EachNode(n, RDIM_EvalBytecodeOp, bc.first_op) {
    for EachIndex(i, ArrayCount(dynamic_ops)) {
      if (dynamic_ops[i] == n->op) {
        is_static = 0;
        goto exit;
      }
    }
  }
  exit:;
  return is_static;
}

internal B32
rdim_static_eval_bytecode_to_voff(RDIM_EvalBytecode bc, U64 image_base, U64 *voff_out)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = 0;
  
  typedef union { U16 u16; U32 u32; U64 u64; S64 s64; F32 f32; F64 f64; } Value;
  U64 stack_cap = 128, stack_count = 0;
  Value *stack = push_array(scratch.arena, Value, stack_cap);
  
  for EachNode(opcode_n, RDIM_EvalBytecodeOp, bc.first_op) {
    // pop values from stack
    Value *svals = 0;
    {
      U32 pop_count = RDI_POPN_FROM_CTRLBITS(rdi_eval_op_ctrlbits_table[opcode_n->op]);
      if (pop_count > stack_count) {
        goto exit;
      }
      stack_count -= pop_count;
      svals = stack + stack_count;
    }
    
    Value imm = { .u64 = opcode_n->p };
    Value nval = {0};
    switch (opcode_n->op) {
      case RDI_EvalOp_Stop: { opcode_n = bc.last_op; } break;
      case RDI_EvalOp_Noop: {} break;
      case RDI_EvalOp_Cond:       { NotImplemented; } break;
      case RDI_EvalOp_Skip: {
        NotImplemented;
      } break;
      case RDI_EvalOp_MemRead:    { InvalidPath;    } break;
      case RDI_EvalOp_RegRead:    { NotImplemented; } break;
      case RDI_EvalOp_RegReadDyn: { NotImplemented; } break;
      case RDI_EvalOp_FrameOff:   { NotImplemented; } break;
      case RDI_EvalOp_ModuleOff: {
        nval.u64 = image_base + imm.u64;
      } break;
      case RDI_EvalOp_TLSOff: {
        nval.u64 = image_base;
      } break;
      case RDI_EvalOp_ConstU8:
      case RDI_EvalOp_ConstU16:
      case RDI_EvalOp_ConstU32:
      case RDI_EvalOp_ConstU64:
      case RDI_EvalOp_ConstU128: {
        nval = imm;
      } break;
      case RDI_EvalOp_ConstString: { NotImplemented; } break;
      case RDI_EvalOp_Abs: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:
          case RDI_EvalTypeGroup_S:   { nval.s64 = abs_s64(svals[0].s64); } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = abs_f32(svals[0].f32); } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = abs_f64(svals[0].f64); } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Neg: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:
          case RDI_EvalTypeGroup_S:   { nval.u64 = ~svals[0].u64 + 1; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = -svals[0].f32;     } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = -svals[0].f64;     } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Add: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 + svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 + svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 + svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 + svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Sub: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 - svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[1].s64 - svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 - svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 - svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Mul: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 * svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 * svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 * svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 * svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Div: {
        B32 is_div_by_zero = 0;
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { is_div_by_zero = svals[1].u64 == 0;    } break;
          case RDI_EvalTypeGroup_S:   { is_div_by_zero = svals[1].s64 == 0;    } break;
          case RDI_EvalTypeGroup_F32: { is_div_by_zero = svals[1].f32 == 0.0f; } break;
          case RDI_EvalTypeGroup_F64: { is_div_by_zero = svals[1].f64 == 0.0;  } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
        
        if (is_div_by_zero) {
          Assert(!"trying to div by zero");
          goto exit;
        }
        
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 / svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 / svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 / svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 / svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Mod: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 % svals[1].u64;    } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 % svals[1].s64;    } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 MOD is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 MOD is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_LShift: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 << svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 << svals[1].u64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 LShift is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 LShift is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_RShift: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0 ] .u64 >> svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[1 ] .s64 >> svals[1].u64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 RShift is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 RShift is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_BitAnd: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 | svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 | svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 bitwise AND is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 bitwise AND is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_BitXor: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 ^ svals[1].u64;    } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 ^ svals[1].s64;    } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 XOR is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 XOR is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_BitNot: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = ~svals[0].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = ~svals[0].u64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 bitwise NOT is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 bitwise NOT is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_LogAnd: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 && svals[1].u64;   } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 && svals[1].s64;   } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 AND is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 AND is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_LogOr: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 || svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 || svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 OR is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 OR is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_LogNot: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = !svals[0].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = !svals[0].u64; } break;
          case RDI_EvalTypeGroup_F32: { Assert(!"F32 NOT is not supported"); } goto exit;
          case RDI_EvalTypeGroup_F64: { Assert(!"F64 NOT is not supported"); } goto exit;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_EqEq: {
        nval.u64 = !!MemoryMatch(&svals[0], &svals[1], sizeof(*svals));
      } break;
      case RDI_EvalOp_NtEq: {
        nval.u64 = !MemoryMatch(&svals[0], &svals[1], sizeof(*svals));
      } break;
      case RDI_EvalOp_LsEq: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 <= svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 <= svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 <= svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 <= svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_GrEq: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 >= svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 >= svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 >= svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 >= svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Less: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 < svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 < svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 < svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 < svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Grtr: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 > svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 > svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 > svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 > svals[1].f64; } break;
          default: { Assert(!"unexpected eval type group"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Trunc: {
        if (0 < imm.u64 && imm.u64 < 64) {
          U64 mask = max_U64 >> (64 - imm.u64);
          nval.u64 = svals[0].u64 & (max_U64 >> (64 - imm.u64));
        } else if (imm.u64 > 64) {
          Assert(!"malformed bytecode");
          goto exit;
        }
      } break;
      case RDI_EvalOp_TruncSigned: {
        if (0 < imm.u64 && imm.u64 < 64) {
          U64 mask = max_U64 >> (64 - imm.u64);
          nval.u64 = svals[0].u64 & (max_U64 >> (64 - imm.u64));
          U64 high = 0;
          if (svals[0].u64 & (1 << (imm.u64 - 1))) {
            high = ~mask;
          }
          nval.u64 = high | (svals[0].u64 & mask);
        } else if (imm.u64 > 64) {
          Assert(!"malformed bytecode");
          goto exit;
        }
      } break;
      case RDI_EvalOp_Convert: {
        U32 in  = imm.u64 & 0xff;
        U32 out = (imm.u64 >> 8) & 0xff;
        if (in != out) {
          switch (in + out*RDI_EvalTypeGroup_COUNT) {
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:   { nval.u64 = (U64)svals[0].f32; } break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:   { nval.u64 = (U64)svals[0].f64; } break;
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:   { nval.s64 = (S64)svals[0].f32; } break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:   { nval.s64 = (S64)svals[0].f64; } break;
            case RDI_EvalTypeGroup_U   + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT: { nval.f32 = (F32)svals[0].u64; } break;
            case RDI_EvalTypeGroup_S   + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT: { nval.f32 = (F32)svals[0].s64; } break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT: { nval.f32 = (F32)svals[0].f64; } break;
            case RDI_EvalTypeGroup_U   + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT: { nval.f64 = (F64)svals[0].u64; } break;
            case RDI_EvalTypeGroup_S   + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT: { nval.f64 = (F64)svals[0].s64; } break;
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT: { nval.f64 = (F64)svals[0].f32; } break;
            default: { Assert(!"unexpected conversion case"); } goto exit;
          }
        }
      } break;
      case RDI_EvalOp_Pick: {
        if (stack_count > imm.u64) {
          nval = stack[stack_count - imm.u64 - 1];
        } else {
          Assert(!"malformed bytecode");
          goto exit;
        }
      } break;
      case RDI_EvalOp_Pop: {} break;
      case RDI_EvalOp_Insert: {
        if (stack_count > imm.u64) {
          Value tval = stack[stack_count-1];
          Value *dst = stack + stack_count - 1 - imm.u64;
          Value *shift = dst + 1;
          MemoryCopy(shift, dst, imm.u64 * sizeof(Value));
          *dst = tval;
        } else {
          Assert(!"malformed bytecode");
          goto exit;
        }
      } break;
      case RDI_EvalOp_ValueRead: {
        U64 bytes_to_read = imm.u64;
        U64 offset        = svals[0].u64;
        if (offset + bytes_to_read <= sizeof(Value)) {
          Value src_val = svals[1];
          MemoryCopy(&nval, (U8 *)&src_val + offset, bytes_to_read);
        }
      } break;
      case RDI_EvalOp_ByteSwap: {
        switch (imm.u64) {
          case 0: {} break;
          case 1: {} break;
          case 2: { nval.u16 = bswap_u16(svals[0].u16); } break;
          case 4: { nval.u32 = bswap_u16(svals[0].u32); } break;
          case 8: { nval.u64 = bswap_u16(svals[0].u64); } break;
          default: { Assert(!"malformed bytecode"); } goto exit;
        }
      } break;
      case RDI_EvalOp_Swap: {
        NotImplemented;
      } break;
      default: { Assert(!"unknown op type"); } break;
    }
    
    // push computed value to the stack
    {
      U64 push_count = RDI_PUSHN_FROM_CTRLBITS(rdi_eval_op_ctrlbits_table[opcode_n->op]);
      if (push_count == 1) {
        if (stack_count < stack_cap) {
          stack[stack_count] = nval;
          stack_count += 1;
        } else {
          Assert(!"stack overflow");
          goto exit;
        }
      }
    }
  }
  
  U64 result = 0;
  if (stack_count >= 1) {
    result = stack[0].u64 - image_base;
  }
  if (voff_out) {
    *voff_out = result;
  }
  
  is_ok = 1;
exit:;
  scratch_end(scratch);
  return is_ok;
}

internal RDI_Language
d2r_rdi_language_from_dw_language(DW_Language v)
{
  RDI_Language result = RDI_Language_NULL;
  switch (v) {
    default: {} break;
    
    case DW_Language_C89:
    case DW_Language_C99:
    case DW_Language_C11:
    case DW_Language_C: {
      result = RDI_Language_C;
    } break;
    
    case DW_Language_CPlusPlus03:
    case DW_Language_CPlusPlus11:
    case DW_Language_CPlusPlus14:
    case DW_Language_CPlusPlus: {
      result = RDI_Language_CPlusPlus;
    } break;
  }
  return result;
}

internal RDI_RegCodeX64
d2r_rdi_reg_code_from_dw_reg_x64(DW_RegX64 v)
{
  RDI_RegCodeX64 result = RDI_RegCode_nil;
  switch (v) {
    default: {} break;
#define X(reg_dw, val_dw, reg_rdi, off, size) case DW_RegX64_##reg_dw: { result = RDI_RegCodeX64_##reg_rdi; } break;
    DW_Regs_X64_XList
#undef X
  }
  return result;
}

internal RDI_RegCode
d2r_rdi_reg_code_from_dw_reg(Arch arch, DW_Reg v)
{
  RDI_RegCode result = RDI_RegCode_nil;
  switch (arch) {
    default: NotImplemented; break;
    case Arch_Null: break;
    case Arch_x64:{ result = d2r_rdi_reg_code_from_dw_reg_x64(v); } break;
  }
  return result;
}

internal D2R_ValueTypeNode *
d2r_value_type_stack_push(Arena *arena, D2R_ValueTypeStack *stack, D2R_ValueType type)
{
  D2R_ValueTypeNode *n;
  if (stack->free_list) {
    n = stack->free_list;
    SLLStackPop(stack->free_list);
  } else {
    n = push_array(arena, D2R_ValueTypeNode, 1);
  }
  n->type = type;
  SLLStackPush(stack->top, n);
  stack->count += 1;
  return n;
}

internal D2R_ValueType
d2r_value_type_stack_pop(D2R_ValueTypeStack *stack)
{
  D2R_ValueType result = D2R_ValueType_Generic;
  if (stack->top) {
    D2R_ValueTypeNode *n = stack->top;
    result = n->type;
    SLLStackPop(stack->top);
    SLLStackPush(stack->free_list, n);
    stack->count -= 1;
  }
  return result;
}

internal D2R_ValueType
d2r_value_type_stack_peek(D2R_ValueTypeStack *stack)
{
  return stack->top ? stack->top->type : D2R_ValueType_Generic;
}

internal D2R_ValueType
d2r_signed_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
#define X(_N, _AT, _S) case _S: return D2R_ValueType_##_N;
    D2R_ValueType_Signed_XList
#undef X
  }
  AssertAlways(!"no suitable signed type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal D2R_ValueType
d2r_unsigned_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
#define X(_N, _AT, _S) case _S: return D2R_ValueType_##_N;
    D2R_ValueType_Unsigned_XList
#undef X
  }
  AssertAlways(!"no suitable unsigned type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal D2R_ValueType
d2r_float_type_from_bit_size(U64 bit_size)
{
#define X(_N, _AT, _S) case _S: return D2R_ValueType_##_N;
  switch (bit_size) {
    D2R_ValueType_Float_XList
  }
#undef X
  AssertAlways(!"no suitable type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal D2R_ArithmeticType
d2r_arithmetic_type_from_value_type(D2R_ValueType v)
{
#define X(_N, _AT, ...) case D2R_ValueType_##_N: return D2R_ArithmeticType_##_AT;
  switch (v) {
    D2R_ValueType_XList
  }
#undef X
  return D2R_ArithmeticType_Null;
}

internal B32
d2r_is_value_type_signed(D2R_ValueType v)
{
  return d2r_arithmetic_type_from_value_type(v) == D2R_ArithmeticType_Signed;
}

internal B32
d2r_is_value_type_integral(D2R_ValueType v)
{
  D2R_ArithmeticType at = d2r_arithmetic_type_from_value_type(v);
  return at == D2R_ArithmeticType_Unsigned || at == D2R_ArithmeticType_Signed;
}

internal B32
d2r_is_value_type_unsigned(D2R_ValueType v)
{
  return d2r_arithmetic_type_from_value_type(v) == D2R_ArithmeticType_Unsigned;
}

internal B32
d2r_is_value_type_float(D2R_ValueType v)
{
  return d2r_arithmetic_type_from_value_type(v) == D2R_ArithmeticType_Float;
}

internal U64
d2r_size_from_value_type(U64 addr_size, D2R_ValueType value_type)
{
  if (value_type == D2R_ValueType_Address) {
    return addr_size;
  }

  switch (value_type) {
#define X(_N, _AT, _S) case D2R_ValueType_##_N: return _S;
    D2R_ValueType_XList
#undef X
  }

  return 0;
}

internal D2R_ValueType
d2r_pick_common_value_type(D2R_ValueType lhs, D2R_ValueType rhs)
{
  if (lhs == rhs) {
    return lhs;
  }
  // unsigned vs unsigned
  else if (d2r_is_value_type_unsigned(lhs) && d2r_is_value_type_unsigned(rhs)) {
    return Max(lhs, rhs);
  }
  // signed vs signed
  else if (d2r_is_value_type_signed(lhs) && d2r_is_value_type_signed(rhs)) {
    return Max(lhs, rhs);
  }
  // (unsigned vs signed) || (signed vs unsigned)
  else if ((d2r_is_value_type_unsigned(lhs) && d2r_is_value_type_signed(rhs)) ||
           (d2r_is_value_type_signed(lhs) && d2r_is_value_type_unsigned(rhs))) {
    U64 lhs_size = d2r_size_from_value_type(0, lhs);
    U64 rhs_size = d2r_size_from_value_type(0, rhs);
    if (lhs_size < rhs_size) {
      return rhs;
    } else if (lhs > rhs_size) {
      return lhs;
    } else {
      return d2r_unsigned_value_type_from_bit_size(lhs_size * 8);
    }
  }
  // float vs int
  else if (d2r_is_value_type_float(lhs) && d2r_is_value_type_integral(rhs)) {
    return lhs;
  }
  // int vs float
  else if (d2r_is_value_type_integral(lhs) && d2r_is_value_type_float(rhs)) {
    return rhs;
  }
  // float vs float
  else if (d2r_is_value_type_float(lhs) && d2r_is_value_type_float(rhs)) {
    return Max(lhs, rhs);
  }
  // address vs int
  else if (lhs == D2R_ValueType_Address && d2r_is_value_type_integral(rhs)) {
    return D2R_ValueType_Address;
  }
  // int vs address
  else if (d2r_is_value_type_integral(lhs) && rhs == D2R_ValueType_Address) {
    return D2R_ValueType_Address;
  }
  // address vs float
  else if (lhs == D2R_ValueType_Address && d2r_is_value_type_float(rhs)) {
    return D2R_ValueType_Generic;
  }
  // float vs address
  else if (d2r_is_value_type_float(lhs) && rhs == D2R_ValueType_Address) {
    return D2R_ValueType_Generic;
  }
  // no conversion for implicit value
  else if (lhs == D2R_ValueType_ImplicitValue || rhs == D2R_ValueType_ImplicitValue) {
    return D2R_ValueType_Generic;
  }
  AssertAlways(!"undefined conversion case");
  return D2R_ValueType_Generic;
}

internal RDI_EvalTypeGroup
d2r_value_type_to_rdi(D2R_ValueType v)
{
  if (v == D2R_ValueType_Generic) {
    return RDI_EvalTypeGroup_Other;
  }

  if (v == D2R_ValueType_Address) {
    return RDI_EvalTypeGroup_U;
  }

  if (d2r_is_value_type_unsigned(v)) {
    return RDI_EvalTypeGroup_U;
  }

  if (d2r_is_value_type_signed(v)) {
    return RDI_EvalTypeGroup_S;
  }

  if (d2r_is_value_type_float(v)) {
    if      (v == D2R_ValueType_F16)  { NotImplemented;                }
    else if (v == D2R_ValueType_F32)  { return RDI_EvalTypeGroup_F32;  }
    else if (v == D2R_ValueType_F48)  { NotImplemented;                }
    else if (v == D2R_ValueType_F64)  { return RDI_EvalTypeGroup_F64;  }
    else if (v == D2R_ValueType_F80)  { return RDI_EvalTypeGroup_F80;  }
    else if (v == D2R_ValueType_F96)  { NotImplemented;                }
    else if (v == D2R_ValueType_F128) { return RDI_EvalTypeGroup_F128; }
    else { InvalidPath; }
  }

  InvalidPath;
  return RDI_EvalTypeGroup_Other;
}

internal D2R_ValueType
d2r_apply_usual_arithmetic_conversions(Arena *arena, D2R_ValueType lhs, D2R_ValueType rhs, RDIM_EvalBytecode *bc)
{
  D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
  if (rhs != common_type) {
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(rhs), d2r_value_type_to_rdi(common_type));
  }
  if (lhs != common_type) {
    rdim_bytecode_push_op(arena, bc, RDI_EvalOp_Swap, 0);
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(lhs), d2r_value_type_to_rdi(common_type));
  }
  return common_type;
}

internal void
d2r_push_arithmetic_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op)
{
  D2R_ValueType rhs         = d2r_value_type_stack_pop(stack);
  D2R_ValueType lhs         = d2r_value_type_stack_pop(stack);
  D2R_ValueType common_type = d2r_apply_usual_arithmetic_conversions(arena, lhs, rhs, bc);
  rdim_bytecode_push_op(arena, bc, op, d2r_value_type_to_rdi(common_type));
  d2r_value_type_stack_push(0, stack, common_type);
}

internal void
d2r_push_relational_op(Arena *arena, D2R_ValueTypeStack *stack, RDIM_EvalBytecode *bc, RDI_EvalOp op)
{
  D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
  D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
  D2R_ValueType common_type;
  if (d2r_is_value_type_integral(lhs) && rhs == D2R_ValueType_Address) {
    rdim_bytecode_push_op(arena, bc, RDI_EvalOp_Swap, 0);
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(lhs), RDI_EvalTypeGroup_U);
    rdim_bytecode_push_op(arena, bc, RDI_EvalOp_Swap, 0);
    common_type = D2R_ValueType_Address;
  } else if (lhs == D2R_ValueType_Address && d2r_is_value_type_integral(rhs)) {
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(rhs), RDI_EvalTypeGroup_U);
    common_type = D2R_ValueType_Address;
  } else {
    common_type = d2r_apply_usual_arithmetic_conversions(arena, lhs, rhs, bc);
  }
  rdim_bytecode_push_op(arena, bc, RDI_EvalOp_EqEq, d2r_value_type_to_rdi(common_type));
  d2r_value_type_stack_push(0, stack, D2R_ValueType_Bool);
}

internal RDIM_EvalBytecode
d2r_bytecode_from_expression(Arena         *arena,
                             DW_Input      *input,
                             U64            image_base,
                             Arch           arch,
                             DW_ListUnit   *addr_lu,
                             String8        raw_expr,
                             DW_CompUnit   *cu,
                             D2R_ValueType *result_type_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  Temp temp    = temp_begin(arena);
  B32  is_ok   = 0;

  RDIM_EvalBytecode bc = {0};
  
  DW_Expr               expr            = dw_expr_from_data(scratch.arena, cu->format, cu->address_size, raw_expr);
  D2R_ValueTypeStack   *stack           = push_array(scratch.arena, D2R_ValueTypeStack, 1);
  RDIM_EvalBytecodeOp **converted_insts = push_array(scratch.arena, RDIM_EvalBytecodeOp *, expr.count);
  U64                   inst_idx        = 0;
  for EachNode(inst, DW_ExprInst, expr.first) {
    RDIM_EvalBytecodeOp *last_op = bc.last_op;
    
    U64 pop_count = dw_pop_count_from_expr_op(inst->opcode);
    if (pop_count > stack->count) {
      Assert(!"not enough values on the stack to evaluate instruction");
      goto exit;
    }
    
    switch (inst->opcode) {
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
        U64 lit = inst->opcode - DW_ExprOp_Lit0;
        rdim_bytecode_push_uconst(arena, &bc, lit);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U8);
      } break;
      case DW_ExprOp_Const1U: {
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u8);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U8);
      } break;
      case DW_ExprOp_Const2U: {
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u16);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U16);
      } break;
      case DW_ExprOp_Const4U: {
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u32);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U32);
      } break;
      case DW_ExprOp_Const8U: {
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u32);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U64);
      } break;
      case DW_ExprOp_Const1S: {
        rdim_bytecode_push_sconst(arena, &bc, inst->operands[0].s8);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_S8);
      } break;
      case DW_ExprOp_Const2S: {
        rdim_bytecode_push_sconst(arena, &bc, inst->operands[0].s16);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_S16);
      } break;
      case DW_ExprOp_Const4S: {
        rdim_bytecode_push_sconst(arena, &bc, inst->operands[0].s32);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_S32);
      } break;
      case DW_ExprOp_Const8S: {
        rdim_bytecode_push_sconst(arena, &bc, inst->operands[0].s64);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_S64);
      } break;
      case DW_ExprOp_ConstU: {
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u64);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_U64);
      } break;
      case DW_ExprOp_ConstS: {
        rdim_bytecode_push_sconst(arena, &bc, inst->operands[0].s64);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_S64);
      } break;
      case DW_ExprOp_Addr: {
        if (inst->operands[0].u64 < image_base) {
          Assert(!"invalid address");
          goto exit;
        }

        U64 voff = inst->operands[0].u64 - image_base;
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, voff);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
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
        U64 reg_code_dw  = inst->opcode - DW_ExprOp_Reg0;
        U64 reg_size     = dw_reg_size_from_code(arch, reg_code_dw);
        U64 reg_pos      = dw_reg_pos_from_code(arch, reg_code_dw);
        
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
        U32         regread_param = RDI_EncodeRegReadParam(reg_code_rdi, reg_size, reg_pos);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, regread_param);
        d2r_value_type_stack_push(scratch.arena, stack, d2r_unsigned_value_type_from_bit_size(reg_size));
      } break;
      case DW_ExprOp_RegX: {
        U64         reg_size      = dw_reg_size_from_code(arch, inst->operands[0].u64);
        U64         reg_pos       = dw_reg_pos_from_code(arch, inst->operands[0].u64);
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_code_from_dw_reg(arch, inst->operands[0].u64);
        U32         regread_param = RDI_EncodeRegReadParam(reg_code_rdi, reg_size, reg_pos);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, regread_param);
        D2R_ValueType value_type;
        if (arch == Arch_x64) {
          if (reg_code_rdi == RDI_RegCodeX64_st0 ||
              reg_code_rdi == RDI_RegCodeX64_st1 ||
              reg_code_rdi == RDI_RegCodeX64_st2 ||
              reg_code_rdi == RDI_RegCodeX64_st3 ||
              reg_code_rdi == RDI_RegCodeX64_st4 ||
              reg_code_rdi == RDI_RegCodeX64_st5 ||
              reg_code_rdi == RDI_RegCodeX64_st6 ||
              reg_code_rdi == RDI_RegCodeX64_st7) {
            value_type = d2r_float_type_from_bit_size(reg_size);
          } else {
            value_type = d2r_unsigned_value_type_from_bit_size(reg_size);
          }
        } else {
          value_type = d2r_unsigned_value_type_from_bit_size(reg_size);
        }
        d2r_value_type_stack_push(scratch.arena, stack, value_type);
      } break;
      case DW_ExprOp_ImplicitValue: {
        if (inst->operands[0].block.size <= sizeof(U64)) {
          U64 implicit_value;
          MemoryCopyStr8(&implicit_value, inst->operands[0].block);
          rdim_bytecode_push_uconst(arena, &bc, implicit_value);
          d2r_value_type_stack_push(scratch.arena, stack, d2r_unsigned_value_type_from_bit_size(inst->operands[0].block.size * 8));
        } else {
          // TODO: currenlty no way to encode string in RDIM_EvalBytecodeOp
          Assert(!"TODO:");
          goto exit;
        }
      } break;
      case DW_ExprOp_Piece: {
        U64 partial_value_size32 = safe_cast_u32(inst->operands[0].u64);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_PartialValue, partial_value_size32);
      } break;
      case DW_ExprOp_BitPiece: {
        U32 piece_bit_size32 = safe_cast_u32(inst->operands[0].u64);
        U32 piece_bit_off32  = safe_cast_u32(inst->operands[1].u64);
        U64 partial_value    = Compose64Bit(piece_bit_size32, piece_bit_off32);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_PartialValueBit, partial_value);
      } break;
      case DW_ExprOp_Pick: {
        U64 idx = 0;
        D2R_ValueTypeNode *n;
        for (n = stack->top; n != 0 || idx == inst->operands[0].u64; n = n->next, idx += 1) { }
        if (idx == inst->operands[0].u64) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, inst->operands[0].u64);
          d2r_value_type_stack_push(scratch.arena, stack, n->type);
        } else {
          Assert(!"out of bounds pick");
          goto exit;
        }
      } break;
      case DW_ExprOp_Over: {
        if (stack->top && stack->top->next) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, 1);
          d2r_value_type_stack_push(scratch.arena, stack, stack->top->next->type);
        } else {
          Assert(!"out of bounds over");
          goto exit;
        }
      } break;
      case DW_ExprOp_PlusUConst: {
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType common_type = d2r_pick_common_value_type(lhs, D2R_ValueType_U64);
        rdim_bytecode_push_uconst(arena, &bc, inst->operands[0].u64);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, d2r_value_type_to_rdi(common_type));
        d2r_value_type_stack_push(scratch.arena, stack, common_type);
      } break;
      case DW_ExprOp_Skip: {
        B32 skip_fwd   = inst->operands[0].s16 >= 0;
        U16 delta      = abs_s64(inst->operands[0].s16);
        U16 cursor     = 0;
        U64 inst_count = 0;
        for (DW_ExprInst *i = skip_fwd ? inst : inst->prev; i != 0 && cursor < delta; i = skip_fwd ? inst->next : inst->prev) {
          cursor += i->size;
          inst_count += 1;
        }

        if (cursor != delta)      { Assert(!"skip landed in middle of an instruction"); goto exit; }
        if (inst_count > min_S16) { Assert(!"operand overflow");                        goto exit; }
        if (inst_idx > max_U32)   { Assert(!"index overflow");                          goto exit; }
        
        U64 imm = Compose64Bit(inst_idx, skip_fwd ? (S16)inst_count : (U16)(-(S16)inst_count));
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Skip, imm);
      } break;
      case DW_ExprOp_Bra: {
        B32 skip_fwd   = inst->operands[0].s16 >= 0;
        U16 delta      = abs_s64(inst->operands[0].s16);
        U16 cursor     = 0;
        U64 inst_count = 0;
        for (DW_ExprInst *i = skip_fwd ? inst->next : inst->prev; i != 0 && cursor < delta; i = skip_fwd ? inst->next : inst->prev) {
          cursor += i->size;
          inst_count += 1;
        }

        if (cursor != delta)      { Assert(!"cond landed in middle of an instruction"); goto exit; }
        if (inst_count > min_S16) { Assert(!"operand overflow");                        goto exit; }
        if (inst_idx > max_U32)   { Assert(!"index overflow");                          goto exit; }

        U64 imm = Compose64Bit(inst_idx, skip_fwd ? inst_count : (U16)(-(S16)inst_count));
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Cond, imm);
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
        U64 reg_code_dw = inst->opcode - DW_ExprOp_BReg0;
        S64 reg_off     = inst->operands[0].s64;
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, reg_code_rdi);
        if (reg_off > 0) {
          rdim_bytecode_push_sconst(arena, &bc, reg_off);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
        }
        
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_BRegX: {
        U64 reg_code_dw = inst->operands[0].u64;
        S64 reg_off     = inst->operands[1].s64;
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegReadDyn, reg_code_rdi);
        if (reg_off > 0) {
          rdim_bytecode_push_sconst(arena, &bc, reg_off);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
        }
        
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_FBReg: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_FrameOff, inst->operands[0].s64);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_Deref: {
        D2R_ValueType address_type = d2r_value_type_stack_pop(stack);
        if (address_type != D2R_ValueType_Address && !d2r_is_value_type_integral(address_type)) {
          Assert(!"value must be of integral type");
          goto exit;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, cu->address_size);
        d2r_value_type_stack_push(scratch.arena, stack, address_type);
      } break;
      case DW_ExprOp_DerefSize: {
        D2R_ValueType address_type = d2r_value_type_stack_pop(stack);
        if (!d2r_is_value_type_integral(address_type) && address_type != D2R_ValueType_Address ) {
          Assert(!"value must be of integral type");
          goto exit;
        }
        U8 deref_size_in_bytes = inst->operands[0].u64;
        if (0 < deref_size_in_bytes && deref_size_in_bytes <= cu->address_size) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, deref_size_in_bytes);
        } else {
          Assert(!"ill formed expression");
          goto exit;
        }
        d2r_value_type_stack_push(scratch.arena, stack, address_type);
      } break;
      case DW_ExprOp_XDeref: {
        Assert(!"multiple address spaces are not supported");
        goto exit;
      } break;
      case DW_ExprOp_XDerefSize: {
        Assert(!"no suitable conversion");
        goto exit;
      } break;
      case DW_ExprOp_Call2:
      case DW_ExprOp_Call4:
      case DW_ExprOp_CallRef: {
        Assert(!"calls are not supported");
        goto exit;
      } break;
      case DW_ExprOp_ImplicitPointer:
      case DW_ExprOp_GNU_ImplicitPointer: {
        // TODO: RDI does not support encoding of implicit pointers
        //Assert(!"TODO: implicit pointer");
        goto exit;
      } break;
      case DW_ExprOp_Convert:
      case DW_ExprOp_GNU_Convert: {
        D2R_ValueType out = D2R_ValueType_Generic;
        if (inst->operands[0].u64 == 0) {
          //
          // 2.5.1
          // Instead of a base type, elements can have a generic type,
          // which is an integral type that has the size of an address
          // on the target machine and unspecified signedness.
          //
          out = D2R_ValueType_Generic;
        } else {
          // find ref tag
          U64 ref_tag_info_off = cu->info_range.min + inst->operands[0].u64;
          DW_TagNode *tag_node = dw_tag_node_from_info_off(cu, ref_tag_info_off);
          if (tag_node == 0) {
            Assert(!"invalid .debug_info offset");
            goto exit;
          }

          DW_Tag tag = tag_node->tag;
          if (tag.kind == DW_TagKind_BaseType) {
            // extract encoding attribute
            DW_ATE encoding = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Encoding);

            // DW_ATE -> RDI_EvalTypeGroup
            switch (encoding) {
            case DW_ATE_Null: {
              out = D2R_ValueType_Generic;
            } break;
            case DW_ATE_Address: {
              out = D2R_ValueType_Address;
            } break;
            case DW_ATE_Boolean: {
              out = D2R_ValueType_S8;
            } break;
            case DW_ATE_SignedChar:
            case DW_ATE_Signed: {
              U64 byte_size = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ByteSize);
              out = d2r_signed_value_type_from_bit_size(byte_size * 8);
            } break;
            case DW_ATE_UnsignedChar:
            case DW_ATE_Unsigned: {
              U64 byte_size = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ByteSize);
              out = d2r_unsigned_value_type_from_bit_size(byte_size * 8);
            } break;
            case DW_ATE_Float: {
              U64 byte_size = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ByteSize);
              out = d2r_float_type_from_bit_size(byte_size * 8);
            } break;
            default: InvalidPath; break;
            }
          } else {
            Assert(!"unexpected tag");
            goto exit;
          }
        }
        
        D2R_ValueType in = d2r_value_type_stack_pop(stack);
        d2r_value_type_stack_push(scratch.arena, stack, out);
        rdim_bytecode_push_convert(arena, &bc, d2r_value_type_to_rdi(in), d2r_value_type_to_rdi(out));
      } break;
      case DW_ExprOp_GNU_ParameterRef: {
        Assert(!"TODO: sample");
        goto exit;
      }
      case DW_ExprOp_DerefType:
      case DW_ExprOp_GNU_DerefType: {
        goto exit;
      }
      case DW_ExprOp_ConstType: 
      case DW_ExprOp_GNU_ConstType: {
        Assert(!"TODO: sample");
        goto exit;
      }
      case DW_ExprOp_RegvalType: {
        Assert(!"sample");
        goto exit;
      }
      case DW_ExprOp_EntryValue:
      case DW_ExprOp_GNU_EntryValue: {
        D2R_ValueType call_site_result_type = 0;
        RDIM_EvalBytecode call_site_bc = d2r_bytecode_from_expression(arena, input, image_base, arch, addr_lu, inst->operands[0].block, cu, &call_site_result_type);
        
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_CallSiteValue, safe_cast_u32(call_site_bc.encoded_size));
        rdim_bytecode_concat_in_place(&bc, &call_site_bc);
        
        d2r_value_type_stack_push(scratch.arena, stack, call_site_result_type);
      } break;
      case DW_ExprOp_Addrx: {
        U64 addr = dw_addr_from_list_unit(addr_lu, inst->operands[0].u64);
        if (addr != max_U64) {
          if (addr >= image_base) {
            U64 voff = addr - image_base;
            rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, voff);
            d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
          } else {
            Assert(!"unable to relocate address");
            goto exit;
          }
        } else {
          Assert(!"out of bounds index");
          goto exit;
        }
      } break;
      case DW_ExprOp_CallFrameCfa: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_PushCfa, 0);
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_PushObjectAddress: {
        Assert(!"TODO: sample");
        goto exit;
      }
      case DW_ExprOp_Nop: {} break;
      case DW_ExprOp_Eq:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_EqEq);   } break;
      case DW_ExprOp_Ge:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_GrEq);   } break;
      case DW_ExprOp_Gt:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_Grtr);   } break;
      case DW_ExprOp_Le:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_LsEq);   } break;
      case DW_ExprOp_Lt:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_Less);   } break;
      case DW_ExprOp_Ne:    { d2r_push_relational_op(arena, stack, &bc, RDI_EvalOp_NtEq);   } break;
      case DW_ExprOp_Div:   { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_Div);    } break;
      case DW_ExprOp_Minus: { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_Sub);    } break;
      case DW_ExprOp_Mul:   { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_Mul);    } break;
      case DW_ExprOp_Plus:  { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_Add);    } break;
      case DW_ExprOp_Xor:   { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_BitXor); } break;
      case DW_ExprOp_And:   { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_BitAnd); } break;
      case DW_ExprOp_Or:    { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_BitOr);  } break;
      case DW_ExprOp_Shl:   { d2r_push_arithmetic_op(arena, stack, &bc, RDI_EvalOp_LShift); } break;
      case DW_ExprOp_Shr: {
        D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (d2r_is_value_type_integral(rhs) && d2r_is_value_type_integral(lhs)) {
          D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
          D2R_ValueType result_type = d2r_unsigned_value_type_from_bit_size(d2r_size_from_value_type((cu->address_size), common_type) * 8);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, d2r_value_type_to_rdi(result_type));
          d2r_value_type_stack_push(scratch.arena, stack, result_type);
        } else {
          Assert(!"operands must be of integral type");
          goto exit;
        }
      } break;
      case DW_ExprOp_Shra: {
        D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (d2r_is_value_type_integral(lhs) && d2r_is_value_type_integral(rhs)) {
          D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
          D2R_ValueType result_type = d2r_signed_value_type_from_bit_size(d2r_size_from_value_type((cu->address_size), common_type) * 8);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, d2r_value_type_to_rdi(result_type));
          d2r_value_type_stack_push(scratch.arena, stack, result_type);
        } else {
          Assert(!"operands must be of integral type");
          goto exit;
        }
      } break;
      case DW_ExprOp_Mod: {
        D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (!d2r_is_value_type_integral(rhs) || !d2r_is_value_type_integral(lhs)) {
          Assert(!"operands must be of integral type");
          goto exit;
        }
        D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Mod, d2r_value_type_to_rdi(common_type));
        d2r_value_type_stack_push(scratch.arena, stack, common_type);
      } break;
      case DW_ExprOp_Abs: {
        if (!d2r_is_value_type_integral(d2r_value_type_stack_peek(stack)) && !d2r_is_value_type_float(d2r_value_type_stack_peek(stack))) {
          Assert(!"operand must be of integral type or float");
          goto exit;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Abs, d2r_value_type_to_rdi(d2r_value_type_stack_peek(stack)));
      } break;
      case DW_ExprOp_Neg: {
        if (!d2r_is_value_type_integral(d2r_value_type_stack_peek(stack))) {
          Assert(!"operand must be of integral type");
          goto exit;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Neg, d2r_value_type_to_rdi(d2r_value_type_stack_peek(stack)));
      } break;
      case DW_ExprOp_Not: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_BitNot, d2r_value_type_to_rdi(d2r_value_type_stack_peek(stack)));
      } break;
      case DW_ExprOp_Dup: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, 0);
        d2r_value_type_stack_push(scratch.arena, stack, d2r_value_type_stack_peek(stack));
      } break;
      case DW_ExprOp_Rot: {
        Assert(!"no suitable conversion");
        goto exit;
      } 
      case DW_ExprOp_Swap: {
        D2R_ValueType a = d2r_value_type_stack_pop(stack);
        D2R_ValueType b = d2r_value_type_stack_pop(stack);
        d2r_value_type_stack_push(scratch.arena, stack, a);
        d2r_value_type_stack_push(scratch.arena, stack, b);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Swap, 0);
      } break;
      case DW_ExprOp_Drop: {
        d2r_value_type_stack_pop(stack);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pop, 0);
      } break;
      case DW_ExprOp_StackValue: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Stop, 0);
        if (stack->top->type == D2R_ValueType_Address) {
          stack->top->type = d2r_unsigned_value_type_from_bit_size(cu->address_size * 8);
        }
      } break;
      case DW_ExprOp_FormTlsAddress:
      case DW_ExprOp_GNU_PushTlsAddress: {
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (!d2r_is_value_type_integral(lhs)) {
          Assert(!"lhs must be of integral type");
          goto exit;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_TLSOff, 0);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, d2r_value_type_to_rdi(lhs));
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_GNU_UnInit: {
        // TODO: flag value as unitialized; this must be last opcode; possible to use with DW_ExprOp_Piece;
      } break;
      default: goto exit;
    }
    
    // store converted instruction
    if (last_op != bc.last_op) {
      RDIM_EvalBytecodeOp *first_converted_op = last_op ? last_op->next : bc.first_op;
      converted_insts[inst_idx] = first_converted_op;
    }
    
    inst_idx += 1;
  }
  
  // fixup bytecode jumps
  for EachNode(op, RDIM_EvalBytecodeOp, bc.first_op) {
    if (op->op == RDI_EvalOp_Skip || op->op == RDI_EvalOp_Cond) {
      // unpack skip info
      U32 inst_idx          = Extract32(op->p, 1);
      S16 skip_count_signed = (S16)Extract32(op->p, 0);
      U16 skip_count        = abs_s64(skip_count_signed);
      B32 skip_fwd          = skip_count_signed > 0;
      
      // setup being/end links
      RDIM_EvalBytecodeOp *begin = 0, *end = 0;
      if (skip_fwd) {
        if (inst_idx + skip_count <= expr.count) {
          begin = converted_insts[inst_idx];
          end   = (inst_idx + skip_count) < expr.count ? converted_insts[inst_idx + skip_count] : 0;
        } else {
          Assert(!"out of bounds skip");
          goto exit;
        }
      } else {
        if (skip_count <= inst_idx) {
          begin = converted_insts[inst_idx - skip_count];
          end   = converted_insts[inst_idx];
        } else {
          Assert(!"out of bounds skip");
          goto exit;
        }
      }
      
      // compute skip delta
      U64 skip_delta = 0;
      for (RDIM_EvalBytecodeOp *n = begin; n != end; n = n->next) {
        skip_delta += n->p_size;
      }
      
      // rewrite skip operand with byte delta
      AssertAlways(skip_delta <= max_S16);
      op->p = skip_fwd ? (S16)skip_delta : -(S16)skip_delta;
    }
  }
  
  if (result_type_out) {
    *result_type_out = d2r_value_type_stack_peek(stack);
  }

  is_ok = 1;
exit:;
  if (!is_ok) {
    MemoryZeroStruct(&bc);
    temp_end(temp);
  }
  scratch_end(scratch);
  return bc;
}

internal RDIM_Location *
d2r_transpile_expression(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr)
{
  RDIM_Location *loc = 0;
  if (expr.size) {
    D2R_ValueType result_type = 0;
    RDIM_EvalBytecode bytecode = d2r_bytecode_from_expression(arena, input, image_base, arch, addr_lu, expr, cu, &result_type);
    
    RDIM_LocationInfo *loc_info = push_array(arena, RDIM_LocationInfo, 1);
    loc_info->kind     = result_type == D2R_ValueType_Address ? RDI_LocationKind_AddrBytecodeStream : RDI_LocationKind_ValBytecodeStream;
    loc_info->bytecode = bytecode;
    
    loc = rdim_location_chunk_list_push_new(arena, locations, D2R_LOCATIONS_CAP, loc_info);
  }
  return loc;
}

internal RDIM_Location *
d2r_location_from_attrib(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind)
{
  String8 expr = dw_exprloc_from_tag_attrib_kind(input, cu, tag, kind);
  RDIM_Location *location = d2r_transpile_expression(arena, locations, input, image_base, arch, cu->addr_lu, cu, expr);
  return location;
}

internal RDIM_LocationCaseList
d2r_locset_from_attrib(Arena                  *arena,
                       RDIM_ScopeChunkList    *scopes,
                       RDIM_Scope             *curr_scope,
                       RDIM_LocationChunkList *locations,
                       DW_Input               *input,
                       DW_CompUnit            *cu,
                       U64                     image_base,
                       Arch                    arch,
                       DW_Tag                  tag,
                       DW_AttribKind           kind)
{
  RDIM_LocationCaseList locset = {0};
  
  // extract attrib from tag
  DW_Attrib      *attrib       = dw_attrib_from_tag(input, cu, tag, kind);
  DW_AttribClass  attrib_class = dw_value_class_from_attrib(cu, attrib);
  
  if (attrib_class == DW_AttribClass_LocList || attrib_class == DW_AttribClass_LocListPtr) {
    Temp scratch = scratch_begin(&arena, 1);
    
    // extract location list from attrib
    DW_LocList loclist = dw_loclist_from_attrib(scratch.arena, input, cu, attrib);
    
    // convert location list to RDIM location set
    for EachNode(loc_n, DW_LocNode, loclist.first) {
      RDIM_Location *location   = d2r_transpile_expression(arena, locations, input, image_base, arch, cu->addr_lu, cu, loc_n->v.expr);
      RDIM_Rng1U64   voff_range = { .min = loc_n->v.range.min -  image_base, .max = loc_n->v.range.max - image_base };
      rdim_push_location_case(arena, scopes, &locset, location, voff_range);
    }
    
    scratch_end(scratch);
  } else if (attrib_class == DW_AttribClass_ExprLoc) {
    // extract expression from attrib
    String8 expr = dw_exprloc_from_attrib(input, cu, attrib);
    
    // convert expression and inherit life-time ranges from enclosed scope
    RDIM_Location *location = d2r_transpile_expression(arena, locations, input, image_base, arch, cu->addr_lu, cu, expr);
    for EachNode(range_n, RDIM_Rng1U64Node, curr_scope->voff_ranges.first) {
      rdim_push_location_case(arena, scopes, &locset, location, range_n->v);
    }
  } else if (attrib_class != DW_AttribClass_Null) {
    log_user_errorf("unexpected attrib class @ .debug_info+%llx", tag.info_off);
  }
  
  return locset;
}

internal RDIM_LocationCaseList
d2r_var_locset_from_tag(Arena                  *arena,
                        RDIM_ScopeChunkList    *scopes,
                        RDIM_Scope             *curr_scope,
                        RDIM_LocationChunkList *locations,
                        DW_Input               *input,
                        DW_CompUnit            *cu,
                        U64                     image_base,
                        Arch                    arch,
                        DW_Tag                  tag)
{
  RDIM_LocationCaseList locset = {0};
  
  B32 has_const_value = dw_tag_has_attrib(input, cu, tag, DW_AttribKind_ConstValue);
  B32 has_location    = dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Location);
  
  if (has_const_value && has_location) {
    log_user_errorf("unexpected variable encoding @ .debug_info+%llx", tag.info_off);
    return locset;
  }
  
  if (has_const_value) {
    // extract const value
    U64 const_value = dw_u64_from_attrib(input, cu, tag, DW_AttribKind_ConstValue);
    
    // make value byte code
    RDIM_EvalBytecode bc = {0};
    rdim_bytecode_push_uconst(arena, &bc, const_value);
    
    // fill out location
    RDIM_LocationInfo *loc_info = push_array(arena, RDIM_LocationInfo, 1);
    loc_info->kind     = RDI_LocationKind_ValBytecodeStream;
    loc_info->bytecode = bc;
    RDIM_Location *loc = rdim_location_chunk_list_push_new(arena, locations, D2R_LOCATIONS_CAP, loc_info);
    
    // push location cases
    for EachNode(range_n, RDIM_Rng1U64Node, curr_scope->voff_ranges.first) {
      rdim_push_location_case(arena, scopes, &locset, loc, range_n->v);
    }
  } else if (has_location) {
    locset = d2r_locset_from_attrib(arena, scopes, curr_scope, locations, input, cu, image_base, arch, tag, DW_AttribKind_Location);
  }
  
  return locset;
}

internal RDIM_Type *
d2r_create_type(Arena *arena, D2R_TypeTable *type_table)
{
  RDIM_Type *type = rdim_type_chunk_list_push(arena, type_table->types, type_table->type_chunk_cap);
  return type;
}

internal RDIM_Type *
d2r_create_type_from_offset(Arena *arena, D2R_TypeTable *type_table, U64 info_off)
{
  RDIM_Type *type = d2r_create_type(arena, type_table);
  Assert(hash_table_search_u64_raw(type_table->ht, info_off) == 0);
  hash_table_push_u64_raw(arena, type_table->ht, info_off, type);
  return type;
}

internal RDIM_Type *
d2r_type_from_offset(D2R_TypeTable *type_table, U64 info_off)
{
  RDIM_Type *type = hash_table_search_u64_raw(type_table->ht, info_off);
  if (type == 0) {
    type = type_table->builtin_types[RDI_TypeKind_NULL];
  }
  return type;
}

internal RDIM_Type *
d2r_type_from_attrib(D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  RDIM_Type *type = type_table->builtin_types[RDI_TypeKind_Void];
  
  // find attrib
  DW_Attrib *attrib = dw_attrib_from_tag(input, cu, tag, kind);
  
  // does tag have this attribute?
  if (attrib->attrib_kind == kind) {
    DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
    
    if (value_class == DW_AttribClass_Reference) {
      // resolve reference
      DW_Reference ref = dw_ref_from_attrib(input, cu, attrib);
      
      if (ref.cu == cu) {
        // find type
        type = d2r_type_from_offset(type_table, ref.info_off);
      } else {
        NotImplemented;
      }
    } else {
      log_user_errorf("unexpected attrib class @ .debug_info+%llx", tag.info_off);
    }
  }
  
  return type;
}

internal D2R_TagIterator *
d2r_tag_iterator_init(Arena *arena, DW_TagNode *root)
{
  D2R_TagIterator *iter = push_array(arena, D2R_TagIterator, 1);
  iter->free_list            = 0;
  iter->stack                = push_array(arena, D2R_TagFrame, 1);
  iter->stack->node          = push_array(arena, DW_TagNode, 1);
  if (root != 0) {
    *iter->stack->node         = *root;
  }
  iter->stack->node->sibling = 0;
  iter->visit_children       = 1;
  iter->tag_node             = root;
  return iter;
}

internal void
d2r_tag_iterator_next(Arena *arena, D2R_TagIterator *iter)
{
  // descend to first child
  if (iter->visit_children) {
    if (iter->stack->node->first_child) {
      D2R_TagFrame *f = iter->free_list;
      if (f) { SLLStackPop(iter->free_list); MemoryZeroStruct(f); }
      else   { f = push_array(arena, D2R_TagFrame, 1); }
      f->node = iter->stack->node->first_child;
      SLLStackPush(iter->stack, f);
      goto exit;
    }
  }
  
  while (iter->stack) {
    // go to sibling
    iter->stack->node = iter->stack->node->sibling;
    if (iter->stack->node) { break; }
    
    // no more siblings, go up
    D2R_TagFrame *f = iter->stack;
    SLLStackPop(iter->stack);
    SLLStackPush(iter->free_list, f);
  }
  
  exit:;
  // update iterator
  iter->visit_children = 1;
  iter->tag_node       = iter->stack ? iter->stack->node : 0;
}

internal void
d2r_tag_iterator_skip_children(D2R_TagIterator *iter)
{
  iter->visit_children = 0;
}

internal DW_TagNode *
d2r_tag_iterator_parent_tag_node(D2R_TagIterator *iter)
{
  return iter->stack->next->node;
}

internal DW_Tag
d2r_tag_iterator_parent_tag(D2R_TagIterator *iter)
{
  DW_TagNode *tag_node = d2r_tag_iterator_parent_tag_node(iter);
  return tag_node->tag;
}

internal void
d2r_flag_converted_type_tag(DW_TagNode *tag_node)
{
  tag_node->tag.v |= D2R_TagFlags_TypeConverted;
}

internal B8
d2r_is_type_tag_converted(DW_TagNode *tag_node)
{
  return !!(tag_node->tag.v & D2R_TagFlags_TypeConverted);
}

internal RDIM_Type *
d2r_find_or_convert_type(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, Arch arch, DW_Tag tag, DW_AttribKind kind)
{
  RDIM_Type *type = type_table->builtin_types[RDI_TypeKind_Void];
  
  // find attrib
  DW_Attrib *attrib = dw_attrib_from_tag(input, cu, tag, kind);
  
  // does tag have this attribute?
  if (attrib->attrib_kind == kind) {
    DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
    
    if (value_class == DW_AttribClass_Reference) {
      // resolve reference
      DW_Reference ref = dw_ref_from_attrib(input, cu, attrib);
      
      if (ref.cu == cu) {
        // find type
        type = d2r_type_from_offset(type_table, ref.info_off);

        // was type converted?
        if (type == 0) {
          // issue type conversion
          DW_TagNode *ref_node = dw_tag_node_from_info_off(cu, ref.info_off);
          d2r_convert_types(arena, type_table, input, cu, cu_lang, arch, ref_node);

          // if we do not have a converted type at this point then debug info is malformed
          type = d2r_type_from_offset(type_table, ref.info_off);
          if (type == 0) {
            type = type_table->builtin_types[RDI_TypeKind_NULL];
          }
        }
      } else {
        NotImplemented;
      }
    } else {
      log_user_errorf("unexpected attrib class @ .debug_info+%llx", tag.info_off);
    }
  }
  
  return type;
}

internal void
d2r_convert_types(Arena         *arena,
                  D2R_TypeTable *type_table,
                  DW_Input      *input,
                  DW_CompUnit   *cu,
                  DW_Language    cu_lang,
                  Arch           arch,
                  DW_TagNode    *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  for (D2R_TagIterator *it = d2r_tag_iterator_init(scratch.arena, root); it->tag_node != 0; d2r_tag_iterator_next(scratch.arena, it)) {
    DW_TagNode *tag_node = it->tag_node;
    DW_Tag      tag      = tag_node->tag;
    
    // skip converted tags
    if (d2r_is_type_tag_converted(tag_node)) {
      d2r_tag_iterator_skip_children(it);
      continue;
    }
    // mark the tag as converted here, because during conversion we may recurse on the same tag
    d2r_flag_converted_type_tag(tag_node);
    
    switch (tag.kind) {
      case DW_TagKind_ClassType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->kind = RDI_TypeKind_IncompleteClass;
          type->name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          Assert(!tag_node->first_child);
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
          RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind        = RDI_TypeKind_Class;
          type->byte_size   = dw_byte_size_32_from_tag(input, cu, tag);
          type->direct_type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
        }
      } break;
      case DW_TagKind_StructureType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind = RDI_TypeKind_IncompleteStruct;
          
          if (tag_node->first_child) {
            d2r_tag_iterator_skip_children(it);
          } else {
            log_user_errorf("childless struct @ .debug_info+%llx", tag.info_off);
          }
        } else {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name      = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind      = RDI_TypeKind_Struct;
          type->byte_size = dw_byte_size_32_from_tag(input, cu, tag);
        }
      } break;
      case DW_TagKind_UnionType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name      = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind      = RDI_TypeKind_IncompleteUnion;
          
          if (tag_node->first_child) {
            d2r_tag_iterator_skip_children(it);
          } else {
            log_user_errorf("childless union @ .debug_info+%llx", tag.info_off);
          }
        } else {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name      = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind      = RDI_TypeKind_Union;
          type->byte_size = dw_byte_size_32_from_tag(input, cu, tag);
        }
      } break;
      case DW_TagKind_EnumerationType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          RDIM_Type *type = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name      = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind      = RDI_TypeKind_IncompleteEnum;
          if (tag_node->first_child) {
            d2r_tag_iterator_skip_children(it);
          } else {
            log_user_errorf("childless enum @ .debug_info+%llx", tag.info_off);
          }
        } else {
          RDIM_Type *enum_base_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
          RDIM_Type *type           = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind        = RDI_TypeKind_Enum;
          type->byte_size   = dw_byte_size_32_from_tag(input, cu, tag);
          type->direct_type = enum_base_type;
        }
      } break;
      case DW_TagKind_SubroutineType: {
        RDIM_Type *ret_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        
        // collect parameters
        RDIM_TypeList param_list = {0};
        for (DW_TagNode *n = tag_node->first_child; n != 0; n = n->sibling) {
          if (n->tag.kind == DW_TagKind_FormalParameter) {
            RDIM_Type *param_type = d2r_type_from_attrib(type_table, input, cu, n->tag, DW_AttribKind_Type);
            rdim_type_list_push(scratch.arena, &param_list, param_type);
          } else if (n->tag.kind == DW_TagKind_UnspecifiedParameters) {
            rdim_type_list_push(scratch.arena, &param_list, type_table->builtin_types[RDI_TypeKind_Variadic]);
          } else {
            log_user_errorf("unexpected tag @ .debug_info+%llx", tag.info_off);
          }
        }
        
        // init proceudre type
        RDIM_Type *type     = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind          = RDI_TypeKind_Function;
        type->byte_size     = cu->address_size;
        type->direct_type   = ret_type;
        type->count         = param_list.count;
        type->param_types   = rdim_array_from_type_list(arena, param_list);
        
        d2r_tag_iterator_skip_children(it);
      } break;
      case DW_TagKind_Typedef: {
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Alias;
        type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        type->direct_type = direct_type;
        for (RDIM_Type *n = direct_type; n != 0; n = n->direct_type) {
          if (n->byte_size) {
            type->byte_size = n->byte_size;
            break;
          }
        }
      } break;
      case DW_TagKind_BaseType: {
        DW_ATE encoding  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Encoding);
        U64    byte_size = dw_byte_size_from_tag(input, cu, tag);
        
        // convert base type encoding to RDI version
        RDI_TypeKind kind = RDI_TypeKind_NULL;
        #define X(t,g,s) case s: kind = RDI_TypeKind_##t; break;
        switch (encoding) {
          case DW_ATE_Null:    kind = RDI_TypeKind_NULL; break;
          case DW_ATE_Address: kind = RDI_TypeKind_Void; break;
          case DW_ATE_Boolean: kind = RDI_TypeKind_Bool; break;
          case DW_ATE_ComplexFloat: {
            switch (byte_size) {
              case 8:  kind = RDI_TypeKind_ComplexF32;  break;
              case 16: kind = RDI_TypeKind_ComplexF64;  break;
              case 24: kind = RDI_TypeKind_ComplexF80;  break;
              case 32: kind = RDI_TypeKind_ComplexF128; break;
              default: log_user_errorf("unexpected size"); break;
            }
          } break;
          case DW_ATE_Float: {
            String8 name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            if (arch == Arch_x64 || arch == Arch_arm64) {
              if (str8_match(name, str8_lit("__float80"), 0)) {
                kind = RDI_TypeKind_F80;
              } else if (str8_match(name, str8_lit("__float128"), 0)) {
                kind = RDI_TypeKind_F128;
              } else if (str8_match(name, str8_lit("_Float16"), 0)) {
                kind = RDI_TypeKind_F16;
              } else if (str8_match(name, str8_lit("__bf16"), 0)) {
                kind = RDI_TypeKind_BF16;
              }
              if (kind != RDI_TypeKind_NULL) { break; }
            }

            switch (byte_size) {
              D2R_ValueType_Float_XList
            default: log_user_errorf("unexpected size"); break; 
            }
          } break;
          case DW_ATE_Signed: {
            String8 name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            if (str8_match(name, str8_lit("wchar_t"), 0)) { goto do_signed_char; }

            switch (byte_size) {
              D2R_ValueType_Signed_XList
              default: log_user_errorf("unexpected size"); break;
            }
          } break;
          do_signed_char:;
          case DW_ATE_SignedChar: {
            switch (byte_size) {
              case 1: kind = RDI_TypeKind_Char8;  break;
              case 2: kind = RDI_TypeKind_Char16; break;
              case 4: kind = RDI_TypeKind_Char32; break;
              default: log_user_errorf("unexpected size"); break;
            }
          } break;
          case DW_ATE_Unsigned: {
            switch (byte_size) {
              D2R_ValueType_Unsigned_XList
              default: log_user_errorf("unexpected size"); break;
            }
          } break;
          case DW_ATE_Utf:
          case DW_ATE_UnsignedChar: {
            switch (byte_size) {
              case 1: kind = RDI_TypeKind_UChar8;  break;
              case 2: kind = RDI_TypeKind_UChar16; break;
              case 4: kind = RDI_TypeKind_UChar32; break;
              default: log_user_errorf("unexpected size"); break;
            }
          } break;
          case DW_ATE_DecimalFloat: {
            switch (byte_size) {
            case 4:  kind = RDI_TypeKind_Decimal32; break;
            case 8:  kind = RDI_TypeKind_Decimal64; break;
            case 16: kind = RDI_TypeKind_Decimal128; break;
            default: log_user_errorf("unexpected size"); break;
            }
          } break;
          case DW_ATE_ImaginaryFloat: {
            NotImplemented;
          } break;
          case DW_ATE_PackedDecimal: {
            NotImplemented;
          } break;
          case DW_ATE_NumericString: {
            NotImplemented;
          } break;
          case DW_ATE_Edited: {
            NotImplemented;
          } break;
          case DW_ATE_SignedFixed: {
            NotImplemented;
          } break;
          case DW_ATE_UnsignedFixed: {
            NotImplemented;
          } break;
          case DW_ATE_Ucs: {
            NotImplemented;
          } break;
          case DW_ATE_Ascii: {
            NotImplemented;
          } break;
          default: log_user_errorf("unexpected base type encoding"); break;
        }
        #undef X
        
        RDIM_Type *type   = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Alias;
        type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        type->direct_type = type_table->builtin_types[kind];
        type->byte_size   = byte_size;
      } break;
      case DW_TagKind_PointerType: {
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        
        // TODO:
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Allocated)) {
          log_infof("TODO: handle allocated attrib @ .debug_info+%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Associated)) {
          log_infof("TODO: handle associated attrib @ .debug_info%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment)) {
          log_infof("TODO: handle alignment attrib @ .debug_info+%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name)) {
          log_infof("TODO: handle name attrib @ .debug_info+%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_AddressClass)) {
          log_infof("TODO: handle address class attrib @ .debug_info+%llx", tag.info_off);
        }
        
        U64 byte_size = cu->address_size;
        if (cu->version == DW_Version_5 || cu->relaxed) {
          dw_try_byte_size_from_tag(input, cu, tag, &byte_size);
        }
        
        RDIM_Type *type   = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Ptr;
        type->byte_size   = byte_size;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_RestrictType: {
        // TODO:
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment)) {
          log_infof("TODO: handle alignment attrib @ .debug_info+%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name)) {
          log_infof("TODO: handle name attrib @ .debug_info+%llx", tag.info_off);
        }
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = cu->address_size;
        type->flags       = RDI_TypeModifierFlag_Restrict;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_VolatileType: {
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name)) {
          log_infof("TODO: handle name attrib @ .debug_info+%llx", tag.info_off);
        }
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = cu->address_size;
        type->flags       = RDI_TypeModifierFlag_Volatile;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_ConstType: {
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name)) {
          log_infof("TODO: handle name attrib @ .debug_info+%llx", tag.info_off);
        }
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment)) {
          log_infof("TODO: handle alignment attrib @ .debug_info+%llx", tag.info_off);
        }
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = cu->address_size;
        type->flags       = RDI_TypeModifierFlag_Const;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_ArrayType: {
        // TODO: @native_vector_support extract byte size from the base type tag
        // and convert to U256, U512, S256 and S512
        B32 is_vector = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_GNU_Vector);
        if (is_vector) { NotImplemented; }

        B32 error = 1;

        // * DWARF vs RDI Array Type Graph *
        //
        // For example lets take following decl:
        //
        //    int (*foo[2])[3];
        // 
        //  This compiles to in DWARF:
        //  
        //  foo -> DW_TAG_ArrayType -> (A0) DW_TAG_Subrange [2]
        //                          \
        //                           -> (B0) DW_TAG_PointerType -> (A1) DW_TAG_ArrayType -> DW_TAG_Subrange [3]
        //                                                      \
        //                                                       -> (B1) DW_TAG_BaseType (int)
        // 
        // RDI expects:
        //  
        //  foo -> Array[2] -> Pointer -> Array[3] -> int
        //
        // Note that DWARF forks the graph on DW_TAG_ArrayType to describe array ranges in branch A and
        // in branch B describes array type which might be a struct, pointer, base type, or any other type tag.
        // However, in RDI we have a simple list of type nodes and to convert we need to append type nodes from
        // B to A.
        struct SubrangeNode { struct SubrangeNode *next; U64 count; };
        struct SubrangeNode *subrange_stack = 0;
        for (DW_TagNode *n = tag_node->first_child; n != 0; n = n->sibling) {
          if (n->tag.kind != DW_TagKind_SubrangeType) {
            log_user_errorf("[%llx] while scanning for DW_TagKind_SubrangeType found an unexpected child tag %S @ %llx\n", tag.info_off, dw_string_from_tag_kind(scratch.arena, n->tag.kind), n->tag.info_off);
            goto array_type_exit;
          }
          
          // resolve lower bound
          U64 lower_bound = 0;
          if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_LowerBound)) {
            B32 is_vla = dw_is_attrib_var_ref(input, cu, dw_attrib_from_tag(input, cu, n->tag, DW_AttribKind_LowerBound));
            if (is_vla) {
              log_user_errorf("[%llx] TODO: lower bound is a variable\n", n->tag.info_off);
            } else {
              lower_bound = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_LowerBound);
            }
          } else {
            lower_bound = dw_pick_default_lower_bound(cu_lang);
          }
          
          // resolve upper bound
          U64 upper_bound = 0;
          if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_Count)) {
            U64 count = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_Count);
            upper_bound = lower_bound + count;
          } else if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_UpperBound)) {
            B32 is_vla = dw_is_attrib_var_ref(input, cu, dw_attrib_from_tag(input, cu, n->tag, DW_AttribKind_UpperBound));
            if (is_vla) {
              log_user_errorf("[%llx] TODO: upper bound is a variable\n", n->tag.info_off);
              goto array_type_exit;
            } else {
              upper_bound = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_UpperBound);
              upper_bound += 1; // turn upper bound into exclusive range
            }
          } else {
            // zero sized array
          }
          
          struct SubrangeNode *s = push_array(scratch.arena, struct SubrangeNode, 1);
          s->count = upper_bound - lower_bound;
          SLLStackPush(subrange_stack, s);
        }
        
        RDIM_Type *array_base_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
        RDIM_Type *direct_type     = array_base_type;
        U64        size_cursor     = array_base_type->byte_size;
        for EachNode(s, struct SubrangeNode, subrange_stack) {
          size_cursor *= s->count;
          
          RDIM_Type *t;
          if (s->next) { t = d2r_create_type(arena, type_table); }
          else         { t = d2r_create_type_from_offset(arena, type_table, tag.info_off); }
          
          t->kind        = RDI_TypeKind_Array;
          t->direct_type = direct_type;
          t->byte_size   = size_cursor;
          t->count       = s->count;
          
          direct_type = t;
        }
        
        error = 0;
        array_type_exit:;

        // in case of an error, assign null to the array type
        if (error) {
          Assert(d2r_type_from_offset(type_table, tag.info_off) == 0); // this should the first time this type is being parsed
          hash_table_push_u64_raw(arena, type_table->ht, tag.info_off, type_table->builtin_types[RDI_TypeKind_NULL]);
        }

        d2r_tag_iterator_skip_children(it);
      } break;
      case DW_TagKind_SubrangeType: {
        log_user_errorf("unexpected tag");
      } break;
      case DW_TagKind_Inheritance: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_StructureType || parent_tag.kind == DW_TagKind_ClassType) {
          RDIM_Type      *parent = d2r_type_from_offset(type_table, parent_tag.info_off);
          RDIM_Type      *type   = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch, tag, DW_AttribKind_Type);
          RDIM_UDTMember *member = rdim_udt_push_member(arena, &g_d2r_shared.udts, parent->udt);
          member->kind           = RDI_MemberKind_Base;
          member->type           = type;
          member->off            = safe_cast_u32(dw_const_u32_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_DataMemberLocation));
        } else {
          log_user_errorf("unexpected parent tag");
        }
      } break;
    }
  }
  scratch_end(scratch);
}

internal B8
d2r_is_udt_tag_converted(DW_TagNode *tag_node)
{
  return !!(tag_node->tag.v & D2R_TagFlags_UdtConverted);
}

internal void
d2r_flag_converted_udt_tag(DW_TagNode *tag_node)
{
  tag_node->tag.v |= D2R_TagFlags_UdtConverted;
}

internal void
d2r_inline_anonymous_udt_member(Arena *arena, RDIM_UDT *top_udt, U64 base_off, RDIM_UDT *udt)
{
  for (RDIM_UDTMember *curr = udt->first_member, *prev = 0; curr != 0; prev = curr, curr = curr->next) {
    if (curr->name.size == 0) {
      d2r_inline_anonymous_udt_member(arena, top_udt, base_off + curr->off, curr->type->udt);
    } else {
      // copy member and adjust its offset relative to inlined position in the root UDT
      RDIM_UDTMember *m = rdim_udt_push_member(arena, &g_d2r_shared.udts, top_udt);
      *m = *curr;
      m->off += base_off;
    }
  }
}

internal void
d2r_convert_udts(Arena         *arena,
                 D2R_TypeTable *type_table,
                 DW_Input      *input,
                 DW_CompUnit   *cu,
                 DW_Language    cu_lang,
                 DW_TagNode    *root)
{
  Temp scratch = scratch_begin(&arena, 1);

  for (D2R_TagIterator *it = d2r_tag_iterator_init(scratch.arena, root); it->tag_node != 0; d2r_tag_iterator_next(scratch.arena, it)) {
    DW_TagNode *tag_node = it->tag_node;
    DW_Tag      tag      = tag_node->tag;

    // skip converted tags
    if (d2r_is_udt_tag_converted(tag_node)) {
      d2r_tag_iterator_skip_children(it);
      continue;
    }
    d2r_flag_converted_udt_tag(tag_node);

    if (tag.kind == DW_TagKind_ClassType || tag.kind == DW_TagKind_StructureType || tag.kind == DW_TagKind_UnionType) {
      B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
      if (is_decl) {
        d2r_tag_iterator_skip_children(it);
      } else {
        RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
        RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &g_d2r_shared.udts, D2R_UDT_CHUNK_CAP);
        udt->self_type = type;
        type->udt      = udt;
      }
    } else if (tag.kind == DW_TagKind_EnumerationType) {
      B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
      if (is_decl) {
        d2r_tag_iterator_skip_children(it);
      } else {
        RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
        RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &g_d2r_shared.udts, D2R_UDT_CHUNK_CAP);
        udt->self_type = type;
        type->udt      = udt;
      }
    } else if (tag.kind == DW_TagKind_Member) {
      DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
      B32 is_parent_udt = parent_tag.kind == DW_TagKind_StructureType ||
                          parent_tag.kind == DW_TagKind_ClassType     ||
                          parent_tag.kind == DW_TagKind_UnionType;
      if (is_parent_udt) {
        RDIM_Type *parent_type = d2r_type_from_offset(type_table, parent_tag.info_off);
        RDIM_Type *type        = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
        String8    name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        U64        off         = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_DataMemberLocation);
        if (name.size == 0) { // inline anonymous members
          DW_Reference  type_ref    = dw_ref_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Type);
          RDIM_Type    *member_type = d2r_type_from_offset(type_table, type_ref.info_off);
          RDIM_UDT     *member_udt  = member_type->udt;
          if (member_udt == 0) {
            DW_Language ref_cu_lang  = dw_const_u64_from_tag_attrib_kind(input, type_ref.cu, type_ref.cu->tag, DW_AttribKind_Language);
            DW_TagNode *ref_tag_node = dw_tag_node_from_info_off(type_ref.cu, type_ref.info_off);
            d2r_convert_udts(arena, type_table, input, type_ref.cu, ref_cu_lang, ref_tag_node);
            Assert(member_type->udt);
            member_udt = member_type->udt;
          }
          d2r_inline_anonymous_udt_member(arena, parent_type->udt, off, member_udt);
        } else {
          RDIM_UDTMember *member  = rdim_udt_push_member(arena, &g_d2r_shared.udts, parent_type->udt);
          member->kind = RDI_MemberKind_DataField;
          member->name = name;
          member->type = type;
          member->off  = off;
        }
      } else {
        log_user_errorf("unexpected parent tag @ .debug_info+%llx", tag.info_off);
      }
    } else if (tag.kind == DW_TagKind_Enumerator) {
      DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
      if (parent_tag.kind == DW_TagKind_EnumerationType) {
        RDIM_Type       *parent_type = d2r_type_from_offset(type_table, parent_tag.info_off);
        RDIM_UDTEnumVal *udt_member  = rdim_udt_push_enum_val(arena, &g_d2r_shared.udts, parent_type->udt);
        udt_member->name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        udt_member->val  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ConstValue);
      } else {
        log_user_errorf("unexpected parent tag @ .debug_info+%llx", tag.info_off);
      }
    }
  }
  scratch_end(scratch);
}

internal RDIM_Scope *
d2r_push_scope(Arena *arena, RDIM_ScopeChunkList *scopes, U64 scope_chunk_cap, D2R_TagFrame *tag_stack, Rng1U64List ranges)
{
  // fill out scope
  RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, scopes, scope_chunk_cap);
  
  // push ranges
  for EachNode(i, Rng1U64Node, ranges.first) {
    rdim_scope_push_voff_range(arena, scopes, scope, (RDIM_Rng1U64){.min = i->v.min, i->v.max});
  }
  
  // associate scope with tag
  tag_stack->scope = scope;
  
  // update scope hierarchy
  DW_TagKind parent_tag_kind = tag_stack->next->node->tag.kind;
  if (parent_tag_kind == DW_TagKind_SubProgram || parent_tag_kind == DW_TagKind_InlinedSubroutine || parent_tag_kind == DW_TagKind_LexicalBlock) {
    RDIM_Scope *parent = tag_stack->next->scope;
    
    scope->parent_scope = parent;
    scope->symbol       = parent->symbol;
    
    if (parent->last_child) {
      parent->last_child->next_sibling = scope;
    }
    SLLQueuePush_N(parent->first_child, parent->last_child, scope, next_sibling);
  }
  
  return scope;
}

internal RDIM_Type **
d2r_collect_proc_params(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_TagNode *cur_node, U64 *param_count_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  RDIM_TypeList list = {0};
  B32 has_vargs = 0;
  for (DW_TagNode *i = cur_node->first_child; i != 0; i = i->sibling) {
    if (i->tag.kind == DW_TagKind_FormalParameter) {
      RDIM_TypeNode *n = push_array(scratch.arena, RDIM_TypeNode, 1);
      n->v             = d2r_type_from_attrib(type_table, input, cu, i->tag, DW_AttribKind_Type);
      SLLQueuePush(list.first, list.last, n);
      ++list.count;
    } else if (i->tag.kind == DW_TagKind_UnspecifiedParameters) {
      has_vargs = 1;
    }
  }
  
  if (has_vargs) {
    RDIM_TypeNode *n = push_array(scratch.arena, RDIM_TypeNode, 1);
    n->v = type_table->builtin_types[RDI_TypeKind_Variadic];
    SLLQueuePush(list.first, list.last, n);
    ++list.count;
  }
  
  // collect params
  *param_count_out  = list.count;
  RDIM_Type **params = rdim_array_from_type_list(arena, list);
  
  scratch_end(scratch);
  return params;
}

internal Rng1U64List
d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag)
{
  // collect non-contiguous range
  Rng1U64List raw_ranges = dw_rnglist_from_tag_attrib_kind(arena, input, cu, tag, DW_AttribKind_Ranges);
  
  // exclude invalid ranges caused by linker optimizations
  Rng1U64List ranges = {0};
  for (Rng1U64Node *n = raw_ranges.first, *next = 0; n != 0; n = next) {
    next = n->next;
    if (n->v.min < image_base || n->v.min > n->v.max) {
      continue;
    }
    rng1u64_list_push_node(&ranges, n);
  }
  
  // debase ranges
  for EachNode(r, Rng1U64Node, ranges.first) {
    r->v.min -= image_base;
    r->v.max -= image_base;
  }
  
  // collect contiguous range
  {
    DW_Attrib *lo_pc_attrib = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_LowPc);
    DW_Attrib *hi_pc_attrib = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_HighPc);
    if (lo_pc_attrib->attrib_kind != DW_AttribKind_Null && hi_pc_attrib->attrib_kind != DW_AttribKind_Null) {
      U64 lo_pc = dw_address_from_attrib(input, cu, lo_pc_attrib);
      
      U64 hi_pc = 0;
      DW_AttribClass hi_pc_class = dw_value_class_from_attrib(cu, hi_pc_attrib);
      if (hi_pc_class == DW_AttribClass_Address) {
        hi_pc = dw_address_from_attrib(input, cu, hi_pc_attrib);
      } else if (hi_pc_class == DW_AttribClass_Const) {
        hi_pc = dw_const_u64_from_attrib(input, cu, hi_pc_attrib);
        hi_pc += lo_pc;
      }
      
      if (lo_pc >= image_base && hi_pc >= image_base) {
        if (lo_pc < hi_pc) {
          rng1u64_list_push(arena, &ranges, rng_1u64(lo_pc - image_base, hi_pc - image_base));
        } else {
          log_user_errorf("invalid range @ .debug_info+%llx", tag.info_off);
        }
      } else {
        // invalid low and hi PC are likely are caused by an optimization pass during linking
      }
    } else if ((lo_pc_attrib->attrib_kind == DW_AttribKind_Null && hi_pc_attrib->attrib_kind != DW_AttribKind_Null) ||
               (lo_pc_attrib->attrib_kind != DW_AttribKind_Null && hi_pc_attrib->attrib_kind == DW_AttribKind_Null)) {
        log_user_errorf("invalid range @ .debug_info+%llx", tag.info_off);
    }
  }
  
  return ranges;
}

internal void
d2r_convert_symbols(Arena         *arena,
                    D2R_TypeTable *type_table,
                    RDIM_Scope    *global_scope,
                    DW_Input      *input,
                    DW_CompUnit   *cu,
                    DW_Language    cu_lang,
                    U64            image_base,
                    Arch           arch,
                    DW_TagNode    *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  for (D2R_TagIterator *it = d2r_tag_iterator_init(scratch.arena, root); it->tag_node != 0; d2r_tag_iterator_next(scratch.arena, it)) {
    DW_TagNode *tag_node = it->tag_node;
    DW_Tag      tag      = tag_node->tag;
    switch (tag.kind) {
      case DW_TagKind_Null: { InvalidPath; } break;
      case DW_TagKind_ClassType:
      case DW_TagKind_StructureType:
      case DW_TagKind_UnionType: {
        // visit children to collect methods and variables
      } break;
      case DW_TagKind_EnumerationType:
      case DW_TagKind_SubroutineType:
      case DW_TagKind_Typedef:
      case DW_TagKind_BaseType:
      case DW_TagKind_PointerType:
      case DW_TagKind_RestrictType:
      case DW_TagKind_VolatileType:
      case DW_TagKind_ConstType:
      case DW_TagKind_ArrayType:
      case DW_TagKind_SubrangeType:
      case DW_TagKind_Inheritance:
      case DW_TagKind_Enumerator:
      case DW_TagKind_Member: {
        d2r_tag_iterator_skip_children(it);
      } break;
      case DW_TagKind_SubProgram: {
        DW_InlKind inl = DW_Inl_NotInlined;
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Inline)) { inl = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Inline); }
        
        switch (inl) {
          case DW_Inl_NotInlined: {
            U64         param_count = 0;
            RDIM_Type **params      = d2r_collect_proc_params(arena, type_table, input, cu, tag_node, &param_count);
            
            // get return type
            RDIM_Type *ret_type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
            
            // fill out proc type
            RDIM_Type *proc_type   = d2r_create_type(arena, type_table);
            proc_type->kind        = RDI_TypeKind_Function;
            proc_type->byte_size   = cu->address_size;
            proc_type->direct_type = ret_type;
            proc_type->count       = param_count;
            proc_type->param_types = params;
            
            // get container type
            RDIM_Type *container_type = 0;
            if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_ContainingType)) {
              container_type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_ContainingType);
            }
            
            // get frame base expression
            String8 frame_base_expr = dw_exprloc_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_FrameBase);
            
            // get proc container symbol
            RDIM_Symbol *proc = rdim_symbol_chunk_list_push(arena, &g_d2r_shared.procs, D2R_PROC_CHUNK_CAP);
            
            // make scope
            Rng1U64List  ranges     = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
            RDIM_Scope  *root_scope = d2r_push_scope(arena, &g_d2r_shared.scopes, D2R_SCOPE_CHUNK_CAP, it->stack, ranges);
            root_scope->symbol      = proc;
            
            // fill out proc
            proc->is_extern        = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_External);
            proc->name             = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            proc->link_name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_LinkageName);
            proc->type             = proc_type;
            proc->container_symbol = 0;
            proc->container_type   = container_type;
            proc->root_scope       = root_scope;
            proc->location_cases   = d2r_locset_from_attrib(arena, &g_d2r_shared.scopes, root_scope, &g_d2r_shared.locations, input, cu, image_base, arch, tag, DW_AttribKind_FrameBase);
            
            // sub program with user-defined parent tag is a method
            DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
            if (parent_tag.kind == DW_TagKind_ClassType || parent_tag.kind == DW_TagKind_StructureType) {
              RDI_MemberKind    member_kind = RDI_MemberKind_NULL;
              DW_VirtualityKind virtuality  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Virtuality);
              switch (virtuality) {
                case DW_VirtualityKind_None:        member_kind = RDI_MemberKind_Method;        break;
                case DW_VirtualityKind_Virtual:     member_kind = RDI_MemberKind_VirtualMethod; break;
                case DW_VirtualityKind_PureVirtual: member_kind = RDI_MemberKind_VirtualMethod; break; // TODO: create kind for pure virutal
                default: { log_user_errorf("unhandled virtuality kind"); } break;
              }
              
              RDIM_Type      *type   = d2r_type_from_offset(type_table, parent_tag.info_off);
              RDIM_UDTMember *member = rdim_udt_push_member(arena, &g_d2r_shared.udts, type->udt);
              member->kind           = member_kind;
              member->type           = type;
              member->name           = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            } else if (parent_tag.kind != DW_TagKind_CompileUnit) {
              log_user_errorf("unexpected tag @ .debug_info+%llx", tag.info_off);
            }
            
            it->stack->scope = root_scope;
          } break;
          case DW_Inl_DeclaredNotInlined:
          case DW_Inl_DeclaredInlined:
          case DW_Inl_Inlined: {
            d2r_tag_iterator_skip_children(it);
          } break;
          default: InvalidPath; break;
        }
      } break;
      case DW_TagKind_InlinedSubroutine: {
        U64         param_count = 0;
        RDIM_Type **params      = d2r_collect_proc_params(arena, type_table, input, cu, tag_node, &param_count);
        
        // get return type
        RDIM_Type *ret_type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
        
        // fill out proc type
        RDIM_Type *proc_type   = d2r_create_type(arena, type_table);
        proc_type->kind        = RDI_TypeKind_Function;
        proc_type->byte_size   = cu->address_size;
        proc_type->direct_type = ret_type;
        proc_type->count       = param_count;
        proc_type->param_types = params;
        
        // get container type
        RDIM_Type *owner = 0;
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_ContainingType)) {
          owner = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_ContainingType);
        }
        
        // fill out inline site
        RDIM_InlineSite *inline_site = rdim_inline_site_chunk_list_push(arena, &g_d2r_shared.inline_sites, D2R_INLINE_SITE_CHUNK_CAP);
        inline_site->name            = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        inline_site->type            = proc_type;
        inline_site->owner           = owner;
        inline_site->line_table      = 0;
        
        // make scope
        Rng1U64List  ranges     = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
        RDIM_Scope  *root_scope = d2r_push_scope(arena, &g_d2r_shared.scopes, D2R_SCOPE_CHUNK_CAP, it->stack, ranges);
        root_scope->inline_site = inline_site;
      } break;
      case DW_TagKind_Variable: {
        String8    name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        RDIM_Type *type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
        
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_SubProgram ||
            parent_tag.kind == DW_TagKind_InlinedSubroutine ||
            parent_tag.kind == DW_TagKind_LexicalBlock) {
          RDIM_Scope *scope = it->stack->next->scope;
          RDIM_Local *local = rdim_scope_push_local(arena, &g_d2r_shared.scopes, scope);
          local->kind           = RDI_LocalKind_Variable;
          local->name           = name;
          local->type           = type;
          local->location_cases = d2r_var_locset_from_tag(arena, &g_d2r_shared.scopes, scope, &g_d2r_shared.locations, input, cu, image_base, arch, tag);
        } else {
          
          // NOTE: due to a bug in clang in stb_sprintf.h local variables
          // are declared in global scope without a name
          if (name.size == 0) { break; }

          // decls do not have location info
          B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
          if (is_decl) { break; }
          
          U64 voff          = max_U64;
          B32 is_thread_var = 0;
          {
            DW_Attrib      *loc_attrib = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_Location);
            DW_AttribClass  loc_class  = dw_value_class_from_attrib(cu, loc_attrib);
            if (loc_class == DW_AttribClass_ExprLoc) {
              Temp temp = temp_begin(scratch.arena);
              
              String8           expr      = dw_exprloc_from_attrib(input, cu, loc_attrib);
              D2R_ValueType     expr_type = 0;
              RDIM_EvalBytecode bc        = d2r_bytecode_from_expression(temp.arena, input, image_base, arch, cu->addr_lu, expr, cu, &expr_type);
              
              // evaluate bytecode to virutal offset if possible
              if (expr_type == D2R_ValueType_Address) {
                B32 is_static = rdim_is_eval_bytecode_static(bc);
                if (is_static) {
                  if (!rdim_static_eval_bytecode_to_voff(bc, image_base, &voff)) {
                    log_user_errorf("failed to evalute byte code to virtual offset @ .debug_info+%llx", tag.info_off);
                  }
                }
              }
              
              // is this a thread variable?
              is_thread_var = rdim_is_bytecode_tls_dependent(bc);
              
              temp_end(temp);
            }
          }
          
          RDIM_SymbolChunkList *var_chunks; U64 var_chunks_cap;
          if (is_thread_var) { var_chunks = &g_d2r_shared.tvars; var_chunks_cap = D2R_TVAR_CHUNK_CAP; }
          else               { var_chunks = &g_d2r_shared.gvars; var_chunks_cap = D2R_GVAR_CHUNK_CAP; }
          
          RDIM_Symbol *var = rdim_symbol_chunk_list_push(arena, var_chunks, var_chunks_cap);
          var->is_extern        = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_External);
          var->name             = name;
          var->link_name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_LinkageName);
          var->type             = type;
          var->offset           = voff;
          var->container_symbol = 0;
          var->container_type   = 0; // TODO: NotImplemented;
        }
      } break;
      case DW_TagKind_FormalParameter: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_SubProgram || parent_tag.kind == DW_TagKind_InlinedSubroutine) {
          RDIM_Scope *scope = it->stack->next->scope;
          RDIM_Local *param = rdim_scope_push_local(arena, &g_d2r_shared.scopes, scope);
          param->kind           = RDI_LocalKind_Parameter;
          param->name           = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          param->type           = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
          param->location_cases = d2r_var_locset_from_tag(arena, &g_d2r_shared.scopes, scope, &g_d2r_shared.locations, input, cu, image_base, arch, tag);
        } else {
          Assert(!"this is a local variable");
          log_user_errorf(".debug_info+%llx out of scope formal parameter", tag.info_off);
        }
      } break;
      case DW_TagKind_LexicalBlock: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_SubProgram ||
            parent_tag.kind == DW_TagKind_InlinedSubroutine ||
            parent_tag.kind == DW_TagKind_LexicalBlock) {
          Rng1U64List ranges = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
          d2r_push_scope(arena, &g_d2r_shared.scopes, D2R_SCOPE_CHUNK_CAP, it->stack, ranges);
        }
      } break;
      case DW_TagKind_CallSite: {
        // TODO
      } break;
      case DW_TagKind_CallSiteParameter: {
        // TODO
      } break;
      case DW_TagKind_Label:
      case DW_TagKind_CompileUnit:
      case DW_TagKind_UnspecifiedParameters:
      case DW_TagKind_Namespace:
      case DW_TagKind_ImportedDeclaration:
      case DW_TagKind_PtrToMemberType:
      case DW_TagKind_TemplateTypeParameter:
      case DW_TagKind_ReferenceType: {
        // TODO:
      } break;
      default:
      {
        // NotImplemented;
      } break;
    }
  }
  scratch_end(scratch);
}

force_inline int
d2r_src_file_lookup_compar(void *raw_a, void *raw_b)
{
  D2R_SrcFileLookup *a = raw_a, *b = raw_b;
  DW_LineFile *file_a = a->file, *file_b = b->file;
  String8 dir_path_a = a->vm->header.dir_table.v[file_a->dir_idx];
  String8 dir_path_b = b->vm->header.dir_table.v[file_b->dir_idx];
  int cmp = str8_compar(dir_path_a, dir_path_b, 0);
  if (cmp == 0) {
    cmp = str8_compar(file_a->path, file_b->path, 0);
  }
  return cmp;
}

force_inline
LFHT_IS_KEY_EQUAL_FUNC(d2r_src_file_lookup_is_equal)
{
  return d2r_src_file_lookup_compar(a, b) == 0;
}

force_inline int
d2r_src_file_lfht_key_value_is_before(void *raw_a, void *raw_b)
{
  return d2r_src_file_lookup_compar(*(D2R_SrcFileLookup **)raw_a, *(D2R_SrcFileLookup **)raw_b) < 0;
}

internal U64
d2r_hash_line_file(String8 dir_path, DW_LineFile *file)
{
  XXH3_state_t hasher = {0};
  XXH3_64bits_reset(&hasher);
  XXH3_64bits_update(&hasher, dir_path.str, dir_path.size);
  XXH3_64bits_update(&hasher, file->path.str, file->path.size);
  XXH3_64bits_update(&hasher, &file->time_stamp, sizeof(file->time_stamp));
  XXH3_64bits_update(&hasher, &file->md5, sizeof(file->md5));
  XXH64_hash_t hash = XXH3_64bits_digest(&hasher);
  return hash;
}

internal void
d2r_sort_ptrs(void **ptrs, U64 count, int (* is_before)(void *a, void *b))
{
  radsort(ptrs, count, is_before);
}

internal D2R_CompUnitContribMap
d2r_cu_contrib_map_from_aranges(Arena *arena, DW_Input *input, U64 image_base)
{
  Temp scratch      = scratch_begin(&arena, 1);
  Temp temp         = temp_begin(arena);
  B32  parse_failed = 1;
  
  Rng1U64List unit_range_list = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_ARanges].data);
  
  D2R_CompUnitContribMap cm = {0};
  cm.info_off_arr   = push_array(temp.arena, U64,                   unit_range_list.count);
  cm.voff_range_arr = push_array(temp.arena, RDIM_Rng1U64ChunkList, unit_range_list.count);
  
  for EachNode(range_n, Rng1U64Node, unit_range_list.first) {
    String8 unit_data   = str8_substr(input->sec[DW_Section_ARanges].data, range_n->v);
    U64     unit_cursor = 0;

    U64 unit_length;
    TryRead(str8_deserial_read_dwarf_packed_size(unit_data, unit_cursor, &unit_length), unit_cursor, exit);
    DW_Format unit_format = DW_FormatFromSize(unit_length);

    DW_Version version;
    TryRead(str8_deserial_read_struct(unit_data, unit_cursor, &version), unit_cursor, exit);
    
    if (version != DW_Version_2) {
      log_user_errorf("unknown .debug_aranges version %u @ 0x%llx", version, range_n->v.min);
      continue;
    }

    U64 cu_info_off;
    TryRead(str8_deserial_read_dwarf_uint(unit_data, unit_cursor, unit_format, &cu_info_off), unit_cursor, exit);

    U8 address_size = 0;
    TryRead(str8_deserial_read_struct(unit_data, unit_cursor, &address_size), unit_cursor, exit);
    
    U8 segment_selector_size = 0;
    TryRead(str8_deserial_read_struct(unit_data, unit_cursor, &segment_selector_size), unit_cursor, exit);
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = unit_cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      unit_cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    RDIM_Rng1U64ChunkList voff_ranges = {0};
    if (segment_selector_size == 0) {
      while (unit_cursor + address_size * 2 <= unit_data.size) {
        U64 address = 0;
        U64 length  = 0;
        TryRead(str8_deserial_read(unit_data, unit_cursor, &address, address_size, address_size), unit_cursor, exit);
        TryRead(str8_deserial_read(unit_data, unit_cursor, &length, address_size, address_size), unit_cursor, exit);
        
        if (address == 0 && length == 0) { break; }
        if (address == 0) { continue; }
        
        if (address < image_base) {
          log_user_errorf("invalid address 0x%llx in .debug_aranges+%llx", range_n->v.min + unit_cursor);
          continue;
        }
        
        U64 min = address - image_base;
        U64 max = min + length;
        rdim_rng1u64_chunk_list_push(temp.arena, &voff_ranges, 256, (RDIM_Rng1U64){.min = min, .max = max});
      }
    } else {
      NotImplemented;
    }
    
    U64 map_idx = cm.count++;
    cm.info_off_arr[map_idx]   = cu_info_off;
    cm.voff_range_arr[map_idx] = voff_ranges;
  }
  
  parse_failed = 0;
  exit:;
  if (parse_failed) {
    MemoryZeroStruct(&cm);
    temp_end(temp);
  }
  scratch_end(scratch);
  return cm;
}

internal RDIM_Rng1U64ChunkList
d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off)
{
  RDIM_Rng1U64ChunkList voff_ranges   = {0};
  U64                   voff_list_idx = u64_array_bsearch(map.info_off_arr, map.count, info_off);
  if (voff_list_idx < map.count) {
    voff_ranges = map.voff_range_arr[voff_list_idx];
  }
  return voff_ranges;
}

internal RDIM_BakeParams
d2r_convert(Arena *arena, D2R_ConvertParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);

  if (lane_idx() == 0) {
    ////////////////////////////////
    
    Arch      arch       = Arch_Null;
    U64       image_base = 0;
    DW_Input  input      = {0};
    PathStyle path_style = PathStyle_Null;
    
    switch (params->exe_kind) {
      case ExecutableImageKind_Null: {} break;
      case ExecutableImageKind_CoffPe: {
        PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, params->exe_data);
        String8             raw_sections  = str8_substr(params->exe_data, pe.section_table_range);
        COFF_SectionHeader *section_table = str8_deserial_get_raw_ptr(raw_sections, 0, sizeof(COFF_SectionHeader) * pe.section_count);
        String8             string_table  = str8_substr(params->exe_data, pe.string_table_range);

        arch       = pe.arch;
        image_base = pe.image_base;
        input      = dw_input_from_coff_section_table(scratch.arena, params->exe_data, string_table, pe.section_count, section_table);
        path_style = PathStyle_WindowsAbsolute;

        g_d2r_shared.binary_sections = c2r_rdi_binary_sections_from_coff_sections(arena, params->exe_data, string_table, pe.section_count, section_table);
      } break;
      case ExecutableImageKind_Elf32:
      case ExecutableImageKind_Elf64: {
        ELF_Bin bin = elf_bin_from_data(scratch.arena, params->dbg_data);

        arch       = arch_from_elf_machine(bin.hdr.e_machine);
        image_base = elf_base_addr_from_bin(&bin);
        input      = dw_input_from_elf_bin(scratch.arena, params->dbg_data, &bin);
        path_style = PathStyle_UnixAbsolute;

        g_d2r_shared.binary_sections = e2r_rdi_binary_sections_from_elf_section_table(arena, bin.shdrs);
      } break;
      default: { InvalidPath; } break;
    }
    
    ////////////////////////////////

    ProfBegin("compute exe hash");
    U64 exe_hash = rdi_hash(params->exe_data.str, params->exe_data.size);
    ProfEnd();
    
    g_d2r_shared.top_level_info = rdim_make_top_level_info(params->exe_name, arch, exe_hash, g_d2r_shared.binary_sections);
    
    ////////////////////////////////
    
    ProfBegin("Parse Unit Contrib Map");
    D2R_CompUnitContribMap cu_contrib_map = {0};
    if (input.sec[DW_Section_ARanges].data.size) {
      cu_contrib_map = d2r_cu_contrib_map_from_aranges(arena, &input, image_base);
    }
    ProfEnd();
    
    ProfBegin("Parse Comop Unit Ranges");
    DW_ListUnitInput lu_input      = dw_list_unit_input_from_input(scratch.arena, &input);
    Rng1U64List      cu_range_list = dw_unit_ranges_from_data(scratch.arena, input.sec[DW_Section_Info].data);
    Rng1U64Array     cu_ranges     = rng1u64_array_from_list(scratch.arena, &cu_range_list);
    ProfEnd();
    
    ////////////////////////////////
    
    ProfBegin("Parse Compile Unit Headers");
    DW_CompUnit *cu_arr = push_array(scratch.arena, DW_CompUnit, cu_ranges.count);
    for EachIndex(cu_idx, cu_ranges.count) {
      cu_arr[cu_idx] = dw_cu_from_info_off(scratch.arena, &input, lu_input, cu_ranges.v[cu_idx].min, params->is_parse_relaxed);
    }
    ProfEnd();
    
    ////////////////////////////////

    ProfBeginV("Convert Line Tables [Count: %llu]", cu_ranges.count);
    RDIM_LineTable **cu_line_tables_rdi = push_array(scratch.arena, RDIM_LineTable *, cu_ranges.count);
    {
      Temp temp = temp_begin(scratch.arena);

      // TODO: per thread task
      ProfBegin("Up front parse line VM headers");
      DW_LineVM **line_vms = push_array(temp.arena, DW_LineVM *, cu_ranges.count);
      for EachIndex(cu_idx, cu_ranges.count) {
        line_vms[cu_idx] = dw_line_vm_init(&input, &cu_arr[cu_idx]);
      }
      ProfEnd();

      // TODO: sync
      //
      // sum all source files (including duplicates)
      U64 total_file_count = 0;
      for EachIndex(cu_idx, cu_ranges.count) { total_file_count += line_vms[cu_idx]->header.file_table.count; }

      // TODO: sync
      ProfBeginV("Dedup source files [Count: %llu]", total_file_count);
      LFHT_NodeChunkList *src_file_lfht_nodes = push_array(temp.arena, LFHT_NodeChunkList, lane_count());
      LFHT_Node          *src_file_lfht       = 0;

      // TODO: per thread
      {
        D2R_SrcFileLookup *lookup = 0;
        for EachIndex(cu_idx, cu_ranges.count) {
          DW_LineVM *vm = line_vms[cu_idx];
          for EachIndex(file_idx, vm->header.file_table.count) {
            DW_LineFile *file = &vm->header.file_table.v[file_idx];

            if (lookup == 0) { lookup = push_array(temp.arena, D2R_SrcFileLookup, 1); }
            lookup->vm   = vm;
            lookup->file = file;

            LFHT_NodeChunkList *node_chunks  = &src_file_lfht_nodes[lane_idx()];
            U64                 hash         = d2r_hash_line_file(vm->header.dir_table.v[file->dir_idx], file);
            B32                 was_inserted = lfht_insert(temp.arena, node_chunks, &src_file_lfht, hash, lookup, d2r_src_file_lookup_is_equal, 0);
            if (was_inserted) {
              lookup = 0;
            }
          }
        }
      }
      ProfEnd();

      // TODO: sync
      ProfBegin("Extract source file lookups");
      U64    unique_file_count = lfht_total_count_from_node_chunk_lists(lane_count(), src_file_lfht_nodes);
      void **lookups           = lfht_data_from_node_chunk_lists(temp.arena, unique_file_count, lane_count(), src_file_lfht_nodes);
      ProfEnd();

      // TODO: sync
      ProfBeginV("Sort unique source files [Count: %llu]", unique_file_count);
      d2r_sort_ptrs(lookups, unique_file_count, d2r_src_file_lfht_key_value_is_before);
      ProfEnd();

      // TODO: per thread task
      ProfBegin("Convert source files");
      for EachIndex(i, unique_file_count) {
        D2R_SrcFileLookup *lookup = lookups[i];

        DW_LineFile  *src = lookup->file;
        RDIM_SrcFile *dst = rdim_src_file_chunk_list_push(arena, &g_d2r_shared.src_files, D2R_SRC_FILE_CAP);

        // make file path
        String8 path;
        {
          String8List path_list = {0};
          str8_list_push_node(&path_list, &(String8Node){ .string = lookup->vm->header.dir_table.v[src->dir_idx] });
          str8_list_push_node(&path_list, &(String8Node){ .string = src->path });
          path = str8_path_list_join_by_style(arena, &path_list, path_style);
        }

        // fill out source file
        dst->path = path;
        if ( ! u128_match(src->md5, u128_zero())) {
          dst->checksum_kind = RDI_ChecksumKind_MD5;
          dst->checksum      = str8_copy(arena, str8_struct(&src->md5));
        } else if (src->time_stamp != 0) {
          dst->checksum_kind = RDI_ChecksumKind_Timestamp;
          dst->checksum      = str8_copy(arena, str8_struct(&src->time_stamp));
        }

        lookup->src_file = dst;
      }
      ProfEnd();
      
      // TODO: sync
      RDIM_LineTableChunkList *lane_line_table_chunks = push_array(temp.arena, RDIM_LineTableChunkList, lane_count());

      // TODO: per thread task
      ProfBegin("Convert line sequences");
      U64 *line_seq_counts_per_lane = push_array(scratch.arena, U64, lane_count());
      for EachIndex(cu_idx, cu_ranges.count) {
        RDIM_LineTableChunkList *lane_line_table_chunk_list = &lane_line_table_chunks[lane_idx()];

        // push new line table for the compile unit
        RDIM_LineTable *line_table = rdim_line_table_chunk_list_push(arena, lane_line_table_chunk_list, D2R_LINE_TABLE_CAP);
        cu_line_tables_rdi[cu_idx] = line_table;

        // push line buffer
#define D2R_LineBufferMax 1024
        struct LineBuffer { U64 file_index; U64 voffs[D2R_LineBufferMax]; U32 line_nums[D2R_LineBufferMax]; U32 col_nums[D2R_LineBufferMax]; } LineBuffer;
        struct LineBuffer *line_buffer      = push_array(temp.arena, struct LineBuffer, 1);
        U64                line_buffer_size = 0;

        // decode line sequences
        DW_LineVM *vm                       = line_vms[cu_idx];
        U64       *line_seq_counts_per_file = push_array(temp.arena, U64, vm->header.file_table.count);
        while (dw_line_vm_step(vm)) {
          if (vm->new_line) {
            // lazy defined files are not supported
            if (vm->state.file_index >= vm->header.file_table.count) { continue; }

            // time to flush the buffer?
            if (((vm->opcode == DW_StdOpcode_ExtendedOpcode && vm->ext_opcode == DW_ExtOpcode_EndSequence) ||
                (line_buffer_size >= D2R_LineBufferMax || line_buffer->file_index != vm->state.file_index))
                && line_buffer_size > 0) {
              // lookup source file
              DW_LineFile       *file     = &vm->header.file_table.v[line_buffer->file_index];
              U64                hash     = d2r_hash_line_file(vm->header.dir_table.v[file->dir_idx], file);
              D2R_SrcFileLookup *lookup   = lfht_search(src_file_lfht, hash, &(D2R_SrcFileLookup){ .file = file, .vm = vm }, d2r_src_file_lookup_is_equal, 0);
              RDIM_SrcFile      *src_file = lookup->src_file;

              // copy line info
              U64 *voffs     = push_array_no_zero(arena, U64, line_buffer_size + 1);
              U32 *line_nums = push_array_no_zero(arena, U32, line_buffer_size);
              voffs[line_buffer_size] = vm->state.address - image_base;
              MemoryCopyTyped(voffs, line_buffer->voffs, line_buffer_size);
              MemoryCopyTyped(line_nums, line_buffer->line_nums, line_buffer_size);

              RDIM_LineSequence *line_seq = rdim_line_table_push_sequence(arena, lane_line_table_chunk_list, line_table, src_file, voffs, line_nums, 0, line_buffer_size);

              // atomic implementation of @rdim_src_file_push_line_sequence
              {
                // associate line fragment with source file
                RDIM_SrcFileLineMapFragment *line_frag = push_array(arena, RDIM_SrcFileLineMapFragment, 1);
                line_frag->seq = line_seq;

                // insert line fragment node
                for (;;) {
                  RDIM_SrcFileLineMapFragment *next = src_file->first_line_map_fragment;
                  line_frag->next = next;
                  RDIM_SrcFileLineMapFragment *curr = ins_atomic_ptr_eval_cond_assign(&src_file->first_line_map_fragment, line_frag, next);
                  if (curr == 0) {
                    ins_atomic_u64_add_eval(&g_d2r_shared.src_files.source_line_map_count, 1);
                  }
                  if (curr == next) { break; }
                }

                // accumulate line sequence counts per file
                line_seq_counts_per_file[vm->state.file_index] += line_seq->line_count;

                // accumulate line sequence counts per lane
                line_seq_counts_per_lane[lane_idx()] += line_seq->line_count;
              }

              // reset line buffer size tracker
              line_buffer_size = 0;
            }

            if (line_buffer_size > 0 && line_buffer->voffs[line_buffer_size - 1] == vm->state.address) {
              line_buffer->line_nums[line_buffer_size - 1] = Min(line_buffer->line_nums[line_buffer_size - 1], safe_cast_u32(vm->state.line));
            } else {
              // append line to the buffer
              line_buffer->file_index                  = vm->state.file_index;
              line_buffer->voffs[line_buffer_size]     = vm->state.address - image_base;
              line_buffer->line_nums[line_buffer_size] = safe_cast_u32(vm->state.line);
              line_buffer_size += 1;
            }
          }
        }

        // update line sequence counts per source file @rdim_src_file_push_line_sequence
        for EachIndex(file_idx, vm->header.file_table.count) {
          if (line_seq_counts_per_file[file_idx] > 0) {
            DW_LineFile       *file     = &vm->header.file_table.v[file_idx];
            U64                hash     = d2r_hash_line_file(vm->header.dir_table.v[file->dir_idx], file);
            D2R_SrcFileLookup *lookup   = lfht_search(src_file_lfht, hash, &(D2R_SrcFileLookup){ .file = file, .vm = vm }, d2r_src_file_lookup_is_equal, 0);
            RDIM_SrcFile      *src_file = lookup->src_file;
            ins_atomic_u64_add_eval(&src_file->total_line_count, line_seq_counts_per_file[file_idx]);
          }
        }
      }

      // TODO: sync
      //
      // update total line sequences count in shared @rdim_src_file_push_line_sequence
      for EachIndex(i, lane_count()) {
        g_d2r_shared.src_files.total_line_count += line_seq_counts_per_lane[i];
      }

      // TODO: sync
      for EachIndex(i, lane_count()) {
        rdim_line_table_chunk_list_concat_in_place(&g_d2r_shared.line_tables, &lane_line_table_chunks[i]);
      }

      // TODO: per thread task
      ProfBegin("Relase line VMs");
      for EachIndex(i, cu_ranges.count) {
        dw_line_vm_release(line_vms[i]);
      }
      ProfEnd();

      temp_end(temp);
    }
    ProfEnd();
    
    ////////////////////////////////
    
    RDIM_Scope *global_scope = rdim_scope_chunk_list_push(arena, &g_d2r_shared.scopes, D2R_SCOPE_CHUNK_CAP);
    
    //////////////////////////////// 
    
    RDIM_Type *builtin_types[RDI_TypeKind_Count] = {0};
    for (RDI_TypeKind type_kind = RDI_TypeKind_FirstBuiltIn; type_kind <= RDI_TypeKind_LastBuiltIn; type_kind += 1) {
      RDIM_Type *type = rdim_type_chunk_list_push(arena, &g_d2r_shared.types, D2R_TYPE_CHUNK_CAP);
      type->kind      = type_kind;
      type->name.str  = rdi_string_from_type_kind(type_kind, &type->name.size);
      type->byte_size = rdi_size_from_basic_type_kind(type_kind);
      builtin_types[type_kind] = type;
    }

    // fixup float80 size
    if (arch == Arch_x64 || arch == Arch_arm64) {
      builtin_types[RDI_TypeKind_F80]->byte_size = 16;
    }
    
    builtin_types[RDI_TypeKind_Void]->byte_size   = rdi_addr_size_from_arch(g_d2r_shared.top_level_info.arch);
    builtin_types[RDI_TypeKind_Handle]->byte_size = rdi_addr_size_from_arch(g_d2r_shared.top_level_info.arch);
    
    builtin_types[RDI_TypeKind_Variadic] = rdim_type_chunk_list_push(arena, &g_d2r_shared.types, D2R_TYPE_CHUNK_CAP);
    builtin_types[RDI_TypeKind_Variadic]->kind = RDI_TypeKind_Variadic;
    
    //////////////////////////////// 
    
    ProfBegin("Convert Units");
    for EachIndex(cu_idx, cu_ranges.count) {
      Temp comp_temp = temp_begin(scratch.arena);
      
      DW_CompUnit *cu = &cu_arr[cu_idx];

      // skip DWO
      {
        if (cu->dwo_id) { goto next_cu; }
        
        String8 dwo_name = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_DwoName);
        if (dwo_name.size) { goto next_cu; }
        
        String8 gnu_dwo_name = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_GNU_DwoName);
        if (gnu_dwo_name.size) { goto next_cu; }
      }
      
      // parse and build tag tree
      DW_TagTree tag_tree = dw_tag_tree_from_cu(comp_temp.arena, &input, cu);
      
      // build (info offset -> tag) hash table to resolve tags with abstract origin
      cu->tag_ht = dw_make_tag_hash_table(comp_temp.arena, tag_tree);
      
      // extract compile unit info
      String8     cu_name = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_Name);
      String8     cu_dir  = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_CompDir);
      String8     cu_prod = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_Producer);
      DW_Language cu_lang = dw_const_u64_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_Language);
      
      // init type table
      D2R_TypeTable *type_table   = push_array(comp_temp.arena, D2R_TypeTable, 1);
      type_table->ht              = hash_table_init(comp_temp.arena, 0x4000);
      type_table->types           = &g_d2r_shared.types;
      type_table->type_chunk_cap  = D2R_TYPE_CHUNK_CAP;
      type_table->builtin_types   = builtin_types;
      
      // convert debug info
      d2r_convert_types(arena, type_table, &input, cu, cu_lang, arch, tag_tree.root);
      d2r_convert_udts(arena, type_table, &input, cu, cu_lang, tag_tree.root);
      d2r_convert_symbols(arena, type_table, global_scope, &input, cu, cu_lang, image_base, arch, tag_tree.root);
      
      RDIM_Rng1U64ChunkList cu_voff_ranges = {0};
      if (cu_idx < cu_contrib_map.count) {
        cu_voff_ranges = d2r_voff_ranges_from_cu_info_off(cu_contrib_map, cu_ranges.v[cu_idx].min);
      } else {
        Rng1U64List range_list  = d2r_range_list_from_tag(scratch.arena, &input, cu, image_base, cu->tag);
        for EachNode(n, Rng1U64Node, range_list.first) {
          rdim_rng1u64_chunk_list_push(arena, &cu_voff_ranges, 512, (RDIM_Rng1U64){ .min = n->v.min, .max = n->v.max });
        }
      }
      
      // convert compile unit
      {
        RDIM_Unit *unit     = rdim_unit_chunk_list_push(arena, &g_d2r_shared.units, D2R_UNIT_CHUNK_CAP);
        unit->unit_name     = cu_name;
        unit->compiler_name = cu_prod;
        unit->source_file   = str8_zero(); // TODO
        unit->object_file   = str8_zero(); // TODO
        unit->archive_file  = str8_zero(); // TODO
        unit->build_path    = cu_dir;
        unit->language      = d2r_rdi_language_from_dw_language(cu_lang);
        unit->line_table    = cu_line_tables_rdi[cu_idx];
        unit->voff_ranges   = cu_voff_ranges;
      }
      
      next_cu:;
      temp_end(comp_temp);
    }
    ProfEnd();
  }
  
  lane_sync();
  
  RDIM_BakeParams bake_params  = {0};
  bake_params.subset_flags     = params->subset_flags;
  bake_params.top_level_info   = g_d2r_shared.top_level_info;
  bake_params.binary_sections  = g_d2r_shared.binary_sections;
  bake_params.units            = g_d2r_shared.units;
  bake_params.types            = g_d2r_shared.types;
  bake_params.udts             = g_d2r_shared.udts;
  bake_params.src_files        = g_d2r_shared.src_files;
  bake_params.line_tables      = g_d2r_shared.line_tables;
  bake_params.locations        = g_d2r_shared.locations;
  bake_params.global_variables = g_d2r_shared.gvars;
  bake_params.thread_variables = g_d2r_shared.tvars;
  bake_params.procedures       = g_d2r_shared.procs;
  bake_params.scopes           = g_d2r_shared.scopes;
  bake_params.inline_sites     = g_d2r_shared.inline_sites;
  
  scratch_end(scratch);
  return bake_params;
}

