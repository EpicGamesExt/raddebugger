// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_PARSE_H
#define EVAL_PARSE_H

////////////////////////////////
//~ rjf: Parse Results

typedef struct E_Parse E_Parse;
struct E_Parse
{
  E_Token *last_token;
  E_ExprChain exprs;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Parse Context

typedef struct E_ParseCtx E_ParseCtx;
struct E_ParseCtx
{
  // rjf: instruction pointer info
  U64 ip_vaddr;
  U64 ip_voff;
  E_Space ip_thread_space;
  
  // rjf: modules
  E_Module *modules;
  U64 modules_count;
  E_Module *primary_module;
  
  // rjf: local identifier resolution maps
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map; // (within `rdis[rdis_primary_idx]`)
  E_String2NumMap *member_map; // (within `rdis[rdis_primary_idx]`)
};

////////////////////////////////
//~ rjf: Parse State (stateful thread-local caching mechanisms, not provided by user)

typedef struct E_ParseState E_ParseState;
struct E_ParseState
{
  Arena *arena;
  U64 arena_eval_start_pos;
  E_ParseCtx *ctx;
};

////////////////////////////////
//~ rjf: Globals

global read_only E_String2NumMap e_string2num_map_nil = {0};
global read_only E_String2ExprMap e_string2expr_map_nil = {0};
global read_only E_Expr e_expr_nil = {&e_expr_nil, &e_expr_nil, &e_expr_nil, &e_expr_nil, &e_expr_nil};
thread_static E_ParseState *e_parse_state = 0;

////////////////////////////////
//~ rjf: Tokenization Functions

#define e_token_at_it(it, arr) (((it) < (arr)->v+(arr)->count) ? (*(it)) : e_token_zero())
internal E_Token e_token_zero(void);
internal void e_token_chunk_list_push(Arena *arena, E_TokenChunkList *list, U64 chunk_size, E_Token *token);
internal E_TokenArray e_token_array_from_chunk_list(Arena *arena, E_TokenChunkList *list);
internal E_TokenArray e_token_array_from_text(Arena *arena, String8 text);
internal E_TokenArray e_token_array_make_first_opl(E_Token *first, E_Token *opl);

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_ParseCtx *e_selected_parse_ctx(void);
internal void e_select_parse_ctx(E_ParseCtx *ctx);
internal U32 e_parse_ctx_module_idx_from_rdi(RDI_Parsed *rdi);

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *e_push_expr(Arena *arena, E_ExprKind kind, void *location);
internal void e_expr_insert_child(E_Expr *parent, E_Expr *prev, E_Expr *child);
internal void e_expr_push_child(E_Expr *parent, E_Expr *child);
internal void e_expr_remove_child(E_Expr *parent, E_Expr *child);
internal void e_expr_push_tag(E_Expr *parent, E_Expr *child);
internal E_Expr *e_expr_ref(Arena *arena, E_Expr *ref);
internal E_Expr *e_expr_ref_deref(Arena *arena, E_Expr *rhs);
internal E_Expr *e_expr_ref_cast(Arena *arena, E_TypeKey type_key, E_Expr *rhs);
internal E_Expr *e_expr_copy(Arena *arena, E_Expr *src);
internal void e_expr_list_push(Arena *arena, E_ExprList *list, E_Expr *expr);

////////////////////////////////
//~ rjf: Expression Tree -> String Conversions

internal void e_append_strings_from_expr(Arena *arena, E_Expr *expr, String8List *out);
internal String8 e_string_from_expr(Arena *arena, E_Expr *expr);

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey e_leaf_type_from_name(String8 name);
internal E_TypeKey e_type_from_expr(E_Expr *expr);
internal void e_push_leaf_ident_exprs_from_expr__in_place(Arena *arena, E_String2ExprMap *map, E_Expr *expr);
internal E_Parse e_parse_type_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);
internal E_Parse e_parse_expr_from_text_tokens__prec(Arena *arena, String8 text, E_TokenArray *tokens, S64 max_precedence, U64 max_chain_count);
internal E_Parse e_parse_expr_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);
internal E_Parse e_parse_expr_from_text(Arena *arena, String8 text);

#endif // EVAL_PARSE_H
