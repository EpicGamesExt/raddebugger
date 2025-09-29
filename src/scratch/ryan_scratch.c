// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "ryan_scratch"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "content/content.h"
#include "artifact_cache/artifact_cache.h"
#include "file_stream/file_stream.h"
#include "rdi/rdi_local.h"
#include "dbg_info/dbg_info2.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "content/content.c"
#include "artifact_cache/artifact_cache.c"
#include "file_stream/file_stream.c"
#include "rdi/rdi_local.c"
#include "dbg_info/dbg_info2.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  DI2_Key key = di2_key_from_path_timestamp(str8_lit("C:/devel/raddebugger/build/raddbg.pdb"), 0);
  di2_open(key);
  for(;;)
  {
    Access *access = access_open();
    RDI_Parsed *rdi = di2_rdi_from_key(access, key, 1, 0);
    if(rdi != &rdi_parsed_nil)
    {
      int x = 0;
    }
    access_close(access);
  }
}
