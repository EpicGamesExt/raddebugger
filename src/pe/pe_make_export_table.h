// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef PE_MAKE_EXPORT_TABLE_H
#define PE_MAKE_EXPORT_TABLE_H

typedef struct PE_ExportParse
{
  String8         obj_path;
  String8         lib_path;
  String8         name;
  String8         alias;
  COFF_ImportType type;
  U16             ordinal;
  U16             hint;
  B32             is_ordinal_assigned;
  B32             is_noname_present;
  B32             is_private;
  B32             is_forwarder;
} PE_ExportParse;

typedef struct PE_ExportParseNode
{
  PE_ExportParse             data;
  struct PE_ExportParseNode *next;
} PE_ExportParseNode;

typedef struct PE_ExportParseList
{
  U64                  count;
  PE_ExportParseNode *first;
  PE_ExportParseNode *last;
} PE_ExportParseList;

typedef struct PE_ExportParsePtrArray
{
  U64               count;
  PE_ExportParse **v;
} PE_ExportParsePtrArray;

typedef struct PE_FinalizedExports
{
  U64 ordinal_low;
  union {
    struct {
      PE_ExportParsePtrArray named_exports;
      PE_ExportParsePtrArray forwarder_exports;
      PE_ExportParsePtrArray ordinal_exports;
    };
    PE_ExportParsePtrArray exports_with_names[2];
    PE_ExportParsePtrArray all[3];
  };
} PE_FinalizedExports;

////////////////////////////////

internal PE_ExportParsePtrArray pe_array_from_export_list(Arena *arena, PE_ExportParseList list);
internal PE_ExportParseNode * pe_export_parse_list_push(Arena *arena, PE_ExportParseList *list, PE_ExportParse data);
internal String8List pe_make_import_lib(Arena *arena, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, String8 debug_symbols, PE_ExportParseList export_list);

#endif // COFF_EXPORT_TABLE_H
