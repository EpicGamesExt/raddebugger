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
  E_Space space;
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

typedef struct E_LookupAccess E_LookupAccess;
struct E_LookupAccess
{
  E_IRTreeAndType irtree_and_type;
};

#define E_LOOKUP_INFO_FUNCTION_SIG(name) E_LookupInfo name(Arena *arena, E_IRTreeAndType *lhs, String8 filter)
#define E_LOOKUP_INFO_FUNCTION_NAME(name) e_lookup_info_##name
#define E_LOOKUP_INFO_FUNCTION_DEF(name) internal E_LOOKUP_INFO_FUNCTION_SIG(E_LOOKUP_INFO_FUNCTION_NAME(name))
typedef E_LOOKUP_INFO_FUNCTION_SIG(E_LookupInfoFunctionType);
E_LOOKUP_INFO_FUNCTION_DEF(default);

#define E_LOOKUP_ACCESS_FUNCTION_SIG(name) E_LookupAccess name(Arena *arena, E_ExprKind kind, E_Expr *lhs, E_Expr *rhs, void *user_data)
#define E_LOOKUP_ACCESS_FUNCTION_NAME(name) e_lookup_access_##name
#define E_LOOKUP_ACCESS_FUNCTION_DEF(name) internal E_LOOKUP_ACCESS_FUNCTION_SIG(E_LOOKUP_ACCESS_FUNCTION_NAME(name))
typedef E_LOOKUP_ACCESS_FUNCTION_SIG(E_LookupAccessFunctionType);
E_LOOKUP_ACCESS_FUNCTION_DEF(default);

#define E_LOOKUP_RANGE_FUNCTION_SIG(name) void name(Arena *arena, E_Expr *lhs, Rng1U64 idx_range, E_Expr **exprs, String8 *exprs_strings, void *user_data)
#define E_LOOKUP_RANGE_FUNCTION_NAME(name) e_lookup_range_##name
#define E_LOOKUP_RANGE_FUNCTION_DEF(name) internal E_LOOKUP_RANGE_FUNCTION_SIG(E_LOOKUP_RANGE_FUNCTION_NAME(name))
typedef E_LOOKUP_RANGE_FUNCTION_SIG(E_LookupRangeFunctionType);
E_LOOKUP_RANGE_FUNCTION_DEF(default);

#define E_LOOKUP_ID_FROM_NUM_FUNCTION_SIG(name) U64 name(U64 num, void *user_data)
#define E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(name) e_lookup_id_from_num_##name
#define E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(name) internal E_LOOKUP_ID_FROM_NUM_FUNCTION_SIG(E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(name))
typedef E_LOOKUP_ID_FROM_NUM_FUNCTION_SIG(E_LookupIDFromNumFunctionType);
E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(default);

#define E_LOOKUP_NUM_FROM_ID_FUNCTION_SIG(name) U64 name(U64 id, void *user_data)
#define E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(name) e_lookup_num_from_id_##name
#define E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(name) internal E_LOOKUP_NUM_FROM_ID_FUNCTION_SIG(E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(name))
typedef E_LOOKUP_NUM_FROM_ID_FUNCTION_SIG(E_LookupNumFromIDFunctionType);
E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(default);

typedef struct E_LookupRule E_LookupRule;
struct E_LookupRule
{
  String8 name;
  E_LookupInfoFunctionType *info;
  E_LookupAccessFunctionType *access;
  E_LookupRangeFunctionType *range;
  E_LookupIDFromNumFunctionType *id_from_num;
  E_LookupNumFromIDFunctionType *num_from_id;
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
//~ rjf: IR Generation Hooks

#define E_IRGEN_FUNCTION_SIG(name) E_IRTreeAndType name(Arena *arena, E_Expr *expr, E_Expr *tag)
#define E_IRGEN_FUNCTION_NAME(name) e_irgen_##name
#define E_IRGEN_FUNCTION_DEF(name) internal E_IRGEN_FUNCTION_SIG(E_IRGEN_FUNCTION_NAME(name))
typedef E_IRGEN_FUNCTION_SIG(E_IRGenFunctionType);
E_IRGEN_FUNCTION_DEF(default);

typedef struct E_IRGenRule E_IRGenRule;
struct E_IRGenRule
{
  String8 name;
  E_IRGenFunctionType *irgen;
};

typedef struct E_IRGenRuleNode E_IRGenRuleNode;
struct E_IRGenRuleNode
{
  E_IRGenRuleNode *next;
  E_IRGenRule v;
};

typedef struct E_IRGenRuleSlot E_IRGenRuleSlot;
struct E_IRGenRuleSlot
{
  E_IRGenRuleNode *first;
  E_IRGenRuleNode *last;
};

typedef struct E_IRGenRuleMap E_IRGenRuleMap;
struct E_IRGenRuleMap
{
  U64 slots_count;
  E_IRGenRuleSlot *slots;
};

////////////////////////////////
//~ rjf: Type Pattern -> Hook Key Data Structure (Auto View Rules)

typedef struct E_AutoHookNode E_AutoHookNode;
struct E_AutoHookNode
{
  E_AutoHookNode *hash_next;
  E_AutoHookNode *pattern_order_next;
  E_TypeKey type_key;
  String8List type_pattern_parts;
  E_Expr *tag_expr;
};

typedef struct E_AutoHookSlot E_AutoHookSlot;
struct E_AutoHookSlot
{
  E_AutoHookNode *first;
  E_AutoHookNode *last;
};

typedef struct E_AutoHookMap E_AutoHookMap;
struct E_AutoHookMap
{
  U64 slots_count;
  E_AutoHookSlot *slots;
  E_AutoHookNode *first_pattern;
  E_AutoHookNode *last_pattern;
};

////////////////////////////////
//~ rjf: Used Tag Map Data Structure

typedef struct E_UsedTagNode E_UsedTagNode;
struct E_UsedTagNode
{
  E_UsedTagNode *next;
  E_UsedTagNode *prev;
  E_Expr *tag;
};

typedef struct E_UsedTagSlot E_UsedTagSlot;
struct E_UsedTagSlot
{
  E_UsedTagNode *first;
  E_UsedTagNode *last;
};

typedef struct E_UsedTagMap E_UsedTagMap;
struct E_UsedTagMap
{
  U64 slots_count;
  E_UsedTagSlot *slots;
};

////////////////////////////////
//~ rjf: Type Key -> Auto Hook Expr List Cache

typedef struct E_TypeAutoHookCacheNode E_TypeAutoHookCacheNode;
struct E_TypeAutoHookCacheNode
{
  E_TypeAutoHookCacheNode *next;
  E_TypeKey key;
  E_ExprList exprs;
};

typedef struct E_TypeAutoHookCacheSlot E_TypeAutoHookCacheSlot;
struct E_TypeAutoHookCacheSlot
{
  E_TypeAutoHookCacheNode *first;
  E_TypeAutoHookCacheNode *last;
};

typedef struct E_TypeAutoHookCacheMap E_TypeAutoHookCacheMap;
struct E_TypeAutoHookCacheMap
{
  U64 slots_count;
  E_TypeAutoHookCacheSlot *slots;
};

////////////////////////////////
//~ rjf: Parse Context

typedef struct E_IRCtx E_IRCtx;
struct E_IRCtx
{
  E_String2ExprMap *macro_map;
  E_LookupRuleMap *lookup_rule_map;
  E_IRGenRuleMap *irgen_rule_map;
  E_AutoHookMap *auto_hook_map;
  E_UsedTagMap *used_tag_map;
  E_TypeAutoHookCacheMap *type_auto_hook_cache_map;
};

////////////////////////////////
//~ rjf: Globals

local_persist read_only E_LookupRule e_lookup_rule__nil =
{
  str8_lit_comp("nil"),
  E_LOOKUP_INFO_FUNCTION_NAME(default),
  E_LOOKUP_ACCESS_FUNCTION_NAME(default),
  E_LOOKUP_RANGE_FUNCTION_NAME(default),
  E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(default),
  E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(default),
};
local_persist read_only E_LookupRule e_lookup_rule__default =
{
  str8_lit_comp("default"),
  E_LOOKUP_INFO_FUNCTION_NAME(default),
  E_LOOKUP_ACCESS_FUNCTION_NAME(default),
  E_LOOKUP_RANGE_FUNCTION_NAME(default),
  E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(default),
  E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(default),
};
local_persist read_only E_IRGenRule e_irgen_rule__default =
{
  str8_lit_comp("default"),
  E_IRGEN_FUNCTION_NAME(default),
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
#define e_lookup_rule_map_insert_new(arena, map, name_, ...) e_lookup_rule_map_insert((arena), (map), &(E_LookupRule){.name = (name_), __VA_ARGS__})

internal E_LookupRule *e_lookup_rule_from_string(String8 string);

////////////////////////////////
//~ rjf: IR Gen Rules

internal E_IRGenRuleMap e_irgen_rule_map_make(Arena *arena, U64 slots_count);
internal void e_irgen_rule_map_insert(Arena *arena, E_IRGenRuleMap *map, E_IRGenRule *rule);
#define e_irgen_rule_map_insert_new(arena, map, name_, ...) e_irgen_rule_map_insert((arena), (map), &(E_IRGenRule){.name = (name_), __VA_ARGS__})

internal E_IRGenRule *e_irgen_rule_from_string(String8 string);

////////////////////////////////
//~ rjf: Auto Hooks

internal E_AutoHookMap e_auto_hook_map_make(Arena *arena, U64 slots_count);
internal void e_auto_hook_map_insert_new(Arena *arena, E_AutoHookMap *map, String8 pattern, String8 tag_expr_string);
internal E_ExprList e_auto_hook_tag_exprs_from_type_key(Arena *arena, E_TypeKey type_key);
internal E_ExprList e_auto_hook_tag_exprs_from_type_key__cached(E_TypeKey type_key);

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
internal E_IRTreeAndType e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr);

//- rjf: irtree -> linear ops/bytecode
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_Space *current_space, E_OpList *out);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal String8 e_bytecode_from_oplist(Arena *arena, E_OpList *oplist);

#endif // EVAL_IR_H
