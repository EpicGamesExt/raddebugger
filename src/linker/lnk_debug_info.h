// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////
// RRT

global read_only String8 g_rrt_magic   = str8_lit_comp("RAD-TYPE-SERVER\0");
global read_only U64     g_rrt_version = 3;

typedef struct LNK_RRT
{
  String8 path;
  LNK_HashKind debug_types_hash;

  String8 type_data_raw;
  union {
    String8    type_hashes;
    U64      **type_hashes_unpacked[CV_TypeIndexSource_COUNT];
  };
  Rng1U64  type_data_ranges[CV_TypeIndexSource_COUNT];
  String8  type_data       [CV_TypeIndexSource_COUNT];
  Rng1U64  ti_ranges       [CV_TypeIndexSource_COUNT];

  U64              obj_count;
  U64             *obj_leaf_counts;
  U64             *obj_time_stamps;
  Rng1U64         *obj_ti_ranges;
  CV_TypeIndex   **obj_ti_maps;
  String8Array     obj_paths;
  Rng1U64         *obj_pch_ti_ranges;
  U32             *obj_pch_indices;
} LNK_RRT;

typedef struct
{
  U64      count;
  LNK_RRT *v;
} LNK_RRT_Array;

////////////////////////////////
// CodeView

typedef enum
{
  LNK_TypeServerKind_Null,
  LNK_TypeServerKind_PDB,
  LNK_TypeServerKind_RRT,
} LNK_TypeServerKind;

typedef struct
{
  CV_TypeServerInfo  ts_info;
  U64                ts_idx;
  LNK_TypeServerKind ts_kind;
  String8            ts_path;
  U64List            obj_indices;
  LNK_RRT           *rrt;
  CV_DebugH         *debug_h;
} LNK_TypeServer;

typedef struct LNK_TypeServerNode  { LNK_TypeServer v; struct LNK_TypeServerNode *next; } LNK_TypeServerNode;
typedef struct LNK_TypeServerList  { U64 count; LNK_TypeServerNode *first, *last;       } LNK_TypeServerList;
typedef struct LNK_TypeServerArray { U64 count; LNK_TypeServer *v;                      } LNK_TypeServerArray;

typedef struct LNK_SymbolInput
{
  U64     obj_idx;
  String8 raw_symbols;
} LNK_SymbolInput;

typedef struct LNK_SymbolInputTask
{
  Rng1U64 input_range;
  U64     weight;
} LNK_SymbolInputTask;

typedef struct
{
  LNK_Config  *config;
  U64          obj_count;
  B32          is_stripped;

  LNK_RRT_Array rrt_input;

  U64           count;
  LNK_Obj     **obj_arr;
  CV_DebugS    *debug_s_arr;
  CV_DebugT    *debug_t_arr;
  CV_DebugH    *debug_h_arr;
  U64          *obj_to_ts;

  String8List *debug_s_list_arr;

  U32Array int_obj_indices;
  U32Array ext_obj_indices;
  U32Array debug_p_indices;
  U32Array type_server_indices;

  Rng1U64              ts_obj_range;
  LNK_TypeServerArray  ts_arr;
  B32                 *is_type_server_discarded; // [ts_arr.count]
  CV_TypeIndex         min_type_indices[CV_TypeIndexSource_COUNT];

  U64                  symbol_input_count;
  LNK_SymbolInput     *symbol_inputs;       // [symbol_input_count]
  Rng1U64             *symbol_input_ranges; // [worker_count]
  U64                  symbol_patch_task_count; //
  LNK_SymbolInputTask *symbol_patch_task; // [symbol_patch_task_count]
} LNK_CodeViewInput;

typedef struct
{
  LNK_CodeViewInput *input;
  String8Array      *raw_types; // [obj_count]
  CV_DebugT         *out_types; // [obj_count]
} LNK_ParseCvTypes;

////////////////////////////////
// Type Merging

typedef U64 LNK_LeafRef;
typedef struct { U64 count; LNK_LeafRef *v; } LNK_LeafRefArray;

#define LNK_LEAF_REF_NULL max_U64

typedef struct
{
  U64          cap;
  LNK_LeafRef *bucket_arr;
} LNK_LeafHashTable;

typedef struct
{
  U64           cap;
  CV_TypeIndex *ti_arr;
  U64          *hash_arr;
} LNK_AssignedTiHash;

typedef struct LNK_LeafRange
{
  struct LNK_LeafRange *next;
  Rng1U64               range;
  CV_DebugT            *debug_t;
} LNK_LeafRange;
typedef struct { U64 count; LNK_LeafRange *first, *last; } LNK_LeafRangeList;

typedef enum
{
  LNK_MergeTypeFlag_BuildObjTiMap       = (1 << 0),
  LNK_MergeTypeFlag_SkipSymbolTypeFixup = (1 << 1),
  LNK_MergeTypeFlag_ExportHashes        = (1 << 2),
} LNK_MergeTypeFlags;

typedef struct
{
  CV_TypeIndex  min_type_indices[CV_TypeIndexSource_COUNT];
  U64           count           [CV_TypeIndexSource_COUNT];
  U8          **v               [CV_TypeIndexSource_COUNT];

  // @type_server
  CV_TypeIndex **obj_ti_maps;
  U64           *hashes[CV_TypeIndexSource_COUNT];
} LNK_MergedTypes;

typedef struct
{
  LNK_CodeViewInput  *input;
  CV_DebugS          *debug_s_arr;
  LNK_LeafHashTable   leaf_ht_arr[CV_TypeIndexSource_COUNT];
  LNK_AssignedTiHash  assigned_ti_arr[CV_TypeIndexSource_COUNT];
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

  CV_TypeIndex     min_type_indices    [CV_TypeIndexSource_COUNT];
  LNK_LeafRefArray unique_leaf_refs_arr[CV_TypeIndexSource_COUNT];

  U64          *obj_ti_map_counts;
  U64          *obj_ti_map_offsets;
  CV_TypeIndex *obj_ti_batch;

  U64        pop_obj_idx;
  Rng1U64   *pop_range;

  LNK_MergedTypes result;
} LNK_MergeTypes;

////////////////////////////////
// PDB

typedef enum
{
  LNK_PDB_BuilderFlag_All         = 0,
  LNK_PDB_BuilderFlag_Tpi         = (1<<0),
  LNK_PDB_BuilderFlag_Ipi         = (1<<1),
  LNK_PDB_BuilderFlag_Modules     = (1<<2),
  LNK_PDB_BuilderFlag_SC          = (1<<4),
  LNK_PDB_BuilderFlag_NATVIS      = (1<<5),
} LNK_PDB_BuilderFlags;

typedef struct
{
  String8              image_data;
  LNK_SymbolTable     *symtab;
  LNK_CodeViewInput   *cv;
  PDB_Context         *pdb;
  CV_StringHashTable   string_ht;
  U64                  string_table_base_offset;
  PDB_DbiModule      **mod_arr;     // [obj_count]
  U32Array            *obj_indices; // [obj_count]

  U64 symbol_count;

  // push DBI SC Map
  PE_BinInfo           pe;
  COFF_SectionHeader **image_section_table;
  U64                  image_section_table_count;
  Rng1U64Array         image_section_virt_ranges;
  Rng1U64Array         image_section_file_ranges;
  U64                 *image_section_file_section_numbers;
  PDB_DbiSCArray      *sc_arrays; // [obj_count]
} LNK_BuildPdb;

typedef struct
{
  LNK_BackgroundFileWriter *file_writer;
  String8                   output_path;
  String8                   temp_output_path;
} LNK_PdbWriter;

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
// RRT

internal String8List lnk_string_list_from_rrt(Arena *arena, LNK_RRT *rrt);
internal B32         lnk_rrt_from_string     (Arena *arena, String8 rrt_data, String8 path, LNK_RRT *rrt_out);

////////////////////////////////
// CodeView

internal LNK_CodeViewInput lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 objs_count, LNK_Obj **objs, LNK_RRT_Array rrt_input);

internal int             lnk_leaf_ref_compare                (LNK_LeafRef a, LNK_LeafRef b);
internal B32             lnk_match_leaf_ref                  (LNK_CodeViewInput *input, LNK_LeafRef a, LNK_LeafRef b);
internal U64             lnk_hash_cv_leaf                    (LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list, B32 discard_cycles);
internal void            lnk_hash_cv_leaf_deep               (Arena *arena, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TypeIndexInfoList ti_info_list);
internal CV_TypeIndex    lnk_assigned_ti_hash_search          (LNK_AssignedTiHash *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal LNK_MergedTypes lnk_merge_types                     (TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input, LNK_MergeTypeFlags merge_flags);
internal void            lnk_replace_type_names_with_hashes  (TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name);

////////////////////////////////
// PDB

internal void             lnk_gc_types (TP_Context *tp, Arena *arena, LNK_CodeViewInput *cv, LNK_MergedTypes *types);
internal LNK_FileArtifact lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types, LNK_PdbWriter writer, LNK_PDB_BuilderFlags builder_flags);
