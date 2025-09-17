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
lnk_lib_from_data(Arena *arena, String8 data, String8 path, U64 input_idx, LNK_Lib *lib_out)
{
  // is data archive?
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  if (type == COFF_Archive_Null) {
    return 0;
  }

  // TODO: report parse errors
  COFF_ArchiveParse parse = coff_archive_parse_from_data(data);
  if (parse.error.size) {
    return 0;
  }

  U32           member_count   = 0;
  U64           symbol_count   = 0;
  String8Array  symbol_names   = {0};
  U16          *symbol_indices = 0;
  U32          *member_offsets = 0;

  // try to init library from optional second member
  if (parse.second_member.member_count) {
    COFF_ArchiveSecondMember second_member = parse.second_member;
    Assert(second_member.symbol_count == second_member.symbol_index_count);
    Assert(second_member.member_count == second_member.member_offset_count);
    
    member_count   = second_member.member_count;
    symbol_count   = second_member.symbol_count;
    member_offsets = second_member.member_offsets;
    symbol_indices = second_member.symbol_indices;

    // parse symbol names
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8List symbol_name_list = str8_split_by_string_chars(scratch.arena, second_member.string_table, str8_lit("\0"), 0);
      Assert(symbol_name_list.node_count >= symbol_count);
      symbol_names = str8_array_from_list(arena, &symbol_name_list);
      scratch_end(scratch);
    }
  } 
  // first member is deprecated however tools emit it for compatibility reasons
  // and lld-link with /DLL emits only first member
  else if (parse.first_member.symbol_count) {
    Temp scratch = scratch_begin(&arena, 1);

    COFF_ArchiveFirstMember first_member = parse.first_member;
    Assert(first_member.symbol_count == first_member.member_offset_count);
    
    symbol_count = first_member.symbol_count;
    
    // convert big endian offsets
    for (U32 offset_idx = 0; offset_idx < symbol_count; offset_idx += 1) {
      first_member.member_offsets[offset_idx] = from_be_u32(first_member.member_offsets[offset_idx]);
    }

    // compress member offsets to match those from the second header
    {
      HashTable *member_off_ht = hash_table_init(scratch.arena, (U64)((F64)first_member.symbol_count * 1.3));
      for EachIndex(symbol_idx, symbol_count) {
        if (!hash_table_search_u32_u32(member_off_ht, first_member.member_offsets[symbol_idx], 0)) {
          hash_table_push_u32_u32(scratch.arena, member_off_ht, first_member.member_offsets[symbol_idx], member_off_ht->count);
        }
      }

      symbol_indices = push_array(arena, U16, first_member.symbol_count);
      for EachIndex(symbol_idx, first_member.symbol_count) {
        U32 member_off = first_member.member_offsets[symbol_idx];
        U32 member_off_idx = 0;
        if (!hash_table_search_u32_u32(member_off_ht, member_off, &member_off_idx)) {
          InvalidPath;
        }
        symbol_indices[symbol_idx] = member_off_idx+1;
      }

      member_count   = member_off_ht->count;
      member_offsets = push_array_no_zero(arena, U32, member_count);

      for EachIndex(bucket_idx, member_off_ht->cap) {
        BucketList *bucket = &member_off_ht->buckets[bucket_idx];
        for (BucketNode *n = bucket->first; n != 0; n = n->next) {
          U32 member_off     = n->v.key_u32;
          U32 member_off_idx = n->v.value_u32;
          member_offsets[member_off_idx] = member_off;
        }
      }
      
      // parse symbol names
      {
        Temp scratch = scratch_begin(&arena, 1);
        String8List symbol_name_list = str8_split_by_string_chars(scratch.arena, first_member.string_table, str8_lit("\0"), 0);
        Assert(symbol_name_list.node_count >= first_member.symbol_count);
        symbol_names = str8_array_from_list(arena, &symbol_name_list);
        scratch_end(scratch);
      }

      // sort lexically symbol names
      LNK_FirstMemberSortKey *sort_keys = push_array_no_zero(scratch.arena, LNK_FirstMemberSortKey, first_member.symbol_count);
      for EachIndex(symbol_idx, first_member.symbol_count) {
        sort_keys[symbol_idx].symbol_name    = symbol_names.v[symbol_idx];
        sort_keys[symbol_idx].member_off_idx = symbol_indices[symbol_idx];
      }
      radsort(sort_keys, first_member.symbol_count, lnk_first_member_sort_key_is_before);

      for EachIndex(symbol_idx, first_member.symbol_count) {
        symbol_names.v[symbol_idx] = sort_keys[symbol_idx].symbol_name;
        symbol_indices[symbol_idx] = sort_keys[symbol_idx].member_off_idx;
      }
    }

    scratch_end(scratch);
  }
  
  // init lib
  lib_out->path              = push_str8_copy(arena, path);
  lib_out->data              = data;
  lib_out->type              = type;
  lib_out->member_count      = member_count;
  lib_out->symbol_count      = Min(symbol_count, symbol_names.count); // TODO: warn about mismatched number of symbol names and symbol count in the header
  lib_out->member_offsets    = member_offsets;
  lib_out->symbol_indices    = symbol_indices;
  lib_out->member_links      = push_array(arena, LNK_Symbol *, member_count);
  lib_out->symbol_names      = symbol_names;
  lib_out->long_names        = parse.long_names;
  lib_out->input_idx         = input_idx;
  
  return 1;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lib_initer)
{
  LNK_LibIniter *task  = raw_task;
  LNK_Input     *input = task->inputs[task_id];

  U64          lib_node_idx = ins_atomic_u64_inc_eval(&task->next_free_lib_idx)-1;
  LNK_LibNode *lib_node     = &task->free_libs[lib_node_idx];

  B32 is_valid_lib = lnk_lib_from_data(arena, input->data, input->path, task->lib_id_base + task_id, &lib_node->data);
  if (is_valid_lib) {
    U64 valid_lib_idx = ins_atomic_u64_inc_eval(&task->valid_libs_count)-1;
    task->valid_libs[valid_lib_idx] = lib_node;
  } else {
    U64 invalid_lib_idx = ins_atomic_u64_inc_eval(&task->invalid_libs_count);
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
lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, U64 inputs_count, LNK_Input **inputs)
{
  Temp scratch = scratch_begin(arena->v, arena->count);

  U64 lib_id_base = list->count;

  // parse libs in parallel
  LNK_LibIniter task = {0};
  task.lib_id_base   = list->count;
  task.free_libs     = push_array(arena->v[0], LNK_LibNode, inputs_count);
  task.valid_libs    = push_array(scratch.arena, LNK_LibNode *, inputs_count);
  task.invalid_libs  = push_array(scratch.arena, LNK_LibNode *, inputs_count);
  task.inputs        = inputs;
  tp_for_parallel(tp, arena, inputs_count, lnk_lib_initer, &task);

  // report invalid libs
  radsort(task.invalid_libs, task.invalid_libs_count, lnk_lib_node_ptr_is_before);
  for EachIndex(i, task.invalid_libs_count) {
    U64 input_idx = task.invalid_libs[i]->data.input_idx;
    lnk_error(LNK_Error_InvalidLib, "%S: failed to parse library", inputs[input_idx]->path);
  }

  // push parsed libs
  radsort(task.valid_libs, task.valid_libs_count, lnk_lib_node_ptr_is_before);
  for EachIndex(i, task.valid_libs_count) {
    lnk_lib_list_push_node(list, task.valid_libs[i]);
  }

  LNK_LibNodeArray result = { .count = task.valid_libs_count, task.valid_libs };

  scratch_end(scratch);
  return result;
}

internal B32
lnk_lib_set_link_symbol(LNK_Lib *lib, U32 member_idx, LNK_Symbol *link_symbol)
{
  local_persist LNK_Symbol null_symbol;

  LNK_Symbol *slot = ins_atomic_ptr_eval_assign(&lib->member_links[member_idx], &null_symbol);

  B32 was_linked = (slot == 0);

  for (LNK_Symbol *leader = link_symbol;;) {
    // update slot symbol if it is empty or link symbol comes before symbol in the slot
    if (slot && slot != &null_symbol) {
      if (lnk_symbol_is_before(slot, leader)) {
        leader = slot;
      }
    } else {
      leader = link_symbol;
    }

    // try to insert back updated slot symbol
    LNK_Symbol *swap = ins_atomic_ptr_eval_cond_assign(&lib->member_links[member_idx], leader, &null_symbol);

    // exit if slot symbol was null
    if (swap == &null_symbol) {
      break;
    }

    // reload slot symbol
    slot = ins_atomic_ptr_eval_assign(&lib->member_links[member_idx], &null_symbol);
  }

  return was_linked;
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

