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
//~ rjf: Tag Parsing Context

typedef struct DW2_TagParseCtx DW2_TagParseCtx;
struct DW2_TagParseCtx
{
  DW_Version version;
  DW_Format format;
  U64 addr_size;
  String8 abbrev_data;
  DW2_AbbrevMap *abbrev_map;
};

////////////////////////////////
//~ rjf: Tag Attributes

typedef struct DW2_AttribVal DW2_AttribVal;
struct DW2_AttribVal
{
  DW_SectionKind section_kind;
  U128 u128;
};

typedef struct DW2_Attrib DW2_Attrib;
struct DW2_Attrib
{
  DW_AttribKind attrib_kind;
  DW_FormKind form_kind;
  DW2_AttribVal val;
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
//~ rjf: Tag Parsing

internal U64 dw2_read_tag(Arena *arena, DW2_TagParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out);

#endif // DWARF_PARSE_2_H
