// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

int
lnk_export_name_compar(const void *a_, const void *b_)
{
  const LNK_Export *a = (const LNK_Export *)a_;
  const LNK_Export *b = (const LNK_Export *)b_;
  return str8_compar_case_sensitive(&a->name, &b->name);
}

int
lnk_export_ordinal_compar(const void *a_, const void *b_)
{
  const LNK_Export *a = (const LNK_Export *)a_;
  const LNK_Export *b = (const LNK_Export *)b_;
  int cmp = u16_compar(&a->ordinal, &b->ordinal);
  return cmp;
}

internal LNK_ExportTable *
lnk_export_table_alloc(void)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();

  LNK_ExportTable *exptab  = push_array(arena, LNK_ExportTable, 1);
  exptab->arena            = arena;
  exptab->voff_size        = sizeof(U32);
  exptab->max_ordinal      = max_U16;
  exptab->is_ordinal_used  = push_array(arena, B8, exptab->max_ordinal);
  exptab->name_export_ht   = hash_table_init(arena, 0x10000);
  exptab->noname_export_ht = hash_table_init(arena, 0x100);

  ProfEnd();
  return exptab;
}

internal void
lnk_export_table_release(LNK_ExportTable **exptab_ptr)
{
  ProfBeginFunction();
  arena_release((*exptab_ptr)->arena);
  *exptab_ptr = NULL;
  ProfEnd();
}

internal LNK_Export *
lnk_export_table_search(LNK_ExportTable *exptab, String8 name)
{
  KeyValuePair *kv = hash_table_search_string(exptab->name_export_ht, name);
  if (kv) {
    return kv->value_raw;
  }
  return 0;
}

internal LNK_Export *
lnk_export_table_push_export(LNK_ExportTable *exptab, LNK_SymbolTable *symtab, LNK_ExportParse *exp_parse)
{
  LNK_Export *exp = 0;
  
  // get export symbol
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, LNK_SymbolScopeFlag_Main, exp_parse->name);
  if (symbol == 0) {
    lnk_error(LNK_Warning_IllExport, "symbol \"%S\" for export doesn't exist", exp_parse->name);
    goto exit;
  }
  symbol = lnk_resolve_symbol(symtab, symbol);
  if (!LNK_Symbol_IsDefined(symbol->type)) {
    lnk_error(LNK_Warning_IllExport, "unable to resolve symbol \"%S\" for export", exp_parse->name);
    goto exit;
  }
  LNK_DefinedSymbol *def = &symbol->u.defined;

  
  // NOTE: It is possible to export a global variable as CODE
  // with following snippet:
  //    int global_bar = 0;
  //    #pragma comment(linker, "/export:global_bar")
  // for some reason MSVC and LLD don't check symbol type and default
  // to CODE instead of DATA. But if you try export global variable with:
  //    #pragma comment(linker, "/export:global_bar,CODE")
  // MSVC and LLD issue an error. For compatibility sake we do the same thing too.
  COFF_ImportType type = coff_import_header_type_from_string(exp_parse->type);
  switch (type) {
  case COFF_ImportHeader_Code: {
    B32 is_export_data = !(def->flags & (LNK_DefinedSymbolFlag_IsFunc|LNK_DefinedSymbolFlag_IsThunk));
    if (is_export_data) {
      lnk_error(LNK_Error_IllExport, "export \"%S\" is DATA but has specifier CODE", exp_parse->name);
    }
  } break;
  case COFF_ImportHeader_Data: {
    B32 is_export_code = !!(def->flags & (LNK_DefinedSymbolFlag_IsFunc|LNK_DefinedSymbolFlag_IsThunk));
    if (is_export_code) {
      lnk_error(LNK_Error_IllExport, "export \"%S\" is CODE but has specifier DATA", exp_parse->name);
    }
  } break;
  case COFF_ImportHeader_Const: {
    lnk_not_implemented("TODO: COFF_ImportHeader_Const");
  } break;
  default: {
    if (exp_parse->type.size) {
      lnk_error(LNK_Error_IllExport, "invalid type \"%S\" for export \"%S\"", exp_parse->type, exp_parse->name);
    }
  } break;
  }


  // error check multiple def
  exp = lnk_export_table_search(exptab, exp_parse->alias);
  if (exp) {
    if (exp->type != type) {
      lnk_error(LNK_Warning_IllExport, "trying to rexport symbol \"%S\"", exp_parse->alias);
    }
    goto exit;
  }
  exp = lnk_export_table_search(exptab, exp_parse->name);
  if (exp) {
    if (exp->type != type) {
      lnk_error(LNK_Warning_IllExport, "multiple export definition for \"%S\"", exp_parse->name);
    }
    goto exit;
  }
  
  
  // find free ordinal
  U16 ordinal;
  for (ordinal = 0; ordinal < exptab->max_ordinal; ++ordinal) {
    if (!exptab->is_ordinal_used[ordinal]) {
      exptab->is_ordinal_used[ordinal] = 1;
      break;
    }
  }
  
  // ordinal alloc error check
  if (ordinal >= exptab->max_ordinal) {
    lnk_error(LNK_Error_OutOfExportOrdinals, "reached export limit of %u, discarding export %S", exptab->max_ordinal, exp_parse->name);
    goto exit;
  }

  
  // fill out export
  exp             = push_array_no_zero(exptab->arena, LNK_Export, 1);
  exp->next       = 0;
  exp->name       = push_str8_copy(exptab->arena, exp_parse->alias.size > 0 ? exp_parse->alias : exp_parse->name);
  exp->symbol     = symbol;
  exp->id         = exptab->name_export_ht->count;
  exp->ordinal    = ordinal;
  exp->type       = type;
  exp->is_private = 0; // exports through directives are public

  hash_table_push_string_raw(exptab->arena, exptab->name_export_ht, exp->name, exp);
  
  exit:;
  return exp;
}

internal LNK_ExportArray
lnk_export_array_from_list(Arena *arena, LNK_ExportList list)
{
  ProfBeginFunction();
  LNK_ExportArray arr;
  arr.count = 0;
  arr.v = push_array_no_zero(arena, LNK_Export, list.count);
  for (LNK_Export *exp = list.first; exp != NULL; exp = exp->next) {
    arr.v[arr.count++] = *exp;
  }
  ProfEnd();
  return arr;
}

internal void
lnk_build_edata(LNK_ExportTable *exptab, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, String8 image_name, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  // is export table empty?
  if (exptab->name_export_ht->count == 0 && exptab->noname_export_ht->count == 0) {
    goto exit;
  }
  
  // compute ordinal bounds
  U64 ordinal_low;
  for (ordinal_low = 0; ordinal_low < exptab->max_ordinal; ++ordinal_low) {
    if (exptab->is_ordinal_used[ordinal_low]) {
      break;
    }
  }
  U64 ordinal_high;
  for (ordinal_high = exptab->max_ordinal - 1; ordinal_high > 0; --ordinal_high) {
    if (exptab->is_ordinal_used[ordinal_high]) {
      break;
    }
  }
  
  LNK_Section *edata = lnk_section_table_search(sectab, str8_lit(".edata"));
  
  // push header
  PE_ExportTableHeader *header       = push_array(edata->arena, PE_ExportTableHeader, 1);
  header->ordinal_base               = safe_cast_u16(ordinal_low + 1);
  header->export_address_table_count = safe_cast_u32(exptab->name_export_ht->count + exptab->noname_export_ht->count);
  header->name_pointer_table_count   = safe_cast_u32(exptab->name_export_ht->count);
  
  String8 header_data = str8((U8*)header, sizeof(*header));
  String8 image_name_cstr = push_cstr(edata->arena, str8_skip_last_slash(image_name));
  
  // push edata chunks
  LNK_Chunk *header_chunk          = lnk_section_push_chunk_data(edata, edata->root, header_data, str8_lit("a"));
  LNK_Chunk *voff_table_chunk      = lnk_section_push_chunk_list(edata, edata->root, str8_lit("b"));
  LNK_Chunk *name_voff_table_chunk = lnk_section_push_chunk_list(edata, edata->root, str8_lit("c"));
  LNK_Chunk *ordinal_table_chunk   = lnk_section_push_chunk_list(edata, edata->root, str8_lit("d"));
  LNK_Chunk *string_buffer_chunk   = lnk_section_push_chunk_list(edata, edata->root, str8_lit("e"));
  LNK_Chunk *image_name_chunk      = lnk_section_push_chunk_data(edata, string_buffer_chunk, image_name_cstr, str8(0,0));
  lnk_chunk_set_debugf(edata->arena, header_chunk,          "EXPORT_HEADER");
  lnk_chunk_set_debugf(edata->arena, voff_table_chunk,      "EXPORT_ADDRESS_TABLE");
  lnk_chunk_set_debugf(edata->arena, name_voff_table_chunk, "EXPORT_NAME_VOFF_TABLE");
  lnk_chunk_set_debugf(edata->arena, ordinal_table_chunk,   "EXPORT_ORDINAL_TABLE");
  lnk_chunk_set_debugf(edata->arena, string_buffer_chunk,   "EXPORT_STRING_BUFFER");
  lnk_chunk_set_debugf(edata->arena, image_name_chunk,      "EXPORT_IMAGE_NAME");
  
  LNK_Symbol *image_name_symbol    = lnk_symbol_table_push_defined_chunk(symtab, str8_lit("export_table.name_voff"),                 LNK_DefinedSymbolVisibility_Internal, 0, image_name_chunk,      0, 0, 0);
  LNK_Symbol *address_table_symbol = lnk_symbol_table_push_defined_chunk(symtab, str8_lit("export_table.export_address_table_voff"), LNK_DefinedSymbolVisibility_Internal, 0, voff_table_chunk,      0, 0, 0);
  LNK_Symbol *name_table_symbol    = lnk_symbol_table_push_defined_chunk(symtab, str8_lit("export_table.name_pointer_table_voff"),   LNK_DefinedSymbolVisibility_Internal, 0, name_voff_table_chunk, 0, 0, 0);
  LNK_Symbol *ordinal_table_symbol = lnk_symbol_table_push_defined_chunk(symtab, str8_lit("export_table.ordinal_table_voff"),        LNK_DefinedSymbolVisibility_Internal, 0, ordinal_table_chunk,   0, 0, 0);
  
  // patch header fields
  lnk_section_push_reloc(edata, header_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ExportTableHeader, name_voff),                 image_name_symbol);
  lnk_section_push_reloc(edata, header_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ExportTableHeader, export_address_table_voff), address_table_symbol);
  lnk_section_push_reloc(edata, header_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ExportTableHeader, name_pointer_table_voff),   name_table_symbol);
  lnk_section_push_reloc(edata, header_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ExportTableHeader, ordinal_table_voff),        ordinal_table_symbol);
  
  // reserve virtual offset chunks
  LNK_Chunk **ordinal_voff_map = push_array(scratch.arena, LNK_Chunk *, exptab->max_ordinal);
  for (U32 i = ordinal_low; i <= ordinal_high; i += 1) {
    String8 sort_index = str8_from_bits_u32(edata->arena, i);
    LNK_Chunk *voff_chunk = lnk_section_push_chunk_bss(edata, voff_table_chunk, exptab->voff_size, sort_index);
    ordinal_voff_map[i] = voff_chunk;
  }
  
  B8 *is_ordinal_bound = push_array(scratch.arena, B8, exptab->max_ordinal);
  HashTable *exp_ht_arr[] = { exptab->name_export_ht, exptab->noname_export_ht };
  for (HashTable **ht_ptr = &exp_ht_arr[0], **ht_opl = ht_ptr + ArrayCount(exp_ht_arr);
       ht_ptr < ht_opl;
       ht_ptr += 1) {
    KeyValuePair *kv_arr = key_value_pairs_from_hash_table(scratch.arena, *ht_ptr);

    for (U64 i = 0; i < (*ht_ptr)->count; ++i) {
      LNK_Export *exp       = kv_arr[i].value_raw;
      String8     name_cstr = push_cstr(edata->arena, exp->name);
      
      // push name string
      LNK_Chunk *name_chunk = lnk_section_push_chunk_data(edata, string_buffer_chunk, name_cstr, str8(0,0));
      lnk_chunk_set_debugf(edata->arena, name_chunk, "export: %S", name_cstr);
      
      // push name symbol
      String8     name_export_name = push_str8f(symtab->arena->v[0], "export.%S", name_cstr);
      LNK_Symbol *name_symbol      = lnk_symbol_table_push_defined_chunk(symtab, name_export_name, LNK_DefinedSymbolVisibility_Internal, 0, name_chunk, 0, 0, 0);
      
      // name voff
      LNK_Chunk *voff_chunk = lnk_section_push_chunk_bss(edata, name_voff_table_chunk, exptab->voff_size, /* export table must be sorted lexically: */ name_cstr);
      lnk_chunk_set_debugf(edata->arena, voff_chunk, "voff for export name %S", name_cstr);
      
      // link reloc with name symbol
      lnk_section_push_reloc(edata, voff_chunk, LNK_Reloc_VIRT_OFF_32, 0, name_symbol);

      // make ordinal relative
      U16 *ordinal_ptr = push_array(edata->arena, U16, 1);
      *ordinal_ptr = (exp->ordinal - ordinal_low);
      
      // ordinal
      LNK_Chunk *ordinal_chunk = lnk_section_push_chunk_raw(edata, ordinal_table_chunk, ordinal_ptr, sizeof(*ordinal_ptr), /* ordinal table is parallel to the name table: */ name_cstr);
      lnk_chunk_set_debugf(edata->arena, ordinal_chunk, "ordinal %u for %S", exp->ordinal, exp->name);
      
      // (ordinal - ordinal_low) -> export virtual offset
      if ( ! is_ordinal_bound[exp->ordinal]) {
        is_ordinal_bound[exp->ordinal] = 1;
        LNK_Chunk *export_func_voff_chunk = ordinal_voff_map[exp->ordinal];
        lnk_section_push_reloc(edata, export_func_voff_chunk, LNK_Reloc_VIRT_OFF_32, 0, exp->symbol);
      }
    }
  }
  
exit:;
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_collect_exports_from_obj_directives(LNK_ExportTable *exptab, LNK_ObjList obj_list, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  for (LNK_ObjNode *obj_node = obj_list.first; obj_node != 0; obj_node = obj_node->next) {
    for (LNK_ExportParse *exp_parse = obj_node->data.export_parse.first; exp_parse != 0; exp_parse = exp_parse->next) {
      lnk_export_table_push_export(exptab, symtab, exp_parse);
    }
  }
  ProfEnd();
}


