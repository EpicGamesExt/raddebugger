// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL2_PARSER_H
#define EVAL2_PARSER_H

////////////////////////////////
//~ rjf: Maps

typedef struct EVAL_String2NumMapNode EVAL_String2NumMapNode;
struct EVAL_String2NumMapNode
{
  EVAL_String2NumMapNode *next;
  String8 string;
  U64 num;
};

typedef struct EVAL_String2NumMapSlot EVAL_String2NumMapSlot;
struct EVAL_String2NumMapSlot
{
  EVAL_String2NumMapNode *first;
  EVAL_String2NumMapNode *last;
};

typedef struct EVAL_String2NumMap EVAL_String2NumMap;
struct EVAL_String2NumMap
{
  U64 slots_count;
  EVAL_String2NumMapSlot *slots;
};

////////////////////////////////
//~ rjf: Token Types

typedef enum EVAL_TokenKind
{
  EVAL_TokenKind_Null,
  EVAL_TokenKind_Identifier,
  EVAL_TokenKind_Numeric,
  EVAL_TokenKind_StringLiteral,
  EVAL_TokenKind_CharLiteral,
  EVAL_TokenKind_Symbol,
  EVAL_TokenKind_COUNT
}
EVAL_TokenKind;

typedef struct EVAL_Token EVAL_Token;
struct EVAL_Token
{
  EVAL_TokenKind kind;
  Rng1U64 range;
};

typedef struct EVAL_TokenChunkNode EVAL_TokenChunkNode;
struct EVAL_TokenChunkNode
{
  EVAL_TokenChunkNode *next;
  EVAL_Token *v;
  U64 count;
  U64 cap;
};

typedef struct EVAL_TokenChunkList EVAL_TokenChunkList;
struct EVAL_TokenChunkList
{
  EVAL_TokenChunkNode *first;
  EVAL_TokenChunkNode *last;
  U64 node_count;
  U64 total_count;
};

typedef struct EVAL_TokenArray EVAL_TokenArray;
struct EVAL_TokenArray
{
  EVAL_Token *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Parser Types

typedef struct EVAL_ParseResult EVAL_ParseResult;
struct EVAL_ParseResult
{
  EVAL_Token *last_token;
  EVAL_Expr *expr;
  EVAL_ErrorList errors;
};

typedef struct EVAL_ParseCtx EVAL_ParseCtx;
struct EVAL_ParseCtx
{
  Architecture arch;
  U64 ip_voff;
  RADDBG_Parsed *rdbg;
  TG_Graph *type_graph;
  EVAL_String2NumMap *regs_map;
  EVAL_String2NumMap *reg_alias_map;
  EVAL_String2NumMap *locals_map;
  EVAL_String2NumMap *member_map;
  EVAL_String2NumMap *local_dynamic_type_override_map;
};

////////////////////////////////
//~ rjf: Globals

read_only global EVAL_String2NumMap eval_string2num_map_nil = {0};
global read_only EVAL_ParseResult eval_parse_result_nil = {0, &eval_expr_nil};

////////////////////////////////
//~ rjf: Basic Functions

internal U64 eval_hash_from_string(String8 string);

////////////////////////////////
//~ rjf: Map Functions

internal EVAL_String2NumMap eval_string2num_map_make(Arena *arena, U64 slot_count);
internal void eval_string2num_map_insert(Arena *arena, EVAL_String2NumMap *map, String8 string, U64 num);
internal U64 eval_num_from_string(EVAL_String2NumMap *map, String8 string);

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Fast Paths

internal EVAL_String2NumMap *eval_push_locals_map_from_raddbg_voff(Arena *arena, RADDBG_Parsed *rdbg, U64 voff);
internal EVAL_String2NumMap *eval_push_member_map_from_raddbg_voff(Arena *arena, RADDBG_Parsed *rdbg, U64 voff);

////////////////////////////////
//~ rjf: Tokenization Functions

#define eval_token_at_it(it, arr) (((it) < (arr)->v+(arr)->count) ? (*(it)) : eval_token_zero())
internal EVAL_Token eval_token_zero(void);
internal void eval_token_chunk_list_push(Arena *arena, EVAL_TokenChunkList *list, U64 chunk_size, EVAL_Token *token);
internal EVAL_TokenArray eval_token_array_from_chunk_list(Arena *arena, EVAL_TokenChunkList *list);
internal EVAL_TokenArray eval_token_array_from_text(Arena *arena, String8 text);
internal EVAL_TokenArray eval_token_array_make_first_opl(EVAL_Token *first, EVAL_Token *opl);

////////////////////////////////
//~ rjf: Parser Functions

internal TG_Key eval_leaf_type_from_name(RADDBG_Parsed *rdbg, String8 name);
internal EVAL_ParseResult eval_parse_type_from_text_tokens(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens);
internal EVAL_ParseResult eval_parse_expr_from_text_tokens__prec(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens, S64 max_precedence);
internal EVAL_ParseResult eval_parse_expr_from_text_tokens(Arena *arena, EVAL_ParseCtx *ctx, String8 text, EVAL_TokenArray *tokens);

#endif // EVAL2_PARSER_H
