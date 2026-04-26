// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

static Mutex               g_log_mutex;
static B32                 g_log_status           [LNK_Log_Count];
static LNK_ErrorMode       g_error_mode_arr       [LNK_Error_Count];
static LNK_ErrorCodeStatus g_error_code_status_arr[LNK_Error_Count];

internal void
lnk_fprintf(FILE *f, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  mutex_take(g_log_mutex);
  fprintf(f, "%.*s", str8_varg(string));
  mutex_drop(g_log_mutex);
  va_end(args);
  scratch_end(scratch);
}

internal void
lnk_exit(int code)
{
  mutex_take(g_log_mutex);
  fflush(stdout);
  fflush(stderr);
  _exit(code);
  InvalidPath;
}

internal void
lnk_log_begin(void)
{
  for (int i = LNK_Error_StopFirst;     i < LNK_Error_StopLast;     ++i) { g_error_mode_arr[i] = LNK_ErrorMode_Stop;     }
  for (int i = LNK_Error_ContinueFirst; i < LNK_Error_ContinueLast; ++i) { g_error_mode_arr[i] = LNK_ErrorMode_Continue; }
  for (int i = LNK_Warning_First;       i < LNK_Warning_Last;       ++i) { g_error_mode_arr[i] = LNK_ErrorMode_Warn;     }
  g_log_mutex = mutex_alloc();
}

internal void
lnk_log_end(void)
{
  mutex_release(g_log_mutex);
}

internal String8
lnk_string_from_error_mode(LNK_ErrorMode mode)
{
  switch (mode) {
  case LNK_ErrorMode_Ignore:   return str8_lit("Ignore");
  case LNK_ErrorMode_Continue: return str8_lit("Error");
  case LNK_ErrorMode_Stop:     return str8_lit("Error");
  case LNK_ErrorMode_Warn:     return str8_lit("Warning");
  }
  return str8_zero();
}

internal void
lnk_errorfv(LNK_ErrorCode code, char *fmt, va_list args)
{
  if (lnk_is_error_code_ignored(code)) { return; }
  Temp scratch = scratch_begin(0,0);
  String8 message = push_str8fv(scratch.arena, fmt, args);
  lnk_fprintf(stderr, "%S(%03d): %S\n", lnk_string_from_error_mode(g_error_mode_arr[code]), code, message);
  scratch_end(scratch);
  if (g_error_mode_arr[code] == LNK_ErrorMode_Stop) { lnk_exit(code); }
}

internal void
lnk_error(LNK_ErrorCode code, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  lnk_errorfv(code, fmt, args);
  va_end(args);
}

internal void
lnk_error_with_loc_fv(LNK_ErrorCode code, String8 obj_path, String8 lib_path, char *fmt, va_list args)
{
  Temp scratch = scratch_begin(0, 0);
  String8 text = push_str8fv(scratch.arena, fmt, args);
  if (obj_path.size) {
    if (lib_path.size) {
      lnk_error(code, "%S(%S): %S", lib_path, str8_skip_last_slash(obj_path), text);
    } else {
      lnk_error(code, "%S: %S", obj_path, text);
    }
  } else {
    lnk_error(code, "RADLINK: %S", text);
  }
  scratch_end(scratch);
}

internal void
lnk_error_with_loc(LNK_ErrorCode code, String8 obj_path, String8 lib_path, char *fmt, ...)
{
  va_list args; va_start(args, fmt);
  lnk_error_with_loc_fv(code, obj_path, lib_path, fmt, args);
  va_end(args);
}

internal void
lnk_supplement_error(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  Temp scratch = scratch_begin(0,0);
  String8 string = push_str8fv(scratch.arena, fmt, args);

  lnk_fprintf(stderr, "\t");
  lnk_fprintf(stderr, "%.*s", str8_varg(string));
  lnk_fprintf(stderr, "\n");

  va_end(args);
  scratch_end(scratch);
}

internal void
lnk_supplement_error_list(String8List list)
{
  for (String8Node *node = list.first; node != 0; node = node->next) {
    lnk_supplement_error("%.*s", str8_varg(node->string));
  }
}

internal void
lnk_ignore_error(LNK_ErrorCode code)
{
  g_error_code_status_arr[code] = LNK_ErrorCodeStatus_Ignore;
}

internal void
lnk_activate_error(LNK_ErrorCode code)
{
  g_error_code_status_arr[code] = LNK_ErrorCodeStatus_Active;
}

internal LNK_ErrorCodeStatus
lnk_get_error_code_status(LNK_ErrorCode code)
{
  return g_error_code_status_arr[code];
}

internal void
lnk_internal_error(LNK_InternalError code, char *file, int line, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  
  String8 issue = push_str8fv(scratch.arena, fmt, args);
  lnk_fprintf(stderr, "internal error #%03d in %s:%u\n", code, file, line);
  lnk_fprintf(stderr, "\t%.*s\n", str8_varg(issue));
  
  va_end(args);
  scratch_end(scratch);
}

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
    lnk_fprintf(stdout, "%.*s\n", str8_varg(string));
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
    "Links",         LNK_Log_Links,
  };
  Assert(ArrayCount(map) == LNK_Log_Count);

  for EachElement(i, map) {
    if (str8_match(str8_cstring(map[i].name), string, StringMatchFlag_CaseInsensitive)) {
      return map[i].type;
    }
  }

  return LNK_Log_Null;
}


