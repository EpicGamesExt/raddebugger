// Copyright (c) 2025 Epic Games Tools
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
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, exp_parse->name);
  if (symbol == 0) {
    lnk_error(LNK_Warning_IllExport, "symbol \"%S\" for export doesn't exist", exp_parse->name);
    goto exit;
  }
  if (symbol->type != LNK_Symbol_Defined) {
    lnk_error(LNK_Warning_IllExport, "unable to resolve symbol \"%S\" for export", exp_parse->name);
    goto exit;
  }

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
    COFF_ParsedSymbol defn = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
    B32 is_export_data = !COFF_SymbolType_IsFunc(defn.type);
    if (is_export_data) {
      lnk_error(LNK_Error_IllExport, "export \"%S\" is DATA but has specifier CODE", exp_parse->name);
    }
  } break;
  case COFF_ImportHeader_Data: {
    COFF_ParsedSymbol defn = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
    B32 is_export_code = COFF_SymbolType_IsFunc(defn.type);
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

internal LNK_InputObjList
lnk_export_table_serialize(Arena *arena, LNK_ExportTable *exptab, String8 image_name, COFF_MachineType machine)
{
  Temp scratch = scratch_begin(&arena, 1);

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
  
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  // fill out export table header
  PE_ExportTableHeader *header       = push_array(obj_writer->arena, PE_ExportTableHeader, 1);
  header->ordinal_base               = safe_cast_u16(ordinal_low + 1);
  header->export_address_table_count = safe_cast_u32(exptab->name_export_ht->count + exptab->noname_export_ht->count);
  header->name_pointer_table_count   = safe_cast_u32(exptab->name_export_ht->count);


  // make iamge name c-string
  String8 image_name_cstr = push_cstr(obj_writer->arena, image_name);

  // push sections
  COFF_ObjSection *header_sect          = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$1"), LNK_EDATA_SECTION_FLAGS, str8_struct(header));
  COFF_ObjSection *voff_table_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$2"), LNK_EDATA_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *name_voff_table_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$3"), LNK_EDATA_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *ordinal_table_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$4"), LNK_EDATA_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *string_buffer_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$5"), LNK_EDATA_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *image_name_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$6"), LNK_EDATA_SECTION_FLAGS, image_name_cstr);

  // push symbols
  COFF_ObjSymbol *image_name_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_NAME_VOFF"),          0, image_name_sect);
  COFF_ObjSymbol *address_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_ADDRESS_TABLE_VOFF"), 0, voff_table_sect);
  COFF_ObjSymbol *name_table_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_NAME_POINTER_VOFF"),  0, name_voff_table_sect);
  COFF_ObjSymbol *ordinal_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_ORDINAL_TABLE_VOFF"), 0, ordinal_table_sect);

  // patch export table header
  switch (machine) {
  case COFF_MachineType_Unknown: break;
  case COFF_MachineType_X64: {
    coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_voff),                 image_name_symbol,    COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, export_address_table_voff), address_table_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_pointer_table_voff),   name_table_symbol,    COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, ordinal_table_voff),        ordinal_table_symbol, COFF_Reloc_X64_Addr32Nb);
  } break;
  default: { NotImplemented; } break;
  }


  B8 *is_ordinal_bound = push_array(scratch.arena, B8, exptab->max_ordinal);
  HashTable *exp_ht_arr[] = { exptab->name_export_ht, exptab->noname_export_ht };

  for (U64 ht_idx = 0; ht_idx < ArrayCount(exp_ht_arr); ht_idx += 1) {
    HashTable *ht = exp_ht_arr[ht_idx];

    KeyValuePair *kv_arr = key_value_pairs_from_hash_table(scratch.arena, exptab->name_export_ht);

    // named exports must be lexically sorted
    if (ht_idx == 0) {
      sort_key_value_pairs_as_string_sensitive(kv_arr, exptab->name_export_ht->count);
    }

    for (U64 i = 0; i < ht->count; i += 1) {
      LNK_Export *exp              = kv_arr[i].value_raw;

      {
        U64     export_name_offset = string_buffer_sect->data.total_size;
        String8 export_name_cstr   = push_cstr(obj_writer->arena, exp->name);
        str8_list_push(obj_writer->arena, &string_buffer_sect->data, export_name_cstr);

        String8         export_name_symbol_name = push_str8f(obj_writer->arena, "EXPORT.%S", exp->name);
        COFF_ObjSymbol *export_name_symbol      = coff_obj_writer_push_symbol_static(obj_writer, export_name_symbol_name, export_name_offset, string_buffer_sect);

        U64 export_name_voff_offset = name_voff_table_sect->data.total_size;
        U64 export_name_voff_size   = sizeof(U32);
        U8 *export_name_voff        = push_array(obj_writer->arena, U8, export_name_voff_size);
        str8_list_push(obj_writer->arena, &name_voff_table_sect->data, str8_array(export_name_voff, export_name_voff_size));

        switch (machine) {
        case COFF_MachineType_Unknown: break;
        case COFF_MachineType_X64: { coff_obj_writer_section_push_reloc(obj_writer, name_voff_table_sect, export_name_voff_offset, export_name_symbol, COFF_Reloc_X64_Addr32Nb); } break;
        default: { NotImplemented; } break;
        }
      }

      {
        U16 *ordinal = push_array(obj_writer->arena, U16, 1);
        *ordinal = exp->ordinal - ordinal_low;
        str8_list_push(obj_writer->arena, &ordinal_table_sect->data, str8_struct(ordinal));

        if ( ! is_ordinal_bound[exp->ordinal]) {
          is_ordinal_bound[exp->ordinal] = 1;

          U64  voff_offset = voff_table_sect->data.total_size;
          U32 *voff        = push_array(obj_writer->arena, U32, 1);
          str8_list_push(obj_writer->arena, &voff_table_sect->data, str8_struct(voff));

          COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, exp->name);
          switch (machine) {
          case COFF_MachineType_Unknown: break;
          case COFF_MachineType_X64: { coff_obj_writer_section_push_reloc(obj_writer, voff_table_sect, voff_offset, symbol, COFF_Reloc_X64_Addr32Nb); } break;
          default: { NotImplemented; } break;
          }
        }
      }
    }
  }


  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);

  LNK_InputObjList result = {0};
  LNK_InputObj *input = lnk_input_obj_list_push(arena, &result);
  input->path         = str8_lit("* Exports *");
  input->dedup_id     = input->path;
  input->data         = obj;

  scratch_end(scratch);
  return result;
}



