// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ RAD Debug Info, (R)AD(D)BG(I) Format Library
//
// Defines standard RDI debug information format types and
// functions.

#ifndef RDI_FORMAT_C
#define RDI_FORMAT_C

RDI_U16 rdi_section_element_size_table[37] =
{
sizeof(RDI_U8),
sizeof(RDI_TopLevelInfo),
sizeof(RDI_U8),
sizeof(RDI_U32),
sizeof(RDI_U32),
sizeof(RDI_BinarySection),
sizeof(RDI_FilePathNode),
sizeof(RDI_SourceFile),
sizeof(RDI_LineTable),
sizeof(RDI_U64),
sizeof(RDI_Line),
sizeof(RDI_Column),
sizeof(RDI_SourceLineMap),
sizeof(RDI_U32),
sizeof(RDI_U32),
sizeof(RDI_U64),
sizeof(RDI_Unit),
sizeof(RDI_VMapEntry),
sizeof(RDI_TypeNode),
sizeof(RDI_UDT),
sizeof(RDI_Member),
sizeof(RDI_EnumMember),
sizeof(RDI_GlobalVariable),
sizeof(RDI_VMapEntry),
sizeof(RDI_ThreadVariable),
sizeof(RDI_Procedure),
sizeof(RDI_Scope),
sizeof(RDI_U64),
sizeof(RDI_VMapEntry),
sizeof(RDI_InlineSite),
sizeof(RDI_Local),
sizeof(RDI_LocationBlock),
sizeof(RDI_U8),
sizeof(RDI_NameMap),
sizeof(RDI_NameMapBucket),
sizeof(RDI_NameMapNode),
sizeof(RDI_U8),
};

RDI_U8 rdi_section_is_required_table[37] =
{
0,
0,
1,
1,
1,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
};

RDI_U16 rdi_eval_op_ctrlbits_table[52] =
{
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(1, 1, 0),
RDI_EVAL_CTRLBITS(2, 0, 0),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(0, 1, 1),
RDI_EVAL_CTRLBITS(8, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(1, 0, 1),
RDI_EVAL_CTRLBITS(2, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(8, 0, 1),
RDI_EVAL_CTRLBITS(16, 0, 1),
RDI_EVAL_CTRLBITS(1, 0, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(2, 1, 1),
RDI_EVAL_CTRLBITS(1, 0, 1),
RDI_EVAL_CTRLBITS(0, 1, 0),
RDI_EVAL_CTRLBITS(1, 0, 0),
RDI_EVAL_CTRLBITS(1, 2, 1),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(4, 0, 0),
RDI_EVAL_CTRLBITS(4, 0, 0),
RDI_EVAL_CTRLBITS(8, 0, 0),
RDI_EVAL_CTRLBITS(0, 0, 0),
};

struct {RDI_EvalConversionKind dst_typegroups[RDI_EvalTypeGroup_COUNT];} rdi_eval_typegroup_conversion_kind_matrix[6] =
{
{{RDI_EvalConversionKind_OtherToOther, RDI_EvalConversionKind_FromOther, RDI_EvalConversionKind_FromOther, RDI_EvalConversionKind_FromOther, RDI_EvalConversionKind_FromOther}},
{{RDI_EvalConversionKind_ToOther, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Legal}},
{{RDI_EvalConversionKind_ToOther, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Legal}},
{{RDI_EvalConversionKind_ToOther, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Legal}},
{{RDI_EvalConversionKind_ToOther, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Legal, RDI_EvalConversionKind_Noop}},
{{RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop, RDI_EvalConversionKind_Noop}},
};

struct {RDI_U8 *str; RDI_U64 size;} rdi_eval_conversion_kind_message_string_table[6] =
{
{(RDI_U8 *)"", sizeof("")},
{(RDI_U8 *)"", sizeof("")},
{(RDI_U8 *)"Cannot convert between these types.", sizeof("Cannot convert between these types.")},
{(RDI_U8 *)"Cannot convert to this type.", sizeof("Cannot convert to this type.")},
{(RDI_U8 *)"Cannot convert this type.", sizeof("Cannot convert this type.")},
{(RDI_U8 *)"", sizeof("")},
};

RDI_PROC RDI_U64
rdi_hash(RDI_U8 *ptr, RDI_U64 size)
{
  RDI_U64 result = 5381;
  RDI_U8 *opl = ptr + size;
  for(;ptr < opl; ptr += 1)
  {
    result = ((result << 5) + result) + *ptr;
  }
  return result;
}

RDI_PROC RDI_U8 *
rdi_string_from_type_kind(RDI_TypeKind kind, RDI_U64 *size_out)
{
RDI_U8 *result = 0;
*size_out = 0;
switch (kind)
{
default:{}break;
case RDI_TypeKind_NULL: {result = (RDI_U8*)"NULL"; *size_out = sizeof("NULL")-1;}break;
case RDI_TypeKind_Void: {result = (RDI_U8*)"Void"; *size_out = sizeof("Void")-1;}break;
case RDI_TypeKind_Handle: {result = (RDI_U8*)"Handle"; *size_out = sizeof("Handle")-1;}break;
case RDI_TypeKind_HResult: {result = (RDI_U8*)"HResult"; *size_out = sizeof("HResult")-1;}break;
case RDI_TypeKind_Char8: {result = (RDI_U8*)"Char8"; *size_out = sizeof("Char8")-1;}break;
case RDI_TypeKind_Char16: {result = (RDI_U8*)"Char16"; *size_out = sizeof("Char16")-1;}break;
case RDI_TypeKind_Char32: {result = (RDI_U8*)"Char32"; *size_out = sizeof("Char32")-1;}break;
case RDI_TypeKind_UChar8: {result = (RDI_U8*)"UChar8"; *size_out = sizeof("UChar8")-1;}break;
case RDI_TypeKind_UChar16: {result = (RDI_U8*)"UChar16"; *size_out = sizeof("UChar16")-1;}break;
case RDI_TypeKind_UChar32: {result = (RDI_U8*)"UChar32"; *size_out = sizeof("UChar32")-1;}break;
case RDI_TypeKind_U8: {result = (RDI_U8*)"U8"; *size_out = sizeof("U8")-1;}break;
case RDI_TypeKind_U16: {result = (RDI_U8*)"U16"; *size_out = sizeof("U16")-1;}break;
case RDI_TypeKind_U32: {result = (RDI_U8*)"U32"; *size_out = sizeof("U32")-1;}break;
case RDI_TypeKind_U64: {result = (RDI_U8*)"U64"; *size_out = sizeof("U64")-1;}break;
case RDI_TypeKind_U128: {result = (RDI_U8*)"U128"; *size_out = sizeof("U128")-1;}break;
case RDI_TypeKind_U256: {result = (RDI_U8*)"U256"; *size_out = sizeof("U256")-1;}break;
case RDI_TypeKind_U512: {result = (RDI_U8*)"U512"; *size_out = sizeof("U512")-1;}break;
case RDI_TypeKind_S8: {result = (RDI_U8*)"S8"; *size_out = sizeof("S8")-1;}break;
case RDI_TypeKind_S16: {result = (RDI_U8*)"S16"; *size_out = sizeof("S16")-1;}break;
case RDI_TypeKind_S32: {result = (RDI_U8*)"S32"; *size_out = sizeof("S32")-1;}break;
case RDI_TypeKind_S64: {result = (RDI_U8*)"S64"; *size_out = sizeof("S64")-1;}break;
case RDI_TypeKind_S128: {result = (RDI_U8*)"S128"; *size_out = sizeof("S128")-1;}break;
case RDI_TypeKind_S256: {result = (RDI_U8*)"S256"; *size_out = sizeof("S256")-1;}break;
case RDI_TypeKind_S512: {result = (RDI_U8*)"S512"; *size_out = sizeof("S512")-1;}break;
case RDI_TypeKind_Bool: {result = (RDI_U8*)"Bool"; *size_out = sizeof("Bool")-1;}break;
case RDI_TypeKind_F16: {result = (RDI_U8*)"F16"; *size_out = sizeof("F16")-1;}break;
case RDI_TypeKind_F32: {result = (RDI_U8*)"F32"; *size_out = sizeof("F32")-1;}break;
case RDI_TypeKind_F32PP: {result = (RDI_U8*)"F32PP"; *size_out = sizeof("F32PP")-1;}break;
case RDI_TypeKind_F48: {result = (RDI_U8*)"F48"; *size_out = sizeof("F48")-1;}break;
case RDI_TypeKind_F64: {result = (RDI_U8*)"F64"; *size_out = sizeof("F64")-1;}break;
case RDI_TypeKind_F80: {result = (RDI_U8*)"F80"; *size_out = sizeof("F80")-1;}break;
case RDI_TypeKind_F128: {result = (RDI_U8*)"F128"; *size_out = sizeof("F128")-1;}break;
case RDI_TypeKind_ComplexF32: {result = (RDI_U8*)"ComplexF32"; *size_out = sizeof("ComplexF32")-1;}break;
case RDI_TypeKind_ComplexF64: {result = (RDI_U8*)"ComplexF64"; *size_out = sizeof("ComplexF64")-1;}break;
case RDI_TypeKind_ComplexF80: {result = (RDI_U8*)"ComplexF80"; *size_out = sizeof("ComplexF80")-1;}break;
case RDI_TypeKind_ComplexF128: {result = (RDI_U8*)"ComplexF128"; *size_out = sizeof("ComplexF128")-1;}break;
case RDI_TypeKind_Modifier: {result = (RDI_U8*)"Modifier"; *size_out = sizeof("Modifier")-1;}break;
case RDI_TypeKind_Ptr: {result = (RDI_U8*)"Ptr"; *size_out = sizeof("Ptr")-1;}break;
case RDI_TypeKind_LRef: {result = (RDI_U8*)"LRef"; *size_out = sizeof("LRef")-1;}break;
case RDI_TypeKind_RRef: {result = (RDI_U8*)"RRef"; *size_out = sizeof("RRef")-1;}break;
case RDI_TypeKind_Array: {result = (RDI_U8*)"Array"; *size_out = sizeof("Array")-1;}break;
case RDI_TypeKind_Function: {result = (RDI_U8*)"Function"; *size_out = sizeof("Function")-1;}break;
case RDI_TypeKind_Method: {result = (RDI_U8*)"Method"; *size_out = sizeof("Method")-1;}break;
case RDI_TypeKind_MemberPtr: {result = (RDI_U8*)"MemberPtr"; *size_out = sizeof("MemberPtr")-1;}break;
case RDI_TypeKind_Struct: {result = (RDI_U8*)"Struct"; *size_out = sizeof("Struct")-1;}break;
case RDI_TypeKind_Class: {result = (RDI_U8*)"Class"; *size_out = sizeof("Class")-1;}break;
case RDI_TypeKind_Union: {result = (RDI_U8*)"Union"; *size_out = sizeof("Union")-1;}break;
case RDI_TypeKind_Enum: {result = (RDI_U8*)"Enum"; *size_out = sizeof("Enum")-1;}break;
case RDI_TypeKind_Alias: {result = (RDI_U8*)"Alias"; *size_out = sizeof("Alias")-1;}break;
case RDI_TypeKind_IncompleteStruct: {result = (RDI_U8*)"IncompleteStruct"; *size_out = sizeof("IncompleteStruct")-1;}break;
case RDI_TypeKind_IncompleteUnion: {result = (RDI_U8*)"IncompleteUnion"; *size_out = sizeof("IncompleteUnion")-1;}break;
case RDI_TypeKind_IncompleteClass: {result = (RDI_U8*)"IncompleteClass"; *size_out = sizeof("IncompleteClass")-1;}break;
case RDI_TypeKind_IncompleteEnum: {result = (RDI_U8*)"IncompleteEnum"; *size_out = sizeof("IncompleteEnum")-1;}break;
case RDI_TypeKind_Bitfield: {result = (RDI_U8*)"Bitfield"; *size_out = sizeof("Bitfield")-1;}break;
case RDI_TypeKind_Variadic: {result = (RDI_U8*)"Variadic"; *size_out = sizeof("Variadic")-1;}break;
case RDI_TypeKind_Count: {result = (RDI_U8*)"Count"; *size_out = sizeof("Count")-1;}break;
}
return result;
}

RDI_PROC RDI_U32
rdi_size_from_basic_type_kind(RDI_TypeKind kind)
{
RDI_U32 result = 0;
switch(kind)
{
default:{}break;
case RDI_TypeKind_Handle:{result = 0xFFFFFFFF;}break;
case RDI_TypeKind_HResult:{result = 4;}break;
case RDI_TypeKind_Char8:{result = 1;}break;
case RDI_TypeKind_Char16:{result = 2;}break;
case RDI_TypeKind_Char32:{result = 4;}break;
case RDI_TypeKind_UChar8:{result = 1;}break;
case RDI_TypeKind_UChar16:{result = 2;}break;
case RDI_TypeKind_UChar32:{result = 4;}break;
case RDI_TypeKind_U8:{result = 1;}break;
case RDI_TypeKind_U16:{result = 2;}break;
case RDI_TypeKind_U32:{result = 4;}break;
case RDI_TypeKind_U64:{result = 8;}break;
case RDI_TypeKind_U128:{result = 16;}break;
case RDI_TypeKind_U256:{result = 32;}break;
case RDI_TypeKind_U512:{result = 64;}break;
case RDI_TypeKind_S8:{result = 1;}break;
case RDI_TypeKind_S16:{result = 2;}break;
case RDI_TypeKind_S32:{result = 4;}break;
case RDI_TypeKind_S64:{result = 8;}break;
case RDI_TypeKind_S128:{result = 16;}break;
case RDI_TypeKind_S256:{result = 32;}break;
case RDI_TypeKind_S512:{result = 64;}break;
case RDI_TypeKind_Bool:{result = 1;}break;
case RDI_TypeKind_F16:{result = 2;}break;
case RDI_TypeKind_F32:{result = 4;}break;
case RDI_TypeKind_F32PP:{result = 4;}break;
case RDI_TypeKind_F48:{result = 6;}break;
case RDI_TypeKind_F64:{result = 8;}break;
case RDI_TypeKind_F80:{result = 10;}break;
case RDI_TypeKind_F128:{result = 16;}break;
case RDI_TypeKind_ComplexF32:{result = 8;}break;
case RDI_TypeKind_ComplexF64:{result = 16;}break;
case RDI_TypeKind_ComplexF80:{result = 20;}break;
case RDI_TypeKind_ComplexF128:{result = 32;}break;
}
return result;
}

RDI_PROC RDI_U32
rdi_addr_size_from_arch(RDI_Arch arch)
{
RDI_U32 result = 0;
switch(arch)
{
default:{}break;
case RDI_Arch_X86:{result = 4;}break;
case RDI_Arch_X64:{result = 8;}break;
}
return result;
}

RDI_PROC RDI_EvalConversionKind
rdi_eval_conversion_kind_from_typegroups(RDI_EvalTypeGroup in, RDI_EvalTypeGroup out)
{
  RDI_EvalConversionKind k = rdi_eval_typegroup_conversion_kind_matrix[in].dst_typegroups[out];
  return k;
}

RDI_PROC RDI_S32
rdi_eval_op_typegroup_are_compatible(RDI_EvalOp op, RDI_EvalTypeGroup group)
{
  RDI_S32 result = 0;
  switch(op)
  {
    case RDI_EvalOp_Neg: case RDI_EvalOp_Add: case RDI_EvalOp_Sub:
    case RDI_EvalOp_Mul: case RDI_EvalOp_Div:
    case RDI_EvalOp_EqEq:case RDI_EvalOp_NtEq:
    case RDI_EvalOp_LsEq:case RDI_EvalOp_GrEq:
    case RDI_EvalOp_Less:case RDI_EvalOp_Grtr:
    {
      if(group != RDI_EvalTypeGroup_Other)
      {
        result = 1;
      }
    }break;
    case RDI_EvalOp_Mod:case RDI_EvalOp_LShift:case RDI_EvalOp_RShift:
    case RDI_EvalOp_BitNot:case RDI_EvalOp_BitAnd:case RDI_EvalOp_BitXor:
    case RDI_EvalOp_BitOr:case RDI_EvalOp_LogNot:case RDI_EvalOp_LogAnd:
    case RDI_EvalOp_LogOr: 
    {
      if(group == RDI_EvalTypeGroup_S || group == RDI_EvalTypeGroup_U)
      {
        result = 1;
      }
    }break;
  }
  return result;
}

RDI_PROC RDI_U8 *
rdi_explanation_string_from_eval_conversion_kind(RDI_EvalConversionKind kind, RDI_U64 *size_out)
{
  *size_out = rdi_eval_conversion_kind_message_string_table[kind].size;
  return rdi_eval_conversion_kind_message_string_table[kind].str;
}

#endif // RDI_FORMAT_C
