#if OS_WINDOWS && defined(LNK_OODLE)

#include "lnk_compressed_obj_format.h"
#include <oodle2.h>

////////////////////////////////
// Runtime state

#define LNK_COBJ_MAX_INITIAL_CACHE_GIB      17u
#define LNK_COBJ_MAX_POST_CACHE_GIB          6u
#define LNK_COBJ_MAX_TOTAL_CACHE_GIB        (LNK_COBJ_MAX_INITIAL_CACHE_GIB + LNK_COBJ_MAX_POST_CACHE_GIB)
#define LNK_COBJ_MIN_TOTAL_CACHE_GIB         4u
#define LNK_COBJ_REGION_CAP                 (1u << 20)
#define LNK_COBJ_DEFAULT_WRITE_GROUP_SLOTS  64u
#define LNK_COBJ_FILE_MAPPING_GRANULARITY   KB(64)
#define LNK_COBJ_STALL_ABORT_US            30000000ull
#define LNK_COBJ_KNOWN_HEADER_FLAGS (LNK_COBJ_FLAG_TYPE_INDEX               | \
                                     LNK_COBJ_FLAG_UDT_HASH_INDEX           | \
                                     LNK_COBJ_FLAG_BASE_RELOC_INDEX         | \
                                     LNK_COBJ_FLAG_PORTABLE_RAW_MAP         | \
                                     LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR      | \
                                     LNK_COBJ_FLAG_PACKED_TYPE_OFFSETS_V2   | \
                                     LNK_COBJ_FLAG_DEBUG_S_INDEX            | \
                                     LNK_COBJ_FLAG_DEBUG_S_SUMMARY          | \
                                     LNK_COBJ_TYPE_INDEX_COUNT_MASK)

// Every compressed input reserves a virtual range matching the original OBJ. Raw segments map
// directly from the portable file. Compressed segments remain placeholders until the vectored
// exception handler decodes them into a bounded pagefile-backed cache and maps the cache slot at
// the faulting logical address. The linker therefore keeps its ordinary pointer-based OBJ parser.

typedef enum LNK_CObjSegState
{
  LNK_CObjSegState_Empty,
  LNK_CObjSegState_Loading,
  LNK_CObjSegState_Ready,
  LNK_CObjSegState_Pinned,
  LNK_CObjSegState_Evicting,
} LNK_CObjSegState;

typedef struct LNK_CObjRuntimeSegment
{
  SRWLOCK lock;
  volatile LONG state;
  U32 slot_idx;
  U32 decode_count;
  B32 isolated_placeholder;
  U8 *address;
} LNK_CObjRuntimeSegment;

struct LNK_CompressedObj
{
  U8 *base;
  U64 reserve_size;
  U64 raw_size;
  U32 segment_size;
  U32 segment_count;
  String8 mapped_file;
  LNK_CObjHeader *header;
  LNK_CObjSegment *directory;
  LNK_CObjTypeIndex *type_indices;
  LNK_CObjUdtHashIndex *udt_hash_indices;
  LNK_CObjBaseRelocEntry *base_reloc_entries;
  U32 base_reloc_entry_count;
  B32 has_base_reloc_index;
  LNK_CObjDebugSEntry *debug_s_entries;
  LNK_CObjDebugSSummary *debug_s_summaries;
  U32 debug_s_entry_count;
  U32 type_index_count;
  LNK_CObjRuntimeSegment *segments;
  HANDLE portable_mapping;
  SRWLOCK placeholder_lock;
  volatile LONG active;
};

typedef struct LNK_CObjWriteGroup
{
  SRWLOCK lock;
  U8 *view;
  volatile LONG completed_count;
  U32 slot_count;
  volatile LONG sealed;
} LNK_CObjWriteGroup;

typedef struct LNK_CObjCache
{
  SRWLOCK lock;
  HANDLE slot_mapping;
  LNK_CObjWriteGroup *write_groups;
  U64 write_group_count;
  U32 write_group_slot_count;
  U64 slot_count;
  U64 active_slot_count;
  U64 segment_size;
  U64 resident_count;
  U64 victim_cursor;
  LNK_CObjRuntimeSegment **slot_segments;
  LNK_CompressedObj **regions;
  U64 region_count;
  U64 sorted_region_count;
  U64 region_cap;
  PVOID (WINAPI *map_view_of_file_3)(HANDLE,HANDLE,PVOID,ULONG64,SIZE_T,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);
  BOOL  (WINAPI *unmap_view_of_file_2)(HANDLE,PVOID,ULONG);
  PVOID (WINAPI *virtual_alloc_2)(HANDLE,PVOID,SIZE_T,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);
  PVOID veh;
  volatile LONG initialized;
  volatile LONG failed;
  U64 faults;
  U64 decodes;
  U64 redecodes;
  U64 evictions;
  U64 eviction_failures;
  U32 last_eviction_error;
  U64 raw_segments;
  U64 decoded_bytes;
  U64 stored_bytes_read;
  U64 occupancy_hwm;
  U64 frozen_segments;
  U64 raw_mapped_segments;
  U64 input_compressed_bytes;
  U64 input_raw_bytes;
  U64 initial_cache_bytes;
  U64 post_boundary_cache_bytes;
  U64 decode_us;
  U64 fault_us;
  U64 eviction_us;
  U64 freeze_us;
  U64 trim_us;
  U64 cleanup_us;
  B32 skip_cleanup;
} LNK_CObjCache;

typedef enum LNK_CObjPolicySource
{
  LNK_CObjPolicySource_Adaptive,
  LNK_CObjPolicySource_Environment,
  LNK_CObjPolicySource_CommandLine,
} LNK_CObjPolicySource;

typedef struct LNK_CObjPolicy
{
  U64 initial_cache_bytes;
  U64 post_boundary_cache_bytes;
  U32 trim_mode;
  B32 freeze_generation;
  B32 one_shot;
  B32 configured;
  LNK_CObjPolicySource initial_source;
  LNK_CObjPolicySource post_source;
  LNK_CObjPolicySource trim_source;
  LNK_CObjPolicySource freeze_source;
  LNK_CObjPolicySource one_shot_source;
} LNK_CObjPolicy;

typedef struct LNK_CObjDecoderScratch
{
  void *memory;
  OO_SINTa size;
  OodleLZ_Compressor compressor;
  B32 configured;
} LNK_CObjDecoderScratch;

global LNK_CObjCache g_lnk_cobj_cache;
global LNK_CObjPolicy g_lnk_cobj_policy;
global U64 g_lnk_cobj_input_capacity_hint;
global volatile LONG g_lnk_cobj_cache_init_state;

// Oodle otherwise creates and destroys decoder state through the process heap for every segment.
// Links decode tens of thousands of independent segments concurrently, so those tiny allocations
// serialize on the heap lock. One block per calling thread is sufficient because every decode is
// unthreaded and synchronous. The container records its codec, so each block is sized for the
// actual Kraken, Mermaid, or Selkie decoder instead of pessimistically reserving Oodle's maximum.
global thread_static LNK_CObjDecoderScratch g_lnk_cobj_decoder_scratch;
global U64 g_lnk_cobj_decoder_scratch_count;
global U64 g_lnk_cobj_decoder_scratch_bytes;

global U64 g_lnk_cobj_window_decodes;
global U64 g_lnk_cobj_window_bytes;
global U64 g_lnk_cobj_window_decoded_bytes;
global B32 g_lnk_cobj_cache_shrunk;
global U64 g_lnk_cobj_redecode_debug_s;
global U64 g_lnk_cobj_redecode_types;
global U64 g_lnk_cobj_redecode_segment_zero;

////////////////////////////////
// Configuration and diagnostics

internal LNK_CObjDecoderScratch
lnk_cobj_decoder_scratch(U32 compressor_value)
{
  OodleLZ_Compressor compressor = (OodleLZ_Compressor)compressor_value;
  if (g_lnk_cobj_decoder_scratch.memory != 0 &&
      g_lnk_cobj_decoder_scratch.configured &&
      g_lnk_cobj_decoder_scratch.compressor == compressor) {
    return g_lnk_cobj_decoder_scratch;
  }

  OO_S32 needed = OodleLZDecoder_MemorySizeNeeded(compressor, -1);
  if (needed <= 0) {
    compressor = OodleLZ_Compressor_Invalid;
    needed = OodleLZDecoder_MemorySizeNeeded(compressor, -1);
  }
  if (needed > 0 && needed > g_lnk_cobj_decoder_scratch.size) {
    void *memory = VirtualAlloc(0, (SIZE_T)needed, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (memory != 0) {
      void *old_memory = g_lnk_cobj_decoder_scratch.memory;
      OO_SINTa old_size = g_lnk_cobj_decoder_scratch.size;
      g_lnk_cobj_decoder_scratch.memory = memory;
      g_lnk_cobj_decoder_scratch.size = needed;
      if (old_memory == 0) {
        InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_decoder_scratch_count);
      } else {
        VirtualFree(old_memory, 0, MEM_RELEASE);
      }
      InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_decoder_scratch_bytes,
                               needed - old_size);
    }
  }

  if (needed > 0 &&
      g_lnk_cobj_decoder_scratch.memory != 0 &&
      g_lnk_cobj_decoder_scratch.size >= needed) {
    g_lnk_cobj_decoder_scratch.compressor = compressor;
    g_lnk_cobj_decoder_scratch.configured = 1;
    return g_lnk_cobj_decoder_scratch;
  }

  // Preserve Oodle's existing internal-allocation fallback if caller memory could not be made.
  LNK_CObjDecoderScratch fallback = {0};
  return fallback;
}

internal OO_SINTa
lnk_cobj_oodle_decompress(U32 compressor, const void *stored, OO_SINTa stored_size,
                          void *raw, OO_SINTa raw_size)
{
  LNK_CObjDecoderScratch scratch = lnk_cobj_decoder_scratch(compressor);
  return OodleLZ_Decompress(stored, stored_size, raw, raw_size,
                            OodleLZ_FuzzSafe_Yes, OodleLZ_CheckCRC_No, OodleLZ_Verbosity_None,
                            0, 0, 0, 0, scratch.memory, scratch.size,
                            OodleLZ_Decode_Unthreaded);
}

internal char *
lnk_cobj_policy_source_string(LNK_CObjPolicySource source)
{
  switch (source) {
  case LNK_CObjPolicySource_CommandLine: return "command-line";
  case LNK_CObjPolicySource_Environment: return "environment";
  default:                               return "adaptive";
  }
}

internal B32
lnk_cobj_env_u64(char *name, U64 *value_out)
{
  char value[64];
  DWORD len = GetEnvironmentVariableA(name, value, sizeof(value));
  if (len == 0 || len >= sizeof(value)) { return 0; }
  char *end = 0;
  U64 value_u64 = strtoull(value, &end, 10);
  if (end == value || *end != 0) { return 0; }
  *value_out = value_u64;
  return 1;
}

internal B32
lnk_cobj_env_switch(char *name, B32 *enabled_out)
{
  char value[16];
  DWORD len = GetEnvironmentVariableA(name, value, sizeof(value));
  if (len == 0 || len >= sizeof(value)) { return 0; }
  *enabled_out = value[0] != '0' && _stricmp(value, "no") != 0 && _stricmp(value, "false") != 0;
  return 1;
}

internal void
lnk_cobj_adaptive_capacities(U64 *initial_bytes_out, U64 *post_bytes_out)
{
  U64 total_gib = 8;
  MEMORYSTATUSEX memory = { sizeof(memory) };
  if (GlobalMemoryStatusEx(&memory)) {
    U64 physical_gib = memory.ullTotalPhys / GB(1);
    U64 commit_available_gib = memory.ullAvailPageFile / GB(1);
    U64 physical_budget_gib = (physical_gib * 3) / 8;
    U64 commit_budget_gib = commit_available_gib / 2;
    total_gib = Clamp(LNK_COBJ_MIN_TOTAL_CACHE_GIB,
                      Min(physical_budget_gib, commit_budget_gib),
                      LNK_COBJ_MAX_TOTAL_CACHE_GIB);
  }
  U64 post_gib = Max(1, (total_gib * LNK_COBJ_MAX_POST_CACHE_GIB) / LNK_COBJ_MAX_TOTAL_CACHE_GIB);
  *initial_bytes_out = (total_gib - post_gib) * GB(1);
  *post_bytes_out = post_gib * GB(1);
}

internal void
lnk_compressed_obj_configure(LNK_Config *config)
{
  LNK_CObjPolicy policy = {0};
  lnk_cobj_adaptive_capacities(&policy.initial_cache_bytes, &policy.post_boundary_cache_bytes);
  policy.freeze_generation = 1;
  policy.trim_mode = 1;

  U64 env_u64 = 0;
  if (lnk_cobj_env_u64("RAD_COBJ_CACHE_GIB", &env_u64) && env_u64 > 0) {
    policy.initial_cache_bytes = env_u64 * GB(1);
    policy.initial_source = LNK_CObjPolicySource_Environment;
  }
  if (lnk_cobj_env_u64("RAD_COBJ_CACHE_MIB", &env_u64) && env_u64 > 0) {
    policy.initial_cache_bytes = env_u64 * MB(1);
    policy.initial_source = LNK_CObjPolicySource_Environment;
  }
  if (lnk_cobj_env_u64("RAD_COBJ_CACHE_SHRINK_GIB", &env_u64) && env_u64 > 0) {
    policy.post_boundary_cache_bytes = env_u64 * GB(1);
    policy.post_source = LNK_CObjPolicySource_Environment;
  }

  B32 env_switch = 0;
  if (lnk_cobj_env_switch("RAD_COBJ_CACHE_FREEZE", &env_switch)) {
    policy.freeze_generation = env_switch;
    policy.freeze_source = LNK_CObjPolicySource_Environment;
  }
  if (lnk_cobj_env_u64("RAD_COBJ_TRIM_WS", &env_u64)) {
    policy.trim_mode = (U32)Clamp(0, env_u64, 2);
    policy.trim_source = LNK_CObjPolicySource_Environment;
  }
  if (lnk_cobj_env_switch("RAD_COBJ_SKIP_CLEANUP", &env_switch)) {
    policy.one_shot = env_switch;
    policy.one_shot_source = LNK_CObjPolicySource_Environment;
  }

  if (config != 0) {
    if (config->cobj_cache_gib > 0) {
      policy.initial_cache_bytes = config->cobj_cache_gib * GB(1);
      policy.initial_source = LNK_CObjPolicySource_CommandLine;
    }
    if (config->cobj_cache_shrink_gib > 0) {
      policy.post_boundary_cache_bytes = config->cobj_cache_shrink_gib * GB(1);
      policy.post_source = LNK_CObjPolicySource_CommandLine;
    }
    if (config->cobj_cache_freeze != LNK_SwitchState_Null) {
      policy.freeze_generation = config->cobj_cache_freeze == LNK_SwitchState_Yes;
      policy.freeze_source = LNK_CObjPolicySource_CommandLine;
    }
    if (config->cobj_trim_ws != LNK_SwitchState_Null) {
      policy.trim_mode = config->cobj_trim_ws == LNK_SwitchState_Yes;
      policy.trim_source = LNK_CObjPolicySource_CommandLine;
    }
    if (config->cobj_one_shot != LNK_SwitchState_Null) {
      policy.one_shot = config->cobj_one_shot == LNK_SwitchState_Yes;
      policy.one_shot_source = LNK_CObjPolicySource_CommandLine;
    }
  }

  policy.initial_cache_bytes = Max(MB(1), policy.initial_cache_bytes);
  policy.post_boundary_cache_bytes = Max(MB(1), policy.post_boundary_cache_bytes);
  policy.configured = 1;
  g_lnk_cobj_policy = policy;
}

internal void
lnk_compressed_obj_prepare_cache(String8 *mapped_files, U64 count)
{
  if (g_lnk_cobj_cache_init_state != 0) { return; }
  U64 segment_capacity = 0;
  for EachIndex(i, count) {
    String8 mapped_file = mapped_files[i];
    if (mapped_file.size < sizeof(LNK_CObjHeader)) { continue; }
    LNK_CObjHeader *header = (LNK_CObjHeader *)mapped_file.str;
    if (header->magic != LNK_COBJ_MAGIC || header->version != LNK_COBJ_VERSION) { continue; }
    // Every OBJ consumes an integral number of cache slots. Summing raw byte counts before
    // aligning underestimates capacity badly for targets containing many small OBJs: sixty-four
    // 20 KiB OBJs need sixty-four slots, not three slots for their ~1.25 MiB byte sum.
    U64 obj_capacity = (U64)header->segment_count * header->segment_size;
    segment_capacity = obj_capacity > max_U64 - segment_capacity ? max_U64 :
                       segment_capacity + obj_capacity;
  }
  if (segment_capacity > g_lnk_cobj_input_capacity_hint) {
    g_lnk_cobj_input_capacity_hint = segment_capacity;
  }
}

internal LNK_CompressedObj *lnk_cobj_region_from_address(U8 *address);

internal int
lnk_cobj_address_ptr_compare(const void *a, const void *b)
{
  U8 *pa = *(U8 **)a;
  U8 *pb = *(U8 **)b;
  return pa < pb ? -1 : pa > pb;
}

internal B32
lnk_cobj_runtime_segment_overlaps_types(LNK_CObjRuntimeSegment *segment)
{
  LNK_CompressedObj *region = segment && segment->address ?
                              lnk_cobj_region_from_address(segment->address) : 0;
  if (region == 0 || region->type_indices == 0) { return 0; }
  U64 seg_idx = (U64)(segment->address - region->base) / region->segment_size;
  Rng1U64 seg_range = r1u64(seg_idx * region->segment_size,
                            Min(region->raw_size, (seg_idx + 1) * region->segment_size));
  for EachIndex(type_idx, region->type_index_count) {
    LNK_CObjTypeIndex *type = &region->type_indices[type_idx];
    Rng1U64 type_range = r1u64(type->raw_section_offset,
                               type->raw_section_offset + type->raw_section_size);
    if (dim_1u64(intersect_1u64(seg_range, type_range)) != 0) { return 1; }
  }
  return 0;
}

internal B32
lnk_cobj_runtime_segment_overlaps_debug_s(LNK_CObjRuntimeSegment *segment)
{
  LNK_CompressedObj *region = segment && segment->address ?
                              lnk_cobj_region_from_address(segment->address) : 0;
  if (region == 0 || region->debug_s_entries == 0) { return 0; }
  U64 seg_idx = (U64)(segment->address - region->base) / region->segment_size;
  Rng1U64 seg_range = r1u64(seg_idx * region->segment_size,
                            Min(region->raw_size, (seg_idx + 1) * region->segment_size));
  for EachIndex(entry_idx, region->debug_s_entry_count) {
    LNK_CObjDebugSEntry *entry = &region->debug_s_entries[entry_idx];
    // Retain the complete span from the section signature through each indexed payload. Taking
    // the union over a section's entries covers its C13 headers/padding as well as payload bytes.
    Rng1U64 debug_s_range = r1u64(entry->raw_section_offset,
                                  (U64)entry->raw_payload_offset + entry->raw_payload_size);
    if (dim_1u64(intersect_1u64(seg_range, debug_s_range)) != 0) { return 1; }
  }
  return 0;
}

internal void
lnk_compressed_obj_log_phase_stats(char *tag)
{
  if (g_lnk_cobj_cache.region_count == 0) { return; }
  char *phase_stats_env = getenv("RAD_COBJ_PHASE_STATS");
  if (phase_stats_env != 0 && phase_stats_env[0] != 0 && phase_stats_env[0] != '0') {
    lnk_log(LNK_Log_Timers, "[cobj phase] %s faults=%llu decodes=%llu redecodes=%llu evictions=%llu window=%llu",
            tag, g_lnk_cobj_cache.faults, g_lnk_cobj_cache.decodes, g_lnk_cobj_cache.redecodes,
            g_lnk_cobj_cache.evictions, g_lnk_cobj_window_decodes);
  }
}

internal B32
lnk_cobj_skip_cleanup_enabled(void)
{
  return g_lnk_cobj_policy.one_shot;
}

////////////////////////////////
// Indexed and explicit-decode access

internal String8
lnk_compressed_obj_direct_range(LNK_CompressedObj *obj, Rng1U64 range)
{
  String8 result = {0};
  if (!obj || range.min >= range.max || range.max > obj->raw_size || !obj->mapped_file.str) { return result; }
  U32 first = (U32)(range.min / obj->segment_size);
  U32 last  = (U32)((range.max - 1) / obj->segment_size);
  if (last >= obj->segment_count) { return result; }

  LNK_CObjSegment *first_entry = &obj->directory[first];
  if (!(first_entry->flags & LNK_COBJ_SEGMENT_RAW) || first_entry->stored_size != first_entry->raw_size) { return result; }
  U64 expected_file_off = first_entry->file_offset + first_entry->stored_size;
  for (U32 seg_idx = first + 1; seg_idx <= last; ++seg_idx) {
    LNK_CObjSegment *entry = &obj->directory[seg_idx];
    if (!(entry->flags & LNK_COBJ_SEGMENT_RAW) || entry->stored_size != entry->raw_size ||
        entry->file_offset != expected_file_off) {
      return result;
    }
    expected_file_off += entry->stored_size;
  }

  // Raw runs are mapped directly into the reserved logical OBJ address range.
  return str8(obj->base + range.min, dim_1u64(range));
}

internal B32
lnk_compressed_obj_type_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjTypeIndexView *out)
{
  MemoryZeroStruct(out);
  if (!obj || !obj->type_indices) { return 0; }
  for (U32 i = 0; i < obj->type_index_count; ++i) {
    LNK_CObjTypeIndex *idx = &obj->type_indices[i];
    if (idx->raw_section_offset == section_range.min && idx->raw_section_size == dim_1u64(section_range)) {
      out->count   = idx->leaf_count;
      if (obj->header->flags & LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR) {
        U64 group_count = ((U64)idx->leaf_count +
                           (((U64)1 << LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT) - 1)) >>
                          LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT;
        U64 group_bytes = group_count * 2 * sizeof(U32);
        out->packed_v2_offset_groups = (U32 *)(obj->mapped_file.str + idx->offsets_file_offset);
        out->packed_v2_offset_payload = obj->mapped_file.str + idx->offsets_file_offset +
                                        ((group_bytes + 7) & ~(U64)7);
        out->offset_checkpoint_shift = LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT;
        out->packed_kind_dictionary = (U16 *)(obj->mapped_file.str + idx->sizes_file_offset);
        out->packed_kind_codes = obj->mapped_file.str + idx->kinds_file_offset;
        out->packed_sidecar = 1;
      } else {
        out->offsets = (U32 *)(obj->mapped_file.str + idx->offsets_file_offset);
        out->sizes = (U16 *)(obj->mapped_file.str + idx->sizes_file_offset);
        out->kinds = (U16 *)(obj->mapped_file.str + idx->kinds_file_offset);
      }
      if (obj->udt_hash_indices) {
        LNK_CObjUdtHashIndex *udt = &obj->udt_hash_indices[i];
        out->complete_udt_hashes = (U64 *)(obj->mapped_file.str + udt->hashes_file_offset);
        out->complete_udt_hash_count = udt->hash_count;
      }
      return 1;
    }
  }
  return 0;
}

internal B32
lnk_compressed_obj_base_reloc_index(LNK_CompressedObj *obj, LNK_CObjBaseRelocView *out)
{
  MemoryZeroStruct(out);
  if (!obj || !obj->has_base_reloc_index) { return 0; }
  out->count = obj->base_reloc_entry_count;
  out->v = obj->base_reloc_entries;
  return 1;
}

internal B32
lnk_compressed_obj_debug_s_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjDebugSView *out)
{
  MemoryZeroStruct(out);
  if (!obj || !obj->debug_s_entries || section_range.min > max_U32) { return 0; }
  U32 wanted = (U32)section_range.min;
  U32 lo = 0, hi = obj->debug_s_entry_count;
  while (lo < hi) {
    U32 mid = lo + (hi - lo) / 2;
    if (obj->debug_s_entries[mid].raw_section_offset < wanted) { lo = mid + 1; }
    else { hi = mid; }
  }
  U32 first = lo;
  while (lo < obj->debug_s_entry_count && obj->debug_s_entries[lo].raw_section_offset == wanted) { lo += 1; }
  if (first == lo) { return 0; }
  out->v = obj->debug_s_entries + first;
  out->summaries = obj->debug_s_summaries ? obj->debug_s_summaries + first : 0;
  out->count = lo - first;
  return 1;
}

internal B32
lnk_compressed_obj_copy_range(LNK_CompressedObj *obj, Rng1U64 range, void *raw_dst, LNK_CObjDecodeWindow *window)
{
  if (!obj || range.min > range.max || range.max > obj->raw_size) { return 0; }
  if (!window->buffer || window->buffer_cap < obj->segment_size) {
    if (window->buffer) { VirtualFree(window->buffer, 0, MEM_RELEASE); }
    window->buffer = VirtualAlloc(0, obj->segment_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    window->buffer_cap = window->buffer ? obj->segment_size : 0;
    window->seg_idx = max_U32;
    if (!window->buffer) { return 0; }
  }
  if (window->obj != obj) { window->obj = obj; window->seg_idx = max_U32; }

  U8 *dst = raw_dst;
  U64 cursor = range.min;
  while (cursor < range.max) {
    U32 seg_idx = (U32)(cursor / obj->segment_size);
    LNK_CObjSegment *entry = &obj->directory[seg_idx];
    U64 in_seg = cursor - (U64)seg_idx * obj->segment_size;
    U64 amount = Min(range.max - cursor, (U64)entry->raw_size - in_seg);
    if (window->seg_idx != seg_idx) {
      U8 *stored = obj->mapped_file.str + entry->file_offset;
      B32 ok = 1;
      B32 decode_direct = in_seg == 0 && amount == entry->raw_size;
      U8 *decode_dst = decode_direct ? dst : window->buffer;
      if (entry->flags & LNK_COBJ_SEGMENT_RAW) {
        MemoryCopy(decode_dst, stored, entry->raw_size);
      } else {
        OO_SINTa got = lnk_cobj_oodle_decompress(obj->header->compressor,
                                                 stored, entry->stored_size,
                                                 decode_dst, entry->raw_size);
        ok = got == entry->raw_size;
      }
      if (!ok) { return 0; }
      InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_window_decodes);
      InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_window_bytes, entry->stored_size);
      InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_window_decoded_bytes, entry->raw_size);
      if (decode_direct) {
        // Large $S and symbol-fixup walks consume complete interior segments once. Decode those
        // bytes into their final arena instead of filling the window and copying them again.
        window->seg_idx = max_U32;
        dst += amount; cursor += amount;
        continue;
      }
      window->seg_idx = seg_idx;
      window->valid_size = entry->raw_size;
    }
    MemoryCopy(dst, window->buffer + in_seg, amount);
    dst += amount; cursor += amount;
  }
  return 1;
}

internal B32
lnk_compressed_obj_copy_string(LNK_CompressedObj *obj, String8 src, void *dst, LNK_CObjDecodeWindow *window)
{
  if (!obj) { return 0; }
  U64 base = (U64)obj->base;
  U64 ptr  = (U64)src.str;
  if (ptr < base || ptr - base > obj->raw_size || src.size > obj->raw_size - (ptr - base)) { return 0; }
  return lnk_compressed_obj_copy_range(obj, r1u64(ptr - base, ptr - base + src.size), dst, window);
}

internal void
lnk_compressed_obj_release_window(LNK_CObjDecodeWindow *window)
{
  if (window->buffer) { VirtualFree(window->buffer, 0, MEM_RELEASE); }
  MemoryZeroStruct(window);
}

internal U32
lnk_compressed_obj_segment_count(LNK_CompressedObj *obj)
{
  return obj ? obj->segment_count : 0;
}

internal U32
lnk_compressed_obj_segment_size(LNK_CompressedObj *obj)
{
  return obj ? obj->segment_size : 0;
}

internal Rng1U64
lnk_compressed_obj_stored_segment_range(LNK_CompressedObj *obj, U32 segment_idx)
{
  Rng1U64 result = {0};
  if (obj && segment_idx < obj->segment_count) {
    LNK_CObjSegment *segment = &obj->directory[segment_idx];
    U64 min = (U64)(obj->mapped_file.str + segment->file_offset);
    result = r1u64(min, min + segment->stored_size);
  }
  return result;
}

////////////////////////////////
// Decoded-segment cache


internal LNK_CObjWriteGroup *
lnk_cobj_alloc_write_groups(U64 slot_count, U32 group_slot_count, U64 *group_count_out)
{
  U64 group_count = CeilIntegerDiv(slot_count, group_slot_count);
  LNK_CObjWriteGroup *groups = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                          group_count * sizeof(*groups));
  if (groups) {
    for EachIndex(group_idx, group_count) {
      U64 first_slot = group_idx * group_slot_count;
      groups[group_idx].slot_count = (U32)Min((U64)group_slot_count, slot_count - first_slot);
    }
    *group_count_out = group_count;
  }
  return groups;
}

internal void
lnk_cobj_release_write_groups(LNK_CObjWriteGroup *groups, U64 group_count)
{
  if (!groups) { return; }
  for EachIndex(group_idx, group_count) {
    if (groups[group_idx].view) { UnmapViewOfFile(groups[group_idx].view); }
  }
  HeapFree(GetProcessHeap(), 0, groups);
}

internal U8 *
lnk_cobj_write_group_acquire(U32 slot_idx, U32 *group_idx_out)
{
  if (!g_lnk_cobj_cache.write_groups) { return 0; }
  U32 group_idx = slot_idx / g_lnk_cobj_cache.write_group_slot_count;
  LNK_CObjWriteGroup *group = &g_lnk_cobj_cache.write_groups[group_idx];
  AcquireSRWLockExclusive(&group->lock);
  U8 *result = 0;
  if (!group->sealed) {
    // Return the group index even when the large write view cannot be mapped. The caller can
    // populate this slot through a temporary per-slot view, but must still count it complete;
    // otherwise the group never seals and none of its slots can ever be recycled.
    *group_idx_out = group_idx;
    if (!group->view) {
      U64 group_off = (U64)group_idx * g_lnk_cobj_cache.write_group_slot_count *
                      g_lnk_cobj_cache.segment_size;
      U64 group_size = (U64)group->slot_count * g_lnk_cobj_cache.segment_size;
      group->view = MapViewOfFile(g_lnk_cobj_cache.slot_mapping, FILE_MAP_WRITE,
                                  (DWORD)(group_off >> 32), (DWORD)group_off, group_size);
    }
    if (group->view) {
      U32 in_group_idx = slot_idx % g_lnk_cobj_cache.write_group_slot_count;
      result = group->view + (U64)in_group_idx * g_lnk_cobj_cache.segment_size;
    }
  }
  ReleaseSRWLockExclusive(&group->lock);
  return result;
}

internal void
lnk_cobj_write_group_complete(U32 group_idx)
{
  LNK_CObjWriteGroup *group = &g_lnk_cobj_cache.write_groups[group_idx];
  AcquireSRWLockExclusive(&group->lock);
  LONG completed_count = InterlockedIncrement(&group->completed_count);
  if (completed_count == group->slot_count) {
    if (group->view) { UnmapViewOfFile(group->view); }
    group->view = 0;
    InterlockedExchange(&group->sealed, 1);
  }
  ReleaseSRWLockExclusive(&group->lock);
}

internal void
lnk_compressed_obj_trim_working_set(void)
{
  if (g_lnk_cobj_policy.trim_mode == 0) { return; }

  U64 begin_us = now_time_us();
  if (g_lnk_cobj_policy.trim_mode == 2) {
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.trim_us, now_time_us() - begin_us);
    lnk_log(LNK_Log_Timers, "[cobj trim] whole-process elapsed=%.2f ms",
            (F64)(now_time_us() - begin_us) / 1000.0);
    return;
  }

  // Drop only the decoded cache views from the process working set.  Unlike the
  // whole-process SetProcessWorkingSetSize path this preserves the linker's hot
  // hash tables and arenas.  The pagefile-section mappings stay intact, so a
  // later pointer access can soft-fault the decoded bytes without running Oodle
  // again; VirtualUnlock on an unlocked range intentionally returns
  // ERROR_NOT_LOCKED after removing resident pages from the caller's working set.
  U64 cache_view_count = 0;
  U64 cache_view_bytes = 0;
  U64 purged_count = 0;
  U64 post_boundary_cache_bytes = g_lnk_cobj_cache.post_boundary_cache_bytes;
  if (post_boundary_cache_bytes > 0 && !g_lnk_cobj_cache_shrunk && g_lnk_cobj_cache.segment_size) {
    U64 wanted_slots = Clamp(1, post_boundary_cache_bytes / g_lnk_cobj_cache.segment_size,
                             g_lnk_cobj_cache.slot_count);
    B32 freeze_generation = g_lnk_cobj_policy.freeze_generation;
    if (freeze_generation) {
      U64 freeze_begin_us = now_time_us();
      // Preserve the complete first-generation mapping but never recycle one of its slots again.
      // Existing logical OBJ pointers therefore remain valid. PDB misses use a fresh, independently
      // evicting cache generation; this costs shrink_gib additional system commit but avoids the
      // thousands of UnmapViewOfFile2 calls and forced re-decodes of a destructive phase reset.
      U64 new_mapping_size = wanted_slots * g_lnk_cobj_cache.segment_size;
      HANDLE new_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                               (DWORD)(new_mapping_size >> 32),
                                               (DWORD)new_mapping_size, 0);
      U64 new_write_group_count = 0;
      LNK_CObjWriteGroup *new_write_groups = g_lnk_cobj_cache.write_groups ?
        lnk_cobj_alloc_write_groups(wanted_slots, g_lnk_cobj_cache.write_group_slot_count,
                                    &new_write_group_count) : 0;
      LNK_CObjRuntimeSegment **new_slots = VirtualAlloc(0, wanted_slots * sizeof(void *),
                                                        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      B32 can_freeze = new_mapping != 0 && new_slots != 0 &&
                       (!g_lnk_cobj_cache.write_groups || new_write_groups != 0);
      AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
      if (can_freeze) {
        for EachIndex(slot_idx, g_lnk_cobj_cache.slot_count) {
          LNK_CObjRuntimeSegment *segment = g_lnk_cobj_cache.slot_segments[slot_idx];
          if (segment != 0 && segment->state != LNK_CObjSegState_Ready &&
              segment->state != LNK_CObjSegState_Pinned) {
            can_freeze = 0;
            break;
          }
        }
      }
      if (can_freeze) {
        HANDLE old_mapping = g_lnk_cobj_cache.slot_mapping;
        LNK_CObjWriteGroup *old_write_groups = g_lnk_cobj_cache.write_groups;
        U64 old_write_group_count = g_lnk_cobj_cache.write_group_count;
        LNK_CObjRuntimeSegment **old_slots = g_lnk_cobj_cache.slot_segments;
        U64 old_slot_count = g_lnk_cobj_cache.slot_count;
        for EachIndex(slot_idx, old_slot_count) {
          LNK_CObjRuntimeSegment *segment = old_slots[slot_idx];
          if (segment != 0) {
            // max_U32 denotes a frozen slot: it is mapped and readable, but is not owned by the
            // active generation's slot table and must never participate in victim selection.
            segment->slot_idx = max_U32;
            g_lnk_cobj_cache.frozen_segments += 1;
          }
        }
        g_lnk_cobj_cache.slot_mapping = new_mapping;
        g_lnk_cobj_cache.write_groups = new_write_groups;
        g_lnk_cobj_cache.write_group_count = new_write_group_count;
        g_lnk_cobj_cache.slot_segments = new_slots;
        g_lnk_cobj_cache.slot_count = wanted_slots;
        g_lnk_cobj_cache.active_slot_count = wanted_slots;
        g_lnk_cobj_cache.resident_count = 0;
        g_lnk_cobj_cache.victim_cursor = 0;
        g_lnk_cobj_cache_shrunk = 1;
        ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
        // Mapped views retain the old pagefile section object. Only its handle and now-unused
        // reverse slot table are released here; ordinary close unmaps each frozen logical view.
        lnk_cobj_release_write_groups(old_write_groups, old_write_group_count);
        CloseHandle(old_mapping);
        U64 trimmed_frozen = 0;
        U64 trim_calls = 0;
        U64 frozen_trim_us = 0;
        char *freeze_trim_env = getenv("RAD_COBJ_CACHE_FREEZE_TRIM");
        if (freeze_trim_env != 0 && freeze_trim_env[0] != 0 && freeze_trim_env[0] != '0') {
          U64 frozen_trim_begin_us = now_time_us();
          // Keep every view valid but offer its resident pages back to the OS. Later consumers
          // incur only soft page faults into the frozen pagefile section, never Oodle re-decodes.
          // This is deliberately optional: it trades some kernel fault work for a lower PDB-phase
          // working set, so deployments can choose the appropriate memory/speed point.
          B32 types_only = _stricmp(freeze_trim_env, "types") == 0;
          B32 except_debug_s = _stricmp(freeze_trim_env, "except_debug_s") == 0 ||
                               _stricmp(freeze_trim_env, "except_debug_s_runs") == 0;
          B32 runs_only = _stricmp(freeze_trim_env, "except_debug_s_runs") == 0;
          U8 **trim_addresses = VirtualAlloc(0, old_slot_count * sizeof(*trim_addresses),
                                             MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
          for EachIndex(slot_idx, old_slot_count) {
            LNK_CObjRuntimeSegment *segment = old_slots[slot_idx];
            B32 should_trim = segment != 0 && segment->address != 0;
            if (should_trim && types_only) {
              should_trim = lnk_cobj_runtime_segment_overlaps_types(segment);
            } else if (should_trim && except_debug_s) {
              should_trim = !lnk_cobj_runtime_segment_overlaps_debug_s(segment);
            }
            if (should_trim) {
              if (trim_addresses != 0) {
                trim_addresses[trimmed_frozen++] = segment->address;
              } else {
                VirtualUnlock(segment->address, g_lnk_cobj_cache.segment_size);
                trimmed_frozen += 1;
                trim_calls += 1;
              }
            }
          }
          if (trim_addresses != 0) {
            qsort(trim_addresses, trimmed_frozen, sizeof(*trim_addresses), lnk_cobj_address_ptr_compare);
            U64 selected_count = trimmed_frozen;
            trimmed_frozen = 0;
            for (U64 first = 0; first < selected_count;) {
              U64 opl = first + 1;
              while (opl < selected_count &&
                     trim_addresses[opl] == trim_addresses[opl - 1] + g_lnk_cobj_cache.segment_size) {
                opl += 1;
              }
              U64 run_count = opl - first;
              if (!runs_only || run_count > 1) {
                VirtualUnlock(trim_addresses[first], run_count * g_lnk_cobj_cache.segment_size);
                trimmed_frozen += run_count;
                trim_calls += 1;
              }
              first = opl;
            }
            VirtualFree(trim_addresses, 0, MEM_RELEASE);
          }
          frozen_trim_us = now_time_us() - frozen_trim_begin_us;
          InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.trim_us, frozen_trim_us);
        }
        VirtualFree(old_slots, 0, MEM_RELEASE);
        U64 freeze_us = now_time_us() - freeze_begin_us;
        InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.freeze_us, freeze_us);
        lnk_log(LNK_Log_Timers, "[cobj freeze] retained=%llu trimmed=%llu trim-calls=%llu new-cache=%llu MiB elapsed=%.2f ms",
                g_lnk_cobj_cache.frozen_segments, trimmed_frozen, trim_calls, new_mapping_size / MB(1),
                (F64)freeze_us / 1000.0);
        return;
      }
      ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
      lnk_cobj_release_write_groups(new_write_groups, new_write_group_count);
      if (new_mapping) { CloseHandle(new_mapping); }
      if (new_slots) { VirtualFree(new_slots, 0, MEM_RELEASE); }
    }
    AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
    // Reset the phase cache completely. Keeping arbitrary retained mappings across the
    // limit transition is unsafe because later victim selection owns an index prefix.
    for EachIndex(slot_idx, g_lnk_cobj_cache.slot_count) {
      LNK_CObjRuntimeSegment *segment = g_lnk_cobj_cache.slot_segments[slot_idx];
      if (!segment || InterlockedCompareExchange(&segment->state, LNK_CObjSegState_Evicting,
                                                  LNK_CObjSegState_Ready) != LNK_CObjSegState_Ready) {
        continue;
      }
      if (segment->address &&
          g_lnk_cobj_cache.unmap_view_of_file_2(GetCurrentProcess(), segment->address,
                                                MEM_PRESERVE_PLACEHOLDER)) {
        g_lnk_cobj_cache.slot_segments[slot_idx] = 0;
        segment->slot_idx = max_U32;
        InterlockedExchange(&segment->state, LNK_CObjSegState_Empty);
        purged_count += 1;
      } else {
        InterlockedExchange(&segment->state, LNK_CObjSegState_Ready);
      }
    }
    // Write-pinned segments (and the unlikely view that failed to unmap) still own their
    // original slots.  Count survivors in the active prefix instead of pretending the reset
    // emptied it.  If that prefix is completely occupied, extend it through the next free slot
    // so a later cache miss cannot spin forever with no possible victim.
    U64 active_slot_count = wanted_slots;
    U64 retained_active_count = 0;
    for EachIndex(slot_idx, active_slot_count) {
      retained_active_count += g_lnk_cobj_cache.slot_segments[slot_idx] != 0;
    }
    if (retained_active_count == active_slot_count) {
      for (U64 slot_idx = active_slot_count; slot_idx < g_lnk_cobj_cache.slot_count; ++slot_idx) {
        retained_active_count += g_lnk_cobj_cache.slot_segments[slot_idx] != 0;
        active_slot_count = slot_idx + 1;
        if (g_lnk_cobj_cache.slot_segments[slot_idx] == 0) { break; }
      }
    }
    g_lnk_cobj_cache.active_slot_count = active_slot_count;
    g_lnk_cobj_cache.resident_count = retained_active_count;
    g_lnk_cobj_cache.victim_cursor %= active_slot_count;
    g_lnk_cobj_cache_shrunk = 1;
    ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
  }
  if (post_boundary_cache_bytes == 0) {
    for EachIndex(region_idx, g_lnk_cobj_cache.region_count) {
      LNK_CompressedObj *region = g_lnk_cobj_cache.regions[region_idx];
      if (!region || !region->active) { continue; }
      for EachIndex(seg_idx, region->segment_count) {
        LNK_CObjRuntimeSegment *segment = &region->segments[seg_idx];
        if ((segment->state == LNK_CObjSegState_Ready || segment->state == LNK_CObjSegState_Pinned) &&
            segment->address) {
          VirtualUnlock(segment->address, region->segment_size);
          cache_view_count += 1;
          cache_view_bytes += region->segment_size;
        }
      }
    }
  }
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.trim_us, now_time_us() - begin_us);
  lnk_log(LNK_Log_Timers, "[cobj trim] purged=%llu active=%llu MiB cache_views=%llu cache=%llu GiB elapsed=%.2f ms",
          purged_count, (g_lnk_cobj_cache.active_slot_count * g_lnk_cobj_cache.segment_size) / MB(1),
          cache_view_count, cache_view_bytes / GB(1),
          (F64)(now_time_us() - begin_us) / 1000.0);
}

internal int
lnk_cobj_region_ptr_compare(const void *a, const void *b)
{
  LNK_CompressedObj *ra = *(LNK_CompressedObj **)a;
  LNK_CompressedObj *rb = *(LNK_CompressedObj **)b;
  return ra->base < rb->base ? -1 : ra->base > rb->base;
}

internal LNK_CompressedObj *
lnk_cobj_region_from_address(U8 *address)
{
  U64 min = 0, max = g_lnk_cobj_cache.region_count;
  while (min < max) {
    U64 mid = min + (max - min) / 2;
    LNK_CompressedObj *region = g_lnk_cobj_cache.regions[mid];
    if (address < region->base) {
      max = mid;
    } else if (address >= region->base + region->reserve_size) {
      min = mid + 1;
    } else {
      return region->active ? region : 0;
    }
  }
  return 0;
}

internal B32
lnk_cobj_evict_one_locked(U32 *slot_idx_out)
{
  U64 victim_slot = max_U64;
  LNK_CObjRuntimeSegment *victim = 0;
  for (U64 probe = 0; probe < g_lnk_cobj_cache.active_slot_count; ++probe) {
    U64 slot_idx = (g_lnk_cobj_cache.victim_cursor + probe) % g_lnk_cobj_cache.active_slot_count;
    LNK_CObjRuntimeSegment *segment = g_lnk_cobj_cache.slot_segments[slot_idx];
    if (segment == 0) {
      *slot_idx_out = (U32)slot_idx;
      g_lnk_cobj_cache.victim_cursor = (slot_idx + 1) % g_lnk_cobj_cache.active_slot_count;
      return 1;
    }
    B32 write_group_recyclable = 1;
    if (g_lnk_cobj_cache.write_groups) {
      U64 group_idx = slot_idx / g_lnk_cobj_cache.write_group_slot_count;
      write_group_recyclable = g_lnk_cobj_cache.write_groups[group_idx].sealed != 0;
    }
    if (segment->state == LNK_CObjSegState_Ready && write_group_recyclable) {
      victim_slot = slot_idx;
      victim = segment;
      break;
    }
  }
  if (victim == 0 || InterlockedCompareExchange(&victim->state, LNK_CObjSegState_Evicting, LNK_CObjSegState_Ready) != LNK_CObjSegState_Ready) {
    return 0;
  }

  U8 *victim_address = victim->address;
  U64 eviction_begin_us = now_time_us();
  B32 unmapped = victim_address != 0 &&
                 g_lnk_cobj_cache.unmap_view_of_file_2(GetCurrentProcess(), victim_address,
                                                       MEM_PRESERVE_PLACEHOLDER);
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.eviction_us,
                           now_time_us() - eviction_begin_us);
  if (!unmapped) {
    g_lnk_cobj_cache.eviction_failures += 1;
    g_lnk_cobj_cache.last_eviction_error = GetLastError();
    InterlockedExchange(&victim->state, LNK_CObjSegState_Ready);
    return 0;
  }
  g_lnk_cobj_cache.slot_segments[victim_slot] = 0;
  Assert(g_lnk_cobj_cache.resident_count > 0);
  g_lnk_cobj_cache.resident_count -= 1;
  victim->slot_idx = max_U32;
  InterlockedExchange(&victim->state, LNK_CObjSegState_Empty);
  g_lnk_cobj_cache.evictions += 1;
  *slot_idx_out = (U32)victim_slot;
  g_lnk_cobj_cache.victim_cursor = (victim_slot + 1) % g_lnk_cobj_cache.active_slot_count;
  return 1;
}

internal B32 lnk_cobj_isolate_portable_segment(LNK_CompressedObj *region, U32 seg_idx);

internal B32
lnk_cobj_materialize(LNK_CompressedObj *region, U32 seg_idx)
{
  LNK_CObjRuntimeSegment *runtime = &region->segments[seg_idx];
  AcquireSRWLockExclusive(&runtime->lock);
  if (runtime->state == LNK_CObjSegState_Ready || runtime->state == LNK_CObjSegState_Pinned) {
    ReleaseSRWLockExclusive(&runtime->lock);
    return 1;
  }
  while (runtime->state == LNK_CObjSegState_Evicting) { YieldProcessor(); }
  InterlockedExchange(&runtime->state, LNK_CObjSegState_Loading);

  U32 slot_idx = max_U32;
  U64 stall_begin_us = now_time_us();
  AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
  while (!lnk_cobj_evict_one_locked(&slot_idx)) {
    ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
    SwitchToThread();
    AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
    U64 elapsed_us = now_time_us() - stall_begin_us;
    if (elapsed_us >= LNK_COBJ_STALL_ABORT_US) {
      ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
      InterlockedExchange(&runtime->state, LNK_CObjSegState_Empty);
      InterlockedExchange(&g_lnk_cobj_cache.failed, 1);
      ReleaseSRWLockExclusive(&runtime->lock);
      return 0;
    }
  }
  g_lnk_cobj_cache.slot_segments[slot_idx] = runtime;
  runtime->slot_idx = slot_idx;
  g_lnk_cobj_cache.resident_count += 1;
  Assert(g_lnk_cobj_cache.resident_count <= g_lnk_cobj_cache.active_slot_count);
  if (g_lnk_cobj_cache.resident_count > g_lnk_cobj_cache.occupancy_hwm) {
    g_lnk_cobj_cache.occupancy_hwm = g_lnk_cobj_cache.resident_count;
  }
  ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);

  LNK_CObjSegment *entry = &region->directory[seg_idx];
  U64 slot_off = (U64)slot_idx * region->segment_size;
  U32 write_group_idx = max_U32;
  U8 *write_view = lnk_cobj_write_group_acquire(slot_idx, &write_group_idx);
  B32 temporary_write_view = write_view == 0;
  if (temporary_write_view) {
    write_view = (U8 *)MapViewOfFile(g_lnk_cobj_cache.slot_mapping, FILE_MAP_WRITE,
                                     (DWORD)(slot_off >> 32), (DWORD)slot_off, region->segment_size);
  }
  B32 ok = write_view != 0;
  B32 slot_written = 0;
  U64 decode_begin_us = now_time_us();
  if (ok) {
    U8 *stored = region->mapped_file.str + entry->file_offset;
    if (entry->flags & LNK_COBJ_SEGMENT_RAW) {
      MemoryCopy(write_view, stored, entry->raw_size);
      InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_cache.raw_segments);
    } else {
      OO_SINTa decoded = lnk_cobj_oodle_decompress(region->header->compressor,
                                                   stored, entry->stored_size,
                                                   write_view, entry->raw_size);
      ok = decoded == entry->raw_size;
    }
    if (ok && entry->raw_size < region->segment_size) {
      MemoryZero(write_view + entry->raw_size, region->segment_size - entry->raw_size);
    }
    slot_written = ok;
    if (temporary_write_view) { UnmapViewOfFile(write_view); }
  }
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.decode_us,
                           now_time_us() - decode_begin_us);
  U8 *target = region->base + (U64)seg_idx * region->segment_size;
  if (ok) { ok = lnk_cobj_isolate_portable_segment(region, seg_idx); }
  if (ok) {
    void *mapped = g_lnk_cobj_cache.map_view_of_file_3(g_lnk_cobj_cache.slot_mapping, GetCurrentProcess(), target,
                                                       slot_off, region->segment_size, MEM_REPLACE_PLACEHOLDER,
                                                       PAGE_READONLY, 0, 0);
    ok = mapped == target;
  }
  if (slot_written && write_group_idx != max_U32) {
    lnk_cobj_write_group_complete(write_group_idx);
  }
  if (!ok) {
    AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
    if (g_lnk_cobj_cache.slot_segments[slot_idx] == runtime) {
      g_lnk_cobj_cache.slot_segments[slot_idx] = 0;
      Assert(g_lnk_cobj_cache.resident_count > 0);
      g_lnk_cobj_cache.resident_count -= 1;
    }
    ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
    runtime->slot_idx = max_U32;
    InterlockedExchange(&runtime->state, LNK_CObjSegState_Empty);
    InterlockedExchange(&g_lnk_cobj_cache.failed, 1);
    ReleaseSRWLockExclusive(&runtime->lock);
    return 0;
  }

  if (runtime->decode_count++ > 0) {
    InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_cache.redecodes);
    if (lnk_cobj_runtime_segment_overlaps_debug_s(runtime)) {
      InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_redecode_debug_s);
    }
    if (lnk_cobj_runtime_segment_overlaps_types(runtime)) {
      InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_redecode_types);
    }
    if (seg_idx == 0) { InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_redecode_segment_zero); }
  }
  InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_cache.decodes);
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.decoded_bytes, entry->raw_size);
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.stored_bytes_read, entry->stored_size);
  InterlockedExchange(&runtime->state, LNK_CObjSegState_Ready);
  ReleaseSRWLockExclusive(&runtime->lock);
  return 1;
}

// In-place COFF patches are rare, but they must retain FILE_MAP_COPY semantics. Raw-stored
// segments already map the portable file directly and can fall through to the generic COW VEH.
// Compressed segments first need a decoded mapping. Pin it so eviction can never discard a
// private modification, then promote the faulting page here instead of depending on the order of
// independently registered vectored exception handlers.
internal B32
lnk_cobj_pin_compressed_segment_for_write(LNK_CompressedObj *region, U32 seg_idx)
{
  LNK_CObjRuntimeSegment *runtime = &region->segments[seg_idx];
  for (;;) {
    LONG state = runtime->state;
    if (state == LNK_CObjSegState_Pinned) { return 1; }
    if (state == LNK_CObjSegState_Ready) {
      if (InterlockedCompareExchange(&runtime->state, LNK_CObjSegState_Pinned,
                                     LNK_CObjSegState_Ready) == LNK_CObjSegState_Ready) {
        return 1;
      }
      continue;
    }
    if (!lnk_cobj_materialize(region, seg_idx)) { return 0; }
  }
}

internal LONG NTAPI
lnk_cobj_veh(EXCEPTION_POINTERS *info)
{
  EXCEPTION_RECORD *er = info->ExceptionRecord;
  if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
    U8 *address = (U8 *)er->ExceptionInformation[1];
    LNK_CompressedObj *region = lnk_cobj_region_from_address(address);
    if (region != 0) {
      U64 fault_begin_us = now_time_us();
      InterlockedIncrement64((volatile LONG64 *)&g_lnk_cobj_cache.faults);
      U32 seg_idx = (U32)((address - region->base) / region->segment_size);
      U64 access_kind = er->ExceptionInformation[0];
      if (access_kind == 0 && seg_idx < region->segment_count &&
          lnk_cobj_materialize(region, seg_idx)) {
        InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.fault_us,
                                 now_time_us() - fault_begin_us);
        return EXCEPTION_CONTINUE_EXECUTION;
      }
      if (access_kind == 1 && seg_idx < region->segment_count &&
          !(region->directory[seg_idx].flags & LNK_COBJ_SEGMENT_RAW)) {
        if (lnk_cobj_pin_compressed_segment_for_write(region, seg_idx)) {
          // Complete the write-fault transition in this handler. Both this handler and the
          // ordinary input COW handler are front-inserted, so relying on which one runs next
          // would make a write-first access depend on their registration order.
          void *page = (void *)((UINT_PTR)address & ~(UINT_PTR)(KB(4) - 1));
          DWORD old_protect = 0;
          if (VirtualProtect(page, KB(4), PAGE_WRITECOPY, &old_protect)) {
            InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.fault_us,
                                     now_time_us() - fault_begin_us);
            return EXCEPTION_CONTINUE_EXECUTION;
          }
        }
      }
      InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.fault_us,
                               now_time_us() - fault_begin_us);
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

internal B32
lnk_cobj_cache_init(U32 segment_size)
{
  LONG init_state = InterlockedCompareExchange(&g_lnk_cobj_cache_init_state, 1, 0);
  if (init_state != 0) {
    while (g_lnk_cobj_cache_init_state == 1) { SwitchToThread(); }
    return g_lnk_cobj_cache_init_state == 2 && g_lnk_cobj_cache.failed == 0 &&
           g_lnk_cobj_cache.segment_size == segment_size;
  }
  InitializeSRWLock(&g_lnk_cobj_cache.lock);
  HMODULE kernelbase = GetModuleHandleA("KernelBase.dll");
  g_lnk_cobj_cache.map_view_of_file_3 = (void *)GetProcAddress(kernelbase, "MapViewOfFile3");
  g_lnk_cobj_cache.unmap_view_of_file_2 = (void *)GetProcAddress(kernelbase, "UnmapViewOfFile2");
  g_lnk_cobj_cache.virtual_alloc_2 = (void *)GetProcAddress(kernelbase, "VirtualAlloc2");
  if (!g_lnk_cobj_cache.map_view_of_file_3 || !g_lnk_cobj_cache.unmap_view_of_file_2 || !g_lnk_cobj_cache.virtual_alloc_2) {
    g_lnk_cobj_cache.failed = 1;
    g_lnk_cobj_cache.initialized = 1;
    InterlockedExchange(&g_lnk_cobj_cache_init_state, 2);
    return 0;
  }

  if (!g_lnk_cobj_policy.configured) { lnk_compressed_obj_configure(0); }
  U64 cache_bytes = g_lnk_cobj_policy.initial_cache_bytes;
  U64 post_boundary_cache_bytes = g_lnk_cobj_policy.post_boundary_cache_bytes;
  U64 segment_size_u64 = segment_size;
  U64 input_capacity_hint = g_lnk_cobj_input_capacity_hint;
  if (g_lnk_cobj_policy.initial_source == LNK_CObjPolicySource_Adaptive) {
    // Keep at least one first-generation slot per write-group lane. This is only 32 MiB for
    // 512 KiB segments, avoids serializing a wave of parallel first faults on tiny targets,
    // and does not override explicit cache-size experiments.
    input_capacity_hint = Max(input_capacity_hint,
                              segment_size_u64 * LNK_COBJ_DEFAULT_WRITE_GROUP_SLOTS);
  }
  if (input_capacity_hint > 0) {
    if (input_capacity_hint < cache_bytes) {
      cache_bytes = AlignPow2(input_capacity_hint, segment_size_u64);
    }
    if (input_capacity_hint < post_boundary_cache_bytes) {
      post_boundary_cache_bytes = AlignPow2(input_capacity_hint, segment_size_u64);
    }
  }
  g_lnk_cobj_cache.skip_cleanup = lnk_cobj_skip_cleanup_enabled();
  g_lnk_cobj_cache.initial_cache_bytes = cache_bytes;
  g_lnk_cobj_cache.post_boundary_cache_bytes = post_boundary_cache_bytes;
  g_lnk_cobj_cache.slot_count = Max(1, cache_bytes / segment_size);
  g_lnk_cobj_cache.active_slot_count = g_lnk_cobj_cache.slot_count;
  g_lnk_cobj_cache.segment_size = segment_size;
  U64 mapping_size = g_lnk_cobj_cache.slot_count * segment_size;
  g_lnk_cobj_cache.slot_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                                       (DWORD)(mapping_size >> 32), (DWORD)mapping_size, 0);
  if (g_lnk_cobj_cache.slot_mapping) {
    g_lnk_cobj_cache.write_group_slot_count = LNK_COBJ_DEFAULT_WRITE_GROUP_SLOTS;
    g_lnk_cobj_cache.write_groups = lnk_cobj_alloc_write_groups(
      g_lnk_cobj_cache.slot_count, g_lnk_cobj_cache.write_group_slot_count,
      &g_lnk_cobj_cache.write_group_count);
  }
  g_lnk_cobj_cache.slot_segments = VirtualAlloc(0, g_lnk_cobj_cache.slot_count * sizeof(void *), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  // Keep the VEH lookup table fixed after publication, but make the ceiling high enough that
  // it cannot constrain practical links. One million pointers cost only 8 MiB on x64.
  g_lnk_cobj_cache.region_cap = LNK_COBJ_REGION_CAP;
  g_lnk_cobj_cache.regions = VirtualAlloc(0, g_lnk_cobj_cache.region_cap * sizeof(void *), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  if (!g_lnk_cobj_cache.slot_mapping || !g_lnk_cobj_cache.slot_segments || !g_lnk_cobj_cache.regions) {
    g_lnk_cobj_cache.failed = 1;
  } else {
    g_lnk_cobj_cache.veh = AddVectoredExceptionHandler(1, lnk_cobj_veh);
    if (!g_lnk_cobj_cache.veh) { g_lnk_cobj_cache.failed = 1; }
  }
  g_lnk_cobj_cache.initialized = 1;
  InterlockedExchange(&g_lnk_cobj_cache_init_state, 2);
  return g_lnk_cobj_cache.failed == 0;
}

////////////////////////////////
// Portable logical-address mapping

internal B32
lnk_cobj_segment_is_direct_raw(LNK_CObjSegment *segment)
{
  return (segment->flags & LNK_COBJ_SEGMENT_RAW) != 0;
}

internal B32
lnk_cobj_split_portable_raw_boundaries(LNK_CompressedObj *region)
{
  // Leave compressed runs intact. Raw runs still need exact placeholders because they are mapped
  // directly from the portable file before any OBJ pointers are published.
  for (U32 seg_idx = 1; seg_idx < region->segment_count; ++seg_idx) {
    B32 prev_raw = lnk_cobj_segment_is_direct_raw(&region->directory[seg_idx - 1]);
    B32 curr_raw = lnk_cobj_segment_is_direct_raw(&region->directory[seg_idx]);
    if (prev_raw != curr_raw) {
      U64 off = (U64)seg_idx * region->segment_size;
      if (!VirtualFree(region->base + off, region->reserve_size - off,
                       MEM_RELEASE|MEM_PRESERVE_PLACEHOLDER)) {
        return 0;
      }
    }
  }
  LNK_CObjSegment *last = &region->directory[region->segment_count - 1];
  if (lnk_cobj_segment_is_direct_raw(last) && last->raw_size < region->segment_size) {
    U64 map_end = (U64)(region->segment_count - 1) * region->segment_size +
                  AlignPow2(last->raw_size, LNK_COBJ_FILE_MAPPING_GRANULARITY);
    if (map_end < region->reserve_size &&
        !VirtualFree(region->base + map_end, region->reserve_size - map_end,
                     MEM_RELEASE|MEM_PRESERVE_PLACEHOLDER)) {
      return 0;
    }
  }
  return 1;
}

internal B32
lnk_cobj_isolate_portable_segment(LNK_CompressedObj *region, U32 seg_idx)
{
  LNK_CObjRuntimeSegment *runtime = &region->segments[seg_idx];
  AcquireSRWLockExclusive(&region->placeholder_lock);
  B32 ok = 1;
  if (!runtime->isolated_placeholder) {
    U32 first = seg_idx;
    while (first > 0 && !region->segments[first - 1].isolated_placeholder &&
           !lnk_cobj_segment_is_direct_raw(&region->directory[first - 1])) {
      first -= 1;
    }
    U32 opl = seg_idx + 1;
    while (opl < region->segment_count && !region->segments[opl].isolated_placeholder &&
           !lnk_cobj_segment_is_direct_raw(&region->directory[opl])) {
      opl += 1;
    }
    U8 *target = region->base + (U64)seg_idx * region->segment_size;
    if (seg_idx > first) {
      ok = VirtualFree(target, (U64)(opl - seg_idx) * region->segment_size,
                       MEM_RELEASE|MEM_PRESERVE_PLACEHOLDER);
    }
    if (ok && seg_idx + 1 < opl) {
      ok = VirtualFree(target + region->segment_size,
                       (U64)(opl - seg_idx - 1) * region->segment_size,
                       MEM_RELEASE|MEM_PRESERVE_PLACEHOLDER);
    }
    if (ok) { runtime->isolated_placeholder = 1; }
  }
  ReleaseSRWLockExclusive(&region->placeholder_lock);
  return ok;
}

internal void
lnk_cobj_unmap_portable_raw_runs(LNK_CompressedObj *region)
{
  for (U32 first = 0; first < region->segment_count;) {
    LNK_CObjSegment *entry = &region->directory[first];
    if (!lnk_cobj_segment_is_direct_raw(entry)) { first += 1; continue; }
    U32 opl = first + 1;
    while (opl < region->segment_count &&
           lnk_cobj_segment_is_direct_raw(&region->directory[opl]) &&
           region->directory[opl].file_offset == region->directory[opl - 1].file_offset + region->segment_size) {
      opl += 1;
    }
    g_lnk_cobj_cache.unmap_view_of_file_2(GetCurrentProcess(),
                                          region->base + (U64)first * region->segment_size,
                                          MEM_PRESERVE_PLACEHOLDER);
    first = opl;
  }
}

internal B32
lnk_cobj_map_portable_raw_runs(LNK_CompressedObj *region)
{
  for (U32 first = 0; first < region->segment_count;) {
    LNK_CObjSegment *entry = &region->directory[first];
    if (!lnk_cobj_segment_is_direct_raw(entry)) { first += 1; continue; }
    U32 opl = first + 1;
    while (opl < region->segment_count &&
           lnk_cobj_segment_is_direct_raw(&region->directory[opl]) &&
           region->directory[opl].file_offset == region->directory[opl - 1].file_offset + region->segment_size) {
      opl += 1;
    }
    U64 map_size = (U64)(opl - first) * region->segment_size;
    if (opl == region->segment_count) {
      LNK_CObjSegment *last = &region->directory[opl - 1];
      if (last->raw_size < region->segment_size) {
        map_size -= region->segment_size - AlignPow2(last->raw_size, LNK_COBJ_FILE_MAPPING_GRANULARITY);
      }
    }
    U64 file_off = entry->file_offset;
    U8 *target = region->base + (U64)first * region->segment_size;
    if ((file_off & (LNK_COBJ_FILE_MAPPING_GRANULARITY - 1)) != 0 ||
        file_off > region->mapped_file.size ||
        map_size > region->mapped_file.size - file_off ||
        g_lnk_cobj_cache.map_view_of_file_3(region->portable_mapping, GetCurrentProcess(), target,
                                            file_off, map_size, MEM_REPLACE_PLACEHOLDER,
                                            PAGE_READONLY, 0, 0) != target) {
      lnk_cobj_unmap_portable_raw_runs(region);
      return 0;
    }
    first = opl;
  }
  return 1;
}

internal void
lnk_cobj_release_placeholder_reservation(LNK_CompressedObj *region)
{
  LNK_CObjSegment *last = &region->directory[region->segment_count - 1];
  B32 split_final_raw = lnk_cobj_segment_is_direct_raw(last) &&
                        last->raw_size < region->segment_size;
  if (region->segment_count > 1 || split_final_raw) {
    VirtualFree(region->base, region->reserve_size, MEM_RELEASE|MEM_COALESCE_PLACEHOLDERS);
  }
  VirtualFree(region->base, 0, MEM_RELEASE);
}

internal void
lnk_cobj_discard_unpublished_region(LNK_CompressedObj *region)
{
  if (!region) { return; }
  region->active = 0;
  lnk_cobj_unmap_portable_raw_runs(region);
  lnk_cobj_release_placeholder_reservation(region);
  if (region->portable_mapping) { CloseHandle(region->portable_mapping); }
  HeapFree(GetProcessHeap(), 0, region);
}

////////////////////////////////
// Container validation and lifetime

internal B32
lnk_cobj_open_portable(LNK_Input *input, String8 mapped_file)
{
  if (mapped_file.size < sizeof(LNK_CObjHeader)) { return 0; }
  LNK_CObjHeader *header = (LNK_CObjHeader *)mapped_file.str;
  if (header->magic != LNK_COBJ_MAGIC) { return 0; }
  B32 has_invalid_flag_dependency =
    ((header->flags & LNK_COBJ_FLAG_UDT_HASH_INDEX) &&
     !(header->flags & LNK_COBJ_FLAG_TYPE_INDEX)) ||
    ((header->flags & LNK_COBJ_FLAG_DEBUG_S_SUMMARY) &&
     !(header->flags & LNK_COBJ_FLAG_DEBUG_S_INDEX)) ||
    ((header->flags & LNK_COBJ_FLAG_PACKED_TYPE_OFFSETS_V2) &&
     !(header->flags & LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR)) ||
    ((header->flags & LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR) &&
     !(header->flags & LNK_COBJ_FLAG_PACKED_TYPE_OFFSETS_V2)) ||
    ((header->flags & LNK_COBJ_TYPE_INDEX_COUNT_MASK) &&
     !(header->flags & LNK_COBJ_FLAG_TYPE_INDEX));
  if (header->version != LNK_COBJ_VERSION || header->header_size != sizeof(*header) ||
      header->raw_size == 0 ||
      header->segment_size < LNK_COBJ_FILE_MAPPING_GRANULARITY ||
      (header->segment_size & (header->segment_size - 1)) != 0 ||
      header->raw_size > max_U64 - (header->segment_size - 1) ||
      header->segment_count != CeilIntegerDiv(header->raw_size, header->segment_size) ||
      header->directory_offset < header->header_size || header->directory_offset > mapped_file.size ||
      (header->flags & ~LNK_COBJ_KNOWN_HEADER_FLAGS) != 0 || has_invalid_flag_dependency ||
      !(header->flags & LNK_COBJ_FLAG_PORTABLE_RAW_MAP) ||
      (U64)header->segment_count * sizeof(LNK_CObjSegment) > mapped_file.size - header->directory_offset) {
    lnk_error(LNK_Error_IllData, "invalid compressed object container: %S", input->path);
    return 0;
  }
  LNK_CObjSegment *directory = (LNK_CObjSegment *)(mapped_file.str + header->directory_offset);
  B32 has_portable_raw_segments = 0;
  U64 raw_mapped_segment_count = 0;
  for (U32 i = 0; i < header->segment_count; ++i) {
    LNK_CObjSegment *entry = &directory[i];
    U64 segment_raw_offset = (U64)i * header->segment_size;
    U32 expected_raw_size = (U32)Min((U64)header->segment_size,
                                     header->raw_size - segment_raw_offset);
    if (entry->raw_size != expected_raw_size || entry->stored_size == 0 ||
        (entry->flags & ~LNK_COBJ_SEGMENT_RAW) != 0 ||
        ((entry->flags & LNK_COBJ_SEGMENT_RAW) && entry->stored_size != entry->raw_size) ||
        entry->file_offset > mapped_file.size || entry->stored_size > mapped_file.size - entry->file_offset) {
      lnk_error(LNK_Error_IllData, "invalid compressed object segment: %S", input->path);
      return 0;
    }
    has_portable_raw_segments |= !!(entry->flags & LNK_COBJ_SEGMENT_RAW);
    raw_mapped_segment_count += !!(entry->flags & LNK_COBJ_SEGMENT_RAW);
  }
  U32 type_index_count = 0;
  LNK_CObjTypeIndex *type_indices = 0;
  LNK_CObjUdtHashIndex *udt_hash_indices = 0;
  LNK_CObjBaseRelocIndex *base_reloc_index = 0;
  LNK_CObjDebugSIndex *debug_s_index = 0;
  if (header->flags & LNK_COBJ_FLAG_TYPE_INDEX) {
    type_index_count = (header->flags & LNK_COBJ_TYPE_INDEX_COUNT_MASK) >> LNK_COBJ_TYPE_INDEX_COUNT_SHIFT;
    U64 dir_bytes = (U64)type_index_count * sizeof(LNK_CObjTypeIndex);
    if (type_index_count == 0 || header->reserved > mapped_file.size || dir_bytes > mapped_file.size - header->reserved) {
      lnk_error(LNK_Error_IllData, "invalid compressed object type index: %S", input->path);
      return 0;
    }
    type_indices = (LNK_CObjTypeIndex *)(mapped_file.str + header->reserved);
    for (U32 i = 0; i < type_index_count; ++i) {
      LNK_CObjTypeIndex *idx = &type_indices[i];
      U64 offset_count = idx->leaf_count;
      U64 offsets_bytes = offset_count * sizeof(U32);
      U64 sizes_bytes = offset_count * sizeof(U16);
      U64 kinds_bytes = offset_count * sizeof(U16);
      if (header->flags & LNK_COBJ_FLAG_PACKED_TYPE_SIDECAR) {
        sizes_bytes = 256 * sizeof(U16);
        kinds_bytes = offset_count;
        U64 group_size = (U64)1 << LNK_COBJ_PACKED_TYPE_OFFSET_V2_SHIFT;
        U64 group_count = (offset_count + group_size - 1) / group_size;
        U64 group_bytes = group_count * 2 * sizeof(U32);
        U64 payload_rel = (group_bytes + 7) & ~(U64)7;
        if (idx->offsets_file_offset > mapped_file.size || group_bytes > mapped_file.size - idx->offsets_file_offset ||
            idx->sizes_file_offset < idx->offsets_file_offset ||
            idx->sizes_file_offset - idx->offsets_file_offset < payload_rel) {
          lnk_error(LNK_Error_IllData, "invalid packed v2 type offset directory: %S", input->path);
          return 0;
        }
        U64 payload_bytes = idx->sizes_file_offset - idx->offsets_file_offset - payload_rel;
        U32 *groups = (U32 *)(mapped_file.str + idx->offsets_file_offset);
        for (U64 group_idx = 0; group_idx < group_count; ++group_idx) {
          U64 first = group_idx * group_size;
          U64 count = Min(group_size, offset_count - first);
          U32 descriptor = groups[group_idx*2 + 1];
          U64 rel = descriptor & ~(U32)1;
          U64 width = (descriptor & 1) ? 3 : 2;
          if (rel > payload_bytes || count * width > payload_bytes - rel) {
            lnk_error(LNK_Error_IllData, "invalid packed v2 type offset payload: %S", input->path);
            return 0;
          }
        }
        offsets_bytes = idx->sizes_file_offset - idx->offsets_file_offset;
      }
      if (idx->raw_section_offset > header->raw_size || idx->raw_section_size > header->raw_size - idx->raw_section_offset ||
          idx->offsets_file_offset > mapped_file.size || offsets_bytes > mapped_file.size - idx->offsets_file_offset ||
          idx->sizes_file_offset > mapped_file.size || sizes_bytes > mapped_file.size - idx->sizes_file_offset ||
          idx->kinds_file_offset > mapped_file.size || kinds_bytes > mapped_file.size - idx->kinds_file_offset) {
        lnk_error(LNK_Error_IllData, "invalid compressed object type index arrays: %S", input->path);
        return 0;
      }
    }
    if (header->flags & LNK_COBJ_FLAG_UDT_HASH_INDEX) {
      U64 udt_dir_off = header->reserved + dir_bytes;
      U64 udt_dir_bytes = (U64)type_index_count * sizeof(LNK_CObjUdtHashIndex);
      if (udt_dir_off > mapped_file.size || udt_dir_bytes > mapped_file.size - udt_dir_off) return 0;
      udt_hash_indices = (LNK_CObjUdtHashIndex *)(mapped_file.str + udt_dir_off);
      for (U32 i = 0; i < type_index_count; ++i) {
        LNK_CObjUdtHashIndex *idx = &udt_hash_indices[i];
        U64 bytes = (U64)idx->hash_count * sizeof(U64);
        if (idx->hashes_file_offset > mapped_file.size || bytes > mapped_file.size - idx->hashes_file_offset) return 0;
      }
    }
  }

  if (header->flags & LNK_COBJ_FLAG_BASE_RELOC_INDEX) {
    U64 dir_off = header->reserved;
    if (header->flags & LNK_COBJ_FLAG_TYPE_INDEX) {
      dir_off += (U64)type_index_count * sizeof(LNK_CObjTypeIndex);
      if (header->flags & LNK_COBJ_FLAG_UDT_HASH_INDEX) {
        dir_off += (U64)type_index_count * sizeof(LNK_CObjUdtHashIndex);
      }
    }
    if (dir_off > mapped_file.size || sizeof(LNK_CObjBaseRelocIndex) > mapped_file.size - dir_off) {
      lnk_error(LNK_Error_IllData, "invalid compressed object base relocation index: %S", input->path);
      return 0;
    }
    base_reloc_index = (LNK_CObjBaseRelocIndex *)(mapped_file.str + dir_off);
    U64 entry_bytes = (U64)base_reloc_index->entry_count * sizeof(LNK_CObjBaseRelocEntry);
    if (base_reloc_index->entries_file_offset > mapped_file.size ||
        entry_bytes > mapped_file.size - base_reloc_index->entries_file_offset) {
      lnk_error(LNK_Error_IllData, "invalid compressed object base relocation entries: %S", input->path);
      return 0;
    }
  }
  if (header->flags & LNK_COBJ_FLAG_DEBUG_S_INDEX) {
    U64 dir_off = header->reserved;
    if (header->flags & LNK_COBJ_FLAG_TYPE_INDEX) {
      dir_off += (U64)type_index_count * sizeof(LNK_CObjTypeIndex);
      if (header->flags & LNK_COBJ_FLAG_UDT_HASH_INDEX) {
        dir_off += (U64)type_index_count * sizeof(LNK_CObjUdtHashIndex);
      }
    }
    if (header->flags & LNK_COBJ_FLAG_BASE_RELOC_INDEX) { dir_off += sizeof(LNK_CObjBaseRelocIndex); }
    if (dir_off > mapped_file.size || sizeof(LNK_CObjDebugSIndex) > mapped_file.size - dir_off) {
      lnk_error(LNK_Error_IllData, "invalid compressed object .debug$S index: %S", input->path);
      return 0;
    }
    debug_s_index = (LNK_CObjDebugSIndex *)(mapped_file.str + dir_off);
    U64 entry_bytes = (U64)debug_s_index->entry_count * sizeof(LNK_CObjDebugSEntry);
    if (debug_s_index->entries_file_offset > mapped_file.size ||
        entry_bytes > mapped_file.size - debug_s_index->entries_file_offset) {
      lnk_error(LNK_Error_IllData, "invalid compressed object .debug$S entries: %S", input->path);
      return 0;
    }
    LNK_CObjDebugSEntry *entries = (LNK_CObjDebugSEntry *)(mapped_file.str + debug_s_index->entries_file_offset);
    for (U32 i = 0; i < debug_s_index->entry_count; ++i) {
      if ((U64)entries[i].raw_payload_offset + entries[i].raw_payload_size > header->raw_size ||
          (i && entries[i].raw_section_offset < entries[i-1].raw_section_offset)) {
        lnk_error(LNK_Error_IllData, "invalid compressed object .debug$S entry: %S", input->path);
        return 0;
      }
    }
    if (header->flags & LNK_COBJ_FLAG_DEBUG_S_SUMMARY) {
      U64 summary_off = AlignPow2(debug_s_index->entries_file_offset + entry_bytes, 8);
      U64 summary_bytes = (U64)debug_s_index->entry_count * sizeof(LNK_CObjDebugSSummary);
      if (summary_off > mapped_file.size || summary_bytes > mapped_file.size - summary_off) {
        lnk_error(LNK_Error_IllData, "invalid compressed object .debug$S summaries: %S", input->path);
        return 0;
      }
    }
  }
  if (!lnk_cobj_cache_init(header->segment_size)) {
    lnk_error(LNK_Error_IllData, "compressed object cache initialization failed");
    return 0;
  }

  HANDLE portable_mapping = 0;
  if (has_portable_raw_segments) {
    Temp scratch = scratch_begin(0, 0);
    String16 path16 = str16_from_8(scratch.arena, input->path);
    HANDLE file = CreateFileW(path16.str, GENERIC_READ,
                              FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                              0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
      // Match ordinary OBJ mapping semantics: expose raw runs through a read-only view of a
      // WRITECOPY section. Untouched pages stay file-backed without up-front commit, while the
      // generic COW VEH can privately promote the rare page patched in place by the linker.
      portable_mapping = CreateFileMappingW(file, 0, PAGE_WRITECOPY, 0, 0, 0);
      CloseHandle(file);
    }
    scratch_end(scratch);
  }
  // A fully compressed container is self-contained in mapped_file and deliberately needs no
  // second handle to its path. Only raw-stored segments require a WRITECOPY section that can be
  // mapped at their logical OBJ offsets.
  if (has_portable_raw_segments && !portable_mapping) {
    lnk_error(LNK_Error_IllData, "unable to open portable compressed object mapping: %S", input->path);
    return 0;
  }

  U64 reserve_size = (header->raw_size + (U64)header->segment_size - 1) & ~((U64)header->segment_size - 1);
  U8 *base = g_lnk_cobj_cache.virtual_alloc_2(GetCurrentProcess(), 0, reserve_size,
                                               MEM_RESERVE|MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, 0, 0);
  if (!base) {
    if (portable_mapping) { CloseHandle(portable_mapping); }
    lnk_error(LNK_Error_IllData, "unable to reserve compressed object view: %S", input->path);
    return 0;
  }

  // These descriptors need stable addresses, not independent VADs. One heap block per OBJ avoids
  // two VirtualAlloc VADs and also halves allocator calls versus separate descriptor/table blocks.
  U64 segments_off = AlignPow2(sizeof(LNK_CompressedObj), 64);
  U64 metadata_size = segments_off + sizeof(LNK_CObjRuntimeSegment) * header->segment_count;
  LNK_CompressedObj *region = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, metadata_size);
  LNK_CObjRuntimeSegment *segments = region ? (LNK_CObjRuntimeSegment *)((U8 *)region + segments_off) : 0;
  if (!region) {
    VirtualFree(base, 0, MEM_RELEASE);
    if (portable_mapping) { CloseHandle(portable_mapping); }
    lnk_error(LNK_Error_IllData, "unable to allocate compressed object metadata: %S", input->path);
    return 0;
  }
  for (U32 i = 0; i < header->segment_count; ++i) {
    InitializeSRWLock(&segments[i].lock);
    segments[i].slot_idx = max_U32;
    segments[i].address = base + (U64)i * header->segment_size;
  }
  region->base = base;
  region->reserve_size = reserve_size;
  region->raw_size = header->raw_size;
  region->segment_size = header->segment_size;
  region->segment_count = header->segment_count;
  region->mapped_file = mapped_file;
  region->header = header;
  region->directory = directory;
  region->type_indices = type_indices;
  region->udt_hash_indices = udt_hash_indices;
  region->segments = segments;
  region->portable_mapping = portable_mapping;
  if (base_reloc_index) {
    region->base_reloc_entries = (LNK_CObjBaseRelocEntry *)(mapped_file.str + base_reloc_index->entries_file_offset);
    region->base_reloc_entry_count = base_reloc_index->entry_count;
    region->has_base_reloc_index = 1;
  }
  if (debug_s_index) {
    region->debug_s_entries = (LNK_CObjDebugSEntry *)(mapped_file.str + debug_s_index->entries_file_offset);
    region->debug_s_entry_count = debug_s_index->entry_count;
    if (header->flags & LNK_COBJ_FLAG_DEBUG_S_SUMMARY) {
      U64 bytes = (U64)debug_s_index->entry_count * sizeof(LNK_CObjDebugSEntry);
      U64 off = AlignPow2(debug_s_index->entries_file_offset + bytes, 8);
      region->debug_s_summaries = (LNK_CObjDebugSSummary *)(mapped_file.str + off);
    }
  }
  region->type_index_count = type_index_count;
  region->active = 1;

  B32 placeholders_ok = lnk_cobj_split_portable_raw_boundaries(region) &&
                        lnk_cobj_map_portable_raw_runs(region);
  if (!placeholders_ok) {
    lnk_cobj_discard_unpublished_region(region);
    lnk_error(LNK_Error_IllData, "unable to map portable compressed object view: %S", input->path);
    return 0;
  }

  B32 registered = 0;
  AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
  if (g_lnk_cobj_cache.region_count < g_lnk_cobj_cache.region_cap) {
    g_lnk_cobj_cache.regions[g_lnk_cobj_cache.region_count++] = region;
    g_lnk_cobj_cache.raw_mapped_segments += raw_mapped_segment_count;
    g_lnk_cobj_cache.input_compressed_bytes += mapped_file.size;
    g_lnk_cobj_cache.input_raw_bytes += header->raw_size;
    registered = 1;
  }
  ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
  if (!registered) {
    lnk_cobj_discard_unpublished_region(region);
    lnk_error(LNK_Error_IllData, "too many compressed object inputs");
    return 0;
  }

  input->compressed_obj = region;
  input->compressed_data = mapped_file;
  input->data = str8(base, header->raw_size);
  return 1;
}

internal B32
lnk_compressed_obj_open(LNK_Input *input, String8 mapped_file)
{
  return lnk_cobj_open_portable(input, mapped_file);
}

internal void
lnk_compressed_obj_finalize_open(void)
{
  // Parallel input mapping appends regions in completion order.  The caller invokes this joined
  // boundary before starting any COFF parser task, so no logical compressed range can fault until
  // the address table has been sorted for lock-free VEH lookup.
  // Archive, generated-object and empty input batches usually add no compressed
  // regions. The table is append-only; do not sort the same addresses again.
  if (!g_lnk_cobj_cache.initialized || g_lnk_cobj_cache.region_count < 2 ||
      g_lnk_cobj_cache.region_count == g_lnk_cobj_cache.sorted_region_count) { return; }
  AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
  qsort(g_lnk_cobj_cache.regions, g_lnk_cobj_cache.region_count,
        sizeof(*g_lnk_cobj_cache.regions), lnk_cobj_region_ptr_compare);
  g_lnk_cobj_cache.sorted_region_count = g_lnk_cobj_cache.region_count;
  ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
}

internal void
lnk_compressed_obj_close(LNK_Input *input)
{
  LNK_CompressedObj *region = input->compressed_obj;
  if (!region) { return; }
  U64 cleanup_begin_us = now_time_us();
  if (g_lnk_cobj_cache.skip_cleanup) {
    // Benchmark/diagnostic mode: the process is about to exit and Windows tears the address
    // space down more efficiently than issuing one UnmapViewOfFile2 call per resident segment.
    // Useful for separating link work from explicit VAD teardown; not the bounded-cache path.
    input->owns_file_map = 0;
    InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.cleanup_us,
                             now_time_us() - cleanup_begin_us);
    return;
  }
  InterlockedExchange(&region->active, 0);
  lnk_cobj_unmap_portable_raw_runs(region);
  for (U32 i = 0; i < region->segment_count; ++i) {
    LNK_CObjRuntimeSegment *segment = &region->segments[i];
    if (segment->state == LNK_CObjSegState_Ready || segment->state == LNK_CObjSegState_Pinned) {
      g_lnk_cobj_cache.unmap_view_of_file_2(GetCurrentProcess(), region->base + (U64)i * region->segment_size,
                                             MEM_PRESERVE_PLACEHOLDER);
      if (segment->slot_idx != max_U32 && segment->slot_idx < g_lnk_cobj_cache.slot_count) {
        AcquireSRWLockExclusive(&g_lnk_cobj_cache.lock);
        if (g_lnk_cobj_cache.slot_segments[segment->slot_idx] == segment) {
          g_lnk_cobj_cache.slot_segments[segment->slot_idx] = 0;
          // Destructive shrinking can leave pinned slots above the active prefix.  They remain
          // mapped for pointer stability but are intentionally excluded from resident_count.
          if (segment->slot_idx < g_lnk_cobj_cache.active_slot_count) {
            Assert(g_lnk_cobj_cache.resident_count > 0);
            g_lnk_cobj_cache.resident_count -= 1;
          }
        }
        ReleaseSRWLockExclusive(&g_lnk_cobj_cache.lock);
      }
    }
  }
  lnk_cobj_release_placeholder_reservation(region);
  if (region->portable_mapping) { CloseHandle(region->portable_mapping); }
  file_map_view_close((FileMap){0}, input->compressed_data.str, r1u64(0, input->compressed_data.size));
  // The sorted address lookup retains region pointers for the life of the process. Keep this
  // small inactive descriptor allocated so an unrelated later access violation cannot make the
  // vectored exception handler dereference freed metadata.
  region->mapped_file = str8_zero();
  region->header = 0;
  region->directory = 0;
  region->type_indices = 0;
  region->udt_hash_indices = 0;
  region->base_reloc_entries = 0;
  region->debug_s_entries = 0;
  region->debug_s_summaries = 0;
  region->portable_mapping = 0;
  input->data = str8_zero();
  input->compressed_data = str8_zero();
  input->compressed_obj = 0;
  input->owns_file_map = 0;
  InterlockedExchangeAdd64((volatile LONG64 *)&g_lnk_cobj_cache.cleanup_us,
                           now_time_us() - cleanup_begin_us);
}

////////////////////////////////
// Statistics

internal void
lnk_compressed_obj_log_stats(void)
{
  if (!g_lnk_cobj_cache.initialized || g_lnk_cobj_cache.region_count == 0) {
    if (g_lnk_cobj_window_decodes) { lnk_log(LNK_Log_Timers, "[cobj window] decodes=%llu stored=%llu MiB", g_lnk_cobj_window_decodes, g_lnk_cobj_window_bytes / MB(1)); }
    return;
  }
  U64 unique_decodes = g_lnk_cobj_cache.decodes - g_lnk_cobj_cache.redecodes;
  U64 total_decodes = g_lnk_cobj_cache.decodes + g_lnk_cobj_window_decodes;
  U64 total_decoded_bytes = g_lnk_cobj_cache.decoded_bytes + g_lnk_cobj_window_decoded_bytes;
  U64 total_stored_bytes = g_lnk_cobj_cache.stored_bytes_read + g_lnk_cobj_window_bytes;
  lnk_log(LNK_Log_Timers,
          "[cobj] files=%llu input=%llu/%llu MiB segment=%llu KiB cache=%llu MiB(%s)->%llu MiB(%s) freeze=%u(%s) trim=%u(%s) one-shot=%u(%s) resident-hwm=%llu faults=%llu unique=%llu decodes=%llu decoded=%llu MiB read=%llu MiB evictions=%llu redecodes=%llu frozen=%llu/%llu MiB raw-mapped=%llu decoder-scratch=%llu/%llu KiB time-ms{decode=%.2f fault=%.2f evict=%.2f freeze=%.2f trim=%.2f cleanup=%.2f}\n",
          g_lnk_cobj_cache.region_count,
          g_lnk_cobj_cache.input_compressed_bytes / MB(1),
          g_lnk_cobj_cache.input_raw_bytes / MB(1),
          g_lnk_cobj_cache.segment_size / KB(1),
          g_lnk_cobj_cache.initial_cache_bytes / MB(1),
          lnk_cobj_policy_source_string(g_lnk_cobj_policy.initial_source),
          g_lnk_cobj_cache.post_boundary_cache_bytes / MB(1),
          lnk_cobj_policy_source_string(g_lnk_cobj_policy.post_source),
          g_lnk_cobj_policy.freeze_generation,
          lnk_cobj_policy_source_string(g_lnk_cobj_policy.freeze_source),
          g_lnk_cobj_policy.trim_mode,
          lnk_cobj_policy_source_string(g_lnk_cobj_policy.trim_source),
          g_lnk_cobj_policy.one_shot,
          lnk_cobj_policy_source_string(g_lnk_cobj_policy.one_shot_source),
          g_lnk_cobj_cache.occupancy_hwm, g_lnk_cobj_cache.faults, unique_decodes, total_decodes,
          total_decoded_bytes / MB(1), total_stored_bytes / MB(1),
          g_lnk_cobj_cache.evictions,
          g_lnk_cobj_cache.redecodes,
          g_lnk_cobj_cache.frozen_segments,
          (g_lnk_cobj_cache.frozen_segments * g_lnk_cobj_cache.segment_size) / MB(1),
          g_lnk_cobj_cache.raw_mapped_segments,
          g_lnk_cobj_decoder_scratch_count,
          g_lnk_cobj_decoder_scratch_bytes / KB(1),
          (F64)g_lnk_cobj_cache.decode_us / 1000.0,
          (F64)g_lnk_cobj_cache.fault_us / 1000.0,
          (F64)g_lnk_cobj_cache.eviction_us / 1000.0,
          (F64)g_lnk_cobj_cache.freeze_us / 1000.0,
          (F64)g_lnk_cobj_cache.trim_us / 1000.0,
          (F64)g_lnk_cobj_cache.cleanup_us / 1000.0);
  if (g_lnk_cobj_cache.frozen_segments) {
    lnk_log(LNK_Log_Timers, "[cobj frozen] segments=%llu", g_lnk_cobj_cache.frozen_segments);
  }
  if (g_lnk_cobj_cache.redecodes) {
    lnk_log(LNK_Log_Timers, "[cobj redecodes] debug_s=%llu types=%llu segment0=%llu",
            g_lnk_cobj_redecode_debug_s, g_lnk_cobj_redecode_types,
            g_lnk_cobj_redecode_segment_zero);
  }
  if (g_lnk_cobj_window_decodes) {
    lnk_log(LNK_Log_Timers, "[cobj window] decodes=%llu stored=%llu MiB",
            g_lnk_cobj_window_decodes, g_lnk_cobj_window_bytes / MB(1));
  }
}

#else
internal void lnk_compressed_obj_configure(struct LNK_Config *config) { (void)config; }
internal void lnk_compressed_obj_prepare_cache(String8 *mapped_files, U64 count) { (void)mapped_files; (void)count; }
internal void lnk_compressed_obj_trim_working_set(void) {}
internal String8 lnk_compressed_obj_direct_range(LNK_CompressedObj *obj, Rng1U64 range) { return str8_zero(); }
internal B32 lnk_compressed_obj_type_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjTypeIndexView *out) { MemoryZeroStruct(out); return 0; }
internal B32 lnk_compressed_obj_base_reloc_index(LNK_CompressedObj *obj, LNK_CObjBaseRelocView *out) { MemoryZeroStruct(out); return 0; }
internal B32 lnk_compressed_obj_debug_s_index(LNK_CompressedObj *obj, Rng1U64 section_range, LNK_CObjDebugSView *out) { MemoryZeroStruct(out); return 0; }
internal B32 lnk_compressed_obj_copy_range(LNK_CompressedObj *obj, Rng1U64 range, void *dst, LNK_CObjDecodeWindow *window) { return 0; }
internal B32 lnk_compressed_obj_copy_string(LNK_CompressedObj *obj, String8 src, void *dst, LNK_CObjDecodeWindow *window) { return 0; }
internal void lnk_compressed_obj_release_window(LNK_CObjDecodeWindow *window) {}
internal U32 lnk_compressed_obj_segment_count(LNK_CompressedObj *obj) { return 0; }
internal U32 lnk_compressed_obj_segment_size(LNK_CompressedObj *obj) { return 0; }
internal Rng1U64 lnk_compressed_obj_stored_segment_range(LNK_CompressedObj *obj, U32 segment_idx) { Rng1U64 r = {0}; return r; }
internal B32
lnk_compressed_obj_open(struct LNK_Input *input, String8 mapped_file)
{
  if (mapped_file.size >= sizeof(LNK_CObjHeader) &&
      ((LNK_CObjHeader *)mapped_file.str)->magic == LNK_COBJ_MAGIC) {
    lnk_error(LNK_Error_IllData,
              "compressed object input requires an Oodle-enabled RAD Link build: %S", input->path);
  }
  return 0;
}
internal void lnk_compressed_obj_finalize_open(void) {}
internal void lnk_compressed_obj_close(struct LNK_Input *input) {}
internal void lnk_compressed_obj_log_stats(void) {}
internal void lnk_compressed_obj_log_phase_stats(char *tag) {}
#endif
