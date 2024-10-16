// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U32
w32_unix_time_from_file_time(FILETIME file_time)
{
  U64 win32_time = ((U64)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
  U64 unix_time64 = ((win32_time - 0x19DB1DED53E8000ULL) / 10000000);
  
  Assert(unix_time64 <= max_U32);
  U32 unix_time32 = (U32)unix_time64;

  return unix_time32;
}

internal B32
os_w32_has_path_volume_prefix(String8 path)
{
  if (path.size >= 2) {
    U8 *ptr = path.str;
    U8 *opl = path.str + path.size;
    UnicodeDecode a = utf8_decode(ptr, (U64)(opl-ptr));
    ptr += a.inc;
    UnicodeDecode b = utf8_decode(ptr, (U64)(opl-ptr));
    return a.codepoint < max_U8 && char_is_alpha(a.codepoint) && b.codepoint == ':';
  }
  return 0;
}

internal B32
os_w32_has_device_prefix(String8 path)
{
  if (path.size >= 3) {
    U8 *ptr = path.str;
    U8 *opl = path.str + path.size;
    UnicodeDecode a = utf8_decode(ptr, (U64)(opl-ptr));
    ptr += a.inc;
    UnicodeDecode b = utf8_decode(ptr, (U64)(opl-ptr));
    ptr += b.inc;
    UnicodeDecode c = utf8_decode(ptr, (U64)(opl-ptr));
    return a.codepoint == '\\' && b.codepoint == '\\' && (c.codepoint == '?' || c.codepoint == '.');
  }
  return 0;
}

internal B32
os_w32_has_unc_prefix(String8 path)
{
  if (path.size >= 2) {
    U8 *ptr = path.str;
    U8 *opl = path.str + path.size;
    UnicodeDecode a = utf8_decode(ptr, (U64)(opl-ptr));
    ptr += a.inc;
    UnicodeDecode b = utf8_decode(ptr, (U64)(opl-ptr));
    return a.codepoint == '\\' && b.codepoint == '\\';
  }
  return 0;
}

internal B32
os_w32_has_root_drive_prefix(String8 path)
{
  if (path.size >= 1) {
    UnicodeDecode a = utf8_decode(path.str, path.size);
    return a.codepoint == '\\';
  }
  return 0;
}

internal B32
os_w32_is_path_relative_current_directory(String8 path)
{
  if (os_w32_has_path_volume_prefix(path)) {
    return 0;
  }
  if (os_w32_has_device_prefix(path)) {
    return 0;
  }
  if (os_w32_has_unc_prefix(path)) {
    return 0;
  }
  if (os_w32_has_root_drive_prefix(path)) {
    return 0;
  }
  return 1;
}

internal String8
os_make_full_path(Arena *arena, String8 path)
{
  String8 full_path;
  if (os_w32_is_path_relative_current_directory(path)) {
    Temp scratch = scratch_begin(&arena, 1);
    String8 current_dir = os_get_current_path(scratch.arena);
    String8List list = {0};
    str8_list_push(scratch.arena, &list, current_dir);
    str8_list_push(scratch.arena, &list, path);
    String8 temp_full_path = str8_list_join(scratch.arena, &list, &(StringJoin){ .sep = str8_lit_comp("\\") });
    String8List split_full_path = str8_split_path(scratch.arena, temp_full_path);
    str8_path_list_resolve_dots_in_place(&split_full_path, PathStyle_WindowsAbsolute);
    full_path = str8_list_join(arena, &split_full_path, &(StringJoin){ .sep = str8_lit_comp("\\") });
    scratch_end(scratch);
  } else {
    full_path = push_str8_copy(arena, path);
  }
  return full_path;
}

internal B32
os_folder_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0,0);

  String8 actual_path = path;
  if (os_w32_is_path_relative_current_directory(path)) {
    String8 current = os_get_current_path(scratch.arena);
    String8List list = {0};
    str8_list_push(scratch.arena, &list, current);
    str8_list_push(scratch.arena, &list, path);
    StringJoin join = { .sep = str8_lit_comp("\\") };
    actual_path  =str8_list_join(scratch.arena, &list, &join);
  }

  String16 path16 = str16_from_8(scratch.arena, actual_path);
  DWORD attributes = GetFileAttributesW((WCHAR *)path16.str);
  B32 exists = (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
  scratch_end(scratch);
  return exists;
}

internal B32
os_set_large_pages(B32 toggle)
{
  B32 is_ok = 0;

  HANDLE token;
  if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
  {
    LUID luid;
    if(LookupPrivilegeValue(0, SE_LOCK_MEMORY_NAME, &luid))
    {
      TOKEN_PRIVILEGES priv;
      priv.PrivilegeCount           = 1;
      priv.Privileges[0].Luid       = luid;
      priv.Privileges[0].Attributes = toggle ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;
      if (AdjustTokenPrivileges(token, 0, &priv, sizeof(priv), 0, 0) == ERROR_SUCCESS) {
        is_ok = 1;
      }
    }
    CloseHandle(token);
  }
  return is_ok;
}

internal U32
os_get_process_start_time_unix(void)
{
  HANDLE handle = GetCurrentProcess();
  FILETIME start_time = {0};
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  if (GetProcessTimes(handle, &start_time, &exit_time, &kernel_time, &user_time)) {
    return w32_unix_time_from_file_time(start_time);
  }
  return 0;
}

