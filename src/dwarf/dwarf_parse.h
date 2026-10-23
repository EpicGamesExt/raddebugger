// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_PARSE_H
#define DWARF_PARSE_H

////////////////////////////////
//~ rjf: Raw Section Data

typedef struct DW_Section DW_Section;
struct DW_Section
{
  String8 name;
  String8 data;
};

typedef struct DW_Raw DW_Raw;
struct DW_Raw
{
  DW_Section sec[DW_SectionKind_COUNT];
  DW_Section sup[DW_SectionKind_COUNT];
};

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

typedef struct DW2_AbbrevAttrib DW2_AbbrevAttrib;
struct DW2_AbbrevAttrib
{
  DW_AttribKind attrib_kind;
  DW_FormKind form_kind;
  U64 implicit_const;
};

typedef struct DW2_AbbrevAttribNode DW2_AbbrevAttribNode;
struct DW2_AbbrevAttribNode
{
  DW2_AbbrevAttribNode *next;
  DW2_AbbrevAttrib v;
};

typedef struct DW2_AbbrevAttribList DW2_AbbrevAttribList;
struct DW2_AbbrevAttribList
{
  DW2_AbbrevAttribNode *first;
  DW2_AbbrevAttribNode *last;
  U64 count;
};

typedef struct DW2_AbbrevHeader DW2_AbbrevHeader;
struct DW2_AbbrevHeader
{
  U64 id;
  DW_TagKind tag_kind;
  B8 has_children;
};

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

typedef struct DW2_UnitAbbrevMapMap DW2_UnitAbbrevMapMap;
struct DW2_UnitAbbrevMapMap
{
  U64 map_count;
  DW2_AbbrevMap *map_from_idx_table;
  DW2_AbbrevMap **map_from_unit_idx_table;
};

////////////////////////////////
//~ rjf: Offset Tables (.debug_str_offsets, .debug_rnglists)

typedef struct DW2_OffsetTable DW2_OffsetTable;
struct DW2_OffsetTable
{
  DW_Format format;
  DW_Version version;
  U8 addr_size;
  U8 segment_selector_size;
  U64 entry_size;
  U64 entries_count;
  B32 entries_are_addrs;
  void *entries;
};

typedef struct DW2_OffsetTableNode DW2_OffsetTableNode;
struct DW2_OffsetTableNode
{
  DW2_OffsetTableNode *next;
  DW2_OffsetTable v;
  Rng1U64 range;
};

typedef struct DW2_OffsetTableList DW2_OffsetTableList;
struct DW2_OffsetTableList
{
  DW2_OffsetTableNode *first;
  DW2_OffsetTableNode *last;
  U64 count;
};

typedef struct DW2_OffsetTableSet DW2_OffsetTableSet;
struct DW2_OffsetTableSet
{
  // rjf: .debug_str_offsets
  U64 str_offsets_tables_count;
  DW2_OffsetTable *str_offsets_tables;
  Rng1U64 *str_offsets_tables_ranges;
  
  // rjf: .debug_rnglists
  U64 rnglists_tables_count;
  DW2_OffsetTable *rnglists_tables;
  Rng1U64 *rnglists_tables_ranges;
  
  // rjf: .debug_addr
  U64 addr_tables_count;
  DW2_OffsetTable *addr_tables;
  Rng1U64 *addr_tables_ranges;
  
  // rjf: .debug_loclists
  U64 loclists_tables_count;
  DW2_OffsetTable *loclists_tables;
  Rng1U64 *loclists_tables_ranges;
};

////////////////////////////////
//~ rjf: Parsing Context Bundle

typedef struct DW2_ParseCtx DW2_ParseCtx;
struct DW2_ParseCtx
{
  DW_Version version;
  DW_Format format;
  DW_ExtFlags exts;
  U64 addr_size;
  U64 unit_base_addr;
  U64 unit_base_info_off;
  DW2_AbbrevMap *abbrev_map;
  DW2_OffsetTable *rnglists_table;
  DW2_OffsetTable *str_offsets_table;
  DW2_OffsetTable *addr_table;
  DW2_OffsetTable *loclists_table;
  String8 unit_dir;
  String8 unit_file;
  DW_Language language;
};

////////////////////////////////
//~ rjf: Tag Attributes

typedef struct DW2_FormVal DW2_FormVal;
struct DW2_FormVal
{
  DW_FormKind kind;
  U128 u128;
  String8 string;
  U64 addr;
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

typedef U32 DW2_LineTableFileFlags;
enum
{
  DW2_LineTableFileFlag_HasMD5        = (1<<0),
  DW2_LineTableFileFlag_HasModifyTime = (1<<1),
};

typedef struct DW2_LineTableFile DW2_LineTableFile;
struct DW2_LineTableFile
{
  String8 file_name;
  DW2_LineTableFileFlags flags;
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
  U64 line_program_off;
  U64 total_unit_data_size; // NOTE(rjf): would be implied by `unit_length`, but DWARF makes that the size *past* the variable-width unit-length field itself
};

typedef struct DW2_LineVMRegs DW2_LineVMRegs;
struct DW2_LineVMRegs
{
  U64 address;
  U64 vliw_op_index;
  U64 file_index;
  U64 line;
  U64 column;
  B32 is_stmt;
  B32 basic_block;
  B32 end_sequence;
  B32 prologue_end;
  B32 epilogue_begin;
  U64 isa;
  U64 discriminator;
};

////////////////////////////////
//~ rjf: Location Lists

typedef struct DW2_Loc DW2_Loc;
struct DW2_Loc
{
  Rng1U64 range;
  String8 expr;
};

typedef struct DW2_LocNode DW2_LocNode;
struct DW2_LocNode
{
  DW2_LocNode *next;
  DW2_Loc v;
};

typedef struct DW2_LocList DW2_LocList;
struct DW2_LocList
{
  DW2_LocNode *first;
  DW2_LocNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Unwind Info

typedef enum DW_CFIKind
{
  DW_CFIKind_Null,
  DW_CFIKind_CIE, // Common Information Entry
  DW_CFIKind_FDE, // Frame Description Entry
  DW_CFIKind_COUNT
}
DW_CFIKind;

typedef struct DW_CFIHeader DW_CFIHeader;
struct DW_CFIHeader
{
  DW_CFIKind kind;
  DW_Format fmt;
  Rng1U64 entry_range;
  U64 cie_pointer_off;
  U64 cie_pointer;
};

typedef struct DW_CIE DW_CIE;
struct DW_CIE
{
  Rng1U64 aug_string_range;
  U64 code_align_factor;
  S64 data_align_factor;
  U64 ret_addr_reg;
  U64 ext[5];
  DW_Format format;
  U8 version;
  U8 address_size;
  U8 segment_selector_size;
};

typedef struct DW_FDE DW_FDE;
struct DW_FDE
{
  U64 cie_pointer;
  Rng1U64 pc_range;
};

typedef struct DW_CFI DW_CFI;
struct DW_CFI
{
  DW_CIE cie;
  DW_FDE fde;
};

////////////////////////////////
//~ rjf: Globals

global read_only DW2_Attrib dw2_attrib_nil = {0};

////////////////////////////////
//~ rjf: Basic Parsing Helpers

internal U64 dw2_read_initial_length(String8 data, U64 off, U64 *out, DW_Format *fmt_out);
internal U64 dw2_read_fmt_u64(String8 data, U64 off, DW_Format format, U64 *out);
internal DW_SectionKind dw_section_kind_from_string(String8 string);

////////////////////////////////
//~ rjf: Unit Range Set Parsing (Top-Level .debug_info, .debug_aranges)

internal Rng1U64Array dw2_unit_ranges_from_data(Arena *arena, String8 data);

////////////////////////////////
//~ rjf: Unit Header Parsing

internal U64 dw2_read_unit_header(String8 data, U64 off, DW2_UnitHeader *out);

////////////////////////////////
//~ rjf: Abbreviation Map Parsing

internal U64 dw2_read_abbrev(Arena *arena, String8 data, U64 off, DW2_AbbrevHeader *header_out, DW2_AbbrevAttribList *attribs_out);
internal DW2_AbbrevMap dw2_abbrev_map_from_data(Arena *arena, String8 data, U64 off);
internal DW2_UnitAbbrevMapMap dw2_unit_abbrev_map_map_from_data(Arena *arena, String8 data, DW2_UnitHeader *unit_headers, U64 unit_headers_count);

////////////////////////////////
//~ rjf: Form Value Parsing

internal U64 dw2_read_form_val(DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW_FormKind form_kind, U64 implicit_const, DW2_FormVal *out);

////////////////////////////////
//~ rjf: Parse Context Construction

internal void dw2_parse_ctx_equip_unit_header(DW2_ParseCtx *ctx_out, DW2_UnitHeader *hdr, U64 unit_base_info_off);
internal void dw2_parse_ctx_equip_unit_abbrev_map(DW2_ParseCtx *ctx_out, DW2_UnitAbbrevMapMap *map, U64 unit_idx);
internal void dw2_parse_ctx_equip_unit_root_tag(DW2_ParseCtx *ctx_out, DW2_Tag *tag, DW2_OffsetTableSet *offset_tables);

////////////////////////////////
//~ rjf: Tag Parsing

internal U64 dw2_read_tag(Arena *arena, DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_Tag *tag_out);
internal DW2_Attrib *dw2_attrib_from_kind(DW2_Tag *tag, DW_AttribKind kind);
internal U64 dw2_reference_info_off_from_form_val(DW2_ParseCtx *ctx, DW2_FormVal *v);

////////////////////////////////
//~ rjf: Line Table Parsing

internal U64 dw2_read_line_table_header(Arena *arena, DW_Raw *raw, DW2_ParseCtx *ctx, String8 data, U64 off, DW2_LineTableHeader *out);

////////////////////////////////
//~ rjf: Offset Table Parsing (.debug_str_offsets, .debug_rnglists)

internal U64 dw2_read_offset_table(String8 data, U64 off, B32 offset_count_entry, DW2_OffsetTable *out);
internal DW2_OffsetTableList dw2_offset_table_list_from_data(Arena *arena, String8 data, B32 offset_count_entry);
internal B32 dw2_try_offset_from_table_idx(DW2_OffsetTable *tbl, U64 idx, U64 *out);
internal DW2_OffsetTableSet dw2_offset_table_set_from_raw(Arena *arena, DW_Raw *raw);

////////////////////////////////
//~ rjf: Range List Parsing (.debug_rnglists)

internal Rng1U64List dw2_rnglist_from_form_val(Arena *arena, DW2_ParseCtx *ctx, DW_Raw *raw, DW2_FormVal form_val);

////////////////////////////////
//~ rjf: Location List Parsing (.debug_loclists)

internal DW2_LocList dw2_loclist_from_form_val(Arena *arena, DW2_ParseCtx *ctx, DW_Raw *raw, DW2_FormVal form_val);

////////////////////////////////
//~ rjf: Unwind Info Parsing (.debug_frame)

internal U64 dw2_read_cfi_header(String8 data, U64 off, DW_CFIHeader *cfi_header_out);
internal U64 dw2_read_cie(String8 data, U64 off, DW_Format fmt, Arch arch, DW_CIE *cie_out);
internal U64 dw2_read_fde(String8 data, U64 off, DW_Format fmt, Arch arch, DW_CIE *cie, DW_FDE *fde_out);
internal B32 dw2_try_cfi_from_data(String8 data, U64 off, Arch arch, DW_CIE *cie_out, DW_FDE *fde_out);

#endif // DWARF_PARSE_H
