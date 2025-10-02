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
#include "dbg_info/dbg_info.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "content/content.c"
#include "artifact_cache/artifact_cache.c"
#include "file_stream/file_stream.c"
#include "rdi/rdi_local.c"
#include "dbg_info/dbg_info.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  local_persist char *pdb_paths[] =
  {
    "C:/devel/raddebugger/build/raddbg.pdb",
    // #include "fn_debug_infos.inc"
  };
  
  DI_Key keys[ArrayCount(pdb_paths)] = {0};
  for EachElement(idx, pdb_paths)
  {
    String8 path = str8_cstring(pdb_paths[idx]);
    keys[idx] = di_key_from_path_timestamp(path, 0);
    di_open(keys[idx]);
  }
  
  for(;;)
  {
    Access *access = access_open();
    B32 got_all_rdis = 1;
    U64 num_rdis_loaded = 0;
    for EachElement(idx, pdb_paths)
    {
      RDI_Parsed *rdi = di_rdi_from_key(access, keys[idx], 1, 0);
      if(rdi == &rdi_parsed_nil)
      {
        got_all_rdis = 0;
      }
      else
      {
        num_rdis_loaded += 1;
      }
    }
    printf("\rloaded [%I64u/%I64u], %I64u active threads, %I64u active processes", num_rdis_loaded, ArrayCount(pdb_paths), di_shared->conversion_thread_count, di_shared->conversion_process_count);
    access_close(access);
    if(got_all_rdis)
    {
      Access *access = access_open();
      String8 search_query = str8_lit("rd_");
      DI_SearchItemArray items = di_search_item_array_from_target_query(access, RDI_SectionKind_Procedures, search_query, max_U64);
      printf("\n");
      printf("fuzzy searched for %.*s, found %I64u items\n", str8_varg(search_query), items.count);
      access_close(access);
      
      String8 match_query = str8_lit("rd_frame");
      DI_Match match = di_match_from_string(match_query, 0, max_U64);
      printf("searched for %.*s, found at %i in [%I64x:%I64x]\n", str8_varg(match_query), match.idx, match.key.u64[0], match.key.u64[1]);
      
      break;
    }
  }
}
