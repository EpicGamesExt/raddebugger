#pragma once

// The portable container layout is codec-independent and is also needed by the no-Oodle stubs.
// Oodle headers and calls remain private to the enabled branch in lnk_compressed_obj.c.
#include "lnk_compressed_obj_format.h"

typedef struct LNK_CompressedObj LNK_CompressedObj;
typedef struct LNK_CObjBaseRelocEntry LNK_CObjBaseRelocEntry;
typedef struct LNK_CObjDebugSEntry LNK_CObjDebugSEntry;
typedef struct LNK_CObjDebugSSummary LNK_CObjDebugSSummary;

typedef struct LNK_CObjTypeIndexView
{
  U64  count;
  U32 *offsets;
  U16 *sizes;
  U16 *kinds;
  U32 *packed_v2_offset_groups;
  U8  *packed_v2_offset_payload;
  U16 *packed_kind_dictionary;
  U8  *packed_kind_codes;
  U64 *complete_udt_hashes;
  U64  complete_udt_hash_count;
  U8   offset_checkpoint_shift;
  B8   packed_sidecar;
} LNK_CObjTypeIndexView;

typedef struct LNK_CObjBaseRelocView
{
  U32 count;
  LNK_CObjBaseRelocEntry *v;
} LNK_CObjBaseRelocView;

typedef struct LNK_CObjDebugSView
{
  U32 count;
  LNK_CObjDebugSEntry *v;
  LNK_CObjDebugSSummary *summaries;
} LNK_CObjDebugSView;

typedef struct LNK_CObjDecodeWindow LNK_CObjDecodeWindow;
struct LNK_CObjDecodeWindow
{
  LNK_CompressedObj *obj;
  U8  *buffer;
  U64  buffer_cap;
  U32  seg_idx;
  U32  valid_size;
};

internal void lnk_compressed_obj_configure(struct LNK_Config *config);
internal void lnk_compressed_obj_prepare_cache(String8 *mapped_files, U64 count);
internal void lnk_compressed_obj_trim_working_set(void);
internal String8 lnk_compressed_obj_direct_range(LNK_CompressedObj *obj, Rng1U64 range);
internal B32  lnk_compressed_obj_type_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjTypeIndexView *out);
internal B32  lnk_compressed_obj_base_reloc_index(LNK_CompressedObj *obj, LNK_CObjBaseRelocView *out);
internal B32  lnk_compressed_obj_debug_s_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjDebugSView *out);
internal B32  lnk_compressed_obj_copy_range(LNK_CompressedObj *obj, Rng1U64 range, void *dst, LNK_CObjDecodeWindow *window);
internal B32  lnk_compressed_obj_copy_string(LNK_CompressedObj *obj, String8 src, void *dst, LNK_CObjDecodeWindow *window);
internal void lnk_compressed_obj_release_window(LNK_CObjDecodeWindow *window);
internal U32  lnk_compressed_obj_segment_count(LNK_CompressedObj *obj);
internal U32  lnk_compressed_obj_segment_size(LNK_CompressedObj *obj);
internal Rng1U64 lnk_compressed_obj_stored_segment_range(LNK_CompressedObj *obj, U32 segment_idx);
internal B32  lnk_compressed_obj_open(struct LNK_Input *input, String8 mapped_file);
internal void lnk_compressed_obj_finalize_open(void);
internal void lnk_compressed_obj_close(struct LNK_Input *input);
internal void lnk_compressed_obj_log_stats(void);
internal void lnk_compressed_obj_log_phase_stats(char *tag);
