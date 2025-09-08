// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_Symbol *
lnk_make_symbol(Arena *arena, String8 name, LNK_Obj *obj, U32 symbol_idx)
{
  LNK_ObjSymbolRefNode *ref = push_array(arena, LNK_ObjSymbolRefNode, 1);
  ref->v.obj                = obj;
  ref->v.symbol_idx         = symbol_idx;

  LNK_Symbol *symbol     = push_array(arena, LNK_Symbol, 1);
  symbol->name           = name;
  symbol->refs           = ref;

  return symbol;
}

internal int
lnk_obj_symbol_ref_is_before(void *raw_a, void *raw_b)
{
  LNK_ObjSymbolRef *a_ref           = raw_a;
  LNK_ObjSymbolRef *b_ref           = raw_b;
  LNK_Lib          *a_lib           = lnk_obj_get_lib(a_ref->obj);
  LNK_Lib          *b_lib           = lnk_obj_get_lib(b_ref->obj);
  U32               a_lib_input_idx = a_lib ? a_lib->input_idx : 0;
  U32               b_lib_input_idx = b_lib ? b_lib->input_idx : 0;
  if (a_lib_input_idx == b_lib_input_idx) {
    if (a_ref->obj->input_idx == b_ref->obj->input_idx) {
      return a_ref->symbol_idx < b_ref->symbol_idx;
    }
    return a_ref->obj->input_idx < b_ref->obj->input_idx;
  }
  return a_lib_input_idx < b_lib_input_idx;
}

internal int
lnk_obj_symbol_ref_ptr_is_before(void *raw_a, void *raw_b)
{
  LNK_ObjSymbolRef **a = raw_a, **b = raw_b;
  return lnk_obj_symbol_ref_is_before(*a, *b);
}

internal int
lnk_symbol_is_before(void *raw_a, void *raw_b)
{
  LNK_Symbol *a = raw_a, *b = raw_b;
  LNK_ObjSymbolRef a_ref = lnk_ref_from_symbol(a);
  LNK_ObjSymbolRef b_ref = lnk_ref_from_symbol(b);
  return lnk_obj_symbol_ref_is_before(&a_ref, &b_ref);
}

internal int
lnk_symbol_ptr_is_before(void *raw_a, void *raw_b)
{
  return lnk_symbol_is_before(*(LNK_Symbol **)raw_a, *(LNK_Symbol **)raw_b);
}

internal void
lnk_symbol_list_push_node(LNK_SymbolList *list, LNK_SymbolNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal LNK_SymbolNode *
lnk_symbol_list_push(Arena *arena, LNK_SymbolList *list, LNK_Symbol *symbol)
{
  LNK_SymbolNode *node = push_array(arena, LNK_SymbolNode, 1);
  node->data           = symbol;
  lnk_symbol_list_push_node(list, node);
  return node;
}

internal LNK_SymbolHashTrie *
lnk_symbol_hash_trie_chunk_list_push(Arena *arena, LNK_SymbolHashTrieChunkList *list, U64 cap)
{
  if (list->last == 0 || list->last->count >= list->last->cap) {
    LNK_SymbolHashTrieChunk *chunk = push_array(arena, LNK_SymbolHashTrieChunk, 1);
    chunk->cap                     = cap;
    chunk->v                       = push_array_no_zero(arena, LNK_SymbolHashTrie, cap);
    SLLQueuePush(list->first, list->last, chunk);
    ++list->count;
  }

  LNK_SymbolHashTrie *result = &list->last->v[list->last->count++];
  return result;
}

internal void
lnk_error_multiply_defined_symbol(LNK_Symbol *dst, LNK_Symbol *src)
{
  LNK_ObjSymbolRef dst_ref = lnk_ref_from_symbol(dst);
  LNK_ObjSymbolRef src_ref = lnk_ref_from_symbol(src);
  lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, dst_ref.obj, "symbol \"%S\" (No. %#x) is multiply defined in %S (No. %#x)", dst->name, dst_ref.symbol_idx, src_ref.obj->path, src_ref.symbol_idx);
}

internal B32
lnk_can_replace_symbol(LNK_Symbol *dst, LNK_Symbol *src)
{
  B32 can_replace = 0;

  COFF_ParsedSymbol           dst_parsed = lnk_parsed_from_symbol(dst);
  COFF_ParsedSymbol           src_parsed = lnk_parsed_from_symbol(src);
  COFF_SymbolValueInterpType  dst_interp = lnk_interp_from_symbol(dst);
  COFF_SymbolValueInterpType  src_interp = lnk_interp_from_symbol(src);
  LNK_ObjSymbolRef            dst_ref    = lnk_ref_from_symbol(dst);
  LNK_ObjSymbolRef            src_ref    = lnk_ref_from_symbol(src);
  LNK_Obj                    *dst_obj    = dst_ref.obj;
  LNK_Obj                    *src_obj    = src_ref.obj;

  // undefined vs regular
  if (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Regular) {
    can_replace = 1;
  }
  // (weak vs undefined) or (undefined vs weak)
  else if ((dst_interp == COFF_SymbolValueInterp_Weak && src_interp == COFF_SymbolValueInterp_Undefined) || (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Weak)) {
    LNK_Symbol *weak, *undef;
    COFF_ParsedSymbol weak_parsed;
    if (dst_interp == COFF_SymbolValueInterp_Weak) {
      weak = dst, undef = src;
      weak_parsed = dst_parsed;
    } else {
      weak = src, undef = dst;
      weak_parsed = src_parsed;
    }

    LNK_ObjSymbolRef    weak_symbol_ref = lnk_ref_from_symbol(weak);
    COFF_SymbolWeakExt *weak_ext        = coff_parse_weak_tag(weak_parsed, weak_symbol_ref.obj->header.is_big_obj);
    if (weak_ext->characteristics == COFF_WeakExt_SearchLibrary) {
      // NOTE: MSVC does not let a weak symbol to replace an undefined one,
      // but LLD links without errors or warnings, meaning undefined symbols
      // are resolved to the weak, which can potentially change behaviour of
      // the linked image
      can_replace = dst_interp == COFF_SymbolValueInterp_Weak;
    } else if (weak_ext->characteristics == COFF_WeakExt_NoLibrary) {
      can_replace = dst_interp == COFF_SymbolValueInterp_Weak;
    } else if (weak_ext->characteristics == COFF_WeakExt_SearchAlias) {
      can_replace = dst_interp == COFF_SymbolValueInterp_Undefined;
    } else {
      can_replace = lnk_symbol_is_before(src, dst);
    }
  }
  // undefined vs undefined
  else if (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Undefined) {
    can_replace = lnk_symbol_is_before(src, dst);
  }
  // undefined vs common
  else if (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Common) {
    can_replace = 1;
  }
  // undefined vs abs
  else if (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Abs) {
    can_replace = 1;
  }
  // undefined vs debug
  else if (dst_interp == COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Debug) {
    can_replace = 1;
  }
  // regular/common/abs/debug vs undefined
  else if (dst_interp != COFF_SymbolValueInterp_Undefined && src_interp == COFF_SymbolValueInterp_Undefined) {
    can_replace = 0;
  }
  // regular vs abs
  else if (dst_interp == COFF_SymbolValueInterp_Regular && src_interp == COFF_SymbolValueInterp_Abs) {
    lnk_error_multiply_defined_symbol(dst, src);
  }
  // abs vs regular
  else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Regular) {
    lnk_error_multiply_defined_symbol(dst, src);
  }
  // abs vs common
  else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Common) {
    if (lnk_symbol_is_before(dst, src)) {
      can_replace = 1;
    } else {
      lnk_error_multiply_defined_symbol(dst, src);
    }
  }
  // common vs abs
  else if (dst_interp == COFF_SymbolValueInterp_Common && src_interp == COFF_SymbolValueInterp_Abs) {
    if (lnk_symbol_is_before(dst, src)) {
      lnk_error_multiply_defined_symbol(dst, src);
    }
  }
  // abs vs abs
  else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Abs) {
    lnk_error_multiply_defined_symbol(dst, src);
  }
  // weak vs weak
  else if (dst_interp == COFF_SymbolValueInterp_Weak && src_interp == COFF_SymbolValueInterp_Weak) {
    COFF_SymbolWeakExt *dst_ext = coff_parse_weak_tag(dst_parsed, dst_ref.obj->header.is_big_obj);
    COFF_SymbolWeakExt *src_ext = coff_parse_weak_tag(src_parsed, src_ref.obj->header.is_big_obj);
    if ((dst_ext->characteristics == COFF_WeakExt_SearchAlias && src_ext->characteristics != COFF_WeakExt_SearchAlias)) {
      if (lnk_symbol_is_before(dst, src) || src_ext->characteristics == COFF_WeakExt_AntiDependency) {
        can_replace = 0;
      } else {
        lnk_error_multiply_defined_symbol(dst, src);
      }
    } else if (dst_ext->characteristics != COFF_WeakExt_SearchAlias && src_ext->characteristics == COFF_WeakExt_SearchAlias) {
      if (lnk_symbol_is_before(src, dst) || dst_ext->characteristics == COFF_WeakExt_AntiDependency) {
        can_replace = 1;
      } else {
        lnk_error_multiply_defined_symbol(dst, src);
      }
    } else if (dst_ext->characteristics == COFF_WeakExt_SearchAlias && src_ext->characteristics == COFF_WeakExt_SearchAlias) {
      lnk_error_multiply_defined_symbol(dst, src);
    } else {
      can_replace = lnk_symbol_is_before(src, dst);
    }
  }
  // weak vs regular/abs/common
  else if (dst_interp == COFF_SymbolValueInterp_Weak && (src_interp == COFF_SymbolValueInterp_Regular || src_interp == COFF_SymbolValueInterp_Abs || src_interp == COFF_SymbolValueInterp_Common)) {
    can_replace = 1;
  }
  // regular/abs/common vs weak
  else if ((dst_interp == COFF_SymbolValueInterp_Regular || dst_interp == COFF_SymbolValueInterp_Abs || dst_interp == COFF_SymbolValueInterp_Common) && src_interp == COFF_SymbolValueInterp_Weak) {
    can_replace = 0;
  }
  // regular/common vs regular/common
  else if ((dst_interp == COFF_SymbolValueInterp_Regular || dst_interp == COFF_SymbolValueInterp_Common) && (src_interp == COFF_SymbolValueInterp_Regular || src_interp == COFF_SymbolValueInterp_Common)) {
    // parse dst symbol properties
    B32                   dst_is_comdat = 0;
    COFF_ComdatSelectType dst_select;
    U32                   dst_section_length;
    U32                   dst_check_sum;
    if (dst_interp == COFF_SymbolValueInterp_Regular) {
      dst_is_comdat = lnk_try_comdat_props_from_section_number(dst_ref.obj, dst_parsed.section_number, &dst_select, 0, &dst_section_length, &dst_check_sum);
    } else if (dst_interp == COFF_SymbolValueInterp_Common) {
      dst_select         = COFF_ComdatSelect_Largest;
      dst_section_length = dst_parsed.value;
      dst_check_sum      = 0;
      dst_is_comdat      = 1;
    }

    // parse src symbol properties
    B32                   src_is_comdat = 0;
    COFF_ComdatSelectType src_select;
    U32                   src_section_length, src_checks;
    U32                   src_check_sum;
    if (src_interp == COFF_SymbolValueInterp_Regular) {
      src_is_comdat = lnk_try_comdat_props_from_section_number(src_ref.obj, src_parsed.section_number, &src_select, 0, &src_section_length, &src_check_sum);
    } else if (src_interp == COFF_SymbolValueInterp_Common) {
      src_select         = COFF_ComdatSelect_Largest;
      src_section_length = src_parsed.value;
      src_check_sum      = 0;
      src_is_comdat      = 1;
    }

    // regular non-comdat vs communal
    if (dst_interp == COFF_SymbolValueInterp_Regular && !dst_is_comdat && src_interp == COFF_SymbolValueInterp_Common) {
      can_replace = 0;
    }
    // communal vs regular non-comdat
    else if (dst_interp == COFF_SymbolValueInterp_Common && src_interp == COFF_SymbolValueInterp_Regular && !src_is_comdat) {
      can_replace = 1;
    }
    // handle COMDATs
    else if (dst_is_comdat && src_is_comdat) {
      if ((src_select == COFF_ComdatSelect_Any && dst_select == COFF_ComdatSelect_Largest)) {
        src_select = COFF_ComdatSelect_Largest;
      }
      if (src_select == COFF_ComdatSelect_Largest && dst_select == COFF_ComdatSelect_Any) {
        dst_select = COFF_ComdatSelect_Largest;
      }

      if (src_select == dst_select) {
        switch (src_select) {
        case COFF_ComdatSelect_Null:
        case COFF_ComdatSelect_Any: {
          if (src_section_length == dst_section_length) {
            can_replace = lnk_obj_is_before(src_obj, dst_obj);
          } else {
            // both COMDATs are valid but to get smaller exe pick smallest
            can_replace = 0;
          }
        } break;
        case COFF_ComdatSelect_NoDuplicates: {
          lnk_error_multiply_defined_symbol(dst, src);
        } break;
        case COFF_ComdatSelect_SameSize: {
          if (dst_section_length == src_section_length) {
            can_replace = lnk_obj_is_before(src_obj, dst_obj);
          } else {
            lnk_error_multiply_defined_symbol(dst, src);
          }
        } break;
        case COFF_ComdatSelect_ExactMatch: {
          COFF_SectionHeader *dst_sect_header = lnk_coff_section_header_from_section_number(dst_obj, dst_parsed.section_number);
          COFF_SectionHeader *src_sect_header = lnk_coff_section_header_from_section_number(src_obj, src_parsed.section_number);
          String8             dst_data        = str8_substr(dst_obj->data, rng_1u64(dst_sect_header->foff, dst_sect_header->foff + dst_sect_header->fsize));
          String8             src_data        = str8_substr(src_obj->data, rng_1u64(src_sect_header->foff, src_sect_header->foff + src_sect_header->fsize));
          B32                 is_exact_match  = 0;
          if (dst_check_sum != 0 && src_check_sum != 0) {
            is_exact_match = dst_check_sum == src_check_sum && str8_match(dst_data, src_data, 0);
          } else {
            is_exact_match = str8_match(dst_data, src_data, 0);
          }

          if (is_exact_match) {
            can_replace = lnk_obj_is_before(src_obj, dst_obj);
          } else {
            lnk_error_multiply_defined_symbol(dst, src);
          }
        } break;
        case COFF_ComdatSelect_Largest: {
          if (dst_section_length == src_section_length) {
            can_replace = lnk_obj_is_before(src_obj, dst_obj);
          } else {
            can_replace = dst_section_length < src_section_length;
          }
        } break;
        case COFF_ComdatSelect_Associative: { /* ignore */ } break;
        default: { InvalidPath; } break;
        }
      } else {
        lnk_error_obj(LNK_Warning_UnresolvedComdat, src_obj,
            "%S: COMDAT selection conflict detected, current selection %S, leader selection %S from %S", 
            src->name, coff_string_from_comdat_select_type(src_select), coff_string_from_comdat_select_type(dst_select), dst_obj);
      }
    } else {
      lnk_error_multiply_defined_symbol(dst, src);
    }
  } else {
    lnk_error(LNK_Error_InvalidPath, "unable to find a suitable replacement logic for symbol combination");
  }

  return can_replace;
}

internal void
lnk_on_symbol_replace(LNK_Symbol *dst, LNK_Symbol *src)
{
  COFF_ParsedSymbol          dst_parsed = lnk_parsed_from_symbol(dst);
  COFF_SymbolValueInterpType dst_interp = lnk_interp_from_symbol(dst);
  LNK_ObjSymbolRef           dst_ref    = lnk_ref_from_symbol(dst);

  if (dst_interp == COFF_SymbolValueInterp_Regular) {
    // remove replaced section from the output
    COFF_SectionHeader *dst_sect = lnk_coff_section_header_from_section_number(dst_ref.obj, dst_parsed.section_number);
    dst_sect->flags |= COFF_SectionFlag_LnkRemove;

    // remove associated sections from the output
    for (U32Node *associated_section = dst_ref.obj->associated_sections[dst_parsed.section_number];
        associated_section != 0;
        associated_section = associated_section->next) {
      COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(dst_ref.obj, associated_section->data);
      section_header->flags |= COFF_SectionFlag_LnkRemove;
    }
  }

  // merge symbol refs
  LNK_ObjSymbolRefNode *src_last_ref;
  for (src_last_ref = src->refs; src_last_ref->next != 0; src_last_ref = src_last_ref->next);
  src_last_ref->next = dst->refs;

  // assert leader section is live
#if BUILD_DEBUG
  {
    COFF_ParsedSymbol          src_parsed = lnk_parsed_from_symbol(src);
    COFF_SymbolValueInterpType src_interp = lnk_interp_from_symbol(src);
    LNK_ObjSymbolRef           src_ref    = lnk_ref_from_symbol(src);

    if (src_interp == COFF_SymbolValueInterp_Regular) {
      COFF_SectionHeader *src_sect = lnk_coff_section_header_from_section_number(src_ref.obj, src_parsed.section_number);
      AssertAlways(~src_sect->flags & COFF_SectionFlag_LnkRemove);
    }
  }
#endif
}

internal void
lnk_symbol_hash_trie_insert_or_replace(Arena                        *arena,
                                       LNK_SymbolHashTrieChunkList  *chunks,
                                       LNK_SymbolHashTrie          **trie,
                                       U64                           hash,
                                       LNK_Symbol                   *symbol)
{
  LNK_SymbolHashTrie **curr_trie_ptr = trie;
  for (U64 h = hash; ; h <<= 2) {
    // load current pointer
    LNK_SymbolHashTrie *curr_trie = ins_atomic_ptr_eval(curr_trie_ptr);

    if (curr_trie == 0) {
      // init node
      LNK_SymbolHashTrie *new_trie = lnk_symbol_hash_trie_chunk_list_push(arena, chunks, 512);
      new_trie->name               = &symbol->name;
      new_trie->symbol             = symbol;
      MemoryZeroArray(new_trie->child);

      // try to insert new node
      LNK_SymbolHashTrie *cmp = ins_atomic_ptr_eval_cond_assign(curr_trie_ptr, new_trie, curr_trie);

      // was symbol inserted?
      if (cmp == curr_trie) {
        break;
      }

      // rollback chunk list push
      --chunks->last->count;

      // retry insert with trie node from another thread
      curr_trie = cmp;
    }

    // load current symbol
    String8 *curr_name = ins_atomic_ptr_eval(&curr_trie->name);

    if (curr_name && str8_match(*curr_name, symbol->name, 0)) {
      for (LNK_Symbol *src = symbol;;) {
        // try replacing current symbol with zero, otherwise loop back and retry
        LNK_Symbol *leader = ins_atomic_ptr_eval_assign(&curr_trie->symbol, 0);

        // apply replacement
        if (leader) {
          if (lnk_can_replace_symbol(leader, src)) {
            // discard leader
            lnk_on_symbol_replace(leader, src);
            leader = src;
          } else {
            // discard source
            lnk_on_symbol_replace(src, leader);
            src = leader;
          }
        } else {
          leader = src;
        }

        // try replacing symbol, if another thread has already taken the slot, rerun replacement loop again
        LNK_Symbol *was_replaced = ins_atomic_ptr_eval_cond_assign(&curr_trie->symbol, leader, 0);

        // symbol replaced, exit
        if (was_replaced == 0) {
          goto exit;
        }
      }
    }

    // pick child and descend
    curr_trie_ptr = curr_trie->child + (h >> 62);
  }
  exit:;
}

internal LNK_SymbolHashTrie *
lnk_symbol_hash_trie_search(LNK_SymbolHashTrie *trie, U64 hash, String8 name)
{
  LNK_SymbolHashTrie  *result   = 0;
  LNK_SymbolHashTrie **curr_ptr = &trie;
  for (U64 h = hash; ; h <<= 2) {
    LNK_SymbolHashTrie *curr = ins_atomic_ptr_eval(curr_ptr);
    if (curr == 0) {
      break;
    }
    if (curr->name && str8_match(*curr->name, name, 0)) {
      result = curr;
      break;
    }
    curr_ptr = curr->child + (h >> 62);
  }
  return result;
}

internal void
lnk_symbol_hash_trie_remove(LNK_SymbolHashTrie *trie)
{
  ins_atomic_ptr_eval_assign(&trie->name,   0);
  ins_atomic_ptr_eval_assign(&trie->symbol, 0);
}

internal LNK_SymbolHashTrieChunk **
lnk_array_from_symbol_hash_trie_chunk_list(Arena *arena, LNK_SymbolHashTrieChunkList *lists, U64 lists_count, U64 *count_out)
{
  U64 chunks_count = 0;
  for EachIndex(i, lists_count) { chunks_count += lists[i].count; }

  LNK_SymbolHashTrieChunk **chunks        = push_array(arena, LNK_SymbolHashTrieChunk *, chunks_count);
  U64                       chunks_cursor = 0;
  for EachIndex(i, lists_count) {
    for (LNK_SymbolHashTrieChunk *chunk = lists[i].first; chunk != 0; chunk = chunk->next) {
      chunks[chunks_cursor++] = chunk;
    }
  }

  if (count_out) {
    *count_out = chunks_count;
  }

  return chunks;
}

internal LNK_ObjSymbolRef
lnk_ref_from_symbol(LNK_Symbol *symbol)
{
  return symbol->refs->v;
}

internal U64
lnk_ref_count_from_symbol(LNK_Symbol *symbol)
{
  U64 count = 0;
  for (LNK_ObjSymbolRefNode *node = symbol->refs; node != 0; node = node->next, count += 1);
  return count;
}

internal LNK_ObjSymbolRef **
lnk_ref_from_symbol_many(Arena *arena, LNK_Symbol *symbol, U64 *count_out)
{
  // TODO: would be simpler if we sorted refs on insert/update
  U64                refs_count = lnk_ref_count_from_symbol(symbol);
  LNK_ObjSymbolRef **refs       = push_array(arena, LNK_ObjSymbolRef *, refs_count);
  U64                i          = 0;
  for (LNK_ObjSymbolRefNode *node = symbol->refs; node != 0; node = node->next, i += 1) {
    refs[i] = &node->v;
  }
  radsort(refs, refs_count, lnk_obj_symbol_ref_ptr_is_before);
  if (count_out) {
    *count_out = refs_count;
  }
  return refs;
}

internal COFF_ParsedSymbol
lnk_parsed_from_symbol(LNK_Symbol *symbol)
{
  LNK_ObjSymbolRef ref = lnk_ref_from_symbol(symbol);
  return lnk_parsed_symbol_from_coff_symbol_idx(ref.obj, ref.symbol_idx);
}

internal COFF_SymbolValueInterpType
lnk_interp_from_symbol(LNK_Symbol *symbol)
{
  COFF_ParsedSymbol symbol_parsed = lnk_parsed_from_symbol(symbol); 
  return coff_interp_from_parsed_symbol(symbol_parsed);
}

internal U64
lnk_symbol_table_hasher(String8 string)
{
  return u64_hash_from_str8(string);
}

internal LNK_SymbolTable *
lnk_symbol_table_init(TP_Arena *arena)
{
  LNK_SymbolTable *symtab = push_array(arena->v[0], LNK_SymbolTable, 1);
  symtab->arena           = arena;
  symtab->chunks          = push_array(arena->v[0], LNK_SymbolHashTrieChunkList, arena->count);
  return symtab;
}

internal void
lnk_symbol_table_push_(LNK_SymbolTable *symtab, Arena *arena, U64 worker_id, LNK_Symbol *symbol)
{
  U64 hash = lnk_symbol_table_hasher(symbol->name);
  lnk_symbol_hash_trie_insert_or_replace(arena, &symtab->chunks[worker_id], &symtab->root, hash, symbol);
}

internal void
lnk_symbol_table_push(LNK_SymbolTable *symtab, LNK_Symbol *symbol)
{
  lnk_symbol_table_push_(symtab, symtab->arena->v[0], 0, symbol);
}

internal LNK_SymbolHashTrie *
lnk_symbol_table_search_(LNK_SymbolTable *symtab, String8 name)
{
  U64 hash = lnk_symbol_table_hasher(name);
  return lnk_symbol_hash_trie_search(symtab->root, hash, name);
}

internal LNK_Symbol *
lnk_symbol_table_search(LNK_SymbolTable *symtab, String8 name)
{
  LNK_SymbolHashTrie *trie = lnk_symbol_table_search_(symtab, name);
  return trie ? trie->symbol : 0;
}

internal LNK_Symbol *
lnk_symbol_table_searchf(LNK_SymbolTable *symtab, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
 
  va_list args; va_start(args, fmt);
  String8 name = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, name);

  scratch_end(scratch);
  return symbol;
}

internal ISectOff
lnk_sc_from_symbol(LNK_Symbol *symbol)
{
  COFF_ParsedSymbol parsed_symbol = lnk_parsed_from_symbol(symbol);
  ISectOff sc = { .isect = parsed_symbol.section_number, .off = parsed_symbol.value };
  return sc;
}

internal U64
lnk_voff_from_symbol(COFF_SectionHeader **image_section_table, LNK_Symbol *symbol)
{
  ISectOff sc   = lnk_sc_from_symbol(symbol);
  U64      voff = image_section_table[sc.isect]->voff + sc.off;
  return voff;
}

internal U64
lnk_foff_from_symbol(COFF_SectionHeader **image_section_table, LNK_Symbol *symbol)
{
  ISectOff sc   = lnk_sc_from_symbol(symbol);
  U64      foff = image_section_table[sc.isect]->foff + sc.off;
  return foff;
}

internal B32
lnk_resolve_weak_symbol(LNK_SymbolTable *symtab, LNK_ObjSymbolRef symbol, LNK_ObjSymbolRef *resolved_symbol_out)
{
  Temp scratch = scratch_begin(0,0);

  B32 is_resolved = 0;

  struct S { struct S *next; LNK_ObjSymbolRef symbol; B32 is_anti_dep; };
  struct S *sf = 0, *sl = 0;

  LNK_ObjSymbolRef current_symbol = symbol;
  for (;;) {
    // guard against self-referencing weak symbols
    struct S *was_visited = 0;
    for (struct S *s = sf; s != 0; s = s->next) {
      if (MemoryCompare(&s->symbol, &current_symbol, sizeof(LNK_ObjSymbolRef)) == 0) { was_visited = s; break; }
    }
    if (was_visited) {
      String8List chain = {0};
      for (struct S *s = sf; s != 0; s = s->next) {
        COFF_ParsedSymbol s_parsed = lnk_parsed_symbol_from_coff_symbol_idx(s->symbol.obj, s->symbol.symbol_idx);
        str8_list_pushf(scratch.arena, &chain, "\t%S Symbol %S (No. %#x) =>", s->symbol.obj->path, s_parsed.name, s->symbol.symbol_idx);
      }
      COFF_ParsedSymbol symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx(symbol.obj, symbol.symbol_idx);
      str8_list_pushf(scratch.arena, &chain, "\t%S Symbol %S (No. %#x)", sf->symbol.obj->path, symbol_parsed.name, sf->symbol.symbol_idx);

      String8 chain_string = str8_list_join(scratch.arena, &chain, &(StringJoin){ .sep = str8_lit("\n") });
      lnk_error_obj(LNK_Error_WeakCycle, symbol.obj, "unable to resolve cyclic symbol %S; ref chain:\n%S", symbol_parsed.name, chain_string);

      goto exit;
    }

    COFF_ParsedSymbol          current_parsed = lnk_parsed_symbol_from_coff_symbol_idx(current_symbol.obj, current_symbol.symbol_idx);
    COFF_SymbolValueInterpType current_interp = coff_interp_symbol(current_parsed.section_number, current_parsed.value, current_parsed.storage_class);
    if (current_interp == COFF_SymbolValueInterp_Weak) {
      // record visited symbol
      struct S *s = push_array(scratch.arena, struct S, 1);
      s->symbol   = current_symbol;
      SLLQueuePush(sf, sl, s);

      // does weak symbol have a definition?
      LNK_Symbol                 *defn_symbol = lnk_symbol_table_search(symtab, current_parsed.name);
      COFF_ParsedSymbol           defn_parsed = lnk_parsed_from_symbol(defn_symbol);
      COFF_SymbolValueInterpType  defn_interp = coff_interp_symbol(defn_parsed.section_number, defn_parsed.value, defn_parsed.storage_class);
      if (defn_interp != COFF_SymbolValueInterp_Weak) {
        current_symbol = lnk_ref_from_symbol(defn_symbol);
        break;
      }

      COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(current_parsed, current_symbol.obj->header.is_big_obj);

      // no definition -- fallback to default symbol
      COFF_ParsedSymbol           tag_parsed = lnk_parsed_symbol_from_coff_symbol_idx(current_symbol.obj, weak_ext->tag_index);
      COFF_SymbolValueInterpType  tag_interp = coff_interp_symbol(tag_parsed.section_number, tag_parsed.value, tag_parsed.storage_class);
      current_symbol = (LNK_ObjSymbolRef){ .obj = current_symbol.obj, .symbol_idx = weak_ext->tag_index };

      if (weak_ext->characteristics == COFF_WeakExt_AntiDependency) {
        if (tag_interp == COFF_SymbolValueInterp_Undefined || tag_interp == COFF_SymbolValueInterp_Weak) {
          LNK_Symbol *dep_symbol = lnk_symbol_table_search(symtab, tag_parsed.name);
          tag_interp = lnk_interp_from_symbol(dep_symbol);
        }
        if (tag_interp == COFF_SymbolValueInterp_Weak) { goto exit; }
      }
    } else if (current_interp == COFF_SymbolValueInterp_Undefined) {
      LNK_Symbol                 *defn_symbol = lnk_symbol_table_search(symtab, current_parsed.name);
      COFF_SymbolValueInterpType  defn_interp = lnk_interp_from_symbol(defn_symbol);

      // unresolved undefined symbol
      if (defn_interp == COFF_SymbolValueInterp_Undefined) { break; }

      // follow symbol definition
      current_symbol = lnk_ref_from_symbol(defn_symbol);
    } else { break; }
  }

  if (resolved_symbol_out) {
    *resolved_symbol_out = current_symbol;
  }
  is_resolved = 1;

exit:;
  scratch_end(scratch);
  return is_resolved;
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_weak_with_default_symbol_task)
{
  LNK_ReplaceWeakSymbolsWithDefaultSymbolTask *task   = raw_task;
  LNK_SymbolTable                             *symtab = task->symtab;
  LNK_SymbolHashTrieChunk                     *chunk  = task->chunks[task_id];
  for EachIndex(i, chunk->count) {
    LNK_Symbol                 *symbol        = chunk->v[i].symbol;
    LNK_ObjSymbolRef            symbol_ref    = lnk_ref_from_symbol(symbol);
    COFF_ParsedSymbol           symbol_parsed = lnk_parsed_from_symbol(symbol);
    COFF_SymbolValueInterpType  symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
    if (symbol_interp == COFF_SymbolValueInterp_Weak) {
      LNK_ObjSymbolRef resolve = {0};
      if (lnk_resolve_weak_symbol(symtab, symbol_ref, &resolve)) {
        COFF_ParsedSymbol          resolve_parsed = lnk_parsed_symbol_from_coff_symbol_idx(resolve.obj, resolve.symbol_idx);
        COFF_SymbolValueInterpType resolve_interp = coff_interp_from_parsed_symbol(resolve_parsed);
        if (resolve_interp == COFF_SymbolValueInterp_Weak) {
          COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(resolve_parsed, symbol_ref.obj->header.is_big_obj);
          if (symbol_ref.obj->header.is_big_obj) {
            COFF_Symbol32 *symbol32  = symbol_parsed.raw_symbol;
            symbol32->section_number = COFF_Symbol_UndefinedSection;
            symbol32->value          = 0;
            symbol32->storage_class  = COFF_SymStorageClass_External;
          } else {
            COFF_Symbol16 *symbol16  = symbol_parsed.raw_symbol;
            symbol16->section_number = COFF_Symbol_UndefinedSection;
            symbol16->value          = 0;
            symbol16->storage_class  = COFF_SymStorageClass_External;
          }
        } else {
          symbol->refs->v = resolve;
        }
      }
    }
  }
}

internal void
lnk_replace_weak_with_default_symbols(TP_Context *tp, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  U64                       chunks_count = 0;
  LNK_SymbolHashTrieChunk **chunks       = lnk_array_from_symbol_hash_trie_chunk_list(scratch.arena, symtab->chunks, symtab->arena->count, &chunks_count);
  tp_for_parallel(tp, 0, chunks_count, lnk_replace_weak_with_default_symbol_task, &(LNK_ReplaceWeakSymbolsWithDefaultSymbolTask){ .symtab = symtab, .chunks = chunks });
  scratch_end(scratch);
  ProfEnd();
}

