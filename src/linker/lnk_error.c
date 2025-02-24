// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

static LNK_ErrorMode g_error_mode_arr[LNK_Error_Count];
static LNK_ErrorCodeStatus g_error_code_status_arr[LNK_Error_Count];
static B32 g_log_status[LNK_Log_Count];

internal void
lnk_exit(int code)
{
  exit(code);
}

internal void
lnk_init_error_handler(void)
{
  for (int i = LNK_Error_StopFirst; i < LNK_Error_StopLast; ++i) {
    g_error_mode_arr[i] = LNK_ErrorMode_Stop;
  }
  for (int i = LNK_Error_First; i < LNK_Error_Last; ++i) {
   g_error_mode_arr[i] = LNK_ErrorMode_Continue;
  }
  for (int i = LNK_Warning_First; i < LNK_Warning_Last; ++i) {
    g_error_mode_arr[i] = LNK_ErrorMode_Warn;
  }
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
  if (g_error_mode_arr[code] == LNK_ErrorMode_Ignore) {
    return;
  }
  if (lnk_is_error_code_ignored(code)) {
    return;
  }
  
  Temp scratch = scratch_begin(0,0);
  String8 message = push_str8fv(scratch.arena, fmt, args);
  String8 string = push_str8f(scratch.arena, "%S(%03d): %S\n", lnk_string_from_error_mode(g_error_mode_arr[code]), code, message);
  fprintf(stderr, "%.*s", str8_varg(string));
  scratch_end(scratch);
  
  if (g_error_mode_arr[code] == LNK_ErrorMode_Stop) {
    lnk_exit(code);
  }
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
      lnk_error(code, "%S(%S): %S", lib_path, obj_path, text);
    } else {
      lnk_error(code, "%S: %S", obj_path, text);
    }
  } else {
    lnk_error(code, "%S", text);
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

  fprintf(stderr, "\t");
  fprintf(stderr, "%.*s", str8_varg(string));
  fprintf(stderr, "\n");

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
lnk_suppress_error(LNK_ErrorCode code)
{
  g_error_code_status_arr[code] = LNK_ErrorCodeStatus_Ignore;
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
  fprintf(stderr, "internal error #%03d in %s:%u\n", code, file, line);
  fprintf(stderr, "\t%.*s\n", str8_varg(issue));
  
  va_end(args);
  scratch_end(scratch);
}

