// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_THREAD_CONTEXT_H
#define BASE_THREAD_CONTEXT_H

////////////////////////////////
//~ rjf: Base Per-Thread State Bundle

typedef struct TCTX TCTX;
struct TCTX
{
  // rjf: scratch arenas
  Arena *arenas[2];
  
  // rjf: thread name
  U8 thread_name[32];
  U64 thread_name_size;
  
  // rjf: wavefront info
  U64 wavefront_idx;
  U64 wavefront_count;
  U64 wavefront_barrier_id;
  
  // rjf: source location info
  char *file_name;
  U64 line_number;
};

////////////////////////////////
//~ rjf: Thread Context Functions

//- rjf: thread-context allocation & selection
internal TCTX *tctx_alloc(void);
internal void tctx_release(TCTX *tctx);
internal void tctx_select(TCTX *tctx);
internal TCTX *tctx_selected(void);

//- rjf: scratch arenas
internal Arena *tctx_get_scratch(Arena **conflicts, U64 count);
#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)

//- rjf: wavefront metadata
internal void tctx_set_wavefront_info(U64 wavefront_idx, U64 wavefront_count);
internal void tctx_wavefront_barrier_wait(void);
#define wavefront_thread(idx, count) tctx_set_wavefront_info((idx), (count))
#define wavefront_barrier() tctx_wavefront_barrier_wait()

//- rjf: thread names
internal void tctx_set_thread_name(String8 name);
internal String8 tctx_get_thread_name(void);

//- rjf: thread source-locations
internal void tctx_write_srcloc(char *file_name, U64 line_number);
internal void tctx_read_srcloc(char **file_name, U64 *line_number);
#define tctx_write_this_srcloc() tctx_write_srcloc(__FILE__, __LINE__)

#endif // BASE_THREAD_CONTEXT_H
