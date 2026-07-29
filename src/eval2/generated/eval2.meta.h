// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef EVAL2_META_H
#define EVAL2_META_H

typedef enum E2_ExprKind
{
E2_ExprKind_Null,
E2_ExprKind_Identifier,
E2_ExprKind_MacroArg,
E2_ExprKind_TypeIdentifier,
E2_ExprKind_Numeric,
E2_ExprKind_StringLiteral,
E2_ExprKind_CharLiteral,
E2_ExprKind_Ptr,
E2_ExprKind_Array,
E2_ExprKind_Function,
E2_ExprKind_Const,
E2_ExprKind_Volatile,
E2_ExprKind_Unsigned,
E2_ExprKind_Signed,
E2_ExprKind_Dot,
E2_ExprKind_Index,
E2_ExprKind_Call,
E2_ExprKind_DerefAsm,
E2_ExprKind_SizeOf,
E2_ExprKind_TypeOf,
E2_ExprKind_Cast,
E2_ExprKind_Deref,
E2_ExprKind_Address,
E2_ExprKind_Pos,
E2_ExprKind_Neg,
E2_ExprKind_LogNot,
E2_ExprKind_BitNot,
E2_ExprKind_Mul,
E2_ExprKind_Div,
E2_ExprKind_Mod,
E2_ExprKind_Add,
E2_ExprKind_Sub,
E2_ExprKind_LShift,
E2_ExprKind_RShift,
E2_ExprKind_Less,
E2_ExprKind_LtEq,
E2_ExprKind_Grtr,
E2_ExprKind_GrEq,
E2_ExprKind_EqEq,
E2_ExprKind_NtEq,
E2_ExprKind_BitAnd,
E2_ExprKind_BitXor,
E2_ExprKind_BitOr,
E2_ExprKind_LogAnd,
E2_ExprKind_LogOr,
E2_ExprKind_Define,
E2_ExprKind_Macro,
E2_ExprKind_Cond,
E2_ExprKind_COUNT,
} E2_ExprKind;

typedef enum E2_LangKind
{
E2_LangKind_CLike,
} E2_LangKind;

typedef enum E2_TypeKind
{
E2_TypeKind_Null,
E2_TypeKind_Void,
E2_TypeKind_Handle,
E2_TypeKind_HResult,
E2_TypeKind_Char8,
E2_TypeKind_Char16,
E2_TypeKind_Char32,
E2_TypeKind_UChar8,
E2_TypeKind_UChar16,
E2_TypeKind_UChar32,
E2_TypeKind_U8,
E2_TypeKind_U16,
E2_TypeKind_U32,
E2_TypeKind_U64,
E2_TypeKind_U128,
E2_TypeKind_U256,
E2_TypeKind_U512,
E2_TypeKind_S8,
E2_TypeKind_S16,
E2_TypeKind_S32,
E2_TypeKind_S64,
E2_TypeKind_S128,
E2_TypeKind_S256,
E2_TypeKind_S512,
E2_TypeKind_Bool,
E2_TypeKind_F16,
E2_TypeKind_F32,
E2_TypeKind_F32PP,
E2_TypeKind_F48,
E2_TypeKind_F64,
E2_TypeKind_F80,
E2_TypeKind_F128,
E2_TypeKind_ComplexF32,
E2_TypeKind_ComplexF64,
E2_TypeKind_ComplexF80,
E2_TypeKind_ComplexF128,
E2_TypeKind_Modifier,
E2_TypeKind_Ptr,
E2_TypeKind_LRef,
E2_TypeKind_RRef,
E2_TypeKind_Array,
E2_TypeKind_Function,
E2_TypeKind_Method,
E2_TypeKind_MemberPtr,
E2_TypeKind_Struct,
E2_TypeKind_Class,
E2_TypeKind_Union,
E2_TypeKind_Enum,
E2_TypeKind_Alias,
E2_TypeKind_IncompleteStruct,
E2_TypeKind_IncompleteUnion,
E2_TypeKind_IncompleteClass,
E2_TypeKind_IncompleteEnum,
E2_TypeKind_Bitfield,
E2_TypeKind_Variadic,
E2_TypeKind_Set,
E2_TypeKind_Lens,
E2_TypeKind_LensSpec,
E2_TypeKind_MetaExpr,
E2_TypeKind_MetaDisplayName,
E2_TypeKind_MetaDescription,
E2_TypeKind_COUNT,
E2_TypeKind_FirstBasic      = E2_TypeKind_Void,
E2_TypeKind_LastBasic       = E2_TypeKind_ComplexF128,
E2_TypeKind_FirstInteger    = E2_TypeKind_Char8,
E2_TypeKind_LastInteger     = E2_TypeKind_S512,
E2_TypeKind_FirstSigned1    = E2_TypeKind_Char8,
E2_TypeKind_LastSigned1     = E2_TypeKind_Char32,
E2_TypeKind_FirstSigned2    = E2_TypeKind_S8,
E2_TypeKind_LastSigned2     = E2_TypeKind_S512,
E2_TypeKind_FirstIncomplete = E2_TypeKind_IncompleteStruct,
E2_TypeKind_LastIncomplete  = E2_TypeKind_IncompleteEnum,
E2_TypeKind_FirstMeta       = E2_TypeKind_MetaExpr,
E2_TypeKind_LastMeta        = E2_TypeKind_MetaDescription,
} E2_TypeKind;

typedef struct E2_ExprKindParseInfo E2_ExprKindParseInfo;
struct E2_ExprKindParseInfo
{
E2_ExprKind expr_kind;
E2_ExprParseKind parse_kind;
S64 precedence;
B8 reverse_children;
String8 pre;
String8 sep;
String8 post;
String8 chain;
};

typedef struct E2_LangInfo E2_LangInfo;
struct E2_LangInfo
{
U64 expr_kind_parse_infos_count;
E2_ExprKindParseInfo *expr_kind_parse_infos;
};

C_LINKAGE_BEGIN
extern B8 e2_expr_kind_allow_type_operands_table[48];
extern B8 e2_expr_kind_is_type_expr_table[48];
extern B8 e2_expr_kind_is_first_operand_type_maybe_table[48];
extern U64 e2_expr_kind_target_operand_count_table[48];
extern E2_LangInfo e2_lang_kind_info_table[1];
extern E2_ExprKindParseInfo e2_expr_kind_parse_info_table__clike[43];
extern U8 e2_type_kind_basic_byte_size_table[61];
extern String8 e2_type_kind_basic_string_table[61];

C_LINKAGE_END

#endif // EVAL2_META_H
