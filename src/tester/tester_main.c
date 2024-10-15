// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 12
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "tester"
#define BUILD_CONSOLE_INTERFACE 1
#define OS_FEATURE_GRAPHICAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "path/path.h"
#include "hash_store/hash_store.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "hash_store/hash_store.c"

////////////////////////////////
//~ rjf: Entry Points

internal B32 frame(void) { return 0; }

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  
  //////////////////////////////
  //- rjf: PDB -> RDI determinism
  //
  String8 name = {0};
  B32 good = 1;
  String8List out = {0};
  {
    name = str8_lit("PDB -> RDI determinism");
    OS_HandleList processes = {0};
    String8List rdi_paths = {0};
    U64 num_repeats_per_pdb = 4;
    String8 pdb_paths[] =
    {
      str8_lit_comp("odintest/test.pdb"),
    };
    for EachElement(pdb_idx, pdb_paths)
    {
      String8 pdb_path = path_normalized_from_string(arena, pdb_paths[pdb_idx]);
      String8 pdb_folder = str8_chop_last_slash(pdb_path);
      String8 repeat_folder = push_str8f(arena, "%S/pdb2rdi_determinism", pdb_folder);
      os_make_directory(repeat_folder);
      for EachIndex(repeat_idx, num_repeats_per_pdb)
      {
        String8 rdi_path = push_str8f(arena, "%S/repeat_%I64u.rdi", repeat_folder, repeat_idx);
        str8_list_push(arena, &rdi_paths, rdi_path);
        os_handle_list_push(arena, &processes, os_cmd_line_launchf("rdi_from_pdb --pdb:%S --out:%S", pdb_path, rdi_path));
      }
    };
    for(OS_HandleNode *n = processes.first; n != 0; n = n->next)
    {
      os_process_join(n->v, max_U64);
    }
    U64 hashes_count = rdi_paths.node_count;
    U128 *hashes = push_array(arena, U128, hashes_count);
    String8 *paths = push_array(arena, String8, hashes_count);
    {
      U64 idx = 0;
      for(String8Node *n = rdi_paths.first; n != 0; n = n->next, idx += 1)
      {
        String8 path = n->string;
        String8 data = os_data_from_file_path(arena, path);
        hashes[idx] = hs_hash_from_data(data);
        paths[idx] = path;
      }
    }
    B32 matches = 1;
    for EachIndex(idx, hashes_count)
    {
      if(!u128_match(hashes[idx], hashes[0]))
      {
        matches = 0;
        break;
      }
    }
    if(!matches)
    {
      good = 0;
      for EachIndex(idx, hashes_count)
      {
        str8_list_pushf(arena, &out, " [%I64u] (%S): 0x%I64x:%I64x\n", idx, paths[idx], hashes[idx].u64[0], hashes[idx].u64[1]);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump results
  //
  fprintf(stderr, "[%s] %.*s\n", good ? "." : "X", str8_varg(name));
  if(!good)
  {
    for(String8Node *n = out.first; n != 0; n = n->next)
    {
      fprintf(stderr, "%.*s", str8_varg(n->string));
    }
  }
  
}
