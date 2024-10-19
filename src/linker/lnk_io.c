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

internal String8Array
lnk_read_data_from_file_path_parallel(TP_Context *tp, Arena *arena, String8Array path_arr)
{
  String8Array result = os_data_from_file_path_parallel(tp, arena, path_arr);
  if (lnk_get_log_status(LNK_Log_IO_Read)) {
    for (U64 i = 0; i < result.count; ++i) {
      lnk_log_read(path_arr.v[i], result.v[i].size);
    }
  }
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

