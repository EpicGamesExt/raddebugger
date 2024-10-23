// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 13
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
  //- rjf: unpack command line
  //
  String8 test_data_folder_path = cmd_line_string(cmdline, str8_lit("test_data"));
  if(test_data_folder_path.size == 0)
  {
    fprintf(stderr, "error(input): The test data folder path was not specified. Specify the path when running the program, like: %.*s --test_data:C:/foo/bar/baz\n", str8_varg(cmdline->exe_name));
    os_abort(1);
  }
  
  //////////////////////////////
  //- rjf: make artifacts directory
  //
  String8 artifacts_path = path_normalized_from_string(arena, str8_lit("./tester_artifacts"));
  os_make_directory(artifacts_path);
  
  //////////////////////////////
  //- rjf: PDB -> RDI determinism
  //
  String8 name = {0};
  B32 good = 1;
  String8List out = {0};
  {
    name = str8_lit("pdb2rdi_determinism");
    U64 num_repeats_per_pdb = 32;
    String8 pdb_paths[] =
    {
      push_str8f(arena, "%S/mule_main/mule_main.pdb", test_data_folder_path),
      push_str8f(arena, "%S/mule_main/mule_module.pdb", test_data_folder_path),
    };
    for EachElement(pdb_idx, pdb_paths)
    {
      // rjf: unpack paths, make output directory
      String8 pdb_path = path_normalized_from_string(arena, pdb_paths[pdb_idx]);
      String8 repeat_folder = push_str8f(arena, "%S/%S", artifacts_path, name);
      os_make_directory(repeat_folder);
      
      // rjf: generate all RDIs
      String8List rdi_paths = {0};
      String8List dump_paths = {0};
      {
        OS_HandleList processes = {0};
        for EachIndex(repeat_idx, num_repeats_per_pdb)
        {
          String8 rdi_path = push_str8f(arena, "%S/repeat_%I64u.rdi", repeat_folder, repeat_idx);
          str8_list_push(arena, &rdi_paths, rdi_path);
          os_handle_list_push(arena, &processes, os_cmd_line_launchf("rdi_from_pdb --deterministic --pdb:%S --out:%S", pdb_path, rdi_path));
        }
        for(OS_HandleNode *n = processes.first; n != 0; n = n->next)
        {
          os_process_join(n->v, max_U64);
        }
      }
      
      // rjf: generate all dumps
      {
        OS_HandleList processes = {0};
        for(String8Node *n = rdi_paths.first; n != 0; n = n->next)
        {
          String8 rdi_path = n->string;
          String8 dump_path = push_str8f(arena, "%S.dump", rdi_path);
          str8_list_push(arena, &dump_paths, dump_path);
          os_handle_list_push(arena, &processes, os_cmd_line_launchf("rdi_dump %S > %S", rdi_path, dump_path));
        }
        for(OS_HandleNode *n = processes.first; n != 0; n = n->next)
        {
          os_process_join(n->v, max_U64);
        }
      }
      
      // rjf: gather all hashes/paths
      U64 rdi_hashes_count = rdi_paths.node_count;
      U128 *rdi_hashes = push_array(arena, U128, rdi_hashes_count);
      String8 *rdi_paths_array = push_array(arena, String8, rdi_hashes_count);
      U64 dump_hashes_count = dump_paths.node_count;
      U128 *dump_hashes = push_array(arena, U128, dump_hashes_count);
      String8 *dump_paths_array = push_array(arena, String8, dump_hashes_count);
      {
        U64 idx = 0;
        for(String8Node *n = rdi_paths.first; n != 0; n = n->next, idx += 1)
        {
          Temp scratch = scratch_begin(0, 0);
          String8 path = n->string;
          String8 data = os_data_from_file_path(scratch.arena, path);
          rdi_hashes[idx] = hs_hash_from_data(data);
          rdi_paths_array[idx] = path;
          scratch_end(scratch);
        }
      }
      {
        U64 idx = 0;
        for(String8Node *n = dump_paths.first; n != 0; n = n->next, idx += 1)
        {
          Temp scratch = scratch_begin(0, 0);
          String8 path = n->string;
          String8 data = os_data_from_file_path(scratch.arena, path);
          dump_hashes[idx] = hs_hash_from_data(data);
          dump_paths_array[idx] = path;
          scratch_end(scratch);
        }
      }
      
      // rjf: determine if all hashes match
      B32 matches = 1;
      for EachIndex(idx, rdi_hashes_count)
      {
        if(!u128_match(rdi_hashes[idx], rdi_hashes[0]))
        {
          matches = 0;
          break;
        }
      }
      for EachIndex(idx, dump_hashes_count)
      {
        if(!u128_match(dump_hashes[idx], dump_hashes[0]))
        {
          matches = 0;
          break;
        }
      }
      
      // rjf: output bad case info
      if(!matches)
      {
        good = 0;
        str8_list_pushf(arena, &out, "  pdb[%I64u] \"%S\"\n", pdb_idx, pdb_path);
        for EachIndex(idx, rdi_hashes_count)
        {
          str8_list_pushf(arena, &out, "    rdi[%I64u] \"%S\": 0x%I64x:%I64x\n", idx, rdi_paths_array[idx], rdi_hashes[idx].u64[0], rdi_hashes[idx].u64[1]);
        }
        for EachIndex(idx, dump_hashes_count)
        {
          str8_list_pushf(arena, &out, "    dump[%I64u] \"%S\": 0x%I64x:%I64x\n", idx, dump_paths_array[idx], dump_hashes[idx].u64[0], dump_hashes[idx].u64[1]);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: dump results
  //
  fprintf(stderr, "[%s] \"%.*s\"\n", good ? "." : "X", str8_varg(name));
  if(!good)
  {
    for(String8Node *n = out.first; n != 0; n = n->next)
    {
      fprintf(stderr, "%.*s", str8_varg(n->string));
    }
  }
  
}
