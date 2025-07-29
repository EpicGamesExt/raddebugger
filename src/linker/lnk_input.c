// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_error_input_obj(LNK_ErrorCode code, LNK_InputObj *input, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  lnk_error_with_loc_fv(code, input->path, input->lib ? input->lib->path : str8_zero(), fmt, args);
  va_end(args);
}

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

internal LNK_InputObj **
lnk_thin_array_from_input_obj_list(Arena *arena, LNK_InputObjList list, U64 *count_out)
{
  for (LNK_InputObj *input = list.first; input != 0; input = input->next) {
    if (input->is_thin) { *count_out += 1; }
  }
  LNK_InputObj **thin_inputs = push_array(arena, LNK_InputObj *, *count_out);
  U64            input_idx   = 0;
  for (LNK_InputObj *input = list.first; input != 0; input = input->next) {
    if (input->is_thin) { thin_inputs[input_idx++] = input; }
  }
  return thin_inputs;
}

internal String8Array
lnk_path_array_from_input_obj_array(Arena *arena, LNK_InputObj **arr, U64 count)
{
  String8Array paths = {0};
  paths.count = count;
  paths.v     = push_array(arena, String8, count);
  for (U64 i = 0; i < count; i += 1) {
    paths.v[i] = arr[i]->path;
  }
  return paths;
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

internal LNK_InputImportNode *
lnk_input_import_list_push(Arena *arena, LNK_InputImportList *list)
{
  LNK_InputImportNode *node = push_array(arena, LNK_InputImportNode, 1);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node; 
}

internal void
lnk_input_import_list_concat_in_place(LNK_InputImportList *list, LNK_InputImportList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_InputImportNode **
lnk_input_import_arr_from_list(Arena *arena, LNK_InputImportList list)
{
  LNK_InputImportNode **result = push_array_no_zero(arena, LNK_InputImportNode *, list.count);
  U64 idx = 0;
  for (LNK_InputImportNode *node = list.first; node != 0; node = node->next) {
    Assert(idx < list.count);
    result[idx++] = node;
  }
  return result;
}

internal LNK_InputImportList
lnk_list_from_input_import_arr(LNK_InputImportNode **arr, U64 count)
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
  LNK_InputImport *a = *(LNK_InputImport **)raw_a;
  LNK_InputImport *b = *(LNK_InputImport **)raw_b;
  return a->input_idx < b->input_idx;
}

int
lnk_input_import_node_compar(const void *raw_a, const void *raw_b)
{
  LNK_InputImportNode * const *a = raw_a;
  LNK_InputImportNode * const *b = raw_b;
  return u64_compar(&(*a)->data.input_idx, &(*b)->data.input_idx);
}

