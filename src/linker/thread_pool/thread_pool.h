// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

struct TP_Context;
#define THREAD_POOL_TASK_FUNC(name) void name(Arena *arena, U64 worker_id, U64 task_id, void *raw_task, struct TP_Context *tp)
typedef THREAD_POOL_TASK_FUNC(TP_TaskFunc);

typedef struct TP_Arena
{
  U64     count;
  Arena **v;
} TP_Arena;

typedef struct TP_Temp
{
  U64   count;
  Temp *v;
} TP_Temp;

typedef struct TP_Worker
{
  U64                id;
  struct TP_Context *pool;
  Thread             handle;
} TP_Worker;

typedef struct TP_Context
{
  B32          is_live;
  Semaphore    exec_semaphore;
  Semaphore    task_semaphore;
  Semaphore    main_semaphore;
  Barrier      run_barrier;
  Barrier      barrier;
  void        *broadcast;
  U64          broadcast_size;
  U64          sum;

  // shared (cross-process) governor mode; all zero in non-shared mode
  B32          is_shared;
  Semaphore    budget_semaphore;       // NAMED: global core budget; init=max=max_worker_count
  Semaphore    wake_semaphore;         // local: governor/dispatcher wakes one parked worker per drop
  Semaphore    governor_semaphore;     // local: main pings governor that a path-A pass is active
  Thread       governor_handle;
  volatile U32 pass_active;            // 1 while a path-A (barrier-free) pass is in flight
  volatile U32 barrier_pass;           // 1 while the current wake cohort is a path-B barrier pass
  volatile S64 granted;                // budget slots currently held by woken path-A workers
  U32          max_worker_count;       // machine core budget (budget_semaphore init/max)

  // FAIR-SHARE barrier-pass cohort state (path B). A barrier pass runs at the
  // cohort the governor currently allows this process to hold: main + however
  // many budget slots are free RIGHT NOW (best-effort, never amassed). Pinned
  // for the pass duration. See tp_barrier_begin/tp_barrier_end.
  U32          barrier_depth;          // >0 while inside a tp_barrier_begin/end bracket (re-entrant guard)
  U32          barrier_saved_workers;  // worker_count to restore at tp_barrier_end
  U32          barrier_cohort_extra;   // budget slots held for this barrier pass (cohort = 1 + this)
  Barrier      barrier_saved;          // pool->barrier to restore at tp_barrier_end
  U64          barrier_begin_us;       // stats: bracket open time (for park accounting)
  U32          barrier_shortfall;      // stats: budget slots we wanted but could not grab for this pass

  U32          worker_count;
  TP_Worker   *worker_arr;

  TP_Arena    *task_arena;
  TP_TaskFunc *task_func;
  void        *task_data;
  U64          task_count;
  U64          task_done;
  S64          task_left;
} TP_Context;

internal TP_Context * tp_alloc(Arena *arena, U32 worker_count, U32 max_worker_count, String8 name);
internal void         tp_release(TP_Context *pool);
internal TP_Arena *   tp_arena_alloc(TP_Context *pool);
internal void         tp_arena_release(TP_Arena **arena_ptr);
internal TP_Temp      tp_temp_begin(TP_Arena *arena);
internal void         tp_temp_end(TP_Temp temp);
#define tp_for_parallel_prof(pool, arena, task_count, task_func, task_data, zone_name) ProfBegin(zone_name); tp_for_parallel(pool, arena, task_count, task_func, task_data); ProfEnd();
internal void         tp_for_parallel(TP_Context *pool, TP_Arena *arena, U64 task_count, TP_TaskFunc *task_func, void *task_data);
// FAIR-SHARE barrier-pass cohort bracket. Between tp_barrier_begin and
// tp_barrier_end, pool->worker_count is PINNED to the cohort this process
// currently holds (1 + budget slots free right now, capped at the full count),
// and pool->barrier is sized to that cohort. Returns the cohort count C. A caller
// that pre-distributes work by tp->worker_count (sizes per-worker arrays, builds
// divide_work ranges, sets a task-data .worker_count) MUST do that setup inside
// the bracket so it sees C, then call tp_for_parallel_reserve with task_count==C.
// Re-entrant: nested begins just return the pinned C. In non-shared mode (or
// worker_count==1) it is a no-op that returns worker_count.
internal U32          tp_barrier_begin(TP_Context *pool);
internal void         tp_barrier_end(TP_Context *pool);
// Barrier-pass dispatch: task_func uses barrier_wait/tp_broadcast/tp_sum_u64. The
// cohort is whatever this process currently holds (fair-share): if not already
// inside a tp_barrier_begin/end bracket this opens one itself, runs the pass at
// the pinned cohort, and closes it. Output is width-independent so any cohort
// (down to 1 = main only) produces byte-identical results. The passed task_count
// is IGNORED for sizing; the pass always runs exactly pool->worker_count (==cohort)
// tasks, one per lane. In non-shared mode it is identical to tp_for_parallel.
internal void         tp_for_parallel_reserve(TP_Context *pool, TP_Arena *arena, U64 task_count, TP_TaskFunc *task_func, void *task_data);
#define tp_for_parallel_reserve_prof(pool, arena, task_count, task_func, task_data, zone_name) ProfBegin(zone_name); tp_for_parallel_reserve(pool, arena, task_count, task_func, task_data); ProfEnd();
internal Rng1U64 *    tp_divide_work(Arena *arena, U64 item_count, U32 worker_count);
#define tp_broadcast(p) tp_broadcast_(tp, task_id, p, sizeof(*p))

// SHARED-mode governor stats (for the end-of-link summary line). All counters
// are only ever touched from shared-mode-only code paths, so the non-shared
// pool pays zero cost. `level` is the count of global budget slots this
// process currently HOLDS (path-A grants + path-B cohort extras); it is
// integrated over time (area_us = sum level x dt, QPC-stamped on every
// grant/release transition) so grant_avg = area/wall. `park_us` accumulates
// worker-microseconds spent waiting on budget while this process had pending
// demand (path A: governor budget waits x wanted-worker count; path B: pass
// duration x cohort shortfall).
typedef struct TP_SharedStats
{
  volatile U64 lock;      // spinlock for the {last_us, level, area_us} integrator
  U64          begin_us;  // pool alloc time (grant_avg denominator start)
  U64          last_us;   // last transition stamp
  S64          level;     // budget slots currently held
  U64          area_us;   // integral of level over time
  volatile U64 park_us;   // worker-us parked on budget while work was available
} TP_SharedStats;

internal void tp_stats_level_add(S64 delta);
internal void tp_stats_park_add(U64 worker_us);
internal void tp_stats_snapshot(F64 *grant_avg_out, F64 *park_seconds_out);

// SHARED-mode cross-process attach counter (summary line: procs=<n>/<maxseen>).
// Lives in a named counter SEMAPHORE "<pool_name>.nproc.v3"; the budget
// semaphore is "<pool_name>.budget.v2". Names carry a version suffix so an old
// radlink pointed at the SAME /RAD_SHARED_THREAD_POOL name never shares kernel
// objects with a new one: mixed old/new farms run independent pools instead of
// corrupting one.
//
// WHY A SEMAPHORE: UBA detours CreateFileMapping and VIRTUALIZES named
// sections per-process, so the previous "<pool_name>.procs.v2" shared-memory
// block reported procs=1/1 under UBA. Named SEMAPHORES pass through the detour
// (the budget semaphore is demonstrably shared in prod: fair grants across
// processes), so the counter is the semaphore's own count:
//   attach = release(+1, &prev) -> n = prev+1 (each attached process holds one permit)
//   detach = 0-timeout wait (-1)
//   read   = release(+1, &prev) -> n = prev (prev already counts our own
//            permit), then 0-timeout wait to undo
// The transient read +1 can inflate a concurrent reader's n by 1 -- advisory
// counter, acceptable. Peak cannot be tracked exactly cross-process without
// shm, so `maxseen` is this process's local max of n observed at attach and at
// the summary read. Best-effort: a process that dies without detaching leaves
// the count high until the semaphore object itself dies with its last handle.
#define TP_SHARED_V     "v2"
#define TP_NPROC_V      "v3"
#define TP_NPROC_MAX    (1u << 20) // far above any plausible concurrent-link count
#define TP_BARRIER_FLOOR_WAIT_US 200000 // tp_barrier_begin: max wait to reach the fair-share cohort floor

internal void tp_procs_snapshot(U32 *attached_out, U32 *maxseen_out); // 0/0 when no shared pool
internal void tp_procs_detach(void);                                  // give the permit back + close (idempotent)

