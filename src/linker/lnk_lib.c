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

internal force_inline U64
lnk_symbol_name_disc(String8 name)
{
  // pack the first 8 bytes big-endian so an integer compare orders exactly like memcmp; pad short
  // names with zeros. this is faithful to str8_compar_case_sensitive INCLUDING the size tie-break
  // (base_strings.c: shorter prefix precedes): at the first differing padded byte either both
  // strings have a real byte there (== memcmp order), or the shorter string's zero pad compares
  // below the longer string's next real byte (== shorter-prefix-precedes). a zero pad byte can tie
  // only with another pad byte or an embedded NUL, and any such tie leaves the discriminators
  // marching in lockstep until a real difference or full equality -- so disc inequality ALWAYS
  // decides the compare, and disc equality falls through to the full str8_compar.
  U64 disc = 0;
  U64 n    = Min(name.size, 8);
  for (U64 i = 0; i < n; i += 1) {
    disc |= (U64)name.str[i] << (56 - i*8);
  }
  return disc;
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
    
    // compress member offsets to match those from the second header
    {
      HashTable *member_off_ht = hash_table_init(scratch.arena, (U64)((F64)first_member.symbol_count * 1.3));
      for EachIndex(symbol_idx, symbol_count) {
        U32 member_off = from_be_u32(first_member.member_offsets[symbol_idx]);
        if (!hash_table_search_u32_u32(member_off_ht, member_off, 0)) {
          hash_table_push_u32_u32(scratch.arena, member_off_ht, member_off, member_off_ht->count);
        }
      }

      symbol_indices = push_array(arena, U16, first_member.symbol_count);
      for EachIndex(symbol_idx, first_member.symbol_count) {
        U32 member_off = from_be_u32(first_member.member_offsets[symbol_idx]);
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

  // build packed discriminators parallel to the (sorted) symbol name dir; bsearch probes read this
  // contiguous array and touch archive string-table bytes only on discriminator ties
  U64 *symbol_discs = push_array_no_zero(arena, U64, symbol_names.count);
  for EachIndex(symbol_idx, symbol_names.count) {
    symbol_discs[symbol_idx] = lnk_symbol_name_disc(symbol_names.v[symbol_idx]);
  }

  // init lib
  lib_out->path              = push_str8_copy(arena, path);
  lib_out->data              = data;
  lib_out->type              = type;
  lib_out->member_count      = member_count;
  lib_out->symbol_count      = Min(symbol_count, symbol_names.count); // TODO: warn about mismatched number of symbol names and symbol count in the header
  lib_out->member_offsets    = member_offsets;
  lib_out->symbol_indices    = symbol_indices;
  lib_out->symbol_names      = symbol_names;
  lib_out->symbol_discs      = symbol_discs;
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
    U64 invalid_lib_idx = ins_atomic_u64_inc_eval(&task->invalid_libs_count)-1;
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
    lnk_error(LNK_Error_InvalidLib, "%S: failed to parse library", inputs[input_idx - lib_id_base]->path);
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

internal force_inline int
lnk_disc_str8_compar(U64 a_disc, String8 *a, U64 b_disc, String8 *b)
{
  // discriminator inequality decides the compare without touching string bytes (see
  // lnk_symbol_name_disc for the order-fidelity argument); ties fall through to the full compare
  if (a_disc != b_disc) {
    return a_disc < b_disc ? -1 : +1;
  }
  return str8_compar_case_sensitive(a, b);
}

// str8_array_bsearch with a packed-discriminator pre-filter: identical probe sequence and result
// (each probe's compare outcome is identical), but probes read the contiguous disc[] array instead
// of chasing symbol_names.v[].str into scattered archive string-table bytes
internal U64
lnk_lib_bsearch_symbol_name(LNK_Lib *lib, String8 value)
{
  String8Array arr  = lib->symbol_names;
  U64         *disc = lib->symbol_discs;
  if (arr.count > 1) {
    U64 value_disc = lnk_symbol_name_disc(value);

    int lo_compar = lnk_disc_str8_compar(value_disc, &value, disc[0], &arr.v[0]);
    if (lo_compar == 0) {
      return 0;
    }

    int hi_compar = lnk_disc_str8_compar(value_disc, &value, disc[arr.count-1], &arr.v[arr.count-1]);
    if (hi_compar == 0) {
      return arr.count-1;
    }

    if (lo_compar > 0 && hi_compar < 0) {
      for (U64 l = 0, r = arr.count-1; l <= r; ) {
        U64 m = l + (r - l) / 2;
        int cmp = lnk_disc_str8_compar(disc[m], &arr.v[m], value_disc, &value);
        if (cmp == 0) {
          return m;
        } else if (cmp < 0) {
          l = m + 1;
        } else {
          r = m - 1;
        }
      }
    }
  } else if (arr.count == 1 && str8_match(arr.v[0], value, 0)) {
    return 0;
  }
  return max_U64;
}

internal force_inline B32
lnk_search_lib(LNK_Lib *lib, String8 symbol_name, U32 *member_idx_out)
{
  U64 symbol_idx = lnk_lib_bsearch_symbol_name(lib, symbol_name);
  if (symbol_idx < lib->symbol_count) {
    if (member_idx_out) {
      *member_idx_out = lib->symbol_indices[symbol_idx]-1;
    }
    return 1;
  }
  return 0;
}

