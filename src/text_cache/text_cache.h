// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TEXT_CACHE_H
#define TEXT_CACHE_H

////////////////////////////////
//~ rjf: Value Types

typedef enum TXT_LineEndKind
{
  TXT_LineEndKind_Null,
  TXT_LineEndKind_LF,
  TXT_LineEndKind_CRLF,
  TXT_LineEndKind_COUNT
}
TXT_LineEndKind;

typedef enum TXT_TokenKind
{
  TXT_TokenKind_Null,
  TXT_TokenKind_Error,
  TXT_TokenKind_Whitespace,
  TXT_TokenKind_Keyword,
  TXT_TokenKind_Identifier,
  TXT_TokenKind_Numeric,
  TXT_TokenKind_String,
  TXT_TokenKind_Symbol,
  TXT_TokenKind_Comment,
  TXT_TokenKind_Meta, // preprocessor, etc.
  TXT_TokenKind_COUNT
}
TXT_TokenKind;

typedef struct TXT_Token TXT_Token;
struct TXT_Token
{
  TXT_TokenKind kind;
  Rng1U64 range;
};

typedef struct TXT_TokenChunkNode TXT_TokenChunkNode;
struct TXT_TokenChunkNode
{
  TXT_TokenChunkNode *next;
  U64 count;
  U64 cap;
  TXT_Token *v;
};

typedef struct TXT_TokenChunkList TXT_TokenChunkList;
struct TXT_TokenChunkList
{
  TXT_TokenChunkNode *first;
  TXT_TokenChunkNode *last;
  U64 chunk_count;
  U64 token_count;
};

typedef struct TXT_TokenNode TXT_TokenNode;
struct TXT_TokenNode
{
  TXT_TokenNode *next;
  TXT_Token v;
};

typedef struct TXT_TokenList TXT_TokenList;
struct TXT_TokenList
{
  TXT_TokenNode *first;
  TXT_TokenNode *last;
  U64 count;
};

typedef struct TXT_TokenArray TXT_TokenArray;
struct TXT_TokenArray
{
  U64 count;
  TXT_Token *v;
};

typedef struct TXT_TokenArrayArray TXT_TokenArrayArray;
struct TXT_TokenArrayArray
{
  U64 count;
  TXT_TokenArray *v;
};

typedef struct TXT_ScopeNode TXT_ScopeNode;
struct TXT_ScopeNode
{
  U64 first_num;
  U64 last_num;
  U64 next_num;
  U64 parent_num;
  Rng1U64 token_idx_range;
};

typedef struct TXT_ScopeNodeArray TXT_ScopeNodeArray;
struct TXT_ScopeNodeArray
{
  TXT_ScopeNode *v;
  U64 count;
};

typedef struct TXT_ScopePt TXT_ScopePt;
struct TXT_ScopePt
{
  U64 token_idx;
  U64 scope_idx;
};

typedef struct TXT_ScopePtArray TXT_ScopePtArray;
struct TXT_ScopePtArray
{
  TXT_ScopePt *v;
  U64 count;
};

typedef struct TXT_TextInfo TXT_TextInfo;
struct TXT_TextInfo
{
  U64 lines_count;
  Rng1U64 *lines_ranges;
  U64 lines_max_size;
  TXT_LineEndKind line_end_kind;
  TXT_TokenArray tokens;
  TXT_ScopePtArray scope_pts;
  TXT_ScopeNodeArray scope_nodes;
  U64 bytes_processed;
  U64 bytes_to_process;
};

typedef struct TXT_LineTokensSlice TXT_LineTokensSlice;
struct TXT_LineTokensSlice
{
  TXT_TokenArray *line_tokens;
};

////////////////////////////////
//~ rjf: Language Kind Types

typedef enum TXT_LangKind
{
  TXT_LangKind_Null,
  TXT_LangKind_C,
  TXT_LangKind_CPlusPlus,
  TXT_LangKind_Odin,
  TXT_LangKind_Jai,
  TXT_LangKind_Zig,
  TXT_LangKind_DisasmX64Intel,
  TXT_LangKind_COUNT
}
TXT_LangKind;

typedef TXT_TokenArray TXT_LangLexFunctionType(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Globals

read_only global TXT_ScopeNode txt_scope_node_nil = {0};

////////////////////////////////
//~ rjf: Basic Helpers

internal TXT_LangKind txt_lang_kind_from_extension(String8 extension);
internal String8 txt_extension_from_lang_kind(TXT_LangKind kind);
internal TXT_LangKind txt_lang_kind_from_arch(Arch arch);
internal TXT_LangLexFunctionType *txt_lex_function_from_lang_kind(TXT_LangKind kind);

////////////////////////////////
//~ rjf: Token Type Functions

internal void txt_token_chunk_list_push(Arena *arena, TXT_TokenChunkList *list, U64 cap, TXT_Token *token);
internal void txt_token_list_push(Arena *arena, TXT_TokenList *list, TXT_Token *token);
internal TXT_TokenArray txt_token_array_from_chunk_list(Arena *arena, TXT_TokenChunkList *list);
internal TXT_TokenArray txt_token_array_from_list(Arena *arena, TXT_TokenList *list);

////////////////////////////////
//~ rjf: Lexing Functions

internal TXT_TokenArray txt_token_array_from_string__c_cpp(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__odin(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__jai(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__zig(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__disasm_x64_intel(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Text Info Extractor Helpers

internal U64 txt_off_from_info_pt(TXT_TextInfo *info, TxtPt pt);
internal TxtPt txt_pt_from_info_off__linear_scan(TXT_TextInfo *info, U64 off);
internal TXT_TokenArray txt_token_array_from_info_line_num__linear_scan(TXT_TextInfo *info, S64 line_num);
internal Rng1U64 txt_expr_off_range_from_line_off_range_string_tokens(U64 off, Rng1U64 line_range, String8 line_text, TXT_TokenArray *line_tokens);
internal Rng1U64 txt_expr_off_range_from_info_data_pt(TXT_TextInfo *info, String8 data, TxtPt pt);
internal String8 txt_string_from_info_data_txt_rng(TXT_TextInfo *info, String8 data, TxtRng rng);
internal String8 txt_string_from_info_data_line_num(TXT_TextInfo *info, String8 data, S64 line_num);
internal TXT_LineTokensSlice txt_line_tokens_slice_from_info_data_line_range(Arena *arena, TXT_TextInfo *info, String8 data, Rng1S64 line_range);
internal TXT_ScopeNode *txt_scope_node_from_info_num(TXT_TextInfo *info, U64 num);
internal TXT_ScopeNode *txt_scope_node_from_info_off(TXT_TextInfo *info, U64 off);
internal TXT_ScopeNode *txt_scope_node_from_info_pt(TXT_TextInfo *info, TxtPt pt);

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Lookups

internal void *txt_artifact_create(String8 key, B32 *retry_out);
internal void txt_artifact_destroy(void *ptr);
internal TXT_TextInfo txt_text_info_from_hash_lang(Access *access, U128 hash, TXT_LangKind lang);
internal TXT_TextInfo txt_text_info_from_key_lang(Access *access, C_Key key, TXT_LangKind lang, U128 *hash_out);

#endif // TEXT_CACHE_H
