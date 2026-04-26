#define T_Group "p2r"

#if OS_WINDOWS

// TODO: location is not baked consistently
#if !COMPILER_CLANG || (COMPILER_CLANG && BUILD_DEBUG)
TEST(p2r_determ)
{
  U64 num_repeats_per_pdb = 32;
  String8 pdb_paths[] =
  {
    push_str8f(arena, "%S/mule_main.pdb", g_test_data),
    push_str8f(arena, "%S/mule_module.pdb", g_test_data),
  };
  for EachElement(pdb_idx, pdb_paths)
  {
    // rjf: unpack paths, make output directory
    String8 pdb_path = path_normalized_from_string(arena, pdb_paths[pdb_idx]);

    // rjf: generate all RDIs
    String8List rdi_paths = {0};
    String8List dump_paths = {0};
    {
      OS_HandleList processes = {0};
      for EachIndex(repeat_idx, num_repeats_per_pdb)
      {
        String8 rdi_name = push_str8f(arena, "repeat_%I64u.rdi", repeat_idx);
        String8 rdi_path = t_make_file_path(arena, rdi_name);
        str8_list_push(arena, &rdi_paths, rdi_path);
        String8 cmdl = str8f(arena, "radbin -rdi -deterministic %S -out:%S", pdb_path, rdi_path);
        OS_Handle process_handle = os_cmd_line_launch(cmdl);
        T_Ok(!os_handle_match(os_handle_zero(), process_handle));
        os_handle_list_push(arena, &processes, process_handle);
      }

      // wait on the converters
      for EachNode(n, OS_HandleNode, processes.first) { os_process_join(n->v, max_U64, 0); }
    }

    // rjf: generate all dumps
    {
      OS_HandleList processes = {0};
      for EachNode(n, String8Node, rdi_paths.first)
      {
        String8 rdi_path = n->string;
        String8 dump_path = push_str8f(arena, "%S.dump", rdi_path);
        str8_list_push(arena, &dump_paths, dump_path);
        OS_Handle process_handle = os_cmd_line_launchf("radbin -dump -deterministic %S -out:%S", rdi_path, dump_path);
        T_Ok(!os_handle_match(os_handle_zero(), process_handle));
        os_handle_list_push(arena, &processes, process_handle);
      }
      for EachNode(n, OS_HandleNode, processes.first) { os_process_join(n->v, max_U64, 0); }
    }

    // rjf: gather all hashes/paths
    U64      rdi_hashes_count  = rdi_paths.node_count;
    U128    *rdi_hashes        = push_array(arena, U128, rdi_hashes_count);
    String8 *rdi_paths_array   = push_array(arena, String8, rdi_hashes_count);
    U64      dump_hashes_count = dump_paths.node_count;
    U128    *dump_hashes       = push_array(arena, U128, dump_hashes_count);
    String8 *dump_paths_array  = push_array(arena, String8, dump_hashes_count);
    {
      U64 idx = 0;
      for EachNode(n, String8Node, rdi_paths.first)
      {
        Temp scratch = scratch_begin(0, 0);
        String8 rdi_path = n->string;
        String8 path     = rdi_path;
        T_Ok(path.size);
        String8 data     = os_data_from_file_path(scratch.arena, path);
        T_Ok(data.size);
        rdi_hashes     [idx] = u128_hash_from_str8(data);
        rdi_paths_array[idx] = path;
        scratch_end(scratch);
        idx += 1;
      }
    }
    {
      U64 idx = 0;
      for EachNode(n, String8Node, dump_paths.first)
      {
        Temp scratch = scratch_begin(0, 0);
        String8 path = n->string;
        T_Ok(path.size);
        String8 data = os_data_from_file_path(scratch.arena, path);
        T_Ok(data.size);
        dump_hashes     [idx] = u128_hash_from_str8(data);
        dump_paths_array[idx] = path;
        scratch_end(scratch);
        idx += 1;
      }
    }

    // rjf: determine if all hashes match
    U64 mismatch_idx = max_U64;
    for EachIndex(idx, rdi_hashes_count)
    {
      if(!u128_match(rdi_hashes[idx], rdi_hashes[0]))
      {
        mismatch_idx = idx;
        break;
      }
    }
    for EachIndex(idx, dump_hashes_count)
    {
      if(!u128_match(dump_hashes[idx], dump_hashes[0]))
      {
        mismatch_idx = idx;
        break;
      }
    }

    // rjf: output bad case info
    if (mismatch_idx != max_U64) {
      U64 idx = mismatch_idx;
      t_outf("  pdb[%I64u] \"%S\"\n", idx, pdb_path);
      t_outf("    rdi[%I64u] 0x%I64x:%I64x \"%S\"\n", idx, rdi_hashes[idx].u64[0], rdi_hashes[idx].u64[1], rdi_paths_array[idx]);
      t_outf("    dump[%I64u] 0x%I64x:%I64x \"%S\"\n", idx, dump_hashes[idx].u64[0], dump_hashes[idx].u64[1], dump_paths_array[idx]);
      T_Ok(0);
    }
  }
}
#endif
#endif

#undef T_Group

