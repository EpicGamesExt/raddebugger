// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO:
//
// [ ] Currently converter relies on clang's -gdwarf-aranges to generate compile unit ranges,
//     however it is optional and in case it is missing converter has to generate the ranges from scopes.
// [ ] Error handling

internal RDI_RegCode
d2r_rdi_reg_from_dw_reg_code_x64(U64 reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX64_##reg_name_dw: return RDI_RegCodeX64_##reg_name_rdi;
    DW_Regs_X64_XList(X)
#undef X
  }
  InvalidPath;
  return 0;
}

internal RDI_RegCode
d2r_rdi_reg_from_dw_reg_code_x86(U64 reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX86_##reg_name_dw: return RDI_RegCodeX86_##reg_name_rdi;
    DW_Regs_X86_XList(X)
#undef X
  }
  InvalidPath;
  return 0;
}

internal RDI_RegCode
d2r_rdi_reg_from_dw_reg_code(Arch arch, U64 reg_code)
{
  switch (arch) {
    case Arch_Null: return 0;
    case Arch_x64:  return d2r_rdi_reg_from_dw_reg_code_x64(reg_code);
    case Arch_x86:  return d2r_rdi_reg_from_dw_reg_code_x86(reg_code);
    default: InvalidPath;
  }
  return 0;
}

internal RDIM_Type *
d2r_create_type(Arena *arena, D2R_TypeTable *type_table)
{
  RDIM_Type *type = rdim_type_chunk_list_push(arena, type_table->types, type_table->type_chunk_cap);
  return type;
}

internal RDIM_Type *
d2r_find_or_create_type_from_offset(Arena *arena, D2R_TypeTable *type_table, U64 info_off)
{
  RDIM_Type *type = 0;
  KeyValuePair *is_type_present = hash_table_search_u64(type_table->ht, info_off);
  if (is_type_present) {
    type = is_type_present->value_raw;
  } else {
    type = d2r_create_type(arena, type_table);
    hash_table_push_u64_raw(arena, type_table->ht, info_off, type);
  }
  return type;
}

internal RDIM_Type *
d2r_type_from_attrib(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Tag tag, DW_AttribKind kind)
{
  RDIM_Type *type = 0;
  
  // find attrib
  DW_Attrib *attrib = dw_attrib_from_tag(input, cu, tag, kind);
  
  // does tag have this attribute?
  if (attrib->attrib_kind == kind) {
    DW_AttribClass value_class = dw_value_class_from_attrib(cu, attrib);
    
    if (value_class == DW_AttribClass_Reference) {
      // resolve reference
      DW_Reference ref = dw_ref_from_attrib_ptr(input, cu, attrib);
      
      // TODO: support for external compile unit references
      AssertAlways(ref.cu == cu);
      
      // find or create type
      type = d2r_find_or_create_type_from_offset(arena, type_table, ref.info_off);
    } else {
      Assert(!"unexpected attrib class");
    }
  } else if (attrib->attrib_kind == DW_Attrib_Null) {
    type = rdim_builtin_type_from_kind(*type_table->types, RDI_TypeKind_NULL);
  }
  
  return type;
}

internal Rng1U64List
d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag)
{
  // collect non-contiguous range
  Rng1U64List ranges = dw_rnglist_from_attrib(arena, input, cu, tag, DW_Attrib_Ranges);
  
  // debase ranges
  for (Rng1U64Node *range_n = ranges.first; range_n != 0; range_n = range_n->next) {
    // TODO: error handling
    AssertAlways(range_n->v.min >= image_base);
    AssertAlways(range_n->v.max >= image_base);
    range_n->v.min -= image_base;
    range_n->v.max -= image_base;
  }
  
  // collect contiguous range
  DW_Attrib *lo_pc_attrib = dw_attrib_from_tag(input, cu, tag, DW_Attrib_LowPc);
  DW_Attrib *hi_pc_attrib = dw_attrib_from_tag(input, cu, tag, DW_Attrib_HighPc);
  if (lo_pc_attrib->attrib_kind != DW_Attrib_Null && hi_pc_attrib->attrib_kind != DW_Attrib_Null) {
    U64 lo_pc = dw_address_from_attrib_ptr(input, cu, lo_pc_attrib);
    
    U64 hi_pc;
    DW_AttribClass hi_pc_class = dw_value_class_from_attrib(cu, hi_pc_attrib);
    if (hi_pc_class == DW_AttribClass_Address) {
      hi_pc = dw_address_from_attrib_ptr(input, cu, hi_pc_attrib);
    } else if (hi_pc_class == DW_AttribClass_Const) {
      hi_pc = dw_const_u64_from_attrib_ptr(input, cu, hi_pc_attrib);
      hi_pc += lo_pc;
    } else {
      AssertAlways(!"undefined attrib encoding");
    }
    
    // TODO: error handling
    AssertAlways(lo_pc >= image_base);
    AssertAlways(hi_pc >= image_base);
    AssertAlways(lo_pc <= hi_pc);
    
    U64 lo_voff = lo_pc - image_base;
    U64 hi_voff = hi_pc - image_base;
    rng1u64_list_push(arena, &ranges, rng_1u64(lo_voff, hi_voff));
  }
  
  return ranges;
}

internal RDIM_Type **
d2r_collect_proc_params(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_TagNode *cur_node, U64 *param_count_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  RDIM_TypeList list = {0};
  B32 has_vargs = 0;
  for (DW_TagNode *i = cur_node->first_child; i != 0; i = i->sibling) {
    if (i->tag.kind == DW_Tag_FormalParameter) {
      RDIM_TypeNode *n = push_array(scratch.arena, RDIM_TypeNode, 1);
      n->v             = d2r_type_from_attrib(arena, type_table, input, cu, i->tag, DW_Attrib_Type);
      SLLQueuePush(list.first, list.last, n);
      ++list.count;
    } else if (i->tag.kind == DW_Tag_UnspecifiedParameters) {
      has_vargs = 1;
    }
  }
  
  if (has_vargs) {
    RDIM_TypeNode *n = push_array(scratch.arena, RDIM_TypeNode, 1);
    n->v = type_table->varg_type;
    SLLQueuePush(list.first, list.last, n);
    ++list.count;
  }
  
  // collect params
  *param_count_out  = list.count;
  RDIM_Type **params = rdim_array_from_type_list(arena, list);
  
  scratch_end(scratch);
  return params;
}

internal RDI_TypeKind
d2r_unsigned_type_kind_from_size(U64 byte_size)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch (byte_size) {
    case 1: result = RDI_TypeKind_U8;  break;
    case 2: result = RDI_TypeKind_U16; break;
    case 4: result = RDI_TypeKind_U32; break;
    case 8: result = RDI_TypeKind_U64; break;
  }
  return result;
}

internal RDI_TypeKind
d2r_signed_type_kind_from_size(U64 byte_size)
{
  RDI_TypeKind result = RDI_TypeKind_NULL;
  switch (byte_size) {
    case 1: result = RDI_TypeKind_S8;  break;
    case 2: result = RDI_TypeKind_S16; break;
    case 4: result = RDI_TypeKind_S32; break;
    case 8: result = RDI_TypeKind_S64; break;
  }
  return result;
}

internal RDI_EvalTypeGroup
d2r_type_group_from_type_kind(RDI_TypeKind x)
{
  switch (x) {
    case RDI_TypeKind_NULL:
    case RDI_TypeKind_Void:
    case RDI_TypeKind_Handle:
    break;
    case RDI_TypeKind_UChar8:
    case RDI_TypeKind_UChar16:
    case RDI_TypeKind_UChar32:
    case RDI_TypeKind_U8:
    case RDI_TypeKind_U16:
    case RDI_TypeKind_U32:
    case RDI_TypeKind_U64:
    case RDI_TypeKind_U128:
    case RDI_TypeKind_U256:
    case RDI_TypeKind_U512:
    return RDI_EvalTypeGroup_U;
    case RDI_TypeKind_Char8:
    case RDI_TypeKind_Char16:
    case RDI_TypeKind_Char32:
    case RDI_TypeKind_S8:
    case RDI_TypeKind_S16:
    case RDI_TypeKind_S32:
    case RDI_TypeKind_S64:
    case RDI_TypeKind_S128:
    case RDI_TypeKind_S256:
    case RDI_TypeKind_S512:
    return RDI_EvalTypeGroup_S;
    case RDI_TypeKind_F32:
    return RDI_EvalTypeGroup_F32;
    case RDI_TypeKind_F64:
    return RDI_EvalTypeGroup_F64;
    default: InvalidPath;
  }
  return RDI_EvalTypeGroup_Other;
}

internal RDIM_EvalBytecode
d2r_bytecode_from_expression(Arena       *arena,
                             DW_Input    *input,
                             U64          image_base,
                             U64          address_size,
                             Arch         arch,
                             DW_ListUnit *addr_lu,
                             String8      expr,
                             DW_CompUnit *cu,
                             B32         *is_addr_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  RDIM_EvalBytecode bc = {0};
  
  *is_addr_out = 0;
  
  struct Frame {
    struct Frame      *next;
    RDI_EvalTypeGroup  value_type;
  };
  struct Frame *stack = 0;
#define push_of_type(type) do {                                 \
struct Frame *f = push_array(scratch.arena, struct Frame, 1); \
f->value_type   = d2r_type_group_from_type_kind(type);        \
SLLStackPush(stack, f);                                       \
} while (0)
#define pop_type()  stack->value_type; SLLStackPop(stack)
#define peek_type() stack->value_type
  
  
  RDI_TypeKind addr_type_kind = RDI_TypeKind_NULL;
  if (address_size == 4) {
    addr_type_kind = RDI_TypeKind_U32;
  } else if (address_size == 8) {
    addr_type_kind = RDI_TypeKind_U64;
  }
  
  
  for (U64 cursor = 0; cursor < expr.size; ) {
    U8 op = 0;
    cursor += str8_deserial_read_struct(expr, cursor, &op);
    
    U64 size_param;
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
        U64 lit = op - DW_ExprOp_Lit0;
        
        rdim_bytecode_push_uconst(arena, &bc, lit);
        push_of_type(RDI_TypeKind_U64);
      } break;
      
      case DW_ExprOp_Const1U: {
        U8 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_uconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_U8);
      } break;
      case DW_ExprOp_Const2U: {
        U16 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_uconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_U16);
      } break;
      case DW_ExprOp_Const4U: {
        U32 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_uconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_U32);
      } break;
      case DW_ExprOp_Const8U: {
        U64 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_uconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_U64);
      } break;
      
      case DW_ExprOp_Const1S: {
        S8 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_sconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_S8);
      } break;
      case DW_ExprOp_Const2S: {
        S16 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_sconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_S16);
      } break;
      case DW_ExprOp_Const4S: {
        S32 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_sconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_S32);
      } break;
      case DW_ExprOp_Const8S: {
        S64 val = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &val);
        
        rdim_bytecode_push_sconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_S64);
      } break;
      
      case DW_ExprOp_ConstU: {
        U64 val = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &val);
        
        rdim_bytecode_push_uconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_U64);
      } break;
      
      case DW_ExprOp_ConstS: {
        S64 val = 0;
        cursor += str8_deserial_read_sleb128(expr, cursor, &val);
        
        rdim_bytecode_push_sconst(arena, &bc, val);
        push_of_type(RDI_TypeKind_S64);
      } break;
      
      case DW_ExprOp_Addr: {
        U64 addr = 0;
        cursor += str8_deserial_read(expr, cursor, &addr, address_size, address_size);
        if (addr >= image_base) {
          U64 voff = addr - image_base;
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, voff);
          push_of_type(addr_type_kind);
        } else {
          // TODO: error handling
          AssertAlways(!"unable to relocate address");
        }
        
        *is_addr_out = 1;
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
        U64 reg_code_dw  = op - DW_ExprOp_Reg0;
        U64 reg_size     = dw_reg_size_from_code(arch, reg_code_dw);
        U64 reg_pos      = dw_reg_pos_from_code(arch, reg_code_dw);
        
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_from_dw_reg_code(arch, reg_code_dw);
        U32         regread_param = RDI_EncodeRegReadParam(reg_code_rdi, reg_size, reg_pos);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, regread_param);
        push_of_type(d2r_unsigned_type_kind_from_size(reg_size));
      } break;
      
      case DW_ExprOp_RegX: {
        U64 reg_code_dw = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &reg_code_dw);
        
        U64 reg_size = dw_reg_size_from_code(arch, reg_code_dw);
        U64 reg_pos  = dw_reg_pos_from_code(arch, reg_code_dw);
        
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_from_dw_reg_code(arch, reg_code_dw);
        U32         regread_param = RDI_EncodeRegReadParam(reg_code_rdi, reg_size, reg_pos);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, regread_param);
        push_of_type(d2r_unsigned_type_kind_from_size(reg_size));
        
        *is_addr_out = 1;
      } break;
      
      case DW_ExprOp_ImplicitValue: {
        U64     val_size = 0;
        String8 val      = {0};
        cursor += str8_deserial_read_uleb128(expr, cursor, &val_size);
        cursor += str8_deserial_read_block(expr, cursor, val_size, &val);
        if (val.size <= sizeof(U64)) {
          U64 val64 = 0;
          MemoryCopy(&val64, val.str, val.size);
          
          rdim_bytecode_push_uconst(arena, &bc, val64);
          push_of_type(d2r_unsigned_type_kind_from_size(val_size));
        } else {
          // TODO: currenlty no way to encode string in RDIM_EvalBytecodeOp
          NotImplemented;
        }
      } break;
      
      case DW_ExprOp_Piece: {
        U64 piece_byte_size = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &piece_byte_size);
        
        U64 partial_value_size32 = safe_cast_u32(piece_byte_size);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_PartialValue, partial_value_size32);
      } break;
      
      case DW_ExprOp_BitPiece: {
        U64 piece_bit_size = 0;
        U64 piece_bit_off  = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &piece_bit_size);
        cursor += str8_deserial_read_uleb128(expr, cursor, &piece_bit_off);
        
        U32 piece_bit_size32 = safe_cast_u32(piece_bit_size);
        U32 piece_bit_off32  = safe_cast_u32(piece_bit_off);
        
        U64 partial_value = ((U64)piece_bit_size32 << 32) | (U64)piece_bit_off32;
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_PartialValueBit, partial_value);
      } break;
      
      case DW_ExprOp_Pick: {
        U8 stack_idx = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &stack_idx);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, stack_idx);
      } break;
      
      case DW_ExprOp_PlusUConst: {
        U64 addend = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &addend);
        rdim_bytecode_push_uconst(arena, &bc, addend);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, RDI_EvalTypeGroup_U);
      } break;
      
      case DW_ExprOp_Skip: {
        S16 skip = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &skip);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Skip, skip);
      } break;
      
      case DW_ExprOp_Bra: {
        NotImplemented;
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
        U64 reg_code_dw = op - DW_ExprOp_BReg0;
        S64 reg_off     = 0;
        cursor += str8_deserial_read_sleb128(expr, cursor, &reg_off);
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_from_dw_reg_code(arch, reg_code_dw);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegReadDyn, reg_code_rdi);
        if (reg_off > 0) {
          rdim_bytecode_push_sconst(arena, &bc, reg_off);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
        }
        push_of_type(RDI_TypeKind_S64);
        
        *is_addr_out = 1;
      } break;
      
      case DW_ExprOp_BRegX: {
        U64 reg_code_dw = 0;
        S64 reg_off     = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &reg_code_dw);
        cursor += str8_deserial_read_sleb128(expr, cursor, &reg_off);
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_from_dw_reg_code(arch, reg_code_dw);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegReadDyn, reg_code_rdi);
        if (reg_off > 0) {
          rdim_bytecode_push_sconst(arena, &bc, reg_off);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, RDI_EvalTypeGroup_S);
        }
        push_of_type(RDI_TypeKind_S64);
        
        *is_addr_out = 1;
      } break;
      
      case DW_ExprOp_FBReg: {
        S64 frame_off = 0;
        cursor += str8_deserial_read_sleb128(expr, cursor, &frame_off);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_FrameOff, frame_off);
        
        *is_addr_out = 1;
      } break;
      
      case DW_ExprOp_Deref: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, address_size);
      } break;
      
      case DW_ExprOp_DerefSize: {
        U8 deref_size_in_bytes = 0;
        cursor += str8_deserial_read_struct(expr, cursor, &deref_size_in_bytes);
        if (0 < deref_size_in_bytes && deref_size_in_bytes <= address_size) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, deref_size_in_bytes);
        } else {
          // TODO: error handling
          AssertAlways(!"ill formed expression");
        }
      } break;
      
      case DW_ExprOp_XDerefSize: {
        // TODO: error handling
        AssertAlways(!"no suitable conversion");
      } break;
      
      case DW_ExprOp_Call2:
      case DW_ExprOp_Call4:
      case DW_ExprOp_CallRef: {
        // TODO: error handling
        AssertAlways(!"calls are not supported");
      } break;
      
      case DW_ExprOp_ImplicitPointer:
      case DW_ExprOp_GNU_ImplicitPointer: {
        // TODO:
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_Convert:
      case DW_ExprOp_GNU_Convert: {
        U64 type_info_off = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &type_info_off);
        
        RDI_EvalTypeGroup in  = stack ? d2r_type_group_from_type_kind(stack->value_type) : RDI_EvalTypeGroup_Other;
        RDI_EvalTypeGroup out = RDI_EvalTypeGroup_Other;
        
        if (type_info_off == 0) {
          //
          // 2.5.1
          // Instead of a base type, elements can have a generic type,
          // which is an integral type that has the size of an address
          // on the target machine and unspecified signedness.
          //
          out = d2r_type_group_from_type_kind(addr_type_kind);
        } else {
          // find ref tag
          DW_TagNode *tag_node = dw_tag_node_from_info_off(cu, type_info_off);
          DW_Tag      tag      = tag_node->tag;
          if (tag.kind == DW_Tag_BaseType) {
            // extract encoding attribute
            DW_ATE encoding = dw_const_u64_from_attrib(input, cu, tag, DW_Attrib_Encoding);
            
            // DW_ATE -> RDI_EvalTypeGroup
            switch (encoding) {
              case DW_ATE_SignedChar:
              case DW_ATE_Signed:   out = RDI_EvalTypeGroup_S; break;
              case DW_ATE_UnsignedChar:
              case DW_ATE_Unsigned: out = RDI_EvalTypeGroup_U; break;
              case DW_ATE_Float: {
                U64 byte_size = dw_const_u64_from_attrib(input, cu, tag, DW_Attrib_ByteSize);
                switch (byte_size) {
                  case 4: out = RDI_EvalTypeGroup_F32; break;
                  case 8: out = RDI_EvalTypeGroup_F64; break;
                  default: InvalidPath;
                }
              } break;
              default: InvalidPath;
            }
          } else {
            AssertAlways(!"unexpected tag"); // TODO: error handling
          }
        }
        
        if (in == RDI_EvalTypeGroup_Other) {
          push_of_type(out);
          break;
        }
        
        // TODO: error handling
        AssertAlways(in != RDI_EvalTypeGroup_Other);
        AssertAlways(out != RDI_EvalTypeGroup_Other);
        
        U16 operand = (U16)in | ((U16)out << 8);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Convert, operand);
      } break;
      
      case DW_ExprOp_GNU_ParameterRef: {
        // TODO:
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_DerefType:
      case DW_ExprOp_GNU_DerefType: {
        // TODO:
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_ConstType: 
      case DW_ExprOp_GNU_ConstType: {
        // TODO:
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_RegvalType: {
        // TODO:
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_EntryValue:
      case DW_ExprOp_GNU_EntryValue: {
        U64     entry_value_expr_size = 0;
        String8 entry_value_expr      = {0};
        cursor += str8_deserial_read_uleb128(expr, cursor, &entry_value_expr_size);
        cursor += str8_deserial_read_block(expr, cursor, entry_value_expr_size, &entry_value_expr);
        
        B32 dummy = 0;
        RDIM_EvalBytecode call_site_bc = d2r_bytecode_from_expression(arena, input, image_base, address_size, arch, addr_lu, entry_value_expr, cu, &dummy);
        
        U32 encoded_size32 = safe_cast_u32(call_site_bc.encoded_size);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_CallSiteValue, encoded_size32);
        rdim_bytecode_concat_in_place(&bc, &call_site_bc);
      } break;
      
      case DW_ExprOp_Addrx: {
        U64 addr_idx = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &addr_idx);
        U64 addr = dw_addr_from_list_unit(addr_lu, addr_idx);
        if (addr != max_U64) {
          if (addr >= image_base) {
            U64 voff = addr - image_base;
            rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, voff);
          } else {
            // TODO: error handling
            AssertAlways(!"unable to relocate address");
          }
        } else {
          // TODO: error handling
          AssertAlways(!"out of bounds index");
        }
      } break;
      
      case DW_ExprOp_CallFrameCfa: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_FrameOff, 0);
      } break;
      
      case DW_ExprOp_FormTlsAddress: {
        // TODO:
        AssertAlways(!"RDI_EvalOp_TLSOff accepts immediate");
      } break;
      
      case DW_ExprOp_PushObjectAddress: {
        AssertAlways(!"sample");
      } break;
      
      case DW_ExprOp_Nop: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Noop, 0);
      } break;
      
      case DW_ExprOp_Eq: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_EqEq, peek_type());
      } break;
      
      case DW_ExprOp_Ge: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_GrEq, peek_type());
      } break;
      
      case DW_ExprOp_Gt: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Grtr, peek_type());
      } break;
      
      case DW_ExprOp_Le: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_LsEq, peek_type());
      } break;
      
      case DW_ExprOp_Lt: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Less, peek_type());
      } break;
      
      case DW_ExprOp_Ne: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_NtEq, peek_type());
      } break;
      
      case DW_ExprOp_Shl: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_LShift, peek_type());
      } break;
      
      case DW_ExprOp_Shr: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, RDI_EvalTypeGroup_U);
      } break;
      
      case DW_ExprOp_Shra: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, RDI_EvalTypeGroup_S);
      } break;
      
      case DW_ExprOp_Xor: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_BitXor, peek_type());
      } break;
      
      case DW_ExprOp_XDeref: {
        // TODO: error handling
        Assert(!"multiple address spaces are not supported");
      } break;
      
      case DW_ExprOp_Abs: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Abs, peek_type());
      } break;
      
      case DW_ExprOp_And: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_BitAnd, peek_type());
      } break;
      
      case DW_ExprOp_Div: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Div, peek_type());
      } break;
      
      case DW_ExprOp_Minus: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Sub, peek_type());
      } break;
      
      case DW_ExprOp_Mod: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Mod, peek_type());
      } break;
      
      case DW_ExprOp_Mul: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Mul, peek_type());
      } break;
      
      case DW_ExprOp_Neg: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Neg, peek_type());
      } break;
      
      case DW_ExprOp_Not: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_BitNot, peek_type());
      } break;
      
      case DW_ExprOp_Or: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_BitOr, peek_type());
      } break;
      
      case DW_ExprOp_Plus: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, peek_type());
      } break;
      
      case DW_ExprOp_Rot: {
        AssertAlways(!"no suitable conversion");
      } break;
      
      case DW_ExprOp_Swap: {
        AssertAlways(!"no suitable conversion");
      } break;
      
      case DW_ExprOp_Dup: {
        AssertAlways(!"no suitable conversion");
      } break;
      
      case DW_ExprOp_Drop: {
        AssertAlways(!"no suitable conversion");
      } break;
      
      case DW_ExprOp_Over: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, 1);
      } break;
      
      case DW_ExprOp_StackValue: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Stop, 0);
      } break;
      
      default: InvalidPath; break;
    }
  }
  
#undef peek_type
#undef pop_type
#undef push_of_type
  scratch_end(scratch);
  return bc;
}

internal RDIM_Location *
d2r_transpile_expression(Arena *arena, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr)
{
  RDIM_Location *loc = 0;
  if (expr.size) {
    B32               is_addr  = 0;
    RDIM_EvalBytecode bytecode = d2r_bytecode_from_expression(arena, input, image_base, address_size, arch, addr_lu, expr, cu, &is_addr);
    
    loc           = push_array(arena, RDIM_Location, 1);
    loc->kind     = is_addr ? RDI_LocationKind_AddrBytecodeStream : RDI_LocationKind_ValBytecodeStream;
    loc->bytecode = bytecode;
  }
  return loc;
}

internal RDIM_Location *
d2r_location_from_attrib(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind)
{
  String8 expr = dw_exprloc_from_attrib(input, cu, tag, kind);
  RDIM_Location *location = d2r_transpile_expression(arena, input, image_base, cu->address_size, arch, cu->addr_lu, cu, expr);
  return location;
}

internal RDIM_LocationSet
d2r_locset_from_attrib(Arena               *arena,
                       DW_Input            *input,
                       DW_CompUnit         *cu,
                       RDIM_ScopeChunkList *scopes,
                       RDIM_Scope          *curr_scope,
                       U64                  image_base,
                       Arch                 arch,
                       DW_Tag               tag,
                       DW_AttribKind        kind)
{
  RDIM_LocationSet locset = {0};
  
  // extract attrib from tag
  DW_Attrib      *attrib       = dw_attrib_from_tag(input, cu, tag, kind);
  DW_AttribClass  attrib_class = dw_value_class_from_attrib(cu, attrib);
  
  if (attrib_class == DW_AttribClass_LocList || attrib_class == DW_AttribClass_LocListPtr) {
    Temp scratch = scratch_begin(&arena, 1);
    
    // extract location list from attrib
    DW_LocList loclist = dw_loclist_from_attrib_ptr(scratch.arena, input, cu, attrib);
    
    // convert location list to RDIM location set
    for (DW_LocNode *loc_n = loclist.first; loc_n != 0; loc_n = loc_n->next) {
      RDIM_Location *location   = d2r_transpile_expression(arena, input, image_base, cu->address_size, arch, cu->addr_lu, cu, loc_n->v.expr);
      RDIM_Rng1U64   voff_range = { .min = loc_n->v.range.min -  image_base, .max = loc_n->v.range.max - image_base };
      rdim_location_set_push_case(arena, scopes, &locset, voff_range, location);
    }
    
    scratch_end(scratch);
  } else if (attrib_class == DW_AttribClass_ExprLoc) {
    // extract expression from attrib
    String8 expr = dw_exprloc_from_attrib_ptr(input, cu, attrib);
    
    // convert expression and inherit life-time ranges from enclosed scope
    RDIM_Location *location = d2r_transpile_expression(arena, input, image_base, cu->address_size, arch, cu->addr_lu, cu, expr);
    for (RDIM_Rng1U64Node *range_n = curr_scope->voff_ranges.first; range_n != 0; range_n = range_n->next) {
      rdim_location_set_push_case(arena, scopes, &locset, range_n->v, location);
    }
  } else if (attrib_class != DW_AttribClass_Null) {
    AssertAlways(!"unexpected attrib class");
  }
  
  return locset;
}

internal RDIM_LocationSet
d2r_var_locset_from_tag(Arena               *arena,
                        DW_Input            *input,
                        DW_CompUnit         *cu,
                        RDIM_ScopeChunkList *scopes,
                        RDIM_Scope          *curr_scope,
                        U64                  image_base,
                        Arch                 arch,
                        DW_Tag               tag)
{
  RDIM_LocationSet locset = {0};
  
  B32 has_const_value = dw_tag_has_attrib(input, cu, tag, DW_Attrib_ConstValue);
  B32 has_location    = dw_tag_has_attrib(input, cu, tag, DW_Attrib_Location);
  
  if (has_const_value && has_location) {
    // TODO: error handling
    AssertAlways(!"unexpected variable encoding");
  }
  
  if (has_const_value) {
    // extract const value
    U64 const_value = dw_u64_from_attrib(input, cu, tag, DW_Attrib_ConstValue);
    
    // make value byte code
    RDIM_EvalBytecode bc = {0};
    rdim_bytecode_push_uconst(arena, &bc, const_value);
    
    // fill out location
    RDIM_Location *loc = push_array(arena, RDIM_Location, 1);
    loc->kind          = RDI_LocationKind_ValBytecodeStream;
    loc->bytecode      = bc;
    
    // push location cases
    for (RDIM_Rng1U64Node *range_n = curr_scope->voff_ranges.first; range_n != 0; range_n = range_n->next) {
      rdim_location_set_push_case(arena, scopes, &locset, range_n->v, loc);
    }
  } else if (has_location) {
    locset = d2r_locset_from_attrib(arena, input, cu, scopes, curr_scope, image_base, arch, tag, DW_Attrib_Location);
  }
  
  return locset;
}

internal D2R_CompUnitContribMap
d2r_cu_contrib_map_from_aranges(Arena *arena, DW_Input *input, U64 image_base)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8     aranges_data    = input->sec[DW_Section_ARanges].data;
  Rng1U64List unit_range_list = dw_unit_ranges_from_data(scratch.arena, aranges_data);
  
  D2R_CompUnitContribMap cm = {0};
  cm.count                  = 0;
  cm.info_off_arr           = push_array(arena, U64,              unit_range_list.count);
  cm.voff_range_arr         = push_array(arena, RDIM_Rng1U64List, unit_range_list.count);
  
  for (Rng1U64Node *range_n = unit_range_list.first; range_n != 0; range_n = range_n->next) {
    String8 unit_data = str8_substr(aranges_data, range_n->v);
    U64     unit_cursor    = 0;
    
    U64 unit_length = 0;
    U64 unit_length_size = str8_deserial_read_dwarf_packed_size(unit_data, unit_cursor, &unit_length);
    if (unit_length_size == 0) {
      continue;
    }
    unit_cursor += unit_length_size;
    
    DW_Version version = 0;
    U64 version_size = str8_deserial_read_struct(unit_data, unit_cursor, &version);
    if (version_size == 0) {
      continue;
    }
    unit_cursor += version;
    
    if (version != DW_Version_2) {
      AssertAlways(!"unknown .debug_aranges version");
      continue;
    }
    
    DW_Format unit_format      = DW_FormatFromSize(unit_length);
    U64       cu_info_off      = 0;
    U64       cu_info_off_size = str8_deserial_read_dwarf_uint(unit_data, unit_cursor, unit_format, &cu_info_off);
    if (cu_info_off_size == 0) {
      continue;
    }
    unit_cursor += cu_info_off_size;
    
    U8 address_size = 0;
    U64 address_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &address_size);
    if (address_size_size == 0) {
      continue;
    }
    unit_cursor += address_size_size;
    
    U8 segment_selector_size = 0;
    U64 segment_selector_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &segment_selector_size);
    if (segment_selector_size_size == 0) {
      continue;
    }
    unit_cursor += segment_selector_size_size;
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = unit_cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      unit_cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    RDIM_Rng1U64List voff_ranges = {0};
    if (segment_selector_size == 0) {
      while (unit_cursor + address_size * 2 <= unit_data.size) {
        U64 address = 0;
        U64 length  = 0;
        unit_cursor += str8_deserial_read(unit_data, unit_cursor, &address, address_size, address_size);
        unit_cursor += str8_deserial_read(unit_data, unit_cursor, &length, address_size, address_size);
        
        if (address == 0 && length == 0) {
          break;
        }
        
        // TODO: error handling
        AssertAlways(address >= image_base);
        
        U64 min = address - image_base;
        U64 max = min + length;
        rdim_rng1u64_list_push(arena, &voff_ranges, (RDIM_Rng1U64){.min = min, .max = max});
      }
    } else {
      // TODO: segment relative addressing
      NotImplemented;
    }
    
    U64 map_idx = cm.count++;
    cm.info_off_arr[map_idx]   = cu_info_off;
    cm.voff_range_arr[map_idx] = voff_ranges;
  }
  
  scratch_end(scratch);
  return cm;
}

internal RDIM_Rng1U64List
d2r_voff_ranges_from_cu_info_off(D2R_CompUnitContribMap map, U64 info_off)
{
  RDIM_Rng1U64List voff_ranges   = {0};
  U64              voff_list_idx = u64_array_bsearch(map.info_off_arr, map.count, info_off);
  if (voff_list_idx < map.count) {
    voff_ranges = map.voff_range_arr[voff_list_idx];
  }
  return voff_ranges;
}

internal RDIM_Scope *
d2r_push_scope(Arena *arena, RDIM_ScopeChunkList *scopes, U64 scope_chunk_cap, D2R_TagNode *tag_stack, Rng1U64List ranges)
{
  // fill out scope
  RDIM_Scope *scope = rdim_scope_chunk_list_push(arena, scopes, scope_chunk_cap);
  
  // push ranges
  for (Rng1U64Node *i = ranges.first; i != 0; i = i->next) {
    rdim_scope_push_voff_range(arena, scopes, scope, (RDIM_Rng1U64){.min = i->v.min, i->v.max});
  }
  
  // associate scope with tag
  tag_stack->scope = scope;
  
  // update scope hierarchy
  DW_TagKind parent_tag_kind = tag_stack->next->cur_node->tag.kind;
  if (parent_tag_kind == DW_Tag_SubProgram || parent_tag_kind == DW_Tag_InlinedSubroutine || parent_tag_kind == DW_Tag_LexicalBlock) {
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

internal RDIM_BakeParams *
d2r_convert(Arena *arena, RDIM_LocalState *local_state, RC_Context *in)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  ////////////////////////////////
  
  ProfBegin("compute exe hash");
  U64 exe_hash = rdi_hash(in->image_data.str, in->image_data.size);
  ProfEnd();
  
  ////////////////////////////////
  
  Arch                   arch            = Arch_Null;
  U64                    image_base      = 0;
  RDIM_BinarySectionList binary_sections = {0};
  DW_Input               input           = {0};
  
  if (in->image == Image_CoffPe) {
    PE_BinInfo pe = pe_bin_info_from_data(scratch.arena, in->image_data);
    
    // get image arch
    arch = pe.arch;
    
    // get image base
    image_base = pe.image_base;
    
    // get image sections
    String8             raw_sections  = str8_substr(in->image_data, pe.section_table_range);
    U64                 section_count = raw_sections.size / sizeof(COFF_SectionHeader);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_sections.str;
    
    // convert sections
    String8 string_table = str8_substr(in->image_data, pe.string_table_range);
    binary_sections = c2r_rdi_binary_sections_from_coff_sections(arena, in->image_data, string_table, section_count, section_table);
    
    // make DWARF input
    input = dw_input_from_coff_section_table(scratch.arena, in->image_data, string_table, section_count, section_table);
  } else if (in->image == Image_Elf32 || in->image == Image_Elf64) {
    ELF_BinInfo elf = elf_bin_from_data(in->debug_data);
    
    // get image arch
    arch = arch_from_elf_machine(elf.hdr.e_machine);
    
    // get image base
    image_base = elf_base_addr_from_bin(&elf.hdr);
    
    // get image sections
    ELF_Shdr64Array shdrs = elf_shdr64_array_from_bin(scratch.arena, in->debug_data, &elf.hdr);
    
    // convert sections
    binary_sections = e2r_rdi_binary_sections_from_elf_section_table(arena, shdrs);
    
    // make DWARF input
    input = dw_input_from_elf_section_table(scratch.arena, in->debug_data, &elf);
  } else {
    InvalidPath;
  }
  
  ////////////////////////////////
  
  RDIM_TopLevelInfo top_level_info = rdim_make_top_level_info(in->image_name, arch, exe_hash, binary_sections);
  
  ////////////////////////////////
  
  U64 arch_addr_size = rdi_addr_size_from_arch(arch);
  
  ////////////////////////////////
  
  static const U64 UNIT_CHUNK_CAP        = 256;
  static const U64 UDT_CHUNK_CAP         = 256;
  static const U64 TYPE_CHUNK_CAP        = 256;
  static const U64 GVAR_CHUNK_CAP        = 256;
  static const U64 TVAR_CHUNK_CAP        = 256;
  static const U64 PROC_CHUNK_CAP        = 256;
  static const U64 SCOPE_CHUNK_CAP       = 256;
  static const U64 INLINE_SITE_CHUNK_CAP = 256;
  static const U64 SRC_FILE_CAP          = 256;
  static const U64 LINE_TABLE_CAP        = 256;
  
  RDIM_UnitChunkList       units        = {0};
  RDIM_UDTChunkList        udts         = {0};
  RDIM_TypeChunkList       types        = rdim_init_type_chunk_list(arena, arch);
  RDIM_SymbolChunkList     gvars        = {0};
  RDIM_SymbolChunkList     tvars        = {0};
  RDIM_SymbolChunkList     procs        = {0};
  RDIM_ScopeChunkList      scopes       = {0};
  RDIM_InlineSiteChunkList inline_sites = {0};
  RDIM_SrcFileChunkList    src_files    = {0};
  RDIM_LineTableChunkList  line_tables  = {0};
  
  ////////////////////////////////
  
  RDIM_Scope *global_scope = rdim_scope_chunk_list_push(arena, &scopes, SCOPE_CHUNK_CAP);
  
  ////////////////////////////////
  
  ProfBegin("Make Unit Contrib Map");
  D2R_CompUnitContribMap cu_contrib_map = {0};
  if (input.sec[DW_Section_ARanges].data.size > 0) {
    cu_contrib_map = d2r_cu_contrib_map_from_aranges(arena, &input, image_base);
  } else {
    // TODO: synthesize cu ranges from scopes
    NotImplemented;
  }
  ProfEnd();
  
  ProfBegin("Parse Comop Unit Ranges");
  DW_ListUnitInput lu_input      = dw_list_unit_input_from_input(scratch.arena, &input);
  Rng1U64List      cu_range_list = dw_unit_ranges_from_data(scratch.arena, input.sec[DW_Section_Info].data);
  Rng1U64Array     cu_ranges     = rng1u64_array_from_list(scratch.arena, &cu_range_list);
  ProfEnd();
  
  ////////////////////////////////
  
  ProfBegin("Parse Compile Unit Headers");
  B32 is_parse_relaxed = !(in->flags & RC_Flag_StrictDwarfParse);
  DW_CompUnit *cu_arr = push_array(scratch.arena, DW_CompUnit, cu_ranges.count);
  for (U64 cu_idx = 0; cu_idx < cu_ranges.count; ++cu_idx) {
    cu_arr[cu_idx] = dw_cu_from_info_off(scratch.arena, &input, lu_input, cu_ranges.v[cu_idx].min, is_parse_relaxed);
  }
  ProfEnd();
  
  ////////////////////////////////
  
  ProfBegin("Parse Line Tables");
  DW_LineTableParseResult *cu_line_tables = push_array(scratch.arena, DW_LineTableParseResult, cu_ranges.count);
  for (U64 cu_idx = 0; cu_idx < cu_ranges.count; ++cu_idx) {
    DW_CompUnit *cu           = &cu_arr[cu_idx];
    String8      cu_stmt_list = dw_line_ptr_from_attrib(&input, cu, cu->tag, DW_Attrib_StmtList);
    String8      cu_dir       = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_CompDir);
    String8      cu_name      = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_Name);
    cu_line_tables[cu_idx] = dw_parsed_line_table_from_data(scratch.arena, cu_stmt_list, &input, cu_dir, cu_name, cu->address_size, cu->str_offsets_lu);
  }
  ProfEnd();
  
  ////////////////////////////////
  
  ProfBegin("Convert Line Tables");
  
  HashTable       *source_file_ht     = hash_table_init(scratch.arena, 0x4000);
  RDIM_LineTable **cu_line_tables_rdi = push_array(scratch.arena, RDIM_LineTable *, cu_ranges.count);
  
  for (U64 cu_idx = 0; cu_idx < cu_ranges.count; ++cu_idx) {
    cu_line_tables_rdi[cu_idx] = rdim_line_table_chunk_list_push(arena, &line_tables, LINE_TABLE_CAP);
    
    DW_LineTableParseResult *line_table   = &cu_line_tables[cu_idx];
    DW_LineVMFileArray      *dir_table    = &line_table->vm_header.dir_table;
    DW_LineVMFileArray      *file_table   = &line_table->vm_header.file_table;
    RDIM_SrcFile           **src_file_map = push_array(scratch.arena, RDIM_SrcFile *, file_table->count);
    for (U64 file_idx = 0; file_idx < file_table->count; ++file_idx) {
      DW_LineFile  *file                 = &file_table->v[file_idx];
      String8       file_path            = dw_path_from_file_idx(scratch.arena, &line_table->vm_header, file_idx);
      String8List   file_path_split      = str8_split_path(scratch.arena, file_path);
      str8_path_list_resolve_dots_in_place(&file_path_split, PathStyle_WindowsAbsolute);
      String8       file_path_resolved   = str8_path_list_join_by_style(scratch.arena, &file_path_split, PathStyle_WindowsAbsolute);
      String8       file_path_normalized = lower_from_str8(scratch.arena, file_path_resolved);
      RDIM_SrcFile *src_file             = hash_table_search_path_raw(source_file_ht, file_path_normalized);
      if (src_file == 0) {
        src_file                   = rdim_src_file_chunk_list_push(arena, &src_files, SRC_FILE_CAP);
        src_file->normal_full_path = push_str8_copy(arena, file_path_normalized);
        hash_table_push_path_raw(scratch.arena, source_file_ht, src_file->normal_full_path, src_file);
      }
      src_file_map[file_idx] = src_file;
    }
    
    for (DW_LineSeqNode *line_seq = line_table->first_seq; line_seq != 0; line_seq = line_seq->next) {
      if (line_seq->count == 0) {
        continue;
      }
      
      U64 *voffs     = push_array(arena, U64, line_seq->count);
      U32 *line_nums = push_array(arena, U32, line_seq->count);
      U16 *col_nums  = 0;
      U64  line_idx  = 0;
      
      DW_LineNode *file_line_n     = line_seq->first;
      U64          file_line_count = 0;
      
      for (DW_LineNode *line_n = file_line_n; line_n != 0; line_n = line_n->next) {
        if (file_line_n->v.file_index != line_n->v.file_index || line_n->next == 0) {
          U64  file_index     = file_line_n->v.file_index;
          U64 *file_voffs     = &voffs[line_idx];
          U32 *file_line_nums = &line_nums[line_idx];
          U16 *file_col_nums  = 0;
          
          U64          lines_written = 0;
          U64          prev_ln       = max_U64;
          DW_LineNode *sentinel      = line_n->v.file_index != file_line_n->v.file_index ? line_n : 0;
          for (; file_line_n != sentinel; file_line_n = file_line_n->next) {
            if (file_line_n->v.line != prev_ln) {
              // TODO: error handling
              AssertAlways(file_line_n->v.address >= image_base);
              
              voffs[line_idx]     = file_line_n->v.address - image_base;
              line_nums[line_idx] = file_line_n->v.line;
              
              ++lines_written;
              ++line_idx;
              
              prev_ln = file_line_n->v.line;
            }
          }
          
          RDIM_SrcFile      *src_file = src_file_map[file_index];
          RDIM_LineSequence *line_seq = rdim_line_table_push_sequence(arena, &line_tables, cu_line_tables_rdi[cu_idx], src_file, file_voffs, file_line_nums, file_col_nums, lines_written);
          rdim_src_file_push_line_sequence(arena, &src_files, src_file, line_seq);
          
          file_line_count = 1;
        } else {
          ++file_line_count;
        }
      }
      
      // handle last line
      if (file_line_n) {
        U64  file_index     = file_line_n->v.file_index;
        U64 *file_voffs     = &voffs[line_idx];
        U32 *file_line_nums = &line_nums[line_idx];
        U16 *file_col_nums  = 0;
        
        for (; file_line_n != 0; file_line_n = file_line_n->next, ++line_idx) {
          // TODO: error handling
          AssertAlways(file_line_n->v.address >= image_base);
          voffs[line_idx]     = file_line_n->v.address - image_base;
          line_nums[line_idx] = file_line_n->v.line;
        }
        
        RDIM_SrcFile      *src_file = src_file_map[file_index];
        RDIM_LineSequence *line_seq = rdim_line_table_push_sequence(arena, &line_tables, cu_line_tables_rdi[cu_idx], src_file, file_voffs, file_line_nums, file_col_nums, file_line_count);
        rdim_src_file_push_line_sequence(arena, &src_files, src_file, line_seq);
      }
      
      //Assert(line_idx == line_seq->count);
    }
  }
  
  ProfEnd();
  
  //////////////////////////////// 
  
  ProfBegin("Convert Units");
  
  for (U64 cu_idx = 0; cu_idx < cu_ranges.count; ++cu_idx) {
    Temp comp_temp = temp_begin(scratch.arena);
    
    DW_CompUnit *cu = &cu_arr[cu_idx];
    
    // parse and build tag tree
    DW_TagTree tag_tree = dw_tag_tree_from_cu(comp_temp.arena, &input, cu);
    
    // build tag hash table for abstract origin resolution
    cu->tag_ht = dw_make_tag_hash_table(comp_temp.arena, tag_tree);
    
    String8 dwo_name     = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_DwoName);
    String8 gnu_dwo_name = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_GNU_DwoName);
    if (dwo_name.size || gnu_dwo_name.size || cu->dwo_id) {
      // TODO: report that we dont support DWO
      continue;
    }
    
    // get unit's contribution ranges
    RDIM_Rng1U64List cu_voff_ranges = d2r_voff_ranges_from_cu_info_off(cu_contrib_map, cu_ranges.v[cu_idx].min);
    
    String8     cu_name      = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_Name);
    String8     cu_dir       = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_CompDir);
    String8     cu_prod      = dw_string_from_attrib(&input, cu, cu->tag, DW_Attrib_Producer);
    DW_Language cu_lang      = dw_const_u64_from_attrib(&input, cu, cu->tag, DW_Attrib_Language);
    
    RDIM_Unit *unit     = rdim_unit_chunk_list_push(arena, &units, UNIT_CHUNK_CAP);
    unit->unit_name     = cu_name;
    unit->compiler_name = cu_prod;
    unit->source_file   = str8_zero();
    unit->object_file   = str8_zero();
    unit->archive_file  = str8_zero();
    unit->build_path    = cu_dir;
    unit->language      = rdi_language_from_dw_language(cu_lang);
    unit->line_table    = cu_line_tables_rdi[cu_idx];
    unit->voff_ranges   = cu_voff_ranges;
    
    D2R_TypeTable *type_table   = push_array(comp_temp.arena, D2R_TypeTable, 1);
    type_table->ht              = hash_table_init(comp_temp.arena, 0x4000);
    type_table->types           = &types;
    type_table->type_chunk_cap  = TYPE_CHUNK_CAP;
    type_table->varg_type       = d2r_create_type(arena, type_table);
    type_table->varg_type->kind = RDI_TypeKind_Variadic;
    
    D2R_TagNode *free_tags = push_array(comp_temp.arena, D2R_TagNode, 1);
    D2R_TagNode *tag_stack = push_array(comp_temp.arena, D2R_TagNode, 1);
    tag_stack->cur_node = tag_tree.root;
    
    while (tag_stack) {
      while (tag_stack->cur_node) {
        DW_TagNode *cur_node       = tag_stack->cur_node;
        DW_Tag      tag            = cur_node->tag;
        B32         visit_children = 1;
        
        switch (tag.kind) {
          case DW_Tag_Null: {
            InvalidPath;
          } break;
          case DW_Tag_ClassType: {
            RDIM_Type *type = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->name      = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            
            B32 is_decl = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_Declaration);
            if (is_decl) {
              type->kind = RDI_TypeKind_IncompleteClass;
              
              Assert(!cur_node->first_child);
              visit_children = 0;
            } else {
              RDIM_UDT *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
              udt->self_type = type;
              
              type->kind        = RDI_TypeKind_Class;
              type->byte_size   = dw_byte_size_32_from_tag(&input, cu, tag);
              type->udt         = udt;
              type->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
              
              tag_stack->type = type;
            }
          } break;
          case DW_Tag_StructureType: {
            RDIM_Type *type = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->name      = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            
            B32 is_decl = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_Declaration);
            if (is_decl) {
              type->kind = RDI_TypeKind_IncompleteStruct;
              
              // TODO: error handling
              Assert(!cur_node->first_child);
              visit_children = 0;
            } else {
              RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
              udt->self_type = type;
              
              type->kind      = RDI_TypeKind_Struct;
              type->udt       = udt;
              type->byte_size = dw_byte_size_32_from_tag(&input, cu, tag);
              
              tag_stack->type = type;
            }
          } break;
          case DW_Tag_UnionType: {
            RDIM_Type *type = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->name      = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            
            B32 is_decl = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_Declaration);
            if (is_decl) {
              type->kind = RDI_TypeKind_IncompleteUnion;
              
              // TODO: error handling
              Assert(!cur_node->first_child);
              visit_children = 0;
            } else {
              RDIM_UDT *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
              udt->self_type = type;
              
              type->kind      = RDI_TypeKind_Union;
              type->byte_size = dw_byte_size_32_from_tag(&input, cu, tag);
              type->udt       = udt;
              
              tag_stack->type = type;
            }
          } break;
          case DW_Tag_EnumerationType: {
            RDIM_Type *type = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->name      = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            
            B32 is_decl = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_Declaration);
            if (is_decl) {
              type->kind = RDI_TypeKind_IncompleteEnum;
              
              // TODO: error handling
              Assert(!cur_node->first_child);
              visit_children = 0;
            } else {
              RDIM_UDT *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
              udt->self_type = type;
              
              type->kind      = RDI_TypeKind_Enum;
              type->byte_size = dw_byte_size_32_from_tag(&input, cu, tag);
              type->udt       = udt;
              
              tag_stack->type = type;
            }
          } break;
          case DW_Tag_SubroutineType: {
            // collect parameters
            RDIM_TypeList param_list = {0};
            for (DW_TagNode *n = cur_node->first_child; n != 0; n = n->sibling) {
              if (n->tag.kind == DW_Tag_FormalParameter) {
                RDIM_Type *param_type = d2r_type_from_attrib(arena, type_table, &input, cu, n->tag, DW_Attrib_Type);
                rdim_type_list_push(comp_temp.arena, &param_list, param_type);
              } else if (n->tag.kind == DW_Tag_UnspecifiedParameters) {
                rdim_type_list_push(comp_temp.arena, &param_list, type_table->varg_type);
              } else {
                // TODO: error handling
                AssertAlways(!"unexpected tag");
              }
            }
            
            // init proceudre type
            RDIM_Type *ret_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            RDIM_Type *type     = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind          = RDI_TypeKind_Function;
            type->byte_size     = arch_addr_size;
            type->direct_type   = ret_type;
            type->count         = param_list.count;
            type->param_types   = rdim_array_from_type_list(arena, param_list);
            
            visit_children = 0;
          } break;
          case DW_Tag_Typedef: {
            RDIM_Type *type = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Alias;
            type->name        = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            type->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
          } break;
          case DW_Tag_BaseType: {
            DW_ATE encoding  = dw_const_u64_from_attrib(&input, cu, tag, DW_Attrib_Encoding);
            U64    byte_size = dw_byte_size_from_tag(&input, cu, tag);
            
            // convert base type encoding to RDI version
            RDI_TypeKind kind = RDI_TypeKind_NULL;
            switch (encoding) {
              case DW_ATE_Null:    kind = RDI_TypeKind_NULL; break;
              case DW_ATE_Address: kind = RDI_TypeKind_Void; break;
              case DW_ATE_Boolean: kind = RDI_TypeKind_Bool; break;
              case DW_ATE_ComplexFloat: {
                switch (byte_size) {
                  case 4:  kind = RDI_TypeKind_ComplexF32;  break;
                  case 8:  kind = RDI_TypeKind_ComplexF64;  break;
                  case 10: kind = RDI_TypeKind_ComplexF80;  break;
                  case 16: kind = RDI_TypeKind_ComplexF128; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
                }
              } break;
              case DW_ATE_Float: {
                switch (byte_size) {
                  case 2:  kind = RDI_TypeKind_F16;  break;
                  case 4:  kind = RDI_TypeKind_F32;  break;
                  case 6:  kind = RDI_TypeKind_F48;  break;
                  case 8:  kind = RDI_TypeKind_F64;  break;
                  case 16: kind = RDI_TypeKind_F128; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
                }
              } break;
              case DW_ATE_Signed: {
                switch (byte_size) {
                  case 1:  kind = RDI_TypeKind_S8;   break;
                  case 2:  kind = RDI_TypeKind_S16;  break;
                  case 4:  kind = RDI_TypeKind_S32;  break;
                  case 8:  kind = RDI_TypeKind_S64;  break;
                  case 16: kind = RDI_TypeKind_S128; break;
                  case 32: kind = RDI_TypeKind_S256; break;
                  case 64: kind = RDI_TypeKind_S512; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
                }
              } break;
              case DW_ATE_SignedChar: {
                switch (byte_size) {
                  case 1: kind = RDI_TypeKind_Char8;  break;
                  case 2: kind = RDI_TypeKind_Char16; break;
                  case 4: kind = RDI_TypeKind_Char32; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
                }
              } break;
              case DW_ATE_Unsigned: {
                switch (byte_size) {
                  case 1:  kind = RDI_TypeKind_U8;   break;
                  case 2:  kind = RDI_TypeKind_U16;  break;
                  case 4:  kind = RDI_TypeKind_U32;  break;
                  case 8:  kind = RDI_TypeKind_U64;  break;
                  case 16: kind = RDI_TypeKind_U128; break;
                  case 32: kind = RDI_TypeKind_U256; break;
                  case 64: kind = RDI_TypeKind_U512; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
                }
              } break;
              case DW_ATE_UnsignedChar: {
                switch (byte_size) {
                  case 1: kind = RDI_TypeKind_UChar8;  break;
                  case 2: kind = RDI_TypeKind_UChar16; break;
                  case 4: kind = RDI_TypeKind_UChar32; break;
                  default: AssertAlways(!"unexpected size"); break; // TODO: error handling
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
              case DW_ATE_DecimalFloat: {
                NotImplemented;
              } break;
              case DW_ATE_Utf: {
                NotImplemented;
              } break;
              case DW_ATE_Ucs: {
                NotImplemented;
              } break;
              case DW_ATE_Ascii: {
                NotImplemented;
              } break;
              default: AssertAlways(!"unexpected base type encoding"); break; // TODO: error handling
            }
            
            RDIM_Type *base_type = rdim_builtin_type_from_kind(types, kind);
            base_type->kind      = kind;
            base_type->byte_size = byte_size;
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Alias;
            type->name        = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            type->direct_type = base_type;
          } break;
          case DW_Tag_PointerType: {
            RDIM_Type *direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            
            // TODO:
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Allocated));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Associated));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Alignment));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Name));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_AddressClass));
            
            U64 byte_size = arch_addr_size;
            if (cu->version == DW_Version_5 || cu->relaxed) {
              dw_try_byte_size_from_tag(&input, cu, tag, &byte_size);
            }
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Ptr;
            type->byte_size   = byte_size;
            type->direct_type = direct_type;
          } break;
          case DW_Tag_RestrictType: {
            // TODO:
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Alignment));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Name));
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Modifier;
            type->byte_size   = arch_addr_size;
            type->flags       = RDI_TypeModifierFlag_Restrict;
            type->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
          } break;
          case DW_Tag_VolatileType: {
            // TODO:
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Name));
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Modifier;
            type->byte_size   = arch_addr_size;
            type->flags       = RDI_TypeModifierFlag_Volatile;
            type->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
          } break;
          case DW_Tag_ConstType: {
            // TODO:
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Name));
            Assert(!dw_tag_has_attrib(&input, cu, tag, DW_Attrib_Alignment));
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Modifier;
            type->byte_size   = arch_addr_size;
            type->flags       = RDI_TypeModifierFlag_Const;
            type->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
          } break;
          case DW_Tag_ArrayType: {
            // * DWARF vs RDI Array Type Graph *
            //
            // For example lets take following decl:
            //
            //    int (*foo[2])[3][4];
            // 
            //  This compiles to in DWARF:
            //  
            //  foo -> DW_TAG_ArrayType -> (A0) DW_TAG_Subrange [2]
            //                          \
            //                           -> (B0) DW_TAG_PointerType -> (A1) DW_TAG_ArrayType -> DW_TAG_Subrange [3] -> DW_Tag_Subrange [4]
            //                                                      \
            //                                                       -> (B1) DW_TAG_BaseType (int)
            // 
            // RDI expects:
            //  
            //  foo -> Array (2) -> Pointer -> Array (3) -> Array (4) -> int
            //
            // Note that DWARF forks the graph on DW_TAG_ArrayType to describe array ranges in branch A and
            // in branch B describes array type which might be a struct, pointer, base type, or any other type tag.
            // However, in RDI we have a simple list of type nodes and to convert we need to append type nodes from
            // B to A.
            
            RDIM_Type *type   = d2r_find_or_create_type_from_offset(arena, type_table, tag.info_off);
            type->kind        = RDI_TypeKind_Array;
            type->direct_type = 0;
            
            U64        subrange_count = 0;
            RDIM_Type *t              = type;
            for (DW_TagNode *n = cur_node->first_child; n != 0; n = n->sibling) {
              if (n->tag.kind != DW_Tag_SubrangeType) {
                // TODO: error handling
                AssertAlways(!"unexpected tag");
                continue;
              }
              
              if (subrange_count > 0) {
                // init array type node
                RDIM_Type *s   = d2r_create_type(arena, type_table);
                s->kind        = RDI_TypeKind_Array;
                s->direct_type = 0;
                
                // append new array type node
                t->direct_type = s;
                t = s;
              }
              
              // resolve array lower bound
              U64 lower_bound = 0;
              if (dw_tag_has_attrib(&input, cu, n->tag, DW_Attrib_LowerBound)) {
                lower_bound = dw_u64_from_attrib(&input, cu, n->tag, DW_Attrib_LowerBound);
              } else {
                lower_bound = dw_pick_default_lower_bound(cu_lang);
              }
              
              // resolve array upper bound
              U64 upper_bound = 0;
              if (dw_tag_has_attrib(&input, cu, n->tag, DW_Attrib_Count)) {
                U64 count = dw_u64_from_attrib(&input, cu, n->tag, DW_Attrib_Count);
                upper_bound = lower_bound + count;
              } else if (dw_tag_has_attrib(&input, cu, n->tag, DW_Attrib_UpperBound)) {
                upper_bound = dw_u64_from_attrib(&input, cu, n->tag, DW_Attrib_UpperBound);
                // turn upper bound into exclusive range
                upper_bound += 1;
              } else {
                // zero size array
              }
              
              t->count = upper_bound - lower_bound;
              ++subrange_count;
            }
            
            Assert(t->direct_type == 0);
            t->direct_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            
            visit_children = 0;
          } break;
          case DW_Tag_SubrangeType: {
            // TODO: error handling
            AssertAlways(!"unexpected tag");
          } break;
          case DW_Tag_Inheritance: {
            DW_TagNode *parent_node = tag_stack->next->cur_node;
            if (parent_node->tag.kind != DW_Tag_StructureType &&
                parent_node->tag.kind != DW_Tag_ClassType) {
              // TODO: error handling
              AssertAlways(!"unexpected parent tag");
            }
            
            RDIM_Type      *parent = tag_stack->next->type;
            RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, parent->udt);
            member->kind           = RDI_MemberKind_Base;
            member->type           = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            member->off            = safe_cast_u32(dw_const_u32_from_attrib(&input, cu, tag, DW_Attrib_DataMemberLocation));
          } break;
          case DW_Tag_Enumerator: {
            DW_TagNode *parent_node = tag_stack->next->cur_node;
            if (parent_node->tag.kind != DW_Tag_EnumerationType) {
              // TODO: error handling
              AssertAlways(!"unexpected parent tag");
            }
            
            RDIM_Type       *type   = tag_stack->next->type;
            RDIM_UDTEnumVal *member = rdim_udt_push_enum_val(arena, &udts, type->udt);
            member->name            = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            member->val             = dw_const_u64_from_attrib(&input, cu, tag, DW_Attrib_ConstValue);
          } break;
          case DW_Tag_Member: {
            DW_TagNode *parent_node = tag_stack->next->cur_node;
            if (parent_node->tag.kind != DW_Tag_StructureType &&
                parent_node->tag.kind != DW_Tag_ClassType     &&
                parent_node->tag.kind != DW_Tag_UnionType     &&
                parent_node->tag.kind != DW_Tag_EnumerationType) {
              // TODO: error handling
              AssertAlways(!"unexpected parent tag");
            }
            
            DW_Attrib      *data_member_location       = dw_attrib_from_tag(&input, cu, tag, DW_Attrib_DataMemberLocation);
            DW_AttribClass  data_member_location_class = dw_value_class_from_attrib(cu, data_member_location);
            if (data_member_location_class == DW_AttribClass_LocList) {
              AssertAlways(!"UDT member with multiple locations are not supported");
            }
            
            RDIM_Type      *type   = tag_stack->next->type;
            RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, type->udt);
            member->kind           = RDI_MemberKind_DataField;
            member->name           = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            member->type           = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            member->off            = dw_const_u64_from_attrib(&input, cu, tag, DW_Attrib_DataMemberLocation);
          } break;
          case DW_Tag_SubProgram: {
            DW_InlKind inl = dw_u64_from_attrib(&input, cu, tag, DW_Attrib_Inline);
            switch (inl) {
              case DW_Inl_NotInlined: {
                U64         param_count = 0;
                RDIM_Type **params      = d2r_collect_proc_params(arena, type_table, &input, cu, cur_node, &param_count);
                
                // get return type
                RDIM_Type *ret_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
                
                // fill out proc type
                RDIM_Type *proc_type   = d2r_create_type(arena, type_table);
                proc_type->kind        = RDI_TypeKind_Function;
                proc_type->byte_size   = arch_addr_size;
                proc_type->direct_type = ret_type;
                proc_type->count       = param_count;
                proc_type->param_types = params;
                
                // get container type
                RDIM_Type *container_type = 0;
                if (dw_tag_has_attrib(&input, cu, tag, DW_Attrib_ContainingType)) {
                  container_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_ContainingType);
                }
                
                // get frame base expression
                String8 frame_base_expr = dw_exprloc_from_attrib(&input, cu, tag, DW_Attrib_FrameBase);
                
                // get proc container symbol
                RDIM_Symbol *proc = rdim_symbol_chunk_list_push(arena, &procs,  PROC_CHUNK_CAP );
                
                // make scope
                Rng1U64List  ranges     = d2r_range_list_from_tag(comp_temp.arena, &input, cu, image_base, tag);
                RDIM_Scope  *root_scope = d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, tag_stack, ranges);
                root_scope->symbol      = proc;
                
                // fill out proc
                proc->is_extern        = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_External);
                proc->name             = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
                proc->link_name        = dw_string_from_attrib(&input, cu, tag, DW_Attrib_LinkageName);
                proc->type             = proc_type;
                proc->container_symbol = 0;
                proc->container_type   = container_type;
                proc->root_scope       = root_scope;
                proc->frame_base       = d2r_locset_from_attrib(arena, &input, cu, &scopes, root_scope, image_base, arch, tag, DW_Attrib_FrameBase);
                
                // sub program with user-defined parent tag is a method
                DW_TagKind parent_tag_kind = tag_stack->next->cur_node->tag.kind;
                if (parent_tag_kind == DW_Tag_ClassType || parent_tag_kind == DW_Tag_StructureType) {
                  RDI_MemberKind    member_kind = RDI_MemberKind_NULL;
                  DW_VirtualityKind virtuality  = dw_const_u64_from_attrib(&input, cu, tag, DW_Attrib_Virtuality);
                  switch (virtuality) {
                    case DW_VirtualityKind_None:        member_kind = RDI_MemberKind_Method;        break;
                    case DW_VirtualityKind_Virtual:     member_kind = RDI_MemberKind_VirtualMethod; break;
                    case DW_VirtualityKind_PureVirtual: member_kind = RDI_MemberKind_VirtualMethod; break; // TODO: create kind for pure virutal
                    //default: InvalidPath; break;
                  }
                  
                  RDIM_Type      *type   = tag_stack->next->type;
                  RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, type->udt);
                  member->kind           = member_kind;
                  member->type           = type;
                  member->name           = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
                } else if (parent_tag_kind != DW_Tag_CompileUnit) {
                  //AssertAlways(!"unexpected tag");
                }
                
                tag_stack->scope = root_scope;
              } break;
              case DW_Inl_DeclaredNotInlined:
              case DW_Inl_DeclaredInlined:
              case DW_Inl_Inlined: {
                visit_children = 0;
              } break;
              default: InvalidPath; break;
            }
          } break;
          case DW_Tag_InlinedSubroutine: {
            U64         param_count = 0;
            RDIM_Type **params      = d2r_collect_proc_params(arena, type_table, &input, cu, tag_stack->cur_node, &param_count);
            
            // get return type
            RDIM_Type *ret_type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            
            // fill out proc type
            RDIM_Type *proc_type   = d2r_create_type(arena, type_table);
            proc_type->kind        = RDI_TypeKind_Function;
            proc_type->byte_size   = arch_addr_size;
            proc_type->direct_type = ret_type;
            proc_type->count       = param_count;
            proc_type->param_types = params;
            
            // get container type
            RDIM_Type *owner = 0;
            if (dw_tag_has_attrib(&input, cu, tag, DW_Attrib_ContainingType)) {
              owner = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_ContainingType);
            }
            
            // fill out inline site
            RDIM_InlineSite *inline_site = rdim_inline_site_chunk_list_push(arena, &inline_sites, INLINE_SITE_CHUNK_CAP);
            inline_site->name            = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            inline_site->type            = proc_type;
            inline_site->owner           = owner;
            inline_site->line_table      = 0;
            
            // make scope
            Rng1U64List  ranges     = d2r_range_list_from_tag(comp_temp.arena, &input, cu, image_base, tag);
            RDIM_Scope  *root_scope = d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, tag_stack, ranges);
            root_scope->inline_site = inline_site;
          } break;
          case DW_Tag_Variable: {
            String8    name = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
            RDIM_Type *type = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
            
            DW_TagKind parent_tag_kind = tag_stack->next->cur_node->tag.kind;
            if (parent_tag_kind == DW_Tag_SubProgram ||
                parent_tag_kind == DW_Tag_InlinedSubroutine ||
                parent_tag_kind == DW_Tag_LexicalBlock) {
              RDIM_Scope *scope = tag_stack->next->scope;
              RDIM_Local *local = rdim_scope_push_local(arena, &scopes, tag_stack->next->scope);
              local->kind       = RDI_LocalKind_Variable;
              local->name       = name;
              local->type       = type;
              local->locset     = d2r_var_locset_from_tag(arena, &input, cu, &scopes, scope, image_base, arch, tag);
            } else {
              
              // NOTE: due to a bug in clang in stb_sprint.h local variables
              // are declared in global scope without a name
              if (name.size == 0) {
                break;
              }
              
              RDIM_Symbol *gvar      = rdim_symbol_chunk_list_push(arena, &gvars, GVAR_CHUNK_CAP);
              gvar->is_extern        = dw_flag_from_attrib(&input, cu, tag, DW_Attrib_External);
              gvar->name             = name;
              gvar->link_name        = dw_string_from_attrib(&input, cu, tag, DW_Attrib_LinkageName);
              gvar->type             = type;
              //gvar->locset           = d2r_locset_from_attrib(arena, &input, cu, &scopes, global_scope, image_base, arch, tag, DW_Attrib_Location);
              gvar->container_symbol = 0;
              gvar->container_type   = 0; // TODO: NotImplemented;
            }
          } break;
          case DW_Tag_FormalParameter: {
            DW_TagKind parent_tag_kind = tag_stack->next->cur_node->tag.kind;
            if (parent_tag_kind == DW_Tag_SubProgram || parent_tag_kind == DW_Tag_InlinedSubroutine) {
              RDIM_Scope *scope = tag_stack->next->scope;
              RDIM_Local *param = rdim_scope_push_local(arena, &scopes, scope);
              param->kind       = RDI_LocalKind_Parameter;
              param->name       = dw_string_from_attrib(&input, cu, tag, DW_Attrib_Name);
              param->type       = d2r_type_from_attrib(arena, type_table, &input, cu, tag, DW_Attrib_Type);
              param->locset     = d2r_var_locset_from_tag(arena, &input, cu, &scopes, scope, image_base, arch, tag);
            } else {
              // TODO: error handling
              AssertAlways(!"this is a local variable");
            }
          } break;
          case DW_Tag_LexicalBlock: {
            if (tag_stack->next->cur_node->tag.kind == DW_Tag_SubProgram ||
                tag_stack->next->cur_node->tag.kind == DW_Tag_InlinedSubroutine ||
                tag_stack->next->cur_node->tag.kind == DW_Tag_LexicalBlock) {
              Rng1U64List ranges = d2r_range_list_from_tag(comp_temp.arena, &input, cu, image_base, tag);
              d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, tag_stack, ranges);
            }
          } break;
          case DW_Tag_CallSite: {
            // TODO
          } break;
          case DW_Tag_CallSiteParameter: {
            // TODO
          } break;
          case DW_Tag_Label:
          case DW_Tag_CompileUnit:
          case DW_Tag_UnspecifiedParameters:
          break;
          case DW_Tag_Namespace: break;
          case DW_Tag_ImportedDeclaration: break;
          case DW_Tag_PtrToMemberType: break;
          case DW_Tag_TemplateTypeParameter: break;
          case DW_Tag_ReferenceType: break;
          default: NotImplemented; break;
        }
        
        if (tag_stack->cur_node->first_child && visit_children) {
          D2R_TagNode *frame = free_tags;
          if (frame) {
            SLLStackPop(free_tags);
            MemoryZeroStruct(frame);
          } else {
            frame = push_array(scratch.arena, D2R_TagNode, 1);
          }
          frame->cur_node = tag_stack->cur_node->first_child;
          SLLStackPush(tag_stack, frame);
        } else {
          tag_stack->cur_node = tag_stack->cur_node->sibling;
        }
      }
      
      // recycle free frame
      D2R_TagNode *frame = tag_stack;
      SLLStackPop(tag_stack);
      SLLStackPush(free_tags, frame);
      
      if (tag_stack) {
        tag_stack->cur_node = tag_stack->cur_node->sibling;
      }
    }
    
    temp_end(comp_temp);
  }
  
  ProfEnd();
  
  {
    for (RDIM_TypeChunkNode *chunk_n = types.first; chunk_n != 0; chunk_n = chunk_n->next) {
      for (U64 i = 0; i < chunk_n->count; ++i) {
        RDIM_Type *type = &chunk_n->v[i];
        if (type->kind == RDI_TypeKind_Alias) {
          for (RDIM_Type *t = type->direct_type; t != 0; t = t->direct_type) {
            if (t->byte_size != 0) {
              type->byte_size = t->byte_size;
              break;
            }
          }
        }
      }
    }
  }
  
  {
    RDIM_TypeNode *type_stack = 0;
    RDIM_TypeNode *free_types = 0;
    
    for (RDIM_TypeChunkNode *chunk_n = types.first; chunk_n != 0; chunk_n = chunk_n->next) {
      for (U64 i = 0; i < chunk_n->count; ++i) {
        RDIM_Type *type = &chunk_n->v[i];
        if (type->kind == RDI_TypeKind_Array) {
          if (type->byte_size != 0)
            continue;
          
          RDIM_Type *t;
          for (t = type; t != 0 && t->kind == RDI_TypeKind_Array; t = t->direct_type) {
            RDIM_TypeNode *f = free_types;
            if (f == 0) {
              f = push_array(scratch.arena, RDIM_TypeNode, 1);
            } else {
              SLLStackPop(free_types);
            }
            f->v = t;
            SLLStackPush(type_stack, f);
          }
          
          U64 base_type_size = 0;
          if (t) {
            base_type_size = t->byte_size;
          }
          
          U64 array_size = base_type_size;
          while (type_stack) {
            if (type_stack->v->count) {
              array_size *= type_stack->v->count;
            } else {
              array_size += type_stack->v->byte_size;
            }
            SLLStackPop(type_stack);
          }
          
          type->count     = 0;
          type->byte_size = array_size;
          
          // recycle frames
          free_types = type_stack;
          type_stack = 0;
        }
      }
    }
  }
  
  //////////////////////////////// 
  
  RDIM_BakeParams *bake_params  = push_array(arena, RDIM_BakeParams, 1);
  bake_params->top_level_info   = top_level_info;
  bake_params->binary_sections  = binary_sections;
  bake_params->units            = units;
  bake_params->types            = types;
  bake_params->udts             = udts;
  bake_params->src_files        = src_files;
  bake_params->line_tables      = line_tables;
  bake_params->global_variables = gvars;
  bake_params->thread_variables = tvars;
  bake_params->procedures       = procs;
  bake_params->scopes           = scopes;
  bake_params->inline_sites     = inline_sites;
  
  scratch_end(scratch);
  return bake_params;
}

internal RDI_Language
rdi_language_from_dw_language(DW_Language v)
{
  RDI_Language result = RDI_Language_NULL;
  switch (v) {
    case DW_Language_Null: result = RDI_Language_NULL; break;
    
    case DW_Language_C89:
    case DW_Language_C99:
    case DW_Language_C11:
    case DW_Language_C:
    result = RDI_Language_C;
    break;
    
    case DW_Language_CPlusPlus03:
    case DW_Language_CPlusPlus11:
    case DW_Language_CPlusPlus14:
    case DW_Language_CPlusPlus:
    result = RDI_Language_CPlusPlus;
    break;
    
    default: NotImplemented; break;
  }
  return result;
}

internal RDI_RegCodeX86
rdi_reg_from_dw_reg_x86(DW_RegX86 v)
{
  RDI_RegCodeX86 result = RDI_RegCode_nil;
  switch (v) {
#define X(reg_dw, val_dw, reg_rdi, ...) case DW_RegX86_##reg_dw: result = RDI_RegCodeX86_##reg_rdi; break;
    DW_Regs_X86_XList(X)
#undef X
    default: NotImplemented; break;
  }
  return result;
}

internal B32
rdi_reg_from_dw_reg_x64(DW_RegX64 v, RDI_RegCodeX64 *code_out, U64 *off_out, U64 *size_out)
{
  RDI_RegCodeX64 result = RDI_RegCode_nil;
  switch (v) {
#define X(reg_dw, val_dw, reg_rdi, off, size) case DW_RegX64_##reg_dw: result = RDI_RegCodeX64_##reg_rdi; *off_out = off; *size_out = size; break;
    DW_Regs_X64_XList(X)
#undef X
    default: NotImplemented; break;
  }
  return result;
}

internal B32
rdi_reg_from_dw_reg(Arch arch, DW_Reg v, RDI_RegCode *code_out, U64 *off_out, U64 *size_out)
{
  RDI_RegCode result = RDI_RegCode_nil;
  switch (arch) {
    case Arch_Null: break;
    case Arch_x86: ; break;
    case Arch_x64: return rdi_reg_from_dw_reg_x64(v, code_out, off_out, size_out);
    default: NotImplemented; break;
  }
  return 0;
}

