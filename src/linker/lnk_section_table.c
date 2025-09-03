// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_SectionContrib *
lnk_section_contrib_chunk_push(LNK_SectionContribChunk *chunk, U64 count)
{
  Assert(chunk->count + count <= chunk->cap);
  LNK_SectionContrib *result = chunk->v[chunk->count];
  chunk->count += count;
  return result;
}

internal LNK_SectionContrib *
lnk_section_contrib_chunk_push_atomic(LNK_SectionContribChunk *chunk, U64 count)
{
  U64 pos = ins_atomic_u64_add_eval(&chunk->count, count) - count;
  Assert(pos + count <= chunk->cap);
  LNK_SectionContrib *result = chunk->v[pos];
  return result;
}

internal LNK_SectionContribChunk *
lnk_section_contrib_chunk_list_push_chunk(Arena *arena, LNK_SectionContribChunkList *list, U64 cap, String8 sort_idx)
{
  LNK_SectionContribChunk *chunk = push_array(arena, LNK_SectionContribChunk, 1);
  chunk->count    = 0;
  chunk->cap      = cap;
  chunk->v        = push_array(arena, LNK_SectionContrib *, cap);
  chunk->v2       = push_array(arena, LNK_SectionContrib, cap);
  chunk->sort_idx = sort_idx;
  for (U64 i = 0; i < cap; i += 1) { chunk->v[i] = &chunk->v2[i]; }
  SLLQueuePush(list->first, list->last, chunk);
  list->chunk_count += 1;
  return chunk;
}

internal void
lnk_section_contrib_chunk_list_concat_in_place(LNK_SectionContribChunkList *list, LNK_SectionContribChunkList *to_concat)
{
  if (list->chunk_count == 0) {
    *list = *to_concat;
  } else {
    list->last->next   = to_concat->first;
    list->last         = to_concat->last;
    list->chunk_count += to_concat->chunk_count;
  }
}

internal LNK_SectionContribChunk **
lnk_array_from_section_contrib_chunk_list(Arena *arena, LNK_SectionContribChunkList list)
{
  LNK_SectionContribChunk **result = push_array(arena, LNK_SectionContribChunk *, list.chunk_count);
  U64 i = 0;
  for (LNK_SectionContribChunk *chunk = list.first; chunk != 0; chunk = chunk->next, i += 1) {
    result[i] = chunk;
  }
  return result;
}

internal void
lnk_section_list_push_node(LNK_SectionList *list, LNK_SectionNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal void
lnk_section_list_remove_node(LNK_SectionList *list, LNK_SectionNode *node)
{
  DLLRemove(list->first, list->last, node);
  list->count -= 1;
}

internal LNK_SectionArray
lnk_section_array_from_list(Arena *arena, LNK_SectionList list)
{
  LNK_SectionArray result;
  result.count = 0;
  result.v = push_array_no_zero(arena, LNK_Section *, list.count);
  for (LNK_SectionNode *node = list.first; node != 0; node = node->next) {
    result.v[result.count] = &node->data;
    result.count += 1;
  }
  return result;
}

internal String8
lnk_make_name_with_flags(Arena *arena, String8 name, COFF_SectionFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List l = {0};
  str8_list_push(scratch.arena, &l, name);
  str8_list_push(scratch.arena, &l, str8_struct(&flags));
  String8 name_with_flags = str8_list_join(arena, &l, 0);
  scratch_end(scratch);
  return name_with_flags;
}

internal LNK_SectionTable *
lnk_section_table_alloc(void)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  LNK_SectionTable *sectab = push_array(arena, LNK_SectionTable, 1);
  sectab->arena            = arena;
  sectab->sect_ht          = hash_table_init(arena, 256);
  ProfEnd();
  return sectab;
}

internal void
lnk_section_table_release(LNK_SectionTable **sectab_ptr)
{
  ProfBeginFunction();
  LNK_SectionTable *sectab = *sectab_ptr;
  arena_release(sectab->arena);
  *sectab_ptr = 0;
  ProfEnd();
}

internal LNK_Section *
lnk_section_table_push(LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags)
{
  ProfBeginFunction();

  LNK_SectionNode *sect_node = push_array(sectab->arena, LNK_SectionNode, 1);
  LNK_Section     *sect      = &sect_node->data;
  sect->name  = push_str8_copy(sectab->arena, name);
  sect->flags = flags;

  LNK_SectionList *sect_list = &sectab->list;
  DLLPushBack(sect_list->first, sect_list->last, sect_node);
  sect_list->count += 1;

  String8 name_with_flags = lnk_make_name_with_flags(sectab->arena, name, flags);
  hash_table_push_string_raw(sectab->arena, sectab->sect_ht, name_with_flags, sect);

  ProfEnd();
  return sect;
}

internal LNK_SectionNode *
lnk_section_table_remove(LNK_SectionTable *sectab, String8 name)
{
  ProfBeginFunction();
  LNK_SectionNode *node;
  for (node = sectab->list.first; node != 0; node = node->next) {
    if (str8_match(node->data.name, name, 0)) {
      lnk_section_list_remove_node(&sectab->list, node);
      break;
    }
  }
  ProfEnd();
  return node;
}

internal void
lnk_section_table_purge(LNK_SectionTable *sectab, String8 name)
{
  Temp scratch = scratch_begin(0,0);

  LNK_SectionNode *node            = lnk_section_table_remove(sectab, name);
  String8          name_with_flags = lnk_make_name_with_flags(scratch.arena, name, node->data.flags);
  KeyValuePair    *kv              = hash_table_search_string(sectab->sect_ht, name_with_flags);
  kv->key_string   = str8_zero();
  kv->value_raw    = 0;

  scratch_end(scratch);
}

internal LNK_Section *
lnk_section_table_search(LNK_SectionTable *sectab, String8 full_or_partial_name, COFF_SectionFlags flags)
{
  Temp scratch = scratch_begin(0,0);

  String8 name    = {0};
  String8 postfix = {0};
  coff_parse_section_name(full_or_partial_name, &name, &postfix);

  String8      name_with_flags = lnk_make_name_with_flags(scratch.arena, name, flags);
  LNK_Section *section         = 0;
  hash_table_search_string_raw(sectab->sect_ht, name_with_flags, &section);

  scratch_end(scratch);
  return section;
}

internal LNK_SectionArray
lnk_section_table_search_many(Arena *arena, LNK_SectionTable *sectab, String8 full_or_partial_name)
{
  String8 name    = {0};
  String8 postfix = {0};
  coff_parse_section_name(full_or_partial_name, &name, &postfix);

  U64 match_count = 0;
  for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
    if (str8_match(sect_n->data.name, name, 0)) {
      match_count += 1;
    }
  }

  LNK_SectionArray result = {0};

  if (match_count > 0) {
    result.count = 0;
    result.v = push_array(arena, LNK_Section *, match_count);

    for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
      if (str8_match(sect_n->data.name, name, 0)) {
        result.v[result.count++] = &sect_n->data;
      }
    }
  }

  return result;
}

internal void
lnk_section_table_merge(LNK_SectionTable *sectab, LNK_MergeDirectiveList merge_list)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  for (LNK_MergeDirectiveNode *merge_node = merge_list.first; merge_node != 0; merge_node = merge_node->next) {
    LNK_MergeDirective *merge = &merge_node->data;

    // guard against illegal merges
    {
      local_persist String8 illegal_merge_sections[] = {
        str8_lit_comp(".rsrc"),
        str8_lit_comp(".reloc"),
      };
      for EachIndex(i, ArrayCount(illegal_merge_sections)) {
        if (str8_match(merge->src, illegal_merge_sections[i], 0)) {
          lnk_error(LNK_Error_IllegalSectionMerge, "illegal to merge %S with %S", illegal_merge_sections[i], merge->dst);
        }
        if (str8_match(merge->dst, illegal_merge_sections[i], 0)) {
          lnk_error(LNK_Error_IllegalSectionMerge, "illegal to merge %S with %S", merge->src, illegal_merge_sections[i]);
        }
      }
    }

    // guard against circular merges
    {
      if (str8_match(merge_node->data.dst, merge_node->data.src, 0)) {
        lnk_error(LNK_Error_CircularMerge, "detected circular /MERGE:%S=%S", merge_node->data.src, merge_node->data.dst);
      }
      for (LNK_SectionNode *sect_n = sectab->merge_list.first; sect_n != 0; sect_n = sect_n->next) {
        if (str8_match(sect_n->data.name, merge_node->data.dst, 0)) {
          lnk_error(LNK_Error_CircularMerge, "detected circular /MERGE:%S=%S", merge_node->data.src, merge_node->data.dst);
        }
      }
    }
    
    // are we trying to merge section that was already merged?
    LNK_Section *merge_sect = 0;
    hash_table_search_string_raw(sectab->sect_ht, merge->src, &merge_sect);
    if (merge_sect && merge_sect->merge_dst) {
      LNK_Section *dst = merge_sect->merge_dst;
      B32 is_ambiguous_merge = !str8_match(dst->name, merge->dst, 0);
      if (is_ambiguous_merge) {
        lnk_error(LNK_Warning_AmbiguousMerge, "Detected ambiguous section merge:");
        lnk_supplement_error("%S => %S (Merged)", merge_sect->name, dst->name);
        lnk_supplement_error("%S => %S", merge_sect->name, merge->dst);
      }
      continue;
    }
    
    // find source seciton
    LNK_SectionArray src_matches = lnk_section_table_search_many(scratch.arena, sectab, merge->src);
    if (src_matches.count == 0) {
      continue;
    }

    LNK_Section *dst;
    {
      LNK_SectionArray dst_matches = lnk_section_table_search_many(scratch.arena, sectab, merge->dst);

      if (dst_matches.count > 1) {
        lnk_error(LNK_Warning_AmbiguousMerge, "unable to merge %S=%S, too many dest sections (%llu)", merge->src, merge->dst, dst_matches.count);
        continue;
      }

      // push a new section if the destination section does not exist
      if (dst_matches.count == 0) {
        dst = lnk_section_table_push(sectab, merge->dst, src_matches.v[0]->flags);
      } else {
        dst = dst_matches.v[0];
      }
    }

    for EachIndex(src_idx, src_matches.count) {
      LNK_Section *src = src_matches.v[src_idx];

      if (src->flags != dst->flags) {
        lnk_error(LNK_Warning_AmbiguousMerge, "unable to merge %S=%S because of conflicting section flags", merge->src, merge->dst);
        continue;
      }

      // merge section with destination
      lnk_section_contrib_chunk_list_concat_in_place(&dst->contribs, &src->contribs);
      src->merge_dst = dst;

      // remove node from output section list
      LNK_SectionNode *merge_node = lnk_section_table_remove(sectab, src->name);

      // move node to the merge list
      lnk_section_list_push_node(&sectab->merge_list, merge_node);
    }
  }
  scratch_end(scratch);
  ProfEnd();
}

internal int
lnk_section_contrib_chunk_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionContribChunk **a = raw_a, **b = raw_b;
  return str8_is_before_case_sensitive(&(*a)->sort_idx, &(*b)->sort_idx);
}

internal void
lnk_sort_section_contribs(LNK_Section *sect)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  LNK_SectionContribChunk **chunks = lnk_array_from_section_contrib_chunk_list(scratch.arena, sect->contribs);
  radsort(chunks, sect->contribs.chunk_count, lnk_section_contrib_chunk_is_before);

  // repopulate chunk list in sorted order
  sect->contribs.first = 0;
  sect->contribs.last  = 0;
  for (U64 chunk_idx = 0; chunk_idx < sect->contribs.chunk_count; chunk_idx += 1) {
    SLLQueuePush(sect->contribs.first, sect->contribs.last, chunks[chunk_idx]);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_finalize_section_layout(LNK_Section *sect, U64 file_align, U64 pad_size)
{
  ProfBegin("Layout Contribs");
  U64 cursor = 0;
  for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
    for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1) {
      LNK_SectionContrib *sc = sc_chunk->v[sc_idx];

      // add section pad bytes
      if (sc->hotpatch) {
        cursor += pad_size;
      }

      // assign offset
      cursor = AlignPow2(cursor, sc->align);
      sc->u.off = cursor;

      // advance cursor
      U64 sc_size = lnk_size_from_section_contrib(sc);
      cursor += sc_size;
    }
  }
  ProfEnd();

  if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
    sect->fsize = AlignPow2(cursor, file_align);
  }
  sect->vsize = cursor;
}

internal void
lnk_assign_section_index(LNK_Section *sect, U64 sect_idx)
{
  sect->sect_idx = sect_idx;

  // assign section indices to contribs
  for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
    for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1) {
      sc_chunk->v[sc_idx]->u.sect_idx = sect_idx;
    }
  }
}

internal void
lnk_assign_section_virtual_space(LNK_Section *sect, U64 sect_align, U64 *voff_cursor)
{
  sect->voff    = *voff_cursor;
  *voff_cursor += sect->vsize;
  *voff_cursor  = AlignPow2(*voff_cursor, sect_align);
}

internal void
lnk_assign_section_file_space(LNK_Section *sect, U64 *foff_cursor)
{
  if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
    sect->foff    = *foff_cursor;
    *foff_cursor += sect->fsize;
  }
}

internal U64
lnk_size_from_section_contrib(LNK_SectionContrib *sc)
{
  U64 size = 0;
  for (String8Node *n = &sc->first_data_node; n != 0; n = n->next) {
    size += n->string.size;
  }
  return size;
}

internal U64
lnk_voff_from_section_contrib(COFF_SectionHeader **image_section_table, LNK_SectionContrib *sc)
{
  COFF_SectionHeader *sect_header = image_section_table[sc->u.sect_idx+1];
  U64 voff = sect_header->voff + sc->u.off;
  return voff;
}

internal U64
lnk_foff_from_section_contrib(COFF_SectionHeader **image_section_table, LNK_SectionContrib *sc)
{
  COFF_SectionHeader *sect_header = image_section_table[sc->u.sect_idx+1];
  U64 foff = sect_header->foff + sc->u.off;
  return foff;
}

internal U64
lnk_fopl_from_section_contrib(COFF_SectionHeader **image_section_table, LNK_SectionContrib *sc)
{
  U64 foff = lnk_foff_from_section_contrib(image_section_table, sc);
  return foff + lnk_size_from_section_contrib(sc);
}

internal LNK_SectionContrib *
lnk_get_first_section_contrib(LNK_Section *sect)
{
  if (sect->contribs.chunk_count > 0) {
    if (sect->contribs.first->count > 0) {
      return sect->contribs.first->v[0];
    }
  }
  return 0;
}

internal LNK_SectionContrib *
lnk_get_last_section_contrib(LNK_Section *sect)
{
  if (sect->contribs.chunk_count > 0) {
    if (sect->contribs.last->count > 0) {
      return sect->contribs.last->v[sect->contribs.last->count-1];
    }
  }
  return 0;
}

internal U64
lnk_get_section_contrib_size(LNK_Section *sect)
{
  LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(sect);
  LNK_SectionContrib *last_sc = lnk_get_last_section_contrib(sect);
  U64 size = (last_sc->u.off - first_sc->u.off) + lnk_size_from_section_contrib(last_sc);
  return size;
}

internal U64
lnk_get_first_section_contrib_voff(COFF_SectionHeader **image_section_table, LNK_Section *sect)
{
  LNK_SectionContrib *sc = lnk_get_first_section_contrib(sect);
  return lnk_voff_from_section_contrib(image_section_table, sc);
}
