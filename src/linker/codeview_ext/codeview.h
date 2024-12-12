// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

//- Symbol and Leaf Headers

typedef struct CV_LeafHeader
{
  CV_LeafSize size;
  CV_LeafKind kind;
} CV_LeafHeader;

typedef struct CV_SymbolHeader
{
  CV_SymSize size;
  CV_SymKind kind;
} CV_SymbolHeader;

////////////////////////////////
// Type Index Helpers

//- $$Symbols

typedef struct CV_Symbol
{
  CV_SymKind kind;
  U64        offset;
  String8    data;
} CV_Symbol;

typedef struct CV_SymbolNode
{
  struct CV_SymbolNode *next;
  struct CV_SymbolNode *prev;
  CV_Symbol             data;
} CV_SymbolNode;

typedef struct CV_SymbolPtrNode
{
  struct CV_SymbolPtrNode *next;
  CV_Symbol               *data;
} CV_SymbolPtrNode;

typedef struct CV_SymbolList
{
  U64            count;
  CV_Signature   signature;
  CV_SymbolNode *first;
  CV_SymbolNode *last;
} CV_SymbolList;

typedef struct CV_SymbolListArray
{
  U64            count;
  CV_SymbolList *v;
} CV_SymbolListArray;

typedef struct CV_SymbolPtrArray
{
  U64             count;
  CV_SymbolNode **v;
} CV_SymbolPtrArray;

typedef struct CV_Scope
{
  struct CV_ScopeList *children;
  struct CV_Scope     *next;
  struct CV_Scope     *prev;
  CV_Symbol            symbol;
} CV_Scope;

typedef struct CV_ScopeList
{
  CV_Scope *first;
  CV_Scope *last;
} CV_ScopeList;

typedef struct CV_ScopeFrame
{
  struct        CV_ScopeFrame *next;
  CV_ScopeList *list;
  CV_Scope     *curr;
  U64           symbol_off;
  U32          *parent_off_ptr;
  U32          *end_off_ptr;
} CV_ScopeFrame;

//- $$FileChksms

typedef struct CV_Checksum
{
  CV_C13Checksum *header;
  String8 value;
} CV_Checksum;

typedef struct CV_ChecksumNode
{
  struct CV_ChecksumNode *next;
  CV_Checksum data;
} CV_ChecksumNode;

typedef struct CV_ChecksumList
{
  U64 count;
  CV_ChecksumNode *first;
  CV_ChecksumNode *last;
} CV_ChecksumList;

//- $$Lines

typedef struct CV_LineArray
{
  U32  file_off;
  U64  line_count;
  U64  col_count;
  U64 *voffs;     // [line_count + 1]
  U32 *line_nums; // [line_count]
  U16 *col_nums;  // [line_count * 2]
} CV_LineArray;

typedef struct CV_File
{
  U32          file_off;
  CV_LineArray lines;
} CV_File;

typedef struct CV_C13LinesHeader
{
  U64 sec_idx;
  U64 sec_off_lo;
  U64 sec_off_hi;
  U64 file_off;
  U64 line_count;
  U64 col_count;
  U64 line_array_off;
  U64 col_array_off;
} CV_C13LinesHeader;

typedef struct CV_C13LinesHeaderNode
{
  struct CV_C13LinesHeaderNode *next;
  CV_C13LinesHeader             v;
} CV_C13LinesHeaderNode;

typedef struct CV_C13LinesHeaderList
{
  CV_C13LinesHeaderNode *first;
  CV_C13LinesHeaderNode *last;
  U64                    count;
} CV_C13LinesHeaderList;

////////////////////////////////

typedef struct CV_TypeServerInfo
{
  String8 name;
  Guid    sig;
  U32     age;
} CV_TypeServerInfo;

typedef struct CV_TypeServerInfoNode
{
  struct CV_TypeServerInfoNode *next;
  CV_TypeServerInfo             data;
} CV_TypeServerInfoNode;

typedef struct CV_TypeServerInfoList
{
  CV_TypeServerInfoNode *first;
  CV_TypeServerInfoNode *last;
  U64                    count;
} CV_TypeServerInfoList;

typedef struct CV_PrecompInfo
{
  CV_TypeIndex start_index;
  U32          sig;
  U32          leaf_count;
  String8      obj_name;
} CV_PrecompInfo;

typedef struct CV_ObjInfo
{
  U32     sig;
  String8 name;
} CV_ObjInfo;

////////////////////////////////
// Accels

typedef struct CV_Line
{
  U64 voff;
  U32 file_off;
  U32 line_num;
  U16 col_num;
} CV_Line;

typedef struct CV_LinesAccel
{
  U64      map_count;
  CV_Line *map;
} CV_LinesAccel;

typedef struct CV_InlineeLinesAccel
{
  U64                        bucket_count;
  U64                        bucket_max;
  CV_C13InlineeLinesParsed **buckets;
} CV_InlineeLinesAccel;

typedef struct CV_InlineBinaryAnnotsParsed
{
  U64           lines_count;
  CV_LineArray *lines;
  Rng1U64List   code_ranges;
} CV_InlineBinaryAnnotsParsed;

typedef struct CV_C13InlineeLinesParsedList
{
  CV_C13InlineeLinesParsedNode *first;
  CV_C13InlineeLinesParsedNode *last;
  U64                           count;
} CV_C13InlineeLinesParsedList;

////////////////////////////////

typedef U32 CV_C13SubSectionIdxKind;
enum
{
  CV_C13SubSectionIdxKind_NULL,
#define X(N,c) CV_C13SubSectionIdxKind_##N,
  CV_C13SubSectionKindXList(X)
#undef X
  CV_C13SubSectionIdxKind_COUNT
};

typedef struct CV_C13SubSectionList
{
  CV_C13SubSectionNode *first;
  CV_C13SubSectionNode *last;
  U64                   count;
} CV_C13SubSectionList;

////////////////////////////////

typedef struct CV_DebugS
{
  String8List data_list[CV_C13SubSectionIdxKind_COUNT];
} CV_DebugS;

typedef struct CV_DebugT
{
  U64  size;
  U64  count;
  U8 **v;
} CV_DebugT;

////////////////////////////////
//~ Leaf Helpers

typedef struct CV_Leaf
{
  CV_LeafKind kind;
  String8     data;
} CV_Leaf;

typedef struct CV_LeafNode
{
  struct CV_LeafNode *next;
  CV_Leaf             data;
} CV_LeafNode;

typedef struct CV_LeafList
{
  U64          count;
  CV_LeafNode *first;
  CV_LeafNode *last;
} CV_LeafList;

////////////////////////////////
//~ String Hash Table

typedef struct CV_StringTableRange
{
  struct CV_StringTableRange *next;
  Rng1U64                     range;
  U64                         debug_s_idx;
} CV_StringTableRange;

typedef struct CV_StringBucket
{
  String8 string;
  union {
    struct {
      U32 idx0;
      U32 idx1;
    };
    U64 offset;
  } u;
} CV_StringBucket;

typedef struct CV_StringHashTable
{
  U64               total_string_size;
  U64               total_insert_count;
  U64               bucket_cap;
  CV_StringBucket **buckets;
} CV_StringHashTable;

typedef struct CV_StringHashTableResult
{
  U64               string_count;
  CV_StringBucket **buckets;
} CV_StringHashTableResult;

////////////////////////////////
//~ Task Contexts

typedef struct
{
  U64              cap;
  union {
    CV_SymbolNode ***buckets;
    CV_SymbolNode  **deref_buckets;
  } u;
  Rng1U64         *ranges;
  CV_SymbolNode  **symbols;
} CV_SymbolDeduperTask;

typedef struct
{
  CV_SymbolList  *list_arr;
  Rng1U64        *list_range_arr;
  U64            *symbol_base_arr;
  CV_SymbolNode **symbol_arr;
} CV_SymbolListSyncer;

typedef struct
{
  CV_DebugS            *arr;
  CV_StringTableRange **range_lists;
  U64                  *string_counts;
  U64                   bucket_cap;
  CV_StringBucket     **buckets;
  U64                   total_string_size;
  U64                   total_insert_count;
} CV_DedupStringTablesTask;

typedef struct
{
  U8               *buffer;
  Rng1U64          *ranges;
  CV_StringBucket **buckets;
} CV_PackStringHashTableTask;

typedef struct
{
  CV_DebugT    debug_t;
  Rng1U64     *ranges;
  String8List *lists;
  String8Node *nodes;
} CV_Str8ListFromDebugT;

////////////////////////////////

internal CV_ObjInfo        cv_obj_info_from_symbol(CV_Symbol symbol);
internal CV_TypeServerInfo cv_type_server_info_from_leaf(CV_Leaf leaf);
internal CV_PrecompInfo    cv_precomp_info_from_leaf(CV_Leaf leaf);

////////////////////////////////
//~ Leaf Helpers

internal U64     cv_compute_leaf_record_size(String8 data, U64 align);
internal U64     cv_serialize_leaf_to_buffer(U8 *buffer, U64 buffer_cursor, U64 buffer_size, CV_LeafKind kind, String8 data, U64 align);
internal String8 cv_serialize_raw_leaf(Arena *arena, CV_LeafKind kind, String8 data, U64 align);
internal String8 cv_serialize_leaf(Arena *arena, CV_Leaf *leaf, U64 align);
internal CV_Leaf cv_make_leaf(Arena *arena, CV_LeafKind kind, String8 data);
internal U64     cv_deserial_leaf(String8 raw_data, U64 off, U64 align, CV_Leaf *leaf_out);
internal CV_Leaf cv_leaf_from_string(String8 raw_data);

////////////////////////////////
//~ Symbol Helpers

internal U64     cv_compute_symbol_record_size(CV_Symbol *symbol, U64 align);
internal U64     cv_serialize_symbol_to_buffer(U8 *buffer, U64 buffer_cursor, U64 buffer_size, CV_Symbol *symbol, U64 align);
internal String8 cv_serialize_symbol(Arena *arena, CV_Symbol *symbol, U64 align);

internal String8       cv_make_symbol(Arena *arena, CV_SymKind kind, String8 data);
internal String8       cv_make_obj_name(Arena *arena, String8 obj_path, U32 sig);
internal String8       cv_make_comp3(Arena *arena,
                                     CV_Compile3Flags flags, CV_Language lang, CV_Arch arch, 
                                     U16 ver_fe_major, U16 ver_fe_minor, U16 ver_fe_build, U16 ver_feqfe,
                                     U16 ver_major, U16 ver_minor, U16 ver_build, U16 ver_qfe,
                                     String8 version_string);
internal String8       cv_make_envblock(Arena *arena, String8List string_list);
internal CV_Symbol     cv_make_proc_ref(Arena *arena, CV_ModIndex imod, U32 stream_offset, String8 name, B32 is_local);
internal CV_Symbol     cv_make_pub32(Arena *arena, CV_Pub32Flags flags, U32 off, U16 isect, String8 name);
internal CV_SymbolList cv_make_proc_refs(Arena *arena, CV_ModIndex imod, CV_SymbolList symbol_list);

////////////////////////////////
// .debug$S Helpers

internal CV_DebugS    cv_parse_debug_s_c13(Arena *arena, String8 raw_debug_s);
internal CV_DebugS    cv_parse_debug_s_c13_list(Arena *arena, String8List raw_debug_s);
internal CV_DebugS    cv_parse_debug_s(Arena *arena, String8 raw_debug_s);
internal void         cv_debug_s_concat_in_place(CV_DebugS *dst, CV_DebugS *src);
internal String8List  cv_data_c13_from_debug_s(Arena *arena, CV_DebugS *debug_s, B32 write_sig);

internal CV_C13SubSectionIdxKind cv_c13_sub_section_idx_from_kind(CV_C13SubSectionKind kind);
internal String8List *           cv_sub_section_ptr_from_debug_s(CV_DebugS *debug_s, CV_C13SubSectionKind kind);
internal String8List             cv_sub_section_from_debug_s(CV_DebugS debug_s, CV_C13SubSectionKind kind);
internal String8                 cv_string_table_from_debug_s(CV_DebugS debug_s);
internal String8                 cv_file_chksms_from_debug_s(CV_DebugS debug_s);

////////////////////////////////
//~ .debug$T helpers

internal CV_DebugT       cv_debug_t_from_data_arr(Arena *arena, String8Array data_arr, U64 align);
internal CV_DebugT       cv_debug_t_from_data(Arena *arena, String8 data, U64 align);
internal CV_Leaf         cv_debug_t_get_leaf(CV_DebugT debug_t, U64 leaf_idx);
internal String8         cv_debug_t_get_raw_leaf(CV_DebugT debug_t, U64 leaf_idx);
internal CV_LeafHeader * cv_debug_t_get_leaf_header(CV_DebugT debug_t, U64 leaf_idx);
internal B32             cv_debug_t_is_pch(CV_DebugT debug_t);
internal B32             cv_debug_t_is_type_server(CV_DebugT debug_t);
internal U64             cv_debug_t_array_count_leaves(U64 count, CV_DebugT *arr);

internal String8List cv_str8_list_from_debug_t_parallel(TP_Context *tp, Arena *arena, CV_DebugT types);

////////////////////////////////
//~ Sub Section helpers

// $$Symbols
internal void              cv_parse_symbol_sub_section(Arena *arena, CV_SymbolList *list, U64 offset_base, String8 data, U64 align);
internal void              cv_symbol_list_push_node(CV_SymbolList *list, CV_SymbolNode *node);
internal CV_SymbolNode *   cv_symbol_list_push(Arena *arena, CV_SymbolList *list);
internal CV_SymbolNode *   cv_symbol_list_push_data(Arena *arena, CV_SymbolList *list, CV_SymKind kind, String8 data);
internal CV_SymbolNode *   cv_symbol_list_push_many(Arena *arena, CV_SymbolList *list, U64 count);
internal void              cv_symbol_list_remove_node(CV_SymbolList *list, CV_SymbolNode *node);
internal void              cv_symbol_list_concat_in_place(CV_SymbolList *list, CV_SymbolList *to_concat);
internal void              cv_symbol_list_concat_in_place_arr(CV_SymbolList *list, U64 count, CV_SymbolList *to_concat);
internal U64               cv_symbol_list_arr_get_count(U64 count, CV_SymbolList *list_arr);
internal String8List       cv_data_from_symbol_list(Arena *arena, CV_SymbolList symbol_list, U64 align);
internal CV_SymbolList     cv_global_scope_symbols_from_list(Arena *arena, CV_SymbolList list);
internal CV_SymbolPtrArray cv_symbol_ptr_array_from_list(Arena *arena, TP_Context *tp, U64 count, CV_SymbolList *symbol_list_arr);

// $$FileChksms
#define CV_MAP_STRING_TO_OFFSET_FUNC(name) U64 name(void *ud, String8 string)
typedef CV_MAP_STRING_TO_OFFSET_FUNC(CV_MapStringToOffsetFunc);

internal void        cv_c13_patch_string_offsets_in_checksum_list(CV_ChecksumList checksum_list, String8 string_data, U64 string_data_base_offset, CV_StringHashTable string_ht);
internal String8List cv_c13_collect_source_file_names(Arena *arena, CV_ChecksumList checksum_list, String8 string_data);

// $$Lines
internal CV_C13LinesHeaderList cv_c13_lines_from_sub_sections(Arena *arena, String8 c13_data, Rng1U64 ss_range);
internal CV_LineArray     cv_c13_line_array_from_data(Arena *arena, String8 c13_data, U64 sec_base, CV_C13LinesHeader parsed_lines);

// $$InlineeLines
internal CV_C13InlineeLinesParsedList cv_c13_inlinee_lines_from_sub_sections(Arena *arena, String8List raw_inlinee_lines);
internal CV_InlineBinaryAnnotsParsed  cv_c13_parse_inline_binary_annots(Arena *arena, U64 parent_voff, CV_C13InlineeLinesParsed *inlinee_parsed, String8 binary_annots);

// $$FrameData
internal void cv_c13_patch_checksum_offsets_in_frame_data_list(String8List frame_data, U32 checksum_rebase);

////////////////////////////////
// $$Lines Accel

internal void            cv_make_c13_files(Arena *arena, String8 c13_data, CV_C13SubSectionList lines, U64 *file_count_out, CV_C13File **files_out);
internal CV_LinesAccel * cv_make_lines_accel(Arena *arena, U64 lines_count, CV_LineArray *lines);
internal CV_Line *       cv_line_from_voff(CV_LinesAccel *accel, U64 voff, U64 *out_line_count);

////////////////////////////////
// $$InlineeLines Accel

internal U64                        cv_c13_inlinee_lines_accel_hash(void *buffer, U64 size);
internal B32                        cv_c13_inlinee_lines_accel_push(CV_InlineeLinesAccel *accel, CV_C13InlineeLinesParsed *parsed);
internal CV_C13InlineeLinesParsed * cv_c13_inlinee_lines_accel_find(CV_InlineeLinesAccel *accel, CV_ItemId inlinee);
internal CV_InlineeLinesAccel *     cv_c13_make_inlinee_lines_accel(Arena *arena, CV_C13InlineeLinesParsedList sub_sects);

////////////////////////////////
// String Hash Table

internal U64                      cv_string_hash_table_hash(String8 string);
internal CV_StringHashTable       cv_dedup_string_tables(TP_Arena *arena, TP_Context *tp, U64 count, CV_DebugS *arr);
internal CV_StringHashTableResult cv_serialize_string_hash_table(Arena *arena, TP_Context *tp, CV_StringHashTable string_ht);
internal String8                  cv_pack_string_hash_table(Arena *arena, TP_Context *tp, CV_StringHashTable string_ht);

////////////////////////////////

internal Rng1U64List cv_make_defined_range_list_from_gaps(Arena *arena, Rng1U64 defrange, CV_LvarAddrGap *gaps, U64 gap_count);


