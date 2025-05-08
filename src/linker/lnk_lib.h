// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Lib
{
  String8          path;
  String8          data;
  COFF_ArchiveType type;
  U32              symbol_count;
  U32 *            member_off_arr;
  String8List      symbol_name_list;
  String8          long_names;
  U64              input_idx;
} LNK_Lib;

typedef struct LNK_LibNode
{
  struct LNK_LibNode *next;
  LNK_Lib             data;
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
  struct LNK_LibNode *last;
} LNK_LibList;

////////////////////////////////

typedef struct
{
  LNK_LibNode     *node_arr;
  String8         *data_arr;
  String8         *path_arr;
  U64              base_input_idx;
} LNK_LibIniter;

////////////////////////////////

internal LNK_Lib          lnk_lib_from_data(Arena *arena, String8 data, String8 path);
internal LNK_LibNodeArray lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr);

