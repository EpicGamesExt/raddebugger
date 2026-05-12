// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_BASE_H
#define WIN32_BASE_H

////////////////////////////////
//~ rjf: Includes / Libraries

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>
#include <tlhelp32.h>
#include <Shlobj.h>
#include <processthreadsapi.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "ole32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "ws2_32")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") // this is required for loading correct comctl32 dll file

////////////////////////////////
//~ rjf: File Iterator Types

typedef struct W32_FileIter W32_FileIter;
struct W32_FileIter
{
  HANDLE handle;
  WIN32_FIND_DATAW find_data;
  B32 is_volume_iter;
  String8Array drive_strings;
  U64 drive_strings_iter_idx;
};
StaticAssert(sizeof(Member(FileIter, memory)) >= sizeof(W32_FileIter), file_iter_memory_size);

////////////////////////////////
//~ rjf: Entity Types

typedef enum W32_EntityKind
{
  W32_EntityKind_Null,
  W32_EntityKind_Thread,
  W32_EntityKind_Mutex,
  W32_EntityKind_RWMutex,
  W32_EntityKind_ConditionVariable,
  W32_EntityKind_Barrier,
}
W32_EntityKind;

typedef struct W32_SYNCHRONIZATION_BARRIER W32_SYNCHRONIZATION_BARRIER;
struct W32_SYNCHRONIZATION_BARRIER
{
  DWORD reserved_0;
  DWORD reserved_1;
#if ARCH_32BIT
  U32 reserved_2[2];
#else
  U64 reserved_2[2];
#endif
  DWORD reserved_3;
  DWORD reserved_4;
};

typedef struct W32_Entity W32_Entity;
struct W32_Entity
{
  W32_Entity *next;
  W32_EntityKind kind;
  union
  {
    struct
    {
      ThreadEntryPointFunctionType *func;
      void *ptr;
      HANDLE handle;
      DWORD tid;
    } thread;
    CRITICAL_SECTION mutex;
    SRWLOCK rw_mutex;
    CONDITION_VARIABLE cv;
    W32_SYNCHRONIZATION_BARRIER sb;
  };
};

////////////////////////////////
//~ rjf: State

typedef struct W32_State W32_State;
struct W32_State
{
  Arena *arena;
  
  // rjf: info
  SystemInfo system_info;
  ProcessInfo process_info;
  U64 microsecond_resolution;
  
  // rjf: entity storage
  CRITICAL_SECTION entity_mutex;
  Arena *entity_arena;
  W32_Entity *entity_free;
};

////////////////////////////////
//~ rjf: Globals

global W32_State w32_state = {0};

////////////////////////////////
//~ rjf: File Info Conversion Helpers

internal FilePropertyFlags w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes);
internal void w32_file_properties_from_attribute_data(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes);

////////////////////////////////
//~ rjf: Time Conversion Helpers

internal void w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in);
internal void w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in);
internal void w32_dense_time_from_file_time(DenseTime *out, FILETIME *in);
internal U32 w32_sleep_ms_from_endt_us(U64 endt_us);

////////////////////////////////
//~ rjf: Entity Functions

internal W32_Entity *w32_entity_alloc(W32_EntityKind kind);
internal void w32_entity_release(W32_Entity *entity);

////////////////////////////////
//~ rjf: Thread Entry Point

internal DWORD w32_thread_entry_point(void *ptr);

#endif // WIN32_BASE_H
