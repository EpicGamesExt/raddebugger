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

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "content/content.c"
#include "artifact_cache/artifact_cache.c"
#include "file_stream/file_stream.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  for(;;)
  {
    C_Key key = fs_key_from_path(str8_lit("C:/devel/raddebugger/build/x.dump"), os_now_microseconds() + 100000);
    U128 hash = c_hash_from_key(key, 0);
    printf("hash: 0x%I64x, 0x%I64x\n", hash.u64[0], hash.u64[1]);
    fflush(stdout);
    if(!u128_match(u128_zero(), hash))
    {
      break;
    }
  }
}
