// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_ExportParse
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
} LNK_ExportParse;

typedef struct LNK_ExportParseNode
{
  LNK_ExportParse             data;
  struct LNK_ExportParseNode *next;
} LNK_ExportParseNode;

typedef struct LNK_ExportParseList
{
  U64                  count;
  LNK_ExportParseNode *first;
  LNK_ExportParseNode *last;
} LNK_ExportParseList;

typedef struct LNK_ExportParsePtrArray
{
  U64               count;
  LNK_ExportParse **v;
} LNK_ExportParsePtrArray;

////////////////////////////////

internal B32 lnk_parse_export_directive_ex(Arena *arena, String8List directive, String8 obj_path, String8 lib_path, LNK_ExportParse *export_out);
internal B32 lnk_parse_export_directive(Arena *arena, String8 directive, String8 obj_path, String8 lib_path, LNK_ExportParse *parse_out);
internal LNK_ExportParsePtrArray lnk_array_from_export_list(Arena *arena, LNK_ExportParseList list);
internal LNK_ExportParseNode * lnk_export_parse_list_push(Arena *arena, LNK_ExportParseList *list, LNK_ExportParse data);

