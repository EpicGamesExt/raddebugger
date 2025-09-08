// Copyright (c) Epic Games Tools
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

internal U64
str8_array_bsearch(String8Array arr, String8 value)
{
  if (arr.count > 1) {
    int lo_compar = str8_compar_case_sensitive(&value, &arr.v[0]);
    if (lo_compar == 0) {
      return 0;
    }

    int hi_compar = str8_compar_case_sensitive(&value, &arr.v[arr.count-1]);
    if (hi_compar == 0){ 
      return arr.count-1;
    }

    if (lo_compar > 0 && hi_compar < 0) {
      for (U64 l = 0, r = arr.count -1; l <= r; ) {
        U64 m = l + (r- l) / 2;
        int cmp = str8_compar_case_sensitive(&arr.v[m], &value);
        if (cmp == 0) {
          return m;
        } else if (cmp < 0) {
          l = m + 1;
        } else {
          r = m - 1;
        }
      }
    }
  } else if (arr.count == 1 && str8_match(arr.v[0], value, 0)) {
    return 0;
  }
  return max_U64;
}
