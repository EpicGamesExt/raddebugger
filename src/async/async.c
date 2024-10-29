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
  async_shared->u2w_ring_size  = MB(8);
  async_shared->u2w_ring_base  = push_array_no_zero(arena, U8, async_shared->u2w_ring_size);
  async_shared->u2w_ring_mutex = os_mutex_alloc();
  async_shared->u2w_ring_cv    = os_condition_variable_alloc();
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
  B32 result = 0;
  ASYNC_Work work = {0};
  work.work_function = work_function;
  work.input         = params->input;
  work.output        = params->output;
  work.semaphore     = params->semaphore;
  OS_MutexScope(async_shared->u2w_ring_mutex) for(;;)
  {
    U64 unconsumed_size = async_shared->u2w_ring_write_pos - async_shared->u2w_ring_read_pos;
    U64 available_size = async_shared->u2w_ring_size - unconsumed_size;
    if(available_size >= sizeof(work))
    {
      result = 1;
      if(!os_handle_match(params->semaphore, os_handle_zero()))
      {
        os_semaphore_take(params->semaphore, max_U64);
      }
      async_shared->u2w_ring_write_pos += ring_write_struct(async_shared->u2w_ring_base, async_shared->u2w_ring_size, async_shared->u2w_ring_write_pos, &work);
      break;
    }
    if(os_now_microseconds() >= params->endt_us)
    {
      break;
    }
    os_condition_variable_wait(async_shared->u2w_ring_cv, async_shared->u2w_ring_mutex, params->endt_us);
  }
  return result;
}

////////////////////////////////
//~ rjf: Task-Based Work Helper

internal void
async_task_list_push(Arena *arena, ASYNC_TaskList *list, ASYNC_Task t)
{
  ASYNC_TaskNode *n = push_array(arena, ASYNC_TaskNode, 1);
  SLLQueuePush(list->first, list->last, n);
  n->v = t;
  list->count += 1;
}

internal ASYNC_Task
async_task_launch_(ASYNC_WorkFunctionType *work_function, ASYNC_WorkParams *params)
{
  ASYNC_Task task = {0};
  task.semaphore = os_semaphore_alloc(1, 1, str8_zero());
  ASYNC_WorkParams params_refined = {0};
  MemoryCopyStruct(&params_refined, params);
  params_refined.endt_us = max_U64;
  params_refined.semaphore = task.semaphore;
  if(params_refined.output == 0)
  {
    params_refined.output = &task.output;
  }
  async_push_work_(work_function, &params_refined);
  return task;
}

internal void *
async_task_join(ASYNC_Task task)
{
  os_semaphore_take(task.semaphore, max_U64);
  os_semaphore_release(task.semaphore);
  MemoryZeroStruct(&task.semaphore);
  void *result = (void *)ins_atomic_u64_eval(&task.output);
  return result;
}

////////////////////////////////
//~ rjf: Work Thread Entry Point

internal void
async_work_thread__entry_point(void *p)
{
  U64 thread_idx = (U64)p;
  ThreadNameF("[async] work thread #%I64u", thread_idx);
  for(;;)
  {
    //- rjf: grab next work
    ASYNC_Work work = {0};
    OS_MutexScope(async_shared->u2w_ring_mutex) for(;;)
    {
      U64 unconsumed_size = async_shared->u2w_ring_write_pos - async_shared->u2w_ring_read_pos;
      if(unconsumed_size >= sizeof(work))
      {
        async_shared->u2w_ring_read_pos += ring_read_struct(async_shared->u2w_ring_base, async_shared->u2w_ring_size, async_shared->u2w_ring_read_pos, &work);
        break;
      }
      os_condition_variable_wait(async_shared->u2w_ring_cv, async_shared->u2w_ring_mutex, max_U64);
    }
    
    //- rjf: run work
    void *work_out = work.work_function(thread_idx, work.input);
    
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
  }
}
