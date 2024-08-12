
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

static int passes = 0;
static int failed = 0;

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

#define MANUAL_CHECKF(fmt, ...) printf( "\x1b[1mManual Check| \x1b[0m" fmt "\n" , ##__VA_ARGS__ )

void
SECTION( char* label )
{
  printf( "\n\x1b[36;1m[%s]\x1b[0m\n", label );
}

int main()
{
  TCTX tctx_;
  tctx_init_and_equip(&tctx_);
  os_init(); test( 1, "os_init" );
  Arena* g_arena = arena_alloc();

  B32 res = 0;
  U64* g_buffer[4096];
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
  MemorySet( res_ring+2048, 103, 4000 );
  test( MemoryCompare( res_ring, res_ring+4096, 128) == 0,
        "os_alloc_ring_buffer write over boundary" );

  // No easy way to check if it suceeded
  os_free_ring_buffer( res_ring, 4096 ); test( 1, "os_free_ring_buffer" );
  MANUAL_CHECKF( "os_page_size output: %lu", os_page_size() );
  MANUAL_CHECKF( "os_large_page_size output: %lu", os_large_page_size() );

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
  // os_string_list_from_system_path( g_arena, OS_SystemPath_ModuleLoad, &syspath );
  // MANUAL_CHECKF( "os_string_list_from_system_path( OS_SystemPath_ModuleLoad ): %s",
                 // syspath.first->string.str );
  test( 0, "os_string_list_from_system_path( OS_SystemPath_ModuleLoad )" );

  // Test setting thread name
  LNX_thread thread = pthread_self();
  String8 thread_name = str8_lit( "test_thread_name" );
  MemoryZeroArray( g_buffer );
  // os_set_thread_name( thread_name );
  pthread_getname_np( thread, g_buffer, 4096 );
  res = MemoryCompare( thread_name.str, g_buffer, thread_name.size );
  test( res == 0, "os_set_thread_name" );

  SECTION( "OS File Handling" );
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
    *filemap.u64 = 0;

  filebuf = os_file_map_view_open( filemap, 0x0, rng_1u64(0, 1024) );

  // Show Passed/Failed
  printf( "\n\x1b[32mPassed\x1b[39m: %d \n", passes );
  printf( "\x1b[31mFailed\x1b[39m: %d \n", failed );

  os_exit_process( 0 ); test( 0, "os_exit_process" );
}
