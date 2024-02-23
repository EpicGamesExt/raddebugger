// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- rjf: [lib]
#include "lib_raddbgi_format/raddbgi_format.h"
#include "lib_raddbgi_format/raddbgi_format.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "raddbgi_make_local/raddbgi_make_local.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "raddbgi_from_pdb.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "raddbgi_make_local/raddbgi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "raddbgi_from_pdb.c"

//- rjf: entry point

int
main(int argc, char **argv)
{
  local_persist TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
#if PROFILE_TELEMETRY
  U64 tm_data_size = MB(512);
  U8 *tm_data = os_reserve(tm_data_size);
  os_commit(tm_data, tm_data_size);
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(1024);
  tmInitialize(tm_data_size, tm_data);
#endif
  ThreadNameF("[main]");
  
  //- rjf: initialize dependencies
  os_init(argc, argv);
  ts_init();
  
  //- rjf: initialize state, parse command line
  Arena *arena = arena_alloc();
  String8List args = os_string_list_from_argcv(arena, argc, argv);
  CmdLine cmdline = cmd_line_from_string_list(arena, args);
  B32 should_capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
  P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(arena, &cmdline);
  
  //- rjf: begin capture
  if(should_capture)
  {
    ProfBeginCapture(argv[0]);
  }
  
  //- rjf: display errors with input
  if(user2convert->errors.node_count > 0 && !user2convert->hide_errors.input)
  {
    for(String8Node *n = user2convert->errors.first; n != 0; n = n->next)
    {
      fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
    }
  }
  
  //- rjf: convert
  P2R_Convert2Bake *convert2bake = 0;
  ProfScope("convert")
  {
    convert2bake = p2r_convert(arena, user2convert);
  }
  
  //- rjf: bake
  P2R_Bake2Serialize *bake2srlz = 0;
  ProfScope("bake")
  {
    bake2srlz = p2r_bake(arena, convert2bake);
  }
  
  //- rjf: serialize
  String8List serialize_out = rdim_serialized_strings_from_params_bake_section_list(arena, &convert2bake->bake_params, &bake2srlz->sections);
  
  //- rjf: write
  ProfScope("write")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, user2convert->output_name);
    U64 off = 0;
    for(String8Node *n = serialize_out.first; n != 0; n = n->next)
    {
      os_file_write(output_file, r1u64(off, off+n->string.size), n->string.str);
      off += n->string.size;
    }
    os_file_close(output_file);
  }
  
  //- rjf: end capture
  if(should_capture)
  {
    ProfEndCapture();
  }
  
  return(0);
}
