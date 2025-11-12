// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_PARSE_2_H
#define DWARF_PARSE_2_H

////////////////////////////////
//~ rjf: Unit Headers

typedef struct DW2_UnitHeader DW2_UnitHeader;
struct DW2_UnitHeader
{
  DW_Version version;
  DW_Format format;
  U64 abbrev_off;
  U64 addr_size;
  DW_CompUnitKind kind;
  U64 dwo_id;
};

////////////////////////////////
//~ rjf: Abbreviation Map (ID -> .debug_abbrev Offset)

typedef struct DW2_AbbrevMapNode DW2_AbbrevMapNode;
struct DW2_AbbrevMapNode
{
  DW2_AbbrevMapNode *next;
  U64 id;
  U64 off;
};

typedef struct DW2_AbbrevMap DW2_AbbrevMap;
struct DW2_AbbrevMap
{
  DW2_AbbrevMapNode **slots;
  U64 slots_count;
};

////////////////////////////////
//~ rjf: Parsing Context Bundle

typedef struct DW2_ParseCtx DW2_ParseCtx;
struct DW2_ParseCtx
{
  DW_Raw *raw;
  // NOTE(rjf): all subsequent fields optional - top-level code fills out whatever
  // it can from context.
  DW_Version version;
  DW_Format format;
  U64 addr_size;
  DW2_AbbrevMap *abbrev_map;
  String8 unit_dir;
  String8 unit_file;
};

////////////////////////////////
//~ rjf: Tag Attributes

typedef struct DW2_FormVal DW2_FormVal;
struct DW2_FormVal
{
  DW_FormKind kind;
  String8 string;
  U128 u128;
};

typedef struct DW2_Attrib DW2_Attrib;
struct DW2_Attrib
{
  DW_AttribKind attrib_kind;
  DW2_FormVal val;
};

typedef struct DW2_AttribNode DW2_AttribNode;
struct DW2_AttribNode
{
  DW2_AttribNode *next;
  DW2_Attrib v;
};

typedef struct DW2_AttribList DW2_AttribList;
struct DW2_AttribList
{
  DW2_AttribNode *first;
  DW2_AttribNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Tags

typedef struct DW2_Tag DW2_Tag;
struct DW2_Tag
{
  DW_TagKind kind;
  B32 has_children;
  DW2_AttribList attribs;
};

////////////////////////////////
//~ rjf: Line Info

typedef struct DW2_LineTableFile DW2_LineTableFile;
struct DW2_LineTableFile
{
  String8 file_name;
  U64 dir_idx;
  U64 modify_time;
  U64 file_size;
  MD5 md5;
  String8 source;
};

typedef struct DW2_LineTableFileNode DW2_LineTableFileNode;
struct DW2_LineTableFileNode
{
  DW2_LineTableFileNode *next;
  DW2_LineTableFile v;
};

typedef struct DW2_LineTableFileList DW2_LineTableFileList;
struct DW2_LineTableFileList
{
  DW2_LineTableFileNode *first;
  DW2_LineTableFileNode *last;
  U64 count;
};

typedef struct DW2_LineTableFileArray DW2_LineTableFileArray;
struct DW2_LineTableFileArray
{
  DW2_LineTableFile *v;
  U64 count;
};

typedef struct DW2_LineTableHeader DW2_LineTableHeader;
struct DW2_LineTableHeader
{
  U64 unit_length;
  DW_Format format;
  DW_Version version;
  U8 addr_size;
  U8 segment_selector_size;
  U64 header_length;
  U8 min_inst_length;
  U8 max_ops_per_inst;
  U8 default_is_stmt;
  S8 line_base;
  U8 line_range;
  U8 opcode_base;
  U64 opcode_lengths_count;
  U8 *opcode_lengths;
  DW2_LineTableFileArray dirs;
  DW2_LineTableFileArray files;
};

////////////////////////////////
//~ rjf: Globals

global read_only DW2_Attrib dw2_attrib_nil = {0};

////////////////////////////////
//~ rjf: Basic Parsing Helpers

internal U64 dw2_read_initial_length(String8 data, U64 off, U64 *out, DW_Format *fmt_out);
internal U64 dw2_read_fmt_u64(String8 data, U64 off, DW_Format format, U64 *out);

////////////////////////////////
//~ rjf: Unit Header Parsing

internal U64 dw2_read_unit_header(String8 data, U64 off, DW2_UnitHeader *out);

////////////////////////////////
//~ rjf: Abbreviation Map Parsing

internal DW2_AbbrevMap dw2_abbrev_map_from_data(Arena *arena, String8 data, U64 off);

////////////////////////////////
//~ rjf: Form Value Parsing

internal U64 dw2_read_form_val(DW2_ParseCtx *ctx, String8 data, U64 off, DW_FormKind form_kind, U64 implicit_const, DW2_FormVal *out);

////////////////////////////////
//~ rjf: Tag Parsing

internal U64 dw2_read_tag(Arena *arena, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out);
internal DW2_Attrib *dw2_attrib_from_kind(DW2_Tag *tag, DW_AttribKind kind);

////////////////////////////////
//~ rjf: Line Table Parsing

internal U64 dw2_read_line_table_header(Arena *arena, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_LineTableHeader *out);

#endif // DWARF_PARSE_2_H
