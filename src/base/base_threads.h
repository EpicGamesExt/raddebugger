// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_THREADS_H
#define BASE_THREADS_H

////////////////////////////////
//~ rjf: Thread Types

typedef struct Thread Thread;
struct Thread
{
  U64 u64[1];
};
typedef void ThreadEntryPointFunctionType(void *p);

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
//~ rjf: Table Stripes

typedef struct Stripe Stripe;
struct Stripe
{
  Arena *arena;
  RWMutex rw_mutex;
  CondVar cv;
  void *free;
};

typedef struct StripeArray StripeArray;
struct StripeArray
{
  Stripe *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Thread Functions

internal Thread thread_launch(ThreadEntryPointFunctionType *f, void *p);
internal B32 thread_join(Thread thread, U64 endt_us);
internal void thread_detach(Thread thread);

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
internal void    rw_mutex_take(RWMutex mutex, B32 write_mode);
internal void    rw_mutex_drop(RWMutex mutex, B32 write_mode);
#define rw_mutex_take_r(m) rw_mutex_take((m), (0))
#define rw_mutex_take_w(m) rw_mutex_take((m), (1))
#define rw_mutex_drop_r(m) rw_mutex_drop((m), (0))
#define rw_mutex_drop_w(m) rw_mutex_drop((m), (1))

//- rjf: condition variables
internal CondVar   cond_var_alloc(void);
internal void      cond_var_release(CondVar cv);
// returns false on timeout, true on signal, (max_wait_ms = max_U64) -> no timeout
internal B32       cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us);
internal B32       cond_var_wait_rw(CondVar cv, RWMutex mutex_rw, B32 write_mode, U64 endt_us);
#define cond_var_wait_rw_r(cv, m, endt) cond_var_wait_rw((cv), (m), (0), (endt))
#define cond_var_wait_rw_w(cv, m, endt) cond_var_wait_rw((cv), (m), (1), (endt))
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
#define RWMutexScope(mutex, write_mode) DeferLoop(rw_mutex_take((mutex), (write_mode)), rw_mutex_drop((mutex), (write_mode)))
#define MutexScopeR(mutex) DeferLoop(rw_mutex_take_r(mutex), rw_mutex_drop_r(mutex))
#define MutexScopeW(mutex) DeferLoop(rw_mutex_take_w(mutex), rw_mutex_drop_w(mutex))
#define MutexScopeRWPromote(mutex) DeferLoop((rw_mutex_drop_r(mutex), rw_mutex_take_w(mutex)), (rw_mutex_drop_w(mutex), rw_mutex_take_r(mutex)))

////////////////////////////////
//~ rjf: Table Stripe Functions

internal StripeArray stripe_array_alloc(Arena *arena);
internal void stripe_array_release(StripeArray *stripes);
internal Stripe *stripe_from_slot_idx(StripeArray *stripes, U64 slot_idx);

#endif // BASE_THREADS_H
