// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_CORE_MAC_H
#define OS_CORE_MAC_H

////////////////////////////////
//~ rjf: Includes

#include <dirent.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

pid_t gettid(void);

extern char **environ;

typedef struct tm tm;
typedef struct timespec timespec;

////////////////////////////////

#define OS_MAC_RETRY_ON_EINTR(expr)          \
  (__extension__({                           \
  __typeof__(expr) __ret;                    \
  do {                                       \
  __ret = (expr);                            \
  } while ((__ret == -1) && errno == EINTR); \
  __ret;                                     \
  }))

////////////////////////////////
//~ rjf: Register Layouts
//
// These are defined in <sys/user.h>, but only for one architecture at a time

typedef struct OS_MAC_GprsX64 OS_MAC_GprsX64;
struct OS_MAC_GprsX64
{
  U64 r15;
  U64 r14;
  U64 r13;
  U64 r12;
  U64 rbp;
  U64 rbx;
  U64 r11;
  U64 r10;
  U64 r9;
  U64 r8;
  U64 rax;
  U64 rcx;
  U64 rdx;
  U64 rsi;
  U64 rdi;
  U64 orig_rax;
  U64 rip;
  U64 cs;
  U64 rflags;
  U64 rsp;
  U64 ss;
  U64 fsbase;
  U64 gsbase;
  U64 ds;
  U64 es;
  U64 fs;
  U64 gs;
};

typedef struct OS_MAC_UserX64 OS_MAC_UserX64;
struct OS_MAC_UserX64
{
  OS_MAC_GprsX64 regs;
  S32 u_fpvalid;
  U32 _pad0;
  X64_FXSave i387;
  U64 u_tsize;
  U64 u_dsize;
  U64 u_ssize;
  U64 start_code;
  U64 start_stack;
  U64 signal;
  U32 reserved;
  U32 _pad1;
  U64 u_ar0;
  U64 u_fpstate;
  U64 magic;
  U8  u_comm[32];
  U64 u_debugreg[8];
};
StaticAssert(sizeof(OS_MAC_UserX64) == 912, g_os_mac_user_x64_size_check);

////////////////////////////////
//~ rjf: File Iterator

typedef struct OS_MAC_FileIter OS_MAC_FileIter;
struct OS_MAC_FileIter
{
  DIR *dir;
  struct dirent *dp;
  String8 path;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(OS_MAC_FileIter), os_mac_file_iter_size_check);

////////////////////////////////
//~ rjf: Safe Call Handler Chain

typedef struct OS_MAC_SafeCallChain OS_MAC_SafeCallChain;
struct OS_MAC_SafeCallChain
{
  OS_MAC_SafeCallChain *next;
  ThreadEntryPointFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ rjf: Entities

typedef enum OS_MAC_EntityKind
{
  OS_MAC_EntityKind_Thread,
  OS_MAC_EntityKind_Mutex,
  OS_MAC_EntityKind_RWMutex,
  OS_MAC_EntityKind_ConditionVariable,
  OS_MAC_EntityKind_Barrier,
}
OS_MAC_EntityKind;

typedef struct OS_MAC_Entity OS_MAC_Entity;
struct OS_MAC_Entity
{
  OS_MAC_Entity *next;
  OS_MAC_EntityKind kind;
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
    struct {
      pthread_mutex_t mutex;
      pthread_cond_t cond;
      unsigned count;
      unsigned left;
      unsigned round;
    } barrier;
  };
};

////////////////////////////////
//~ rjf: State

typedef struct OS_MAC_State OS_MAC_State;
struct OS_MAC_State
{
  Arena *arena;
  OS_SystemInfo system_info;
  OS_ProcessInfo process_info;
  pthread_mutex_t entity_mutex;
  Arena *entity_arena;
  OS_MAC_Entity *entity_free;
  U64 default_env_count;
  char **default_env;
};

////////////////////////////////
//~ rjf: Globals

global OS_MAC_State os_mac_state = {0};
thread_static OS_MAC_SafeCallChain *os_mac_safe_call_chain = 0;

////////////////////////////////
//~ rjf: Helpers

internal DateTime os_mac_date_time_from_tm(tm in, U32 msec);
internal tm os_mac_tm_from_date_time(DateTime dt);
internal timespec os_mac_timespec_from_date_time(DateTime dt);
internal DenseTime os_mac_dense_time_from_timespec(timespec in);
internal FileProperties os_mac_file_properties_from_stat(struct stat *s);
internal void os_mac_safe_call_sig_handler(int x);

////////////////////////////////
//~ rjf: Entities

internal OS_MAC_Entity *os_mac_entity_alloc(OS_MAC_EntityKind kind);
internal void os_mac_entity_release(OS_MAC_Entity *entity);

////////////////////////////////
//~ rjf: Thread Entry Point

internal void *os_mac_thread_entry_point(void *ptr);

#endif // OS_CORE_MAC_H
