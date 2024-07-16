// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef EVAL_META_H
#define EVAL_META_H

typedef U32 EVAL_ExprKind;
typedef enum EVAL_ExprKindEnum
{
EVAL_ExprKind_Nil,
EVAL_ExprKind_ArrayIndex,
EVAL_ExprKind_MemberAccess,
EVAL_ExprKind_Deref,
EVAL_ExprKind_Address,
EVAL_ExprKind_Cast,
EVAL_ExprKind_Sizeof,
EVAL_ExprKind_Neg,
EVAL_ExprKind_LogNot,
EVAL_ExprKind_BitNot,
EVAL_ExprKind_Mul,
EVAL_ExprKind_Div,
EVAL_ExprKind_Mod,
EVAL_ExprKind_Add,
EVAL_ExprKind_Sub,
EVAL_ExprKind_LShift,
EVAL_ExprKind_RShift,
EVAL_ExprKind_Less,
EVAL_ExprKind_LsEq,
EVAL_ExprKind_Grtr,
EVAL_ExprKind_GrEq,
EVAL_ExprKind_EqEq,
EVAL_ExprKind_NtEq,
EVAL_ExprKind_BitAnd,
EVAL_ExprKind_BitXor,
EVAL_ExprKind_BitOr,
EVAL_ExprKind_LogAnd,
EVAL_ExprKind_LogOr,
EVAL_ExprKind_Ternary,
EVAL_ExprKind_LeafBytecode,
EVAL_ExprKind_LeafMember,
EVAL_ExprKind_LeafU64,
EVAL_ExprKind_LeafF64,
EVAL_ExprKind_LeafF32,
EVAL_ExprKind_TypeIdent,
EVAL_ExprKind_Ptr,
EVAL_ExprKind_Array,
EVAL_ExprKind_Func,
EVAL_ExprKind_Define,
EVAL_ExprKind_LeafIdent,
EVAL_ExprKind_COUNT,
} EVAL_ExprKindEnum;

typedef enum EVAL_ResultCode
{
EVAL_ResultCode_Good,
EVAL_ResultCode_DivideByZero,
EVAL_ResultCode_BadOp,
EVAL_ResultCode_BadOpTypes,
EVAL_ResultCode_BadMemRead,
EVAL_ResultCode_BadRegRead,
EVAL_ResultCode_BadFrameBase,
EVAL_ResultCode_BadModuleBase,
EVAL_ResultCode_BadTLSBase,
EVAL_ResultCode_InsufficientStackSpace,
EVAL_ResultCode_MalformedBytecode,
EVAL_ResultCode_COUNT,
} EVAL_ResultCode;

C_LINKAGE_BEGIN
extern U8 eval_expr_kind_child_counts[40];
extern String8 eval_expr_kind_strings[40];
extern String8 eval_result_code_display_strings[11];
extern String8 eval_expr_op_strings[40];

C_LINKAGE_END

#endif // EVAL_META_H
