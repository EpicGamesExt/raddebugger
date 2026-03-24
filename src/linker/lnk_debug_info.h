// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////
// CodeView

typedef struct
{
  CV_TypeServerInfo ts_info;
  U64               ts_idx;
  String8           ts_path;
  U64List           obj_indices;
} LNK_TypeServer;
typedef struct LNK_TypeServerNode  { LNK_TypeServer v; struct LNK_TypeServerNode *next; } LNK_TypeServerNode;
typedef struct LNK_TypeServerList  { U64 count; LNK_TypeServerNode *first, *last;       } LNK_TypeServerList;
typedef struct LNK_TypeServerArray { U64 count; LNK_TypeServer *v;                      } LNK_TypeServerArray;

typedef struct LNK_SymbolInput
{
  U64     obj_idx;
  String8 raw_symbols;
} LNK_SymbolInput;

typedef struct
{
  LNK_IO_Flags io_flags;
  U64          obj_count;

  U64           count;
  LNK_Obj     **obj_arr;
  CV_DebugS    *debug_s_arr;
  CV_DebugT    *debug_t_arr;
  CV_DebugH    *debug_h_arr;
  U64          *obj_to_ts;

  String8List *debug_s_list_arr;
  String8List *debug_p_list_arr;
  String8List *debug_t_list_arr;

  U32Array int_obj_indices;
  U32Array ext_obj_indices;
  U32Array debug_p_indices;
  U32Array type_server_indices;

  Rng1U64              ts_obj_range;
  LNK_TypeServerArray  ts_arr;
  B32                 *is_type_server_discarded; // [ts_arr.count]
  CV_TypeIndex         min_type_indices[CV_TypeIndexSource_COUNT];

  U64              symbol_input_count;
  LNK_SymbolInput *symbol_inputs;       // [symbol_input_count]
  Rng1U64         *symbol_input_ranges; // [worker_count]
} LNK_CodeViewInput;

typedef struct
{
  LNK_CodeViewInput *input;
  String8Array      *raw_types; // [obj_count]
  CV_DebugT         *out_types; // [obj_count]
} LNK_ParseCvTypes;

////////////////////////////////
// Type Merging

typedef struct { U32 obj_idx; U32 leaf_idx;  } LNK_LeafRef;
typedef struct { U64 count; LNK_LeafRef **v; } LNK_LeafRefArray;

typedef struct
{
  U64           cap;
  LNK_LeafRef **bucket_arr;
} LNK_LeafHashTable;

typedef struct LNK_LeafRange
{
  struct LNK_LeafRange *next;
  Rng1U64               range;
  CV_DebugT            *debug_t;
} LNK_LeafRange;
typedef struct { U64 count; LNK_LeafRange *first, *last; } LNK_LeafRangeList;

typedef struct
{
  CV_TypeIndex min_type_indices[CV_TypeIndexSource_COUNT];
  U64   count[CV_TypeIndexSource_COUNT];
  U8  **v    [CV_TypeIndexSource_COUNT];
} LNK_MergedTypes;

typedef struct
{
  LNK_CodeViewInput  *input;
  CV_DebugS          *debug_s_arr;
  LNK_LeafHashTable   leaf_ht_arr[CV_TypeIndexSource_COUNT];
  Arena             **fixed_arenas;
  CV_TypeIndexSource  ti_source;
  U32Array            indices;
  Rng1U64            *ranges;

  // count types per source
  LNK_LeafRangeList *leaf_ranges_per_task;
  U64                per_source_count[CV_TypeIndexSource_COUNT];

  // extract present buckets
  U64 *counts [CV_TypeIndexSource_COUNT];
  U64 *offsets[CV_TypeIndexSource_COUNT];

  // leaf ref radix sort
  U64           obj_idx_bit_count_0;
  U64           obj_idx_bit_count_1;
  U64           obj_idx_bit_count_2;
  U64           counts_max;
  U32         **counts_arr;
  LNK_LeafRef **dst;
  LNK_LeafRef **src;
  U64           pass_idx;

  // assign type indices
  U64                 assigned_type_caps  [CV_TypeIndexSource_COUNT];
  CV_TypeIndex       *assigned_type_hts   [CV_TypeIndexSource_COUNT];
  CV_TypeIndex        min_type_indices    [CV_TypeIndexSource_COUNT];
  LNK_LeafRefArray    unique_leaf_refs_arr[CV_TypeIndexSource_COUNT];

  LNK_MergedTypes result;
} LNK_MergeTypes;

////////////////////////////////
// PDB

typedef struct
{
  String8            image_data;
  LNK_SymbolTable   *symtab;
  LNK_CodeViewInput *cv;

  PDB_Context    *pdb;
  PDB_DbiModule **mod_arr; // [obj_count]

  Arena **fixed_arenas;

  // GSI symbol dedup
  U64    global_symbol_count;
  U64    bucket_cap;
  void **buckets;      // [bucket_count]
  U64   *insert_count; // [worker_count]

  Rng1U64 *symbol_ranges; // [worker_count]
  U64      symbol_count;
  void   **symbol_arr;
  U32     *symbol_hashes; // [symbol_count]

  U64           *proc_ref_counts; // [worker_count]
  U64           *proc_ref_sizes;  // [worker_count]
  U64           *proc_ref_hashes; // [total_proc_ref_count]
  U64           *proc_ref_offs;   // [worker_count]
  String8        proc_refs;
  U64            proc_ref_count;
  CV_SymbolNode *proc_ref_nodes;  // [proc_ref_count]
  Arena         *proc_ref_arena;

  U64            *public_symbol_sizes;        // [worker_count]
  U64            *public_symbol_offs;         // [worker_count]
  U8             *public_symbol_buffer;
  U64            *public_symbol_node_counts;  // [worker_count]
  U64            *public_symbol_node_offsets; // [worker_count]
  CV_SymbolNode  *public_symbol_nodes;        // [public_symbol_total_count]
  CV_SymbolList  *public_symbols;             // [worker_count]
  U32           **public_symbol_hashes;       // [worker_count][public_symbol.count]

  U64 *symbol_sizes; // [obj_count]

  // process C13 data
  String8List        *source_file_names_list_arr;
  CV_StringHashTable  string_ht;

  // build DBI modules
  String8List *globrefs_arr;                 // [obj_count]
  U32         *mod_sizes;                    // [obj_count]
  U64         *serialized_symbol_data_sizes; // [obj_count]

  // push DBI SC Map
  PE_BinInfo                  pe;
  COFF_SectionHeader        **image_section_table;
  U64                         image_section_table_count;
  Rng1U64Array                image_section_file_ranges;
  Rng1U64Array                image_section_virt_ranges;
  PDB_DbiSectionContribList  *sc_list; // [obj_count]

  // make public symbols
  CV_SymbolList *pub_list_arr;
} LNK_BuildPdb;

typedef struct
{
  U64          leaf_count;
  U8         **leaf_arr;
  Rng1U64     *ranges;
  U64          hash_length;
  B32          make_map;
  TP_Arena    *map_arena;
  String8List *maps;
} LNK_TypeNameReplacer;

////////////////////////////////
// RDI

typedef struct
{
  String8 name;
  U64     leaf_idx;
} LNK_UDTNameBucket;

typedef struct
{
  U64                 leaf_count;
  U8                **leaf_arr;
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

typedef struct
{
  U64                      leaf_count[CV_TypeIndexSource_COUNT];
  U8                     **leaf_arr[CV_TypeIndexSource_COUNT];
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
  LNK_Obj              **obj_arr;
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
  COFF_SectionHeaderArray   image_sects;
  LNK_Obj                 **obj_arr;
  CV_DebugS                *debug_s_arr;
  U64                       leaf_arr_count_ipi;
  U8                      **leaf_arr_ipi;
  LNK_SymbolInput          *symbol_inputs;
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

internal LNK_CodeViewInput lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_IO_Flags io_flags, String8List lib_dir_list, String8List alt_pch_dirs, U64 objs_count, LNK_Obj **objs);

internal int             lnk_leaf_ref_compare                (LNK_LeafRef a, LNK_LeafRef b);
internal int             lnk_leaf_ref_is_before              (void *raw_a, void *raw_b);
internal B32             lnk_match_leaf_ref                  (LNK_CodeViewInput *input, LNK_LeafRef a, LNK_LeafRef b);
internal U64             lnk_hash_cv_leaf                    (LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list, B32 discard_cycles);
internal void            lnk_hash_cv_leaf_deep               (Arena *arena, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list);
internal LNK_LeafRef *   lnk_leaf_hash_table_insert_or_update(LNK_LeafHashTable *leaf_ht, LNK_CodeViewInput *input, CV_DebugH *hashes, U64 hash, LNK_LeafRef *new_bucket);
internal LNK_LeafRef *   lnk_leaf_hash_table_search          (LNK_LeafHashTable *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal LNK_MergedTypes lnk_merge_types                     (TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input);
internal void            lnk_replace_type_names_with_hashes  (TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name);

////////////////////////////////
// PDB

internal String8List lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types);

////////////////////////////////
// RDI

internal U64                  lnk_udt_name_hash_table_hash         (String8 string);
internal LNK_UDTNameBucket ** lnk_udt_name_hash_table_from_leaf_arr(TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, U64 *buckets_cap_out);
internal LNK_UDTNameBucket *  lnk_udt_name_hash_table_lookup       (LNK_UDTNameBucket **buckets, U64 cap, String8 name);
internal CV_TypeIndex *       lnk_build_udt_fwdmap                 (TP_Context *tp, Arena *arena, CV_DebugT debug_t, CV_TypeIndex ti_lo, LNK_UDTNameBucket **udt_name_buckets, U64 udt_name_buckets_cap);

internal RDIB_TypeRef           lnk_rdib_type_from_itype           (LNK_ConvertTypesToRDI *task, CV_TypeIndex itype);
internal RDI_MemberKind         lnk_rdib_method_kind_from_cv_prop  (CV_MethodProp prop);
internal LNK_SourceFileBucket * lnk_src_file_hash_table_hash       (String8 file_path, CV_C13ChecksumKind checksum_kind, String8 checksum_bytes);
internal LNK_SourceFileBucket * lnk_src_file_hash_table_lookup_slot(LNK_SourceFileBucket **src_file_buckets, U64 src_file_buckets_cap, U64 hash, String8 file_path, CV_C13ChecksumKind checksum_kind, String8 checksum_bytes);

internal String8List
lnk_build_rad_debug_info(TP_Context               *tp,
                         TP_Arena                 *tp_arena,
                         OperatingSystem           os,
                         RDI_Arch                  arch,
                         String8                   image_name,
                         String8                   image_data,
                         U64                       obj_count,
                         LNK_Obj                 **obj_arr,
                         CV_DebugS                *debug_s_arr,
                         U64                       symbol_input_count,
                         LNK_SymbolInput *symbol_inputs,
                         CV_SymbolListArray       *parsed_symbols,
                         LNK_MergedTypes           types);

internal U64                  lnk_udt_name_hash_table_hash        (String8 string);
internal LNK_UDTNameBucket ** lnk_udt_name_hash_table_from_leaf_arr(TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, U64 *buckets_cap_out);
internal LNK_UDTNameBucket *  lnk_udt_name_hash_table_lookup      (LNK_UDTNameBucket **buckets, U64 cap, String8 name);

internal CV_TypeIndex * lnk_build_udt_fwdmap(TP_Context         *tp,
                                             Arena              *arena,
                                             CV_DebugT           debug_t,
                                             CV_TypeIndex        ti_lo,
                                             LNK_UDTNameBucket **udt_name_buckets,
                                             U64                 udt_name_buckets_cap);

internal void           lnk_init_rdib_itype_map          (Arena *arena, RDI_Arch arch, RDIB_Type **itype_map, RDIB_TypeChunkList *rdib_types_list);
internal RDIB_TypeRef   lnk_rdib_type_from_itype         (LNK_ConvertTypesToRDI *task, CV_TypeIndex itype);
internal RDI_MemberKind lnk_rdib_method_kind_from_cv_prop(CV_MethodProp prop);

