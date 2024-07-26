// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): Eval Decode Function

internal void
eval_print_decode_from_bytecode(FILE *out, String8 bytecode){
  U8 *ptr = bytecode.str;
  U8 *opl = bytecode.str + bytecode.size;
  
  for (;ptr < opl;){
    // consume opcode
    SYMS_EvalOp op = (SYMS_EvalOp)*ptr;
    if (op >= SYMS_EvalOp_COUNT){
      fprintf(out, "decode error: undefined op code\n");
      goto done;
    }
    U8 ctrlbits = syms_eval_opcode_ctrlbits[op];
    ptr += 1;
    
    // decode
    U64 imm = 0;
    U32 decode_size = (ctrlbits >> SYMS_EvalOpCtrlBits_DecodeShft)&SYMS_EvalOpCtrlBits_DecodeMask;
    {
      U8 *next_ptr = ptr + decode_size;
      if (next_ptr > opl){
        fprintf(out, "decode error: expected constant goes past the end of bytecode\n");
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
    
    // op string & control bits
    SYMS_String8 op_string = syms_eval_opcode_strings[op];
    
    // print
    fprintf(out, "%.*s 0x%llx\n", str8_varg(op_string), imm);
  }
  done:;
}
