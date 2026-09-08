// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum T_COFF_SymbolKind
{
  T_COFF_SymbolKind_Null,
  T_COFF_SymbolKind_External,
  T_COFF_SymbolKind_ExternalFunction,
  T_COFF_SymbolKind_Static,
  T_COFF_SymbolKind_SectionDefinition,
  T_COFF_SymbolKind_Weak,
  T_COFF_SymbolKind_Absolute,
  T_COFF_SymbolKind_Undefined,
  T_COFF_SymbolKind_UndefinedFunction,
  T_COFF_SymbolKind_UndefinedSection,
  T_COFF_SymbolKind_Section,
  T_COFF_SymbolKind_Common,
} T_COFF_SymbolKind;

typedef struct T_COFF_Relocation T_COFF_Relocation;
struct T_COFF_Relocation
{
  T_COFF_Relocation *next;
  MD_Node *node;
  String8 id;
  String8 symbol_id;
  U32 offset;
  COFF_RelocType type;
};

typedef struct T_COFF_Section T_COFF_Section;
struct T_COFF_Section
{
  T_COFF_Section *next;
  MD_Node *node;
  String8 id;
  String8 name;
  String8 data;
  COFF_SectionFlags flags;
  T_COFF_Relocation *first_relocation;
  T_COFF_Relocation *last_relocation;
  U64 relocation_count;
  COFF_ObjSection *encoded;
};

typedef struct T_COFF_Symbol T_COFF_Symbol;
struct T_COFF_Symbol
{
  T_COFF_Symbol *next;
  MD_Node *node;
  String8 id;
  String8 name;
  String8 section_id;
  String8 associate_id;
  String8 fallback_id;
  U32 value;
  U32 size;
  T_COFF_SymbolKind kind;
  COFF_ComdatSelectType selection;
  COFF_WeakExtType weak_search;
  COFF_SymStorageClass storage_class;
  COFF_ObjSymbol *encoded;
};

typedef struct T_COFF_Directive T_COFF_Directive;
struct T_COFF_Directive
{
  T_COFF_Directive *next;
  MD_Node *node;
  String8 string;
};

typedef struct T_COFF_Object T_COFF_Object;
struct T_COFF_Object
{
  MD_Node *node;
  COFF_MachineType machine;
  COFF_TimeStamp timestamp;
  T_COFF_Section *first_section;
  T_COFF_Section *last_section;
  U64 section_count;
  T_COFF_Symbol *first_symbol;
  T_COFF_Symbol *last_symbol;
  U64 symbol_count;
  T_COFF_Directive *first_directive;
  T_COFF_Directive *last_directive;
  U64 directive_count;
};

typedef enum T_COFF_LibraryMemberKind
{
  T_COFF_LibraryMemberKind_Null,
  T_COFF_LibraryMemberKind_Object,
  T_COFF_LibraryMemberKind_Import,
  T_COFF_LibraryMemberKind_DllImport,
} T_COFF_LibraryMemberKind;

typedef struct T_COFF_LibraryMember T_COFF_LibraryMember;
struct T_COFF_LibraryMember
{
  T_COFF_LibraryMember *next;
  MD_Node *node;
  String8 id;
  String8 path;
  T_COFF_LibraryMemberKind kind;
  T_COFF_Object *object;
  String8 dll;
  String8 name;
  COFF_MachineType machine;
  COFF_TimeStamp timestamp;
  COFF_ImportType import_type;
  COFF_ImportByType import_by;
  U16 hint_or_ordinal;
};

typedef struct T_COFF_Library T_COFF_Library;
struct T_COFF_Library
{
  MD_Node *node;
  COFF_TimeStamp timestamp;
  U16 mode;
  B32 second_linker_member;
  T_COFF_LibraryMember *first_member;
  T_COFF_LibraryMember *last_member;
  U64 member_count;
};

typedef enum T_COFF_ModelKind
{
  T_COFF_ModelKind_Null,
  T_COFF_ModelKind_Object,
  T_COFF_ModelKind_Library,
} T_COFF_ModelKind;

typedef struct T_COFF_Model T_COFF_Model;
struct T_COFF_Model
{
  T_COFF_ModelKind kind;
  union
  {
    T_COFF_Object *object;
    T_COFF_Library *library;
  };
};

internal T_Result t_coff_validate(T_ParseContext *ctx, T_Artifact *artifact);
internal T_Result t_coff_encode(T_Context *ctx, T_Artifact *artifact);
internal T_Result t_coff_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out);
