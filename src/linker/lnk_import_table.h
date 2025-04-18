// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define LNK_IMPORT_DLL_HASH_TABLE_BUCKET_COUNT  512
#define LNK_IMPORT_FUNC_HASH_TABLE_BUCKET_COUNT 2048

typedef struct LNK_ImportFunc
{
  struct LNK_ImportFunc *next;
  String8                name;
  String8                thunk_symbol_name;
  String8                iat_symbol_name;
} LNK_ImportFunc;

typedef struct LNK_ImportDLL
{
  struct LNK_ImportDLL  *next;
  struct LNK_ImportFunc *first_func;
  struct LNK_ImportFunc *last_func;
  COFF_ObjWriter        *obj_writer;
  COFF_ObjSection       *dll_sect;
  COFF_ObjSection       *int_sect;
  COFF_ObjSection       *ilt_sect;
  COFF_ObjSection       *iat_sect;
  COFF_ObjSection       *biat_sect;
  COFF_ObjSection       *uiat_sect;
  COFF_ObjSection       *code_sect;
  COFF_ObjSymbol        *tail_merge_symbol;
  String8                name;
  HashTable             *func_ht;
} LNK_ImportDLL;

enum
{
  LNK_ImportTableFlag_Delayed  = (1 << 0),
  LNK_ImportTableFlag_EmitBiat = (1 << 1),
  LNK_ImportTableFlag_EmitUiat = (1 << 2),
};
typedef U32 LNK_ImportTableFlags;

typedef struct LNK_ImportTable
{
  Arena                *arena;
  COFF_MachineType      machine;
  LNK_ImportDLL        *first_dll;
  LNK_ImportDLL        *last_dll;
  LNK_ImportTableFlags  flags;
  HashTable            *dll_ht;
} LNK_ImportTable;

////////////////////////////////

internal LNK_ImportTable * lnk_import_table_alloc(LNK_ImportTableFlags flags);
internal void              lnk_import_table_release(LNK_ImportTable **imptab);
internal LNK_ImportDLL *   lnk_import_table_push_dll_static(LNK_ImportTable *imptab, String8 dll_name, COFF_MachineType machine);
internal LNK_ImportDLL *   lnk_import_table_push_dll_delayed(LNK_ImportTable *imptab, String8 dll_name, COFF_MachineType machine);
internal LNK_ImportDLL *   lnk_import_table_search_dll(LNK_ImportTable *imptab, String8 name);

internal LNK_ImportFunc *  lnk_import_table_push_func_static(LNK_ImportTable *imptab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header);
internal LNK_ImportFunc *  lnk_import_table_push_func_delayed(LNK_ImportTable *imptab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header);
internal LNK_ImportFunc *  lnk_import_table_search_func(LNK_ImportDLL *dll, String8 name);

internal COFF_ObjSymbol * lnk_emit_indirect_jump_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *iat_symbol, String8 thunk_name);
internal COFF_ObjSymbol * lnk_emit_load_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *imp_addr_ptr, COFF_ObjSymbol *tail_merge, String8 func_name);
internal COFF_ObjSymbol * lnk_emit_tail_merge_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, String8 dll_name, COFF_ObjSymbol *dll_import_desc);

internal String8 lnk_build_import_entry_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine);
internal String8 lnk_build_null_import_descriptor_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine);
internal String8 lnk_build_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine);

internal LNK_InputObjList lnk_import_table_serialize(Arena *arena, LNK_ImportTable *imptab, String8 image_name, COFF_MachineType machine);
