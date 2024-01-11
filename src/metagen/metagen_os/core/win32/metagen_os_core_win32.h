// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_H
#define WIN32_H

////////////////////////////////
//~ NOTE(allen): Negotiate the windows header include order

#if OS_FEATURE_SOCKET
#include <WinSock2.h>
#endif

#include <Windows.h>
#include <Shlobj.h>

#if OS_FEATURE_GRAPHICAL
#include <shellscalingapi.h>
#endif

#if OS_FEATURE_SOCKET
#include <WS2tcpip.h>
#include <Mswsock.h>
#endif

#include <processthreadsapi.h>

////////////////////////////////
//~ NOTE(allen): File Iterator

typedef struct W32_FileIter W32_FileIter;
struct W32_FileIter
{
  HANDLE handle;
  WIN32_FIND_DATAW find_data;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(W32_FileIter), file_iter_memory_size);

////////////////////////////////
//~ NOTE(allen): Threading Entities

typedef enum W32_EntityKind
{
  W32_EntityKind_Null,
  W32_EntityKind_Thread,
  W32_EntityKind_Mutex,
  W32_EntityKind_RWMutex,
  W32_EntityKind_ConditionVariable,
}
W32_EntityKind;

typedef struct W32_Entity W32_Entity;
struct W32_Entity
{
  W32_Entity *next;
  W32_EntityKind kind;
  volatile U32 reference_mask;
  union{
    struct{
      OS_ThreadFunctionType *func;
      void *ptr;
      HANDLE handle;
      DWORD tid;
    } thread;
    CRITICAL_SECTION mutex;
    SRWLOCK rw_mutex;
    CONDITION_VARIABLE cv;
  };
};

////////////////////////////////
//~ rjf: Helpers

//- rjf: files
internal FilePropertyFlags w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes);
internal void w32_file_properties_from_attributes(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes);

//- rjf: time
internal void w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in);
internal void w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in);
internal void w32_dense_time_from_file_time(DenseTime *out, FILETIME *in);
internal U32 w32_sleep_ms_from_endt_us(U64 endt_us);

//- rjf: entities
internal W32_Entity* w32_alloc_entity(W32_EntityKind kind);
internal void w32_free_entity(W32_Entity *entity);

//- rjf: threads
internal DWORD w32_thread_base(void *ptr);

#endif //WIN32_H
