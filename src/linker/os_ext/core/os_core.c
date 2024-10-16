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

static struct
{
  String8         string;
  OperatingSystem os;
} g_os_map[] = {
  { str8_lit_comp("windows"), OperatingSystem_Windows, },
  { str8_lit_comp("linux"),   OperatingSystem_Linux,   },
  { str8_lit_comp("mac"),     OperatingSystem_Mac,     },
};

internal OperatingSystem
operating_system_from_string(String8 string)
{
  for (U64 i = 0; i < ArrayCount(g_os_map); ++i) {
    if (str8_match(g_os_map[i].string, string, StringMatchFlag_CaseInsensitive)) {
      return g_os_map[i].os;
    }
  }
  return OperatingSystem_Null;
}

internal B32
os_try_guid_from_string(String8 string, OS_Guid *guid_out)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_parsed = 0;
  String8List list = str8_split_by_string_chars(scratch.arena, string, str8_lit("-"), StringSplitFlag_KeepEmpties);
  if (list.node_count == 5) {
    String8 data1_str    = list.first->string;
    String8 data2_str    = list.first->next->string;
    String8 data3_str    = list.first->next->next->string;
    String8 data4_hi_str = list.first->next->next->next->string;
    String8 data4_lo_str = list.first->next->next->next->next->string;
    if (str8_is_integer(data1_str, 16) && 
        str8_is_integer(data2_str, 16) &&
        str8_is_integer(data3_str, 16) &&
        str8_is_integer(data4_hi_str, 16) &&
        str8_is_integer(data4_lo_str, 16)) {
      U64 data1    = u64_from_str8(data1_str, 16);
      U64 data2    = u64_from_str8(data2_str, 16);
      U64 data3    = u64_from_str8(data3_str, 16);
      U64 data4_hi = u64_from_str8(data4_hi_str, 16);
      U64 data4_lo = u64_from_str8(data4_lo_str, 16);
      if (data1 <= max_U32 &&
          data2 <= max_U16 &&
          data3 <= max_U16 &&
          data4_hi <= max_U16 &&
          data4_lo <= 0xffffffffffff) {
        guid_out->data1 = (U32)data1;
        guid_out->data2 = (U16)data2;
        guid_out->data3 = (U16)data3;
        U64 data4 = (data4_hi << 48) | data4_lo;
        MemoryCopy(&guid_out->data4[0], &data4, sizeof(data4));
        is_parsed = 1;
      }
    }
  }
  scratch_end(scratch);
  return is_parsed;
}

internal OS_Guid
os_guid_from_string(String8 string)
{
  OS_Guid guid = {0};
  os_try_guid_from_string(string, &guid);
  return guid;
}
