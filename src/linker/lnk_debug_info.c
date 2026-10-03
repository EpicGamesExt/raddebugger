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
  cv_debug_s_exclude_kind(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);

  // discard inline sites
  cv_debug_s_exclude_kind(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_s_task)
{
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  U32List    sect_list      = task->debug_s_section_numbers[obj_idx];
  CV_DebugS *debug_s        = &task->debug_s_arr[obj_idx];
  LNK_Obj   *obj            = task->obj_arr[obj_idx];
  U64        string_count   = 0;
  U64        checksum_count = 0;

  for EachNode(n, U32Node, sect_list.first) {
    String8 raw_debug_s = lnk_obj_section_data_from_number(obj, n->data);

    // Keep one block per section number, even when a section is skipped, so
    // relocation consumers retain the correspondence with the source sections.
    String8Node *block = str8_list_push(arena, &debug_s->data, str8_zero());

    // skip empty .debug$S sections
    if (raw_debug_s.size == 0) { continue; }

    // skip unknown debug info formats
    if (raw_debug_s.size < sizeof(CV_Signature) || cv_signature_from_debug_s(raw_debug_s) != CV_Signature_C13) {
      lnk_error_obj(LNK_Error_IllData, obj, "unsupported .debug$S signature");
      continue;
    }

    String8 data = str8_skip(raw_debug_s, sizeof(CV_Signature));
    CV_DebugS_Iter it = { .debug_s = debug_s, .block = data };
    while (cv_debug_s_iter_next(&it)) {
      string_count   += it.kind == CV_C13SubSectionKind_StringTable;
      checksum_count += it.kind == CV_C13SubSectionKind_FileChksms;
    }
    if (it.malformed) {
      data.size = it.cursor;
      lnk_error_obj(LNK_Error_IllData, obj, ".debug$S subsection outside section");
    }
    block->string = data;
    debug_s->data.total_size += data.size;

    // make sure there is one string table
    if (string_count > 1) {
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S (No. %u) has %u string table sub-sections defined, picking first sub-section", n->data, string_count);
    }

    // make sure there is one file checksum table
    if (checksum_count > 1) {
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S (No. %u) has %u file checksum sub-sections defined, picking first sub-section", n->data, checksum_count);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_h_task)
{
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  LNK_Obj *obj = task->obj_arr[obj_idx];
  if (obj->coff.debug_h_section_number > 0) {
    String8    raw_debug_h     = lnk_obj_section_data_from_number(obj, obj->coff.debug_h_section_number);
    CV_DebugH *debug_h         = &task->debug_h_arr[obj_idx];
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
    if (lnk_hash_kind_from_llvm(ghash.hash_alg) != task->config->debug_types_hash) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H hash algorithm: got %S, expected %S; types will be rehashed",
                    llvm_string_from_ghash_alg(ghash.hash_alg),
                    lnk_string_hash_kind(task->config->debug_types_hash));
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
  }

  exit:;
}

internal
THREAD_POOL_TASK_FUNC(lnk_strip_debug_t_sig_task)
{
  U64               obj_idx = task_id;
  LNK_ParseCvTypes *task    = raw_task;

  String8 *d = &task->raw_types[obj_idx];
  if (d->size == 0) { goto exit; }
  if (d->size < sizeof(CV_Signature)) { lnk_error_obj(LNK_Error_IllData, task->input->obj_arr[obj_idx], ".debug$T must have at least 4 bytes for CodeView signature"); goto exit; }

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

  exit:;
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_t_task)
{
  ProfBeginFunction();
  LNK_ParseCvTypes *task = raw_task;
  task->out_types[task_id] = cv_debug_t_from_data(arena, task->raw_types[task_id], CV_LeafAlign);
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

  String8 type_data_raw                             = {0};
  String8 source_data_arr[CV_TypeIndexSource_COUNT] = {0};
  Rng1U64 ti_ranges      [CV_TypeIndexSource_COUNT] = {0};

  switch (ts->ts_kind) {
  case LNK_TypeServerKind_Null: break;
  case LNK_TypeServerKind_RRT: {
    type_data_raw = ts->rrt->type_data_raw;
    MemoryCopyArray(source_data_arr, ts->rrt->type_data);
    MemoryCopyArray(ti_ranges, ts->rrt->ti_ranges);
  } break;
  case LNK_TypeServerKind_PDB: {
    // read PDB from disk
    B8      msf_data_read = 0;
    String8 msf_data      = lnk_read_data_from_file_path(scratch.arena, ts->ts_path, &msf_data_read);

    if ( ! msf_data_read) {
      LNK_Obj *dep_obj = 0;
      if (ts->obj_indices.count) {
        dep_obj = task->obj_arr[ts->obj_indices.first->data];
      }
      lnk_error_obj(LNK_Error_UnableToOpenTypeServer, dep_obj, "failed to read type server from path: %S", ts->ts_path);
      goto exit;
    }

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

    Rng1U64 leaf_ranges[CV_TypeIndexSource_COUNT] = {0};
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      MSF_StreamNumber sn = type_streams[ti_source];
      if (sn == 0) { continue; }
      if (!pdb_extract_type_server_info(msf_data, st, sn, &ti_ranges[ti_source], &leaf_ranges[ti_source])) { goto exit; }
    }

    // alloc buffer where TPI and IPI are adjacent
    U64 buffer_size = 0;
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { buffer_size += dim_1u64(leaf_ranges[ti_source]); }
    buffer_size = AlignPow2(buffer_size, 4) + ARENA_HEADER_SIZE;
    U8    *buffer = push_array(arena, U8, buffer_size);
    Arena *fixed  = arena_alloc( .reserve_size = buffer_size, .commit_size = buffer_size, .optional_backing_buffer = buffer );

    // read both streams into a contiguous buffer
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      MSF_StreamNumber sn = type_streams[ti_source];
      if (sn == 0) { continue; }
      source_data_arr[ti_source] = msf_data_from_stream_number_ex(fixed, msf_data, st, sn, leaf_ranges[ti_source], PDB_LEAF_ALIGN);
      Assert(source_data_arr[ti_source].size == dim_1u64(leaf_ranges[ti_source]));
    }
    // assert streams are adjacent in the buffer
    for (U64 i = 1; i+1 < CV_TypeIndexSource_COUNT; i += 1) { AssertAlways(source_data_arr[i].str + source_data_arr[i].size == source_data_arr[i+1].str); }
    type_data_raw = str8(buffer + ARENA_HEADER_SIZE, buffer_size - ARENA_HEADER_SIZE);
  } break;
  default: InvalidPath; break;
  }

  // map type server to -> .debug$T
  U64        obj_idx = task->type_server_indices.v[task_id];
  CV_DebugT *debug_t = &task->debug_t_arr[obj_idx];

  // read types
  CV_DebugT d = cv_debug_t_from_data(arena, type_data_raw, PDB_LEAF_ALIGN);

  // @type_server .debugtype_data
  debug_t->count   = d.count;
  debug_t->data    = d.data;
  debug_t->offsets = d.offsets;
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_ranges[i] = ti_ranges[i];                                       }
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_base[i]   = IntFromPtr(source_data_arr[i].str - type_data_raw.str); }
  MemoryCopyTyped(debug_t->source_counts,  d.source_counts, CV_TypeIndexSource_COUNT);
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

internal String8List
lnk_string_list_from_rrt(Arena *arena, LNK_RRT *rrt)
{
  ProfBeginFunction();

  // pack obj file paths
  String8 obj_paths = {0};
  {
    U64 total_obj_file_path_size = 0;
    for EachIndex(i, rrt->obj_count) {
      total_obj_file_path_size += rrt->obj_paths.v[i].size + 1;
    }

    U8 *file_paths_buffer = push_array_no_zero(arena, U8, total_obj_file_path_size);
    U64 file_paths_cursor = 0;
    for EachIndex(i, rrt->obj_count) {
      MemoryCopyStr8(file_paths_buffer + file_paths_cursor, rrt->obj_paths.v[i]);
      file_paths_cursor += rrt->obj_paths.v[i].size;

      file_paths_buffer[file_paths_cursor] = 0;
      file_paths_cursor += 1;
    }
    Assert(file_paths_cursor == total_obj_file_path_size);

    obj_paths = str8(file_paths_buffer, file_paths_cursor);
  }

  String8List rrt_data = {0};

  // (1) magic
  str8_list_push(arena, &rrt_data, g_rrt_magic);

  // (2) version
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, g_rrt_version)));

  // (3) debug types hash
  str8_list_push(arena, &rrt_data, str8_struct(&rrt->debug_types_hash));

  // (4) type data ranges
  str8_list_push(arena, &rrt_data, str8_array_fixed(rrt->type_data_ranges));

  // (5) type data
  str8_list_push(arena, &rrt_data, rrt->type_data_raw);

  // (6) type index ranges
  str8_list_push(arena, &rrt_data, str8_array_fixed(rrt->ti_ranges));

  // (7) type hashes size
  U64 total_hash_count = 0;
  for EachIndex(i, CV_TypeIndexSource_COUNT) { total_hash_count += dim_1u64(rrt->ti_ranges[i]); }
  U64 type_hashes_size = sizeof(**rrt->type_hashes_unpacked) * total_hash_count;
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, type_hashes_size)));

  // (8) type hashes
  for EachIndex(i, CV_TypeIndexSource_COUNT) {
    U64 type_count = dim_1u64(rrt->ti_ranges[i]);
    str8_list_push(arena, &rrt_data, str8_array(rrt->type_hashes_unpacked[i], type_count));
  }

  // (9) object count
  str8_list_push(arena, &rrt_data, str8_struct(&rrt->obj_count));

  // (10) per object type index ranges
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_ti_ranges, rrt->obj_count));

  // (11) per object time stamps
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_time_stamps, rrt->obj_count));

  // (12) per object leaf counts
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_leaf_counts, rrt->obj_count));

  // (13) per object file reverse lookup table for type indices
  for EachIndex(obj_idx, rrt->obj_count) {
    CV_TypeIndex *obj_ti_map   = rrt->obj_ti_maps[obj_idx];
    U64           obj_ti_count = rrt->obj_leaf_counts[obj_idx];
    Assert(dim_1u64(rrt->obj_pch_ti_ranges[obj_idx]) + obj_ti_count <= dim_1u64(rrt->obj_ti_ranges[obj_idx]));
    str8_list_push(arena, &rrt_data, str8_array(obj_ti_map, obj_ti_count));
  }

  // (14) object file paths size
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, obj_paths.size)));

  // (15) object file paths block
  str8_list_push(arena, &rrt_data, obj_paths);

  // (16) PCH type index ranges
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_pch_ti_ranges, rrt->obj_count));

  // (17) PCH object indices
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_pch_indices, rrt->obj_count));

  ProfEnd();
  return rrt_data;
}

internal B32
lnk_rrt_from_string(Arena *arena, String8 rrt_data, String8 path, LNK_RRT *rrt_out)
{
  B32 is_ok = 0;
  U64 cursor = 0;

  // (1) magic
  if (rrt_data.size < g_rrt_magic.size ||
      ! str8_match(str8_prefix(rrt_data, g_rrt_magic.size), g_rrt_magic, 0)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file has invalid magic value", path);
    goto exit;
  }
  cursor += g_rrt_magic.size;

  // (2) version
  U64 version = 0;
  U64 version_size = str8_deserial_read_struct(rrt_data, cursor, &version);
  if (version_size != sizeof(version)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain enough bytes to read the version", path);
    goto exit;
  }
  cursor += version_size;

  // match version
  if (version != g_rrt_version) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT version mismatch, got %llu, expected 2 or %llu", path, version, g_rrt_version);
    goto exit;
  }

  // (3) debug types hash
  LNK_HashKind debug_types_hash = LNK_HashKind_BLAKE3;
  if (version == g_rrt_version) {
    U64 debug_types_hash_size = str8_deserial_read_struct(rrt_data, cursor, &debug_types_hash);
    if (debug_types_hash_size != sizeof(debug_types_hash)) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain enough bytes to read the debug types hash", path);
      goto exit;
    }
    cursor += debug_types_hash_size;

    if (debug_types_hash != LNK_HashKind_BLAKE3 && debug_types_hash != LNK_HashKind_XXHash) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file has invalid debug types hash %u", path, debug_types_hash);
      goto exit;
    }
  }

  // (4) type data ranges
  Rng1U64 type_data_ranges[CV_TypeIndexSource_COUNT] = {0};
  U64 type_data_ranges_size = str8_deserial_read_array(rrt_data, cursor, type_data_ranges, ArrayCount(type_data_ranges));
  if (type_data_ranges_size != sizeof(type_data_ranges)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT is missing type data ranges", path);
    goto exit;
  }
  cursor += type_data_ranges_size;

  // comppute size types from all the sources
  U64 total_type_data_size = 0;
  for EachElement(i, type_data_ranges) total_type_data_size += dim_1u64(type_data_ranges[i]);

  // (5) type data
  String8 type_data_raw      = {0};
  U64     type_data_raw_size = str8_deserial_read_block(rrt_data, cursor, total_type_data_size, &type_data_raw);
  if (type_data_raw_size != total_type_data_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type data file (%M)", path, total_type_data_size);
    goto exit;
  }
  cursor += type_data_raw_size;

  // (6) type index ranges
  Rng1U64 ti_ranges[CV_TypeIndexSource_COUNT] = {0};
  U64 ti_ranges_size = str8_deserial_read_array(rrt_data, cursor, ti_ranges, ArrayCount(ti_ranges));
  if (ti_ranges_size != sizeof(ti_ranges)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain type indices", path);
    goto exit;
  }
  cursor += ti_ranges_size;

  // (7) type hashes size
  U64 type_hashes_size      = 0;
  U64 type_hashes_size_size = str8_deserial_read_struct(rrt_data, cursor, &type_hashes_size);
  if (type_hashes_size_size != sizeof(type_hashes_size)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type hashes", path);
    goto exit;
  }
  cursor += type_hashes_size_size;

  // validate type hashes size
  if (type_hashes_size) {
    U64 type_count = 0;
    for EachIndex(i, CV_TypeIndexSource_COUNT) { type_count += dim_1u64(ti_ranges[i]); }
    U64 expected_size = type_count * sizeof(U64);
    if (expected_size != type_hashes_size) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file type hash size (%llu) does not match expected size (%llu)", path, type_hashes_size, expected_size);
      goto exit;
    }
  }

  // (8) type hashes
  String8 type_hashes = {0};
  U64 type_hashes_read_size = str8_deserial_read_block(rrt_data, cursor, type_hashes_size, &type_hashes);
  if (type_hashes_read_size != type_hashes_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type hashes (%M)", path, type_hashes_size);
    goto exit;
  }
  cursor += type_hashes_read_size;

  // hash count must match type count
  if (type_hashes_size) {
    U64 type_count = 0;
    for EachIndex(i, CV_TypeIndexSource_COUNT) { type_count += dim_1u64(ti_ranges[i]); }
    U64 hash_count = type_hashes.size / sizeof(U64);
    if (type_count != hash_count) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file hash count (%llu) does not match type count (%llu)", path, hash_count, type_count);
      goto exit;
    }
  }

  // (9) object count
  U64 obj_count = 0;
  U64 obj_count_size = str8_deserial_read_struct(rrt_data, cursor, &obj_count);
  if (obj_count_size == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object count", path);
    goto exit;
  }
  cursor += obj_count_size;

  // (10) per object type index ranges
  Rng1U64 *obj_ti_ranges = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_ti_ranges) * obj_count); 
  if (obj_ti_ranges == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object type index ranges", path);
    goto exit;
  }
  cursor += sizeof(*obj_ti_ranges) * obj_count;

  // (11) last observed time stamp of the object files
  U64 *obj_time_stamps = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_time_stamps) * obj_count);
  if (obj_time_stamps == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object timestamps", path);
    goto exit;
  }
  cursor += sizeof(*obj_time_stamps) * obj_count;

  // (12) per object leaf counts
  U64 *obj_leaf_counts = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_leaf_counts) * obj_count);
  if (obj_leaf_counts == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object leaf counts", path);
    goto exit;
  }
  cursor += sizeof(*obj_leaf_counts) * obj_count;

  // (13) per object file reverse lookup table for type indices
  CV_TypeIndex **obj_ti_maps = push_array(arena, CV_TypeIndex *, obj_count);
  for EachIndex(obj_idx, obj_count) {
    U64 obj_ti_count = obj_leaf_counts[obj_idx];
    obj_ti_maps[obj_idx] = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_ti_count * sizeof(*obj_ti_maps[obj_idx]));
    if (obj_ti_maps[obj_idx] == 0) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: failed to read objects type index map from RRT file", path);
      goto exit;
    }
    cursor += obj_ti_count * sizeof(*obj_ti_maps[obj_idx]);
  }

  // (14) object file paths size
  U64 obj_file_paths_size = 0;
  U64 obj_file_paths_size_size = str8_deserial_read_struct(rrt_data, cursor, &obj_file_paths_size);
  if (obj_file_paths_size_size == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object file path block size", path);
    goto exit;
  }
  cursor += obj_file_paths_size_size;

  // (15) object file paths block
  String8 obj_file_paths_block = {0};
  U64 obj_file_paths_block_size = str8_deserial_read_block(rrt_data, cursor, obj_file_paths_size, &obj_file_paths_block);
  if (obj_file_paths_block_size != obj_file_paths_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object file path block (%M)", path, obj_file_paths_size);
    goto exit;
  }
  cursor += obj_file_paths_block_size;

  // (16) PCH type index ranges
  Rng1U64 *obj_pch_ti_ranges = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_count * sizeof(*obj_pch_ti_ranges));
  if (obj_pch_ti_ranges == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read object PCH type index ranges");
    goto exit;
  }
  cursor += obj_count * sizeof(*obj_pch_ti_ranges);

  U32 *obj_pch_indices = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_count * sizeof(*obj_pch_indices));
  if (obj_pch_indices == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read object PCH indices");
    goto exit;
  }
  cursor += obj_count * sizeof(*obj_pch_indices);

  for EachIndex(obj_idx, obj_count) {
    if (dim_1u64(obj_pch_ti_ranges[obj_idx]) + obj_leaf_counts[obj_idx] > dim_1u64(obj_ti_ranges[obj_idx])) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT object leaf count exceeds object type index range", path);
      goto exit;
    }
  }

  if (cursor != rrt_data.size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: failed to parse RRT file", path);
    goto exit;
  }

  // unpack type data
  type_data_raw = push_str8_copy(arena, type_data_raw);
  String8 type_data[CV_TypeIndexSource_COUNT] = {0};
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    type_data[ti_source] = str8_substr(type_data_raw, type_data_ranges[ti_source]);
  }

  // unpack obj file paths
  Temp scratch = scratch_begin(&arena, 1);
  String8List obj_file_paths_list = str8_split_by_string_chars(scratch.arena, obj_file_paths_block, str8_lit("\0"), 0);
  String8Array obj_paths = str8_array_from_list(arena, &obj_file_paths_list);
  scratch_end(scratch);

  // fill out result
  if (rrt_out) {
    rrt_out->path                = path;
    rrt_out->debug_types_hash    = debug_types_hash;
    rrt_out->type_data_raw       = type_data_raw;
    rrt_out->type_hashes         = type_hashes;
    MemoryCopyArray(rrt_out->type_data_ranges, type_data_ranges);
    MemoryCopyArray(rrt_out->type_data,        type_data);
    MemoryCopyArray(rrt_out->ti_ranges,        ti_ranges);
    rrt_out->obj_count           = obj_count;
    rrt_out->obj_leaf_counts     = obj_leaf_counts;
    rrt_out->obj_time_stamps     = obj_time_stamps;
    rrt_out->obj_ti_ranges       = obj_ti_ranges;
    rrt_out->obj_ti_maps         = obj_ti_maps;
    rrt_out->obj_ti_ranges       = obj_ti_ranges;
    rrt_out->obj_ti_maps         = obj_ti_maps;
    rrt_out->obj_paths           = obj_paths;
    rrt_out->obj_pch_ti_ranges   = obj_pch_ti_ranges;
    rrt_out->obj_pch_indices     = obj_pch_indices;
  }

  is_ok = 1;
  exit:;
  return is_ok;
}

internal LNK_RRT_Array
lnk_rrt_array_from_config(Arena *arena, LNK_Config *config)
{
  ProfBegin("Parse RRT");
  LNK_RRT_Array rrt_arr = { .v = push_array(arena, LNK_RRT, config->input_list[LNK_Input_RRT].node_count) };
  for EachNode(n, String8Node, config->input_list[LNK_Input_RRT].first) {
    B8      was_rrt_read = 0;
    String8 rrt_path     = n->string;
    String8 raw_rrt      = lnk_read_data_from_file_path(arena, rrt_path, &was_rrt_read);
    if (raw_rrt.size == 0 || was_rrt_read == 0) {
      lnk_error(LNK_Error_IllData, "ERROR: failed to open \"%S\"", rrt_path);
      continue;
    }
    LNK_RRT rrt = {0};
    lnk_rrt_from_string(arena, raw_rrt, n->string, &rrt);
    rrt_arr.v[rrt_arr.count++] = rrt;
  }
  ProfEnd();
  return rrt_arr;
}

internal LNK_CodeViewInput
lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 obj_count, LNK_Obj **obj_arr, LNK_RRT_Array rrt_input)
{
  ProfBegin("Extract CodeView");
  Temp scratch = scratch_begin(0,0);

  LNK_CodeViewInput input = {
    .config       = config,
    .obj_count    = obj_count,
    .count        = obj_count,
    .obj_arr      = obj_arr,
    .rrt_input    = rrt_input,
    .ts_obj_range = r1u64(0,0),
  };

  HashMap rrt_hm = {0};
  ProfScope("Make obj path -> RRT hash map") {
    for EachIndex(rrt_idx, rrt_input.count) {
      for EachIndex(obj_idx, rrt_input.v[rrt_idx].obj_paths.count) {
        hash_map_push_path_u64(scratch.arena, &rrt_hm, rrt_input.v[rrt_idx].obj_paths.v[obj_idx], Compose64Bit(rrt_idx, obj_idx));
      }
    }
  }

  ProfScope("Apply RRT to Objs") {
    // hash map (obj path, obj idx)
    HashMap obj_path_hm = {0};
    for EachIndex(obj_idx, obj_count) {
      hash_map_push_path_u64(scratch.arena, &obj_path_hm, obj_arr[obj_idx]->path, obj_idx);
    }

    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj            = obj_arr[obj_idx];
      U64     *packed_rrt_idx = hash_map_search_path_u64(&rrt_hm, obj->path);

      // obj is not part of any input RRT
      if (packed_rrt_idx == 0) { continue; }

      // unpack index
      U32      rrt_idx     = *packed_rrt_idx >> 32;
      U32      rrt_obj_idx = *packed_rrt_idx & max_U32;
      LNK_RRT *rrt         = &rrt_input.v[rrt_idx];

      // obj was recompiled, do not apply RRT indirection
      FileProperties obj_file_props = properties_from_file_path(obj->path);
      if (rrt->obj_time_stamps[rrt_obj_idx] != obj_file_props.modified) { continue; }

      // invalidate debug section pointers
      obj->coff.debug_t_section_number = 0;
      obj->coff.debug_p_section_number = 0;
      obj->coff.debug_h_section_number = 0;

      // apply type index map 
      obj->ti_range = rrt->obj_ti_ranges[rrt_obj_idx];
      obj->ti_map   = rrt->obj_ti_maps  [rrt_obj_idx];

      // apply PCH info
      U32 rrt_pch_obj_idx = rrt->obj_pch_indices[rrt_obj_idx];
      if (rrt_pch_obj_idx < rrt->obj_count) {
        String8  rrt_pch_obj_path = rrt->obj_paths.v[rrt_pch_obj_idx];
        U64      pch_obj_idx      = *hash_map_search_path_u64(&obj_path_hm, rrt_pch_obj_path);
        obj->pch_ti_range = rrt->obj_pch_ti_ranges[rrt_obj_idx];
        obj->pch_obj_idx  = pch_obj_idx;
      } else {
        obj->pch_ti_range = r1u64(0,0);
        obj->pch_obj_idx  = ~0;
      }
    }
  }
  
  // collect debug info sections
  input.debug_s_section_numbers = lnk_collect_obj_section_numbers(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$S"), 0);

  // profiler info
  if (lnk_get_log_status(LNK_Log_Debug) || PROFILE_TELEMETRY) {
    U64 total_debug_s_size = 0, total_debug_t_size = 0, total_debug_p_size = 0, total_debug_h_size = 0;
    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj = obj_arr[obj_idx];
      for EachNode(n, U32Node, input.debug_s_section_numbers[obj_idx].first) { total_debug_s_size += lnk_obj_section_data_from_number(obj, n->data).size; }
      if (obj->coff.debug_t_section_number > 0) { total_debug_t_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_t_section_number)->fsize; }
      if (obj->coff.debug_p_section_number > 0) { total_debug_p_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_p_section_number)->fsize; }
      if (config->ghash) {
        if (obj->coff.debug_h_section_number > 0) { total_debug_h_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_h_section_number)->fsize; }
      }
    }

    U64 total_rrt_type_size = 0;
    U64 total_rrt_hash_size = 0;
    for EachIndex(rrt_idx, rrt_input.count) {
      total_rrt_type_size += rrt_input.v[rrt_idx].type_data_raw.size;
      total_rrt_hash_size += rrt_input.v[rrt_idx].type_hashes.size;
    }
	
    ProfNoteV("Total .debug$S Input Size: %M", total_debug_s_size);
    ProfNoteV("Total .debug$T Input Size: %M", total_debug_t_size);
    ProfNoteV("Total .debug$P Input Size: %M", total_debug_p_size);
    ProfNoteV("Total .debug$H Input Size: %M", total_debug_h_size);
    ProfNoteV("Total RRT-Type Input Size: %M", total_rrt_type_size);
    ProfNoteV("Total RRT-Hash Input Size: %M", total_rrt_hash_size);
	
    if (lnk_get_log_status(LNK_Log_Debug)) {
      lnk_log(LNK_Log_Debug, "[Total .debug$S Input Size %M]", total_debug_s_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$T Input Size %M]", total_debug_t_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$P Input Size %M]", total_debug_p_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$H Input Size %M]", total_debug_h_size);
      lnk_log(LNK_Log_Debug, "[Total RRT-Type Input Size %M]", total_rrt_type_size);
      lnk_log(LNK_Log_Debug, "[Total RRT-Hash Input Size %M]", total_rrt_hash_size);
    }
  }

  CV_DebugT *debug_p_arr;
  ProfScope("Parse CodeView") {
    // parse .debug$S
    input.debug_s_arr = push_array(tp_arena->v[0], CV_DebugS, input.obj_count);
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_s_task, &input, "Parse .debug$S");

    // collect .debug$P and .debug$T
    String8 *raw_debug_p_arr = push_array(scratch.arena, String8, obj_count);
    String8 *raw_debug_t_arr = push_array(scratch.arena, String8, obj_count);
    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj = obj_arr[obj_idx];
      if (obj->coff.debug_t_section_number > 0) { raw_debug_t_arr[obj_idx] = lnk_obj_section_data_from_number(obj, obj->coff.debug_t_section_number); }
      if (obj->coff.debug_p_section_number > 0) { raw_debug_p_arr[obj_idx] = lnk_obj_section_data_from_number(obj, obj->coff.debug_p_section_number); }
    }

    LNK_ParseCvTypes parse_types = { .input = &input };

    // .debug$P
    debug_p_arr = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = raw_debug_p_arr;
    parse_types.out_types = debug_p_arr;
    tp_for_parallel_prof(tp, 0,        obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip Signature .debug$P");
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_t_task,     &parse_types, "Parse .debug$P");

    // .debug$T
    input.debug_t_arr     = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = raw_debug_t_arr;
    parse_types.out_types = input.debug_t_arr;
    tp_for_parallel_prof(tp, 0, obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip Signature .debug$T");
    tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_t_task, &parse_types, "Parse .debug$T");

    // .debug$H
    input.debug_h_arr = push_array(tp_arena->v[0], CV_DebugH, input.obj_count);
    if (config->ghash) {
      tp_for_parallel_prof(tp, tp_arena, obj_count, lnk_parse_debug_h_task, &input, "Parse .debug$H");
    }
  }

  // sort objs based on type: PCH, /Zi (external), /Z7 (internal)
  input.debug_p_indices.v = push_array(tp_arena->v[0], U32, obj_count); 
  input.ext_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  input.int_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  for EachIndex(obj_idx, obj_count) {
    CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];
    CV_DebugT *debug_p = &debug_p_arr[obj_idx];
    U32Array  *arr_ptr;
    if      (hash_map_search_path_u64(&rrt_hm, obj_arr[obj_idx]->path)) { arr_ptr = &input.ext_obj_indices; }
    else if (debug_p->count > 0 && debug_t->count == 0)                 { arr_ptr = &input.debug_p_indices; }
    else if (cv_debug_t_is_type_server_ref(debug_t))                    { arr_ptr = &input.ext_obj_indices; }
    else                                                                { arr_ptr = &input.int_obj_indices; }
    arr_ptr->v[arr_ptr->count++] = obj_idx;

    if (debug_t->count == 0 && debug_p->count > 0) {
      *debug_t = *debug_p;
    } else if (debug_t->count && debug_p->count) {
      lnk_error_obj(LNK_Warning_MultipleDebugTAndDebugP, obj_arr[obj_idx], "multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections");
      MemoryZeroStruct(debug_t);
      MemoryZeroStruct(debug_p);
    }
  }

  ProfScope("Set up PDB and RRT") {
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
      U64  obj_idx         = input.ext_obj_indices.v[i];
      U64 *packed_rrt_info = hash_map_search_path_u64(&rrt_hm, obj_arr[obj_idx]->path);

      LNK_TypeServerKind ts_kind = LNK_TypeServerKind_Null;
      CV_TypeServerInfo  ts_info = {0};
      String8            ts_path = {0};
      LNK_RRT           *rrt     = 0;
      if (packed_rrt_info) {
        U32 rrt_idx = *packed_rrt_info >> 32;
        rrt     = &rrt_input.v[rrt_idx];
        ts_kind = LNK_TypeServerKind_RRT;
        ts_path = rrt->path;
      } else {
        CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];
        CV_Leaf   leaf     = cv_debug_t_get_leaf(debug_t, 0);
        ts_kind = LNK_TypeServerKind_PDB;
        ts_info = cv_type_server_info_from_leaf(leaf);
        ts_path = lnk_find_first_file(scratch.arena, config->lib_dir_list, ts_info.name);
      }

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
        ts->ts_kind = ts_kind;
        ts->ts_path = push_str8_copy(tp_arena->v[0], ts_path);
        ts->rrt     = rrt;
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

    // wire RRT hashes to type servers .debug$H
    for EachIndex(ts_idx, ts_arr.count) {
      LNK_TypeServer *ts = &ts_arr.v[ts_idx];
      if (ts->rrt && ts->rrt->debug_types_hash == config->debug_types_hash) {
        U64        ts_obj_idx = input.ts_obj_range.min + ts_idx;
        CV_DebugT *debug_t    = &input.debug_t_arr[ts_obj_idx];
        CV_DebugH *debug_h    = &input.debug_h_arr[ts_obj_idx];
        debug_h->count = ts->rrt->type_hashes.size / sizeof(U64);
        debug_h->v     = (U64 *)ts->rrt->type_hashes.str;
      }
    }

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

  ProfScope("Set up Leaf Discard Bit Arrays") {
    input.is_leaf_discarded = push_array(tp_arena->v[0], U32Array, input.count);
    for EachIndex(obj_idx, input.count) {
      CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];
      input.is_leaf_discarded[obj_idx] = bit_array_init32(tp_arena->v[0], debug_t->count);
    }
  }
 
  ProfScope("Set up PCH") {
    // register PCH file paths
    HashMap debug_p_hm_path = {0};
    HashMap debug_p_hm_name = {0}; // (obj name, U64List of PCH obj indices)
    for EachIndex(i, input.debug_p_indices.count) {
      U64      obj_idx = input.debug_p_indices.v[i];
      LNK_Obj *obj     = obj_arr[obj_idx];

      // register file path -> obj idx map
      String8 obj_path = path_absolute_dst_from_relative_dst_src(scratch.arena, obj_arr[obj_idx]->path, config->work_dir);
      if (hash_map_search_path_u64(&debug_p_hm_path, obj_path)) {
        lnk_error_obj(LNK_Warning_DuplicateObjPath, obj, "duplicate obj path %S", obj_path);
      } else {
        hash_map_push_path_u64(scratch.arena, &debug_p_hm_path, obj_path, obj_idx);
      }

      // register file name -> obj idx map
      String8  obj_name      = str8_skip_last_slash(obj_path);
      U64List *match_indices = hash_map_search_path_raw(&debug_p_hm_name, obj_name);
      if (match_indices == 0) {
        match_indices = push_array(scratch.arena, U64List, 1);
        hash_map_push_path_raw(scratch.arena, &debug_p_hm_name, obj_name, match_indices);
      }
      u64_list_push(scratch.arena, match_indices, obj_idx);
    }

    for EachIndex(i, input.int_obj_indices.count) {
      U64             obj_idx = input.int_obj_indices.v[i];
      CV_DebugT      *debug_t = &input.debug_t_arr[obj_idx];
      CV_PrecompInfo *precomp = &debug_t->precomp;

      // skip objs that do not depend on PCH
      if ( ! debug_t->has_pch) { continue; }

      // find PCH obj by file path
      String8  obj_path            = path_absolute_dst_from_relative_dst_src(scratch.arena, precomp->obj_name, config->work_dir);
      U64     *debug_p_obj_idx_ptr = hash_map_search_path_u64(&debug_p_hm_path, obj_path);
      U64      debug_p_obj_idx     = debug_p_obj_idx_ptr ? *debug_p_obj_idx_ptr : max_U64;

      // fallback: find PCH obj by signature
      if (debug_p_obj_idx_ptr == 0) {
        for EachIndex(pch_i, input.debug_p_indices.count) {
          U64        pch_obj_idx = input.debug_p_indices.v[pch_i];
          CV_DebugT *pch_debug_t = &input.debug_t_arr[pch_obj_idx];
          if (precomp->leaf_count >= pch_debug_t->count) { continue; }

          CV_Leaf end_leaf = cv_debug_t_get_leaf(pch_debug_t, precomp->leaf_count);
          if (end_leaf.kind != CV_LeafKind_ENDPRECOMP)        { continue; }
          if (end_leaf.data.size < sizeof(CV_LeafEndPreComp)) { continue; }

          CV_LeafEndPreComp *end_precomp = str8_deserial_get_raw_ptr(end_leaf.data, 0, sizeof(*end_precomp));
          if (end_precomp->sig != precomp->sig) { continue; }

          debug_p_obj_idx = pch_obj_idx;
          break;
        }
      }

      if (debug_p_obj_idx > input.obj_count) {
        lnk_error_obj(LNK_Error_PrecompObjNotFound, obj_arr[obj_idx], "LF_PRECOMP references non-existent obj %S; discarding debug info", obj_path);
        lnk_discard_cv_debug_info(&input, obj_idx);
        continue;
      }

      // get PCH leaf data
      CV_DebugT *debug_p = &input.debug_t_arr[debug_p_obj_idx];

      // error check LF_PRECOMP
      if (precomp->start_index > CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Warning_AtypicalStartIndex,    obj_arr[obj_idx], "atypical start index 0x%x in LF_PRECOMP", precomp->start_index); }
      if (precomp->start_index < CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Error_InvalidStartIndex,       obj_arr[obj_idx], "invalid start index 0x%x in LF_PRECOMP; must be >= 0x%x", precomp->start_index, CV_MinComplexTypeIndex); continue; }
      if (precomp->leaf_count  >= debug_p->count)        { lnk_error_obj(LNK_Error_InvalidPrecompLeafCount, obj_arr[obj_idx], "leaf count %u LF_PRECOMP exceeds leaf count %u in .debug$P in %S", precomp->leaf_count, debug_p->count, obj_arr[debug_p_obj_idx]->path); continue; }

      // get LF_ENDPRECOMP
      CV_Leaf            endprecomp_leaf = cv_debug_t_get_leaf(debug_p, precomp->leaf_count);
      CV_LeafEndPreComp *endprecomp      = str8_deserial_get_raw_ptr(endprecomp_leaf.data, 0, sizeof(*endprecomp));

      // error check LF_ENDPRECOMP
      if (endprecomp_leaf.kind      != CV_LeafKind_ENDPRECOMP)    { lnk_error_obj(LNK_Error_EndprecompNotFound, obj_arr[obj_idx], "missing LF_ENDPRECOMP [0x%x] in %S", precomp->leaf_count, obj_arr[debug_p_obj_idx]->path); continue; }
      if (endprecomp_leaf.data.size != sizeof(CV_LeafEndPreComp)) { lnk_error_obj(LNK_Error_IllData,            obj_arr[obj_idx], "invalid size 0x%x for LF_ENDPRECOMP", endprecomp_leaf.data.size); continue; }
      if (endprecomp->sig           != precomp->sig)              { lnk_error_obj(LNK_Error_PrecompSigMismatch, obj_arr[obj_idx], "PCH signature mismatch, expected 0x%x got 0x%x; PCH obj %S", precomp->sig, endprecomp->sig, obj_arr[debug_p_obj_idx]->path); continue; }

      // associate .debug$P and .debug$T
      debug_t->pch_obj_idx = debug_p_obj_idx;
    }

    // discard LF_ENDPRECOMP from .debug$P
    for EachIndex(i, input.debug_p_indices.count) {
      U64            debug_p_idx = input.debug_p_indices.v[i];
      CV_DebugT     *debug_p     = &input.debug_t_arr[debug_p_idx];
      for EachIndex(i, debug_p->count) {
        U64            lf_idx = debug_p->count - (i + 1);
        CV_LeafHeader *lf     = cv_debug_t_get_leaf_header(debug_p, lf_idx);
        if (lf->kind == CV_LeafKind_ENDPRECOMP) {
          bit_array_set_bit32(input.is_leaf_discarded[debug_p_idx], lf_idx, 1);
          break;
        }
      }
    }
  }

  scratch_end(scratch);
  ProfEnd();
  return input;
}

internal force_inline LNK_LeafRef
lnk_leaf_ref_make(U64 obj_idx, U64 leaf_idx)
{
  LNK_LeafRef result = Compose64Bit(safe_cast_u32(obj_idx), safe_cast_u32(leaf_idx));
  Assert(result != LNK_LEAF_REF_NULL);
  return result;
}

internal force_inline U32
lnk_leaf_ref_obj_idx(LNK_LeafRef ref)
{
  return (U32)(ref >> 32);
}

internal force_inline U32
lnk_leaf_ref_leaf_idx(LNK_LeafRef ref)
{
  return (U32)ref;
}

internal LNK_LeafRef
lnk_leaf_ref_from_ti(LNK_CodeViewInput *input, U32 obj_idx, CV_TypeIndexSource source, CV_TypeIndex ti)
{
  // ti range: external type server
  U64 ts_idx = input->obj_to_ts[obj_idx];
  if (ts_idx != max_U64) {
    U64             ts_debug_t_idx = input->ts_obj_range.min + ts_idx;
    CV_DebugT      *ts_debug_t     = input->debug_t_arr + ts_debug_t_idx;
    LNK_TypeServer *ts             = &input->ts_arr.v[ts_idx];

    // RRT indirection
    if (ts->rrt) {
      LNK_Obj *obj = input->obj_arr[obj_idx];

      // RRT-PCH indirection
      if (contains_1u64(obj->pch_ti_range, ti)) {
        obj_idx = obj->pch_obj_idx;
        obj     = input->obj_arr[obj_idx];
      }

      Assert(contains_1u64(obj->ti_range, ti));

      // translate type index to original leaf index
      U64 leaf_idx_og  = (ti - obj->ti_range.min);
      leaf_idx_og     -= dim_1u64(obj->pch_ti_range);

      // map original leaf index to RRT type index
      CV_TypeIndex final_ti = obj->ti_map[leaf_idx_og];
      return lnk_leaf_ref_make(ts_debug_t_idx, cv_leaf_idx_from_ti(ts_debug_t, source, final_ti));
    } 
    
    return lnk_leaf_ref_make(ts_debug_t_idx, cv_leaf_idx_from_ti(ts_debug_t, source, ti));
  }

  CV_DebugT *debug_t = input->debug_t_arr + obj_idx;

  // ti_range: PCH
  if (contains_1u64(debug_t->pch_ti_range[source], ti)) {
    return lnk_leaf_ref_make(debug_t->pch_obj_idx,
                             cv_leaf_idx_from_ti(&input->debug_t_arr[debug_t->pch_obj_idx], source, ti));
  }

  // ti range: internal type server
  return lnk_leaf_ref_make(obj_idx, cv_leaf_idx_from_ti(debug_t, source, ti));
}

internal U64
lnk_hash_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  return input->debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)].v[lnk_leaf_ref_leaf_idx(leaf_ref)];
}

internal int
lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b)
{
  return a < b ? -1 : a > b ? +1 : 0;
}

internal B32
lnk_match_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef a, LNK_LeafRef b)
{
  U64 a_hash = lnk_hash_from_leaf_ref(input, a);
  U64 b_hash = lnk_hash_from_leaf_ref(input, b);
  return a_hash == b_hash;
}

internal U64
lnk_hash_cv_leaf(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list, B32 discard_cycles)
{
  U32                 obj_idx        = lnk_leaf_ref_obj_idx(leaf_ref);
  U32                 leaf_idx       = lnk_leaf_ref_leaf_idx(leaf_ref);
  CV_DebugT          *debug_t        = &input->debug_t_arr[obj_idx];
  CV_Leaf             leaf           = cv_debug_t_get_leaf(debug_t, leaf_idx);
  CV_TypeIndexSource  curr_ti_source = cv_type_index_source_from_leaf_kind(leaf.kind);
  CV_TypeIndex        curr_ti        = cv_ti_from_leaf_idx(debug_t, curr_ti_source, leaf_idx);

  // init hasher
  LNK_Hasher hasher;
  lnk_hasher_init(&hasher, input->config->debug_types_hash);

  // hash bytes around indices
  {
    U64 last_ti_off = 0;
    for EachNode(ti_info, CV_TypeIndexInfo, ti_info_list.first) {
      U8 *bytes = leaf.data.str + last_ti_off;
      U64 size  = ti_info->offset - last_ti_off;
      lnk_hasher_update(&hasher, bytes, size);
      last_ti_off = ti_info->offset + sizeof(CV_TypeIndex);
    }

    Assert(leaf.data.size >= last_ti_off);
    U8 *bytes = leaf.data.str + last_ti_off;
    U64 size  = leaf.data.size - last_ti_off;
    lnk_hasher_update(&hasher, bytes, size);
  }

  // mix-in sub leaf hashes
  for EachNode(sub_ti_n, CV_TypeIndexInfo, ti_info_list.first) {
    CV_TypeIndex *sub_ti_ptr = str8_deserial_get_raw_ptr(leaf.data, sub_ti_n->offset, sizeof(*sub_ti_ptr));
    CV_TypeIndex  sub_ti     = memory_read32(sub_ti_ptr);
    
    // simple indices are stable across compile units 
    if (sub_ti < debug_t->ti_ranges[sub_ti_n->source].min) {
      lnk_hasher_update_struct(&hasher, &sub_ti);
      continue;
    }

    if (sub_ti >= debug_t->ti_ranges[sub_ti_n->source].max) {
      // discard type
      U32 leaf_idx = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      bit_array_set_bit32(input->is_leaf_discarded[obj_idx], leaf_idx, 1);

      // reset hasher
      lnk_hasher_init(&hasher, input->config->debug_types_hash);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) out of bounds type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // discard type with a cyclic-ref
    B32 is_type_graph_cyclic = discard_cycles && sub_ti > 0 && sub_ti > curr_ti;
    if (is_type_graph_cyclic) {
      // discard type
      U32 leaf_idx = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      bit_array_set_bit32(input->is_leaf_discarded[obj_idx], leaf_idx, 1);

      // reset hasher
      lnk_hasher_init(&hasher, input->config->debug_types_hash);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) forward refs member type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, sub_ti_n->offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // type index -> hash
    LNK_LeafRef sub_ref  = lnk_leaf_ref_from_ti(input, obj_idx, sub_ti_n->source, sub_ti);
    U64         sub_hash = input->debug_h_arr[lnk_leaf_ref_obj_idx(sub_ref)].v[lnk_leaf_ref_leaf_idx(sub_ref)];

    // mix-in sub-type hash
    lnk_hasher_update_struct(&hasher, &sub_hash);
  }

  // hash leaf header
  CV_LeafHeader *leaf_header_ptr = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
  lnk_hasher_update_struct(&hasher, leaf_header_ptr);

  // finalize the type hash
  U64 hash = lnk_hasher_digest64(&hasher);

  Assert(hash != 0);
  Assert(input->debug_h_arr[obj_idx].v[leaf_idx] == 0 ||
         input->debug_h_arr[obj_idx].v[leaf_idx] == 1);
  input->debug_h_arr[obj_idx].v[leaf_idx] = hash;

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
  U32        root_obj_idx = lnk_leaf_ref_obj_idx(root_leaf_ref);
  CV_DebugT *root_debug_t = &input->debug_t_arr[root_obj_idx];
  HashStack *root_frame = push_array(temp.arena, HashStack, 1);
  root_frame->leaf_ref     = root_leaf_ref;
  root_frame->ti_info_list = root_ti_info_list;
  root_frame->ti_info      = root_ti_info_list.first;
  root_frame->leaf         = cv_debug_t_get_leaf(root_debug_t, lnk_leaf_ref_leaf_idx(root_leaf_ref));
  root_frame->ti_source    = cv_type_index_source_from_leaf_kind(root_frame->leaf.kind);
  root_frame->ti           = cv_ti_from_leaf_idx(root_debug_t, root_frame->ti_source, lnk_leaf_ref_leaf_idx(root_leaf_ref));

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
      if ( ! contains_1u64(input->debug_t_arr[root_obj_idx].ti_ranges[ti_info->source], ti)) { continue; }

      // skip hashed types
      LNK_LeafRef leaf_ref = lnk_leaf_ref_from_ti(input, root_obj_idx, ti_info->source, ti);
      U32         obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
      U32         leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
      if (input->debug_h_arr[obj_idx].v[leaf_idx] != 0) { continue; }
      input->debug_h_arr[obj_idx].v[leaf_idx] = 1;

      // recurse down to sub types
      HashStack *frame = push_array(temp.arena, HashStack, 1);
      frame->leaf_ref     = leaf_ref;
      frame->leaf         = cv_debug_t_get_leaf(&input->debug_t_arr[obj_idx], leaf_idx);
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

internal CV_TypeIndex
lnk_assigned_ti_hash_search(LNK_AssignedTiHash *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  if (ht->cap == 0) { return 0; }
  CV_DebugH *debug_h  = &input->debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)];
  U64        hash     = debug_h->v[lnk_leaf_ref_leaf_idx(leaf_ref)];
  U64        best_idx = hash % ht->cap;
  U64        idx      = best_idx;
  do {
    CV_TypeIndex ti = ht->ti_arr[idx];
    if (ti == 0) { break; }
    if (lnk_hash_from_leaf_ref(input, ht->leaf_refs[ti - ht->min_ti]) == hash) { return ti; }
    idx = (idx + 1) == ht->cap ? 0 : (idx + 1);
  } while (idx != best_idx);

  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  LNK_MergeTypes *task         = raw_task;
  U32             obj_idx      = task->indices.v[task_id];
  CV_DebugT      *debug_t      = &task->input->debug_t_arr[obj_idx];

  for EachIndex(leaf_idx, debug_t->count) {
    Temp                 temp    = temp_begin(scratch.arena);
    CV_Leaf              leaf    = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexInfoList ti_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf(task->input, lnk_leaf_ref_make(obj_idx, leaf_idx), ti_list, 1);
    temp_end(temp);
  }
  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_deep_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  for EachIndex(leaf_idx, debug_t->count) {
    if (task->input->debug_h_arr[obj_idx].v[leaf_idx] != 0) { continue; }
    Temp                 temp    = temp_begin(scratch.arena);
    CV_Leaf              leaf    = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexInfoList ti_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf_deep(temp.arena, task->input, lnk_leaf_ref_make(obj_idx, leaf_idx), ti_list);
    temp_end(temp);
  }
  scratch_end(scratch);
  ProfEnd();
}

internal U64
lnk_leaf_hash_table_double_cap(U64 cap)
{
  if (cap > (max_U64 - ARENA_HEADER_SIZE) / (2 * sizeof(LNK_LeafRef))) {
    lnk_internal_error(LNK_InternalError_OutOfMemory, __FILE__, __LINE__, "leaf hash table capacity overflow");
    lnk_exit(1);
  }
  return cap * 2;
}

internal LNK_LeafHashTable
lnk_leaf_hash_table_alloc(U64 cap)
{
  Assert(IsPow2(cap) && cap >= LNK_LEAF_HT_MIN_CAP);
  U64    size  = ARENA_HEADER_SIZE + cap * sizeof(LNK_LeafRef);
  Arena *arena = arena_alloc(.name = "LEAF_BUCKETS", .reserve_size = size, .commit_size = size);

  LNK_LeafHashTable  result = { 
    .arena       = arena,
    .cap         = cap,
    .initial_cap = cap,
    .bucket_arr  = push_array_no_zero(arena, LNK_LeafRef, cap)
  };
  MemorySet(result.bucket_arr, 0xff, cap * sizeof(LNK_LeafRef));
  
  return result;
}

typedef enum
{
  LNK_LeafInsert_Full,
  LNK_LeafInsert_Existing,
  LNK_LeafInsert_New,
} LNK_LeafInsert;

internal force_inline LNK_LeafInsert
lnk_leaf_hash_table_insert(LNK_CodeViewInput *input, LNK_LeafHashTable *ht, LNK_LeafRef ref, U64 hash, B32 dedup)
{
  U64 best_idx = hash & (ht->cap - 1);
  U64 idx      = best_idx;
  
  do {
    LNK_LeafRef curr = ins_atomic_u64_eval(&ht->bucket_arr[idx]);
    while (curr == LNK_LEAF_REF_NULL || (dedup && lnk_hash_from_leaf_ref(input, curr) == hash)) {
      if (curr != LNK_LEAF_REF_NULL && lnk_leaf_ref_compare(ref, curr) >= 0) {
        return LNK_LeafInsert_Existing;
      }

      LNK_LeafRef cmp = ins_atomic_u64_eval_cond_assign(&ht->bucket_arr[idx], ref, curr);
      
      if (cmp == curr) {
        return curr == LNK_LEAF_REF_NULL ? LNK_LeafInsert_New : LNK_LeafInsert_Existing;
      }
      curr = cmp;
    }
    idx = (idx + 1) & (ht->cap - 1);
  } while (idx != best_idx);

  return LNK_LeafInsert_Full;
}

internal void
lnk_leaf_hash_table_count_inserts(LNK_MergeTypes *task, CV_TypeIndexSource source, U64 count)
{
  if (count) {
    LNK_LeafHashTable *ht = &task->leaf_ht_arr[source];
    U64 total = ins_atomic_u64_add_eval(&ht->count, count);
    if (total >= (ht->cap - (ht->cap / 4))) {
      ins_atomic_u32_or(&task->grow_mask, 1u << source);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_dedup_task)
{
  LNK_MergeTypes    *task         = raw_task;
  LNK_LeafDedupWork *work         = &task->dedup_work[task_id];
  U64                obj_idx      = work->obj_idx;
  CV_DebugT         *debug_t      = &task->input->debug_t_arr[obj_idx];
  CV_DebugH         *debug_h      = &task->input->debug_h_arr[obj_idx];
  U32Array           is_discarded = task->input->is_leaf_discarded[obj_idx];
  
  ProfBeginDynamic("dedup in obj 0x%llx (%.*s) leaf count %llu", obj_idx, str8_varg(task->input->obj_arr[obj_idx]->path), debug_t->count);

  U64 inserted[CV_TypeIndexSource_COUNT] = {0};
  U64 leaf_idx = work->cursor;
  for (; leaf_idx < work->opl; ++leaf_idx) {
    if ((leaf_idx & 63) == 0 && ins_atomic_u32_eval(&task->grow_mask)) { break; }
    
    LNK_LeafRef leaf_ref = lnk_leaf_ref_make(obj_idx, leaf_idx);
    if (!task->skip_dedup && bit_array_is_set32(is_discarded, leaf_idx)) { continue; }

    CV_LeafHeader      *header      = cv_debug_t_get_leaf_header(debug_t, leaf_idx);             // leaf index -> leaf header
    CV_LeafKind         kind        = memory_read16(MemberFromPtr(CV_LeafHeader, header, kind)); // leaf header -> leaf kind
    CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(kind);                 // leaf kind -> type stream
    LNK_LeafHashTable  *leaf_ht     = &task->leaf_ht_arr[leaf_source];                           // type stream -> hash table
    LNK_LeafInsert      result      = lnk_leaf_hash_table_insert(task->input, leaf_ht, leaf_ref, debug_h->v[leaf_idx], !task->skip_dedup);
    
    if (result == LNK_LeafInsert_Full) {
      ins_atomic_u32_or(&task->grow_mask, 1u << leaf_source);
      break; // retry this leaf after growth, including in release builds
    }

    if (result == LNK_LeafInsert_New && ++inserted[leaf_source] == 64) {
      lnk_leaf_hash_table_count_inserts(task, leaf_source, inserted[leaf_source]);
      inserted[leaf_source] = 0;
    }
  }
  work->cursor = leaf_idx;
  
  for EachIndex(source, CV_TypeIndexSource_COUNT) {
    lnk_leaf_hash_table_count_inserts(task, source, inserted[source]);
  }

  ProfEnd();
}

typedef struct
{
  LNK_CodeViewInput *input;
  LNK_LeafHashTable *src;
  LNK_LeafHashTable *dst;
  Rng1U64           *ranges;
} LNK_RehashLeaves;

internal
THREAD_POOL_TASK_FUNC(lnk_rehash_leaves_task)
{
  LNK_RehashLeaves *task = raw_task;
  for EachInRange(i, task->ranges[task_id]) {
    LNK_LeafRef ref = task->src->bucket_arr[i];
    if (ref == LNK_LEAF_REF_NULL) { continue; }
    U64            hash   = lnk_hash_from_leaf_ref(task->input, ref);
    LNK_LeafInsert result = lnk_leaf_hash_table_insert(task->input, task->dst, ref, hash, 0);
    AssertAlways(result == LNK_LeafInsert_New);
  }
}

internal void
lnk_grow_leaf_hash_table(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafHashTable *ht)
{
  Temp scratch = scratch_begin(0,0);
  
  LNK_LeafHashTable grown = lnk_leaf_hash_table_alloc(lnk_leaf_hash_table_double_cap(ht->cap));
  
  LNK_RehashLeaves rehash = {
    input,
    ht,
    &grown,
    tp_divide_work(scratch.arena, ht->cap, tp->worker_count)
  };
  tp_for_parallel(tp, 0, tp->worker_count, lnk_rehash_leaves_task, &rehash);
  
  grown.count       = ht->count;
  grown.initial_cap = ht->initial_cap;
  grown.grow_count  = ht->grow_count + 1;

  arena_release(ht->arena);
  
  *ht = grown;
  
  scratch_end(scratch);
}

internal void
lnk_dedup_leaf_work(TP_Context *tp, LNK_MergeTypes *task, U64 count, LNK_LeafDedupWork *work, B32 skip_dedup)
{
  task->dedup_work = work;
  task->skip_dedup = skip_dedup;
  for (;;) {
    task->grow_mask = 0;
    tp_for_parallel(tp, 0, count, lnk_leaf_dedup_task, task);
    if (!task->grow_mask) { break; }

    B32 pending = 0;
    for EachIndex(i, count) { pending |= work[i].cursor != work[i].opl; }
    if (!pending) { break; }

    for EachIndex(source, CV_TypeIndexSource_COUNT) {
      if (task->grow_mask & (1u << source)) {
        lnk_grow_leaf_hash_table(tp, task->input, &task->leaf_ht_arr[source]);
      }
    }
  }
}

internal void
lnk_dedup_leaf_indices(TP_Context *tp, LNK_MergeTypes *task, U32Array indices)
{
  Temp scratch = scratch_begin(0,0);
  LNK_LeafDedupWork *work = push_array(scratch.arena, LNK_LeafDedupWork, indices.count);
  for EachIndex(i, indices.count) {
    work[i].obj_idx = indices.v[i];
    work[i].opl = task->input->debug_t_arr[indices.v[i]].count;
  }
  lnk_dedup_leaf_work(tp, task, indices.count, work, 0);
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_present_buckets_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task = raw_task;
  LNK_LeafHashTable *ht   = &task->leaf_ht_arr[task->ti_source];

  for EachInRange(bucket_idx, task->ranges[task_id]) {
    if (ht->bucket_arr[bucket_idx] != LNK_LEAF_REF_NULL) {
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
    if (ht->bucket_arr[bucket_idx] != LNK_LEAF_REF_NULL) {
      unique_leaf_refs.v[cursor++] = ht->bucket_arr[bucket_idx];
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_assign_type_indices_task)
{
  LNK_MergeTypes *task  = raw_task;

  CV_TypeIndexSource  ti_source         = task->ti_source;
  LNK_LeafRefArray    unique_leaf_refs  = task->unique_leaf_refs_arr[ti_source];
  CV_TypeIndex        min_type_index    = task->min_type_indices[ti_source];
  LNK_AssignedTiHash *assigned          = &task->assigned_ti_arr[ti_source];
  CV_DebugH          *debug_h_arr       = task->input->debug_h_arr;

  for EachInRange(i, task->ranges[task_id]) {
    LNK_LeafRef   leaf_ref   = unique_leaf_refs.v[i];
    CV_TypeIndex  type_index = min_type_index + i;

    U64 hash     = debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)].v[lnk_leaf_ref_leaf_idx(leaf_ref)];
    U64 best_idx = hash % assigned->cap;
    U64 idx      = best_idx;

    B32 is_inserted = 0;
    do {
      CV_TypeIndex curr_type_index = ins_atomic_u32_eval(&assigned->ti_arr[idx]);
      if (curr_type_index == 0) {
        CV_TypeIndex cmp_type_index = ins_atomic_u32_eval_cond_assign(&assigned->ti_arr[idx], type_index, curr_type_index);
        if (cmp_type_index == curr_type_index) {
          is_inserted = 1;
          break;
        }
      }
      // advance
      idx = (idx + 1) == assigned->cap ? 0 : (idx + 1);
    } while (idx != best_idx);
    Assert(is_inserted);
  }
}

internal void
lnk_fixup_cv_type_indices(LNK_MergedTypes *ctx, U32 obj_idx, String8 data, CV_TypeIndexInfoList ti_info_list)
{
  if (ctx == 0 || ctx->input == 0) { return; }
  for EachNode(n, CV_TypeIndexInfo, ti_info_list.first) {
    CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(data, n->offset, sizeof(*ti_ptr));
    if (ti_ptr == 0) {
      lnk_error_obj(LNK_Error_CvIllSymbolData, ctx->input->obj_arr[obj_idx], "type index outside debug record");
      continue;
    }
    CV_TypeIndex  ti     = memory_read32(ti_ptr);

    // skip basic types
    if (ti < ctx->input->min_type_indices[n->source]) { continue; }

    LNK_LeafRef  leaf_ref = lnk_leaf_ref_from_ti(ctx->input, obj_idx, n->source, ti);
    CV_TypeIndex final_ti = lnk_assigned_ti_hash_search(&ctx->assigned_ti_arr[n->source], ctx->input, leaf_ref);
    memory_write32(ti_ptr, final_ti);

#if LNK_PARANOID
    if (final_ti == 0) {
      lnk_error_obj(LNK_Error_InvalidTypeIndex, ctx->input->obj_arr[obj_idx], "no itype 0x%x", ti);
    }
#endif
  }
}

internal void
lnk_patch_type_copy(Arena *arena, LNK_MergedTypes *types, CV_TypeIndexSource source, U64 leaf_idx, U8 *dst)
{
  CV_Leaf leaf = cv_leaf_from_ptr(dst);
  U32 obj_idx = lnk_leaf_ref_obj_idx(types->leaf_refs[source].v[leaf_idx]);
  CV_TypeIndexInfoList indices = cv_get_leaf_type_index_offsets(arena, leaf.kind, leaf.data);
  lnk_fixup_cv_type_indices(types, obj_idx, leaf.data, indices);
}

internal String8
lnk_raw_type_from_index(LNK_MergedTypes *types, CV_TypeIndexSource source, U64 idx)
{
  LNK_LeafRef ref = types->leaf_refs[source].v[idx];
  CV_DebugT *debug_t = &types->input->debug_t_arr[lnk_leaf_ref_obj_idx(ref)];
  return cv_debug_t_get_raw_leaf(debug_t, lnk_leaf_ref_leaf_idx(ref));
}

internal void
lnk_patch_symbol_copy(LNK_MergedTypes *types, U32 obj_idx, CV_Symbol symbol)
{
  CV_SymKind final_kind = cv_final_symbol_kind(symbol.kind);

  if (final_kind != symbol.kind) {
    if (CV_IsProc32(symbol.kind)) {
      CV_SymProc32 *proc32 = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*proc32));
      if (proc32 == 0) {
        lnk_error_obj(LNK_Error_CvIllSymbolData, types->input->obj_arr[obj_idx], "truncated procedure record");
        goto exit;
      }

      CV_TypeIndex id = memory_read32(&proc32->itype);
      if (id >= types->input->min_type_indices[CV_TypeIndexSource_IPI]) {
        // FUNC_ID and MFUNC_ID carry a TPI function type in their second field.
        LNK_LeafRef id_ref     = lnk_leaf_ref_from_ti(types->input, obj_idx, CV_TypeIndexSource_IPI, id);
        U32         id_obj_idx = lnk_leaf_ref_obj_idx(id_ref);
        CV_Leaf     id_leaf    = cv_debug_t_get_leaf(&types->input->debug_t_arr[id_obj_idx], lnk_leaf_ref_leaf_idx(id_ref));

        if (id_leaf.kind != CV_LeafKind_FUNC_ID && id_leaf.kind != CV_LeafKind_MFUNC_ID) {
          lnk_error_obj(LNK_Error_InvalidTypeIndex, types->input->obj_arr[obj_idx], "procedure ID does not reference a function ID");
          goto exit;
        }
        if (id_leaf.data.size < 2 * sizeof(CV_TypeIndex)) {
          lnk_error_obj(LNK_Error_CvIllSymbolData, types->input->obj_arr[obj_idx], "truncated procedure ID id_leaf");
          goto exit;
        }

        // extract procedure type ID type
        CV_TypeIndex ti = memory_read32(id_leaf.data.str + sizeof(CV_TypeIndex));

        // basic/no-type values are copied unchanged; only complex types need lookup
        if (ti >= types->input->min_type_indices[CV_TypeIndexSource_TPI]) {
          LNK_AssignedTiHash *tpi_ht   = &types->assigned_ti_arr[CV_TypeIndexSource_TPI];
          LNK_LeafRef         type_ref = lnk_leaf_ref_from_ti(types->input, id_obj_idx, CV_TypeIndexSource_TPI, ti);
          ti = lnk_assigned_ti_hash_search(tpi_ht, types->input, type_ref);
        }

        memory_write32(&proc32->itype, ti);
      }
    }

    // update symbol kind
    memory_write16(cv_kind_ptr_from_symbol(symbol), final_kind);
  } else {
    Temp scratch = scratch_begin(0, 0);
    CV_TypeIndexInfoList indices = cv_get_symbol_type_index_offsets(scratch.arena, symbol.kind, symbol.data);
    lnk_fixup_cv_type_indices(types, obj_idx, symbol.data, indices);
    scratch_end(scratch);
  }

  exit:;
}

internal
THREAD_POOL_TASK_FUNC(lnk_unbucket_hashes_task)
{
  LNK_MergeTypes *task = raw_task;
  Rng1U64 range = task->ranges[task_id];
  for EachInRange(i, range) {
    LNK_LeafRef  leaf_ref = task->unique_leaf_refs_arr[task->ti_source].v[i];
    U32          obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
    U32          leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
    CV_DebugT   *debug_t  = &task->input->debug_t_arr[obj_idx];
    String8      raw_leaf = cv_debug_t_get_raw_leaf(debug_t, leaf_idx);
    task->result.hashes[task->ti_source][i] = task->input->debug_h_arr[obj_idx].v[leaf_idx];
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_obj_ti_map)
{
  LNK_MergeTypes      *task  = raw_task;
  LNK_CodeViewInput   *input = task->input;

  U64           obj_idx    = task_id;
  CV_DebugT    *debug_t    = &input->debug_t_arr[obj_idx];
  CV_TypeIndex *obj_ti_map = task->obj_ti_batch + task->obj_ti_map_offsets[obj_idx];

  for EachIndex(leaf_idx, debug_t->count) {
    CV_Leaf             leaf       = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexSource  source     = cv_type_index_source_from_leaf_kind(leaf.kind);
    LNK_LeafRef         leaf_ref = lnk_leaf_ref_make(obj_idx, leaf_idx);
    LNK_AssignedTiHash *assigned = &task->assigned_ti_arr[source];
    obj_ti_map[leaf_idx] = lnk_assigned_ti_hash_search(assigned, input, leaf_ref);
  }

  task->result.obj_ti_maps[obj_idx] = obj_ti_map;
}

internal LNK_MergedTypes
lnk_merge_types(TP_Context *tp, Arena *arena, LNK_CodeViewInput *input, LNK_MergeTypeFlags merge_flags)
{
  ProfBeginFunction();

  // dedup tables can be much larger than the eventual PDB; give them their
  // own lifetime instead of retaining their backing in the PDB arena's cache
  Arena *merge_arena = arena_alloc(.name = "TYPE_MERGE");
  
  Temp scratch = temp_begin(merge_arena);

  LNK_MergeTypes task = { .input = input, .result.input = input };

  ProfScope("Produce Hashes")
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
          debug_h->v     = push_array(arena, U64, debug_h->count);

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

  ProfBegin("Leaf Hash Table Init");
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    // largest individual type set is a cheap starting hint
    U64 largest_type_count = 0;
    for EachIndex(obj_idx, input->count) {
      largest_type_count = Max(largest_type_count, input->debug_t_arr[obj_idx].source_counts[ti_source]);
    }

    // choose capacity that can hold largest_type_count at no more than 75% occupancy
    U64 cap = LNK_LEAF_HT_MIN_CAP;
    while ((cap - (cap / 4)) < largest_type_count) { cap = lnk_leaf_hash_table_double_cap(cap); }
    
    task.leaf_ht_arr[ti_source] = lnk_leaf_hash_table_alloc(cap);

    #if PROFILE_TELEMETRY
    tmMessage(0, TMMF_ICON_NOTE, "%.*s Bucket Count: %.*s", str8_varg(cv_string_from_type_index_source(ti_source)), str8_varg(str8_from_count(scratch.arena, task.leaf_ht_arr[ti_source].cap)));
    #endif
  }
  ProfEnd();

  U32Array dedup_type_server_indices = input->type_server_indices;

  ProfScope("Prepopulate hash table with largest type-set")
  {
    LNK_TypeServer *largest_ts = 0;
    for EachIndex(i, input->ts_arr.count) {
      LNK_TypeServer *ts = &input->ts_arr.v[i];
      if (ts->rrt == 0) { continue; }
      if (largest_ts == 0 || (largest_ts->rrt->type_data_raw.size < ts->rrt->type_data_raw.size)) {
        largest_ts = ts;
      }
    }

    if (largest_ts) {
      U32                obj_idx = safe_cast_u32(input->ts_obj_range.min + largest_ts->ts_idx);
      Rng1U64           *ranges  = tp_divide_work(scratch.arena, input->debug_t_arr[obj_idx].count, tp->worker_count);
      LNK_LeafDedupWork *work    = push_array(scratch.arena, LNK_LeafDedupWork, tp->worker_count);
      for EachIndex(i, tp->worker_count) {
        work[i] = (LNK_LeafDedupWork){ obj_idx, ranges[i].min, ranges[i].max };
      }
      lnk_dedup_leaf_work(tp, &task, tp->worker_count, work, 1);

      U32Array new_dedup_type_server_indices = { .v = push_array(scratch.arena, U32, input->type_server_indices.count) };
      for EachIndex(i, input->type_server_indices.count) {
        if (input->type_server_indices.v[i] == obj_idx) { continue; }
        new_dedup_type_server_indices.v[new_dedup_type_server_indices.count++] = input->type_server_indices.v[i];
      }
      dedup_type_server_indices = new_dedup_type_server_indices;
    }
  }

  ProfBegin("Leaf Dedup");
  ProfScope(".debug$P")     lnk_dedup_leaf_indices(tp, &task, input->debug_p_indices);
  ProfScope(".debug$T")     lnk_dedup_leaf_indices(tp, &task, input->int_obj_indices);
  ProfScope("Type Servers") lnk_dedup_leaf_indices(tp, &task, dedup_type_server_indices);
  ProfEnd();

  ProfBegin("Extract present buckets from the leaf hash tables");

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    task.ti_source          = ti_source;
    task.counts[ti_source]  = push_array(scratch.arena, U64, tp->worker_count);
    task.ranges             = tp_divide_work(scratch.arena, task.leaf_ht_arr[ti_source].cap, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_count_present_buckets_task, &task, "Count present buckets");

    task.unique_leaf_refs_arr[ti_source].count = sum_array_u64(tp->worker_count, task.counts[ti_source]);
    Assert(task.unique_leaf_refs_arr[ti_source].count == task.leaf_ht_arr[ti_source].count);
    task.unique_leaf_refs_arr[ti_source].v     = push_array_no_zero(arena, LNK_LeafRef, task.unique_leaf_refs_arr[ti_source].count);
    task.offsets[ti_source]                    = offsets_from_counts_array_u64(scratch.arena, task.counts[ti_source], tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_get_present_buckets_task, &task, "Copy present buckets");

    LNK_LeafHashTable *ht = &task.leaf_ht_arr[ti_source];
    if (ht->count) {
      lnk_log(LNK_Log_LinkStats, "Type dedup %S: %llu unique, %llu slots, %llu initial slots, %llu growths, %M backing",
              cv_string_from_type_index_source(ti_source), ht->count, ht->cap, ht->initial_cap, ht->grow_count, ht->cap * sizeof(LNK_LeafRef));
    }
    arena_release(ht->arena);
    MemoryZeroStruct(ht);

    // sort output leaves based on { location index, leaf index } to guarantee determinism
    ProfScope("Radix Sort") {
      u64_array_sort_radix_parallel(tp, task.unique_leaf_refs_arr[ti_source].count, task.unique_leaf_refs_arr[ti_source].v);
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
      task.ti_source                            = ti_source;
      task.assigned_ti_arr[ti_source].cap       = ((task.unique_leaf_refs_arr[ti_source].count * 13) / 10);
      task.assigned_ti_arr[ti_source].ti_arr    = push_array(arena, CV_TypeIndex, task.assigned_ti_arr[ti_source].cap);
      task.assigned_ti_arr[ti_source].min_ti    = CV_MinComplexTypeIndex;
      task.assigned_ti_arr[ti_source].leaf_refs = task.unique_leaf_refs_arr[ti_source].v;

      task.min_type_indices[ti_source] = CV_MinComplexTypeIndex;
      task.ranges = tp_divide_work(scratch.arena, task.unique_leaf_refs_arr[ti_source].count, tp->worker_count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_assign_type_indices_task, &task, "Assign Type Indices");
    }
    ProfEnd();

    MemoryCopyArray(task.result.assigned_ti_arr, task.assigned_ti_arr);
    MemoryCopyArray(task.result.leaf_refs, task.unique_leaf_refs_arr);
  }
  ProfEnd();

  // @type_server
  if (merge_flags & LNK_MergeTypeFlag_BuildObjTiMap) {
    task.obj_ti_map_counts = push_array(scratch.arena, U64, input->obj_count);
    for EachIndex(obj_idx, input->obj_count) {
      task.obj_ti_map_counts[obj_idx] = input->debug_t_arr[obj_idx].count;
    }

    U64 total_ti_count = sum_array_u64(input->obj_count, task.obj_ti_map_counts);
    task.obj_ti_map_offsets = offsets_from_counts_array_u64(scratch.arena, task.obj_ti_map_counts, input->obj_count);
    task.obj_ti_batch       = push_array_no_zero(arena, CV_TypeIndex, total_ti_count);
    task.result.obj_ti_maps = push_array_no_zero(arena, CV_TypeIndex *, input->obj_count);
      
    tp_for_parallel_prof(tp, 0, input->obj_count, lnk_build_obj_ti_map, &task, "Build TI Map");
  }

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    LNK_LeafRefArray unique_leaf_refs = task.unique_leaf_refs_arr[ti_source];

    task.result.count[ti_source] = unique_leaf_refs.count;

    if (merge_flags & LNK_MergeTypeFlag_ExportHashes) {
      task.ti_source = ti_source;
      task.ranges = tp_divide_work(scratch.arena, unique_leaf_refs.count, tp->worker_count);
      task.result.hashes[ti_source] = push_array_no_zero(arena, U64, unique_leaf_refs.count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_unbucket_hashes_task, &task, "Export Hashes");
    }
  }

  MemoryCopyTyped(task.result.min_type_indices, input->min_type_indices, CV_TypeIndexSource_COUNT);

  temp_end(scratch);
  arena_release(merge_arena);
  tp_release_scratch_free_blocks(tp);
  ProfEnd();
  return task.result;
}

internal void
lnk_hash_type_name_lenient(U8 *raw_leaf, U64 hash_length, Arena *map_arena, String8List *map)
{
  ProfBeginFunction();
  B32 make_map = map_arena != 0;

  U64  hash_max_chars = hash_length*2;
  char temp[128];

  {
    CV_Leaf leaf = cv_leaf_from_ptr(raw_leaf);
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
          str8_list_pushf(map_arena, map, "%llx %S\n", name_hash, udt_info.unique_name);
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
          CV_LeafHeader *header = (CV_LeafHeader *)raw_leaf;
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
          CV_LeafHeader *header = (CV_LeafHeader *)raw_leaf;
          Assert(new_size <= max_U16);
          memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);
        }
      }
    }
  }

  ProfEnd();
}

internal void
lnk_hash_type_name_full(U8 *raw_leaf, U64 hash_length, Arena *map_arena, String8List *map)
{
  ProfBeginFunction();
  B32 make_map = map_arena != 0;

  U64 hash_max_chars = hash_length*2;

  {
    CV_Leaf leaf = cv_leaf_from_ptr(raw_leaf);
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
        CV_LeafHeader *header   = (CV_LeafHeader *)raw_leaf;
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

internal LNK_DebugRelocs
lnk_debug_relocs_init(Arena *arena, LNK_Obj *obj, U64 section_number)
{
  COFF_RelocArray relocs = lnk_coff_reloc_info_from_section_number(obj, section_number);
  B32 sorted = 1;
  for (U64 i = 1; i < relocs.count; ++i) {
    if (relocs.v[i - 1].apply_off > relocs.v[i].apply_off) { sorted = 0; break; }
  }

  if (!sorted) {
    lnk_error_obj(LNK_Warning_UnsortedRelocations,
                  obj,
                  "relocations are not sorted in section %S (%llu); sorting by offset",
                  lnk_obj_section_name_from_section_number(obj, section_number),
                  section_number);

    // copy & sort the relocations
    COFF_Reloc *copy = push_array_no_zero(arena, COFF_Reloc, relocs.count);
    Temp temp = temp_begin(arena);
    U64 *keys = push_array_no_zero(temp.arena, U64, relocs.count);
    for EachIndex(i, relocs.count) { keys[i] = ((U64)relocs.v[i].apply_off << 32) | i; }
    radsort(keys, relocs.count, u64_is_before);
    for EachIndex(i, relocs.count) { copy[i] = relocs.v[(U32)keys[i]]; }
    temp_end(temp);

    relocs.v = copy;
  }

  LNK_DebugRelocs result = {
    .obj            = obj,
    .section_number = section_number,
    .section_data   = lnk_obj_section_data_from_number(obj, section_number),
    .first          = relocs.count ? relocs.v : 0,
    .cursor         = relocs.count ? relocs.v : 0,
    .opl            = relocs.count ? relocs.v + relocs.count : 0,
  };
  return result;
}

internal void
lnk_relocate_debug_copy(LNK_BuildPdb *task, LNK_DebugRelocs *relocs, String8 source, U8 *dst)
{
  if (relocs->cursor == relocs->opl) { return; }

  LNK_Obj     *obj        = relocs->obj;
  U64          image_base = lnk_get_base_addr(task->cv->config);
  U64          source_off = source.str - relocs->section_data.str;
  COFF_Reloc **cursor     = &relocs->cursor;
  COFF_Reloc  *opl        = relocs->opl;

  while (*cursor < opl && (*cursor)->apply_off < source_off) { ++*cursor; }

  for (; *cursor < opl && (*cursor)->apply_off < source_off + source.size; ++*cursor) {
    COFF_Reloc *reloc = *cursor;

    COFF_RelocValue value;
    if (!lnk_resolve_reloc_value(obj, relocs->section_number, reloc, reloc - relocs->first, image_base, task->image_section_table, task->image_section_table_count, &value)) { continue; }

    if (value.size == 0) { continue; }

    U64 off = reloc->apply_off - source_off;
    if (value.size > source.size - off) {
      lnk_error_obj(LNK_Error_IllegalRelocation, obj, "debug relocation at 0x%x crosses a record boundary", reloc->apply_off);
      continue;
    }

    lnk_apply_reloc_value(dst + off, value);
  }
}

internal U64
lnk_write_pdb_type(LNK_WritePdbTypes *ctx, U64 leaf_idx, String8 source, String8 dst, Arena *map_arena, String8List *map)
{
  LNK_Config *config     = ctx->types->input->config;
  B32         hash_names = ctx->source == CV_TypeIndexSource_TPI &&
                           config->pdb_hash_type_names != LNK_TypeNameHashMode_None &&
                           config->pdb_hash_type_names != LNK_TypeNameHashMode_Null;

  if (dst.str == 0 && !hash_names) { return source.size; }

  Temp scratch = scratch_begin(&map_arena, 1);
  String8 data = source;

  if (hash_names) {
    data = push_str8_copy(scratch.arena, source);

    U64 hash_length = Clamp(1, config->pdb_hash_type_name_length, LNK_LEAF_HT_MIN_CAP);
    if (config->pdb_hash_type_names == LNK_TypeNameHashMode_Lenient) {
      lnk_hash_type_name_lenient(data.str, hash_length, map_arena, map);
    } else {
      lnk_hash_type_name_full(data.str, hash_length, map_arena, map);
    }

    data = cv_raw_leaf_from_ptr(data.str);
  }

  if (dst.str) {
    AssertAlways(data.size <= dst.size);
    MemoryCopy(dst.str, data.str, data.size);
    lnk_patch_type_copy(scratch.arena, ctx->types, ctx->source, leaf_idx, dst.str);
  }

  U64 size = data.size;

  scratch_end(scratch);
  return size;
}

internal
THREAD_POOL_TASK_FUNC(lnk_measure_pdb_types_task)
{
  LNK_WritePdbTypes *task = raw_task;
  for EachInRange(i, task->ranges[task_id]) {
    Temp scratch = scratch_begin(0, 0);
    String8List map = {0};
    String8 source = lnk_raw_type_from_index(task->types, task->source, i);
    task->offsets[task_id] += lnk_write_pdb_type(task, i, source, str8_zero(), task->write_name_map ? scratch.arena : 0, &map);
    if (task->write_name_map) { task->map_offsets[task_id] += map.total_size; }
    scratch_end(scratch);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_write_pdb_types_task)
{
  LNK_WritePdbTypes *task = raw_task;

  U64 offset     = task->offsets[task_id];
  U64 map_cursor = task->write_name_map ? task->map_offsets[task_id] : 0;

  for EachInRange(i, task->ranges[task_id]) {
    Temp scratch = scratch_begin(0, 0);

    String8List map     = {0};
    String8     dst     = str8_substr(task->output, r1u64(offset, task->offsets[task_id + 1]));
    String8     source  = lnk_raw_type_from_index(task->types, task->source, i);
    U64         written = lnk_write_pdb_type(task, i, source, dst, task->write_name_map ? scratch.arena : 0, &map);
    CV_Leaf     leaf    = cv_leaf_from_ptr(dst.str);
    U32         hash    = 0;

    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo udt = cv_get_udt_info(leaf.kind, leaf.data);
      if (!(udt.props & CV_TypeProp_FwdRef)) {
        hash = pdb_hash_udt(udt, leaf.data) % task->hash_bucket_count;
      }
    }
    task->hashes[i] = hash;

    if (task->write_name_map) {
      Assert(map_cursor <= task->map_offsets[task_id + 1] && map.total_size <= task->map_offsets[task_id + 1] - map_cursor);
      for EachNode(n, String8Node, map.first) {
        MemoryCopy(task->map_data.str + map_cursor, n->string.str, n->string.size);
        map_cursor += n->string.size;
      }
    }

    if (i % PDB_TYPE_HINT_STEP == 0) {
      Assert(offset < PDB_TYPE_OFFSET_MAX);
      task->hints[i / PDB_TYPE_HINT_STEP] = (PDB_TpiOffHint){
        .itype = task->types->min_type_indices[task->source] + i,
        .off   = (PDB_TypeOffset)offset,
      };
    }
    offset += written;

    scratch_end(scratch);
  }
  Assert(offset == task->offsets[task_id + 1]);

  if (task->write_name_map) {
    Assert(map_cursor == task->map_offsets[task_id + 1]);
  }
}

internal void
lnk_build_pdb_type_stream(Arena *arena, TP_Context *tp, PDB_Context *pdb, LNK_MergedTypes *types, CV_TypeIndexSource source, U64 leaf_count, String8 *name_map_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  PDB_TypeServer   *ts         = pdb->type_servers[source];
  MSF_StreamNumber  sn         = source == CV_TypeIndexSource_TPI ? PDB_FixedStream_Tpi : PDB_FixedStream_Ipi;
  U64               hint_count = CeilIntegerDiv(leaf_count, PDB_TYPE_HINT_STEP);
  LNK_WritePdbTypes task = {
    .types             = types,
    .source            = source,
    .write_name_map    = name_map_out != 0,
    .map_offsets       = name_map_out ? push_array(scratch.arena, U64, tp->worker_count + 1) : 0,
    .ranges            = tp_divide_work(scratch.arena, leaf_count, tp->worker_count),
    .offsets           = push_array(scratch.arena, U64, tp->worker_count + 1),
    .hash_bucket_count = ts->bucket_cap,
  };

  // first measure the final records, including any shortened type names
  tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_measure_pdb_types_task, &task, "Measure PDB Types");
  u64_array_counts_to_offsets(tp->worker_count + 1, task.offsets);
  U64 data_size = task.offsets[tp->worker_count];
  if (task.write_name_map) {
    u64_array_counts_to_offsets(tp->worker_count + 1, task.map_offsets);
    U64 map_size = task.map_offsets[tp->worker_count];
    task.map_data = str8(push_array_no_zero(arena, U8, map_size), map_size);
    *name_map_out = task.map_data;
  }

  // allocate once and write into disjoint ranges of the final TPI/IPI stream
  AssertAlways(msf_stream_resize(pdb->msf, sn, safe_cast_u32(sizeof(PDB_TpiHeader) + data_size)));
  task.output = str8_skip(msf_stream_data(pdb->msf, sn), sizeof(PDB_TpiHeader));

  PDB_TypeHashStreamInfo hash_info = pdb_type_hash_stream_prepare(ts, &pdb->info->strtab, pdb->msf, leaf_count, hint_count);
  String8 hash_data = msf_stream_data(pdb->msf, ts->hash_sn);
  task.hashes = (U32 *)hash_data.str;
  task.hints  = hash_data.str ? (PDB_TpiOffHint *)(hash_data.str + hash_info.ti_offs.off) : 0;
  tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_write_pdb_types_task, &task, "Write PDB Types");

  // hash values and hints were written alongside their finalized records
  pdb_type_server_write_header(ts, pdb->msf, sn, leaf_count, data_size, hash_info);

  scratch_end(scratch);
  ProfEnd();
}

internal B32
lnk_global_symbol_is_kept(CV_Symbol symbol, U64 *depth)
{
  B32 keep = cv_is_global_symbol(symbol.kind) || (*depth == 0 && cv_is_typedef(symbol.kind));
  CV_SymKind kind = cv_final_symbol_kind(symbol.kind);
  if      (cv_is_scope_symbol(kind)) { *depth += 1;                      }
  else if (cv_is_end_symbol(kind))   { Assert(*depth != 0); *depth -= 1; }
  return keep;
}

internal U64
lnk_count_global_symbols(LNK_CodeViewInput *cv, U32Array objs)
{
  U64 count = 0;
  for EachIndex(i, objs.count) {
    U64 depth = 0;
    CV_DebugS_Iter it = cv_debug_s_iter_init(&cv->debug_s_arr[objs.v[i]]);
    while (cv_debug_s_iter_next_kind(&it, CV_C13SubSectionKind_Symbols)) {
      for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= it.data.size; ) {
        CV_Symbol symbol = {0};
        TryReadBreak(cv_read_symbol(it.data, cursor, CV_SymbolAlign, &symbol), cursor);

        if (lnk_global_symbol_is_kept(symbol, &depth)) {
          count += 1;
        }
      }
    }
  }
  return count;
}

internal B32
lnk_insert_global_symbol(void **buckets, U64 cap, String8 raw)
{
  U64        first  = u64_hash_from_str8(raw) % cap;
  U64        idx    = first;
  CV_Symbol  symbol = cv_symbol_from_ptr(raw.str);

  for (U64 probes = 0; probes < cap; ) {
    void *curr = ins_atomic_ptr_eval_cond_assign(&buckets[idx], 0, 0);
    if (curr == 0) {
      // insert only the patched symbol; inserted records are never replaced,
      // so the caller may rollback its candidate only when insertion returns false
      if (ins_atomic_ptr_eval_cond_assign(&buckets[idx], raw.str, 0) != 0) { continue; }
      return 1;
    }

    // symbol is already in the hash table -> stop
    if (cv_symbol_match(cv_symbol_from_ptr(curr), symbol)) { return 0; }

    idx = (idx + 1) == cap ? 0 : idx + 1;
    probes += 1;
  }

  // table is sized incorrectly when hit
  InvalidPath;
  return 0;
}

internal B32
lnk_is_pdb_module_symbol(CV_Symbol *symbol, U64 *scope_depth)
{
  symbol->kind = cv_final_symbol_kind(symbol->kind);

  if (symbol->kind == CV_SymKind_SKIP                    ||
      cv_is_global_symbol(symbol->kind)                  ||
      (cv_is_typedef(symbol->kind) && *scope_depth == 0) ||
      symbol->kind == 0x1176) {
    return 0;
  }

  if      (cv_is_scope_symbol(symbol->kind)) { *scope_depth += 1; }
  else if (cv_is_end_symbol(symbol->kind))   { *scope_depth -= 1; }

  return 1;
}

internal
THREAD_POOL_TASK_FUNC(lnk_move_global_symbols_to_gsi)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildPdb   *task        = raw_task;
  PDB_GsiContext *gsi         = task->pdb->gsi;
  PDB_PsiContext *psi         = task->pdb->psi;
  U32Array        obj_indices = task->obj_indices[task_id];

  if (!task->cv->is_stripped) ProfScope("Global Symbols")
  {
    U64    global_count = tp_sum_u64(tp, task_id, lnk_count_global_symbols(task->cv, obj_indices));
    U64    bucket_cap   = Max(1, global_count * 13 / 10);
    void **buckets      = 0;

    if (task_id == 0) {
      buckets = push_array(scratch.arena, void *, bucket_cap);
    }
    tp_broadcast(&buckets);

    for EachIndex(i, obj_indices.count) {
      U32            obj_idx = obj_indices.v[i];
      U64            depth   = 0;
      CV_DebugS_Iter it = cv_debug_s_iter_init(&task->cv->debug_s_arr[obj_idx]);
      for (U32Node *section = task->cv->debug_s_section_numbers[obj_idx].first; cv_debug_s_iter_next_block(&it); section = section->next) {
        Assert(section != 0);
        Temp section_temp = scratch_begin(&scratch.arena, 1);
        
        LNK_DebugRelocs relocs = lnk_debug_relocs_init(section_temp.arena, task->cv->obj_arr[obj_idx], section->data);
        while (cv_debug_s_iter_next(&it)) {
          if (it.kind != CV_C13SubSectionKind_Symbols) { continue; }

          for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= it.data.size; ) {
            CV_Symbol symbol = {0};
            TryReadBreak(cv_read_symbol(it.data, cursor, CV_SymbolAlign, &symbol), cursor);

            if (lnk_global_symbol_is_kept(symbol, &depth)) {
              Temp candidate_temp = temp_begin(scratch.arena);

              String8 symbol_read = cv_raw_from_symbol(cv_ptr_from_symbol(symbol));
              String8 symbol_copy = push_str8_copy(scratch.arena, symbol_read);

              lnk_relocate_debug_copy(task, &relocs, symbol_read, symbol_copy.str);
              lnk_patch_symbol_copy(task->types, obj_idx, cv_symbol_from_ptr(symbol_copy.str));

              if (!lnk_insert_global_symbol(buckets, bucket_cap, symbol_copy)) {
                temp_end(candidate_temp);
              }
            }
          }
        }

        scratch_end(section_temp);
      }
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
      CV_SymbolNode *nodes = push_array_no_zero(scratch.arena, CV_SymbolNode, symbol_count);
      for EachIndex(i, symbol_count) {
        CV_SymbolNode *n = &nodes[i];
        n->prev        = n->next = 0;
        n->data        = cv_symbol_from_ptr(symbol_arr[i]);
        n->data.offset = i;
        gsi_push_(gsi, symbol_hashes[i], n);
      }
    }
  }

  ProfScope("Proc Refs")
  {
    U64 *proc_ref_counts = 0;
    if (task_id == 0) {
      proc_ref_counts = push_array(scratch.arena, U64, tp->worker_count);
    }
    tp_broadcast(&proc_ref_counts);

    U64 proc_ref_count = 0;
    for EachIndex(i, obj_indices.count) {
      U64     obj_idx = obj_indices.v[i];
      String8 symbols = task->mod_views[obj_idx].symbols;
      for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.size; ) {
        CV_Symbol symbol = {0};
        TryReadBreak(cv_read_symbol(symbols, cursor, PDB_SYMBOL_ALIGN, &symbol), cursor);
        if (symbol.kind == CV_SymKind_GPROC32 || symbol.kind == CV_SymKind_LPROC32) {
          proc_ref_count += 1;
        }
      }
    }
    proc_ref_counts[task_id] = proc_ref_count;
    barrier_wait(tp->barrier);

    U64             total_proc_ref_count = tp_sum_u64(tp, task_id, proc_ref_count);
    U32            *proc_ref_hashes      = 0;
    U64            *proc_ref_indices     = 0;
    CV_SymbolNode  *proc_ref_nodes       = 0;
    if (task_id == 0) {
      proc_ref_hashes  = push_array(scratch.arena, U32, total_proc_ref_count);
      proc_ref_indices = offsets_from_counts_array_u64(scratch.arena, proc_ref_counts, tp->worker_count);
      proc_ref_nodes   = push_array(scratch.arena, CV_SymbolNode, total_proc_ref_count);
    }
    tp_broadcast(&proc_ref_hashes);
    tp_broadcast(&proc_ref_indices);
    tp_broadcast(&proc_ref_nodes);

    U64 proc_ref_idx = proc_ref_indices[task_id];
    for EachIndex(i, obj_indices.count) {
      U64         obj_idx = obj_indices.v[i];
      String8     symbols = task->mod_views[obj_idx].symbols;
      CV_ModIndex imod    = task->mod_arr[obj_idx]->imod;
      for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.size; ) {
        CV_Symbol symbol = {0};
        U64 symbol_offset = sizeof(CV_Signature) + cursor;
        TryReadBreak(cv_read_symbol(symbols, cursor, PDB_SYMBOL_ALIGN, &symbol), cursor);

        if (symbol.kind == CV_SymKind_GPROC32 || symbol.kind == CV_SymKind_LPROC32) {
          String8 name = cv_name_from_symbol(symbol.kind, symbol.data);
          proc_ref_nodes [proc_ref_idx].data = cv_make_proc_ref(scratch.arena, imod, safe_cast_u32(symbol_offset), name, cv_is_lproc(symbol));
          proc_ref_nodes [proc_ref_idx].data.offset = symbol_offset;
          proc_ref_hashes[proc_ref_idx] = gsi_hash(gsi, name);
          proc_ref_idx += 1;
        }
      }
    }
    barrier_wait(tp->barrier);

    Assert(proc_ref_idx == proc_ref_indices[task_id] + proc_ref_counts[task_id]);

    // push proc refs
    if (task_id == 0) {
      U64 total_proc_ref_count = sum_array_u64(tp->worker_count, proc_ref_counts);
      for EachIndex(i, total_proc_ref_count) { gsi_push_(gsi, proc_ref_hashes[i], &proc_ref_nodes[i]); }
    }
    barrier_wait(tp->barrier);
  }

  ProfScope("Public Symbols")
  {
    LNK_SymbolHashTrieChunkList   symbol_chunks        = task->symtab->chunks[task_id];
    CV_SymbolList                *public_symbols       = 0; // [worker_count]
    U32                         **public_symbol_hashes = 0; // [worker_count][public_symbol.count]
    if (task_id == 0) {
      public_symbols       = push_array(scratch.arena, CV_SymbolList, tp->worker_count);
      public_symbol_hashes = push_array(scratch.arena, U32 *,         tp->worker_count);
    }
    tp_broadcast(&public_symbols);
    tp_broadcast(&public_symbol_hashes);

    CV_SymbolList *public_symbol_list = &public_symbols[task_id];
    for EachNode(chunk, LNK_SymbolHashTrieChunk, symbol_chunks.first) {
      for EachIndex(i, chunk->count) {
        LNK_Symbol        *symbol        = chunk->v[i].symbol;
        LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
        COFF_ParsedSymbol  symbol_parsed = lnk_parsed_from_symbol(symbol);

        // discard removed and non-section symbols
        if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }
        COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
        if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

        // make CV public symbols
        CV_Pub32Flags flags      = COFF_SymbolType_IsFunc(symbol_parsed.type) ? CV_Pub32Flag_Function : 0;
        ISectOff      sc         = lnk_sc_from_symbol(symbol);
        CV_Symbol     pub_symbol = cv_make_pub32(scratch.arena, flags, safe_cast_u32(sc.off), safe_cast_u16(sc.isect), symbol->name);
        cv_symbol_list_push(scratch.arena, public_symbol_list, pub_symbol);
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

  {
    Temp serial_scratch = scratch_begin(&scratch.arena, 1);

    PDB_Context                  *pdb        = task->pdb;
    PDB_GsiContext               *contexts[] = { pdb->psi->gsi, pdb->gsi };
    PDB_GsiSerializeSymbolsTask  *serial     = 0;
    PDB_GsiHashRecord           **hashes     = 0;
    U32                          *addr_map   = 0;
    if (task_id == 0) {
      serial = push_array(serial_scratch.arena, PDB_GsiSerializeSymbolsTask, 2);
      hashes = push_array(serial_scratch.arena, PDB_GsiHashRecord *, 2);
      for EachElement(c, contexts) {
        serial[c].symbol_align    = contexts[c]->symbol_align;
        serial[c].bucket_arr      = contexts[c]->bucket_arr;
        serial[c].bucket_size_arr = push_array(serial_scratch.arena, U64, contexts[c]->bucket_count);
      }
    }
    tp_broadcast(&serial);
    tp_broadcast(&hashes);

    for EachElement(c, contexts) {
      for (U64 b = task_id; b < contexts[c]->bucket_count; b += tp->worker_count) {
        gsi_size_buckets_task(scratch.arena, worker_id, b, &serial[c], tp);
      }
    }
    barrier_wait(tp->barrier);

    if (task_id == 0) {
      U64 total_size = 0;
      for EachElement(c, contexts) {
        total_size += sum_array_u64(contexts[c]->bucket_count, serial[c].bucket_size_arr);
      }

      if (total_size) {
        PDB_DbiContext *dbi = pdb->dbi;
        if (dbi->symbols_sn == MSF_INVALID_STREAM_NUMBER) {
          dbi->symbols_sn = msf_stream_alloc(pdb->msf);
        }
        AssertAlways(msf_stream_resize(pdb->msf, dbi->symbols_sn, safe_cast_u32(total_size)));

        U8 *symbol_data = msf_stream_data(pdb->msf, dbi->symbols_sn).str;
        U64 base        = 0;
        for EachElement(c, contexts) {
          PDB_GsiContext *gsi = contexts[c];
          if (!gsi->symbol_count) { continue; }

          PDB_GsiSerializeSymbolsTask *s = &serial[c];
          s->symbol_data_base    = base;
          s->buffer              = symbol_data + base;
          s->bucket_off_arr      = offsets_from_counts_array_u64(serial_scratch.arena, s->bucket_size_arr, gsi->bucket_count);
          s->sort_record_arr     = push_array_no_zero(serial_scratch.arena, PDB_GsiSortRecord, gsi->symbol_count);
          s->sort_record_arr_arr = push_array_no_zero(serial_scratch.arena, PDB_GsiSortRecord *, gsi->bucket_count);

          U64 occupied = 0, record_idx = 0;
          for EachIndex(b, gsi->bucket_count) {
            s->sort_record_arr_arr[b] = s->sort_record_arr + record_idx;
            record_idx += gsi->bucket_arr[b].count;
            occupied   += gsi->bucket_arr[b].count != 0;
          }

          U64 bitmap_count = gsi->bucket_count / gsi->word_size + 1;
          PDB_GsiHeader header = {
            .signature            = PDB_GsiSignature_Basic,
            .version              = PDB_GsiVersion_V70,
            .hash_record_arr_size = sizeof(PDB_GsiHashRecord) * gsi->symbol_count,
            .bucket_data_size     = sizeof(U32) * (bitmap_count + occupied)
          };
          U64 hash_size = sizeof(header) + header.hash_record_arr_size + header.bucket_data_size;
          U64 prefix    = c == 0 ? sizeof(PDB_PsiHeader) : 0;
          U64 suffix    = c == 0 ? sizeof(U32) * gsi->symbol_count : 0;

          MSF_StreamNumber *sn = c == 0 ? &dbi->publics_sn : &dbi->globals_sn;
          if (*sn == MSF_INVALID_STREAM_NUMBER) { *sn = msf_stream_alloc(pdb->msf); }
          AssertAlways(msf_stream_resize(pdb->msf, *sn, safe_cast_u32(prefix + hash_size + suffix)));

          U8 *dst = msf_stream_data(pdb->msf, *sn).str;
          if (c == 0) {
            PDB_PsiHeader psi_header = { .sym_hash_size = hash_size, .addr_map_size = suffix };
            MemoryCopy(dst, &psi_header, sizeof(psi_header));
            addr_map = (U32 *)(dst + prefix + hash_size);
          }

          MemoryCopy(dst + prefix, &header, sizeof(header));
          hashes[c] = (PDB_GsiHashRecord *)(dst + prefix + sizeof(header));

          U32 *bitmap     = (U32 *)((U8 *)hashes[c] + header.hash_record_arr_size);
          U32 *compressed = bitmap + bitmap_count;
          MemoryZeroTyped(bitmap, bitmap_count);

          U64 hash_idx = 0, compressed_idx = 0;
          for EachIndex(b, gsi->bucket_count) {
            U64 n = gsi->bucket_arr[b].count;
            if (n) {
              bitmap[b / gsi->word_size] |= 1u << (b % gsi->word_size);
              compressed[compressed_idx++] = hash_idx * sizeof(PDB_GsiHashRecordOffsetCalc);
            }
            hash_idx += n;
          }
          base += sum_array_u64(gsi->bucket_count, s->bucket_size_arr);
        }
        Assert(base == total_size);
        msf_stream_seek(pdb->msf, dbi->symbols_sn, total_size);
      }
    }
    tp_broadcast(&addr_map);

    for EachElement(c, contexts) {
      PDB_GsiContext *gsi = contexts[c];
      for (U64 b = task_id; b < gsi->bucket_count; b += tp->worker_count) {
        U64 count = gsi->bucket_arr[b].count;
        if (!count) { continue; }

        if (c == 0) {
          gsi_serialize_pub32(scratch.arena, worker_id, b, &serial[c], tp);
        } else {
          gsi_serialize_symbols_task(scratch.arena, worker_id, b, &serial[c], tp);
        }

        PDB_GsiSortRecord *records = serial[c].sort_record_arr_arr[b];
        U64                first   = records - serial[c].sort_record_arr;
        for EachIndex(i, count) {
          hashes[c][first + i] = (PDB_GsiHashRecord){ .symbol_off = records[i].offset + 1, .cref = 1 };
        }
      }
    }
    barrier_wait(tp->barrier);

    if (contexts[0]->symbol_count) {
      PDB_GsiSortRecord *records = serial[0].sort_record_arr;
      U64                count   = contexts[0]->symbol_count;
      if (count < PDB_PSI_ADDR_MAP_RADIX_MIN_COUNT) {
        if (task_id == 0) {
          gsi_record_sort_by_sc(records, count);
        }
        barrier_wait(tp->barrier);

        for (U64 i = task_id; i < count; i += tp->worker_count) {
          addr_map[i] = safe_cast_u32(records[i].offset);
        }
      } else {
        PDB_PsiAddrMapSortTask *sort = 0;
        if (task_id == 0) {
          sort                 = push_array(serial_scratch.arena, PDB_PsiAddrMapSortTask, 1);
          sort->ranges         = tp_divide_work(serial_scratch.arena, count, tp->worker_count);
          sort->gsi_record_arr = records;
          sort->src            = push_array_no_zero(serial_scratch.arena, PDB_PsiAddrMapSortRecord, count);
          sort->dst            = push_array_no_zero(serial_scratch.arena, PDB_PsiAddrMapSortRecord, count);
          sort->counts         = push_array_no_zero(serial_scratch.arena, U64, tp->worker_count * PDB_PSI_ADDR_MAP_RADIX_COUNT);
        }
        tp_broadcast(&sort);

        psi_addr_map_radix_init_task(serial_scratch.arena, worker_id, task_id, sort, tp);
        barrier_wait(tp->barrier);

        U32 shifts[] = { 0, 11, 22, 32, 43 };
        U32 counts[] = { 1 << 11, 1 << 11, 1 << 10, 1 << 11, 1 << 5 };
        for EachElement(pass, shifts) {
          if (task_id == 0) { sort->digit_shift = shifts[pass]; sort->digit_count = counts[pass]; }
          barrier_wait(tp->barrier);

          psi_addr_map_radix_histogram_task(serial_scratch.arena, worker_id, task_id, sort, tp);
          barrier_wait(tp->barrier);

          if (task_id == 0) {
            U64 cursor = 0;
            for EachIndex(digit, sort->digit_count) {
              for EachIndex(lane, tp->worker_count) {
                U64 *n = &sort->counts[lane * PDB_PSI_ADDR_MAP_RADIX_COUNT + digit];
                U64 size = *n;
                *n = cursor;
                cursor += size;
              }
            }
            Assert(cursor == count);
          }
          barrier_wait(tp->barrier);

          psi_addr_map_radix_sort_task(serial_scratch.arena, worker_id, task_id, sort, tp);
          barrier_wait(tp->barrier);

          if (task_id == 0) { Swap(PDB_PsiAddrMapSortRecord *, sort->src, sort->dst); }
          barrier_wait(tp->barrier);
        }

        Rng1U64 *runs      = 0;
        U64      run_count = 0;
        U64     *next_run  = 0;
        if (task_id == 0) {
          for (U64 first = 0; first < count;) {
            U64 opl = first + 1;
            while (opl < count && sort->src[opl].key == sort->src[first].key) { ++opl; }
            run_count += opl - first > 1;
            first = opl;
          }

          runs = push_array_no_zero(serial_scratch.arena, Rng1U64, run_count);
          U64 run_idx = 0;
          for (U64 first = 0; first < count;) {
            U64 opl = first + 1;
            while (opl < count && sort->src[opl].key == sort->src[first].key) { ++opl; }
            if (opl - first > 1) { runs[run_idx++] = r1u64(first, opl); }
            first = opl;
          }
          next_run = push_array(serial_scratch.arena, U64, 1);
        }
        tp_broadcast(&runs);
        tp_broadcast(&run_count);
        tp_broadcast(&next_run);

        for (;;) {
          U64 i = ins_atomic_u64_inc_eval(next_run) - 1;
          if (i >= run_count) { break; }
          radsort(sort->src + runs[i].min, dim_1u64(runs[i]), psi_addr_map_name_compar_is_before);
        }
        barrier_wait(tp->barrier);

        for EachInRange(i, sort->ranges[task_id]) {
          addr_map[i] = safe_cast_u32(sort->src[i].record->offset);
        }
      }
      barrier_wait(tp->barrier);
    }
    barrier_wait(tp->barrier);

    scratch_end(serial_scratch);
  }
  scratch_end(scratch);
}

internal U64
lnk_write_debug_payload(LNK_BuildPdb *task, U32 obj_idx, LNK_DebugRelocs *relocs, CV_DebugS_Iter *it, String8 module, U64 offset)
{
  Assert(module.str != 0);

  U64 size = str8_write(module, offset, it->data);

  if (size) {
    U8 *dst = module.str + offset;
    lnk_relocate_debug_copy(task, relocs, it->data, dst);

    if (it->kind == CV_C13SubSectionKind_InlineeLines) {
      Temp scratch = scratch_begin(0, 0);
      String8              data    = str8(dst, size);
      CV_TypeIndexInfoList indices = cv_get_inlinee_type_index_offsets(scratch.arena, data);
      lnk_fixup_cv_type_indices(task->types, obj_idx, data, indices);
      scratch_end(scratch);
    }
  }

  return size;
}

global read_only CV_C13SubSectionKind g_lnk_pdb_module_c13_layout[] =
{
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

internal U64
lnk_layout_debug_s_for_pdb_module(CV_DebugS debug_s, U64 symbol_size, Rng1U64 *ss_ranges)
{
  MemoryZeroTyped(ss_ranges, CV_C13SubSectionIdxKind_COUNT);
  CV_DebugS_Stats stats = cv_debug_s_stats(&debug_s);

  ss_ranges[CV_C13SubSectionIdxKind_Symbols] = r1u64(symbol_size ? sizeof(CV_Signature) : 0, symbol_size);

  U64 cursor = symbol_size;

  for EachElement(i, g_lnk_pdb_module_c13_layout) {
    CV_C13SubSectionKind kind = g_lnk_pdb_module_c13_layout[i];
    U64 idx = cv_c13_sub_section_idx_from_kind(kind);
    if (stats.size[idx] == 0) { continue; }
    ss_ranges[idx] = r1u64s(cursor + sizeof(CV_C13SubSectionHeader), stats.size[idx]);
    cursor         = AlignPow2(ss_ranges[idx].max, CV_C13SubSectionAlign);
  }

  // lines have individual headers; their destination cursor includes those
  // headers and padding; other C13 cursors cover only the combined payload
  ss_ranges[CV_C13SubSectionIdxKind_Lines].min = cursor;
  cursor += stats.lines_size;
  ss_ranges[CV_C13SubSectionIdxKind_Lines].max = cursor;

  if (symbol_size) {
    ss_ranges[CV_C13SubSectionIdxKind_GlobalRefs] = r1u64s(cursor + sizeof(U32), stats.size[CV_C13SubSectionIdxKind_GlobalRefs]);
    cursor = ss_ranges[CV_C13SubSectionIdxKind_GlobalRefs].max;
  }

  return cursor;
}

internal U64
lnk_size_debug_s_for_pdb_module(PDB_DbiModule *mod, CV_DebugS debug_s)
{
  mod->sym_data_size = 0;
  mod->c11_data_size = 0;

  CV_DebugS_Iter it = cv_debug_s_iter_init(&debug_s);
  U64 scope_depth = 0;
  while (cv_debug_s_iter_next_kind(&it, CV_C13SubSectionKind_Symbols)) {
    if (it.data.size && mod->sym_data_size == 0) { mod->sym_data_size = sizeof(CV_Signature); }
    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= it.data.size; ) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(it.data, cursor, CV_SymbolAlign, &symbol), cursor);
      if (lnk_is_pdb_module_symbol(&symbol, &scope_depth)) {
        mod->sym_data_size += cv_size_from_symbol(&symbol, PDB_SYMBOL_ALIGN);
      }
    }
  }

  Rng1U64 ranges_c13[CV_C13SubSectionIdxKind_COUNT];
  U64     size_c13 = lnk_layout_debug_s_for_pdb_module(debug_s, mod->sym_data_size, ranges_c13);
  
  mod->c13_data_size = ranges_c13[CV_C13SubSectionIdxKind_Lines].max - mod->sym_data_size;
  mod->globrefs_size = size_c13 - ranges_c13[CV_C13SubSectionIdxKind_Lines].max;
  
  return size_c13;
}

internal U64
lnk_write_debug_s_to_pdb_module(LNK_BuildPdb *task, U32 obj_idx, String8 module)
{
  PDB_DbiModule *mod    = task->mod_arr[obj_idx];
  CV_DebugS     debug_s = task->cv->debug_s_arr[obj_idx];

  Rng1U64 ss_ranges[CV_C13SubSectionIdxKind_COUNT];
  U64     module_size = lnk_layout_debug_s_for_pdb_module(debug_s, mod->sym_data_size, ss_ranges);
  AssertAlways(module_size == module.size);
  Assert(module_size == mod->sym_data_size + mod->c11_data_size + mod->c13_data_size + mod->globrefs_size);

  if (mod->sym_data_size) {
    CV_Signature signature = CV_Signature_C13;
    str8_write(module, 0, str8_struct(&signature));

    U64 idx  = CV_C13SubSectionIdxKind_GlobalRefs;
    U32 size = safe_cast_u32(dim_1u64(ss_ranges[idx]));
    str8_write(module, ss_ranges[idx].min - sizeof(size), str8_struct(&size));
  }

  // module storage is zero-initialized, including C13 alignment padding
  for EachElement(i, g_lnk_pdb_module_c13_layout) {
    CV_C13SubSectionKind kind = g_lnk_pdb_module_c13_layout[i];
    U64                  idx  = cv_c13_sub_section_idx_from_kind(kind);
    if (dim_1u64(ss_ranges[idx]) == 0) { continue; }

    CV_C13SubSectionHeader header = { .kind = kind, .size = safe_cast_u32(dim_1u64(ss_ranges[idx])) };
    str8_write(module, ss_ranges[idx].min - sizeof(header), str8_struct(&header));
  }

  if (task->cv->is_stripped) {
    // stripped stubs are already relocated, type-free, and PDB-aligned
    U64 *offset = &ss_ranges[CV_C13SubSectionIdxKind_Symbols].min;
    CV_DebugS_Iter it = cv_debug_s_iter_init(&debug_s);
    while (cv_debug_s_iter_next_kind(&it, CV_C13SubSectionKind_Symbols)) {
      *offset += str8_write(module, *offset, it.data);
    }
  } else {
    Temp scratch = scratch_begin(0, 0);

    CV_DebugS_Iter it = cv_debug_s_iter_init(&debug_s);
    U64 scope_depth = 0;
    for (U32Node *section = task->cv->debug_s_section_numbers[obj_idx].first;
         cv_debug_s_iter_next_block(&it);
         section = section->next) {
      Assert(section != 0);
      Temp section_temp = temp_begin(scratch.arena);

      LNK_DebugRelocs relocs = lnk_debug_relocs_init(section_temp.arena, task->cv->obj_arr[obj_idx], section->data);

      while (cv_debug_s_iter_next(&it)) {
        U64  kind   = cv_c13_sub_section_idx_from_kind(it.kind);
        U64 *offset = &ss_ranges[kind].min;

        if (it.kind == CV_C13SubSectionKind_Symbols) {
          for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= it.data.size; ) {
            CV_Symbol symbol = {0};
            TryReadBreak(cv_read_symbol(it.data, cursor, CV_SymbolAlign, &symbol), cursor);

            CV_SymKind source_kind = symbol.kind;
            if (lnk_is_pdb_module_symbol(&symbol, &scope_depth) == 0) { continue; }

            U64  size = cv_write_symbol(module.str, *offset, module.size, &symbol, PDB_SYMBOL_ALIGN);
            U8  *dst  = module.str + *offset;

            U8      *symbol_ptr  = cv_ptr_from_symbol(symbol);
            U64      symbol_size = sizeof(CV_SymbolHeader) + symbol.data.size;
            String8  symbol_raw  = str8(symbol_ptr, symbol_size);
            lnk_relocate_debug_copy(task, &relocs, symbol_raw, dst);

            CV_Symbol copy = cv_symbol_from_ptr(dst);
            copy.kind      = source_kind;

            lnk_patch_symbol_copy(task->types, obj_idx, copy);

            *offset += size;
          }
        } else if (it.data.size && *offset < ss_ranges[kind].max) {
          if (it.kind == CV_C13SubSectionKind_Lines) {
            CV_C13SubSectionHeader header = { .kind = it.kind, .size = safe_cast_u32(it.data.size) };
            *offset += str8_write(module, *offset, str8_struct(&header));
          }
          *offset += lnk_write_debug_payload(task, obj_idx, &relocs, &it, module, *offset);
          if (it.kind == CV_C13SubSectionKind_Lines) {
            *offset = AlignPow2(*offset, CV_C13SubSectionAlign);
          }
        }
      }
      temp_end(section_temp);
    }

    scratch_end(scratch);
  }

  for EachElement(i, ss_ranges) { Assert(ss_ranges[i].min == ss_ranges[i].max); }

  // patch code view tree offsets in symbols
  String8 symbols = str8_substr(module, r1u64(sizeof(CV_Signature), mod->sym_data_size));
  cv_patch_symbol_tree_offsets(symbols, sizeof(CV_Signature), PDB_SYMBOL_ALIGN);

  return module_size;
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
    lnk_size_debug_s_for_pdb_module(task->mod_arr[obj_idx], task->cv->debug_s_arr[obj_idx]);
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
    U64            obj_idx  = obj_indices.v[i];
    PDB_DbiModule *mod      = task->mod_arr[obj_idx];

    if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }

    String8 mod_data = msf_stream_data(task->pdb->msf, mod->sn);
    Assert(mod_data.size > 0);

    U64 written = lnk_write_debug_s_to_pdb_module(task, obj_idx, mod_data);
    Assert(written == mod_data.size);

  }
  barrier_wait(tp->barrier);

  {
    for EachIndex(i, obj_indices.count) {
      Temp temp = temp_begin(scratch.arena);

      U64            obj_idx = obj_indices.v[i];
      PDB_DbiModule *mod     = task->mod_arr[obj_idx];

      if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }

      String8     mod_data       = msf_stream_data(task->pdb->msf, mod->sn);
      Rng1U64     c13_data_range = r1u64(mod->sym_data_size + mod->c11_data_size, mod->sym_data_size + mod->c11_data_size + mod->c13_data_size);
      String8     c13_data       = str8_substr(mod_data, c13_data_range);

      CV_DebugS debug_s      = task->cv->debug_s_arr[obj_idx];
      String8   string_table = cv_string_table_from_debug_s(&debug_s);

      // checksum is always at the head of C13 data
      String8 file_chksms_raw = {0};
      {
        CV_C13SubSectionHeader header = {0};
        str8_deserial_read_struct(c13_data, 0, &header);

        if (header.kind == CV_C13SubSectionKind_FileChksms) {
          Rng1U64 file_chksms_range = r1u64(sizeof(CV_C13SubSectionHeader), sizeof(CV_C13SubSectionHeader) + header.size);
          file_chksms_raw = str8_substr(c13_data, file_chksms_range);
        }
      }

      // fixup file name offsets in checksum headers
      if (file_chksms_raw.size) {
        U64         cursor  = 0;
        for (;;) {
          CV_C13Checksum header = {0};
          if (str8_deserial_read_struct(file_chksms_raw, cursor, &header) != sizeof(header)) { break; }

          String8          name     = str8_cstring_capped(string_table.str + header.name_off, string_table.str + string_table.size);
          CV_StringBucket *bucket   = cv_string_hash_table_lookup(task->string_ht, name);
          U64              name_off = task->string_table_base_offset + bucket->u.offset;

          memory_write32(file_chksms_raw.str + cursor + OffsetOf(CV_C13Checksum, name_off), safe_cast_u32(name_off));
          cursor += AlignPow2(sizeof(header) + header.len, CV_FileCheckSumsAlign);
        }
      }

      // assign source files (file map is the backing)
      mod->source_files      = string_table;
      mod->source_file_count = dbi_count_source_files(string_table);

      temp_end(temp);
    }
    barrier_wait(tp->barrier);
  }

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_pdb_strings_and_modules)
{
  LNK_BuildPdb *task = raw_task;
  Temp scratch = scratch_begin(&task->build_arena, 1);
  U32Array obj_indices = task->obj_indices[task_id];

  // all lanes hold these scratch scopes until the final string-table consumer
  U64 string_count = 0;
  for EachIndex(i, obj_indices.count) {
    String8 strings = cv_string_table_from_debug_s(&task->cv->debug_s_arr[obj_indices.v[i]]);
    for EachIndex(j, strings.size) { string_count += strings.str[j] == 0; }
  }

  U64 total_string_count = tp_sum_u64(tp, task_id, string_count);
  CV_DedupStringTablesTask *dedup = 0;
  if (task_id == 0) {
    dedup = push_array(scratch.arena, CV_DedupStringTablesTask, 1);
    dedup->arr = task->cv->debug_s_arr;
    dedup->bucket_cap = Max(1, total_string_count * 13 / 10);
    dedup->buckets = push_array(scratch.arena, CV_StringBucket *, dedup->bucket_cap);
  }
  tp_broadcast(&dedup);

  for EachIndex(i, obj_indices.count) {
    cv_dedup_strings_in_debug_s_arr_task(scratch.arena, worker_id, obj_indices.v[i], dedup, tp);
  }
  barrier_wait(tp->barrier);

  if (task_id == 0) {
    task->string_ht = (CV_StringHashTable){
      .total_string_size  = dedup->total_string_size,
      .total_insert_count = dedup->total_insert_count,
      .bucket_cap         = dedup->bucket_cap,
      .buckets            = dedup->buckets,
    };
    cv_string_hash_table_assign_buffer_offsets(tp, task->string_ht);
    task->string_table_base_offset = task->pdb->info->strtab.size;
    pdb_strtab_add_cv_string_hash_table(&task->pdb->info->strtab, task->string_ht);
  }
  barrier_wait(tp->barrier);

  if (task->build_modules) {
    lnk_write_pdb_modules(scratch.arena, worker_id, task_id, task, tp);
    barrier_wait(tp->barrier);
  }
  
  PDB_DbiBuildFileInfoTask *file_info = 0;
  if (task_id == 0) {
    file_info = push_array(task->build_arena, PDB_DbiBuildFileInfoTask, 1);
    *file_info = dbi_prepare_file_info(task->build_arena, task->pdb->dbi->module_list, task->string_ht);
    task->file_info = file_info->data;
  }
  tp_broadcast(&file_info);

  for (U64 i = task_id; i < file_info->mod_count; i += tp->worker_count) {
    dbi_build_file_info_assign_file_offsets_task(0, worker_id, i, file_info, tp);
  }

  for (U64 i = task_id; i < task->string_ht.bucket_cap; i += tp->worker_count) {
    CV_StringBucket *bucket = task->string_ht.buckets[i];
    if (bucket) {
      MemoryCopy(file_info->string_buffer.str + bucket->u.offset, bucket->string.str, bucket->string.size);
      file_info->string_buffer.str[bucket->u.offset + bucket->string.size] = 0;
    }
  }
  barrier_wait(tp->barrier);

  if (task_id == 0) {
    MemoryZeroStruct(&task->string_ht);
    MemoryZeroStruct(&file_info->string_ht);
    for EachNode(mod, PDB_DbiModule, task->pdb->dbi->module_list.first) { mod->source_files = str8_zero(); }
  }
  barrier_wait(tp->barrier);
  
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_push_dbi_sec_contrib_task)
{
  // TODO: compute CRC for relocations

  U64            obj_idx = task_id;
  LNK_BuildPdb  *task    = raw_task;
  PDB_DbiModule *mod     = task->mod_arr    [obj_idx];
  LNK_Obj       *obj     = task->cv->obj_arr[obj_idx];

  PDB_DbiSCArray *sc_array = &task->sc_arrays[obj_idx];

  for LNK_EachCoffSection(it, obj) {
    // filter by section flags
    if (*it.v.flags & (COFF_SectionFlag_LnkInfo | COFF_SectionFlag_LnkRemove | LNK_SECTION_FLAG_DEBUG)) { continue; }

    // skip unwind info for the section contribution
    String8 section_name = lnk_obj_section_name_from_section_number(obj, it.v.section_number);
    if (str8_match(section_name, str8_lit(".pdata"), 0)) { continue; }

    // load section and determine its type
    LNK_ObjSection  section = it.v;
    B32             is_virt = !!(*section.flags & COFF_SectionFlag_CntUninitializedData);

    // pick section range
    Rng1U64 section_range = is_virt ? section.vrange : section.frange;
    if (dim_1u64(section_range) == 0) { continue; }

    // map the SC offset to the image section range that contains it
    Rng1U64Array *image_ranges  = is_virt ? &task->image_section_virt_ranges : &task->image_section_file_ranges;
    U64           search_result = rng1u64_array_num_from_value__binary_search(image_ranges, section_range.min);

    // log & skip SC offsets that failed to map
    if (search_result == 0) {
      Temp scratch = scratch_begin(0,0);
      lnk_log(LNK_Log_Debug, "%S: failed to map section offset 0x%llx into the linked image; skipping this section", lnk_loc_from_obj(scratch.arena, obj), section_range.min);
      scratch_end(scratch);
      continue;
    }

    // unpack image range index
    U64 range_idx = search_result - 1;

    // fill out & push section contribution
    U64 sc_idx = sc_array->count++;
    if (sc_array->v == 0) { continue; } // count pass
    Assert(sc_idx < sc_array->cap);
    PDB_DbiSC *sc = &sc_array->v[sc_idx];
    sc->base.sec     = (U16)(is_virt ? range_idx : task->image_section_file_section_numbers[range_idx]);
    sc->base.pad0    = 0;
    sc->base.sec_off = section_range.min - image_ranges->v[range_idx].min;
    sc->base.size    = dim_1u64(section_range);
    sc->base.flags   = *section.flags;
    sc->base.mod     = mod->imod;
    sc->base.pad1    = 0;
    sc->data_crc     = is_virt ? 0 : crc32_from_string(str8_substr(task->image_data, section_range));
    sc->reloc_crc    = 0;
  }

  if (sc_array->v == 0) { return; }

  // find first code section contribution for the Mod1::fUpdateSecContrib
  for EachIndex(i, sc_array->count) {
    if (sc_array->v[i].base.flags & COFF_SectionFlag_CntCode) {
      mod->first_sc = sc_array->v[i];
      break;
    }
  }
}

typedef struct
{
  LNK_BackgroundFileWriter *writer;
  LNK_BackgroundFile       *file;
  MSF_StreamNumber         *sealed_streams;
  U64                       sealed_stream_count;
  U64                       sealed_stream_cap;
} LNK_PdbOutput;

internal void
lnk_pdb_output_enqueue_stream(LNK_PdbOutput *output, MSF_Context *msf, MSF_StreamNumber sn)
{
  if (sn == MSF_INVALID_STREAM_NUMBER) { return; }
  Assert(output->sealed_stream_count < output->sealed_stream_cap);
  output->sealed_streams[output->sealed_stream_count++] = sn;

  MSF_Stream *stream = msf_find_stream(msf, sn);
  Assert(stream != 0);
  if (stream->page_list.count == 0) { return; }

  U64 stream_offset = 0;
  MSF_PageNumber run_first_pn = 0;
  MSF_PageNumber run_last_pn  = 0;
  U8            *run_data     = 0;
  U64            run_size     = 0;
  for EachNode(page, MSF_PageNode, stream->page_list.first) {
    U8 *page_data = stream->data + stream_offset;
    stream_offset += msf->page_size;
    if (run_data != 0 && page->pn == run_last_pn + 1 && page_data == run_data + run_size) {
      run_last_pn = page->pn;
      run_size += msf->page_size;
    } else {
      if (run_data != 0) {
        lnk_background_file_writer_enqueue(output->writer, output->file, (U64)run_first_pn * msf->page_size, str8(run_data, run_size));
      }
      run_first_pn = run_last_pn = page->pn;
      run_data = page_data;
      run_size = msf->page_size;
    }
  }
  if (run_data != 0) {
    lnk_background_file_writer_enqueue(output->writer, output->file, (U64)run_first_pn * msf->page_size, str8(run_data, run_size));
  }
}

internal void
lnk_pdb_output_finalize_stream(void *user_data, MSF_Context *msf, MSF_StreamNumber sn)
{
  lnk_pdb_output_enqueue_stream(user_data, msf, sn);
}

internal void
lnk_pdb_output_enqueue_remaining(LNK_PdbOutput *output, MSF_Context *msf)
{
  Temp scratch = scratch_begin(0,0);
  U64 save_size  = msf_get_save_size(msf);
  U64 page_count = CeilIntegerDiv(save_size, msf->page_size);
  U8 *is_written = push_array(scratch.arena, U8, page_count);

  for EachIndex(i, output->sealed_stream_count) {
    MSF_Stream *stream = msf_find_stream(msf, output->sealed_streams[i]);
    Assert(stream != 0);
    for EachNode(page, MSF_PageNode, stream->page_list.first) {
      Assert(page->pn < page_count);
      is_written[page->pn] = 1;
    }
  }

  U64 run_first_pn = 0;
  U8 *run_data = 0;
  U64 run_size = 0;
  for EachIndex(pn, page_count) {
    U8 *page_data = msf_data_from_pn(msf, pn).str;
    U64 page_size = Min(msf->page_size, save_size - pn * msf->page_size);
    if (!is_written[pn]) {
      if (run_data != 0 && page_data == run_data + run_size) {
        run_size += page_size;
      } else {
        if (run_data != 0) {
          lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size));
        }
        run_first_pn = pn;
        run_data = page_data;
        run_size = page_size;
      }
    } else if (run_data != 0) {
      lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size));
      run_data = 0;
      run_size = 0;
    }
  }
  if (run_data != 0) {
    lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size));
  }
  scratch_end(scratch);
}

internal LNK_FileArtifact
lnk_build_pdb(TP_Context *tp, Arena *arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types, LNK_PdbWriter writer, LNK_PDB_BuilderFlags builder_flags, LNK_PdbModules *modules_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  if (builder_flags == LNK_PDB_BuilderFlag_All) {
    builder_flags = ~0;
  }

  LNK_BuildPdb task = {
    .image_data                         = image_data,
    .symtab                             = symtab,
    .cv                                 = cv,
    .types                              = &cv_types,
    .pdb                                = pdb_alloc_(arena, config->pdb_page_size, config->machine, config->time_stamp, config->age, config->guid),
    .build_arena                        = scratch.arena,
    .build_modules                      = !!(builder_flags & LNK_PDB_BuilderFlag_Modules),
    .mod_arr                            = push_array(scratch.arena, PDB_DbiModule *, cv->obj_count),
    .pe                                 = pe_bin_info_from_data(scratch.arena, image_data),
    .image_section_table                = coff_section_table_from_data(scratch.arena, image_data, task.pe.section_table_range),
    .image_section_table_count          = task.pe.section_count+1,
    .image_section_virt_ranges.count    = task.image_section_table_count,
    .image_section_virt_ranges.v        = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
    .image_section_file_ranges.v        = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
    .image_section_file_section_numbers = push_array(scratch.arena, U64, task.image_section_table_count),
  };

  LNK_PdbOutput  output     = {0};
  LNK_PdbOutput *output_ptr = 0;
  if (writer.output_path.size > 0) {
    output.writer            = writer.file_writer;
    output.file              = lnk_background_file_writer_begin_file(output.writer, writer.output_path, writer.temp_output_path);
    if (output.file != 0) {
      output.sealed_stream_cap = cv->obj_count + 128;
      output.sealed_streams    = push_array_no_zero(scratch.arena, MSF_StreamNumber, output.sealed_stream_cap);
      output_ptr               = &output;
    }
  }

  PDB_BuildHooks build_hooks = {0};
  if (output_ptr != 0) {
    build_hooks.stream_finalize = lnk_pdb_output_finalize_stream;
    build_hooks.user_data       = output_ptr;
  }

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

  if (task.build_modules) {
    for EachIndex(obj_idx, cv->obj_count) {
      task.mod_arr[obj_idx] = dbi_push_module(task.pdb->dbi, cv->obj_arr[obj_idx]->path, lnk_obj_get_lib_path(cv->obj_arr[obj_idx]));
    }
  }
  tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_build_pdb_strings_and_modules, &task, "Build Strings and Modules");
  
  if (task.build_modules) {
    Arena *prepared_arena = modules_out ? arena : scratch.arena;
    task.mod_views = push_array(prepared_arena, PDB_ModuleView, cv->obj_count);
   
    for EachIndex(obj_idx, cv->obj_count) {
      PDB_DbiModule *mod = task.mod_arr[obj_idx];
      if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }
      String8 module = msf_stream_data(task.pdb->msf, mod->sn);
      task.mod_views[obj_idx] = dbi_module_view(mod, module);
    }
   
    if (modules_out) {
      *modules_out = (LNK_PdbModules){ .count = cv->obj_count, .modules = task.mod_views };
    }
  }

  if (output_ptr && task.build_modules) {
    for EachIndex(obj_idx, cv->obj_count) { lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.mod_arr[obj_idx]->sn); }
  }

  String8 type_name_map = {0};
  CV_TypeIndexSource sources[] = { CV_TypeIndexSource_TPI, CV_TypeIndexSource_IPI };
  for EachElement(i, sources) {
    CV_TypeIndexSource source = sources[i];

    LNK_PDB_BuilderFlags flag       = source == CV_TypeIndexSource_TPI ? LNK_PDB_BuilderFlag_Tpi : LNK_PDB_BuilderFlag_Ipi;
    U64                  leaf_count = (builder_flags & flag) ? cv_types.count[source] : 0;
    String8             *map_out    = source == CV_TypeIndexSource_TPI && config->pdb_hash_type_name_map.size && cv_types.input ? &type_name_map : 0;
    lnk_build_pdb_type_stream(scratch.arena, tp, task.pdb, &cv_types, source, leaf_count, map_out);

    if (output_ptr) {
      MSF_StreamNumber sn = source == CV_TypeIndexSource_TPI ? PDB_FixedStream_Tpi : PDB_FixedStream_Ipi;
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, sn);
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->type_servers[source]->hash_sn);
    }
  }

  if (config->pdb_hash_type_name_map.size && cv_types.input) {
    lnk_write_data_to_file_path(config->pdb_hash_type_name_map, str8_zero(), type_name_map);
  }

  if (builder_flags & LNK_PDB_BuilderFlag_Modules) {
    ProfScope("Move Global Symbols") tp_for_parallel(tp, 0, tp->worker_count, lnk_move_global_symbols_to_gsi, &task);
    if (output_ptr != 0) {
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->publics_sn);
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->globals_sn);
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->symbols_sn);
    }
  }
  
  // global-symbol indexing has finished with its temporary tables; reclaim
  // only popped scratch blocks; module views and queued PDB pages remain live
  tp_release_scratch_free_blocks(tp);

  if (builder_flags & LNK_PDB_BuilderFlag_SC) {
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
          U64 section_file_idx = task.image_section_file_ranges.count++;
          task.image_section_file_ranges.v[section_file_idx]        = r1u64s(sect_header->foff, sect_header->fsize);
          task.image_section_file_section_numbers[section_file_idx] = i;
        }

        task.image_section_virt_ranges.v[i] = rng_1u64(sect_header->voff, sect_header->voff + sect_header->vsize);
      }

      task.sc_arrays = push_array(scratch.arena, PDB_DbiSCArray, cv->obj_count);
      tp_for_parallel(tp, 0, cv->obj_count, lnk_push_dbi_sec_contrib_task, &task);

      PDB_DbiSCArray *sec_contribs = &task.pdb->dbi->sec_contribs;
      U64             new_count    = sec_contribs->count;
      for EachIndex(obj_idx, cv->obj_count) {
        new_count += task.sc_arrays[obj_idx].count;
      }
      PDB_DbiSC *new_v = push_array_no_zero(task.pdb->dbi->arena, PDB_DbiSC, new_count);
      MemoryCopyTyped(new_v, sec_contribs->v, sec_contribs->count);
      U64 cursor = sec_contribs->count;
      for EachIndex(obj_idx, cv->obj_count) {
        PDB_DbiSCArray *src = &task.sc_arrays[obj_idx];
        src->v = new_v + cursor;
        src->cap = src->count;
        cursor += src->count;
        src->count = 0;
      }
      tp_for_parallel(tp, 0, cv->obj_count, lnk_push_dbi_sec_contrib_task, &task);
      sec_contribs->v     = new_v;
      sec_contribs->count = new_count;
      sec_contribs->cap   = new_count;
    }
    ProfEnd();
  }

  if (builder_flags & LNK_PDB_BuilderFlag_NATVIS) {
    ProfBegin("Build NatVis");
    {
      String8Array  natvis_file_path_arr = str8_array_from_list(scratch.arena, &config->natvis_list);
      B8           *natvis_was_read      = push_array(scratch.arena, B8, natvis_file_path_arr.count);
      String8Array  natvis_file_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, natvis_file_path_arr, natvis_was_read);

      for EachIndex(i, natvis_file_data_arr.count) {
        String8 natvis_file_path = natvis_file_path_arr.v[i];
        String8 natvis_file_data = natvis_file_data_arr.v[i];

        // did we read the file?
        if (natvis_was_read[i] == 0 || natvis_file_data.size == 0) {
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
  }

  pdb_build_dbi_info(tp, task.pdb, task.file_info, 0, cv->is_stripped, &build_hooks);

  MSF_Error msf_err = msf_build(task.pdb->msf);
  if (msf_err != MSF_Error_Ok) {
    lnk_error(LNK_Error_UnableToSerializeMsf, "unable to serialize MSF: %s", msf_error_to_string(msf_err));
  }

  if (output_ptr != 0) {
    lnk_pdb_output_enqueue_remaining(output_ptr, task.pdb->msf);
  }

  ProfBegin("Get Page Nodes");
  LNK_FileArtifact artifact = { .data = msf_get_page_data_nodes(arena, task.pdb->msf) };
  ProfEnd();

  if (output_ptr != 0) {
    lnk_background_file_writer_end_file(output_ptr->writer, output_ptr->file, artifact.data.total_size);
  }

  // NOTE: linker is about to exit so we can skip memory release
  // and let windows free memory since it does this faster
#if 0
  ProfBegin("Context Release");
  pdb_release(&pdb);
  ProfEnd();
#endif

  scratch_end(scratch);
  ProfEnd();
  return artifact;
}

internal LNK_FileArtifact
lnk_build_stripped_pdb(TP_Context *tp, Arena *arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, U64 obj_count, LNK_Obj **obj_arr, LNK_PdbModules pdb_modules)
{
  Temp scratch = scratch_begin(&arena, 1);

  // emit static symbol info (PSI is for externals only)
  CV_DebugS *stripped_debug_s_arr = push_array(arena, CV_DebugS, obj_count);
  for EachIndex(obj_idx, obj_count) {
    CV_DebugS *debug_s_dst = &stripped_debug_s_arr[obj_idx];
    U64        proc_size   = lnk_size_stripped_symbols(pdb_modules.modules[obj_idx].symbols, PDB_SYMBOL_ALIGN);
    if (proc_size) {
      String8  symbols       = pdb_modules.modules[obj_idx].symbols;
      U64      buffer_size   = proc_size;
      U8      *buffer        = cv_debug_s_alloc_subsection(arena, debug_s_dst, CV_C13SubSectionKind_Symbols, buffer_size).str;
      U64      buffer_cursor = 0;
      for (U64 cursor = 0; cursor < symbols.size; ) {
        // parse symbol form PDB module
        CV_Symbol symbol = {0};
        TryReadBreak(cv_read_symbol(symbols, cursor, PDB_SYMBOL_ALIGN, &symbol), cursor);

        // skip everything but static procedure symbols 
        if (cv_is_lproc(symbol) == 0) { continue; }

        // copy the procedure symbol to output buffer
        U8 *dst_proc = buffer + buffer_cursor;
        buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &symbol, PDB_SYMBOL_ALIGN);

        // strip procedure symbol
        CV_SymProc32 *proc32 = (CV_SymProc32 *)(dst_proc + sizeof(CV_SymbolHeader));
        memory_write32(&proc32->parent, 0);
        memory_write32(&proc32->end, 0);
        memory_write32(&proc32->next, 0);
        memory_write32(&proc32->itype, 0);

        // close procedure scope with S_END
        buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &(CV_Symbol){ .kind = CV_SymKind_END }, PDB_SYMBOL_ALIGN);
      }

      Assert(buffer_cursor == buffer_size);
    }
  }

  LNK_CodeViewInput stripped_cv = {
    .config      = config,
    .is_stripped = 1,
    .obj_arr     = obj_arr,
    .obj_count   = obj_count,
    .count       = obj_count,
    .debug_s_arr = stripped_debug_s_arr
  };
  LNK_MergedTypes  null_types      = {0};
  LNK_PdbWriter    null_pdb_writer = {0};
  LNK_FileArtifact pdb_artifact    = lnk_build_pdb(tp, lnk_get_huge_arena(), image_data, config, symtab, &stripped_cv, null_types, null_pdb_writer, LNK_PDB_BuilderFlag_All, 0);
  lnk_write_data_list_to_file_path(config->pdb_stripped_name, str8f(scratch.arena, "%S.tmp", config->pdb_stripped_name), pdb_artifact.data);

  scratch_end(scratch);
  return pdb_artifact;
}

