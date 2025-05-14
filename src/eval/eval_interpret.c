// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal void
e_select_interpret_ctx(E_InterpretCtx *ctx, RDI_Parsed *primary_rdi, U64 ip_voff)
{
  e_interpret_ctx = ctx;
  
  // compute and apply frame base
  if(primary_rdi != 0)
  {
    E_Interpretation frame_base = { .code = ~0 };
    
    RDI_Procedure *proc = rdi_procedure_from_voff(primary_rdi, ip_voff);
    for(U64 loc_block_idx = proc->frame_base_location_first; loc_block_idx < proc->frame_base_location_opl; loc_block_idx += 1)
    {
      RDI_LocationBlock *block = rdi_element_from_name_idx(primary_rdi, LocationBlocks, loc_block_idx);
      if (block->scope_off_first <= ip_voff && ip_voff < block->scope_off_opl) {
        U64  all_location_data_size = 0;
        U8  *all_location_data      = rdi_table_from_name(primary_rdi, LocationData, &all_location_data_size);
        if(block->location_data_off + sizeof(RDI_LocationKind) <= all_location_data_size)
        {
          RDI_LocationKind loc_kind = *(RDI_LocationKind *)(all_location_data + block->location_data_off);
          if(loc_kind == RDI_LocationKind_ValBytecodeStream || loc_kind == RDI_LocationKind_AddrBytecodeStream)
          {
            U8      *bytecode_ptr  = all_location_data + block->location_data_off + sizeof(RDI_LocationKind);
            U8      *bytecode_opl  = all_location_data + all_location_data_size;
            U64      bytecode_size = rdi_size_from_bytecode_stream(bytecode_ptr, bytecode_opl);
            String8  bytecode      = str8(bytecode_ptr, bytecode_size);
            frame_base = e_interpret(bytecode);
          }
          else if(loc_kind != RDI_LocationKind_NULL)
          {
            NotImplemented;
          }
        }
        break;
      }
    }
    
    if(frame_base.code == E_InterpretationCode_Good)
    {
      *ctx->frame_base = frame_base.value.u64;
    }
    else
    {
      ctx->frame_base = 0;
    }
  }
}

////////////////////////////////
//~ rjf: Space Reading Helpers

internal U64
e_space_gen(E_Space space)
{
  U64 result = 0;
  if(e_base_ctx->space_gen != 0)
  {
    result = e_base_ctx->space_gen(e_base_ctx->space_rw_user_data, space);
  }
  return result;
}

internal B32
e_space_read(E_Space space, void *out, Rng1U64 range)
{
  ProfBeginFunction();
  B32 result = 0;
  if(e_interpret_ctx->space_read != 0)
  {
    result = e_interpret_ctx->space_read(e_interpret_ctx->space_rw_user_data, space, out, range);
  }
  ProfEnd();
  return result;
}

internal B32
e_space_write(E_Space space, void *in, Rng1U64 range)
{
  ProfBeginFunction();
  B32 result = 0;
  if(e_interpret_ctx->space_write != 0)
  {
    result = e_interpret_ctx->space_write(e_interpret_ctx->space_rw_user_data, space, in, range);
  }
  ProfEnd();
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
  E_Space selected_space = {0};
  if(bytecode.size != 0)
  {
    selected_space = e_interpret_ctx->primary_space;
  }
  
  //- rjf: iterate bytecode & perform ops
  U8 *ptr = bytecode.str;
  U8 *opl = bytecode.str + bytecode.size;
  for(;ptr < opl;)
  {
    // rjf: consume next opcode
    RDI_EvalOp op = (RDI_EvalOp)*ptr;
    U16 ctrlbits = 0;
    if(op < RDI_EvalOp_COUNT)
    {
      ctrlbits = rdi_eval_op_ctrlbits_table[op];
    }
    else switch(op)
    {
      case E_IRExtKind_SetSpace:     {ctrlbits = RDI_EVAL_CTRLBITS(32, 0, 0);}break;
      default:
      {
        result.code = E_InterpretationCode_BadOp;
        goto done;
      }break;
    }
    ptr += 1;
    
    // rjf: decode
    E_Value imm = {0};
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
      MemoryCopy(&imm, ptr, decode_size);
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
        MemoryCopy(&selected_space, &imm, sizeof(selected_space));
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
        ptr += imm.u64;
      }break;
      
      case RDI_EvalOp_Skip:
      {
        ptr += imm.u64;
      }break;
      
      case RDI_EvalOp_MemRead:
      {
        U64 addr = svals[0].u64;
        U64 size = imm.u64;
        B32 good_read = e_space_read(selected_space, &nval, r1u64(addr, addr+size));
        if(!good_read)
        {
          result.code = E_InterpretationCode_BadMemRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegRead:
      {
        U8 rdi_reg_code     = (imm.u64&0x0000FF)>>0;
        U8 byte_size        = (imm.u64&0x00FF00)>>8;
        U8 byte_off         = (imm.u64&0xFF0000)>>16;
        REGS_RegCode base_reg_code = regs_reg_code_from_arch_rdi_code(e_interpret_ctx->reg_arch, rdi_reg_code);
        REGS_Rng rng = regs_reg_code_rng_table_from_arch(e_interpret_ctx->reg_arch)[base_reg_code];
        U64 off = (U64)rng.byte_off + byte_off;
        U64 size = (U64)byte_size;
        B32 good_read = e_space_read(e_interpret_ctx->reg_space, &nval, r1u64(off, off+size));
        if(!good_read)
        {
          result.code = E_InterpretationCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_RegReadDyn:
      {
        U64 off  = svals[0].u64;
        U64 size = bit_size_from_arch(e_interpret_ctx->reg_arch)/8;
        B32 good_read = e_space_read(e_interpret_ctx->reg_space, &nval, r1u64(off, off+size));
        if(!good_read)
        {
          result.code = E_InterpretationCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_FrameOff:
      {
        if(e_interpret_ctx->frame_base != 0)
        {
          nval.u64 = *e_interpret_ctx->frame_base + imm.u64;
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
          nval.u64 = *e_interpret_ctx->module_base + imm.u64;
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
          nval.u64 = *e_interpret_ctx->tls_base + imm.u64;
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
      case RDI_EvalOp_ConstU128:
      {
        nval = imm;
      }break;
      
      case RDI_EvalOp_ConstString:
      {
        MemoryCopy(&nval, ptr, imm.u64);
        ptr += imm.u64;
      }break;
      
      case RDI_EvalOp_Abs:
      {
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32;
          if(svals[0].f32 < 0)
          {
            nval.f32 = -svals[0].f32;
          }
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = -svals[0].f32;
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 + svals[1].f32;
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32 - svals[1].f32;
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.f32 = svals[0].f32*svals[1].f32;
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
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
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
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
        else if(imm.u64 == RDI_EvalTypeGroup_U ||
                imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U)
        {
          nval.u64 = svals[0].u64 >> svals[1].u64;
        }
        else if(imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_U ||
           imm.u64 == RDI_EvalTypeGroup_S)
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
        B32 result = MemoryMatchArray(svals[0].u512.u64, svals[1].u512.u64);
        nval.u64 = !!result;
      }break;
      
      case RDI_EvalOp_NtEq:
      {
        B32 result = MemoryMatchArray(svals[0].u512.u64, svals[1].u512.u64);
        nval.u64 = !result;
      }break;
      
      case RDI_EvalOp_LsEq:
      {
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 <= svals[1].f32);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 <= svals[1].f64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 <= svals[1].u64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 >= svals[1].f32);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 >= svals[1].f64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 >= svals[1].u64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 < svals[1].f32);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 < svals[1].f64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 < svals[1].u64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_S)
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
        if(imm.u64 == RDI_EvalTypeGroup_F32)
        {
          nval.u64 = (svals[0].f32 > svals[1].f32);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_F64)
        {
          nval.u64 = (svals[0].f64 > svals[1].f64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_U)
        {
          nval.u64 = (svals[0].u64 > svals[1].u64);
        }
        else if(imm.u64 == RDI_EvalTypeGroup_S)
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
        if(0 < imm.u64)
        {
          U64 mask = 0;
          if(imm.u64 < 64)
          {
            mask = max_U64 >> (64 - imm.u64);
          }
          nval.u64 = svals[0].u64&mask;
        }
      }break;
      
      case RDI_EvalOp_TruncSigned:
      {
        if(0 < imm.u64)
        {
          U64 mask = 0;
          if(imm.u64 < 64)
          {
            mask = max_U64 >> (64 - imm.u64);
          }
          U64 high = 0;
          if(svals[0].u64 & (1 << (imm.u64 - 1)))
          {
            high = ~mask;
          }
          nval.u64 = high|(svals[0].u64&mask);
        }
      }break;
      
      case RDI_EvalOp_Convert:
      {
        U32 in = imm.u64&0xFF;
        U32 out = (imm.u64 >> 8)&0xFF;
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
        if(stack_count > imm.u64)
        {
          nval = stack[stack_count - imm.u64 - 1];
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
        if(stack_count > imm.u64)
        {
          if(imm.u64 > 0)
          {
            E_Value tval = stack[stack_count - 1];
            E_Value *dst = stack + stack_count - 1 - imm.u64;
            E_Value *shift = dst + 1;
            MemoryCopy(shift, dst, imm.u64*sizeof(E_Value));
            *dst = tval;
          }
        }
        else
        {
          result.code = E_InterpretationCode_BadOp;
          goto done;
        }
      }break;
      
      case RDI_EvalOp_ValueRead:
      {
        U64 bytes_to_read = imm.u64;
        U64 offset = svals[0].u64;
        if(offset + bytes_to_read <= sizeof(E_Value))
        {
          E_Value src_val = svals[1];
          MemoryCopy(&nval.u512.u64[0], (U8 *)(&src_val.u512.u64[0]) + offset, bytes_to_read);
        }
      }break;
      
      case RDI_EvalOp_ByteSwap:
      {
        U64 byte_size = imm.u64;
        switch(byte_size)
        {
          default:
          {
            result.code = E_InterpretationCode_BadOp;
            goto done;
          }break;
          case 2:{nval.u16 = bswap_u16(svals[0].u16);}break;
          case 4:{nval.u32 = bswap_u32(svals[0].u32);}break;
          case 8:{nval.u64 = bswap_u64(svals[0].u64);}break;
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
  result.space = selected_space;
  scratch_end(scratch);
  return result;
}
