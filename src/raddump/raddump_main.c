// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD Dumper"

////////////////////////////////

#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"

////////////////////////////////

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "path/path.h"
#include "coff/coff.h"
#include "coff/coff_enum.h"
#include "pe/pe.h"
#include "msvc_crt/msvc_crt.h"
#include "msvc_crt/msvc_crt_enum.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "codeview/codeview_enum.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "coff/coff.c"
#include "coff/coff_enum.c"
#include "pe/pe.c"
#include "msvc_crt/msvc_crt.c"
#include "msvc_crt/msvc_crt_enum.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "codeview/codeview_enum.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
 
#include "linker/base_ext/base_core.h"
#include "linker/base_ext/base_core.c"
#include "linker/path_ext/path.h"
#include "linker/path_ext/path.c"
#include "linker/thread_pool/thread_pool.h"
#include "linker/thread_pool/thread_pool.c"
#include "linker/codeview_ext/codeview.h"
#include "linker/codeview_ext/codeview.c"
#include "linker/hash_table.h"
#include "linker/hash_table.c"

#include "raddump/raddump.h"
#include "raddump/raddump.c"

////////////////////////////////

global read_only struct
{
  RD_Option opt;
  char     *name;
  char     *help;
} g_rd_dump_option_map[] = {
  { RD_Option_Help,       "help",       "Print help and exit"                           },
  { RD_Option_Version,    "v",          "Print version and exit"                        },
  { RD_Option_Headers,    "headers",    "Dump DOS header, file header, optional header, and/or archive header" },
  { RD_Option_Sections,   "sections",   "Dump section headers as table"                 },
  { RD_Option_Rawdata,    "rawdata",    "Dump raw section data"                         },
  { RD_Option_Codeview,   "cv",         "Dump CodeView"                                 },
  { RD_Option_Disasm,     "disasm",     "Disassemble code sections"                     },
  { RD_Option_Symbols,    "symbols",    "Dump COFF symbol table"                        },
  { RD_Option_Relocs,     "relocs",     "Dump relocations"                              },
  { RD_Option_Exceptions, "exceptions", "Dump exceptions"                               },
  { RD_Option_Tls,        "tls",        "Dump Thread Local Storage directory"           },
  { RD_Option_Debug,      "debug",      "Dump debug directory"                          },
  { RD_Option_Imports,    "imports",    "Dump import table"                             },
  { RD_Option_Exports,    "exports",    "Dump export table"                             },
  { RD_Option_LoadConfig, "loadconfig", "Dump load config"                              },
  { RD_Option_Resources,  "resources",  "Dump resource directory"                       },
  { RD_Option_LongNames,  "longnames",  "Dump archive long names"                       },
};

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

  // parse options
  RD_Option opts = 0;
  {
    for (CmdLineOpt *cmd = cmdline->options.first; cmd != 0; cmd = cmd->next) {
      RD_Option opt = 0;
      for (U64 opt_idx = 0; opt_idx < ArrayCount(g_rd_dump_option_map); ++opt_idx) {
        String8 opt_name = str8_cstring(g_rd_dump_option_map[opt_idx].name);
        if (str8_match(cmd->string, opt_name, StringMatchFlag_CaseInsensitive)) {
          opt = g_rd_dump_option_map[opt_idx].opt;
          break;
        } else if (str8_match(cmd->string, str8_lit("all"), StringMatchFlag_CaseInsensitive)) {
          opt = ~0ull & ~(RD_Option_Help|RD_Option_Version);
          break;
        }
      }

      if (opt == 0) {
        fprintf(stderr, "Unknown argument: \"%.*s\"\n", str8_varg(cmd->string));
        os_abort(1);
      }

      opts |= opt;
    }
  }

  // print help
  if (opts & RD_Option_Help) {
    for (U64 opt_idx = 0; opt_idx < ArrayCount(g_rd_dump_option_map); ++opt_idx) {
      fprintf(stdout, "-%s %s\n", g_rd_dump_option_map[opt_idx].name, g_rd_dump_option_map[opt_idx].help);
    }
    os_abort(0);
  }

  // print version
  if (opts & RD_Option_Version) {
    fprintf(stdout, BUILD_TITLE_STRING_LITERAL "\n");
    fprintf(stdout, "\traddump <options> <inputs>\n");
    os_abort(0);
  }

  // input check
  if (cmdline->inputs.node_count == 0) {
    fprintf(stderr, "No input file specified\n");
    os_abort(1);
  } else if (cmdline->inputs.node_count > 1) {
    fprintf(stderr, "Too many inputs specified, expected one\n");
    os_abort(1);
  }

  // read input
  String8 file_path = str8_list_first(&cmdline->inputs);
  String8 raw_data  = os_data_from_file_path(scratch.arena, file_path);

  // is read ok?
  if (raw_data.size == 0) {
    fprintf(stderr, "Unable to read input file \"%.*s\"\n", str8_varg(file_path));
    os_abort(1);
  }

  // make indent
  String8 indent;
  {
    U64 indent_buffer_size = RD_INDENT_WIDTH * RD_INDENT_MAX;
    U8 *indent_buffer      = push_array(scratch.arena, U8, indent_buffer_size);
    MemorySet(indent_buffer, ' ', indent_buffer_size);
    indent = str8(indent_buffer, 0);
  }

  // format input
  String8List out = {0};
  format_preamble(scratch.arena, &out, indent, file_path, raw_data);
  if (coff_is_archive(raw_data) || coff_is_thin_archive(raw_data)) {
    coff_format_archive(scratch.arena, &out, indent, raw_data, opts);
  } else if (coff_is_big_obj(raw_data)) {
    coff_format_big_obj(scratch.arena, &out, indent, raw_data, opts);
  } else if (coff_is_obj(raw_data)) {
    coff_format_obj(scratch.arena, &out, indent, raw_data, opts);
  } else if (is_pe(raw_data)) {
    pe_format(scratch.arena, &out, indent, raw_data, opts);
  } else if (pe_is_res(raw_data)) {
    //tool_out_coff_res(stdout, file_data);
  }
  
  // print formatted string
  String8 out_string = str8_list_join(scratch.arena, &out, &(StringJoin){ .sep = str8_lit("\n"),});
  fprintf(stdout, "%.*s", str8_varg(out_string));

  scratch_end(scratch);
}

