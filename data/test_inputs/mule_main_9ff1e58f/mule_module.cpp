// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if _WIN32
#define export_function extern "C" __declspec(dllexport)
#else
#define export_function extern "C"
#endif

#if _WIN32
# define thread_var __declspec(thread)
#else
# define thread_var __thread
#endif

typedef struct OnlyInModule OnlyInModule;
struct OnlyInModule
{
  int x;
  int y;
  int z;
  char *name;
};

typedef struct Basics Basics;
struct Basics
{
  int a;
  int b;
  int c;
  int d;
};

static OnlyInModule only_in_module_global =
{
  1, 2, 3, "foobar",
};

thread_var float tls_a = 1.015625f;
thread_var int   tls_b = -100;

export_function void
dll_tls_eval_test(void)
{
  tls_a *= 1.5f;
  tls_b *= -2;
  only_in_module_global.x += 1;
  only_in_module_global.y += 2;
  only_in_module_global.z += 3;
}

export_function void
dll_type_eval_tests(void)
{
  Basics basics1 = {1, 2, 3, 4};
  Basics basics2 = {4, 5, 6, 7};
  OnlyInModule only_in_module = {123, 456, 789, "this type is only in the module!"};
  int x = 0;
  (void)x;
}
