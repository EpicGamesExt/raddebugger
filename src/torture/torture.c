// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// command line
global String8      g_stdout_file_name = str8_lit_comp("torture.out");
global String8      g_wdir;
global String8      g_out = str8_lit_comp("torture");
global B32          g_verbose;
global B32          g_redirect_stdout = 1;
global B32          g_stop_on_first_fail_or_crash = 1;
global String8      g_linker;
global String8      g_test_data;

// tests
U64    g_torture_test_count;
T_Test g_torture_tests[0xffffff];

// invoke
global U64      g_last_exit_code;
global Arena   *g_output_arena;
global String8  g_output;
global String8  g_errors;

internal String8
t_test_name_from_idx(Arena *arena, U64 test_idx)
{
  return str8f(arena, "%s::%s", g_torture_tests[test_idx].group, g_torture_tests[test_idx].label);
}

internal char *
t_string_from_result(T_RunStatus v)
{
  switch (v) {
  case T_RunStatus_Fail:  return "FAIL";
  case T_RunStatus_Crash: return "CRASH";
  case T_RunStatus_Pass:  return "PASS";
  }
  return 0;
}

internal void
t_break_if_debugger_present(void)
{
#if OS_WINDOWS
  if(IsDebuggerPresent())
  {
    DebugBreak();
  }
#endif
}

internal B32
t_write_file_list(String8 name, String8List data)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  B32 is_written = os_write_data_list_to_file_path(path, data);
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
  String8 data = os_data_from_file_path(arena, path);
  scratch_end(scratch);
  return data;
}

internal B32
t_delete_file(String8 name)
{
  Temp scratch = scratch_begin(0,0);
  String8 path = t_make_file_path(scratch.arena, name);
  B32 is_deleted = os_delete_file_at_path(path);
  scratch_end(scratch);
  return is_deleted;
}

internal void
t_delete_dir(String8 path)
{
  Temp scratch = scratch_begin(0,0);

  OS_FileIter *file_iter = os_file_iter_begin(scratch.arena, path, 0);
  for (;;) {
    OS_FileInfo info = {0};
    if ( ! os_file_iter_next(scratch.arena, file_iter, &info)) { break; }

    if (info.props.flags & FilePropertyFlag_IsFolder) {
      t_delete_dir(str8f(scratch.arena, "%S/%S", path, info.name));
      continue;
    }

    String8 file_path = str8f(scratch.arena, "%S/%S", path, info.name);
    if ( ! os_delete_file_at_path(file_path)) {
      fprintf(stderr, "ERROR: unable to delete file %.*s\n", str8_varg(file_path));
    }
  }
  os_file_iter_end(file_iter);

  // TODO: delete directories

  scratch_end(scratch);
}

internal String8
t_make_file_path(Arena *arena, String8 name)
{
  return push_str8f(arena, "%S\\%S", g_wdir, name);
}

internal B32
t_make_dir(String8 name)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = os_make_directory(t_make_file_path(scratch.arena, name));
  scratch_end(scratch);
  return is_ok;
}

internal void
t_run_caller(void *raw_ctx)
{
  Temp scratch = scratch_begin(0,0);
  T_RunCtx *ctx = raw_ctx;  
  String8List test_out = {0};
  ctx->result.status = T_RunStatus_Pass;
  ctx->run(scratch.arena, ctx->user_data, &ctx->result, &test_out);
  if (ctx->result.status == T_RunStatus_Fail || ctx->result.status == T_RunStatus_Crash) {
    for EachNode(n, String8Node, test_out.first) {
      fprintf(stderr, "%.*s", str8_varg(n->string));
    }
    if (g_errors.size) {
      fprintf(stderr, "STDERR:\n", str8_varg(g_errors));
    }
    if (g_output.size) {
      fprintf(stderr, "STDOUT:\n", str8_varg(g_output));
    }
  }
  scratch_end(scratch);
}

internal T_RunResult
t_run(T_Run run, String8 user_data)
{
  T_RunCtx ctx = { .run = run, .user_data = user_data, .result.status = T_RunStatus_Fail };
  t_run_caller(&ctx);
  fflush(stdout);
  fflush(stderr);
  return ctx.result;
}

internal String8
t_radbin_path(void)
{
  local_persist String8 path = {0};
  if (path.size == 0) {
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
#if OS_WINDOWS
    path = os_full_path_from_path(arena, str8_lit("radbin.exe"));
#else
    path = os_full_path_from_path(arena, str8_lit("radbin"));
#endif
    AssertAlways(path.size);
  }
  return path;
}

internal String8
t_cl_path(void)
{
  local_persist String8 path = {0};
  if (path.size == 0) {
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
#if OS_WINDOWS
    wchar_t full_cl_path[MAX_PATH];
    DWORD full_cl_path_size = SearchPathW(0, L"cl.exe", 0, ArrayCount(full_cl_path), full_cl_path, 0);
    path = str8_from_16(arena, str16((U16*)full_cl_path, full_cl_path_size));
#else
    path = str8_lit("cl");
#endif
    AssertAlways(path.size);
  }
  return path;
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
  if (path.size == 0) {
    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
#if OS_WINDOWS
    path = os_full_path_from_path(arena, str8_lit("radlink.exe"));
#else
    path = os_full_path_from_path(arena, str8_lit("radlink"));
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
#if OS_WINDOWS
    path = os_full_path_from_path(arena, str8_lit("raddbg.exe"));
#else
    path = os_full_path_from_path(arena, str8_lit("raddbg"));
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
    String8 cwd = os_get_current_path(scratch.arena);
    cwd = str8_chop_last_slash(cwd);
    cwd = str8f(scratch.arena, "%S", cwd);
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
  arena_pop_to(g_output_arena, 0);
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

  OS_Handle read_capture_handles [ArrayCount(captures_win32)] = {0};
  OS_Handle write_capture_handles[ArrayCount(captures_win32)] = {0};
  for EachElement(i, captures_win32) { read_capture_handles[i]  = (OS_Handle){ (U64)captures_win32[i].read_pipe_handle  }; }
  for EachElement(i, captures_win32) { write_capture_handles[i] = (OS_Handle){ (U64)captures_win32[i].write_pipe_handle }; }
#else
# error NotImplemented
#endif

  // Build Launch Options
  OS_ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .env         = env,
    .stdout_file = write_capture_handles[stdout_idx],
    .stderr_file = write_capture_handles[stderr_idx],
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  str8_list_push_front(scratch.arena, &launch_opts.cmd_line, exe_path);

  // invoke exe
  OS_Handle process_handle = os_process_launch(&launch_opts);
  if (os_handle_match(process_handle, os_handle_zero())) { goto exit; }

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

      for EachElement(i, captures_win32) {
        while (captures_win32[i].state == Win32CaptureState_Null) {
          // init overlapped so when child writes to capture buffer this event is signaled
          AssertAlways(ResetEvent(captures_win32[i].event));
          MemoryZeroStruct(&captures_win32[i].overlapped);
          captures_win32[i].overlapped.hEvent = captures_win32[i].event;

          // begin overlapped read
          DWORD read_size = 0;
          if (ReadFile(captures_win32[i].read_pipe_handle, captures_win32[i].buffer, captures_win32[i].buffer_size, &read_size, &captures_win32[i].overlapped)) {
            if (read_size > 0) {
              String8 string      = str8(captures_win32[i].buffer, read_size);
              String8 string_copy = str8_copy(scratch.arena, string);
              str8_list_push(scratch.arena, captures_win32[i].parts, string_copy);
            } else {
              captures_win32[i].state = Win32CaptureState_EOF;
            }
          } else {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
              captures_win32[i].state = Win32CaptureState_Pending;
            } else {
              AssertAlways(error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE);
              captures_win32[i].state = Win32CaptureState_EOF;
            }
            break;
          }
        }

        if (captures_win32[i].state == Win32CaptureState_Pending) {
          // event now should signal whenever pipe has data to read
          captures_win32[i].wait_idx = wait_handle_count++;
          wait_handles[captures_win32[i].wait_idx] = captures_win32[i].event;
        } else {
          captures_win32[i].wait_idx = max_U64;
        }
      }

      // exit if there are no handles
      if (wait_handle_count == 0) { break; }

      // compute wait time
      DWORD wait_ms = INFINITE;
      if (timeout_us != max_U64) {
        U64 now_us = os_now_microseconds(); 
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
# error NotImplemented
#endif


  // update output global
  g_output = str8_list_join(g_output_arena, &stdout_parts, 0);
  g_errors = str8_list_join(g_output_arena, &stderr_parts, 0);

  // write to the output file
  if (g_redirect_stdout) {
    os_write_data_to_file_path(g_stdout_file_name, g_output);
  }

  is_ok = 1; // process was launched (does not mean exited successfully)
  exit:;
  for EachElement(i, read_capture_handles)  { os_file_close(read_capture_handles[i]);  }
  for EachElement(i, write_capture_handles) { os_file_close(write_capture_handles[i]); }
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

internal String8
t_chop_line(String8 *output)
{
  U64     new_line_pos = str8_find_needle(*output, 0, str8_lit("\n"), 0);
  String8 line         = str8_prefix(*output, new_line_pos);
  if (str8_ends_with(line, str8_lit("\r"), 0)) {
    line = str8_chop(line, 1);
  }
  *output = str8_skip(*output, new_line_pos + 1);
  return line;
}

internal B32
t_match_line(String8 *output, String8 expected_line)
{
  String8 before_chop = *output;
  String8 line        = t_chop_line(output);
  B32     is_match    = str8_match(line, expected_line, 0);
  if ( ! is_match) {
    *output = before_chop;
  }
  return is_match;
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

internal int
t_test_compar(const void *raw_a, const void *raw_b)
{
  const T_Test *a = raw_a, *b = raw_b;
  int cmp = str8_compar(str8_cstring(a->group), str8_cstring(b->group), 0);
  if (cmp == 0) {
    cmp = u64_compar(&a->decl_line, &b->decl_line);
  }
  return cmp;
}

internal int
t_test_is_before(void *raw_a, void *raw_b)
{
  return t_test_compar(raw_a, raw_b) < 0;
}

internal String8List
t_file_paths_from_dir(Arena *arena, String8 dir)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List dirs = {0};
  String8List files = {0};

  OS_FileIter *iter = os_file_iter_begin(scratch.arena, dir, 0);
  OS_FileInfo file;
  while (os_file_iter_next(scratch.arena, iter, &file)) {
    if (file.props.flags & FilePropertyFlag_IsFolder) {
      str8_list_pushf(scratch.arena, &dirs, "%S/%S", dir, file.name);
    } else {
      str8_list_pushf(arena, &files, "%S/%S", dir, file.name);
    }
  }
  os_file_iter_end(iter);

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
          String8 data_a = os_data_from_file_path(temp.arena, sorted_a.v[i]);
          String8 data_b = os_data_from_file_path(temp.arena, sorted_b.v[i]);
          is_match = str8_match(data_a, data_b, 0);
          if ( ! is_match) {
            OS_Handle h;
            h = os_cmd_line_launchf("dumpbin /all %S /out:a.txt", sorted_a.v[i]);
            os_process_join(h, max_U64, 0);
            h = os_cmd_line_launchf("dumpbin /all %S /out:b.txt", sorted_b.v[i]);
            os_process_join(h, max_U64, 0);
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
t_entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

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
      PrintHeader("Help");
      fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
      fprintf(stderr, " Usage: torture [Options] [Files]\n\n");
      fprintf(stderr, " Options:\n");
      fprintf(stderr, "   -linker:{path}        Path to PE/COFF linker\n");
      fprintf(stderr, "   -target:{name[,name]} Selects targets to test\n");
      fprintf(stderr, "   -skip:{name[,name]}   Selects targets to skip\n");
      fprintf(stderr, "   -list                 Print available test targets and exit\n");
      fprintf(stderr, "   -out:{path}           Directory path for test outputs (default \"%.*s\")\n", str8_varg(g_out));
      fprintf(stderr, "   -verbose              Enable verbose mode\n");
      fprintf(stderr, "   -print_stdout         Print to console stdout and stderr of a run\n");
      fprintf(stderr, "   -help                 Print help menu and exit\n");
      os_abort(0);
    }
  }

  // register debugger tests
  String8 test_folder_path = str8f(scratch.arena, "%S/torture/dbg_tests", t_src_path());
  t_dbg_register_script_tests(scratch.arena, test_folder_path);

  //
  // Handle -list
  //
  {
    if (cmd_line_has_flag(cmdline, str8_lit("list"))) {
      PrintHeader("Tests");
      for EachIndex(i, g_torture_test_count) {
        fprintf(stdout, "  %s\n", g_torture_tests[i].label);
      }
      os_abort(0);
    }
  }

  //
  // Handle -linker
  //
  {
    CmdLineOpt *linker_opt = cmd_line_opt_from_string(cmdline, str8_lit("linker"));
    if (linker_opt == 0) {
      linker_opt = cmd_line_opt_from_string(cmdline, str8_lit("l"));
    }
    if (linker_opt) {
      if (linker_opt->value_strings.node_count == 1) {
        g_linker = linker_opt->value_string;
      } else {
        fprintf(stderr, "ERROR: -linker has invalid number of arguments\n");
        os_abort(1);
      }
    } else {
      // assume default linker
      g_linker = str8_lit("radlink");
    }
  }

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
  String8List target = cmdline->inputs;
  {
    CmdLineOpt *target_opt = cmd_line_opt_from_string(cmdline, str8_lit("target"));
    if (target_opt == 0) {
      target_opt = cmd_line_opt_from_string(cmdline, str8_lit("t"));
    }
    if (target_opt) {
      HashTable *ht = hash_table_init(scratch.arena, g_torture_test_count*2);

      String8List targets = target_opt->value_strings;
      str8_list_concat_in_place(&targets, &cmdline->inputs);
      
      if (targets.node_count > 0) {
        for EachNode(pattern_n, String8Node, targets.first) {
          B32 do_namespace = str8_find_needle(pattern_n->string, 0, str8_lit("::"), 0) < pattern_n->string.size;
          for EachIndex(test_idx, g_torture_test_count) {
            String8 name = str8_cstring(g_torture_tests[test_idx].label);
            if (do_namespace) {
              name = t_test_name_from_idx(scratch.arena, test_idx);
            }
            if (str8_match_wildcard(name, pattern_n->string, 0)) {
              if ( ! hash_table_search_string(ht, name)) {
                hash_table_push_string_raw(scratch.arena, ht, name, 0);
                str8_list_push(scratch.arena, &target, str8_cstring(g_torture_tests[test_idx].label));
              }
            }
          }
        }

        if (ht->count == 0) {
          fprintf(stderr, "ERROR: -target matches not found for the following patterns: ");
          for EachNode(n, String8Node, target_opt->value_strings.first) {
            fprintf(stderr, "\"%.*s\"\n", str8_varg(n->string));
          }
          os_abort(1);
        }
      } else {
        fprintf(stderr, "ERROR: -target has invalid number of arguments\n");
      }
    }
  }

  g_verbose                     = cmd_line_has_flag(cmdline, str8_lit("verbose"));
  g_redirect_stdout             = !cmd_line_has_flag(cmdline, str8_lit("print_stdout"));
  g_stop_on_first_fail_or_crash = !cmd_line_has_flag(cmdline, str8_lit("keep_going"));
  g_output_arena                = arena_alloc();

  // default options when running under debugger
#if OS_WINDOWS
  if (!cmd_line_has_flag(cmdline, str8_lit("print_stdout")) && IsDebuggerPresent()) {
    g_redirect_stdout = 0;
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
  os_make_directory(g_out);
  if (!os_folder_path_exists(g_out)) {
    fprintf(stderr, "ERROR: unable to create output directory \"%.*s\"\n", str8_varg(g_out));
    os_abort(1);
  }

  //
  // Clean up output from previous run
  //
  os_delete_file_at_path(g_stdout_file_name);

  //
  // Run tests
  //
  {
    //radsort(g_torture_tests, g_torture_test_count, t_test_is_before);
    qsort(g_torture_tests, g_torture_test_count, sizeof(*g_torture_tests), t_test_compar);

    U64List target_indices_list = {0};
    if (target.node_count == 0) {
      for EachIndex(i, g_torture_test_count) { u64_list_push(scratch.arena, &target_indices_list, i); }
    } else {
      for EachNode(target_n, String8Node, target.first) {
        B32 is_target_unknown = 1;
        for EachIndex(i, g_torture_test_count) {
          if (str8_match(str8_cstring(g_torture_tests[i].label), target_n->string, 0)) {
            u64_list_push(scratch.arena, &target_indices_list, i);
            is_target_unknown = 0;
            break;
          }
        }
        if (is_target_unknown) {
          fprintf(stderr, "ERROR: unknown target \"%.*s\"\n", str8_varg(target_n->string));
        }
      }
    }

    //
    // -skip
    //
    U64List final_target_list = {0};
    CmdLineOpt *skip_opt   = cmd_line_opt_from_string(cmdline, str8_lit("skip"));
    CmdLineOpt *s_opt      = cmd_line_opt_from_string(cmdline, str8_lit("s"));
    String8List skip_list = skip_opt ? skip_opt->value_strings : s_opt ? s_opt->value_strings : (String8List){0};
    for EachNode(n, U64Node, target_indices_list.first) {
      // should test be skipped?
      B32 include_test = 1;
      String8 test_name = t_test_name_from_idx(scratch.arena, n->data);
      for EachNode(pattern_n, String8Node, skip_list.first) {
        if (str8_match_wildcard(test_name, pattern_n->string, 0)) {
          include_test = 0;
          break;
        }
      }

      if (include_test) {
        u64_list_push(scratch.arena, &final_target_list, n->data);
      }
    }

    U64 skip_count = target_indices_list.count - final_target_list.count;
    U64Array target_indices = u64_array_from_list(scratch.arena, &final_target_list);

    U64 max_label_size = 0;
    U64 max_group_size = 0;
    for EachIndex(i, target_indices.count) {
      U64 test_idx = target_indices.v[i];
      max_label_size = Max(max_label_size, cstring8_length((U8*)g_torture_tests[test_idx].label));
      max_group_size = Max(max_group_size, cstring8_length((U8*)g_torture_tests[test_idx].group));
    }

    PrintHeader("Tests");
    U64 pass_count  = 0;
    U64 fail_count  = 0;
    U64 crash_count = 0;
    U64 max_digit_count = count_digits_u64(target_indices.count, 10);
    U64 total_time_start = os_now_microseconds();
    typedef struct { U64 target_idx, d; } Slowest;
    Slowest slowest[5] = {0};
    for EachElement(i, slowest) { slowest[i].target_idx = max_U64; }
    for EachIndex(i, target_indices.count) {
      U64 target_idx = target_indices.v[i];

      // print run progress
      U64 dots_min = 10;
      U64 dots_count = (max_label_size - cstring8_length((U8*)g_torture_tests[target_idx].label)) + dots_min;
      U64 curr_digit_count = count_digits_u64(i+1, 10);
      int idx_align_space_count = (int)(max_digit_count - curr_digit_count);
      fprintf(stdout, "[%.*s%I64u/%I64u] ", idx_align_space_count, spaces, i+1, target_indices.count);
      fprintf(stdout, "%s %.*s:: %s", g_torture_tests[target_idx].group, (int)(max_group_size - cstring8_length((U8*)g_torture_tests[target_idx].group)), spaces, g_torture_tests[target_idx].label);
      fprintf(stdout, " %.*s ", (int)dots_count, dots);

      // setup output directory
      g_wdir = push_str8f(scratch.arena, "%S\\%s", g_out, g_torture_tests[target_idx].label);
      g_wdir = os_full_path_from_path(scratch.arena, g_wdir);

      // delete files from last run in the work directory
      if (os_folder_path_exists(g_wdir)) {
        t_delete_dir(g_wdir);
      }
      os_make_directory(g_wdir);

      if (!os_folder_path_exists(g_out)) {
        fprintf(stderr, "ERROR: unable to create output directory for test run %.*s\n", str8_varg(g_wdir));
        continue;
      }

      // run test
      U64 run_start_time = os_now_microseconds();
      T_RunResult result = t_run(g_torture_tests[target_idx].r, g_torture_tests[target_idx].user_data);
      U64 run_end_time = os_now_microseconds();

      // print result
      if (result.status == T_RunStatus_Pass) {
        fprintf(stdout, "\x1b[32m" "%s" "\x1b[0m", t_string_from_result(result.status));
        pass_count += 1;
      } else if (result.status == T_RunStatus_Fail) {
        fprintf(stdout, "\x1b[31m" "%s" "\x1b[0m", t_string_from_result(result.status));
        fail_count += 1;
      } else if (result.status == T_RunStatus_Crash) {
        fprintf(stdout, "\x1b[33m" "%s" "\x1b[0m", t_string_from_result(result.status));
        crash_count += 1;
      }

      if (result.status == T_RunStatus_Pass) {
        U64      d = run_end_time - run_start_time;
        DateTime t = date_time_from_micro_seconds(d);
        String8  s = string_from_elapsed_time(scratch.arena, t);
        fprintf(stdout, " %.*s", str8_varg(s));

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

      if (result.status == T_RunStatus_Fail) {
        fprintf(stdout, "  ERROR: %s:%d: condition: \"%s\"\n", result.fail_file, result.fail_line, result.fail_cond);
      }

      if (result.status == T_RunStatus_Fail || result.status == T_RunStatus_Crash) {
        if (g_stop_on_first_fail_or_crash) { goto exit; }
      }
    }
    fprintf(stderr, "\n");
    U64 total_time_end = os_now_microseconds();

    PrintHeader("Summary");
    U64 total_time_dt = total_time_end - total_time_start;
    String8 total_time_str = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(total_time_dt));
    fprintf(stderr, "  Passed   %llu\n", pass_count);
    fprintf(stderr, "  Failed   %llu\n", fail_count);
    fprintf(stderr, "  Crashed  %llu\n", crash_count);
    fprintf(stderr, "  Skipped  %llu\n", skip_count);
    fprintf(stderr, "  Time     %.*s\n", str8_varg(total_time_str));
    fprintf(stderr, "\n");

    {
      fprintf(stderr, "  Slow Tests\n");
      U64 label_max = 0;
      U64 group_max = 0;
      for EachElement(i, slowest) {
        Slowest s = slowest[i];
        if (s.target_idx >= g_torture_test_count) { break; }
        label_max = Max(strlen(g_torture_tests[s.target_idx].label), label_max);
        group_max = Max(strlen(g_torture_tests[s.target_idx].group), group_max);
      }

      for EachElement(i, slowest) {
        Slowest s = slowest[i];
        if (s.target_idx >= g_torture_test_count) { break; }
        String8 elapsed_time = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(s.d));
        fprintf(stderr, "    %s %.*s:: %s %.*s %.*s\n",
                g_torture_tests[s.target_idx].group,
                (int)(group_max - strlen(g_torture_tests[s.target_idx].group)), spaces,
                g_torture_tests[s.target_idx].label,
                (int)(label_max - strlen(g_torture_tests[s.target_idx].label)) + 4, dots,
                str8_varg(elapsed_time));
      }

      fprintf(stderr, "\n");
    }

    exit:;
    if (fail_count + crash_count != 0) {
      fflush(stdout);
      os_abort(fail_count + crash_count);
    }
  }

  scratch_end(scratch);
}
