// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD Dumper"

////////////////////////////////

#include "linker/base_ext/base_blake3.h"
#include "linker/base_ext/base_blake3.c"
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"
#include "third_party/md5/md5.c"
#include "third_party/md5/md5.h"
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"
#define SINFL_IMPLEMENTATION
#include "third_party/sinfl/sinfl.h"

////////////////////////////////

#include "base/base_inc.h"
#include "linker/base_ext/base_inc.h"
#include "os/os_inc.h"
#include "async/async.h"
#include "rdi_format/rdi_format_local.h"
#include "rdi_make/rdi_make_local.h"
#include "path/path.h"
#include "linker/hash_table.h"
#include "coff/coff.h"
#include "coff/coff_enum.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "msvc_crt/msvc_crt.h"
#include "msvc_crt/msvc_crt_enum.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "codeview/codeview_enum.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "dwarf/dwarf.h"
#include "dwarf/dwarf_parse.h"
#include "dwarf/dwarf_expr.h"
#include "dwarf/dwarf_unwind.h"
#include "dwarf/dwarf_coff.h"
#include "dwarf/dwarf_elf.h"
#include "dwarf/dwarf_enum.h"
#include "radcon/radcon.h"
#include "radcon/radcon_coff.h"
#include "radcon/radcon_cv.h"
#include "radcon/radcon_elf.h"
#include "radcon/radcon_pdb.h"
#include "radcon/radcon_dwarf.h"

#include "base/base_inc.c"
#include "linker/base_ext/base_inc.c"
#include "os/os_inc.c"
#include "async/async.c"
#include "rdi_format/rdi_format_local.c"
#include "rdi_make/rdi_make_local.c"
#include "path/path.c"
#include "linker/hash_table.c"
#include "coff/coff.c"
#include "coff/coff_enum.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "msvc_crt/msvc_crt.c"
#include "msvc_crt/msvc_crt_enum.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "codeview/codeview_enum.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "dwarf/dwarf.c"
#include "dwarf/dwarf_parse.c"
#include "dwarf/dwarf_expr.c"
#include "dwarf/dwarf_unwind.c"
#include "dwarf/dwarf_coff.c"
#include "dwarf/dwarf_elf.c"
#include "dwarf/dwarf_enum.c"
#include "radcon/radcon_coff.c"
#include "radcon/radcon_cv.c"
#include "radcon/radcon_elf.c"
#include "radcon/radcon_pdb.c"
#include "radcon/radcon_dwarf.c"
#include "radcon/radcon.c"

#include "linker/thread_pool/thread_pool.h"
#include "linker/thread_pool/thread_pool.c"
#include "linker/codeview_ext/codeview.h"
#include "linker/codeview_ext/codeview.c"
#include "linker/rdi/rdi.h"
#include "linker/rdi/rdi.c"

#include "raddump/raddump.h"
#include "raddump/raddump.c"

////////////////////////////////

global read_only struct
{
  RD_Option opt;
  char     *name;
  char     *help;
} g_rd_dump_option_map[] = {
  { RD_Option_Help,             "help",                "Print help and exit"    },
  { RD_Option_Version,          "version",             "Print version and exit" },
  
  { RD_Option_NoRdi,            "nordi",               "Don't load RAD Debug Info" },
  
  { RD_Option_Headers,          "headers",             "Dump DOS header, file header, optional header, and/or archive header" },
  { RD_Option_Sections,         "sections",            "Dump section headers as table"                                        },
  { RD_Option_Rawdata,          "rawdata",             "Dump raw section data"                                                },
  { RD_Option_Codeview,         "cv",                  "Dump CodeView"                                                        },
  { RD_Option_Disasm,           "disasm",              "Disassemble code sections"                                            },
  { RD_Option_Symbols,          "symtab",              "Dump COFF symbol table"                                               },
  { RD_Option_Relocs,           "relocs",              "Dump relocations"                                                     },
  { RD_Option_Exceptions,       "exceptions",          "Dump exceptions"                                                      },
  { RD_Option_Tls,              "tls",                 "Dump Thread Local Storage directory"                                  },
  { RD_Option_Debug,            "debug",               "Dump debug directory"                                                 },
  { RD_Option_Imports,          "imports",             "Dump import table"                                                    },
  { RD_Option_Exports,          "exports",             "Dump export table"                                                    },
  { RD_Option_LoadConfig,       "loadconfig",          "Dump load config"                                                     },
  { RD_Option_Resources,        "resources",           "Dump resource directory"                                              },
  { RD_Option_LongNames,        "longnames",           "Dump archive long names"                                              },
  
  { RD_Option_DebugInfo,        "debug_info",          "Dump .debug_info"                                            },
  { RD_Option_DebugAbbrev,      "debug_abbrev",        "Dump .debug_abbrev"                                          },
  { RD_Option_DebugLine,        "debug_line",          "Dump .debug_line"                                            },
  { RD_Option_DebugStr,         "debug_str",           "Dump .debug_str"                                             },
  { RD_Option_DebugLoc,         "debug_loc",           "Dump .debug_loc"                                             },
  { RD_Option_DebugRanges,      "debug_ranges",        "Dump .debug_ranges"                                          },
  { RD_Option_DebugARanges,     "debug_aranges",       "Dump .debug_aranges"                                         },
  { RD_Option_DebugAddr,        "debug_addr",          "Dump .debug_addr"                                            },
  { RD_Option_DebugLocLists,    "debug_loclists",      "Dump .debug_loclists"                                        },
  { RD_Option_DebugRngLists,    "debug_rnglists",      "Dump .debug_rnglists"                                        },
  { RD_Option_DebugPubNames,    "debug_pubnames",      "Dump .debug_pubnames"                                        },
  { RD_Option_DebugPubTypes,    "debug_pubtypes",      "Dump .debug_putypes"                                         },
  { RD_Option_DebugLineStr,     "debug_linestr",       "Dump .debug_linestr"                                         },
  { RD_Option_DebugStrOffsets,  "debug_stroffsets",    "Dump .debug_stroffsets"                                      },
  { RD_Option_Dwarf,            "dwarf",               "Dump all DWARF sections"                                     },
  { RD_Option_RelaxDwarfParser, "relax_dwarf_parser",  "Relaxes version requirement on attribute and form encodings" },
  
  { RD_Option_RdiDataSections,     "rdi_data_sections",     "Dump data sections"      },
  { RD_Option_RdiTopLevelInfo,     "rdi_top_level_info",    "Dump top level info"     },
  { RD_Option_RdiBinarySections,   "rdi_binary_sections",   "Dump binary sections"    },
  { RD_Option_RdiFilePaths,        "rdi_file_paths",        "Dump file paths"         },
  { RD_Option_RdiSourceFiles,      "rdi_source_files",      "Dump source files"       },
  { RD_Option_RdiLineTables,       "rdi_line_tables",       "Dump line tables"        },
  { RD_Option_RdiSourceLineMaps,   "rdi_source_line_maps",  "Dump source line maps"   },
  { RD_Option_RdiUnits,            "rdi_units",             "Dump units"              },
  { RD_Option_RdiUnitVMap,         "rdi_units_virtual_map", "Dump units virtual map"  },
  { RD_Option_RdiTypeNodes,        "rdi_type_nodes",        "Dump type nodes"         },
  { RD_Option_RdiUserDefinedTypes, "rdi_udt",               "Dump user defined types" },
  { RD_Option_RdiGlobalVars,       "rdi_global_vars",       "Dump global variables"   },
  { RD_Option_RdiThreadVars,       "rdi_thread_vars",       "Dump thread variables"   },
  { RD_Option_RdiConstants,        "rdi_constants",         "Dump constants"          },
  { RD_Option_RdiScopes,           "rdi_scopes",            "Dump scopes"             },
  { RD_Option_RdiScopeVMap,        "rdi_scope_virtual_map", "Dump scope virtual map"  },
  { RD_Option_RdiInlineSites,      "rdi_inline_sites",      "Dump inline sites"       },
  { RD_Option_RdiNameMaps,         "rdi_name_maps",         "Dump name maps"          },
  { RD_Option_RdiStrings,          "rdi_strings",           "Dump strings"            },
  
  { RD_Option_Help,             "h",                   "Alias for -help"       },
  { RD_Option_Version,          "v",                   "Alias for -version"    },
  { RD_Option_Sections,         "s",                   "Alias for -sections"   },
  { RD_Option_Exceptions,       "e",                   "Alias for -exceptions" },
  { RD_Option_Imports,          "i",                   "Alias for -imports"    },
  { RD_Option_Exports,          "x",                   "Alias for -exports"    },
  { RD_Option_LoadConfig,       "l",                   "Alias for -loadconifg" },
  { RD_Option_Resources,        "c",                   "Alias for -resources"  },
  { RD_Option_Relocs,           "r",                   "Alias for -relocs"     },
};

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  
  // make indent
  String8List *out = push_array(arena, String8List, 1);
  String8      indent;
  {
    U64 indent_buffer_size = RD_INDENT_WIDTH * RD_INDENT_MAX;
    U8 *indent_buffer      = push_array(arena, U8, indent_buffer_size);
    MemorySet(indent_buffer, ' ', indent_buffer_size);
    indent = str8(indent_buffer, 0);
  }
  
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
        } else if (str8_match_lit("all", cmd->string, StringMatchFlag_CaseInsensitive)) {
          opt = ~0ull & ~(RD_Option_Help|RD_Option_Version);
          break;
        }
      }
      
      if (opt == 0) {
        rd_errorf("Unknown argument: \"%S\"", cmd->string);
        goto exit;
      }
      
      opts |= opt;
    }
  }
  
  // print help
  if (opts & RD_Option_Help) {
    int longest_cmd_switch = 0;
    for (U64 opt_idx = 0; opt_idx < ArrayCount(g_rd_dump_option_map); ++opt_idx) {
      longest_cmd_switch = Max(longest_cmd_switch, strlen(g_rd_dump_option_map[opt_idx].name));
    }
    rd_printf(BUILD_TITLE_STRING_LITERAL);
    rd_newline();
    rd_printf("# Help");
    rd_indent();
    for (U64 opt_idx = 0; opt_idx < ArrayCount(g_rd_dump_option_map); ++opt_idx) {
      char *name = g_rd_dump_option_map[opt_idx].name;
      char *help = g_rd_dump_option_map[opt_idx].help;
      int indent_size = longest_cmd_switch - strlen(name) + 1;
      rd_printf("-%s%.*s%s", g_rd_dump_option_map[opt_idx].name, indent_size, indent.str, g_rd_dump_option_map[opt_idx].help);
    }
    rd_unindent();
    goto exit;
  }
  
  // print version
  if (opts & RD_Option_Version) {
    rd_printf(BUILD_TITLE_STRING_LITERAL);
    rd_printf("\traddump <options> <inputs>");
    goto exit;
  }
  
  // input check
  if (cmdline->inputs.node_count == 0) {
    rd_errorf("No input file specified");
    goto exit;
  } else if (cmdline->inputs.node_count > 1) {
    rd_errorf("Too many inputs specified, expected one");
    goto exit;
  }
  
  // read input
  String8 file_path = str8_list_first(&cmdline->inputs);
  String8 raw_data  = os_data_from_file_path(arena, file_path);
  
  // is read ok?
  if (raw_data.size == 0) {
    rd_errorf("Unable to read input file \"%S\"", file_path);
    goto exit;
  }
  
  // format input
  rd_format_preamble(arena, out, indent, file_path, raw_data);
  if (rd_is_rdi(raw_data)) {
    RDI_Parsed rdi = {0};
    RDI_ParseStatus parse_status = rdi_parse(raw_data.str, raw_data.size, &rdi);
    switch (parse_status) {
      case RDI_ParseStatus_Good: {
        RD_Option rdi_print_opts = opts;
        if ((rdi_print_opts & RD_Option_RdiAll) == 0) {
          rdi_print_opts |= RD_Option_RdiAll;
        }
        rdi_print(arena, out, indent, &rdi, rdi_print_opts); 
      } break;
      case RDI_ParseStatus_HeaderDoesNotMatch:       rd_errorf("RDI Parse: header does not match");                 break;
      case RDI_ParseStatus_UnsupportedVersionNumber: rd_errorf("RDI Parse: unsupported version");                   break;
      case RDI_ParseStatus_InvalidDataSecionLayout:  rd_errorf("RDI Parse: invalid data section layout");           break;
      case RDI_ParseStatus_MissingRequiredSection:   rd_errorf("RDI Parse: missing required section");              break;
      default:                                       rd_errorf("RDI Parse: unknown parse status %u", parse_status); break;
    }
  } else if (coff_is_regular_archive(raw_data) || coff_is_thin_archive(raw_data)) {
    coff_print_archive(arena, out, indent, raw_data, opts);
  } else if (coff_is_big_obj(raw_data)) {
    coff_print_big_obj(arena, out, indent, raw_data, opts);
  } else if (coff_is_obj(raw_data)) {
    coff_print_obj(arena, out, indent, raw_data, opts);
  } else if (pe_check_magic(raw_data)) {
    RDI_Parsed *rdi = 0;
    if (!(opts & RD_Option_NoRdi)) {
      rdi = rd_rdi_from_pe(arena, file_path);
    }
    pe_print(arena, out, indent, raw_data, opts, rdi);
  } else if (pe_is_res(raw_data)) {
    //tool_out_coff_res(stdout, file_data);
  } else if (elf_check_magic(raw_data)) {
    //elf_print_dwarf_expressions(arena, out, indent, raw_data);
  }
  
  exit:;
  // print formatted string
  String8 out_string = str8_list_join(arena, out, &(StringJoin){ .sep = str8_lit("\n"),});
  fprintf(stdout, "%.*s", str8_varg(out_string));
  
  arena_release(arena);
}

