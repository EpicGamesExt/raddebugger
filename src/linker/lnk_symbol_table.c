// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_init_symbol(LNK_Symbol *symbol, String8 name, LNK_SymbolType type)
{
  symbol->name = name;
  symbol->type = type;
}

internal void
lnk_init_defined_symbol(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags)
{
  switch (visibility) {
  case LNK_DefinedSymbolVisibility_Static:   lnk_init_symbol(symbol, name, LNK_Symbol_DefinedStatic);   break;
  case LNK_DefinedSymbolVisibility_Extern:   lnk_init_symbol(symbol, name, LNK_Symbol_DefinedExtern);   break;
  case LNK_DefinedSymbolVisibility_Internal: lnk_init_symbol(symbol, name, LNK_Symbol_DefinedInternal); break;
  }
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->flags             = flags;
  def->value_type        = LNK_DefinedSymbolValue_Null;
}

internal void
lnk_init_defined_symbol_chunk(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, LNK_Chunk *chunk, U64 offset, COFF_ComdatSelectType selection, U32 check_sum)
{
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->value_type        = LNK_DefinedSymbolValue_Chunk;
  def->u.chunk           = chunk;
  def->u.chunk_offset    = offset;
  def->u.check_sum       = check_sum;
  def->u.selection       = selection;
}

internal void
lnk_init_defined_symbol_va(LNK_Symbol *symbol, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, U64 va)
{
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  LNK_DefinedSymbol *def = &symbol->u.defined;
  def->value_type        = LNK_DefinedSymbolValue_VA;
  def->u.va              = va;
}

internal void
lnk_init_undefined_symbol(LNK_Symbol *symbol, String8 name, LNK_SymbolScopeFlags scope_flags)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Undefined);
  symbol->u.undefined.scope_flags = scope_flags;
}

internal void
lnk_init_weak_symbol(LNK_Symbol *symbol, String8 name, COFF_WeakExtType lookup, LNK_Symbol *fallback)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Weak);
  symbol->u.weak.scope_flags     = LNK_SymbolScopeFlag_Defined;
  symbol->u.weak.lookup_type     = lookup;
  symbol->u.weak.fallback_symbol = fallback;
}

internal void
lnk_init_lazy_symbol(LNK_Symbol *symbol, String8 name, LNK_Lib *lib, U64 member_offset)
{
  lnk_init_symbol(symbol, name, LNK_Symbol_Lazy);
  symbol->u.lazy.lib           = lib;
  symbol->u.lazy.member_offset = member_offset;
}

internal LNK_Symbol *
lnk_make_defined_symbol(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol(symbol, name, visibility, flags);
  return symbol;
}

internal LNK_Symbol * 
lnk_make_defined_symbol_chunk(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, LNK_Chunk *chunk, U64 offset, COFF_ComdatSelectType selection, U32 check_sum)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol_chunk(symbol, name, visibility, flags, chunk, offset, selection, check_sum);
  return symbol;
}

internal LNK_Symbol *
lnk_make_defined_symbol_va(Arena *arena, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, U64 va)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_defined_symbol_va(symbol, name, visibility, flags, va);
  return symbol;
}

internal LNK_Symbol * 
lnk_make_undefined_symbol(Arena *arena, String8 name, LNK_SymbolScopeFlags flags)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_undefined_symbol(symbol, name, flags);
  return symbol;
}

internal LNK_Symbol *
lnk_make_weak_symbol(Arena *arena, String8 name, COFF_WeakExtType lookup, LNK_Symbol *fallback)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_weak_symbol(symbol, name, lookup, fallback);
  return symbol;
}

internal LNK_Symbol *
lnk_make_lazy_symbol(Arena *arena, String8 name, LNK_Lib *lib, U64 member_offset)
{
  LNK_Symbol *symbol = push_array_no_zero(arena, LNK_Symbol, 1);
  lnk_init_lazy_symbol(symbol, name, lib, member_offset);
  return symbol;
}

internal LNK_Chunk *
lnk_chunk_from_symbol(LNK_Symbol *symbol)
{
  if (LNK_Symbol_IsDefined(symbol->type) && symbol->u.defined.value_type == LNK_DefinedSymbolValue_Chunk) {
    return symbol->u.defined.u.chunk;
  }
  return 0;
}

////////////////////////////////

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

internal void
lnk_symbol_list_concat_in_place(LNK_SymbolList *list, LNK_SymbolList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_SymbolNode *
lnk_symbol_list_search_node(LNK_SymbolList list, String8 name, StringMatchFlags flags)
{
  for (LNK_SymbolNode *node = list.first; node != 0; node = node->next) {
    if (str8_match(node->data->name, name, flags)) {
      return node;
    }
  }
  return 0;
}

internal LNK_Symbol *
lnk_symbol_list_search(LNK_SymbolList list, String8 name, StringMatchFlags flags)
{
  LNK_SymbolNode *node = lnk_symbol_list_search_node(list, name, flags);
  return node ? node->data : 0;
}

internal LNK_SymbolList
lnk_symbol_list_from_array(Arena *arena, LNK_SymbolArray arr)
{
  LNK_SymbolList list = {0};
  LNK_SymbolNode *node_arr = push_array_no_zero(arena, LNK_SymbolNode, arr.count);
  for (U64 i = 0; i < arr.count; i += 1) {
    LNK_SymbolNode *node = &node_arr[i];
    node->next           = 0;
    node->data           = &arr.v[i];
    lnk_symbol_list_push_node(&list, node);
  }
  return list;
}

internal LNK_SymbolNodeArray
lnk_symbol_node_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolNodeArray result = {0};
  result.count               = 0;
  result.v                   = push_array_no_zero(arena, LNK_SymbolNode *, list.count);
  for (LNK_SymbolNode *i = list.first; i != 0; i = i->next, ++result.count) {
    result.v[result.count] = i;
  }
  return result;
}

internal LNK_SymbolArray
lnk_symbol_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolArray arr = {0};
  arr.count           = 0;
  arr.v               = push_array_no_zero(arena, LNK_Symbol, list.count);
  for (LNK_SymbolNode *node = list.first; node != 0; node = node->next) {
    arr.v[arr.count++] = *node->data;
  }
  return arr;
}

////////////////////////////////

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

internal B32
lnk_can_replace_symbol(LNK_Symbol *dst, LNK_Symbol *src)
{
  Assert(src->type != LNK_Symbol_Undefined);
  Assert(dst != src);
  Assert(str8_match(dst->name, src->name, 0));

  B32 can_replace = 0;

  // lazy vs lazy
  if (dst->type == LNK_Symbol_Lazy && src->type == LNK_Symbol_Lazy) {
    // link.exe picks symbol from lib that is discovered first
    LNK_Lib *dst_lib = dst->u.lazy.lib;
    LNK_Lib *src_lib = src->u.lazy.lib;
    can_replace = dst_lib->input_idx > src_lib->input_idx;
  }
  // lazy vs weak
  else if (dst->type == LNK_Symbol_Lazy && (LNK_Symbol_IsDefined(src->type) || src->type == LNK_Symbol_Weak)) {
    can_replace = 1;
  }
  // weak vs strong
  else if (dst->type == LNK_Symbol_Weak && LNK_Symbol_IsDefined(src->type)) {
    can_replace = 1;
  }
  // weak vs weak
  else if (dst->type == LNK_Symbol_Weak && src->type == LNK_Symbol_Weak) {
    B32 is_fallback_same = str8_match(dst->u.weak.fallback_symbol->name, src->u.weak.fallback_symbol->name, 0);
    if (is_fallback_same) {
      if (src->obj && !dst->obj) {
        can_replace = 1;
      } else if (src->obj && dst->obj) {
        can_replace = src->obj->input_idx < dst->obj->input_idx;
      }
    } else {
      lnk_error(LNK_Error_MultiplyDefinedSymbol, "multiply defined weak symbol %S, symbol defined in:", src->name);
      lnk_supplement_error("%S", dst->obj->path);
      lnk_supplement_error("%S", src->obj->path);
    }
  }
  // defined VA vs defined chunk
  else if (LNK_Symbol_IsDefined(dst->type) && dst->u.defined.value_type == LNK_DefinedSymbolValue_VA &&
           LNK_Symbol_IsDefined(src->type)) {
    can_replace = 1;
  }
  // defined chunk vs defined chunk
  else if (LNK_Symbol_IsDefined(dst->type) && dst->u.defined.value_type == LNK_DefinedSymbolValue_Chunk &&
           LNK_Symbol_IsDefined(src->type) && src->u.defined.value_type == LNK_DefinedSymbolValue_Chunk) {
    LNK_DefinedSymbol *dst_defn = &dst->u.defined;
    LNK_DefinedSymbol *src_defn = &src->u.defined;

    Assert(dst_defn->u.chunk->is_discarded == 0);
    Assert(dst_defn->u.chunk->type == LNK_Chunk_Leaf);
    Assert(src_defn->u.chunk->type == LNK_Chunk_Leaf);

    COFF_ComdatSelectType dst_select = dst_defn->u.selection;
    COFF_ComdatSelectType src_select = src_defn->u.selection;

    // handle objs compiled with /GR- and /GR
    if ((src_select == COFF_ComdatSelect_Any && dst_select == COFF_ComdatSelect_Largest) ||
        (src_select == COFF_ComdatSelect_Largest && dst_select == COFF_ComdatSelect_Any)) {
      dst_select = COFF_ComdatSelect_Largest;
      src_select = COFF_ComdatSelect_Largest;
    }

    if (src_select == dst_select) {
      LNK_Chunk *dst_chunk      = dst_defn->u.chunk;
      LNK_Chunk *src_chunk      = src_defn->u.chunk;
      U64        dst_chunk_size = lnk_chunk_get_size(dst_chunk);
      U64        src_chunk_size = lnk_chunk_get_size(src_chunk);

      switch (src_select) {
      case COFF_ComdatSelect_Null:
      case COFF_ComdatSelect_Any: {
        if (src_chunk_size == dst_chunk_size) {
          can_replace = src_chunk->input_idx < dst_chunk->input_idx;
        } else {
          // both COMDATs are valid but to get smaller exe pick smallest
          can_replace = src_chunk_size < dst_chunk_size;
        }
      } break;
      case COFF_ComdatSelect_NoDuplicates: {
        lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, src->obj, "multiply defined symbol %S in %S.", dst->name, dst->obj->path);
      } break;
      case COFF_ComdatSelect_SameSize: {
        if (dst_chunk_size != src_chunk_size) {
          lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, src->obj, "multiply defined symbol %S in %S.", dst->name, dst->obj->path);
        }
      } break;
      case COFF_ComdatSelect_ExactMatch: {
        if (dst_defn->u.check_sum != src_defn->u.check_sum) {
          lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, src->obj, "multiply defined symbol %S in %S.", dst->name, dst->obj->path);
        }
      } break;
      case COFF_ComdatSelect_Largest: {
        if (dst_chunk_size == src_chunk_size) {
          if (dst_defn->u.chunk->u.leaf.str == 0 && src_defn->u.chunk->u.leaf.size > 0) {
            // handle communal variable
            //
            // MSVC CRT relies on this behaviour (e.g. __scrt_ucrt_dll_is_in_use in ucrt_detection.c)
            can_replace = 1;
          } else {
            can_replace = src_chunk->input_idx < dst_chunk->input_idx;
          }
        } else {
          can_replace = dst_chunk_size < src_chunk_size;
        }
      } break;
      case COFF_ComdatSelect_Associative: {
        // ignore
      } break;
      default: {
        lnk_error_obj(LNK_Error_InvalidPath, src->obj, "unknown COMDAT selection %#x", src->obj, src_select);
      } break;
      }
    } else {
      String8 src_select_str = coff_string_from_comdat_select_type(src_defn->u.selection); 
      String8 dst_select_str = coff_string_from_comdat_select_type(dst_defn->u.selection);
      lnk_error_obj(LNK_Warning_UnresolvedComdat, src->obj,
                "%S: COMDAT selection conflict detected, current selection %S, leader selection %S from %S", 
                src->name, src_select_str, dst_select_str, dst->obj->path);
    }
  } else {
    lnk_error(LNK_Error_InvalidPath, "unable to find a suitable replacement logic for symbol combination");
  }

  return can_replace;
}

internal void
lnk_on_symbol_replace(LNK_Symbol *dst, LNK_Symbol *src)
{
  Assert(dst != src);

  if (dst->type == LNK_Symbol_Lazy && src->type == LNK_Symbol_Lazy) {
    dst->u.lazy = src->u.lazy;
  } else if (LNK_Symbol_IsDefined(dst->type)) {
    LNK_DefinedSymbol *dst_defined = &dst->u.defined;

    if (dst_defined->value_type == LNK_DefinedSymbolValue_Chunk) {
      // discard chunk from output
      dst_defined->u.chunk->is_discarded = 1;

      if (LNK_Symbol_IsDefined(src->type)) {
        LNK_DefinedSymbol *src_defined = &src->u.defined;

        if (src_defined->value_type == LNK_DefinedSymbolValue_Chunk) {
          // static symbols that are not part of obj's symbol table might point to discarded chunk
          dst_defined->u.chunk->ref = src_defined->u.chunk->ref;

          // copy offset because after folding COMDATS we might end
          // up with larger sized chunk and, for instance, a vftable
          // might have a function pointer preceeding lead symbol
          dst_defined->u.chunk        = src_defined->u.chunk;
          dst_defined->u.chunk_offset = src_defined->u.chunk_offset;
        }
      } else {
        InvalidPath;
      }
    }
  }
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
        LNK_Symbol *dst = ins_atomic_ptr_eval_assign(&curr_trie->symbol, 0);

        // apply replacement logic
        LNK_Symbol *current_symbol = dst;
        if (dst) {
          if (lnk_can_replace_symbol(dst, src)) {
            // HACK: patch dst because relocations might point to it
            lnk_on_symbol_replace(dst, src);
            current_symbol = src;
          } else {
            // discard source
            lnk_on_symbol_replace(src, dst);
          }
        }

        // try replacing symbol, if another thread has already taken the slot, rerun the whole loop
        dst = ins_atomic_ptr_eval_cond_assign(&curr_trie->symbol, current_symbol, 0);

        // symbol replaced, exit
        if (dst == 0) {
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
    if (curr->symbol) {
      if (str8_match(curr->symbol->name, name, 0)) {
        result = curr;
        break;
      }
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

////////////////////////////////

internal U64
lnk_symbol_hash(String8 string)
{
  XXH3_state_t hasher; XXH3_64bits_reset(&hasher);
  XXH3_64bits_update(&hasher, &string.size, sizeof(string.size));
  XXH3_64bits_update(&hasher, string.str, string.size);
  XXH64_hash_t result = XXH3_64bits_digest(&hasher);
  return result;
}

internal LNK_SymbolTable *
lnk_symbol_table_init(TP_Arena *arena)
{
  LNK_SymbolTable *symtab = push_array(arena->v[0], LNK_SymbolTable, 1);
  symtab->arena           = arena;
  for (U64 i = 0; i < LNK_SymbolScopeIndex_Count; ++i) {
    symtab->chunk_lists[i] = push_array(arena->v[0], LNK_SymbolHashTrieChunkList, arena->count);
  }
  return symtab;
}

internal LNK_Symbol *
lnk_symbol_table_search_hash(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, U64 hash, String8 name)
{
  LNK_Symbol *result = 0;
  while (scope_flags) {
    LNK_SymbolScopeIndex scope_idx = ctz64(scope_flags);
    scope_flags &= scope_flags - 1;

    LNK_SymbolHashTrie *match = lnk_symbol_hash_trie_search(symtab->scopes[scope_idx], hash, name);
    if (match) {
      result = match->symbol;
      break;
    }
  }
  return result;
}

internal LNK_Symbol *
lnk_symbol_table_search(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope, String8 name)
{
  U64 hash = lnk_symbol_hash(name);
  return lnk_symbol_table_search_hash(symtab, scope, hash, name);
}

internal LNK_Symbol *
lnk_symbol_table_searchf(LNK_SymbolTable *symtab, LNK_SymbolScopeFlags scope_flags, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  
  va_list args;
  va_start(args, fmt);
  String8 name = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, scope_flags, name);

  scratch_end(scratch);
  return symbol;
}

internal void
lnk_symbol_table_push_(LNK_SymbolTable *symtab, Arena *arena, LNK_SymbolHashTrieChunkList *chunk_list, LNK_SymbolScopeIndex scope_idx, U64 hash, LNK_Symbol *symbol)
{
  lnk_symbol_hash_trie_insert_or_replace(arena, chunk_list, &symtab->scopes[scope_idx], hash, symbol);
}

internal void
lnk_symbol_table_push_hash(LNK_SymbolTable *symtab, U64 hash, LNK_Symbol *symbol)
{
  switch (symbol->type) {
  case LNK_Symbol_Null: break;

  case LNK_Symbol_DefinedExtern: {
    lnk_symbol_table_push_(symtab, symtab->arena->v[0], &symtab->chunk_lists[LNK_SymbolScopeIndex_Defined][0], LNK_SymbolScopeIndex_Defined, hash, symbol);
  } break;

  case LNK_Symbol_DefinedInternal: {
    lnk_symbol_table_push_(symtab, symtab->arena->v[0], &symtab->chunk_lists[LNK_SymbolScopeIndex_Internal][0], LNK_SymbolScopeIndex_Internal, hash, symbol);
  } break;

  case LNK_Symbol_Weak: {
    lnk_symbol_table_push_(symtab, symtab->arena->v[0], &symtab->chunk_lists[LNK_SymbolScopeIndex_Weak][0], LNK_SymbolScopeIndex_Weak, hash, symbol);
  } break;

  case LNK_Symbol_Lazy: {
    lnk_symbol_table_push_(symtab, symtab->arena->v[0], &symtab->chunk_lists[LNK_SymbolScopeIndex_Lib][0], LNK_SymbolScopeIndex_Lib, hash, symbol);
  } break;

  // symbols not supported
  case LNK_Symbol_Undefined:
  case LNK_Symbol_DefinedStatic: {
    InvalidPath;
  } break;
  }
}

internal void
lnk_symbol_table_push(LNK_SymbolTable *symtab, LNK_Symbol *symbol)
{
  U64 hash = lnk_symbol_hash(symbol->name);
  lnk_symbol_table_push_hash(symtab, hash, symbol);
}

internal void
lnk_symbol_table_remove(LNK_SymbolTable *symtab, LNK_SymbolScopeIndex scope, String8 name)
{
  U64                 hash = lnk_symbol_hash(name);
  LNK_SymbolHashTrie *trie = lnk_symbol_hash_trie_search(symtab->scopes[scope], hash, name);
  if (trie) {
    lnk_symbol_hash_trie_remove(trie);
  }
}

internal LNK_Symbol *
lnk_symbol_table_push_defined_chunk(LNK_SymbolTable *symtab, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, LNK_Chunk *chunk, U64 offset, COFF_ComdatSelectType selection, U32 check_sum)
{
  LNK_Symbol *symbol = lnk_make_defined_symbol_chunk(symtab->arena->v[0], name, visibility, flags, chunk, offset, selection, check_sum);
  lnk_symbol_table_push(symtab, symbol);
  return symbol;
}

internal LNK_Symbol *
lnk_symbol_table_push_defined(LNK_SymbolTable *symtab, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags)
{
  LNK_Symbol *symbol = lnk_make_defined_symbol(symtab->arena->v[0], name, visibility, flags);
  lnk_symbol_table_push(symtab, symbol);
  return symbol;
}

internal LNK_Symbol *
lnk_symbol_table_push_defined_va(LNK_SymbolTable *symtab, String8 name, LNK_DefinedSymbolVisibility visibility, LNK_DefinedSymbolFlags flags, U64 va)
{
  LNK_Symbol *symbol = lnk_make_defined_symbol_va(symtab->arena->v[0], name, visibility, flags, va);
  lnk_symbol_table_push(symtab, symbol);
  return symbol;
}

internal LNK_Symbol *
lnk_symbol_table_push_weak(LNK_SymbolTable *symtab, String8 weak_name, COFF_WeakExtType lookup, String8 strong_name)
{
  weak_name   = push_str8_copy(symtab->arena->v[0], weak_name);
  strong_name = push_str8_copy(symtab->arena->v[0], strong_name);
  LNK_Symbol *strong_symbol = lnk_make_undefined_symbol(symtab->arena->v[0], strong_name, LNK_SymbolScopeFlag_Main);
  LNK_Symbol *weak_symbol   = lnk_make_weak_symbol(symtab->arena->v[0], weak_name, COFF_WeakExt_SearchAlias, strong_symbol);
  lnk_symbol_table_push(symtab, weak_symbol);
  return weak_symbol;
}

internal LNK_Symbol *
lnk_resolve_symbol(LNK_SymbolTable *symtab, LNK_Symbol *resolve_symbol)
{
  LNK_Symbol *symbol = resolve_symbol;
  B32 run_resolver;
  do {
    run_resolver = 0;
    switch (symbol->type) {
    case LNK_Symbol_Null: break;
    case LNK_Symbol_Undefined: {
      LNK_UndefinedSymbol *undef_symbol = &symbol->u.undefined;
      LNK_Symbol *def = lnk_symbol_table_search(symtab, undef_symbol->scope_flags, symbol->name);
      if (def) {
        symbol = def;
        run_resolver = 1;
      }
    } break;
    case LNK_Symbol_Weak: {
      LNK_WeakSymbol *weak = &symbol->u.weak;
      LNK_Symbol *def = lnk_symbol_table_search(symtab, weak->scope_flags, symbol->name);
      if (def) {
        Assert(LNK_Symbol_IsDefined(def->type));
        symbol = def;
      } else {
        symbol = symbol->u.weak.fallback_symbol;
      } 
      run_resolver = 1;
    } break;
    case LNK_Symbol_DefinedExtern: {
      // search for defined symbol because we don't update symbol pointers in relocations
      // whenver we replace them in the symbol table
      symbol = lnk_symbol_table_search(symtab, LNK_SymbolScopeFlag_Defined, symbol->name);
      Assert(symbol);
    } break;
    case LNK_Symbol_DefinedStatic:
    case LNK_Symbol_DefinedInternal: { 
      // symbol resolved
    } break;
    default: NotImplemented;
    }
  } while (run_resolver);
  return symbol;
}

#if 0

internal void
lnk_symbol_hash_trie_debug(LNK_SymbolHashTrie *root)
{
  Temp scratch = scratch_begin(0,0);

  struct Stack {
    struct Stack       *next;
    U64                 i;
    LNK_SymbolHashTrie *trie;
  };

  struct Stack *stack = push_array(scratch.arena, struct Stack, 1);
  stack->i            = 0;
  stack->trie         = root;

  U64 cur_depth = 1;
  U64 max_depth = 0;

  char *dashes = "--------------------------------";

  FILE *f = fopen("trie.txt", "w");

  while (stack) {
    for (; stack->i < ArrayCount(stack->trie->child); ++stack->i) {

      if (stack->i == 0 && stack->trie->symbol) {
        fprintf(f, "%.*s%.*s\n", (int)cur_depth, dashes, str8_varg(stack->trie->symbol->name));
      }

      if (stack->trie->child[stack->i] != 0) {
        struct Stack *frame = push_array(scratch.arena, struct Stack, 1);
        frame->i            = 0;
        frame->trie         = stack->trie->child[stack->i];

        stack->i += 1;
        SLLStackPush(stack, frame);

        cur_depth += 1;
        max_depth = Max(cur_depth, max_depth);

        break;
      }
    }

    if (stack->i >= ArrayCount(stack->trie->child)) {
      cur_depth -= 1;
      SLLStackPop(stack);
    }
  }

  fprintf(f, "Max Depth: %llu\n", max_depth);
  fclose(f);

  scratch_end(scratch);
}

#endif
