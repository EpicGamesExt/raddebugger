// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "Epic Games Tools (R) RAD CLI Binary Utility"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "linker/hash_table.h"
#include "os/os_inc.h"
#include "async/async.h"
#include "rdi/rdi_local.h"
#include "rdi_make/rdi_make_local.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "dwarf/dwarf_inc.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_coff/rdi_from_coff.h"
#include "rdi_from_elf/rdi_from_elf.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "rdi_breakpad_from_pdb/rdi_breakpad_from_pdb.h"
#include "rdi_from_dwarf/rdi_from_dwarf.h"
#include "radbin/radbin.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "linker/hash_table.c"
#include "os/os_inc.c"
#include "async/async.c"
#include "rdi/rdi_local.c"
#include "rdi_make/rdi_make_local.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "dwarf/dwarf_inc.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_coff/rdi_from_coff.c"
#include "rdi_from_elf/rdi_from_elf.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "rdi_breakpad_from_pdb/rdi_breakpad_from_pdb.c"
#include "rdi_from_dwarf/rdi_from_dwarf.c"
#include "radbin/radbin.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  rb_entry_point(cmdline);
}
