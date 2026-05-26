// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

#define T_RESET  "\x1b[0m"
#define T_RED    "\x1b[31m"
#define T_GREEN  "\x1b[32m"
#define T_YELLOW "\x1b[33m"
#define T_BLUE   "\x1b[34m"

typedef struct
{
  TestInfo *test;
  CmdLine *cmdline;
  String8 user_data;
  TestResult result;
} T_RunCtx;

#define TEST(name) Test(name)
#define SKIP(name) SkippedTest(name)
#define T_Ok(c) TestCheck(c)

#define T_MatchLinef(out, ...) T_Ok(t_match_linef(out, __VA_ARGS__))
#define t_outf(...) str8_list_pushf(arena, ctx->test_out, ## __VA_ARGS__)

////////////////////////////////////////////////////////////////

#define TIMEOUT_US(x)  (x)
#define TIMEOUT_MS(x)  TIMEOUT_US((x)*1000ull)
#define TIMEOUT_SEC(x) TIMEOUT_MS((x)*1000ull)

#define ENDT_US(x)  (now_time_us() + (x))
#define ENDT_MS(x)  TIMEOUT_US((x)*1000ull)
#define ENDT_SEC(x) TIMEOUT_MS((x)*1000ull)

////////////////////////////////////////////////////////////////

// output directory
internal B32 t_write_file_list(String8 name, String8List data);
internal B32 t_write_file(String8 name, String8 data);
internal String8 t_read_file(Arena *arena, String8 name);
internal B32     t_delete_file(String8 name);
internal String8 t_make_file_path(Arena *arena, String8 name);

// test runner
internal void       t_run_caller(void *raw_ctx);
internal void       t_run_fail_handler(void *raw_ctx);
internal TestResult t_run(CmdLine *cmdline, TestInfo *test, String8 user_data);

// tools
internal String8 t_radbin_path(void);
internal String8 t_cl_path(void);
internal String8 t_clang_path(void);
internal String8 t_gcc_path(void);
internal String8 t_radlink_path(void);
internal String8 t_cwd_path(void);
internal String8 t_src_path(void);

internal B32 t_invoke(String8 exe, String8 cmdline, U64 timeout);
internal B32 t_invoke_cl(char *fmt, ...);
internal B32 t_invoke_linkerf(char *fmt, ...);
internal B32 t_invoke_radbin(char *fmt, ...);
internal void t_kill_all(String8 pattern);
#define t_invoke_linker_timeout(c, t)       T_Ok(t_invoke(t_radlink_path(), c, t))
#define t_invoke_linker_timeoutf(t, f, ...) t_invoke_linker_timeout(push_str8f(arena, f, ##__VA_ARGS__), t)
#define t_invoke_linker(c)                  t_invoke_linker_timeout(c, max_U64)

internal void t_kill_all(String8 pattern);

internal String8 t_chop_line(String8 *string);
internal B32 t_match_line(String8 *output, String8 expected_line);
internal B32 t_match_linef(String8 *output, char *fmt, ...);

// files helper
internal String8List t_file_paths_from_dir(Arena *arena, String8 dir);

// printer
internal void t_infof(char *fmt, ...);
internal void t_errorf(char *fmt, ...);

