// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if PROFILE_SPALL
global SpallBuffer *spall_buffers[1024];
global U64 spall_buffer_count = 0;

internal inline void
spall_begin(char *fmt, ...)
{
  if(spall_buffer.data == 0)
  {
    spall_buffer.length = MB(1);
    spall_buffer.data = reserve_memory(spall_buffer.length);
    commit_memory(spall_buffer.data, spall_buffer.length);
    spall_buffer_init(&spall_profile, &spall_buffer);
    U64 idx = ins_atomic_u64_inc_eval(&spall_buffer_count) - 1;
    if(idx < ArrayCount(spall_buffers))
    {
      spall_buffers[idx] = &spall_buffer;
    }
  }
  if(spall_pid == 0)
  {
    spall_pid = get_process_info()->pid;
  }
  if(spall_tid == 0)
  {
    spall_tid = tid();
  }
  char name[256];
  va_list args;
  va_start(args, fmt);
  int size = raddbg_vsnprintf(name, sizeof(name), fmt, args);
  va_end(args);
  size = Clamp(0, size, (int)sizeof(name) - 1);
  spall_buffer_begin_ex(&spall_profile, &spall_buffer, name, size, now_time_us(), spall_tid, spall_pid);
}

internal void
spall_thread_end(void)
{
  U64 count = Min(spall_buffer_count, ArrayCount(spall_buffers));
  for(U64 idx = 0; idx < count; idx += 1)
  {
    if(spall_buffers[idx] == &spall_buffer)
    {
      spall_buffer_flush(&spall_profile, &spall_buffer);
      spall_buffers[idx] = 0;
    }
  }
}

internal void
spall_flush_all(void)
{
  U64 count = Min(spall_buffer_count, ArrayCount(spall_buffers));
  for(U64 idx = 0; idx < count; idx += 1)
  {
    if(spall_buffers[idx] != 0)
    {
      spall_buffer_flush(&spall_profile, spall_buffers[idx]);
    }
  }
  spall_flush(&spall_profile);
}
#endif
