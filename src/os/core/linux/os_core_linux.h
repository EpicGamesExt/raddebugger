// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_CORE_LINUX_H
#define OS_CORE_LINUX_H

////////////////////////////////
//~ dan: Includes

#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <features.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <mntent.h>
#include <paths.h>
#include <stdatomic.h>
#include <linux/futex.h>
#include <linux/stat.h>
#include <setjmp.h>

// Explicit declarations for GNU extensions that might not be exposed
// by headers even when _GNU_SOURCE is defined.
// TODO this feels really stupid and not addressing the root cause of the problem:
extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);
// From <pthread.h>
int pthread_timedjoin_np(pthread_t thread, void **retval, const struct timespec *abstime);
// From <semaphore.h>
// Use _GNU_SOURCE as the guard since sem_clockwait is often tied to it
// and the specific POSIX macros might not be reliable everywhere.
int sem_clockwait(sem_t *sem, clockid_t clock_id, const struct timespec *abstime);

#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH 0x1000
#endif

pid_t gettid(void);
int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t size);

typedef struct tm tm;
typedef struct timespec timespec;

////////////////////////////////
//~ dan: File Iterator

typedef struct OS_LNX_FileIter OS_LNX_FileIter;
struct OS_LNX_FileIter
{
  DIR *dir;
  struct dirent *dp;
  String8 path;
  B32 is_volume_iter;
  FILE *mount_file_stream;
  String8Array volume_mount_points;
  U64 volume_iter_idx;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(OS_LNX_FileIter), os_lnx_file_iter_size_check);

////////////////////////////////
//~ dan: Safe Call Handler Chain

typedef struct OS_LNX_SafeCallChain OS_LNX_SafeCallChain;
struct OS_LNX_SafeCallChain
{
  OS_LNX_SafeCallChain *next;
  OS_ThreadFunctionType *fail_handler;
  void *ptr;
};

////////////////////////////////
//~ dan: Entities

typedef enum OS_LNX_EntityKind
{
  OS_LNX_EntityKind_Thread,
  OS_LNX_EntityKind_Mutex,
  OS_LNX_EntityKind_RWMutex,
  OS_LNX_EntityKind_ConditionVariable,
  OS_LNX_EntityKind_Process,
  OS_LNX_EntityKind_Semaphore,
  OS_LNX_EntityKind_SafeCallChain,
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
    struct // OS_LNX_EntityKind_Mutex
    {
      atomic_uint futex; // 0: unlocked, 1: locked uncontended, 2: locked contended
      atomic_uint owner_tid;
      atomic_uint recursion_depth;
    } mutex;
    struct // OS_LNX_EntityKind_RWMutex
    {
      atomic_uint futex; // Stores counts: lower 16 bits readers, upper 16 bits writers/waiters
                         // Special state for write lock held.
#define RW_MUTEX_READ_MASK       0x0000FFFFu // Lower 16 bits for reader count
#define RW_MUTEX_WRITE_MASK      0xFFFF0000u // Upper 16 bits used for writer/waiter state
#define RW_MUTEX_WRITE_HELD_MASK 0x80000000u // Top bit indicates writer holds the lock
#define RW_MUTEX_WRITE_WAIT_MASK 0x40000000u // Next bit indicates writers are waiting
#define RW_MUTEX_READ_WAIT_MASK  0x20000000u // Next bit indicates readers are waiting
    } rw_mutex;
    struct // OS_LNX_EntityKind_ConditionVariable
    {
      atomic_uint futex; // Sequence number or generation count for wakeups
      // CVs need an associated mutex, handled externally by the API user
    } cv;
    struct
    {
      pid_t pid;
    } process;
    struct
    {
      sem_t *named_handle; // Handle for named semaphores from sem_open
      sem_t unnamed_handle; // Embedded handle for unnamed semaphores
      String8 name; // OS-internal name (e.g., with leading slash prepended) for named semaphores
      B32 is_named;
    } semaphore;
    struct // OS_LNX_EntityKind_SafeCallChain
    {
      OS_ThreadFunctionType *fail_handler;
      void *ptr;
      sigjmp_buf jmp_buf;
      struct sigaction original_actions[6]; // Max signals we handle
      int signals_handled[6];
      int num_signals_handled;
      OS_LNX_Entity *caller_chain_node; // Link to caller's node in the TLS chain
    } safe_call_chain;
  };
};

////////////////////////////////
//~ dan: State

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
//~ dan: Globals

global OS_LNX_State os_lnx_state = {0};
thread_static OS_LNX_Entity *os_lnx_safe_call_chain = 0;

////////////////////////////////
//~ dan: Helpers

internal DateTime os_lnx_date_time_from_tm(tm in, U32 msec);
internal tm os_lnx_tm_from_date_time(DateTime dt);
internal timespec os_lnx_timespec_from_date_time(DateTime dt);
internal DenseTime os_lnx_dense_time_from_timespec(timespec in);
internal DenseTime os_lnx_dense_time_from_statx_timestamp(struct statx_timestamp in);
internal FileProperties os_lnx_file_properties_from_stat(struct stat *s);
internal void os_lnx_safe_call_sig_handler(int sig);

////////////////////////////////
//~ dan: Entities

internal OS_LNX_Entity *os_lnx_entity_alloc(OS_LNX_EntityKind kind);
internal void os_lnx_entity_release(OS_LNX_Entity *entity);

////////////////////////////////
//~ dan: Futex Helpers

internal int os_lnx_futex(atomic_uint *uaddr, int futex_op, unsigned int val, const struct timespec *timeout, unsigned int val3);
internal int os_lnx_futex_wait(atomic_uint *uaddr, unsigned int expected_val, const struct timespec *timeout);
internal int os_lnx_futex_wake(atomic_uint *uaddr, int num_to_wake);

////////////////////////////////
//~ dan: Thread Entry Point

internal void *os_lnx_thread_entry_point(void *ptr);

#endif // OS_CORE_LINUX_H
