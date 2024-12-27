// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct U64Node
{
  struct U64Node *next;
  U64             data;
} U64Node;

typedef struct U64List
{
  U64      count;
  U64Node *first;
  U64Node *last;
} U64List;

typedef struct VoidNode
{
  struct VoidNode *next;
  void            *v;
} VoidNode;

////////////////////////////////

internal U64Node * u64_list_push(Arena *arena, U64List *list, U64 data);
internal void      u64_list_concat_in_place(U64List *list, U64List *to_concat);
internal U64Array  u64_array_from_list(Arena *arena, U64List *list);

internal U64Array u64_array_remove_duplicates(Arena *arena, U64Array in);

internal void u32_array_sort(U64 count, U32 *v);
internal void u64_array_sort(U64 count, U64 *v);
internal B32  u32_array_compare(U32Array a, U32Array b);

internal U64 sum_array_u64(U64 count, U64 *v);
internal U64 max_array_u64(U64 count, U64 *v);
internal U64 min_array_u64(U64 count, U64 *v);

internal void  counts_to_offsets_array_u32(U64 count, U32 *arr);
internal void  counts_to_offsets_array_u64(U64 count, U64 *arr);

internal U32 * offsets_from_counts_array_u32(Arena *arena, U32 *v, U64 count);
internal U64 * offsets_from_counts_array_u64(Arena *arena, U64 *v, U64 count);

