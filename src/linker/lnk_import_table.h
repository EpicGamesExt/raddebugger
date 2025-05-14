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
  LNK_Chunk             *dll_chunk;
  LNK_Chunk             *int_table_chunk;
  LNK_Chunk             *ilt_table_chunk;
  LNK_Chunk             *iat_table_chunk;
  LNK_Chunk             *biat_table_chunk;
  LNK_Chunk             *uiat_table_chunk;
  LNK_Chunk             *code_table_chunk;
  LNK_Symbol            *tail_merge_symbol;
  String8                name;
  COFF_MachineType       machine;
  HashTable             *func_ht;
} LNK_ImportDLL;

enum
{
  LNK_ImportTableFlag_EmitBiat = (1 << 0),
  LNK_ImportTableFlag_EmitUiat = (1 << 1),
};
typedef U32 LNK_ImportTableFlags;

typedef struct LNK_ImportTable
{
  Arena                *arena;
  COFF_MachineType      machine;
  LNK_ImportDLL        *first_dll;
  LNK_ImportDLL        *last_dll;
  LNK_Section          *data_sect;
  LNK_Section          *code_sect;
  LNK_Chunk            *dll_table_chunk;
  LNK_Chunk            *int_chunk;
  LNK_Chunk            *handle_table_chunk;
  LNK_Chunk            *iat_chunk;
  LNK_Chunk            *ilt_chunk;
  LNK_Chunk            *biat_chunk;
  LNK_Chunk            *uiat_chunk;
  LNK_Chunk            *code_chunk;
  LNK_ImportTableFlags  flags;
  HashTable            *dll_ht;
} LNK_ImportTable;

internal LNK_ImportTable * lnk_import_table_alloc_static(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, COFF_MachineType machine);
internal LNK_ImportTable * lnk_import_table_alloc_delayed(LNK_SectionTable *sectab, LNK_SymbolTable *symtab, COFF_MachineType machine, B32 is_unloadable, B32 is_bindable);
internal void              lnk_import_table_release(LNK_ImportTable **imptab);
internal LNK_ImportDLL *   lnk_import_table_push_dll_static(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, String8 dll_name, COFF_MachineType machine);
internal LNK_ImportDLL *   lnk_import_table_push_dll_delayed(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, String8 dll_name, COFF_MachineType machine);
internal LNK_ImportFunc *  lnk_import_table_push_func_static(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header);
internal LNK_ImportFunc *  lnk_import_table_push_func_delayed(LNK_ImportTable *imptab, LNK_SymbolTable *symtab, LNK_ImportDLL *dll, COFF_ParsedArchiveImportHeader *header);
internal LNK_ImportDLL *   lnk_import_table_search_dll(LNK_ImportTable *imptab, String8 name);
internal LNK_ImportFunc *  lnk_import_table_search_func(LNK_ImportDLL *dll, String8 name);

internal String8 lnk_ordinal_data_from_hint(Arena *arena, COFF_MachineType machine, U16 hint);

internal LNK_Chunk * lnk_emit_indirect_jump_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *addr_ptr);
internal LNK_Chunk * lnk_emit_load_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *imp_addr_ptr, LNK_Symbol *tail_merge);
internal LNK_Chunk * lnk_emit_tail_merge_thunk_x64(LNK_Section *sect, LNK_Chunk *parent, LNK_Symbol *dll_import_descriptor, LNK_Symbol *delay_load_helper);

internal LNK_Symbol * lnk_emit_load_thunk_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name);
internal LNK_Symbol * lnk_emit_jmp_thunk_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name);
internal LNK_Symbol * lnk_emit_tail_merge_symbol(LNK_SymbolTable *symtab, LNK_Chunk *chunk, String8 func_name);

