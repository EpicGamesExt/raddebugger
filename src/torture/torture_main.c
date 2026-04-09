// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Build Options

#define BUILD_TITLE "TORTURE"

#define BUILD_CONSOLE_INTERFACE 1
#define OS_FEATURE_GRAPHICAL 1
#define DMN_INIT_MANUAL 1
#define CTRL_INIT_MANUAL 1
#define OS_GFX_INIT_MANUAL 1
#define FP_INIT_MANUAL 1
#define R_INIT_MANUAL 1
#define FNT_INIT_MANUAL 1
#define D_INIT_MANUAL 1
#define RD_INIT_MANUAL 1
#define NO_ASYNC 1

////////////////////////////////

#include "base/base_inc.h"
#include "x64/x64.h"
#include "linker/hash_table.h"
#include "linker/lf_hash_table.h"
#include "os/os_inc.h"
#include "artifact_cache/artifact_cache.h"
#include "rdi/rdi_local.h"
#include "rdi_make/rdi_make_local.h"
#include "mdesk/mdesk.h"
#include "config/config_inc.h"
#include "content/content.h"
#include "file_stream/file_stream.h"
#include "text/text.h"
#include "mutable_text/mutable_text.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "coff/coff_obj_writer.h"
#include "coff/coff_lib_writer.h"
#include "pe/pe.h"
#include "pe/pe_section_flags.h"
#include "elf/elf.h"
#include "gnu/gnu.h"
#include "gnu/gnu_parse.h"
#include "elf/elf_parse.h"
#include "elf/elf_dump.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "dwarf/dwarf_inc.h"
#include "rdi_from_coff/rdi_from_coff.h"
#include "rdi_from_elf/rdi_from_elf.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "rdi_from_dwarf/rdi_from_dwarf.h"
#include "obj/obj.h"
#include "radbin/radbin.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "regs/dwarf/regs_dwarf.h"
#include "dbg_info/dbg_info.h"
#include "disasm/disasm.h"
#include "stap/stap_parse.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "eval_visualization/eval_visualization_inc.h"
#include "ctrl/ctrl_inc.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "dbg_engine/dbg_engine_inc.h"
#include "raddbg/raddbg_inc.h"
#include "linker/base_ext/base_core.h"
#include "linker/base_ext/base_arena.h"
#include "linker/base_ext/base_arrays.h"
#include "linker/base_ext/base_bit_array.h"
#include "linker/thread_pool/thread_pool.h"
#include "linker/codeview_ext/codeview.h"
#include "linker/pdb_ext/msf_builder.h"
#include "linker/lnk_cmd_line.h"
#include "linker/lnk_cmd_line.c"
#include "linker/lnk_log.h"
#include "torture.h"
#include "torture_radlink.h"

#include "base/base_inc.c"
#include "x64/x64.c"
#include "linker/hash_table.c"
#include "linker/lf_hash_table.c"
#include "os/os_inc.c"
#include "artifact_cache/artifact_cache.c"
#include "rdi/rdi_local.c"
#include "rdi_make/rdi_make_local.c"
#include "mdesk/mdesk.c"
#include "config/config_inc.c"
#include "content/content.c"
#include "file_stream/file_stream.c"
#include "text/text.c"
#include "mutable_text/mutable_text.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "coff/coff_lib_writer.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "gnu/gnu.c"
#include "gnu/gnu_parse.c"
#include "elf/elf_parse.c"
#include "elf/elf_dump.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "dwarf/dwarf_inc.c"
#include "rdi_from_coff/rdi_from_coff.c"
#include "rdi_from_elf/rdi_from_elf.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "rdi_from_dwarf/rdi_from_dwarf.c"
#include "obj/obj.c"
#include "radbin/radbin.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "regs/dwarf/regs_dwarf.c"
#include "dbg_info/dbg_info.c"
#include "disasm/disasm.c"
#include "stap/stap_parse.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "eval_visualization/eval_visualization_inc.c"
#include "ctrl/ctrl_inc.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "dbg_engine/dbg_engine_inc.c"
#include "raddbg/raddbg_inc.c"
#include "linker/base_ext/base_core.c"
#include "linker/base_ext/base_arena.c"
#include "linker/base_ext/base_arrays.c"
#include "linker/base_ext/base_bit_array.c"
#include "linker/thread_pool/thread_pool.c"
#include "linker/codeview_ext/codeview.c"
#include "linker/pdb_ext/msf_builder.c"

#include "torture.c"
#include "torture_base.c"
#include "torture_radlink.c"
#include "torture_dwarf.c"
#include "torture_d2r.c"
#include "torture_p2r.c"
#include "torture_eval.c"

internal B32 frame(void) { return 0; }

////////////////////////////////

internal void
entry_point(CmdLine *cmdline)
{
  t_entry_point(cmdline);
}

