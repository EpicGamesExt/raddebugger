// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals/Thread-Locals

C_LINKAGE thread_static Log *log_active;
#if !BUILD_SUPPLEMENTARY_UNIT
C_LINKAGE thread_static Log *log_active = 0;
#endif

////////////////////////////////
//~ rjf: Log Creation/Selection

internal Log *
log_alloc(void)
{
  Arena *arena = arena_alloc();
  Log *log = push_array(arena, Log, 1);
  log->arena = arena;
  log->log_buffer_start_pos = arena_pos(arena);
  return log;
}

internal void
log_release(Log *log)
{
  arena_release(log->arena);
}

internal void
log_select(Log *log)
{
  log_active = log;
}

////////////////////////////////
//~ rjf: Log Building/Clearing

internal void
log_msg(String8 string)
{
  if(log_active != 0)
  {
    str8_list_push(log_active->arena, &log_active->log_buffer_strings, string);
  }
}

internal void
log_msgf(char *fmt, ...)
{
  if(log_active != 0)
  {
    Temp scratch = scratch_begin(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    log_msg(string);
    va_end(args);
    scratch_end(scratch);
  }
}

internal void
log_clear(void)
{
  if(log_active != 0)
  {
    arena_pop_to(log_active->arena, log_active->log_buffer_start_pos);
    MemoryZeroStruct(&log_active->log_buffer_strings);
  }
}
