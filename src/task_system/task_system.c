////////////////////////////////
//~ rjf: Basic Type Functions

internal TS_Ticket
ts_ticket_zero(void)
{
  TS_Ticket ticket = {0};
  return ticket;
}

internal void
ts_ticket_list_push(Arena *arena, TS_TicketList *list, TS_Ticket ticket)
{
  TS_TicketNode *n = push_array(arena, TS_TicketNode, 1);
  n->v = ticket;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

////////////////////////////////
//~ rjf: Top-Level Layer Initialization

internal void
ts_init(void)
{
  Arena *arena = arena_alloc();
  ts_shared = push_array(arena, TS_Shared, 1);
  ts_shared->arena = arena;
  ts_shared->artifact_slots_count = 1024;
  ts_shared->artifact_stripes_count = 64;
  ts_shared->artifact_slots = push_array(arena, TS_TaskArtifactSlot, ts_shared->artifact_slots_count);
  ts_shared->artifact_stripes = push_array(arena, TS_TaskArtifactStripe, ts_shared->artifact_stripes_count);
  for(U64 idx = 0; idx < ts_shared->artifact_stripes_count; idx += 1)
  {
    ts_shared->artifact_stripes[idx].arena = arena_alloc();
    ts_shared->artifact_stripes[idx].cv = os_condition_variable_alloc();
    ts_shared->artifact_stripes[idx].rw_mutex = os_rw_mutex_alloc();
  }
  ts_shared->u2t_ring_size = MB(1);
  ts_shared->u2t_ring_base = push_array_no_zero(arena, U8, ts_shared->u2t_ring_size);
  ts_shared->u2t_ring_mutex = os_mutex_alloc();
  ts_shared->u2t_ring_cv = os_condition_variable_alloc();
  ts_shared->task_threads_count = os_logical_core_count()-1;
  ts_shared->task_threads = push_array(arena, TS_TaskThread, ts_shared->task_threads_count);
  for(U64 idx = 0; idx < ts_shared->task_threads_count; idx += 1)
  {
    ts_shared->task_threads[idx].arena = arena_alloc();
    ts_shared->task_threads[idx].thread = os_launch_thread(ts_task_thread__entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Top-Level Accessors

internal U64
ts_thread_count(void)
{
  return ts_shared->task_threads_count;
}

////////////////////////////////
//~ rjf: High-Level Task Kickoff / Joining

internal TS_Ticket
ts_kickoff(TS_TaskFunctionType *entry_point, Arena **optional_arena_ptr, void *p)
{
  // rjf: obtain number & slot/stripefor next artifact
  U64 artifact_num = ins_atomic_u64_inc_eval(&ts_shared->artifact_num_gen);
  U64 slot_idx = artifact_num%ts_shared->artifact_slots_count;
  U64 stripe_idx = slot_idx%ts_shared->artifact_stripes_count;
  TS_TaskArtifactSlot *slot = &ts_shared->artifact_slots[slot_idx];
  TS_TaskArtifactStripe *stripe = &ts_shared->artifact_stripes[stripe_idx];
  
  // rjf: allocate artifact
  TS_TaskArtifact *artifact = 0;
  OS_MutexScopeW(stripe->rw_mutex)
  {
    artifact = stripe->free_artifact;
    if(artifact != 0)
    {
      SLLStackPop(stripe->free_artifact);
    }
    else
    {
      artifact = push_array_no_zero(stripe->arena, TS_TaskArtifact, 1);
    }
    artifact->num          = artifact_num;
    artifact->task_is_done = 0;
    artifact->result       = 0;
  }
  
  // rjf: form ticket out of artifact info
  TS_Ticket ticket = {artifact_num, (U64)artifact};
  
  // rjf: push task info to task ring buffer
  OS_MutexScope(ts_shared->u2t_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ts_shared->u2t_ring_write_pos - ts_shared->u2t_ring_read_pos;
    U64 available_size = ts_shared->u2t_ring_size-unconsumed_size;
    if(available_size >= sizeof(entry_point) + sizeof(p) + sizeof(ticket))
    {
      Arena *task_arena = 0;
      if(optional_arena_ptr != 0)
      {
        task_arena = *optional_arena_ptr;
      }
      ts_shared->u2t_ring_write_pos += ring_write_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_write_pos, &entry_point);
      ts_shared->u2t_ring_write_pos += ring_write_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_write_pos, &task_arena);
      ts_shared->u2t_ring_write_pos += ring_write_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_write_pos, &p);
      ts_shared->u2t_ring_write_pos += ring_write_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_write_pos, &ticket);
      if(optional_arena_ptr != 0)
      {
        *optional_arena_ptr = 0;
      }
      break;
    }
    os_condition_variable_wait(ts_shared->u2t_ring_cv, ts_shared->u2t_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ts_shared->u2t_ring_cv);
  
  return ticket;
}

internal void *
ts_join(TS_Ticket ticket, U64 endt_us)
{
  void *result = 0;
  U64 artifact_num = ticket.u64[0];
  U64 slot_idx = artifact_num%ts_shared->artifact_slots_count;
  U64 stripe_idx = slot_idx%ts_shared->artifact_stripes_count;
  TS_TaskArtifactSlot *slot = &ts_shared->artifact_slots[slot_idx];
  TS_TaskArtifactStripe *stripe = &ts_shared->artifact_stripes[stripe_idx];
  TS_TaskArtifact *artifact = (TS_TaskArtifact *)ticket.u64[1];
  if(artifact != 0)
  {
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      B64 task_is_done = artifact->task_is_done;
      if(task_is_done)
      {
        OS_MutexScopeRWPromote(stripe->rw_mutex)
        {
          result = artifact->result;
          SLLStackPush(stripe->free_artifact, artifact);
        }
        break;
      }
      if(os_now_microseconds() >= endt_us)
      {
        break;
      }
      os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Task Threads

internal void
ts_u2t_dequeue_task(TS_TaskFunctionType **entry_point_out, Arena **arena_out, void **p_out, TS_Ticket *ticket_out)
{
  OS_MutexScope(ts_shared->u2t_ring_mutex) for(;;)
  {
    U64 unconsumed_size = ts_shared->u2t_ring_write_pos - ts_shared->u2t_ring_read_pos;
    if(unconsumed_size >= sizeof(*entry_point_out) + sizeof(*p_out) + sizeof(*ticket_out))
    {
      ts_shared->u2t_ring_read_pos += ring_read_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_read_pos, entry_point_out);
      ts_shared->u2t_ring_read_pos += ring_read_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_read_pos, arena_out);
      ts_shared->u2t_ring_read_pos += ring_read_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_read_pos, p_out);
      ts_shared->u2t_ring_read_pos += ring_read_struct(ts_shared->u2t_ring_base, ts_shared->u2t_ring_size, ts_shared->u2t_ring_read_pos, ticket_out);
      break;
    }
    os_condition_variable_wait(ts_shared->u2t_ring_cv, ts_shared->u2t_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(ts_shared->u2t_ring_cv);
}

internal void
ts_task_thread__entry_point(void *p)
{
  U64 thread_idx = (U64)p;
  ThreadNameF("[ts] task thread #%I64u", thread_idx+1);
  TS_TaskThread *thread = &ts_shared->task_threads[thread_idx];
  for(;;)
  {
    //- rjf: grab next task
    TS_TaskFunctionType *task_function = 0;
    Arena *task_arena = 0;
    void *task_params = 0;
    TS_Ticket task_ticket = {0};
    ts_u2t_dequeue_task(&task_function, &task_arena, &task_params, &task_ticket);
    
    //- rjf: use task thread's arena if none specified
    if(task_arena == 0)
    {
      task_arena = thread->arena;
    }
    
    //- rjf: run task
    void *task_result = task_function(task_arena, thread_idx, task_params);
    
    //- rjf: store into artifact
    U64 artifact_num = task_ticket.u64[0];
    U64 slot_idx = artifact_num%ts_shared->artifact_slots_count;
    U64 stripe_idx = slot_idx%ts_shared->artifact_stripes_count;
    TS_TaskArtifactSlot *slot = &ts_shared->artifact_slots[slot_idx];
    TS_TaskArtifactStripe *stripe = &ts_shared->artifact_stripes[stripe_idx];
    TS_TaskArtifact *artifact = (TS_TaskArtifact *)task_ticket.u64[1];
    OS_MutexScopeW(stripe->rw_mutex)
    {
      artifact->task_is_done = 1;
      artifact->result = task_result;
    }
    os_condition_variable_broadcast(stripe->cv);
  }
}
