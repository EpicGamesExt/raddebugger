// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_LibNode *
lnk_lib_list_reserve(Arena *arena, LNK_LibList *list, U64 count)
{
  LNK_LibNode *arr = 0;
  if (count) {
    arr = push_array(arena, LNK_LibNode, count);
    for (LNK_LibNode *ptr = arr, *opl = arr + count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += count;
  }
  return arr;
}

internal LNK_Lib
lnk_lib_from_data(Arena *arena, String8 data, String8 path)
{
  ProfBeginFunction();

  U64     symbol_count;
  String8 string_table;
  U32    *member_off_arr;

  // is data archive?
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  if (type == COFF_Archive_Null) {
    lnk_not_implemented("TODO: data is not archive");
  }

  COFF_ArchiveParse parse = coff_archive_parse_from_data(data);

  // report archive parser errors
  if (parse.error.size) {
    lnk_error(LNK_Error_IllData, "%S: %S", path, parse.error);
  }
  
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
  LNK_Lib lib = {0};
  lib.path             = push_str8_copy(arena, path);
  lib.data             = data;
  lib.type             = type;
  lib.symbol_count     = symbol_count;
  lib.member_off_arr   = member_off_arr;
  lib.symbol_name_list = symbol_name_list;
  lib.long_names       = parse.long_names;
  
  ProfEnd();
  return lib;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lib_initer)
{
  LNK_LibIniter *task       = raw_task;
  LNK_LibNode   *lib_node   = task->node_arr + task_id;
  LNK_Lib       *lib        = &lib_node->data;
  String8        data       = task->data_arr[task_id];
  String8        path       = task->path_arr[task_id];
  
  *lib = lnk_lib_from_data(arena, data, path);
  lib->input_idx = task->base_input_idx + task_id;
}

internal LNK_LibNodeArray
lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr)
{
  Assert(data_arr.count == path_arr.count);
  U64 lib_count = data_arr.count;
  
  LNK_LibIniter task  = {0};
  task.node_arr       = lnk_lib_list_reserve(arena->v[0], list, lib_count);
  task.data_arr       = data_arr.v;
  task.path_arr       = path_arr.v;
  task.base_input_idx = list->count;
  tp_for_parallel(tp, arena, lib_count, lnk_lib_initer, &task);

  LNK_LibNodeArray arr = {0};
  arr.count            = lib_count;
  arr.v                = task.node_arr;
  return arr;
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
    U64 hash = lnk_symbol_hash(symbol->name);
    lnk_symbol_table_push_(symtab, arena, worker_id, LNK_SymbolScope_Lib, hash, symbol);
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

