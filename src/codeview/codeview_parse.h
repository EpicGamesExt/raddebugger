// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CODEVIEW_PARSE_H
#define CODEVIEW_PARSE_H

////////////////////////////////
//~ CodeView Common Parser Types

// CV_Numeric layout
// x: U16
// buf: U8[]
// case (x < 0x8000):  kind=U16 val=x
// case (x >= 0x8000): kind=x   val=buf

typedef struct CV_NumericParsed CV_NumericParsed;
struct CV_NumericParsed
{
  CV_NumericKind  kind;
  U8             *val;
  U64             encoded_size;
};

typedef struct CV_RecRange CV_RecRange;
struct CV_RecRange
{
  U32          off;
  CV_RecHeader hdr;
};

#define CV_REC_RANGE_CHUNK_SIZE 511

typedef struct CV_RecRangeChunk CV_RecRangeChunk;
struct CV_RecRangeChunk
{
  struct CV_RecRangeChunk *next;
  CV_RecRange              ranges[CV_REC_RANGE_CHUNK_SIZE];
};

typedef struct CV_RecRangeStream CV_RecRangeStream;
struct CV_RecRangeStream
{
  CV_RecRangeChunk *first_chunk;
  CV_RecRangeChunk *last_chunk;
  U64               total_count;
};

typedef struct CV_RecRangeArray CV_RecRangeArray;
struct CV_RecRangeArray
{
  CV_RecRange *ranges;
  U64          count;
};

////////////////////////////////
//~ CodeView Sym Parser Types

typedef struct CV_SymTopLevelInfo CV_SymTopLevelInfo;
struct CV_SymTopLevelInfo
{
  CV_Arch     arch;
  CV_Language language;
  String8     compiler_name;
};

typedef struct CV_SymParsed CV_SymParsed;
struct CV_SymParsed
{
  // source information
  String8 data;
  U64     sym_align;
  
  // sym index derived from source
  CV_RecRangeArray sym_ranges;
  
  // top-level info derived from the syms
  CV_SymTopLevelInfo info;
};

////////////////////////////////
//~ CodeView Leaf Parser Types

typedef struct CV_LeafParsed CV_LeafParsed;
struct CV_LeafParsed
{
  // source information
  String8   data;
  CV_TypeId itype_first;
  CV_TypeId itype_opl;
  
  // leaf index derived from source
  CV_RecRangeArray leaf_ranges;
};

////////////////////////////////
//~ CodeView C13 Info Parser Types

typedef struct CV_C13InlineSiteDecoder CV_C13InlineSiteDecoder;
struct CV_C13InlineSiteDecoder
{
  U64                cursor;
  U64                parent_voff;
  CV_InlineRangeKind range_kind;
  U32                code_length;
  U32                code_offset;
  U32                file_off;
  S32                ln;
  S32                cn;
  U64                code_offset_lo;
  B32                code_offset_changed;
  B32                code_offset_lo_changed;
  B32                code_length_changed;
  B32                ln_changed;
  B32                file_off_changed;
  Rng1U64            last_range;
  U32                file_count;
  Rng1U64            file_last_range;
  U64                file_line_count;
  U64                file_last_ln;
};

typedef U32 CV_C13InlineSiteDecoderStepFlags;
enum
{
  CV_C13InlineSiteDecoderStepFlag_EmitRange       = (1 << 0),
  CV_C13InlineSiteDecoderStepFlag_ExtendLastRange = (1 << 1),
  CV_C13InlineSiteDecoderStepFlag_EmitFile        = (1 << 2),
  CV_C13InlineSiteDecoderStepFlag_EmitLine        = (1 << 3),
};

typedef struct CV_C13InlineSiteDecoderStep CV_C13InlineSiteDecoderStep;
struct CV_C13InlineSiteDecoderStep
{
  CV_C13InlineSiteDecoderStepFlags flags;
  Rng1U64                     range;
  U64                         line_voff;
  U64                         line_voff_end;
  U64                         ln;
  U64                         cn;
  U32                         file_off;
};

typedef struct CV_C13LinesParsed CV_C13LinesParsed;
struct CV_C13LinesParsed
{
  // raw info
  U32 sec_idx;
  U32 file_off;
  U64 secrel_base_off;
  
  // parsed info
  String8  file_name;
  U64     *voffs;     // [line_count + 1]
  U32     *line_nums; // [line_count]
  U16     *col_nums;  // [2*line_count]
  U32      line_count;
};

typedef struct CV_C13LinesParsedNode CV_C13LinesParsedNode;
struct CV_C13LinesParsedNode
{
  CV_C13LinesParsedNode *next;
  CV_C13LinesParsed      v;
};

typedef struct CV_C13InlineeLinesParsed CV_C13InlineeLinesParsed;
struct CV_C13InlineeLinesParsed
{
  CV_ItemId  inlinee;
  U32        file_off;
  String8    file_name;
  U32        first_source_ln;
  U32        extra_file_count;
  U32       *extra_files;
};

typedef struct CV_C13InlineeLinesParsedNode CV_C13InlineeLinesParsedNode;
struct CV_C13InlineeLinesParsedNode
{
  CV_C13InlineeLinesParsedNode *next;
  CV_C13InlineeLinesParsedNode *hash_next;
  CV_C13InlineeLinesParsed      v;
};

typedef struct CV_C13SubSectionNode CV_C13SubSectionNode;
struct CV_C13SubSectionNode
{
  struct CV_C13SubSectionNode  *next;
  CV_C13SubSectionKind          kind;
  U32                           off;
  U32                           size;
  CV_C13LinesParsedNode        *lines_first;
  CV_C13LinesParsedNode        *lines_last;
  CV_C13InlineeLinesParsedNode *inlinee_lines_first;
  CV_C13InlineeLinesParsedNode *inlinee_lines_last;
};

typedef struct CV_C13Parsed CV_C13Parsed;
struct CV_C13Parsed
{
  // rjf: source data
  String8 data;
  
  // rjf: full sub-section list
  CV_C13SubSectionNode *first_sub_section;
  CV_C13SubSectionNode *last_sub_section;
  U64                   sub_section_count;
  
  // rjf: fastpath to file checksums section
  CV_C13SubSectionNode *file_chksms_sub_section;
  
  // rjf: fastpath to map inlinee CV_ItemId -> CV_InlineeLinesParsed quickly
  CV_C13InlineeLinesParsedNode **inlinee_lines_parsed_slots;
  U64                            inlinee_lines_parsed_slots_count;
};

typedef struct CV_UDTInfo CV_UDTInfo;
struct CV_UDTInfo
{
  String8      name;
  String8      unique_name;
  CV_TypeProps props;
};

////////////////////////////////
//~ CodeView Compound Types

typedef struct CV_TypeIdArray CV_TypeIdArray;
struct CV_TypeIdArray
{
  CV_TypeId *itypes;
  U64        count;
};

////////////////////////////////

//- Hasher

internal U64 cv_hash_from_string(String8 string);
internal U64 cv_hash_from_item_id(CV_ItemId item_id);

//- Numeric Decoder

internal CV_NumericParsed cv_numeric_from_data_range(U8 *first, U8 *opl);
internal U64              cv_read_numeric(String8 data, U64 offset, CV_NumericParsed *out);

internal B32 cv_numeric_fits_in_u64(CV_NumericParsed *num);
internal B32 cv_numeric_fits_in_s64(CV_NumericParsed *num);
internal B32 cv_numeric_fits_in_f64(CV_NumericParsed *num);

internal U64 cv_u64_from_numeric(CV_NumericParsed *num);
internal S64 cv_s64_from_numeric(CV_NumericParsed *num);
internal F64 cv_f64_from_numeric(CV_NumericParsed *num);

//- Inlinee Lines Binary Annot Decoder

internal U64 cv_decode_inline_annot_u32(String8 data, U64 offset, U32 *out_value);
internal U64 cv_decode_inline_annot_s32(String8 data, U64 offset, S32 *out_value);
internal S32 cv_inline_annot_signed_from_unsigned_operand(U32 value);

internal CV_C13InlineSiteDecoder      cv_c13_inline_site_decoder_init(U32 file_off, U32 first_source_ln, U32 parent_voff);
internal CV_C13InlineSiteDecoderStep cv_c13_inline_site_decoder_step(CV_C13InlineSiteDecoder *decoder, String8 binary_annots);

//- Symbol/Leaf Helpers

internal B32 cv_is_udt_name_anon(String8 name);

internal B32 cv_is_global_symbol(CV_SymKind kind);
internal B32 cv_is_typedef(CV_SymKind kind);
internal B32 cv_is_scope_symbol(CV_SymKind kind);
internal B32 cv_is_end_symbol(CV_SymKind kind);

internal B32                cv_is_udt(CV_LeafKind kind);
internal B32                cv_is_leaf_type_server(CV_LeafKind kind);
internal B32                cv_is_leaf_pch(CV_LeafKind kind);
internal CV_TypeIndexSource cv_type_index_source_from_leaf_kind(CV_LeafKind leaf_kind);

internal CV_TypeIndexInfoList cv_get_symbol_type_index_offsets(Arena *arena, CV_SymKind kind, String8 data);
internal CV_TypeIndexInfoList cv_get_leaf_type_index_offsets(Arena *arena, CV_LeafKind leaf_kind, String8 data);
internal CV_TypeIndexInfoList cv_get_inlinee_type_index_offsets(Arena *arena, String8 raw_data);
internal String8Array         cv_get_data_around_type_indices(Arena *arena, CV_TypeIndexInfoList ti_list, String8 data);

internal U64     cv_name_offset_from_symbol(CV_SymKind kind, String8 data);
internal String8 cv_name_from_symbol(CV_SymKind kind, String8 data);

internal CV_UDTInfo cv_get_udt_info(CV_LeafKind kind, String8 data);
internal String8    cv_name_from_udt_info(CV_UDTInfo udt_info);

//- rjf: record range stream parsing
internal CV_RecRangeStream * cv_rec_range_stream_from_data(Arena *arena, String8 data, U64 align);
internal CV_RecRangeArray    cv_rec_range_array_from_stream(Arena *arena, CV_RecRangeStream *stream);

//- rjf: sym stream parsing
internal CV_SymParsed * cv_sym_from_data(Arena *arena, String8 sym_data, U64 sym_align);

//- rjf: leaf stream parsing
internal CV_LeafParsed * cv_leaf_from_data(Arena *arena, String8 leaf_data, CV_TypeId first);
internal CV_C13Parsed  * cv_c13_parsed_from_data(Arena *arena, String8 c13_data, String8 strtbl, COFF_SectionHeaderArray sections);

#endif // CODEVIEW_PARSE_H

