// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global read_only String8 g_null_string;

internal String8
str8_cstring_capped_reverse(void *start, void *cap)
{
  char *ptr = cap;
  while (ptr > (char *)start) {
    --ptr;
    if (*ptr == '\0') break;
  }
  U64 null_offset = (U64)(ptr - (char *) start);
  String8 result = str8((U8 *) start, null_offset);
  return result;
}

internal U64
str8_find_needle_reverse(String8 string, U64 start_pos, String8 needle, StringMatchFlags flags)
{
  for (S64 i = string.size - start_pos - needle.size; i >= 0; --i) {
    String8 haystack = str8_substr(string, rng_1u64(i, i + needle.size));
    if (str8_match(haystack, needle, flags)) {
      return (U64)i + needle.size;
    }
  }
  return 0;
}

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

internal String8
str8_from_bits_u32(Arena *arena, U32 x)
{
  U8 c0 = 'a' + ((x >> 28) & 0xf);
  U8 c1 = 'a' + ((x >> 24) & 0xf);
  U8 c2 = 'a' + ((x >> 20) & 0xf);
  U8 c3 = 'a' + ((x >> 16) & 0xf);
  U8 c4 = 'a' + ((x >> 12) & 0xf);
  U8 c5 = 'a' + ((x >> 8) & 0xf);
  U8 c6 = 'a' + ((x >> 4) & 0xf);
  U8 c7 = 'a' + ((x >> 0) & 0xf);
  String8 result = push_str8f(arena, "%c%c%c%c%c%c%c%c", c0, c1, c2, c3, c4, c5, c6, c7);
  return result;
}

internal String8
str8_from_bits_u64(Arena *arena, U64 x)
{
  U8 c0 = 'a' + ((x >> 60) & 0xf);
  U8 c1 = 'a' + ((x >> 56) & 0xf);
  U8 c2 = 'a' + ((x >> 52) & 0xf);
  U8 c3 = 'a' + ((x >> 48) & 0xf);
  U8 c4 = 'a' + ((x >> 44) & 0xf);
  U8 c5 = 'a' + ((x >> 40) & 0xf);
  U8 c6 = 'a' + ((x >> 36) & 0xf);
  U8 c7 = 'a' + ((x >> 32) & 0xf);
  U8 c8 = 'a' + ((x >> 28) & 0xf);
  U8 c9 = 'a' + ((x >> 24) & 0xf);
  U8 ca = 'a' + ((x >> 20) & 0xf);
  U8 cb = 'a' + ((x >> 16) & 0xf);
  U8 cc = 'a' + ((x >> 12) & 0xf);
  U8 cd = 'a' + ((x >>  8) & 0xf);
  U8 ce = 'a' + ((x >>  4) & 0xf);
  U8 cf = 'a' + ((x >>  0) & 0xf);
  String8 result = push_str8f(arena,
                              "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                              c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);
  return result;
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

internal String8
str8_from_memory_size2(Arena *arena, U64 size)
{
  String8 result;
  if (size < KB(1)) {
    result = push_str8f(arena, "%llu Bytes", size);
  } else if (size < MB(1)) {
    result = push_str8f(arena, "%llu.%02llu KiB", size / KB(1), ((size * 100) / KB(1)) % 100);
  } else if (size < GB(1)) {
    result = push_str8f(arena, "%llu.%02llu MiB", size / MB(1), ((size * 100) / MB(1)) % 100);
  } else if (size < TB(1)) {
    result = push_str8f(arena, "%llu.%02llu GiB", size / GB(1), ((size * 100) / GB(1)) % 100);
  } else {
    result = push_str8f(arena, "%llu.%02llu TiB", size / TB(1), ((size * 100) / TB(1)) % 100);
  }
  return result;
}

internal String8
str8_from_count(Arena *arena, U64 count)
{
  String8 result;
  if (count < 1000) {
    result = push_str8f(arena, "%llu", count);
  } else if (count < 1000000) {
    U64 frac = ((count * 100) / 1000) % 100;
    if (frac) {
      result = push_str8f(arena, "%llu.%02lluK", count / 1000, frac);
    } else {
      result = push_str8f(arena, "%lluK", count / 1000);
    }
  } else if (count < 1000000000) {
    U64 frac = ((count * 100) / 1000000) % 100;
    if (frac) {
      result = push_str8f(arena, "%llu.%02lluM", count / 1000000, frac);
    } else {
      result = push_str8f(arena, "%lluM", count / 1000000);
    }
  } else {
    U64 frac = ((count * 100) * 1000000000) % 100;
    if (frac) {
      result = push_str8f(arena, "%llu.%02lluB", count / 1000000000, frac);
    } else {
      result = push_str8f(arena, "%lluB", count / 1000000000, frac);
    }
  }
  return result;
}

internal U64
hash_from_str8(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

