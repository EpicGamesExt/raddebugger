// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

/*
** Program to run in debugger organized to provide tests for
** stepping, breakpoints, evaluation, cross-module calls.
*/

#include "raddbg_markup/raddbg_markup.h"

////////////////////////////////
// NOTE(allen): System For DLL Testing

typedef void TestFunction(void);

static void          mule_init(void);
static TestFunction* mule_get_module_function(char *name);

#if _WIN32

#include <Windows.h>

HMODULE mule_dll = 0;

static void
mule_init(void){
  mule_dll = LoadLibraryA("mule_module.dll");
}

static TestFunction*
mule_get_module_function(char *name){
  TestFunction *result = (TestFunction*)GetProcAddress(mule_dll, name);
  return(result);
}

#else

static void
mule_init(void){
  // TODO(allen): implement
}

static TestFunction*
mule_get_module_function(char *name){
  // TODO(allen): implement
  return(0);
}

#endif


////////////////////////////////
// NOTE(nick): Entry Point

int
mule_main(int argc, char **argv);

#if _WIN32
#include <windows.h>
#include <malloc.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
  int argc = __argc;
  char **argv = __argv;
  int result = mule_main(argc, argv);
  return(result);
}
#else
int main(int argc, char **argv){
  return(mule_main(argc, argv));
}
#endif

////////////////////////////////
// NOTE(nick): BSS section test

#if defined(__clang__)
# pragma clang section bss="muleBSS"
#elif defined(_MSC_VER)
// NOTE(nick): clang-cl is borken it allocates memory and sets Initialized Flag on the seciton.
// This is was reported by Jeff => https://bugs.llvm.org/show_bug.cgi?id=47939
//
// This is still unresolved, last checked Sep 11, 2023.
# pragma bss_seg("muleBSS")
#else
# error "bss not defined"
#endif
char global_variable_in_bss[4096*10000];

////////////////////////////////
// NOTE(allen): Inline Stepping (Built In Separate Unit)

extern unsigned int fixed_frac_bits;
unsigned int inline_stepping_tests(void);


////////////////////////////////
// NOTE(rjf): -O2 Optimized Code (Built In Separate Unit)

void optimized_build_eval_tests(void);
void optimized_struct_parameters_eval_tests(void);

////////////////////////////////
// NOTE(allen): Type Coverage Eval

#include <stdint.h>

struct Basics{
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
  
  int z;
};

struct Basics_Stdint{
  int8_t   a;
  uint8_t  b;
  int16_t  c;
  uint16_t d;
  int32_t  e;
  uint32_t f;
  int64_t  g;
  uint64_t h;
  float    i;
  double   j;
};

struct Pair{
  int x;
  float y;
};

struct Fixed_Array{
  Pair pairs[10];
  int count;
};

struct Dynamic_Array{
  Pair *pairs;
  int count;
};

struct Struct_With_Embedded_Arrays{
  int x;
  float y;
  Pair pairs[10];
  char z;
};

typedef unsigned int Custom_Index_Type;

typedef void Function_No_Params_Type(void);
typedef void Function_Few_Params_Type(Pair *pairs, int count, Function_No_Params_Type *no_params_type);

static Function_No_Params_Type *ty_no_params = 0;
static Function_Few_Params_Type *ty_few_params = 0;

struct Callback{
  Function_Few_Params_Type *few_params;
  Function_No_Params_Type *no_params;
  Pair pair;
};

union Vector_R2{
  struct{
    float x;
    float y;
  };
  float v[2];
};

enum Kind{
  Kind_None,
  Kind_First,
  Kind_Second,
  Kind_Third,
  Kind_Fourth,
  Kind_COUNT,
};

enum Flag{
  Flag_None = 0,
  Flag_First = 1,
  Flag_Second = 2,
  Flag_Third = 4,
  Flag_Fourth = 8,
  Flag_AllMoreNarrow = 0xFF,
  Flag_AllNarrow = 0xFFFF,
  Flag_All = 0xFFFFFFFF,
};

struct Has_Enums{
  Kind kind;
  Flag flags;
};

struct Discriminated_Union{
  Kind kind;
  union{
    struct{
      int x;
      int y;
      Vector_R2 vector;
    } first;
    Pair second;
    struct{
      Function_Few_Params_Type *few_params;
      Pair pairs[4];
    } third;
    struct{
      Kind sub_kind;
      Flag flags;
    } fourth;
  };
};

struct Linked_List{
  Linked_List *next;
  Linked_List *prev;
  int x;
};

enum{
  Anonymous_A,
  Anonymous_B,
  Anonymous_C,
  Anonymous_D,
};

typedef Kind Alias1;
typedef Flag Alias2;
typedef Has_Enums Alias3;
typedef Discriminated_Union Alias4;

struct Has_A_Pre_Forward_Reference{
  struct Gets_Referenced_Forwardly *pointer;
};

struct Gets_Referenced_Forwardly{
  int x;
  int y;
};

struct Has_A_Post_Forward_Reference{
  struct Gets_Referenced_Forwardly value;
};

static void
no_params1(void){
  
}

static void
few_params1(Pair *pairs, int count, Function_No_Params_Type *no_params_type){
  
}

static void
type_coverage_eval_tests(void){
  
  Basics basics = {-1, 1, -2, 2, -4, 4, -8, 8, 1.5f, 1.50000000000001};
  Basics_Stdint basics_stdint = {-1, 1, -2, 2, -4, 4, -8, 8, 1.5f, 1.50000000000001};
  
  char string[] = "Hello World!";
  char longer_text[] =
    "Suppose there was some text\n"
    "With multiple lines in it\r\n"
    "\t> What ways might it be rendered?\n"
    "\t> How would it deal with line endings?\r\n";
  
  void *pointer = &basics;
  Basics *pointer_to_basics = &basics;
  Basics **pointer_to_pointer_to_basics = &pointer_to_basics;
  
  Fixed_Array fixed = {
    {
      { 3,  4.f},
      { 5,  6.f},
      { 7,  8.f},
      { 9, 10.f},
      {11, 12.f},
      {13, 14.f},
      {15, 16.f},
      {17, 18.f},
      {19, 20.f},
    },
    9
  };
  Pair memory_[] = {
    {100,  1.f},
    {101,  2.f},
    {102,  4.f},
    {103,  8.f},
    {104, 16.f},
    {105, 32.f},
  };
  Dynamic_Array dynamic = {
    memory_,
    6
  };
  
  raddbg_pin(basics);
  raddbg_pin(fixed);
  raddbg_pin(pointer);
  
  Struct_With_Embedded_Arrays swea = {0};
  {
    swea.x = 4;
    swea.y = 23.5f;
    swea.pairs[0].x = 100;
    swea.pairs[0].y = 123.f;
    swea.pairs[2].x = 300;
    swea.pairs[2].y = 323.f;
    swea.pairs[5].x = 600;
    swea.pairs[5].y = 623.f;
    swea.z = 'z';
  }
  
  Custom_Index_Type custom_index = 42;
  Custom_Index_Type more_custom_indices[] = {
    04,13,22,31,40
  };
  
  Function_No_Params_Type *ptr_no_params = no_params1;
  Function_No_Params_Type **ptr_ptr_no_params = &ptr_no_params;
  Function_Few_Params_Type *ptr_few_params = few_params1;
  Function_Few_Params_Type **ptr_ptr_few_params = &ptr_few_params;
  Callback callback = {few_params1, no_params1, {1, 2.f}};
  
  Vector_R2 vector = {1.f, 2.f};
  
  Has_Enums has_enums = {(Kind)4, (Flag)7};
  
  Discriminated_Union discriminated_union = {Kind_First};
  discriminated_union.first.x = 16;
  discriminated_union.first.y = 8;
  discriminated_union.first.vector.x = 4.f;
  discriminated_union.first.vector.y = 2.f;
  
  Linked_List list = {&list, &list, 0};
  
  Alias1 a1 = has_enums.kind;
  Alias2 a2 = has_enums.flags;
  Alias3 a3 = has_enums;
  Alias4 a4 = discriminated_union;
  
  Has_A_Pre_Forward_Reference  r1 = {0};
  Has_A_Post_Forward_Reference r2 = {0};
  
  Basics &basics_ref = basics;
  
  union
  {
    int x;
    char y[4];
  } integer_slicing = {123456789};
  
  typedef struct stks
  {
    void *left;
    size_t len;
  } stks;
  stks stks_test[256] = {0};
  stks *stks_first = &stks_test[0];
  stks *stks_ptr = stks_first + 8;
  
  TestFunction *function = mule_get_module_function("dll_type_eval_tests");
  function();
  
  int x = (int)(Anonymous_D);
}

////////////////////////////////
// NOTE(allen): Mutating Variables Eval

static const int con_some_constant = 4;
static const float con_some_constant_f = 0.04f;

static int mut_x = 0;
static int mut_y;
static int mut_xarray[4] = {0, 1, 2, 3};
static int *mut_xptr;

static float mut_f = 0;
static float mut_g;
static float mut_farray[4] = {0.5f, 1.5f, 2.5f, 3.5f};
static float *mut_fptr;

static float mut_arrayarray[3][3];

static Linked_List mut_link;

static void
mutate_in_function(int *array, int count){
  for (int i = 0; i < count; i += 1){
    array[i] += 1;
  }
  
  for (int i = 0; i < 4; i += 1){
    mut_farray[i] += 1.f;
  }
}

static void
mutating_variables_eval_tests(void){
  ////////////////////////////////
  // NOTE(allen): Basics
  
  int array_literal[10] = {
    10, 20, 30, 40, 50, 60, 70, 80, 90,
  };
  
  Basics struct_literal = {
    -1, 1, -2, 2, -4, 4, -8, 8, 1.5f, 1.50000000000001,
  };
  
  array_literal[0] = struct_literal.e = 9;
  
  int x = mut_x;
  int y = x + 10 + con_some_constant;
  mut_y = y;
  mut_xarray[0] += 0;
  mut_xarray[1] += x;
  mut_xarray[2] += y;
  mut_xarray[3] += x + y;
  
  mut_xptr = &mut_xarray[2];
  
  *mut_xptr -= (y - x)/2;
  *(mut_xptr - 1) += 11;
  
  float f = mut_f + .333f + con_some_constant_f;
  float g = f + 10.1f;
  mut_g = g;
  mut_farray[0] += 0.000001f;
  mut_farray[1] += f;
  mut_farray[2] += g;
  mut_farray[3] += f + g;
  
  mut_fptr = &mut_farray[3];
  
  *mut_fptr -= (g - f)*0.5f;
  *(mut_fptr - 1) += 1.f;
  
  float a = 0.777f;
  for (int i = 0; i < 3; i += 1){
    float b = a*a - 1.f;
    for (int j = 0; j < 3; j += 1){
      mut_arrayarray[i][j] = b;
      b += 0.111f;
    }
    a += 0.333f;
  }
  
  ////////////////////////////////
  // NOTE(allen): Changes in functions
  
  mutate_in_function(array_literal, 10);
  
  mutate_in_function(array_literal, 10);
  
  ////////////////////////////////
  // NOTE(allen): Changes through pointers
  
  Basics basic = struct_literal;
  Basics advanced = struct_literal;
  
  Basics *struct_pointer = &basic;
  
  basic.a += 1;
  advanced.a += 1;
  struct_pointer->a += 1;
  
  struct_pointer = &advanced;
  
  basic.b += 1;
  advanced.b += 1;
  struct_pointer->b += 1;
  
  Linked_List links[5];
  for (int i = 0; i < 5; i += 1){
    links[i].next = &links[i + 1];
    links[i].prev = &links[i - 1];
    links[i].x = i;
  }
  links[0].prev = 0;
  links[4].next = &mut_link;
  mut_link.prev = &links[4];
  mut_link.next = 0;
  mut_link.x = 1000;
  
  Linked_List *link_ptr = links;
  
  link_ptr = link_ptr->next;
  
  link_ptr = &links[4];
  link_ptr = &mut_link;
  
  Linked_List sentinel = {0};
  sentinel.x = -1;
  sentinel.next = &links[0];
  links[0].prev = &sentinel;
  sentinel.prev = &mut_link;
  mut_link.next = &sentinel;
  
  link_ptr = &sentinel;
}

////////////////////////////////
// NOTE(allen): Global Eval

struct NestedNodeInner{
  unsigned int small0;
  unsigned int small1;
  unsigned int big0;
  unsigned int big1;
};

struct NestedNodeOuter{
  NestedNodeOuter *next;
  NestedNodeInner *inner_nodes;
  unsigned int inner_node_count;
};

static void
nested_types_eval_tests(void){
  // doing some setup
  NestedNodeOuter *outer1 = (NestedNodeOuter*)malloc(sizeof(NestedNodeOuter));
  NestedNodeOuter *outer2 = (NestedNodeOuter*)malloc(sizeof(NestedNodeOuter));
  NestedNodeOuter *outer3 = (NestedNodeOuter*)malloc(sizeof(NestedNodeOuter));
  
  outer1->next = outer2;
  outer2->next = outer3;
  outer3->next = 0;
  
  outer1->inner_nodes = (NestedNodeInner*)malloc(sizeof(NestedNodeInner)*10);
  outer1->inner_node_count = 10;
  
  outer2->inner_nodes = (NestedNodeInner*)malloc(sizeof(NestedNodeInner)*10);
  outer2->inner_node_count = 10;
  
  outer3->inner_nodes = (NestedNodeInner*)malloc(sizeof(NestedNodeInner)*10);
  outer3->inner_node_count = 10;
  
  for (unsigned int i = 0; i < 10; i += 1){
    outer1->inner_nodes[i].small0 = i;
    outer1->inner_nodes[i].small1 = 2*i;
    outer1->inner_nodes[i].big0 = 0xFFFFFF + 0xF*i;
    outer1->inner_nodes[i].big1 = 0xFFFFFF + 0xFF*i;
    
    outer2->inner_nodes[i].small0 = 1 + i;
    outer2->inner_nodes[i].small1 = 3*i;
    outer2->inner_nodes[i].big0 = 0x1000000 + 0x10*i;
    outer2->inner_nodes[i].big1 = 0x1000000 + 0x101*i;
    
    outer3->inner_nodes[i].small0 = 2 + i;
    outer3->inner_nodes[i].small1 = 4*i;
    outer3->inner_nodes[i].big0 = 0x8000000 + 0xF0*i;
    outer3->inner_nodes[i].big1 = 0x8000000 + 0xF0F*i;
  }
  
  // okay eval it here
  int x = 0;
}

////////////////////////////////
// NOTE(rjf): Struct Parameters Eval

static void
struct_parameter_helper(Basics basics)
{
  basics.a += 1;
  basics.a += 1;
  basics.a += 1;
}

static void
struct_parameters_eval_tests(void)
{
  Basics basics = {-1, 1, -2, 2, -4, 4, -8, 8, 1.5f, 1.50000000000001};
  struct_parameter_helper(basics);
}

////////////////////////////////
// NOTE(allen): Global Eval

static int g_abc = 100;
static float g_xyz = 21.f;
static Alias1 g_kind = Kind_First;

// TODO(allen): more global test types

static void
complicated_global_mutation(int *x){
  *x = (int)g_xyz;
}

static void
cross_unit_global_mutation(void){
  fixed_frac_bits = 10;
}

static void
global_eval_tests(void){
  g_abc = 11*11;
  g_xyz = (float)g_abc - 21.f;
  
  int z = g_abc;
  complicated_global_mutation(&z);
  
  complicated_global_mutation(&g_abc);
  
  if (g_kind == Kind_First){
    g_abc -= 1;
    g_kind = Kind_None;
  }
  
  cross_unit_global_mutation();
  
  static int l_abc = 200;
  static float l_xyz = 42.f;
  static Alias1 l_kind = Kind_Second;
  
  l_abc = g_abc*2;
  l_xyz = g_xyz*2;
  l_kind = (Alias1)(g_kind + 1);
}

////////////////////////////////
// NOTE(allen): Return Eval

static int
complicated_return_expression(void){
  int x = 171717;
  return((x % 13) <= 5?(x % 19)*11:(x - 500)%200);
}

static void
return_eval_tests(void){
  complicated_return_expression();
}

////////////////////////////////
// NOTE(allen): TLS Eval

#if _WIN32
# define thread_var __declspec(thread)
#else
# define thread_var __thread
#endif

thread_var int tls_a = 100;
thread_var int tls_b = 999;

static void
tls_eval_tests(void){
  tls_a = (tls_a + tls_b)/2;
  tls_b = tls_b - tls_a;
  
  TestFunction *dll_tls_eval_test = mule_get_module_function("dll_tls_eval_test");
  if (dll_tls_eval_test != 0){
    dll_tls_eval_test();
  }
}

////////////////////////////////
// NOTE(allen): Complicated Type Coverage Eval

struct Complicated_Type_Members{
  int x600[2][2][2][2];
  int *x601[2][2][2][2];
  int (*x602)[2][2][2][2];
  int (*x603[2])[2][2][2];
  int (*(*x604[2])[2])[2][2];
  int (*(*(*x605[2])[2])[2])[2];
  
  int (*x33[2])(void);
  int (*x34[3])(void);
  int (*x35[2][2])(void);
  
  int (*(*z33)(void))[2];
  int (*(*z34)(void))[3];
  int (*(*z35)(void))[2][2];
  
  int   (*(*f2)(void))(void);
  int (*(*(*f3)(void))(void))(void);
  int   (*(*f4)(int))(void);
  int   (*(*f5)(void))(int);
  int   (*(*f6)(int))(int);
  int (*(*(*f7_growing)(char))(short))(int);
  int (*(*(*f7_shrinking)(int))(short))(char);
};

static void
complicated_type_coverage_tests(void){
  Complicated_Type_Members m = {0};
  
  int x1 = {0};
  int *x2 = {0};
  int **x3 = {0};
  
  int x4a[2] = {0};
  int *x4[2] = {0};
  int x5a[3] = {0};
  int *x5[3] = {0};
  int *x6[2][2] = {0};
  int (*x7)[2] = {0};
  int (*x8)[3] = {0};
  int (*x9)[2][2] = {0};
  
  int x600[2][2][2][2] = {0};
  int *x601[2][2][2][2] = {0};
  int (*x602)[2][2][2][2] = {0};
  int (*x603[2])[2][2][2] = {0};
  int (*(*x604[2])[2])[2][2] = {0};
  int (*(*(*x605[2])[2])[2])[2] = {0};
  
  int x606_growing  [2][3][4][5] = {0};
  int x606_shrinking[5][4][3][2] = {0};
  
  int (*(*(*x607_growing  [2])[3])[4])[5] = {0};
  int (*(*(*x607_shrinking[5])[4])[3])[2] = {0};
  
  int **x10[2] = {0};
  int **x11[3] = {0};
  int **x12[2][2] = {0};
  int *(*x13)[2] = {0};
  int *(*x14)[3] = {0};
  int *(*x15)[2][2] = {0};
  int **x16[2] = {0};
  int **x17[3] = {0};
  int **x18[2][2] = {0};
  
  int (*y1[2])[2] = {0};
  int (*y2[3])[2] = {0};
  int (*y3[2][2])[2] = {0};
  int (*y4[2])[3] = {0};
  int (*y5[3])[3] = {0};
  int (*y6[2][2])[3] = {0};
  int (*y7[2])[2][2] = {0};
  int (*y8[3])[2][2] = {0};
  int (*y9[2][2])[2][2] = {0};
  
  int (*x19)(void) = {0};
  int (*x20)(int) = {0};
  int (*x21)(int, int) = {0};
  int (*x22)(int*, int) = {0};
  int (*x23)(int**, int) = {0};
  int (*x24)(int**, int*) = {0};
  int (*x25)(int**, int**) = {0};
  
  int *(*x26)(void) = {0};
  int *(*x27)(int) = {0};
  int *(*x28)(int, int) = {0};
  int *(*x29)(int*, int) = {0};
  int *(*x30)(int**, int) = {0};
  int *(*x31)(int**, int*) = {0};
  int *(*x32)(int**, int**) = {0};
  
  int (*x33[2])(void) = {0};
  int (*x34[3])(void) = {0};
  int (*x35[2][2])(void) = {0};
  
  int (*x36[2])(int) = {0};
  int (*x37[3])(int) = {0};
  int (*x38[2][2])(int) = {0};
  
  int (*x39[2])(int, int) = {0};
  int (*x40[3])(int, int) = {0};
  int (*x41[2][2])(int, int) = {0};
  
  int (*x42[2])(int*, int) = {0};
  int (*x43[3])(int*, int) = {0};
  int (*x44[2][2])(int*, int) = {0};
  
  int (*x45[2])(int**, int) = {0};
  int (*x46[3])(int**, int) = {0};
  int (*x47[2][2])(int**, int) = {0};
  
  int (*x48[2])(int**, int*) = {0};
  int (*x49[3])(int**, int*) = {0};
  int (*x50[2][2])(int**, int*) = {0};
  
  int (*x51[2])(int**, int**) = {0};
  int (*x52[3])(int**, int**) = {0};
  int (*x53[2][2])(int**, int**) = {0};
  
  int (*(*z33)(void))[2] = {0};
  int (*(*z34)(void))[3] = {0};
  int (*(*z35)(void))[2][2] = {0};
  
  int (*(*z36)(int))[2] = {0};
  int (*(*z37)(int))[3] = {0};
  int (*(*z38)(int))[2][2] = {0};
  
  int (*(*z39)(int, int))[2] = {0};
  int (*(*z40)(int, int))[3] = {0};
  int (*(*z41)(int, int))[2][2] = {0};
  
  int (*(*z42)(int*, int))[2] = {0};
  int (*(*z43)(int*, int))[3] = {0};
  int (*(*z44)(int*, int))[2][2] = {0};
  
  int (*(*z45)(int**, int))[2] = {0};
  int (*(*z46)(int**, int))[3] = {0};
  int (*(*z47)(int**, int))[2][2] = {0};
  
  int (*(*z48)(int**, int*))[2] = {0};
  int (*(*z49)(int**, int*))[3] = {0};
  int (*(*z50)(int**, int*))[2][2] = {0};
  
  int (*(*z51)(int**, int**))[2] = {0};
  int (*(*z52)(int**, int**))[3] = {0};
  int (*(*z53)(int**, int**))[2][2] = {0};
  
  int (*(*z303[2])(void)) = {0};
  int (*(*z304[3])(void)) = {0};
  int (*(*z305[2][2])(void)) = {0};
  
  int (*(*z306[2])(int)) = {0};
  int (*(*z307[3])(int)) = {0};
  int (*(*z308[2][2])(int)) = {0};
  
  int (*(*z309[2])(int, int)) = {0};
  int (*(*z400[3])(int, int)) = {0};
  int (*(*z401[2][2])(int, int)) = {0};
  
  int (*(*z402[2])(int*, int)) = {0};
  int (*(*z403[3])(int*, int)) = {0};
  int (*(*z404[2][2])(int*, int)) = {0};
  
  int (*(*z405[2])(int**, int)) = {0};
  int (*(*z406[3])(int**, int)) = {0};
  int (*(*z407[2][2])(int**, int)) = {0};
  
  int (*(*z408[2])(int**, int*)) = {0};
  int (*(*z409[3])(int**, int*)) = {0};
  int (*(*z500[2][2])(int**, int*)) = {0};
  
  int (*(*z501[2])(int**, int**)) = {0};
  int (*(*z502[3])(int**, int**)) = {0};
  int (*(*z503[2][2])(int**, int**)) = {0};
  
  int   (*(*f2)(void))(void) = {0};
  int (*(*(*f3)(void))(void))(void) = {0};
  int   (*(*f4)(int))(void) = {0};
  int   (*(*f5)(void))(int) = {0};
  int   (*(*f6)(int))(int) = {0};
  int (*(*(*f7_growing)(char))(short))(int) = {0};
  int (*(*(*f7_shrinking)(int))(short))(char) = {0};
  
  int (*f8)(int (*)(void)) = {0};
  int (*f9)(void (*)(int)) = {0};
  void (*f10)(int (*)(int)) = {0};
  int (*f11)(int, int (*)(void)) = {0};
  int (*f12)(int (*)(void), int) = {0};
  int (*f13)(int (*)(void), int (*)(void)) = {0};
  
  int (*f14)(int (*)(void)) = {0};
  int (*f15)(int (*)(int (*)(void))) = {0};
  int (*f16)(int (*)(int (*)(int (*)(void)))) = {0};
  int (*f17)(int (*)(int (*)(int (*)(int (*)(void))))) = {0};
  
  int (*f18)(int (*)(void)) = {0};
  int (*f19)(int (*(*)(void))(void)) = {0};
  int (*f20)(int (*(*(*)(void))(void))(void)) = {0};
  int (*f21)(int (*(*(*(*)(void))(void))(void))(void)) = {0};
  
  int (*(*(*(*f22)(void))(void))(void))(void) = {0};
  int (*(*(*(*f23)(int [2]))(void))(void))(void) = {0};
  int (*(*(*(*f24)(int *[2]))(int [3]))(void))(void) = {0};
  int (*(*(*(*f25)(int (*)[2]))(int *[3]))(int [4]))(void) = {0};
  int (*(*(*(*f26)(int **(**)[2]))(int (*)[3]))(int *[4]))(int [5]) = {0};
  
  int x = 0;
}

////////////////////////////////
// NOTE(allen): Extended Type Coverage Eval

template<class X>
struct Template_Example{
  X x;
  int y;
};

template<class X, class Y>
struct Template_Example2{
  X x;
  Y y;
};

template<class X, class Y>
struct Template_Example3{
  X x;
  Y y;
  Template_Example3(X x, Y y)
  {
    this->x = x;
    this->y = y;
  }
  ~Template_Example3()
  {
    int x = 2;
    int y = 5;
    int z = x + y;
  }
};

struct SingleInheritanceBase
{
  int x;
  int y;
};

struct SingleInheritanceDerived : SingleInheritanceBase
{
  int z;
  int w;
};

struct Has_Members{
  int a;
  int b;
  uint64_t c;
  uint64_t d;
  Basics bas;
  
  int w(void){ return a; }
  int x(void){ return b; }
  uint64_t y(void){ return c; }
  uint64_t z(void){ return d; }
  Basics bas_f(void){ return bas; }
};

struct Has_Static_Members{
  int a;
  int b;
  static uint64_t c;
  static uint64_t d;
  
  int w(void){ return a; }
  int x(void){ return b; }
  static uint64_t y(void){ return c; }
  static uint64_t z(void){ return d; }
};

uint64_t Has_Static_Members::c = 0;
uint64_t Has_Static_Members::d = 0;

struct Pointer_To_Member{
  int      Has_Members::*member_ptr_int;
  uint64_t Has_Members::*member_ptr_u64;
  Basics   Has_Members::*member_ptr_bas;
  
  int      (Has_Members::*method_ptr_int)(void);
  uint64_t (Has_Members::*method_ptr_u64)(void);
  Basics   (Has_Members::*method_ptr_bas)(void);
};

struct Has_Sub_Types{
  struct Sub_Type1{
    int x;
    int y;
  };
  
  struct Sub_Type2{
    float x;
    float y;
  };
  
  Sub_Type1 a;
  Sub_Type2 b;
};

struct Conflicting_Type_Names{
  struct Sub_Type1{
    uint64_t z;
  };
  
  struct Sub_Type2{
    int64_t z;
  };
  
  Sub_Type1 a;
  Sub_Type2 b;
};

struct Has_Private_Sub_Types{
  Has_Private_Sub_Types(char x1, char y1,
                        float x2, int y2,
                        int x3, float y3){
    this->a.x = x1;
    this->a.y = y1;
    this->b.x = x2;
    this->b.y = y2;
    this->c.x = x3;
    this->c.y = y3;
  }
  
  struct Public_Sub_Type{
    char x;
    char y;
  };
  Public_Sub_Type a;
  
  protected:
  struct Protected_Sub_Type{
    float x;
    int y;
  };
  Protected_Sub_Type b;
  
  private:
  struct Private_Sub_Type{
    int x;
    float y;
  };
  Private_Sub_Type c;
};

struct Vtable_Parent{
  virtual void a_virtual_function(void) = 0;
  virtual void b_virtual_function(void) = 0;
  virtual void c_virtual_function(void) = 0;
  
  void a_virtual_function(int r){
    for (int i = 0; i < r; i += 1){
      a_virtual_function();
    }
  }
};

struct Vtable_Child : Vtable_Parent{
  int x;
  int y;
  
  Vtable_Child(int a, int b){
    x = a;
    y = b;
  }
  virtual void a_virtual_function(void){
    x = 0;
  };
  virtual void b_virtual_function(void){
    y = 0;
  };
  virtual void c_virtual_function(void){
    x = y;
  };
};

struct Vinheritance_Base{
  int x;
  int y;
  
  virtual void a_virtual_function(void){
    x = 0;
  };
  virtual void b_virtual_function(void){
    y = 0;
  };
  virtual void x_virtual_function(void){
    y = x;
  };
};

struct Vinheritance_MidLeft : virtual Vinheritance_Base{
  float left;
  
  virtual void c1_virtual_function(void){
    left = 0;
  };
  virtual void c2_virtual_function(void){
    left = 0;
  };
};

struct Vinheritance_MidRight : virtual Vinheritance_Base{
  float right;
  
  virtual void d_virtual_function(void){
    right = 0;
  };
};

struct Vinheritance_Child : Vinheritance_MidLeft, Vinheritance_MidRight{
  char *name;
  
  virtual void a_virtual_function(void){
    x = 1;
  };
  virtual void c1_virtual_function(void){
    left = 1;
  };
};

struct Minheritance_Base{
  int x;
  int y;
};

struct Minheritance_MidLeft : Minheritance_Base{
  float left;
};

struct Minheritance_MidRight : Minheritance_Base{
  float right;
};

struct Minheritance_Child : Minheritance_MidLeft, Minheritance_MidRight{
  char *name;
};

struct Pure
{
  virtual ~Pure() = default;
  virtual void Foo() = 0;
};

struct PureChild : Pure
{
  virtual ~PureChild() = default;
  virtual void Foo() {a += 1;}
  double a = 0;
};

struct Base
{
  int x;
  int y;
  int z;
  virtual ~Base() = default;
  virtual void Foo() = 0;
};

struct Derived : Base
{
  int r;
  int g;
  int b;
  int a;
  virtual ~Derived() = default;
  virtual void Foo() {a += 1;}
};

struct DerivedA : Base
{
  float a;
  float b;
  virtual void Foo() {a += 1;}
  virtual ~DerivedA() = default;
};

struct DerivedB : Base
{
  double c;
  double d;
  virtual void Foo() {c += 1;}
  virtual ~DerivedB() = default;
};

struct OverloadedMethods{
  int x;
  int cool_method(void){
    return(x);
  }
  int cool_method(int z){
    int r = x;
    x = z;
    return(r);
  }
  void cool_method(int y, int z){
    if (x < z){
      x = y;
    }
    else{
      x = z;
    }
  }
};

struct HasStaticConstMembers
{
  int a;
  int b;
  static int c;
  static int d;
  static const int e = 789;
  static const int f = 101112;
};

int HasStaticConstMembers::c = 123;
int HasStaticConstMembers::d = 456;

struct Has_A_Constructor{
  int n;
  int d;
  Has_A_Constructor(int a, int b){
    int gcd = 1;
    {
      int x = a;
      int y = b;
      if (x < y){
        y = a;
        x = b;
      }
      for (;y > 0;){
        int z = x%y;
        x = y;
        y = z;
      }
      gcd = x;
    }
    n = a/gcd;
    d = b/gcd;
  }
  
  static int N;
  static int D;
  ~Has_A_Constructor(){
    int m = N*d + n*D;
    int e = d*D;
    N = m;
    D = d;
  }
};

int Has_A_Constructor::N = 0;
int Has_A_Constructor::D = 1;

struct Constructor_Gotcha_Test{
  int x;
  int y;
  void Constructor_Gotcha(void){
    x = y = 0;
  }
};

struct Has_A_Friend{
  friend struct Modifies_Other;
  int get_x(void){ return x; }
  int get_y(void){ return y; }
  
  private:
  int x;
  int y;
};

struct Modifies_Other{
  int x;
  int y;
  
  void talk_to_friend(Has_A_Friend *other){
    other->x = y;
    other->y = x;
  }
};

namespace UserNamespace{
  namespace SubA{
    struct Foo{
      int x;
      int y;
    };
  };
  namespace SubB{
    struct Foo{
      float u;
      float v;
    };
  };
  
  SubA::Foo foo_a = {10, 20};
  SubB::Foo foo_b = {0.1f, 0.05f};
  
  static void namespaced_function(void){
    foo_a.x = (int)(foo_a.y*foo_b.u);
    foo_b.v = (float)(foo_a.x*foo_b.v);
  }
};

static void
call_with_pass_by_reference(int &x){
  x += 1;
}

static void
call_with_pass_by_const_reference(const int &x){
  int y = x;
}

static void
extended_type_coverage_eval_tests(void){
  ////////////////////////////////
  // NOTE(allen): Extensions to base type system.
  {
    int x = 0;
    const int *x_ptr = &x;
    int *const x_cptr = &x;
    
    call_with_pass_by_reference(x);
    
    call_with_pass_by_const_reference(x);
  }
  
  ////////////////////////////////
  // NOTE(allen): Extensions to user defined types
  {
    Template_Example<float> temp_f = {1.f, 2};
    Template_Example<void*> temp_v = {(void*)&temp_f, 2};
    Template_Example<Template_Example<float> > temp_tf = {temp_f, 2};
    Template_Example2<int, float> temp_if = {2, 1.f};
    Template_Example3<int, float> temp3_if(2, 1.f);
    Template_Example3<void *, float> temp3_vi((void *)&temp3_if, 1.f);
    Template_Example3<int, Template_Example2<int, float>> temp3_itif(123, temp_if);
    
    SingleInheritanceDerived sid;
    sid.x = 123;
    sid.y = 456;
    sid.z = 789;
    sid.w = 999;
    
    Pointer_To_Member pointer_to_member = {
      &Has_Members::a, &Has_Members::c, &Has_Members::bas,
      &Has_Members::x, &Has_Members::z, &Has_Members::bas_f,
    };
    
    Has_Static_Members has_static_members = { 10, 20 };
    Has_Static_Members::c = 100;
    Has_Static_Members::d = 110;
    has_static_members.x();
    has_static_members.y();
    has_static_members.z();
    has_static_members.w();
    
    Has_Sub_Types has_sub_types = {
      {100, 200},
      {.1f, .2f},
    };
    
    Conflicting_Type_Names conflicting_type_names = {
      {10}, {-20},
    };
    
    Has_Private_Sub_Types has_private_sub_types(1, 2, 4, 8, 16, 32);
    
    Vtable_Child vtable_child(1, 2);
    vtable_child.a_virtual_function();
    
    Vinheritance_Child vinheritance_child;
    vinheritance_child.name  = "foobar";
    vinheritance_child.left  = 10.5f;
    vinheritance_child.right = 13.0f;
    vinheritance_child.x     = -1;
    vinheritance_child.y     = -1;
    
    Minheritance_Child minheritance_child;
    minheritance_child.name  = "foobar";
    minheritance_child.left  = 10.5f;
    minheritance_child.right = 13.0f;
    minheritance_child.Minheritance_MidLeft::x = -1;
    minheritance_child.Minheritance_MidLeft::y = -1;
    minheritance_child.Minheritance_MidRight::x = +1;
    minheritance_child.Minheritance_MidRight::y = +1;
    
    Pure *child = new PureChild();
    child->Foo();
    child->Foo();
    child->Foo();
    delete child;
    
    Base *derived = new Derived();
    derived->Foo();
    derived->Foo();
    derived->Foo();
    delete derived;
    
    Base *base_array[1024] = {0};
    for(int i = 0; i < sizeof(base_array)/sizeof(base_array[0]); i += 1)
    {
      if(i & 1 == 1)
      {
        base_array[i] = new DerivedA();
      }
      else
      {
        base_array[i] = new DerivedB();
      }
    }
    
    OverloadedMethods overloaded_methods;
    {
      overloaded_methods.x = 0;
      int a = overloaded_methods.cool_method();
      overloaded_methods.cool_method(-10, 100);
      int b = overloaded_methods.cool_method(100);
      overloaded_methods.cool_method(b*2, a*2);
      int c = overloaded_methods.cool_method(a + b);
      int z = c;
    }
    
    Has_A_Constructor construct_me(360, 25);
    
    Has_A_Friend has_a_friend;
    
    Modifies_Other modifies_other;
    modifies_other.x = 57;
    modifies_other.y = 66;
    
    modifies_other.talk_to_friend(&has_a_friend);
    
    int x = has_a_friend.get_x();
    int y = has_a_friend.get_y();
    int z = x;
    
    HasStaticConstMembers static_const_members = {0};
    static_const_members.a = 123 + HasStaticConstMembers::c * HasStaticConstMembers::e;
    static_const_members.b = 456 + HasStaticConstMembers::d * HasStaticConstMembers::f;
  }
  
  ////////////////////////////////
  // NOTE(allen): Namespaces
  {
    UserNamespace::namespaced_function();
  }
}

////////////////////////////////
//~ rjf: Templated Function Eval Tests

typedef struct TemplateArg TemplateArg;
struct TemplateArg
{
  int x;
  int y;
  int z;
  float a;
  float b;
  float c;
  char *name;
};

template<typename T> static T
templated_factorial(T t)
{
  T result = t;
  if(t > 1)
  {
    result *= templated_factorial<T>(t-1);
  }
  return result;
}

template<typename T> static T
compute_template_arg_info(T t)
{
  int sum = t.x + t.y + t.z;
  int size = sizeof(t);
  float sum_f = t.a + t.b + t.c;
  OutputDebugStringA(t.name);
  return t;
}

static void
templated_function_eval_tests(void)
{
  int int_factorial = templated_factorial<int>(10);
  float float_factorial = templated_factorial<float>(10);
  TemplateArg arg = {1, 2, 3, 4.f, 5.f, 6.f, "my template arg"};
  compute_template_arg_info(arg);
  int x = 0;
}

////////////////////////////////
//~ NOTE(allen): C Type Coverage

extern "C"{
#include "mule_c.h"
}

////////////////////////////////
//~ rjf: Fancy Visualization Eval Tests

static unsigned int
mule_bswap_u32(unsigned int x)
{
  unsigned int result = (((x & 0xFF000000) >> 24) |
                         ((x & 0x00FF0000) >> 8)  |
                         ((x & 0x0000FF00) << 8)  |
                         ((x & 0x000000FF) << 24));
  return result;
}

static void
fancy_viz_eval_tests(void)
{
  //- rjf: colors
  float example_color_4f32[4] = {1.00f, 0.85f, 0.25f, 1.00f};
  unsigned int example_color_u32 = 0xff6f30ff;
  struct {float r, g, b, a;} example_color_struct = {0.50f, 0.95f, 0.75f, 1.00f};
  int x0 = 0;
  
  //- rjf: multiline text
  char *long_string = ("This is an example of some very long text with line breaks\n"
                       "in it. This is a very common kind of data which is inspected\n"
                       "in the debugger while programming, and it is often a pain\n"
                       "when it is poorly supported.\n");
  char *code_string = ("#include <stdio.h>\n"
                       "\n"
                       "int main(int argc, char**argv)\n"
                       "{\n"
                       "  printf(\"Hello, World!\\n\");\n"
                       "  return 0;\n"
                       "}\n\n");
  int x1 = 0;
  raddbg_pin(long_string,          "text");
  raddbg_pin(code_string,          "text: (lang:c)");
  raddbg_pin(fancy_viz_eval_tests, "disasm: (arch:x64)");
  
  //- rjf: bitmaps
  unsigned int background_color = 0x00000000;
  unsigned int main_color       = 0xff2424ff;
  unsigned int shine_color      = 0xff5693ff;
  unsigned int shadow_color     = 0xff238faf;
  unsigned int bg = mule_bswap_u32(background_color);
  unsigned int cl = mule_bswap_u32(main_color);
  unsigned int sn = mule_bswap_u32(shine_color);
  unsigned int sh = mule_bswap_u32(shadow_color);
  unsigned int bitmap[] =
  {
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, cl, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, cl, cl, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, cl, cl, bg, cl, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, bg, bg, cl, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, cl, cl, cl, bg, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, cl, sn, sn, cl, cl, cl, sh, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, sh, sn, cl, cl, cl, sh, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, sh, cl, cl, cl, sh, sh, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, sh, sh, sh, bg, bg, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg,
    bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg, bg,
  };
  raddbg_pin(bitmap, "bitmap:(w:18, h:18)");
  for(int i = 0; i < sizeof(bitmap)/sizeof(bitmap[0]); i += 1)
  {
    unsigned int r = bitmap[i]&0x000000ff;
    unsigned int a = bitmap[i]&0xff000000;
    bitmap[i] = bitmap[i]>>8;
    bitmap[i] &= ~0xffff0000;
    bitmap[i] |= (r<<16);
    bitmap[i] |= (a);
  }
  for(int i = 0; i < sizeof(bitmap)/sizeof(bitmap[0]); i += 1)
  {
    unsigned int r = bitmap[i]&0x000000ff;
    unsigned int a = bitmap[i]&0xff000000;
    bitmap[i] = bitmap[i]>>8;
    bitmap[i] &= ~0xffff0000;
    bitmap[i] |= (r<<16);
    bitmap[i] |= (a);
  }
  for(int i = 0; i < sizeof(bitmap)/sizeof(bitmap[0]); i += 1)
  {
    unsigned int r = bitmap[i]&0x000000ff;
    unsigned int a = bitmap[i]&0xff000000;
    bitmap[i] = bitmap[i]>>8;
    bitmap[i] &= ~0xffff0000;
    bitmap[i] |= (r<<16);
    bitmap[i] |= (a);
  }
  int x2 = 0;
  
  //- rjf: 3D geometry
  float vertex_data[] = // pos.x, pos.y, pos.z, nor.x, nor.y, nor.z, tex.u, tex.v, col.r, col.g, col.b, ...
  {
    -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  2.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  8.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 10.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f,  0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  2.0f,  2.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  8.0f,  2.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  2.0f,  8.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  8.0f,  8.0f,  0.973f,  0.480f,  0.002f,
    -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 10.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  2.0f, 10.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,  8.0f, 10.0f,  0.973f,  0.480f,  0.002f,
    1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 10.0f, 10.0f,  0.973f,  0.480f,  0.002f,
    1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  1.0f, -0.6f,  1.0f,  0.0f,  0.0f,  2.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  8.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 10.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  2.0f,  2.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  8.0f,  2.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  2.0f,  8.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  8.0f,  8.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 10.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -1.0f, -0.6f,  1.0f,  0.0f,  0.0f,  2.0f, 10.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  8.0f, 10.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 10.0f, 10.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  2.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  8.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 10.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  2.0f,  2.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  8.0f,  2.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  2.0f,  8.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  8.0f,  8.0f,  0.612f,  0.000f,  0.069f,
    1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 10.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  2.0f, 10.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  8.0f, 10.0f,  0.612f,  0.000f,  0.069f,
    -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 10.0f, 10.0f,  0.612f,  0.000f,  0.069f,
    -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  1.0f,  0.6f, -1.0f,  0.0f,  0.0f,  2.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  1.0f, -0.6f, -1.0f,  0.0f,  0.0f,  8.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 10.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  2.0f,  2.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  8.0f,  2.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  2.0f,  8.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  8.0f,  8.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 10.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -1.0f,  0.6f, -1.0f,  0.0f,  0.0f,  2.0f, 10.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -1.0f, -0.6f, -1.0f,  0.0f,  0.0f,  8.0f, 10.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 10.0f, 10.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  2.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  8.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 10.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f,  0.6f,  0.0f,  1.0f,  0.0f,  2.0f,  2.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f,  0.6f,  0.0f,  1.0f,  0.0f,  8.0f,  2.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f, -0.6f,  0.0f,  1.0f,  0.0f,  2.0f,  8.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f, -0.6f,  0.0f,  1.0f,  0.0f,  8.0f,  8.0f,  0.000f,  0.254f,  0.637f,
    -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 10.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  2.0f, 10.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  8.0f, 10.0f,  0.000f,  0.254f,  0.637f,
    1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 10.0f, 10.0f,  0.000f,  0.254f,  0.637f,
    -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  2.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  8.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 10.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f, -0.6f,  0.0f, -1.0f,  0.0f,  2.0f,  2.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f, -0.6f,  0.0f, -1.0f,  0.0f,  8.0f,  2.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f,  0.6f,  0.0f, -1.0f,  0.0f,  2.0f,  8.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f,  0.6f,  0.0f, -1.0f,  0.0f,  8.0f,  8.0f,  0.001f,  0.447f,  0.067f,
    -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 10.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  2.0f, 10.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  8.0f, 10.0f,  0.001f,  0.447f,  0.067f,
    1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 10.0f, 10.0f,  0.001f,  0.447f,  0.067f,
    -0.6f,  0.6f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f,  0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -0.6f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  0.6f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -0.6f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -0.6f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f, -0.6f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    -0.6f,  0.6f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  0.6f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    0.6f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.973f,  0.480f,  0.002f,
    1.0f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    1.0f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.897f,  0.163f,  0.011f,
    0.6f,  0.6f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f,  0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -0.6f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -0.6f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f, -0.6f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    0.6f,  0.6f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  0.6f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -0.6f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.612f,  0.000f,  0.069f,
    -1.0f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f, -0.6f,  0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f, -0.6f, -0.6f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  0.6f,  0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -1.0f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f,  0.6f, -0.6f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.127f,  0.116f,  0.408f,
    -0.6f,  1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f,  1.0f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  1.0f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    0.6f,  0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.000f,  0.254f,  0.637f,
    -0.6f, -0.6f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -0.6f, -0.6f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -0.6f,  0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -0.6f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f, -0.6f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -0.6f, -0.6f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -1.0f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    -0.6f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -0.6f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
    0.6f, -1.0f,  0.6f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.001f,  0.447f,  0.067f,
  };
  unsigned int index_data[] =
  {
    0,   1,   9,   9,   8,   0,   1,   2,   5,   5,   4,   1,   6,   7,  10,  10,   9,   6,   2,   3,  11,  11,  10,   2,
    12,  13,  21,  21,  20,  12,  13,  14,  17,  17,  16,  13,  18,  19,  22,  22,  21,  18,  14,  15,  23,  23,  22,  14,
    24,  25,  33,  33,  32,  24,  25,  26,  29,  29,  28,  25,  30,  31,  34,  34,  33,  30,  26,  27,  35,  35,  34,  26,
    36,  37,  45,  45,  44,  36,  37,  38,  41,  41,  40,  37,  42,  43,  46,  46,  45,  42,  38,  39,  47,  47,  46,  38,
    48,  49,  57,  57,  56,  48,  49,  50,  53,  53,  52,  49,  54,  55,  58,  58,  57,  54,  50,  51,  59,  59,  58,  50,
    60,  61,  69,  69,  68,  60,  61,  62,  65,  65,  64,  61,  66,  67,  70,  70,  69,  66,  62,  63,  71,  71,  70,  62,
    72,  73,  74,  74,  75,  72,  76,  77,  78,  78,  79,  76,  80,  81,  82,  82,  83,  80,  84,  85,  86,  86,  87,  84,
    88,  89,  90,  90,  91,  88,  92,  93,  94,  94,  95,  92,  96,  97,  98,  98,  99,  96, 100, 101, 102, 102, 103, 100,
    104, 105, 106, 106, 107, 104, 108, 109, 110, 110, 111, 108, 112, 113, 114, 114, 115, 112, 116, 117, 118, 118, 119, 116,
    120, 121, 122, 122, 123, 120, 124, 125, 126, 126, 127, 124, 128, 129, 130, 130, 131, 128, 132, 133, 134, 134, 135, 132,
    136, 137, 138, 138, 139, 136, 140, 141, 142, 142, 143, 140, 144, 145, 146, 146, 147, 144, 148, 149, 150, 150, 151, 148,
    152, 153, 154, 154, 155, 152, 156, 157, 158, 158, 159, 156, 160, 161, 162, 162, 163, 160, 164, 165, 166, 166, 167, 164,
  };
  raddbg_pin(index_data, "geo: { count:(sizeof index_data / 4), vertices_base:(vertex_data), vertices_size:(sizeof vertex_data) }");
  int x3 = 0;
}

////////////////////////////////
// NOTE(allen): Function Overload Resolution

static int
overloaded_function(float y){
  int r = (int)(y + 0.5f);
  return(r);
}

static int
overloaded_function(float y, int x){
  int r = overloaded_function(y) + x;
  return(r);
}

static int
overloaded_function(int x){
  float y = (float)x;
  int r = overloaded_function(y, 1);
  return(r);
}

////////////////////////////////
// NOTE(allen): Control Flow Stepping

static void
control_flow_stepping_tests(void){
  {
    int a = 1;
    if (a < 1){
      a += 1;
    }
    if (a < 2){
      a += 2;
    }
  }
  
  {
    int a = 1;
    if (a < 1)
    {
      a += 1;
    }
    if (a < 2)
    {
      a += 2;
    }
  }
  
  {
    int a = 1;
    if (a < 1)
      a += 1;
    if (a < 2)
      a += 2;
  }
  
  {
    int a = 1;
    int b = 2;
    if (a <= b){
      if (a == b){
        b += 1;
      }
      else{
        a += 1;
      }
    }
    else{
      if (a%2){
        a = b;
      }
      else{
        a = b - 1;
      }
    }
  }
  
  {
    int a = 1;
    int b = 2;
    if (a <= b)
    {
      if (a == b)
      {
        b += 1;
      }
      else
      {
        a += 1;
      }
    }
    else
    {
      if (a%2)
      {
        a = b;
      }
      else
      {
        a = b - 1;
      }
    }
  }
  
  {
    int a = 1;
    int b = 2;
    if (a <= b)
      if (a == b)
      b += 1;
    else
      a += 1;
    else
      if (a%2)
      a = b;
    else
      a = b - 1;
  }
  
  {
    int x = 0;
    for (int i = 0; i < 10; i += 1){
      x += i;
    }
  }
  
  {
    int x = 0;
    for (int i = 0; i < 10; i += 1)
    {
      x += i;
    }
  }
  
  {
    int x = 0;
    for (int i = 0; i < 10; i += 1)
      x += i;
  }
  
  {
    int x = 0;
    for (int i = 0; i < 10; i += 1) x += i;
  }
  
  {
    int a = 1;
    for (;a < 10;){
      switch (a){
        case 0: case 1: case 2:
        {
          a += 2;
        }break;
        
        default:
        case 4:
        case 5:
        {
          a += 1;
        }break;
        
        case 6: a += 1; break;
        case 7: a += 1;
        case 8:
        case 9: a += 1;
      }
    }
  }
  
  {
    int i = 0;
    while (i < 5){
      i += 1;
    }
    
    while (i < 10)
    {
      i += 1;
    }
    
    while (i < 15)
      i += 1;
    
    while (i < 20) i += 1;
  }
  
  {
    int i = 0;
    do
    {
      i += 1;
    } while (i < 10);
  }
  
  {
    int i = 17;
    
    check_again:
    if (i <= 1) goto done;
    if ((i&1) == 0) goto even_case;
    
    odd_case:
    i = 3*i + 1;
    
    even_case:
    i /= 2;
    goto check_again;
    
    done:;
  }
  
  {
    int x = 15;
    label_same_line:; x -= 1; if(x > 0) { goto label_same_line; } else { goto end_label_same_line; }
  }
  end_label_same_line:;
}

////////////////////////////////
// NOTE(allen): Indirect Call/Jump Stepping Tests

typedef int FunctionType(int);

static int
function_foo(int a){
  if (a < 1){
    a += 1;
  }
  if (a < 2){
    a += 2;
  }
  return(a);
}

static int
function_bar(int x){
  for (int i = 0; i < 10; i += 1){
    x += i;
  }
  return(x);
}


static void
indirect_call_jump_stepping_tests(void){
  int z = 1;
  FunctionType *ptr = function_foo;
  z = ptr(z);
  if ((z & 1) == 0){
    ptr = function_bar;
  }
  z = ptr(z);
  
  switch (z&7){
    case 0:
    {
      z += 2;
      ptr = function_bar;
    }break;
    
    case 1:
    {
      z += 1;
      ptr = function_bar;
    }break;
    
    case 2:
    {
      z *= 2;
      ptr = function_bar;
    }break;
    
    case 3:
    {
      z -= 10;
      ptr = function_foo;
    }break;
    
    case 4:
    {
      z -= 5;
      ptr = function_foo;
    }break;
    
    case 5:
    {
      z = z ^ 0x10;
      ptr = function_foo;
    }break;
    
    case 6:
    {
      z = z & ~0x10;
      ptr = function_foo;
    }break;
    
    case 7:
    {
      z = z | 0x10;
      ptr = function_foo;
    }break;
  }
  
  z = ptr(z);
}

////////////////////////////////
// NOTE(rjf): alloca (Variable-Width Stack Changes) Stepping Tests

static void
alloca_stepping_tests(void)
{
  int x = 1;
  int y = 3;
  int z = 5;
  
#if _WIN32
  int *mem = (int *)_alloca((x+y+z)*sizeof(int));
  mem[0] = x;
  mem[1] = y;
  mem[2] = z;
#else
  int *mem = (int *)__builtin_alloca((x+y+z)*sizeof(int));
  mem[0] = x;
  mem[1] = y;
  mem[2] = z;
#endif
}

////////////////////////////////
// NOTE(allen): Overloaded Line Stepping

static int
function_get_integer(void){
  return(1);
}

static void
function_with_multiple_parameters(int x, int y){
  x += y;
}

static int
recursive_single_line(int x){ return(x <= 1?0:x + recursive_single_line(x/2)); }

static int shared_1(int x) { return(x); } static int shared_2(int x) { return(1 + shared_1(x)); }

static void
overloaded_line_stepping_tests(void){
  function_with_multiple_parameters(function_get_integer(), function_get_integer());
  function_with_multiple_parameters(function_get_integer(), function_get_integer());
  function_with_multiple_parameters(function_get_integer(), function_get_integer());
  
  recursive_single_line(50);
  recursive_single_line(50);
  recursive_single_line(50);
  
  shared_2(5);
  shared_2(5);
  shared_2(5);
  
  function_get_integer(); shared_1(1); shared_1(2);
  
  if ((shared_2(10) && shared_2(-1)) ||
      shared_2(function_get_integer())){
    int x = 0;
  }
  else{
    int y = 0;
  }
}

////////////////////////////////
// NOTE(allen): Long Jump Stepping

#include <setjmp.h>

static jmp_buf global_jump_buffer;
static int global_jump_x;

static void
long_jump_from_function(void){
  int spin = 0;
  for (; spin < 5; spin += 1);
  longjmp(global_jump_buffer, 2);
  global_jump_x = spin;
}

static void
long_jump_wrapped_in_function(void){
  global_jump_x = 0;
  int val = setjmp(global_jump_buffer);
  if (val == 0){
    global_jump_x = 1;
    longjmp(global_jump_buffer, 1);
  }
  else if (val == 1){
    if (global_jump_x == 1){
      global_jump_x = 2;
      long_jump_from_function();
    }
  }
  else if (val == 2){
    global_jump_x = 3;
  }
}

static void
long_jump_stepping_tests(void){
  
  long_jump_wrapped_in_function();
  
  long_jump_wrapped_in_function();
  
  long_jump_wrapped_in_function();
  
}

////////////////////////////////
// NOTE(allen): Recursion Stepping

static int
recursive_call(int x){
  if (x <= 1){
    return(x);
  }
  
  int r1 = recursive_call(x - 1);
  int r2 = recursive_call(x - 2);
  return(r1 + r2);
}

static int
tail_recursive_call(int x, int m){
  if (x <= 1){
    return(m);
  }
  return(tail_recursive_call(x - 1, x*m));
}

static void
recursion_stepping_tests(void){
  
  recursive_call(4);
  
  recursive_call(4);
  
  tail_recursive_call(5, 1);
  
  tail_recursive_call(5, 1);
  
}

////////////////////////////////
// NOTE(rjf): Debug Strings

static void
debug_string_tests(void)
{
#if _WIN32
  for(int i = 0; i < 100; i += 1)
  {
    OutputDebugStringA("Hello, World!\n");
  }
#endif
}

////////////////////////////////
// NOTE(allen): Exception Stepping

int *global_null_read_pointer = 0;
static void
trip(void){
  *global_null_read_pointer = 0;
}

static void
cpp_exception_in_function(void){
  int v = 0;
  try{
    throw 1;
  }
  catch (...){
    v = 1;
  }
}

static void
cpp_throw_in_function(void){
  throw 1;
}

static void
win32_exception_in_function(void){
#if _WIN32
  int v = 0;
  __try{
    trip();
    v = 1;
  }
  __except (EXCEPTION_EXECUTE_HANDLER){
    v = 2;
  }
  
  v = 3;
  __try{
    trip();
    v = 4;
  }
  __except (EXCEPTION_EXECUTE_HANDLER){
    v = 5;
  }
#endif
}

static void
cpp_recursive_exception(int x){
  try{
    if (x > 1){
      throw 1;
    }
  }
  catch (...){
    x -= 1;
    cpp_recursive_exception(x);
    x += 1;
  }
}

static void
win32_recursive_exception(int x){
#if _WIN32
  __try{
    if (x > 1){
      throw 1;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER){
    x -= 1;
    win32_recursive_exception(x);
    x += 1;
  }
#endif
}

static void
exception_stepping_tests(void){
  {
    int v = 0;
    try{
      throw 1;
    }
    catch (...){
      v = 1;
    }
  }
  
  {
    int v = 0;
    try{
      cpp_throw_in_function();
    }
    catch (...){
      v = 1;
    }
  }
  
  cpp_exception_in_function();
  cpp_exception_in_function();
  
#if _WIN32
  win32_exception_in_function();
  win32_exception_in_function();
#endif
  
  // NOTE(allen): Exception in catch tests
  {
    int v = 0;
    try{
      v = 1;
      throw 1;
    }
    catch (...){
      try{
        v = 2;
        throw 2;
      }
      catch (...){
        v = 3;
      }
    }
  }
  
  {
    int v = 0;
    try{
      v = 1;
      throw 1;
    }
    catch (...){
      cpp_exception_in_function();
    }
  }
  
#if _WIN32
  {
    int v = 0;
    try{
      v = 1;
      throw 1;
    }
    catch (...){
      win32_exception_in_function();
    }
  }
#endif
  
  cpp_recursive_exception(4);
  cpp_recursive_exception(4);
  cpp_recursive_exception(4);
  
#if _WIN32
  win32_recursive_exception(4);
  win32_recursive_exception(4);
  win32_recursive_exception(4);
#endif
  
  // NOTE(allen): Try in try tests
  {
    int v = 0;
    try{
      try{
        v = 1;
        throw 1;
      }
      catch (...){
        v = 2;
      }
      throw 2;
    }
    catch (...){
      v = 3;
    }
  }
  
  {
    int v = 0;
    try{
      try{
        v = 1;
        cpp_throw_in_function();
      }
      catch (...){
        v = 2;
      }
      throw 2;
    }
    catch (...){
      v = 3;
    }
  }
  
  {
    int v = 0;
    try{
      cpp_exception_in_function();
      throw 2;
    }
    catch (...){
      v = 3;
    }
  }
  
#if _WIN32
  {
    int v = 0;
    try{
      win32_exception_in_function();
      throw 2;
    }
    catch (...){
      v = 3;
    }
  }
#endif
  
}

typedef void (*callback_t)(int a);
static void
dynamic_step_test(void){
#if _WIN32
#if defined(_x86_64) || defined( __x86_64__ ) || defined( _M_X64 ) || defined( _M_AMD64 )
	void *page = VirtualAlloc(0, 4096, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  char *ptr = (char*)page;
  *ptr++ =  0x51; // push rcx
  *ptr++ = 0x59; // pop rcx
  *ptr++ = 0xC3; // ret
	callback_t cb = (callback_t)page;
	cb(1);
#endif
#endif
}

////////////////////////////////

int
mule_main(int argc, char** argv){
  mule_init();
  
  // NOTE(allen): Eval Tests
  type_coverage_eval_tests();
  
  mutating_variables_eval_tests();
  
  nested_types_eval_tests();
  
  struct_parameters_eval_tests();
  
  global_eval_tests();
  
  return_eval_tests();
  
  tls_eval_tests();
  
  complicated_type_coverage_tests();
  
  extended_type_coverage_eval_tests();
  
  templated_function_eval_tests();
  
  c_type_coverage_eval_tests();
  
  c_type_with_bitfield_usage();
  
  optimized_build_eval_tests();
  
  optimized_struct_parameters_eval_tests();
  
  fancy_viz_eval_tests();
  
  // NOTE(allen): Stepping Tests
  control_flow_stepping_tests();
  
  indirect_call_jump_stepping_tests();
  
  alloca_stepping_tests();
  
  inline_stepping_tests();
  
  overloaded_line_stepping_tests();
  
  overloaded_function(100);
  
  dynamic_step_test();
  
  long_jump_stepping_tests();
  
  recursion_stepping_tests();
  
  debug_string_tests();
  
  exception_stepping_tests();
  
  return(0);
}


