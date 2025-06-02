// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "convertperf"

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "path/path.h"
#include "async/async.h"
#include "rdi_format/rdi_format_local.h"
#include "dbgi/dbgi.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "async/async.c"
#include "rdi_format/rdi_format_local.c"
#include "dbgi/dbgi.c"

////////////////////////////////
//~ rjf: Entry Points

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  String8 list_path = str8_list_first(&cmdline->inputs);
  String8 list_data = os_data_from_file_path(arena, list_path);
  U8 splits[] = {'\n'};
  String8List lines = str8_split(arena, list_data, splits, ArrayCount(splits), 0);
  OS_HandleList processes = {0};
  String8Node *processes_first_path_n = 0;
  U64 limit = 64;
  U64 idx = 0;
  for(String8Node *n = lines.first; n != 0; n = n->next)
  {
    String8 dll_path = n->string;
    ProfScope("kick off %.*s", str8_varg(dll_path))
    {
      String8 dll_path_no_ext = str8_chop_last_dot(dll_path);
      String8 dll_name = str8_skip_last_slash(dll_path_no_ext);
      String8 pdb_path = push_str8f(arena, "%S.pdb", dll_path_no_ext);
      String8 rdi_path = push_str8f(arena, "dump/%S.rdi", dll_name);
      OS_Handle handle = os_cmd_line_launchf("raddbg --convert --pdb:%S --out:%S", pdb_path, rdi_path);
      os_handle_list_push(arena, &processes, handle);
      if(processes_first_path_n == 0)
      {
        processes_first_path_n = n;
      }
      idx += 1;
    }
    if(idx >= limit)
    {
      String8Node *line_n = processes_first_path_n;
      for(OS_HandleNode *n = processes.first; n != 0; n = n->next, line_n = line_n->next)
      {
        ProfScope("join %.*s", str8_varg(line_n->string))
        {
          os_process_join(n->v, max_U64);
        }
      }
      idx = 0;
      MemoryZeroStruct(&processes);
      processes_first_path_n = 0;
    }
  }
}
