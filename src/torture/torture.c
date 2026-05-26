// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// command line
global String8      g_stdout_file_name = str8_lit_comp("torture.out");
global String8      g_wdir;
global String8      g_out = str8_lit_comp("torture_artifacts");
global B32          g_verbose;
global B32          g_redirect_stdout = 1;
global B32          g_stop_on_first_fail_or_crash = 1;
global B32          g_build_only = 0;
global String8      g_test_data;

// tests
global TestInfo *g_sorted_test_infos[ArrayCount(test_infos)] = {0};
global B32      g_is_first_print;

// invoke
global U64      g_last_exit_code;
global Arena   *g_output_arena;
global String8  g_output;
global String8  g_errors;

// tools
global B32     g_gui;
global String8 g_radbin_path;
global String8 g_cl_path;
global String8 g_clang_path;
global String8 g_gcc_path;
global String8 g_linker_path;

////////////////////////////////

internal String8
t_name_from_test_info(Arena *arena, TestInfo *test_info)
{
  String8 result = str8f(arena, "%S/%S", test_info->layer, test_info->label);
  return result;
}

internal String8List
t_test_group_from_name(Arena *arena, String8 pattern)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List matches = {0};
  for EachIndex(i, test_infos_count) {
    TestInfo *test_info = g_sorted_test_infos[i];
    if (str8_match_wildcard(t_name_from_test_info(scratch.arena, test_info), pattern, 0)) {
      str8_list_push(arena, &matches, test_info->layer);
    }
  }
  scratch_end(scratch);
  return matches;
}

////////////////////////////////

internal Linker
t_id_linker(void)
{
  String8 name = str8_chop_last_dot(str8_skip_last_slash(g_linker_path));
  if (str8_match(name, str8_lit("radlink"),  StringMatchFlag_CaseInsensitive)) { return Linker_radlink;  }
  if (str8_match(name, str8_lit("link"),     StringMatchFlag_CaseInsensitive)) { return Linker_msvc; }
  if (str8_match(name, str8_lit("lld-link"), StringMatchFlag_CaseInsensitive)) { return Linker_lld; }
  return Linker_Null;
}

internal B32
t_write_file_list(String8 name, String8List data)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  B32 is_written = write_data_list_to_file_path(path, data);
  scratch_end(scratch);
  return is_written;
}

internal B32
t_write_file(String8 name, String8 data)
{
  String8Node temp_node = {0};
  temp_node.string = data;
  
  String8List temp_list = {0};
  str8_list_push_node(&temp_list, &temp_node);
  
  return t_write_file_list(name, temp_list);
}

internal String8
t_read_file(Arena *arena, String8 name)
{
  Temp scratch = scratch_begin(&arena,1);
  String8 path = t_make_file_path(scratch.arena, name);
  String8 data = data_from_file_path(arena, path);
  scratch_end(scratch);
  return data;
}

internal B32
t_delete_file(String8 name)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  B32 is_deleted = delete_file_at_path(path);
  scratch_end(scratch);
  return is_deleted;
}

internal void
t_delete_dir(String8 path)
{
  Temp scratch = scratch_begin(0,0);
  
  FileIter *file_iter = file_iter_begin(scratch.arena, path, 0);
  for (;;) {
    FileInfo info = {0};
    if ( ! file_iter_next(scratch.arena, file_iter, &info)) { break; }
    
    if (info.props.flags & FilePropertyFlag_IsFolder) {
      t_delete_dir(str8f(scratch.arena, "%S/%S", path, info.name));
      continue;
    }
    
    String8 file_path = str8f(scratch.arena, "%S/%S", path, info.name);
    if ( ! delete_file_at_path(file_path)) {
      fprintf(stderr, "ERROR: unable to delete file %.*s\n", str8_varg(file_path));
    }
  }
  file_iter_end(file_iter);
  
  // TODO: delete directories
  
  scratch_end(scratch);
}

internal String8
t_make_file_path(Arena *arena, String8 name)
{
#if OS_WINDOWS
  return push_str8f(arena, "%S\\%S", g_wdir, name);
#else
  return push_str8f(arena, "%S/%S", g_wdir, name);
#endif
}

internal B32
t_make_dir(String8 name)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = make_directory(t_make_file_path(scratch.arena, name));
  scratch_end(scratch);
  return is_ok;
}

internal void
t_run_caller(void *raw_ctx)
{
  Temp scratch = scratch_begin(0,0);
  
  g_is_first_print = 1;
  
  T_RunCtx *ctx = raw_ctx;  
  ctx->result.status = TestStatus_Pass;
  
  String8List test_out = {0};
  
  if (ctx->test->skip) {
    ctx->result.status = TestStatus_Skip;
  } else {
    ctx->test->test_fn(scratch.arena, ctx->cmdline, g_wdir, &ctx->result, &test_out);
  }
  
  if (ctx->result.status == TestStatus_Fail || ctx->result.status == TestStatus_Crash) {
    for EachNode(n, String8Node, test_out.first) {
      t_errorf("%S", n->string);
    }
    if (g_errors.size) {
      t_errorf("%S\n", g_errors);
    }
    if (g_output.size) {
      t_errorf("%S\n", g_output);
    }
  }
  
  scratch_end(scratch);
}

internal TestResult
t_run(CmdLine *cmdline, TestInfo *test, String8 user_data)
{
  T_RunCtx ctx = { .test = test, .cmdline = cmdline, .user_data = user_data, .result.status = TestStatus_Fail };
  t_run_caller(&ctx);
  
  if (ctx.result.status == TestStatus_Fail || ctx.result.status == TestStatus_Crash) {
    if (g_output.size > 0 || g_errors.size > 0) {
      t_errorf("Last captured output:\n");
      if (g_output.size) { t_errorf("%S\n", g_output); }
      if (g_errors.size) { t_errorf("%S\n", g_errors); }
    }
  }
  
  fflush(stdout);
  fflush(stderr);
  
  return ctx.result;
}

internal String8
t_radbin_path(void)
{
  if (g_radbin_path.size == 0) {
    local_persist U8 buffer[4096];
    Arena *arena = arena_alloc_(&(ArenaParams){ .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer });
#if OS_WINDOWS
    g_radbin_path = full_path_from_path(arena, str8_lit("radbin.exe"));
#else
    g_radbin_path = full_path_from_path(arena, str8_lit("radbin"));
#endif
  }
  AssertAlways(g_radbin_path.size);
  return g_radbin_path;
}

internal String8
t_cl_path(void)
{
  if (g_cl_path.size == 0) {
#if OS_WINDOWS
    local_persist U8 buffer[4096];
    Arena *arena = arena_alloc_(&(ArenaParams){ .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer });
    wchar_t full_path[MAX_PATH];
    DWORD full_path_size = SearchPathW(0, L"cl.exe", 0, ArrayCount(full_path), full_path, 0);
    g_cl_path = str8_from_16(arena, str16((U16*)full_path, full_path_size));
#else
    g_cl_path = str8_zero();
#endif
  }
  AssertAlways(g_cl_path.size);
  return g_cl_path;
}

internal String8
t_clang_path(void)
{
  if (g_clang_path.size == 0) {
#if OS_WINDOWS
    local_persist U8 buffer[4096];
    Arena  *arena = arena_alloc_(&(ArenaParams){ .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer });
    wchar_t full_path[MAX_PATH];
    DWORD   full_path_size = SearchPathW(0, L"clang.exe", 0, ArrayCount(full_path), full_path, 0);
    g_clang_path = str8_from_16(arena, str16((U16*)full_path, full_path_size));
#else
    g_clang_path = str8_lit("clang");
#endif
  }
  AssertAlways(g_clang_path.size);
  return g_clang_path;
}

internal String8
t_gcc_path(void)
{
  if (g_gcc_path.size == 0) {
#if OS_WINDOWS
    local_persist U8 buffer[4096];
    Arena  *arena = arena_alloc_(&(ArenaParams){ .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer });
    wchar_t full_path[MAX_PATH];
    DWORD   full_path_size = SearchPathW(0, L"gcc.exe", 0, ArrayCount(full_path), full_path, 0);
    g_gcc_path = str8_from_16(arena, str16((U16*)full_path, full_path_size));
#else
    g_gcc_path = str8_lit("gcc");
#endif
  }
  AssertAlways(g_gcc_path.size);
  return g_gcc_path;
}

internal String8
t_cl_version(void)
{
  local_persist String8 version;
  
  if ( ! version.size) {
    Temp scratch = scratch_begin(0, 0);
    
    t_invoke_cl("");
    AssertAlways(g_last_exit_code == 0);
    
    String8 needle = str8_lit("Version");
    U64 version_lo = str8_find_needle(g_output, 0, needle, 0);
    version_lo += needle.size + 1;
    AssertAlways(version_lo < g_output.size);
    
    U64 version_hi = str8_find_needle(g_output, version_lo, str8_lit(" "), 0);
    AssertAlways(version_hi < g_output.size);
    
    version = str8_substr(g_output, r1u64(version_lo, version_hi));
    AssertAlways(version.size > 0);
    
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
    version = str8_copy(arena, version);
    
    MemoryZeroStruct(&g_output);
    scratch_end(scratch);
  }
  
  return version;
}

internal String8
t_radlink_path(void)
{
  local_persist String8 path = {0};
  if(g_linker_path.size != 0)
  {
    path = g_linker_path;
  }
  else if (path.size == 0) {
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
#if OS_WINDOWS
    path = full_path_from_path(arena, str8_lit("radlink.exe"));
#else
    path = full_path_from_path(arena, str8_lit("radlink"));
#endif
    AssertAlways(path.size);
  }
  return path;
}

internal String8
t_raddbg_path(void)
{
  local_persist String8 path = {0};
  if (path.size == 0) {
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
    String8 raddbg_base_name = g_gui ? str8_lit("raddbg") : str8_lit("raddbg_non_graphical");
#if OS_WINDOWS
    Temp scratch = scratch_begin(0, 0);
    path = full_path_from_path(arena, str8f(scratch.arena, "%S.exe", raddbg_base_name));
    scratch_end(scratch);
#else
    path = full_path_from_path(arena, raddbg_base_name);
#endif
    AssertAlways(path.size);
  }
  return path;
}

internal String8
t_cwd_path(void)
{
  local_persist U8 path[4096] = {0};
  if (path[0] == 0) {
    Temp scratch = scratch_begin(0, 0);
    String8 cwd = get_current_path(scratch.arena);
    
    // TODO: linux and windows return two different things, we need to settle what to do here
    // should get_current_path return directory paths with slash or no slash
    if ((str8_match_wildcard(cwd, str8_lit("*\\"), 0) && str8_match_wildcard(cwd, str8_lit("*/"), 0))) {
      cwd = str8_chop_last_slash(cwd);
      cwd = str8_chop_last_slash(cwd);
    } else {
      cwd = str8_chop_last_slash(cwd);
    }
    
    MemoryCopyStr8(path, cwd);
    path[cwd.size] = 0;
    scratch_end(scratch);
  }
  return str8_cstring_capped(path, path+sizeof(path));
}

internal String8
t_src_path(void)
{
  local_persist U8 path[4096] = {0};
  if (path[0] == 0) {
    Temp scratch = scratch_begin(0, 0);
    String8 src = str8f(scratch.arena, "%S/src", t_cwd_path());
    MemoryCopyStr8(path, src);
    path[src.size] = 0;
    scratch_end(scratch);
  }
  return str8_cstring_capped(path, path+sizeof(path));
}

internal B32
t_invoke_env(String8 exe_path, String8 cmdline, String8List env, U64 timeout_us)
{
  Temp scratch = scratch_begin(&g_output_arena,1);
  
  B32 is_ok = 0;
  
  // clean up global state
  arena_clear(g_output_arena);
  MemoryZeroStruct(&g_output);
  g_last_exit_code = max_U64;
  
  String8List stdout_parts = {0};
  String8List stderr_parts = {0};
  U64 stdout_idx = 0;
  U64 stderr_idx = 1;
  
#if OS_WINDOWS
  typedef enum {
    Win32CaptureState_Null,
    Win32CaptureState_Pending,
    Win32CaptureState_EOF,
  } Win32CaptureState;
  typedef struct {
    HANDLE             read_pipe_handle;
    HANDLE             write_pipe_handle;
    HANDLE             event;
    OVERLAPPED         overlapped;
    U64                buffer_size;
    U8                *buffer;
    U64                wait_idx;
    String8List       *parts;
    Win32CaptureState  state;
  } Win32Capture;
  
  Win32Capture captures_win32[2] = {0};
  for EachElement(i, captures_win32) {
    // create read pipe
    local_persist U64 pipe_counter; ins_atomic_u64_inc_eval(&pipe_counter);
    String8  pipe_name   = str8f(scratch.arena, "\\\\.\\pipe\\rad_torture_%u_%llu", GetCurrentProcessId(), pipe_counter);
    String16 pipe_name16 = str16_from_8(scratch.arena, pipe_name);
    SECURITY_ATTRIBUTES read_at  = { .nLength = sizeof(read_at),  .bInheritHandle = 0 };
    captures_win32[i].read_pipe_handle = CreateNamedPipeW(pipe_name16.str, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_WAIT, 1, MB(1), MB(1), 0, &read_at);
    AssertAlways(captures_win32[i].read_pipe_handle != INVALID_HANDLE_VALUE);
    
    // create overlapped write file
    SECURITY_ATTRIBUTES write_at = { .nLength = sizeof(write_at), .bInheritHandle = 1 };
    captures_win32[i].write_pipe_handle = CreateFileW(pipe_name16.str, GENERIC_WRITE, 0, &write_at, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    AssertAlways(captures_win32[i].write_pipe_handle != INVALID_HANDLE_VALUE);
    
    // create event for overlapped
    captures_win32[i].event = CreateEventW(0, 1, 0, 0);
    AssertAlways(captures_win32[i].event != NULL);
    
    // alloc capture buffer
    captures_win32[i].buffer_size = MB(1);
    captures_win32[i].buffer = push_array(scratch.arena, U8, captures_win32[i].buffer_size);
  }
  captures_win32[stdout_idx].parts = &stdout_parts;
  captures_win32[stderr_idx].parts = &stderr_parts;
  
  File read_capture_handles [ArrayCount(captures_win32)] = {0};
  File write_capture_handles[ArrayCount(captures_win32)] = {0};
  for EachElement(i, captures_win32) { read_capture_handles[i]  = (File){ (U64)captures_win32[i].read_pipe_handle  }; }
  for EachElement(i, captures_win32) { write_capture_handles[i] = (File){ (U64)captures_win32[i].write_pipe_handle }; }
#else
  typedef struct {
    int            fds[2];
    String8List   *parts;
    B32            is_live;
    struct pollfd *poll_fd;
  } LinuxCapture;
  
  LinuxCapture captures_linux[2] = {0};
  for EachElement(i, captures_linux) {
    if (pipe2(captures_linux[i].fds, 0) != 0) {
      fprintf(stderr, "ERROR: failed to create pipe for output capture\n");
      goto exit;
    }
    captures_linux[i].is_live = 1;
  }
  
  captures_linux[0].parts = &stdout_parts;
  captures_linux[1].parts = &stderr_parts;
  
  File read_capture_handles[2] = {0}, write_capture_handles[2] = {0};
  read_capture_handles[0].u64[0] = captures_linux[0].fds[0];
  read_capture_handles[1].u64[0] = captures_linux[1].fds[0];
  write_capture_handles[0].u64[0] = captures_linux[0].fds[1];
  write_capture_handles[1].u64[0] = captures_linux[1].fds[1];
#endif
  // Build Launch Options
  ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .consoleless = 1,
    .env         = env,
    .stdout_file = write_capture_handles[stdout_idx],
    .stderr_file = write_capture_handles[stderr_idx],
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  str8_list_push_front(scratch.arena, &launch_opts.cmd_line, exe_path);
  
  String8 full_cmd_line = str8_list_join(scratch.arena, &launch_opts.cmd_line, &(StringJoin){ .sep = str8_lit(" ") });
  
  // invoke exe
  Process process_handle = process_launch(&launch_opts);
  if (process_match(process_handle, process_zero())) { goto exit; }
  
  // capture process output
#if OS_WINDOWS
  {
    // close handle so last to ReadFile does not block
    for EachElement(i, captures_win32) {
      CloseHandle(captures_win32[i].write_pipe_handle);
      MemoryZeroStruct(&write_capture_handles[i]);
    }
    
    B32 is_process_live = 1;
    for (U64 endt_us = ENDT_US(timeout_us);;) {
      HANDLE wait_handles[ArrayCount(captures_win32) + 1] = {0};
      U64    wait_handle_count = 0;
      
      // queue process
      if (is_process_live) {
        wait_handles[wait_handle_count++] = (HANDLE)process_handle.u64[0];
      }
      
      for EachElement(capture_idx, captures_win32) {
        while (captures_win32[capture_idx].state == Win32CaptureState_Null) {
          // init overlapped so when child writes to capture buffer this event is signaled
          AssertAlways(ResetEvent(captures_win32[capture_idx].event));
          MemoryZeroStruct(&captures_win32[capture_idx].overlapped);
          captures_win32[capture_idx].overlapped.hEvent = captures_win32[capture_idx].event;
          
          // begin overlapped read
          DWORD read_size = 0;
          if (ReadFile(captures_win32[capture_idx].read_pipe_handle, captures_win32[capture_idx].buffer, captures_win32[capture_idx].buffer_size, &read_size, &captures_win32[capture_idx].overlapped)) {
            if (read_size > 0) {
              String8 string      = str8(captures_win32[capture_idx].buffer, read_size);
              String8 string_copy = str8_copy(scratch.arena, string);
              str8_list_push(scratch.arena, captures_win32[capture_idx].parts, string_copy);
            } else {
              captures_win32[capture_idx].state = Win32CaptureState_EOF;
            }
          } else {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
              captures_win32[capture_idx].state = Win32CaptureState_Pending;
            } else {
              AssertAlways(error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE);
              captures_win32[capture_idx].state = Win32CaptureState_EOF;
            }
            break;
          }
        }
        
        if (captures_win32[capture_idx].state == Win32CaptureState_Pending) {
          // event now should signal whenever pipe has data to read
          captures_win32[capture_idx].wait_idx = wait_handle_count++;
          wait_handles[captures_win32[capture_idx].wait_idx] = captures_win32[capture_idx].event;
        } else {
          captures_win32[capture_idx].wait_idx = max_U64;
        }
      }
      
      // exit if there are no handles
      if (wait_handle_count == 0) { break; }
      
      // compute wait time
      DWORD wait_ms = INFINITE;
      if (timeout_us != max_U64) {
        U64 now_us = now_time_us();
        wait_ms = now_us < endt_us ? ClampTop((endt_us - now_us + 999) / 1000, max_U32-1) : 0;
      }
      
      // wait on process and read pipes
      DWORD wait_result = WaitForMultipleObjects(wait_handle_count, wait_handles, 0, wait_ms);
      
      if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + wait_handle_count) {
        DWORD wait_idx = wait_result - WAIT_OBJECT_0;
        
        if (is_process_live && wait_idx == 0) {
          DWORD exit_code = 0;
          if(GetExitCodeProcess((HANDLE)process_handle.u64[0], &exit_code)) {
            g_last_exit_code = exit_code;
          }
          is_process_live = 0;
        } else {
          // find signaled pipe
          U64 pipe_idx = max_U64;
          for EachElement(i, captures_win32) {
            if (captures_win32[i].wait_idx == wait_idx) {
              pipe_idx = i;
            }
          }
          AssertAlways(pipe_idx != max_U64);
          
          DWORD read_size;
          if (GetOverlappedResult(captures_win32[pipe_idx].read_pipe_handle, &captures_win32[pipe_idx].overlapped, &read_size, 0)) {
            if (read_size > 0) {
              // append capture part
              String8 string      = str8(captures_win32[pipe_idx].buffer, read_size);
              String8 string_copy = str8_copy(scratch.arena, string);
              str8_list_push(scratch.arena, captures_win32[pipe_idx].parts, string_copy);
              
              // queue next overlapped read
              captures_win32[pipe_idx].state = Win32CaptureState_Null;
            } else {
              captures_win32[pipe_idx].state = Win32CaptureState_EOF;
            }
          } else {
            captures_win32[pipe_idx].state = Win32CaptureState_EOF;
          }
        }
      } else if (wait_result == WAIT_TIMEOUT) {
        // nothing woke up in the given timeout -- stop reading pipes and being exit
        break;
      }
    }
    
    // (timeout) kill process if alive so we can safeley cancel async IO
    if (is_process_live) {
      if (TerminateProcess((HANDLE)process_handle.u64[0], 999)) {
        if (WaitForSingleObject((HANDLE)process_handle.u64[0], 10000) != WAIT_OBJECT_0) {
          Assert(0 && "process is taking too long to exit");
        }
      } else { Assert(0 && "failed to kill process"); }
      
      DWORD exit_code = 0;
      if (GetExitCodeProcess((HANDLE)process_handle.u64[0], &exit_code)) {
        g_last_exit_code = exit_code;
      } else { Assert(0 && "failed to get process exit code"); }
    }
    
    // (timeout) cancel pending async IO
    for EachElement(i, captures_win32) {
      if (captures_win32[i].state == Win32CaptureState_Pending) {
        BOOL  cancel_ok    = CancelIoEx(captures_win32[i].read_pipe_handle, &captures_win32[i].overlapped);
        DWORD cancel_error = cancel_ok ? ERROR_SUCCESS : GetLastError();
        AssertAlways(cancel_ok || cancel_error == ERROR_NOT_FOUND);
        
        DWORD read_size = 0;
        if (GetOverlappedResult(captures_win32[i].read_pipe_handle, &captures_win32[i].overlapped, &read_size, 1)) {
          if (read_size > 0) {
            String8 string      = str8(captures_win32[i].buffer, read_size);
            String8 string_copy = str8_copy(scratch.arena, string);
            str8_list_push(scratch.arena, captures_win32[i].parts, string_copy);
          }
        } else {
          DWORD error = GetLastError();
          AssertAlways(error == ERROR_OPERATION_ABORTED || error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE);
        }
        captures_win32[i].state = Win32CaptureState_EOF;
      }
    }
    
    // close windows specific handles
    CloseHandle((HANDLE)process_handle.u64[0]);
    for EachElement(i, captures_win32) {
      CloseHandle(captures_win32[i].event);
    }
  }
#elif OS_LINUX
  // close handle so read(2) does not block
  for EachElement(i, write_capture_handles) {
    close((int)write_capture_handles[i].u64[0]);
    MemoryZeroStruct(&write_capture_handles[i]);
  }
  
  pid_t pid   = (pid_t)process_handle.u64[0];
  int   pidfd = syscall(SYS_pidfd_open, pid, 0);
  if (pidfd < 0) {
    fprintf(stderr, "ERROR: failed to translate pid(%d) to pidfd\n", pid);
    goto exit;
  }
  
  B32  is_process_live          = 1;
  U64  endt_us                  = ENDT_US(timeout_us);
  U64  read_buffer_default_size = MB(1);
  U64  read_buffer_size         = read_buffer_default_size;
  U8  *read_buffer              = push_array(scratch.arena, U8, read_buffer_default_size);
  for (;;) {
    struct pollfd fds[3] = {0};
    int           nfds   = 0;
    
    // append process
    if (is_process_live) {
      fds[nfds++] = (struct pollfd){ .fd = pidfd, .events = POLLIN | POLLHUP };
    }
    
    // append pipes
    for EachElement(i, captures_linux) {
      if (captures_linux[i].is_live) {
        captures_linux[i].poll_fd = &fds[nfds];
        fds[nfds++] = (struct pollfd){ .fd = captures_linux[i].fds[0], .events = POLLIN | POLLHUP };
      }
    }
    
    // exit if there are no more handles to poll
    if (nfds == 0) { break; }
    
    // compute wait time
    int wait_ms = -1;
    if (timeout_us != max_U64) {
      U64 now_us = now_time_us();
      wait_ms = now_us < endt_us ? ClampTop((endt_us - now_us + 999) / 1000, max_U32-1) : 0;
    }
    
    // wait for kernel to signal any of the wait handles
    int poll_result = poll(fds, nfds, wait_ms);
    if (poll_result < 0) {
      fprintf(stderr, "ERROR: poll failed with errono %d\n", errno);
      break;
    }
    if (poll_result == 0) {
      fprintf(stderr, "WARNING: poll timeout\n");
      break;
    }
    // handle case where process exited while waiting for a signal
    if (is_process_live) {
      if (fds[0].revents & POLLIN) {
        // reap process
        int status;
        if (waitpid(pid, &status, 0) >= 0) {
          g_last_exit_code = WEXITSTATUS(status);
        } else {
          fprintf(stderr, "ERROR: failed to reap process %d\n", pid);
        }
        
        // signal on process fd means exit
        is_process_live = 0;
      }
    }
    
    for EachElement(i, captures_linux) {
      if (captures_linux[i].is_live) {
        if (captures_linux[i].poll_fd->revents & POLLIN) {
          for (;;) {
            if (read_buffer_size == 0) {
              read_buffer_size = read_buffer_default_size;
              read_buffer      = push_array(scratch.arena, U8, read_buffer_default_size);
            }
            
            ssize_t read_size = LNX_RETRY_ON_EINTR(read(captures_linux[i].poll_fd->fd, read_buffer, read_buffer_size));
            if (read_size > 0) {
              str8_list_push(scratch.arena, captures_linux[i].parts, str8(read_buffer, read_size));
              read_buffer      += read_size;
              read_buffer_size -= read_size;
            } else if (read_size == 0) {
              captures_linux[i].is_live = 0;
              break;
            } else {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no more data in the pipe
              } else {
                fprintf(stderr, "ERROR: failed to read pipe, errno %d\n", errno);
                captures_linux[i].is_live = 0;
              }
              break;
            }
          }
        }
        
        if (captures_linux[i].poll_fd->revents & POLLHUP) {
          captures_linux[i].is_live = 0;
        }
      }
    }
  }
  
  // close process handle
  if (close(pidfd) < 0) {
    fprintf(stderr, "ERROR: failed to close process handle %d\n", pidfd);
  }
#endif
  
  t_infof("Invoke: {\n");
  t_infof("  CMDL: %S\n", full_cmd_line);
  t_infof("  WDIR: %S\n", g_wdir);
  t_infof("  Exit: %u\n", g_last_exit_code);
  t_infof("}\n");
  
  // update output global
  g_output = str8_list_join(g_output_arena, &stdout_parts, 0);
  g_errors = str8_list_join(g_output_arena, &stderr_parts, 0);
  
  // write to the output file
  if (g_redirect_stdout) {
    write_data_to_file_path(g_stdout_file_name, g_output);
  }
  
  is_ok = 1; // process was launched (does not mean exited successfully)
  exit:;
  for EachElement(i, read_capture_handles)  { file_close(read_capture_handles[i]);  }
  for EachElement(i, write_capture_handles) { file_close(write_capture_handles[i]); }
  
  scratch_end(scratch);
  return is_ok;
}

internal B32
t_invoke(String8 exe_path, String8 cmdline, U64 timeout)
{
  return t_invoke_env(exe_path, cmdline, (String8List){0}, timeout);
}

internal B32
t_invoke_cl(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 cmdl = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  B32 is_ok = t_invoke(t_cl_path(), cmdl, max_U64);
  scratch_end(scratch);
  return is_ok;
}

internal B32
t_invoke_linkerf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 cmdl = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  B32 is_ok = t_invoke(t_radlink_path(), cmdl, max_U64);
  scratch_end(scratch);
  return is_ok;
}

internal B32
t_invoke_radbin(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 cmdl = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  B32 is_ok = t_invoke(t_radbin_path(), cmdl, max_U64);
  scratch_end(scratch);
  return is_ok;
}

internal void
t_kill_all(String8 pattern)
{
  Temp scratch = scratch_begin(0,0);
  DMN_ProcessIter it = {0};
  dmn_process_iter_begin(&it);
  DMN_ProcessInfo info = {0};
  while (dmn_process_iter_next(scratch.arena, &it, &info)) {
    if (str8_match_wildcard(info.name, pattern, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive)) {
#if OS_WINDOWS
      if (!t_invoke(str8_lit("taskkill"), str8f(scratch.arena, "/PID %u /F", info.pid), max_U64)) { fprintf(stderr, "ERROR: failed to invoke taskkill\n"); }
#elif OS_LINUX
      NotImplemented; // TODO: test
      if (!t_invoke(str8_lit("kill"), str8f(scratch.arena, " -9 %u", info.pid), max_U64)) { fprintf(stderr, "ERROR: failed to invoke kill\n"); }
#else
# error NotImplemented
#endif
      if (g_last_exit_code != 0) { fprintf(stderr, "ERROR: failed to kill %u\n", info.pid); }
    }
  }
  dmn_process_iter_end(&it);
  scratch_end(scratch);
}

// TODO: obsolete
internal String8
t_chop_line(String8 *string)
{
  return str8_chop_line(string);
}
// TODO: obsolete
internal B32
t_match_line(String8 *output, String8 expected_line)
{
  return str8_match_wildcard(t_chop_line(output), expected_line, 0);
}

internal B32
t_match_linef(String8 *output, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 expected_line = push_str8fv(scratch.arena, fmt, args);
  B32 is_match = t_match_line(output, expected_line);
  va_end(args);
  scratch_end(scratch);
  return is_match;
}

force_inline int
t_test_info_is_before(TestInfo **a, TestInfo **b)
{
  String8 layer_a = a[0]->layer;
  String8 layer_b = b[0]->layer;
  int cmp = str8_compar(layer_a, layer_b, 0);
  if(cmp == 0)
  {
    cmp = u64_compar(&a[0]->decl_line, &b[0]->decl_line);
  }
  return cmp;
}

internal String8List
t_file_paths_from_dir(Arena *arena, String8 dir)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List dirs = {0};
  String8List files = {0};
  
  FileIter *iter = file_iter_begin(scratch.arena, dir, 0);
  FileInfo file;
  while (file_iter_next(scratch.arena, iter, &file)) {
    if (file.props.flags & FilePropertyFlag_IsFolder) {
      str8_list_pushf(scratch.arena, &dirs, "%S/%S", dir, file.name);
    } else {
      str8_list_pushf(arena, &files, "%S/%S", dir, file.name);
    }
  }
  file_iter_end(iter);
  
  String8List result = {0};
  for EachNode(n, String8Node, dirs.first) {
    String8List sub_result = t_file_paths_from_dir(arena, n->string);
    str8_list_concat_in_place(&result, &sub_result);
  }
  
  scratch_end(scratch);
  //str8_list_concat_in_place(&result, &dirs);
  str8_list_concat_in_place(&result, &files);
  return result;
}

internal int str8_is_before_case_ignore_case(void *a, void *b) { return str8_compar_ignore_case(a, b) < 0; }
internal void t_sort_str8_array(String8Array a) { radsort(a.v, a.count, str8_is_before_case_ignore_case); }

internal B32
t_match_folders(String8 a, String8 b)
{
  Temp scratch = scratch_begin(0,0);
  String8List files_a = t_file_paths_from_dir(scratch.arena, a);
  //for EachNode(n, String8Node, files_a.first) printf("%.*s\n", str8_varg(n->string));
  String8List files_b = t_file_paths_from_dir(scratch.arena, b);
  B32 is_match = 0;
  
  String8Array sorted_a = str8_array_from_list(scratch.arena, &files_a);
  String8Array sorted_b = str8_array_from_list(scratch.arena, &files_b);
  t_sort_str8_array(sorted_a);
  t_sort_str8_array(sorted_b);
  
  if (sorted_a.count == sorted_b.count) {
    for EachIndex(i, sorted_a.count) {
      Temp temp = temp_begin(scratch.arena);
      String8 ext_a = str8_skip_last_dot(str8_skip_last_slash(sorted_a.v[i]));
      String8 ext_b = str8_skip_last_dot(str8_skip_last_slash(sorted_b.v[i]));
      if (str8_match(ext_a, ext_b, 0)) {
        if (str8_match(ext_a, str8_lit("obj"), StringMatchFlag_CaseInsensitive) ||
            str8_match(ext_a, str8_lit("lib"), StringMatchFlag_CaseInsensitive)) {
          String8 data_a = data_from_file_path(temp.arena, sorted_a.v[i]);
          String8 data_b = data_from_file_path(temp.arena, sorted_b.v[i]);
          is_match = str8_match(data_a, data_b, 0);
          if ( ! is_match) {
            Process h;
            h = launch_cmd_linef("dumpbin /all %S /out:a.txt", sorted_a.v[i]);
            process_join(h, max_U64, 0);
            h = launch_cmd_linef("dumpbin /all %S /out:b.txt", sorted_b.v[i]);
            process_join(h, max_U64, 0);
          }
          Assert(is_match);
          if (!is_match) { break; }
        }
      }
      else { InvalidPath; }
      temp_end(temp);
    }
  }
  
  scratch_end(scratch);
  return is_match;
}

internal void
t_infof(char *fmt, ...)
{
  if (g_verbose) {
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 result = push_str8fv(scratch.arena, fmt, args);
    if (g_is_first_print) {
      g_is_first_print = 0;
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "%.*s", str8_varg(result));
    va_end(args);
    scratch_end(scratch);
  }
}

internal void
t_errorf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 result = push_str8fv(scratch.arena, fmt, args);
  if (g_is_first_print) {
    g_is_first_print = 0;
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "%.*s", str8_varg(result));
  va_end(args);
  scratch_end(scratch);
}

internal void
t_help(void)
{
  fprintf(stderr, "--- Help -------------------------------------------------------\n");
  fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
  fprintf(stderr, " Usage: torture [Options] <Input>\n\n");
  fprintf(stderr, " Options:\n");
  fprintf(stderr, "   -list                 Print available test targets\n");
  fprintf(stderr, "   --gui                 Launch debugger with window\n");
  fprintf(stderr, "   -cl:<path>            Override default cl path\n");
  fprintf(stderr, "   -clang:<path>         Override default clang path\n");
  fprintf(stderr, "   -gcc:<path>           Override default gcc path\n");
  fprintf(stderr, "   -linker:<path>        Path to PE/COFF linker\n");
  fprintf(stderr, "   -print_stdout         Print to console stdout and stderr of a run\n");
  fprintf(stderr, "   -out:<path>           Directory path for test outputs (default \"%.*s\")\n", str8_varg(g_out));
  fprintf(stderr, "   -build_only           Build debugger harness without running the tests\n");
  fprintf(stderr, "   -verbose              Enable verbose mode\n");
  fprintf(stderr, "   -help                 Print help menu and exit\n");
  fprintf(stderr, "\nInputs are wildcard expressions. Prefix with ! to skip matches, or + to force-run matches.\n");
  fprintf(stderr, "\nExamples:\n");
  fprintf(stderr, "    torture *                Run all tests\n");
  fprintf(stderr, "    torture bit_array        Run 'bit_array' test\n");
  fprintf(stderr, "    torture !*               Skip all tests\n");
  fprintf(stderr, "    torture * !bit_array     Run all tests but skip 'bit_array'\n");
  fprintf(stderr, "    torture +*               Force-run all tests\n");
}

internal void
t_entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);
  U64 exit_code = max_U64;
  
  U64 dashes_size = 9999;
  U8 *dashes = push_array(scratch.arena, U8, dashes_size);
  MemorySet(dashes, '-', dashes_size);
  
  U64 dots_size = 9999;
  U8 *dots      = push_array(scratch.arena, U8, dots_size);
  MemorySet(dots, '.', dots_size);
  
  char *spaces = "                                                                                      ";
  
#define PrintHeader(p) fprintf(stderr, "--- %s %.*s\n", p, Max((80-4) - (int)strlen(p), 3), dashes)
  
  //
  // Handle -help
  //
  {
    B32 print_help = cmd_line_has_flag(cmdline, str8_lit("help")) ||
      cmd_line_has_flag(cmdline, str8_lit("h"));
    if (print_help) {
      t_help();
      goto exit;
    }
  }
  
  // Gather tests
  {
    for EachIndex(i, test_infos_count) { g_sorted_test_infos[i] = &test_infos[i]; }
    radsort(g_sorted_test_infos, test_infos_count, t_test_info_is_before);
  }
  
  //
  // Handle -list
  //
  {
    if (cmd_line_has_flag(cmdline, str8_lit("list"))) {
      PrintHeader("Tests");
      for EachIndex(i, test_infos_count) {
        fprintf(stdout, "  %.*s\n", str8_varg(g_sorted_test_infos[i]->label));
      }
      abort_self(0);
    }
  }
  
  //
  // Compiler overrides
  //
  g_gui           = cmd_line_has_flag(cmdline, str8_lit("gui"));
  g_cl_path       = cmd_line_string(cmdline, str8_lit("cl"));
  g_clang_path    = cmd_line_string(cmdline, str8_lit("clang"));
  g_gcc_path      = cmd_line_string(cmdline, str8_lit("gcc"));
  g_linker_path   = cmd_line_string(cmdline, str8_lit("linker"));
  
  //
  // Handle -test_data
  //
  {
    g_test_data = cmd_line_string(cmdline, str8_lit("test_data"));
    if (g_test_data.size == 0) {
      g_test_data = str8f(scratch.arena, "%S/build", t_cwd_path());
      //fprintf(stderr, "WARNING: The test data folder path was not specified. Specify the path when running the program, assuming: %.*s --test_data:%.*s\n", str8_varg(cmdline->exe_name), str8_varg(g_test_data));
    }
  }
  
  //
  // Handle optional -target
  //
  U64List targets = {0};
  {
    String8List inputs = {0};
    
    CmdLineOpt *target_opt = 0;
    if (target_opt == 0) { target_opt = cmd_line_opt_from_string(cmdline, str8_lit("target")); }
    if (target_opt == 0) { target_opt = cmd_line_opt_from_string(cmdline, str8_lit("t"));      }
    
    // handle explicit target switch 
    if (target_opt) {
      str8_list_concat_in_place(&inputs, &target_opt->value_strings);
    }
    
    // accept inputs from the command line as target tests to run
    str8_list_concat_in_place(&inputs, &cmdline->inputs);
    
    // no inputs -> print help and exit
    if (inputs.node_count == 0) {
      t_help();
      goto exit;
    }
    
    HashMap hm = {0};
    for EachNode(input_n, String8Node, inputs.first) {
      String8 t = input_n->string;
      
      // parse mode
      typedef enum { Mode_Default, Mode_Skip, Mode_Force, } Mode;
      Mode mode = Mode_Default;
      if      (str8_match_wildcard(t, str8_lit("+*"), 0)) { mode = Mode_Force; t = str8_skip(t, 1); }
      else if (str8_match_wildcard(t, str8_lit("!*"), 0)) { mode = Mode_Skip;  t = str8_skip(t, 1); }
      
      if (str8_find_needle(t, 0, str8_lit("/"), 0) >= t.size) {
        t = str8f(scratch.arena, "*/%S", t);
      }
      
      U64 match_count = 0;
      
      for EachIndex(test_idx, test_infos_count) {
        TestInfo *test_info = g_sorted_test_infos[test_idx];
        
        // match test names
        String8 test_name = t_name_from_test_info(scratch.arena, test_info);
        
        if (str8_match_wildcard(test_name, t, StringMatchFlag_CaseInsensitive)) {
          // TODO(rjf): do we really need to mutate the test infos here? this test won't
          // even run, so I am not sure what setting the bit does - we already collect
          // the tests that we *will* run
#if 0
          // set skip flag
          switch (mode) {
            case Mode_Default: break;
            case Mode_Skip:  g_torture_tests[test_idx]->skip = 1; break;
            case Mode_Force: g_torture_tests[test_idx]->skip = 0; break;
          }
#endif
          
          // append test when not in skipping mode
          if ( ! hash_map_search_string_u64(&hm, test_name)) {
            hash_map_push_string_u64(scratch.arena, &hm, test_name, 1);
            u64_list_push(scratch.arena, &targets, test_idx);
          }
          
          match_count += 1;
        }
      }
      
      if (match_count == 0) {
        fprintf(stderr, "WARNING: no matches found for input: %.*s\n", str8_varg(input_n->string));
      }
    }
  }
  
  g_verbose                     = cmd_line_has_flag(cmdline, str8_lit("verbose"));
  g_redirect_stdout             = !cmd_line_has_flag(cmdline, str8_lit("print_stdout"));
  g_stop_on_first_fail_or_crash = !cmd_line_has_flag(cmdline, str8_lit("keep_going"));
  g_build_only                  = cmd_line_has_flag(cmdline, str8_lit("build_only"));
  g_output_arena                = arena_alloc();
  
  // default options when running under debugger
#if OS_WINDOWS
  if (!cmd_line_has_flag(cmdline, str8_lit("print_stdout")) && IsDebuggerPresent()) {
    g_redirect_stdout = 0;
  }
  
  // automatically close child processes on exit
  {
    HANDLE job_handle = CreateJobObjectA(0, 0);
    AssertAlways(job_handle != 0);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info = { .BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE };
    AssertAlways(SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info)));
    AssertAlways(AssignProcessToJobObject(job_handle, GetCurrentProcess()));
  }
#endif
  
  // Handle -out
  {
    CmdLineOpt *out_opt = cmd_line_opt_from_string(cmdline, str8_lit("out"));
    if (out_opt) {
      if (out_opt->value_strings.node_count == 1) {
        g_out = out_opt->value_string;
      } else {
        fprintf(stderr, "ERROR: -out invalid number of arguments");
      }
    }
  }
  
  //
  // Make Output Directory
  //
  make_directory(g_out);
  if (!folder_path_exists(g_out)) {
    fprintf(stderr, "ERROR: unable to create output directory \"%.*s\"\n", str8_varg(g_out));
    goto exit;
  }
  
  //
  // Clean up output from previous run
  //
  delete_file_at_path(g_stdout_file_name);
  
  //
  // Run tests
  //
  {  
    U64Array target_indices = u64_array_from_list(scratch.arena, &targets);
    
    U64 max_label_size = 0;
    U64 max_group_size = 0;
    for EachIndex(i, target_indices.count) {
      U64 test_idx = target_indices.v[i];
      TestInfo *test_info = g_sorted_test_infos[test_idx];
      max_label_size = Max(max_label_size, test_info->label.size);
      max_group_size = Max(max_group_size, test_info->layer.size);
    }
    
    U64 run_counters[TestStatus_COUNT] = {0};
    U64 max_digit_count  = count_digits_u64(target_indices.count, 10);
    U64 total_time_start = now_time_us();
    
    typedef struct { U64 target_idx, d; } Slowest;
    Slowest slowest[5] = {0};
    for EachElement(i, slowest) { slowest[i].target_idx = max_U64; }
    
    U64List skipped_tests = {0};
    
    for EachIndex(i, target_indices.count) {
      if (i == 0) { PrintHeader("Tests"); }
      
      U64 target_idx = target_indices.v[i];
      TestInfo *test = g_sorted_test_infos[target_idx];
      
      // print run progress
      U64 dots_min = 10;
      U64 dots_count = (max_label_size - test->label.size) + dots_min;
      U64 curr_digit_count = count_digits_u64(i+1, 10);
      int idx_align_space_count = (int)(max_digit_count - curr_digit_count);
      fprintf(stdout, "[%.*s%llu/%llu] ", idx_align_space_count, spaces, (unsigned long long)i+1, (unsigned long long)target_indices.count);
      fprintf(stdout, "%.*s %.*s/ %.*s", str8_varg(test->layer), (int)(max_group_size - test->layer.size), spaces, str8_varg(test->label));
      fprintf(stdout, " %.*s ", (int)dots_count, dots);
      fflush(stdout);
      
      // setup output directory
      g_wdir = str8f(scratch.arena, "%S/%S", g_out, test->label);
      g_wdir = full_path_from_path(scratch.arena, g_wdir);
      
      // delete files from last run in the work directory
      if (folder_path_exists(g_wdir)) {
        t_delete_dir(g_wdir);
      }
      make_directory(g_wdir);
      
      if (!folder_path_exists(g_out)) {
        fprintf(stderr, "ERROR: unable to create output directory for test run %.*s\n", str8_varg(g_wdir));
        continue;
      }
      
      // run test
      U64 run_start_time = now_time_us();
      TestResult result = t_run(cmdline, test, str8_zero());
      U64 run_end_time = now_time_us();
      
      // update
      run_counters[result.status] += 1;
      
      // rjf: map status -> color / string
      char *status_name_cstr = 0;
      char *color_cstr = 0;
      switch(result.status)
      {
        default:
        case TestStatus_Pass: {status_name_cstr = "PASS";  color_cstr = T_GREEN;}break;
        case TestStatus_Fail: {status_name_cstr = "FAIL";  color_cstr = T_RED;}break;
        case TestStatus_Crash:{status_name_cstr = "CRASH"; color_cstr = T_RED;}break;
        case TestStatus_Skip: {status_name_cstr = "SKIP";  color_cstr = T_YELLOW;}break;
      }
      
      // print run status
      fprintf(stdout, "%s%s" T_RESET, color_cstr, status_name_cstr);
      
      if (result.status == TestStatus_Pass) {
        U64      d = run_end_time - run_start_time;
        DateTime t = date_time_from_micro_seconds(d);
        String8  s = string_from_elapsed_time(scratch.arena, t);
        fprintf(stdout, " %.*s", str8_varg(s));
        
        fflush(stdout);
        
        U64 insert_idx = max_U64;
        for EachElement(i, slowest) {
          if (d > slowest[i].d) {
            insert_idx = i;
            break;
          }
        }
        if (insert_idx < ArrayCount(slowest)) {
          for (U64 i = ArrayCount(slowest) - 1; i > insert_idx; i -= 1) {
            slowest[i] = slowest[i - 1];
          }
          slowest[insert_idx].target_idx = target_idx;
          slowest[insert_idx].d          = d;
        }
      }
      fprintf(stdout, "\n");
      
      if (result.status == TestStatus_Fail) {
        fprintf(stdout, "  ERROR: %s:%d: condition: \"%s\"\n", result.fail_file, result.fail_line, result.fail_cond);
      }
      
      if (result.status == TestStatus_Fail || result.status == TestStatus_Crash) {
        if (g_stop_on_first_fail_or_crash) { goto exit; }
      }
      
      if (result.status == TestStatus_Skip) {
        u64_list_push(scratch.arena, &skipped_tests, target_idx);
      }
    }
    U64 total_time_end = now_time_us();
    
    if (target_indices.count > 0 && sum_array_u64(ArrayCount(run_counters), run_counters) > 0) {
      U64     total_time_dt  = total_time_end - total_time_start;
      String8 total_time_str = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(total_time_dt));
      
      fprintf(stderr, "\n");
      PrintHeader("Summary");
      fprintf(stderr, "  Passed   %llu\n", (unsigned long long)run_counters[TestStatus_Pass]);
      fprintf(stderr, "  Failed   %llu\n", (unsigned long long)run_counters[TestStatus_Fail]);
      fprintf(stderr, "  Crashed  %llu\n", (unsigned long long)run_counters[TestStatus_Crash]);
      fprintf(stderr, "  Skipped  %llu\n", (unsigned long long)run_counters[TestStatus_Skip]);
      fprintf(stderr, "  Time     %.*s\n", str8_varg(total_time_str));
      
      U64 slow_count = 0;
      for EachElement(i, slowest) {
        Slowest s = slowest[i];
        if (s.target_idx >= test_infos_count) { break; }
        slow_count += 1;
      }
      
      if (slow_count > 3) {
        U64 label_max = 0;
        U64 group_max = 0;
        for EachElement(i, slowest) {
          Slowest s = slowest[i];
          label_max = Max(g_sorted_test_infos[s.target_idx]->label.size, label_max);
          group_max = Max(g_sorted_test_infos[s.target_idx]->layer.size, group_max);
        }
        
        fprintf(stderr, "  \nSlow Tests\n");
        for EachElement(i, slowest) {
          Slowest s = slowest[i];
          if (s.target_idx >= test_infos_count) { break; }
          TestInfo *test_info = g_sorted_test_infos[i];
          String8 elapsed_time = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(s.d));
          fprintf(stderr, "    %.*s %.*s/ %.*s %.*s %.*s\n",
                  str8_varg(test_info->layer),
                  (int)(group_max - test_info->layer.size), spaces,
                  str8_varg(test_info->label),
                  (int)(label_max - test_info->layer.size) + 4, dots,
                  str8_varg(elapsed_time));
        }
      }
    }
    
    exit_code = run_counters[TestStatus_Fail] + run_counters[TestStatus_Crash];
    exit:;
  }
  
  scratch_end(scratch);
  exit(exit_code);
}

