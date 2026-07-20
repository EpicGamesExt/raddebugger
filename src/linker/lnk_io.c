shared_function int
lnk_open_file_read(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max)
{
  File handle = file_open(AccessFlag_Read|AccessFlag_ShareRead, str8((U8*)path, path_size));
  Assert(sizeof(handle) <= handle_buffer_max);
  MemoryCopy(handle_buffer, &handle, sizeof(handle));
  return !file_match(handle, file_zero());
}

shared_function int
lnk_open_file_write(char *path, uint64_t path_size, void *handle_buffer, uint64_t handle_buffer_max)
{
  ProfBeginFunction();
  File handle = file_open(AccessFlag_Write, str8((U8*)path, path_size));
  Assert(sizeof(handle) <= handle_buffer_max);
  MemoryCopy(handle_buffer, &handle, sizeof(handle));
  ProfEnd();
  return !file_match(handle, file_zero());
}

shared_function void
lnk_close_file(void *raw_handle)
{
  File handle = *(File *)raw_handle;
  file_close(handle);
}

shared_function uint64_t
lnk_size_from_file(void *raw_handle)
{
  File handle = *(File *)raw_handle;
  FileProperties props  = properties_from_file(handle);
  return props.size;
}

shared_function uint64_t
lnk_read_file(void *raw_handle, void *buffer, uint64_t buffer_max)
{
  File handle = *(File *)raw_handle;
  U64 read_size = file_read(handle, rng_1u64(0, buffer_max), buffer);
  Assert(read_size == buffer_max);
  return read_size;
}

shared_function uint64_t
lnk_write_file(void *raw_handle, uint64_t offset, void *buffer, uint64_t buffer_size)
{
  File handle = *(File*)raw_handle;
  U64 write_size = file_write(handle, r1u64(offset, offset + buffer_size), buffer);
  Assert(write_size == buffer_size);
  return write_size;
}

internal String8
lnk_find_first_file(Arena *arena, String8List dir_list, String8 path)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  String8 result = {0};
  if (file_path_exists(path)) {
    PathStyle path_style = path_style_from_str8(path);
    if (path_style == PathStyle_Relative) {
      String8 current_path = get_current_path(scratch.arena);
      String8List l = {0};
      str8_list_push(scratch.arena, &l, current_path);
      str8_list_push(scratch.arena, &l, path);
      result = str8_path_list_join_by_style(arena, &l, PathStyle_SystemAbsolute);
    } else {
      result = path;
    }
  } else {
    String8 file_name = str8_skip_last_slash(path);
    for EachNode(n, String8Node, dir_list.first) {
      String8 full_path = push_str8f(scratch.arena, "%S/%S", n->string, file_name);
      if (file_path_exists(full_path)) {
        result = push_str8_copy(arena, full_path);
        break;
      }
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal File
lnk_file_open_with_rename_permissions(String8 path)
{
  ProfBeginFunction();
  File file_handle = file_zero();
#if OS_WINDOWS
  Temp scratch = scratch_begin(0,0);

  // open file with permissions to rename
  String16            path16              = str16_from_8(scratch.arena, path);
  SECURITY_ATTRIBUTES security_attributes = { sizeof(security_attributes) };
  HANDLE native_handle = CreateFileW((WCHAR*)path16.str,
                                     GENERIC_WRITE|DELETE,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     &security_attributes,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
  if (native_handle != INVALID_HANDLE_VALUE) {
    file_handle.u64[0] = (U64)native_handle;
  }

  scratch_end(scratch);
#else
  file_handle = file_open(AccessFlag_Read|AccessFlag_Write, path);
#endif
  ProfEnd();
  return file_handle;
}

internal B32
lnk_file_set_delete_on_close(File handle, B32 delete_file)
{
  B32 is_set = 0;
#if OS_WINDOWS
  FILE_DISPOSITION_INFO file_disposition = {0};
  file_disposition.DeleteFile            = (BOOL)delete_file;
  is_set = SetFileInformationByHandle((HANDLE)handle.u64[0], FileDispositionInfo, &file_disposition, sizeof(file_disposition));
#elif OS_LINUX
  // no equivalent
  is_set = 1;
#else
# error "TODO: file rename"
#endif
  return is_set;
}

internal B32
lnk_file_rename(File handle, String8 new_name)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_renamed = 0;
#if OS_WINDOWS
  String16 new_name16 = str16_from_8(scratch.arena, new_name);

  U64 file_rename_info_size = sizeof(FILE_RENAME_INFO);
  U64 buffer_size           = file_rename_info_size + sizeof(new_name16.str)*new_name16.size;
  U8 *buffer                = push_array(scratch.arena, U8, buffer_size);

  FILE_RENAME_INFO *rename_info = (FILE_RENAME_INFO *)buffer;
  rename_info->ReplaceIfExists  = 1;
  rename_info->FileNameLength   = new_name16.size * sizeof(new_name16.str[0]);
  MemoryCopy(rename_info->FileName, new_name16.str, new_name16.size * sizeof(new_name16.str[0]));

  is_renamed = SetFileInformationByHandle((HANDLE)handle.u64[0], FileRenameInfo, buffer, buffer_size);
#else
  char fd_proc_path[128];
  raddbg_snprintf(fd_proc_path, sizeof(fd_proc_path), "/proc/self/fd/%d", (int)handle.u64[0]);

  U64      path_max  = 4096;
  char    *path      = push_array(scratch.arena, char, path_max);
  ssize_t  path_size = readlink(fd_proc_path, path, path_max);

  if (path_size > 0) {
    is_renamed = rename(path, (char *)push_cstr(scratch.arena, new_name).str) == 0;
  }
#endif
  scratch_end(scratch);
  return is_renamed;
}

internal void
lnk_log_read(String8 path, U64 size)
{
  lnk_log(LNK_Log_IO_Read, "Read from \"%S\" %M", path, size);
}

internal String8
lnk_read_data_from_file_path(Arena *arena, LNK_IO_Flags io_flags, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  TP_Context *single_thread_ctx = tp_alloc(scratch.arena, 1, 1, str8_zero());
  String8Array data_arr = lnk_read_data_from_file_path_parallel(single_thread_ctx, arena, io_flags, (String8Array){ .count = 1, .v = &path });
  scratch_end(scratch);
  return data_arr.v[0];
}

internal
THREAD_POOL_TASK_FUNC(lnk_data_size_from_file_path_task)
{
  LNK_DiskReader *task = raw_task;
  String8         path = task->path_arr.v[task_id];

  File handle = {0};
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

  File handle      = task->handle_arr[task_id];
  U64       buffer_size = task->size_arr[task_id];
  U8       *buffer      = task->buffer + task->off_arr[task_id];

  U64 read_size = lnk_read_file(&handle, buffer, buffer_size);
  Assert(read_size == buffer_size);

  task->data_arr.v[task_id] = str8(buffer, read_size);
}

internal
THREAD_POOL_TASK_FUNC(lnk_memory_map_file_task)
{
  LNK_DiskReader *task = raw_task;
  Temp scratch = scratch_begin(&arena, 1);
#if OS_WINDOWS
  String16 path16      = str16_from_8(scratch.arena, task->path_arr.v[task_id]);
  if (task->io_flags & LNK_IO_Flags_MemoryMapFilesReadWrite) {
    HANDLE file_handle = CreateFileW(path16.str, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle != INVALID_HANDLE_VALUE) {
      HANDLE mapping_handle = CreateFileMappingA(file_handle, 0, PAGE_READWRITE, 0, 0, 0);
      if (mapping_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size = {0};
        GetFileSizeEx(file_handle, &file_size);
        void *file_data = MapViewOfFile(mapping_handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, file_size.QuadPart);
        if (file_data) {
          task->data_arr.v[task_id] = str8(file_data, file_size.QuadPart);
        }
        CloseHandle(mapping_handle);
      }
      CloseHandle(file_handle);
    }
  } else {
    HANDLE file_handle = CreateFileW(path16.str, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle != INVALID_HANDLE_VALUE) {
      HANDLE mapping_handle = CreateFileMappingA(file_handle, 0, PAGE_WRITECOPY, 0, 0, 0);
      if (mapping_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size = {0};
        GetFileSizeEx(file_handle, &file_size);
        void *file_data = MapViewOfFile(mapping_handle, FILE_MAP_COPY, 0, 0, file_size.QuadPart);
        if (file_data) {
          task->data_arr.v[task_id] = str8(file_data, file_size.QuadPart);
        }
        CloseHandle(mapping_handle);
      }
      CloseHandle(file_handle);
    }
  }
#elif OS_LINUX
  int fd = open((char *)push_cstr(scratch.arena, task->path_arr.v[task_id]).str, O_RDONLY);
  if (fd != -1) {
    struct stat st = {0};
    if (fstat(fd, &st) == 0) {
      void *file_data = mmap(0, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
      if (file_data != MAP_FAILED) {
        task->data_arr.v[task_id] = str8(file_data, st.st_size);
      }
    }
    close(fd);
  }
#else
# error "memory mapping files is not supported on this platform"
#endif
  scratch_end(scratch);
}

internal String8Array
lnk_read_data_from_file_path_parallel(TP_Context *tp, Arena *arena, LNK_IO_Flags io_flags, String8Array path_arr)
{
  ProfBeginFunction();
  LNK_DiskReader reader = {0};

  if (io_flags & (LNK_IO_Flags_MemoryMapFilesReadWrite|LNK_IO_Flags_MemoryMapFilesReadOnly)) {
    reader.io_flags       = io_flags;
    reader.path_arr       = path_arr;
    reader.data_arr.count = path_arr.count;
    reader.data_arr.v     = push_array(arena, String8, path_arr.count);
    tp_for_parallel(tp, 0, path_arr.count, lnk_memory_map_file_task, &reader);
  } else {
    Temp scratch = scratch_begin(&arena,1);

    reader.path_arr       = path_arr;
    reader.handle_arr     = push_array_no_zero(scratch.arena, File, path_arr.count);
    reader.size_arr       = push_array_no_zero(scratch.arena, U64, path_arr.count);

    // open handles and get sizes
    tp_for_parallel(tp, 0, path_arr.count, lnk_data_size_from_file_path_task, &reader);

    // compute file buffer size
    U64 total_data_size = sum_array_u64(path_arr.count, reader.size_arr);

    // assign offsets into file buffer
    U64 *off_arr = push_array_no_zero(scratch.arena, U64, path_arr.count);
    MemoryCopyTyped(off_arr, reader.size_arr, path_arr.count);
    u64_array_counts_to_offsets(path_arr.count, off_arr);

    reader.io_flags = io_flags;
    reader.data_arr = str8_array_reserve(arena, path_arr.count);
    reader.off_arr  = off_arr;
    reader.buffer   = push_array_no_zero(arena, U8, total_data_size);

    // read files and close handles
    tp_for_parallel(tp, 0, path_arr.count, lnk_data_from_file_path_task, &reader);

    scratch_end(scratch);
  }
  
  String8Array result = {0};
  result.count        = path_arr.count;
  result.v            = reader.data_arr.v;

  if (lnk_get_log_status(LNK_Log_IO_Read)) {
    for (U64 i = 0; i < result.count; ++i) {
      lnk_log_read(path_arr.v[i], result.v[i].size);
    }
  }

  ProfEnd();
  return result;
}

internal void
lnk_write_data_list_to_file_path(String8 path, String8 temp_path, String8List data)
{
  ProfBeginV("Write %M to %S", data.total_size, path);

  B32       open_with_rename = (temp_path.size > 0);
  File file_handle      = {0};
  String8   open_file_path   = {0};
  if (open_with_rename) {
    file_handle    = lnk_file_open_with_rename_permissions(temp_path);
    open_file_path = temp_path;

    // mark file to be deleted on exit, so we don't leave corrupted files on disk
    if (!lnk_file_set_delete_on_close(file_handle, 1)) {
      lnk_error(LNK_Error_IO, "failed to update file disposition on %S", open_file_path);
    }
  } else {
    lnk_open_file_write((char*)path.str, path.size, &file_handle, sizeof(file_handle));
    open_file_path = path;
  }

  if (!file_match(file_handle, file_zero())) {
    // try to reserve up front file size
    if (!file_reserve_size(file_handle, data.total_size)) {
      lnk_log(LNK_Log_IO_Write, "Failed to pre-allocate file %S with size %M", open_file_path, data.total_size);
    }

    // write data nodes
    U64 bytes_written = 0;
    for (String8Node *data_n = data.first; data_n != 0; data_n = data_n->next) {
      U64 write_size = lnk_write_file(&file_handle, bytes_written, data_n->string.str, data_n->string.size);
      if (write_size != data_n->string.size) {
        break;
      }
      bytes_written += data_n->string.size;
    }
    B32 is_write_complete = (bytes_written == data.total_size);

    if (is_write_complete) {
      // rename temp file
      if (open_with_rename) {
        // all writes succeeded, remove delete on exit flag
        if (!lnk_file_set_delete_on_close(file_handle, 0)) {
          lnk_error(LNK_Error_IO, "failed to update file disposition on %S", open_file_path);
        }

        if (lnk_file_rename(file_handle, path)) {
          lnk_log(LNK_Log_IO_Write, "Renamed %S -> %S", temp_path, path);
        } else {
          lnk_error(LNK_Error_IO, "failed to rename %S -> %S", temp_path, path);
        }
      }
    }

    // clean up file handle
    lnk_close_file(&file_handle);

    // log write
    if (is_write_complete) {
      if (lnk_get_log_status(LNK_Log_IO_Write)) {
        lnk_log(LNK_Log_IO_Write, "File \"%S\" %M written", path, data.total_size);
      }
    } else {
      lnk_error(LNK_Error_IO, "incomplete write, %M written, expected %M, file %S", bytes_written, data.total_size, path);
    }
  } else {
    lnk_error(LNK_Error_NoAccess, "don't have access to write to %S", path);
  }
  
  ProfEnd();
}

internal void
lnk_write_data_to_file_path(String8 path, String8 temp_path, String8 data)
{
  Temp scratch = scratch_begin(0,0);
  String8List data_list = {0};
  str8_list_push(scratch.arena, &data_list, data);
  lnk_write_data_list_to_file_path(path, temp_path, data_list);
  scratch_end(scratch);
}

internal String8
lnk_data_from_file_artifact(Arena *arena, LNK_FileArtifact *artifact)
{
  return str8_list_join(arena, &artifact->data, 0);
}

// --- Background Writer -------------------------------------------------------

struct LNK_BackgroundFile
{
  LNK_BackgroundFile *next;
  String8             path;
  String8             temp_path;
  String8             open_path;
  File                file;
  U64                 bytes_written;
  B32                 open_with_rename;
  B32                 is_open;
  B32                 is_finished;
  B32                 is_complete;
  B32                 write_failed;
};

typedef enum
{
  LNK_BackgroundFileJobKind_Write,
  LNK_BackgroundFileJobKind_EndFile,
  LNK_BackgroundFileJobKind_EndWriter,
} LNK_BackgroundFileJobKind;

typedef struct
{
  LNK_BackgroundFileJobKind kind;
  LNK_BackgroundFile       *file;
  U64                       file_off;
  U64                       expected_byte_count;
  String8                   data;
} LNK_BackgroundFileWriteJob;

internal void
lnk_background_file_writer_end_file_on_thread(LNK_BackgroundFile *file, U64 expected_byte_count)
{
  B32 is_complete = !file->write_failed && file->bytes_written == expected_byte_count;
  if (is_complete && file->open_with_rename) {
    if (!lnk_file_set_delete_on_close(file->file, 0)) {
      lnk_error(LNK_Error_IO, "failed to update file disposition on %S", file->open_path);
      is_complete = 0;
    } else if (!lnk_file_rename(file->file, file->path)) {
      lnk_error(LNK_Error_IO, "failed to rename %S -> %S", file->temp_path, file->path);
      is_complete = 0;
    }
  }

  lnk_close_file(&file->file);
  file->is_open     = 0;
  file->is_complete = is_complete;

  if (!is_complete) {
    lnk_error(LNK_Error_IO, "incomplete write, %M written, expected %M, file %S", file->bytes_written, expected_byte_count, file->path);
  } else if (lnk_get_log_status(LNK_Log_IO_Write)) {
    lnk_log(LNK_Log_IO_Write, "File \"%S\" %M written", file->path, expected_byte_count);
  }
}

internal void
lnk_background_file_writer_thread(void *raw_writer)
{
  ProfBeginFunction();
  set_thread_namef("Background File Writer");

  LNK_BackgroundFileWriter *writer = raw_writer;
  for (;;) {
    LNK_BackgroundFileWriteJob job = {0};
    RingGuard guard = guarded_ring_open(writer->queue);
    B32 is_read = guarded_ring_read_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_read);

    if (job.kind == LNK_BackgroundFileJobKind_EndWriter) { break; }

    if (job.kind == LNK_BackgroundFileJobKind_Write) {
      U64 write_size = lnk_write_file(&job.file->file, job.file_off, job.data.str, job.data.size);
      if (write_size != job.data.size) {
        job.file->write_failed = 1;
      }
      job.file->bytes_written += write_size;
    } else if (job.kind == LNK_BackgroundFileJobKind_EndFile) {
      lnk_background_file_writer_end_file_on_thread(job.file, job.expected_byte_count);
    }
  }
  ProfEnd();
}

internal void
lnk_background_file_writer_begin(LNK_BackgroundFileWriter *writer)
{
  ProfBegin("Background File Writer Begin");

  writer->queue_arena = arena_alloc(.reserve_size = MB(2), .commit_size = MB(2), .name = "BACKGROUND_FILE_WRITE_QUEUE");
  writer->queue       = guarded_ring_alloc(writer->queue_arena, MB(1));
  ProfEnd();
}

internal LNK_BackgroundFile *
lnk_background_file_writer_begin_file(LNK_BackgroundFileWriter *writer, String8 path, String8 temp_path)
{
  ProfBegin("Background File Writer Begin File");

  if (!writer->is_running) {
    writer->thread     = thread_launch(lnk_background_file_writer_thread, writer);
    writer->is_running = writer->thread.u64[0] != 0;
    if (!writer->is_running) {
      lnk_error(LNK_Error_IO, "failed to start background file writer");
      ProfEnd();
      return 0;
    }
  }

  LNK_BackgroundFile *file = push_array(writer->queue_arena, LNK_BackgroundFile, 1);

  file->path             = path;
  file->temp_path        = temp_path;
  file->open_with_rename = (temp_path.size > 0);

  if (file->open_with_rename) {
    file->file      = lnk_file_open_with_rename_permissions(temp_path);
    file->open_path = temp_path;
  } else {
    lnk_open_file_write((char *)path.str, path.size, &file->file, sizeof(file->file));
    file->open_path = path;
  }

  file->is_open = !file_match(file->file, file_zero());
  if (!file->is_open) {
    lnk_error(LNK_Error_NoAccess, "don't have access to write to %S", path);
    goto exit;
  }

  if (file->open_with_rename && !lnk_file_set_delete_on_close(file->file, 1)) {
    lnk_error(LNK_Error_IO, "failed to update file disposition on %S", file->open_path);
    lnk_close_file(&file->file);
    file->is_open = 0;
    goto exit;
  }

  SLLQueuePush(writer->file_first, writer->file_last, file);

  exit:;
  ProfEnd();
  return file->is_open ? file : 0;
}

internal void
lnk_background_file_writer_enqueue(LNK_BackgroundFileWriter *writer, LNK_BackgroundFile *file, U64 file_off, String8 data)
{
  ProfBegin("Background File Writer Enqueue");

  Assert(writer->is_running);
  Assert(file->is_open && !file->is_finished);

  if (data.size > 0) {
    LNK_BackgroundFileWriteJob job = {
      .kind     = LNK_BackgroundFileJobKind_Write,
      .file     = file,
      .file_off = file_off,
      .data     = data,
    };
    RingGuard guard      = guarded_ring_open(writer->queue);
    B32       is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_written);
  }

  ProfEnd();
}

internal void
lnk_background_file_writer_end_file(LNK_BackgroundFileWriter *writer, LNK_BackgroundFile *file, U64 expected_byte_count)
{
  ProfBegin("Background File Writer End File");

  Assert(writer->is_running);
  Assert(file->is_open && !file->is_finished);
  file->is_finished = 1;

  LNK_BackgroundFileWriteJob job = {
    .kind                = LNK_BackgroundFileJobKind_EndFile,
    .file                = file,
    .expected_byte_count = expected_byte_count,
  };
  RingGuard guard      = guarded_ring_open(writer->queue);
  B32       is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
  guarded_ring_close(&guard);
  Assert(is_written);

  ProfEnd();
}

internal void
lnk_background_file_writer_end(LNK_BackgroundFileWriter *writer)
{
  ProfBegin("Background File Writer End");

  if (writer->is_running) {
    LNK_BackgroundFileWriteJob job        = { .kind = LNK_BackgroundFileJobKind_EndWriter };
    RingGuard                  guard      = guarded_ring_open(writer->queue);
    B32                        is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_written);
    thread_join(writer->thread, -1);
  }

  for EachNode(file, LNK_BackgroundFile, writer->file_first) {
    if (file->is_open) {
      lnk_error(LNK_Error_IO, "unfinished background write, file %S", file->path);
      lnk_close_file(&file->file);
      file->is_open = 0;
    }
  }

  guarded_ring_release(writer->queue);
  arena_release(writer->queue_arena);
  writer->is_running = 0;

  ProfEnd();
}
