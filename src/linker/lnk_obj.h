// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Input -------------------------------------------------------------------

typedef struct LNK_Obj
{
  String8                  path;
  String8                  data;
  struct LNK_Lib          *lib;
  struct LNK_LibMemberRef *trigger_symbol;
  U32                      input_idx;
  COFF_FileHeaderInfo      header;
  U32                     *comdats;
  B8                       hotpatch;
  B8                       exclude_from_debug_info;
  U32Node                **associated_sections;
  LNK_SymbolHashTrie     **symlinks;

  struct LNK_ObjNode *node;
} LNK_Obj;

typedef struct LNK_ObjNode
{
  struct LNK_ObjNode *next;
  struct LNK_ObjNode *prev;
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
  struct LNK_Input **inputs;
  LNK_ObjNode       *objs;
  U64                obj_id_base;
  U32                machine;
} LNK_ObjIniter;

typedef struct
{
  LNK_SymbolTable  *symtab;
  LNK_Obj         **objs;
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
internal void lnk_error_input_obj(LNK_ErrorCode code, struct LNK_Input *input, char *fmt, ...);

// --- Input -------------------------------------------------------------------

internal LNK_Obj ** lnk_array_from_obj_list(Arena *arena, LNK_ObjList list);
internal void       lnk_obj_list_push_node_many(LNK_ObjList *list, U64 count, LNK_ObjNode *nodes);
internal void       lnk_obj_list_push_node(LNK_ObjList *list, LNK_ObjNode *node);

internal void       lnk_inputer_push_obj_symbols(TP_Context *tp, TP_Arena *arena, LNK_SymbolTable *symtab, U64 objs_count, LNK_ObjNode *objs);

// --- Metadata ----------------------------------------------------------------

internal U32          lnk_obj_get_features(LNK_Obj *obj);
internal U32          lnk_obj_get_comp_id(LNK_Obj *obj);
internal U32          lnk_obj_get_vol_md(LNK_Obj *obj);
internal String8      lnk_obj_get_lib_path(LNK_Obj *obj);
internal U32          lnk_obj_get_removed_section_number(LNK_Obj *obj);
internal LNK_Symbol * lnk_obj_get_comdat_symlink(LNK_Obj *obj, U64 section_number);

// --- Symbol & Section Helpers ------------------------------------------------

internal COFF_ParsedSymbol    lnk_parsed_symbol_from_coff(LNK_Obj *obj, void *coff_symbol);
internal COFF_ParsedSymbol    lnk_parsed_symbol_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx);
internal COFF_SectionHeader * lnk_coff_section_header_from_section_number(LNK_Obj *obj, U64 section_number);
internal COFF_SectionHeader * lnk_coff_section_table_from_obj(LNK_Obj *obj);
internal B32                  lnk_try_comdat_props_from_section_number(LNK_Obj *obj, U32 section_number, COFF_ComdatSelectType *select_out, U32 *section_number_out, U32 *section_length_out, U32 *check_sum_out);
internal B32                  lnk_is_coff_section_debug(LNK_Obj *obj, U64 sect_idx);

// --- Helpers ----------------------------------------------------------------- 

internal String8List * lnk_collect_obj_sections(TP_Context *tp, TP_Arena *arena, U64 objs_count, LNK_Obj **objs, String8 name, B32 collect_discarded);
internal B32           lnk_obj_is_before(void *raw_a, void *raw_b);

// --- Directive Parser --------------------------------------------------------

internal void              lnk_parse_msvc_linker_directive(Arena *arena, LNK_Obj *obj, LNK_DirectiveInfo *directive_info, String8 buffer);
internal String8List       lnk_raw_directives_from_obj(Arena *arena, LNK_Obj *obj);
internal LNK_DirectiveInfo lnk_directive_info_from_raw_directives(Arena *arena, LNK_Obj *obj, String8List raw_directives);

