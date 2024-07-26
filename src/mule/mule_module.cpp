// Copyright (c) 2024 Epic Games Tools
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

typedef struct Basics Basics;
struct Basics
{
  int a;
  int b;
  int c;
  int d;
};

thread_var float tls_a = 1.015625f;
thread_var int   tls_b = -100;

export_function void
dll_tls_eval_test(void)
{
  tls_a *= 1.5f;
  tls_b *= -2;
}

export_function void
dll_type_eval_tests(void)
{
  Basics basics1 = {1, 2, 3, 4};
  Basics basics2 = {4, 5, 6, 7};
  int x = 0;
  (void)x;
}
