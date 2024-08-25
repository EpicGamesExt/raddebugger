
// From Core
#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "rdi_make/rdi_make_local.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "rdi_make/rdi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"

// Tests only

internal int total = 0;
internal int passes = 0;
internal int failed = 0;
internal int manual_checks = 0;

/*
  ASNI Color Codes
  Default - \x1b[39m
  Green - \x1b[32m
  Red - \x1b[31m
*/

void
test( B32 passed, char* description )
{
  static B32 alternate_color = 1;
  char* alternating = (alternate_color ? "\x1b[38;5;180m" : "\x1b[0m" );
  alternate_color = !alternate_color;
  ++total;
  if (passed)
  {
    ++passes;
    printf( "\x1b[1mTest \x1b[32mPassed\x1b[39m |\x1b[0m %s%s\x1b[0m\n", alternating, description );
  }
  else
  {
    ++failed;
    printf( "\x1b[1mTest \x1b[31mFailed\x1b[39m |\x1b[0m %s%s\x1b[0m\n", alternating, description );
  }
}

#define MANUAL_CHECKF(fmt, ...) printf( "\x1b[1mManual Check| \x1b[0m" fmt "\n" , ##__VA_ARGS__ ); \
  ++manual_checks

void
SECTION( char* label )
{
  printf( "\n\x1b[36;1m[%s]\x1b[0m\n", label );
}

typedef struct Dummy_Payload Dummy_Payload;
struct Dummy_Payload
{
  OS_Handle mutex;
  U32* last_lock;
};

void
dummy_routine( void* restrict payload )
{
  Arena* g_arena = arena_alloc();
  OS_Handle process = {0};
  String8List process_command = {0};
  OS_LaunchOptions process_config = {0};
  if (OS_WINDOWS)
  {
    // Untested
    str8_list_push( g_arena, &process_command, str8_lit("C:/Windows/cmd.exe") );
  }
  else
  {
    str8_list_push( g_arena, &process_command, str8_lit("/usr/bin/echo") );
    str8_list_push( g_arena, &process_command, str8_lit("Make me a sandwich!") );
  }
  process_config.cmd_line = process_command;
  /* res = os_launch_process( &process_config, &process ); test( res, "os_launch_process" ); */
  struct Dummy_Payload* _payload = (Dummy_Payload*)payload;
  printf( "2nd Thread Locking\n" );
  os_mutex_take_( _payload->mutex );
  *(_payload->last_lock) = 2;
  printf( "2nd Thread Unlocked\n" );
  os_mutex_drop_( _payload->mutex );
}

typedef struct Share Share;
struct Share
{
  OS_Handle rwlock;
  U32 rwlock_last;
};

void
rwlock_routine( void* restrict payload )
{
  Share* g_share_data = payload;
  // Should be locked by prexisting writer lock
  os_rw_mutex_take_r( g_share_data->rwlock );
  g_share_data->rwlock_last += 1;
  os_rw_mutex_drop_r( g_share_data->rwlock );
}

/* NOTE(mallchad): These tests were built with testing the Linux layer in mind,
   it was supposed to be cross-platform, and it may still work, but the API is
   not friendly to error checking, so treat these files a scratchpad than rather
   than gospel.
*/
int main()
{
  srand( time(NULL) );
  U64 seed = rand();
  printf( "Setting seed to random number: %d\n", seed );

  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  os_init(); test( 1, "os_init" );
  Arena* g_arena = arena_alloc();
  U8* g_shared_base = NULL;
  U8* g_shared = NULL;
  OS_Handle g_shared_shm = {0};
  String8 g_shared_name = str8_lit("rad_test_global");
  g_shared_shm = os_shared_memory_alloc( MB(100), g_shared_name );
  g_shared = os_shared_memory_view_open( g_shared_shm, rng_1u64(0, MB(100)) );
  g_shared_base = g_shared;
  AssertAlways( g_shared != NULL );

  B32 res = 0;
  U64* g_buffer[4096];
  Share* g_share_data = (Share*)g_shared; g_shared += sizeof(Share);

  SECTION( "OS Memory Management" );
  // test various sizes of mmap because failure might be size dependant
  void* res_reserve = NULL;
  void* error_ptr = (void*)-1;
  res_reserve = os_reserve(1); test( res_reserve != error_ptr, "os_reserve 1" );
  res_reserve = os_reserve(3000); test( res_reserve != error_ptr, "os_reserve 3000" );
  res_reserve = os_reserve(4096); test( res_reserve != error_ptr, "os_reserve 4096" );
  res_reserve = os_reserve(4097); test( res_reserve != error_ptr, "os_reserve 4097" );
  res_reserve = os_reserve(200400); test( res_reserve != error_ptr, "os_reserve 200400" );
  res_reserve = os_reserve(1000200); test( res_reserve != error_ptr, "os_reserve 1000200" );
  res_reserve = os_reserve(4000800); test( res_reserve != error_ptr, "os_reserve 4000800" );

  B32 res_commit = 0;
  void* commit_buffer = os_reserve(4096);
  if (res_reserve != error_ptr)
  {
    res_commit = os_commit( commit_buffer, 4096); test( res_commit, "os_commit" );
  }
  res = 0;
  if (res_commit)
  {
    // Sorry, all 3 will crash on failure.
    MemoryZero( commit_buffer, 4096 ); test( 1, "os_commit is writable" );
    if (res_commit)
    {
      // Not easily testable
      os_decommit( commit_buffer, 4096 ); test( 1, "os_decommit" );
      os_release( commit_buffer, 4096 ); test( 1, "os_release" );
    }
  }

  res_reserve = os_reserve_large(4096); test( res_reserve != error_ptr, "os_reserve_large" );
  test( os_large_pages_enabled(), "Large Pages Enabled Locally" );
  res = os_commit_large( res_reserve, 4096 ); test( res, "os_commit_large" );

  B32 large_page_on = os_large_pages_enabled();
  res = os_set_large_pages( !large_page_on); test( res, "os_set_large_pages toggle");

  U64 large_size = os_large_page_size(); test( large_size > 0 && large_size < GB(512),
                                               "os_large_page_size" );
  void* res_ring = NULL;
  res_ring = os_alloc_ring_buffer( 4096, (U64*)g_buffer ); test( res_ring != error_ptr,
                                                              "os_alloc_ring_buffer" );
  /* Try to write over the buffer boundary, should write to the start of buffer
     if allocated correcctly */
  MemorySet( res_ring+2048, (char)seed, 4000 );
  test( MemoryCompare( res_ring, res_ring+4096, 128) == 0,
        "os_alloc_ring_buffer write over boundary" );

  // No easy way to check if it suceeded
  os_free_ring_buffer( res_ring, 4096 ); test( 1, "os_free_ring_buffer" );
  MANUAL_CHECKF( "os_page_size output: %lu", os_page_size() );
  MANUAL_CHECKF( "os_large_page_size output: %lu", os_large_page_size() );

  // Shared memory tests
  String8 shm_name = str8_lit( "rad_test_shm" );
  OS_Handle shm = {0};
  OS_Handle shm_same = {0};
  void* shm_buf = NULL;
  void* shm_buf2 = NULL;
  shm = os_shared_memory_alloc( 4096, shm_name ); test( 1, "os_shared_memory_alloc" );
  shm_same = os_shared_memory_open( shm_name ); test( 1, "os_shared_memory_open" );
  shm_buf = os_shared_memory_view_open( shm, rng_1u64(0, 2000 ) );
  shm_buf2 = os_shared_memory_view_open( shm_same, rng_1u64(0, 2000 ) );
  // Make sure its writable
  MemorySet( shm_buf, seed, 2000 ); test( shm_buf, "os_shared_memory_view_open" );
  res = MemoryCompare( shm_buf, shm_buf2, 2000 );
  test( res == 0, "os_shared_memory integrity check" );
  os_shared_memory_view_close( shm, shm_buf );
  os_shared_memory_close( shm_same );
  os_shared_memory_close( shm );


  SECTION( "OS Miscellaneous" );
  // Valid Hostname
  String8 hostname = os_machine_name();
  B32 hostname_good = (hostname.size > 0);
  U8 x_char;
  for (int i=0; i<hostname.size; ++i)
  {
    x_char = hostname.str[i];
    if ( char_is_alpha( x_char ) || char_is_digit( x_char, 10 ) || x_char == '-') { continue; }
    hostname_good = 0;
  } test( hostname_good, "os_machine_name" );
  MANUAL_CHECKF( "os_machine_name: %s", os_machine_name().str );

  MANUAL_CHECKF( "os_allocation_granularity: %lu", os_allocation_granularity() );
  MANUAL_CHECKF( "os_logical_core_count: %lu", os_logical_core_count() );
  MANUAL_CHECKF( "os_get_pid, %d", os_get_pid() );
  MANUAL_CHECKF( "os_get_tid, %d", os_get_tid() );
  String8List env = os_get_environment(); test( env.first != NULL, "os_get_environment" );
  MANUAL_CHECKF( "os_get_environment Nodes: %lu Third Node: %s",
                 env.node_count,
                 env.first->next->next->string.str);
  // Path query tests
  String8List syspath = {0};
  os_string_list_from_system_path( g_arena, OS_SystemPath_Binary, &syspath );
  MANUAL_CHECKF( "os_string_list_from_system_path( OS_SystemPath_Binary ): %s",
                syspath.first->string.str ); // Fail
  os_string_list_from_system_path( g_arena, OS_SystemPath_Initial, &syspath );
  MANUAL_CHECKF( "os_string_list_from_system_path( OS_SystemPath_Initial ): %s",
               syspath.first->string.str );
  os_string_list_from_system_path( g_arena, OS_SystemPath_UserProgramData, &syspath );
  MANUAL_CHECKF( "os_string_list_from_system_path( OS_SystemPath_UserProgramData ): %s",
               syspath.first->string.str );
  // TODO: Not implimented on Linux
  // os_string_list_from_system_path( g_arena, OS_SystemPath_ModuleLoad, &syspath );
  // MANUAL_CHECKF( "os_string_list_from_system_path( OS_SystemPath_ModuleLoad ): %s",
                 // syspath.first->string.str );
  test( 0, "os_string_list_from_system_path( OS_SystemPath_ModuleLoad )" );

  // Test setting thread name
  LNX_thread thread_self = pthread_self();
  String8 thread_name = str8_lit( "test_thread_name" );
  MemoryZeroArray( g_buffer );
  os_set_thread_name( thread_name );
  prctl( PR_GET_NAME , g_buffer );
  // Thread name capped to 16 on Linux
  if (OS_WINDOWS)
  { res = MemoryCompare( thread_name.str, g_buffer, thread_name.size ); }
  else if (OS_LINUX)
  { res = MemoryCompare( thread_name.str, g_buffer, Min(thread_name.size, 15) ); }

  test( res == 0, "os_set_thread_name" );
  MANUAL_CHECKF( "Current thread name: %s", g_buffer );

  SECTION( "OS File Handling" );
  // Do an open, read/write, close test
  OS_Handle file = {0};
  String8 qbf = str8_lit( "The quick brown fox jumped over the lazy dog" );
  String8 filename = str8_lit("os_file_test.txt");
  MemoryZeroArray( g_buffer );
  file = os_file_open( OS_AccessFlag_Read | OS_AccessFlag_Write, filename );
  test( *file.u64 != 0, "os_file_open" );
  os_file_write( file, rng_1u64(0, qbf.size), qbf.str );
  os_file_read( file, rng_1u64(0, qbf.size), g_buffer );
  res = 0 == MemoryCompare( qbf.str, g_buffer, qbf.size );
  test( res, "os_file_read"); test( res, "os_file_write");
  os_file_close( file );
  res = os_delete_file_at_path( filename ); test( res, "os_delete_file_at_path" );

  void* filebuf = NULL;
  OS_Handle filemap = {0};
  filename = str8_lit("os_file_map_test.txt");
  file = os_file_open( OS_AccessFlag_Read | OS_AccessFlag_Write, filename );
  filemap = os_file_map_open( 0x0, file );
  test( *filemap.u64 != 0, "os_file_map_open" );

  filebuf = os_file_map_view_open( filemap, 0x0, rng_1u64(0, 1024) );
  test( filebuf, "os_file_map_view_open" );
  os_file_map_view_close( filemap, filebuf ); test( 1, "os_file_map_view_close" );
  os_file_map_close( filemap ); test( 1, "os_file_map_close" );

  // File Iteration Tests
  OS_FileIter* file_iter = NULL;
  OS_FileInfo iter_info = {0};
  file_iter = os_file_iter_begin( g_arena, str8_lit("."), 0x0 );
  res = os_file_iter_next( g_arena, file_iter, &iter_info );
  test( res, "os_file_iter_next" ); test( res, "os_file_iter_begin iteration" );
  MANUAL_CHECKF( "os_file_iter_next filename: %s", iter_info.name );
  os_file_iter_end( file_iter );

  // Make Directory Tests
  String8 mkdir_file = str8_lit( "os_mkdir_test" );
  res = os_make_directory(  mkdir_file ); test( res, "os_make_directory" );
  res = os_delete_file_at_path( mkdir_file ); test( res, "os_delete_file_at_path delete directory" );

  MANUAL_CHECKF( "os_now_unix (Wall Clock): %lu", os_now_unix() );
  DateTime time_universal = os_now_universal_time();
  DateTime time_to_local = os_local_time_from_universal_time( &time_universal );
  DateTime time_to_universal = os_universal_time_from_local_time( &time_to_local );
  MANUAL_CHECKF( "os_now_universal_time: year: %d \n"
                 "month: %d \n"
                 "week: %d \n"
                 "day: %d \n"
                 "time: %d:%d:%d:%d:%d \n",
                 time_universal.year,
                 time_universal.mon,
                 time_universal.wday,
                 time_universal.day,
                 time_universal.hour,
                 time_universal.min,
                 time_universal.sec,
                 time_universal.msec,
                 time_universal.micro_sec );
  MANUAL_CHECKF( "os_universal_time_from_local_time: year: %d \n"
                 "month: %d \n"
                 "week: %d \n"
                 "day: %d \n"
                 "time: %d:%d:%d:%d:%d",
                 time_to_universal.year,
                 time_to_universal.mon,
                 time_to_universal.wday,
                 time_to_universal.day,
                 time_to_universal.hour,
                 time_to_universal.min,
                 time_to_universal.sec,
                 time_to_universal.msec,
                 time_to_universal.micro_sec );
  MANUAL_CHECKF( "os_local_time_from_universal_time: year: %d \n"
                 "month: %d \n"
                 "week: %d \n"
                 "day: %d \n"
                 "time: %d:%d:%d:%d:%d",
                 time_to_local.year,
                 time_to_local.mon,
                 time_to_local.wday,
                 time_to_local.day,
                 time_to_local.hour,
                 time_to_local.min,
                 time_to_local.sec,
                 time_to_local.msec,
                 time_to_local.micro_sec );
  MANUAL_CHECKF( "os_now_microseconds (CPU clock): %lu", os_now_microseconds() );

  U64 sleep_start = os_now_microseconds();
  os_sleep_milliseconds( 1 );
  U64 sleep_elapsed_us = (os_now_microseconds() - sleep_start);
  test( sleep_elapsed_us > 999, "os_sleep_milliseconds(1)" );
  MANUAL_CHECKF( "Sleep Time: %lu us", sleep_elapsed_us );

  // Launch Process
  OS_Handle process = {0};
  String8List process_command = {0};
  OS_LaunchOptions process_config = {0};
  if (OS_WINDOWS)
  {
    // Untested
    str8_list_push( g_arena, &process_command, str8_lit("C:/Windows/cmd.exe") );
  }
  else
  {
    str8_list_push( g_arena, &process_command, str8_lit("/usr/bin/echo") );
    str8_list_push( g_arena, &process_command, str8_lit("Make me a sandwich!") );
  }
  process_config.cmd_line = process_command;
  /* res = os_launch_process( &process_config, &process ); test( res, "os_launch_process" ); */

  SECTION( "Thread Syncronization" );
  OS_Handle mut = {0};
  OS_Handle thread2 = {0};
  struct Dummy_Payload dummy_payload = {0};
  U32* last_lock = (U32*)g_shared; g_shared += sizeof(U32);
  *last_lock = 1;

  // Basic Mutex Test
  mut = os_mutex_alloc();
  dummy_payload.mutex = mut;
  dummy_payload.last_lock = last_lock;
  os_mutex_take_( mut );
  thread2 = os_launch_thread( dummy_routine, &dummy_payload, NULL );
  *last_lock = 1;
  os_sleep_milliseconds( 100 );
  *last_lock = 1;
  os_mutex_drop_( mut );
  os_sleep_milliseconds( 100 ); test( *last_lock == 2, "os_mutex thread ordering check" );
  os_mutex_release( mut );
  /* os_release_thread_handle( thread2 ); */

  // Basic rwlock Test
  // NOTE: Bogus test, don't know how to check sanely
  g_share_data->rwlock_last = 0;
  g_share_data->rwlock = os_rw_mutex_alloc();
  os_rw_mutex_take_w( g_share_data->rwlock );
  OS_Handle rwlock_thread_1 = os_launch_thread( rwlock_routine, g_share_data, 0x0 );
  OS_Handle rwlock_thread_2 = os_launch_thread( rwlock_routine, g_share_data, 0x0 );
  os_sleep_milliseconds( 50 );
  os_rw_mutex_drop_w( g_share_data->rwlock );
  os_sleep_milliseconds( 50 );
  MANUAL_CHECKF( "rwlock value: %u", g_share_data->rwlock_last );
  os_release_thread_handle( rwlock_thread_1 );
  os_release_thread_handle( rwlock_thread_2 );

  // Finalization
  SECTION( "Summary" );
  total = passes + manual_checks;
  printf( "\x1b[32mPassed\x1b[39m: %d \n", passes );
  printf( "\x1b[31mFailed\x1b[39m: %d \n", failed );
  printf( "\x1b[1mManual Checks Run\x1b[39m: %d \n", manual_checks );
  printf( "\x1b[1mTotal Tests Run: %d \n", total );

  os_exit_process( 0 ); test( 0, "os_exit_process" );
}
