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
//~ rjf: Evaluated String ID Map

typedef struct E_StringIDNode E_StringIDNode;
struct E_StringIDNode
{
  E_StringIDNode *hash_next;
  E_StringIDNode *id_next;
  U64 id;
  String8 string;
};

typedef struct E_StringIDSlot E_StringIDSlot;
struct E_StringIDSlot
{
  E_StringIDNode *first;
  E_StringIDNode *last;
};

typedef struct E_StringIDMap E_StringIDMap;
struct E_StringIDMap
{
  U64 id_slots_count;
  E_StringIDSlot *id_slots;
  U64 hash_slots_count;
  E_StringIDSlot *hash_slots;
};

////////////////////////////////
//~ rjf: IR Context

typedef struct E_IRCtx E_IRCtx;
struct E_IRCtx
{
  // rjf: instruction pointer info
  U64 ip_vaddr;
  U64 ip_voff;
  E_Space ip_thread_space;
  
  // rjf: modules
  E_Module *modules;
  U64 modules_count;
  E_Module *primary_module;
  
  // rjf: identifier-resolution maps
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map; // (within `primary_module`)
  E_String2NumMap *member_map; // (within `primary_module`)
  E_String2ExprMap *macro_map;
  
  // rjf: hook maps
  E_LookupRuleMap *lookup_rule_map;
  E_IRGenRuleMap *irgen_rule_map;
  E_AutoHookMap *auto_hook_map;
};

////////////////////////////////
//~ rjf: IR State

typedef struct E_IRState E_IRState;
struct E_IRState
{
  Arena *arena;
  U64 arena_eval_start_pos;
  
  // rjf: ir context
  E_IRCtx *ctx;
  
  // rjf: unpacked ctx
  RDI_Procedure *ip_procedure;
  
  // rjf: caches
  E_UsedTagMap *used_tag_map;
  E_TypeAutoHookCacheMap *type_auto_hook_cache_map;
  U64 string_id_gen;
  E_StringIDMap *string_id_map;
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
thread_static E_IRState *e_ir_state = 0;

////////////////////////////////
//~ rjf: Expr Kind Enum Functions

internal RDI_EvalOp e_opcode_from_expr_kind(E_ExprKind kind);
internal B32        e_expr_kind_is_comparison(E_ExprKind kind);

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

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
internal void e_auto_hook_map_insert_new_(Arena *arena, E_AutoHookMap *map, E_AutoHookParams *params);
#define e_auto_hook_map_insert_new(arena, map, ...) e_auto_hook_map_insert_new_((arena), (map), &(E_AutoHookParams){.type_key = zero_struct, __VA_ARGS__})
internal E_ExprList e_auto_hook_tag_exprs_from_type_key(Arena *arena, E_TypeKey type_key);
internal E_ExprList e_auto_hook_tag_exprs_from_type_key__cached(E_TypeKey type_key);

////////////////////////////////
//~ rjf: Evaluated String IDs

internal U64 e_id_from_string(String8 string);
internal String8 e_string_from_id(U64 id);

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
internal E_IRNode *e_irtree_leaf_u128(Arena *arena, U128 u128);
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

//- rjf: rule tag poison checking
internal B32 e_tag_is_poisoned(E_Expr *tag);
internal void e_tag_poison(E_Expr *tag);
internal void e_tag_unpoison(E_Expr *tag);

//- rjf: top-level irtree/type extraction
internal E_IRTreeAndType e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr);

//- rjf: irtree -> linear ops/bytecode
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_Space *current_space, E_OpList *out);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal String8 e_bytecode_from_oplist(Arena *arena, E_OpList *oplist);

//- rjf: leaf-bytecode expression extensions
internal E_Expr *e_expr_irext_member_access(Arena *arena, E_Expr *lhs, E_IRTreeAndType *lhs_irtree, String8 member_name);
internal E_Expr *e_expr_irext_array_index(Arena *arena, E_Expr *lhs, E_IRTreeAndType *lhs_irtree, U64 index);
internal E_Expr *e_expr_irext_deref(Arena *arena, E_Expr *rhs, E_IRTreeAndType *rhs_irtree);
internal E_Expr *e_expr_irext_cast(Arena *arena, E_Expr *rhs, E_IRTreeAndType *rhs_irtree, E_TypeKey type_key);

////////////////////////////////
//~ rjf: Expression & IR-Tree => Lookup Rule

internal E_LookupRuleTagPair e_lookup_rule_tag_pair_from_expr_irtree(E_Expr *expr, E_IRTreeAndType *irtree);

#endif // EVAL_IR_H
