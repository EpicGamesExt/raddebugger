// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

static Arena *g_huge_arena = 0;

internal Arena *
lnk_get_huge_arena(void)
{
  if (g_huge_arena == 0) {
    // 2MB commit quantum (vs the 64KB default): this arena backs multi-GB debug
    // info merges; the larger quantum cuts VirtualAlloc(MEM_COMMIT) syscalls
    // (all serialized on the process address-space lock) ~32x for at most 2MB
    // of slack past the high-water mark.
    g_huge_arena = arena_alloc(.commit_size = MB(2), .name = "HUGE");
  }
  return g_huge_arena;
}

// Handle of the in-flight background arena-release thread (at most one). Joined
// (a) before launching the next reaper and (b) at the end of the link, next to
// the image-write-thread join. NOTE: do NOT thread_detach right after
// thread_launch -- that releases the W32_Entity the new thread's entry point is
// about to read (startup race).
static Thread g_arena_reaper_thread = {0};

internal void
lnk_arena_release_thread(void *raw_arena)
{
  // REAPER: releasing a huge arena costs ~50-100ms/GB of committed pages in the
  // kernel (MiDeleteVaDirect/MiDecommitFreePage walk every PTE under
  // VirtualFree(MEM_RELEASE)), and that work serializes on the process
  // address-space lock -- chunking it across the thread pool does NOT make it
  // faster (measured: 8 GiB in 860 ms chunked-parallel vs 371 ms serial). So
  // instead take it off the critical path entirely: release on a background
  // thread while the main thread proceeds. Caller must hand over EXCLUSIVE
  // ownership -- no reference to the arena (or memory inside it) may survive
  // the thread_launch.
  ProfBeginFunction();
  U64 begin_us = now_time_us();
  Arena *arena = raw_arena;

  U64 committed_size = 0;
  for (Arena *n = arena->current; n != 0; n = n->prev)   { committed_size += n->cmt; }
#if ARENA_FREE_LIST
  for (Arena *n = arena->free_last; n != 0; n = n->prev) { committed_size += n->cmt; }
#endif

  arena_release(arena);

  lnk_log(LNK_Log_Timers, "[teardown] background release of %llu MiB arena took %.2f ms (off main thread)",
          committed_size / MB(1), (F64)(now_time_us() - begin_us) / 1000.0);
  ProfEnd();
}

// Reaper entry point for a per-worker arena array (TP_Arena). Same ownership rule as
// lnk_arena_release_thread: caller hands over EXCLUSIVE ownership. The TP_Arena header and its
// v[] array live inside v[0] (tp_arena_alloc layout), which tp_arena_release frees last, so the
// walk below and the release order are safe.
internal void
lnk_tp_arena_release_thread(void *raw_arena)
{
  ProfBeginFunction();
  U64       begin_us = now_time_us();
  TP_Arena *tp_arena = raw_arena;

  U64 committed_size = 0;
  for EachIndex(i, tp_arena->count) {
    for (Arena *n = tp_arena->v[i]->current; n != 0; n = n->prev)   { committed_size += n->cmt; }
#if ARENA_FREE_LIST
    for (Arena *n = tp_arena->v[i]->free_last; n != 0; n = n->prev) { committed_size += n->cmt; }
#endif
  }

  tp_arena_release(&tp_arena);

  lnk_log(LNK_Log_Timers, "[teardown] background release of %llu MiB worker arenas took %.2f ms (off main thread)",
          committed_size / MB(1), (F64)(now_time_us() - begin_us) / 1000.0);
  ProfEnd();
}

////////////////////////////////////////////////////////////////////////////////
//~ Fault-storm mitigation: batched PrefetchVirtualMemory over mapped input
//  ranges. The .debug$S/$T parse and type-merge loops first-touch tens of GB of
//  memory-mapped obj sections one 4K page fault at a time; with ~100+ links in
//  flight on a build farm those per-page traps saturate the kernel machine-wide
//  (prod, 126 concurrent links: dbg phase 682s kernel vs 303s user, 42M faults
//  in mcvi alone). PrefetchVirtualMemory populates the ranges in bulk (large MM
//  batches, no per-page trap), so issue it over each phase's input ranges right
//  before the parse walk. Purely a paging hint: no output byte depends on it,
//  and failure is silently ignored (pre-Win8 OS / memory pressure). A lone
//  link skips the hint because walking its already-cached 80+ GiB input set is
//  measurable overhead; concurrent shared-pool links retain it to reduce the
//  machine-wide fault storm.

#if OS_WINDOWS
// declared locally so we do not depend on the SDK's _WIN32_WINNT gate for
// WIN32_MEMORY_RANGE_ENTRY; layout matches memoryapi.h exactly
typedef struct LNK_Win32MemoryRangeEntry
{
  void  *VirtualAddress;
  SIZE_T NumberOfBytes;
} LNK_Win32MemoryRangeEntry;
typedef BOOL LNK_Win32PrefetchVirtualMemoryFunc(HANDLE process, ULONG_PTR count, LNK_Win32MemoryRangeEntry *ranges, ULONG flags); // WINAPI omitted: x64-only convention

// entries per task: the kernel's per-page population work dominates the
// syscall overhead, so small batches fanned out over the pool parallelize the
// MM work (14 GiB of mcvi input: ~0.9 s serial -> a wide parallel burst)
#define LNK_PREFETCH_BATCH_SIZE 256

typedef struct
{
  LNK_Win32PrefetchVirtualMemoryFunc *proc;
  U64                                 entry_count;
  LNK_Win32MemoryRangeEntry          *entries;
} LNK_PrefetchTask;

internal
THREAD_POOL_TASK_FUNC(lnk_prefetch_task)
{
  LNK_PrefetchTask *task = raw_task;
  U64 lo = task_id * LNK_PREFETCH_BATCH_SIZE;
  U64 hi = Min(lo + LNK_PREFETCH_BATCH_SIZE, task->entry_count);
  if (lo < hi) {
    task->proc(GetCurrentProcess(), (ULONG_PTR)(hi - lo), task->entries + lo, 0);
  }
}
#endif

// Run a per-item parallel-for on at most `cap` workers. The debug-input stages
// this wraps are page-fault-bound: the kernel working-set-insert path tops out
// near ~3M pages/s regardless of thread count, so lanes past ~20 only convert
// free cores into spin inside the fault handler (the knee moved up after giant-input
// jobification; 24+ lanes still regress wall and sharply increase kernel CPU). Items
// are pulled from a shared cursor, so per-item outputs land in the same
// item-indexed slots as the uncapped path -- output is byte-identical.
typedef struct
{
  TP_TaskFunc *func;
  void        *data;
  U64          item_count;
  U64          cursor;
} LNK_CappedForTask;

internal
THREAD_POOL_TASK_FUNC(lnk_capped_for_task)
{
  LNK_CappedForTask *wrap = raw_task;
  for (;;) {
    U64 item_idx = ins_atomic_u64_inc_eval(&wrap->cursor) - 1;
    if (item_idx >= wrap->item_count) { break; }
    wrap->func(arena, worker_id, item_idx, wrap->data, tp);
  }
}

internal void
lnk_tp_for_parallel_capped(TP_Context *tp, TP_Arena *task_arena, U64 cap, U64 item_count, TP_TaskFunc *func, void *data)
{
  if (cap == 0 || cap >= item_count) {
    tp_for_parallel(tp, task_arena, item_count, func, data);
  } else {
    LNK_CappedForTask wrap = { .func = func, .data = data, .item_count = item_count };
    tp_for_parallel(tp, task_arena, cap, lnk_capped_for_task, &wrap);
  }
}

#define lnk_tp_for_parallel_capped_prof(pool, arena, cap, item_count, task_func, task_data, zone_name) ProfBegin(zone_name); lnk_tp_for_parallel_capped(pool, arena, cap, item_count, task_func, task_data); ProfEnd();

internal void
lnk_prefetch_ranges(TP_Context *tp, U64 worker_cap, U64 range_count, Rng1U64 *ranges)
{
#if OS_WINDOWS
  // resolve once (Win8+; on older OS fall through silently). Only called from
  // serial phase-setup code, so the local_persist init has no race.
  local_persist LNK_Win32PrefetchVirtualMemoryFunc *prefetch_proc          = 0;
  local_persist B32                                 prefetch_proc_resolved = 0;
  if (!prefetch_proc_resolved) {
    prefetch_proc_resolved = 1;
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (kernel32 != 0) {
      prefetch_proc = (LNK_Win32PrefetchVirtualMemoryFunc *)GetProcAddress(kernel32, "PrefetchVirtualMemory");
    }
  }
  if (prefetch_proc == 0 || range_count == 0) { return; }

  Temp scratch = scratch_begin(0,0);

  // Coalesce page-aligned neighbors with a single linear pass: ranges arrive
  // obj-by-obj in file-offset order, so adjacent sections of the same mapped
  // obj (the common case by far) fold into one entry. No sort -- the API does
  // not require ordered or disjoint ranges, overlap just costs a cheap re-walk.
  LNK_Win32MemoryRangeEntry *entries     = push_array_no_zero(scratch.arena, LNK_Win32MemoryRangeEntry, range_count);
  U64                        entry_count = 0;
  U64 pending_min = 0, pending_max = 0;
  for EachIndex(range_idx, range_count) {
    if (ranges[range_idx].min >= ranges[range_idx].max) { continue; }
    U64 min = AlignDownPow2(ranges[range_idx].min, KB(4));
    U64 max = AlignPow2    (ranges[range_idx].max, KB(4));
    if (pending_max != 0 && min <= pending_max && max >= pending_min) {
      pending_min = Min(pending_min, min);
      pending_max = Max(pending_max, max);
      continue;
    }
    if (pending_max != 0) {
      entries[entry_count].VirtualAddress = (void *)pending_min;
      entries[entry_count].NumberOfBytes  = (SIZE_T)(pending_max - pending_min);
      entry_count += 1;
    }
    pending_min = min;
    pending_max = max;
  }
  if (pending_max != 0) {
    entries[entry_count].VirtualAddress = (void *)pending_min;
    entries[entry_count].NumberOfBytes  = (SIZE_T)(pending_max - pending_min);
    entry_count += 1;
  }

  // fan the batches out over the pool: population is per-page kernel work, so
  // this turns a serial ~1 s stall into a wide parallel burst. Purely advisory
  // syscalls with no output -- any batch interleaving is fine.
  LNK_PrefetchTask task        = { .proc = prefetch_proc, .entry_count = entry_count, .entries = entries };
  U64              batch_count = CeilIntegerDiv(entry_count, LNK_PREFETCH_BATCH_SIZE);
  if (tp != 0 && batch_count > 1) {
    lnk_tp_for_parallel_capped(tp, 0, worker_cap, batch_count, lnk_prefetch_task, &task);
  } else {
    for EachIndex(batch_idx, batch_count) { lnk_prefetch_task(0, 0, batch_idx, &task, 0); }
  }

  scratch_end(scratch);
#endif
}

// PrefetchVirtualMemory was added to reduce the machine-wide page-fault storm
// when many shared-pool linker processes run concurrently. For a lone link it
// only populates pages that the parsing walks immediately touch again, adding a
// full extra pass over tens of GiB. The shared-pool process counter is advisory,
// which is exactly the precision this paging hint needs.
internal B32
lnk_should_prefetch_mapped_input(void)
{
  U32 attached_process_count = 0;
  U32 max_process_count      = 0;
  tp_procs_snapshot(&attached_process_count, &max_process_count);
  return attached_process_count > 1;
}

internal void
lnk_discard_cv_debug_info(LNK_CodeViewInput *input, U64 obj_idx)
{
  // discard types
  MemoryZeroStruct(&input->debug_t_arr[obj_idx]);

  // discard symbols (provenance zeroed in lockstep with the data list)
  String8List *symbols_ptr = cv_sub_section_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
  MemoryZeroStruct(symbols_ptr);
  MemoryZeroStruct(cv_sub_section_prov_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols));

  // discard inline sites
  String8List *inlineelines_ptr = cv_sub_section_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  MemoryZeroStruct(inlineelines_ptr);
  MemoryZeroStruct(cv_sub_section_prov_ptr_from_debug_s(&input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines));
}

// Build the same lightweight C13 lists/provenance as cv_debug_s_from_data without walking the
// compressed logical section.  Payload pointers are intentionally not dereferenced here; later
// consumers retain the exact original virtual addresses and fault/decode only if they truly need
// record bytes.
internal B32
lnk_debug_s_from_compressed_index(Arena *arena, LNK_Obj *obj, U32 sect_idx, CV_DebugS *out)
{
  if (!obj->compressed_obj) { return 0; }
  LNK_ObjSection section = lnk_obj_section_from_section_number(obj, sect_idx + 1);
  LNK_CObjDebugSView view = {0};
  if (!lnk_compressed_obj_debug_s_index(obj->compressed_obj, section.frange, &view)) { return 0; }
  for EachIndex(i, view.count) {
    LNK_CObjDebugSEntry *entry = &view.v[i];
    U64 payload_min = entry->raw_payload_offset;
    U64 payload_max = payload_min + entry->raw_payload_size;
    if (payload_min < section.frange.min || payload_max > section.frange.max) { return 0; }
    U64 idx = cv_c13_sub_section_idx_from_kind(entry->kind);
    str8_list_push(arena, &out->data_list[idx], str8(obj->coff.data.str + payload_min, entry->raw_payload_size));
    cv_debug_s_prov_list_push(arena, &out->prov_list[idx], payload_min - section.frange.min,
                              entry->raw_payload_size, sect_idx, 0);
    if (view.summaries && entry->kind == CV_C13SubSectionKind_Symbols) {
      LNK_CObjDebugSSummary *summary = &view.summaries[i];
      CV_DebugSProvNode *prov = out->prov_list[idx].last;
      prov->module_symbol_size = summary->module_symbol_size;
      prov->gsi_candidate_count = summary->gsi_candidate_count;
      prov->proc_ref_count = summary->proc_ref_count;
      prov->symbol_summary_valid = 1;
    }
  }
  return 1;
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_s_task)
{
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  String8List sect_list = task->debug_s_list_arr[obj_idx];
  CV_DebugS  *debug_s   = &task->debug_s_arr    [obj_idx];

  U32Array sect_indices = task->debug_s_sect_idx_arr[obj_idx];
  Assert(sect_indices.count == sect_list.node_count);

  U64 input_ordinal = 0;
  for (String8Node *n = sect_list.first; n != 0; n = n->next, input_ordinal += 1) {
    // parse & merge sub sections
    U32 sect_idx = sect_indices.v[input_ordinal];
    CV_DebugS ds = {0};
    if (!lnk_debug_s_from_compressed_index(arena, task->obj_arr[obj_idx], sect_idx, &ds)) {
      ds = cv_debug_s_from_data(arena, n->string);
      cv_debug_s_tag_prov_sect(&ds, sect_idx);
    }
    cv_debug_s_concat_in_place(debug_s, &ds);

    // make sure there is one string table
    String8List string_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_StringTable);
    if (string_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S has %u string table sub-sections defined, picking first sub-section", string_data_list.node_count);
    }

    // make sure there is one file checksum table
    String8List checksum_data_list = cv_sub_section_from_debug_s(*debug_s, CV_C13SubSectionKind_FileChksms);
    if (checksum_data_list.node_count > 1) {
      // TODO: print section index
      lnk_error_obj(LNK_Warning_IllData, task->obj_arr[obj_idx], ".debug$S has %u file checksum sub-sections defined, picking first sub-section", checksum_data_list.node_count);
    }
  }

  // ICF-folded functions' associated .debug$S (dead-stripped, excluded from the list above):
  // merge ONLY their Lines subsections. The reloc patcher patched them to the fold leader's RVA,
  // so source breakpoints on folded bodies bind; symbol records stay dropped (that is the bulk
  // of link.exe's size cost for the same feature). File ids in these Lines index the obj-wide
  // FILECHKSMS merged above, so they stay consistent within this module. Mark 2 (fold joins a
  // different source location and has locals) keeps the WHOLE record tree instead, so the watch
  // window labels a folded frame with that source's own variable names.
  {
    LNK_Obj *obj = task->obj_arr[obj_idx];
    if (obj->icf_lines_only != 0) {
      for (U32 section_number = 1; section_number <= obj->coff.sections.count_no_null; section_number += 1) {
        if (!obj->icf_lines_only[section_number]) { continue; }
        CV_DebugS ds = {0};
        if (!lnk_debug_s_from_compressed_index(arena, obj, section_number - 1, &ds)) {
          String8 raw_data = lnk_obj_section_data_from_number(obj, section_number);
          ds = cv_debug_s_from_data(arena, raw_data);
          cv_debug_s_tag_prov_sect(&ds, section_number - 1); // merged from a different child section
        }
        if (obj->icf_lines_only[section_number] == 2) {
          cv_debug_s_concat_in_place(debug_s, &ds);
        } else {
          cv_debug_s_concat_sub_section_in_place(debug_s, &ds, CV_C13SubSectionKind_Lines);
        }
      }
    }
  }

  cv_debug_s_validate_prov(debug_s);
}

internal int
lnk_symbol_input_task_is_before(void *raw_a, void *raw_b)
{
  LNK_SymbolInputTask *a = raw_a, *b = raw_b;

  if (a->weight == b->weight) {
    return a->input_range.min < b->input_range.min;
  }

  return a->weight > b->weight;
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_h_task)
{
  U64                obj_idx = task_id;
  LNK_CodeViewInput *task    = raw_task;

  LNK_Obj *obj = task->obj_arr[obj_idx];
  if (obj->coff.debug_h_section_number > 0) {
    String8    raw_debug_h     = lnk_obj_section_data_from_number(obj, obj->coff.debug_h_section_number);
    CV_DebugH *debug_h         = &task->debug_h_arr[obj_idx];
    LLVM_GHash ghash           = {0};
    U64        ghash_read_size = str8_deserial_read_struct(raw_debug_h, 0, &ghash);

    // was header read completely?
    if (ghash_read_size != sizeof(ghash)) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    ".debug$H section is too small to contain the header");
      goto exit;
    }

    // validate magic
    if (ghash.magic != LLVM_GHash_Magic) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    ".debug$H contains invalid magic: got 0x%x, expected 0x%x", ghash.magic, LLVM_GHash_Magic);
      goto exit;
    }

    // validate version
    if (ghash.version != LLVM_GHash_CurrentVersion) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H version: got %u, expected %u",
                    ghash.version, LLVM_GHash_CurrentVersion);
      goto exit;
    }

    // validate hashing algorithm
    if (lnk_hash_kind_from_llvm(ghash.hash_alg) != task->config->debug_types_hash) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H hash algorithm: got %S, expected %S; types will be rehashed",
                    llvm_string_from_ghash_alg(ghash.hash_alg),
                    lnk_string_hash_kind(task->config->debug_types_hash));
      goto exit;
    }

    // input.debug_h_arr must be 1:1 with input.debug_t_arr
    U64 hash_size  = llvm_hash_size_from_alg(ghash.hash_alg);
    U64 hash_count = (raw_debug_h.size - sizeof(ghash)) / hash_size;
    if (hash_count != task->debug_t_arr[obj_idx].count) {
      lnk_error_obj(LNK_Warning_GHash, task->obj_arr[obj_idx],
                    "mismatched .debug$H hash count and type count: got %llu hashes for %llu types",
                    hash_count, task->debug_t_arr[obj_idx].count);
      goto exit;
    }

    // load hashes
    String8 hashes = str8_substr(raw_debug_h, r1u64(sizeof(ghash), raw_debug_h.size));
    debug_h->count = hash_count;
    debug_h->v     = (U64 *)hashes.str;
  }

  exit:;
}

internal
THREAD_POOL_TASK_FUNC(lnk_strip_debug_t_sig_task)
{
  U64               obj_idx = task_id;
  LNK_ParseCvTypes *task    = raw_task;

  for EachIndex(i, task->raw_types[obj_idx].count) {
    String8 *d = task->raw_types[obj_idx].v + i;
    if (d->size == 0)                   { continue; }
    if (d->size < sizeof(CV_Signature)) { lnk_error_obj(LNK_Error_IllData, task->input->obj_arr[obj_idx], ".debug$T must have at least 4 bytes for CodeView signature"); continue; }

    CV_Signature sig = cv_signature_from_debug_s(*d);
    switch (sig) {
    default: {
      lnk_error_obj(LNK_Warning_IllData, task->input->obj_arr[obj_idx], "unknown CodeView type signature in section (TODO: print section index)");
      *d = str8(0,0);
    } break;
    case CV_Signature_C13: {
      *d = str8_skip(*d, sizeof(CV_Signature));
    } break;
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_t_task)
{
  ProfBeginFunction();
  LNK_ParseCvTypes *task = raw_task;
  if (task->out_types[task_id].offsets != 0 || task->out_types[task_id].sidecar_packed) {
    // Container sidecar or giant obj already supplied the index.
  } else if (task->raw_types[task_id].count > 0) {
    task->out_types[task_id] = cv_debug_t_from_data(arena, task->raw_types[task_id].v[0], CV_LeafAlign);
  } else {
    MemoryZeroStruct(&task->out_types[task_id]);
  }
  ProfEnd();
}

typedef struct LNK_BackgroundPrefetch
{
  U64 range_count;
  Rng1U64 *ranges;
} LNK_BackgroundPrefetch;

internal void
lnk_background_prefetch_thread(void *raw_task)
{
  LNK_BackgroundPrefetch *task = raw_task;
  lnk_prefetch_ranges(0, 0, task->range_count, task->ranges);
}

#define LNK_WINNER_PREFETCH_WORKER_MAX 32
typedef struct LNK_WinnerPrefetchFlight
{
  Thread threads[LNK_WINNER_PREFETCH_WORKER_MAX];
  LNK_BackgroundPrefetch tasks[LNK_WINNER_PREFETCH_WORKER_MAX];
  U32 worker_count;
  U64 range_count;
  U64 stored_bytes;
  U64 begin_us;
} LNK_WinnerPrefetchFlight;

internal int
lnk_prefetch_range_compare(const void *a, const void *b)
{
  Rng1U64 *ra = (Rng1U64 *)a;
  Rng1U64 *rb = (Rng1U64 *)b;
  return ra->min < rb->min ? -1 : ra->min > rb->min;
}

internal void
lnk_winner_prefetch_start(LNK_WinnerPrefetchFlight *flight, U32 worker_count,
                          U64 range_count, Rng1U64 *ranges, U64 stored_bytes)
{
  if (range_count == 0) { return; }
  qsort(ranges, range_count, sizeof(*ranges), lnk_prefetch_range_compare);
  flight->worker_count = Min(worker_count, range_count);
  flight->range_count = range_count;
  flight->stored_bytes = stored_bytes;
  flight->begin_us = now_time_us();
  for EachIndex(worker_idx, flight->worker_count) {
    U64 min = range_count * worker_idx / flight->worker_count;
    U64 max = range_count * (worker_idx + 1) / flight->worker_count;
    flight->tasks[worker_idx] = (LNK_BackgroundPrefetch){max - min, ranges + min};
    flight->threads[worker_idx] = thread_launch(lnk_background_prefetch_thread,
                                                &flight->tasks[worker_idx]);
  }
}

internal void
lnk_winner_prefetch_join(LNK_WinnerPrefetchFlight *flight)
{
  for EachIndex(worker_idx, flight->worker_count) {
    if (flight->threads[worker_idx].u64[0]) {
      thread_join(flight->threads[worker_idx], max_U64);
    }
  }
  if (flight->worker_count) {
    lnk_log(LNK_Log_Timers, "[cobj prefetch] workers=%u ranges=%llu stored=%llu MiB elapsed=%.3fs",
            flight->worker_count, flight->range_count, flight->stored_bytes / MB(1),
            (F64)(now_time_us() - flight->begin_us) / 1e6);
  }
}

// Install the mapped leaf index carried by a compressed-object container.
// This replaces the full leaf-body chain walk with a compact kind scan; the
// body bytes remain behind the bounded decoder until a real consumer asks for
// a leaf.
internal
THREAD_POOL_TASK_FUNC(lnk_parse_debug_t_sidecar_task)
{
  LNK_ParseCvTypes *task = raw_task;
  U64 obj_idx = task_id;
  if (task->raw_types[obj_idx].count == 0) { return; }
  LNK_Obj *obj = task->input->obj_arr[obj_idx];
  U32 section_number = task->is_debug_p ? obj->coff.debug_p_section_number : obj->coff.debug_t_section_number;
  if (!obj->compressed_obj || section_number == 0 || section_number > obj->coff.sections.count_no_null) { return; }
  LNK_ObjSection sect = lnk_obj_section_from_section_number(obj, section_number);
  Rng1U64 leaf_range = rng_1u64(sect.frange.min + sizeof(CV_Signature), sect.frange.max);
  LNK_CObjTypeIndexView view = {0};
  if (!lnk_compressed_obj_type_index(obj->compressed_obj, leaf_range, &view)) { return; }

  CV_DebugT *out = &task->out_types[obj_idx];
  out->data          = task->raw_types[obj_idx].v[0];
  out->count         = view.count;
  out->offsets       = view.offsets;
  out->sidecar_sizes = view.sizes;
  out->sidecar_kinds = view.kinds;
  out->sidecar_packed_v2_offset_groups = view.packed_v2_offset_groups;
  out->sidecar_packed_v2_offset_payload = view.packed_v2_offset_payload;
  out->sidecar_packed_kind_dictionary = view.packed_kind_dictionary;
  out->sidecar_packed_kind_codes = view.packed_kind_codes;
  out->sidecar_packed = view.packed_sidecar;
  out->sidecar_offset_checkpoint_shift = view.offset_checkpoint_shift;
  out->sidecar_raw_base = leaf_range.min;
  out->sidecar_complete_udt_hashes = view.complete_udt_hashes;
  out->sidecar_complete_udt_hash_count = view.complete_udt_hash_count;
  for EachIndex(i, view.count) {
    out->source_counts[cv_type_index_source_from_leaf_kind(cv_debug_t_get_leaf_kind(out, i))] += 1;
  }
  for EachElement(i, out->ti_ranges) { out->ti_ranges[i] = r1u64(CV_MinComplexTypeIndex, CV_MinComplexTypeIndex + out->count); }
  if (out->count && cv_debug_t_get_leaf_kind(out, 0) == CV_LeafKind_PRECOMP) {
    CV_PrecompInfo precomp_info = cv_precomp_info_from_leaf(cv_debug_t_get_leaf(out, 0));
    for EachElement(i, out->ti_ranges) { out->ti_ranges[i].max += precomp_info.leaf_count; }
  }
}

// Giant .debug$T parse: a single SharedPCH-scale .debug$T is a multi-second
// SERIAL pointer-chase (each leaf's offset depends on the previous leaf's
// size), and a handful of such objs bound the whole capped parse stage
// (measured: one 6.5 s obj on the FN editor DLL while the rest of the pool
// sat parked). Speculative mid-stream resynchronization is unsound for
// CodeView (arbitrary payload bytes chain "validly", so a wrong guess is not
// locally detectable), so instead: one cheap serial hop per giant walks the
// true chain recording a checkpoint offset every LNK_GIANT_DEBUG_T_INTERVAL
// leaves, then the full pool re-walks the intervals from those true
// boundaries, storing leaf offsets and classifying kinds. Offsets come from
// the true chain and per-source counts are reduced in interval order, so the
// result is bit-identical to the serial parse.
#define LNK_GIANT_DEBUG_T_SIZE     MB(16)
#define LNK_GIANT_DEBUG_T_INTERVAL (64*1024)

typedef U64 LNK_GiantSourceCounts[CV_TypeIndexSource_COUNT];

typedef struct
{
  U64                    obj_idx;
  String8                data;
  U64                    leaf_count;
  U64                    interval_count;
  U32                   *checkpoints;      // [interval_count] leaf offset at each interval start
  LNK_GiantSourceCounts *interval_counts;
} LNK_GiantDebugT;

typedef struct
{
  LNK_ParseCvTypes *parse;
  LNK_GiantDebugT  *giants;
  U32              *interval_giant;  // flat interval index -> giant index
  U32              *interval_local;  // flat interval index -> interval within giant
} LNK_GiantDebugTTask;

internal
THREAD_POOL_TASK_FUNC(lnk_giant_debug_t_hop_task)
{
  ProfBeginFunction();
  LNK_GiantDebugTTask *task = raw_task;
  LNK_GiantDebugT     *g    = &task->giants[task_id];
  String8              data = g->data;

  // bare chain walk; bounds replicate cv_read_leaf exactly (incl. its
  // total-size quirks) so the leaf set matches the serial parse bit for bit.
  // CV_LeafAlign == 1 => stride is sizeof(CV_LeafHeader) + (size - sizeof(CV_LeafKind)).
  U64 checkpoint_cap = data.size / (sizeof(CV_LeafHeader) * LNK_GIANT_DEBUG_T_INTERVAL) + 2;
  g->checkpoints = push_array_no_zero(arena, U32, checkpoint_cap);

  U64 leaf_count = 0;
  if (data.size >= sizeof(CV_LeafHeader)) {
    for (U64 cursor = 0; cursor < data.size; ) {
      CV_LeafHeader header = { .v = memory_read32(data.str + cursor) };
      if (header.size < sizeof(CV_LeafKind))                   { break; }
      if (sizeof(CV_LeafSize) + (U64)header.size > data.size)  { break; }
      U64 stride = AlignPow2(sizeof(CV_LeafHeader) + (U64)(header.size - sizeof(CV_LeafKind)), CV_LeafAlign);
      if (stride > data.size)                                  { break; }
      if ((leaf_count % LNK_GIANT_DEBUG_T_INTERVAL) == 0) {
        Assert(leaf_count / LNK_GIANT_DEBUG_T_INTERVAL < checkpoint_cap);
        g->checkpoints[leaf_count / LNK_GIANT_DEBUG_T_INTERVAL] = (U32)cursor;
      }
      leaf_count += 1;
      cursor     += stride;
    }
  }

  g->leaf_count     = leaf_count;
  g->interval_count = CeilIntegerDiv(leaf_count, LNK_GIANT_DEBUG_T_INTERVAL);
  g->interval_counts = push_array(arena, LNK_GiantSourceCounts, g->interval_count);

  CV_DebugT *out = &task->parse->out_types[g->obj_idx];
  MemoryZeroStruct(out);
  out->data    = data;
  out->count   = leaf_count;
  out->offsets = push_array_no_zero(arena, U32, leaf_count);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_giant_debug_t_interval_task)
{
  ProfBeginFunction();
  LNK_GiantDebugTTask *task      = raw_task;
  LNK_GiantDebugT     *g         = &task->giants[task->interval_giant[task_id]];
  U64                  local_idx = task->interval_local[task_id];
  String8              data      = g->data;
  CV_DebugT           *out       = &task->parse->out_types[g->obj_idx];

  U64  leaf_lo = local_idx * LNK_GIANT_DEBUG_T_INTERVAL;
  U64  leaf_hi = Min(leaf_lo + LNK_GIANT_DEBUG_T_INTERVAL, g->leaf_count);
  U64  cursor  = g->checkpoints[local_idx];
  U64 *counts  = g->interval_counts[local_idx];

  for (U64 leaf_idx = leaf_lo; leaf_idx < leaf_hi; leaf_idx += 1) {
    CV_LeafHeader header = { .v = memory_read32(data.str + cursor) };
    out->offsets[leaf_idx] = (U32)cursor;
    counts[cv_type_index_source_from_leaf_kind(header.kind)] += 1;
    cursor += AlignPow2(sizeof(CV_LeafHeader) + (U64)(header.size - sizeof(CV_LeafKind)), CV_LeafAlign);
  }
  ProfEnd();
}

internal void
lnk_parse_giant_debug_t(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 obj_count, LNK_ParseCvTypes *parse)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  // gather giants
  U64              giant_count = 0;
  LNK_GiantDebugT *giants      = push_array(scratch.arena, LNK_GiantDebugT, obj_count);
  for EachIndex(obj_idx, obj_count) {
    if (parse->out_types[obj_idx].offsets == 0 && !parse->out_types[obj_idx].sidecar_packed &&
        parse->raw_types[obj_idx].count > 0 && parse->raw_types[obj_idx].v[0].size >= LNK_GIANT_DEBUG_T_SIZE) {
      giants[giant_count].obj_idx = obj_idx;
      giants[giant_count].data    = parse->raw_types[obj_idx].v[0];
      giant_count += 1;
    }
  }

  if (giant_count > 0) {
    LNK_GiantDebugTTask task = { .parse = parse, .giants = giants };

    // phase 1: serial chain hop per giant (capped: the hop is the fault-bound
    // dependent-load chase, extra lanes only spin)
    lnk_tp_for_parallel_capped_prof(tp, tp_arena, config->debug_worker_cap, giant_count, lnk_giant_debug_t_hop_task, &task, "Giant .debug$T Hop");

    // flatten intervals
    U64 interval_total = 0;
    for EachIndex(i, giant_count) { interval_total += giants[i].interval_count; }
    task.interval_giant = push_array_no_zero(scratch.arena, U32, interval_total);
    task.interval_local = push_array_no_zero(scratch.arena, U32, interval_total);
    for (U64 i = 0, flat = 0; i < giant_count; i += 1) {
      for EachIndex(k, giants[i].interval_count) {
        task.interval_giant[flat] = (U32)i;
        task.interval_local[flat] = (U32)k;
        flat += 1;
      }
    }

    // phase 2: offsets + kind classification from true checkpoints; pages are
    // hot from the hop, so this is CPU-bound -- full pool width
    tp_for_parallel_prof(tp, 0, interval_total, lnk_giant_debug_t_interval_task, &task, "Giant .debug$T Intervals");

    // phase 3: deterministic interval-order reduce + the serial-parse tail
    for EachIndex(i, giant_count) {
      LNK_GiantDebugT *g   = &giants[i];
      CV_DebugT       *out = &parse->out_types[g->obj_idx];
      for EachIndex(k, g->interval_count) {
        for EachElement(s, out->source_counts) { out->source_counts[s] += g->interval_counts[k][s]; }
      }
#if BUILD_DEBUG
      { U64 total = 0; for EachElement(s, out->source_counts) { total += out->source_counts[s]; } Assert(total == g->leaf_count); }
#endif
      for EachElement(s, out->ti_ranges) { out->ti_ranges[s] = r1u64(CV_MinComplexTypeIndex, CV_MinComplexTypeIndex + out->count); }
      CV_Leaf leaf = cv_debug_t_get_leaf(out, 0);
      if (leaf.kind == CV_LeafKind_PRECOMP) {
        CV_PrecompInfo precomp_info = cv_precomp_info_from_leaf(leaf);
        for EachElement(s, out->ti_ranges) { out->ti_ranges[s].max += precomp_info.leaf_count; }
      }
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_read_type_servers_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  LNK_CodeViewInput *task = raw_task;

  B32             discard_debug_info = 1;
  U64             ts_idx             = task_id;
  LNK_TypeServer *ts                 = &task->ts_arr.v[ts_idx];

  String8 type_data_raw                             = {0};
  String8 source_data_arr[CV_TypeIndexSource_COUNT] = {0};
  Rng1U64 ti_ranges      [CV_TypeIndexSource_COUNT] = {0};

  switch (ts->ts_kind) {
  case LNK_TypeServerKind_Null: break;
  case LNK_TypeServerKind_RRT: {
    type_data_raw = ts->rrt->type_data_raw;
    MemoryCopyArray(source_data_arr, ts->rrt->type_data);
    MemoryCopyArray(ti_ranges, ts->rrt->ti_ranges);
  } break;
  case LNK_TypeServerKind_PDB: {
    // read PDB from disk
    B8      msf_data_read = 0;
    String8 msf_data      = lnk_read_data_from_file_path(scratch.arena, task->config->io_flags, ts->ts_path, &msf_data_read);

    if ( ! msf_data_read) {
      LNK_Obj *dep_obj = 0;
      if (ts->obj_indices.count) {
        dep_obj = task->obj_arr[ts->obj_indices.first->data];
      }
      lnk_error_obj(LNK_Error_UnableToOpenTypeServer, dep_obj, "failed to read type server from path: %S", ts->ts_path);
      goto exit;
    }

    // check magic
    if (!msf_check_magic_70(msf_data) && msf_check_magic_20(msf_data)) { goto exit; }

    // read the stream table
    MSF_RawStreamTable *st = msf_raw_stream_table_from_data(scratch.arena, msf_data);
    if (st == 0) { goto exit; }

    // PDB must have these streams
    if (PDB_FixedStream_Tpi >= st->stream_count || PDB_FixedStream_Ipi >= st->stream_count || PDB_FixedStream_Info >= st->stream_count) { goto exit; }

    // read info stream
    String8       info_data  = msf_data_from_stream_number(scratch.arena, msf_data, st, PDB_FixedStream_Info);
    PDB_InfoParse info_parse = {0};
    pdb_info_parse_from_data(info_data, &info_parse);

    // match GUID from obj against one in the type server
    if (!MemoryMatchStruct(&info_parse.guid, &ts->ts_info.sig)) {
      lnk_error(LNK_Warning_MismatchedTypeServerSignature,
                "%S: signature mismatch in type server read from disk, expected %S, got %S",
                ts->ts_info.name,
                string_from_guid(scratch.arena, ts->ts_info.sig),
                string_from_guid(scratch.arena, info_parse.guid));
      goto exit;
    }

    MSF_StreamNumber type_streams[CV_TypeIndexSource_COUNT] = {0};
    type_streams[CV_TypeIndexSource_TPI] = PDB_FixedStream_Tpi;
    type_streams[CV_TypeIndexSource_IPI] = PDB_FixedStream_Ipi;

    Rng1U64 leaf_ranges[CV_TypeIndexSource_COUNT] = {0};
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      MSF_StreamNumber sn = type_streams[ti_source];
      if (sn == 0) { continue; }
      if (!pdb_extract_type_server_info(msf_data, st, sn, &ti_ranges[ti_source], &leaf_ranges[ti_source])) { goto exit; }
    }

    // alloc buffer where TPI and IPI are adjacent
    U64 buffer_size = 0;
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { buffer_size += dim_1u64(leaf_ranges[ti_source]); }
    buffer_size = AlignPow2(buffer_size, 4) + ARENA_HEADER_SIZE;
    U8    *buffer = push_array(arena, U8, buffer_size);
    Arena *fixed  = arena_alloc( .reserve_size = buffer_size, .commit_size = buffer_size, .optional_backing_buffer = buffer );

    // read both streams into a contiguous buffer
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      MSF_StreamNumber sn = type_streams[ti_source];
      if (sn == 0) { continue; }
      source_data_arr[ti_source] = msf_data_from_stream_number_ex(fixed, msf_data, st, sn, leaf_ranges[ti_source], PDB_LEAF_ALIGN);
      Assert(source_data_arr[ti_source].size == dim_1u64(leaf_ranges[ti_source]));
    }
    // assert streams are adjacent in the buffer
    for (U64 i = 1; i+1 < CV_TypeIndexSource_COUNT; i += 1) { AssertAlways(source_data_arr[i].str + source_data_arr[i].size == source_data_arr[i+1].str); }
    type_data_raw = str8(buffer + ARENA_HEADER_SIZE, buffer_size - ARENA_HEADER_SIZE);
  } break;
  default: InvalidPath; break;
  }

  // map type server to -> .debug$T
  U64        obj_idx = task->type_server_indices.v[task_id];
  CV_DebugT *debug_t = &task->debug_t_arr[obj_idx];

  // read types
  CV_DebugT d = cv_debug_t_from_data(arena, type_data_raw, PDB_LEAF_ALIGN);

  // @type_server .debugtype_data
  debug_t->count   = d.count;
  debug_t->data    = d.data;
  debug_t->offsets = d.offsets;
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_ranges[i] = ti_ranges[i];                                       }
  for EachIndex(i, CV_TypeIndexSource_COUNT) { debug_t->ti_base[i]   = IntFromPtr(source_data_arr[i].str - type_data_raw.str); }
  MemoryCopyTyped(debug_t->source_counts,  d.source_counts, CV_TypeIndexSource_COUNT);
  MemoryCopyTyped(debug_t->source_offsets, d.source_counts, CV_TypeIndexSource_COUNT);
  u64_array_counts_to_offsets(CV_TypeIndexSource_COUNT, debug_t->source_offsets);

  discard_debug_info = 0;
  exit:;
  if (discard_debug_info) {
    // an error occurred while loading external type server, discard
    // parts debug info in dependent objs that rely on types
    for EachNode(n, U64Node, task->ts_arr.v[ts_idx].obj_indices.first) {
      lnk_discard_cv_debug_info(task, n->data);
    }
  }

  task->is_type_server_discarded[task_id] = discard_debug_info;

  ProfEnd();
  scratch_end(scratch);
}

internal String8List
lnk_string_list_from_rrt(Arena *arena, LNK_RRT *rrt)
{
  ProfBeginFunction();

  // pack obj file paths
  String8 obj_paths = {0};
  {
    U64 total_obj_file_path_size = 0;
    for EachIndex(i, rrt->obj_count) {
      total_obj_file_path_size += rrt->obj_paths.v[i].size + 1;
    }

    U8 *file_paths_buffer = push_array_no_zero(arena, U8, total_obj_file_path_size);
    U64 file_paths_cursor = 0;
    for EachIndex(i, rrt->obj_count) {
      MemoryCopyStr8(file_paths_buffer + file_paths_cursor, rrt->obj_paths.v[i]);
      file_paths_cursor += rrt->obj_paths.v[i].size;

      file_paths_buffer[file_paths_cursor] = 0;
      file_paths_cursor += 1;
    }
    Assert(file_paths_cursor == total_obj_file_path_size);

    obj_paths = str8(file_paths_buffer, file_paths_cursor);
  }

  String8List rrt_data = {0};

  // (1) magic
  str8_list_push(arena, &rrt_data, g_rrt_magic);

  // (2) version
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, g_rrt_version)));

  // (3) debug types hash
  str8_list_push(arena, &rrt_data, str8_struct(&rrt->debug_types_hash));

  // (4) type data ranges
  str8_list_push(arena, &rrt_data, str8_array_fixed(rrt->type_data_ranges));

  // (5) type data
  str8_list_push(arena, &rrt_data, rrt->type_data_raw);

  // (6) type index ranges
  str8_list_push(arena, &rrt_data, str8_array_fixed(rrt->ti_ranges));

  // (7) type hashes size
  U64 total_hash_count = 0;
  for EachIndex(i, CV_TypeIndexSource_COUNT) { total_hash_count += dim_1u64(rrt->ti_ranges[i]); }
  U64 type_hashes_size = sizeof(**rrt->type_hashes_unpacked) * total_hash_count;
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, type_hashes_size)));

  // (8) type hashes
  for EachIndex(i, CV_TypeIndexSource_COUNT) {
    U64 type_count = dim_1u64(rrt->ti_ranges[i]);
    str8_list_push(arena, &rrt_data, str8_array(rrt->type_hashes_unpacked[i], type_count));
  }

  // (9) object count
  str8_list_push(arena, &rrt_data, str8_struct(&rrt->obj_count));

  // (10) per object type index ranges
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_ti_ranges, rrt->obj_count));

  // (11) per object time stamps
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_time_stamps, rrt->obj_count));

  // (12) per object leaf counts
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_leaf_counts, rrt->obj_count));

  // (13) per object file reverse lookup table for type indices
  for EachIndex(obj_idx, rrt->obj_count) {
    CV_TypeIndex *obj_ti_map   = rrt->obj_ti_maps[obj_idx];
    U64           obj_ti_count = rrt->obj_leaf_counts[obj_idx];
    Assert(dim_1u64(rrt->obj_pch_ti_ranges[obj_idx]) + obj_ti_count <= dim_1u64(rrt->obj_ti_ranges[obj_idx]));
    str8_list_push(arena, &rrt_data, str8_array(obj_ti_map, obj_ti_count));
  }

  // (14) object file paths size
  str8_list_push(arena, &rrt_data, str8_struct(push_u64(arena, obj_paths.size)));

  // (15) object file paths block
  str8_list_push(arena, &rrt_data, obj_paths);

  // (16) PCH type index ranges
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_pch_ti_ranges, rrt->obj_count));

  // (17) PCH object indices
  str8_list_push(arena, &rrt_data, str8_array(rrt->obj_pch_indices, rrt->obj_count));

  ProfEnd();
  return rrt_data;
}

internal B32
lnk_rrt_from_string(Arena *arena, String8 rrt_data, String8 path, LNK_RRT *rrt_out)
{
  B32 is_ok = 0;
  U64 cursor = 0;

  // (1) magic
  if (rrt_data.size < g_rrt_magic.size ||
      ! str8_match(str8_prefix(rrt_data, g_rrt_magic.size), g_rrt_magic, 0)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file has invalid magic value", path);
    goto exit;
  }
  cursor += g_rrt_magic.size;

  // (2) version
  U64 version = 0;
  U64 version_size = str8_deserial_read_struct(rrt_data, cursor, &version);
  if (version_size != sizeof(version)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain enough bytes to read the version", path);
    goto exit;
  }
  cursor += version_size;

  // match version
  if (version != g_rrt_version) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT version mismatch, got %llu, expected 2 or %llu", path, version, g_rrt_version);
    goto exit;
  }

  // (3) debug types hash
  LNK_HashKind debug_types_hash = LNK_HashKind_BLAKE3;
  if (version == g_rrt_version) {
    U64 debug_types_hash_size = str8_deserial_read_struct(rrt_data, cursor, &debug_types_hash);
    if (debug_types_hash_size != sizeof(debug_types_hash)) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain enough bytes to read the debug types hash", path);
      goto exit;
    }
    cursor += debug_types_hash_size;

    if (debug_types_hash != LNK_HashKind_BLAKE3 && debug_types_hash != LNK_HashKind_XXHash) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file has invalid debug types hash %u", path, debug_types_hash);
      goto exit;
    }
  }

  // (4) type data ranges
  Rng1U64 type_data_ranges[CV_TypeIndexSource_COUNT] = {0};
  U64 type_data_ranges_size = str8_deserial_read_array(rrt_data, cursor, type_data_ranges, ArrayCount(type_data_ranges));
  if (type_data_ranges_size != sizeof(type_data_ranges)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT is missing type data ranges", path);
    goto exit;
  }
  cursor += type_data_ranges_size;

  // comppute size types from all the sources
  U64 total_type_data_size = 0;
  for EachElement(i, type_data_ranges) total_type_data_size += dim_1u64(type_data_ranges[i]);

  // (5) type data
  String8 type_data_raw      = {0};
  U64     type_data_raw_size = str8_deserial_read_block(rrt_data, cursor, total_type_data_size, &type_data_raw);
  if (type_data_raw_size != total_type_data_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type data file (%M)", path, total_type_data_size);
    goto exit;
  }
  cursor += type_data_raw_size;

  // (6) type index ranges
  Rng1U64 ti_ranges[CV_TypeIndexSource_COUNT] = {0};
  U64 ti_ranges_size = str8_deserial_read_array(rrt_data, cursor, ti_ranges, ArrayCount(ti_ranges));
  if (ti_ranges_size != sizeof(ti_ranges)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file does not contain type indices", path);
    goto exit;
  }
  cursor += ti_ranges_size;

  // (7) type hashes size
  U64 type_hashes_size      = 0;
  U64 type_hashes_size_size = str8_deserial_read_struct(rrt_data, cursor, &type_hashes_size);
  if (type_hashes_size_size != sizeof(type_hashes_size)) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type hashes", path);
    goto exit;
  }
  cursor += type_hashes_size_size;

  // validate type hashes size
  if (type_hashes_size) {
    U64 type_count = 0;
    for EachIndex(i, CV_TypeIndexSource_COUNT) { type_count += dim_1u64(ti_ranges[i]); }
    U64 expected_size = type_count * sizeof(U64);
    if (expected_size != type_hashes_size) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file type hash size (%llu) does not match expected size (%llu)", path, type_hashes_size, expected_size);
      goto exit;
    }
  }

  // (8) type hashes
  String8 type_hashes = {0};
  U64 type_hashes_read_size = str8_deserial_read_block(rrt_data, cursor, type_hashes_size, &type_hashes);
  if (type_hashes_read_size != type_hashes_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read type hashes (%M)", path, type_hashes_size);
    goto exit;
  }
  cursor += type_hashes_read_size;

  // hash count must match type count
  if (type_hashes_size) {
    U64 type_count = 0;
    for EachIndex(i, CV_TypeIndexSource_COUNT) { type_count += dim_1u64(ti_ranges[i]); }
    U64 hash_count = type_hashes.size / sizeof(U64);
    if (type_count != hash_count) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file hash count (%llu) does not match type count (%llu)", path, hash_count, type_count);
      goto exit;
    }
  }

  // (9) object count
  U64 obj_count = 0;
  U64 obj_count_size = str8_deserial_read_struct(rrt_data, cursor, &obj_count);
  if (obj_count_size == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object count", path);
    goto exit;
  }
  cursor += obj_count_size;

  // (10) per object type index ranges
  Rng1U64 *obj_ti_ranges = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_ti_ranges) * obj_count); 
  if (obj_ti_ranges == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object type index ranges", path);
    goto exit;
  }
  cursor += sizeof(*obj_ti_ranges) * obj_count;

  // (11) last observed time stamp of the object files
  U64 *obj_time_stamps = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_time_stamps) * obj_count);
  if (obj_time_stamps == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object timestamps", path);
    goto exit;
  }
  cursor += sizeof(*obj_time_stamps) * obj_count;

  // (12) per object leaf counts
  U64 *obj_leaf_counts = str8_deserial_get_raw_ptr(rrt_data, cursor, sizeof(*obj_leaf_counts) * obj_count);
  if (obj_leaf_counts == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is missing the object leaf counts", path);
    goto exit;
  }
  cursor += sizeof(*obj_leaf_counts) * obj_count;

  // (13) per object file reverse lookup table for type indices
  CV_TypeIndex **obj_ti_maps = push_array(arena, CV_TypeIndex *, obj_count);
  for EachIndex(obj_idx, obj_count) {
    U64 obj_ti_count = obj_leaf_counts[obj_idx];
    obj_ti_maps[obj_idx] = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_ti_count * sizeof(*obj_ti_maps[obj_idx]));
    if (obj_ti_maps[obj_idx] == 0) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: failed to read objects type index map from RRT file", path);
      goto exit;
    }
    cursor += obj_ti_count * sizeof(*obj_ti_maps[obj_idx]);
  }

  // (14) object file paths size
  U64 obj_file_paths_size = 0;
  U64 obj_file_paths_size_size = str8_deserial_read_struct(rrt_data, cursor, &obj_file_paths_size);
  if (obj_file_paths_size_size == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object file path block size", path);
    goto exit;
  }
  cursor += obj_file_paths_size_size;

  // (15) object file paths block
  String8 obj_file_paths_block = {0};
  U64 obj_file_paths_block_size = str8_deserial_read_block(rrt_data, cursor, obj_file_paths_size, &obj_file_paths_block);
  if (obj_file_paths_block_size != obj_file_paths_size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read the object file path block (%M)", path, obj_file_paths_size);
    goto exit;
  }
  cursor += obj_file_paths_block_size;

  // (16) PCH type index ranges
  Rng1U64 *obj_pch_ti_ranges = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_count * sizeof(*obj_pch_ti_ranges));
  if (obj_pch_ti_ranges == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read object PCH type index ranges");
    goto exit;
  }
  cursor += obj_count * sizeof(*obj_pch_ti_ranges);

  U32 *obj_pch_indices = str8_deserial_get_raw_ptr(rrt_data, cursor, obj_count * sizeof(*obj_pch_indices));
  if (obj_pch_indices == 0) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: RRT file is too small to read object PCH indices");
    goto exit;
  }
  cursor += obj_count * sizeof(*obj_pch_indices);

  for EachIndex(obj_idx, obj_count) {
    if (dim_1u64(obj_pch_ti_ranges[obj_idx]) + obj_leaf_counts[obj_idx] > dim_1u64(obj_ti_ranges[obj_idx])) {
      lnk_error(LNK_Error_IllData, "ERROR: %S: RRT object leaf count exceeds object type index range", path);
      goto exit;
    }
  }

  if (cursor != rrt_data.size) {
    lnk_error(LNK_Error_IllData, "ERROR: %S: failed to parse RRT file", path);
    goto exit;
  }

  // unpack type data
  type_data_raw = push_str8_copy(arena, type_data_raw);
  String8 type_data[CV_TypeIndexSource_COUNT] = {0};
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    type_data[ti_source] = str8_substr(type_data_raw, type_data_ranges[ti_source]);
  }

  // unpack obj file paths
  Temp scratch = scratch_begin(&arena, 1);
  String8List obj_file_paths_list = str8_split_by_string_chars(scratch.arena, obj_file_paths_block, str8_lit("\0"), 0);
  String8Array obj_paths = str8_array_from_list(arena, &obj_file_paths_list);
  scratch_end(scratch);

  // fill out result
  if (rrt_out) {
    rrt_out->path                = path;
    rrt_out->debug_types_hash    = debug_types_hash;
    rrt_out->type_data_raw       = type_data_raw;
    rrt_out->type_hashes         = type_hashes;
    MemoryCopyArray(rrt_out->type_data_ranges, type_data_ranges);
    MemoryCopyArray(rrt_out->type_data,        type_data);
    MemoryCopyArray(rrt_out->ti_ranges,        ti_ranges);
    rrt_out->obj_count           = obj_count;
    rrt_out->obj_leaf_counts     = obj_leaf_counts;
    rrt_out->obj_time_stamps     = obj_time_stamps;
    rrt_out->obj_ti_ranges       = obj_ti_ranges;
    rrt_out->obj_ti_maps         = obj_ti_maps;
    rrt_out->obj_ti_ranges       = obj_ti_ranges;
    rrt_out->obj_ti_maps         = obj_ti_maps;
    rrt_out->obj_paths           = obj_paths;
    rrt_out->obj_pch_ti_ranges   = obj_pch_ti_ranges;
    rrt_out->obj_pch_indices     = obj_pch_indices;
  }

  is_ok = 1;
  exit:;
  return is_ok;
}

internal LNK_RRT_Array
lnk_rrt_array_from_config(Arena *arena, LNK_Config *config)
{
  ProfBegin("Parse RRT");
  LNK_RRT_Array rrt_arr = { .v = push_array(arena, LNK_RRT, config->input_list[LNK_Input_RRT].node_count) };
  for EachNode(n, String8Node, config->input_list[LNK_Input_RRT].first) {
    B8      was_rrt_read = 0;
    String8 rrt_path     = n->string;
    String8 raw_rrt      = lnk_read_data_from_file_path(arena, config->io_flags, rrt_path, &was_rrt_read);
    if (raw_rrt.size == 0 || was_rrt_read == 0) {
      lnk_error(LNK_Error_IllData, "ERROR: failed to open \"%S\"", rrt_path);
      continue;
    }
    LNK_RRT rrt = {0};
    lnk_rrt_from_string(arena, raw_rrt, n->string, &rrt);
    rrt_arr.v[rrt_arr.count++] = rrt;
  }
  ProfEnd();
  return rrt_arr;
}

////////////////////////////////
// IFC header-unit debug-record resolution

typedef struct LNK_IfcMapEntry
{
  String8 ifc_path;  // absolute .ifc path
  U64     blob_slot; // resolved blob slot + 1; 0 = not yet resolved (filled by the serial
                     // discovery replay in lnk_apply_ifc_debug_records, memoizes path lookups)
} LNK_IfcMapEntry;

// Parse the trivial /ifcMap TOML by hand:
//   [[header-unit]]
//   name = ["quote", '<header-unit-path>']
//   ifc  = "<abs .ifc path>"
// Registers basename(header-unit-path) -> .ifc path in `hm` (hash_map of path->raw LNK_IfcMapEntry*).
internal void
lnk_parse_ifc_map_toml(Arena *arena, HashMap *hm, String8 toml_data)
{
  U64 cursor = 0;
  String8 cur_name = {0};
  while (cursor < toml_data.size) {
    // read a line
    U64 line_end = cursor;
    while (line_end < toml_data.size && toml_data.str[line_end] != '\n') { line_end += 1; }
    String8 line = str8_skip_chop_whitespace(str8_substr(toml_data, r1u64(cursor, line_end)));
    cursor = line_end + 1;

    if (line.size == 0 || line.str[0] == '#') { continue; }

    if (str8_match(str8_prefix(line, 4), str8_lit("name"), 0)) {
      // name = ["quote", '<path>']  -- extract the last single-quoted token
      U64 q0 = str8_find_needle(line, 0, str8_lit("'"), 0);
      if (q0 < line.size) {
        U64 q1 = str8_find_needle(line, q0 + 1, str8_lit("'"), 0);
        if (q1 < line.size) {
          cur_name = str8_substr(line, r1u64(q0 + 1, q1));
        }
      }
    } else if (str8_match(str8_prefix(line, 3), str8_lit("ifc"), 0)) {
      U64 q0 = str8_find_needle(line, 0, str8_lit("\""), 0);
      if (q0 < line.size && cur_name.size) {
        U64 q1 = str8_find_needle(line, q0 + 1, str8_lit("\""), 0);
        if (q1 < line.size) {
          String8 ifc_path = str8_substr(line, r1u64(q0 + 1, q1));
          // key by basename of the header-unit path (matches LF_IFC_RECORD header_unit_path basename)
          String8 base = str8_skip_last_slash(cur_name);
          // header-unit paths use backslashes; normalize to last path component
          U64 bs = str8_find_needle_reverse(base, 0, str8_lit("\\"), 0);
          if (bs) { base = str8_skip(base, bs); }
          LNK_IfcMapEntry *e = push_array(arena, LNK_IfcMapEntry, 1);
          e->ifc_path = push_str8_copy(arena, ifc_path);
          hash_map_push_string_raw(arena, hm, push_str8_copy(arena, base), e);
        }
      }
      cur_name = str8_zero();
    }
  }
}

// Reads every /ifcMap toml, materializes the union of header-unit basename -> .ifc path.
internal HashMap
lnk_build_ifc_map(Arena *arena, LNK_Config *config)
{
  HashMap hm = {0};
  Temp scratch = scratch_begin(&arena, 1);
  for EachNode(n, String8Node, config->ifc_map_list.first) {
    B8      was_read = 0;
    String8 toml     = lnk_read_data_from_file_path(scratch.arena, 0, n->string, &was_read);
    if ( ! was_read || toml.size == 0) {
      lnk_error(LNK_Error_Cmdl, "/ifcMap: unable to read TOML '%S'", n->string);
      continue;
    }
    lnk_parse_ifc_map_toml(arena, &hm, toml);
  }
  scratch_end(scratch);
  return hm;
}

// LF_IFC_RECORD (0x1522) body layout (header {len,kind} already stripped from leaf.data):
//   u16 version (==2); u32 ifc_type_index X; u8[16] guid; u8[16] hash; char[] header_unit_path NUL
typedef struct LNK_IfcRecord
{
  U32     ifc_type_index;  // X: TI into the .ifc debug-records blob (base 0x1000)
  U8      guid[16];
  U8      hash[16];
  String8 header_unit_path;
  B32     is_valid;
} LNK_IfcRecord;

internal LNK_IfcRecord
lnk_parse_ifc_record(String8 leaf_data)
{
  LNK_IfcRecord rec = {0};
  if (leaf_data.size < 2 + 4 + 16 + 16) { return rec; }
  U64 off = 0;
  U16 version; off += str8_deserial_read_struct(leaf_data, off, &version);
  off += str8_deserial_read_struct(leaf_data, off, &rec.ifc_type_index);
  MemoryCopy(rec.guid, leaf_data.str + off, 16); off += 16;
  MemoryCopy(rec.hash, leaf_data.str + off, 16); off += 16;
  rec.header_unit_path = str8_cstring_capped(leaf_data.str + off, leaf_data.str + leaf_data.size);
  rec.is_valid = 1;
  return rec;
}

// Per-blob closure + NOTYPE-prune (third pass of lnk_apply_ifc_debug_records). Each blob is fully
// independent: it reads/writes only its own ref_bits[blob_i] and its own blob DebugT leaves, so the
// work parallelizes across the ~13 blobs with no shared state. The worklist scratch comes from the
// per-worker arena. Closure counts are written per-blob and summed afterward (order-independent).
// Open-addressing U64 hash set keyed by unique_name hash. Used to record which UDT unique_names
// already have a COMPLETE definition in a non-blob (consuming) obj, so the blob prune can keep a
// blob's complete definition only for names that NO normal obj completes (blob-only types). cap is
// a power of two; 0-hash is reserved as the empty sentinel (we OR in a bit so a real 0 can't occur).
typedef struct LNK_U64Set { U64 *slots; U64 cap; } LNK_U64Set;

internal U64
lnk_uname_hash(String8 s)
{
  U64 h = 5381;
  for EachIndex(c, s.size) { h = ((h << 5) + h) ^ (U64)s.str[c]; }
  return h | 1; // never 0 (0 is the empty sentinel)
}

internal void
lnk_u64set_add(LNK_U64Set *set, U64 h)
{
  U64 i = h & (set->cap - 1);
  for (;;) {
    if (set->slots[i] == 0)  { set->slots[i] = h; return; }
    if (set->slots[i] == h)  { return; }
    i = (i + 1) & (set->cap - 1);
  }
}

// Thread-safe insert mirroring lnk_icf_map_put_atomic (lnk.c). The empty sentinel is 0 (not
// LNK_ICF_EMPTY); h is guaranteed nonzero by lnk_uname_hash (`| 1`) so a real key can never be 0.
// Claim an empty slot with an atomic CAS 0->h; the CAS winner owns it. On a lost race re-read the
// same slot (it may now hold our h via a duplicate, or another key) before advancing. Duplicate
// keys across objs are legal and idempotent (a slot already holding h returns), so any insertion
// order yields the identical final membership -- the set is read-only (lnk_u64set_has) afterward.
internal void
lnk_u64set_add_atomic(LNK_U64Set *set, U64 h)
{
  U64 i = h & (set->cap - 1);
  for (;;) {
    if (set->slots[i] == 0) {
      if (ins_atomic_u64_eval_cond_assign(&set->slots[i], h, 0) == 0) { return; }
      continue;
    }
    if (set->slots[i] == h) { return; }
    i = (i + 1) & (set->cap - 1);
  }
}

internal B32
lnk_u64set_has(LNK_U64Set *set, U64 h)
{
  U64 i = h & (set->cap - 1);
  for (;;) {
    if (set->slots[i] == 0) { return 0; }
    if (set->slots[i] == h) { return 1; }
    i = (i + 1) & (set->cap - 1);
  }
}

// Parallel collect of complete-definition unique_name hashes per non-blob obj. Each obj is scanned
// independently (read-only over its .debug$T leaves) and emits its hashes into a per-obj list; a
// serial pass then adds them to the shared open-addressing set. Moves the ~1.25s serial cv_get_udt_info
// scan off the main thread. Determinism: set membership is order-independent, serial-add reproduces.
typedef struct LNK_IfcCompleteScanTask
{
  LNK_CodeViewInput *input;
  U64              **out_hashes;  // per-obj hash array (allocated by task)
  U64               *out_counts;  // per-obj count
} LNK_IfcCompleteScanTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_complete_scan_task)
{
  LNK_IfcCompleteScanTask *task = raw_task;
  U64        obj_idx = task_id;
  CV_DebugT *dt      = &task->input->debug_t_arr[obj_idx];
  if (dt->sidecar_complete_udt_hashes) {
    task->out_hashes[obj_idx] = dt->sidecar_complete_udt_hashes;
    task->out_counts[obj_idx] = dt->sidecar_complete_udt_hash_count;
    return;
  }
  U64       *hashes  = push_array_no_zero(arena, U64, dt->count ? dt->count : 1);
  U64        n       = 0;
  for EachIndex(leaf_idx, dt->count) {
    CV_Leaf    leaf = cv_debug_t_get_leaf(dt, leaf_idx);
    CV_UDTInfo ui   = cv_get_udt_info(leaf.kind, leaf.data);
    if (!(ui.props & CV_TypeProp_HasUniqueName) || ui.unique_name.size == 0) { continue; }
    if (ui.props & CV_TypeProp_FwdRef) { continue; }
    hashes[n++] = lnk_uname_hash(ui.unique_name);
  }
  task->out_hashes[obj_idx] = hashes;
  task->out_counts[obj_idx] = n;
}

// Parallel merge of the per-obj complete-def hash lists into the shared set. Replaces the serial
// lnk_u64set_add loop (the ~914ms hotspot -- 801ms of it first-touch KiPageFault on a single thread
// faulting a 128MB+ set). lnk_u64set_add_atomic spreads both the random-scatter probes AND the
// page faults across the pool. Determinism: keys may legitimately duplicate across objs, but the
// insert is idempotent and the set is read-only afterward (lnk_u64set_has), so insertion order
// cannot change the final membership -- output is bit-identical to the serial merge.
typedef struct LNK_IfcSetMergeTask
{
  Rng1U64    *ranges;
  U64       **out_hashes;
  U64        *out_counts;
  LNK_U64Set *set;
  U64         nonblob_count;
} LNK_IfcSetMergeTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_set_merge_task)
{
  LNK_IfcSetMergeTask *task = raw_task;
  for EachInRange(obj_idx, task->ranges[task_id]) {
    U64 *h = task->out_hashes[obj_idx];
    U64  n = task->out_counts[obj_idx];
    for EachIndex(t, n) { lnk_u64set_add_atomic(task->set, h[t]); }
  }
}

// Fused discovery+redirect scan (passes 1+2 of lnk_apply_ifc_debug_records): each consuming
// obj's .debug$T is swept ONCE, in parallel, for 0x1522 (LF_IFC_RECORD) leaves. The worker
// parses each record, resolves its header-unit basename against the read-only ifc_map_hm, and
// emits one raw record per 0x1522 leaf in ascending leaf_idx order. Workers write NOTHING
// (no NOTYPE, no discovery, no redirects): every order-sensitive effect -- .ifc first-encounter
// slot assignment, NOTYPE rewrites, redirect hash-map push order, ref_bits seeding -- is
// replayed SERIALLY from these records in ascending obj_idx, then ascending leaf_idx: the exact
// order of the original serial passes, so output is byte-for-byte identical.
typedef struct LNK_IfcRawRec
{
  U64              leaf_idx;       // 0x1522 leaf index inside the consuming obj
  CV_TypeIndex     K;              // consuming obj's local placeholder TI
  U32              ifc_type_index; // X: TI into the .ifc blob (base 0x1000)
  U8               guid[16];
  U8               hash[16];
  LNK_IfcMapEntry *entry;          // basename -> map entry (0: invalid record or no map hit)
  U64              blob_i_plus1;   // resolved blob slot + 1 (serial replay fills; 0 = unresolved)
  U64              blob_leaf_idx;  // resolved leaf inside the blob (serial replay fills)
} LNK_IfcRawRec;

typedef struct LNK_IfcScanTask
{
  LNK_CodeViewInput *input;
  HashMap           *ifc_map_hm; // read-only in workers
  LNK_IfcRawRec    **out_recs;   // per-obj ordered raw records (allocated by task)
  U64               *out_counts; // per-obj record count
} LNK_IfcScanTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_scan_task)
{
  LNK_IfcScanTask   *task    = raw_task;
  U64                obj_idx = task_id;
  LNK_CodeViewInput *input   = task->input;
  CV_DebugT         *debug_t = &input->debug_t_arr[obj_idx];

  // count 0x1522 leaves first to size the per-obj record array
  U64 ifc_leaf_count = 0;
  for EachIndex(leaf_idx, debug_t->count) {
    if (cv_debug_t_get_leaf_kind(debug_t, leaf_idx) == 0x1522) { ifc_leaf_count += 1; }
  }
  if (ifc_leaf_count == 0) { task->out_recs[obj_idx] = 0; task->out_counts[obj_idx] = 0; return; }

  LNK_IfcRawRec *recs = push_array_no_zero(arena, LNK_IfcRawRec, ifc_leaf_count);
  U64            n    = 0;

  for EachIndex(leaf_idx, debug_t->count) {
    if (cv_debug_t_get_leaf_kind(debug_t, leaf_idx) != 0x1522) { continue; }

    CV_Leaf       leaf = cv_debug_t_get_leaf(debug_t, leaf_idx);
    LNK_IfcRecord rec  = lnk_parse_ifc_record(leaf.data);

    LNK_IfcRawRec *r  = &recs[n++];
    r->leaf_idx       = leaf_idx;
    r->K              = cv_ti_from_leaf_idx(debug_t, CV_TypeIndexSource_TPI, leaf_idx);
    r->ifc_type_index = rec.ifc_type_index;
    r->entry          = 0;
    r->blob_i_plus1   = 0;
    r->blob_leaf_idx  = 0;

    if (rec.is_valid) {
      MemoryCopy(r->guid, rec.guid, 16);
      MemoryCopy(r->hash, rec.hash, 16);
      String8 base = str8_skip_last_slash(rec.header_unit_path);
      U64 bs = str8_find_needle_reverse(base, 0, str8_lit("\\"), 0);
      if (bs) { base = str8_skip(base, bs); }
      r->entry = hash_map_search_string_raw(task->ifc_map_hm, base);
    }
  }

  task->out_recs[obj_idx]   = recs;
  task->out_counts[obj_idx] = n;
}

// Parallel per-obj record resolution + placeholder NOTYPE (runs after discovery/read/injection,
// when entry->blob_slot, ifc_files, and the injected blob debug_t entries are all frozen/read-only).
// Each worker fills its own obj's raw records in place (blob_i_plus1/blob_leaf_idx), rewrites its
// own 0x1522 leaves to NOTYPE (per-obj disjoint, constant value -- order-free), and reports the
// resolved count + K range. The serial replay below then only pushes redirects in the original
// (obj_idx, leaf_idx) order, so hash-map push order and all outputs stay bit-identical.
typedef struct LNK_IfcResolveTask
{
  LNK_CodeViewInput *input;
  IFC_File          *ifc_files;
  LNK_IfcRawRec    **recs;       // per-obj raw records from the scan
  U64               *counts;     // per-obj record count
  U64               *res_counts; // out: per-obj resolved record count
  U64               *k_first;    // out: first resolved K (valid when res_counts != 0)
  U64               *k_last;     // out: last resolved K (valid when res_counts != 0)
} LNK_IfcResolveTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_resolve_task)
{
  LNK_IfcResolveTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_CodeViewInput  *input   = task->input;
  LNK_IfcRawRec      *recs    = task->recs[obj_idx];
  U64                 n       = task->counts[obj_idx];
  if (n == 0) { task->res_counts[obj_idx] = 0; return; }

  U64 res_count = 0;
  U64 k_first = 0, k_last = 0;
  for EachIndex(t, n) {
    LNK_IfcRawRec *r = &recs[t];
    // exclude the placeholder leaf from output regardless: rewrite to NOTYPE. P4: journaled
    // (KIND_ONLY -- the old write changed only the kind, size/payload stay) instead of
    // CoW-dirtying the mapped view; capacity is pre-reserved serially (arena == 0 here)
    lnk_notype_journal_push(0, &input->notype_journal[obj_idx], (U32)r->leaf_idx, 1, 0 /* bitmap pre-allocated */);
    if (r->entry == 0 || r->entry->blob_slot == 0) { continue; }
    U64       blob_i = r->entry->blob_slot - 1;
    IFC_File *f      = &task->ifc_files[blob_i];
    B32 hash_ok = MemoryMatch(r->guid, f->content_hash, 16) &&
                  MemoryMatch(r->hash, f->content_hash + 16, 16);
    if (!f->is_valid || !hash_ok) { continue; }
    CV_DebugT *bdt = &input->debug_t_arr[input->ifc_obj_range.min + blob_i];
    U64 blob_leaf_idx = cv_leaf_idx_from_ti(bdt, CV_TypeIndexSource_TPI, r->ifc_type_index);
    if (blob_leaf_idx >= bdt->count) { continue; }
    r->blob_i_plus1  = blob_i + 1;
    r->blob_leaf_idx = blob_leaf_idx;
    if (res_count == 0) { k_first = r->K; }
    k_last     = r->K;
    res_count += 1;
  }
  task->res_counts[obj_idx] = res_count;
  task->k_first[obj_idx]    = k_first;
  task->k_last[obj_idx]     = k_last;
}

// Parallel .ifc read + `.msvc.trait.debug-records` parse into PRE-ASSIGNED slots. Slot order
// (== blob obj order == output order) is fixed by the serial discovery replay before any file
// is read, so going wide here cannot reorder anything. Workers do not call lnk_error: read
// failures are collected per slot and reported serially in slot order afterward (identical
// message order to the old serial read; LNK_Error_Cmdl stops the link either way). Worker-arena
// allocations (file bytes + leaf offsets) are long-lived, same as the parallel .debug$T parse
// (lnk_parse_debug_t_task pattern).
typedef struct LNK_IfcReadTask
{
  String8   *paths;        // per-slot .ifc path
  IFC_File  *ifc_files;    // per-slot output
  CV_DebugT *blob_debug_t; // per-slot output
  String8   *errors;       // per-slot read error (size 0 = ok)
} LNK_IfcReadTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_read_task)
{
  LNK_IfcReadTask *task = raw_task;
  U64              slot = task_id;
  String8          err  = {0};
  IFC_File         f    = ifc_file_read(arena, task->paths[slot], &err);
  task->ifc_files[slot] = f;
  task->errors[slot]    = err;
  if (f.is_valid) {
    // parse the raw CV leaf stream (no signature, TI base 0x1000)
    task->blob_debug_t[slot] = cv_debug_t_from_data(arena, f.debug_records, 1);
  } else {
    MemoryZeroStruct(&task->blob_debug_t[slot]);
  }
}

typedef struct LNK_IfcCloseTask
{
  LNK_CodeViewInput *input;
  U8               **ref_bits;
  U64               *closure_leaves;  // per-blob output: # leaves surviving in closure
  LNK_U64Set        *nonblob_complete; // unique_name hashes completed by some non-blob obj
} LNK_IfcCloseTask;

internal
THREAD_POOL_TASK_FUNC(lnk_ifc_close_blob_task)
{
  LNK_IfcCloseTask  *task    = raw_task;
  U64                blob_i  = task_id;
  LNK_CodeViewInput *input   = task->input;
  U64        blob_obj_idx = input->ifc_obj_range.min + blob_i;
  CV_DebugT *bdt          = &input->debug_t_arr[blob_obj_idx];
  U8        *bits         = task->ref_bits[blob_i];
  U64        closure      = 0;
  if (bdt->count == 0) { task->closure_leaves[blob_i] = 0; return; }

  Temp  wtemp    = temp_begin(arena);

  // Extra closure roots for forward-ref completion: in CodeView a forward-ref UDT is completed by
  // ANY same-unique_name complete definition in the PDB. A consuming obj typically emits only a
  // forward-ref of a header-unit type; the full-merge build incidentally kept the matching complete
  // definition from the .ifc blob, so the debugger could complete it. On-demand would drop that
  // definition (nothing references it by TI), leaving the type incomplete vs full-merge. To preserve
  // fidelity WITHOUT dragging the whole blob, root every blob complete-def UDT whose unique_name has
  // NO complete definition in any non-blob obj (i.e. blob-only types -- trait/delegate marker structs
  // etc.). Common types (FString, FGuid, ...) are completed by normal objs, so their redundant blob
  // copies stay pruned. Members of the kept defs are pulled by the closure walk below.
  for EachIndex(leaf_idx, bdt->count) {
    if (bits[leaf_idx >> 3] & (1u << (leaf_idx & 7))) { continue; } // already a root
    CV_Leaf    leaf = cv_debug_t_get_leaf(bdt, leaf_idx);
    CV_UDTInfo ui   = cv_get_udt_info(leaf.kind, leaf.data);
    if (!(ui.props & CV_TypeProp_HasUniqueName) || ui.unique_name.size == 0) { continue; }
    if (ui.props & CV_TypeProp_FwdRef) { continue; }      // only complete definitions
    U64 h = lnk_uname_hash(ui.unique_name);
    if (lnk_u64set_has(task->nonblob_complete, h)) { continue; } // a normal obj already completes it
    bits[leaf_idx >> 3] |= (U8)(1u << (leaf_idx & 7));
  }

  U64  *worklist = push_array_no_zero(wtemp.arena, U64, bdt->count);
  U64   wl_count = 0;
  for EachIndex(leaf_idx, bdt->count) {
    if (bits[leaf_idx >> 3] & (1u << (leaf_idx & 7))) { worklist[wl_count++] = leaf_idx; }
  }

  while (wl_count) {
    U64     leaf_idx = worklist[--wl_count];
    CV_Leaf leaf     = cv_debug_t_get_leaf(bdt, leaf_idx);
    Temp    itemp    = temp_begin(wtemp.arena);
    CV_TiOffsets ti_offs = cv_leaf_ti_offsets(itemp.arena, leaf.kind, leaf.data);
    for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&ti_offs); ti_idx < ti_count; ti_idx += 1) {
      CV_TiOff ti_info = cv_ti_offset_at(&ti_offs, ti_idx);
      CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(leaf.data, ti_info.offset, sizeof(*ti_ptr));
      if (ti_ptr == 0) { continue; }
      CV_TypeIndex sub_ti = memory_read32(ti_ptr);
      if (sub_ti < bdt->ti_ranges[ti_info.source].min ||
          sub_ti >= bdt->ti_ranges[ti_info.source].max) { continue; }
      U64 sub_leaf_idx = cv_leaf_idx_from_ti(bdt, ti_info.source, sub_ti);
      if (sub_leaf_idx >= bdt->count) { continue; }
      if (bits[sub_leaf_idx >> 3] & (1u << (sub_leaf_idx & 7))) { continue; }
      bits[sub_leaf_idx >> 3] |= (U8)(1u << (sub_leaf_idx & 7));
      worklist[wl_count++] = sub_leaf_idx;
    }
    temp_end(itemp);
  }
  temp_end(wtemp);

  for EachIndex(leaf_idx, bdt->count) {
    if (bits[leaf_idx >> 3] & (1u << (leaf_idx & 7))) { closure += 1; continue; }
    CV_LeafHeader *hdr = cv_debug_t_get_leaf_header(bdt, leaf_idx);
    if (hdr->kind == CV_LeafKind_NOTYPE) { continue; }
    memory_write16(MemberFromPtr(CV_LeafHeader, hdr, kind), (U16)CV_LeafKind_NOTYPE);
    memory_write16(MemberFromPtr(CV_LeafHeader, hdr, size), (U16)sizeof(CV_LeafKind));
  }
  task->closure_leaves[blob_i] = closure;
}

// Injects referenced .ifc debug-records blobs as extra "objs" in `input`, scans every
// consuming obj's .debug$T for LF_IFC_RECORD (0x1522) leaves, registers each placeholder
// local TI -> blob leaf redirect, and rewrites the 0x1522 leaf to NOTYPE so it is excluded
// from the output TPI. Must run after .debug$T is parsed and before min-type-index / symbol
// setup (which iterate input->count).
internal void
lnk_apply_ifc_debug_records(TP_Context *tp, TP_Arena *tp_arena, LNK_CodeViewInput *input, LNK_Config *config)
{
  ProfBeginFunction();
  U64 apply_begin_us = now_time_us();
  Temp scratch = scratch_begin(&tp_arena->v[0], 1);
  Arena *arena = tp_arena->v[0];

  // basename -> .ifc path
  HashMap ifc_map_hm = lnk_build_ifc_map(scratch.arena, config);
  U64 discover_begin_us = now_time_us();

  // --- fused scan (old passes 1+2, parallel): ONE sweep of every consuming obj's .debug$T
  // emits per-obj raw 0x1522 records in ascending leaf_idx order (record parse + basename ->
  // ifc_map_hm entry resolution happen in the workers; nothing is written). Every
  // order-sensitive effect is replayed serially from these records below. ---
  LNK_IfcScanTask scan = {0};
  scan.input      = input;
  scan.ifc_map_hm = &ifc_map_hm;
  scan.out_recs   = push_array(scratch.arena, LNK_IfcRawRec *, input->obj_count);
  scan.out_counts = push_array(scratch.arena, U64,             input->obj_count);
  tp_for_parallel(tp, tp_arena, input->obj_count, lnk_ifc_scan_task, &scan);
  U64 scan_end_us = now_time_us();
  lnk_log(LNK_Log_Timers, "[IFC] parallel scan in %.2f ms", (F64)(scan_end_us - discover_begin_us) / 1000.0);

  // --- serial discovery replay: assign .ifc blob slots in first-encounter order (ascending
  // obj_idx, then ascending leaf_idx -- identical to the old serial pass) WITHOUT reading any
  // file, so the reads can go wide below. De-dup by path; entry->blob_slot memoizes the path
  // lookup. 256-slot cap semantics preserved: on overflow the entry stays unresolved and its
  // records never redirect. ---
  HashMap   ifc_path_to_blobidx = {0}; // path -> (blob slot index + 1)
  IFC_File *ifc_files           = push_array(scratch.arena, IFC_File, 256);
  U64       ifc_file_count      = 0;
  CV_DebugT blob_debug_t[256]   = {0};
  String8   slot_paths[256]     = {0};
  for EachIndex(obj_idx, input->obj_count) {
    LNK_IfcRawRec *recs = scan.out_recs[obj_idx];
    U64            n    = scan.out_counts[obj_idx];
    for EachIndex(t, n) {
      LNK_IfcMapEntry *e = recs[t].entry;
      if (e == 0 || e->blob_slot) { continue; }
      U64 *slot = hash_map_search_string_u64(&ifc_path_to_blobidx, e->ifc_path);
      if (slot == 0) {
        if (ifc_file_count >= 256) { continue; }
        hash_map_push_string_u64(scratch.arena, &ifc_path_to_blobidx, e->ifc_path, ifc_file_count + 1);
        slot_paths[ifc_file_count] = e->ifc_path;
        ifc_file_count += 1;
        slot = hash_map_search_string_u64(&ifc_path_to_blobidx, e->ifc_path);
      }
      e->blob_slot = *slot;
    }
  }

  if (ifc_file_count == 0) { goto done; }
  U64 discover_end_us = now_time_us();
  lnk_log(LNK_Log_Timers, "[IFC] discover replay in %.2f ms", (F64)(discover_end_us - scan_end_us) / 1000.0);

  // --- parallel .ifc read + debug-records parse into the pre-assigned slots; report read
  // errors serially in slot order (identical message order to the old serial read). ---
  {
    LNK_IfcReadTask read = {0};
    read.paths        = slot_paths;
    read.ifc_files    = ifc_files;
    read.blob_debug_t = blob_debug_t;
    read.errors       = push_array(scratch.arena, String8, ifc_file_count);
    tp_for_parallel(tp, tp_arena, ifc_file_count, lnk_ifc_read_task, &read);
    for EachIndex(i, ifc_file_count) {
      if (!ifc_files[i].is_valid) { lnk_error(LNK_Error_Cmdl, "/ifcDebugRecords: %S", read.errors[i]); }
    }
    lnk_log(LNK_Log_Timers, "[IFC] read+parse %llu blob(s) in %.2f ms", ifc_file_count, (F64)(now_time_us() - discover_end_us) / 1000.0);
  }

  // --- inject blob objs into the parallel arrays (like type servers, but in ifc_obj_range) ---
  U64 prev_count = input->count;
  U64 new_count  = prev_count + ifc_file_count;

  LNK_Obj  **obj_arr2     = push_array(arena, LNK_Obj *, new_count);
  CV_DebugS *debug_s_arr2 = push_array(arena, CV_DebugS, new_count);
  CV_DebugT *debug_t_arr2 = push_array(arena, CV_DebugT, new_count);
  CV_DebugH *debug_h_arr2 = push_array(arena, CV_DebugH, new_count);
  U64       *obj_to_ts2   = push_array(arena, U64,       new_count);

  MemoryCopyTyped(obj_arr2,     input->obj_arr,     prev_count);
  MemoryCopyTyped(debug_s_arr2, input->debug_s_arr, prev_count);
  MemoryCopyTyped(debug_t_arr2, input->debug_t_arr, prev_count);
  MemoryCopyTyped(debug_h_arr2, input->debug_h_arr, prev_count);
  MemoryCopyTyped(obj_to_ts2,   input->obj_to_ts,   prev_count);
  MemorySet(obj_to_ts2 + prev_count, 0xff, ifc_file_count * sizeof(U64)); // blobs are not type servers

  // blob obj indices + index list for hash-deep / dedup
  U32Array ifc_indices = { .v = push_array(arena, U32, ifc_file_count) };
  for EachIndex(i, ifc_file_count) {
    U64 blob_obj_idx = prev_count + i;
    LNK_Obj *blob_obj = push_array(arena, LNK_Obj, 1);
    blob_obj->path = ifc_files[i].path;
    obj_arr2[blob_obj_idx]     = blob_obj;
    debug_t_arr2[blob_obj_idx] = blob_debug_t[i];
    ifc_indices.v[ifc_indices.count++] = (U32)blob_obj_idx;
  }

  input->count       = new_count;
  input->obj_arr     = obj_arr2;
  input->debug_s_arr = debug_s_arr2;
  input->debug_t_arr = debug_t_arr2;
  input->debug_h_arr = debug_h_arr2;
  input->obj_to_ts   = obj_to_ts2;
  input->ifc_obj_range = r1u64(prev_count, new_count);
  input->ifc_indices   = ifc_indices; // hashed + deduped before int objs (see lnk_merge_types)

  // --- on-demand pruning state: per blob, a "referenced" bitset of leaf indices that
  // are reachable from some consuming obj's 0x1522 redirect (the closure roots). Only these
  // + their transitive blob-internal deps get merged; the rest are rewritten to NOTYPE so the
  // hash/dedup pipeline skips ~all of the ~1.5M blob leaves that nothing references. ---
  U8 **ref_bits = push_array(scratch.arena, U8 *, ifc_file_count);
  for EachIndex(i, ifc_file_count) {
    U64 c = blob_debug_t[i].count;
    ref_bits[i] = push_array(scratch.arena, U8, (c + 7) / 8); // zero-init -> nothing referenced yet
  }

  // --- parallel record resolution + placeholder NOTYPE: per-obj disjoint, order-free (see
  // lnk_ifc_resolve_task). All inputs (entry->blob_slot, ifc_files, blob debug_t) are frozen
  // after the discovery/read/injection steps above. ---
  input->has_ifc_redirects   = 1;
  input->ifc_redirect_bits   = push_array(arena, U64 *,   input->count);
  input->ifc_redirect_ti_rng = push_array(arena, Rng1U64, input->count);
  U64 redirect_count  = 0;
  U64 resolve_begin_us = now_time_us();
  LNK_IfcResolveTask resolve = {0};
  resolve.input      = input;
  resolve.ifc_files  = ifc_files;
  resolve.recs       = scan.out_recs;
  resolve.counts     = scan.out_counts;
  resolve.res_counts = push_array(scratch.arena, U64, input->obj_count);
  resolve.k_first    = push_array(scratch.arena, U64, input->obj_count);
  resolve.k_last     = push_array(scratch.arena, U64, input->obj_count);
  // P4: pre-reserve NOTYPE journal capacity + bitmap serially (one entry per 0x1522 record);
  // the parallel resolve task pushes with arena == 0 and must never allocate
  for EachIndex(obj_idx, input->obj_count) {
    U64 n = scan.out_counts[obj_idx];
    if (n == 0) { continue; }
    LNK_NotypeJournal *journal = &input->notype_journal[obj_idx];
    U32 need = journal->count + (U32)n;
    if (need > journal->cap) {
      U32 *new_v = push_array_no_zero(arena, U32, need);
      MemoryCopyTyped(new_v, journal->v, journal->count);
      journal->v   = new_v;
      journal->cap = need;
    }
    if (journal->bitmap == 0) {
      CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
      journal->bit_cap = debug_t->count;
      journal->bitmap  = push_array(arena, U64, (journal->bit_cap + 63) / 64);
    }
  }
  tp_for_parallel(tp, 0, input->obj_count, lnk_ifc_resolve_task, &resolve);
  lnk_log(LNK_Log_Timers, "[IFC] parallel resolve in %.2f ms", (F64)(now_time_us() - resolve_begin_us) / 1000.0);

  // --- serial redirect replay: push redirects in ascending obj_idx, then ascending leaf_idx --
  // the exact original serial order -- so the redirect hash-map push order, ref_bits seeding,
  // and redirect_count are bit-for-bit identical to the serial code.
  U64 merge_begin_us = now_time_us();
  for EachIndex(obj_idx, input->obj_count) {
    LNK_IfcRawRec *recs = scan.out_recs[obj_idx];
    U64            n    = scan.out_counts[obj_idx];
    if (n == 0 || resolve.res_counts[obj_idx] == 0) { continue; }

    // exact key filter range: records are K-ascending (the scan emits leaf_idx ascending and
    // cv_ti_from_leaf_idx is monotonic), so [k_first, k_last] spans all resolved keys.
    Rng1U64 krng = r1u64(resolve.k_first[obj_idx], resolve.k_last[obj_idx] + 1);
    input->ifc_redirect_ti_rng[obj_idx] = krng;
    input->ifc_redirect_bits[obj_idx]   = push_array(arena, U64, (dim_1u64(krng) + 63) / 64);
    for EachIndex(t, n) {
      LNK_IfcRawRec *r = &recs[t];
      if (r->blob_i_plus1 == 0) { continue; }
      U64 blob_i       = r->blob_i_plus1 - 1;
      U64 blob_obj_idx = input->ifc_obj_range.min + blob_i;
      hash_map_push_u64_u64(arena, &input->ifc_redirect_hm,
                            Compose64Bit(obj_idx, r->K),
                            Compose64Bit(blob_obj_idx, r->blob_leaf_idx));
      U64 rel = r->K - krng.min;
      input->ifc_redirect_bits[obj_idx][rel >> 6] |= (1ull << (rel & 63));
      redirect_count += 1;
      // seed closure root: this blob leaf is referenced
      ref_bits[blob_i][r->blob_leaf_idx >> 3] |= (U8)(1u << (r->blob_leaf_idx & 7));
    }
  }

  lnk_log(LNK_Log_Timers, "[IFC] redirect replay in %.2f ms", (F64)(now_time_us() - merge_begin_us) / 1000.0);

  // --- third pass: per blob, close the referenced set over blob-internal sub-TIs, then
  // NOTYPE every leaf not in the closure. cv_leaf_idx_from_ti on a raw blob is source-agnostic
  // (source_offsets are 0, all ti_ranges == [0x1000, 0x1000+count)) so a sub-TI maps directly to
  // leaf_idx = ti - 0x1000 regardless of its CV_TypeIndexSource label. Walk is iterative (worklist).
  U64 total_blob_leaves = 0, total_closure_leaves = 0;
  U64 closure_begin_us  = now_time_us();
  for EachIndex(blob_i, ifc_file_count) {
    total_blob_leaves += input->debug_t_arr[input->ifc_obj_range.min + blob_i].count;
  }
  if (ifc_file_count) {
    // Build the set of unique_names that already have a COMPLETE definition in some non-blob obj.
    // The blob prune keeps a blob complete-def only when its name is absent here (blob-only type),
    // so forward-refs that no normal obj can complete still get their definition (full-merge fidelity)
    // while redundant blob copies of normally-defined types stay pruned. Size to ~2x the non-blob
    // complete-def count, rounded up to a power of two, for low load factor.
    U64 nonblob_complete_estimate = 0;
    for EachIndex(obj_idx, input->ifc_obj_range.min) {
      nonblob_complete_estimate += input->debug_t_arr[obj_idx].source_counts[CV_TypeIndexSource_TPI];
    }
    LNK_U64Set nonblob_complete = {0};
    nonblob_complete.cap = 1;
    while (nonblob_complete.cap < (nonblob_complete_estimate * 2 + 16)) { nonblob_complete.cap <<= 1; }
    nonblob_complete.slots = push_array(scratch.arena, U64, nonblob_complete.cap);
    // parallel scan: each non-blob obj emits its complete-def hashes; serial merge adds to the set.
    U64 nonblob_count = input->ifc_obj_range.min;
    if (nonblob_count) {
      LNK_IfcCompleteScanTask scan = {0};
      scan.input      = input;
      scan.out_hashes = push_array(scratch.arena, U64 *, nonblob_count);
      scan.out_counts = push_array(scratch.arena, U64,   nonblob_count);
      tp_for_parallel(tp, tp_arena, nonblob_count, lnk_ifc_complete_scan_task, &scan);
      U64 scan_hash_count = 0;
      U64 scan_sidecar_count = 0;
      for EachIndex(obj_idx, nonblob_count) {
        scan_hash_count += scan.out_counts[obj_idx];
        scan_sidecar_count += (input->debug_t_arr[obj_idx].sidecar_complete_udt_hashes != 0);
      }
      lnk_log(LNK_Log_Timers, "[IFC] complete UDT scan: hashes=%llu sidecars=%llu/%llu",
              scan_hash_count, scan_sidecar_count, nonblob_count);
      // parallel atomic-CAS merge (replaces the serial lnk_u64set_add loop): output-identical
      // because set membership is order-independent + idempotent (see lnk_u64set_add_atomic).
      LNK_IfcSetMergeTask merge = {0};
      merge.ranges        = tp_divide_work(scratch.arena, nonblob_count, tp->worker_count);
      merge.out_hashes    = scan.out_hashes;
      merge.out_counts    = scan.out_counts;
      merge.set           = &nonblob_complete;
      merge.nonblob_count = nonblob_count;
      tp_for_parallel(tp, 0, tp->worker_count, lnk_ifc_set_merge_task, &merge);
    }

    LNK_IfcCloseTask close_task = {0};
    close_task.input            = input;
    close_task.ref_bits         = ref_bits;
    close_task.closure_leaves   = push_array(scratch.arena, U64, ifc_file_count);
    close_task.nonblob_complete = &nonblob_complete;
    tp_for_parallel(tp, tp_arena, ifc_file_count, lnk_ifc_close_blob_task, &close_task);
    for EachIndex(blob_i, ifc_file_count) { total_closure_leaves += close_task.closure_leaves[blob_i]; }
  }
  (void)tp;
  lnk_log(LNK_Log_Timers, "[IFC] closure pass in %.2f ms", (F64)(now_time_us() - closure_begin_us) / 1000.0);

  lnk_log(LNK_Log_Debug, "[IFC] injected %llu .ifc blob(s), %llu record redirect(s); on-demand closure %llu / %llu blob leaves (%.1f%%)",
          ifc_file_count, redirect_count, total_closure_leaves, total_blob_leaves,
          total_blob_leaves ? (100.0 * (F64)total_closure_leaves / (F64)total_blob_leaves) : 0.0);

done:
  scratch_end(scratch);
  lnk_log(LNK_Log_Timers, "[IFC] apply total in %.2f ms", (F64)(now_time_us() - apply_begin_us) / 1000.0);
  ProfEnd();
}

////////////////////////////////
// parallel setup tasks for lnk_make_code_view_input

// Loop 3 (PCH/ext/int classification). The expensive predicates -- the read-only rrt_hm lookup
// and cv_debug_t_is_type_server_ref -- run in parallel per obj. Each obj's class tag and PCH-merge
// mutation are fully independent, so this pass is data-parallel. The ordered 3-array compaction
// and the MultipleDebugTAndDebugP warning are then replayed SERIALLY in obj_idx order, so output
// (array contents/order + warning order + discarded set) is byte-identical to the serial loop.
typedef struct LNK_CvClassifyTask
{
  LNK_CodeViewInput *input;
  CV_DebugT         *debug_p_arr;
  HashMap           *rrt_hm; // read-only after build
  LNK_Obj          **obj_arr;
  U8                *class_tag; // 0=debug_p, 1=ext, 2=int
  U8                *warn_multi;
} LNK_CvClassifyTask;

internal
THREAD_POOL_TASK_FUNC(lnk_cv_classify_task)
{
  LNK_CvClassifyTask *t       = raw_task;
  U64                 obj_idx = task_id;
  CV_DebugT          *debug_t = &t->input->debug_t_arr[obj_idx];
  CV_DebugT          *debug_p = &t->debug_p_arr[obj_idx];

  // classify (same predicate order/precedence as the serial loop)
  U8 tag;
  if      (hash_map_search_path_u64(t->rrt_hm, t->obj_arr[obj_idx]->path)) { tag = 1; }
  else if (debug_p->count > 0 && debug_t->count == 0)                      { tag = 0; }
  else if (cv_debug_t_is_type_server_ref(debug_t))                         { tag = 1; }
  else                                                                     { tag = 2; }
  t->class_tag[obj_idx] = tag;

  // per-obj independent debug_t mutation (identical to serial)
  if (debug_t->count == 0 && debug_p->count > 0) {
    *debug_t = *debug_p;
  } else if (debug_t->count && debug_p->count) {
    t->warn_multi[obj_idx] = 1; // defer warning to serial obj-order replay
    MemoryZeroStruct(debug_t);
    MemoryZeroStruct(debug_p);
  }
}

// Loop 4 (Make Symbol Inputs) count pass. cv_sub_section_from_debug_s is a pure read of the
// already-parsed data_list, so caching each obj's Symbols sub-section list in parallel is safe.
typedef struct LNK_CvSymTask
{
  LNK_CodeViewInput *input;
  String8List       *per_obj_syms;
  U64               *counts;   // per-obj node_count (count pass)
  U64               *offsets;  // per-obj symbol_inputs offset (fill pass)
} LNK_CvSymTask;

internal
THREAD_POOL_TASK_FUNC(lnk_cv_sym_count_task)
{
  LNK_CvSymTask *t       = raw_task;
  U64            obj_idx = task_id;
  t->per_obj_syms[obj_idx] = cv_sub_section_from_debug_s(t->input->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
  t->counts[obj_idx]       = t->per_obj_syms[obj_idx].node_count;
}

// Loop 4 fill pass. Each obj writes a disjoint, contiguous range of symbol_inputs starting at its
// prefix-sum offset, in node order -- byte-identical to the serial append (which walked obj_idx
// ascending, each obj's nodes in list order).
internal
THREAD_POOL_TASK_FUNC(lnk_cv_sym_fill_task)
{
  LNK_CvSymTask *t       = raw_task;
  U64            obj_idx = task_id;
  U64            cur     = t->offsets[obj_idx];
  String8List    s       = t->per_obj_syms[obj_idx];
  for EachNode(n, String8Node, s.first) {
    LNK_SymbolInput *in = &t->input->symbol_inputs[cur++];
    in->obj_idx     = obj_idx;
    in->raw_symbols = n->string;
  }
}

internal LNK_CodeViewInput
lnk_make_code_view_input(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 obj_count, LNK_Obj **obj_arr, LNK_RRT_Array rrt_input)
{
  ProfBegin("Extract CodeView");
  Temp scratch = scratch_begin(0,0);

  LNK_CodeViewInput input = { .config = config, .obj_count = obj_count, .count = obj_count, .obj_arr = obj_arr, .rrt_input = rrt_input, .ts_obj_range = r1u64(0,0) };

  // $T streaming (ring P4): per-real-obj NOTYPE journals (see LNK_NotypeJournal). Real objs
  // only ([0, obj_count)); pseudo objs appended later mutate their arena-backed $T in place.
  input.notype_journal = push_array(tp_arena->v[0], LNK_NotypeJournal, obj_count ? obj_count : 1);

  HashMap rrt_hm = {0};
  ProfScope("Make obj path -> RRT hash map")
  {
    for EachIndex(rrt_idx, rrt_input.count) {
      for EachIndex(obj_idx, rrt_input.v[rrt_idx].obj_paths.count) {
        hash_map_push_path_u64(scratch.arena, &rrt_hm, rrt_input.v[rrt_idx].obj_paths.v[obj_idx], Compose64Bit(rrt_idx, obj_idx));
      }
    }
  }

  ProfBegin("Apply RRT to Objs");

  // hash map (obj path, obj idx). Kept SERIAL: HashMap is a 4-ary trie whose insert mutates shared
  // child pointers + arena-allocates nodes -> not safe for concurrent insert. Only built (and
  // consulted) when there is at least one input RRT; the monolithic Engine.dll link has none.
  HashMap obj_path_hm = {0};
  if (rrt_input.count) {
    for EachIndex(obj_idx, obj_count) {
      hash_map_push_path_u64(scratch.arena, &obj_path_hm, obj_arr[obj_idx]->path, obj_idx);
    }

    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj            = obj_arr[obj_idx];
      U64     *packed_rrt_idx = hash_map_search_path_u64(&rrt_hm, obj->path);

      // obj is not part of any input RRT
      if (packed_rrt_idx == 0) { continue; }

      // unpack index
      U32      rrt_idx     = *packed_rrt_idx >> 32;
      U32      rrt_obj_idx = *packed_rrt_idx & max_U32;
      LNK_RRT *rrt         = &rrt_input.v[rrt_idx];

      // obj was recompiled, do not apply RRT indirection
      FileProperties obj_file_props = properties_from_file_path(obj->path);
      if (rrt->obj_time_stamps[rrt_obj_idx] != obj_file_props.modified) { continue; }

    // invalidate debug section pointers
    obj->coff.debug_t_section_number = 0;
    obj->coff.debug_p_section_number = 0;
    obj->coff.debug_h_section_number = 0;

      // apply type index map
      obj->ti_range = rrt->obj_ti_ranges[rrt_obj_idx];
      obj->ti_map   = rrt->obj_ti_maps  [rrt_obj_idx];

      // apply PCH info
      U32 rrt_pch_obj_idx = rrt->obj_pch_indices[rrt_obj_idx];
      if (rrt_pch_obj_idx < rrt->obj_count) {
        String8  rrt_pch_obj_path = rrt->obj_paths.v[rrt_pch_obj_idx];
        U64      pch_obj_idx      = *hash_map_search_path_u64(&obj_path_hm, rrt_pch_obj_path);
        obj->pch_ti_range = rrt->obj_pch_ti_ranges[rrt_obj_idx];
        obj->pch_obj_idx  = pch_obj_idx;
      } else {
        obj->pch_ti_range = r1u64(0,0);
        obj->pch_obj_idx  = ~0;
      }
    }
  }
  ProfEnd();
  
  ProfBegin("Collect CodeView");
  input.debug_s_list_arr = lnk_collect_obj_sections(tp, tp_arena, obj_count, obj_arr, str8_lit(".debug$S"), 0, &input.debug_s_sect_idx_arr);
  ProfEnd();

  // batch-populate the mapped .debug$S/$T/$P/$H input ranges before the parse
  // loops below first-touch them page by page (see lnk_prefetch_ranges)
  if (lnk_should_prefetch_mapped_input()) ProfScope("Prefetch CodeView")
  {
    Temp temp = temp_begin(scratch.arena);

    U64 range_cap = 3 * obj_count; // debug$T + debug$P + debug$H
    for EachIndex(obj_idx, obj_count) { range_cap += input.debug_s_list_arr[obj_idx].node_count; }

    Rng1U64 *ranges      = push_array_no_zero(temp.arena, Rng1U64, range_cap);
    U64      range_count = 0;
    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj = obj_arr[obj_idx];

      for EachNode(n, String8Node, input.debug_s_list_arr[obj_idx].first) {
        if (n->string.size) { ranges[range_count++] = rng_1u64((U64)n->string.str, (U64)n->string.str + n->string.size); }
      }
      if (obj->coff.debug_t_section_number) {
        String8 data = lnk_obj_section_data_from_number(obj, obj->coff.debug_t_section_number);
        if (data.size) { ranges[range_count++] = rng_1u64((U64)data.str, (U64)data.str + data.size); }
      }
      if (obj->coff.debug_p_section_number) {
        String8 data = lnk_obj_section_data_from_number(obj, obj->coff.debug_p_section_number);
        if (data.size) { ranges[range_count++] = rng_1u64((U64)data.str, (U64)data.str + data.size); }
      }
      if (config->ghash && obj->coff.debug_h_section_number) {
        String8 data = lnk_obj_section_data_from_number(obj, obj->coff.debug_h_section_number);
        if (data.size) { ranges[range_count++] = rng_1u64((U64)data.str, (U64)data.str + data.size); }
      }
    }
    Assert(range_count <= range_cap);
    U64 prefetch_begin_us = now_time_us();
    U64 prefetch_bytes    = 0;
    for EachIndex(range_idx, range_count) { prefetch_bytes += dim_1u64(ranges[range_idx]); }
    lnk_prefetch_ranges(tp, config->debug_worker_cap, range_count, ranges);
    lnk_log(LNK_Log_Timers, "[mcvi] prefetched %llu debug section ranges (%llu MiB) in %.2f ms",
            range_count, prefetch_bytes / MB(1), (F64)(now_time_us() - prefetch_begin_us) / 1000.0);

    temp_end(temp);
  }

  // profiler info
  if (lnk_get_log_status(LNK_Log_Debug) || PROFILE_TELEMETRY) {
    U64 total_debug_s_size = 0, total_debug_t_size = 0, total_debug_p_size = 0, total_debug_h_size = 0;
    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj = obj_arr[obj_idx];

      for EachNode(n, String8Node, input.debug_s_list_arr[obj_idx].first) { total_debug_s_size += n->string.size; }

      if (obj->coff.debug_t_section_number > 0) {
        total_debug_t_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_t_section_number)->fsize;
      }
      if (obj->coff.debug_p_section_number > 0) {
        total_debug_p_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_p_section_number)->fsize;
      }
      if (config->ghash) {
        if (obj->coff.debug_h_section_number > 0) {
          total_debug_h_size += lnk_coff_section_header_from_section_number(obj, obj->coff.debug_h_section_number)->fsize;
        }
      }
    }

    U64 total_rrt_type_size = 0;
    U64 total_rrt_hash_size = 0;
    for EachIndex(rrt_idx, rrt_input.count) {
      total_rrt_type_size += rrt_input.v[rrt_idx].type_data_raw.size;
      total_rrt_hash_size += rrt_input.v[rrt_idx].type_hashes.size;
    }
	
    ProfNoteV("Total .debug$S Input Size: %M", total_debug_s_size);
    ProfNoteV("Total .debug$T Input Size: %M", total_debug_t_size);
    ProfNoteV("Total .debug$P Input Size: %M", total_debug_p_size);
    ProfNoteV("Total .debug$H Input Size: %M", total_debug_h_size);
    ProfNoteV("Total RRT-Type Input Size: %M", total_rrt_type_size);
    ProfNoteV("Total RRT-Hash Input Size: %M", total_rrt_hash_size);
	
    if (lnk_get_log_status(LNK_Log_Debug)) {
      lnk_log(LNK_Log_Debug, "[Total .debug$S Input Size %M]", total_debug_s_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$T Input Size %M]", total_debug_t_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$P Input Size %M]", total_debug_p_size);
      lnk_log(LNK_Log_Debug, "[Total .debug$H Input Size %M]", total_debug_h_size);
      lnk_log(LNK_Log_Debug, "[Total RRT-Type Input Size %M]", total_rrt_type_size);
      lnk_log(LNK_Log_Debug, "[Total RRT-Hash Input Size %M]", total_rrt_hash_size);
    }
  }

  ProfBegin("Parse CodeView");
  CV_DebugT *debug_p_arr;
  {
    // parse .debug$S
    input.debug_s_arr = push_array(tp_arena->v[0], CV_DebugS, input.obj_count);
    lnk_compressed_obj_log_phase_stats("before parse $S");
    lnk_tp_for_parallel_capped_prof(tp, tp_arena, config->debug_worker_cap, obj_count, lnk_parse_debug_s_task, &input, "Parse .debug$S");
    lnk_compressed_obj_log_phase_stats("after parse $S");
    {
      U64 bytes[CV_C13SubSectionIdxKind_COUNT] = {0};
      U64 nodes[CV_C13SubSectionIdxKind_COUNT] = {0};
      for EachIndex(obj_idx, obj_count) {
        for EachElement(k, input.debug_s_arr[obj_idx].data_list) {
          bytes[k] += input.debug_s_arr[obj_idx].data_list[k].total_size;
          nodes[k] += input.debug_s_arr[obj_idx].data_list[k].node_count;
        }
      }
      for EachIndex(k, CV_C13SubSectionIdxKind_COUNT) {
        if (bytes[k] || nodes[k]) {
          lnk_log(LNK_Log_Timers, "[debugS parsed] kind=%x bytes=%llu nodes=%llu",
                  cv_c13_sub_section_kind_from_idx(k), bytes[k], nodes[k]);
        }
      }
    }

    // collect .debug$P and .debug$T
    String8Array *raw_debug_p_arr = push_array(scratch.arena, String8Array, obj_count);
    String8Array *raw_debug_t_arr = push_array(scratch.arena, String8Array, obj_count);
    for EachIndex(obj_idx, obj_count) {
      LNK_Obj *obj = obj_arr[obj_idx];

      if (obj->coff.debug_t_section_number > 0) {
        raw_debug_t_arr[obj_idx].count = 1;
        raw_debug_t_arr[obj_idx].v     = push_array(scratch.arena, String8, 1);
        raw_debug_t_arr[obj_idx].v[0]  = lnk_obj_section_data_from_number(obj, obj->coff.debug_t_section_number);
      }

      if (obj->coff.debug_p_section_number > 0) {
        raw_debug_p_arr[obj_idx].count = 1;
        raw_debug_p_arr[obj_idx].v     = push_array(scratch.arena, String8, 1);
        raw_debug_p_arr[obj_idx].v[0]  = lnk_obj_section_data_from_number(obj, obj->coff.debug_p_section_number);
      }
    }

    LNK_ParseCvTypes parse_types = { .input = &input };

    // parse .debug$P
    debug_p_arr = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = raw_debug_p_arr;
    parse_types.out_types = debug_p_arr;
    parse_types.is_debug_p = 1;
    lnk_tp_for_parallel_capped_prof(tp, 0,        config->debug_worker_cap, obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip .debug$P");
    lnk_tp_for_parallel_capped_prof(tp, 0,        config->debug_worker_cap, obj_count, lnk_parse_debug_t_sidecar_task, &parse_types, "Index .debug$P sidecars");
    lnk_tp_for_parallel_capped_prof(tp, tp_arena, config->debug_worker_cap, obj_count, lnk_parse_debug_t_task,     &parse_types, "Parse .debug$P");

    // parse .debug$T
    input.debug_t_arr     = push_array(tp_arena->v[0], CV_DebugT, obj_count);
    parse_types.raw_types = raw_debug_t_arr;
    parse_types.out_types = input.debug_t_arr;
    parse_types.is_debug_p = 0;
    lnk_tp_for_parallel_capped_prof(tp, 0,        config->debug_worker_cap, obj_count, lnk_strip_debug_t_sig_task, &parse_types, "Strip .debug$T");
    lnk_tp_for_parallel_capped_prof(tp, 0,        config->debug_worker_cap, obj_count, lnk_parse_debug_t_sidecar_task, &parse_types, "Index .debug$T sidecars");
    lnk_parse_giant_debug_t(tp, tp_arena, config, obj_count, &parse_types);
    lnk_tp_for_parallel_capped_prof(tp, tp_arena, config->debug_worker_cap, obj_count, lnk_parse_debug_t_task,     &parse_types, "Parse .debug$T");

    // parse .debug$H
    input.debug_h_arr = push_array(tp_arena->v[0], CV_DebugH, input.obj_count);
    if (config->ghash) {
      lnk_tp_for_parallel_capped_prof(tp, tp_arena, config->debug_worker_cap, obj_count, lnk_parse_debug_h_task, &input, "Parse .debug$H");
    }
  }
  ProfEnd();
  lnk_compressed_obj_log_phase_stats("after parse CodeView");

  // sort objs based on type: PCH, /Zi (external), /Z7 (internal)
  input.debug_p_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  input.ext_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  input.int_obj_indices.v = push_array(tp_arena->v[0], U32, obj_count);
  ProfScope("Classify Objs")
  {
    // parallel: classify each obj + apply per-obj debug_t mutation (see lnk_cv_classify_task)
    LNK_CvClassifyTask classify = {0};
    classify.input       = &input;
    classify.debug_p_arr = debug_p_arr;
    classify.rrt_hm      = &rrt_hm;
    classify.obj_arr     = obj_arr;
    classify.class_tag   = push_array(scratch.arena, U8, obj_count ? obj_count : 1);
    classify.warn_multi  = push_array(scratch.arena, U8, obj_count ? obj_count : 1);
    tp_for_parallel_prof(tp, 0, obj_count, lnk_cv_classify_task, &classify, "Classify Objs (parallel)");

    // serial obj-order compaction into the 3 ordered arrays + deterministic warning emission.
    // Cache-linear single pass; preserves the exact element order + warning order of the old loop.
    for EachIndex(obj_idx, obj_count) {
      U32Array *arr_ptr;
      switch (classify.class_tag[obj_idx]) {
        case 0:  arr_ptr = &input.debug_p_indices; break;
        case 1:  arr_ptr = &input.ext_obj_indices; break;
        default: arr_ptr = &input.int_obj_indices; break;
      }
      arr_ptr->v[arr_ptr->count++] = obj_idx;

      if (classify.warn_multi[obj_idx]) {
        lnk_error_obj(LNK_Warning_MultipleDebugTAndDebugP, obj_arr[obj_idx], "multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections");
      }
    }
  }

  ProfScope("Set up PDB and RRT")
  {
    input.obj_to_ts = push_array(tp_arena->v[0], U64, input.count);
    MemorySet(input.obj_to_ts, 0xff, input.count * sizeof(input.obj_to_ts[0]));

    LNK_TypeServerList  ts_list = {0};
    HashTable          *ts_ht   = hash_table_init(scratch.arena, 256);

    // push null type server (slots with broken type servers are set to null)
    LNK_TypeServerNode *null_ts = push_array(scratch.arena, LNK_TypeServerNode, 1);
    SLLQueuePush(ts_list.first, ts_list.last, null_ts);
    ts_list.count += 1;
    null_ts->v.ts_path = str8_lit("");
    hash_table_push_path_raw(scratch.arena, ts_ht, str8_lit(""), null_ts);

    for EachIndex(i, input.ext_obj_indices.count) {
      // first leaf is always type server
      U64 obj_idx = input.ext_obj_indices.v[i];

      U64 *packed_rrt_info = hash_map_search_path_u64(&rrt_hm, obj_arr[obj_idx]->path);

      LNK_TypeServerKind ts_kind = LNK_TypeServerKind_Null;
      CV_TypeServerInfo  ts_info = {0};
      String8            ts_path = {0};
      LNK_RRT           *rrt     = 0;
      if (packed_rrt_info) {
        U32 rrt_idx = *packed_rrt_info >> 32;
        rrt     = &rrt_input.v[rrt_idx];
        ts_kind = LNK_TypeServerKind_RRT;
        ts_path = rrt->path;
      } else {
        CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];
        CV_Leaf   leaf     = cv_debug_t_get_leaf(debug_t, 0);
        ts_kind = LNK_TypeServerKind_PDB;
        ts_info = cv_type_server_info_from_leaf(leaf);
        // P4: ts_info.name points into the raw $T leaf 0 bytes of the mapped obj view -- copy it
        // out so nothing downstream retains a raw-view pointer (ts_info is stored in ts_arr and
        // read during/after the merge)
        ts_info.name = push_str8_copy(tp_arena->v[0], ts_info.name);
        ts_path = lnk_find_first_file(scratch.arena, config->lib_dir_list, ts_info.name);
      }

      if (ts_path.size == 0) {
        lnk_discard_cv_debug_info(&input, obj_idx);
        continue;
      }

      // insert new type server
      LNK_TypeServer *ts = hash_table_search_path_raw(ts_ht, ts_path);
      if (ts == 0) {
        LNK_TypeServerNode *n = push_array(scratch.arena, LNK_TypeServerNode, 1);
        SLLQueuePush(ts_list.first, ts_list.last, n);
        ts_list.count += 1;
        ts = &n->v;
        ts->ts_info = ts_info;
        ts->ts_idx  = ts_ht->count;
        ts->ts_kind = ts_kind;
        ts->ts_path = push_str8_copy(tp_arena->v[0], ts_path);
        ts->rrt     = rrt;
        hash_table_push_path_raw(scratch.arena, ts_ht, ts->ts_path, ts);
      }
      
      // signature check
      if ( ! MemoryMatchStruct(&ts_info.sig, &ts->ts_info.sig)) {
        lnk_error_obj(LNK_Error_ExternalTypeServerConflict,
                      obj_arr[obj_idx],
                      "type server signature conflicts with type server from '%S'",
                      obj_arr[ts->obj_indices.first->data]->path);
        lnk_discard_cv_debug_info(&input, obj_idx);
        continue;
      }

      // type server -> obj
      u64_list_push(tp_arena->v[0], &ts->obj_indices, obj_idx);

      // obj -> type server
      input.obj_to_ts[obj_idx] = ts->ts_idx;
    }

    // list -> array
    LNK_TypeServerArray ts_arr = { .v = push_array(tp_arena->v[0], LNK_TypeServer, ts_list.count) };
    for EachNode(n, LNK_TypeServerNode, ts_list.first) { ts_arr.v[ts_arr.count++] = n->v; }

    // extend arrays to include type servers
    if (ts_arr.count) {
      LNK_CodeViewInput prev = input;

      input.count += ts_arr.count;
      input.obj_arr        = push_array(tp_arena->v[0], LNK_Obj *, input.count);
      input.debug_s_arr    = push_array(tp_arena->v[0], CV_DebugS, input.count);
      input.debug_t_arr    = push_array(tp_arena->v[0], CV_DebugT, input.count);
      input.debug_h_arr    = push_array(tp_arena->v[0], CV_DebugH, input.count);
      input.obj_to_ts      = push_array(tp_arena->v[0], U64,       input.count);

      MemoryCopyTyped(input.obj_arr,        prev.obj_arr,        prev.count);
      MemoryCopyTyped(input.debug_s_arr,    prev.debug_s_arr,    prev.count);
      MemoryCopyTyped(input.debug_t_arr,    prev.debug_t_arr,    prev.count);
      MemoryCopyTyped(input.debug_h_arr,    prev.debug_h_arr,    prev.count);
      MemoryCopyTyped(input.obj_to_ts,      prev.obj_to_ts,      prev.count);
      MemorySet(input.obj_to_ts + input.obj_count, 0xff, ts_arr.count * sizeof(input.obj_to_ts[0]));

      input.ts_obj_range = r1u64(prev.count, input.count);

      // alloc dummy objs with for each loaded type server
      // (one obj per type server; this used to push a ts_arr.count-sized array
      //  per iteration and use only its first element -- O(T^2) arena growth)
      LNK_Obj *ts_objs = push_array(tp_arena->v[0], LNK_Obj, ts_arr.count);
      for EachIndex(i, ts_arr.count) {
        ts_objs[i].path = ts_arr.v[i].ts_path;
        input.obj_arr[prev.count + i] = &ts_objs[i];
      }

      // make type server indices
      input.type_server_indices.count = ts_arr.count;
      input.type_server_indices.v     = push_array(tp_arena->v[0], U32, ts_arr.count);
      for EachIndex(i, ts_arr.count) { input.type_server_indices.v[i] = prev.count + i; }
    }

    input.ts_arr                   = ts_arr;
    input.is_type_server_discarded = push_array(tp_arena->v[0], B32, input.ts_arr.count);
    tp_for_parallel_prof(tp, tp_arena, input.ts_arr.count, lnk_read_type_servers_task, &input, "read type servers");

    // undiscard null type server
    input.is_type_server_discarded[0] = 0;

    // wire RRT hashes to type servers .debug$H
    for EachIndex(ts_idx, ts_arr.count) {
      LNK_TypeServer *ts = &ts_arr.v[ts_idx];
      if (ts->rrt && ts->rrt->debug_types_hash == config->debug_types_hash) {
        U64        ts_obj_idx = input.ts_obj_range.min + ts_idx;
        CV_DebugT *debug_t    = &input.debug_t_arr[ts_obj_idx];
        CV_DebugH *debug_h    = &input.debug_h_arr[ts_obj_idx];
        debug_h->count = ts->rrt->type_hashes.size / sizeof(U64);
        debug_h->v     = (U64 *)ts->rrt->type_hashes.str;
      }
    }

    // report bad type servers
    String8List unopen_type_server_list = {0};
    for EachIndex(ts_idx, ts_arr.count) {
      if ( ! input.is_type_server_discarded[ts_idx]) { continue; }
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t%S\n", ts_arr.v[ts_idx].ts_path);
      str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\tDependent objs:\n");
      for EachNode(n, U64Node, input.ts_arr.v[ts_idx].obj_indices.first) {
        str8_list_pushf(scratch.arena, &unopen_type_server_list, "\t\t\t%S\n", obj_arr[n->data]->path);
      }
    }
    if (unopen_type_server_list.node_count) {
      String8List error_msg_list = {0};
      str8_list_pushf(scratch.arena, &error_msg_list, "unable to open external type server(s):\n");
      str8_list_concat_in_place(&error_msg_list, &unopen_type_server_list);
      lnk_error(LNK_Error_UnableToOpenTypeServer, "%S", str8_list_join(scratch.arena, &error_msg_list, 0));
    }
  }
 
  ProfBegin("Set up PCH");
  {
    // register PCH file paths
    HashMap debug_p_hm_path = {0};
    HashMap debug_p_hm_name = {0}; // (obj name, U64List of PCH obj indices)
    for EachIndex(i, input.debug_p_indices.count) {
      U64      obj_idx = input.debug_p_indices.v[i];
      LNK_Obj *obj     = obj_arr[obj_idx];

      // register file path -> obj idx map
      String8 obj_path = path_absolute_dst_from_relative_dst_src(scratch.arena, obj_arr[obj_idx]->path, config->work_dir);
      if (hash_map_search_path_u64(&debug_p_hm_path, obj_path)) {
        lnk_error_obj(LNK_Warning_DuplicateObjPath, obj, "duplicate obj path %S", obj_path);
      } else {
        hash_map_push_path_u64(scratch.arena, &debug_p_hm_path, obj_path, obj_idx);
      }

      // register file name -> obj idx map
      String8  obj_name      = str8_skip_last_slash(obj_path);
      U64List *match_indices = hash_map_search_path_raw(&debug_p_hm_name, obj_name);
      if (match_indices == 0) {
        match_indices = push_array(scratch.arena, U64List, 1);
        hash_map_push_path_raw(scratch.arena, &debug_p_hm_name, obj_name, match_indices);
      }
      u64_list_push(scratch.arena, match_indices, obj_idx);
    }

    for EachIndex(i, input.int_obj_indices.count) {
      U64        obj_idx = input.int_obj_indices.v[i];
      CV_DebugT *debug_t = &input.debug_t_arr[obj_idx];

      // skip objs that do not depend on PCH
      if ( ! cv_debug_t_is_pch(debug_t)) { continue; }

      // find PCH obj by file path
      CV_PrecompInfo  precomp             = cv_precomp_info_from_leaf(cv_debug_t_get_leaf(debug_t, 0));
      String8         obj_path            = path_absolute_dst_from_relative_dst_src(scratch.arena, precomp.obj_name, config->work_dir);
      U64            *debug_p_obj_idx_ptr = hash_map_search_path_u64(&debug_p_hm_path, obj_path);
      U64             debug_p_obj_idx     = debug_p_obj_idx_ptr ? *debug_p_obj_idx_ptr : max_U64;

      // find PCH obj by signature
      if (debug_p_obj_idx_ptr == 0) {
        for EachIndex(pch_i, input.debug_p_indices.count) {
          U64        pch_obj_idx = input.debug_p_indices.v[pch_i];
          CV_DebugT *pch_debug_t = &input.debug_t_arr[pch_obj_idx];
          if (precomp.leaf_count >= pch_debug_t->count) { continue; }

          CV_Leaf end_leaf = cv_debug_t_get_leaf(pch_debug_t, precomp.leaf_count);
          if (end_leaf.kind != CV_LeafKind_ENDPRECOMP)        { continue; }
          if (end_leaf.data.size < sizeof(CV_LeafEndPreComp)) { continue; }

          CV_LeafEndPreComp *end_precomp = str8_deserial_get_raw_ptr(end_leaf.data, 0, sizeof(*end_precomp));
          if (end_precomp->sig != precomp.sig) { continue; }

          debug_p_obj_idx = pch_obj_idx;
          break;
        }
      }

      if (debug_p_obj_idx > input.obj_count) {
        lnk_error_obj(LNK_Error_PrecompObjNotFound, obj_arr[obj_idx], "LF_PRECOMP references non-existent obj %S; discarding debug info", obj_path);
        lnk_discard_cv_debug_info(&input, obj_idx);
        continue;
      }

      // get PCH leaf data
      CV_DebugT *debug_p = &input.debug_t_arr[debug_p_obj_idx];

      // error check LF_PRECOMP
      if (precomp.start_index > CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Warning_AtypicalStartIndex,    obj_arr[obj_idx], "atypical start index 0x%x in LF_PRECOMP", precomp.start_index); }
      if (precomp.start_index < CV_MinComplexTypeIndex) { lnk_error_obj(LNK_Error_InvalidStartIndex,       obj_arr[obj_idx], "invalid start index 0x%x in LF_PRECOMP; must be >= 0x%x", precomp.start_index, CV_MinComplexTypeIndex); continue; }
      if (precomp.leaf_count  >= debug_p->count)        { lnk_error_obj(LNK_Error_InvalidPrecompLeafCount, obj_arr[obj_idx], "leaf count %u LF_PRECOMP exceeds leaf count %u in .debug$P in %S", precomp.leaf_count, debug_p->count, obj_arr[debug_p_obj_idx]->path); continue; }

      // get LF_PRECOMP
      CV_Leaf            endprecomp_leaf = cv_debug_t_get_leaf(debug_p, precomp.leaf_count);
      CV_LeafEndPreComp *endprecomp      = str8_deserial_get_raw_ptr(endprecomp_leaf.data, 0, sizeof(*endprecomp));

      // error check LF_ENDPRECOMP
      if (endprecomp_leaf.kind      != CV_LeafKind_ENDPRECOMP)    { lnk_error_obj(LNK_Error_EndprecompNotFound, obj_arr[obj_idx], "missing LF_ENDPRECOMP [0x%x] in %S", precomp.leaf_count, obj_arr[debug_p_obj_idx]->path); continue; }
      if (endprecomp_leaf.data.size != sizeof(CV_LeafEndPreComp)) { lnk_error_obj(LNK_Error_IllData,            obj_arr[obj_idx], "invalid size 0x%x for LF_ENDPRECOMP", endprecomp_leaf.data.size); continue; }
      if (endprecomp->sig           != precomp.sig)               { lnk_error_obj(LNK_Error_PrecompSigMismatch, obj_arr[obj_idx], "PCH signature mismatch, expected 0x%x got 0x%x; PCH obj %S", precomp.sig, endprecomp->sig, obj_arr[debug_p_obj_idx]->path); continue; }

      for (U64 i = 1; i < CV_TypeIndexSource_COUNT; i += 1) { debug_t->pch_ti_range[i] = r1u64(precomp.start_index, precomp.start_index + precomp.leaf_count); }
      debug_t->pch_obj_idx  = debug_p_obj_idx;

      // remove CV_LeafKind_PRECOMP
      debug_t->count    -= 1;
      debug_t->sidecar_leaf_bias += 1;
    }

    // remove LF_ENDPRECOMP from .debug$P -- P4: journal the NOTYPE rewrite instead of dirtying
    // the mapped $P view (the backward header scan to FIND it still reads the raw view; those
    // pages are hot from the parse)
    for EachIndex(i, input.debug_p_indices.count) {
      U64            debug_p_idx = input.debug_p_indices.v[i];
      CV_DebugT     *debug_p     = &input.debug_t_arr[debug_p_idx];
      for EachIndex(i, debug_p->count) {
        U64            lf_idx = debug_p->count - (i + 1);
        if (cv_debug_t_get_leaf_kind(debug_p, lf_idx) == CV_LeafKind_ENDPRECOMP) {
          lnk_notype_journal_push(tp_arena->v[0], &input.notype_journal[debug_p_idx], (U32)lf_idx, 0, debug_p->count);
          break;
        }
      }
    }
  }
  ProfEnd();

  // resolve MSVC header-unit IFC debug records (LF_IFC_RECORD 0x1522) -> real CodeView types.
  // injects .ifc debug-records blobs as extra objs and registers placeholder-TI redirects.
  if (config->ifc_debug_records == LNK_SwitchState_Yes && config->ifc_map_list.node_count) {
    lnk_apply_ifc_debug_records(tp, tp_arena, &input, config);
  }

  // set default min type index
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { input.min_type_indices[ti_source] = CV_MinComplexTypeIndex; }

  // PCH and /Z7 objs have default min type index set to CV_MinComplexTypeIndex
  // but type servers can bump up the lower bound. In practice nobody does this.
  // But to cover all our bases loop through type servers and compute max
  // lower bound.
  for EachInRange(ts_idx, input.ts_obj_range) {
    CV_DebugT *debug_t = &input.debug_t_arr[ts_idx];
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      input.min_type_indices[ti_source] = Max(input.min_type_indices[ti_source], debug_t->ti_ranges[ti_source].min);
    }
  }

  ProfBegin("Make Symbol Inputs");
  {
    // count symbol blocks (cache each obj's Symbols sub-section list so the fill pass below
    // does not re-decode .debug$S a second time -- cv_sub_section_from_debug_s walks subsections).
    String8List  *per_obj_syms = push_array(scratch.arena, String8List, input.count ? input.count : 1);
    LNK_CvSymTask sym_task      = {0};
    sym_task.input        = &input;
    sym_task.per_obj_syms = per_obj_syms;
    sym_task.counts       = push_array(scratch.arena, U64, input.count ? input.count : 1);

    // parallel: cache each obj's Symbols sub-section list + count nodes
    tp_for_parallel_prof(tp, 0, input.count, lnk_cv_sym_count_task, &sym_task, "Count Symbol Inputs");
    input.symbol_input_count = sum_array_u64(input.count, sym_task.counts);
    sym_task.offsets         = offsets_from_counts_array_u64(scratch.arena, sym_task.counts, input.count);

    // alloc block pointers
    input.symbol_inputs = push_array_no_zero(tp_arena->v[0], LNK_SymbolInput, input.symbol_input_count ? input.symbol_input_count : 1);

    // parallel fill into disjoint per-obj ranges at prefix-sum offsets (byte-identical order)
    tp_for_parallel_prof(tp, 0, input.count, lnk_cv_sym_fill_task, &sym_task, "Fill Symbol Inputs");

    ProfBegin("Make Ranges");

    U64 total_input_size = 0;
    for EachIndex(i, input.symbol_input_count) { total_input_size += input.symbol_inputs[i].raw_symbols.size; }

    U64 max_weight = CeilIntegerDiv(total_input_size, tp->worker_count);
    U64 cursor     = 0;
    input.symbol_input_ranges      = push_array(tp_arena->v[0], Rng1U64, tp->worker_count);
    input.symbol_input_range_count = tp->worker_count;
    for EachIndex(i, tp->worker_count) {
      if (cursor >= input.symbol_input_count) { break; }
      U64 begin  = cursor;
      U64 weight = 0;
      for (; cursor < input.symbol_input_count; cursor += 1) {
        if (weight >= max_weight) { break; }
        weight += input.symbol_inputs[cursor].raw_symbols.size;
      }
      input.symbol_input_ranges[i] = r1u64(begin, cursor);
    }

    ProfEnd();

    if (input.symbol_input_count) {
      ProfBegin("Balance Symbol Inputs");

      U64 task_cap    = Min(input.symbol_input_count, tp->worker_count * 16);
      U64 task_weight = Max(1, CeilIntegerDiv(total_input_size, task_cap));

      input.symbol_patch_task = push_array_no_zero(tp_arena->v[0], LNK_SymbolInputTask, task_cap);

      cursor = 0;
      while (cursor < input.symbol_input_count) {
        U64 begin  = cursor;
        U64 weight = 0;
        do {
          weight += input.symbol_inputs[cursor++].raw_symbols.size;
        } while (cursor < input.symbol_input_count && weight < task_weight);

        Assert(input.symbol_patch_task_count < task_cap);
        LNK_SymbolInputTask *task = &input.symbol_patch_task[input.symbol_patch_task_count++];
        task->input_range = r1u64(begin, cursor);
        task->weight      = weight;
      }

      radsort(input.symbol_patch_task, input.symbol_patch_task_count, lnk_symbol_input_task_is_before);

      ProfEnd();
    }
  }
  ProfEnd();
  lnk_compressed_obj_log_phase_stats("after make symbol inputs");

  scratch_end(scratch);
  ProfEnd();
  return input;
}

internal force_inline LNK_LeafRef
lnk_leaf_ref_make(U64 obj_idx, U64 leaf_idx)
{
  // Type indices consume the low 32 bits. Realistic object counts fit comfortably in the
  // remaining 29 payload bits; the top three bits stay free for the dedup-table hash tag.
  Assert(obj_idx < (1ull << 29) - 1);
  LNK_LeafRef result = (obj_idx << 32) | safe_cast_u32(leaf_idx);
  Assert(result != LNK_LEAF_REF_NULL);
  return result;
}

internal force_inline U32
lnk_leaf_ref_obj_idx(LNK_LeafRef ref)
{
  return (U32)((ref >> 32) & ((1ull << 29) - 1));
}

internal force_inline U32
lnk_leaf_ref_leaf_idx(LNK_LeafRef ref)
{
  return (U32)ref;
}

// $T streaming (ring P4): NOTYPE journal ops. Pushes are single-threaded per obj (input-phase
// pushes are serial or per-obj tasks; hash-phase pushes come from the obj's own hash task), so
// no atomics. Sorted insert keeps lookup a bsearch; the common shapes are 0 entries (fast path)
// or an append at the tail (input-phase entries land before hash-phase entries only for $P objs
// that are also IFC consumers -- the linear tail walk handles the interleave).
internal void
lnk_notype_journal_push(Arena *arena, LNK_NotypeJournal *journal, U32 leaf_idx, B32 kind_only, U64 bit_cap)
{
  if (journal->count == journal->cap) {
    AssertAlways(arena != 0); // pseudo objs never journal; real-obj pushes always have an arena
    U32  new_cap = journal->cap ? journal->cap * 2 : 8;
    U32 *new_v   = push_array_no_zero(arena, U32, new_cap);
    MemoryCopyTyped(new_v, journal->v, journal->count);
    journal->v   = new_v;
    journal->cap = new_cap;
  }
  if (journal->bitmap == 0) {
    AssertAlways(arena != 0);
    journal->bitmap  = push_array(arena, U64, (bit_cap + 63) / 64);
    journal->bit_cap = bit_cap;
  }
  // out-of-span indices (the pre-existing `curr_ti - min` quirk on invalid-TI error paths can
  // exceed the leaf count) get a journal entry but no bit; readers only query leaf_idx < count,
  // so the entry is unreachable either way (the old in-place write was equally out-of-bounds)
  if (leaf_idx < journal->bit_cap) { journal->bitmap[leaf_idx >> 6] |= (1ull << (leaf_idx & 63)); }
  U32 entry = leaf_idx | (kind_only ? LNK_NOTYPE_JOURNAL_KIND_ONLY : 0);
  U32 i     = journal->count;
  for (; i > 0 && (journal->v[i-1] & ~LNK_NOTYPE_JOURNAL_KIND_ONLY) > leaf_idx; i -= 1) {
    journal->v[i] = journal->v[i-1];
  }
  Assert(i == 0 || (journal->v[i-1] & ~LNK_NOTYPE_JOURNAL_KIND_ONLY) != leaf_idx);
  journal->v[i]   = entry;
  journal->count += 1;
}

// O(1) hot-path test: bitmap == 0 for every obj without journal entries (the common case)
internal B32
lnk_notype_journal_test(LNK_NotypeJournal *journal, U64 leaf_idx)
{
  return journal->bitmap != 0 && leaf_idx < journal->bit_cap && ((journal->bitmap[leaf_idx >> 6] >> (leaf_idx & 63)) & 1);
}

internal B32
lnk_notype_journal_find(LNK_NotypeJournal *journal, U32 leaf_idx, B32 *kind_only_out)
{
  if (journal->count == 0) { return 0; }
  U32 lo = 0, hi = journal->count;
  while (lo < hi) {
    U32 mid = lo + (hi - lo) / 2;
    U32 key = journal->v[mid] & ~LNK_NOTYPE_JOURNAL_KIND_ONLY;
    if (key < leaf_idx)      { lo = mid + 1; }
    else if (key > leaf_idx) { hi = mid;     }
    else {
      if (kind_only_out) { *kind_only_out = !!(journal->v[mid] & LNK_NOTYPE_JOURNAL_KIND_ONLY); }
      return 1;
    }
  }
  return 0;
}

// Journal-aware leaf read: raw views hold PRE-rewrite bytes for real objs, so present the
// journaled view of the leaf to the hashers (identical to what the old in-place writes produced:
// full rewrite => { kind=LF_NOTYPE, empty payload }; KIND_ONLY => kind=LF_NOTYPE, payload kept).
internal CV_Leaf
lnk_cv_leaf_from_leaf_ref(Arena *arena, LNK_CObjDecodeWindow *decode_window,
                          LNK_CodeViewInput *input, U32 obj_idx, U32 leaf_idx)
{
  CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
  CV_Leaf    leaf    = {0};
  LNK_Obj   *obj     = obj_idx < input->obj_count ? input->obj_arr[obj_idx] : 0;
  if (obj && obj->compressed_obj && (debug_t->sidecar_sizes || debug_t->sidecar_packed) && decode_window) {
    U64 raw_size = cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx);
    U8 *raw_leaf = push_array_no_zero(arena, U8, raw_size);
    U64 leaf_off = cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
    Rng1U64 raw_range = rng_1u64(debug_t->sidecar_raw_base + leaf_off,
                                 debug_t->sidecar_raw_base + leaf_off + raw_size);
    if (lnk_compressed_obj_copy_range(obj->compressed_obj, raw_range, raw_leaf, decode_window)) {
      cv_read_leaf(str8(raw_leaf, raw_size), 0, 1, &leaf);
    } else {
      leaf = cv_debug_t_get_leaf(debug_t, leaf_idx);
    }
  } else {
    leaf = cv_debug_t_get_leaf(debug_t, leaf_idx);
  }
  // hot path: one bitmap test; journals are empty for ~all objs
  if (obj_idx < input->obj_count && lnk_notype_journal_test(&input->notype_journal[obj_idx], leaf_idx)) {
    B32 kind_only = 0;
    lnk_notype_journal_find(&input->notype_journal[obj_idx], leaf_idx, &kind_only);
    leaf.kind = CV_LeafKind_NOTYPE;
    if (!kind_only) { leaf.data.size = 0; }
  }
  return leaf;
}

// Journal-aware raw leaf size (materialize buffer sizing + copy). Hot path = the ORIGINAL
// header read (one bitmap test on top); only a full NOTYPE rewrite changes the answer (the
// leaf shrank to its 4-byte header; KIND_ONLY keeps the size field).
#define LNK_LEAF_MATERIALIZE_FULL_NOTYPE 1
#define LNK_LEAF_MATERIALIZE_KIND_NOTYPE 2

internal U64
lnk_leaf_ref_materialize_meta(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  U32        obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
  U32        leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
  CV_DebugT *debug_t  = &input->debug_t_arr[obj_idx];
  U64        raw_size = cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx);
  U64        rewrite  = 0;
  if (obj_idx < input->obj_count && lnk_notype_journal_test(&input->notype_journal[obj_idx], leaf_idx)) {
    B32 kind_only = 0;
    lnk_notype_journal_find(&input->notype_journal[obj_idx], leaf_idx, &kind_only);
    if (kind_only) {
      rewrite = LNK_LEAF_MATERIALIZE_KIND_NOTYPE;
    } else {
      raw_size = sizeof(CV_LeafHeader);
      rewrite = LNK_LEAF_MATERIALIZE_FULL_NOTYPE;
    }
  }
  return (raw_size << 2) | rewrite;
}

internal LNK_LeafRef
lnk_leaf_ref_from_ti(LNK_CodeViewInput *input, U32 obj_idx, CV_TypeIndexSource source, CV_TypeIndex ti)
{
  // IFC redirect: a consuming obj's local LF_IFC_RECORD placeholder TI is mapped
  // to a leaf inside an injected .ifc debug-records blob obj. The blob leaves then
  // dedup/hash/fixup natively through the rest of this function.
  if (input->has_ifc_redirects && source == CV_TypeIndexSource_TPI) {
    // exact per-obj bitset filter: bit set iff Compose64Bit(obj_idx, ti) is a key in
    // ifc_redirect_hm. skips the (miss-dominated) per-call key hash + map walk; on a set
    // bit the original map search runs unchanged, so behavior is bit-identical.
    U64 *bits = input->ifc_redirect_bits[obj_idx];
    if (bits != 0 && contains_1u64(input->ifc_redirect_ti_rng[obj_idx], ti)) {
      U64 rel = ti - input->ifc_redirect_ti_rng[obj_idx].min;
      if (bits[rel >> 6] & (1ull << (rel & 63))) {
        U64 *packed = hash_map_search_u64_u64(&input->ifc_redirect_hm, Compose64Bit(obj_idx, ti));
        if (packed) {
          return lnk_leaf_ref_make((U32)(*packed >> 32), (U32)(*packed & max_U32));
        }
      }
    }
  }

  // ti range: external type server
  U64 ts_idx = input->obj_to_ts[obj_idx];
  if (ts_idx != max_U64) {
    U64             ts_debug_t_idx = input->ts_obj_range.min + ts_idx;
    CV_DebugT      *ts_debug_t     = input->debug_t_arr + ts_debug_t_idx;
    LNK_TypeServer *ts             = &input->ts_arr.v[ts_idx];

    // RRT indirection
    if (ts->rrt) {
      LNK_Obj *obj = input->obj_arr[obj_idx];

      // RRT-PCH indirection
      if (contains_1u64(obj->pch_ti_range, ti)) {
        obj_idx = obj->pch_obj_idx;
        obj     = input->obj_arr[obj_idx];
      }

      Assert(contains_1u64(obj->ti_range, ti));

      // translate type index to original leaf index
      U64 leaf_idx_og  = (ti - obj->ti_range.min);
      leaf_idx_og     -= dim_1u64(obj->pch_ti_range);

      // map original leaf index to RRT type index
      CV_TypeIndex final_ti = obj->ti_map[leaf_idx_og];
      return lnk_leaf_ref_make(ts_debug_t_idx, cv_leaf_idx_from_ti(ts_debug_t, source, final_ti));
    } 
    
    return lnk_leaf_ref_make(ts_debug_t_idx, cv_leaf_idx_from_ti(ts_debug_t, source, ti));
  }

  CV_DebugT *debug_t = input->debug_t_arr + obj_idx;

  // ti_range: PCH
  if (contains_1u64(debug_t->pch_ti_range[source], ti)) {
    return lnk_leaf_ref_make(debug_t->pch_obj_idx,
                             cv_leaf_idx_from_ti(&input->debug_t_arr[debug_t->pch_obj_idx], source, ti));
  }

  // ti range: internal type server
  return lnk_leaf_ref_make(obj_idx, cv_leaf_idx_from_ti(debug_t, source, ti));
}

internal U64
lnk_hash_from_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  return input->debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)].v[lnk_leaf_ref_leaf_idx(leaf_ref)];
}

internal int
lnk_leaf_ref_compare(LNK_LeafRef a, LNK_LeafRef b)
{
  return a < b ? -1 : a > b ? +1 : 0;
}

internal B32
lnk_match_leaf_ref(LNK_CodeViewInput *input, LNK_LeafRef a, LNK_LeafRef b)
{
  U64 a_hash = lnk_hash_from_leaf_ref(input, a);
  U64 b_hash = lnk_hash_from_leaf_ref(input, b);
  return a_hash == b_hash;
}

#define LNK_LEAF_BUCKET_TAG_MASK (7ull << 61)

internal force_inline LNK_LeafRef
lnk_leaf_bucket_tag(LNK_LeafRef ref, U64 hash)
{
  Assert((ref & LNK_LEAF_BUCKET_TAG_MASK) == 0);
  LNK_LeafRef result = ref | (hash & LNK_LEAF_BUCKET_TAG_MASK);
  Assert(result != LNK_LEAF_REF_NULL);
  return result;
}

internal force_inline LNK_LeafRef
lnk_leaf_bucket_untag(LNK_LeafRef ref)
{
  return ref & ~LNK_LEAF_BUCKET_TAG_MASK;
}

// P4: `leaf` is the caller's (journal-aware) read of the leaf -- this function no longer
// re-reads it from the raw view. `journal_arena` backs NOTYPE journal growth for real objs
// (pseudo objs keep in-place rewrites and may pass 0).
internal U64
lnk_hash_cv_leaf(LNK_CodeViewInput *input, Arena *journal_arena, LNK_LeafRef leaf_ref, CV_Leaf leaf, CV_TiOffsets ti_offs, B32 discard_cycles)
{
  U32                 obj_idx        = lnk_leaf_ref_obj_idx(leaf_ref);
  U32                 leaf_idx       = lnk_leaf_ref_leaf_idx(leaf_ref);
  CV_DebugT          *debug_t        = &input->debug_t_arr[obj_idx];
  CV_TypeIndexSource  curr_ti_source = cv_type_index_source_from_leaf_kind(leaf.kind);
  CV_TypeIndex        curr_ti        = cv_ti_from_leaf_idx(debug_t, curr_ti_source, leaf_idx);
  U64                 ti_count       = cv_ti_offsets_count(&ti_offs);

  // init hasher
  LNK_Hasher hasher;
  lnk_hasher_init(&hasher, input->config->debug_types_hash);

  // hash bytes around indices
  {
    U64 last_ti_off = 0;
    for (U64 ti_idx = 0; ti_idx < ti_count; ti_idx += 1) {
      CV_TiOff ti_info = cv_ti_offset_at(&ti_offs, ti_idx);
      U8 *bytes = leaf.data.str + last_ti_off;
      U64 size  = ti_info.offset - last_ti_off;
      lnk_hasher_update(&hasher, bytes, size);
      last_ti_off = ti_info.offset + sizeof(CV_TypeIndex);
    }

    Assert(leaf.data.size >= last_ti_off);
    U8 *bytes = leaf.data.str + last_ti_off;
    U64 size  = leaf.data.size - last_ti_off;
    lnk_hasher_update(&hasher, bytes, size);
  }

  // P4: set when a discard below rewrote THIS leaf's header (the final header mix-in must
  // then hash the NOTYPE header, exactly like the old post-write pointer read did)
  B32 self_discarded = 0;

  // mix-in sub leaf hashes
  for (U64 ti_idx = 0; ti_idx < ti_count; ti_idx += 1) {
    CV_TiOff      sub_ti_n   = cv_ti_offset_at(&ti_offs, ti_idx);
    CV_TypeIndex *sub_ti_ptr = str8_deserial_get_raw_ptr(leaf.data, sub_ti_n.offset, sizeof(*sub_ti_ptr));
    CV_TypeIndex  sub_ti     = memory_read32(sub_ti_ptr);

    // simple indices are stable across compile units
    if (sub_ti < debug_t->ti_ranges[sub_ti_n.source].min) {
      lnk_hasher_update_struct(&hasher, &sub_ti);
      continue;
    }

    if (sub_ti >= debug_t->ti_ranges[sub_ti_n.source].max) {
      // discard type: journal the NOTYPE rewrite for view-backed real objs (raw input pages
      // stay clean); pseudo objs keep the in-place write on their arena-backed copy
      U32 leaf_idx = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      if (obj_idx < input->obj_count) {
        lnk_notype_journal_push(journal_arena, &input->notype_journal[obj_idx], leaf_idx, 0, debug_t->count);
      } else {
        U8 *leaf_header = debug_t->data.str + cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
        memory_write16(leaf_header + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
        memory_write16(leaf_header + OffsetOf(CV_LeafHeader, size), sizeof(CV_LeafKind));
      }
      if (leaf_idx == lnk_leaf_ref_leaf_idx(leaf_ref)) { self_discarded = 1; }

      // reset hasher
      lnk_hasher_init(&hasher, input->config->debug_types_hash);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) out of bounds type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, (U64)sub_ti_n.offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // discard type with a cyclic-ref
    B32 is_type_graph_cyclic = discard_cycles && sub_ti > 0 && sub_ti > curr_ti;
    if (is_type_graph_cyclic) {
      // discard type (journal for real objs, in-place for pseudo -- see the invalid-TI branch)
      U32 leaf_idx = curr_ti - debug_t->ti_ranges[curr_ti_source].min;
      if (obj_idx < input->obj_count) {
        lnk_notype_journal_push(journal_arena, &input->notype_journal[obj_idx], leaf_idx, 0, debug_t->count);
      } else {
        U8 *leaf_header = debug_t->data.str + cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
        memory_write16(leaf_header + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
        memory_write16(leaf_header + OffsetOf(CV_LeafHeader, size), sizeof(CV_LeafKind));
      }
      if (leaf_idx == lnk_leaf_ref_leaf_idx(leaf_ref)) { self_discarded = 1; }

      // reset hasher
      lnk_hasher_init(&hasher, input->config->debug_types_hash);

      // log error
      Temp    scratch       = scratch_begin(0,0);
      String8 leaf_kind_str = cv_string_from_leaf_kind(leaf.kind);
      String8 error_msg     = push_str8f(scratch.arena, "LF_%S(type_index: 0x%x) forward refs member type index 0x%x (leaf struct offset: 0x%llx)", leaf_kind_str, curr_ti, sub_ti, (U64)sub_ti_n.offset);
      lnk_error_obj(LNK_Error_InvalidTypeIndex, input->obj_arr[obj_idx], "%S", error_msg);
      scratch_end(scratch);

      break;
    }

    // type index -> hash
    LNK_LeafRef sub_ref  = lnk_leaf_ref_from_ti(input, obj_idx, sub_ti_n.source, sub_ti);
    U64         sub_hash = input->debug_h_arr[lnk_leaf_ref_obj_idx(sub_ref)].v[lnk_leaf_ref_leaf_idx(sub_ref)];

    // mix-in sub-type hash
    lnk_hasher_update_struct(&hasher, &sub_hash);
  }

  // hash leaf header. Hot path = the ORIGINAL raw pointer read; only journaled leaves (whose
  // raw header is unpatched) and self-discards (whose rewrite went to the journal for real
  // objs) reconstruct the header ({ size = data.size + sizeof(kind), kind } is byte-identical
  // to what the old post-in-place-write read produced)
  if (self_discarded) {
    CV_LeafHeader leaf_header = { .size = sizeof(CV_LeafKind), .kind = CV_LeafKind_NOTYPE };
    lnk_hasher_update_struct(&hasher, &leaf_header);
  } else if (obj_idx < input->obj_count &&
             lnk_notype_journal_test(&input->notype_journal[obj_idx], leaf_idx)) {
    CV_LeafHeader leaf_header = { .size = (CV_LeafSize)(leaf.data.size + sizeof(CV_LeafKind)), .kind = leaf.kind };
    lnk_hasher_update_struct(&hasher, &leaf_header);
  } else if (debug_t->sidecar_sizes || debug_t->sidecar_packed) {
    // The sparse raw view deliberately leaves .debug$T bodies (including the
    // four-byte leaf header) as holes.  The body above came from the bounded
    // decode window; reconstruct the byte-identical header from its sidecar
    // instead of mixing four zero bytes from the hole into the type hash.
    CV_LeafHeader leaf_header = { .size = (CV_LeafSize)(cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx) - sizeof(CV_LeafSize)), .kind = leaf.kind };
    lnk_hasher_update_struct(&hasher, &leaf_header);
  } else {
    lnk_hasher_update_struct(&hasher, cv_debug_t_get_leaf_header(debug_t, leaf_idx));
  }

  // finalize the type hash
  U64 hash = lnk_hasher_digest64(&hasher);

  Assert(hash != 0);
  Assert(input->debug_h_arr[obj_idx].v[leaf_idx] == 0 ||
         input->debug_h_arr[obj_idx].v[leaf_idx] == 1);
  input->debug_h_arr[obj_idx].v[leaf_idx] = hash;

  return hash;
}

internal void
lnk_hash_cv_leaf_deep(Arena               *arena,
                      LNK_CObjDecodeWindow *decode_window,
                      LNK_CodeViewInput   *input,
                      LNK_LeafRef          root_leaf_ref,
                      CV_TiOffsets         root_ti_offs)
{
  Temp temp = temp_begin(arena);

  typedef struct HashStack {
    struct HashStack    *next;
    LNK_LeafRef          leaf_ref;
    CV_TiOffsets         ti_offs;
    U64                  ti_next;
    U64                  ti_count;
    CV_Leaf              leaf;
    CV_TypeIndex         ti;
    CV_TypeIndexSource   ti_source;
  } HashStack;

  // set up root frame
  U32        root_obj_idx = lnk_leaf_ref_obj_idx(root_leaf_ref);
  CV_DebugT *root_debug_t = &input->debug_t_arr[root_obj_idx];
  HashStack *root_frame = push_array(temp.arena, HashStack, 1);
  root_frame->leaf_ref     = root_leaf_ref;
  root_frame->ti_offs      = root_ti_offs;
  root_frame->ti_next      = 0;
  root_frame->ti_count     = cv_ti_offsets_count(&root_ti_offs);
  root_frame->leaf         = lnk_cv_leaf_from_leaf_ref(temp.arena, decode_window, input, lnk_leaf_ref_obj_idx(root_leaf_ref), lnk_leaf_ref_leaf_idx(root_leaf_ref));
  root_frame->ti_source    = cv_type_index_source_from_leaf_kind(root_frame->leaf.kind);
  root_frame->ti           = cv_ti_from_leaf_idx(root_debug_t, root_frame->ti_source, lnk_leaf_ref_leaf_idx(root_leaf_ref));

  HashStack *stack = root_frame;
  while (stack) {
    while (stack->ti_next < stack->ti_count) {
      CV_TiOff ti_info = cv_ti_offset_at(&stack->ti_offs, stack->ti_next);

      // advance iterator
      stack->ti_next += 1;

      // get type index info
      CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(stack->leaf.data, ti_info.offset, sizeof(*ti_ptr));
      CV_TypeIndex  ti     = memory_read32(ti_ptr);

      // skip out of bounds indices
      if ( ! contains_1u64(input->debug_t_arr[root_obj_idx].ti_ranges[ti_info.source], ti)) { continue; }

      // skip hashed types
      LNK_LeafRef leaf_ref = lnk_leaf_ref_from_ti(input, root_obj_idx, ti_info.source, ti);
      U32         obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
      U32         leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
      if (input->debug_h_arr[obj_idx].v[leaf_idx] != 0) { continue; }
      input->debug_h_arr[obj_idx].v[leaf_idx] = 1;

      // recurse down to sub types
      HashStack *frame = push_array(temp.arena, HashStack, 1);
      frame->leaf_ref     = leaf_ref;
      frame->leaf         = lnk_cv_leaf_from_leaf_ref(temp.arena, decode_window, input, obj_idx, leaf_idx);
      frame->ti_offs      = cv_leaf_ti_offsets(temp.arena, frame->leaf.kind, frame->leaf.data);
      frame->ti_next      = 0;
      frame->ti_count     = cv_ti_offsets_count(&frame->ti_offs);
      frame->ti           = ti;
      frame->ti_source    = ti_info.source;
      SLLStackPush(stack, frame);
      break;
    }

    // no more type indices, pop frame
    if (stack->ti_next >= stack->ti_count) {
      // deep hashing only runs on pseudo objs (type servers / .ifc blobs) and never leaves the
      // root obj, so no journal arena is needed (in-place rewrite path)
      lnk_hash_cv_leaf(input, 0, stack->leaf_ref, stack->leaf, stack->ti_offs, 0);
      SLLStackPop(stack);
    }
  }

  temp_end(temp);
}

// Map a uniformly distributed 64-bit hash into [0, cap) with one multiply-high. Unlike `% cap`,
// this does not require a runtime integer division; hash tables only require a stable uniform
// mapping, not the remainder specifically.
force_inline U64
lnk_hash_range(U64 hash, U64 cap)
{
#if COMPILER_MSVC && ARCH_X64
  U64 high;
  _umul128(hash, cap, &high);
  return high;
#elif (COMPILER_CLANG || COMPILER_GCC) && ARCH_64BIT
  return (U64)(((__uint128_t)hash * (__uint128_t)cap) >> 64);
#else
  U64 hash_lo = (U32)hash, hash_hi = hash >> 32;
  U64 cap_lo  = (U32)cap,  cap_hi  = cap  >> 32;
  U64 p00 = hash_lo * cap_lo;
  U64 p01 = hash_lo * cap_hi;
  U64 p10 = hash_hi * cap_lo;
  U64 p11 = hash_hi * cap_hi;
  U64 carry = ((p00 >> 32) + (U32)p01 + (U32)p10) >> 32;
  return p11 + (p01 >> 32) + (p10 >> 32) + carry;
#endif
}

internal CV_TypeIndex
lnk_assigned_ti_hash_search(LNK_AssignedTiHash *ht, LNK_CodeViewInput *input, LNK_LeafRef leaf_ref)
{
  CV_DebugH *debug_h  = &input->debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)];
  U64        hash     = debug_h->v[lnk_leaf_ref_leaf_idx(leaf_ref)];
  U64        best_idx = lnk_hash_range(hash, ht->cap);
  U64        idx      = best_idx;
  do {
    U8 *entry = ht->v + idx * LNK_ASSIGNED_TI_ENTRY_SIZE;
    CV_TypeIndex ti = memory_read32(entry + LNK_ASSIGNED_TI_TI_OFF);
    if (ti == 0) { break; }
    if (memory_read64(entry + LNK_ASSIGNED_TI_HASH_OFF) == hash) { return ti; }
    idx = (idx + 1) == ht->cap ? 0 : (idx + 1);
  } while (idx != best_idx);

  return 0;
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task    = raw_task;
  U32             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  for EachIndex(leaf_idx, debug_t->count) {
    Temp         temp    = temp_begin(task->fixed_arenas[worker_id]);
    CV_Leaf      leaf    = lnk_cv_leaf_from_leaf_ref(temp.arena, &task->decode_windows[worker_id], task->input, obj_idx, leaf_idx);
    CV_TiOffsets ti_offs = cv_leaf_ti_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf(task->input, arena, lnk_leaf_ref_make(obj_idx, leaf_idx), leaf, ti_offs, 1);
    temp_end(temp);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_hash_debug_t_deep_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  B32 is_ifc_blob = task->input->has_ifc_redirects && contains_1u64(task->input->ifc_obj_range, obj_idx);
  for EachIndex(leaf_idx, debug_t->count) {
    if (task->input->debug_h_arr[obj_idx].v[leaf_idx] != 0) { continue; }
    if (is_ifc_blob && cv_debug_t_get_leaf_kind(debug_t, leaf_idx) == CV_LeafKind_NOTYPE) { continue; } // blob $T is arena-backed + mutated in place -- raw read is post-rewrite
    Temp         temp    = temp_begin(task->fixed_arenas[worker_id]);
    CV_Leaf      leaf    = lnk_cv_leaf_from_leaf_ref(temp.arena, &task->decode_windows[worker_id], task->input, obj_idx, leaf_idx);
    CV_TiOffsets ti_offs = cv_leaf_ti_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_hash_cv_leaf_deep(temp.arena, &task->decode_windows[worker_id], task->input, lnk_leaf_ref_make(obj_idx, leaf_idx), ti_offs);
    temp_end(temp);
  }
  ProfEnd();
}

// Deterministic sampling for the unique-leaf estimator: process every K-th leaf POSITION per obj.
// Position-based (leaf_idx % K), never value-based, so the sampled set -- and therefore the
// estimate and the table caps -- is a pure function of the input, schedule-independent. The
// dominant duplication pattern is whole-stream duplication (the same PCH/type-server leaf sequence
// repeated across objs), where a unique hash sits at the SAME position in every copy: the sampled
// distinct count then scales ~1/K, which LNK_ESTIMATE_SAMPLE_SCALE compensates for. The scale is
// calibrated (see the estimate block in lnk_merge_types); an undershoot is caught by the existing
// deterministic overflow-retry at total-based caps, an overshoot is clamped by Min(fallback cap).
//
// SCALE calibration: sampled-distinct is between distinct (fully position-scattered duplication)
// and distinct/K (whole-stream duplication or unique-heavy input), so the true ratio is in [1, K].
// SCALE * 1.9 (the downstream safety factor) must cover the worst-case ratio K to keep the
// overflow-retry off for every duplication pattern: SCALE = 5.0 gives 5.0*1.9 = 9.5 >= K = 8
// (1.19x margin over the bound, which also absorbs linear-counting noise). Measured on the FN
// editor-scale link: ratio 3.99 (TPI) / 6.13 (IPI); SCALE = 5.0 reproduces the unsampled
// estimator's caps exactly (64M/16M) at load factors 0.35/0.39.
#define LNK_ESTIMATE_SAMPLE_STRIDE 8
#define LNK_ESTIMATE_SAMPLE_SCALE  5.0

internal
THREAD_POOL_TASK_FUNC(lnk_estimate_unique_leaves_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  CV_DebugH      *debug_h = &task->input->debug_h_arr[obj_idx];
  // same prune rule as lnk_leaf_dedup_task: NOTYPE'd IFC blob leaves were never hashed and are
  // never inserted, so they must not contribute to the estimate either
  B32 is_ifc_blob = task->input->has_ifc_redirects && contains_1u64(task->input->ifc_obj_range, obj_idx);
  // P4: raw views hold pre-rewrite bytes for journaled real-obj leaves -- overlay LF_NOTYPE
  // via the per-obj journal bitmap (0 for ~all objs; one register test per leaf)
  U64 *notype_bm  = 0;
  U64  notype_cap = 0;
  if (obj_idx < task->input->obj_count) {
    notype_bm  = task->input->notype_journal[obj_idx].bitmap;
    notype_cap = task->input->notype_journal[obj_idx].bit_cap;
  }
  for (U64 leaf_idx = 0; leaf_idx < debug_t->count; leaf_idx += LNK_ESTIMATE_SAMPLE_STRIDE) {
    CV_LeafKind    kind   = cv_debug_t_get_leaf_kind(debug_t, leaf_idx);
    if (notype_bm && leaf_idx < notype_cap && ((notype_bm[leaf_idx >> 6] >> (leaf_idx & 63)) & 1)) { kind = CV_LeafKind_NOTYPE; }
    if (is_ifc_blob && kind == CV_LeafKind_NOTYPE) { continue; }
    CV_TypeIndexSource leaf_source = cv_type_index_source_from_leaf_kind(kind);
    U64                bit_idx     = debug_h->v[leaf_idx] & (task->estimate_bitmap_bits[leaf_source] - 1);
    U32               *word        = &task->estimate_bitmap[leaf_source][bit_idx / 32];
    U32                bit         = 1u << (bit_idx % 32);
    // atomic OR is commutative -> final bitmap contents are schedule-independent (deterministic);
    // pre-check skips the interlocked op for already-set bits (the common case on dup-heavy input)
    if ((ins_atomic_u32_eval(word) & bit) == 0) { ins_atomic_u32_or(word, bit); }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_populate_leaf_ht)
{
  LNK_MergeTypes *task = raw_task;

  U64        obj_idx = task->pop_obj_idx;
  CV_DebugT *debug_t = &task->input->debug_t_arr[task->pop_obj_idx];
  CV_DebugH *debug_h = &task->input->debug_h_arr[task->pop_obj_idx];

  for EachInRange(leaf_idx, task->pop_range[task_id]) {
    // another worker overflowed an estimate-sized table -- the whole dedup result is discarded
    // and retried with the total-based caps, so bail out early
    if (ins_atomic_u32_eval(&task->leaf_ht_overflow) != 0) { break; }

    LNK_LeafRef leaf_ref = lnk_leaf_ref_make(obj_idx, leaf_idx);
    B32 is_inserted_or_updated = 1;

    // pop obj is a type-server pseudo obj: arena-backed, mutated in place, never journaled --
    // the raw header read is the post-rewrite kind (original code path)
    CV_LeafKind         kind        = cv_debug_t_get_leaf_kind(debug_t, leaf_idx);
    CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(kind);                 // leaf kind -> type stream
    LNK_LeafHashTable  *leaf_ht     = &task->leaf_ht_arr[leaf_source];                           // type stream -> hash table
    U64                 hash        = debug_h->v[leaf_idx];                                     // leaf ref -> hash
    U64                 best_idx    = hash & (leaf_ht->cap - 1);                                // hash -> bucket index
    LNK_LeafRef         tagged      = lnk_leaf_bucket_tag(leaf_ref, hash);
    U64                 idx         = best_idx;

    do {
      LNK_LeafRef curr_tagged = ins_atomic_u64_eval(&leaf_ht->bucket_arr[idx]);
      if (curr_tagged == LNK_LEAF_REF_NULL) {
        LNK_LeafRef cmp = ins_atomic_u64_eval_cond_assign(&leaf_ht->bucket_arr[idx], tagged, curr_tagged);
        if (cmp == curr_tagged) {
          goto exit;
        }
      }

      // advance to next bucket
      idx = (idx + 1) & (leaf_ht->cap - 1);
    } while (idx != best_idx);
    is_inserted_or_updated = 0;
    exit:;
    if (!is_inserted_or_updated && leaf_source != CV_TypeIndexSource_NULL) {
      // TPI/IPI table is full (estimate undershot) -- flag for a deterministic retry with the
      // total-based caps. the NULL-source table is deliberately undersized and silently drops
      // leaves that do not fit (pre-existing behavior; they are never emitted).
      ins_atomic_u32_eval_assign(&task->leaf_ht_overflow, 1);
      break;
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_leaf_dedup_task)
{
  LNK_MergeTypes *task    = raw_task;
  U64             obj_idx = task->indices.v[task_id];
  CV_DebugT      *debug_t = &task->input->debug_t_arr[obj_idx];
  CV_DebugH      *debug_h = &task->input->debug_h_arr[obj_idx];
  B32             is_ifc_blob = task->input->has_ifc_redirects && contains_1u64(task->input->ifc_obj_range, obj_idx);
  
  ProfBeginDynamic("dedup in obj 0x%llx (%.*s) leaf count %llu", obj_idx, str8_varg(task->input->obj_arr[obj_idx]->path), debug_t->count);

  // P4: raw views hold pre-rewrite bytes for journaled real-obj leaves -- overlay LF_NOTYPE via
  // the per-obj journal bitmap (0 for ~all objs). Blob objs are pseudo (in-place rewrites), so
  // their skip below keeps the plain raw read.
  U64 *notype_bm  = 0;
  U64  notype_cap = 0;
  if (obj_idx < task->input->obj_count) {
    notype_bm  = task->input->notype_journal[obj_idx].bitmap;
    notype_cap = task->input->notype_journal[obj_idx].bit_cap;
  }

  for EachIndex(leaf_idx, debug_t->count) {
    // another worker overflowed an estimate-sized table -- the whole dedup result is discarded
    // and retried with the total-based caps, so bail out early
    if (ins_atomic_u32_eval(&task->leaf_ht_overflow) != 0) { break; }
    B32 is_inserted_or_updated = 1;

    LNK_LeafRef         leaf_ref    = lnk_leaf_ref_make(obj_idx, leaf_idx);
    CV_LeafKind         kind        = cv_debug_t_get_leaf_kind(debug_t, leaf_idx);
    if (is_ifc_blob && kind == CV_LeafKind_NOTYPE) { continue; }
    if (notype_bm && leaf_idx < notype_cap && ((notype_bm[leaf_idx >> 6] >> (leaf_idx & 63)) & 1)) { kind = CV_LeafKind_NOTYPE; }
    CV_TypeIndexSource  leaf_source = cv_type_index_source_from_leaf_kind(kind);                 // leaf kind -> type stream
    LNK_LeafHashTable  *leaf_ht     = &task->leaf_ht_arr[leaf_source];                           // type stream -> hash table
    U64                 hash        = debug_h->v[leaf_idx];                                     // leaf ref -> hash
    U64                 best_idx    = hash & (leaf_ht->cap - 1);                                // hash -> bucket index
    LNK_LeafRef         tagged      = lnk_leaf_bucket_tag(leaf_ref, hash);
    U64                 idx         = best_idx;

    do {
      // load leaf ref
      LNK_LeafRef curr_tagged = ins_atomic_u64_eval(&leaf_ht->bucket_arr[idx]);

      while (curr_tagged == LNK_LEAF_REF_NULL ||
             ((curr_tagged & LNK_LEAF_BUCKET_TAG_MASK) == (tagged & LNK_LEAF_BUCKET_TAG_MASK) &&
              lnk_hash_from_leaf_ref(task->input, lnk_leaf_bucket_untag(curr_tagged)) == hash)) {
        LNK_LeafRef curr = curr_tagged == LNK_LEAF_REF_NULL ? LNK_LEAF_REF_NULL : lnk_leaf_bucket_untag(curr_tagged);
        // exit if leaf ref is not recent
        if (curr != LNK_LEAF_REF_NULL && lnk_leaf_ref_compare(leaf_ref, curr) >= 0) {
          goto exit;
        }

        // try to update the bucket
        LNK_LeafRef cmp = ins_atomic_u64_eval_cond_assign(&leaf_ht->bucket_arr[idx], tagged, curr_tagged);
        if (cmp == curr_tagged) {
          goto exit;
        }

        // another thread updated the bucket -- retry
        curr_tagged = cmp;
      }

      // advance to next bucket
      idx = (idx + 1) & (leaf_ht->cap - 1);
    } while (idx != best_idx);
    
    is_inserted_or_updated = 0;
    exit:;
    if (!is_inserted_or_updated && leaf_source != CV_TypeIndexSource_NULL) {
      // TPI/IPI table is full (estimate undershot) -- flag for a deterministic retry with the
      // total-based caps. the NULL-source table is deliberately undersized and silently drops
      // leaves that do not fit (pre-existing behavior; they are never emitted).
      ins_atomic_u32_eval_assign(&task->leaf_ht_overflow, 1);
      break;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_present_buckets_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task = raw_task;
  LNK_LeafHashTable *ht   = &task->leaf_ht_arr[task->ti_source];

  for EachInRange(bucket_idx, task->ranges[task_id]) {
    if (ht->bucket_arr[bucket_idx] != LNK_LEAF_REF_NULL) {
      task->counts[task->ti_source][task_id] += 1;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_get_present_buckets_task)
{
  ProfBeginFunction();

  LNK_MergeTypes *task = raw_task;

  U64                cursor           = task->offsets[task->ti_source][task_id];
  LNK_LeafHashTable *ht               = &task->leaf_ht_arr[task->ti_source];
  LNK_LeafRefArray   unique_leaf_refs = task->unique_leaf_refs_arr[task->ti_source];

  for EachInRange(bucket_idx, task->ranges[task_id]) {
    if (ht->bucket_arr[bucket_idx] != LNK_LEAF_REF_NULL) {
      LNK_LeafRef ref = lnk_leaf_bucket_untag(ht->bucket_arr[bucket_idx]);
      unique_leaf_refs.v[cursor++] = ref;

      // TPI/IPI materialization is the next compressed-payload consumer. Claim each segment
      // once while the winner is already hot in this extraction pass. A leaf can straddle a
      // segment boundary, so claim the full raw range rather than only its first byte.
      if (task->winner_segment_bitmap && task->ti_source != CV_TypeIndexSource_NULL &&
          lnk_leaf_ref_obj_idx(ref) < task->input->obj_count) {
        U32 obj_idx = lnk_leaf_ref_obj_idx(ref);
        U32 leaf_idx = lnk_leaf_ref_leaf_idx(ref);
        LNK_Obj *obj = task->input->obj_arr[obj_idx];
        CV_DebugT *debug_t = &task->input->debug_t_arr[obj_idx];
        U32 segment_size = lnk_compressed_obj_segment_size(obj->compressed_obj);
        if (segment_size && (debug_t->sidecar_sizes || debug_t->sidecar_packed)) {
          U64 raw_min = debug_t->sidecar_raw_base + cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
          U64 raw_max = raw_min + cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx);
          U32 first = (U32)(raw_min / segment_size);
          U32 opl   = (U32)CeilIntegerDiv(raw_max, segment_size);
          U32 count = lnk_compressed_obj_segment_count(obj->compressed_obj);
          opl = Min(opl, count);
          for (U32 segment_idx = first; segment_idx < opl; ++segment_idx) {
            U64 global_idx = task->winner_segment_offsets[obj_idx] + segment_idx;
            U64 *word = &task->winner_segment_worker_bitmaps[task_id * task->winner_segment_word_count +
                                                              (global_idx >> 6)];
            U64 bit = (U64)1 << (global_idx & 63);
            *word |= bit;
          }
        }
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_assign_type_indices_task)
{
  LNK_MergeTypes *task  = raw_task;

  CV_TypeIndexSource  ti_source         = task->ti_source;
  LNK_LeafRefArray    unique_leaf_refs  = task->unique_leaf_refs_arr[ti_source];
  CV_TypeIndex        min_type_index    = task->min_type_indices[ti_source];
  LNK_AssignedTiHash *assigned          = &task->assigned_ti_arr[ti_source];
  CV_DebugH          *debug_h_arr       = task->input->debug_h_arr;

  for EachInRange(i, task->ranges[task_id]) {
    LNK_LeafRef   leaf_ref   = unique_leaf_refs.v[i];
    CV_TypeIndex  type_index = min_type_index + i;

    U64 hash     = debug_h_arr[lnk_leaf_ref_obj_idx(leaf_ref)].v[lnk_leaf_ref_leaf_idx(leaf_ref)];
    U64 best_idx = lnk_hash_range(hash, assigned->cap);
    U64 idx      = best_idx;

    B32 is_inserted = 0;
    do {
      U8 *entry = assigned->v + idx * LNK_ASSIGNED_TI_ENTRY_SIZE;
      CV_TypeIndex *ti_ptr = (CV_TypeIndex *)(entry + LNK_ASSIGNED_TI_TI_OFF);
      CV_TypeIndex curr_type_index = *ti_ptr;
      if (curr_type_index == 0) {
        CV_TypeIndex cmp_type_index = ins_atomic_u32_eval_cond_assign(ti_ptr, type_index, curr_type_index);
        if (cmp_type_index == curr_type_index) {
          memory_write64(entry + LNK_ASSIGNED_TI_HASH_OFF, hash);
          is_inserted = 1;
          break;
        }
      }
      // advance
      idx = (idx + 1) == assigned->cap ? 0 : (idx + 1);
    } while (idx != best_idx);
    Assert(is_inserted);
  }
}

internal void
lnk_fixup_cv_type_indices(LNK_MergeTypes *ctx, U32 obj_idx, String8 data, CV_TiOffsets ti_offs)
{
  for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&ti_offs); ti_idx < ti_count; ti_idx += 1) {
    CV_TiOff      n      = cv_ti_offset_at(&ti_offs, ti_idx);
    CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(data, n.offset, sizeof(*ti_ptr));
    CV_TypeIndex  ti     = memory_read32(ti_ptr);

    // skip basic types
    if (ti < ctx->input->min_type_indices[n.source]) { continue; }

    LNK_LeafRef  leaf_ref = lnk_leaf_ref_from_ti(ctx->input, obj_idx, n.source, ti);
    CV_TypeIndex final_ti = lnk_assigned_ti_hash_search(&ctx->assigned_ti_arr[n.source], ctx->input, leaf_ref);
    memory_write32(ti_ptr, final_ti);

#if LNK_PARANOID
    if (final_ti == 0) {
      lnk_error_obj(LNK_Error_InvalidTypeIndex, ctx->input->obj_arr[obj_idx], "no itype 0x%x", ti);
    }
#endif
  }
}

// Streaming-ring P2 slice A: upper-bound count of journal entries the ti_offs walk emits.
// Exact for the TI-fixup part -- an entry is emitted iff the raw TI is >= the source's min
// (the "skip basic types" test), which needs no merge-state lookups.
internal U64
lnk_count_cv_type_index_fixups(LNK_MergeTypes *ctx, String8 data, CV_TiOffsets ti_offs)
{
  U64 count = 0;
  for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&ti_offs); ti_idx < ti_count; ti_idx += 1) {
    CV_TiOff      n      = cv_ti_offset_at(&ti_offs, ti_idx);
    CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(data, n.offset, sizeof(*ti_ptr));
    if (memory_read32(ti_ptr) >= ctx->input->min_type_indices[n.source]) { count += 1; }
  }
  return count;
}

// 8B/16B entry emit + decode helpers. `off` is relative to the run's node base; narrow runs
// are chosen up front (node size < 2GiB guarantees every off fits off:31), so the narrow
// branch never truncates.
force_inline void
lnk_debug_s_patch_emit(LNK_DebugSPatchArray *journal, U64 off, U32 value, U32 size)
{
  if (journal->is_wide) {
    ((LNK_DebugSPatchWide *)journal->v)[journal->count++] = (LNK_DebugSPatchWide){ .off = off, .value = value, .size = size };
  } else {
    Assert(off < (1ull << 31));
    ((LNK_DebugSPatch *)journal->v)[journal->count++] = (LNK_DebugSPatch){ .off_w = (U32)((off << 1) | (size == 4 ? 1 : 0)), .value = value };
  }
}

force_inline U64
lnk_debug_s_patch_off_at(LNK_DebugSPatchArray *journal, U64 k)
{
  return journal->is_wide ? ((LNK_DebugSPatchWide *)journal->v)[k].off
                          : (U64)(((LNK_DebugSPatch *)journal->v)[k].off_w >> 1);
}

force_inline U32
lnk_debug_s_patch_value_at(LNK_DebugSPatchArray *journal, U64 k)
{
  return journal->is_wide ? ((LNK_DebugSPatchWide *)journal->v)[k].value
                          : ((LNK_DebugSPatch *)journal->v)[k].value;
}

// journal-emitting twin of lnk_fixup_cv_type_indices: identical TI resolution (assigned-TI
// hash search over the merge result), but the write is RECORDED instead of applied -- the $S
// bytes stay pre-fixup until the per-obj replay at module write. `base` = the run's node base
// (entries store node-relative offsets).
internal void
lnk_journal_cv_type_index_fixups(LNK_MergeTypes *ctx, U32 obj_idx, String8 data, CV_TiOffsets ti_offs, LNK_DebugSPatchArray *journal, U8 *base)
{
  for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&ti_offs); ti_idx < ti_count; ti_idx += 1) {
    CV_TiOff      n      = cv_ti_offset_at(&ti_offs, ti_idx);
    CV_TypeIndex *ti_ptr = str8_deserial_get_raw_ptr(data, n.offset, sizeof(*ti_ptr));
    CV_TypeIndex  ti     = memory_read32(ti_ptr);

    // skip basic types
    if (ti < ctx->input->min_type_indices[n.source]) { continue; }

    LNK_LeafRef  leaf_ref = lnk_leaf_ref_from_ti(ctx->input, obj_idx, n.source, ti);
    CV_TypeIndex final_ti = lnk_assigned_ti_hash_search(&ctx->assigned_ti_arr[n.source], ctx->input, leaf_ref);
    lnk_debug_s_patch_emit(journal, (U64)((U8 *)ti_ptr - base), final_ti, 4);

#if LNK_PARANOID
    if (final_ti == 0) {
      lnk_error_obj(LNK_Error_InvalidTypeIndex, ctx->input->obj_arr[obj_idx], "no itype 0x%x", ti);
    }
#endif
  }
}

// Fuses the old lnk_cv_patcher_symbols_task (symbol-record TI fixup) and lnk_fixup_symbols_task
// (*_ID kind rewrite + itype -> FUNC_ID/MFUNC_ID itype resolve) into one journal-building walk
// per symbol input. Runs AFTER the materialize pass so the itype resolve reads the fixed-up
// merged IPI leaf copies -- exactly what the old standalone pass consumed. The old second pass
// read proc32->itype back from memory AFTER the first pass patched it; here that value is the
// resolved TI recorded for the record's itype slot (raw bytes when the slot wasn't journaled:
// basic types, MIPS/IA64 kinds cv_symbol_ti_offsets has no entry for). Emitting BOTH itype
// entries in order (resolved IPI TI, then the FUNC_ID itype) makes the sequential replay land
// on the same end-state bytes for every branch, including the early-out edge cases.
internal
THREAD_POOL_TASK_FUNC(lnk_journal_symbol_fixups_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task = raw_task;

  Arena        *journal_arena  = task->journal_arena->v[worker_id];
  U64           leaf_count_ipi = task->result.count    [CV_TypeIndexSource_IPI];
  U8          **leaf_arr_ipi   = task->result.v        [CV_TypeIndexSource_IPI];
  CV_TypeIndex  min_ti_ipi     = task->min_type_indices[CV_TypeIndexSource_IPI];

  Rng1U64 range = task->input->symbol_patch_task[task_id].input_range;
  for EachInRange(i, range) {
    LNK_SymbolInput symbols = task->input->symbol_inputs[i];

    // The journal builder makes two immediate linear passes over each Symbols subsection.
    // Keeping those reads behind the shared faulting view lets concurrently scheduled objs
    // evict one another between passes.  A subsection-local copy streams every compressed
    // segment once through the worker's decode window, then both parser passes hit ordinary
    // committed memory.  The fixed arena temp is rewound for every input, so peak storage is
    // bounded by one Symbols subsection per active worker rather than the corpus size.
    Temp symbols_temp = temp_begin(task->fixed_arenas[worker_id]);
    LNK_Obj *obj = task->input->obj_arr[symbols.obj_idx];
    if (obj->compressed_obj && symbols.raw_symbols.size) {
      U8 *copy = push_array_no_zero(symbols_temp.arena, U8, symbols.raw_symbols.size);
      if (lnk_compressed_obj_copy_string(obj->compressed_obj, symbols.raw_symbols, copy,
                                         &task->decode_windows[worker_id])) {
        symbols.raw_symbols = str8(copy, symbols.raw_symbols.size);
      }
    }

    // upper-bound pass: TI entries (exact) + kind rewrite / itype slots for *_ID records
    U64 cap = 0;
    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);
      cap += lnk_count_cv_type_index_fixups(task, symbol.data, cv_symbol_ti_offsets(symbol.kind, symbol.data));
      switch (symbol.kind) {
      case CV_SymKind_PROC_ID_END:    cap += 1; break;
      case CV_SymKind_LPROC32_ID:
      case CV_SymKind_GPROC32_ID:
      case CV_SymKind_LPROC32_DPC_ID:
      case CV_SymKind_LPROCMIPS_ID:
      case CV_SymKind_GPROCMIPS_ID:
      case CV_SymKind_LPROCIA64_ID:
      case CV_SymKind_GPROCIA64_ID:   cap += 2; break;
      default: break;
      }
    }

    U8 *base = symbols.raw_symbols.str;

    LNK_DebugSPatchArray *journal = &task->input->debug_s_sym_fixups[i];
    journal->is_wide = (symbols.raw_symbols.size >> 31) != 0; // narrow off:31 covers the whole node otherwise
    journal->v       = journal->is_wide ? (void *)push_array_no_zero(journal_arena, LNK_DebugSPatchWide, cap)
                                        : (void *)push_array_no_zero(journal_arena, LNK_DebugSPatch,     cap);
    journal->count   = 0;

    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);

      CV_TiOffsets ti_offs           = cv_symbol_ti_offsets(symbol.kind, symbol.data);
      U64          rec_journal_start = journal->count;
      lnk_journal_cv_type_index_fixups(task, symbols.obj_idx, symbol.data, ti_offs, journal, base);

      // convert symbol to final type
      CV_SymKind *sym_kind_ptr = cv_kind_ptr_from_symbol(symbol);
      CV_SymKind  new_kind     = CV_SymKind_END;
      switch (symbol.kind) {
      case CV_SymKind_PROC_ID_END: {
        lnk_debug_s_patch_emit(journal, (U64)((U8 *)sym_kind_ptr - base), CV_SymKind_END, 2);
      } break;

      case CV_SymKind_LPROC32_ID:     new_kind = CV_SymKind_LPROC32;     goto fixup_id;
      case CV_SymKind_GPROC32_ID:     new_kind = CV_SymKind_GPROC32;     goto fixup_id;
      case CV_SymKind_LPROC32_DPC_ID: new_kind = CV_SymKind_LPROC32_DPC; goto fixup_id;
      case CV_SymKind_LPROCMIPS_ID:   new_kind = CV_SymKind_LPROCMIPS;   goto fixup_id;
      case CV_SymKind_GPROCMIPS_ID:   new_kind = CV_SymKind_GPROCMIPS;   goto fixup_id;
      case CV_SymKind_LPROCIA64_ID:   new_kind = CV_SymKind_LPROCIA64;   goto fixup_id;
      case CV_SymKind_GPROCIA64_ID:   new_kind = CV_SymKind_GPROCIA64;   goto fixup_id;
      fixup_id:; {
        lnk_debug_s_patch_emit(journal, (U64)((U8 *)sym_kind_ptr - base), new_kind, 2);

        CV_SymProc32 *proc32 = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*proc32));

        // effective post-TI-fixup itype (what the old pass read back from patched memory)
        U64          itype_off = (U64)((U8 *)&proc32->itype - base);
        CV_TypeIndex itype     = proc32->itype;
        for (U64 k = rec_journal_start; k < journal->count; k += 1) {
          if (lnk_debug_s_patch_off_at(journal, k) == itype_off) { itype = lnk_debug_s_patch_value_at(journal, k); break; }
        }

        if (itype < min_ti_ipi) {
          // TODO: in some cases destructors don't have a type, need a repro
          break;
        }

        if ((itype - min_ti_ipi) > leaf_count_ipi) {
          Assert(0 && "TODO: error handle corrupted type index");
          break;
        }

        U64     leaf_idx  = itype - min_ti_ipi;
        String8 leaf_data = str8(leaf_arr_ipi[leaf_idx], max_U64);

        CV_Leaf leaf;
        if (cv_read_leaf(leaf_data, 0, 1, &leaf) == 0) { InvalidPath; }

        U64 min_leaf_size = cv_header_struct_size_from_leaf_kind(leaf.kind);
        if (min_leaf_size > leaf.data.size) { Assert(!"TODO: error handle corrupt leaf"); break; }

        if (leaf.kind == CV_LeafKind_FUNC_ID) {
          CV_LeafFuncId *func_id = str8_deserial_get_raw_ptr(leaf.data, 0, sizeof(*func_id));
          lnk_debug_s_patch_emit(journal, itype_off, func_id->itype, 4);
        } else if (leaf.kind == CV_LeafKind_MFUNC_ID) {
          CV_LeafMFuncId *mfunc_id = str8_deserial_get_raw_ptr(leaf.data, 0, sizeof(*mfunc_id));
          lnk_debug_s_patch_emit(journal, itype_off, mfunc_id->itype, 4);
        } else {
          Assert(!"TODO: erorr handle unexpected leaf type");
          break;
        }
      } break;

      default: break;
      }
    }
    Assert(journal->count <= cap);
    temp_end(symbols_temp);
  }
  ProfEnd();
}

// journal-building replacement for the old lnk_cv_patcher_inlines_task (per obj)
internal
THREAD_POOL_TASK_FUNC(lnk_journal_inline_fixups_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task          = raw_task;
  U64             obj_idx       = task_id;
  String8List     inlinee_lines = cv_sub_section_from_debug_s(task->input->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  Arena          *fixed_arena   = task->fixed_arenas[worker_id];
  Arena          *journal_arena = task->journal_arena->v[worker_id];

  // exact count; a node >= 2GiB forces the obj's whole run set to wide entries
  U64 cap     = 0;
  B32 is_wide = 0;
  for EachNode(inline_data_n, String8Node, inlinee_lines.first) {
    Temp temp = temp_begin(fixed_arena);
    CV_TiOffsets ti_offs = cv_inlinee_ti_offsets(temp.arena, inline_data_n->string);
    cap += lnk_count_cv_type_index_fixups(task, inline_data_n->string, ti_offs);
    is_wide |= (inline_data_n->string.size >> 31) != 0;
    temp_end(temp);
  }

  LNK_DebugSInlineJournal *journal = &task->input->debug_s_inline_fixups[obj_idx];
  journal->patches.is_wide = is_wide;
  journal->patches.v       = is_wide ? (void *)push_array_no_zero(journal_arena, LNK_DebugSPatchWide, cap)
                                     : (void *)push_array_no_zero(journal_arena, LNK_DebugSPatch,     cap);
  journal->patches.count   = 0;
  journal->node_counts     = push_array_no_zero(journal_arena, U32, inlinee_lines.node_count ? inlinee_lines.node_count : 1);

  U64 node_idx = 0;
  for EachNode(inline_data_n, String8Node, inlinee_lines.first) {
    Temp temp = temp_begin(fixed_arena);
    U64 run_start = journal->patches.count;
    CV_TiOffsets ti_offs = cv_inlinee_ti_offsets(temp.arena, inline_data_n->string);
    lnk_journal_cv_type_index_fixups(task, obj_idx, inline_data_n->string, ti_offs, &journal->patches, inline_data_n->string.str);
    journal->node_counts[node_idx++] = (U32)(journal->patches.count - run_start);
    temp_end(temp);
  }
  Assert(journal->patches.count == cap);
  ProfEnd();
}

internal void
lnk_apply_debug_s_patch_run(U8 *base, LNK_DebugSPatchArray *arr, U64 lo, U64 opl)
{
  if (arr->is_wide) {
    LNK_DebugSPatchWide *v = arr->v;
    for (U64 k = lo; k < opl; k += 1) {
      if (v[k].size == 4) { memory_write32(base + v[k].off, v[k].value); }
      else                { memory_write16(base + v[k].off, (U16)v[k].value); }
    }
  } else {
    LNK_DebugSPatch *v = arr->v;
    for (U64 k = lo; k < opl; k += 1) {
      U8 *ptr = base + (v[k].off_w >> 1);
      if (v[k].off_w & 1) { memory_write32(ptr, v[k].value); }
      else                { memory_write16(ptr, (U16)v[k].value); }
    }
  }
}

// Replays the obj's deferred $S TI/kind fixups (journaled in lnk_merge_types). Entries write
// only into the obj's own $S backing bytes (its patched section copies / raw-mapped sections;
// $S is never shared across objs, unlike $T PCH refs), so this is safe inside any per-obj
// parallel loop and produces the same bytes regardless of schedule. Write order == journal
// emission order: symbol inputs in index order (= Symbols data_list node order), then the
// InlineeLines node runs in data_list order.
internal void
lnk_apply_debug_s_fixups_for_obj(LNK_CodeViewInput *cv, U64 obj_idx)
{
  if (!cv->has_debug_s_fixup_journal) { return; }

  for (U64 i = cv->debug_s_sym_fixup_offsets[obj_idx], opl = cv->debug_s_sym_fixup_offsets[obj_idx+1]; i < opl; i += 1) {
    Assert(cv->symbol_inputs[i].obj_idx == obj_idx);
    lnk_apply_debug_s_patch_run(cv->symbol_inputs[i].raw_symbols.str, &cv->debug_s_sym_fixups[i], 0, cv->debug_s_sym_fixups[i].count);
  }

  LNK_DebugSInlineJournal *inline_journal = &cv->debug_s_inline_fixups[obj_idx];
  if (inline_journal->patches.count > 0) {
    String8List inlinee_lines = cv_sub_section_from_debug_s(cv->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
    U64 node_idx = 0, cursor = 0;
    for EachNode(inline_data_n, String8Node, inlinee_lines.first) {
      U64 run = inline_journal->node_counts[node_idx++];
      lnk_apply_debug_s_patch_run(inline_data_n->string.str, &inline_journal->patches, cursor, cursor + run);
      cursor += run;
    }
    Assert(cursor == inline_journal->patches.count);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_apply_debug_s_fixups_task)
{
  LNK_CodeViewInput *cv = raw_task;
  lnk_apply_debug_s_fixups_for_obj(cv, task_id);
}

// Eager whole-input replay for configs that consume fixed-up $S bytes before (or without)
// the module-write pass: /OPT:GCTYPES reads + rewrites $S type indices right after the merge,
// and a /PDBSTRIPPED-only build re-walks $S without ever writing modules. Consumes the
// journal -- the module-write replay is skipped afterwards.
// Consumes + releases the journal: drops every reference (entry arrays, per-input/per-obj
// headers, offsets table all live inside the DEBUG_S_FIXUP_JOURNAL arenas), then hands the
// arena set to the background reaper. Idempotent; no-op when the journal was never built
// (SkipSymbolTypeFixup / stripped cv) or already consumed.
internal void
lnk_release_debug_s_fixup_journal(LNK_CodeViewInput *cv)
{
  if (cv->debug_s_fixup_journal_arenas == 0) { return; }
  cv->debug_s_sym_fixups        = 0;
  cv->debug_s_inline_fixups     = 0;
  cv->debug_s_sym_fixup_offsets = 0;
  cv->has_debug_s_fixup_journal = 0;
  if (g_arena_reaper_thread.u64[0] != 0) { thread_join(g_arena_reaper_thread, max_U64); }
  g_arena_reaper_thread = thread_launch(lnk_tp_arena_release_thread, cv->debug_s_fixup_journal_arenas);
  cv->debug_s_fixup_journal_arenas = 0;
}

internal void
lnk_apply_debug_s_fixups_eager(TP_Context *tp, LNK_CodeViewInput *cv)
{
  if (!cv->has_debug_s_fixup_journal) { return; }
  ProfBegin("Apply $S Fixups (eager)");
  tp_for_parallel(tp, 0, cv->obj_count, lnk_apply_debug_s_fixups_task, cv);
  lnk_release_debug_s_fixup_journal(cv); // consumed -- module write skips replay AND release
  ProfEnd();
}

// high-water mark of a single obj's windowed $S bytes (telemetry: bounds the per-worker
// window arena growth; reported under /RAD_LOG:Debug at the end of Write Modules)
global U64 g_debug_s_window_hwm = 0;

// ===== Streaming-ring P3.3: the window ==========================================================
// Materializes an obj's parsed .debug$S subsections as CONSUMABLE bytes in `arena` (a per-worker
// scratch reset per obj -- the "window"), without ever writing to the raw mapped input views and
// without any persistent patched copy:
//   - every source section a provenance record references is either aliased RAW (no relocs, no
//     in-window mutation needed) or COPIED into the window and reloc-patched there via
//     lnk_obj_apply_relocs_to_buffer (the exact routine the image-build patcher uses -- relocs,
//     symbol tables and the image section table are immutable by module-write time, so the bytes
//     are identical to the retired image-time patch-on-copy);
//   - Symbols/InlineeLines sections are always windowed while the $S fixup journal is alive
//     (replay writes into the window, never a mapped view), and the obj's journal runs replay
//     here in build order -- entries are absolute writes, so every re-fill of the window replays
//     to identical bytes (relocs are RMW but always start from the immutable raw addend);
//   - synthetic / untracked nodes alias their existing linker-made bytes.
// The returned CV_DebugS carries data_list only (no provenance -- parity walks run against the
// original struct). `symbols_only` limits the fill to the Symbols subsection (epilogue dedup
// re-reads, /PDBSTRIPPED pre-pass). Callers own the arena lifetime; nothing in the result may
// outlive it except aliased raw/synthetic node bytes.
internal CV_DebugS
lnk_obj_window_debug_s(Arena *arena, LNK_CodeViewInput *cv, U64 obj_idx, U64 image_base, COFF_SectionHeader **image_section_table, B32 symbols_only)
{
  LNK_Obj   *obj = cv->obj_arr[obj_idx];
  CV_DebugS *src = &cv->debug_s_arr[obj_idx];
  CV_DebugS  out = {0};
  LNK_CObjDecodeWindow decode_window = {0};

  U64 idx_symbols  = cv_c13_sub_section_idx_from_kind(CV_C13SubSectionKind_Symbols);
  U64 idx_inlinees = cv_c13_sub_section_idx_from_kind(CV_C13SubSectionKind_InlineeLines);

  // decide per-section representation from the sections the prov records reference
  // (0 = unused, 1 = alias raw view, 2 = window copy + relocs)
  U64  sect_count = obj->coff.sections.count_no_null;
  U8  *sect_state = push_array(arena, U8,   sect_count);
  U8 **sect_base  = push_array(arena, U8 *, sect_count);
  for EachElement(k, src->data_list) {
    if (symbols_only && k != idx_symbols) { continue; }
    if (src->prov_list[k].count == 0)     { continue; } // untracked: nodes alias as-is below
    B32 is_journal_kind = (k == idx_symbols || k == idx_inlinees) && cv->has_debug_s_fixup_journal;
    for (CV_DebugSProvNode *prov = src->prov_list[k].first; prov != 0; prov = prov->next) {
      if (prov->is_synthetic || prov->sect_idx == CV_DebugSProvSect_Nil) { continue; }
      U8 state = 1;
      if (is_journal_kind) {
        state = 2;
      } else {
        U64 section_number = (U64)prov->sect_idx + 1;
        COFF_SectionHeader *hdr = &obj->coff.sections.headers[section_number];
        if (lnk_coff_relocs_from_section_header(obj, hdr).count > 0) { state = 2; }
      }
      if (state > sect_state[prov->sect_idx]) { sect_state[prov->sect_idx] = state; }
    }
  }

  // materialize window sections: raw memcpy + reloc application
  COFF_SectionHeader *raw_section_table = (COFF_SectionHeader *)str8_substr(obj->coff.data, obj->coff.header.section_table_range).str;
  U64 window_size = 0;
  for EachIndex(sect_idx, sect_count) {
    if (sect_state[sect_idx] == 0) { continue; }
    U64            section_number = sect_idx + 1;
    LNK_ObjSection section        = lnk_obj_section_from_section_number(obj, section_number);
    String8        raw     = lnk_compressed_obj_direct_range(obj->compressed_obj, section.frange);
    B32 explicitly_decoded = 0;
    if (raw.size == 0 && obj->compressed_obj != 0) {
      U8 *copy = push_array_no_zero(arena, U8, dim_1u64(section.frange));
      if (lnk_compressed_obj_copy_range(obj->compressed_obj, section.frange, copy, &decode_window)) {
        raw = str8(copy, dim_1u64(section.frange));
        explicitly_decoded = 1;
      }
    }
    if (raw.size == 0) { raw = str8_substr(obj->coff.data, section.frange); }
    if (sect_state[sect_idx] == 1) {
      sect_base[sect_idx] = raw.str;
      if (explicitly_decoded) { window_size += raw.size; }
    } else {
      U8 *copy = raw.str;
      if (!explicitly_decoded) {
        copy = push_array_no_zero(arena, U8, raw.size);
        MemoryCopy(copy, raw.str, raw.size);
      }
      lnk_obj_apply_relocs_to_buffer(obj, section_number, section.header, str8(copy, raw.size), image_base, image_section_table);
      sect_base[sect_idx] = copy;
      window_size += raw.size;
    }
  }
  if (window_size > 0) {
    for (U64 hwm = g_debug_s_window_hwm; window_size > hwm; hwm = g_debug_s_window_hwm) {
      ins_atomic_u64_eval_cond_assign(&g_debug_s_window_hwm, window_size, hwm);
    }
  }

  // build the remapped CV_DebugS: tracked nodes -> section base + prov offset,
  // synthetic/untracked nodes alias their existing bytes
  for EachElement(k, src->data_list) {
    if (symbols_only && k != idx_symbols) { continue; }
    CV_DebugSProvNode *prov = src->prov_list[k].count ? src->prov_list[k].first : 0;
    for (String8Node *n = src->data_list[k].first; n != 0; n = n->next) {
      String8 s = n->string;
      if (prov != 0 && !prov->is_synthetic && prov->sect_idx != CV_DebugSProvSect_Nil) {
        Assert(prov->size == n->string.size);
        s = str8(sect_base[prov->sect_idx] + prov->off, prov->size);
      }
      str8_list_push(arena, &out.data_list[k], s);
      if (prov != 0) { prov = prov->next; }
    }
  }

#if BUILD_DEBUG
  // the string table is consumed from the RAW view by cv_dedup_string_tables and the
  // module-write checksum/source-file pass -- prove relocs never alter it (journal entries
  // cannot: they only target Symbols/InlineeLines runs by construction)
  if (!symbols_only) {
    U64                idx_strtab = cv_c13_sub_section_idx_from_kind(CV_C13SubSectionKind_StringTable);
    CV_DebugSProvNode *prov       = src->prov_list[idx_strtab].count ? src->prov_list[idx_strtab].first : 0;
    for (String8Node *n = src->data_list[idx_strtab].first; n != 0 && prov != 0; n = n->next, prov = prov->next) {
      if (prov->is_synthetic || prov->sect_idx == CV_DebugSProvSect_Nil) { continue; }
      if (sect_state[prov->sect_idx] == 2) {
        COFF_SectionHeader *raw_hdr = &raw_section_table[prov->sect_idx];
        String8 raw = str8_substr(obj->coff.data, r1u64s(raw_hdr->foff, raw_hdr->fsize));
        Assert(MemoryMatch(sect_base[prov->sect_idx] + prov->off, raw.str + prov->off, prov->size));
      }
    }
  }
#endif

  // replay the obj's deferred TI/kind fixup journal into the window: same runs, same order as
  // lnk_apply_debug_s_fixups_for_obj -- only the destination base differs (the i-th symbol
  // input IS the i-th Symbols data_list node, asserted below)
  if (cv->has_debug_s_fixup_journal) {
    {
      String8Node *n = out.data_list[idx_symbols].first;
      for (U64 i = cv->debug_s_sym_fixup_offsets[obj_idx], opl = cv->debug_s_sym_fixup_offsets[obj_idx+1]; i < opl; i += 1, n = n->next) {
        AssertAlways(n != 0);
        Assert(cv->symbol_inputs[i].obj_idx == obj_idx);
        Assert(cv->symbol_inputs[i].raw_symbols.size == n->string.size);
        lnk_apply_debug_s_patch_run(n->string.str, &cv->debug_s_sym_fixups[i], 0, cv->debug_s_sym_fixups[i].count);
      }
    }
    if (!symbols_only) {
      LNK_DebugSInlineJournal *inline_journal = &cv->debug_s_inline_fixups[obj_idx];
      if (inline_journal->patches.count > 0) {
        U64 node_idx = 0, cursor = 0;
        for (String8Node *n = out.data_list[idx_inlinees].first; n != 0; n = n->next) {
          U64 run = inline_journal->node_counts[node_idx++];
          lnk_apply_debug_s_patch_run(n->string.str, &inline_journal->patches, cursor, cursor + run);
          cursor += run;
        }
        Assert(cursor == inline_journal->patches.count);
      }
    }
  }

  lnk_compressed_obj_release_window(&decode_window);
  return out;
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_unique_leaf_sizes_task)
{
  LNK_MergeTypes *task = raw_task;
  Rng1U64 range = task->ranges[task_id];
  U64 size = 0;
  for EachInRange(i, range) {
    // Cache size + NOTYPE disposition in result.v's otherwise-unused slots. Materialization
    // consumes and replaces each value with the final pointer, avoiding a second journal lookup
    // and raw-leaf header read without allocating a side array.
    U64 meta = lnk_leaf_ref_materialize_meta(task->input, task->unique_leaf_refs_arr[task->ti_source].v[i]);
    task->result.v[task->ti_source][i] = (U8 *)meta;
    size += meta >> 2;
  }
  task->leaf_buffer_offsets[task_id] = size; // exclusive-scanned into offsets on the main thread
}

// Materialize unique leaves: copy each unique raw leaf (existing sorted order) into one contiguous
// private buffer and apply the type-index fixup to the COPY. This fuses the old
// lnk_cv_patcher_leaves_task (which patched TIs in-place into the mapped input, dirtying one
// copy-on-write page per touched .debug$T page) with the old lnk_unbucket_raw_leaves_task (which
// pointed result.v into the input). result.v now points into the copy: identical bytes, identical
// order, clean input pages.
internal
THREAD_POOL_TASK_FUNC(lnk_assign_unique_leaf_destinations_task)
{
  LNK_MergeTypes *task        = raw_task;
  Rng1U64         range       = task->ranges[task_id];
  U8             *cursor      = task->leaf_buffer + task->leaf_buffer_offsets[task_id];
  for EachInRange(i, range) {
    U64 meta = (U64)task->result.v[task->ti_source][i];
    U64 raw_size = meta >> 2;
    U64 rewrite = meta & 3;
    Assert(((U64)cursor & 3) == 0);
    task->result.v[task->ti_source][i] = (U8 *)((U64)cursor | rewrite);
    cursor += raw_size;
  }
}

internal void
lnk_materialize_unique_leaf_to(LNK_MergeTypes *task, CV_TypeIndexSource source, U64 i,
                               U8 *cursor, U64 raw_size, U64 rewrite,
                               Arena *fixed_arena, LNK_CObjDecodeWindow *decode_window)
{
    LNK_LeafRef leaf_ref = task->unique_leaf_refs_arr[source].v[i];
    U32         obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
    U32         leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
    CV_DebugT  *debug_t  = &task->input->debug_t_arr[obj_idx];

    // copy raw leaf into the private buffer. Hot path = the ORIGINAL read+memcpy (one bitmap
    // test on top); journaled leaves replay the NOTYPE rewrite into the COPY (full rewrite =>
    // bare { size=sizeof(CV_LeafKind), kind=LF_NOTYPE } header; KIND_ONLY (0x1522) => copy then
    // patch the kind). Byte-identical to the old post-in-place-write copy.
    if (rewrite == LNK_LEAF_MATERIALIZE_FULL_NOTYPE) {
      memory_write16(cursor + OffsetOf(CV_LeafHeader, size), sizeof(CV_LeafKind));
      memory_write16(cursor + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
    } else {
      LNK_Obj *obj = obj_idx < task->input->obj_count ? task->input->obj_arr[obj_idx] : 0;
      B32 copied = 0;
      if (obj && obj->compressed_obj && (debug_t->sidecar_sizes || debug_t->sidecar_packed)) {
        U64 leaf_off = cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
        Rng1U64 raw_range = rng_1u64(debug_t->sidecar_raw_base + leaf_off,
                                     debug_t->sidecar_raw_base + leaf_off + raw_size);
        copied = lnk_compressed_obj_copy_range(obj->compressed_obj, raw_range, cursor, decode_window);
      }
      if (!copied) {
        U8 *raw_leaf = debug_t->data.str + cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
        MemoryCopy(cursor, raw_leaf, raw_size);
      }
      if (rewrite == LNK_LEAF_MATERIALIZE_KIND_NOTYPE) {
        memory_write16(cursor + OffsetOf(CV_LeafHeader, kind), CV_LeafKind_NOTYPE);
      }
    }
    task->result.v[source][i] = cursor;

    // fixup type indices on the copy (same math the in-place leaf patcher applied)
    Temp temp = temp_begin(fixed_arena);
    CV_Leaf leaf = {
      .kind = memory_read16(task->result.v[source][i] + OffsetOf(CV_LeafHeader, kind)),
      .data = str8(task->result.v[source][i] + sizeof(CV_LeafHeader), raw_size - sizeof(CV_LeafHeader)),
    };
    CV_TiOffsets ti_offs = cv_leaf_ti_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_fixup_cv_type_indices(task, obj_idx, leaf.data, ti_offs);
    temp_end(temp);
}

internal void
lnk_materialize_tagged_unique_leaf(LNK_MergeTypes *task, CV_TypeIndexSource source, U64 i,
                                   Arena *fixed_arena, LNK_CObjDecodeWindow *decode_window)
{
  LNK_LeafRef leaf_ref = task->unique_leaf_refs_arr[source].v[i];
  U32 obj_idx = lnk_leaf_ref_obj_idx(leaf_ref);
  U32 leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
  CV_DebugT *debug_t = &task->input->debug_t_arr[obj_idx];
  U64 tagged = (U64)task->result.v[source][i];
  U64 rewrite = tagged & 3;
  U8 *cursor = (U8 *)(tagged & ~(U64)3);
  U64 raw_size = rewrite == LNK_LEAF_MATERIALIZE_FULL_NOTYPE ? sizeof(CV_LeafHeader) :
                 cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx);
  lnk_materialize_unique_leaf_to(task, source, i, cursor, raw_size, rewrite, fixed_arena, decode_window);
}

internal
THREAD_POOL_TASK_FUNC(lnk_materialize_unique_leaves_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task = raw_task;
  Rng1U64 range = task->ranges[task_id];
  U8 *cursor = task->leaf_buffer + task->leaf_buffer_offsets[task_id];
  for EachInRange(i, range) {
    U64 meta = (U64)task->result.v[task->ti_source][i];
    U64 raw_size = meta >> 2;
    U64 rewrite = meta & 3;
    lnk_materialize_unique_leaf_to(task, task->ti_source, i, cursor, raw_size, rewrite,
                                   task->fixed_arenas[worker_id], &task->decode_windows[worker_id]);
    cursor += raw_size;
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_materialize_fused_tpi_ipi_task)
{
  ProfBeginFunction();
  LNK_MergeTypes *task = raw_task;
  U64 tpi_i = task->materialize_obj_offsets[CV_TypeIndexSource_TPI][task_id];
  U64 tpi_opl = task->materialize_obj_offsets[CV_TypeIndexSource_TPI][task_id + 1];
  U64 ipi_i = task->materialize_obj_offsets[CV_TypeIndexSource_IPI][task_id];
  U64 ipi_opl = task->materialize_obj_offsets[CV_TypeIndexSource_IPI][task_id + 1];
  while (tpi_i < tpi_opl || ipi_i < ipi_opl) {
    CV_TypeIndexSource source;
    U64 i;
    if (ipi_i >= ipi_opl ||
        (tpi_i < tpi_opl &&
         lnk_leaf_ref_leaf_idx(task->unique_leaf_refs_arr[CV_TypeIndexSource_TPI].v[tpi_i]) <
         lnk_leaf_ref_leaf_idx(task->unique_leaf_refs_arr[CV_TypeIndexSource_IPI].v[ipi_i]))) {
      source = CV_TypeIndexSource_TPI;
      i = tpi_i++;
    } else {
      source = CV_TypeIndexSource_IPI;
      i = ipi_i++;
    }
    lnk_materialize_tagged_unique_leaf(task, source, i, task->fixed_arenas[worker_id],
                                       &task->decode_windows[worker_id]);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_unbucket_hashes_task)
{
  LNK_MergeTypes *task = raw_task;
  Rng1U64 range = task->ranges[task_id];
  for EachInRange(i, range) {
    LNK_LeafRef leaf_ref = task->unique_leaf_refs_arr[task->ti_source].v[i];
    U32         obj_idx  = lnk_leaf_ref_obj_idx(leaf_ref);
    U32         leaf_idx = lnk_leaf_ref_leaf_idx(leaf_ref);
    task->result.hashes[task->ti_source][i] = task->input->debug_h_arr[obj_idx].v[leaf_idx];
  }
}

internal void
lnk_profile_cobj_winner_segments(LNK_MergeTypes *task)
{
  char *env = getenv("RAD_COBJ_PROFILE_WINNERS");
  if (env == 0 || env[0] == '0') { return; }
  enum { BucketCount = 16, SizeCount = 4 };
  U64 segment_sizes[SizeCount] = { KB(128), KB(256), KB(512), MB(1) };
  U64 touched[CV_TypeIndexSource_COUNT][BucketCount][SizeCount] = {0};
  U64 leaf_counts[CV_TypeIndexSource_COUNT][BucketCount] = {0};
  U64 leaf_bytes[CV_TypeIndexSource_COUNT][BucketCount] = {0};

  for (U32 source = CV_TypeIndexSource_TPI; source <= CV_TypeIndexSource_IPI; ++source) {
    U64 last_key[SizeCount] = { max_U64, max_U64, max_U64, max_U64 };
    LNK_LeafRefArray refs = task->unique_leaf_refs_arr[source];
    for EachIndex(i, refs.count) {
      LNK_LeafRef ref = refs.v[i];
      U32 obj_idx = lnk_leaf_ref_obj_idx(ref);
      U32 leaf_idx = lnk_leaf_ref_leaf_idx(ref);
      if (obj_idx >= task->input->obj_count) { continue; }
      LNK_Obj *obj = task->input->obj_arr[obj_idx];
      CV_DebugT *debug_t = &task->input->debug_t_arr[obj_idx];
      if (!obj->compressed_obj || (!debug_t->sidecar_sizes && !debug_t->sidecar_packed)) { continue; }
      U64 bucket = (U64)obj_idx * BucketCount / task->input->obj_count;
      U64 raw_off = debug_t->sidecar_raw_base + cv_debug_t_get_leaf_offset(debug_t, leaf_idx);
      leaf_counts[source][bucket] += 1;
      leaf_bytes[source][bucket] += cv_debug_t_get_raw_leaf_size(debug_t, leaf_idx);
      for (U32 s = 0; s < SizeCount; ++s) {
        U64 key = ((U64)obj_idx << 32) | (raw_off / segment_sizes[s]);
        if (key != last_key[s]) {
          touched[source][bucket][s] += 1;
          last_key[s] = key;
        }
      }
    }
  }

  for (U32 source = CV_TypeIndexSource_TPI; source <= CV_TypeIndexSource_IPI; ++source) {
    for (U32 bucket = 0; bucket < BucketCount; ++bucket) {
      lnk_log(LNK_Log_Timers,
              "[cobj winners] src=%u bucket=%u objs=%u-%u leaves=%llu leafMiB=%llu seg128=%llu seg256=%llu seg512=%llu seg1024=%llu",
              source, bucket,
              (task->input->obj_count * bucket) / BucketCount,
              (task->input->obj_count * (bucket + 1)) / BucketCount - 1,
              leaf_counts[source][bucket], leaf_bytes[source][bucket] / MB(1),
              touched[source][bucket][0], touched[source][bucket][1],
              touched[source][bucket][2], touched[source][bucket][3]);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_build_obj_ti_map)
{
  LNK_MergeTypes      *task  = raw_task;
  LNK_CodeViewInput   *input = task->input;

  U64           obj_idx    = task_id;
  CV_DebugT    *debug_t    = &input->debug_t_arr[obj_idx];
  CV_TypeIndex *obj_ti_map = task->obj_ti_batch + task->obj_ti_map_offsets[obj_idx];

  // P4: journal bitmap overlays LF_NOTYPE on journaled real-obj leaves (raw bytes are pre-rewrite)
  U64 *notype_bm  = input->notype_journal[obj_idx].bitmap;
  U64  notype_cap = input->notype_journal[obj_idx].bit_cap;

  for EachIndex(leaf_idx, debug_t->count) {
    CV_LeafKind kind = cv_debug_t_get_leaf_kind(debug_t, leaf_idx);
    if (notype_bm && leaf_idx < notype_cap && ((notype_bm[leaf_idx >> 6] >> (leaf_idx & 63)) & 1)) { kind = CV_LeafKind_NOTYPE; }
    CV_TypeIndexSource  source   = cv_type_index_source_from_leaf_kind(kind);
    LNK_LeafRef         leaf_ref = lnk_leaf_ref_make(obj_idx, leaf_idx);
    LNK_AssignedTiHash *assigned = &task->assigned_ti_arr[source];
    obj_ti_map[leaf_idx] = lnk_assigned_ti_hash_search(assigned, input, leaf_ref);
  }

  task->result.obj_ti_maps[obj_idx] = obj_ti_map;
}

internal LNK_MergedTypes
lnk_merge_types(TP_Context *tp, TP_Arena *tp_temp, LNK_CodeViewInput *input, LNK_MergeTypeFlags merge_flags)
{
  ProfBeginFunction();
  Temp scratch = temp_begin(lnk_get_huge_arena());

  LNK_MergeTypes task = { .input = input };
  // scratch bound: CV_TiOff arrays for member-walk leaves are built with doubling growth
  // (sum of caps <= ~4x entry count, entries <= max_U16/8 per leaf => <= ~512KB), plus
  // deep-hash stack frames; 2x the legacy per-node list bound keeps comfortable headroom
  U64 max_ti_list_size = 2 * sizeof(CV_TypeIndexInfo) * (max_U16 / sizeof(CV_TypeIndex));
  task.fixed_arenas = alloc_fixed_size_arena_array(scratch.arena, tp->worker_count, max_ti_list_size, max_ti_list_size);
  task.decode_windows = push_array(scratch.arena, LNK_CObjDecodeWindow, tp->worker_count);

  ProfBegin("Produce Hashes");
  {
    ProfBegin("Alloc Hashes");
    struct HashTarget {
      TP_TaskFunc *hasher_task;
      U32Array     indices;
      U32Array     hash_indices;
    } hash_targets[] = {
      { lnk_hash_debug_t_deep_task, input->ifc_indices         }, // hash .ifc blobs first: int-obj leaves redirect into them
      { lnk_hash_debug_t_task,      input->debug_p_indices     }, // hash .debug$P first so we can mix in hashes for precompiled sub leaves when hashing leaves in .debug$T
      { lnk_hash_debug_t_task,      input->int_obj_indices     },
      { lnk_hash_debug_t_deep_task, input->type_server_indices },
    };

    for EachElement(i, hash_targets) {
      // reserve array for obj indices that need hashing
      U32Array *h = &hash_targets[i].hash_indices;
      h->count = 0;
      h->v     = push_array(scratch.arena, U32, hash_targets[i].indices.count);

      for EachIndex(k, hash_targets[i].indices.count) {
        U32        obj_idx = hash_targets[i].indices.v[k];
        CV_DebugH *debug_h = &input->debug_h_arr[obj_idx];

        if (debug_h->count == 0) {
          // alloc hashes
          CV_DebugT *debug_t = &input->debug_t_arr[obj_idx];
          debug_h->count = debug_t->count;
          debug_h->v     = push_array(scratch.arena, U64, debug_h->count);

          // schedule obj types to be hashed
          h->v[h->count++] = obj_idx;
        } else {
          // hash was loaded from .debug$H
        }
      }
    }
    ProfEnd();

    // batch-populate the .debug$T/$P leaf data the hashers below walk leaf by
    // leaf; under farm-wide memory pressure these mapped pages were trimmed
    // since the parse phase touched them (see lnk_prefetch_ranges)
    if (lnk_should_prefetch_mapped_input()) ProfScope("Prefetch Type Data")
    {
      Temp temp = temp_begin(scratch.arena);

      U64 range_cap = 0;
      for EachElement(i, hash_targets) { range_cap += hash_targets[i].hash_indices.count; }

      Rng1U64 *ranges      = push_array_no_zero(temp.arena, Rng1U64, range_cap);
      U64      range_count = 0;
      for EachElement(i, hash_targets) {
        for EachIndex(k, hash_targets[i].hash_indices.count) {
          String8 data = input->debug_t_arr[hash_targets[i].hash_indices.v[k]].data;
          if (data.size) { ranges[range_count++] = rng_1u64((U64)data.str, (U64)data.str + data.size); }
        }
      }
      U64 prefetch_begin_us = now_time_us();
      U64 prefetch_bytes    = 0;
      for EachIndex(range_idx, range_count) { prefetch_bytes += dim_1u64(ranges[range_idx]); }
      lnk_prefetch_ranges(tp, input->config->debug_worker_cap, range_count, ranges);
      lnk_log(LNK_Log_Timers, "[merge] prefetched %llu type data ranges (%llu MiB) in %.2f ms",
              range_count, prefetch_bytes / MB(1), (F64)(now_time_us() - prefetch_begin_us) / 1000.0);

      temp_end(temp);
    }

    for EachElement(i, hash_targets) {
      task.indices = hash_targets[i].hash_indices;
      ProfBegin("Hash [Count: %.*s]", str8_varg(str8_from_count(scratch.arena, task.indices.count)));
      // P4: pass real worker arenas -- the shallow hasher's invalid-TI/cyclic discards push
      // NOTYPE journal entries (rare error paths, bytes are negligible on tp_temp)
      tp_for_parallel(tp, tp_temp, task.indices.count, hash_targets[i].hasher_task, &task);
      ProfEnd();
    }

#if BUILD_DEBUG
    for EachIndex(i, input->count) {
      for EachIndex(k, input->debug_h_arr[i].count) {
        Assert(input->debug_h_arr[i].v[k] != 0);
      }
    }
#endif

    // for external objs wire hash sections to type servers hashes
    for EachIndex(i, input->ext_obj_indices.count) {
      U64 dst_obj_idx = input->ext_obj_indices.v[i];
      U64 src_obj_idx = input->obj_to_ts[dst_obj_idx];
      CV_DebugH *dst_debug_h = &input->debug_h_arr[dst_obj_idx];
      CV_DebugH *src_debug_h = &input->debug_h_arr[src_obj_idx];
      *dst_debug_h = *src_debug_h;
    }
  }
  ProfEnd();

  // bucket_arr (the ~1.3x-total-leaf-count probe tables) is only live through the dedup + extract
  // phases: its last read is in lnk_get_present_buckets_task ("Copy present buckets") which copies
  // bucket pointers into unique_leaf_refs. Allocate it in a dedicated arena so we can release that
  // multi-GB working set immediately after the extract loop, before the merge-types/PDB-build peak.
  // (A temp_begin on scratch.arena would not work: many surviving allocations -- unique_leaf_refs,
  // assigned_ti, radix scratch -- land in scratch.arena after bucket_arr.)
  Arena *bucket_arena = arena_alloc(.name = "LEAF_BUCKETS");

  ProfBegin("Leaf Hash Table Init");
  // fallback caps derived from TOTAL (pre-dedup, pre-prune) leaf counts. total >= unique always, so
  // these caps can never overflow; they are also the caps the estimate-based sizing clamps against
  // and retries with. NOTE: the NULL-source cap is deliberately derived from the pre-prune
  // source_counts (see the IFC pruning comment in lnk_leaf_dedup_task) -- pruned NOTYPE leaves are
  // never inserted, so pre-prune totals always cover the insert set with slack.
  U64 leaf_ht_cap_fallback[CV_TypeIndexSource_COUNT] = {0};
  {
    U64 total_counts[CV_TypeIndexSource_COUNT] = {0};
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      for EachIndex(obj_idx, input->count) { total_counts[ti_source] += input->debug_t_arr[obj_idx].source_counts[ti_source]; }
      // pow2 cap so bucket index is hash & (cap-1) (mask) instead of hash % cap (a 64-bit DIV in the
      // densest dedup probe loop). u64_up_to_pow2(1.3*count) keeps load factor <= ~0.65.
      leaf_ht_cap_fallback[ti_source] = u64_up_to_pow2(1 + ((total_counts[ti_source] * 13) / 10)); // * 1.3, pow2
    }

    // On dup-heavy input (PCH/type-server fan-out) unique count is a small fraction of total, and a
    // total-sized probe table wastes multi-GB of demand-zero page faults on 64B-apart random probes.
    // Estimate the distinct-hash count from the already-produced debug_h hashes (Produce Hashes
    // completes above) with a per-source presence bitmap + linear counting, and size the tables from
    // that instead. Everything here is a pure function of the input hashes, so the caps -- and the
    // overflow/retry decision below -- are identical run to run.
    ProfBegin("Estimate Unique Leaves");
    U64 estimate_begin_us = now_time_us();
    {
      // sweep exactly the objs whose leaves get inserted: prepopulate + the four dedup passes
      U32Array sweep_arrs[] = { input->debug_p_indices, input->int_obj_indices, input->type_server_indices, input->ifc_indices };
      U32Array sweep_indices = {0};
      for EachElement(i, sweep_arrs) { sweep_indices.count += sweep_arrs[i].count; }
      sweep_indices.v     = push_array_no_zero(scratch.arena, U32, sweep_indices.count);
      sweep_indices.count = 0;
      for EachElement(i, sweep_arrs) {
        MemoryCopy(sweep_indices.v + sweep_indices.count, sweep_arrs[i].v, sizeof(U32) * sweep_arrs[i].count);
        sweep_indices.count += sweep_arrs[i].count;
      }

      for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
        // at most ceil(total/K) hashes are inserted under K-th-position sampling, so bits >=
        // total/K >= sampled-unique keeps the bitmap load < 1, where linear counting is accurate;
        // clamp keeps the transient bitmap allocation bounded (16MB per source at the top end)
        U64 sampled_total = (total_counts[ti_source] + LNK_ESTIMATE_SAMPLE_STRIDE - 1) / LNK_ESTIMATE_SAMPLE_STRIDE;
        task.estimate_bitmap_bits[ti_source] = u64_up_to_pow2(Clamp(1ull << 16, sampled_total, 1ull << 27));
        task.estimate_bitmap     [ti_source] = push_array(scratch.arena, U32, task.estimate_bitmap_bits[ti_source] / 32);
      }

      task.indices = sweep_indices;
      tp_for_parallel(tp, 0, task.indices.count, lnk_estimate_unique_leaves_task, &task);

      for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
        U64 bit_count  = task.estimate_bitmap_bits[ti_source];
        U64 word_count = bit_count / 32;
        U64 set_count  = 0;
        for EachIndex(word_idx, word_count) { set_count += count_bits_set32(task.estimate_bitmap[ti_source][word_idx]); }

        U64 cap = leaf_ht_cap_fallback[ti_source];
        U64 zero_count = bit_count - set_count;
        // the NULL-source table keeps the historic total-based cap: objs synthesized outside the
        // parse path never populate source_counts, so its cap can be far below the number of
        // NULL-source leaves thrown at it. that has always been tolerated -- NULL-source leaves are
        // never emitted, the table just drops what does not fit (see the fall-through handling in
        // lnk_leaf_dedup_task) -- so it must not participate in estimate sizing or overflow retry.
        if (ti_source != CV_TypeIndexSource_NULL && set_count > 0 && zero_count > 0) {
          // linear counting: distinct ~= m * ln(m / zeros)
          F64 estimate = (F64)bit_count * log((F64)bit_count / (F64)zero_count);
          if (estimate < (F64)set_count) { estimate = (F64)set_count; }
          // scale the sampled distinct count back up to a full-population estimate (see the
          // sampling comment above lnk_estimate_unique_leaves_task)
          estimate *= LNK_ESTIMATE_SAMPLE_SCALE;
          // 1.9x safety keeps the load factor <= ~0.55 even before pow2 rounding; overflow (only
          // possible if the estimate undershoots by >1.8x) is caught and retried deterministically
          U64 target = (U64)(estimate * 1.9) + 4096;
          cap = Min(u64_up_to_pow2(target), leaf_ht_cap_fallback[ti_source]);
          lnk_log(LNK_Log_Timers, "[typededup] estimate src=%llu: set=%llu est=%.0f cap=%llu (fallback %llu)",
                  ti_source, set_count, estimate, cap, leaf_ht_cap_fallback[ti_source]);
        }
        task.leaf_ht_arr[ti_source].cap = cap;
      }
    }
    lnk_log(LNK_Log_Timers, "[typededup] unique-leaf estimate in %.2f ms", (F64)(now_time_us() - estimate_begin_us) / 1000.0);
    ProfEnd();

    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      task.leaf_ht_arr[ti_source].bucket_arr = push_array_no_zero(bucket_arena, LNK_LeafRef, task.leaf_ht_arr[ti_source].cap);
      MemorySet(task.leaf_ht_arr[ti_source].bucket_arr, 0xff, sizeof(LNK_LeafRef) * task.leaf_ht_arr[ti_source].cap);

#if PROFILE_TELEMETRY
      tmMessage(0, TMMF_ICON_NOTE, "%.*s Bucket Count: %.*s", str8_varg(cv_string_from_type_index_source(ti_source)), str8_varg(str8_from_count(scratch.arena, task.leaf_ht_arr[ti_source].cap)));
#endif
    }
  }
  ProfEnd();

  U32Array dedup_type_server_indices = input->type_server_indices;

  LNK_TypeServer *largest_ts = 0;
  {
    for EachIndex(i, input->ts_arr.count) {
      LNK_TypeServer *ts = &input->ts_arr.v[i];
      if (ts->rrt == 0) { continue; }
      if (largest_ts == 0 || (largest_ts->rrt->type_data_raw.size < ts->rrt->type_data_raw.size)) {
        largest_ts = ts;
      }
    }

    if (largest_ts) {
      task.pop_obj_idx = input->ts_obj_range.min + largest_ts->ts_idx;
      task.pop_range   = tp_divide_work(scratch.arena, task.input->debug_t_arr[task.pop_obj_idx].count, tp->worker_count);

      U32Array new_dedup_type_server_indices = { .v = push_array(scratch.arena, U32, input->type_server_indices.count) };
      for EachIndex(i, input->type_server_indices.count) {
        if (input->type_server_indices.v[i] == task.pop_obj_idx) { continue; }
        new_dedup_type_server_indices.v[new_dedup_type_server_indices.count++] = input->type_server_indices.v[i];
      }
      dedup_type_server_indices = new_dedup_type_server_indices;
    }
  }

  for (U64 attempt = 0; ; attempt += 1) {
    ProfBegin("Prepopulate hash table with largest type-set");
    if (largest_ts) {
      tp_for_parallel(tp, tp_temp, tp->worker_count, lnk_populate_leaf_ht, &task);
    }
    ProfEnd();

    ProfBegin("Leaf Dedup");
    task.indices = input->debug_p_indices;
    tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, ".debug$P");

    task.indices = input->int_obj_indices;
    tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, ".debug$T");

    task.indices = dedup_type_server_indices;
    tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, "Type Servers");

    task.indices = input->ifc_indices;
    tp_for_parallel_prof(tp, tp_temp, task.indices.count, lnk_leaf_dedup_task, &task, "IFC Blobs");
    ProfEnd();

    if (ins_atomic_u32_eval(&task.leaf_ht_overflow) == 0) { break; }

    // an estimate-sized table overflowed: rebuild every probe table at the always-sufficient
    // total-based caps and redo the passes. the input hashes are deterministic, so this branch is
    // taken (or not) identically every run, and the retried result is what the total-based sizing
    // would have produced in the first place. the fallback caps cannot overflow, so at most one retry.
    AssertAlways(attempt == 0);
    lnk_log(LNK_Log_Debug, "leaf dedup: unique-count estimate overflowed, retrying with total-based table caps");
    ins_atomic_u32_eval_assign(&task.leaf_ht_overflow, 0);
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      task.leaf_ht_arr[ti_source].cap        = leaf_ht_cap_fallback[ti_source];
      task.leaf_ht_arr[ti_source].bucket_arr = push_array_no_zero(bucket_arena, LNK_LeafRef, leaf_ht_cap_fallback[ti_source]);
      MemorySet(task.leaf_ht_arr[ti_source].bucket_arr, 0xff, sizeof(LNK_LeafRef) * leaf_ht_cap_fallback[ti_source]);
    }
  }

  ProfBegin("Extract present buckets from the leaf hash tables");

  B32 winner_prefetch_enabled = 0;
  {
    char *env = getenv("RAD_COBJ_PREFETCH_WINNERS");
    winner_prefetch_enabled = (env == 0 || env[0] != '0');
    if (winner_prefetch_enabled) {
      task.winner_segment_offsets = push_array(scratch.arena, U64, input->obj_count + 1);
      for EachIndex(obj_idx, input->obj_count) {
        task.winner_segment_offsets[obj_idx + 1] = task.winner_segment_offsets[obj_idx] +
          lnk_compressed_obj_segment_count(input->obj_arr[obj_idx]->compressed_obj);
      }
      U64 total_segments = task.winner_segment_offsets[input->obj_count];
      if (total_segments) {
        task.winner_segment_word_count = CeilIntegerDiv(total_segments, 64);
        task.winner_segment_bitmap = push_array(scratch.arena, U64, task.winner_segment_word_count);
        task.winner_segment_worker_bitmaps = push_array(scratch.arena, U64,
                                                         task.winner_segment_word_count * tp->worker_count);
        task.winner_segment_keys = push_array_no_zero(scratch.arena, U64, total_segments);
      }
    }
  }

  U32 winner_prefetch_worker_count = 8;
  {
    char *env = getenv("RAD_COBJ_PREFETCH_WORKERS");
    if (env) { winner_prefetch_worker_count = Clamp(1, atoi(env), LNK_WINNER_PREFETCH_WORKER_MAX); }
  }
  LNK_WinnerPrefetchFlight winner_prefetch_flights[CV_TypeIndexSource_COUNT] = {0};
  U64 winner_prefetch_key_cursor = 0;

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    task.ti_source          = ti_source;
    task.counts[ti_source]  = push_array(scratch.arena, U64, tp->worker_count);
    task.ranges             = tp_divide_work(scratch.arena, task.leaf_ht_arr[ti_source].cap, tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_count_present_buckets_task, &task, "Count present buckets");

    task.unique_leaf_refs_arr[ti_source].count = sum_array_u64(tp->worker_count, task.counts[ti_source]);
    task.unique_leaf_refs_arr[ti_source].v     = push_array_no_zero(scratch.arena, LNK_LeafRef, task.unique_leaf_refs_arr[ti_source].count);
    task.offsets[ti_source]                    = offsets_from_counts_array_u64(scratch.arena, task.counts[ti_source], tp->worker_count);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_get_present_buckets_task, &task, "Copy present buckets");

    lnk_log(LNK_Log_Timers, "[typededup] src=%llu: unique=%llu cap=%llu load=%.3f",
            ti_source, task.unique_leaf_refs_arr[ti_source].count, task.leaf_ht_arr[ti_source].cap,
            task.leaf_ht_arr[ti_source].cap ? (F64)task.unique_leaf_refs_arr[ti_source].count / (F64)task.leaf_ht_arr[ti_source].cap : 0.0);

    // Each type source gets an independent flight. TPI remains in flight while IPI is extracted
    // and sorted, and IPI may join it; both are joined only immediately before materialization.
    // Reduce lane-private bitmaps after extraction. The bitmap is only ~180k bits for UEFN;
    // scanning it is negligible and avoids a contended atomic operation for every winning leaf.
    if (task.winner_segment_bitmap && ti_source != CV_TypeIndexSource_NULL) {
      U64 obj_idx = 0;
      for EachIndex(word_idx, task.winner_segment_word_count) {
        U64 merged = 0;
        for EachIndex(lane, tp->worker_count) {
          merged |= task.winner_segment_worker_bitmaps[lane * task.winner_segment_word_count + word_idx];
        }
        U64 new_bits = merged & ~task.winner_segment_bitmap[word_idx];
        task.winner_segment_bitmap[word_idx] |= merged;
        while (new_bits) {
          U64 bit_idx = ctz64(new_bits);
          U64 global_idx = word_idx * 64 + bit_idx;
          while (obj_idx + 1 < input->obj_count &&
                 global_idx >= task.winner_segment_offsets[obj_idx + 1]) {
            obj_idx += 1;
          }
          U64 segment_idx = global_idx - task.winner_segment_offsets[obj_idx];
          task.winner_segment_keys[task.winner_segment_key_count++] = (obj_idx << 32) | segment_idx;
          new_bits &= new_bits - 1;
        }
      }
    }
    U64 key_opl = task.winner_segment_key_count;
    if (key_opl > winner_prefetch_key_cursor) {
      U64 count = key_opl - winner_prefetch_key_cursor;
      Rng1U64 *ranges = push_array_no_zero(scratch.arena, Rng1U64, count);
      U64 stored_bytes = 0;
      for EachIndex(i, count) {
        U64 key = task.winner_segment_keys[winner_prefetch_key_cursor + i];
        U32 obj_idx = (U32)(key >> 32);
        U32 segment_idx = (U32)key;
        ranges[i] = lnk_compressed_obj_stored_segment_range(input->obj_arr[obj_idx]->compressed_obj, segment_idx);
        stored_bytes += dim_1u64(ranges[i]);
      }
      winner_prefetch_key_cursor = key_opl;
      lnk_winner_prefetch_start(&winner_prefetch_flights[ti_source], winner_prefetch_worker_count,
                                count, ranges, stored_bytes);
    }

    // sort output leaves based on { location index, leaf index } to guarantee determinism
    ProfScope("Radix Sort") {
      u64_array_sort_radix_parallel(tp, task.unique_leaf_refs_arr[ti_source].count, task.unique_leaf_refs_arr[ti_source].v);
#if BUILD_DEBUG
      LNK_LeafRefArray arr = task.unique_leaf_refs_arr[ti_source];
      for (U64 i = 1; i < arr.count; ++i) {
        AssertAlways(lnk_leaf_ref_compare(arr.v[i-1], arr.v[i]) <= 0);
      }
#endif
    }
  }

  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    lnk_winner_prefetch_join(&winner_prefetch_flights[ti_source]);
  }

  lnk_profile_cobj_winner_segments(&task);

  // bucket_arr is fully consumed (copied into unique_leaf_refs / sorted) -- release the probe tables
  // now so this multi-GB working set is gone before the merge-types/PDB-build peak. Handed to a
  // background reaper thread: a serial VirtualFree(MEM_RELEASE) of these multi-GB committed blocks
  // costs ~350ms+ of main-thread kernel time (MiDeleteVaDirect/MiDecommitFreePage), which otherwise
  // sits on the critical path between dedup and the type-index fixup passes. All pointers into the
  // arena are dropped below before the launch returns ownership to the reaper.
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) { task.leaf_ht_arr[ti_source].bucket_arr = 0; }
  if (g_arena_reaper_thread.u64[0] != 0) { thread_join(g_arena_reaper_thread, max_U64); }
  g_arena_reaper_thread = thread_launch(lnk_arena_release_thread, bucket_arena);

  #if PROFILE_TELEMETRY
  tmMessage(0, TMMF_ICON_NOTE, "TPI Count: %.*s", str8_varg(str8_from_count(scratch.arena, task.unique_leaf_refs_arr[CV_TypeIndexSource_TPI].count)));
  tmMessage(0, TMMF_ICON_NOTE, "IPI Count: %.*s", str8_varg(str8_from_count(scratch.arena, task.unique_leaf_refs_arr[CV_TypeIndexSource_IPI].count)));
  #endif

  ProfEnd();

  ProfBegin("Fixup Type Indices");
  {
    ProfBegin("Assign type indices");
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      task.ti_source                         = ti_source;
      task.assigned_ti_arr[ti_source].cap    = ((task.unique_leaf_refs_arr[ti_source].count * 13) / 10);
      // Keep hash + assigned TI adjacent: the fixup passes probe this table hundreds of
      // millions of times and usually need both fields. Packed 12-byte entries preserve the
      // exact former 8+4 byte footprint while normally requiring one cache line instead of two.
      task.assigned_ti_arr[ti_source].v = push_array(scratch.arena, U8, task.assigned_ti_arr[ti_source].cap * LNK_ASSIGNED_TI_ENTRY_SIZE);

      task.min_type_indices[ti_source] = CV_MinComplexTypeIndex;
      task.ranges = tp_divide_work(scratch.arena, task.unique_leaf_refs_arr[ti_source].count, tp->worker_count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_assign_type_indices_task, &task, "Assign Type Indices");
    }
    ProfEnd();

    // NOTE: the $S symbol/inlinee TI fixups are journaled below (after the materialize pass);
    // the bytes are patched per obj at the start of the module-write visit.

    // NOTE: the leaf TI-fixup is fused into the unbucket/materialize pass below -- it copies each
    // unique leaf into a private buffer and patches the copy, instead of patching the mapped input
    // (which copy-on-writes one page per touched .debug$T page).
  }
  ProfEnd();

  // @type_server
  if (merge_flags & LNK_MergeTypeFlag_BuildObjTiMap) {
    task.result.obj_ti_maps = push_array(tp_temp->v[0], CV_TypeIndex *, input->obj_count);
    task.obj_ti_map_counts = push_array(scratch.arena, U64, input->obj_count);
    for EachIndex(obj_idx, input->obj_count) {
      task.obj_ti_map_counts[obj_idx] = input->debug_t_arr[obj_idx].count;
    }

    U64 total_ti_count = sum_array_u64(input->obj_count, task.obj_ti_map_counts);
    task.obj_ti_map_offsets = offsets_from_counts_array_u64(scratch.arena, task.obj_ti_map_counts, input->obj_count);
    task.obj_ti_batch       = push_array_no_zero(tp_temp->v[0], CV_TypeIndex, total_ti_count);
    task.result.obj_ti_maps = push_array_no_zero(tp_temp->v[0], CV_TypeIndex *, input->obj_count);
      
    tp_for_parallel_prof(tp, 0, input->obj_count, lnk_build_obj_ti_map, &task, "Build TI Map");
  }

  lnk_compressed_obj_log_phase_stats("before type materialize");
  U64 materialize_begin_us = now_time_us();
  B32 has_compressed_inputs = 0;
  for EachIndex(obj_idx, input->obj_count) {
    if (input->obj_arr[obj_idx]->compressed_obj) { has_compressed_inputs = 1; break; }
  }
  char *fuse_materialize_env = getenv("RAD_COBJ_FUSE_TYPE_MATERIALIZE");
  B32 fuse_materialize = has_compressed_inputs &&
                         (fuse_materialize_env == 0 || fuse_materialize_env[0] != '0');
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    LNK_LeafRefArray unique_leaf_refs = task.unique_leaf_refs_arr[ti_source];

    task.ti_source               = ti_source;
    task.result.count[ti_source] = unique_leaf_refs.count;
    task.result.v    [ti_source] = push_array(tp_temp->v[0], U8 *, unique_leaf_refs.count);
    task.ranges                  = tp_divide_work(scratch.arena, unique_leaf_refs.count, tp->worker_count);

    // per-lane byte totals for the materialize buffer, exclusive-scanned into offsets
    task.leaf_buffer_offsets = push_array_no_zero(scratch.arena, U64, tp->worker_count + 1);
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_count_unique_leaf_sizes_task, &task, "Count Leaf Sizes");
    {
      U64 acc = 0;
      for EachIndex(lane, tp->worker_count) {
        U64 lane_size = task.leaf_buffer_offsets[lane];
        task.leaf_buffer_offsets[lane] = acc;
        acc += lane_size;
      }
      task.leaf_buffer_offsets[tp->worker_count] = acc;
      // standalone allocation (not an arena push): pdb_build_types copies these bytes
      // into MSF pages and nothing reads cv_types.v afterwards, so the linker path
      // releases the buffer right after -- multi-GB on AutoRTFM-scale inputs. An arena
      // push could not be handed back without poisoning later pushes into the range.
      if (acc > 0) {
        task.leaf_buffer = reserve_memory(acc);
        if (task.leaf_buffer == 0 || !commit_memory(task.leaf_buffer, acc)) {
          lnk_error(LNK_Error_Boot, "failed to allocate %M for merged type leaves", acc);
        }
        task.result.leaf_buffers[ti_source] = str8(task.leaf_buffer, acc);
      } else {
        task.leaf_buffer = push_array_no_zero(tp_temp->v[0], U8, 1);
      }
    }
    if (fuse_materialize && ti_source != CV_TypeIndexSource_NULL) {
      tp_for_parallel(tp, 0, tp->worker_count, lnk_assign_unique_leaf_destinations_task, &task);
    } else {
      // Exact original raw-OBJ path: count/meta and destination copy remain a
      // single lane-balanced traversal with no tagged-pointer setup pass.
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_materialize_unique_leaves_task, &task, "Materialize NULL Leaves");
    }
  }

  // TPI and IPI leaf kinds are interleaved in each .debug$T stream. Build an
  // object range index for each sorted winner array, then merge the two ranges
  // by leaf index inside one task. A compressed object's decode window now sees
  // each physical segment once instead of once in the TPI pass and again in the
  // IPI pass. Destination pointers were assigned above, so output order and
  // bytes remain exactly the same as the separate passes.
  if (fuse_materialize) {
    for (U32 source = CV_TypeIndexSource_TPI; source <= CV_TypeIndexSource_IPI; ++source) {
      LNK_LeafRefArray refs = task.unique_leaf_refs_arr[source];
      U64 *offsets = task.materialize_obj_offsets[source] = push_array_no_zero(scratch.arena, U64, input->count + 1);
      U64 cursor = 0;
      for EachIndex(obj_idx, input->count) {
        offsets[obj_idx] = cursor;
        while (cursor < refs.count && lnk_leaf_ref_obj_idx(refs.v[cursor]) == obj_idx) { cursor += 1; }
      }
      offsets[input->count] = cursor;
      Assert(cursor == refs.count);
    }

    tp_for_parallel_prof(tp, 0, input->count, lnk_materialize_fused_tpi_ipi_task, &task, "Materialize + Fixup TPI/IPI");
  }
  lnk_log(LNK_Log_Timers, "[cobj materialize] fused=%u time=%.3fs", fuse_materialize,
          (F64)(now_time_us() - materialize_begin_us) / 1e6);
  lnk_compressed_obj_log_phase_stats("after type materialize");

  if (merge_flags & LNK_MergeTypeFlag_ExportHashes) {
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      LNK_LeafRefArray unique_leaf_refs = task.unique_leaf_refs_arr[ti_source];
      task.ti_source = ti_source;
      task.ranges = tp_divide_work(scratch.arena, unique_leaf_refs.count, tp->worker_count);
      task.result.hashes[ti_source] = push_array_no_zero(tp_temp->v[0], U64, unique_leaf_refs.count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_unbucket_hashes_task, &task, "Export Hashes");
    }
  }

  // Streaming-ring P2 slice A: the $S TI/kind fixups no longer patch bytes here. While the
  // merge state they consume is still alive -- the assigned-TI hash tables (merge scratch,
  // dead at temp_end below) and the materialized IPI leaf copies (released after
  // pdb_build_types, i.e. BEFORE the module-write pass) -- record every write into per-input /
  // per-obj journals. lnk_write_pdb_modules replays an obj's journal at the start of its
  // module-write visit; configs that read fixed-up $S earlier (or never write modules) run
  // lnk_apply_debug_s_fixups_eager instead (see lnk.c).
  if (~merge_flags & LNK_MergeTypeFlag_SkipSymbolTypeFixup) {
    ProfBegin("Journal $S Fixups");
    // The journal is dead after the last per-obj replay (module write / eager apply) but is
    // GB-class at FN scale -- park it on dedicated per-worker arenas (TYPE_MERGE_SCRATCH
    // pattern: TP_Arena header + v[] live inside v[0], released last by tp_arena_release) so
    // lnk_release_debug_s_fixup_journal can hand the whole set to the background reaper
    // instead of the commit riding the link-lifetime TP arena to process exit.
    {
      Temp temp = temp_begin(scratch.arena);
      Arena **arr = push_array(temp.arena, Arena *, tp->worker_count);
      for EachIndex(i, tp->worker_count) { arr[i] = arena_alloc(.commit_size = MB(2), .name = "DEBUG_S_FIXUP_JOURNAL"); }
      TP_Arena *journal_arenas = push_array(arr[0], TP_Arena, 1);
      journal_arenas->count    = tp->worker_count;
      journal_arenas->v        = push_array(arr[0], Arena *, tp->worker_count);
      MemoryCopyTyped(journal_arenas->v, arr, tp->worker_count);
      input->debug_s_fixup_journal_arenas = journal_arenas;
      temp_end(temp);
    }

    // NOTE: all per-obj journal arrays span input->count, NOT input->obj_count -- injected
    // type-server / .ifc blob pseudo objs live at indices [obj_count, count) in the parallel
    // arrays (their $S is empty, so their journals stay empty, but the inline task dispatches
    // over input->count and must have a slot to write).
    task.journal_arena               = input->debug_s_fixup_journal_arenas;
    task.debug_s_arr                 = input->debug_s_arr;
    input->debug_s_sym_fixups        = push_array(task.journal_arena->v[0], LNK_DebugSPatchArray, input->symbol_input_count ? input->symbol_input_count : 1);
    input->debug_s_inline_fixups     = push_array(task.journal_arena->v[0], LNK_DebugSInlineJournal, input->count ? input->count : 1);
    input->debug_s_sym_fixup_offsets = push_array(task.journal_arena->v[0], U64, input->count + 1);
    {
      // symbol_inputs are filled per obj at prefix-sum offsets => obj-contiguous, ascending
      U64 *counts = push_array(scratch.arena, U64, input->count ? input->count : 1);
      for EachIndex(i, input->symbol_input_count) { counts[input->symbol_inputs[i].obj_idx] += 1; }
      U64 acc = 0;
      for EachIndex(obj_idx, input->count) { input->debug_s_sym_fixup_offsets[obj_idx] = acc; acc += counts[obj_idx]; }
      input->debug_s_sym_fixup_offsets[input->count] = acc;
      Assert(acc == input->symbol_input_count);
    }
    tp_for_parallel_prof(tp, 0, input->symbol_patch_task_count, lnk_journal_symbol_fixups_task, &task, "Journal Symbol Fixups");
    tp_for_parallel_prof(tp, 0, input->count,                   lnk_journal_inline_fixups_task, &task, "Journal Inline Fixups");
    input->has_debug_s_fixup_journal = 1;
    lnk_compressed_obj_log_phase_stats("after journal $S fixups");

    if (lnk_get_log_status(LNK_Log_Debug)) {
      U64 entry_count = 0, entry_bytes = 0, wide_runs = 0;
      for EachIndex(i, input->symbol_input_count) {
        LNK_DebugSPatchArray *a = &input->debug_s_sym_fixups[i];
        entry_count += a->count; entry_bytes += a->count * (a->is_wide ? sizeof(LNK_DebugSPatchWide) : sizeof(LNK_DebugSPatch)); wide_runs += !!a->is_wide;
      }
      for EachIndex(i, input->count) {
        LNK_DebugSPatchArray *a = &input->debug_s_inline_fixups[i].patches;
        entry_count += a->count; entry_bytes += a->count * (a->is_wide ? sizeof(LNK_DebugSPatchWide) : sizeof(LNK_DebugSPatch)); wide_runs += !!a->is_wide;
      }
      U64 assigned_bytes = 0;
      for EachIndex(s, CV_TypeIndexSource_COUNT) { assigned_bytes += task.assigned_ti_arr[s].cap * LNK_ASSIGNED_TI_ENTRY_SIZE; }
      U64 leaf_buffer_bytes = task.result.leaf_buffers[CV_TypeIndexSource_TPI].size + task.result.leaf_buffers[CV_TypeIndexSource_IPI].size;
      lnk_log(LNK_Log_Debug, "$S fixup journal: %llu entries / %llu bytes (%llu wide runs) (state it decouples from module write: assigned-TI tables %llu bytes, merged leaf buffers %llu bytes)",
              entry_count, entry_bytes, wide_runs, assigned_bytes, leaf_buffer_bytes);
    }
    ProfEnd();
  }

  for EachIndex(i, tp->worker_count) { lnk_compressed_obj_release_window(&task.decode_windows[i]); }

  MemoryCopyTyped(task.result.min_type_indices, input->min_type_indices, CV_TypeIndexSource_COUNT);

  temp_end(scratch);
  ProfEnd();
  return task.result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_type_names_with_hashes_lenient_task)
{
  ProfBeginFunction();

  LNK_TypeNameReplacer *task        = raw_task;
  Rng1U64               range       = task->ranges[task_id];
  U64                   leaf_count  = task->leaf_count;
  U8                  **leaf_arr    = task->leaf_arr;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64  hash_max_chars = hash_length*2;
  char temp[128];

  for EachInRange(leaf_idx, range) {
    CV_Leaf leaf = cv_leaf_from_ptr(leaf_arr[leaf_idx]);
    if (leaf.kind == CV_LeafKind_STRUCTURE || leaf.kind == CV_LeafKind_CLASS) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);

      if ((udt_info.props & CV_TypeProp_HasUniqueName) &&
           udt_info.unique_name.size > hash_max_chars &&
           udt_info.name.size > hash_max_chars) {
        // hash unique name
        U64 name_hash;
        blake3_hasher hasher; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.unique_name.str, udt_info.unique_name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> unique name map
        if (make_map) {
          str8_list_pushf(map_arena, map, "%llx %S\n", name_hash, str8_varg(udt_info.unique_name));
        }

        // parse leaf size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        String8 lambda_prefix = str8_lit("<lambda_");
        U64     colon_pos     = str8_find_needle_reverse(udt_info.name, 0, lambda_prefix, 0);
        B32     is_lambda     = colon_pos != 0;

        if (is_lambda) {
          U64 size = raddbg_snprintf(temp, sizeof(temp), "%llx", (long long)name_hash);
          Assert(size < udt_info.name.size);
          Assert(size < udt_info.unique_name.size);
          MemoryCopy(udt_info.name.str, temp, size+1);
          MemoryCopy(udt_info.name.str+size+1, temp, size+1);
          udt_info.name.size        = size;
          udt_info.unique_name.size = size;

          // update leaf header
          U64 new_size = sizeof(CV_LeafKind) +
                                sizeof(CV_LeafStruct) +
                                numeric_size +
                                udt_info.name.size + 1 +
                                udt_info.unique_name.size + 1;
          CV_LeafHeader *header = (CV_LeafHeader *)leaf_arr[leaf_idx];
          Assert(new_size <= max_U16);
          memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);
        } else {
          // replace uniuqe type name with hash
          udt_info.unique_name.str  = udt_info.name.str + udt_info.name.size + 1;
          udt_info.unique_name.size = raddbg_snprintf((char *)udt_info.unique_name.str, udt_info.unique_name.size, "%llx", (long long)name_hash);

          // update leaf header
          U64 new_size = sizeof(CV_LeafKind) +
                                sizeof(CV_LeafStruct) +
                                numeric_size +
                                udt_info.name.size + 1 +
                                udt_info.unique_name.size + 1;
          CV_LeafHeader *header = (CV_LeafHeader *)leaf_arr[leaf_idx];
          Assert(new_size <= max_U16);
          memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);
        }
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_replace_type_names_with_hashes_full_task)
{
  ProfBeginFunction();

  LNK_TypeNameReplacer *task        = raw_task;
  Rng1U64               range       = task->ranges[task_id];
  U64                   leaf_count  = task->leaf_count;
  U8                  **leaf_arr    = task->leaf_arr;
  U64                   hash_length = task->hash_length;

  B32          make_map  = task->make_map;
  Arena       *map_arena = 0;
  String8List *map       = 0;
  if (make_map) {
    map_arena = task->map_arena->v[task_id];
    map       = &task->maps[task_id];
  }

  U64 hash_max_chars = hash_length*2;

  for EachInRange(leaf_idx, range) {
    CV_Leaf leaf = cv_leaf_from_ptr(leaf_arr[leaf_idx]);
    if (leaf.kind == CV_LeafKind_STRUCTURE || leaf.kind == CV_LeafKind_CLASS) {
      CV_UDTInfo udt_info = cv_get_udt_info(leaf.kind, leaf.data);

      if (udt_info.name.size > hash_max_chars) {
        // pick name to hash
        String8 name;
        if (udt_info.props & CV_TypeProp_HasUniqueName) {
          name = udt_info.unique_name;
        } else {
          name = udt_info.name;
        }

        // hash name
        U64 name_hash;
        blake3_hasher hasher = {0}; blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, udt_info.name.str, udt_info.name.size);
        blake3_hasher_finalize(&hasher, (U8*)&name_hash, sizeof(name_hash));

        // emit hash -> name map
        if (make_map) {
          str8_list_pushf(map_arena, map, "%llx %S\n", name_hash, name);
        }

        // replace name with hash
        udt_info.name.size = raddbg_snprintf((char *)udt_info.name.str, udt_info.name.size, "%llx", (long long)name_hash);

        // parse struct size
        CV_NumericParsed dummy;
        U64 numeric_size = cv_read_numeric(leaf.data, sizeof(CV_LeafStruct), &dummy);

        // update header
        U64            new_size = sizeof(CV_LeafKind) + sizeof(CV_LeafStruct) + numeric_size + udt_info.name.size + 1;
        CV_LeafHeader *header   = (CV_LeafHeader *)leaf_arr[leaf_idx];
        Assert(new_size <= max_U16);
        memory_write16(MemberFromPtr(CV_LeafHeader, header, size), (U16)new_size);

        // discard unique name
        CV_LeafStruct *lf = (CV_LeafStruct *)(header + 1);
        lf->props &= ~CV_TypeProp_HasUniqueName;
      }
    }
  }

  ProfEnd();
}

internal void
lnk_replace_type_names_with_hashes(TP_Context *tp, TP_Arena *arena, U64 leaf_count, U8 **leaf_arr, LNK_TypeNameHashMode mode, U64 hash_length, String8 map_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  // init task context
  LNK_TypeNameReplacer task = {0};
  task.leaf_count           = leaf_count;
  task.leaf_arr             = leaf_arr;
  task.ranges               = tp_divide_work(scratch.arena, leaf_count, tp->worker_count);
  task.hash_length          = Clamp(1, hash_length, 16);

  if (map_name.size > 0) {
    task.make_map  = 1;
    task.map_arena = tp_arena_alloc(tp);
    task.maps      = push_array(scratch.arena, String8List, tp->worker_count);
  }

  // pick task function
  TP_TaskFunc *func = 0;
  switch (mode) {
  case LNK_TypeNameHashMode_Null: 
  case LNK_TypeNameHashMode_None:
    break;

  case LNK_TypeNameHashMode_Lenient: func = lnk_replace_type_names_with_hashes_lenient_task; break;
  case LNK_TypeNameHashMode_Full:    func = lnk_replace_type_names_with_hashes_full_task;    break;
  }

  // run task
  tp_for_parallel(tp, arena, tp->worker_count, func, &task);

  // optionally write out map file 
  if (task.make_map) {
    String8List map = {0};
    str8_list_concat_in_place_array(&map, task.maps, tp->worker_count);
    lnk_write_data_list_to_file_path(map_name, str8_zero(), map);
    tp_arena_release(&task.map_arena);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_move_global_symbols_to_gsi)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildPdb   *task = raw_task;
  PDB_GsiContext *gsi  = task->pdb->gsi;
  PDB_PsiContext *psi  = task->pdb->psi;
  Assert(gsi->bucket_count == PDB_GSI_V70_BUCKET_COUNT);
  Assert(psi->gsi->bucket_count == PDB_GSI_V70_BUCKET_COUNT);

  ProfBegin("Global Symbols");
  {
    // Exact dedup + compaction ran inside the module-write phase while each transformed $S
    // window was alive; section backing is already released by the time this pass runs.
    // Consume only materialized winner records (pointer array in hash-table slot order,
    // payload on surviving per-worker arenas): hash names, size buckets, sharded fill. This
    // pass never touches $S. Winner-array order is schedule-dependent, but every GSI bucket
    // chain is content-sorted at serialization (gsi_symbol_is_before), so output is stable.
    U64    symbol_count = task->gsi_winner_count;
    void **symbol_arr   = task->gsi_winner_ptrs; // [symbol_count] materialized copies

    Rng1U64 *symbol_ranges = 0; // [worker_count]
    U32     *symbol_hashes = 0; // [symbol_count]
    if (task_id == 0) {
      symbol_ranges = tp_divide_work(scratch.arena, symbol_count, tp->worker_count);
      symbol_hashes = push_array_no_zero(scratch.arena, U32, symbol_count ? symbol_count : 1);
    }
    tp_broadcast(&symbol_ranges);
    tp_broadcast(&symbol_hashes);

    // hash symbols (reads the materialized winner bytes -- byte-identical to the $S records
    // they were copied from, so every hash value matches the pre-direct-dedup flow)
    Rng1U64 symbol_range = symbol_ranges[task_id];
    for EachInRange(i, symbol_range) {
      CV_Symbol symbol = cv_symbol_from_ptr(symbol_arr[i]);
      String8   name   = cv_name_from_symbol(symbol.kind, symbol.data);
      symbol_hashes[i] = gsi_hash(gsi, name);
    }
    barrier_wait(tp->barrier);

    // size buckets up front so each one reallocs at most once for this wave (arena pushes are
    // single-threaded on task 0; sizing has no determinism impact)
    if (task_id == 0) {
      U64 *bucket_adds = push_array(scratch.arena, U64, gsi->bucket_count);
      for EachIndex(i, symbol_count) { bucket_adds[symbol_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1)] += 1; }
      for EachIndex(bucket_idx, gsi->bucket_count) {
        if (bucket_adds[bucket_idx]) { gsi_reserve(gsi, bucket_idx, bucket_adds[bucket_idx]); }
      }
    }
    barrier_wait(tp->barrier);

    // push global symbols, sharded by bucket range: worker i owns buckets [i*B/W, (i+1)*B/W) and
    // walks the FULL symbol sequence in global order, inserting only symbols whose bucket lands in
    // its range. each bucket has a single owner and receives its inserts in global sequence order,
    // so per-bucket order (which is serialized into the PDB) is byte-identical to a serial loop,
    // for any worker count -- no locks, no atomics.
    {
      U64 shard_min = (task_id * gsi->bucket_count) / tp->worker_count;
      U64 shard_max = ((task_id + 1) * gsi->bucket_count) / tp->worker_count;
      for EachIndex(i, symbol_count) {
        U64 bucket_idx = symbol_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1);
        if (bucket_idx < shard_min || bucket_idx >= shard_max) { continue; }
        PDB_GsiSymbolBucket *bucket = &gsi->bucket_arr[bucket_idx];
        CV_Symbol *dst = &bucket->v[bucket->count];
        // the bucket value points at the winner's MATERIALIZED copy (made at the end of Write
        // Modules, byte-identical to the original record, alive through GSI serialization) --
        // no copy and no $S read here
        *dst = cv_symbol_from_ptr(symbol_arr[i]);
        // deterministic same-name tie-break for the per-bucket sort in gsi_serialize_symbols_task
        // (gsi_symbol_is_before compares name -> offset -> kind -> data bytes): key on the content
        // hash of the full raw record, never a slot/array position. The winner ARRAY order is a
        // dedup-implementation detail (currently compacted open-addressing slot order) and
        // per-bucket insert order follows it -- the content-keyed sort is what makes the
        // serialized bytes invariant to it. On collision the comparator's kind/data-bytes
        // fallback stays content-deterministic, and byte-identical records cannot reach the
        // sort (dedup folds them), so the pointer tiebreaker stays unreachable. Hashing the
        // materialized copy yields the exact pre-dedup-restructure value (bytes identical).
        dst->offset = u64_hash_from_str8(cv_raw_from_symbol(symbol_arr[i]));
        bucket->count += 1;
      }
    }
    barrier_wait(tp->barrier);
    if (task_id == 0) { gsi->symbol_count += symbol_count; }
  }
  ProfEnd();

  ProfBegin("Proc Refs");
  {
    // P2b: proc-refs were pre-built per obj at module write (payloads on the surviving
    // procref_payload_arenas); flatten the per-obj segments POSITIONALLY in ascending
    // obj-index order -- deterministic for any cohort width or schedule, never completion
    // order. (The old flat order was the obj_indices lane concatenation, which was already
    // cohort-dependent and relied on the content sort at serialization; obj-index order is
    // strictly more deterministic.) Zero $S reads.
    U64        total_proc_ref_count = 0;
    U64       *procref_offsets      = 0; // [obj_count+1] prefix sums in obj-index order
    U32       *proc_ref_hashes      = 0; // [total_proc_ref_count]
    CV_Symbol *proc_ref_symbols     = 0; // [total_proc_ref_count]
    if (task_id == 0) {
      procref_offsets = push_array_no_zero(scratch.arena, U64, task->cv->obj_count + 1);
      U64 acc = 0;
      for EachIndex(obj_idx, task->cv->obj_count) {
        procref_offsets[obj_idx] = acc;
        acc += task->preext[obj_idx].procref_count;
      }
      procref_offsets[task->cv->obj_count] = acc;
      total_proc_ref_count = acc;
      proc_ref_hashes  = push_array_no_zero(scratch.arena, U32,       total_proc_ref_count ? total_proc_ref_count : 1);
      proc_ref_symbols = push_array_no_zero(scratch.arena, CV_Symbol, total_proc_ref_count ? total_proc_ref_count : 1);
    }
    tp_broadcast(&total_proc_ref_count);
    tp_broadcast(&procref_offsets);
    tp_broadcast(&proc_ref_hashes);
    tp_broadcast(&proc_ref_symbols);

    for (U64 obj_idx = task_id; obj_idx < task->cv->obj_count; obj_idx += tp->worker_count) {
      LNK_GsiPreExtractObj *pre = &task->preext[obj_idx];
      if (pre->procref_count == 0) { continue; }
      MemoryCopyTyped(&proc_ref_symbols[procref_offsets[obj_idx]], pre->procref_syms,   pre->procref_count);
      MemoryCopyTyped(&proc_ref_hashes [procref_offsets[obj_idx]], pre->procref_hashes, pre->procref_count);
    }
    barrier_wait(tp->barrier);

    // size buckets up front so each one reallocs at most once for this wave (arena pushes are
    // single-threaded on task 0; sizing has no determinism impact)
    if (task_id == 0) {
      U64 *bucket_adds = push_array(scratch.arena, U64, gsi->bucket_count);
      for EachIndex(i, total_proc_ref_count) { bucket_adds[proc_ref_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1)] += 1; }
      for EachIndex(bucket_idx, gsi->bucket_count) {
        if (bucket_adds[bucket_idx]) { gsi_reserve(gsi, bucket_idx, bucket_adds[bucket_idx]); }
      }
    }
    barrier_wait(tp->barrier);

    // push proc refs, sharded by bucket range (single owner per bucket, inserts in global
    // order -> per-bucket order identical to a serial loop for any worker count)
    {
      U64 shard_min = (task_id * gsi->bucket_count) / tp->worker_count;
      U64 shard_max = ((task_id + 1) * gsi->bucket_count) / tp->worker_count;
      for EachIndex(i, total_proc_ref_count) {
        U64 bucket_idx = proc_ref_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1);
        if (bucket_idx < shard_min || bucket_idx >= shard_max) { continue; }
        PDB_GsiSymbolBucket *bucket = &gsi->bucket_arr[bucket_idx];
        bucket->v[bucket->count] = proc_ref_symbols[i];
        bucket->count += 1;
      }
    }
    barrier_wait(tp->barrier);
    if (task_id == 0) { gsi->symbol_count += total_proc_ref_count; }
  }
  ProfEnd();

  // Patched debug-section copies were released at the end of the module-write phase; nothing
  // here reads $S. Global winner bytes and the procref value/hash arrays + payloads consumed
  // above live on the surviving procref_payload_arenas through GSI serialization.

  ProfBegin("Public Symbols");
  {
    // FAIR-SHARE: task->symtab->chunks is a FIXED [symtab->arena->count] partition built at
    // full pool width, but this barrier pass runs at the pinned cohort C == tp->worker_count
    // (C <= fixed). Walk the fixed lanes strided by the cohort so every fixed lane is
    // processed exactly once for any C, and keep every per-lane array in FIXED-lane order so
    // the flattened global order below (which feeds the sharded PSI insert) is byte-identical
    // to a full-width run. At C == fixed the strided loops degenerate to lane == task_id.
    U64 fixed_lane_count = task->symtab->arena->count;

    U64 *public_symbol_sizes       = 0; // [fixed_lane_count]
    U64 *public_symbol_node_counts = 0; // [fixed_lane_count]
    if (task_id == 0) {
      public_symbol_sizes       = push_array(scratch.arena, U64, fixed_lane_count);
      public_symbol_node_counts = push_array(scratch.arena, U64, fixed_lane_count);
    }
    tp_broadcast(&public_symbol_sizes);
    tp_broadcast(&public_symbol_node_counts);

    // compute buffer size for CV public symbols
    for (U64 lane = task_id; lane < fixed_lane_count; lane += tp->worker_count) {
      LNK_SymbolHashTrieChunkList symbol_chunks       = task->symtab->chunks[lane];
      U64                         public_symbol_size  = 0;
      U64                         public_symbol_count = 0;
      for EachNode(chunk, LNK_SymbolHashTrieChunk, symbol_chunks.first) {
        for EachIndex(i, chunk->count) {
          LNK_Symbol        *symbol        = chunk->v[i].symbol;
          LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
          COFF_ParsedSymbol  symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symbol_ref.obj, symbol_ref.symbol_idx);

          if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }
          COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
          if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

          public_symbol_size  += AlignPow2(sizeof(CV_SymPub32) + symbol->name.size + 1, sizeof(void *));
          public_symbol_count += 1;
          public_symbol_node_counts[lane] += 1;
        }
      }
      public_symbol_sizes      [lane] += public_symbol_size;
      public_symbol_node_counts[lane] += public_symbol_count;
    }
    barrier_wait(tp->barrier);

    Arena         **public_symbol_arenas      = 0;
    CV_Symbol     **public_symbol_vals        = 0; // [fixed_lane_count][public_symbol_lane_counts[lane]]
    U64            *public_symbol_lane_counts = 0; // [fixed_lane_count]
    U32           **public_symbol_hashes      = 0; // [fixed_lane_count][public_symbol_lane_counts[lane]]
    if (task_id == 0) {
      public_symbol_arenas      = alloc_arena_many(psi->gsi->arena, fixed_lane_count, public_symbol_sizes);
      public_symbol_vals        = push_array(scratch.arena, CV_Symbol *, fixed_lane_count);
      public_symbol_lane_counts = push_array(scratch.arena, U64, fixed_lane_count);
      public_symbol_hashes      = push_array(scratch.arena, U32 *, fixed_lane_count);
    }
    tp_broadcast(&public_symbol_arenas);
    tp_broadcast(&public_symbol_vals);
    tp_broadcast(&public_symbol_lane_counts);
    tp_broadcast(&public_symbol_hashes);

    // make CV public symbols (per-lane CV_Symbol value arrays on scratch; payload bytes stay on
    // public_symbol_arenas). lane arrays live on the owning worker's scratch and are only read by
    // the same lane->worker striding below, then copied into the flat array.
    for (U64 lane = task_id; lane < fixed_lane_count; lane += tp->worker_count) {
      LNK_SymbolHashTrieChunkList symbol_chunks       = task->symtab->chunks[lane];
      Arena                      *public_symbol_arena = public_symbol_arenas[lane];
      CV_Symbol                  *vals                = push_array_no_zero(scratch.arena, CV_Symbol, public_symbol_node_counts[lane]);
      U64                         val_count           = 0;
      for EachNode(chunk, LNK_SymbolHashTrieChunk, symbol_chunks.first) {
        for EachIndex(i, chunk->count) {
          LNK_Symbol        *symbol        = chunk->v[i].symbol;
          LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
          COFF_ParsedSymbol  symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symbol_ref.obj, symbol_ref.symbol_idx);

          // discard removed and non-section symbols
          if (symbol_parsed.section_number == lnk_obj_get_removed_section_number(symbol_ref.obj)) { continue; }
          COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
          if (symbol_interp != COFF_SymbolValueInterp_Regular) { continue; }

          CV_Pub32Flags flags = COFF_SymbolType_IsFunc(symbol_parsed.type) ? CV_Pub32Flag_Function : 0;
          ISectOff      sc    = lnk_sc_from_symbol(symbol);
          Assert(val_count < public_symbol_node_counts[lane]);
          vals[val_count++] = cv_make_pub32(public_symbol_arena, flags, safe_cast_u32(sc.off), safe_cast_u16(sc.isect), symbol->name);
        }
      }
      public_symbol_vals       [lane] = vals;
      public_symbol_lane_counts[lane] = val_count;
    }
    barrier_wait(tp->barrier);

    // hash public symbols
    for (U64 lane = task_id; lane < fixed_lane_count; lane += tp->worker_count) {
      U64        lane_count = public_symbol_lane_counts[lane];
      CV_Symbol *vals       = public_symbol_vals[lane];
      U32       *hashes     = push_array(scratch.arena, U32, lane_count);
      for EachIndex(k, lane_count) {
        String8 name = cv_name_from_symbol(vals[k].kind, vals[k].data);
        hashes[k] = gsi_hash(gsi, name);
      }
      public_symbol_hashes[lane] = hashes;
    }
    barrier_wait(tp->barrier);

    // flatten the per-worker symbol arrays (in worker order, matching the old serial walk) into
    // one global-order value/hash array, so the sharded insert below can walk it
    U64        public_symbol_total_count = 0;
    U64       *public_symbol_offsets     = 0; // [fixed_lane_count]
    CV_Symbol *public_symbol_flat_vals   = 0; // [public_symbol_total_count]
    U32       *public_symbol_flat_hashes = 0; // [public_symbol_total_count]
    if (task_id == 0) {
      U64 *list_counts = push_array_no_zero(scratch.arena, U64, fixed_lane_count);
      for EachIndex(i, fixed_lane_count) { list_counts[i] = public_symbol_lane_counts[i]; }
      public_symbol_offsets     = offsets_from_counts_array_u64(scratch.arena, list_counts, fixed_lane_count);
      public_symbol_total_count = sum_array_u64(fixed_lane_count, list_counts);
      public_symbol_flat_vals   = push_array_no_zero(scratch.arena, CV_Symbol, public_symbol_total_count);
      public_symbol_flat_hashes = push_array_no_zero(scratch.arena, U32, public_symbol_total_count);
    }
    tp_broadcast(&public_symbol_total_count);
    tp_broadcast(&public_symbol_offsets);
    tp_broadcast(&public_symbol_flat_vals);
    tp_broadcast(&public_symbol_flat_hashes);
    for (U64 lane = task_id; lane < fixed_lane_count; lane += tp->worker_count) {
      U64 cursor = public_symbol_offsets[lane];
      U64 lane_count = public_symbol_lane_counts[lane];
      for EachIndex(k, lane_count) {
        public_symbol_flat_vals  [cursor] = public_symbol_vals  [lane][k];
        public_symbol_flat_hashes[cursor] = public_symbol_hashes[lane][k];
        cursor += 1;
      }
    }
    barrier_wait(tp->barrier);

    // size buckets up front so each one reallocs at most once for this wave (arena pushes are
    // single-threaded on task 0; sizing has no determinism impact)
    if (task_id == 0) {
      PDB_GsiContext *pub_gsi     = psi->gsi;
      U64            *bucket_adds = push_array(scratch.arena, U64, pub_gsi->bucket_count);
      for EachIndex(i, public_symbol_total_count) { bucket_adds[public_symbol_flat_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1)] += 1; }
      for EachIndex(bucket_idx, pub_gsi->bucket_count) {
        if (bucket_adds[bucket_idx]) { gsi_reserve(pub_gsi, bucket_idx, bucket_adds[bucket_idx]); }
      }
    }
    barrier_wait(tp->barrier);

    // insert public symbols into PSI, sharded by bucket range (single owner per bucket, inserts in
    // global order -> per-bucket order identical to a serial loop for any worker count)
    {
      PDB_GsiContext *pub_gsi   = psi->gsi;
      U64             shard_min = (task_id * pub_gsi->bucket_count) / tp->worker_count;
      U64             shard_max = ((task_id + 1) * pub_gsi->bucket_count) / tp->worker_count;
      for EachIndex(i, public_symbol_total_count) {
        U64 bucket_idx = public_symbol_flat_hashes[i] & (PDB_GSI_V70_BUCKET_COUNT - 1);
        if (bucket_idx < shard_min || bucket_idx >= shard_max) { continue; }
        PDB_GsiSymbolBucket *bucket = &pub_gsi->bucket_arr[bucket_idx];
        bucket->v[bucket->count] = public_symbol_flat_vals[i];
        bucket->count += 1;
      }
    }
    barrier_wait(tp->barrier);
    if (task_id == 0) { psi->gsi->symbol_count += public_symbol_total_count; }
  }
  ProfEnd();

  scratch_end(scratch);
}


// Streaming-ring P1.2 parity walk (debug builds only -- it doubles the reads): prove the
// dormant provenance recorded at parse time is authoritative by re-resolving every tracked
// subsection node through lnk_resolve_debug_s_node and comparing CONTENT against the node's
// String8. For reloc-PATCHED sections both the node slice and the resolver point into the
// same section_data_copies bytes (pointers may even be equal), so the assert is on content,
// which also holds across the in-place $S TI/kind fixups (they mutate the shared bytes).
// Skips untracked lists (prov count == 0: wholesale synthetic constructions) and synthetic
// nodes (no backing section). Valid only while lnk_obj_section_data_from_number still returns what the
// parse consumed, i.e. before the sect-data copies release in lnk_move_global_symbols_to_gsi.
internal void
lnk_assert_debug_s_prov_parity(LNK_Obj *obj, CV_DebugS *debug_s)
{
#if BUILD_DEBUG
  for EachElement(k, debug_s->data_list) {
    if (debug_s->prov_list[k].count == 0) { continue; } // untracked construction
    Assert(debug_s->prov_list[k].count == debug_s->data_list[k].node_count);
    CV_DebugSProvNode *prov = debug_s->prov_list[k].first;
    for (String8Node *data_n = debug_s->data_list[k].first; data_n != 0; data_n = data_n->next, prov = prov->next) {
      String8 resolved = lnk_resolve_debug_s_node(obj, prov);
      if (prov->is_synthetic) { Assert(resolved.size == 0); continue; }
      Assert(resolved.size == data_n->string.size);
      Assert(str8_match(resolved, data_n->string, 0));
    }
    Assert(prov == 0);
  }
#endif
}

internal U64
lnk_write_debug_s_to_pdb_module(PDB_DbiModule *mod, CV_DebugS debug_s, String8Node *buf, U64 *buf_pos, LNK_GsiPreExtractObj *pre)
{
  U64 mod_cursor = 0;

  mod->sym_data_size = 0;
  mod->c11_data_size = 0;
  mod->c13_data_size = 0;
  mod->globrefs_size = 0;

  String8List symbols = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);

  if (symbols.total_size) {
    // signature
    U64 sig_size = str8_buffer_write_u32(buf, buf_pos, CV_Signature_C13);
    mod->sym_data_size += sig_size;
    mod_cursor         += sig_size;

    U64 symbols_idx = cv_c13_sub_section_idx_from_kind(CV_C13SubSectionKind_Symbols);
    CV_DebugSProvList *symbol_prov = &debug_s.prov_list[symbols_idx];
    B32 use_summary = (buf == 0 && pre != 0 && symbol_prov->count == symbols.node_count);
    if (use_summary) {
      for (CV_DebugSProvNode *prov = symbol_prov->first; prov != 0; prov = prov->next) {
        if (!prov->symbol_summary_valid) { use_summary = 0; break; }
      }
    }
    if (use_summary) {
      for (CV_DebugSProvNode *prov = symbol_prov->first; prov != 0; prov = prov->next) {
        mod_cursor += prov->module_symbol_size;
        mod->sym_data_size += prov->module_symbol_size;
        pre->cand_count += prov->gsi_candidate_count;
        pre->procref_count += prov->proc_ref_count;
      }
    } else {
      // write symbols
      U64 scope_depth = 0;
      for EachNode(n, String8Node, symbols.first) {
        U64 cand_depth  = 0;
        B32 cand_active = 1;
        for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= n->string.size; ) {
          CV_Symbol symbol = {0};
          TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);

          if (pre != 0) {
            if (cand_active && (cv_is_global_symbol(symbol.kind) || (cand_depth == 0 && cv_is_typedef(symbol.kind)))) {
              pre->cand_count += 1;
            }
            if (symbol.kind == CV_SymKind_GPROC32    || symbol.kind == CV_SymKind_LPROC32 ||
                symbol.kind == CV_SymKind_GPROC32_ID || symbol.kind == CV_SymKind_LPROC32_ID) {
              pre->procref_count += 1;
            }
            if (cand_active) {
              if (cv_is_scope_symbol(symbol.kind)) {
                cand_depth += 1;
              } else if (cv_is_end_symbol(symbol.kind)) {
                if (cand_depth == 0) { Assert(0 && "malformed symbol stream"); cand_active = 0; }
                else                 { cand_depth -= 1; }
              }
            }
          }

          if      (symbol.kind == CV_SymKind_SKIP)                 { continue; }
          else if (cv_is_global_symbol(symbol.kind))               { continue; }
          else if (cv_is_typedef(symbol.kind) && scope_depth == 0) { continue; }
          else if (symbol.kind == 0x1176)                          { continue; }

          if      (cv_is_scope_symbol(symbol.kind)) { scope_depth += 1; }
          else if (cv_is_end_symbol(symbol.kind))   { scope_depth -= 1; }

          String8 raw_symbol = cv_raw_from_symbol(cv_ptr_from_symbol(symbol));
          U64 symbol_size = (raw_symbol.size & (PDB_SYMBOL_ALIGN - 1)) == 0 ? str8_buffer_write(buf, buf_pos, raw_symbol)
                                                                           : cv_write_symbol_buf(buf, buf_pos, &symbol, PDB_SYMBOL_ALIGN);
          mod_cursor         += symbol_size;
          mod->sym_data_size += symbol_size;
        }
      }
    }
  }

  // write file checksums, inlinee lines etc.
  CV_C13SubSectionKind mod_c13_layout[] = {
    CV_C13SubSectionKind_FileChksms,
    CV_C13SubSectionKind_FrameData,
    CV_C13SubSectionKind_InlineeLines,
    CV_C13SubSectionKind_CrossScopeImports,
    CV_C13SubSectionKind_CrossScopeExports,
    CV_C13SubSectionKind_IlLines,
    CV_C13SubSectionKind_FuncMDTokenMap,
    CV_C13SubSectionKind_TypeMDTokenMap,
    CV_C13SubSectionKind_MergedAssemblyInput,
    CV_C13SubSectionKind_CoffSymbolRVA,
    CV_C13SubSectionKind_XfgHashType,
    CV_C13SubSectionKind_XfgHashVirtual,
  };
  for EachElement(i, mod_c13_layout) {
    String8List data = cv_sub_section_from_debug_s(debug_s, mod_c13_layout[i]);
    if (data.total_size == 0) { continue; }
    U64 ss_size = 0;
    ss_size += str8_buffer_write(buf, buf_pos, str8_struct((&(CV_C13SubSectionHeader ){ .kind = mod_c13_layout[i], .size = safe_cast_u32(data.total_size) })));
    ss_size += str8_buffer_write_string_list(buf, buf_pos, data);
    ss_size += str8_buffer_write_zeroes(buf, buf_pos, AlignPadPow2(mod_cursor + ss_size, CV_C13SubSectionAlign));
    mod_cursor         += ss_size;
    mod->c13_data_size += ss_size;
  }

  // write line tables
  String8List lines = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);
  for EachNode(n, String8Node, lines.first) {
    if (n->string.size == 0) { continue; }
    U64 ss_size = 0;
    ss_size += str8_buffer_write(buf, buf_pos, str8_struct((&(CV_C13SubSectionHeader){ .kind = CV_C13SubSectionKind_Lines, .size = safe_cast_u32(n->string.size) })));
    ss_size += str8_buffer_write(buf, buf_pos, n->string);
    ss_size += str8_buffer_write_zeroes(buf, buf_pos, AlignPadPow2(mod_cursor + ss_size, CV_C13SubSectionAlign));
    mod_cursor         += ss_size;
    mod->c13_data_size += ss_size;
  }

  // write global refs
  if (mod->sym_data_size) {
    String8List globrefs = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_GlobalRefs);
    mod->globrefs_size += str8_buffer_write_u32(buf, buf_pos, safe_cast_u32(globrefs.total_size));
    mod->globrefs_size += str8_buffer_write_string_list(buf, buf_pos, globrefs);
    mod_cursor += mod->globrefs_size;
  }

  return mod_cursor;
}

typedef struct LNK_PdbOutput LNK_PdbOutput;
internal void lnk_pdb_output_enqueue_stream(LNK_PdbOutput *output, MSF_Context *msf, MSF_StreamNumber sn);

#define LNK_GSI_DEDUP_RESERVED ((void *)(U64)1)

// Exact concurrent content set. Claiming an empty slot with a sentinel before allocating is
// important: only the thread that adds a distinct record copies bytes, so the common duplicate
// case consumes neither transient metadata nor discarded arena space. Published pointers own
// immutable bytes on a per-worker surviving arena and are safe to compare immediately.
internal void
lnk_gsi_deduper_insert_copy(void **buckets, U64 bucket_cap, U64 hash, void *symbol_ptr, Arena *dst_arena)
{
  String8 raw      = cv_raw_from_symbol(symbol_ptr);
  U64     best_idx = lnk_hash_range(hash, bucket_cap);
  U64     idx      = best_idx;
  for (;;) {
    void *curr = ins_atomic_ptr_eval(&buckets[idx]);
    if (curr == 0) {
      void *cmp = ins_atomic_ptr_eval_cond_assign(&buckets[idx], LNK_GSI_DEDUP_RESERVED, 0);
      if (cmp == 0) {
        U8 *copy = push_array_no_zero(dst_arena, U8, raw.size);
        MemoryCopy(copy, raw.str, raw.size);
        ins_atomic_ptr_eval_assign(&buckets[idx], copy);
        return;
      }
      curr = cmp;
    }
    while (curr == LNK_GSI_DEDUP_RESERVED) { curr = ins_atomic_ptr_eval(&buckets[idx]); }
    String8 seen = cv_raw_from_symbol(curr);
    if (seen.size == raw.size && MemoryMatch(seen.str, raw.str, raw.size)) { return; }
    idx = (idx + 1 == bucket_cap) ? 0 : idx + 1;
    Assert(idx != best_idx);
  }
}

// Streaming-ring P2b: extract the obj's GSI inputs -- direct global-record winners and
// proc-refs -- inside the module-write per-obj visit, so lnk_move_global_symbols_to_gsi does
// no $S record-decode walks (its only remaining $S touch is the bucket-fill materialize of
// the dedup winners' bytes through the refs). Runs right after the obj's deferred $S fixup
// replay, so records are post-fixup (identical bytes to what the old post-modules collect
// walk saw).
//
// Faithful to the two walks it replaces:
// - candidate walk: per-NODE scope depth reset + malformed-end break (the old collect ran per
//   symbol input, and symbol inputs are exactly the Symbols data_list nodes);
// - proc-ref walk: obj-continuous scope depth + module-stream cursor (starts at
//   sizeof(CV_Signature); records the module write drops -- SKIP, globals, top-level typedefs,
//   0x1176 -- do not advance it).
// Exact global winners plus proc-ref value/hash arrays and payloads go on the surviving
// procref_payload_arenas (referenced until GSI serialization), same lifetime as the old
// proc_ref_arenas.
internal void
lnk_extract_gsi_inputs_for_obj(LNK_BuildPdb *task, U64 obj_idx, U64 task_id, CV_DebugS *debug_s_ptr)
{
  CV_DebugS   debug_s = *debug_s_ptr; // window copy (g_debug_s_window) or the patched backing
  String8List symbols = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Symbols);
  if (symbols.total_size == 0) { return; }

  LNK_GsiPreExtractObj *pre        = &task->preext[obj_idx];
  PDB_GsiContext       *gsi        = task->pdb->gsi;
  Arena                *payload_arena = task->procref_payload_arenas[task_id];

  // Dedup globals and build proc refs in one decode walk over the post-reloc/post-fixup
  // window. The size-only pass counted both record sets from invariant raw headers, so all
  // arrays and the shared table are exact-sized before this visit.
  U64 procref_count = pre->procref_count;
  if (procref_count) {
    pre->procref_syms   = push_array_no_zero(payload_arena, CV_Symbol, procref_count);
    pre->procref_hashes = push_array_no_zero(payload_arena, U32,       procref_count);
  }

  CV_ModIndex imod          = task->mod_arr[obj_idx]->imod;
  U64         symbol_cursor = sizeof(CV_Signature);
  U64         scope_depth   = 0;
  U64         cand_count    = 0;
  U64         procref_idx   = 0;
  for EachNode(n, String8Node, symbols.first) {
    U64 cand_depth  = 0;
    B32 cand_active = 1;
    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= n->string.size; ) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);

      if (cand_active && (cv_is_global_symbol(symbol.kind) || (cand_depth == 0 && cv_is_typedef(symbol.kind)))) {
        U8 *ptr = cv_ptr_from_symbol(symbol);
        lnk_gsi_deduper_insert_copy(task->gsi_dedup_buckets, task->gsi_dedup_bucket_cap,
                                    u64_hash_from_str8(cv_raw_from_symbol(ptr)), ptr, payload_arena);
        cand_count += 1;
      }
      if (cand_active) {
        if (cv_is_scope_symbol(symbol.kind)) {
          cand_depth += 1;
        } else if (cv_is_end_symbol(symbol.kind)) {
          if (cand_depth == 0) { Assert(0 && "malformed symbol stream"); cand_active = 0; }
          else                 { cand_depth -= 1; }
        }
      }

      B32 is_module_symbol = (symbol.kind != CV_SymKind_SKIP &&
                              !cv_is_global_symbol(symbol.kind) &&
                              !(cv_is_typedef(symbol.kind) && scope_depth == 0) &&
                              symbol.kind != 0x1176);
      if (is_module_symbol) {
        if      (cv_is_scope_symbol(symbol.kind)) { scope_depth += 1; }
        else if (cv_is_end_symbol(symbol.kind))   { scope_depth -= 1; }

        if (symbol.kind == CV_SymKind_GPROC32 || symbol.kind == CV_SymKind_LPROC32) {
          String8 name = cv_name_from_symbol(symbol.kind, symbol.data);
          pre->procref_syms[procref_idx] = cv_make_proc_ref(payload_arena, imod, symbol_cursor, name, cv_is_lproc(symbol));
          pre->procref_syms[procref_idx].offset = symbol_cursor;
          pre->procref_hashes[procref_idx] = gsi_hash(gsi, name);
          procref_idx += 1;
        }

        symbol_cursor += cv_write_symbol_buf(0, 0, &symbol, PDB_SYMBOL_ALIGN);
      }
    }
  }
  Assert(cand_count == pre->cand_count);
  Assert(procref_idx == procref_count);
}

internal
THREAD_POOL_TASK_FUNC(lnk_write_pdb_modules)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildPdb *task        = raw_task;
  U32Array      obj_indices = task->obj_indices[task_id];

  // compute sizes for module streams
  for EachIndex(i, obj_indices.count) {
    U64 obj_idx = obj_indices.v[i];
    // P3.4: sizing needs NO window fill. Every term of the module stream size is either pure
    // subsection metadata (C13 layout kinds come from the data_list INDEX, sizes/alignment
    // from node sizes) or -- for sym_data_size -- a read-only walk of the Symbols records'
    // size prefixes + kinds. Both are invariant under the window transforms: relocs and TI
    // fixups never touch record size/kind fields, and the journal's kind REWRITES are
    // classification-invariant for every predicate the sizing walk uses
    // (cv_is_scope_symbol treats GPROC32_ID/LPROC32_ID like their non-ID rewrites,
    // cv_is_end_symbol treats PROC_ID_END like END, and no rewrite produces or removes a
    // global/typedef/SKIP/0x1176 kind). So size straight off the raw mapped view under
    // g_debug_s_window (the patched backing otherwise) -- the write pass below asserts the
    // re-accumulated sizes match. Fixup replay, parity, and the GSI extraction all move to
    // the write pass's single fill.
    lnk_write_debug_s_to_pdb_module(task->mod_arr[obj_idx], task->cv->debug_s_arr[obj_idx], 0, 0, &task->preext[obj_idx]);
  }
  barrier_wait(tp->barrier);

  // Allocate module streams and the exact global-symbol content set. Candidate count depends
  // only on record headers, so this table is ready before the one transformed window visit.
  if (task_id == 0) {
    U64 cand_count = 0;
    for EachIndex(obj_idx, task->cv->obj_count) { cand_count += task->preext[obj_idx].cand_count; }
    task->gsi_dedup_bucket_cap = Max(cand_count * 13 / 10, 1);
    task->gsi_dedup_buckets    = push_array(scratch.arena, void *, task->gsi_dedup_bucket_cap);
    for EachIndex(obj_idx, task->cv->obj_count) {
      PDB_DbiModule *mod = task->mod_arr[obj_idx];
      U64 mod_size = mod->sym_data_size + mod->c11_data_size + mod->c13_data_size + mod->globrefs_size;
      if (mod_size > 0) {
        task->mod_arr[obj_idx]->sn = msf_stream_alloc_ex(task->pdb->msf, mod_size);
      }
    }
  }
  barrier_wait(tp->barrier);

  // write .debug$S to modules
  for EachIndex(i, obj_indices.count) {
    Temp temp = temp_begin(scratch.arena);

    U64            obj_idx  = obj_indices.v[i];
    PDB_DbiModule *mod      = task->mod_arr[obj_idx];

    if (mod->sn == MSF_INVALID_STREAM_NUMBER) {
#if BUILD_DEBUG
      // an obj skipped here must have NO Symbols payload -- otherwise sym_data_size >= sig
      // would have allocated a stream -- so skipping the extraction below loses nothing
      Assert(cv_sub_section_from_debug_s(task->cv->debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols).total_size == 0);
#endif
      temp_end(temp);
      continue;
    }

    // P3.4: THE single window fill per obj in module write (sizing above reads raw metadata/
    // headers only). Copy mode ( /OPT:GCTYPES ) consumes the patched backing; its journal was
    // consumed eagerly, so the replay call is a structural no-op.
    CV_DebugS debug_s;
    if (g_debug_s_window) {
      debug_s = lnk_obj_window_debug_s(temp.arena, task->cv, obj_idx, task->pe.image_base, task->image_section_table, 0);
    } else {
      lnk_apply_debug_s_fixups_for_obj(task->cv, obj_idx);
      debug_s = task->cv->debug_s_arr[obj_idx];
    }
    lnk_assert_debug_s_prov_parity(task->cv->obj_arr[obj_idx], &task->cv->debug_s_arr[obj_idx]); // debug-only P1.2 parity
    // P2b: globals-candidate + proc-ref extraction fused into this (now only) fill
    lnk_extract_gsi_inputs_for_obj(task, obj_idx, task_id, &debug_s);

    String8List mod_data = msf_data_from_sn(temp.arena, task->pdb->msf, mod->sn);

    if (mod_data.node_count) {
      // raw-header sizing vs post-transform write must agree byte-for-byte
      U64 size_check[4] = { mod->sym_data_size, mod->c11_data_size, mod->c13_data_size, mod->globrefs_size };
      String8Node buf = *mod_data.first;
      U64         pos = 0;
      lnk_write_debug_s_to_pdb_module(mod, debug_s, &buf, &pos, 0);
      AssertAlways(mod->sym_data_size == size_check[0]);
      AssertAlways(mod->c11_data_size == size_check[1]);
      AssertAlways(mod->c13_data_size == size_check[2]);
      AssertAlways(mod->globrefs_size == size_check[3]);

      // sub range symbol data pages and patch symbol tree offsets
      if (mod->sym_data_size) {
        Rng1U64     sym_data_range = r1u64(sizeof(CV_Signature), mod->sym_data_size);
        String8List mod_symbols    = str8_list_substr(temp.arena, mod_data, sym_data_range);
        Assert(mod_symbols.total_size == dim_1u64(sym_data_range));
        cv_patch_symbol_tree_offsets(mod_symbols, sizeof(CV_Signature), PDB_SYMBOL_ALIGN);
      }
    }

    temp_end(temp);
  }
  barrier_wait(tp->barrier);

  {
    // count strings in string tables
    U64 *string_counts;
    {
      if (task_id == 0) {
        string_counts = push_array(scratch.arena, U64, tp->worker_count);
      }
      tp_broadcast(&string_counts);

      for EachIndex(i, obj_indices.count) {
        U64       obj_idx      = obj_indices.v[i];
        CV_DebugS debug_s      = task->cv->debug_s_arr[obj_idx];
        String8   string_table = cv_string_table_from_debug_s(debug_s);
        U64       string_count = 0;
        for (U64 cursor = 0; cursor < string_table.size; cursor += 1) {
          if (string_table.str[cursor] == '\0') {
            string_counts[task_id] += 1;
          }
        }
      }
      barrier_wait(tp->barrier);
    }

    Arena **string_arenas = 0;
    if (task_id == 0) {
      string_arenas = alloc_arena_array(task->pdb->dbi->arena, tp->worker_count, string_counts, String8Node);
    }
    tp_broadcast(&string_arenas);

    for EachIndex(i, obj_indices.count) {
      Temp temp = temp_begin(scratch.arena);

      U64            obj_idx = obj_indices.v[i];
      PDB_DbiModule *mod     = task->mod_arr[obj_idx];

      if (mod->sn == MSF_INVALID_STREAM_NUMBER) { continue; }

      String8List mod_data       = msf_data_from_sn(temp.arena, task->pdb->msf, mod->sn);
      Rng1U64     c13_data_range = r1u64(mod->sym_data_size + mod->c11_data_size, mod->sym_data_size + mod->c11_data_size + mod->c13_data_size);
      String8List c13_data       = str8_list_substr(temp.arena, mod_data, c13_data_range);

      CV_DebugS debug_s      = task->cv->debug_s_arr[obj_idx];
      String8   string_table = cv_string_table_from_debug_s(debug_s);

      // checksum is always at the head of C13 data
      String8List file_chksms_raw = {0};
      {
        String8Node buf     = *c13_data.first;
        U64         buf_pos = 0;

        CV_C13SubSectionHeader header = {0};
        str8_buffer_read(&buf, &buf_pos, sizeof(header), &header);

        if (header.kind == CV_C13SubSectionKind_FileChksms) {
          Rng1U64 file_chksms_range = r1u64(sizeof(CV_C13SubSectionHeader), sizeof(CV_C13SubSectionHeader) + header.size);
          file_chksms_raw = str8_list_substr(temp.arena, c13_data, file_chksms_range);
        }
      }

      // fixup file name offsets in checksum headers
      if (file_chksms_raw.total_size) {
        String8Node buf     = *file_chksms_raw.first;
        U64         buf_pos = 0;
        U64         cursor  = 0;
        for (;;) {
          CV_C13Checksum header = {0};
          if (str8_buffer_peek(&buf, &buf_pos, sizeof(header), &header) != sizeof(header)) { break; }

          String8          name     = str8_cstring_capped(string_table.str + header.name_off, string_table.str + string_table.size);
          CV_StringBucket *bucket   = cv_string_hash_table_lookup(task->string_ht, name);
          U64              name_off = task->string_table_base_offset + bucket->u.offset;

          // update name offset
          {
            String8Node buf_copy     = buf;
            U64         buf_pos_copy = buf_pos;
            str8_buffer_skip(&buf_copy, &buf_pos_copy, OffsetOf(CV_C13Checksum, name_off));
            str8_buffer_write_u32(&buf_copy, &buf_pos_copy, safe_cast_u32(name_off));
          }

          str8_buffer_skip(&buf, &buf_pos, AlignPow2(sizeof(header) + header.len, CV_FileCheckSumsAlign));
        }
      }

      // collect mod source files
      String8List source_file_list = str8_split_by_string_chars(string_arenas[task_id], string_table, str8_lit("\0"), 0);
      {
        // the split nodes alias the string table inside the obj's $S backing (str8_split
        // does not copy) and DBI file-info hashes these bytes after the backing dies --
        // whether that backing is a debug-section COPY (released at the end of this phase)
        // or a reloc-free RAW-MAPPED view (alive today, dies in P5). P3.1: repoint
        // UNCONDITIONALLY. Every piece is present in string_ht -- the dedup task split the
        // very same table -- and its bucket bytes were rehomed to a surviving blob before
        // the strtab add, so repoint at the bucket's copy (byte-identical).
        for EachNode(n, String8Node, source_file_list.first) {
          CV_StringBucket *bucket = cv_string_hash_table_lookup(task->string_ht, n->string);
          Assert(bucket != 0);
          if (bucket != 0) { n->string = bucket->string; }
        }
      }
      str8_list_concat_in_place(&mod->source_file_list, &source_file_list);

      temp_end(temp);

      // the module stream is final (symbols + C13 written, checksum name
      // offsets patched): hand it to the background writer NOW so its pages
      // flush + decommit while the remaining modules are still being written
      // -- enqueueing all modules after the pass kept the entire module
      // payload (GB-class) committed through the globals phase
      if (task->output != 0) {
        lnk_pdb_output_enqueue_stream(task->output, task->pdb->msf, mod->sn);
      }

    }
    barrier_wait(tp->barrier);
  }

  // Compact the exact winners published during the one module-write window visit. Bucket
  // order is deterministic for a fixed content set; downstream GSI buckets content-sort
  // records before serialization, so the pointer-array order is byte-invisible.
  ProfBegin("Compact Global Symbol Winners");
  {
    U64    *compact_counts = 0;
    U64     winner_count   = 0;
    void  **winner_ptrs    = 0;
    if (task_id == 0) { compact_counts = push_array(scratch.arena, U64, tp->worker_count); }
    tp_broadcast(&compact_counts);

    U64 slot_lo = (task_id * task->gsi_dedup_bucket_cap) / tp->worker_count;
    U64 slot_hi = ((task_id + 1) * task->gsi_dedup_bucket_cap) / tp->worker_count;
    for (U64 slot = slot_lo; slot < slot_hi; slot += 1) {
      void *p = task->gsi_dedup_buckets[slot];
      Assert(p != LNK_GSI_DEDUP_RESERVED);
      compact_counts[task_id] += (p != 0);
    }
    barrier_wait(tp->barrier);

    for EachIndex(w, tp->worker_count) { winner_count += compact_counts[w]; }
    if (task_id == 0) {
      winner_ptrs = push_array_no_zero(task->pdb->gsi->arena, void *, winner_count ? winner_count : 1);
    }
    tp_broadcast(&winner_ptrs);

    U64 dst = 0;
    for (U64 w = 0; w < task_id; w += 1) { dst += compact_counts[w]; }
    for (U64 slot = slot_lo; slot < slot_hi; slot += 1) {
      void *p = task->gsi_dedup_buckets[slot];
      if (p != 0) { winner_ptrs[dst++] = p; }
    }
    barrier_wait(tp->barrier);
    if (task_id == 0) {
      task->gsi_winner_count = winner_count;
      task->gsi_winner_ptrs  = winner_ptrs;
    }
  }
  ProfEnd();

    // Copies release: moved here from mid-globals. Direct winner materialization above was
    // the LAST $S reader on this path -- release the patched debug-section copies before the
    // globals/publics staging begins instead of overlapping it. Same /PDBSTRIPPED gate as
    // before (its second build re-walks cv->debug_s_arr Symbols after lnk_build_pdb returns;
    // the materialize still ran -- winner ptrs never dangle either way). The barrier above
    // guarantees every worker is done reading before task 0 releases section copies.
    if (task_id == 0) {
      if (task->free_sect_copies) {
        ProfScope("Release Sect Data Copies") {
          for EachIndex(obj_idx, task->cv->obj_count) {
            lnk_obj_drop_section_data_copies(task->cv->obj_arr[obj_idx]);
          }
          for EachIndex(i, g_sect_copy_arena_count) {
            if (g_sect_copy_arenas[i] != 0) { arena_release(g_sect_copy_arenas[i]); g_sect_copy_arenas[i] = 0; }
          }
        }
      }
    }

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_push_dbi_sec_contrib_task)
{
  // TODO: compute CRC for relocations

  U64            obj_idx = task_id;
  LNK_BuildPdb  *task    = raw_task;
  PDB_DbiModule *mod     = task->mod_arr    [obj_idx];
  LNK_Obj       *obj     = task->cv->obj_arr[obj_idx];

  PDB_DbiSCArray *sc_array = &task->sc_arrays[obj_idx];
  sc_array->v   = push_array_no_zero(arena, PDB_DbiSC, obj->coff.sections.count_no_null);
  sc_array->cap = obj->coff.sections.count_no_null;

  for LNK_EachCoffSection(it, obj) {
    // filter by section flags
    if (*it.v.flags & (COFF_SectionFlag_LnkInfo | COFF_SectionFlag_LnkRemove | LNK_SECTION_FLAG_DEBUG)) { continue; }

    // skip unwind info for the section contribution
    String8 section_name = lnk_obj_section_name_from_section_number(obj, it.v.section_number);
    if (str8_match(section_name, str8_lit(".pdata"), 0)) { continue; }

    // load section and determine its type
    LNK_ObjSection  section = it.v;
    B32             is_virt = !!(*section.flags & COFF_SectionFlag_CntUninitializedData);

    // pick section range
    Rng1U64 section_range = is_virt ? section.vrange : section.frange;
    if (dim_1u64(section_range) == 0) { continue; }

    // map the SC offset to the image section range that contains it
    Rng1U64Array *image_ranges  = is_virt ? &task->image_section_virt_ranges : &task->image_section_file_ranges;
    U64           search_result = rng1u64_array_num_from_value__binary_search(image_ranges, section_range.min);

    // log & skip SC offsets that failed to map
    if (search_result == 0) {
      Temp scratch = scratch_begin(0,0);
      lnk_log(LNK_Log_Debug, "%S: failed to map section offset 0x%llx into the linked image; skipping this section", lnk_loc_from_obj(scratch.arena, obj), section_range.min);
      scratch_end(scratch);
      continue;
    }

    // unpack image range index
    U64 range_idx = search_result - 1;

    // fill out & push section contribution
    PDB_DbiSC *sc = &sc_array->v[sc_array->count++];
    sc->base.sec     = (U16)(is_virt ? range_idx : task->image_section_file_section_numbers[range_idx]);
    sc->base.pad0    = 0;
    sc->base.sec_off = section_range.min - image_ranges->v[range_idx].min;
    sc->base.size    = dim_1u64(section_range);
    sc->base.flags   = *section.flags;
    sc->base.mod     = mod->imod;
    sc->base.pad1    = 0;
    sc->data_crc     = is_virt ? 0 : crc32_from_string(str8_substr(task->image_data, section_range));
    sc->reloc_crc    = 0;
  }

  // find first code section contribution for the Mod1::fUpdateSecContrib
  for EachIndex(i, sc_array->count) {
    if (sc_array->v[i].base.flags & COFF_SectionFlag_CntCode) {
      mod->first_sc = sc_array->v[i];
      break;
    }
  }
}

struct LNK_PdbOutput
{
  LNK_BackgroundFileWriter *writer;
  LNK_BackgroundFile       *file;
  MSF_StreamNumber         *sealed_streams;
  U64                       sealed_stream_count;
  U64                       sealed_stream_cap;
  B32                       decommit_flushed; // off when /RAD_DEBUG re-reads the PDB page memory for RDI conversion
};

typedef struct LNK_MsfPageCursor
{
  MSF_PageDataNode *node;
  U64               node_idx;
  U64               pages_per_node;
} LNK_MsfPageCursor;

internal U8 *
lnk_msf_data_from_pn(LNK_MsfPageCursor *cursor, MSF_Context *msf, MSF_PageNumber pn)
{
  U64 node_idx = pn / cursor->pages_per_node;
  if (node_idx < cursor->node_idx) {
    cursor->node     = msf->page_data_list.first;
    cursor->node_idx = 0;
  }
  while (cursor->node_idx < node_idx) {
    cursor->node = cursor->node->next;
    cursor->node_idx += 1;
  }
  Assert(cursor->node != 0);
  return cursor->node->data + (pn % cursor->pages_per_node) * msf->page_size;
}

internal void
lnk_pdb_output_enqueue_stream(LNK_PdbOutput *output, MSF_Context *msf, MSF_StreamNumber sn)
{
  if (sn == MSF_INVALID_STREAM_NUMBER) { return; }
  // atomic slot: module streams enqueue from pool workers as each module
  // finishes; order in sealed_streams is irrelevant (set semantics for the
  // remaining-pages bitmap), file writes are positional, so output bytes are
  // unaffected by completion order
  U64 slot = ins_atomic_u64_add_eval(&output->sealed_stream_count, 1) - 1;
  Assert(slot < output->sealed_stream_cap);
  output->sealed_streams[slot] = sn;

  MSF_Stream *stream = msf_find_stream(msf, sn);
  Assert(stream != 0);
  if (stream->page_list.count == 0) { return; }

  LNK_MsfPageCursor cursor = {
    .node           = msf->page_data_list.first,
    .pages_per_node = msf_get_data_node_size(msf->page_size) / msf->page_size,
  };
  MSF_PageNumber run_first_pn = 0;
  MSF_PageNumber run_last_pn  = 0;
  U8            *run_data     = 0;
  U64            run_size     = 0;
  for EachNode(page, MSF_PageNode, stream->page_list.first) {
    U8 *page_data = lnk_msf_data_from_pn(&cursor, msf, page->pn);
    if (run_data != 0 && page->pn == run_last_pn + 1 && page_data == run_data + run_size) {
      run_last_pn = page->pn;
      run_size += msf->page_size;
    } else {
      if (run_data != 0) {
        lnk_background_file_writer_enqueue(output->writer, output->file, (U64)run_first_pn * msf->page_size, str8(run_data, run_size), output->decommit_flushed);
      }
      run_first_pn = run_last_pn = page->pn;
      run_data = page_data;
      run_size = msf->page_size;
    }
  }
  if (run_data != 0) {
    lnk_background_file_writer_enqueue(output->writer, output->file, (U64)run_first_pn * msf->page_size, str8(run_data, run_size), output->decommit_flushed);
  }
}

internal void
lnk_pdb_output_finalize_stream(void *user_data, MSF_Context *msf, MSF_StreamNumber sn)
{
  lnk_pdb_output_enqueue_stream(user_data, msf, sn);
}

// Timers telemetry: cumulative enqueued/completed bytes at a named point in the build
internal void
lnk_pdb_output_log_mark(LNK_PdbOutput *output, char *tag)
{
  if (output == 0) { return; }
  lnk_log(LNK_Log_Timers, "[pdbw] t=%.3fs %s: enq=%.2f GiB done=%.2f GiB",
          (F64)(now_time_us() - output->writer->begin_time_us) / 1e6, tag,
          (F64)ins_atomic_u64_eval(&output->writer->bytes_enqueued) / GB(1),
          (F64)ins_atomic_u64_eval(&output->writer->bytes_completed) / GB(1));
}

internal void
lnk_pdb_output_enqueue_remaining(LNK_PdbOutput *output, MSF_Context *msf)
{
  Temp scratch = scratch_begin(0,0);
  U64 save_size  = msf_get_save_size(msf);
  U64 page_count = CeilIntegerDiv(save_size, msf->page_size);
  U8 *is_written = push_array(scratch.arena, U8, page_count);

  for EachIndex(i, output->sealed_stream_count) {
    MSF_Stream *stream = msf_find_stream(msf, output->sealed_streams[i]);
    Assert(stream != 0);
    for EachNode(page, MSF_PageNode, stream->page_list.first) {
      Assert(page->pn < page_count);
      is_written[page->pn] = 1;
    }
  }

  LNK_MsfPageCursor cursor = {
    .node           = msf->page_data_list.first,
    .pages_per_node = msf_get_data_node_size(msf->page_size) / msf->page_size,
  };
  U64 run_first_pn = 0;
  U8 *run_data = 0;
  U64 run_size = 0;
  for EachIndex(pn, page_count) {
    U8 *page_data = lnk_msf_data_from_pn(&cursor, msf, pn);
    U64 page_size = Min(msf->page_size, save_size - pn * msf->page_size);
    if (!is_written[pn]) {
      if (run_data != 0 && page_data == run_data + run_size) {
        run_size += page_size;
      } else {
        if (run_data != 0) {
          lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size), /*decommit*/ 0); // MSF metadata (FPM/header/stream table) is read after this enqueue
        }
        run_first_pn = pn;
        run_data = page_data;
        run_size = page_size;
      }
    } else if (run_data != 0) {
      lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size), /*decommit*/ 0); // MSF metadata (FPM/header/stream table) is read after this enqueue
      run_data = 0;
      run_size = 0;
    }
  }
  if (run_data != 0) {
    lnk_background_file_writer_enqueue(output->writer, output->file, run_first_pn * msf->page_size, str8(run_data, run_size), /*decommit*/ 0); // MSF metadata (FPM/header/stream table) is read after this enqueue
  }
  scratch_end(scratch);
}

////////////////////////////////
// Type Garbage Collection
//
// After type merging, prune merged TPI/IPI leaves that are not reachable from any surviving
// symbol record (the GC roots), compact them, and remap all type indices. link.exe keeps a
// large unreferenced-type set; pruning it is a transparent PDB-size win (debug-info only -- the
// image is untouched). Runs on the final post-fixup type indices in place.

typedef struct LNK_GCTypes
{
  LNK_CodeViewInput *cv;
  U64                min   [CV_TypeIndexSource_COUNT]; // first type index per source
  U64                orig_n[CV_TypeIndexSource_COUNT]; // pre-GC leaf count per source
  U8                *mark    [CV_TypeIndexSource_COUNT]; // reachable bitmap, indexed by (ti - min)
  CV_TypeIndex      *remap   [CV_TypeIndexSource_COUNT]; // old leaf idx -> new type index
  U8               **leaf_v  [CV_TypeIndexSource_COUNT]; // original leaf pointer arrays
  U32               *udt_next;                           // TPI fwdref<->definition unique_name ring
  Rng1U64           *sym_ranges;
  B32                do_rewrite;                         // 0 = mark roots, 1 = rewrite to compacted indices
  // transitive-closure frontier: indices marked but not yet expanded. Each leaf is appended once
  // (the atomic mark gates it), so frontier[s] is sized orig_n[s] and fcount[s] is its atomic tail.
  U32               *frontier[CV_TypeIndexSource_COUNT];
  U32               *fcount  [CV_TypeIndexSource_COUNT]; // atomic append cursor per source
  // per-source scratch (set before dispatch)
  CV_TypeIndexSource cur_source;
  U8               **cur_leaf_v;
  Rng1U64           *cur_ranges;
  U64                round_begin, round_end;             // frontier slice processed this round
} LNK_GCTypes;

typedef struct LNK_GCNamePair { U64 hash; U32 idx; } LNK_GCNamePair;

internal int
lnk_gc_name_pair_is_before(void *raw_a, void *raw_b)
{
  LNK_GCNamePair *a = raw_a, *b = raw_b;
  return a->hash != b->hash ? (a->hash < b->hash) : (a->idx < b->idx);
}

internal void
lnk_gc_mark_ti(LNK_GCTypes *g, CV_TypeIndexSource s, CV_TypeIndex ti)
{
  U64 lo = g->min[s];
  if (ti >= lo) { U64 idx = ti - lo; if (idx < g->orig_n[s]) { g->mark[s][idx] = 1; } }
}

// walk a record's type-index sites; mark roots (do_rewrite==0) or rewrite to compacted indices (==1)
internal void
lnk_gc_visit_offsets(LNK_GCTypes *g, String8 data, CV_TiOffsets ti_offs)
{
  for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&ti_offs); ti_idx < ti_count; ti_idx += 1) {
    CV_TiOff     n  = cv_ti_offset_at(&ti_offs, ti_idx);
    U8          *p  = data.str + n.offset;
    CV_TypeIndex ti = memory_read32(p);
    if (g->do_rewrite) {
      U64 lo = g->min[n.source];
      if (ti >= lo) { U64 idx = ti - lo; if (idx < g->orig_n[n.source]) { memory_write32(p, g->remap[n.source][idx]); } }
    } else {
      lnk_gc_mark_ti(g, n.source, ti);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_gc_syms_task)
{
  LNK_GCTypes *g = raw_task;
  Temp scratch = scratch_begin(0, 0);
  for EachInRange(i, g->sym_ranges[task_id]) {
    LNK_SymbolInput symbols = g->cv->symbol_inputs[i];
    for (U64 cursor = 0; cursor + sizeof(CV_SymbolHeader) <= symbols.raw_symbols.size; ) {
      CV_Symbol symbol = {0};
      TryReadBreak(cv_read_symbol(symbols.raw_symbols, cursor, CV_SymbolAlign, &symbol), cursor);
      lnk_gc_visit_offsets(g, symbol.data, cv_symbol_ti_offsets(symbol.kind, symbol.data));
    }
  }
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_gc_inlines_task)
{
  LNK_GCTypes *g = raw_task;
  U64 obj_idx = task_id;
  Temp scratch = scratch_begin(0, 0);
  String8List inlinee_lines = cv_sub_section_from_debug_s(g->cv->debug_s_arr[obj_idx], CV_C13SubSectionKind_InlineeLines);
  for EachNode(dn, String8Node, inlinee_lines.first) {
    Temp temp = temp_begin(scratch.arena);
    CV_TiOffsets l = cv_inlinee_ti_offsets(temp.arena, dn->string);
    lnk_gc_visit_offsets(g, dn->string, l);
    temp_end(temp);
  }
  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_gc_rewrite_leaves_task)
{
  LNK_GCTypes *g = raw_task;
  Temp scratch = scratch_begin(0, 0);
  for EachInRange(i, g->cur_ranges[task_id]) {
    Temp temp = temp_begin(scratch.arena);
    CV_Leaf leaf = cv_leaf_from_ptr(g->cur_leaf_v[i]);
    CV_TiOffsets l = cv_leaf_ti_offsets(temp.arena, leaf.kind, leaf.data);
    lnk_gc_visit_offsets(g, leaf.data, l);
    temp_end(temp);
  }
  scratch_end(scratch);
}

typedef struct LNK_GCRingTask
{
  U8            **leaf_v;
  Rng1U64        *ranges;
  U64            *counts;
  U64            *offsets;
  LNK_GCNamePair *pairs;
} LNK_GCRingTask;

// parallel: count UDT leaves with a unique_name per range (pass 0) / emit (hash,idx) pairs (pass 1)
internal
THREAD_POOL_TASK_FUNC(lnk_gc_ring_count_task)
{
  LNK_GCRingTask *t = raw_task;
  U64 n = 0;
  for EachInRange(i, t->ranges[task_id]) {
    CV_Leaf leaf = cv_leaf_from_ptr(t->leaf_v[i]);
    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo ui = cv_get_udt_info(leaf.kind, leaf.data);
      if (ui.props & CV_TypeProp_HasUniqueName) { n += 1; }
    }
  }
  t->counts[task_id] = n;
}

internal
THREAD_POOL_TASK_FUNC(lnk_gc_ring_fill_task)
{
  LNK_GCRingTask *t = raw_task;
  U64 cur = t->offsets[task_id];
  for EachInRange(i, t->ranges[task_id]) {
    CV_Leaf leaf = cv_leaf_from_ptr(t->leaf_v[i]);
    if (cv_is_udt(leaf.kind)) {
      CV_UDTInfo ui = cv_get_udt_info(leaf.kind, leaf.data);
      if (ui.props & CV_TypeProp_HasUniqueName) {
        U64 h = 14695981039346656037ull;
        for EachIndex(c, ui.unique_name.size) { h = (h ^ ui.unique_name.str[c]) * 0x100000001b3ull; }
        t->pairs[cur].hash = h; t->pairs[cur].idx = (U32)i; cur += 1;
      }
    }
  }
}

// mark a leaf reachable and, if this is its first mark, append it to its source's frontier so a
// later round expands it. Atomic mark gates the append, so each leaf lands on the frontier once.
internal void
lnk_gc_mark_enqueue(LNK_GCTypes *g, CV_TypeIndexSource ns, U64 ci)
{
  if (ci >= g->orig_n[ns]) { return; }
  if (g->mark[ns][ci])     { return; } // fast non-atomic skip: already reachable (the common edge)
  // only the worker that wins the 0->1 transition appends, so the atomic runs once per leaf (not
  // once per reference edge).
  if (!ins_atomic_u8_eval_assign(&g->mark[ns][ci], 1)) {
    U32 pos = ins_atomic_u32_inc_eval(g->fcount[ns]) - 1;
    g->frontier[ns][pos] = (U32)ci;
  }
}

// one bulk-synchronous round of transitive closure: expand the frontier slice [round_begin,
// round_end) of cur_source -- visit each leaf and enqueue the leaves it references (and its
// unique_name UDT counterparts). Frontier-driven, so total work is O(reachable leaves), not
// O(rounds * total leaves).
internal
THREAD_POOL_TASK_FUNC(lnk_gc_expand_task)
{
  LNK_GCTypes       *g = raw_task;
  CV_TypeIndexSource s = g->cur_source;
  Temp scratch = scratch_begin(0, 0);
  for EachInRange(local, g->cur_ranges[task_id]) {
    U32 i = g->frontier[s][g->round_begin + local];

    Temp temp = temp_begin(scratch.arena);
    CV_Leaf leaf = cv_leaf_from_ptr(g->leaf_v[s][i]);
    CV_TiOffsets l = cv_leaf_ti_offsets(temp.arena, leaf.kind, leaf.data);
    for (U64 ti_idx = 0, ti_count = cv_ti_offsets_count(&l); ti_idx < ti_count; ti_idx += 1) {
      CV_TiOff     n  = cv_ti_offset_at(&l, ti_idx);
      CV_TypeIndex ti = memory_read32(leaf.data.str + n.offset);
      U64 lo = g->min[n.source];
      if (ti >= lo) { lnk_gc_mark_enqueue(g, n.source, ti - lo); }
    }
    temp_end(temp);

    if (s == CV_TypeIndexSource_TPI) {
      for (U32 j = g->udt_next[i]; j != i; j = g->udt_next[j]) {
        lnk_gc_mark_enqueue(g, CV_TypeIndexSource_TPI, j);
      }
    }
  }
  scratch_end(scratch);
}

internal void
lnk_gc_types(TP_Context *tp, Arena *arena, LNK_CodeViewInput *cv, LNK_MergedTypes *types)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  LNK_GCTypes g = {0};
  g.cv = cv;
  U64 total_leaves = 0;
  for EachIndex(s, CV_TypeIndexSource_COUNT) {
    g.min[s]    = types->min_type_indices[s];
    g.orig_n[s] = types->count[s];
    g.leaf_v[s] = types->v[s];
    g.mark[s]   = push_array(scratch.arena, U8, g.orig_n[s] ? g.orig_n[s] : 1); // zeroed
    total_leaves += g.orig_n[s];
  }

  // mark roots: every type index referenced by a surviving symbol / inlinee record
  g.do_rewrite = 0;
  g.sym_ranges = tp_divide_work(scratch.arena, cv->symbol_input_count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_syms_task, &g);
  tp_for_parallel(tp, 0, cv->obj_count,    lnk_gc_inlines_task, &g);

  // link UDT leaves that share a unique_name into rings, so that marking any one (e.g. a
  // forward ref reached as a member-pointer target) also keeps its full definition -- needed
  // for the debugger to complete types referenced only by name. TPI only (IPI has no UDTs).
  U64  n_tpi   = g.orig_n[CV_TypeIndexSource_TPI];
  g.udt_next   = push_array_no_zero(scratch.arena, U32, n_tpi ? n_tpi : 1);
  for EachIndex(i, n_tpi) { g.udt_next[i] = (U32)i; }
  {
    LNK_GCRingTask rt = {0};
    rt.leaf_v  = g.leaf_v[CV_TypeIndexSource_TPI];
    rt.ranges  = tp_divide_work(scratch.arena, n_tpi, tp->worker_count);
    rt.counts  = push_array(scratch.arena, U64, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_ring_count_task, &rt);
    rt.offsets = offsets_from_counts_array_u64(scratch.arena, rt.counts, tp->worker_count);
    U64 np     = sum_array_u64(tp->worker_count, rt.counts);
    rt.pairs   = push_array_no_zero(scratch.arena, LNK_GCNamePair, np ? np : 1);
    tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_ring_fill_task, &rt);

    radsort(rt.pairs, np, lnk_gc_name_pair_is_before);
    for (U64 a = 0; a < np; ) {
      U64 b = a + 1;
      while (b < np && rt.pairs[b].hash == rt.pairs[a].hash) { b += 1; }
      for (U64 k = a; k < b; k += 1) { g.udt_next[rt.pairs[k].idx] = rt.pairs[(k + 1 < b) ? (k + 1) : a].idx; }
      a = b;
    }
  }

  // transitive closure (parallel, bulk-synchronous): repeat rounds that visit each
  // marked-but-unexpanded leaf and mark what it references, until a round marks nothing new
  // seed the frontier with the root-marked leaves (one O(total leaves) scan), then expand
  // frontier slices until both sources drain. Each leaf is expanded exactly once.
  U32 fcount[CV_TypeIndexSource_COUNT] = {0};
  U64 start [CV_TypeIndexSource_COUNT] = {0};
  for EachIndex(s, CV_TypeIndexSource_COUNT) {
    g.frontier[s] = push_array_no_zero(scratch.arena, U32, g.orig_n[s] ? g.orig_n[s] : 1);
    g.fcount[s]   = &fcount[s];
    for EachIndex(i, g.orig_n[s]) { if (g.mark[s][i]) { g.frontier[s][fcount[s]++] = (U32)i; } }
  }
  for (;;) {
    B32 any = 0;
    for EachIndex(s, CV_TypeIndexSource_COUNT) {
      U64 begin = start[s], end = fcount[s]; // fcount may grow during the round (cross-source enqueues)
      if (begin < end) {
        any = 1;
        g.cur_source  = (CV_TypeIndexSource)s;
        g.round_begin = begin;
        g.round_end   = end;
        g.cur_ranges  = tp_divide_work(scratch.arena, end - begin, tp->worker_count);
        tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_expand_task, &g);
        start[s] = end;
      }
    }
    if (!any) { break; }
  }

  // compact each source: assign new contiguous type indices to the kept leaves. The leaf
  // pointer array is compacted IN PLACE (kept count only shrinks, so the write cursor never
  // passes the read cursor) and remap lives in scratch -- so the GC adds nothing to the arena
  // that survives into the (peak) PDB build.
  U64 kept_total = 0;
  for EachIndex(s, CV_TypeIndexSource_COUNT) {
    g.remap[s]      = push_array_no_zero(scratch.arena, CV_TypeIndex, g.orig_n[s] ? g.orig_n[s] : 1);
    U8 **v          = g.leaf_v[s];
    U64  new_n      = 0;
    for EachIndex(idx, g.orig_n[s]) {
      if (g.mark[s][idx]) { g.remap[s][idx] = (CV_TypeIndex)(g.min[s] + new_n); v[new_n++] = v[idx]; }
      else                { g.remap[s][idx] = 0; /* T_NOTYPE; never referenced by a kept record */ }
    }
    types->count[s] = new_n;
    kept_total += new_n;
  }

  // rewrite all type-index references to the compacted indices
  g.do_rewrite = 1;
  tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_syms_task, &g);
  tp_for_parallel(tp, 0, cv->obj_count,    lnk_gc_inlines_task, &g);
  for EachIndex(s, CV_TypeIndexSource_COUNT) {
    g.cur_source = (CV_TypeIndexSource)s;
    g.cur_leaf_v = types->v[s];
    g.cur_ranges = tp_divide_work(scratch.arena, types->count[s], tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, lnk_gc_rewrite_leaves_task, &g);
  }

  if (lnk_get_log_status(LNK_Log_Debug)) {
    lnk_log(LNK_Log_Debug, "type GC: kept %llu of %llu leaves (pruned %llu)", kept_total, total_leaves, total_leaves - kept_total);
  }

  scratch_end(scratch);
  ProfEnd();
}

typedef struct
{
  U64 weight;
  U32 obj_idx;
} LNK_ObjDistWeight;

force_inline int
lnk_obj_dist_weight_is_before(void *raw_a, void *raw_b)
{
  LNK_ObjDistWeight *a = raw_a, *b = raw_b;
  if (a->weight != b->weight) { return a->weight > b->weight; }
  return a->obj_idx < b->obj_idx; // deterministic total order
}

// FAIR-SHARE: distribute cv->obj_count objs across `worker_count` lane buckets.
// Rebuilt per barrier pass so the distribution matches the cohort C that pass
// runs at (lnk_move_global_symbols_to_gsi / lnk_write_pdb_modules read
// task->obj_indices[task_id] for lanes [0,C)). Output is width- and
// assignment-independent -- per-obj results land in per-obj slots (module
// streams) or in GSI bucket chains that are content-sorted at serialization
// (gsi_symbol_is_before radsorts every chain) -- so any deterministic partition
// produces byte-identical PDB bytes; only the per-lane balance changes.
//
// `weights` (optional, [obj_count]) upgrades the round-robin to a greedy LPT
// (longest-processing-time) assignment: objs are taken in weight-descending
// order (obj_idx tie-break -> deterministic) and each goes to the least-loaded
// lane. Round-robin ignores per-obj symbol-stream size, so a lane that draws
// several giant objs holds the whole barrier pass at the final barrier while
// the other lanes idle.
internal void
lnk_build_pdb_distribute_obj_indices(Arena *arena, LNK_BuildPdb *task, U64 obj_count, U32 worker_count, U64 *weights)
{
  task->obj_indices = push_array(arena, U32Array, worker_count);
  if (weights == 0) {
    U64 objs_per_worker = CeilIntegerDiv(obj_count, worker_count);
    for EachIndex(i, worker_count)  { task->obj_indices[i].v = push_array(arena, U32, objs_per_worker ? objs_per_worker : 1); }
    for EachIndex(obj_idx, obj_count) {
      U32Array *obj_indices = &task->obj_indices[obj_idx % worker_count];
      obj_indices->v[obj_indices->count++] = (U32)obj_idx;
    }
  } else {
    Temp scratch = scratch_begin(&arena, 1);

    LNK_ObjDistWeight *order = push_array_no_zero(scratch.arena, LNK_ObjDistWeight, obj_count);
    for EachIndex(obj_idx, obj_count) { order[obj_idx] = (LNK_ObjDistWeight){ .weight = weights[obj_idx], .obj_idx = (U32)obj_idx }; }
    radsort(order, obj_count, lnk_obj_dist_weight_is_before);

    U64 *loads  = push_array(scratch.arena, U64, worker_count);
    U32 *assign = push_array_no_zero(scratch.arena, U32, obj_count);
    for EachIndex(i, obj_count) {
      U32 min_lane = 0;
      for (U32 lane = 1; lane < worker_count; lane += 1) { if (loads[lane] < loads[min_lane]) { min_lane = lane; } }
      assign[order[i].obj_idx] = min_lane;
      loads[min_lane] += order[i].weight + 1; // +1 spreads zero-weight objs too
      task->obj_indices[min_lane].count += 1;
    }

    for EachIndex(lane, worker_count) {
      task->obj_indices[lane].v     = push_array_no_zero(arena, U32, task->obj_indices[lane].count);
      task->obj_indices[lane].count = 0;
    }
    // fill in ascending obj order per lane (deterministic, cache-friendly iteration)
    for EachIndex(obj_idx, obj_count) {
      U32Array *obj_indices = &task->obj_indices[assign[obj_idx]];
      obj_indices->v[obj_indices->count++] = (U32)obj_idx;
    }

    scratch_end(scratch);
  }
}

internal LNK_FileArtifact
lnk_build_pdb(TP_Context *tp, TP_Arena *tp_arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_CodeViewInput *cv, LNK_MergedTypes cv_types, LNK_PdbWriter writer, LNK_PDB_BuilderFlags builder_flags, struct LNK_Inputer *inputer)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);

  if (builder_flags == LNK_PDB_BuilderFlag_All) {
    builder_flags = ~0;
  }

  // ini= bucket: pdb_alloc_ commits the MSF + type-server tables (fresh pages,
  // ~132K faults on the editor link) -- under a storm every fresh commit pays
  // the page-repurpose path, so this span needs its own attribution
  lnk_summary_phase_begin(LNK_SummaryPhase_PdbIni);

  LNK_BuildPdb task = {
    .image_data                         = image_data,
    .symtab                             = symtab,
    .cv                                 = cv,
    .pdb                                = pdb_alloc_(lnk_get_huge_arena(), config->pdb_page_size, config->machine, config->time_stamp, config->age, config->guid),
    .mod_arr                            = push_array(scratch.arena, PDB_DbiModule *, cv->obj_count),
    .pe                                 = pe_bin_info_from_data(scratch.arena, image_data),
    .image_section_table                = coff_section_table_from_data(scratch.arena, image_data, task.pe.section_table_range),
    .image_section_table_count          = task.pe.section_count+1,
    .image_section_virt_ranges.count    = task.image_section_table_count,
    .image_section_virt_ranges.v        = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
    .image_section_file_ranges.v        = push_array(scratch.arena, Rng1U64, task.image_section_table_count),
    .image_section_file_section_numbers = push_array(scratch.arena, U64, task.image_section_table_count),
  };

  LNK_PdbOutput  output     = {0};
  LNK_PdbOutput *output_ptr = 0;
  if (writer.output_path.size > 0) {
    output.writer            = writer.file_writer;
    output.decommit_flushed  = (config->rad_debug != LNK_SwitchState_Yes);
    output.file              = lnk_background_file_writer_begin_file(output.writer, writer.output_path, writer.temp_output_path);
    if (output.file != 0) {
      output.sealed_stream_cap = cv->obj_count + 128;
      output.sealed_streams    = push_array_no_zero(scratch.arena, MSF_StreamNumber, output.sealed_stream_cap);
      output_ptr               = &output;
    }
  }

  task.output = output_ptr;

  // patched debug-section copies (obj->section_data_copies) release at the END of the
  // module-write phase. P3.3: with g_debug_s_window set (default) $S never has copies --
  // only the (essentially nonexistent) reloc-bearing non-$S debug sections do -- and the
  // /PDBSTRIPPED pre-pass re-reads through lnk_obj_window_debug_s, so the gate below only
  // still matters for the /OPT:GCTYPES copy path, where the stripping loop re-walks
  // cv->debug_s_arr Symbols aliasing the copies after this build returns. The RDI converter
  // is safe (it reads the PDB artifact pages, not obj debug sections).
  task.free_sect_copies = (config->pdb_stripped_name.size == 0);

  PDB_BuildHooks build_hooks = {0};
  if (output_ptr != 0) {
    build_hooks.stream_finalize = lnk_pdb_output_finalize_stream;
    build_hooks.user_data       = output_ptr;
  }

  // set min type indices
  for EachElement(ti_source, cv_types.min_type_indices) { task.pdb->type_servers[ti_source]->ti_lo = cv_types.min_type_indices[ti_source]; }

  // per-worker obj indices are (re)distributed per barrier pass to the cohort
  // that pass actually runs at (FAIR-SHARE: tp->worker_count is pinned to the
  // cohort C inside each tp_barrier_begin/end bracket, and the
  // lnk_write_pdb_modules task reads task.obj_indices[task_id] for lanes [0,C);
  // P2b made lnk_move_global_symbols_to_gsi obj_indices-free -- it strides the
  // pre-extracted per-obj tables directly). Distributing to the full
  // worker_count up front would leave objs in buckets [C,worker_count)
  // unprocessed when C<worker_count. See lnk_build_pdb_distribute_obj_indices.

  lnk_summary_phase_end(LNK_SummaryPhase_PdbIni);

  // push types
  lnk_summary_phase_begin(LNK_SummaryPhase_PdbTpi);
  if (builder_flags & LNK_PDB_BuilderFlag_Ipi) {
    pdb_type_server_push_parallel(tp, task.pdb->type_servers[CV_TypeIndexSource_IPI], cv_types.count[CV_TypeIndexSource_IPI], cv_types.v[CV_TypeIndexSource_IPI]);
  }
  if (builder_flags & LNK_PDB_BuilderFlag_Tpi) {
    pdb_type_server_push_parallel(tp, task.pdb->type_servers[CV_TypeIndexSource_TPI], cv_types.count[CV_TypeIndexSource_TPI], cv_types.v[CV_TypeIndexSource_TPI]);
  }
  lnk_summary_phase_end(LNK_SummaryPhase_PdbTpi);

  lnk_summary_phase_begin(LNK_SummaryPhase_PdbStr);
  ProfBegin("Merge String Tables");
  task.string_ht = cv_dedup_string_tables(tp_arena, tp, cv->obj_count, cv->debug_s_arr);
  cv_string_hash_table_assign_buffer_offsets(tp, task.string_ht);

  // the deduped buckets alias the objs' $S string tables in place -- patched debug-section
  // COPIES for reloc-carrying $S, RAW-MAPPED view bytes for reloc-free $S -- and both /names
  // (pdb_strtab_build memcpys bucket bytes at serialize time in pdb_build_dbi_info) and DBI
  // file-info hash these bytes AFTER the backing dies (copies release at the end of Write
  // Modules; views die in P5) -- rehome the winning buckets into their own blob first
  // (total = the deduped /names payload, tiny next to the backing being released). P3.1:
  // UNCONDITIONAL, and the bucket walk below is backing-agnostic (every non-null winning
  // bucket is copied, whatever its bytes alias), so raw-mapped tables are covered too.
  // Offsets are already assigned; bytes are identical, so the /names stream and every
  // recorded offset are unchanged -- /PDBSTRIPPED is unaffected (its second build makes its
  // own string_ht from its own debug_s_arr; strtab serialize reads content-identical bytes).
  if (task.string_ht.total_string_size > 0) {
    ProfBegin("Materialize String Table Bytes");
    U8 *blob   = push_array_no_zero(tp_arena->v[0], U8, task.string_ht.total_string_size);
    U64 cursor = 0;
    for EachIndex(bucket_idx, task.string_ht.bucket_cap) {
      CV_StringBucket *bucket = task.string_ht.buckets[bucket_idx];
      if (bucket == 0) { continue; }
      Assert(cursor + bucket->string.size <= task.string_ht.total_string_size);
      MemoryCopy(blob + cursor, bucket->string.str, bucket->string.size);
      bucket->string.str = blob + cursor;
      cursor += bucket->string.size;
    }
    ProfEnd();
  }
  ProfEnd();
  lnk_summary_phase_end(LNK_SummaryPhase_PdbStr);

  task.string_table_base_offset = task.pdb->info->strtab.size;
  ProfBegin("Add string tables");
  pdb_strtab_add_cv_string_hash_table(&task.pdb->info->strtab, task.string_ht);
  ProfEnd();
  pdb_build_types(tp, task.pdb, &build_hooks);
  lnk_compressed_obj_log_phase_stats("before write modules");
  lnk_pdb_output_log_mark(output_ptr, "after pdb_build_types (TPI/IPI sealed)");

  // merged leaf bytes are now in MSF pages (and on their way to disk); no consumer of
  // cv_types.v remains on this path (RDI converts from the PDB artifact, the RRT export
  // is a separate boot mode), so hand the multi-GB materialize buffers back to the OS
  for EachElement(ti_source, cv_types.leaf_buffers) {
    if (cv_types.leaf_buffers[ti_source].size > 0) {
      release_memory(cv_types.leaf_buffers[ti_source].str, cv_types.leaf_buffers[ti_source].size);
      cv_types.leaf_buffers[ti_source] = str8_zero();
      cv_types.v[ti_source]     = 0; // poison dangling leaf pointers
      cv_types.count[ti_source] = 0;
    }
  }

  if (builder_flags & LNK_PDB_BuilderFlag_Modules) {
    ProfScope ("Alloc Modules")
      for EachIndex(obj_idx, cv->obj_count) {
        task.mod_arr[obj_idx] = dbi_push_module(task.pdb->dbi, cv->obj_arr[obj_idx]->path, lnk_obj_get_lib_path(cv->obj_arr[obj_idx]));
      }

ProfScope("Write Modules")
    {
      lnk_summary_phase_begin(LNK_SummaryPhase_PdbMod);
      U64 phase_begin_us = now_time_us();
      // FAIR-SHARE: pin the cohort, distribute objs over exactly the cohort lanes,
      // then run the barrier pass at that cohort. tp_barrier_begin sets
      // tp->worker_count := C for the bracket.
      U32 C = tp_barrier_begin(tp);
      // weight = total debug$S byte size: the module-stream write walks every
      // subsection of the obj (symbols + lines + checksums + ...)
      U64 *weights = push_array_no_zero(scratch.arena, U64, cv->obj_count);
      for EachIndex(obj_idx, cv->obj_count) {
        weights[obj_idx] = cv_total_sub_section_size_from_debug_s(&cv->debug_s_arr[obj_idx]);
      }
      lnk_build_pdb_distribute_obj_indices(scratch.arena, &task, cv->obj_count, C, weights);

      // Per-obj counts/proc-refs plus per-lane surviving payload arenas. Exact global-symbol
      // winners are copied into these arenas directly from the one transformed $S window;
      // Move Global Symbols later consumes the compacted winner pointers and proc-ref tables.
      task.preext             = push_array(scratch.arena, LNK_GsiPreExtractObj, cv->obj_count);
      task.preext_arena_count = C;
      task.procref_payload_arenas = push_array(scratch.arena, Arena *, C);
      for EachIndex(i, C) {
        task.procref_payload_arenas[i] = arena_alloc(.commit_size = MB(2), .name = "GSI_PROC_REFS");
      }
#if BUILD_DEBUG
      // the fused extraction walks objs [0, obj_count) only; the old globals collect walked
      // every symbol input, which spans [0, cv->count) INCLUDING injected type-server/.ifc
      // blob pseudo-objs -- prove those never carry a Symbols subsection so nothing is missed
      for (U64 pseudo_idx = cv->obj_count; pseudo_idx < cv->count; pseudo_idx += 1) {
        Assert(cv_sub_section_from_debug_s(cv->debug_s_arr[pseudo_idx], CV_C13SubSectionKind_Symbols).total_size == 0);
      }
#endif
      tp_for_parallel_reserve(tp, 0, C, lnk_write_pdb_modules, &task); // BARRIER pass (path B): barrier_wait/tp_broadcast
      tp_barrier_end(tp);
      lnk_compressed_obj_log_phase_stats("after write modules");
      lnk_log(LNK_Log_Timers, "[pdb] write modules in %.2f ms (cohort %u)", (F64)(now_time_us() - phase_begin_us) / 1000.0, C);
      lnk_pdb_output_log_mark(output_ptr, "after write modules");
      if (g_debug_s_window && lnk_get_log_status(LNK_Log_Debug)) {
        // bounds the per-worker window arena growth: worst case commit = cohort x this value
        lnk_log(LNK_Log_Debug, "[pdb] $S window high-water: %llu bytes (largest single obj window)", g_debug_s_window_hwm);
      }
      lnk_summary_phase_end(LNK_SummaryPhase_PdbMod);
    }

    // the module-write phase ran the last $S fixup replay on this path -- hand the GB-class
    // journal arenas to the background reaper now, before the GSI/PSI commit peak. No-op when
    // the eager path already consumed it (or for the stripped/SkipSymbolTypeFixup cv, which
    // never built one). P3.3: when a /PDBSTRIPPED build follows, the journal must SURVIVE --
    // the stripping pre-pass re-reads Symbols nodes through lnk_obj_window_debug_s, whose fill
    // replays the journal (nothing persists into the raw views anymore); the pre-pass releases
    // it when done.
    if (config->pdb_stripped_name.size == 0) {
      lnk_release_debug_s_fixup_journal(cv);
    }

    // module streams were enqueued per-obj inside lnk_write_pdb_modules
ProfScope("Move Global Symbols")
    {
      lnk_summary_phase_begin(LNK_SummaryPhase_PdbGsi);
      U64 phase_begin_us = now_time_us();
      U32 C = tp_barrier_begin(tp);
      // P2b: no obj_indices distribution -- the pass reads no $S bytes; it flattens the
      // pre-extracted per-obj segments with obj-index striding and even-split inserts
      tp_for_parallel_reserve(tp, 0, C, lnk_move_global_symbols_to_gsi, &task); // BARRIER pass (path B): tp_sum_u64/tp_broadcast/barrier_wait
      tp_barrier_end(tp);
      lnk_log(LNK_Log_Timers, "[pdb] move global symbols in %.2f ms (cohort %u)", (F64)(now_time_us() - phase_begin_us) / 1000.0, C);
      lnk_summary_phase_end(LNK_SummaryPhase_PdbGsi);
    }

    lnk_summary_phase_begin(LNK_SummaryPhase_PdbSym);
    ProfScope("Build GSI and PSI") pdb_build_gsi_psi(tp, task.pdb);
    lnk_summary_phase_end(LNK_SummaryPhase_PdbSym);
    if (output_ptr != 0) {
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->publics_sn);
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->globals_sn);
      lnk_pdb_output_enqueue_stream(output_ptr, task.pdb->msf, task.pdb->dbi->symbols_sn);
      lnk_pdb_output_log_mark(output_ptr, "after GSI/PSI streams sealed");
    }
  }
  
  if (builder_flags & LNK_PDB_BuilderFlag_SC) {
    lnk_summary_phase_begin(LNK_SummaryPhase_PdbSc);
    ProfBegin("Build Section Contrib Map");
    {
      ProfBegin("Build DBI Section Headers");
      for (U64 sect_idx = 1; sect_idx < task.image_section_table_count; sect_idx += 1) {
        dbi_push_section(task.pdb->dbi, task.image_section_table[sect_idx]);
      }
      ProfEnd();

      for EachIndex(i, task.image_section_table_count) {
        COFF_SectionHeader *sect_header = task.image_section_table[i];

        if (~sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
          U64 section_file_idx = task.image_section_file_ranges.count++;
          task.image_section_file_ranges.v[section_file_idx]        = r1u64s(sect_header->foff, sect_header->fsize);
          task.image_section_file_section_numbers[section_file_idx] = i;
        }

        task.image_section_virt_ranges.v[i] = rng_1u64(sect_header->voff, sect_header->voff + sect_header->vsize);
      }

      task.sc_arrays = push_array(scratch.arena, PDB_DbiSCArray, cv->obj_count);
      tp_for_parallel(tp, tp_arena, cv->obj_count, lnk_push_dbi_sec_contrib_task, &task);

      PDB_DbiSCArray *sec_contribs = &task.pdb->dbi->sec_contribs;
      U64             new_count    = sec_contribs->count;
      for EachIndex(obj_idx, cv->obj_count) {
        new_count += task.sc_arrays[obj_idx].count;
      }
      PDB_DbiSC *new_v = push_array_no_zero(task.pdb->dbi->arena, PDB_DbiSC, new_count);
      MemoryCopyTyped(new_v, sec_contribs->v, sec_contribs->count);
      U64 cursor = sec_contribs->count;
      for EachIndex(obj_idx, cv->obj_count) {
        PDB_DbiSCArray *src = &task.sc_arrays[obj_idx];
        MemoryCopyTyped(new_v + cursor, src->v, src->count);
        cursor += src->count;
      }
      sec_contribs->v     = new_v;
      sec_contribs->count = new_count;
      sec_contribs->cap   = new_count;
    }
    ProfEnd();
    lnk_summary_phase_end(LNK_SummaryPhase_PdbSc);
  }

  // Streaming-ring P5: the SC pass above was the LAST reader of the memory-mapped input
  // views on this path (audited: pass B of the module-write epilogue = last .debug$S read,
  // publics in "Move Global Symbols" = last COFF symbol-table + string-table read [long
  // public names alias the view's COFF string table, symbol records fall back to the view
  // when no symbol_table_copy exists], SC = last section-header/name read; NatVis below
  // reads its own files, /names serializes the P3.1 rehomed blob, DBI file-info hashes the
  // repointed bucket copies, MSF build/serialize reads PDB pages only). Release the views
  // NOW with the exit path's own capped parallel sweep, relocated: the pool is idle between
  // the SC pass and the serial NatVis/DBI/MSF tail, so the sweep lands at parallel-unmap
  // wall (~1s FN-scale; a single background thread here serialized ~60s of unmap CPU and
  // showed up as +6.5s FN wall) and mapped input residency is gone before the tail + PDB
  // write drain instead of held to the end of the link. The sweep zeroes data/owns_file_map
  // per input, so the exit-time calls (lnk_inputer_release_file_maps,
  // lnk_release_input_views) turn into no-ops -- idempotent. Gated OFF when a /PDBSTRIPPED
  // build follows (its pre-build strip loop re-walks Symbols through lnk_obj_window_debug_s
  // = raw view reads after this function returns; the caller also passes inputer==0 for the
  // stripped build itself) and under /OPT:GCTYPES (copy mode: reloc-free $S slices in
  // debug_s_arr alias the raw views in place -- keep the exit-time release). Only for the
  // CoW read-only mapping mode, mirroring lnk_release_input_views (read-write-shared unmap
  // flushes dirty pages back to the input files).
  if (inputer != 0 &&
      config->pdb_stripped_name.size == 0 &&
      config->opt_gc_types != LNK_SwitchState_Yes &&
      (config->io_flags & LNK_IO_Flags_MemoryMapFilesReadOnly) &&
      !(config->io_flags & LNK_IO_Flags_MemoryMapFilesReadWrite)) {
    ProfBegin("Release Input Views Early");
    U64 unmap_begin_us = now_time_us();
    // All thread-pool consumers above have joined. Keep compressed-object teardown at this
    // explicit quiescence point; per-object or background close can race lazy materialization.
    lnk_inputer_release_file_maps(tp, config->debug_worker_cap, inputer);
    lnk_log(LNK_Log_Timers, "[pdb] early input-view release in %.2f ms",
            (F64)(now_time_us() - unmap_begin_us) / 1000.0);
    ProfEnd();
  }

  if (builder_flags & LNK_PDB_BuilderFlag_NATVIS) {
    ProfBegin("Build NatVis");
    {
      String8Array  natvis_file_path_arr = str8_array_from_list(scratch.arena, &config->natvis_list);
      B8           *natvis_was_read      = push_array(scratch.arena, B8, natvis_file_path_arr.count);
      String8Array  natvis_file_data_arr = lnk_read_data_from_file_path_parallel(tp, scratch.arena, config->io_flags, natvis_file_path_arr, natvis_was_read);

      for EachIndex(i, natvis_file_data_arr.count) {
        String8 natvis_file_path = natvis_file_path_arr.v[i];
        String8 natvis_file_data = natvis_file_data_arr.v[i];

        // did we read the file?
        if (natvis_was_read[i] == 0 || natvis_file_data.size == 0) {
          lnk_error(LNK_Warning_FileNotFound, "unable to open natvis file \"%S\"", natvis_file_path);
          continue;
        }

        // sanity check file extension or VS wont load NatVis
        String8 ext = str8_skip_last_dot(natvis_file_path);
        if (!str8_match(ext, str8_lit("natvis"), StringMatchFlag_CaseInsensitive)) {
          lnk_error(LNK_Warning_Natvis, "Visual Studio expects .natvis extension: \"%S\"", natvis_file_path);
        }

        // add natvis to PDB
        PDB_SrcError error = pdb_add_src(task.pdb->info, task.pdb->msf, natvis_file_path, natvis_file_data, PDB_SrcComp_NULL);
        if (error != PDB_SrcError_OK) {
          lnk_error(LNK_Error_Natvis, "%S", pdb_string_from_src_error(error));
        }
      }
    }
    ProfEnd();
  }

  lnk_summary_phase_begin(LNK_SummaryPhase_PdbMsf);
  lnk_pdb_output_log_mark(output_ptr, "before pdb_build_dbi_info");
  pdb_build_dbi_info(tp, task.pdb, task.string_ht, 0, cv->is_stripped, &build_hooks);
  lnk_pdb_output_log_mark(output_ptr, "after pdb_build_dbi_info");

  MSF_Error msf_err = msf_build(task.pdb->msf);
  if (msf_err != MSF_Error_OK) {
    lnk_error(LNK_Error_UnableToSerializeMsf, "unable to serialize MSF: %s", msf_error_to_string(msf_err));
  }

  if (output_ptr != 0) {
    lnk_pdb_output_log_mark(output_ptr, "after msf_build");
    lnk_pdb_output_enqueue_remaining(output_ptr, task.pdb->msf);
    lnk_pdb_output_log_mark(output_ptr, "after enqueue_remaining");
  }

  ProfBegin("Get Page Nodes");
  LNK_FileArtifact artifact = { .data = msf_get_page_data_nodes(tp_arena->v[0], task.pdb->msf) };
  ProfEnd();

  if (output_ptr != 0) {
    lnk_background_file_writer_end_file(output_ptr->writer, output_ptr->file, artifact.data.total_size);
  }
  lnk_summary_phase_end(LNK_SummaryPhase_PdbMsf);


  // NOTE: linker is about to exit so we can skip memory release
  // and let windows free memory since it does this faster
#if 0
  ProfBegin("Context Release");
  pdb_release(&pdb);
  ProfEnd();
#endif

  scratch_end(scratch);
  ProfEnd();
  return artifact;
}
