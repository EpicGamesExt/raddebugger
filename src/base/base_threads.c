// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Table Stripe Functions

internal StripeArray
stripe_array_alloc(Arena *arena)
{
  StripeArray array = {0};
  array.count = get_system_info()->logical_processor_count;
  array.v = push_array(arena, Stripe, array.count);
  for EachIndex(idx, array.count)
  {
    array.v[idx].arena = arena_alloc();
    array.v[idx].rw_mutex = rw_mutex_alloc();
    array.v[idx].cv = cond_var_alloc();
  }
  return array;
}

internal void
stripe_array_release(StripeArray *stripes)
{
  for EachIndex(idx, stripes->count)
  {
    arena_release(stripes->v[idx].arena);
    rw_mutex_release(stripes->v[idx].rw_mutex);
    cond_var_release(stripes->v[idx].cv);
  }
}

internal Stripe *
stripe_from_slot_idx(StripeArray *stripes, U64 slot_idx)
{
  Stripe *stripe = &stripes->v[slot_idx%stripes->count];
  return stripe;
}

////////////////////////////////
//~ rjf: Thread Info Helpers

internal void
set_thread_name(String8 string)
{
  ProfThreadName("%.*s", str8_varg(string));
  set_platform_thread_name(string);
}

internal void
set_thread_namef(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  set_thread_name(string);
  va_end(args);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Platform-Abstracted Synchronization Primitive Functions

//- rjf: slow barriers

typedef struct BarrierNode BarrierNode;
struct BarrierNode
{
  BarrierNode *next;
  U64 count;
  U64 threads_left_to_enter;
  U64 threads_left_to_leave;
  RWMutex rw_mutex;
  CondVar cv;
};

typedef struct BarrierTCTX BarrierTCTX;
struct BarrierTCTX
{
  Arena *arena;
  BarrierNode *free_barrier_node;
};

thread_static BarrierTCTX *barrier_tctx = 0;

internal Barrier
slow_barrier_alloc(U64 count)
{
  if(barrier_tctx == 0)
  {
    Arena *arena = arena_alloc();
    barrier_tctx = push_array(arena, BarrierTCTX, 1);
    barrier_tctx->arena = arena;
  }
  BarrierNode *n = barrier_tctx->free_barrier_node;
  if(n != 0)
  {
    SLLStackPop(barrier_tctx->free_barrier_node);
  }
  else
  {
    n = push_array_no_zero(barrier_tctx->arena, BarrierNode, 1);
  }
  MemoryZeroStruct(n);
  n->count = count;
  n->threads_left_to_enter = count;
  n->rw_mutex = rw_mutex_alloc();
  n->cv = cond_var_alloc();
  Barrier result = {(U64)n};
  return result;
}

internal void
slow_barrier_release(Barrier barrier)
{
  if(barrier_tctx == 0)
  {
    Arena *arena = arena_alloc();
    barrier_tctx = push_array(arena, BarrierTCTX, 1);
    barrier_tctx->arena = arena;
  }
  BarrierNode *n = (BarrierNode *)barrier.u64[0];
  rw_mutex_release(n->rw_mutex);
  cond_var_release(n->cv);
  SLLStackPush(barrier_tctx->free_barrier_node, n);
}

internal void
slow_barrier_wait(Barrier barrier)
{
  ProfBeginFunction();
  BarrierNode *n = (BarrierNode *)barrier.u64[0];
  U64 threads_left_to_enter = ins_atomic_u64_dec_eval(&n->threads_left_to_enter);
  
  //- rjf: threads left to enter > 0 => wait
  if(threads_left_to_enter > 0)
  {
    // rjf: first try a spin loop
    B32 done_waiting = 0;
    ProfScope("spin loop wait") for(U64 spin_count = 0; spin_count < 10000; spin_count += 1)
    {
      if(ins_atomic_u64_eval(&n->threads_left_to_leave) != 0)
      {
        done_waiting = 1;
        break;
      }
    }
    
    // rjf: not done waiting -> need to do slow wait on condition variable
    if(!done_waiting) ProfScope("slow wait")
    {
      RWMutexScope(n->rw_mutex, 0) for(;;)
      {
        if(ins_atomic_u64_eval(&n->threads_left_to_leave) != 0)
        {
          break;
        }
        cond_var_wait_rw(n->cv, n->rw_mutex, 0, max_U64);
      }
    }
    
    // rjf: decrement leave counter
    if(ins_atomic_u64_dec_eval(&n->threads_left_to_leave) > 0)
    {
      ProfScope("signal") cond_var_signal(n->cv);
    }
  }
  
  //- rjf: threads left to enter == 0 -> last thread, wakeup
  else
  {
    ins_atomic_u64_eval_assign(&n->threads_left_to_enter, n->count);
    ProfScope("wake up") RWMutexScope(n->rw_mutex, 1)
    {
      ins_atomic_u64_eval_assign(&n->threads_left_to_leave, n->count-1);
    }
    ProfScope("signal") cond_var_signal(n->cv);
  }
  
  //- rjf: wait for threads left to leave == 0
  ProfScope("wait for threads to leave") 
  {
    for(U64 spin_count = 0;; spin_count += 1)
    {
      if(ins_atomic_u64_eval(&n->threads_left_to_leave) == 0)
      {
        break;
      }
    }
  }
  
  ProfEnd();
}
