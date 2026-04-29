// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

////////////////////////////////

typedef enum
{
  T_RunStatus_Fail,
  T_RunStatus_Crash,
  T_RunStatus_Pass,
} T_RunStatus;

typedef struct
{
  T_RunStatus status;
  char       *fail_file;
  int         fail_line;
  char       *fail_cond;
} T_RunResult;

#define T_RunSig(name) void t_##name(Arena *arena, String8 user_data, T_RunResult *result_out, String8List *test_out)
typedef                void (*T_Run)(Arena *arena, String8 user_data, T_RunResult *result_out, String8List *test_out);

typedef struct
{
  T_Run       run;
  String8     user_data;
  T_RunResult result;
} T_RunCtx;

typedef struct
{
  char   *group;
  char   *label;
  int     decl_line;
  T_Run   r;
  String8 user_data;
} T_Test;

extern U64    g_torture_test_count;
extern T_Test g_torture_tests[0xffffff];
extern U64    g_torture_running_test_idx;

internal void t_break_if_debugger_present(void);

#define T_AddTest(name, l, ...)            \
  T_RunSig(name);                          \
  __VA_ARGS__ void t_add_test_##name(void) \
  {                                        \
    g_torture_tests[g_torture_test_count++] = (T_Test){ .group = T_Group, .label = Stringify(name), .r = &t_##name, .decl_line = l }; \
  }

#if COMPILER_MSVC
# pragma section(".CRT$XCU", read)
# define TEST_(name)                                                          \
  T_AddTest(name, __LINE__)                                                   \
  __declspec(allocate(".CRT$XCU")) void(*r_##name)(void) = t_add_test_##name; \
  __pragma(comment(linker, "/include:" Stringify(r_##name)))
#else
# define TEST_(name) T_AddTest(name, __LINE__, __attribute__((constructor)))
#endif

#define TEST(name) \
  TEST_(name)      \
  T_RunSig(name)

#define T_Ok(c) do { if (!(c)) { result_out->fail_file = __FILE__; result_out->fail_line = __LINE__; result_out->fail_cond = Stringify(c); result_out->status = T_RunStatus_Fail; t_break_if_debugger_present(); return; } } while(0)
#define T_MatchLinef(out, ...) T_Ok(t_match_linef(out, __VA_ARGS__))
#define t_outf(...) str8_list_pushf(arena, test_out, ## __VA_ARGS__)

////////////////////////////////////////////////////////////////

#define TIMEOUT_US(x)  (x)
#define TIMEOUT_MS(x)  TIMEOUT_US((x)*1000ull)
#define TIMEOUT_SEC(x) TIMEOUT_MS((x)*1000ull)

#define ENDT_US(x)  (os_now_microseconds() + (x))
#define ENDT_MS(x)  TIMEOUT_US((x)*1000ull)
#define ENDT_SEC(x) TIMEOUT_MS((x)*1000ull)

////////////////////////////////

// output directory
internal B32 t_write_file_list(String8 name, String8List data);
internal B32 t_write_file(String8 name, String8 data);
internal String8 t_read_file(Arena *arena, String8 name);
internal B32     t_delete_file(String8 name);
internal String8 t_make_file_path(Arena *arena, String8 name);

// test runner
internal void        t_run_caller(void *raw_ctx);
internal void        t_run_fail_handler(void *raw_ctx);
internal T_RunResult t_run(T_Run run, String8 user_data);

internal String8 t_radbin_path(void);
internal String8 t_cl_path(void);
internal String8 t_radlink_path(void);
internal String8 t_cwd_path(void);
internal String8 t_src_path(void);

internal B32 t_invoke(String8 exe, String8 cmdline, U64 timeout);
internal B32 t_invoke_cl(char *fmt, ...);
internal B32 t_invoke_linkerf(char *fmt, ...);
internal B32 t_invoke_radbin(char *fmt, ...);
internal void t_kill_all(String8 pattern);

#define t_invoke_linker_timeout(c, t)       T_Ok(t_invoke(g_linker, c, t))
#define t_invoke_linker_timeoutf(t, f, ...) t_invoke_linker_timeout(push_str8f(arena, f, ##__VA_ARGS__), t)
#define t_invoke_linker(c)                  t_invoke_linker_timeout(c, max_U64)

