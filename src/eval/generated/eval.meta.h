// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef EVAL_META_H
#define EVAL_META_H

typedef enum E_TokenKind
{
E_TokenKind_Null,
E_TokenKind_Identifier,
E_TokenKind_Numeric,
E_TokenKind_StringLiteral,
E_TokenKind_CharLiteral,
E_TokenKind_Symbol,
E_TokenKind_COUNT,
} E_TokenKind;

typedef enum E_TypeKind
{
E_TypeKind_Null,
E_TypeKind_Void,
E_TypeKind_Handle,
E_TypeKind_HResult,
E_TypeKind_Char8,
E_TypeKind_Char16,
E_TypeKind_Char32,
E_TypeKind_UChar8,
E_TypeKind_UChar16,
E_TypeKind_UChar32,
E_TypeKind_U8,
E_TypeKind_U16,
E_TypeKind_U32,
E_TypeKind_U64,
E_TypeKind_U128,
E_TypeKind_U256,
E_TypeKind_U512,
E_TypeKind_S8,
E_TypeKind_S16,
E_TypeKind_S32,
E_TypeKind_S64,
E_TypeKind_S128,
E_TypeKind_S256,
E_TypeKind_S512,
E_TypeKind_Bool,
E_TypeKind_F16,
E_TypeKind_F32,
E_TypeKind_F32PP,
E_TypeKind_F48,
E_TypeKind_F64,
E_TypeKind_F80,
E_TypeKind_F128,
E_TypeKind_ComplexF32,
E_TypeKind_ComplexF64,
E_TypeKind_ComplexF80,
E_TypeKind_ComplexF128,
E_TypeKind_Modifier,
E_TypeKind_Ptr,
E_TypeKind_LRef,
E_TypeKind_RRef,
E_TypeKind_Array,
E_TypeKind_Function,
E_TypeKind_Method,
E_TypeKind_MemberPtr,
E_TypeKind_Struct,
E_TypeKind_Class,
E_TypeKind_Union,
E_TypeKind_Enum,
E_TypeKind_Alias,
E_TypeKind_IncompleteStruct,
E_TypeKind_IncompleteUnion,
E_TypeKind_IncompleteClass,
E_TypeKind_IncompleteEnum,
E_TypeKind_Bitfield,
E_TypeKind_Variadic,
E_TypeKind_Collection,
E_TypeKind_COUNT,
E_TypeKind_FirstBasic      = E_TypeKind_Void,
E_TypeKind_LastBasic       = E_TypeKind_ComplexF128,
E_TypeKind_FirstInteger    = E_TypeKind_Char8,
E_TypeKind_LastInteger     = E_TypeKind_S512,
E_TypeKind_FirstSigned1    = E_TypeKind_Char8,
E_TypeKind_LastSigned1     = E_TypeKind_Char32,
E_TypeKind_FirstSigned2    = E_TypeKind_S8,
E_TypeKind_LastSigned2     = E_TypeKind_S512,
E_TypeKind_FirstIncomplete = E_TypeKind_IncompleteStruct,
E_TypeKind_LastIncomplete  = E_TypeKind_IncompleteEnum,
} E_TypeKind;

typedef U32 E_ExprKind;
typedef enum E_ExprKindEnum
{
E_ExprKind_Nil,
E_ExprKind_Ref,
E_ExprKind_ArrayIndex,
E_ExprKind_MemberAccess,
E_ExprKind_Deref,
E_ExprKind_Address,
E_ExprKind_Cast,
E_ExprKind_Sizeof,
E_ExprKind_ByteSwap,
E_ExprKind_Neg,
E_ExprKind_LogNot,
E_ExprKind_BitNot,
E_ExprKind_Mul,
E_ExprKind_Div,
E_ExprKind_Mod,
E_ExprKind_Add,
E_ExprKind_Sub,
E_ExprKind_LShift,
E_ExprKind_RShift,
E_ExprKind_Less,
E_ExprKind_LsEq,
E_ExprKind_Grtr,
E_ExprKind_GrEq,
E_ExprKind_EqEq,
E_ExprKind_NtEq,
E_ExprKind_BitAnd,
E_ExprKind_BitXor,
E_ExprKind_BitOr,
E_ExprKind_LogAnd,
E_ExprKind_LogOr,
E_ExprKind_Ternary,
E_ExprKind_LeafBytecode,
E_ExprKind_LeafMember,
E_ExprKind_LeafStringLiteral,
E_ExprKind_LeafU64,
E_ExprKind_LeafF64,
E_ExprKind_LeafF32,
E_ExprKind_LeafIdent,
E_ExprKind_LeafOffset,
E_ExprKind_LeafFilePath,
E_ExprKind_TypeIdent,
E_ExprKind_Ptr,
E_ExprKind_Array,
E_ExprKind_Func,
E_ExprKind_Define,
E_ExprKind_COUNT,
} E_ExprKindEnum;

typedef enum E_InterpretationCode
{
E_InterpretationCode_Good,
E_InterpretationCode_DivideByZero,
E_InterpretationCode_BadOp,
E_InterpretationCode_BadOpTypes,
E_InterpretationCode_BadMemRead,
E_InterpretationCode_BadRegRead,
E_InterpretationCode_BadFrameBase,
E_InterpretationCode_BadModuleBase,
E_InterpretationCode_BadTLSBase,
E_InterpretationCode_InsufficientStackSpace,
E_InterpretationCode_MalformedBytecode,
E_InterpretationCode_COUNT,
} E_InterpretationCode;

C_LINKAGE_BEGIN
extern String8 e_token_kind_strings[6];
extern String8 e_expr_kind_strings[45];
extern String8 e_interpretation_code_display_strings[11];
extern E_OpInfo e_expr_kind_op_info_table[45];
extern U8 e_kind_basic_byte_size_table[56];
extern String8 e_kind_basic_string_table[56];

C_LINKAGE_END

#endif // EVAL_META_H
