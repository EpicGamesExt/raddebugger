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
  LNK_Config  *config;
  U64          obj_count;
  B32          is_stripped;

  U64           count;
  LNK_Obj     **obj_arr;
  CV_DebugS    *debug_s_arr;
  CV_DebugT    *debug_t_arr;
  CV_DebugH    *debug_h_arr;
  U64          *obj_to_ts;

  String8List *debug_s_list_arr;
  String8List *debug_p_list_arr;
  String8List *debug_t_list_arr;
  String8List *debug_h_list_arr;

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
  PDB_DbiModule      **mod_arr;     // [obj_count]
  U32Array            *obj_indices; // [obj_count]

  U64 symbol_count;

  // push DBI SC Map
  PE_BinInfo                  pe;
  COFF_SectionHeader        **image_section_table;
  U64                         image_section_table_count;
  Rng1U64Array                image_section_file_ranges;
  Rng1U64Array                image_section_virt_ranges;
  PDB_DbiSectionContribList  *sc_list; // [obj_count]
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
// CodeView

internal LNK_CodeViewInput lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 objs_count, LNK_Obj **objs);

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

internal String8List lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types, LNK_PDB_BuilderFlags builder_flags);

