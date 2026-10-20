// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Lib LNK_Lib;
typedef struct LNK_LibList LNK_LibList;

typedef struct LNK_ExportDefinition
{
  String8          name;
  LNK_ObjSymbolRef ref;
} LNK_ExportDefinition;

typedef struct LNK_ExportCandidate
{
  struct LNK_ExportCandidate *next;
  LNK_NameTrieNode           *definition;
} LNK_ExportCandidate;

typedef struct LNK_ExportTable
{
  TP_Arena              *arena;
  COFF_MachineType        machine;
  LNK_NameTrieNode       *exact;
  LNK_NameTrieNode       *undecorated;
  LNK_NameTrieChunkList  *chunks;
} LNK_ExportTable;

typedef enum LNK_ExportMatchKind
{
  LNK_ExportMatch_None,
  LNK_ExportMatch_Definition,
  LNK_ExportMatch_ArchiveMember,
  LNK_ExportMatch_Ambiguous,
} LNK_ExportMatchKind;

typedef struct LNK_ExportMatch
{
  LNK_ExportMatchKind  kind;
  String8              canonical_name;
  LNK_ObjSymbolRef     definition;
  LNK_Lib             *lib;
  U32                  member_idx;
} LNK_ExportMatch;

internal LNK_ExportTable *lnk_export_table_init(TP_Arena *arena);
internal String8          lnk_export_undecorated_name(COFF_MachineType machine, String8 name);
internal void             lnk_export_table_insert_definition(LNK_ExportTable *table, Arena *arena, U64 worker_id, LNK_ObjSymbolRef ref, String8 name);
internal LNK_ExportMatch  lnk_export_table_match(LNK_ExportTable *table, LNK_SymbolTable *symtab, COFF_MachineType machine, String8 name, LNK_LibList *libs);
internal void             lnk_export_apply_match(Arena *arena, PE_ExportParse *exp, LNK_ExportMatch match);

