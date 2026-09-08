// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global LNK_Timer g_timers[LNK_Timer_Count];

// summary (v2): every timer/phase boundary also stamps process-wide CPU +
// fault counters, so each bucket reports wall/user/kernel/faults
global LNK_SummaryCounters g_timer_counters_begin[LNK_Timer_Count];
global LNK_SummaryCounters g_timer_counters_end  [LNK_Timer_Count];

internal LNK_SummaryCounters
lnk_summary_counters_now(void)
{
  LNK_SummaryCounters c = { .wall_us = now_time_us() };
#if OS_WINDOWS
  FILETIME create_ft, exit_ft, kernel_ft, user_ft;
  if (GetProcessTimes(GetCurrentProcess(), &create_ft, &exit_ft, &kernel_ft, &user_ft)) {
    c.user_us = (((U64)user_ft.dwHighDateTime   << 32) | user_ft.dwLowDateTime)   / 10;
    c.kern_us = (((U64)kernel_ft.dwHighDateTime << 32) | kernel_ft.dwLowDateTime) / 10;
  }
  PROCESS_MEMORY_COUNTERS pmc = { (DWORD)sizeof(pmc) };
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    c.faults = pmc.PageFaultCount;
  }
#endif
  return c;
}

internal LNK_SummaryCounters
lnk_summary_counters_sub_sat(LNK_SummaryCounters a, LNK_SummaryCounters b)
{
  LNK_SummaryCounters c;
  c.wall_us = a.wall_us > b.wall_us ? a.wall_us - b.wall_us : 0;
  c.user_us = a.user_us > b.user_us ? a.user_us - b.user_us : 0;
  c.kern_us = a.kern_us > b.kern_us ? a.kern_us - b.kern_us : 0;
  c.faults  = a.faults  > b.faults  ? a.faults  - b.faults  : 0;
  return c;
}

internal LNK_SummaryCounters
lnk_summary_counters_add(LNK_SummaryCounters a, LNK_SummaryCounters b)
{
  LNK_SummaryCounters c;
  c.wall_us = a.wall_us + b.wall_us;
  c.user_us = a.user_us + b.user_us;
  c.kern_us = a.kern_us + b.kern_us;
  c.faults  = a.faults  + b.faults;
  return c;
}

internal void
lnk_timer_begin(LNK_TimerType timer)
{
  g_timer_counters_begin[timer] = lnk_summary_counters_now();
  g_timers[timer].begin         = g_timer_counters_begin[timer].wall_us;
}

internal void
lnk_timer_end(LNK_TimerType timer)
{
  g_timer_counters_end[timer] = lnk_summary_counters_now();
  g_timers[timer].end         = g_timer_counters_end[timer].wall_us;
}

global LNK_SummaryCounters g_summary_phase      [LNK_SummaryPhase_Count];
global LNK_SummaryCounters g_summary_phase_start[LNK_SummaryPhase_Count];

internal void
lnk_summary_phase_begin(LNK_SummaryPhase phase)
{
  g_summary_phase_start[phase] = lnk_summary_counters_now();
}

internal void
lnk_summary_phase_end(LNK_SummaryPhase phase)
{
  // atomic adds: the Write bracket runs on the background image-write thread
  LNK_SummaryCounters now = lnk_summary_counters_now();
  ins_atomic_u64_add_eval(&g_summary_phase[phase].wall_us, now.wall_us - g_summary_phase_start[phase].wall_us);
  ins_atomic_u64_add_eval(&g_summary_phase[phase].user_us, now.user_us - g_summary_phase_start[phase].user_us);
  ins_atomic_u64_add_eval(&g_summary_phase[phase].kern_us, now.kern_us - g_summary_phase_start[phase].kern_us);
  ins_atomic_u64_add_eval(&g_summary_phase[phase].faults,  now.faults  - g_summary_phase_start[phase].faults);
}

internal String8
lnk_string_from_timer_type(LNK_TimerType type)
{
  switch (type) {
  case LNK_Timer_Image: return str8_lit("Image");
  case LNK_Timer_Pdb:   return str8_lit("PDB");
  case LNK_Timer_Rdi:   return str8_lit("RDI");
  case LNK_Timer_Lib:   return str8_lit("Lib");
  case LNK_Timer_Debug: return str8_lit("Debug");
  default: InvalidPath;
  }
  return str8_zero();
}

