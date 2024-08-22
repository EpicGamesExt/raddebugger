// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

////////////////////////////////
//~ rjf: Includes / Libraries

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>
#include <tlhelp32.h>
#include <Shlobj.h>
#include <processthreadsapi.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "shell32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "comctl32")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") // this is required for loading correct comctl32 dll file

////////////////////////////////
//~ rjf: File Iterator Types

typedef struct OS_W32_FileIter OS_W32_FileIter;
struct OS_W32_FileIter
{
  HANDLE handle;
  WIN32_FIND_DATAW find_data;
  B32 is_volume_iter;
  String8Array drive_strings;
  U64 drive_strings_iter_idx;
};
StaticAssert(sizeof(Member(OS_FileIter, memory)) >= sizeof(OS_W32_FileIter), file_iter_memory_size);

////////////////////////////////
//~ rjf: Entity Types

typedef enum OS_W32_EntityKind
{
  OS_W32_EntityKind_Null,
  OS_W32_EntityKind_Thread,
  OS_W32_EntityKind_Mutex,
  OS_W32_EntityKind_RWMutex,
  OS_W32_EntityKind_ConditionVariable,
}
OS_W32_EntityKind;

typedef struct OS_W32_Entity OS_W32_Entity;
struct OS_W32_Entity
{
  OS_W32_Entity *next;
  OS_W32_EntityKind kind;
  union
  {
    struct
    {
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
//~ rjf: State

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State
{
  Arena *arena;
  
  // rjf: info
  OS_SystemInfo system_info;
  OS_ProcessInfo process_info;
  U64 microsecond_resolution;
  
  // rjf: entity storage
  CRITICAL_SECTION entity_mutex;
  Arena *entity_arena;
  OS_W32_Entity *entity_free;
};

////////////////////////////////
//~ rjf: Globals

global OS_W32_State os_w32_state = {0};

////////////////////////////////
//~ rjf: File Info Conversion Helpers

internal FilePropertyFlags os_w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes);
internal void os_w32_file_properties_from_attribute_data(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes);

////////////////////////////////
//~ rjf: Time Conversion Helpers

internal void os_w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in);
internal void os_w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in);
internal void os_w32_dense_time_from_file_time(DenseTime *out, FILETIME *in);
internal U32 os_w32_sleep_ms_from_endt_us(U64 endt_us);

////////////////////////////////
//~ rjf: Entity Functions

internal OS_W32_Entity *os_w32_entity_alloc(OS_W32_EntityKind kind);
internal void os_w32_entity_release(OS_W32_Entity *entity);

////////////////////////////////
//~ rjf: Thread Entry Point

internal DWORD os_w32_thread_entry_point(void *ptr);

#endif // OS_CORE_WIN32_H
