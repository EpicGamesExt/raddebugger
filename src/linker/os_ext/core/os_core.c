// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal
THREAD_POOL_TASK_FUNC(os_data_size_from_file_path_task)
{
  OS_DataSizeFromFilePathTask *task = raw_task;

  String8        path   = task->path_arr.v[task_id];
  OS_Handle      handle = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, path);
  FileProperties props  = os_properties_from_file(handle);

  task->handle_arr[task_id] = handle;
  task->size_arr[task_id]   = props.size;
}

internal
THREAD_POOL_TASK_FUNC(os_data_from_file_path_task)
{
  OS_DataFromFilePathTask *task = raw_task;

  OS_Handle handle = task->handle_arr[task_id];
  U64       size   = task->size_arr[task_id];
  U8       *buffer = task->buffer + task->off_arr[task_id];

  U64 read_size = os_file_read(handle, rng_1u64(0, size), buffer);
  Assert(read_size == size);

  task->data_arr.v[task_id] = str8(buffer, read_size);

  os_file_close(handle);
}

internal String8Array
os_data_from_file_path_parallel(TP_Context *tp, Arena *arena, String8Array path_arr)
{
  Temp scratch = scratch_begin(&arena,1);

  OS_Handle *handle_arr = push_array_no_zero(scratch.arena, OS_Handle, path_arr.count);
  U64       *size_arr   = push_array_no_zero(scratch.arena, U64, path_arr.count);
  U64       *off_arr    = push_array_no_zero(scratch.arena, U64, path_arr.count);

  // open handles and get sizes
  OS_DataSizeFromFilePathTask sizer;
  sizer.path_arr   = path_arr;
  sizer.size_arr   = size_arr;
  sizer.handle_arr = handle_arr;
  tp_for_parallel(tp, 0, path_arr.count, os_data_size_from_file_path_task, &sizer);

  // compute file buffer size
  U64 total_data_size = sum_array_u64(path_arr.count, sizer.size_arr);

  // assign offsets into file buffer
  MemoryCopyTyped(off_arr, sizer.size_arr, path_arr.count);
  counts_to_offsets_array_u64(path_arr.count, off_arr);

  // read files and close handles
  OS_DataFromFilePathTask reader;
  reader.data_arr   = str8_array_reserve(arena, path_arr.count);
  reader.handle_arr = handle_arr;
  reader.size_arr   = size_arr;;
  reader.off_arr    = off_arr;
  reader.buffer     = push_array_no_zero(arena, U8, total_data_size);
  tp_for_parallel(tp, 0, path_arr.count, os_data_from_file_path_task, &reader);
  
  String8Array result = {0};
  result.count = path_arr.count;
  result.v     = reader.data_arr.v;

  scratch_end(scratch);
  return result;
}

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


