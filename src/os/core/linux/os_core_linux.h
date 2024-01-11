// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_H
#define LINUX_H

////////////////////////////////
//~ NOTE(allen): Get all these linux includes

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

////////////////////////////////
//~ NOTE(allen): File Iterator

struct LNX_FileIter{
  int fd;
  DIR *dir;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(LNX_FileIter), file_iter_memory_size);

////////////////////////////////
//~ NOTE(allen): Threading Entities

enum LNX_EntityKind{
  LNX_EntityKind_Null,
  LNX_EntityKind_Thread,
  LNX_EntityKind_Mutex,
  LNX_EntityKind_ConditionVariable,
};

struct LNX_Entity{
  LNX_Entity *next;
  LNX_EntityKind kind;
  volatile U32 reference_mask;
  union{
    struct{
      OS_ThreadFunctionType *func;
      void *ptr;
      pthread_t handle;
    } thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
  };
};

////////////////////////////////
//~ NOTE(allen): Safe Call Chain

struct LNX_SafeCallChain{
  LNX_SafeCallChain *next;
  OS_ThreadFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ NOTE(allen): Helpers

internal B32 lnx_write_list_to_file_descriptor(int fd, String8List list);

internal void lnx_date_time_from_tm(DateTime *out, struct tm *in, U32 msec);
internal void lnx_tm_from_date_time(struct tm *out, DateTime *in);
internal void lnx_dense_time_from_timespec(DenseTime *out, struct timespec *in);
internal void lnx_file_properties_from_stat(FileProperties *out, struct stat *in);

internal String8 lnx_string_from_signal(int signum);
internal String8 lnx_string_from_errno(int error_number);

internal LNX_Entity* lnx_alloc_entity(LNX_EntityKind kind);
internal void lnx_free_entity(LNX_Entity *entity);
internal void* lnx_thread_base(void *ptr);

internal void lnx_safe_call_sig_handler(int);

#endif //LINUX_H
