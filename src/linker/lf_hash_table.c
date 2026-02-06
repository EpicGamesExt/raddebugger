// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
lfht_insert(Arena                *arena,
            LFHT_NodeChunkList   *node_chunks,
            LFHT_Node           **root_ptr,
            U64                   hash,
            void                 *data,
            LFHT_IsKeyEqualFunc  *is_key_equal,
            void                 *is_key_equal_ud)
{
  B32 was_inserted = 0;
  LFHT_Node **curr_ptr = root_ptr;
  for (U64 h = hash; ; h <<= 2) {
    LFHT_Node *curr = ins_atomic_ptr_eval(curr_ptr);

    if (curr == 0) {
      if (node_chunks->last == 0 || node_chunks->last->count >= node_chunks->last->max) {
        LFHT_NodeChunk *c = push_array(arena, LFHT_NodeChunk, 1);
        c->max   = 1024;
        c->count = 0;
        c->v     = push_array_no_zero(arena, LFHT_Node, 1024);

        SLLQueuePush(node_chunks->first, node_chunks->last, c);
        node_chunks->chunk_count += 1;
      }

      LFHT_Node *n = &node_chunks->last->v[node_chunks->last->count];
      MemoryZeroStruct(n);
      n->data = data;

      LFHT_Node *cmp = ins_atomic_ptr_eval_cond_assign(curr_ptr, n, curr);
      if (cmp == curr) {
        node_chunks->last->count += 1;
        node_chunks->total_count += 1;
        was_inserted = 1;
        break;
      }
    }

    // was the key already inserted?
    if (is_key_equal(is_key_equal_ud, curr->data, data)) { break; }

    // descend
    curr_ptr = &curr->children[h >> 62];
  }
  return was_inserted;
}

internal void *
lfht_search(LFHT_Node *root, U64 hash, void *data, LFHT_IsKeyEqualFunc *is_key_equal, void *is_key_equal_ud)
{
  LFHT_Node *curr = root;
  for (U64 h = hash; ; h <<= 2) {
    if (curr == 0) { break; }
    if (is_key_equal(is_key_equal_ud, curr->data, data)) { break; }
    curr = curr->children[h >> 62];
  }
  return curr ? curr->data : 0;
}

internal U64
lfht_total_count_from_node_chunk_lists(U64 list_count, LFHT_NodeChunkList *lists)
{
  U64 total_count = 0;
  for EachIndex(i, list_count) { total_count += lists[i].total_count; }
  return total_count;
}

internal void **
lfht_data_from_node_chunk_lists(Arena *arena, U64 total_count, U64 list_count, LFHT_NodeChunkList *lists)
{
  U64    result_count = 0;
  void **result       = push_array(arena, void *, total_count);
  for EachIndex(list_idx, list_count) {
    LFHT_NodeChunkList l = lists[list_idx];
    for EachNode(c, LFHT_NodeChunk, l.first) {
      Assert(result_count + c->count <= total_count);
      for EachIndex(item_idx, c->count) {
        result[result_count++] = c->v[item_idx].data;
      }
    }
  }

  return result;
}


