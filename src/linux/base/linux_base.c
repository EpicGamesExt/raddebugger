// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal DateTime
lnx_date_time_from_tm(tm in, U32 msec)
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
lnx_tm_from_date_time(DateTime dt)
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
lnx_timespec_from_date_time(DateTime dt)
{
  tm tm_val = lnx_tm_from_date_time(dt);
  time_t seconds = timegm(&tm_val);
  timespec result = {0};
  result.tv_sec = seconds;
  return result;
}

internal DenseTime
lnx_dense_time_from_timespec(timespec in)
{
  DenseTime result = 0;
  {
    struct tm tm_time = {0};
    gmtime_r(&in.tv_sec, &tm_time);
    DateTime date_time = lnx_date_time_from_tm(tm_time, in.tv_nsec/Million(1));
    result = dense_time_from_date_time(date_time);
  }
  return result;
}

internal FileProperties
lnx_file_properties_from_stat(struct stat *s)
{
  FileProperties props = {0};
  props.size     = s->st_size;
  props.created  = lnx_dense_time_from_timespec(s->st_ctim);
  props.modified = lnx_dense_time_from_timespec(s->st_mtim);
  if(s->st_mode & S_IFDIR)
  {
    props.flags |= FilePropertyFlag_IsFolder;
  }
  return props;
}

internal void
lnx_safe_call_sig_handler(int x)
{
  LNX_SafeCallChain *chain = lnx_safe_call_chain;
  if(chain != 0 && chain->fail_handler != 0)
  {
    chain->fail_handler(chain->ptr);
  }
  abort();
}

////////////////////////////////
//~ rjf: Entities

internal LNX_Entity *
lnx_entity_alloc(LNX_EntityKind kind)
{
  LNX_Entity *entity = 0;
  DeferLoop(pthread_mutex_lock(&lnx_state.entity_mutex),
            pthread_mutex_unlock(&lnx_state.entity_mutex))
  {
    entity = lnx_state.entity_free;
    if(entity)
    {
      SLLStackPop(lnx_state.entity_free);
    }
    else
    {
      entity = push_array_no_zero(lnx_state.entity_arena, LNX_Entity, 1);
    }
  }
  MemoryZeroStruct(entity);
  entity->kind = kind;
  return entity;
}

internal void
lnx_entity_release(LNX_Entity *entity)
{
  DeferLoop(pthread_mutex_lock(&lnx_state.entity_mutex),
            pthread_mutex_unlock(&lnx_state.entity_mutex))
  {
    SLLStackPush(lnx_state.entity_free, entity);
  }
}

////////////////////////////////
//~ rjf: Thread Entry Point

internal void *
lnx_thread_entry_point(void *ptr)
{
  LNX_Entity *entity = (LNX_Entity *)ptr;
  ThreadEntryPointFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  supplement_thread_base_entry_point(func, thread_ptr);
  return 0;
}

////////////////////////////////
//~ rjf: @per_os_impl Debugger Attachment Checking

internal B32
debugger_is_attached(void)
{
  B32 result = 0;
  // TODO(rjf)
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Platform Time Functions

internal U64
now_time_us(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  U64 result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
  return result;
}

internal U32
now_time_unix(void)
{
  time_t t = time(0);
  return (U32)t;
}

internal DateTime
now_time_universal(void)
{
  time_t t = 0;
  time(&t);
  struct tm universal_tm = {0};
  gmtime_r(&t, &universal_tm);
  DateTime result = lnx_date_time_from_tm(universal_tm, 0);
  return result;
}

internal DateTime
universal_from_local_time(DateTime *dt)
{
  // rjf: local DateTime -> universal time_t
  tm local_tm = lnx_tm_from_date_time(*dt);
  local_tm.tm_isdst = -1;
  time_t universal_t = mktime(&local_tm);
  
  // rjf: universal time_t -> DateTime
  tm universal_tm = {0};
  gmtime_r(&universal_t, &universal_tm);
  DateTime result = lnx_date_time_from_tm(universal_tm, 0);
  return result;
}

internal DateTime
local_from_universal_time(DateTime *dt)
{
  // rjf: universal DateTime -> local time_t
  tm universal_tm = lnx_tm_from_date_time(*dt);
  universal_tm.tm_isdst = -1;
  time_t universal_t = timegm(&universal_tm);
  tm local_tm = {0};
  localtime_r(&universal_t, &local_tm);
  
  // rjf: local tm -> DateTime
  DateTime result = lnx_date_time_from_tm(local_tm, 0);
  return result;
}

internal void
sleep_ms(U32 ms)
{
  usleep(ms*Thousand(1));
}

////////////////////////////////
//~ rjf: @per_os_impl Platform GUID Functions

internal Guid
make_guid(void)
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
//~ rjf: @per_os_impl Platform Memory Allocation

//- rjf: basic

internal void *
reserve_memory(U64 size)
{
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(result == MAP_FAILED)
  {
    result = 0;
  }
  return result;
}

internal B32
commit_memory(void *ptr, U64 size)
{
  mprotect(ptr, size, PROT_READ|PROT_WRITE);
  return 1;
}

internal void
decommit_memory(void *ptr, U64 size)
{
  madvise(ptr, size, MADV_DONTNEED);
  mprotect(ptr, size, PROT_NONE);
}

internal void
release_memory(void *ptr, U64 size)
{
  munmap(ptr, size);
}

//- rjf: large pages

internal void *
reserve_memory_large(U64 size)
{
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
  if(result == MAP_FAILED)
  {
    result = 0;
  }
  return result;
}

internal B32
commit_memory_large(void *ptr, U64 size)
{
  mprotect(ptr, size, PROT_READ|PROT_WRITE);
  return 1;
}

////////////////////////////////
//~ rjf: @os_hooks Shared Memory

internal SharedMemory
shared_memory_alloc(U64 size, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = push_str8_copy(scratch.arena, name);
  int id = shm_open((char *)name_copy.str, O_RDWR|O_CREAT, 0666);
  ftruncate(id, size);
  SharedMemory result = {(U64)id};
  scratch_end(scratch);
  return result;
}

internal SharedMemory
shared_memory_open(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = push_str8_copy(scratch.arena, name);
  int id = shm_open((char *)name_copy.str, O_RDWR, 0);
  SharedMemory result = {(U64)id};
  scratch_end(scratch);
  return result;
}

internal void
shared_memory_close(SharedMemory handle)
{
  if(MemoryIsZeroStruct(&handle)){return;}
  int id = (int)handle.u64[0];
  close(id);
}

internal void *
shared_memory_view_open(SharedMemory handle, Rng1U64 range)
{
  if(MemoryIsZeroStruct(&handle)){return 0;}
  int id = (int)handle.u64[0];
  void *base = mmap(0, dim_1u64(range), PROT_READ|PROT_WRITE, MAP_SHARED, id, range.min);
  if(base == MAP_FAILED)
  {
    base = 0;
  }
  return base;
}

internal void
shared_memory_view_close(SharedMemory handle, void *ptr, Rng1U64 range)
{
  if(MemoryIsZeroStruct(&handle)){return;}
  munmap(ptr, dim_1u64(range));
}

////////////////////////////////
//~ rjf: @per_os_impl System Info

internal SystemInfo *
get_system_info(void)
{
  return &lnx_state.system_info;
}

////////////////////////////////
//~ rjf: @per_os_impl Current Thread Info

internal U32
tid(void)
{
  U32 result = gettid();
  return result;
}

internal void
set_platform_thread_name(String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  String8 name_copy = str8_copy(scratch.arena, name);
  pthread_t current_thread = pthread_self();
  pthread_setname_np(current_thread, (char *)name_copy.str);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @per_os_impl Thread Functions

internal Thread
thread_launch(ThreadEntryPointFunctionType *f, void *p)
{
  LNX_Entity *entity = lnx_entity_alloc(LNX_EntityKind_Thread);
  entity->thread.func = f;
  entity->thread.ptr = p;
  {
    int pthread_result = pthread_create(&entity->thread.handle, 0, lnx_thread_entry_point, entity);
    if(pthread_result == -1)
    {
      lnx_entity_release(entity);
      entity = 0;
    }
  }
  Thread handle = {(U64)entity};
  return handle;
}

internal B32
thread_join(Thread thread, U64 endt_us)
{
  if(MemoryIsZeroStruct(&thread)) { return 0; }
  LNX_Entity *entity = (LNX_Entity *)thread.u64[0];
  int join_result = pthread_join(entity->thread.handle, 0);
  B32 result = (join_result == 0);
  lnx_entity_release(entity);
  return result;
}

internal void
thread_detach(Thread thread)
{
  if(MemoryIsZeroStruct(&thread)) { return; }
  LNX_Entity *entity = (LNX_Entity *)thread.u64[0];
  lnx_entity_release(entity);
}

////////////////////////////////
//~ rjf: @per_os_impl Synchronization Primitive Functions

//- rjf: recursive mutexes

internal Mutex
mutex_alloc(void)
{
  LNX_Entity *entity = lnx_entity_alloc(LNX_EntityKind_Mutex);
  int init_result = pthread_mutex_init(&entity->mutex_handle, 0);
  if(init_result == -1)
  {
    lnx_entity_release(entity);
    entity = 0;
  }
  Mutex handle = {(U64)entity};
  return handle;
}

internal void
mutex_release(Mutex mutex)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  pthread_mutex_destroy(&entity->mutex_handle);
  lnx_entity_release(entity);
}

internal void
mutex_take(Mutex mutex)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  pthread_mutex_lock(&entity->mutex_handle);
}

internal void
mutex_drop(Mutex mutex)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  pthread_mutex_unlock(&entity->mutex_handle);
}

//- rjf: reader/writer mutexes

internal RWMutex
rw_mutex_alloc(void)
{
  LNX_Entity *entity = lnx_entity_alloc(LNX_EntityKind_RWMutex);
  int init_result = pthread_rwlock_init(&entity->rwmutex_handle, 0);
  if(init_result == -1)
  {
    lnx_entity_release(entity);
    entity = 0;
  }
  RWMutex handle = {(U64)entity};
  return handle;
}

internal void
rw_mutex_release(RWMutex mutex)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  pthread_rwlock_destroy(&entity->rwmutex_handle);
  lnx_entity_release(entity);
}

internal void
rw_mutex_take(RWMutex mutex, B32 write_mode)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  if(write_mode)
  {
    pthread_rwlock_wrlock(&entity->rwmutex_handle);
  }
  else
  {
    pthread_rwlock_rdlock(&entity->rwmutex_handle);
  }
}

internal void
rw_mutex_drop(RWMutex mutex, B32 write_mode)
{
  if(MemoryIsZeroStruct(&mutex)) { return; }
  LNX_Entity *entity = (LNX_Entity *)mutex.u64[0];
  pthread_rwlock_unlock(&entity->rwmutex_handle);
}

//- rjf: condition variables

internal CondVar
cond_var_alloc(void)
{
  LNX_Entity *entity = lnx_entity_alloc(LNX_EntityKind_ConditionVariable);
  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  int init_result = pthread_cond_init(&entity->cv.cond_handle, &attr);
  pthread_condattr_destroy(&attr);
  if(init_result == -1)
  {
    lnx_entity_release(entity);
    entity = 0;
  }
  int init2_result = 0;
  if(entity)
  {
    init2_result = pthread_mutex_init(&entity->cv.rwlock_mutex_handle, 0);
  }
  if(init2_result == -1)
  {
    pthread_cond_destroy(&entity->cv.cond_handle);
    lnx_entity_release(entity);
    entity = 0;
  }
  CondVar handle = {(U64)entity};
  return handle;
}

internal void
cond_var_release(CondVar cv)
{
  if(MemoryIsZeroStruct(&cv)) { return; }
  LNX_Entity *entity = (LNX_Entity *)cv.u64[0];
  pthread_cond_destroy(&entity->cv.cond_handle);
  pthread_mutex_destroy(&entity->cv.rwlock_mutex_handle);
  lnx_entity_release(entity);
}

internal B32
cond_var_wait(CondVar cv, Mutex mutex, U64 endt_us)
{
  if(MemoryIsZeroStruct(&cv)) { return 0; }
  if(MemoryIsZeroStruct(&mutex)) { return 0; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  LNX_Entity *mutex_entity = (LNX_Entity *)mutex.u64[0];
  struct timespec endt_timespec;
  endt_timespec.tv_sec = endt_us/Million(1);
  endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us/Million(1))*Million(1));
  int wait_result = pthread_cond_timedwait(&cv_entity->cv.cond_handle, &mutex_entity->mutex_handle, &endt_timespec);
  B32 result = (wait_result != ETIMEDOUT);
  return result;
}

internal B32
cond_var_wait_rw(CondVar cv, RWMutex mutex_rw, B32 write_mode, U64 endt_us)
{
  // TODO(rjf): because pthread does not supply cv/rw natively, I had to hack
  // this together, but this would probably just be a lot better if we just
  // implemented the primitives ourselves with e.g. futexes
  //
  if(MemoryIsZeroStruct(&cv)) { return 0; }
  if(MemoryIsZeroStruct(&mutex_rw)) { return 0; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  LNX_Entity *rw_mutex_entity = (LNX_Entity *)mutex_rw.u64[0];
  struct timespec endt_timespec;
  endt_timespec.tv_sec = endt_us/Million(1);
  endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us/Million(1))*Million(1));
  B32 result = 0;
  pthread_mutex_lock(&cv_entity->cv.rwlock_mutex_handle);
  pthread_rwlock_unlock(&rw_mutex_entity->rwmutex_handle);
  for(;;)
  {
    int wait_result = pthread_cond_timedwait(&cv_entity->cv.cond_handle, &cv_entity->cv.rwlock_mutex_handle, &endt_timespec);
    if(wait_result != ETIMEDOUT)
    {
      if(write_mode)
      {
        pthread_rwlock_wrlock(&rw_mutex_entity->rwmutex_handle);
      }
      else
      {
        pthread_rwlock_rdlock(&rw_mutex_entity->rwmutex_handle);
      }
      result = 1;
      break;
    }
    if(wait_result == ETIMEDOUT)
    {
      if(write_mode)
      {
        pthread_rwlock_wrlock(&rw_mutex_entity->rwmutex_handle);
      }
      else
      {
        pthread_rwlock_rdlock(&rw_mutex_entity->rwmutex_handle);
      }
      break;
    }
  }
  pthread_mutex_unlock(&cv_entity->cv.rwlock_mutex_handle);
  return result;
}

internal void
cond_var_signal(CondVar cv)
{
  if(MemoryIsZeroStruct(&cv)) { return; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  pthread_cond_signal(&cv_entity->cv.cond_handle);
}

internal void
cond_var_broadcast(CondVar cv)
{
  if(MemoryIsZeroStruct(&cv)) { return; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  pthread_cond_broadcast(&cv_entity->cv.cond_handle);
}

//- rjf: cross-process semaphores

internal Semaphore
semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  Semaphore result = {0};
  if(name.size > 0)
  {
    for EachIndex(attempt_idx, 64)
    {
      String8 name_copy = str8_copy(scratch.arena, name);
      sem_t *s = sem_open((char *)name_copy.str, O_CREAT | O_EXCL, 0666, initial_count);
      if(s == SEM_FAILED)
      {
        s = sem_open((char *)name_copy.str, 0);
      }
      if(s != SEM_FAILED)
      {
        result.u64[0] = (U64)s;
        break;
      }
    }
  }
  else
  {
    sem_t *s = mmap(0, sizeof(*s), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    AssertAlways(s != MAP_FAILED);
    int err = sem_init(s, 0, initial_count);
    if(err == 0)
    {
      result.u64[0] = (U64)s;
    }
  }
  scratch_end(scratch);
  return result;
}

internal void
semaphore_release(Semaphore semaphore)
{
  int err = munmap((void*)semaphore.u64[0], sizeof(sem_t));
  AssertAlways(err == 0);
}

internal Semaphore
semaphore_open(String8 name)
{
  Semaphore result = {0};
  {
    Temp scratch = scratch_begin(0, 0);
    String8 name_copy = str8_copy(scratch.arena, name);
    sem_t *s = sem_open((char *)name_copy.str, 0);
    if(s != SEM_FAILED)
    {
      result.u64[0] = (U64)s;
    }
    scratch_end(scratch);
  }
  return result;
}

internal void
semaphore_close(Semaphore semaphore)
{
  if(semaphore.u64[0] != 0)
  {
    sem_t *s = (sem_t *)semaphore.u64[0];
    sem_close(s);
  }
}

internal B32
semaphore_take(Semaphore semaphore, U64 endt_us)
{
  int err = -1;
  if(semaphore.u64[0] != 0)
  {
    struct timespec t = { .tv_sec = endt_us / 1000000, .tv_nsec = (endt_us % 1000000) * 1000 };
    err = LNX_RETRY_ON_EINTR(sem_clockwait((sem_t*)semaphore.u64[0], CLOCK_MONOTONIC, &t));
  }
  return err == 0;
}

internal void
semaphore_drop(Semaphore semaphore)
{
  int err = -1;
  if(semaphore.u64[0] != 0)
  {
    err = LNX_RETRY_ON_EINTR(sem_post((sem_t*)semaphore.u64[0]));
    Assert(err == 0);
  }
}

//- rjf: barriers

internal Barrier
barrier_alloc(U64 count)
{
  LNX_Entity *entity = lnx_entity_alloc(LNX_EntityKind_Barrier);
  if(entity != 0)
  {
    pthread_barrier_init(&entity->barrier, 0, count);
  }
  Barrier result = {IntFromPtr(entity)};
  return result;
}

internal void
barrier_release(Barrier barrier)
{
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(barrier.u64[0]);
  if(entity != 0)
  {
    pthread_barrier_destroy(&entity->barrier);
    lnx_entity_release(entity);
  }
}

internal void
barrier_wait(Barrier barrier)
{
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(barrier.u64[0]);
  if(entity != 0)
  {
    pthread_barrier_wait(&entity->barrier);
  }
}

////////////////////////////////
//~ rjf: @per_os_impl Safe Calls

internal void
safe_call(ThreadEntryPointFunctionType *func, ThreadEntryPointFunctionType *fail_handler, void *ptr)
{
  // rjf: push handler to chain
  LNX_SafeCallChain chain = {0};
  SLLStackPush(lnx_safe_call_chain, &chain);
  chain.fail_handler = fail_handler;
  chain.ptr = ptr;
  
  // rjf: set up sig handler info
  struct sigaction new_act = {0};
  new_act.sa_handler = lnx_safe_call_sig_handler;
  int signals_to_handle[] =
  {
    SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP,
  };
  struct sigaction og_act[ArrayCount(signals_to_handle)] = {0};
  
  // rjf: attach handler info for all signals
  for(U32 i = 0; i < ArrayCount(signals_to_handle); i += 1)
  {
    sigaction(signals_to_handle[i], &new_act, &og_act[i]);
  }
  
  // rjf: call function
  func(ptr);
  
  // rjf: reset handler info for all signals
  for(U32 i = 0; i < ArrayCount(signals_to_handle); i += 1)
  {
    sigaction(signals_to_handle[i], &og_act[i], 0);
  }
}

////////////////////////////////
//~ rjf: @per_os_impl File System (Implemented Per-OS)

//- rjf: files

internal File
file_open(AccessFlags flags, String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  int lnx_flags = 0;
  if(flags & AccessFlag_Read && flags & AccessFlag_Write)
  {
    lnx_flags = O_RDWR;
  }
  else if(flags & AccessFlag_Write)
  {
    lnx_flags = O_WRONLY;
  }
  else if(flags & AccessFlag_Read)
  {
    lnx_flags = O_RDONLY;
  }
  if(flags & AccessFlag_Append)
  {
    lnx_flags |= O_APPEND;
  }
  if(flags & (AccessFlag_Write|AccessFlag_Append))
  {
    lnx_flags |= O_CREAT;
  }
  lnx_flags |= O_CLOEXEC;
  int fd = open((char *)path_copy.str, lnx_flags, 0755);
  File handle = {0};
  if(fd != -1)
  {
    handle.u64[0] = fd;
  }
  scratch_end(scratch);
  return handle;
}

internal void
file_close(File file)
{
  if(file_match(file, file_zero())) { return; }
  int fd = (int)file.u64[0];
  close(fd);
}

internal U64
file_read(File file, Rng1U64 rng, void *out_data)
{
  if(file_match(file, file_zero())) { return 0; }
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
file_write(File file, Rng1U64 rng, void *data)
{
  if(file_match(file, file_zero())) { return 0; }
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
file_set_times(File file, DateTime date_time)
{
  if(file_match(file, file_zero())) { return 0; }
  int fd = (int)file.u64[0];
  timespec time = lnx_timespec_from_date_time(date_time);
  timespec times[2] = {time, time};
  int futimens_result = futimens(fd, times);
  B32 good = (futimens_result != -1);
  return good;
}

internal FileProperties
properties_from_file(File file)
{
  if(file_match(file, file_zero())) { return (FileProperties){0}; }
  int fd = (int)file.u64[0];
  struct stat fd_stat = {0};
  int fstat_result = fstat(fd, &fd_stat);
  FileProperties props = {0};
  if(fstat_result != -1)
  {
    props = lnx_file_properties_from_stat(&fd_stat);
  }
  return props;
}

internal FileID
id_from_file(File file)
{
  if(file_match(file, file_zero())) { return (FileID){0}; }
  int fd = (int)file.u64[0];
  struct stat fd_stat = {0};
  int fstat_result = fstat(fd, &fd_stat);
  FileID id = {0};
  if(fstat_result != -1)
  {
    id.v[0] = fd_stat.st_dev;
    id.v[1] = fd_stat.st_ino;
  }
  return id;
}

internal B32
file_reserve_size(File file, U64 size)
{
  int status = fallocate((int)file.u64[0], FALLOC_FL_KEEP_SIZE, 0, size);
  return status == 0;
}

internal B32
delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  String8 path_copy = push_str8_copy(scratch.arena, path);
  if(remove((char *)path_copy.str) != -1)
  {
    result = 1;
  }
  scratch_end(scratch);
  return result;
}

internal B32
copy_file_path(String8 dst, String8 src)
{
  B32 result = 0;
  File src_h = file_open(AccessFlag_Read, src);
  File dst_h = file_open(AccessFlag_Write, dst);
  if(!file_match(src_h, file_zero()) &&
     !file_match(dst_h, file_zero()))
  {
    int src_fd = (int)src_h.u64[0];
    int dst_fd = (int)dst_h.u64[0];
    FileProperties src_props = properties_from_file(src_h);
    U64 size = src_props.size;
    U64 total_bytes_copied = 0;
    U64 bytes_left_to_copy = size;
    for(;bytes_left_to_copy > 0;)
    {
      off_t sendfile_off = total_bytes_copied;
      int send_result = sendfile(dst_fd, src_fd, &sendfile_off, bytes_left_to_copy);
      if(send_result <= 0)
      {
        break;
      }
      U64 bytes_copied = (U64)send_result;
      bytes_left_to_copy -= bytes_copied;
      total_bytes_copied += bytes_copied;
    }
  }
  file_close(src_h);
  file_close(dst_h);
  return result;
}

internal B32
move_file_path(String8 dst, String8 src)
{
  B32 good = 0;
  Temp scratch = scratch_begin(0, 0);
  {
    char *src_cstr = (char *)str8_copy(scratch.arena, src).str;
    char *dst_cstr = (char *)str8_copy(scratch.arena, dst).str;
    int rename_result = rename(src_cstr, dst_cstr);
    good = (rename_result != -1);
  }
  scratch_end(scratch);
  return good;
}

internal String8
full_path_from_path(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 path_copy = str8_copy(scratch.arena, path);
  char buffer[PATH_MAX] = {0};
  realpath((char *)path_copy.str, buffer);
  String8 result = str8_copy(arena, str8_cstring(buffer));
  scratch_end(scratch);
  return result;
}

internal B32
file_path_exists(String8 path)
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
folder_path_exists(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 exists = 0;
  String8  path_copy = str8_copy(scratch.arena, path);
  DIR *handle = opendir((char *)path_copy.str);
  if(handle)
  {
    closedir(handle);
    exists = 1;
  }
  scratch_end(scratch);
  return exists;
}

internal FileProperties
properties_from_file_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = str8_copy(scratch.arena, path);
  struct stat f_stat = {0};
  int stat_result = stat((char *)path_copy.str, &f_stat);
  FileProperties props = {0};
  if(stat_result != -1)
  {
    props = lnx_file_properties_from_stat(&f_stat);
  }
  scratch_end(scratch);
  return props;
}

//- rjf: file maps

internal FileMap
file_map_open(AccessFlags flags, File file)
{
  FileMap map = {file.u64[0]};
  return map;
}

internal void
file_map_close(FileMap map)
{
  // NOTE(rjf): nothing to do; `map` handles are the same as `file` handles in
  // the linux implementation (on Windows they require separate handles)
}

internal void *
file_map_view_open(FileMap map, AccessFlags flags, Rng1U64 range)
{
  if(MemoryIsZeroStruct(&map)) { return 0; }
  int fd = (int)map.u64[0];
  int prot_flags = 0;
  if(flags & AccessFlag_Write) { prot_flags |= PROT_WRITE; }
  if(flags & AccessFlag_Read)  { prot_flags |= PROT_READ; }
  int map_flags = MAP_PRIVATE;
  void *base = mmap(0, dim_1u64(range), prot_flags, map_flags, fd, range.min);
  if(base == MAP_FAILED)
  {
    base = 0;
  }
  return base;
}

internal void
file_map_view_close(FileMap map, void *ptr, Rng1U64 range)
{
  munmap(ptr, dim_1u64(range));
}

//- rjf: directory iteration

internal FileIter *
file_iter_begin(Arena *arena, String8 path, FileIterFlags flags)
{
  FileIter *base_iter = push_array(arena, FileIter, 1);
  base_iter->flags = flags;
  LNX_FileIter *iter = (LNX_FileIter *)base_iter->memory;
  {
    String8 path_copy = push_str8_copy(arena, path);
    iter->dir = opendir((char *)path_copy.str);
    iter->path = path_copy;
  }
  return base_iter;
}

internal B32
file_iter_next(Arena *arena, FileIter *iter, FileInfo *info_out)
{
  B32 good = 0;
  LNX_FileIter *lnx_iter = (LNX_FileIter *)iter->memory;
  for(;lnx_iter->dir != 0;)
  {
    // rjf: get next entry
    lnx_iter->dp = readdir(lnx_iter->dir);
    good = (lnx_iter->dp != 0);
    
    // rjf: unpack entry info
    struct stat st = {0};
    int stat_result = 0;
    if(good)
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8 full_path = push_str8f(scratch.arena, "%S/%s", lnx_iter->path, lnx_iter->dp->d_name);
      stat_result = stat((char *)full_path.str, &st);
      scratch_end(scratch);
    }
    
    // rjf: determine if filtered
    B32 filtered = 0;
    if(good)
    {
      filtered = ((st.st_mode == S_IFDIR && iter->flags & FileIterFlag_SkipFolders) ||
                  (st.st_mode == S_IFREG && iter->flags & FileIterFlag_SkipFiles) ||
                  (lnx_iter->dp->d_name[0] == '.' && lnx_iter->dp->d_name[1] == 0) ||
                  (lnx_iter->dp->d_name[0] == '.' && lnx_iter->dp->d_name[1] == '.' && lnx_iter->dp->d_name[2] == 0));
    }
    
    // rjf: output & exit, if good & unfiltered
    if(good && !filtered)
    {
      info_out->name = push_str8_copy(arena, str8_cstring(lnx_iter->dp->d_name));
      if(stat_result != -1)
      {
        info_out->props = lnx_file_properties_from_stat(&st);
      }
      break;
    }
    
    // rjf: exit if not good
    if(!good)
    {
      break;
    }
  }
  return good;
}

internal void
file_iter_end(FileIter *iter)
{
  LNX_FileIter *lnx_iter = (LNX_FileIter *)iter->memory;
  closedir(lnx_iter->dir);
}

//- rjf: directory creation

internal B32
make_directory(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  String8 path_copy = push_str8_copy(scratch.arena, path);
  if(mkdir((char *)path_copy.str, 0755) != -1)
  {
    result = 1;
  }
  else
  {
    // match windows behavior
    result = file_path_exists(path);
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Aborting

internal void
abort_self(S32 exit_code)
{
  exit(exit_code);
}

////////////////////////////////
//~ rjf: @per_os_impl Process Info

internal ProcessInfo *
get_process_info(void)
{
  return &lnx_state.process_info;
}

internal String8
get_current_path(Arena *arena)
{
  char *cwdir = getcwd(0, 0);
  String8 string = str8_copy(arena, str8_cstring(cwdir));
  free(cwdir);
  return string;
}

internal U32
get_process_start_time_unix(void)
{
  Temp scratch = scratch_begin(0,0);
  U64 start_time = 0;
  pid_t pid = getpid();
  String8 path = str8f(scratch.arena, "/proc/%u", pid);
  struct stat st;
  int err = stat((char *)path.str, &st);
  if(err == 0)
  {
    start_time = st.st_mtime;
  }
  scratch_end(scratch);
  return (U32)start_time;
}

////////////////////////////////
//~ rjf: @per_os_impl Child Processes

internal Process
process_launch(ProcessLaunchParams *params)
{
  Process handle = {0};
  posix_spawn_file_actions_t file_actions = {0};
  int file_actions_init_code = posix_spawn_file_actions_init(&file_actions);
  if(file_actions_init_code == 0)
  {
    Temp scratch = scratch_begin(0, 0);
    if(params->path.size != 0)
    {
      int chdir_code = posix_spawn_file_actions_addchdir_np(&file_actions, (char *)push_cstr(scratch.arena, params->path).str);
      Assert(chdir_code == 0);
    }
    int stdout_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stdout_file.u64[0], STDOUT_FILENO);
    int stderr_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stderr_file.u64[0], STDERR_FILENO);
    int stdin_code = posix_spawn_file_actions_adddup2(&file_actions, (int)params->stdin_file.u64[0], STDIN_FILENO);
    posix_spawnattr_t attr = {0};
    int attr_init_code = posix_spawnattr_init(&attr);
    if(attr_init_code == 0)
    {
      // package argv
      char **argv = push_array(scratch.arena, char *, params->cmd_line.node_count + 1);
      {
        argv[0] = (char *)push_cstr(scratch.arena, params->cmd_line.first->string).str;
        U64 arg_idx = 1;
        for EachNode(n, String8Node, params->cmd_line.first->next)
        {
          argv[arg_idx] = (char *)push_cstr(scratch.arena, n->string).str;
          arg_idx += 1;
        }
      }
      
      // package envp
      char **envp = 0;
      if(params->inherit_env)
      {
        envp = lnx_state.default_env;
      }
      else
      {
        envp = push_array(scratch.arena, char *, params->env.node_count + 2);
        U64 env_idx = 0;
        for EachNode(n, String8Node, params->cmd_line.first)
        {
          envp[env_idx] = (char *)n->string.str;
          env_idx += 1;
        }
      }
      
      // spawn process
      pid_t pid = 0;
      int spawn_code = posix_spawnp(&pid, argv[0], &file_actions, &attr, argv, envp);
      if(spawn_code == 0)
      {
        handle.u64[0] = (U64)pid;
      }
      
      // clean up attributes
      int attr_destroy_code = posix_spawnattr_destroy(&attr);
    }
    scratch_end(scratch);
    
    // clean up file actions
    int file_actions_destroy_code = posix_spawn_file_actions_destroy(&file_actions);
  }
  return handle;
}

internal B32
process_join(Process process, U64 endt_us, U64 *exit_code_out)
{
  B32 result = 0;
  
  pid_t pid = (pid_t)process.u64[0];
  for(;;)
  {
    int status = 0;
    pid_t wait_result = LNX_RETRY_ON_EINTR(waitpid(pid, &status, (endt_us == max_U64) ? 0 : WNOHANG));
    
    if((wait_result == pid) && (WIFEXITED(status) || WIFSIGNALED(status)))
    {
      result = 1;
      if(exit_code_out != 0)
      {
        if     (WIFEXITED(status))   { *exit_code_out = WEXITSTATUS(status); }
        else if(WIFSIGNALED(status)) { *exit_code_out = WTERMSIG(status) + 128; }
      }
      break;
    }
    
    if(wait_result == -1) { break; }
    if(endt_us == 0)      { break; }
    
    U64 now_us = now_time_us();
    if(now_us >= endt_us) { break; }
    
    U64 left_us  = endt_us - now_us;
    U64 sleep_us = Min(left_us, Thousand(1));
    usleep((useconds_t)sleep_us);
  }
  return result;
}

internal void
process_detach(Process process)
{
  // no need to close pid
}

internal B32
process_kill(Process process)
{
  int error_code = kill((pid_t)process.u64[0], SIGKILL);
  B32 is_killed = error_code == 0;
  return is_killed;
}

////////////////////////////////
//~ rjf: @per_os_impl Dynamically-Loaded Libraries

internal Library
library_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  char *path_cstr = (char *)str8_copy(scratch.arena, path).str;
  void *so = dlopen(path_cstr, RTLD_LAZY|RTLD_LOCAL);
  Library lib = { (U64)so };
  scratch_end(scratch);
  return lib;
}

internal void
library_close(Library lib)
{
  void *so = (void *)lib.u64;
  dlclose(so);
}

internal VoidProc *
library_load_proc(Library lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  void *so = (void *)lib.u64;
  char *name_cstr = (char *)str8_copy(scratch.arena, name).str;
  VoidProc *proc = (VoidProc *)dlsym(so, name_cstr);
  scratch_end(scratch);
  return proc;
}

////////////////////////////////
//~ rjf: Entry Point

internal void
lnx_signal_handler(int sig, siginfo_t *info, void *arg)
{
  local_persist volatile U32 first = 0;
  if (ins_atomic_u32_eval_cond_assign(&first, 1, 0) != 0)
  {
    for(;;)
    {
      sleep(UINT32_MAX);
    }
  }
  
  local_persist void *ips[4096];
  int ips_count = backtrace(ips, ArrayCount(ips));
  
  fprintf(stderr, "A fatal signal was received: %s (%d). The process is terminating.\n", strsignal(sig), sig);
  fprintf(stderr, "Create a new issue with this report at %s.\n\n", BUILD_ISSUES_LINK_STRING_LITERAL);
  fprintf(stderr, "Callstack:\n");
  for EachIndex(i, ips_count)
  {
    Dl_info info = {0};
    dladdr(ips[i], &info);
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "llvm-symbolizer --relative-address -f -e %s %lu", info.dli_fname, (unsigned long)ips[i] - (unsigned long)info.dli_fbase);
    FILE *f = popen(cmd, "r");
    if(f)
    {
      char func_name[256], file_name[256];
      if(fgets(func_name, sizeof(func_name), f) && fgets(file_name, sizeof(file_name), f))
      {
        String8 func = str8_cstring(func_name);
        if(func.size > 0) func.size -= 1;
        String8 module = str8_skip_last_slash(str8_cstring(info.dli_fname));
        String8 file   = str8_skip_last_slash(str8_cstring_capped(file_name, file_name + sizeof(file_name)));
        if(file.size > 0) file.size -= 1;
        
        B32 no_func = str8_match(func, str8_lit("??"), StringMatchFlag_RightSideSloppy);
        B32 no_file = str8_match(file, str8_lit("??"), StringMatchFlag_RightSideSloppy);
        if(no_func) { func = str8_zero(); }
        if(no_file) { file = str8_zero(); }
        
        fprintf(stderr, "%ld. [0x%016lx] %.*s%s%.*s %.*s\n", i+1, (unsigned long)ips[i], (int)module.size, module.str, (!no_func || !no_file) ? ", " : "", (int)func.size, func.str, (int)file.size, file.str);
      }
      pclose(f);
    }
    else
    {
      fprintf(stderr, "%ld. [0x%016lx] %s\n", i+1, (unsigned long)ips[i], info.dli_fname);
    }
  }
  fprintf(stderr, "\nVersion: %s%s\n\n", BUILD_VERSION_STRING_LITERAL, BUILD_GIT_HASH_STRING_LITERAL_APPEND);
  
  _exit(1);
}

int
main(int argc, char **argv)
{
  // install signal handler for the crash call stacks
  {
    struct sigaction handler = { .sa_sigaction = lnx_signal_handler, .sa_flags = SA_SIGINFO, };
    sigfillset(&handler.sa_mask);
    sigaction(SIGILL, &handler, NULL);
    sigaction(SIGTRAP, &handler, NULL);
    sigaction(SIGABRT, &handler, NULL);
    sigaction(SIGFPE, &handler, NULL);
    sigaction(SIGBUS, &handler, NULL);
    sigaction(SIGSEGV, &handler, NULL);
    sigaction(SIGQUIT, &handler, NULL);
  }
  
  //- rjf: set up OS layer
  {
    //- rjf: get statically-allocated system/process info
    {
      SystemInfo *info = &lnx_state.system_info;
      info->logical_processor_count = (U32)get_nprocs();
      info->page_size               = (U64)getpagesize();
      info->large_page_size         = MB(2);
      info->allocation_granularity  = info->page_size;
    }
    {
      ProcessInfo *info = &lnx_state.process_info;
      info->pid = (U32)getpid();
    }
    
    //- rjf: set up thread context
    TCTX *tctx = tctx_alloc();
    tctx_select(tctx);
    
    //- rjf: set up dynamically allocated state
    lnx_state.arena = arena_alloc();
    lnx_state.entity_arena = arena_alloc();
    pthread_mutex_init(&lnx_state.entity_mutex, 0);
    
    // cache default environment
    {
      U64 env_count = 0;
      for(; __environ[env_count] != 0; env_count += 1) {}
      char **default_env = push_array(lnx_state.arena, char *, env_count+1);
      for EachIndex(idx, env_count)
      {
        default_env[idx] = (char *)str8_copy(lnx_state.arena, str8_cstring(__environ[idx])).str;
      }
      default_env[env_count] = 0;
      lnx_state.default_env_count = env_count;
      lnx_state.default_env       = default_env;
    }
    
    //- rjf: grab dynamically allocated system info
    {
      Temp scratch = scratch_begin(0, 0);
      SystemInfo *info = &lnx_state.system_info;
      
      // rjf: get machine name
      B32 got_final_result = 0;
      U8 *buffer = 0;
      int size = 0;
      for(S64 cap = 4096, r = 0; r < 4; cap *= 2, r += 1)
      {
        scratch_end(scratch);
        buffer = push_array(scratch.arena, U8, cap);
        int gethostname_result = gethostname((char*)buffer, cap);
        size = cstring8_length(buffer);
        if(gethostname_result == 0 && size < cap)
        {
          got_final_result = 1;
          break;
        }
      }
      
      // rjf: save name to info
      if(got_final_result && size > 0)
      {
        info->machine_name.size = size;
        info->machine_name.str = push_array_no_zero(lnx_state.arena, U8, info->machine_name.size + 1);
        MemoryCopy(info->machine_name.str, buffer, info->machine_name.size);
        info->machine_name.str[info->machine_name.size] = 0;
      }
      
      scratch_end(scratch);
    }
    
    //- rjf: grab dynamically allocated process info
    {
      Temp scratch = scratch_begin(0, 0);
      ProcessInfo *info = &lnx_state.process_info;
      
      // rjf: grab binary path
      {
        // rjf: get self string
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
        
        // rjf: save
        if(got_final_result && size > 0)
        {
          String8 full_name = str8(buffer, size);
          info->binary_file_path = push_str8_copy(lnx_state.arena, full_name);
          info->binary_path = str8_chop_last_slash(info->binary_file_path);
        }
      }
      
      // rjf: grab initial directory
      {
        info->initial_path = get_current_path(lnx_state.arena);
      }
      
      // rjf: grab home directory
      {
        char *home = getenv("HOME");
        info->user_program_data_path = str8_cstring(home);
      }
      
      scratch_end(scratch);
    }
  }
  
  //- rjf: call into "real" entry point
  main_thread_base_entry_point(argc, argv);
}
