// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
tp_execute_tasks(TP_Worker *worker)
{
  TP_Context *pool = worker->pool;
  
  Arena *arena = NULL;
  if (pool->worker_arena) {
    arena = pool->worker_arena->v[worker->id];
  }
  
  for (;;) {
    // get task id
    U64 next_task_id = ins_atomic_u64_inc_eval(&pool->next_task_id);
    if (next_task_id > pool->task_count) {
      break;
    }
    
    // invoke task func
    U64 task_id = next_task_id - 1;
    pool->task_func(arena, worker->id, task_id, pool->task_data);
  }
}

internal void
tp_worker_main(void *raw_worker)
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);

  TP_Worker *worker = (TP_Worker *)raw_worker;
  TP_Context *pool = worker->pool;

  while (pool->is_live) {
    if (os_semaphore_take(pool->task_semaphore, max_U64)) {
      tp_execute_tasks(worker);
      
      // before last worker takes semaphore wake up main worker
      U64 take_count = ins_atomic_u64_dec_eval(&pool->take_count);
      if (take_count == 1) {
        os_semaphore_drop(pool->main_semaphore);
      }
    } else {
      Assert(!"time out");
    }
  }
}

internal TP_Context * 
tp_alloc(Arena *arena, U32 worker_count)
{
  ProfBeginDynamic("Alloc Thread Pool [Worker Count: %u]", worker_count);
  Assert(worker_count > 0);
  
  // init pool
  TP_Context *pool = push_array(arena, TP_Context, 1);
  if (worker_count > 1) {
    pool->task_semaphore = os_semaphore_alloc(0, worker_count - 1, str8(0,0));
    pool->main_semaphore = os_semaphore_alloc(0, 1, str8(0,0));
  }
  pool->is_live        = 1;
  pool->worker_count   = worker_count;
  pool->worker_arr     = push_array(arena, TP_Worker, worker_count);
  
  // init worker data
  for (U64 i = 0; i < worker_count; i += 1) {
    TP_Worker *worker = &pool->worker_arr[i];
    worker->id = i;
    worker->pool = pool;
  }
  
  // launch worker threads
  for (U64 i = 1; i < worker_count; i += 1) {
    TP_Worker *worker = &pool->worker_arr[i];
    worker->handle = os_thread_launch(tp_worker_main, worker, 0);
  }
  
  ProfEnd();
  return pool;
}

internal void
tp_release(TP_Context *pool)
{
  pool->is_live = 0;
  os_semaphore_release(pool->task_semaphore);
  os_semaphore_release(pool->main_semaphore);
  for (U64 i = 1; i < pool->worker_count; i += 1) {
    os_thread_detach(pool->worker_arr[i].handle);
  }
  MemoryZeroStruct(pool);
}

internal TP_Arena *
tp_arena_alloc(TP_Context *pool)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  Arena **arr = push_array(scratch.arena, Arena *, pool->worker_count);
  for (U64 i = 0; i < pool->worker_count; ++i) {
    arr[i] = arena_alloc();
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
tp_for_parallel(TP_Context *pool, TP_Arena *arena, U64 task_count, TP_TaskFunc *task_func, void *task_data)
{
  Assert(!arena || arena->count == pool->worker_count);

  // setup pool state
  pool->worker_arena = arena;
  pool->task_count = task_count;
  pool->task_func = task_func;
  pool->task_data = task_data;
  pool->next_task_id = 0;
  pool->take_count = 0;
  
  // do we have enough work for other workers?
  pool->take_count = Min(pool->task_count, pool->worker_count);
  U64 drop_count = pool->take_count;
  for (U64 worker_idx = 1; worker_idx < drop_count; worker_idx += 1) {
    os_semaphore_drop(pool->task_semaphore);
  }
  
  // execute tasks on main worker too
  TP_Worker *main_worker = &pool->worker_arr[0];
  tp_execute_tasks(main_worker);
  
  if (drop_count > 1) {
    // wait for workers to finish assigned tasks
    os_semaphore_take(pool->main_semaphore, max_U64);
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
