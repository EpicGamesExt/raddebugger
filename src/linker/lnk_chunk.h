// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

#define LNK_DEBUG_CHUNKS 1

#if LNK_DEBUG_CHUNKS
# define lnk_chunk_set_debugf(a, c, f, ...) do { (c)->debug = push_str8f((a), f, __VA_ARGS__); } while(0)
#else
# define lnk_chunk_set_debugf(a, c, f, ...) (void)(c)
#endif

////////////////////////////////

typedef struct LNK_ChunkRef
{
  U64 sect_id;
  U64 chunk_id;
} LNK_ChunkRef;

typedef enum
{
  LNK_Chunk_Null,
  LNK_Chunk_Leaf,
  LNK_Chunk_LeafArray,
  LNK_Chunk_List,
} LNK_ChunkType;

typedef struct LNK_Chunk
{
  LNK_ChunkRef      ref;
  LNK_ChunkType     type;
  U64               align;
  U64               min_size;
  B32               is_discarded;
  B32               sort_chunk;
  String8           sort_idx;
  U64               input_idx;
  COFF_SectionFlags flags;
  struct LNK_Chunk *associate;
  union {
    String8                leaf;
    struct LNK_ChunkList  *list;
    struct LNK_ChunkArray *arr;
  } u;
  struct LNK_Obj *obj;
#if LNK_DEBUG_CHUNKS
  String8 debug;
#endif
} LNK_Chunk, * LNK_ChunkPtr;

typedef struct LNK_ChunkNode
{
  struct LNK_ChunkNode *next;
  LNK_ChunkPtr          data;
} LNK_ChunkNode;

typedef struct LNK_ChunkArray
{
  U64           count;
  LNK_ChunkPtr *v;
} LNK_ChunkArray;

typedef struct LNK_ChunkList
{
  U64            count;
  LNK_ChunkNode *first;
  LNK_ChunkNode *last;
} LNK_ChunkList;

typedef enum LNK_ChunkOpType
{
  LNK_ChunkOp_Null,
  LNK_ChunkOp_WriteString,
  LNK_ChunkOp_Align,
  LNK_ChunkOp_Begin,
  LNK_ChunkOp_End,
  LNK_ChunkOp_EndVirt,
} LNK_ChunkOpType;

typedef struct LNK_ChunkOp
{
  struct LNK_ChunkOp *next;
  LNK_ChunkOpType     type;
  union {
    String8    string;
    LNK_Chunk *chunk;
    struct {
      U64 val;
      U64 x;
    } align;
    LNK_Chunk *leaf;
  } u;
} LNK_ChunkOp;

typedef struct LNK_ChunkOpList
{
  U64          total_chunk_count;
  LNK_ChunkOp *first;
  LNK_ChunkOp *last;
} LNK_ChunkOpList;

typedef struct LNK_ChunkPad
{
  U64 off;
  U64 size;
} LNK_ChunkPad;

typedef struct LNK_ChunkPadArray
{
  U64           count;
  LNK_ChunkPad *v;
} LNK_ChunkPadArray;
typedef struct LNK_ChunkPadArrayNode
{
  struct LNK_ChunkPadArrayNode *next;
  U64                 cap;
  LNK_ChunkPadArray data;
} LNK_ChunkPadArrayNode;
typedef struct LNK_ChunkPadArrayList
{
  U64                    count;
  LNK_ChunkPadArrayNode *first;
  LNK_ChunkPadArrayNode *last;
} LNK_ChunkPadArrayList;

typedef struct LNK_ChunkLayout
{
  U64                     total_count;
  LNK_Chunk             **chunk_ptr_array;       // discarded chunks point to g_null_chunk
  U64                    *chunk_off_array;       // discarded chunks have offset set to max_U64
  U64                    *chunk_file_size_array; // discarded chunks have offset set to max_U64
  U64                    *chunk_virt_size_array; // discarded chunks have offset set to max_U64
  U64                     pad_array_count;
  LNK_ChunkPadArray      *pad_array;
} LNK_ChunkLayout;

typedef struct LNK_ChunkManager
{
  LNK_Chunk *root;
  U64        total_chunk_count;
} LNK_ChunkManager;

////////////////////////////////

typedef struct
{
  U64              offset;
  LNK_ChunkLayout *layout;
} LNK_OffsetChunks;

typedef struct
{
  LNK_ChunkLayout  layout;
  String8          buffer;
  U8               fill_byte;
  Rng1U64         *ranges;
} LNK_ChunkLayoutSerializer;

////////////////////////////////

read_only global LNK_Chunk  g_null_chunk      = { 0, 0, /* is_discarded: */ 1 };
read_only global LNK_Chunk *g_null_chunk_ptr  = &g_null_chunk;

////////////////////////////////

internal LNK_ChunkRef lnk_chunk_ref(U64 sect_id, U64 chunk_id);
internal B32          lnk_chunk_ref_is_equal(LNK_ChunkRef a, LNK_ChunkRef b);

internal LNK_ChunkNode *  lnk_chunk_list_push(Arena *arena, LNK_ChunkList *list, LNK_Chunk *chunk);
internal void             lnk_chunk_list_concat_in_place(LNK_ChunkList *list, LNK_ChunkList *to_concat);
internal void             lnk_chunk_list_concat_in_place_arr(LNK_ChunkList *list, LNK_ChunkList *arr, U64 count);
internal LNK_ChunkList ** lnk_make_chunk_list_arr_arr(Arena *arena, U64 slot_count, U64 per_count);
internal void             lnk_chunk_array_sort(LNK_ChunkArray arr);

internal LNK_ChunkManager * lnk_chunk_manager_alloc(Arena *arena, U64 id, U64 align);
internal LNK_Chunk *        lnk_chunk_push(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index);
internal LNK_Chunk *        lnk_chunk_push_leaf(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index, void *raw_ptr, U64 raw_size);
internal LNK_Chunk *        lnk_chunk_push_list(Arena *arena, LNK_ChunkManager *cman, LNK_Chunk *parent, String8 sort_index);
internal LNK_ChunkNode *    lnk_chunk_deep_copy(Arena *arena, LNK_Chunk *chunk);
internal LNK_ChunkNode *    lnk_merge_chunks(Arena *arena, LNK_ChunkManager *dst_cman, LNK_Chunk *dst, LNK_Chunk *src, U64 *id_map_out, U64 id_map_max);
internal void               lnk_chunk_associate(LNK_Chunk *head, LNK_Chunk *associate);
internal B32                lnk_chunk_is_discarded(LNK_Chunk *chunk);
internal U64                lnk_chunk_get_size(LNK_Chunk *chunk);
internal U64                lnk_chunk_list_get_node_count(LNK_Chunk *chunk);

internal void lnk_chunk_op_list_push_node(LNK_ChunkOpList *list, LNK_ChunkOp *op);

internal LNK_ChunkOp * lnk_push_chunk_op_begin(Arena *arena, U64 chunk_id);
internal LNK_ChunkOp * lnk_push_chunk_op_end_virt(Arena *arena);
internal LNK_ChunkOp * lnk_push_chunk_op_end_file(Arena *arena);
internal LNK_ChunkOp * lnk_push_chunk_op_align(Arena *arena, U64 align, U64 val);
internal LNK_ChunkOp * lnk_push_chunk_op_write(Arena *arena, String8 string);

internal LNK_ChunkLayout lnk_layout_from_chunk(Arena *arena, LNK_Chunk *root, U64 total_chunk_count);
internal LNK_ChunkLayout lnk_build_chunk_layout(Arena *arena, LNK_ChunkManager *cman);

#define LNK_CHUNK_VISITOR_SIG(name) B32 name(U64 sect_id, LNK_Chunk *chunk, void *ud)
typedef LNK_CHUNK_VISITOR_SIG(LNK_ChunkVisitorSig);
internal void lnk_visit_chunks(U64 sect_id, LNK_Chunk *root, LNK_ChunkVisitorSig *cb, void *ud);

internal LNK_ChunkNode * lnk_chunk_ptr_list_reserve(Arena *arena, LNK_ChunkList *list, U64 count);
internal String8Array    lnk_data_arr_from_chunk_ptr_list(Arena *arena, LNK_ChunkList list);
internal String8Array *  lnk_data_arr_from_chunk_ptr_list_arr(Arena *arena, LNK_ChunkList *list_arr, U64 count);

internal String8 lnk_string_from_chunk_type(LNK_ChunkType type);

