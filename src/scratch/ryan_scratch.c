// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "ryan_scratch"
// #define OS_FEATURE_GRAPHICAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "rdi_make/rdi_make_local.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "rdi_make/rdi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"

////////////////////////////////
//~ rjf: Entry Point

typedef struct FooBar FooBar;
struct FooBar
{
  U64 x;
  U64 y;
  U64 z;
  String8 name;
};

//-

Type String8__str_ptr_type = {TypeKind_Ptr, 0, sizeof(void *), &type_leaves[TypeKind_U8], {0}, str8_lit_comp("size")};

Member String8__members[] =
{
  {str8_lit_comp("str"),  &String8__str_ptr_type,     OffsetOf(String8, str)},
  {str8_lit_comp("size"), &type_leaves[TypeKind_U64], OffsetOf(String8, size)},
};

Type String8__type =
{
  TypeKind_Struct,
  0,
  sizeof(String8),
  &type_leaves[0],
  str8_lit_comp("String8"),
  {0},
  ArrayCount(String8__members),
  String8__members,
};

//-

extern Type String8Node__type;
Type String8Node__ptr_type = {TypeKind_Ptr, 0, sizeof(void *), &String8Node__type};

Member String8Node__members[] =
{
  {str8_lit_comp("next"),   &String8Node__ptr_type,     OffsetOf(String8Node, next)},
  {str8_lit_comp("string"), type(String8),              OffsetOf(String8Node, string)},
};

Type String8Node__type =
{
  TypeKind_Struct,
  0,
  sizeof(String8Node),
  &type_leaves[0],
  str8_lit_comp("String8Node"),
  {0},
  ArrayCount(String8Node__members),
  String8Node__members,
};

//-

Member String8List__members[] =
{
  {str8_lit_comp("first"),      &String8Node__ptr_type,     OffsetOf(String8List, first)},
  {str8_lit_comp("last"),       &String8Node__ptr_type,     OffsetOf(String8List, last), MemberFlag_DoNotSerialize},
  {str8_lit_comp("node_count"), &type_leaves[TypeKind_U64], OffsetOf(String8List, node_count)},
  {str8_lit_comp("total_size"), &type_leaves[TypeKind_U64], OffsetOf(String8List, total_size)},
};

Type String8List__type =
{
  TypeKind_Struct,
  0,
  sizeof(String8List),
  &type_leaves[0],
  str8_lit_comp("String8List"),
  {0},
  ArrayCount(String8List__members),
  String8List__members,
};

//-

Member FooBar__members[] =
{
  {str8_lit_comp("x"),    &type_leaves[TypeKind_U64], OffsetOf(FooBar, x)},
  {str8_lit_comp("y"),    &type_leaves[TypeKind_U64], OffsetOf(FooBar, y)},
  {str8_lit_comp("z"),    &type_leaves[TypeKind_U64], OffsetOf(FooBar, z)},
  {str8_lit_comp("name"), type(String8),              OffsetOf(FooBar, name)},
};

Type FooBar__type =
{
  TypeKind_Struct,
  0,
  sizeof(FooBar),
  &type_leaves[0],
  str8_lit_comp("FooBar"),
  {0},
  ArrayCount(FooBar__members),
  FooBar__members,
};

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  
  String8List strs = {0};
  str8_list_pushf(arena, &strs, "foobar: %i", 123);
  str8_list_pushf(arena, &strs, "xyzxyzxyz");
  str8_list_pushf(arena, &strs, "abc abc abc");
  str8_list_pushf(arena, &strs, "123");
  str8_list_pushf(arena, &strs, "456");
  str8_list_pushf(arena, &strs, "789");
  str8_list_pushf(arena, &strs, "111");
  str8_list_pushf(arena, &strs, "222");
  str8_list_pushf(arena, &strs, "333");
  String8 strs_serialized = serialized_from_struct(arena, String8List, &strs);
  
  String8 test_name = str8_lit("foobar 123");
  String8 test_name_serialized = serialized_from_struct(arena, String8, &test_name);
  
  FooBar foobar = {1, 2, 3, str8_lit("foobar 123 hello world")};
  String8 foobar_serialized = serialized_from_struct(arena, FooBar, &foobar);
  
  int x = 0;
  
#if 0
  OS_Handle window = os_window_open(v2f32(1280, 720), 0, str8_lit("Window"));
  os_window_first_paint(window);
  for(B32 quit = 0; !quit;)
  {
    Temp scratch = scratch_begin(0, 0);
    OS_EventList events = os_get_events(scratch.arena, 0);
    for(OS_Event *ev = events.first; ev != 0; ev = ev->next)
    {
      if(ev->kind != OS_EventKind_MouseMove)
      {
        printf("%.*s (%.*s)\n", str8_varg(os_string_from_event_kind(ev->kind)), str8_varg(os_g_key_display_string_table[ev->key]));
        fflush(stdout);
      }
    }
    for(OS_Event *ev = events.first; ev != 0; ev = ev->next)
    {
      if(ev->kind == OS_EventKind_WindowClose)
      {
        quit = 1;
        break;
      }
    }
    scratch_end(scratch);
  }
#endif
}
