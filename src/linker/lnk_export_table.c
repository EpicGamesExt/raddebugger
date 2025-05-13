// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
lnk_name_from_export_parse(LNK_ExportParse *exp)
{
  String8 name;
  if (exp->is_forwarder) {
    name = exp->alias;
  } else if (exp->alias.size) {
    name = exp->alias;
  } else {
    name = exp->name;
  }
  return name;
}

internal B32
lnk_parse_export_directive_ex(Arena *arena, String8List directive, String8 obj_path, String8 lib_path, LNK_ExportParse *export_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  B32 is_parsed = 0;

  // parse "alias=name"
  String8     name  = {0};
  String8     alias = {0};
  String8List flags = {0};
  {
    String8List alias_name_split = str8_split_by_string_chars(scratch.arena, directive.first->string, str8_lit("="), 0);
    if (alias_name_split.node_count == 2) {
      alias = alias_name_split.first->string;
      name  = alias_name_split.last->string;
    } else if (alias_name_split.node_count == 1) {
      name = alias_name_split.first->string;
    } else {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_with_loc(LNK_Error_IllExport, obj_path, lib_path, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }

    flags = directive;
    str8_list_pop_front(&flags);
  }

  // discard alias to itself
  if (str8_match(name, alias, 0)) {
    alias = str8_zero();
  }

  // does directive have ordinal?
  U16 ordinal16       = 0;
  String8 ordinal     = {0};
  String8 noname_flag = {0};
  if (str8_match(str8_prefix(str8_list_first(&flags), 1), str8_lit("@"), 0)) {
    // parse ordinal
    ordinal = str8_skip(str8_list_pop_front(&flags)->string, 1);
    if (str8_is_integer(ordinal, 10)) {
      U64 ordinal64 = u64_from_str8(ordinal, 10);
      if (ordinal64 <= max_U16) {
        ordinal16 = (U16)ordinal64;
      } else {
        String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
        lnk_error_with_loc(LNK_Error_IllExport, obj_path, lib_path, "ordinal value must fit into 16-bit integer, \"/EXPORT:%S\"", d);
        goto exit;
      }
    } else {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_with_loc(LNK_Error_IllExport, obj_path, lib_path, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }

    // detect NONAME flag
    if (str8_match(str8_list_first(&flags), str8_lit("NONAME"), StringMatchFlag_CaseInsensitive)) {
      noname_flag = str8_list_pop_front(&flags)->string;
    }
  }

  // detect PRIVATE flag
  String8 private_flag = {0};
  if (str8_match(str8_list_first(&flags), str8_lit("PRIVATE"), StringMatchFlag_CaseInsensitive)) {
    private_flag = str8_list_pop_front(&flags)->string;
  }

  // parse export type
  COFF_ImportType type = COFF_ImportHeader_Code;
  if (flags.node_count) {
    type = coff_import_header_type_from_string(str8_list_pop_front(&flags)->string);
    if (type == COFF_ImportType_Invalid) {
      String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
      lnk_error_with_loc(LNK_Error_IllExport, obj_path, lib_path, "invalid export directive \"/EXPORT:%S\"", d);
      goto exit;
    }
  }

  // are there leftover nodes?
  if (flags.node_count != 0) {
    String8 d = str8_list_join(scratch.arena, &directive, &(StringJoin){.sep=str8_lit(",")});
    lnk_error_with_loc(LNK_Error_IllExport, obj_path, lib_path, "invalid export directive \"/EXPORT:%S\"", d);
    goto exit;
  }

  // fill out export
  export_out->obj_path            = obj_path;
  export_out->lib_path            = lib_path;
  export_out->name                = push_str8_copy(arena, name);
  export_out->alias               = push_str8_copy(arena, alias);
  export_out->type                = type;
  export_out->ordinal             = ordinal16;
  export_out->is_ordinal_assigned = ordinal.size > 0;
  export_out->is_noname_present   = noname_flag.size > 0;
  export_out->is_private          = private_flag.size > 0;
  export_out->is_forwarder        = str8_find_needle(name, 0, str8_lit("."), 0) < name.size;

  is_parsed = 1;
  
exit:;
  scratch_end(scratch);
  ProfEnd();
  return is_parsed;
}

internal B32
lnk_parse_export_directive(Arena *arena, String8 directive, String8 obj_path, String8 lib_path, LNK_ExportParse *export_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List split_directive = str8_split_by_string_chars(scratch.arena, directive, str8_lit(","), 0);
  B32 is_parsed = lnk_parse_export_directive_ex(arena, split_directive, obj_path, lib_path, export_out);
  scratch_end(scratch);
  return is_parsed;
}

internal LNK_ExportParsePtrArray
lnk_array_from_export_list(Arena *arena, LNK_ExportParseList list)
{
  LNK_ExportParsePtrArray result = {0};
  result.v = push_array_no_zero(arena, LNK_ExportParse *, list.count);
  for (LNK_ExportParseNode *exp = list.first; exp != 0; exp = exp->next) {
    result.v[result.count++] = &exp->data;
  }
  return result;
}

internal void
lnk_export_parse_list_push_node(LNK_ExportParseList *list, LNK_ExportParseNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal LNK_ExportParseNode *
lnk_export_parse_list_push(Arena *arena, LNK_ExportParseList *list, LNK_ExportParse data)
{
  LNK_ExportParseNode *node = push_array(arena, LNK_ExportParseNode, 1);
  node->data = data;
  lnk_export_parse_list_push_node(list, node);
  return node;
}

internal void
lnk_export_parse_list_concat_in_place(LNK_ExportParseList *list, LNK_ExportParseList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal int
lnk_named_export_is_before(void *raw_a, void *raw_b)
{
  LNK_ExportParse *a = *(LNK_ExportParse **)raw_a;
  LNK_ExportParse *b = *(LNK_ExportParse **)raw_b;
  int cmp = str8_compar_case_sensitive(&a->name, &b->name);
  return cmp < 0;
}

internal int
lnk_ordinal_export_is_before(void *raw_a, void *raw_b)
{
  LNK_ExportParse *a = raw_a;
  LNK_ExportParse *b = raw_b;
  return a->ordinal < b->ordinal;
}

internal String8
lnk_make_edata_obj(Arena               *arena,
                   LNK_SymbolTable     *symtab,
                   String8              image_name,
                   COFF_MachineType     machine,
                   LNK_ExportParseList  export_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  // compute max ordinal and used ordinal flag array
  U64 ordinal_low = max_U64;
  B8 *is_ordinal_used = push_array(arena, B8, max_U16);
  for (LNK_ExportParseNode *exp_n = export_list.first; exp_n != 0; exp_n = exp_n->next) {
    LNK_ExportParse *exp = &exp_n->data;
    if (exp->is_ordinal_assigned) {
      ordinal_low = Min(ordinal_low, exp->ordinal);
      is_ordinal_used[exp->ordinal] = 1;
    }
  }

  LNK_ExportParsePtrArray named_exports     = {0};
  LNK_ExportParsePtrArray noname_exports    = {0};
  LNK_ExportParsePtrArray forwarder_exports = {0};
  {
    // group exports based on flags
    LNK_ExportParseList named_exports_list     = {0};
    LNK_ExportParseList noname_exports_list    = {0};
    LNK_ExportParseList forwarder_exports_list = {0};
    for (LNK_ExportParseNode *exp_n = export_list.first, *exp_n_next; exp_n != 0; exp_n = exp_n_next) {
      exp_n_next = exp_n->next;
      if (exp_n->data.is_forwarder) {
        lnk_export_parse_list_push_node(&forwarder_exports_list, exp_n);
      } else if (exp_n->data.is_noname_present) {
        AssertAlways(exp_n->data.is_ordinal_assigned);
        lnk_export_parse_list_push_node(&noname_exports_list, exp_n);
      } else {
        lnk_export_parse_list_push_node(&named_exports_list, exp_n);
      }
    }

    // list -> array
    named_exports     = lnk_array_from_export_list(scratch.arena, named_exports_list);
    noname_exports    = lnk_array_from_export_list(scratch.arena, noname_exports_list);
    forwarder_exports = lnk_array_from_export_list(scratch.arena, forwarder_exports_list);

    // sort exports
    radsort(named_exports.v, named_exports.count, lnk_named_export_is_before);
    radsort(noname_exports.v, noname_exports.count, lnk_ordinal_export_is_before);
    radsort(forwarder_exports.v, forwarder_exports.count, lnk_named_export_is_before);

    MemoryZeroStruct(&export_list);
    lnk_export_parse_list_concat_in_place(&export_list, &named_exports_list);
    lnk_export_parse_list_concat_in_place(&export_list, &forwarder_exports_list);
    lnk_export_parse_list_concat_in_place(&export_list, &noname_exports_list);
  }

  // assign omitted ordinals
  {
    U16 last_ordinal = ordinal_low;
    for (U64 exp_idx = 0; exp_idx < named_exports.count; exp_idx += 1) {
      LNK_ExportParse *exp = named_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        for (; last_ordinal < max_U16 && is_ordinal_used[last_ordinal] != 0; last_ordinal += 1);
        exp->ordinal            = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
    for (U64 exp_idx = 0; exp_idx < forwarder_exports.count; exp_idx += 1) {
      LNK_ExportParse *exp = forwarder_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        for (; last_ordinal < max_U16 && is_ordinal_used[last_ordinal] != 0; last_ordinal += 1);
        exp->ordinal            = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
    for (U64 exp_idx = 0; exp_idx < noname_exports.count; exp_idx += 1) {
      LNK_ExportParse *exp = noname_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        exp->ordinal             = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
  }

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  // push sections
  COFF_ObjSection *voff_table_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$2"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *name_voff_table_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$3"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *ordinal_table_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$4"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, str8_zero());
  COFF_ObjSection *string_table_sect    = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$5"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_zero());
  COFF_ObjSection *image_name_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$6"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, push_cstr(obj_writer->arena, image_name));

  ProfBegin("Virtual Offset Table");
  {
    B8                      *is_ordinal_bound = push_array(scratch.arena, B8, max_U16);
    LNK_ExportParsePtrArray *all_exports[]    = { &named_exports, &forwarder_exports, &noname_exports };

    for (U64 arr_idx = 0; arr_idx < ArrayCount(all_exports); arr_idx += 1) {
      for (U64 exp_idx = 0; exp_idx < all_exports[arr_idx]->count; exp_idx += 1) {
        LNK_ExportParse *exp = all_exports[arr_idx]->v[exp_idx];
        if (is_ordinal_bound[exp->ordinal] == 0) {
          // alloc only one slot per ordinal, so it's possible to map ordinal to a virtual offset
          is_ordinal_bound[exp->ordinal] = 1;

          // create slot for the ordinal virtual offset
          U64  voff_offset = voff_table_sect->data.total_size;
          U32 *voff        = push_array(obj_writer->arena, U32, 1);
          str8_list_push(obj_writer->arena, &voff_table_sect->data, str8_struct(voff));

          COFF_ObjSymbol *exp_symbol;
          if (exp->is_forwarder) {
            U64     forwarder_name_offset = string_table_sect->data.total_size;
            String8 forwarder_name_cstr   = push_cstr(obj_writer->arena, exp->name);
            str8_list_push(obj_writer->arena, &string_table_sect->data, forwarder_name_cstr);
            // symbol to the name string
            exp_symbol = coff_obj_writer_push_symbol_static(obj_writer, exp->name, forwarder_name_offset, string_table_sect);
          } else {
            // function or global var symbol
            exp_symbol = coff_obj_writer_push_symbol_undef(obj_writer, exp->name);
          }

          U16 ordinal_nb = exp->ordinal - ordinal_low;
          coff_obj_writer_section_push_reloc(obj_writer, voff_table_sect, ordinal_nb*sizeof(U32), exp_symbol, coff_virt_off_reloc_from_machine(machine));
        }
      }
    }
  }
  ProfEnd();

  ProfBegin("Named & Forwarder Exports");
  {
    LNK_ExportParsePtrArray *exports_with_names[] = { &named_exports, &forwarder_exports };

    // assign hints
    for (U64 arr_idx = 0, hint = 0; arr_idx < ArrayCount(exports_with_names); arr_idx += 1) {
      for (U64 exp_idx = 0; exp_idx < exports_with_names[arr_idx]->count; exp_idx += 1, hint += 1) {
        LNK_ExportParse *exp = exports_with_names[arr_idx]->v[exp_idx];
        exp->hint = hint;
      }
    }

    for (U64 arr_idx = 0; arr_idx < ArrayCount(exports_with_names); arr_idx += 1) {
      LNK_ExportParsePtrArray *exports = exports_with_names[arr_idx];
      for (U64 exp_idx = 0; exp_idx < exports->count; exp_idx += 1) {
        LNK_ExportParse *exp = exports->v[exp_idx];

        String8 name = lnk_name_from_export_parse(exp);

        // store symbol name string
        U64     export_name_offset = string_table_sect->data.total_size;
        String8 export_name_cstr   = push_cstr(obj_writer->arena, name);
        str8_list_push(obj_writer->arena, &string_table_sect->data, export_name_cstr);

        // create symbol for the name string
        String8         export_name_symbol_name = push_str8f(obj_writer->arena, "RAD_NAME:%S", name);
        COFF_ObjSymbol *export_name_symbol      = coff_obj_writer_push_symbol_extern(obj_writer, export_name_symbol_name, export_name_offset, string_table_sect);

        // create slot for export virtual offset
        U64 export_name_voff_offset = name_voff_table_sect->data.total_size;
        U8 *export_name_voff        = push_array(obj_writer->arena, U8, sizeof(U32));
        str8_list_push(obj_writer->arena, &name_voff_table_sect->data, str8_array(export_name_voff, sizeof(U32)));

        // write string's virtual offset
        coff_obj_writer_section_push_reloc(obj_writer, name_voff_table_sect, export_name_voff_offset, export_name_symbol, coff_virt_off_reloc_from_machine(machine));

        // create and store export's ordinal
        U16 *ordinal = push_array(obj_writer->arena, U16, 1);
        *ordinal = exp->ordinal - ordinal_low;
        str8_list_push(obj_writer->arena, &ordinal_table_sect->data, str8_struct(ordinal));
      }
    }
  }
  ProfEnd();

  ProfBegin("NONAME Exports");
  {
    for (U64 exp_idx = 0; exp_idx < noname_exports.count; exp_idx += 1) {
      // create and store export's ordinal
      LNK_ExportParse *exp = noname_exports.v[exp_idx];
      U16 *ordinal = push_array(obj_writer->arena, U16, 1);
      *ordinal = exp->ordinal - ordinal_low;
      str8_list_push(obj_writer->arena, &ordinal_table_sect->data, str8_struct(ordinal));
    }
  }
  ProfEnd();

  // fill out export table header
  PE_ExportTableHeader *header       = push_array(obj_writer->arena, PE_ExportTableHeader, 1);
  header->time_stamp                 = COFF_TimeStamp_Max;
  header->ordinal_base               = safe_cast_u16(ordinal_low);
  header->export_address_table_count = safe_cast_u32(voff_table_sect->data.node_count);
  header->name_pointer_table_count   = safe_cast_u32(name_voff_table_sect->data.node_count);

  // push header field's symbols
  COFF_ObjSymbol *image_name_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_NAME_VOFF"),          0, image_name_sect);
  COFF_ObjSymbol *address_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_ADDRESS_TABLE_VOFF"), 0, voff_table_sect);
  COFF_ObjSymbol *name_table_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_NAME_POINTER_VOFF"),  0, name_voff_table_sect);
  COFF_ObjSymbol *ordinal_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_ORDINAL_TABLE_VOFF"), 0, ordinal_table_sect);

  // push export table header section
  COFF_ObjSection *header_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$1"), LNK_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_struct(header));
  coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_HEADER"), 0, header_sect);

  // patch export table header
  COFF_RelocType virt_off_reloc_type = coff_virt_off_reloc_from_machine(machine);
  coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_voff),                 image_name_symbol,    virt_off_reloc_type);
  coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, export_address_table_voff), address_table_symbol, virt_off_reloc_type);
  coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_pointer_table_voff),   name_table_symbol,    virt_off_reloc_type);
  coff_obj_writer_section_push_reloc(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, ordinal_table_voff),        ordinal_table_symbol, virt_off_reloc_type);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);

  os_write_data_to_file_path(str8_lit("foo.obj"), obj);

  scratch_end(scratch);
  return obj;
}

internal String8List
lnk_build_import_lib(Arena *arena, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, LNK_ExportParseList export_list)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  dll_name = str8_skip_last_slash(dll_name);

  // These objects appear in first three members of any lib that linker produces with /dll.
  // Objects are used by MSVC linker to build import table.
  String8 import_entry_obj           = lnk_build_import_entry_obj(scratch.arena, dll_name, time_stamp, machine);
  String8 null_import_descriptor_obj = lnk_build_null_import_descriptor_obj(scratch.arena, time_stamp, machine);
  String8 null_thunk_data_obj        = lnk_build_null_thunk_data_obj(scratch.arena, dll_name, time_stamp, machine);

  COFF_LibWriter *lib_writer = coff_lib_writer_alloc();

  // push import table nulls
  coff_lib_writer_push_obj(lib_writer, dll_name, import_entry_obj);
  coff_lib_writer_push_obj(lib_writer, dll_name, null_import_descriptor_obj);
  coff_lib_writer_push_obj(lib_writer, dll_name, null_thunk_data_obj);

  // push exports
  for (LNK_ExportParseNode *exp_n = export_list.first; exp_n != 0; exp_n = exp_n->next) {
    LNK_ExportParse *exp = &exp_n->data;
    if (exp->is_noname_present) {
      coff_lib_writer_push_export_by_ordinal(lib_writer, machine, time_stamp, dll_name, exp->type, exp->ordinal);
    } else {
      String8 name = lnk_name_from_export_parse(exp);
      COFF_LibWriterSymbolNode *member_symbol = coff_lib_writer_push_export_by_name(lib_writer, machine, time_stamp, dll_name, exp->type, name, exp->hint);

      COFF_LibWriterSymbol imp_symbol = {0};
      imp_symbol.name       = push_str8f(lib_writer->arena, "__imp_%S", name);
      imp_symbol.member_idx = member_symbol->data.member_idx;
      coff_lib_writer_symbol_list_push(lib_writer->arena, &lib_writer->symbol_list, imp_symbol);
    }
  }

  // serialize lib
  String8List lib = coff_lib_writer_serialize(arena, lib_writer, COFF_TimeStamp_Max, 0, /* emit second member: */ 1);
  coff_lib_writer_release(&lib_writer);
  
  scratch_end(scratch);
  ProfEnd();
  return lib;
}

