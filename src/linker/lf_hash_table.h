// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LF_HASH_TABLE_H
#define LF_HASH_TABLE_H

typedef struct LFHT_Node
{
  void             *data;
  struct LFHT_Node *children[4];
} LFHT_Node;

typedef struct LFHT_NodeChunk
{
  struct LFHT_NodeChunk *next;
  U64                    max;
  U64                    count;
  LFHT_Node             *v;
} LFHT_NodeChunk;

typedef struct LFHT_NodeChunkList
{
  U64             chunk_count;
  U64             total_count;
  LFHT_NodeChunk *first;
  LFHT_NodeChunk *last;
} LFHT_NodeChunkList;

#define LFHT_IS_KEY_EQUAL_FUNC(name) B32 name(void *ud, void *a, void *b)
typedef LFHT_IS_KEY_EQUAL_FUNC(LFHT_IsKeyEqualFunc);

internal B32 lfht_insert(Arena *arena, LFHT_NodeChunkList *node_chunks, LFHT_Node **root_ptr, U64 hash, void *data, LFHT_IsKeyEqualFunc *is_key_equal, void *is_key_equal_ud);

internal void ** lfht_data_from_node_chunk_lists(Arena *arena, U64 total_count, U64 list_count, LFHT_NodeChunkList *lists);

#endif // LF_HASH_TABLE_H
