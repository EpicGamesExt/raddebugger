// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO:
//
// [ ] Currently converter relies on clang's -gdwarf-aranges to generate compile unit ranges,
//     however it is optional and in case it is missing converter has to generate the ranges from scopes.
// [ ] Error handling

////////////////////////////////

static const U64 UNIT_CHUNK_CAP        = 256;
static const U64 UDT_CHUNK_CAP         = 256;
static const U64 TYPE_CHUNK_CAP        = 256;
static const U64 SRC_FILE_CAP          = 256;
static const U64 LINE_TABLE_CAP        = 256;
static const U64 LOCATIONS_CAP         = 256;
static const U64 GVAR_CHUNK_CAP        = 256;
static const U64 TVAR_CHUNK_CAP        = 256;
static const U64 PROC_CHUNK_CAP        = 256;
static const U64 SCOPE_CHUNK_CAP       = 256;
static const U64 INLINE_SITE_CHUNK_CAP = 256;

RDIM_TopLevelInfo        top_level_info  = {0};
RDIM_BinarySectionList   binary_sections = {0};
RDIM_UnitChunkList       units           = {0};
RDIM_UDTChunkList        udts            = {0};
RDIM_TypeChunkList       types           = {0};
RDIM_SrcFileChunkList    src_files       = {0};
RDIM_LineTableChunkList  line_tables     = {0};
RDIM_LocationChunkList   locations       = {0};
RDIM_SymbolChunkList     gvars           = {0};
RDIM_SymbolChunkList     tvars           = {0};
RDIM_SymbolChunkList     procs           = {0};
RDIM_ScopeChunkList      scopes          = {0};
RDIM_InlineSiteChunkList inline_sites    = {0};

////////////////////////////////

internal B32
rdim_is_eval_bytecode_static(RDIM_EvalBytecode bc)
{
  B32 is_static = 1;
  RDI_EvalOp dynamic_ops[] = { RDI_EvalOp_MemRead, RDI_EvalOp_RegRead, RDI_EvalOp_RegReadDyn, RDI_EvalOp_CFA };
  for EachNode (n, RDIM_EvalBytecodeOp, bc.first_op) {
    for EachIndex(i, ArrayCount(dynamic_ops)) {
      is_static = 0;
      goto exit;
    }
  }
  exit:;
  return is_static;
}

internal U64
rdim_do_static_bytecode_eval(RDIM_EvalBytecode bc, U64 image_base)
{
  NotImplemented;
  return 0;
}

////////////////////////////////
//~ rjf: Enum Conversion Helpers

internal RDI_Language
d2r_rdi_language_from_dw_language(DW_Language v)
{
  RDI_Language result = RDI_Language_NULL;
  switch(v)
  {
    default:{}break;
    
    case DW_Language_C89:
    case DW_Language_C99:
    case DW_Language_C11:
    case DW_Language_C:
    {
      result = RDI_Language_C;
    }break;
    
    case DW_Language_CPlusPlus03:
    case DW_Language_CPlusPlus11:
    case DW_Language_CPlusPlus14:
    case DW_Language_CPlusPlus:
    {
      result = RDI_Language_CPlusPlus;
    }break;
  }
  return result;
}

internal RDI_RegCodeX86
d2r_rdi_reg_code_from_dw_reg_x86(DW_RegX86 v)
{
  RDI_RegCodeX86 result = RDI_RegCode_nil;
  switch(v)
  {
    default:{}break;
#define X(reg_dw, val_dw, reg_rdi, ...) case DW_RegX86_##reg_dw: result = RDI_RegCodeX86_##reg_rdi; break;
    DW_Regs_X86_XList(X)
#undef X
  }
  return result;
}

internal RDI_RegCodeX64
d2r_rdi_reg_code_from_dw_reg_x64(DW_RegX64 v)
{
  RDI_RegCodeX64 result = RDI_RegCode_nil;
  switch(v)
  {
    default:{}break;
#define X(reg_dw, val_dw, reg_rdi, off, size) case DW_RegX64_##reg_dw:{result = RDI_RegCodeX64_##reg_rdi;}break;
    DW_Regs_X64_XList(X)
#undef X
  }
  return result;
}

internal RDI_RegCode
d2r_rdi_reg_code_from_dw_reg(Arch arch, DW_Reg v)
{
  RDI_RegCode result = RDI_RegCode_nil;
  switch(arch)
  {
    default:
    case Arch_Null:
    case Arch_x86:{result = d2r_rdi_reg_code_from_dw_reg_x86(v);}break;
    case Arch_x64:{result = d2r_rdi_reg_code_from_dw_reg_x64(v);}break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Type Conversion Helpers

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
      
      // TODO: support for external compile unit references
      AssertAlways(ref.cu == cu);
      
      // find type
      type = d2r_type_from_offset(type_table, ref.info_off);
    } else {
      Assert(!"unexpected attrib class");
    }
  }
  
  return type;
}

internal Rng1U64List
d2r_range_list_from_tag(Arena *arena, DW_Input *input, DW_CompUnit *cu, U64 image_base, DW_Tag tag)
{
  // collect non-contiguous range
  Rng1U64List ranges = dw_rnglist_from_tag_attrib_kind(arena, input, cu, tag, DW_AttribKind_Ranges);
  
  // debase ranges
  for EachNode(r, Rng1U64Node, ranges.first) {
    // TODO: error handling
    AssertAlways(r->v.min >= image_base);
    AssertAlways(r->v.max >= image_base);
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
      } else {
        AssertAlways(!"unexpected attribute encoding");
      }
      
      if (lo_pc >= image_base && hi_pc >= image_base) {
        if (lo_pc < hi_pc) {
          rng1u64_list_push(arena, &ranges, rng_1u64(lo_pc - image_base, hi_pc - image_base));
        } else {
          // TODO: error handling
        }
      } else {
        // invalid low and hi PC are likely are caused by an optimization pass during linking
      }
    } else if ((lo_pc_attrib->attrib_kind == DW_AttribKind_Null && hi_pc_attrib->attrib_kind != DW_AttribKind_Null) ||
               (lo_pc_attrib->attrib_kind != DW_AttribKind_Null && hi_pc_attrib->attrib_kind == DW_AttribKind_Null)) {
      // TODO: error handling
    }
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

////////////////////////////////
//~ rjf: Bytecode Conversion Helpers

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
        
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
        U32         regread_param = RDI_EncodeRegReadParam(reg_code_rdi, reg_size, reg_pos);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RegRead, regread_param);
        push_of_type(d2r_unsigned_type_kind_from_size(reg_size));
      } break;
      
      case DW_ExprOp_RegX: {
        U64 reg_code_dw = 0;
        cursor += str8_deserial_read_uleb128(expr, cursor, &reg_code_dw);
        
        U64 reg_size = dw_reg_size_from_code(arch, reg_code_dw);
        U64 reg_pos  = dw_reg_pos_from_code(arch, reg_code_dw);
        
        RDI_RegCode reg_code_rdi  = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
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
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
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
        
        RDI_RegCode reg_code_rdi = d2r_rdi_reg_code_from_dw_reg(arch, reg_code_dw);
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
          if (tag.kind == DW_TagKind_BaseType) {
            // extract encoding attribute
            DW_ATE encoding = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Encoding);
            
            // DW_ATE -> RDI_EvalTypeGroup
            switch (encoding) {
              case DW_ATE_SignedChar:
              case DW_ATE_Signed:   out = RDI_EvalTypeGroup_S; break;
              case DW_ATE_UnsignedChar:
              case DW_ATE_Unsigned: out = RDI_EvalTypeGroup_U; break;
              case DW_ATE_Float: {
                U64 byte_size = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ByteSize);
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
      
      case DW_ExprOp_GNU_PushTlsAddress: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, 0);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Sub, peek_type());
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_TLSOff, 0);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, peek_type());
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
d2r_transpile_expression(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr)
{
  RDIM_Location *loc = 0;
  if (expr.size) {
    B32               is_addr  = 0;
    RDIM_EvalBytecode bytecode = d2r_bytecode_from_expression(arena, input, image_base, address_size, arch, addr_lu, expr, cu, &is_addr);
    
    RDIM_LocationInfo *loc_info = push_array(arena, RDIM_LocationInfo, 1);
    loc_info->kind     = is_addr ? RDI_LocationKind_AddrBytecodeStream : RDI_LocationKind_ValBytecodeStream;
    loc_info->bytecode = bytecode;
    
    loc = rdim_location_chunk_list_push_new(arena, locations, LOCATIONS_CAP, loc_info);
  }
  return loc;
}

internal RDIM_Location *
d2r_location_from_attrib(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, DW_CompUnit *cu, U64 image_base, Arch arch, DW_Tag tag, DW_AttribKind kind)
{
  String8 expr = dw_exprloc_from_tag_attrib_kind(input, cu, tag, kind);
  RDIM_Location *location = d2r_transpile_expression(arena, locations, input, image_base, cu->address_size, arch, cu->addr_lu, cu, expr);
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
      RDIM_Location *location   = d2r_transpile_expression(arena, locations, input, image_base, cu->address_size, arch, cu->addr_lu, cu, loc_n->v.expr);
      RDIM_Rng1U64   voff_range = { .min = loc_n->v.range.min -  image_base, .max = loc_n->v.range.max - image_base };
      rdim_push_location_case(arena, scopes, &locset, location, voff_range);
    }
    
    scratch_end(scratch);
  } else if (attrib_class == DW_AttribClass_ExprLoc) {
    // extract expression from attrib
    String8 expr = dw_exprloc_from_attrib(input, cu, attrib);
    
    // convert expression and inherit life-time ranges from enclosed scope
    RDIM_Location *location = d2r_transpile_expression(arena, locations, input, image_base, cu->address_size, arch, cu->addr_lu, cu, expr);
    for EachNode(range_n, RDIM_Rng1U64Node, curr_scope->voff_ranges.first) {
      rdim_push_location_case(arena, scopes, &locset, location, range_n->v);
    }
  } else if (attrib_class != DW_AttribClass_Null) {
    AssertAlways(!"unexpected attrib class");
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
    // TODO: error handling
    AssertAlways(!"unexpected variable encoding");
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
    RDIM_Location *loc = rdim_location_chunk_list_push_new(arena, locations, LOCATIONS_CAP, loc_info);
    
    // push location cases
    for EachNode(range_n, RDIM_Rng1U64Node, curr_scope->voff_ranges.first) {
      rdim_push_location_case(arena, scopes, &locset, loc, range_n->v);
    }
  } else if (has_location) {
    locset = d2r_locset_from_attrib(arena, scopes, curr_scope, locations, input, cu, image_base, arch, tag, DW_AttribKind_Location);
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
  cm.info_off_arr           = push_array(arena, U64,                   unit_range_list.count);
  cm.voff_range_arr         = push_array(arena, RDIM_Rng1U64ChunkList, unit_range_list.count);
  
  for EachNode(range_n, Rng1U64Node, unit_range_list.first) {
    String8 unit_data = str8_substr(aranges_data, range_n->v);
    U64     unit_cursor    = 0;
    
    U64 unit_length = 0;
    U64 unit_length_size = str8_deserial_read_dwarf_packed_size(unit_data, unit_cursor, &unit_length);
    if (unit_length_size == 0) { continue; }
    unit_cursor += unit_length_size;
    
    DW_Version version = 0;
    U64 version_size = str8_deserial_read_struct(unit_data, unit_cursor, &version);
    if (version_size == 0) { continue; }
    unit_cursor += version;
    
    if (version != DW_Version_2) {
      AssertAlways(!"unknown .debug_aranges version");
      continue;
    }
    
    DW_Format unit_format      = DW_FormatFromSize(unit_length);
    U64       cu_info_off      = 0;
    U64       cu_info_off_size = str8_deserial_read_dwarf_uint(unit_data, unit_cursor, unit_format, &cu_info_off);
    if (cu_info_off_size == 0) { continue; }
    unit_cursor += cu_info_off_size;
    
    U8 address_size = 0;
    U64 address_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &address_size);
    if (address_size_size == 0) { continue; }
    unit_cursor += address_size_size;
    
    U8 segment_selector_size = 0;
    U64 segment_selector_size_size = str8_deserial_read_struct(unit_data, unit_cursor, &segment_selector_size);
    if (segment_selector_size_size == 0) { continue; }
    unit_cursor += segment_selector_size_size;
    
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
        unit_cursor += str8_deserial_read(unit_data, unit_cursor, &address, address_size, address_size);
        unit_cursor += str8_deserial_read(unit_data, unit_cursor, &length, address_size, address_size);
        
        if (address == 0 && length == 0) { break; }
        if (address == 0) { continue; }
        
        // TODO: error handling
        AssertAlways(address >= image_base);
        
        U64 min = address - image_base;
        U64 max = min + length;
        rdim_rng1u64_chunk_list_push(arena, &voff_ranges, 256, (RDIM_Rng1U64){.min = min, .max = max});
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

////////////////////////////////
//~ rjf: Compilation Unit / Scope Conversion Helpers

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

////////////////////////////////
//~ rjf: Main Conversion Entry Point

internal D2R_TagIterator *
d2r_tag_iterator_init(Arena *arena, DW_TagNode *root)
{
  D2R_TagIterator *iter = push_array(arena, D2R_TagIterator, 1);
  iter->free_list            = 0;
  iter->stack                = push_array(arena, D2R_TagFrame, 1);
  iter->stack->node          = push_array(arena, DW_TagNode, 1);
  *iter->stack->node         = *root;
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
d2r_flag_converted_tag(DW_TagNode *tag_node)
{
  tag_node->tag.v[0] = 1;
}

internal B8
d2r_is_tag_converted(DW_TagNode *tag_node)
{
  return tag_node->tag.v[0];
}

internal RDIM_Type *
d2r_find_or_convert_type(Arena *arena, D2R_TypeTable *type_table, DW_Input *input, DW_CompUnit *cu, DW_Language cu_lang, U64 arch_addr_size, DW_Tag tag, DW_AttribKind kind)
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
      
      // TODO: support for external compile unit references
      AssertAlways(ref.cu == cu);
      
      // find type
      type = d2r_type_from_offset(type_table, ref.info_off);
      
      // was type converted?
      if (type == 0) {
        // issue type conversion
        DW_TagNode *ref_node = dw_tag_node_from_info_off(cu, ref.info_off);
        d2r_convert_types(arena, type_table, input, cu, cu_lang, arch_addr_size, ref_node);
        
        // if we do not have a converted type at this point then debug info is malformed
        type = d2r_type_from_offset(type_table, ref.info_off);
        Assert(type);
      }
    } else {
      Assert(!"unexpected attrib class");
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
                  U64            arch_addr_size,
                  DW_TagNode    *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  for (D2R_TagIterator *it = d2r_tag_iterator_init(scratch.arena, root); it->tag_node != 0; d2r_tag_iterator_next(scratch.arena, it)) {
    DW_TagNode *tag_node = it->tag_node;
    DW_Tag      tag      = tag_node->tag;
    
    // skip converted tags
    if (d2r_is_tag_converted(tag_node)) {
      d2r_tag_iterator_skip_children(it);
      continue;
    }
    // mark the tag as converted here, because during conversion we may recurse on the same tag
    d2r_flag_converted_tag(tag_node);
    
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
          RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
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
          
          // TODO: error handling
          Assert(!tag_node->first_child);
          d2r_tag_iterator_skip_children(it);
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
          
          // TODO: error handling
          Assert(!tag_node->first_child);
          d2r_tag_iterator_skip_children(it);
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
          // TODO: error handling
          Assert(!tag_node->first_child);
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *enum_base_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
          RDIM_Type *type           = d2r_create_type_from_offset(arena, type_table, tag.info_off);
          type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          type->kind        = RDI_TypeKind_Enum;
          type->byte_size   = dw_byte_size_32_from_tag(input, cu, tag);
          type->direct_type = enum_base_type;
        }
      } break;
      case DW_TagKind_SubroutineType: {
        RDIM_Type *ret_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        
        // collect parameters
        RDIM_TypeList param_list = {0};
        for (DW_TagNode *n = tag_node->first_child; n != 0; n = n->sibling) {
          if (n->tag.kind == DW_TagKind_FormalParameter) {
            RDIM_Type *param_type = d2r_type_from_attrib(type_table, input, cu, n->tag, DW_AttribKind_Type);
            rdim_type_list_push(scratch.arena, &param_list, param_type);
          } else if (n->tag.kind == DW_TagKind_UnspecifiedParameters) {
            rdim_type_list_push(scratch.arena, &param_list, type_table->builtin_types[RDI_TypeKind_Variadic]);
          } else {
            // TODO: error handling
            AssertAlways(!"unexpected tag");
          }
        }
        
        // init proceudre type
        RDIM_Type *type     = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind          = RDI_TypeKind_Function;
        type->byte_size     = arch_addr_size;
        type->direct_type   = ret_type;
        type->count         = param_list.count;
        type->param_types   = rdim_array_from_type_list(arena, param_list);
        
        d2r_tag_iterator_skip_children(it);
      } break;
      case DW_TagKind_Typedef: {
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
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
        
        RDIM_Type *type   = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Alias;
        type->name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        type->direct_type = type_table->builtin_types[kind];
        type->byte_size   = byte_size;
      } break;
      case DW_TagKind_PointerType: {
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        
        // TODO:
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Allocated));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Associated));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_AddressClass));
        
        U64 byte_size = arch_addr_size;
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
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name));
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = arch_addr_size;
        type->flags       = RDI_TypeModifierFlag_Restrict;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_VolatileType: {
        // TODO:
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name));
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = arch_addr_size;
        type->flags       = RDI_TypeModifierFlag_Volatile;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_ConstType: {
        // TODO:
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name));
        Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Alignment));
        
        RDIM_Type *direct_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        RDIM_Type *type        = d2r_create_type_from_offset(arena, type_table, tag.info_off);
        type->kind        = RDI_TypeKind_Modifier;
        type->byte_size   = arch_addr_size;
        type->flags       = RDI_TypeModifierFlag_Const;
        type->direct_type = direct_type;
      } break;
      case DW_TagKind_ArrayType: {
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
            // TODO: error handling
            AssertAlways(!"unexpected tag");
            continue;
          }
          
          // resolve lower bound
          U64 lower_bound = 0;
          if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_LowerBound)) {
            lower_bound = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_LowerBound);
          } else {
            lower_bound = dw_pick_default_lower_bound(cu_lang);
          }
          
          // resolve upper bound
          U64 upper_bound = 0;
          if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_Count)) {
            U64 count = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_Count);
            upper_bound = lower_bound + count;
          } else if (dw_tag_has_attrib(input, cu, n->tag, DW_AttribKind_UpperBound)) {
            upper_bound = dw_u64_from_attrib(input, cu, n->tag, DW_AttribKind_UpperBound);
            // turn upper bound into exclusive range
            upper_bound += 1;
          } else {
            // zero sized array
          }
          
          struct SubrangeNode *s = push_array(scratch.arena, struct SubrangeNode, 1);
          s->count = upper_bound - lower_bound;
          SLLStackPush(subrange_stack, s);
        }
        
        RDIM_Type *array_base_type = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
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
        
        d2r_tag_iterator_skip_children(it);
      } break;
      case DW_TagKind_SubrangeType: {
        // TODO: error handling
        AssertAlways(!"unexpected tag");
      } break;
      case DW_TagKind_Inheritance: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind != DW_TagKind_StructureType && parent_tag.kind != DW_TagKind_ClassType) {
          // TODO: error handling
          AssertAlways(!"unexpected parent tag");
        }
        
        RDIM_Type      *parent = d2r_type_from_offset(type_table, parent_tag.info_off);
        RDIM_Type      *type   = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
        RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, parent->udt);
        member->kind           = RDI_MemberKind_Base;
        member->type           = type;
        member->off            = safe_cast_u32(dw_const_u32_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_DataMemberLocation));
      } break;
    }
  }
  scratch_end(scratch);
}

internal void
d2r_convert_udts(Arena         *arena,
                 D2R_TypeTable *type_table,
                 DW_Input      *input,
                 DW_CompUnit   *cu,
                 DW_Language    cu_lang,
                 U64            arch_addr_size,
                 DW_TagNode    *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  for (D2R_TagIterator *it = d2r_tag_iterator_init(scratch.arena, root); it->tag_node != 0; d2r_tag_iterator_next(scratch.arena, it)) {
    DW_TagNode *tag_node = it->tag_node;
    DW_Tag      tag      = tag_node->tag;
    switch (tag.kind) {
      case DW_TagKind_ClassType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
          RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
          udt->self_type = type;
          type->udt      = udt;
        }
      } break;
      case DW_TagKind_StructureType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
          RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
          udt->self_type = type;
          type->udt      = udt;
        }
      } break;
      case DW_TagKind_UnionType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
          RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
          udt->self_type = type;
          type->udt      = udt;
        }
      } break;
      case DW_TagKind_EnumerationType: {
        B32 is_decl = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Declaration);
        if (is_decl) {
          d2r_tag_iterator_skip_children(it);
        } else {
          RDIM_Type *type = d2r_type_from_offset(type_table, tag.info_off);
          RDIM_UDT  *udt  = rdim_udt_chunk_list_push(arena, &udts, UDT_CHUNK_CAP);
          udt->self_type = type;
          type->udt      = udt;
        }
      } break;
      case DW_TagKind_Member: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        B32 is_parent_udt = parent_tag.kind == DW_TagKind_StructureType ||
          parent_tag.kind == DW_TagKind_ClassType     ||
          parent_tag.kind == DW_TagKind_UnionType;
        if (is_parent_udt) {
          DW_Attrib      *data_member_location       = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_DataMemberLocation);
          DW_AttribClass  data_member_location_class = dw_value_class_from_attrib(cu, data_member_location);
          if (data_member_location_class == DW_AttribClass_LocList) {
            AssertAlways(!"UDT member with multiple locations are not supported");
          }
          
          RDIM_Type      *parent_type = d2r_type_from_offset(type_table, parent_tag.info_off);
          RDIM_UDTMember *udt_member  = rdim_udt_push_member(arena, &udts, parent_type->udt);
          udt_member->kind = RDI_MemberKind_DataField;
          udt_member->name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          udt_member->type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
          udt_member->off  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_DataMemberLocation);
        } else {
          // TODO: error handling
          AssertAlways(!"unexpected parent tag");
        }
      } break;
      case DW_TagKind_Enumerator: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_EnumerationType) {
          RDIM_Type       *parent_type = d2r_type_from_offset(type_table, parent_tag.info_off);
          RDIM_UDTEnumVal *udt_member  = rdim_udt_push_enum_val(arena, &udts, parent_type->udt);
          udt_member->name = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          udt_member->val  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_ConstValue);
        } else {
          // TODO: error handling
          AssertAlways(!"unexpected parent tag");
        }
      } break;
    }
  }
  scratch_end(scratch);
}

internal void
d2r_convert_symbols(Arena         *arena,
                    D2R_TypeTable *type_table,
                    RDIM_Scope    *global_scope,
                    DW_Input      *input,
                    DW_CompUnit   *cu,
                    DW_Language    cu_lang,
                    U64            arch_addr_size,
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
        DW_InlKind inl = dw_u64_from_attrib(input, cu, tag, DW_AttribKind_Inline);
        switch (inl) {
          case DW_Inl_NotInlined: {
            U64         param_count = 0;
            RDIM_Type **params      = d2r_collect_proc_params(arena, type_table, input, cu, tag_node, &param_count);
            
            // get return type
            RDIM_Type *ret_type = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
            
            // fill out proc type
            RDIM_Type *proc_type   = d2r_create_type(arena, type_table);
            proc_type->kind        = RDI_TypeKind_Function;
            proc_type->byte_size   = arch_addr_size;
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
            RDIM_Symbol *proc = rdim_symbol_chunk_list_push(arena, &procs,  PROC_CHUNK_CAP);
            
            // make scope
            Rng1U64List  ranges     = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
            RDIM_Scope  *root_scope = d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, it->stack, ranges);
            root_scope->symbol      = proc;
            
            // fill out proc
            proc->is_extern        = dw_flag_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_External);
            proc->name             = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            proc->link_name        = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_LinkageName);
            proc->type             = proc_type;
            proc->container_symbol = 0;
            proc->container_type   = container_type;
            proc->root_scope       = root_scope;
            proc->location_cases   = d2r_locset_from_attrib(arena, &scopes, root_scope, &locations, input, cu, image_base, arch, tag, DW_AttribKind_FrameBase);
            
            // sub program with user-defined parent tag is a method
            DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
            if (parent_tag.kind == DW_TagKind_ClassType || parent_tag.kind == DW_TagKind_StructureType) {
              RDI_MemberKind    member_kind = RDI_MemberKind_NULL;
              DW_VirtualityKind virtuality  = dw_const_u64_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Virtuality);
              switch (virtuality) {
                case DW_VirtualityKind_None:        member_kind = RDI_MemberKind_Method;        break;
                case DW_VirtualityKind_Virtual:     member_kind = RDI_MemberKind_VirtualMethod; break;
                case DW_VirtualityKind_PureVirtual: member_kind = RDI_MemberKind_VirtualMethod; break; // TODO: create kind for pure virutal
                //default: InvalidPath; break;
              }
              
              RDIM_Type      *type   = d2r_type_from_offset(type_table, parent_tag.info_off);
              RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, type->udt);
              member->kind           = member_kind;
              member->type           = type;
              member->name           = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
            } else if (parent_tag.kind != DW_TagKind_CompileUnit) {
              //AssertAlways(!"unexpected tag");
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
        proc_type->byte_size   = arch_addr_size;
        proc_type->direct_type = ret_type;
        proc_type->count       = param_count;
        proc_type->param_types = params;
        
        // get container type
        RDIM_Type *owner = 0;
        if (dw_tag_has_attrib(input, cu, tag, DW_AttribKind_ContainingType)) {
          owner = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_ContainingType);
        }
        
        // fill out inline site
        RDIM_InlineSite *inline_site = rdim_inline_site_chunk_list_push(arena, &inline_sites, INLINE_SITE_CHUNK_CAP);
        inline_site->name            = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
        inline_site->type            = proc_type;
        inline_site->owner           = owner;
        inline_site->line_table      = 0;
        
        // make scope
        Rng1U64List  ranges     = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
        RDIM_Scope  *root_scope = d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, it->stack, ranges);
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
          RDIM_Local *local = rdim_scope_push_local(arena, &scopes, scope);
          local->kind           = RDI_LocalKind_Variable;
          local->name           = name;
          local->type           = type;
          local->location_cases = d2r_var_locset_from_tag(arena, &scopes, scope, &locations, input, cu, image_base, arch, tag);
        } else {
          
          // NOTE: due to a bug in clang in stb_sprint.h local variables
          // are declared in global scope without a name
          if (name.size == 0) { break; }
          
          B32 is_thread_var = 0;
          U64 voff          = 0;
          {
            DW_Attrib      *loc_attrib = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_Location);
            DW_AttribClass  loc_class  = dw_value_class_from_attrib(cu, loc_attrib);
            if (loc_class == DW_AttribClass_ExprLoc) {
              String8           expr    = dw_exprloc_from_attrib(input, cu, loc_attrib);
              B32               is_addr = 0;
              RDIM_EvalBytecode bc      = d2r_bytecode_from_expression(arena, input, image_base, arch_addr_size, arch, cu->addr_lu, expr, cu, &is_addr);
              
              for EachNode(n, RDIM_EvalBytecodeOp, bc.first_op) {
                if (n->op == RDI_EvalOp_TLSOff) {
                  is_thread_var = 1;
                  break;
                }
              }
              
              if (is_addr) {
                if (rdim_is_eval_bytecode_static(bc)) {
                  voff = rdim_do_static_bytecode_eval(bc, image_base);
                }
              }
            }
          }
          
          RDIM_SymbolChunkList *var_chunks; U64 var_chunks_cap;
          if (is_thread_var) { var_chunks = &tvars; var_chunks_cap = TVAR_CHUNK_CAP; }
          else               { var_chunks = &gvars; var_chunks_cap = GVAR_CHUNK_CAP; }
          
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
          RDIM_Local *param = rdim_scope_push_local(arena, &scopes, scope);
          param->kind           = RDI_LocalKind_Parameter;
          param->name           = dw_string_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_Name);
          param->type           = d2r_type_from_attrib(type_table, input, cu, tag, DW_AttribKind_Type);
          param->location_cases = d2r_var_locset_from_tag(arena, &scopes, scope, &locations, input, cu, image_base, arch, tag);
        } else {
          // TODO: error handling
          AssertAlways(!"this is a local variable");
        }
      } break;
      case DW_TagKind_LexicalBlock: {
        DW_Tag parent_tag = d2r_tag_iterator_parent_tag(it);
        if (parent_tag.kind == DW_TagKind_SubProgram ||
            parent_tag.kind == DW_TagKind_InlinedSubroutine ||
            parent_tag.kind == DW_TagKind_LexicalBlock) {
          Rng1U64List ranges = d2r_range_list_from_tag(scratch.arena, input, cu, image_base, tag);
          d2r_push_scope(arena, &scopes, SCOPE_CHUNK_CAP, it->stack, ranges);
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
      default: NotImplemented; break;
    }
  }
  scratch_end(scratch);
}

internal RDIM_BakeParams
d2r_convert(Arena *arena, D2R_ConvertParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  if (lane_idx() == 0) {
    ////////////////////////////////
    
    ProfBegin("compute exe hash");
    U64 exe_hash = rdi_hash(params->exe_data.str, params->exe_data.size);
    ProfEnd();
    
    ////////////////////////////////
    
    Arch     arch       = Arch_Null;
    U64      image_base = 0;
    DW_Input input      = {0};
    
    switch(params->exe_kind) {
      default:{}break;
      case ExecutableImageKind_CoffPe: {
        PE_BinInfo          pe            = pe_bin_info_from_data(scratch.arena, params->exe_data);
        String8             raw_sections  = str8_substr(params->exe_data, pe.section_table_range);
        COFF_SectionHeader *section_table = str8_deserial_get_raw_ptr(raw_sections, 0, sizeof(COFF_SectionHeader) * pe.section_count);
        String8             string_table  = str8_substr(params->exe_data, pe.string_table_range);
        arch            = pe.arch;
        image_base      = pe.image_base;
        binary_sections = c2r_rdi_binary_sections_from_coff_sections(arena, params->exe_data, string_table, pe.section_count, section_table);
        input           = dw_input_from_coff_section_table(scratch.arena, params->exe_data, string_table, pe.section_count, section_table);
      } break;
      case ExecutableImageKind_Elf32:
      case ExecutableImageKind_Elf64: {
        ELF_Bin bin = elf_bin_from_data(scratch.arena, params->dbg_data);
        arch            = arch_from_elf_machine(bin.hdr.e_machine);
        image_base      = elf_base_addr_from_bin(&bin);
        binary_sections = e2r_rdi_binary_sections_from_elf_section_table(arena, bin.shdrs);
        input           = dw_input_from_elf_bin(scratch.arena, params->dbg_data, &bin);
      } break;
    }
    
    ////////////////////////////////
    
    top_level_info = rdim_make_top_level_info(params->exe_name, arch, exe_hash, binary_sections);
    
    ////////////////////////////////
    
    U64 arch_addr_size = rdi_addr_size_from_arch(top_level_info.arch);
    
    ////////////////////////////////
    
    RDIM_Scope *global_scope = rdim_scope_chunk_list_push(arena, &scopes, SCOPE_CHUNK_CAP);
    
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
    // TODO(rjf): parse should always be relaxed. any verification checks we do
    // should just be logged via log_info(...), and then the caller of this
    // converter can collect those & display as necessary.
    B32 is_parse_relaxed = 1;
    DW_CompUnit *cu_arr = push_array(scratch.arena, DW_CompUnit, cu_ranges.count);
    for EachIndex(cu_idx, cu_ranges.count) {
      cu_arr[cu_idx] = dw_cu_from_info_off(scratch.arena, &input, lu_input, cu_ranges.v[cu_idx].min, is_parse_relaxed);
    }
    ProfEnd();
    
    ////////////////////////////////
    
    ProfBegin("Parse Line Tables");
    DW_LineTableParseResult *cu_line_tables = push_array(scratch.arena, DW_LineTableParseResult, cu_ranges.count);
    for EachIndex(cu_idx, cu_ranges.count) {
      DW_CompUnit *cu           = &cu_arr[cu_idx];
      String8      cu_stmt_list = dw_line_ptr_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_StmtList);
      String8      cu_dir       = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_CompDir);
      String8      cu_name      = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_Name);
      cu_line_tables[cu_idx] = dw_parsed_line_table_from_data(scratch.arena, cu_stmt_list, &input, cu_dir, cu_name, cu->address_size, cu->str_offsets_lu);
    }
    ProfEnd();
    
    ////////////////////////////////
    
    ProfBegin("Convert Line Tables");
    HashTable       *source_file_ht     = hash_table_init(scratch.arena, 0x4000);
    RDIM_LineTable **cu_line_tables_rdi = push_array(scratch.arena, RDIM_LineTable *, cu_ranges.count);
    for EachIndex(cu_idx, cu_ranges.count) {
      cu_line_tables_rdi[cu_idx] = rdim_line_table_chunk_list_push(arena, &line_tables, LINE_TABLE_CAP);
      
      DW_LineTableParseResult *line_table   = &cu_line_tables[cu_idx];
      DW_LineVMFileArray      *dir_table    = &line_table->vm_header.dir_table;
      DW_LineVMFileArray      *file_table   = &line_table->vm_header.file_table;
      RDIM_SrcFile           **src_file_map = push_array(scratch.arena, RDIM_SrcFile *, file_table->count);
      for EachIndex(file_idx, file_table->count) {
        DW_LineFile  *file                 = &file_table->v[file_idx];
        String8       file_path            = dw_path_from_file_idx(scratch.arena, &line_table->vm_header, file_idx);
        String8List   file_path_split      = str8_split_path(scratch.arena, file_path);
        str8_path_list_resolve_dots_in_place(&file_path_split, PathStyle_WindowsAbsolute);
        String8       file_path_resolved   = str8_path_list_join_by_style(scratch.arena, &file_path_split, PathStyle_WindowsAbsolute);
        RDIM_SrcFile *src_file             = hash_table_search_path_raw(source_file_ht, file_path_resolved);
        if (src_file == 0) {
          src_file       = rdim_src_file_chunk_list_push(arena, &src_files, SRC_FILE_CAP);
          src_file->path = push_str8_copy(arena, file_path_resolved);
          hash_table_push_path_raw(scratch.arena, source_file_ht, src_file->path, src_file);
        }
        src_file_map[file_idx] = src_file;
      }
      
      for EachNode(line_seq, DW_LineSeqNode, line_table->first_seq) {
        if (line_seq->count == 0) { continue; }
        
        U64 *voffs     = push_array(arena, U64, line_seq->count);
        U32 *line_nums = push_array(arena, U32, line_seq->count);
        U16 *col_nums  = 0;
        U64  line_idx  = 0;
        
        DW_LineNode *file_line_n     = line_seq->first;
        U64          file_line_count = 0;
        
        for EachNode(line_n, DW_LineNode, file_line_n) {
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
                if (file_line_n->v.address == 0) { continue; }
                
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
            file_line_count += 1;
          }
        }
        
        // handle last line
        if (file_line_n) {
          U64  file_index     = file_line_n->v.file_index;
          U64 *file_voffs     = &voffs[line_idx];
          U32 *file_line_nums = &line_nums[line_idx];
          U16 *file_col_nums  = 0;
          
          for (; file_line_n != 0; file_line_n = file_line_n->next, line_idx += 1) {
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
    
    RDIM_Type *builtin_types[RDI_TypeKind_Count] = {0};
    for (RDI_TypeKind type_kind = RDI_TypeKind_FirstBuiltIn; type_kind <= RDI_TypeKind_LastBuiltIn; type_kind += 1) {
      RDIM_Type *type = rdim_type_chunk_list_push(arena, &types, TYPE_CHUNK_CAP);
      type->kind      = type_kind;
      type->name.str  = rdi_string_from_type_kind(type_kind, &type->name.size);
      type->byte_size = rdi_size_from_basic_type_kind(type_kind);
      builtin_types[type_kind] = type;
    }
    builtin_types[RDI_TypeKind_Void]->byte_size = arch_addr_size;
    builtin_types[RDI_TypeKind_Handle]->byte_size = arch_addr_size;
    
    builtin_types[RDI_TypeKind_Variadic] = rdim_type_chunk_list_push(arena, &types, TYPE_CHUNK_CAP);
    builtin_types[RDI_TypeKind_Variadic]->kind = RDI_TypeKind_Variadic;
    
    //////////////////////////////// 
    
    ProfBegin("Convert Units");
    for EachIndex(cu_idx, cu_ranges.count) {
      Temp comp_temp = temp_begin(scratch.arena);
      
      DW_CompUnit *cu = &cu_arr[cu_idx];
      
      // parse and build tag tree
      DW_TagTree tag_tree = dw_tag_tree_from_cu(comp_temp.arena, &input, cu);
      
      // skip DWO
      {
        if (cu->dwo_id) { goto next_cu; }
        
        String8 dwo_name = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_DwoName);
        if (dwo_name.size) { goto next_cu; }
        
        String8 gnu_dwo_name = dw_string_from_tag_attrib_kind(&input, cu, cu->tag, DW_AttribKind_GNU_DwoName);
        if (gnu_dwo_name.size) { goto next_cu; }
      }
      
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
      type_table->types           = &types;
      type_table->type_chunk_cap  = TYPE_CHUNK_CAP;
      type_table->builtin_types   = builtin_types;
      
      // convert debug info
      d2r_convert_types(arena, type_table, &input, cu, cu_lang, arch_addr_size, tag_tree.root);
      d2r_convert_udts(arena, type_table, &input, cu, cu_lang, arch_addr_size, tag_tree.root);
      d2r_convert_symbols(arena, type_table, global_scope, &input, cu, cu_lang, arch_addr_size, image_base, arch, tag_tree.root);
      
      RDIM_Rng1U64ChunkList cu_voff_ranges = {0};
      if (cu_idx < cu_contrib_map.count) {
        cu_voff_ranges = d2r_voff_ranges_from_cu_info_off(cu_contrib_map, cu_ranges.v[cu_idx].min);
      } else {
        // TODO: synthesize cu ranges from scopes
        NotImplemented;
      }
      
      // convert compile unit
      {
        RDIM_Unit *unit     = rdim_unit_chunk_list_push(arena, &units, UNIT_CHUNK_CAP);
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
  bake_params.top_level_info   = top_level_info;
  bake_params.binary_sections  = binary_sections;
  bake_params.units            = units;
  bake_params.types            = types;
  bake_params.udts             = udts;
  bake_params.src_files        = src_files;
  bake_params.line_tables      = line_tables;
  bake_params.locations        = locations;
  bake_params.global_variables = gvars;
  bake_params.thread_variables = tvars;
  bake_params.procedures       = procs;
  bake_params.scopes           = scopes;
  bake_params.inline_sites     = inline_sites;
  
  scratch_end(scratch);
  return bake_params;
}

