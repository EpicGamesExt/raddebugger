// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_LibNode *
lnk_lib_list_pop_node_atomic(LNK_LibList *list)
{
  for (;;) {
    LNK_LibNode *expected = list->first;
    LNK_LibNode *current  = ins_atomic_ptr_eval_cond_assign(&list->first, expected->next, expected);
    if (expected == current) { 
      ins_atomic_u64_dec_eval(&list->count);
      return expected;
    }
  }
}

internal void
lnk_lib_list_push_node_atomic(LNK_LibList *list, LNK_LibNode *node)
{
  for (;;) {
    LNK_LibNode *expected = list->first;
    LNK_LibNode *current  = ins_atomic_ptr_eval_cond_assign(&list->first, node, expected);
    if (current == expected) {
      node->next = expected;
      ins_atomic_u64_inc_eval(&list->count);
      return;
    }
  }
}

internal void
lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node)
{
  SLLStackPush(list->first, node);
  list->count += 1;
}

internal LNK_LibList
lnk_lib_list_reserve(Arena *arena, U64 count)
{
  LNK_LibList result = {0};
  LNK_LibNode *nodes = push_array(arena, LNK_LibNode, count);
  for (U64 i = 0; i < count; i += 1) { lnk_lib_list_push_node(&result, &nodes[i]); }
  return result;
}

internal LNK_LibNodeArray
lnk_array_from_lib_list(Arena *arena, LNK_LibList list)
{
  LNK_LibNodeArray result = {0};
  result.v = push_array(arena, LNK_LibNode, list.count);
  for (LNK_LibNode *n = list.first; n != 0; n = n->next) { result.v[result.count++] = *n; }
  return result;
}

internal B32
lnk_lib_from_data(Arena *arena, String8 data, String8 path, LNK_Lib *lib_out)
{
  ProfBeginFunction();

  // is data archive?
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  if (type == COFF_Archive_Null) {
    return 0;
  }

  COFF_ArchiveParse parse = coff_archive_parse_from_data(data);
  if (parse.error.size) {
    return 0;
  }

  U64     symbol_count;
  String8 string_table;
  U32    *member_off_arr;

  // try to init library from optional second member
  if (parse.second_member.member_count) {
    COFF_ArchiveSecondMember second_member = parse.second_member;
    Assert(second_member.symbol_count == second_member.symbol_index_count);
    Assert(second_member.member_count == second_member.member_offset_count);
    
    symbol_count   = second_member.symbol_count;
    string_table   = second_member.string_table;
    member_off_arr = push_array_no_zero(arena, U32, symbol_count);
    
    // decompress member offsets
    for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += 1) {
      U16 off_number = second_member.symbol_indices[symbol_idx];
      if (0 < off_number && off_number <= second_member.member_count) {
        member_off_arr[symbol_idx] = second_member.member_offsets[off_number - 1];
      } else {
        // TODO: log bad offset
        member_off_arr[symbol_idx] = max_U32;
      }
    }
  } 
  // first member is deprecated however tools emit it for compatibility reasons
  // and lld-link with /DLL emits only first member
  else if (parse.first_member.symbol_count) {
    COFF_ArchiveFirstMember first_member = parse.first_member;
    Assert(first_member.symbol_count == first_member.member_offset_count);
    
    symbol_count   = first_member.symbol_count;
    string_table   = first_member.string_table;
    member_off_arr = first_member.member_offsets;
    
    // convert big endian offsets
    for (U32 offset_idx = 0; offset_idx < symbol_count; offset_idx += 1) {
      member_off_arr[offset_idx] = from_be_u32(member_off_arr[offset_idx]);
    }
  } else {
    symbol_count   = 0;
    string_table   = str8_zero();
    member_off_arr = 0;
  }
  
  // parse string table
  String8List symbol_name_list = str8_split_by_string_chars(arena, string_table, str8_lit("\0"), StringSplitFlag_KeepEmpties);
  Assert(symbol_name_list.node_count >= symbol_count);
  symbol_count = Min(symbol_count, symbol_name_list.node_count);
  
  // init lib
  lib_out->path             = push_str8_copy(arena, path);
  lib_out->data             = data;
  lib_out->type             = type;
  lib_out->symbol_count     = symbol_count;
  lib_out->member_off_arr   = member_off_arr;
  lib_out->symbol_name_list = symbol_name_list;
  lib_out->long_names       = parse.long_names;
  
  ProfEnd();
  return 1;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lib_initer)
{
  LNK_LibIniter *task = raw_task;

  LNK_LibNode *lib_node = lnk_lib_list_pop_node_atomic(&task->free_libs);
  lib_node->data.input_idx = task_id;

  B32 is_valid_lib = lnk_lib_from_data(arena, task->data_arr[task_id], task->path_arr[task_id], &lib_node->data);
  if (is_valid_lib) {
    lnk_lib_list_push_node_atomic(&task->valid_libs, lib_node);
  } else {
    lnk_lib_list_push_node_atomic(&task->invalid_libs, lib_node);
  }
}

internal int
lnk_lib_node_is_before(void *a, void *b)
{
  return ((LNK_LibNode*)a)->data.input_idx < ((LNK_LibNode*)b)->data.input_idx;
}

internal LNK_LibNodeArray
lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr)
{
  Temp scratch = scratch_begin(arena->v, arena->count);

  Assert(data_arr.count == path_arr.count);
  U64 lib_count = data_arr.count;

  // parse libs in parallel
  LNK_LibIniter task = {0};
  task.free_libs     = lnk_lib_list_reserve(scratch.arena, lib_count);
  task.data_arr      = data_arr.v;
  task.path_arr      = path_arr.v;
  tp_for_parallel(tp, arena, lib_count, lnk_lib_initer, &task);

  // report invalid libs
  LNK_LibNodeArray invalid_libs = lnk_array_from_lib_list(scratch.arena, task.invalid_libs);
  radsort(invalid_libs.v, invalid_libs.count, lnk_lib_node_is_before);
  for (U64 i = 0; i < task.invalid_libs.count; i += 1) {
    U64 input_idx = invalid_libs.v[i].data.input_idx;
    lnk_error(LNK_Error_InvalidLib, "%S: failed to parse library", path_arr.v[input_idx]);
  }

  // push parsed libs
  LNK_LibNodeArray result = lnk_array_from_lib_list(arena->v[0], task.valid_libs);
  radsort(result.v, result.count, lnk_lib_node_is_before);
  for (U64 i = result.count; i > 0; i -= 1) {
    result.v[i-1].data.input_idx = list->count;
    lnk_lib_list_push_node(list, &result.v[i-1]);
  }

  scratch_end(scratch);
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_push_lib_symbols_task)
{
  LNK_SymbolPusher *task   = raw_task;
  LNK_SymbolTable  *symtab = task->symtab;
  LNK_Lib          *lib    = &task->u.libs.v[task_id].data;

  String8Node *name_node = lib->symbol_name_list.first;
  for (U64 symbol_idx = 0; symbol_idx < lib->symbol_count; ++symbol_idx, name_node = name_node->next) {
    LNK_Symbol *symbol = lnk_make_lib_symbol(arena, name_node->string, lib, lib->member_off_arr[symbol_idx]);
    lnk_symbol_table_push_(symtab, arena, worker_id, LNK_SymbolScope_Lib, symbol);
  }
}

internal void
lnk_input_lib_symbols(TP_Context *tp, LNK_SymbolTable *symtab, LNK_LibNodeArray libs)
{
  ProfBeginFunction();
  LNK_SymbolPusher task = {0};
  task.symtab           = symtab;
  task.u.libs           = libs;
  tp_for_parallel(tp, symtab->arena, libs.count, lnk_push_lib_symbols_task, &task);
  ProfEnd();
}

