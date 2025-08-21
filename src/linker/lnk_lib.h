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
  U64           count;
  LNK_LibNode **v;
} LNK_LibNodeArray;

typedef struct LNK_LibList
{
  U64          count;
  LNK_LibNode *first;
  LNK_LibNode *last;
} LNK_LibList;

// --- Workers Contexts --------------------------------------------------------
 
typedef struct
{
  String8      *data_arr;
  String8      *path_arr;
  U64           next_free_lib_idx;
  U64           valid_libs_count;
  U64           invalid_libs_count;
  LNK_LibNode  *free_libs;
  LNK_LibNode **valid_libs;
  LNK_LibNode **invalid_libs;
} LNK_LibIniter;

// -----------------------------------------------------------------------------

internal int lnk_lib_node_is_before(void *a, void *b);
internal int lnk_lib_node_ptr_is_before(void *raw_a, void *raw_b);

internal B32              lnk_lib_from_data(Arena *arena, String8 data, String8 path, LNK_Lib *lib_out);
internal void             lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node);
internal LNK_LibNodeArray lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr);

internal B32 lnk_search_lib(LNK_Lib *lib, String8 symbol_name, U32 *member_idx_out);

