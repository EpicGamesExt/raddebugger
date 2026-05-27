// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Modern Windows SDK Functions
//
// (We must dynamically link to them, since they can be missing in older SDKs)

typedef HRESULT W32_SetThreadDescription_Type(HANDLE hThread, PCWSTR lpThreadDescription);
global W32_SetThreadDescription_Type *w32_SetThreadDescription_func = 0;
typedef BOOL W32_InitializeSynchronizationBarrier_Type(W32_SYNCHRONIZATION_BARRIER *lpBarrier, LONG lTotalThreads, LONG lSpinCount);
global W32_InitializeSynchronizationBarrier_Type *w32_InitializeSynchronizationBarrier_func = 0;
typedef BOOL W32_DeleteSynchronizationBarrier_Type(W32_SYNCHRONIZATION_BARRIER *lpBarrier);
global W32_DeleteSynchronizationBarrier_Type *w32_DeleteSynchronizationBarrier_func = 0;
typedef BOOL W32_EnterSynchronizationBarrier_Type(W32_SYNCHRONIZATION_BARRIER *lpBarrier, DWORD dwFlags);
global W32_EnterSynchronizationBarrier_Type *w32_EnterSynchronizationBarrier_func = 0;
global RIO_EXTENSION_FUNCTION_TABLE w32_rio_functions = {0};

////////////////////////////////
//~ rjf: File Info Conversion Helpers

internal FilePropertyFlags
w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes)
{
  FilePropertyFlags flags = 0;
  if(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    flags |= FilePropertyFlag_IsFolder;
  }
  return flags;
}

internal void
w32_file_properties_from_attribute_data(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes)
{
  properties->size = Compose64Bit(attributes->nFileSizeHigh, attributes->nFileSizeLow);
  w32_dense_time_from_file_time(&properties->created, &attributes->ftCreationTime);
  w32_dense_time_from_file_time(&properties->modified, &attributes->ftLastWriteTime);
  properties->flags = w32_file_property_flags_from_dwFileAttributes(attributes->dwFileAttributes);
}

////////////////////////////////
//~ rjf: Time Conversion Helpers

internal void
w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in)
{
  out->year    = in->wYear;
  out->mon     = in->wMonth - 1;
  out->wday    = in->wDayOfWeek;
  out->day     = in->wDay;
  out->hour    = in->wHour;
  out->min     = in->wMinute;
  out->sec     = in->wSecond;
  out->msec    = in->wMilliseconds;
}

internal void
w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in)
{
  out->wYear         = (WORD)(in->year);
  out->wMonth        = in->mon + 1;
  out->wDay          = in->day;
  out->wHour         = in->hour;
  out->wMinute       = in->min;
  out->wSecond       = in->sec;
  out->wMilliseconds = in->msec;
}

internal void
w32_dense_time_from_file_time(DenseTime *out, FILETIME *in)
{
  SYSTEMTIME systime = {0};
  FileTimeToSystemTime(in, &systime);
  DateTime date_time = {0};
  w32_date_time_from_system_time(&date_time, &systime);
  *out = dense_time_from_date_time(date_time);
}

internal U32
w32_sleep_ms_from_endt_us(U64 endt_us)
{
  U32 sleep_ms = 0;
  if(endt_us == max_U64)
  {
    sleep_ms = INFINITE;
  }
  else
  {
    U64 begint = now_time_us();
    if(begint < endt_us)
    {
      U64 sleep_us = endt_us - begint;
      sleep_ms = (U32)((sleep_us + 999)/1000);
    }
  }
  return sleep_ms;
}

internal U32
w32_unix_time_from_file_time(FILETIME file_time)
{
  U64 win32_time = ((U64)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
  U64 unix_time64 = ((win32_time - 0x19DB1DED53E8000ULL) / 10000000);
  
  Assert(unix_time64 <= max_U32);
  U32 unix_time32 = (U32)unix_time64;
  
  return unix_time32;
}

////////////////////////////////
//~ rjf: Entity Functions

internal W32_Entity *
w32_entity_alloc(W32_EntityKind kind)
{
  W32_Entity *result = 0;
  EnterCriticalSection(&w32_state.entity_mutex);
  {
    result = w32_state.entity_free;
    if(result)
    {
      SLLStackPop(w32_state.entity_free);
    }
    else
    {
      result = push_array_no_zero(w32_state.entity_arena, W32_Entity, 1);
    }
    MemoryZeroStruct(result);
  }
  LeaveCriticalSection(&w32_state.entity_mutex);
  result->kind = kind;
  return result;
}

internal void
w32_entity_release(W32_Entity *entity)
{
  entity->kind = W32_EntityKind_Null;
  EnterCriticalSection(&w32_state.entity_mutex);
  SLLStackPush(w32_state.entity_free, entity);
  LeaveCriticalSection(&w32_state.entity_mutex);
}

////////////////////////////////
//~ rjf: Thread Entry Point

internal DWORD
w32_thread_entry_point(void *ptr)
{
  W32_Entity *entity = (W32_Entity *)ptr;
  ThreadEntryPointFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  supplement_thread_base_entry_point(func, thread_ptr);
  return 0;
}

////////////////////////////////
//~ rjf: @per_os_impl Debugger Attachment Checking

internal B32
debugger_is_attached(void)
{
  B32 result = IsDebuggerPresent();
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Platform Time Functions

internal U64
now_time_us(void)
{
  U64 result = 0;
  LARGE_INTEGER large_int_counter;
  if(QueryPerformanceCounter(&large_int_counter))
  {
    result = (large_int_counter.QuadPart*Million(1))/w32_state.microsecond_resolution;
  }
  return result;
}

internal U32
now_time_unix(void)
{
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  U32 unix_time = w32_unix_time_from_file_time(file_time);
  return unix_time;
}

internal DateTime
now_time_universal(void)
{
  SYSTEMTIME systime = {0};
  GetSystemTime(&systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal DateTime
universal_from_local_time(DateTime *date_time)
{
  SYSTEMTIME systime = {0};
  w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  LocalFileTimeToFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal DateTime
local_from_universal_time(DateTime *date_time)
{
  SYSTEMTIME systime = {0};
  w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  FileTimeToLocalFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal void
sleep_ms(U32 msec)
{
  Sleep(msec);
}

////////////////////////////////
//~ rjf: @per_os_impl Platform GUID Functions

internal Guid
make_guid(void)
{
  Guid result;
  MemoryZeroStruct(&result);
  UUID uuid;
  RPC_STATUS rpc_status = UuidCreate(&uuid);
  if(rpc_status == RPC_S_OK)
  {
    result.data1 = uuid.Data1;
    result.data2 = uuid.Data2;
    result.data3 = uuid.Data3;
    MemoryCopyArray(result.data4, uuid.Data4);
  }
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Platform Memory Allocation

//- rjf: basic

internal void *
reserve_memory(U64 size)
{
  void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
  return result;
}

internal B32
commit_memory(void *ptr, U64 size)
{
  B32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
  if(w32_rio_functions.RIORegisterBuffer)
  {
    // wine does not implement these functions
    w32_rio_functions.RIODeregisterBuffer(w32_rio_functions.RIORegisterBuffer(ptr, size));
  }
#if PROFILE_TELEMETRY
  tmAlloc(0, ptr, size / 1024, "Win32 Commit");
#endif
  return result;
}

internal void
decommit_memory(void *ptr, U64 size)
{
  VirtualFree(ptr, size, MEM_DECOMMIT);
#if PROFILE_TELEMETRY
  tmFree(0, ptr);
#endif
}

internal void
release_memory(void *ptr, U64 size)
{
  // NOTE(rjf): size not used - not necessary on Windows, but necessary for other OSes.
  VirtualFree(ptr, 0, MEM_RELEASE);
}

//- rjf: large pages

internal void *
reserve_memory_large(U64 size)
{
  // we commit on reserve because windows
  void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
  return result;
}

internal B32
commit_memory_large(void *ptr, U64 size)
{
  return 1;
}

////////////////////////////////
//~ rjf: @os_hooks Shared Memory

internal SharedMemory
shared_memory_alloc(U64 size, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE file = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                   0,
                                   PAGE_READWRITE,
                                   (U32)((size & 0xffffffff00000000) >> 32),
                                   (U32)((size & 0x00000000ffffffff)),
                                   (WCHAR *)name16.str);
  SharedMemory result = {(U64)file};
  scratch_end(scratch);
  return result;
}

internal SharedMemory
shared_memory_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE file = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, (WCHAR *)name16.str);
  SharedMemory result = {(U64)file};
  scratch_end(scratch);
  return result;
}

internal void
shared_memory_close(SharedMemory handle)
{
  HANDLE file = (HANDLE)(handle.u64[0]);
  CloseHandle(file);
}

internal void *
shared_memory_view_open(SharedMemory handle, Rng1U64 range)
{
  HANDLE file = (HANDLE)(handle.u64[0]);
  U64 offset = range.min;
  U64 size = range.max-range.min;
  void *ptr = MapViewOfFile(file, FILE_MAP_ALL_ACCESS,
                            (U32)((offset & 0xffffffff00000000) >> 32),
                            (U32)((offset & 0x00000000ffffffff)),
                            size);
  return ptr;
}

internal void
shared_memory_view_close(SharedMemory handle, void *ptr, Rng1U64 range)
{
  UnmapViewOfFile(ptr);
}

////////////////////////////////
//~ rjf: @per_os_impl System Info

internal SystemInfo *
get_system_info(void)
{
  return &w32_state.system_info;
}

////////////////////////////////
//~ rjf: @per_os_impl Current Thread Info

internal U32
tid(void)
{
  DWORD id = GetCurrentThreadId();
  return (U32)id;
}

internal void
set_platform_thread_name(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: windows 10 style
  if(w32_SetThreadDescription_func)
  {
    String16 name16 = str16_from_8(scratch.arena, name);
    HRESULT hr = w32_SetThreadDescription_func(GetCurrentThread(), (WCHAR*)name16.str);
  }
  
  // rjf: raise-exception style
  {
    String8 name_copy = push_str8_copy(scratch.arena, name);
#pragma pack(push,8)
    typedef struct THREADNAME_INFO THREADNAME_INFO;
    struct THREADNAME_INFO
    {
      U32 dwType;     // Must be 0x1000.
      char *szName;   // Pointer to name (in user addr space).
      U32 dwThreadID; // Thread ID (-1=caller thread).
      U32 dwFlags;    // Reserved for future use, must be zero.
    };
#pragma pack(pop)
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = (char *)name_copy.str;
    info.dwThreadID = tid();
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
      RaiseException(0x406D1388, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
  }
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @per_os_impl Thread Functions

internal Thread
thread_launch(ThreadEntryPointFunctionType *f, void *p)
{
  W32_Entity *entity = w32_entity_alloc(W32_EntityKind_Thread);
  entity->thread.func = f;
  entity->thread.ptr = p;
  entity->thread.handle = CreateThread(0, 0, w32_thread_entry_point, entity, 0, &entity->thread.tid);
  Thread result = {IntFromPtr(entity)};
  return result;
}

internal B32
thread_join(Thread thread, U64 endt_us)
{
  DWORD sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  W32_Entity *entity = (W32_Entity *)PtrFromInt(thread.u64[0]);
  DWORD wait_result = WAIT_OBJECT_0;
  if(entity != 0)
  {
    wait_result = WaitForSingleObject(entity->thread.handle, sleep_ms);
    CloseHandle(entity->thread.handle);
    w32_entity_release(entity);
  }
  return (wait_result == WAIT_OBJECT_0);
}

internal void
thread_detach(Thread thread)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(thread.u64[0]);
  if(entity != 0)
  {
    CloseHandle(entity->thread.handle);
    w32_entity_release(entity);
  }
}

////////////////////////////////
//~ rjf: @os_hooks Safe Calls

internal void
safe_call(ThreadEntryPointFunctionType *func, ThreadEntryPointFunctionType *fail_handler, void *ptr)
{
  __try
  {
    func(ptr);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    if(fail_handler != 0)
    {
      fail_handler(ptr);
    }
  }
}

////////////////////////////////
//~ rjf: @per_os_impl Synchronization Primitive Functions

//- rjf: recursive mutexes

internal Mutex
mutex_alloc(void)
{
  W32_Entity *entity = w32_entity_alloc(W32_EntityKind_Mutex);
  InitializeCriticalSection(&entity->mutex);
  Mutex result = {IntFromPtr(entity)};
  return result;
}

internal void
mutex_release(Mutex mutex)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  DeleteCriticalSection(&entity->mutex);
  w32_entity_release(entity);
}

internal void
mutex_take(Mutex mutex)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  EnterCriticalSection(&entity->mutex);
}

internal void
mutex_drop(Mutex mutex)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  LeaveCriticalSection(&entity->mutex);
}

//- rjf: reader/writer mutexes

internal RWMutex
rw_mutex_alloc(void)
{
  W32_Entity *entity = w32_entity_alloc(W32_EntityKind_RWMutex);
  InitializeSRWLock(&entity->rw_mutex);
  RWMutex result = {IntFromPtr(entity)};
  return result;
}

internal void
rw_mutex_release(RWMutex rw_mutex)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  w32_entity_release(entity);
}

internal void
rw_mutex_take(RWMutex rw_mutex, B32 write_mode)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  if(write_mode)
  {
    AcquireSRWLockExclusive(&entity->rw_mutex);
  }
  else
  {
    AcquireSRWLockShared(&entity->rw_mutex);
  }
}

internal void
rw_mutex_drop(RWMutex rw_mutex, B32 write_mode)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  if(write_mode)
  {
    ReleaseSRWLockExclusive(&entity->rw_mutex);
  }
  else
  {
    ReleaseSRWLockShared(&entity->rw_mutex);
  }
}

//- rjf: condition variables

internal CondVar
cond_var_alloc(void)
{
  W32_Entity *entity = w32_entity_alloc(W32_EntityKind_ConditionVariable);
  InitializeConditionVariable(&entity->cv);
  CondVar result = {IntFromPtr(entity)};
  return result;
}

internal void
cond_var_release(CondVar cv)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  w32_entity_release(entity);
}

internal B32
cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us)
{
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if(sleep_ms > 0)
  {
    W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
    W32_Entity *mutex_entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
    result = SleepConditionVariableCS(&entity->cv, &mutex_entity->mutex, sleep_ms);
  }
  return result;
}

internal B32
cond_var_wait_rw(CondVar cv, RWMutex mutex_rw, B32 write_mode, U64 endt_us)
{
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if(sleep_ms > 0)
  {
    W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
    W32_Entity *mutex_entity = (W32_Entity*)PtrFromInt(mutex_rw.u64[0]);
    result = SleepConditionVariableSRW(&entity->cv, &mutex_entity->rw_mutex, sleep_ms,
                                       write_mode ? 0 : CONDITION_VARIABLE_LOCKMODE_SHARED);
  }
  return result;
}

internal void
cond_var_signal(CondVar cv)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  WakeConditionVariable(&entity->cv);
}

internal void
cond_var_broadcast(CondVar cv)
{
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  WakeAllConditionVariable(&entity->cv);
}

//- rjf: cross-process semaphores

internal Semaphore
semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE handle = CreateSemaphoreW(0, initial_count, max_count, (WCHAR *)name16.str);
  Semaphore result = {(U64)handle};
  scratch_end(scratch);
  return result;
}

internal void
semaphore_release(Semaphore semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  CloseHandle(handle);
}

internal Semaphore
semaphore_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE handle = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, 0, (WCHAR *)name16.str);
  Semaphore result = {(U64)handle};
  scratch_end(scratch);
  return result;
}

internal void
semaphore_close(Semaphore semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  CloseHandle(handle);
}

internal B32
semaphore_take(Semaphore semaphore, U64 endt_us)
{
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  HANDLE handle = (HANDLE)semaphore.u64[0];
  DWORD wait_result = WaitForSingleObject(handle, sleep_ms);
  B32 result = (wait_result == WAIT_OBJECT_0);
  return result;
}

internal void
semaphore_drop(Semaphore semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  ReleaseSemaphore(handle, 1, 0);
}

//- rjf: barriers

internal Barrier
barrier_alloc(U64 count)
{
  Barrier result = {0};
  if(w32_InitializeSynchronizationBarrier_func != 0)
  {
    W32_Entity *entity = w32_entity_alloc(W32_EntityKind_Barrier);
    if(entity != 0)
    {
      BOOL init_good = w32_InitializeSynchronizationBarrier_func(&entity->sb, count, -1);
      (void)init_good;
    }
    result.u64[0] = IntFromPtr(entity);
  }
  else
  {
    result = slow_barrier_alloc(count);
  }
  return result;
}

internal void
barrier_release(Barrier barrier)
{
  if(w32_InitializeSynchronizationBarrier_func != 0)
  {
    W32_Entity *entity = (W32_Entity*)PtrFromInt(barrier.u64[0]);
    if(entity != 0)
    {
      w32_DeleteSynchronizationBarrier_func(&entity->sb);
      w32_entity_release(entity);
    }
  }
  else
  {
    slow_barrier_release(barrier);
  }
}

internal void
barrier_wait(Barrier barrier)
{
  if(w32_InitializeSynchronizationBarrier_func != 0)
  {
    W32_Entity *entity = (W32_Entity*)PtrFromInt(barrier.u64[0]);
    if(entity != 0)
    {
      w32_EnterSynchronizationBarrier_func(&entity->sb, 0);
    }
  }
  else
  {
    slow_barrier_wait(barrier);
  }
}

////////////////////////////////
//~ rjf: @per_os_impl File System (Implemented Per-OS)

//- rjf: files

internal File
file_open(AccessFlags flags, String8 path)
{
  File result = {0};
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  DWORD access_flags = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;
  SECURITY_ATTRIBUTES security_attributes = {sizeof(security_attributes), 0, 0};
  if(flags & AccessFlag_Read)        {access_flags |= GENERIC_READ;}
  if(flags & AccessFlag_Write)       {access_flags |= GENERIC_WRITE;}
  if(flags & AccessFlag_Execute)     {access_flags |= GENERIC_EXECUTE;}
  if(flags & AccessFlag_ShareRead)   {share_mode |= FILE_SHARE_READ;}
  if(flags & AccessFlag_ShareWrite)  {share_mode |= FILE_SHARE_WRITE|FILE_SHARE_DELETE;}
  if(flags & AccessFlag_Write)       {creation_disposition = CREATE_ALWAYS;}
  if(flags & AccessFlag_Append)      {creation_disposition = OPEN_ALWAYS; access_flags |= FILE_APPEND_DATA; }
  if(flags & AccessFlag_Inherited)
  {
    security_attributes.bInheritHandle = 1;
  }
  HANDLE file = CreateFileW((WCHAR *)path16.str, access_flags, share_mode, &security_attributes, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if(file != INVALID_HANDLE_VALUE)
  {
    result.u64[0] = (U64)file;
  }
  else
  {
    DWORD err = GetLastError();
    (void)err;
  }
  scratch_end(scratch);
  return result;
}

internal void
file_close(File file)
{
  if(file_match(file, file_zero())) { return; }
  HANDLE handle = (HANDLE)file.u64[0];
  BOOL result = CloseHandle(handle);
  (void)result;
}

internal U64
file_read(File file, Rng1U64 rng, void *out_data)
{
  if(file_match(file, file_zero())) { return 0; }
  
  HANDLE  handle = (HANDLE)file.u64[0];
  U8     *ptr    = out_data;
  U64     off    = rng.min;
  while(off != rng.max)
  {
    U64        amt64      = rng.max - off;
    U32        amt32      = (U32)Min(MB(32), amt64);
    DWORD      read_size  = 0;
    OVERLAPPED overlapped = { .Offset = (U32)off, .OffsetHigh = (U32)(off >> 32) };
    if( ! ReadFile(handle, ptr, amt32, &read_size, &overlapped))
    {
      break;
    }
    ptr += read_size;
    off += read_size;
  }
  
  U64 total_read_size = off - rng.min;
  return total_read_size;
}

internal U64
file_write(File file, Rng1U64 rng, void *data)
{
  if(file_match(file, file_zero())) { return 0; }
  HANDLE win_handle = (HANDLE)file.u64[0];
  U64 src_off = 0;
  U64 dst_off = rng.min;
  U64 total_write_size = dim_1u64(rng);
  for(;;)
  {
    void *bytes_src = (U8 *)data + src_off;
    U64 bytes_left = total_write_size - src_off;
    DWORD write_size = Min(MB(1), bytes_left);
    DWORD bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = (dst_off&0x00000000ffffffffull);
    overlapped.OffsetHigh = (dst_off&0xffffffff00000000ull) >> 32;
    BOOL success = WriteFile(win_handle, bytes_src, write_size, &bytes_written, &overlapped);
    if(success == 0)
    {
      break;
    }
    src_off += bytes_written;
    dst_off += bytes_written;
    if(bytes_left == 0)
    {
      break;
    }
  }
  return src_off;
}

internal B32
file_set_time(File file, DateTime time)
{
  if(file_match(file, file_zero())) { return 0; }
  B32 result = 0;
  HANDLE handle = (HANDLE)file.u64[0];
  SYSTEMTIME system_time = {0};
  w32_system_time_from_date_time(&system_time, &time);
  FILETIME file_time = {0};
  result = (SystemTimeToFileTime(&system_time, &file_time) &&
            SetFileTime(handle, &file_time, &file_time, &file_time));
  return result;
}

internal FileProperties
properties_from_file(File file)
{
  if(file_match(file, file_zero())) { FileProperties r = {0}; return r; }
  FileProperties props = {0};
  HANDLE handle = (HANDLE)file.u64[0];
  BY_HANDLE_FILE_INFORMATION info;
  BOOL info_good = GetFileInformationByHandle(handle, &info);
  if(info_good)
  {
    U32 size_lo = info.nFileSizeLow;
    U32 size_hi = info.nFileSizeHigh;
    props.size     = (U64)size_lo | (((U64)size_hi)<<32);
    w32_dense_time_from_file_time(&props.modified, &info.ftLastWriteTime);
    w32_dense_time_from_file_time(&props.created, &info.ftCreationTime);
    props.flags = w32_file_property_flags_from_dwFileAttributes(info.dwFileAttributes);
  }
  return props;
}

internal FileID
id_from_file(File file)
{
  if(file_match(file, file_zero())) { FileID r = {0}; return r; }
  FileID result = {0};
  HANDLE handle = (HANDLE)file.u64[0];
  BY_HANDLE_FILE_INFORMATION info;
  BOOL is_ok = GetFileInformationByHandle(handle, &info);
  if(is_ok)
  {
    result.v[0] = info.dwVolumeSerialNumber;
    result.v[1] = info.nFileIndexLow;
    result.v[2] = info.nFileIndexHigh;
  }
  return result;
}

internal B32
file_reserve_size(File file, U64 size)
{
  HANDLE handle = (HANDLE)file.u64[0];
  
  FILE_ALLOCATION_INFO alloc_info    = {0};
  alloc_info.AllocationSize.LowPart  = size & max_U32;
  alloc_info.AllocationSize.HighPart = (size >> 32) & max_U32;
  
  BOOL is_reserved = SetFileInformationByHandle(handle, FileAllocationInfo, &alloc_info, sizeof(alloc_info));
  return is_reserved;
}

internal B32
delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  B32 result = DeleteFileW((WCHAR*)path16.str);
  scratch_end(scratch);
  return result;
}

internal B32
copy_file_path(String8 dst, String8 src)
{
  Temp scratch = scratch_begin(0, 0);
  String16 dst16 = str16_from_8(scratch.arena, dst);
  String16 src16 = str16_from_8(scratch.arena, src);
  B32 result = CopyFileW((WCHAR*)src16.str, (WCHAR*)dst16.str, 0);
  scratch_end(scratch);
  return result;
}

internal B32
move_file_path(String8 dst, String8 src)
{
  Temp scratch = scratch_begin(0, 0);
  String16 dst16 = str16_from_8(scratch.arena, dst);
  String16 src16 = str16_from_8(scratch.arena, src);
  B32 result = MoveFileW((WCHAR*)src16.str, (WCHAR*)dst16.str);
  scratch_end(scratch);
  return result;
}

internal String8
full_path_from_path(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  DWORD     buffer_size = Max(MAX_PATH, path.size * 2) + 1;
  String16  path16      = str16_from_8(scratch.arena, path);
  WCHAR    *buffer      = push_array_no_zero(scratch.arena, WCHAR, buffer_size);
  DWORD     path16_size = GetFullPathNameW((WCHAR*)path16.str, buffer_size, buffer, NULL);
  if(path16_size > buffer_size)
  {
    arena_pop(scratch.arena, buffer_size);
    buffer_size = path16_size + 1;
    buffer      = push_array_no_zero(scratch.arena, WCHAR, buffer_size);
    path16_size = GetFullPathNameW((WCHAR*)path16.str, buffer_size, buffer, NULL);
  }
  String8 full_path = str8_from_16(arena, str16((U16*)buffer, path16_size));
  scratch_end(scratch);
  return full_path;
}

internal B32
file_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0,0);
  String16 path16 = str16_from_8(scratch.arena, path);
  DWORD attributes = GetFileAttributesW((WCHAR *)path16.str);
  B32 exists = (attributes != INVALID_FILE_ATTRIBUTES) && !!(~attributes & FILE_ATTRIBUTE_DIRECTORY);
  scratch_end(scratch);
  return exists;
}

internal B32
folder_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0,0);
  String16 path16     = str16_from_8(scratch.arena, path);
  DWORD    attributes = GetFileAttributesW((WCHAR *)path16.str);
  B32      exists     = (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
  scratch_end(scratch);
  return exists;
}

internal FileProperties
properties_from_file_path(String8 path)
{
  WIN32_FIND_DATAW find_data = {0};
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  HANDLE handle = FindFirstFileW((WCHAR *)path16.str, &find_data);
  FileProperties props = {0};
  if(handle != INVALID_HANDLE_VALUE)
  {
    props.size = Compose64Bit(find_data.nFileSizeHigh, find_data.nFileSizeLow);
    w32_dense_time_from_file_time(&props.created, &find_data.ftCreationTime);
    w32_dense_time_from_file_time(&props.modified, &find_data.ftLastWriteTime);
    props.flags = w32_file_property_flags_from_dwFileAttributes(find_data.dwFileAttributes);
  }
  else
  {
    Temp scratch = scratch_begin(0, 0);
    WCHAR buffer[512] = {0};
    DWORD length = GetLogicalDriveStringsW(sizeof(buffer), buffer);
    U64 last_slash_pos = 0;
    for(;last_slash_pos < path.size; last_slash_pos = str8_find_needle(path, last_slash_pos+1, str8_lit("/"), StringMatchFlag_SlashInsensitive));
    String8 path_trimmed = str8_prefix(path, last_slash_pos);
    for(U64 off = 0; off < (U64)length;)
    {
      String16 next_drive_string_16 = str16_cstring((U16 *)buffer+off);
      off += next_drive_string_16.size+1;
      String8 next_drive_string = str8_from_16(scratch.arena, next_drive_string_16);
      next_drive_string = str8_chop_last_slash(next_drive_string);
      if(str8_match(path_trimmed, next_drive_string, StringMatchFlag_CaseInsensitive))
      {
        props.flags |= FilePropertyFlag_IsFolder;
        break;
      }
    }
    scratch_end(scratch);
  }
  FindClose(handle);
  scratch_end(scratch);
  return props;
}

//- rjf: file maps

internal FileMap
file_map_open(AccessFlags flags, File file)
{
  FileMap map = {0};
  {
    HANDLE file_handle = (HANDLE)file.u64[0];
    DWORD protect_flags = 0;
    {
      switch(flags)
      {
        default:{}break;
        case AccessFlag_Read:
        {protect_flags = PAGE_READONLY;}break;
        case AccessFlag_Write:
        case AccessFlag_Read|AccessFlag_Write:
        {protect_flags = PAGE_READWRITE;}break;
        case AccessFlag_Execute:
        case AccessFlag_Read|AccessFlag_Execute:
        {protect_flags = PAGE_EXECUTE_READ;}break;
        case AccessFlag_Execute|AccessFlag_Write|AccessFlag_Read:
        case AccessFlag_Execute|AccessFlag_Write:
        {protect_flags = PAGE_EXECUTE_READWRITE;}break;
      }
    }
    HANDLE map_handle = CreateFileMappingA(file_handle, 0, protect_flags, 0, 0, 0);
    map.u64[0] = (U64)map_handle;
  }
  return map;
}

internal void
file_map_close(FileMap map)
{
  HANDLE handle = (HANDLE)map.u64[0];
  BOOL result = CloseHandle(handle);
  (void)result;
}

internal void *
file_map_view_open(FileMap map, AccessFlags flags, Rng1U64 range)
{
  HANDLE handle = (HANDLE)map.u64[0];
  U32 off_lo = (U32)((range.min&0x00000000ffffffffull)>>0);
  U32 off_hi = (U32)((range.min&0xffffffff00000000ull)>>32);
  U64 size = dim_1u64(range);
  DWORD access_flags = 0;
  {
    switch(flags)
    {
      default:{}break;
      case AccessFlag_Read:
      {
        access_flags = FILE_MAP_READ;
      }break;
      case AccessFlag_Write:
      {
        access_flags = FILE_MAP_WRITE;
      }break;
      case AccessFlag_Read|AccessFlag_Write:
      {
        access_flags = FILE_MAP_ALL_ACCESS;
      }break;
      case AccessFlag_Execute:
      case AccessFlag_Read|AccessFlag_Execute:
      case AccessFlag_Write|AccessFlag_Execute:
      case AccessFlag_Read|AccessFlag_Write|AccessFlag_Execute:
      {
        access_flags = FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE;
      }break;
    }
  }
  void *result = MapViewOfFile(handle, access_flags, off_hi, off_lo, size);
  DWORD err = GetLastError();
  return result;
}

internal void
file_map_view_close(FileMap map, void *ptr, Rng1U64 range)
{
  BOOL result = UnmapViewOfFile(ptr);
  (void)result;
}

//- rjf: directory iteration

internal FileIter *
file_iter_begin(Arena *arena, String8 path, FileIterFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 path_with_wildcard = push_str8_cat(scratch.arena, path, str8_lit("\\*"));
  String16 path16 = str16_from_8(scratch.arena, path_with_wildcard);
  FileIter *iter = push_array(arena, FileIter, 1);
  iter->flags = flags;
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  if(path.size == 0)
  {
    w32_iter->is_volume_iter = 1;
    WCHAR buffer[512] = {0};
    DWORD length = GetLogicalDriveStringsW(sizeof(buffer), buffer);
    String8List drive_strings = {0};
    for(U64 off = 0; off < (U64)length;)
    {
      String16 next_drive_string_16 = str16_cstring((U16 *)buffer+off);
      off += next_drive_string_16.size+1;
      String8 next_drive_string = str8_from_16(arena, next_drive_string_16);
      next_drive_string = str8_chop_last_slash(next_drive_string);
      str8_list_push(scratch.arena, &drive_strings, next_drive_string);
    }
    w32_iter->drive_strings = str8_array_from_list(arena, &drive_strings);
    w32_iter->drive_strings_iter_idx = 0;
  }
  else
  {
    w32_iter->handle = FindFirstFileExW((WCHAR*)path16.str, FindExInfoBasic, &w32_iter->find_data, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
  }
  scratch_end(scratch);
  return iter;
}

internal B32
file_iter_next(Arena *arena, FileIter *iter, FileInfo *info_out)
{
  B32 result = 0;
  FileIterFlags flags = iter->flags;
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  switch(w32_iter->is_volume_iter)
  {
    //- rjf: file iteration
    default:
    case 0:
    {
      if (!(flags & FileIterFlag_Done) && w32_iter->handle != INVALID_HANDLE_VALUE)
      {
        do
        {
          // check is usable
          B32 usable_file = 1;
          
          WCHAR *file_name = w32_iter->find_data.cFileName;
          DWORD attributes = w32_iter->find_data.dwFileAttributes;
          if (file_name[0] == '.'){
            if (flags & FileIterFlag_SkipHiddenFiles){
              usable_file = 0;
            }
            else if (file_name[1] == 0){
              usable_file = 0;
            }
            else if (file_name[1] == '.' && file_name[2] == 0){
              usable_file = 0;
            }
          }
          if (attributes & FILE_ATTRIBUTE_DIRECTORY){
            if (flags & FileIterFlag_SkipFolders){
              usable_file = 0;
            }
          }
          else{
            if (flags & FileIterFlag_SkipFiles){
              usable_file = 0;
            }
          }
          
          // emit if usable
          if (usable_file){
            info_out->name = str8_from_16(arena, str16_cstring((U16*)file_name));
            info_out->props.size = (U64)w32_iter->find_data.nFileSizeLow | (((U64)w32_iter->find_data.nFileSizeHigh)<<32);
            w32_dense_time_from_file_time(&info_out->props.created,  &w32_iter->find_data.ftCreationTime);
            w32_dense_time_from_file_time(&info_out->props.modified, &w32_iter->find_data.ftLastWriteTime);
            info_out->props.flags = w32_file_property_flags_from_dwFileAttributes(attributes);
            result = 1;
            if (!FindNextFileW(w32_iter->handle, &w32_iter->find_data)){
              iter->flags |= FileIterFlag_Done;
            }
            break;
          }
        }while(FindNextFileW(w32_iter->handle, &w32_iter->find_data));
      }
    }break;
    
    //- rjf: volume iteration
    case 1:
    {
      result = w32_iter->drive_strings_iter_idx < w32_iter->drive_strings.count;
      if(result != 0)
      {
        MemoryZeroStruct(info_out);
        info_out->name = w32_iter->drive_strings.v[w32_iter->drive_strings_iter_idx];
        info_out->props.flags |= FilePropertyFlag_IsFolder;
        w32_iter->drive_strings_iter_idx += 1;
      }
    }break;
  }
  if(!result)
  {
    iter->flags |= FileIterFlag_Done;
  }
  return result;
}

internal void
file_iter_end(FileIter *iter)
{
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  HANDLE zero_handle;
  MemoryZeroStruct(&zero_handle);
  if(!MemoryMatchStruct(&zero_handle, &w32_iter->handle))
  {
    FindClose(w32_iter->handle);
  }
}

//- rjf: directory creation

internal B32
make_directory(String8 path)
{
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, path);
  WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
  GetFileAttributesExW((WCHAR*)name16.str, GetFileExInfoStandard, &attributes);
  if(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    result = 1;
  }
  else if(CreateDirectoryW((WCHAR*)name16.str, 0))
  {
    result = 1;
  }
  scratch_end(scratch);
  return(result);
}

////////////////////////////////
//~ rjf: @per_os_impl Aborting

internal void
abort_self(S32 exit_code)
{
  ExitProcess(exit_code);
}

////////////////////////////////
//~ rjf: @per_os_impl Process Info

internal ProcessInfo *
get_process_info(void)
{
  return &w32_state.process_info;
}

internal String8
get_current_path(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  DWORD length = GetCurrentDirectoryW(0, 0);
  U16 *memory = push_array_no_zero(scratch.arena, U16, length + 1);
  length = GetCurrentDirectoryW(length + 1, (WCHAR*)memory);
  String8 name = str8_from_16(arena, str16(memory, length));
  scratch_end(scratch);
  return name;
}

internal U32
get_process_start_time_unix(void)
{
  U32 result = 0;
  HANDLE handle = GetCurrentProcess();
  FILETIME start_time = {0};
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  if(GetProcessTimes(handle, &start_time, &exit_time, &kernel_time, &user_time))
  {
    result = w32_unix_time_from_file_time(start_time);
  }
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Child Processes

internal Process
process_launch(ProcessLaunchParams *params)
{
  Process result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: form full command string
  String8 cmd = {0};
  {
    StringJoin join_params = {0};
    join_params.pre = str8_lit("\"");
    join_params.sep = str8_lit("\" \"");
    join_params.post = str8_lit("\"");
    cmd = str8_list_join(scratch.arena, &params->cmd_line, &join_params);
  }
  
  //- rjf: form environment
  B32 use_null_env_arg = 0;
  String8 env = {0};
  {
    StringJoin join_params2 = {0};
    join_params2.sep = str8_lit("\0");
    join_params2.post = str8_lit("\0");
    String8List all_opts = params->env;
    if(params->inherit_env != 0)
    {
      if(all_opts.node_count != 0)
      {
        MemoryZeroStruct(&all_opts);
        for(String8Node *n = params->env.first; n != 0; n = n->next)
        {
          str8_list_push(scratch.arena, &all_opts, n->string);
        }
        for(String8Node *n = w32_state.process_info.environment.first; n != 0; n = n->next)
        {
          str8_list_push(scratch.arena, &all_opts, n->string);
        }
      }
      else
      {
        use_null_env_arg = 1;
      }
    }
    if(use_null_env_arg == 0)
    {
      env = str8_list_join(scratch.arena, &all_opts, &join_params2);
    }
  }
  
  //- rjf: utf-8 -> utf-16
  String16 cmd16 = str16_from_8(scratch.arena, cmd);
  String16 dir16 = str16_from_8(scratch.arena, params->path);
  String16 env16 = {0};
  if(use_null_env_arg == 0)
  {
    env16 = str16_from_8(scratch.arena, env);
  }
  
  //- rjf: determine creation flags
  DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
  if(params->consoleless)
  {
    creation_flags |= CREATE_NO_WINDOW;
  }
  
  //- rjf: launch
  BOOL inherit_handles = 0;
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  if(!file_match(params->stdout_file, file_zero()))
  {
    HANDLE stdout_handle = (HANDLE)params->stdout_file.u64[0];
    startup_info.hStdOutput = stdout_handle;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    inherit_handles = 1;
  }
  if(!file_match(params->stderr_file, file_zero()))
  {
    HANDLE stderr_handle = (HANDLE)params->stderr_file.u64[0];
    startup_info.hStdError = stderr_handle;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    inherit_handles = 1;
  }
  if(!file_match(params->stdin_file, file_zero()))
  {
    HANDLE stdin_handle = (HANDLE)params->stdin_file.u64[0];
    startup_info.hStdInput = stdin_handle;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    inherit_handles = 1;
  }
  PROCESS_INFORMATION process_info = {0};
  if(CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, inherit_handles, creation_flags, use_null_env_arg ? 0 : (WCHAR*)env16.str, (WCHAR*)dir16.str, &startup_info, &process_info))
  {
    result.u64[0] = (U64)process_info.hProcess;
    CloseHandle(process_info.hThread);
  }
  
  scratch_end(scratch);
  return result;
}

internal U64
pid_from_process(Process process)
{
  HANDLE process_handle = (HANDLE)process.u64[0];
  U64 result = (U64)GetProcessId(process_handle);
  return result;
}

internal B32
process_join(Process process, U64 endt_us, U64 *exit_code_out)
{
  HANDLE process_handle = (HANDLE)(process.u64[0]);
  DWORD sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  DWORD result = WaitForSingleObject(process_handle, sleep_ms);
  B32 process_joined = (result == WAIT_OBJECT_0);
  if(process_joined && exit_code_out)
  {
    DWORD exit_code = 0;
    if(GetExitCodeProcess(process_handle, &exit_code))
    {
      *exit_code_out = exit_code;
    }
  }
  if(process_joined)
  {
    CloseHandle(process_handle);
  }
  return process_joined;
}

internal void
process_detach(Process process)
{
  HANDLE process_handle = (HANDLE)(process.u64[0]);
  CloseHandle(process_handle);
}

internal B32
process_kill(Process process)
{
  HANDLE process_handle = (HANDLE)process.u64[0];
  BOOL was_terminated = TerminateProcess(process_handle, 999);
  return was_terminated;
}

////////////////////////////////
//~ rjf: @per_os_impl Dynamically-Loaded Libraries

internal Library
library_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  HMODULE mod = LoadLibraryW((LPCWSTR)path16.str);
  Library result = { (U64)mod };
  scratch_end(scratch);
  return result;
}

internal void
library_close(Library lib)
{
  HMODULE mod = (HMODULE)lib.u64[0];
  FreeLibrary(mod);
}

internal VoidProc *
library_load_proc(Library lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  HMODULE mod = (HMODULE)lib.u64[0];
  name = push_str8_copy(scratch.arena, name);
  VoidProc *result = (VoidProc*)GetProcAddress(mod, (LPCSTR)name.str);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Entry Point

#include <dbghelp.h>
#undef OS_WINDOWS // shlwapi uses its own OS_WINDOWS include inside
#include <shlwapi.h>

internal B32 win32_g_is_quiet = 0;
internal B32 win32_g_gen_dump = 0;

internal HRESULT WINAPI
win32_dialog_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LONG_PTR data)
{
  if(msg == TDN_HYPERLINK_CLICKED)
  {
    ShellExecuteW(NULL, L"open", (LPWSTR)lparam, NULL, NULL, SW_SHOWNORMAL);
  }
  return S_OK;
}

internal LONG WINAPI
win32_exception_filter(EXCEPTION_POINTERS* exception_ptrs)
{
  if(win32_g_is_quiet)
  {
    ExitProcess(1);
  }
  
  static volatile LONG first = 0;
  if(InterlockedCompareExchange(&first, 1, 0) != 0)
  {
    // prevent failures in other threads to popup same message box
    // this handler just shows first thread that crashes
    // we are terminating afterwards anyway
    for (;;) Sleep(1000);
  }
  
  WCHAR buffer[4096] = {0};
  int buflen = 0;
  
  DWORD exception_code = exception_ptrs->ExceptionRecord->ExceptionCode;
  buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"A fatal exception (code 0x%x) occurred. The process is terminating.\n", exception_code);
  
  // load dbghelp dynamically just in case if it is missing
  BOOL (WINAPI *dbg_MiniDumpWriteDump)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam) = 0;
  HMODULE dbghelp = LoadLibraryA("dbghelp.dll");
  if(dbghelp)
  {
    DWORD (WINAPI *dbg_SymSetOptions)(DWORD SymOptions);
    BOOL (WINAPI *dbg_SymInitializeW)(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);
    BOOL (WINAPI *dbg_StackWalk64)(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                                   LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                   PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                   PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
    PVOID (WINAPI *dbg_SymFunctionTableAccess64)(HANDLE hProcess, DWORD64 AddrBase);
    DWORD64 (WINAPI *dbg_SymGetModuleBase64)(HANDLE hProcess, DWORD64 qwAddr);
    BOOL (WINAPI *dbg_SymFromAddrW)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol);
    BOOL (WINAPI *dbg_SymGetLineFromAddrW64)(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line);
    BOOL (WINAPI *dbg_SymGetModuleInfoW64)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);
    
    *(FARPROC*)&dbg_SymSetOptions            = GetProcAddress(dbghelp, "SymSetOptions");
    *(FARPROC*)&dbg_SymInitializeW           = GetProcAddress(dbghelp, "SymInitializeW");
    *(FARPROC*)&dbg_StackWalk64              = GetProcAddress(dbghelp, "StackWalk64");
    *(FARPROC*)&dbg_SymFunctionTableAccess64 = GetProcAddress(dbghelp, "SymFunctionTableAccess64");
    *(FARPROC*)&dbg_SymGetModuleBase64       = GetProcAddress(dbghelp, "SymGetModuleBase64");
    *(FARPROC*)&dbg_SymFromAddrW             = GetProcAddress(dbghelp, "SymFromAddrW");
    *(FARPROC*)&dbg_SymGetLineFromAddrW64    = GetProcAddress(dbghelp, "SymGetLineFromAddrW64");
    *(FARPROC*)&dbg_SymGetModuleInfoW64      = GetProcAddress(dbghelp, "SymGetModuleInfoW64");
    *(FARPROC*)&dbg_MiniDumpWriteDump        = GetProcAddress(dbghelp, "MiniDumpWriteDump");
    
    if(dbg_SymSetOptions && dbg_SymInitializeW && dbg_StackWalk64 && dbg_SymFunctionTableAccess64 && dbg_SymGetModuleBase64 && dbg_SymFromAddrW && dbg_SymGetLineFromAddrW64 && dbg_SymGetModuleInfoW64)
    {
      HANDLE process = GetCurrentProcess();
      HANDLE thread = GetCurrentThread();
      CONTEXT context = *exception_ptrs->ContextRecord;
      
      WCHAR module_path[MAX_PATH];
      GetModuleFileNameW(NULL, module_path, ArrayCount(module_path));
      PathRemoveFileSpecW(module_path);
      
      dbg_SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
      if(dbg_SymInitializeW(process, module_path, TRUE))
      {
        // check that raddbg.pdb file is good
        B32 raddbg_pdb_valid = 0;
        {
          IMAGEHLP_MODULEW64 module = {0};
          module.SizeOfStruct = sizeof(module);
          if(dbg_SymGetModuleInfoW64(process, (DWORD64)&win32_exception_filter, &module))
          {
            raddbg_pdb_valid = (module.SymType == SymPdb);
          }
        }
        
        if(!raddbg_pdb_valid)
        {
          buflen += wnsprintfW(buffer + buflen, sizeof(buffer) - buflen,
                               L"\nThe PDB debug information file for this executable is not valid or was not found. Please rebuild binary to get the call stack.\n");
        }
        else
        {
          STACKFRAME64 frame = {0};
          DWORD image_type;
#if defined(_M_AMD64)
          image_type = IMAGE_FILE_MACHINE_AMD64;
          frame.AddrPC.Offset = context.Rip;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context.Rbp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context.Rsp;
          frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_ARM64)
          image_type = IMAGE_FILE_MACHINE_ARM64;
          frame.AddrPC.Offset = context.Pc;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context.Fp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context.Sp;
          frame.AddrStack.Mode = AddrModeFlat;
#else
#  error Arch not supported!
#endif
          
#if BUILD_CONSOLE_INTERFACE
          buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\nCreate a new issue with this report at %S.\n\n", BUILD_ISSUES_LINK_STRING_LITERAL);
#else
          buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen,
                               L"\nPress Ctrl+C to copy this text to clipboard, then create a new issue at\n"
                               L"<a href=\"%S\">%S</a>\n\n", BUILD_ISSUES_LINK_STRING_LITERAL, BUILD_ISSUES_LINK_STRING_LITERAL);
#endif
          buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"Call stack:\n");
          
          U64 frame_offset = 0;
          
          if (frame.AddrPC.Offset == 0)
          {
            // if IP address is 0 then most likely we have called indirectly on NULL function pointer
            // which means no useful stack unwinding will happen, because there's no unwind info for address 0
            // but we can try reading 8 bytes of return address from stack, and start unwinding there
            
            ULONG_PTR hi, lo;
            GetCurrentThreadStackLimits(&lo, &hi);
            if (frame.AddrStack.Offset >= lo && frame.AddrStack.Offset <= hi - sizeof(void*))
            {
              frame.AddrPC.Offset = *(DWORD64*)frame.AddrStack.Offset - 1;
              frame.AddrStack.Offset += sizeof(void*);
#if defined(_M_AMD64)
              context.Rip = frame.AddrPC.Offset;
              context.Rsp = frame.AddrStack.Offset;
#elif defined(_M_ARM64)
              context.Pc = frame.AddrPC.Offset;
              context.Sp = frame.AddrStack.Offset;
#endif
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"1. [NULL]\n");
            frame_offset = 1;
          }
          
          for(U32 idx=0; ;idx++)
          {
            const U32 max_frames = 32;
            if(idx == max_frames)
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"...");
              break;
            }
            
            if(!dbg_StackWalk64(image_type, process, thread, &frame, &context, 0, dbg_SymFunctionTableAccess64, dbg_SymGetModuleBase64, 0))
            {
              break;
            }
            
            U64 address = frame.AddrPC.Offset;
            if(address == 0)
            {
              break;
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"%u. [0x%I64x]", frame_offset + idx + 1, address);
            
            struct {
              SYMBOL_INFOW info;
              WCHAR name[MAX_SYM_NAME];
            } symbol = {0};
            
            symbol.info.SizeOfStruct = sizeof(symbol.info);
            symbol.info.MaxNameLen = MAX_SYM_NAME;
            
            DWORD64 displacement = 0;
            if(dbg_SymFromAddrW(process, address, &displacement, &symbol.info))
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L" %s +%u", symbol.info.Name, (DWORD)displacement);
              
              IMAGEHLP_LINEW64 line = {0};
              line.SizeOfStruct = sizeof(line);
              
              DWORD line_displacement = 0;
              if(dbg_SymGetLineFromAddrW64(process, address, &line_displacement, &line))
              {
                buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L", %s line %u", PathFindFileNameW(line.FileName), line.LineNumber);
              }
            }
            else
            {
              IMAGEHLP_MODULEW64 module = {0};
              module.SizeOfStruct = sizeof(module);
              if(dbg_SymGetModuleInfoW64(process, address, &module))
              {
                buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L" %s", module.ModuleName);
              }
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\n");
          }
        }
      }
    }
  }
  
  buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\nVersion: %S%S", BUILD_VERSION_STRING_LITERAL, BUILD_GIT_HASH_STRING_LITERAL_APPEND);
  
  B32 generate_crash_dump = win32_g_gen_dump;
#if BUILD_CONSOLE_INTERFACE
  fwprintf(stderr, L"\n--- Fatal Exception ---\n");
  fwprintf(stderr, L"%s\n\n", buffer);
#else
  int selected_button = 0;
  TASKDIALOG_BUTTON generate_dump = {1, L"Generate Crash Dump File"};
  TASKDIALOGCONFIG dialog = {0};
  dialog.cbSize = sizeof(dialog);
  dialog.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
  dialog.pszMainIcon = TD_ERROR_ICON;
  dialog.dwCommonButtons = TDCBF_CLOSE_BUTTON;
  dialog.pszWindowTitle = L"Fatal Exception";
  dialog.pszContent = buffer;
  dialog.pfCallback = &win32_dialog_callback;
  dialog.cButtons = 1;
  dialog.pButtons = &generate_dump;
  TaskDialogIndirect(&dialog, &selected_button, 0, 0);
  generate_crash_dump = (selected_button == generate_dump.nButtonID);
#endif
  
  if(dbg_MiniDumpWriteDump && generate_crash_dump)
  {
    WCHAR dump_file_path[MAX_PATH] = {0};
    SHGetFolderPathW(0, CSIDL_DESKTOP, 0, 0, dump_file_path);
    PathAppendW(dump_file_path, L"raddbg_crash_dump.dmp");
    HANDLE file = CreateFileW(dump_file_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(file != INVALID_HANDLE_VALUE)
    {
      MINIDUMP_EXCEPTION_INFORMATION info = {0};
      info.ThreadId = GetCurrentThreadId();
      info.ExceptionPointers = exception_ptrs;
      info.ClientPointers = FALSE;
      BOOL dump_successful = dbg_MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &info, 0, 0);
      CloseHandle(file);
      if(dump_successful)
      {
#if !BUILD_CONSOLE_INTERFACE
        // opens explorer and selects file
        SFGAOF flags = 0;
        PIDLIST_ABSOLUTE list = 0;
        if (SUCCEEDED(SHParseDisplayName(dump_file_path, NULL, &list, 0, &flags)))
        {
          SHOpenFolderAndSelectItems(list, 0, NULL, 0);
          CoTaskMemFree(list);
        }
#endif
      }
    }
  }
  
  ExitProcess(1);
}

#undef OS_WINDOWS // shlwapi uses its own OS_WINDOWS include inside
#define OS_WINDOWS 1

internal void
w32_entry_point_caller(int argc, WCHAR **wargv)
{
  SetUnhandledExceptionFilter(&win32_exception_filter);
  
  //- rjf: dynamically load windows functions which are not guaranteed
  // in all SDKs
  {
    HMODULE module = LoadLibraryA("kernel32.dll");
    w32_SetThreadDescription_func = (W32_SetThreadDescription_Type *)GetProcAddress(module, "SetThreadDescription");
    w32_InitializeSynchronizationBarrier_func = (W32_InitializeSynchronizationBarrier_Type *)GetProcAddress(module, "InitializeSynchronizationBarrier");
    w32_DeleteSynchronizationBarrier_func = (W32_DeleteSynchronizationBarrier_Type *)GetProcAddress(module, "DeleteSynchronizationBarrier");
    w32_EnterSynchronizationBarrier_func = (W32_EnterSynchronizationBarrier_Type *)GetProcAddress(module, "EnterSynchronizationBarrier");
    if(w32_InitializeSynchronizationBarrier_func == 0)
    {
      w32_DeleteSynchronizationBarrier_func = 0;
      w32_EnterSynchronizationBarrier_func = 0;
    }
    FreeLibrary(module);
  }
  
  //- rjf: try to allow large pages if we can
  B32 large_pages_allowed = 0;
  {
    HANDLE token;
    if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    {
      LUID luid;
      if(LookupPrivilegeValue(0, SE_LOCK_MEMORY_NAME, &luid))
      {
        TOKEN_PRIVILEGES priv;
        priv.PrivilegeCount           = 1;
        priv.Privileges[0].Luid       = luid;
        priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        large_pages_allowed = !!AdjustTokenPrivileges(token, 0, &priv, sizeof(priv), 0, 0);
      }
      CloseHandle(token);
    }
  }
  
  //- rjf: get RIO extension function table
  {
    // NOTE(mmozeiko): need to get function pointers to RIO functions, and that requires dummy socket
    WSADATA WinSockData;
    WSAStartup(MAKEWORD(2, 2), &WinSockData);
    GUID guid = WSAID_MULTIPLE_RIO;
    DWORD rio_byte = 0;
    SOCKET Sock = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
    WSAIoctl(Sock, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), (void**)&w32_rio_functions, sizeof(w32_rio_functions), &rio_byte, 0, 0);
    closesocket(Sock);
  }
  
  //- rjf: get system info
  SYSTEM_INFO sysinfo = {0};
  GetSystemInfo(&sysinfo);
  
  //- rjf: set up non-dynamically-alloc'd state
  //
  // (we need to set up some basics before this layer can supply
  // memory allocation primitives)
  {
    w32_state.microsecond_resolution  = 1;
    LARGE_INTEGER large_int_resolution;
    if(QueryPerformanceFrequency(&large_int_resolution))
    {
      w32_state.microsecond_resolution = large_int_resolution.QuadPart;
    }
  }
  {
    SystemInfo *info = &w32_state.system_info;
    info->logical_processor_count = (U64)sysinfo.dwNumberOfProcessors;
    info->page_size               = sysinfo.dwPageSize;
    info->large_page_size         = GetLargePageMinimum();
    info->allocation_granularity  = sysinfo.dwAllocationGranularity;
  }
  {
    ProcessInfo *info = &w32_state.process_info;
    info->large_pages_allowed = large_pages_allowed;
    info->pid = GetCurrentProcessId();
  }
  
  //- rjf: extract arguments
  Arena *args_arena = arena_alloc(.reserve_size = MB(1), .commit_size = KB(32));
  char **argv = push_array(args_arena, char *, argc);
  for(int i = 0; i < argc; i += 1)
  {
    String16 arg16 = str16_cstring((U16 *)wargv[i]);
    String8 arg8 = str8_from_16(args_arena, arg16);
    if(str8_match(arg8, str8_lit("--quiet"), StringMatchFlag_CaseInsensitive) ||
       str8_match(arg8, str8_lit("-quiet"), StringMatchFlag_CaseInsensitive))
    {
      win32_g_is_quiet = 1;
    }
    if(str8_match(arg8, str8_lit("--large_pages"), StringMatchFlag_CaseInsensitive) ||
       str8_match(arg8, str8_lit("-large_pages"), StringMatchFlag_CaseInsensitive))
    {
      arena_default_flags        = ArenaFlag_LargePages;
      arena_default_reserve_size = Max(MB(64), w32_state.system_info.large_page_size);
      arena_default_commit_size  = arena_default_reserve_size;
    }
    if(str8_match(arg8, str8_lit("--gen_crash_dump"), StringMatchFlag_CaseInsensitive) ||
       str8_match(arg8, str8_lit("-gen_crash_dump"), StringMatchFlag_CaseInsensitive))
    {
      win32_g_gen_dump = 1;
    }
    argv[i] = (char *)arg8.str;
  }
  
  //- rjf: set up thread context
  TCTX *tctx = tctx_alloc();
  tctx_select(tctx);
  
  //- rjf: set up dynamically-alloc'd state
  Arena *arena = arena_alloc();
  {
    w32_state.arena = arena;
    {
      SystemInfo *info = &w32_state.system_info;
      U8 buffer[MAX_COMPUTERNAME_LENGTH + 1] = {0};
      DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
      if(GetComputerNameA((char*)buffer, &size))
      {
        info->machine_name = push_str8_copy(arena, str8(buffer, size));
      }
    }
  }
  {
    ProcessInfo *info = &w32_state.process_info;
    {
      Temp scratch = scratch_begin(0, 0);
      DWORD size = KB(32);
      U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
      DWORD length = GetModuleFileNameW(0, (WCHAR*)buffer, size);
      String8 name8 = str8_from_16(scratch.arena, str16(buffer, length));
      info->binary_file_path = push_str8_copy(arena, name8);
      info->binary_path = str8_chop_last_slash(info->binary_file_path);
      scratch_end(scratch);
    }
    info->initial_path = get_current_path(arena);
    {
      Temp scratch = scratch_begin(0, 0);
      U64 size = KB(32);
      U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
      if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR*)buffer)))
      {
        info->user_program_config_data_path = str8_from_16(arena, str16_cstring(buffer));
        info->user_program_cache_data_path = info->user_program_logs_data_path = info->user_program_config_data_path;
      }
      scratch_end(scratch);
    }
    {
      WCHAR *this_proc_env = GetEnvironmentStringsW();
      U64 start_idx = 0;
      for(U64 idx = 0;; idx += 1)
      {
        if(this_proc_env[idx] == 0)
        {
          if(start_idx == idx)
          {
            break;
          }
          else
          {
            String16 string16 = str16((U16 *)this_proc_env + start_idx, idx - start_idx);
            String8 string = str8_from_16(arena, string16);
            if(str8_match(string, s("_NT_SYMBOL_PATH="), StringMatchFlag_RightSideSloppy))
            {
              info->symbol_cache_path = str8_skip(string, 16);
              info->symbol_cache_path = str8_skip_chop_whitespace(info->symbol_cache_path);
            }
            str8_list_push(arena, &info->environment, string);
            start_idx = idx+1;
          }
        }
      }
    }
    if(info->symbol_cache_path.size == 0)
    {
      Temp scratch = scratch_begin(0, 0);
      U64 size = KB(32);
      U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
      if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, 0, (WCHAR*)buffer)))
      {
        String8 local_appdata = str8_from_16(scratch.arena, str16_cstring(buffer));
        info->symbol_cache_path = str8f(arena, "%S\\Temp\\SymbolCache", local_appdata);
        
      }
      scratch_end(scratch);
    }
  }
  
  //- rjf: set up entity storage
  InitializeCriticalSection(&w32_state.entity_mutex);
  w32_state.entity_arena = arena_alloc();
  
  //- rjf: call into "real" entry point
  main_thread_base_entry_point(argc, argv);
}

#if BUILD_CONSOLE_INTERFACE
int wmain(int argc, WCHAR **argv)
{
  w32_entry_point_caller(argc, argv);
  return 0;
}
#else
int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
  CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  w32_entry_point_caller(__argc, __wargv);
  return 0;
}
#endif
