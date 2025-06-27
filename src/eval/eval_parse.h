// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_PARSE_H
#define EVAL_PARSE_H

////////////////////////////////
//~ rjf: Tokenization Functions

#define e_token_at_it(it, arr) (((arr)->v <= (it) && (it) < (arr)->v+(arr)->count) ? (*(it)) : e_token_zero())
internal E_Token e_token_zero(void);
internal void e_token_chunk_list_push(Arena *arena, E_TokenChunkList *list, U64 chunk_size, E_Token *token);
internal E_TokenArray e_token_array_from_chunk_list(Arena *arena, E_TokenChunkList *list);
internal E_TokenArray e_token_array_from_text(Arena *arena, String8 text);
internal E_TokenArray e_token_array_make_first_opl(E_Token *first, E_Token *opl);

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *e_push_expr(Arena *arena, E_ExprKind kind, Rng1U64 range);
internal void e_expr_insert_child(E_Expr *parent, E_Expr *prev, E_Expr *child);
internal void e_expr_push_child(E_Expr *parent, E_Expr *child);
internal void e_expr_remove_child(E_Expr *parent, E_Expr *child);
internal E_Expr *e_expr_ref(Arena *arena, E_Expr *ref);
internal E_Expr *e_expr_copy(Arena *arena, E_Expr *src);
internal void e_expr_list_push(Arena *arena, E_ExprList *list, E_Expr *expr);

////////////////////////////////
//~ rjf: Expression Tree -> String Conversions

internal void e_append_strings_from_expr(Arena *arena, E_Expr *expr, String8 parent_expr_string, String8List *out);
internal String8 e_string_from_expr(Arena *arena, E_Expr *expr, String8 parent_expr_string);

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey e_leaf_builtin_type_key_from_name(String8 name);
internal E_TypeKey e_leaf_type_key_from_name(String8 name);
internal E_TypeKey e_type_key_from_expr(E_Expr *expr);
internal E_Parse e_push_type_parse_from_text_tokens(Arena *arena, String8 text, E_TokenArray tokens);
internal E_Parse e_push_parse_from_string_tokens__prec(Arena *arena, String8 text, E_TokenArray tokens, S64 max_precedence, U64 max_chain_count);
internal E_Parse e_push_parse_from_string(Arena *arena, String8 text);

#endif // EVAL_PARSE_H
