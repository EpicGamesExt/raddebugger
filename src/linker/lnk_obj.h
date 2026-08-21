// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// --- Input -------------------------------------------------------------------

typedef struct LNK_ObjSymbolArray
{
  U64                   count;
  U32Array               primary_bits;
  U32                  *block_bases;
  U64                  *values;
  U32                  *section_numbers;
  COFF_SymbolType      *types;
  COFF_SymStorageClass *storage_classes;
  U8                   *aux_counts;
  U32                  *name_offsets;
  U32                  *name_sizes;
} LNK_ObjSymbolArray;

typedef struct LNK_ObjSectionArray
{
  // Parallel arrays are indexed by COFF section number; slot zero is null.
  U64                 count_no_null;
  COFF_SectionHeader *headers;
  U32                *comdats;
  U32Node           **associated_section_numbers;
} LNK_ObjSectionArray;

typedef struct LNK_ObjCoff
{
  String8             data;
  COFF_FileHeaderInfo header;
  LNK_ObjSectionArray sections;
  LNK_ObjSymbolArray  symbols;
  U32                 debug_t_section_number;
  U32                 debug_p_section_number;
  U32                 debug_h_section_number;
  U32                 llvm_addrsig_section_number;
  B8                  hotpatch;
} LNK_ObjCoff;

typedef struct LNK_Obj
{
  String8 path;

  LNK_ObjCoff coff;

  // flags
  B8 exclude_from_debug_info;

  U32 input_idx;

  // link state
  LNK_ObjSymbolRef  *symlinks; // indexed by COFF section number; slot zero is null

  // link
  struct LNK_LibMemberRef *link_member;
  struct LNK_ObjNode      *self;

  // @type_server
  Rng1U64         ti_range;
  CV_TypeIndex   *ti_map;
  Rng1U64         pch_ti_range;
  U64             pch_obj_idx;
} LNK_Obj;

typedef struct LNK_ObjSection
{
  LNK_Obj            *obj;
  U64                 section_number;
  COFF_SectionHeader *header;
  COFF_SectionFlags  *flags;
  Rng1U64             vrange;
  Rng1U64             frange;
  U32                 reloc_count;
} LNK_ObjSection;


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

// --- Iterators ---------------------------------------------------------------

typedef struct LNK_ObjSectionIter
{
  LNK_ObjSection v;
} LNK_ObjSectionIter;

typedef struct LNK_ObjSymbolIter
{
  U64               next_symbol_idx;
  U64               next_primary_idx;
  U64               symbol_idx;
  U64               primary_idx;
  COFF_ParsedSymbol v;
} LNK_ObjSymbolIter;

#define LNK_EachCoffSection(it, obj) (LNK_ObjSectionIter it = {0}; lnk_obj_section_iter_next((obj), &(it));)
#define LNK_EachCoffSymbol(it, obj)  (LNK_ObjSymbolIter  it = {0}; lnk_obj_symbol_iter_next((obj), &(it));)

// --- Maps --------------------------------------------------------------------

typedef struct LNK_SectionOffsetSymbol
{
  U64 key;
  U32 symbol_idx;
} LNK_SectionOffsetSymbol;

typedef struct LNK_ObjSymbolMap
{
  U64                      count;
  LNK_SectionOffsetSymbol *v;
} LNK_ObjSymbolMap;

typedef struct LNK_LineTableBlock
{
  String8            raw_lines;
  CV_C13LinesHeader *header;
} LNK_LineTableBlock;

typedef struct LNK_LineTable
{
  U64            key_min;
  U64            key_max;
  U64            prefix_max;
  U64            block_first;
  U64            block_count;
  CV_LinesAccel *accel;
} LNK_LineTable;

typedef struct LNK_InlineSite
{
  U32        parent_idx_plus_one;
  U32        depth;
  CV_ItemId  inlinee;
  String8    name;
  U64        callee_count;
  String8   *callee_names;
  U64        line_count;
  CV_Line   *lines;
} LNK_InlineSite;

typedef struct LNK_InlineRange
{
  U64 key_min;
  U64 key_max;
  U64 prefix_max;
  U32 site_idx;
} LNK_InlineRange;

typedef struct LNK_ObjLineMap
{
  Arena              *arena;
  String8             debug_checksums;
  String8             debug_strings;
  U64                 line_table_block_count;
  LNK_LineTableBlock *line_table_blocks;
  U64                 table_count;
  LNK_LineTable      *tables;
  U64                 inline_site_count;
  LNK_InlineSite     *inline_sites;
  U64                 inline_range_count;
  LNK_InlineRange    *inline_ranges;
} LNK_ObjLineMap;

// --- Directive Parser --------------------------------------------------------

typedef struct LNK_Directive
{
  struct LNK_Directive *next;
  String8               id;
  String8               value;
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

internal String8 lnk_loc_from_obj(Arena *arena, LNK_Obj *obj);
internal void lnk_error_obj(LNK_ErrorCode code, LNK_Obj *obj, char *fmt, ...);
internal void lnk_error_input_obj(LNK_ErrorCode code, struct LNK_Input *input, char *fmt, ...);

// --- Input -------------------------------------------------------------------

internal LNK_Obj ** lnk_array_from_obj_list(Arena *arena, LNK_ObjList list);
internal void       lnk_obj_list_push_node_many(LNK_ObjList *list, U64 count, LNK_ObjNode *nodes);
internal void       lnk_obj_list_push_node(LNK_ObjList *list, LNK_ObjNode *node);

// --- Metadata ----------------------------------------------------------------

internal U32              lnk_obj_get_features(LNK_Obj *obj);
internal struct LNK_Lib * lnk_obj_get_lib(LNK_Obj *obj);
internal String8          lnk_obj_get_lib_path(LNK_Obj *obj);
internal U32              lnk_obj_get_removed_section_number(LNK_Obj *obj);
internal B32              lnk_obj_get_comdat_symlink_from_section_number(LNK_Obj *obj, U64 section_number, LNK_ObjSymbolRef *symlink_out);
internal U32List          lnk_obj_collect_associated_section_numbers(Arena *arena, LNK_Obj *obj, U32 root_section_number, COFF_SectionFlags skip_flags);

// --- Symbol & Section Helpers ------------------------------------------------

internal force_inline COFF_ParsedSymbol lnk_parsed_symbol_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx);
internal force_inline COFF_ParsedSymbol lnk_parsed_symbol_from_coff_symbol_idx_no_name(LNK_Obj *obj, U64 symbol_idx);
internal force_inline String8           lnk_symbol_name_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx);
internal force_inline U64               lnk_obj_primary_symbol_idx_from_coff_symbol_idx(LNK_Obj *obj, U64 symbol_idx);

internal COFF_SectionHeader * lnk_coff_section_header_from_section_number(LNK_Obj *obj, U64 section_number);
internal String8              lnk_obj_section_data_from_number(LNK_Obj *obj, U64 section_number);
internal U64                  lnk_obj_foff_from_section_data_ptr(LNK_Obj *obj, void *ptr);
internal String8              lnk_obj_section_name_from_section_number(LNK_Obj *obj, U64 section_number);
internal LNK_ObjSection       lnk_obj_section_from_section_number(LNK_Obj *obj, U64 section_number);
internal COFF_RelocArray      lnk_coff_relocs_from_section_header(LNK_Obj *obj, COFF_SectionHeader *section_header);
internal String8              lnk_coff_string_table_from_obj(LNK_Obj *obj);
internal String8              lnk_coff_symbol_table_from_obj(LNK_Obj *obj);
internal B32                  lnk_try_comdat_props_from_section_number(LNK_Obj *obj, U32 section_number, COFF_ComdatSelectType *select_out, U32 *section_number_out, U32 *section_length_out, U32 *check_sum_out);

internal force_inline B32 lnk_obj_section_iter_next(LNK_Obj *obj, LNK_ObjSectionIter *it);
internal force_inline B32 lnk_obj_symbol_iter_next(LNK_Obj *obj, LNK_ObjSymbolIter *it);

// --- Helpers ----------------------------------------------------------------- 

internal String8List * lnk_collect_obj_sections(TP_Context *tp, TP_Arena *arena, U64 objs_count, LNK_Obj **objs, String8 name, B32 collect_discarded);
internal B32           lnk_obj_is_before(void *raw_a, void *raw_b);

// --- Directive Parser --------------------------------------------------------

internal void              lnk_parse_msvc_linker_directive(Arena *arena, LNK_Obj *obj, LNK_DirectiveInfo *directive_info, String8 buffer);
internal String8List       lnk_raw_directives_from_obj(Arena *arena, LNK_Obj *obj);
internal LNK_DirectiveInfo lnk_directive_info_from_raw_directives(Arena *arena, LNK_Obj *obj, String8List raw_directives);

// --- Maps ---------------------------------------------------------------------

// COFF map
internal LNK_ObjSymbolMap * lnk_symbol_map_from_obj(Arena *arena, LNK_Obj *obj);
internal U32                lnk_symbol_from_section_offset(LNK_ObjSymbolMap *map, U32 section_number, U32 offset);

// CodeView map
internal LNK_ObjLineMap * lnk_line_map_from_obj(Arena *arena, LNK_Obj *obj);
internal CV_Line *        lnk_lines_from_section_offset(LNK_ObjLineMap *map, U64 section_number, U64 offset, U64 *line_count_out);
internal LNK_InlineSite * lnk_inline_site_from_section_offset(LNK_ObjLineMap *map, U64 section_number, U64 offset, LNK_Symbol *callee_symbol);
internal CV_Line *        lnk_line_from_inline_site(LNK_InlineSite *site, U32 offset);

