// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal int
str8_compar(String8 a, String8 b, B32 ignore_case)
{
  int cmp = 0;
  U64 size = Min(a.size, b.size);
  if (ignore_case) {
    for (U64 i = 0; i < size; ++i) {
      U8 la = char_to_lower(a.str[i]);
      U8 lb = char_to_lower(b.str[i]);
      if (la < lb) {
        cmp = -1;
        break;
      } else if (la > lb) {
        cmp = +1;
        break;
      }
    } 
  } else {
    for (U64 i = 0; i < size; ++i) {
      if (a.str[i] < b.str[i]) {
        cmp = -1;
        break;
      } else if (a.str[i] > b.str[i]) {
        cmp = +1;
        break;
      }
    } 
  }
  
  if (cmp == 0) {
    // shorter prefix must precede longer prefixes
    if (a.size > b.size) {
      cmp = +1;
    } else if (b.size > a.size) {
      cmp = -1;
    }
  }
  
  return cmp;
}

internal int
str8_compar_ignore_case(const void *a, const void *b)
{
  return str8_compar(*(String8*)a, *(String8*)b, 1);
}

internal int
str8_compar_case_sensitive(const void *a, const void *b)
{
  return str8_compar(*(String8*)a, *(String8*)b, 0);
}

internal int
str8_is_before_case_sensitive(const void *a, const void *b)
{
  int cmp = str8_compar_case_sensitive(a, b);
  return cmp < 0;
}

internal String8Node *
str8_list_push_raw(Arena *arena, String8List *list, void *data_ptr, U64 data_size)
{
  String8 data = str8((U8 *)data_ptr, data_size);
  String8Node *node = str8_list_push(arena, list, data);
  return node;
}

internal U64
str8_list_push_pad(Arena *arena, String8List *list, U64 offset, U64 align)
{
  U64 pad_size = AlignPow2(offset, align) - offset;
  U8 *pad = push_array(arena, U8, pad_size);
  MemorySet(pad, 0, pad_size);
  str8_list_push(arena, list, str8(pad, pad_size));
  return pad_size;
}

internal U64
str8_list_push_pad_front(Arena *arena, String8List *list, U64 offset, U64 align)
{
  U64 pad_size = AlignPow2(offset, align) - offset;
  U8 *pad = push_array(arena, U8, pad_size);
  MemorySet(pad, 0, pad_size);
  str8_list_push_front(arena, list, str8(pad, pad_size));
  return pad_size;
}

internal String8List
str8_list_arr_concat(String8List *v, U64 count)
{
  String8List result = {0};
  for (U64 i = 0; i < count; i += 1) {
    str8_list_concat_in_place(&result, &v[i]);
  }
  return result;
}

internal String8Node *
str8_list_push_many(Arena *arena, String8List *list, U64 count)
{
  String8Node *arr = push_array(arena, String8Node, count);
  for (U64 i = 0; i < count; ++i) {
    str8_list_push_node(list, arr + i);
  }
  return arr;
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
hash_from_str8(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

