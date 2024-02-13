// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_COMPILER_H
#define EVAL_COMPILER_H

////////////////////////////////
//~ allen: EVAL Bytecode Helpers

internal String8 eval_bytecode_from_oplist(Arena *arena, EVAL_OpList *list);

internal void eval_oplist_push_op(Arena *arena, EVAL_OpList *list, RDI_EvalOp op, U64 p);
internal void eval_oplist_push_uconst(Arena *arena, EVAL_OpList *list, U64 x);
internal void eval_oplist_push_sconst(Arena *arena, EVAL_OpList *list, S64 x);

internal void eval_oplist_push_bytecode(Arena *arena, EVAL_OpList *list, String8 bytecode);

internal void eval_oplist_concat_in_place(EVAL_OpList *left_dst, EVAL_OpList *right_destroyed);

////////////////////////////////
//~ allen: EVAL Expression Info Functions

internal RDI_EvalOp eval_opcode_from_expr_kind(EVAL_ExprKind kind);
internal B32            eval_expr_kind_is_comparison(EVAL_ExprKind kind);

////////////////////////////////
//~ allen: EVAL Expression Constructors

internal EVAL_Expr* eval_expr(Arena *arena, EVAL_ExprKind kind, void *location, EVAL_Expr *c0, EVAL_Expr *c1, EVAL_Expr *c2);
internal EVAL_Expr* eval_expr_u64(Arena *arena, void *location, U64 u64);
internal EVAL_Expr* eval_expr_f64(Arena *arena, void *location, F64 f64);
internal EVAL_Expr* eval_expr_f32(Arena *arena, void *location, F32 f32);
internal EVAL_Expr* eval_expr_child_and_u64(Arena *arena, EVAL_ExprKind kind, void *location, EVAL_Expr *child, U64 u64);
internal EVAL_Expr* eval_expr_leaf_member(Arena *arena, void *location, String8 name);
internal EVAL_Expr* eval_expr_leaf_ident(Arena *arena, void *location, String8 name);
internal EVAL_Expr* eval_expr_leaf_bytecode(Arena *arena, void *location, TG_Key type_key, String8 bytecode, EVAL_EvalMode mode);
internal EVAL_Expr* eval_expr_leaf_op_list(Arena *arena, void *location, TG_Key type_key, EVAL_OpList *ops, EVAL_EvalMode mode);
internal EVAL_Expr* eval_expr_leaf_type(Arena *arena, void *location, TG_Key type_key);

////////////////////////////////
//~ allen: EVAL Type Information Transformers

internal RDI_EvalTypeGroup eval_type_group_from_kind(TG_Kind kind);

internal TG_Key eval_type_unwrap_enum(TG_Graph *graph, RDI_Parsed *rdbg, TG_Key key);
internal TG_Key eval_type_promote(TG_Graph *graph, RDI_Parsed *rdbg, TG_Key key);
internal TG_Key eval_type_coerce(TG_Graph *graph, RDI_Parsed *rdbg, TG_Key l, TG_Key r);

internal B32 eval_type_match(TG_Graph *graph, RDI_Parsed *rdbg, TG_Key l, TG_Key r);

internal B32 eval_kind_is_integer(TG_Kind kind);
internal B32 eval_kind_is_signed(TG_Kind kind);
internal B32 eval_kind_is_basic_or_enum(TG_Kind kind);

////////////////////////////////
//~ allen: EVAL IR-Tree Constructors

internal EVAL_IRTree* eval_irtree_const_u(Arena *arena, U64 v);
internal EVAL_IRTree* eval_irtree_unary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, EVAL_IRTree *c);
internal EVAL_IRTree* eval_irtree_binary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, EVAL_IRTree *l, EVAL_IRTree *r);
internal EVAL_IRTree* eval_irtree_binary_op_u(Arena *arena, RDI_EvalOp op, EVAL_IRTree *l, EVAL_IRTree *r);
internal EVAL_IRTree* eval_irtree_conditional(Arena *arena, EVAL_IRTree *c, EVAL_IRTree *l, EVAL_IRTree *r);
internal EVAL_IRTree* eval_irtree_bytecode_no_copy(Arena *arena, String8 bytecode);

////////////////////////////////
//~ allen: EVAL IR-Tree High Level Helpers

internal EVAL_IRTree* eval_irtree_mem_read_type(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_IRTree *c, TG_Key type_key);
internal EVAL_IRTree* eval_irtree_convert_lo(Arena *arena, EVAL_IRTree *c, RDI_EvalTypeGroup out, RDI_EvalTypeGroup in);
internal EVAL_IRTree* eval_irtree_trunc(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_IRTree *c, TG_Key type_key);
internal EVAL_IRTree* eval_irtree_convert_hi(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_IRTree *c, TG_Key out, TG_Key in);
internal EVAL_IRTree* eval_irtree_resolve_to_value(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_EvalMode from_mode, EVAL_IRTree *tree, TG_Key type_key);

////////////////////////////////
//~ allen: EVAL Compiler Phases

internal void eval_push_leaf_ident_exprs_from_expr__in_place(Arena *arena, EVAL_String2ExprMap *map, EVAL_Expr *expr, EVAL_ErrorList *eout);
internal TG_Key eval_type_from_type_expr(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_Expr *expr, EVAL_ErrorList *eout);
internal EVAL_IRTreeAndType eval_irtree_and_type_from_expr(Arena *arena, TG_Graph *graph, RDI_Parsed *rdbg, EVAL_String2ExprMap *leaf_ident_expr_map, EVAL_Expr *expr, EVAL_ErrorList *eout);
internal void eval_oplist_from_irtree(Arena *arena, EVAL_IRTree *tree, EVAL_OpList *out);

#endif //EVAL_COMPILER_H
