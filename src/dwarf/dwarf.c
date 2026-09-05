// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Includes

#include "dwarf/generated/dwarf.meta.c"
#if defined(X64_H)
# include "dwarf/x64/dwarf_x64.c"
#endif
#if defined(ARM64_H)
# include "dwarf/arm64/dwarf_arm64.c"
#endif

////////////////////////////////
//~ rjf: X-List Helpers

//- rjf: format address sizes

internal U64
dw_addr_size_from_format(DW_Format fmt)
{
  U64 result = 0;
  switch(fmt)
  {
    default:{}break;
#define X(name, string, addr_size) case DW_Format_##name:{result = (addr_size);}break;
    DW_Format_XList
#undef X
  }
  return result;
}

//- rjf: attribute classes

internal DW_AttribClass
dw_attrib_class_from_kind(DW_Version version, DW_ExtFlags exts, DW_AttribKind k)
{
  DW_AttribClass result = 0;
  if(result == 0) switch(k)
  {
    default:{}break;
#define X(name, code, version, classes) case DW_AttribKind_##name:{result = (classes);}break;
    DW_AttribKind_XList
#undef X
  }
  for(B32 retry = 0; !result && retry <= 1; retry += 1)
  {
    if(result == 0 && exts & DW_ExtFlag_GNU) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_GNU_AttribKind_##name:{result = (classes);}break;
      DW_GNU_AttribKind_XList
#undef X
    }
    if(result == 0 && exts & DW_ExtFlag_LLVM) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_LLVM_AttribKind_##name:{result = (classes);}break;
      DW_LLVM_AttribKind_XList
#undef X
    }
    if(result == 0 && exts & DW_ExtFlag_Apple) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_Apple_AttribKind_##name:{result = (classes);}break;
      DW_Apple_AttribKind_XList
#undef X
    }
    if(result == 0 && exts & DW_ExtFlag_MIPS) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_MIPS_AttribKind_##name:{result = (classes);}break;
      DW_MIPS_AttribKind_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

internal DW_AttribClass
dw_attrib_class_from_form_kind(DW_Version version, DW_ExtFlags exts, DW_FormKind k)
{
  DW_AttribClass result = 0;
  if(result == 0) switch(k)
  {
    default:{}break;
#define X(name, code, version, classes) case DW_FormKind_##name:{result = (classes);}break;
    DW_FormKind_XList
#undef X
  }
  for(B32 retry = 0; !result && retry <= 1; retry += 1)
  {
    if(result == 0 && exts & DW_ExtFlag_GNU) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_GNU_FormKind_##name:{result = (classes);}break;
      DW_GNU_FormKind_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

//- rjf: expr op metadata

internal DW_ExprOpInfo *
dw_info_from_expr_op(DW_Version version, DW_ExtFlags exts, DW_ExprOp op)
{
  DW_ExprOpInfo *result = &dw_expr_op_info_nil;
  if(result == &dw_expr_op_info_nil) switch(op)
  {
    default:{}break;
#define X(name_, code, version_, operand_count_, pop_count_, push_count_, operand_kind_0, operand_kind_1, valid_for_cfa_exprs_) case DW_ExprOp_##name_:{local_persist DW_ExprOpInfo info = {.push_count = (push_count_), .pop_count = (pop_count_), .operand_count = (operand_count_), .valid_for_cfa_exprs = (valid_for_cfa_exprs_), .operand_kinds = {DW_ExprOperandKind_##operand_kind_0, DW_ExprOperandKind_##operand_kind_1}, .name = str8_lit_comp(#name_), .version = DW_Version_##version_}; result = &info;}break;
    DW_ExprOp_XList
#undef X
  }
  for(B32 retry = 0; (result == &dw_expr_op_info_nil) && retry <= 1; retry += 1)
  {
    if((result == &dw_expr_op_info_nil) && exts & DW_ExtFlag_GNU) switch(op)
    {
      default:{}break;
#define X(name_, code, operand_count_, pop_count_, push_count_, operand_kind_0, operand_kind_1, valid_for_cfa_exprs_) case DW_GNU_ExprOp_##name_:{local_persist DW_ExprOpInfo info = {.push_count = (push_count_), .pop_count = (pop_count_), .operand_count = (operand_count_), .valid_for_cfa_exprs = (valid_for_cfa_exprs_), .operand_kinds = {DW_ExprOperandKind_##operand_kind_0, DW_ExprOperandKind_##operand_kind_1}, .name = str8_lit_comp(#name_)}; result = &info;}break;
      DW_GNU_ExprOp_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

//- rjf: enum -> string

internal String8
dw_string_from_format(DW_Format fmt)
{
  String8 result = {0};
  switch(fmt)
  {
    default:{}break;
#define X(name, string, addr_size) case DW_Format_##name:{result = str8_lit(string);}break;
    DW_Format_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_tag_kind(DW_Version version, DW_ExtFlags exts, DW_TagKind k)
{
  String8 result = {0};
  if(result.size == 0) switch(k)
  {
    default:{}break;
#define X(name, code, version) case DW_TagKind_##name:{result = s(#name);}break;
    DW_TagKind_XList
#undef X
  }
  for(B32 retry = 0; !result.size && retry <= 1; retry += 1)
  {
    if(result.size == 0 && exts & DW_ExtFlag_GNU) switch(k)
    {
#define X(name, code) case DW_GNU_TagKind_##name:{result = s(#name);}break;
      DW_GNU_TagKind_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

internal String8
dw_string_from_attrib_kind(DW_Version version, DW_ExtFlags exts, DW_AttribKind k)
{
  String8 result = {0};
  if(result.size == 0) switch(k)
  {
    default:{}break;
#define X(name, code, version, classes) case DW_AttribKind_##name:{result = s(#name);}break;
    DW_AttribKind_XList
#undef X
  }
  for(B32 retry = 0; !result.size && retry <= 1; retry += 1)
  {
    if(result.size == 0 && exts & DW_ExtFlag_GNU) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_GNU_AttribKind_##name:{result = s(#name);}break;
      DW_GNU_AttribKind_XList
#undef X
    }
    if(result.size == 0 && exts & DW_ExtFlag_LLVM) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_LLVM_AttribKind_##name:{result = s(#name);}break;
      DW_LLVM_AttribKind_XList
#undef X
    }
    if(result.size == 0 && exts & DW_ExtFlag_Apple) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_Apple_AttribKind_##name:{result = s(#name);}break;
      DW_Apple_AttribKind_XList
#undef X
    }
    if(result.size == 0 && exts & DW_ExtFlag_MIPS) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_MIPS_AttribKind_##name:{result = s(#name);}break;
      DW_MIPS_AttribKind_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

internal String8
dw_string_from_form_kind(DW_Version version, DW_ExtFlags exts, DW_FormKind k)
{
  String8 result = {0};
  if(result.size == 0) switch(k)
  {
    default:{}break;
#define X(name, code, version, classes) case DW_FormKind_##name:{result = s(#name);}break;
    DW_FormKind_XList
#undef X
  }
  for(B32 retry = 0; !result.size && retry <= 1; retry += 1)
  {
    if(result.size == 0 && exts & DW_ExtFlag_GNU) switch(k)
    {
      default:{}break;
#define X(name, code, classes) case DW_GNU_FormKind_##name:{result = s(#name);}break;
      DW_GNU_FormKind_XList
#undef X
    }
    exts = DW_ExtFlag_All;
  }
  return result;
}

internal String8
dw_string_from_language(DW_Language k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_Language_##name:{result = s(#name);}break;
    DW_Language_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_comp_unit_kind(DW_CompUnitKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_CompUnitKind_##name:{result = s(#name);}break;
    DW_CompUnitKind_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_inl_kind(DW_InlKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_InlKind_##name:{result = s(#name);}break;
    DW_InlKind_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_access_kind(DW_AccessKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_AccessKind_##name:{result = s(#name);}break;
    DW_AccessKind_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_calling_convention_kind(DW_CallingConventionKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_CallingConventionKind_##name:{result = s(#name);}break;
    DW_CallingConventionKind_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_ate(DW_ATE k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_ATE_##name:{result = s(#name);}break;
    DW_ATE_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_visibility_kind(DW_VisibilityKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, ...) case DW_VisibilityKind_##name:{result = s(#name);}break;
    DW_VisibilityKind_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_std_opcode(DW_StdOpcode opcode)
{
  String8 result = {0};
  switch(opcode)
  {
    default:{}break;
#define X(name, ...) case DW_StdOpcode_##name:{result = s(#name);}break;
    DW_StdOpcode_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_ext_opcode(DW_ExtOpcode opcode)
{
  String8 result = {0};
  switch(opcode)
  {
    default:{}break;
#define X(name, ...) case DW_ExtOpcode_##name:{result = s(#name);}break;
    DW_ExtOpcode_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_lle(DW_LLE lle)
{
  String8 result = {0};
  switch(lle)
  {
    default:{}break;
#define X(name, ...) case DW_LLE_##name:{result = s(#name);}break;
    DW_LLE_XList
#undef X
  }
  return result;
}

internal String8
dw_string_from_rle(DW_RLE rle)
{
  String8 result = {0};
  switch(rle)
  {
    default:{}break;
#define X(name, ...) case DW_RLE_##name:{result = s(#name);}break;
    DW_RLE_XList
#undef X
  }
  return result;
}

internal String8
dw_regular_name_from_section_kind(DW_SectionKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, regular_name, ...) case DW_SectionKind_##name:{result = str8_lit(regular_name);}break;
    DW_SectionKind_XList
#undef X
  }
  return result;
}

internal String8
dw_macho_name_from_section_kind(DW_SectionKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, regular_name, macho_name, ...) case DW_SectionKind_##name:{result = str8_lit(macho_name);}break;
    DW_SectionKind_XList
#undef X
  }
  return result;
}

internal String8
dw_dwo_name_from_section_kind(DW_SectionKind k)
{
  String8 result = {0};
  switch(k)
  {
    default:{}break;
#define X(name, regular_name, macho_name, dwo_name, ...) case DW_SectionKind_##name:{result = str8_lit(dwo_name);}break;
    DW_SectionKind_XList
#undef X
  }
  return result;
}

//- rjf: architecture info

internal U64
dw_reg_count_from_arch(Arch arch)
{
  U64 result = 0;
  switch(arch)
  {
    default:{}break;
    case Arch_x64:{result = DW_RegCodeX64_LAST;}break;
    case Arch_arm64:{result = DW_RegCodeARM64_LAST;}break;
  }
  return result;
}
