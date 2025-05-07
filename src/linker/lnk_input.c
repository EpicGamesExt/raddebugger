// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
lnk_string_from_input_source(LNK_InputSourceType input_source)
{
  String8 result = str8_zero();
  switch (input_source) {
  case LNK_InputSource_CmdLine: result = str8_lit("CmdLine"); break;
  case LNK_InputSource_Default: result = str8_lit("Default"); break;
  case LNK_InputSource_Obj:     result = str8_lit("Obj");     break;
  default:                      InvalidPath;
  }
  return result;
}

internal void
lnk_input_obj_list_push_node(LNK_InputObjList *list, LNK_InputObj *node)
{
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
}

internal LNK_InputObj *
lnk_input_obj_list_push(Arena *arena, LNK_InputObjList *list)
{
  LNK_InputObj *node = push_array(arena, LNK_InputObj, 1);
  lnk_input_obj_list_push_node(list, node);
  return node;
}

internal void
lnk_input_obj_list_concat_in_place(LNK_InputObjList *list, LNK_InputObjList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_InputObj **
lnk_array_from_input_obj_list(Arena *arena, LNK_InputObjList list)
{
  LNK_InputObj **result = push_array_no_zero(arena, LNK_InputObj *, list.count);
  U64 i = 0;
  for (LNK_InputObj *n = list.first; n != 0; n = n->next, ++i) {
    Assert(i < list.count);
    result[i] = n;
  }
  return result;
}

internal int
lnk_input_obj_compar(const void *raw_a, const void *raw_b)
{
  const LNK_InputObj **a = (const LNK_InputObj **) raw_a;
  const LNK_InputObj **b = (const LNK_InputObj **) raw_b;
  int cmp = str8_compar_case_sensitive(&(*a)->path, &(*b)->path);
  return cmp;
}

internal int
lnk_input_obj_compar_is_before(void *raw_a, void *raw_b)
{
  LNK_InputObj **a = raw_a;
  LNK_InputObj **b = raw_b;
  int cmp = str8_compar_case_sensitive(&(*a)->path, &(*b)->path);
  int is_before = cmp < 0;
  return is_before;
}

internal LNK_InputObjList
lnk_list_from_input_obj_arr(LNK_InputObj **arr, U64 count)
{
  LNK_InputObjList list = {0};
  for (U64 i = 0; i < count; ++i) {
    SLLQueuePush(list.first, list.last, arr[i]);
    ++list.count;
  }
  return list;
}

internal LNK_InputObjList
lnk_input_obj_list_from_string_list(Arena *arena, String8List list)
{
  LNK_InputObjList input_list = {0};
  for (String8Node *path = list.first; path != 0; path = path->next) {
    LNK_InputObj *input = lnk_input_obj_list_push(arena, &input_list);
    input->is_thin  = 1;
    input->dedup_id = path->string;
    input->path     = path->string;
  }
  return input_list;
}

internal LNK_InputImport *
lnk_input_import_list_push(Arena *arena, LNK_InputImportList *list)
{
  LNK_InputImport *node = push_array(arena, LNK_InputImport, 1);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node; 
}

internal void
lnk_input_import_list_concat_in_place(LNK_InputImportList *list, LNK_InputImportList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_InputImport **
lnk_input_import_arr_from_list(Arena *arena, LNK_InputImportList list)
{
  LNK_InputImport **result = push_array_no_zero(arena, LNK_InputImport *, list.count);
  U64 idx = 0;
  for (LNK_InputImport *node = list.first; node != 0; node = node->next) {
    Assert(idx < list.count);
    result[idx++] = node;
  }
  return result;
}

internal LNK_InputImportList
lnk_list_from_input_import_arr(LNK_InputImport **arr, U64 count)
{
  LNK_InputImportList list = {0};
  for (U64 i = 0; i < count; i += 1) {
    SLLQueuePush(list.first, list.last, arr[i]);
    list.count += 1;
  }
  return list;
}

int
lnk_input_import_is_before(void *raw_a, void *raw_b)
{
  LNK_InputImport **a = raw_a;
  LNK_InputImport **b = raw_b;
  int cmp = str8_compar_ignore_case(&(*a)->import_header.dll_name, &(*b)->import_header.dll_name);
  if (cmp == 0) {
    cmp = str8_compar_case_sensitive(&(*a)->import_header.func_name, &(*b)->import_header.func_name);
  }
  return cmp < 0;
}

int
lnk_input_import_compar(const void *raw_a, const void *raw_b)
{
  LNK_InputImport * const *a = raw_a;
  LNK_InputImport * const *b = raw_b;
  int cmp = str8_compar_ignore_case(&(*a)->import_header.dll_name, &(*b)->import_header.dll_name);
  if (cmp == 0) {
    cmp = str8_compar_case_sensitive(&(*a)->import_header.func_name, &(*b)->import_header.func_name);
  }
  return cmp;
}

