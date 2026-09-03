// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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

internal void
tp_worker_main_shared(void *raw_worker)
{
  TP_Worker  *worker = raw_worker;
  TP_Context *pool   = worker->pool;
  for (; pool->is_live; ) {
    if (semaphore_take(pool->exec_semaphore, max_U64)) {
      tp_run_tasks(pool, worker);
    }
  }
}

internal TP_Context * 
tp_alloc(Arena *arena, U32 worker_count, U32 max_worker_count, String8 name)
{
  ProfBeginDynamic("Alloc Thread Pool [Worker Count: %u]", worker_count);
  AssertAlways(worker_count > 0);

  B32 is_shared = (name.size > 0);

  // alloc semaphores
  Semaphore exec_semaphore = {0};
  if (worker_count > 1) {
    if (is_shared) {
      AssertAlways(worker_count <= max_worker_count);
      exec_semaphore = semaphore_alloc(0, worker_count, str8_zero());
    }
  }

  // pick entry point for the workers
  void *worker_entry = is_shared ? tp_worker_main_shared : tp_worker_main;

  // init pool
  TP_Context *pool      = push_array(arena, TP_Context, 1);
  pool->exec_semaphore  = exec_semaphore;
  pool->run_barrier     = barrier_alloc(worker_count);
  pool->barrier         = barrier_alloc(worker_count);
  pool->is_live         = 1;
  pool->worker_count    = worker_count;
  pool->worker_arr      = push_array(arena, TP_Worker, worker_count);
  
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
  
  ProfEnd();
  return pool;
}

internal void
tp_release(TP_Context *pool)
{
  pool->is_live = 0;

  B32 is_shared = pool->exec_semaphore.u64[0] != 0;
  if (is_shared) {
    for EachIndex(i, pool->worker_count) {
      semaphore_drop(pool->exec_semaphore);
    }
  }
  for (U64 i = 1; i < pool->worker_count; i += 1) {
    thread_detach(pool->worker_arr[i].handle);
  }
  if (is_shared) {
    semaphore_release(pool->exec_semaphore);
  }
  barrier_release(pool->run_barrier);
  barrier_release(pool->barrier);

  MemoryZeroStruct(pool);
}

internal TP_Arena *
tp_arena_alloc(TP_Context *pool)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  Arena **arr = push_array(scratch.arena, Arena *, pool->worker_count);
  for (U64 i = 0; i < pool->worker_count; ++i) {
    arr[i] = arena_alloc("THREAD_POOL");
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

internal void
tp_for_parallel(TP_Context *pool, TP_Arena *task_arena, U64 task_count, TP_TaskFunc *task_func, void *task_data)
{
  // fast path for single tasks to bypass syncs
  if (task_count == 1) {
    Arena *arena = task_arena ? task_arena->v[0] : 0;
    task_func(arena, 0, 0, task_data, pool);
    return;
  }

  if (task_count) {
    // init run
    pool->task_arena = task_arena;
    pool->task_func  = task_func;
    pool->task_data  = task_data;
    pool->task_count = task_count;
    pool->task_done  = 0;
    pool->task_left  = task_count;

    // if we are in shared mode -> ping
    if (*pool->exec_semaphore.u64) {
      U64 drop_count64 = pool->worker_count - 1;
      U32 drop_count   = safe_cast_u32(drop_count64);
      semaphore_drop_count(pool->exec_semaphore, drop_count);
    }

    // run tasks on main worker
    tp_run_tasks(pool, pool->worker_arr);
    Assert(pool->task_done == task_count);
  }
}

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

