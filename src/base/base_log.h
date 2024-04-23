// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_LOG_H
#define BASE_LOG_H

////////////////////////////////
//~ rjf: Log Types

typedef struct LogScope LogScope;
struct LogScope
{
  LogScope *next;
  U64 pos;
  String8List strings;
};

typedef struct Log Log;
struct Log
{
  Arena *arena;
  LogScope *top_scope;
};

////////////////////////////////
//~ rjf: Log Creation/Selection

internal Log *log_alloc(void);
internal void log_release(Log *log);
internal void log_select(Log *log);

////////////////////////////////
//~ rjf: Log Building

internal void log_msg(String8 string);
internal void log_msgf(char *fmt, ...);

////////////////////////////////
//~ rjf: Log Scopes

internal void log_scope_begin(void);
internal String8 log_scope_end(Arena *arena);

#endif // BASE_LOG_H
