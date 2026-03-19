// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

static Arena *g_huge_arena = 0;

internal Arena *
lnk_get_huge_arena(void)
{
  if (g_huge_arena == 0) {
    g_huge_arena = arena_alloc(.name = "HUGE");
  }
  return g_huge_arena;
}

internal void
lnk_discard_cv_debug_info(LNK_CodeViewInput *input, U64 obj_idx)
{
  // discard symbols
  String8List *symbols_ptr = cv_sub_section_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
  MemoryZeroStruct(symbols_ptr);

  // discard inline sites
  String8List *inlineelines_ptr = cv_sub_section_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  MemoryZeroStruct(inlineelines_ptr);
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_s_task)
{
  U64                      obj_idx = task_id;
  LNK_ParseDebugSTaskData *task    = raw_task;

  LNK_Obj    *obj       = task->obj_arr[obj_idx];
  String8List sect_list = task->sect_list_arr[obj_idx];
  CV_DebugS  *debug_s   = &task->debug_s_arr[obj_idx];

  for (String8Node *node = sect_list.first; node != 0; node = node->next) {
    // parse & merge sub sections
    CV_DebugS ds = cv_debug_s_from_data(arena, node->string);
    cv_debug_s_concat_in_place(debug_s, &ds);

    // make sure there is one string table
    String8List string_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_StringTable);
    if (string_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, obj, ".debug$S has %u string table sub-sections defined, picking first sub-section", string_data_list.node_count);
    }

    // make sure there is one file checksum table
    String8List checksum_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_FileChksms);
    if (checksum_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, obj, ".debug$S has %u file checksum sub-sections defined, picking first sub-section", checksum_data_list.node_count);
    }
  }
}

internal CV_DebugS *
lnk_parse_debug_s_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, String8List *sect_list_arr)
{
  ProfBeginFunction();

  LNK_ParseDebugSTaskData task_data = {0};
  task_data.obj_arr                 = obj_arr;
  task_data.sect_list_arr           = sect_list_arr;
  task_data.debug_s_arr             = push_array(arena->v[0], CV_DebugS, obj_count);

  tp_for_parallel(tp, arena, obj_count, lnk_parse_debug_s_task, &task_data);

  ProfEnd();
  return task_data.debug_s_arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_strip_debug_t_sig_task)
{
  U64                         obj_idx = task_id;
  LNK_CheckDebugTSigTaskData *task    = raw_task;

  String8Array data_arr = task->data_arr_arr[obj_idx];
  LNK_Obj *obj = task->obj_arr[obj_idx];

  for EachIndex(i, data_arr.count) {
    String8 *d = data_arr.v + i;
    if (d->size == 0) { continue; }

    if (d->size < sizeof(CV_Signature)) {
      lnk_error_obj(LNK_Error_IllData, obj, ".debug$T must have at least 4 bytes for CodeView signature");
    }

    CV_Signature sig = cv_signature_from_debug_s(*d);
    switch (sig) {
    default: {
      lnk_error_obj(LNK_Warning_IllData, obj, "unknown CodeView type signature in section (TODO: print section index)");
      *d = str8(0,0);
    } break;
    case CV_Signature_C13: {
      *d = str8_skip(*d, sizeof(CV_Signature));
    } break;
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_t_task)
{
  ProfBeginFunction();
  LNK_ParseDebugTTaskData *task  = raw_task;
  if (task->data_arr_arr[task_id].count > 0) {
    task->debug_t_arr[task_id] = cv_debug_t_from_data(arena, task->data_arr_arr[task_id].v[0], CV_LeafAlign);
  } else {
    MemoryZeroStruct(&task->debug_t_arr[task_id]);
  }
  ProfEnd();
}

internal CV_DebugT *
lnk_parse_debug_t_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, String8List *debug_t_list_arr)
{
  ProfBeginFunction();
  
  // list -> array
  String8Array *data_arr_arr = str8_array_from_list_arr(arena->v[0], debug_t_list_arr, obj_count);

  // validate signatures
  LNK_CheckDebugTSigTaskData check_sig;
  check_sig.obj_arr      = obj_arr;
  check_sig.data_arr_arr = data_arr_arr;
  tp_for_parallel(tp, 0, obj_count, lnk_strip_debug_t_sig_task, &check_sig);

  // parse debug types
  LNK_ParseDebugTTaskData parse;
  parse.data_arr_arr = data_arr_arr;
  parse.debug_t_arr  = push_array_no_zero(arena->v[0], CV_DebugT, obj_count);
  tp_for_parallel(tp, arena, obj_count, lnk_parse_debug_t_task, &parse);

  ProfEnd();
  return parse.debug_t_arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_cv_symbols_task)
{
  LNK_ParseCVSymbolsTaskData *task  = raw_task;
  LNK_SymbolInput            *input = &task->inputs[task_id];
  cv_parse_symbol_sub_section(arena, input->symbol_list, 0, input->raw_symbols, CV_SymbolAlign);
}

internal
THREAD_POOL_TASK_FUNC(lnk_read_type_servers_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  LNK_CodeViewInput *task = raw_task;

  B32             discard_debug_info = 1;
  U64             ts_idx             = task_id;
  LNK_TypeServer *ts                 = &task->ts_arr.v[ts_idx];

  // read PDB from disk
  String8 msf_data = lnk_read_data_from_file_path(scratch.arena, task->io_flags, ts->ts_path);

  // check magic
  if (!msf_check_magic_70(msf_data) && msf_check_magic_20(msf_data)) { goto exit; }

  // read the stream table
  MSF_RawStreamTable *st = msf_raw_stream_table_from_data(scratch.arena, msf_data);
  if (st == 0) { goto exit; }

  // PDB must have these streams
  if (PDB_FixedStream_Tpi >= st->stream_count || PDB_FixedStream_Ipi >= st->stream_count || PDB_FixedStream_Info >= st->stream_count) { goto exit; }

  // read info stream
  String8       info_data  = msf_data_from_stream_number(scratch.arena, msf_data, st, PDB_FixedStream_Info);
  PDB_InfoParse info_parse = {0};
  pdb_info_parse_from_data(info_data, &info_parse);

  // match GUID from obj against one in the type server
  if (!MemoryMatchStruct(&info_parse.guid, &ts->ts_info.sig)) {
    lnk_error(LNK_Warning_MismatchedTypeServerSignature,
              "%S: signature mismatch in type server read from disk, expected %S, got %S",
              ts->ts_info.name,
              string_from_guid(scratch.arena, ts->ts_info.sig),
              string_from_guid(scratch.arena, info_parse.guid));
    goto exit;
  }

  MSF_StreamNumber type_streams[CV_TypeIndexSource_COUNT] = {};
  type_streams[CV_TypeIndexSource_TPI] = PDB_FixedStream_Tpi;
  type_streams[CV_TypeIndexSource_IPI] = PDB_FixedStream_Ipi;

  Rng1U64 ti_ranges  [CV_TypeIndexSource_COUNT] = {0};
  Rng1U64 leaf_ranges[CV_TypeIndexSource_COUNT] = {0};
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    MSF_StreamNumber sn = type_streams[ti_source];
    if (sn == 0) { continue; }
    if (!pdb_extract_type_server_info(msf_data, st, sn, &ti_ranges[ti_source], &leaf_ranges[ti_source])) { goto exit; }
  }

  // alloc buffer where TPI and IPI are adjecent
  U64 buffer_size = 0;
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { buffer_size += dim_1u64(leaf_ranges[ti_source]); }
  buffer_size = AlignPow2(buffer_size, 4) + ARENA_HEADER_SIZE;
  U8    *buffer = push_array(arena, U8, buffer_size);
  Arena *fixed  = arena_alloc( .reserve_size = buffer_size, .commit_size = buffer_size, .optional_backing_buffer = buffer );

  // read both streams into a contiguous buffer
  String8 source_data_arr[CV_TypeIndexSource_COUNT] = {0};
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    MSF_StreamNumber sn = type_streams[ti_source];
    if (sn == 0) { continue; }
    source_data_arr[ti_source] = msf_data_from_stream_number_ex(fixed, msf_data, st, sn, leaf_ranges[ti_source], PDB_LEAF_ALIGN);
    Assert(source_data_arr[ti_source].size == dim_1u64(leaf_ranges[ti_source]));
  }
  // assert streams are adjecent in the buffer
  for (U64 i = 1; i+1 < CV_TypeIndexSource_COUNT; i += 1) { AssertAlways(source_data_arr[i].str + source_data_arr[i].size == source_data_arr[i+1].str); }
  String8 type_data = str8(buffer + ARENA_HEADER_SIZE, buffer_size - ARENA_HEADER_SIZE);

  // map type server to -> .debug$T
  U64        obj_idx = task->type_server_indices.v[task_id];
  CV_DebugT *debug_t = &task->debug_t_arr[obj_idx];

  // read types
  CV_DebugT d = cv_debug_t_from_data(arena, type_data, PDB_LEAF_ALIGN);

  // @type_server .debug$T
  debug_t->count   = d.count;
  debug_t->data    = d.data;
  debug_t->offsets = d.offsets;
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_ranges[i] = ti_ranges[i]; }
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_base[i] = IntFromPtr(source_data_arr[i].str - type_data.str); }
  MemoryCopyTyped(debug_t->source_counts, d.source_counts, CV_TypeIndexSource_COUNT);
  MemoryCopyTyped(debug_t->source_offsets, d.source_counts, CV_TypeIndexSource_COUNT);
  u64_array_counts_to_offsets(CV_TypeIndexSource_COUNT, debug_t->source_offsets);

  discard_debug_info = 0;
  exit:;
  if (discard_debug_info) {
    // an error occurred while loading external type server, discard
    // parts debug info in dependent objs that rely on types
    for EachNode(n, U64Node, task->ts_arr.v[ts_idx].obj_indices.first) {
      lnk_discard_cv_debug_info(task, n->data);
    }
  }

  task->is_type_server_discarded[task_id] = discard_debug_info;

  ProfEnd();
  scratch_end(scratch);
}

internal LNK_CodeViewInput
lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_IO_Flags io_flags, String8List lib_dir_list, String8List alt_pch_dirs, U64 obj_count, LNK_Obj **obj_arr)
{
  ProfBegin("Extract CodeView");
  Temp scratch = scratch_begin(0,0);

  LNK_CodeViewInput input = { .io_flags = io_flags, .count = obj_count, .obj_arr = obj_arr, .ts_obj_range = r1u64(0,0) };
  
  ProfBegin("Collect CodeView");
  // TODO: fix memory leak, we need a Temp wrapper for pool arena
  String8List *debug_s_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$S"), 0);
  String8List *debug_p_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$P"), 0);
  String8List *debug_t_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$T"), 0);
  ProfEnd();

  if (lnk_get_log_status(LNK_Log_Debug) || PROFILE_TELEMETRY) {
    U64 total_debug_s_size = 0, total_debug_t_size = 0, total_debug_p_size = 0;
    for EachIndex(obj_idx, obj_count) {
      for EachNode(n, String8Node, debug_s_list_arr[obj_idx].first) { total_debug_s_size += n->string.size; }
      for EachNode(n, String8Node, debug_t_list_arr[obj_idx].first) { total_debug_t_size += n->string.size; }
      for EachNode(n, String8Node, debug_p_list_arr[obj_idx].first) { total_debug_p_size += n->string.size; }
    }
	
    ProfNoteV("Total .debug$S Input Size: %M", total_debug_s_size);
    ProfNoteV("Total .debug$T Input Size: %M", total_debug_t_size);
    ProfNoteV("Total .debug$P Input Size: %M", total_debug_p_size);
	
    if (lnk_get_log_status(LNK_Log_Debug)) {
      lnk_log(LNK_Log_Debug, "[Total .debug$S Input Size %M]", total_debug_s_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$T Input Size %M]", total_debug_t_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$P Input Size %M]", total_debug_p_size);
    }
  }

  ProfBegin("Parse CodeView");
  CV_DebugT *debug_p_arr = lnk_parse_debug_t_sections(tp, tp_arena, obj_count, obj_arr, debug_p_list_arr);
  input.debug_s_arr = lnk_parse_debug_s_sections(tp, tp_arena, obj_count, obj_arr, debug_s_list_arr);
  input.debug_t_arr = lnk_parse_debug_t_sections(tp, tp_arena, obj_count, obj_arr, debug_t_list_arr);
  input.debug_h_arr = push_array(tp_arena->v[0], CV_DebugH, obj_count); // TODO: collect & parse .debug$H
  ProfEnd();

  ProfBegin("Set up symbol parsing");
  for EachIndex(obj_idx, obj_count) { input.symbol_input_count += cv_sub_section_from_debug_s(input.debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols).node_count; }
  input.symbol_inputs  = push_array_no_zero(tp_arena->v[0], LNK_SymbolInput,    input.symbol_input_count);
  input.parsed_symbols = push_array_no_zero(tp_arena->v[0], CV_SymbolListArray, obj_count);
  {
    CV_SymbolList *reserved_lists = push_array(tp_arena->v[0], CV_SymbolList, input.symbol_input_count);
    for (U64 obj_idx = 0, input_idx = 0; obj_idx < obj_count; ++obj_idx) {
      String8List raw_symbols = cv_sub_section_from_debug_s(input.debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);

      // init parse output
      if (raw_symbols.node_count > 0) {
        input.parsed_symbols[obj_idx].count = raw_symbols.node_count;
        input.parsed_symbols[obj_idx].v     = reserved_lists + input_idx;
      } else {
        input.parsed_symbols[obj_idx].count = 0;
        input.parsed_symbols[obj_idx].v     = 0;
      }

      // init worker input
      for (String8Node *data_n = raw_symbols.first; data_n != 0; data_n = data_n->next, ++input_idx) {
        Assert(input_idx < input.symbol_input_count);
        LNK_SymbolInput *in = &input.symbol_inputs[input_idx];
        in->obj_idx                  = obj_idx;
        in->symbol_list              = &reserved_lists[input_idx];
        in->raw_symbols              = data_n->string;
      }
    }

    tp_for_parallel_prof(tp, tp_arena, input.symbol_input_count, lnk_parse_cv_symbols_task, &(LNK_ParseCVSymbolsTaskData){ .inputs = input.symbol_inputs }, "Symbol Parse");
  }
  ProfEnd();

  // sort objs based on type: PCH, /Zi (external), /Z7 (internal)
  input.debug_p_indices.v = push_array(tp_arena->v[0], U32, obj_count); 
  input.ext_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  input.int_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  for EachIndex(obj_idx, obj_count) {
    CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];
    CV_DebugT *debug_p = &debug_p_arr[obj_idx];
    U32Array  *arr_ptr;
    if      (debug_p->count > 0 && debug_t->count == 0) { arr_ptr = &input.debug_p_indices; }
    else if (cv_debug_t_is_type_server_ref(debug_t))    { arr_ptr = &input.ext_obj_indices; }
    else                                                { arr_ptr = &input.int_obj_indices; }
    arr_ptr->v[arr_ptr->count++] = obj_idx;

    if (debug_t->count == 0 && debug_p->count > 0) {
      *debug_t = *debug_p;
    } else if (debug_t->count && debug_p->count) {
      lnk_error_obj(LNK_Warning_MultipleDebugTAndDebugP, obj_arr[obj_idx], "multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections");
      MemoryZeroStruct(debug_t);
      MemoryZeroStruct(debug_p);
    }
  }

  ProfScope("Set up type servers")
  {
    input.obj_to_ts = push_array(tp_arena->v[0], U64, input.count);
    LNK_TypeServerList  ts_list = {0};
    HashTable          *ts_ht   = hash_table_init(scratch.arena, 256);

    // push null so unopened type servers are resolved to this type server
    LNK_TypeServerNode *null_ts = push_array(scratch.arena, LNK_TypeServerNode, 1);
    SLLQueuePush(ts_list.first, ts_list.last, null_ts);
    ts_list.count += 1;
    null_ts->v.ts_path = str8_lit("");
    hash_table_push_path_raw(scratch.arena, ts_ht, str8_lit(""), null_ts);

    for EachIndex(i, input.ext_obj_indices.count) {
      // first leaf is always type server
      U64                obj_idx = input.ext_obj_indices.v[i];
      CV_DebugT         *debug_t = &input.debug_t_arr[obj_idx];
      CV_Leaf            leaf    = cv_debug_t_get_leaf(debug_t, 0);
      CV_TypeServerInfo  ts_info = cv_type_server_info_from_leaf(leaf);
      String8            ts_path = lnk_find_first_file(scratch.arena, lib_dir_list, ts_info.name);

      // insert new type server
      LNK_TypeServer *ts = hash_table_search_path_raw(ts_ht, ts_path);
      if (ts == 0) {
        LNK_TypeServerNode *n = push_array(scratch.arena, LNK_TypeServerNode, 1);
        SLLQueuePush(ts_list.first, ts_list.last, n);
        ts_list.count += 1;
        ts = &n->v;
        ts->ts_info = ts_info;
        ts->ts_idx  = ts_ht->count;
        ts->ts_path = push_str8_copy(tp_arena->v[0], ts_path);
        hash_table_push_path_raw(scratch.arena, ts_ht, ts->ts_path, ts);
      }
      
      // signature check
      if ( ! MemoryMatchStruct(&ts_info.sig, &ts->ts_info.sig)) {
        lnk_error_obj(LNK_Error_ExternalTypeServerConflict,
                      obj_arr[obj_idx],
                      "type server signature conflicts with type server from '%S'",
                      obj_arr[ts->obj_indices.first->data]->path);
        continue;
      }

      // type server -> obj
      u64_list_push(tp_arena->v[0], &ts->obj_indices, obj_idx);

      // obj -> type server
      input.obj_to_ts[obj_idx] = ts->ts_idx;
    }

    // list -> array
    LNK_TypeServerArray ts_arr = { .v = push_array(tp_arena->v[0], LNK_TypeServer, ts_list.count) };
    for EachNode(n, LNK_TypeServerNode, ts_list.first) { ts_arr.v[ts_arr.count++] = n->v; }

    // extend arrays to include type servers
    if (ts_arr.count) {
      LNK_CodeViewInput prev = input;

      input.count += ts_arr.count;
      input.obj_arr        = push_array(tp_arena->v[0], LNK_Obj *,          input.count);
      input.debug_s_arr    = push_array(tp_arena->v[0], CV_DebugS,          input.count);
      input.debug_t_arr    = push_array(tp_arena->v[0], CV_DebugT,          input.count);
      input.debug_h_arr    = push_array(tp_arena->v[0], CV_DebugH,          input.count);
      input.parsed_symbols = push_array(tp_arena->v[0], CV_SymbolListArray, input.count);
      input.obj_to_ts      = push_array(tp_arena->v[0], U64,                input.count);

      MemoryCopyTyped(input.obj_arr,        prev.obj_arr,        prev.count);
      MemoryCopyTyped(input.debug_s_arr,    prev.debug_s_arr,    prev.count);
      MemoryCopyTyped(input.debug_t_arr,    prev.debug_t_arr,    prev.count);
      MemoryCopyTyped(input.parsed_symbols, prev.parsed_symbols, prev.count);
      MemoryCopyTyped(input.obj_to_ts,      prev.obj_to_ts,      prev.count);

      input.ts_obj_range = r1u64(prev.count, input.count);

      // alloc dummy objs with for each loaded type server
      for EachIndex(i, ts_arr.count) {
        LNK_Obj *ts_obj = push_array(tp_arena->v[0], LNK_Obj, ts_arr.count);
        ts_obj->path = ts_arr.v[i].ts_path;
        input.obj_arr[prev.count + i] = ts_obj;
      }

      // make type server indices
      input.type_server_indices.count = ts_arr.count;
      input.type_server_indices.v     = push_array(tp_arena->v[0], U32, ts_arr.count);
      for EachIndex(i, ts_arr.count) { input.type_server_indices.v[i] = prev.count + i; }
    }

    input.ts_arr                   = ts_arr;
    input.is_type_server_discarded = push_array(tp_arena->v[0], B32, input.ts_arr.count);
    tp_for_parallel_prof(tp, tp_arena, input.ts_arr.count, lnk_read_type_servers_task, &input, "read type servers");

    // fixup null type server
    input.is_type_server_discarded[0] = 0;

    // report bad type servers
    String8List unopen_type_server_list = {0};
    for EachIndex(ts_idx, ts_arr.count) {
      if ( ! input.is_type_server_discarded[ts_idx]) { continue; }
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t%S\n", ts_arr.v[ts_idx].ts_path);
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\tDependent obj(s):\n");
      for EachNode(n, U64Node, input.ts_arr.v[ts_idx].obj_indices.first) {
        str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\t\t%S\n", obj_arr[n->data]->path);
      }
    }
    if (unopen_type_server_list.node_count) {
      String8List error_msg_list = {0};
      str8_list_pushf(scratch.arena, &error_msg_list, "unable to open external type server(s):\n");
      str8_list_concat_in_place(&error_msg_list, &unopen_type_server_list);
      lnk_error(LNK_Error_UnableToOpenTypeServer, "%S", str8_list_join(scratch.arena, &error_msg_list, 0));
    }
  }
 
  ProfBegin("Set up PCH indirection");
  {
    // hash_table<obj_path, obj_idx>
    String8    work_dir   = os_get_current_path(scratch.arena);
    HashTable *debug_p_ht = hash_table_init(scratch.arena, obj_count);
    for EachIndex(i, input.debug_p_indices.count) {
      U64     obj_idx  = input.debug_p_indices.v[i];
      String8 obj_path = path_absolute_dst_from_relative_dst_src(scratch.arena, obj_arr[obj_idx]->path, work_dir);
      if (hash_table_search_path(debug_p_ht, obj_path)) {
        lnk_error_obj(LNK_Warning_DuplicateObjPath, obj_arr[obj_idx], "duplicate obj path %S", obj_path);
      } else {
        hash_table_push_path_u64(scratch.arena, debug_p_ht, obj_path, obj_idx);
      }
    }

    for EachIndex(i, input.int_obj_indices.count) {
      U64        obj_idx = input.int_obj_indices.v[i];
      CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];

      // skip objs that do not depend on PCH
      if ( ! cv_debug_t_is_pch(debug_t)) { continue; }

      // find PCH obj
      CV_PrecompInfo precomp         = cv_precomp_info_from_leaf(cv_debug_t_get_leaf(debug_t, 0));
      String8        obj_path        = path_absolute_dst_from_relative_dst_src(scratch.arena, precomp.obj_name, work_dir);
      U64            debug_p_obj_idx = max_U64;
      if ( ! hash_table_search_path_u64(debug_p_ht, obj_path, &debug_p_obj_idx)) {
        // try alternative directory for the PCH
        String8 obj_name = str8_skip_last_slash(obj_path);
        for EachNode(alt_dir_n, String8Node, alt_pch_dirs.first) {
          String8 alt_obj_path = str8f(scratch.arena, "%S/%S", alt_dir_n->string, obj_name);
          if (hash_table_search_path_u64(debug_p_ht, alt_obj_path, &debug_p_obj_idx)) { break; }
        }
        if (debug_p_obj_idx == max_U64) {
          lnk_error_obj(LNK_Error_PrecompObjNotFound, obj_arr[obj_idx], "LF_PRECOMP references non-existent obj %S; discarding debug info", obj_path);
          lnk_discard_cv_debug_info(&input, obj_idx);
        }
      }

      // get PCH leaf data
      CV_DebugT *debug_p = &input.debug_t_arr[debug_p_obj_idx];

      // error check LF_PRECOMP
      if (precomp.start_index > CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Warning_AtypicalStartIndex,    obj_arr[obj_idx], "atypical start index 0x%x in LF_PRECOMP", precomp.start_index); }
      if (precomp.start_index < CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Error_InvalidStartIndex,       obj_arr[obj_idx], "invalid start index 0x%x in LF_PRECOMP; must be >= 0x%x", precomp.start_index, CV_MinComplexTypeIndex); continue; }
      if (precomp.leaf_count  >= debug_p->count)        { lnk_error_obj(LNK_Error_InvalidPrecompLeafCount, obj_arr[obj_idx], "leaf count %u LF_PRECOMP exceeds leaf count %u in .debug$P in %S", precomp.leaf_count, debug_p->count, obj_arr[debug_p_obj_idx]->path); continue; }

      // get LF_PRECOMP
      CV_Leaf            endprecomp_leaf = cv_debug_t_get_leaf(debug_p, precomp.leaf_count);
      CV_LeafEndPreComp *endprecomp      = str8_deserial_get_raw_ptr(endprecomp_leaf.data, 0, sizeof(*endprecomp));

      // error check LF_ENDPRECOMP
      if (endprecomp_leaf.kind      != CV_LeafKind_ENDPRECOMP)    { lnk_error_obj(LNK_Error_EndprecompNotFound, obj_arr[obj_idx], "missing LF_ENDPRECOMP [0x%x] in %S", precomp.leaf_count, obj_arr[debug_p_obj_idx]->path); continue; }
      if (endprecomp_leaf.data.size != sizeof(CV_LeafEndPreComp)) { lnk_error_obj(LNK_Error_IllData,            obj_arr[obj_idx], "invalid size 0x%x for LF_ENDPRECOMP", endprecomp_leaf.data.size); continue; }
      if (endprecomp->sig           != precomp.sig)               { lnk_error_obj(LNK_Error_PrecompSigMismatch, obj_arr[obj_idx], "PCH signature mismatch, expected 0x%x got 0x%x; PCH obj %S", precomp.sig, endprecomp->sig, obj_arr[debug_p_obj_idx]->path); continue; }

      for (U64 i = 1; i < CV_TypeIndexSource_COUNT; i += 1) { debug_t->pch_ti_range[i] = r1u64(precomp.start_index, precomp.start_index + precomp.leaf_count); }
      debug_t->pch_obj_idx  = debug_p_obj_idx;

      // remove CV_LeafKind_PRECOMP
      debug_t->count    -= 1;
      debug_t->offsets  += 1;
    }

    // remove CV_LeafKind_ENDPRECOMP from .debug$P
    for EachIndex(i, input.debug_p_indices.count) {
      U64            debug_p_idx = input.debug_p_indices.v[i];
      CV_DebugT     *debug_p     = &input.debug_t_arr[debug_p_idx];
      for EachIndex(i, debug_p->count) {
        U64            lf_idx = debug_p->count - (i + 1);
        CV_LeafHeader *lf     = cv_debug_t_get_leaf_header(debug_p, lf_idx);
        if (lf->kind == CV_LeafKind_ENDPRECOMP) {
          memory_write16(&lf->kind, CV_LeafKind_NOTYPE);
          memory_write16(&lf->size, sizeof(CV_LeafKind));
          break;
        }
      }
    }
  }
  ProfEnd();

  // PCH and /Z7 objs have default min type index set to CV_MinComplexTypeIndex
  // but type servers can bump up the lower bound. In practice nobody does this.
  // But to cover all our bases loop through type servers and compute max
  // lower bound.
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { input.min_type_indices[ti_source] = CV_MinComplexTypeIndex; }
  for EachInRange(ts_idx, input.ts_obj_range) {
    CV_DebugT *debug_t = &input.debug_t_arr[ts_idx];
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      input.min_type_indices[ti_source] = Max(input.min_type_indices[ti_source], debug_t->ti_ranges[ti_source].min);
    }
  }

  // :zero_out_symbol_sub_section
  ProfBegin("Zero-out Symbols Sub-sections");
  for EachIndex(i, obj_count) { MemoryZeroStruct(cv_sub_section_ptr_from_debug_s(&input.debug_s_arr[i], CV_C13SubSectionKind_Symbols)); }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return input;
}

internal LNK_LeafRef
lnk_leaf_ref_from_ti(LNK_CodeViewInput *input, U32 obj_idx, CV_TypeIndexSource source, CV_TypeIndex ti)
{
  LNK_LeafRef leaf_ref;
  CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
  if (contains_1u64(debug_t->pch_ti_range[source], ti)) {
    leaf_ref = (LNK_LeafRef){
      .obj_idx  = debug_t->pch_obj_idx,
      .leaf_idx = cv_leaf_idx_from_ti(&input->debug_t_arr[debug_t->pch_obj_idx], source, ti)
    };
  } else {
    U64 ts_idx = input->obj_to_ts[obj_idx];
    if (ts_idx != 0) {
      U64 ts_obj_idx = input->ts_obj_range.min + ts_idx;
      CV_DebugT *ts_debug_t = &input->debug_t_arr[ts_obj_idx];
      leaf_ref = (LNK_LeafRef){
        .obj_idx  = ts_obj_idx,
        .leaf_idx = cv_leaf_idx_from_ti(ts_debug_t, source, ti)
      };
    } else {
      leaf_ref = (LNK_LeafRef){
        .obj_idx  = obj_idx,
        .leaf_idx = cv_leaf_idx_from_ti(debug_t, source, ti)
      };
    }
  }
  return leaf_ref;
}

internal int
lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b)
{
  int cmp = 0;
  if (a.obj_idx < b.obj_idx) {
    cmp = -1;
  } else if (a.obj_idx > b.obj_idx) {
    cmp = +1;
  } else {
    if (a.leaf_idx < b.leaf_idx) {
      cmp = -1;
    } else if (a.leaf_idx > b.leaf_idx) {
      cmp = +1;
    }
  }
  return cmp;
}

internal int
lnk_leaf_ref_is_before(void *raw_a, void *raw_b)
{
  return lnk_leaf_ref_compare(**(LNK_LeafRef **)raw_a, **(LNK_LeafRef **)raw_b) < 0;
}

internal B32
lnk_match_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef a, LNK_LeafRef b)
{
  B32 is_match = 0;
  CV_DebugT *a_debug_t = &input->debug_t_arr[a.obj_idx];
  CV_DebugT *b_debug_t = &input->debug_t_arr[b.obj_idx];
  U64        a_hash    = input->debug_h_arr[a.obj_idx].v[a.leaf_idx];
  U64        b_hash    = input->debug_h_arr[b.obj_idx].v[b.leaf_idx];
  if (a_hash == b_hash) {
    CV_Leaf a_leaf = cv_debug_t_get_leaf(a_debug_t, a.leaf_idx);
    CV_Leaf b_leaf = cv_debug_t_get_leaf(b_debug_t, b.leaf_idx);
    Assert(a_leaf.kind == b_leaf.kind);
    is_match = 1;
  }
  return is_match;
}

internal U64
lnk_hash_cv_leaf(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list, B32 discard_cycles)
{
  CV_DebugT          *debug_t        = &input->debug_t_arr[leaf_ref.obj_idx];
  CV_Leaf             leaf           = cv_debug_t_get_leaf(debug_t, leaf_ref.leaf_idx);
  CV_TypeIndexSource  curr_ti_source = cv_type_index_source_from_leaf_kind(leaf.kind);
  CV_TypeIndex        curr_ti        = cv_ti_from_leaf_idx(debug_t, curr_ti_source, leaf_ref.leaf_idx);

  // init hasher
  blake3_hasher hasher; blake3_hasher_init(&hasher);

  // hash leaf header
  {
    CV_LeafHeader header;
    header.size = (U16)leaf.data.size;
    header.kind = leaf.kind;
    blake3_hasher_update(&hasher, &header, sizeof(header));
  }

  // hash bytes around indices
  {
    U64 last_ti_off = 0;
    for EachNode(ti_info, CV_TypeIndexInfo, ti_info_list.first) {
      U8 *bytes = leaf.data.str + last_ti_off;
      U64 size  = ti_info->offset - last_ti_off;
      blake3_hasher_update(&hasher, bytes, size);
      last_ti_off = ti_info->offset + sizeof(CV_TypeIndex);
    }

    Assert(leaf.data.size >= last_ti_off);
    U8 *bytes = leaf.data.str + last_ti_off;
    U64 size  = leaf.data.size - last_ti_off;
    blake3_hasher_update(&hasher, bytes, size);
  }

  // mix-in sub leaf hashes
  for EachNode(sub_ti_n, CV_TypeIndexInfo, ti_info_list.first) {
    CV_TypeIndex *sub_ti_ptr = str8_deserial_get_raw_ptr(leaf.data, sub_ti_n->offset, sizeof(*sub_ti_ptr));
    CV_TypeIndex  sub_ti     = memory_read32(sub_ti_ptr);
    
    // simple indices are stable across compile units 
    if (sub_ti < debug_t->ti_ranges[sub_ti_n->source].min) {
      blake3_hasher_update(&hasher, &sub_ti, sizeof(sub_ti));
      continue;
    }

    if (sub_ti >= debug_t->ti_ranges[sub_ti_n->source].max) {
      Temp scratch = scratch_begin(0,0);

      String8 out_of_bounds_data = push_str8f(scratch.arena, "out_of_bounds_ti_%u_in_%u", sub_ti, leaf_ref.obj_idx);
      blake3_hasher_update(&hasher, out_of_bounds_data.str, out_of_bounds_data.size);

      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) out of bounds type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[leaf_ref.obj_idx], "%S", error_msg);

      scratch_end(scratch);
      continue;
    }

    // discard type with a cyclic-ref
    B32 is_type_graph_cyclic = discard_cycles && sub_ti > 0 && sub_ti > curr_ti;
    if (is_type_graph_cyclic) {
      // discard type
      U32  leaf_idx    = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      U8  *leaf_header = debug_t->data.str + debug_t->offsets[leaf_idx];
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, size), 0);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) forward refs member type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[leaf_ref.obj_idx], "%S", error_msg);
      scratch_end(scratch);

      continue;
    }

    // type index -> hash
    LNK_LeafRef sub_ref  = lnk_leaf_ref_from_ti(input, leaf_ref.obj_idx, sub_ti_n->source, sub_ti);
    U64         sub_hash = input->debug_h_arr[sub_ref.obj_idx].v[sub_ref.leaf_idx];

    // mix-in sub-type hash
    blake3_hasher_update(&hasher, &sub_hash, sizeof(sub_hash));
  }

  U64 hash;
  blake3_hasher_finalize(&hasher, (U8 *) &hash, sizeof(hash));

  Assert(hash != 0);
  Assert(input->debug_h_arr[leaf_ref.obj_idx].v[leaf_ref.leaf_idx] == 0 ||
         input->debug_h_arr[leaf_ref.obj_idx].v[leaf_ref.leaf_idx] == 1);
  input->debug_h_arr[leaf_ref.obj_idx].v[leaf_ref.leaf_idx] = hash;
  return hash;
}

internal void
lnk_hash_cv_leaf_deep(Arena               *arena,
                      LNK_CodeViewInput   *input,
                      LNK_LeafRef          root_leaf_ref,
                      CV_TypeIndexInfoList root_ti_info_list)
{
  Temp temp = temp_begin(arena);

  typedef struct HashStack {
    struct HashStack    *next;
    LNK_LeafRef          leaf_ref;
    CV_TypeIndexInfoList ti_info_list;
    CV_TypeIndexInfo    *ti_info;
    CV_Leaf              leaf;
    CV_TypeIndex         ti;
    CV_TypeIndexSource   ti_source;
  } HashStack;

  // set up root frame
  CV_DebugT *root_debug_t = &input->debug_t_arr[root_leaf_ref.obj_idx];
  HashStack *root_frame = push_array(temp.arena, HashStack, 1);
  root_frame->leaf_ref     = root_leaf_ref;
  root_frame->ti_info_list = root_ti_info_list;
  root_frame->ti_info      = root_ti_info_list.first;
  root_frame->leaf         = cv_debug_t_get_leaf(root_debug_t, root_leaf_ref.leaf_idx);
  root_frame->ti_source    = cv_type_index_source_from_leaf_kind(root_frame->leaf.kind);
  root_frame->ti           = cv_ti_from_leaf_idx(root_debug_t, root_frame->ti_source, root_leaf_ref.leaf_idx);

  HashStack *stack = root_frame;
  while (stack) {
    while (stack->ti_info) {
      CV_TypeIndexInfo *ti_info = stack->ti_info;

      // advance iterator
      stack->ti_info = stack->ti_info->next;

      // get type index info
      CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(stack->leaf.data, ti_info->offset, sizeof(*ti_ptr));
      CV_TypeIndex  ti     = memory_read32(ti_ptr);

      // skip out of bounds indices
      if ( ! contains_1u64(input->debug_t_arr[root_leaf_ref.obj_idx].ti_ranges[ti_info->source], ti)) { continue; }

      // skip hashed types
      LNK_LeafRef leaf_ref = lnk_leaf_ref_from_ti(input, root_leaf_ref.obj_idx, ti_info->source, ti);
      if (input->debug_h_arr[leaf_ref.obj_idx].v[leaf_ref.leaf_idx] != 0) { continue; }
      input->debug_h_arr[leaf_ref.obj_idx].v[leaf_ref.leaf_idx] = 1;

      // recurse down to sub types
      HashStack *frame = push_array(temp.arena, HashStack, 1);
      frame->leaf_ref     = leaf_ref;
      frame->leaf         = cv_debug_t_get_leaf(&input->debug_t_arr[leaf_ref.obj_idx], leaf_ref.leaf_idx);
      frame->ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, frame->leaf.kind, frame->leaf.data);
      frame->ti_info      = frame->ti_info_list.first;
      frame->ti           = ti;
      frame->ti_source    = ti_info->source;
      SLLStackPush(stack, frame);
      break;
    }

    // no more type indices, pop frame
    if ( ! stack->ti_info) {
      lnk_hash_cv_leaf(input, stack->leaf_ref, stack->ti_info_list, 0);
      SLLStackPop(stack);
    }
  }

  temp_end(temp);
}

internal LNK_LeafRef *
lnk_leaf_hash_table_search(LNK_LeafHashTable *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  LNK_LeafRef *match = 0;

  CV_DebugT *debug_t         = &input->debug_t_arr[leaf_ref.obj_idx];
  CV_DebugH *debug_h         = &input->debug_h_arr[leaf_ref.obj_idx];
  U64        hash            = debug_h->v[leaf_ref.leaf_idx];
  U64        best_bucket_idx = hash % ht->cap;
  U64        bucket_idx      = best_bucket_idx;
  do {
    LNK_LeafRef *bucket = ht->bucket_arr[bucket_idx];
    if (bucket == 0) { break; }

    if (lnk_match_leaf_ref(input, *bucket, leaf_ref)) {
      match = bucket;
      break;
    }

    bucket_idx = (bucket_idx + 1) == ht->cap ? 0 : (bucket_idx + 1);
  } while (bucket_idx != best_bucket_idx);

  return match;
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task    = raw_task;
  U32             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  for EachIndex(leaf_idx, debug_t->count) {
    Temp                 temp    = temp_begin(task->fixed_arenas[worker_id]);
    CV_Leaf              leaf    = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexInfoList ti_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf(task->input, (LNK_LeafRef){ obj_idx, leaf_idx }, ti_list, 1);
    temp_end(temp);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_deep_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  for EachIndex(leaf_idx, debug_t->count) {
    if (task->input->debug_h_arr[obj_idx].v[leaf_idx] != 0) { continue; }
    Temp                 temp    = temp_begin(task->fixed_arenas[worker_id]);
    CV_Leaf              leaf    = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexInfoList ti_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf_deep(temp.arena, task->input, (LNK_LeafRef){ obj_idx, leaf_idx }, ti_list);
    temp_end(temp);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_dedup_task)
{
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  CV_DebugH      *debug_h = &task->input->debug_h_arr[obj_idx];
  ProfBeginDynamic("dedup in obj 0x%x (%S) leaf count %u", obj_idx, task->input->obj_arr[obj_idx]->path, debug_t->count);

  LNK_LeafRef *bucket = 0;
  for EachIndex(leaf_idx, debug_t->count) {
    // read leaf header
    CV_LeafHeader *header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);

    // read leaf kind
    CV_LeafKind kind = memory_read16(MemberFromPtr(CV_LeafHeader, header, kind));

    // alloc new bucket and assign type ref
    if (bucket == 0) { bucket = push_array_no_zero(arena, LNK_LeafRef, 1); }
    *bucket = (LNK_LeafRef){ .obj_idx = obj_idx, .leaf_idx = leaf_idx };

    CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(kind);
    LNK_LeafHashTable  *leaf_ht     = &task->leaf_ht_arr[leaf_source];

    {
      B32 is_inserted_or_updated = 0;
      U64 best_idx               = debug_h->v[leaf_idx] % leaf_ht->cap;
      U64 idx                    = best_idx;
      U64 loop_count = 0;
      do {
        retry:;
        LNK_LeafRef *curr_bucket = leaf_ht->bucket_arr[idx];
        if (curr_bucket == 0) {
          LNK_LeafRef *compare_bucket = ins_atomic_ptr_eval_cond_assign(&leaf_ht->bucket_arr[idx], bucket, curr_bucket);
          if (compare_bucket == curr_bucket) {
            // success, bucket was inserted
            bucket = 0;
            is_inserted_or_updated = 1;
            break;
          }
          // another thread took the bucket...
          goto retry;
        } else if (lnk_match_leaf_ref(task->input, *curr_bucket, *bucket)) {
          int leaf_cmp = lnk_leaf_ref_compare(*curr_bucket, *bucket);

          if (leaf_cmp <= 0) {
            // are we inserting bucket that was already inserterd?
            Assert(leaf_cmp < 0);
            is_inserted_or_updated = 1;
            // don't need to update, more recent leaf is in the bucket
            break;
          }

          LNK_LeafRef *compare_bucket = ins_atomic_ptr_eval_cond_assign(&leaf_ht->bucket_arr[idx], bucket, curr_bucket);
          if (compare_bucket == curr_bucket) {
            // reuse replaced bucket
            bucket = compare_bucket;
            is_inserted_or_updated = 1;
            break;
          }

          // another thread took the bucket...
          goto retry;
        }

        // advance
        idx = ((idx + 1) == leaf_ht->cap ? 0 : (idx + 1));
      } while (idx != best_idx);
      Assert(is_inserted_or_updated);
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_present_buckets_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task = raw_task;
  LNK_LeafHashTable *ht   = &task->leaf_ht_arr[task->ti_source];

  for EachInRange(bucket_idx, task->ranges[task_id]) {
    if (ht->bucket_arr[bucket_idx] != 0) {
      task->counts[task->ti_source][task_id] += 1;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_get_present_buckets_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task = raw_task;

  U64                cursor           = task->offsets[task->ti_source][task_id];
  LNK_LeafHashTable *ht               = &task->leaf_ht_arr[task->ti_source];
  LNK_LeafRefArray   unique_leaf_refs = task->unique_leaf_refs_arr[task->ti_source];

  for EachInRange(bucket_idx, task->ranges[task_id]) {
    if (ht->bucket_arr[bucket_idx]) {
      unique_leaf_refs.v[cursor++] = ht->bucket_arr[bucket_idx];
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_ref_histo_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task       = raw_task;
  Rng1U64         range      = task->ranges[task_id];
  U32            *counts_ptr = task->counts_arr[task_id];

  U32 obj_idx_bit_count_0 = task->obj_idx_bit_count_0;
  U32 obj_idx_bit_count_1 = task->obj_idx_bit_count_1;
  U32 obj_idx_bit_count_2 = task->obj_idx_bit_count_2;

  MemoryZeroTyped(task->counts_arr[task_id], task->counts_max);

  switch (task->pass_idx) {
  case 0: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit0 = BitExtract(bucket->leaf_idx, 10, 0);
      counts_ptr[leaf_digit0] += 1;
    }
  } break;
  case 1: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit1 = BitExtract(bucket->leaf_idx, 11, 10);
      counts_ptr[leaf_digit1] += 1;
    }
  } break;
  case 2: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit2 = BitExtract(bucket->leaf_idx, 11, 21);
      counts_ptr[leaf_digit2] += 1;
    }
  } break;

  case 3: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit0 = BitExtract(bucket->obj_idx, obj_idx_bit_count_0, 0);
      counts_ptr[digit0] += 1;
    }
  } break;
  case 4: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit1 = BitExtract(bucket->obj_idx, obj_idx_bit_count_1, obj_idx_bit_count_0);
      counts_ptr[digit1] += 1;
    }
  } break;
  case 5: {
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit2 = BitExtract(bucket->obj_idx, obj_idx_bit_count_2, obj_idx_bit_count_0 + obj_idx_bit_count_1);
      counts_ptr[digit2] += 1;
    }
  } break;
  default: InvalidPath;
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_loc_idx_radix_sort_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task                = raw_task;
  Rng1U64         range               = task->ranges[task_id];
  U32            *counts_ptr          = task->counts_arr[task_id];
  U32             obj_idx_bit_count_0 = task->obj_idx_bit_count_0;
  U32             obj_idx_bit_count_1 = task->obj_idx_bit_count_1;
  U32             obj_idx_bit_count_2 = task->obj_idx_bit_count_2;

  switch (task->pass_idx) {
  //
  // Sort items on leaf index
  //
  case 0: {
    ProfBegin("Leaf Sort Low");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit0 = BitExtract(bucket->leaf_idx, 10, 0);
      task->dst[counts_ptr[leaf_digit0]++] = bucket;
    }
    ProfEnd();
  } break;
  case 1: {
    ProfBegin("Leaf Sort Mid");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit1 = BitExtract(bucket->leaf_idx, 11, 10);
      task->dst[counts_ptr[leaf_digit1]++] = bucket;
    }
    ProfEnd();
  } break;
  case 2: {
    ProfBegin("Leaf Sort High");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 leaf_digit2 = BitExtract(bucket->leaf_idx, 11, 21);
      task->dst[counts_ptr[leaf_digit2]++] = bucket;
    }
    ProfEnd();
  } break;

  //
  // Sort items on obj and type server index
  //
  case 3: {
    ProfBegin("Obj Sort Low");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit0 = BitExtract(bucket->obj_idx, obj_idx_bit_count_0, 0);
      task->dst[counts_ptr[digit0]++] = bucket;
    }
    ProfEnd();
  } break;
  case 4: {
    ProfBegin("Obj Sort Mid");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit1 = BitExtract(bucket->obj_idx, obj_idx_bit_count_1, obj_idx_bit_count_0);
      task->dst[counts_ptr[digit1]++] = bucket;
    }
    ProfEnd();
  } break;
  case 5: {
    ProfBegin("Obj Sort High");
    for EachInRange(i, range) {
      LNK_LeafRef *bucket = task->src[i];
      U64 digit2 = BitExtract(bucket->obj_idx, obj_idx_bit_count_2, obj_idx_bit_count_0 + obj_idx_bit_count_1);
      Assert(counts_ptr[digit2] != max_U32);
      task->dst[counts_ptr[digit2]++] = bucket;
    }
    ProfEnd();
  } break;

  default: InvalidPath;
  }

  ProfEnd();
}

internal void
lnk_leaf_ref_array_sort(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafRefArray arr, U64 debug_t_count)
{
  Temp scratch = scratch_begin(0,0);

  ProfBeginDynamic("Leaf Sort [Leaf Count: %.*s]", str8_varg(str8_from_count(scratch.arena, arr.count)));


  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_assign_type_indices_task)
{
  LNK_MergeTypes *task  = raw_task;

  CV_TypeIndexSource  ti_source         = task->ti_source;
  LNK_LeafRefArray    unique_leaf_refs  = task->unique_leaf_refs_arr[ti_source];
  CV_TypeIndex        min_type_index    = task->min_type_indices[ti_source];
  U64                 assigned_type_cap = task->assigned_type_caps[ti_source];
  CV_TypeIndex       *assigned_type_ht  = task->assigned_type_hts[ti_source];

  for EachInRange(i, task->ranges[task_id]) {
    LNK_LeafRef  *leaf_ref   = unique_leaf_refs.v[i];
    CV_TypeIndex  type_index = min_type_index + i;

    U64 hash     = u64_hash_from_str8(str8_struct(leaf_ref));
    U64 best_idx = hash % assigned_type_cap;
    U64 idx      = best_idx;

    B32 is_inserted = 0;
    do {
      CV_TypeIndex curr_type_index = assigned_type_ht[idx];
      if (curr_type_index == 0) {
        CV_TypeIndex cmp_type_index = ins_atomic_u32_eval_cond_assign(&assigned_type_ht[idx], type_index, curr_type_index);
        if (cmp_type_index == curr_type_index) {
          is_inserted = 1;
          break;
        }
      }
      // advance
      idx = (idx + 1) == assigned_type_cap ? 0 : (idx + 1);
    } while (idx != best_idx);
    Assert(is_inserted);
  }
}

internal CV_TypeIndex
lnk_assigned_type_ht_search(U64 cap, CV_TypeIndex *ht, CV_TypeIndex min_type_index, LNK_LeafRefArray unique_leaf_refs, LNK_LeafRef *v, U64 hash)
{
  U64 best_idx = hash % cap;
  U64 idx      = best_idx;
  do {
    CV_TypeIndex type_index = ht[idx];
    if (type_index < min_type_index) { break; }

    U64          leaf_idx = type_index - min_type_index;
    LNK_LeafRef *compar   = unique_leaf_refs.v[leaf_idx];
    if (MemoryMatchStruct(compar,v)) { return type_index; }
      
    idx = (idx + 1) == cap ? 0 : (idx + 1);
  } while(idx != best_idx);

  InvalidPath;
  return 0;
}

internal void
lnk_fixup_cv_type_indices(LNK_MergeTypes *ctx, U32 obj_idx, String8 data, CV_TypeIndexInfoList ti_info_list)
{
  for EachNode(n, CV_TypeIndexInfo, ti_info_list.first) {
    CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(data, n->offset, sizeof(*ti_ptr));
    CV_TypeIndex  ti     = memory_read32(ti_ptr);

    // skip basic types
    if (ti < ctx->input->min_type_indices[n->source]) { continue; }

    U64        ts_idx  = ctx->input->obj_to_ts[obj_idx];
    CV_DebugT *debug_t = ts_idx ? &ctx->input->debug_t_arr[ctx->input->ts_obj_range.min + ts_idx] : &ctx->input->debug_t_arr[obj_idx];

    CV_TypeIndex final_ti = 0;
    if (contains_1u64(debug_t->ti_ranges[n->source], ti)) {
      LNK_LeafRef        leaf_ref   = lnk_leaf_ref_from_ti(ctx->input, obj_idx, n->source, ti);
      LNK_LeafHashTable *leaf_ht    = &ctx->leaf_ht_arr[n->source];
      LNK_LeafRef       *final_leaf = lnk_leaf_hash_table_search(leaf_ht, ctx->input, leaf_ref);
      U64                final_hash = u64_hash_from_str8(str8_struct(final_leaf));
      final_ti = lnk_assigned_type_ht_search(ctx->assigned_type_caps  [n->source],
                                             ctx->assigned_type_hts   [n->source],
                                             ctx->min_type_indices    [n->source],
                                             ctx->unique_leaf_refs_arr[n->source],
                                             final_leaf,
                                             final_hash);
    } else {
      Assert(0 && "invalid type index");
    }

    memory_write32(ti_ptr, final_ti);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_cv_patcher_symbols_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task = raw_task;
  for EachInRange(i, task->ranges[task_id]) {
    LNK_SymbolInput symbols = task->input->symbol_inputs[i];
    for EachNode(n, CV_SymbolNode, symbols.symbol_list->first) {
      Temp temp = temp_begin(task->fixed_arenas[task_id]);
      CV_TypeIndexInfoList ti_info_list = cv_get_symbol_type_index_offsets(temp.arena, n->data.kind, n->data.data);
      lnk_fixup_cv_type_indices(task, symbols.obj_idx, n->data.data, ti_info_list);
      temp_end(temp);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_cv_patcher_inlines_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task          = raw_task;
  U64             obj_idx       = task_id;
  String8List     inlinee_lines = cv_sub_section_from_debug_s(task->input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  for EachNode(inline_data_n, String8Node, inlinee_lines.first) {
    Temp temp = temp_begin(task->fixed_arenas[worker_id]);
    CV_TypeIndexInfoList ti_info_list = cv_get_inlinee_type_index_offsets(temp.arena, inline_data_n->string);
    lnk_fixup_cv_type_indices(task, obj_idx, inline_data_n->string, ti_info_list);
    temp_end(temp);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_cv_patcher_leaves_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task = raw_task;
  for EachInRange(leaf_ref_idx, task->ranges[task_id]) {
    Temp temp = temp_begin(task->fixed_arenas[task_id]);
    LNK_LeafRef          *patch        = task->unique_leaf_refs_arr[task->ti_source].v[leaf_ref_idx];
    CV_DebugT            *debug_t      = &task->input->debug_t_arr[patch->obj_idx];
    CV_Leaf               leaf         = cv_debug_t_get_leaf(debug_t, patch->leaf_idx);
    CV_TypeIndexInfoList  ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_fixup_cv_type_indices(task, patch->obj_idx, leaf.data, ti_info_list);
    temp_end(temp);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_unbucket_raw_leaves_task)
{
  LNK_MergeTypes *task = raw_task;
  for EachInRange(i, task->ranges[task_id]) {
    LNK_LeafRef  leaf_ref = *task->unique_leaf_refs_arr[task->ti_source].v[i];
    CV_DebugT   *debug_t  = &task->input->debug_t_arr[leaf_ref.obj_idx];
    String8      raw_leaf = cv_debug_t_get_raw_leaf(debug_t, leaf_ref.leaf_idx);
    task->result.v[task->ti_source][i] = raw_leaf.str;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_fixup_symbols_task)
{
  LNK_MergeTypes *task = raw_task;

  U64          leaf_count_ipi = task->result.count    [CV_TypeIndexSource_IPI];
  U8         **leaf_arr_ipi   = task->result.v        [CV_TypeIndexSource_IPI];
  CV_TypeIndex min_ti_ipi     = task->min_type_indices[CV_TypeIndexSource_IPI];

  for EachNode(symnode, CV_SymbolNode, task->input->symbol_inputs[task_id].symbol_list->first) {
    CV_Symbol *symbol = &symnode->data;

    // convert symbol to final type
    switch (symbol->kind) {
    case CV_SymKind_LPROC32_ID:     symbol->kind = CV_SymKind_LPROC32;     goto fixup_id;
    case CV_SymKind_GPROC32_ID:     symbol->kind = CV_SymKind_GPROC32;     goto fixup_id;
    case CV_SymKind_LPROC32_DPC_ID: symbol->kind = CV_SymKind_LPROC32_DPC; goto fixup_id;
    case CV_SymKind_LPROCMIPS_ID:   symbol->kind = CV_SymKind_LPROCMIPS;   goto fixup_id;
    case CV_SymKind_GPROCMIPS_ID:   symbol->kind = CV_SymKind_GPROCMIPS;   goto fixup_id;
    case CV_SymKind_LPROCIA64_ID:   symbol->kind = CV_SymKind_LPROCIA64;   goto fixup_id;
    case CV_SymKind_GPROCIA64_ID:   symbol->kind = CV_SymKind_GPROCIA64;   goto fixup_id;
    fixup_id:; {
      CV_SymProc32 *proc32 = (CV_SymProc32 *) symbol->data.str;
      if (proc32->itype < min_ti_ipi) {
        // TODO: in some cases destructors don't have a type, need a repro
        break;
      }

      if ((proc32->itype - min_ti_ipi) > leaf_count_ipi) {
        Assert("TODO: error handle corrupted type index");
        break;
      }

      U64     leaf_idx  = proc32->itype - min_ti_ipi;
      String8 leaf_data = str8(leaf_arr_ipi[leaf_idx], max_U64);

      CV_Leaf leaf;
      cv_read_leaf(leaf_data, 0, 1, &leaf);

      U64 min_leaf_size = cv_header_struct_size_from_leaf_kind(leaf.kind);
      if (min_leaf_size > leaf.data.size) {
        Assert(!"TODO: error handle corrupt leaf");
        break;
      }

      if (leaf.kind == CV_LeafKind_FUNC_ID) {
        proc32->itype = ((CV_LeafFuncId *) leaf.data.str)->itype;
      } else if (leaf.kind == CV_LeafKind_MFUNC_ID) {
        proc32->itype = ((CV_LeafMFuncId *) leaf.data.str)->itype;
      } else {
        Assert(!"TODO: erorr handle unexpected leaf type");
        break;
      }
    } break;
    case CV_SymKind_PROC_ID_END: symbol->kind = CV_SymKind_END; break;
    }
  }
}

internal LNK_MergedTypes
lnk_merge_types(TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input)
{
  ProfBeginFunction();
  Temp scratch = temp_begin(lnk_get_huge_arena());

  LNK_MergeTypes task = { .input = input, };
  U64 max_ti_list_size = sizeof(CV_TypeIndexInfo) * (max_U16 / sizeof(CV_TypeIndex));
  task.fixed_arenas = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, max_ti_list_size, max_ti_list_size);

  ProfBegin("Produce Hashes");
  {
    ProfBegin("Alloc Hashes");
    U32Array indices[] = {
      input->debug_p_indices,
      input->int_obj_indices,
      input->type_server_indices,
    };
    for EachElement(i, indices) {
      for EachIndex(k, indices[i].count) {
        U64        obj_idx = indices[i].v[k];
        CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
        CV_DebugH *debug_h = &input->debug_h_arr[obj_idx];
        debug_h->count = debug_t->count;
        debug_h->v     = push_array(scratch.arena, U64, debug_h->count);
      }
    }
    ProfEnd();
       
    // hash .debug$P first so we can mix in hashes for precompiled sub leaves when hashing leaves in .debug$T
    ProfBegin("Hash");
    task.indices = input->debug_p_indices;
    ProfScope(".debug$P [Count: %llu]", input->debug_p_indices.count)
      tp_for_parallel(tp, 0, task.indices.count, lnk_hash_debug_t_task, &task);

    task.indices = input->int_obj_indices;
    ProfScope(".debug$T [Count: %.*s]", str8_varg(str8_from_count(scratch.arena, task.indices.count)))
      tp_for_parallel(tp, 0, task.indices.count, lnk_hash_debug_t_task, &task);

    task.indices = input->type_server_indices;
    ProfScope("Type Servers [Count: %.*s]", str8_varg(str8_from_count(scratch.arena, task.indices.count)));
      tp_for_parallel(tp, 0, task.indices.count, lnk_hash_debug_t_deep_task, &task);
    ProfEnd();

#if BUILD_DEBUG
    for EachIndex(i, input->count) {
      for EachIndex(k, input->debug_h_arr[i].count) {
        Assert(input->debug_h_arr[i].v[k] != 0 &&
               input->debug_h_arr[i].v[k] != 1);
      }
    }
#endif

    // for external objs wire hash sections to type servers hashes
    for EachIndex(i, input->ext_obj_indices.count) {
      U64 dst_obj_idx = input->ext_obj_indices.v[i];
      U64 src_obj_idx = input->obj_to_ts[dst_obj_idx];
      CV_DebugH *dst_debug_h = &input->debug_h_arr[dst_obj_idx];
      CV_DebugH *src_debug_h = &input->debug_h_arr[src_obj_idx];
      *dst_debug_h = *src_debug_h;
    }
  }
  ProfEnd();

  ProfBegin("Leaf Hash Table Init");
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    U64 total_count = 0;
    for EachIndex(obj_idx, input->count) { total_count += input->debug_t_arr[obj_idx].source_counts[ti_source]; }

    task.leaf_ht_arr[ti_source].cap = total_count;
    task.leaf_ht_arr[ti_source].cap = 1 + ((task.leaf_ht_arr[ti_source].cap * 13) / 10); // * 1.3
    task.leaf_ht_arr[ti_source].bucket_arr = push_array(scratch.arena, LNK_LeafRef *, task.leaf_ht_arr[ti_source].cap);

#if PROFILE_TELEMETRY
    tmMessage(0, TMMF_ICON_NOTE, "%.*s Bucket Count: %.*s", str8_varg(cv_string_from_type_index_source(ti_source)), str8_varg(str8_from_count(scratch.arena, task.leaf_ht_arr[ti_source].cap)));
#endif
  }
  ProfEnd();

  ProfBegin("Leaf Dedup");
  task.indices = input->debug_p_indices;
  tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, ".debug$P");

  task.indices = input->int_obj_indices;
  tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, ".debug$T");

  task.indices = input->type_server_indices;
  tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, "Type Servers");
  ProfEnd();

  ProfBegin("Extract present buckets from the leaf hash tables");

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    task.ti_source          = ti_source;
    task.counts[ti_source]  = push_array(scratch.arena, U64, tp->worker_count);
    task.ranges             = tp_divide_work(scratch.arena, task.leaf_ht_arr[ti_source].cap, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_count_present_buckets_task, &task, "Count present buckets");

    task.unique_leaf_refs_arr[ti_source].count = sum_array_u64(tp->worker_count, task.counts[ti_source]);
    task.unique_leaf_refs_arr[ti_source].v     = push_array_no_zero(scratch.arena, LNK_LeafRef *, task.unique_leaf_refs_arr[ti_source].count);
    task.offsets[ti_source]                    = offsets_from_counts_array_u64(scratch.arena, task.counts[ti_source], tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_get_present_buckets_task, &task, "Copy present buckets");

    // sort output leaves based on { location index, leaf index } to guarantee determinism
    {
      LNK_LeafRefArray arr = task.unique_leaf_refs_arr[ti_source];
      if (arr.count > 140000) {
        ProfBegin("Radix");

        U32 obj_idx_max_bits = 32 - clz32(input->count);

        task.obj_idx_bit_count_0   = Clamp(0, (S32)obj_idx_max_bits - 21, 11);
        task.obj_idx_bit_count_1   = Clamp(0, (S32)obj_idx_max_bits - 10, 11);
        task.obj_idx_bit_count_2   = Clamp(0, (S32)obj_idx_max_bits,      10);
        task.counts_max            = (1 << 11);
        task.ranges                = tp_divide_work(scratch.arena, arr.count, tp->worker_count);
        task.dst                   = push_array_no_zero(scratch.arena, LNK_LeafRef *, arr.count);
        task.src                   = arr.v;

        ProfBegin("Push Counts");
        task.counts_arr = push_array_no_zero(scratch.arena, U32 *, tp->worker_count);
        for (U64 i = 0; i < tp->worker_count; ++i) {
          // zero-out happens in histogram step
          task.counts_arr[i] = push_array_no_zero(scratch.arena, U32, task.counts_max);
        }
        ProfEnd();

        for (task.pass_idx = 0; task.pass_idx < 6; ++task.pass_idx) {
          ProfBeginDynamic("Pass: %u", task.pass_idx);

          ProfBegin("Histo");
          tp_for_parallel(tp, 0, tp->worker_count, lnk_leaf_ref_histo_task, &task);
          ProfEnd();

          B32 is_range_not_empty = 0;
          for (U64 task_id = 0; task_id < tp->worker_count; ++task_id) {
            is_range_not_empty = task.counts_arr[task_id][0] != dim_1u64(task.ranges[task_id]);
            if (is_range_not_empty) {
              break;
            }
          }

          ProfBegin("Counts -> Offsets");
          {
            U64 digit_cursor = 0;
            for EachIndex(digit_idx, task.counts_max) {
              for EachIndex(task_id, tp->worker_count) {
                U64 count = task.counts_arr[task_id][digit_idx];
                task.counts_arr[task_id][digit_idx] = digit_cursor;
                digit_cursor += count;
              }
            }
            Assert(digit_cursor == arr.count);
          }
          ProfEnd();

          ProfBegin("Sort");
          tp_for_parallel(tp, 0, tp->worker_count, lnk_loc_idx_radix_sort_task, &task);
          Swap(LNK_LeafRef **, task.src, task.dst);
          ProfEnd();

          ProfEnd();
        }

        if (task.src != arr.v) {
          MemoryCopyTyped(arr.v, task.dst, arr.count);
        }

        ProfEnd();
      } else {
        ProfBegin("Radsort");
        radsort(arr.v, arr.count, lnk_leaf_ref_is_before);
        ProfEnd();
      }

#if 0
      for (U64 i = 1; i < arr.count; ++i) {
        AssertAlways(arr.v[i-1]->obj_idx <= arr.v[i]->obj_idx);
        if (arr.v[i-1]->obj_idx == arr.v[i]->obj_idx) {
          AssertAlways(arr.v[i-1]->obj_idx <= arr.v[i]->obj_idx);
        }
      }
#endif
    }
  }

  #if PROFILE_TELEMETRY
  tmMessage(0, TMMF_ICON_NOTE, "TPI Count: %.*s", str8_varg(str8_from_count(scratch.arena, task.unique_leaf_refs_arr[CV_TypeIndexSource_TPI].count)));
  tmMessage(0, TMMF_ICON_NOTE, "IPI Count: %.*s", str8_varg(str8_from_count(scratch.arena, task.unique_leaf_refs_arr[CV_TypeIndexSource_IPI].count)));
  #endif

  ProfEnd();

  ProfBegin("Assign type indices");
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    task.ti_source                     = ti_source;
    task.assigned_type_caps[ti_source] = (task.unique_leaf_refs_arr[ti_source].count * 13) / 10;
    task.assigned_type_hts [ti_source] = push_array(scratch.arena, CV_TypeIndex, task.assigned_type_caps[ti_source]);
    task.min_type_indices  [ti_source] = CV_MinComplexTypeIndex;
    task.ranges                        = tp_divide_work(scratch.arena, task.unique_leaf_refs_arr[ti_source].count, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_assign_type_indices_task, &task, "Assign Type Indices");
  }
  ProfEnd();

  ProfBegin("Patch Type Indices");
  {
    task.ranges = tp_divide_work(scratch.arena, input->symbol_input_count, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_cv_patcher_symbols_task, &task, "Symbols");

    task.ranges      = 0;
    task.debug_s_arr = input->debug_s_arr;
    tp_for_parallel_prof(tp, 0, input->count, lnk_cv_patcher_inlines_task, &task, "Inlines");

    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      task.ti_source = ti_source;
      task.ranges    = tp_divide_work(scratch.arena, task.unique_leaf_refs_arr[ti_source].count, tp->worker_count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_cv_patcher_leaves_task, &task, "Leaves");
    }
  }
  ProfEnd();

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    LNK_LeafRefArray unique_leaf_refs = task.unique_leaf_refs_arr[ti_source];
    task.ti_source               = ti_source;
    task.result.count[ti_source] = unique_leaf_refs.count;
    task.result.v    [ti_source] = push_array(tp_temp->v[0], U8 *, unique_leaf_refs.count);
    task.ranges                  = tp_divide_work(scratch.arena, unique_leaf_refs.count, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, lnk_unbucket_raw_leaves_task, &task);
  }

  tp_for_parallel_prof(tp, 0, input->symbol_input_count, lnk_fixup_symbols_task, &task, "fixup type indices in symbols");

  MemoryCopyTyped(task.result.min_type_indices, input->min_type_indices, CV_TypeIndexSource_COUNT);

  temp_end(scratch);
  ProfEnd();
  return task.result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_type_names_with_hashes_lenient_task)
{
  ProfBeginFunction();

  LNK_TypeNameReplacer *task        = raw_task;
  Rng1U64               range       = task->ranges[task_id];
  U64                   leaf_count  = task->leaf_count;
  U8                  **leaf_arr    = task->leaf_arr;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64  hash_max_chars = hash_length*2;
  char temp[128];

  for EachInRange(leaf_idx, range) {
    CV_Leaf leaf = cv_leaf_from_ptr(leaf_arr[leaf_idx]);
    if (leaf.kind == CV_LeafKind_STRUCTURE || leaf.kind == CV_LeafKind_CLASS) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);

      if ((udt_info.props & CV_TypeProp_HasUniqueName) &&
           udt_info.unique_name.size > hash_max_chars &&
           udt_info.name.size > hash_max_chars) {
        // hash unique name
        U64 name_hash;
        blake3_hasher hasher; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.unique_name.str, udt_info.unique_name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> unique name map
        if (make_map) {
          str8_list_pushf(map_arena, map, "%llx %S\n", name_hash, str8_varg(udt_info.unique_name));
        }

        // parse leaf size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        String8 lambda_prefix = str8_lit("<lambda_");
        U64     colon_pos     = str8_find_needle_reverse(udt_info.name, 0, lambda_prefix, 0);
        B32     is_lambda     = colon_pos != 0;

        if (is_lambda) {
          U64 size = raddbg_snprintf(temp, sizeof(temp), "%llx", name_hash);
          Assert(size < udt_info.name.size);
          Assert(size < udt_info.unique_name.size);
          MemoryCopy(udt_info.name.str, temp, size+1);
          MemoryCopy(udt_info.name.str+size+1, temp, size+1);
          udt_info.name.size        = size;
          udt_info.unique_name.size = size;

          // update leaf header
          U64 new_size = sizeof(CV_LeafKind) +
                                sizeof(CV_LeafStruct) +
                                numeric_size +
                                udt_info.name.size + 1 +
                                udt_info.unique_name.size + 1;
          CV_LeafHeader *header = (CV_LeafHeader *)leaf_arr[leaf_idx];
          Assert(new_size <= max_U16);
          memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);
        } else {
          // replace uniuqe type name with hash
          udt_info.unique_name.str  = udt_info.name.str + udt_info.name.size + 1;
          udt_info.unique_name.size = raddbg_snprintf((char *)udt_info.unique_name.str, udt_info.unique_name.size, "%llx", name_hash);

          // update leaf header
          U64 new_size = sizeof(CV_LeafKind) +
                                sizeof(CV_LeafStruct) +
                                numeric_size +
                                udt_info.name.size + 1 +
                                udt_info.unique_name.size + 1;
          CV_LeafHeader *header = (CV_LeafHeader *)leaf_arr[leaf_idx];
          Assert(new_size <= max_U16);
          memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);
        }
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_type_names_with_hashes_full_task)
{
  ProfBeginFunction();

  LNK_TypeNameReplacer *task        = raw_task;
  Rng1U64               range       = task->ranges[task_id];
  U64                   leaf_count  = task->leaf_count;
  U8                  **leaf_arr    = task->leaf_arr;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64 hash_max_chars = hash_length*2;

  for EachInRange(leaf_idx, range) {
    CV_Leaf leaf = cv_leaf_from_ptr(leaf_arr[leaf_idx]);
    if (leaf.kind == CV_LeafKind_STRUCTURE || leaf.kind == CV_LeafKind_CLASS) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);

      if (udt_info.name.size > hash_max_chars) {
        // pick name to hash
        String8 name;
        if (udt_info.props & CV_TypeProp_HasUniqueName) {
          name = udt_info.unique_name;
        } else {
          name = udt_info.name;
        }

        // hash name
        U64 name_hash;
        blake3_hasher hasher = {0}; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.name.str, udt_info.name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> name map
        if (make_map) {
          str8_list_pushf(map_arena, map, "%llx %S\n", name_hash, name);
        }

        // replace name with hash
        udt_info.name.size = raddbg_snprintf((char *)udt_info.name.str, udt_info.name.size, "%llx", name_hash);

        // parse struct size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        // update header
        U64            new_size = sizeof(CV_LeafKind) + sizeof(CV_LeafStruct) + numeric_size + udt_info.name.size + 1;
        CV_LeafHeader *header   = (CV_LeafHeader *)leaf_arr[leaf_idx];
        Assert(new_size <= max_U16);
        memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);

        // discard unique name
        CV_LeafStruct *lf = (CV_LeafStruct *)(header + 1);
        lf->props &= ~CV_TypeProp_HasUniqueName;
      }
    }
  }

  ProfEnd();
}

internal void
lnk_replace_type_names_with_hashes(TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  // init task context
  LNK_TypeNameReplacer task = {0};
  task.leaf_count           = leaf_count;
  task.leaf_arr             = leaf_arr;
  task.ranges               = tp_divide_work(scratch.arena, leaf_count, tp->worker_count);
  task.hash_length          = Clamp(1, hash_length, 16);

  if (map_name.size > 0) {
    task.make_map  = 1;
    task.map_arena = tp_arena_alloc(tp);
    task.maps      = push_array(scratch.arena, String8List, tp->worker_count);
  }

  // pick task function
  TP_TaskFunc *func = 0;
  switch (mode) {
  case LNK_TypeNameHashMode_Null: 
  case LNK_TypeNameHashMode_None:
    break;

  case LNK_TypeNameHashMode_Lenient: func = lnk_replace_type_names_with_hashes_lenient_task; break;
  case LNK_TypeNameHashMode_Full:    func = lnk_replace_type_names_with_hashes_full_task;    break;
  }

  // run task
  tp_for_parallel(tp, arena, tp->worker_count, func, &task);

  // optionally write out map file 
  if (task.make_map) {
    String8List map = {0};
    str8_list_concat_in_place_array(&map, task.maps, tp->worker_count);
    lnk_write_data_list_to_file_path(map_name, str8_zero(), map);
    tp_arena_release(&task.map_arena);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_filter_out_gsi_symbols_task)
{
  U64                         obj_idx        = task_id;
  LNK_ProcessSymDataTaskData *task           = raw_task;
  CV_SymbolList              *gsi_list       = &task->gsi_list_arr[obj_idx];
  CV_SymbolListArray          parsed_symbols = task->parsed_symbols[obj_idx];

  CV_SymbolList global_list  = {0};
  CV_SymbolList typedef_list = {0};
  for (U64 i = 0; i < parsed_symbols.count; ++i) {
    CV_SymbolList *list = &parsed_symbols.v[i];
    U64 depth = 0;
    for (CV_SymbolNode *curr = list->first, *next; curr != 0; curr = next) {
      next = curr->next;

      if (cv_is_global_symbol(curr->data.kind)) {
        cv_symbol_list_remove_node(list, curr);
        cv_symbol_list_push_node(&global_list, curr);
      } else if (cv_is_typedef(curr->data.kind)) {
        if (depth == 0) {
          cv_symbol_list_remove_node(list, curr);
          cv_symbol_list_push_node(&typedef_list, curr);
        }
      }
      // Undocumented symbol that appears only in objs.
      //  MSVC removes these symbols from output.
      //
      //  LLD-link replaces symbol with S_SKIP:
      //  https://github.com/llvm/llvm-project/blob/main/lld/COFF/PDB.cpp#L575
      else if (curr->data.kind == 0x1176) {
        cv_symbol_list_remove_node(list, curr);
      }

      if (cv_is_scope_symbol(curr->data.kind)) {
        ++depth;
      } else if (cv_is_end_symbol(curr->data.kind)) {
        Assert(depth > 0);
        --depth;
      }
    }
  }
 
  // collect GSI symbols
  Assert(gsi_list->count == 0);
  cv_symbol_list_concat_in_place(gsi_list, &global_list);
  cv_symbol_list_concat_in_place(gsi_list, &typedef_list);
}

internal
THREAD_POOL_TASK_FUNC(lnk_make_proc_refs_task)
{
  ProfBeginFunction();

  U64                         obj_idx        = task_id;
  LNK_ProcessSymDataTaskData *task           = raw_task;
  PDB_DbiModule              *mod            = task->mod_arr[obj_idx];
  CV_SymbolList              *gsi_list       = &task->gsi_list_arr[obj_idx];
  CV_SymbolListArray          parsed_symbols = task->parsed_symbols[obj_idx];

  for (U64 i = 0; i < parsed_symbols.count; ++i) {
    CV_SymbolList list      = parsed_symbols.v[i];
    CV_SymbolList proc_refs = cv_make_proc_refs(arena, mod->imod, list);
    cv_symbol_list_concat_in_place(gsi_list, &proc_refs);
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_fixup_symbol_offsets_task)
{
  ProfBeginFunction();

  LNK_ProcessSymDataTaskData *task    = raw_task;
  CV_SymbolListArray          symbols = task->parsed_symbols[task_id];

  // fixup symbol offsets and estimate symbol data size
  U64 size = sizeof(CV_Signature);
  for EachIndex(i, symbols.count) {
    size += cv_patch_symbol_tree_offsets(symbols.v[i], size, PDB_SYMBOL_ALIGN);
  }
  task->serialized_symbol_data_sizes[task_id] = AlignPow2(size, CV_C13SubSectionAlign);

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_process_c13_data_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena,1);

  U64                     obj_idx = task_id;
  LNK_ProcessC13DataTask *task    = raw_task;
  CV_DebugS               debug_s = task->debug_s_arr[obj_idx];

  // parse checksum data
  String8List     checksum_data = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
  CV_ChecksumList checksum_list = cv_c13_parse_checksum_data_list(scratch.arena, checksum_data);

  // get strings sub-section
  String8 string_data = cv_string_table_from_debug_s(debug_s);

  // collect source file names from checksum headers
  String8List source_file_names_list = cv_c13_collect_source_file_names(arena, checksum_list, string_data);

  // relocate checksum data 
  cv_c13_patch_string_offsets_in_checksum_list(checksum_list, string_data, task->string_data_base_offset, task->string_ht);

  // store for later pass
  task->source_file_names_list_arr[obj_idx] = source_file_names_list;

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_write_module_data_task)
{
  LNK_WriteModuleDataTask *task = raw_task;

  U64                 obj_idx        = task_id;
  PDB_DbiModule      *mod            = task->mod_arr                     [obj_idx];
  CV_DebugS          *debug_s        = &task->debug_s_arr                [obj_idx];
  CV_SymbolListArray  parsed_symbols = task->parsed_symbols              [obj_idx];
  String8List         globrefs       = task->globrefs_arr                [obj_idx];
  U64                 sym_data_size  = task->serialized_symbol_data_sizes[obj_idx];

  MSF_UInt stream_cap = msf_stream_get_cap(task->msf, mod->sn);

  // write symbols
  if (sym_data_size > 0) {
    Temp scratch = scratch_begin(&arena, 1);

    U64 temp_max  = max_U16;
    U64 temp_size = 0;
    U8 *temp      = push_array(scratch.arena, U8, temp_max);

    msf_stream_write_u32(task->msf, mod->sn, CV_Signature_C13);

    for EachIndex(i, parsed_symbols.count) {
      for EachNode(symbol_n, CV_SymbolNode, parsed_symbols.v[i].first) {
        U64 symbol_size = cv_size_from_symbol(&symbol_n->data, PDB_SYMBOL_ALIGN);

        // flush temp
        if (temp_size + symbol_size > temp_max) {
          Assert(temp_size <= (stream_cap - msf_stream_get_pos(task->msf, mod->sn)));
          msf_stream_write(task->msf, mod->sn, temp, temp_size);
          temp_size = 0;
        }

        U64 serial_size = cv_write_symbol(temp, temp_size, temp_max, &symbol_n->data, PDB_SYMBOL_ALIGN);
        Assert(serial_size == symbol_size);
        temp_size += serial_size;
      }
    }

    // flush remaining temp
    Assert(temp_size <= (stream_cap - msf_stream_get_pos(task->msf, mod->sn)));
    msf_stream_write(task->msf, mod->sn, temp, temp_size);
    msf_stream_align(task->msf, mod->sn, CV_C13SubSectionAlign);

    U64 size = msf_stream_get_pos(task->msf, mod->sn);
    // assert our symbol size estimate was correct
    Assert(sym_data_size == msf_stream_get_pos(task->msf, mod->sn));

    scratch_end(scratch);
  }

  // write rest of c13 data
  U64 c13_data_size;
  {
    U64 c13_start_pos = msf_stream_get_pos(task->msf, mod->sn);

    for EachElement(layout_idx, debug_s->data_list) {
      if (layout_idx == CV_C13SubSectionIdxKind_Lines || layout_idx == CV_C13SubSectionIdxKind_Symbols) { continue; }

      CV_C13SubSectionKind  kind = cv_c13_sub_section_kind_from_idx(layout_idx);
      String8List          *data = cv_sub_section_ptr_from_debug_s(debug_s, kind);
      if (data->total_size == 0) { continue; }

      Assert(AlignPow2(sizeof(U32)*2 + data->total_size, PDB_SYMBOL_ALIGN) <= (stream_cap - msf_stream_get_pos(task->msf, mod->sn)));
      msf_stream_write_u32 (task->msf, mod->sn, kind);
      msf_stream_write_u32 (task->msf, mod->sn, safe_cast_u32(data->total_size));
      msf_stream_write_list(task->msf, mod->sn, *data);
      msf_stream_align(task->msf, mod->sn, CV_C13SubSectionAlign);
    }

    String8List *line_data = cv_sub_section_ptr_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);
    for EachNode(line_n, String8Node, line_data->first) {
      if (line_n->string.size == 0) { continue; }

      Assert(AlignPow2(sizeof(U32)*2 + line_n->string.size, PDB_SYMBOL_ALIGN) <= (stream_cap - msf_stream_get_pos(task->msf, mod->sn)));
      msf_stream_write_u32   (task->msf, mod->sn, CV_C13SubSectionKind_Lines);
      msf_stream_write_u32   (task->msf, mod->sn, safe_cast_u32(line_n->string.size));
      msf_stream_write_string(task->msf, mod->sn, line_n->string);
      msf_stream_align(task->msf, mod->sn, CV_C13SubSectionAlign);
    }

    U64 c13_end_pos = msf_stream_get_pos(task->msf, mod->sn);
    c13_data_size = c13_end_pos - c13_start_pos;
    Assert(c13_data_size == cv_size_from_debug_s(debug_s, CV_C13SubSectionAlign));
  }

  // write global refs
  Assert(globrefs.total_size <= (stream_cap - msf_stream_get_pos(task->msf, mod->sn)));
  msf_stream_write_list(task->msf, mod->sn, globrefs);

  // update module data sizes
  mod->sym_data_size = safe_cast_u32(sym_data_size);
  mod->c11_data_size = 0;
  mod->c13_data_size = safe_cast_u32(c13_data_size);
  mod->globrefs_size = safe_cast_u32(globrefs.total_size);

  // make stream has enough memory so it doens't trigger memory allocations in MSF
  // during multi-thread write
  AssertAlways(task->mod_sizes[obj_idx] == msf_stream_get_pos(task->msf, mod->sn));
}

internal
THREAD_POOL_TASK_FUNC(lnk_cv_symbol_ptr_array_hasher)
{
  LNK_CvSymbolPtrArrayHasher *task  = raw_task;
  Rng1U64                     range = task->range_arr[task_id];
  for (U64 symbol_idx = range.min; symbol_idx < range.max; ++symbol_idx) {
    task->hash_arr[symbol_idx] = XXH3_64bits(task->arr[symbol_idx]->data.data.str, task->arr[symbol_idx]->data.data.size);
  }
}

internal U64 *
lnk_hash_cv_symbol_ptr_arr(TP_Context *tp, Arena *arena, CV_SymbolPtrArray arr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_CvSymbolPtrArrayHasher task = {0};
  task.hash_arr                   = push_array_no_zero(arena, U64, arr.count);
  task.arr                        = arr.v;
  task.range_arr = tp_divide_work(scratch.arena, arr.count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_cv_symbol_ptr_array_hasher, &task);

  scratch_end(scratch);
  ProfEnd();
  return task.hash_arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_push_dbi_sec_contrib_task)
{
  // TODO: use chunked lists for SC
  // TODO: put back unused sc nodes
  // TODO: compute CRC for relocations

  U64                             obj_idx = task_id;
  LNK_PushDbiSecContribTaskData  *task    = raw_task;
  PDB_DbiModule                  *mod     = task->mod_arr[obj_idx];
  LNK_Obj                        *obj     = task->obj_arr[obj_idx];

  COFF_SectionHeader        *obj_section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  PDB_DbiSectionContribNode *sc_arr            = push_array_no_zero(arena, PDB_DbiSectionContribNode, obj->header.section_count_no_null);
  U64                        sc_count          = 0;
  
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *obj_sect_header = &obj_section_table[sect_idx];

    if (obj_sect_header->flags & COFF_SectionFlag_LnkInfo)   { continue; }
    if (obj_sect_header->flags & COFF_SectionFlag_LnkRemove) { continue; }
    if (obj_sect_header->flags & LNK_SECTION_FLAG_DEBUG)     { continue; }

    U64     sect_number;
    String8 sect_data;
    U32     sect_off;
    U32     data_crc;
    if (obj_sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
      if (obj_sect_header->vsize == 0) {
        continue;
      }
      sect_number = rng_1u64_array_bsearch(task->image_section_virt_ranges, obj_sect_header->voff);
      Assert(sect_number < task->image_section_virt_ranges.count);
      sect_data   = str8_zero();
      sect_off    = obj_sect_header->voff - task->image_section_virt_ranges.v[sect_number].min;
      data_crc    = 0;
    } else {
      if (obj_sect_header->fsize == 0) {
        continue;
      }
      sect_number = rng_1u64_array_bsearch(task->image_section_file_ranges, obj_sect_header->foff);
      Assert(sect_number < task->image_section_file_ranges.count);
      sect_data   = str8_substr(task->image_data, rng_1u64(obj_sect_header->foff, obj_sect_header->foff + obj_sect_header->fsize));
      sect_off    = obj_sect_header->foff - task->image_section_file_ranges.v[sect_number].min;
      data_crc    = update_crc32(0, sect_data.str, sect_data.size);
    }

    // fill out SC
    PDB_DbiSectionContribNode *sc = sc_arr + sc_count++;
    sc->data.base.sec             = (U16)sect_number;
    sc->data.base.pad0            = 0;
    sc->data.base.sec_off         = sect_off;
    sc->data.base.size            = obj_sect_header->vsize;
    sc->data.base.flags           = obj_sect_header->flags;
    sc->data.base.mod             = mod->imod;
    sc->data.base.pad1            = 0;
    sc->data.data_crc             = 0;
    sc->data.reloc_crc            = 0; 

    dbi_sec_contrib_list_push_node(&task->sc_list[obj_idx], sc);
  }

  // Mod1::fUpdateSecContrib
  if (sc_count > 0) {
    for (U64 sc_idx = 0; sc_idx < sc_count; ++sc_idx) {
      if (sc_arr[sc_idx].data.base.flags & COFF_SectionFlag_CntCode) {
        mod->first_sc = sc_arr[sc_idx].data;
        break;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_pdb_public_symbols_defined_task)
{
  ProfBeginFunction();

  LNK_BuildPublicSymbolsTask *task = raw_task;
  for (LNK_SymbolHashTrieChunk *chunk = task->chunk_lists[task_id].first; chunk != 0; chunk = chunk->next) {
    CV_SymbolNode *nodes    = push_array_no_zero(arena, CV_SymbolNode, chunk->count);
    U64            node_idx = 0;
    for EachIndex(i, chunk->count) {
      LNK_Symbol        *symbol        = chunk->v[i].symbol;
      LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
      COFF_ParsedSymbol  symbol_parsed = lnk_parsed_from_symbol(symbol);

      if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }

      COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
      if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

      CV_Pub32Flags flags = 0;
      if (COFF_SymbolType_IsFunc(symbol_parsed.type)) { flags |= CV_Pub32Flag_Function; }

      ISectOff sc             = lnk_sc_from_symbol(symbol);
      U16      symbol_isect16 = safe_cast_u16(sc.isect);
      U32      symbol_off32   = safe_cast_u32(sc.off);

      nodes[node_idx].data = cv_make_pub32(arena, flags, symbol_off32, symbol_isect16, symbol->name);
      cv_symbol_list_push_node(&task->pub_list_arr[task_id], &nodes[node_idx]);
      node_idx += 1;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_gsi_hash_cv_list_task)
{
  ProfBeginFunction();

  LNK_BuildPublicSymbolsTask *task  = raw_task;
  Rng1U64                     range = task->symbol_ranges[task_id];

  for (U64 symbol_idx = range.min; symbol_idx < range.max; ++symbol_idx) {
    CV_Symbol *symbol = &task->symbols.v[symbol_idx]->data;
    String8 name = cv_name_from_symbol(symbol->kind, symbol->data);
    task->hashes[symbol_idx] = gsi_hash(task->gsi, name);
  }

  ProfEnd();
}

internal void
lnk_build_pdb_public_symbols(TP_Context            *tp,
                             TP_Arena              *arena,
                             LNK_SymbolTable       *symtab,
                             PDB_PsiContext        *psi)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Defined");
  LNK_BuildPublicSymbolsTask task = {0};
  task.pub_list_arr = push_array(scratch.arena, CV_SymbolList, tp->worker_count);
  task.chunk_lists  = symtab->chunks;
  tp_for_parallel(tp, arena, tp->worker_count, lnk_build_pdb_public_symbols_defined_task, &task);
  ProfEnd();

  CV_SymbolPtrArray symbols = cv_symbol_ptr_array_from_list(scratch.arena, tp, tp->worker_count, task.pub_list_arr);

  ProfBegin("GSI Push");
  gsi_push_many_arr(tp, psi->gsi, symbols.count, symbols.v);
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
}

internal String8List
lnk_build_pdb(TP_Context               *tp,
              TP_Arena                 *tp_arena,
              String8                   image_data,
              LNK_Config               *config,
              LNK_SymbolTable          *symtab,
              U64                       obj_count,
              LNK_Obj                 **obj_arr,
              CV_DebugS                *debug_s_arr,
              U64                       symbol_input_count,
              LNK_SymbolInput          *symbol_inputs,
              CV_SymbolListArray       *parsed_symbols,
              LNK_MergedTypes           types)
{
  ProfBegin("PDB");
  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);
  Temp huge_arena_temp = temp_begin(lnk_get_huge_arena());

  PE_BinInfo           pe                        = pe_bin_info_from_data(scratch.arena, image_data);
  COFF_SectionHeader **image_section_table       = coff_section_table_from_data(scratch.arena, image_data, pe.section_table_range);
  U64                  image_section_table_count = pe.section_count+1;

  ProfBegin("Setup PDB Context");
  PDB_Context *pdb = pdb_alloc_(huge_arena_temp.arena, config->pdb_page_size, config->machine, config->time_stamp, config->age, config->guid);
  ProfEnd();

  // set min type indices
  for EachElement(ti_source, types.min_type_indices) { pdb->type_servers[ti_source]->ti_lo = types.min_type_indices[ti_source]; }

  // move patched type data
  //
  // leaf data is stored in g_file_arena which has linker's life-time
  // and this way we skip redundant leaf copy to the type server to make things faster
  pdb_type_server_push_parallel(tp, pdb->type_servers[CV_TypeIndexSource_IPI], types.count[CV_TypeIndexSource_IPI], types.v[CV_TypeIndexSource_IPI]);
  pdb_type_server_push_parallel(tp, pdb->type_servers[CV_TypeIndexSource_TPI], types.count[CV_TypeIndexSource_TPI], types.v[CV_TypeIndexSource_TPI]);

  ProfBegin("Collect Symbols for GSI");
  CV_SymbolList *gsi_list_arr = push_array(scratch.arena, CV_SymbolList, obj_count);
  {
    LNK_ProcessSymDataTaskData task = {0};
    task.gsi_list_arr               = gsi_list_arr;
    task.parsed_symbols             = parsed_symbols;
    tp_for_parallel(tp, 0, obj_count, lnk_filter_out_gsi_symbols_task, &task);
  }
  ProfEnd();

  ProfBegin("Reserve DBI Modules");
  PDB_DbiModule **mod_arr = push_array(tp_arena->v[0], PDB_DbiModule *, obj_count);
  for EachIndex(obj_idx, obj_count) {
    LNK_Obj *obj = obj_arr[obj_idx];
    mod_arr[obj_idx] = dbi_push_module(pdb->dbi, obj->path, lnk_obj_get_lib_path(obj));

    // we don't support symbol append
    Assert(mod_arr[obj_idx]->sn == MSF_INVALID_STREAM_NUMBER);
  }
  ProfEnd();

  ProfBegin("Build String Table");
  CV_StringHashTable string_ht = cv_dedup_string_tables(tp_arena, tp, obj_count, debug_s_arr);
  cv_string_hash_table_assign_buffer_offsets(tp, string_ht);
  U64 string_data_base_offset = pdb->info->strtab.size;
  pdb_strtab_add_cv_string_hash_table(&pdb->info->strtab, string_ht);
  ProfEnd();

  ProfBegin("Build Source Files List");
  {
    // push source files per module info
    LNK_ProcessC13DataTask task     = {0};
    task.debug_s_arr                = debug_s_arr;
    task.string_data_base_offset    = string_data_base_offset;
    task.source_file_names_list_arr = push_array_no_zero(tp_arena->v[0], String8List, obj_count);
    task.string_ht                  = string_ht;
    tp_for_parallel(tp, tp_arena, obj_count, lnk_process_c13_data_task, &task);

    for EachIndex(obj_idx, obj_count) {
      String8List source_file_list = str8_list_copy(pdb->dbi->arena, &task.source_file_names_list_arr[obj_idx]);
      str8_list_concat_in_place(&mod_arr[obj_idx]->source_file_list, &source_file_list);
    }

    // zero out string table sub section
    for EachIndex(obj_idx, obj_count) {
      MemoryZeroStruct(&debug_s_arr[obj_idx].data_list[CV_C13SubSectionIdxKind_StringTable]);
    }
  }
  ProfEnd();

  ProfBegin("Build DBI Modules");
  {
    TP_Temp temp = tp_temp_begin(tp_arena);
    {
      ProfBegin("Fixup Symbol Offsets");
      U64 *serialized_symbol_data_sizes = push_array(scratch.arena, U64, obj_count);
      tp_for_parallel(tp, tp_arena, obj_count, lnk_fixup_symbol_offsets_task, &(LNK_ProcessSymDataTaskData){ .parsed_symbols = parsed_symbols, .serialized_symbol_data_sizes = serialized_symbol_data_sizes });
      ProfEnd();

      // TODO: actually collect offsets and pass them here
      ProfBegin("Build Empty Global Reference Array");
      String8List *globrefs_arr = push_array(tp_arena->v[0], String8List, obj_count);
      for EachIndex(obj_idx, obj_count) {
        String8List *globrefs = &globrefs_arr[obj_idx];
        str8_serial_begin(tp_arena->v[0], globrefs);
        Assert(globrefs->total_size == 0);
        str8_serial_push_u32(tp_arena->v[0], globrefs, globrefs->total_size);
      }
      ProfEnd();

      // reserve memory for module streams
      ProfBegin("Reserve Modules Memory");
      U32 *mod_sizes = push_array(scratch.arena, U32, obj_count);
      for EachIndex(obj_idx, obj_count) {
        // compute number of bytes needed for module data
        U64 mod_size = 0;
        mod_size += cv_size_from_debug_s(&debug_s_arr[obj_idx], CV_C13SubSectionAlign);
        mod_size += serialized_symbol_data_sizes[obj_idx];
        mod_size += globrefs_arr[obj_idx].total_size;

        mod_sizes[obj_idx] = safe_cast_u32(mod_size);

        // allocate stream for module
        mod_arr[obj_idx]->sn = msf_stream_alloc_ex(pdb->msf, mod_sizes[obj_idx]);
      }
      ProfEnd();

      // copy data to module streams
      ProfBegin("Write Modules Data");
      LNK_WriteModuleDataTask write_module_data_task_data = {0};
      write_module_data_task_data.msf                          = pdb->msf;
      write_module_data_task_data.mod_arr                      = mod_arr;
      write_module_data_task_data.debug_s_arr                  = debug_s_arr;
      write_module_data_task_data.serialized_symbol_data_sizes = serialized_symbol_data_sizes;
      write_module_data_task_data.parsed_symbols               = parsed_symbols;
      write_module_data_task_data.globrefs_arr                 = globrefs_arr;
      write_module_data_task_data.mod_sizes                    = mod_sizes;
      tp_for_parallel(tp, 0, obj_count, lnk_write_module_data_task, &write_module_data_task_data);
      ProfEnd();
    }
    tp_temp_end(temp);
  }
  ProfEnd();

  ProfBegin("Make Proc Refs");
  {
    LNK_ProcessSymDataTaskData task = {0};
    task.mod_arr                    = mod_arr;
    task.gsi_list_arr               = gsi_list_arr;
    task.parsed_symbols             = parsed_symbols;
    tp_for_parallel(tp, tp_arena, obj_count, lnk_make_proc_refs_task, &task);
  }
  ProfEnd();

  ProfBegin("Push Global Symbols");
  {
    CV_SymbolPtrArray global_symbols = cv_symbol_ptr_array_from_list(scratch.arena, tp, obj_count, gsi_list_arr);
    cv_dedup_symbol_ptr_array(tp, &global_symbols);
    gsi_push_many_arr(tp, pdb->gsi, global_symbols.count, global_symbols.v);
  }
  ProfEnd();
  
  ProfBegin("Build DBI Section Headers");
  {
    for (U64 sect_idx = 1; sect_idx < image_section_table_count; sect_idx += 1) {
      dbi_push_section(pdb->dbi, image_section_table[sect_idx]);
    }
  }
  ProfEnd();

  ProfBegin("Build Section Contrib Map");
  {

    Rng1U64Array image_section_file_ranges = {0};
    image_section_file_ranges.count = 0;
    image_section_file_ranges.v     = push_array(scratch.arena, Rng1U64, image_section_table_count);
    Rng1U64Array image_section_virt_ranges = {0};
    image_section_virt_ranges.count = image_section_table_count;
    image_section_virt_ranges.v     = push_array(scratch.arena, Rng1U64, image_section_table_count);
    for (U64 i = 0; i < image_section_table_count; i += 1) {
      COFF_SectionHeader *sect_header = image_section_table[i];
      if (~sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
        image_section_file_ranges.v[image_section_file_ranges.count++] = rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize);
      }
      image_section_virt_ranges.v[i] = rng_1u64(sect_header->voff, sect_header->voff + sect_header->vsize);
    }

    LNK_PushDbiSecContribTaskData task = {0};
    task.obj_arr                       = obj_arr;
    task.mod_arr                       = mod_arr;
    task.sc_list                       = push_array(scratch.arena, PDB_DbiSectionContribList, obj_count);
    task.image_data                    = image_data;
    task.image_section_file_ranges     = image_section_file_ranges;
    task.image_section_virt_ranges     = image_section_virt_ranges;
    tp_for_parallel(tp, tp_arena, obj_count, lnk_push_dbi_sec_contrib_task, &task);

    dbi_sec_list_concat_arr(&pdb->dbi->sec_contrib_list, obj_count, task.sc_list);
  }
  ProfEnd();

  ProfBegin("Build NatVis");
  {
    String8Array natvis_file_path_arr = str8_array_from_list(scratch.arena, &config->natvis_list);
    String8Array natvis_file_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, config->io_flags, natvis_file_path_arr);

    for (U64 i = 0; i < natvis_file_data_arr.count; ++i) {
      String8 natvis_file_path = natvis_file_path_arr.v[i];
      String8 natvis_file_data = natvis_file_data_arr.v[i];

      // did we read the file?
      if (natvis_file_data.size == 0) {
        lnk_error(LNK_Warning_FileNotFound, "unable to open natvis file \"%S\"", natvis_file_path);
        continue;
      }

      // sanity check file extension or VS wont load NatVis
      String8 ext = str8_skip_last_dot(natvis_file_path);
      if (!str8_match(ext, str8_lit("natvis"), StringMatchFlag_CaseInsensitive)) {
        lnk_error(LNK_Warning_Natvis, "Visual Studio expects .natvis extension: \"%S\"", natvis_file_path);
      }

      // add natvis to PDB
      PDB_SrcError error = pdb_add_src(pdb->info, pdb->msf, natvis_file_path, natvis_file_data, PDB_SrcComp_NULL);
      if (error != PDB_SrcError_OK) {
        lnk_error(LNK_Error_Natvis, "%S", pdb_string_from_src_error(error));
      }
    }
  }
  ProfEnd();
  
  lnk_build_pdb_public_symbols(tp, tp_arena, symtab, pdb->psi);
  
  pdb_build(tp, tp_arena, pdb, string_ht);

  MSF_Error msf_err = msf_build(pdb->msf);
  if (msf_err != MSF_Error_OK) {
    lnk_error(LNK_Error_UnableToSerializeMsf, "unable to serialize MSF: %s", msf_error_to_string(msf_err));
  }

  ProfBegin("Get Page Nodes");
  String8List page_data_list = msf_get_page_data_nodes(tp_arena->v[0], pdb->msf);
  ProfEnd();

  
  // NOTE: linker is about to exit so we can skip memory release
  // and let windows free memory since it does this faster
#if 0
  ProfBegin("Context Release");
  pdb_release(&pdb);
  ProfEnd();
#endif

  temp_end(huge_arena_temp);
  scratch_end(scratch);
  ProfEnd();
  return page_data_list;
}

internal U64
lnk_udt_name_hash_table_hash(String8 string)
{
  return XXH3_64bits(string.str, string.size);
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_udt_name_hash_table_task)
{
  LNK_BuildUDTNameHashTableTask *task = raw_task;

  LNK_UDTNameBucket *new_bucket = 0;
  for EachInRange(leaf_idx, task->ranges[task_id]) {
    String8 leaf_data = str8(task->leaf_arr[leaf_idx], max_U64);

    CV_Leaf leaf;
    cv_read_leaf(leaf_data, 0, 1, &leaf);

    // is this UDT?
    if ( ! cv_is_udt(leaf.kind)) { continue; }

    // skip forward references
    CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
    if (udt_info.props & CV_TypeProp_FwdRef) { continue; }

    // skip anon UDT
    if (cv_is_udt_name_anon(udt_info.name)) { continue; }

    // UDT name -> bucket index
    String8 name       = cv_name_from_udt_info(udt_info);
    U64     hash       = lnk_udt_name_hash_table_hash(name);
    U64     best_idx   = hash % task->buckets_cap;
    U64     bucket_idx = best_idx;

    // push and fill bucket
    if (new_bucket == 0) { new_bucket = push_array(arena, LNK_UDTNameBucket, 1); }
    new_bucket->name = name;
    new_bucket->leaf_idx = leaf_idx;

    B32 is_inserted_or_updated = 0;
    do {
      retry:;
            LNK_UDTNameBucket *curr_bucket = task->buckets[bucket_idx];

            if (curr_bucket == 0) {
              LNK_UDTNameBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&task->buckets[bucket_idx], new_bucket, curr_bucket);

              if (compare_bucket == curr_bucket) {
                // success, bucket was inserted
                is_inserted_or_updated = 1;
                break;
              }

              // another thread took the bucket...
              goto retry;
            } else if (str8_match(curr_bucket->name, name, 0)) {
              // there is more than one UDT with identical name, pick most recent and ignore others

              if (leaf_idx < curr_bucket->leaf_idx) {
                LNK_UDTNameBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&task->buckets[bucket_idx], new_bucket, curr_bucket);
                if (compare_bucket == curr_bucket) {
                  is_inserted_or_updated = 1;
                  break;
                }
              } else {
                // don't need to update, more recent leaf is in the bucket
                break;
              }

              // another thread took the bucket...
              goto retry;
            }

            // advance
            bucket_idx = (bucket_idx + 1) % task->buckets_cap;
    } while (bucket_idx != best_idx);

    if (is_inserted_or_updated) {
      new_bucket = 0;
    }
  }
}


internal LNK_UDTNameBucket **
lnk_udt_name_hash_table_from_leaf_arr(TP_Context *tp,
                                     TP_Arena   *arena,
                                     U64         leaf_count,
                                     U8        **leaf_arr,
                                     U64        *buckets_cap_out)
{
  Temp scratch = scratch_begin(&arena->v[0], 1);
  LNK_BuildUDTNameHashTableTask task = {0};
  task.leaf_count  = leaf_count;
  task.leaf_arr    = leaf_arr;
  task.buckets_cap = (leaf_count * 13) / 10;
  task.buckets     = push_array(arena->v[0], LNK_UDTNameBucket *, task.buckets_cap);
  task.ranges      = tp_divide_work(scratch.arena, leaf_count, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, lnk_build_udt_name_hash_table_task, &task);
  *buckets_cap_out = task.buckets_cap;
  scratch_end(scratch);
  return task.buckets;
}

internal LNK_UDTNameBucket *
lnk_udt_name_hash_table_lookup(LNK_UDTNameBucket **buckets, U64 cap, String8 name)
{
  U64 hash       = lnk_udt_name_hash_table_hash(name);
  U64 best_idx   = hash % cap;
  U64 bucket_idx = best_idx;
  do {
    if (buckets[bucket_idx] == 0) {
      break;
    }
    if (str8_match(buckets[bucket_idx]->name, name, 0)) {
      return buckets[bucket_idx];
    }
    bucket_idx = (bucket_idx + 1) % cap;
  } while (bucket_idx != best_idx);
  return 0;
}

internal CV_TypeIndex
lnk_udt_name_hash_table_lookup_itype(LNK_UDTNameBucket **buckets, U64 cap, String8 name)
{
  LNK_UDTNameBucket *bucket = lnk_udt_name_hash_table_lookup(buckets, cap, name);
  if (bucket != 0) {
    return CV_MinComplexTypeIndex + bucket->leaf_idx;
  }
  return 0;
}

internal RDIB_Type *
lnk_push_converted_codeview_type(Arena *arena, RDIB_TypeChunkList *list, RDIB_Type **itype_map, CV_TypeIndex itype)
{
  RDIB_Type *type = rdib_type_chunk_list_push(arena, list, 8196);
  type->final_idx = 0;
  type->itype     = itype;

  Assert(itype_map[itype] == 0);
  itype_map[itype] = type;

  return type;
}

internal void
lnk_push_basic_itypes(Arena *arena, RDIB_DataModel data_model, RDIB_Type **itype_map, RDIB_TypeChunkList *rdib_types_list)
{
  RDI_TypeKind short_type      = rdib_short_type_from_data_model(data_model);
  RDI_TypeKind ushort_type     = rdib_unsigned_short_type_from_data_model(data_model);
  RDI_TypeKind int_type        = rdib_int_type_from_data_model(data_model);
  RDI_TypeKind uint_type       = rdib_unsigned_int_type_from_data_model(data_model);
  RDI_TypeKind long_type       = rdib_long_type_from_data_model(data_model);
  RDI_TypeKind ulong_type      = rdib_unsigned_long_type_from_data_model(data_model);
  RDI_TypeKind long_long_type  = rdib_long_long_type_from_data_model(data_model);
  RDI_TypeKind ulong_long_type = rdib_unsigned_long_long_type_from_data_model(data_model);
  RDI_TypeKind ptr_type        = rdib_pointer_size_t_type_from_data_model(data_model);

  struct {
    char *       name;
    RDI_TypeKind kind_rdi;
    CV_LeafKind  kind_cv;
    B32          make_pointer_near;
    B32          make_pointer_32;
    B32          make_pointer_64;
  } table[] = {
    { "void"                 , RDI_TypeKind_Void       , CV_BasicType_VOID       , 1, 1, 1 },
    { "HRESULT"              , RDI_TypeKind_Handle     , CV_BasicType_HRESULT    , 0, 1, 1 },
    { "signed char"          , RDI_TypeKind_Char8      , CV_BasicType_CHAR       , 1, 1, 1 }, // TODO: we need Signed Char8 in RDI
    { "short"                , short_type              , CV_BasicType_SHORT      , 1, 1, 1 },
    { "long"                 , long_type               , CV_BasicType_LONG       , 1, 1, 1 },
    { "long long"            , long_long_type          , CV_BasicType_QUAD       , 1, 1, 1 },
    { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_OCT        , 1, 1, 1 }, // GCC/Clang type
    { "unsigned char"        , RDI_TypeKind_UChar8     , CV_BasicType_UCHAR      , 1, 1, 1 },
    { "unsigned short"       , ushort_type             , CV_BasicType_USHORT     , 1, 1, 1 },
    { "unsigned long"        , ulong_type              , CV_BasicType_ULONG      , 1, 1, 1 },
    { "unsigned long long"   , ulong_long_type         , CV_BasicType_UQUAD      , 1, 1, 1 },
    { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UOCT       , 1, 1, 1 }, // GCC/Clang type
    { "bool"                 , RDI_TypeKind_S8         , CV_BasicType_BOOL8      , 1, 1, 1 }, // TODO: we need a actual boolean type in RDI so we can format value as true/false.
    { "__bool16"             , RDI_TypeKind_S16        , CV_BasicType_BOOL16     , 1, 1, 1 }, // not real C type
    { "__bool32"             , RDI_TypeKind_S32        , CV_BasicType_BOOL32     , 1, 1, 1 }, // not real C type
    { "float"                , RDI_TypeKind_F32        , CV_BasicType_FLOAT32    , 1, 1, 1 },
    { "double"               , RDI_TypeKind_F64        , CV_BasicType_FLOAT64    , 1, 1, 1 },
    { "long double"          , RDI_TypeKind_F80        , CV_BasicType_FLOAT80    , 1, 1, 1 },
    { "__float128"           , RDI_TypeKind_F128       , CV_BasicType_FLOAT128   , 1, 1, 1 }, // GCC/Clang type
    { "__float48"            , RDI_TypeKind_F48        , CV_BasicType_FLOAT48    , 1, 1, 1 }, // not real C type
    { "__float32pp"          , RDI_TypeKind_F32PP      , CV_BasicType_FLOAT32PP  , 1, 1, 1 }, // not real C type
    { "_Complex float"       , RDI_TypeKind_ComplexF32 , CV_BasicType_COMPLEX32  , 0, 0, 0 },
    { "_Complex double"      , RDI_TypeKind_ComplexF64 , CV_BasicType_COMPLEX64  , 0, 0, 0 },
    { "_Complex long double" , RDI_TypeKind_ComplexF80 , CV_BasicType_COMPLEX80  , 0, 0, 0 },
    { "_Complex __float128"  , RDI_TypeKind_ComplexF128, CV_BasicType_COMPLEX128 , 0, 0, 0 },
    { "__int8"               , RDI_TypeKind_S8         , CV_BasicType_INT8       , 1, 1, 1 },
    { "__uint8"              , RDI_TypeKind_U8         , CV_BasicType_UINT8      , 1, 1, 1 },
    { "__int16"              , RDI_TypeKind_S16        , CV_BasicType_INT16      , 1, 1, 1 },
    { "__uint16"             , RDI_TypeKind_U16        , CV_BasicType_UINT16     , 1, 1, 1 },
    { "int"                  , int_type                , CV_BasicType_INT32      , 1, 1, 1 },
    { "unsigned int"         , uint_type               , CV_BasicType_UINT32     , 1, 1, 1 },
    { "__int64"              , RDI_TypeKind_S64        , CV_BasicType_INT64      , 1, 1, 1 },
    { "__uint64"             , RDI_TypeKind_U64        , CV_BasicType_UINT64     , 1, 1, 1 },
    { "__int128"             , RDI_TypeKind_S128       , CV_BasicType_INT128     , 1, 1, 1 },
    { "__uint128"            , RDI_TypeKind_U128       , CV_BasicType_UINT128    , 1, 1, 1 },
    { "char"                 , RDI_TypeKind_Char8      , CV_BasicType_RCHAR      , 1, 1, 1 }, // always ASCII
    { "wchar_t"              , RDI_TypeKind_UChar16    , CV_BasicType_WCHAR      , 1, 1, 1 }, // on windows always UTF-16
    { "char8_t"              , RDI_TypeKind_Char8      , CV_BasicType_CHAR8      , 1, 1, 1 }, // always UTF-8
    { "char16_t"             , RDI_TypeKind_Char16     , CV_BasicType_CHAR16     , 1, 1, 1 }, // always UTF-16
    { "char32_t"             , RDI_TypeKind_Char32     , CV_BasicType_CHAR32     , 1, 1, 1 }, // always UTF-32
    { "__pointer"            , ptr_type                , CV_BasicType_PTR        , 0, 0, 0 }
  };

  for (U64 i = 0; i < ArrayCount(table); ++i) {
    U64 builtin_size;
    if (table[i].kind_rdi == RDI_TypeKind_Void || table[i].kind_rdi == RDI_TypeKind_Handle) {
      builtin_size = rdi_size_from_basic_type_kind(ptr_type);
    } else {
      builtin_size = rdi_size_from_basic_type_kind(table[i].kind_rdi);
    }

    RDIB_Type *builtin    = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv);
    builtin->kind         = table[i].kind_rdi;
    builtin->builtin.name = str8_cstring(table[i].name);
    builtin->builtin.size = builtin_size;

    RDIB_Type **wrapper = push_array(arena, RDIB_Type *, 1);
    *wrapper = builtin;

    if (table[i].make_pointer_near) {
      RDIB_Type *ptr_near    = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x100);
      ptr_near->kind         = RDI_TypeKind_Ptr;
      ptr_near->ptr.size     = rdi_size_from_basic_type_kind(ptr_type);
      ptr_near->ptr.type_ref = wrapper;
    }
    if (table[i].make_pointer_32) {
      RDIB_Type *ptr_32    = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x400);
      ptr_32->kind         = RDI_TypeKind_Ptr;
      ptr_32->ptr.size     = 4;
      ptr_32->ptr.type_ref = wrapper;
    }
    if (table[i].make_pointer_64) {
      RDIB_Type *ptr_64    = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x600);
      ptr_64->kind         = RDI_TypeKind_Ptr;
      ptr_64->ptr.size     = 8;
      ptr_64->ptr.type_ref = wrapper;
    }

#if 0
      RDIB_Type *ptr_far   = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x200);
      RDIB_Type *ptr_huge  = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x300);
      RDIB_Type *ptr_16_32 = lnk_push_converted_codeview_type(arena, rdib_types_list, itype_map, table[i].kind_cv | 0x500);

      ptr_far->kind           = RDI_TypeKind_Ptr;
      ptr_far->ptr.size       = rdi_size_from_basic_type_kind(ptr_type);
      ptr_far->ptr.type_ref   = wrapper;

      ptr_huge->kind          = RDI_TypeKind_Ptr;
      ptr_huge->ptr.size      = 4;
      ptr_huge->ptr.type_ref  = wrapper;


      ptr_16_32->kind         = RDI_TypeKind_Ptr;
      ptr_16_32->ptr.size     = 6;
      ptr_16_32->ptr.type_ref = wrapper;
#endif
  }
}

internal RDIB_TypeRef
lnk_rdib_type_from_itype(LNK_ConvertTypesToRDI *task, CV_TypeIndex itype)
{
  RDIB_TypeRef result    = &task->tpi_itype_map[0];
  Rng1U64      tpi_range = task->itype_ranges[CV_TypeIndexSource_TPI];

  if (itype < tpi_range.min) {
    // check for supported CodeView pointer formats:
    AssertAlways(BitExtract(itype, 8, 8) == /* near   */  0x1 ||
                 BitExtract(itype, 8, 8) == /* 32 bit */  0x4 ||
                 BitExtract(itype, 8, 8) == /* 64 bit */  0x6 ||
                 BitExtract(itype, 8, 8) == /* regular */ 0x0);
  }

  if (itype < tpi_range.max) {
    CV_TypeIndex final_itype = itype;

    // try to resovle forward reference (defn might be missing)
    if (itype >= tpi_range.min) {
      CV_Leaf leaf;
      cv_read_leaf(str8(task->leaf_arr[CV_TypeIndexSource_TPI][itype - tpi_range.min], max_U64), 0, 1, &leaf);
      if (cv_is_udt(leaf.kind)) {
        CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
        if (udt_info.props & CV_TypeProp_FwdRef) {
          String8      name           = cv_name_from_udt_info(udt_info);
          CV_TypeIndex resolved_itype = lnk_udt_name_hash_table_lookup_itype(task->udt_name_buckets, task->udt_name_bucket_cap, name);
          if (resolved_itype != 0) {
            final_itype = resolved_itype;
          }
        }
      }
    }

    result = &task->tpi_itype_map[final_itype];
  }

  return result;
}

internal RDI_MemberKind
lnk_rdib_method_kind_from_cv_prop(CV_MethodProp prop)
{
  switch (prop) {
  case CV_MethodProp_Vanilla:     return RDI_MemberKind_Method;
  case CV_MethodProp_Virtual:     return RDI_MemberKind_VirtualMethod;
  case CV_MethodProp_Static:      return RDI_MemberKind_StaticMethod;
  case CV_MethodProp_Friend:      NotImplemented;
  case CV_MethodProp_Intro:       return RDI_MemberKind_VirtualMethod;
  case CV_MethodProp_PureVirtual: return RDI_MemberKind_VirtualMethod;
  case CV_MethodProp_PureIntro:   return RDI_MemberKind_VirtualMethod;
  }
  return RDI_MemberKind_NULL;
}

internal
THREAD_POOL_TASK_FUNC(lnk_convert_types_to_rdi_task)
{
  ProfBeginFunction();
  LNK_ConvertTypesToRDI *task = raw_task;

  // upfront push output type array
  U64 leaf_count = dim_1u64(task->ranges[task_id]);
  rdib_type_chunk_list_reserve(arena, &task->rdib_types_lists[task_id], leaf_count);

  for(U64 leaf_idx = task->ranges[task_id].min; leaf_idx < task->ranges[task_id].max; ++leaf_idx) {
    U64     itype = task->itype_ranges[CV_TypeIndexSource_TPI].min + leaf_idx;
    CV_Leaf src   = cv_leaf_from_ptr(task->leaf_arr[CV_TypeIndexSource_TPI][leaf_idx]);

    switch (src.kind) {
    case CV_LeafKind_MODIFIER: {
      CV_LeafModifier *modifier = (CV_LeafModifier *) src.data.str;

      RDIB_Type *dst         = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
      dst->kind              = RDI_TypeKind_Modifier;
      dst->modifier.flags    = rdi_type_modifier_flags_from_cv_modifier_flags(modifier->flags);
      dst->modifier.type_ref = lnk_rdib_type_from_itype(task, modifier->itype);
    } break;
    case CV_LeafKind_POINTER: {
      CV_LeafPointer *ptr      = (CV_LeafPointer *) src.data.str;
      CV_PointerKind  ptr_kind = CV_PointerAttribs_Extract_Kind(ptr->attribs);
      CV_PointerMode  ptr_mode = CV_PointerAttribs_Extract_Mode(ptr->attribs);
      U32             ptr_size = CV_PointerAttribs_Extract_Size(ptr->attribs);
      (void)ptr_kind;

      // parse ahead type chain and squash modifiers
      RDI_TypeModifierFlags modifier_flags = rdi_type_modifier_flags_from_cv_pointer_attribs(ptr->attribs);
      CV_TypeIndex          next_itype;
      for (next_itype = ptr->itype; task->itype_ranges[CV_TypeIndexSource_TPI].min <= next_itype && next_itype < task->itype_ranges[CV_TypeIndexSource_TPI].max;) {
        U64     next_leaf_idx = next_itype - task->itype_ranges[CV_TypeIndexSource_TPI].min;
        CV_Leaf next_leaf     = cv_leaf_from_ptr(task->leaf_arr[CV_TypeIndexSource_TPI][next_leaf_idx]);
        if (next_leaf.kind != CV_LeafKind_MODIFIER) {
          break;
        }

        // parse LF_MODIFIER
        CV_LeafModifier       *sym_modifier = (CV_LeafModifier *) next_leaf.data.str;
        RDI_TypeModifierFlags  flags        = rdi_type_modifier_flags_from_cv_modifier_flags(sym_modifier->flags);

        // accumulate modifier flags
        modifier_flags |= flags;

        // advance
        next_itype = sym_modifier->itype;
      }

      if (modifier_flags == 0) {
        // No modifer just generate pointer type.
        RDIB_Type *dst    = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
        dst->kind         = rdi_type_kind_from_pointer(ptr->attribs, ptr_mode);
        dst->ptr.size     = ptr_size;
        dst->ptr.type_ref = lnk_rdib_type_from_itype(task, ptr->itype);
      } else {
        // CodeView embeds modifier in pointer struct, we don't have an equivalent 
        // so generate a modifier type in pointer slot and link with pointer type.

        RDIB_Type *ptr_type    = rdib_type_chunk_list_push(arena, &task->rdib_types_lists[task_id], task->type_cap);
        ptr_type->kind         = rdi_type_kind_from_pointer(ptr->attribs, ptr_mode);
        ptr_type->ptr.type_ref = lnk_rdib_type_from_itype(task, next_itype);
        RDIB_Type **indirect_ptr_type = push_array(arena, RDIB_Type *, 1);
        *indirect_ptr_type = ptr_type;

        RDIB_Type *dst         = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
        dst->kind              = RDI_TypeKind_Modifier;
        dst->modifier.flags    = modifier_flags;
        dst->modifier.type_ref = indirect_ptr_type;
      }
    } break;
    case CV_LeafKind_PROCEDURE: {
      CV_LeafProcedure *proc = (CV_LeafProcedure *) src.data.str;

      RDIB_Type *dst        = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
      dst->kind             = RDI_TypeKind_Function;
      dst->func.return_type = lnk_rdib_type_from_itype(task, proc->ret_itype);
      dst->func.params_type = lnk_rdib_type_from_itype(task, proc->arg_itype);
    } break;
    case CV_LeafKind_MFUNCTION: {
      CV_LeafMFunction *mfunc = (CV_LeafMFunction *) src.data.str;
      B32 is_static_method = mfunc->this_itype == 0;
      RDIB_Type *dst = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);

      if (is_static_method) {
        dst->kind                      = RDI_TypeKindExt_StaticMethod;
        dst->static_method.class_type  = lnk_rdib_type_from_itype(task, mfunc->class_itype);
        dst->static_method.return_type = lnk_rdib_type_from_itype(task, mfunc->ret_itype);
        dst->static_method.params_type = lnk_rdib_type_from_itype(task, mfunc->arg_itype);
      } else {
        dst->kind               = RDI_TypeKind_Method;
        dst->method.class_type  = lnk_rdib_type_from_itype(task, mfunc->class_itype);
        dst->method.this_type   = lnk_rdib_type_from_itype(task, mfunc->this_itype);
        dst->method.return_type = lnk_rdib_type_from_itype(task, mfunc->ret_itype);
        dst->method.params_type = lnk_rdib_type_from_itype(task, mfunc->arg_itype);
      }
    } break;
    case CV_LeafKind_BITFIELD: {
      CV_LeafBitField *bitfield = (CV_LeafBitField *) src.data.str;

      RDIB_Type *dst           = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
      dst->kind                = RDI_TypeKind_Bitfield;
      dst->bitfield.off        = bitfield->pos;
      dst->bitfield.count      = bitfield->len;
      dst->bitfield.value_type = lnk_rdib_type_from_itype(task, bitfield->itype);
    } break;
    case CV_LeafKind_ARRAY: {
      CV_LeafArray     *array = (CV_LeafArray *) src.data.str;
      CV_NumericParsed  size  = cv_numeric_from_data_range(src.data.str + sizeof(CV_LeafArray), src.data.str + src.data.size);

      RDIB_Type *dst        = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
      dst->kind             = RDI_TypeKind_Array;
      dst->array.entry_type = lnk_rdib_type_from_itype(task, array->entry_itype);
      dst->array.size       = cv_u64_from_numeric(&size);
    } break;
    case CV_LeafKind_CLASS:
    case CV_LeafKind_STRUCTURE: {
      CV_LeafStruct    *udt  = (CV_LeafStruct *) src.data.str;
      CV_NumericParsed  size = cv_numeric_from_data_range(src.data.str + sizeof(CV_LeafStruct), src.data.str + src.data.size);

      String8 name;
      String8 link_name;
      if (udt->props & CV_TypeProp_HasUniqueName) {
        name      = str8_cstring_capped(src.data.str + sizeof(CV_LeafStruct) + size.encoded_size, src.data.str + src.data.size);
        link_name = str8_cstring_capped_reverse(name.str + name.size + 1, src.data.str + src.data.size);
      } else {
        name      = str8_cstring_capped_reverse(src.data.str + sizeof(CV_LeafStruct) + size.encoded_size, src.data.str + src.data.size);
        link_name = name;
      }

      RDIB_Type *dst               = lnk_push_converted_codeview_type(arena, &task->rdib_types_struct_lists[task_id], task->tpi_itype_map, itype);
      dst->udt.name                = name;
      dst->udt.link_name           = link_name;
      dst->udt.members             = lnk_rdib_type_from_itype(task, udt->field_itype);
      dst->udt.struct_type.size    = cv_u64_from_numeric(&size);
      dst->udt.struct_type.derived = lnk_rdib_type_from_itype(task, udt->derived_itype);
      dst->udt.struct_type.vtshape = lnk_rdib_type_from_itype(task, udt->vshape_itype);

      if (udt->props & CV_TypeProp_FwdRef) {
        dst->kind = src.kind == CV_LeafKind_CLASS ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct;
      } else {
        dst->kind = src.kind == CV_LeafKind_CLASS ? RDI_TypeKind_Class : RDI_TypeKind_Struct;
      }
    } break;
    case CV_LeafKind_CLASS2:
    case CV_LeafKind_STRUCT2: {
      CV_LeafStruct2   *udt  = (CV_LeafStruct2 *) src.data.str;
      CV_NumericParsed  size = cv_numeric_from_data_range(src.data.str + sizeof(CV_LeafStruct2), src.data.str + src.data.size);

      String8 name;
      String8 link_name;
      if (udt->props & CV_TypeProp_HasUniqueName) {
        name      = str8_cstring_capped(src.data.str + sizeof(CV_LeafStruct2) + size.encoded_size, src.data.str + src.data.size);
        link_name = str8_cstring_capped_reverse(name.str + name.size + 1, src.data.str + src.data.size);
      } else {
        name      = str8_cstring_capped_reverse(src.data.str + sizeof(CV_LeafStruct2) + size.encoded_size, src.data.str + src.data.size);
        link_name = name;
      }

      RDIB_Type *dst = lnk_push_converted_codeview_type(arena, &task->rdib_types_struct_lists[task_id], task->tpi_itype_map, itype);
      dst->udt.name                = name;
      dst->udt.link_name           = link_name;
      dst->udt.members             = lnk_rdib_type_from_itype(task, udt->field_itype);
      dst->udt.struct_type.size    = cv_u64_from_numeric(&size);
      dst->udt.struct_type.derived = lnk_rdib_type_from_itype(task, udt->derived_itype);
      dst->udt.struct_type.vtshape = lnk_rdib_type_from_itype(task, udt->vshape_itype);

      if (udt->props & CV_TypeProp_FwdRef) {
        dst->kind = src.kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_IncompleteClass : RDI_TypeKind_IncompleteStruct;
      } else {
        dst->kind = src.kind == CV_LeafKind_CLASS2 ? RDI_TypeKind_Class : RDI_TypeKind_Struct;
      }
    } break;
    case CV_LeafKind_UNION: {
      CV_LeafUnion     *udt  = (CV_LeafUnion *) src.data.str;
      CV_NumericParsed  size = cv_numeric_from_data_range(src.data.str + sizeof(CV_LeafUnion), src.data.str + src.data.size);

      String8 name;
      String8 link_name;
      if (udt->props & CV_TypeProp_HasUniqueName) {
        name      = str8_cstring_capped(src.data.str + sizeof(CV_LeafUnion) + size.encoded_size, src.data.str + src.data.size);
        link_name = str8_cstring_capped_reverse(name.str + name.size + 1, src.data.str + src.data.size);
      } else {
        name      = str8_cstring_capped_reverse(src.data.str + sizeof(CV_LeafUnion) + size.encoded_size, src.data.str + src.data.size);
        link_name = name;
      }

      RDIB_Type *dst           = lnk_push_converted_codeview_type(arena, &task->rdib_types_union_lists[task_id], task->tpi_itype_map, itype);
      dst->udt.name            = name;
      dst->udt.link_name       = link_name;
      dst->udt.members         = lnk_rdib_type_from_itype(task, udt->field_itype);
      dst->udt.union_type.size = cv_u64_from_numeric(&size);

      if (udt->props & CV_TypeProp_FwdRef) {
        dst->kind = RDI_TypeKind_IncompleteUnion;
      } else {
        dst->kind = RDI_TypeKind_Union;
      }
    } break;
    case CV_LeafKind_ENUM: {
      CV_LeafEnum *udt  = (CV_LeafEnum *) src.data.str;

      String8 name;
      String8 link_name;
      if (udt->props & CV_TypeProp_HasUniqueName) {
        name      = str8_cstring_capped(src.data.str + sizeof(*udt), src.data.str + src.data.size);
        link_name = str8_cstring_capped_reverse(name.str + name.size + 1, src.data.str + src.data.size);
      } else {
        name      = str8_cstring_capped_reverse(src.data.str + sizeof(*udt), src.data.str + src.data.size);
        link_name = name;
      }

      RDIB_Type *dst               = lnk_push_converted_codeview_type(arena, &task->rdib_types_enum_lists[task_id], task->tpi_itype_map, itype);
      dst->kind                    = (RDI_TypeKindExt)RDI_TypeKind_Enum;
      dst->udt.name                = name;
      dst->udt.link_name           = link_name;
      dst->udt.members             = lnk_rdib_type_from_itype(task, udt->field_itype);
      dst->udt.enum_type.base_type = lnk_rdib_type_from_itype(task, udt->base_itype);

      if (udt->props & CV_TypeProp_FwdRef) {
        dst->kind = RDI_TypeKind_IncompleteEnum;
      } else {
        dst->kind = (RDI_TypeKindExt)RDI_TypeKind_Enum;
      }
    } break;
    case CV_LeafKind_ARGLIST: {
      CV_LeafArgList *arglist = (CV_LeafArgList *) src.data.str;
      CV_TypeIndex   *itypes  = (CV_TypeIndex *) (arglist + 1);

      if (arglist->count * sizeof(CV_TypeIndex) + sizeof(CV_LeafArgList) > src.data.size) {
        AssertAlways("error: ill-formed LF_ARGLIST");
        break;
      }

      RDIB_Type *dst = lnk_push_converted_codeview_type(arena, &task->rdib_types_params_lists[task_id], task->tpi_itype_map, itype);
      dst->kind         = RDI_TypeKindExt_Params; // there is no Params kind in RDI
      dst->params.count = arglist->count;
      dst->params.types = push_array(arena, RDIB_TypeRef, arglist->count);
      for (U64 param_idx = 0; param_idx < arglist->count; ++param_idx) {
        // strange way to encode variadic params, when outside LF_ARGLIST LF_NOTYPE actually means null...
        if (itypes[param_idx] == CV_LeafKind_NOTYPE) { 
          dst->params.types[param_idx] = task->variadic_type_ref;
        } else {
          dst->params.types[param_idx] = lnk_rdib_type_from_itype(task, itypes[param_idx]);
        }
      }
    } break;
    case CV_LeafKind_FIELDLIST: {
      RDIB_UDTMemberChunkList *rdib_member_list;
      RDIB_TypeChunkList      *rdib_member_types;
      B32 is_enum = sizeof(CV_LeafKind) <= src.data.size && (*(CV_LeafKind *)src.data.str == CV_LeafKind_ENUMERATE);
      if (is_enum) {
        rdib_member_list  = &task->rdib_enum_members_lists[worker_id];
        rdib_member_types = &task->rdib_types_enum_members_lists[worker_id];
      } else {
        rdib_member_list  = &task->rdib_udt_members_lists[worker_id];
        rdib_member_types = &task->rdib_types_udt_members_lists[worker_id];
      }

      RDIB_Type *dst = lnk_push_converted_codeview_type(arena, rdib_member_types, task->tpi_itype_map, itype);
      dst->kind = RDI_TypeKindExt_Members;

      for (U64 cursor = 0; cursor + sizeof(CV_LeafKind) <= src.data.size; ) {
        CV_LeafKind field_kind = *(CV_LeafKind *) (src.data.str + cursor);
        cursor += sizeof(field_kind);

        // do we have bytes to read?
        U64 header_size = cv_header_struct_size_from_leaf_kind(field_kind);
        if (cursor + header_size > src.data.size) {
          break;
        }

        switch (field_kind) {
        case CV_LeafKind_INDEX: {
          CV_LeafIndex *index = (CV_LeafIndex *) (src.data.str + cursor);
          cursor += sizeof(*index);

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB member list pointer
          member->kind                = RDI_MemberKindExt_MemberListPointer;
          member->member_list_pointer = lnk_rdib_type_from_itype(task, index->itype);
        } break;
        case CV_LeafKind_MEMBER: {
          // prase CodeView struct/class/union data member
          CV_LeafMember   *leaf_member = (CV_LeafMember *) (src.data.str + cursor);
          CV_NumericParsed offset      = cv_numeric_from_data_range((U8 *)(leaf_member + 1), src.data.str + src.data.size);
          String8          name        = str8_cstring_capped(src.data.str + cursor + sizeof(CV_LeafMember) + offset.encoded_size, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafMember);
          cursor += offset.encoded_size;
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB data member
          member->kind                = RDI_MemberKind_DataField;
          member->data_field.name     = name;
          member->data_field.type_ref = lnk_rdib_type_from_itype(task, leaf_member->itype);
          member->data_field.offset   = cv_u64_from_numeric(&offset);
        } break;
        case CV_LeafKind_STMEMBER: {
          // parse CodeView static member
          CV_LeafStMember *st_member = (CV_LeafStMember *) (src.data.str + cursor);
          String8         name      = str8_cstring_capped(st_member + 1, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafStMember);
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB static member
          member->kind                 = RDI_MemberKind_StaticData;
          member->static_data.name     = name;
          member->static_data.type_ref = lnk_rdib_type_from_itype(task, st_member->itype);
        } break;
        case CV_LeafKind_METHOD: {
          // parse CodeView over-loaded method
          CV_LeafMethod *method = (CV_LeafMethod *) (src.data.str + cursor);
          String8        name   = str8_cstring_capped(method + 1, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafMethod);
          cursor += name.size + 1;

          if (contains_1u64(task->itype_ranges[CV_TypeIndexSource_TPI], method->list_itype)) {
            U64     method_list_leaf_idx = method->list_itype - task->itype_ranges[CV_TypeIndexSource_TPI].min;
            CV_Leaf method_list_leaf     = cv_leaf_from_ptr(task->leaf_arr[CV_TypeIndexSource_TPI][method_list_leaf_idx]);
            if (method_list_leaf.kind == CV_LeafKind_METHODLIST) {
              for (U64 cursor = 0; cursor + sizeof(CV_LeafMethodListMember) <= method_list_leaf.data.size; ) {
                // parse CodeView method overload info
                CV_LeafMethodListMember *list_member = (CV_LeafMethodListMember *) (method_list_leaf.data.str + cursor);
                CV_MethodProp            prop        = CV_FieldAttribs_Extract_MethodProp(list_member->attribs);
                cursor += sizeof(CV_LeafMethodListMember);
                U32 vftable_offset = 0;
                if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro) {
                  str8_deserial_read_struct(src.data, cursor, &vftable_offset);
                  cursor += sizeof(vftable_offset);
                }

                // push new node
                RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
                rdib_udt_member_list_push_node(&dst->members.list, member);

                // fill out RDIB method
                member->kind                  = RDI_MemberKind_Method;
                member->method.kind           = lnk_rdib_method_kind_from_cv_prop(prop);
                member->method.name           = name;
                member->method.type_ref       = lnk_rdib_type_from_itype(task, list_member->itype);
                member->method.vftable_offset = vftable_offset;
              }
            } else {
              Assert(!"error: expected LF_METHODLIST");
            }
          }
        } break;
        case CV_LeafKind_ONEMETHOD: {
          // parse CodeView method
          CV_LeafOneMethod *one_method = (CV_LeafOneMethod *) (src.data.str + cursor);
          CV_MethodProp     prop       = CV_FieldAttribs_Extract_MethodProp(one_method->attribs);
          cursor += sizeof(CV_LeafOneMethod);
          U32 vftable_offset = 0;
          if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro) {
            str8_deserial_read_struct(src.data, cursor, &vftable_offset);
            cursor += sizeof(vftable_offset);
          }
          String8 name = str8_cstring_capped(src.data.str + cursor, src.data.str + src.data.size);
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB member
          member->kind                  = RDI_MemberKind_Method;
          member->method.kind           = lnk_rdib_method_kind_from_cv_prop(prop);
          member->method.name           = name;
          member->method.type_ref       = lnk_rdib_type_from_itype(task, one_method->itype);
          member->method.vftable_offset = vftable_offset;
        } break;
        case CV_LeafKind_NESTTYPE: {
          // parse CodeView nested type
          CV_LeafNestType *nest_type = (CV_LeafNestType *) (src.data.str + cursor);
          String8          name      = str8_cstring_capped(nest_type + 1, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafNestType);
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB nested type member
          member->kind                 = RDI_MemberKind_NestedType;
          member->nested_type.name     = name;
          member->nested_type.type_ref = lnk_rdib_type_from_itype(task, nest_type->itype);
        } break;
        case CV_LeafKind_NESTTYPEEX: {
          // parse CodeView nested type extended
          CV_LeafNestTypeEx *nest_type_ex = (CV_LeafNestTypeEx *) (src.data.str + cursor);
          String8            name         = str8_cstring_capped(nest_type_ex + 1, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafNestTypeEx);
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB nested type member
          member->kind                 = RDI_MemberKind_NestedType;
          member->nested_type.name     = name;
          member->nested_type.type_ref = lnk_rdib_type_from_itype(task, nest_type_ex->itype);
        } break;
        case CV_LeafKind_BCLASS: {
          // parse CodeView base class member
          CV_LeafBClass    *bclass = (CV_LeafBClass *) (src.data.str + cursor);
          CV_NumericParsed  offset = cv_numeric_from_data_range((U8 *)(bclass + 1), src.data.str + src.data.size);
          cursor += sizeof(CV_LeafBClass);
          cursor += offset.encoded_size;

          U64 offset64 = cv_u64_from_numeric(&offset);

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB base class member
          member->kind                = RDI_MemberKind_Base;
          member->base_class.type_ref = lnk_rdib_type_from_itype(task, bclass->itype);
          member->base_class.offset   = offset64;
        } break;
        case CV_LeafKind_VBCLASS:
        case CV_LeafKind_IVBCLASS: {
          // parse CodeView virtual base class
          CV_LeafVBClass   *vbclass    = (CV_LeafVBClass *) (src.data.str + cursor);
          CV_NumericParsed  vbptr_off  = cv_numeric_from_data_range(src.data.str + cursor + sizeof(*vbclass), src.data.str + src.data.size);
          CV_NumericParsed  vtable_off = cv_numeric_from_data_range(src.data.str + cursor + sizeof(*vbclass) + vbptr_off.encoded_size, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafVBClass);
          cursor += vbptr_off.encoded_size;
          cursor += vtable_off.encoded_size;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB virtual base class member
          member->kind                          = RDI_MemberKind_VirtualBase;
          member->virtual_base_class.type_ref   = lnk_rdib_type_from_itype(task, vbclass->itype);
          member->virtual_base_class.vbptr_off  = cv_u64_from_numeric(&vbptr_off);
          member->virtual_base_class.vtable_off = cv_u64_from_numeric(&vtable_off);
        } break;
        case CV_LeafKind_VFUNCTAB: {
          // parse CodeView virtual function table
          CV_LeafVFuncTab *vfunc_tab = (CV_LeafVFuncTab *) (src.data.str + cursor);
          cursor += sizeof(*vfunc_tab);

          // TODO: we don't have an equivalent in RDI
        } break;
        case CV_LeafKind_ENUMERATE: {
          // parse CodeView enum member
          CV_LeafEnumerate *enumerate = (CV_LeafEnumerate *) (src.data.str + cursor);
          CV_NumericParsed  value     = cv_numeric_from_data_range((U8 *) (enumerate + 1), src.data.str + src.data.size);
          String8           name      = str8_cstring_capped(src.data.str + cursor + sizeof(CV_LeafEnumerate) + value.encoded_size, src.data.str + src.data.size);
          cursor += sizeof(CV_LeafEnumerate);
          cursor += value.encoded_size;
          cursor += name.size + 1;

          // push new node
          RDIB_UDTMember *member = rdib_udt_member_chunk_list_push(arena, rdib_member_list, task->udt_cap);
          rdib_udt_member_list_push_node(&dst->members.list, member);

          // fill out RDIB enum member
          member->kind            = RDI_MemberKind_NULL;
          member->enumerate.name  = name;
          member->enumerate.value = cv_u64_from_numeric(&value);
        } break;
        default: InvalidPath;
        }

        cursor = AlignPow2(cursor, 4);
      }
    } break;
    case CV_LeafKind_METHODLIST: {
      // see CV_LeafKind_METHOD
    } break;
    case CV_LeafKind_LABEL: {
      // ???
    } break;
    case CV_LeafKind_VTSHAPE: {
      RDIB_Type *dst = lnk_push_converted_codeview_type(arena, &task->rdib_types_lists[task_id], task->tpi_itype_map, itype);
      dst->kind = RDI_TypeKindExt_VirtualTable;
      // ???
    } break;
    case CV_LeafKind_VFTABLE: {
      // ???
    } break;
    default: InvalidPath; break;
    }

#undef push_converted_type
  }
  ProfEnd();
}

internal U64
lnk_src_file_hash_cv(String8 normal_full_path, CV_C13ChecksumKind checksum_kind, String8 checksum)
{
  XXH3_state_t state;
  XXH3_INITSTATE(&state);
  XXH3_64bits_reset(&state);
  XXH3_64bits_update(&state, normal_full_path.str, normal_full_path.size);
  XXH3_64bits_update(&state, &checksum_kind, sizeof(checksum_kind));
  XXH3_64bits_update(&state, checksum.str, checksum.size);
  XXH64_hash_t result = XXH3_64bits_digest(&state);
  return result;
}

internal String8
lnk_normalize_src_file_path(Arena *arena, String8 file_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 result = file_path;
  result = lower_from_str8(scratch.arena, result);
  result = path_convert_slashes(scratch.arena, result, PathStyle_UnixAbsolute);
  result = push_str8_copy(arena, result);
  scratch_end(scratch);
  return result;
}

internal LNK_SourceFileBucket *
lnk_src_file_hash_table_lookup_slot(LNK_SourceFileBucket **buckets,
                                    U64                    cap,
                                    U64                    hash,
                                    String8                normal_path,
                                    CV_C13ChecksumKind    checksum_kind,
                                    String8                checksum)
{
  U64 best_idx   = hash % cap;
  U64 bucket_idx = best_idx;

  RDIB_SourceFile temp  = {0};
  temp.normal_full_path = normal_path;
  temp.checksum_kind    = checksum_kind;
  temp.checksum         = checksum;

  do {
    if (buckets[bucket_idx] == 0) {
      break;
    }
    if (rdib_source_file_match(buckets[bucket_idx]->src_file, &temp, OperatingSystem_CURRENT)) {
      return buckets[bucket_idx];
    }
    bucket_idx = (bucket_idx + 1) % cap;
  } while (bucket_idx != best_idx);

  return 0;
}


internal LNK_SourceFileBucket *
lnk_src_file_insert_or_update(LNK_SourceFileBucket **buckets, U64 cap, U64 hash, LNK_SourceFileBucket *new_bucket)
{
  LNK_SourceFileBucket *result = 0;

  U64 best_idx = hash % cap;
  U64 idx      = best_idx;
  do {
    retry:;
    LNK_SourceFileBucket *curr_bucket = buckets[idx];

    if (curr_bucket == 0) {
      LNK_SourceFileBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {
        // success, bucket was inserted
        result = curr_bucket;
        break;
      }

      // another thread took the bucket...
      goto retry;
    } else if (rdib_source_file_match(curr_bucket->src_file, new_bucket->src_file, OperatingSystem_CURRENT)) {
      // do we need to update value in the bucket?
      int cmp = u64_compar(&curr_bucket->obj_idx, &new_bucket->obj_idx);
      if (cmp <= 0) {
        // are we inserting bucket that was already inserterd?
        Assert(cmp < 0);

        // don't need to update, more recent value is in the bucket
        break;
      }

      LNK_SourceFileBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {
        // success, bucket was inserted
        result = compare_bucket;
        break;
      }

      // another thread took the bucket...
      goto retry;
    }

    // advance
    idx = (idx + 1);
    idx = idx == cap ? 0 : idx;
  } while (idx != best_idx);

  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_source_files_task)
{
  U64                              unit_idx        = task_id;
  LNK_ConvertSourceFilesToRDITask *task            = raw_task;
  CV_DebugS                        debug_s         = task->debug_s_arr[unit_idx];
  String8List                      raw_chksms_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);

  U64 count = 0;

  for (String8Node *raw_chksms_n = raw_chksms_list.first; raw_chksms_n != 0; raw_chksms_n = raw_chksms_n->next) {
    for(U64 cursor = 0; cursor + sizeof(CV_C13Checksum) <= raw_chksms_n->string.size; ) {
      // parse header
      CV_C13Checksum *header = (CV_C13Checksum *) (raw_chksms_n->string.str + cursor);

      // update count
      ++count;

      // advance cursor
      cursor += sizeof(*header);
      cursor += header->len;
      cursor = AlignPow2(cursor, CV_FileCheckSumsAlign);
    }
  }

  // update total count
  ins_atomic_u64_add_eval(&task->total_src_file_count, count);
}

internal
THREAD_POOL_TASK_FUNC(lnk_insert_src_files_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  U64                              obj_idx         = task_id;
  LNK_ConvertSourceFilesToRDITask *task            = raw_task;
  CV_DebugS                        debug_s         = task->debug_s_arr[obj_idx];
  String8List                      raw_chksms_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
  String8List                      raw_strtab_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_StringTable);

  if (raw_strtab_list.node_count > 1) {
    lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], "Multiple string table sub-sections, picking first one.");
  }
  if (raw_chksms_list.node_count > 1) {
    lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], "Multiple file checksum sub-sections, picking first one.");
  }

  String8               string_table = cv_string_table_from_debug_s(debug_s);
  LNK_SourceFileBucket *curr_bucket  = 0;

  for (String8Node *raw_chksms_n = raw_chksms_list.first; raw_chksms_n != 0; raw_chksms_n = raw_chksms_n->next) {
    for (U64 cursor = 0; cursor + sizeof(CV_C13Checksum) <= raw_chksms_n->string.size; ) {
      // parse header
      CV_C13Checksum *header = (CV_C13Checksum *) (raw_chksms_n->string.str + cursor);

      // grab checksum
      String8 checksum = str8_substr(raw_chksms_n->string, rng_1u64(cursor + sizeof(CV_C13Checksum), 
                                                                    cursor + sizeof(CV_C13Checksum) + header->len));

      // grab file path
      Assert(header->name_off < string_table.size);
      String8 file_path = str8_cstring_capped(string_table.str + header->name_off, string_table.str + string_table.size);

      // normalize file path
      String8 normal_path = lnk_normalize_src_file_path(arena, file_path);

      // push new bucket
      if (curr_bucket == 0) {
        curr_bucket           = push_array(arena, LNK_SourceFileBucket, 1);
        curr_bucket->src_file = push_array(arena, RDIB_SourceFile, 1);
      }

      // fill out obj idx so we can decide which source file to keep in the hash table
      curr_bucket->obj_idx = obj_idx;

      // fill out part with source file info
      curr_bucket->src_file->file_path        = file_path;
      curr_bucket->src_file->normal_full_path = normal_path;
      curr_bucket->src_file->checksum_kind    = rdi_checksum_from_cv_c13(header->kind);
      curr_bucket->src_file->checksum         = checksum;
      curr_bucket->src_file->line_table_frags = 0;

      // insert bucket
      U64                   normal_path_hash = lnk_src_file_hash_cv(normal_path, header->kind, checksum);
      LNK_SourceFileBucket *insert_result    = lnk_src_file_insert_or_update(task->src_file_buckets, task->src_file_buckets_cap, normal_path_hash, curr_bucket);

      if (curr_bucket == insert_result) {
        // bucket was inserted into empty slot, reset current bucket
        curr_bucket = 0;
      } else if (curr_bucket != insert_result) {
        // reuse evicted bucket
        curr_bucket = insert_result;
      }

      // advance cursor
      cursor += sizeof(*header);
      cursor += header->len;
      cursor = AlignPow2(cursor, CV_FileCheckSumsAlign);
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal RDIB_Type *
lnk_find_container_type(String8 name, Rng1U64 tpi_itype_range, LNK_UDTNameBucket **udt_name_buckets, U64 udt_name_buckets_cap, RDIB_Type **tpi_itype_map)
{
  CV_TypeIndex container_itype = 0;

  String8 delim     = str8_lit("::");
  U64     delim_pos = str8_find_needle_reverse(name, 0, delim, 0);
  if (delim_pos > 0) {
    U64     container_name_size = delim_pos - delim.size;
    String8 container_name      = str8_prefix(name, container_name_size);
    container_itype = lnk_udt_name_hash_table_lookup_itype(udt_name_buckets, udt_name_buckets_cap, container_name);
  }

  RDIB_Type *container = 0;
  if (container_itype > 0) {
    Assert(container_itype < tpi_itype_range.max);
    container = tpi_itype_map[container_itype];
  }

  return container;
}

internal RDIB_Type *
lnk_type_from_itype(CV_TypeIndex itype, Rng1U64 tpi_itype_range, RDIB_Type **tpi_itype_map, LNK_Obj *obj, CV_SymKind symbol_kind, U64 symbol_offset)
{
  RDIB_Type *type = 0;
  if (itype < tpi_itype_range.max) {
    type = tpi_itype_map[itype];
  } else {
    lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Out of bounds type index 0x%x in S_%S @ 0x%llx.",
                  itype, cv_string_from_sym_kind(symbol_kind), symbol_offset);
  }
  return type;
}

internal U64
lnk_voff_from_sect_off(U64 sect_idx, U64 sect_off, COFF_SectionHeaderArray image_sects, LNK_Obj *obj, CV_SymKind symbol_kind, U64 symbol_offset)
{
  U64 voff = 0;
  if (sect_idx < image_sects.count) {
    voff = image_sects.v[sect_idx].voff + sect_off;
  } else {
    lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Out of bounds section index 0x%x in S_%S @ 0x%llx.",
                  sect_idx, cv_string_from_sym_kind(symbol_kind), symbol_offset);
  }
  return voff;
}

internal Rng1U64
lnk_virt_range_from_sect_off_size(U64 sect_idx, U64 sect_off, U64 size, COFF_SectionHeaderArray image_sects, LNK_Obj *obj, CV_SymKind symbol_kind, U64 symbol_offset)
{
  Rng1U64 virt_range = {0};
  if (sect_idx < image_sects.count) {
    U64 voff = image_sects.v[sect_idx].voff + sect_off;
    virt_range = rng_1u64(voff, voff + size);
  } else {
    lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Out of bounds section index 0x%x in S_%S @ 0x%llx.",
                  sect_idx, cv_string_from_sym_kind(symbol_kind), symbol_offset);
  }
  return virt_range;
}

internal void
lnk_error_on_invalid_defrange_symbol(LNK_Obj *obj, CV_Symbol symbol)
{
  lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Unable to parse symbol stream, unexpected S_%S without preceding S_LOCAL @ 0x%llx.",
                cv_string_from_sym_kind(symbol.kind), symbol.offset);
}

internal void
lnk_error_on_missing_cv_frameproc(LNK_Obj *obj, CV_Symbol symbol)
{
  lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Missing S_FRAMEPROC, unable to parse S_%S @ 0x%llx.",
                cv_string_from_sym_kind(symbol.kind), symbol.offset);
}

internal
THREAD_POOL_TASK_FUNC(lnk_find_obj_compiler_info_task)
{
  ProfBeginFunction();

  LNK_ConvertUnitToRDITask *task           = raw_task;
  CV_SymbolListArray        parsed_symbols = task->parsed_symbols[task_id];
  LNK_CodeViewCompilerInfo *comp_info      = &task->comp_info_arr[task_id];

  comp_info->arch          = (CV_Arch)~0u;
  comp_info->language      = (CV_Language)~0u;
  comp_info->compiler_name = str8_zero();

  // infer unit compiler data from S_COMPILE* which always follows S_OBJ
  for (U64 symbol_list_idx = 0; symbol_list_idx < parsed_symbols.count; ++symbol_list_idx) {
    CV_SymbolList symbol_list = parsed_symbols.v[symbol_list_idx];
    for (CV_SymbolNode *symbol_n = symbol_list.first; symbol_n != 0; symbol_n = symbol_n->next) {
      CV_Symbol symbol = symbol_n->data;
      if (symbol.kind == CV_SymKind_COMPILE) {
        AssertAlways(sizeof(CV_SymCompile) <= symbol.data.size);
        CV_SymCompile *compile = (CV_SymCompile *)symbol.data.str;
        comp_info->arch          = compile->machine;
        comp_info->language      = CV_CompileFlags_Extract_Language(compile->flags);
        comp_info->compiler_name = str8_cstring_capped(compile + 1, symbol.data.str + symbol.data.size);
        goto exit;
      } else if (symbol.kind == CV_SymKind_COMPILE2) {
        AssertAlways(sizeof(CV_SymCompile2) <= symbol.data.size);
        CV_SymCompile2 *compile2 = (CV_SymCompile2 *)symbol.data.str;
        comp_info->arch          = compile2->machine;
        comp_info->language      = CV_Compile2Flags_Extract_Language(compile2->flags);
        comp_info->compiler_name = str8_cstring_capped(compile2 + 1, symbol.data.str + symbol.data.size);
        goto exit;
      } else if (symbol.kind == CV_SymKind_COMPILE3) {
        AssertAlways(sizeof(CV_SymCompile3) <= symbol.data.size);
        CV_SymCompile3 *compile3 = (CV_SymCompile3 *)symbol.data.str;
        comp_info->arch          = compile3->machine;
        comp_info->language      = CV_Compile3Flags_Extract_Language(compile3->flags);
        comp_info->compiler_name = str8_cstring_capped(compile3 + 1, symbol.data.str + symbol.data.size);
        goto exit;
      }
    }
  }
  exit:;

  LNK_Obj *obj = task->obj_arr[task_id];

  // fill out unit info
  U64 unit_chunk_idx = task_id / task->unit_chunk_cap;
  U64 local_unit_idx = task_id - unit_chunk_idx * task->unit_chunk_cap;

  RDIB_Unit *dst     = &task->units[unit_chunk_idx].v[local_unit_idx];
  dst->arch          = rdi_arch_from_cv_arch(comp_info->arch);
  dst->unit_name     = str8_skip_last_slash(obj->path);
  dst->compiler_name = comp_info->compiler_name;
  dst->source_file   = str8_zero();
  dst->object_file   = push_str8_copy(arena, obj->path);
  dst->archive_file  = lnk_obj_get_lib_path(obj);
  dst->build_path    = str8_zero();
  dst->language      = rdi_language_from_cv_language(comp_info->language);

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_convert_line_tables_to_rdi_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  U64                       unit_idx = task_id;
  LNK_ConvertUnitToRDITask *task     = raw_task;
  LNK_Obj                  *obj      = task->obj_arr[unit_idx];
  CV_DebugS                 debug_s  = task->debug_s_arr[unit_idx];

  U64        unit_chunk_idx = unit_idx / task->unit_chunk_cap;
  U64        local_unit_idx = unit_idx - unit_chunk_idx * task->unit_chunk_cap;
  RDIB_Unit *dst            = &task->units[unit_chunk_idx].v[local_unit_idx];

  // find sub sections
  String8     raw_string_table = cv_string_table_from_debug_s(debug_s);
  String8     raw_file_chksms  = cv_file_chksms_from_debug_s(debug_s);
  String8List raw_lines_list   = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);

  // emit line table fragments for each source file from C13 line info
  dst->line_table = rdib_line_table_chunk_list_push(arena, &task->line_tables[worker_id], task->line_table_cap);

  for (String8Node *raw_lines_node = raw_lines_list.first; raw_lines_node != 0; raw_lines_node = raw_lines_node->next) {
    String8               raw_lines   = raw_lines_node->string;
    CV_C13LinesHeaderList parsed_list = cv_c13_lines_from_sub_sections(scratch.arena, raw_lines, rng_1u64(0, raw_lines.size));
    for (CV_C13LinesHeaderNode *lines_node = parsed_list.first; lines_node != 0; lines_node = lines_node->next) {
      CV_C13LinesHeader parsed_lines = lines_node->v;

      // parse checksum header
      if (parsed_lines.file_off + sizeof(CV_C13Checksum) > raw_file_chksms.size) {
        lnk_error_obj(LNK_Warning_IllData, obj, "Out of bounds $$FILE_CHECKSUM offset (0x%llx) in line table header.", parsed_lines.file_off);
        continue;
      }
      CV_C13Checksum *checksum_header = (CV_C13Checksum *) (raw_file_chksms.str + parsed_lines.file_off);
      if (parsed_lines.file_off + sizeof(CV_C13Checksum) + checksum_header->len > raw_file_chksms.size) {
        lnk_error_obj(LNK_Warning_IllData, obj, "Not enough bytes to read file checksum @ 0x%llx.", parsed_lines.file_off);
        continue;
      }
      String8 file_path      = str8_cstring_capped(raw_string_table.str + checksum_header->name_off, raw_string_table.str + raw_string_table.size);
      String8 checksum_bytes = str8((U8 *) (checksum_header + 1), checksum_header->len);

      // read out lines
      if (0 == parsed_lines.sec_idx || parsed_lines.sec_idx > task->image_sects.count) {
        lnk_error_obj(LNK_Warning_IllData, obj, "Out of bounds section index (%u) in $$LINES; skip line info for \"%S\".", parsed_lines.sec_idx, file_path);
        continue;
      }
      COFF_SectionHeader *sect  = &task->image_sects.v[parsed_lines.sec_idx];
      CV_LineArray        lines = cv_c13_line_array_from_data(arena, raw_lines, sect->voff, parsed_lines);

      // find source file for this line table
      String8               normal_path     = lnk_normalize_src_file_path(scratch.arena, file_path);
      U64                   src_file_hash   = lnk_src_file_hash_cv(normal_path, checksum_header->kind, checksum_bytes);
      LNK_SourceFileBucket *src_file_bucket = lnk_src_file_hash_table_lookup_slot(task->src_file_buckets, task->src_file_buckets_cap, src_file_hash, normal_path, checksum_header->kind, checksum_bytes);
      if (src_file_bucket == 0) {
        lnk_error_obj(LNK_Error_UnexpectedCodePath, obj, "Unable to find source file in the hash table: \"%S\".", file_path);
        continue;
      }
      RDIB_SourceFile *src_file = src_file_bucket->src_file;

      // fill out line table fragment and atomically insert
      RDIB_LineTableFragment *frag = rdib_line_table_push(arena, dst->line_table);
      frag->src_file   = src_file;
      frag->voffs      = lines.voffs;
      frag->line_nums  = lines.line_nums;
      frag->col_nums   = lines.col_nums;
      frag->line_count = lines.line_count;
      frag->col_count  = lines.col_count;

      // build list of line table fragments per file
      frag->next_src_file = ins_atomic_ptr_eval_assign(&src_file->line_table_frags, frag);
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_inlinee_lines_accels_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_ConvertUnitToRDITask *task    = raw_task;
  CV_DebugS                 debug_s = task->debug_s_arr[task_id];

  String8List                   raw_inlinee_lines   = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_InlineeLines);
  CV_C13InlineeLinesParsedList  inlinee_lines       = cv_c13_inlinee_lines_from_sub_sections(arena, raw_inlinee_lines);
  CV_InlineeLinesAccel         *inlinee_lines_accel = cv_c13_make_inlinee_lines_accel(arena, inlinee_lines);

  task->inlinee_lines_accel_arr[task_id] = inlinee_lines_accel;

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_convert_symbols_to_rdi_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_ConvertUnitToRDITask *task                = raw_task;
  LNK_SymbolInput  symbols_input       = task->symbol_inputs[task_id];
  LNK_Obj                  *obj                 = task->obj_arr[symbols_input.obj_idx];
  LNK_CodeViewCompilerInfo  comp_info           = task->comp_info_arr[symbols_input.obj_idx];
  CV_InlineeLinesAccel     *inlinee_lines_accel = task->inlinee_lines_accel_arr[symbols_input.obj_idx];

  RDI_Arch arch_rdi = rdi_arch_from_cv_arch(comp_info.arch);

  struct ScopeFrame {
    struct ScopeFrame *prev;
    RDIB_Scope        *scope;
    RDIB_Procedure    *proc;
    CV_ProcFlags       proc_flags;
    CV_SymFrameproc   *frameproc;
    U64                param_count;
    U64                regrel32_idx;
    RDIB_Variable     *defrange_target;
  };
#define push_scope_frame() do { \
  struct ScopeFrame *frame; \
  if (free_scope_stack != 0) { \
    frame = free_scope_stack; \
    SLLStackPop_N(free_scope_stack, prev); \
  } else { \
    frame = push_array(scratch.arena, struct ScopeFrame, 1); \
  } \
  SLLStackPush_N(scope_stack, frame, prev); \
} while (0)

  struct ScopeFrame *scope_stack      = 0;
  struct ScopeFrame *free_scope_stack = 0;

  // root frame
  push_scope_frame();

  for (CV_SymbolNode *symbol_n = symbols_input.symbol_list->first; symbol_n != 0; symbol_n = symbol_n->next) {
    CV_Symbol symbol = symbol_n->data;

    switch (symbol.kind) {
    case CV_SymKind_COMPILE:
    case CV_SymKind_COMPILE2:
    case CV_SymKind_COMPILE3: {
      // handled above
    } break;
    case CV_SymKind_INLINESITE_END:
    case CV_SymKind_END: {
      if (scope_stack != 0) {
        // move top frame to free stack
        struct ScopeFrame *free_frame = scope_stack;
        SLLStackPop_N(scope_stack, prev);
        SLLStackPush_N(free_scope_stack, free_frame, prev);
      } else {
        lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Encountered unbalanced blocks. Unable to finish symbol parse.");
        goto exit;
      }
    } break;
    case CV_SymKind_BLOCK32: {
      CV_SymBlock32 *block32    = (CV_SymBlock32 *) symbol.data.str;
      Rng1U64        virt_range = lnk_virt_range_from_sect_off_size(block32->sec, block32->off, block32->len, task->image_sects, obj, symbol.kind, symbol.offset);

      // push new scope node
      RDIB_Scope *scope = rdib_scope_chunk_list_push(arena, &task->scopes[worker_id], task->symbol_chunk_cap);

      // fill out scope
      scope->container_proc = scope_stack->proc;
      scope->parent         = scope_stack->scope;
      SLLQueuePush_N(scope_stack->scope->first_child, scope_stack->scope->last_child, scope, next_sibling);
      rng1u64_list_push(arena, &scope->ranges, virt_range);

#if 0
      if (scope->parent) {
        Assert(virt_range.min >= scope->parent->ranges.first->v.min);
        Assert(virt_range.max <= scope->parent->ranges.first->v.max);
      }
#endif

      // push new scope stack frame
      push_scope_frame();
      scope_stack->scope      = scope;
      scope_stack->proc       = scope->container_proc;
      scope_stack->proc_flags = scope_stack->proc_flags;
      scope_stack->frameproc  = scope_stack->prev->frameproc;
    } break;
    case CV_SymKind_GDATA32:
    case CV_SymKind_LDATA32: {
      CV_SymData32 *data32         = (CV_SymData32 *) symbol.data.str;
      String8       name           = str8_cstring_capped(data32 + 1, symbol.data.str + symbol.data.size);
      RDIB_Type    *type           = lnk_type_from_itype(data32->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
      RDIB_Type    *container_type = lnk_find_container_type(name, task->tpi_itype_range, task->udt_name_buckets, task->udt_name_buckets_cap, task->tpi_itype_map);
      U64           data_voff      = lnk_voff_from_sect_off(data32->sec, data32->off, task->image_sects, obj, symbol.kind, symbol.offset);

      B32 is_comp_gen = symbol.kind == CV_SymKind_LDATA32 && name.size == 0 && type == 0;
      if (!is_comp_gen) {

      // get link name through virtual offset look up
      String8 link_name = {0};
      if (symbol.kind == CV_SymKind_GDATA32) {
        BucketNode *pair = hash_table_search_u64(task->extern_symbol_voff_ht, data_voff);
        if (pair != 0) {
          LNK_Symbol *link_symbol = pair->v.value_raw;
          link_name = link_symbol->name;
        }
      }

      // make module relative location
      RDIB_LocationList locations = {0};
      {
        RDIB_EvalBytecode bytecode = {0};
        rdib_bytecode_push_op(arena, &bytecode, RDI_EvalOp_ModuleOff, data_voff);

        U64 data_size = rdib_size_from_type(type);
        if (data_size == 0) {
          data_size = max_U64;
        }

        Rng1U64List ranges = {0};
        rng1u64_list_push(arena, &ranges, rng_1u64(data_voff, data_voff + data_size));

        RDIB_Location location = rdib_make_location_addr_byte_stream(ranges, bytecode);
        rdib_location_list_push(arena, &locations, location);
      }

      RDIB_VariableChunkList *var_chunk_list = symbol.kind == CV_SymKind_GDATA32 ?
                                               &task->extern_gvars[worker_id] : &task->static_gvars[worker_id];

      // push new node
      RDIB_Variable *gvar  = rdib_variable_chunk_list_push(arena, var_chunk_list, task->symbol_chunk_cap);
      gvar->link_flags     = symbol.kind == CV_SymKind_GDATA32 ? RDI_LinkFlag_External : 0;
      gvar->name           = name;
      gvar->link_name      = link_name;
      gvar->type           = type;
      gvar->container_type = container_type;
      gvar->container_proc = scope_stack->proc;
      gvar->locations      = locations;
      }
    } break;
    case CV_SymKind_LTHREAD32:
    case CV_SymKind_GTHREAD32: {
      CV_SymThread32 *thread32       = (CV_SymThread32 *) symbol.data.str;
      String8         name           = str8_cstring_capped(thread32 + 1, symbol.data.str + symbol.data.size);
      RDIB_Type      *type           = lnk_type_from_itype(thread32->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
      RDIB_Type      *container_type = lnk_find_container_type(name, task->tpi_itype_range, task->udt_name_buckets, task->udt_name_buckets_cap, task->tpi_itype_map);

      // make TLS offset location
      RDIB_LocationList locations = {0};
      {
        RDIB_EvalBytecode bytecode = {0};
        rdib_bytecode_push_op(arena, &bytecode, RDI_EvalOp_TLSOff, thread32->tls_off);
          
        Rng1U64List ranges = {0};
        rng1u64_list_push(arena, &ranges, rng_1u64(0, max_U64));

        RDIB_Location location = rdib_make_location_addr_byte_stream(ranges, bytecode);
        rdib_location_list_push(arena, &locations, location);
      }

      // push new node
      RDIB_VariableChunkList *tvar_list = symbol.kind == CV_SymKind_GTHREAD32 ? &task->extern_tvars[worker_id] : &task->static_tvars[worker_id];
      RDIB_Variable          *tvar      = rdib_variable_chunk_list_push(arena, tvar_list, task->symbol_chunk_cap);

      // fill out thread variable
      tvar->link_flags     = symbol.kind == CV_SymKind_GTHREAD32 ? RDI_LinkFlag_External : 0;
      tvar->name           = name;
      tvar->link_name      = str8(0,0);
      tvar->type           = type;
      tvar->container_type = container_type;
      tvar->container_proc = scope_stack->proc;
      tvar->locations      = locations;
    } break;
    case CV_SymKind_LPROC32_ID:
    case CV_SymKind_GPROC32_ID: {
      AssertAlways(!"linker converts *_ID symbols in post-process step, if we ever get to this case then we have a bug in post-process step");
    } break;
    case CV_SymKind_LPROC32:
    case CV_SymKind_GPROC32: {
      CV_SymProc32 *proc32     = (CV_SymProc32 *) symbol.data.str;
      String8       name       = str8_cstring_capped(proc32 + 1, symbol.data.str + symbol.data.size);
      RDIB_Type    *type       = lnk_type_from_itype(proc32->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
      Rng1U64       virt_range = lnk_virt_range_from_sect_off_size(proc32->sec, proc32->off, proc32->len, task->image_sects, obj, symbol.kind, symbol.offset);

      // infer container type for method
      RDIB_Type *container_type = 0;
      if (type != 0) {
        if (type->kind == RDI_TypeKind_Method) {
          container_type = (RDIB_Type *) type->method.class_type;
        } else if (type->kind == RDI_TypeKindExt_StaticMethod) {
          container_type = (RDIB_Type *) type->static_method.class_type;
        }
      }

      // get link name through virtual offset look up
      String8 link_name = str8(0,0);
      if (symbol.kind == CV_SymKind_GPROC32) {
        LNK_Symbol *link_symbol = hash_table_search_u64_raw(task->extern_symbol_voff_ht, virt_range.min);
        if (link_symbol) {
          link_name = link_symbol->name;
        }
      }

      // scan ahead for context S_FRAMEPROC (must be defined in scope of PROC symbol)
      CV_SymFrameproc *frameproc = 0;
      {
        U64 depth = 1;
        for (CV_SymbolNode *lookahead = symbol_n->next; lookahead != 0; lookahead = lookahead->next) {
          if (lookahead->data.kind == CV_SymKind_FRAMEPROC) {
            frameproc = (CV_SymFrameproc *) lookahead->data.data.str;
            break;
          }
          if (cv_is_scope_symbol(lookahead->data.kind)) {
            ++depth;
          } else if (cv_is_end_symbol(lookahead->data.kind)) {
            --depth;
            if (depth == 0) {
              break;
            }
          }
        }
      }

      // push new procedure node
      RDIB_ProcedureChunkList *proc_list = symbol.kind == CV_SymKind_GPROC32 ? &task->extern_procs[worker_id] : &task->static_procs[worker_id];
      RDIB_Procedure          *proc      = rdib_procedure_chunk_list_push(arena, proc_list, task->symbol_chunk_cap);

      // push new scope node
      RDIB_Scope *root_scope     = rdib_scope_chunk_list_push(arena, &task->scopes[worker_id], task->symbol_chunk_cap);
      root_scope->container_proc = proc;
      root_scope->parent         = scope_stack->scope;
      if (scope_stack->scope != 0) {
        SLLQueuePush_N(scope_stack->scope->first_child, scope_stack->scope->last_child, root_scope, next_sibling);
      }
      rng1u64_list_push(arena, &root_scope->ranges, virt_range);

      // fill out procedure
      proc->link_flags     = symbol.kind == CV_SymKind_GPROC32 ? RDI_LinkFlag_External : 0;
      proc->name           = name;
      proc->link_name      = link_name;
      proc->type           = type;
      proc->container_type = container_type;
      proc->container_proc = scope_stack->proc;
      proc->scope          = root_scope;

      // push scope frame
      push_scope_frame();
      scope_stack->scope      = root_scope;
      scope_stack->proc       = proc;
      scope_stack->proc_flags = proc32->flags;
      scope_stack->frameproc  = frameproc;

      // set number of params for procedure on scope so we can figure out which S_REGREL32 is param
      {
        B32 is_proc_scope = (scope_stack->proc->scope == scope_stack->scope);
        if (is_proc_scope) {
          RDIB_Type *params = 0;
          if (scope_stack->proc != 0) {
            RDIB_Type *proc_type = scope_stack->proc->type;
            if (proc_type != 0) {
              if (proc_type->kind == RDI_TypeKind_NULL) {
                // compiler generates procedures with no type for __try/__except, lambdas, and etc.
              } else if (proc_type->kind == RDI_TypeKind_Function) {
                params = (RDIB_Type *)proc_type->func.params_type;
              } else if (proc_type->kind == RDI_TypeKind_Method) {
                params = (RDIB_Type *)proc_type->method.params_type;
              } else if (proc_type->kind == RDI_TypeKindExt_StaticMethod) {
                params = (RDIB_Type *)proc_type->static_method.params_type;
              } else {
                InvalidPath;
              }
            }
          }
          if (params != 0) {
            AssertAlways(params->kind == RDI_TypeKindExt_Params);
            scope_stack->param_count  = params->params.count;
            scope_stack->regrel32_idx = 0;
          }
        }
      }
    } break;
    case CV_SymKind_THUNK32: {
      CV_SymThunk32 *thunk32    = (CV_SymThunk32 *) symbol.data.str;
      String8        name       = str8_cstring_capped(thunk32 + 1, symbol.data.str + symbol.data.size);
      Rng1U64        virt_range = lnk_virt_range_from_sect_off_size(thunk32->sec, thunk32->off, thunk32->len, task->image_sects, obj, symbol.kind, symbol.offset);

      // scan ahead for context S_FRAMEPROC (must be defined in scope of PROC symbol)
      CV_SymFrameproc *frameproc = 0;
      {
        U64 depth = 1;
        for (CV_SymbolNode *lookahead = symbol_n->next; lookahead != 0; lookahead = lookahead->next) {
          if (lookahead->data.kind == CV_SymKind_FRAMEPROC) {
            frameproc = (CV_SymFrameproc *) lookahead->data.data.str;
            break;
          }
          if (cv_is_scope_symbol(lookahead->data.kind)) {
            ++depth;
          } else if (cv_is_end_symbol(lookahead->data.kind)) {
            --depth;
            if (depth == 0) {
              break;
            }
          }
        }
      }

      // push new procedure node
      RDIB_ProcedureChunkList *proc_list = &task->static_procs[worker_id];
      RDIB_Procedure          *thunk     = rdib_procedure_chunk_list_push(arena, proc_list, task->symbol_chunk_cap);

      // push new scope node
      RDIB_Scope *root_scope     = rdib_scope_chunk_list_push(arena, &task->scopes[worker_id], task->symbol_chunk_cap);
      root_scope->container_proc = thunk;
      root_scope->parent         = scope_stack->scope;
      if (scope_stack->scope != 0) {
        SLLQueuePush_N(scope_stack->scope->first_child, scope_stack->scope->last_child, root_scope, next_sibling);
      }
      rng1u64_list_push(arena, &root_scope->ranges, virt_range);

      // fill out procedure
      thunk->name  = name;
      thunk->type  = 0;
      thunk->scope = root_scope;

      // push scope frame
      push_scope_frame();
      scope_stack->scope      = root_scope;
      scope_stack->proc       = thunk;
      scope_stack->proc_flags = 0;
      scope_stack->frameproc  = frameproc;
    } break;
    case CV_SymKind_REGREL32: {
      if (~scope_stack->proc_flags & CV_ProcFlag_OptDbgInfo) {
        CV_SymRegrel32 *regrel32 = (CV_SymRegrel32 *) symbol.data.str;
        String8         name     = str8_cstring_capped(regrel32 + 1, symbol.data.str + symbol.data.size);
        RDIB_Type      *type     = lnk_type_from_itype(regrel32->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);

        RDI_LocalKind local_kind = RDI_LocalKind_Variable;
        B32           is_ref     = 0;
        if (scope_stack->regrel32_idx < scope_stack->param_count) {
          local_kind = RDI_LocalKind_Parameter;
          if (type != 0) { 
            U64 byte_size = rdib_size_from_type(type);
            switch (comp_info.arch) {
            case CV_Arch_8086: is_ref = byte_size > 4 || !IsPow2OrZero(byte_size); break;
            case CV_Arch_X64:  is_ref = byte_size > 8 || !IsPow2OrZero(byte_size); break;
            default: NotImplemented;
            }
          }
        }

        // push node
        RDIB_Variable *local = rdib_variable_chunk_list_push(arena, &task->locals[worker_id], task->symbol_chunk_cap);
        SLLQueuePush(scope_stack->scope->local_first, scope_stack->scope->local_last, local);
        ++scope_stack->scope->local_count;

        // fill out local
        local->link_flags = 0;
        local->name       = name;
        local->kind       = local_kind;
        local->type       = type;

        // encode location
        RDI_RegCode reg_code   = rdi_reg_code_from_cv(comp_info.arch, regrel32->reg);
        U32         value_size = 8;
        U32         value_pos  = 0;
        rdib_push_location_addr_reg_off(arena, &local->locations, arch_rdi, reg_code, value_size, value_pos, (S64)regrel32->reg_off, is_ref, scope_stack->scope->ranges);

        // advance reg rel index
        ++scope_stack->regrel32_idx;
      }
    } break;
    case CV_SymKind_LOCAL: {
      CV_SymLocal *sym_local = (CV_SymLocal *) symbol.data.str;
      String8      name      = str8_cstring_capped(sym_local + 1, symbol.data.str + symbol.data.size);
      RDIB_Type   *type      = lnk_type_from_itype(sym_local->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);

      // reset defrange target
      scope_stack->defrange_target = 0;

      if (sym_local->flags & CV_LocalFlag_Global) {
        // TODO: apply global modifications
      } else if (sym_local->flags & CV_LocalFlag_Static) {
        // TODO: apply local modifications
      }

      // push New node
      RDIB_Variable *local = rdib_variable_chunk_list_push(arena, &task->locals[worker_id], task->symbol_chunk_cap);
      SLLQueuePush(scope_stack->scope->local_first, scope_stack->scope->local_last, local);
      ++scope_stack->scope->local_count;

      // fill out local
      local->link_flags = 0;
      local->kind       = sym_local->flags & CV_LocalFlag_Param ? RDI_LocalKind_Parameter : RDI_LocalKind_Variable;
      local->name       = name;
      local->type       = type;

      scope_stack->defrange_target = local;
    } break;
    case CV_SymKind_FILESTATIC: {
      CV_SymFileStatic *file_static = (CV_SymFileStatic *) symbol.data.str;
      String8           name        = str8_cstring_capped(file_static + 1, symbol.data.str + symbol.data.size);
      RDIB_Type        *type        = lnk_type_from_itype(file_static->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);

      // push New node
      RDIB_Variable *local = rdib_variable_chunk_list_push(arena, &task->locals[worker_id], task->symbol_chunk_cap);
      SLLQueuePush(scope_stack->scope->local_first, scope_stack->scope->local_last, local);
      ++scope_stack->scope->local_count;

      // fill out local
      local->link_flags = 0;
      local->kind       = RDI_LocalKind_Variable;
      local->name       = name;
      local->type       = type;

      // set target for following defrange modifications
      scope_stack->defrange_target = local;
    } break;
    case CV_SymKind_DEFRANGE_REGISTER: {
      if (scope_stack->defrange_target == 0) {
        lnk_error_on_invalid_defrange_symbol(obj, symbol);
        break;
      }

      CV_SymDefrangeRegister *defrange_reg = (CV_SymDefrangeRegister *) symbol.data.str;
      RDI_RegCode             reg_code     = rdi_reg_code_from_cv(comp_info.arch, defrange_reg->reg);
      CV_LvarAddrGap         *gaps         = (CV_LvarAddrGap *) (defrange_reg + 1);
      U64                     gap_count    = (symbol.data.size - sizeof(*defrange_reg)) / sizeof(*gaps);

      Rng1U64       defrange = lnk_virt_range_from_sect_off_size(defrange_reg->range.sec, defrange_reg->range.off, defrange_reg->range.len, task->image_sects, obj, symbol.kind, symbol.offset);
      Rng1U64List   ranges   = cv_make_defined_range_list_from_gaps(arena, defrange, gaps, gap_count);
      RDIB_Location location = rdib_make_location_val_reg(ranges, reg_code);

      rdib_location_list_push(arena, &scope_stack->defrange_target->locations, location);
    } break;
    case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL: {
      if (scope_stack->defrange_target == 0) {
        lnk_error_on_invalid_defrange_symbol(obj, symbol);
        break;
      }
      if (scope_stack->frameproc == 0) {
        lnk_error_on_missing_cv_frameproc(obj, symbol);
        break;
      }

      CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel *)symbol.data.str;
      CV_LvarAddrGap                *gaps           = (CV_LvarAddrGap *) (defrange_fprel + 1);
      U64                            gap_count      = (symbol.data.size - sizeof(*defrange_fprel)) / sizeof(gaps[0]);

      B32                   is_local_param = scope_stack->defrange_target->kind == RDI_LocalKind_Parameter;
      CV_EncodedFramePtrReg encoded_fp_reg = cv_pick_fp_encoding(scope_stack->frameproc, is_local_param);
      CV_Reg                fp_reg         = cv_decode_fp_reg(comp_info.arch, encoded_fp_reg);
      RDI_RegCode           fp_reg_rdi     = rdi_reg_code_from_cv(comp_info.arch, fp_reg);
      Rng1U64               defrange       = lnk_virt_range_from_sect_off_size(defrange_fprel->range.sec, defrange_fprel->range.off, defrange_fprel->range.len, task->image_sects, obj, symbol.kind, symbol.offset);
      Rng1U64List           ranges         = cv_make_defined_range_list_from_gaps(arena, defrange, gaps, gap_count);
      U32                   value_pos      = 0;
      U32                   value_size     = rdi_addr_size_from_arch(arch_rdi);

      rdib_push_location_addr_reg_off(arena, &scope_stack->defrange_target->locations, arch_rdi, fp_reg_rdi, value_size, value_pos, (S64)defrange_fprel->off, 0, ranges);
    } break;
    case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER: {
      if (scope_stack->defrange_target == 0) {
        lnk_error_on_invalid_defrange_symbol(obj, symbol);
        break;
      }

      CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister *) symbol.data.str;
      CV_LvarAddrGap                 *gaps                       = (CV_LvarAddrGap *) (defrange_subfield_register + 1);
      U64                             gap_count                  = (symbol.data.size - sizeof(*defrange_subfield_register)) / sizeof(gaps[0]);
      RDI_RegCode                     reg_rdi                    = rdi_reg_code_from_cv(comp_info.arch, defrange_subfield_register->reg);
      U32                             value_pos                  = CV_DefrangeSubfieldRegister_Extract_ParentOffset(defrange_subfield_register->field_offset);
      U32                             value_size                 = cv_size_from_reg(comp_info.arch, defrange_subfield_register->reg) - value_pos;
      Rng1U64                         defrange                   = lnk_virt_range_from_sect_off_size(defrange_subfield_register->range.sec, defrange_subfield_register->range.off, defrange_subfield_register->range.len, task->image_sects, obj, symbol.kind, symbol.offset);
      Rng1U64List                     ranges                     = cv_make_defined_range_list_from_gaps(arena, defrange, gaps, gap_count);

      rdib_push_location_addr_reg_off(arena, &scope_stack->defrange_target->locations, arch_rdi, reg_rdi, value_size, value_pos, 0, 0, ranges);
    } break;
    case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE: {
      if (scope_stack->defrange_target == 0) {
        lnk_error_on_invalid_defrange_symbol(obj, symbol);
        break;
      }
      if (scope_stack->frameproc == 0) {
        lnk_error_on_missing_cv_frameproc(obj, symbol);
        break;
      }

      CV_SymDefrangeFramepointerRelFullScope *defrange_fprelfs = (CV_SymDefrangeFramepointerRelFullScope *) symbol.data.str; 
      B32                                     is_local_param   = scope_stack->defrange_target->kind == RDI_LocalKind_Parameter;
      CV_EncodedFramePtrReg                   encoded_fp_reg   = cv_pick_fp_encoding(scope_stack->frameproc, is_local_param);
      CV_Reg                                  fp_reg           = cv_decode_fp_reg(comp_info.arch, encoded_fp_reg);
      RDI_RegCode                             fp_reg_rdi       = rdi_reg_code_from_cv(comp_info.arch, fp_reg);
      U32                                     value_size       = cv_size_from_reg(comp_info.arch, fp_reg);
      U32                                     value_pos        = 0;
      Rng1U64List                             ranges           = scope_stack->scope->ranges; // variable is available everywhere in the scope

      rdib_push_location_addr_reg_off(arena, &scope_stack->defrange_target->locations, arch_rdi, fp_reg_rdi, value_size, value_pos, (S64)defrange_fprelfs->off, 0, ranges);
    } break;
    case CV_SymKind_DEFRANGE_REGISTER_REL: {
      if (scope_stack->defrange_target == 0) {
        lnk_error_on_invalid_defrange_symbol(obj, symbol);
        break;
      }

      CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel *) symbol.data.str;
      CV_LvarAddrGap            *gaps                  = (CV_LvarAddrGap *) (defrange_register_rel + 1);
      U64                        gap_count             = (symbol.data.size - sizeof(*defrange_register_rel)) / sizeof(gaps[0]);
      RDI_RegCode                reg_rdi               = rdi_reg_code_from_cv(comp_info.arch, defrange_register_rel->reg);
      U64                        value_size            = cv_size_from_reg(comp_info.arch, defrange_register_rel->reg);
      U64                        value_pos             = 0;
      Rng1U64                    defrange              = lnk_virt_range_from_sect_off_size(defrange_register_rel->range.sec, defrange_register_rel->range.off, defrange_register_rel->range.len, task->image_sects, obj, symbol.kind, symbol.offset);
      Rng1U64List                ranges                = cv_make_defined_range_list_from_gaps(arena, defrange, gaps, gap_count);

      rdib_push_location_addr_reg_off(arena, &scope_stack->defrange_target->locations, arch_rdi, reg_rdi, value_size, value_pos, (S64)defrange_register_rel->reg_off, 0, ranges);
    } break;
    case CV_SymKind_INLINESITE: {
      CV_SymInlineSite *sym_inline_site = (CV_SymInlineSite *) symbol.data.str;
      String8           binary_annots   = str8_skip(symbol.data, sizeof(*sym_inline_site));

      U64 parent_voff = 0;
      if (scope_stack != 0) {
        RDIB_Scope *proc_scope = scope_stack->proc->scope;
        Assert(proc_scope->ranges.count == 1);
        Rng1U64 scope_vrange = proc_scope->ranges.first->v;
        parent_voff = scope_vrange.min;
      } else {
        Assert(!"S_INLINESITE doesn't have a parent procedure symbol");
      }

      // parse binary annots
      CV_C13InlineeLinesParsed    *inlinee_parsed       = cv_c13_inlinee_lines_accel_find(inlinee_lines_accel, sym_inline_site->inlinee);
      CV_InlineBinaryAnnotsParsed  binary_annots_parsed = cv_c13_parse_inline_binary_annots(arena, parent_voff, inlinee_parsed, binary_annots);

      String8    name  = str8_zero();
      RDIB_Type *type  = 0;
      RDIB_Type *owner = 0;
      if (task->ipi_itype_range.min <= sym_inline_site->inlinee && sym_inline_site->inlinee < task->ipi_itype_range.max) {
        U64     leaf_idx = sym_inline_site->inlinee - task->tpi_itype_range.min;
        CV_Leaf leaf     = cv_leaf_from_ptr(task->leaf_arr_ipi[leaf_idx]);
        if (leaf.kind == CV_LeafKind_MFUNC_ID) {
          if (sizeof(CV_LeafMFuncId) <= leaf.data.size) {
            CV_LeafMFuncId *mfunc_id = (CV_LeafMFuncId *) leaf.data.str;
            name  = str8_cstring_capped_reverse(mfunc_id + 1, leaf.data.str + leaf.data.size);
            type  = lnk_type_from_itype(mfunc_id->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
            owner = lnk_type_from_itype(mfunc_id->owner_itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
          } else {
            Assert(!"invalid leaf size");
          }
        } else if (leaf.kind == CV_LeafKind_FUNC_ID) {
          if (sizeof(CV_LeafFuncId) <= leaf.data.size) {
            CV_LeafFuncId *func_id = (CV_LeafFuncId *) leaf.data.str;
            name  = str8_cstring_capped_reverse(func_id + 1, leaf.data.str + leaf.data.size);
            type  = lnk_type_from_itype(func_id->itype, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
            owner = lnk_type_from_itype(func_id->scope_string_id, task->tpi_itype_range, task->tpi_itype_map, obj, symbol.kind, symbol.offset);
          } else {
            Assert(!"invalid leaf size");
          }
        } else {
          Assert(!"inlinee must pointer to LF_FUNC_ID or LF_MFUNC_ID");
        }
      } else {
        Assert(!"out of bounds inlinee");
      }

      // fill out inline site
      RDIB_InlineSite *inline_site = rdib_inline_site_chunk_list_push(arena, &task->inline_sites[worker_id], task->inline_site_cap);
      inline_site->name            = name;
      inline_site->type            = type;
      inline_site->owner           = owner;
      
      inline_site->convert_ref.ud0 = binary_annots_parsed.lines;
      inline_site->convert_ref.ud1 = binary_annots_parsed.lines_count;
      inline_site->convert_ref.ud2 = symbols_input.obj_idx;

      // fill out scope
      RDIB_Scope *scope = rdib_scope_chunk_list_push(arena, &task->scopes[worker_id], task->symbol_chunk_cap);
      scope->container_proc = scope_stack->proc;
      scope->parent         = scope_stack->scope;
      scope->inline_site    = inline_site;
      scope->ranges         = binary_annots_parsed.code_ranges;

      // push new scope stack frame
      push_scope_frame();
      scope_stack->scope      = scope;
      scope_stack->proc       = scope->container_proc;
      scope_stack->proc_flags = scope_stack->proc_flags;
      scope_stack->frameproc  = scope_stack->prev->frameproc;
    } break;
    default: break;
    }
  }
  exit:;

#undef push_scope_frame

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_convert_inline_site_line_tables_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_ConvertUnitToRDITask *task  = raw_task;
  RDIB_InlineSiteChunk     *chunk = task->inline_site_chunks[task_id];

  RDIB_LineTableFragmentChunkList frag_chunk_list = {0};

  for (U64 i = 0; i < chunk->count; ++i) {
    RDIB_InlineSite *inline_site = &chunk->v[i];

    CV_LineArray *lines_arr   = inline_site->convert_ref.ud0;
    U64           lines_count = inline_site->convert_ref.ud1;
    U64           obj_idx     = inline_site->convert_ref.ud2;

    CV_DebugS debug_s          = task->debug_s_arr[obj_idx];
    String8   raw_string_table = cv_string_table_from_debug_s(debug_s);
    String8   raw_file_chksms  = cv_file_chksms_from_debug_s(debug_s);

    if (lines_count > 0) {
      inline_site->line_table = rdib_line_table_chunk_list_push(arena, &task->line_tables[worker_id], task->line_table_cap);
    } else {
      inline_site->line_table = task->null_line_table;
    }

    // emit line tables for each file (yes, it is possbile to split inline site among two or more files via #include)
    for (U64 file_idx = 0; file_idx < lines_count; ++file_idx) {
      CV_LineArray lines = lines_arr[file_idx];

      // prase checksum header
      CV_C13Checksum *checksum_header = (CV_C13Checksum *) (raw_file_chksms.str + lines.file_off);
      if (lines.file_off + sizeof(CV_C13Checksum) + checksum_header->len > raw_file_chksms.size) {
        lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], "Not enough bytes to read file checksum @ 0x%llx.", lines.file_off);
        continue;
      }
      String8 file_path      = str8_cstring_capped(raw_string_table.str + checksum_header->name_off, raw_string_table.str + raw_string_table.size);
      String8 checksum_bytes = str8((U8 *) (checksum_header + 1), checksum_header->len);
      
      // find source file for this line table
      String8               normal_path     = lnk_normalize_src_file_path(scratch.arena, file_path);
      U64                   src_file_hash   = lnk_src_file_hash_cv(normal_path, checksum_header->kind, checksum_bytes);
      LNK_SourceFileBucket *src_file_bucket = lnk_src_file_hash_table_lookup_slot(task->src_file_buckets, task->src_file_buckets_cap, src_file_hash, normal_path, checksum_header->kind, checksum_bytes);
      if (src_file_bucket == 0) {
        lnk_error_obj(LNK_Error_UnexpectedCodePath, task->obj_arr[obj_idx], "Unable to find source file in the hash table: \"%S\".", file_path);
        continue;
      }
      RDIB_SourceFile *src_file = src_file_bucket->src_file;

      // fill out line table fragment
      RDIB_LineTableFragment *frag = rdib_line_table_fragment_chunk_list_push(arena, &frag_chunk_list, chunk->count);
      frag->src_file   = src_file;
      frag->voffs      = lines.voffs;
      frag->line_nums  = lines.line_nums;
      frag->col_nums   = lines.col_nums;
      frag->line_count = lines.line_count;
      frag->col_count  = lines.col_count;

      // build list of fragments per line table
      rdib_line_table_push_fragment_node(inline_site->line_table, frag);

      // build list of line table fragments per file
      frag->next_src_file = ins_atomic_ptr_eval_assign(&src_file->line_table_frags, frag);
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_collect_obj_virtual_ranges_task)
{
  ProfBeginFunction();

  LNK_ConvertUnitToRDITask *task = raw_task;

  U64      unit_idx = task_id;
  LNK_Obj *obj      = task->obj_arr[unit_idx];

  U64 unit_chunk_idx = unit_idx / task->unit_chunk_cap;
  U64 local_unit_idx = unit_idx - unit_chunk_idx * task->unit_chunk_cap;

  RDIB_Unit *dst        = &task->units[unit_chunk_idx].v[local_unit_idx];
  dst->virt_range_count = 0;
  dst->virt_ranges      = push_array_no_zero(arena, Rng1U64, obj->header.section_count_no_null);

  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;

  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];

    if (sect_header->flags & COFF_SectionFlag_LnkRemove) {
      continue;
    }
    if (sect_header->vsize == 0) {
      continue;
    }

    dst->virt_ranges[dst->virt_range_count] = rng_1u64(sect_header->voff, sect_header->voff + sect_header->vsize);
    ++dst->virt_range_count;
  }

  // free unused memory
  arena_pop(arena, sizeof(dst->virt_ranges[0]) * (obj->header.section_count_no_null - dst->virt_range_count));

  ProfEnd();
}

internal String8List
lnk_build_rad_debug_info(TP_Context               *tp,
                         TP_Arena                 *tp_arena,
                         OperatingSystem           os,
                         RDI_Arch                  arch,
                         String8                   image_name,
                         String8                   image_data,
                         U64                       obj_count,
                         LNK_Obj                 **obj_arr,
                         CV_DebugS                *debug_s_arr,
                         U64                       symbol_input_count,
                         LNK_SymbolInput          *symbol_inputs,
                         CV_SymbolListArray       *parsed_symbols,
                         LNK_MergedTypes           types)
{
  ProfBegin("RDI");
  Temp scratch = scratch_begin(tp_arena->v,tp_arena->count);

  RDIB_Input input = rdib_init_input(scratch.arena);

  COFF_SectionHeaderArray image_sects;
  String8                 image_strtab;
  {
    PE_BinInfo pe = pe_bin_info_from_data(scratch.arena, image_data);
    image_sects.count = pe.section_count;
    image_sects.v = (COFF_SectionHeader *)str8_substr(image_data, pe.section_table_range).str;
    image_strtab = str8_substr(image_data, pe.string_table_range);
  }

  ProfBegin("Top Level Info");
  {
    U64 image_vsize = 0;
    for (U64 sect_idx = 0; sect_idx < image_sects.count; sect_idx++) {
      COFF_SectionHeader *sect = &image_sects.v[sect_idx];
      image_vsize = Max(image_vsize, sect->voff + sect->vsize);
    }

    input.top_level_info.arch            = arch;
    input.top_level_info.exe_name        = image_name;
    input.top_level_info.exe_hash        = rdi_hash(image_data.str, image_data.size);
    input.top_level_info.voff_max        = image_vsize;
    input.top_level_info.producer_string = push_str8f(scratch.arena, "%s [Debug Info: CodeView]", BUILD_VERSION_STRING_LITERAL);
  }
  ProfEnd();

  ProfBegin("Sections");
  {
    input.sect_count = image_sects.count;
    input.sections   = push_array(scratch.arena, RDIB_BinarySection, image_sects.count);
    for (U64 sect_idx = 0; sect_idx < image_sects.count; ++sect_idx) {
      COFF_SectionHeader *src = &image_sects.v[sect_idx];
      RDIB_BinarySection *dst = &input.sections[sect_idx];
      String8 sect_name = coff_name_from_section_header(image_strtab, src);

      dst->name       = push_str8_copy(scratch.arena, sect_name);
      dst->flags      = rdi_binary_section_flags_from_coff_section_flags(src->flags);
      dst->voff_first = src->voff;
      dst->voff_opl   = src->voff + src->vsize;
      dst->foff_first = src->foff;
      dst->foff_opl   = src->foff + src->fsize;
    }
  }
  ProfEnd();

  // assing low and high type indices per source
  Rng1U64 itype_ranges[CV_TypeIndexSource_COUNT];
  for (U64 i = 0; i < ArrayCount(itype_ranges); ++i) {
    itype_ranges[i] = rng_1u64(CV_MinComplexTypeIndex, CV_MinComplexTypeIndex + types.count[i]);
  }

  ProfBegin("Convert Types");
  U64                 udt_name_buckets_cap;
  LNK_UDTNameBucket **udt_name_buckets;
  RDIB_Type         **tpi_itype_map;
  {
    ProfBegin("Push TPI itype -> RDIB Type map");
    tpi_itype_map = push_array(scratch.arena, RDIB_Type *, itype_ranges[CV_TypeIndexSource_TPI].max);
    ProfEnd();

    ProfBegin("Push Built-in Types");
    RDIB_DataModel data_model = rdib_infer_data_model(os, arch);
    lnk_push_basic_itypes(scratch.arena, data_model, tpi_itype_map, &input.types);
    ProfEnd();

    Assert(tpi_itype_map[0] == 0);
    tpi_itype_map[0] = input.null_type;

    ProfBegin("Build UDT Name Hash Table");
    // TODO: fix memory life-time
    udt_name_buckets_cap = 0;
    udt_name_buckets     = lnk_udt_name_hash_table_from_leaf_arr(tp, tp_arena, types.count[CV_TypeIndexSource_TPI], types.v[CV_TypeIndexSource_TPI], &udt_name_buckets_cap);
    ProfEnd();


    ProfBegin("Convert CodeView types to RDIB Types");
    LNK_ConvertTypesToRDI task         = {0};
    MemoryCopyTyped(task.leaf_count, types.count, CV_TypeIndexSource_COUNT);
    MemoryCopyTyped(task.leaf_arr,   types.v,     CV_TypeIndexSource_COUNT);
    task.type_cap                      = input.type_cap;
    task.udt_cap                       = input.udt_cap;
    task.variadic_type_ref             = rdib_make_type_ref(scratch.arena, input.variadic_type);
    task.itype_ranges                  = itype_ranges;
    task.tpi_itype_map                 = tpi_itype_map;
    task.udt_name_bucket_cap           = udt_name_buckets_cap;
    task.udt_name_buckets              = udt_name_buckets;
    task.rdib_types_lists              = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_struct_lists       = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_union_lists        = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_enum_lists         = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_udt_members_lists  = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_enum_members_lists = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_types_params_lists       = push_array(scratch.arena, RDIB_TypeChunkList,      tp->worker_count);
    task.rdib_udt_members_lists        = push_array(scratch.arena, RDIB_UDTMemberChunkList, tp->worker_count);
    task.rdib_enum_members_lists       = push_array(scratch.arena, RDIB_UDTMemberChunkList, tp->worker_count);
    task.ranges                        = tp_divide_work(scratch.arena, types.count[CV_TypeIndexSource_TPI], tp->worker_count);
    tp_for_parallel(tp, tp_arena, tp->worker_count, lnk_convert_types_to_rdi_task, &task);
    ProfEnd();

    ProfBegin("Concat converted types");
    rdib_type_chunk_list_concat_in_place_many      (&input.types,             task.rdib_types_lists,              tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.struct_list,       task.rdib_types_struct_lists,       tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.union_list,        task.rdib_types_union_lists,        tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.enum_list,         task.rdib_types_enum_lists,         tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.param_types,       task.rdib_types_params_lists,       tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.member_types,      task.rdib_types_udt_members_lists,  tp->worker_count);
    rdib_type_chunk_list_concat_in_place_many      (&input.enum_types,        task.rdib_types_enum_members_lists, tp->worker_count);
    rdib_udt_member_chunk_list_concat_in_place_many(&input.udt_members,       task.rdib_udt_members_lists,        tp->worker_count);
    rdib_udt_member_chunk_list_concat_in_place_many(&input.enum_members,      task.rdib_enum_members_lists,       tp->worker_count);
    ProfEnd();

    // types are converted and we can remove indirection and release 'itype_map'
    ProfBegin("Deref Type Refs");
    rdib_deref_type_refs(tp, &input.types);
    rdib_deref_type_refs(tp, &input.struct_list);
    rdib_deref_type_refs(tp, &input.union_list);
    rdib_deref_type_refs(tp, &input.enum_list); 
    rdib_deref_type_refs(tp, &input.param_types);
    rdib_deref_type_refs(tp, &input.member_types);
    rdib_deref_type_refs(tp, &input.enum_types);
    ProfEnd();
  }
  ProfEnd();

  // Loop over source files in objs and build a hash table
  // for path -> source file maps. During symbol conversion
  // we use the hash table to lookup source files and append
  // inline site line tables.
  U64                    src_file_buckets_cap;
  LNK_SourceFileBucket **src_file_buckets;
  {
    ProfBegin("Build Source File Hash Table");

    LNK_ConvertSourceFilesToRDITask task = {0};
    task.obj_arr     = obj_arr;
    task.debug_s_arr = debug_s_arr;

    ProfBegin("Count Source Files");
    tp_for_parallel(tp, 0, obj_count, lnk_count_source_files_task, &task);
    ProfEnd();

    ProfBeginDynamic("Insert Source Files [Count %llu]", task.total_src_file_count);
    task.src_file_buckets_cap = (U64)(task.total_src_file_count * 1.3);
    task.src_file_buckets     = push_array(tp_arena->v[0], LNK_SourceFileBucket*, task.src_file_buckets_cap);
    tp_for_parallel(tp, tp_arena, obj_count, lnk_insert_src_files_task, &task);
    ProfEnd();

    src_file_buckets_cap = task.src_file_buckets_cap;
    src_file_buckets     = task.src_file_buckets;

    ProfEnd();
  }

  // Copy source files to a contiguous array and update source file pointers
  // in buckets so we can do lookup and compute source file index in output array
  // with a pointer subtraction.
  ProfBegin("Source Files");
  for (U64 bucket_idx = 0; bucket_idx < src_file_buckets_cap; ++bucket_idx) {
    LNK_SourceFileBucket *bucket = src_file_buckets[bucket_idx];
    if (bucket != 0) {
      RDIB_SourceFile *new_src_file = rdib_source_file_chunk_list_push(scratch.arena, &input.src_files, input.src_file_chunk_cap);

      // restore chunk pointer after copy
      RDIB_SourceFileChunk *new_src_file_chunk = new_src_file->chunk;
      *new_src_file       = *bucket->src_file;
      new_src_file->chunk = new_src_file_chunk;

      bucket->src_file = new_src_file;
    }
  }
  ProfEnd();

  ProfBegin("Units");
  {
    LNK_ConvertUnitToRDITask task = {0};
    task.image_sects              = image_sects;
    task.obj_arr                  = obj_arr;
    task.debug_s_arr              = debug_s_arr;
    task.leaf_arr_count_ipi       = types.count[CV_TypeIndexSource_IPI];
    task.leaf_arr_ipi             = types.v[CV_TypeIndexSource_IPI];
    task.symbol_inputs            = symbol_inputs;
    task.parsed_symbols           = parsed_symbols;
    task.ipi_itype_range          = itype_ranges[CV_TypeIndexSource_IPI];
    task.tpi_itype_range          = itype_ranges[CV_TypeIndexSource_TPI];
    task.tpi_itype_map            = tpi_itype_map;
    task.src_file_buckets_cap     = src_file_buckets_cap;
    task.src_file_buckets         = src_file_buckets;
    task.udt_name_buckets         = udt_name_buckets;
    task.udt_name_buckets_cap     = udt_name_buckets_cap;
    task.src_file_chunk_cap       = input.src_file_chunk_cap;
    task.line_table_cap           = input.line_table_cap;
    task.symbol_chunk_cap         = input.symbol_chunk_cap;
    task.unit_chunk_cap           = input.unit_chunk_cap;
    task.inline_site_cap          = input.inline_site_cap;
    task.null_line_table          = input.null_line_table;
    task.extern_symbol_voff_ht    = hash_table_init(scratch.arena, 256);
    task.units                    = rdib_unit_chunk_list_reserve_ex(scratch.arena, &input.units, input.unit_chunk_cap, obj_count);
    task.scopes                   = push_array(scratch.arena, RDIB_ScopeChunkList,      tp->worker_count);
    task.locals                   = push_array(scratch.arena, RDIB_VariableChunkList,   tp->worker_count);
    task.extern_gvars             = push_array(scratch.arena, RDIB_VariableChunkList,   tp->worker_count);
    task.static_gvars             = push_array(scratch.arena, RDIB_VariableChunkList,   tp->worker_count);
    task.extern_tvars             = push_array(scratch.arena, RDIB_VariableChunkList,   tp->worker_count);
    task.static_tvars             = push_array(scratch.arena, RDIB_VariableChunkList,   tp->worker_count);
    task.extern_procs             = push_array(scratch.arena, RDIB_ProcedureChunkList,  tp->worker_count);
    task.static_procs             = push_array(scratch.arena, RDIB_ProcedureChunkList,  tp->worker_count);
    task.inline_sites             = push_array(scratch.arena, RDIB_InlineSiteChunkList, tp->worker_count);
    task.line_tables              = push_array(scratch.arena, RDIB_LineTableChunkList,  tp->worker_count);

    ProfBegin("Gather Compiler Info");
    task.comp_info_arr = push_array(scratch.arena, LNK_CodeViewCompilerInfo, obj_count);
    tp_for_parallel(tp, tp_arena, obj_count, lnk_find_obj_compiler_info_task, &task);
    ProfEnd();

    ProfBegin("Convert Line Tables");
    tp_for_parallel(tp, tp_arena, obj_count, lnk_convert_line_tables_to_rdi_task, &task);
    ProfEnd();

    ProfBegin("Build Inlinee Lines Accels");
    task.inlinee_lines_accel_arr = push_array(scratch.arena, CV_InlineeLinesAccel *, obj_count);
    tp_for_parallel(tp, tp_arena, obj_count, lnk_build_inlinee_lines_accels_task, &task);
    ProfEnd();

    ProfBegin("Convert Symbols");
    tp_for_parallel(tp, tp_arena, symbol_input_count, lnk_convert_symbols_to_rdi_task, &task);
    ProfEnd();

    ProfBegin("Convert Inline Sites Line Tables");
    rdib_inline_site_chunk_list_concat_in_place_many(&input.inline_sites, task.inline_sites, tp->worker_count);
    task.inline_site_chunks = rdib_array_from_inline_site_chunk_list(scratch.arena, input.inline_sites);
    tp_for_parallel(tp, tp_arena, input.inline_sites.count, lnk_convert_inline_site_line_tables_task, &task);
    ProfEnd();

    ProfBegin("Collect Units Virtual Ranges");
    tp_for_parallel(tp, tp_arena, obj_count, lnk_collect_obj_virtual_ranges_task, &task);
    ProfEnd();

    rdib_line_table_chunk_list_concat_in_place_many(&input.line_tables, task.line_tables, tp->worker_count);
    rdib_scope_chunk_list_concat_in_place_many(&input.scopes, task.scopes, tp->worker_count);
    rdib_variable_chunk_list_concat_in_place_many(&input.locals, task.locals, tp->worker_count);
    rdib_variable_chunk_list_concat_in_place_many(&input.extern_gvars, task.extern_gvars, tp->worker_count);
    rdib_variable_chunk_list_concat_in_place_many(&input.static_gvars, task.static_gvars, tp->worker_count);
    rdib_variable_chunk_list_concat_in_place_many(&input.extern_tvars, task.extern_tvars, tp->worker_count);
    rdib_variable_chunk_list_concat_in_place_many(&input.static_tvars, task.static_tvars, tp->worker_count);
    rdib_procedure_chunk_list_concat_in_place_many(&input.extern_procs, task.extern_procs, tp->worker_count);
    rdib_procedure_chunk_list_concat_in_place_many(&input.static_procs, task.static_procs, tp->worker_count);
  }
  ProfEnd();

  String8List rdi_data = rdib_finish(tp, tp_arena, &input);

  scratch_end(scratch);
  ProfEnd();
  return rdi_data;
}

