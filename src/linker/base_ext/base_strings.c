// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
str8_starts_with(String8 string, String8 expected_prefix)
{
  return str8_match(str8_prefix(string, expected_prefix.size), expected_prefix, 0);
}

internal String8Node *
str8_list_pop_front(String8List *list)
{
  String8Node *node = 0;
  if (list->node_count) {
	node = list->first;
    Assert(list->total_size >= list->first->string.size);
    list->node_count -= 1;
    list->total_size -= list->first->string.size;
    SLLQueuePop(list->first, list->last);
  }
  return node;
}

