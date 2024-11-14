// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Top-Level Layer Initialization

internal void
async_init(void)
{
  Arena *arena = arena_alloc();
  async_shared = push_array(arena, ASYNC_Shared, 1);
  async_shared->arena = arena;
  for EachEnumVal(ASYNC_Priority, p)
  {
    ASYNC_Ring *ring = &async_shared->rings[p];
    ring->ring_size  = MB(8);
    ring->ring_base  = push_array_no_zero(arena, U8, ring->ring_size);
    ring->ring_mutex = os_mutex_alloc();
    ring->ring_cv    = os_condition_variable_alloc();
  }
  async_shared->ring_mutex = os_mutex_alloc();
  async_shared->ring_cv = os_condition_variable_alloc();
  async_shared->work_threads_count = Max(1, os_get_system_info()->logical_processor_count-1);
  async_shared->work_threads   = push_array(arena, OS_Handle, async_shared->work_threads_count);
  for EachIndex(idx, async_shared->work_threads_count)
  {
    async_shared->work_threads[idx] = os_thread_launch(async_work_thread__entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Top-Level Accessors

internal U64
async_thread_count(void)
{
  return async_shared->work_threads_count;
}

////////////////////////////////
//~ rjf: Work Kickoffs

internal B32
async_push_work_(ASYNC_WorkFunctionType *work_function, ASYNC_WorkParams *params)
{
  // rjf: choose ring
  ASYNC_Ring *ring = &async_shared->rings[params->priority];
  
  // rjf: build work package 
  ASYNC_Work work = {0};
  work.work_function = work_function;
  work.input              = params->input;
  work.output             = params->output;
  work.semaphore          = params->semaphore;
  work.completion_counter = params->completion_counter;
  
  // rjf: loop; try to write into user -> writer ring buffer. if we're on a
  // worker thread, determine if we need to execute this task locally on this
  // thread, and skip ring buffer if so.
  B32 queued_in_ring_buffer = 0;
  B32 need_to_execute_on_this_thread = 0;
  OS_MutexScope(ring->ring_mutex) for(;;)
  {
    U64 num_available_work_threads = (async_shared->work_threads_count - ins_atomic_u64_eval(&async_shared->work_threads_live_count));
    if(num_available_work_threads == 0 && async_work_thread_depth > 0)
    {
      need_to_execute_on_this_thread = 1;
      break;
    }
    U64 unconsumed_size = ring->ring_write_pos - ring->ring_read_pos;
    U64 available_size = ring->ring_size - unconsumed_size;
    if(available_size >= sizeof(work))
    {
      queued_in_ring_buffer = 1;
      if(!os_handle_match(params->semaphore, os_handle_zero()))
      {
        os_semaphore_take(params->semaphore, max_U64);
      }
      ring->ring_write_pos += ring_write_struct(ring->ring_base, ring->ring_size, ring->ring_write_pos, &work);
      break;
    }
    if(os_now_microseconds() >= params->endt_us)
    {
      break;
    }
    os_condition_variable_wait(ring->ring_cv, ring->ring_mutex, params->endt_us);
  }
  
  // rjf: broadcast ring buffer cv if we wrote successfully
  if(queued_in_ring_buffer)
  {
    os_condition_variable_broadcast(ring->ring_cv);
    os_condition_variable_broadcast(async_shared->ring_cv);
  }
  
  // rjf: if we did not queue successfully, and we have determined that
  // we need to execute this work on the current thread, then execute the
  // work before returning
  if(need_to_execute_on_this_thread)
  {
    async_execute_work(work);
  }
  
  // rjf: return success signal
  B32 result = (queued_in_ring_buffer || need_to_execute_on_this_thread);
  return result;
}

////////////////////////////////
//~ rjf: Task-Based Work Helper

internal void
async_task_list_push(Arena *arena, ASYNC_TaskList *list, ASYNC_Task *t)
{
  ASYNC_TaskNode *n = push_array(arena, ASYNC_TaskNode, 1);
  SLLQueuePush(list->first, list->last, n);
  n->v = t;
  list->count += 1;
}

internal ASYNC_Task *
async_task_launch_(Arena *arena, ASYNC_WorkFunctionType *work_function, ASYNC_WorkParams *params)
{
  ASYNC_Task *task = push_array(arena, ASYNC_Task, 1);
  task->semaphore = os_semaphore_alloc(1, 1, str8_zero());
  ASYNC_WorkParams params_refined = {0};
  MemoryCopyStruct(&params_refined, params);
  params_refined.endt_us = max_U64;
  params_refined.semaphore = task->semaphore;
  if(params_refined.output == 0)
  {
    params_refined.output = &task->output;
  }
  async_push_work_(work_function, &params_refined);
  return task;
}

internal void *
async_task_join(ASYNC_Task *task)
{
  void *result = 0;
  if(task != 0 && !os_handle_match(task->semaphore, os_handle_zero()))
  {
    os_semaphore_take(task->semaphore, max_U64);
    os_semaphore_release(task->semaphore);
    MemoryZeroStruct(&task->semaphore);
    result = (void *)ins_atomic_u64_eval(&task->output);
  }
  return result;
}

////////////////////////////////
//~ rjf: Work Execution

internal ASYNC_Work
async_pop_work(void)
{
  ProfBeginFunction();
  ASYNC_Work work = {0};
  B32 done = 0;
  ASYNC_Priority taken_priority = ASYNC_Priority_Low;
  OS_MutexScope(async_shared->ring_mutex) for(;!done;)
  {
    for(ASYNC_Priority priority = ASYNC_Priority_High;; priority = (ASYNC_Priority)(priority - 1))
    {
      ASYNC_Ring *ring = &async_shared->rings[priority];
      OS_MutexScope(ring->ring_mutex)
      {
        U64 unconsumed_size = ring->ring_write_pos - ring->ring_read_pos;
        if(unconsumed_size >= sizeof(work))
        {
          ring->ring_read_pos += ring_read_struct(ring->ring_base, ring->ring_size, ring->ring_read_pos, &work);
          done = 1;
          taken_priority = priority;
        }
      }
      if(priority == ASYNC_Priority_Low)
      {
        break;
      }
    }
    if(!done)
    {
      os_condition_variable_wait(async_shared->ring_cv, async_shared->ring_mutex, max_U64);
    }
  }
  os_condition_variable_broadcast(async_shared->ring_cv);
  os_condition_variable_broadcast(async_shared->rings[taken_priority].ring_cv);
  ProfEnd();
  return work;
}

internal void
async_execute_work(ASYNC_Work work)
{
  //- rjf: run work
  async_work_thread_depth += 1;
  void *work_out = work.work_function(async_work_thread_idx, work.input);
  async_work_thread_depth -= 1;
  
  //- rjf: store output
  if(work.output != 0)
  {
    ins_atomic_u64_eval_assign((U64 *)work.output, (U64)work_out);
  }
  
  //- rjf: release semaphore
  if(!os_handle_match(work.semaphore, os_handle_zero()))
  {
    os_semaphore_drop(work.semaphore);
  }
  
  //- rjf: increment completion counter
  if(work.completion_counter != 0)
  {
    ins_atomic_u64_inc_eval(work.completion_counter);
  }
}

////////////////////////////////
//~ rjf: Work Thread Entry Point

internal void
async_work_thread__entry_point(void *p)
{
  U64 thread_idx = (U64)p;
  ThreadNameF("[async] work thread #%I64u", thread_idx);
  async_work_thread_idx = thread_idx;
  for(;;)
  {
    ASYNC_Work work = async_pop_work();
    ins_atomic_u64_inc_eval(&async_shared->work_threads_live_count);
    async_execute_work(work);
    ins_atomic_u64_dec_eval(&async_shared->work_threads_live_count);
  }
}
