// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ RAD Debug Info, (R)AD(D)BG(I) Format Library
//
// Defines standard RDI debug information format types and
// functions.

#ifndef RDI_FORMAT_C
#define RDI_FORMAT_C

RDI_U8 rdi_eval_op_ctrlbits_table[45] =
{
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(1, 1, 0),
RDI_EVAL_CTRLBITS(1, 0, 0),
RDI_EVAL_CTRLBITS(1, 1, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(0, 1, 1),
RDI_EVAL_CTRLBITS(1, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(0, 0, 0),
RDI_EVAL_CTRLBITS(1, 0, 1),
RDI_EVAL_CTRLBITS(2, 0, 1),
RDI_EVAL_CTRLBITS(4, 0, 1),
RDI_EVAL_CTRLBITS(8, 0, 1),
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
{(RDI_U8 *)"Other", sizeof("Other")},
{(RDI_U8 *)"U", sizeof("U")},
{(RDI_U8 *)"S", sizeof("S")},
{(RDI_U8 *)"F32", sizeof("F32")},
{(RDI_U8 *)"F64", sizeof("F64")},
{(RDI_U8 *)"COUNT", sizeof("COUNT")},
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

RDI_PROC RDI_U32
rdi_size_from_basic_type_kind(RDI_TypeKind kind)
{
RDI_U32 result = 0;
switch(kind)
{
default:{}break;
case RDI_TypeKind_Handle:{result = 0xFFFFFFFF;}break;
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
