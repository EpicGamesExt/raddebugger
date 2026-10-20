// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_NameTrieNode *
lnk_name_trie_find_or_insert(Arena *arena, LNK_NameTrieChunkList *chunks, LNK_NameTrieNode **root,
                            U64 hash, String8 *name, void *value, B32 *inserted_out)
{
  *inserted_out = 0;
  LNK_NameTrieNode **slot = root;
  for (U64 h = hash; ; h <<= 2) {
    LNK_NameTrieNode *node = ins_atomic_ptr_eval(slot);
    if (node == 0) {
      // Each worker owns its chunk list; only tree publication is shared.
      if (chunks->last == 0 || chunks->last->count == chunks->last->cap) {
        LNK_NameTrieChunk *chunk = push_array(arena, LNK_NameTrieChunk, 1);
        chunk->cap = 0x1000;
        chunk->v = push_array_no_zero(arena, LNK_NameTrieNode, chunk->cap);
        SLLQueuePush(chunks->first, chunks->last, chunk);
        chunks->count += 1;
      }
      LNK_NameTrieNode *new_node = &chunks->last->v[chunks->last->count++];
      new_node->name  = name;
      new_node->value = value;
      MemoryZeroArray(new_node->child);

      LNK_NameTrieNode *other = ins_atomic_ptr_eval_cond_assign(slot, new_node, 0);
      if (other == 0) {
        *inserted_out = 1;
        return new_node;
      }
      chunks->last->count -= 1;
      node = other;
    }

    String8 *key = ins_atomic_ptr_eval(&node->name);
    if (key && str8_match(*key, *name, 0)) {
      return node;
    }
    slot = &node->child[h >> 62];
  }
}

internal LNK_NameTrieNode *
lnk_name_trie_search(LNK_NameTrieNode *root, U64 hash, String8 name)
{
  LNK_NameTrieNode **slot = &root;
  for (U64 h = hash; ; h <<= 2) {
    LNK_NameTrieNode *node = ins_atomic_ptr_eval(slot);
    if (node == 0) { return 0; }
    String8 *key = ins_atomic_ptr_eval(&node->name);
    if (key && str8_match(*key, name, 0)) { return node; }
    slot = &node->child[h >> 62];
  }
}

internal void
lnk_name_trie_remove(LNK_NameTrieNode *node)
{
  ins_atomic_ptr_eval_assign(&node->name, 0);
  ins_atomic_ptr_eval_assign(&node->value, 0);
}

internal void
lnk_name_trie_chunk_list_concat(LNK_NameTrieChunkList *list, LNK_NameTrieChunkList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_NameTrieChunk **
lnk_name_trie_chunk_array(Arena *arena, LNK_NameTrieChunkList *lists, U64 lists_count, U64 *count_out)
{
  U64 count = 0;
  for EachIndex(i, lists_count) { count += lists[i].count; }
  LNK_NameTrieChunk **chunks = push_array(arena, LNK_NameTrieChunk *, count);
  U64 cursor = 0;
  for EachIndex(i, lists_count) {
    for EachNode(chunk, LNK_NameTrieChunk, lists[i].first) {
      chunks[cursor++] = chunk;
    }
  }
  if (count_out) { *count_out = count; }
  return chunks;
}
