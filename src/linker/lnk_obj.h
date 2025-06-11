// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Input -------------------------------------------------------------------

typedef struct LNK_Obj
{
  String8             data;
  String8             path;
  String8             lib_path;
  U32                 input_idx;
  COFF_FileHeaderInfo header;
  U32                *comdats;
  B8                  hotpatch;
} LNK_Obj;

typedef struct LNK_ObjNode
{
  struct LNK_ObjNode *next;
  LNK_Obj             data;
} LNK_ObjNode;

typedef struct LNK_ObjList
{
  U64          count;
  LNK_ObjNode *first;
  LNK_ObjNode *last;
} LNK_ObjList;

typedef struct LNK_ObjNodeArray
{
  U64          count;
  LNK_ObjNode *v;
} LNK_ObjNodeArray;

typedef struct LNK_SymbolInputResult
{
  LNK_SymbolList weak_symbols;
  LNK_SymbolList undef_symbols;
} LNK_SymbolInputResult;

// --- Directive Parser --------------------------------------------------------

typedef struct LNK_Directive
{
  struct LNK_Directive *next;
  String8               id;
  String8List           value_list;
} LNK_Directive;

typedef struct LNK_DirectiveList
{
  U64            count;
  LNK_Directive *first;
  LNK_Directive *last;
} LNK_DirectiveList;

typedef struct LNK_DirectiveInfo
{
  LNK_DirectiveList v[LNK_CmdSwitch_Count];
} LNK_DirectiveInfo;

// --- Workers Contexts --------------------------------------------------------

typedef struct
{
  LNK_InputObj    **inputs;
  LNK_ObjNodeArray  objs;
  U64               obj_id_base;
  U32               machine;
} LNK_ObjIniter;

typedef struct
{
  LNK_SymbolTable *symtab;
  LNK_ObjNodeArray objs;
  LNK_SymbolList  *weak_lists;
  LNK_SymbolList  *undef_lists;
} LNK_InputCoffSymbolTable;

typedef struct
{
  LNK_Obj    **objs;
  String8      name;
  B32          collect_discarded;
  String8List *out_lists;
} LNK_SectionCollector;

// --- Error -------------------------------------------------------------------

internal void lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...);

// --- Input -------------------------------------------------------------------

internal LNK_Obj **            lnk_array_from_obj_list(Arena *arena, LNK_ObjList list);
internal LNK_ObjNodeArray      lnk_obj_list_push_parallel(TP_Context *tp, TP_Arena *tp_arena, LNK_ObjList *obj_list, COFF_MachineType machine, U64 input_count, LNK_InputObj **inputs);
internal LNK_SymbolInputResult lnk_input_obj_symbols(TP_Context *tp, TP_Arena *arena, LNK_SymbolTable *symtab, LNK_ObjNodeArray objs);

// --- Metadata ----------------------------------------------------------------

internal U32 lnk_obj_get_features(LNK_Obj *obj);
internal U32 lnk_obj_get_comp_id(LNK_Obj *obj);
internal U32 lnk_obj_get_vol_md(LNK_Obj *obj);

// --- Symbol & Section Helpers ------------------------------------------------

internal COFF_ParsedSymbol    lnk_parsed_symbol_from_coff(LNK_Obj *obj, void *coff_symbol);
internal COFF_ParsedSymbol    lnk_parsed_symbol_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx);
internal COFF_SectionHeader * lnk_coff_section_header_from_section_number(LNK_Obj *obj, U64 section_number);
internal B32                  lnk_is_coff_section_debug(LNK_Obj *obj, U64 sect_idx);

// --- Helpers ----------------------------------------------------------------- 

internal String8List * lnk_collect_obj_sections(TP_Context *tp, TP_Arena *arena, U64 objs_count, LNK_Obj **objs, String8 name, B32 collect_discarded);

// --- Directive Parser --------------------------------------------------------

internal void lnk_parse_msvc_linker_directive(Arena *arena, LNK_Obj *obj, LNK_DirectiveInfo *directive_info, String8 buffer);

