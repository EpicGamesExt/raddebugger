// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_set_log_status(LNK_LogType type, B32 is_enabled)
{
  g_log_status[type] = is_enabled;
}

internal B32
lnk_get_log_status(LNK_LogType type)
{
  B32 status = g_log_status[type];
  return status;
}

internal void
lnk_log(LNK_LogType type, char *fmt, ...)
{
  B32 is_log_enabled = g_log_status[type];
  if (is_log_enabled) {
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    fprintf(stdout, "%.*s\n", str8_varg(string));
    va_end(args);
    scratch_end(scratch);
  }
}

internal LNK_LogType
lnk_log_type_from_string(String8 string)
{
  static struct {
    char       *name;
    LNK_LogType type;
  } map[] = {
    "Null",          LNK_Log_Null,
    "Debug",         LNK_Log_Debug,
    "InputObj",      LNK_Log_InputObj,
    "InputLib",      LNK_Log_InputLib,
    "IO_Read",       LNK_Log_IO_Read,
    "IO_Write",      LNK_Log_IO_Write,
    "SizeBreakdown", LNK_Log_SizeBreakdown,
    "LinkStats",     LNK_Log_LinkStats,
    "Timers",        LNK_Log_Timers,
  };
  Assert(ArrayCount(map) == LNK_Log_Count);

  for (U64 i = 0; i < ArrayCount(map); ++i) {
    if (str8_match(str8_cstring(map[i].name), string, StringMatchFlag_CaseInsensitive)) {
      return map[i].type;
    }
  }

  return LNK_Log_Null;
}


