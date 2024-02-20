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
  ThreadName("[main]");
  
  //- rjf: initialize dependencies
  os_init(argc, argv);
  ts_init();
  
  //- rjf: initialize state, parse command line
  Arena *arena = arena_alloc();
  String8List args = os_string_list_from_argcv(arena, argc, argv);
  CmdLine cmdline = cmd_line_from_string_list(arena, args);
  B32 should_capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
  P2R_ConvertIn *convert_in = p2r_convert_in_from_cmd_line(arena, &cmdline);
  
  //- rjf: begin capture
  if(should_capture)
  {
    ProfBeginCapture(argv[0]);
  }
  
  //- rjf: display errors with input
  if(convert_in->errors.node_count > 0 && !convert_in->hide_errors.input)
  {
    for(String8Node *n = convert_in->errors.first; n != 0; n = n->next)
    {
      fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
    }
  }
  
  //- rjf: convert
  P2R_ConvertOut *convert_out = 0;
  ProfScope("convert")
  {
    convert_out = p2r_convert(arena, convert_in);
  }
  
  //- rjf: bake
  String8List bake_strings = {0};
  ProfScope("bake")
  {
    RDIM_BakeParams bake_params = {0};
    {
      bake_params.top_level_info   = convert_out->top_level_info;
      bake_params.binary_sections  = convert_out->binary_sections;
      bake_params.units            = convert_out->units;
      bake_params.types            = convert_out->types;
      bake_params.udts             = convert_out->udts;
      bake_params.global_variables = convert_out->global_variables;
      bake_params.thread_variables = convert_out->thread_variables;
      bake_params.procedures       = convert_out->procedures;
      bake_params.scopes           = convert_out->scopes;
    }
    RDIM_BakeSectionList sections = rdim_bake_sections_from_params(arena, &bake_params);
    bake_strings = rdim_blobs_from_bake_sections(arena, &sections);
  }
  
  //- rjf: write
  ProfScope("write")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, convert_in->output_name);
    U64 off = 0;
    for(String8Node *n = bake_strings.first; n != 0; n = n->next)
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
