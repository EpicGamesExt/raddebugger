// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if PROFILE_SPALL
internal inline void
spall_begin(char *fmt, ...)
{
  if(spall_buffer.data == 0)
  {
    spall_buffer.length = MB(1);
    spall_buffer.data   = os_reserve(spall_buffer.length);
    os_commit(spall_buffer.data, spall_buffer.length);
    spall_buffer_init(&spall_profile, &spall_buffer);
  }
  if(spall_pid == 0)
  {
    spall_pid = os_get_process_info()->pid;
  }
  if(spall_tid == 0)
  {
    spall_tid = os_tid();
  }
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  spall_buffer_begin_ex(&spall_profile, &spall_buffer, string.str, string.size, os_now_microseconds(), spall_tid, spall_pid);
  va_end(args);
  scratch_end(scratch);
}
#endif
