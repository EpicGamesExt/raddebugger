internal void
lnk_log_read(String8 path, U64 size)
{
  Temp scratch = scratch_begin(0,0);
  String8 size_str = str8_from_memory_size2(scratch.arena, size);
  lnk_log(LNK_Log_IO_Read, "Read from \"%S\" %S", path, size_str);
  scratch_end(scratch);
}

internal String8
lnk_read_data_from_file_path(Arena *arena, String8 path)
{
  String8 data = os_data_from_file_path(arena, path);
  if (lnk_get_log_status(LNK_Log_IO_Read)) {
    lnk_log_read(path, data.size);
  }
  return data;
}

internal
THREAD_POOL_TASK_FUNC(lnk_data_size_from_file_path_task)
{
  LNK_DiskReader *task = raw_task;

  String8        path   = task->path_arr.v[task_id];
  OS_Handle      handle = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, path);
  FileProperties props  = os_properties_from_file(handle);

  task->handle_arr[task_id] = handle;
  task->size_arr[task_id]   = props.size;
}

internal
THREAD_POOL_TASK_FUNC(lnk_data_from_file_path_task)
{
  LNK_DiskReader *task = raw_task;

  OS_Handle handle = task->handle_arr[task_id];
  U64       size   = task->size_arr[task_id];
  U8       *buffer = task->buffer + task->off_arr[task_id];

  U64 read_size = os_file_read(handle, rng_1u64(0, size), buffer);
  Assert(read_size == size);

  task->data_arr.v[task_id] = str8(buffer, read_size);

  os_file_close(handle);
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
#if PROFILE_TELEMETRY
  {
    Temp scratch = scratch_begin(0, 0);
    String8 size_str = str8_from_memory_size2(scratch.arena, data.total_size);
    ProfBeginDynamic("Write %.*s to %.*s", str8_varg(size_str), str8_varg(path));
    scratch_end(scratch);
  }
#endif
  B32 is_written = os_write_data_list_to_file_path(path, data);
  if (is_written) {
    if (lnk_get_log_status(LNK_Log_IO_Write)) {
      Temp scratch = scratch_begin(0,0);
      String8 size_str = str8_from_memory_size2(scratch.arena, data.total_size);
      lnk_log(LNK_Log_IO_Write, "File \"%S\" %S written", path, size_str);
      scratch_end(scratch);
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

