// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OBJ_H
#define OBJ_H

typedef enum
{
  OBJ_RefKind_Null,
  OBJ_RefKind_Section,
} OBJ_RefKind;

typedef enum
{
  OBJ_SymbolScope_Null,
  OBJ_SymbolScope_Local,
  OBJ_SymbolScope_Global,
} OBJ_SymbolScope;

typedef struct OBJ_Symbol
{
  String8          name;
  OBJ_SymbolScope  scope;
  OBJ_RefKind      ref_kind;
  void            *ref;
} OBJ_Symbol;

typedef struct OBJ_SymbolNode
{
  OBJ_Symbol             v;
  struct OBJ_SymbolNode *next;
} OBJ_SymbolNode;

typedef struct OBJ_SymbolList
{
  U64             count;
  OBJ_SymbolNode *first;
  OBJ_SymbolNode *last;
} OBJ_SymbolList;

typedef enum
{
  OBJ_RelocKind_Null,
  OBJ_RelocKind_SecRel,
} OBJ_RelocKind;

typedef struct OBJ_Reloc
{
  OBJ_RelocKind  kind;
  U64            offset;
  OBJ_Symbol    *symbol;
} OBJ_Reloc;

typedef struct OBJ_RelocNode
{
  OBJ_Reloc             v;
  struct OBJ_RelocNode *next;
} OBJ_RelocNode;

typedef struct OBJ_RelocList
{
  U64            count;
  OBJ_RelocNode *first;
  OBJ_RelocNode *last;
} OBJ_RelocList;

typedef enum OBJ_SectionFlags
{
  OBJ_SectionFlag_Read  = (1 << 0),
  OBJ_SectionFlag_Write = (1 << 1),
  OBJ_SectionFlag_Exec  = (1 << 2),
  OBJ_SectionFlag_Load  = (1 << 3),
} OBJ_SectionFlags;

typedef struct OBJ_Section
{
  String8          name;
  String8List      data;
  OBJ_SectionFlags flags;
  OBJ_RelocList    relocs;
  OBJ_Symbol      *symbol;
} OBJ_Section;

typedef struct OBJ_SectionNode
{
  OBJ_Section             v;
  struct OBJ_SectionNode *next;
} OBJ_SectionNode;

typedef struct OBJ_SectionList
{
  U64              count;
  OBJ_SectionNode *first;
  OBJ_SectionNode *last;
} OBJ_SectionList;

typedef struct OBJ
{
  Arena          *arena;
  U64             time_stamp;
  Arch            arch;
  OBJ_SectionList sections;
  OBJ_SymbolList  symbols;
} OBJ;

////////////////////////////////

internal OBJ *         obj_alloc(U64 time_stamp, Arch arch);
internal void          obj_release(OBJ **obj_ptr);
internal OBJ_Symbol *  obj_push_symbol(OBJ *obj, String8 name, OBJ_SymbolScope scope, OBJ_RefKind ref_kind, void *ref); 
internal OBJ_Section * obj_push_section(OBJ *obj, String8 name, OBJ_SectionFlags flags);
internal OBJ_Reloc *   obj_push_reloc(OBJ *obj, OBJ_Section *sec, OBJ_RelocKind kind, U64 offset, OBJ_Symbol *symbol);

#endif // OBJ_H

