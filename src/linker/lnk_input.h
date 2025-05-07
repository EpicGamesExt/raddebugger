// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum
{
  LNK_InputSource_CmdLine, // specified on command line
  LNK_InputSource_Default, // specified through defaultlib switch
  LNK_InputSource_Obj,     // refrenced from objects
  LNK_InputSource_Count
} LNK_InputSourceType;

typedef String8Node LNK_InputLib;
typedef String8List LNK_InputLibList;

typedef struct LNK_InputImport
{
  COFF_ParsedArchiveImportHeader       import_header;
  struct LNK_InputImport *next;
} LNK_InputImport;

typedef struct LNK_InputImportList
{
  U64              count;
  LNK_InputImport *first;
  LNK_InputImport *last;
} LNK_InputImportList;

typedef struct LNK_InputObj
{
  struct LNK_InputObj *next;
  B32                  is_thin;
  B32                  has_disk_read_failed;
  String8              dedup_id;
  String8              path;
  String8              data;
  String8              lib_path;
} LNK_InputObj;

typedef struct LNK_InputObjList
{
  U64           count;
  LNK_InputObj *first;
  LNK_InputObj *last;
} LNK_InputObjList;

////////////////////////////////

internal String8 lnk_string_from_input_source(LNK_InputSourceType input_source);

internal void             lnk_input_obj_list_push_node(LNK_InputObjList *list, LNK_InputObj *node);
internal LNK_InputObj *   lnk_input_obj_list_push(Arena *arena, LNK_InputObjList *list);
internal void             lnk_input_obj_list_concat_in_place(LNK_InputObjList *list, LNK_InputObjList *to_concat);

internal LNK_InputObj **  lnk_array_from_input_obj_list(Arena *arena, LNK_InputObjList list);
internal LNK_InputObjList lnk_list_from_input_obj_arr(LNK_InputObj **arr, U64 count);
internal LNK_InputObjList lnk_input_obj_list_from_string_list(Arena *arena, String8List list);

internal LNK_InputImport *   lnk_input_import_list_push(Arena *arena, LNK_InputImportList *list);
internal void                lnk_input_import_list_concat_in_place(LNK_InputImportList *list, LNK_InputImportList *to_concat);
internal LNK_InputImport **  lnk_input_import_arr_from_list(Arena *arena, LNK_InputImportList list);
internal LNK_InputImportList lnk_list_from_input_import_arr(LNK_InputImport **arr, U64 count);

