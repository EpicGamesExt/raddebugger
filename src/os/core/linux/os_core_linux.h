// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_H
#define LINUX_H

////////////////////////////////
//~ NOTE(allen): Get all these linux includes

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <linux/memfd.h>
#include <linux/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uuid/uuid.h>

//////////////////////////////////
// Helper Typedefs

// -- Time --
// Time broken into major and minor parts
typedef struct timespec LNX_timespec;
typedef struct timeval LNX_timeval;
// Deconstructed Date-Time
typedef struct tm LNX_date;

// -- File --
// File Statistics
typedef S32 LNX_fd;
typedef struct stat LNX_fstat;
// Opaque directory stream of directory contents
typedef DIR LNX_dir;
// Opaque directory entry/file
typedef struct dirent LNX_dir_entry;

// -- Other --
typedef uuid_t LNX_uuid;

// -- Syncronization Primitives--
typedef sem_t LNX_semaphore;
typedef pthread_mutex_t LNX_mutex;
typedef pthread_mutexattr_t LNX_mutex_attr;
typedef pthread_rwlock_t LNX_rwlock;
typedef pthread_rwlockattr_t LNX_rwlock_attr;
typedef pthread_cond_t LNX_cond;

////////////////////////////////
//~ NOTE(allen): Threading Entities
typedef enum  LNX_EntityKind  LNX_EntityKind;
enum LNX_EntityKind{
  LNX_EntityKind_Null,
  LNX_EntityKind_Thread,
  LNX_EntityKind_Mutex,
  LNX_EntityKind_Rwlock,
  LNX_EntityKind_ConditionVariable,
  LNX_EntityKind_Semaphore,
  LNX_EntityKind_MemoryMap,
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
    struct{
      S32 fd;
      U32 flags;                // TODO: Maybe delete
      void* data;
      U64 size;
      String8 shm_name;
    } map;
    LNX_mutex mutex;
    LNX_rwlock rwlock;
    LNX_cond cond;
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

// Helper Structs
typedef struct LNX_version LNX_version;
struct  LNX_version {
  U32 major;
  U32 minor;
  U32 patch;
  String8 string;
};

typedef struct LNX_file_iter LNX_file_iter;
struct LNX_file_iter {
  LNX_dir* dir_stream;
  S32 dir_fd;
};

internal B32 lnx_write_list_to_file_descriptor(int fd, String8List list);
 /* Helper function to return a timespec using a adjtime stable high precision clock
  This should not be affected by setting the system clock or RTC*/
internal LNX_timespec lnx_now_precision_timespec();
// Get the current system time that is affected by setting the system clock
internal LNX_timespec lnx_now_system_timespec();

// Typecast Functions
internal void lnx_date_time_from_tm(DateTime *out, struct tm *in, U32 msec);
internal void lnx_tm_from_date_time(struct tm *out, DateTime *in);
internal void lnx_timespec_from_date_time(LNX_timespec* out, DateTime* in);
internal void lnx_timeval_from_date_time(LNX_timeval* out, DateTime* in);
internal void lnx_dense_time_from_timespec(DenseTime *out, LNX_timespec *in);
internal void lnx_timeval_from_dense_time(LNX_timeval* out, DenseTime* in);
internal void lnx_timespec_from_dense_time(LNX_timespec* out, DenseTime* in);
internal void lnx_timespec_from_timeval(LNX_timespec* out, LNX_timeval* in);
internal void lnx_timeval_from_timespec(LNX_timeval* out, LNX_timespec* in);
internal void lnx_file_properties_from_stat(FileProperties *out, struct stat *in);

// Convert OS_AccessFlags to 'mmap' compatible 'PROT_' flags
internal U32 lnx_prot_from_os_flags(OS_AccessFlags flags);
// Convert OS_AccessFlags to 'open' compatible 'O_' flags
internal U32 lnx_open_from_os_flags(OS_AccessFlags flags);

// Stable and consistent handle conversions
internal U32 lnx_fd_from_handle(OS_Handle file);
internal OS_Handle lnx_handle_from_fd(U32 fd);
internal LNX_Entity* lnx_entity_from_handle(OS_Handle handle, LNX_EntityKind type);
internal OS_Handle lnx_handle_from_entity(LNX_Entity* entity);

internal String8 lnx_string_from_signal(int signum);
internal String8 lnx_string_from_errno(int error_number);

internal LNX_Entity* lnx_alloc_entity(LNX_EntityKind kind);
internal void lnx_free_entity(LNX_Entity *entity);
internal void* lnx_thread_base(void *ptr);

internal void lnx_safe_call_sig_handler(int _);

#endif //LINUX_H
