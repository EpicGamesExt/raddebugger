// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

#define PDB_NATURAL_ALIGN 4
#define PDB_SYMBOL_ALIGN PDB_NATURAL_ALIGN

////////////////////////////////
// Hash table

#define PDB_HASH_TABLE_PACK_FUNC(name) void name(Arena *arena, String8List *local_data_srl, String8List *key_value_srl, String8 key, String8 value, void *ud)
typedef PDB_HASH_TABLE_PACK_FUNC(PDB_HashTablePackFunc);

#define PDB_HASH_TABLE_UNPACK_FUNC(name) B32 name(void *ud, String8 local_data, String8 key_value_data, U64 *key_value_cursor, String8 *key_out, String8 *value_out)
typedef PDB_HASH_TABLE_UNPACK_FUNC(PDB_HashTableUnpackFunc);

typedef struct PDB_HashTableBucket
{
  String8 key;
  String8 value;
} PDB_HashTableBucket;

typedef struct PDB_HashTable
{
  Arena               *arena;
  PDB_HashTableBucket *bucket_arr;
  U32Array             present_bits;
  U32Array             deleted_bits;
  U32   			         max;
  U32                  count;
} PDB_HashTable;

typedef enum
{
  PDB_HashTableParseError_OK,
  PDB_HashTableParseError_OUT_OF_BYTES,
  PDB_HashTableParseError_CORRUPTED
} PDB_HashTableParseError;

////////////////////////////////
// String Table

typedef struct PDB_StringTableBucket
{
  String8          data;
  PDB_StringOffset offset;
  PDB_StringIndex  istr;
} PDB_StringTableBucket;

typedef struct PDB_StringTable
{
  Arena *arena;
  U32    version;
  U32    size;
  U32    bucket_count;
  U32    bucket_max;
  U32   *ibucket_array;
  PDB_StringTableBucket **bucket_array;
} PDB_StringTable;

typedef enum
{
  PDB_StringTableOpenError_OK,
  PDB_StringTableOpenError_BAD_MAGIC,
  PDB_StringTableOpenError_UNKNOWN_VERSION,
  PDB_StringTableOpenError_CORRUPTED,
  PDB_StringTableOpenError_STRING_OFFSET_OUT_OF_BOUNDS,
  PDB_StringTableOpenError_OFFSETS_EXCEED_BUCKET_COUNT
} PDB_StringTableOpenError;

////////////////////////////////
// Type Server

#define PDB_TYPE_HINT_STEP 128
#define PDB_LEAF_ALIGN PDB_NATURAL_ALIGN 

typedef enum
{
  PDB_OpenTypeServerError_OK,
  PDB_OpenTypeServerError_UNKNOWN,
  PDB_OpenTypeServerError_INVALID_BUCKET_COUNT,
  PDB_OpenTypeServerError_INVALID_TI_RANGE,
  PDB_OpenTypeServerError_UNSUPPORTED_VERSION,
} PDB_OpenTypeServerError;

typedef struct PDB_TypeBucket
{
  struct PDB_TypeBucket *next;
  String8                raw_leaf;
  CV_TypeIndex           type_index;
} PDB_TypeBucket;

typedef struct PDB_TypeServer
{
  Arena             *arena;
  CV_TypeIndex       ti_lo;
  String8List        leaf_list;
  U64                bucket_cap;
  PDB_TypeBucket   **buckets;
  MSF_StreamNumber   hash_sn;
  PDB_HashTable      hash_adj;
} PDB_TypeServer;

typedef struct PDB_TypeHashStreamInfo
{
  PDB_OffsetSize hash_vals;
  PDB_OffsetSize ti_offs;
  PDB_OffsetSize hash_adj;
} PDB_TypeHashStreamInfo;

typedef struct PDB_TypeServerParse
{
  Rng1U64 ti_range;
  String8 leaf_data;
} PDB_TypeServerParse;

typedef struct
{
  CV_DebugT       debug_t;
  U64            *udt_counts;
  U64            *udt_offsets;
  Rng1U64        *ranges;
  PDB_TypeServer *type_server;
  PDB_TypeBucket *udt_buckets;
} PDB_PushLeafTask;

typedef struct
{
  PDB_TypeServer *ts;
  U32            *map;
} PDB_WriteTypeToBucketMap;

typedef struct
{
  CV_TypeIndex    ti_lo;
  CV_TypeIndex    ti_hi;
  U64             hint_count;
  PDB_TpiOffHint *hint_arr;
  String8Node   **lf_arr;
  Rng1U64        *lf_range_arr;
  U64            *lf_cursor_arr;
  U8             *lf_buf;
  U64             lf_buf_size;
} PDB_WriteTypesTask;

////////////////////////////////
// Info

typedef struct PDB_InfoParse
{
  PDB_TpiVersion   version;
  COFF_TimeStamp   time_stamp;
  U32              age;
  Guid             guid;
  String8          extra_info;
} PDB_InfoParse;

typedef struct PDB_InfoContext
{
  Arena            *arena;
  COFF_TimeStamp    time_stamp;
  U32               age;
  Guid              guid;
  PDB_FeatureFlags  flags;
  PDB_HashTable     named_stream_ht;
  PDB_HashTable     src_header_block_ht;
  PDB_StringTable   strtab;
} PDB_InfoContext;

////////////////////////////////
// SRC Header Block

typedef enum
{
  PDB_SrcError_OK,
  PDB_SrcError_DUPLICATE_NAME_STREAM,
  PDB_SrcError_DUPLICATE_ENTRY,
  PDB_SrcError_UNABLE_TO_WRITE_DATA,
  PDB_SrcError_UNSUPPORTED_COMPRESSION,
  PDB_SrcError_UNKNOWN
} PDB_SrcError;

////////////////////////////////
// GSI

#define PDB_GSI_V70_SYMBOL_ALIGN 4
#define PDB_GSI_V70_WORD_SIZE    32
#define PDB_GSI_V70_BUCKET_COUNT 4096
#define PDB_GSI_V70_BITMAP_COUNT ((PDB_GSI_V70_BUCKET_COUNT / PDB_GSI_V70_WORD_SIZE) + 1)
#define PDB_GSI_V70_BITMAP_SIZE  (PDB_GSI_V70_BITMAP_COUNT * sizeof(U32))

typedef struct PDB_GsiContext
{
  Arena         *arena;
  U64            word_size;
  U64            symbol_align;
  U64            bucket_count;
  U64            symbol_count;
  CV_SymbolList *bucket_arr;
} PDB_GsiContext;

typedef struct PDB_GsiSortRecord
{
  ISectOff isect_off;
  String8 name;
  U64 offset;
} PDB_GsiSortRecord;

typedef struct PDB_GsiBuildResult
{
  PDB_GsiHeader      header;
  U64                hash_record_count;
  PDB_GsiHashRecord *hash_record_arr;
  PDB_GsiSortRecord *sort_record_arr;
  U64                bitmap_count;
  U32               *bitmap;
  U64                compressed_bucket_count;
  U32               *compressed_bucket_arr;
  U64                total_hash_size;
  String8            symbol_data;
} PDB_GsiBuildResult;

typedef struct PDB_GsiSerializeSymbolsTask
{
  U64                  symbol_align;
  CV_SymbolList       *bucket_arr;
  U64                 *bucket_size_arr;
  U64                 *bucket_off_arr;
  U8                  *buffer;
  PDB_GsiSortRecord  **sort_record_arr_arr;
  PDB_GsiSortRecord   *sort_record_arr;
} PDB_GsiSerializeSymbolsTask;

////////////////////////////////
// PSI

typedef struct PDB_PsiContext
{
  Arena *arena;
  PDB_GsiContext *gsi;
} PDB_PsiContext;

////////////////////////////////
// DBI

#define PDB_MODULE_ALIGN PDB_NATURAL_ALIGN

typedef struct PDB_DbiModule
{
  struct PDB_DbiModule *next;
  MSF_StreamNumber      sn;
  CV_ModIndex           imod;
  PDB_DbiSectionContrib first_sc;
  U64                   sym_data_size;
  U64                   c11_data_size;
  U64                   c13_data_size;
  U64                   globrefs_size; // TODO: what is this for?
  String8               obj_path;
  String8               lib_path;
  String8List           source_file_list;
} PDB_DbiModule;

typedef struct PDB_DbiModuleList
{
  PDB_DbiModule *first;
  PDB_DbiModule *last;
  U64            count;
} PDB_DbiModuleList;

typedef struct PDB_DbiSectionContribNode
{
  struct PDB_DbiSectionContribNode *next;
  PDB_DbiSectionContrib             data;
} PDB_DbiSectionContribNode;

typedef struct PDB_DbiSectionContribList
{
  PDB_DbiSectionContribNode *first;
  PDB_DbiSectionContribNode *last;
  U64                        count;
} PDB_DbiSectionContribList;

typedef struct PDB_DbiSectionNode
{
  struct PDB_DbiSectionNode *next;
  COFF_SectionHeader         data;
} PDB_DbiSectionNode;

typedef struct PDB_DbiSectionList
{
  U64                 count;
  PDB_DbiSectionNode *first;
  PDB_DbiSectionNode *last;
} PDB_DbiSectionList;

typedef struct PDB_DbiContext
{
  Arena                *arena;
  U32                   age;
  COFF_MachineType      machine;
  MSF_StreamNumber      globals_sn;
  MSF_StreamNumber      publics_sn;
  MSF_StreamNumber      symbols_sn;
  PDB_DbiModuleList     module_list;
  PDB_DbiSectionContribList sec_contrib_list;
  PDB_DbiSectionList    section_list;
  PDB_StringTable       ec_names;
  MSF_StreamNumber      dbg_streams[PDB_DbiStream_COUNT];
} PDB_DbiContext;

////////////////////////////////
// PDB

typedef struct PDB_Context
{
  Arena           *arena;
  MSF_Context     *msf;
  PDB_InfoContext *info;
  PDB_DbiContext  *dbi;
  PDB_GsiContext  *gsi;
  PDB_PsiContext  *psi;
  PDB_TypeServer  *type_servers[CV_TypeIndexSource_COUNT];
} PDB_Context;

////////////////////////////////

typedef struct
{
  PDB_GsiContext *gsi;
  Rng1U64        *ranges;
  CV_SymbolNode **symbols;
  U32            *hashes;
} GSI_SymbolHasherTask;

typedef struct
{
  CV_StringHashTable   string_ht;
  PDB_DbiModule      **mod_arr;
  U16                 *imod_arr;
  U16                 *source_file_name_count_arr;
  U32                **source_file_name_offset_arr;
} PDB_DbiBuildFileInfoTask;

////////////////////////////////
// PDB

internal PDB_Context *    pdb_alloc(U64 page_size, COFF_MachineType machine, COFF_TimeStamp time_stamp, U32 age, Guid guid);
internal PDB_Context *    pdb_open(String8 data);
internal void             pdb_release(PDB_Context **pdb_ptr);
internal void             pdb_build(TP_Context *tp, TP_Arena *pool_temp, PDB_Context *pdb, CV_StringHashTable string_ht);
internal void             pdb_set_machine(PDB_Context *pdb, COFF_MachineType machine);
internal void             pdb_set_guid(PDB_Context *pdb, Guid guid);
internal void             pdb_set_time_stamp(PDB_Context *pdb, COFF_TimeStamp time_stamp);
internal void             pdb_set_age(PDB_Context *pdb, U32 age);
internal COFF_MachineType pdb_get_machine(PDB_Context *pdb);
internal COFF_TimeStamp   pdb_get_time_stamp(PDB_Context *pdb);
internal U32              pdb_get_age(PDB_Context *pdb);
internal Guid             pdb_get_guid(PDB_Context *pdb);

////////////////////////////////
// Info

internal PDB_InfoContext * pdb_info_alloc(U32 age, COFF_TimeStamp time_stamp, Guid guid);
internal void              pdb_info_parse_from_data(String8 data, PDB_InfoParse *parse_out);
internal PDB_InfoContext * pdb_info_open(MSF_Context *msf, MSF_StreamNumber sn);
internal void              pdb_info_build(PDB_InfoContext *info, MSF_Context *msf, MSF_StreamNumber sn);
internal void              pdb_info_release(PDB_InfoContext **info_ptr);
internal MSF_StreamNumber  pdb_push_named_stream(PDB_HashTable *named_stream_ht, MSF_Context *msf, String8 name);
internal MSF_StreamNumber  pdb_find_named_stream(PDB_HashTable *named_stream_ht, String8 name);
internal PDB_SrcError      pdb_add_src(PDB_InfoContext *info, MSF_Context *msf, String8 file_path, String8 file_data, PDB_SrcCompType comp);

////////////////////////////////
// GSI

internal PDB_GsiContext *   gsi_alloc(void);
internal PDB_GsiContext *   gsi_open(MSF_Context *msf, MSF_StreamNumber sn, String8 symbol_data);
internal void               gsi_build(TP_Context *tp, PDB_GsiContext *gsi, MSF_Context *msf, MSF_StreamNumber gsi_sn, MSF_StreamNumber symbols_sn);
internal void               gsi_release(PDB_GsiContext **gsi_ptr);
internal void               gsi_write_build_result(TP_Context *tp, PDB_GsiBuildResult build, MSF_Context *msf, MSF_StreamNumber sn, MSF_StreamNumber symbols_sn);
internal PDB_GsiBuildResult gsi_build_ex(TP_Context *tp, Arena *arena, PDB_GsiContext *gsi, U64 symbol_data_base, B32 export_symbol_ptr_arr, U64 msf_page_size);
internal U32                gsi_hash(PDB_GsiContext *gsi, String8 input);
internal CV_SymbolNode *    gsi_push(PDB_GsiContext *gsi, CV_Symbol *symbol);
internal void               gsi_push_many_arr(TP_Context *tp, PDB_GsiContext *gsi, U64 count, CV_SymbolNode **symbol_arr);
internal void               gsi_push_many_list(PDB_GsiContext *gsi, U64 count, U32 *hash_arr, CV_SymbolList *list);
internal CV_SymbolNode *    gsi_search(PDB_GsiContext *gsi, CV_Symbol *symbol);

////////////////////////////////
// PSI

internal PDB_PsiContext * psi_alloc(void);
internal PDB_PsiContext * psi_open(MSF_Context *msf, MSF_StreamNumber sn, String8 symbol_data);
internal void             psi_build(TP_Context *tp, PDB_PsiContext *psi, MSF_Context *msf, MSF_StreamNumber sn, MSF_StreamNumber symbols_sn);
internal void             psi_release(PDB_PsiContext **psi_ptr);
internal CV_SymbolNode *  psi_push(PDB_PsiContext *psi, CV_Pub32Flags flags, U32 offset, U16 isect, String8 name);

// TODO:
//internal CV_Symbol psi_neareset_symbol(PDB_PsiContext *psi, U16 isect, U32 off);
//internal void      psi_push_thunk_map(PDB_PsiContext *psi, U32 *thunk_map, U32 thunk_count, U32 thunk_size, PDB_SO *sect_map, U32 sect_count, ISectOff thunk_table);

////////////////////////////////
// DBI

internal PDB_DbiContext *          dbi_alloc(COFF_MachineType machine, U32 age);
internal PDB_DbiContext *          dbi_open(MSF_Context *msf, MSF_StreamNumber sn);
internal void                      dbi_build(TP_Context *tp, PDB_DbiContext *dbi, MSF_Context *msf, MSF_StreamNumber dbi_sn, CV_StringHashTable string_ht);
internal void                      dbi_release(PDB_DbiContext **dbi_ptr);
internal PDB_DbiModule *           dbi_push_module(PDB_DbiContext *dbi, String8 obj_path, String8 lib_path);
internal String8                   dbi_module_read_symbol_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod);
internal String8                   dbi_module_read_c11_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod);
internal String8                   dbi_module_read_c13_data(Arena *arena, MSF_Context *msf, PDB_DbiModule *mod);
internal void                      dbi_module_push_section_contrib(PDB_DbiContext *dbi, PDB_DbiModule *mod, ISectOff isect_off, U32 size,  U32 data_crc, U32 reloc_crc, COFF_SectionFlags flags);
internal String8List *             dbi_open_file_info(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header);
internal PDB_DbiModuleList         dbi_open_module_info(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header, String8List *file_info);
internal PDB_DbiSectionContribList dbi_open_sec_contrib(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header);
internal PDB_StringTable           dbi_open_ec_names(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header);
internal void                      dbi_open_dbg_streams(MSF_StreamNumber *dbg_streams, MSF_Context *msf, MSF_StreamNumber sn, PDB_DbiHeader *dbi_header);
internal PDB_DbiSectionList        dbi_open_section_headers(Arena *arena, MSF_Context *msf, MSF_StreamNumber sn);
internal void                      dbi_build_section_header_stream(PDB_DbiContext *dbi, MSF_Context *msf, MSF_StreamNumber sn);

////////////////////////////////
// Hash Table

internal void                    pdb_hash_table_alloc(PDB_HashTable *ht, U32 max);
internal void                    pdb_hash_table_release(PDB_HashTable *ht);
internal PDB_HashTableParseError pdb_hash_table_from_data(PDB_HashTable *ht, String8 data, B32 has_local_data, PDB_HashTableUnpackFunc *unpack_func, void *unpack_ud, U64 *read_bytes_out);
internal String8                 pdb_data_from_hash_table(Arena *arena, PDB_HashTable *ht, B32 has_local_data, PDB_HashTablePackFunc *pack_func, void *pack_ud);
internal void                    pdb_hash_table_set(PDB_HashTable *ht, String8 key, String8 value);
internal B32                     pdb_hash_table_get(PDB_HashTable *ht, String8 key, String8 *value_out);
internal void                    pdb_hash_table_delete(PDB_HashTable *ht, String8 key);
internal B32                     pdb_hash_table_try_set(PDB_HashTable *ht, String8 key, String8 value);
internal B32                     pdb_hash_table_is_present(PDB_HashTable *ht, U32 k);
internal B32                     pdb_hash_table_is_deleted(PDB_HashTable *ht, U32 k);
internal U32                     pdb_hash_table_hash(String8 key);
internal void                    pdb_hash_table_grow(PDB_HashTable *ht, U64 new_capacity);
internal void                    pdb_hash_table_get_present_keys_and_values(Arena *arena, PDB_HashTable *ht, String8Array *keys_out, String8Array *values_out);

////////////////////////////////

internal PDB_HashTableParseError pdb_hash_adj_hash_table_from_data(PDB_HashTable *ht, String8 data, PDB_StringTable *strtab, U64 *read_bytes_out);
internal PDB_HashTableParseError pdb_src_header_block_ht_from_data(PDB_HashTable *ht, String8 data, PDB_StringTable *strtab, U64 *read_bytes_out);
internal PDB_HashTableParseError pdb_named_stream_ht_from_data(PDB_HashTable *ht, String8 data, U64 *read_bytes_out);

internal String8 pdb_data_from_hash_adj_hash_table(Arena *arena, PDB_HashTable *ht, PDB_StringTable *strtab);
internal String8 pdb_data_from_src_header_block_ht(Arena *arena, PDB_HashTable *ht, PDB_StringTable *strtab);
internal String8 pdb_data_from_named_stream_ht(Arena *arena, PDB_HashTable *ht);

////////////////////////////////
// String Table

internal void                     pdb_strtab_alloc(PDB_StringTable *strtab, U32 max);
internal PDB_StringTableOpenError pdb_strtab_open(PDB_StringTable *strtab, MSF_Context *msf, MSF_StreamNumber sn);
internal void                     pdb_strtab_build(PDB_StringTable *strtab, MSF_Context *msf, MSF_StreamNumber sn);
internal void                     pdb_strtab_release(PDB_StringTable *strtab);
internal PDB_StringIndex          pdb_strtab_add(PDB_StringTable *strtab, String8 string);
internal B32                      pdb_strtab_search(PDB_StringTable *strtab, String8 string, PDB_StringIndex *index_out);
internal String8                  pdb_strtab_string_from_offset(PDB_StringTable *strtab, PDB_StringOffset offset);
internal PDB_StringOffset         pdb_strtab_string_to_offset(PDB_StringTable *strtab, PDB_StringIndex stridx);
internal U32                      pdb_strtab_get_serialized_size(PDB_StringTable *strtab);
internal B32                      pdb_strtab_try_add(PDB_StringTable *strtab, String8 string, PDB_StringIndex *index_out);
internal void                     pdb_strtab_grow(PDB_StringTable *strtab, U64 new_max);
internal U32                      pdb_strtab_hash(PDB_StringTable *strtab, String8 string);

////////////////////////////////
// Type Server

internal PDB_OpenTypeServerError pdb_type_server_parse_from_data_v80(String8 data, PDB_TypeServerParse *parse_out);
internal PDB_OpenTypeServerError pdb_type_server_parse_from_data(String8 data, PDB_TypeServerParse *parse_out);
internal PDB_TypeServer *        pdb_type_server_alloc(U64 bucket_count);
internal PDB_TypeServer *        pdb_type_server_open_v80(MSF_Context *msf, MSF_StreamNumber sn, PDB_StringTable *strtab);
internal PDB_TypeServer *        pdb_type_server_open(MSF_Context *msf, MSF_StreamNumber sn, PDB_StringTable *strtab);
internal void                    pdb_type_server_build(TP_Context *tp, PDB_TypeServer *ts, PDB_StringTable *strtab, MSF_Context *msf, MSF_StreamNumber sn);
internal void                    pdb_type_server_release(PDB_TypeServer **serv_ptr);
internal void                    pdb_type_server_push(PDB_TypeServer *ts, String8 raw_leaf);
internal void                    pdb_type_server_push_parallel(TP_Context *tp, PDB_TypeServer *ts, CV_DebugT types);
//internal CV_LeafNode *     pdb_type_server_leaf_from_string(PDB_TypeServer *ts, String8 string);
internal String8Node *           pdb_type_server_reserve(PDB_TypeServer *ts, U64 count);
internal String8Node *           pdb_type_server_make_leaf(PDB_TypeServer *ts, CV_LeafKind kind, String8 data);
internal void                    pdb_type_server_push_bucket(PDB_TypeServer *ts, CV_Leaf *leaf);
internal PDB_TypeHashStreamInfo  pdb_type_hash_stream_build(TP_Context *tp, PDB_TypeServer *ts, PDB_StringTable *strtab, MSF_Context *msf, PDB_TpiOffHint *hint_arr, U64 hint_count);

////////////////////////////////
// Enum -> String

internal String8 pdb_string_from_src_error(PDB_SrcError error);
internal String8 pdb_string_from_open_type_server_error(PDB_OpenTypeServerError error);


