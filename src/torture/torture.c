// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// command line
global String8 g_stdout_file_name = str8_lit_comp("torture.out");
global String8 g_wdir;
global String8 g_out = str8_lit_comp("torture");
global B32     g_verbose;
global B32     g_redirect_stdout = 1;
global B32     g_stop_on_first_fail_or_crash = 1;
global String8 g_linker;
global String8 g_test_data;

// tests
U64    g_torture_test_count;
T_Test g_torture_tests[0xffffff];

// invoke
global int g_last_exit_code;

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
  ctx->run(scratch.arena, &ctx->result, &test_out);
  if (ctx->result.status == T_RunStatus_Fail) {
    for EachNode(n, String8Node, test_out.first) {
      fprintf(stderr, "%.*s", str8_varg(n->string));
    }
  }
  scratch_end(scratch);
}

internal void
t_run_fail_handler(void *raw_ctx)
{
  T_RunCtx *ctx = raw_ctx;
  ctx->result.status = T_RunStatus_Crash;
  fflush(stdout);
  fflush(stderr);
}

internal T_RunResult
t_run(T_Run run)
{
  T_RunCtx ctx = { .run = run };

  B32 do_safe_call = 1;
#if OS_WINDOWS
  if (IsDebuggerPresent()) {
    do_safe_call = 0;
  }
#endif
  if (do_safe_call) {
    os_safe_call(t_run_caller, t_run_fail_handler, &ctx);
  } else {
    t_run_caller(&ctx);
  }
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

    String8 output = {0};
    t_invoke_(t_cl_path(), str8_zero(), max_U64, scratch.arena, &output);
    AssertAlways(g_last_exit_code == 0);

    String8 needle = str8_lit("Version");
    U64 version_lo = str8_find_needle(output, 0, needle, 0);
    version_lo += needle.size + 1;
    AssertAlways(version_lo < output.size);

    U64 version_hi = str8_find_needle(output, version_lo, str8_lit(" "), 0);
    AssertAlways(version_hi < output.size);

    version = str8_substr(output, r1u64(version_lo, version_hi));
    AssertAlways(version.size > 0);

    local_persist U8 buffer[4096];
    ArenaParams params = { .reserve_size = sizeof(buffer), .commit_size = sizeof(buffer), .optional_backing_buffer = buffer };
    Arena *arena = arena_alloc_(&params);
    version = str8_copy(arena, version);

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
t_invoke_env(String8 exe_path, String8 cmdline, String8List env, U64 timeout, Arena *output_arena, String8 *output_out)
{
  Temp scratch = scratch_begin(&output_arena,1);

  B32 is_ok = 0;

  B32 capture_output = output_out || g_redirect_stdout;

  g_last_exit_code = -1;

  OS_Handle read_pipe_handle = {0}, write_pipe_handle = {0};
  if (capture_output) {
    HANDLE read_pipe, write_pipe;
    SECURITY_ATTRIBUTES at = { .nLength = sizeof(at), .bInheritHandle = 1 };
    if (!CreatePipe(&read_pipe, &write_pipe, &at, 0)) {
      AssertAlways(0 && "failed to create a pipe");
    }
    read_pipe_handle  = (OS_Handle){ .u64[0] = (U64)read_pipe  };
    write_pipe_handle = (OS_Handle){ .u64[0] = (U64)write_pipe };
  }

  // Build Launch Options
  OS_ProcessLaunchParams launch_opts = {
    .path        = g_wdir,
    .inherit_env = 1,
    .env         = env,
    .stdout_file = write_pipe_handle,
    .stderr_file = write_pipe_handle,
    .cmd_line    = lnk_arg_list_parse_windows_rules(scratch.arena, cmdline),
  };
  str8_list_push_front(scratch.arena, &launch_opts.cmd_line, exe_path);

  if (g_verbose) {
    String8 full_cmd_line = str8_list_join(scratch.arena, &launch_opts.cmd_line, &(StringJoin){ .sep = str8_lit(" ") });
    fprintf(stdout, "Command Line: %.*s\n", str8_varg(full_cmd_line));
    fprintf(stdout, "Working Dir:  %.*s\n", str8_varg(g_wdir));
  }

  // invoke exe
  OS_Handle process_handle = os_process_launch(&launch_opts);

  // close handle so last to ReadFile does not block
  os_file_close(write_pipe_handle);

  if ( ! os_handle_match(process_handle, os_handle_zero())) {
    if (capture_output) {
      // capture process output
      String8List  output = {0};
      for (;;) {
        String8 string = os_file_read_cstring(scratch.arena, read_pipe_handle, 0);
        if (string.size == 0) { break; }
        str8_list_push(scratch.arena, &output, string);
      }
      os_file_close(read_pipe_handle);

      if (output_out) {
        *output_out = str8_list_join(output_arena, &output, 0);
      }

      // write to the output file
      if (g_redirect_stdout) {
        os_write_data_list_to_file_path(g_stdout_file_name, output);
      }
    }

    U64 exit_code_u64 = 0;
    if (os_process_join(process_handle, timeout, &exit_code_u64)) {
      g_last_exit_code = (int)exit_code_u64;
      is_ok            = 1;
    } else {
      os_process_kill(process_handle);
    }
  }

  scratch_end(scratch);
  return is_ok;
}

internal B32
t_invoke_(String8 exe_path, String8 cmdline, U64 timeout, Arena *output_arena, String8 *output_out)
{
  return t_invoke_env(exe_path, cmdline, (String8List){0}, timeout, output_arena, output_out);
}

internal B32
t_invoke(String8 exe_path, String8 cmdline, U64 timeout)
{
  return t_invoke_(exe_path, cmdline, timeout, 0, 0);
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
t_test_is_before(void *raw_a, void *raw_b)
{
  T_Test *a = raw_a, *b = raw_b;
  int cmp = str8_compar(str8_cstring(a->group), str8_cstring(b->group), 0);
  if (cmp == 0) {
    cmp = u64_compar(&a->decl_line, &b->decl_line);
  }
  return cmp < 0;
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
      fprintf(stderr, "   -list                 Print available test targets and exit\n");
      fprintf(stderr, "   -out:{path}           Directory path for test outputs (default \"%.*s\")\n", str8_varg(g_out));
      fprintf(stderr, "   -verbose              Enable verbose mode\n");
      fprintf(stderr, "   -print_stdout         Print to console stdout and stderr of a run\n");
      fprintf(stderr, "   -help                 Print help menu and exit\n");
      os_abort(0);
    }
  }

#if 0
  //
  // config
  //
  {
    DateTime start_time_uni = os_now_universal_time();
    DateTime start_time_loc = os_local_time_from_universal(&start_time_uni);
    PrintHeader("Config");
    fprintf(stderr, "  Build   %s\n", BUILD_TITLE_STRING_LITERAL);
    fprintf(stderr, "  Start   %.*s\n", str8_varg(string_from_date_time(scratch.arena, &start_time_loc)));
    fprintf(stderr, "\n");
    fprintf(stderr, "  Tools\n");
#if OS_WINDOWS
    AssertAlways(t_cl_path().size && t_cl_version().size);
    fprintf(stderr, "    MSVC    %.*s\n", str8_varg(t_cl_version()));
    fprintf(stderr, "            %.*s\n", str8_varg(t_cl_path()));
#endif
    fprintf(stderr, "    radlink %.*s\n", str8_varg(t_radlink_version()));
    fprintf(stderr, "            %.*s\n", str8_varg(t_radlink_path()));
    fprintf(stderr, "    radbin  %.*s\n", str8_varg(t_radbin_version()));
    fprintf(stderr, "            %.*s\n", str8_varg(t_radbin_path()));
    fprintf(stderr, "\n");
  }
#endif

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
      if (target_opt->value_strings.node_count > 0) {
        for EachNode(pattern_n, String8Node, target_opt->value_strings.first) {
          B32 do_namespace = str8_find_needle(pattern_n->string, 0, str8_lit("::"), 0) < pattern_n->string.size;
          for EachIndex(test_idx, g_torture_test_count) {
            String8 name = str8_cstring(g_torture_tests[test_idx].label);
            if (do_namespace) {
              name = str8f(scratch.arena, "%s::%s", g_torture_tests[test_idx].group, g_torture_tests[test_idx].label);
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
    radsort(g_torture_tests, g_torture_test_count, t_test_is_before);

    U64  target_indices_count;
    U64 *target_indices;
    if (target.node_count == 0) {
      target_indices_count = g_torture_test_count;
      target_indices       = push_array(scratch.arena, U64, g_torture_test_count);
      for EachIndex(i, target_indices_count) { target_indices[i] = i; }
    } else {
      target_indices_count = 0;
      target_indices       = push_array(scratch.arena, U64, target.node_count);

      for EachNode(target_n, String8Node, target.first) {
        B32 is_target_unknown = 1;
        for EachIndex(i, g_torture_test_count) {
          if (str8_match(str8_cstring(g_torture_tests[i].label), target_n->string, 0)) {
            target_indices[target_indices_count++] = i;
            is_target_unknown = 0;
            break;
          }
        }
        if (is_target_unknown) {
          fprintf(stderr, "ERROR: unknown target \"%.*s\"\n", str8_varg(target_n->string));
        }
      }
    }

    U64 max_label_size = 0;
    U64 max_group_size = 0;
    for EachIndex(i, target_indices_count) {
      U64 test_idx = target_indices[i];
      max_label_size = Max(max_label_size, cstring8_length((U8*)g_torture_tests[test_idx].label));
      max_group_size = Max(max_group_size, cstring8_length((U8*)g_torture_tests[test_idx].group));
    }

    PrintHeader("Tests");
    U64 pass_count  = 0;
    U64 fail_count  = 0;
    U64 crash_count = 0;
    U64 max_digit_count = count_digits_u64(target_indices_count, 10);
    U64 total_time_start = os_now_microseconds();
    typedef struct { U64 target_idx, d; } Slowest;
    Slowest slowest[5] = {0};
    for EachElement(i, slowest) { slowest[i].target_idx = max_U64; }
    for EachIndex(i, target_indices_count) {
      U64 target_idx = target_indices[i];

      // print run progress
      U64 dots_min = 10;
      U64 dots_count = (max_label_size - cstring8_length((U8*)g_torture_tests[target_idx].label)) + dots_min;
      U64 curr_digit_count = count_digits_u64(i+1, 10);
      int idx_align_space_count = (int)(max_digit_count - curr_digit_count);
      fprintf(stdout, "[%.*s%I64u/%I64u] ", idx_align_space_count, spaces, i+1, target_indices_count);
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
      T_RunResult result = t_run(g_torture_tests[target_idx].r);
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
    fprintf(stderr, "  Passed   %u\n", (int)pass_count);
    fprintf(stderr, "  Failed   %u\n", (int)fail_count);
    fprintf(stderr, "  Crashed  %u\n", (int)crash_count);
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

