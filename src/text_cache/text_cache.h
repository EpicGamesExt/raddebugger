// Copyright (c) 2024 Epic Games Tools
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

typedef struct TXT_TextInfo TXT_TextInfo;
struct TXT_TextInfo
{
  U64 lines_count;
  Rng1U64 *lines_ranges;
  U64 lines_max_size;
  TXT_LineEndKind line_end_kind;
  TXT_TokenArray tokens;
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
//~ rjf: Cache Types

typedef struct TXT_Node TXT_Node;
struct TXT_Node
{
  // rjf: links
  TXT_Node *next;
  TXT_Node *prev;
  
  // rjf: key
  U128 hash;
  TXT_LangKind lang;
  
  // rjf: artifacts
  Arena *arena;
  TXT_TextInfo info;
  
  // rjf: metadata
  B32 is_working;
  U64 scope_ref_count;
  U64 last_time_touched_us;
  U64 last_user_clock_idx_touched;
  U64 load_count;
};

typedef struct TXT_Slot TXT_Slot;
struct TXT_Slot
{
  TXT_Node *first;
  TXT_Node *last;
};

typedef struct TXT_Stripe TXT_Stripe;
struct TXT_Stripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
};

////////////////////////////////
//~ rjf: Scoped Access

typedef struct TXT_Touch TXT_Touch;
struct TXT_Touch
{
  TXT_Touch *next;
  U128 hash;
  TXT_LangKind lang;
};

typedef struct TXT_Scope TXT_Scope;
struct TXT_Scope
{
  TXT_Scope *next;
  TXT_Touch *top_touch;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct TXT_TCTX TXT_TCTX;
struct TXT_TCTX
{
  Arena *arena;
  TXT_Scope *free_scope;
  TXT_Touch *free_touch;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct TXT_Shared TXT_Shared;
struct TXT_Shared
{
  Arena *arena;
  
  // rjf: user clock
  U64 user_clock_idx;
  
  // rjf: cache
  U64 slots_count;
  U64 stripes_count;
  TXT_Slot *slots;
  TXT_Stripe *stripes;
  TXT_Node **stripes_free_nodes;
  
  // rjf: user -> parse thread
  U64 u2p_ring_size;
  U8 *u2p_ring_base;
  U64 u2p_ring_write_pos;
  U64 u2p_ring_read_pos;
  OS_Handle u2p_ring_cv;
  OS_Handle u2p_ring_mutex;
  
  // rjf: evictor thread
  OS_Handle evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static TXT_TCTX *txt_tctx = 0;
global TXT_Shared *txt_shared = 0;

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
//~ rjf: Main Layer Initialization

internal void txt_init(void);

////////////////////////////////
//~ rjf: Thread Context Initialization

internal void txt_tctx_ensure_inited(void);

////////////////////////////////
//~ rjf: Scoped Access

internal TXT_Scope *txt_scope_open(void);
internal void txt_scope_close(TXT_Scope *scope);
internal void txt_scope_touch_node__stripe_r_guarded(TXT_Scope *scope, TXT_Node *node);

////////////////////////////////
//~ rjf: Cache Lookups

internal TXT_TextInfo txt_text_info_from_hash_lang(TXT_Scope *scope, U128 hash, TXT_LangKind lang);
internal TXT_TextInfo txt_text_info_from_key_lang(TXT_Scope *scope, U128 key, TXT_LangKind lang, U128 *hash_out);

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

////////////////////////////////
//~ rjf: Parse Threads

internal B32 txt_u2p_enqueue_req(U128 hash, TXT_LangKind lang, U64 endt_us);
internal void txt_u2p_dequeue_req(U128 *hash_out, TXT_LangKind *lang_out);
ASYNC_WORK_DEF(txt_parse_work);

////////////////////////////////
//~ rjf: Evictor Threads

internal void txt_evictor_thread__entry_point(void *p);

#endif // TEXT_CACHE_H
