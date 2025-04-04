// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

internal void
lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  lnk_error_with_loc_fv(code, obj->path, obj->lib_path, fmt, args);
  va_end(args);
}

////////////////////////////////

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

internal void
lnk_input_obj_list_concat_in_place(LNK_InputObjList *list, LNK_InputObjList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
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

////////////////////////////////

internal LNK_Obj **
lnk_obj_arr_from_list(Arena *arena, LNK_ObjList list)
{
  LNK_Obj **arr = push_array_no_zero(arena, LNK_Obj *, list.count);
  U64 idx = 0;
  for (LNK_ObjNode *node = list.first; node != 0; node = node->next, ++idx) {
    arr[idx] = &node->data;
  }
  return arr;
}

internal LNK_ObjNodeArray
lnk_obj_list_reserve(Arena *arena, LNK_ObjList *list, U64 count)
{
  LNK_ObjNodeArray arr = {0};
  if (count) {
    arr.count = count;
    arr.v = push_array(arena, LNK_ObjNode, count);
    for (LNK_ObjNode *ptr = arr.v, *opl = arr.v + arr.count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += count;
  } else {
    MemoryZeroStruct(&arr);
  }
  
  return arr;
}

internal LNK_ChunkList
lnk_obj_search_chunks(Arena *arena, LNK_Obj *obj, String8 name, String8 postfix, B32 collect_discarded)
{
  LNK_ChunkList list = {0};
  for (U64 sect_idx = 0; sect_idx < obj->chunk_count; ++sect_idx) {
    String8 obj_sect_name = obj->sect_name_arr[sect_idx];
    String8 obj_sect_sort = obj->sect_sort_arr[sect_idx];

    B32 is_match = str8_match(obj_sect_name, name, 0) &&
                   str8_match(obj_sect_sort, postfix, 0);

    if (is_match) {
      LNK_ChunkPtr chunk = obj->chunk_arr[sect_idx];

      if (!collect_discarded && lnk_chunk_is_discarded(chunk)) {
        continue;
      }

      LNK_ChunkNode *node = push_array_no_zero(arena, LNK_ChunkNode, 1);
      node->next          = 0;
      node->data          = chunk;

      SLLQueuePush(list.first, list.last, node);
      ++list.count;
    }
  }
  return list;
}

internal
THREAD_POOL_TASK_FUNC(lnk_collect_obj_chunks_task)
{
  U64                           obj_idx  = task_id;
  LNK_CollectObjChunksTaskData *task     = raw_task;
  LNK_Obj                      *obj      = task->obj_arr[obj_idx];
  LNK_ChunkList                *list_ptr = &task->list_arr[obj_idx];
  *list_ptr = lnk_obj_search_chunks(arena, obj, task->name, task->postfix, task->collect_discarded);
}

internal LNK_ChunkList *
lnk_collect_obj_chunks(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, String8 name, String8 postfix, B32 collect_discarded)
{
  LNK_CollectObjChunksTaskData task_data = {0};
  task_data.obj_arr                      = obj_arr;
  task_data.name                         = name;
  task_data.postfix                      = postfix;
  task_data.list_arr                     = push_array_no_zero(arena->v[0], LNK_ChunkList, obj_count);
  task_data.collect_discarded            = collect_discarded;
  tp_for_parallel(tp, arena, obj_count, lnk_collect_obj_chunks_task, &task_data);
  return task_data.list_arr;
}

internal
THREAD_POOL_TASK_FUNC(lnk_symbol_collector)
{
  LNK_SymbolCollector *task  = raw_task;
  Rng1U64              range = task->range_arr[task_id];
  LNK_SymbolList      *list  = &task->out_arr[task_id];
  for (U64 obj_idx = range.min; obj_idx < range.max; ++obj_idx) {
    LNK_Obj *obj = &task->in_arr.v[obj_idx].data;
    for (LNK_SymbolNode *node = obj->symbol_list.first; node != 0; node = node->next) {
      if (node->data->type == task->type) {
        lnk_symbol_list_push(arena, list, node->data);
      }
    }
  }
}

internal LNK_SymbolList
lnk_run_symbol_collector(TP_Context *tp, TP_Arena *arena, LNK_ObjNodeArray arr, LNK_SymbolType symbol_type)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  LNK_SymbolCollector task_data;
  task_data.type      = symbol_type;
  task_data.range_arr = tp_divide_work(scratch.arena, arr.count, tp->worker_count);
  task_data.in_arr    = arr;
  task_data.out_arr   = push_array(scratch.arena, LNK_SymbolList, tp->worker_count);

  tp_for_parallel(tp, arena, tp->worker_count, lnk_symbol_collector, &task_data);

  LNK_SymbolList list = {0};
  for (U64 ithread = 0; ithread < tp->worker_count; ++ithread) {
    lnk_symbol_list_concat_in_place(&list, &task_data.out_arr[ithread]);
  }

  scratch_end(scratch);
  ProfEnd();
  return list;
}

internal
THREAD_POOL_TASK_FUNC(lnk_default_lib_collector)
{
  LNK_DefaultLibCollector *task   = raw_task;
  Rng1U64                  range  = task->range_arr[task_id];
  String8List             *result = &task->out_arr[task_id];
  for (U64 obj_idx = range.min; obj_idx < range.max; obj_idx += 1) {
    LNK_Obj *obj = &task->in_arr.v[obj_idx].data;
    for (LNK_Directive *dir = obj->directive_info.v[LNK_CmdSwitch_DefaultLib].first; dir != 0; dir = dir->next) {
      str8_list_concat_in_place(result, &dir->value_list);
    }
  }
}

internal LNK_InputLibList
lnk_collect_default_lib_obj_arr(TP_Context *tp, TP_Arena *arena, LNK_ObjNodeArray arr)
{
  Temp scratch = scratch_begin(0,0);

  LNK_DefaultLibCollector task_data;
  task_data.range_arr = tp_divide_work(scratch.arena, arr.count, tp->worker_count);
  task_data.in_arr    = arr;
  task_data.out_arr   = push_array(scratch.arena, LNK_InputLibList, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, lnk_default_lib_collector, &task_data);

  String8List result = str8_list_arr_concat(task_data.out_arr, tp->worker_count);

  scratch_end(scratch);
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_manifest_dependency_collector)
{
  LNK_ManifestDependencyCollector *task  = raw_task;
  Rng1U64                          range = task->range_arr[task_id];
  String8List                     *list  = &task->out_arr[task_id];

  LNK_ObjNode *obj_ptr = &task->in_arr[range.min];
  LNK_ObjNode *obj_opl = &task->in_arr[range.max];

  for (; obj_ptr < obj_opl; obj_ptr += 1) {
    LNK_Obj           *obj  = &obj_ptr->data;
    LNK_DirectiveList *dirs = &obj->directive_info.v[LNK_CmdSwitch_ManifestDependency];
    for (LNK_Directive *dir = dirs->first; dir != 0; dir = dir->next) {
      String8List dep = str8_list_copy(arena, &dir->value_list);
      str8_list_concat_in_place(list, &dep);
    }
  }
}

internal String8List
lnk_collect_manifest_dependency_list(TP_Context *tp, TP_Arena *arena, LNK_ObjNodeArray obj_node_arr)
{
  Temp scratch = scratch_begin(arena->v, arena->count);

  LNK_ManifestDependencyCollector task_data = {0};
  task_data.in_arr                          = obj_node_arr.v;
  task_data.out_arr                         = push_array(scratch.arena, String8List, tp->worker_count);
  task_data.range_arr                       = tp_divide_work(scratch.arena, obj_node_arr.count, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, lnk_manifest_dependency_collector, &task_data);

  String8List result = str8_list_arr_concat(task_data.out_arr, tp->worker_count);

  scratch_end(scratch);
  return result;
}

internal void
lnk_sect_defn_list_push_node(LNK_SectDefnList *list, LNK_SectDefn *node)
{
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
}

internal LNK_SectDefn *
lnk_sect_defn_list_push(Arena *arena, LNK_SectDefnList *list, LNK_Obj *obj, String8 name, U64 idx, COFF_SectionFlags flags)
{
  LNK_SectDefn *node = push_array_no_zero(arena, LNK_SectDefn, 1);
  node->next         = 0;
  node->obj          = obj;
  node->name         = name;
  node->idx          = idx;
  node->flags        = flags;
  lnk_sect_defn_list_push_node(list, node);
  return node;
}

internal void
lnk_sect_defn_list_concat_in_place(LNK_SectDefnList *list, LNK_SectDefnList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
lnk_sect_defn_list_concat_in_place_arr(LNK_SectDefnList *list, LNK_SectDefnList *to_concat_arr, U64 count)
{
  SLLConcatInPlaceArray(list, to_concat_arr, count);
}

internal
THREAD_POOL_TASK_FUNC(lnk_obj_initer)
{
  LNK_ObjIniter *task    = raw_task;
  LNK_InputObj  *input   = task->inputs[task_id];
  LNK_Obj       *obj     = &task->obj_node_arr[task_id].data;
  U64            obj_idx = task->obj_id_base + task_id;
  
  //
  // parse obj header
  //
  COFF_FileHeaderInfo coff_info              = coff_file_header_info_from_data(input->data);
  String8             raw_coff_section_table = str8_substr(input->data, coff_info.section_table_range);
  String8             raw_coff_symbol_table  = str8_substr(input->data, coff_info.symbol_table_range);
  String8             raw_coff_string_table  = str8_substr(input->data, coff_info.string_table_range);

  //
  // error check: section table / symbol table / string table
  //
  if (raw_coff_section_table.size != dim_1u64(coff_info.section_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read section header table");
  }
  if (raw_coff_symbol_table.size != dim_1u64(coff_info.symbol_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read symbol table");
  }
  if (raw_coff_string_table.size != dim_1u64(coff_info.string_table_range)) {
    lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "corrupted file, unable to read string table");
  }

  U64          chunk_count    = coff_info.section_count_no_null + /* :common_block */ 1;
  String8      *sect_name_arr = push_array_no_zero(arena, String8,      chunk_count);
  String8      *sect_sort_arr = push_array_no_zero(arena, String8,      chunk_count);
  LNK_Chunk    *chunk_arr     = push_array(arena,         LNK_Chunk,    chunk_count);
  LNK_ChunkPtr *chunk_ptr_arr = push_array_no_zero(arena, LNK_ChunkPtr, chunk_count);

  for (U64 chunk_idx = 0; chunk_idx < chunk_count; chunk_idx += 1) {
    chunk_ptr_arr[chunk_idx] = &chunk_arr[chunk_idx];
  }

  //
  // setup :common_block
  //

  U64 common_block_idx = chunk_count - 1;
  sect_name_arr[common_block_idx] = str8_lit(".bss");
  sect_sort_arr[common_block_idx] = str8_lit("~");

  LNK_Chunk *master_common_block    = &chunk_arr[common_block_idx];
  master_common_block->ref          = lnk_chunk_ref(0,0); // :chunk_ref_assign
  master_common_block->align        = 1;
  master_common_block->is_discarded = 0;
  master_common_block->sort_chunk   = 0;
  master_common_block->type         = LNK_Chunk_List;
  master_common_block->sort_idx     = sect_sort_arr[common_block_idx];
  master_common_block->input_idx    = LNK_MakeChunkInputIdx(obj_idx, common_block_idx);
  master_common_block->flags        = LNK_BSS_SECTION_FLAGS;
  master_common_block->associate    = 0;
  master_common_block->u.list       = push_array(arena, LNK_ChunkList, 1);
  master_common_block->obj          = obj;
  lnk_chunk_set_debugf(arena, master_common_block, "obj[%llx] master common block", obj_idx);

  //
  // parse section table
  //
  COFF_SectionHeader *coff_section_table = (COFF_SectionHeader *)raw_coff_section_table.str;
  for (U64 sect_idx = 0; sect_idx < coff_info.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *coff_sect_header = &coff_section_table[sect_idx];

    // read name
    String8 sect_name = coff_name_from_section_header(raw_coff_string_table, coff_sect_header);
    
    // parse name
    coff_parse_section_name(sect_name, &sect_name_arr[sect_idx], &sect_sort_arr[sect_idx]);

    // find contents
    String8 sect_data;
    if (coff_sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
      sect_data = str8(0, coff_sect_header->fsize);
    } else {
      if (coff_sect_header->fsize > 0) {
        Rng1U64 sect_range = rng_1u64(coff_sect_header->foff, coff_sect_header->foff + coff_sect_header->fsize);
        sect_data = str8_substr(input->data, sect_range);

        if (contains_1u64(coff_info.header_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.header_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into file header)", sect_name, sect_idx+1);
        }
        if (contains_1u64(coff_info.section_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.section_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into section header table)", sect_name, sect_idx+1);
        }
        if (contains_1u64(coff_info.symbol_table_range, coff_sect_header->foff) ||
            (coff_sect_header->fsize > 0 && contains_1u64(coff_info.symbol_table_range, sect_range.max-1))) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data (file offsets point into symbol table)", sect_name, sect_idx+1);
        }
        if (dim_1u64(sect_range) != coff_sect_header->fsize) {
          lnk_error_with_loc(LNK_Error_IllData, input->path, input->lib_path, "header (%S No. %#llx) defines out of bounds section data", sect_name, sect_idx+1);
        }
      } else {
        sect_data = str8_zero();
      }
    }

    // fill out chunk
    LNK_Chunk *chunk    = &chunk_arr[sect_idx];
    chunk->align        = coff_align_size_from_section_flags(coff_sect_header->flags);
    chunk->is_discarded = !!(coff_sect_header->flags & COFF_SectionFlag_LnkRemove);
    chunk->sort_chunk   = 1;
    chunk->type         = LNK_Chunk_Leaf;
    chunk->sort_idx     = sect_sort_arr[sect_idx];
    chunk->input_idx    = LNK_MakeChunkInputIdx(obj_idx, sect_idx);
    chunk->flags        = coff_sect_header->flags;
    chunk->u.leaf       = sect_data;
    chunk->obj          = obj;
    lnk_chunk_set_debugf(arena, chunk, "obj[%llx] sect[%llx]", obj_idx, sect_idx);
  }

  //
  // :function_pad_min
  //
  U64 function_pad_min;
  if (task->function_pad_min) {
    function_pad_min = *task->function_pad_min;
  } else {
    function_pad_min = lnk_get_default_function_pad_min(coff_info.machine);
  }

  //
  // convert from COFF
  //
  void            *coff_symbol_table = raw_coff_symbol_table.str;
  LNK_SymbolArray  symbol_arr        = lnk_symbol_array_from_coff(arena, obj, input->path, input->lib_path, coff_info.is_big_obj, function_pad_min, coff_info.section_count_no_null, coff_section_table, coff_info.symbol_count, coff_symbol_table, raw_coff_string_table, chunk_ptr_arr, master_common_block);
  LNK_SymbolList   symbol_list       = lnk_symbol_list_from_array(arena, symbol_arr);
  LNK_RelocList   *reloc_list_arr    = lnk_reloc_list_array_from_coff(arena, coff_info.machine, input->data, coff_info.section_count_no_null, coff_section_table, chunk_ptr_arr, symbol_arr);

  //
  // parse directives
  //
  LNK_DirectiveInfo directive_info = lnk_directive_info_from_sections(arena, input->path, input->lib_path, coff_info.section_count_no_null, reloc_list_arr, sect_name_arr, chunk_arr);

  // parse exports
  LNK_ExportParseList export_parse = {0};
  for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Export].first; dir != 0; dir = dir->next) {
    lnk_parse_export_directive(arena, &export_parse, dir->value_list, input->path, input->lib_path);
  }

  // push /export symbols
  for (LNK_ExportParse *exp = export_parse.first; exp != 0; exp = exp->next) {
    LNK_Symbol *symbol = lnk_make_undefined_symbol(arena, exp->name, LNK_SymbolScopeFlag_Main);
    lnk_symbol_list_push(arena, &symbol_list, symbol);
  }

  // push /include symbols 
  String8List include_symbol_list = {0};
  for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Include].first; dir != 0; dir = dir->next) {
    str8_list_concat_in_place(&include_symbol_list, &dir->value_list);
  }

  // parse /alternatename
  LNK_AltNameList alt_name_list = {0};
  for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_AlternateName].first; dir != 0; dir = dir->next) {
    String8 *invalid_string = lnk_parse_alt_name_directive_list(arena, dir->value_list, &alt_name_list);
    if (invalid_string != 0) {
      lnk_error_with_loc(LNK_Error_Cmdl, input->path, input->lib_path, "invalid syntax \"%S\", expected format \"FROM=TO\"", *invalid_string);
    }
  }


  // fill out obj
  obj->data                = input->data;
  obj->path                = push_str8_copy(arena, input->path);
  obj->lib_path            = push_str8_copy(arena, input->lib_path);
  obj->input_idx           = obj_idx;
  obj->machine             = coff_info.machine;
  obj->chunk_count         = chunk_count;
  obj->sect_count          = coff_info.section_count_no_null;
  obj->sect_name_arr       = sect_name_arr;
  obj->sect_sort_arr       = sect_sort_arr;
  obj->chunk_arr           = chunk_ptr_arr;
  obj->symbol_list         = symbol_list;
  obj->sect_reloc_list_arr = reloc_list_arr;
  obj->directive_info      = directive_info;
  obj->export_parse        = export_parse;
  obj->include_symbol_list = include_symbol_list;
  obj->alt_name_list       = alt_name_list;
}

internal
THREAD_POOL_TASK_FUNC(lnk_obj_new_sect_scanner)
{
  LNK_ObjNewSectScanner *task = raw_task;

  Rng1U64    range = task->range_arr[task_id];
  HashTable *ht    = hash_table_init(arena, 128);

  for (U64 obj_idx = range.min; obj_idx < range.max; obj_idx += 1) {
    LNK_Obj *obj = &task->obj_node_arr[obj_idx].data;

    for (U64 chunk_idx = 0; chunk_idx < obj->chunk_count; chunk_idx += 1) {
      String8           sect_name  = obj->sect_name_arr[chunk_idx];
      COFF_SectionFlags sect_flags = obj->chunk_arr[chunk_idx]->flags & ~COFF_SectionFlags_LnkFlags;

      KeyValuePair *is_present = hash_table_search_string(ht, sect_name);
      if (is_present) {
        if (lnk_is_error_code_active(LNK_Warning_SectionFlagsConflict)) {
          LNK_SectDefn *defn = is_present->value_raw;
          if (defn->flags != sect_flags) {
            lnk_sect_defn_list_push(arena, &task->defn_arr[task_id], obj, sect_name, chunk_idx, sect_flags);
          }
        }
      } else {
        LNK_SectDefn *defn = lnk_sect_defn_list_push(arena, &task->defn_arr[task_id], obj, sect_name, chunk_idx, sect_flags);
        hash_table_push_string_raw(arena, ht, sect_name, defn);
      }
    }
  }
}

LNK_CHUNK_VISITOR_SIG(lnk_chunk_get_count_cb)
{
  U64 *counter = (U64 *)ud;
  *counter += 1;
  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_chunk_counter)
{
  U64               obj_idx = task_id;
  LNK_ChunkCounter *task    = raw_task;
  LNK_Obj          *obj     = &task->obj_arr[obj_idx].data;
  for (U64 chunk_idx = 0; chunk_idx < obj->chunk_count; chunk_idx += 1) {
    String8      name  = obj->sect_name_arr[chunk_idx];
    LNK_Chunk   *chunk = obj->chunk_arr[chunk_idx];
    LNK_Section *sect  = lnk_section_table_search(task->sectab, name);

    U64 count = 0;
    lnk_visit_chunks(0, chunk, lnk_chunk_get_count_cb, &count);

    task->chunk_counts[sect->id][obj_idx] += count;
  }
}

internal
LNK_CHUNK_VISITOR_SIG(lnk_chunk_ref_assign)
{
  LNK_ChunkRefAssign *ctx = ud;

  // alloc chunk id
  U64 chunk_id = *ctx->chunk_id;
  *ctx->chunk_id += 1;
  
  // set chunk ref
  chunk->ref = lnk_chunk_ref(sect_id, chunk_id);

  // keep visiting chunks
  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_chunk_ref_assigner)
{
  LNK_ChunkRefAssigner *task  = raw_task;
  Rng1U64               range = task->range_arr[task_id];

  for (U64 obj_idx = range.min; obj_idx < range.max; obj_idx += 1) {
    LNK_Obj *obj = &task->obj_arr[obj_idx].data;

    for (U64 chunk_idx = 0; chunk_idx < obj->chunk_count; chunk_idx += 1) {
      String8    name  = obj->sect_name_arr[chunk_idx];
      String8    sort  = obj->sect_sort_arr[chunk_idx];
      LNK_Chunk *chunk = obj->chunk_arr[chunk_idx];

      // :find_chunk_section
      LNK_Section *sect = lnk_section_table_search(task->sectab, name);

      // :chunk_ref_assign
      LNK_ChunkRefAssign ctx = {0};
      ctx.cman               = sect->cman;
      ctx.chunk_id           = &task->chunk_ids[sect->id][obj_idx];
      lnk_visit_chunks(sect->id, chunk, lnk_chunk_ref_assign, &ctx);

      // push to section chunk list
      LNK_ChunkList **chunk_list_arr_arr = sort.size ? task->chunk_list_arr_arr : task->nosort_chunk_list_arr_arr;
      lnk_chunk_list_push(arena, &chunk_list_arr_arr[sect->id][task_id], chunk);
    }
  }
}

internal LNK_ObjNodeArray
lnk_obj_list_push_parallel(TP_Context        *tp,
                           TP_Arena          *arena,
                           LNK_ObjList       *obj_list,
                           LNK_SectionTable  *sectab,
                           U64               *function_pad_min,
                           U64                input_count,
                           LNK_InputObj     **inputs)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);
  
  U64              obj_id_base = obj_list->count;
  LNK_ObjNodeArray obj_arr     = lnk_obj_list_reserve(arena->v[0], obj_list, input_count);
  
  ProfBegin("Obj Initer");
  {
    LNK_ObjIniter task    = {0};
    task.inputs           = inputs;
    task.obj_id_base      = obj_id_base;
    task.obj_node_arr     = obj_arr.v;
    task.function_pad_min = function_pad_min;
    tp_for_parallel(tp, arena, input_count, lnk_obj_initer, &task);
  }
  ProfEnd();
  
  if (sectab) {
    ProfBegin("Section Table Update");
    {
      TP_Temp temp = tp_temp_begin(arena);

      LNK_ObjNewSectScanner task;
      task.range_arr    = tp_divide_work(arena->v[0], obj_arr.count, tp->worker_count);
      task.obj_node_arr = obj_arr.v;
      task.defn_arr     = push_array(arena->v[0], LNK_SectDefnList, tp->worker_count);
      task.conf_arr     = push_array(arena->v[0], LNK_SectDefnList, tp->worker_count);
      tp_for_parallel(tp, arena, tp->worker_count, lnk_obj_new_sect_scanner, &task);


      LNK_SectDefnList defn_list = {0};
      LNK_SectDefnList conf_list = {0};
      lnk_sect_defn_list_concat_in_place_arr(&defn_list, task.defn_arr, tp->worker_count);
      lnk_sect_defn_list_concat_in_place_arr(&conf_list, task.conf_arr, tp->worker_count);


      HashTable *ht = hash_table_init(arena->v[0], 128);
      for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != 0; sect_node = sect_node->next) {
        LNK_Section *sect = &sect_node->data;
        hash_table_push_string_u64(arena->v[0], ht, sect->name, sect->flags);
      }


      LNK_SectDefnList new_list = {0};
      for (LNK_SectDefn *curr = defn_list.first, *next; curr != 0; curr = next) {
        next = curr->next;
        curr->next = 0;

        KeyValuePair *is_present = hash_table_search_string(ht, curr->name);
        if (is_present) {
          if (lnk_is_error_code_active(LNK_Warning_SectionFlagsConflict)) {
            COFF_SectionFlags flags = is_present->value_u64;
            if (flags != curr->flags) {
              lnk_sect_defn_list_push_node(&conf_list, curr);
            } else {
              // section is present or is in new_list
            }
          }
        } else {
          lnk_sect_defn_list_push_node(&new_list, curr);
          hash_table_push_string_u64(arena->v[0], ht, curr->name, curr->flags);
        }
      }


      for (LNK_SectDefn *defn = conf_list.first; defn != 0; defn = defn->next) {
        KeyValuePair *is_present = hash_table_search_string(ht, defn->name);
        if (!is_present) {
          InvalidPath;
        }
        U64               sect_number        = (defn->idx + 1);
        COFF_SectionFlags expected_flags     = is_present->value_u64;
        String8           expected_flags_str = coff_string_from_section_flags(scratch.arena, expected_flags);
        String8           current_flags_str  = coff_string_from_section_flags(scratch.arena, defn->flags);
        lnk_error_obj(LNK_Warning_SectionFlagsConflict, defn->obj, "detected section flags conflict in %S(No. %X); expected {%S} but got {%S}", defn->name, sect_number, expected_flags_str, current_flags_str);
      }


      // push new sections for :find_chunk_section
      for (LNK_SectDefn *curr = new_list.first; curr != 0; curr = curr->next) {
        lnk_section_table_push(sectab, curr->name, curr->flags & ~COFF_SectionFlags_LnkFlags);
      }

      tp_temp_end(temp);
    }
    ProfEnd();

    ProfBegin("Count Chunks Per Section");
    U64 **chunk_ids;
    {
      U64 **chunk_counts = push_array_no_zero(scratch.arena, U64 *, sectab->id_max);
      for (U64 sect_idx = 0; sect_idx < sectab->id_max; sect_idx += 1) {
        chunk_counts[sect_idx] = push_array(scratch.arena, U64, obj_arr.count);
      }

      LNK_ChunkCounter task = {0};
      task.sectab               = sectab;
      task.obj_arr          = obj_arr.v;
      task.chunk_counts     = chunk_counts;
      tp_for_parallel(tp, 0, obj_arr.count, lnk_chunk_counter, &task);

      chunk_ids = chunk_counts;
      for (U64 sect_idx = 1; sect_idx < sectab->id_max; sect_idx += 1) {
        LNK_Section *sect = lnk_section_table_search_id(sectab, sect_idx);
        if (!sect) continue;
        for (U64 obj_idx = 0; obj_idx < obj_arr.count; obj_idx += 1) {
          U64 chunk_id_base = sect->cman->total_chunk_count;
          sect->cman->total_chunk_count += chunk_counts[sect_idx][obj_idx];
          chunk_ids[sect_idx][obj_idx] = chunk_id_base;
        }
      }
    }
    ProfEnd();

    ProfBegin("Assign Chunk Refs");
    {
      LNK_ChunkRefAssigner task;
      task.sectab                        = sectab;
      task.range_arr                 = tp_divide_work(scratch.arena, obj_arr.count, tp->worker_count);
      task.chunk_ids                 = chunk_ids;
      task.obj_arr                   = obj_arr.v;
      task.nosort_chunk_list_arr_arr = lnk_make_chunk_list_arr_arr(scratch.arena, sectab->id_max, tp->worker_count);
      task.chunk_list_arr_arr        = lnk_make_chunk_list_arr_arr(scratch.arena, sectab->id_max, tp->worker_count);
      tp_for_parallel(tp, arena, tp->worker_count, lnk_chunk_ref_assigner, &task);

      // merge chunks
      for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != 0; sect_node = sect_node->next) {
        LNK_Section *sect = &sect_node->data;
        lnk_chunk_list_concat_in_place_arr(sect->nosort_chunk->u.list, task.nosort_chunk_list_arr_arr[sect->id], tp->worker_count);
        lnk_chunk_list_concat_in_place_arr(sect->root->u.list, task.chunk_list_arr_arr[sect->id], tp->worker_count);
      }
    }
    ProfEnd();
  }
  
  ProfEnd();
  scratch_end(scratch);
  return obj_arr;
}

internal LNK_SymbolArray
lnk_symbol_array_from_coff(Arena              *arena,
                           LNK_Obj            *obj,
                           String8             obj_path,
                           String8             lib_path,
                           B32                 is_big_obj,
                           U64                 function_pad_min,
                           U64                 sect_count,
                           COFF_SectionHeader *section_table,
                           U64                 symbol_count,
                           void               *symbol_table,
                           String8             string_table,
                           LNK_ChunkPtr       *chunk_table,
                           LNK_Chunk          *master_common_block)
{
  if (function_pad_min) {
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      // read symbol
      if (is_big_obj) {
        symbol = coff_parse_symbol32(string_table, &((COFF_Symbol32 *)symbol_table)[symbol_idx]);
      } else {
        symbol = coff_parse_symbol16(string_table, &((COFF_Symbol16 *)symbol_table)[symbol_idx]);
      }

      // is this a function symbol?
      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular && COFF_SymbolType_IsFunc(symbol.type)) {
        if (symbol.section_number == 0 || symbol.section_number > sect_count) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "out ouf bounds section index in symbol \"%S (%u)\"", symbol.name, symbol.section_number);
        }
        if (symbol.value > section_table[symbol.section_number-1].fsize) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "out of bounds section offset in symbol \"%S (%u)\"", symbol.name, symbol.value);
        }

        LNK_Chunk *chunk = chunk_table[symbol.section_number-1];
        if (symbol.value > 0) {
          // convert leaf to list
          //
          // there is no way to know up front how many splits we have,
          // so lazily convert chunks when see two or more functions
          // in a section
          if (chunk->type == LNK_Chunk_Leaf) {
            // make a list chunk
            LNK_Chunk *chunk_list    = push_array(arena, LNK_Chunk, 1);
            chunk_list->type         = LNK_Chunk_List;
            chunk_list->align        = chunk->align;
            chunk_list->is_discarded = chunk->is_discarded;
            chunk_list->sort_idx     = chunk->sort_idx;
            chunk_list->input_idx    = chunk->input_idx;
            chunk_list->flags        = chunk->flags;
            chunk_list->u.list       = push_array(arena, LNK_ChunkList, 1);
            chunk_list->obj          = obj;
            lnk_chunk_set_debugf(arena, chunk_list, "function chunk list for %S", symbol.name);

            // update properties on first chunk
            chunk->min_size   = function_pad_min;
            chunk->sort_chunk = 0;
            chunk->sort_idx   = str8_zero();
            chunk->input_idx  = 0;

            // push leaf to list
            lnk_chunk_list_push(arena, chunk_list->u.list, chunk);

            // set list as target chunk
            chunk = chunk_list;

            // set list chunk to be head of this section
            chunk_table[symbol.section_number-1] = chunk_list;
          }

          // find chunk that is near symbol
          U64            offset_cursor = 0;
          LNK_ChunkNode *current       = chunk->u.list->last;
          for (LNK_ChunkNode *c = chunk->u.list->first; c != 0; c = c->next) {
            Assert(c->data->type == LNK_Chunk_Leaf);
            if (offset_cursor + c->data->u.leaf.size >= symbol.value) {
              current = c;
              break;
            }
            offset_cursor += c->data->u.leaf.size;
          }
          Assert(current->data->type == LNK_Chunk_Leaf);

          if (offset_cursor < symbol.value) {
            // bifurcate chunk at symbol offset
            U64     split_pos        = symbol.value - offset_cursor;
            Rng1U64 left_data_range  = rng_1u64(0, split_pos);
            Rng1U64 right_data_range = rng_1u64(left_data_range.max, current->data->u.leaf.size);
            String8 left_data        = str8_substr(current->data->u.leaf, left_data_range);
            String8 right_data       = str8_substr(current->data->u.leaf, right_data_range);

            // create new chunk
            LNK_Chunk *split_chunk    = push_array(arena, LNK_Chunk, 1);
            split_chunk->type         = LNK_Chunk_Leaf;
            split_chunk->align        = current->data->align;
            split_chunk->min_size     = function_pad_min;
            split_chunk->is_discarded = current->data->is_discarded;
            split_chunk->flags        = current->data->flags;
            split_chunk->u.leaf       = right_data;
            split_chunk->obj          = obj;
            lnk_chunk_set_debugf(arena, split_chunk, "chunk split on function %S sect %x split pos %#llx", symbol.name, symbol.section_number, split_pos);

            LNK_ChunkNode *split_node = push_array(arena, LNK_ChunkNode, 1);
            split_node->data = split_chunk;

            // update split chunk data
            current->data->u.leaf = left_data;

            // insert split chunk after current chunk 
            if (split_node->next == 0) {
              chunk->u.list->last = split_node;
            }
            split_node->next = current->next;
            current->next = split_node;
            chunk->u.list->count += 1;
          }
        }
      }
    }
  }

  LNK_SymbolArray symbol_array = {0};
  symbol_array.count           = symbol_count;
  symbol_array.v               = push_array(arena, LNK_Symbol, symbol_array.count);

  COFF_ParsedSymbol parsed_symbol;
  for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += (1 + parsed_symbol.aux_symbol_count)) {
    void *aux_symbols;
    if (is_big_obj) {
      COFF_Symbol32 *ptr = &((COFF_Symbol32 *)symbol_table)[symbol_idx];
      parsed_symbol      = coff_parse_symbol32(string_table, ptr);
      aux_symbols        = parsed_symbol.aux_symbol_count ? ptr+1 : 0;
    } else {
      COFF_Symbol16 *ptr = (COFF_Symbol16 *)symbol_table + symbol_idx;
      parsed_symbol      = coff_parse_symbol16(string_table, ptr);
      aux_symbols        = parsed_symbol.aux_symbol_count ? ptr+1 : 0;
    }

    if (symbol_idx + parsed_symbol.aux_symbol_count + 1 > symbol_count) {
      lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "symbol %S (No. %llx) has out of bounds aux symbol count %llu", parsed_symbol.name, symbol_idx, parsed_symbol.aux_symbol_count);
    }

    COFF_SymbolValueInterpType interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
    switch (interp) {
      case COFF_SymbolValueInterp_Regular: {
        if (parsed_symbol.section_number == 0 || parsed_symbol.section_number > sect_count) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "symbol %S (No. %llx) has out ouf bounds section index %x", parsed_symbol.name, symbol_idx, parsed_symbol.section_number);
        }
        if (parsed_symbol.value > section_table[parsed_symbol.section_number-1].fsize) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "symbol %S (No. %llx) has out of bounds section offset %x into section %x", parsed_symbol.name, symbol_idx, parsed_symbol.value, parsed_symbol.section_number);
        }


        LNK_DefinedSymbolVisibility visibility = LNK_DefinedSymbolVisibility_Static;
        if (parsed_symbol.storage_class == COFF_SymStorageClass_External) {
          visibility = LNK_DefinedSymbolVisibility_Extern;
        }
        LNK_DefinedSymbolFlags flags = 0;
        if (COFF_SymbolType_IsFunc(parsed_symbol.type)) {
          flags |= LNK_DefinedSymbolFlag_IsFunc;
        }


        LNK_Chunk             *chunk     = chunk_table[parsed_symbol.section_number-1];
        U64                    offset    = parsed_symbol.value;
        COFF_ComdatSelectType  selection = COFF_ComdatSelect_Any;
        U64                    check_sum = 0;


        B32 is_comdat = (section_table[parsed_symbol.section_number-1].flags & COFF_SectionFlag_LnkCOMDAT) &&
                        parsed_symbol.value == 0 &&
                        parsed_symbol.aux_symbol_count > 0 &&
                        parsed_symbol.type.u.lsb == COFF_SymType_Null &&
                        parsed_symbol.storage_class == COFF_SymStorageClass_Static;
        if (is_comdat) {
          COFF_SymbolSecDef *secdef = aux_symbols;

          selection = secdef->selection;
          check_sum = secdef->check_sum;

          // create association link between chunks
          if (secdef->selection == COFF_ComdatSelect_Associative) {
            U32 secdef_number = secdef->number_lo;

            // promote secdef number to 32 bits
            if (is_big_obj) {
              secdef_number |= (U32)secdef->number_hi << 16;
            }

            // associate chunks
            if (secdef_number > 0 && secdef_number <= sect_count) {
              LNK_Chunk *head_chunk      = chunk_table[secdef_number-1];
              LNK_Chunk *associate_chunk = chunk_table[parsed_symbol.section_number-1];
              lnk_chunk_associate(head_chunk, associate_chunk);
            } else {
              lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "symbol %S (No. %llx) has out of bounds section definition number %u", parsed_symbol.name, symbol_idx, secdef_number);
            }
          }
        }


        if (chunk->type == LNK_Chunk_List) {
          LNK_Chunk *closest_chunk = chunk->u.list->last->data;
          U64        offset_cursor = 0;
          for (LNK_ChunkNode *c = chunk->u.list->first; c != 0; c = c->next) {
            if (offset_cursor + c->data->u.leaf.size > offset) {
              closest_chunk = c->data;
              break;
            }
            offset_cursor += c->data->u.leaf.size;
          }
          Assert(offset >= offset_cursor);
          offset -= offset_cursor;
          chunk   = closest_chunk;
        }
        Assert(chunk->type == LNK_Chunk_Leaf);

        lnk_init_defined_symbol_chunk(&symbol_array.v[symbol_idx], parsed_symbol.name, visibility, flags, chunk, offset, selection, check_sum);
        symbol_array.v[symbol_idx].obj = obj;
      } break;
      case COFF_SymbolValueInterp_Weak: {
        if (parsed_symbol.aux_symbol_count == 0) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "weak symbol \"%S (%u)\" must at least one aux symbol", parsed_symbol.name, symbol_idx);
        }
        
        COFF_SymbolWeakExt *weak_ext = aux_symbols;
        if (weak_ext->tag_index >= symbol_count) {
          lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "weak symbol \"%S (%u)\" points to out of bounds symbol", parsed_symbol.name, symbol_idx);
        }

        LNK_Symbol *symbol          = &symbol_array.v[symbol_idx];
        LNK_Symbol *fallback_symbol = &symbol_array.v[weak_ext->tag_index];
        lnk_init_weak_symbol(symbol, parsed_symbol.name, weak_ext->characteristics, fallback_symbol);

        symbol->obj          = obj;
        fallback_symbol->obj = obj;
      } break;
      case COFF_SymbolValueInterp_Undefined: {
        LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
        lnk_init_undefined_symbol(symbol, parsed_symbol.name, LNK_SymbolScopeFlag_Main);
        symbol->obj = obj;
      } break;
      case COFF_SymbolValueInterp_Common: {
        // :common_block
        //
        // TODO: sort chunks on size to reduce bss usage
        LNK_Chunk *chunk = push_array(arena, LNK_Chunk, 1);
        chunk->align     = Min(32, u64_up_to_pow2(parsed_symbol.value)); // link.exe caps align at 32 bytes
        chunk->type      = LNK_Chunk_Leaf;
        chunk->flags     = master_common_block->flags;
        chunk->u.leaf    = str8(0, parsed_symbol.value);
        chunk->obj       = obj;
        lnk_chunk_set_debugf(arena, chunk, "common block %S", parsed_symbol.name);
        lnk_chunk_list_push(arena, master_common_block->u.list, chunk);

        LNK_DefinedSymbolFlags flags = 0;
        if (COFF_SymbolType_IsFunc(parsed_symbol.type)) {
          flags |= LNK_DefinedSymbolFlag_IsFunc;
        }

        LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
        lnk_init_defined_symbol_chunk(symbol, parsed_symbol.name, LNK_DefinedSymbolVisibility_Extern, flags, chunk, 0, COFF_ComdatSelect_Largest, 0);
        symbol->obj = obj;
      } break;
      case COFF_SymbolValueInterp_Abs: {
        // Never code or data, synthetic symbol. COFF spec says bits in value are used
        // as flags in symbol @feat.00, other symbols like @comp.id and @vol.md are undocumented.
        // LLVM uses undocumented mask 0x4800 on @feat.00 to tell if object was compiled with /guard:cf.

        LNK_DefinedSymbolVisibility visibility = LNK_DefinedSymbolVisibility_Static;
        if (parsed_symbol.storage_class == COFF_SymStorageClass_External) {
          visibility = LNK_DefinedSymbolVisibility_Extern;
        }

        LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
        lnk_init_defined_symbol_va(symbol, parsed_symbol.name, visibility, 0, parsed_symbol.value);
        symbol->obj = obj;
      } break;
      case COFF_SymbolValueInterp_Debug: {
      } break;
    }
  }

  return symbol_array;
}

internal LNK_RelocList *
lnk_reloc_list_array_from_coff(Arena *arena, COFF_MachineType machine, String8 coff_data, U64 sect_count, COFF_SectionHeader *coff_sect_arr, LNK_ChunkPtr *chunk_ptr_arr, LNK_SymbolArray symbol_array)
{
  LNK_RelocList *reloc_list_arr = push_array_no_zero(arena, LNK_RelocList, sect_count);
  for (U64 sect_idx = 0; sect_idx < sect_count; ++sect_idx) {
    COFF_SectionHeader *coff_sect_header = &coff_sect_arr[sect_idx];
    COFF_RelocInfo      coff_reloc_info  = coff_reloc_info_from_section_header(coff_data, coff_sect_header);
    COFF_Reloc         *coff_reloc_v     = (COFF_Reloc *)(coff_data.str + coff_reloc_info.array_off);
    LNK_Chunk          *sect_chunk       = chunk_ptr_arr[sect_idx];
    reloc_list_arr[sect_idx] = lnk_reloc_list_from_coff_reloc_array(arena, machine, sect_chunk, symbol_array, coff_reloc_v, coff_reloc_info.count);
  }
  return reloc_list_arr;
}

internal void
lnk_parse_msvc_linker_directive(Arena *arena, String8 obj_path, String8 lib_path, LNK_DirectiveInfo *directive_info, String8 buffer)
{
  Temp scratch = scratch_begin(&arena, 1);

  local_persist B32 init_table = 1;
  local_persist B8  is_legal[LNK_CmdSwitch_Count];
  if (init_table) {
    init_table = 0;
    is_legal[LNK_CmdSwitch_AlternateName]      = 1;
    is_legal[LNK_CmdSwitch_DefaultLib]         = 1;
    is_legal[LNK_CmdSwitch_DisallowLib]        = 1;
    is_legal[LNK_CmdSwitch_EditAndContinue]    = 1;
    is_legal[LNK_CmdSwitch_Entry]              = 1;
    is_legal[LNK_CmdSwitch_Export]             = 1;
    is_legal[LNK_CmdSwitch_FailIfMismatch]     = 1;
    is_legal[LNK_CmdSwitch_GuardSym]           = 1;
    is_legal[LNK_CmdSwitch_Include]            = 1;
    is_legal[LNK_CmdSwitch_InferAsanLibs]      = 1;
    is_legal[LNK_CmdSwitch_InferAsanLibsNo]    = 1;
    is_legal[LNK_CmdSwitch_ManifestDependency] = 1;
    is_legal[LNK_CmdSwitch_Merge]              = 1;
    is_legal[LNK_CmdSwitch_NoDefaultLib]       = 1;
    is_legal[LNK_CmdSwitch_Release]            = 1;
    is_legal[LNK_CmdSwitch_Section]            = 1;
    is_legal[LNK_CmdSwitch_Stack]              = 1;
    is_legal[LNK_CmdSwitch_SubSystem]          = 1;
    is_legal[LNK_CmdSwitch_ThrowingNew]        = 1;
  }
  
  String8 to_parse;
  {
    local_persist const U8 bom_sig[]   = { 0xEF, 0xBB, 0xBF };
    local_persist const U8 ascii_sig[] = { 0x20, 0x20, 0x20 };
    if (MemoryMatch(buffer.str, &bom_sig[0], sizeof(bom_sig))) {
      to_parse = str8_zero();
      lnk_error_with_loc(LNK_Error_IllData, obj_path, lib_path, "TODO: support for BOM encoding");
    } else if (MemoryMatch(buffer.str, &ascii_sig[0], sizeof(ascii_sig))) {
      to_parse = str8_skip(buffer, sizeof(ascii_sig));
    } else {
      to_parse = buffer;
    }
  }
  
  String8List arg_list = lnk_arg_list_parse_windows_rules(scratch.arena, to_parse);
  LNK_CmdLine cmd_line = lnk_cmd_line_parse_windows_rules(scratch.arena, arg_list);

  for (LNK_CmdOption *opt = cmd_line.first_option; opt != 0; opt = opt->next) {
    LNK_CmdSwitchType type = lnk_cmd_switch_type_from_string(opt->string);

    if (type == LNK_CmdSwitch_Null) {
      lnk_error_with_loc(LNK_Warning_UnknownDirective, obj_path, lib_path, "unknown directive \"%S\"", opt->string);
      continue;
    }
    if (!is_legal[type]) {
      lnk_error_with_loc(LNK_Warning_IllegalDirective, obj_path, lib_path, "illegal directive \"%S\"", opt->string);
      continue;
    }

    LNK_Directive *directive = push_array_no_zero(arena, LNK_Directive, 1);
    directive->next          = 0;
    directive->id            = push_str8_copy(arena, opt->string);
    directive->value_list    = str8_list_copy(arena, &opt->value_strings);

    LNK_DirectiveList *directive_list = &directive_info->v[type];
    SLLQueuePush(directive_list->first, directive_list->last, directive);
    ++directive_list->count;
  }
  
  scratch_end(scratch);
}

internal LNK_DirectiveInfo
lnk_directive_info_from_sections(Arena         *arena,
                                 String8        obj_path,
                                 String8        lib_path,
                                 U64            chunk_count,
                                 LNK_RelocList *reloc_list_arr,
                                 String8       *sect_name_arr,
                                 LNK_Chunk     *chunk_arr)
{
  LNK_DirectiveInfo directive_info = {0};
  for (U64 chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
    String8    sect_name  = sect_name_arr[chunk_idx];
    LNK_Chunk *sect_chunk = chunk_arr + chunk_idx;
    if (str8_match_lit(".drectve", sect_name, 0)) {
      if (sect_chunk->type == LNK_Chunk_Leaf) {
        if (sect_chunk->u.leaf.size >= 3) {
          if (~sect_chunk->flags & COFF_SectionFlag_LnkInfo) {
            lnk_error_with_loc(LNK_Warning_IllData, obj_path, lib_path, "%S missing COFF_SectionFlag_LnkInfo", sect_name);
          }
          if (reloc_list_arr[chunk_idx].count > 0) {
            lnk_error_with_loc(LNK_Warning_DirectiveSectionWithRelocs, obj_path, lib_path, "directive section %S(%#x) has relocations", sect_name, (chunk_idx+1));
          }
          lnk_parse_msvc_linker_directive(arena, obj_path, lib_path, &directive_info, sect_chunk->u.leaf);
        } else {
          lnk_error_with_loc(LNK_Warning_IllData, obj_path, lib_path, "unable to parse %S", sect_name);
        }
      } else {
        Assert(!"linker directive section chunk must be of leaf type");
      }
    }
  }
  return directive_info;
}

internal MSCRT_FeatFlags
lnk_obj_get_features(LNK_Obj *obj)
{
  MSCRT_FeatFlags result = 0;
  LNK_Symbol *sym = lnk_symbol_list_search(obj->symbol_list, str8_lit("@feat.00"), 0);
  if (sym) {
    Assert(LNK_Symbol_IsDefined(sym->type));
    Assert(sym->u.defined.value_type == LNK_DefinedSymbolValue_VA);
    result = sym->u.defined.u.va;
  }
  return result;
}

internal U32
lnk_obj_get_comp_id(LNK_Obj *obj)
{
  U32 result = 0;
  LNK_Symbol *sym = lnk_symbol_list_search(obj->symbol_list, str8_lit("@comp.id"), 0);
  if (sym) {
    Assert(LNK_Symbol_IsDefined(sym->type));
    Assert(sym->u.defined.value_type == LNK_DefinedSymbolValue_VA);
    result = sym->u.defined.u.va;
  }
  return result;
}

internal U32
lnk_obj_get_vol_md(LNK_Obj *obj)
{
  U32 result = 0;
  LNK_Symbol *sym = lnk_symbol_list_search(obj->symbol_list, str8_lit("@vol.md"), 0);
  if (sym) {
    Assert(LNK_Symbol_IsDefined(sym->type));
    Assert(sym->u.defined.value_type == LNK_DefinedSymbolValue_VA);
    result = sym->u.defined.u.va;
  }
  return result;
}

