// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <stdio.h>

////////////////////////////////
//~ rjf: Globals

global pthread_mutex_t lnx_mutex = {0};
global Arena *lnx_perm_arena = 0;
global String8List lnx_cmd_line_args = {0};
global LNX_Entity lnx_entity_buffer[1024];
global LNX_Entity *lnx_entity_free = 0;
global String8 lnx_initial_path = {0};
thread_static LNX_SafeCallChain *lnx_safe_call_chain = 0;

////////////////////////////////
//~ rjf: Helpers

internal B32
lnx_write_list_to_file_descriptor(int fd, String8List list){
  B32 success = true;
  
  String8Node *node = list.first;
  if (node != 0){
    U8 *ptr = node->string.str;;
    U8 *opl = ptr + node->string.size;
    
    U64 p = 0;
    for (;p < list.total_size;){
      U64 amt64 = (U64)(opl - ptr);
      U32 amt = u32_from_u64_saturate(amt64);
      S64 written_amt = write(fd, ptr, amt);
      if (written_amt < 0){
        break;
      }
      p += written_amt;
      ptr += written_amt;
      
      Assert(ptr <= opl);
      if (ptr == opl){
        node = node->next;
        if (node == 0){
          if (p < list.total_size){
            success = false;
          }
          break;
        }
        ptr = node->string.str;
        opl = ptr + node->string.size;
      }
    }
  }
  
  return(success);
}

internal void
lnx_date_time_from_tm(DateTime *out, struct tm *in, U32 msec){
  out->msec = msec;
  out->sec  = in->tm_sec;
  out->min  = in->tm_min;
  out->hour = in->tm_hour;
  out->day  = in->tm_mday - 1;
  out->wday = in->tm_wday;
  out->mon  = in->tm_mon;
  out->year = in->tm_year + 1900;
}

internal void
lnx_tm_from_date_time(struct tm *out, DateTime *in){
  out->tm_sec  = in->sec;
  out->tm_min  = in->min;
  out->tm_hour = in->hour;
  out->tm_mday = in->day + 1;
  out->tm_mon  = in->mon;
  out->tm_year = in->year - 1900;
}

internal void
lnx_dense_time_from_timespec(DenseTime *out, struct timespec *in){
  struct tm tm_time = {0};
  gmtime_r(&in->tv_sec, &tm_time);
  DateTime date_time = {0};
  lnx_date_time_from_tm(&date_time, &tm_time, in->tv_nsec/Million(1));
  *out = dense_time_from_date_time(date_time);
}

internal void
lnx_file_properties_from_stat(FileProperties *out, struct stat *in){
  MemoryZeroStruct(out);
  out->size = in->st_size;
  lnx_dense_time_from_timespec(&out->created, &in->st_ctim);
  lnx_dense_time_from_timespec(&out->modified, &in->st_mtim);
  if ((in->st_mode & S_IFDIR) != 0){
    out->flags |= FilePropertyFlag_IsFolder;
  }
}

internal String8
lnx_string_from_signal(int signum){
  String8 result = str8_lit("<unknown-signal>");
  switch (signum){
    case SIGABRT:
    {
      result = str8_lit("SIGABRT");
    }break;
    case SIGALRM:
    {
      result = str8_lit("SIGALRM");
    }break;
    case SIGBUS:
    {
      result = str8_lit("SIGBUS");
    }break;
    case SIGCHLD:
    {
      result = str8_lit("SIGCHLD");
    }break;
    case SIGCONT:
    {
      result = str8_lit("SIGCONT");
    }break;
    case SIGFPE:
    {
      result = str8_lit("SIGFPE");
    }break;
    case SIGHUP:
    {
      result = str8_lit("SIGHUP");
    }break;
    case SIGILL:
    {
      result = str8_lit("SIGILL");
    }break;
    case SIGINT:
    {
      result = str8_lit("SIGINT");
    }break;
    case SIGIO:
    {
      result = str8_lit("SIGIO");
    }
    case SIGKILL:
    {
      result = str8_lit("SIGKILL");
    }break;
    case SIGPIPE:
    {
      result = str8_lit("SIGPIPE");
    }break;
    case SIGPROF:
    {
      result = str8_lit("SIGPROF");
    }break;
    case SIGPWR:
    {
      result = str8_lit("SIGPWR");
    }break;
    case SIGQUIT:
    {
      result = str8_lit("SIGQUIT");
    }break;
    case SIGSEGV:
    {
      result = str8_lit("SIGSEGV");
    }break;
    case SIGSTKFLT:
    {
      result = str8_lit("SIGSTKFLT");
    }break;
    case SIGSTOP:
    {
      result = str8_lit("SIGSTOP");
    }break;
    case SIGTSTP:
    {
      result = str8_lit("SIGTSTP");
    }break;
    case SIGSYS:
    {
      result = str8_lit("SIGSYS");
    }break;
    case SIGTERM:
    {
      result = str8_lit("SIGTERM");
    }break;
    case SIGTRAP:
    {
      result = str8_lit("SIGTRAP");
    }break;
    case SIGTTIN:
    {
      result = str8_lit("SIGTTIN");
    }break;
    case SIGTTOU:
    {
      result = str8_lit("SIGTTOU");
    }break;
    case SIGURG:
    {
      result = str8_lit("SIGURG");
    }break;
    case SIGUSR1:
    {
      result = str8_lit("SIGUSR1");
    }break;
    case SIGUSR2:
    {
      result = str8_lit("SIGUSR2");
    }break;
    case SIGVTALRM:
    {
      result = str8_lit("SIGVTALRM");
    }break;
    case SIGXCPU:
    {
      result = str8_lit("SIGXCPU");
    }break;
    case SIGXFSZ:
    {
      result = str8_lit("SIGXFSZ");
    }break;
    case SIGWINCH:
    {
      result = str8_lit("SIGWINCH");
    }break;
  }
  return(result);
}

internal String8
lnx_string_from_errno(int error_number){
  String8 result = str8_lit("<unknown-errno>");
  switch (error_number){
    case EPERM:
    {
      result = str8_lit("EPERM");
    }break;
    case ENOENT:
    {
      result = str8_lit("ENOENT");
    }break;
    case ESRCH:
    {
      result = str8_lit("ESRCH");
    }break;
    case EINTR:
    {
      result = str8_lit("EINTR");
    }break;
    case EIO:
    {
      result = str8_lit("EIO");
    }break;
    case ENXIO:
    {
      result = str8_lit("ENXIO");
    }break;
    case E2BIG:
    {
      result = str8_lit("E2BIG");
    }break;
    case ENOEXEC:
    {
      result = str8_lit("ENOEXEC");
    }break;
    case EBADF:
    {
      result = str8_lit("EBADF");
    }break;
    case ECHILD:
    {
      result = str8_lit("ECHILD");
    }break;
    case EAGAIN:
    {
      result = str8_lit("EAGAIN");
    }break;
    case ENOMEM:
    {
      result = str8_lit("ENOMEM");
    }break;
    case EACCES:
    {
      result = str8_lit("EACCES");
    }break;
    case EFAULT:
    {
      result = str8_lit("EFAULT");
    }break;
    case ENOTBLK:
    {
      result = str8_lit("ENOTBLK");
    }break;
    case EBUSY:
    {
      result = str8_lit("EBUSY");
    }break;
    case EEXIST:
    {
      result = str8_lit("EEXIST");
    }break;
    case EXDEV:
    {
      result = str8_lit("EXDEV");
    }break;
    case ENODEV:
    {
      result = str8_lit("ENODEV");
    }break;
    case ENOTDIR:
    {
      result = str8_lit("ENOTDIR");
    }break;
    case EISDIR:
    {
      result = str8_lit("EISDIR");
    }break;
    case EINVAL:
    {
      result = str8_lit("EINVAL");
    }break;
    case ENFILE:
    {
      result = str8_lit("ENFILE");
    }break;
    case EMFILE:
    {
      result = str8_lit("EMFILE");
    }break;
    case ENOTTY:
    {
      result = str8_lit("ENOTTY");
    }break;
    case ETXTBSY:
    {
      result = str8_lit("ETXTBSY");
    }break;
    case EFBIG:
    {
      result = str8_lit("EFBIG");
    }break;
    case ENOSPC:
    {
      result = str8_lit("ENOSPC");
    }break;
    case ESPIPE:
    {
      result = str8_lit("ESPIPE");
    }break;
    case EROFS:
    {
      result = str8_lit("EROFS");
    }break;
    case EMLINK:
    {
      result = str8_lit("EMLINK");
    }break;
    case EPIPE:
    {
      result = str8_lit("EPIPE");
    }break;
    case EDOM:
    {
      result = str8_lit("EDOM");
    }break;
    case ERANGE:
    {
      result = str8_lit("ERANGE");
    }break;
    case EDEADLK:
    {
      result = str8_lit("EDEADLK");
    }break;
    case ENAMETOOLONG:
    {
      result = str8_lit("ENAMETOOLONG");
    }break;
    case ENOLCK:
    {
      result = str8_lit("ENOLCK");
    }break;
    case ENOSYS:
    {
      result = str8_lit("ENOSYS");
    }break;
    case ENOTEMPTY:
    {
      result = str8_lit("ENOTEMPTY");
    }break;
    case ELOOP:
    {
      result = str8_lit("ELOOP");
    }break;
    case ENOMSG:
    {
      result = str8_lit("ENOMSG");
    }break;
    case EIDRM:
    {
      result = str8_lit("EIDRM");
    }break;
    case ECHRNG:
    {
      result = str8_lit("ECHRNG");
    }break;
    case EL2NSYNC:
    {
      result = str8_lit("EL2NSYNC");
    }break;
    case EL3HLT:
    {
      result = str8_lit("EL3HLT");
    }break;
    case EL3RST:
    {
      result = str8_lit("EL3RST");
    }break;
    case ELNRNG:
    {
      result = str8_lit("ELNRNG");
    }break;
    case EUNATCH:
    {
      result = str8_lit("EUNATCH");
    }break;
    case ENOCSI:
    {
      result = str8_lit("ENOCSI");
    }break;
    case EL2HLT:
    {
      result = str8_lit("EL2HLT");
    }break;
    case EBADE:
    {
      result = str8_lit("EBADE");
    }break;
    case EBADR:
    {
      result = str8_lit("EBADR");
    }break;
    case EXFULL:
    {
      result = str8_lit("EXFULL");
    }break;
    case ENOANO:
    {
      result = str8_lit("ENOANO");
    }break;
    case EBADRQC:
    {
      result = str8_lit("EBADRQC");
    }break;
    case EBADSLT:
    {
      result = str8_lit("EBADSLT");
    }break;
    case EBFONT:
    {
      result = str8_lit("EBFONT");
    }break;
    case ENOSTR:
    {
      result = str8_lit("ENOSTR");
    }break;
    case ENODATA:
    {
      result = str8_lit("ENODATA");
    }break;
    case ETIME:
    {
      result = str8_lit("ETIME");
    }break;
    case ENOSR:
    {
      result = str8_lit("ENOSR");
    }break;
    case ENONET:
    {
      result = str8_lit("ENONET");
    }break;
    case ENOPKG:
    {
      result = str8_lit("ENOPKG");
    }break;
    case EREMOTE:
    {
      result = str8_lit("EREMOTE");
    }break;
    case ENOLINK:
    {
      result = str8_lit("ENOLINK");
    }break;
    case EADV:
    {
      result = str8_lit("EADV");
    }break;
    case ESRMNT:
    {
      result = str8_lit("ESRMNT");
    }break;
    case ECOMM:
    {
      result = str8_lit("ECOMM");
    }break;
    case EPROTO:
    {
      result = str8_lit("EPROTO");
    }break;
    case EMULTIHOP:
    {
      result = str8_lit("EMULTIHOP");
    }break;
    case EDOTDOT:
    {
      result = str8_lit("EDOTDOT");
    }break;
    case EBADMSG:
    {
      result = str8_lit("EBADMSG");
    }break;
    case EOVERFLOW:
    {
      result = str8_lit("EOVERFLOW");
    }break;
    case ENOTUNIQ:
    {
      result = str8_lit("ENOTUNIQ");
    }break;
    case EBADFD:
    {
      result = str8_lit("EBADFD");
    }break;
    case EREMCHG:
    {
      result = str8_lit("EREMCHG");
    }break;
    case ELIBACC:
    {
      result = str8_lit("ELIBACC");
    }break;
    case ELIBBAD:
    {
      result = str8_lit("ELIBBAD");
    }break;
    case ELIBSCN:
    {
      result = str8_lit("ELIBSCN");
    }break;
    case ELIBMAX:
    {
      result = str8_lit("ELIBMAX");
    }break;
    case ELIBEXEC:
    {
      result = str8_lit("ELIBEXEC");
    }break;
    case EILSEQ:
    {
      result = str8_lit("EILSEQ");
    }break;
    case ERESTART:
    {
      result = str8_lit("ERESTART");
    }break;
    case ESTRPIPE:
    {
      result = str8_lit("ESTRPIPE");
    }break;
    case EUSERS:
    {
      result = str8_lit("EUSERS");
    }break;
    case ENOTSOCK:
    {
      result = str8_lit("ENOTSOCK");
    }break;
    case EDESTADDRREQ:
    {
      result = str8_lit("EDESTADDRREQ");
    }break;
    case EMSGSIZE:
    {
      result = str8_lit("EMSGSIZE");
    }break;
    case EPROTOTYPE:
    {
      result = str8_lit("EPROTOTYPE");
    }break;
    case ENOPROTOOPT:
    {
      result = str8_lit("ENOPROTOOPT");
    }break;
    case EPROTONOSUPPORT:
    {
      result = str8_lit("EPROTONOSUPPORT");
    }break;
    case ESOCKTNOSUPPORT:
    {
      result = str8_lit("ESOCKTNOSUPPORT");
    }break;
    case EOPNOTSUPP:
    {
      result = str8_lit("EOPNOTSUPP");
    }break;
    case EPFNOSUPPORT:
    {
      result = str8_lit("EPFNOSUPPORT");
    }break;
    case EAFNOSUPPORT:
    {
      result = str8_lit("EAFNOSUPPORT");
    }break;
    case EADDRINUSE:
    {
      result = str8_lit("EADDRINUSE");
    }break;
    case EADDRNOTAVAIL:
    {
      result = str8_lit("EADDRNOTAVAIL");
    }break;
    case ENETDOWN:
    {
      result = str8_lit("ENETDOWN");
    }break;
    case ENETUNREACH:
    {
      result = str8_lit("ENETUNREACH");
    }break;
    case ENETRESET:
    {
      result = str8_lit("ENETRESET");
    }break;
    case ECONNABORTED:
    {
      result = str8_lit("ECONNABORTED");
    }break;
    case ECONNRESET:
    {
      result = str8_lit("ECONNRESET");
    }break;
    case ENOBUFS:
    {
      result = str8_lit("ENOBUFS");
    }break;
    case EISCONN:
    {
      result = str8_lit("EISCONN");
    }break;
    case ENOTCONN:
    {
      result = str8_lit("ENOTCONN");
    }break;
    case ESHUTDOWN:
    {
      result = str8_lit("ESHUTDOWN");
    }break;
    case ETOOMANYREFS:
    {
      result = str8_lit("ETOOMANYREFS");
    }break;
    case ETIMEDOUT:
    {
      result = str8_lit("ETIMEDOUT");
    }break;
    case ECONNREFUSED:
    {
      result = str8_lit("ECONNREFUSED");
    }break;
    case EHOSTDOWN:
    {
      result = str8_lit("EHOSTDOWN");
    }break;
    case EHOSTUNREACH:
    {
      result = str8_lit("EHOSTUNREACH");
    }break;
    case EALREADY:
    {
      result = str8_lit("EALREADY");
    }break;
    case EINPROGRESS:
    {
      result = str8_lit("EINPROGRESS");
    }break;
    case ESTALE:
    {
      result = str8_lit("ESTALE");
    }break;
    case EUCLEAN:
    {
      result = str8_lit("EUCLEAN");
    }break;
    case ENOTNAM:
    {
      result = str8_lit("ENOTNAM");
    }break;
    case ENAVAIL:
    {
      result = str8_lit("ENAVAIL");
    }break;
    case EISNAM:
    {
      result = str8_lit("EISNAM");
    }break;
    case EREMOTEIO:
    {
      result = str8_lit("EREMOTEIO");
    }break;
    case EDQUOT:
    {
      result = str8_lit("EDQUOT");
    }break;
    case ENOMEDIUM:
    {
      result = str8_lit("ENOMEDIUM");
    }break;
    case EMEDIUMTYPE:
    {
      result = str8_lit("EMEDIUMTYPE");
    }break;
    case ECANCELED:
    {
      result = str8_lit("ECANCELED");
    }break;
    case ENOKEY:
    {
      result = str8_lit("ENOKEY");
    }break;
    case EKEYEXPIRED:
    {
      result = str8_lit("EKEYEXPIRED");
    }break;
    case EKEYREVOKED:
    {
      result = str8_lit("EKEYREVOKED");
    }break;
    case EKEYREJECTED:
    {
      result = str8_lit("EKEYREJECTED");
    }break;
    case EOWNERDEAD:
    {
      result = str8_lit("EOWNERDEAD");
    }break;
    case ENOTRECOVERABLE:
    {
      result = str8_lit("ENOTRECOVERABLE");
    }break;
    case ERFKILL:
    {
      result = str8_lit("ERFKILL");
    }break;
    case EHWPOISON:
    {
      result = str8_lit("EHWPOISON");
    }break;
  }
  return(result);
}

internal LNX_Entity*
lnx_alloc_entity(LNX_EntityKind kind){
  pthread_mutex_lock(&lnx_mutex);
  LNX_Entity *result = lnx_entity_free;
  Assert(result != 0);
  SLLStackPop(lnx_entity_free);
  pthread_mutex_unlock(&lnx_mutex);
  result->kind = kind;
  return(result);
}

internal void
lnx_free_entity(LNX_Entity *entity){
  entity->kind = LNX_EntityKind_Null;
  pthread_mutex_lock(&lnx_mutex);
  SLLStackPush(lnx_entity_free, entity);
  pthread_mutex_unlock(&lnx_mutex);
}

internal void*
lnx_thread_base(void *ptr){
  LNX_Entity *entity = (LNX_Entity*)ptr;
  OS_ThreadFunctionType *func = entity->thread.func;
  void *thread_ptr = entity->thread.ptr;
  
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  func(thread_ptr);
  tctx_release();
  
  // remove my bit
  U32 result = __sync_fetch_and_and(&entity->reference_mask, ~0x2);
  // if the other bit is also gone, free entity
  if ((result & 0x1) == 0){
    lnx_free_entity(entity);
  }
  return(0);
}

internal void
lnx_safe_call_sig_handler(int){
  LNX_SafeCallChain *chain = lnx_safe_call_chain;
  if (chain != 0 && chain->fail_handler != 0){
    chain->fail_handler(chain->ptr);
  }
  abort();
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_init(void)
{
  // NOTE(allen): Initialize linux layer mutex
  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int pthread_result = pthread_mutex_init(&lnx_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (pthread_result == -1){
      abort();
    }
  }
  MemoryZeroArray(lnx_entity_buffer);
  {
    LNX_Entity *ptr = lnx_entity_free = lnx_entity_buffer;
    for (U64 i = 1; i < ArrayCount(lnx_entity_buffer); i += 1, ptr += 1){
      ptr->next = ptr + 1;
    }
    ptr->next = 0;
  }
  
  // NOTE(allen): Permanent memory allocator for this layer
  Arena *perm_arena = arena_alloc();
  lnx_perm_arena = perm_arena;
  
  // NOTE(allen): Initialize Paths
  lnx_initial_path = os_get_path(lnx_perm_arena, OS_SystemPath_Current);
  
  // NOTE(rjf): Setup command line args
  lnx_cmd_line_args = os_string_list_from_argcv(lnx_perm_arena, argc, argv);
}

////////////////////////////////
//~ rjf: @os_hooks Memory Allocation (Implemented Per-OS)

internal void*
os_reserve(U64 size){
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  return(result);
}

internal B32
os_commit(void *ptr, U64 size){
  mprotect(ptr, size, PROT_READ|PROT_WRITE);
  // TODO(allen): can we test this?
  return(true);
}

internal void*
os_reserve_large(U64 size){
  NotImplemented;
  return 0;
}

internal B32
os_commit_large(void *ptr, U64 size){
  NotImplemented;
  return 0;
}

internal void
os_decommit(void *ptr, U64 size){
  madvise(ptr, size, MADV_DONTNEED);
  mprotect(ptr, size, PROT_NONE);
}

internal void
os_release(void *ptr, U64 size){
  munmap(ptr, size);
}

internal void
os_set_large_pages(B32 flag)
{
  NotImplemented;
}

internal B32
os_large_pages_enabled(void)
{
  NotImplemented;
  return 0;
}

internal U64
os_large_page_size(void)
{
  NotImplemented;
  return 0;
}

internal void*
os_alloc_ring_buffer(U64 size, U64 *actual_size_out)
{
  NotImplemented;
  return 0;
}

internal void
os_free_ring_buffer(void *ring_buffer, U64 actual_size)
{
  NotImplemented;
}

////////////////////////////////
//~ rjf: @os_hooks System Info (Implemented Per-OS)

internal String8
os_machine_name(void){
  local_persist B32 first = true;
  local_persist String8 name = {0};
  
  // TODO(allen): let's just pre-compute this at init and skip the complexity
  pthread_mutex_lock(&lnx_mutex);
  if (first){
    Temp scratch = scratch_begin(0, 0);
    first = false;
    
    // get name
    B32 got_final_result = false;
    U8 *buffer = 0;
    int size = 0;
    for (S64 cap = 4096, r = 0;
         r < 4;
         cap *= 2, r += 1){
      scratch.restore();
      buffer = push_array_no_zero(scratch.arena, U8, cap);
      size = gethostname((char*)buffer, cap);
      if (size < cap){
        got_final_result = true;
        break;
      }
    }
    
    // save string
    if (got_final_result && size > 0){
      name.size = size;
      name.str = push_array_no_zero(lnx_perm_arena, U8, name.size + 1);
      MemoryCopy(name.str, buffer, name.size);
      name.str[name.size] = 0;
    }
    
    scratch_end(scratch);
  }
  pthread_mutex_unlock(&lnx_mutex);
  
  return(name);
}

internal U64
os_page_size(void){
  int size = getpagesize();
  return((U64)size);
}

internal U64
os_allocation_granularity(void)
{
  // On linux there is no equivalent of "dwAllocationGranularity"
  os_page_size();
}

internal U64
os_logical_core_count(void)
{
  // TODO(rjf): check this
  return get_nprocs();
}

////////////////////////////////
//~ rjf: @os_hooks Process & Thread Info (Implemented Per-OS)

internal String8List
os_get_command_line_arguments(void)
{
  return lnx_cmd_line_args;
}

internal S32
os_get_pid(void){
  S32 result = getpid();
  return(result);
}

internal S32
os_get_tid(void){
  S32 result = 0;
#ifdef SYS_gettid
  result = syscall(SYS_gettid);
#else
  result = gettid();
#endif
  return(result);
}

internal String8List
os_get_environment(void)
{
  NotImplemented;
  String8List result = {0};
  return result;
}

internal U64
os_string_list_from_system_path(Arena *arena, OS_SystemPath path, String8List *out){
  U64 result = 0;
  
  switch (path){
    case OS_SystemPath_Binary:
    {
      local_persist B32 first = true;
      local_persist String8 name = {0};
      
      // TODO(allen): let's just pre-compute this at init and skip the complexity
      pthread_mutex_lock(&lnx_mutex);
      if (first){
        Temp scratch = scratch_begin(&arena, 1);
        first = false;
        
        // get self string
        B32 got_final_result = false;
        U8 *buffer = 0;
        int size = 0;
        for (S64 cap = PATH_MAX, r = 0;
             r < 4;
             cap *= 2, r += 1){
          scratch.restore();
          buffer = push_array_no_zero(scratch.arena, U8, cap);
          size = readlink("/proc/self/exe", (char*)buffer, cap);
          if (size < cap){
            got_final_result = true;
            break;
          }
        }
        
        // save string
        if (got_final_result && size > 0){
          String8 full_name = str8(buffer, size);
          String8 name_chopped = string_path_chop_last_slash(full_name);
          name = push_str8_copy(lnx_perm_arena, name_chopped);
        }
        
        scratch_end(scratch);
      }
      pthread_mutex_unlock(&lnx_mutex);
      
      result = 1;
      str8_list_push(arena, out, name);
    }break;
    
    case OS_SystemPath_Initial:
    {
      Assert(lnx_initial_path.str != 0);
      result = 1;
      str8_list_push(arena, out, lnx_initial_path);
    }break;
    
    case OS_SystemPath_Current:
    {
      char *cwdir = getcwd(0, 0);
      String8 string = push_str8_copy(arena, str8_cstring(cwdir));
      free(cwdir);
      result = 1;
      str8_list_push(arena, out, string);
    }break;
    
    case OS_SystemPath_UserProgramData:
    {
      char *home = getenv("HOME");
      String8 string = str8_cstring(home);
      result = 1;
      str8_list_push(arena, out, string);
    }break;
    
    case OS_SystemPath_ModuleLoad:
    {
      // TODO(allen): this one is big and complicated and only needed for making
      // a debugger, skipping for now.
      NotImplemented;
    }break;
  }
  
  return(result);
}

////////////////////////////////
//~ rjf: @os_hooks Process Control (Implemented Per-OS)

internal void
os_exit_process(S32 exit_code){
  exit(exit_code);
}

////////////////////////////////
//~ rjf: @os_hooks File System (Implemented Per-OS)

//- rjf: files

internal OS_Handle
os_file_open(OS_AccessFlags flags, String8 path)
{
  OS_Handle file = {0};
  NotImplemented;
  return file;
}

internal void
os_file_close(OS_Handle file)
{
  NotImplemented;
}

internal U64
os_file_read(OS_Handle file, Rng1U64 rng, void *out_data)
{
  NotImplemented;
  return 0;
}

internal void
os_file_write(OS_Handle file, Rng1U64 rng, void *data)
{
  NotImplemented;
}

internal B32
os_file_set_times(OS_Handle file, DateTime time)
{
  NotImplemented;
}

internal FileProperties
os_properties_from_file(OS_Handle file)
{
  FileProperties props = {0};
  NotImplemented;
  return props;
}

internal OS_FileID
os_id_from_file(OS_Handle file)
{
  // TODO(nick): querry struct stat with fstat(2) and use st_dev and st_ino as ids
  OS_FileID id = {0};
  NotImplemented;
  return id;
}

internal B32
os_delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = false;
  String8 name_copy = push_str8_copy(scratch.arena, name);
  if (remove((char*)name_copy.str) != -1){
    result = true;
  }
  scratch_end(scratch);
  return(result);
}

internal B32
os_copy_file_path(String8 dst, String8 src)
{
  NotImplemented;
  return 0;
}

internal String8
os_full_path_from_path(Arena *arena, String8 path)
{
  // TODO: realpath can be used to resolve full path
  String8 result = {0};
  NotImplemented;
  return result;
}

internal B32
os_file_path_exists(String8 path)
{
  NotImplemented;
  return 0;
}

internal FileProperties
os_properties_from_file_path(String8 path)
{
  FileProperties props = {0};
  NotImplemented;
  return props;
}

//- rjf: file maps

internal OS_Handle
os_file_map_open(OS_AccessFlags flags, OS_Handle file)
{
  NotImplemented;
  OS_Handle handle = {0};
  return handle;
}

internal void
os_file_map_close(OS_Handle map)
{
  NotImplemented;
}

internal void *
os_file_map_view_open(OS_Handle map, OS_AccessFlags flags, Rng1U64 range)
{
  NotImplemented;
  return 0;
}

internal void
os_file_map_view_close(OS_Handle map, void *ptr)
{
  NotImplemented;
}

//- rjf: directory iteration

internal OS_FileIter *
os_file_iter_begin(Arena *arena, String8 path, OS_FileIterFlags flags)
{
  NotImplemented;
  return 0;
}

internal B32
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  NotImplemented;
  return 0;
}

internal void
os_file_iter_end(OS_FileIter *iter)
{
  NotImplemented;
}

//- rjf: directory creation

internal B32
os_make_directory(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = false;
  String8 name_copy = push_str8_copy(scratch.arena, name);
  if (mkdir((char*)name_copy.str, 0777) != -1){
    result = true;
  }
  scratch_end(scratch);
  return(result);
}

////////////////////////////////
//~ rjf: @os_hooks Shared Memory (Implemented Per-OS)

internal OS_Handle
os_shared_memory_alloc(U64 size, String8 name)
{
  OS_Handle result = {0};
  NotImplemented;
  return result;
}

internal OS_Handle
os_shared_memory_open(String8 name)
{
  OS_Handle result = {0};
  NotImplemented;
  return result;
}

internal void
os_shared_memory_close(OS_Handle handle)
{
  NotImplemented;
}

internal void *
os_shared_memory_view_open(OS_Handle handle, Rng1U64 range)
{
  NotImplemented;
  return 0;
}

internal void
os_shared_memory_view_close(OS_Handle handle, void *ptr)
{
  NotImplemented;
}

////////////////////////////////
//~ rjf: @os_hooks Time (Implemented Per-OS)

internal OS_UnixTime
os_now_unix(void)
{
  time_t t = time(0);
  return (OS_UnixTime)t;
}

internal DateTime
os_now_universal_time(void){
  time_t t = 0;
  time(&t);
  struct tm universal_tm = {0};
  gmtime_r(&t, &universal_tm);
  DateTime result = {0};
  lnx_date_time_from_tm(&result, &universal_tm, 0);
  return(result);
}

internal DateTime
os_universal_time_from_local_time(DateTime *local_time){
  // local time -> universal time (using whatever types it takes)
  struct tm local_tm = {0};
  lnx_tm_from_date_time(&local_tm, local_time);
  local_tm.tm_isdst = -1;
  time_t universal_t = mktime(&local_tm);
  
  // whatever type we ended up with -> DateTime (don't alter the space along the way)
  struct tm universal_tm = {0};
  gmtime_r(&universal_t, &universal_tm);
  DateTime result = {0};
  lnx_date_time_from_tm(&result, &universal_tm, 0);
  return(result);
}

internal DateTime
os_local_time_from_universal_time(DateTime *universal_time){
  // universal time -> local time (using whatever types it takes)
  struct tm universal_tm = {0};
  lnx_tm_from_date_time(&universal_tm, universal_time);
  universal_tm.tm_isdst = -1;
  time_t universal_t = timegm(&universal_tm);
  struct tm local_tm = {0};
  localtime_r(&universal_t, &local_tm);
  
  // whatever type we ended up with -> DateTime (don't alter the space along the way)
  DateTime result = {0};
  lnx_date_time_from_tm(&result, &local_tm, 0);
  return(result);
}

internal U64
os_now_microseconds(void){
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  U64 result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
  return(result);
}

internal void
os_sleep_milliseconds(U32 msec){
  usleep(msec*Thousand(1));
}

internal OS_UnixTime
os_get_process_start_time(void)
{
  NotImplemented;
  return 0;
}

////////////////////////////////
//~ rjf: @os_hooks Child Processes (Implemented Per-OS)

internal B32
os_launch_process(OS_LaunchOptions *options){
  // TODO(allen): I want to redo this API before I bother implementing it here
  NotImplemented;
  return(false);
}

////////////////////////////////
//~ rjf: @os_hooks Threads (Implemented Per-OS)

internal OS_Handle
os_launch_thread(OS_ThreadFunctionType *func, void *ptr, void *params){
  // entity
  LNX_Entity *entity = lnx_alloc_entity(LNX_EntityKind_Thread);
  entity->reference_mask = 0x3;
  entity->thread.func = func;
  entity->thread.ptr = ptr;
  
  // pthread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int pthread_result = pthread_create(&entity->thread.handle, &attr, lnx_thread_base, entity);
  pthread_attr_destroy(&attr);
  if (pthread_result == -1){
    lnx_free_entity(entity);
    entity = 0;
  }
  
  // cast to opaque handle
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_release_thread_handle(OS_Handle thread){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(thread.id);
  // remove my bit
  U32 result = __sync_fetch_and_and(&entity->reference_mask, ~0x1);
  // if the other bit is also gone, free entity
  if ((result & 0x2) == 0){
    lnx_free_entity(entity);
  }
}

////////////////////////////////
//~ rjf: @os_hooks Synchronization Primitives (Implemented Per-OS)

// NOTE(allen): Mutexes are recursive - support counted acquire/release nesting
// on a single thread

//- rjf: recursive mutexes

internal OS_Handle
os_mutex_alloc(void){
  // entity
  LNX_Entity *entity = lnx_alloc_entity(LNX_EntityKind_Mutex);
  
  // pthread
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  int pthread_result = pthread_mutex_init(&entity->mutex, &attr);
  pthread_mutexattr_destroy(&attr);
  if (pthread_result == -1){
    lnx_free_entity(entity);
    entity = 0;
  }
  
  // cast to opaque handle
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_mutex_release(OS_Handle mutex){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.id);
  pthread_mutex_destroy(&entity->mutex);
  lnx_free_entity(entity);
}

internal void
os_mutex_take_(OS_Handle mutex){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.id);
  pthread_mutex_lock(&entity->mutex);
}

internal void
os_mutex_drop_(OS_Handle mutex){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.id);
  pthread_mutex_unlock(&entity->mutex);
}

//- rjf: reader/writer mutexes

internal OS_Handle
os_rw_mutex_alloc(void)
{
  OS_Handle result = {0};
  NotImplemented;
  return result;
}

internal void
os_rw_mutex_release(OS_Handle rw_mutex)
{
  NotImplemented;
}

internal void
os_rw_mutex_take_r_(OS_Handle mutex)
{
  NotImplemented;
}

internal void
os_rw_mutex_drop_r_(OS_Handle mutex)
{
  NotImplemented;
}

internal void
os_rw_mutex_take_w_(OS_Handle mutex)
{
  NotImplemented;
}

internal void
os_rw_mutex_drop_w_(OS_Handle mutex)
{
  NotImplemented;
}

//- rjf: condition variables

internal OS_Handle
os_condition_variable_alloc(void){
  // entity
  LNX_Entity *entity = lnx_alloc_entity(LNX_EntityKind_ConditionVariable);
  
  // pthread
  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
  int pthread_result = pthread_cond_init(&entity->cond, &attr);
  pthread_condattr_destroy(&attr);
  if (pthread_result == -1){
    lnx_free_entity(entity);
    entity = 0;
  }
  
  // cast to opaque handle
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_condition_variable_release(OS_Handle cv){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(cv.id);
  pthread_cond_destroy(&entity->cond);
  lnx_free_entity(entity);
}

internal B32
os_condition_variable_wait_(OS_Handle cv, OS_Handle mutex, U64 endt_us){
  B32 result = false;
  LNX_Entity *entity_cond = (LNX_Entity*)PtrFromInt(cv.id);
  LNX_Entity *entity_mutex = (LNX_Entity*)PtrFromInt(mutex.id);
  // TODO(allen): implement the time control
  pthread_cond_timedwait(&entity_cond->cond, &entity_mutex->mutex);
  return(result);
}

internal B32
os_condition_variable_wait_rw_r_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  NotImplemented;
  return 0;
}

internal B32
os_condition_variable_wait_rw_w_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  NotImplemented;
  return 0;
}

internal void
os_condition_variable_signal_(OS_Handle cv){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(cv.id);
  pthread_cond_signal(&entity->cond);
}

internal void
os_condition_variable_broadcast_(OS_Handle cv){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(cv.id);
  DontCompile;
}

//- rjf: cross-process semaphores

internal OS_Handle
os_semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  OS_Handle result = {0};
  NotImplemented;
  return result;
}

internal void
os_semaphore_release(OS_Handle semaphore)
{
  NotImplemented;
}

internal OS_Handle
os_semaphore_open(String8 name)
{
  OS_Handle result = {0};
  NotImplemented;
  return result;
}

internal void
os_semaphore_close(OS_Handle semaphore)
{
  NotImplemented;
}

internal B32
os_semaphore_take(OS_Handle semaphore)
{
  NotImplemented;
  return 0;
}

internal void
os_semaphore_drop(OS_Handle semaphore)
{
  NotImplemented;
}

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

internal OS_Handle
os_library_open(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  char *path_cstr = (char *)push_str8_copy(scratch.arena, path).str;
  void *so = dlopen(path_cstr, RTLD_LAZY);
  OS_Handle lib = { (U64)so };
  scratch_end(scratch);
  return lib;
}

internal VoidProc *
os_library_load_proc(OS_Handle lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  void *so = (void *)lib.id;
  char *name_cstr = (char *)push_str8_copy(scratch.arena, name).str;
  VoidProc *proc = (VoidProc *)dlsym(so, name_cstr);
  scratch_end(scratch);
  return proc;
}

internal void
os_library_close(OS_Handle lib)
{
  void *so = (void *)lib.id;
  dlclose(so);
}

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

internal void
os_safe_call(OS_ThreadFunctionType *func, OS_ThreadFunctionType *fail_handler, void *ptr){
  LNX_SafeCallChain chain = {0};
  SLLStackPush(lnx_safe_call_chain, &chain);
  chain.fail_handler = fail_handler;
  chain.ptr = ptr;
  
  struct sigaction new_act = {0};
  new_act.sa_handler = lnx_safe_call_sig_handler;
  
  int signals_to_handle[] = {
    SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP,
  };
  struct sigaction og_act[ArrayCount(signals_to_handle)] = {0};
  
  for (U32 i = 0; i < ArrayCount(signals_to_handle); i += 1){
    sigaction(signals_to_handle[i], &new_act, &og_act[i]);
  }
  
  func(ptr);
  
  for (U32 i = 0; i < ArrayCount(signals_to_handle); i += 1){
    sigaction(signals_to_handle[i], &og_act[i], 0);
  }
}

////////////////////////////////

internal OS_Guid
os_make_guid(void)
{
  NotImplemented;
}

