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
lnk_read_data_from_file_path(Arena *arena, LNK_IO_Flags io_flags, String8 path, B8 *was_read_out)
{
  Temp scratch = scratch_begin(&arena, 1);
  TP_Context *single_thread_ctx = tp_alloc(scratch.arena, 1, 1, str8_zero());
  String8Array data_arr = lnk_read_data_from_file_path_parallel(single_thread_ctx, arena, io_flags, (String8Array){ .count = 1, .v = &path }, was_read_out);
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
  LNK_DiskReader *task   = raw_task;
  File            handle = task->handle_arr[task_id];
  if ( ! MemoryIsZeroStruct(&handle)) {
    U64  buffer_size = task->size_arr[task_id];
    U8  *buffer      = task->buffer + task->off_arr[task_id];
    U64  read_size   = lnk_read_file(&handle, buffer, buffer_size);
    AssertAlways(read_size == buffer_size); // TODO: handle partial loads as invalid

    task->data_arr.v[task_id] = str8(buffer, read_size);

    if (task->was_read) {
      task->was_read[task_id] = 1;
    }
  }
}

#if OS_WINDOWS
// Input views are mapped from PAGE_WRITECOPY sections with FILE_MAP_READ access, so an
// untouched view carries no pagefile commit charge. Mapping with FILE_MAP_COPY instead
// would charge commit for the ENTIRE view at map time -- even for pages never written --
// so N concurrent big links would hold input-set-sized commit for their whole runtime
// (measured 22.4 GiB of a 49.7 GiB peak on a large editor DLL link) and feed build-farm
// memory admission limits for no benefit.
//
// The few remaining writers that patch input bytes in place (IFC 0x1522 LF_IFC_RECORD
// NOTYPE pokes, LF_ENDPRECOMP removal in debug$P; a couple of bytes per page, a handful
// of pages per link) hit this vectored handler, which promotes JUST the faulting page to
// PAGE_WRITECOPY and retries. Commit is then charged per dirtied page instead of per
// view. Write semantics are identical to the old FILE_MAP_COPY mapping: the first write
// makes the page private, the input file is never modified. Pages of ordinary allocations
// or of read-write mappings never reach the handler (they do not fault on write), so
// /RAD_MEMORY_MAP_FILES:READ_WRITE and no-map modes are unaffected.
global volatile LONG g_lnk_cow_veh_installed;
global volatile LONG g_lnk_cow_promoted_pages;

internal LONG NTAPI
lnk_cow_page_promote_veh(EXCEPTION_POINTERS *info)
{
  EXCEPTION_RECORD *er = info->ExceptionRecord;
  if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2 && er->ExceptionInformation[0] == 1) {
    void *addr = (void *)er->ExceptionInformation[1];
    MEMORY_BASIC_INFORMATION mbi = {0};
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) >= sizeof(mbi) && mbi.Type == MEM_MAPPED) {
      if (mbi.Protect == PAGE_READONLY) {
        void *page = (void *)((UINT_PTR)addr & ~(UINT_PTR)(KB(4) - 1));
        DWORD old_protect = 0;
        if (VirtualProtect(page, KB(4), PAGE_WRITECOPY, &old_protect)) {
          InterlockedIncrement(&g_lnk_cow_promoted_pages);
          return EXCEPTION_CONTINUE_EXECUTION;
        }
      } else if (mbi.Protect == PAGE_WRITECOPY || mbi.Protect == PAGE_READWRITE) {
        // another thread promoted this page between our fault and the query; retry the write
        return EXCEPTION_CONTINUE_EXECUTION;
      }
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

internal
THREAD_POOL_TASK_FUNC(lnk_memory_map_file_task)
{
  LNK_DiskReader *task = raw_task;
  Temp scratch = scratch_begin(&arena, 1);

#if OS_WINDOWS
  String16 path16 = str16_from_8(scratch.arena, task->path_arr.v[task_id]);

  // TODO: deprecate CoW file maps; they count against private byte space and * by design * do not allow over-commit;
  // and if program's total file map size exceeds the cap, kernel terminates the whole process with an out-of-memory error;
  // exception handler helpes us work-around this limitation but we stil cannot link on devices with small private byte space;
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
          if (task->was_read) {
            task->was_read[task_id] = 1;
          }
        }

        CloseHandle(mapping_handle);
      }

      CloseHandle(file_handle);
    }
  } else {
    HANDLE file_handle = CreateFileW(path16.str, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle != INVALID_HANDLE_VALUE) {
      if (InterlockedCompareExchange(&g_lnk_cow_veh_installed, 1, 0) == 0) {
        AddVectoredExceptionHandler(1, lnk_cow_page_promote_veh);
      }
      HANDLE mapping_handle = CreateFileMappingA(file_handle, 0, PAGE_WRITECOPY, 0, 0, 0);
      if (mapping_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size = {0};
        GetFileSizeEx(file_handle, &file_size);
        // FILE_MAP_READ view of a WRITECOPY section: zero commit charge at map time;
        // pages become writable one at a time through lnk_cow_page_promote_veh
        void *file_data = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, file_size.QuadPart);
        if (file_data) {
          task->data_arr.v[task_id] = str8(file_data, file_size.QuadPart);
          if (task->was_read) {
            task->was_read[task_id] = 1;
          }
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
        if (task->was_read) {
          task->was_read[task_id] = 1;
        }
      }
    }
    close(fd);
  }
#else
# error "memory mapping files is not supported on this platform"
#endif
  scratch_end(scratch);
}

// Experimental lane cap for opening thousands of independent object mappings. Each lane pulls
// file indices from a shared cursor, so this preserves the ordinary per-file mapping operation and
// output ordering while letting us measure the Windows object-manager concurrency knee.
typedef struct
{
  LNK_DiskReader *reader;
  U64             item_count;
  U64             cursor;
} LNK_MemoryMapCappedTask;

internal
THREAD_POOL_TASK_FUNC(lnk_memory_map_file_capped_task)
{
  LNK_MemoryMapCappedTask *task = raw_task;
  for (;;) {
    U64 item_idx = ins_atomic_u64_inc_eval(&task->cursor) - 1;
    if (item_idx >= task->item_count) { break; }
    lnk_memory_map_file_task(arena, worker_id, item_idx, task->reader, tp);
  }
}

internal String8Array
lnk_read_data_from_file_path_parallel(TP_Context *tp, Arena *arena, LNK_IO_Flags io_flags, String8Array path_arr, B8 *was_read)
{
  ProfBeginFunction();
  LNK_DiskReader reader = {0};

  if (io_flags & (LNK_IO_Flags_MemoryMapFilesReadWrite|LNK_IO_Flags_MemoryMapFilesReadOnly)) {
    reader.io_flags       = io_flags;
    reader.path_arr       = path_arr;
    reader.data_arr.count = path_arr.count;
    reader.data_arr.v     = push_array(arena, String8, path_arr.count);
    reader.was_read       = was_read;
    char *map_worker_env = getenv("RAD_COBJ_MAP_WORKERS");
    U64 map_worker_cap = map_worker_env ? strtoull(map_worker_env, 0, 10) : 0;
#if OS_WINDOWS
    if (!map_worker_env && path_arr.count >= 64) {
      // Standalone compressed objects expose a sparse raw COFF view. A small sample detects the
      // homogeneous compressed corpus without opening or touching ordinary raw object contents.
      // Raw links retain the original unrestricted tp_for_parallel call below.
      Temp detect_scratch = scratch_begin(&arena, 1);
      U64 sample_count = Min(path_arr.count, 32);
      for EachIndex(i, sample_count) {
        String16 path16 = str16_from_8(detect_scratch.arena, path_arr.v[i]);
        DWORD attrs = GetFileAttributesW(path16.str);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_SPARSE_FILE)) {
          map_worker_cap = 8;
          break;
        }
      }
      scratch_end(detect_scratch);
    }
#endif
    if (map_worker_cap > 0 && map_worker_cap < path_arr.count) {
      LNK_MemoryMapCappedTask map_task = { .reader = &reader, .item_count = path_arr.count };
      tp_for_parallel(tp, 0, map_worker_cap, lnk_memory_map_file_capped_task, &map_task);
    } else {
      // Default/raw behavior remains the original unrestricted parallel-for.
      tp_for_parallel(tp, 0, path_arr.count, lnk_memory_map_file_task, &reader);
    }
  } else {
    Temp scratch = scratch_begin(&arena,1);

    reader.path_arr   = path_arr;
    reader.handle_arr = push_array_no_zero(scratch.arena, File, path_arr.count);
    reader.size_arr   = push_array_no_zero(scratch.arena, U64, path_arr.count);
    reader.was_read   = was_read;

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
  B32                 open_failed;
};

typedef enum
{
  LNK_BackgroundFileJobKind_OpenFile,
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
  B32                       decommit_after_write; // hand the buffer's pages back to the OS once written
} LNK_BackgroundFileWriteJob;

typedef struct
{
  void *ptr;
  U64   size; // zero marks the end of the queue
} LNK_BackgroundFileDecommitJob;

internal void
lnk_background_file_decommit_thread(void *raw_writer)
{
  ProfBeginFunction();
  set_thread_namef("PDB Page Decommit");

  LNK_BackgroundFileWriter *writer = raw_writer;
  for (;;) {
    LNK_BackgroundFileDecommitJob job = {0};
    RingGuard guard = guarded_ring_open(writer->decommit_queue);
    B32 is_read = guarded_ring_read_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_read);
    if (job.size == 0) { break; }
    decommit_memory(job.ptr, job.size);
    ins_atomic_u64_add_eval(&writer->decommit_bytes_completed, job.size);
  }

  ProfEnd();
}

// Opening the output file can cost seconds on the caller's thread when it
// replaces a previous multi-GB artifact (NTFS truncate/dealloc of the old
// allocation happens inside CreateFile). Jobs on one file are queue-ordered,
// so deferring the open to the writer thread keeps every write behind it
// while the caller returns immediately.
internal void
lnk_background_file_writer_open_file_on_thread(LNK_BackgroundFile *file)
{
  if (file->open_with_rename) {
    file->file      = lnk_file_open_with_rename_permissions(file->temp_path);
    file->open_path = file->temp_path;
  } else {
    lnk_open_file_write((char *)file->path.str, file->path.size, &file->file, sizeof(file->file));
    file->open_path = file->path;
  }

  file->is_open = !file_match(file->file, file_zero());
  if (!file->is_open) {
    file->open_failed  = 1;
    file->write_failed = 1;
    return;
  }

  if (file->open_with_rename && !lnk_file_set_delete_on_close(file->file, 1)) {
    lnk_error(LNK_Error_IO, "failed to update file disposition on %S", file->open_path);
    lnk_close_file(&file->file);
    file->is_open      = 0;
    file->open_failed  = 1;
    file->write_failed = 1;
  }
}

internal void
lnk_background_file_writer_end_file_on_thread(LNK_BackgroundFile *file, U64 expected_byte_count)
{
  if (file->open_failed) {
    lnk_error(LNK_Error_NoAccess, "don't have access to write to %S", file->path);
    file->is_complete = 0;
    return;
  }

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

internal int
lnk_background_file_write_job_is_before(const void *raw_a, const void *raw_b)
{
  const LNK_BackgroundFileWriteJob *a = raw_a, *b = raw_b;
  if (a->file != b->file) { return a->file < b->file ? -1 : +1; }
  if (a->file_off != b->file_off) { return a->file_off < b->file_off ? -1 : +1; }
  return 0;
}

// Write jobs on one file cover disjoint file ranges (each MSF page is enqueued exactly
// once: sealed streams are disjoint page sets, and the final remainder pass skips pages
// already enqueued), so reordering Write jobs among themselves cannot change the output.
// Control jobs (OpenFile/EndFile/EndWriter) are ordering barriers: the batch drain stops
// at the first control job and the batched writes are issued BEFORE it runs, preserving
// the queue-order guarantees (every write lands behind its file's open and before its
// file's end).
internal void
lnk_background_file_writer_issue_write_batch(LNK_BackgroundFileWriter *writer, LNK_BackgroundFileWriteJob *batch, U64 count)
{
  qsort(batch, count, sizeof(batch[0]), lnk_background_file_write_job_is_before);
  for (U64 i = 0; i < count; ) {
    // coalesce jobs that are contiguous in BOTH file space and memory into one write;
    // MSF page numbers map linearly to page-data-node memory, so page runs from
    // interleaved stream enqueues merge back into large sequential writes here
    U64 j = i;
    U64 end_off = batch[i].file_off  + batch[i].data.size;
    U8 *end_ptr = batch[i].data.str  + batch[i].data.size;
    for (; j + 1 < count; j += 1) {
      LNK_BackgroundFileWriteJob *next = &batch[j+1];
      if (next->file != batch[i].file                                   ||
          next->decommit_after_write != batch[i].decommit_after_write  ||
          next->file_off != end_off                                     ||
          next->data.str != end_ptr) {
        break;
      }
      end_off += next->data.size;
      end_ptr += next->data.size;
    }
    U64 merged_size = end_off - batch[i].file_off;

    // Keep at most one MSF data node of dead-but-still-committed page data in
    // flight.  Besides bounding the memory lag versus synchronous decommit,
    // this prevents one multi-node VirtualFree from delaying all reclamation.
    U64 decommit_pipeline_cap = MB(128);
    U64 part_cap = writer->is_decommit_running && batch[i].decommit_after_write ? decommit_pipeline_cap : max_U64;
    for (U64 part_off = 0; part_off < merged_size; ) {
      U64 part_size = Min(merged_size - part_off, part_cap);
      U8 *part_ptr = batch[i].data.str + part_off;
      if (batch[i].file->is_open) {
        U64 write_size = lnk_write_file(&batch[i].file->file, batch[i].file_off + part_off, part_ptr, part_size);
        if (write_size != part_size) {
          batch[i].file->write_failed = 1;
        }
        batch[i].file->bytes_written += write_size;
        U64 done_now = ins_atomic_u64_add_eval(&writer->bytes_completed, write_size);
        writer->writes_issued += 1;
        if ((done_now >> 29) != ((done_now - write_size) >> 29)) { // every 512MiB
          lnk_log(LNK_Log_Timers, "[pdbw] t=%.3fs written=%.2f GiB enq=%.2f GiB jobs=%llu/%llu writes=%llu avg=%llu KiB", (F64)(now_time_us() - writer->begin_time_us) / 1e6, (F64)done_now / GB(1), (F64)ins_atomic_u64_eval(&writer->bytes_enqueued) / GB(1), writer->jobs_completed, ins_atomic_u64_eval(&writer->jobs_enqueued), writer->writes_issued, (done_now / writer->writes_issued) / KB(1));
        }
      }
      if (batch[i].decommit_after_write) {
        // The buffer is immutable and write-once (a sealed MSF stream run); once it is
        // on disk its pages are dead weight in the commit charge. Round INWARD to whole
        // OS pages so neighbors sharing the boundary pages are untouched.
        U64 lo = AlignPow2((U64)part_ptr, KB(4));
        U64 hi = ((U64)part_ptr + part_size) & ~(U64)(KB(4) - 1);
        if (lo < hi) {
          U64 decommit_size = hi - lo;
          if (writer->is_decommit_running) {
            while (ins_atomic_u64_eval(&writer->decommit_bytes_enqueued) -
                   ins_atomic_u64_eval(&writer->decommit_bytes_completed) + decommit_size > decommit_pipeline_cap) {
              sleep_ms(0);
            }
            ins_atomic_u64_add_eval(&writer->decommit_bytes_enqueued, decommit_size);
            LNK_BackgroundFileDecommitJob job = { .ptr = (void *)lo, .size = decommit_size };
            RingGuard guard = guarded_ring_open(writer->decommit_queue);
            B32 is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
            guarded_ring_close(&guard);
            Assert(is_written);
          } else {
            decommit_memory((void *)lo, decommit_size);
          }
        }
      }
      part_off += part_size;
    }
    if (batch[i].file->is_open) {
      writer->jobs_completed += (j + 1 - i);
    }
    i = j + 1;
  }
}

internal void
lnk_background_file_writer_thread(void *raw_writer)
{
  ProfBeginFunction();
  set_thread_namef("Background File Writer");

  LNK_BackgroundFileWriter *writer = raw_writer;

  writer->decommit_thread = thread_launch(lnk_background_file_decommit_thread, writer);
  writer->is_decommit_running = writer->decommit_thread.u64[0] != 0;

  U64 batch_cap = 8192;
  LNK_BackgroundFileWriteJob *batch = (LNK_BackgroundFileWriteJob *)reserve_memory(batch_cap * sizeof(batch[0]));
  commit_memory(batch, batch_cap * sizeof(batch[0]));

  for (B32 quit = 0; !quit; ) {
    LNK_BackgroundFileWriteJob job = {0};
    {
      RingGuard guard = guarded_ring_open(writer->queue);
      B32 is_read = guarded_ring_read_struct_or_wait(&guard, &job, max_U64);
      guarded_ring_close(&guard);
      Assert(is_read);
    }

    // batch up every immediately-available Write job (stop at the first control job --
    // it is an ordering barrier) so scattered small page runs coalesce into few large
    // sequential writes instead of thousands of QD-1 16KB-class WriteFile calls
    U64 batch_count = 0;
    B32 have_control = 0;
    LNK_BackgroundFileWriteJob control = {0};
    if (job.kind == LNK_BackgroundFileJobKind_Write) {
      batch[batch_count++] = job;
      while (batch_count < batch_cap) {
        LNK_BackgroundFileWriteJob more = {0};
        RingGuard guard   = guarded_ring_open(writer->queue);
        B32       is_read = guarded_ring_try_read(&guard, sizeof(more), &more);
        guarded_ring_close(&guard);
        if (!is_read) { break; }
        if (more.kind == LNK_BackgroundFileJobKind_Write) {
          batch[batch_count++] = more;
        } else {
          control      = more;
          have_control = 1;
          break;
        }
      }
    } else {
      control      = job;
      have_control = 1;
    }

    if (batch_count > 0) {
      lnk_background_file_writer_issue_write_batch(writer, batch, batch_count);
    }

    if (have_control) {
      if (control.kind == LNK_BackgroundFileJobKind_EndWriter) {
        quit = 1;
      } else if (control.kind == LNK_BackgroundFileJobKind_OpenFile) {
        lnk_background_file_writer_open_file_on_thread(control.file);
        lnk_log(LNK_Log_Timers, "[pdbw] t=%.3fs file open done (%S)", (F64)(now_time_us() - writer->begin_time_us) / 1e6, control.file->open_path);
      } else if (control.kind == LNK_BackgroundFileJobKind_EndFile) {
        lnk_background_file_writer_end_file_on_thread(control.file, control.expected_byte_count);
      }
    }
  }

  if (writer->is_decommit_running) {
    LNK_BackgroundFileDecommitJob job = {0};
    RingGuard guard = guarded_ring_open(writer->decommit_queue);
    B32 is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_written);
    thread_join(writer->decommit_thread, max_U64);
    writer->is_decommit_running = 0;
  }

  release_memory(batch, batch_cap * sizeof(batch[0]));
  ProfEnd();
}

internal void
lnk_background_file_writer_begin(LNK_BackgroundFileWriter *writer)
{
  ProfBegin("Background File Writer Begin");

  writer->queue_arena   = arena_alloc(.reserve_size = MB(2), .commit_size = MB(2), .name = "BACKGROUND_FILE_WRITE_QUEUE");
  writer->queue         = guarded_ring_alloc(writer->queue_arena, MB(1));
  writer->decommit_queue = guarded_ring_alloc(writer->queue_arena, KB(64));
  writer->begin_time_us = now_time_us();
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

  SLLQueuePush(writer->file_first, writer->file_last, file);

  // the open runs on the writer thread (multi-GB truncate of a previous
  // artifact is seconds of caller-thread stall otherwise); jobs on one file
  // are queue-ordered, so every write lands behind the open. An open failure
  // flips write_failed and is reported from the EndFile job.
  {
    LNK_BackgroundFileWriteJob job = { .kind = LNK_BackgroundFileJobKind_OpenFile, .file = file };
    RingGuard guard      = guarded_ring_open(writer->queue);
    B32       is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_written);
  }

  ProfEnd();
  return file;
}

internal void
lnk_background_file_writer_enqueue(LNK_BackgroundFileWriter *writer, LNK_BackgroundFile *file, U64 file_off, String8 data, B32 decommit_after_write)
{
  ProfBegin("Background File Writer Enqueue");

  Assert(writer->is_running);
  Assert(!file->is_finished); // is_open is owned by the writer thread (open is a queued job)

  if (data.size > 0) {
    ins_atomic_u64_add_eval(&writer->bytes_enqueued, data.size);
    ins_atomic_u64_add_eval(&writer->jobs_enqueued, 1);
    LNK_BackgroundFileWriteJob job = {
      .kind                 = LNK_BackgroundFileJobKind_Write,
      .file                 = file,
      .file_off             = file_off,
      .data                 = data,
      .decommit_after_write = decommit_after_write,
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
  Assert(!file->is_finished); // is_open is owned by the writer thread (open is a queued job)
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
    U64 wait_begin_us = now_time_us();
    LNK_BackgroundFileWriteJob job        = { .kind = LNK_BackgroundFileJobKind_EndWriter };
    RingGuard                  guard      = guarded_ring_open(writer->queue);
    B32                        is_written = guarded_ring_write_struct_or_wait(&guard, &job, max_U64);
    guarded_ring_close(&guard);
    Assert(is_written);
    thread_join(writer->thread, -1);
    lnk_log(LNK_Log_Timers, "[pdbw] t=%.3fs writer end: drain wait %.2f ms (enq=%.2f GiB done=%.2f GiB)",
            (F64)(now_time_us() - writer->begin_time_us) / 1e6,
            (F64)(now_time_us() - wait_begin_us) / 1000.0,
            (F64)writer->bytes_enqueued / GB(1), (F64)writer->bytes_completed / GB(1));
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
