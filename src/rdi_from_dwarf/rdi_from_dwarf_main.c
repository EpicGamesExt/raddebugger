// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_TITLE "Epic Games Tools (R) DWARF Converter"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////

#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"

////////////////////////////////

#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"
#include "lib_rdi_format/rdi_format_parse.h"
#include "lib_rdi_format/rdi_format_parse.c"

////////////////////////////////

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "async/async.h"
#include "rdi_make/rdi_make_local.h"
#include "linker/path_ext/path.h"
#include "linker/hash_table.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "dwarf/dwarf.h"
#include "dwarf/dwarf_parse.h"
#include "dwarf/dwarf_coff.h"
#include "pe/pe.h"
#include "linker/rdi/rdi_coff.h"
#include "rdi_from_dwarf/rdi_from_dwarf.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "async/async.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "rdi_make/rdi_make_local.c"
#include "linker/rdi/rdi_coff.c"
#include "linker/path_ext/path.c"
#include "linker/hash_table.c"
#include "dwarf/dwarf.c"
#include "dwarf/dwarf_parse.c"
#include "dwarf/dwarf_coff.c"
#include "rdi_from_dwarf/rdi_from_dwarf.c"

////////////////////////////////
// Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  // initialize state and unpack command line
  Arena *arena = arena_alloc();
  B32 do_help = (cmd_line_has_flag(cmdline, str8_lit("help")) ||
                 cmd_line_has_flag(cmdline, str8_lit("h"))    ||
                 cmd_line_has_flag(cmdline, str8_lit("?")));
  
  D2R_User2Convert *user2convert = d2r_user2convert_from_cmdln(arena, cmdline);
  
  // display help
  if (do_help) {
    fprintf(stderr, "--- rdi_from_dwarf ------------------------------------------------------------\n\n");
    
    fprintf(stderr, "This utility converts debug information from DWARF into the RAD Debug Info\n");
    fprintf(stderr, "format. The following arguments are accepted:\n\n");
    
    fprintf(stderr, "--exe:<path> [optional] Specifies the path of the executable filefor which the\n");
    fprintf(stderr, "                        debug info was generated.\n");
    fprintf(stderr, "--debug:<path>          Specifies the path of the .DEBUG debug info file to\n");
    fprintf(stderr, "                        convert.\n");
    fprintf(stderr, "--out:<path>            Specifies the path at which the output will be written.\n\n");
    
    if (!do_help) {
      for (String8Node *n = user2convert->errors.first; n != 0; n = n->next) {
        fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
      }
    }
    
    os_abort(0);
  }
  
  RDIM_LocalState *rdim_local_state = rdim_local_init();
  
  ProfBegin("convert");
  RDIM_BakeParams *convert2bake = d2r_convert(arena, user2convert);
  ProfEnd();
  
  ProfBegin("bake");
  RDIM_BakeResults bake2srlz = d2r_bake(rdim_local_state, convert2bake);
  ProfEnd();
  
  ProfBegin("serialize bake");
  RDIM_SerializedSectionBundle srlz2file = rdim_serialized_section_bundle_from_bake_results(&bake2srlz);
  ProfEnd();
  
  RDIM_SerializedSectionBundle srlz2file_compressed = srlz2file;
  if (cmd_line_has_flag(cmdline, str8_lit("compress"))) {
    ProfBegin("compress");
    srlz2file_compressed = d2r_compress(arena, srlz2file);
    ProfEnd();
  }
  
  ProfBegin("serialize blobs");
  String8List blobs = rdim_file_blobs_from_section_bundle(arena, &srlz2file_compressed);
  ProfEnd();
  
  ProfBegin("write");
  if (!os_write_data_list_to_file_path(user2convert->output_name, blobs)) {
    fprintf(stderr, "error(ouptut): unable to write to %.*s\n", str8_varg(user2convert->output_name));
  }
  ProfEnd();
}

