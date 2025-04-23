// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Globals

global pthread_mutex_t lnx_mutex = {0};
global Arena *lnx_perm_arena = 0;
global LNX_Entity lnx_entity_buffer[16384];
global LNX_Entity *lnx_entity_free = 0;
global String8 lnx_initial_path = {0};
thread_static LNX_SafeCallChain *lnx_safe_call_chain = 0;

global U64 lnx_page_size = 4096;
// TODO(mallchad): This can't be used until the huge page allocation count is checked
// TODO(mallchad): Pretty sure this can be used now????
global B32 lnx_huge_page_enabled = 0;
global B32 lnx_huge_page_use_1GB = 0;
global U16 lnx_ring_buffers_created = 0;
global U16 lnx_ring_buffers_limit = 65000;
global String8List lnx_environment = {0};
global U64 lnx_hugepage_count_2MB = 0;
global U32 lnx_hugepage_count_min = 250;  // (500 MB/2MB)

global String8 lnx_hostname = {0};
global LNX_version lnx_kernel_version = {0};
global String8 lnx_architecture  = {0};
global String8 lnx_kernel_type = {0};

////////////////////////////////n
// Forward Declares
// NOTE: Half of these are just to silence the compiler.
int
memfd_create (const char *__name, unsigned int __flags) __THROW;
ssize_t
copy_file_range (int __infd, __off64_t *__pinoff,
                         int __outfd, __off64_t *__poutoff,
                         size_t __length, unsigned int __flags);
extern int
creat(const char *__file, mode_t __mode);


////////////////////////////////
//~ rjf: Helpers

// TODO: Marked as old / review
internal B32
lnx_write_list_to_file_descriptor(int fd, String8List list){
  B32 success = 1;

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
            success = 0;
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

// TODO: Marked as old / review
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

// TODO: Marked as old / review
internal void
lnx_tm_from_date_time(struct tm *out, DateTime *in){
  out->tm_sec  = in->sec;
  out->tm_min  = in->min;
  out->tm_hour = in->hour;
  out->tm_mday = in->day + 1;
  out->tm_mon  = in->mon;
  out->tm_year = in->year - 1900;
}

internal void lnx_timespec_from_date_time(LNX_timespec* out, DateTime* in)
{
  NotImplemented;
}

internal void lnx_timeval_from_date_time(LNX_timeval* out, DateTime* in)
{
  NotImplemented;
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
lnx_timeval_from_dense_time(LNX_timeval* out, DenseTime* in)
{
  // Miliseconds to Microseconds, should be U64 to long
  out->tv_sec = 0;
  out->tv_usec = 1000* (*in);
}

internal void
lnx_timespec_from_dense_time(LNX_timespec* out, DenseTime* in)
{
   // Miliseconds to Seconds, should be U64 to long
  out->tv_sec = (*in / 1000);
  out->tv_nsec = 0;
}

void
lnx_timespec_from_timeval(LNX_timespec* out, LNX_timeval* in )
{
  out->tv_sec = in->tv_sec;
  out->tv_nsec = in->tv_usec / 1000;
}

void
lnx_timeval_from_timespec(LNX_timeval* out, LNX_timespec* in )
{
  out->tv_sec = in->tv_sec;
  out->tv_usec = in->tv_nsec * 1000;
}

/* NOTE: ctime has very little to do with "creation time"- it's more about
   inode modifications -but for this purpose it's usually considered the closest
   analogue. Manage your own file-creation data if you actually want that info.

   There's way more info to draw from but leaving for now
   https://man7.org/linux/man-pages/man3/stat.3type.html */
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

internal U32
lnx_prot_from_os_flags(OS_AccessFlags flags)
{
  U32 result = 0x0;
  if (flags & OS_AccessFlag_Read) { result |= PROT_READ; }
  if (flags & OS_AccessFlag_Write) { result |= PROT_WRITE; }
  if (flags & OS_AccessFlag_Execute) { result |= PROT_EXEC; }
  return result;
}

internal U32
lnx_open_from_os_flags(OS_AccessFlags flags)
{
  U32 result = 0x0;
  // read/write flags are mutually exclusie on Linux `open()`
  if ((flags & OS_AccessFlag_Write) &&
      (flags & OS_AccessFlag_Read)) { result |= O_RDWR | O_CREAT; }
  else if (flags & OS_AccessFlag_Read) { result |= O_RDONLY; }
  else if (flags & OS_AccessFlag_Write) { result |= O_WRONLY | O_CREAT; }

  // Doesn't make any sense on Linux, use os_file_map_open for execute permissions
  // else if (flags & OS_AccessFlag_Execute) {}
  // Shared doesn't make sense on Linux, file locking is explicit not set at open
  // if(flags & OS_AccessFlag_Shared)  {}

  return result;
}

internal U32
 lnx_fd_from_handle(OS_Handle file)
{
  return *file.u64;
}

internal OS_Handle
lnx_handle_from_fd(U32 fd)
{
  OS_Handle result = {0};
  *result.u64 = fd;
  return result;
}

internal LNX_Entity*
lnx_entity_from_handle(OS_Handle handle, LNX_EntityKind type)
{
  LNX_Entity* result = (LNX_Entity*)PtrFromInt(*handle.u64);
  if (*handle.u64) { Assert(result->kind == type); }
  return result;
}
internal OS_Handle lnx_handle_from_entity(LNX_Entity* entity)
{
  OS_Handle result = {0};
  *result.u64 = IntFromPtr(entity);
  return result;
}

// TODO: Marked as old / review
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

// TODO: Marked as old / review
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

// TODO: Marked as old / review
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

// TODO: Marked as old / review
internal void
lnx_free_entity(LNX_Entity *entity){
  entity->kind = LNX_EntityKind_Null;
  pthread_mutex_lock(&lnx_mutex);
  SLLStackPush(lnx_entity_free, entity);
  pthread_mutex_unlock(&lnx_mutex);
}

// TODO: Marked as old / review
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

// TODO: Marked as old / review
internal void
lnx_safe_call_sig_handler(int _){
  LNX_SafeCallChain *chain = lnx_safe_call_chain;
  if (chain != 0 && chain->fail_handler != 0){
    chain->fail_handler(chain->ptr);
  }
  abort();
}

internal LNX_timespec
lnx_now_precision_timespec()
{
  LNX_timespec result;
  clock_gettime(CLOCK_MONOTONIC_RAW, &result);

  return result;
}

internal LNX_timespec lnx_now_system_timespec()
{
  LNX_timespec result;
  clock_gettime(CLOCK_REALTIME, &result);
  return result;
}


////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

// TODO: Marked as old / review
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

  // Initialize Paths
  // Don't make assumptions just let the path be as big as it needs to be
  U8 _initial_path[1000];
  S32 _initial_size = readlink("/proc/self/exe", (char*)_initial_path, 1000);

  if (_initial_size > 0)
  {
    String8 _initial_tmp = str8(_initial_path, _initial_size);
    str8_chop_last_slash(_initial_tmp);
    lnx_initial_path = push_str8_copy(lnx_perm_arena, _initial_tmp);
  }
  // Give empty string for relative path on the offchance it fails
  else { lnx_initial_path = str8_lit(""); }

  // Environment initialization
  Temp scratch = scratch_begin(0, 0);
  String8 env;
  LNX_fd env_fd = open("/proc/self/environ", O_RDONLY);
  U64 env_limit = 100000;
  U8* environ_buffer = push_array(scratch.arena, U8, env_limit);
  // *SIGH*, the 'environ' file doesn't have a size sso 'os_file_read' can't query it.
  U64 read_success = read(env_fd, environ_buffer, env_limit);
  close(env_fd);

  if (read_success)
  {
    for (U8* x_string=(U8*)environ_buffer;
         (*x_string != 0 && x_string<environ_buffer + env_limit -1 );)
    {
      env = str8_cstring((char*)x_string);
      env = push_str8_copy(lnx_perm_arena, env);
      if (env.size)
      {
        str8_list_push(lnx_perm_arena, &lnx_environment, env);
        x_string += env.size + 1;
      }
    }
  }

  lnx_page_size = (U64)getpagesize();

  // Kernel, Hostname and Architecture Info
  struct utsname kernel = {0};
  S32 uname_error = uname(&kernel);

  // TODO: Parse the string
  lnx_hostname = push_str8_copy( lnx_perm_arena, str8_cstring(kernel.nodename) );
  lnx_kernel_type = push_str8_copy( lnx_perm_arena, str8_cstring(kernel.sysname) ) ;
  lnx_kernel_version.major;
  lnx_kernel_version.minor;
  lnx_kernel_version.patch;
  lnx_kernel_version.string = push_str8_copy( lnx_perm_arena, str8_cstring(kernel.release) );
  lnx_architecture = push_str8_copy( lnx_perm_arena, str8_cstring(kernel.machine) );

  /* We HAVE to set umask to 00 or we get the wrong chmod when trying to create new files
     including memory objects, very annoying.
     umask is *also* not thread-safe. Fun! */
  umask(00);

  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @os_hooks Memory Allocation (Implemented Per-OS)

// TODO: Marked as old / review
internal void*
os_reserve(U64 size){
  void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(result == MAP_FAILED)
  { result = 0; }

  return(result);
}

/* NOTE(mallchad): I wanted to use MADV_POPULATE_READ/WRITE to fault the pages
   into memory here but my kernel was *just* old enough to not have the headers
   for this feature so I will use a trick with mlock instead. Since that will
   work better for users on very old kernels */
internal B32
os_commit(void *ptr, U64 size){
  U32 error = mprotect(ptr, size, PROT_READ|PROT_WRITE);
  mlock(ptr, size);
  munlock(ptr, size);
  // TODO(allen): can we test this?
  return (error == 0);
}

// TODO: Need to check if enough huge pages are available or it will just straight up fail
internal void*
os_reserve_large(U64 size){
  F32 huge_threshold = 0.5f;
  F32 huge_use_1GB = size > (1e9f * huge_threshold);
  U32 page_size = huge_use_1GB ? MAP_HUGE_1GB : MAP_HUGE_2MB;
  void *result;
  if (lnx_huge_page_enabled)
  {
    /* NOTE: Huge pages are permanantly in memory unless paged out under high
     memory pressure, MAP_POPULATE is probably not relevant, but you can mlock
     it anyway if you want to be sure */
    result = mmap(0, size, PROT_NONE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | page_size,
                  -1, 0);
  } else { result = os_reserve(size); }

  return result;
}

internal B32
os_commit_large(void *ptr, U64 size){
  U32 error = mprotect(ptr, size, PROT_READ|PROT_WRITE);
  mlock(ptr, size);
  munlock(ptr, size);
  return (error == 0);
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

// Enable huge pages
// NOTE: Linux has huge pages instead of larges pages but there's no "nice" way to
// enable them from an unprivillaged application, that is unless you want to
// spawn a subprocess just for performing privileged actions
internal B32
os_set_large_pages(B32 flag)
{
  lnx_huge_page_enabled = os_large_pages_enabled();
  return (lnx_huge_page_enabled == flag);
}

internal B32
os_large_pages_enabled(void)
{
  /* NOTE(mallchad): This is aparently the reccomended way to check for hugepage support.
     Do not ask... */
  U8 buffer[32];
  MemoryZeroArray(buffer);
  LNX_fd fd = open("/proc/sys/vm/nr_hugepages", O_RDONLY);
  // NOTE: Fake files have fake behaviour. Keep making this mistake. Virtual files have no size.

  String8 data = {0};
  Assert(fd >= 0);              // Something is seriously wrong if we can't access this file
  if (fd < 0) { return 0; }

  data.str = buffer;
  data.size = read(fd, buffer, 32);
  Assert(data.size > 0);        // Probably indicative of platform quirk
  if ( data.size <= 0) { return 0; } // Guard against underflow
  U8 last_char = (data.str[ data.size - 1]);
  if (last_char == '\n') { --data.size; }
  close(fd);

  U64 hugepage_count = 0;
  hugepage_count = u64_from_str8(data, 10);
  B32 enable_largepages = (hugepage_count > lnx_hugepage_count_min);

  return enable_largepages;
}

/* NOTE: The size seems to be consistent across Linux systems, it's configurable
  but the use case seems to be niche enough and the interface weird enough that
  most people won't do it.  The `/proc/meminfo` virtual file can be queried for
  hugepage size. */
internal U64
os_large_page_size(void)
{
  U64 size = (lnx_huge_page_use_1GB ? GB(1) : MB(2));
  return size;
}

internal void*
os_alloc_ring_buffer(U64 size, U64 *actual_size_out)
{
  Temp scratch = scratch_begin(0, 0);
  void* result = NULL;

  // Make fle descriptor
  String8 base = str8_lit("metagen_ring_xxxx");
  String8 filename_anonymous = push_str8_copy(scratch.arena, base);
  base16_from_data(filename_anonymous.str + filename_anonymous.size -4,
                   (U8*)&lnx_ring_buffers_created,
                   sizeof(lnx_ring_buffers_created));
  if (lnx_ring_buffers_created < lnx_ring_buffers_limit)
  {
    B32 use_huge = (size > os_large_page_size() && lnx_huge_page_enabled);
    U32 flag_huge1 = (use_huge ? MFD_HUGETLB : 0x0);
    U32 flag_huge2 = (use_huge ? (MAP_HUGETLB | MAP_HUGE_1GB)  : 0x0);
    U32 flag_prot = PROT_READ | PROT_WRITE;
    U32 flag_map_initial = (flag_huge2 | MAP_ANONYMOUS | MAP_SHARED | MAP_POPULATE);
    U32 flag_map_ring = (MAP_FIXED | MAP_SHARED);

    /* mmap circular buffer trick, create twice the buffer and double map it
       with a shared file descriptor.  Make sure to prevent mmap from changing
       location with MAP_FIXED */
    S32 fd = memfd_create((const char*)filename_anonymous.str, flag_huge1);
    Assert(fd != -1);
    /* NOTE(mallchad): This is your warnnig to remember to resize/truncate
       memory files before writing, this burned me like 3 times already */
    ftruncate(fd, size);

    result = mmap(NULL, 2* size, flag_prot, flag_map_initial, -1, 0);
    void* ring_head = mmap(result, size, flag_prot, flag_map_ring, fd, 0);
    void* ring_tail = mmap(result + size, size, flag_prot, flag_map_ring, fd, 0);
    Assert(ring_head != (void*)-1);
    Assert(ring_tail != (void*)-1);
    *actual_size_out = size*2;
    // Closing fd doesn't invalidate mapping
    close(fd);
  }
  scratch_end(scratch);
  return result;
}

internal void
os_free_ring_buffer(void *ring_buffer, U64 actual_size)
{
  munmap(ring_buffer, actual_size);
}

////////////////////////////////
//~ rjf: @os_hooks System Info (Implemented Per-OS)

/* NOTE: Cache the result if you don't want to re-trigger the lookup */
internal String8
os_machine_name(void){
  String8 result = {0};

  pthread_mutex_lock(&lnx_mutex);
  Temp scratch = scratch_begin(0, 0);

  // NOTE: Hostname spec caps at 253 ASCII chars its a problem if its larger
  U8* buffer = push_array(scratch.arena, U8, 256);
  B32 failure = gethostname((char*)buffer, 256);

  if (failure == 0)
  {
    result = push_str8_copy(lnx_perm_arena, str8_cstring((char*)buffer));
  }
  scratch_end(scratch);
  pthread_mutex_unlock(&lnx_mutex);

  return result;
}

/* Precomptued at init
 NOTE: This setting can actually be changed at runtime, its not a super
important thing but it might be a better user experience if they can fix/improve
it without restarting. */
internal U64
os_page_size(void)
{
  return lnx_page_size;
}

internal U64
os_allocation_granularity(void)
{
  /* On linux there is no equivalent of "dwAllocationGranularity" but you can
   set the page size only if you have root or similar privillages. You could see
   if there is a CAP privillage you can set for this. There are 2 huge page
   sizes and 1 normal page size however */
  return os_page_size();
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
  String8List result = {0};
  NotImplemented;               /* This doesn't appear to be used any longer */
  return result;
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
  return lnx_environment;
}

// TODO: Marked as old / review
internal U64
os_string_list_from_system_path(Arena *arena, OS_SystemPath path, String8List *out){
  U64 result = 0;

  switch (path){
    case OS_SystemPath_Binary:
    {
      local_persist B32 first = 1;
      local_persist String8 name = {0};

      // TODO(allen): let's just pre-compute this at init and skip the complexity
      pthread_mutex_lock(&lnx_mutex);
      if (first){
        Temp scratch = scratch_begin(&arena, 1);
        first = 0;

        // get self string
        B32 got_final_result = 0;
        U8 *buffer = 0;
        int size = 0;
        /* NOTE: PATH_MAX aparently lies about the max path size so its
           suggested to redo if it hits the max size */
        for (S64 cap = PATH_MAX, r = 0;
             r < 4;
             cap *= 2, r += 1){
          buffer = push_array_no_zero(scratch.arena, U8, cap);
          size = readlink("/proc/self/exe", (char*)buffer, cap);
          if (size < cap){
            got_final_result = 1;
            break;
          }
        }

        // save string
        if (got_final_result && size > 0){
          String8 full_name = str8(buffer, size);
          String8 name_chopped = str8_chop_last_slash(full_name);
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

/* NOTE: It was tempting to use 'pthread_setname_np' but that is bound to glibc
   and potentially the best compat would be free of glibc specific functions.

   Also aparently the Linux kernel enforces a 16 char thread name- the process
   name/cmdline can be arbitrarily long
   TODO: Rename cmdline to desired thread name*/
internal void
os_set_thread_name(String8 string)
{
  LNX_thread self = pthread_self();
  prctl(PR_SET_NAME, string.str);
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
  U32 access_flags = lnx_open_from_os_flags(flags);

  /* NOTE(mallchad): openat is supposedly meant to help prevent race conditions
     with file moving or possibly a better way to put it is it helps resist
     tampering with the referenced through relinks (assumption), ie symlink / mv .
     Hopefully its more robust- we can close the dirfd it's kernel managed
     now. The working directory can be changed but I don't know how best to
     utilize it so leaving it for now. */

  // String8 file_dir = str8_chop_last_slash(path);
  // S32 dir_fd = open(file_dir, O_PATH);
  // S32 fd = openat(fle_dir, path.str, access_flags);

  // Create file on write if doesn't exist, mode: rw-rw----
  S32 fd = openat(AT_FDCWD, (char*)path.str, access_flags, 0660);
  // close(dirfd);

  // No Error
  if (fd != -1)
  {
    *file.u64 = fd;
  }
  return file;
}

internal void
os_file_close(OS_Handle file)
{
  if(os_handle_match(file, os_handle_zero())) { return; }
  S32 fd = *file.u64;
  close(fd);
}

/* NOTE: You can just use file maps on Linux and map the whole file because it
  doesn't commit it to physical memory immediately and so shouldn't cost anything over file
  mappings. Use MADV prefetch hints for performance.
  NOTE: Aparently there is a race condition in relation to `stat` and
  `lstat` so using fstat instead
  https://cwe.mitre.org/data/definitions/367.html */
internal U64
os_file_read(OS_Handle file, Rng1U64 rng, void *out_data)
{
  S32 fd = *file.u64;
  struct stat file_info;
  fstat(fd, &file_info);
  U64 filesize = file_info.st_size;

  // Make sure not to read more than the size of the file
  Rng1U64 clamped = r1u64(ClampTop(rng.min, filesize), ClampTop(rng.max, filesize));
  U64 read_amount = clamped.max - clamped.min;
  lseek(fd, clamped.min, SEEK_SET);
  S64 read_bytes = read(fd, out_data, read_amount);

  // Return 0 instead of -1 on error
  return (read_bytes > 0 ? read_bytes : 0) ;
}

internal void
os_file_write(OS_Handle file, Rng1U64 rng, void *data)
{
  // Zero Valid Argument, often but not always STDIN
  if ((*file.u64 < 0) || (data == NULL) || (rng.min - rng.max == 0)) { return; }
  S32 fd = lnx_fd_from_handle(file);
  LNX_fstat file_info;
  fstat(fd, &file_info);
  U64 filesize = file_info.st_size;
  U64 write_size = rng.max - rng.min;

  AssertAlways(write_size > 0);
  lseek(fd, rng.min, SEEK_SET);
  // Expands file size if written off file end
  U64 bytes_written = write(fd, data, write_size);
  Assert("Zero or less written bytes is usually inticative of a bug" && bytes_written > 0);
}

internal B32
os_file_set_times(OS_Handle file, DateTime time)
{
  S32 fd = *file.u64;
  LNX_timeval access_and_modification[2];
  DenseTime tmp = dense_time_from_date_time(time);
  lnx_timeval_from_dense_time(access_and_modification, &tmp);
  access_and_modification[1] = access_and_modification[0];
  B32 error = futimes(fd, access_and_modification);

  return (error != -1);
}

internal FileProperties
os_properties_from_file(OS_Handle file)
{
  FileProperties result = {0};
  S32 fd = *file.u64;
  LNX_fstat props = {0};
  B32 error = fstat(fd, &props);

  if (error == 0)
  {
    lnx_file_properties_from_stat(&result, &props);
  }
  return result;
}

internal FileProperties
os_properties_from_file_path(String8 path)
{
  FileProperties result = {0};
  LNX_fstat props = {0};
  B32 error = stat((char*)path.str, &props);
  if (error == 0)
  {
    lnx_file_properties_from_stat(&result, &props);
  }
  return result;
}

internal OS_FileID
os_id_from_file(OS_Handle file)
{
  OS_FileID result = {0};
  U32 fd = *file.u64;
  LNX_fstat props = {0};
  B32 error = fstat(fd, &props);

  if (error == 0)
  {
    result.v[0] = props.st_dev;
    result.v[1] = props.st_ino;
    result.v[2] = 0;
  }
  return result;
}

// TODO: Marked as old / review
internal B32
os_delete_file_at_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  String8 name_copy = push_str8_copy(scratch.arena, path);
  if (remove((char*)name_copy.str) != -1){
    result = 1;
  }
  scratch_end(scratch);
  return(result);
}

internal B32
os_copy_file_path(String8 dst, String8 src)
{
  S32 source_fd = open((char*)src.str, 0x0, O_RDONLY);
  S32 dest_fd = creat((char*)dst.str, O_WRONLY);
  LNX_fstat props = {0};

  S32 filesize = 0;
  S32 bytes_written = 0;
  B32 success = 0;

  fstat(source_fd, &props);
  filesize = props.st_size;

  if (source_fd == 0 && dest_fd == 0)
  {
    bytes_written = copy_file_range(source_fd, NULL, dest_fd, NULL, filesize, 0x0);
    success = (bytes_written == filesize);
  }
  close(source_fd);
  close(dest_fd);
  return success;
}

internal String8
os_full_path_from_path(Arena *arena, String8 path)
{
  String8 tmp = {0};
  /* NOTE: PATH_MAX aparently lies about the max path size so its suggested to
     redo if it hits the max size */
  char buffer[PATH_MAX+10];
  MemoryZeroArray(buffer);
  char* success = realpath((char*)path.str, buffer);
  if (success)
  {
    tmp = str8_cstring(buffer);
  }
  return (push_str8_copy(lnx_perm_arena, tmp));
}

internal B32
os_file_path_exists(String8 path)
{
  LNX_fstat _stub;
  B32 exists = (0 == stat((char*)path.str, &_stub));
  return exists;
}

//- rjf: file maps

/* Does not map a view of the file into memory until a view is opened
 'OS_AccessFlags' is not relevant here */
internal OS_Handle
os_file_map_open(OS_AccessFlags flags, OS_Handle file)
{
  OS_Handle result = {0};

  if (*file.u64 == 0) { return result; }
  LNX_Entity* entity = lnx_alloc_entity(LNX_EntityKind_MemoryMap);
  S32 fd = *file.u64;
  entity->map.fd = fd;
  *result.u64 = IntFromPtr(entity);

  return result;
}

/* NOTE(mallchad): munmap needs sizing data and I didn't know how to impliment
   it without introducing a bunch of unnecesary complexity or changing the API,
   hopefully it's okay but it doesn't really qualify as "threading entities"
   anymore :/ */
internal void
os_file_map_close(OS_Handle map)
{
  LNX_Entity* entity = lnx_entity_from_handle(map, LNX_EntityKind_MemoryMap);
  msync(entity->map.data, entity->map.size, MS_SYNC);
  B32 failure = munmap(entity->map.data, entity->map.size);
  /* NOTE: It shouldn't be that important if filemap fails but it ideally shouldn't
     happen particularly when dealing with gigabytes of memory. */
  Assert(failure == 0);
  lnx_free_entity(entity);
}

/* NOTE(mallcahd): It looks this was supposed to a way to make a sub-portion of
  a file, presumably to make very large files reasonably sensible to manage. But
  right now the usage seems to just read from 0 starting point always, so I'm
  just leaving it that way for now. Both the win32 and linux backend will break
  if you try to use this differently for some reason at time of writing
  [2024-07-11 Thu 17:22]

  If you would like to complete this function to work the way outlined above,
  then take the first page-boundary aligned boundary before offset.
  NOTE: You can try using MADV prefetch hints for performance. */
internal void *
os_file_map_view_open(OS_Handle map, OS_AccessFlags flags, Rng1U64 range)
{
  LNX_Entity* entity = lnx_entity_from_handle(map, LNX_EntityKind_MemoryMap);
  if (entity == 0) { return NULL; }

  S32 fd = entity->map.fd;
  struct stat file_info;
  fstat(fd, &file_info);
  U64 filesize = file_info.st_size;
  U64 range_size = range.max - range.min;
  /* TODO(mallchad): I can't figure out how to get the exact huge page size, the
     mmap offset will just error if its not exact
  B32 use_huge = (size > os_large_page_size() && lnx_huge_page_enabled);
  F32 huge_threshold = 0.5f;
  F32 huge_use_1GB = (size > (1e9f * huge_threshold) &&use_huge);
  U64 page_size = (huge_use_1GB ? huge_size :
                   use_huge ?lnx_huge_sze : lnx_page_size );
  U64 page_flag = huge_use_1GB ? MAP_HUGE_1GB : MAP_HUGE_2MB;
  */
  U64 page_size = lnx_page_size;
  U32 page_flag = 0x0;
  U32 map_flags = page_flag | MAP_POPULATE;
  map_flags |= (flags & OS_AccessFlag_ShareRead ? MAP_SHARED : MAP_PRIVATE);
  map_flags |= (flags & OS_AccessFlag_ShareWrite ? MAP_SHARED : MAP_PRIVATE);

  U32 prot_flags = lnx_prot_from_os_flags(flags);
  U64 aligned_offset = AlignDownPow2(range.min, lnx_page_size);
  U64 view_start_from_offset = (aligned_offset % lnx_page_size);
  U64 map_size = (view_start_from_offset + range_size);

  void *address = mmap(NULL, map_size, prot_flags, map_flags, fd, aligned_offset);
  entity->map.data = address;
  entity->map.size = map_size;
  void* result = (address + view_start_from_offset);
  // Return NULL on error
  if (address == (void*)-1) { result = NULL; }

  return result;
}

internal void
os_file_map_view_close(OS_Handle map, void *ptr)
{
  LNX_Entity* entity = lnx_entity_from_handle(map, LNX_EntityKind_MemoryMap);
  AssertAlways( entity->map.data && (entity->map.data != (void*)-1) );
  /* NOTE: Make sure contents are synced with OS on the off chance the backing
     file isn't POSIX compliant. Use MS_ASYNC if you want performance. */
  msync(entity->map.data, entity->map.size, MS_SYNC);
  munmap(entity->map.data, entity->map.size);
}

//- rjf: directory iteration

internal OS_FileIter *
os_file_iter_begin(Arena *arena, String8 path, OS_FileIterFlags flags)
{
  OS_FileIter* result = push_array(arena, OS_FileIter, 1);
  result->flags = flags;
  // String8 tmp = push_str8_copy(arena, path);
  LNX_dir* directory = opendir((char*)path.str);
  LNX_fd dir_fd = open((char*)path.str, 0x0, 0x0);
  LNX_file_iter* out_iter = (LNX_file_iter*)result->memory;
  out_iter->dir_stream = directory;
  out_iter->dir_fd = dir_fd;

  // Never fail, just let the iterator return false.
  return result;
}

 /* NOTE(mallchad): I have no idea what the return is for so it always returns true
  unless the iterator is done */
internal B32
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  MemoryZeroStruct(info_out);
  if (iter == NULL) { return 0; }

  LNX_dir* directory = NULL;
  LNX_dir_entry* file = NULL;
  LNX_fd working_path = -1;
  FileProperties props = {0};
  LNX_fstat stats = {0};
  B32 no_file = 0;

  LNX_file_iter* iter_data = (LNX_file_iter*)iter->memory;
  directory = iter_data->dir_stream;
  working_path = iter_data->dir_fd;
  if (directory == NULL || working_path == -1) { return 0; }

  // Should hopefully never infinite loop because readdir reaches NULL quickly
  for (;;)
  {
    file = readdir(directory);
    no_file = (file == NULL);
    if (no_file) { iter->flags |= OS_FileIterFlag_Done; return 0; }

    if (iter->flags & OS_FileIterFlag_SkipFiles && file->d_type == DT_REG ) { continue; }
    if (iter->flags & OS_FileIterFlag_SkipFolders && file->d_type == DT_DIR ) { continue; }
    // TODO: Aparently this part of the API is in an indeterminate state. So hardcoding the behavior.
    // if (iter->flags & OS_FileIterFlag_SkipHiddenFiles && file->d_name[0] == '.'  ) { continue; }
    if (file->d_name[0] == '.') { continue; }
    break;
  }

  LNX_fd fd = openat(working_path, file->d_name, 0x0, 0x0);
  Assert(fd != -1); if (fd == -1) { return 0; }
  S32 stats_err = fstat(fd, &stats);
  Assert(stats_err == 0); if (stats_err != 0) { return 0; }
  close(fd);

  info_out->name = push_str8_copy(arena, str8_cstring(file->d_name));
  lnx_file_properties_from_stat(&info_out->props, &stats);

  return 1;
}

internal void
os_file_iter_end(OS_FileIter *iter)
{
  LNX_file_iter* iter_info = (LNX_file_iter*)iter->memory;
  int stream_failure = closedir(iter_info->dir_stream);
  int fd_failure = close(iter_info->dir_fd);
  // Could be disabled
  Assert(stream_failure == 0 && fd_failure == 0);
}

//- rjf: directory creation

internal B32
os_make_directory(String8 path)
{
  B32 result = 0;
  mkdir((char*)path.str, 0770);
  DIR* dirfd = opendir((char*)path.str);
  result = (ENOENT != errno);
  closedir(dirfd);

  return(result);
}

////////////////////////////////
//~ rjf: @os_hooks Shared Memory (Implemented Per-OS)

internal OS_Handle
os_shared_memory_alloc(U64 size, String8 name)
{
  OS_Handle result = {0};
  LNX_Entity* entity = lnx_alloc_entity(LNX_EntityKind_MemoryMap);
  // Create, reuse if already open, mode: rw-rw----
  S32 fd = shm_open((char*)name.str, O_RDWR | O_CREAT, 0660);
  /* NOTE(mallchad): This is your warnnig to remember to resize/truncate
     memory files before writing, this burned me like 3 times already */
  ftruncate(fd, size);
  Assert("Failed to alloc shared memory" && fd != -1);

  B32 use_huge = (size > os_large_page_size() && lnx_huge_page_enabled);
  U32 flag_huge = (use_huge ? MAP_HUGETLB : 0x0);
  U32 flag_prot = PROT_READ | PROT_WRITE;
  U32 map_flags = MAP_SHARED | flag_huge | MAP_POPULATE;
  void* mapping = mmap(NULL, size, flag_prot, map_flags, fd, 0);
  Assert("Failed map memory for shared memory" && mapping != MAP_FAILED);

  entity->map.data = mapping;
  entity->map.size = size;
  entity->map.fd = fd;
  entity->map.shm_name = push_str8_copy( lnx_perm_arena, name );
  result = lnx_handle_from_entity(entity);
  return result;
}

internal OS_Handle
os_shared_memory_open(String8 name)
{
  OS_Handle result = {0};
  LNX_Entity* entity = lnx_alloc_entity(LNX_EntityKind_MemoryMap);
  LNX_fd fd = shm_open((char*)name.str, O_RDWR, 0660 );
  Assert(fd != -1);

  entity->map.fd = fd;
  entity->map.shm_name = push_str8_copy( lnx_perm_arena, name );
  *result.u64 = IntFromPtr(entity);

  return result;
}

internal void
os_shared_memory_close(OS_Handle handle)
{
  LNX_Entity* entity = lnx_entity_from_handle(handle, LNX_EntityKind_MemoryMap);
  shm_unlink( (char*)(entity->map.shm_name.str) );
}

internal void *
os_shared_memory_view_open(OS_Handle handle, Rng1U64 range)
{
  void* result = NULL;
  LNX_Entity* entity = lnx_entity_from_handle(handle, LNX_EntityKind_MemoryMap);
  LNX_fd fd = entity->map.fd;

  B32 use_huge = (range.max > os_large_page_size() && lnx_huge_page_enabled);
  U32 flag_huge = (use_huge ? MAP_HUGETLB : 0x0);
  U32 flag_prot = PROT_READ | PROT_WRITE;
  U32 map_flags = MAP_SHARED | flag_huge | MAP_POPULATE;
  void* mapping = mmap(NULL, range.max, flag_prot, map_flags, fd, 0);
  Assert("Failed map memory for shared memory" && mapping != MAP_FAILED);

  result = mapping + range.min; // Get the offset to the map opening
  entity->map.data = mapping;
  entity->map.size = range.max;
  if (result == (void*)-1) { result = NULL; }

  return result;
}

internal void
os_shared_memory_view_close(OS_Handle handle, void *ptr)
{
  LNX_Entity entity = *lnx_entity_from_handle(handle, LNX_EntityKind_MemoryMap);
  munmap(entity.map.data, entity.map.size);
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
  // NOTE: pedantic is it acutally worth it to use CLOCK_MONOTONIC_RAW?
  // CLOCK_MONOTONIC is to occasional adjtime adjustments, the max error appears
  // to be large.
  // https://man7.org/linux/man-pages/man3/adjtime.3.html
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  U64 result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
  return(result);
}

internal void
os_sleep_milliseconds(U32 msec)
{
  LNX_timespec duration = {0};
  duration.tv_nsec = (msec* Million(1));
  nanosleep(&duration, NULL);
}

////////////////////////////////
//~ rjf: @os_hooks Child Processes (Implemented Per-OS)

/* NOTE: A terminal can be opened for any process but there is no default
 terminal emulator on Linux, so instead you Have to cycle through a bunch of
 common ones (there aren't that many). There is the XDG desktop specification
 but that's a little bit of a chore to parse and its not even always present.

The environment is inhereited by default but is easy to change in the future
with an execl variant. */
internal B32
os_launch_process(OS_LaunchOptions *options, OS_Handle *handle_out)
{
  Temp scratch = scratch_begin(0, 0);
  U8* buffer = (void*)mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  B32* success_shared = (B32*)buffer;
  S32* child_pid = (S32*)buffer+4;
  *success_shared = 1;
  *child_pid = 0;
  // TODO(allen): I want to redo this API before I bother implementing it here
  // TODO(mallchad): Is this comment still relevant? gut says yes.
  String8List cmdline_list = options->cmd_line;
  char** cmdline = (char**)push_array(scratch.arena, char*, cmdline_list.node_count + 4);
  String8Node* x_node = cmdline_list.first;
  for (int i=0; i<cmdline_list.node_count; ++i)
  {
    cmdline[i] = (char*)x_node->string.str;
    x_node = x_node->next;
  }

  if (options->inherit_env) { NotImplemented; };
  if (options->consoleless == 0) { NotImplemented; };
  S32 pid = 0;
  pid = fork();
  // Child
  if (pid)
  {
    *child_pid = pid;
    if (child_pid > 0) { *(handle_out->u64) = (U64)child_pid; }
    execv(*cmdline, cmdline);
    *success_shared = 0;
    exit(0);
  } // Parent
  else { waitpid(*child_pid, NULL, 0x0); } // TODO: Heck? Can't figure out how to wait on 'exec'
  os_sleep_milliseconds(20);              // Getto fix for no-wait issue
  B32 success = *success_shared;
  munmap(success_shared, 4096);
  scratch_end(scratch);
  return success;
}

internal B32
os_process_wait(OS_Handle handle, U64 endt_us)
{
  NotImplemented;
  return 0;
}

internal void
os_process_release_handle(OS_Handle handle)
{
  NotImplemented;
}

////////////////////////////////
//~ rjf: @os_hooks Threads (Implemented Per-OS)

// TODO: Marked as old / review
// TODO(mallchad): Isn't one of *params arg useuless? Should it be deleted?
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

internal B32
os_thread_wait(OS_Handle handle, U64 endt_us)
{
  // TODO: Supposed to be thread sleep?
  LNX_Entity* entity = lnx_entity_from_handle(handle, LNX_EntityKind_Thread);
  NotImplemented;
  return 0;
}

// TODO: Marked as old / review
internal void
os_release_thread_handle(OS_Handle thread){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(thread.u64[0]);
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
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.u64[0]);
  pthread_mutex_destroy(&entity->mutex);
  lnx_free_entity(entity);
}

internal void
os_mutex_take_(OS_Handle mutex){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.u64[0]);
  S32 error = 0;
  while(error = pthread_mutex_lock(&entity->mutex)) { }
}

internal void
os_mutex_drop_(OS_Handle mutex){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(mutex.u64[0]);
  pthread_mutex_unlock(&entity->mutex);
}

//- rjf: reader/writer mutexes

internal OS_Handle
os_rw_mutex_alloc(void)
{
  OS_Handle result = {0};
  LNX_rwlock_attr attr;
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
  LNX_rwlock rwlock = {0};
  int pthread_result = pthread_rwlock_init(&rwlock, &attr);
  // This can be cleaned up now.
  pthread_rwlockattr_destroy(&attr);

  Assert(pthread_result == 0);
  LNX_Entity *entity = lnx_alloc_entity(LNX_EntityKind_Rwlock);
  entity->rwlock = rwlock;
  *result.u64 = IntFromPtr(entity);
  return result;
}

internal void
os_rw_mutex_release(OS_Handle rw_mutex)
{
  LNX_Entity* entity = lnx_entity_from_handle(rw_mutex, LNX_EntityKind_Rwlock);
  pthread_rwlock_destroy(&entity->rwlock);
}

internal void
os_rw_mutex_take_r_(OS_Handle mutex)
{
  // Is blocking varient
  LNX_Entity* entity = lnx_entity_from_handle(mutex, LNX_EntityKind_Rwlock);
  pthread_rwlock_rdlock(&entity->rwlock);
}

internal void
os_rw_mutex_drop_r_(OS_Handle mutex)
{
  // NOTE: Aparently it results in undefined behaviour if there is no pre-existing lock
  // https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_rwlock_unlock.html
  LNX_Entity* entity = lnx_entity_from_handle(mutex, LNX_EntityKind_Rwlock);
  pthread_rwlock_unlock(&entity->rwlock);
}

internal void
os_rw_mutex_take_w_(OS_Handle mutex)
{
  LNX_Entity* entity = lnx_entity_from_handle(mutex, LNX_EntityKind_Rwlock);
  pthread_rwlock_wrlock(&(entity->rwlock));
}

internal void
os_rw_mutex_drop_w_(OS_Handle mutex)
{
  LNX_Entity* entity = lnx_entity_from_handle(mutex, LNX_EntityKind_Rwlock);
  pthread_rwlock_unlock(&(entity->rwlock));
}

//- rjf: condition variables

internal OS_Handle
os_condition_variable_alloc(void){
  // entity
  LNX_Entity *entity = lnx_alloc_entity(LNX_EntityKind_ConditionVariable);

  // pthread
  pthread_condattr_t attr_cond;
  pthread_mutexattr_t attr_mutex;

  // Make sure condition uses CPU clock time
  pthread_condattr_setclock(&attr_cond, CLOCK_MONOTONIC_RAW);
  pthread_condattr_setpshared(&attr_cond, PTHREAD_PROCESS_PRIVATE);
  pthread_mutexattr_init(&attr_mutex);
  pthread_mutexattr_settype(&attr_mutex, PTHREAD_MUTEX_RECURSIVE);
  int cond_result = pthread_cond_init(&entity->condition_variable.cond, &attr_cond);
  int mutex_result = pthread_mutex_init(&entity->condition_variable.mutex, &attr_mutex);

  pthread_condattr_destroy(&attr_cond);
  pthread_mutexattr_destroy(&attr_mutex);

  Assert(cond_result == 0);

  // cast to opaque handle
  OS_Handle result = {IntFromPtr(entity)};
  return(result);
}

internal void
os_condition_variable_release(OS_Handle cv){
  LNX_Entity *entity = (LNX_Entity*)PtrFromInt(cv.u64[0]);
  pthread_cond_destroy(&entity->condition_variable.cond);
  pthread_mutex_destroy(&entity->condition_variable.mutex);
  lnx_free_entity(entity);
}

internal B32
os_condition_variable_wait_(OS_Handle cv, OS_Handle mutex, U64 endt_us){
  B32 result = 0;
  LNX_Entity *entity_cond = lnx_entity_from_handle(cv, LNX_EntityKind_ConditionVariable);
  LNX_Entity *entity_mutex = lnx_entity_from_handle(mutex, LNX_EntityKind_Mutex);
  LNX_timespec timeout_stamp = {0};
  timeout_stamp.tv_sec = (endt_us / Million(1));
  timeout_stamp.tv_nsec = ((endt_us % Million(1)) * Thousand(1));
  // TODO(mallchad): is endt supposed to be "end time?"

  // The timeout is received as a system clock timespec of when to stop waiting
  B32 wait_result = pthread_cond_timedwait(&entity_cond->condition_variable.cond,
                                           &entity_mutex->mutex,
                                           &timeout_stamp);
  result = (wait_result == 0);
  return(result);
}

internal B32
os_condition_variable_wait_rw_r_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  // TODO(rjf): because pthread does not supply cv/rw natively, I had to hack
  // this together, but this would probably just be a lot better if we just
  // implemented the primitives ourselves with e.g. futexes
  //
  if(os_handle_match(cv, os_handle_zero())) { return 0; }
  if(os_handle_match(mutex_rw, os_handle_zero())) { return 0; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  LNX_Entity *rw_mutex_entity = (LNX_Entity *)mutex_rw.u64[0];
  struct timespec endt_timespec;
  endt_timespec.tv_sec = endt_us/Million(1);
  endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us/Million(1))*Million(1));
  B32 result = 0;
  for(;;)
  {
    pthread_mutex_lock(&cv_entity->condition_variable.mutex);
    int wait_result = pthread_cond_timedwait(&cv_entity->condition_variable.cond,
                                             &cv_entity->condition_variable.mutex,
                                             &endt_timespec);
    if(wait_result != ETIMEDOUT)
    {
      pthread_rwlock_rdlock(&rw_mutex_entity->rwlock);
      pthread_mutex_unlock(&cv_entity->condition_variable.mutex);
      result = 1;
      break;
    }
    pthread_mutex_unlock(&cv_entity->condition_variable.mutex);
    if(wait_result == ETIMEDOUT)
    {
      break;
    }
  }
  return result;
}

internal B32
os_condition_variable_wait_rw_w_(OS_Handle cv, OS_Handle mutex_rw, U64 endt_us)
{
  // TODO(rjf): because pthread does not supply cv/rw natively, I had to hack
  // this together, but this would probably just be a lot better if we just
  // implemented the primitives ourselves with e.g. futexes
  //
  if(os_handle_match(cv, os_handle_zero())) { return 0; }
  if(os_handle_match(mutex_rw, os_handle_zero())) { return 0; }
  LNX_Entity *cv_entity = (LNX_Entity *)cv.u64[0];
  LNX_Entity *rw_mutex_entity = (LNX_Entity *)mutex_rw.u64[0];
  struct timespec endt_timespec;
  endt_timespec.tv_sec = endt_us/Million(1);
  endt_timespec.tv_nsec = Thousand(1) * (endt_us - (endt_us/Million(1))*Million(1));
  B32 result = 0;
  for(;;)
  {
    pthread_mutex_lock(&cv_entity->condition_variable.mutex);
    int wait_result = pthread_cond_timedwait(&cv_entity->condition_variable.cond,
                                             &cv_entity->condition_variable.mutex,
                                             &endt_timespec);
    if(wait_result != ETIMEDOUT)
    {
      pthread_rwlock_wrlock(&rw_mutex_entity->rwlock);
      pthread_mutex_unlock(&cv_entity->condition_variable.mutex);
      result = 1;
      break;
    }
    pthread_mutex_unlock(&cv_entity->condition_variable.mutex);
    if(wait_result == ETIMEDOUT)
    {
      break;
    }
  }
  return result;
}

internal void
os_condition_variable_signal_(OS_Handle cv)
{
  LNX_Entity *entity = lnx_entity_from_handle(cv, LNX_EntityKind_ConditionVariable);
  pthread_cond_signal(&entity->condition_variable.cond);
}

internal void
os_condition_variable_broadcast_(OS_Handle cv)
{
  LNX_Entity *entity = lnx_entity_from_handle(cv, LNX_EntityKind_ConditionVariable);
  pthread_cond_broadcast(&entity->condition_variable.cond);
}

//- rjf: cross-process semaphores

internal OS_Handle
os_semaphore_alloc(U32 initial_count, U32 max_count, String8 name)
{
  OS_Handle result = {0};

  // Create the named semaphore
  // create | reuse pre-existing, rw-rw----
  sem_t* semaphore = sem_open((char*)name.str, O_RDWR | O_CREAT,  0660, initial_count);
  if (semaphore != SEM_FAILED)
  {
    LNX_Entity* entity = lnx_alloc_entity(LNX_EntityKind_Semaphore);
    entity->semaphore.handle = semaphore;
    entity->semaphore.max_value = max_count;
    result = lnx_handle_from_entity(entity);
  }
  return result;
}

internal void
os_semaphore_release(OS_Handle semaphore)
{
  os_semaphore_close(semaphore);
}

internal OS_Handle
os_semaphore_open(String8 name)
{
  OS_Handle result = {0};
  LNX_Entity* handle = lnx_alloc_entity(LNX_EntityKind_Semaphore);
  LNX_semaphore* semaphore;
  semaphore = sem_open((char*)name.str, 0600);
  handle->semaphore.handle = semaphore;
  Assert("Failed to open POSIX semaphore." || semaphore != SEM_FAILED);

  result = lnx_handle_from_entity(handle);
  return result;
}

internal void
os_semaphore_close(OS_Handle semaphore)
{
  LNX_Entity* entity = lnx_entity_from_handle(semaphore, LNX_EntityKind_Semaphore);
  LNX_semaphore* _semaphore = entity->semaphore.handle;
  sem_close(_semaphore);
}

internal B32
os_semaphore_take(OS_Handle semaphore, U64 endt_us)
{
  U32 wait_result = 0;
  LNX_timespec wait_until = lnx_now_precision_timespec();
  wait_until.tv_nsec += endt_us;

  LNX_Entity* entity = lnx_entity_from_handle(semaphore, LNX_EntityKind_Semaphore);
  LNX_semaphore* _semaphore = entity->semaphore.handle;
  // We have to impliment max_count ourselves
  S32 current_value = 0;
  sem_getvalue(_semaphore, &current_value);
  if (entity->semaphore.max_value > current_value)
  {
    sem_timedwait(_semaphore, &wait_until);
  }
  return (wait_result != -1);
}

internal void
os_semaphore_drop(OS_Handle semaphore)
{
  LNX_Entity* entity = lnx_entity_from_handle(semaphore, LNX_EntityKind_Semaphore);
  sem_t* _semaphore = entity->semaphore.handle;
  sem_post(_semaphore);
}

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

// TODO: Marked as old / review
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

// TODO: Marked as old / review
internal VoidProc *
os_library_load_proc(OS_Handle lib, String8 name)
{
  Temp scratch = scratch_begin(0, 0);
  void *so = (void *)lib.u64[0];
  char *name_cstr = (char *)push_str8_copy(scratch.arena, name).str;
  VoidProc *proc = (VoidProc *)dlsym(so, name_cstr);
  scratch_end(scratch);
  return proc;
}

internal void
os_library_close(OS_Handle lib)
{
  void *so = (void *)lib.u64[0];
  dlclose(so);
}

////////////////////////////////
//~ rjf: @os_hooks Dynamically-Loaded Libraries (Implemented Per-OS)

// TODO: Marked as old / review
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
  OS_Guid result = {0};
  LNX_uuid tmp;;
  MemoryZeroArray(tmp);

  uuid_generate(tmp);
  MemoryCopy(&result.data1, 0+ tmp, 4);
  MemoryCopy(&result.data2, 4+ tmp, 2);
  MemoryCopy(&result.data3, 6+ tmp, 2);
  MemoryCopy(&result.data4, 8+ tmp, 8);

  return result;
}
int
lnx_entry_point(int argc, char** argv)
{
  return 1;
}
int
main(int argc, char** argv)
{
  main_thread_base_entry_point(entry_point, argv, (U64)argc);
}
