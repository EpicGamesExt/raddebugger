// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "lib_raddbgi_format/raddbgi_format.h"
#include "lib_raddbgi_format/raddbgi_format.c"

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "raddbgi_cons_local/raddbgi_cons_local.h"

#include "raddbgi_from_pdb.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "raddbgi_cons_local/raddbgi_cons_local.c"

#include "raddbgi_from_pdb.c"

int
main(int argc, char **argv){
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
  
  Arena *arena = arena_alloc();
  String8List args = os_string_list_from_argcv(arena, argc, argv);
  CmdLine cmdline = cmd_line_from_string_list(arena, args);
  
  ProfBeginCapture("raddbgi_from_pdb");
  
  //- rjf: parse arguments
  P2R_Params *params = p2r_params_from_cmd_line(arena, &cmdline);
  
  //- rjf: show input errors
  if (params->errors.node_count > 0 &&
      !params->hide_errors.input){
    for (String8Node *node = params->errors.first;
         node != 0;
         node = node->next){
      fprintf(stderr, "error(input): %.*s\n", str8_varg(node->string));
    }
  }
  
  //- rjf: open output file
  String8 output_name = push_str8_copy(arena, params->output_name);
  FILE *out_file = fopen((char*)output_name.str, "wb");
  if(out_file == 0 && !params->hide_errors.output)
  {
    fprintf(stderr, "error(output): could not open output file\n");
  }
  
  //- rjf: convert
  P2R_Out *out = 0;
  if(out_file != 0)
  {
    out = p2r_convert(arena, params);
  }
  
  //- rjf: print dump
  if(out != 0)
  {
    for(String8Node *node = out->dump.first; node != 0; node = node->next)
    {
      fwrite(node->string.str, 1, node->string.size, stdout);
    }
  }
  
  //- rjf: bake file
  if(out != 0 && out->good_parse && params->output_name.size > 0 && out->good_parse)
  {
    String8List baked = {0};
    rdim_bake_file(arena, out->root, &baked);
    for(String8Node *node = baked.first; node != 0; node = node->next)
    {
      fwrite(node->string.str, node->string.size, 1, out_file);
    }
  }
  
  //- rjf: close output file
  if(out_file != 0)
  {
    fclose(out_file);
  }
  
  ProfEndCapture();
  return(0);
}
