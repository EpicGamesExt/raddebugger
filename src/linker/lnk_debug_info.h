// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

typedef struct LNK_PchInfo
{
  CV_TypeIndex ti_lo;
  CV_TypeIndex ti_hi;
  U64          debug_p_obj_idx;
} LNK_PchInfo;

typedef struct LNK_CodeViewSymbolsInput
{
  U64            obj_idx;
  CV_SymbolList *symbol_list;
  String8        raw_symbols;
} LNK_CodeViewSymbolsInput;

typedef struct LNK_CodeViewInput
{
  U64             count;
  U64             internal_count;
  U64             external_count;
  U64             type_server_count;
  String8        *type_server_path_arr; // [type_server_count]
  String8        *type_server_data_arr; // [type_server_count]
  U64List        *ts_to_obj_arr;        // [type_server_count]
  LNK_Obj        *obj_arr;              // [count]
  LNK_PchInfo    *pch_arr;              // [count]
  CV_DebugS      *debug_s_arr;          // [count]
  CV_DebugT      *debug_p_arr;          // [count]
  CV_DebugT      *debug_t_arr;          // [count]
  CV_DebugT      *merged_debug_t_p_arr; // [count]

  U64                       total_symbol_input_count;
  LNK_CodeViewSymbolsInput *symbol_inputs;  // [total_symbol_input_count]
  CV_SymbolListArray       *parsed_symbols; // [count]

  LNK_Obj                  *internal_obj_arr;         // [internal_count]
  CV_DebugS                *internal_debug_s_arr;     // [internal_count]
  CV_DebugT                *internal_debug_t_arr;     // [internal_count]
  CV_DebugT                *internal_debug_p_arr;     // [internal_count]
  U64                      internal_total_symbol_input_count;
  LNK_CodeViewSymbolsInput *internal_symbol_inputs;   // [internal_total_symbol_input_count]
  CV_SymbolListArray       *internal_parsed_symbols;  // [internal_count]

  LNK_Obj                   *external_obj_arr;           // [external_count]
  CV_DebugS                 *external_debug_s_arr;       // [external_count]
  CV_DebugT                 *external_debug_t_arr;       // [external_count]
  CV_DebugT                 *external_debug_p_arr;       // [external_count]
  U64                        external_total_symbol_input_count;
  LNK_CodeViewSymbolsInput  *external_symbol_inputs;     // [exteranl_total_symbol_input_count]
  CV_SymbolListArray        *external_parsed_symbols;    // [external_count]
  Rng1U64                  **external_ti_ranges;         // [type_server_count]
  CV_DebugT                **external_leaves;            // [type_server_count]
  U64                       *external_obj_to_ts_idx_arr; // [external_count]
  Rng1U64                    external_obj_range;
} LNK_CodeViewInput;

////////////////////////////////

typedef enum
{
  LNK_LeafLocType_Internal,
  LNK_LeafLocType_External,
  LNK_LeafLocType_Count
} LNK_LeafLocType;

#define LNK_LeafRefFlag_LocIdxExternal (1 << 31)
#define LNK_LeafRefFlag_LeafIdxIPI     (1 << 31)
typedef struct
{
  U32 enc_loc_idx;
  U32 enc_leaf_idx;
} LNK_LeafRef;

typedef struct LNK_LeafRange
{
  struct LNK_LeafRange *next;
  Rng1U64               range;
  CV_DebugT            *debug_t;
} LNK_LeafRange;

typedef struct LNK_LeafRangeList
{
  U64            count;
  LNK_LeafRange *first;
  LNK_LeafRange *last;
} LNK_LeafRangeList;

typedef struct
{
  LNK_LeafRef  leaf_ref;
  CV_TypeIndex type_index;
} LNK_LeafBucket;

typedef struct
{
  U64              count;
  LNK_LeafBucket **v;
} LNK_LeafBucketArray;

typedef struct
{
  U64              cap;
  LNK_LeafBucket **bucket_arr;
} LNK_LeafHashTable;

typedef union
{
  struct {
    U128Array **internal_hashes;
    U128Array **external_hashes;
  };
  U128Array **v[CV_TypeIndexSource_COUNT];
} LNK_LeafHashes;

////////////////////////////////

typedef struct
{
  LNK_Obj      **obj_arr;
  LNK_ChunkList *sect_list_arr;
  CV_DebugS     *debug_s_arr;
} LNK_ParseDebugSTaskData;

typedef struct
{
  LNK_Obj     **obj_arr;
  String8Array *data_arr_arr;
} LNK_CheckDebugTSigTaskData;

typedef struct
{
  LNK_Obj     **obj_arr;
  String8Array *data_arr_arr;
  CV_DebugT    *debug_t_arr;
} LNK_ParseDebugTTaskData;

typedef struct
{
  String8Array   data_arr;
  MSF_Parsed   **msf_parse_arr;
} LNK_MsfParsedFromDataTask;

typedef struct
{
  CV_TypeServerInfo  *ts_info_arr;
  MSF_Parsed        **msf_parse_arr;
  Rng1U64           **external_ti_ranges;
  CV_DebugT         **external_leaves;
  B8                 *is_corrupted;
} LNK_GetExternalLeavesTask;

////////////////////////////////

typedef struct
{
  LNK_LeafRangeList  *leaf_ranges_per_task;
  U64               **count_arr_arr;
} LNK_CountPerSourceLeafTask;

typedef struct
{
  LNK_CodeViewInput *input;
  LNK_LeafHashes    *hashes;
  Arena            **fixed_arenas;
  CV_DebugT         *debug_t_arr;
} LNK_LeafHasherTask;

typedef struct
{
  LNK_CodeViewInput  *input;
  LNK_LeafHashes     *hashes;
  LNK_LeafHashTable  *leaf_ht_arr;
  CV_DebugT          *debug_t_arr;
} LNK_LeafDedupInternal;

typedef struct
{
  LNK_CodeViewInput  *input;
  LNK_LeafHashes     *hashes;
  LNK_LeafHashTable  *leaf_ht_arr;
  CV_TypeIndexSource  dedup_ti_source;
} LNK_LeafDedupExternal;

typedef struct
{
  LNK_LeafHashTable   *ht;
  U64                 *count_arr;
  Rng1U64             *range_arr;
  U64                 *offset_arr;
  LNK_LeafBucketArray  result;
} LNK_GetPresentBucketsTask;

typedef struct
{
  U64             loc_idx_bit_count_0;
  U64             loc_idx_bit_count_1;
  U64             loc_idx_bit_count_2;
  U64             counts_max;
  U32           **counts_arr;
  Rng1U64        *ranges;
  LNK_LeafBucket **dst;
  LNK_LeafBucket **src;
  U64             loc_idx_max;
  U64             pass_idx;
} LNK_LeafRadixSortTask;

typedef struct
{
  U32             *counts;
  U32             *offsets;
  LNK_LeafBucket **dst;
  LNK_LeafBucket **src;
  Rng1U64         *ranges;
} LNK_LeafLocRadixSortTask;

typedef struct
{
  Rng1U64            *range_arr;
  CV_TypeIndex        min_type_index;
  LNK_LeafBucketArray bucket_arr;
} LNK_AssignTypeIndicesTask;

typedef struct
{
  LNK_CodeViewInput  *input;
  LNK_LeafBucket    **bucket_arr;
  U8                **raw_leaf_arr;
  Rng1U64            *range_arr;
} LNK_UnbucketRawLeavesTask;

typedef struct
{
  LNK_CodeViewInput  *input;
  LNK_LeafHashes     *hashes;
  LNK_LeafHashTable  *leaf_ht_arr;
  CV_SymbolList      *symbol_list_arr;
  Arena             **arena_arr;
} LNK_PatchSymbolTypesTask;

typedef struct
{
  LNK_CodeViewInput *input;
  LNK_LeafHashes    *hashes;
  LNK_LeafHashTable *leaf_ht_arr;
  CV_DebugS         *debug_s_arr;
} LNK_PatchInlinesTask;

typedef struct
{
  LNK_CodeViewInput  *input;
  LNK_LeafHashes     *hashes;
  LNK_LeafHashTable  *leaf_ht_arr;
  LNK_LeafBucket    **bucket_arr;
  Rng1U64            *range_arr;
  Arena             **fixed_arena_arr;
} LNK_PatchLeavesTask;

////////////////////////////////

typedef struct
{
  String8List *data_list_arr;
} LNK_ProcessedCodeViewC11Data;

typedef struct
{
  String8List *data_list_arr;
  String8List *source_file_names_list_arr;
} LNK_ProcessedCodeViewC13Data;

typedef struct
{
  LNK_CodeViewSymbolsInput *inputs;
} LNK_ParseCVSymbolsTaskData;

typedef struct
{
  U64                        total_symbol_input_count;
  LNK_CodeViewSymbolsInput  *symbol_inputs;
  CV_SymbolListArray        *parsed_symbols;
  PDB_DbiModule            **mod_arr;
  String8List               *symbol_data_arr;
  CV_SymbolList             *gsi_list_arr;
} LNK_ProcessSymDataTaskData;

typedef struct
{
  CV_DebugS          *debug_s_arr;
  MSF_Context        *msf;
  PDB_DbiModule      **dbi_mod_arr;
  String8List        *c13_data_arr;
  String8List        *source_file_names_list_arr;
  U64                 string_data_base_offset;
  CV_StringHashTable  string_ht;
} LNK_ProcessC13DataTask;

typedef struct
{
  MSF_Context    *msf;
  PDB_DbiModule **mod_arr;
  String8List    *symbol_data_arr;
  String8List    *c11_data_list_arr;
  String8List    *c13_data_list_arr;
  String8List    *globrefs_arr;
} LNK_WriteModuleDataTask;

typedef struct
{
  LNK_Obj                    *obj_arr;
  LNK_Section               **sect_id_map;
  PDB_DbiModule             **mod_arr;
  PDB_DbiSectionContribList  *sc_list;
  String8                     image_data;
} LNK_PushDbiSecContribTaskData;

typedef struct
{
  U32Array      *hash_arr_arr;
  CV_SymbolList *list_arr;
} LNK_HashCVSymbolListTask;

typedef struct
{
  U64            *hash_arr;
  CV_SymbolNode **arr;
  Rng1U64        *range_arr;
} LNK_CvSymbolPtrArrayHasher;

typedef struct
{
  LNK_Section                 **sect_id_map;
  LNK_SymbolHashTrieChunkList  *chunk_lists;
  CV_SymbolList                *pub_list_arr;

  Rng1U64           *symbol_ranges;
  PDB_GsiContext    *gsi;
  CV_SymbolPtrArray  symbols;
  U32               *hashes;
} LNK_BuildPublicSymbolsTask;

typedef struct
{
  CV_TypeIndex              ipi_min_type_index;
  CV_DebugT                 ipi_types;
  LNK_CodeViewSymbolsInput *symbol_inputs;
  CV_SymbolListArray       *parsed_symbols;
} LNK_PostProcessCvSymbolsTask;

typedef struct
{
  Rng1U64           *range_arr;
  CV_SymbolPtrNode **bucket_arr;
  CV_SymbolPtrNode **out_arr;
  U64               *out_count_arr;
} LNK_GsiDeduper;

typedef struct
{
  Rng1U64           *range_arr;
  CV_SymbolPtrNode **bucket_arr;
  U64               *symbol_base_arr;
  CV_SymbolNode    **symbol_arr;
} LNK_GsiUnbucket;

typedef struct
{
  CV_DebugT    debug_t;
  Rng1U64     *ranges;
  U64          hash_length;
  B32          make_map;
  TP_Arena    *map_arena;
  String8List *maps;
} LNK_TypeNameReplacer;

////////////////////////////////
// RAD Debug Info

typedef struct
{
  String8 name;
  U64     leaf_idx;
} LNK_UDTNameBucket;

typedef struct
{
  CV_DebugT           debug_t;
  Rng1U64            *ranges;
  U64                 buckets_cap;
  LNK_UDTNameBucket **buckets;
} LNK_BuildUDTNameHashTableTask;

typedef struct
{
  CV_DebugT           debug_t;
  CV_TypeIndex        ti_lo;
  Rng1U64            *ranges;
  U64                 udt_name_buckets_cap;
  LNK_UDTNameBucket **udt_name_buckets;
  CV_TypeIndex       *fwdmap;
} LNK_BuildUDTFwdMapTask;

////////////////////////////////

typedef struct
{
  CV_DebugT               *types;
  U64                      type_cap;
  U64                      udt_cap;
  RDIB_TypeRef             variadic_type_ref;
  Rng1U64                 *itype_ranges;
  U64                      udt_name_bucket_cap;
  LNK_UDTNameBucket      **udt_name_buckets;
  RDIB_Type              **tpi_itype_map;
  RDIB_TypeChunkList      *rdib_types_lists;
  RDIB_TypeChunkList      *rdib_types_struct_lists;
  RDIB_TypeChunkList      *rdib_types_union_lists;
  RDIB_TypeChunkList      *rdib_types_enum_lists;
  RDIB_TypeChunkList      *rdib_types_params_lists;
  RDIB_TypeChunkList      *rdib_types_udt_members_lists;
  RDIB_TypeChunkList      *rdib_types_enum_members_lists;
  RDIB_UDTMemberChunkList *rdib_udt_members_lists;
  RDIB_UDTMemberChunkList *rdib_enum_members_lists;
  Rng1U64                 *ranges;
} LNK_ConvertTypesToRDI;

typedef struct
{
  U64              obj_idx;
  RDIB_SourceFile *src_file;
} LNK_SourceFileBucket;

typedef struct
{
  LNK_Obj               *obj_arr;
  CV_DebugS             *debug_s_arr;
  U64                    total_src_file_count;
  LNK_SourceFileBucket **src_file_buckets;
  U64                    src_file_buckets_cap;
} LNK_ConvertSourceFilesToRDITask;

typedef struct
{
  CV_Arch     arch;
  CV_Language language;
  String8     compiler_name;
} LNK_CodeViewCompilerInfo;

typedef struct
{
  LNK_SectionArray          image_sects;
  LNK_Section             **sect_id_map;
  LNK_Obj                  *obj_arr;
  CV_DebugS                *debug_s_arr;
  CV_DebugT                 ipi;
  LNK_CodeViewSymbolsInput *symbol_inputs;
  CV_SymbolListArray       *parsed_symbols;
  Rng1U64                   ipi_itype_range;
  Rng1U64                   tpi_itype_range;
  RDIB_Type               **tpi_itype_map;
  U64                       src_file_buckets_cap;
  LNK_SourceFileBucket    **src_file_buckets;
  LNK_UDTNameBucket       **udt_name_buckets;
  U64                       line_table_cap;
  U64                       udt_name_buckets_cap;
  U64                       src_file_chunk_cap;
  U64                       symbol_chunk_cap;
  U64                       unit_chunk_cap;
  U64                       inline_site_cap;
  RDIB_LineTable           *null_line_table;
  HashTable                *extern_symbol_voff_ht;
  LNK_CodeViewCompilerInfo *comp_info_arr;
  CV_InlineeLinesAccel    **inlinee_lines_accel_arr;

  RDIB_InlineSiteChunk **inline_site_chunks;

  // output
  RDIB_UnitChunk           *units;
  RDIB_VariableChunkList   *locals;
  RDIB_ScopeChunkList      *scopes;
  RDIB_VariableChunkList   *extern_gvars;
  RDIB_VariableChunkList   *static_gvars;
  RDIB_VariableChunkList   *extern_tvars;
  RDIB_VariableChunkList   *static_tvars;
  RDIB_ProcedureChunkList  *extern_procs;
  RDIB_ProcedureChunkList  *static_procs;
  RDIB_InlineSiteChunkList *inline_sites;
  RDIB_LineTableChunkList  *line_tables;
} LNK_ConvertUnitToRDITask;

////////////////////////////////
// CodeView

internal CV_DebugS *       lnk_parse_debug_s_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, LNK_ChunkList *sect_list_arr);
internal CV_DebugT *       lnk_parse_debug_t_sections(TP_Context *tp, TP_Arena *arena, U64 obj_count, LNK_Obj **obj_arr, LNK_ChunkList *debug_t_list_arr);
internal CV_SymbolList *   lnk_cv_symbol_list_arr_from_debug_s_arr(TP_Context *tp, TP_Arena *arena, U64 obj_count, CV_DebugS *debug_s_arr);
internal LNK_PchInfo *     lnk_setup_pch(Arena *arena, U64 obj_count, LNK_Obj *obj_arr, CV_DebugT *debug_t_arr, CV_DebugT *debug_p_arr, CV_SymbolListArray *parsed_symbols);

internal LNK_CodeViewInput lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, String8List lib_dir_list, LNK_ObjList obj_list);

internal LNK_LeafRef      lnk_leaf_ref(U32 idx, U32 leaf_idx);
internal LNK_LeafRef      lnk_obj_leaf_ref(U32 obj_idx, U32 leaf_idx);
internal LNK_LeafRef      lnk_ts_leaf_ref(CV_TypeIndexSource ti_source, U32 ts_idx, U32 leaf_idx);
internal int              lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b);
internal LNK_LeafLocType  lnk_loc_type_from_leaf_ref(LNK_LeafRef leaf_ref);
internal LNK_LeafLocType  lnk_loc_type_from_obj_idx(LNK_CodeViewInput *input, U64 obj_idx);
internal U64              lnk_loc_idx_from_obj_idx(LNK_CodeViewInput *input, U64 obj_idx);
internal CV_TypeIndex     lnk_ti_lo_from_loc(LNK_CodeViewInput *input, LNK_LeafLocType loc_type, U64 loc_idx, CV_TypeIndexSource ti_source);
internal CV_TypeIndex     lnk_ti_lo_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal String8          lnk_data_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal CV_Leaf          lnk_cv_leaf_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal U128             lnk_hash_from_leaf_ref(LNK_LeafHashes *hashes, LNK_LeafRef leaf_ref);
internal LNK_LeafRef      lnk_leaf_ref_from_loc_idx_and_ti(LNK_CodeViewInput *input, LNK_LeafLocType loc_type, CV_TypeIndexSource ti_source, U64 loc_idx, CV_TypeIndex obj_ti);
internal B32              lnk_match_leaf_ref(LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef a, LNK_LeafRef b);
internal B32              lnk_match_leaf_ref_deep(Arena *arena, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef a, LNK_LeafRef b);
internal U128             lnk_hash_cv_leaf(Arena *arena, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafLocType loc_type, U32 loc_idx, Rng1U64 *ti_ranges, CV_TypeIndex curr_ti, CV_Leaf leaf, CV_TypeIndexInfoList ti_info_list);
internal void             lnk_hash_cv_leaf_deep(Arena *arena, LNK_CodeViewInput *input, Rng1U64 *ti_ranges, CV_DebugT *leaves, LNK_LeafHashes *hashes, LNK_LeafLocType loc_type, U32 loc_idx, CV_TypeIndexInfoList ti_info_list, String8 data);
internal LNK_LeafBucket * lnk_leaf_hash_table_insert_or_update(LNK_LeafHashTable *leaf_ht, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, U128 hash, LNK_LeafBucket *new_bucket);
internal LNK_LeafBucket * lnk_leaf_hash_table_search(LNK_LeafHashTable *ht, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafRef leaf_ref);

internal void                lnk_cv_debug_t_count_leaves_per_source(TP_Context *tp, U64 count, CV_DebugT *debug_t_arr, U64 *per_source_count_arr);
internal void                lnk_hash_debug_t_arr(TP_Context *tp, Arena *arena, U64 obj_count, CV_DebugT *debug_t_arr, U128Array *hash_arr_arr);
internal LNK_LeafBucketArray lnk_present_bucket_array_from_leaf_hash_table(TP_Context *tp, Arena *arena, LNK_LeafHashTable *ht);
internal void                lnk_leaf_bucket_array_sort_radix_subset_parallel(TP_Context *tp, U64 bucket_count, U64 loc_idx_max, LNK_LeafBucket **dst, LNK_LeafBucket **src);
internal void                lnk_leaf_bucket_array_sort_radix_parallel(TP_Context *tp, LNK_LeafBucketArray arr, U64 obj_count, U64 type_server_count);
internal void                lnk_assign_type_indices(TP_Context *tp, LNK_LeafBucketArray bucket_arr, CV_TypeIndex min_type_index);
internal void                lnk_patch_symbols(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafHashTable *leaf_ht_arr);
internal void                lnk_patch_inlines(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafHashTable *leaf_ht_arr, U64 obj_count, CV_DebugS *debug_s_arr);
internal void                lnk_patch_leaves(TP_Context *tp, LNK_CodeViewInput *input, LNK_LeafHashes *hashes, LNK_LeafHashTable *leaf_ht_arr, LNK_LeafBucketArray bucket_arr);
internal String8Node *       lnk_copy_raw_leaf_arr_to_type_server(TP_Context *tp, CV_DebugT types, PDB_TypeServer *type_server);
internal CV_DebugT *         lnk_import_types(TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input);

internal void lnk_replace_type_names_with_hashes(TP_Context *tp, TP_Arena *arena, CV_DebugT debug_t, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name);

////////////////////////////////
// RAD Debug info

internal U64                  lnk_udt_name_hash_table_hash(String8 string);
internal LNK_UDTNameBucket ** lnk_udt_name_hash_table_from_debug_t(TP_Context *tp, TP_Arena *arena, CV_DebugT debug_t, U64 *buckets_cap_out);
internal LNK_UDTNameBucket *  lnk_udt_name_hash_table_lookup(LNK_UDTNameBucket **buckets, U64 cap, String8 name);
internal CV_TypeIndex *       lnk_build_udt_fwdmap(TP_Context *tp, Arena *arena, CV_DebugT debug_t, CV_TypeIndex ti_lo, LNK_UDTNameBucket **udt_name_buckets, U64 udt_name_buckets_cap);

internal RDIB_TypeRef           lnk_rdib_type_from_itype(LNK_ConvertTypesToRDI *task, CV_TypeIndex itype);
internal RDI_MemberKind         lnk_rdib_method_kind_from_cv_prop(CV_MethodProp prop);
internal LNK_SourceFileBucket * lnk_src_file_hash_table_hash(String8 file_path, CV_C13ChecksumKind checksum_kind, String8 checksum_bytes);
internal LNK_SourceFileBucket * lnk_src_file_hash_table_lookup_slot(LNK_SourceFileBucket **src_file_buckets, U64 src_file_buckets_cap, U64 hash, String8 file_path, CV_C13ChecksumKind checksum_kind, String8 checksum_bytes);

internal String8List lnk_build_rad_debug_info(TP_Context               *tp,
                                              TP_Arena                 *tp_arena,
                                              OperatingSystem           os,
                                              RDI_Arch                  arch,
                                              String8                   image_name,
                                              String8                   image_data,
                                              LNK_SectionArray          image_sects,
                                              LNK_Section             **sect_id_map,
                                              U64                       obj_count,
                                              LNK_Obj                  *obj_arr,
                                              CV_DebugS                *debug_s_arr,
                                              U64                       total_symbol_input_count,
                                              LNK_CodeViewSymbolsInput *symbol_inputs,
                                              CV_SymbolListArray       *parsed_symbols,
                                              CV_DebugT                 types[CV_TypeIndexSource_COUNT]);

////////////////////////////////
// PDB

internal LNK_ProcessedCodeViewC11Data lnk_process_c11_data(TP_Context *tp, TP_Arena *arena, U64 obj_count, CV_DebugS *debug_s_arr, U64 string_data_base_offset, CV_StringHashTable string_ht, MSF_Context *msf, PDB_DbiModule **mod_arr);
internal LNK_ProcessedCodeViewC13Data lnk_process_c13_data(TP_Context *tp, TP_Arena *arena, U64 obj_count, CV_DebugS *debug_s_arr, U64 string_data_base_offset, CV_StringHashTable string_ht, MSF_Context *msf, PDB_DbiModule **mod_arr);
internal U64 *                        lnk_hash_cv_symbol_ptr_arr(TP_Context *tp, Arena *arena, CV_SymbolPtrArray arr);
internal CV_SymbolPtrArray            lnk_dedup_gsi_symbols(TP_Context *tp, Arena *arena, PDB_GsiContext *gsi, U64 obj_count, CV_SymbolList *symbol_list_arr);

internal void lnk_build_pdb_public_symbols(TP_Context            *tp,
                                           TP_Arena              *arena,
                                           LNK_SymbolTable       *symtab,
                                           LNK_Section          **sect_id_map,
                                           PDB_PsiContext        *psi);

internal String8List lnk_build_pdb(TP_Context               *tp,
                                   TP_Arena                 *tp_arena,
                                   String8                   image_data,
                                   Guid                      guid,
                                   COFF_MachineType          machine,
                                   COFF_TimeStamp            time_stamp,
                                   U32                       age,
                                   U64                       page_size,
                                   String8                   pdb_name,
                                   String8List               lib_dir_list,
                                   String8List               natvis_list,
                                   LNK_SymbolTable          *symtab,
                                   LNK_Section             **sect_id_map,
                                   U64                       obj_count,
                                   LNK_Obj                  *obj_arr,
                                   CV_DebugS                *debug_s_arr,
                                   U64                       total_symbol_input_count,
                                   LNK_CodeViewSymbolsInput *symbol_inputs,
                                   CV_SymbolListArray       *parsed_symbols,
                                   CV_DebugT                 types[CV_TypeIndexSource_COUNT]);

////////////////////////////////
// RAD Debug Info

internal U64                  lnk_udt_name_hash_table_hash(String8 string);
internal LNK_UDTNameBucket ** lnk_udt_name_hash_table_from_debug_t(TP_Context *tp, TP_Arena *arena, CV_DebugT debug_t, U64 *buckets_cap_out);
internal LNK_UDTNameBucket *  lnk_udt_name_hash_table_lookup(LNK_UDTNameBucket **buckets, U64 cap, String8 name);

internal CV_TypeIndex * lnk_build_udt_fwdmap(TP_Context         *tp,
                                             Arena              *arena,
                                             CV_DebugT           debug_t,
                                             CV_TypeIndex        ti_lo,
                                             LNK_UDTNameBucket **udt_name_buckets,
                                             U64                 udt_name_buckets_cap);

internal void           lnk_init_rdib_itype_map(Arena *arena, RDI_Arch arch, RDIB_Type **itype_map, RDIB_TypeChunkList *rdib_types_list);
internal RDIB_TypeRef   lnk_rdib_type_from_itype(LNK_ConvertTypesToRDI *task, CV_TypeIndex itype);
internal RDI_MemberKind lnk_rdib_method_kind_from_cv_prop(CV_MethodProp prop);

