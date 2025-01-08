// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
dw_offset_size_from_mode(DW_Mode mode)
{
  U64 result = 0;
  switch (mode) {
    case DW_Mode_Null: break;
    case DW_Mode_32Bit: result = 4; break;
    case DW_Mode_64Bit: result = 8; break;
    default: InvalidPath; break;
  }
  return result;
}

internal DW_AttribClass
dw_pick_attrib_value_class(DW_Version ver, DW_Ext ext, DW_Language lang, B32 relaxed, DW_AttribKind attrib_kind, DW_FormKind form_kind)
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

  if (attrib_kind != DW_Attrib_Null && form_kind != DW_Form_Null) {
    //Assert(result != DW_AttribClass_Null && result != DW_AttribClass_Undefined);
  }

  return result;
}

