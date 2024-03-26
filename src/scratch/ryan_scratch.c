// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 9
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "ryan_scratch"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "lib_raddbgi_format/raddbgi_format.h"
#include "lib_raddbgi_format/raddbgi_format_parse.h"
#include "lib_raddbgi_format/raddbgi_format.c"
#include "lib_raddbgi_format/raddbgi_format_parse.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "raddbgi_make_local/raddbgi_make_local.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "file_stream/file_stream.h"
#include "text_cache/text_cache.h"
#include "path/path.h"
#include "txti/txti.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "raddbgi_from_pdb/raddbgi_from_pdb.h"
#include "regs/regs.h"
#include "regs/raddbgi/regs_raddbgi.h"
#include "type_graph/type_graph.h"
#include "dbgi/dbgi.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "unwind/unwind.h"
#include "ctrl/ctrl_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "raddbgi_make_local/raddbgi_make_local.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "file_stream/file_stream.c"
#include "text_cache/text_cache.c"
#include "path/path.c"
#include "txti/txti.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "raddbgi_from_pdb/raddbgi_from_pdb.c"
#include "regs/regs.c"
#include "regs/raddbgi/regs_raddbgi.c"
#include "type_graph/type_graph.c"
#include "dbgi/dbgi.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "unwind/unwind.c"
#include "ctrl/ctrl_inc.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  
}
