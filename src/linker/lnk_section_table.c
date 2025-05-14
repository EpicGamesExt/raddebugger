// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_SectionNode *
lnk_section_list_remove(LNK_SectionList *list, String8 name)
{
  LNK_SectionNode *section = lnk_section_list_search_node(list, name);
  
  if (list->count > 0) {
    if (list->first == section) {
      list->first = list->first->next;
      list->count -= 1;
      
      if (list->last == section) {
        list->last = NULL;
      }
    } else {
      for (LNK_SectionNode *curr = list->first, *prev = NULL; curr != NULL; prev = curr, curr = curr->next) {
        if (curr == section) {
          prev->next = curr->next;
          list->count -= 1;
          
          if (list->last == curr) {
            list->last = prev;
          }
          
          break;
        }
      }
    }
  }
  return section;
}

internal LNK_SectionNode *
lnk_section_list_search_node(LNK_SectionList *list, String8 name)
{
  LNK_SectionNode *node;
  for (node = list->first; node != 0; node = node->next) {
    if (str8_match(node->data.name, name, 0)) {
      break;
    }
  }
  return node;
}

internal LNK_Section *
lnk_section_list_search(LNK_SectionList *list, String8 name)
{
  LNK_SectionNode *node = lnk_section_list_search_node(list, name);
  return node != NULL ? &node->data : NULL;
}

internal LNK_SectionArray
lnk_section_array_from_list(Arena *arena, LNK_SectionList list)
{
  LNK_SectionArray result;
  result.count = 0;
  result.v = push_array_no_zero(arena, LNK_Section, list.count);
  for (LNK_SectionNode *node = list.first; node != 0; node = node->next) {
    result.v[result.count] = node->data;
    result.count += 1;
  }
  return result;
}

internal LNK_SectionPtrArray
lnk_section_ptr_array_from_list(Arena *arena, LNK_SectionList list)
{
  LNK_SectionPtrArray result;
  result.count = 0;
  result.v = push_array_no_zero(arena, LNK_Section *, list.count);
  for (LNK_SectionNode *node = list.first; node != 0; node = node->next) {
    result.v[result.count] = &node->data;
    result.count += 1;
  }
  return result;
}

internal String8
lnk_make_section_sort_index(Arena *arena, String8 name, COFF_SectionFlags flags, U64 section_index)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  // pack sections with run-time data closer
  String8List sort_index_list = {0};
  if (flags & COFF_SectionFlag_MemDiscardable) {
    str8_list_pushf(scratch.arena, &sort_index_list, "b");
  } else {
    str8_list_pushf(scratch.arena, &sort_index_list, "a");
  }
  
  if (str8_match_lit(".null", name, 0)) {
    // null section always first
    str8_list_pushf(scratch.arena, &sort_index_list, "a");
  } else if (str8_match_lit(".rsrc", name, 0)) {
    // section with resource data must be last because during runtime windows might append pages
    str8_list_pushf(scratch.arena, &sort_index_list, "c");
  } else {
    str8_list_pushf(scratch.arena, &sort_index_list, "b");
  }
  
  // sort sections based on the contents
  if (flags & COFF_SectionFlag_CntCode) {
    str8_list_pushf(scratch.arena, &sort_index_list, "a");
    if (str8_match_lit(".text", name, 0)) {
      str8_list_pushf(scratch.arena, &sort_index_list, "a");
    } else {
      str8_list_pushf(scratch.arena, &sort_index_list, "b");
    }
  } else if (flags & COFF_SectionFlag_CntInitializedData) {
    str8_list_pushf(scratch.arena, &sort_index_list, "b");
    if (str8_match_lit(".data", name, 0)) {
      str8_list_pushf(scratch.arena, &sort_index_list, "a");
    } else if (str8_match_lit(".rdata", name, 0)) {
      str8_list_pushf(scratch.arena, &sort_index_list, "b");
    } else if (str8_match_lit(".tls", name, 0)) {
      str8_list_pushf(scratch.arena, &sort_index_list, "c");
    } else {
      str8_list_pushf(scratch.arena, &sort_index_list, "d");
    }
  } else if (flags & COFF_SectionFlag_CntUninitializedData) {
    str8_list_pushf(scratch.arena, &sort_index_list, "c");
  } else {
    str8_list_pushf(scratch.arena, &sort_index_list, "d");
  }
  
  // sort sections based on read/write access so final section layout looks cleaner
  if (flags & COFF_SectionFlag_MemRead && ~flags & COFF_SectionFlag_MemWrite) {
    str8_list_pushf(scratch.arena, &sort_index_list, "a");
  } else {
    str8_list_pushf(scratch.arena, &sort_index_list, "b");
  }
  
  String8 order_index = str8_from_bits_u32(scratch.arena, safe_cast_u32(section_index));
  str8_list_push(scratch.arena, &sort_index_list, order_index);
  
  String8 result = str8_list_join(arena, &sort_index_list, 0);
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal void
lnk_section_associate_chunks(LNK_Section *sect, LNK_Chunk *head, LNK_Chunk *associate)
{
  lnk_chunk_associate(head, associate);
}

internal LNK_Chunk *
lnk_section_push_chunk_raw(LNK_Section *sect, LNK_Chunk *parent, void *raw_ptr, U64 raw_size, String8 sort_index)
{
  return lnk_chunk_push_leaf(sect->arena, sect->cman, parent, sort_index, raw_ptr, raw_size);
}

internal LNK_Chunk *
lnk_section_push_chunk_data(LNK_Section *sect, LNK_Chunk *parent, String8 data, String8 sort_index)
{
  return lnk_section_push_chunk_raw(sect, parent, data.str, data.size, sort_index);
}

internal LNK_Chunk *
lnk_section_push_chunk_u32(LNK_Section *sect, LNK_Chunk *parent, U32 value, String8 sort_index)
{
  U32 *ptr = push_array_no_zero(sect->arena, U32, 1);
  *ptr = value;
  return lnk_section_push_chunk_raw(sect, parent, ptr, sizeof(*ptr), sort_index);
}

internal LNK_Chunk *
lnk_section_push_chunk_u64(LNK_Section *sect, LNK_Chunk *parent, U32 value, String8 sort_index)
{
  U64 *ptr = push_array_no_zero(sect->arena, U64, 1);
  *ptr = value;
  return lnk_section_push_chunk_raw(sect, parent, ptr, sizeof(*ptr), sort_index);
}

internal LNK_Chunk *
lnk_section_push_chunk_bss(LNK_Section *sect, LNK_Chunk *parent, U64 size, String8 sort_index)
{
  return lnk_section_push_chunk_raw(sect, parent, 0, size, sort_index);
}

internal LNK_Chunk *
lnk_section_push_chunk_list(LNK_Section *sect, LNK_Chunk *parent, String8 sort_index)
{
  return lnk_chunk_push_list(sect->arena, sect->cman, parent, sort_index);
}

internal LNK_Reloc *
lnk_section_push_reloc(LNK_Section *sect, LNK_Chunk *chunk, LNK_RelocType type, U64 apply_off, LNK_Symbol *symbol)
{
  Assert(symbol);
  LNK_Reloc *reloc = lnk_reloc_list_push(sect->arena, &sect->reloc_list);
  reloc->chunk     = chunk;
  reloc->type      = type;
  reloc->apply_off = apply_off;
  reloc->symbol    = symbol;
  return reloc;
}

internal LNK_Reloc *
lnk_section_push_reloc_undefined(LNK_Section *sect, LNK_Chunk *chunk, LNK_RelocType type, U64 apply_off, String8 undefined_symbol_name, LNK_SymbolScopeFlags scope_flags)
{
  LNK_Symbol *symbol = lnk_make_undefined_symbol(sect->arena, undefined_symbol_name, scope_flags);
  LNK_Reloc *reloc = lnk_section_push_reloc(sect, chunk, type, apply_off, symbol);
  return reloc;
}

internal void
lnk_section_merge(LNK_Section *dst, LNK_Section *src)
{
  ProfBeginFunction();

  // set merge info
  src->is_merged     = 1;
  src->merge_sect_id = dst->id;
  src->id_map        = push_array_no_zero(src->arena, U64, src->cman->total_chunk_count);
  
  // put source root in a wrapper list so it has unique sort index otherwise
  // after we merge sections sort indices might conflict
  LNK_Chunk *src_root_wrapper = lnk_section_push_chunk_list(dst, dst->cman->root, str8(0,0));

  // merge roots
  lnk_merge_chunks(dst->arena, dst->cman, src_root_wrapper, src->cman->root, src->id_map, src->cman->total_chunk_count);
  
  // copy relocations
  lnk_reloc_list_concat_in_place(&dst->reloc_list, &src->reloc_list);

  ProfEnd();
}

internal U8
lnk_code_align_byte_from_machine(COFF_MachineType machine)
{
  U8 align_byte = 0;
  switch (machine) {
  case COFF_MachineType_X64:
  case COFF_MachineType_X86: {
    align_byte = 0xCC;
  } break;
  default: {
    lnk_not_implemented("TODO: set align value for machine %S", coff_string_from_machine_type(machine));
  } break;
  }
  return align_byte;
}

internal void
lnk_section_build_data(LNK_Section *sect, COFF_MachineType machine)
{
  if (sect->is_loose) {
    if (sect->has_layout) {
      sect->layout = lnk_build_chunk_layout(sect->arena, sect->cman);
    } else {
      sect->layout.total_count           = sect->cman->total_chunk_count;
      sect->layout.chunk_ptr_array       = lnk_make_chunk_id_map(sect->arena, sect->cman);
      sect->layout.chunk_off_array       = 0;
      sect->layout.chunk_file_size_array = 0;
      sect->layout.chunk_virt_size_array = 0;
      sect->layout.pad_array_count     = 0;
      sect->layout.pad_array           = 0;
    }
    sect->is_loose = 0;
  }
}

internal LNK_SectionTable *
lnk_section_table_alloc(U64 section_virt_off, U64 sect_align, U64 file_align)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  LNK_SectionTable *sectab = push_array(arena, LNK_SectionTable, 1);
  sectab->arena            = arena;
  sectab->section_virt_off = section_virt_off;
  sectab->sect_align       = sect_align;
  sectab->file_align       = file_align;
  ProfEnd();
  return sectab;
}

internal void
lnk_section_table_release(LNK_SectionTable **st_ptr)
{
  ProfBeginFunction();
  LNK_SectionTable *sectab = *st_ptr;
  arena_release(sectab->arena);
  *st_ptr = NULL;
  ProfEnd();
}

internal LNK_Section *
lnk_section_table_push(LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags)
{
  ProfBeginFunction();
  LNK_SectionList *sect_list = &sectab->list;
  
  LNK_SectionNode *sect_node  = push_array(sectab->arena, LNK_SectionNode, 1);
  String8          sort_index = lnk_make_section_sort_index(sectab->arena, name, flags, sectab->id_max);
  
  B32 found = 0;
  for (LNK_SectionNode *curr = sect_list->first, *prev = NULL; curr != NULL; prev = curr, curr = curr->next) {
    LNK_Section *sect = &curr->data;
    int cmp = str8_compar_case_sensitive(&sort_index, &sect->sort_index);
    if (cmp < 0) {
      if (prev == NULL) {
        SLLQueuePushFront(sect_list->first, sect_list->last, sect_node);
      } else {
        prev->next = sect_node;
        sect_node->next = curr;
      }
      found = 1;
      break;
    }
  }
  
  if (!found) {
    SLLQueuePush(sect_list->first, sect_list->last, sect_node);
  }
  sect_list->count += 1;

  U64 sect_id = sectab->id_max;
  sectab->id_max += 1;
  
  LNK_Section *sect  = &sect_node->data;
  sect->arena        = arena_alloc();
  sect->id           = sect_id;
  sect->name         = push_str8_copy(sect->arena, name);
  sect->sort_index   = sort_index;
  sect->flags        = flags;
  sect->cman         = lnk_chunk_manager_alloc(sect->arena, sect_id, sectab->file_align);
  sect->root         = sect->cman->root;
  sect->nosort_chunk = lnk_chunk_push_list(sect->arena, sect->cman, sect->root, str8(0,0));
  sect->nosort_chunk->sort_chunk = 0;
  sect->emit_header  = 1;
  sect->has_layout   = 1;
  sect->is_loose     = 1;

  lnk_chunk_set_debugf(sect->arena, sect->root, "root chunk for %S", name);
  lnk_chunk_set_debugf(sect->arena, sect->nosort_chunk, "nosort chunk for %S", name);
  
  ProfEnd();
  return sect;
}

internal LNK_Section *
lnk_section_table_push_null(LNK_SectionTable *sectab)
{
  LNK_SectionList *list = &sectab->list;
  SLLQueuePushFront(list->first, list->last, sectab->null_sect);
  list->count += 1;
  return &sectab->null_sect->data;
}

LNK_CHUNK_VISITOR_SIG(lnk_chunk_has_leaf)
{
  B32 stop = 0;
  if (chunk->type == LNK_Chunk_Leaf) {
    B32 has_data = !lnk_chunk_is_discarded(chunk) && chunk->u.leaf.size > 0;
    if (has_data) {
      B32 *no_data = (B32*)ud;
      *no_data = 0;
      stop = 1;
    }
  }
  return stop;
}

LNK_CHUNK_VISITOR_SIG(lnk_chunk_mark_discarded)
{
  chunk->is_discarded = 1;
  B32 stop = 0;
  return stop;
}

internal void
lnk_section_table_remove(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, String8 name)
{
  ProfBeginFunction();
  
  // remove node from list
  LNK_SectionNode *sect_node = lnk_section_list_remove(&sectab->list, name);
  LNK_Section *sect = &sect_node->data;
  
  // remove symbol for section root chunk
  lnk_symbol_table_remove(symtab, LNK_SymbolScopeIndex_Internal, sect->symbol_name);
  
  // mark chunks as discarded
  lnk_visit_chunks(sect->id, sect->root, lnk_chunk_mark_discarded, NULL);
  
  // push to empties
  SLLQueuePush(sectab->empties_list.first, sectab->empties_list.last, sect_node);
  sectab->empties_list.count += 1;

  ProfEnd();
}

internal LNK_Section *
lnk_section_table_search(LNK_SectionTable *sectab, String8 name)
{
  return lnk_section_list_search(&sectab->list, name);
}

internal LNK_Section *
lnk_section_table_search_id(LNK_SectionTable *sectab, U64 id)
{
  for (LNK_SectionNode *node = sectab->list.first; node != NULL; node = node->next) {
    if (node->data.id == id) {
      return &node->data;
    }
  }
  return NULL;
}

internal void
lnk_section_table_merge(LNK_SectionTable *sectab, LNK_MergeDirectiveList merge_list)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  LNK_Section **src_dst = push_array(scratch.arena, LNK_Section *, sectab->id_max);
  for (LNK_MergeDirectiveNode *merge_node = merge_list.first; merge_node != NULL; merge_node = merge_node->next) {
    LNK_MergeDirective *merge = &merge_node->data;
    
    // are we trying to merge section that was already merged?
    LNK_Section *merge_sect = lnk_section_list_search(&sectab->merge_list, merge->src);
    if (merge_sect) {
      LNK_Section *dst = src_dst[merge_sect->id];
      B32 is_ambiguous_merge = !str8_match(dst->name, merge->dst, 0);
      if (is_ambiguous_merge) {
        lnk_error(LNK_Warning_AmbiguousMerge, "Detected ambiguous section merge:");
        lnk_supplement_error("%S => %S (Merged)", merge_sect->name, dst->name);
        lnk_supplement_error("%S => %S", merge_sect->name, merge->dst);
      }
      continue;
    }
    
    // find source seciton
    LNK_Section *src = lnk_section_table_search(sectab, merge->src);
    if (src == NULL) {
      lnk_error(LNK_Warning_IllData, "Can't find section \"%S\" to merge with \"%S\"", merge->src, merge->dst);
      // TODO: supplement obj path if applicable
      continue;
    }
    
    // handle case where destination section doesn't exist
    LNK_Section *dst = lnk_section_table_search(sectab, merge->dst);
    if (dst == NULL) {
      src->name = push_str8_copy(src->arena, merge->dst);
      src_dst[src->id] = src;
      continue;
    }
    
    // update map
    src_dst[src->id] = dst;
    
    // merge section with destination
    lnk_section_merge(dst, src);
    
    // remove from output section list
    LNK_SectionNode *src_node = lnk_section_list_remove(&sectab->list, src->name);
    
    // push section to merged list
    SLLQueuePush(sectab->merge_list.first, sectab->merge_list.last, src_node);
    sectab->merge_list.count += 1;
  }
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_section_table_remove_empties(LNK_SectionTable *sectab, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  String8List name_list = {0};
  for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != NULL; sect_node = sect_node->next) {
    LNK_Section *sect = &sect_node->data;
    
    B32 no_data = 1;
    lnk_visit_chunks(sect->id, sect->root, lnk_chunk_has_leaf, (void*)&no_data);
    
    if (no_data) {
      String8 name = push_str8_copy(scratch.arena, sect->name);
      str8_list_push(scratch.arena, &name_list, name);
    }
  }
  
  for (String8Node *name = name_list.first; name != NULL; name = name->next) {
    lnk_section_table_remove(sectab, symtab, name->string);
  }
  scratch_end(scratch);
  ProfEnd();
}

internal LNK_SectionArray
lnk_section_table_get_output_sections(Arena *arena, LNK_SectionTable *sectab)
{
  LNK_SectionArray result = {0};
  result.count            = 0;
  result.v                = push_array(arena, LNK_Section, sectab->list.count);

  for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != 0; sect_node = sect_node->next) {
    if (sect_node->data.emit_header && sect_node->data.has_layout) {
      Assert(result.count < sectab->list.count);
      result.v[result.count] = sect_node->data;
      result.count += 1;
    }
  }

  U64 unused_entry_count = sectab->list.count - result.count;
  arena_pop(arena, unused_entry_count * sizeof(result.v[0]));

  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_section_data_builder)
{
  LNK_SectionDataBuilder *task  = raw_task;
  Rng1U64                 range = task->range_arr[task_id];
  for (U64 sect_idx = range.min; sect_idx < range.max; ++sect_idx) {
    lnk_section_build_data(task->sect_arr[sect_idx], task->machine);
  }
}

internal void
lnk_section_table_build_data(TP_Context *tp, LNK_SectionTable *sectab, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  LNK_SectionPtrArray sect_arr = lnk_section_ptr_array_from_list(scratch.arena, sectab->list);

  LNK_SectionDataBuilder task = {0};
  task.machine                = machine;
  task.range_arr              = tp_divide_work(scratch.arena, sect_arr.count, tp->worker_count);
  task.sect_arr               = sect_arr.v;
  tp_for_parallel(tp, 0, tp->worker_count, lnk_section_data_builder, &task);

  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_section_table_assign_virtual_offsets(LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  LNK_Section **sect_id_map = lnk_sect_id_map_from_section_table(scratch.arena, sectab);
  U64           cursor      = sectab->section_virt_off;
  Assert(cursor >= 0x1000);
  for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != NULL; sect_node = sect_node->next) {
    if (sect_node == sectab->null_sect) continue;
    LNK_Section *sect = &sect_node->data;
    if (!sect->has_layout) continue;
    sect->virt_off = cursor;
    U64 sect_size = lnk_virt_size_from_chunk_ref(sect_id_map, sect->root->ref);
    cursor += sect_size;
    cursor = AlignPow2(cursor, sectab->sect_align);
  }
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_section_table_assign_file_offsets(LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  U64 cursor = 0;
  for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != NULL; sect_node = sect_node->next) {
    LNK_Section *sect = &sect_node->data;
    if (sect->flags & COFF_SectionFlag_CntUninitializedData) {
      continue;
    }
    if (!sect->has_layout) continue;
    sect->file_off = cursor;
    U64 root_size = sect->layout.chunk_file_size_array[sect->root->ref.chunk_id];
    cursor += root_size;
  }
  ProfEnd();
}

internal void
lnk_section_table_assign_indices(LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  U64 isect = 0;
  for (LNK_SectionNode *sect_node = sectab->list.first; sect_node != NULL; sect_node = sect_node->next) {
    LNK_Section *sect = &sect_node->data;
    if (sect->emit_header) {
      sect->isect = isect++;
    }
  }
  ProfEnd();
}

internal String8
lnk_section_table_serialize(TP_Context *tp, Arena *arena, LNK_SectionTable *sectab, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  U64 image_size = 0;
  for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
    LNK_Section *sect = &sect_n->data;
    if (sect->has_layout) {
      U64 root_size = sect->layout.chunk_file_size_array[sect->root->ref.chunk_id];
      image_size += root_size;
    }
  }

  U8      *image_buffer = push_array_no_zero(arena, U8, image_size);
  String8  image        = str8(image_buffer, image_size);
  U64      image_cursor = 0;

  for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
    LNK_Section *sect = &sect_n->data;
    if (sect->has_layout) {
      if (sect->flags & COFF_SectionFlag_CntUninitializedData) {
        continue;
      }

      U64     sect_size = sect->layout.chunk_file_size_array[sect->root->ref.chunk_id];
      String8 sect_data = str8_substr(image, rng_1u64(image_cursor, image_cursor + sect_size));

      U8 fill_byte = 0;
      if (sect->flags & COFF_SectionFlag_CntCode) {
        fill_byte = lnk_code_align_byte_from_machine(machine);
      }

      lnk_serialize_chunk_layout(tp, sect->layout, sect_data, fill_byte);

      image_cursor += sect_size;
    }
  }

  scratch_end(scratch);
  ProfEnd();
  return image;
}

internal LNK_ChunkPtr **
lnk_chunk_id_map_from_section_table(Arena *arena, LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  LNK_ChunkPtr **chunk_id_map = push_array(arena, LNK_ChunkPtr *, sectab->id_max);
  for (LNK_SectionNode *node = sectab->list.first; node != 0; node = node->next) {
    LNK_Section *sect = &node->data;
    chunk_id_map[sect->id] = lnk_make_chunk_id_map(arena, sect->cman);
  }
  if (sectab->list.first->data.id != 0) {
    chunk_id_map[0] = push_array(arena, LNK_ChunkPtr, 1);
    chunk_id_map[0][0] = g_null_chunk_ptr;
  }
  ProfEnd();
  return chunk_id_map;
}

internal LNK_Section **
lnk_sect_id_map_from_section_table(Arena *arena, LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  LNK_Section **map = push_array(arena, LNK_Section *, sectab->id_max);
  LNK_SectionList *list_arr[] = { &sectab->list, &sectab->merge_list, &sectab->empties_list };
  for (U64 list_idx = 0; list_idx < ArrayCount(list_arr); ++list_idx) {
    for (LNK_SectionNode *sect_node = list_arr[list_idx]->first; sect_node != NULL; sect_node = sect_node->next) {
      LNK_Section *sect = &sect_node->data;
      Assert(sect->id < sectab->id_max);
      Assert(map[sect->id] == NULL);
      map[sect->id] = sect;
    }
  }
  if (map[0] == NULL) {
    LNK_Section *sect = push_array(arena, LNK_Section, 1);
    sect->layout.chunk_off_array = push_array(arena, U64, 1);
    sect->layout.chunk_file_size_array = push_array(arena, U64, 1);
    sect->layout.chunk_virt_size_array = push_array(arena, U64, 1);
    map[0] = sect;
  }
  ProfEnd();
  return map;
}

internal LNK_ChunkRef
lnk_get_final_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = chunk_ref;
  if (sect_id_map[chunk_ref.sect_id]->is_merged) {
    final_chunk_ref.sect_id = sect_id_map[chunk_ref.sect_id]->merge_sect_id;
    final_chunk_ref.chunk_id = sect_id_map[chunk_ref.sect_id]->id_map[chunk_ref.chunk_id];
    // we don't support sections that were merged more than once.
    Assert(!sect_id_map[final_chunk_ref.sect_id]->is_merged);
  }
  return final_chunk_ref;
}

internal LNK_Section *
lnk_sect_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef input_chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, input_chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  return sect;
}

internal LNK_Chunk *
lnk_chunk_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkPtr **chunk_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Chunk *chunk = chunk_id_map[final_chunk_ref.sect_id][final_chunk_ref.chunk_id];
  return chunk;
}

internal U64
lnk_isect_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_Section *sect = lnk_sect_from_chunk_ref(sect_id_map, chunk_ref);
  U64 isect = sect->isect;
  return isect;
}

internal U64
lnk_off_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  U64 off = sect->layout.chunk_off_array[final_chunk_ref.chunk_id];
  return off;
}

internal U64
lnk_virt_off_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  U64 off = sect->layout.chunk_off_array[final_chunk_ref.chunk_id];
  U64 virt_off = off + sect->virt_off;
  return virt_off;
}

internal U64
lnk_file_off_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  U64 off = sect->layout.chunk_off_array[final_chunk_ref.chunk_id];
  U64 file_off = off + sect->file_off;
  return file_off;
}

internal U64
lnk_virt_size_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  U64 virt_size = sect->layout.chunk_virt_size_array[final_chunk_ref.chunk_id];
  return virt_size;
}

internal U64
lnk_file_size_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];
  U64 file_size = sect->layout.chunk_file_size_array[final_chunk_ref.chunk_id];
  return file_size;
}

internal String8
lnk_data_from_chunk_ref(LNK_Section **sect_id_map, String8 image_data, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef  final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section  *sect            = sect_id_map[final_chunk_ref.sect_id];
  String8 chunk_data;
  if (sect->has_layout) {
    U64 chunk_size = lnk_file_size_from_chunk_ref(sect_id_map, chunk_ref);
    U64 chunk_foff = lnk_file_off_from_chunk_ref(sect_id_map, chunk_ref);
    chunk_data = str8_substr(image_data, r1u64(chunk_foff, chunk_foff + chunk_size));
  } else {
    LNK_Chunk *chunk = sect->layout.chunk_ptr_array[final_chunk_ref.chunk_id];
    Assert(chunk->type == LNK_Chunk_Leaf);
    chunk_data = chunk->u.leaf;
  }

  return chunk_data;
}

internal String8
lnk_data_from_chunk_ref_no_pad(LNK_Section **sect_id_map, String8 image_data, LNK_ChunkRef chunk_ref)
{
  LNK_ChunkRef final_chunk_ref = lnk_get_final_chunk_ref(sect_id_map, chunk_ref);
  LNK_Section *sect = sect_id_map[final_chunk_ref.sect_id];

  String8 chunk_data;
  if (sect->has_layout) {
    U64 chunk_size = lnk_virt_size_from_chunk_ref(sect_id_map, chunk_ref);
    U64 chunk_foff = lnk_file_off_from_chunk_ref(sect_id_map, chunk_ref);
    chunk_data = str8_substr(image_data, r1u64(chunk_foff, chunk_foff + chunk_size));
  } else {
    LNK_Chunk *chunk = sect->layout.chunk_ptr_array[final_chunk_ref.chunk_id];
    Assert(chunk->type == LNK_Chunk_Leaf);
    chunk_data = chunk->u.leaf;
  }

  return chunk_data;
}

internal ISectOff
lnk_sc_from_chunk_ref(LNK_Section **sect_id_map, LNK_ChunkRef chunk_ref)
{
  ISectOff sc = {0};
  sc.isect = lnk_isect_from_chunk_ref(sect_id_map, chunk_ref);
  sc.off = lnk_off_from_chunk_ref(sect_id_map, chunk_ref);
  return sc;
}

internal U64
lnk_virt_off_from_reloc(LNK_Section **sect_id_map, LNK_Reloc *reloc)
{
  U64 virt_off = lnk_virt_off_from_chunk_ref(sect_id_map, reloc->chunk->ref);
  virt_off += reloc->apply_off;
  return virt_off;
}

internal U64
lnk_isect_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  LNK_ChunkRef symbol_chunk_ref = symbol->u.defined.u.chunk->ref;
  U64 symbol_isect = lnk_isect_from_chunk_ref(sect_id_map, symbol_chunk_ref);
  return symbol_isect;
}

internal U64
lnk_sect_off_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  LNK_ChunkRef symbol_chunk_ref = symbol->u.defined.u.chunk->ref;
  U64 chunk_off = lnk_off_from_chunk_ref(sect_id_map, symbol_chunk_ref);
  U64 symbol_off = chunk_off + symbol->u.defined.u.chunk_offset;
  return symbol_off;
}

internal U64
lnk_virt_off_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  LNK_ChunkRef symbol_chunk_ref = symbol->u.defined.u.chunk->ref;
  U64 chunk_voff = lnk_virt_off_from_chunk_ref(sect_id_map, symbol_chunk_ref);
  U64 symbol_voff = chunk_voff + symbol->u.defined.u.chunk_offset;
  return symbol_voff;
}

internal U64
lnk_file_off_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  LNK_ChunkRef symbol_chunk_ref = symbol->u.defined.u.chunk->ref;
  U64 chunk_foff = lnk_file_off_from_chunk_ref(sect_id_map, symbol_chunk_ref);
  U64 symbol_foff = chunk_foff + symbol->u.defined.u.chunk_offset;
  return symbol_foff;
}

internal U64
lnk_virt_size_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  U64 symbol_chunk_virt_size = lnk_virt_size_from_chunk_ref(sect_id_map, symbol->u.defined.u.chunk->ref);
  return symbol_chunk_virt_size;
}

internal U64
lnk_file_size_from_symbol(LNK_Section **sect_id_map, LNK_Symbol *symbol)
{
  Assert(LNK_Symbol_IsDefined(symbol->type));
  U64 symbol_chunk_file_size = lnk_file_size_from_chunk_ref(sect_id_map, symbol->u.defined.u.chunk->ref);
  return symbol_chunk_file_size;
}

#if LNK_DEBUG_CHUNKS
internal void
lnk_dump_chunks(LNK_SectionTable *sectab)
{
  Temp scratch = scratch_begin(0, 0);
  LNK_ChunkPtr **chunk_id_map = lnk_chunk_id_map_from_section_table(scratch.arena, sectab);
  LNK_Section **sect_id_map = lnk_sect_id_map_from_section_table(scratch.arena, sectab); 
  for (U64 sect_id = 0; sect_id < sectab->id_max; ++sect_id) {
    LNK_Section *sect = sect_id_map[sect_id];
    if (!sect) continue;
    if (sect->is_merged) continue;
    if (str8_match_lit(".text", sect->name, 0)) {
      for (U64 chunk_id = 0; chunk_id < sect->cman->total_chunk_count; ++chunk_id) {
        LNK_ChunkRef chunk_ref = { sect_id, chunk_id };
        LNK_Chunk *chunk = lnk_chunk_from_chunk_ref(sect_id_map, chunk_id_map, chunk_ref);
        U64 chunk_foff = sect->file_off + sect->layout.chunk_off_array[chunk_id];
        printf("%llu {%04llX,%04llX} 0x%08llX %.*s\n", chunk_foff, sect_id, chunk_id, chunk_foff, str8_varg(chunk->debug));
      }
    }
  }
  scratch_end(scratch);
}
#endif

