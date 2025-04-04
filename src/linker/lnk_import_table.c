// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_ImportTable *
lnk_import_table_alloc_static(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, COFF_MachineType machine)
{
  ProfBeginFunction();
  
  LNK_Section *data_sect = lnk_section_table_push(sectab, str8_lit(".idata"), LNK_IDATA_SECTION_FLAGS);
  LNK_Section *code_sect = lnk_section_table_search(sectab, str8_lit(".text"));
  
  LNK_Chunk *dll_table_chunk = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *int_chunk       = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *iat_chunk       = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *ilt_chunk       = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *code_chunk      = lnk_section_push_chunk_list(code_sect, code_sect->root, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, dll_table_chunk, "DLL_TABLE"           );
  lnk_chunk_set_debugf(data_sect->arena, int_chunk,       "IMPORT_NAME_TABLE"   );
  lnk_chunk_set_debugf(data_sect->arena, iat_chunk,       "IMPORT_ADDRESS_TABLE");
  lnk_chunk_set_debugf(data_sect->arena, ilt_chunk,       "IMPORT_LOOKUP_TABLE" );
  lnk_chunk_set_debugf(data_sect->arena, code_chunk,      "IMPORT_TABLE_CODE"   );

  LNK_Chunk *null_dll_import = lnk_section_push_chunk_data(data_sect, dll_table_chunk, str8(0, sizeof(PE_ImportEntry)), str8_lit("zzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_dll_import, "DLL_DIRECTORY_TERMINATOR");
  
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_IMPORT_DLL_TABLE_SYMBOL_NAME) , LNK_DefinedSymbolVisibility_Internal, 0, dll_table_chunk, 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_IMPORT_NAME_TABLE_SYMBOL_NAME), LNK_DefinedSymbolVisibility_Internal, 0, int_chunk      , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_IMPORT_IAT_SYMBOL_NAME)       , LNK_DefinedSymbolVisibility_Internal, 0, iat_chunk      , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_IMPORT_ILT_SYMBOL_NAME)       , LNK_DefinedSymbolVisibility_Internal, 0, ilt_chunk      , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_IMPORT_JMP_SYMBOL_NAME)       , LNK_DefinedSymbolVisibility_Internal, 0, code_chunk     , 0, 0, 0);
  
  Arena *arena = arena_alloc();
  LNK_ImportTable *imptab = push_array(arena, LNK_ImportTable, 1);
  imptab->machine         = machine;
  imptab->arena           = arena;
  imptab->data_sect       = data_sect;
  imptab->code_sect       = code_sect;
  imptab->dll_table_chunk = dll_table_chunk;
  imptab->int_chunk       = int_chunk;
  imptab->iat_chunk       = iat_chunk;
  imptab->ilt_chunk       = ilt_chunk;
  imptab->code_chunk      = code_chunk;
  imptab->dll_ht          = hash_table_init(arena, LNK_IMPORT_DLL_HASH_TABLE_BUCKET_COUNT);

  ProfEnd();
  return imptab;
}

internal LNK_ImportTable *
lnk_import_table_alloc_delayed(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, COFF_MachineType machine, B32 is_unloadable, B32 is_bindable)
{
  ProfBeginFunction();
  
  LNK_Section *data_sect = lnk_section_table_push(sectab, str8_lit(".didat"), LNK_DEBUG_DIR_SECTION_FLAGS);
  LNK_Section *code_sect = lnk_section_table_search(sectab, str8_lit(".text"));
  
  LNK_Chunk *dll_table_chunk    = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *int_chunk          = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *handle_table_chunk = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *iat_chunk          = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *ilt_chunk          = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *biat_chunk         = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *uiat_chunk         = lnk_section_push_chunk_list(data_sect, data_sect->root, str8_zero());
  LNK_Chunk *code_chunk         = lnk_section_push_chunk_list(code_sect, code_sect->root, str8_zero());
  
  LNK_Chunk *null_dll_import = lnk_section_push_chunk_data(data_sect, dll_table_chunk, str8(0, sizeof(PE_DelayedImportEntry)), str8_lit("~0"));
  lnk_chunk_set_debugf(data_sect->arena, null_dll_import, "DLL_DIRECTORY_TERMINATOR");

  if (is_unloadable) {
    U64 import_size = coff_word_size_from_machine(machine);
    LNK_Chunk *null_uiat_chunk = lnk_section_push_chunk_bss(data_sect, uiat_chunk, import_size, str8_lit("~1"));
    lnk_chunk_set_debugf(data_sect->arena, null_uiat_chunk, "UIAT_TERMINATOR");
  }

  if (is_bindable) {
    U64 import_size = coff_word_size_from_machine(machine);
    LNK_Chunk *null_biat_chunk = lnk_section_push_chunk_bss(data_sect, biat_chunk, import_size, str8_lit("~2"));
    lnk_chunk_set_debugf(data_sect->arena, null_biat_chunk, "BIAT_TERMINATOR");
  }
  
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_DLL_TABLE_SYMBOL_NAME)   , LNK_DefinedSymbolVisibility_Internal, 0, dll_table_chunk   , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_INT_SYMBOL_NAME)         , LNK_DefinedSymbolVisibility_Internal, 0, int_chunk         , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_HANDLE_TABLE_SYMBOL_NAME), LNK_DefinedSymbolVisibility_Internal, 0, handle_table_chunk, 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_IAT_SYMBOL_NAME)         , LNK_DefinedSymbolVisibility_Internal, 0, iat_chunk         , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_ILT_SYMBOL_NAME)         , LNK_DefinedSymbolVisibility_Internal, 0, ilt_chunk         , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_BIAT_SYMBOL_NAME)        , LNK_DefinedSymbolVisibility_Internal, 0, biat_chunk        , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_UIAT_SYMBOL_NAME)        , LNK_DefinedSymbolVisibility_Internal, 0, uiat_chunk        , 0, 0, 0);
  lnk_symbol_table_push_defined_chunk(symtab, str8_cstring(LNK_DELAYED_IMPORT_CODE_SYMBOL_NAME)        , LNK_DefinedSymbolVisibility_Internal, 0, code_chunk        , 0, 0, 0);
  
  LNK_ImportTableFlags flags = 0;
  if (is_unloadable) {
    flags |= LNK_ImportTableFlag_EmitUiat;
  }
  if (is_bindable) {
    flags |= LNK_ImportTableFlag_EmitBiat;
  }
  
  Arena *arena = arena_alloc();
  LNK_ImportTable *imptab    = push_array(arena, LNK_ImportTable, 1);
  imptab->arena              = arena;
  imptab->machine            = machine;
  imptab->data_sect          = data_sect;
  imptab->code_sect          = code_sect;
  imptab->dll_table_chunk    = dll_table_chunk;
  imptab->int_chunk          = int_chunk;
  imptab->handle_table_chunk = handle_table_chunk;
  imptab->iat_chunk          = iat_chunk;
  imptab->ilt_chunk          = ilt_chunk;
  imptab->biat_chunk         = biat_chunk;
  imptab->uiat_chunk         = uiat_chunk;
  imptab->code_chunk         = code_chunk;
  imptab->flags              = flags;
  imptab->dll_ht             = hash_table_init(arena, LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT);
  
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
  KeyValuePair *kv = hash_table_search_path(imptab->dll_ht, name);
  if (kv) {
    Assert(kv->value_raw);
    return kv->value_raw;
  }
  return 0;
}

internal LNK_ImportFunc *
lnk_import_table_search_func(LNK_ImportDLL *dll, String8 name)
{
  KeyValuePair *kv = hash_table_search_string(dll->func_ht, name);
  if (kv) {
    Assert(kv->value_raw);
    return kv->value_raw;
  }
  return 0;
}

internal LNK_ImportDLL *
lnk_import_table_push_dll_static(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();

  // TODO: error handle
  Assert(imptab->machine == machine);
  
  LNK_Section *data_sect = imptab->data_sect;
  LNK_Section *code_sect = imptab->code_sect;
  
  LNK_Chunk *int_table_chunk  = lnk_section_push_chunk_list(data_sect, imptab->int_chunk,  str8_zero());
  LNK_Chunk *ilt_table_chunk  = lnk_section_push_chunk_list(data_sect, imptab->ilt_chunk,  str8_zero()); 
  LNK_Chunk *iat_table_chunk  = lnk_section_push_chunk_list(data_sect, imptab->iat_chunk,  str8_zero());
  LNK_Chunk *code_table_chunk = lnk_section_push_chunk_list(code_sect, imptab->code_chunk, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, int_table_chunk,  "%S.INT",  dll_name);
  lnk_chunk_set_debugf(data_sect->arena, ilt_table_chunk,  "%S.ILT",  dll_name);
  lnk_chunk_set_debugf(data_sect->arena, iat_table_chunk,  "%S.IAT",  dll_name);
  lnk_chunk_set_debugf(data_sect->arena, code_table_chunk, "%S.CODE", dll_name);
  
  String8     ilt_symbol_name = push_str8f(symtab->arena->v[0], "%S.lookup_table_voff", dll_name);
  LNK_Symbol *ilt_symbol      = lnk_symbol_table_push_defined_chunk(symtab, ilt_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, ilt_table_chunk, 0, 0, 0);
  
  String8    iat_symbol_name = push_str8f(symtab->arena->v[0], "%S.import_addr_table_voff", dll_name);
  LNK_Symbol *iat_symbol     = lnk_symbol_table_push_defined_chunk(symtab, iat_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, iat_table_chunk, 0, 0, 0);
  
  String8    dll_name_cstr  = push_cstr(data_sect->arena, dll_name);
  LNK_Chunk *dll_name_chunk = lnk_section_push_chunk_data(data_sect, int_table_chunk, dll_name_cstr, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, dll_name_chunk, "DLL name chunk (%S)", dll_name);
  
  String8     dll_name_voff_name   = push_str8f(symtab->arena->v[0], "%S.name_voff", dll_name);
  LNK_Symbol *dll_name_voff_symbol = lnk_symbol_table_push_defined_chunk(symtab, dll_name_voff_name, LNK_DefinedSymbolVisibility_Internal, 0, dll_name_chunk, 0, 0, 0);
  
  // chunk for dll directory entry
  PE_ImportEntry *dir       = push_array(imptab->arena, PE_ImportEntry, 1);
  LNK_Chunk      *dll_chunk = lnk_section_push_chunk_data(data_sect, imptab->dll_table_chunk, str8_struct(dir), str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, dll_chunk, "DLL Directory for %S", dll_name);
  
  // patch dll import fields
  lnk_section_push_reloc(data_sect, dll_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ImportEntry, lookup_table_voff), ilt_symbol);
  lnk_section_push_reloc(data_sect, dll_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ImportEntry, name_voff), dll_name_voff_symbol);
  lnk_section_push_reloc(data_sect, dll_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_ImportEntry, import_addr_table_voff), iat_symbol);
  
  U64 import_size = coff_word_size_from_machine(machine);
  
  // null entry to terminate import lookup table array
  LNK_Chunk *null_ilt_chunk = lnk_section_push_chunk_data(data_sect, ilt_table_chunk, str8(0, import_size), str8_lit("zzzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_ilt_chunk, "%S: ILT terminator", dll_name);
  
  // null entry to terminate import address table array
  LNK_Chunk *null_iat_chunk = lnk_section_push_chunk_data(data_sect, iat_table_chunk, str8(0, import_size), str8_lit("zzzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_iat_chunk, "%S: IAT terminator", dll_name);
  
  // push to list
  LNK_ImportDLL *dll    = push_array(imptab->arena, LNK_ImportDLL, 1);
  dll->name             = push_str8_copy(imptab->arena, dll_name);
  dll->dll_chunk        = dll_chunk;
  dll->int_table_chunk  = int_table_chunk;
  dll->ilt_table_chunk  = ilt_table_chunk;
  dll->iat_table_chunk  = iat_table_chunk;
  dll->code_table_chunk = code_table_chunk;
  dll->machine          = machine;
  dll->func_ht          = hash_table_init(imptab->arena, LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT);

  lnk_import_table_push_dll_node(imptab, dll);
  
  ProfEnd();
  return dll;
}

internal LNK_ImportDLL *
lnk_import_table_push_dll_delayed(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, String8 dll_name, COFF_MachineType machine)
{
  ProfBeginFunction();

  Assert(imptab->machine == machine);
  
  U64 handle_size = coff_word_size_from_machine(machine);
  U64 import_size = coff_word_size_from_machine(machine);
  
  // shortcuts
  LNK_Section *data_sect = imptab->data_sect;
  LNK_Section *code_sect = imptab->code_sect;
  
  // init DLL entry
  PE_DelayedImportEntry *imp_desc = push_array(data_sect->arena, PE_DelayedImportEntry, 1);
  imp_desc->attributes            = 1;
  imp_desc->name_voff             = 0; // relocated
  imp_desc->module_handle_voff    = 0; // relocated
  imp_desc->iat_voff              = 0; // relocated
  imp_desc->name_table_voff       = 0; // relocated
  imp_desc->bound_table_voff      = 0; // relocated
  imp_desc->unload_table_voff     = 0; // relocated
  imp_desc->time_stamp            = 0;
  
  // emit entry chunk
  String8    imp_desc_data  = str8_struct(imp_desc);
  LNK_Chunk *imp_desc_chunk = lnk_section_push_chunk_data(data_sect, imptab->dll_table_chunk, imp_desc_data, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, imp_desc_chunk, "%S.IMP_DESC", dll_name);
  
  // emit entry symbol
  String8     imp_desc_name   = push_str8f(symtab->arena->v[0], "__DELAY_IMPORT_DESCRIPTOR_%S", dll_name);
  LNK_Symbol *imp_desc_symbol = lnk_symbol_table_push_defined_chunk(symtab, imp_desc_name, LNK_DefinedSymbolVisibility_Extern, 0, imp_desc_chunk, 0, 0, 0);
  
  // emit string table chunk
  LNK_Chunk *int_table_chunk = lnk_section_push_chunk_list(data_sect, imptab->int_chunk, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, int_table_chunk, "%S.DELAY_INT", dll_name);
  
  String8     int_table_symbol_name = push_str8f(symtab->arena->v[0], "delayed.%S.int", dll_name);
  LNK_Symbol *int_table_symbol      = lnk_symbol_table_push_defined_chunk(symtab, int_table_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, int_table_chunk, 0, 0, 0);
  
  LNK_Chunk *null_string_chunk = lnk_section_push_chunk_list(data_sect, int_table_chunk, str8_lit("zzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_string_chunk, "%S.STRING_TABLE_NULL", dll_name);
  
  // emit DLL name chunk
  String8    name_chunk_data = push_cstr(data_sect->arena, dll_name);
  LNK_Chunk *name_chunk      = lnk_section_push_chunk_data(data_sect, int_table_chunk, name_chunk_data, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, name_chunk, "%S.DELAY_NAME", dll_name);
  
  String8     name_symbol_name = push_str8f(symtab->arena->v[0], "delayed.%S.name", dll_name);
  LNK_Symbol *name_symbol      = lnk_symbol_table_push_defined_chunk(symtab, name_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, name_chunk, 0, 0, 0);
  
  // patch DLL name voff
  lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, name_voff), name_symbol);
  
  // emit DLL handle chunk
  LNK_Chunk *handle_chunk = lnk_section_push_chunk_bss(data_sect, imptab->handle_table_chunk, handle_size, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, handle_chunk, "%S.DELAY_HANDLE", dll_name);
  
  String8     handle_name   = push_str8f(symtab->arena->v[0], "delayed.%S.handle", dll_name);
  LNK_Symbol *handle_symbol = lnk_symbol_table_push_defined_chunk(symtab, handle_name, LNK_DefinedSymbolVisibility_Internal, 0, handle_chunk, 0, 0, 0);
  
  // patch DLL handle voff
  lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, module_handle_voff), handle_symbol);
  
  // emit IAT chunk
  LNK_Chunk *iat_table_chunk = lnk_section_push_chunk_list(data_sect, imptab->iat_chunk, str8_zero());
  lnk_chunk_set_debugf(data_sect->arena, iat_table_chunk, "%S.DELAY_IAT", dll_name);
  
  String8     iat_table_name   = push_str8f(symtab->arena->v[0], "delayed.%S.iat", dll_name);
  LNK_Symbol *iat_table_symbol = lnk_symbol_table_push_defined_chunk(symtab, iat_table_name, LNK_DefinedSymbolVisibility_Internal, 0, iat_table_chunk, 0, 0, 0);
  
  LNK_Chunk *null_iat_chunk = lnk_section_push_chunk_bss(data_sect, iat_table_chunk, import_size, str8_lit("zzzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_iat_chunk, "%S.DELAY_IAT_TERMINATOR", dll_name);
  
  // emit ILT chunk
  LNK_Chunk *ilt_table_chunk = lnk_section_push_chunk_list(data_sect, imptab->ilt_chunk, str8_zero());
  
  LNK_Chunk *null_ilt_chunk = lnk_section_push_chunk_bss(data_sect, ilt_table_chunk, import_size, str8_lit("zzzzzz"));
  lnk_chunk_set_debugf(data_sect->arena, null_ilt_chunk, "%S.DELAY_ILT_TERMINATOR", dll_name);
  
  String8     ilt_table_name   = push_str8f(symtab->arena->v[0], "delayed.%S.ilt", dll_name);
  LNK_Symbol *ilt_table_symbol = lnk_symbol_table_push_defined_chunk(symtab, ilt_table_name, LNK_DefinedSymbolVisibility_Extern, 0, ilt_table_chunk, 0, 0, 0);
  
  // patch import address table voff
  lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, iat_voff), iat_table_symbol);
  
  // patch string table voff
  lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, name_table_voff), ilt_table_symbol);
  
  // emit bound table chunk
  LNK_Chunk *biat_chunk = 0;
  if (imptab->flags & LNK_ImportTableFlag_EmitBiat) {
    biat_chunk = lnk_section_push_chunk_list(data_sect, imptab->biat_chunk, str8_zero());
    lnk_chunk_set_debugf(data_sect->arena, biat_chunk, "%S.DELAY_BIAT", dll_name);
    
    String8     biat_symbol_name = push_str8f(symtab->arena->v[0], "delayed.%S.BIAT", dll_name);
    LNK_Symbol *biat_symbol      = lnk_symbol_table_push_defined_chunk(symtab, biat_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, biat_chunk, 0, 0, 0);
    
    // patch BIAT field off
    lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, bound_table_voff), biat_symbol);
  }
  
  // emit unload table chunk
  LNK_Chunk *uiat_chunk = NULL;
  if (imptab->flags & LNK_ImportTableFlag_EmitUiat) {
    uiat_chunk = lnk_section_push_chunk_list(data_sect, imptab->uiat_chunk, str8_zero());
    lnk_chunk_set_debugf(data_sect->arena, uiat_chunk, "%S.DELAY_UIAT", dll_name);
    
    String8     uiat_symbol_name = push_str8f(symtab->arena->v[0], "delayed.%S.UIAT", dll_name);
    LNK_Symbol *uiat_symbol      = lnk_symbol_table_push_defined_chunk(symtab, uiat_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, uiat_chunk, 0, 0, 0);
    
    // patch UIAT field voff
    lnk_section_push_reloc(data_sect, imp_desc_chunk, LNK_Reloc_VIRT_OFF_32, OffsetOf(PE_DelayedImportEntry, unload_table_voff), uiat_symbol);
  }
  
  // emit chunk for DLL thunk/load code
  LNK_Chunk *code_chunk = lnk_section_push_chunk_list(code_sect, imptab->code_chunk, str8_zero());
  lnk_chunk_set_debugf(code_sect->arena, code_chunk, "%S.DLAY_CODE", dll_name);
  
  // emit tail merge
  LNK_Chunk *tail_merge_chunk = 0;
  switch (machine) {
  case COFF_MachineType_X64: {
    LNK_Symbol *delay_load_helper_symbol = lnk_make_undefined_symbol(symtab->arena->v[0], str8_lit(LNK_DELAY_LOAD_HELPER2_SYMBOL_NAME), LNK_SymbolScopeFlag_Main);
    tail_merge_chunk = lnk_emit_tail_merge_thunk_x64(code_sect, code_chunk, imp_desc_symbol, delay_load_helper_symbol);
    lnk_chunk_set_debugf(code_sect->arena, code_chunk, "%S.X64_TAIL_MERGE", dll_name);
  } break;
  default: {
    lnk_not_implemented("TODO: __tailMerge for %S", coff_string_from_machine_type(machine));
  } break;
  }
  
  // fill out result
  LNK_ImportDLL *dll     = push_array(imptab->arena, LNK_ImportDLL, 1);
  dll->dll_chunk         = imp_desc_chunk;
  dll->int_table_chunk   = int_table_chunk;
  dll->iat_table_chunk   = iat_table_chunk;
  dll->ilt_table_chunk   = ilt_table_chunk;
  dll->biat_table_chunk  = biat_chunk;
  dll->uiat_table_chunk  = uiat_chunk;
  dll->code_table_chunk  = code_chunk;
  dll->tail_merge_symbol = lnk_emit_tail_merge_symbol(symtab, tail_merge_chunk, dll_name);
  dll->name              = push_str8_copy(imptab->arena, dll_name);
  dll->machine           = machine;
  dll->func_ht           = hash_table_init(imptab->arena, LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT);

  lnk_import_table_push_dll_node(imptab, dll);
  
  ProfEnd();
  return dll;
}

internal LNK_ImportFunc *
lnk_import_table_push_func_static(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header)
{
  ProfBeginFunction();
  
  Assert(header->machine == dll->machine); // TODO: error handling

  LNK_Section *data_sect = imptab->data_sect;
  LNK_Section *code_sect = imptab->code_sect;
  
  LNK_Chunk *int_table_chunk  = dll->int_table_chunk;
  LNK_Chunk *ilt_table_chunk  = dll->ilt_table_chunk;
  LNK_Chunk *iat_table_chunk  = dll->iat_table_chunk;
  LNK_Chunk *code_table_chunk = dll->code_table_chunk;
  
  LNK_Chunk *ilt_chunk = g_null_chunk_ptr;
  LNK_Chunk *iat_chunk = g_null_chunk_ptr;
  
  U64 import_size = coff_word_size_from_machine(dll->machine);
  
  // generate sort index (optional)
  String8 sort_index = str8_from_bits_u32(data_sect->arena, header->hint_or_ordinal);
  
  switch (header->import_by) {
  case COFF_ImportBy_Ordinal: {
    String8 ordinal_data = lnk_ordinal_data_from_hint(data_sect->arena, dll->machine, header->hint_or_ordinal);
    ilt_chunk = lnk_section_push_chunk_data(data_sect, ilt_table_chunk, ordinal_data, sort_index);
    iat_chunk = lnk_section_push_chunk_data(data_sect, iat_table_chunk, ordinal_data, sort_index);
    lnk_chunk_set_debugf(data_sect->arena, ilt_chunk, "ILT entry for %S.%u", dll->name, header->hint_or_ordinal);
    lnk_chunk_set_debugf(data_sect->arena, iat_chunk, "IAT entry for %S.%u", dll->name, header->hint_or_ordinal);

    // associate chunks
    lnk_section_associate_chunks(data_sect, iat_chunk, ilt_chunk);
  } break;
  case COFF_ImportBy_Name: {
    // put together name look up entry
    String8    int_data  = coff_make_import_lookup(data_sect->arena, header->hint_or_ordinal, header->func_name);
    LNK_Chunk *int_chunk = lnk_section_push_chunk_data(data_sect, int_table_chunk, int_data, str8_zero());
    lnk_chunk_set_debugf(data_sect->arena, int_chunk, "INT entry for %S.%S (Hint: %u)", dll->name, header->func_name, header->hint_or_ordinal);
    
    // create symbol for lookup chunk
    String8     int_symbol_name = push_str8f(symtab->arena->v[0], "static.%S.%S.name", dll->name, header->func_name);
    LNK_Symbol *int_symbol      = lnk_symbol_table_push_defined_chunk(symtab, int_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, int_chunk, 0, 0, 0);
    
    // in the file IAT mirrors ILT, dynamic linker later overwrites it with imported function addresses.
    ilt_chunk = lnk_section_push_chunk_bss(data_sect, ilt_table_chunk, import_size, sort_index);
    iat_chunk = lnk_section_push_chunk_bss(data_sect, iat_table_chunk, import_size, sort_index);
    lnk_chunk_set_debugf(data_sect->arena, ilt_chunk, "ILT entry for %S.%S", dll->name, header->func_name);
    lnk_chunk_set_debugf(data_sect->arena, iat_chunk, "IAT entry for %S.%S", dll->name, header->func_name);

    // associate chunks
    lnk_section_associate_chunks(data_sect, iat_chunk, ilt_chunk);
    lnk_section_associate_chunks(data_sect, iat_chunk, int_chunk);
    
    // patch IAT and ILT
    lnk_section_push_reloc(data_sect, ilt_chunk, LNK_Reloc_VIRT_OFF_32, 0, int_symbol);
    lnk_section_push_reloc(data_sect, iat_chunk, LNK_Reloc_VIRT_OFF_32, 0, int_symbol);
  } break;
  case COFF_ImportBy_Undecorate: {
    lnk_not_implemented("TODO: COFF_ImportBy_Undecorate");
  } break;
  case COFF_ImportBy_NameNoPrefix: {
    lnk_not_implemented("TODO: COFF_ImportBy_NameNoPrefix");
  } break;
  }
  
  String8     ilt_symbol_name = push_str8f(symtab->arena->v[0], "static.%S.%S.ilt", dll->name, header->func_name);
  String8     iat_symbol_name = push_str8f(symtab->arena->v[0], "__imp_%S", header->func_name);
  LNK_Symbol *ilt_symbol      = lnk_symbol_table_push_defined_chunk(symtab, ilt_symbol_name, LNK_DefinedSymbolVisibility_Internal, 0, ilt_chunk, 0, 0, 0);
  LNK_Symbol *iat_symbol      = lnk_symbol_table_push_defined_chunk(symtab, iat_symbol_name, LNK_DefinedSymbolVisibility_Extern, 0, iat_chunk, 0, 0, 0);

  // generate thunks
  LNK_Symbol *jmp_thunk_symbol = g_null_symbol_ptr;
  if (header->type == COFF_ImportHeader_Code) {
    switch (dll->machine) {
    case COFF_MachineType_X64: {
      // generate jump thunk
      LNK_Chunk *jmp_thunk_chunk = lnk_emit_indirect_jump_thunk_x64(code_sect, code_table_chunk, iat_symbol);
      lnk_section_associate_chunks(data_sect, iat_chunk, jmp_thunk_chunk);
      lnk_chunk_set_debugf(data_sect->arena, jmp_thunk_chunk, "Jump thunk to %S.%S", dll->name, iat_symbol->name);

      // push jump thunk symbol
      String8 jmp_thunk_symbol_name = push_str8_copy(symtab->arena->v[0], header->func_name);
      jmp_thunk_symbol = lnk_emit_jmp_thunk_symbol(symtab, jmp_thunk_chunk, jmp_thunk_symbol_name);
    } break;
    default: lnk_not_implemented("TODO: support for machine 0x%X", dll->machine); break;
    }
  }
  
  // fill out import
  LNK_ImportFunc *func    = push_array(imptab->arena, LNK_ImportFunc, 1);
  func->name              = push_str8_copy(imptab->arena, header->func_name);
  func->thunk_symbol_name = push_str8_copy(imptab->arena, jmp_thunk_symbol->name);
  func->iat_symbol_name   = push_str8_copy(imptab->arena, iat_symbol->name);

  lnk_import_table_push_func_node(imptab, dll, func);
  
  ProfEnd();
  return func;
}

internal LNK_ImportFunc *
lnk_import_table_push_func_delayed(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header)
{
  ProfBeginFunction();
  
  Assert(dll->machine == header->machine); // TODO: error handle

  U64 import_size = coff_word_size_from_machine(dll->machine);
  
  LNK_Section *data_sect = imptab->data_sect;
  LNK_Section *code_sect = imptab->code_sect;
  
  LNK_Chunk *int_table_chunk  = dll->int_table_chunk;
  LNK_Chunk *ilt_table_chunk  = dll->ilt_table_chunk;
  LNK_Chunk *iat_table_chunk  = dll->iat_table_chunk;
  LNK_Chunk *biat_table_chunk = dll->biat_table_chunk;
  LNK_Chunk *uiat_table_chunk = dll->uiat_table_chunk;
  LNK_Chunk *code_table_chunk = dll->code_table_chunk;
  
  LNK_Chunk *ilt_chunk  = g_null_chunk_ptr;
  LNK_Chunk *iat_chunk  = g_null_chunk_ptr;
  LNK_Chunk *uiat_chunk = g_null_chunk_ptr;
  LNK_Chunk *biat_chunk = g_null_chunk_ptr;
  
  LNK_Symbol *int_symbol = 0;
  
  // generate sort index (optional)
  String8 sort_index = str8_from_bits_u32(data_sect->arena, header->hint_or_ordinal);
  
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
}

internal String8
lnk_ordinal_data_from_hint(Arena *arena, COFF_MachineType machine, U16 hint)
{
  String8 ordinal_data = str8_zero();
  switch (machine) {
  case COFF_MachineType_X64: {
    U64 *ordinal = push_array(arena, U64, 1);
    *ordinal     = coff_make_ordinal64(hint);
    ordinal_data = str8_struct(ordinal);
  } break;
  case COFF_MachineType_X86: {
    U32 *ordinal = push_array(arena, U32, 1);
    *ordinal     = coff_make_ordinal32(hint);
    ordinal_data = str8_struct(ordinal);
  } break;
  default: lnk_not_implemented("TODO: support for machine 0x%x", machine);
  }
  return ordinal_data;
}

internal LNK_Chunk *
lnk_emit_indirect_jump_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *addr_ptr)
{
  ProfBeginFunction();
  
  static U8 thunk[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 }; // jmp [__imp_<FUNC_NAME>]
  
  // emit chunk
  String8 jmp_data = push_str8_copy(sect->arena, str8_array_fixed(thunk));
  LNK_Chunk *jmp_chunk = lnk_section_push_chunk_data(sect, parent, jmp_data, str8_zero());
  
  // patch thunk with imports address
  static const U64 JMP_OPERAND_OFFSET = 2;
  lnk_section_push_reloc(sect, jmp_chunk, LNK_Reloc_REL32, JMP_OPERAND_OFFSET, addr_ptr);
  
  ProfEnd();
  return jmp_chunk;
}

internal LNK_Chunk *
lnk_emit_load_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *imp_addr_ptr, LNK_Symbol *tail_merge)
{
  ProfBeginFunction();
  
  static U8 load_thunk[] = {
    0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00,  // lea rax, [__imp_<FUNC_NAME>]
    0xE9, 0x00, 0x00, 0x00, 0x00               // jmp __tailMerge_<DLL_NAME>
  };
  
  // emit load thunk chunk
  String8 load_thunk_data = push_str8_copy(sect->arena, str8_array_fixed(load_thunk));
  LNK_Chunk *load_thunk_chunk = lnk_section_push_chunk_data(sect, parent, load_thunk_data, str8_zero());
  
  // patch lea with IAT entry
  static const U64 LEA_OPERAND_OFFSET = 3;
  lnk_section_push_reloc(sect, load_thunk_chunk, LNK_Reloc_REL32, LEA_OPERAND_OFFSET, imp_addr_ptr);
  
  // patch jmp __tailMerge_<DLL_NAME>
  static const U64 JMP_OPERAND_OFFSET = 8;
  lnk_section_push_reloc(sect, load_thunk_chunk, LNK_Reloc_REL32, JMP_OPERAND_OFFSET, tail_merge);
  
  ProfEnd();
  return load_thunk_chunk;
}

internal LNK_Chunk *
lnk_emit_tail_merge_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *dll_import_descriptor, LNK_Symbol *delay_load_helper)
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
  String8    tail_merge_data  = push_str8_copy(sect->arena, str8_array_fixed(tail_merge));
  LNK_Chunk *tail_merge_chunk = lnk_section_push_chunk_data(sect, parent, tail_merge_data, str8_zero());
  
  // patch lea __DELAY_IMPORT_DESCRIPTOR_<DLL_NAME>
  static const U64 LEA_OPERAND_OFFSET = 54;
  lnk_section_push_reloc(sect, tail_merge_chunk, LNK_Reloc_REL32, LEA_OPERAND_OFFSET, dll_import_descriptor);
  
  // patch call __delayLoadHelper2
  static const U64 CALL_OPERAND_OFFSET = 59;
  lnk_section_push_reloc(sect, tail_merge_chunk, LNK_Reloc_REL32, CALL_OPERAND_OFFSET, delay_load_helper);
  
  ProfEnd();
  return tail_merge_chunk;
}

internal LNK_Symbol *
lnk_emit_load_thunk_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name)
{
  ProfBeginFunction();
  // emit load thunk symbol
  String8     load_thunk_name   = push_str8f(symtab->arena->v[0], "__imp_load_%S", func_name);
  LNK_Symbol *load_thunk_symbol = lnk_symbol_table_push_defined_chunk(symtab, load_thunk_name, LNK_DefinedSymbolVisibility_Extern, LNK_DefinedSymbolFlag_IsFunc|LNK_DefinedSymbolFlag_IsThunk, chunk, 0, COFF_ComdatSelect_NoDuplicates, 0);
  ProfEnd();
  return load_thunk_symbol;
}

internal LNK_Symbol *
lnk_emit_jmp_thunk_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name)
{
  ProfBeginFunction();
  String8     jmp_thunk_name   = push_str8f(symtab->arena->v[0], "%S", func_name);
  LNK_Symbol *jmp_thunk_symbol = lnk_symbol_table_push_defined_chunk(symtab, jmp_thunk_name, LNK_DefinedSymbolVisibility_Extern, LNK_DefinedSymbolFlag_IsFunc|LNK_DefinedSymbolFlag_IsThunk, chunk, 0, COFF_ComdatSelect_Any, 0);
  ProfEnd();
  return jmp_thunk_symbol;
}

internal LNK_Symbol *
lnk_emit_tail_merge_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name)
{
  ProfBeginFunction();
  String8     tail_merge_name   = push_str8f(symtab->arena->v[0], "__tailMerge_%S", func_name);
  LNK_Symbol *tail_merge_symbol = lnk_symbol_table_push_defined_chunk(symtab, tail_merge_name, LNK_DefinedSymbolVisibility_Extern, LNK_DefinedSymbolFlag_IsFunc|LNK_DefinedSymbolFlag_IsThunk, chunk, 0, COFF_ComdatSelect_NoDuplicates, 0);
  ProfEnd();
  return tail_merge_symbol;
}

