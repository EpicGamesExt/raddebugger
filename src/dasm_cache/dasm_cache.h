// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DASM_CACHE_H
#define DASM_CACHE_H

////////////////////////////////
//~ rjf: Stringification Types

typedef U32 DASM_StyleFlags;
enum
{
  DASM_StyleFlag_Addresses        = (1<<0),
  DASM_StyleFlag_CodeBytes        = (1<<1),
  DASM_StyleFlag_SourceFilesNames = (1<<2),
  DASM_StyleFlag_SourceLines      = (1<<3),
  DASM_StyleFlag_SymbolNames      = (1<<4),
};

typedef enum DASM_Syntax
{
  DASM_Syntax_Intel,
  DASM_Syntax_ATT,
  DASM_Syntax_COUNT
}
DASM_Syntax;

////////////////////////////////
//~ rjf: Disassembling Parameters Bundle

typedef struct DASM_Params DASM_Params;
struct DASM_Params
{
  U64 vaddr;
  Architecture arch;
  DASM_StyleFlags style_flags;
  DASM_Syntax syntax;
  U64 base_vaddr;
  String8 exe_path;
};

////////////////////////////////
//~ rjf: Instruction Types

typedef struct DASM_Inst DASM_Inst;
struct DASM_Inst
{
  U64 code_off;
  U64 addr;
  Rng1U64 text_range;
};

typedef struct DASM_InstChunkNode DASM_InstChunkNode;
struct DASM_InstChunkNode
{
  DASM_InstChunkNode *next;
  DASM_Inst *v;
  U64 cap;
  U64 count;
};

typedef struct DASM_InstChunkList DASM_InstChunkList;
struct DASM_InstChunkList
{
  DASM_InstChunkNode *first;
  DASM_InstChunkNode *last;
  U64 node_count;
  U64 inst_count;
};

typedef struct DASM_InstArray DASM_InstArray;
struct DASM_InstArray
{
  DASM_Inst *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Value Bundle Type

typedef struct DASM_Info DASM_Info;
struct DASM_Info
{
  U128 text_key;
  DASM_InstArray insts;
};

////////////////////////////////
//~ rjf: Cache Types

typedef struct DASM_Node DASM_Node;
struct DASM_Node
{
  // rjf: links
  DASM_Node *next;
  DASM_Node *prev;
  
  // rjf: key
  U128 hash;
  DASM_Params params;
  
  // rjf: value
  Arena *info_arena;
  DASM_Info info;
  
  // rjf: metadata
  B32 is_working;
  U64 scope_ref_count;
  U64 last_time_touched_us;
  U64 last_user_clock_idx_touched;
  U64 load_count;
};

typedef struct DASM_Slot DASM_Slot;
struct DASM_Slot
{
  DASM_Node *first;
  DASM_Node *last;
};

typedef struct DASM_Stripe DASM_Stripe;
struct DASM_Stripe
{
  Arena *arena;
  OS_Handle rw_mutex;
  OS_Handle cv;
  DASM_Node *free_node;
};

////////////////////////////////
//~ rjf: Scoped Access Types

typedef struct DASM_Touch DASM_Touch;
struct DASM_Touch
{
  DASM_Touch *next;
  U128 hash;
  DASM_Params params;
};

typedef struct DASM_Scope DASM_Scope;
struct DASM_Scope
{
  DASM_Scope *next;
  DASM_Touch *top_touch;
  U64 base_pos;
};

////////////////////////////////
//~ rjf: Thread Context

typedef struct DASM_TCTX DASM_TCTX;
struct DASM_TCTX
{
  Arena *arena;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct DASM_Shared DASM_Shared;
struct DASM_Shared
{
  Arena *arena;
  
  // rjf: user clock
  U64 user_clock_idx;
  
  // rjf: cache
  U64 slots_count;
  U64 stripes_count;
  DASM_Slot *slots;
  DASM_Stripe *stripes;
  
  // rjf: user -> parse thread
  U64 u2p_ring_size;
  U8 *u2p_ring_base;
  U64 u2p_ring_write_pos;
  U64 u2p_ring_read_pos;
  OS_Handle u2p_ring_cv;
  OS_Handle u2p_ring_mutex;
  
  // rjf: parse threads
  U64 parse_thread_count;
  OS_Handle *parse_threads;
  
  // rjf: evictor thread
  OS_Handle evictor_thread;
};

////////////////////////////////
//~ rjf: Globals

thread_static DASM_TCTX *dasm_tctx = 0;
global DASM_Shared *dasm_shared = 0;

////////////////////////////////
//~ rjf: Parameter Type Functions

internal B32 dasm_params_match(DASM_Params *a, DASM_Params *b);

////////////////////////////////
//~ rjf: Instruction Type Functions

internal void dasm_inst_chunk_list_push(Arena *arena, DASM_InstChunkList *list, U64 cap, DASM_Inst *inst);
internal DASM_InstArray dasm_inst_array_from_chunk_list(Arena *arena, DASM_InstChunkList *list);
internal U64 dasm_inst_array_idx_from_code_off__linear_scan(DASM_InstArray *array, U64 off);
internal U64 dasm_inst_array_code_off_from_idx(DASM_InstArray *array, U64 idx);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void dasm_init(void);

////////////////////////////////
//~ rjf: User Clock

internal void dasm_user_clock_tick(void);
internal U64 dasm_user_clock_idx(void);

////////////////////////////////
//~ rjf: Scoped Access

internal DASM_Scope *dasm_scope_open(void);
internal void dasm_scope_close(DASM_Scope *scope);
internal void dasm_scope_touch_node__stripe_r_guarded(DASM_Scope *scope, DASM_Node *node);

////////////////////////////////
//~ rjf: Cache Lookups

internal DASM_Info dasm_info_from_hash_params(DASM_Scope *scope, U128 hash, DASM_Params *params);
internal DASM_Info dasm_info_from_key_params(DASM_Scope *scope, U128 key, DASM_Params *params, U128 *hash_out);

////////////////////////////////
//~ rjf: Parse Threads

internal B32 dasm_u2p_enqueue_req(U128 hash, DASM_Params *params, U64 endt_us);
internal void dasm_u2p_dequeue_req(Arena *arena, U128 *hash_out, DASM_Params *params_out);
internal void dasm_parse_thread__entry_point(void *p);

////////////////////////////////
//~ rjf: Evictor Threads

internal void dasm_evictor_thread__entry_point(void *p);

#endif // DASM_CACHE_H
