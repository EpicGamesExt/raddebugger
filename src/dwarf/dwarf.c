// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64
dw_reg_size_from_code_x86(DW_Reg reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX86_##reg_name_dw: return reg_size;
    DW_Regs_X86_XList(X)
#undef X
  }
  return 0;
}

internal U64
dw_reg_pos_from_code_x86(DW_Reg reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX86_##reg_name_dw: return reg_pos;
    DW_Regs_X86_XList(X)
#undef X
  }
  return max_U64;
}

internal U64
dw_reg_size_from_code_x64(DW_Reg reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX64_##reg_name_dw: return reg_size;
    DW_Regs_X64_XList(X)
#undef X
  }
  return 0;
}

internal U64
dw_reg_pos_from_code_x64(DW_Reg reg_code)
{
  switch (reg_code) {
#define X(reg_name_dw, reg_code_dw, reg_name_rdi, reg_pos, reg_size) case DW_RegX64_##reg_name_dw: return reg_pos;
    DW_Regs_X64_XList(X)
#undef X
  }
  return max_U64;
}

internal U64
dw_reg_size_from_code(Arch arch, DW_Reg reg_code)
{
  switch (arch) {
    case Arch_Null: break;
    case Arch_x86: return dw_reg_size_from_code_x86(reg_code);
    case Arch_x64: return dw_reg_size_from_code_x64(reg_code);
    default: NotImplemented; break;
  }
  return 0;
}

internal U64
dw_reg_pos_from_code(Arch arch, DW_Reg reg_code)
{
  switch (arch) {
    case Arch_Null: break;
    case Arch_x86: return dw_reg_pos_from_code_x86(reg_code);
    case Arch_x64: return dw_reg_pos_from_code_x64(reg_code);
    default: NotImplemented; break;
  }
  return max_U64;
}

internal U64
dw_reg_count_from_arch(Arch arch)
{
  switch (arch) {
  default: { NotImplemented; } // fall-through
  case Arch_Null: return 0;
  case Arch_x86: return DW_RegX86_Last;
  case Arch_x64: return DW_RegX64_Last;
  }
}

internal U64
dw_reg_max_size_from_arch(Arch arch)
{
  local_persist U64 max_size = 0;
  if (max_size == 0) {
    U64 max_idx  = dw_reg_count_from_arch(arch);
    for EachIndex(reg_idx, max_idx) {
      U64 reg_size = dw_reg_size_from_code(arch, reg_idx);
      max_size = Max(max_size, reg_size);
    }
  }
  return max_size;
}

internal U64
dw_sp_from_arch(Arch arch)
{
  switch (arch) {
  default: NotImplemented;
  case Arch_Null: return 0;
  case Arch_x86:  return DW_RegX86_Esp;
  case Arch_x64:  return DW_RegX64_Rsp;
  }
}

internal DW_AttribClass
dw_attrib_class_from_attrib_v2(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_V2_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_v3(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_V3_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_v4(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_V4_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_v5(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_V5_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_gnu(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_GNU_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_llvm(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_LLVM_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_apple(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_APPLE_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_mips(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_AttribKind_##_N: return _C;
    DW_AttribKind_ClassFlags_MIPS_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib(DW_Version ver, DW_Ext ext, DW_AttribKind k)
{
  DW_AttribClass result = DW_AttribClass_Null;
  
  while (ext) {
    U64 z = 64-clz64(ext);
    if (z == 0) {
      break;
    }
    U64 flag = 1 << (z-1);
    ext &= ~flag;
    
    switch (flag) {
      case DW_Ext_Null: break;
      case DW_Ext_GNU:   result = dw_attrib_class_from_attrib_gnu(k);   break;
      case DW_Ext_LLVM:  result = dw_attrib_class_from_attrib_llvm(k);  break;
      case DW_Ext_APPLE: result = dw_attrib_class_from_attrib_apple(k); break;
      case DW_Ext_MIPS:  result = dw_attrib_class_from_attrib_mips(k);  break;
      default: InvalidPath; break;
    }
    
    if (result != DW_AttribClass_Null) {
      break;
    }
  }
  
  if (result == DW_AttribClass_Null) {
    switch (ver) {
      case DW_Version_Null: break;
      case DW_Version_1:    AssertAlways(!"DWARF V1 is not supported");      break;
      case DW_Version_2:    result = dw_attrib_class_from_attrib_v2(k); break;
      case DW_Version_3:    result = dw_attrib_class_from_attrib_v3(k); break;
      case DW_Version_4:    result = dw_attrib_class_from_attrib_v4(k); break;
      case DW_Version_5:    result = dw_attrib_class_from_attrib_v5(k); break;
      default: InvalidPath; break;
    }
  }
  
  return result;
}

internal DW_AttribClass
dw_attrib_class_from_form_kind(DW_Version ver, DW_FormKind k)
{
#define X(_N,_C) case DW_Form_##_N: return _C;
  
  switch (k) {
    DW_Form_AttribClass_GNU_XList(X)
  }
  
  switch (ver) {
    case DW_Version_5: {
      switch (k) {
        DW_Form_AttribClass_V5_XList(X)
      }
    } break;
    case DW_Version_4: {
      switch (k) {
        DW_Form_AttribClass_V4_XList(X)
      }
    } break;
    case DW_Version_3: {
      switch (k) {
        DW_Form_AttribClass_V2_XList(X)
      }
    } break;
    case DW_Version_2: {
      switch (k) {
        DW_Form_AttribClass_V2_XList(X)
      }
    } break;
    case DW_Version_1: {
    } break;
    case DW_Version_Null: break;
  }
#undef X
  
  return DW_AttribClass_Null;
}

internal B32
dw_are_attrib_class_and_form_kind_compatible(DW_Version ver, DW_AttribClass attrib_class, DW_FormKind form_kind)
{
  DW_AttribClass compat_flags = dw_attrib_class_from_form_kind(ver, form_kind);
  B32            are_compat = (attrib_class & compat_flags) != 0;
  return are_compat;
}

internal String8
dw_name_string_from_section_kind(DW_SectionKind k)
{
  switch (k) {
#define X(_N,_L,_M,_D) case DW_Section_##_N: return str8_lit(_L);
    DW_SectionKind_XList(X)
#undef X
  }
  return str8_zero();
}

internal String8
dw_mach_name_string_from_section_kind(DW_SectionKind k)
{
  switch (k) {
#define X(_N,_L,_M,_D) case DW_Section_##_N: return str8_lit(_M);
    DW_SectionKind_XList(X)
#undef X
  }
  return str8_zero();
}

internal String8
dw_dwo_name_string_from_section_kind(DW_SectionKind k)
{
  switch (k) {
#define X(_N,_L,_M,_D) case DW_Section_##_N: return str8_lit(_D); 
    DW_SectionKind_XList(X)
#undef X
  }
  return str8_zero();
}

internal U64
dw_size_from_format(DW_Format format)
{
  U64 result = 0;
  switch (format) {
    case DW_Format_Null: break;
    case DW_Format_32Bit: result = 4; break;
    case DW_Format_64Bit: result = 8; break;
    default: InvalidPath; break;
  }
  return result;
}

internal DW_AttribClass
dw_pick_attrib_value_class(DW_Version ver, DW_Ext ext, B32 relaxed, DW_AttribKind attrib_kind, DW_FormKind form_kind)
{
  // NOTE(rjf): DWARF's spec specifies two mappings:
  // (DW_AttribKind) => List(DW_AttribClass)
  // (DW_FormKind)   => List(DW_AttribClass)
  //
  // This function's purpose is to find the overlapping class between an
  // DW_AttribKind and DW_FormKind.
  
  DW_AttribClass attrib_class = dw_attrib_class_from_attrib(ver, ext, attrib_kind);
  DW_AttribClass form_class   = dw_attrib_class_from_form_kind(ver, form_kind);
  
  if(relaxed)
  {
    if(attrib_class == DW_AttribClass_Null || form_class == DW_AttribClass_Null)
    {
      attrib_class = dw_attrib_class_from_attrib(DW_Version_Last, ext, attrib_kind);
      form_class   = dw_attrib_class_from_form_kind(DW_Version_Last, form_kind);
    }
  }
  
  DW_AttribClass result = DW_AttribClass_Null;
  if(attrib_class != DW_AttribClass_Null && form_class != DW_AttribClass_Null)
  {
    result = DW_AttribClass_Undefined;
    
    for(U32 i = 0; i < 32; ++i)
    {
      U32 n = 1u << i;
      if((attrib_class & n) != 0 && (form_class & n) != 0)
      {
        result = ((DW_AttribClass) n);
        break;
      }
    }
  }
  
  return result;
}

internal U64
dw_pick_default_lower_bound(DW_Language lang)
{
  U64 lower_bound = max_U64;
  switch (lang) {
    case DW_Language_Null: break;
    case DW_Language_C89:
    case DW_Language_C:
    case DW_Language_CPlusPlus:
    case DW_Language_C99:
    case DW_Language_CPlusPlus03:
    case DW_Language_CPlusPlus11:
    case DW_Language_C11:
    case DW_Language_CPlusPlus14:
    case DW_Language_Java:
    case DW_Language_ObjC:
    case DW_Language_ObjCPlusPlus:
    case DW_Language_UPC:
    case DW_Language_D:
    case DW_Language_Python:
    case DW_Language_OpenCL:
    case DW_Language_Go:
    case DW_Language_Haskell:
    case DW_Language_OCaml:
    case DW_Language_Rust:
    case DW_Language_Swift:
    case DW_Language_Dylan:
    case DW_Language_RenderScript:
    case DW_Language_BLISS:
    lower_bound = 0;
    break;
    case DW_Language_Ada83:
    case DW_Language_Cobol74:
    case DW_Language_Cobol85:
    case DW_Language_Fortran77:
    case DW_Language_Fortran90:
    case DW_Language_Pascal83:
    case DW_Language_Modula2:
    case DW_Language_Ada95:
    case DW_Language_Fortran95:
    case DW_Language_PLI:
    case DW_Language_Modula3:
    case DW_Language_Julia:
    case DW_Language_Fortran03:
    case DW_Language_Fortran08:
    lower_bound = 1;
    default:
    NotImplemented;
    break;
  }
  return lower_bound;
}

internal U64
dw_operand_count_from_expr_op(DW_ExprOp op)
{
  switch (op) {
#define X(_N, _ID, _OPER_COUNT, _POP_COUNT, _PUSH_COUNT) case _ID: return _OPER_COUNT;
    DW_Expr_V3_XList(X)
    DW_Expr_V4_XList(X)
    DW_Expr_V5_XList(X)
    DW_Expr_GNU_XList(X)
#undef X
  default: { NotImplemented; } break;
  }
  return 0;
}

internal U64
dw_pop_count_from_expr_op(DW_ExprOp op)
{
  switch (op) {
#define X(_N, _ID, _OPER_COUNT, _POP_COUNT, _PUSH_COUNT) case _ID: return _POP_COUNT;
    DW_Expr_V3_XList(X)
    DW_Expr_V4_XList(X)
    DW_Expr_V5_XList(X)
    DW_Expr_GNU_XList(X)
#undef X
  default: { NotImplemented; } break;
  }
  return 0;
}

internal U64
dw_push_count_from_expr_op(DW_ExprOp op) 
{
  switch (op) {
#define X(_N, _ID, _OPER_COUNT, _POP_COUNT, _PUSH_COUNT) case _ID: return _PUSH_COUNT;
    DW_Expr_V3_XList(X)
    DW_Expr_V4_XList(X)
    DW_Expr_V5_XList(X)
    DW_Expr_GNU_XList(X)
#undef X
  default: { NotImplemented; } break;
  }
  return 0;
}

internal U64
dw_operand_count_from_cfa_opcode(DW_CFA_Opcode opcode)
{
  switch (opcode) {
#define X(_N, _ID, ...) case _ID: { local_persist DW_CFA_OperandType t[] = { DW_CFA_OperandType_Null, __VA_ARGS__ }; return ArrayCount(t)-1; }
    DW_CFA_Kind_XList(X)
#undef X
  default: { NotImplemented; } break;
  }
  return 0;
}

internal B32
dw_is_cfa_expr_opcode_invalid(DW_ExprOp opcode)
{
  B32 is_invalid = 0;
  switch (opcode) {
  case DW_ExprOp_Addrx:
  case DW_ExprOp_Call2:
  case DW_ExprOp_Call4:
  case DW_ExprOp_CallRef:
  case DW_ExprOp_ConstType:
  case DW_ExprOp_Constx:
  case DW_ExprOp_Convert:
  case DW_ExprOp_DerefType:
  case DW_ExprOp_RegvalType:
  case DW_ExprOp_Reinterpret:
  case DW_ExprOp_PushObjectAddress:
  case DW_ExprOp_CallFrameCfa: {
    is_invalid = 1;
  } break;
  default: break;
  }
  return is_invalid;
}

internal B32
dw_is_new_row_cfa_opcode(DW_CFA_Opcode opcode)
{
  B32 is_new_row_op = 0;
  switch (opcode) {
  case DW_CFA_SetLoc:
  case DW_CFA_AdvanceLoc:
  case DW_CFA_AdvanceLoc1:
  case DW_CFA_AdvanceLoc2:
  case DW_CFA_AdvanceLoc4: {
    is_new_row_op = 1;
  } break;
  default: break;
  }
  return is_new_row_op;
}

internal DW_CFA_OperandType *
dw_operand_types_from_cfa_op(DW_CFA_Opcode opcode)
{
  switch (opcode) {
#define X(_N, _ID, ...) case _ID: { local_persist DW_CFA_OperandType t[] = { DW_CFA_OperandType_Null, __VA_ARGS__ }; return &t[0] + 1; }
    DW_CFA_Kind_XList(X)
#undef X
  default: { NotImplemented; } break;
  }
  return 0;
}

////////////////////////////////
//~ rjf: String <=> Enum

internal String8
dw_string_from_format(DW_Format format)
{
  switch (format) {
  case DW_Format_Null:  return str8_zero();
  case DW_Format_32Bit: return str8_lit("DWARF32");
  case DW_Format_64Bit: return str8_lit("DWARF64");
  }
  return str8_zero();
}

internal String8
dw_string_from_expr_op(Arena *arena, DW_Version ver, DW_Ext ext, DW_ExprOp op)
{
  String8 result = {0};
  
#define X(_N,...) case DW_ExprOp_##_N: result = str8_lit(Stringify(_N)); goto exit;
  if (ext & DW_Ext_GNU) {
    switch (op) {
      DW_Expr_GNU_XList(X); 
    }
  }
  
  switch (ver) {
    case DW_Version_5: {
      switch (op) {
        DW_Expr_V5_XList(X)
      }
    } // fall-through
    case DW_Version_4: {
      switch (op) {
        DW_Expr_V4_XList(X)
      }
    } // fall-through
    case DW_Version_3:
    case DW_Version_2:
    case DW_Version_1: {
      switch (op) {
        DW_Expr_V3_XList(X)
      }
    } // fall-through
    case DW_Version_Null:
    break;
  }
#undef X
  
  result = push_str8f(arena, "%x", op);
  
  exit:;
  return result;
}

internal String8
dw_string_from_tag_kind(Arena *arena, DW_TagKind kind)
{
  switch (kind) {
    case DW_TagKind_Null: return str8_lit("Null");
#define X(_N,_ID) case DW_TagKind_##_N: return str8_lit(Stringify(_N));
    DW_TagKind_V3_XList(X)
      DW_TagKind_V5_XList(X)
      DW_TagKind_GNU_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_attrib_kind(Arena *arena, DW_Version ver, DW_Ext ext, DW_AttribKind kind)
{
#define X(_N,...) case DW_AttribKind_##_N:{result = str8_lit(Stringify(_N));}break;
  String8 result = {0};
  
  //- rjf: try extensions
  if(result.size != 0)
  {
    while(ext)
    {
      U64 z = 64-clz64(ext);
      if(z == 0)
      {
        break;
      }
      U64 flag = 1 << (z-1);
      ext &= ~flag;
      switch(flag)
      {
        default:{}break;
        case DW_Ext_Null:  break;
        case DW_Ext_GNU:   switch (kind) { DW_AttribKind_GNU_XList(X)   } break;
        case DW_Ext_LLVM:  switch (kind) { DW_AttribKind_LLVM_XList(X)  } break;
        case DW_Ext_APPLE: switch (kind) { DW_AttribKind_APPLE_XList(X) } break;
        case DW_Ext_MIPS:  switch (kind) { DW_AttribKind_MIPS_XList(X)  } break;
      }
    }
  }
  
  //- rjf: try version
  if(result.size == 0)
  {
    for(U64 retry = 0; retry < 2; retry += 1)
    {
      DW_Version version = retry ? DW_Version_5 : ver;
      switch(version)
      {
        case DW_Version_5: { switch(kind) { DW_AttribKind_V5_XList(X) } } // fall-through
        case DW_Version_4: { switch(kind) { DW_AttribKind_V4_XList(X) } } // fall-through
        case DW_Version_3: { switch(kind) { DW_AttribKind_V3_XList(X) } } // fall-through
        case DW_Version_2: { switch(kind) { DW_AttribKind_V2_XList(X) } } // fall-through
        case DW_Version_1: {}break;
        case DW_Version_Null:{}break;
        default:{}break;
      }
    }
  }
  
  //- rjf: fallback
  if(result.size == 0)
  {
    result = push_str8f(arena, "#%u", kind);
  }
  
#undef X
  return result;
}

internal String8
dw_string_from_form_kind(Arena *arena, DW_Version ver, DW_FormKind kind)
{
#define X(_N,...) case DW_Form_##_N: return str8_lit(Stringify(_N));
  switch (ver) {
    case DW_Version_5: {
      switch (kind) {
        DW_Form_V5_XList(X)
      }
    } // fall-through
    case DW_Version_4: {
      switch (kind) {
        DW_Form_V4_XList(X)
      }
    } // fall-through
    case DW_Version_3: 
    case DW_Version_2: {
      switch (kind) {
        DW_Form_V2_XList(X)
      }
    } // fall-through
    case DW_Version_Null: break;
  }
#undef X
  String8 result = push_str8f(arena, "%x", kind);
  return result;
}

internal String8
dw_string_from_language(Arena *arena, DW_Language kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_Language_##_N: return str8_lit(Stringify(_N));
    DW_Language_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_inl(Arena *arena, DW_InlKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_Inl_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_access_kind(Arena *arena, DW_AccessKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_AccessKind_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_calling_convetion(Arena *arena, DW_CallingConventionKind kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_CallingConventionKind_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_attrib_type_encoding(Arena *arena, DW_ATE kind)
{
  switch (kind) {
#define X(_N,_ID) case _ID: return str8_lit(Stringify(_N));
    DW_ATE_XList(X)
#undef X
  }
  return push_str8f(arena, "%llx", kind);
}

internal String8
dw_string_from_std_opcode(Arena *arena, DW_StdOpcode kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_StdOpcode_##_N: return str8_lit(Stringify(_N));
    DW_StdOpcode_XList(X)
#undef X
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_ext_opcode(Arena *arena, DW_ExtOpcode kind)
{
  switch (kind) {
#define X(_N,_ID) case DW_ExtOpcode_##_N: return str8_lit(Stringify(_N));
    DW_ExtOpcode_XList(X)
#undef X
    default: InvalidPath; break;
  }
  return push_str8f(arena, "%x", kind);
}

internal String8
dw_string_from_loc_list_entry_kind(Arena *arena, DW_LLE kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_section_kind(Arena *arena, DW_SectionKind kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_rng_list_entry_kind(Arena *arena, DW_RLE kind)
{
  NotImplemented;
  return str8_zero();
}

internal String8
dw_string_from_register(Arena *arena, Arch arch, U64 reg_id)
{
  String8 reg_str = str8_zero();
  switch (arch) {
    case Arch_Null: break;
    case Arch_x86: {
      switch (reg_id) {
#define X(_N, _ID, ...) case DW_RegX86_##_N: reg_str = str8_lit(Stringify(_N)); break;
        DW_Regs_X86_XList(X)
#undef X
      }
    } break;
    case Arch_x64: {
      switch (reg_id) {
#define X(_N, _ID, ...) case DW_RegX64_##_N: reg_str = str8_lit(Stringify(_N)); break;
        DW_Regs_X64_XList(X)
#undef X
      }
    } break;
    case Arch_arm32: NotImplemented; break;
    case Arch_arm64: NotImplemented; break;
    default: InvalidPath; break;
  }
  if (reg_str.size == 0) {
    reg_str = push_str8f(arena, "%#llx", reg_id);
  }
  return reg_str;
}

internal String8
dw_string_from_cfa_opcode(DW_CFA_Opcode opcode)
{
  switch (opcode) {
#define X(_NAME, _ID, ...) case _ID: return str8_lit(Stringify(_NAME));
    DW_CFA_Kind_XList(X)
#undef X
  default: InvalidPath; break;
  }
  return str8_zero();
}
