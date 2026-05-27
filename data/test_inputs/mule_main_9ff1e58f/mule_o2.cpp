// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

static int important_s32 = 0;
static float important_f32 = 0;

#if _WIN32
#include <Windows.h>
#endif

static void
do_something_with_intermediate_values(void)
{
  static int another_important_s32 = 0;
  static float another_important_f32 = 0;
  
  another_important_s32 = (int)important_f32;
  another_important_f32 = (float)important_s32;
  
#if _WIN32
  char buffer[256] = "Hello, World!\n";
  buffer[0] += important_s32 + another_important_s32;
  buffer[1] += (int)another_important_f32 * important_f32;
  OutputDebugStringA(buffer);
#endif
}

static void
store_important_s32(int *ptr)
{
  important_s32 = *ptr;
}

static void
store_important_f32(float *ptr)
{
  important_f32 = *ptr;
}

void
optimized_build_eval_tests(void)
{
  int simple_sum = 0;
  for(int i = 0; i < 10000; i += 1)
  {
    simple_sum += i;
  }
  store_important_s32(&simple_sum);
  
  do_something_with_intermediate_values();
  
  static struct {float x, y;} vec2s[] =
  {
    { 10.f, 76.f },
    { 40.f, 50.f },
    { -230.f, 20.f },
    { 27.f, 27.f },
    { 57.f, -57.f },
    { -37.f, 97.f },
    { 99.f, 67.f },
    { 99.f, 37.f },
    { 99.f, 57.f },
  };
  {
    struct{float x, y;}sum = {0};
    int count = sizeof(vec2s)/sizeof(vec2s[0]);
    for(int i = 0; i < count; i += 1)
    {
      sum.x += vec2s[i].x;
      sum.y += vec2s[i].y;
    }
    struct{float x, y;}avg = {sum.x/count, sum.y/count};
    float f32 = avg.x * avg.y;
    store_important_f32(&f32);
  }
  
  do_something_with_intermediate_values();
  
  int factorial = 1;
  for(int i = 10; i > 0; i -= 1)
  {
    factorial *= i;
  }
  store_important_s32(&factorial);
  
  do_something_with_intermediate_values();
}

////////////////////////////////
// NOTE(allen): Struct Parameters Eval

struct OptimizedBasics{
  char a;
  unsigned char b;
  short c;
  unsigned short d;
  int e;
  unsigned int f;
  long long g;
  unsigned long long h;
  float i;
  double j;
};

static void
optimized_struct_parameter_helper(int *ptr, OptimizedBasics basics)
{
  basics.a += *ptr;
  basics.a += 1;
  basics.a += 1;
}

void
optimized_struct_parameters_eval_tests(void)
{
  int x = 10;
  OptimizedBasics basics = {-1, 1, -2, 2, -4, 4, -8, 8, 1.5f, 1.50000000000001};
  optimized_struct_parameter_helper(&x, basics);
}
