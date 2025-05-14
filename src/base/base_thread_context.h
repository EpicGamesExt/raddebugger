// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_THREAD_CONTEXT_H
#define BASE_THREAD_CONTEXT_H

////////////////////////////////
// NOTE(allen): Thread Context

typedef struct TCTX TCTX;
struct TCTX
{
  Arena *arenas[2];
  
  U8 thread_name[32];
  U64 thread_name_size;
  
  char *file_name;
  U64 line_number;
};

////////////////////////////////
// NOTE(allen): Thread Context Functions

internal void      tctx_init_and_equip(TCTX *tctx);
internal void      tctx_release(void);
internal TCTX*     tctx_get_equipped(void);

internal Arena*    tctx_get_scratch(Arena **conflicts, U64 count);

internal void      tctx_set_thread_name(String8 name);
internal String8   tctx_get_thread_name(void);

internal void      tctx_write_srcloc(char *file_name, U64 line_number);
internal void      tctx_read_srcloc(char **file_name, U64 *line_number);
#define tctx_write_this_srcloc() tctx_write_srcloc(__FILE__, __LINE__)

#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)

#endif // BASE_THREAD_CONTEXT_H
