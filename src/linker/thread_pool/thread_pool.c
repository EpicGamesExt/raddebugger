// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//
// DUAL-PATH thread pool.
//
//   NON-SHARED mode (no /RAD_SHARED_THREAD_POOL): UPSTREAM's barrier
//     implementation, VERBATIM. Workers park on a kernel barrier between passes
//     (zero-syscall steady state, no per-pass semaphore traffic). tp_run_tasks
//     brackets the work loop in barrier_wait at entry+exit; tp_worker_main loops
//     calling it; tp_for_parallel just inits state and joins as worker 0. No
//     governor thread, no budget/wake/governor semaphores, no main_semaphore.
//
//   SHARED mode (/RAD_SHARED_THREAD_POOL): OUR cross-process governor. Workers
//     are PARKED on wake_semaphore and woken one-per-grant; a per-process
//     governor thread borrows global budget slots and wakes parked workers;
//     completion is signalled via main_semaphore; path-B barrier passes run a
//     fair-share cohort (tp_barrier_begin/end + tp_for_parallel_reserve).
//
// Dispatch and the worker entry point branch on pool->is_shared (== name.size>0).
// All governor/budget state is allocated only when is_shared, so non-shared mode
// has zero extra threads and zero extra synchronization objects vs upstream.
//

////////////////////////////////////////////////////////////////////////////////
//~ NON-SHARED (upstream barrier) path -- VERBATIM from origin/dev.

internal void
tp_run_tasks(TP_Context *pool, TP_Worker *worker)
{
  barrier_wait(pool->run_barrier);

  for (;;) {
    S64 task_left = ins_atomic_u64_dec_eval(&pool->task_left);

    // are there any tasks left to run?
    if (task_left < 0) {
      break;
    }

    // run task
    Arena *arena   = pool->task_arena ? pool->task_arena->v[worker->id] : 0;
    U64    task_id = pool->task_count - (task_left+1);
    pool->task_func(arena, worker->id, task_id, pool->task_data, pool);

    // update task done count
    ins_atomic_u64_inc_eval(&pool->task_done);
  }

  barrier_wait(pool->run_barrier);
}

internal void
tp_worker_main(void *raw_worker)
{
  TP_Worker  *worker = raw_worker;
  TP_Context *pool   = worker->pool;
  for (; pool->is_live; ) {
    tp_run_tasks(pool, worker);
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ SHARED-mode governor stats (summary line). Zero cost when the pool is off:
//  every call site below is on a shared-mode-only path.

#define TP_STALL_ABORT_US  30000000ull

global TP_SharedStats g_tp_shared_stats;

internal void
tp_stats_level_add(S64 delta)
{
  // transitions are rare (one per grant/release, thousands per link), so a tiny
  // spinlock around the integrator is cheaper than any clever lock-free scheme
  for (; ins_atomic_u64_eval_cond_assign(&g_tp_shared_stats.lock, 1, 0) != 0; ) { }
  U64 now_us = now_time_us();
  g_tp_shared_stats.area_us += (U64)(g_tp_shared_stats.level * (S64)(now_us - g_tp_shared_stats.last_us));
  g_tp_shared_stats.last_us  = now_us;
  g_tp_shared_stats.level   += delta;
  ins_atomic_u64_eval_assign(&g_tp_shared_stats.lock, 0);
}

internal void
tp_stats_park_add(U64 worker_us)
{
  ins_atomic_u64_add_eval(&g_tp_shared_stats.park_us, worker_us);
}

internal void
tp_stats_snapshot(F64 *grant_avg_out, F64 *park_seconds_out)
{
  *grant_avg_out    = 0;
  *park_seconds_out = 0;
  if (g_tp_shared_stats.begin_us != 0) {
    tp_stats_level_add(0); // finalize the integral up to now
    U64 wall_us = g_tp_shared_stats.last_us - g_tp_shared_stats.begin_us;
    if (wall_us > 0) {
      *grant_avg_out = (F64)g_tp_shared_stats.area_us / (F64)wall_us;
    }
    *park_seconds_out = (F64)g_tp_shared_stats.park_us / 1000000.0;
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ SHARED-mode cross-process attach counter (summary line procs=). Counter
//  SEMAPHORE, not a named section -- see the "<pool_name>.nproc.v3" comment in
//  thread_pool.h for why (UBA virtualizes named sections per-process).

global Semaphore g_tp_procs_sem;      // zero handle = not attached
global U32       g_tp_procs_maxseen;  // process-local max of observed n

internal void
tp_procs_attach(Arena *scratch_arena, String8 name)
{
  String8   sem_name = push_str8f(scratch_arena, "%S.nproc." TP_NPROC_V, name);
  Semaphore sem      = semaphore_alloc(0, TP_NPROC_MAX, sem_name); // create-or-open, count starts at 0
  if (sem.u64[0] == 0) {
    return; // best-effort: no semaphore, procs= prints 0/0
  }
  U32 prev = 0;
  if (!semaphore_drop_prev(sem, &prev)) { // attach: hold one permit
    semaphore_release(sem);
    return;
  }
  g_tp_procs_sem     = sem;
  g_tp_procs_maxseen = prev + 1;
}

internal void
tp_procs_snapshot(U32 *attached_out, U32 *maxseen_out)
{
  *attached_out = 0;
  *maxseen_out  = 0;
  if (g_tp_procs_sem.u64[0] != 0) {
    U32 prev = 0;
    if (semaphore_drop_prev(g_tp_procs_sem, &prev)) { // read: +1 ...
      semaphore_take(g_tp_procs_sem, 0);              // ... then undo (0-timeout, count > 0 by construction)
      // prev = count BEFORE the transient release = #attached, which already
      // includes THIS process's attach permit -- no +1 (unlike attach)
      U32 n = prev;
      g_tp_procs_maxseen = Max(g_tp_procs_maxseen, n);
      *attached_out      = n;
    }
    *maxseen_out = g_tp_procs_maxseen;
  }
}

internal void
tp_procs_detach(void)
{
  if (g_tp_procs_sem.u64[0] != 0) {
    semaphore_take(g_tp_procs_sem, 0); // give the attach permit back (0-timeout: never block an exit path)
    semaphore_release(g_tp_procs_sem);
    MemoryZeroStruct(&g_tp_procs_sem);
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ SHARED (cross-process governor) path -- OURS.

internal void
tp_for_parallel_init_state(TP_Context *pool, TP_Arena *task_arena, U64 task_count, TP_TaskFunc *task_func, void *task_data)
{
  pool->task_arena = task_arena;
  pool->task_func  = task_func;
  pool->task_data  = task_data;
  pool->task_count = task_count;
  pool->task_done  = 0;
  ins_atomic_u64_eval_assign(&pool->task_left, task_count);
}

//
// SHARED work loop. Semaphore-completion model (no barrier bracket): pure
// work-stealing on the atomic task_left decrement; the last finisher pings
// main_semaphore so the dispatching main thread can return.
//
internal void
tp_run_tasks_shared(TP_Context *pool, TP_Worker *worker)
{
  for (;;) {
    S64 task_left = ins_atomic_u64_dec_eval(&pool->task_left);

    // are there any tasks left to run?
    if (task_left < 0) {
      break;
    }

    // run task
    Arena *arena   = pool->task_arena ? pool->task_arena->v[worker->id] : 0;
    U64    task_id = pool->task_count - (task_left+1);
    pool->task_func(arena, worker->id, task_id, pool->task_data, pool);

    // cache task count so we dont touch pool memory after atomic inc
    U64 task_count = pool->task_count;

    // on last task ping main thread (main_semaphore is null when worker_count==1,
    // in which case main runs everything inline and never waits)
    U64 task_done = ins_atomic_u64_inc_eval(&pool->task_done);
    if (task_done == task_count && pool->worker_count > 1) {
      semaphore_drop(pool->main_semaphore);
    }
  }
}

//
// SHARED worker. Parked on wake_semaphore. Woken one-per-grant. Two wake kinds:
//   - path A (barrier-free, governor-driven): the worker was woken because the
//     governor acquired a global budget slot for it. When the worker drains
//     (tp_run_tasks_shared returns), it RETURNS that slot: release(budget) +
//     granted--, so the slot can flow to another process mid-pass.
//   - path B (barrier pass): the dispatching thread reserved cohort slots up
//     front and woke exactly the cohort's workers. The worker just runs the pass
//     and re-parks; the dispatcher releases the slots in bulk afterwards. The
//     worker must NOT touch the budget here (cohort must stay live for the pass).
//
internal void
tp_worker_main_shared(void *raw_worker)
{
  TP_Worker  *worker = raw_worker;
  TP_Context *pool   = worker->pool;
  for (; pool->is_live; ) {
    if (!semaphore_take(pool->wake_semaphore, max_U64)) {
      continue;
    }
    if (!pool->is_live) {
      break;
    }
    // capture pass kind at wake time (only one pass kind is active at once)
    B32 barrier_pass = pool->barrier_pass;

    tp_run_tasks_shared(pool, worker);

    if (!barrier_pass) {
      // path A: hand my budget slot back so another process can use it
      tp_stats_level_add(-1);
      ins_atomic_u64_dec_eval(&pool->granted);
      semaphore_drop(pool->budget_semaphore);
    }
  }
}

//
// Per-process governor. Sleeps until main signals a path-A pass is active, then
// acquires global budget slots (only while THIS process has pending demand) and
// wakes one local parked worker per slot. Slots are returned by the workers
// themselves when they drain, so the governor only ever ACQUIRES.
//
internal void
tp_governor_main(void *raw_pool)
{
  TP_Context *pool = raw_pool;
  for (; pool->is_live; ) {
    // wait for a pass to begin (or for shutdown)
    if (!semaphore_take(pool->governor_semaphore, max_U64)) {
      continue;
    }
    if (!pool->is_live) {
      break;
    }

    // Grant slots while the pass is live and there is demand. Cap total live
    // grants at worker_count-1 (main/worker 0 is the worker_count-th runner and
    // never consumes a slot). `granted` is decremented by workers as they drain.
    for (; ins_atomic_u32_eval(&pool->pass_active); ) {
      S64 task_left = ins_atomic_u64_eval((U64 *)&pool->task_left);
      S64 demand    = task_left > 0 ? task_left : 0;
      S64 cap       = (S64)pool->worker_count - 1;
      S64 live      = ins_atomic_u64_eval((U64 *)&pool->granted);
      S64 want      = Min(cap, demand) - live;

      if (want > 0) {
        // Bounded wait so we re-check pass_active/demand and never block forever
        // on budget that may never free if the pass ends first.
        U64 wait_begin_us = now_time_us();
        B32 got_slot      = semaphore_take(pool->budget_semaphore, wait_begin_us + 1000);
        // stats: while we waited here, `want` runnable workers sat parked on budget
        tp_stats_park_add((now_time_us() - wait_begin_us) * (U64)want);
        if (got_slot) {
          // Publish the grant (granted++) BEFORE checking pass_active, and ABORT
          // with granted-- if the pass already ended. This makes main's path-A
          // drain-spin (waits granted==0) observe any in-flight grant and block
          // until the governor resolves it -- so main cannot exit tp_for_parallel
          // and start a path-B barrier pass (which sets barrier_pass=1) while a
          // grant is pending. Hence a woken worker ALWAYS captures barrier_pass==0
          // for a path-A grant and does its paired granted--.
          //
          // The earlier "check pass_active, then granted++" ordering was NOT
          // atomic: the governor could pass the check, get preempted while main
          // ended the pass + drained granted to 0 + started a path-B pass, then
          // wake a worker that captured barrier_pass==1, skipped granted--, and
          // wedged granted>0 forever (observed: main spinning in tp_for_parallel,
          // all workers parked). granted++ first closes that window.
          ins_atomic_u64_inc_eval(&pool->granted);
          if (ins_atomic_u32_eval(&pool->pass_active)) {
            tp_stats_level_add(+1);
            semaphore_drop(pool->wake_semaphore);
          } else {
            ins_atomic_u64_dec_eval(&pool->granted);   // abort: pass ended
            semaphore_drop(pool->budget_semaphore);     // give the slot back
          }
        }
      } else {
        // This pass needs no more grants. task_left only decreases, and a worker
        // returns its grant only after draining the shared queue: demand cannot
        // become uncovered again within this pass. Park on the outer semaphore;
        // the next path-A dispatch (or shutdown) supplies a persistent wake.
        // Sleep(0) here just yielded repeatedly, burning a core while long-running
        // tasks finished, including the entire explicit input-unmap pass.
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ Alloc / release (dual).

internal TP_Context *
tp_alloc(Arena *arena, U32 worker_count, U32 max_worker_count, String8 name)
{
  ProfBeginDynamic("Alloc Thread Pool [Worker Count: %u]", worker_count);
  AssertAlways(worker_count > 0);

  B32   is_shared = (name.size > 0);
  Temp  scratch   = scratch_begin(&arena, 1);

  // init pool
  TP_Context *pool   = push_array(arena, TP_Context, 1);
  pool->run_barrier  = barrier_alloc(worker_count);
  pool->barrier      = barrier_alloc(worker_count);
  pool->is_live      = 1;
  pool->is_shared    = is_shared;
  pool->worker_count = worker_count;
  pool->worker_arr   = push_array(arena, TP_Worker, worker_count);

  // alloc semaphores
  if (is_shared) {
    // SHARED: governor + budget + wake + completion. Only allocated here, so
    // non-shared mode pays for none of it.
    if (worker_count > 1) {
      AssertAlways(worker_count <= max_worker_count);

      pool->main_semaphore = semaphore_alloc(0, 1, str8_zero());

      // ONE NAMED cross-process semaphore. CreateSemaphoreW on an existing name
      // returns the existing object (first process inits with this count; later
      // processes attach and the supplied count is ignored by the OS), so all
      // processes share one BUDGET.
      //   BUDGET: init=max=max_worker_count (the machine core budget).
      // FAIR-SHARE: there is no longer a barrier-lock. A barrier pass (path B)
      // does NOT amass the full cohort; it runs at whatever budget is free right
      // now (best-effort), so multiple processes can run barrier passes
      // concurrently and none can deadlock waiting to amass the machine.
      // ".v2" LAYOUT-VERSION suffix: see TP_SharedBlock in thread_pool.h -- old
      // exes ("%S.budget", no procs section) and new exes must never share
      // kernel objects for the same pool name
      String8 budget_name = push_str8f(scratch.arena, "%S.budget." TP_SHARED_V, name);
      pool->budget_semaphore    = semaphore_alloc(max_worker_count, max_worker_count, budget_name);
      pool->max_worker_count    = max_worker_count;

      // local wake/governor signalling. governor_semaphore is a 0/1 "at least one
      // pending pass" flag: main pings it with semaphore_drop_if_room (a redundant
      // ping while one is already pending is a harmless no-op, since the pending
      // signal will make the governor re-evaluate the current pass_active anyway).
      pool->wake_semaphore      = semaphore_alloc(0, worker_count, str8_zero());
      pool->governor_semaphore  = semaphore_alloc(0, 1, str8_zero());
    }
  }

  // pick entry point for the workers
  void *worker_entry = is_shared ? tp_worker_main_shared : tp_worker_main;

  // init worker data
  for (U64 i = 0; i < worker_count; i += 1) {
    TP_Worker *worker = &pool->worker_arr[i];
    worker->id        = i;
    worker->pool      = pool;
  }

  // launch worker threads
  for (U64 i = 1; i < worker_count; i += 1) {
    TP_Worker *worker = &pool->worker_arr[i];
    worker->handle    = thread_launch(worker_entry, worker);
  }

  // launch the per-process governor (shared mode only)
  if (is_shared && worker_count > 1) {
    pool->governor_handle = thread_launch(tp_governor_main, pool);
  }

  // stats: start the grant_avg integration window (shared mode only)
  if (is_shared) {
    g_tp_shared_stats.begin_us = g_tp_shared_stats.last_us = now_time_us();
    tp_procs_attach(scratch.arena, name); // summary line procs= (attach counter + peak watermark)
  }

  scratch_end(scratch);
  ProfEnd();
  return pool;
}

internal void
tp_release(TP_Context *pool)
{
  pool->is_live = 0;

  if (pool->is_shared) {
    if (pool->worker_count > 1) {
      // wake governor so it observes !is_live and exits (a pending ping is fine)
      semaphore_drop_if_room(pool->governor_semaphore);
      // wake every parked worker so each observes !is_live and exits. Wakes here
      // are NOT path-A grants (no budget was taken), so mark barrier_pass to keep
      // workers from touching the budget on their way out.
      pool->barrier_pass = 1;
      for (U64 i = 1; i < pool->worker_count; i += 1) {
        semaphore_drop(pool->wake_semaphore);
      }
    }
    for (U64 i = 1; i < pool->worker_count; i += 1) {
      thread_detach(pool->worker_arr[i].handle);
    }
    if (pool->worker_count > 1) {
      thread_detach(pool->governor_handle);
      semaphore_release(pool->budget_semaphore);
      semaphore_release(pool->wake_semaphore);
      semaphore_release(pool->governor_semaphore);
      semaphore_release(pool->main_semaphore);
    }
  } else {
    // NON-SHARED: upstream verbatim. Workers are parked on the barrier; flipping
    // is_live and waking the barrier lets each observe !is_live and exit.
    for (U64 i = 1; i < pool->worker_count; i += 1) {
      thread_detach(pool->worker_arr[i].handle);
    }
  }

  barrier_release(pool->run_barrier);
  barrier_release(pool->barrier);

  MemoryZeroStruct(pool);
}

////////////////////////////////////////////////////////////////////////////////
//~ Arenas / temps -- shared by both modes (unchanged).

internal TP_Arena *
tp_arena_alloc(TP_Context *pool)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  Arena **arr = push_array(scratch.arena, Arena *, pool->worker_count);
  for (U64 i = 0; i < pool->worker_count; ++i) {
    // 2MB commit quantum: these per-worker arenas take the bulk of the link's
    // ~50GB of MEM_COMMIT growth; the default 64KB quantum turns that into
    // ~800K NtAllocateVirtualMemory calls from 64 threads serialized on the
    // process address-space lock. Slack is bounded by workers x live arenas x
    // quantum (single-digit MBs per worker), far below the syscall cost.
    arr[i] = arena_alloc(.commit_size = MB(2), .name = "THREAD_POOL");
  }
  Arena **dst = push_array(arr[0], Arena *, pool->worker_count);
  MemoryCopy(dst, arr, sizeof(Arena*) * pool->worker_count);
  TP_Arena *worker_arena_arr = push_array(arr[0], TP_Arena, 1);
  worker_arena_arr->count = pool->worker_count;
  worker_arena_arr->v = dst;
  scratch_end(scratch);
  ProfEnd();
  return worker_arena_arr;
}

internal void
tp_arena_release(TP_Arena **arena_ptr)
{
  ProfBeginFunction();
  for (U64 i = 1; i < (*arena_ptr)->count; ++i) {
    arena_release((*arena_ptr)->v[i]);
  }
  arena_release((*arena_ptr)->v[0]);
  *arena_ptr = NULL;
  ProfEnd();
}

internal TP_Temp
tp_temp_begin(TP_Arena *arena)
{
  ProfBeginFunction();

  Temp first_temp = temp_begin(arena->v[0]);

  TP_Temp temp;
  temp.count = arena->count;
  temp.v     = push_array_no_zero(first_temp.arena, Temp, arena->count);

  temp.v[0] = first_temp;

  for (U64 arena_idx = 1; arena_idx < arena->count; arena_idx += 1) {
    temp.v[arena_idx] = temp_begin(arena->v[arena_idx]);
  }

  ProfEnd();
  return temp;
}

internal void
tp_temp_end(TP_Temp temp)
{
  ProfBeginFunction();
  for (U64 temp_idx = temp.count - 1; temp_idx > 0; temp_idx -= 1) {
    temp_end(temp.v[temp_idx]);
  }
  ProfEnd();
}

////////////////////////////////////////////////////////////////////////////////
//~ Dispatch (dual).

internal void
tp_for_parallel(TP_Context *pool, TP_Arena *task_arena, U64 task_count, TP_TaskFunc *task_func, void *task_data)
{
  if (task_count == 0) {
    return;
  }

  if (!pool->is_shared) {
    //
    // NON-SHARED: UPSTREAM verbatim. Init state, then join the barrier as worker
    // 0; the already-parked workers (looping in tp_run_tasks) rendezvous at the
    // entry barrier, steal tasks, and rendezvous again at the exit barrier. No
    // semaphores, no governor.
    //
    pool->task_arena = task_arena;
    pool->task_func  = task_func;
    pool->task_data  = task_data;
    pool->task_count = task_count;
    pool->task_done  = 0;
    pool->task_left  = task_count;

    // run tasks on main worker
    tp_run_tasks(pool, &pool->worker_arr[0]);
    return;
  }

  //
  // SHARED: OUR governor dispatch.
  //
  tp_for_parallel_init_state(pool, task_arena, task_count, task_func, task_data);

  if (pool->worker_count == 1) {
    // no workers: main runs everything inline
    tp_run_tasks_shared(pool, &pool->worker_arr[0]);
    return;
  }

  // PATH A: barrier-free dispatch (the common case). Pure work-stealing via the
  // atomic task_left decrement in tp_run_tasks_shared. Main (worker 0) ALWAYS
  // runs and never consumes a global budget slot -> per-process forward-progress
  // guarantee. The governor opportunistically borrows budget slots and wakes
  // local parked workers; each woken worker returns its slot when it drains.

  // announce a path-A pass and let the governor recruit workers as budget frees
  pool->barrier_pass = 0;
  ins_atomic_u32_eval_assign(&pool->pass_active, 1);
  semaphore_drop_if_room(pool->governor_semaphore);

  // main always runs (no slot consumed)
  tp_run_tasks_shared(pool, &pool->worker_arr[0]);

  // all tasks done (last finisher pinged main_semaphore)
  semaphore_take(pool->main_semaphore, max_U64);

  // End the pass so the governor stops issuing new grants.
  ins_atomic_u32_eval_assign(&pool->pass_active, 0);

  // CRITICAL: before returning we must guarantee that no woken worker is still
  // inside tp_run_tasks_shared. Otherwise the next pass's init_state (which resets
  // task_left/task_done) would race a straggler still looping on this pass and
  // corrupt the counters / lose the completion ping -> deadlock.
  //
  // Every governor grant is paired with exactly one wake permit and one worker
  // that, on draining, does `granted--; drop(budget)`. Even grants issued in the
  // tiny window before pass_active was cleared have a pending wake permit that a
  // worker will consume, drain immediately (task_left<0), and account for. So
  // `granted` monotonically drains to 0 once the governor has stopped; spin
  // until it does. This is brief (workers see task_left<0 and exit at once).
  U64 stall_begin_us = now_time_us();
  for (; ins_atomic_u64_eval((U64 *)&pool->granted) != 0; ) {
    sleep_ms(0);
    U64 elapsed_us = now_time_us() - stall_begin_us;
    if (elapsed_us >= TP_STALL_ABORT_US) {
      // Never perform stderr I/O here. Under UBA that enters the WriteFile detour and can block,
      // turning the diagnostic path itself into a permanent hang before the fail-fast executes.
      AssertAlways(0);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ FAIR-SHARE barrier-pass cohort bracket (SHARED path B; no-op in non-shared).
//
// A barrier pass (path B) runs at the cohort this process currently holds, NOT
// the full machine. tp_barrier_begin grabs whatever budget slots are FREE RIGHT
// NOW (best-effort, never blocking to amass), up to worker_count-1, and pins the
// pool to cohort C = 1 (main) + grabbed slots for the pass duration:
//   - pool->worker_count := C   (so every tp->worker_count read -- divide_work,
//                                 lane_count, per-worker array sizing in the
//                                 caller's setup -- sees the cohort)
//   - pool->barrier      := a fresh C-sized barrier (so barrier_wait/broadcast/sum
//                                 rendezvous exactly the C participants)
//   - the grabbed slots are HELD until tp_barrier_end (cohort stays live; the
//     governor only touches budget during a path-A pass, which cannot overlap a
//     barrier pass within this process).
//
// Deadlock-freedom: tp_barrier_begin NEVER blocks on budget. If the machine is
// busy and zero slots are free, C == 1 and the pass runs serially on main. A
// process therefore ALWAYS makes progress (>= main) and never waits to amass ->
// no starvation, no deadlock, no barrier-lock. Output is width-independent
// (proven: w1 == w64), so a cohort-C pass is byte-identical to a full-width pass.
//
// In NON-SHARED mode tp_barrier_begin/end are no-ops (return pool->worker_count /
// early-out) and tp_for_parallel_reserve degrades to the plain upstream
// full-width barrier pass via tp_for_parallel.
//
internal U32
tp_barrier_begin(TP_Context *pool)
{
  if (!pool->is_shared || pool->worker_count == 1) {
    return pool->worker_count; // no-op: non-shared / single-worker
  }
  if (pool->barrier_depth > 0) {
    pool->barrier_depth += 1;   // nested: cohort already pinned
    return pool->worker_count;
  }

  // best-effort grab: take as many free budget slots as we can WITHOUT blocking
  // (endt_us==0 -> WaitForSingleObject(.,0) non-blocking poll). Stop at the first
  // empty take or when we hold worker_count-1.
  U32 want  = pool->worker_count - 1;
  U32 extra = 0;
  for (; extra < want; ) {
    if (semaphore_take(pool->budget_semaphore, 0)) {
      extra += 1;
    } else {
      break; // no more free slots right now
    }
  }

  // FAIR-SHARE FLOOR: the cohort is pinned for the whole bracket, so a bracket
  // opened at a bad instant (siblings momentarily holding the machine) would run
  // a long phase at width 1-2 even after the machine empties. If the free-slot
  // sweep landed below this process's fair share (machine budget / attached
  // processes), keep taking with bounded waits until we reach it or the deadline
  // expires. Slots flow back continuously as sibling path-A workers drain, so
  // this normally fills within a few ms; if every sibling is pinned in its own
  // long bracket the deadline bounds the wait and we proceed with what we hold --
  // never a deadlock, cohort >= 1 always.
  if (extra < want) {
    U32 procs = 0, procs_maxseen = 0;
    tp_procs_snapshot(&procs, &procs_maxseen);
    if (procs > 1) {
      U32 fair = pool->max_worker_count / procs;
      fair     = Clamp(1, fair, want + 1);
      if (1 + extra < fair) {
        U64 deadline_us = now_time_us() + TP_BARRIER_FLOOR_WAIT_US;
        for (; 1 + extra < fair; ) {
          U64 now_us = now_time_us();
          if (now_us >= deadline_us) { break; }
          U64 slice_us = Min(deadline_us - now_us, 5000);
          if (semaphore_take(pool->budget_semaphore, now_us + slice_us)) {
            extra += 1;
          }
        }
      }
    }
  }

  U32 cohort = 1 + extra; // main + grabbed workers

  pool->barrier_saved_workers = pool->worker_count;
  pool->barrier_cohort_extra  = extra;
  pool->barrier_saved         = pool->barrier;
  pool->barrier               = barrier_alloc(cohort);
  pool->worker_count          = cohort;
  pool->barrier_pass          = 1;
  pool->barrier_depth         = 1;

  // stats: cohort slots are held for the whole bracket; the shortfall is the
  // budget we wanted but could not grab (parked lanes while this pass runs)
  pool->barrier_begin_us  = now_time_us();
  pool->barrier_shortfall = want - extra;
  if (extra > 0) { tp_stats_level_add((S64)extra); }

  return cohort;
}

internal void
tp_barrier_end(TP_Context *pool)
{
  if (!pool->is_shared || pool->barrier_saved_workers == 0) {
    return; // no-op: non-shared / single-worker / not in a bracket
  }
  pool->barrier_depth -= 1;
  if (pool->barrier_depth > 0) {
    return; // nested: outer bracket still owns the cohort
  }

  U32 extra = pool->barrier_cohort_extra;

  // stats: release the held slots from the integral; account parked lanes
  if (extra > 0) { tp_stats_level_add(-(S64)extra); }
  if (pool->barrier_shortfall > 0) {
    tp_stats_park_add((now_time_us() - pool->barrier_begin_us) * (U64)pool->barrier_shortfall);
  }
  pool->barrier_begin_us  = 0;
  pool->barrier_shortfall = 0;

  // restore the full-width pool + the original barrier
  barrier_release(pool->barrier);
  pool->barrier      = pool->barrier_saved;
  pool->worker_count = pool->barrier_saved_workers;
  pool->barrier_pass = 0;

  pool->barrier_saved_workers = 0;
  pool->barrier_cohort_extra  = 0;
  MemoryZeroStruct(&pool->barrier_saved);

  // hand the grabbed budget slots back to the machine
  semaphore_drop_n(pool->budget_semaphore, extra);
}

//
// PATH B: barrier-pass dispatch (fair-share). Runs the task once per lane on the
// CURRENT cohort (main + woken workers). If the caller has not already opened a
// tp_barrier_begin/end bracket, this opens one (cohort = whatever is free now),
// runs, and closes it. The passed task_count is ignored for sizing -- the pass
// always runs exactly pool->worker_count tasks (== cohort).
//
// In NON-SHARED mode this degrades to the plain upstream full-width barrier pass
// (tp_for_parallel), so non-shared barrier passes behave exactly as upstream.
//
internal void
tp_for_parallel_reserve(TP_Context *pool, TP_Arena *task_arena, U64 task_count, TP_TaskFunc *task_func, void *task_data)
{
  if (task_count == 0) {
    return;
  }

  if (!pool->is_shared || pool->worker_count == 1) {
    // non-shared (or single worker): identical to the plain dispatch (upstream
    // full-width barrier pass in non-shared mode)
    tp_for_parallel(pool, task_arena, task_count, task_func, task_data);
    return;
  }

  // open a cohort bracket unless the caller already pinned one
  B32 opened = 0;
  if (pool->barrier_depth == 0) {
    tp_barrier_begin(pool);
    opened = 1;
  }

  U32 cohort = pool->worker_count; // pinned for the whole pass

  if (cohort == 1) {
    // machine fully busy: run serially on main (byte-identical -- width independent)
    tp_for_parallel_init_state(pool, task_arena, cohort, task_func, task_data);
    tp_run_tasks_shared(pool, &pool->worker_arr[0]);
  } else {
    tp_for_parallel_init_state(pool, task_arena, cohort, task_func, task_data);

    // wake exactly the cohort's workers (ids 1..cohort-1). These are barrier-pass
    // wakes: workers must NOT return budget on drain -- the slots are held by the
    // bracket and released in tp_barrier_end so the cohort stays live for the pass.
    semaphore_drop_n(pool->wake_semaphore, cohort - 1);

    // main is the cohort-th participant (lane 0)
    tp_run_tasks_shared(pool, &pool->worker_arr[0]);

    // wait for the cohort to finish
    semaphore_take(pool->main_semaphore, max_U64);
  }

  if (opened) {
    tp_barrier_end(pool);
  }
}

////////////////////////////////////////////////////////////////////////////////
//~ Helpers -- shared by both modes (unchanged).

internal Rng1U64 *
tp_divide_work(Arena *arena, U64 item_count, U32 worker_count)
{
  U64      per_count = CeilIntegerDiv(item_count, worker_count);
  Rng1U64 *range_arr = push_array_no_zero(arena, Rng1U64, worker_count + 1);
  for (U64 i = 0; i < worker_count; i += 1) {
    range_arr[i] = rng_1u64(Min(item_count, i * per_count),
                            Min(item_count, i * per_count + per_count));
  }

  // thread_pool_dummy_range:
  range_arr[worker_count] = rng_1u64(item_count, item_count);

  return range_arr;
}

internal void
tp_broadcast_(TP_Context *tp, U64 task_id, void *ptr, U64 ptr_size)
{
  if (task_id == 0) {
    tp->broadcast      = ptr;
    tp->broadcast_size = ptr_size;
  }
  barrier_wait(tp->barrier);

  if (task_id != 0) {
    MemoryCopy(ptr, tp->broadcast, tp->broadcast_size);
  }
  barrier_wait(tp->barrier);
}

internal U64
tp_sum_u64(TP_Context *tp, U64 task_id, U64 v)
{
  if (task_id == 0) {
    tp->sum = 0;
  }
  barrier_wait(tp->barrier);

  ins_atomic_u64_add_eval(&tp->sum, v);
  barrier_wait(tp->barrier);

  U64 result = tp->sum;
  barrier_wait(tp->barrier);
  return result;
}
