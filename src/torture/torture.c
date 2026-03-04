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

internal String8
t_make_file_path(Arena *arena, String8 name)
{
  return push_str8f(arena, "%S\\%S", g_wdir, name);
}

internal void
t_run_caller(void *raw_ctx)
{
  T_RunCtx *ctx = raw_ctx;  
  ctx->result = ctx->run();
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
  T_RunCtx ctx = {0};
  ctx.run      = run;
  os_safe_call(t_run_caller, t_run_fail_handler, &ctx);
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

internal B32
t_invoke_(String8 exe_path, String8 cmdline, U64 timeout, Arena *output_arena, String8 *output_out)
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

    os_process_detach(process_handle);
  }

  scratch_end(scratch);
  return is_ok;
}

internal B32
t_invoke(String8 exe_path, String8 cmdline, U64 timeout)
{
  return t_invoke_(exe_path, cmdline, timeout, 0, 0);
}

internal B32
t_match_line(String8 *output, String8 expected_line)
{
  U64     new_line_pos = str8_find_needle(*output, 0, str8_lit("\n"), 0);
  String8 line         = str8_prefix(*output, new_line_pos);
  if (str8_ends_with(line, str8_lit("\r"), 0)) {
    line = str8_chop(line, 1);
  }

  B32 is_match = str8_match(line, expected_line, 0);
  if (is_match) {
    *output = str8_skip(*output, new_line_pos + 1);
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

  //
  // Handle -help
  //
  {
    B32 print_help = cmd_line_has_flag(cmdline, str8_lit("help")) ||
                     cmd_line_has_flag(cmdline, str8_lit("h"));
     if (print_help) {
      fprintf(stderr, "--- Help -----------------------------------------------------------------------\n");
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

  //
  // Handle -list
  //
  {
    if (cmd_line_has_flag(cmdline, str8_lit("list"))) {
      fprintf(stdout, "--- Tests --------------------------------------------------------------------\n");
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
  // Handle optional -target
  //
  String8List target = cmdline->inputs;
  {
    CmdLineOpt *target_opt = cmd_line_opt_from_string(cmdline, str8_lit("target"));
    if (target_opt == 0) {
      target_opt = cmd_line_opt_from_string(cmdline, str8_lit("t"));
    }
    if (target_opt) {
      if (target_opt->value_strings.node_count > 0) {
        str8_list_concat_in_place(&target, &target_opt->value_strings);
      } else {
        fprintf(stderr, "ERROR: -target has invalid number of arguments\n");
      }
    }
  }

  g_verbose                     = cmd_line_has_flag(cmdline, str8_lit("verbose"));
  g_redirect_stdout             = !cmd_line_has_flag(cmdline, str8_lit("print_stdout"));
  g_stop_on_first_fail_or_crash = !cmd_line_has_flag(cmdline, str8_lit("keep_going"));

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

    U64 max_label_size = 0;
    U64 max_group_size = 0;
    for EachIndex(i, g_torture_test_count) {
      max_label_size = Max(max_label_size, cstring8_length((U8*)g_torture_tests[i].label));
      max_group_size = Max(max_group_size, cstring8_length((U8*)g_torture_tests[i].group));
    }

    U64 dots_min = 10;
    U64 dots_size = max_label_size+dots_min;
    U8 *dots      = push_array(scratch.arena, U8, dots_size);
    MemorySet(dots, '.', dots_size);

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

    U64 pass_count  = 0;
    U64 fail_count  = 0;
    U64 crash_count = 0;
    for EachIndex(i, target_indices_count) {
      U64 target_idx = target_indices[i];

      // print run progress
      U64 dots_count = (max_label_size - cstring8_length((U8*)g_torture_tests[target_idx].label)) + dots_min;
      char *spaces = "                                                                                      ";
      fprintf(stdout, "[%2I64u/%2I64u] ", i+1, target_indices_count);
      fprintf(stdout, "(%s) %.*s%s", g_torture_tests[target_idx].group, (int)(max_group_size - cstring8_length((U8*)g_torture_tests[target_idx].group)), spaces, g_torture_tests[target_idx].label);
      fprintf(stdout, "%.*s", (int)dots_count, dots);

      // setup output directory
      g_wdir = push_str8f(scratch.arena, "%S\\%s", g_out, g_torture_tests[target_idx].label);
      g_wdir = os_full_path_from_path(scratch.arena, g_wdir);
      os_make_directory(g_wdir);
      if (!os_folder_path_exists(g_out)) {
        fprintf(stderr, "ERROR: unable to create output directory for test run %.*s\n", str8_varg(g_wdir));
        continue;
      }

      // run test
      T_RunResult result = t_run(g_torture_tests[target_idx].r);

      // print result
      if (result.status == T_RunStatus_Pass) {
        fprintf(stdout, "\x1b[32m" "%s" "\x1b[0m" "\n", t_string_from_result(result.status));
        pass_count += 1;
      } else if (result.status == T_RunStatus_Fail) {
        fprintf(stdout, "\x1b[31m" "%s" "\x1b[0m" "\n", t_string_from_result(result.status));
        fail_count += 1;
      } else if (result.status == T_RunStatus_Crash) {
        fprintf(stdout, "\x1b[33m" "%s" "\x1b[0m" "\n", t_string_from_result(result.status));
        crash_count += 1;
      }

      if (result.status == T_RunStatus_Fail) {
        fprintf(stdout, "  ERROR: %s:%d: condition: \"%s\"\n", result.fail_file, result.fail_line, result.fail_cond);
      }

      if (result.status == T_RunStatus_Fail || result.status == T_RunStatus_Crash) {
        if (g_stop_on_first_fail_or_crash) { goto exit; }
      }
    }

    fprintf(stdout, "*** Passed: %I64u, Failed: %I64u, Crashed: %I64u ***\n", pass_count, fail_count, crash_count);

    exit:;
    if (fail_count + crash_count != 0) {
      fflush(stdout);
      os_abort(fail_count + crash_count);
    }
  }

  scratch_end(scratch);
}

