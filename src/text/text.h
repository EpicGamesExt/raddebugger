// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TEXT_H
#define TEXT_H

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
  TXT_TokenKind_Char,
  TXT_TokenKind_Symbol,
  TXT_TokenKind_LineComment,
  TXT_TokenKind_BlockComment,
  TXT_TokenKind_Meta, // preprocessor, etc.
  TXT_TokenKind_COUNT
}
TXT_TokenKind;

typedef struct TXT_TokenizerRule TXT_TokenizerRule;
struct TXT_TokenizerRule
{
  TXT_TokenKind token_kind;
  String8 open_string;
  String8 close_string;
  U32 close_advance;
  B32 nesting;
  B32 escaping;
  U32 parent_num;
};

typedef struct TXT_TokenizerRulePtrNode TXT_TokenizerRulePtrNode;
struct TXT_TokenizerRulePtrNode
{
  TXT_TokenizerRulePtrNode *next;
  TXT_TokenizerRule *v;
};

typedef struct TXT_TokenizerRuleArray TXT_TokenizerRuleArray;
struct TXT_TokenizerRuleArray
{
  TXT_TokenizerRule *v;
  U64 count;
};

typedef struct TXT_Token TXT_Token;
struct TXT_Token
{
  TXT_TokenKind kind;
  Rng1U64 range;
};

typedef struct TXT_TokenPt TXT_TokenPt;
struct TXT_TokenPt
{
  TXT_TokenKind kind;
  U64 off;
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
  U64 scope_num;
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
  U64 big_token_pts_count;
  TXT_TokenPt *big_token_pts;
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
//~ rjf: Value Modification Patches

typedef struct TXT_Patch TXT_Patch;
struct TXT_Patch
{
  Rng1U64 range;
  String8 replace;
};

typedef struct TXT_PatchNode TXT_PatchNode;
struct TXT_PatchNode
{
  TXT_PatchNode *next;
  TXT_PatchNode *prev;
  TXT_Patch v;
};

typedef struct TXT_PatchList TXT_PatchList;
struct TXT_PatchList
{
  TXT_PatchNode *first;
  TXT_PatchNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Value Reading Types

typedef struct TXT_LineMapRangeNode TXT_LineMapRangeNode;
struct TXT_LineMapRangeNode
{
  TXT_LineMapRangeNode *next;
  Rng1U64 num_range;
  Rng1U64 *ranges;
  S64 delta;
};

typedef struct TXT_LineMap TXT_LineMap;
struct TXT_LineMap
{
  TXT_LineMapRangeNode *first_range;
  TXT_LineMapRangeNode *last_range;
  U64 total_line_count;
};

typedef struct TXT_TokenPtMapRangeNode TXT_TokenPtMapRangeNode;
struct TXT_TokenPtMapRangeNode
{
  TXT_TokenPtMapRangeNode *next;
  Rng1U64 num_range;
  TXT_TokenPt *pts;
  S64 delta;
};

typedef struct TXT_TokenPtMap TXT_TokenPtMap;
struct TXT_TokenPtMap
{
  TXT_TokenPtMapRangeNode *first_range;
  TXT_TokenPtMapRangeNode *last_range;
  U64 total_pt_count;
};

typedef struct TXT_Patched TXT_Patched;
struct TXT_Patched
{
  MemoryMap memory_map;
  U64 size;
  TXT_LineMap line_map;
  TXT_TokenPtMap token_pt_map;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/text.meta.h"

////////////////////////////////
//~ rjf: Language Kind Types

typedef TXT_TokenArray TXT_LangLexFunctionType(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Globals

read_only global TXT_ScopeNode txt_scope_node_nil = {0};
read_only global Rng1U64 txt_info_line_range_nil = {0};
read_only global TXT_TextInfo txt_info_nil =
{
  1,
  &txt_info_line_range_nil,
  0,
  TXT_LineEndKind_Null,
};

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
//~ rjf: Patch Functions

internal void txt_patch_list_push_new(Arena *arena, TXT_PatchList *list, Rng1U64 range, String8 replace);

////////////////////////////////
//~ rjf: Lexing Functions

internal TXT_TokenArray txt_token_array_from_lang_kind_string(Arena *arena, TXT_LangKind lang_kind, String8 string);

internal TXT_TokenArray txt_token_array_from_string__c_cpp(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__odin(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__jai(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__zig(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__rust(Arena *arena, U64 *bytes_processed_counter, String8 string);
internal TXT_TokenArray txt_token_array_from_string__disasm_x64_intel(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Text Info Extractor Helpers

internal void txt_line_map_push(Arena *arena, TXT_LineMap *map, Rng1U64 num_range, Rng1U64 *ranges, S64 delta);
internal U64 txt_line_num_from_off(TXT_LineMap *map, U64 off);
internal Rng1U64 txt_range_from_line_num(TXT_LineMap *map, U64 num);
internal void txt_token_pt_map_push(Arena *arena, TXT_TokenPtMap *map, Rng1U64 num_range, TXT_TokenPt *pts, S64 delta);
internal U64 txt_token_pt_num_from_off(TXT_TokenPtMap *map, U64 off);
internal TXT_TokenPt txt_token_pt_from_num(TXT_TokenPtMap *map, U64 num);
internal TXT_TokenArray txt_token_array_from_data(Arena *arena, TXT_LangKind lang_kind, TXT_TokenPt ctx_token_pt, String8 data, U64 base_off, U64 limit);
internal TXT_Patched txt_patched_from_info_data_patches(Arena *arena, TXT_TextInfo *info, String8 data, TXT_PatchList *patches);

//~ TODO(rjf): old unpatched text viz code:

internal U64 txt_off_from_pt(TXT_TextInfo *info, TXT_PatchList *patches, TxtPt pt);
internal TxtPt txt_pt_from_off__linear_scan(TXT_TextInfo *info, TXT_PatchList *patches, U64 off);
internal TXT_TokenArray txt_token_array_from_info_line_num__linear_scan(TXT_TextInfo *info, S64 line_num);
internal Rng1U64 txt_expr_off_range_from_line_off_range_string_tokens(U64 off, Rng1U64 line_range, String8 line_text, TXT_TokenArray *line_tokens);
internal Rng1U64 txt_expr_off_range_from_info_data_pt(TXT_TextInfo *info, String8 data, TxtPt pt);
internal String8 txt_string_from_info_data_txt_rng(TXT_TextInfo *info, String8 data, TXT_PatchList *patches, TxtRng rng);
internal String8 txt_string_from_info_data_line_num(TXT_TextInfo *info, String8 data, S64 line_num);
internal TXT_LineTokensSlice txt_line_tokens_slice_from_info_data_line_range(Arena *arena, TXT_TextInfo *info, String8 data, Rng1S64 line_range);
internal TXT_ScopeNode *txt_scope_node_from_info_num(TXT_TextInfo *info, U64 num);
internal TXT_ScopeNode *txt_scope_node_from_info_off(TXT_TextInfo *info, U64 off);
internal TXT_ScopeNode *txt_scope_node_from_info_pt(TXT_TextInfo *info, TXT_PatchList *patches, TxtPt pt);

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Lookups

internal AC_Artifact txt_artifact_create(String8 key, B32 *cancel_signal, AC_Status *status_out, U64 *gen_out);
internal void txt_artifact_destroy(AC_Artifact artifact);
internal TXT_TextInfo txt_text_info_from_hash_lang(Access *access, U128 hash, TXT_LangKind lang);
internal TXT_TextInfo txt_text_info_from_key_lang(Access *access, C_Key key, TXT_LangKind lang, U128 *hash_out);

#endif // TEXT_H
