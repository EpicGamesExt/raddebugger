// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Lib
{
  String8          path;
  String8          data;
  COFF_ArchiveType type;
  U32              symbol_count;
  U32             *member_offsets;
  U16             *symbol_indices;
  B8              *was_member_queued;
  String8Array     symbol_names;
  String8          long_names;
  U64              input_idx;
} LNK_Lib;

typedef struct LNK_LibNode
{
  LNK_Lib             data;
  struct LNK_LibNode *next;
} LNK_LibNode;

typedef struct LNK_LibNodeArray
{
  U64          count;
  LNK_LibNode *v;
} LNK_LibNodeArray;

typedef struct LNK_LibList
{
  U64                 count;
  struct LNK_LibNode *first;
} LNK_LibList;
 
typedef struct
{
  String8     *data_arr;
  String8     *path_arr;
  LNK_LibList  free_libs;
  LNK_LibList  valid_libs;
  LNK_LibList  invalid_libs;
} LNK_LibIniter;

internal LNK_LibNode *    lnk_lib_list_pop_node_atomic(LNK_LibList *list);
internal void             lnk_lib_list_push_node_atomic(LNK_LibList *list, LNK_LibNode *node);
internal void             lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node);
internal LNK_LibList      lnk_lib_list_reserve(Arena *arena, U64 count);
internal LNK_LibNodeArray lnk_array_from_lib_list(Arena *arena, LNK_LibList list);

internal B32              lnk_lib_from_data(Arena *arena, String8 data, String8 path, LNK_Lib *lib_out);
internal LNK_LibNodeArray lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr);

