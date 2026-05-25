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

#define TEST_FUNCTION_SIG(name) void name(Arena *arena, TestResult *result_out, String8List *test_out)
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
};

#if BUILD_TESTS
global U64 test_infos_count = 0;
global TestInfo test_infos[0xffff] = {0};
#define AddTest(name, file_name, line, skip_, ...) \
TEST_FUNCTION_DEF(name);\
__VA_ARGS__ void add_test__##name(void)\
{\
String8 file = str8_lit(file_name);\
U64 src_pos = str8_find_needle(file, 0, s("src/"), StringMatchFlag_SlashInsensitive);\
String8 layer_folder = str8_skip(file, src_pos+4);\
U64 layer_slash_pos = str8_find_needle(layer_folder, 0, s("/"), StringMatchFlag_SlashInsensitive);\
String8 layer_name = str8_prefix(layer_folder, layer_slash_pos);\
test_infos[test_infos_count].layer     = layer_name;\
test_infos[test_infos_count].label     = str8_lit(#name);\
test_infos[test_infos_count].decl_line = (line);\
test_infos[test_infos_count].skip      = (skip_);\
test_infos[test_infos_count].test_fn   = test__##name;\
test_infos_count += 1;\
}
# if COMPILER_MSVC
#  pragma section(".CRT$XCU", read)
#  define DeclareTest(name, skip) \
AddTest(name, __FILE__, __LINE__, (skip))\
__declspec(allocate(".CRT$XCU")) void (*add_test_ptr__##name)(void) = add_test__##name;\
__pragma(comment(linker, "/include:" Stringify(add_test_ptr__##name)))
# elif COMPILER_GCC || COMPILER_CLANG
#  define DeclareTest(name, skip) AddTest(name, __FILE__, __LINE__, (skip), __attribute__((constructor)))
# else
#  error DeclareTest not defined for this compiler.
# endif
#else
# define DeclareTest(name, skip)
#endif

#define Test(name) DeclareTest(name, 0)\
TEST_FUNCTION_DEF(name)
#define SkippedTest(name) DeclareTest(name, 1)\
TEST_FUNCTION_DEF(name)

#define TestCheck(c) do { if (!(c)) {\
*result_out = (TestResult){ .fail_file = __FILE__, .fail_line = __LINE__, .fail_cond = Stringify(c) };\
if(debugger_is_attached()) { Trap(); }\
return;\
} } while(0)

#endif // BASE_TEST_H
