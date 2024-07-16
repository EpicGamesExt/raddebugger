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
//~ rjf: State

typedef struct OS_LNX_State OS_LNX_State;
struct OS_LNX_State
{
  Arena *arena;
  OS_SystemInfo system_info;
  OS_ProcessInfo process_info;
};

////////////////////////////////
//~ rjf: Globals

global OS_LNX_State os_lnx_state = {0};

#endif // OS_CORE_LINUX_H
