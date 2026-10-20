// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_ExportTable *
lnk_export_table_init(TP_Arena *arena)
{
  LNK_ExportTable *table = push_array(arena->v[0], LNK_ExportTable, 1);
  table->arena = arena;
  table->chunks = push_array(arena->v[0], LNK_NameTrieChunkList, arena->count);
  return table;
}

internal String8
lnk_export_undecorated_name(COFF_MachineType machine, String8 name)
{
  if (machine != COFF_MachineType_X86 || name.size == 0 ||
      (name.str[0] != '_' && name.str[0] != '@')) { return str8_zero(); }

  String8 result = str8_skip(name, 1);
  U64 suffix = str8_find_needle(result, 0, str8_lit("@"), 0);
  if (suffix < result.size && str8_is_integer(str8_skip(result, suffix + 1), 10)) {
    result = str8_prefix(result, suffix);
  } else if (name.str[0] == '@') {
    return str8_zero();
  }
  return result;
}

internal void
lnk_export_apply_match(Arena *arena, PE_ExportParse *exp, LNK_ExportMatch match)
{
  if ((match.kind == LNK_ExportMatch_Definition || match.kind == LNK_ExportMatch_ArchiveMember) &&
      !str8_match(exp->name, match.canonical_name, 0)) {
    if (!exp->alias.size) { exp->alias = exp->name; }
    exp->name = push_str8_copy(arena, match.canonical_name);
  }
}

internal void
lnk_export_table_insert_definition(LNK_ExportTable *table, Arena *arena, U64 worker_id, LNK_ObjSymbolRef ref, String8 name)
{
  LNK_ExportDefinition *definition = push_array(arena, LNK_ExportDefinition, 1);
  definition->name = push_str8_copy(arena, name);
  definition->ref = ref;

  B32 inserted = 0;
  LNK_NameTrieNode *exact = lnk_name_trie_find_or_insert(arena, &table->chunks[worker_id], &table->exact,
                                                      u64_hash_from_str8(name), &definition->name, definition, &inserted);
  if (!inserted) {
    // duplicate definitions of one decorated name are not ambiguity; keep a
    // deterministic representative without executing linker conflict rules
    for (;;) {
      LNK_ExportDefinition *old = ins_atomic_ptr_eval(&exact->value);
      if (!lnk_obj_symbol_ref_is_before(&definition->ref, &old->ref)) { break; }
      if (ins_atomic_ptr_eval_cond_assign(&exact->value, definition, old) == old) { break; }
    }
    return;
  }

  String8 undecorated = lnk_export_undecorated_name(table->machine, definition->name);
  if (undecorated.size) {
    String8 *key = push_array(arena, String8, 1);
    *key = undecorated;
    LNK_ExportCandidate *candidate = push_array(arena, LNK_ExportCandidate, 1);
    candidate->definition = exact;
    LNK_NameTrieNode *bucket = lnk_name_trie_find_or_insert(arena, &table->chunks[worker_id], &table->undecorated,
                                                         u64_hash_from_str8(*key), key, candidate, &inserted);
    if (!inserted) {
      do {
        candidate->next = ins_atomic_ptr_eval(&bucket->value);
      } while (ins_atomic_ptr_eval_cond_assign(&bucket->value, candidate, candidate->next) != candidate->next);
    }
  }
}

internal LNK_ExportMatch
lnk_export_match_definition(LNK_SymbolTable *symtab, String8 name, LNK_ObjSymbolRef raw_ref)
{
  LNK_ObjSymbolRef ref = raw_ref;
  if (symtab) {
    LNK_Symbol *symbol = lnk_symbol_table_search(symtab, name);
    // matching runs during input collection, before COMDAT section symlinks
    // exist; follow current symbol/weak bindings without querying layout state
    if (!symbol || !lnk_resolve_weak_symbol(symtab, lnk_ref_from_symbol(symbol), &ref)) {
      return (LNK_ExportMatch){0};
    }
  }
  if (!ref.obj) { return (LNK_ExportMatch){0}; }
  COFF_ParsedSymbol symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(ref.obj, ref.symbol_idx);
  COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
  if (interp != COFF_SymbolValueInterp_Regular && interp != COFF_SymbolValueInterp_Common && interp != COFF_SymbolValueInterp_Abs) {
    return (LNK_ExportMatch){0};
  }
  if (symtab && interp == COFF_SymbolValueInterp_Regular &&
      (ref.obj->coff.sections.headers[symbol.section_number].flags & COFF_SectionFlag_LnkRemove)) {
    return (LNK_ExportMatch){0};
  }
  return (LNK_ExportMatch){ .kind = LNK_ExportMatch_Definition, .canonical_name = name, .definition = ref };
}

internal LNK_ExportMatch
lnk_export_table_match(LNK_ExportTable *table, LNK_SymbolTable *symtab, COFF_MachineType machine, String8 name, LNK_LibList *libs)
{
  // exact definitions (including resolved weak aliases) precede decoration
  LNK_ExportMatch result = {0};
  if (symtab) {
    result = lnk_export_match_definition(symtab, name, (LNK_ObjSymbolRef){0});
  } else {
    LNK_NameTrieNode *node = lnk_name_trie_search(table->exact, u64_hash_from_str8(name), name);
    if (node) {
      LNK_ExportDefinition *definition = node->value;
      result = lnk_export_match_definition(0, definition->name, definition->ref);
    }
  }
  if (result.kind != LNK_ExportMatch_None) { return result; }

  // an exact archive symbol also precedes undecorated candidates; preserve
  // library search order and return a member identity without extracting it
  if (libs) {
    for EachNode(node, LNK_LibNode, libs->first) {
      U32 member_idx;
      if (lnk_search_lib(&node->data, name, &member_idx)) {
        return (LNK_ExportMatch){ .kind           = LNK_ExportMatch_ArchiveMember,
                                  .canonical_name = name,
                                  .lib            = &node->data,
                                  .member_idx     = member_idx
        };
      }
    }
  }

  if (machine != COFF_MachineType_X86) { return result; }
  LNK_NameTrieNode *bucket = lnk_name_trie_search(table->undecorated, u64_hash_from_str8(name), name);
  if (bucket) {
    for EachNode(candidate, LNK_ExportCandidate, bucket->value) {
      LNK_ExportDefinition *definition = candidate->definition->value;
      LNK_ExportMatch       match      = lnk_export_match_definition(symtab, definition->name, definition->ref);
      if (match.kind == LNK_ExportMatch_None) { continue; }
      if (result.kind != LNK_ExportMatch_None && !str8_match(result.canonical_name, match.canonical_name, 0)) {
        return (LNK_ExportMatch){ .kind = LNK_ExportMatch_Ambiguous };
      }
      result = match;
    }
  }
  if (result.kind != LNK_ExportMatch_None) { return result; }

  // only pending export requests need this scan; do not parse archive objects
  // or allocate an export record for every indexed library symbol
  if (libs) {
    for EachNode(node, LNK_LibNode, libs->first) {
      LNK_Lib *lib = &node->data;
      for EachIndex(i, lib->symbol_count) {
        String8 canonical   = lib->symbol_names.v[i];
        String8 undecorated = lnk_export_undecorated_name(machine, canonical);

        if (!undecorated.size || !str8_match(undecorated, name, 0)) { continue; }

        if (result.kind != LNK_ExportMatch_None) {
          if (!str8_match(result.canonical_name, canonical, 0)) {
            return (LNK_ExportMatch){ .kind = LNK_ExportMatch_Ambiguous };
          }
          continue;
        }

        result = (LNK_ExportMatch){ .kind           = LNK_ExportMatch_ArchiveMember,
                                    .canonical_name = canonical,
                                    .lib            = lib,
                                    .member_idx     = lib->symbol_indices[i] - 1
        };
      }
    }
  }
  return result;
}
