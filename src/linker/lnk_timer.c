// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

global LNK_Timer g_timers[LNK_Timer_Count];

internal void
lnk_timer_begin(LNK_TimerType timer)
{
  g_timers[timer].begin = os_now_microseconds();
}

internal void
lnk_timer_end(LNK_TimerType timer)
{
  g_timers[timer].end = os_now_microseconds();
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

