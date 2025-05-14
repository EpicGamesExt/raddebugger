// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_LibNode *
lnk_lib_list_reserve(Arena *arena, LNK_LibList *list, U64 count)
{
  LNK_LibNode *arr = 0;
  if (count) {
    arr = push_array(arena, LNK_LibNode, count);
    for (LNK_LibNode *ptr = arr, *opl = arr + count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += count;
  }
  return arr;
}

internal LNK_LibMemberNode *
lnk_lib_member_list_push(Arena *arena, LNK_LibMemberList *list, LNK_LibMember member)
{
  LNK_LibMemberNode *n = push_array_no_zero(arena, LNK_LibMemberNode, 1);
  n->next              = 0;
  n->data              = member;
  
  SLLQueuePush(list->first, list->last, n);
  ++list->count;
  
  return n;
}

internal LNK_LibMember *
lnk_lib_member_array_from_list(Arena *arena, LNK_LibMemberList list)
{
  ProfBeginFunction();
  LNK_LibMember *arr = push_array_no_zero(arena, LNK_LibMember, list.count);
  LNK_LibMember *ptr = arr;
  for (LNK_LibMemberNode *i = list.first; i != 0; i = i->next, ptr += 1) {
    ptr->name = push_str8_copy(arena, i->data.name);
    ptr->data = push_str8_copy(arena, i->data.data); 
  }
  ProfEnd();
  return arr;
}

internal LNK_LibSymbolNode *
lnk_lib_symbol_list_push(Arena *arena, LNK_LibSymbolList *list, LNK_LibSymbol symbol)
{
  LNK_LibSymbolNode *n = push_array_no_zero(arena, LNK_LibSymbolNode, 1);
  n->next              = 0;
  n->data              = symbol;
  
  SLLQueuePush(list->first, list->last, n);
  ++list->count;
  
  return n;
}

internal LNK_LibSymbol *
lnk_lib_symbol_array_from_list(Arena *arena, LNK_LibSymbolList list)
{
  LNK_LibSymbol *arr = push_array_no_zero(arena, LNK_LibSymbol, list.count + 2);
  LNK_LibSymbol *ptr = arr + 1;
  for (LNK_LibSymbolNode *i = list.first; i != 0; i = i->next, ptr += 1) {
    ptr->name = push_str8_copy(arena, i->data.name);
    ptr->member_idx = i->data.member_idx;
  }
  MemoryZeroStruct(&arr[0]);
  MemoryZeroStruct(&arr[list.count+1]);
  return arr;
}

int
lnk_lib_symbol_name_compar(const void *raw_a, const void *raw_b)
{
  const LNK_Symbol *sa = (const LNK_Symbol *)raw_a;
  const LNK_Symbol *sb = (const LNK_Symbol *)raw_b;
  return str8_compar_case_sensitive(&sa->name, &sb->name);
}

int
lnk_lib_symbol_name_compar_is_before(void *raw_a, void *raw_b)
{
  int compar = lnk_lib_symbol_name_compar(raw_a, raw_b);
  int is_before = compar < 0;
  return is_before;
}

internal void
lnk_lib_symbol_array_sort(LNK_LibSymbol *arr, U64 count)
{
  Assert(count >= 2);
  radsort(arr + 1, count - 2, lnk_lib_symbol_name_compar_is_before);
}

////////////////////////////////

internal LNK_Lib
lnk_lib_from_data(Arena *arena, String8 data, String8 path)
{
  ProfBeginFunction();

  U64     symbol_count;
  String8 string_table;
  U32    *member_off_arr;

  // is data archive?
  COFF_ArchiveType type = coff_archive_type_from_data(data);
  if (type == COFF_Archive_Null) {
    lnk_not_implemented("TODO: data is not archive");
  }

  COFF_ArchiveParse parse = coff_archive_parse_from_data(data);

  // report archive parser errors
  if (parse.error.size) {
    lnk_error(LNK_Error_IllData, "%S: %S", path, parse.error);
  }
  
  // try to init library from optional second member
  if (parse.second_member.member_count) {
    COFF_ArchiveSecondMember second_member = parse.second_member;
    Assert(second_member.symbol_count == second_member.symbol_index_count);
    Assert(second_member.member_count == second_member.member_offset_count);
    
    symbol_count   = second_member.symbol_count;
    string_table   = second_member.string_table;
    member_off_arr = push_array_no_zero(arena, U32, symbol_count);
    
    // decompress member offsets
    for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += 1) {
      U16 off_number = second_member.symbol_indices[symbol_idx];
      if (0 < off_number && off_number <= second_member.member_count) {
        member_off_arr[symbol_idx] = second_member.member_offsets[off_number - 1];
      } else {
        // TODO: log bad offset
        member_off_arr[symbol_idx] = max_U32;
      }
    }
  } 
  // first member is deprecated however tools emit it for compatibility reasons
  // and lld-link with /DLL emits only first member
  else if (parse.first_member.symbol_count) {
    COFF_ArchiveFirstMember first_member = parse.first_member;
    Assert(first_member.symbol_count == first_member.member_offset_count);
    
    symbol_count   = first_member.symbol_count;
    string_table   = first_member.string_table;
    member_off_arr = first_member.member_offsets;
    
    // convert big endian offsets
    for (U32 offset_idx = 0; offset_idx < symbol_count; offset_idx += 1) {
      member_off_arr[offset_idx] = from_be_u32(member_off_arr[offset_idx]);
    }
  } else {
    symbol_count   = 0;
    string_table   = str8_zero();
    member_off_arr = 0;
  }
  
  // parse string table
  String8List symbol_name_list = str8_split_by_string_chars(arena, string_table, str8_lit("\0"), StringSplitFlag_KeepEmpties);
  Assert(symbol_name_list.node_count >= symbol_count);
  symbol_count = Min(symbol_count, symbol_name_list.node_count);
  
  // init lib
  LNK_Lib lib = {0};
  lib.path             = push_str8_copy(arena, path);
  lib.data             = data;
  lib.type             = type;
  lib.symbol_count     = symbol_count;
  lib.member_off_arr   = member_off_arr;
  lib.symbol_name_list = symbol_name_list;
  lib.long_names       = parse.long_names;
  
  ProfEnd();
  return lib;
}

internal
THREAD_POOL_TASK_FUNC(lnk_lib_initer)
{
  LNK_LibIniter *task       = raw_task;
  LNK_LibNode   *lib_node   = task->node_arr + task_id;
  LNK_Lib       *lib        = &lib_node->data;
  String8        data       = task->data_arr[task_id];
  String8        path       = task->path_arr[task_id];
  
  *lib = lnk_lib_from_data(arena, data, path);
  lib->input_idx = task->base_input_idx + task_id;
}

internal LNK_LibNodeArray
lnk_lib_list_push_parallel(TP_Context *tp, TP_Arena *arena, LNK_LibList *list, String8Array data_arr, String8Array path_arr)
{
  Assert(data_arr.count == path_arr.count);
  U64 lib_count = data_arr.count;
  
  LNK_LibIniter task  = {0};
  task.node_arr       = lnk_lib_list_reserve(arena->v[0], list, lib_count);
  task.data_arr       = data_arr.v;
  task.path_arr       = path_arr.v;
  task.base_input_idx = list->count;
  tp_for_parallel(tp, arena, lib_count, lnk_lib_initer, &task);

  LNK_LibNodeArray arr = {0};
  arr.count            = lib_count;
  arr.v                = task.node_arr;
  return arr;
}

#if 0
internal LNK_LibNode *
lnk_lib_list_push(Arena *arena, LNK_LibList *list, String8 data, String8 path)
{
  ProfBeginFunction();
  
  TP_Arena pool_arena = {0};
  pool_arena.count    = 1;
  pool_arena.v        = &arena;
  
  String8Array data_arr = {0};
  data_arr.count        = 1;
  data_arr.v            = &data;
  
  String8Array path_arr = {0};
  path_arr.count        = 1;
  path_arr.v            = &path;
  
  LNK_LibNodeArray node_arr = lnk_lib_list_push_parallel(&pool_arena, list, data_arr, path_arr);

  ProfEnd();
  return node_arr.v;
}
#endif

////////////////////////////////

internal LNK_LibWriter *
lnk_lib_writer_alloc(void)
{
  Arena *arena = arena_alloc();
  LNK_LibWriter *writer = push_array(arena, LNK_LibWriter, 1);
  writer->arena = arena;
  return writer;
}

internal void
lnk_lib_writer_release(LNK_LibWriter **writer_ptr)
{
  arena_release((*writer_ptr)->arena);
  *writer_ptr = 0;
}

internal void
lnk_lib_writer_push_obj(LNK_LibWriter *writer, LNK_Obj *obj)
{
  ProfBeginFunction();
  
  U64 member_idx = writer->member_list.count;
  
  // push obj member
  LNK_LibMember member = {0};
  member.name          = obj->path;
  member.data          = obj->data;
  lnk_lib_member_list_push(writer->arena, &writer->member_list, member);
  
  // push external symbols
  for (LNK_SymbolNode *node = obj->symbol_list.first; node != 0; node = node->next) {
    LNK_Symbol *symbol = node->data;
    B32 is_extern = symbol->type == LNK_Symbol_DefinedExtern;
    if (is_extern) {
      LNK_LibSymbol lib_symbol = {0};
      lib_symbol.name       = symbol->name;
      lib_symbol.member_idx = member_idx;
      lnk_lib_symbol_list_push(writer->arena, &writer->symbol_list, lib_symbol);
    }
  }

  ProfEnd();
}

internal void
lnk_lib_writer_push_export(LNK_LibWriter *writer, COFF_MachineType machine, U64 time_stamp, String8 dll_name, LNK_Export *exp)
{
  ProfBeginFunction();
  
  U64 member_idx = writer->member_list.count;

  // make import header
  String8 import_data;
  if (exp->name.size) {
    U16 hint = safe_cast_u16(exp->id);
    import_data = coff_make_import_header_by_name(writer->arena, dll_name, machine, time_stamp, exp->name, hint, exp->type);
  } else {
    U16 ordinal = safe_cast_u16(exp->id);
    import_data = coff_make_import_header_by_ordinal(writer->arena, dll_name, machine, time_stamp, ordinal, exp->type);
  }
  
  // push import member
  LNK_LibMember member = {0};
  member.name          = dll_name;
  member.data          = import_data;
  lnk_lib_member_list_push(writer->arena, &writer->member_list, member);
  
  switch (exp->type) {
  case COFF_ImportHeader_Code: {
    LNK_LibSymbol def_symbol = {0};
    def_symbol.name          = push_str8_copy(writer->arena, exp->name);
    def_symbol.member_idx    = member_idx;
    lnk_lib_symbol_list_push(writer->arena, &writer->symbol_list, def_symbol);
  }
  case COFF_ImportHeader_Data: {
    LNK_LibSymbol imp_symbol = {0};
    imp_symbol.name          = push_str8f(writer->arena, "__imp_%S", exp->name);
    imp_symbol.member_idx    = member_idx;
    lnk_lib_symbol_list_push(writer->arena, &writer->symbol_list, imp_symbol);
  } break;
  case COFF_ImportHeader_Const: {
    NotImplemented;
  } break;
  default: InvalidPath;
  }

  ProfEnd();
}

internal LNK_LibBuild
lnk_lib_build_from_writer(Arena *arena, LNK_LibWriter *writer)
{
  ProfBeginFunction();
  
  LNK_LibBuild lib = {0};
  lib.symbol_count = writer->symbol_list.count + 2;
  lib.member_count = writer->member_list.count;
  lib.symbol_array = lnk_lib_symbol_array_from_list(arena, writer->symbol_list);
  lib.member_array = lnk_lib_member_array_from_list(arena, writer->member_list);
  lnk_lib_symbol_array_sort(lib.symbol_array, lib.symbol_count);

  ProfEnd();
  return lib;
}

internal String8List
lnk_coff_archive_from_lib_build(Arena *arena, LNK_LibBuild *lib, B32 emit_second_member, COFF_TimeStamp time_stamp, U32 mode)
{
  ProfBeginFunction();
  
  Temp scratch = scratch_begin(&arena, 1);

  U64            symbol_count = lib->symbol_count - 2;
  LNK_LibSymbol *symbol_arr   = lib->symbol_array + 1;

  HashTable   *name_ht          = hash_table_init(scratch.arena, 1024);
  U64         *member_off_arr   = push_array_no_zero(scratch.arena, U64, lib->member_count);
  String8List  long_names_list  = {0};
  String8List  member_data_list = {0};
  
  for (U64 member_idx = 0; member_idx < lib->member_count; member_idx += 1) {
    LNK_LibMember *member = &lib->member_array[member_idx];
    
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
    
    member_off_arr[member_idx] = member_data_list.total_size;
    
    String8 member_data = member->data;
    String8 member_header = lnk_build_lib_member_header(arena, name, time_stamp, 0, 0, mode, member_data.size);
    
    str8_list_push(arena, &member_data_list, member_header);
    str8_list_push(arena, &member_data_list, member_data);
    str8_list_push_pad(arena, &member_data_list, member_data_list.total_size, COFF_Archive_MemberAlign);
  }
  
  // long names member
  if (long_names_list.total_size > 0) {
    String8 header = lnk_build_lib_member_header(arena, str8_lit("//"), time_stamp, 0, 0, mode, long_names_list.total_size);
    String8 data = str8_list_join(arena, &long_names_list, 0);
    U64 member_offset = member_data_list.total_size + data.size + header.size;
    str8_list_push_pad_front(arena, &member_data_list, member_offset, COFF_Archive_MemberAlign);
    str8_list_push_front(arena, &member_data_list, data);
    str8_list_push_front(arena, &member_data_list, header);
  }
  
  // compute size for symbol string table
  U32 name_buffer_size = 0;
  for (LNK_LibSymbol *ptr = &symbol_arr[0], *opl = ptr + symbol_count; ptr < opl; ptr += 1) {
    name_buffer_size += ptr->name.size;
    name_buffer_size += 1; // null
  }

  // write symbol name buffer
  U8 *name_buffer = push_array_no_zero(scratch.arena, U8, name_buffer_size);
  {
    U64 name_cursor = 0;
    for (LNK_LibSymbol *ptr = &symbol_arr[0], *opl = ptr + symbol_count; ptr < opl; ptr += 1) {
      MemoryCopy(name_buffer + name_cursor, ptr->name.str, ptr->name.size);
      name_buffer[name_cursor + ptr->name.size] = '\0';
      name_cursor += ptr->name.size + 1;
    }
  }
  
  U64 member_base_off;
  {
    U64 sizeof_first_header  = sizeof(COFF_ArchiveMemberHeader) + sizeof(U32) + sizeof(U32) * symbol_count + name_buffer_size;
    U64 sizeof_second_header = sizeof(COFF_ArchiveMemberHeader) + sizeof(U32) + sizeof(U32) * lib->member_count + sizeof(U32) + sizeof(U16) * symbol_count + name_buffer_size;
    U64 sizeof_long_names    = sizeof(COFF_ArchiveMemberHeader) + long_names_list.total_size;
    
    sizeof_first_header  = AlignPow2(sizeof_first_header,  COFF_Archive_MemberAlign);
    sizeof_second_header = AlignPow2(sizeof_second_header, COFF_Archive_MemberAlign);
    sizeof_long_names    = AlignPow2(sizeof_long_names,    COFF_Archive_MemberAlign);

    member_base_off = sizeof(g_coff_archive_sig);
    member_base_off += sizeof_first_header;
    if (emit_second_member) {
      member_base_off += sizeof_second_header;
    }
    if (long_names_list.total_size > 0) {
      member_base_off += sizeof_long_names;
    }
  }
  
  // second linker member
  if (emit_second_member) {
    U32 member_count32 = safe_cast_u32(lib->member_count);
    U32 symbol_count32 = safe_cast_u32(symbol_count);

    U32 *member_off32_arr = push_array_no_zero(scratch.arena, U32, lib->member_count);
    U16 *member_idx16_arr = push_array_no_zero(scratch.arena, U16, symbol_count);

    // write member offset array
    for (U64 member_idx = 0; member_idx < lib->member_count; member_idx += 1) {
      U64 member_off = member_base_off + member_off_arr[member_idx];
      U32 member_off32 = safe_cast_u32(member_off);
      member_off32_arr[member_idx] = member_off32; 
    }

    // write member offset indices for each symbol
    for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += 1) {
      // member offset indices are 1-based
      U64 member_idx = symbol_arr[symbol_idx].member_idx + 1;
      U16 member_idx16 = safe_cast_u16(member_idx);
      member_idx16_arr[symbol_idx] = member_idx16;
    }

    // layout second member data
    String8List second_member_data_list = {0};
    str8_list_push(scratch.arena, &second_member_data_list, str8_struct(&member_count32));
    str8_list_push(scratch.arena, &second_member_data_list, str8_array(member_off32_arr, lib->member_count));
    str8_list_push(scratch.arena, &second_member_data_list, str8_struct(&symbol_count32));
    str8_list_push(scratch.arena, &second_member_data_list, str8_array(member_idx16_arr, symbol_count));
    str8_list_push(scratch.arena, &second_member_data_list, str8(name_buffer, name_buffer_size));

    String8 member_data   = str8_list_join(arena, &second_member_data_list, 0);
    String8 member_header = lnk_build_lib_member_header(arena, str8_lit("/"), time_stamp, 0, 0, mode, member_data.size);
    
    U64 member_offset = member_data_list.total_size + member_data.size + member_header.size;
    str8_list_push_pad_front(arena, &member_data_list, member_offset, COFF_Archive_MemberAlign);
    str8_list_push_front(arena, &member_data_list, member_data);
    str8_list_push_front(arena, &member_data_list, member_header);
  }
  
  // first linker member (obsolete, but kept for compatability reasons)
  {
    U32  symbol_count_be  = from_be_u32(symbol_count);
    U32 *member_off32_arr = push_array_no_zero(scratch.arena, U32, symbol_count);

    for (U64 symbol_idx = 0; symbol_idx < symbol_count; symbol_idx += 1) {
      LNK_LibSymbol *symbol = &symbol_arr[symbol_idx];

      // write big endian member offset
      U64 member_off = member_base_off + member_off_arr[symbol->member_idx];
      U32 member_off32 = from_be_u32(safe_cast_u32(member_off));
      member_off32_arr[symbol_idx] = member_off32;
    }

    // layout first member data
    String8List first_member_data_list = {0};
    str8_list_push(scratch.arena, &first_member_data_list, str8_struct(&symbol_count_be));
    str8_list_push(scratch.arena, &first_member_data_list, str8_array(member_off32_arr, symbol_count));
    str8_list_push(scratch.arena, &first_member_data_list, str8(name_buffer, name_buffer_size));

    String8 member_data   = str8_list_join(arena, &first_member_data_list, 0);
    String8 member_header = lnk_build_lib_member_header(arena, str8_lit("/"), time_stamp, 0, 0, mode, member_data.size);
    
    U64 member_offset = sizeof(g_coff_archive_sig) + member_header.size + member_data.size;
    str8_list_push_pad_front(arena, &member_data_list, member_offset, COFF_Archive_MemberAlign);
    str8_list_push_front(arena, &member_data_list, member_data);
    str8_list_push_front(arena, &member_data_list, member_header);
  }
  
  // archive signature
  str8_list_push_front(arena, &member_data_list, str8_struct(&g_coff_archive_sig));
  
  scratch_end(scratch);
  ProfEnd();
  return member_data_list; 
}

////////////////////////////////

internal LNK_LibBuild
lnk_build_lib(Arena *arena, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, LNK_ObjList obj_list, LNK_ExportTable *exptab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_LibWriter *writer = lnk_lib_writer_alloc();
  for (LNK_ObjNode *obj_node = obj_list.first; obj_node != 0; obj_node = obj_node->next) {
    lnk_lib_writer_push_obj(writer, &obj_node->data);
  }

  KeyValuePair *raw_export_arr = key_value_pairs_from_hash_table(scratch.arena, exptab->name_export_ht);
  for (U64 i = 0; i < exptab->name_export_ht->count; ++i) {
    LNK_Export *exp = raw_export_arr[i].value_raw;
    lnk_lib_writer_push_export(writer, machine, time_stamp, dll_name, exp);
  }
  LNK_LibBuild lib = lnk_lib_build_from_writer(arena, writer);
  lnk_lib_writer_release(&writer);

  scratch_end(scratch);
  ProfEnd();
  return lib;
}

internal String8List
lnk_build_import_entry_obj(Arena *arena, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();
  
  Assert(machine == COFF_MachineType_X64);
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));
  
  String8List list = {0};
  
  COFF_FileHeader *file_header = push_array(arena, COFF_FileHeader, 1);
  file_header->machine = machine;
  str8_list_push(arena, &list, str8_struct(file_header));
  
  file_header->section_count = 2;
  COFF_SectionHeader *coff_sect_header_array = push_array(arena, COFF_SectionHeader, file_header->section_count);
  str8_list_push(arena, &list, str8_array(coff_sect_header_array, file_header->section_count));
  
  PE_ImportEntry *import_entry = push_array(arena, PE_ImportEntry, 1);
  U64 import_entry_off = list.total_size;
  str8_list_push(arena, &list, str8_struct(import_entry));
  
  String8 dll_name_cstr = push_cstr(arena, dll_name);
  U64 dll_name_off = list.total_size;
  str8_list_push(arena, &list, dll_name_cstr);
  
  U32 import_entry_reloc_count = 3;
  COFF_Reloc *import_entry_reloc_array = push_array(arena, COFF_Reloc, import_entry_reloc_count);
  U64 import_entry_reloc_off = list.total_size;
  str8_list_push(arena, &list, str8_array(import_entry_reloc_array, import_entry_reloc_count));
  
  file_header->symbol_count = 7;
  COFF_Symbol16 *symbol_array = push_array(arena, COFF_Symbol16, file_header->symbol_count);
  file_header->symbol_table_foff = safe_cast_u32(list.total_size);
  str8_list_push(arena, &list, str8_array(symbol_array, file_header->symbol_count));
  
  U64 string_table_base = list.total_size;
  U32 *string_table_size_ptr = push_array(arena, U32, 1);
  str8_list_push(arena, &list, str8_struct(string_table_size_ptr));
  
  // PE_ImportEntry
  {
    COFF_SectionHeader *sect = &coff_sect_header_array[0];
    sect->name[0] = '.';
    sect->name[1] = 'i';
    sect->name[2] = 'd';
    sect->name[3] = 'a';
    sect->name[4] = 't';
    sect->name[5] = 'a';
    sect->name[6] = '$';
    sect->name[7] = '2';
    sect->fsize       = sizeof(PE_ImportEntry);
    sect->foff        = import_entry_off;
    sect->reloc_count = import_entry_reloc_count;
    sect->relocs_foff = import_entry_reloc_off;
    sect->flags       = COFF_SectionFlag_CntInitializedData|(COFF_SectionAlign_4Bytes << COFF_SectionFlag_AlignShift)|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  }
  {
    COFF_Reloc *lookup_table_voff_reloc = &import_entry_reloc_array[0];
    lookup_table_voff_reloc->apply_off  = OffsetOf(PE_ImportEntry, lookup_table_voff);
    lookup_table_voff_reloc->isymbol    = 3;
    lookup_table_voff_reloc->type       = COFF_Reloc_X64_Addr32Nb;
    
    COFF_Reloc *name_voff_reloc = &import_entry_reloc_array[1];
    name_voff_reloc->apply_off  = OffsetOf(PE_ImportEntry, name_voff);
    name_voff_reloc->isymbol    = 2;
    name_voff_reloc->type       = COFF_Reloc_X64_Addr32Nb;
    
    COFF_Reloc *import_addr_table_voff = &import_entry_reloc_array[2];
    import_addr_table_voff->apply_off  = OffsetOf(PE_ImportEntry, import_addr_table_voff);
    import_addr_table_voff->isymbol    = 4;
    import_addr_table_voff->type       = COFF_Reloc_X64_Addr32Nb;
  }
  
  // dll name
  {
    COFF_SectionHeader *sect = &coff_sect_header_array[1];
    sect->name[0] = '.';
    sect->name[1] = 'i';
    sect->name[2] = 'd';
    sect->name[3] = 'a';
    sect->name[4] = 't';
    sect->name[5] = 'a';
    sect->name[6] = '$';
    sect->name[7] = '6';
    sect->fsize = dll_name_cstr.size;
    sect->foff  = dll_name_off;
    sect->flags = COFF_SectionFlag_CntInitializedData|(COFF_SectionAlign_2Bytes << COFF_SectionFlag_AlignShift)|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  }
  
  // import descriptor
  {
    String8 dll_name_no_ext = str8_substr(dll_name, r1u64(0, dll_name.size - 4));
    String8 symbol_name = push_str8f(arena, "__IMPORT_DESCRIPTOR_%S", dll_name_no_ext);
    
    U64 symbol_name_off = (list.total_size - string_table_base);
    str8_list_push(arena, &list, push_cstr(arena, symbol_name));
    
    COFF_Symbol16 *symbol                      = &symbol_array[0];
    symbol->name.long_name.zeroes              = 0;
    symbol->name.long_name.string_table_offset = symbol_name_off;
    symbol->section_number                     = 1;
    symbol->storage_class                      = COFF_SymStorageClass_External;
  }
  
  // .idata$2
  {
    COFF_Symbol16 *symbol = &symbol_array[1];
    symbol->name.short_name[0] = '.';
    symbol->name.short_name[1] = 'i';
    symbol->name.short_name[2] = 'd';
    symbol->name.short_name[3] = 'a';
    symbol->name.short_name[4] = 't';
    symbol->name.short_name[5] = 'a';
    symbol->name.short_name[6] = '$';
    symbol->name.short_name[7] = '2';
    symbol->section_number = 1;
    symbol->storage_class  = COFF_SymStorageClass_Section;
  }
  
  // .idata$6
  {
    COFF_Symbol16 *symbol = &symbol_array[2];
    symbol->name.short_name[0] = '.';
    symbol->name.short_name[1] = 'i';
    symbol->name.short_name[2] = 'd';
    symbol->name.short_name[3] = 'a';
    symbol->name.short_name[4] = 't';
    symbol->name.short_name[5] = 'a';
    symbol->name.short_name[6] = '$';
    symbol->name.short_name[7] = '6';
    symbol->section_number = 2;
    symbol->storage_class  = COFF_SymStorageClass_Static;
  }
  
  // .idata$4
  {
    COFF_Symbol16 *symbol = &symbol_array[3];
    symbol->name.short_name[0] = '.';
    symbol->name.short_name[1] = 'i';
    symbol->name.short_name[2] = 'd';
    symbol->name.short_name[3] = 'a';
    symbol->name.short_name[4] = 't';
    symbol->name.short_name[5] = 'a';
    symbol->name.short_name[6] = '$';
    symbol->name.short_name[7] = '4';
    symbol->section_number = COFF_Symbol_UndefinedSection;
    symbol->storage_class  = COFF_SymStorageClass_Section;
  }
  
  // .idata$5
  {
    COFF_Symbol16 *symbol = &symbol_array[4];
    symbol->name.short_name[0] = '.';
    symbol->name.short_name[1] = 'i';
    symbol->name.short_name[2] = 'd';
    symbol->name.short_name[3] = 'a';
    symbol->name.short_name[4] = 't';
    symbol->name.short_name[5] = 'a';
    symbol->name.short_name[6] = '$';
    symbol->name.short_name[7] = '5';
    symbol->section_number = COFF_Symbol_UndefinedSection;
    symbol->storage_class  = COFF_SymStorageClass_Section;
  }
  
  // __NULL_IMPORT_DESCRIPTOR
  {
    U64 symbol_name_off = (list.total_size - string_table_base);
    str8_list_push(arena, &list, push_cstr(arena, str8_lit("__NULL_IMPORT_DESCRIPTOR")));
    
    COFF_Symbol16 *symbol                      = &symbol_array[5];
    symbol->name.long_name.zeroes              = 0;
    symbol->name.long_name.string_table_offset = symbol_name_off;
    symbol->section_number                     = COFF_Symbol_UndefinedSection;
    symbol->storage_class                      = COFF_SymStorageClass_External;
  }
  
  // NULL_THUNK_DATA
  {
    String8 dll_name_no_ext = str8_substr(dll_name, r1u64(0, dll_name.size - 4));
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    
    U64 symbol_name_off = (list.total_size - string_table_base);
    str8_list_push(arena, &list, push_cstr(arena, symbol_name));
    
    COFF_Symbol16 *symbol                      = &symbol_array[6];
    symbol->name.long_name.zeroes              = 0;
    symbol->name.long_name.string_table_offset = symbol_name_off;
    symbol->section_number                     = COFF_Symbol_UndefinedSection;
    symbol->storage_class                      = COFF_SymStorageClass_External;
  }
  
  // update string table size
  *string_table_size_ptr = (list.total_size - string_table_base);
  
  ProfEnd();
  return list;
}

internal String8List
lnk_build_null_import_descriptor_obj(Arena *arena, COFF_MachineType machine)
{
  ProfBeginFunction();
  
  String8List list = {0};
  
  COFF_FileHeader *coff_header = push_array(arena, COFF_FileHeader, 1);
  coff_header->machine = machine;
  str8_list_push(arena, &list, str8_struct(coff_header));
  
  coff_header->section_count = 1;
  COFF_SectionHeader *coff_sect_header_array = push_array(arena, COFF_SectionHeader, coff_header->section_count);
  str8_list_push(arena, &list, str8_array(coff_sect_header_array, coff_header->section_count));
  
  U64 null_import_data_size = 20;
  U8 *null_import_data      = push_array(arena, U8, null_import_data_size);
  U64 null_import_data_off  = list.total_size;
  str8_list_push(arena, &list, str8(null_import_data, null_import_data_size));
  
  coff_header->symbol_count = 1;
  COFF_Symbol16 *symbol_array = push_array(arena, COFF_Symbol16, coff_header->symbol_count);
  coff_header->symbol_table_foff = safe_cast_u32(list.total_size);
  str8_list_push(arena, &list, str8_array(symbol_array, coff_header->symbol_count));
  
  U64  string_table_base     = list.total_size;
  U32 *string_table_size_ptr = push_array(arena, U32, 1);
  str8_list_push(arena, &list, str8_struct(string_table_size_ptr));
  
  {
    COFF_SectionHeader *sect = &coff_sect_header_array[0];
    sect->name[0] = '.';
    sect->name[1] = 'i';
    sect->name[2] = 'd';
    sect->name[3] = 'a';
    sect->name[4] = 't';
    sect->name[5] = 'a';
    sect->name[6] = '$';
    sect->name[7] = '3';
    sect->fsize = null_import_data_size;
    sect->foff  = null_import_data_off;
    sect->flags = COFF_SectionFlag_CntInitializedData|(COFF_SectionAlign_4Bytes << COFF_SectionFlag_AlignShift)|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  }
  
  {
    U64 symbol_name_off = list.total_size - string_table_base;
    str8_list_push(arena, &list, push_cstr(arena, str8_lit("__NULL_IMPORT_DESCRIPTOR")));
    
    COFF_Symbol16 *symbol                      = &symbol_array[0];
    symbol->name.long_name.zeroes              = 0;
    symbol->name.long_name.string_table_offset = symbol_name_off;
    symbol->section_number                     = 1;
    symbol->storage_class                      = COFF_SymStorageClass_External;
  }
  
  // update string table size
  *string_table_size_ptr = (list.total_size - string_table_base);
  
  ProfEnd();
  return list;
}

internal String8List
lnk_build_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();
  
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));
  
  String8List list = {0};
  
  COFF_FileHeader *coff_header = push_array(arena, COFF_FileHeader, 1);
  coff_header->machine = machine;
  str8_list_push(arena, &list, str8_struct(coff_header));
  
  coff_header->section_count = 2;
  COFF_SectionHeader *coff_sect_header_array = push_array(arena, COFF_SectionHeader, coff_header->section_count);
  str8_list_push(arena, &list, str8_array(coff_sect_header_array, coff_header->section_count));
  
  U64 lookup_entry_data_size = 8;
  U8 *lookup_entry_data      = push_array(arena, U8, lookup_entry_data_size);
  U64 lookup_entry_data_off  = list.total_size;
  str8_list_push(arena, &list, str8(lookup_entry_data, lookup_entry_data_size));
  
  U64 null_thunk_data_size = 8;
  U8 *null_thunk_data      = push_array(arena, U8, null_thunk_data_size);
  U64 null_thunk_data_off  = list.total_size;
  str8_list_push(arena, &list, str8(null_thunk_data, null_thunk_data_size));
  
  coff_header->symbol_count = 1;
  COFF_Symbol16 *symbol_array = push_array(arena, COFF_Symbol16, coff_header->symbol_count);
  coff_header->symbol_table_foff = safe_cast_u32(list.total_size);
  str8_list_push(arena, &list, str8_array(symbol_array, coff_header->symbol_count));
  
  U64 string_table_base = list.total_size;
  U32 *string_table_size_ptr = push_array(arena, U32, 1);
  str8_list_push(arena, &list, str8_struct(string_table_size_ptr));
  
  {
    COFF_SectionHeader *sect = &coff_sect_header_array[0];
    sect->name[0] = '.';
    sect->name[1] = 'i';
    sect->name[2] = 'd';
    sect->name[3] = 'a';
    sect->name[4] = 't';
    sect->name[5] = 'a';
    sect->name[6] = '$';
    sect->name[7] = '5';
    sect->fsize = lookup_entry_data_size;
    sect->foff  = lookup_entry_data_off;
    sect->flags = COFF_SectionFlag_CntInitializedData | (COFF_SectionAlign_8Bytes << COFF_SectionFlag_AlignShift)|(COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite);
  }
  
  {
    COFF_SectionHeader *sect = &coff_sect_header_array[1];
    sect->name[0] = '.';
    sect->name[1] = 'i';
    sect->name[2] = 'd';
    sect->name[3] = 'a';
    sect->name[4] = 't';
    sect->name[5] = 'a';
    sect->name[6] = '$';
    sect->name[7] = '4';
    sect->fsize = null_thunk_data_size;
    sect->foff  = null_thunk_data_off;
    sect->flags = COFF_SectionFlag_CntInitializedData|(COFF_SectionAlign_8Bytes << COFF_SectionFlag_AlignShift)|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  }
  
  {
    String8 dll_name_no_ext = str8_substr(dll_name, r1u64(0, dll_name.size - 4));
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    
    U64 symbol_name_off = list.total_size - string_table_base;
    str8_list_push(arena, &list, push_cstr(arena, symbol_name));
    
    COFF_Symbol16 *symbol                      = &symbol_array[0];
    symbol->name.long_name.zeroes              = 0;
    symbol->name.long_name.string_table_offset = symbol_name_off;
    symbol->section_number                     = 1;
    symbol->storage_class                      = COFF_SymStorageClass_External;
  }
  
  // update string table size
  *string_table_size_ptr = (list.total_size - string_table_base);
  
  ProfEnd();
  return list;
}

internal String8
lnk_build_lib_member_header(Arena *arena, String8 name, COFF_TimeStamp time_stamp, U16 user_id, U16 group_id, U16 mode, U32 size)
{
  ProfBeginFunction();
  
  Assert(name.size < 16);
  Assert(user_id < 10000);
  Assert(group_id < 10000);
  Assert(mode < 10000);
  Assert(size < 1000000000);
  
  //U64 sizeof_member_header = /* name */ 16 + /* time */ 12 + /* user_id */ 6 + /* group id */ 6 + /* mode */ 8 + /* size */ 10;
  
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  str8_list_pushf(scratch.arena, &list, "%-16.*s", str8_varg(name));
  str8_list_pushf(scratch.arena, &list, "%-12u", time_stamp);
  str8_list_pushf(scratch.arena, &list, "%-6u", user_id);
  str8_list_pushf(scratch.arena, &list, "%-6u", group_id);
  str8_list_pushf(scratch.arena, &list, "%-8u", mode);
  str8_list_pushf(scratch.arena, &list, "%-10u", size);
  str8_list_pushf(scratch.arena, &list, "`\n");
  String8 result = str8_list_join(arena, &list, 0);

  Assert(result.size == sizeof(COFF_ArchiveMemberHeader));
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal String8List
lnk_build_import_lib(TP_Context *tp, TP_Arena *arena, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 lib_name, String8 dll_name, LNK_ExportTable *exptab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  dll_name = str8_skip_last_slash(dll_name);
  
  // These objects appear in first three members of any lib that linker produces with /dll.
  // Objects are used by MSVC linker to build import table.
  String8List import_obj_array[3];
  import_obj_array[0] = lnk_build_import_entry_obj(scratch.arena, dll_name, machine);
  import_obj_array[1] = lnk_build_null_import_descriptor_obj(scratch.arena, machine);
  import_obj_array[2] = lnk_build_null_thunk_data_obj(scratch.arena, dll_name, machine);

  // build input list
  LNK_InputObjList input_obj_list = {0};
  for (U64 i = 0; i < ArrayCount(import_obj_array); ++i) {
    LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
    input->data         = str8_list_join(scratch.arena, &import_obj_array[i], 0);
    input->path         = dll_name;
    input->lib_path     = lib_name;
  }

  LNK_InputObj     **inputs   = lnk_array_from_input_obj_list(scratch.arena, input_obj_list);
  LNK_SectionTable  *sectab       = lnk_section_table_alloc(0,0,0);
  LNK_ObjList        obj_list = {0};
  lnk_obj_list_push_parallel(tp, arena, &obj_list, sectab, 0, input_obj_list.count, inputs);
  
  LNK_LibBuild import_lib = lnk_build_lib(scratch.arena, machine, time_stamp, dll_name, obj_list, exptab);
  B32 emit_second_member = 0; // MSVC linker refuses to link with lib that has the second member.
  String8List coff_archive_data = lnk_coff_archive_from_lib_build(arena->v[0], &import_lib, emit_second_member, time_stamp, /* -rw-r--r-- */ 644);
  
  // cleanup memory
  lnk_section_table_release(&sectab);
  scratch_end(scratch);

  ProfEnd();
  return coff_archive_data;
}

