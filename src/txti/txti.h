// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef TXTI_H
#define TXTI_H

////////////////////////////////
//~ NOTE(rjf): Text Info Layer Overview (8/30/2023)
//
// This layer's purpose is to provide access to a mutable cache of parsed
// information about textual data, backed by filesystem contents. This
// cache associates unique handles (`TXTI_Handle`) with "entities", where
// entities contain information about textual contents of each line, how many
// lines of text the entity contains, and the tokenization of each line.
//
// While it is generally correct for a debugger to only provide read-only UIs
// for text viewing (so that the user does not mistakenly modify a buffer and
// continue debugging with it, even if the line or symbol info is no longer
// reflective of the source code), there are cases where read/write buffers are
// useful, and so this layer needs to support that, and the disabling of writes
// can remain as a high-level UI decision (rather than a lack of capability of
// the debugger). Mutable buffers may be used for editing config files (or
// otherwise any files that are not relevant to actively-used debug info),
// or debug logs.
//
// This layer is also responsible for hot-reloading changed files from disk,
// *and* for reporting conflicts, if a buffer was mutated within the process,
// but was also modified on disk.
//
// In order to avoid hanging UI while larger files are being edited or lexed,
// all buffer loading, mutation, & parsing happen on "mutator threads". These
// threads consume messages (`TXTI_Msg`), which command them to reload a file
// from disk, or to replace textual ranges in a buffer. After completing those
// operations, the buffer is lexed/parsed.
//
// Entities have *two* buffer data structures -- this allows a mutator thread
// to apply edits to one while the other can be read by user threads. For each
// editing operation, the mutator threads apply identical operations in a
// *rotation-based* order. If the currently-viewable buffer is slot 0, the edit
// will first be applied to slot 1, and before edits are reflected in slot 0,
// the mutator thread will bump a "buffer mutation counter", such that before
// any edits are made to slot 0, the viewable buffer is changed to being within
// slot *1*.
//
// Importantly, entities map to a *unique* mutator thread -- it is not possible
// for multiple mutator threads to be attempting to write to the same entity at
// the same time, as this could not produce meaningful or coherent results.
// This way, all edits to each entity are applied serially.

////////////////////////////////
//~ rjf: Handle Type

typedef struct TXTI_Handle TXTI_Handle;
struct TXTI_Handle
{
  U64 u64[2];
};

////////////////////////////////
//~ rjf: Parsed Text Info Types

typedef enum TXTI_LineEndKind
{
  TXTI_LineEndKind_Null,
  TXTI_LineEndKind_LF,
  TXTI_LineEndKind_CRLF,
  TXTI_LineEndKind_COUNT
}
TXTI_LineEndKind;

typedef enum TXTI_TokenKind
{
  TXTI_TokenKind_Null,
  TXTI_TokenKind_Error,
  TXTI_TokenKind_Whitespace,
  TXTI_TokenKind_Keyword,
  TXTI_TokenKind_Identifier,
  TXTI_TokenKind_Numeric,
  TXTI_TokenKind_String,
  TXTI_TokenKind_Symbol,
  TXTI_TokenKind_Comment,
  TXTI_TokenKind_Meta, // preprocessor, etc.
  TXTI_TokenKind_COUNT
}
TXTI_TokenKind;

typedef struct TXTI_Token TXTI_Token;
struct TXTI_Token
{
  TXTI_TokenKind kind;
  Rng1U64 range;
};

typedef struct TXTI_TokenChunkNode TXTI_TokenChunkNode;
struct TXTI_TokenChunkNode
{
  TXTI_TokenChunkNode *next;
  U64 count;
  U64 cap;
  TXTI_Token *v;
};

typedef struct TXTI_TokenChunkList TXTI_TokenChunkList;
struct TXTI_TokenChunkList
{
  TXTI_TokenChunkNode *first;
  TXTI_TokenChunkNode *last;
  U64 chunk_count;
  U64 token_count;
};

typedef struct TXTI_TokenNode TXTI_TokenNode;
struct TXTI_TokenNode
{
  TXTI_TokenNode *next;
  TXTI_Token v;
};

typedef struct TXTI_TokenList TXTI_TokenList;
struct TXTI_TokenList
{
  TXTI_TokenNode *first;
  TXTI_TokenNode *last;
  U64 count;
};

typedef struct TXTI_TokenArray TXTI_TokenArray;
struct TXTI_TokenArray
{
  U64 count;
  TXTI_Token *v;
};

typedef struct TXTI_TokenArrayArray TXTI_TokenArrayArray;
struct TXTI_TokenArrayArray
{
  U64 count;
  TXTI_TokenArray *v;
};

////////////////////////////////
//~ rjf: Language Kinds

typedef enum TXTI_LangKind
{
  TXTI_LangKind_Null,
  TXTI_LangKind_C,
  TXTI_LangKind_CPlusPlus,
  TXTI_LangKind_COUNT
}
TXTI_LangKind;

typedef TXTI_TokenArray TXTI_LangLexFunctionType(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Buffer Entity Types

#define TXTI_ENTITY_BUFFER_COUNT 2

typedef struct TXTI_Buffer TXTI_Buffer;
struct TXTI_Buffer
{
  // rjf: arenas
  Arena *data_arena;
  Arena *analysis_arena;
  
  // rjf: raw textual data
  String8 data;
  
  // rjf: line range info
  U64 lines_count;
  Rng1U64 *lines_ranges;
  U64 lines_max_size;
  
  // rjf: tokens
  TXTI_TokenArray tokens;
};

typedef struct TXTI_Entity TXTI_Entity;
struct TXTI_Entity
{
  // rjf: top-level info
  TXTI_Entity *next;
  String8 path;
  U64 id;
  U64 timestamp;
  U64 mut_gen;
  
  // rjf: metadata
  TXTI_LineEndKind line_end_kind;
  TXTI_LangKind lang_kind;
  U64 bytes_processed;
  U64 bytes_to_process;
  U64 working_count;
  
  // rjf: double-buffered mutable text buffers
  U64 buffer_apply_gen;
  TXTI_Buffer buffers[TXTI_ENTITY_BUFFER_COUNT];
};

typedef struct TXTI_EntitySlot TXTI_EntitySlot;
struct TXTI_EntitySlot
{
  TXTI_Entity *first;
  TXTI_Entity *last;
};

typedef struct TXTI_EntityMap TXTI_EntityMap;
struct TXTI_EntityMap
{
  U64 slots_count;
  TXTI_EntitySlot *slots;
};

////////////////////////////////
//~ rjf: Striped Access Types

typedef struct TXTI_Stripe TXTI_Stripe;
struct TXTI_Stripe
{
  Arena *arena;
  OS_Handle cv;
  OS_Handle rw_mutex;
};

typedef struct TXTI_StripeTable TXTI_StripeTable;
struct TXTI_StripeTable
{
  U64 count;
  TXTI_Stripe *v;
};

////////////////////////////////
//~ rjf: Entity Introspection Result Types

typedef struct TXTI_BufferInfo TXTI_BufferInfo;
struct TXTI_BufferInfo
{
  String8 path;
  U64 timestamp;
  TXTI_LineEndKind line_end_kind;
  TXTI_LangKind lang_kind;
  U64 total_line_count;
  U64 last_line_size;
  U64 max_line_size;
  U64 mut_gen;
  U64 buffer_apply_gen;
  U64 bytes_processed;
  U64 bytes_to_process;
};

typedef struct TXTI_Slice TXTI_Slice;
struct TXTI_Slice
{
  U64 line_count;
  String8 *line_text;
  Rng1U64 *line_ranges;
  TXTI_TokenArray *line_tokens;
};

////////////////////////////////
//~ rjf: User -> Mutator Thread Messages

typedef enum TXTI_MsgKind
{
  TXTI_MsgKind_Null,
  TXTI_MsgKind_Append,
  TXTI_MsgKind_Reload,
  TXTI_MsgKind_COUNT
}
TXTI_MsgKind;

typedef struct TXTI_Msg TXTI_Msg;
struct TXTI_Msg
{
  TXTI_MsgKind kind;
  TXTI_Handle handle;
  String8 string;
};

typedef struct TXTI_MsgNode TXTI_MsgNode;
struct TXTI_MsgNode
{
  TXTI_MsgNode *next;
  TXTI_Msg v;
};

typedef struct TXTI_MsgList TXTI_MsgList;
struct TXTI_MsgList
{
  TXTI_MsgNode *first;
  TXTI_MsgNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Central State

typedef struct TXTI_MutThread TXTI_MutThread;
struct TXTI_MutThread
{
  OS_Handle thread;
  Arena *msg_arena;
  TXTI_MsgList msg_list;
  OS_Handle msg_mutex;
  OS_Handle msg_cv;
};

typedef struct TXTI_State TXTI_State;
struct TXTI_State
{
  // rjf: arena
  Arena *arena;
  
  // rjf: entities state
  TXTI_EntityMap entity_map;
  TXTI_StripeTable entity_map_stripes;
  U64 entity_id_gen;
  
  // rjf: mutator threads
  U64 mut_thread_count;
  TXTI_MutThread *mut_threads;
  
  // rjf: detector thread
  OS_Handle detector_thread;
};

////////////////////////////////
//~ rjf: Globals

global TXTI_State *txti_state = 0;

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void txti_init(void);

////////////////////////////////
//~ rjf: Basic Helpers

internal U64 txti_hash_from_string(String8 string);
internal TXTI_LangKind txti_lang_kind_from_extension(String8 extension);

////////////////////////////////
//~ rjf: Token Type Functions

internal void txti_token_chunk_list_push(Arena *arena, TXTI_TokenChunkList *list, U64 cap, TXTI_Token *token);
internal void txti_token_list_push(Arena *arena, TXTI_TokenList *list, TXTI_Token *token);
internal TXTI_TokenArray txti_token_array_from_chunk_list(Arena *arena, TXTI_TokenChunkList *list);
internal TXTI_TokenArray txti_token_array_from_list(Arena *arena, TXTI_TokenList *list);

////////////////////////////////
//~ rjf: Lexing Functions

internal TXTI_TokenArray txti_token_array_from_string__cpp(Arena *arena, U64 *bytes_processed_counter, String8 string);

////////////////////////////////
//~ rjf: Message Type Functions

internal void txti_msg_list_push(Arena *arena, TXTI_MsgList *msgs, TXTI_Msg *msg);
internal void txti_msg_list_concat_in_place(TXTI_MsgList *dst, TXTI_MsgList *src);
internal TXTI_MsgList txti_msg_list_deep_copy(Arena *arena, TXTI_MsgList *src);

////////////////////////////////
//~ rjf: Entities API

//- rjf: opening entities & correllation w/ path
internal TXTI_Handle txti_handle_from_path(String8 path);

//- rjf: buffer introspection
internal TXTI_BufferInfo txti_buffer_info_from_handle(Arena *arena, TXTI_Handle handle);
internal TXTI_Slice txti_slice_from_handle_line_range(Arena *arena, TXTI_Handle handle, Rng1S64 line_range);
internal String8 txti_string_from_handle_txt_rng(Arena *arena, TXTI_Handle handle, TxtRng range);
internal String8 txti_string_from_handle_line_num(Arena *arena, TXTI_Handle handle, S64 line_num);
internal Rng1U64 txti_expr_range_from_line_off_range_string_tokens(U64 off, Rng1U64 line_range, String8 line_text, TXTI_TokenArray *line_tokens);
internal TxtRng txti_expr_range_from_handle_pt(TXTI_Handle handle, TxtPt pt);

//- rjf: buffer mutations
internal void txti_reload(TXTI_Handle handle, String8 path);
internal void txti_append(TXTI_Handle handle, String8 string);

////////////////////////////////
//~ rjf: Mutator Threads

internal void txti_mut_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Detector Thread

internal void txti_detector_thread_entry_point(void *p);

#endif //TXTI_H
