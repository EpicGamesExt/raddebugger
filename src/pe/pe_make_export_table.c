// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
pe_name_from_export_parse(PE_ExportParse *exp)
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

internal U16
pe_hint_or_ordinal_from_export_parse(PE_ExportParse *exp)
{
  U16 hint_or_ordinal = max_U16;
  if (exp->import_by == COFF_ImportBy_Ordinal) {
    hint_or_ordinal = exp->ordinal;
  } else if (exp->import_by == COFF_ImportBy_Name) {
    hint_or_ordinal = exp->hint;
  } else {
    NotImplemented;
  }
  return hint_or_ordinal;
}

internal PE_ExportParsePtrArray
pe_array_from_export_list(Arena *arena, PE_ExportParseList list)
{
  PE_ExportParsePtrArray result = {0};
  result.v = push_array_no_zero(arena, PE_ExportParse *, list.count);
  for (PE_ExportParseNode *exp = list.first; exp != 0; exp = exp->next) {
    result.v[result.count++] = &exp->data;
  }
  return result;
}

internal void
pe_export_parse_list_push_node(PE_ExportParseList *list, PE_ExportParseNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal PE_ExportParseNode *
pe_export_parse_list_push(Arena *arena, PE_ExportParseList *list, PE_ExportParse data)
{
  PE_ExportParseNode *node = push_array(arena, PE_ExportParseNode, 1);
  node->data = data;
  pe_export_parse_list_push_node(list, node);
  return node;
}

internal void
pe_export_parse_list_concat_in_place(PE_ExportParseList *list, PE_ExportParseList *to_concat)
{
  if (to_concat->count) {
    if (list->count) {
      list->last->next = to_concat->first;
      list->last = to_concat->last;
    } else {
      list->first = to_concat->first;
      list->last = to_concat->last;
    }
    list->count += to_concat->count;
    MemoryZeroStruct(to_concat);
  }
}

internal int
pe_named_export_is_before(void *raw_a, void *raw_b)
{
  PE_ExportParse *a = *(PE_ExportParse **)raw_a;
  PE_ExportParse *b = *(PE_ExportParse **)raw_b;
  int cmp = str8_compar_case_sensitive(&a->name, &b->name);
  return cmp < 0;
}

internal int
pe_ordinal_export_is_before(void *raw_a, void *raw_b)
{
  PE_ExportParse *a = raw_a;
  PE_ExportParse *b = raw_b;
  return a->ordinal < b->ordinal;
}

internal PE_FinalizedExports
pe_finalize_export_list(Arena *arena, PE_ExportParseList export_list)
{
  PE_ExportParsePtrArray named_exports = {0};
  PE_ExportParsePtrArray ordinal_exports = {0};
  PE_ExportParsePtrArray forwarder_exports = {0};
  {
    // group exports based on flags
    PE_ExportParseList named_exports_list = {0};
    PE_ExportParseList ordinal_exports_list = {0};
    PE_ExportParseList forwarder_exports_list = {0};
    for (PE_ExportParseNode *exp_n = export_list.first, *exp_n_next; exp_n != 0; exp_n = exp_n_next) {
      exp_n_next = exp_n->next;
      if (exp_n->data.is_forwarder) {
        pe_export_parse_list_push_node(&forwarder_exports_list, exp_n);
      } else if (exp_n->data.is_noname_present) {
        AssertAlways(exp_n->data.is_ordinal_assigned);
        pe_export_parse_list_push_node(&ordinal_exports_list, exp_n);
      } else {
        pe_export_parse_list_push_node(&named_exports_list, exp_n);
      }
    }

    // list -> array
    named_exports = pe_array_from_export_list(arena, named_exports_list);
    forwarder_exports = pe_array_from_export_list(arena, forwarder_exports_list);
    ordinal_exports = pe_array_from_export_list(arena, ordinal_exports_list);

    // sort exports
    radsort(named_exports.v, named_exports.count, pe_named_export_is_before);
    radsort(ordinal_exports.v, ordinal_exports.count, pe_ordinal_export_is_before);
    radsort(forwarder_exports.v, forwarder_exports.count, pe_named_export_is_before);

    MemoryZeroStruct(&export_list);
    pe_export_parse_list_concat_in_place(&export_list, &named_exports_list);
    pe_export_parse_list_concat_in_place(&export_list, &forwarder_exports_list);
    pe_export_parse_list_concat_in_place(&export_list, &ordinal_exports_list);
  }

  // compute max ordinal and used ordinal flag array
  U64 ordinal_low = max_U64;
  B8 *is_ordinal_used = push_array(arena, B8, max_U16);
  for (PE_ExportParseNode *exp_n = export_list.first; exp_n != 0; exp_n = exp_n->next) {
    PE_ExportParse *exp = &exp_n->data;
    if (exp->is_ordinal_assigned) {
      ordinal_low = Min(ordinal_low, exp->ordinal);
      is_ordinal_used[exp->ordinal] = 1;
    }
  }
  if (ordinal_low == max_U64) {
    ordinal_low = 1;
  }

  // assign omitted ordinals
  {
    U16 last_ordinal = ordinal_low;
    for (U64 exp_idx = 0; exp_idx < named_exports.count; exp_idx += 1) {
      PE_ExportParse *exp = named_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        for (; last_ordinal < max_U16 && is_ordinal_used[last_ordinal] != 0; last_ordinal += 1);
        exp->ordinal            = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
    for (U64 exp_idx = 0; exp_idx < forwarder_exports.count; exp_idx += 1) {
      PE_ExportParse *exp = forwarder_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        for (; last_ordinal < max_U16 && is_ordinal_used[last_ordinal] != 0; last_ordinal += 1);
        exp->ordinal            = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
    for (U64 exp_idx = 0; exp_idx < ordinal_exports.count; exp_idx += 1) {
      PE_ExportParse *exp = ordinal_exports.v[exp_idx];
      if (!exp->is_ordinal_assigned) {
        exp->ordinal             = last_ordinal;
        exp->is_ordinal_assigned = 1;
        is_ordinal_used[last_ordinal] = 1;
      }
    }
  }

  // assign hints
  {
    U64 hint = 0;
    for (U64 exp_idx = 0; exp_idx < named_exports.count; exp_idx += 1, hint += 1) {
      named_exports.v[exp_idx]->hint = hint;
    }
    for (U64 exp_idx = 0; exp_idx < forwarder_exports.count; exp_idx += 1, hint += 1) {
      forwarder_exports.v[exp_idx]->hint = hint;
    }
  }

  PE_FinalizedExports result = {0};
  result.ordinal_low = ordinal_low;
  result.named_exports = named_exports;
  result.forwarder_exports = forwarder_exports;
  result.ordinal_exports = ordinal_exports;

  return result;
}

internal String8
pe_make_edata_obj(Arena               *arena,
                  String8              image_name,
                  COFF_TimeStamp       time_stamp,
                  COFF_MachineType     machine,
                  PE_FinalizedExports  finalized_exports)
{
  Temp scratch = scratch_begin(&arena, 1);

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  // push sections
  COFF_ObjSection *voff_table_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$2"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *name_voff_table_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$3"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *ordinal_table_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$4"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, str8_zero());
  COFF_ObjSection *string_table_sect    = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$5"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_zero());
  COFF_ObjSection *image_name_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$6"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, push_cstr(obj_writer->arena, image_name));

  ProfBegin("Virtual Offset Table");
  {
    B8 *is_ordinal_bound = push_array(scratch.arena, B8, max_U16);

    for (U64 arr_idx = 0; arr_idx < ArrayCount(finalized_exports.all); arr_idx += 1) {
      for (U64 exp_idx = 0; exp_idx < finalized_exports.all[arr_idx].count; exp_idx += 1) {
        PE_ExportParse *exp = finalized_exports.all[arr_idx].v[exp_idx];
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

          U16 ordinal_nb = exp->ordinal - finalized_exports.ordinal_low;
          coff_obj_writer_section_push_reloc_voff(obj_writer, voff_table_sect, ordinal_nb*sizeof(U32), exp_symbol);
        }
      }
    }
  }
  ProfEnd();

  ProfBegin("Named & Forwarder Exports");
  {
    for (U64 arr_idx = 0; arr_idx < ArrayCount(finalized_exports.exports_with_names); arr_idx += 1) {
      PE_ExportParsePtrArray exports = finalized_exports.exports_with_names[arr_idx];
      for (U64 exp_idx = 0; exp_idx < exports.count; exp_idx += 1) {
        PE_ExportParse *exp = exports.v[exp_idx];

        String8 name = pe_name_from_export_parse(exp);

        // store symbol name string
        U64     export_name_offset = string_table_sect->data.total_size;
        String8 export_name_cstr   = push_cstr(obj_writer->arena, name);
        str8_list_push(obj_writer->arena, &string_table_sect->data, export_name_cstr);

        // create symbol for the name string
        String8         export_name_symbol_name = push_str8f(obj_writer->arena, "%S", name);
        COFF_ObjSymbol *export_name_symbol      = coff_obj_writer_push_symbol_static(obj_writer, export_name_symbol_name, export_name_offset, string_table_sect);

        // create slot for export virtual offset
        U64 export_name_voff_offset = name_voff_table_sect->data.total_size;
        U8 *export_name_voff        = push_array(obj_writer->arena, U8, sizeof(U32));
        str8_list_push(obj_writer->arena, &name_voff_table_sect->data, str8_array(export_name_voff, sizeof(U32)));

        // write string's virtual offset
        coff_obj_writer_section_push_reloc_voff(obj_writer, name_voff_table_sect, export_name_voff_offset, export_name_symbol);

        // create and store export's ordinal
        U16 *ordinal = push_array(obj_writer->arena, U16, 1);
        *ordinal = exp->ordinal - finalized_exports.ordinal_low;
        str8_list_push(obj_writer->arena, &ordinal_table_sect->data, str8_struct(ordinal));
      }
    }
  }
  ProfEnd();

  ProfBegin("Ordinal Exports");
  {
    for (U64 exp_idx = 0; exp_idx < finalized_exports.ordinal_exports.count; exp_idx += 1) {
      // create and store export's ordinal
      PE_ExportParse *exp = finalized_exports.ordinal_exports.v[exp_idx];
      U16 *ordinal = push_array(obj_writer->arena, U16, 1);
      *ordinal = exp->ordinal - finalized_exports.ordinal_low;
      str8_list_push(obj_writer->arena, &ordinal_table_sect->data, str8_struct(ordinal));
    }
  }
  ProfEnd();

  // fill out export table header
  PE_ExportTableHeader *header       = push_array(obj_writer->arena, PE_ExportTableHeader, 1);
  header->time_stamp                 = time_stamp;
  header->ordinal_base               = safe_cast_u16(finalized_exports.ordinal_low);
  header->export_address_table_count = safe_cast_u32(voff_table_sect->data.node_count);
  header->name_pointer_table_count   = safe_cast_u32(name_voff_table_sect->data.node_count);

  // push header field's symbols
  COFF_ObjSymbol *image_name_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_NAME_VOFF"),          0, image_name_sect);
  COFF_ObjSymbol *address_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_ADDRESS_TABLE_VOFF"), 0, voff_table_sect);
  COFF_ObjSymbol *name_table_symbol    = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_NAME_POINTER_VOFF"),  0, name_voff_table_sect);
  COFF_ObjSymbol *ordinal_table_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_HEADER_ORDINAL_TABLE_VOFF"), 0, ordinal_table_sect);

  // push export table header section
  COFF_ObjSection *header_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".edata$1"), PE_EDATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_struct(header));
  coff_obj_writer_push_symbol_static(obj_writer, str8_lit("EXPORT_TABLE_HEADER"), 0, header_sect);

  // patch export table header
  coff_obj_writer_section_push_reloc_voff(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_voff),                 image_name_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, export_address_table_voff), address_table_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, name_pointer_table_voff),   name_table_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, header_sect, OffsetOf(PE_ExportTableHeader, ordinal_table_voff),        ordinal_table_symbol);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);

  scratch_end(scratch);
  return obj;
}

internal String8
pe_make_import_lib(Arena *arena, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, String8 debug_symbols, PE_ExportParseList export_list)
{
  ProfBeginFunction();

  COFF_LibWriter *lib_writer = coff_lib_writer_alloc();

  // These objects appear in first three members of any lib that linker produces with /dll.
  // Objects are used by MSVC linker to build import table.
  String8 import_entry_obj = pe_make_import_entry_obj(lib_writer->arena, dll_name, time_stamp, machine, debug_symbols);
  String8 null_import_descriptor_obj = pe_make_null_import_descriptor_obj(lib_writer->arena, time_stamp, machine, debug_symbols);
  String8 null_thunk_data_obj = pe_make_null_thunk_data_obj(lib_writer->arena, dll_name, time_stamp, machine, debug_symbols);

  // push import table nulls
  coff_lib_writer_push_obj(lib_writer, dll_name, import_entry_obj);
  coff_lib_writer_push_obj(lib_writer, dll_name, null_import_descriptor_obj);
  coff_lib_writer_push_obj(lib_writer, dll_name, null_thunk_data_obj);

  // push exports
  for (PE_ExportParseNode *exp_n = export_list.first; exp_n != 0; exp_n = exp_n->next) {
    PE_ExportParse *exp = &exp_n->data;
    if (exp->is_private) {
      continue;
    }
    String8 name = pe_name_from_export_parse(exp);
    U16 hint_or_ordinal = pe_hint_or_ordinal_from_export_parse(exp);
    coff_lib_writer_push_import(lib_writer, machine, time_stamp, dll_name, exp->import_by, name, hint_or_ordinal, exp->type);
  }

  // serialize lib
  String8 lib = coff_lib_writer_serialize(arena, lib_writer, COFF_TimeStamp_Max, 0, /* emit second member: */ 1);
  coff_lib_writer_release(&lib_writer);
  
  ProfEnd();
  return lib;
}
