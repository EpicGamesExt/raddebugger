// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_IR_H
#define EVAL_IR_H

////////////////////////////////
//~ rjf: Identifier Resolution Rule Types

typedef enum E_IdentifierResolutionPath
{
  E_IdentifierResolutionPath_WildcardInst,
  E_IdentifierResolutionPath_ParentExpr,
  E_IdentifierResolutionPath_ParentExprMember,
  E_IdentifierResolutionPath_ImplicitThisMember,
  E_IdentifierResolutionPath_Local,
  E_IdentifierResolutionPath_DebugInfoMatch,
  E_IdentifierResolutionPath_BuiltInConstants,
  E_IdentifierResolutionPath_BuiltInTypes,
  E_IdentifierResolutionPath_Registers,
  E_IdentifierResolutionPath_RegisterAliases,
  E_IdentifierResolutionPath_Macros,
}
E_IdentifierResolutionPath;

typedef struct E_IdentifierResolutionRule E_IdentifierResolutionRule;
struct E_IdentifierResolutionRule
{
  E_IdentifierResolutionPath *paths;
  U64 count;
};

////////////////////////////////
//~ rjf: IR State

typedef struct E_IRCacheNode E_IRCacheNode;
struct E_IRCacheNode
{
  E_IRCacheNode *next;
  E_Expr *expr;
  E_IRNode *overridden_node;
  E_IRTreeAndType irtree;
};

typedef struct E_IRCacheSlot E_IRCacheSlot;
struct E_IRCacheSlot
{
  E_IRCacheNode *first;
  E_IRCacheNode *last;
};

typedef struct E_IRState E_IRState;
struct E_IRState
{
  Arena *arena;
  U64 arena_eval_start_pos;
  
  // rjf: ir context
  E_IRCtx *ctx;
  
  // rjf: unpacked ctx
  RDI_Procedure *thread_ip_procedure;
  
  // rjf: overridden irtree
  E_IRTreeAndType *overridden_irtree;
  B32 disallow_autohooks;
  B32 disallow_chained_fastpaths;
  
  // rjf: caches
  E_UsedExprMap *used_expr_map;
  E_TypeAutoHookCacheMap *type_auto_hook_cache_map;
  U64 string_id_gen;
  E_StringIDMap *string_id_map;
  U64 ir_cache_slots_count;
  E_IRCacheSlot *ir_cache_slots;
};

////////////////////////////////
//~ rjf: Globals

E_IdentifierResolutionPath e_default_identifier_resolution_paths[] =
{
  E_IdentifierResolutionPath_WildcardInst,
  E_IdentifierResolutionPath_ParentExpr,
  E_IdentifierResolutionPath_ParentExprMember,
  E_IdentifierResolutionPath_ImplicitThisMember,
  E_IdentifierResolutionPath_Local,
  E_IdentifierResolutionPath_BuiltInConstants,
  E_IdentifierResolutionPath_BuiltInTypes,
  E_IdentifierResolutionPath_DebugInfoMatch,
  E_IdentifierResolutionPath_Registers,
  E_IdentifierResolutionPath_RegisterAliases,
  E_IdentifierResolutionPath_Macros,
};
E_IdentifierResolutionRule e_default_identifier_resolution_rule =
{
  e_default_identifier_resolution_paths,
  ArrayCount(e_default_identifier_resolution_paths),
};

E_IdentifierResolutionPath e_callable_identifier_resolution_paths[] =
{
  E_IdentifierResolutionPath_Macros,
  E_IdentifierResolutionPath_WildcardInst,
  E_IdentifierResolutionPath_ParentExpr,
  E_IdentifierResolutionPath_ParentExprMember,
  E_IdentifierResolutionPath_ImplicitThisMember,
  E_IdentifierResolutionPath_Local,
  E_IdentifierResolutionPath_BuiltInConstants,
  E_IdentifierResolutionPath_BuiltInTypes,
  E_IdentifierResolutionPath_DebugInfoMatch,
  E_IdentifierResolutionPath_Registers,
  E_IdentifierResolutionPath_RegisterAliases,
};
E_IdentifierResolutionRule e_callable_identifier_resolution_rule =
{
  e_callable_identifier_resolution_paths,
  ArrayCount(e_callable_identifier_resolution_paths),
};

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
internal E_IRNode *e_irtree_binary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, U64 operand_size, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_binary_op_u(Arena *arena, RDI_EvalOp op, U64 operand_size, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_conditional(Arena *arena, E_IRNode *c, E_IRNode *l, E_IRNode *r);
internal E_IRNode *e_irtree_bytecode_no_copy(Arena *arena, String8 bytecode);
internal E_IRNode *e_irtree_string_literal(Arena *arena, String8 string);
internal E_IRNode *e_irtree_set_space(Arena *arena, E_Space space, E_IRNode *c);
internal E_IRNode *e_irtree_mem_read_type(Arena *arena, E_IRNode *c, E_TypeKey type_key);
internal E_IRNode *e_irtree_convert_lo(Arena *arena, E_IRNode *c, RDI_EvalTypeGroup out, RDI_EvalTypeGroup in);
internal E_IRNode *e_irtree_trunc(Arena *arena, E_IRNode *c, E_TypeKey type_key);
internal E_IRNode *e_irtree_convert_hi(Arena *arena, E_IRNode *c, E_TypeKey out, E_TypeKey in);
internal E_IRNode *e_irtree_resolve_to_value(Arena *arena, E_Mode from_mode, E_IRNode *tree, E_TypeKey type_key);

//- rjf: expression poison checking
internal B32 e_expr_is_poisoned(E_Expr *expr);
internal void e_expr_poison(E_Expr *expr);
internal void e_expr_unpoison(E_Expr *expr);

//- rjf: top-level irtree/type extraction
E_TYPE_ACCESS_FUNCTION_DEF(default);
internal E_IRTreeAndType e_push_irtree_and_type_from_expr(Arena *arena, E_IRTreeAndType *root_parent, E_IdentifierResolutionRule *identifier_resolution_rule, B32 disallow_autohooks, B32 disallow_chained_fastpaths, E_Expr *root_expr);

//- rjf: irtree -> linear ops/bytecode
internal void e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_Space *current_space, E_OpList *out);
internal E_OpList e_oplist_from_irtree(Arena *arena, E_IRNode *root);
internal String8 e_bytecode_from_oplist(Arena *arena, E_OpList *oplist);

//- rjf: leaf-bytecode expression extensions
internal E_Expr *e_expr_irext_member_access(Arena *arena, E_Expr *lhs, E_IRTreeAndType *lhs_irtree, String8 member_name);

#endif // EVAL_IR_H
