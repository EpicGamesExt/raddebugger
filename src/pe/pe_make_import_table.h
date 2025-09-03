// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PE_MAKE_IMPORT_TABLE_H
#define PE_MAKE_IMPORT_TABLE_H

typedef struct PE_MakeImport
{
  String8 header;
  B32     make_jump_thunk;
} PE_MakeImport;

typedef struct PE_MakeImportNode
{
  PE_MakeImport             v;
  struct PE_MakeImportNode *next;
} PE_MakeImportNode;

typedef struct PE_MakeImportList
{
  U64                      count;
  PE_MakeImportNode *first;
  PE_MakeImportNode *last;
} PE_MakeImportList;

// -----------------------------------------------------------------------------

internal void                pe_make_import_header_list_push_node(PE_MakeImportList *list, PE_MakeImportNode *node);
internal PE_MakeImportNode * pe_make_import_header_list_push(Arena *arena, PE_MakeImportList *list, PE_MakeImport header);

internal COFF_ObjSymbol * pe_make_indirect_jump_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *iat_symbol, String8 thunk_name);
internal COFF_ObjSymbol * pe_make_load_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *imp_addr_ptr, COFF_ObjSymbol *tail_merge, String8 func_name);
internal COFF_ObjSymbol * pe_make_tail_merge_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, String8 dll_name, String8 delay_load_helper_name, COFF_ObjSymbol *dll_import_descriptor);

internal String8 pe_make_import_dll_obj_static(Arena *arena, COFF_TimeStamp time_stmap, COFF_MachineType machine, String8 dll_name, String8 debug_symbols, PE_MakeImportList import_headers);
internal String8 pe_make_import_dll_obj_delayed(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 dll_name, String8 delay_load_helper_name, String8 debug_symbols, PE_MakeImportList import_headers, B32 emit_biat, B32 emit_uiat);

internal String8 pe_make_import_entry_obj_delayed(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);
internal String8 pe_make_null_import_descriptor_delayed(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);
internal String8 pe_make_null_thunk_data_delayed(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);

internal String8 pe_make_import_entry_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);
internal String8 pe_make_null_import_descriptor_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);
internal String8 pe_make_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols);

#endif // PE_MAKE_IMPORT_TABLE_H
