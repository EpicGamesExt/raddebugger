// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Build Options

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "TORTURE"
#define NO_ASYNC 1

////////////////////////////////

#include "base/base_inc.h"
#include "linker/hash_table.h"
#include "os/os_inc.h"
#include "rdi/rdi_local.h"
#include "rdi_make/rdi_make_local.h"
#include "obj/obj.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "coff/coff_obj_writer.h"
#include "coff/coff_lib_writer.h"
#include "pe/pe.h"
#include "pe/pe_section_flags.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "elf/elf.h"
#include "elf/elf_parse.h"
#include "dwarf/dwarf_inc.h"
#include "regs/regs.h"
#include "regs/dwarf/regs_dwarf.h"
#include "linker/base_ext/base_core.h"
#include "linker/base_ext/base_arena.h"
#include "linker/base_ext/base_arrays.h"
#include "linker/thread_pool/thread_pool.h"
#include "linker/codeview_ext/codeview.h"
#include "linker/lnk_cmd_line.h"
#include "linker/lnk_cmd_line.c"
#include "linker/lnk_log.h"
#include "torture.h"
#include "torture_radlink.h"

#include "base/base_inc.c"
#include "linker/hash_table.c"
#include "os/os_inc.c"
#include "rdi/rdi_local.c"
#include "rdi_make/rdi_make_local.c"
#include "obj/obj.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "coff/coff_lib_writer.c"
#include "pe/pe.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "elf/elf.c"
#include "elf/elf_parse.c"
#include "dwarf/dwarf_inc.c"
#include "regs/regs.c"
#include "regs/dwarf/regs_dwarf.c"
#include "linker/base_ext/base_core.c"
#include "linker/base_ext/base_arena.c"
#include "linker/base_ext/base_arrays.c"
#include "linker/thread_pool/thread_pool.c"
#include "linker/codeview_ext/codeview.c"
#include "torture.c"
#include "torture_base.c"
#include "torture_radlink.c"
#include "torture_dwarf.c"
#include "torture_d2r.c"

////////////////////////////////

internal void
entry_point(CmdLine *cmdline)
{
  t_entry_point(cmdline);
}

