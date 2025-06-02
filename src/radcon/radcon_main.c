// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_TITLE "Epic Games Tools (R) RAD Debug Info Converter"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
// Third Party

#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"
#define XXH_STATIC_LINKING_ONLY
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#define SINFL_IMPLEMENTATION
#include "third_party/sinfl/sinfl.h"
#include "third_party/radsort/radsort.h"

////////////////////////////////
// RDI Format Library

#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"

////////////////////////////////
// Headers

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "async/async.h"
#include "path/path.h"
#include "rdi_make/rdi_make_local.h"
#include "linker/hash_table.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "dwarf/dwarf.h"
#include "dwarf/dwarf_parse.h"
#include "dwarf/dwarf_coff.h"
#include "dwarf/dwarf_elf.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "radcon.h"
#include "radcon_coff.h"
#include "radcon_elf.h"
#include "radcon_cv.h"
#include "radcon_dwarf.h"
#include "radcon_pdb.h"

////////////////////////////////
// Implementations

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "async/async.c"
#include "path/path.c"
#include "rdi_make/rdi_make_local.c"
#include "linker/hash_table.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "dwarf/dwarf.c"
#include "dwarf/dwarf_parse.c"
#include "dwarf/dwarf_coff.c"
#include "dwarf/dwarf_elf.c"
#include "radcon.c"
#include "radcon_coff.c"
#include "radcon_elf.c"
#include "radcon_cv.c"
#include "radcon_dwarf.c"
#include "radcon_pdb.c"

////////////////////////////////

internal void
entry_point(CmdLine *cmdl)
{
  rc_main(cmdl);
}

