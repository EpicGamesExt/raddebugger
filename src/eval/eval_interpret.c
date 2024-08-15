// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_InterpretCtx *
e_selected_interpret_ctx(void)
{
  return e_interpret_ctx;
}

internal void
e_select_interpret_ctx(E_InterpretCtx *ctx)
{
  e_interpret_ctx = ctx;
}

////////////////////////////////
//~ rjf: Space Reading Helpers

internal B32
e_space_read(E_Space space, void *out, Rng1U64 range)
{
  B32 result = 0;
  switch(space)
  {
    case E_Space_FIXED_COUNT:
    case E_Space_Null:{}break;
    case E_Space_Regs:
    {
      Rng1U64 legal_range = r1u64(0, e_interpret_ctx->reg_size);
      Rng1U64 read_range = intersect_1u64(legal_range, range);
      U64 read_size = dim_1u64(read_range);
      MemoryCopy(out, (U8 *)e_interpret_ctx->reg_data + read_range.min, read_size);
      result = (read_size == dim_1u64(range));
    }break;
    default:
    if(e_interpret_ctx->space_read != 0)
    {
      result = e_interpret_ctx->space_read(e_interpret_ctx->space_read_user_data, space, out, range);
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Interpretation Functions

internal E_Interpretation
e_interpret(String8 bytecode)
{
  E_Interpretation result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: allocate stack & "registers"
  U64 stack_cap = 128; // TODO(rjf): scan bytecode; determine maximum stack depth
  E_Value *stack = push_array_no_zero(scratch.arena, E_Value, stack_cap);
  U64 stack_count = 0;
  E_Space selected_space = e_interpret_ctx->primary_space;
  
  //- rjf: iterate bytecode & perform ops
  U8 *ptr = bytecode.str;
  U8 *opl = bytecode.str + bytecode.size;
  for(;ptr < opl;)
  {
    // rjf: consume next opcode
    RDI_EvalOp op = (RDI_EvalOp)*ptr;
    U8 ctrlbits = 0;
    if(op < RDI_EvalOp_COUNT)
    {
      ctrlbits = rdi_eval_op_ctrlbits_table[op];
    }
    else switch(op)
    {
      case E_IRExtKind_SetSpace:{ctrlbits = RDI_EVAL_CTRLBITS(8, 0, 0);}break;
      default:
      {
        result.code = E_InterpretationCode_BadOp;
        goto done;
      }break;
    }
    ptr += 1;
    
    // rjf: decode
    U64 imm = 0;
    {
      U32 decode_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
      U8 *next_ptr = ptr + decode_size;
      if(next_ptr > opl)
      {
        result.code = E_InterpretationCode_BadOp;
        goto done;
      }
      // TODO(rjf): guarantee 8 bytes padding after the end of serialized
      // bytecode; read 8 bytes and mask
      switch(decode_size)
      {
        case 1:{imm = *ptr;}break;
        case 2:{imm = *(U16*)ptr;}break;
        case 4:{imm = *(U32*)ptr;}break;
        case 8:{imm = *(U64*)ptr;}break;
      }
      ptr = next_ptr;
    }
    
    // rjf: pop
    E_Value *svals = 0;
    {
      U32 pop_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
      if(pop_count > stack_count)
      {
        result.code = E_InterpretationCode_BadOp;
        goto done;
      }
      if(pop_count <= stack_count)
      {
        stack_count -= pop_count;
        svals = stack + stack_count;
      }
    }
    
    // rjf: interpret op, given decodes/pops
    E_Value nval = {0};
    switch(op)
    {
      case E_IRExtKind_SetSpace:
      {
        selected_space = imm;
      }break;
      
      case RDI_EvalOp_Stop:
      {
        goto done;
      }break;
      
      case RDI_EvalOp_Noop:
      {
        // do nothing
      }break;
      
      case RDI_EvalOp_Cond:
      if(svals[0].u64)
      {
        ptr += imm;
      }break;
      
      case RDI_EvalOp_Skip:
      {
        ptr += imm;
      }break;
      
      case RDI_EvalOp_MemRead:
      {
        U64 addr = svals[0].u64;
        U64 size = imm;
        B32 good_read = e_space_read(selected_space, &nval, r1u64(addr, addr+size));
        if(!good_read)
        {
          result.code = E_InterpretationCode_BadMemRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegRead:
      {
        U8 rdi_reg_code     = (imm&0x0000FF)>>0;
        U8 byte_size        = (imm&0x00FF00)>>8;
        U8 byte_off         = (imm&0xFF0000)>>16;
        REGS_RegCode base_reg_code = regs_reg_code_from_arch_rdi_code(e_interpret_ctx->arch, rdi_reg_code);
        REGS_Rng rng = regs_reg_code_rng_table_from_architecture(e_interpret_ctx->arch)[base_reg_code];
        U64 off = (U64)rng.byte_off + byte_off;
        U64 size = (U64)byte_size;
        if(off + size <= e_interpret_ctx->reg_size)
        {
          MemoryCopy(&nval, (U8*)e_interpret_ctx->reg_data + off, size);
        }
        else
        {
          result.code = E_InterpretationCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegReadDyn:
      {
        U64 off  = svals[0].u64;
        U64 size = bit_size_from_arch(e_interpret_ctx->arch)/8;
        if(off + size <= e_interpret_ctx->reg_size)
        {
          MemoryCopy(&nval, (U8*)e_interpret_ctx->reg_data + off, size);
        }
        else
        {
          result.code = E_InterpretationCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_FrameOff:
      {
        if(e_interpret_ctx->frame_base != 0)
        {
          nval.u64 = *e_interpret_ctx->frame_base + imm;
        }
        else
        {
          result.code = E_InterpretationCode_BadFrameBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_ModuleOff:
      {
        if(e_interpret_ctx->module_base != 0)
        {
          nval.u64 = *e_interpret_ctx->module_base + imm;
        }
        else
        {
          result.code = E_InterpretationCode_BadModuleBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_TLSOff:
      {
        if(e_interpret_ctx->tls_base != 0)
        {
          nval.u64 = *e_interpret_ctx->tls_base + imm;
        }
        else
        {
          result.code = E_InterpretationCode_BadTLSBase;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_ConstU8:
      case RDI_EvalOp_ConstU16:
      case RDI_EvalOp_ConstU32:
      case RDI_EvalOp_ConstU64:
      {
        nval.u64 = imm;
      }break;
      
      case RDI_EvalOp_ConstString:
      {
        MemoryCopy(&nval, ptr, imm);
        ptr += imm;
      }break;
      
      case RDI_EvalOp_Abs:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32;
          if(svals[0].f32 < 0)
          {
            nval.f32 = -svals[0].f32;
          }
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64;
          if(svals[0].f64 < 0)
          {
            nval.f64 = -svals[0].f64;
          }
        }
        else
        {
          nval.s64 = svals[0].s64;
          if(svals[0].s64 < 0)
          {
            nval.s64 = -svals[0].s64;
          }
        }
      }break;
      
      case RDI_EvalOp_Neg:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = -svals[0].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = -svals[0].f64;
        }
        else
        {
          nval.u64 = (~svals[0].u64) + 1;
        }
      }break;
      
      case RDI_EvalOp_Add:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 + svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64 + svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64 + svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Sub:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 - svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64 - svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64 - svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Mul:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32*svals[1].f32;
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.f64 = svals[0].f64*svals[1].f64;
        }
        else
        {
          nval.u64 = svals[0].u64*svals[1].u64;
        }
      }break;
      
      case RDI_EvalOp_Div:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          if(svals[1].f32 != 0.f)
          {
            nval.f32 = svals[0].f32/svals[1].f32;
          }
          else
          {
            result.code = E_InterpretationCode_DivideByZero;
            goto done;
          }
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          if(svals[1].f64 != 0.)
          {
            nval.f64 = svals[0].f64/svals[1].f64;
          }
          else
          {
            result.code = E_InterpretationCode_DivideByZero;
            goto done;
          }
        }
        else if(imm == RDI_EvalTypeGroup_U ||
                imm == RDI_EvalTypeGroup_S)
        {
          if(svals[1].u64 != 0)
          {
            nval.u64 = svals[0].u64/svals[1].u64;
          }
          else
          {
            result.code = E_InterpretationCode_DivideByZero;
            goto done;
          }
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Mod:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          if(svals[1].u64 != 0)
          {
            nval.u64 = svals[0].u64%svals[1].u64;
          }
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LShift:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64 << svals[1].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RShift:
      {
        if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = svals[0].u64 >> svals[1].u64;
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].s64 >> svals[1].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitAnd:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64&svals[1].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitOr:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64|svals[1].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitXor:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = svals[0].u64^svals[1].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_BitNot:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = ~svals[0].u64;
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogAnd:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].u64 && svals[1].u64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogOr:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].u64 || svals[1].u64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_LogNot:
      {
        if(imm == RDI_EvalTypeGroup_U ||
           imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (!svals[0].u64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_EqEq:
      {
        B32 result = MemoryMatchArray(svals[0].u512, svals[1].u512);
        nval.u64 = !!result;
      }break;
      
      case RDI_EvalOp_NtEq:
      {
        B32 result = MemoryMatchArray(svals[0].u512, svals[1].u512);
        nval.u64 = !result;
      }break;
      
      case RDI_EvalOp_LsEq:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 <= svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 <= svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 <= svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 <= svals[1].s64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_GrEq:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 >= svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 >= svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 >= svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 >= svals[1].s64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Less:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 < svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 < svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 < svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 < svals[1].s64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Grtr:
      {
        if(imm == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 > svals[1].f32);
        }
        else if(imm == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 > svals[1].f64);
        }
        else if(imm == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 > svals[1].u64);
        }
        else if(imm == RDI_EvalTypeGroup_S)
        {
          nval.u64 = (svals[0].s64 > svals[1].s64);
        }
        else
        {
          result.code = E_InterpretationCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Trunc:
      {
        if(0 < imm)
        {
          U64 mask = 0;
          if(imm < 64)
          {
            mask = max_U64 >> (64 - imm);
          }
          nval.u64 = svals[0].u64&mask;
        }
      }break;
      
      case RDI_EvalOp_TruncSigned:
      {
        if(0 < imm)
        {
          U64 mask = 0;
          if(imm < 64)
          {
            mask = max_U64 >> (64 - imm);
          }
          U64 high = 0;
          if(svals[0].u64 & (1 << (imm - 1)))
          {
            high = ~mask;
          }
          nval.u64 = high|(svals[0].u64&mask);
        }
      }break;
      
      case RDI_EvalOp_Convert:
      {
        U32 in = imm&0xFF;
        U32 out = (imm >> 8)&0xFF;
        if(in != out)
        {
          switch(in + out*RDI_EvalTypeGroup_COUNT)
          {
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f32;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_U*RDI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f32;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_S*RDI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_U + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].u64;
            }break;
            case RDI_EvalTypeGroup_S + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].s64;
            }break;
            case RDI_EvalTypeGroup_F64 + RDI_EvalTypeGroup_F32*RDI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].f64;
            }break;
            
            case RDI_EvalTypeGroup_U + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].u64;
            }break;
            case RDI_EvalTypeGroup_S + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].s64;
            }break;
            case RDI_EvalTypeGroup_F32 + RDI_EvalTypeGroup_F64*RDI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].f32;
            }break;
          }
        }
      }break;
      
      case RDI_EvalOp_Pick:
      {
        if(stack_count > imm)
        {
          nval = stack[stack_count - imm - 1];
        }
        else
        {
          result.code = E_InterpretationCode_BadOp;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_Pop:
      {
        // do nothing - the pop is handled by the control bits
      }break;
      
      case RDI_EvalOp_Insert:
      {
        if(stack_count > imm)
        {
          if(imm > 0)
          {
            E_Value tval = stack[stack_count - 1];
            E_Value *dst = stack + stack_count - 1 - imm;
            E_Value *shift = dst + 1;
            MemoryCopy(shift, dst, imm*sizeof(E_Value));
            *dst = tval;
          }
        }
        else
        {
          result.code = E_InterpretationCode_BadOp;
          goto done;
        }
      }break;
    }
    
    // rjf: push
    {
      U64 push_count = RDI_PUSHN_FROM_CTRLBITS(ctrlbits);
      if(push_count == 1)
      {
        if(stack_count < stack_cap)
        {
          stack[stack_count] = nval;
          stack_count += 1;
        }
        else
        {
          result.code = E_InterpretationCode_InsufficientStackSpace;
          goto done;
        }
      }
    }
  }
  done:;
  
  if(stack_count >= 1)
  {
    result.value = stack[0];
  }
  scratch_end(scratch);
  return result;
}
