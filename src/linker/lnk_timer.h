// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum LNK_TimerType
{
  LNK_Timer_Image,
  LNK_Timer_Pdb,
  LNK_Timer_Rdi,
  LNK_Timer_Lib,
  LNK_Timer_Debug,
  LNK_Timer_Count
} LNK_TimerType;

typedef struct LNK_Timer
{
  U64 begin;
  U64 end;
} LNK_Timer;

internal void lnk_timer_begin(LNK_TimerType timer);
internal void lnk_timer_end(LNK_TimerType timer);

// Per-phase counter snapshot for the end-of-link summary line (v2). Each
// boundary stamp is wall (QPC) + process-wide user/kernel CPU (GetProcessTimes)
// + process-wide soft+hard fault count (GetProcessMemoryInfo.PageFaultCount):
// 2 syscalls per boundary, negligible. Deltas are PROCESS-WIDE, so a phase that
// overlaps a concurrent thread's work (e.g. the image-write thread overlapping
// the debug-info phases) counts that work too -- attribution, not accounting.
typedef struct LNK_SummaryCounters
{
  U64 wall_us;
  U64 user_us; // process user CPU, all threads
  U64 kern_us; // process kernel CPU, all threads
  U64 faults;  // process page faults (soft+hard)
} LNK_SummaryCounters;

internal LNK_SummaryCounters lnk_summary_counters_now(void);
internal LNK_SummaryCounters lnk_summary_counters_sub_sat(LNK_SummaryCounters a, LNK_SummaryCounters b); // per-field saturating a-b
internal LNK_SummaryCounters lnk_summary_counters_add(LNK_SummaryCounters a, LNK_SummaryCounters b);     // per-field a+b

// Phase accumulators for the end-of-link summary line. Unlike LNK_Timer
// (single begin/end shot), these ACCUMULATE across repeated brackets (e.g.
// lnk_load_inputs runs once per input round; the PDB sub-phases run again for
// the /PDBSTRIPPED build). Always measured. Image/Debug/PDB/RDI buckets come
// from g_timers (which stamp the same counters); these cover the phases that
// had no timer, plus the dbg/pdb sub-buckets.
typedef enum LNK_SummaryPhase
{
  LNK_SummaryPhase_Input,    // lnk_load_inputs (parse/load objs+libs), all rounds
  LNK_SummaryPhase_Resolve,  // lnk_link_inputs minus contained Input time (lib search + member resolution)
  LNK_SummaryPhase_Icf,      // lnk_opt_icf
  LNK_SummaryPhase_Ref,      // lnk_opt_ref
  LNK_SummaryPhase_Write,    // image write thread (overlaps debug info)

  // dbg umbrella sub-buckets (printed as dbgg[...])
  LNK_SummaryPhase_DbgMcvi,  // lnk_make_code_view_input
  LNK_SummaryPhase_DbgMerge, // lnk_merge_types

  // pdb sub-buckets (printed as pdbg[...]); brackets sit on the pre-existing
  // Prof/timer boundaries inside lnk_build_pdb + the write at its call site
  LNK_SummaryPhase_PdbHsh,   // lnk_replace_type_names_with_hashes (/RAD_PDB_HASH_TYPE_NAMES): parallel rewrite touching every merged TPI leaf -- storm re-fault amplifier
  LNK_SummaryPhase_PdbIni,   // lnk_build_pdb task init: pdb_alloc_ (MSF + type-server tables, ~132K fresh commits on the editor link)
  LNK_SummaryPhase_PdbGsi,   // lnk_move_global_symbols_to_gsi barrier pass ("Move Global Symbols")
  LNK_SummaryPhase_PdbSym,   // pdb_build_gsi_psi ("Build GSI and PSI": symrec + GSI/PSI hash streams)
  LNK_SummaryPhase_PdbMod,   // lnk_write_pdb_modules barrier pass ("Write Modules")
  LNK_SummaryPhase_PdbTpi,   // pdb_type_server_push_parallel TPI+IPI + pdb_type_server_build TPI/IPI
  LNK_SummaryPhase_PdbStr,   // string tables: cv_dedup_string_tables + offset assign + strtab add ("Merge String Tables"/"Add string tables")
  LNK_SummaryPhase_PdbSc,    // "Build Section Contrib Map" (per-obj section contribs + DBI section headers)
  LNK_SummaryPhase_PdbMsf,   // dbi_build + pdb_info_build + msf_build + page-node gather
  LNK_SummaryPhase_PdbWr,    // PDB file write (lnk_write_data_list_to_file_path in lnk_io)

  LNK_SummaryPhase_Count
} LNK_SummaryPhase;

internal void lnk_summary_phase_begin(LNK_SummaryPhase phase);
internal void lnk_summary_phase_end(LNK_SummaryPhase phase);

