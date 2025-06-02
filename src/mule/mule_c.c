// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

/*
* Program to run in debugger organized to provide tests for
* single threaded stepping, breakpoints, evaluation.
*/

////////////////////////////////
// NOTE(allen): Complex Types

#include <complex.h>

void
c_type_coverage_eval_tests(void){
#if _WIN32
  _Fcomplex x = _FCbuild(0.f, 1.f);
  _Dcomplex y = _Cbuild(0.f, -1.f);
  
#else
  float complex x = 0.f + 1.f*I;
  double complex y = 0.0 - 1.0*I;
  
#endif
}

////////////////////////////////
// NOTE(allen): Reuse Type Names From Another Module

#include <stdint.h>

typedef struct Basics{
  double a;
  float b;
  unsigned long long c;
  long long d;
  unsigned int e;
  int f;
  unsigned short g;
  short h;
  unsigned char i;
  char j;
  
  int z;
} Basics;

typedef struct Basics_Stdint{
  double   x1;
  float    x2;
  uint64_t x3;
  int64_t  x4;
  uint32_t x5;
  int32_t  x6;
  uint16_t x7;
  int16_t  x8;
  uint8_t  x9;
  int8_t   x0;
} Basics_Stdint;

typedef struct Pair{
  int i;
  float f;
} Pair;

void
c_versions_of_same_types(void){
  Basics basics = { 1.5f, 1.50000000000001, -1, 1, -2, 2, -4, 4, -8, 8, };
  Basics_Stdint basics_stdint = { 1.5f, 1.50000000000001, -1, 1, -2, 2, -4, 4, -8, 8, };
  Pair memory_[] = {
    {100,  1.f},
    {101,  2.f},
    {102,  4.f},
    {103,  8.f},
    {104, 16.f},
    {105, 32.f},
  };
  
  int x = memory_[3].i + basics.f;
}

////////////////////////////////
//~ NOTE(rjf): Bitfields

typedef struct TypeWithBitfield TypeWithBitfield;
struct TypeWithBitfield
{
  int v : 14;
  int w : 4;
  int x : 32;
  int y : 4;
  int z : 10;
};

typedef struct BitfieldType64 BitfieldType64;
struct BitfieldType64
{
  uint64_t size    : 63;
  uint64_t is_free : 1;
};

void
c_type_with_bitfield_usage(void)
{
  TypeWithBitfield b = {0};
  b.v = 100;
  b.w = 6;
  b.x = 434512;
  b.y = 7;
  b.z = 12;
  int x = (b.v + b.x);
  int y = (b.y - b.z);
  int z = (b.w) + 5;
  BitfieldType64 b64 = {0};
  b64.size = 524288;
  b64.is_free = 1;
  int abc = 0;
}
