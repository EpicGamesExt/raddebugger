// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ allen: Eval Machine Functions

internal EVAL_Result
eval_interpret(EVAL_Machine *machine, String8 bytecode)
{
  ProfBeginFunction();
  EVAL_Result result = {0};
  
  // TODO(allen): We could scan the bytecode and figure out the
  // maximum depth of the stack
  Temp scratch = scratch_begin(0, 0);
  U64 stack_cap = 128;
  EVAL_Slot *stack = push_array_no_zero(scratch.arena, EVAL_Slot, stack_cap);
  
  U64 stack_count = 0;
  U8 *ptr = bytecode.str;
  U8 *opl = bytecode.str + bytecode.size;
  
  for (;ptr < opl;){
    // consume opcode
    RADDBGI_EvalOp op = (RADDBGI_EvalOp)*ptr;
    if (op >= RADDBGI_EvalOp_COUNT){
      result.code = EVAL_ResultCode_BadOp;
      goto done;
    }
    U8 ctrlbits = raddbgi_eval_opcode_ctrlbits[op];
    ptr += 1;
    
    // decode
    U64 imm = 0;
    {
      U32 decode_size = RADDBGI_DECODEN_FROM_CTRLBITS(ctrlbits);
      U8 *next_ptr = ptr + decode_size;
      if (next_ptr > opl){
        result.code = EVAL_ResultCode_BadOp;
        goto done;
      }
      // TODO(allen): to improve this:
      //  gaurantee 8 bytes padding after the end of serialized bytecode
      //  read 8 bytes and mask
      switch (decode_size){
        case 1: imm = *ptr; break;
        case 2: imm = *(U16*)ptr; break;
        case 4: imm = *(U32*)ptr; break;
        case 8: imm = *(U64*)ptr; break;
      }
      ptr = next_ptr;
    }
    
    // pop
    EVAL_Slot *svals = 0;
    {
      U32 pop_count = RADDBGI_POPN_FROM_CTRLBITS(ctrlbits);
      if (pop_count > stack_count){
        result.code = EVAL_ResultCode_BadOp;
        goto done;
      }
      if (pop_count <= stack_count){
        stack_count -= pop_count;
        svals = stack + stack_count;
      }
    }
    
    // interpret
    EVAL_Slot nval = {0};
    switch (op){
      case RADDBGI_EvalOp_Stop:
      {
        goto done;
      }break;
      
      case RADDBGI_EvalOp_Noop:
      {
        // do nothing
      }break;
      
      case RADDBGI_EvalOp_Cond:
      {
        if (svals[0].u64){
          ptr += imm;
        }
      }break;
      
      case RADDBGI_EvalOp_Skip:
      {
        ptr += imm;
      }break;
      
      case RADDBGI_EvalOp_MemRead:
      {
        U64 addr = svals[0].u64;
        U64 size = imm;
        B32 good_read = 0;
        if (machine->memory_read != 0 &&
            machine->memory_read(machine->u, &nval, addr, size)){
          good_read = 1;
        }
        if (!good_read){
          result.code = EVAL_ResultCode_BadMemRead;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_RegRead:
      {
        U8 raddbgi_reg_code = (imm&0x0000FF)>>0;
        U8 byte_size        = (imm&0x00FF00)>>8;
        U8 byte_off         = (imm&0xFF0000)>>16;
        REGS_RegCode base_reg_code = regs_reg_code_from_arch_raddbgi_code(machine->arch, raddbgi_reg_code);
        REGS_Rng rng = regs_reg_code_rng_table_from_architecture(machine->arch)[base_reg_code];
        U64 off = (U64)rng.byte_off + byte_off;
        U64 size = (U64)byte_size;
        if (off + size <= machine->reg_size){
          MemoryCopy(&nval, (U8*)machine->reg_data + off, size);
        }
        else{
          result.code = EVAL_ResultCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_RegReadDyn:
      {
        U64 off  = svals[0].u64;
        U64 size = bit_size_from_arch(machine->arch)/8;
        if (off + size <= machine->reg_size){
          MemoryCopy(&nval, (U8*)machine->reg_data + off, size);
        }
        else{
          result.code = EVAL_ResultCode_BadRegRead;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_FrameOff:
      {
        if (machine->frame_base != 0){
          nval.u64 = *machine->frame_base + imm;
        }
        else{
          result.code = EVAL_ResultCode_BadFrameBase;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_ModuleOff:
      {
        if (machine->module_base != 0){
          nval.u64 = *machine->module_base + imm;
        }
        else{
          result.code = EVAL_ResultCode_BadModuleBase;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_TLSOff:
      {
        if (machine->tls_base != 0){
          nval.u64 = *machine->tls_base + imm;
        }
        else{
          result.code = EVAL_ResultCode_BadTLSBase;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_ConstU8:
      case RADDBGI_EvalOp_ConstU16:
      case RADDBGI_EvalOp_ConstU32:
      case RADDBGI_EvalOp_ConstU64:
      {
        nval.u64 = imm;
      }break;
      
      case RADDBGI_EvalOp_Abs:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.f32 = svals[0].f32;
          if (svals[0].f32 < 0){
            nval.f32 = -svals[0].f32;
          }
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.f64 = svals[0].f64;
          if (svals[0].f64 < 0){
            nval.f64 = -svals[0].f64;
          }
        }
        else{
          nval.s64 = svals[0].s64;
          if (svals[0].s64 < 0){
            nval.s64 = -svals[0].s64;
          }
        }
      }break;
      
      case RADDBGI_EvalOp_Neg:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.f32 = -svals[0].f32;
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.f64 = -svals[0].f64;
        }
        else{
          nval.u64 = (~svals[0].u64) + 1;
        }
      }break;
      
      case RADDBGI_EvalOp_Add:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.f32 = svals[0].f32 + svals[1].f32;
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.f64 = svals[0].f64 + svals[1].f64;
        }
        else{
          nval.u64 = svals[0].u64 + svals[1].u64;
        }
      }break;
      
      case RADDBGI_EvalOp_Sub:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.f32 = svals[0].f32 - svals[1].f32;
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.f64 = svals[0].f64 - svals[1].f64;
        }
        else{
          nval.u64 = svals[0].u64 - svals[1].u64;
        }
      }break;
      
      case RADDBGI_EvalOp_Mul:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.f32 = svals[0].f32*svals[1].f32;
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.f64 = svals[0].f64*svals[1].f64;
        }
        else{
          nval.u64 = svals[0].u64*svals[1].u64;
        }
      }break;
      
      case RADDBGI_EvalOp_Div:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          if (svals[1].f32 != 0.f){
            nval.f32 = svals[0].f32/svals[1].f32;
          }
          else
          {
            result.code = EVAL_ResultCode_DivideByZero;
            goto done;
          }
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          if (svals[1].f64 != 0.){
            nval.f64 = svals[0].f64/svals[1].f64;
          }
          else
          {
            result.code = EVAL_ResultCode_DivideByZero;
            goto done;
          }
        }
        else if (imm == RADDBGI_EvalTypeGroup_U ||
                 imm == RADDBGI_EvalTypeGroup_S){
          if (svals[1].u64 != 0){
            nval.u64 = svals[0].u64/svals[1].u64;
          }
          else
          {
            result.code = EVAL_ResultCode_DivideByZero;
            goto done;
          }
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_Mod:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          if (svals[1].u64 != 0){
            nval.u64 = svals[0].u64%svals[1].u64;
          }
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_LShift:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = svals[0].u64 << svals[1].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_RShift:
      {
        if (imm == RADDBGI_EvalTypeGroup_U){
          nval.u64 = svals[0].u64 >> svals[1].u64;
        }
        else if (imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = svals[0].s64 >> svals[1].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_BitAnd:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = svals[0].u64&svals[1].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_BitOr:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = svals[0].u64|svals[1].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_BitXor:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = svals[0].u64^svals[1].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_BitNot:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = ~svals[0].u64;
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_LogAnd:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].u64 && svals[1].u64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_LogOr:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].u64 || svals[1].u64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_LogNot:
      {
        if (imm == RADDBGI_EvalTypeGroup_U ||
            imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (!svals[0].u64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_EqEq:
      {
        nval.u64 = (svals[0].u64 == svals[1].u64);
      }break;
      
      case RADDBGI_EvalOp_NtEq:
      {
        nval.u64 = (svals[0].u64 != svals[1].u64);
      }break;
      
      case RADDBGI_EvalOp_LsEq:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.u64 = (svals[0].f32 <= svals[1].f32);
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.u64 = (svals[0].f64 <= svals[1].f64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_U){
          nval.u64 = (svals[0].u64 <= svals[1].u64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].s64 <= svals[1].s64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_GrEq:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.u64 = (svals[0].f32 >= svals[1].f32);
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.u64 = (svals[0].f64 >= svals[1].f64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_U){
          nval.u64 = (svals[0].u64 >= svals[1].u64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].s64 >= svals[1].s64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_Less:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.u64 = (svals[0].f32 < svals[1].f32);
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.u64 = (svals[0].f64 < svals[1].f64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_U){
          nval.u64 = (svals[0].u64 < svals[1].u64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].s64 < svals[1].s64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_Grtr:
      {
        if (imm == RADDBGI_EvalTypeGroup_F32){
          nval.u64 = (svals[0].f32 > svals[1].f32);
        }
        else if (imm == RADDBGI_EvalTypeGroup_F64){
          nval.u64 = (svals[0].f64 > svals[1].f64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_U){
          nval.u64 = (svals[0].u64 > svals[1].u64);
        }
        else if (imm == RADDBGI_EvalTypeGroup_S){
          nval.u64 = (svals[0].s64 > svals[1].s64);
        }
        else{
          result.code = EVAL_ResultCode_BadOpTypes;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_Trunc:
      {
        if (0 < imm){
          U64 mask = 0;
          if (imm < 64){
            mask = max_U64 >> (64 - imm);
          }
          nval.u64 = svals[0].u64&mask;
        }
      }break;
      
      case RADDBGI_EvalOp_TruncSigned:
      {
        if (0 < imm){
          U64 mask = 0;
          if (imm < 64){
            mask = max_U64 >> (64 - imm);
          }
          U64 high = 0;
          if (svals[0].u64 & (1 << (imm - 1))){
            high = ~mask;
          }
          nval.u64 = high|(svals[0].u64&mask);
        }
      }break;
      
      case RADDBGI_EvalOp_Convert:
      {
        U32 in = imm&0xFF;
        U32 out = (imm >> 8)&0xFF;
        if (in != out){
          switch (in + out*RADDBGI_EvalTypeGroup_COUNT){
            case RADDBGI_EvalTypeGroup_F32 + RADDBGI_EvalTypeGroup_U*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f32;
            }break;
            case RADDBGI_EvalTypeGroup_F64 + RADDBGI_EvalTypeGroup_U*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.u64 = (U64)svals[0].f64;
            }break;
            
            case RADDBGI_EvalTypeGroup_F32 + RADDBGI_EvalTypeGroup_S*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f32;
            }break;
            case RADDBGI_EvalTypeGroup_F64 + RADDBGI_EvalTypeGroup_S*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.s64 = (S64)svals[0].f64;
            }break;
            
            case RADDBGI_EvalTypeGroup_U + RADDBGI_EvalTypeGroup_F32*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].u64;
            }break;
            case RADDBGI_EvalTypeGroup_S + RADDBGI_EvalTypeGroup_F32*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].s64;
            }break;
            case RADDBGI_EvalTypeGroup_F64 + RADDBGI_EvalTypeGroup_F32*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f32 = (F32)svals[0].f64;
            }break;
            
            case RADDBGI_EvalTypeGroup_U + RADDBGI_EvalTypeGroup_F64*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].u64;
            }break;
            case RADDBGI_EvalTypeGroup_S + RADDBGI_EvalTypeGroup_F64*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].s64;
            }break;
            case RADDBGI_EvalTypeGroup_F32 + RADDBGI_EvalTypeGroup_F64*RADDBGI_EvalTypeGroup_COUNT:
            {
              nval.f64 = (F64)svals[0].f32;
            }break;
          }
        }
      }break;
      
      case RADDBGI_EvalOp_Pick:
      {
        if (stack_count > imm){
          nval = stack[stack_count - imm - 1];
        }
        else{
          result.code = EVAL_ResultCode_BadOp;
          goto done;
        }
      }break;
      
      case RADDBGI_EvalOp_Pop:
      {
        // do nothing - the pop is handled by the control bits
      }break;
      
      case RADDBGI_EvalOp_Insert:
      {
        if (stack_count > imm){
          if (imm > 0){
            EVAL_Slot tval = stack[stack_count - 1];
            EVAL_Slot *dst = stack + stack_count - 1 - imm;
            EVAL_Slot *shift = dst + 1;
            MemoryCopy(shift, dst, imm*sizeof(EVAL_Slot));
            *dst = tval;
          }
        }
        else{
          result.code = EVAL_ResultCode_BadOp;
          goto done;
        }
      }break;
    }
    
    // push
    {
      U64 push_count = RADDBGI_PUSHN_FROM_CTRLBITS(ctrlbits);
      if (push_count == 1){
        if (stack_count < stack_cap){
          stack[stack_count] = nval;
          stack_count += 1;
        }
        else{
          result.code = EVAL_ResultCode_InsufficientStackSpace;
          goto done;
        }
      }
    }
    
  }
  done:;
  
  if (stack_count == 1){
    result.value = stack[0];
  }
  else if(result.code == EVAL_ResultCode_Good){
    result.code = EVAL_ResultCode_MalformedBytecode;
  }
  
  scratch_end(scratch);
  ProfEnd();
  return(result);
}