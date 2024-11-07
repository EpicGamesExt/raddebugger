// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8List
os_file_search(Arena *arena, String8List dir_list, String8 file_path)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  String8List match_list; MemoryZeroStruct(&match_list);

  if (os_file_path_exists(file_path)) {
    String8 str = push_str8_copy(arena, file_path);
    str8_list_push(arena, &match_list, str);
  }

  PathStyle file_path_style = path_style_from_str8(file_path);
  B32 is_relative = file_path_style != PathStyle_WindowsAbsolute &&
                    file_path_style != PathStyle_UnixAbsolute;

  if (is_relative) {
    for (String8Node *i = dir_list.first; i != 0; i = i->next) {
      String8List path_list = {0};
      str8_list_push(scratch.arena, &path_list, i->string);
      str8_list_push(scratch.arena, &path_list, file_path);
      String8 path = str8_path_list_join_by_style(scratch.arena, &path_list, PathStyle_SystemAbsolute);
      B32 file_exists = os_file_path_exists(path);
      if (file_exists) {
        B32 is_unique = 1;
        OS_FileID file_id = os_id_from_file_path(path);
        for (String8Node *k = match_list.first; k != 0; k = k->next) {
          OS_FileID test_id = os_id_from_file_path(k->string);
          int cmp = os_file_id_compare(test_id, file_id) != 0;
          if (cmp == 0) {
            is_unique = 0;
            break;
          }
        }
        if (is_unique) {
          String8 str = push_str8_copy(arena, path);
          str8_list_push(arena, &match_list, str);
        }
      }
    }
  }

  scratch_end(scratch);
  ProfEnd();
  return match_list;
}


