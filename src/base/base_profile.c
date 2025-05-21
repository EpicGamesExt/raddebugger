// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if PROFILE_SPALL
internal inline void
spall_begin(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  spall_buffer_begin(&spall_profile, &spall_buffer, string.str, string.size, os_now_microseconds());
  va_end(args);
  scratch_end(scratch);
}
#endif
