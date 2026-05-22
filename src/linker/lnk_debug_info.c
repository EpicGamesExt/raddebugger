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
  // discard types
  MemoryZeroStruct(&input->debug_t_arr[obj_idx]);

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
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  String8List sect_list = task->debug_s_list_arr[obj_idx];
  CV_DebugS  *debug_s   = &task->debug_s_arr    [obj_idx];

  for EachNode(n, String8Node, sect_list.first) {
    // parse & merge sub sections
    CV_DebugS ds = cv_debug_s_from_data(arena, n->string);
    cv_debug_s_concat_in_place(debug_s, &ds);

    // make sure there is one string table
    String8List string_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_StringTable);
    if (string_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S has %u string table sub-sections defined, picking first sub-section", string_data_list.node_count);
    }

    // make sure there is one file checksum table
    String8List checksum_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_FileChksms);
    if (checksum_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S has %u file checksum sub-sections defined, picking first sub-section", checksum_data_list.node_count);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_h_task)
{
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  String8List  sect_list =  task->debug_h_list_arr[obj_idx];
  CV_DebugH   *debug_h   = &task->debug_h_arr     [obj_idx];

  if (sect_list.node_count > 0) {
    if (sect_list.node_count > 1) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "obj contains multiple .debug$H sections; using first one");
    }

    // select first .debug$H
    String8    raw_debug_h     = sect_list.first->string;
    LLVM_GHash ghash           = {0};
    U64        ghash_read_size = str8_deserial_read_struct(raw_debug_h, 0, &ghash);

    // was header read completely?
    if (ghash_read_size != sizeof(ghash)) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    ".debug$H section is too small to contain the header");
      goto exit;
    }

    // validate magic
    if (ghash.magic != LLVM_GHash_Magic) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    ".debug$H contains invalid magic: got 0x%x, expected 0x%x", ghash.magic, LLVM_GHash_Magic);
      goto exit;
    }

    // validate version
    if (ghash.version != LLVM_GHash_CurrentVersion) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H version: got %u, expected %u",
                    ghash.version, LLVM_GHash_CurrentVersion);
      goto exit;
    }

    // validate hashing algorithm
    if (ghash.hash_alg != task->config->type_hash_alg) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H hash algorithm: got %S, expected %S",
                    llvm_string_from_ghash_alg(ghash.hash_alg),
                    llvm_string_from_ghash_alg(task->config->type_hash_alg));
      goto exit;
    }

    // input.debug_h_arr must be 1:1 with input.debug_t_arr
    U64 hash_size  = llvm_hash_size_from_alg(ghash.hash_alg);
    U64 hash_count = (raw_debug_h.size - sizeof(ghash)) / hash_size;
    if (hash_count != task->debug_t_arr[obj_idx].count) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H hash count and type count: got %llu hashes for %llu types",
                    hash_count, task->debug_t_arr[obj_idx].count);
      goto exit;
    }

    // load hashes
    String8 hashes = str8_substr(raw_debug_h, r1u64(sizeof(ghash), raw_debug_h.size));
    debug_h->count = hash_count;
    debug_h->v     = (U64 *)hashes.str;

    exit:;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_strip_debug_t_sig_task)
{
  U64               obj_idx = task_id;
  LNK_ParseCvTypes *task    = raw_task;

  for EachIndex(i, task->raw_types[obj_idx].count) {
    String8 *d = task->raw_types[obj_idx].v + i;
    if (d->size == 0)                   { continue; }
    if (d->size < sizeof(CV_Signature)) { lnk_error_obj(LNK_Error_IllData, task->input->obj_arr[obj_idx], ".debug$T must have at least 4 bytes for CodeView signature"); continue; }

    CV_Signature sig = cv_signature_from_debug_s(*d);
    switch (sig) {
    default: {
      lnk_error_obj(LNK_Warning_IllData, task->input->obj_arr[obj_idx], "unknown CodeView type signature in section (TODO: print section index)");
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
  LNK_ParseCvTypes *task = raw_task;
  if (task->raw_types[task_id].count > 0) {
    task->out_types[task_id] = cv_debug_t_from_data(arena, task->raw_types[task_id].v[0], CV_LeafAlign);
  } else {
    MemoryZeroStruct(&task->out_types[task_id]);
  }
  ProfEnd();
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
  String8 msf_data = lnk_read_data_from_file_path(scratch.arena, task->config->io_flags, ts->ts_path);

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

  MSF_StreamNumber type_streams[CV_TypeIndexSource_COUNT] = {0};
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
lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 obj_count, LNK_Obj **obj_arr)
{
  ProfBegin("Extract CodeView");
  Temp scratch = scratch_begin(0,0);

  LNK_CodeViewInput input = { .config = config, .obj_count = obj_count, .count = obj_count, .obj_arr = obj_arr, .ts_obj_range = r1u64(0,0) };
  
  ProfBegin("Collect CodeView");
  input.debug_s_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$S"), 0);
  input.debug_p_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$P"), 0);
  input.debug_t_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$T"), 0);
  if (config->ghash) {
    input.debug_h_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$H"), 0);
  }
  ProfEnd();

  if (lnk_get_log_status(LNK_Log_Debug) || PROFILE_TELEMETRY) {
    U64 total_debug_s_size = 0, total_debug_t_size = 0, total_debug_p_size = 0, total_debug_h_size = 0;
    for EachIndex(obj_idx, obj_count) {
      for EachNode(n, String8Node, input.debug_s_list_arr[obj_idx].first) { total_debug_s_size += n->string.size; }
      for EachNode(n, String8Node, input.debug_t_list_arr[obj_idx].first) { total_debug_t_size += n->string.size; }
      for EachNode(n, String8Node, input.debug_p_list_arr[obj_idx].first) { total_debug_p_size += n->string.size; }
      if (config->ghash) {
        for EachNode(n, String8Node, input.debug_h_list_arr[obj_idx].first) { total_debug_h_size += n->string.size; }
      }
    }
	
    ProfNoteV("Total .debug$S Input Size: %M", total_debug_s_size);
    ProfNoteV("Total .debug$T Input Size: %M", total_debug_t_size);
    ProfNoteV("Total .debug$P Input Size: %M", total_debug_p_size);
    ProfNoteV("Total .debug$H Input Size: %M", total_debug_h_size);
	
    if (lnk_get_log_status(LNK_Log_Debug)) {
      lnk_log(LNK_Log_Debug, "[Total .debug$S Input Size %M]", total_debug_s_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$T Input Size %M]", total_debug_t_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$P Input Size %M]", total_debug_p_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$H Input Size %M]", total_debug_h_size);
    }
  }

  ProfBegin("Parse CodeView");
  CV_DebugT *debug_p_arr;
  {
    input.debug_s_arr = push_array(tp_arena->v[0], CV_DebugS, input.obj_count);
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_s_task, &input, "Parse .debug$S");

    LNK_ParseCvTypes parse_types = { .input = &input };
    debug_p_arr = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = str8_array_from_list_arr(scratch.arena, input.debug_p_list_arr, obj_count);
    parse_types.out_types = debug_p_arr;
    tp_for_parallel_prof(tp, 0,        obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip .debug$P");
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_t_task,     &parse_types, "Parse .debug$P");

    input.debug_t_arr     = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = str8_array_from_list_arr(scratch.arena, input.debug_t_list_arr, obj_count);
    parse_types.out_types = input.debug_t_arr;
    tp_for_parallel_prof(tp, 0,        obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip .debug$T");
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_t_task,     &parse_types, "Parse .debug$T");

    input.debug_h_arr = push_array(tp_arena->v[0], CV_DebugH, input.obj_count);
    if (config->ghash) {
      tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_h_task, &input, "Parse .debug$H");
    }
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

  ProfScope("Set up /Zi")
  {
    input.obj_to_ts = push_array(tp_arena->v[0], U64, input.count);
    MemorySet(input.obj_to_ts, 0xff, input.count * sizeof(input.obj_to_ts[0]));

    LNK_TypeServerList  ts_list = {0};
    HashTable          *ts_ht   = hash_table_init(scratch.arena, 256);

    // push null type server (slots with broken type servers are set to null)
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
      String8            ts_path = lnk_find_first_file(scratch.arena, config->lib_dir_list, ts_info.name);

      if (ts_path.size == 0) {
        lnk_discard_cv_debug_info(&input, obj_idx);
        continue;
      }

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
        lnk_discard_cv_debug_info(&input, obj_idx);
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
      input.obj_arr        = push_array(tp_arena->v[0], LNK_Obj *, input.count);
      input.debug_s_arr    = push_array(tp_arena->v[0], CV_DebugS, input.count);
      input.debug_t_arr    = push_array(tp_arena->v[0], CV_DebugT, input.count);
      input.debug_h_arr    = push_array(tp_arena->v[0], CV_DebugH, input.count);
      input.obj_to_ts      = push_array(tp_arena->v[0], U64,       input.count);

      MemoryCopyTyped(input.obj_arr,        prev.obj_arr,        prev.count);
      MemoryCopyTyped(input.debug_s_arr,    prev.debug_s_arr,    prev.count);
      MemoryCopyTyped(input.debug_t_arr,    prev.debug_t_arr,    prev.count);
      MemoryCopyTyped(input.debug_h_arr,    prev.debug_h_arr,    prev.count);
      MemoryCopyTyped(input.obj_to_ts,      prev.obj_to_ts,      prev.count);
      MemorySet(input.obj_to_ts + input.obj_count, 0xff, ts_arr.count * sizeof(input.obj_to_ts[0]));

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

    // undiscard null type server
    input.is_type_server_discarded[0] = 0;

    // report bad type servers
    String8List unopen_type_server_list = {0};
    for EachIndex(ts_idx, ts_arr.count) {
      if ( ! input.is_type_server_discarded[ts_idx]) { continue; }
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t%S\n", ts_arr.v[ts_idx].ts_path);
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\tDependent objs:\n");
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
 
  ProfBegin("Set up PCH");
  {
    // hash_table<obj_path, obj_idx>
    String8    work_dir   = get_current_path(scratch.arena);
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
        for EachNode(alt_dir_n, String8Node, config->alt_pch_dirs.first) {
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

    // remove LF_ENDPRECOMP from .debug$P
    for EachIndex(i, input.debug_p_indices.count) {
      U64            debug_p_idx = input.debug_p_indices.v[i];
      CV_DebugT     *debug_p     = &input.debug_t_arr[debug_p_idx];
      for EachIndex(i, debug_p->count) {
        U64            lf_idx = debug_p->count - (i + 1);
        CV_LeafHeader *lf     = cv_debug_t_get_leaf_header(debug_p, lf_idx);
        if (lf->kind == CV_LeafKind_ENDPRECOMP) {
          memory_write16(&lf->size, sizeof(lf->kind));
          memory_write16(&lf->kind, CV_LeafKind_NOTYPE);
          break;
        }
      }
    }
  }
  ProfEnd();

  // set default min type index
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { input.min_type_indices[ti_source] = CV_MinComplexTypeIndex; }

  // PCH and /Z7 objs have default min type index set to CV_MinComplexTypeIndex
  // but type servers can bump up the lower bound. In practice nobody does this.
  // But to cover all our bases loop through type servers and compute max
  // lower bound.
  for EachInRange(ts_idx, input.ts_obj_range) {
    CV_DebugT *debug_t = &input.debug_t_arr[ts_idx];
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      input.min_type_indices[ti_source] = Max(input.min_type_indices[ti_source], debug_t->ti_ranges[ti_source].min);
    }
  }

  ProfBegin("Make Symbol Inputs");
  {
    // count symbol blocks
    for EachIndex(obj_idx, input.count) {
      String8List s = cv_sub_section_from_debug_s(input.debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
      input.symbol_input_count += s.node_count;
    }
  
    // alloc block pointers
    input.symbol_inputs = push_array_no_zero(tp_arena->v[0], LNK_SymbolInput, input.symbol_input_count);

    U64 symbol_input_count = 0;
    for EachIndex(obj_idx, input.count) {
      String8List s = cv_sub_section_from_debug_s(input.debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
      for EachNode(n, String8Node, s.first) {
        Assert(symbol_input_count < input.symbol_input_count);
        LNK_SymbolInput *in = &input.symbol_inputs[symbol_input_count++];
        in->obj_idx     = obj_idx;
        in->raw_symbols = n->string;
      }
    }

    ProfBegin("Make Ranges");
    U64 total_input_size = 0;
    for EachIndex(i, input.symbol_input_count) { total_input_size += input.symbol_inputs[i].raw_symbols.size; }

    U64 max_weight = CeilIntegerDiv(total_input_size, tp->worker_count);
    U64 cursor     = 0;
    input.symbol_input_ranges = push_array(tp_arena->v[0], Rng1U64, tp->worker_count);
    for EachIndex(i, tp->worker_count) {
      if (cursor >= input.symbol_input_count) { break; }
      U64 begin  = cursor;
      U64 weight = 0;
      for (; cursor < input.symbol_input_count; cursor += 1) {
        if (weight >= max_weight) { break; }
        weight += input.symbol_inputs[cursor].raw_symbols.size;
      }
      input.symbol_input_ranges[i] = r1u64(begin, cursor);
    }
    ProfEnd();
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return input;
}

internal LNK_LeafRef
lnk_leaf_ref_from_ti(LNK_CodeViewInput *input, U32 obj_idx, CV_TypeIndexSource source, CV_TypeIndex ti)
{
  CV_DebugT *debug_t = input->debug_t_arr + obj_idx;

  // ti_range: PCH
  if (contains_1u64(debug_t->pch_ti_range[source], ti)) {
    return (LNK_LeafRef){ debug_t->pch_obj_idx,
                          cv_leaf_idx_from_ti(&input->debug_t_arr[debug_t->pch_obj_idx], source, ti) };
  }

  // ti range: external type server
  U64 ts_idx = input->obj_to_ts[obj_idx];
  if (ts_idx != max_U64) {
    U64        ts_debug_t_idx = input->ts_obj_range.min + ts_idx;
    CV_DebugT *ts_debug_t     = input->debug_t_arr + ts_debug_t_idx;
    return (LNK_LeafRef){ ts_debug_t_idx, cv_leaf_idx_from_ti(ts_debug_t, source, ti) };
  }

  // ti range: internal type server
  return (LNK_LeafRef){ obj_idx, cv_leaf_idx_from_ti(debug_t, source, ti) };
}

internal int
lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b)
{
  if (a.obj_idx == b.obj_idx) {
    return a.leaf_idx < b.leaf_idx ? -1 :
           a.leaf_idx > b.leaf_idx ? +1 : 0;
  }
  return a.obj_idx < b.obj_idx ? -1 :
         a.obj_idx > b.obj_idx ? +1 : 0;
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
  U64 a_hash   = input->debug_h_arr[a.obj_idx].v[a.leaf_idx];
  U64 b_hash   = input->debug_h_arr[b.obj_idx].v[b.leaf_idx];
  if (a_hash == b_hash) {
    Assert(cv_debug_t_get_leaf(&input->debug_t_arr[a.obj_idx], a.leaf_idx).kind == cv_debug_t_get_leaf(&input->debug_t_arr[b.obj_idx], b.leaf_idx).kind);
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
      // discard type
      U32  leaf_idx    = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      U8  *leaf_header = debug_t->data.str + debug_t->offsets[leaf_idx];
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, size), sizeof(CV_LeafKind));

      // reset hasher
      blake3_hasher_init(&hasher);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) out of bounds type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[leaf_ref.obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // discard type with a cyclic-ref
    B32 is_type_graph_cyclic = discard_cycles && sub_ti > 0 && sub_ti > curr_ti;
    if (is_type_graph_cyclic) {
      // discard type
      U32  leaf_idx    = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      U8  *leaf_header = debug_t->data.str + debug_t->offsets[leaf_idx];
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
      memory_write16(leaf_header + OffsetOf(CV_LeafHeader, size), sizeof(CV_LeafKind));

      // reset hasher
      blake3_hasher_init(&hasher);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) forward refs member type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[leaf_ref.obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // type index -> hash
    LNK_LeafRef sub_ref  = lnk_leaf_ref_from_ti(input, leaf_ref.obj_idx, sub_ti_n->source, sub_ti);
    U64         sub_hash = input->debug_h_arr[sub_ref.obj_idx].v[sub_ref.leaf_idx];

    // mix-in sub-type hash
    blake3_hasher_update(&hasher, &sub_hash, sizeof(sub_hash));
  }

  // hash leaf header
  CV_LeafHeader *leaf_header = cv_debug_t_get_leaf_header(debug_t, leaf_ref.leaf_idx);
  blake3_hasher_update(&hasher, leaf_header, sizeof(*leaf_header));

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
    // alloc new bucket and assign type ref
    if (bucket == 0) { bucket = push_array_no_zero(arena, LNK_LeafRef, 1); }

    // fill bucket
    *bucket = (LNK_LeafRef){ .obj_idx = obj_idx, .leaf_idx = leaf_idx };

    B32 is_inserted_or_updated = 1;

    CV_LeafHeader      *header      = cv_debug_t_get_leaf_header(debug_t, leaf_idx);             // leaf index -> leaf header
    CV_LeafKind         kind        = memory_read16(MemberFromPtr(CV_LeafHeader, header, kind)); // leaf header -> leaf kind
    CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(kind);                 // leaf kind -> type stream
    LNK_LeafHashTable  *leaf_ht     = &task->leaf_ht_arr[leaf_source];                           // type stream -> hash table
    U64                 best_idx    = debug_h->v[leaf_idx] % leaf_ht->cap;                       // leaf ref -> hash -> bucket index
    U64                 idx         = best_idx;
    do {
      // load leaf ref
      LNK_LeafRef *curr = ins_atomic_ptr_eval(&leaf_ht->bucket_arr[idx]);

      while (curr == 0 || lnk_match_leaf_ref(task->input, *curr, *bucket)) {
        // exit if leaf ref is not recent
        if (curr != 0 && lnk_leaf_ref_compare(*bucket, *curr) >= 0) {
          goto exit;
        }

        // try to update the bucket
        LNK_LeafRef *cmp = ins_atomic_ptr_eval_cond_assign(&leaf_ht->bucket_arr[idx], bucket, curr);
        if (cmp == curr) {
          bucket = 0;
          goto exit;
        }

        // another thread updated the bucket -- retry
        curr = cmp;
      }

      // advance to next bucket
      idx = ((idx + 1) == leaf_ht->cap ? 0 : (idx + 1));
    } while (idx != best_idx);
    is_inserted_or_updated = 0;
    exit:;
    Assert(is_inserted_or_updated);
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

    CV_DebugT *debug_t;
    if (ctx->input->obj_to_ts[obj_idx] != max_U64) {
      U64 ts_idx      = ctx->input->obj_to_ts[obj_idx];
      U64 debug_t_idx = ctx->input->ts_obj_range.min + ts_idx;
      debug_t = ctx->input->debug_t_arr + debug_t_idx;
    } else {
      debug_t = ctx->input->debug_t_arr + obj_idx;
    }

    CV_TypeIndex final_ti = 0;
    if (contains_1u64(debug_t->ti_ranges[n->source], ti)) {
      LNK_LeafRef        leaf_ref   = lnk_leaf_ref_from_ti(ctx->input, obj_idx, n->source, ti);
      LNK_LeafHashTable *leaf_ht    = &ctx->leaf_ht_arr[n->source];
      LNK_LeafRef       *final_leaf = lnk_leaf_hash_table_search(leaf_ht, ctx->input, leaf_ref);
      if (final_leaf) {
        U64 final_hash = u64_hash_from_str8(str8_struct(final_leaf));
        final_ti = lnk_assigned_type_ht_search(ctx->assigned_type_caps  [n->source],
                                               ctx->assigned_type_hts   [n->source],
                                               ctx->min_type_indices    [n->source],
                                               ctx->unique_leaf_refs_arr[n->source],
                                               final_leaf,
                                               final_hash);
      }
#if BUILD_DEBUG
      else {
        lnk_error_obj(LNK_Error_InvalidTypeIndex, ctx->input->obj_arr[obj_idx], "no itype 0x%x", ti);
      }
#endif
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
  Rng1U64 range = task->input->symbol_input_ranges[task_id];
  for EachInRange(i, range) {
    LNK_SymbolInput symbols = task->input->symbol_inputs[i];
    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
      Temp temp = temp_begin(task->fixed_arenas[task_id]);

      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);

      CV_TypeIndexInfoList ti_info_list = cv_get_symbol_type_index_offsets(temp.arena, symbol.kind, symbol.data);
      lnk_fixup_cv_type_indices(task, symbols.obj_idx, symbol.data, ti_info_list);

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
  Arena          *fixed_arena   = task->fixed_arenas[worker_id];
  for EachNode(inline_data_n, String8Node, inlinee_lines.first) {
    Temp temp = temp_begin(fixed_arena);
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
  LNK_MergeTypes *task        = raw_task;
  Rng1U64         range       = task->ranges[task_id];
  Arena          *fixed_arena = task->fixed_arenas[task_id];
  for EachInRange(leaf_ref_idx, range) {
    Temp temp = temp_begin(fixed_arena);
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
  Rng1U64 range = task->ranges[task_id];
  for EachInRange(i, range) {
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

  LNK_SymbolInput   symbols        = task->input->symbol_inputs[task_id];
  U64               leaf_count_ipi = task->result.count    [CV_TypeIndexSource_IPI];
  U8              **leaf_arr_ipi   = task->result.v        [CV_TypeIndexSource_IPI];
  CV_TypeIndex      min_ti_ipi     = task->min_type_indices[CV_TypeIndexSource_IPI];

  for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
    CV_Symbol symbol = {0};
    TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);

    // convert symbol to final type
    CV_SymKind *sym_kind_ptr = cv_kind_ptr_from_symbol(symbol);
    switch (*sym_kind_ptr) {
    case CV_SymKind_PROC_ID_END: *sym_kind_ptr = CV_SymKind_END; break;

    case CV_SymKind_LPROC32_ID:     *sym_kind_ptr = CV_SymKind_LPROC32;     goto fixup_id;
    case CV_SymKind_GPROC32_ID:     *sym_kind_ptr = CV_SymKind_GPROC32;     goto fixup_id;
    case CV_SymKind_LPROC32_DPC_ID: *sym_kind_ptr = CV_SymKind_LPROC32_DPC; goto fixup_id;
    case CV_SymKind_LPROCMIPS_ID:   *sym_kind_ptr = CV_SymKind_LPROCMIPS;   goto fixup_id;
    case CV_SymKind_GPROCMIPS_ID:   *sym_kind_ptr = CV_SymKind_GPROCMIPS;   goto fixup_id;
    case CV_SymKind_LPROCIA64_ID:   *sym_kind_ptr = CV_SymKind_LPROCIA64;   goto fixup_id;
    case CV_SymKind_GPROCIA64_ID:   *sym_kind_ptr = CV_SymKind_GPROCIA64;   goto fixup_id;
    fixup_id:; {
      CV_SymProc32 *proc32 = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*proc32));
      if (proc32->itype < min_ti_ipi) {
        // TODO: in some cases destructors don't have a type, need a repro
        break;
      }

      if ((proc32->itype - min_ti_ipi) > leaf_count_ipi) {
        Assert(0 && "TODO: error handle corrupted type index");
        break;
      }

      U64     leaf_idx  = proc32->itype - min_ti_ipi;
      String8 leaf_data = str8(leaf_arr_ipi[leaf_idx], max_U64);

      CV_Leaf leaf;
      if (cv_read_leaf(leaf_data, 0, 1, &leaf) == 0) { InvalidPath; }

      U64 min_leaf_size = cv_header_struct_size_from_leaf_kind(leaf.kind);
      if (min_leaf_size > leaf.data.size) { Assert(!"TODO: error handle corrupt leaf"); break; }

      if (leaf.kind == CV_LeafKind_FUNC_ID) {
        CV_LeafFuncId *func_id = str8_deserial_get_raw_ptr(leaf.data, 0, sizeof(*func_id));
        proc32->itype = func_id->itype;
      } else if (leaf.kind == CV_LeafKind_MFUNC_ID) {
        CV_LeafMFuncId *mfunc_id = str8_deserial_get_raw_ptr(leaf.data, 0, sizeof(*mfunc_id));
        proc32->itype = mfunc_id->itype;
      } else {
        Assert(!"TODO: erorr handle unexpected leaf type");
        break;
      }
    } break;
    }
  }
}

internal LNK_MergedTypes
lnk_merge_types(TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input)
{
  ProfBeginFunction();
  Temp scratch = temp_begin(lnk_get_huge_arena());

  LNK_MergeTypes task = { .input = input };
  U64 max_ti_list_size = sizeof(CV_TypeIndexInfo) * (max_U16 / sizeof(CV_TypeIndex));
  task.fixed_arenas = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, max_ti_list_size, max_ti_list_size);

  ProfBegin("Produce Hashes");
  {
    ProfBegin("Alloc Hashes");
    struct HashTarget {
      TP_TaskFunc *hasher_task;
      U32Array     indices;
      U32Array     hash_indices;
    } hash_targets[] = {
      { lnk_hash_debug_t_task,      input->debug_p_indices     }, // hash .debug$P first so we can mix in hashes for precompiled sub leaves when hashing leaves in .debug$T
      { lnk_hash_debug_t_task,      input->int_obj_indices     },
      { lnk_hash_debug_t_deep_task, input->type_server_indices },
    };

    for EachElement(i, hash_targets) {
      // reserve array for obj indices that need hashing
      U32Array *h = &hash_targets[i].hash_indices;
      h->count = 0;
      h->v     = push_array(scratch.arena, U32, hash_targets[i].indices.count);

      for EachIndex(k, hash_targets[i].indices.count) {
        U32        obj_idx = hash_targets[i].indices.v[k];
        CV_DebugH *debug_h = &input->debug_h_arr[obj_idx];

        if (debug_h->count == 0) {
          // alloc hashes
          CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
          debug_h->count = debug_t->count;
          debug_h->v     = push_array(scratch.arena, U64, debug_h->count);

          // schedule obj types to be hashed
          h->v[h->count++] = obj_idx;
        } else {
          // hash was loaded from .debug$H
        }
      }
    }
    ProfEnd();

    for EachElement(i, hash_targets) {
      task.indices = hash_targets[i].hash_indices;
      ProfBegin("Hash [Count: %.*s]", str8_varg(str8_from_count(scratch.arena, task.indices.count)));
      tp_for_parallel(tp, 0, task.indices.count, hash_targets[i].hasher_task, &task);
      ProfEnd();
    }

#if BUILD_DEBUG
    for EachIndex(i, input->count) {
      for EachIndex(k, input->debug_h_arr[i].count) {
        Assert(input->debug_h_arr[i].v[k] != 0);
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

  ProfBegin("Fixup Type Indices");
  {
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

    task.ranges = tp_divide_work(scratch.arena, input->symbol_input_count, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_cv_patcher_symbols_task, &task, "Fixup Symbol Type Indices");

    task.ranges      = 0;
    task.debug_s_arr = input->debug_s_arr;
    tp_for_parallel_prof(tp, 0, input->count, lnk_cv_patcher_inlines_task, &task, "Fixup Inlines Type Indices");

    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      task.ti_source = ti_source;
      task.ranges    = tp_divide_work(scratch.arena, task.unique_leaf_refs_arr[ti_source].count, tp->worker_count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_cv_patcher_leaves_task, &task, "Fixup Types Type Indices");
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

  tp_for_parallel_prof(tp, 0, input->symbol_input_count, lnk_fixup_symbols_task, &task, "Fixup ID Symbols");

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
          U64 size = raddbg_snprintf(temp, sizeof(temp), "%llx", (long long)name_hash);
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
          udt_info.unique_name.size = raddbg_snprintf((char *)udt_info.unique_name.str, udt_info.unique_name.size, "%llx", (long long)name_hash);

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
        udt_info.name.size = raddbg_snprintf((char *)udt_info.name.str, udt_info.name.size, "%llx", (long long)name_hash);

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
THREAD_POOL_TASK_FUNC(lnk_move_global_symbols_to_gsi)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildPdb   *task        = raw_task;
  PDB_GsiContext *gsi         = task->pdb->gsi;
  PDB_PsiContext *psi         = task->pdb->psi;
  U32Array        obj_indices = task->obj_indices[task_id];

  ProfBegin("Global Symbols");
  {
    VoidList global_symbols = {0};
    for EachInRange(i, task->cv->symbol_input_ranges[task_id]) {
      LNK_SymbolInput symbols = task->cv->symbol_inputs[i];
      for (U64 cursor = 0, depth = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
        CV_Symbol symbol = {0};
        TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);

        if (cv_is_global_symbol(symbol.kind) || (depth == 0 && cv_is_typedef(symbol.kind))) {
          void *ptr = cv_ptr_from_symbol(symbol);
          void_list_push(scratch.arena, &global_symbols, ptr);
        }

        if (cv_is_scope_symbol(symbol.kind)) {
          depth += 1;
        } else if (cv_is_end_symbol(symbol.kind)) {
          if (depth == 0) { Assert(0 && "malformed symbol stream"); break; }
          depth -= 1;
        }
      }
    }

    // collect global data and global typedefs
    U64 global_symbol_count = tp_sum_u64(tp, task_id, global_symbols.count);

    U64    bucket_cap;
    void **buckets;
    if (task_id == 0) {
      bucket_cap = global_symbol_count * 13 / 10;
      buckets    = push_array(scratch.arena, void *, bucket_cap);
    }
    tp_broadcast(&bucket_cap);
    tp_broadcast(&buckets);

    // insert symbols into hash table
    for EachNode(n, VoidNode, global_symbols.first) {
      String8 raw  = cv_raw_from_symbol(n->v);
      U64     hash = u64_hash_from_str8(raw);
      cv_symbol_deduper_insert_or_update(buckets, bucket_cap, hash, n->v);
    }
    barrier_wait(tp->barrier);

    U64       symbol_count  = 0;
    void    **symbol_arr    = 0; // [symbol_count]
    Rng1U64  *symbol_ranges = 0; // [worker_count]
    U32      *symbol_hashes = 0; // [symbol_count]
    if (task_id == 0) {
      ProfBeginV("Compact Buckets [bucket_cap %llu]", bucket_cap);
      for EachIndex(src, bucket_cap) {
        buckets[symbol_count] = buckets[src];
        symbol_count += buckets[src] != 0;
      }
      ProfEnd();

      symbol_arr    = buckets;
      symbol_ranges = tp_divide_work(scratch.arena, symbol_count, tp->worker_count);
      symbol_hashes = push_array_no_zero(scratch.arena, U32, symbol_count);
    }
    tp_broadcast(&symbol_count);
    tp_broadcast(&symbol_arr);
    tp_broadcast(&symbol_ranges);
    tp_broadcast(&symbol_hashes);

    // hash symbols
    Rng1U64 symbol_range = symbol_ranges[task_id];
    for EachInRange(i, symbol_range) {
      CV_Symbol symbol = cv_symbol_from_ptr(symbol_arr[i]);
      String8   name   = cv_name_from_symbol(symbol.kind, symbol.data);
      symbol_hashes[i] = gsi_hash(gsi, name);
    }
    barrier_wait(tp->barrier);

    // push global symbols
    if (task_id == 0) {
      CV_SymbolNode *nodes = push_array_no_zero(gsi->arena, CV_SymbolNode, symbol_count);
      for EachIndex(i, symbol_count) {
        CV_SymbolNode *n = &nodes[i];
        n->prev = n->next = 0;
        n->data = cv_symbol_from_ptr(symbol_arr[i]);
        n->data.offset = i;
        gsi_push_(gsi, symbol_hashes[i], n);
      }
    }
  }
  ProfEnd();

  ProfBegin("Proc Refs");
  {
    U64 *proc_ref_sizes  = 0;
    U64 *proc_ref_counts = 0;
    if (task_id == 0) {
      proc_ref_sizes  = push_array(scratch.arena, U64, tp->worker_count);
      proc_ref_counts = push_array(scratch.arena, U64, tp->worker_count);
    }
    tp_broadcast(&proc_ref_sizes);
    tp_broadcast(&proc_ref_counts);

    U64 proc_ref_size  = 0;
    U64 proc_ref_count = 0;
    for EachIndex(i, obj_indices.count) {
      U64         obj_idx        = obj_indices.v[i];
      CV_DebugS   debug_s        = task->cv->debug_s_arr[obj_idx];
      String8List symbols        = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
      for EachNode(n, String8Node, symbols.first) {
        for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= n->string.size; ) {
          CV_Symbol symbol = {0};
          TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);

          if (symbol.kind == CV_SymKind_GPROC32 || symbol.kind == CV_SymKind_LPROC32) {
            String8 name = cv_name_from_symbol(symbol.kind, symbol.data);
            proc_ref_size  += AlignPow2(sizeof(CV_SymRef2) + name.size + 1, sizeof(void *));
            proc_ref_count += 1;
          }
        }
      }
    }
    proc_ref_sizes[task_id]  = proc_ref_size;
    proc_ref_counts[task_id] = proc_ref_count;
    barrier_wait(tp->barrier);

    U64 total_proc_ref_size  = tp_sum_u64(tp, task_id, proc_ref_size);
    U64 total_proc_ref_count = tp_sum_u64(tp, task_id, proc_ref_count);

    U64            *proc_ref_hashes  = 0;
    U64            *proc_ref_indices = 0;
    Arena         **proc_ref_arenas  = 0;
    CV_SymbolNode  *proc_ref_nodes   = 0;
    if (task_id == 0) {
      proc_ref_hashes  = push_array(scratch.arena, U64, total_proc_ref_count);
      proc_ref_indices = offsets_from_counts_array_u64(scratch.arena, proc_ref_counts, tp->worker_count);
      proc_ref_arenas  = alloc_arena_many(gsi->arena, tp->worker_count, proc_ref_sizes);
      proc_ref_nodes   = push_array(gsi->arena, CV_SymbolNode, total_proc_ref_count);
    }
    tp_broadcast(&proc_ref_hashes);
    tp_broadcast(&proc_ref_indices);
    tp_broadcast(&proc_ref_arenas);
    tp_broadcast(&proc_ref_nodes);

    Arena *proc_ref_arena = proc_ref_arenas[task_id];
    U64    proc_ref_idx   = proc_ref_indices[task_id];
    for EachIndex(i, obj_indices.count) {
      U64         obj_idx       = obj_indices.v[i];
      CV_DebugS   debug_s       = task->cv->debug_s_arr[obj_idx];
      String8List symbols       = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
      CV_ModIndex imod          = task->mod_arr[obj_idx]->imod;
      U64         symbol_cursor = sizeof(CV_Signature);
      U64         scope_depth   = 0;
      for EachNode(n, String8Node, symbols.first) {
        for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= n->string.size; ) {
          CV_Symbol symbol = {0};
          TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);

          if      (symbol.kind == CV_SymKind_SKIP)                 { continue; }
          else if (cv_is_global_symbol(symbol.kind))               { continue; }
          else if (cv_is_typedef(symbol.kind) && scope_depth == 0) { continue; }
          else if (symbol.kind == 0x1176)                          { continue; }

          if      (cv_is_scope_symbol(symbol.kind)) { scope_depth += 1; }
          else if (cv_is_end_symbol(symbol.kind))   { scope_depth -= 1; }

          if (symbol.kind == CV_SymKind_GPROC32 || symbol.kind == CV_SymKind_LPROC32) {
            String8 name = cv_name_from_symbol(symbol.kind, symbol.data);
            proc_ref_nodes [proc_ref_idx].data = cv_make_proc_ref(proc_ref_arena, imod, symbol_cursor, name, cv_is_lproc(symbol));
            proc_ref_nodes [proc_ref_idx].data.offset = symbol_cursor;
            proc_ref_hashes[proc_ref_idx] = gsi_hash(gsi, name);
            proc_ref_idx += 1;
          }

          symbol_cursor += cv_write_symbol_buf(0, 0, &symbol, PDB_SYMBOL_ALIGN);
        }
      }
    }
    barrier_wait(tp->barrier);

    // push proc refs
    if (task_id == 0) {
      U64 total_proc_ref_count = sum_array_u64(tp->worker_count, proc_ref_counts);
      for EachIndex(i, total_proc_ref_count) { gsi_push_(gsi, proc_ref_hashes[i], &proc_ref_nodes[i]); }
    }
    barrier_wait(tp->barrier);
  }
  ProfEnd();

  ProfBegin("Public Symbols");
  {
    U64 *public_symbol_sizes       = 0; // [worker_count]
    U64 *public_symbol_node_counts = 0; // [worker_count]
    if (task_id == 0) {
      public_symbol_sizes       = push_array(scratch.arena, U64, tp->worker_count);
      public_symbol_node_counts = push_array(scratch.arena, U64, tp->worker_count);
    }
    tp_broadcast(&public_symbol_sizes);
    tp_broadcast(&public_symbol_node_counts);

    // compute buffer size for CV public symbols
    LNK_SymbolHashTrieChunkList symbol_chunks       = task->symtab->chunks[task_id];
    U64                         public_symbol_size  = 0;
    U64                         public_symbol_count = 0;
    for EachNode(chunk, LNK_SymbolHashTrieChunk, symbol_chunks.first) {
      for EachIndex(i, chunk->count) {
        LNK_Symbol        *symbol        = chunk->v[i].symbol;
        LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
        COFF_ParsedSymbol  symbol_parsed = lnk_parsed_from_symbol(symbol);

        if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }
        COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
        if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

        public_symbol_size  += AlignPow2(sizeof(CV_SymPub32) + symbol->name.size + 1, sizeof(void *));
        public_symbol_count += 1;
        public_symbol_node_counts[task_id] += 1;
      }
    }
    public_symbol_sizes      [task_id] += public_symbol_size;
    public_symbol_node_counts[task_id] += public_symbol_count;
    barrier_wait(tp->barrier);

    Arena         **public_symbol_arenas      = 0;
    Arena         **public_symbol_node_arenas = 0;
    CV_SymbolList  *public_symbols            = 0; // [worker_count]
    U32           **public_symbol_hashes      = 0; // [worker_count][public_symbol.count]
    if (task_id == 0) {
      U64 public_symbol_total_count  = sum_array_u64(tp->worker_count, public_symbol_node_counts);
      public_symbol_arenas      = alloc_arena_many(psi->gsi->arena, tp->worker_count, public_symbol_sizes);
      public_symbol_node_arenas = alloc_arena_array(psi->gsi->arena, tp->worker_count, public_symbol_node_counts, CV_SymbolNode);
      public_symbols            = push_array(scratch.arena, CV_SymbolList, tp->worker_count);
      public_symbol_hashes      = push_array(scratch.arena, U32 *, tp->worker_count);
    }
    tp_broadcast(&public_symbol_arenas);
    tp_broadcast(&public_symbol_node_arenas);
    tp_broadcast(&public_symbols);
    tp_broadcast(&public_symbol_hashes);

    // make CV public symbols
    Arena         *public_symbol_arena      = public_symbol_arenas     [task_id];
    Arena         *public_symbol_node_arena = public_symbol_node_arenas[task_id];
    CV_SymbolList *public_symbol_list       = &public_symbols          [task_id];
    for EachNode(chunk, LNK_SymbolHashTrieChunk, symbol_chunks.first) {
      for EachIndex(i, chunk->count) {
        LNK_Symbol        *symbol        = chunk->v[i].symbol;
        LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
        COFF_ParsedSymbol  symbol_parsed = lnk_parsed_from_symbol(symbol);

        // discard removed and non-section symbols
        if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }
        COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
        if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

        CV_Pub32Flags flags      = COFF_SymbolType_IsFunc(symbol_parsed.type) ? CV_Pub32Flag_Function : 0;
        ISectOff      sc         = lnk_sc_from_symbol(symbol);
        CV_Symbol     pub_symbol = cv_make_pub32(public_symbol_arena, flags, safe_cast_u32(sc.off), safe_cast_u16(sc.isect), symbol->name);
        cv_symbol_list_push(public_symbol_node_arena, public_symbol_list, pub_symbol);
      }
    }
    barrier_wait(tp->barrier);

    // hash public symbols
    {
      U64  hash_idx = 0;
      U32 *hashes   = push_array(scratch.arena, U32, public_symbols[task_id].count);
      for EachNode(n, CV_SymbolNode, public_symbols[task_id].first) {
        String8 name = cv_name_from_symbol(n->data.kind, n->data.data);
        hashes[hash_idx++] = gsi_hash(gsi, name);
      }
      public_symbol_hashes[task_id] = hashes;
    }
    barrier_wait(tp->barrier);

    // insert public symbols into PSI
    if (task_id == 0) {
      for EachIndex(i, tp->worker_count) {
        U64 k = 0;
        for (CV_SymbolNode *curr = public_symbols[i].first, *next = 0; curr != 0; curr = next, k += 1) {
          next = curr->next;
          curr->next = 0;
          gsi_push_(psi->gsi, public_symbol_hashes[i][k], curr);
        }
      }
    }
    barrier_wait(tp->barrier);
  }
  ProfEnd();

  scratch_end(scratch);
}

internal U64
lnk_write_debug_s_to_pdb_module(PDB_DbiModule *mod, CV_DebugS debug_s, String8Node *buf, U64 *buf_pos)
{
  U64 mod_cursor = 0;

  mod->sym_data_size = 0;
  mod->c11_data_size = 0;
  mod->c13_data_size = 0;
  mod->globrefs_size = 0;

  String8List symbols = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);

  if (symbols.total_size) {
    // signature
    U64 sig_size = str8_buffer_write_u32(buf, buf_pos, CV_Signature_C13);
    mod->sym_data_size += sig_size;
    mod_cursor         += sig_size;

    // write symbols
    U64 scope_depth = 0;
    for EachNode(n, String8Node, symbols.first) {
      for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= n->string.size; ) {
        CV_Symbol symbol = {0};
        TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);

        if      (symbol.kind == CV_SymKind_SKIP)                 { continue; }
        else if (cv_is_global_symbol(symbol.kind))               { continue; }
        else if (cv_is_typedef(symbol.kind) && scope_depth == 0) { continue; }
        else if (symbol.kind == 0x1176)                          { continue; }

        if      (cv_is_scope_symbol(symbol.kind)) { scope_depth += 1; }
        else if (cv_is_end_symbol(symbol.kind))   { scope_depth -= 1; }

        U64 symbol_size = cv_write_symbol_buf(buf, buf_pos, &symbol, PDB_SYMBOL_ALIGN);
        mod_cursor         += symbol_size;
        mod->sym_data_size += symbol_size;
      }
    }
  }

  // write file checksums, inlinee lines etc.
  CV_C13SubSectionKind mod_c13_layout[] = {
    CV_C13SubSectionKind_FileChksms,
    CV_C13SubSectionKind_FrameData,
    CV_C13SubSectionKind_InlineeLines,
    CV_C13SubSectionKind_CrossScopeImports,
    CV_C13SubSectionKind_CrossScopeExports,
    CV_C13SubSectionKind_IlLines,
    CV_C13SubSectionKind_FuncMDTokenMap,
    CV_C13SubSectionKind_TypeMDTokenMap,
    CV_C13SubSectionKind_MergedAssemblyInput,
    CV_C13SubSectionKind_CoffSymbolRVA,
    CV_C13SubSectionKind_XfgHashType,
    CV_C13SubSectionKind_XfgHashVirtual,
  };
  for EachElement(i, mod_c13_layout) {
    String8List data = cv_sub_section_from_debug_s(debug_s, mod_c13_layout[i]);
    if (data.total_size == 0) { continue; }
    U64 ss_size = 0;
    ss_size += str8_buffer_write(buf, buf_pos, str8_struct((&(CV_C13SubSectionHeader ){ .kind = mod_c13_layout[i], .size = safe_cast_u32(data.total_size) })));
    ss_size += str8_buffer_write_string_list(buf, buf_pos, data);
    ss_size += str8_buffer_write_zeroes(buf, buf_pos, AlignPadPow2(mod_cursor + ss_size, CV_C13SubSectionAlign));
    mod_cursor         += ss_size;
    mod->c13_data_size += ss_size;
  }

  // write line tables
  String8List lines = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);
  for EachNode(n, String8Node, lines.first) {
    if (n->string.size == 0) { continue; }
    U64 ss_size = 0;
    ss_size += str8_buffer_write(buf, buf_pos, str8_struct((&(CV_C13SubSectionHeader){ .kind = CV_C13SubSectionKind_Lines, .size = safe_cast_u32(n->string.size) })));
    ss_size += str8_buffer_write(buf, buf_pos, n->string);
    ss_size += str8_buffer_write_zeroes(buf, buf_pos, AlignPadPow2(mod_cursor + ss_size, CV_C13SubSectionAlign));
    mod_cursor         += ss_size;
    mod->c13_data_size += ss_size;
  }

  // write global refs
  if (mod->sym_data_size) {
    String8List globrefs = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_GlobalRefs);
    mod->globrefs_size += str8_buffer_write_u32(buf, buf_pos, safe_cast_u32(globrefs.total_size));
    mod->globrefs_size += str8_buffer_write_string_list(buf, buf_pos, globrefs);
    mod_cursor += mod->globrefs_size;
  }

  return mod_cursor;
}

internal
THREAD_POOL_TASK_FUNC(lnk_write_pdb_modules)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildPdb *task        = raw_task;
  U32Array      obj_indices = task->obj_indices[task_id];

  // compute sizes for module streams
  for EachIndex(i, obj_indices.count) {
    U64 obj_idx = obj_indices.v[i];
    lnk_write_debug_s_to_pdb_module(task->mod_arr[obj_idx], task->cv->debug_s_arr[obj_idx], 0, 0);
  }
  barrier_wait(tp->barrier);

  // alloc module streams
  if (task_id == 0) {
    for EachIndex(obj_idx, task->cv->obj_count) {
      PDB_DbiModule *mod = task->mod_arr[obj_idx];
      U64 mod_size = mod->sym_data_size + mod->c11_data_size + mod->c13_data_size + mod->globrefs_size;
      if (mod_size > 0) {
        task->mod_arr[obj_idx]->sn = msf_stream_alloc_ex(task->pdb->msf, mod_size);
      }
    }
  }
  barrier_wait(tp->barrier);

  // write .debug$S to modules
  for EachIndex(i, obj_indices.count) {
    Temp temp = temp_begin(scratch.arena);

    U64            obj_idx  = obj_indices.v[i];
    PDB_DbiModule *mod      = task->mod_arr[obj_idx];

    if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }

    CV_DebugS      debug_s  = task->cv->debug_s_arr[obj_idx];
    String8List    mod_data = msf_data_from_sn(temp.arena, task->pdb->msf, mod->sn);

    if (mod_data.node_count) {
      String8Node buf = *mod_data.first;
      U64         pos = 0;
      lnk_write_debug_s_to_pdb_module(mod, debug_s, &buf, &pos);

      // sub range symbol data pages and patch symbol tree offsets
      if (mod->sym_data_size) {
        Rng1U64     sym_data_range = r1u64(sizeof(CV_Signature), mod->sym_data_size);
        String8List mod_symbols    = str8_list_substr(temp.arena, mod_data, sym_data_range);
        Assert(mod_symbols.total_size == dim_1u64(sym_data_range));
        cv_patch_symbol_tree_offsets(mod_symbols, sizeof(CV_Signature), PDB_SYMBOL_ALIGN);
      }
    }

    temp_end(temp);
  }
  barrier_wait(tp->barrier);

  {
    // count strings in string tables
    U64 *string_counts;
    {
      if (task_id == 0) {
        string_counts = push_array(scratch.arena, U64, tp->worker_count);
      }
      tp_broadcast(&string_counts);

      for EachIndex(i, obj_indices.count) {
        U64       obj_idx      = obj_indices.v[i];
        CV_DebugS debug_s      = task->cv->debug_s_arr[obj_idx];
        String8   string_table = cv_string_table_from_debug_s(debug_s);
        U64       string_count = 0;
        for (U64 cursor = 0; cursor < string_table.size; cursor += 1) {
          if (string_table.str[cursor] == '\0') {
            string_counts[task_id] += 1;
          }
        }
      }
      barrier_wait(tp->barrier);
    }

    Arena **string_arenas = 0;
    if (task_id == 0) {
      string_arenas = alloc_arena_array(task->pdb->dbi->arena, tp->worker_count, string_counts, String8Node);
    }
    tp_broadcast(&string_arenas);

    for EachIndex(i, obj_indices.count) {
      Temp temp = temp_begin(scratch.arena);

      U64            obj_idx = obj_indices.v[i];
      PDB_DbiModule *mod     = task->mod_arr[obj_idx];

      if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }

      String8List mod_data       = msf_data_from_sn(temp.arena, task->pdb->msf, mod->sn);
      Rng1U64     c13_data_range = r1u64(mod->sym_data_size + mod->c11_data_size, mod->sym_data_size + mod->c11_data_size + mod->c13_data_size);
      String8List c13_data       = str8_list_substr(temp.arena, mod_data, c13_data_range);

      CV_DebugS debug_s      = task->cv->debug_s_arr[obj_idx];
      String8   string_table = cv_string_table_from_debug_s(debug_s);

      // checksum is always at the head of C13 data
      String8List file_chksms_raw = {0};
      {
        String8Node buf     = *c13_data.first;
        U64         buf_pos = 0;

        CV_C13SubSectionHeader header = {0};
        str8_buffer_read(&buf, &buf_pos, sizeof(header), &header);

        if (header.kind == CV_C13SubSectionKind_FileChksms) {
          Rng1U64 file_chksms_range = r1u64(sizeof(CV_C13SubSectionHeader), sizeof(CV_C13SubSectionHeader) + header.size);
          file_chksms_raw = str8_list_substr(temp.arena, c13_data, file_chksms_range);
        }
      }

      // fixup file name offsets in checksum headers
      if (file_chksms_raw.total_size) {
        String8Node buf     = *file_chksms_raw.first;
        U64         buf_pos = 0;
        U64         cursor  = 0;
        for (;;) {
          CV_C13Checksum header = {0};
          if (str8_buffer_peek(&buf, &buf_pos, sizeof(header), &header) != sizeof(header)) { break; }

          String8          name     = str8_cstring_capped(string_table.str + header.name_off, string_table.str + string_table.size);
          CV_StringBucket *bucket   = cv_string_hash_table_lookup(task->string_ht, name);
          U64              name_off = task->pdb->info->strtab.size + bucket->u.offset;

          // update name offset
          {
            String8Node buf_copy     = buf;
            U64         buf_pos_copy = buf_pos;
            str8_buffer_skip(&buf_copy, &buf_pos_copy, OffsetOf(CV_C13Checksum, name_off));
            str8_buffer_write_u32(&buf_copy, &buf_pos_copy, safe_cast_u32(name_off));
          }

          str8_buffer_skip(&buf, &buf_pos, AlignPow2(sizeof(header) + header.len, CV_FileCheckSumsAlign));
        }
      }

      // collect mod source files
      String8List source_file_list = str8_split_by_string_chars(string_arenas[task_id], string_table, str8_lit("\0"), 0);
      str8_list_concat_in_place(&mod->source_file_list, &source_file_list);

      temp_end(temp);
    }
    barrier_wait(tp->barrier);
  }

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_push_dbi_sec_contrib_task)
{
  // TODO: use chunked lists for SC
  // TODO: put back unused sc nodes
  // TODO: compute CRC for relocations

  U64            obj_idx = task_id;
  LNK_BuildPdb  *task    = raw_task;
  PDB_DbiModule *mod     = task->mod_arr    [obj_idx];
  LNK_Obj       *obj     = task->cv->obj_arr[obj_idx];

  COFF_SectionHeader        *obj_section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  PDB_DbiSectionContribNode *sc_arr            = push_array_no_zero(arena, PDB_DbiSectionContribNode, obj->header.section_count_no_null);
  U64                        sc_count          = 0;
  
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *obj_sect_header = &obj_section_table[sect_idx];

    if (obj_sect_header->flags & COFF_SectionFlag_LnkInfo)   { continue; }
    if (obj_sect_header->flags & COFF_SectionFlag_LnkRemove) { continue; }
    if (obj_sect_header->flags & LNK_SECTION_FLAG_DEBUG)     { continue; }

    String8 header_name = str8_cstring_capped(obj_sect_header->name, obj_sect_header->name + sizeof(obj_sect_header->name));
    if (str8_match(header_name, str8_lit(".pdata"), 0)) { continue; }

    U64     sect_number;
    String8 sect_data;
    U32     sect_off;
    U32     data_crc;
    if (obj_sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
      if (obj_sect_header->vsize == 0) { continue; }

      U64 sect_num = rng1u64_array_num_from_value__binary_search(&task->image_section_virt_ranges, obj_sect_header->voff);
      sect_number = sect_num-1;
      Assert(sect_number < task->image_section_virt_ranges.count);
      sect_data   = str8_zero();
      sect_off    = obj_sect_header->voff - task->image_section_virt_ranges.v[sect_number].min;
      data_crc    = 0;
    } else {
      if (obj_sect_header->fsize == 0) { continue; }

      U64 sect_num = rng1u64_array_num_from_value__binary_search(&task->image_section_file_ranges, obj_sect_header->foff);
      sect_number = sect_num-1;
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

internal String8List
lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);

  LNK_BuildPdb task = {
    .image_data                      = image_data,
    .symtab                          = symtab,
    .cv                              = cv,
    .pdb                             = pdb_alloc_(lnk_get_huge_arena(), config->pdb_page_size, config->machine, config->time_stamp, config->age, config->guid),
    .mod_arr                         = push_array(scratch.arena, PDB_DbiModule *, cv->obj_count),
    .pe                              = pe_bin_info_from_data(scratch.arena, image_data),
    .image_section_table             = coff_section_table_from_data(scratch.arena, image_data, task.pe.section_table_range),
    .image_section_table_count       = task.pe.section_count+1,
    .image_section_virt_ranges.count = task.image_section_table_count,
    .image_section_virt_ranges.v     = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
    .image_section_file_ranges.v     = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
  };

  // set min type indices
  for EachElement(ti_source, cv_types.min_type_indices) { task.pdb->type_servers[ti_source]->ti_lo = cv_types.min_type_indices[ti_source]; }

  // per worker obj indices
  {
    U64 objs_per_worker = CeilIntegerDiv(cv->obj_count, tp->worker_count);
    task.obj_indices = push_array(scratch.arena, U32Array, tp->worker_count);
    for EachIndex(i, tp->worker_count)    { task.obj_indices[i].v = push_array(scratch.arena, U32, objs_per_worker); }
    for EachIndex(obj_idx, cv->obj_count) {
      U32Array *obj_indices = &task.obj_indices[obj_idx % tp->worker_count];
      obj_indices->v[obj_indices->count++] = obj_idx;
    }
  }

  // push types
  pdb_type_server_push_parallel(tp, task.pdb->type_servers[CV_TypeIndexSource_IPI], cv_types.count[CV_TypeIndexSource_IPI], cv_types.v[CV_TypeIndexSource_IPI]);
  pdb_type_server_push_parallel(tp, task.pdb->type_servers[CV_TypeIndexSource_TPI], cv_types.count[CV_TypeIndexSource_TPI], cv_types.v[CV_TypeIndexSource_TPI]);

  ProfBegin("Merge String Tables");
  task.string_ht = cv_dedup_string_tables(tp_arena, tp, cv->obj_count, cv->debug_s_arr);
  cv_string_hash_table_assign_buffer_offsets(tp, task.string_ht);
  ProfEnd();

  ProfScope ("Alloc Modules")
    for EachIndex(obj_idx, cv->obj_count) {
      task.mod_arr[obj_idx] = dbi_push_module(task.pdb->dbi, cv->obj_arr[obj_idx]->path, lnk_obj_get_lib_path(cv->obj_arr[obj_idx]));
    }

  ProfScope("Move Global Symbols")
    tp_for_parallel(tp, 0, tp->worker_count, lnk_move_global_symbols_to_gsi, &task);

  ProfScope("Build GSI and PSI")
    pdb_build_gsi_psi(tp, task.pdb);

  ProfScope("Write Modules")
    tp_for_parallel(tp, 0, tp->worker_count, lnk_write_pdb_modules, &task);

  ProfBegin("Add string tables");
  pdb_strtab_add_cv_string_hash_table(&task.pdb->info->strtab, task.string_ht);
  ProfEnd();
  
  ProfBegin("Build Section Contrib Map");
  {
    ProfBegin("Build DBI Section Headers");
    for (U64 sect_idx = 1; sect_idx < task.image_section_table_count; sect_idx += 1) {
      dbi_push_section(task.pdb->dbi, task.image_section_table[sect_idx]);
    }
    ProfEnd();

    for EachIndex(i, task.image_section_table_count) {
      COFF_SectionHeader *sect_header = task.image_section_table[i];
      if (~sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
        task.image_section_file_ranges.v[task.image_section_file_ranges.count++] = rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize);
      }
      task.image_section_virt_ranges.v[i] = rng_1u64(sect_header->voff, sect_header->voff + sect_header->vsize);
    }

    task.sc_list = push_array(scratch.arena, PDB_DbiSectionContribList, cv->obj_count);
    tp_for_parallel(tp, tp_arena, cv->obj_count, lnk_push_dbi_sec_contrib_task, &task);
    dbi_sec_list_concat_arr(&task.pdb->dbi->sec_contrib_list, cv->obj_count, task.sc_list);
  }
  ProfEnd();

  ProfBegin("Build NatVis");
  {
    String8Array natvis_file_path_arr = str8_array_from_list(scratch.arena, &config->natvis_list);
    String8Array natvis_file_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, config->io_flags, natvis_file_path_arr);

    for EachIndex(i, natvis_file_data_arr.count) {
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
      PDB_SrcError error = pdb_add_src(task.pdb->info, task.pdb->msf, natvis_file_path, natvis_file_data, PDB_SrcComp_NULL);
      if (error != PDB_SrcError_OK) {
        lnk_error(LNK_Error_Natvis, "%S", pdb_string_from_src_error(error));
      }
    }
  }
  ProfEnd();

  pdb_build(tp, tp_arena, task.pdb, task.string_ht, 0, cv->is_stripped);

  MSF_Error msf_err = msf_build(task.pdb->msf);
  if (msf_err != MSF_Error_OK) {
    lnk_error(LNK_Error_UnableToSerializeMsf, "unable to serialize MSF: %s", msf_error_to_string(msf_err));
  }

  ProfBegin("Get Page Nodes");
  String8List page_data_list = msf_get_page_data_nodes(tp_arena->v[0], task.pdb->msf);
  ProfEnd();
  
  // NOTE: linker is about to exit so we can skip memory release
  // and let windows free memory since it does this faster
#if 0
  ProfBegin("Context Release");
  pdb_release(&pdb);
  ProfEnd();
#endif

  scratch_end(scratch);
  ProfEnd();
  return page_data_list;
}

