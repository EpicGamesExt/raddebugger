// Copyright (c) 2024 Epic Games Tools
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

