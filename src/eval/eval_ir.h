// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_IR_H
#define EVAL_IR_H

////////////////////////////////
//~ rjf: Bytecode Operation Types

enum
{
  E_IRExtKind_Bytecode = RDI_EvalOp_COUNT,
  E_IRExtKind_SetSpace,
  E_IRExtKind_COUNT
};

typedef struct E_Op E_Op;
struct E_Op
{
  E_Op *next;
  RDI_EvalOp opcode;
  E_Value value;
  String8 string;
};

typedef struct E_OpList E_OpList;
struct E_OpList
{
  E_Op *first;
  E_Op *last;
  U64 op_count;
  U64 encoded_size;
};

////////////////////////////////
//~ rjf: IR Tree Types

typedef struct E_IRNode E_IRNode;
struct E_IRNode
{
  E_IRNode *first;
  E_IRNode *last;
  E_IRNode *next;
  RDI_EvalOp op;
  String8 string;
  E_Value value;
};

typedef struct E_IRTreeAndType E_IRTreeAndType;
struct E_IRTreeAndType
{
  E_IRNode *root;
  E_TypeKey type_key;
  E_Mode mode;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Member/Index Lookup Hooks

typedef struct E_LookupInfo E_LookupInfo;
struct E_LookupInfo
{
  void *user_data;
  U64 named_expr_count;
  U64 idxed_expr_count;
};

typedef struct E_Lookup E_Lookup;
struct E_Lookup
{
  E_IRTreeAndType irtree_and_type;
};

#define E_LOOKUP_INFO_FUNCTION_SIG(name) E_LookupInfo name(Arena *arena, E_Expr *lhs)
#define E_LOOKUP_INFO_FUNCTION_NAME(name) e_lookup_info_##name
#define E_LOOKUP_INFO_FUNCTION_DEF(name) internal E_LOOKUP_INFO_FUNCTION_SIG(E_LOOKUP_INFO_FUNCTION_NAME(name))
typedef E_LOOKUP_INFO_FUNCTION_SIG(E_LookupInfoFunctionType);

#define E_LOOKUP_FUNCTION_SIG(name) E_Lookup name(Arena *arena, E_ExprKind kind, E_Expr *lhs, E_Expr *rhs, void *user_data)
#define E_LOOKUP_FUNCTION_NAME(name) e_lookup_##name
#define E_LOOKUP_FUNCTION_DEF(name) internal E_LOOKUP_FUNCTION_SIG(E_LOOKUP_FUNCTION_NAME(name))
typedef E_LOOKUP_FUNCTION_SIG(E_LookupFunctionType);

E_LOOKUP_INFO_FUNCTION_DEF(default);
E_LOOKUP_FUNCTION_DEF(default);

typedef struct E_LookupRule E_LookupRule;
struct E_LookupRule
{
  String8 name;
  E_LookupInfoFunctionType *lookup_info;
  E_LookupFunctionType *lookup;
};

typedef struct E_LookupRuleNode E_LookupRuleNode;
struct E_LookupRuleNode
{
  E_LookupRuleNode *next;
  E_LookupRule v;
};

typedef struct E_LookupRuleSlot E_LookupRuleSlot;
struct E_LookupRuleSlot
{
  E_LookupRuleNode *first;
  E_LookupRuleNode *last;
};

typedef struct E_LookupRuleMap E_LookupRuleMap;
struct E_LookupRuleMap
{
  E_LookupRuleSlot *slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Parse Context

typedef struct E_IRCtx E_IRCtx;
struct E_IRCtx
{
  E_String2ExprMap *macro_map;
  E_LookupRuleMap *lookup_rule_map;
};

////////////////////////////////
//~ rjf: Globals

local_persist read_only E_LookupRule e_lookup_rule__default =
{
  str8_lit_comp("default"),
  E_LOOKUP_INFO_FUNCTION_NAME(default),
  E_LOOKUP_FUNCTION_NAME(default),
};
global read_only E_IRNode e_irnode_nil = {&e_irnode_nil, &e_irnode_nil, &e_irnode_nil};
thread_static E_IRCtx *e_ir_ctx = 0;

////////////////////////////////
//~ rjf: Expr Kind Enum Functions

internal RDI_EvalOp e_opcode_from_expr_kind(E_ExprKind kind);
internal B32        e_expr_kind_is_comparison(E_ExprKind kind);

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_IRCtx *e_selected_ir_ctx(void);
internal void e_select_ir_ctx(E_IRCtx *ctx);

////////////////////////////////
//~ rjf: Lookups

internal E_LookupRuleMap e_lookup_rule_map_make(Arena *arena, U64 slots_count);
internal void e_lookup_rule_map_insert(Arena *arena, E_LookupRuleMap *map, E_LookupRule *rule);
#define e_lookup_rule_map_insert_new(arena, map, name_, lookup_info_, lookup_) e_lookup_rule_map_insert((arena), (map), &(E_LookupRule){.name = (name_), .lookup_info = (lookup_info_), .lookup = (lookup_)})

internal E_LookupRule *e_lookup_rule_from_string(String8 string);
E_LOOKUP_INFO_FUNCTION_DEF(default);
E_LOOKUP_FUNCTION_DEF(default);

////////////////////////////////
//~ rjf: IR-ization Functions

//- rjf: op list functions
internal void e_oplist_push_op(Arena *arena, E_OpList *list, RDI_EvalOp opcode, E_Value value);
internal void e_oplist_push_uconst(Arena *arena, E_OpList *list, U64 x);
internal void e_oplist_push_sconst(Arena *arena, E_OpList *list, S64 x);
internal void e_oplist_push_bytecode(Arena *arena, E_OpList *list, String8 bytecode);
internal void e_oplist_push_set_space(Arena *arena, E_OpList *list, E_Space space);
internal void e_oplist_push_string_literal(Arena *arena, E_OpList *list, String8 string);
internal void e_oplist_concat_in_place(E_OpList *dst, E_OpList *to_push);

//- rjf: ir tree core building helpers
internal E_IRNode *e_push_irnode(Arena *arena, RDI_EvalOp op);
internal void e_irnode_push_child(E_IRNode *parent, E_IRNode *child);

//- rjf: ir subtree building helpers
internal E_IRNode *e_irtree_const_u(Arena *arena, U64 v);
internal E_IRNode *e_irtree_unary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, E_IRNode *c);
internal E_IRNode *e_irtree_binary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_binary_op_u(Arena *arena, RDI_EvalOp op, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_conditional(Arena *arena, E_IRNode *c, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_bytecode_no_copy(Arena *arena, String8 bytecode);
internal E_IRNode *e_irtree_string_literal(Arena *arena, String8 string);
internal E_IRNode *e_irtree_set_space(Arena *arena, E_Space space, E_IRNode *c);
internal E_IRNode *e_irtree_mem_read_type(Arena *arena, E_IRNode *c, E_TypeKey type_key);
internal E_IRNode *e_irtree_convert_lo(Arena *arena, E_IRNode *c, RDI_EvalTypeGroup out, RDI_EvalTypeGroup in);
internal E_IRNode *e_irtree_trunc(Arena *arena, E_IRNode *c, E_TypeKey type_key);
internal E_IRNode *e_irtree_convert_hi(Arena *arena, E_IRNode *c, E_TypeKey out, E_TypeKey in);
internal E_IRNode *e_irtree_resolve_to_value(Arena *arena, E_Mode from_mode, E_IRNode *tree, E_TypeKey type_key);

//- rjf: top-level irtree/type extraction
internal E_IRTreeAndType e_irtree_and_type_from_expr__space(Arena *arena, E_Space *current_space, E_Expr *expr);
internal E_IRTreeAndType e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr);

//- rjf: irtree -> linear ops/bytecode
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_OpList *out);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal String8 e_bytecode_from_oplist(Arena *arena, E_OpList *oplist);

#endif // EVAL_IR_H
