// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_BASE_H
#define LINUX_BASE_H

////////////////////////////////
//~ rjf: Includes

#include <dirent.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <features.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

pid_t gettid(void);
int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t size);

typedef struct tm tm;
typedef struct timespec timespec;

////////////////////////////////
//~ rjf: Linux Call Interruption Retry Helper

#define LNX_RETRY_ON_EINTR(expr)          \
(__extension__({                           \
__typeof__(expr) __ret;                    \
do {                                       \
__ret = (expr);                            \
} while ((__ret == -1) && errno == EINTR); \
__ret;                                     \
}))


////////////////////////////////
//~ rjf: File Iterator

typedef struct LNX_FileIter LNX_FileIter;
struct LNX_FileIter
{
  DIR *dir;
  struct dirent *dp;
  String8 path;
};
StaticAssert(sizeof(Member(FileIter, memory)) >= sizeof(LNX_FileIter), os_lnx_file_iter_size_check);

////////////////////////////////
//~ rjf: Safe Call Handler Chain

typedef struct LNX_SafeCallChain LNX_SafeCallChain;
struct LNX_SafeCallChain
{
  LNX_SafeCallChain *next;
  ThreadEntryPointFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ rjf: Entities

typedef enum LNX_EntityKind
{
  LNX_EntityKind_Thread,
  LNX_EntityKind_Mutex,
  LNX_EntityKind_RWMutex,
  LNX_EntityKind_ConditionVariable,
  LNX_EntityKind_Barrier,
}
LNX_EntityKind;

typedef struct LNX_Entity LNX_Entity;
struct LNX_Entity
{
  LNX_Entity *next;
  LNX_EntityKind kind;
  union
  {
    struct
    {
      pthread_t handle;
      ThreadEntryPointFunctionType *func;
      void *ptr;
    } thread;
    pthread_mutex_t mutex_handle;
    pthread_rwlock_t rwmutex_handle;
    struct
    {
      pthread_cond_t cond_handle;
      pthread_mutex_t rwlock_mutex_handle;
    } cv;
    pthread_barrier_t barrier;
  };
};

////////////////////////////////
//~ rjf: State

typedef struct LNX_State LNX_State;
struct LNX_State
{
  Arena *arena;
  SystemInfo system_info;
  ProcessInfo process_info;
  pthread_mutex_t entity_mutex;
  Arena *entity_arena;
  LNX_Entity *entity_free;
  U64 default_env_count;
  char **default_env;
};

////////////////////////////////
//~ rjf: Globals

global LNX_State os_lnx_state = {0};
thread_static LNX_SafeCallChain *os_lnx_safe_call_chain = 0;

////////////////////////////////
//~ rjf: Helpers

internal DateTime os_lnx_date_time_from_tm(tm in, U32 msec);
internal tm os_lnx_tm_from_date_time(DateTime dt);
internal timespec os_lnx_timespec_from_date_time(DateTime dt);
internal DenseTime os_lnx_dense_time_from_timespec(timespec in);
internal FileProperties os_lnx_file_properties_from_stat(struct stat *s);
internal void os_lnx_safe_call_sig_handler(int x);

////////////////////////////////
//~ rjf: Entities

internal LNX_Entity *os_lnx_entity_alloc(LNX_EntityKind kind);
internal void os_lnx_entity_release(LNX_Entity *entity);

////////////////////////////////
//~ rjf: Thread Entry Point

internal void *os_lnx_thread_entry_point(void *ptr);

#endif // LINUX_BASE_H
