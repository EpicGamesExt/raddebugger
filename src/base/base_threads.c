// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Thread Functions

internal Thread
thread_launch(ThreadEntryPointFunctionType *f, void *p)
{
  Thread thread = os_thread_launch(f, p);
  return thread;
}

internal B32
thread_join(Thread thread, U64 endt_us)
{
  B32 result = os_thread_join(thread, endt_us);
  return result;
}

internal void
thread_detach(Thread thread)
{
  os_thread_detach(thread);
}

////////////////////////////////
//~ rjf: Synchronization Primitive Functions

//- rjf: recursive mutexes

internal Mutex mutex_alloc(void)                 {return os_mutex_alloc();}
internal void  mutex_release(Mutex mutex)        {os_mutex_release(mutex);}
internal void  mutex_take(Mutex mutex)           {os_mutex_take(mutex);}
internal void  mutex_drop(Mutex mutex)           {os_mutex_drop(mutex);}

//- rjf: reader/writer mutexes

internal RWMutex rw_mutex_alloc(void)            {return os_rw_mutex_alloc();}
internal void    rw_mutex_release(RWMutex mutex) {os_rw_mutex_release(mutex);}
internal void    rw_mutex_take(RWMutex mutex, B32 write_mode) {os_rw_mutex_take(mutex, write_mode);}
internal void    rw_mutex_drop(RWMutex mutex, B32 write_mode) {os_rw_mutex_drop(mutex, write_mode);}

//- rjf: condition variables

internal CondVar   cond_var_alloc(void)                                                        {return os_cond_var_alloc();}
internal void      cond_var_release(CondVar cv)                                                {os_cond_var_release(cv);}
internal B32       cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us)                         {return os_cond_var_wait(cv, mutex, endt_us);}
internal B32       cond_var_wait_rw(CondVar cv, RWMutex mutex_rw, B32 write_mode, U64 endt_us) {return os_cond_var_wait_rw(cv, mutex_rw, write_mode, endt_us);}
internal void      cond_var_signal(CondVar cv)                                                 {os_cond_var_signal(cv);}
internal void      cond_var_broadcast(CondVar cv)                                              {os_cond_var_broadcast(cv);}

//- rjf: cross-process semaphores

internal Semaphore semaphore_alloc(U32 initial_count, U32 max_count, String8 name) {return os_semaphore_alloc(initial_count, max_count, name);}
internal void      semaphore_release(Semaphore semaphore)                          {os_semaphore_release(semaphore);}
internal Semaphore semaphore_open(String8 name)                                    {return os_semaphore_open(name);}
internal void      semaphore_close(Semaphore semaphore)                            {os_semaphore_close(semaphore);}
internal B32       semaphore_take(Semaphore semaphore, U64 endt_us)                {return os_semaphore_take(semaphore, endt_us);}
internal void      semaphore_drop(Semaphore semaphore)                             {os_semaphore_drop(semaphore);}

//- rjf: barriers

internal Barrier   barrier_alloc(U64 count)         {return os_barrier_alloc(count);}
internal void      barrier_release(Barrier barrier) {os_barrier_release(barrier);}
internal void      barrier_wait(Barrier barrier)    {os_barrier_wait(barrier);}
