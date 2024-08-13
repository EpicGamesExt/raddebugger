// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_PARSE_H
#define EVAL_PARSE_H

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval.meta.h"

////////////////////////////////
//~ rjf: Messages

typedef enum E_MsgKind
{
  E_MsgKind_Null,
  E_MsgKind_MalformedInput,
  E_MsgKind_MissingInfo,
  E_MsgKind_ResolutionFailure,
  E_MsgKind_InterpretationError,
  E_MsgKind_COUNT
}
E_MsgKind;

typedef struct E_Msg E_Msg;
struct E_Msg
{
  E_Msg *next;
  E_MsgKind kind;
  void *location;
  String8 text;
};

typedef struct E_MsgList E_MsgList;
struct E_MsgList
{
  E_Msg *first;
  E_Msg *last;
  E_MsgKind max_kind;
  U64 count;
};

////////////////////////////////
//~ rjf: Token Types

typedef struct E_Token E_Token;
struct E_Token
{
  E_TokenKind kind;
  Rng1U64 range;
};

typedef struct E_TokenChunkNode E_TokenChunkNode;
struct E_TokenChunkNode
{
  E_TokenChunkNode *next;
  E_Token *v;
  U64 count;
  U64 cap;
};

typedef struct E_TokenChunkList E_TokenChunkList;
struct E_TokenChunkList
{
  E_TokenChunkNode *first;
  E_TokenChunkNode *last;
  U64 node_count;
  U64 total_count;
};

typedef struct E_TokenArray E_TokenArray;
struct E_TokenArray
{
  E_Token *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Expression Tree Types

typedef enum E_Mode
{
  E_Mode_Null,
  E_Mode_Value,
  E_Mode_Addr,
  E_Mode_Reg,
}
E_Mode;

typedef struct E_Expr E_Expr;
struct E_Expr
{
  E_Expr *first;
  E_Expr *last;
  E_Expr *next;
  void *location;
  E_ExprKind kind;
  E_Mode mode;
  E_TypeKey type_key;
  U32 u32;
  F32 f32;
  U64 u64;
  F64 f64;
  String8 string;
};

////////////////////////////////
//~ rjf: Map Types

//- rjf: string -> num

typedef struct E_String2NumMapNode E_String2NumMapNode;
struct E_String2NumMapNode
{
  E_String2NumMapNode *order_next;
  E_String2NumMapNode *hash_next;
  String8 string;
  U64 num;
};

typedef struct E_String2NumMapNodeArray E_String2NumMapNodeArray;
struct E_String2NumMapNodeArray
{
  E_String2NumMapNode **v;
  U64 count;
};

typedef struct E_String2NumMapSlot E_String2NumMapSlot;
struct E_String2NumMapSlot
{
  E_String2NumMapNode *first;
  E_String2NumMapNode *last;
};

typedef struct E_String2NumMap E_String2NumMap;
struct E_String2NumMap
{
  U64 slots_count;
  U64 node_count;
  E_String2NumMapSlot *slots;
  E_String2NumMapNode *first;
  E_String2NumMapNode *last;
};

//- rjf: string -> expr

typedef struct E_String2ExprMapNode E_String2ExprMapNode;
struct E_String2ExprMapNode
{
  E_String2ExprMapNode *hash_next;
  String8 string;
  E_Expr *expr;
  U64 poison_count;
};

typedef struct E_String2ExprMapSlot E_String2ExprMapSlot;
struct E_String2ExprMapSlot
{
  E_String2ExprMapNode *first;
  E_String2ExprMapNode *last;
};

typedef struct E_String2ExprMap E_String2ExprMap;
struct E_String2ExprMap
{
  U64 slots_count;
  E_String2ExprMapSlot *slots;
};

////////////////////////////////
//~ rjf: Parse Context

typedef struct E_ParseCtx E_ParseCtx;
struct E_ParseCtx
{
  // rjf: architecture
  Architecture arch;
  
  // rjf: instruction pointer info
  U64 ip_vaddr;
  U64 ip_voff; // (within module, which uses `rdis[rdis_primary_idx]` for debug info)
  
  // rjf: debug info
  RDI_Parsed **rdis;
  Rng1U64 *rdis_vaddr_ranges;
  U64 rdis_count;
  U64 rdis_primary_idx;
  
  // rjf: identifier resolution maps
  E_String2NumMap *regs_map;
  E_String2NumMap *reg_alias_map;
  E_String2NumMap *locals_map; // (within `rdis[rdis_primary_idx]`)
  E_String2NumMap *member_map; // (within `rdis[rdis_primary_idx]`)
};

////////////////////////////////
//~ rjf: Parse Results

typedef struct E_Parse E_Parse;
struct E_Parse
{
  E_Token *last_token;
  E_Expr *expr;
  E_MsgList msgs;
};

////////////////////////////////
//~ rjf: Globals

global read_only E_String2NumMap e_string2num_map_nil = {0};
global read_only E_String2ExprMap e_string2expr_map_nil = {0};
global read_only E_Expr e_expr_nil = {&e_expr_nil, &e_expr_nil, &e_expr_nil};
thread_static E_ParseCtx *e_parse_ctx = 0;

////////////////////////////////
//~ rjf: Basic Helper Functions

internal U64 e_hash_from_string(U64 seed, String8 string);

////////////////////////////////
//~ rjf: Basic Map Functions

//- rjf: string -> num
internal E_String2NumMap e_string2num_map_make(Arena *arena, U64 slot_count);
internal void e_string2num_map_insert(Arena *arena, E_String2NumMap *map, String8 string, U64 num);
internal U64 e_num_from_string(E_String2NumMap *map, String8 string);
internal E_String2NumMapNodeArray e_string2num_map_node_array_from_map(Arena *arena, E_String2NumMap *map);
internal int e_string2num_map_node_qsort_compare__num_ascending(E_String2NumMapNode **a, E_String2NumMapNode **b);
internal void e_string2num_map_node_array_sort__in_place(E_String2NumMapNodeArray *array);

//- rjf: string -> expr
internal E_String2ExprMap e_string2expr_map_make(Arena *arena, U64 slot_count);
internal void e_string2expr_map_insert(Arena *arena, E_String2ExprMap *map, String8 string, E_Expr *expr);
internal void e_string2expr_map_inc_poison(E_String2ExprMap *map, String8 string);
internal void e_string2expr_map_dec_poison(E_String2ExprMap *map, String8 string);
internal E_Expr *e_expr_from_string(E_String2ExprMap *map, String8 string);

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);
internal E_String2NumMap *e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff);

////////////////////////////////
//~ rjf: Message Functions

internal void e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text);
internal void e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...);
internal void e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push);

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
internal U32 e_parse_ctx_idx_from_rdi(RDI_Parsed *rdi);

////////////////////////////////
//~ rjf: Expression Tree Building Functions

internal E_Expr *e_push_expr(Arena *arena, E_ExprKind kind, void *location);
internal void e_expr_push_child(E_Expr *parent, E_Expr *child);

////////////////////////////////
//~ rjf: Parsing Functions

internal E_TypeKey e_leaf_type_from_name(String8 name);
internal E_TypeKey e_type_from_expr(E_Expr *expr);
internal void e_push_leaf_ident_exprs_from_expr__in_place(Arena *arena, E_String2ExprMap *map, E_Expr *expr);
internal E_Parse e_parse_type_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);
internal E_Parse e_parse_expr_from_text_tokens__prec(Arena *arena, String8 text, E_TokenArray *tokens, S64 max_precedence);
internal E_Parse e_parse_expr_from_text_tokens(Arena *arena, String8 text, E_TokenArray *tokens);

#endif // EVAL_PARSE_H
