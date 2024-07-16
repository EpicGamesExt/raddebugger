// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_CORE_LINUX_H
#define OS_CORE_LINUX_H

////////////////////////////////
//~ rjf: Includes

#define _GNU_SOURCE
#include <features.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <sys/random.h>

int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t size);

typedef struct tm tm;
typedef struct timespec timespec;

////////////////////////////////
//~ rjf: File Iterator

typedef struct OS_LNX_FileIter OS_LNX_FileIter;
struct OS_LNX_FileIter
{
  DIR *dir;
  struct dirent *dp;
  String8 path;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(OS_LNX_FileIter), os_lnx_file_iter_size_check);

////////////////////////////////
//~ rjf: Safe Call Handler Chain

typedef struct OS_LNX_SafeCallChain OS_LNX_SafeCallChain;
struct OS_LNX_SafeCallChain
{
  OS_LNX_SafeCallChain *next;
  OS_ThreadFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ rjf: Entities

typedef enum OS_LNX_EntityKind
{
  OS_LNX_EntityKind_Thread,
  OS_LNX_EntityKind_Mutex,
  OS_LNX_EntityKind_RWMutex,
  OS_LNX_EntityKind_ConditionVariable,
}
OS_LNX_EntityKind;

typedef struct OS_LNX_Entity OS_LNX_Entity;
struct OS_LNX_Entity
{
  OS_LNX_Entity *next;
  OS_LNX_EntityKind kind;
  union
  {
    struct
    {
      pthread_t handle;
      OS_ThreadFunctionType *func;
      void *ptr;
    } thread;
    pthread_mutex_t mutex_handle;
    pthread_rwlock_t rwmutex_handle;
    struct
    {
      pthread_cond_t cond_handle;
      pthread_mutex_t rwlock_mutex_handle;
    } cv;
  };
};

////////////////////////////////
//~ rjf: State

typedef struct OS_LNX_State OS_LNX_State;
struct OS_LNX_State
{
  Arena *arena;
  OS_SystemInfo system_info;
  OS_ProcessInfo process_info;
  pthread_mutex_t entity_mutex;
  Arena *entity_arena;
  OS_LNX_Entity *entity_free;
};

////////////////////////////////
//~ rjf: Globals

global OS_LNX_State os_lnx_state = {0};
thread_static OS_LNX_SafeCallChain *os_lnx_safe_call_chain = 0;

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

internal OS_LNX_Entity *os_lnx_entity_alloc(OS_LNX_EntityKind kind);
internal void os_lnx_entity_release(OS_LNX_Entity *entity);

////////////////////////////////
//~ rjf: Thread Entry Point

internal void *os_lnx_thread_entry_point(void *ptr);

#endif // OS_CORE_LINUX_H
