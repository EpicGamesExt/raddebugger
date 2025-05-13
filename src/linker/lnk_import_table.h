// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_ImportFunc
{
  struct LNK_ImportFunc *next;
  struct LNK_Symbol     *symbol;
} LNK_ImportFunc;

typedef struct LNK_ImportDLL
{
  struct LNK_ImportDLL  *next;
  struct LNK_ImportFunc *first_func;
  struct LNK_ImportFunc *last_func;
  String8                name;
} LNK_ImportDLL;

typedef struct LNK_ImportTable
{
  Arena         *arena;
  LNK_ImportDLL *first_dll;
  LNK_ImportDLL *last_dll;
  U64            dll_count;
  HashTable     *dll_ht;
} LNK_ImportTable;

////////////////////////////////

internal LNK_ImportTable * lnk_import_table_alloc(void);
internal void              lnk_import_table_release(LNK_ImportTable **imptab_ptr);
internal LNK_ImportDLL *   lnk_import_table_push_dll(LNK_ImportTable *imptab, String8 dll_name, COFF_MachineType machine);
internal LNK_ImportDLL *   lnk_import_table_search_dll(LNK_ImportTable *imptab, String8 dll_name);
internal LNK_ImportFunc *  lnk_import_table_push_func(LNK_ImportTable *imptab, LNK_ImportDLL *dll, struct LNK_Symbol *symbol);

internal COFF_ObjSymbol * lnk_emit_indirect_jump_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *iat_symbol, String8 thunk_name);
internal COFF_ObjSymbol * lnk_emit_load_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *imp_addr_ptr, COFF_ObjSymbol *tail_merge, String8 func_name);
internal COFF_ObjSymbol * lnk_emit_tail_merge_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, String8 dll_name, COFF_ObjSymbol *dll_import_descriptor);

internal String8 lnk_build_import_entry_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine);
internal String8 lnk_build_null_import_descriptor_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine);
internal String8 lnk_build_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine);

internal void lnk_emit_dll_import_debug_symbols(COFF_ObjWriter *obj_writer, String8 dll_name);
internal String8 lnk_obj_from_import_dll_static(Arena *arena, COFF_MachineType machine, LNK_ImportDLL *dll);
internal String8 lnk_obj_from_import_dll_delayed(Arena *arena, COFF_MachineType machine, LNK_ImportDLL *dll, B32 emit_biat, B32 emit_uiat);

internal String8Array lnk_make_import_dlls_static(Arena *arena, LNK_ImportTable *imptab, COFF_MachineType machine, String8 image_name);
internal String8Array lnk_make_import_dlls_delayed(Arena *arena, LNK_ImportTable *imptab, COFF_MachineType machine, String8 image_name, B32 emit_biat, B32 emit_uiat);

