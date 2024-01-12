// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma comment(lib, "user32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "shell32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "rpcrt4")

////////////////////////////////
//~ allen: Definitions For Symbols That Are Sometimes Missing in Older Windows SDKs

#if !defined(MEM_PRESERVE_PLACEHOLDER)
#define MEM_PRESERVE_PLACEHOLDER 0x2
#endif
#if !defined(MEM_RESERVE_PLACEHOLDER)
# define MEM_REPLACE_PLACEHOLDER 0x00004000
#endif
#if !defined(MEM_RESERVE_PLACEHOLDER)
# define MEM_RESERVE_PLACEHOLDER 0x00040000
#endif

typedef PVOID W32_VirtualAlloc2_Type(HANDLE Process,
                                     PVOID  BaseAddress,
                                     SIZE_T Size,
                                     ULONG  AllocationType,
                                     ULONG  PageProtection,
                                     void*  ExtendedParameters,
                                     ULONG  ParameterCount);
typedef PVOID W32_MapViewOfFile3_Type(HANDLE  FileMapping,
                                      HANDLE  Process,
                                      PVOID   BaseAddress,
                                      ULONG64 Offset,
                                      SIZE_T  ViewSize,
                                      ULONG   AllocationType,
                                      ULONG   PageProtection,
                                      void*   ExtendedParameters,
                                      ULONG   ParameterCount);

global W32_VirtualAlloc2_Type  *w32_VirtualAlloc2_func  = 0;
global W32_MapViewOfFile3_Type *w32_MapViewOfFile3_func = 0;

////////////////////////////////
//~ rjf: Globals

global Arena *w32_perm_arena = 0;
global String8List w32_cmd_line_args = {0};
global String8List w32_environment = {0};
global CRITICAL_SECTION w32_mutex = {0};
global String8 w32_initial_path = {0};
global U64 w32_microsecond_resolution = 0;
global W32_Entity *w32_entity_free = 0;
global B32 w32_large_pages_enabled = 0;

////////////////////////////////
//~ rjf: Helpers

//- rjf: files

internal FilePropertyFlags
w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes){
  FilePropertyFlags flags = 0;
  if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
    flags |= FilePropertyFlag_IsFolder;
  }
  return(flags);
}

internal void
w32_file_properties_from_attributes(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes){
  properties->size = Compose64Bit(attributes->nFileSizeHigh, attributes->nFileSizeLow);
  w32_dense_time_from_file_time(&properties->created, &attributes->ftCreationTime);
  w32_dense_time_from_file_time(&properties->modified, &attributes->ftLastWriteTime);
  properties->flags = w32_file_property_flags_from_dwFileAttributes(attributes->dwFileAttributes);
}

//- rjf: time

internal void
w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in){
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
w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in){
  out->wYear         = (WORD)(in->year);
  out->wMonth        = in->mon + 1;
  out->wDay          = in->day;
  out->wHour         = in->hour;
  out->wMinute       = in->min;
  out->wSecond       = in->sec;
  out->wMilliseconds = in->msec;
}

internal void
w32_dense_time_from_file_time(DenseTime *out, FILETIME *in){
  SYSTEMTIME systime_utc = {0};
  FileTimeToSystemTime(in, &systime_utc);
  SYSTEMTIME systime_local = {0};
  SystemTimeToTzSpecificLocalTime(NULL, &systime_utc, &systime_local);
  DateTime date_time = {0};
  w32_date_time_from_system_time(&date_time, &systime_local);
  *out = dense_time_from_date_time(date_time);
}

internal U32
w32_sleep_ms_from_endt_us(U64 endt_us){
  U32 sleep_ms = 0;
  if (endt_us == max_U64){
    sleep_ms = INFINITE;
  }
  else{
    U64 begint = os_now_microseconds();
    if (begint < endt_us){
      U64 sleep_us = endt_us - begint;
      sleep_ms = (U32)((sleep_us + 999)/1000);
    }
  }
  return(sleep_ms);
}

//- rjf: entities

internal W32_Entity*
w32_alloc_entity(W32_EntityKind kind){
  EnterCriticalSection(&w32_mutex);
  W32_Entity *result = w32_entity_free;
  if(result != 0)
  {
    SLLStackPop(w32_entity_free);
  }
  else
  {
    result = push_array_no_zero(w32_perm_arena, W32_Entity, 1);
  }
  MemoryZeroStruct(result);
  Assert(result != 0);
  LeaveCriticalSection(&w32_mutex);
  MemoryZeroStruct(result);
  result->kind = kind;
  return(result);
}

internal void
w32_free_entity(W32_Entity *entity){
  entity->kind = W32_EntityKind_Null;
  EnterCriticalSection(&w32_mutex);
  SLLStackPush(w32_entity_free, entity);
  LeaveCriticalSection(&w32_mutex);
}

//- rjf: threads

internal DWORD
w32_thread_base(void *ptr){
  W32_Entity *entity = (W32_Entity*)ptr;
  OS_ThreadFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  
  func(thread_ptr);
  
  // remove my bit
  LONG result = InterlockedAnd((LONG*)&entity->reference_mask, ~0x2);
  // if the other bit is also gone, free entity
  if ((result & 0x1) == 0){
    w32_free_entity(entity);
  }
  return(0);
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_init(int argc, char **argv){
  // Load Fancy Memory Functions
  {
    HMODULE module = LoadLibraryA("kernel32.dll");
    if (module != 0){
      w32_VirtualAlloc2_func = (W32_VirtualAlloc2_Type*)GetProcAddress(module, "VirtualAlloc2");
      w32_MapViewOfFile3_func = (W32_MapViewOfFile3_Type*)GetProcAddress(module, "MapViewOfFile3");
      FreeLibrary(module);
    }
  }
  
  // Thread handshake
  InitializeCriticalSection(&w32_mutex);
  
  // Permanent memory allocator for this layer
  w32_perm_arena = arena_alloc();
  
  // Init microsecond counter resolution
  LARGE_INTEGER large_int_resolution;
  if (QueryPerformanceFrequency(&large_int_resolution)){
    w32_microsecond_resolution = large_int_resolution.QuadPart;
  }
  else{
    w32_microsecond_resolution = 1;
  }
  
  // Setup initial path
  w32_initial_path = os_string_from_system_path(w32_perm_arena, OS_SystemPath_Current);
  
  // Setup command line arguments
  w32_cmd_line_args = os_string_list_from_argcv(w32_perm_arena, argc, argv);
  
  // rjf: setup environment variables
  {
    CHAR *this_proc_env = GetEnvironmentStrings();
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
          String8 string = str8((U8 *)this_proc_env + start_idx, idx - start_idx);
          str8_list_push(w32_perm_arena, &w32_environment, string);
          start_idx = idx+1;
        }
      }
    }
  }
}

////////////////////////////////
//~ rjf: @os_hooks Memory Allocation (Implemented Per-OS)

internal void*
os_reserve(U64 size){
  void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
  return(result);
}

internal B32
os_commit(void *ptr, U64 size){
  B32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
  return(result);
}

internal void*
os_reserve_large(U64 size){
  // we commit on reserve because windows
  void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
  return(result);
}

internal B32
os_commit_large(void *ptr, U64 size){
  return 1;
}

internal void
os_decommit(void *ptr, U64 size){
  VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal void
os_release(void *ptr, U64 size){
  // NOTE(rjf): size not used - not necessary on Windows, but necessary for other OSes.
  VirtualFree(ptr, 0, MEM_RELEASE);
}

internal B32
os_set_large_pages(B32 flag)
{
  B32 is_ok = 0;
  HANDLE token;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
    LUID luid;
    if (LookupPrivilegeValue(0, SE_LOCK_MEMORY_NAME, &luid)) {
      TOKEN_PRIVILEGES priv;
      priv.PrivilegeCount           = 1;
      priv.Privileges[0].Luid       = luid;
      priv.Privileges[0].Attributes = flag ? SE_PRIVILEGE_ENABLED: 0;
      if (AdjustTokenPrivileges(token, 0, &priv, sizeof(priv), 0, 0)) {
        w32_large_pages_enabled = flag;
        is_ok = 1;
      }
    }
    CloseHandle(token);
  }
  return is_ok;
}

internal B32
os_large_pages_enabled(void)
{
  return w32_large_pages_enabled;
}

internal U64
os_large_page_size(void)
{
  U64 page_size = GetLargePageMinimum();
  return page_size;
}

internal void*
os_alloc_ring_buffer(U64 size, U64 *actual_size_out){
  void *result = 0;
  
#define W32_MAX_RING_SIZE GB(1)
  
  Assert(IsPow2(size));
  Assert(size <= W32_MAX_RING_SIZE);
  
  // get allocation granularity
  SYSTEM_INFO info = {0};
  GetSystemInfo(&info);
  Assert(IsPow2(info.dwAllocationGranularity));
  
  // align size
  U64 aligned_size = AlignPow2(size, (U64)(info.dwAllocationGranularity));
  
  // split size
  U32 lo_size = (U32)(aligned_size & 0xFFFFFFFF);
  U32 hi_size = (U32)(aligned_size >> 32);
  
  // create pagefile-backed section
  HANDLE section = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, hi_size, lo_size, 0);
  
  if (section != 0){
    if (w32_VirtualAlloc2_func != 0 && w32_MapViewOfFile3_func != 0){
      void *ptr1 = 0;
      void *ptr2 = 0;
      void *view1 = 0;
      void *view2 = 0;
      
      // reserve virtual space placeholder
      ptr1 = w32_VirtualAlloc2_func(0, 0, aligned_size*2,
                                    MEM_RESERVE|MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, 0, 0);
      if (ptr1 != 0){
        
        // split off the first part of placeholder
        VirtualFree(ptr1, aligned_size, MEM_RELEASE|MEM_PRESERVE_PLACEHOLDER);
        ptr2 = ((U8*)ptr1 + aligned_size);
        
        // create views
        view1 = w32_MapViewOfFile3_func(section, 0, ptr1, 0, aligned_size,
                                        MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
        view2 = w32_MapViewOfFile3_func(section, 0, ptr2, 0, aligned_size,
                                        MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
        if (view1 != 0 && view2 != 0){
          result = ptr1;
          *actual_size_out = aligned_size;
        }
      }
      
      // cleanup
      if (result == 0){
        if (ptr1 != 0){
          VirtualFree(ptr1, 0, MEM_RELEASE);
        }
        if (ptr2 != 0){
          VirtualFree(ptr2, 0, MEM_RELEASE);
        }
        if (view1 != 0){
          UnmapViewOfFileEx(view1, 0);
        }
        if (view2 != 0){
          UnmapViewOfFileEx(view2, 0);
        }
      }
    }
    
    // no fancy memory functions available
    else{
      for (U64 addr = GB(16);
           addr < GB(272);
           addr += W32_MAX_RING_SIZE){
        
        // create the first view
        void *view1 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, aligned_size, (void*)addr);
        if (view1 != 0){
          
          // create the second view
          void *view2 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, aligned_size, (U8*)view1 + aligned_size);
          
          // on success make this the result
          if (view2 != 0){
            result = view1;
            *actual_size_out = aligned_size;
            break;
          }
          
          // cleanup view1 on failure
          UnmapViewOfFile(view1);
        }
      }
    }
  }
  
  // cleanup
  if (section != 0){
    CloseHandle(section);
  }
  
  return(result);
}

internal void
os_free_ring_buffer(void *ring_buffer, U64 actual_size){
  void *ptr1 = ring_buffer;
  void *ptr2 = ((U8*)ptr1 + actual_size);
  VirtualFree(ptr1, 0, MEM_RELEASE);
  VirtualFree(ptr2, 0, MEM_RELEASE);
  UnmapViewOfFileEx(ptr1, 0);
  UnmapViewOfFileEx(ptr2, 0);
}

////////////////////////////////
//~ rjf: @os_hooks System Info (Implemented Per-OS)

internal String8
os_machine_name(void){
  local_persist U8 buffer[MAX_COMPUTERNAME_LENGTH + 1];
  local_persist String8 string = {0};
  local_persist B32 first = 1;
  if (first){
    first = 0;
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA((char*)buffer, &size)){
      string = str8(buffer, size);
    }
  }
  return(string);
}

internal U64
os_page_size(void){
  SYSTEM_INFO sysinfo = {0};
  GetSystemInfo(&sysinfo);
  return(sysinfo.dwPageSize);
}

internal U64
os_allocation_granularity(void)
{
  SYSTEM_INFO sysinfo = {0};
  GetSystemInfo(&sysinfo);
  return sysinfo.dwAllocationGranularity;
}

internal U64
os_logical_core_count(void)
{
  SYSTEM_INFO sysinfo = {0};
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
}

////////////////////////////////
//~ rjf: @os_hooks Process Info (Implemented Per-OS)

internal String8List
os_get_command_line_arguments(void)
{
  return w32_cmd_line_args;
}

internal S32
os_get_pid(void){
  DWORD id = GetCurrentProcessId();
  return((S32)id);
}

internal S32
os_get_tid(void){
  DWORD id = GetCurrentThreadId();
  return((S32)id);
}

internal String8List
os_get_environment(void)
{
  return w32_environment;
}

internal U64
os_string_list_from_system_path(Arena *arena, OS_SystemPath path, String8List *out){
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 result = 0;
  
  switch (path){
    case OS_SystemPath_Binary:
    {
      local_persist B32 first = 1;
      local_persist String8 name = {0};
      
      // TODO(allen): let's just pre-compute this at init and skip the complexity
      EnterCriticalSection(&w32_mutex);
      if (first){
        first = 0;
        DWORD size = KB(32);
        U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
        DWORD length = GetModuleFileNameW(0, (WCHAR*)buffer, size);
        String8 name8 = str8_from_16(scratch.arena, str16(buffer, length));
        String8 name_chopped = str8_chop_last_slash(name8);
        name = push_str8_copy(w32_perm_arena, name_chopped);
      }
      LeaveCriticalSection(&w32_mutex);
      
      result = 1;
      str8_list_push(arena, out, name);
    }break;
    
    case OS_SystemPath_Initial:
    {
      Assert(w32_initial_path.str != 0);
      result = 1;
      str8_list_push(arena, out, w32_initial_path);
    }break;
    
    case OS_SystemPath_Current:
    {
      DWORD length = GetCurrentDirectoryW(0, 0);
      U16 *memory = push_array_no_zero(scratch.arena, U16, length + 1);
      length = GetCurrentDirectoryW(length + 1, (WCHAR*)memory);
      String8 name = str8_from_16(arena, str16(memory, length));
      result = 1;
      str8_list_push(arena, out, name);
    }break;
    
    case OS_SystemPath_UserProgramData:
    {
      local_persist B32 first = 1;
      local_persist String8 name = {0};
      if (first){
        first = 0;
        U64 size = KB(32);
        U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
        if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR*)buffer))){
          name = str8_from_16(scratch.arena, str16_cstring(buffer));
          EnterCriticalSection(&w32_mutex);
          U8 *buffer8 = push_array_no_zero(w32_perm_arena, U8, name.size);
          LeaveCriticalSection(&w32_mutex);
          MemoryCopy(buffer8, name.str, name.size);
          name.str = buffer8;
        }
      }
      result = 1;
      str8_list_push(arena, out, name);
    }break;
    
    case OS_SystemPath_ModuleLoad:
    {
      U64 og_count = out->node_count;
      
      {
        UINT cap = GetSystemDirectoryW(0, 0);
        if (cap > 0){
          U16 *buffer = push_array_no_zero(scratch.arena, U16, cap);
          UINT size = GetSystemDirectoryW((WCHAR*)buffer, cap);
          if (size > 0){
            str8_list_push(arena, out, str8_from_16(arena, str16(buffer, size)));
          }
        }
      }
      
      {
        UINT cap = GetWindowsDirectoryW(0, 0);
        if (cap > 0){
          U16 *buffer = push_array_no_zero(scratch.arena, U16, cap);
          UINT size = GetWindowsDirectoryW((WCHAR*)buffer, cap);
          if (size > 0){
            str8_list_push(arena, out, str8_from_16(arena, str16(buffer, size)));
          }
        }
      }
      
      result = out->node_count - og_count;
    }break;
  }
  
  scratch_end(scratch);
  return(result);
}

////////////////////////////////
//~ rjf: @os_hooks Process Control (Implemented Per-OS)

internal void
os_exit_process(S32 exit_code){
  ExitProcess(exit_code);
}

////////////////////////////////
//~ rjf: @os_hooks File System (Implemented Per-OS)

//- rjf: files

internal OS_Handle
os_file_open(OS_AccessFlags flags, String8 path)
{
  OS_Handle result = {0};
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  DWORD access_flags = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;
  if(flags & OS_AccessFlag_Read)    {access_flags |= GENERIC_READ;}
  if(flags & OS_AccessFlag_Write)   {access_flags |= GENERIC_WRITE;}
  if(flags & OS_AccessFlag_Execute) {access_flags |= GENERIC_EXECUTE;}
  if(flags & OS_AccessFlag_Shared)  {share_mode = (!!(flags & OS_AccessFlag_Write)*FILE_SHARE_WRITE)|FILE_SHARE_READ;}
  if(flags & OS_AccessFlag_Write)   {creation_disposition = CREATE_ALWAYS;}
  HANDLE file = CreateFileW((WCHAR *)path16.str, access_flags, share_mode, 0, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if(file != INVALID_HANDLE_VALUE)
  {
    result.u64[0] = (U64)file;
  }
  scratch_end(scratch);
  return result;
}

internal void
os_file_close(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { return; }
  HANDLE handle = (HANDLE)file.u64[0];
  CloseHandle(handle);
}

internal U64
os_file_read(OS_Handle file, Rng1U64 rng, void *out_data)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  HANDLE handle = (HANDLE)file.u64[0];
  
  // rjf: clamp range by file size
  U64 size = 0;
  GetFileSizeEx(handle, (LARGE_INTEGER *)&size);
  Rng1U64 rng_clamped  = r1u64(ClampTop(rng.min, size), ClampTop(rng.max, size));
  U64 total_read_size = 0;
  
  // rjf: read loop
  {
    U64 to_read = dim_1u64(rng_clamped);
    for(U64 off = rng.min; total_read_size < to_read;)
    {
      U64 amt64 = to_read - total_read_size;
      U32 amt32 = u32_from_u64_saturate(amt64);
      DWORD read_size = 0;
      OVERLAPPED overlapped = {0};
      overlapped.Offset     = (off&0x00000000ffffffffull);
      overlapped.OffsetHigh = (off&0xffffffff00000000ull) >> 32;
      ReadFile(handle, (U8 *)out_data + total_read_size, amt32, &read_size, &overlapped);
      off += read_size;
      total_read_size += read_size;
      if(read_size != amt32)
      {
        break;
      }
    }
  }
  
  return total_read_size;
}

internal void
os_file_write(OS_Handle file, Rng1U64 rng, void *data)
{
  if(os_handle_match(file, os_handle_zero())) { return; }
  HANDLE win_handle = (HANDLE)file.u64[0];
  U64 src_off = 0;
  U64 dst_off = rng.min;
  U64 bytes_to_write_total = rng.max-rng.min;
  for(;src_off < bytes_to_write_total;)
  {
    void *bytes_src = (void *)((U8 *)data + src_off);
    U64 bytes_to_write_64 = (bytes_to_write_total-src_off);
    U32 bytes_to_write_32 = u32_from_u64_saturate(bytes_to_write_64);
    U32 bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset     = (dst_off&0x00000000ffffffffull);
    overlapped.OffsetHigh = (dst_off&0xffffffff00000000ull) >> 32;
    BOOL success = WriteFile(win_handle, bytes_src, bytes_to_write_32, (DWORD *)&bytes_written, &overlapped);
    if(success == 0)
    {
      break;
    }
    src_off += bytes_written;
    dst_off += bytes_written;
  }
}

internal B32
os_file_set_times(OS_Handle file, DateTime time)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
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
os_properties_from_file(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { FileProperties r = {0}; return r; }
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

internal OS_FileID
os_id_from_file(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { OS_FileID r = {0}; return r; }
  OS_FileID result = {0};
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
os_delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  B32 result = DeleteFileW((WCHAR*)path16.str);
  scratch_end(scratch);
  return result;
}

internal B32
os_copy_file_path(String8 dst, String8 src)
{
  Temp scratch = scratch_begin(0, 0);
  String16 dst16 = str16_from_8(scratch.arena, dst);
  String16 src16 = str16_from_8(scratch.arena, src);
  B32 result = CopyFileW((WCHAR*)src16.str, (WCHAR*)dst16.str, 0);
  scratch_end(scratch);
  return result;
}

internal String8
os_full_path_from_path(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  DWORD buffer_size = MAX_PATH + 1;
  U16 *buffer = push_array_no_zero(scratch.arena, U16, buffer_size);
  String16 path16 = str16_from_8(scratch.arena, path);
  DWORD path16_size = GetFullPathNameW((WCHAR*)path16.str, buffer_size, (WCHAR*)buffer, NULL);
  String8 full_path = str8_from_16(arena, str16(buffer, path16_size));
  scratch_end(scratch);
  return full_path;
}

internal B32
os_file_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0,0);
  String16 path16 = str16_from_8(scratch.arena, path);
  DWORD attributes = GetFileAttributesW((WCHAR *)path16.str);
  B32 exists = (attributes != INVALID_FILE_ATTRIBUTES) && !!(~attributes & FILE_ATTRIBUTE_DIRECTORY);
  scratch_end(scratch);
  return exists;
}

//- rjf: file maps

internal OS_Handle
os_file_map_open(OS_AccessFlags flags, OS_Handle file)
{
  OS_Handle map = {0};
  {
    HANDLE file_handle = (HANDLE)file.u64[0];
    DWORD protect_flags = 0;
    {
      switch(flags)
      {
        default:{}break;
        case OS_AccessFlag_Read:
        {protect_flags = PAGE_READONLY;}break;
        case OS_AccessFlag_Write:
        case OS_AccessFlag_Read|OS_AccessFlag_Write:
        {protect_flags = PAGE_READWRITE;}break;
        case OS_AccessFlag_Execute:
        case OS_AccessFlag_Read|OS_AccessFlag_Execute:
        {protect_flags = PAGE_EXECUTE_READ;}break;
        case OS_AccessFlag_Execute|OS_AccessFlag_Write|OS_AccessFlag_Read:
        case OS_AccessFlag_Execute|OS_AccessFlag_Write:
        {protect_flags = PAGE_EXECUTE_READWRITE;}break;
      }
    }
    HANDLE map_handle = CreateFileMappingA(file_handle, 0, protect_flags, 0, 0, 0);
    map.u64[0] = (U64)map_handle;
  }
  return map;
}

internal void
os_file_map_close(OS_Handle map)
{
  HANDLE handle = (HANDLE)map.u64[0];
  CloseHandle(handle);
}

internal void *
os_file_map_view_open(OS_Handle map, OS_AccessFlags flags, Rng1U64 range)
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
      case OS_AccessFlag_Read:
      {
        access_flags = FILE_MAP_READ;
      }break;
      case OS_AccessFlag_Write:
      {
        access_flags = FILE_MAP_WRITE;
      }break;
      case OS_AccessFlag_Read|OS_AccessFlag_Write:
      {
        access_flags = FILE_MAP_ALL_ACCESS;
      }break;
      case OS_AccessFlag_Execute:
      case OS_AccessFlag_Read|OS_AccessFlag_Execute:
      case OS_AccessFlag_Write|OS_AccessFlag_Execute:
      case OS_AccessFlag_Read|OS_AccessFlag_Write|OS_AccessFlag_Execute:
      {
        access_flags = FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE;
      }break;
    }
  }
  void *result = MapViewOfFile(handle, access_flags, off_hi, off_lo, size);
  return result;
}

internal void
os_file_map_view_close(OS_Handle map, void *ptr)
{
  UnmapViewOfFile(ptr);
}

//- rjf: directory iteration

internal OS_FileIter *
os_file_iter_begin(Arena *arena, String8 path, OS_FileIterFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 path_with_wildcard = push_str8_cat(scratch.arena, path, str8_lit("\\*"));
  String16 path16 = str16_from_8(scratch.arena, path_with_wildcard);
  OS_FileIter *iter = push_array(arena, OS_FileIter, 1);
  iter->flags = flags;
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  w32_iter->handle = FindFirstFileW((WCHAR*)path16.str, &w32_iter->find_data);
  scratch_end(scratch);
  return iter;
}

internal B32
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  B32 result = 0;
  OS_FileIterFlags flags = iter->flags;
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  if (!(flags & OS_FileIterFlag_Done) && w32_iter->handle != INVALID_HANDLE_VALUE)
  {
    do
    {
      // check is usable
      B32 usable_file = 1;
      
      WCHAR *file_name = w32_iter->find_data.cFileName;
      DWORD attributes = w32_iter->find_data.dwFileAttributes;
      if (file_name[0] == '.'){
        if (flags & OS_FileIterFlag_SkipHiddenFiles){
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
        if (flags & OS_FileIterFlag_SkipFolders){
          usable_file = 0;
        }
      }
      else{
        if (flags & OS_FileIterFlag_SkipFiles){
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
          iter->flags |= OS_FileIterFlag_Done;
        }
        break;
      }
    }while(FindNextFileW(w32_iter->handle, &w32_iter->find_data));
    
    if (!result){
      iter->flags |= OS_FileIterFlag_Done;
    }
  }
  return result;
}

internal void
os_file_iter_end(OS_FileIter *iter)
{
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  FindClose(w32_iter->handle);
}

//- rjf: directory creation

internal B32
os_make_directory(String8 path)
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
//~ rjf: @os_hooks Shared Memory (Implemented Per-OS)

internal OS_Handle
os_shared_memory_alloc(U64 size, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE file = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                   0,
                                   PAGE_READWRITE,
                                   (U32)((size & 0xffffffff00000000) >> 32),
                                   (U32)((size & 0x00000000ffffffff)),
                                   (WCHAR *)name16.str);
  OS_Handle result = {(U64)file};
  scratch_end(scratch);
  return result;
}

internal OS_Handle
os_shared_memory_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE file = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, (WCHAR *)name16.str);
  OS_Handle result = {(U64)file};
  scratch_end(scratch);
  return result;
}

internal void
os_shared_memory_close(OS_Handle handle)
{
  HANDLE file = (HANDLE)(handle.u64[0]);
  CloseHandle(file);
}

internal void *
os_shared_memory_view_open(OS_Handle handle, Rng1U64 range)
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
os_shared_memory_view_close(OS_Handle handle, void *ptr)
{
  UnmapViewOfFile(ptr);
}

////////////////////////////////
//~ rjf: @os_hooks Time (Implemented Per-OS)

internal OS_UnixTime
os_now_unix(void)
{
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  
  U64 win32_time = ((U64)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
  U64 unix_time64 = ((win32_time - 0x19DB1DED53E8000ULL) / 10000000);
  
  Assert(unix_time64 <= OS_UNIX_TIME_MAX);
  OS_UnixTime unix_time32 = (OS_UnixTime)unix_time64;
  
  return unix_time32;
}

internal DateTime
os_now_universal_time(void){
  SYSTEMTIME systime = {0};
  GetSystemTime(&systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return(result);
}

internal DateTime
os_universal_time_from_local_time(DateTime *date_time){
  SYSTEMTIME systime = {0};
  w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  LocalFileTimeToFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return(result);
}

internal DateTime
os_local_time_from_universal_time(DateTime *date_time){
  SYSTEMTIME systime = {0};
  w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  FileTimeToLocalFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  w32_date_time_from_system_time(&result, &systime);
  return(result);
}

internal U64
os_now_microseconds(void){
  U64 result = 0;
  LARGE_INTEGER large_int_counter;
  if (QueryPerformanceCounter(&large_int_counter)){
    result = (large_int_counter.QuadPart*Million(1))/w32_microsecond_resolution;
  }
  return(result);
}

internal void
os_sleep_milliseconds(U32 msec){
  Sleep(msec);
}

////////////////////////////////
//~ rjf: @os_hooks Child Processes (Implemented Per-OS)

internal B32
os_launch_process(OS_LaunchOptions *options, OS_Handle *handle_out){
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  
  StringJoin join_params = {0};
  join_params.pre = str8_lit("\"");
  join_params.sep = str8_lit("\" \"");
  join_params.post = str8_lit("\"");
  String8 cmd = str8_list_join(scratch.arena, &options->cmd_line, &join_params);
  
  StringJoin join_params2 = {0};
  join_params2.sep = str8_lit("\0");
  join_params2.post = str8_lit("\0");
  B32 use_null_env_arg = 0;
  String8List all_opts = options->env;
  if(options->inherit_env != 0)
  {
    if(all_opts.node_count != 0)
    {
      MemoryZeroStruct(&all_opts);
      for(String8Node *n = options->env.first; n != 0; n = n->next)
      {
        str8_list_push(scratch.arena, &all_opts, n->string);
      }
      for(String8Node *n = w32_environment.first; n != 0; n = n->next)
      {
        str8_list_push(scratch.arena, &all_opts, n->string);
      }
    }
    else
    {
      use_null_env_arg = 1;
    }
  }
  String8 env = {0};
  if(use_null_env_arg == 0)
  {
    env = str8_list_join(scratch.arena, &all_opts, &join_params2);
  }
  
  String16 cmd16 = str16_from_8(scratch.arena, cmd);
  String16 dir16 = str16_from_8(scratch.arena, options->path);
  String16 env16 = {0};
  if(use_null_env_arg == 0)
  {
    env16 = str16_from_8(scratch.arena, env);
  }
  
  DWORD creation_flags = 0;
  if(options->consoleless)
  {
    creation_flags |= CREATE_NO_WINDOW;
  }
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {0};
  if (CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, 0, creation_flags, use_null_env_arg ? 0 : (WCHAR*)env16.str, (WCHAR*)dir16.str,
                     &startup_info, &process_info)){
    if (handle_out == 0){
      CloseHandle(process_info.hProcess);
    }
    CloseHandle(process_info.hThread);
    
    if (handle_out != 0){
      OS_Handle handle_result = {(U64)process_info.hProcess};
      *handle_out = handle_result;
    }
    result = 1;
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
os_process_wait(OS_Handle handle, U64 endt_us){
  HANDLE process = (HANDLE)(handle.u64[0]);
  DWORD sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  DWORD result = WaitForSingleObject(process, sleep_ms);
  return (result == WAIT_OBJECT_0);
}

internal void
os_process_release_handle(OS_Handle handle){
  HANDLE process = (HANDLE)(handle.u64[0]);
  CloseHandle(process);
}

////////////////////////////////
//~ rjf: @os_hooks Threads (Implemented Per-OS)

internal OS_Handle
os_launch_thread(OS_ThreadFunctionType *func, void *ptr, void *params){
  W32_Entity *entity = w32_alloc_entity(W32_EntityKind_Thread);
  entity->reference_mask = 0x3;
  entity->thread.func = func;
  entity->thread.ptr = ptr;
  entity->thread.handle = CreateThread(0, 0, w32_thread_base, entity, 0, &entity->thread.tid);
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_release_thread_handle(OS_Handle thread){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(thread.u64[0]);
  // remove my bit
  LONG result = InterlockedAnd((LONG*)&entity->reference_mask, ~0x1);
  // if the other bit is also gone, free entity
  if ((result & 0x2) == 0){
    w32_free_entity(entity);
  }
}

////////////////////////////////
//~ rjf: @os_hooks Synchronization Primitives (Implemented Per-OS)

//- rjf: mutexes

internal OS_Handle
os_mutex_alloc(void){
  W32_Entity *entity = w32_alloc_entity(W32_EntityKind_Mutex);
  InitializeCriticalSection(&entity->mutex);
  
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_mutex_release(OS_Handle mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  w32_free_entity(entity);
}

internal void
os_mutex_take_(OS_Handle mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  EnterCriticalSection(&entity->mutex);
}

internal void
os_mutex_drop_(OS_Handle mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
  LeaveCriticalSection(&entity->mutex);
}

//- rjf: reader/writer mutexes

internal OS_Handle
os_rw_mutex_alloc(void){
  W32_Entity *entity = w32_alloc_entity(W32_EntityKind_RWMutex);
  InitializeSRWLock(&entity->rw_mutex);
  
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_rw_mutex_release(OS_Handle rw_mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  w32_free_entity(entity);
}

internal void
os_rw_mutex_take_r_(OS_Handle rw_mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  AcquireSRWLockShared(&entity->rw_mutex);
}

internal void
os_rw_mutex_drop_r_(OS_Handle rw_mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  ReleaseSRWLockShared(&entity->rw_mutex);
}

internal void
os_rw_mutex_take_w_(OS_Handle rw_mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  AcquireSRWLockExclusive(&entity->rw_mutex);
}

internal void
os_rw_mutex_drop_w_(OS_Handle rw_mutex){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  ReleaseSRWLockExclusive(&entity->rw_mutex);
}

//- rjf: condition variables

internal OS_Handle
os_condition_variable_alloc(void){
  W32_Entity *entity = w32_alloc_entity(W32_EntityKind_ConditionVariable);
  InitializeConditionVariable(&entity->cv);
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_condition_variable_release(OS_Handle cv){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  w32_free_entity(entity);
}

internal B32
os_condition_variable_wait_(OS_Handle cv, OS_Handle mutex, U64 endt_us){
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if (sleep_ms > 0){
    W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
    W32_Entity *mutex_entity = (W32_Entity*)PtrFromInt(mutex.u64[0]);
    result = SleepConditionVariableCS(&entity->cv, &mutex_entity->mutex, sleep_ms);
  }
  return(result);
}

internal B32
os_condition_variable_wait_rw_r_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us){
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if (sleep_ms > 0){
    W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
    W32_Entity *mutex_entity = (W32_Entity*)PtrFromInt(mutex_rw.u64[0]);
    result = SleepConditionVariableSRW(&entity->cv, &mutex_entity->rw_mutex, sleep_ms,
                                       CONDITION_VARIABLE_LOCKMODE_SHARED);
  }
  return(result);
}

internal B32
os_condition_variable_wait_rw_w_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us){
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if (sleep_ms > 0){
    W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
    W32_Entity *mutex_entity = (W32_Entity*)PtrFromInt(mutex_rw.u64[0]);
    result = SleepConditionVariableSRW(&entity->cv, &mutex_entity->rw_mutex, sleep_ms, 0);
  }
  return(result);
}

internal void
os_condition_variable_signal_(OS_Handle cv){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  WakeConditionVariable(&entity->cv);
}

internal void
os_condition_variable_broadcast_(OS_Handle cv){
  W32_Entity *entity = (W32_Entity*)PtrFromInt(cv.u64[0]);
  WakeAllConditionVariable(&entity->cv);
}

//- rjf: cross-process semaphores

internal OS_Handle
os_semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE handle = CreateSemaphoreW(0, initial_count, max_count, (WCHAR *)name16.str);
  OS_Handle result = {(U64)handle};
  scratch_end(scratch);
  return result;
}

internal void
os_semaphore_release(OS_Handle semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  CloseHandle(handle);
}

internal OS_Handle
os_semaphore_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String16 name16 = str16_from_8(scratch.arena, name);
  HANDLE handle = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS , 0, (WCHAR *)name16.str);
  OS_Handle result = {(U64)handle};
  scratch_end(scratch);
  return result;
}

internal void
os_semaphore_close(OS_Handle semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  CloseHandle(handle);
}

internal B32
os_semaphore_take(OS_Handle semaphore, U64 endt_us)
{
  U32 sleep_ms = w32_sleep_ms_from_endt_us(endt_us);
  HANDLE handle = (HANDLE)semaphore.u64[0];
  DWORD wait_result = WaitForSingleObject(handle, sleep_ms);
  B32 result = (wait_result == WAIT_OBJECT_0);
  return result;
}

internal void
os_semaphore_drop(OS_Handle semaphore)
{
  HANDLE handle = (HANDLE)semaphore.u64[0];
  ReleaseSemaphore(handle, 1, 0);
}

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

internal OS_Handle
os_library_open(String8 path){
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  HMODULE mod = LoadLibraryW((LPCWSTR)path16.str);
  OS_Handle result = { (U64)mod };
  scratch_end(scratch);
  return(result);
}

internal VoidProc*
os_library_load_proc(OS_Handle lib, String8 name){
  Temp scratch = scratch_begin(0, 0);
  HMODULE mod = (HMODULE)lib.u64[0];
  name = push_str8_copy(scratch.arena, name);
  VoidProc *result = (VoidProc*)GetProcAddress(mod, (LPCSTR)name.str);
  scratch_end(scratch);
  return(result);
}

internal void
os_library_close(OS_Handle lib){
  HMODULE mod = (HMODULE)lib.u64[0];
  FreeLibrary(mod);
}

////////////////////////////////
//~ rjf: @os_hooks Safe Calls (Implemented Per-OS)

internal void
os_safe_call(OS_ThreadFunctionType *func, OS_ThreadFunctionType *fail_handler, void *ptr){
  __try{
    func(ptr);
  }
  __except (EXCEPTION_EXECUTE_HANDLER){
    if (fail_handler != 0){
      fail_handler(ptr);
    }
    ExitProcess(1);
  }
}

////////////////////////////////
//~ rjf: @os_hooks GUIDs (Implemented Per-OS)

internal OS_Guid
os_make_guid(void)
{
  OS_Guid result; MemoryZeroStruct(&result);
  UUID uuid;
  RPC_STATUS rpc_status = UuidCreate(&uuid);
  if (rpc_status == RPC_S_OK) {
    result.data1 = uuid.Data1;
    result.data2 = uuid.Data2;
    result.data3 = uuid.Data3;
    MemoryCopyArray(result.data4, uuid.Data4);
  }
  return result;
}

