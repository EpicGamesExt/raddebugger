// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_BaseRelocPageArray
lnk_base_reloc_page_array_from_list(Arena* arena, LNK_BaseRelocPageList list)
{
  LNK_BaseRelocPageArray result = {0};
  result.count                  = 0;
  result.v                      = push_array_no_zero(arena, LNK_BaseRelocPage, list.count);
  for (LNK_BaseRelocPageNode* n = list.first; n != 0; n = n->next) {
    result.v[result.count++] = n->v;
  }
  Assert(result.count == list.count);
  return result;
}

internal void
lnk_emit_base_reloc_info(Arena                 *arena,
                         LNK_Section          **sect_id_map,
                         U64                    page_size,
                         HashTable             *page_ht,
                         LNK_BaseRelocPageList *page_list,
                         LNK_Reloc             *reloc)
{
  B32 is_addr = (reloc->type == LNK_Reloc_ADDR_64 || reloc->type == LNK_Reloc_ADDR_32);
  if (is_addr) {
    U64 reloc_voff = lnk_virt_off_from_reloc(sect_id_map, reloc);
    U64 page_voff  = AlignDownPow2(reloc_voff, page_size);

    LNK_BaseRelocPageNode *page;
    {
      String8 raw_page;
      B32 is_page_present = hash_table_search_u64(page_ht, page_voff, &raw_page);
      if (is_page_present) {
        page = *(LNK_BaseRelocPageNode **) raw_page.str;
      } else {
        // fill out page
        page = push_array(arena, LNK_BaseRelocPageNode, 1);
        page->v.voff = page_voff;

        // push page
        SLLQueuePush(page_list->first, page_list->last, page);
        page_list->count += 1;

        // register page voff
        hash_table_push_u64(arena, page_ht, page_voff, str8_struct(&page));
      }
    }

    u64_list_push(arena, &page->v.entries, reloc_voff);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_emit_base_relocs_from_reloc_array_task)
{
  LNK_BaseRelocTask     *task      = raw_task;
  Rng1U64                range     = task->range_arr[task_id];
  LNK_BaseRelocPageList *page_list = &task->list_arr[task_id];
  HashTable             *page_ht   = task->page_ht_arr[task_id];

  for (U64 reloc_idx = range.min; reloc_idx < range.max; reloc_idx += 1) {
    LNK_Reloc *reloc = task->reloc_arr[reloc_idx];
    lnk_emit_base_reloc_info(arena, task->sect_id_map, task->page_size, page_ht, page_list, reloc);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_emit_base_relocs_from_objs_task)
{
  LNK_ObjBaseRelocTask  *task      = raw_task;
  LNK_Obj               *obj       = task->obj_arr[task_id];
  LNK_BaseRelocPageList *page_list = &task->list_arr[worker_id];
  HashTable             *page_ht   = task->page_ht_arr[worker_id];

  for (U64 sect_idx = 0; sect_idx < obj->sect_count; sect_idx += 1) {
    B32 is_live = !lnk_chunk_is_discarded(&obj->chunk_arr[sect_idx]);
    if (is_live) {
      LNK_RelocList reloc_list = obj->sect_reloc_list_arr[sect_idx];
      for (LNK_Reloc *reloc = reloc_list.first; reloc != 0; reloc = reloc->next) {
        lnk_emit_base_reloc_info(arena,
                                 task->sect_id_map,
                                 task->page_size,
                                 page_ht,
                                 page_list,
                                 reloc);
      }
    }
  }
}

int
lnk_base_reloc_page_compar(void *raw_a, void *raw_b)
{
  LNK_BaseRelocPage* a = raw_a;
  LNK_BaseRelocPage* b = raw_b;
  int is_before = a->voff < b->voff;
  return is_before;
}

internal void
lnk_base_reloc_page_array_sort(LNK_BaseRelocPageArray arr)
{
  ProfBeginFunction();
  radsort(arr.v, arr.count, lnk_base_reloc_page_compar);
  ProfEnd();
}

internal void
lnk_build_base_relocs(TP_Context       *tp,
                      LNK_SectionTable *st,
                      LNK_SymbolTable  *symtab,
                      COFF_MachineType  machine,
                      U64               page_size,
                      LNK_ObjList       obj_list)
{
  ProfBeginFunction();

  TP_Arena *arena = g_file_arena;
  TP_Temp   temp  = tp_temp_begin(arena);

  lnk_section_table_build_data(st, machine);
  lnk_section_table_assign_virtual_offsets(st);

  LNK_Section **sect_id_map = lnk_sect_id_map_from_section_table(arena->v[0], st);

  LNK_BaseRelocPageList  *page_list_arr = push_array(arena->v[0], LNK_BaseRelocPageList, tp->worker_count);
  HashTable             **page_ht_arr   = push_array_no_zero(arena->v[0], HashTable *, tp->worker_count);
  for (U64 i = 0; i < tp->worker_count; ++i) {
    page_ht_arr[i] = hash_table_init(arena->v[0], 1024);
  }

  // emit pages from relocs defined in section table
  ProfBegin("Emit Relocs From Section Table");
  for (LNK_SectionNode *sect_node = st->list.first; sect_node != 0; sect_node = sect_node->next) {
    LNK_BaseRelocTask task = {0};
    task.page_size         = page_size;
    task.sect_id_map       = sect_id_map;
    task.list_arr          = page_list_arr;
    task.page_ht_arr       = page_ht_arr;
    task.reloc_arr         = lnk_reloc_array_from_list(arena->v[0], sect_node->data.reloc_list);
    task.range_arr         = tp_divide_work(arena->v[0], sect_node->data.reloc_list.count, tp->worker_count);
    tp_for_parallel(tp, arena, tp->worker_count, lnk_emit_base_relocs_from_reloc_array_task, &task);
  }
  ProfEnd();

  // emit pages from relocs defined in objs
  ProfBegin("Emit Relocs From Objs");
  {
    LNK_ObjBaseRelocTask task = {0};
    task.page_size            = page_size;
    task.sect_id_map          = sect_id_map;
    task.page_ht_arr          = page_ht_arr;
    task.list_arr             = page_list_arr;
    task.obj_arr              = lnk_obj_arr_from_list(arena->v[0], obj_list);
    tp_for_parallel(tp, arena, obj_list.count, lnk_emit_base_relocs_from_objs_task, &task);
  }
  ProfEnd();

  // merge page lists

  ProfBegin("Merge Worker Page Lists");

  HashTable             *main_ht        = page_ht_arr[0];
  LNK_BaseRelocPageList *main_page_list = &page_list_arr[0];

  for (U64 list_idx = 1; list_idx < tp->worker_count; ++list_idx) {
    LNK_BaseRelocPageList src = page_list_arr[list_idx];

    for (LNK_BaseRelocPageNode *src_page = src.first, *src_next; src_page != 0; src_page = src_next) {
      src_next = src_page->next;

      String8 raw_page;
      B32 is_page_present = hash_table_search(main_ht, str8_struct(&src_page->v.voff), &raw_page);

      if (is_page_present) {
        // page exists concat voffs
        Assert(raw_page.size == sizeof(LNK_BaseRelocPageNode));
        LNK_BaseRelocPageNode *page = (LNK_BaseRelocPageNode *) raw_page.str;
        Assert(page != src_page);
        u64_list_concat_in_place(&page->v.entries, &src_page->v.entries);
      } else {
        // push page to main list
        SLLQueuePush(main_page_list->first, main_page_list->last, src_page);
        main_page_list->count += 1;

        // store lookup voff 
        hash_table_push_nocopy(arena->v[0], main_ht, str8_struct(&src_page->v.voff), str8_struct(src_page));
      }
    }
  }

  ProfEnd();
  
  // push storage for section
  LNK_Section *base_reloc_sect   = lnk_section_table_push(st, str8_lit(".reloc"), LNK_RELOC_SECTION_FLAGS);
  LNK_Symbol  *base_reloc_symbol = lnk_make_defined_symbol_chunk(symtab->arena, str8_lit(LNK_BASE_RELOC_SYMBOL_NAME), LNK_DefinedSymbolVisibility_Internal, 0, base_reloc_sect->root, 0, 0, 0);
  lnk_symbol_table_push(symtab, base_reloc_symbol);

  ProfBegin("Page List -> Array");
  LNK_BaseRelocPageArray page_arr = lnk_base_reloc_page_array_from_list(base_reloc_sect->arena, *main_page_list);
  ProfEnd();

  ProfBegin("Sort Pages on VOFF");
  lnk_base_reloc_page_array_sort(page_arr);
  ProfEnd();

  HashTable *voff_ht = hash_table_init(arena->v[0], page_size);
  
  ProfBegin("Serialize Pages");
  for (U64 page_idx = 0; page_idx < page_arr.count; ++page_idx) {
    LNK_BaseRelocPage *page = &page_arr.v[page_idx];

    // push buffer
    U64 buf_align = sizeof(U32);
    U64 buf_size  = AlignPow2(sizeof(U32)*2 + sizeof(U16)*page->entries.count, buf_align);
    U8 *buf       = push_array_no_zero(base_reloc_sect->arena, U8, buf_size);

    // setup pointers into buffer
    U32 *page_voff_ptr  = (U32*)buf;
    U32 *block_size_ptr = page_voff_ptr + 1;
    U16 *reloc_arr_base = (U16*)(block_size_ptr + 1);
    U16 *reloc_arr_ptr  = reloc_arr_base;

    // write reloc array
    for (U64Node *i = page->entries.first; i != 0; i = i->next) {
      // was base reloc entry made?
      if (hash_table_search_u64(voff_ht, i->data, 0)) {
        continue;
      }
      hash_table_push_u64(arena->v[0], voff_ht, i->data, str8(0,0));

      // write entry
      U64 rel_off = i->data - page->voff;
      Assert(rel_off <= page_size);
      *reloc_arr_ptr++ = PE_BaseRelocMake(PE_BaseRelocKind_DIR64, rel_off);
    }

    // write pad
    U64 pad_reloc_count = AlignPadPow2(page->entries.count, sizeof(reloc_arr_ptr[0]));
    MemoryZeroTyped(reloc_arr_ptr, pad_reloc_count); // fill pad with PE_BaseRelocKind_ABSOLUTE
    reloc_arr_ptr += pad_reloc_count;

    // compute block size
    U64 reloc_arr_size = (U64)((U8*)reloc_arr_ptr - (U8*)reloc_arr_base);
    U64 block_size     = sizeof(*page_voff_ptr) + sizeof(*block_size_ptr) + reloc_arr_size;
    
    // write header
    *page_voff_ptr  = safe_cast_u32(page->voff);
    *block_size_ptr = safe_cast_u32(block_size);
    Assert(*block_size_ptr <= buf_size);
    
    // push page chunk
    lnk_section_push_chunk_raw(base_reloc_sect, base_reloc_sect->root, buf, block_size, str8(0,0));

    // purge voffs for next run
    hash_table_purge(voff_ht);
  }
  ProfEnd();

  tp_temp_end(temp);
  ProfEnd();
}

