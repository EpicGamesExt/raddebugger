////////////////////////////////
// Shared File API

shared_function int
lnk_open_file_read(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max)
{
  OS_Handle handle = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, str8((U8*)path, path_size));
  Assert(sizeof(handle) <= handle_buffer_max);
  MemoryCopy(handle_buffer, &handle, sizeof(handle));
  return !os_handle_match(handle, os_handle_zero());
}

shared_function int
lnk_open_file_write(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max)
{
  OS_Handle handle = os_file_open(OS_AccessFlag_Write, str8((U8*)path, path_size));
  Assert(sizeof(handle) <= handle_buffer_max);
  MemoryCopy(handle_buffer, &handle, sizeof(handle));
  return !os_handle_match(handle, os_handle_zero());
}

shared_function void
lnk_close_file(void *raw_handle)
{
  OS_Handle handle = *(OS_Handle *)raw_handle;
  os_file_close(handle);
}

shared_function uint64_t
lnk_size_from_file(void *raw_handle)
{
  OS_Handle handle = *(OS_Handle *)raw_handle;
  FileProperties props  = os_properties_from_file(handle);
  return props.size;
}

shared_function uint64_t
lnk_read_file(void *raw_handle, void *buffer, uint64_t buffer_max)
{
  OS_Handle handle = *(OS_Handle *)raw_handle;
  U64 read_size = os_file_read(handle, rng_1u64(0, buffer_max), buffer);
  Assert(read_size == buffer_max);
  return read_size;
}

shared_function uint64_t
lnk_write_file(void *raw_handle, uint64_t offset, void *buffer, uint64_t buffer_size)
{
  OS_Handle handle = *(OS_Handle*)raw_handle;
  U64 write_size = os_file_write(handle, r1u64(offset, offset + buffer_size), buffer);
  return write_size;
}

////////////////////////////////

internal void
lnk_log_read(String8 path, U64 size)
{
  lnk_log(LNK_Log_IO_Read, "Read from \"%S\" %M", path, size);
}

internal String8
lnk_read_data_from_file_path(Arena *arena, String8 path)
{
  String8 data = str8_zero();
  OS_Handle handle = {0};
  int is_open = lnk_open_file_read((char*)path.str, path.size, &handle, sizeof(handle));
  if (is_open) {
    U64  buffer_size = lnk_size_from_file(&handle);
    U8  *buffer      = push_array_no_zero(arena, U8, buffer_size);
    U64  read_size   = lnk_read_file(&handle, buffer, buffer_size);

    data = str8(buffer, read_size);
	
    lnk_close_file(&handle);

    if (read_size != buffer_size) {
      lnk_error(LNK_Warning_IllData, "incomplete file read occurred, read %u bytes, expected %u bytes, file %S", path);
    }

    if (lnk_get_log_status(LNK_Log_IO_Read)) {
      lnk_log_read(path, data.size);
    }
  } else {
    lnk_error(LNK_Error_FileNotFound, "unable to open file %S", path);
  }
  return data;
}

internal
THREAD_POOL_TASK_FUNC(lnk_data_size_from_file_path_task)
{
  LNK_DiskReader *task = raw_task;
  String8         path = task->path_arr.v[task_id];

  OS_Handle handle = {0};
  U64       size   = 0;

  int is_open = lnk_open_file_read((char*)path.str, path.size, &handle, sizeof(handle));
  if (is_open) {
    size = lnk_size_from_file(&handle);
  }

  task->handle_arr[task_id] = handle;
  task->size_arr[task_id]   = size;
}

internal
THREAD_POOL_TASK_FUNC(lnk_data_from_file_path_task)
{
  LNK_DiskReader *task = raw_task;

  OS_Handle handle      = task->handle_arr[task_id];
  U64       buffer_size = task->size_arr[task_id];
  U8       *buffer      = task->buffer + task->off_arr[task_id];

  U64 read_size = lnk_read_file(&handle, buffer, buffer_size);
  Assert(read_size == buffer_size);

  task->data_arr.v[task_id] = str8(buffer, read_size);
}

internal String8Array
lnk_read_data_from_file_path_parallel(TP_Context *tp, Arena *arena, String8Array path_arr)
{
  Temp scratch = scratch_begin(&arena,1);

  LNK_DiskReader reader = {0};
  reader.path_arr       = path_arr;
  reader.handle_arr     = push_array_no_zero(scratch.arena, OS_Handle, path_arr.count);
  reader.size_arr       = push_array_no_zero(scratch.arena, U64, path_arr.count);

  // open handles and get sizes
  tp_for_parallel(tp, 0, path_arr.count, lnk_data_size_from_file_path_task, &reader);

  // compute file buffer size
  U64 total_data_size = sum_array_u64(path_arr.count, reader.size_arr);

  // assign offsets into file buffer
  U64 *off_arr = push_array_no_zero(scratch.arena, U64, path_arr.count);
  MemoryCopyTyped(off_arr, reader.size_arr, path_arr.count);
  counts_to_offsets_array_u64(path_arr.count, off_arr);

  reader.data_arr = str8_array_reserve(arena, path_arr.count);
  reader.off_arr  = off_arr;
  reader.buffer   = push_array_no_zero(arena, U8, total_data_size);

  // read files and close handles
  tp_for_parallel(tp, 0, path_arr.count, lnk_data_from_file_path_task, &reader);
  
  String8Array result = {0};
  result.count        = path_arr.count;
  result.v            = reader.data_arr.v;

  if (lnk_get_log_status(LNK_Log_IO_Read)) {
    for (U64 i = 0; i < result.count; ++i) {
      lnk_log_read(path_arr.v[i], result.v[i].size);
    }
  }

  scratch_end(scratch);
  return result;
}

internal void
lnk_write_data_list_to_file_path(String8 path, String8List data)
{
  ProfBeginV("Write %M to %S", data.total_size, path);

  B32 is_written = 0;

  OS_Handle handle;
  if (lnk_open_file_write((char*)path.str, path.size, &handle, sizeof(handle))) {
    U64 offset = 0;
    for (String8Node *data_n = data.first; data_n != 0; data_n = data_n->next) {
      U64 write_size = lnk_write_file(&handle, offset, data_n->string.str, data_n->string.size);
      if (write_size != data_n->string.size) {
        break;
      }
      offset += data_n->string.size;
    }

    lnk_close_file(&handle);

    is_written = (offset == data.total_size);
    if (is_written) {
      if (lnk_get_log_status(LNK_Log_IO_Write)) {
        lnk_log(LNK_Log_IO_Write, "File \"%S\" %M written", path, data.total_size);
      }
    } else {
      lnk_error(LNK_Error_IO, "incomplete write occurred, %M written, expected %M, file %S",
                offset, data.total_size, path);
    }
  } else {
    lnk_error(LNK_Error_NoAccess, "don't have access to write to %S", path);
  }
  
  ProfEnd();
}

internal void
lnk_write_data_to_file_path(String8 path, String8 data)
{
  Temp scratch = scratch_begin(0,0);
  String8List data_list = {0};
  str8_list_push(scratch.arena, &data_list, data);
  lnk_write_data_list_to_file_path(path, data_list);
  scratch_end(scratch);
}

internal String8List
lnk_file_search(Arena *arena, String8List dir_list, String8 file_path)
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

