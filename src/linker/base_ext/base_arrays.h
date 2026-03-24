// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct VoidNode { void *v;  struct VoidNode *next; } VoidNode;
typedef struct U32Node  { U32 data; struct U32Node  *next; } U32Node;
typedef struct U64Node  { U64 data; struct U64Node  *next; } U64Node;
typedef struct S64Node  { S64 v;    struct S64Node  *next; } S64Node;

typedef struct VoidList { U64 count; VoidNode *first, *last; } VoidList;
typedef struct U64List  { U64 count; U64Node  *first, *last; } U64List;
typedef struct S64List  { U64 count; S64Node  *first, *last; } S64List;

typedef struct S64Array { U64 count; S64 *v; } S64Array;

////////////////////////////////

internal U64  void_list_count_nodes  (VoidNode *head);
internal void void_node_concat       (VoidNode **head, VoidNode *node);
internal void void_node_concat_atomic(VoidNode **head, VoidNode *node);

internal void      u64_list_push_node      (U64List *list, U64Node *n);
internal U64Node * u64_list_push           (Arena *arena, U64List *list, U64 v);
internal void      u64_list_concat_in_place(U64List *list, U64List *to_concat);

internal B32   u32_array_compare            (U32Array a, U32Array b);
internal U32 * u32_array_offsets_from_counts(Arena *arena, U32 *v, U64 count);
internal void  u32_array_counts_to_offsets  (U64 count, U32 *arr);
internal void  u32_array_sort               (U64 count, U32 *v);
internal void  u32_pair_sort_radix          (U64 count, PairU32 *arr);

internal U64 *    offsets_from_counts_array_u64(Arena *arena, U64 *v, U64 count);
internal void     u64_array_counts_to_offsets  (U64 count, U64 *arr);
internal void     u64_array_sort               (U64 count, U64 *v);
internal U64      u64_array_max                (U64 count, U64 *v);
internal U64      u64_array_min                (U64 count, U64 *v);
internal U64      sum_array_u64                (U64 count, U64 *v);
internal U64      sum_array_u64                (U64 count, U64 *v);
internal U64Array u64_array_remove_duplicates  (Arena *arena, U64Array in);
internal U64Array u64_array_from_list          (Arena *arena, U64List *list);

internal void      s64_list_push_node      (S64List *list, S64Node *n);
internal S64Node * s64_list_push           (Arena *arena, S64List *list, S64 v);
internal void      s64_list_concat_in_place(S64List *list, S64List *to_concat);
internal S64Array  s64_array_from_list     (Arena *arena, S64List *list);

