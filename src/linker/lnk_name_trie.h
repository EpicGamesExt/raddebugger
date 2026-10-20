// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// Keys and initial values are initialized before atomic publication. Keys
// must remain alive even when a typed table replaces the associated value.
typedef struct LNK_NameTrieNode
{
  String8                 *name;
  void                    *value;
  struct LNK_NameTrieNode *child[4];
} LNK_NameTrieNode;

typedef struct LNK_NameTrieChunk
{
  struct LNK_NameTrieChunk *next;
  U64                      count;
  U64                      cap;
  LNK_NameTrieNode         *v;
} LNK_NameTrieChunk;

typedef struct LNK_NameTrieChunkList
{
  U64                count;
  LNK_NameTrieChunk *first;
  LNK_NameTrieChunk *last;
} LNK_NameTrieChunkList;

internal LNK_NameTrieNode *lnk_name_trie_find_or_insert(Arena *arena, LNK_NameTrieChunkList *chunks, LNK_NameTrieNode **root, U64 hash, String8 *name, void *value, B32 *inserted_out);
internal LNK_NameTrieNode *lnk_name_trie_search(LNK_NameTrieNode *root, U64 hash, String8 name);
// Removal is a phase-boundary operation; readers must have finished using
// the value. Preserve children so colliding keys remain reachable.
internal void lnk_name_trie_remove(LNK_NameTrieNode *node);
internal void lnk_name_trie_chunk_list_concat(LNK_NameTrieChunkList *list, LNK_NameTrieChunkList *to_concat);
internal LNK_NameTrieChunk **lnk_name_trie_chunk_array(Arena *arena, LNK_NameTrieChunkList *lists, U64 lists_count, U64 *count_out);
