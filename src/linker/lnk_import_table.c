// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_ImportTable *
lnk_import_table_alloc(LNK_ImportTableFlags flags)
{
  ProfBeginFunction();

  Arena *arena = arena_alloc();
  LNK_ImportTable *imptab = push_array(arena, LNK_ImportTable, 1);
  imptab->arena           = arena;
  imptab->flags           = flags;
  imptab->dll_ht          = hash_table_init(arena, LNK_IMPORT_DLL_HASH_TABLE_BUCKET_COUNT);

  ProfEnd();
  return imptab;
}

internal void
lnk_import_table_release(LNK_ImportTable **imptab_ptr)
{
  ProfBeginFunction();
  arena_release((*imptab_ptr)->arena);
  *imptab_ptr = 0;
  ProfEnd();
}

internal BucketNode *
lnk_import_table_push_dll_node(LNK_ImportTable *imptab, LNK_ImportDLL *dll)
{
  // update list
  SLLQueuePush(imptab->first_dll, imptab->last_dll, dll);

  // update name -> dll hash table
  return hash_table_push_path_raw(imptab->arena, imptab->dll_ht, dll->name, dll);
}

internal BucketNode *
lnk_import_table_push_func_node(LNK_ImportTable *imptab, LNK_ImportDLL *dll, LNK_ImportFunc *func)
{
  // update list
  SLLQueuePush(dll->first_func, dll->last_func, func);

  // update name -> func hash table
  return hash_table_push_string_raw(imptab->arena, dll->func_ht, func->name, func);
}

internal LNK_ImportDLL *
lnk_import_table_search_dll(LNK_ImportTable *imptab, String8 name)
{
  return hash_table_search_path_raw(imptab->dll_ht, name);
}

internal LNK_ImportFunc *
lnk_import_table_search_func(LNK_ImportDLL *dll, String8 name)
{
  LNK_ImportFunc *func = 0;
  hash_table_search_string_raw(dll->func_ht, name, &func);
  return func;
}

internal LNK_ImportDLL *
lnk_import_table_push_dll_static(LNK_ImportTable *imptab, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  U64               import_size  = coff_word_size_from_machine(machine);
  COFF_SectionFlags import_align = coff_section_flag_from_align_size(import_size);

  PE_ImportEntry *impdesc       = push_array(obj_writer->arena, PE_ImportEntry, 1);
  String8         dll_name_cstr = push_cstr(obj_writer->arena, dll_name);

  COFF_ObjSection *dll_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$2"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_struct(impdesc));
  COFF_ObjSection *ilt_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$4"), LNK_IDATA_SECTION_FLAGS|import_align,                  str8_zero());
  COFF_ObjSection *iat_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$5"), LNK_IDATA_SECTION_FLAGS|import_align,                  str8_zero());
  COFF_ObjSection *int_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$6"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes,  str8_zero());
  COFF_ObjSection *dll_name_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$7"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes,  dll_name_cstr);
  COFF_ObjSection *code_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".text$i"),  LNK_TEXT_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes,   str8_zero());

  COFF_ObjSymbol *ilt_symbol      = coff_obj_writer_push_symbol_static(obj_writer, ilt_sect->name,      0, ilt_sect);
  COFF_ObjSymbol *iat_symbol      = coff_obj_writer_push_symbol_static(obj_writer, iat_sect->name,      0, iat_sect);
  COFF_ObjSymbol *dll_name_symbol = coff_obj_writer_push_symbol_static(obj_writer, dll_name_sect->name, 0, dll_name_sect);
  
  switch (machine) {
  case COFF_MachineType_Unknown: {} break;
  case COFF_MachineType_X64: {
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, lookup_table_voff),      ilt_symbol,      COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, name_voff),              dll_name_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, import_addr_table_voff), iat_symbol,      COFF_Reloc_X64_Addr32Nb);
  } break;
  default: { NotImplemented; } break;
  }
  
  // push to list
  LNK_ImportDLL *dll = push_array(imptab->arena, LNK_ImportDLL, 1);
  dll->obj_writer    = obj_writer;
  dll->name          = push_str8_copy(imptab->arena, dll_name);
  dll->dll_sect      = dll_sect;
  dll->int_sect      = int_sect;
  dll->ilt_sect      = ilt_sect;
  dll->iat_sect      = iat_sect;
  dll->code_sect     = code_sect;
  dll->func_ht       = hash_table_init(imptab->arena, LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT);

  lnk_import_table_push_dll_node(imptab, dll);
  
  ProfEnd();
  return dll;
}

internal LNK_ImportDLL *
lnk_import_table_push_dll_delayed(LNK_ImportTable *imptab, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, machine);

  // import descriptor
  PE_DelayedImportEntry *impdesc = push_array(obj_writer->arena, PE_DelayedImportEntry, 1);
  impdesc->attributes            = 1;

  // DLL name cstring
  String8 dll_name_cstr = push_cstr(obj_writer->arena, dll_name);

  // DLL handle
  U64 handle_size = coff_word_size_from_machine(machine);
  U8 *handle      = push_array(obj_writer->arena, U8, handle_size);

  // import align
  U64               import_size  = coff_word_size_from_machine(machine);
  COFF_SectionFlags import_align = coff_section_flag_from_align_size(import_size);

  COFF_ObjSection *dll_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$2"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_struct(impdesc));
  COFF_ObjSection *ilt_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$4"), LNK_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *iat_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$5"), LNK_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *int_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$6"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, str8_zero());
  COFF_ObjSection *dll_name_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$7"), LNK_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, dll_name_cstr);
  COFF_ObjSection *biat_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$8"), LNK_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *uiat_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$9"), LNK_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *code_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".text$i"),  LNK_TEXT_SECTION_FLAGS,                               str8_zero());
  COFF_ObjSection *handle_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".data$h"),  LNK_DATA_SECTION_FLAGS,                               str8_array(handle, handle_size));

  COFF_ObjSymbol *dll_symbol      = coff_obj_writer_push_symbol_static(obj_writer, dll_sect->name,      0, dll_sect);
  COFF_ObjSymbol *dll_name_symbol = coff_obj_writer_push_symbol_static(obj_writer, dll_name_sect->name, 0, dll_name_sect);
  COFF_ObjSymbol *handle_symbol   = coff_obj_writer_push_symbol_static(obj_writer, handle_sect->name,   0, handle_sect);
  COFF_ObjSymbol *iat_symbol      = coff_obj_writer_push_symbol_static(obj_writer, iat_sect->name,      0, iat_sect);
  COFF_ObjSymbol *ilt_symbol      = coff_obj_writer_push_symbol_static(obj_writer, ilt_sect->name,      0, ilt_sect);

  switch (machine) {
  case COFF_MachineType_Unknown: {} break;
  case COFF_MachineType_X64: {
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, name_voff),          dll_name_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, module_handle_voff), handle_symbol,   COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, iat_voff),           iat_symbol,      COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, name_table_voff),    ilt_symbol,      COFF_Reloc_X64_Addr32Nb);
  } break;
  default: { NotImplemented; } break;
  }

  if (imptab->flags & LNK_ImportTableFlag_EmitBiat) {
    COFF_ObjSymbol *biat_symbol = coff_obj_writer_push_symbol_static(obj_writer, biat_sect->name, 0, biat_sect);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, bound_table_voff), biat_symbol, COFF_Reloc_X64_Addr32Nb);
  }
  if (imptab->flags & LNK_ImportTableFlag_EmitUiat) {
    COFF_ObjSymbol *uiat_symbol = coff_obj_writer_push_symbol_static(obj_writer, uiat_sect->name, 0, uiat_sect);
    coff_obj_writer_section_push_reloc(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, unload_table_voff), uiat_symbol, COFF_Reloc_X64_Addr32Nb);
  }
  
  // emit tail merge
  COFF_ObjSymbol *tail_merge_symbol = 0;
  switch (machine) {
  case COFF_MachineType_Unknown: {} break;
  case COFF_MachineType_X64:     { tail_merge_symbol = lnk_emit_tail_merge_thunk_x64(obj_writer, code_sect, dll_name, dll_symbol); } break;
  default:                       { NotImplemented; } break;
  }
  
  // fill out result
  LNK_ImportDLL *dll     = push_array(imptab->arena, LNK_ImportDLL, 1);
  dll->dll_sect          = dll_sect;
  dll->int_sect          = int_sect;
  dll->iat_sect          = iat_sect;
  dll->ilt_sect          = ilt_sect;
  dll->biat_sect         = biat_sect;
  dll->uiat_sect         = uiat_sect;
  dll->code_sect         = code_sect;
  dll->tail_merge_symbol = tail_merge_symbol;
  dll->name              = push_str8_copy(imptab->arena, dll_name);
  dll->func_ht           = hash_table_init(imptab->arena, LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT);

  lnk_import_table_push_dll_node(imptab, dll);
  
  ProfEnd();
  return dll;
}

internal LNK_ImportFunc *
lnk_import_table_push_func_static(LNK_ImportTable *imptab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header)
{
  ProfBeginFunction();

  COFF_ObjWriter *obj_writer = dll->obj_writer;
  
  String8 iat_symbol_name = push_str8f(obj_writer->arena, "__imp_%S", header->func_name);
  U64     iat_offset      = dll->iat_sect->data.total_size;
  U64     ilt_offset      = dll->ilt_sect->data.total_size;
  U64     int_offset      = dll->int_sect->data.total_size;

  COFF_ObjSymbol *iat_symbol = 0;
  switch (header->import_by) {
  case COFF_ImportBy_Ordinal: {
    String8 ordinal_data = coff_ordinal_data_from_hint(obj_writer->arena, header->machine, header->hint_or_ordinal);
    str8_list_push(obj_writer->arena, &dll->ilt_sect->data, ordinal_data);
    str8_list_push(obj_writer->arena, &dll->iat_sect->data, ordinal_data);

    iat_symbol = coff_obj_writer_push_symbol_extern(obj_writer, iat_symbol_name, iat_offset, dll->iat_sect);
  } break;
  case COFF_ImportBy_Name: {
    // put together name look up entry
    String8 int_data = coff_make_import_lookup(obj_writer->arena, header->hint_or_ordinal, header->func_name);
    str8_list_push(obj_writer->arena, &dll->int_sect->data, int_data);

    // create symbol for lookup entry
    COFF_ObjSymbol *int_symbol = coff_obj_writer_push_symbol_static(obj_writer, dll->int_sect->name, int_offset, dll->int_sect);

    // in the file IAT mirrors ILT, dynamic linker later overwrites it with imported function addresses
    U64 import_size  = coff_word_size_from_machine(header->machine);
    U8 *import_entry = push_array(obj_writer->arena, U8, import_size);
    str8_list_push(obj_writer->arena, &dll->ilt_sect->data, str8_array(import_entry, import_size));
    str8_list_push(obj_writer->arena, &dll->iat_sect->data, str8_array(import_entry, import_size));

    iat_symbol = coff_obj_writer_push_symbol_extern(obj_writer, iat_symbol_name, iat_offset, dll->iat_sect);
    
    // patch IAT and ILT
    coff_obj_writer_section_push_reloc(obj_writer, dll->ilt_sect, ilt_offset, int_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, dll->iat_sect, iat_offset, int_symbol, COFF_Reloc_X64_Addr32Nb);
  } break;
  case COFF_ImportBy_Undecorate: {
    NotImplemented;
  } break;
  case COFF_ImportBy_NameNoPrefix: {
    NotImplemented;
  } break;
  }

  // generate thunks
  COFF_ObjSymbol *jmp_thunk_symbol = 0;
  if (header->type == COFF_ImportHeader_Code) {
    switch (header->machine) {
    case COFF_MachineType_Unknown: {} break;
    case COFF_MachineType_X64:     { jmp_thunk_symbol = lnk_emit_indirect_jump_thunk_x64(obj_writer, dll->code_sect, iat_symbol, header->func_name); } break;
    default:                       { NotImplemented; } break;
    }
  }
  
  // fill out import
  LNK_ImportFunc *func    = push_array(imptab->arena, LNK_ImportFunc, 1);
  func->name              = push_str8_copy(imptab->arena, header->func_name);
  func->thunk_symbol_name = push_str8_copy(imptab->arena, jmp_thunk_symbol->name);
  func->iat_symbol_name   = iat_symbol_name;

  lnk_import_table_push_func_node(imptab, dll, func);
  
  ProfEnd();
  return func;
}

internal LNK_ImportFunc *
lnk_import_table_push_func_delayed(LNK_ImportTable *imptab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header)
{
#if 0
  ProfBeginFunction();
  
  COFF_ObjWriter *obj_writer = dll->obj_writer;
  Assert(dll->machine == header->machine); // TODO: error handle

  U64 import_size = coff_word_size_from_machine(dll->machine);
  
  LNK_Symbol *int_symbol = 0;
  
  // generate thunks
  LNK_Symbol *jmp_thunk_symbol  = g_null_symbol_ptr;
  LNK_Symbol *load_thunk_symbol = g_null_symbol_ptr;
  LNK_Chunk  *jmp_thunk_chunk   = 0;
  LNK_Chunk  *load_thunk_chunk  = 0;
  if (header->type == COFF_ImportHeader_Code) {
    switch (dll->machine) {
    case COFF_MachineType_X64: {
      String8     iat_symbol_name = push_str8f(symtab->arena->v[0], "__imp_%S", header->func_name);
      LNK_Symbol *iat_symbol      = lnk_make_undefined_symbol(symtab->arena->v[0], iat_symbol_name, LNK_SymbolScopeFlag_Main);
      
      // emit jmp thunk chunk
      jmp_thunk_chunk  = lnk_emit_indirect_jump_thunk_x64(code_sect, code_table_chunk, iat_symbol);
      jmp_thunk_symbol = lnk_emit_jmp_thunk_symbol(symtab, jmp_thunk_chunk, header->func_name);
      
      // emit load thunk
      load_thunk_chunk  = lnk_emit_load_thunk_x64(code_sect, code_table_chunk, iat_symbol, dll->tail_merge_symbol);
      load_thunk_symbol = lnk_emit_load_thunk_symbol(symtab, load_thunk_chunk, header->func_name);
    } break;
    default: lnk_not_implemented("TODO: support for machine 0x%X", dll->machine); break;
    }
  }
  
  switch (header->import_by) {
  case COFF_ImportBy_Ordinal: {
    String8 ordinal_data = lnk_ordinal_data_from_hint(data_sect->arena, dll->machine, header->hint_or_ordinal);
    Assert(ordinal_data.size == import_size);
    ilt_chunk = lnk_section_push_chunk_data(data_sect, ilt_table_chunk, ordinal_data, sort_index);
    iat_chunk = lnk_section_push_chunk_bss(data_sect, iat_table_chunk, import_size, sort_index);
    lnk_section_push_reloc(data_sect, iat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);

    lnk_section_associate_chunks(data_sect, iat_chunk, ilt_chunk);
    if (imptab->flags & LNK_ImportTableFlag_EmitBiat) {
      biat_chunk = lnk_section_push_chunk_bss(data_sect, biat_table_chunk, import_size, sort_index);
      lnk_section_push_reloc(data_sect, biat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);
      lnk_section_associate_chunks(data_sect, iat_chunk, biat_chunk);
    }
    if (imptab->flags & LNK_ImportTableFlag_EmitUiat) {
      uiat_chunk = lnk_section_push_chunk_bss(data_sect, uiat_table_chunk, import_size, sort_index);
      lnk_section_push_reloc(data_sect, uiat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);
      lnk_section_associate_chunks(data_sect, iat_chunk, uiat_chunk);
    }
  } break;
  case COFF_ImportBy_Name: {
    // put together name look up entry
    String8    int_data  = coff_make_import_lookup(data_sect->arena, header->hint_or_ordinal, header->func_name);
    LNK_Chunk *int_chunk = lnk_section_push_chunk_data(data_sect, int_table_chunk, int_data, str8_zero());
    
    // create symbol for lookup chunk
    String8 int_symbol_name = push_str8f(symtab->arena->v[0], "%S.%S.name.delayed", dll->name, header->func_name);
    int_symbol = lnk_symbol_table_push_defined_chunk(symtab, int_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, int_chunk, 0, 0, 0);
    
    // dynamic linker patches this voff on DLL load event
    ilt_chunk = lnk_section_push_chunk_bss(data_sect, ilt_table_chunk, import_size, sort_index);
    lnk_chunk_set_debugf(data_sect->arena, ilt_chunk, "ILT entry (delayed) %S.%S", dll->name, header->func_name);
    
    // patch-in ILT with import voff
    lnk_section_push_reloc(data_sect, ilt_chunk, LNK_Reloc_VIRT_OFF_32, 0, int_symbol);
    
    // in the file IAT mirrors ILT, dynamic linker later overwrites it with imported function addresses.
    iat_chunk = lnk_section_push_chunk_bss(data_sect, iat_table_chunk, import_size, sort_index);
    lnk_chunk_set_debugf(data_sect->arena, iat_chunk, "IAT entre (delayed) %S.%S", dll->name, header->func_name);

    // associate chunks
    lnk_section_associate_chunks(data_sect, iat_chunk, ilt_chunk);
    lnk_section_associate_chunks(data_sect, iat_chunk, int_chunk);
    
    // patch-in thunk address
    lnk_section_push_reloc(data_sect, iat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);
    
    if (imptab->flags & LNK_ImportTableFlag_EmitBiat) {
      biat_chunk = lnk_section_push_chunk_bss(data_sect, biat_table_chunk, import_size, sort_index);
      lnk_chunk_set_debugf(data_sect->arena, biat_chunk, "%S.biat.%S (delayed)", dll->name, header->func_name);
      
      // patch-in thunk address
      lnk_section_push_reloc(data_sect, biat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);
    }
    
    if (imptab->flags & LNK_ImportTableFlag_EmitUiat) {
      uiat_chunk = lnk_section_push_chunk_bss(data_sect, uiat_table_chunk, import_size, sort_index);
      lnk_chunk_set_debugf(data_sect->arena, uiat_chunk, "%S.uiat.%S (delayed)", dll->name, header->func_name);
      
      // patch-in thunk address
      lnk_section_push_reloc(data_sect, uiat_chunk, LNK_Reloc_ADDR_64, 0, load_thunk_symbol);
    }
  } break;
  case COFF_ImportBy_Undecorate: {
    lnk_not_implemented("TODO: COFF_ImportBy_Undecorate");
  } break;
  case COFF_ImportBy_NameNoPrefix: {
    lnk_not_implemented("TODO: COFF_ImportBy_NameNoPrefix");
  } break;
  }

  if (jmp_thunk_chunk) {
    lnk_section_associate_chunks(data_sect, iat_chunk, jmp_thunk_chunk);
  }
  if (load_thunk_chunk) {
    lnk_section_associate_chunks(data_sect, iat_chunk, load_thunk_chunk);
  }
  
  String8     iat_symbol_name = push_str8f(symtab->arena->v[0], "__imp_%S", header->func_name);
  LNK_Symbol *iat_symbol      = lnk_symbol_table_push_defined_chunk(symtab, iat_symbol_name, LNK_DefinedSymbolVisibility_Extern, 0, iat_chunk, 0, 0, 0);
  
  String8     ilt_symbol_name = push_str8f(symtab->arena->v[0], "%S.%S.ilt.delayed", dll->name, header->func_name);
  LNK_Symbol *ilt_symbol      = lnk_symbol_table_push_defined_chunk(symtab, ilt_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, ilt_chunk, 0, 0, 0);
  
  // fill out import
  LNK_ImportFunc *func    = push_array(imptab->arena, LNK_ImportFunc, 1);
  func->name              = push_str8_copy(imptab->arena, header->func_name);
  func->thunk_symbol_name = push_str8_copy(imptab->arena, jmp_thunk_symbol->name);
  func->iat_symbol_name   = push_str8_copy(imptab->arena, iat_symbol->name);

  lnk_import_table_push_func_node(imptab, dll, func);
  
  ProfEnd();
  return func;
#endif
  return 0;
}

internal COFF_ObjSymbol *
lnk_emit_indirect_jump_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *iat_symbol, String8 thunk_name)
{
  ProfBeginFunction();
  
  static U8 thunk[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 }; // jmp [__imp_<FUNC_NAME>]
  
  // emit chunk
  String8 jmp_data        = push_str8_copy(obj_writer->arena, str8_array_fixed(thunk));
  U64     jmp_data_offset = code_sect->data.total_size;
  str8_list_push(obj_writer->arena, &code_sect->data, jmp_data);
  
  // patch thunk with imports address
  static const U64 JMP_OPERAND_OFFSET = 2;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, jmp_data_offset + JMP_OPERAND_OFFSET, iat_symbol, COFF_Reloc_X64_Rel32); 
  
  COFF_ObjSymbol *jmp_thunk_symbol = coff_obj_writer_push_symbol_extern(obj_writer, thunk_name, jmp_data_offset, code_sect);

  ProfEnd();
  return jmp_thunk_symbol;
}

internal COFF_ObjSymbol *
lnk_emit_load_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *imp_addr_ptr, COFF_ObjSymbol *tail_merge, String8 func_name)
{
  ProfBeginFunction();
  
  static U8 load_thunk[] = {
    0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00,  // lea rax, [__imp_<FUNC_NAME>]
    0xE9, 0x00, 0x00, 0x00, 0x00               // jmp __tailMerge_<DLL_NAME>
  };
  
  // emit load thunk chunk
  U64     load_thunk_data_offset = code_sect->data.total_size;
  String8 load_thunk_data        = push_str8_copy(obj_writer->arena, str8_array_fixed(load_thunk));
  str8_list_push(obj_writer->arena, &code_sect->data, load_thunk_data);
  
  // patch lea with IAT entry
  static const U64 LEA_OPERAND_OFFSET = 3;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, load_thunk_data_offset + LEA_OPERAND_OFFSET, imp_addr_ptr, COFF_Reloc_X64_Rel32);
  
  // patch jmp __tailMerge_<DLL_NAME>
  static const U64 JMP_OPERAND_OFFSET = 8;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, load_thunk_data_offset + JMP_OPERAND_OFFSET, tail_merge, COFF_Reloc_X64_Rel32);

  // emit symbol
  String8         thunk_name        = push_str8f(obj_writer->arena, "__imp_load_%S", func_name);
  COFF_ObjSymbol *load_thunk_symbol = coff_obj_writer_push_symbol_extern(obj_writer, thunk_name, load_thunk_data_offset, code_sect);
  
  ProfEnd();
  return load_thunk_symbol;
}

internal COFF_ObjSymbol *
lnk_emit_tail_merge_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, String8 dll_name, COFF_ObjSymbol *dll_import_descriptor)
{
  ProfBeginFunction();
  
  static U8 tail_merge[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08,                   // mov         qword ptr [rsp+8],rcx  
    0x48, 0x89, 0x54, 0x24, 0x10,                   // mov         qword ptr [rsp+10h],rdx  
    0x4C, 0x89, 0x44, 0x24, 0x18,                   // mov         qword ptr [rsp+18h],r8  
    0x4C, 0x89, 0x4C, 0x24, 0x20,                   // mov         qword ptr [rsp+20h],r9  
    0x48, 0x83, 0xEC, 0x68,                         // sub         rsp,68h  
    0x66, 0x0F, 0x7F, 0x44, 0x24, 0x20,             // movdqa      xmmword ptr [rsp+20h],xmm0  
    0x66, 0x0F, 0x7F, 0x4C, 0x24, 0x30,             // movdqa      xmmword ptr [rsp+30h],xmm1  
    0x66, 0x0F, 0x7F, 0x54, 0x24, 0x40,             // movdqa      xmmword ptr [rsp+40h],xmm2  
    0x66, 0x0F, 0x7F, 0x5C, 0x24, 0x50,             // movdqa      xmmword ptr [rsp+50h],xmm3  
    0x48, 0x8B, 0xD0,                               // mov         rdx,rax  
    0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,       // lea         rcx,[__DELAY_IMPORT_DESCRIPTOR_<DLL_NAME>]  
    0xE8, 0x00, 0x00, 0x00, 0x00,                   // call        __delayLoadHelper2
    0x66, 0x0F, 0x6F, 0x44, 0x24, 0x20,             // movdqa      xmm0,xmmword ptr [rsp+20h]  
    0x66, 0x0F, 0x6F, 0x4C, 0x24, 0x30,             // movdqa      xmm1,xmmword ptr [rsp+30h]  
    0x66, 0x0F, 0x6F, 0x54, 0x24, 0x40,             // movdqa      xmm2,xmmword ptr [rsp+40h]  
    0x66, 0x0F, 0x6F, 0x5C, 0x24, 0x50,             // movdqa      xmm3,xmmword ptr [rsp+50h]  
    0x48, 0x8B, 0x4C, 0x24, 0x70,                   // mov         rcx,qword ptr [rsp+70h]  
    0x48, 0x8B, 0x54, 0x24, 0x78,                   // mov         rdx,qword ptr [rsp+78h]  
    0x4C, 0x8B, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, // mov         r8,qword ptr [rsp+80h]  
    0x4C, 0x8B, 0x8C, 0x24, 0x88, 0x00, 0x00, 0x00, // mov         r9,qword ptr [rsp+88h]  
    0x48, 0x83, 0xC4, 0x68,                         // add         rsp,68h  
    0xFF, 0xE0,                                     // jmp         rax
  };
  
  // emit tail merge chunk
  String8 tail_merge_data = push_str8_copy(obj_writer->arena, str8_array_fixed(tail_merge));
  U64     tail_merge_off  = code_sect->data.total_size;
  str8_list_push(obj_writer->arena, &code_sect->data, tail_merge_data);
  
  // patch lea __DELAY_IMPORT_DESCRIPTOR_<DLL_NAME>
  static const U64 LEA_OPERAND_OFFSET = 54;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, tail_merge_off + LEA_OPERAND_OFFSET, dll_import_descriptor, COFF_Reloc_X64_Rel32);

  COFF_ObjSymbol *delay_load_helper = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit(LNK_DELAY_LOAD_HELPER2_SYMBOL_NAME));
  
  // patch call __delayLoadHelper2
  static const U64 CALL_OPERAND_OFFSET = 59;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, tail_merge_off + CALL_OPERAND_OFFSET, delay_load_helper, COFF_Reloc_X64_Rel32);

  // emit symbol
  String8 tail_merge_name = push_str8f(obj_writer->arena, "__tailMerge_%S", dll_name);
  COFF_ObjSymbol *tail_merge_symbol = coff_obj_writer_push_symbol_extern(obj_writer, tail_merge_name, tail_merge_off, code_sect);
  
  ProfEnd();
  return tail_merge_symbol;
}

internal String8
lnk_build_import_entry_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  Assert(machine == COFF_MachineType_X64);
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));

  String8 dll_name_no_ext = str8_chop_last_dot(dll_name);

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  String8 debug_symbols;
  {
    CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };
    String8       comp3_data  = lnk_make_linker_compile3(scratch.arena, machine);
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);
    debug_symbols = lnk_make_debug_s(obj_writer->arena, symbol_list);
  }

  String8 dll_name_cstr = push_cstr(obj_writer->arena, dll_name);
  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), LNK_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata2 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$2"), LNK_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *idata6 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$6"), LNK_DATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, dll_name_cstr);

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x1018175, COFF_SymStorageClass_Static);
  {
    String8 symbol_name = push_str8f(arena, "__IMPORT_DESCRIPTOR_%S", dll_name_no_ext);
    coff_obj_writer_push_symbol_extern(obj_writer, symbol_name, 0, idata2);
  }
  COFF_ObjSymbol *idata2_symbol = coff_obj_writer_push_symbol_sect(obj_writer, idata2->name, idata2);
  COFF_ObjSymbol *idata6_symbol = coff_obj_writer_push_symbol_static(obj_writer, idata6->name, 0, idata6);
  COFF_ObjSymbol *idata4_symbol = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".idata$4"), COFF_SectionFlag_MemWrite|COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData);
  COFF_ObjSymbol *idata5_symbol = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".idata$5"), COFF_SectionFlag_MemWrite|COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData);
  coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__NULL_IMPORT_DESCRIPTOR"));
  {
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    coff_obj_writer_push_symbol_undef(obj_writer, symbol_name);
  }

  {
    PE_ImportEntry *import_entry = push_array(obj_writer->arena, PE_ImportEntry, 1);
    str8_list_push(obj_writer->arena, &idata2->data, str8_struct(import_entry));
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, name_voff),              idata6_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, lookup_table_voff),      idata4_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, import_addr_table_voff), idata5_symbol, COFF_Reloc_X64_Addr32Nb);
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);

  coff_obj_writer_release(&obj_writer);

  scratch_end(scratch);
  ProfEnd();
  return obj;
}

internal String8
lnk_build_null_import_descriptor_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  String8 debug_symbols;
  {
    CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };
    String8       comp3_data  = lnk_make_linker_compile3(scratch.arena, machine);
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);
    debug_symbols = lnk_make_debug_s(obj_writer->arena, symbol_list);
  }

  PE_ImportEntry *import_desc = push_array(obj_writer->arena, PE_ImportEntry, 1);
  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), LNK_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata3 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$3"), LNK_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_struct(import_desc));

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x01018175, COFF_SymStorageClass_Static);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("__NULL_IMPORT_DESCRIPTOR"), 0, idata3);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);

  coff_obj_writer_release(&obj_writer);

  scratch_end(scratch);
  ProfEnd();
  return obj;
}

internal String8
lnk_build_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  String8 debug_symbols;
  {
    CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };
    String8       comp3_data  = lnk_make_linker_compile3(scratch.arena, machine);
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);
    debug_symbols = lnk_make_debug_s(obj_writer->arena, symbol_list);
  }

  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), LNK_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata4 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$4"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite, str8_zero());
  COFF_ObjSection *idata5 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$5"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite, str8_zero());

  U64 import_size = coff_word_size_from_machine(machine);

  U8 *null_thunk  = push_array(obj_writer->arena, U8, import_size);
  U8 *null_lookup = push_array(obj_writer->arena, U8, import_size);

  str8_list_push(obj_writer->arena, &idata5->data, str8_array(null_thunk, import_size));
  str8_list_push(obj_writer->arena, &idata4->data, str8_array(null_lookup, import_size));

  idata4->flags |= coff_section_flag_from_align_size(import_size);
  idata5->flags |= coff_section_flag_from_align_size(import_size);

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x1018175, COFF_SymStorageClass_Static);
  {
    String8 dll_name_no_ext = str8_chop_last_dot(dll_name);
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    coff_obj_writer_push_symbol_extern(obj_writer, symbol_name, 0, idata5);
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  
  coff_obj_writer_release(&obj_writer);

  scratch_end(scratch);
  ProfEnd();
  return obj;
}

internal LNK_InputObjList
lnk_import_table_serialize(Arena *arena, LNK_ImportTable *imptab, String8 image_name, COFF_MachineType machine)
{
  Temp scratch = scratch_begin(&arena, 1);

  // append .debug$S
  for (LNK_ImportDLL *dll = imptab->first_dll; dll != 0; dll = dll->next) {
    String8 debug_symbols = {0};
    {
      Temp temp = temp_begin(scratch.arena);

      CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };

      // S_OBJ
      String8 obj_data = cv_make_obj_name(temp.arena, dll->name, 0);
      cv_symbol_list_push_data(temp.arena, &symbol_list, CV_SymKind_OBJNAME, obj_data);

      // S_COMPILE3
      String8 comp3_data = lnk_make_linker_compile3(temp.arena, dll->obj_writer->machine);
      cv_symbol_list_push_data(temp.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);

      // S_END
      cv_symbol_list_push_data(temp.arena, &symbol_list, CV_SymKind_END, str8_zero());

      // TODO: add thunks

      // serialize symbols
      debug_symbols = lnk_make_debug_s(dll->obj_writer->arena, symbol_list);

      temp_end(temp);
    }
    coff_obj_writer_push_section(dll->obj_writer, str8_lit(".debug$S"), LNK_DEBUG_SECTION_FLAGS, debug_symbols);
  }

  LNK_InputObjList result = {0};

  // serialize obj writers
  for (LNK_ImportDLL *dll = imptab->first_dll; dll != 0; dll = dll->next) {
    String8 obj = coff_obj_writer_serialize(arena, dll->obj_writer);

    LNK_InputObj *input = lnk_input_obj_list_push(arena, &result);
    input->data         = obj;
    input->path         = push_str8f(arena, "Import:%S", dll->name);
    input->dedup_id     = input->path;
  }

  // generate null terminator objs 
  String8 null_import_entry_obj      = lnk_build_import_entry_obj(arena, image_name, COFF_TimeStamp_Max, machine);
  String8 null_import_descriptor_obj = lnk_build_null_import_descriptor_obj(arena, COFF_TimeStamp_Max, machine);
  String8 null_thunk_data_obj        = lnk_build_null_thunk_data_obj(arena, image_name, COFF_TimeStamp_Max, machine);

  // pick null terminator obj paths
  String8 null_import_path;
  String8 null_import_descriptor_path;
  String8 null_thunk_path;
  if (imptab->flags & LNK_ImportTableFlag_Delayed) {
    null_import_path            = str8_lit("DELAYED_NULL_IMPORT_ENTRY_OBJ");
    null_import_descriptor_path = str8_lit("DELAYED_NULL_IMPORT_DESCRIPTOR_OBJ");
    null_thunk_path             = str8_lit("DELAYED_NULL_THUNK_OBJ");
  } else {
    null_import_path            = str8_lit("NULL_IMPORT_ENTRY_OBJ");
    null_import_descriptor_path = str8_lit("NULL_IMPORT_DESCRIPTOR_OBJ");
    null_thunk_path             = str8_lit("NULL_THUNK_OBJ");
  }

  os_write_data_to_file_path(str8_lit("null_import_entry.obj"), null_import_entry_obj);
  os_write_data_to_file_path(str8_lit("null_import_descriptor.obj"), null_import_descriptor_obj);
  os_write_data_to_file_path(str8_lit("null_thunk.obj"), null_thunk_data_obj);

  // append null terminators
  {
    LNK_InputObj *input = lnk_input_obj_list_push(arena, &result);
    input->data         = null_import_entry_obj;
    input->path         = null_import_path;
    input->dedup_id     = input->path;
  }
  {
    LNK_InputObj *input = lnk_input_obj_list_push(arena, &result);
    input->data         = null_import_descriptor_obj;
    input->path         = null_import_descriptor_path;
    input->dedup_id     = input->path;
  }
  {
    LNK_InputObj *input = lnk_input_obj_list_push(arena, &result);
    input->data         = null_thunk_data_obj;
    input->path         = null_thunk_path;
    input->dedup_id     = input->path;
  }

  scratch_end(scratch);
  return result;
}

