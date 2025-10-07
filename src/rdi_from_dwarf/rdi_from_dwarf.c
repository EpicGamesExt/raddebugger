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

internal U64
rdim_virt_off_from_eval_bytecode(RDIM_EvalBytecode bc, U64 image_base)
{
  Temp scratch = scratch_begin(0,0);
  
  typedef union { U16 u16; U32 u32; U64 u64; S64 s64; F32 f32; F64 f64; } Value;
  U64 stack_cap = 128, stack_count = 0;
  Value *stack = push_array(scratch.arena, Value, stack_cap);
  
  for EachNode(opcode_n, RDIM_EvalBytecodeOp, bc.first_op) {
    // pop values from stack
    Value *svals = 0;
    {
      U32 pop_count = RDI_POPN_FROM_CTRLBITS(rdi_eval_op_ctrlbits_table[opcode_n->op]);
      if (pop_count > stack_count) {
        // TODO: report error
        Assert(!"malformed byte code");
        break;
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
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Neg: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:
          case RDI_EvalTypeGroup_S:   { nval.u64 = ~svals[0].u64 + 1; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = -svals[0].f32;     } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = -svals[0].f64;     } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Add: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 + svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 + svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 + svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 + svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Sub: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 - svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[1].s64 - svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 - svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 - svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Mul: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 * svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 * svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 * svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 * svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
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
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
        
        // TODO: report error
        AssertAlways(!is_div_by_zero);
        
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 / svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 / svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 / svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 / svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Mod: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 % svals[1].u64;    } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 % svals[1].s64;    } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 MOD is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 MOD is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_LShift: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 << svals[1].u64;      } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 << svals[1].u64;      } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 LShift is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 LShift is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_RShift: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 >> svals[1].u64;      } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[1].s64 >> svals[1].u64;      } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 RShift is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 RShift is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_BitAnd: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 | svals[1].u64;            } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 | svals[1].s64;            } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 bitwise AND is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 bitwise AND is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_BitXor: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 ^ svals[1].u64;    } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 ^ svals[1].s64;    } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 XOR is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 XOR is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_BitNot: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = ~svals[0].u64;                          } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = ~svals[0].u64;                          } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 bitwise NOT is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 bitwise NOT is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_LogAnd: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 && svals[1].u64;   } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 && svals[1].s64;   } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 AND is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 AND is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_LogOr: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 || svals[1].u64;  } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].u64 || svals[1].s64;  } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 OR is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 OR is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_LogNot: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = !svals[0].u64;                  } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = !svals[0].u64;                  } break;
          case RDI_EvalTypeGroup_F32: { AssertAlways(!"F32 NOT is not supported"); } break; // TODO: report error
          case RDI_EvalTypeGroup_F64: { AssertAlways(!"F64 NOT is not supported"); } break; // TODO: report error
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
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
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_GrEq: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 >= svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 >= svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 >= svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 >= svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Less: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 < svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 < svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 < svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 < svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Grtr: {
        switch (imm.u64) {
          case RDI_EvalTypeGroup_Other: {} break;
          case RDI_EvalTypeGroup_U:   { nval.u64 = svals[0].u64 > svals[1].u64; } break;
          case RDI_EvalTypeGroup_S:   { nval.s64 = svals[0].s64 > svals[1].s64; } break;
          case RDI_EvalTypeGroup_F32: { nval.f32 = svals[0].f32 > svals[1].f32; } break;
          case RDI_EvalTypeGroup_F64: { nval.f64 = svals[0].f64 > svals[1].f64; } break;
          default: { AssertAlways(!"unexpected eval type group"); } break; // report error
        }
      } break;
      case RDI_EvalOp_Trunc: {
        if (0 < imm.u64 && imm.u64 < 64) {
          U64 mask = max_U64 >> (64 - imm.u64);
          nval.u64 = svals[0].u64 & (max_U64 >> (64 - imm.u64));
        } else if (imm.u64 > 64) {
          // TODO: report error
          AssertAlways(!"malformed bytecode");
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
          // TODO: report error
          AssertAlways(!"malformed bytecode");
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
            default: { Assert(!"unexpected conversion case"); } break; // report error
          }
        }
      } break;
      case RDI_EvalOp_Pick: {
        if (stack_count > imm.u64) {
          nval = stack[stack_count - imm.u64 - 1];
        } else {
          // TODO: report error
          AssertAlways(!"malformed bytecode");
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
          // TODO: report error
          AssertAlways(!"malformed bytecode");
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
          default: { AssertAlways(!"malformed bytecode"); } break; // TODO: report error
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
          AssertAlways(!"stack overflow"); // TODO: report error
        }
      }
    }
  }
  
  U64 result = 0;
  if (stack_count >= 1) {
    result = stack[0].u64 - image_base;
  }
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Bytecode Conversion Helpers

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
d2r_unsigned_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
    case 8:   return D2R_ValueType_U8;
    case 16:  return D2R_ValueType_U16;
    case 32:  return D2R_ValueType_U32;
    case 64:  return D2R_ValueType_U64;
    case 128: return D2R_ValueType_U128;
    case 256: return D2R_ValueType_U256;
    case 512: return D2R_ValueType_U512;
  }
  AssertAlways(!"no suitable unsigned type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal D2R_ValueType
d2r_signed_value_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
    case 8:   return D2R_ValueType_S8;
    case 16:  return D2R_ValueType_S16;
    case 32:  return D2R_ValueType_S32;
    case 64:  return D2R_ValueType_S64;
    case 128: return D2R_ValueType_S128;
    case 256: return D2R_ValueType_S256;
    case 512: return D2R_ValueType_S512;
  }
  AssertAlways(!"no suitable signed type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal D2R_ValueType
d2r_float_type_from_bit_size(U64 bit_size)
{
  switch (bit_size) {
    case 4: return D2R_ValueType_F32;
    case 8: return D2R_ValueType_F64;
  }
  AssertAlways(!"no suitable type was found for the specified size");
  return D2R_ValueType_Generic;
}

internal RDI_EvalTypeGroup
d2r_value_type_to_rdi(D2R_ValueType v)
{
  RDI_EvalTypeGroup result = RDI_EvalTypeGroup_Other;
  switch(v)
  {
    case D2R_ValueType_Generic:
    {result = RDI_EvalTypeGroup_Other;}break;
    case D2R_ValueType_U8:
    case D2R_ValueType_U16:
    case D2R_ValueType_U32:
    case D2R_ValueType_U64:
    {result = RDI_EvalTypeGroup_U;}break;
    case D2R_ValueType_S8:
    case D2R_ValueType_S16:
    case D2R_ValueType_S32:
    case D2R_ValueType_S64:
    {result = RDI_EvalTypeGroup_S;}break;
    case D2R_ValueType_F32:
    {result = RDI_EvalTypeGroup_F32;}break;
    case D2R_ValueType_F64:
    {result = RDI_EvalTypeGroup_F64;}break;
    case D2R_ValueType_Address:
    {result = RDI_EvalTypeGroup_U;}break;
    default:
    case D2R_ValueType_ImplicitValue:
    {AssertAlways(!"unable to convert value type to RDI equivalent");}break;
  }
  return result;
}

internal U64
d2r_size_from_value_type(U64 addr_size, D2R_ValueType value_type)
{
  switch (value_type) {
    case D2R_ValueType_Address: return addr_size;
    case D2R_ValueType_U8:   return 1;
    case D2R_ValueType_U16:  return 2;
    case D2R_ValueType_U32:  return 4;
    case D2R_ValueType_U64:  return 8;
    case D2R_ValueType_U128: return 16;
    case D2R_ValueType_U256: return 32;
    case D2R_ValueType_U512: return 64;
    case D2R_ValueType_S8:   return 1;
    case D2R_ValueType_S16:  return 2;
    case D2R_ValueType_S32:  return 4;
    case D2R_ValueType_S64:  return 8;
    case D2R_ValueType_S128: return 16;
    case D2R_ValueType_S256: return 32;
    case D2R_ValueType_S512: return 64;
    case D2R_ValueType_F32:  return 4;
    case D2R_ValueType_F64:  return 8;
    default: return 0;
  }
}

internal D2R_ValueType
d2r_pick_common_value_type(D2R_ValueType lhs, D2R_ValueType rhs)
{
  if (lhs == rhs) {
    return lhs;
  }
  // unsigned vs unsigned
  else if (D2R_ValueType_IsUnsigned(lhs) && D2R_ValueType_IsUnsigned(rhs)) {
    return Max(lhs, rhs);
  }
  // signed vs signed
  else if (D2R_ValueType_IsSigned(lhs) && D2R_ValueType_IsSigned(rhs)) {
    return Max(lhs, rhs);
  }
  // (unsigned vs signed) || (signed vs unsigned)
  else if ((D2R_ValueType_IsUnsigned(lhs) && D2R_ValueType_IsSigned(rhs)) ||
           (D2R_ValueType_IsSigned(lhs) && D2R_ValueType_IsUnsigned(rhs))) {
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
  else if (D2R_ValueType_IsFloat(lhs) && D2R_ValueType_IsInt(rhs)) {
    return lhs;
  }
  // int vs float
  else if (D2R_ValueType_IsInt(lhs) && D2R_ValueType_IsFloat(rhs)) {
    return rhs;
  }
  // float vs float
  else if (D2R_ValueType_IsFloat(lhs) && D2R_ValueType_IsFloat(rhs)) {
    return Max(lhs, rhs);
  }
  // address vs int
  else if (lhs == D2R_ValueType_Address && D2R_ValueType_IsInt(rhs)) {
    return D2R_ValueType_Address;
  }
  // int vs address
  else if (D2R_ValueType_IsInt(lhs) && rhs == D2R_ValueType_Address) {
    return D2R_ValueType_Address;
  }
  // address vs float
  else if (lhs == D2R_ValueType_Address && D2R_ValueType_IsFloat(rhs)) {
    return D2R_ValueType_Generic;
  }
  // float vs address
  else if (D2R_ValueType_IsFloat(lhs) && rhs == D2R_ValueType_Address) {
    return D2R_ValueType_Generic;
  }
  // no conversion for implicit value
  else if (lhs == D2R_ValueType_ImplicitValue || rhs == D2R_ValueType_ImplicitValue) {
    return D2R_ValueType_Generic;
  }
  AssertAlways(!"undefined conversion case");
  return D2R_ValueType_Generic;
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
  if (D2R_ValueType_IsInt(lhs) && rhs == D2R_ValueType_Address) {
    rdim_bytecode_push_op(arena, bc, RDI_EvalOp_Swap, 0);
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(lhs), RDI_EvalTypeGroup_U);
    rdim_bytecode_push_op(arena, bc, RDI_EvalOp_Swap, 0);
    common_type = D2R_ValueType_Address;
  } else if (lhs == D2R_ValueType_Address && D2R_ValueType_IsInt(rhs)) {
    rdim_bytecode_push_convert(arena, bc, d2r_value_type_to_rdi(rhs), RDI_EvalTypeGroup_U);
    common_type = D2R_ValueType_Address;
  } else {
    common_type = d2r_apply_usual_arithmetic_conversions(arena, lhs, rhs, bc);
  }
  rdim_bytecode_push_op(arena, bc, RDI_EvalOp_EqEq, d2r_value_type_to_rdi(common_type));
  d2r_value_type_stack_push(0, stack, D2R_ValueType_Bool);
}

internal RDIM_EvalBytecode
d2r_bytecode_from_expression(Arena       *arena,
                             DW_Input    *input,
                             U64          image_base,
                             U64          address_size,
                             Arch         arch,
                             DW_ListUnit *addr_lu,
                             String8      raw_expr,
                             DW_CompUnit *cu,
                             D2R_ValueType *result_type_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  Temp arena_restore_point = temp_begin(arena);
  
  RDIM_EvalBytecode bc = {0};
  
  DW_Expr               expr            = dw_expr_from_data(scratch.arena, cu->format, address_size, raw_expr);
  D2R_ValueTypeStack   *stack           = push_array(scratch.arena, D2R_ValueTypeStack, 1);
  RDIM_EvalBytecodeOp **converted_insts = push_array(scratch.arena, RDIM_EvalBytecodeOp *, expr.count);
  B32                   is_ok           = 1;
  U64                   inst_idx        = 0;
  for EachNode(inst, DW_ExprInst, expr.first) {
    RDIM_EvalBytecodeOp *last_op = bc.last_op;
    
    U64 pop_count = dw_pop_count_from_expr_op(inst->opcode);
    if (pop_count > stack->count) {
      // TODO: report error
      Assert(!"not enough values on the stack to evaluate instruction");
      is_ok = 0;
      break;
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
        if (inst->operands[0].u64 >= image_base) {
          U64 voff = inst->operands[0].u64 - image_base;
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_ModuleOff, voff);
          d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
        } else {
          is_ok = 0;
        }
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
        d2r_value_type_stack_push(scratch.arena, stack, d2r_unsigned_value_type_from_bit_size(reg_size));
      } break;
      case DW_ExprOp_ImplicitValue: {
        if (inst->operands[0].block.size <= sizeof(U64)) {
          U64 implicit_value;
          MemoryCopyStr8(&implicit_value, inst->operands[0].block);
          rdim_bytecode_push_uconst(arena, &bc, implicit_value);
          d2r_value_type_stack_push(scratch.arena, stack, d2r_unsigned_value_type_from_bit_size(inst->operands[0].block.size * 8));
        } else {
          // TODO: currenlty no way to encode string in RDIM_EvalBytecodeOp
          NotImplemented;
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
          // TODO: report error
          AssertAlways(!"out of bounds pick");
        }
      } break;
      case DW_ExprOp_Over: {
        if (stack->top && stack->top->next) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Pick, 1);
          d2r_value_type_stack_push(scratch.arena, stack, stack->top->next->type);
        } else {
          // TODO: report error
          AssertAlways(!"out of bounds over");
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
          cursor += inst->size;
          inst_count += 1;
        }
        
        // TODO: report error (skip does not land on first byte of an instruction)
        AssertAlways(cursor == delta);
        // TODO: report overflow
        AssertAlways(inst_count <= min_S16);
        AssertAlways(inst_idx <= max_U32);
        
        U64 imm = Compose64Bit(inst_idx, skip_fwd ? (S16)inst_count : -(S16)inst_count);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Skip, imm);
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
        if (address_type != D2R_ValueType_Address && !D2R_ValueType_IsInt(address_type)) {
          // TODO: report error
          Assert(!"value must be of integral type");
          break;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, address_size);
        d2r_value_type_stack_push(scratch.arena, stack, address_type);
      } break;
      case DW_ExprOp_DerefSize: {
        D2R_ValueType address_type = d2r_value_type_stack_pop(stack);
        if (!D2R_ValueType_IsInt(address_type) && address_type != D2R_ValueType_Address ) {
          // TODO: report error
          Assert(!"value must be of integral type");
          break;
        }
        U8 deref_size_in_bytes = inst->operands[0].u64;
        if (0 < deref_size_in_bytes && deref_size_in_bytes <= address_size) {
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_MemRead, deref_size_in_bytes);
        } else {
          // TODO: error handling
          AssertAlways(!"ill formed expression");
        }
        d2r_value_type_stack_push(scratch.arena, stack, address_type);
      } break;
      case DW_ExprOp_XDeref: {
        // TODO: error handling
        AssertAlways(!"multiple address spaces are not supported");
      } break;
      // TODO: error handling
      case DW_ExprOp_XDerefSize: { AssertAlways(!"no suitable conversion"); } break;
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
          DW_TagNode *tag_node = dw_tag_node_from_info_off(cu, inst->operands[0].u64);
          DW_Tag      tag      = tag_node->tag;
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
            AssertAlways(!"unexpected tag"); // TODO: error handling
          }
        }
        
        D2R_ValueType in = d2r_value_type_stack_pop(stack);
        d2r_value_type_stack_push(scratch.arena, stack, out);
        rdim_bytecode_push_convert(arena, &bc, d2r_value_type_to_rdi(in), d2r_value_type_to_rdi(out));
      } break;
      // TODO:
      case DW_ExprOp_GNU_ParameterRef: { AssertAlways(!"sample"); } break;
      // TODO:
      case DW_ExprOp_DerefType:
      case DW_ExprOp_GNU_DerefType: { AssertAlways(!"sample"); } break;
      // TODO:
      case DW_ExprOp_ConstType: 
      case DW_ExprOp_GNU_ConstType: { AssertAlways(!"sample"); } break;
      // TODO:
      case DW_ExprOp_RegvalType: { AssertAlways(!"sample"); } break;
      case DW_ExprOp_EntryValue:
      case DW_ExprOp_GNU_EntryValue: {
        D2R_ValueType call_site_result_type = 0;
        RDIM_EvalBytecode call_site_bc = d2r_bytecode_from_expression(arena, input, image_base, address_size, arch, addr_lu, inst->operands[0].block, cu, &call_site_result_type);
        
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
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      case DW_ExprOp_FormTlsAddress: {
        // TODO:
        AssertAlways(!"RDI_EvalOp_TLSOff accepts immediate");
      } break;
      case DW_ExprOp_PushObjectAddress: {
        AssertAlways(!"sample");
      } break;
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
        if (D2R_ValueType_IsInt(rhs) && D2R_ValueType_IsInt(lhs)) {
          D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
          D2R_ValueType result_type = d2r_unsigned_value_type_from_bit_size(d2r_size_from_value_type((address_size), common_type) * 8);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, d2r_value_type_to_rdi(result_type));
          d2r_value_type_stack_push(scratch.arena, stack, result_type);
        } else {
          // TODO: report error
          AssertAlways(!"operands must be of integral type");
        }
      } break;
      case DW_ExprOp_Shra: {
        D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (D2R_ValueType_IsInt(lhs) && D2R_ValueType_IsInt(rhs)) {
          D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
          D2R_ValueType result_type = d2r_signed_value_type_from_bit_size(d2r_size_from_value_type((address_size), common_type) * 8);
          rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_RShift, d2r_value_type_to_rdi(result_type));
          d2r_value_type_stack_push(scratch.arena, stack, result_type);
        } else {
          // TODO: report error
          AssertAlways(!"operands must be of integral type");
        }
      } break;
      case DW_ExprOp_Mod: {
        D2R_ValueType rhs = d2r_value_type_stack_pop(stack);
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (!D2R_ValueType_IsInt(rhs) || !D2R_ValueType_IsInt(lhs)) {
          // TODO: report error
          AssertAlways(!"operands must be of integral type");
          is_ok = 0;
          break;
        }
        D2R_ValueType common_type = d2r_pick_common_value_type(lhs, rhs);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Mod, d2r_value_type_to_rdi(common_type));
        d2r_value_type_stack_push(scratch.arena, stack, common_type);
      } break;
      case DW_ExprOp_Abs: {
        if (!D2R_ValueType_IsInt(d2r_value_type_stack_peek(stack)) && !D2R_ValueType_IsFloat(d2r_value_type_stack_peek(stack))) {
          // TODO: report error
          AssertAlways(!"operand must be of integral type or float");
          is_ok = 0;
          break;
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Abs, d2r_value_type_to_rdi(d2r_value_type_stack_peek(stack)));
      } break;
      case DW_ExprOp_Neg: {
        if (!D2R_ValueType_IsInt(d2r_value_type_stack_peek(stack))) {
          // TODO: report error
          AssertAlways(!"operand must be of integral type");
          is_ok = 0;
          break;
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
      case DW_ExprOp_Rot:  { AssertAlways(!"no suitable conversion"); } break;
      case DW_ExprOp_Swap: { AssertAlways(!"no suitable conversion"); } break;
      case DW_ExprOp_Drop: { AssertAlways(!"no suitable conversion"); } break;
      case DW_ExprOp_StackValue: {
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Stop, 0);
        if (stack->top->type == D2R_ValueType_Address) {
          stack->top->type = d2r_unsigned_value_type_from_bit_size(address_size * 8);
        }
      } break;
      case DW_ExprOp_GNU_PushTlsAddress: {
        D2R_ValueType lhs = d2r_value_type_stack_pop(stack);
        if (!D2R_ValueType_IsInt(lhs)) {
          // TODO: report error
          AssertAlways(!"lhs must be of integral type");
        }
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_TLSOff, 0);
        rdim_bytecode_push_op(arena, &bc, RDI_EvalOp_Add, d2r_value_type_to_rdi(lhs));
        d2r_value_type_stack_push(scratch.arena, stack, D2R_ValueType_Address);
      } break;
      
      default: InvalidPath; break;
    }
    if (!is_ok) { break; }
    
    // store converted instruction
    if (last_op != bc.last_op) {
      RDIM_EvalBytecodeOp *first_converted_op = last_op ? last_op->next : bc.first_op;
      converted_insts[inst_idx] = first_converted_op;
    }
    
    inst_idx += 1;
  }
  
  if (is_ok) {
    // fixup bytecode
    for EachNode(op, RDIM_EvalBytecodeOp, bc.first_op) {
      if (op->op == RDI_EvalOp_Skip) {
        // unpack skip info
        U32 inst_idx          = Extract32(op->p, 0);
        S16 skip_count_signed = (S16)Extract32(op->p, 1);
        U16 skip_count        = abs_s64(skip_count_signed);
        B32 skip_fwd          = skip_count_signed > 0;
        
        // setup being/end links
        RDIM_EvalBytecodeOp *begin = 0, *end = 0;
        if (skip_fwd) {
          if (inst_idx + skip_count <= expr.count) {
            begin = converted_insts[inst_idx];
            end   = (inst_idx + skip_count) < expr.count ? converted_insts[inst_idx + skip_count] : 0;
          } else {
            // TODO: report error
            AssertAlways(!"out of bounds skip");
          }
        } else {
          if (skip_count <= inst_idx) {
            begin = converted_insts[inst_idx - skip_count];
            end   = converted_insts[inst_idx];
          } else {
            // TODO: report error
            AssertAlways(!"out of bounds skip");
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
  } else {
    MemoryZeroStruct(&bc);
    temp_end(arena_restore_point);
  }
  
  scratch_end(scratch);
  return bc;
}

internal RDIM_Location *
d2r_transpile_expression(Arena *arena, RDIM_LocationChunkList *locations, DW_Input *input, U64 image_base, U64 address_size, Arch arch, DW_ListUnit *addr_lu, DW_CompUnit *cu, String8 expr)
{
  RDIM_Location *loc = 0;
  if (expr.size) {
    D2R_ValueType result_type = 0;
    RDIM_EvalBytecode bytecode = d2r_bytecode_from_expression(arena, input, image_base, address_size, arch, addr_lu, expr, cu, &result_type);
    
    RDIM_LocationInfo *loc_info = push_array(arena, RDIM_LocationInfo, 1);
    loc_info->kind     = result_type == D2R_ValueType_Address ? RDI_LocationKind_AddrBytecodeStream : RDI_LocationKind_ValBytecodeStream;
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
  if(root != 0)
  {
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
        if(type == 0)
        {
          type = type_table->builtin_types[RDI_TypeKind_NULL];
        }
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
        // TODO(rjf): this is not an invalid case; it shows up in `mule_main` pointer types
        // Assert(!dw_tag_has_attrib(input, cu, tag, DW_AttribKind_Name));
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
        if(parent->udt != 0)
        {
          RDIM_Type      *type   = d2r_find_or_convert_type(arena, type_table, input, cu, cu_lang, arch_addr_size, tag, DW_AttribKind_Type);
          RDIM_UDTMember *member = rdim_udt_push_member(arena, &udts, parent->udt);
          member->kind           = RDI_MemberKind_Base;
          member->type           = type;
          member->off            = safe_cast_u32(dw_const_u32_from_tag_attrib_kind(input, cu, tag, DW_AttribKind_DataMemberLocation));
        }
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
          
          U64 voff          = max_U64;
          B32 is_thread_var = 0;
          {
            DW_Attrib      *loc_attrib = dw_attrib_from_tag(input, cu, tag, DW_AttribKind_Location);
            DW_AttribClass  loc_class  = dw_value_class_from_attrib(cu, loc_attrib);
            if (loc_class == DW_AttribClass_ExprLoc) {
              Temp temp = temp_begin(scratch.arena);
              
              String8           expr      = dw_exprloc_from_attrib(input, cu, loc_attrib);
              D2R_ValueType     expr_type = 0;
              RDIM_EvalBytecode bc        = d2r_bytecode_from_expression(temp.arena, input, image_base, arch_addr_size, arch, cu->addr_lu, expr, cu, &expr_type);
              
              // evaluate bytecode to virutal offset if possible
              if (expr_type == D2R_ValueType_Address) {
                B32 is_static = rdim_is_eval_bytecode_static(bc);
                if (is_static) {
                  voff = rdim_virt_off_from_eval_bytecode(bc, image_base);
                }
              }
              
              // is this a thread variable?
              is_thread_var = rdim_is_bytecode_tls_dependent(bc);
              
              temp_end(temp);
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
      default:
      {
        // NotImplemented;
      }break;
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
        Rng1U64List range_list  = d2r_range_list_from_tag(scratch.arena, &input, cu, image_base, cu->tag);
        for EachNode(n, Rng1U64Node, range_list.first) {
          rdim_rng1u64_chunk_list_push(arena, &cu_voff_ranges, 512, (RDIM_Rng1U64){ .min = n->v.min, .max = n->v.max });
        }
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

