// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_Timer *
lnk_timers(void)
{
  local_persist LNK_Timer timers[LNK_Timer_Count];
  return timers;
}

internal void
lnk_timer_begin(LNK_TimerType t)
{
  ProfBeginV("%.*s", str8_varg(lnk_string_from_timer_type(t)));
  lnk_timers()[t].begin_us = now_time_us();
}

internal void
lnk_timer_end(LNK_TimerType t)
{
  lnk_timers()[t].end_us = now_time_us();
  ProfEnd();
}

internal U64
lnk_timer_dt(LNK_TimerType t)
{
  LNK_Timer *timers = lnk_timers();
  return timers[t].end_us - timers[t].begin_us;
}

internal String8
lnk_string_from_timer_type(LNK_TimerType type)
{
  switch (type) {
#define X(n,...) case LNK_Timer_##n: return str8_lit(Stringify(n));
    LNK_TimerType_XList
#undef X
  default: InvalidPath;
  }
  return str8_zero();
}

