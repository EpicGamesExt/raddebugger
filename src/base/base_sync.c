// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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
internal void    rw_mutex_take_r(RWMutex mutex)  {os_rw_mutex_take_r(mutex);}
internal void    rw_mutex_drop_r(RWMutex mutex)  {os_rw_mutex_drop_r(mutex);}
internal void    rw_mutex_take_w(RWMutex mutex)  {os_rw_mutex_take_w(mutex);}
internal void    rw_mutex_drop_w(RWMutex mutex)  {os_rw_mutex_drop_w(mutex);}

//- rjf: condition variables

internal CondVar   cond_var_alloc(void)                                          {return os_cond_var_alloc();}
internal void      cond_var_release(CondVar cv)                                  {os_cond_var_release(cv);}
internal B32       cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us)           {return os_cond_var_wait(cv, mutex, endt_us);}
internal B32       cond_var_wait_rw_r(CondVar cv, RWMutex mutex_rw, U64 endt_us) {return os_cond_var_wait_rw_r(cv, mutex_rw, endt_us);}
internal B32       cond_var_wait_rw_w(CondVar cv, RWMutex mutex_rw, U64 endt_us) {return os_cond_var_wait_rw_w(cv, mutex_rw, endt_us);}
internal void      cond_var_signal(CondVar cv)                                   {os_cond_var_signal(cv);}
internal void      cond_var_broadcast(CondVar cv)                                {os_cond_var_broadcast(cv);}

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
