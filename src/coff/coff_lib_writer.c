// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal COFF_LibWriterSymbolNode *
coff_lib_writer_symbol_list_push(Arena *arena, COFF_LibWriterSymbolList *list, COFF_LibWriterSymbol symbol)
{
  COFF_LibWriterSymbolNode *node = push_array_no_zero(arena, COFF_LibWriterSymbolNode, 1);
  node->next = 0;
  node->data = symbol;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal COFF_LibWriterMemberNode *
coff_lib_writer_member_list_push(Arena *arena, COFF_LibWriterMemberList *list, COFF_LibWriterMember member)
{
  COFF_LibWriterMemberNode *node = push_array_no_zero(arena, COFF_LibWriterMemberNode, 1);
  node->next = 0;
  node->data = member;
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal COFF_LibWriterSymbol *
coff_lib_writer_symbol_array_from_list(Arena *arena, COFF_LibWriterSymbolList list)
{
  COFF_LibWriterSymbol *arr = push_array_no_zero(arena, COFF_LibWriterSymbol, list.count + 2);
  COFF_LibWriterSymbol *ptr = arr + 1;
  for (COFF_LibWriterSymbolNode *i = list.first; i != 0; i = i->next, ptr += 1) {
    ptr->name       = push_str8_copy(arena, i->data.name);
    ptr->member_idx = i->data.member_idx;
  }
  MemoryZeroStruct(&arr[0]);
  MemoryZeroStruct(&arr[list.count+1]);
  return arr;
}

internal COFF_LibWriterMember *
coff_lib_writer_member_array_from_list(Arena *arena, COFF_LibWriterMemberList list)
{
  COFF_LibWriterMember *arr = push_array_no_zero(arena, COFF_LibWriterMember, list.count);
  COFF_LibWriterMember *ptr = arr;
  for (COFF_LibWriterMemberNode *i = list.first; i != 0; i = i->next, ptr += 1) {
    ptr->name = push_str8_copy(arena, i->data.name);
    ptr->data = push_str8_copy(arena, i->data.data); 
  }
  return arr;
}

internal int
coff_lib_writer_symbol_name_compar(const void *raw_a, const void *raw_b)
{
  const COFF_LibWriterSymbol *sa = raw_a;
  const COFF_LibWriterSymbol *sb = raw_b;
  return str8_compar_case_sensitive(&sa->name, &sb->name);
}

internal int
coff_lib_writer_symbol_is_before(void *raw_a, void *raw_b)
{
  int compar = coff_lib_writer_symbol_name_compar(raw_a, raw_b);
  return compar < 0;
}

internal void
coff_lib_writer_symbol_array_sort(COFF_LibWriterSymbol *arr, U64 count)
{
  Assert(count >= 2);
  radsort(arr + 1, count - 2, coff_lib_writer_symbol_is_before);
}

internal COFF_LibWriter *
coff_lib_writer_alloc(void)
{
  Arena *arena = arena_alloc();
  COFF_LibWriter *writer = push_array(arena, COFF_LibWriter, 1);
  writer->arena = arena;
  return writer;
}

internal void
coff_lib_writer_release(COFF_LibWriter **writer_ptr)
{
  arena_release((*writer_ptr)->arena);
  *writer_ptr = 0;
}

internal void
coff_lib_writer_push_obj(COFF_LibWriter *writer, String8 obj_path, String8 obj_data)
{
  U64 member_idx = writer->member_list.count;
  
  // push obj member
  COFF_LibWriterMember member = {0};
  member.name          = obj_path;
  member.data          = obj_data;
  coff_lib_writer_member_list_push(writer->arena, &writer->member_list, member);
  
  // push external symbols
  {
    COFF_FileHeaderInfo obj_header   = coff_file_header_info_from_data(obj_data);
    String8             string_table = str8_substr(obj_data, obj_header.string_table_range);
    String8             symbol_table = str8_substr(obj_data, obj_header.symbol_table_range);

    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < obj_header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      void *symbol_ptr;
      if (obj_header.is_big_obj) {
        symbol_ptr = &((COFF_Symbol32 *)symbol_table.str)[symbol_idx];
        symbol     = coff_parse_symbol32(string_table, symbol_ptr);
      } else {
        symbol_ptr = &((COFF_Symbol16 *)symbol_table.str)[symbol_idx];
        symbol     = coff_parse_symbol16(string_table, symbol_ptr);
      }

      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        if (symbol.storage_class == COFF_SymStorageClass_External) {
          COFF_LibWriterSymbol lib_symbol = {0};
          lib_symbol.name          = symbol.name;
          lib_symbol.member_idx    = member_idx;
          coff_lib_writer_symbol_list_push(writer->arena, &writer->symbol_list, lib_symbol);
        }
      }
    }
  }
}

internal void
coff_lib_writer_push_export(COFF_LibWriter *writer, String8 raw_import_header)
{
  U64                            member_idx    = writer->member_list.count;
  COFF_ParsedArchiveImportHeader import_header = coff_archive_import_from_data(raw_import_header);

  // push import member
  COFF_LibWriterMember member = {0};
  member.name = import_header.dll_name;
  member.data = raw_import_header;
  coff_lib_writer_member_list_push(writer->arena, &writer->member_list, member);
  
  switch (import_header.type) {
  case COFF_ImportHeader_Code: {
    COFF_LibWriterSymbol def_symbol = {0};
    def_symbol.name       = push_str8_copy(writer->arena, import_header.func_name);
    def_symbol.member_idx = member_idx;
    coff_lib_writer_symbol_list_push(writer->arena, &writer->symbol_list, def_symbol);

    COFF_LibWriterSymbol imp_symbol = {0};
    imp_symbol.name = push_str8f(writer->arena, "__imp_%S", import_header.func_name);
    imp_symbol.member_idx = member_idx;
    coff_lib_writer_symbol_list_push(writer->arena, &writer->symbol_list, def_symbol);
  } break;
  case COFF_ImportHeader_Data: {
    COFF_LibWriterSymbol imp_symbol = {0};
    imp_symbol.name       = push_str8f(writer->arena, "__imp_%S", import_header.func_name);
    imp_symbol.member_idx = member_idx;
    coff_lib_writer_symbol_list_push(writer->arena, &writer->symbol_list, imp_symbol);
  } break;
  case COFF_ImportHeader_Const: { NotImplemented; } break;
  default: { InvalidPath; } break;
  }
}

internal void
coff_lib_writer_push_export_by_ordinal(COFF_LibWriter *lib_writer, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, COFF_ImportType import_type, U16 ordinal)
{
  String8 import_header = coff_make_import_header_by_ordinal(lib_writer->arena, machine, time_stamp, dll_name, ordinal, import_type);
  coff_lib_writer_push_export(lib_writer, import_header);
}

internal void
coff_lib_writer_push_export_by_name(COFF_LibWriter *lib_writer, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, COFF_ImportType import_type, String8 name, U16 hint)
{
  String8 import_header = coff_make_import_header_by_name(lib_writer->arena, machine, time_stamp, dll_name, name, hint, import_type);
  coff_lib_writer_push_export(lib_writer, import_header);
}

internal String8List
coff_lib_writer_serialize(Arena *arena, COFF_LibWriter *lib_writer, COFF_TimeStamp time_stamp, U16 mode, B32 emit_second_member)
{
  Temp scratch = scratch_begin(&arena, 1);

  // symbol & member lists -> arrays
  U64                   symbols_count;
  COFF_LibWriterSymbol *symbols;
  U64                   member_count;
  COFF_LibWriterMember *member_array;
  {
    U64                   symbols_count_with_null = lib_writer->symbol_list.count + 2;
    COFF_LibWriterSymbol *symbols_with_null       = coff_lib_writer_symbol_array_from_list(scratch.arena, lib_writer->symbol_list);
    coff_lib_writer_symbol_array_sort(symbols_with_null, symbols_count_with_null);
    symbols_count = symbols_count_with_null - 2;
    symbols       = symbols_with_null + 1;

    member_count = lib_writer->member_list.count;
    member_array = coff_lib_writer_member_array_from_list(scratch.arena, lib_writer->member_list);
  }

  // serialize members
  U64         *member_offsets   = push_array_no_zero(scratch.arena, U64, member_count);
  String8List  long_names_list  = {0};
  String8List  member_data_list = {0};
  {
    HashTable *name_ht = hash_table_init(scratch.arena, 1024);
    for (U64 member_idx = 0; member_idx < member_count; member_idx += 1) {
      COFF_LibWriterMember *member = &member_array[member_idx];

      // make member name
      String8 name;
      U64 name_with_slash_size = member->name.size + 1;
      if (name_with_slash_size > COFF_Archive_MaxShortNameSize) {
        // have we seen this member name before?
        KeyValuePair *is_present = hash_table_search_string(name_ht, member->name);
        if (is_present) {
          name = is_present->value_string;
        } else {
          name = push_str8f(scratch.arena, "/%u", long_names_list.total_size);
          str8_list_pushf(scratch.arena, &long_names_list, "%S/\n", member->name);
          hash_table_push_string_string(scratch.arena, name_ht, member->name, name);
        }
      } else {
        name = push_str8f(scratch.arena, "%S/", member->name);
      }

      member_offsets[member_idx] = member_data_list.total_size;

      String8 member_data   = member->data;
      String8 member_header = coff_make_lib_member_header(arena, name, time_stamp, 0, 0, mode, member_data.size);

      str8_list_push(arena, &member_data_list, member_header);
      str8_list_push(arena, &member_data_list, member_data);
      {
        U64 pad_size = AlignPadPow2(member_data_list.total_size, COFF_Archive_MemberAlign);
        U8 *pad      = push_array(arena, U8, pad_size);
        str8_list_push(arena, &member_data_list, str8(pad, pad_size));
      }
    }
  }
  
  // long names member
  if (long_names_list.total_size) {
    String8 header        = coff_make_lib_member_header(arena, str8_lit("//"), time_stamp, 0, 0, mode, long_names_list.total_size);
    String8 data          = str8_list_join(arena, &long_names_list, 0);
    U64     member_offset = member_data_list.total_size + data.size + header.size;
    {
      U64 pad_size = AlignPadPow2(member_offset, COFF_Archive_MemberAlign);
      U8 *pad      = push_array(arena, U8, pad_size);
      str8_list_push_front(arena, &member_data_list, str8(pad, pad_size));
    }
    str8_list_push_front(arena, &member_data_list, data);
    str8_list_push_front(arena, &member_data_list, header);
  }
  
  // compute size for symbol string table
  U32 name_buffer_size = 0;
  for (COFF_LibWriterSymbol *ptr = &symbols[0], *opl = ptr + symbols_count; ptr < opl; ptr += 1) {
    name_buffer_size += ptr->name.size;
    name_buffer_size += 1; // null
  }

  // write symbol name buffer
  U8 *name_buffer = push_array_no_zero(scratch.arena, U8, name_buffer_size);
  {
    U64 name_cursor = 0;
    for (COFF_LibWriterSymbol *ptr = &symbols[0], *opl = ptr + symbols_count; ptr < opl; ptr += 1) {
      MemoryCopy(name_buffer + name_cursor, ptr->name.str, ptr->name.size);
      name_buffer[name_cursor + ptr->name.size] = '\0';
      name_cursor += ptr->name.size + 1;
    }
  }
  
  U64 members_base_offset;
  {
    U64 sizeof_first_header  = sizeof(COFF_ArchiveMemberHeader) + sizeof(U32) + sizeof(U32) * symbols_count + name_buffer_size;
    U64 sizeof_second_header = sizeof(COFF_ArchiveMemberHeader) + sizeof(U32) + sizeof(U32) * member_count + sizeof(U32) + sizeof(U16) * symbols_count + name_buffer_size;
    U64 sizeof_long_names    = sizeof(COFF_ArchiveMemberHeader) + long_names_list.total_size;
    
    sizeof_first_header  = AlignPow2(sizeof_first_header,  COFF_Archive_MemberAlign);
    sizeof_second_header = AlignPow2(sizeof_second_header, COFF_Archive_MemberAlign);
    sizeof_long_names    = AlignPow2(sizeof_long_names,    COFF_Archive_MemberAlign);

    members_base_offset = sizeof(g_coff_archive_sig);
    members_base_offset += sizeof_first_header;
    if (emit_second_member) {
      members_base_offset += sizeof_second_header;
    }
    if (long_names_list.total_size) {
      members_base_offset += sizeof_long_names;
    }
  }
  
  // second linker member
  if (emit_second_member) {
    U32 member_count32 = safe_cast_u32(member_count);
    U32 symbol_count32 = safe_cast_u32(symbols_count);

    U32 *member_off32_arr = push_array_no_zero(scratch.arena, U32, member_count);
    U16 *member_idx16_arr = push_array_no_zero(scratch.arena, U16, symbols_count);

    // write member offset array
    for (U64 member_idx = 0; member_idx < member_count; member_idx += 1) {
      U64 member_offset = members_base_offset + member_offsets[member_idx];
      U32 member_off32 = safe_cast_u32(member_offset);
      member_off32_arr[member_idx] = member_off32; 
    }

    // write member offset indices for each symbol
    for (U64 symbol_idx = 0; symbol_idx < symbols_count; symbol_idx += 1) {
      // member offset indices are 1-based
      U64 member_idx = symbols[symbol_idx].member_idx + 1;
      U16 member_idx16 = safe_cast_u16(member_idx);
      member_idx16_arr[symbol_idx] = member_idx16;
    }

    // layout second member data
    String8List second_member_data_list = {0};
    str8_list_push(scratch.arena, &second_member_data_list, str8_struct(&member_count32));
    str8_list_push(scratch.arena, &second_member_data_list, str8_array(member_off32_arr, member_count));
    str8_list_push(scratch.arena, &second_member_data_list, str8_struct(&symbol_count32));
    str8_list_push(scratch.arena, &second_member_data_list, str8_array(member_idx16_arr, symbols_count));
    str8_list_push(scratch.arena, &second_member_data_list, str8(name_buffer, name_buffer_size));

    String8 member_data   = str8_list_join(arena, &second_member_data_list, 0);
    String8 member_header = coff_make_lib_member_header(arena, str8_lit("/"), time_stamp, 0, 0, mode, member_data.size);
    
    U64 member_offset = member_data_list.total_size + member_data.size + member_header.size;
    {
      U64 pad_size = AlignPadPow2(member_offset, COFF_Archive_MemberAlign);
      U8 *pad      = push_array(arena, U8, pad_size);
      str8_list_push_front(arena, &member_data_list, str8(pad, pad_size));
    }
    str8_list_push_front(arena, &member_data_list, member_data);
    str8_list_push_front(arena, &member_data_list, member_header);
  }
  
  // first linker member (obsolete, but kept for compatability reasons)
  {
    U32  symbol_count_be  = from_be_u32(symbols_count);
    U32 *member_off32_arr = push_array_no_zero(scratch.arena, U32, symbols_count);

    for (U64 symbol_idx = 0; symbol_idx < symbols_count; symbol_idx += 1) {
      COFF_LibWriterSymbol *symbol = &symbols[symbol_idx];

      // write big endian member offset
      U64 member_offset = members_base_offset + member_offsets[symbol->member_idx];
      U32 member_off32 = from_be_u32(safe_cast_u32(member_offset));
      member_off32_arr[symbol_idx] = member_off32;
    }

    // layout first member data
    String8List first_member_data_list = {0};
    str8_list_push(scratch.arena, &first_member_data_list, str8_struct(&symbol_count_be));
    str8_list_push(scratch.arena, &first_member_data_list, str8_array(member_off32_arr, symbols_count));
    str8_list_push(scratch.arena, &first_member_data_list, str8(name_buffer, name_buffer_size));

    String8 member_data   = str8_list_join(arena, &first_member_data_list, 0);
    String8 member_header = coff_make_lib_member_header(arena, str8_lit("/"), time_stamp, 0, 0, mode, member_data.size);
    
    U64 member_offset = sizeof(g_coff_archive_sig) + member_header.size + member_data.size;
    {
      U64 pad_size = AlignPadPow2(member_offset, COFF_Archive_MemberAlign);
      U8 *pad      = push_array(arena, U8, pad_size);
      str8_list_push_front(arena, &member_data_list, str8(pad, pad_size));
    }
    str8_list_push_front(arena, &member_data_list, member_data);
    str8_list_push_front(arena, &member_data_list, member_header);
  }
  
  // archive signature
  str8_list_push_front(arena, &member_data_list, str8_struct(&g_coff_archive_sig));
  
  scratch_end(scratch);
  return member_data_list; 
}

