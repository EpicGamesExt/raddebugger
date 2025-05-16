// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef COFF_LIB_WRITER_H
#define COFF_LIB_WRITER_H

typedef struct COFF_LibWriterMember
{
  String8 name;
  String8 data;
} COFF_LibWriterMember;

typedef struct COFF_LibWriterMemberNode
{
  COFF_LibWriterMember             data;
  struct COFF_LibWriterMemberNode *next;
} COFF_LibWriterMemberNode;

typedef struct COFF_LibWriterMemberList
{
  U64                       count;
  COFF_LibWriterMemberNode *first;
  COFF_LibWriterMemberNode *last;
} COFF_LibWriterMemberList;

typedef struct COFF_LibWriterSymbol
{
  String8 name;
  U64     member_idx;
} COFF_LibWriterSymbol;

typedef struct COFF_LibWriterSymbolNode
{
  COFF_LibWriterSymbol             data;
  struct COFF_LibWriterSymbolNode *next;
} COFF_LibWriterSymbolNode;

typedef struct COFF_LibWriterSymbolList
{
  U64                       count;
  COFF_LibWriterSymbolNode *first;
  COFF_LibWriterSymbolNode *last;
} COFF_LibWriterSymbolList;

typedef struct COFF_LibWriter
{
  Arena                   *arena;
  COFF_LibWriterMemberList member_list;
  COFF_LibWriterSymbolList symbol_list;
} COFF_LibWriter;

////////////////////////////////

internal COFF_LibWriterSymbolNode * coff_lib_writer_symbol_list_push(Arena *arena, COFF_LibWriterSymbolList *list, COFF_LibWriterSymbol symbol);
internal COFF_LibWriterMemberNode * coff_lib_writer_member_list_push(Arena *arena, COFF_LibWriterMemberList *list, COFF_LibWriterMember member);

internal COFF_LibWriterSymbol * coff_lib_writer_symbol_array_from_list(Arena *arena, COFF_LibWriterSymbolList list);
internal COFF_LibWriterMember * coff_lib_writer_member_array_from_list(Arena *arena, COFF_LibWriterMemberList list);

internal void coff_lib_writer_symbol_array_sort(COFF_LibWriterSymbol *arr, U64 count);

internal COFF_LibWriter * coff_lib_writer_alloc(void);
internal void             coff_lib_writer_release(COFF_LibWriter **writer_ptr);
internal U64              coff_lib_writer_push_obj(COFF_LibWriter *writer, String8 obj_path, String8 obj_data);
internal void             coff_lib_writer_push_import(COFF_LibWriter *lib_writer, COFF_MachineType machine, COFF_TimeStamp time_stamp, String8 dll_name, COFF_ImportByType import_by, String8 name, U16 hint_or_ordinal, COFF_ImportType import_type);
internal String8List      coff_lib_writer_serialize(Arena *arena, COFF_LibWriter *lib_writer, COFF_TimeStamp time_stamp, U16 mode, B32 emit_second_member);

#endif // COFF_LIB_WRITER_H

