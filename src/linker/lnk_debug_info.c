// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_s_task)
{
  U64                      obj_idx = task_id;
  LNK_ParseDebugSTaskData *task    = raw_task;

  LNK_Obj      *obj       = task->obj_arr[obj_idx];
  LNK_ChunkList sect_list = task->sect_list_arr[obj_idx];
  CV_DebugS    *debug_s   = &task->debug_s_arr[obj_idx];

  for (LNK_ChunkNode *node = sect_list.first; node != 0; node = node->next) {
    LNK_ChunkPtr chunk = node->data;
    Assert(chunk->type == LNK_Chunk_Leaf);

    // parse & merge sub sections
    CV_DebugS ds = cv_parse_debug_s(arena, chunk->u.leaf);
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
lnk_parse_debug_s_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, LNK_ChunkList *sect_list_arr)
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
THREAD_POOL_TASK_FUNC(lnk_check_debug_t_sig_and_get_data_task)
{
  U64                         obj_idx = task_id;
  LNK_CheckDebugTSigTaskData *task    = raw_task;

  String8Array data_arr = task->data_arr_arr[obj_idx];
  LNK_Obj *obj = task->obj_arr[obj_idx];

  for (String8 *data_ptr = &data_arr.v[0], *data_opl = data_arr.v + data_arr.count;
       data_ptr < data_opl;
       ++data_ptr) {
    if (data_ptr->size == 0) {
      continue;
    }

    if (data_ptr->size < sizeof(CV_Signature)) {
      // TODO: print section index
      lnk_error_obj(LNK_Error_IllData, obj, ".debug$T must have at least 4 bytes for CodeView signature");
    }

    CV_Signature *sig_ptr = (CV_Signature *)data_ptr->str;
    switch (*sig_ptr) {
    default: {
      lnk_error_obj(LNK_Warning_IllData, obj, "unknown CodeView type signature in section (TODO: print section index)");
      *data_ptr = str8(0,0);
    } break;
    case CV_Signature_C6:  {
      lnk_not_implemented("TODO: C6 types");
      *data_ptr = str8(0,0);
    } break;
    case CV_Signature_C7: {
      lnk_not_implemented("TODO: C7 types");
      *data_ptr = str8(0,0);
    } break;
    case CV_Signature_C11: {
      lnk_not_implemented("TODO: C11 types");
      *data_ptr = str8(0,0);
    } break;
    case CV_Signature_C13: {
      data_ptr->str += sizeof(CV_Signature);
      data_ptr->size -= sizeof(CV_Signature);
    } break;
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_t_task)
{
  ProfBeginFunction();
  U64                      obj_idx  = task_id;
  LNK_ParseDebugTTaskData *task     = raw_task;
  String8Array             data_arr = task->data_arr_arr[obj_idx];
  CV_DebugT               *debug_t  = &task->debug_t_arr[obj_idx];
  *debug_t = cv_debug_t_from_data_arr(arena, data_arr, CV_LeafAlign);
  ProfEnd();
}

internal CV_DebugT *
lnk_parse_debug_t_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, LNK_ChunkList *debug_t_list_arr)
{
  ProfBeginFunction();
  
  // list -> array
  String8Array *data_arr_arr = lnk_data_arr_from_chunk_ptr_list_arr(arena->v[0], debug_t_list_arr, obj_count);

  // validate signatures
  LNK_CheckDebugTSigTaskData check_sig;
  check_sig.obj_arr      = obj_arr;
  check_sig.data_arr_arr = data_arr_arr;
  tp_for_parallel(tp, 0, obj_count, lnk_check_debug_t_sig_and_get_data_task, &check_sig);

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
  LNK_CodeViewSymbolsInput   *input = &task->inputs[task_id];
  cv_parse_symbol_sub_section(arena, input->symbol_list, 0, input->raw_symbols, CV_SymbolAlign);
}

internal LNK_PchInfo *
lnk_setup_pch(Arena *arena, U64 obj_count, LNK_Obj *obj_arr, CV_DebugT *debug_t_arr, CV_DebugT *debug_p_arr, CV_SymbolListArray *parsed_symbols)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 work_dir = os_get_current_path(scratch.arena);

  HashTable      *debug_p_ht     = hash_table_init(scratch.arena, obj_count);
  CV_LeafHeader **endprecomp_arr = push_array(scratch.arena, CV_LeafHeader *, obj_count);

  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    CV_DebugT *debug_p = &debug_p_arr[obj_idx];
    CV_DebugT *debug_t = &debug_t_arr[obj_idx];

    if (debug_t->count && debug_p->count) {
        lnk_error_obj(LNK_Warning_MultipleDebugTAndDebugP,
                      &obj_arr[obj_idx],
                      "multiple sections with debug types detected, obj must have either .debug$T or .debug$P (using .debug$T for type server)");
      continue;
    }

    if (debug_p->count) {
      String8 obj_path = obj_arr[obj_idx].path;      
      obj_path = path_absolute_dst_from_relative_dst_src(scratch.arena, obj_path, work_dir);
      if (hash_table_search_path(debug_p_ht, obj_path)) {
        lnk_error_obj(LNK_Warning_DuplicateObjPath, &obj_arr[obj_idx], "duplicate obj path %S", obj_path);
      } else {
        hash_table_push_path_u64(scratch.arena, debug_p_ht, obj_path, obj_idx);
      }
    }
  }

  LNK_PchInfo* pch_arr = push_array_no_zero(arena, LNK_PchInfo, obj_count);
  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    CV_DebugT debug_t = debug_t_arr[obj_idx];
    if (cv_debug_t_is_pch(debug_t)) {
      CV_Leaf        precomp_leaf = cv_debug_t_get_leaf(debug_t, 0);
      CV_PrecompInfo precomp      = cv_precomp_info_from_leaf(precomp_leaf);

      String8 obj_path = path_absolute_dst_from_relative_dst_src(scratch.arena, precomp.obj_name, work_dir);

      // map obj name in LF_PRECOMP to obj index
      U64 debug_p_obj_idx;
      if (!hash_table_search_path_u64(debug_p_ht, obj_path, &debug_p_obj_idx)) {
        lnk_error_obj(LNK_Error_PrecompObjNotFound, &obj_arr[obj_idx], "LF_PRECOMP references non-existent obj %S", obj_path);
        lnk_exit(LNK_Error_PrecompObjNotFound);
      }

      // get LF_PRECOMP
      CV_DebugT          debug_p         = debug_p_arr[debug_p_obj_idx];
      CV_Leaf            endprecomp_leaf = cv_debug_t_get_leaf(debug_p, precomp.leaf_count);
      CV_LeafEndPreComp *endprecomp      = (CV_LeafEndPreComp*) endprecomp_leaf.data.str;

      // error check LF_PRECOMP
      if (precomp.start_index > CV_MinComplexTypeIndex) {
        lnk_error_obj(LNK_Warning_AtypicalStartIndex, &obj_arr[obj_idx], "atypical start index 0x%X in LF_PRECOMP", precomp.start_index);
      }
      if (precomp.start_index < CV_MinComplexTypeIndex) {
        lnk_error_obj(LNK_Error_InvalidStartIndex, &obj_arr[obj_idx], "invalid start index 0x%X in LF_PRECOMP; must be >= 0x%X", precomp.start_index, CV_MinComplexTypeIndex);
      }
      if (precomp.leaf_count > debug_p.count) {
        lnk_error_obj(LNK_Error_InvalidPrecompLeafCount, &obj_arr[obj_idx], "leaf count %u LF_PRECOMP exceeds leaf count %u in .debug$P in %S", precomp.leaf_count, debug_p.count, obj_arr[debug_p_obj_idx].path);
      }

      // error check LF_ENDPRECOMP
      if (endprecomp_leaf.kind != CV_LeafKind_ENDPRECOMP) {
        lnk_error_obj(LNK_Error_EndprecompNotFound, &obj_arr[obj_idx], "unable to find LF_ENDPRECOMP @ 0x%X in %S", precomp.leaf_count, obj_arr[debug_p_obj_idx].path);
      }
      if (endprecomp_leaf.data.size != sizeof(CV_LeafEndPreComp)) {
        lnk_error_obj(LNK_Error_IllData, &obj_arr[obj_idx], "invalid size 0x%X for LF_ENDPRECOMP", endprecomp_leaf.data.size);
      }
      if (endprecomp->sig != precomp.sig) {
        lnk_error_obj(LNK_Error_PrecompSigMismatch, &obj_arr[obj_idx], "signature mismatch between LF_PRECOMP(0x%X) and LF_ENDPRECOMP(0x%X); precomp obj %S", precomp.sig, endprecomp->sig, obj_arr[debug_p_obj_idx].path);
      }
      { // check against S_OBJNAME sig in precompiled obj $$SYMBOLS
        CV_SymbolList symbol_list = parsed_symbols[debug_p_obj_idx].v[0];
        if (symbol_list.count) {
          CV_ObjInfo obj_info = cv_obj_info_from_symbol(symbol_list.first->data);
          if (obj_info.sig != 0 && obj_info.sig != precomp.sig) {
            lnk_error_obj(LNK_Error_PrecompSigMismatch, &obj_arr[obj_idx], "signature mismatch between LF_PRECOMP(0x%X) and S_OBJNAME(0x%X) in %S", precomp.sig, obj_info.sig, &obj_arr[debug_p_obj_idx].path);
          }
        } else {
          lnk_error_obj(LNK_Warning_PrecompObjSymbolsNotFound, &obj_arr[obj_idx], "symbols not found, unable to chceck LF_PRECOMP signature against S_OBJ");
        }
      }

      // see :pch_check
      LNK_PchInfo *pch     = &pch_arr[obj_idx];
      pch->ti_lo           = precomp.start_index;
      pch->ti_hi           = precomp.start_index + precomp.leaf_count;
      pch->debug_p_obj_idx = debug_p_obj_idx;

      // [start_index, start_index+type_index_count)
      debug_t_arr[obj_idx].count -= 1;
      debug_t_arr[obj_idx].v     += 1;

      endprecomp_arr[debug_p_obj_idx] = cv_debug_t_get_leaf_header(debug_p, precomp.leaf_count);
    } else {
      LNK_PchInfo *pch     = &pch_arr[obj_idx];
      pch->ti_lo           = CV_MinComplexTypeIndex;
      pch->ti_hi           = CV_MinComplexTypeIndex;
      pch->debug_p_obj_idx = 0; // :null_obj
    }
  }
 
  // remove LF_ENDPRECOMP
  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    if (endprecomp_arr[obj_idx]) {
      endprecomp_arr[obj_idx]->kind = CV_LeafKind_NOTYPE;
      endprecomp_arr[obj_idx]->size = sizeof(CV_LeafKind);
    }
  }

  scratch_end(scratch);
  return pch_arr;
}

internal void
lnk_do_debug_info_discard(CV_DebugS *debug_s_arr, CV_SymbolListArray *parsed_symbols, U64 obj_idx)
{
  // remove symbols
  for (U64 i = 0; i < parsed_symbols[obj_idx].count; ++i) {
    MemoryZeroStruct(&parsed_symbols[obj_idx].v[i]);
  }

  // remove inline sites
  String8List *inlineelines_ptr = cv_sub_section_ptr_from_debug_s(&debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  MemoryZeroStruct(inlineelines_ptr);
}

internal
THREAD_POOL_TASK_FUNC(lnk_msf_parsed_from_data_task)
{
  ProfBeginFunction();
  LNK_MsfParsedFromDataTask *task = raw_task;
  // TODO: pick Info, TPI and IPI to flattten to make sure we don't waste compute on throw-away streams
  task->msf_parse_arr[task_id] = msf_parsed_from_data(arena, task->data_arr.v[task_id]);
  ProfEnd();
}

internal MSF_Parsed **
lnk_msf_parsed_from_data_parallel(TP_Arena *arena, TP_Context *tp, String8Array data_arr)
{
  ProfBeginFunction();
  LNK_MsfParsedFromDataTask task = {0};
  task.data_arr                  = data_arr;
  task.msf_parse_arr             = push_array_no_zero(arena->v[0], MSF_Parsed *, data_arr.count);
  tp_for_parallel(tp, arena, data_arr.count, lnk_msf_parsed_from_data_task, &task);
  ProfEnd();
  return task.msf_parse_arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_get_external_leaves_task)
{
  ProfBeginFunction();

  U64                        ts_idx    = task_id;
  LNK_GetExternalLeavesTask *task      = raw_task;
  MSF_Parsed                *msf_parse = task->msf_parse_arr[ts_idx];

  task->external_ti_ranges[ts_idx] = push_array(arena, Rng1U64,   CV_TypeIndexSource_COUNT);
  task->external_leaves[ts_idx]    = push_array(arena, CV_DebugT, CV_TypeIndexSource_COUNT);
  task->is_corrupted[ts_idx]       = 1;

  if (msf_parse) {
    PDB_OpenTypeServerError tpi_error = PDB_OpenTypeServerError_UNKNOWN;
    PDB_OpenTypeServerError ipi_error = PDB_OpenTypeServerError_UNKNOWN;   

    PDB_TypeServerParse tpi_parse, ipi_parse;
    if (PDB_FixedStream_Tpi < msf_parse->stream_count && PDB_FixedStream_Ipi < msf_parse->stream_count) {
      tpi_error = pdb_type_server_parse_from_data(msf_parse->streams[PDB_FixedStream_Tpi], &tpi_parse);
      ipi_error = pdb_type_server_parse_from_data(msf_parse->streams[PDB_FixedStream_Ipi], &ipi_parse);
    }

    if (tpi_error == PDB_OpenTypeServerError_OK && ipi_error == PDB_OpenTypeServerError_OK) {
      task->is_corrupted[ts_idx] = 0;

      task->external_ti_ranges[ts_idx][CV_TypeIndexSource_NULL] = rng_1u64(0,0);
      task->external_ti_ranges[ts_idx][CV_TypeIndexSource_TPI ] = tpi_parse.ti_range;
      task->external_ti_ranges[ts_idx][CV_TypeIndexSource_IPI ] = ipi_parse.ti_range;

      MemoryZeroStruct(&task->external_leaves[ts_idx][CV_TypeIndexSource_NULL]);
      task->external_leaves[ts_idx][CV_TypeIndexSource_TPI] = cv_debug_t_from_data(arena, tpi_parse.leaf_data, PDB_LEAF_ALIGN);
      task->external_leaves[ts_idx][CV_TypeIndexSource_IPI] = cv_debug_t_from_data(arena, ipi_parse.leaf_data, PDB_LEAF_ALIGN);
    } else {
      if (tpi_error != PDB_OpenTypeServerError_OK) {
        lnk_error(LNK_Error_UnableToOpenTypeServer, "failed to open TPI in %S, reson %S", task->ts_info_arr[ts_idx].name, pdb_string_from_open_type_server_error(tpi_error));
      }
      if (ipi_error != PDB_OpenTypeServerError_OK) {
        lnk_error(LNK_Error_UnableToOpenTypeServer, "failed to open IPI in %S, reason %S", task->ts_info_arr[ts_idx].name, pdb_string_from_open_type_server_error(ipi_error));
      }
    }
  }

  ProfEnd();
}

internal CV_DebugT *
lnk_merge_debug_t_and_debug_p(Arena *arena, U64 obj_count, CV_DebugT *debug_t_arr, CV_DebugT *debug_p_arr)
{
  CV_DebugT *result = push_array_no_zero(arena, CV_DebugT, obj_count);
  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    CV_DebugT *debug_p = &debug_p_arr[obj_idx];
    CV_DebugT *debug_t = &debug_t_arr[obj_idx];
    if (debug_p->count) {
      Assert(!debug_t->count);
      result[obj_idx] = *debug_p;
    } else if (debug_t->count) {
      Assert(!debug_p->count);
      result[obj_idx] = *debug_t;
    } else {
      MemoryZeroStruct(&result[obj_idx]);
    }
  }
  return result;
}

internal LNK_CodeViewInput
lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, String8List lib_dir_list, LNK_ObjList obj_list)
{
  ProfBegin("Extract CodeView");
  Temp scratch = scratch_begin(0,0);

  // obj list -> array
  U64       obj_count = obj_list.count;
  LNK_Obj **obj_arr   = lnk_obj_arr_from_list(tp_arena->v[0], obj_list);
  
  // gather debug info sections from objs
  ProfBegin("Collect CodeView");
  // TODO: fix memory leak, we need a Temp wrapper for pool arena
  B32 collect_discarded_flag = 0;
  LNK_ChunkList *debug_s_list_arr = lnk_collect_obj_chunks(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug"), str8_lit("S"), collect_discarded_flag);
  LNK_ChunkList *debug_p_list_arr = lnk_collect_obj_chunks(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug"), str8_lit("P"), collect_discarded_flag);
  LNK_ChunkList *debug_t_list_arr = lnk_collect_obj_chunks(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug"), str8_lit("T"), collect_discarded_flag);
  ProfEnd();

  if (lnk_get_log_status(LNK_Log_Debug) || PROFILE_TELEMETRY) {
    U64 total_debug_s_size = 0, total_debug_t_size = 0, total_debug_p_size = 0;
    for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
      for (LNK_ChunkNode *chunk = debug_s_list_arr[obj_idx].first; chunk != 0; chunk = chunk->next) {
        total_debug_s_size += chunk->data->u.leaf.size;
      }
      for (LNK_ChunkNode *chunk = debug_t_list_arr[obj_idx].first; chunk != 0; chunk = chunk->next) {
        total_debug_t_size += chunk->data->u.leaf.size;
      }
      for (LNK_ChunkNode *chunk = debug_p_list_arr[obj_idx].first; chunk != 0; chunk = chunk->next) {
        total_debug_p_size += chunk->data->u.leaf.size;
      }
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

  // TODO: temp hack, remove when we have null obj with .debug$T
  {
    String8 raw_null_leaf = cv_serialize_raw_leaf(scratch.arena, CV_LeafKind_NOTYPE, str8(0,0), 1);

    String8List srl = {0};
    str8_serial_begin(scratch.arena, &srl);
    str8_serial_push_u32(scratch.arena, &srl, CV_Signature_C13);
    str8_serial_push_string(scratch.arena, &srl, raw_null_leaf);
    String8 null_debug_data = str8_serial_end(tp_arena->v[0], &srl);
    
    LNK_Chunk *null_chunk = push_array(tp_arena->v[0], LNK_Chunk, 1);
    null_chunk->type      = LNK_Chunk_Leaf;
    null_chunk->u.leaf    = null_debug_data;
    lnk_chunk_list_push(tp_arena->v[0], &debug_t_list_arr[0], null_chunk);
  }

  ProfBegin("Parse CodeView");
  CV_DebugS *debug_s_arr = lnk_parse_debug_s_sections(tp, tp_arena, obj_count, obj_arr, debug_s_list_arr);
  CV_DebugT *debug_p_arr = lnk_parse_debug_t_sections(tp, tp_arena, obj_count, obj_arr, debug_p_list_arr);
  CV_DebugT *debug_t_arr = lnk_parse_debug_t_sections(tp, tp_arena, obj_count, obj_arr, debug_t_list_arr);
  ProfEnd();
 
  ProfBegin("Sort Type Servers");

  U64 external_count = 0, internal_count = 0;
  LNK_Obj   *sorted_obj_arr     = push_array_no_zero(tp_arena->v[0], LNK_Obj, obj_count);
  CV_DebugS *sorted_debug_s_arr = push_array_no_zero(tp_arena->v[0], CV_DebugS, obj_count);
  CV_DebugT *sorted_debug_t_arr = push_array_no_zero(tp_arena->v[0], CV_DebugT, obj_count);
  CV_DebugT *sorted_debug_p_arr = push_array_no_zero(tp_arena->v[0], CV_DebugT, obj_count);
  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    B32 is_type_server = cv_debug_t_is_type_server(debug_t_arr[obj_idx]);
    if (is_type_server) {
      Assert(internal_count + external_count < obj_count);
      U64 slot_idx = (obj_count - external_count - 1);
      ++external_count;

      // TODO: report error: somehow obj was compiled with /Zi and /Yc
      Assert(debug_p_arr[obj_idx].count == 0);
      
      sorted_obj_arr[slot_idx]     = *obj_arr[obj_idx];
      sorted_debug_s_arr[slot_idx] = debug_s_arr[obj_idx];
      sorted_debug_t_arr[slot_idx] = debug_t_arr[obj_idx];
      MemoryZeroStruct(&sorted_debug_p_arr[slot_idx]);
    } else {
      Assert(internal_count + external_count < obj_count);
      U64 slot_idx = internal_count;
      ++internal_count;
      
      sorted_obj_arr[slot_idx]     = *obj_arr[obj_idx];
      sorted_debug_s_arr[slot_idx] = debug_s_arr[obj_idx];
      sorted_debug_t_arr[slot_idx] = debug_t_arr[obj_idx];
      sorted_debug_p_arr[slot_idx] = debug_p_arr[obj_idx];
    }
  }

  ProfEnd();
  
  // setup pointers to arrays
  LNK_Obj   *internal_obj_arr     = sorted_obj_arr;
  LNK_Obj   *external_obj_arr     = sorted_obj_arr + internal_count;
  CV_DebugS *internal_debug_s_arr = sorted_debug_s_arr;
  CV_DebugS *external_debug_s_arr = sorted_debug_s_arr + internal_count;
  CV_DebugT *internal_debug_t_arr = sorted_debug_t_arr;
  CV_DebugT *external_debug_t_arr = sorted_debug_t_arr + internal_count;
  CV_DebugT *internal_debug_p_arr = sorted_debug_p_arr;
  CV_DebugT *external_debug_p_arr = sorted_debug_p_arr + internal_count;

  ProfBegin("Parse Symbols");

  ProfBegin("Count Symbol Inputs");
  U64 internal_total_symbol_input_count = 0;
  U64 external_total_symbol_input_count = 0;
  for (U64 obj_idx = 0; obj_idx < internal_count; ++obj_idx) {
    String8List raw_symbols = cv_sub_section_from_debug_s(internal_debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
    internal_total_symbol_input_count += raw_symbols.node_count;
  }
  for (U64 obj_idx = 0; obj_idx < external_count; ++obj_idx) {
    String8List raw_symbols = cv_sub_section_from_debug_s(external_debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
    external_total_symbol_input_count += raw_symbols.node_count;
  }
  ProfEnd();

  ProfBegin("Prepare Symbol Inputs");
  U64                       total_symbol_input_count = internal_total_symbol_input_count + external_total_symbol_input_count;
  LNK_CodeViewSymbolsInput *symbol_inputs            = push_array_no_zero(tp_arena->v[0], LNK_CodeViewSymbolsInput, total_symbol_input_count);
  CV_SymbolListArray       *parsed_symbols           = push_array_no_zero(tp_arena->v[0], CV_SymbolListArray,       obj_count);
  {
    CV_SymbolList *reserved_lists = push_array(tp_arena->v[0], CV_SymbolList, total_symbol_input_count);
    for (U64 obj_idx = 0, input_idx = 0; obj_idx < obj_count; ++obj_idx) {
      String8List raw_symbols = cv_sub_section_from_debug_s(sorted_debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);

      // init parse output
      if (raw_symbols.node_count > 0) {
        parsed_symbols[obj_idx].count = raw_symbols.node_count;
        parsed_symbols[obj_idx].v     = reserved_lists + input_idx;
      } else {
        parsed_symbols[obj_idx].count = 0;
        parsed_symbols[obj_idx].v     = 0;
      }

      // init worker input
      for (String8Node *data_n = raw_symbols.first; data_n != 0; data_n = data_n->next, ++input_idx) {
        Assert(input_idx < total_symbol_input_count);
        LNK_CodeViewSymbolsInput *in = &symbol_inputs[input_idx];
        in->obj_idx                  = obj_idx;
        in->symbol_list              = &reserved_lists[input_idx];
        in->raw_symbols              = data_n->string;
      }
    }
  }
  ProfEnd();

  ProfBegin("Symbol Parse");
  LNK_ParseCVSymbolsTaskData task = {0};
  task.inputs                     = symbol_inputs;
  tp_for_parallel(tp, tp_arena, total_symbol_input_count, lnk_parse_cv_symbols_task, &task);
  ProfEnd();

  // TODO: do we rely on this behaviour?
  //
  // :zero_out_symbol_sub_section
  ProfBegin("Zero-out Symbols Sub-sections");
  for (U64 i = 0; i < obj_count; ++i) {
    CV_DebugS *debug_s = &sorted_debug_s_arr[i];
    String8List *symbols_ptr = cv_sub_section_ptr_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
    MemoryZeroStruct(symbols_ptr);
  }
  ProfEnd();

  ProfEnd();

  CV_SymbolListArray       *internal_parsed_symbols = parsed_symbols;
  CV_SymbolListArray       *external_parsed_symbols = parsed_symbols + internal_count;
  LNK_CodeViewSymbolsInput *internal_symbol_inputs  = symbol_inputs;
  LNK_CodeViewSymbolsInput *external_symbol_inputs  = symbol_inputs + internal_count;

  LNK_PchInfo *pch_arr = lnk_setup_pch(tp_arena->v[0],
                                       internal_count,
                                       internal_obj_arr,
                                       internal_debug_t_arr,
                                       internal_debug_p_arr,
                                       internal_parsed_symbols);

  CV_DebugT *merged_debug_t_p_arr = lnk_merge_debug_t_and_debug_p(tp_arena->v[0], internal_count, internal_debug_t_arr, internal_debug_p_arr);

  ProfBegin("Analyze & Read External Type Server Files");
  String8Array ts_path_arr;
  Rng1U64    **external_ti_ranges;
  CV_DebugT  **external_leaves;
  U64         *obj_to_ts_idx_arr = push_array_no_zero(tp_arena->v[0], U64, external_count);
  U64List     *ts_to_obj_arr     = push_array(tp_arena->v[0], U64List, external_count);
  {
    HashTable             *type_server_path_ht   = hash_table_init(scratch.arena, 256);
    HashTable             *ignored_path_ht       = hash_table_init(scratch.arena, 256);
    CV_TypeServerInfoList  ts_info_list = {0};

    // push null
    CV_TypeServerInfoNode *null_ts_info = push_array(scratch.arena, CV_TypeServerInfoNode, 1);
    SLLQueuePush(ts_info_list.first, ts_info_list.last, null_ts_info);
    ++ts_info_list.count;

    for (U64 obj_idx = 0; obj_idx < external_count; ++obj_idx) {
      // first leaf always type server
      CV_DebugT         debug_t = external_debug_t_arr[obj_idx];
      CV_Leaf           leaf    = cv_debug_t_get_leaf(debug_t, 0);
      CV_TypeServerInfo ts      = cv_type_server_info_from_leaf(leaf);

      // search disk for type server
      String8List match_list = lnk_file_search(scratch.arena, lib_dir_list, ts.name);

      // chop file name from path and search on it
      //
      // TODO: check if ts.name is a path and in that case do file search
      if (match_list.node_count == 0) {
        String8 file_name = str8_skip_last_slash(ts.name);
        match_list = lnk_file_search(scratch.arena, lib_dir_list, file_name);
      }

      B32 do_debug_info_discard = 0;

      // too many matches?
      if (match_list.node_count > 1) {
        if (!hash_table_search_path(ignored_path_ht, ts.name)) {
          hash_table_push_path_u64(scratch.arena, ignored_path_ht, ts.name, 0);
          lnk_error_obj(LNK_Warning_MultipleExternalTypeServers, obj_arr[obj_idx], "located multiple external type servers:");
          lnk_supplement_error_list(match_list);
        }
        do_debug_info_discard = 1;
      } 
      // no match?
      else if (match_list.node_count == 0) {
        if (!hash_table_search_path(ignored_path_ht, ts.name)) {
          hash_table_push_string_u64(scratch.arena, ignored_path_ht, ts.name, 0);
          lnk_error_obj(LNK_Warning_MissingExternalTypeServer, obj_arr[obj_idx], "unable to open external type server %S", ts.name);
        }
        do_debug_info_discard = 1;
      }

      // external type server is missing, discard parts of debug info that need types
      if (do_debug_info_discard) {
        lnk_do_debug_info_discard(external_debug_s_arr, external_parsed_symbols, obj_idx);
        continue;
      }

      String8 path = match_list.first->string;
      {
        struct HT_Value {
          CV_TypeServerInfo  ts;
          LNK_Obj           *obj;
          U64                ts_idx;
        };

        // was this type server queued?
        KeyValuePair *is_path_queued = hash_table_search_path(type_server_path_ht, path);
        if (is_path_queued) {
          struct HT_Value *present = is_path_queued->value_raw;
          
          // make sure type servers sigs match
          if (MemoryMatchStruct(&ts.sig, &present->ts.sig)) {
            // wire obj to type server data
            obj_to_ts_idx_arr[obj_idx] = present->ts_idx;

            // wire type server to obj
            u64_list_push(tp_arena->v[0], &ts_to_obj_arr[present->ts_idx], obj_idx);
          } else {
            lnk_error_obj(LNK_Error_ExternalTypeServerConflict,
                          obj_arr[obj_idx],
                          "external type server signature conflicts with type server loaded from '%S'",
                          present->obj->path);
          }
        } else {
          U64 ts_idx = ts_info_list.count;

          // when we search matches on disk we store path on scratch,
          // make path copy in case we need it for error reporting
          path = push_str8_copy(tp_arena->v[0], path);

          // fill out type server info we read from obj
          CV_TypeServerInfoNode *ts_info_node = push_array(scratch.arena, CV_TypeServerInfoNode, 1);
          ts_info_node->data                  = ts;
          ts_info_node->data.name             = path;

          // push to type server info list
          SLLQueuePush(ts_info_list.first, ts_info_list.last, ts_info_node);
          ts_info_list.count += 1;
          
          // wire obj to type server
          obj_to_ts_idx_arr[obj_idx] = ts_idx;

          // wire type server to obj
          u64_list_push(tp_arena->v[0], &ts_to_obj_arr[ts_idx], obj_idx);
          
          // fill out value
          struct HT_Value *value = push_array(scratch.arena, struct HT_Value, 1);
          value->ts     = ts;
          value->obj    = obj_arr[obj_idx];
          value->ts_idx = ts_idx;
          
          // update hash table
          hash_table_push_path_raw(scratch.arena, type_server_path_ht, path, value);
        }
      }
    }

    // type server info list -> array
    ts_path_arr.count              = ts_info_list.count;
    ts_path_arr.v                  = push_array(tp_arena->v[0], String8, ts_info_list.count);
    CV_TypeServerInfo *ts_info_arr = push_array(scratch.arena, CV_TypeServerInfo, ts_info_list.count);
    {
      U64 idx = 0;
      for (CV_TypeServerInfoNode *n = ts_info_list.first; n != 0; n = n->next, ++idx) {
        ts_path_arr.v[idx] = n->data.name;
        ts_info_arr[idx]   = n->data;
      }
    }

    // read type servers from disk in parallel
    {
      ProfBegin("Read External Type Servers");
      String8Array msf_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, ts_path_arr);
      ProfEnd();

      MSF_Parsed **msf_parse_arr = lnk_msf_parsed_from_data_parallel(tp_arena, tp, msf_data_arr);

      ProfBegin("Error check type servers");
      for (U64 ts_idx = 0; ts_idx < msf_data_arr.count; ++ts_idx) {
        MSF_Parsed *msf_parse = msf_parse_arr[ts_idx];

        B32 do_debug_info_discard = 0;

        if (!msf_parse) {
          do_debug_info_discard = 1;
        } else {
          PDB_InfoParse info_parse = {0};
          pdb_info_parse_from_data(msf_parse->streams[PDB_FixedStream_Info], &info_parse);
          if (!MemoryMatchStruct(&info_parse.guid, &ts_info_arr[ts_idx].sig)) {
            Temp scratch = scratch_begin(0,0);
            String8 expected_sig_str = string_from_guid(scratch.arena, ts_info_arr[ts_idx].sig);
            String8 on_disk_sig_str  = string_from_guid(scratch.arena, info_parse.guid);
            lnk_error(LNK_Warning_MismatchedTypeServerSignature, "%S: signature mismatch in type server read from disk, expected %S, got %S",
                ts_info_arr[ts_idx].name, expected_sig_str, on_disk_sig_str);
            scratch_end(scratch);

            do_debug_info_discard = 1;
          }
        }

        if (do_debug_info_discard) {
          U64List obj_idx_list = ts_to_obj_arr[ts_idx];
          for (U64Node *obj_idx_n = obj_idx_list.first; obj_idx_n != 0; obj_idx_n = obj_idx_n->next) {
            lnk_do_debug_info_discard(external_debug_s_arr, external_parsed_symbols, obj_idx_n->data);
          }
        }
      }
      ProfEnd();

      ProfBeginDynamic("Open External Type Servers [Count %llu]", ts_path_arr.count);
      LNK_GetExternalLeavesTask task = {0};
      task.ts_info_arr               = ts_info_arr;
      task.msf_parse_arr             = msf_parse_arr;
      task.external_ti_ranges        = push_array_no_zero(tp_arena->v[0], Rng1U64 *, msf_data_arr.count);
      task.external_leaves           = push_array_no_zero(tp_arena->v[0], CV_DebugT *, msf_data_arr.count);
      task.is_corrupted              = push_array_no_zero(scratch.arena, B8, msf_data_arr.count);
      tp_for_parallel(tp, tp_arena, msf_data_arr.count, lnk_get_external_leaves_task, &task);
      ProfEnd();

      String8List unopen_type_server_list = {0};

      // discard debug info that depends on the missing type server 
      for (U64 ts_idx = 1; ts_idx < msf_data_arr.count; ++ts_idx) {
        if (task.is_corrupted[ts_idx]) {
          U64List obj_idx_list = ts_to_obj_arr[ts_idx];
          for (U64Node *node = obj_idx_list.first; node != 0; node = node->next) {
            lnk_do_debug_info_discard(external_debug_s_arr, external_parsed_symbols, node->data);
          }
        }
      }

      // format error 
      for (U64 ts_idx = 1; ts_idx < msf_data_arr.count; ++ts_idx) {
        if (task.is_corrupted[ts_idx]) {
          U64List obj_idx_list = ts_to_obj_arr[ts_idx];
          str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t%S\n", ts_path_arr.v[ts_idx]);
          str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\tDependent obj(s):\n");
          for (U64Node *obj_idx_node = obj_idx_list.first; obj_idx_node != 0; obj_idx_node = obj_idx_node->next) {
            String8 obj_path = external_obj_arr[obj_idx_node->data].path;
            str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\t\t%S\n", obj_path);
          }
        }
      }
      if (unopen_type_server_list.node_count) {
        String8List error_msg_list = { 0 };
        str8_list_pushf(scratch.arena, &error_msg_list, "unable to open external type server(s):\n");
        str8_list_concat_in_place(&error_msg_list, &unopen_type_server_list);
        String8 error_msg = str8_list_join(scratch.arena, &error_msg_list, 0);
        lnk_error(LNK_Error_UnableToOpenTypeServer, "%S", error_msg);
      }

      // output
      external_ti_ranges = task.external_ti_ranges;
      external_leaves    = task.external_leaves;
    }
  }
  ProfEnd();

  // fill out result
  LNK_CodeViewInput cv                 = {0};
  cv.count                             = obj_count;
  cv.internal_count                    = internal_count;
  cv.external_count                    = external_count;
  cv.type_server_count                 = ts_path_arr.count;
  cv.type_server_path_arr              = ts_path_arr.v;
  cv.ts_to_obj_arr                     = ts_to_obj_arr;
  cv.obj_arr                           = sorted_obj_arr;
  cv.pch_arr                           = pch_arr;
  cv.debug_s_arr                       = sorted_debug_s_arr;
  cv.debug_p_arr                       = sorted_debug_p_arr;
  cv.debug_t_arr                       = sorted_debug_t_arr;
  cv.merged_debug_t_p_arr              = merged_debug_t_p_arr;
  cv.total_symbol_input_count          = total_symbol_input_count;
  cv.symbol_inputs                     = symbol_inputs;
  cv.parsed_symbols                    = parsed_symbols;
  cv.internal_obj_arr                  = internal_obj_arr;
  cv.external_obj_arr                  = external_obj_arr;
  cv.internal_debug_s_arr              = internal_debug_s_arr;
  cv.external_debug_s_arr              = external_debug_s_arr;
  cv.internal_debug_t_arr              = internal_debug_t_arr;
  cv.external_debug_t_arr              = external_debug_t_arr;
  cv.internal_debug_p_arr              = internal_debug_p_arr;
  cv.external_debug_p_arr              = external_debug_p_arr;
  cv.internal_total_symbol_input_count = internal_total_symbol_input_count;
  cv.internal_symbol_inputs            = internal_symbol_inputs;
  cv.internal_parsed_symbols           = internal_parsed_symbols;
  cv.external_total_symbol_input_count = external_total_symbol_input_count;
  cv.external_symbol_inputs            = external_symbol_inputs;
  cv.external_parsed_symbols           = external_parsed_symbols;
  cv.external_ti_ranges                = external_ti_ranges;
  cv.external_leaves                   = external_leaves;
  cv.external_obj_to_ts_idx_arr        = obj_to_ts_idx_arr;
  cv.external_obj_range                = rng_1u64(internal_count, internal_count + external_count);
  
  scratch_end(scratch);
  ProfEnd();
  return cv;
}

////////////////////////////////
// Leaf Deduper


internal LNK_LeafRef
lnk_leaf_ref(U32 enc_loc_idx, U32 enc_leaf_idx)
{
  LNK_LeafRef ref;
  ref.enc_loc_idx  = enc_loc_idx;
  ref.enc_leaf_idx = enc_leaf_idx;
  return ref;
}

internal LNK_LeafRef
lnk_obj_leaf_ref(U32 obj_idx, U32 leaf_idx)
{
  return lnk_leaf_ref(obj_idx, leaf_idx);
}

internal LNK_LeafRef
lnk_ts_leaf_ref(CV_TypeIndexSource ti_source, U32 ts_idx, U32 leaf_idx)
{
  ts_idx |= LNK_LeafRefFlag_LocIdxExternal;

  if (ti_source == CV_TypeIndexSource_IPI) {
    leaf_idx |= LNK_LeafRefFlag_LeafIdxIPI;
  }

  return lnk_leaf_ref(ts_idx, leaf_idx);
}

internal int
lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b)
{
  int cmp = 0;
  if (a.enc_loc_idx < b.enc_loc_idx) {
    cmp = -1;
  } else if (a.enc_loc_idx > b.enc_loc_idx) {
    cmp = +1;
  } else {
    if (a.enc_leaf_idx < b.enc_leaf_idx) {
      cmp = -1;
    } else if (a.enc_leaf_idx > b.enc_leaf_idx) {
      cmp = +1;
    }
  }
  return cmp;
}

internal int
lnk_leaf_ref_is_before(void *raw_a, void *raw_b)
{
  LNK_LeafRef **a = raw_a;
  LNK_LeafRef **b = raw_b;
  int is_before;
  if ((*a)->enc_loc_idx == (*b)->enc_loc_idx) {
    is_before = (*a)->enc_leaf_idx < (*b)->enc_leaf_idx;
  } else {
    is_before = (*a)->enc_loc_idx < (*b)->enc_loc_idx;
  }
  return is_before;
}

internal LNK_LeafLocType
lnk_loc_type_from_leaf_ref(LNK_LeafRef leaf_ref)
{
  if (leaf_ref.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal) {
    return LNK_LeafLocType_External;
  }
  return LNK_LeafLocType_Internal;
}

internal LNK_LeafLocType
lnk_loc_type_from_obj_idx(LNK_CodeViewInput *input, U64 obj_idx)
{
  if (input->external_obj_range.min <= obj_idx && obj_idx < input->external_obj_range.max) {
    return LNK_LeafLocType_External;
  }
  return LNK_LeafLocType_Internal;
}

internal U64
lnk_loc_idx_from_obj_idx(LNK_CodeViewInput *input, U64 obj_idx)
{
  if (input->external_obj_range.min <= obj_idx && obj_idx < input->external_obj_range.max) {
    return input->external_obj_to_ts_idx_arr[obj_idx - input->external_obj_range.min];
  }
  return obj_idx;
}

internal CV_TypeIndex
lnk_ti_lo_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  CV_TypeIndex ti_lo;

  LNK_LeafLocType loc_type = lnk_loc_type_from_leaf_ref(leaf_ref);
  switch (loc_type) {
  case LNK_LeafLocType_Internal: {
    ti_lo = CV_MinComplexTypeIndex;
  } break;
  case LNK_LeafLocType_External: {
    U64                ts_idx    = leaf_ref.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
    CV_TypeIndexSource ti_source = (leaf_ref.enc_loc_idx & LNK_LeafRefFlag_LeafIdxIPI) ? CV_TypeIndexSource_IPI : CV_TypeIndexSource_TPI;
    ti_lo = input->external_ti_ranges[ts_idx][ti_source].min;
  } break;
  default: ti_lo = 0; break;
  }

  return ti_lo;
}

internal CV_TypeIndex
lnk_ti_lo_from_loc(LNK_CodeViewInput *input, LNK_LeafLocType loc_type, U64 loc_idx, CV_TypeIndexSource ti_source)
{
  CV_TypeIndex ti_lo = 0;
  if (loc_type == LNK_LeafLocType_Internal) {
    ti_lo = CV_MinComplexTypeIndex;
  } else if (loc_type == LNK_LeafLocType_External) {
    ti_lo = input->external_ti_ranges[loc_idx][ti_source].min;
  }
  return ti_lo;
}

internal String8
lnk_data_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  String8 data;

  LNK_LeafLocType loc_type = lnk_loc_type_from_leaf_ref(leaf_ref);
  switch (loc_type) {
  case LNK_LeafLocType_Internal: {
    U32       obj_idx  = leaf_ref.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
    U32       leaf_idx = leaf_ref.enc_leaf_idx;
    CV_DebugT debug_t  = input->merged_debug_t_p_arr[obj_idx];
    data = cv_debug_t_get_raw_leaf(debug_t, leaf_idx);
  } break;

  case LNK_LeafLocType_External: {
    U64                ts_idx    = leaf_ref.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
    U64                leaf_idx  = leaf_ref.enc_leaf_idx & ~LNK_LeafRefFlag_LeafIdxIPI;
    CV_TypeIndexSource ti_source = leaf_ref.enc_leaf_idx & LNK_LeafRefFlag_LeafIdxIPI ? CV_TypeIndexSource_IPI : CV_TypeIndexSource_TPI;
    CV_DebugT          debug_t   = input->external_leaves[ts_idx][ti_source];
    data = cv_debug_t_get_raw_leaf(debug_t, leaf_idx);
  } break;

  default: data = str8(0,0); break;
  }

  return data;
}

internal CV_TypeIndex
lnk_type_index_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  CV_TypeIndex type_index = 0;
  LNK_LeafLocType loc_type = lnk_loc_type_from_leaf_ref(leaf_ref);
  switch (loc_type) {
  case LNK_LeafLocType_Internal: {
    LNK_PchInfo pch_info = input->pch_arr[leaf_ref.enc_loc_idx];
    type_index = pch_info.ti_hi + leaf_ref.enc_leaf_idx;
  } break;
  case LNK_LeafLocType_External: {
    CV_TypeIndex lo = lnk_ti_lo_from_leaf_ref(input, leaf_ref);
    type_index = lo + leaf_ref.enc_leaf_idx & ~LNK_LeafRefFlag_LeafIdxIPI;
  } break;
  default: InvalidPath;
  }
  return type_index;
}

internal CV_Leaf
lnk_cv_leaf_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  String8 raw_leaf = lnk_data_from_leaf_ref(input, leaf_ref);
  CV_Leaf leaf;
  cv_deserial_leaf(raw_leaf, 0, 1, &leaf);
  return leaf;
}

internal U128
lnk_hash_from_leaf_ref(LNK_LeafHashes *hashes, LNK_LeafRef leaf_ref)
{
  LNK_LeafLocType    loc_type;
  CV_TypeIndexSource ti_source;
  if (leaf_ref.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal) {
    loc_type  = LNK_LeafLocType_External;
    ti_source = (leaf_ref.enc_leaf_idx & LNK_LeafRefFlag_LeafIdxIPI) ? CV_TypeIndexSource_IPI : CV_TypeIndexSource_TPI;
  } else {
    loc_type  = LNK_LeafLocType_Internal;
    ti_source = CV_TypeIndexSource_TPI;
  }

  U32 loc_idx  = leaf_ref.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
  U32 leaf_idx = leaf_ref.enc_leaf_idx & ~LNK_LeafRefFlag_LeafIdxIPI;
  U128 hash    = hashes->v[loc_type][loc_idx][ti_source].v[leaf_idx];

  return hash;
}

internal LNK_LeafRef
lnk_leaf_ref_from_loc_idx_and_ti(LNK_CodeViewInput  *input,
                                 LNK_LeafLocType     loc_type,
                                 CV_TypeIndexSource  ti_source,
                                 U64                 loc_idx,
                                 CV_TypeIndex        obj_ti)
{
  LNK_LeafRef leaf_ref;

  switch (loc_type) {
  case LNK_LeafLocType_External: {
    U64 ts_idx = loc_idx;

    CV_TypeIndex ti_lo = input->external_ti_ranges[ts_idx][ti_source].min;
    Assert(obj_ti >= ti_lo);

    // encode leaf index for type server
    leaf_ref = lnk_ts_leaf_ref(ti_source, ts_idx, obj_ti - ti_lo);
  } break;

  case LNK_LeafLocType_Internal: {
    U64 obj_idx = loc_idx;

    LNK_PchInfo pch = input->pch_arr[obj_idx];
    if (obj_ti < pch.ti_lo) {
      CV_TypeIndex ti_lo = CV_MinComplexTypeIndex;
      Assert(obj_ti >= ti_lo);
      leaf_ref = lnk_obj_leaf_ref(obj_idx, obj_ti - ti_lo);
    }
    // PCH indirection
    else if (obj_ti < pch.ti_hi) {
      // we don't support nested precompiled types
      Assert(input->pch_arr[pch.debug_p_obj_idx].debug_p_obj_idx == /* null_obj: */ 0);
      Assert(input->pch_arr[pch.debug_p_obj_idx].ti_lo == input->pch_arr[pch.debug_p_obj_idx].ti_hi);
      leaf_ref = lnk_obj_leaf_ref(pch.debug_p_obj_idx, obj_ti - pch.ti_lo);
    } else {
      leaf_ref = lnk_obj_leaf_ref(obj_idx, pch.ti_lo + (obj_ti - pch.ti_hi) - CV_MinComplexTypeIndex);
    }
  } break;

  default: leaf_ref = lnk_leaf_ref(0, 0); break;
  }

  return leaf_ref;
}

internal B32
lnk_match_leaf_ref(LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef a, LNK_LeafRef b)
{
  B32 are_same = 0;

  U128 a_hash = lnk_hash_from_leaf_ref(hashes, a);
  U128 b_hash = lnk_hash_from_leaf_ref(hashes, b);

  if (u128_match(a_hash, b_hash)) {
    CV_Leaf a_leaf = lnk_cv_leaf_from_leaf_ref(input, a);
    CV_Leaf b_leaf = lnk_cv_leaf_from_leaf_ref(input, b);
    Assert(a_leaf.kind == b_leaf.kind);
#if 0
    {
      Temp scratch = scratch_begin(0,0);
      CV_TypeIndexInfoList ti_info_list   = cv_get_leaf_type_index_offsets(scratch.arena, a_leaf.kind, a_leaf.data);
      String8Array         a_raw_data_arr = cv_get_data_around_type_indices(scratch.arena, ti_info_list, a_leaf.data);
      String8Array         b_raw_data_arr = cv_get_data_around_type_indices(scratch.arena, ti_info_list, b_leaf.data);
      for (U64 i = 0; i < a_raw_data_arr.count; ++i) {
        String8 a_chunk = a_raw_data_arr.v[i];
        String8 b_chunk = b_raw_data_arr.v[i];
        Assert(str8_match(a_chunk, b_chunk, 0));
      }
      scratch_end(scratch);
    }
#endif
    are_same = 1;
  }

  return are_same;
}

internal B32
lnk_match_leaf_ref_deep(Arena *arena, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef a, LNK_LeafRef b)
{
  B32 are_equal = 0;

  U128 a_hash = lnk_hash_from_leaf_ref(hashes, a);
  U128 b_hash = lnk_hash_from_leaf_ref(hashes, b);
  
  if (u128_match(a_hash, b_hash)) {
    String8 a_raw_leaf = lnk_data_from_leaf_ref(input, a);
    String8 b_raw_leaf = lnk_data_from_leaf_ref(input, b);

    CV_LeafHeader *a_header = (CV_LeafHeader *) a_raw_leaf.str;
    CV_LeafHeader *b_header = (CV_LeafHeader *) b_raw_leaf.str;

    if (a_header->kind == b_header->kind && a_header->size == b_header->size) {
      CV_Leaf a_leaf = cv_leaf_from_string(a_raw_leaf);
      CV_Leaf b_leaf = cv_leaf_from_string(b_raw_leaf);

      Temp temp = temp_begin(arena);

      CV_TypeIndexInfoList ti_info_list   = cv_get_leaf_type_index_offsets(temp.arena, a_leaf.kind, a_leaf.data);
      String8Array         a_raw_data_arr = cv_get_data_around_type_indices(temp.arena, ti_info_list, a_leaf.data);
      String8Array         b_raw_data_arr = cv_get_data_around_type_indices(temp.arena, ti_info_list, b_leaf.data);

      are_equal = 1;

      for (U64 i = 0; i < a_raw_data_arr.count; ++i) {
        String8 a_chunk = a_raw_data_arr.v[i];
        String8 b_chunk = b_raw_data_arr.v[i];
        Assert(a_chunk.size == b_chunk.size);
        are_equal = str8_match(a_chunk, b_chunk, 0);
        if (!are_equal) {
          goto skip_type_index_compare;
        }
      }

      CV_TypeIndex a_ti_lo = lnk_ti_lo_from_leaf_ref(input, a);
      CV_TypeIndex b_ti_lo = lnk_ti_lo_from_leaf_ref(input, b);
      AssertAlways(a_ti_lo == b_ti_lo);

      for (CV_TypeIndexInfo *ti_info = ti_info_list.first; ti_info != 0; ti_info = ti_info->next) {
        CV_TypeIndex *a_ti_ptr = (CV_TypeIndex *) (a_leaf.data.str + ti_info->offset);
        CV_TypeIndex *b_ti_ptr = (CV_TypeIndex *)(b_leaf.data.str + ti_info->offset);

        if (*a_ti_ptr >= a_ti_lo && *b_ti_ptr >= b_ti_lo) {
          LNK_LeafLocType a_loc_type = (a.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal) >> 31;
          LNK_LeafLocType b_loc_type = (b.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal) >> 31;

          U64 a_loc_idx = a.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
          U64 b_loc_idx = b.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;

          LNK_LeafRef a_sub_leaf_ref = lnk_leaf_ref_from_loc_idx_and_ti(input, a_loc_type, ti_info->source, a_loc_idx, *a_ti_ptr);
          LNK_LeafRef b_sub_leaf_ref = lnk_leaf_ref_from_loc_idx_and_ti(input, b_loc_type, ti_info->source, b_loc_idx, *b_ti_ptr);

          are_equal = lnk_match_leaf_ref_deep(arena, input, hashes, a_sub_leaf_ref, b_sub_leaf_ref);
          if (!are_equal) {
            break;
          }
        }
        // compare simple leaves
        else {
          are_equal = *a_ti_ptr == *b_ti_ptr;
          if (!are_equal) {
            break;
          }
        }
      }

skip_type_index_compare:;
      temp_end(temp);
    }
  }

  return are_equal;
}

internal U128
lnk_hash_cv_leaf(Arena               *arena,
                 LNK_CodeViewInput   *input,
                 LNK_LeafHashes      *hashes,
                 LNK_LeafLocType      loc_type,
                 U32                  loc_idx,
                 Rng1U64             *ti_ranges,
                 CV_TypeIndex         curr_ti,
                 CV_Leaf              leaf,
                 CV_TypeIndexInfoList ti_info_list)
{
  // init hasher
  blake3_hasher hasher; blake3_hasher_init(&hasher);

  // hash leaf size
  blake3_hasher_update(&hasher, &leaf.data.size, sizeof leaf.data.size);

  // hash leaf kind
  blake3_hasher_update(&hasher, &leaf.kind, sizeof leaf.kind);

  // hash bytes around indices
  {
    Temp temp = temp_begin(arena);
    String8Array raw_data_arr = cv_get_data_around_type_indices(temp.arena, ti_info_list, leaf.data);
    for (U64 i = 0; i < raw_data_arr.count; ++i) {
      blake3_hasher_update(&hasher, raw_data_arr.v[i].str, raw_data_arr.v[i].size);
    }
    temp_end(temp);
  }

  // mix-in sub leaf hashes
  for (CV_TypeIndexInfo *ti_n = ti_info_list.first; ti_n != 0; ti_n = ti_n->next) {
    CV_TypeIndex sub_ti = *(CV_TypeIndex *) (leaf.data.str + ti_n->offset);

    // is type index complex?
    if (sub_ti >= ti_ranges[ti_n->source].min) {
      // Mostly leaves are laid out as DAG and we can get to sub leaf hash through index lookup,
      // however MASM doesn't follow DAG rule, for example:
      //
      // Engine\Source\Developer\Windows\LiveCoding\Private\External\LC_JumpToSelf.asm
      //  .debug$T (No. 4):
      //    LF_PROCEDURE (0x1000) [0008-0014]
      //     Return type:         3
      //     Call Convention:     Near C
      //     Function Attribs:    NULL
      //     Argumnet Count:      0
      //     Argument List Type:  1001
      //   LF_ARGLIST (0x1001) [0018-001C]
      //     Types 0
      //   LF_LABEL (0x1002) [0020-0024]
      //     $UNDEFINED: E
      // 
      // Note: LF_ARGLIST(0x1001) > LF_PROCEDURE(0x1000)
      // 
      // Luckily we don't have many leaves that break DAG rule and we can skip without 
      // much memory and perf penalty (In Ancient Game we skip 7 leaves) 
      if (sub_ti < curr_ti) {
        LNK_LeafRef sub_leaf_ref = lnk_leaf_ref_from_loc_idx_and_ti(input, loc_type, ti_n->source, loc_idx, sub_ti);

        // query sub hash
        U128 sub_hash = lnk_hash_from_leaf_ref(hashes, sub_leaf_ref);

        // make sure sub hash was computed (:zero_hash_array)
        Assert(!u128_match(sub_hash, u128_zero()));

        // mix-in sub hash
        blake3_hasher_update(&hasher, &sub_hash, sizeof sub_hash);
      } else {
        Temp scratch = scratch_begin(0,0);
        String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
        String8 leaf_info     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) forward refs member type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, ti_n->offset);
        if (loc_type == LNK_LeafLocType_Internal) {
          lnk_error_obj(LNK_Error_InvalidTypeIndex, input->internal_obj_arr+loc_idx, "%S", leaf_info);
        } else if (loc_type == LNK_LeafLocType_External) {
          lnk_error(LNK_Error_InvalidTypeIndex, "%S: %S", input->type_server_path_arr[loc_idx], leaf_info);
        } else {
          InvalidPath;
        }
        scratch_end(scratch);
      }
    }
    // simple indices are stable across compile units 
    else {
      blake3_hasher_update(&hasher, &sub_ti, sizeof sub_ti);
    }
  }

  U128 hash;
  blake3_hasher_finalize(&hasher, (U8 *) &hash, sizeof hash);

  return hash;
}

internal void
lnk_hash_cv_leaf_deep(Arena               *arena,
                      LNK_CodeViewInput   *input,
                      Rng1U64             *ti_ranges,
                      CV_DebugT           *leaves,
                      LNK_LeafHashes      *hashes,
                      LNK_LeafLocType      loc_type,
                      U32                  loc_idx,
                      CV_TypeIndexInfoList ti_info_list,
                      String8              data)
{
  Temp temp = temp_begin(arena);

  struct stack_s {
    struct stack_s      *next;
    CV_TypeIndexInfoList ti_info_list;
    CV_TypeIndexInfo    *ti_info;
    CV_Leaf              leaf;
    String8              data;
    CV_TypeIndex         ti;
    CV_TypeIndexSource   ti_source;
  };

  // set up root frame
  struct stack_s *root_frame = push_array_no_zero(temp.arena, struct stack_s, 1);
  root_frame->next         = 0;
  root_frame->ti_info_list = ti_info_list;
  root_frame->ti_info      = ti_info_list.first;
  root_frame->data         = data;
  root_frame->ti           = 0;
  root_frame->ti_source    = CV_TypeIndexSource_NULL;
  MemoryZeroStruct(&root_frame->leaf);

  U128Array *curr_hashes = hashes->v[loc_type][loc_idx];

  struct stack_s *stack = root_frame;
  while (stack) {
    while (stack->ti_info) {
      CV_TypeIndexInfo *curr_ti_info = stack->ti_info;

      // advance iterator
      stack->ti_info = stack->ti_info->next;

      // get type index info
      CV_TypeIndex *ti_ptr = (CV_TypeIndex *) (stack->data.str + curr_ti_info->offset);

      // is index complex?
      if (*ti_ptr >= ti_ranges[curr_ti_info->source].min) {
        // TODO: handle malformed index
        AssertAlways(*ti_ptr < ti_ranges[curr_ti_info->source].max);
        U64 ti_idx = (*ti_ptr - ti_ranges[curr_ti_info->source].min);

        // was leaf hashed?
        if (MemoryIsZeroStruct(&curr_hashes[curr_ti_info->source].v[ti_idx])) { // :zero_hash_array
          CV_Leaf leaf = cv_debug_t_get_leaf(leaves[curr_ti_info->source], ti_idx);

          // find index offsets
          CV_TypeIndexInfoList sub_ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);

          // do we have sub leaves?
          if (sub_ti_info_list.count) {
            // fill out new frame
            struct stack_s *frame = push_array_no_zero(temp.arena, struct stack_s, 1);
            frame->next         = 0;
            frame->ti_info_list = sub_ti_info_list;
            frame->ti_info      = sub_ti_info_list.first;
            frame->leaf         = leaf;
            frame->data         = leaf.data;
            frame->ti           = *ti_ptr;
            frame->ti_source    = curr_ti_info->source;

            // recurse to sub leaf
            SLLStackPush(stack, frame);
            break;
          } else {
            curr_hashes[curr_ti_info->source].v[ti_idx] = lnk_hash_cv_leaf(temp.arena,
                                                                           input,
                                                                           hashes,
                                                                           loc_type,
                                                                           loc_idx,
                                                                           ti_ranges,
                                                                           CV_TypeIndex_Max,
                                                                           leaf,
                                                                           sub_ti_info_list);
          }
        }
      }
    }

    // no more type indices, pop frame
    if (!stack->ti_info) {

      if (stack != root_frame) {
        // sub leaves are hashed we can now hash parent leaf
        Temp temp2 = temp_begin(temp.arena);
        U64 leaf_idx = stack->ti - ti_ranges[stack->ti_source].min;
        curr_hashes[stack->ti_source].v[leaf_idx] = lnk_hash_cv_leaf(temp2.arena,
                                                                     input,
                                                                     hashes,
                                                                     loc_type,
                                                                     loc_idx,
                                                                     ti_ranges,
                                                                     CV_TypeIndex_Max,
                                                                     stack->leaf,
                                                                     stack->ti_info_list);
        temp_end(temp2);
      }

      SLLStackPop(stack);
    }
  }

  temp_end(temp);
}

internal LNK_LeafBucket *
lnk_leaf_hash_table_insert_or_update(LNK_LeafHashTable *leaf_ht, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, U128 new_hash, LNK_LeafBucket *new_bucket)
{
  LNK_LeafBucket *result                 = 0;
  B32             is_inserted_or_updated = 0;

  U64 best_idx = u128_mod64(new_hash, leaf_ht->cap);
  U64 idx      = best_idx;

  do {
    retry:;
    LNK_LeafBucket *curr_bucket = leaf_ht->bucket_arr[idx];

    if (curr_bucket == 0) {
      LNK_LeafBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&leaf_ht->bucket_arr[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {
        // success, bucket was inserted
        is_inserted_or_updated = 1;
        break;
      }

      // another thread took the bucket...
      goto retry;
    } else if (lnk_match_leaf_ref(input, hashes, curr_bucket->leaf_ref, new_bucket->leaf_ref)) {
      int leaf_cmp = lnk_leaf_ref_compare(curr_bucket->leaf_ref, new_bucket->leaf_ref);

      if (leaf_cmp <= 0) {
        // are we inserting bucket that was already inserterd?
        Assert(leaf_cmp < 0);

        result = new_bucket;

        is_inserted_or_updated = 1;

        // don't need to update, more recent leaf is in the bucket
        break;
      }

      LNK_LeafBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&leaf_ht->bucket_arr[idx], new_bucket, curr_bucket);
      if (compare_bucket == curr_bucket) {
        result = compare_bucket;

        is_inserted_or_updated = 1;
        break;
      }

      // another thread took the bucket...
      goto retry;
    }

    // advance
    idx = (idx + 1) % leaf_ht->cap;
  } while (idx != best_idx);

  Assert(is_inserted_or_updated);

  return result;
}

internal LNK_LeafBucket *
lnk_leaf_hash_table_search(LNK_LeafHashTable *ht, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef leaf_ref)
{
  LNK_LeafBucket *match = 0;

  U128 hash            = lnk_hash_from_leaf_ref(hashes, leaf_ref);
  U64  best_bucket_idx = u128_mod64(hash, ht->cap);
  U64  bucket_idx      = best_bucket_idx;

  do {
    LNK_LeafBucket *bucket = ht->bucket_arr[bucket_idx];

    if (bucket == 0) {
      break;
    }

    if (lnk_match_leaf_ref(input, hashes, bucket->leaf_ref, leaf_ref)) {
      match = bucket;
      break;
    }

    bucket_idx = (bucket_idx + 1) % ht->cap;
  } while (bucket_idx != best_bucket_idx);

  return match;
}

////////////////////////////////

internal
THREAD_POOL_TASK_FUNC(lnk_count_per_source_leaf_task)
{
  ProfBeginFunction();

  LNK_CountPerSourceLeafTask *task            = raw_task;
  LNK_LeafRangeList           leaf_range_list = task->leaf_ranges_per_task[task_id];

  for (LNK_LeafRange *leaf_range = leaf_range_list.first; leaf_range != 0; leaf_range = leaf_range->next) {
    CV_DebugT debug_t = *leaf_range->debug_t;
    for (U64 leaf_idx = leaf_range->range.min; leaf_idx < leaf_range->range.max; ++leaf_idx) {
      CV_LeafHeader      *leaf_header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
      CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(leaf_header->kind);
      task->count_arr_arr[leaf_source][task_id] += 1;
    }
  }

  ProfEnd();
}

internal void
lnk_cv_debug_t_count_leaves_per_source(TP_Context *tp, U64 count, CV_DebugT *debug_t_arr, U64 per_source_count_arr[CV_TypeIndexSource_COUNT])
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  ProfBegin("Compute Per Task Ranges");
  U64                per_task_leaf_count  = 10000;
  LNK_LeafRangeList *leaf_ranges_per_task = push_array(scratch.arena, LNK_LeafRangeList, tp->worker_count);
  for (U64 i = 0, task_weight = 0, task_id = 0; i < count; ++i) {
    CV_DebugT *debug_t = &debug_t_arr[i];
    for (U64 k = 0; k < debug_t->count; k += per_task_leaf_count) {
      U64 cap = per_task_leaf_count - task_weight;

      LNK_LeafRange *leaf_range = push_array(scratch.arena, LNK_LeafRange, 1);
      leaf_range->range         = rng_1u64(k, Min(k + cap, debug_t->count));
      leaf_range->debug_t       = debug_t;

      LNK_LeafRangeList *list = &leaf_ranges_per_task[task_id];
      SLLQueuePush(list->first, list->last, leaf_range);
      ++list->count;

      task_weight += dim_1u64(leaf_range->range);
      if (task_weight >= per_task_leaf_count) {
        task_id     = (task_id + 1) % tp->worker_count;
        task_weight = 0;
      }
    }
  }
  ProfEnd();


  LNK_CountPerSourceLeafTask task;
  task.leaf_ranges_per_task = leaf_ranges_per_task;
  task.count_arr_arr        = push_matrix_u64(scratch.arena, CV_TypeIndexSource_COUNT, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_count_per_source_leaf_task, &task);

  for (U64 i = 0; i < CV_TypeIndexSource_COUNT; ++i) {
    per_source_count_arr[i] += sum_array_u64(tp->worker_count, task.count_arr_arr[i]);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_task)
{
  ProfBeginFunction();

  U64                 obj_idx = task_id;
  LNK_LeafHasherTask *task    = raw_task;

  Arena     *fixed_arena = task->fixed_arenas[worker_id];
  CV_DebugT  debug_t     = task->debug_t_arr[obj_idx];
  U128Array  out_hashes  = task->hashes->v[LNK_LeafLocType_Internal][obj_idx][CV_TypeIndexSource_TPI];

  Rng1U64 ti_ranges[CV_TypeIndexSource_COUNT];
  for (U64 ti_source = 0; ti_source < ArrayCount(ti_ranges); ++ti_source) {
    ti_ranges[ti_source] = rng_1u64(task->input->pch_arr[obj_idx].ti_lo, task->input->pch_arr[obj_idx].ti_hi + debug_t.count);
  }

  for (U64 leaf_idx = 0; leaf_idx < debug_t.count; ++leaf_idx) {
    Temp temp = temp_begin(fixed_arena);

    // :debug_zero_hash_assert make sure we don't write same hash more than once
    //Assert(MemoryIsZeroStruct(&out_hash_arr.v[leaf_idx])); 

    CV_TypeIndex         curr_ti      = lnk_type_index_from_leaf_ref(task->input, lnk_leaf_ref(obj_idx, leaf_idx));
    CV_Leaf              leaf         = cv_debug_t_get_leaf(debug_t, leaf_idx);
    CV_TypeIndexInfoList ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);

    out_hashes.v[leaf_idx] = lnk_hash_cv_leaf(temp.arena,
                                              task->input,
                                              task->hashes,
                                              LNK_LeafLocType_Internal,
                                              obj_idx,
                                              ti_ranges,
                                              curr_ti,
                                              leaf,
                                              ti_info_list);

    temp_end(temp);
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_type_server_leaves_task)
{
  ProfBeginFunction();

  LNK_LeafHasherTask *task    = raw_task;
  U64                 obj_idx = task_id;

  LNK_CodeViewInput *input  = task->input;
  LNK_LeafHashes    *hashes = task->hashes;

  CV_SymbolListArray parsed_symbols = input->external_parsed_symbols[obj_idx];
  CV_DebugS          debug_s        = input->external_debug_s_arr[obj_idx];
  U64                ts_idx         = input->external_obj_to_ts_idx_arr[obj_idx];
  CV_DebugT         *leaves         = input->external_leaves[ts_idx];
  Rng1U64           *ti_ranges      = input->external_ti_ranges[ts_idx];

  // hash leaves referenced in symbols
  for (U64 i = 0; i < parsed_symbols.count; ++i) {
    CV_SymbolList symbol_list = parsed_symbols.v[i];
    for (CV_SymbolNode *symnode = symbol_list.first; symnode != 0; symnode = symnode->next) {
      Temp temp = temp_begin(task->fixed_arenas[worker_id]);
      CV_TypeIndexInfoList ti_info_list = cv_get_symbol_type_index_offsets(temp.arena, symnode->data.kind, symnode->data.data);
      lnk_hash_cv_leaf_deep(temp.arena, task->input, ti_ranges, leaves, hashes, LNK_LeafLocType_External, ts_idx, ti_info_list, symnode->data.data);
      temp_end(temp);
    }
  }
  
  // hash leaves referenced in inlinees
  String8List inline_data_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_InlineeLines);
  for (String8Node *inline_data_node = inline_data_list.first; inline_data_node != 0; inline_data_node = inline_data_node->next) {
    Temp temp = temp_begin(task->fixed_arenas[worker_id]);
    CV_TypeIndexInfoList ti_info_list = cv_get_inlinee_type_index_offsets(temp.arena, inline_data_node->string);
    lnk_hash_cv_leaf_deep(temp.arena, task->input, ti_ranges, leaves, hashes, LNK_LeafLocType_External, ts_idx, ti_info_list, inline_data_node->string);
    temp_end(temp);
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_dedup_internal_task)
{
  LNK_LeafDedupInternal *task    = raw_task;
  U64                    obj_idx = task_id;
  CV_DebugT              debug_t = task->debug_t_arr[obj_idx];

  ProfBeginDynamic("Leaf Dedup Task 0x%X [Leaf Count %u]", obj_idx, task->debug_t_arr[obj_idx].count);
  
  LNK_LeafBucket *bucket = 0;
  for (U64 leaf_idx = 0; leaf_idx < debug_t.count; ++leaf_idx) {
    CV_LeafHeader     *leaf_header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
    CV_TypeIndexSource ti_source   = cv_type_index_source_from_leaf_kind(leaf_header->kind);
    LNK_LeafHashTable *leaf_ht     = &task->leaf_ht_arr[ti_source];

    LNK_LeafRef leaf_ref  = lnk_obj_leaf_ref(obj_idx, leaf_idx);
    U128        leaf_hash = lnk_hash_from_leaf_ref(task->hashes, leaf_ref);

    if (bucket == 0) {
      bucket = push_array_no_zero(arena, LNK_LeafBucket, 1);
    }
    bucket->leaf_ref = leaf_ref;

    LNK_LeafBucket *inserted_or_updated = lnk_leaf_hash_table_insert_or_update(leaf_ht, task->input, task->hashes, leaf_hash, bucket);

    if (inserted_or_updated != bucket) {
      bucket = 0;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_dedup_external_task)
{
  ProfBeginFunction();

  LNK_LeafDedupExternal *task   = raw_task;
  U64                    ts_idx = task_id;

  LNK_CodeViewInput *input      = task->input;
  LNK_LeafHashTable *leaf_ht    = &task->leaf_ht_arr[task->dedup_ti_source];
  U128Array          hashes     = task->hashes->external_hashes[ts_idx][task->dedup_ti_source];
  U64                leaf_count = dim_1u64(input->external_ti_ranges[ts_idx][task->dedup_ti_source]);

  LNK_LeafBucket *bucket = 0;

  for (U64 leaf_idx = 0; leaf_idx < leaf_count; ++leaf_idx) {
    if (!MemoryIsZeroStruct(&hashes.v[leaf_idx])) { // :zero_hash_check
      LNK_LeafRef leaf_ref  = lnk_ts_leaf_ref(task->dedup_ti_source, ts_idx, leaf_idx);
      U128        leaf_hash = lnk_hash_from_leaf_ref(task->hashes, leaf_ref);

      if (bucket == 0) {
        bucket = push_array_no_zero(arena, LNK_LeafBucket, 1);
      }
      bucket->leaf_ref = leaf_ref;

      LNK_LeafBucket *inserted_or_updated = lnk_leaf_hash_table_insert_or_update(leaf_ht, task->input, task->hashes, leaf_hash, bucket);

      if (inserted_or_updated != bucket) {
        bucket = 0;
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_present_buckets_task)
{
  ProfBeginFunction();
  LNK_GetPresentBucketsTask *task = raw_task;
  for (U64 bucket_idx = task->range_arr[task_id].min; bucket_idx < task->range_arr[task_id].max; ++bucket_idx) {
    if (task->ht->bucket_arr[bucket_idx] != 0) {
      task->count_arr[task_id] += 1;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_get_present_buckets_task)
{
  ProfBeginFunction();

  LNK_GetPresentBucketsTask *task = raw_task;

  Rng1U64            range  = task->range_arr[task_id];
  U64                cursor = task->offset_arr[task_id];
  LNK_LeafHashTable *ht     = task->ht;

  for (U64 bucket_idx = range.min; bucket_idx < range.max; ++bucket_idx) {
    if (ht->bucket_arr[bucket_idx]) {
      task->result.v[cursor++] = ht->bucket_arr[bucket_idx];
    }
  }

  ProfEnd();
}

internal LNK_LeafBucketArray
lnk_present_bucket_array_from_leaf_hash_table(TP_Context *tp, Arena *arena, LNK_LeafHashTable *ht)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_GetPresentBucketsTask task = {0};
  task.ht                        = ht;
  task.count_arr                 = push_array(scratch.arena, U64, tp->worker_count);
  task.range_arr                 = tp_divide_work(scratch.arena, ht->cap, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_count_present_buckets_task, &task);

  LNK_LeafBucketArray result;
  result.count = sum_array_u64(tp->worker_count, task.count_arr);
  result.v     = push_array_no_zero(arena, LNK_LeafBucket *, result.count);

  task.result     = result;
  task.offset_arr = offsets_from_counts_array_u64(scratch.arena, task.count_arr, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_get_present_buckets_task, &task);

  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_ref_histo_task)
{
  ProfBeginFunction();

  LNK_LeafRadixSortTask *task       = raw_task;
  Rng1U64                range      = task->ranges[task_id];
  U32                   *counts_ptr = task->counts_arr[task_id];

  U32 loc_idx_bit_count_0 = task->loc_idx_bit_count_0;
  U32 loc_idx_bit_count_1 = task->loc_idx_bit_count_1;
  U32 loc_idx_bit_count_2 = task->loc_idx_bit_count_2;

  MemoryZeroTyped(task->counts_arr[task_id], task->counts_max);

  switch (task->pass_idx) {
  case 0: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit0 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 10, 0);
      ++counts_ptr[leaf_digit0];
    }
  } break;
  case 1: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit1 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 11, 10);
      ++counts_ptr[leaf_digit1];
    }
  } break;
  case 2: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit2 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 11, 21 - 1); // don't take into account IPI flag
      ++counts_ptr[leaf_digit2];
    }
  } break;

  case 3: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit0 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_0, 0);
      ++counts_ptr[digit0];
    }
  } break;
  case 4: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit1 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_1, loc_idx_bit_count_0);
      ++counts_ptr[digit1];
    }
  } break;
  case 5: {
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit2 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_2, loc_idx_bit_count_0 + loc_idx_bit_count_1);

      U64 loc_bit = !!(bucket->leaf_ref.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal);
      digit2 |= loc_bit << loc_idx_bit_count_2;

      ++counts_ptr[digit2];
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

  LNK_LeafRadixSortTask *task                = raw_task;
  Rng1U64                range               = task->ranges[task_id];
  U32                   *counts_ptr          = task->counts_arr[task_id];
  U32                    loc_idx_bit_count_0 = task->loc_idx_bit_count_0;
  U32                    loc_idx_bit_count_1 = task->loc_idx_bit_count_1;
  U32                    loc_idx_bit_count_2 = task->loc_idx_bit_count_2;

  switch (task->pass_idx) {
  //
  // Sort items on leaf index
  //
  case 0: {
    ProfBegin("Leaf Sort Low");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit0 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 10, 0);
      task->dst[counts_ptr[leaf_digit0]++] = bucket;
    }
    ProfEnd();
  } break;
  case 1: {
    ProfBegin("Leaf Sort Mid");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit1 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 11, 10);
      task->dst[counts_ptr[leaf_digit1]++] = bucket;
    }
    ProfEnd();
  } break;
  case 2: {
    ProfBegin("Leaf Sort High");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 leaf_digit2 = BitExtract(bucket->leaf_ref.enc_leaf_idx, 11, 21 - 1); // don't take into account IPI flag
      task->dst[counts_ptr[leaf_digit2]++] = bucket;
    }
    ProfEnd();
  } break;

  //
  // Sort items on obj and type server index
  //
  case 3: {
    ProfBegin("Loc Sort Low");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit0 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_0, 0);
      task->dst[counts_ptr[digit0]++] = bucket;
    }
    ProfEnd();
  } break;
  case 4: {
    ProfBegin("Loc Sort Mid");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit1 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_1, loc_idx_bit_count_0);
      task->dst[counts_ptr[digit1]++] = bucket;
    }
    ProfEnd();
  } break;
  case 5: {
    ProfBegin("Loc Sort High");
    for (U64 i = range.min; i < range.max; ++i) {
      LNK_LeafBucket *bucket = task->src[i];
      U64 digit2 = BitExtract(bucket->leaf_ref.enc_loc_idx, loc_idx_bit_count_2, loc_idx_bit_count_0 + loc_idx_bit_count_1);

      U64 loc_bit = !!(bucket->leaf_ref.enc_loc_idx & LNK_LeafRefFlag_LocIdxExternal);
      digit2 |= loc_bit << loc_idx_bit_count_2;

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
lnk_leaf_bucket_array_sort(TP_Context *tp, LNK_LeafBucketArray arr, U64 obj_count, U64 type_server_count)
{
  Temp scratch = scratch_begin(0,0);

#if PROFILE_TELEMETRY
  String8 leaf_count_string        = str8_from_count(scratch.arena, arr.count);
  String8 obj_count_string         = str8_from_count(scratch.arena, obj_count);
  String8 type_server_count_string = str8_from_count(scratch.arena, type_server_count);
  ProfBeginDynamic("Leaf Sort [Leaf Count: %.*s, Obj Count: %.*s, Type Server Count: %.*s]", str8_varg(leaf_count_string), str8_varg(obj_count_string), str8_varg(type_server_count_string));
#endif

  if (arr.count > 140000) {
    ProfBegin("Radix");

    U32 loc_idx_max_bits = 32 - clz32(Max(obj_count, type_server_count));

    LNK_LeafRadixSortTask task = {0};
    task.loc_idx_bit_count_0   = Clamp(0, (S32)loc_idx_max_bits - 21, 11);
    task.loc_idx_bit_count_1   = Clamp(0, (S32)loc_idx_max_bits - 10, 11);
    task.loc_idx_bit_count_2   = Clamp(0, (S32)loc_idx_max_bits,      10);
    task.counts_max            = (1 << 11);
    task.loc_idx_max           = arr.count;
    task.ranges                = tp_divide_work(scratch.arena, arr.count, tp->worker_count);
    task.dst                   = push_array_no_zero(scratch.arena, LNK_LeafBucket *, arr.count);
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
        for (U64 digit_idx = 0; digit_idx < task.counts_max; ++digit_idx) {
          for (U64 task_id = 0; task_id < tp->worker_count; ++task_id) {
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
      Swap(LNK_LeafBucket **, task.src, task.dst);
      ProfEnd();

      ProfEnd();
    }

    if (task.src != arr.v) {
      MemoryCopyTyped(arr.v, task.dst, arr.count);
    }

#if 0
    for (U64 i = 1; i < arr.count; ++i) {
      AssertAlways(arr.v[i-1]->leaf_ref.enc_loc_idx <= arr.v[i]->leaf_ref.enc_loc_idx);
      if (arr.v[i-1]->leaf_ref.enc_loc_idx == arr.v[i]->leaf_ref.enc_loc_idx) {
        AssertAlways(arr.v[i-1]->leaf_ref.enc_leaf_idx <= arr.v[i]->leaf_ref.enc_leaf_idx);
      }
    }
#endif

    ProfEnd();
  } else {
    ProfBegin("Radsort");
    radsort(arr.v, arr.count, lnk_leaf_ref_is_before);
    ProfEnd();
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_assign_type_indices_task)
{
  LNK_AssignTypeIndicesTask *task  = raw_task;
  Rng1U64                    range = task->range_arr[task_id];
  for (U64 i = range.min; i < range.max; ++i) {
    LNK_LeafBucket *bucket = task->bucket_arr.v[i];
    bucket->type_index = task->min_type_index + i;
  }
}

internal void
lnk_assign_type_indices(TP_Context *tp, LNK_LeafBucketArray bucket_arr, CV_TypeIndex min_type_index)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  LNK_AssignTypeIndicesTask task;
  task.range_arr      = tp_divide_work(scratch.arena, bucket_arr.count, tp->worker_count);
  task.bucket_arr     = bucket_arr;
  task.min_type_index = min_type_index;
  tp_for_parallel(tp, 0, tp->worker_count, lnk_assign_type_indices_task, &task);

  ProfEnd();
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_symbols_task)
{
  LNK_PatchSymbolTypesTask *task         = raw_task;
  Arena                    *fixed_arena  = task->arena_arr[worker_id];
  LNK_CodeViewSymbolsInput  symbol_input = task->input->symbol_inputs[task_id];

  LNK_LeafLocType loc_type = lnk_loc_type_from_obj_idx(task->input, symbol_input.obj_idx);
  U64             loc_idx  = lnk_loc_idx_from_obj_idx(task->input, symbol_input.obj_idx);

  CV_TypeIndex ti_lo_arr[CV_TypeIndexSource_COUNT];
  ti_lo_arr[CV_TypeIndexSource_NULL] = lnk_ti_lo_from_loc(task->input, loc_type, loc_idx, CV_TypeIndexSource_NULL);
  ti_lo_arr[CV_TypeIndexSource_TPI ] = lnk_ti_lo_from_loc(task->input, loc_type, loc_idx, CV_TypeIndexSource_TPI);
  ti_lo_arr[CV_TypeIndexSource_IPI ] = lnk_ti_lo_from_loc(task->input, loc_type, loc_idx, CV_TypeIndexSource_IPI);

  for (CV_SymbolNode *symnode = symbol_input.symbol_list->first; symnode != 0; symnode = symnode->next) {
    Temp temp = temp_begin(fixed_arena);

    // find type index offsets in symbol
    CV_TypeIndexInfoList ti_list = cv_get_symbol_type_index_offsets(temp.arena, symnode->data.kind, symnode->data.data);
    
    // overwrite type indices in symbol
    for (CV_TypeIndexInfo *ti_info = ti_list.first; ti_info != 0; ti_info = ti_info->next) {
      CV_TypeIndex *ti_ptr = (CV_TypeIndex *) (symnode->data.data.str + ti_info->offset);
      if (*ti_ptr >= ti_lo_arr[ti_info->source]) {
        LNK_LeafHashTable *leaf_ht     = &task->leaf_ht_arr[ti_info->source];
        LNK_LeafRef        leaf_ref    = lnk_leaf_ref_from_loc_idx_and_ti(task->input, loc_type, ti_info->source, loc_idx, *ti_ptr);
        LNK_LeafBucket    *leaf_bucket = lnk_leaf_hash_table_search(leaf_ht, task->input, task->hashes, leaf_ref);

        // we overwrite section memory directly
        *ti_ptr = leaf_bucket->type_index;
      }
    }
    
    temp_end(temp);
  }
}

internal void
lnk_patch_symbols(TP_Context         *tp,
                  LNK_CodeViewInput  *input,
                  LNK_LeafHashes     *hashes,
                  LNK_LeafHashTable  *leaf_ht_arr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  U64 max_ti_list_size = sizeof(CV_TypeIndexInfo) * (max_U16 / sizeof(CV_TypeIndex));

  LNK_PatchSymbolTypesTask task = {0};
  task.input           = input;
  task.hashes          = hashes;
  task.leaf_ht_arr     = leaf_ht_arr;
  task.arena_arr       = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, max_ti_list_size, max_ti_list_size);
  tp_for_parallel(tp, 0, input->total_symbol_input_count, lnk_patch_symbols_task, &task);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_inlines_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  LNK_PatchInlinesTask *task = raw_task;

  U64             loc_idx          = lnk_loc_idx_from_obj_idx(task->input, task_id);
  LNK_LeafLocType loc_type         = lnk_loc_type_from_obj_idx(task->input, task_id);
  String8List     inline_data_list = cv_sub_section_from_debug_s(task->debug_s_arr[task_id], CV_C13SubSectionKind_InlineeLines);

  for (String8Node *inline_data_node = inline_data_list.first; inline_data_node != 0; inline_data_node = inline_data_node->next) {
    Temp temp = temp_begin(scratch.arena);

    // get indices offsets
    CV_TypeIndexInfoList ti_info_list = cv_get_inlinee_type_index_offsets(temp.arena, inline_data_node->string);

    for (CV_TypeIndexInfo *ti_info = ti_info_list.first; ti_info != 0; ti_info = ti_info->next) {
      CV_TypeIndex *ti_ptr = (CV_TypeIndex *) (inline_data_node->string.str + ti_info->offset);
      CV_TypeIndex  ti_lo  = lnk_ti_lo_from_loc(task->input, loc_type, loc_idx, ti_info->source);
      if (*ti_ptr >= ti_lo) {
        LNK_LeafRef     leaf_ref    = lnk_leaf_ref_from_loc_idx_and_ti(task->input, loc_type, ti_info->source, loc_idx, *ti_ptr);
        LNK_LeafBucket *leaf_bucket = lnk_leaf_hash_table_search(&task->leaf_ht_arr[ti_info->source], task->input, task->hashes, leaf_ref);
        
        // patch index
        *ti_ptr = leaf_bucket->type_index;
      }
    }

    temp_end(temp);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_patch_inlines(TP_Context         *tp,
                  LNK_CodeViewInput  *input,
                  LNK_LeafHashes     *hashes,
                  LNK_LeafHashTable  *leaf_ht_arr,
                  U64                 obj_count,
                  CV_DebugS          *debug_s_arr)
{
  ProfBeginFunction();
  
  LNK_PatchInlinesTask task = {0};
  task.input       = input;
  task.hashes      = hashes;
  task.leaf_ht_arr = leaf_ht_arr;
  task.debug_s_arr = debug_s_arr;
  tp_for_parallel(tp, 0, obj_count, lnk_patch_inlines_task, &task);

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_leaves_task)
{
  ProfBeginFunction();

  LNK_PatchLeavesTask *task  = raw_task;
  Rng1U64              range = task->range_arr[task_id];

  for (U64 bucket_idx = range.min; bucket_idx < range.max; ++bucket_idx) {
    Temp temp = temp_begin(task->fixed_arena_arr[task_id]);

    LNK_LeafBucket *bucket = task->bucket_arr[bucket_idx];

    U64             loc_idx  = bucket->leaf_ref.enc_loc_idx & ~LNK_LeafRefFlag_LocIdxExternal;
    LNK_LeafLocType loc_type = lnk_loc_type_from_leaf_ref(bucket->leaf_ref);
    CV_TypeIndex    ti_lo    = lnk_ti_lo_from_leaf_ref(task->input, bucket->leaf_ref);
    String8         raw_leaf = lnk_data_from_leaf_ref(task->input, bucket->leaf_ref);
    CV_Leaf         leaf     = cv_leaf_from_string(raw_leaf);

    // get type indices offsets
    CV_TypeIndexInfoList ti_info_list = cv_get_leaf_type_index_offsets(temp.arena, leaf.kind, leaf.data);
    for (CV_TypeIndexInfo *ti_info = ti_info_list.first; ti_info != 0; ti_info = ti_info->next) {
      CV_TypeIndex *ti_ptr = (CV_TypeIndex *) (leaf.data.str + ti_info->offset);
      if (*ti_ptr >= ti_lo) {
        LNK_LeafHashTable *leaf_ht         = &task->leaf_ht_arr[ti_info->source];
        LNK_LeafRef        sub_leaf_ref    = lnk_leaf_ref_from_loc_idx_and_ti(task->input, loc_type, ti_info->source, loc_idx, *ti_ptr);
        LNK_LeafBucket    *sub_leaf_bucket = lnk_leaf_hash_table_search(leaf_ht, task->input, task->hashes, sub_leaf_ref);

         // patch index
        *ti_ptr = sub_leaf_bucket->type_index;
      }
    }

    temp_end(temp);
  }

  ProfEnd();
}

internal void
lnk_patch_leaves(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafHashTable *leaf_ht_arr, LNK_LeafBucketArray bucket_arr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  LNK_PatchLeavesTask task;
  task.input           = input;
  task.hashes          = hashes;
  task.leaf_ht_arr     = leaf_ht_arr;
  task.bucket_arr      = bucket_arr.v;
  task.range_arr       = tp_divide_work(scratch.arena, bucket_arr.count, tp->worker_count);
  task.fixed_arena_arr = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, MB(1), MB(1));
  tp_for_parallel(tp, 0, tp->worker_count, lnk_patch_leaves_task, &task);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_unbucket_raw_leaves_task)
{
  LNK_UnbucketRawLeavesTask *task  = raw_task;
  Rng1U64                    range = task->range_arr[task_id];
  for (U64 i = range.min; i < range.max; ++i) {
    String8 raw_leaf = lnk_data_from_leaf_ref(task->input, task->bucket_arr[i]->leaf_ref);
    task->raw_leaf_arr[i] = raw_leaf.str;
  }
}

internal CV_DebugT
lnk_unbucket_leaf_array(TP_Context *tp, Arena *arena, LNK_CodeViewInput *input, LNK_LeafBucketArray bucket_arr)
{
  ProfBeginDynamic("Unbucket Leaves [Count %llu]", bucket_arr.count);
  Temp scratch = scratch_begin(&arena, 1);

  LNK_UnbucketRawLeavesTask task = {0};
  task.input        = input;
  task.bucket_arr   = bucket_arr.v;
  task.raw_leaf_arr = push_array_no_zero(arena, U8 *, bucket_arr.count);
  task.range_arr    = tp_divide_work(scratch.arena, bucket_arr.count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_unbucket_raw_leaves_task, &task);

  CV_DebugT debug_t = {0};
  debug_t.count = bucket_arr.count;
  debug_t.v     = task.raw_leaf_arr;

  scratch_end(scratch);
  ProfEnd();
  return debug_t;
}

internal
THREAD_POOL_TASK_FUNC(lnk_post_process_cv_symbols_task)
{
  LNK_PostProcessCvSymbolsTask *task         = raw_task;
  LNK_CodeViewSymbolsInput      symbol_input = task->symbol_inputs[task_id];

  for (CV_SymbolNode *symnode = symbol_input.symbol_list->first; symnode != 0; symnode = symnode->next) {
    CV_Symbol *symbol = &symnode->data;

    if (symbol->kind == CV_SymKind_LPROC32_ID || symbol->kind == CV_SymKind_GPROC32_ID || symbol->kind == CV_SymKind_LPROC32_DPC) {
      CV_SymProc32 *proc32 = (CV_SymProc32 *) symbol->data.str;
      if (proc32->itype >= task->ipi_min_type_index) {
        if ((proc32->itype - task->ipi_min_type_index) < task->ipi_types.count) {
          U64     leaf_idx = proc32->itype - task->ipi_min_type_index;
          CV_Leaf leaf     = cv_debug_t_get_leaf(task->ipi_types, leaf_idx);

          if (leaf.kind == CV_LeafKind_FUNC_ID) {
            if (leaf.data.size >= sizeof(CV_LeafFuncId)) {
              proc32->itype = ((CV_LeafFuncId *) leaf.data.str)->itype;
            } else {
              Assert(!"TODO: error handle corrupt leaf");
            }
          } else if (leaf.kind == CV_LeafKind_MFUNC_ID) {
            if (leaf.data.size >= sizeof(CV_LeafMFuncId)) {
              proc32->itype = ((CV_LeafMFuncId *) leaf.data.str)->itype;
            } else {
              Assert(!"TODO: error handle corrupt leaf");
            }
          } else {
            Assert(!"TODO: erorr handle unexpected leaf type");
          }
        } else {
          Assert("TODO: error handle corrupted type index");
        }
      } else {
        // TODO: in some cases destructors don't have a type, need a repro
      }
    }

    // convert symbol to final type
    switch (symbol->kind) {
    case CV_SymKind_LPROC32_ID:     symbol->kind = CV_SymKind_LPROC32;     break;
    case CV_SymKind_GPROC32_ID:     symbol->kind = CV_SymKind_GPROC32;     break;
    case CV_SymKind_LPROC32_DPC_ID: symbol->kind = CV_SymKind_LPROC32_DPC; break;
    case CV_SymKind_LPROCMIPS_ID:   symbol->kind = CV_SymKind_LPROCMIPS;   break;
    case CV_SymKind_GPROCMIPS_ID:   symbol->kind = CV_SymKind_GPROCMIPS;   break;
    case CV_SymKind_LPROCIA64_ID:   symbol->kind = CV_SymKind_LPROCIA64;   break;
    case CV_SymKind_GPROCIA64_ID:   symbol->kind = CV_SymKind_GPROCIA64;   break;
    case CV_SymKind_PROC_ID_END:    symbol->kind = CV_SymKind_END;         break;
    }
  }
}

internal CV_DebugT *
lnk_import_types(TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input)
{
  ProfBegin("Import Types");

  ProfBegin("Hash Leaves");
  LNK_LeafHashes *hashes = push_array(tp_temp->v[0], LNK_LeafHashes, 1);
  {
    Temp scratch = scratch_begin(tp_temp->v, tp_temp->count);

    // push internal hash arrays
    //
    // TPI and IPI leaves in .debug$T are stored in one array (we don't move them
    // to respective arrays before this point to save on memory move)
    ProfBegin("Push Internal Hash Arrays");
    hashes->internal_hashes = push_array_no_zero(tp_temp->v[0], U128Array *, input->internal_count);
    for (U64 obj_idx = 0; obj_idx < input->internal_count; ++obj_idx) {
      CV_DebugT debug_t = input->merged_debug_t_p_arr[obj_idx];

      U128Array arr = {0};
      arr.count     = debug_t.count;
      arr.v         = push_array_no_zero(tp_temp->v[0], U128, debug_t.count);
      // :debug_zero_hash_assert
#if BUILD_DEBUG
      MemoryZeroTyped(arr.v, arr.count);
#endif

      hashes->internal_hashes[obj_idx] = push_array(tp_temp->v[0], U128Array, CV_TypeIndexSource_COUNT);
      for (U64 ti_source = 0; ti_source < CV_TypeIndexSource_COUNT; ++ti_source) {
        hashes->internal_hashes[obj_idx][ti_source] = arr;
      }
    }
    ProfEnd();

    // push external hash arrays
    ProfBegin("Push External Hash Arrays");
    hashes->external_hashes = push_array_no_zero(tp_temp->v[0], U128Array *, input->type_server_count);
    for (U64 ts_idx = 0; ts_idx < input->type_server_count; ++ts_idx) {
      hashes->external_hashes[ts_idx] = push_array_no_zero(tp_temp->v[0], U128Array, CV_TypeIndexSource_COUNT);
      for (U64 ti_source = 0; ti_source < CV_TypeIndexSource_COUNT; ++ti_source) {
        U64 leaf_count = dim_1u64(input->external_ti_ranges[ts_idx][ti_source]);
        hashes->external_hashes[ts_idx][ti_source].count = leaf_count;
        hashes->external_hashes[ts_idx][ti_source].v     = push_array(tp_temp->v[0], U128, leaf_count); // :zero_hash_check
      }
    }
    ProfEnd();

    LNK_LeafHasherTask task = {0};
    task.input        = input;
    task.hashes       = hashes;
    task.fixed_arenas = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, MB(1), MB(1));

    // hash .debug$P first so we can mix in hashes for precompiled sub leaves when hashing leaves in .debug$T
    ProfBeginDynamic("Hash .debug$P [Count: %llu]", input->internal_count);
    task.debug_t_arr = input->internal_debug_p_arr;
    tp_for_parallel(tp, 0, input->internal_count, lnk_hash_debug_t_task, &task);
    ProfEnd();

#if PROFILE_TELEMETRY
    String8 count_string = str8_from_count(scratch.arena, input->internal_count);
    ProfBegin("Hash .debug$T [Count: %.*s]", str8_varg(count_string));
#endif
    task.debug_t_arr = input->internal_debug_t_arr;
    tp_for_parallel(tp, 0, input->internal_count, lnk_hash_debug_t_task, &task);
    ProfEnd();

    ProfBegin("Hash Type Server Leaves [Count: %.*s]", str8_varg(count_string));
    tp_for_parallel(tp, 0, input->external_count, lnk_hash_type_server_leaves_task, &task);
    ProfEnd();

    scratch_end(scratch);
  }
  ProfEnd();

  ProfBegin("Leaf Hash Table Init");
  LNK_LeafHashTable leaf_ht_arr[CV_TypeIndexSource_COUNT] = { 0 };
  U64 internal_per_source_count[CV_TypeIndexSource_COUNT] = { 0 };
  U64 external_per_source_count[CV_TypeIndexSource_COUNT] = { 0 };
  {
    // count internal leaves
    lnk_cv_debug_t_count_leaves_per_source(tp, input->internal_count, input->internal_debug_p_arr, internal_per_source_count);
    lnk_cv_debug_t_count_leaves_per_source(tp, input->internal_count, input->internal_debug_t_arr, internal_per_source_count);

    // count external leaves
    for (U64 ts_idx = 0; ts_idx < input->type_server_count; ++ts_idx) {
      for (U64 ti_source = 0; ti_source < CV_TypeIndexSource_COUNT; ++ti_source) {
        external_per_source_count[ti_source] += dim_1u64(input->external_ti_ranges[ts_idx][ti_source]);
      }
    }

    // push buckets per source
    for (U64 ti_source = 0; ti_source < CV_TypeIndexSource_COUNT; ++ti_source) {
      U64 bucket_cap = 0;
      bucket_cap += internal_per_source_count[ti_source];
      bucket_cap += external_per_source_count[ti_source];
      bucket_cap  = (U64) ((F64) bucket_cap * 1.3);

      #if PROFILE_TELEMETRY
      tmMessage(0, TMMF_ICON_NOTE, "%.*s Bucket Count: %llu", str8_varg(cv_string_from_type_index_source(ti_source)), bucket_cap);
      #endif

      leaf_ht_arr[ti_source].cap        = bucket_cap;
      leaf_ht_arr[ti_source].bucket_arr = push_array(tp_temp->v[0], LNK_LeafBucket *, bucket_cap);
    }
  }
  ProfEnd();

#if PROFILE_TELEMETRY
  String8 obj_count_string = str8_from_count(tp_temp->v[0], input->internal_count);
  String8 tpi_count_string = str8_from_count(tp_temp->v[0], internal_per_source_count[CV_TypeIndexSource_TPI]);
  String8 ipi_count_string = str8_from_count(tp_temp->v[0], internal_per_source_count[CV_TypeIndexSource_IPI]);
  ProfBeginDynamic("Internal Leaf Dedup [Obj Count: %.*s, TPI: %.*s, IPI: %.*s]",
                           str8_varg(obj_count_string),
                           str8_varg(tpi_count_string),
                           str8_varg(ipi_count_string));
#endif
  {

    LNK_LeafDedupInternal task;
    task.input       = input;
    task.hashes      = hashes;
    task.leaf_ht_arr = leaf_ht_arr;

    ProfBegin("Dedup .debug$P");
    task.debug_t_arr = input->internal_debug_p_arr;
    tp_for_parallel(tp, tp_temp, input->internal_count, lnk_leaf_dedup_internal_task, &task);
    ProfEnd();

    ProfBegin("Dedup .debug$T");
    task.debug_t_arr = input->internal_debug_t_arr;
    tp_for_parallel(tp, tp_temp, input->internal_count, lnk_leaf_dedup_internal_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBeginDynamic("External Leaf Import [Type Server Count: %llu, Dependent Obj Count: %llu]", input->type_server_count, input->external_count);
  {
    LNK_LeafDedupExternal task = {0};
    task.input                 = input;
    task.hashes                = hashes;
    task.leaf_ht_arr           = leaf_ht_arr;

    ProfBeginDynamic("Dedup TPI [Leaf Count %llu]", external_per_source_count[CV_TypeIndexSource_TPI]);
    task.dedup_ti_source = CV_TypeIndexSource_TPI;
    tp_for_parallel(tp, tp_temp, input->type_server_count, lnk_leaf_dedup_external_task, &task);
    ProfEnd();

    ProfBeginDynamic("Dedup IPI [Leaf Count %llu]", external_per_source_count[CV_TypeIndexSource_IPI]);
    task.dedup_ti_source = CV_TypeIndexSource_IPI;
    tp_for_parallel(tp, tp_temp, input->type_server_count, lnk_leaf_dedup_external_task, &task);
    ProfEnd();
  }
  ProfEnd();

  // extract present buckets from the hash tables
  LNK_LeafBucketArray tpi_arr = lnk_present_bucket_array_from_leaf_hash_table(tp, tp_temp->v[0], &leaf_ht_arr[CV_TypeIndexSource_TPI]);
  LNK_LeafBucketArray ipi_arr = lnk_present_bucket_array_from_leaf_hash_table(tp, tp_temp->v[0], &leaf_ht_arr[CV_TypeIndexSource_IPI]);

  // sort output leaves based on { location index, leaf index } to guarantee determinism
  lnk_leaf_bucket_array_sort(tp, ipi_arr, input->internal_count, input->type_server_count);
  lnk_leaf_bucket_array_sort(tp, tpi_arr, input->internal_count, input->type_server_count);

  // assign type indices to each bucket
  lnk_assign_type_indices(tp, tpi_arr, CV_MinComplexTypeIndex);
  lnk_assign_type_indices(tp, ipi_arr, CV_MinComplexTypeIndex);

  // patch indices in symbols, inline sites, and leaves
  lnk_patch_symbols(tp, input, hashes, leaf_ht_arr);
  lnk_patch_inlines(tp, input, hashes, leaf_ht_arr, input->count, input->debug_s_arr);
  lnk_patch_leaves(tp, input, hashes, leaf_ht_arr, tpi_arr);
  lnk_patch_leaves(tp, input, hashes, leaf_ht_arr, ipi_arr);

  CV_DebugT tpi_types = lnk_unbucket_leaf_array(tp, tp_temp->v[0], input, tpi_arr);
  CV_DebugT ipi_types = lnk_unbucket_leaf_array(tp, tp_temp->v[0], input, ipi_arr);

  ProfBegin("Post Process CV Symbols");
  {
    LNK_PostProcessCvSymbolsTask task = {0};
    task.ipi_min_type_index           = CV_MinComplexTypeIndex;
    task.ipi_types                    = ipi_types;
    task.symbol_inputs                = input->symbol_inputs;
    task.parsed_symbols               = input->parsed_symbols;
    tp_for_parallel(tp, 0, input->total_symbol_input_count, lnk_post_process_cv_symbols_task, &task);
  }
  ProfEnd();

  CV_DebugT *types = push_array(tp_temp->v[0], CV_DebugT, CV_TypeIndexSource_COUNT);
  types[CV_TypeIndexSource_TPI] = tpi_types;
  types[CV_TypeIndexSource_IPI] = ipi_types;

  ProfEnd();
  return types;
}

internal U64
lnk_format_u128(U8 *buf, U64 buf_max, U64 length, U128 v)
{
  U64 size = 0;
  if (length > 0 && buf_max > 0) {
    if (length <= 8) {
      U64 mask = length == 8 ? max_U64 : (1ull << (length*8)) - 1;
      size = raddbg_snprintf((char*)buf, buf_max - 1, "%llX", (long long)(v.u64[0] & mask));
    } else {
      U64 mask1 = length == 16 ? max_U64 : (1ull << ((length-8)*8)) - 1;
      size = raddbg_snprintf((char*)buf, buf_max, "%llX%llX", (long long)(v.u64[1] & mask1), (long long)v.u64[0]);
    }
  }
  return size;
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_type_names_with_hashes_lenient_task)
{
  ProfBeginFunction();

  LNK_TypeNameReplacer *task        = raw_task;
  Rng1U64               range       = task->ranges[task_id];
  CV_DebugT             debug_t     = task->debug_t;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64 hash_max_chars = hash_length*2;
  U8  temp[128];

  for (U64 leaf_idx = range.min; leaf_idx < range.max; ++leaf_idx) {
    CV_Leaf leaf = cv_debug_t_get_leaf(debug_t, leaf_idx);
    if (leaf.kind == CV_LeafKind_STRUCTURE || leaf.kind == CV_LeafKind_CLASS) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);

      if ((udt_info.props & CV_TypeProp_HasUniqueName) &&
           udt_info.unique_name.size > hash_max_chars &&
           udt_info.name.size > hash_max_chars) {
        // hash unique name
        U128 name_hash;
        blake3_hasher hasher; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.unique_name.str, udt_info.unique_name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> unique name map
        if (make_map) {
          lnk_format_u128(temp, sizeof(temp), hash_length, name_hash);
          str8_list_pushf(map_arena, map, "%s %S\n", temp, str8_varg(udt_info.unique_name));
        }

        // parse leaf size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        String8 lambda_prefix = str8_lit("<lambda_");
        U64     colon_pos     = str8_find_needle_reverse(udt_info.name, 0, lambda_prefix, 0);
        B32     is_lambda     = colon_pos != 0;

        if (is_lambda) {
          U64 size = lnk_format_u128(temp, sizeof(temp), hash_length, name_hash);
          Assert(size < udt_info.name.size);
          Assert(size < udt_info.unique_name.size);
          MemoryCopy(udt_info.name.str, temp, size+1);
          MemoryCopy(udt_info.name.str+size+1, temp, size+1);
          udt_info.name.size        = size;
          udt_info.unique_name.size = size;

          // update leaf header
          CV_LeafHeader *header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
          header->size          = sizeof(CV_LeafKind) +
                                  sizeof(CV_LeafStruct) +
                                  numeric_size +
                                  udt_info.name.size + 1 +
                                  udt_info.unique_name.size + 1;
        } else {
          // replace uniuqe type name with hash
          udt_info.unique_name.str  = udt_info.name.str + udt_info.name.size + 1;
          udt_info.unique_name.size = lnk_format_u128(udt_info.unique_name.str, udt_info.unique_name.size, hash_length, name_hash);

          // update leaf header
          CV_LeafHeader *header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
          header->size          = sizeof(CV_LeafKind) +
                                  sizeof(CV_LeafStruct) +
                                  numeric_size +
                                  udt_info.name.size + 1 +
                                  udt_info.unique_name.size + 1;
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
  CV_DebugT             debug_t     = task->debug_t;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64 hash_max_chars = hash_length*2;
  U8  temp[128];

  for (U64 leaf_idx = range.min; leaf_idx < range.max; ++leaf_idx) {
    CV_Leaf leaf = cv_debug_t_get_leaf(debug_t, leaf_idx);
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
        U128 name_hash;
        blake3_hasher hasher; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.name.str, udt_info.name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> name map
        if (make_map) {
          lnk_format_u128(temp, sizeof(temp), hash_length, name_hash);
          str8_list_pushf(map_arena, map, "%s %.*s\n", temp, str8_varg(name));
        }

        // replace name with hash
        udt_info.name.size = lnk_format_u128(udt_info.name.str, udt_info.name.size, hash_length, name_hash);

        // parse struct size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        // update header
        CV_LeafHeader *header = cv_debug_t_get_leaf_header(debug_t, leaf_idx);
        header->size          = sizeof(CV_LeafKind) + sizeof(CV_LeafStruct) + numeric_size + udt_info.name.size + 1;

        // discard unique name
        CV_LeafStruct *lf = (CV_LeafStruct *)(header + 1);
        lf->props &= ~CV_TypeProp_HasUniqueName;
      }
    }
  }

  ProfEnd();
}

internal void
lnk_replace_type_names_with_hashes(TP_Context *tp, TP_Arena *arena, CV_DebugT debug_t, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  // init task context
  LNK_TypeNameReplacer task = {0};
  task.debug_t              = debug_t;
  task.ranges               = tp_divide_work(scratch.arena, debug_t.count, tp->worker_count);
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

////////////////////////////////
// PDB Builder

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
THREAD_POOL_TASK_FUNC(lnk_process_sym_data_task)
{
  ProfBeginFunction();

  U64                         obj_idx        = task_id;
  LNK_ProcessSymDataTaskData *task           = raw_task;
  CV_SymbolListArray          parsed_symbols = task->parsed_symbols[obj_idx];

  static CV_Signature MODULE_SYMBOL_SIGNATURE = CV_Signature_C13;

  ProfBegin("Compute Buffer Size");
  U64 buffer_size = sizeof(MODULE_SYMBOL_SIGNATURE);
  for (U64 i = 0; i < parsed_symbols.count; ++i) {
    CV_SymbolList list = parsed_symbols.v[i];
    U64 data_size = cv_patch_symbol_tree_offsets(list, buffer_size, PDB_SYMBOL_ALIGN);
    buffer_size += data_size;
  }
  ProfEnd();

  // alloc buffer
  U8 *buffer        = push_array_no_zero(arena, U8, buffer_size);
  U64 buffer_cursor = 0;

  // MS Symbol and Type Information p.4:
  //   "The first four bytes of the $$SYMBOLS segment is used as a signature to specify the version of
  //    the Symbol and Type OMF contained in the $$SYMBOLS segment."
  CV_Signature *sig_ptr = (CV_Signature *) (buffer + buffer_cursor);
  *sig_ptr = MODULE_SYMBOL_SIGNATURE;
  buffer_cursor += sizeof(*sig_ptr);

  ProfBegin("Serialize Symbols");
  for (U64 i = 0; i < parsed_symbols.count; ++i) {
    CV_SymbolList list = parsed_symbols.v[i];
    for (CV_SymbolNode *symbol_n = list.first; symbol_n != 0; symbol_n = symbol_n->next) {
      symbol_n->data.offset = buffer_cursor;
      buffer_cursor += cv_serialize_symbol_to_buffer(buffer, buffer_cursor, buffer_size, &symbol_n->data, PDB_SYMBOL_ALIGN);
    }
  }
  ProfEnd();

  // output
  Assert(task->symbol_data_arr[obj_idx].total_size == 0);
  str8_list_push(arena, &task->symbol_data_arr[obj_idx], str8(buffer, buffer_size));
  
  ProfEnd();
}

internal LNK_ProcessedCodeViewC11Data
lnk_process_c11_data(TP_Context *tp, TP_Arena *arena, U64 obj_count, CV_DebugS *debug_s_arr, U64 string_data_base_offset, CV_StringHashTable string_ht, MSF_Context *msf, PDB_DbiModule **mod_arr)
{
  // TODO: handle c11 data
  String8List *data_list_arr = push_array(arena->v[0], String8List, obj_count);
  LNK_ProcessedCodeViewC11Data result;
  result.data_list_arr = data_list_arr;
  return result;
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

  // get module sub-sections
  PDB_DbiModule *mod          = task->dbi_mod_arr[obj_idx];
  String8        mod_c13_data = dbi_module_read_c13_data(scratch.arena, task->msf, mod);
  CV_DebugS      mod_debug_s  = cv_parse_debug_s_c13(scratch.arena, mod_c13_data);

  // relocate line and frame data 
  String8List *mod_checksum_data        = cv_sub_section_ptr_from_debug_s(&mod_debug_s, CV_C13SubSectionKind_FileChksms);
  U64          checksum_base            = mod_checksum_data->total_size;
  B32          is_checksum_patch_needed = checksum_base > 0;
  if (is_checksum_patch_needed) {
    String8List line_data  = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);
    String8List frame_data = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FrameData);
    cv_c13_patch_checksum_offsets_in_line_data_list(line_data, checksum_base);
    cv_c13_patch_checksum_offsets_in_frame_data_list(frame_data, checksum_base);
  }

  // push obj c13 data to module
  cv_debug_s_concat_in_place(&mod_debug_s, &debug_s);

  // serialize c13 data
  B32 include_sig = 0;
  String8List c13_data = cv_data_c13_from_debug_s(arena, &mod_debug_s, include_sig);

  // store for later pass
  task->c13_data_arr[obj_idx]               = c13_data;
  task->source_file_names_list_arr[obj_idx] = source_file_names_list;

  scratch_end(scratch);
  ProfEnd();
}

internal LNK_ProcessedCodeViewC13Data
lnk_process_c13_data(TP_Context *tp, TP_Arena *arena, U64 obj_count, CV_DebugS *debug_s_arr, U64 string_data_base_offset, CV_StringHashTable string_ht, MSF_Context *msf, PDB_DbiModule **mod_arr)
{
  ProfBeginFunction();

  LNK_ProcessC13DataTask task     = {0};
  task.debug_s_arr                = debug_s_arr;
  task.msf                        = msf;
  task.dbi_mod_arr                = mod_arr;
  task.c13_data_arr               = push_array_no_zero(arena->v[0], String8List, obj_count);
  task.source_file_names_list_arr = push_array_no_zero(arena->v[0], String8List, obj_count);
  task.string_data_base_offset    = string_data_base_offset;
  task.string_ht                  = string_ht;
  tp_for_parallel(tp, arena, obj_count, lnk_process_c13_data_task, &task);
  
  // fill out result
  LNK_ProcessedCodeViewC13Data result = {0};
  result.data_list_arr                = task.c13_data_arr;
  result.source_file_names_list_arr   = task.source_file_names_list_arr;

  ProfEnd();
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_write_module_data_task)
{
  U64 obj_idx = task_id;
  LNK_WriteModuleDataTask *task = raw_task;

  PDB_DbiModule *mod = task->mod_arr[obj_idx];
  String8List sym_data = task->symbol_data_arr[obj_idx];
  String8List c11_data = task->c11_data_list_arr[obj_idx];
  String8List c13_data = task->c13_data_list_arr[obj_idx];
  String8List globrefs = task->globrefs_arr[obj_idx];
  
  U32 sym_data_size32 = safe_cast_u32(sym_data.total_size);
  U32 c11_data_size32 = safe_cast_u32(c11_data.total_size);
  U32 c13_data_size32 = safe_cast_u32(c13_data.total_size);
  U32 globrefs_size32 = safe_cast_u32(globrefs.total_size);
  
  // layout module data
  String8List module_data = {0};
  str8_list_concat_in_place(&module_data, &sym_data);
  str8_list_concat_in_place(&module_data, &c11_data);
  str8_list_concat_in_place(&module_data, &c13_data);
  str8_list_concat_in_place(&module_data, &globrefs);

  // make stream has enough memory so it doens't trigger memory allocations in MSF
  // during multi-thread write
  MSF_UInt stream_pos = msf_stream_get_pos(task->msf, mod->sn);
  if (stream_pos != 0) {
    Assert(!"stream must be at start position");
  }
  MSF_UInt stream_cap = msf_stream_get_cap(task->msf, mod->sn);
  if (stream_cap < module_data.total_size) {
    Assert(!"not enough bytes in destination stream to copy module data");
  }
  
  // write data
  B32 is_write_ok = msf_stream_write_list(task->msf, mod->sn, module_data);
  
  // update module data sizes
  if (is_write_ok) {
    mod->sym_data_size = sym_data_size32;
    mod->c11_data_size = c11_data_size32;
    mod->c13_data_size = c13_data_size32;
    mod->globrefs_size = globrefs_size32;
  } else {
    // TODO: error handle
  }
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
  U64                             obj_idx    = task_id;
  LNK_PushDbiSecContribTaskData  *task       = raw_task;
  LNK_Section                  **sect_id_map = task->sect_id_map;
  PDB_DbiModule                  *mod        = task->mod_arr[obj_idx];
  LNK_Obj                        *obj        = &task->obj_arr[obj_idx];
  PDB_DbiSectionContribList      *dst_list   = &task->sc_list[obj_idx];
  String8                         image_data = task->image_data;

  // TODO: use chunked lists for SC

  // TODO: put back unused nodes
  PDB_DbiSectionContribNode *sc_arr   = push_array_no_zero(arena, PDB_DbiSectionContribNode, obj->sect_count);
  U64                        sc_count = 0;
  
  for (U64 chunk_idx = 0; chunk_idx < obj->sect_count; ++chunk_idx) {
    LNK_Chunk *chunk = obj->chunk_arr[chunk_idx];
    
    if (!chunk || lnk_chunk_is_discarded(chunk)) {
      continue;
    }

    LNK_Section *sect = lnk_sect_from_chunk_ref(task->sect_id_map, chunk->ref);
    if (!sect->has_layout) {
      continue;
    }

    // query chunk info
    ISectOff     chunk_sc   = lnk_sc_from_chunk_ref       (sect_id_map, chunk->ref);
    String8      chunk_data = lnk_data_from_chunk_ref     (sect_id_map, image_data, chunk->ref);
    LNK_Section *chunk_sect = lnk_sect_from_chunk_ref     (sect_id_map, chunk->ref);
    U64          chunk_size = lnk_file_size_from_chunk_ref(sect_id_map, chunk->ref);
    
    // compute chunk CRC
    U32 data_crc  = update_crc32(0, chunk_data.str, chunk_data.size);
    U32 reloc_crc = 0; // TODO: compute CRC for relocations block

    // fill out SC
    PDB_DbiSectionContribNode *sc = sc_arr + sc_count++;
    sc->data.base.sec             = safe_cast_u16(chunk_sc.isect);
    sc->data.base.pad0            = 0;
    sc->data.base.sec_off         = chunk_sc.off;
    sc->data.base.size            = safe_cast_u32(chunk_size);
    sc->data.base.flags           = chunk_sect->flags;
    sc->data.base.mod             = mod->imod;
    sc->data.base.pad1            = 0;
    sc->data.data_crc             = data_crc;
    sc->data.reloc_crc            = reloc_crc;

    dbi_sec_contrib_list_push_node(dst_list, sc);
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

////////////////////////////////

internal
THREAD_POOL_TASK_FUNC(lnk_build_pdb_public_symbols_defined_task)
{
  ProfBeginFunction();

  LNK_BuildPublicSymbolsTask   *task        = raw_task;
  LNK_Section                 **sect_id_map = task->sect_id_map;
  CV_SymbolList                *pub_list    = &task->pub_list_arr[task_id];
  LNK_SymbolHashTrieChunkList   chunk_list  = task->chunk_lists[task_id];

  for (LNK_SymbolHashTrieChunk *chunk = chunk_list.first; chunk != 0; chunk = chunk->next) {
    CV_SymbolNode *nodes = push_array_no_zero(arena, CV_SymbolNode, chunk->count);

    for (U64 i = 0, node_idx = 0; i < chunk->count; ++i) {
      LNK_Symbol *symbol = chunk->v[i].symbol;

      Assert(LNK_Symbol_IsDefined(symbol->type));

      LNK_DefinedSymbol *defined_symbol = &symbol->u.defined;
      if (defined_symbol->value_type == LNK_DefinedSymbolValue_Chunk) {
        CV_Pub32Flags flags = 0;
        if (defined_symbol->flags & LNK_DefinedSymbolFlag_IsFunc || defined_symbol->flags & LNK_DefinedSymbolFlag_IsThunk) {
          flags |= CV_Pub32Flag_Function;
        }

        U64 symbol_off   = lnk_sect_off_from_symbol(sect_id_map, symbol);
        U64 symbol_isect = lnk_isect_from_symbol(sect_id_map, symbol);

        U32 symbol_off32   = safe_cast_u32(symbol_off);
        U16 symbol_isect16 = safe_cast_u16(symbol_isect);

        nodes[node_idx].data = cv_make_pub32(arena, flags, symbol_off32, symbol_isect16, symbol->name);
        cv_symbol_list_push_node(pub_list, &nodes[node_idx]);

        ++node_idx;
      }
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
                             LNK_Section          **sect_id_map,
                             PDB_PsiContext        *psi)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Defined");
  LNK_BuildPublicSymbolsTask task = {0};
  task.sect_id_map                = sect_id_map;
  task.pub_list_arr               = push_array(scratch.arena, CV_SymbolList, tp->worker_count);
  task.chunk_lists = symtab->chunk_lists[LNK_SymbolScopeIndex_Defined];
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
              Guid                      guid,
              COFF_MachineType          machine,
              COFF_TimeStamp            time_stamp,
              U32                       age,
              U64                       page_size,
              String8                   pdb_name,
              String8List               lib_dir_list,
              String8List               natvis_list,
              LNK_SymbolTable          *symtab,
              LNK_Section             **sect_id_map,
              U64                       obj_count,
              LNK_Obj                  *obj_arr,
              CV_DebugS                *debug_s_arr,
              U64                       total_symbol_input_count,
              LNK_CodeViewSymbolsInput *symbol_inputs,
              CV_SymbolListArray       *parsed_symbols,
              CV_DebugT                 types[CV_TypeIndexSource_COUNT])
{
  ProfBegin("PDB");
  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);

  ProfBegin("Setup PDB Context");
  PDB_Context *pdb = pdb_alloc(page_size, machine, time_stamp, age, guid);
  ProfEnd();

  // move patched type data
  //
  // leaf data is stored in g_file_arena which has linker's life-time
  // and this way we skip redundant leaf copy to the type server to make things faster
  pdb_type_server_push_parallel(tp, pdb->type_servers[CV_TypeIndexSource_IPI], types[CV_TypeIndexSource_IPI]);
  pdb_type_server_push_parallel(tp, pdb->type_servers[CV_TypeIndexSource_TPI], types[CV_TypeIndexSource_TPI]);

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
  for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
    LNK_Obj *obj = obj_arr + obj_idx;
    mod_arr[obj_idx] = dbi_push_module(pdb->dbi, obj->path, obj->lib_path);

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

  ProfBegin("Build DBI Modules");
  {
    TP_Temp temp = tp_temp_begin(tp_arena);
    {
      ProfBegin("Reloc Module Data");

      ProfBegin("Serialize Symbols");
      String8List *serialized_symbol_data = push_array(scratch.arena, String8List, obj_count);
      {
        LNK_ProcessSymDataTaskData task = {0};
        task.symbol_inputs              = symbol_inputs;
        task.parsed_symbols             = parsed_symbols;
        task.mod_arr                    = mod_arr;
        task.symbol_data_arr            = serialized_symbol_data;
        tp_for_parallel(tp, tp_arena, obj_count, lnk_process_sym_data_task, &task);
      }
      ProfEnd();

      LNK_ProcessedCodeViewC11Data processed_c11 = lnk_process_c11_data(tp, tp_arena, obj_count, debug_s_arr, string_data_base_offset, string_ht, pdb->msf, mod_arr);
      LNK_ProcessedCodeViewC13Data processed_c13 = lnk_process_c13_data(tp, tp_arena, obj_count, debug_s_arr, string_data_base_offset, string_ht, pdb->msf, mod_arr);

      ProfEnd();

      // TODO: actually collect offsets and pass them here
      ProfBegin("Build Empty Global Reference Array");
      String8List *globrefs_arr = push_array(tp_arena->v[0], String8List, obj_count);
      for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
        String8List *globrefs = &globrefs_arr[obj_idx];
        str8_serial_begin(tp_arena->v[0], globrefs);
        Assert(globrefs->total_size == 0);
        str8_serial_push_u32(tp_arena->v[0], globrefs, globrefs->total_size);
      }
      ProfEnd();

      // reserve memory for module streams
      ProfBegin("Reserve Modules Memory");
      for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
        // compute number of bytes needed for module data
        U64 mod_size = 0;
        mod_size += serialized_symbol_data[obj_idx].total_size;
        mod_size += processed_c11.data_list_arr[obj_idx].total_size;
        mod_size += processed_c13.data_list_arr[obj_idx].total_size;
        mod_size += globrefs_arr[obj_idx].total_size;

        U32 mod_size32 = safe_cast_u32(mod_size);

        // allocate stream for module
        PDB_DbiModule *mod = mod_arr[obj_idx];
        mod->sn = msf_stream_alloc_ex(pdb->msf, mod_size32);
      }
      ProfEnd();

      // copy data to module streams
      ProfBegin("Write Modules Data");
      LNK_WriteModuleDataTask write_module_data_task_data;
      write_module_data_task_data.msf               = pdb->msf;
      write_module_data_task_data.mod_arr           = mod_arr;
      write_module_data_task_data.symbol_data_arr   = serialized_symbol_data;
      write_module_data_task_data.c11_data_list_arr = processed_c11.data_list_arr;
      write_module_data_task_data.c13_data_list_arr = processed_c13.data_list_arr;
      write_module_data_task_data.globrefs_arr      = globrefs_arr;
      tp_for_parallel(tp, 0, obj_count, lnk_write_module_data_task, &write_module_data_task_data);
      ProfEnd();

      // push source files per module info
      ProfBegin("Build Source Files List");
      for (U64 obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
        PDB_DbiModule *mod = mod_arr[obj_idx];
        String8List source_file_list_scratch = processed_c13.source_file_names_list_arr[obj_idx];
        String8List source_file_list = str8_list_copy(pdb->dbi->arena, &source_file_list_scratch);
        str8_list_concat_in_place(&mod->source_file_list, &source_file_list);
      }
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
    LNK_Symbol         *coff_sect_array_symbol = lnk_symbol_table_searchf(symtab, LNK_SymbolScopeFlag_Internal, LNK_COFF_SECT_HEADER_ARRAY_SYMBOL_NAME);
    LNK_Chunk          *coff_sect_chunk        = lnk_chunk_from_symbol(coff_sect_array_symbol);
    String8             coff_sect_chunk_data   = lnk_data_from_chunk_ref(sect_id_map, image_data, coff_sect_chunk->ref);
    U64                 coff_sect_count        = coff_sect_chunk_data.size / sizeof(COFF_SectionHeader);
    COFF_SectionHeader *coff_sect_ptr          = (COFF_SectionHeader*)coff_sect_chunk_data.str;
    for (COFF_SectionHeader *hdr_ptr = &coff_sect_ptr[0], *opl = hdr_ptr + coff_sect_count;
         hdr_ptr < opl;
         ++hdr_ptr) {
      dbi_push_section(pdb->dbi, hdr_ptr);
    }
  }
  ProfEnd();

  ProfBegin("Build Section Contrib Map");
  {
    LNK_PushDbiSecContribTaskData task = {0};
    task.obj_arr                       = obj_arr;
    task.sect_id_map                   = sect_id_map;
    task.mod_arr                       = mod_arr;
    task.sc_list                       = push_array(scratch.arena, PDB_DbiSectionContribList, obj_count);
    task.image_data                    = image_data;
    tp_for_parallel(tp, tp_arena, obj_count, lnk_push_dbi_sec_contrib_task, &task);

    dbi_sec_list_concat_arr(&pdb->dbi->sec_contrib_list, obj_count, task.sc_list);
  }
  ProfEnd();

  ProfBegin("Build NatVis");
  {
    String8Array natvis_file_path_arr = str8_array_from_list(scratch.arena, &natvis_list);
    String8Array natvis_file_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, natvis_file_path_arr);

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
  
  lnk_build_pdb_public_symbols(tp, tp_arena, symtab, sect_id_map, pdb->psi);
  
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

  scratch_end(scratch);
  ProfEnd();
  return page_data_list;
}

////////////////////////////////
// RAD Debug Info

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

  for (U64 leaf_idx = task->ranges[task_id].min; leaf_idx < task->ranges[task_id].max; ++leaf_idx) {
    CV_Leaf leaf = cv_debug_t_get_leaf(task->debug_t, leaf_idx);
    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);
      if (~udt_info.props & CV_TypeProp_FwdRef) {
        if (!cv_is_udt_name_anon(udt_info.name)) {
          String8 name       = cv_name_from_udt_info(udt_info);
          U64     hash       = lnk_udt_name_hash_table_hash(name);
          U64     best_idx   = hash % task->buckets_cap;
          U64     bucket_idx = best_idx;

          if (new_bucket == 0) {
            new_bucket = push_array(arena, LNK_UDTNameBucket, 1);
          }
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
    }
  }
}


internal LNK_UDTNameBucket **
lnk_udt_name_hash_table_from_debug_t(TP_Context *tp,
                                     TP_Arena   *arena,
                                     CV_DebugT   debug_t,
                                     U64        *buckets_cap_out)
{
  Temp scratch = scratch_begin(&arena->v[0], 1);
  LNK_BuildUDTNameHashTableTask task = {0};
  task.debug_t     = debug_t;
  task.buckets_cap = (U64)((F64)debug_t.count * 1.3);
  task.buckets     = push_array(arena->v[0], LNK_UDTNameBucket *, task.buckets_cap);
  task.ranges      = tp_divide_work(scratch.arena, debug_t.count, tp->worker_count);
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
      U64     leaf_idx = itype - tpi_range.min;
      CV_Leaf leaf     = cv_debug_t_get_leaf(task->types[CV_TypeIndexSource_TPI], leaf_idx);
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
    CV_Leaf src   = cv_debug_t_get_leaf(task->types[CV_TypeIndexSource_TPI], leaf_idx);

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
        CV_Leaf next_leaf     = cv_debug_t_get_leaf(task->types[CV_TypeIndexSource_TPI], next_leaf_idx);
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
            CV_Leaf method_list_leaf     = cv_debug_t_get_leaf(task->types[CV_TypeIndexSource_TPI], method_list_leaf_idx);
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
    if (rdib_source_file_match(buckets[bucket_idx]->src_file, &temp, operating_system_from_context())) {
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
    } else if (rdib_source_file_match(curr_bucket->src_file, new_bucket->src_file, operating_system_from_context())) {
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
    lnk_error_obj(LNK_Warning_IllData, &task->obj_arr[obj_idx], "Multiple string table sub-sections, picking first one.");
  }
  if (raw_chksms_list.node_count > 1) {
    lnk_error_obj(LNK_Warning_IllData, &task->obj_arr[obj_idx], "Multiple file checksum sub-sections, picking first one.");
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
lnk_virt_off_from_sect_off(U64 sect_idx, U64 sect_off, LNK_SectionArray image_sects, LNK_Obj *obj, CV_SymKind symbol_kind, U64 symbol_offset)
{
  U64 virt_off = 0;
  if (sect_idx < image_sects.count) {
    virt_off = image_sects.v[sect_idx].virt_off + sect_off;
  } else {
    lnk_error_obj(LNK_Error_CvIllSymbolData, obj, "Out of bounds section index 0x%x in S_%S @ 0x%llx.",
                  sect_idx, cv_string_from_sym_kind(symbol_kind), symbol_offset);
  }
  return virt_off;
}

internal Rng1U64
lnk_virt_range_from_sect_off_size(U64 sect_idx, U64 sect_off, U64 size, LNK_SectionArray image_sects, LNK_Obj *obj, CV_SymKind symbol_kind, U64 symbol_offset)
{
  Rng1U64 virt_range = {0};
  if (sect_idx < image_sects.count) {
    U64 virt_off = image_sects.v[sect_idx].virt_off + sect_off;
    virt_range = rng_1u64(virt_off, virt_off + size);
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

  LNK_Obj *obj = &task->obj_arr[task_id];

  // fill out unit info
  U64 unit_chunk_idx = task_id / task->unit_chunk_cap;
  U64 local_unit_idx = task_id - unit_chunk_idx * task->unit_chunk_cap;

  RDIB_Unit *dst     = &task->units[unit_chunk_idx].v[local_unit_idx];
  dst->arch          = rdi_arch_from_cv_arch(comp_info->arch);
  dst->unit_name     = str8_skip_last_slash(obj->path);
  dst->compiler_name = comp_info->compiler_name;
  dst->source_file   = str8_zero();
  dst->object_file   = push_str8_copy(arena, obj->path);
  dst->archive_file  = push_str8_copy(arena, obj->lib_path);
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
  LNK_Obj                  *obj      = &task->obj_arr[unit_idx];
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
      LNK_Section  *sect  = &task->image_sects.v[parsed_lines.sec_idx];
      CV_LineArray  lines = cv_c13_line_array_from_data(arena, raw_lines, sect->virt_off, parsed_lines);

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
  LNK_CodeViewSymbolsInput  symbols_input       = task->symbol_inputs[task_id];
  LNK_Obj                  *obj                 = &task->obj_arr[symbols_input.obj_idx];
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
      U64           data_voff      = lnk_virt_off_from_sect_off(data32->sec, data32->off, task->image_sects, obj, symbol.kind, symbol.offset);

      B32 is_comp_gen = symbol.kind == CV_SymKind_LDATA32 && name.size == 0 && type == 0;
      if (!is_comp_gen) {

      // get link name through virtual offset look up
      String8 link_name = {0};
      if (symbol.kind == CV_SymKind_GDATA32) {
        KeyValuePair *pair = hash_table_search_u64(task->extern_symbol_voff_ht, data_voff);
        if (pair != 0) {
          LNK_Symbol *link_symbol = pair->value_raw;
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
        KeyValuePair *pair = hash_table_search_u64(task->extern_symbol_voff_ht, virt_range.min);
        if (pair != 0) {
          LNK_Symbol *link_symbol = pair->value_raw;
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
        CV_Leaf leaf     = cv_debug_t_get_leaf(task->ipi, leaf_idx);
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
        LNK_Obj *obj = task->obj_arr + obj_idx;
        lnk_error_obj(LNK_Warning_IllData, obj, "Not enough bytes to read file checksum @ 0x%llx.", lines.file_off);
        continue;
      }
      String8 file_path      = str8_cstring_capped(raw_string_table.str + checksum_header->name_off, raw_string_table.str + raw_string_table.size);
      String8 checksum_bytes = str8((U8 *) (checksum_header + 1), checksum_header->len);
      
      // find source file for this line table
      String8               normal_path     = lnk_normalize_src_file_path(scratch.arena, file_path);
      U64                   src_file_hash   = lnk_src_file_hash_cv(normal_path, checksum_header->kind, checksum_bytes);
      LNK_SourceFileBucket *src_file_bucket = lnk_src_file_hash_table_lookup_slot(task->src_file_buckets, task->src_file_buckets_cap, src_file_hash, normal_path, checksum_header->kind, checksum_bytes);
      if (src_file_bucket == 0) {
        LNK_Obj *obj = task->obj_arr + obj_idx;
        lnk_error_obj(LNK_Error_UnexpectedCodePath, obj, "Unable to find source file in the hash table: \"%S\".", file_path);
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
  LNK_Obj *obj      = &task->obj_arr[unit_idx];

  U64 unit_chunk_idx = unit_idx / task->unit_chunk_cap;
  U64 local_unit_idx = unit_idx - unit_chunk_idx * task->unit_chunk_cap;

  RDIB_Unit *dst        = &task->units[unit_chunk_idx].v[local_unit_idx];
  dst->virt_range_count = 0;
  dst->virt_ranges      = push_array_no_zero(arena, Rng1U64, obj->sect_count);

  for (U64 chunk_idx = 0; chunk_idx < obj->sect_count; ++chunk_idx) {
    LNK_Chunk *chunk = obj->chunk_arr[chunk_idx];
    if (!chunk || lnk_chunk_is_discarded(chunk)) {
      continue;
    }

    LNK_Section *sect = lnk_sect_from_chunk_ref(task->sect_id_map, chunk->ref);
    if (!sect->has_layout) {
      continue;
    }

    U64 chunk_voff = lnk_virt_off_from_chunk_ref(task->sect_id_map, chunk->ref);
    U64 chunk_size = lnk_virt_size_from_chunk_ref(task->sect_id_map, chunk->ref);

    if (chunk_size == 0) {
      continue;
    }

    dst->virt_ranges[dst->virt_range_count] = rng_1u64(chunk_voff, chunk_voff + chunk_size);
    ++dst->virt_range_count;
  }

  // free unused memory
  arena_pop(arena, sizeof(dst->virt_ranges[0]) * (obj->sect_count - dst->virt_range_count));

  ProfEnd();
}

internal String8List
lnk_build_rad_debug_info(TP_Context               *tp,
                         TP_Arena                 *tp_arena,
                         OperatingSystem           os,
                         RDI_Arch                  arch,
                         String8                   image_name,
                         String8                   image_data,
                         LNK_SectionArray          image_sects,
                         LNK_Section             **sect_id_map,
                         U64                       obj_count,
                         LNK_Obj                  *obj_arr,
                         CV_DebugS                *debug_s_arr,
                         U64                       total_symbol_input_count,
                         LNK_CodeViewSymbolsInput *symbol_inputs,
                         CV_SymbolListArray       *parsed_symbols,
                         CV_DebugT                 types[CV_TypeIndexSource_COUNT])
{
  ProfBegin("RDI");
  Temp scratch = scratch_begin(0,0);

  RDIB_Input input = rdib_init_input(scratch.arena);

  ProfBegin("Top Level Info");
  {
    U64 image_vsize = 0;
    for (U64 sect_idx = 0; sect_idx < image_sects.count; sect_idx++) {
      LNK_Section *sect  = &image_sects.v[sect_idx];
      U64 sect_virt_size = lnk_virt_size_from_chunk_ref(sect_id_map, sect->root->ref);
      U64 sect_voff_max  = sect->virt_off + sect_virt_size;
      image_vsize = Max(image_vsize, sect_voff_max);
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
      LNK_Section        *src = &image_sects.v[sect_idx];
      RDIB_BinarySection *dst = &input.sections[sect_idx];

      U64 sect_virt_size = lnk_virt_size_from_chunk_ref(sect_id_map, src->root->ref);
      U64 sect_file_size = lnk_file_size_from_chunk_ref(sect_id_map, src->root->ref);

      dst->name       = push_str8_copy(scratch.arena, src->name);
      dst->flags      = rdi_binary_section_flags_from_coff_section_flags(src->flags);
      dst->voff_first = src->virt_off;
      dst->voff_opl   = src->virt_off + sect_virt_size;
      dst->foff_first = src->file_off;
      dst->foff_opl   = src->file_off + sect_file_size;
    }
  }
  ProfEnd();

  // assing low and high type indices per source
  Rng1U64 itype_ranges[CV_TypeIndexSource_COUNT];
  for (U64 i = 0; i < ArrayCount(itype_ranges); ++i) {
    itype_ranges[i] = rng_1u64(CV_MinComplexTypeIndex, CV_MinComplexTypeIndex + types[i].count);
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
    udt_name_buckets     = lnk_udt_name_hash_table_from_debug_t(tp, tp_arena, types[CV_TypeIndexSource_TPI], &udt_name_buckets_cap);
    ProfEnd();


    ProfBegin("Convert CodeView types to RDIB Types");
    LNK_ConvertTypesToRDI task         = {0};
    task.types                         = types;
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
    task.ranges                        = tp_divide_work(scratch.arena, types[CV_TypeIndexSource_TPI].count, tp->worker_count);
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
    task.sect_id_map              = sect_id_map;
    task.obj_arr                  = obj_arr;
    task.debug_s_arr              = debug_s_arr;
    task.ipi                      = types[CV_TypeIndexSource_IPI];
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
    tp_for_parallel(tp, tp_arena, total_symbol_input_count, lnk_convert_symbols_to_rdi_task, &task);
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

