// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_THREAD_CONTEXT_H
#define BASE_THREAD_CONTEXT_H

////////////////////////////////
//~ rjf: Lane Group Context

typedef struct LaneCtx LaneCtx;
struct LaneCtx
{
  U64 lane_idx;
  U64 lane_count;
  Barrier barrier;
};

////////////////////////////////
//~ rjf: Access Scopes

typedef struct AccessPt AccessPt;
struct AccessPt
{
  U64 access_refcount;
  U64 last_time_touched_us;
  U64 last_update_idx_touched;
};

typedef struct Touch Touch;
struct Touch
{
  Touch *next;
  AccessPt *pt;
  CondVar cv;
};

typedef struct Access Access;
struct Access
{
  Access *next;
  Touch *top_touch;
};

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
  
  // rjf: lane context
  LaneCtx lane_ctx;
  
  // rjf: source location info
  char *file_name;
  U64 line_number;
  
  // rjf: accesses
  Arena *access_arena;
  Access *free_access;
  Touch *free_touch;
  
  // rjf: progress
  U64 *progress_counter_ptr;
  U64 *progress_target_ptr;
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

//- rjf: lane metadata
internal LaneCtx tctx_set_lane_ctx(LaneCtx lane_ctx);
internal void tctx_lane_barrier_wait(void);
#define lane_idx() (tctx_selected()->lane_ctx.lane_idx)
#define lane_count() (tctx_selected()->lane_ctx.lane_count)
#define lane_from_task_idx(idx) ((idx)%lane_count())
#define lane_ctx(ctx) tctx_set_lane_ctx((ctx))
#define lane_sync() tctx_lane_barrier_wait()
#define lane_range(count) m_range_from_n_idx_m_count(lane_idx(), lane_count(), (count))

//- rjf: thread names
internal void tctx_set_thread_name(String8 name);
internal String8 tctx_get_thread_name(void);

//- rjf: thread source-locations
internal void tctx_write_srcloc(char *file_name, U64 line_number);
internal void tctx_read_srcloc(char **file_name, U64 *line_number);
#define tctx_write_this_srcloc() tctx_write_srcloc(__FILE__, __LINE__)

//- rjf: access scopes
internal Access *access_open(void);
internal void access_close(Access *access);
internal void access_touch(Access *access, AccessPt *pt, CondVar cv);

//- rjf: access points
internal B32 access_pt_is_expired(AccessPt *pt);

//- rjf: progress counters
#define set_progress_ptr(ptr) (tctx_selected()->progress_counter_ptr = (ptr))
#define set_progress_target_ptr(ptr) (tctx_selected()->progress_target_ptr = (ptr))
#define set_progress(val) (tctx_selected()->progress_counter_ptr ? ins_atomic_u64_eval_assign(tctx_selected()->progress_counter_ptr, (val)) : (void)0)
#define add_progress(val) (tctx_selected()->progress_counter_ptr ? ins_atomic_u64_add_eval(tctx_selected()->progress_counter_ptr, (val)) : (void)0)
#define set_progress_target(val) (tctx_selected()->progress_target_ptr ? ins_atomic_u64_eval_assign(tctx_selected()->progress_target_ptr, (val)) : (void)0)

#endif // BASE_THREAD_CONTEXT_H
