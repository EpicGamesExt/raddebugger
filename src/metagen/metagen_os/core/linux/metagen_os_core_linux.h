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
#include <linux/mman.h>
#include <linux/memfd.h>
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <semaphore.h>

//////////////////////////////////
 // Helper Typedefs
// Time broken into major and minor parts
typedef struct timespec LNX_timespec;
typedef struct timeval LNX_timeval;
// Deconstructed Date-Time
typedef struct tm lnx_date;
// File Statistics
typedef struct stat lnx_fstat;

// Syncronization Primitives
typedef sem_t LNX_semaphore;
typedef pthread_mutex_t LNX_mutex;
typedef pthread_mutexattr_t LNX_mutex_attr;
typedef pthread_rwlock_t LNX_rwlock;
typedef pthread_rwlockattr_t LNX_rwlock_attr;

////////////////////////////////
//~ NOTE(allen): File Iterator

typedef struct LNX_FileIter LNX_FileIter;
struct LNX_FileIter{
  int fd;
  DIR *dir;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(LNX_FileIter), file_iter_memory_size);

////////////////////////////////
//~ NOTE(allen): Threading Entities
typedef enum  LNX_EntityKind  LNX_EntityKind;
enum LNX_EntityKind{
  LNX_EntityKind_Null,
  LNX_EntityKind_Thread,
  LNX_EntityKind_Mutex,
  LNX_EntityKind_Rwlock,
  LNX_EntityKind_ConditionVariable,
  LNX_EntityKind_Semaphore
};

typedef struct LNX_Entity LNX_Entity;
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
    struct{
      sem_t* handle;
      U32 max_value;
    } semaphore;
    LNX_mutex mutex;
    LNX_rwlock rwlock;
    pthread_cond_t cond;
  };
};

////////////////////////////////
//~ NOTE(allen): Safe Call Chain

typedef struct  LNX_SafeCallChain LNX_SafeCallChain;
struct LNX_SafeCallChain{
  LNX_SafeCallChain *next;
  OS_ThreadFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ NOTE(allen): Helpers

internal B32 lnx_write_list_to_file_descriptor(int fd, String8List list);
// Get high percision CPU clock based timestamp, unaffected by time settings
internal LNX_timespec lnx_now_precision_timespec();
// Get the current system time that is affected by setting the system clock
internal LNX_timespec lnx_now_system_timespec();

// Typecast Functions
internal void lnx_date_time_from_tm(DateTime *out, struct tm *in, U32 msec);
internal void lnx_tm_from_date_time(struct tm *out, DateTime *in);
internal void LNX_timespec_from_date_time(LNX_timespec* out, DateTime* in);
internal void LNX_timeval_from_date_time(LNX_timeval* out, DateTime* in);
internal void lnx_dense_time_from_timespec(DenseTime *out, LNX_timespec *in);
internal void LNX_timeval_from_dense_time(LNX_timeval* out, DenseTime* in);
internal void LNX_timespec_from_dense_time(LNX_timespec* out, DenseTime* in);
void LNX_timespec_from_timeval(LNX_timespec* out, LNX_timeval* in);
void LNX_timeval_from_timespec(LNX_timeval* out, LNX_timespec* in);
internal void lnx_file_properties_from_stat(FileProperties *out, struct stat *in);

// Convert OS_AccessFlags to 'mmap' compatible 'PROT_' flags
internal U32 lnx_mmap_from_os_flags(OS_AccessFlags flags);
// Convert OS_AccessFlags to 'open' compatible 'O_' flags
internal U32 lnx_open_from_os_flags(OS_AccessFlags flags);

// Stable and consistent handle conversions
internal U32 lnx_fd_from_handle(OS_Handle file);
internal OS_Handle lnx_handle_from_fd(U32 fd);
internal LNX_Entity* lnx_entity_from_handle(OS_Handle handle);
internal OS_Handle lnx_handle_from_entity(LNX_Entity* entity);

internal String8 lnx_string_from_signal(int signum);
internal String8 lnx_string_from_errno(int error_number);

internal LNX_Entity* lnx_alloc_entity(LNX_EntityKind kind);
internal void lnx_free_entity(LNX_Entity *entity);
internal void* lnx_thread_base(void *ptr);

internal void lnx_safe_call_sig_handler(int _);

#endif //LINUX_H
