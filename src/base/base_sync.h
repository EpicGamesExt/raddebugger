// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_SYNC_H
#define BASE_SYNC_H

////////////////////////////////
//~ rjf: Synchronization Primitive Types

typedef struct Mutex Mutex;
struct Mutex
{
  U64 u64[1];
};

typedef struct RWMutex RWMutex;
struct RWMutex
{
  U64 u64[1];
};

typedef struct CondVar CondVar;
struct CondVar
{
  U64 u64[1];
};

typedef struct Semaphore Semaphore;
struct Semaphore
{
  U64 u64[1];
};

typedef struct Barrier Barrier;
struct Barrier
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Synchronization Primitive Functions

//- rjf: recursive mutexes
internal Mutex mutex_alloc(void);
internal void  mutex_release(Mutex mutex);
internal void  mutex_take(Mutex mutex);
internal void  mutex_drop(Mutex mutex);

//- rjf: reader/writer mutexes
internal RWMutex rw_mutex_alloc(void);
internal void    rw_mutex_release(RWMutex mutex);
internal void    rw_mutex_take_r(RWMutex mutex);
internal void    rw_mutex_drop_r(RWMutex mutex);
internal void    rw_mutex_take_w(RWMutex mutex);
internal void    rw_mutex_drop_w(RWMutex mutex);

//- rjf: condition variables
internal CondVar   cond_var_alloc(void);
internal void      cond_var_release(CondVar cv);
// returns false on timeout, true on signal, (max_wait_ms = max_U64) -> no timeout
internal B32       cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us);
internal B32       cond_var_wait_rw_r(CondVar cv, RWMutex mutex_rw, U64 endt_us);
internal B32       cond_var_wait_rw_w(CondVar cv, RWMutex mutex_rw, U64 endt_us);
internal void      cond_var_signal(CondVar cv);
internal void      cond_var_broadcast(CondVar cv);

//- rjf: cross-process semaphores
internal Semaphore semaphore_alloc(U32 initial_count, U32 max_count, String8 name);
internal void      semaphore_release(Semaphore semaphore);
internal Semaphore semaphore_open(String8 name);
internal void      semaphore_close(Semaphore semaphore);
internal B32       semaphore_take(Semaphore semaphore, U64 endt_us);
internal void      semaphore_drop(Semaphore semaphore);

//- rjf: barriers
internal Barrier   barrier_alloc(U64 count);
internal void      barrier_release(Barrier barrier);
internal void      barrier_wait(Barrier barrier);

//- rjf: scope macros
#define MutexScope(mutex) DeferLoop(mutex_take(mutex), mutex_drop(mutex))
#define MutexScopeR(mutex) DeferLoop(rw_mutex_take_r(mutex), rw_mutex_drop_r(mutex))
#define MutexScopeW(mutex) DeferLoop(rw_mutex_take_w(mutex), rw_mutex_drop_w(mutex))
#define MutexScopeRWPromote(mutex) DeferLoop((rw_mutex_drop_r(mutex), rw_mutex_take_w(mutex)), (rw_mutex_drop_w(mutex), rw_mutex_take_r(mutex)))

#endif // BASE_SYNC_H
