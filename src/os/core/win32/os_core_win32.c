// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Modern Windows SDK Functions
//
// (We must dynamically link to them, since they can be missing in older SDKs)

typedef HRESULT W32_SetThreadDescription_Type(HANDLE hThread, PCWSTR lpThreadDescription);
global W32_SetThreadDescription_Type *w32_SetThreadDescription_func = 0;

////////////////////////////////
//~ rjf: File Info Conversion Helpers

internal FilePropertyFlags
os_w32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes)
{
  FilePropertyFlags flags = 0;
  if(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    flags |= FilePropertyFlag_IsFolder;
  }
  return flags;
}

internal void
os_w32_file_properties_from_attribute_data(FileProperties *properties, WIN32_FILE_ATTRIBUTE_DATA *attributes)
{
  properties->size = Compose64Bit(attributes->nFileSizeHigh, attributes->nFileSizeLow);
  os_w32_dense_time_from_file_time(&properties->created, &attributes->ftCreationTime);
  os_w32_dense_time_from_file_time(&properties->modified, &attributes->ftLastWriteTime);
  properties->flags = os_w32_file_property_flags_from_dwFileAttributes(attributes->dwFileAttributes);
}

////////////////////////////////
//~ rjf: Time Conversion Helpers

internal void
os_w32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in)
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
os_w32_system_time_from_date_time(SYSTEMTIME *out, DateTime *in)
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
os_w32_dense_time_from_file_time(DenseTime *out, FILETIME *in)
{
  SYSTEMTIME systime = {0};
  FileTimeToSystemTime(in, &systime);
  DateTime date_time = {0};
  os_w32_date_time_from_system_time(&date_time, &systime);
  *out = dense_time_from_date_time(date_time);
}

internal U32
os_w32_sleep_ms_from_endt_us(U64 endt_us)
{
  U32 sleep_ms = 0;
  if(endt_us == max_U64)
  {
    sleep_ms = INFINITE;
  }
  else
  {
    U64 begint = os_now_microseconds();
    if(begint < endt_us)
    {
      U64 sleep_us = endt_us - begint;
      sleep_ms = (U32)((sleep_us + 999)/1000);
    }
  }
  return sleep_ms;
}

////////////////////////////////
//~ rjf: Entity Functions

internal OS_W32_Entity *
os_w32_entity_alloc(OS_W32_EntityKind kind)
{
  OS_W32_Entity *result = 0;
  EnterCriticalSection(&os_w32_state.entity_mutex);
  {
    result = os_w32_state.entity_free;
    if(result)
    {
      SLLStackPop(os_w32_state.entity_free);
    }
    else
    {
      result = push_array_no_zero(os_w32_state.entity_arena, OS_W32_Entity, 1);
    }
    MemoryZeroStruct(result);
  }
  LeaveCriticalSection(&os_w32_state.entity_mutex);
  result->kind = kind;
  return result;
}

internal void
os_w32_entity_release(OS_W32_Entity *entity)
{
  entity->kind = OS_W32_EntityKind_Null;
  EnterCriticalSection(&os_w32_state.entity_mutex);
  SLLStackPush(os_w32_state.entity_free, entity);
  LeaveCriticalSection(&os_w32_state.entity_mutex);
}

////////////////////////////////
//~ rjf: Thread Entry Point

internal DWORD
os_w32_thread_entry_point(void *ptr)
{
  OS_W32_Entity *entity = (OS_W32_Entity *)ptr;
  OS_ThreadFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  func(thread_ptr);
  tctx_release();
  return 0;
}

////////////////////////////////
//~ rjf: @os_hooks System/Process Info (Implemented Per-OS)

internal OS_SystemInfo *
os_get_system_info(void)
{
  return &os_w32_state.system_info;
}

internal OS_ProcessInfo *
os_get_process_info(void)
{
  return &os_w32_state.process_info;
}

internal String8
os_get_current_path(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  DWORD length = GetCurrentDirectoryW(0, 0);
  U16 *memory = push_array_no_zero(scratch.arena, U16, length + 1);
  length = GetCurrentDirectoryW(length + 1, (WCHAR*)memory);
  String8 name = str8_from_16(arena, str16(memory, length));
  scratch_end(scratch);
  return name;
}

////////////////////////////////
//~ rjf: @os_hooks Memory Allocation (Implemented Per-OS)

//- rjf: basic

internal void *
os_reserve(U64 size)
{
  void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
  return result;
}

internal B32
os_commit(void *ptr, U64 size)
{
  B32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
  return result;
}

internal void
os_decommit(void *ptr, U64 size)
{
  VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal void
os_release(void *ptr, U64 size)
{
  // NOTE(rjf): size not used - not necessary on Windows, but necessary for other OSes.
  VirtualFree(ptr, 0, MEM_RELEASE);
}

//- rjf: large pages

internal void *
os_reserve_large(U64 size)
{
  // we commit on reserve because windows
  void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
  return result;
}

internal B32
os_commit_large(void *ptr, U64 size)
{
  return 1;
}

////////////////////////////////
//~ rjf: @os_hooks Thread Info (Implemented Per-OS)

internal U32
os_tid(void)
{
  DWORD id = GetCurrentThreadId();
  return (U32)id;
}

internal void
os_set_thread_name(String8 name)
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
    info.dwThreadID = os_tid();
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
//~ rjf: @os_hooks Aborting (Implemented Per-OS)

internal void
os_abort(S32 exit_code)
{
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
  if(flags & OS_AccessFlag_ShareRead)  {share_mode |= FILE_SHARE_READ;}
  if(flags & OS_AccessFlag_ShareWrite) {share_mode |= FILE_SHARE_WRITE|FILE_SHARE_DELETE;}
  if(flags & OS_AccessFlag_Write)   {creation_disposition = CREATE_ALWAYS;}
  if(flags & OS_AccessFlag_Append)  {creation_disposition = OPEN_ALWAYS;}
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
  BOOL result = CloseHandle(handle);
  (void)result;
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

internal U64
os_file_write(OS_Handle file, Rng1U64 rng, void *data)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  HANDLE win_handle = (HANDLE)file.u64[0];
  U64 src_off = 0;
  U64 dst_off = rng.min;
  U64 bytes_to_write_total = rng.max-rng.min;
  U64 total_bytes_written = 0;
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
    total_bytes_written += bytes_written;
  }
  return total_bytes_written;
}

internal B32
os_file_set_time(OS_Handle file, DateTime time)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  B32 result = 0;
  HANDLE handle = (HANDLE)file.u64[0];
  SYSTEMTIME system_time = {0};
  os_w32_system_time_from_date_time(&system_time, &time);
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
    os_w32_dense_time_from_file_time(&props.modified, &info.ftLastWriteTime);
    os_w32_dense_time_from_file_time(&props.created, &info.ftCreationTime);
    props.flags = os_w32_file_property_flags_from_dwFileAttributes(info.dwFileAttributes);
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

internal FileProperties
os_properties_from_file_path(String8 path)
{
  WIN32_FIND_DATAW find_data = {0};
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  HANDLE handle = FindFirstFileW((WCHAR *)path16.str, &find_data);
  FileProperties props = {0};
  if(handle != INVALID_HANDLE_VALUE)
  {
    props.size = Compose64Bit(find_data.nFileSizeHigh, find_data.nFileSizeLow);
    os_w32_dense_time_from_file_time(&props.created, &find_data.ftCreationTime);
    os_w32_dense_time_from_file_time(&props.modified, &find_data.ftLastWriteTime);
    props.flags = os_w32_file_property_flags_from_dwFileAttributes(find_data.dwFileAttributes);
  }
  FindClose(handle);
  scratch_end(scratch);
  return props;
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
  BOOL result = CloseHandle(handle);
  (void)result;
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
os_file_map_view_close(OS_Handle map, void *ptr, Rng1U64 range)
{
  BOOL result = UnmapViewOfFile(ptr);
  (void)result;
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
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter->memory;
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
    w32_iter->handle = FindFirstFileW((WCHAR*)path16.str, &w32_iter->find_data);
  }
  scratch_end(scratch);
  return iter;
}

internal B32
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  B32 result = 0;
  OS_FileIterFlags flags = iter->flags;
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter->memory;
  switch(w32_iter->is_volume_iter)
  {
    //- rjf: file iteration
    default:
    case 0:
    {
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
            os_w32_dense_time_from_file_time(&info_out->props.created,  &w32_iter->find_data.ftCreationTime);
            os_w32_dense_time_from_file_time(&info_out->props.modified, &w32_iter->find_data.ftLastWriteTime);
            info_out->props.flags = os_w32_file_property_flags_from_dwFileAttributes(attributes);
            result = 1;
            if (!FindNextFileW(w32_iter->handle, &w32_iter->find_data)){
              iter->flags |= OS_FileIterFlag_Done;
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
    iter->flags |= OS_FileIterFlag_Done;
  }
  return result;
}

internal void
os_file_iter_end(OS_FileIter *iter)
{
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter->memory;
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
os_shared_memory_view_close(OS_Handle handle, void *ptr, Rng1U64 range)
{
  UnmapViewOfFile(ptr);
}

////////////////////////////////
//~ rjf: @os_hooks Time (Implemented Per-OS)

internal U64
os_now_microseconds(void)
{
  U64 result = 0;
  LARGE_INTEGER large_int_counter;
  if(QueryPerformanceCounter(&large_int_counter))
  {
    result = (large_int_counter.QuadPart*Million(1))/os_w32_state.microsecond_resolution;
  }
  return result;
}

internal U32
os_now_unix(void)
{
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  U64 win32_time = ((U64)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
  U64 unix_time64 = ((win32_time - 0x19DB1DED53E8000ULL) / 10000000);
  U32 unix_time32 = (U32)unix_time64;
  return unix_time32;
}

internal DateTime
os_now_universal_time(void)
{
  SYSTEMTIME systime = {0};
  GetSystemTime(&systime);
  DateTime result = {0};
  os_w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal DateTime
os_universal_time_from_local(DateTime *date_time)
{
  SYSTEMTIME systime = {0};
  os_w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  LocalFileTimeToFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  os_w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal DateTime
os_local_time_from_universal(DateTime *date_time)
{
  SYSTEMTIME systime = {0};
  os_w32_system_time_from_date_time(&systime, date_time);
  FILETIME ftime = {0};
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local = {0};
  FileTimeToLocalFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = {0};
  os_w32_date_time_from_system_time(&result, &systime);
  return result;
}

internal void
os_sleep_milliseconds(U32 msec)
{
  Sleep(msec);
}

////////////////////////////////
//~ rjf: @os_hooks Child Processes (Implemented Per-OS)

internal OS_Handle
os_process_launch(OS_ProcessLaunchParams *params)
{
  OS_Handle result = {0};
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
        for(String8Node *n = os_w32_state.process_info.environment.first; n != 0; n = n->next)
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
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {0};
  if(CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, 0, creation_flags, use_null_env_arg ? 0 : (WCHAR*)env16.str, (WCHAR*)dir16.str, &startup_info, &process_info))
  {
    result.u64[0] = (U64)process_info.hProcess;
    CloseHandle(process_info.hThread);
  }
  
  scratch_end(scratch);
  return result;
}

internal B32
os_process_join(OS_Handle handle, U64 endt_us)
{
  HANDLE process = (HANDLE)(handle.u64[0]);
  DWORD sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
  DWORD result = WaitForSingleObject(process, sleep_ms);
  return (result == WAIT_OBJECT_0);
}

internal void
os_process_detach(OS_Handle handle)
{
  HANDLE process = (HANDLE)(handle.u64[0]);
  CloseHandle(process);
}

////////////////////////////////
//~ rjf: @os_hooks Threads (Implemented Per-OS)

internal OS_Handle
os_thread_launch(OS_ThreadFunctionType *func, void *ptr, void *params)
{
  OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_EntityKind_Thread);
  entity->thread.func = func;
  entity->thread.ptr = ptr;
  entity->thread.handle = CreateThread(0, 0, os_w32_thread_entry_point, entity, 0, &entity->thread.tid);
  OS_Handle result = {IntFromPtr(entity)};
  return result;
}

internal B32
os_thread_join(OS_Handle handle, U64 endt_us)
{
  DWORD sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
  OS_W32_Entity *entity = (OS_W32_Entity *)PtrFromInt(handle.u64[0]);
  DWORD wait_result = WAIT_OBJECT_0;
  if(entity != 0)
  {
    wait_result = WaitForSingleObject(entity->thread.handle, sleep_ms);
  }
  os_w32_entity_release(entity);
  return (wait_result == WAIT_OBJECT_0);
}

internal void
os_thread_detach(OS_Handle thread)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(thread.u64[0]);
  os_w32_entity_release(entity);
}

////////////////////////////////
//~ rjf: @os_hooks Synchronization Primitives (Implemented Per-OS)

//- rjf: mutexes

internal OS_Handle
os_mutex_alloc(void)
{
  OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_EntityKind_Mutex);
  InitializeCriticalSection(&entity->mutex);
  OS_Handle result = {IntFromPtr(entity)};
  return result;
}

internal void
os_mutex_release(OS_Handle mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(mutex.u64[0]);
  os_w32_entity_release(entity);
}

internal void
os_mutex_take(OS_Handle mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(mutex.u64[0]);
  EnterCriticalSection(&entity->mutex);
}

internal void
os_mutex_drop(OS_Handle mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(mutex.u64[0]);
  LeaveCriticalSection(&entity->mutex);
}

//- rjf: reader/writer mutexes

internal OS_Handle
os_rw_mutex_alloc(void)
{
  OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_EntityKind_RWMutex);
  InitializeSRWLock(&entity->rw_mutex);
  OS_Handle result = {IntFromPtr(entity)};
  return result;
}

internal void
os_rw_mutex_release(OS_Handle rw_mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  os_w32_entity_release(entity);
}

internal void
os_rw_mutex_take_r(OS_Handle rw_mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  AcquireSRWLockShared(&entity->rw_mutex);
}

internal void
os_rw_mutex_drop_r(OS_Handle rw_mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  ReleaseSRWLockShared(&entity->rw_mutex);
}

internal void
os_rw_mutex_take_w(OS_Handle rw_mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  AcquireSRWLockExclusive(&entity->rw_mutex);
}

internal void
os_rw_mutex_drop_w(OS_Handle rw_mutex)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(rw_mutex.u64[0]);
  ReleaseSRWLockExclusive(&entity->rw_mutex);
}

//- rjf: condition variables

internal OS_Handle
os_condition_variable_alloc(void)
{
  OS_W32_Entity *entity = os_w32_entity_alloc(OS_W32_EntityKind_ConditionVariable);
  InitializeConditionVariable(&entity->cv);
  OS_Handle result = {IntFromPtr(entity)};
  return result;
}

internal void
os_condition_variable_release(OS_Handle cv)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
  os_w32_entity_release(entity);
}

internal B32
os_condition_variable_wait(OS_Handle cv, OS_Handle mutex, U64 endt_us)
{
  U32 sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if(sleep_ms > 0)
  {
    OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
    OS_W32_Entity *mutex_entity = (OS_W32_Entity*)PtrFromInt(mutex.u64[0]);
    result = SleepConditionVariableCS(&entity->cv, &mutex_entity->mutex, sleep_ms);
  }
  return result;
}

internal B32
os_condition_variable_wait_rw_r(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  U32 sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if(sleep_ms > 0)
  {
    OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
    OS_W32_Entity *mutex_entity = (OS_W32_Entity*)PtrFromInt(mutex_rw.u64[0]);
    result = SleepConditionVariableSRW(&entity->cv, &mutex_entity->rw_mutex, sleep_ms,
                                       CONDITION_VARIABLE_LOCKMODE_SHARED);
  }
  return result;
}

internal B32
os_condition_variable_wait_rw_w(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  U32 sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
  BOOL result = 0;
  if(sleep_ms > 0)
  {
    OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
    OS_W32_Entity *mutex_entity = (OS_W32_Entity*)PtrFromInt(mutex_rw.u64[0]);
    result = SleepConditionVariableSRW(&entity->cv, &mutex_entity->rw_mutex, sleep_ms, 0);
  }
  return result;
}

internal void
os_condition_variable_signal(OS_Handle cv)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
  WakeConditionVariable(&entity->cv);
}

internal void
os_condition_variable_broadcast(OS_Handle cv)
{
  OS_W32_Entity *entity = (OS_W32_Entity*)PtrFromInt(cv.u64[0]);
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
  U32 sleep_ms = os_w32_sleep_ms_from_endt_us(endt_us);
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
os_library_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String16 path16 = str16_from_8(scratch.arena, path);
  HMODULE mod = LoadLibraryW((LPCWSTR)path16.str);
  OS_Handle result = { (U64)mod };
  scratch_end(scratch);
  return result;
}

internal VoidProc*
os_library_load_proc(OS_Handle lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  HMODULE mod = (HMODULE)lib.u64[0];
  name = push_str8_copy(scratch.arena, name);
  VoidProc *result = (VoidProc*)GetProcAddress(mod, (LPCSTR)name.str);
  scratch_end(scratch);
  return result;
}

internal void
os_library_close(OS_Handle lib)
{
  HMODULE mod = (HMODULE)lib.u64[0];
  FreeLibrary(mod);
}

////////////////////////////////
//~ rjf: @os_hooks Safe Calls (Implemented Per-OS)

internal void
os_safe_call(OS_ThreadFunctionType *func, OS_ThreadFunctionType *fail_handler, void *ptr)
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
//~ rjf: @os_hooks Entry Points (Implemented Per-OS)

#include <dbghelp.h>
#undef OS_WINDOWS // shlwapi uses its own OS_WINDOWS include inside
#include <shlwapi.h>

internal B32 win32_g_is_quiet = 0;

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
    
    if(dbg_SymSetOptions && dbg_SymInitializeW && dbg_StackWalk64 && dbg_SymFunctionTableAccess64 && dbg_SymGetModuleBase64 && dbg_SymFromAddrW && dbg_SymGetLineFromAddrW64 && dbg_SymGetModuleInfoW64)
    {
      HANDLE process = GetCurrentProcess();
      HANDLE thread = GetCurrentThread();
      CONTEXT* context = exception_ptrs->ContextRecord;
      
      dbg_SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
      if(dbg_SymInitializeW(process, L"", TRUE))
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
          frame.AddrPC.Offset = context->Rip;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context->Rbp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context->Rsp;
          frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_ARM64)
          image_type = IMAGE_FILE_MACHINE_ARM64;
          frame.AddrPC.Offset = context->Pc;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context->Fp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context->Sp;
          frame.AddrStack.Mode = AddrModeFlat;
#else
#  error Architecture not supported!
#endif
          
          for(U32 idx=0; ;idx++)
          {
            const U32 max_frames = 32;
            if(idx == max_frames)
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"...");
              break;
            }
            
            if(!dbg_StackWalk64(image_type, process, thread, &frame, context, 0, dbg_SymFunctionTableAccess64, dbg_SymGetModuleBase64, 0))
            {
              break;
            }
            
            U64 address = frame.AddrPC.Offset;
            if(address == 0)
            {
              break;
            }
            
            if(idx==0)
            {
#if BUILD_CONSOLE_INTERFACE
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\nCreate a new issue with this report at %S.\n\n", BUILD_ISSUES_LINK_STRING_LITERAL);
#else
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen,
                                   L"\nPress Ctrl+C to copy this text to clipboard, then create a new issue at\n"
                                   L"<a href=\"%S\">%S</a>\n\n", BUILD_ISSUES_LINK_STRING_LITERAL, BUILD_ISSUES_LINK_STRING_LITERAL);
#endif
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"Call stack:\n");
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"%u. [0x%I64x]", idx + 1, address);
            
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
  
#if BUILD_CONSOLE_INTERFACE
  fwprintf(stderr, L"\n--- Fatal Exception ---\n");
  fwprintf(stderr, L"%s\n\n", buffer);
#else
  TASKDIALOGCONFIG dialog = {0};
  dialog.cbSize = sizeof(dialog);
  dialog.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
  dialog.pszMainIcon = TD_ERROR_ICON;
  dialog.dwCommonButtons = TDCBF_CLOSE_BUTTON;
  dialog.pszWindowTitle = L"Fatal Exception";
  dialog.pszContent = buffer;
  dialog.pfCallback = &win32_dialog_callback;
  TaskDialogIndirect(&dialog, 0, 0, 0);
#endif
  
  ExitProcess(1);
}

#undef OS_WINDOWS // shlwapi uses its own OS_WINDOWS include inside
#define OS_WINDOWS 1

internal void
w32_entry_point_caller(int argc, WCHAR **wargv)
{
  SetUnhandledExceptionFilter(&win32_exception_filter);
  
  //- rjf: do OS layer initialization
  {
    // rjf: dynamically load windows functions which are not guaranteed
    // in all SDKs
    {
      HMODULE module = LoadLibraryA("kernel32.dll");
      w32_SetThreadDescription_func = (W32_SetThreadDescription_Type *)GetProcAddress(module, "SetThreadDescription");
      FreeLibrary(module);
    }
    
    // rjf: try to enable large pages if we can
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
          AdjustTokenPrivileges(token, 0, &priv, sizeof(priv), 0, 0);
        }
        CloseHandle(token);
      }
    }
    
    // rjf: get system info
    SYSTEM_INFO sysinfo = {0};
    GetSystemInfo(&sysinfo);
    
    // rjf: set up non-dynamically-alloc'd state
    //
    // (we need to set up some basics before this layer can supply
    // memory allocation primitives)
    {
      os_w32_state.microsecond_resolution  = 1;
      LARGE_INTEGER large_int_resolution;
      if(QueryPerformanceFrequency(&large_int_resolution))
      {
        os_w32_state.microsecond_resolution = large_int_resolution.QuadPart;
      }
    }
    {
      OS_SystemInfo *info = &os_w32_state.system_info;
      info->logical_processor_count = (U64)sysinfo.dwNumberOfProcessors;
      info->page_size               = sysinfo.dwPageSize;
      info->large_page_size         = GetLargePageMinimum();
      info->allocation_granularity  = sysinfo.dwAllocationGranularity;
    }
    {
      OS_ProcessInfo *info = &os_w32_state.process_info;
      info->pid = GetCurrentProcessId();
    }
    
    // rjf: set up thread context
    local_persist TCTX tctx;
    tctx_init_and_equip(&tctx);
    
    // rjf: set up dynamically-alloc'd state
    Arena *arena = arena_alloc();
    {
      os_w32_state.arena = arena;
      {
        OS_SystemInfo *info = &os_w32_state.system_info;
        U8 buffer[MAX_COMPUTERNAME_LENGTH + 1] = {0};
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if(GetComputerNameA((char*)buffer, &size))
        {
          info->machine_name = push_str8_copy(arena, str8(buffer, size));
        }
      }
    }
    {
      OS_ProcessInfo *info = &os_w32_state.process_info;
      {
        Temp scratch = scratch_begin(0, 0);
        DWORD size = KB(32);
        U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
        DWORD length = GetModuleFileNameW(0, (WCHAR*)buffer, size);
        String8 name8 = str8_from_16(scratch.arena, str16(buffer, length));
        String8 name_chopped = str8_chop_last_slash(name8);
        info->binary_path = push_str8_copy(arena, name_chopped);
        scratch_end(scratch);
      }
      info->initial_path = os_get_current_path(arena);
      {
        Temp scratch = scratch_begin(0, 0);
        U64 size = KB(32);
        U16 *buffer = push_array_no_zero(scratch.arena, U16, size);
        if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR*)buffer)))
        {
          info->user_program_data_path = str8_from_16(arena, str16_cstring(buffer));
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
              str8_list_push(arena, &info->environment, string);
              start_idx = idx+1;
            }
          }
        }
      }
    }
    
    // rjf: set up entity storage
    InitializeCriticalSection(&os_w32_state.entity_mutex);
    os_w32_state.entity_arena = arena_alloc();
  }
  
  //- rjf: extract arguments
  Arena *args_arena = arena_alloc(.reserve_size = MB(1), .commit_size = KB(32));
  char **argv = push_array(args_arena, char *, argc);
  for(int i = 0; i < argc; i += 1)
  {
    String16 arg16 = str16_cstring((U16 *)wargv[i]);
    String8 arg8 = str8_from_16(args_arena, arg16);
    if(str8_match(arg8, str8_lit("--quiet"), StringMatchFlag_CaseInsensitive))
    {
      win32_g_is_quiet = 1;
    }
    argv[i] = (char *)arg8.str;
  }
  
  //- rjf: call into "real" entry point
  main_thread_base_entry_point(entry_point, argv, (U64)argc);
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
  w32_entry_point_caller(__argc, __wargv);
  return 0;
}
#endif
