// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_EXPR_H
#define DWARF_EXPR_H

typedef struct DW_ExprContext
{
  Arch      arch;
  DW_Format format;
  U64       cfa;
  U64       tls_base;
} DW_ExprContext;

#define DW_ExprValueType_IsSigned(x)   ((x) == DW_ExprValueType_S8 || (x) == DW_ExprValueType_S16 || (x) == DW_ExprValueType_S32 || (x) == DW_ExprValueType_S64 || (x) == DW_ExprValueType_S128 || (x) == DW_ExprValueType_S256 || (x) == DW_ExprValueType_S512)
#define DW_ExprValueType_IsUnsigned(x) ((x) == DW_ExprValueType_U8 || (x) == DW_ExprValueType_U16 || (x) == DW_ExprValueType_U32 || (x) == DW_ExprValueType_U64 || (x) == DW_ExprValueType_U128 || (x) == DW_ExprValueType_U256 || (x) == DW_ExprValueType_U512)
#define DW_ExprValueType_IsFloat(x)    ((x) == DW_ExprValueType_F32 || (x) == DW_ExprValueType_F64)
#define DW_ExprValueType_IsInt(x)      (DW_ExprValueType_IsSigned(x) || DW_ExprValueType_IsUnsigned(x) || (x) == DW_ExprValueType_Addr)
typedef enum
{
  DW_ExprValueType_Generic,
  DW_ExprValueType_U8,
  DW_ExprValueType_U16,
  DW_ExprValueType_U32,
  DW_ExprValueType_U64,
  DW_ExprValueType_U128,
  DW_ExprValueType_U256,
  DW_ExprValueType_U512,
  DW_ExprValueType_S8,
  DW_ExprValueType_S16,
  DW_ExprValueType_S32,
  DW_ExprValueType_S64,
  DW_ExprValueType_S128,
  DW_ExprValueType_S256,
  DW_ExprValueType_S512,
  DW_ExprValueType_F32,
  DW_ExprValueType_F64,
  DW_ExprValueType_Addr,
  DW_ExprValueType_Implicit,
  DW_ExprValueType_Bool,
} DW_ExprValueType;

typedef S8 DW_ExprBool;

typedef struct DW_ExprValue
{
  DW_ExprValueType type;
  union {
    U8          u8;
    U16         u16;
    U32         u32;
    U64         u64;
    U128        u128;
    U256        u256;
    U512        u512;
    S8          s8;
    S16         s16;
    S32         s32;
    S64         s64;
    F32         f32;
    F64         f64;
    U64         addr;
    DW_ExprBool boolean;
    String8     implicit;
    String8     generic;
  };
} DW_ExprValue;

typedef struct DW_ExprValueNode
{
  DW_ExprValue v;
  struct DW_ExprValueNode *next;
} DW_ExprValueNode;

typedef struct DW_ExprStack
{
  U64               count;
  DW_ExprValueNode *top;
} DW_ExprStack;

typedef enum
{
  DW_PieceKind_Null,
  DW_PieceKind_Value,
  DW_PieceKind_Undefined,
} DW_PieceKind;

typedef struct DW_Piece
{
  DW_PieceKind kind;
  union {
    U64 undef_bit_size;
    struct {
      U64   bit_size;
      void *ptr;
    } value;
  };
} DW_Piece;

typedef struct DW_PieceNode
{
  DW_Piece v;
  struct DW_PieceNode *next;
} DW_PieceNode;

typedef struct DW_PieceList
{
  U64           count;
  DW_PieceNode *first;
  DW_PieceNode *last;
} DW_PieceList;

typedef struct DW_ExprResult
{
  int x;
} DW_ExprResult;

////////////////////////////////

// pieces
internal DW_PieceNode * dw_piece_list_push(Arena *arena, DW_PieceList *list, DW_Piece v);

// size -> type
internal DW_ExprValueType dw_expr_unsigned_value_type_from_bit_size(U64 bit_size);
internal DW_ExprValueType dw_expr_signed_value_type_from_bit_size(U64 bit_size);
internal DW_ExprValueType dw_expr_float_type_from_bit_size(U64 bit_size);

// type -> size
internal U64 dw_expr_byte_size_from_value_type(U64 addr_size, DW_ExprValueType k);

// typer
internal DW_ExprValueType dw_expr_pick_common_value_type(DW_ExprValueType lhs, DW_ExprValueType rhs);
internal DW_ExprValueType dw_expr_pick_common_compar_value_type(DW_ExprValueType lhs, DW_ExprValueType rhs);
internal DW_ExprValue     dw_expr_cast(DW_ExprValue value, DW_ExprValueType type);

// arithmetic operators
internal DW_ExprValue dw_expr_add(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_minus(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_mul(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_div(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_mod(DW_ExprValue lhs, DW_ExprValue rhs);

// comparison operators
internal DW_ExprValue dw_expr_eq(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_ge(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_gt(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_le(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_lt(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_ne(DW_ExprValue lhs, DW_ExprValue rhs);

// bitwise operators
internal DW_ExprValue dw_expr_xor(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_and(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_or(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_shl(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_shr(DW_ExprValue lhs, DW_ExprValue rhs);
internal DW_ExprValue dw_expr_shra(DW_ExprValue lhs, DW_ExprValue rhs);

// unary operators
internal DW_ExprValue dw_expr_abs(DW_ExprValue value);
internal DW_ExprValue dw_expr_neg(DW_ExprValue value);
internal DW_ExprValue dw_expr_not(DW_ExprValue value);

// stack
internal DW_ExprValueNode * dw_expr_stack_push(Arena *arena, DW_ExprStack *stack, DW_ExprValue value);
internal DW_ExprValueNode * dw_expr_stack_push_unsigned(Arena *arena, DW_ExprStack *stack, void *value, U64 value_size);
internal DW_ExprValue       dw_expr_stack_pop(DW_ExprStack *stack);
internal DW_ExprValue       dw_expr_stack_peek(DW_ExprStack *stack);
internal DW_ExprValueNode * dw_expr_stack_pick(DW_ExprStack *stack, U64 idx);

// value helpers
internal String8 dw_string_from_expr_value(Arena *arena, U64 addr_size, DW_ExprValue v);

// evaluator
internal DW_UnwindStatus dw_eval_expr(Arena *arena, DW_ExprContext *ctx, DW_Expr expr, DW_RegRead *reg_read, void *reg_read_ud, DW_ExprValue *value_out);

#endif //DWARF_EXPR_H

