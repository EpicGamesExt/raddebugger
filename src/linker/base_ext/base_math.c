// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
rng_1u64_list_push_node(Rng1U64List *list, Rng1U64Node *node)
{
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
}

internal Rng1U64Node *
rng_1u64_list_push(Arena *arena, Rng1U64List *list, Rng1U64 range)
{
  Rng1U64Node *node = push_array(arena, Rng1U64Node, 1);
  node->v = range;
  rng_1u64_list_push_node(list, node);
  return node;
}

