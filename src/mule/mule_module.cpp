// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

/*
** Program to run in debugger organized to provide tests for
** stepping, breakpoints, evaluation, cross-module calls.
*/

////////////////////////////////
// NOTE(allen): Setup


#if _WIN32
#define export_function extern "C" __declspec(dllexport)
#else
#define export_function extern "C"
#endif


////////////////////////////////
// NOTE(allen): TLS Eval

#if _WIN32
# define thread_var __declspec(thread)
#else
# define thread_var __thread
#endif

thread_var float tls_a = 1.015625f;
thread_var int   tls_b = -100;

export_function void
dll_tls_eval_test(void){
  tls_a *= 1.5f;
  tls_b *= -2;
}


