// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_LOG_H
#define BASE_LOG_H

////////////////////////////////
//~ rjf: Log Type

typedef struct Log Log;
struct Log
{
  Arena *arena;
  U64 log_buffer_start_pos;
  String8List log_buffer_strings;
};

////////////////////////////////
//~ rjf: Log Creation/Selection

internal Log *log_alloc(void);
internal void log_release(Log *log);
internal void log_select(Log *log);

////////////////////////////////
//~ rjf: Log Building/Clearing

internal void log_msg(String8 string);
internal void log_msgf(char *fmt, ...);
internal void log_clear(void);

#endif // BASE_LOG_H
