// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Export
{
  struct LNK_Export  *next;
  String8             name;
  LNK_Symbol         *symbol;
  U32                 id;
  U16                 ordinal;
  COFF_ImportType     type;
  B32                 is_private;
} LNK_Export;

typedef struct LNK_ExportList
{
  U64         count;
  LNK_Export *first;
  LNK_Export *last;
} LNK_ExportList;

typedef struct LNK_ExportArray
{
  U64         count;
  LNK_Export *v;
} LNK_ExportArray;

typedef struct LNK_ExportTable
{
  Arena         *arena;
  HashTable     *name_export_ht;
  HashTable     *noname_export_ht;
  U64            voff_size;
  U64            max_ordinal;
  B8            *is_ordinal_used;
} LNK_ExportTable;

internal LNK_ExportTable * lnk_export_table_alloc(void);
internal void              lnk_export_table_release(LNK_ExportTable **exptab_ptr);
internal LNK_Export *      lnk_export_table_search(LNK_ExportTable *exptab, String8 name);
internal void              lnk_collect_exports_from_def_files(LNK_ExportTable *exptab, String8List path_list);
internal void              lnk_build_edata(LNK_ExportTable *exptab, LNK_SectionTable *sectab, LNK_SymbolTable *symtab, String8 image_name, COFF_MachineType machine);
internal void lnk_collect_exports_from_obj_directives(LNK_ExportTable *exptab, LNK_ObjList obj_list, LNK_SymbolTable *symtab);

