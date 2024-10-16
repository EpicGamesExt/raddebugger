// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 12
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "rdi_from_pdb"
#define BUILD_CONSOLE_INTERFACE 1

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
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_pdb.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "rdi_make/rdi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_pdb.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  //- rjf: initialize state, unpack command line
  Arena *arena = arena_alloc();
  B32 do_help = (cmd_line_has_flag(cmdline, str8_lit("help")) ||
                 cmd_line_has_flag(cmdline, str8_lit("h")) ||
                 cmd_line_has_flag(cmdline, str8_lit("?")));
  P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(arena, cmdline);
  
  //- rjf: display help
  if(do_help || user2convert->errors.node_count != 0)
  {
    fprintf(stderr, "--- rdi_from_pdb --------------------------------------------------------------\n\n");
    
    fprintf(stderr, "This utility converts debug information from PDBs into the RAD Debug Info\n");
    fprintf(stderr, "format. The following arguments are accepted:\n\n");
    
    fprintf(stderr, "--exe:<path> [optional] Specifies the path of the executable file for which the\n");
    fprintf(stderr, "                        debug info was generated.\n");
    fprintf(stderr, "--pdb:<path>            Specifies the path of the PDB debug info file to\n");
    fprintf(stderr, "                        convert.\n");
    fprintf(stderr, "--out:<path>            Specifies the path at which the output Breakpad debug\n");
    fprintf(stderr, "                        info will be written.\n\n");
    
    if(!do_help)
    {
      for(String8Node *n = user2convert->errors.first; n != 0; n = n->next)
      {
        fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
      }
    }
    os_abort(0);
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
  P2R_Serialize2File *srlz2file = 0;
  ProfScope("serialize")
  {
    srlz2file = push_array(arena, P2R_Serialize2File, 1);
    srlz2file->bundle = rdim_serialized_section_bundle_from_bake_results(&bake2srlz->bake_results);
  }
  
  //- rjf: compress
  P2R_Serialize2File *srlz2file_compressed = srlz2file;
  if(cmd_line_has_flag(cmdline, str8_lit("compress"))) ProfScope("compress")
  {
    srlz2file_compressed = push_array(arena, P2R_Serialize2File, 1);
    srlz2file_compressed = p2r_compress(arena, srlz2file);
  }
  
  //- rjf: serialize
  String8List blobs = rdim_file_blobs_from_section_bundle(arena, &srlz2file_compressed->bundle);
  
  //- rjf: write
  ProfScope("write")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, user2convert->output_name);
    U64 off = 0;
    for(String8Node *n = blobs.first; n != 0; n = n->next)
    {
      os_file_write(output_file, r1u64(off, off+n->string.size), n->string.str);
      off += n->string.size;
    }
    os_file_close(output_file);
  }
}
