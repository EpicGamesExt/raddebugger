// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////

internal void
lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 text = push_str8fv(scratch.arena, fmt, args);
  
  if (obj->lib_path.size) {
    lnk_error(code, "%S(%S): %S", obj->lib_path, obj->path, text);
  } else {
    lnk_error(code, "%S: %S", obj->path, text);
  }

  va_end(args);
  scratch_end(scratch);
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
    LNK_Obj     *obj = &task->in_arr.v[obj_idx].data;
    String8List list = lnk_parse_default_lib_directive(arena, &obj->directive_info.v[LNK_Directive_DefaultLib]);
    str8_list_concat_in_place(result, &list);
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
    LNK_DirectiveList *dirs = &obj->directive_info.v[LNK_Directive_ManifestDependency];
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
  Temp scratch = scratch_begin(&arena, 1);

  LNK_ObjIniter *task     = raw_task;
  LNK_InputObj  *input    = task->inputs[task_id];
  U64            obj_idx  = task->obj_id_base + task_id;
  LNK_ObjNode   *obj_node = task->obj_node_arr + task_id;
  LNK_Obj       *obj      = &obj_node->data;
  
  // cache path, we need it for error reports and debug stuff
  String8 cached_path     = push_str8_copy(arena, input->path);
  String8 cached_lib_path = push_str8_copy(arena, input->lib_path);

  // parse coff obj
  COFF_HeaderInfo     coff_info      = coff_header_info_from_data(input->data);
  COFF_SectionHeader *coff_sect_arr  = (COFF_SectionHeader *)(input->data.str + coff_info.section_array_off);
  void               *coff_symbols   = input->data.str + coff_info.symbol_off;

  // handle machines we dont support
  if (coff_info.machine != COFF_MachineType_UNKNOWN && coff_info.machine != COFF_MachineType_X64) {
    lnk_error(LNK_Error_UnsupportedMachine, "%S: %S machine is supported", input->path, coff_string_from_machine_type(coff_info.machine));
  }

  U64 chunk_count = 0;
  chunk_count += coff_info.section_count_no_null;
  chunk_count += 1; // :common_block

  String8   *sect_name_arr = push_array_no_zero(arena, String8, chunk_count);
  String8   *sect_sort_arr = push_array_no_zero(arena, String8, chunk_count);
  LNK_Chunk *chunk_arr     = push_array_no_zero(arena, LNK_Chunk, chunk_count);

  // init section name and postfix array
  for (U64 sect_idx = 0; sect_idx < coff_info.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *coff_sect = &coff_sect_arr[sect_idx];

    // read name
    String8 sect_name = coff_section_header_get_name(coff_sect, input->data, coff_info.string_table_off);
    
    // parse section name
    String8 name, postfix;
    coff_parse_section_name(sect_name, &name, &postfix);

    // fill out
    sect_name_arr[sect_idx] = name;
    sect_sort_arr[sect_idx] = postfix;
  }

  // :common_block
  U64 common_block_idx = chunk_count - 1;
  sect_name_arr[common_block_idx] = str8_lit(".bss");
  sect_sort_arr[common_block_idx] = str8_lit("~");

  for (U64 sect_idx = 0; sect_idx < coff_info.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *coff_sect = &coff_sect_arr[sect_idx];

    String8 data;
    if (coff_sect->flags & COFF_SectionFlag_CNT_UNINITIALIZED_DATA) {
      data = str8(0, coff_sect->fsize);
    } else {
      data = str8(input->data.str + coff_sect->foff, coff_sect->fsize);
    }

    LNK_Chunk *chunk    = &chunk_arr[sect_idx];
    chunk->ref          = lnk_chunk_ref(0,0); // :chunk_ref_assign
    chunk->align        = coff_align_size_from_section_flags(coff_sect->flags);
    chunk->min_size     = 0;
    chunk->is_discarded = !!(coff_sect->flags & COFF_SectionFlag_LNK_REMOVE);
    chunk->sort_chunk   = 1;
    chunk->type         = LNK_Chunk_Leaf;
    chunk->sort_idx     = sect_sort_arr[sect_idx];
    chunk->input_idx    = LNK_MakeChunkInputIdx(obj_idx, sect_idx);
    chunk->flags        = coff_sect->flags;
    chunk->associate    = 0;
    chunk->u.leaf       = data;
    lnk_chunk_set_debugf(arena, chunk, "%S: name: %S, obj_idx: 0x%llX isect: 0x%llX", cached_path, sect_name_arr[sect_idx], obj_idx, sect_idx);
  }

  // :common_block
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
  lnk_chunk_set_debugf(arena, master_common_block, "%S: master common block", cached_path);

  LNK_ChunkPtr *chunk_ptr_arr = push_array_no_zero(arena, LNK_ChunkPtr, chunk_count);
  for (U64 i = 0; i < chunk_count; ++i) {
    chunk_ptr_arr[i] = &chunk_arr[i];
  }

  // convert from coff
  B32             is_big_obj     = coff_info.type == COFF_DataType_BIG_OBJ;
  LNK_SymbolArray symbol_arr     = lnk_symbol_array_from_coff(arena, input->data, obj, cached_path, is_big_obj, task->function_pad_min, coff_info.string_table_off, coff_info.section_count_no_null, coff_sect_arr, coff_info.symbol_count, coff_symbols, chunk_ptr_arr, master_common_block);
  LNK_SymbolList  symbol_list    = lnk_symbol_list_from_array(arena, symbol_arr);
  LNK_RelocList  *reloc_list_arr = lnk_reloc_list_array_from_coff(arena, coff_info.machine, input->data, coff_info.section_count_no_null, coff_sect_arr, chunk_ptr_arr, symbol_arr);

  // fill out obj
  obj->data                = input->data;
  obj->path                = cached_path;
  obj->lib_path            = cached_lib_path;
  obj->input_idx           = obj_idx;
  obj->machine             = coff_info.machine;
  obj->chunk_count         = chunk_count;
  obj->sect_count          = coff_info.section_count_no_null;
  obj->sect_name_arr       = sect_name_arr;
  obj->sect_sort_arr       = sect_sort_arr;
  obj->chunk_arr           = chunk_ptr_arr;
  obj->symbol_list         = symbol_list;
  obj->sect_reloc_list_arr = reloc_list_arr;
  obj->directive_info      = lnk_init_directives(arena, cached_path, coff_info.section_count_no_null, sect_name_arr, chunk_arr);

  // parse exports
  LNK_ExportParseList export_parse = {0};
  for (LNK_Directive *dir = obj->directive_info.v[LNK_Directive_Export].first; dir != 0; dir = dir->next) {
    lnk_parse_export_direcive(arena, &obj->export_parse, dir->value_list, obj);
  }

  // push /export symbols
  for (LNK_ExportParse *exp = export_parse.first; exp != 0; exp = exp->next) {
    LNK_Symbol *symbol = lnk_make_undefined_symbol(arena, exp->name, LNK_SymbolScopeFlag_Main);
    lnk_symbol_list_push(arena, &obj->symbol_list, symbol);
  }

  // push /include symbols 
  for (LNK_Directive *dir = obj->directive_info.v[LNK_Directive_Include].first; dir != 0; dir = dir->next) {
    str8_list_concat_in_place(&obj->include_symbol_list, &dir->value_list);
  }

  // parse /alternatename
  for (LNK_Directive *dir = obj->directive_info.v[LNK_Directive_AlternateName].first; dir != 0; dir = dir->next) {
    String8 *invalid_string = lnk_parse_alt_name_directive_list(arena, dir->value_list, &obj->alt_name_list);
    if (invalid_string != 0) {
      lnk_error_obj(LNK_Error_Cmdl, obj, "invalid syntax \"%S\", expected format \"FROM=TO\"", *invalid_string);
    }
  }

  scratch_end(scratch);
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
      COFF_SectionFlags sect_flags = obj->chunk_arr[chunk_idx]->flags & ~COFF_SectionFlags_LNK_FLAGS;

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
    String8 name = obj->sect_name_arr[chunk_idx];
    LNK_Chunk   *chunk = obj->chunk_arr[chunk_idx];
    LNK_Section *sect  = lnk_section_table_search(task->st, name);

    U64 count = 0;
    lnk_visit_chunks(0, chunk, lnk_chunk_get_count_cb, &count);

    task->chunk_count_arr_arr[sect->id][obj_idx] += count;
  }
}

internal
LNK_CHUNK_VISITOR_SIG(lnk_chunk_ref_assign)
{
  LNK_ChunkRefAssign *ctx = ud;

  // alloc chunk id
  U64 chunk_id = ctx->chunk_id_arr_arr[sect_id][ctx->obj_idx];
  ctx->chunk_id_arr_arr[sect_id][ctx->obj_idx] += 1;
  
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
      LNK_Section *sect = lnk_section_table_search(task->st, name);
      Assert(sect);

      // :chunk_ref_assign
      LNK_ChunkRefAssign ctx;
      ctx.cman             = sect->cman;
      ctx.chunk_id_arr_arr = task->chunk_id_arr_arr;
      ctx.obj_idx          = obj_idx;
      lnk_visit_chunks(sect->id, chunk, lnk_chunk_ref_assign, &ctx);

      // push to section chunk list
      LNK_ChunkList **chunk_list_arr_arr = sort.size ? task->chunk_list_arr_arr : task->nosort_chunk_list_arr_arr;
      lnk_chunk_list_push(arena, &chunk_list_arr_arr[sect->id][task_id], chunk);
    }
  }
}

internal LNK_ObjNodeArray
lnk_obj_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_ObjList *obj_list, LNK_SectionTable *st, U64 function_pad_min, U64 input_count, LNK_InputObj **inputs)
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
  
  if (st) {
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
      for (LNK_SectionNode *sect_node = st->list.first; sect_node != 0; sect_node = sect_node->next) {
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
        lnk_section_table_push(st, curr->name, curr->flags & ~COFF_SectionFlags_LNK_FLAGS);
      }

      tp_temp_end(temp);
    }
    ProfEnd();

    ProfBegin("Count Chunks Per Section");
    U64 **chunk_id_arr_arr;
    {
      U64 **chunk_count_arr_arr = push_array_no_zero(scratch.arena, U64 *, st->id_max);
      for (U64 sect_idx = 0; sect_idx < st->id_max; sect_idx += 1) {
        chunk_count_arr_arr[sect_idx] = push_array(scratch.arena, U64, obj_arr.count);
      }

      LNK_ChunkCounter task;
      task.st                  = st;
      task.obj_arr             = obj_arr.v;
      task.chunk_count_arr_arr = chunk_count_arr_arr;
      tp_for_parallel(tp, 0, obj_arr.count, lnk_chunk_counter, &task);

      chunk_id_arr_arr = chunk_count_arr_arr;
      for (U64 sect_idx = 1; sect_idx < st->id_max; sect_idx += 1) {
        LNK_Section *sect = lnk_section_table_search_id(st, sect_idx);
        if (!sect) continue;
        for (U64 obj_idx = 0; obj_idx < obj_arr.count; obj_idx += 1) {
          U64 chunk_id_base = sect->cman->total_chunk_count;
          sect->cman->total_chunk_count += chunk_count_arr_arr[sect_idx][obj_idx];
          chunk_id_arr_arr[sect_idx][obj_idx] = chunk_id_base;
        }
      }
    }
    ProfEnd();

    ProfBegin("Assign Chunk Refs");
    {
      LNK_ChunkRefAssigner task;
      task.st                        = st;
      task.range_arr                 = tp_divide_work(scratch.arena, obj_arr.count, tp->worker_count);
      task.chunk_id_arr_arr          = chunk_id_arr_arr;
      task.obj_arr                   = obj_arr.v;
      task.nosort_chunk_list_arr_arr = lnk_make_chunk_list_arr_arr(scratch.arena, st->id_max, tp->worker_count);
      task.chunk_list_arr_arr        = lnk_make_chunk_list_arr_arr(scratch.arena, st->id_max, tp->worker_count);
      tp_for_parallel(tp, arena, tp->worker_count, lnk_chunk_ref_assigner, &task);

      // merge chunks
      for (LNK_SectionNode *sect_node = st->list.first; sect_node != 0; sect_node = sect_node->next) {
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
                           String8             coff_data,
                           LNK_Obj            *obj,
                           String8             obj_path,
                           B32                 is_big_obj,
                           U64                 function_pad_min,
                           U64                 string_table_off,
                           U64                 sect_count,
                           COFF_SectionHeader *coff_sect_arr,
                           U64                 coff_symbol_count,
                           void               *coff_symbols,
                           LNK_ChunkPtr       *chunk_ptr_arr,
                           LNK_Chunk          *master_common_block)
{
  LNK_SymbolArray symbol_array;
  symbol_array.count = coff_symbol_count;
  symbol_array.v     = push_array(arena, LNK_Symbol, symbol_array.count);
  
  for (U64 symbol_idx = 0; symbol_idx < coff_symbol_count; ++symbol_idx) {
    String8              name;
    U32                  section_number;
    U32                  value;
    COFF_SymStorageClass storage_class;
    COFF_SymbolType      type;
    U64                  aux_symbol_count;
    void                *aux_symbols;
    if (is_big_obj) {
      COFF_Symbol32 *coff_symbol32 = (COFF_Symbol32 *)coff_symbols + symbol_idx;
      name             = coff_read_symbol_name(coff_data, string_table_off, &coff_symbol32->name);
      section_number   = coff_symbol32->section_number;
      value            = coff_symbol32->value;
      storage_class    = coff_symbol32->storage_class;
      type             = coff_symbol32->type;
      aux_symbol_count = coff_symbol32->aux_symbol_count;
      aux_symbols      = coff_symbol32 + 1;
    } else {
      COFF_Symbol16 *coff_symbol16 = (COFF_Symbol16 *)coff_symbols + symbol_idx;
      name             = coff_read_symbol_name(coff_data, string_table_off, &coff_symbol16->name);
      section_number   = coff_symbol16->section_number;
      value            = coff_symbol16->value;
      storage_class    = coff_symbol16->storage_class;
      type             = coff_symbol16->type;
      aux_symbol_count = coff_symbol16->aux_symbol_count;
      aux_symbols      = coff_symbol16 + 1;

      // promote special section numbers to 32 bit
      if (section_number == COFF_SYMBOL_DEBUG_SECTION_16) {
        section_number = COFF_SYMBOL_DEBUG_SECTION;
      } else if (section_number == COFF_SYMBOL_ABS_SECTION_16) {
        section_number = COFF_SYMBOL_ABS_SECTION;
      }
    }

    if (symbol_idx + aux_symbol_count > coff_symbol_count) {
      lnk_error(LNK_Error_IllData, "%S: symbol %S has out of bounds aux symbol count %u", obj_path, name, aux_symbol_count);
    }

    COFF_SymbolValueInterpType interp = coff_interp_symbol(section_number, value, storage_class);
    switch (interp) {
    case COFF_SymbolValueInterp_REGULAR: {
      if (section_number == 0 || section_number > sect_count) {
        lnk_error(LNK_Error_IllData, "%S: out ouf bounds section index in symbol \"%S (%u)\"", obj_path, name, section_number);
        break;
      }

      COFF_SectionHeader *coff_sect_header = &coff_sect_arr[section_number-1];

      if (value > coff_sect_header->fsize) {
        lnk_error(LNK_Error_IllData, "%S: out of bounds section offset in symbol \"%S (%u)\"", obj_path, name, value);
        break;
      }

      LNK_DefinedSymbolVisibility visibility = LNK_DefinedSymbolVisibility_Static;
      if (storage_class == COFF_SymStorageClass_EXTERNAL) {
        visibility = LNK_DefinedSymbolVisibility_Extern;
      }

      LNK_DefinedSymbolFlags flags = 0;
      if (COFF_SymbolType_IsFunc(type)) {
        flags |= LNK_DefinedSymbolFlag_IsFunc;
      }

      LNK_Chunk             *chunk     = chunk_ptr_arr[section_number-1];
      U64                    offset    = value;
      COFF_ComdatSelectType  selection = COFF_ComdatSelectType_ANY;
      U64                    check_sum = 0;

      if (coff_sect_header->flags & COFF_SectionFlag_LNK_COMDAT) {
        B32 has_static_def = value == 0 &&
                             type.u.lsb == COFF_SymType_NULL &&
                             storage_class == COFF_SymStorageClass_STATIC &&
                             aux_symbol_count == 1;
        if (has_static_def) {
          COFF_SymbolSecDef *secdef = aux_symbols;

          selection = secdef->selection;
          check_sum = secdef->check_sum;
          
          if (secdef->selection == COFF_ComdatSelectType_ASSOCIATIVE) {
            U32 secdef_number = secdef->number_lo;
            if (is_big_obj) {
              secdef_number |= (U32)secdef->number_hi << 16;
            }

            if (secdef_number == 0 || secdef_number > sect_count) {
              lnk_error(LNK_Error_IllData, "%S: symbol %u has out of bounds section definition number %u", name, symbol_idx, secdef_number);
            }

            LNK_Chunk *head_chunk      = chunk_ptr_arr[secdef_number-1];
            LNK_Chunk *associate_chunk = chunk_ptr_arr[section_number-1];
            lnk_chunk_associate(arena, head_chunk, associate_chunk);
          }
        }
      }

      if (function_pad_min) {
        if ((flags & LNK_DefinedSymbolFlag_IsFunc)) {
          if (offset > 0) {
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
              chunk_list->associate    = chunk->associate;
              chunk_list->u.list       = push_array(arena, LNK_ChunkList, 1);
              lnk_chunk_set_debugf(arena, chunk_list, "%S: function chunk list for %S", obj_path, name);

              // reset chunk properties
              chunk->align      = 1;
              chunk->min_size   = function_pad_min;
              chunk->sort_chunk = 0;
              chunk->sort_idx   = str8_zero();
              chunk->input_idx  = 0;
              chunk->associate  = 0;

              // push leaf to list
              lnk_chunk_list_push(arena, chunk_list->u.list, chunk);

              // set list as target chunk
              chunk = chunk_list;

              // set list chunk to be head of this section
              chunk_ptr_arr[section_number-1] = chunk_list;
            }

            // find chunk that is near symbol
            U64            cursor  = 0;
            LNK_ChunkNode *current = chunk->u.list->last;
            for (LNK_ChunkNode *c = chunk->u.list->first; c != 0; c = c->next) {
              Assert(c->data->type == LNK_Chunk_Leaf);
              if (cursor + c->data->u.leaf.size > value) {
                current = c;
                break;
              }
              cursor += c->data->u.leaf.size;
            }

            if (cursor < value) {
              // bifurcate chunk at symbol offset
              U64     split_pos        = value - cursor;
              Rng1U64 left_data_range  = rng_1u64(0, split_pos);
              Rng1U64 right_data_range = rng_1u64(left_data_range.max, current->data->u.leaf.size);
              String8 left_data        = str8_substr(current->data->u.leaf, left_data_range);
              String8 right_data       = str8_substr(current->data->u.leaf, right_data_range);

              // create new chunk for new part
              LNK_Chunk *split_chunk    = push_array(arena, LNK_Chunk, 1);
              split_chunk->align        = 1;
              split_chunk->is_discarded = current->data->is_discarded;
              split_chunk->type         = LNK_Chunk_Leaf;
              split_chunk->flags        = current->data->flags;
              split_chunk->u.leaf       = right_data;
              lnk_chunk_set_debugf(arena, chunk, "%S: chunk split on function %S", obj_path, name);

              LNK_ChunkNode *split_node = push_array(arena, LNK_ChunkNode, 1);
              split_node->data = split_chunk;

              // update split chunk data
              current->data->u.leaf = left_data;

              // insert split chunk after current chunk 
              split_node->next = current->next;
              current->next = split_node;
              chunk->u.list->count += 1;

              // point symbol to new split chunk
              chunk  = split_chunk;
              offset = 0;
            } else {
              // chunk was already split at the offset
              chunk  = current->data;
              offset = value - cursor;
            }
          }

          chunk->min_size = function_pad_min;
        }

        // if symbol points to bifurcated section then we have an alias symbol (e.g. memmove is an alias to memcpy)
        // and we need to find chunk that is near symbol
        if (chunk->type == LNK_Chunk_List) {
          U64 cursor = 0;
          for (LNK_ChunkNode *c = chunk->u.list->first; c != 0; c = c->next) {
            if (cursor + c->data->u.leaf.size > value) {
              chunk  = c->data;
              offset = offset - cursor;
              break;
            }
            cursor += c->data->u.leaf.size;
          }
        }
      }

      Assert(chunk->type == LNK_Chunk_Leaf);
      LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
      lnk_init_defined_symbol_chunk(symbol, name, visibility, flags, chunk, offset, selection, check_sum);
      symbol->obj = obj;
    } break;
    case COFF_SymbolValueInterp_WEAK: {
      if (aux_symbol_count == 0) {
        lnk_error(LNK_Error_IllData, "%S: Weak symbol \"%S (%u)\" must at least one aux symbol", obj_path, name, symbol_idx);
      }
      
      COFF_SymbolWeakExt *weak_ext = aux_symbols;
      if (weak_ext->tag_index >= coff_symbol_count) {
        lnk_error(LNK_Error_IllData, "%S: Weak symbol \"%S (%u)\" points to out of bounds symbol", obj_path, name, symbol_idx);
        break;
      }

      LNK_Symbol *symbol          = &symbol_array.v[symbol_idx];
      LNK_Symbol *fallback_symbol = &symbol_array.v[weak_ext->tag_index];
      lnk_init_weak_symbol(symbol, name, weak_ext->characteristics, fallback_symbol);

      symbol->obj          = obj;
      fallback_symbol->obj = obj;
    } break;
    case COFF_SymbolValueInterp_UNDEFINED: {
      LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
      lnk_init_undefined_symbol(symbol, name, LNK_SymbolScopeFlag_Main);
      symbol->obj = obj;
    } break;
    case COFF_SymbolValueInterp_COMMON: {
      // :common_block
      //
      // TODO: sort chunks on size to reduce bss usage
      LNK_Chunk *chunk = push_array(arena, LNK_Chunk, 1);
      chunk->align     = Min(32, u64_up_to_pow2(value)); // link.exe caps align at 32 bytes
      chunk->type      = LNK_Chunk_Leaf;
      chunk->flags     = master_common_block->flags;
      chunk->u.leaf    = str8(0, value);
      lnk_chunk_set_debugf(arena, chunk, "%S: common block %S", obj_path, name);
      lnk_chunk_list_push(arena, master_common_block->u.list, chunk);

      LNK_DefinedSymbolFlags flags = 0;
      if (COFF_SymbolType_IsFunc(type)) {
        flags |= LNK_DefinedSymbolFlag_IsFunc;
      }

      LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
      lnk_init_defined_symbol_chunk(symbol, name, LNK_DefinedSymbolVisibility_Extern, flags, chunk, 0, COFF_ComdatSelectType_LARGEST, 0);
      symbol->obj = obj;
    } break;
    case COFF_SymbolValueInterp_ABS: {
      // Never code or data, synthetic symbol. COFF spec says bits in value are used
      // as flags in symbol @feat.00, other symbols like @comp.id and @vol.md are undocumented.
      // LLVM uses undocumented mask 0x4800 on @feat.00 to tell if object was compiled with /guard:cf.

      LNK_DefinedSymbolVisibility visibility = LNK_DefinedSymbolVisibility_Static;
      if (storage_class == COFF_SymStorageClass_EXTERNAL) {
        visibility = LNK_DefinedSymbolVisibility_Extern;
      }

      LNK_Symbol *symbol = &symbol_array.v[symbol_idx];
      lnk_init_defined_symbol_va(symbol, name, visibility, 0, value);
      symbol->obj = obj;
    } break;
    case COFF_SymbolValueInterp_DEBUG: break;
    }
    
    // skip aux symbols
    symbol_idx += aux_symbol_count;
  }

  return symbol_array;
}

internal LNK_RelocList *
lnk_reloc_list_array_from_coff(Arena *arena, COFF_MachineType machine, String8 coff_data, U64 sect_count, COFF_SectionHeader *coff_sect_arr, LNK_ChunkPtr *chunk_ptr_arr, LNK_SymbolArray symbol_array)
{
  LNK_RelocList *reloc_list_arr = push_array_no_zero(arena, LNK_RelocList, sect_count);
  for (U64 sect_idx = 0; sect_idx < sect_count; ++sect_idx) {
    COFF_SectionHeader *coff_header     = &coff_sect_arr[sect_idx];
    COFF_RelocInfo      coff_reloc_info = coff_reloc_info_from_section_header(coff_data, coff_header);
    COFF_Reloc         *coff_reloc_v    = (COFF_Reloc *)(coff_data.str + coff_reloc_info.array_off);
    LNK_Chunk          *sect_chunk      = chunk_ptr_arr[sect_idx];
    reloc_list_arr[sect_idx] = lnk_reloc_list_from_coff_reloc_array(arena, machine, sect_chunk, symbol_array, coff_reloc_v, coff_reloc_info.count);
  }
  return reloc_list_arr;
}

internal LNK_DirectiveInfo
lnk_init_directives(Arena *arena, String8 obj_path, U64 chunk_count, String8 *sect_name_arr, LNK_Chunk *chunk_arr)
{
  LNK_DirectiveInfo directive_info = {0};
  for (U64 chunk_idx = 0; chunk_idx < chunk_count; chunk_idx += 1) {
    String8    sect_name  = sect_name_arr[chunk_idx];
    LNK_Chunk *sect_chunk = &chunk_arr[chunk_idx];
    Assert(sect_chunk->type == LNK_Chunk_Leaf);

    if (!str8_match(sect_name, str8_lit(".drectve"), 0)) {
      continue;
    }
    if (sect_chunk->u.leaf.size < 3) {
      lnk_error(LNK_Warning_IllData, "%S: can't parse %S", obj_path, sect_name);
      continue;
    }
    if (~sect_chunk->flags & COFF_SectionFlag_LNK_INFO) {
      lnk_error(LNK_Warning_IllData, "%S: %S missing COFF_SectionFlag_LNK_INFO.", obj_path, sect_name);
    }

    // TODO: warn if section has relocations

    lnk_parse_directives(arena, &directive_info, sect_chunk->u.leaf, obj_path);
    int bad_vs = 0; (void)bad_vs;
  }
  return directive_info;
}

internal COFF_FeatFlags
lnk_obj_get_features(LNK_Obj *obj)
{
  COFF_FeatFlags result = 0;
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

