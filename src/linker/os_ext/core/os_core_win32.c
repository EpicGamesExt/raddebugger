// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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

