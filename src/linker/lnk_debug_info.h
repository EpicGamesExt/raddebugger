// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_CObjDecodeWindow LNK_CObjDecodeWindow;

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

// Streaming-ring P2 slice A: deferred .debug$S TI/kind fixup journal. Built inside
// lnk_merge_types while the merge state it needs is still alive (assigned-TI hash tables on
// merge scratch, materialized IPI leaf copies released after pdb_build_types); replayed per
// obj at the START of the module-write visit (lnk_write_pdb_modules sizing loop), before any
// consumer reads the obj's $S bytes. Entries alias the obj's own $S backing (patched copies /
// raw maps); replay applies entries in build order, so the byte end-state is identical to the
// old in-place lnk_cv_patcher_symbols / lnk_cv_patcher_inlines / lnk_fixup_symbols passes.
// Entries are 8B, keyed per NODE run: the target is a byte offset relative to the run's
// subsection-node base (symbols: the LNK_SymbolInput node; inlinees: each InlineeLines
// data_list node in list order), so no pointer is stored. off:31 | width:1 (set = 32-bit
// type-index write, clear = 16-bit kind rewrite). A node >= 2GiB cannot encode -> that run
// falls back to 16B wide entries (per-run is_wide flag), decided up front from the node size.
typedef struct LNK_DebugSPatch     { U32 off_w; U32 value; }           LNK_DebugSPatch;
typedef struct LNK_DebugSPatchWide { U64 off; U32 value; U32 size; }   LNK_DebugSPatchWide;
typedef struct LNK_DebugSPatchArray
{
  void *v; // LNK_DebugSPatch[] or LNK_DebugSPatchWide[] (is_wide)
  U64   count;
  B32   is_wide;
} LNK_DebugSPatchArray;
typedef struct LNK_DebugSInlineJournal
{
  LNK_DebugSPatchArray patches;     // all InlineeLines entries of the obj, node runs in data_list order
  U32                 *node_counts; // [InlineeLines node_count] entries per node run
} LNK_DebugSInlineJournal;

// $T streaming (ring P4): per-real-obj journal of LF_NOTYPE kind rewrites. Raw mapped
// .debug$T/$P views are never written anymore (writes used to CoW-dirty one input page per
// discard); instead the rewrite is journaled here and mirrored into CV_DebugT.tags, and the
// downstream byte readers (hash current-leaf reads, unique-leaf sizing, materialize) replay it.
// Entry = leaf_idx | optional KIND_ONLY flag:
//   full rewrite (default): header becomes { size=sizeof(CV_LeafKind), kind=LF_NOTYPE } -- the
//     ENDPRECOMP strip and the invalid-TI/cyclic discards in lnk_hash_cv_leaf;
//   KIND_ONLY: only the kind becomes LF_NOTYPE, size/payload preserved -- the LF_IFC_RECORD
//     (0x1522) placeholder rewrite in lnk_ifc_resolve_task.
// Entries are kept sorted by leaf_idx (sorted insert; pushes are single-threaded per obj).
// Pseudo objs (type servers, .ifc blobs; obj_idx >= obj_count) keep their in-place writes:
// their $T is arena-backed, not a mapped input view.
//
// HOT-PATH CONTRACT: readers stay on the ORIGINAL raw-byte code path unless the per-obj
// bitmap says the leaf is journaled. `bitmap == 0` (the overwhelmingly common case: journals
// exist only for $P objs, IFC consumers, and invalid-input error paths) costs one load+branch;
// only journaled leaves pay the bsearch + reconstruct.
#define LNK_NOTYPE_JOURNAL_KIND_ONLY (1u << 31)
typedef struct LNK_NotypeJournal
{
  U32 *v; // (leaf_idx | LNK_NOTYPE_JOURNAL_KIND_ONLY?) ascending by leaf_idx
  U32  count;
  U32  cap;
  U64 *bitmap;  // 1 bit / parse-time leaf slot; 0 until the first push
  U64  bit_cap; // bits in bitmap
} LNK_NotypeJournal;

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
  U32Array    *debug_s_sect_idx_arr; // per obj: 0-based sect_idx of each debug_s_list_arr node (provenance tagging)

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
  Rng1U64             *symbol_input_ranges; // [symbol_input_range_count]
  U64                  symbol_patch_task_count; //
  LNK_SymbolInputTask *symbol_patch_task; // [symbol_patch_task_count]
  // FAIR-SHARE: fixed lane count symbol_input_ranges was built for (full pool width at build
  // time). Barrier passes may run at a pinned cohort C < this; they must walk lanes
  // [task_id, symbol_input_range_count) strided by the cohort, NOT index by task_id alone.
  U64              symbol_input_range_count;

  // deferred $S TI/kind fixup journal (see LNK_DebugSPatch). Null / flag == 0 when the merge
  // ran with LNK_MergeTypeFlag_SkipSymbolTypeFixup, or after the journal was consumed
  // (lnk_release_debug_s_fixup_journal: eager path, or end of the module-write pass).
  // All journal storage (entry arrays + the three tables below) lives inside dedicated
  // per-worker DEBUG_S_FIXUP_JOURNAL arenas, handed to the background reaper at consume
  // time -- GB-class at FN scale, dead after the last per-obj replay.
  B32                      has_debug_s_fixup_journal;
  TP_Arena                *debug_s_fixup_journal_arenas;
  LNK_DebugSPatchArray    *debug_s_sym_fixups;        // [symbol_input_count] one node run per symbol input
  LNK_DebugSInlineJournal *debug_s_inline_fixups;     // [count]
  U64                     *debug_s_sym_fixup_offsets; // [count+1] obj -> symbol_inputs range (inputs are obj-contiguous)

  // IFC (header-unit debug-record) resolution:
  // redirects a consuming obj's local LF_IFC_RECORD placeholder TI to a leaf in
  // an injected .ifc debug-records blob "obj". Consulted first in lnk_leaf_ref_from_ti.
  // key   = Compose64Bit(obj_idx, local_ti)
  // value = Compose64Bit(blob_obj_idx, blob_leaf_idx)
  B32      has_ifc_redirects;
  HashMap  ifc_redirect_hm;
  Rng1U64  ifc_obj_range; // [min,max) range of injected blob objs in the parallel arrays
  U32Array ifc_indices;   // obj indices of injected .ifc blob objs (hashed/deduped first)
  // exact per-obj key filter for ifc_redirect_hm: bit set iff Compose64Bit(obj_idx, ti) was pushed.
  // lets lnk_leaf_ref_from_ti skip the (miss-dominated) hash-map search entirely; on a set bit the
  // original map is searched unchanged, so results are bit-identical to always searching.
  U64    **ifc_redirect_bits;   // [count]; null == obj has no redirect keys
  Rng1U64 *ifc_redirect_ti_rng; // [count]; [min,max) local-TI span covered by the obj's bitset

  // $T streaming (ring P4): [obj_count] NOTYPE-rewrite journals, real objs only (see
  // LNK_NotypeJournal). Pseudo objs at [obj_count, count) mutate their arena-backed $T in place.
  LNK_NotypeJournal *notype_journal;
} LNK_CodeViewInput;

typedef struct
{
  LNK_CodeViewInput *input;
  String8Array      *raw_types; // [obj_count]
  CV_DebugT         *out_types; // [obj_count]
  B32                is_debug_p;
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

#define LNK_ASSIGNED_TI_ENTRY_SIZE 12
#define LNK_ASSIGNED_TI_HASH_OFF    0
#define LNK_ASSIGNED_TI_TI_OFF      8

typedef struct
{
  U64 cap;
  U8 *v;
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

  // standalone backing for the merged leaf bytes v[] points into (one per source);
  // owned by the caller, released after pdb_build_types on the linker path
  String8 leaf_buffers[CV_TypeIndexSource_COUNT];
} LNK_MergedTypes;

typedef struct
{
  LNK_CodeViewInput  *input;
  CV_DebugS          *debug_s_arr;
  LNK_LeafHashTable   leaf_ht_arr[CV_TypeIndexSource_COUNT];
  LNK_AssignedTiHash  assigned_ti_arr[CV_TypeIndexSource_COUNT];
  Arena             **fixed_arenas;
  struct LNK_CObjDecodeWindow *decode_windows;
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

  // deterministic unique-leaf estimate: distinct-hash bitmaps (per ti source) filled with
  // commutative atomic ORs over the precomputed debug_h hashes -> same input, same bits, same
  // estimate every run. sized pow2 so bit index is hash & (bits-1).
  U32 *estimate_bitmap     [CV_TypeIndexSource_COUNT]; // [estimate_bitmap_bits/32]
  U64  estimate_bitmap_bits[CV_TypeIndexSource_COUNT]; // pow2

  // set when a probe wraps without finding a slot (estimate-sized table overflowed); dedup is
  // retried once with the always-sufficient total-based caps. deterministic: overflow happens
  // iff the unique count exceeds cap, which is a function of the input alone.
  U32  leaf_ht_overflow;

  // materialize unique leaves (unbucket + leaf TI-fixup fused, applied to a private copy so the
  // fixup never dirties the copy-on-write input mapping)
  U64 *leaf_buffer_offsets; // [worker_count+1] per-lane byte offsets into leaf_buffer
  U8  *leaf_buffer;
  U64 *materialize_obj_offsets[CV_TypeIndexSource_COUNT]; // [input->count+1], sorted-ref ranges

  // Compressed winner payload prefetch. segment_offsets maps an OBJ-local segment index to a
  // compact global bit/key index. Copy-present atomically claims segments as it discovers the
  // winning leaves, so no later scan of the multi-million-entry winner arrays is needed.
  U64 *winner_segment_offsets; // [input->obj_count+1]
  U64 *winner_segment_bitmap;
  U64 *winner_segment_worker_bitmaps; // [worker_count * winner_segment_word_count]
  U64  winner_segment_word_count;
  U64 *winner_segment_keys;    // obj_idx:32 | segment_idx:32
  U64  winner_segment_key_count;

  // $S fixup journal build: per-worker arenas the LNK_DebugSPatch entry arrays land on
  // (must outlive the merge -- replay happens at module write)
  TP_Arena *journal_arena;

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

// Streaming-ring P2b: per-obj global-symbol counts + proc-refs. Global-symbol records are
// content-deduped directly from the one module-write window and only exact winners are copied
// to surviving storage; no candidate metadata or payload survives the window. Proc-ref
// payloads AND the per-obj procref value/hash arrays live on the surviving
// procref_payload_arenas (consumed post-release by lnk_move_global_symbols_to_gsi, which
// flattens per-obj segments in ascending obj-index order -- deterministic and
// cohort-independent -- and otherwise reads only the materialized winner records).
typedef struct
{
  U64        cand_count;     // global records (cv_is_global_symbol + top-level typedefs)
  U64        procref_count;  // GPROC32/LPROC32 records
  CV_Symbol *procref_syms;   // [procref_count] cv_make_proc_ref results (arrays + payload on survive arenas)
  U32       *procref_hashes; // [procref_count] gsi_hash(name)
} LNK_GsiPreExtractObj;

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
  struct LNK_PdbOutput *output; // when non-null, module streams enqueue to the background writer as they complete

  // when set, lnk_build_pdb drops every obj's patched debug-section copies
  // (LNK_Obj.section_data_copies) and releases the SECT_DATA_COPIES arenas at the END of the
  // module-write phase, right after direct winner dedup/materialization (the last $S reader
  // on this path). Must be 0 when a /PDBSTRIPPED build follows -- it re-walks
  // cv->debug_s_arr after lnk_build_pdb. P3.1: the /names bucket rehome and the
  // mod->source_file_list repoint are UNCONDITIONAL (they must also cover string tables in
  // reloc-free RAW-MAPPED $S sections, which never had copies to begin with).
  B32 free_sect_copies;

  // P2b pre-extraction state (see LNK_GsiPreExtractObj). procref_payload_arenas hold exact
  // global-symbol winners plus the procref value/hash arrays and cv_make_proc_ref payloads.
  // They must survive until GSI serialization
  // (like the old proc_ref_arenas, they live to process exit).
  LNK_GsiPreExtractObj *preext;                 // [cv->obj_count]
  Arena               **procref_payload_arenas; // [preext_arena_count]
  U64                   preext_arena_count;     // Write Modules cohort width

  // Transient exact-content set used only inside Write Modules. Empty slots are claimed with
  // a reservation sentinel before a winner is copied, so duplicates allocate no payload.
  U64                   gsi_dedup_bucket_cap;
  void                **gsi_dedup_buckets;

  // Direct exact-dedup output: the winner pointer array is compacted from hash-table slot
  // order onto gsi->arena; payload bytes remain on the per-worker surviving arenas above.
  // Both stay alive through GSI serialization. This is the only global-symbol input read by
  // lnk_move_global_symbols_to_gsi.
  U64    gsi_winner_count;
  void **gsi_winner_ptrs; // [gsi_winner_count]
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
internal void            lnk_notype_journal_push             (Arena *arena, LNK_NotypeJournal *journal, U32 leaf_idx, B32 kind_only, U64 bit_cap);
internal B32             lnk_notype_journal_test             (LNK_NotypeJournal *journal, U64 leaf_idx);
internal B32             lnk_notype_journal_find             (LNK_NotypeJournal *journal, U32 leaf_idx, B32 *kind_only_out);
internal CV_Leaf         lnk_cv_leaf_from_leaf_ref           (Arena *arena, LNK_CObjDecodeWindow *decode_window, LNK_CodeViewInput *input, U32 obj_idx, U32 leaf_idx);
internal U64             lnk_leaf_ref_materialize_meta       (LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal U64             lnk_hash_cv_leaf                    (LNK_CodeViewInput *input, Arena *journal_arena, LNK_LeafRef leaf_ref, CV_Leaf leaf, CV_TiOffsets ti_offs, B32 discard_cycles);
internal void            lnk_hash_cv_leaf_deep               (Arena *arena, LNK_CObjDecodeWindow *decode_window, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref, CV_TiOffsets ti_offs);
internal CV_TypeIndex    lnk_assigned_ti_hash_search          (LNK_AssignedTiHash *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref);
internal LNK_MergedTypes lnk_merge_types                     (TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input, LNK_MergeTypeFlags merge_flags);
internal void            lnk_apply_debug_s_fixups_for_obj    (LNK_CodeViewInput *cv, U64 obj_idx);
internal void            lnk_apply_debug_s_fixups_eager      (TP_Context *tp, LNK_CodeViewInput *cv);
internal CV_DebugS       lnk_obj_window_debug_s              (Arena *arena, LNK_CodeViewInput *cv, U64 obj_idx, U64 image_base, COFF_SectionHeader **image_section_table, B32 symbols_only);
internal void            lnk_release_debug_s_fixup_journal   (LNK_CodeViewInput *cv);
internal void            lnk_replace_type_names_with_hashes  (TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name);

////////////////////////////////
// PDB

internal void             lnk_gc_types (TP_Context *tp, Arena *arena, LNK_CodeViewInput *cv, LNK_MergedTypes *types);
struct LNK_Inputer;
internal LNK_FileArtifact lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types, LNK_PdbWriter writer, LNK_PDB_BuilderFlags builder_flags, struct LNK_Inputer *inputer);
