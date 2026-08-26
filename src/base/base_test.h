// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_TEST_H
#define BASE_TEST_H

typedef enum TestStatus
{
  TestStatus_Fail,
  TestStatus_Crash,
  TestStatus_Pass,
  TestStatus_Skip,
  TestStatus_COUNT
}
TestStatus;

typedef struct TestResult TestResult;
struct TestResult
{
  TestStatus status;
  char *fail_file;
  int fail_line;
  char *fail_cond;
};

typedef struct TestCtx TestCtx;
struct TestCtx
{
  CmdLine *cmdline;
  String8 exemplars_path;
  String8 artifacts_path;
  String8 input_data_path;
  String8 user_data;
  TestResult *result_out;
  String8List *test_out;
};

#define TEST_FUNCTION_SIG(name) void name(Arena *arena, TestCtx *ctx)
#define TEST_FUNCTION_DEF(name) TEST_FUNCTION_SIG(test__##name)
typedef TEST_FUNCTION_SIG(TestFunctionType);

typedef struct TestInfo TestInfo;
struct TestInfo
{
  String8 layer;
  String8 label;
  S64 decl_line;
  B32 skip;
  TestFunctionType *test_fn;
  String8 user_data;
};

#if BUILD_TESTS

// alloc storage for the tests
global U16      test_infos_count    = 0;
global TestInfo test_infos[0xffff] = {0};

internal String8 test_build_exe_path(Arena *arena, String8 name);
internal String8 test_input_path(Arena *arena, TestCtx *ctx, String8 name);
internal String8 test_input_exe_path(Arena *arena, TestCtx *ctx, String8 name);
internal String8 test_exemplar_path(Arena *arena, TestCtx *ctx, String8 name);
internal String8 test_layer_from_file_path(String8 file_path);
internal void base_register_test(char *func_name, TestFunctionType *fn, char *file_path, int line, int skip);

#define AddTest(name, file_path, line, skip_, ...) \
TEST_FUNCTION_DEF(name);                         \
__VA_ARGS__ void add_test__##name(void) { base_register_test(Stringify(name), test__##name, file_path, line, skip_); }

# if COMPILER_MSVC
#  pragma section(".CRT$XCU", read)
#  define DeclareTest(name, skip)                                                                                      \
/* register test          */ AddTest(name, __FILE__, __LINE__, (skip))                                               \
/* alloc function pointer */ __declspec(allocate(".CRT$XCU")) void (*add_test_ptr__##name)(void) = add_test__##name; \
/* do not GC test caller  */ __pragma(comment(linker, "/include:" Stringify(add_test_ptr__##name)))
# elif COMPILER_GCC || COMPILER_CLANG
// clang and gcc allocate memory for the function pointer automatically
#  define DeclareTest(name, skip) AddTest(name, __FILE__, __LINE__, (skip), __attribute__((constructor)))
# else
#  error DeclareTest not defined for this compiler.
# endif
#else
# define DeclareTest(name, skip)
#endif

#define Test(name)        DeclareTest(name, 0) TEST_FUNCTION_DEF(name)
#define SkippedTest(name) DeclareTest(name, 1) TEST_FUNCTION_DEF(name)

#define TestCheck(c) do { if (!(c)) {                                                                                                          \
/* record failed check     */ ctx->result_out[0] = (TestResult){ .fail_file = __FILE__, .fail_line = __LINE__, .fail_cond = Stringify(c) }; \
/* under debugger? -> trap */ if(debugger_is_attached()) { Trap(); }                                                                        \
/* exit test               */ return;                                                                                                       \
} } while(0)

#define TestSkip() do {                                           \
ctx->result_out[0] = (TestResult){ .status = TestStatus_Skip }; \
return;                                                         \
} while(0)

// test log
#define test_out(string) str8_list_push(arena, ctx->test_out, (string))
#define test_outf(...)   str8_list_pushf(arena, ctx->test_out, __VA_ARGS__)

#endif // BASE_TEST_H
