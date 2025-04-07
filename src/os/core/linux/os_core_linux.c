// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ dan: Helpers

internal DateTime
os_lnx_date_time_from_tm(tm in, U32 msec)
{
  DateTime dt = {0};
  dt.sec  = in.tm_sec;
  dt.min  = in.tm_min;
  dt.hour = in.tm_hour;
  dt.day  = in.tm_mday-1;
  dt.mon  = in.tm_mon;
  dt.year = in.tm_year+1900;
  dt.msec = msec;
  return dt;
}

internal tm
os_lnx_tm_from_date_time(DateTime dt)
{
  tm result = {0};
  result.tm_sec = dt.sec;
  result.tm_min = dt.min;
  result.tm_hour= dt.hour;
  result.tm_mday= dt.day+1;
  result.tm_mon = dt.mon;
  result.tm_year= dt.year-1900;
  return result;
}

internal timespec
os_lnx_timespec_from_date_time(DateTime dt)
{
  tm tm_val = os_lnx_tm_from_date_time(dt);
  time_t seconds = timegm(&tm_val);
  timespec result = {0};
  result.tv_sec = seconds;
  return result;
}

internal DenseTime
os_lnx_dense_time_from_timespec(timespec in)
{
  DenseTime result = 0;
  // Check for zero timespec before converting
  if (in.tv_sec != 0 || in.tv_nsec != 0)
  {
    struct tm tm_time = {0};
    // Use gmtime_r for thread safety
    time_t seconds = in.tv_sec; // Copy to time_t
    if (gmtime_r(&seconds, &tm_time) != NULL) // Pass pointer to time_t
    {
        // Note: st_mtime/st_ctime etc. are usually UTC
        DateTime date_time = os_lnx_date_time_from_tm(tm_time, in.tv_nsec / Million(1));
        result = dense_time_from_date_time(date_time);
    }
  }
  return result;
}

internal DenseTime
os_lnx_dense_time_from_statx_timestamp(struct statx_timestamp in)
{
  DenseTime result = 0;
  // Check for zero timestamp before converting
  if (in.tv_sec != 0 || in.tv_nsec != 0)
  {
    struct tm tm_time = {0};
    // Use gmtime_r for thread safety
    time_t seconds = in.tv_sec; // Copy to time_t
    if (gmtime_r(&seconds, &tm_time) != NULL) // Pass pointer to time_t
    {
        // Convert nanoseconds to milliseconds for DateTime
        DateTime date_time = os_lnx_date_time_from_tm(tm_time, in.tv_nsec / Million(1));
        result = dense_time_from_date_time(date_time);
    }
  }
  return result;
}

internal FileProperties
os_lnx_file_properties_from_stat(struct stat *s)
{
  FileProperties props = {0};
  props.size = s->st_size;
  // NOTE(rjf): Fallback path for creation time. `os_properties_from_file*` functions
  // will attempt `statx` first. This function handles `stat` results.
#if defined(st_birthtim) // Check if st_birthtim is available (more specific than st_ctim)
  props.created = os_lnx_dense_time_from_timespec(s->st_birthtim);
#else // Fallback to st_ctim if st_birthtim is not available
  props.created = os_lnx_dense_time_from_timespec(s->st_ctim);
#endif
  props.modified = os_lnx_dense_time_from_timespec(s->st_mtim);
  if(S_ISDIR(s->st_mode)) // Use S_ISDIR macro for clarity
  {
    props.flags |= FilePropertyFlag_IsFolder;
  }
  return props;
}

internal int
os_lnx_futex(atomic_uint *uaddr, int futex_op, unsigned int val, const struct timespec *timeout, unsigned int val3)
{
    // NOTE(rjf): `syscall` needs non-const pointer for timeout for some reason.
    struct timespec *timeout_nonconst = (struct timespec*)timeout;
    return syscall(SYS_futex, uaddr, futex_op, val, timeout_nonconst, NULL, val3);
}

internal int
os_lnx_futex_wait(atomic_uint *uaddr, unsigned int expected_val, const struct timespec *timeout)
{
    // NOTE(rjf): timeout can be NULL for infinite wait. FUTEX_WAIT_PRIVATE is used for intra-process sync.
    return os_lnx_futex(uaddr, FUTEX_WAIT_PRIVATE, expected_val, timeout, 0);
}

internal int
os_lnx_futex_wake(atomic_uint *uaddr, int num_to_wake)
{
    // NOTE(rjf): FUTEX_WAKE_PRIVATE is used for intra-process sync.
    return os_lnx_futex(uaddr, FUTEX_WAKE_PRIVATE, num_to_wake, NULL, 0);
}

////////////////////////////////
//~ dan: Entities

internal OS_LNX_Entity *
os_lnx_entity_alloc(OS_LNX_EntityKind kind)
{
  OS_LNX_Entity *entity = 0;
  DeferLoop(pthread_mutex_lock(&os_lnx_state.entity_mutex),
            pthread_mutex_unlock(&os_lnx_state.entity_mutex))
  {
    entity = os_lnx_state.entity_free;
    if(entity)
    {
      SLLStackPop(os_lnx_state.entity_free);
    }
    else
    {
      entity = push_array_no_zero(os_lnx_state.entity_arena, OS_LNX_Entity, 1);
    }
  }
  MemoryZeroStruct(entity);
  entity->kind = kind;

  // Initialize based on kind
  switch(kind)
  {
    case OS_LNX_EntityKind_Mutex:
      atomic_init(&entity->mutex.futex, 0); // 0: unlocked
      atomic_init(&entity->mutex.owner_tid, 0);
      atomic_init(&entity->mutex.recursion_depth, 0);
      break;
    case OS_LNX_EntityKind_RWMutex:
      atomic_init(&entity->rw_mutex.futex, 0);
      break;
    case OS_LNX_EntityKind_ConditionVariable:
      atomic_init(&entity->cv.futex, 0); // Initialize sequence number/generation count
      break;
    case OS_LNX_EntityKind_SafeCallChain:
      // Initialization happens in os_safe_call
      break;
    // Other kinds (Thread, Process, Semaphore) are initialized elsewhere or don't need specific init here
    default: break;
  }

  return entity;
}

internal void
os_lnx_entity_release(OS_LNX_Entity *entity)
{
  // NOTE(rjf): Additional cleanup per-kind can go here if needed, e.g., free name strings for semaphores.
  // For SafeCallChain, cleanup (handler restoration) happens within os_safe_call itself.
  DeferLoop(pthread_mutex_lock(&os_lnx_state.entity_mutex),
            pthread_mutex_unlock(&os_lnx_state.entity_mutex))
  {
    SLLStackPush(os_lnx_state.entity_free, entity);
  }
}

////////////////////////////////
//~ dan: Thread Entry Point

internal void *
os_lnx_thread_entry_point(void *ptr)
{
  OS_LNX_Entity *entity = (OS_LNX_Entity *)ptr;
  OS_ThreadFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  func(thread_ptr);
  tctx_release();
  return 0;
}

////////////////////////////////
//~ dan: @os_hooks System/Process Info (Implemented Per-OS)

internal OS_SystemInfo *
os_get_system_info(void)
{
  return &os_lnx_state.system_info;
}

internal OS_ProcessInfo *
os_get_process_info(void)
{
  return &os_lnx_state.process_info;
}

internal String8
os_get_current_path(Arena *arena)
{
  char *cwdir = getcwd(0, 0);
  String8 string = push_str8_copy(arena, str8_cstring(cwdir));
  free(cwdir);
  return string;
}

internal U32
os_get_process_start_time_unix(void)
{
  Temp scratch = scratch_begin(0,0);
  U64 start_time = 0;
  pid_t pid = getpid();
  String8 path = push_str8f(scratch.arena, "/proc/%u", pid);
  struct stat st;
  int err = stat((char*)path.str, &st);
  if(err == 0)
  {
    start_time = st.st_mtime;
  }
  scratch_end(scratch);
  return (U32)start_time;
}

////////////////////////////////
//~ dan: @os_hooks Memory Allocation (Implemented Per-OS)

//- rjf: basic

internal void *
os_reserve(U64 size)
{
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(result == MAP_FAILED)
  {
    result = 0;
  }
  return result;
}

internal B32
os_commit(void *ptr, U64 size)
{
  int result = mprotect(ptr, size, PROT_READ|PROT_WRITE);
  if (result == -1) {
      perror("mprotect commit");
      // Consider returning 0 on failure, matching Win32 VirtualAlloc failure.
      // For now, maintain original behavior but log error.
      // return 0; 
  }
  return 1; // Original behavior: always return success
}

internal void
os_decommit(void *ptr, U64 size)
{
  int madvise_result = madvise(ptr, size, MADV_DONTNEED);
  if (madvise_result == -1) {
      perror("madvise decommit");
  }
  int mprotect_result = mprotect(ptr, size, PROT_NONE);
  if (mprotect_result == -1) {
      perror("mprotect decommit");
  }
}

internal void
os_release(void *ptr, U64 size)
{
  int result = munmap(ptr, size);
  if (result == -1) {
      perror("munmap release");
  }
}

//- rjf: large pages

internal void *
os_reserve_large(U64 size)
{
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
  if(result == MAP_FAILED)
  {
    result = 0;
  }
  return result;
}

internal B32
os_commit_large(void *ptr, U64 size)
{
  int result = mprotect(ptr, size, PROT_READ|PROT_WRITE);
  if (result == -1) {
      perror("mprotect commit_large");
      return 0;
  }
  return 1;
}

////////////////////////////////
//~ dan: @os_hooks Thread Info (Implemented Per-OS)

internal U32
os_tid(void)
{
  U32 result = gettid();
  return result;
}

internal void
os_set_thread_name(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = push_str8_copy(scratch.arena, name);
  pthread_t current_thread = pthread_self();
  int err = pthread_setname_np(current_thread, (char *)name_copy.str);
  if (err != 0) {
    // Optional: Log error, but it's often non-critical
    // fprintf(stderr, "Warning: Failed to set thread name: %s\n", strerror(err));
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ dan: @os_hooks Aborting (Implemented Per-OS)

internal void
os_abort(S32 exit_code)
{
  exit(exit_code);
}

////////////////////////////////
//~ dan: @os_hooks File System (Implemented Per-OS)

//- rjf: files

internal OS_Handle
os_file_open(OS_AccessFlags flags, String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  int lnx_flags = 0;
  if(flags & OS_AccessFlag_Read && flags & OS_AccessFlag_Write)
  {
    lnx_flags = O_RDWR;
  }
  else if(flags & OS_AccessFlag_Write)
  {
    lnx_flags = O_WRONLY;
  }
  else if(flags & OS_AccessFlag_Read)
  {
    lnx_flags = O_RDONLY;
  }
  if(flags & OS_AccessFlag_Append)
  {
    lnx_flags |= O_APPEND;
  }
  if(flags & (OS_AccessFlag_Write|OS_AccessFlag_Append))
  {
    lnx_flags |= O_CREAT;
  }
  int fd = open((char *)path_copy.str, lnx_flags, 0755);
  OS_Handle handle = {0};
  if(fd != -1)
  {
    handle.u64[0] = fd;
    // Set FD_CLOEXEC based on inheritance flag
    if (!(flags & OS_AccessFlag_Inherited))
    {
        int fd_flags = fcntl(fd, F_GETFD);
        if (fd_flags != -1)
        {
            fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC);
        }
        else
        {
             // Handle fcntl error (optional: log? close fd?)
             close(fd);
             fd = -1;
             handle = os_handle_zero();
        }
    }
  }
  scratch_end(scratch);
  return handle;
}

internal void
os_file_close(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { return; }
  int fd = (int)file.u64[0];
  close(fd);
}

internal U64
os_file_read(OS_Handle file, Rng1U64 rng, void *out_data)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  int fd = (int)file.u64[0];
  U64 total_num_bytes_to_read = dim_1u64(rng);
  U64 total_num_bytes_read = 0;
  U64 total_num_bytes_left_to_read = total_num_bytes_to_read;
  for(;total_num_bytes_left_to_read > 0;)
  {
    int read_result = pread(fd, (U8 *)out_data + total_num_bytes_read, total_num_bytes_left_to_read, rng.min + total_num_bytes_read);
    if(read_result >= 0)
    {
      total_num_bytes_read += read_result;
      total_num_bytes_left_to_read -= read_result;
    }
    else if(errno != EINTR)
    {
      break;
    }
  }
  return total_num_bytes_read;
}

internal U64
os_file_write(OS_Handle file, Rng1U64 rng, void *data)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  int fd = (int)file.u64[0];
  U64 total_num_bytes_to_write = dim_1u64(rng);
  U64 total_num_bytes_written = 0;
  U64 total_num_bytes_left_to_write = total_num_bytes_to_write;
  for(;total_num_bytes_left_to_write > 0;)
  {
    int write_result = pwrite(fd, (U8 *)data + total_num_bytes_written, total_num_bytes_left_to_write, rng.min + total_num_bytes_written);
    if(write_result >= 0)
    {
      total_num_bytes_written += write_result;
      total_num_bytes_left_to_write -= write_result;
    }
    else if(errno != EINTR)
    {
      break;
    }
  }
  return total_num_bytes_written;
}

internal B32
os_file_set_times(OS_Handle file, DateTime date_time)
{
  if(os_handle_match(file, os_handle_zero())) { return 0; }
  int fd = (int)file.u64[0];
  timespec time = os_lnx_timespec_from_date_time(date_time);
  timespec times[2] = {time, time};
  int futimens_result = futimens(fd, times);
  B32 good = (futimens_result != -1);
  return good;
}

internal FileProperties
os_properties_from_file(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { return (FileProperties){0}; }
  int fd = (int)file.u64[0];
  FileProperties props = {0};

#if defined(__NR_statx) // Check if statx syscall number is defined
  struct statx stx = {0};
  // Try statx first to get birth time if available
  if (syscall(__NR_statx, fd, "", AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW, STATX_BTIME | STATX_MTIME | STATX_SIZE | STATX_TYPE, &stx) == 0)
  {
    // Populate common properties
    props.size = stx.stx_size;
    props.modified = os_lnx_dense_time_from_statx_timestamp(stx.stx_mtime);
    if(S_ISDIR(stx.stx_mode)) { props.flags |= FilePropertyFlag_IsFolder; }

    // Use birth time if available
    if (stx.stx_mask & STATX_BTIME) {
        props.created = os_lnx_dense_time_from_statx_timestamp(stx.stx_btime);
    } else {
        // statx succeeded but didn't return birth time, fall back to fstat/st_ctim/st_birthtim logic
        struct stat fd_stat = {0};
        if (fstat(fd, &fd_stat) == 0) {
            props = os_lnx_file_properties_from_stat(&fd_stat); // Re-use stat logic for fallback create time
            // Keep size, modified, flags from statx as they are likely more accurate or available
            props.size = stx.stx_size;
            props.modified = os_lnx_dense_time_from_statx_timestamp(stx.stx_mtime);
            props.flags = 0; // Reset flags and re-set based on statx mode
            if(S_ISDIR(stx.stx_mode)) { props.flags |= FilePropertyFlag_IsFolder; }
        }
        // If fstat fails here, props remain partially filled from statx, created time is zero.
    }
    return props; // Return props obtained from statx (potentially with fallback create time)
  }
  // If statx failed, fall through to fstat below
#endif // __NR_statx

  // Fallback to fstat if statx is not available or failed
  struct stat fd_stat = {0};
  int fstat_result = fstat(fd, &fd_stat);
  if(fstat_result != -1)
  {
    props = os_lnx_file_properties_from_stat(&fd_stat);
  }
  return props;
}

internal OS_FileID
os_id_from_file(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { return (OS_FileID){0}; }
  int fd = (int)file.u64[0];
  struct stat fd_stat = {0};
  int fstat_result = fstat(fd, &fd_stat);
  OS_FileID id = {0};
  if(fstat_result != -1)
  {
    id.v[0] = fd_stat.st_dev;
    id.v[1] = fd_stat.st_ino;
  }
  return id;
}

internal B32
os_delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  String8 path_copy = push_str8_copy(scratch.arena, path);
  if(remove((char*)path_copy.str) != -1)
  {
    result = 1;
  }
  scratch_end(scratch);
  return result;
}

internal B32
os_copy_file_path(String8 dst, String8 src)
{
  B32 result = 0;
  OS_Handle src_h = os_file_open(OS_AccessFlag_Read, src);
  OS_Handle dst_h = os_file_open(OS_AccessFlag_Write, dst);
  if(!os_handle_match(src_h, os_handle_zero()) &&
     !os_handle_match(dst_h, os_handle_zero()))
  {
    int src_fd = (int)src_h.u64[0];
    int dst_fd = (int)dst_h.u64[0];
    FileProperties src_props = os_properties_from_file(src_h);
    U64 size = src_props.size;
    U64 total_bytes_copied = 0;
    U64 bytes_left_to_copy = size;
    B32 copy_error = 0;
    for(;bytes_left_to_copy > 0;)
    {
      off_t sendfile_off = total_bytes_copied;
      // Use ssize_t for sendfile result as it can return -1 on error
      ssize_t send_result = sendfile(dst_fd, src_fd, &sendfile_off, bytes_left_to_copy);
      if(send_result <= 0)
      {
        // Error or end of file reached prematurely
        copy_error = 1;
        break;
      }
      U64 bytes_copied = (U64)send_result;
      bytes_left_to_copy -= bytes_copied;
      total_bytes_copied += bytes_copied;
    }
    
    // Check if the entire file was copied without errors
    if (!copy_error && total_bytes_copied == size)
    {
      result = 1;
      // Set the destination file times to match the source modification time
      DateTime modified_dt = date_time_from_dense_time(src_props.modified);
      os_file_set_times(dst_h, modified_dt);
    }
  }
  os_file_close(src_h);
  os_file_close(dst_h);
  return result;
}

internal String8
os_full_path_from_path(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  char buffer[PATH_MAX] = {0};
  realpath((char *)path_copy.str, buffer);
  String8 result = push_str8_copy(arena, str8_cstring(buffer));
  scratch_end(scratch);
  return result;
}

internal B32
os_file_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  int access_result = access((char *)path_copy.str, F_OK);
  B32 result = 0;
  if(access_result == 0)
  {
    result = 1;
  }
  scratch_end(scratch);
  return result;
}

internal B32
os_folder_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32      exists    = 0;
  String8  path_copy = push_str8_copy(scratch.arena, path);
  DIR     *handle    = opendir((char*)path_copy.str);
  if(handle)
  {
    closedir(handle);
    exists = 1;
  }
  scratch_end(scratch);
  return exists;
}

internal FileProperties
os_properties_from_file_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  FileProperties props = {0};

#if defined(__NR_statx) // Check if statx syscall number is defined
  struct statx stx = {0};
  // Try statx first to get birth time if available
  // Use AT_SYMLINK_NOFOLLOW to match stat() behavior unless symlinks should be followed
  if (syscall(__NR_statx, AT_FDCWD, (char *)path_copy.str, AT_SYMLINK_NOFOLLOW, STATX_BTIME | STATX_MTIME | STATX_SIZE | STATX_TYPE, &stx) == 0)
  {
     // Populate common properties
    props.size = stx.stx_size;
    props.modified = os_lnx_dense_time_from_statx_timestamp(stx.stx_mtime);
    if(S_ISDIR(stx.stx_mode)) { props.flags |= FilePropertyFlag_IsFolder; }

    // Use birth time if available
    if (stx.stx_mask & STATX_BTIME) {
      props.created = os_lnx_dense_time_from_statx_timestamp(stx.stx_btime);
    } else {
      // statx succeeded but didn't return birth time, fall back to stat/st_ctim/st_birthtim logic
      struct stat f_stat = {0};
      if (stat((char *)path_copy.str, &f_stat) == 0) {
        props = os_lnx_file_properties_from_stat(&f_stat); // Re-use stat logic for fallback create time
        // Keep size, modified, flags from statx
        props.size = stx.stx_size;
        props.modified = os_lnx_dense_time_from_statx_timestamp(stx.stx_mtime);
        props.flags = 0; // Reset flags and re-set based on statx mode
        if(S_ISDIR(stx.stx_mode)) { props.flags |= FilePropertyFlag_IsFolder; }
      }
      // If stat fails here, props remain partially filled from statx, created time is zero.
    }
    scratch_end(scratch);
    return props; // Return props obtained from statx (potentially with fallback create time)
  }
  // If statx failed, fall through to stat below
#endif // __NR_statx

  // Fallback to stat if statx is not available or failed
  struct stat f_stat = {0};
  int stat_result = stat((char *)path_copy.str, &f_stat);
  if(stat_result != -1)
  {
    props = os_lnx_file_properties_from_stat(&f_stat);
  }
  scratch_end(scratch);
  return props;
}

//- rjf: file maps

internal OS_Handle
os_file_map_open(OS_AccessFlags flags, OS_Handle file)
{
  OS_Handle map = file;
  return map;
}

internal void
os_file_map_close(OS_Handle map)
{
  // NOTE(rjf): nothing to do; `map` handles are the same as `file` handles in
  // the linux implementation (on Windows they require separate handles)
}

internal void *
os_file_map_view_open(OS_Handle map, OS_AccessFlags flags, Rng1U64 range)
{
  if(os_handle_match(map, os_handle_zero())) { return 0; }
  int fd = (int)map.u64[0];
  int prot_flags = 0;
  if(flags & OS_AccessFlag_Write) { prot_flags |= PROT_WRITE; }
  if(flags & OS_AccessFlag_Read)  { prot_flags |= PROT_READ; }
  int map_flags = MAP_PRIVATE;
  void *base = mmap(0, dim_1u64(range), prot_flags, map_flags, fd, range.min);
  if(base == MAP_FAILED)
  {
    base = 0;
  }
  return base;
}

internal void
os_file_map_view_close(OS_Handle map, void *ptr, Rng1U64 range)
{
  munmap(ptr, dim_1u64(range));
  int result = munmap(ptr, dim_1u64(range));
  if (result == -1) {
    perror("munmap file_map_view_close");
  }
}

//- rjf: directory iteration

internal OS_FileIter *
os_file_iter_begin(Arena *arena, String8 path, OS_FileIterFlags flags)
{
  OS_FileIter *base_iter = push_array(arena, OS_FileIter, 1);
  base_iter->flags = flags;
  OS_LNX_FileIter *iter = (OS_LNX_FileIter *)base_iter->memory;
  MemoryZeroStruct(iter); // Initialize struct

  if(path.size == 0)
  {
    // Handle volume iteration (mount points on Linux)
    iter->is_volume_iter = 1;
    iter->mount_file_stream = setmntent(_PATH_MOUNTED, "r"); // Readonly access to mount table
    if (iter->mount_file_stream != NULL)
    {
        String8List mount_points_list = {0};
        struct mntent *mount_entry;
        String8 skip_fs_types[] = {
          str8_lit("proc"), str8_lit("sysfs"), str8_lit("devtmpfs"), str8_lit("tmpfs"), 
          str8_lit("cgroup"), str8_lit("cgroup2"), str8_lit("debugfs"), str8_lit("devpts"), 
          str8_lit("securityfs"), str8_lit("pstore"), str8_lit("autofs"), str8_lit("mqueue"),
          str8_lit("hugetlbfs"), str8_lit("binfmt_misc"), str8_lit("fusectl"), 
          str8_lit("tracefs"), str8_lit("configfs"), str8_lit("fuse.gvfsd-fuse"),
          // Add other pseudo/virtual/network FS types to skip as needed
        };
 
        while ((mount_entry = getmntent(iter->mount_file_stream)) != NULL)
        {
          // Filter out pseudo-filesystems and unwanted types
          B32 skip = 0;
          String8 fs_type = str8_cstring(mount_entry->mnt_type);
          for (U64 i = 0; i < ArrayCount(skip_fs_types); ++i) {
              if (str8_match(fs_type, skip_fs_types[i], 0)) {
                  skip = 1;
                  break;
              }
          }
          if (skip) {
              continue;
          }
          
          // TODO(rjf): Consider filtering based on mnt_opts (e.g., skip "noauto")?
 
          String8 mount_dir = str8_cstring(mount_entry->mnt_dir);
          // TODO(rjf): Check for duplicates if the same device is mounted multiple times?
          // For now, assume unique mount points reported by getmntent are sufficient.
          str8_list_push(arena, &mount_points_list, mount_dir);
        }
        iter->volume_mount_points = str8_array_from_list(arena, &mount_points_list);
        iter->volume_iter_idx = 0;
        // We keep mount_file_stream open until os_file_iter_end
    }
    else
    {
        // Failed to open mount table, log error? Volume iteration won't work.
        iter->volume_mount_points.count = 0; // Ensure iteration ends immediately
    }
    // If setmntent fails, volume_mount_points will be empty, and iteration will end immediately.
  }
  else
  {
    // Handle regular directory iteration
    iter->is_volume_iter = 0;
    String8 path_copy = push_str8_copy(arena, path);
    iter->dir = opendir((char *)path_copy.str);
    iter->path = path_copy;
    // iter->mount_file_stream will be NULL
  }

  return base_iter;
}

internal B32
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  B32 good = 0;
  OS_LNX_FileIter *lnx_iter = (OS_LNX_FileIter *)iter->memory;

  if (lnx_iter->is_volume_iter)
  {
    // Volume (Mount Point) Iteration - Unchanged
    if (lnx_iter->volume_iter_idx < lnx_iter->volume_mount_points.count)
    {
      MemoryZeroStruct(info_out);
      info_out->name = lnx_iter->volume_mount_points.v[lnx_iter->volume_iter_idx];
      info_out->props.flags |= FilePropertyFlag_IsFolder;
      // Props remain minimal for volume iteration
      lnx_iter->volume_iter_idx++;
      good = 1;
    }
  }
  else
  {
    // Regular Directory Iteration
    if (lnx_iter->dir == 0) // Check if opendir failed in begin
    {
      iter->flags |= OS_FileIterFlag_Done;
      return 0;
    }

    for (;;) // Loop until a valid, unfiltered entry is found or the directory ends
    {
      // 1. Read next directory entry
      errno = 0; // Reset errno before readdir
      lnx_iter->dp = readdir(lnx_iter->dir);

      if (lnx_iter->dp == 0) // End of directory or error
      {
        if (errno != 0) {
          // An error occurred during readdir, treat as end of iteration for now.
          // A more robust implementation might log the error here via errno.
          // Optional: Log error?
        }
        good = 0; // Mark as not found this iteration
        iter->flags |= OS_FileIterFlag_Done; // Mark iteration as finished
        break; // Exit loop
      }

      // 2. Filter "." and ".."
      char *d_name = lnx_iter->dp->d_name;
      if (d_name[0] == '.') {
        if (d_name[1] == 0 || (d_name[1] == '.' && d_name[2] == 0)) {
          continue; // Skip "." and ".."
        }
        // 3. Filter hidden files if requested
        if (iter->flags & OS_FileIterFlag_SkipHiddenFiles) {
          continue; // Skip hidden files (e.g., ".bashrc")
        }
      }

      // 4. Always use stat for definitive type and properties
      Temp scratch = scratch_begin(&arena, 1);
      String8 full_path = {0};
      if (lnx_iter->path.size == 1 && lnx_iter->path.str[0] == '/') { // Handle root case
        full_path = push_str8f(scratch.arena, "/%s", d_name);
      } else {
        full_path = push_str8f(scratch.arena, "%S/%s", lnx_iter->path, d_name);
      }
      struct stat st = {0};
      // Use stat() to follow symlinks, matching FindFirstFile behavior.
      int stat_result = stat((char *)full_path.str, &st);
      scratch_end(scratch);

      // 5. Handle stat result and apply filters
      if (stat_result != 0) {
        // Stat failed (e.g., broken link, permissions). Skip this entry.
        // Optional: Log errno here? For now, just skip.
        continue;
      }

      // Stat succeeded, check filters based on actual type
      B32 is_dir = S_ISDIR(st.st_mode);
      B32 filtered = 0;
      if (is_dir && (iter->flags & OS_FileIterFlag_SkipFolders)) {
        filtered = 1;
      } else if (!is_dir && (iter->flags & OS_FileIterFlag_SkipFiles)) {
        filtered = 1;
      }

      if (!filtered) {
        // 6. Populate output and break loop
        info_out->name = push_str8_copy(arena, str8_cstring(d_name));
        info_out->props = os_lnx_file_properties_from_stat(&st);
        good = 1; // Found a valid entry
        break;    // Exit loop
      }
      // If filtered, continue the loop to find the next entry
    }
  }

  // Return true only if a valid item was found *this call*
  return good;
}

internal void
os_file_iter_end(OS_FileIter *iter)
{
  OS_LNX_FileIter *lnx_iter = (OS_LNX_FileIter *)iter->memory;
  if (lnx_iter->is_volume_iter)
  {
    if (lnx_iter->mount_file_stream != 0)
    {
      endmntent(lnx_iter->mount_file_stream);
      lnx_iter->mount_file_stream = 0;
    }
  }
  else
  {
    if (lnx_iter->dir != 0)
    {
  closedir(lnx_iter->dir);
      lnx_iter->dir = 0;
    }
  }
}

//- rjf: directory creation

internal B32
os_make_directory(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  String8 path_copy = push_str8_copy(scratch.arena, path);
  if(mkdir((char*)path_copy.str, 0755) != -1)
  {
    result = 1;
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ dan: @os_hooks Shared Memory (Implemented Per-OS)

internal OS_Handle
os_shared_memory_alloc(U64 size, String8 name)
{
  OS_Handle result = os_handle_zero(); // Initialize to zero handle
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = push_str8_copy(scratch.arena, name);
  
  // Use O_CREAT | O_RDWR to create if not exists, or open if it does.
  // Use appropriate permissions (e.g., 0666).
  int id = shm_open((char *)name_copy.str, O_CREAT | O_RDWR, 0666); 
  
  if (id != -1) // Check if shm_open succeeded
  {
    // Truncate the file to the desired size.
    if (ftruncate(id, size) == 0) // Check if ftruncate succeeded
    {
      result.u64[0] = (U64)id; // Success path
    }
    else
    {
      perror("ftruncate shared memory");
      close(id); // Close the descriptor on ftruncate failure
      // Optional: shm_unlink the name if we created it and failed to size it? 
      // Might be too aggressive if another process already had it open.
      // For now, just close and return zero handle.
    }
  }
  else
  {
    perror("shm_open shared memory");
    // shm_open failed, result remains zero handle
  }
  
  scratch_end(scratch);
  return result;
}

internal OS_Handle
os_shared_memory_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = push_str8_copy(scratch.arena, name);
  int id = shm_open((char *)name_copy.str, O_RDWR, 0);
  OS_Handle result = {(U64)id};
  scratch_end(scratch);
  return result;
}

internal void
os_shared_memory_close(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())){return;}
  int id = (int)handle.u64[0];
  close(id);
}

internal void *
os_shared_memory_view_open(OS_Handle handle, Rng1U64 range)
{
  if(os_handle_match(handle, os_handle_zero())){return 0;}
  int id = (int)handle.u64[0];
  void *base = mmap(0, dim_1u64(range), PROT_READ|PROT_WRITE, MAP_SHARED, id, range.min);
  if(base == MAP_FAILED)
  {
    base = 0;
  }
  return base;
}

internal void
os_shared_memory_view_close(OS_Handle handle, void *ptr, Rng1U64 range)
{
  if(os_handle_match(handle, os_handle_zero())){return;}
  munmap(ptr, dim_1u64(range));
  int result = munmap(ptr, dim_1u64(range));
  if (result == -1) {
      perror("munmap shared_memory_view_close");
  }
}

////////////////////////////////
//~ dan: @os_hooks Time (Implemented Per-OS)

internal U64
os_now_microseconds(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  U64 result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
  return result;
}

internal U32
os_now_unix(void)
{
  time_t t = time(0);
  return (U32)t;
}

internal DateTime
os_now_universal_time(void)
{
  time_t t = 0;
  time(&t);
  struct tm universal_tm = {0};
  gmtime_r(&t, &universal_tm);
  DateTime result = os_lnx_date_time_from_tm(universal_tm, 0);
  return result;
}

internal DateTime
os_universal_time_from_local(DateTime *date_time)
{
  // local DateTime -> universal time_t
  tm local_tm = os_lnx_tm_from_date_time(*date_time);
  local_tm.tm_isdst = -1;
  time_t universal_t = mktime(&local_tm);
  
  // universal time_t -> DateTime
  tm universal_tm = {0};
  gmtime_r(&universal_t, &universal_tm);
  DateTime result = os_lnx_date_time_from_tm(universal_tm, 0);
  return result;
}

internal DateTime
os_local_time_from_universal(DateTime *date_time)
{
  // universal DateTime -> local time_t
  tm universal_tm = os_lnx_tm_from_date_time(*date_time);
  universal_tm.tm_isdst = -1;
  time_t universal_t = timegm(&universal_tm);
  tm local_tm = {0};
  localtime_r(&universal_t, &local_tm);
  
  // local tm -> DateTime
  DateTime result = os_lnx_date_time_from_tm(local_tm, 0);
  return result;
}

internal void
os_sleep_milliseconds(U32 msec)
{
  usleep(msec*Thousand(1));
}

////////////////////////////////
//~ dan: @os_hooks Child Processes (Implemented Per-OS)

internal OS_Handle
os_process_launch(OS_ProcessLaunchParams *params)
{
  OS_Handle result = {0};
  Temp scratch = scratch_begin(0, 0);
  
  pid_t pid = fork();
  
  if (pid == -1)
  {
    // Fork failed
    perror("fork");
  }
  else if (pid == 0)
  {
    // Child process
    
    // Redirect stdout if requested
    if (!os_handle_match(params->stdout_file, os_handle_zero()))
    {
      int fd = (int)params->stdout_file.u64[0];
      if (dup2(fd, STDOUT_FILENO) == -1) { perror("dup2 stdout"); _exit(1); }
      // Close the original fd in child, not strictly necessary as exec replaces fds,
      // but good practice if exec fails. If Inherited flag was set, this might
      // close the parent's handle too if not careful about handle inheritance flags
      // during file open. Assuming non-inherited handles for simplicity here.
      // close(fd); // Let's skip closing for now, relies on correct OS_AccessFlag_Inherited usage
    }
    
    // Redirect stderr if requested
    if (!os_handle_match(params->stderr_file, os_handle_zero()))
    {
      int fd = (int)params->stderr_file.u64[0];
      if (dup2(fd, STDERR_FILENO) == -1) { perror("dup2 stderr"); _exit(1); }
      // close(fd);
    }
    
    // Redirect stdin if requested
    if (!os_handle_match(params->stdin_file, os_handle_zero()))
    {
      int fd = (int)params->stdin_file.u64[0];
      if (dup2(fd, STDIN_FILENO) == -1) { perror("dup2 stdin"); _exit(1); }
      // close(fd);
    }
    
    // Change directory if requested
    if (params->path.size > 0)
    {
      String8 path_copy = push_str8_copy(scratch.arena, params->path);
      if (chdir((char *)path_copy.str) == -1)
      {
        perror("chdir");
        _exit(1);
      }
    }
    
    // Prepare command line arguments
    char **argv = push_array(scratch.arena, char *, params->cmd_line.node_count + 1);
    U64 argv_idx = 0;
    for(String8Node *n = params->cmd_line.first; n != 0; n = n->next)
    {
      String8 arg_copy = push_str8_copy(scratch.arena, n->string);
      argv[argv_idx++] = (char *)arg_copy.str;
    }
    argv[argv_idx] = NULL;
    
    // Prepare environment
    char **envp = NULL;
    if (params->inherit_env == 0 && params->env.node_count > 0)
    {
        // Use only specified environment variables
        envp = push_array(scratch.arena, char *, params->env.node_count + 1);
        U64 envp_idx = 0;
        for (String8Node *n = params->env.first; n != 0; n = n->next)
        {
            String8 env_copy = push_str8_copy(scratch.arena, n->string);
            envp[envp_idx++] = (char *)env_copy.str;
        }
        envp[envp_idx] = NULL;
    }
    else if (params->inherit_env != 0 && params->env.node_count > 0)
    {
        // Inherit existing environment and append/overwrite specified ones
        extern char **environ;
        String8List merged_env_list = {0};

        // Pass 1: Add inherited variables if they are not overridden by custom ones.
        if (environ)
        {
            for (char **existing_env = environ; *existing_env; ++existing_env)
            {
                String8 existing_var = str8_cstring(*existing_env);
                U64 eq_pos_existing = str8_find_needle(existing_var, 0, str8_lit("="), 0);
                String8 existing_key = str8_prefix(existing_var, eq_pos_existing);
                B32 overwritten = 0;

                // Check if this key exists in the custom environment variables
                for (String8Node *n = params->env.first; n != 0; n = n->next)
                {
                    String8 custom_var = n->string;
                    U64 eq_pos_custom = str8_find_needle(custom_var, 0, str8_lit("="), 0);
                    String8 custom_key = str8_prefix(custom_var, eq_pos_custom);
                    if (str8_match(existing_key, custom_key, 0)) {
                        overwritten = 1;
                        break;
                    }
                }

                if (!overwritten)
                {
                    // Add the inherited variable to the list
                    str8_list_push(scratch.arena, &merged_env_list, existing_var);
                }
            }
        }

        // Pass 2: Add all custom variables (handles appending new and overwriting existing)
        for (String8Node *n = params->env.first; n != 0; n = n->next)
        {
            str8_list_push(scratch.arena, &merged_env_list, n->string);
        }

        // Convert the list to the final envp array
        envp = push_array(scratch.arena, char *, merged_env_list.node_count + 1);
        U64 envp_idx = 0;
        for (String8Node *n = merged_env_list.first; n != 0; n = n->next)
        {
            // Need to copy the strings again as they might point to transient memory (environ)
            String8 env_copy = push_str8_copy(scratch.arena, n->string);
            envp[envp_idx++] = (char *)env_copy.str;
        }
        envp[envp_idx] = NULL; // Null-terminate the list
    }
    else if (params->inherit_env == 0 && params->env.node_count == 0)
    {
        // Use minimal empty environment
        envp = push_array(scratch.arena, char *, 1);
        envp[0] = NULL;
    }
    else // inherit_env == 1 and params->env.node_count == 0
    {
        // Inherit existing environment (default behavior of execvp)
        extern char **environ;
        envp = environ;
    }
    
    // Execute the command using execvpe which allows specifying environment
    execvpe(argv[0], argv, envp);
    
    // execvpe only returns on error
    perror("execvpe");
    _exit(127); // Indicate exec failed
  }
  else
  {
    // Parent process
    OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_Process);
    if (entity)
    {
      entity->process.pid = pid;
      result.u64[0] = (U64)entity;
    }
    else
    {
      // Failed to allocate entity, kill the child? Or let it become a zombie?
      // Let's just return zero handle for now.
      kill(pid, SIGKILL); // Or SIGTERM
      waitpid(pid, NULL, 0); // Reap the killed child
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal B32
os_process_join(OS_Handle handle, U64 endt_us)
{
  if (os_handle_match(handle, os_handle_zero())) { return 0; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)handle.u64[0];
  if (entity == 0 || entity->kind != OS_LNX_EntityKind_Process) { return 0; }
  
  pid_t pid = entity->process.pid;
  B32 joined = 0;
  
  for (;;)
  {
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    
    if (result == pid)
    {
      // Process exited or was signalled
      joined = 1;
      break;
    }
    else if (result == 0)
    {
      // Process still running
      U64 now_us = os_now_microseconds();
      if (now_us >= endt_us)
      {
        // Timeout reached
        break;
      }
      
      // Calculate sleep time
      U64 remaining_us = endt_us - now_us;
      U32 sleep_ms = (U32)(remaining_us / 1000);
      // Prevent sleeping for too long if timeout is very close, minimum sleep 1ms
      sleep_ms = Max(1, Min(sleep_ms, 100));
      os_sleep_milliseconds(sleep_ms);
    }
    else
    {
      // Error in waitpid (e.g., process doesn't exist, permissions issue)
      perror("waitpid");
      break;
    }
  }
  
  if (joined)
  {
    os_lnx_entity_release(entity);
  }
  
  return joined;
}

internal void
os_process_detach(OS_Handle handle)
{
  if (os_handle_match(handle, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)handle.u64[0];
  if (entity != 0 && entity->kind == OS_LNX_EntityKind_Process)
  {
    // We just release our tracking entity. The OS will handle the actual
    // process becoming an orphan or being re-parented to init (pid 1).
    os_lnx_entity_release(entity);
  }
}

////////////////////////////////
//~ dan: @os_hooks Threads (Implemented Per-OS)

internal OS_Handle
os_thread_launch(OS_ThreadFunctionType *func, void *ptr, void *params)
{
  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_Thread);
  entity->thread.func = func;
  entity->thread.ptr = ptr;
  {
    int pthread_result = pthread_create(&entity->thread.handle, 0, os_lnx_thread_entry_point, entity);
    if(pthread_result == -1)
    {
      os_lnx_entity_release(entity);
      entity = 0;
    }
  }
  OS_Handle handle = {(U64)entity};
  return handle;
}

internal B32
os_thread_join(OS_Handle handle, U64 endt_us)
{
  if(os_handle_match(handle, os_handle_zero())) { return 0; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)handle.u64[0];
  B32 result = 0;
  int join_result = 0;

  if (endt_us == max_U64)
  {
    // Infinite wait
    join_result = pthread_join(entity->thread.handle, 0);
  }
  else
  {
    // Timed wait using absolute deadline
    struct timespec ts_deadline;
    ts_deadline.tv_sec = endt_us / Million(1);
    ts_deadline.tv_nsec = (endt_us % Million(1)) * Thousand(1);

    // NOTE: Requires _GNU_SOURCE, which is defined in os_core_linux.h
    join_result = pthread_timedjoin_np(entity->thread.handle, 0, &ts_deadline);
  }

  // Join succeeded if return code is 0. ETIMEDOUT indicates timeout.
  result = (join_result == 0);

  // Release the entity regardless of join success, similar to Win32 closing the handle.
  // The caller should not attempt to join again.
  os_lnx_entity_release(entity);
  return result;
}

internal void
os_thread_detach(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)handle.u64[0];
  os_lnx_entity_release(entity);
}

////////////////////////////////
//~ dan: @os_hooks Synchronization Primitives (Implemented Per-OS)

//- rjf: mutexes

internal OS_Handle
os_mutex_alloc(void)
{
  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_Mutex);
  // Initialization is now done in os_lnx_entity_alloc
  if(entity == 0) // Check if allocation failed
  {
      return os_handle_zero();
  }
  OS_Handle handle = {(U64)entity};
  return handle;
}

internal void
os_mutex_release(OS_Handle mutex)
{
  if(os_handle_match(mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)mutex.u64[0];
  // No explicit destruction needed for atomics
  os_lnx_entity_release(entity);
}

internal void
os_mutex_take(OS_Handle mutex)
{
  if(os_handle_match(mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)mutex.u64[0];
  U32 tid = os_tid();

  // Check for recursion
  if (atomic_load_explicit(&entity->mutex.owner_tid, memory_order_relaxed) == tid) {
    atomic_fetch_add_explicit(&entity->mutex.recursion_depth, 1, memory_order_relaxed);
    return;
  }

  // Try to acquire the lock uncontended
  unsigned int zero = 0;
  if (atomic_compare_exchange_strong_explicit(&entity->mutex.futex, &zero, 1, memory_order_acquire, memory_order_relaxed))
  {
    // Acquired uncontended
    atomic_store_explicit(&entity->mutex.owner_tid, tid, memory_order_relaxed);
    atomic_store_explicit(&entity->mutex.recursion_depth, 1, memory_order_relaxed);
    return;
  }

  // Lock is contended, prepare to wait
  do {
    // Mark as contended if it's currently locked uncontended (state 1)
    unsigned int one = 1;
    if (atomic_compare_exchange_strong_explicit(&entity->mutex.futex, &one, 2, memory_order_relaxed, memory_order_relaxed))
    {
        // Successfully marked as contended, now wait
        os_lnx_futex_wait(&entity->mutex.futex, 2, NULL); // Wait indefinitely if it's 2 (contended)
    }

    // After waking up or if it was already contended (state 2), try to acquire again.
    // We need to transition from 0 (unlocked) to 2 (contended lock) because other waiters might exist.
    zero = 0;
  } while (!atomic_compare_exchange_strong_explicit(&entity->mutex.futex, &zero, 2, memory_order_acquire, memory_order_relaxed));

  // Acquired contended lock
  atomic_store_explicit(&entity->mutex.owner_tid, tid, memory_order_relaxed);
  atomic_store_explicit(&entity->mutex.recursion_depth, 1, memory_order_relaxed);
}

internal void
os_mutex_drop(OS_Handle mutex)
{
  if(os_handle_match(mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)mutex.u64[0];
  U32 tid = os_tid();

  // Check ownership (optional but good practice)
  Assert(atomic_load_explicit(&entity->mutex.owner_tid, memory_order_relaxed) == tid);

  // Decrement recursion depth
  unsigned int rec_depth = atomic_fetch_sub_explicit(&entity->mutex.recursion_depth, 1, memory_order_relaxed);

  // If recursion depth > 1 before decrement, we're done
  if (rec_depth > 1) {
    return;
  }

  // Reset owner TID as we are fully unlocking
  atomic_store_explicit(&entity->mutex.owner_tid, 0, memory_order_relaxed);

  // Release the lock
  // Transition from 2 (contended) or 1 (uncontended) to 0 (unlocked)
  if (atomic_exchange_explicit(&entity->mutex.futex, 0, memory_order_release) == 2)
  {
    // If it was contended (state 2), wake one waiting thread
    os_lnx_futex_wake(&entity->mutex.futex, 1);
  }
}

//- rjf: reader/writer mutexes

internal OS_Handle
os_rw_mutex_alloc(void)
{
  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_RWMutex);
  // Initialization is now done in os_lnx_entity_alloc
  if(entity == 0) // Check if allocation failed
  {
      return os_handle_zero();
  }
  OS_Handle handle = {(U64)entity};
  return handle;
}

internal void
os_rw_mutex_release(OS_Handle rw_mutex)
{
  if(os_handle_match(rw_mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)rw_mutex.u64[0];
  // No explicit destruction needed for atomics
  os_lnx_entity_release(entity);
}

internal void
os_rw_mutex_take_r(OS_Handle rw_mutex)
{
  if(os_handle_match(rw_mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)rw_mutex.u64[0];
  atomic_uint *futex = &entity->rw_mutex.futex;

  unsigned int current_state = atomic_load_explicit(futex, memory_order_relaxed);
  while (1) {
      // Check if a writer holds the lock or writers are waiting (writer preference)
      if (current_state & (RW_MUTEX_WRITE_HELD_MASK | RW_MUTEX_WRITE_WAIT_MASK)) {
          // Mark that readers are waiting
          unsigned int expected = current_state;
          unsigned int desired = current_state | RW_MUTEX_READ_WAIT_MASK;
          // Attempt to set the wait flag, weak is fine as we'll re-check and wait anyway
          atomic_compare_exchange_weak_explicit(futex, &expected, desired, memory_order_relaxed, memory_order_relaxed);
          // Use the potentially updated state (expected) for waiting
          current_state = expected | RW_MUTEX_READ_WAIT_MASK; // Assume wait flag is now set for the wait condition
          
          // Wait if still necessary (writer held or waiting)
          os_lnx_futex_wait(futex, current_state, NULL); 
          
          // Reload state after wake and retry loop
          current_state = atomic_load_explicit(futex, memory_order_relaxed);
          continue;
      }
      
      // Attempt to increment reader count
      unsigned int new_state = current_state + 1;
      // Check for reader count overflow (highly unlikely with 16 bits)
      if ((new_state & RW_MUTEX_READ_MASK) == 0) { 
          current_state = atomic_load_explicit(futex, memory_order_relaxed); // Reload state on overflow attempt
          continue; 
      }
      
      // Try acquiring the read lock using weak CAS
      if (atomic_compare_exchange_weak_explicit(futex, &current_state, new_state, memory_order_acquire, memory_order_relaxed)) {
          // Successfully acquired read lock
          break;
      }
      // CAS failed, loop continues with updated current_state (automatically loaded by failed CAS)
  }
}

internal void
os_rw_mutex_drop_r(OS_Handle rw_mutex)
{
  if(os_handle_match(rw_mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)rw_mutex.u64[0];
  atomic_uint *futex = &entity->rw_mutex.futex;

  // Decrement reader count and get the state *before* decrementing
  unsigned int prev_state = atomic_fetch_sub_explicit(futex, 1, memory_order_release);
  
  unsigned int readers_before = prev_state & RW_MUTEX_READ_MASK;
  unsigned int writers_waiting = prev_state & RW_MUTEX_WRITE_WAIT_MASK;

  // If this was the last reader AND writers are waiting
  if (readers_before == 1 && writers_waiting) {
      // Wake one potential writer. The woken writer will handle clearing the wait flag if it acquires the lock.
      os_lnx_futex_wake(futex, 1); 
  }
}

internal void
os_rw_mutex_take_w(OS_Handle rw_mutex)
{
  if(os_handle_match(rw_mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)rw_mutex.u64[0];
  atomic_uint *futex = &entity->rw_mutex.futex;

  unsigned int current_state = atomic_load_explicit(futex, memory_order_relaxed);
  while (1) {
      if (current_state == 0) { // Lock is free
          // Try to acquire uncontended using weak CAS
          if (atomic_compare_exchange_weak_explicit(futex, &current_state, RW_MUTEX_WRITE_HELD_MASK, memory_order_acquire, memory_order_relaxed)) {
               return; // Acquired write lock
          }
          // CAS failed, loop continues with updated current_state
          continue;
      }
  
      // Lock is not free (readers exist, writer holds, or waiters exist)
      unsigned int expected = current_state;
      unsigned int desired = current_state | RW_MUTEX_WRITE_WAIT_MASK;
      
      // Mark that a writer is waiting, if not already marked
      if (!(current_state & RW_MUTEX_WRITE_WAIT_MASK)) {
          atomic_compare_exchange_weak_explicit(futex, &expected, desired, memory_order_relaxed, memory_order_relaxed);
          // Use the potentially updated state for the wait check
          current_state = expected | RW_MUTEX_WRITE_WAIT_MASK;
      } else {
          current_state = expected; // Use the state loaded or from failed CAS
      }
  
      // Wait if the lock is held by readers or another writer
      if (current_state & (RW_MUTEX_READ_MASK | RW_MUTEX_WRITE_HELD_MASK)) {
          os_lnx_futex_wait(futex, current_state, NULL); // Wait on the current state
          // Reload state after wake and retry loop
          current_state = atomic_load_explicit(futex, memory_order_relaxed);
      } else {
           // Lock is not held (only waiter flags might be set). Try to acquire.
           expected = current_state & ~RW_MUTEX_WRITE_HELD_MASK; // Expect lock not held
           desired = (expected | RW_MUTEX_WRITE_HELD_MASK) & ~RW_MUTEX_WRITE_WAIT_MASK; // Set HELD, clear our WAIT bit
  
           if (atomic_compare_exchange_weak_explicit(futex, &expected, desired, memory_order_acquire, memory_order_relaxed)) {
              // Acquired write lock
              return;
           }
           // CAS failed, retry loop with updated current_state (expected)
           current_state = expected; 
      }
  }
}

internal void
os_rw_mutex_drop_w(OS_Handle rw_mutex)
{
  if(os_handle_match(rw_mutex, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)rw_mutex.u64[0];
  atomic_uint *futex = &entity->rw_mutex.futex;

  // Atomically clear the write-held flag, get the state *before* clearing.
  unsigned int prev_state = atomic_fetch_and_explicit(futex, ~RW_MUTEX_WRITE_HELD_MASK, memory_order_release);
  
  // Check wait flags from the previous state to decide who to wake.
  if (prev_state & RW_MUTEX_WRITE_WAIT_MASK) {
      // Writers were waiting. Wake one.
      os_lnx_futex_wake(futex, 1); 
  } else if (prev_state & RW_MUTEX_READ_WAIT_MASK) {
      // No writers waiting, but readers were. Try to clear the read wait flag and wake all readers.
      unsigned int current_state_after_drop = atomic_load_explicit(futex, memory_order_relaxed);
      while(current_state_after_drop & RW_MUTEX_READ_WAIT_MASK) {
          unsigned int desired = current_state_after_drop & ~RW_MUTEX_READ_WAIT_MASK;
          if(atomic_compare_exchange_weak_explicit(futex, &current_state_after_drop, desired, memory_order_relaxed, memory_order_relaxed)) {
               break; // Flag cleared
          }
          // retry if CAS failed, current_state_after_drop is updated by CAS
      }
      os_lnx_futex_wake(futex, INT_MAX); // Wake all potentially waiting readers
  }
}

//- rjf: condition variables

internal OS_Handle
os_condition_variable_alloc(void)
{
  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_ConditionVariable);
  // Initialization is now done in os_lnx_entity_alloc
  if(entity == 0) // Check if allocation failed
  {
      return os_handle_zero();
  }
  OS_Handle handle = {(U64)entity};
  return handle;
}

internal void
os_condition_variable_release(OS_Handle cv)
{
  if(os_handle_match(cv, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)cv.u64[0];
  // No explicit destruction needed for atomics
  os_lnx_entity_release(entity);
}

internal B32
os_condition_variable_wait(OS_Handle cv, OS_Handle mutex, U64 endt_us)
{
  if(os_handle_match(cv, os_handle_zero())) { return 0; }
  OS_LNX_Entity *cv_entity = (OS_LNX_Entity *)cv.u64[0];
  OS_LNX_Entity *mutex_entity = (OS_LNX_Entity *)mutex.u64[0];
  struct timespec endt_timespec;
  endt_timespec.tv_sec = endt_us/Million(1);
  endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us/Million(1))*Million(1));

  // 1. Read the current sequence number
  unsigned int current_seq = atomic_load_explicit(&cv_entity->cv.futex, memory_order_relaxed);
 
  // 2. Unlock the associated *external* mutex (must be done by caller before wait)
  //    IMPORTANT: The API expects the mutex passed here is the one protecting the condition.
  //    The original code's use of rwlock_mutex_handle inside the CV entity was incorrect.
  os_mutex_drop(mutex); // Release the *external* mutex
 
  // 3. Wait on the futex, checking if the sequence number has changed
  int wait_result = os_lnx_futex_wait(&cv_entity->cv.futex, current_seq, &endt_timespec);
 
  // 4. Re-lock the external mutex (must be done by caller *after* wait returns)
  os_mutex_take(mutex);
 
  // 5. Determine result
  B32 result = (wait_result != -1 || errno != ETIMEDOUT);
  // Futex wait returns 0 on success (woken), -1 on error. Check errno for timeout.
  return result;
}

internal B32
os_condition_variable_wait_rw_r(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  if(os_handle_match(cv, os_handle_zero()) || os_handle_match(mutex_rw, os_handle_zero())) { return 0; }
  OS_LNX_Entity *cv_entity = (OS_LNX_Entity *)cv.u64[0];
  OS_LNX_Entity *rw_mutex_entity = (OS_LNX_Entity *)mutex_rw.u64[0]; // Assuming mutex_rw is the RW mutex handle
  
  if (cv_entity == 0 || cv_entity->kind != OS_LNX_EntityKind_ConditionVariable ||
      rw_mutex_entity == 0 || rw_mutex_entity->kind != OS_LNX_EntityKind_RWMutex)
  {
      // Invalid handle types
      return 0;
  }

  struct timespec *timeout_ts_ptr = 0; // Default to infinite wait
  struct timespec timeout_ts;
  if (endt_us != max_U64)
  {
      // Check for immediate timeout
      U64 now_us = os_now_microseconds();
      if (now_us >= endt_us) {
          errno = ETIMEDOUT; // Set errno for timeout
          return 0; // Timeout already expired
      }
      // Use monotonic clock for absolute timeout
      timeout_ts.tv_sec = endt_us / Million(1);
      timeout_ts.tv_nsec = (endt_us % Million(1)) * 1000;
      timeout_ts_ptr = &timeout_ts;
  }
  
  // Similar logic to normal CV wait, but using the RW lock
  unsigned int current_seq = atomic_load_explicit(&cv_entity->cv.futex, memory_order_relaxed);
 
  // Unlock the *external* RW lock (Read mode)
  os_rw_mutex_drop_r(mutex_rw);
 
  // Wait on the futex using the sequence number and timeout
  int wait_result = os_lnx_futex_wait(&cv_entity->cv.futex, current_seq, timeout_ts_ptr);
 
  // Re-lock the *external* RW lock (Read mode)
  os_rw_mutex_take_r(mutex_rw);
 
  // Check result: 0 means woken up, -1 means error. Check errno for timeout.
  B32 result = (wait_result == 0);
  if (wait_result == -1 && errno == ETIMEDOUT) {
      result = 0; // Explicitly return false on timeout
  } else if (wait_result == -1) {
      // Some other error occurred during wait
      perror("os_lnx_futex_wait in os_condition_variable_wait_r");
      result = 0; // Consider other errors as failure
  }
  
  return result;
}

internal B32
os_condition_variable_wait_rw_w(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  if(os_handle_match(cv, os_handle_zero()) || os_handle_match(mutex_rw, os_handle_zero())) { return 0; }
  OS_LNX_Entity *cv_entity = (OS_LNX_Entity *)cv.u64[0];
  OS_LNX_Entity *rw_mutex_entity = (OS_LNX_Entity *)mutex_rw.u64[0]; // Assuming mutex_rw is the RW mutex handle
  
  if (cv_entity == 0 || cv_entity->kind != OS_LNX_EntityKind_ConditionVariable ||
      rw_mutex_entity == 0 || rw_mutex_entity->kind != OS_LNX_EntityKind_RWMutex)
  {
      // Invalid handle types
      return 0;
  }

  struct timespec *timeout_ts_ptr = 0; // Default to infinite wait
  struct timespec timeout_ts;
  if (endt_us != max_U64)
  {
      // Check for immediate timeout
      U64 now_us = os_now_microseconds();
      if (now_us >= endt_us) {
          errno = ETIMEDOUT; // Set errno for timeout
          return 0; // Timeout already expired
      }
      // Use monotonic clock for absolute timeout
      timeout_ts.tv_sec = endt_us / Million(1);
      timeout_ts.tv_nsec = (endt_us % Million(1)) * 1000;
      timeout_ts_ptr = &timeout_ts;
  }

  // Similar logic to normal CV wait, but using the RW lock
  unsigned int current_seq = atomic_load_explicit(&cv_entity->cv.futex, memory_order_relaxed);
 
  // Unlock the *external* RW lock (Write mode)
  os_rw_mutex_drop_w(mutex_rw);
 
  // Wait on the futex using the sequence number and timeout
  int wait_result = os_lnx_futex_wait(&cv_entity->cv.futex, current_seq, timeout_ts_ptr);
 
  // Re-lock the *external* RW lock (Write mode)
  os_rw_mutex_take_w(mutex_rw);
 
  // Check result: 0 means woken up, -1 means error. Check errno for timeout.
  B32 result = (wait_result == 0);
  if (wait_result == -1 && errno == ETIMEDOUT) {
      result = 0; // Explicitly return false on timeout
  } else if (wait_result == -1) {
      // Some other error occurred during wait
      perror("os_lnx_futex_wait in os_condition_variable_wait_w");
      result = 0; // Consider other errors as failure
  }
  
  return result;
}

internal void
os_condition_variable_signal(OS_Handle cv)
{
  if(os_handle_match(cv, os_handle_zero())) { return; }
  OS_LNX_Entity *cv_entity = (OS_LNX_Entity *)cv.u64[0];
  // Increment sequence number to mark a change
  atomic_fetch_add_explicit(&cv_entity->cv.futex, 1, memory_order_relaxed);
  // Wake one waiting thread
  os_lnx_futex_wake(&cv_entity->cv.futex, 1);
}

internal void
os_condition_variable_broadcast(OS_Handle cv)
{
  if(os_handle_match(cv, os_handle_zero())) { return; }
  OS_LNX_Entity *cv_entity = (OS_LNX_Entity *)cv.u64[0];
  // Increment sequence number
  atomic_fetch_add_explicit(&cv_entity->cv.futex, 1, memory_order_relaxed);
  // Wake all waiting threads
  os_lnx_futex_wake(&cv_entity->cv.futex, INT_MAX); // INT_MAX wakes all
}

//- rjf: cross-process semaphores

internal OS_Handle
os_semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  // NOTE(rjf): max_count ignored on linux - POSIX semaphores don't have a max count concept
  // like Windows semaphores do. We clamp initial_count to SEM_VALUE_MAX.
  if (initial_count > SEM_VALUE_MAX)
  {
    initial_count = SEM_VALUE_MAX;
  }

  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_Semaphore);
  if (!entity)
  {
    return os_handle_zero();
  }

  if (name.size > 0)
  {
    // Named Semaphore
    entity->semaphore.is_named = 1;
    Temp scratch = scratch_begin(0, 0); // Use scratch for temporary name manipulation

    // POSIX requires named semaphores to start with '/'
    String8 name_copy = name;
    if (name.str[0] != '/')
    {
      name_copy = push_str8f(scratch.arena, "/%S", name);
    }

    // Store the final name (potentially prefixed) in the entity arena
    entity->semaphore.name = push_str8_copy(os_lnx_state.entity_arena, name_copy);

    // O_EXCL ensures we create a new one or fail if it exists
    entity->semaphore.named_handle = sem_open((char *)entity->semaphore.name.str, O_CREAT | O_EXCL | O_RDWR, 0666, initial_count);
    scratch_end(scratch);

    if (entity->semaphore.named_handle == SEM_FAILED)
    {
      perror("sem_open (alloc)");
      // Arena free for name happens when entity is released
      os_lnx_entity_release(entity);
      return os_handle_zero();
    }
  }
  else
  {
    // Unnamed Semaphore (process-local)
    entity->semaphore.is_named = 0;
    entity->semaphore.name = str8_zero();
    entity->semaphore.named_handle = SEM_FAILED; // Explicitly mark named handle as invalid

    // Initialize the embedded unnamed semaphore (pshared = 0 for process-local)
    int err = sem_init(&entity->semaphore.unnamed_handle, 0, initial_count);
    if (err != 0)
    {
      perror("sem_init");
      os_lnx_entity_release(entity);
      return os_handle_zero();
    }
  }

  OS_Handle result = {(U64)entity};
  return result;
}

internal void
os_semaphore_release(OS_Handle semaphore)
{
  if (os_handle_match(semaphore, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)semaphore.u64[0];
  if (entity == 0 || entity->kind != OS_LNX_EntityKind_Semaphore) { return; }

  if (entity->semaphore.is_named)
  {
    if (entity->semaphore.named_handle != SEM_FAILED)
    {
      sem_close(entity->semaphore.named_handle);
      // NOTE(improvement): Removed sem_unlink. Release should only close the handle
      // for named semaphores, similar to Win32 CloseHandle, allowing other
      // openers to continue using it. Unlinking should be a separate concern
      // if needed (e.g., a dedicated destroy function or manual cleanup).
      // Name string is freed when entity_arena is managed
    }
  }
  else
  {
    // Destroy the embedded unnamed semaphore
    sem_destroy(&entity->semaphore.unnamed_handle);
  }

  os_lnx_entity_release(entity);
}

internal OS_Handle
os_semaphore_open(String8 name)
{
  if (name.size == 0) { return os_handle_zero(); } // Named semaphores must have a name

  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_Semaphore);
  if (!entity)
  {
    return os_handle_zero();
  }

  entity->semaphore.is_named = 1;
  Temp scratch = scratch_begin(0, 0);

  String8 name_copy = name;
  if (name.str[0] != '/')
  {
    name_copy = push_str8f(scratch.arena, "/%S", name);
  }
  entity->semaphore.name = push_str8_copy(os_lnx_state.entity_arena, name_copy);

  // Open existing named semaphore
  entity->semaphore.named_handle = sem_open((char *)entity->semaphore.name.str, O_RDWR);
  scratch_end(scratch);

  if (entity->semaphore.named_handle == SEM_FAILED)
  {
    perror("sem_open (open)");
    // Arena free for name happens when entity is released
    os_lnx_entity_release(entity);
    return os_handle_zero();
  }

  OS_Handle result = {(U64)entity};
  return result;
}

internal void
os_semaphore_close(OS_Handle semaphore)
{
  // This function behaves similarly to release for named semaphores,
  // but doesn't unlink the name or destroy unnamed semaphores.
  // It just closes the handle/connection for this process.
  if (os_handle_match(semaphore, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)semaphore.u64[0];
  if (entity == 0 || entity->kind != OS_LNX_EntityKind_Semaphore) { return; }

  if (entity->semaphore.is_named && entity->semaphore.named_handle != SEM_FAILED)
  {
    sem_close(entity->semaphore.named_handle);
    // Do not unlink here, just close the process's handle to it.
  }
  // For unnamed semaphores, there's no "close" separate from destroy/release.

  // Release the entity tracking structure itself.
  os_lnx_entity_release(entity);
}

internal B32
os_semaphore_take(OS_Handle semaphore, U64 endt_us)
{
  if (os_handle_match(semaphore, os_handle_zero())) { return 0; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)semaphore.u64[0];
  if (entity == 0 || entity->kind != OS_LNX_EntityKind_Semaphore) { return 0; }

  sem_t *sem_handle = entity->semaphore.is_named ? entity->semaphore.named_handle : &entity->semaphore.unnamed_handle;
  if (entity->semaphore.is_named && sem_handle == SEM_FAILED) { return 0; } // Check named handle validity

  int err = 0;

  if (endt_us == max_U64)
  {
    // Infinite wait
    while ((err = sem_wait(sem_handle)) == -1 && errno == EINTR)
    {
      continue; // Restart if interrupted by handler
    }
  }
  else
  {
    // Timed wait using monotonic clock
    struct timespec ts_deadline;
    U64 now_us = os_now_microseconds();

    if (now_us >= endt_us) {
      // Timeout already expired, try once non-blockingly
      err = sem_trywait(sem_handle);
      if (err == -1 && errno == EAGAIN) { // EAGAIN means would block
        errno = ETIMEDOUT; // Report as timeout
      }
    } else {
      // Calculate absolute deadline based on CLOCK_MONOTONIC
      ts_deadline.tv_sec = endt_us / Million(1);
      ts_deadline.tv_nsec = (endt_us % Million(1)) * 1000;

      // Call sem_clockwait with the absolute monotonic deadline
      // Requires _POSIX_TIMEOUTS and _POSIX_MONOTONIC_CLOCK, enabled by _GNU_SOURCE
      while ((err = sem_clockwait(sem_handle, CLOCK_MONOTONIC, &ts_deadline)) == -1 && errno == EINTR)
      {
        continue; // Restart if interrupted by handler
      }
    }
  }

  if (err == 0)
  {
    return 1; // Success
  }
  else
  {
    // ETIMEDOUT is expected on timeout, other errors are reported.
    if (errno != ETIMEDOUT) {
        // Updated error message context
        perror("sem_wait/sem_trywait/sem_clockwait");
    }
    return 0; // Timeout or other error
  }
}

internal void
os_semaphore_drop(OS_Handle semaphore)
{
  if (os_handle_match(semaphore, os_handle_zero())) { return; }
  OS_LNX_Entity *entity = (OS_LNX_Entity *)semaphore.u64[0];
  if (entity == 0 || entity->kind != OS_LNX_EntityKind_Semaphore) { return; }

  sem_t *sem_handle = entity->semaphore.is_named ? entity->semaphore.named_handle : &entity->semaphore.unnamed_handle;
  if (entity->semaphore.is_named && sem_handle == SEM_FAILED) { return; } // Check named handle validity

  int err = 0;

  // sem_post is generally non-blocking unless SEM_VALUE_MAX is reached (on some systems)
  while ((err = sem_post(sem_handle)) == -1 && errno == EINTR)
  {
      continue; // Restart if interrupted by handler
  }

  if (err != 0) {
      // SEM_OVF is the most likely error if max value is exceeded,
      // but other errors are possible.
      perror("sem_post");
  }
}

////////////////////////////////
//~ dan: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

internal OS_Handle
os_library_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  char *path_cstr = (char *)push_str8_copy(scratch.arena, path).str;
  void *so = dlopen(path_cstr, RTLD_LAZY|RTLD_LOCAL);
  OS_Handle lib = { (U64)so };
  scratch_end(scratch);
  return lib;
}

internal VoidProc*
os_library_load_proc(OS_Handle lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  void *so = (void *)lib.u64;
  char *name_cstr = (char *)push_str8_copy(scratch.arena, name).str;
  VoidProc *proc = (VoidProc *)dlsym(so, name_cstr);
  scratch_end(scratch);
  return proc;
}

internal void
os_library_close(OS_Handle lib)
{
  void *so = (void *)lib.u64;
  dlclose(so);
}

////////////////////////////////
//~ dan: @os_hooks Safe Calls (Implemented Per-OS)

internal void
os_safe_call(OS_ThreadFunctionType *func, OS_ThreadFunctionType *fail_handler, void *ptr)
{
  // Allocate an entity for this safe call frame
  OS_LNX_Entity *entity = os_lnx_entity_alloc(OS_LNX_EntityKind_SafeCallChain);
  if (!entity) {
    // Allocation failed, cannot proceed safely. Maybe abort?
    os_abort(-1); 
    return;
  }

  // Store parameters and link to previous chain node
  entity->safe_call_chain.fail_handler = fail_handler;
  entity->safe_call_chain.ptr = ptr;
  entity->safe_call_chain.caller_chain_node = os_lnx_safe_call_chain;
  entity->safe_call_chain.num_signals_handled = 0;

  // Push this frame onto the thread-local chain
  os_lnx_safe_call_chain = entity;
  
  // Save current signal mask
  sigset_t original_mask;
  pthread_sigmask(SIG_BLOCK, NULL, &original_mask);

  // Set up signal handlers for critical errors
  struct sigaction new_action = {0};
  new_action.sa_handler = os_lnx_safe_call_sig_handler; // The handler now just jumps
  sigfillset(&new_action.sa_mask); // Block all signals within the handler
  new_action.sa_flags = SA_NODEFER; // Don't block the signal itself in the handler
  
  int signals_to_handle[] = {
    SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGABRT, SIGTRAP
  };
  const int num_signals = ArrayCount(signals_to_handle);

  for(int i = 0; i < num_signals; ++i)
  {
    int sig = signals_to_handle[i];
    if (sigaction(sig, &new_action, &entity->safe_call_chain.original_actions[i]) == 0)
    {
        entity->safe_call_chain.signals_handled[entity->safe_call_chain.num_signals_handled] = sig;
        entity->safe_call_chain.num_signals_handled++;
    }
    // TODO(rjf): Consider logging failure to set a handler?
  }

  // Use sigsetjmp to save the context. Returns 0 on initial call.
  int jmp_result = sigsetjmp(entity->safe_call_chain.jmp_buf, 1); // Save signal mask
  
  if (jmp_result == 0)
  {
    // Initial call: execute the user function
    func(ptr);
  }
  else
  {
    // Jumped back from signal handler (jmp_result is the signal number)
    // Call the fail handler *after* unwinding from the signal context.
    if(entity->safe_call_chain.fail_handler != 0)
    {
      entity->safe_call_chain.fail_handler(entity->safe_call_chain.ptr);
    }
    else
    {
        // Default behavior if no fail handler: abort?
        fprintf(stderr, "Unhandled fatal signal %d in os_safe_call, aborting.\n", jmp_result);
        os_abort(jmp_result); // Exit with signal number
    }
  }

  // Cleanup: Restore original signal handlers
  for(int i = 0; i < entity->safe_call_chain.num_signals_handled; ++i)
  {
      int sig = entity->safe_call_chain.signals_handled[i];
      sigaction(sig, &entity->safe_call_chain.original_actions[i], NULL);
  }

  // Restore original signal mask
  pthread_sigmask(SIG_SETMASK, &original_mask, NULL);

  // Pop this frame from the thread-local chain
  os_lnx_safe_call_chain = entity->safe_call_chain.caller_chain_node;

  // Release the entity for this frame
  os_lnx_entity_release(entity);
}

////////////////////////////////
//~ dan: @os_hooks GUIDs (Implemented Per-OS)

internal Guid
os_make_guid(void)
{
  Guid guid = {0};
  getrandom(guid.v, sizeof(guid.v), 0);
  guid.data3 &= 0x0fff;
  guid.data3 |= (4 << 12);
  guid.data4[0] &= 0x3f;
  guid.data4[0] |= 0x80;
  return guid;
}

////////////////////////////////
//~ dan: Signal Handler for Safe Calls

internal void
os_lnx_safe_call_sig_handler(int sig)
{
  // This handler should be minimal and async-signal-safe.
  // It jumps back to the corresponding os_safe_call frame.
  if(os_lnx_safe_call_chain != 0)
  {
    // Restore the original handler for this signal *before* longjmping
    // to prevent potential re-entry issues if the signal occurs again
    // immediately after the jump. Find the original action for `sig`.
    OS_LNX_Entity *current_frame = os_lnx_safe_call_chain;
    for(int i = 0; i < current_frame->safe_call_chain.num_signals_handled; ++i)
    {
        if(current_frame->safe_call_chain.signals_handled[i] == sig)
        {
            sigaction(sig, &current_frame->safe_call_chain.original_actions[i], NULL);
            break; // Found and restored
        }
    }
    // Now jump back. Pass the signal number as the return value.
    siglongjmp(current_frame->safe_call_chain.jmp_buf, sig);
  }
  else
  {
    // Should not happen if os_safe_call is used correctly. 
    // If it does, we have no context to jump back to. Abort.
    fprintf(stderr, "Fatal signal %d outside of os_safe_call context! Aborting.\n", sig);
    // Reset to default handler and re-raise? Or just exit?
    signal(sig, SIG_DFL); 
    raise(sig);
    _exit(sig); // Force exit if raise doesn't terminate
  }
}

////////////////////////////////
//~ dan: @os_hooks Entry Points (Implemented Per-OS)

int
main(int argc, char **argv)
{
  //- rjf: set up OS layer
  {
    //- rjf: get statically-allocated system/process info
    {
      OS_SystemInfo *info = &os_lnx_state.system_info;
      info->logical_processor_count = (U32)get_nprocs();
      info->page_size               = (U64)getpagesize();
      info->large_page_size         = MB(2);
      info->allocation_granularity  = info->page_size;
    }
    {
      OS_ProcessInfo *info = &os_lnx_state.process_info;
      info->pid = (U32)getpid();
    }
    
    //- rjf: set up thread context
    local_persist TCTX tctx;
    tctx_init_and_equip(&tctx);
    
    //- rjf: set up dynamically allocated state
    os_lnx_state.arena = arena_alloc();
    os_lnx_state.entity_arena = arena_alloc();
    pthread_mutex_init(&os_lnx_state.entity_mutex, 0);
    
    //- rjf: grab dynamically allocated system info
    {
      Temp scratch = scratch_begin(0, 0);
      OS_SystemInfo *info = &os_lnx_state.system_info;
      
      // get machine name
      B32 got_final_result = 0;
      U8 *buffer = 0;
      int size = 0;
      for(S64 cap = 4096, r = 0; r < 4; cap *= 2, r += 1)
      {
        scratch_end(scratch);
        buffer = push_array_no_zero(scratch.arena, U8, cap);
        size = gethostname((char*)buffer, cap);
        if(size < cap)
        {
          got_final_result = 1;
          break;
        }
      }
      
      // save name to info
      if(got_final_result && size > 0)
      {
        info->machine_name.size = size;
        info->machine_name.str = push_array_no_zero(os_lnx_state.arena, U8, info->machine_name.size + 1);
        MemoryCopy(info->machine_name.str, buffer, info->machine_name.size);
        info->machine_name.str[info->machine_name.size] = 0;
      }
      
      scratch_end(scratch);
    }
    
    //- rjf: grab dynamically allocated process info
    {
      Temp scratch = scratch_begin(0, 0);
      OS_ProcessInfo *info = &os_lnx_state.process_info;
      
      // grab binary path
      {
        // get self string
        B32 got_final_result = 0;
        U8 *buffer = 0;
        int size = 0;
        for(S64 cap = PATH_MAX, r = 0; r < 4; cap *= 2, r += 1)
        {
          scratch_end(scratch);
          buffer = push_array_no_zero(scratch.arena, U8, cap);
          size = readlink("/proc/self/exe", (char*)buffer, cap);
          if(size < cap)
          {
            got_final_result = 1;
            break;
          }
        }
        
        // save
        if(got_final_result && size > 0)
        {
          String8 full_name = str8(buffer, size);
          String8 name_chopped = str8_chop_last_slash(full_name);
          info->binary_path = push_str8_copy(os_lnx_state.arena, name_chopped);
        }
      }
      
      // grab initial directory
      {
        info->initial_path = os_get_current_path(os_lnx_state.arena);
      }
      
      // grab home directory
      {
        char *home = getenv("HOME");
        info->user_program_data_path = str8_cstring(home);
      }
      
      // grab environment variables
      {
        extern char **environ;
        if (environ != 0)
        {
          for (char **env_ptr = environ; *env_ptr != 0; env_ptr += 1)
          {
            String8 env_var = str8_cstring(*env_ptr);
            str8_list_push(os_lnx_state.arena, &info->environment, env_var);
          }
        }
      }
      
      scratch_end(scratch);
    }
  }
  
  //- rjf: call into "real" entry point
  main_thread_base_entry_point(argc, argv);
}

