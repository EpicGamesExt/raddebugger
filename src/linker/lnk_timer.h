// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define LNK_TimerType_XList \
  X(Image)                  \
  X(Pdb)                    \
  X(Rdi)                    \
  X(Lib)                    \
  X(Debug)                  \
  // end

typedef enum LNK_TimerType
{
#define X(n,...) LNK_Timer_##n,
  LNK_TimerType_XList
#undef X
  LNK_Timer_Count
} LNK_TimerType;

typedef struct LNK_Timer
{
  U64 begin_us;
  U64 end_us;
} LNK_Timer;

internal LNK_Timer * lnk_timers(void);
internal void        lnk_timer_begin(LNK_TimerType timer);
internal void        lnk_timer_end(LNK_TimerType timer);
internal U64         lnk_timer_dt(LNK_TimerType timer);
internal String8     lnk_string_from_timer_type(LNK_TimerType type);

