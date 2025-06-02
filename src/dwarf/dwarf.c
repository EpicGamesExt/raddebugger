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

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_v2(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_V2_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_v3(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_V3_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_v4(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_V4_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_v5(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_V5_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_gnu(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_GNU_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_llvm(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_LLVM_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_apple(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_APPLE_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind_mips(DW_AttribKind k)
{
  switch (k) {
#define X(_N,_C) case DW_Attrib_##_N: return _C;
    DW_AttribKind_ClassFlags_MIPS_XList(X)
#undef X
  }
  return DW_AttribClass_Null;
}

internal DW_AttribClass
dw_attrib_class_from_attrib_kind(DW_Version ver, DW_Ext ext, DW_AttribKind k)
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
      case DW_Ext_GNU:   result = dw_attrib_class_from_attrib_kind_gnu(k);   break;
      case DW_Ext_LLVM:  result = dw_attrib_class_from_attrib_kind_llvm(k);  break;
      case DW_Ext_APPLE: result = dw_attrib_class_from_attrib_kind_apple(k); break;
      case DW_Ext_MIPS:  result = dw_attrib_class_from_attrib_kind_mips(k);  break;
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
      case DW_Version_2:    result = dw_attrib_class_from_attrib_kind_v2(k); break;
      case DW_Version_3:    result = dw_attrib_class_from_attrib_kind_v3(k); break;
      case DW_Version_4:    result = dw_attrib_class_from_attrib_kind_v4(k); break;
      case DW_Version_5:    result = dw_attrib_class_from_attrib_kind_v5(k); break;
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
  
  DW_AttribClass attrib_class = dw_attrib_class_from_attrib_kind(ver, ext, attrib_kind);
  DW_AttribClass form_class   = dw_attrib_class_from_form_kind(ver, form_kind);
  
  if(relaxed)
  {
    if(attrib_class == DW_AttribClass_Null || form_class == DW_AttribClass_Null)
    {
      attrib_class = dw_attrib_class_from_attrib_kind(DW_Version_Last, ext, attrib_kind);
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

