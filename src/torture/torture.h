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

typedef T_RunResult (*T_Run)(void);

typedef struct
{
  T_Run       run;
  T_RunResult result;
} T_RunCtx;

typedef struct
{
  char  *group;
  char  *label;
  int    decl_line;
  T_Run  r;
} T_Test;

extern U64    g_torture_test_count;
extern T_Test g_torture_tests[0xffffff];

#define T_AddTest(name, l, ...)                                \
T_RunResult t_##name(void);                                    \
__VA_ARGS__ void t_add_test_##name(void)                       \
{                                                              \
g_torture_tests[g_torture_test_count].group = T_Group;         \
g_torture_tests[g_torture_test_count].label = Stringify(name); \
g_torture_tests[g_torture_test_count].r     = &t_##name;       \
g_torture_tests[g_torture_test_count].decl_line = l;           \
g_torture_test_count += 1;                                     \
}

#if COMPILER_MSVC
#pragma section(".CRT$XCU", read)
# define T_BeginTest_(name)                                                 \
T_AddTest(name, __LINE__)                                                   \
__declspec(allocate(".CRT$XCU")) void(*r_##name)(void) = t_add_test_##name; \
__pragma(comment(linker, "/include:" Stringify(r_##name)))
#else
# define T_BeginTest_(name) \
T_AddTest(name, __LINE__, __attribute__((constructor)))
#endif

#define T_BeginTest(name)                 \
T_BeginTest_(name)                        \
T_RunResult t_##name(void) {              \
Temp        scratch = scratch_begin(0,0); \
T_RunResult result_ = { .status = T_RunStatus_Fail };

#define T_EndTest                  \
result_.status = T_RunStatus_Pass; \
exit__:;                           \
scratch_end(scratch);              \
return result_;                    \
}

#define T_Ok(c) do { if (!(c)) { result_.fail_file = __FILE__; result_.fail_line = __LINE__; result_.fail_cond = Stringify(c); goto exit__; } } while(0)
#define T_MatchLinef(out, ...) T_Ok(t_match_linef(out, __VA_ARGS__))

////////////////////////////////

// output directory
internal B32 t_write_file_list(String8 name, String8List data);
internal B32 t_write_file(String8 name, String8 data);
internal String8 t_read_file(Arena *arena, String8 name);
internal String8 t_make_file_path(Arena *arena, String8 name);

// test runner
internal void        t_run_caller(void *raw_ctx);
internal void        t_run_fail_handler(void *raw_ctx);
internal T_RunResult t_run(T_Run run);

internal B32 t_invoke(String8 exe, String8 cmdline, U64 timeout);


