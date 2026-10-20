// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal int
lnk_lib_node_is_before(void *a, void *b)
{
  return ((LNK_LibNode*)a)->data.input_idx < ((LNK_LibNode*)b)->data.input_idx;
}

internal int
lnk_lib_node_ptr_is_before(void *raw_a, void *raw_b)
{
  return lnk_lib_node_is_before(*(LNK_Lib **)raw_a, *(LNK_Lib **)raw_b);
}

internal B32
lnk_first_member_sort_key_is_before(void *raw_a, void *raw_b)
{
  LNK_FirstMemberSortKey *a = raw_a, *b = raw_b;
  return str8_is_before_case_sensitive(&a->symbol_name, &b->symbol_name);
}

internal B32
lnk_lib_from_data(Arena *arena, String8 data, String8 path, U64 input_idx, B32 whole_archive, LNK_Lib *lib_out)
{
  Temp scratch = scratch_begin(&arena, 1);

  COFF_ArchiveType type = coff_archive_type_from_data(data);
  if (type == COFF_Archive_Null) { goto fail; } // TODO: report that input is not a library

  COFF_ArchiveParse parse = coff_archive_parse_from_data(data);
  if (parse.error.size) { goto fail; } // TODO: report parser errors

  // validate the selected index's array lengths before accessing them
  if (parse.has_second_header) {
    COFF_ArchiveSecondMember *second = &parse.second_member;
    if (second->symbol_count != second->symbol_index_count) { goto fail; } // TODO: report count mismatch on symbols and member indices
    if (second->member_count != second->member_offset_count) { goto fail; } // TODO: report count mismatch on members and member offsets
  } else if (parse.has_first_header) {
    COFF_ArchiveFirstMember *first = &parse.first_member;
    if (first->symbol_count != first->member_offset_count) { goto fail; } // TODO: report count mismatch on symbols and member offsets
  }

  U32      member_count        = 0;
  U32     *member_offsets      = 0;
  U32List  member_offsets_list = {0};
  HashMap  offset_to_index     = {0}; // byte offset -> one based member index

  if (whole_archive) {
    U64 cursor = type == COFF_Archive_Thin
      ? coff_thin_archive_member_iter_init(data)
      : coff_regular_archive_member_iter_init(data);
    
    while (cursor < data.size) {
      U64                member_offset    = cursor;
      COFF_ArchiveMember member           = {0};
      B32                is_member_parsed = type == COFF_Archive_Thin
                                              ? coff_thin_archive_member_iter_next(data, &cursor, &member)
                                              : coff_regular_archive_member_iter_next(data, &cursor, &member);

      if ( ! is_member_parsed) { goto fail; } // TODO: report corrupted archive

      if (cursor <= member_offset || cursor > data.size) { goto fail; } // TODO: report truncated archive

      // skip metadata
      if (str8_matchi(member.header.name, str8_lit("/")) ||
          str8_matchi(member.header.name, str8_lit("//"))) {
        continue;
      }

      if (member_offset > max_U32 || member_offsets_list.count >= max_U32) { goto fail; } // TODO: report archive exceeds supported offset or member count range

      U64 member_idx = member_offsets_list.count + 1;
      hash_map_push_u64_u64(scratch.arena, &offset_to_index, member_offset, member_idx);
      u32_list_push(scratch.arena, &member_offsets_list, (U32)member_offset);
    }

    member_count   = safe_cast_u32(member_offsets_list.count);
    member_offsets = u32_array_from_list(arena, &member_offsets_list).v;
  } else {
    // borrow the second header's offset table; do not inspect member bodies
    if (parse.has_second_header) {
      member_count   = parse.second_member.member_count;
      member_offsets = parse.second_member.member_offsets;
    } else if (parse.has_first_header) {
      COFF_ArchiveFirstMember *first = &parse.first_member;

      // the first header stores an offset per symbol. Share a member index
      // between symbols with the same offset without scanning the archive
      for EachIndex(i, first->symbol_count) {
        U32 offset = from_be_u32(memory_read32(first->member_offsets + i));

        if (hash_map_search_u64_u64(&offset_to_index, offset) == 0) {
          if (member_offsets_list.count >= max_U32) { goto fail; } // TODO: report archive exceeds supported member count range

          U64 member_index = member_offsets_list.count + 1;
          hash_map_push_u64_u64(scratch.arena, &offset_to_index, offset, member_index);
          u32_list_push(scratch.arena, &member_offsets_list, offset);
        }
      }

      member_count   = safe_cast_u32(member_offsets_list.count);
      member_offsets = u32_array_from_list(arena, &member_offsets_list).v;
    }
  }

  U64           symbol_count   = 0;
  U32          *symbol_indices = 0;
  String8       string_table   = {0};
  String8Array  symbol_names   = {0};
  B32           sort_symbols   = 0;

  if (parse.has_second_header) {
    COFF_ArchiveSecondMember *second = &parse.second_member;

    symbol_count   = second->symbol_count;
    symbol_indices = push_array(arena, U32, symbol_count);
    string_table   = second->string_table;

    for EachIndex(i, symbol_count) {
      U16 disk_index = memory_read16(second->symbol_indices + i);

      if (disk_index == 0 || disk_index > second->member_count) { goto fail; } // TODO: report out of bounds member offset index

      if (whole_archive) {
        U32  offset       = memory_read32(second->member_offsets + disk_index - 1);
        U64 *member_index = hash_map_search_u64_u64(&offset_to_index, offset);
        if (member_index == 0) { goto fail; } // TODO: report symbol offset does not identify a physical member
        symbol_indices[i] = safe_cast_u32(*member_index);
      } else {
        symbol_indices[i] = disk_index;
      }
    }
  } else if (parse.has_first_header) {
    COFF_ArchiveFirstMember *first = &parse.first_member;

    symbol_count   = first->symbol_count;
    symbol_indices = push_array(arena, U32, symbol_count);
    string_table   = first->string_table;
    sort_symbols   = 1;

    for EachIndex(i, symbol_count) {
      U32  offset       = from_be_u32(memory_read32(first->member_offsets + i));
      U64 *member_index = hash_map_search_u64_u64(&offset_to_index, offset);

      if (member_index == 0) { goto fail; } // TODO: report malformed member offset

      symbol_indices[i] = safe_cast_u32(*member_index);
    }
  }

  // read exactly the advertised names; do not include archive padding in
  // the searchable symbol array or discard empty entries before validation
  symbol_names.count = symbol_count;
  symbol_names.v     = push_array(arena, String8, symbol_count);

  U64 string_cursor = 0;
  for EachIndex(i, symbol_count) {
    U64 start = string_cursor;
    while (string_cursor < string_table.size &&
           string_table.str[string_cursor] != 0) {
      string_cursor += 1;
    }
    if (string_cursor == string_table.size) { goto fail; } // TODO: report missing symbol name or NUL terminator
    if (string_cursor == start) { goto fail; } // TODO: report empty symbol name

    symbol_names.v[i] = str8(string_table.str + start, string_cursor - start);
    string_cursor += 1;
  }

  // GNU-format index members can include trailing zero alignment bytes
  for (; string_cursor < string_table.size; string_cursor += 1) {
    if (string_table.str[string_cursor] != 0) { goto fail; } // TODO: report unexpected data after the symbol names
  }

  if (sort_symbols && symbol_count > 1) {
    LNK_FirstMemberSortKey *sort_keys = push_array_no_zero(scratch.arena, LNK_FirstMemberSortKey, symbol_count);
    for EachIndex(symbol_idx, symbol_count) {
      sort_keys[symbol_idx].symbol_name    = symbol_names.v[symbol_idx];
      sort_keys[symbol_idx].member_off_idx = symbol_indices[symbol_idx];
    }
    radsort(sort_keys, symbol_count, lnk_first_member_sort_key_is_before);

    for EachIndex(symbol_idx, symbol_count) {
      symbol_names.v[symbol_idx] = sort_keys[symbol_idx].symbol_name;
      symbol_indices[symbol_idx] = sort_keys[symbol_idx].member_off_idx;
    }
  }
  
  // fill out library record for the parsed library
  lib_out->path           = push_str8_copy(arena, path);
  lib_out->data           = data;
  lib_out->type           = type;
  lib_out->member_count   = member_count;
  lib_out->symbol_count   = safe_cast_u32(symbol_count);
  lib_out->member_offsets = member_offsets;
  lib_out->symbol_indices = symbol_indices;
  lib_out->symbol_names   = symbol_names;
  lib_out->long_names     = parse.long_names;
  lib_out->input_idx      = input_idx;
  
  scratch_end(scratch);
  return 1;

  fail:;
  scratch_end(scratch);
  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lib_initer)
{
  LNK_LibIniter *task          = raw_task;
  LNK_Input     *input         = task->inputs[task_id];
  LNK_Config    *config        = task->config;
  String8        lib_name      = str8_chop_last_dot(str8_skip_last_slash(input->path));
  B32            whole_archive = config->whole_archive_all || (hash_map_search_path_u64(&config->whole_archive_ht, lib_name) != 0);
  U64            lib_node_idx  = ins_atomic_u64_inc_eval(&task->next_free_lib_idx) - 1;
  LNK_LibNode   *lib_node      = &task->free_libs[lib_node_idx];
  B32            is_valid_lib  = lnk_lib_from_data(arena, input->data, input->path, task->lib_id_base + task_id, whole_archive, &lib_node->data);
  if (is_valid_lib) {
    U64 valid_lib_idx = ins_atomic_u64_inc_eval(&task->valid_libs_count) - 1;
    task->valid_libs[valid_lib_idx] = lib_node;
  } else {
    lib_node->data.path      = input->path;
    lib_node->data.input_idx = task->lib_id_base + task_id;

    U64 invalid_lib_idx = ins_atomic_u64_inc_eval(&task->invalid_libs_count) - 1;
    task->invalid_libs[invalid_lib_idx] = lib_node;
  }
}

internal LNK_Lib **
lnk_array_from_lib_list(Arena *arena, LNK_LibList list)
{
  LNK_Lib **arr = push_array_no_zero(arena, LNK_Lib *, list.count);
  U64 idx = 0;
  for (LNK_LibNode *node = list.first; node != 0; node = node->next, ++idx) {
    arr[idx] = &node->data;
  }
  return arr;
}

internal void
lnk_lib_list_push_node(LNK_LibList *list, LNK_LibNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal LNK_LibNodeArray
lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_LibList *list, U64 inputs_count, LNK_Input **inputs)
{
  Temp scratch = scratch_begin(arena->v, arena->count);

  // invoke wide library initializers
  LNK_LibIniter task = {0};
  task.config        = config;
  task.lib_id_base   = list->count;
  task.free_libs     = push_array(arena->v[0], LNK_LibNode, inputs_count);
  task.valid_libs    = push_array(scratch.arena, LNK_LibNode *, inputs_count);
  task.invalid_libs  = push_array(scratch.arena, LNK_LibNode *, inputs_count);
  task.inputs        = inputs;
  tp_for_parallel(tp, arena, inputs_count, lnk_lib_initer, &task);

  // report invalid libs
  radsort(task.invalid_libs, task.invalid_libs_count, lnk_lib_node_ptr_is_before);
  for EachIndex(i, task.invalid_libs_count) {
    lnk_error(LNK_Error_InvalidLib, "%S: failed to parse library", task.invalid_libs[i]->data.path); // TODO: report fail reason
  }
  if (task.invalid_libs_count) {
    lnk_exit(LNK_Error_InvalidLib);
  }

  // push parsed libs
  radsort(task.valid_libs, task.valid_libs_count, lnk_lib_node_ptr_is_before);
  for EachIndex(i, task.valid_libs_count) { lnk_lib_list_push_node(list, task.valid_libs[i]); }

  LNK_LibNodeArray result = { .count = task.valid_libs_count, task.valid_libs };

  scratch_end(scratch);
  return result;
}

internal force_inline B32
lnk_search_lib(LNK_Lib *lib, String8 symbol_name, U32 *member_idx_out)
{
  U64 symbol_idx = str8_array_bsearch(lib->symbol_names, symbol_name);
  if (symbol_idx < lib->symbol_count) {
    if (member_idx_out) {
      *member_idx_out = lib->symbol_indices[symbol_idx]-1;
    }
    return 1;
  }
  return 0;
}

