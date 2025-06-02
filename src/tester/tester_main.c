// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

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
#include "rdi_format/rdi_format_local.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "eval/eval_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "hash_store/hash_store.c"
#include "rdi_format/rdi_format_local.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "eval/eval_inc.c"

////////////////////////////////
//~ rjf: Entry Points

internal B32 frame(void) { return 0; }

internal void
entry_point(CmdLine *cmdline)
{
  Arena *arena = arena_alloc();
  E_TypeCtx *type_ctx = push_array(arena, E_TypeCtx, 1);
  e_select_type_ctx(type_ctx);
  E_ParseCtx *parse_ctx = push_array(arena, E_ParseCtx, 1);
  e_select_parse_ctx(parse_ctx);
  E_IRCtx *ir_ctx = push_array(arena, E_IRCtx, 1);
  e_select_ir_ctx(ir_ctx);
  E_InterpretCtx *interpret_ctx = push_array(arena, E_InterpretCtx, 1);
  e_select_interpret_ctx(interpret_ctx, 0, 0);
  
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
  //- rjf: set up list of test artifacts
  //
  typedef struct Test Test;
  struct Test
  {
    Test *next;
    String8 name;
    String8List out;
    B32 good;
  };
  Test *first_test = 0;
  Test *last_test = 0;
#define Test(name_identifier) \
Test *test_##name_identifier = push_array(arena, Test, 1);\
test_##name_identifier->name = str8_lit(#name_identifier);\
test_##name_identifier->good = 1;\
SLLQueuePush(first_test, last_test, test_##name_identifier);\
for(Test *test = test_##name_identifier; test != 0; test = 0)
  
  //////////////////////////////
  //- rjf: PDB -> RDI determinism
  //
  Test(pdb2rdi_determinism)
  {
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
      String8 repeat_folder = push_str8f(arena, "%S/%S", artifacts_path, test->name);
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
        test->good = 0;
        str8_list_pushf(arena, &test->out, "  pdb[%I64u] \"%S\"\n", pdb_idx, pdb_path);
        for EachIndex(idx, rdi_hashes_count)
        {
          str8_list_pushf(arena, &test->out, "    rdi[%I64u] \"%S\": 0x%I64x:%I64x\n", idx, rdi_paths_array[idx], rdi_hashes[idx].u64[0], rdi_hashes[idx].u64[1]);
        }
        for EachIndex(idx, dump_hashes_count)
        {
          str8_list_pushf(arena, &test->out, "    dump[%I64u] \"%S\": 0x%I64x:%I64x\n", idx, dump_paths_array[idx], dump_hashes[idx].u64[0], dump_hashes[idx].u64[1]);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: eval compiler basics
  //
  Test(eval_compiler_basics)
  {
    String8 exprs[] =
    {
      str8_lit("123"),
      str8_lit("1 + 2"),
      str8_lit("foo"),
      str8_lit("foo(bar)"),
      str8_lit("foo(bar(baz))"),
    };
    String8List logs = {0};
    for EachElement(idx, exprs)
    {
      String8 log = e_debug_log_from_expr_string(arena, exprs[idx]);
      str8_list_push(arena, &logs, log);
    }
    String8 log = str8_list_join(arena, &logs, 0);
    String8 test_artifacts_path = push_str8f(arena, "%S/%S", artifacts_path, test->name);
    os_make_directory(test_artifacts_path);
    String8 current_file_path = push_str8f(arena, "%S/current.txt", test_artifacts_path);
    String8 correct_file_path = push_str8f(arena, "%S/%S/correct.txt", test_data_folder_path, test->name);
    os_write_data_to_file_path(current_file_path, log);
    String8 current_file_data = log;
    String8 correct_file_data = os_data_from_file_path(arena, correct_file_path);
    test->good = str8_match(correct_file_data, current_file_data, 0);
  }
  
  //////////////////////////////
  //- rjf: dump results
  //
  B32 all_good = 1;
  for(Test *t = first_test; t != 0; t = t->next)
  {
    if(!t->good)
    {
      all_good = 0;
      break;
    }
  }
  fprintf(stderr, "[%s]\n", all_good ? "." : "X");
  for(Test *t = first_test; t != 0; t = t->next)
  {
    fprintf(stderr, "    [%s] \"%.*s\"\n", t->good ? "." : "X", str8_varg(t->name));
    if(!t->good)
    {
      for(String8Node *n = t->out.first; n != 0; n = n->next)
      {
        fprintf(stderr, "        %.*s", str8_varg(n->string));
      }
    }
  }
  
}
