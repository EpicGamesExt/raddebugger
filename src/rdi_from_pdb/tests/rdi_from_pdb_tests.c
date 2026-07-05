// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

Test(p2r_regressions)
{
  String8 radbin_path = test_build_exe_path(arena, s("radbin"));
  String8 pdb_paths[] =
  {
    test_input_path(arena, ctx, s("mule_main_9ff1e58f/mule_main.pdb")),
    test_input_path(arena, ctx, s("mule_main_9ff1e58f/mule_module.pdb")),
  };
  
  // rjf: generate RDIs
  {
    ProcessList processes = {0};
    for EachElement(pdb_idx, pdb_paths)
    {
      String8 pdb_path = pdb_paths[pdb_idx];
      String8 rdi_path = str8f(arena, "%S/%I64u.rdi", ctx->artifacts_path, pdb_idx);
      Process process = launch_cmd_line(str8f(arena, "%S --rdi --deterministic %S --out:%S", radbin_path, pdb_path, rdi_path));
      process_list_push(arena, &processes, process);
    }
    for EachNode(n, ProcessNode, processes.first)
    {
      process_join(n->v, max_U64, 0);
    }
  }
  
  // rjf: generate dumps
  {
    ProcessList processes = {0};
    for EachElement(pdb_idx, pdb_paths)
    {
      String8 rdi_path = str8f(arena, "%S/%I64u.rdi", ctx->artifacts_path, pdb_idx);
      String8 dump_path = str8f(arena, "%S/current_%I64u", ctx->artifacts_path, pdb_idx);
      Process process = launch_cmd_line(str8f(arena, "%S --dump --deterministic %S --out:%S", radbin_path, rdi_path, dump_path));
      process_list_push(arena, &processes, process);
    }
    for EachNode(n, ProcessNode, processes.first)
    {
      process_join(n->v, max_U64, 0);
    }
  }
  
  // rjf: check against exemplars
  {
    for EachElement(pdb_idx, pdb_paths)
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8 dump_path_current = str8f(scratch.arena, "%S/current_%I64u", ctx->artifacts_path, pdb_idx);
      String8 dump_data_current = data_from_file_path(scratch.arena, dump_path_current);
      String8 dump_path_exemplar = test_exemplar_path(scratch.arena, ctx, str8f(scratch.arena, "exemplar_%I64u", pdb_idx));
      String8 dump_data_exemplar = data_from_file_path(scratch.arena, dump_path_exemplar);
      B32 dump_matches = str8_match(dump_data_current, dump_data_exemplar, 0);
      if(!dump_matches)
      {
        String8 diff_cmd = str8f(scratch.arena, "diff %S %S",
                                 path_normalized_from_string(scratch.arena, dump_path_current),
                                 path_normalized_from_string(scratch.arena, dump_path_exemplar));
        test_outf("Current log does not match exemplar; run `%S`\n", diff_cmd);
      }
      TestCheck(dump_matches);
      scratch_end(scratch);
    }
  }
}

Test(p2r_determinism)
{
  U64 num_repeats_per_pdb = 16;
  String8 radbin_path = test_build_exe_path(arena, s("radbin"));
  String8 pdb_paths[] =
  {
    test_input_path(arena, ctx, s("mule_main_9ff1e58f/mule_main.pdb")),
    test_input_path(arena, ctx, s("mule_main_9ff1e58f/mule_module.pdb")),
  };
  for EachElement(pdb_idx, pdb_paths)
  {
    // rjf: unpack paths, make output directory
    String8 pdb_path = pdb_paths[pdb_idx];
    
    // rjf: generate all RDIs
    String8List rdi_paths = {0};
    String8List dump_paths = {0};
    {
      ProcessList processes = {0};
      for EachIndex(repeat_idx, num_repeats_per_pdb)
      {
        String8 rdi_name = str8f(arena, "repeat_%I64u.rdi", repeat_idx);
        String8 rdi_path = str8f(arena, "%S/%S", ctx->artifacts_path, rdi_name);
        str8_list_push(arena, &rdi_paths, rdi_path);
        String8 cmdl = str8f(arena, "%S --rdi --deterministic %S --out:%S", radbin_path, pdb_path, rdi_path);
        Process process = launch_cmd_line(cmdl);
        TestCheck(!process_match(process_zero(), process));
        process_list_push(arena, &processes, process);
      }
      for EachNode(n, ProcessNode, processes.first)
      {
        process_join(n->v, max_U64, 0);
      }
    }
    
    // rjf: generate all dumps
    {
      ProcessList processes = {0};
      for EachNode(n, String8Node, rdi_paths.first)
      {
        String8 rdi_path = n->string;
        String8 dump_path = str8f(arena, "%S.dump", rdi_path);
        str8_list_push(arena, &dump_paths, dump_path);
        Process process_handle = launch_cmd_linef("%S --dump --deterministic %S --out:%S", radbin_path, rdi_path, dump_path);
        TestCheck(!process_match(process_zero(), process_handle));
        process_list_push(arena, &processes, process_handle);
      }
      for EachNode(n, ProcessNode, processes.first)
      {
        process_join(n->v, max_U64, 0);
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
      for EachNode(n, String8Node, rdi_paths.first)
      {
        Temp scratch = scratch_begin(0, 0);
        String8 rdi_path = n->string;
        String8 path = rdi_path;
        String8 data = data_from_file_path(scratch.arena, path);
        TestCheck(data.size != 0);
        rdi_hashes[idx] = u128_hash_from_str8(data);
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
        String8 data = data_from_file_path(scratch.arena, path);
        TestCheck(data.size != 0);
        dump_hashes[idx] = u128_hash_from_str8(data);
        dump_paths_array[idx] = path;
        scratch_end(scratch);
        idx += 1;
      }
    }
    
    // rjf: determine if all hashes match
    U64 mismatch_num = 0;
    for EachIndex(idx, rdi_hashes_count)
    {
      if(!u128_match(rdi_hashes[idx], rdi_hashes[0]))
      {
        mismatch_num = idx+1;
        break;
      }
    }
    for EachIndex(idx, dump_hashes_count)
    {
      if(!u128_match(dump_hashes[idx], dump_hashes[0]))
      {
        mismatch_num = idx+1;
        break;
      }
    }
    
    // rjf: output bad case info
    if(mismatch_num != 0)
    {
      U64 idx = mismatch_num-1;
      test_outf("  pdb[%I64u] \"%S\"\n", idx, pdb_path);
      test_outf("    rdi[%I64u] 0x%I64x:%I64x \"%S\"\n", idx, rdi_hashes[idx].u64[0], rdi_hashes[idx].u64[1], rdi_paths_array[idx]);
      test_outf("    dump[%I64u] 0x%I64x:%I64x \"%S\"\n", idx, dump_hashes[idx].u64[0], dump_hashes[idx].u64[1], dump_paths_array[idx]);
    }
    TestCheck(mismatch_num == 0);
  }
}

////////////////////////////////
//~ Regression coverage: malformed PDB/MSF input must not crash radbin
//
// Covers EpicGames/raddebugger #832, #833, #834, #835 -- each constructs a
// tiny (single-page-per-stream) but structurally valid MSF 7.0 container,
// then corrupts exactly the field the original report relied on. All inputs
// here are hand-built in-process (no external test data required).

internal String8
p2r_test_build_info_stream(Arena *arena, PDB_InfoVersion version, U32 hash_table_count)
{
  U8  buf[128] = {0};
  U8 *p        = buf;
  U32 v;
  v = version; MemoryCopy(p, &v, 4); p += 4; // version
  v = 0;       MemoryCopy(p, &v, 4); p += 4; // time
  v = 1;       MemoryCopy(p, &v, 4); p += 4; // age
  B32 recognized = (version == PDB_InfoVersion_VC70_DEP || version == PDB_InfoVersion_VC70 ||
                     version == PDB_InfoVersion_VC80    || version == PDB_InfoVersion_VC110 ||
                     version == PDB_InfoVersion_VC140);
  if(recognized)
  {
    Guid guid = {0};
    MemoryCopy(p, &guid, sizeof(guid)); p += sizeof(guid);
  }
  v = 0;                 MemoryCopy(p, &v, 4); p += 4; // names_len
  v = hash_table_count;  MemoryCopy(p, &v, 4); p += 4; // hash_table_count
  v = hash_table_count;  MemoryCopy(p, &v, 4); p += 4; // hash_table_max
  v = 0;                 MemoryCopy(p, &v, 4); p += 4; // num_present_words
  v = 0;                 MemoryCopy(p, &v, 4); p += 4; // num_deleted_words
  if(hash_table_count > 0)
  {
    v = 0; MemoryCopy(p, &v, 4); p += 4; // relative_name_off
    v = 0; MemoryCopy(p, &v, 4); p += 4; // stream number
  }
  U64 size = (U64)(p - buf);
  U8 *result_buf = push_array_no_zero(arena, U8, size);
  MemoryCopy(result_buf, buf, size);
  return str8(result_buf, size);
}

internal String8
p2r_test_build_dbi_stream(Arena *arena, U16 gsi_sn, U16 psi_sn)
{
  U8  buf[64] = {0};
  U8 *p        = buf;
  U32 v32; U16 v16;
  v32 = PDB_DbiHeaderSignature_V1; MemoryCopy(p, &v32, 4); p += 4; // sig
  v32 = PDB_DbiVersion_70;         MemoryCopy(p, &v32, 4); p += 4; // version
  v32 = 1;                         MemoryCopy(p, &v32, 4); p += 4; // age
  v16 = gsi_sn;                    MemoryCopy(p, &v16, 2); p += 2; // gsi_sn
  v16 = 0;                         MemoryCopy(p, &v16, 2); p += 2; // build_number
  v16 = psi_sn;                    MemoryCopy(p, &v16, 2); p += 2; // psi_sn
  v16 = 0;                         MemoryCopy(p, &v16, 2); p += 2; // pdb_version
  v16 = 0xFFFF;                    MemoryCopy(p, &v16, 2); p += 2; // sym_sn (unused by this test)
  v16 = 0;                         MemoryCopy(p, &v16, 2); p += 2; // pdb_version2
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // module_info_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // sec_con_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // sec_map_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // file_info_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // tsm_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // mfc_index
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // dbg_header_size
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // ec_info_size
  v16 = 0; MemoryCopy(p, &v16, 2); p += 2; // flags
  v16 = 0; MemoryCopy(p, &v16, 2); p += 2; // machine
  v32 = 0; MemoryCopy(p, &v32, 4); p += 4; // reserved
  U64 size = (U64)(p - buf);
  Assert(size == 64);
  U8 *result_buf = push_array_no_zero(arena, U8, size);
  MemoryCopy(result_buf, buf, size);
  return str8(result_buf, size);
}

// Builds a minimal MSF 7.0 container with 6 fixed streams:
//   0: (empty)          -- "old MSF directory" (unused, tolerated absent)
//   1: info_stream      -- PDB Info Stream
//   2: (empty)          -- Tpi (intentionally absent/invalid; tolerated)
//   3: dbi_stream       -- DBI Stream
//   4: (empty)          -- Ipi (intentionally absent/invalid; tolerated)
//   5: (empty)          -- valid, in-range GSI/PSI target for the non-crashing cases
// `info_stream` and `dbi_stream` must each fit within a single 512-byte page.
internal String8
p2r_test_build_msf70(Arena *arena, String8 info_stream, String8 dbi_stream)
{
  Assert(info_stream.size <= 512);
  Assert(dbi_stream.size  <= 512);

  U32 page_size        = 512;
  U32 stream_count     = 6;
  U32 stream_sizes[6]  = {0, (U32)info_stream.size, 0, (U32)dbi_stream.size, 0, 0};
  U32 info_page        = 3;
  U32 dbi_page         = 4;
  U32 total_page_count = 5;
  U32 directory_size   = 4 + 4*stream_count + 4*2; // count + sizes[6] + 2 index entries (streams 1 & 3 each have 1 page)

  U8 directory[512] = {0};
  {
    U8 *p = directory;
    MemoryCopy(p, &stream_count, 4); p += 4;
    for(U32 i = 0; i < stream_count; i += 1) { MemoryCopy(p, &stream_sizes[i], 4); p += 4; }
    MemoryCopy(p, &info_page, 4); p += 4;
    MemoryCopy(p, &dbi_page,  4); p += 4;
  }

  U8 header[512] = {0};
  {
    U8 *p = header;
    MemoryCopy(p, msf_msf70_magic, sizeof(msf_msf70_magic)); p += sizeof(msf_msf70_magic);
    U32 v;
    v = page_size;        MemoryCopy(p, &v, 4); p += 4; // page_size
    v = 1;                 MemoryCopy(p, &v, 4); p += 4; // active_fpm
    v = total_page_count;  MemoryCopy(p, &v, 4); p += 4; // page_count
    v = directory_size;    MemoryCopy(p, &v, 4); p += 4; // stream_table_size
    v = 0;                 MemoryCopy(p, &v, 4); p += 4; // reserved
    v = 1;                 MemoryCopy(p, &v, 4); p += 4; // root_pn -> map page (page 1)
  }

  U8 map_page[512] = {0};
  { U32 v = 2; MemoryCopy(map_page, &v, 4); } // directory data lives on page 2

  U8 *buf = push_array(arena, U8, total_page_count*page_size);
  MemoryCopy(buf + 0*page_size, header,    page_size);
  MemoryCopy(buf + 1*page_size, map_page,  page_size);
  MemoryCopy(buf + 2*page_size, directory, page_size);
  MemoryCopy(buf + info_page*page_size, info_stream.str, info_stream.size);
  MemoryCopy(buf + dbi_page*page_size,  dbi_stream.str,  dbi_stream.size);

  return str8(buf, (U64)total_page_count*page_size);
}

Test(p2r_malformed_input_hardening)
{
  String8 radbin_path = test_build_exe_path(arena, s("radbin"));

  String8 info_ok = p2r_test_build_info_stream(arena, PDB_InfoVersion_VC70, 0);
  String8 dbi_ok  = p2r_test_build_dbi_stream(arena, /*gsi_sn*/5, /*psi_sn*/5);

  typedef struct { String8 name; String8 data; } MalformedCase;
  MalformedCase cases[5];

  // sanity: the "good" construction itself must round-trip without crashing
  cases[0].name = s("good_baseline_sanity");
  cases[0].data = p2r_test_build_msf70(arena, info_ok, dbi_ok);

  // #832: MSF page_size field == 0 -> would divide by zero in msf_raw_stream_table_from_data
  cases[1].name = s("832_msf_page_size_zero");
  cases[1].data = str8_copy(arena, cases[0].data);
  {
    U32 zero = 0;
    MemoryCopy(cases[1].data.str + sizeof(msf_msf70_magic), &zero, 4); // page_size is right after the magic
  }

  // #833: unrecognized PDB info version with hash_table_count>0 -> unset auth_guid dereferenced
  cases[2].name = s("833_pdb_info_unknown_version_null_guid");
  cases[2].data = p2r_test_build_msf70(arena, p2r_test_build_info_stream(arena, 0x12345678, 1), dbi_ok);

  // #834 (and its duplicate #835): an out-of-range DBI stream number makes the
  // PSI/GSI path build a String8 with a null base and a wrapped-around size,
  // which pdb_gsi_from_data then treats as a header pointer. Here psi_sn is out
  // of range; the size-range check in pdb_gsi_from_data is what makes this case
  // fail without the fix (a plain null check alone would not), so this single
  // case regression-locks that guard with teeth.
  cases[3].name = s("834_dbi_psi_sn_out_of_range");
  cases[3].data = p2r_test_build_msf70(arena, info_ok, p2r_test_build_dbi_stream(arena, /*gsi_sn*/5, /*psi_sn*/0xDEAD));

  // MSF header stream_table_size < 4 -> directory_size < 4 -> out-of-bounds
  // stream-count read and a (directory_size - 4) underflow into a ~1G count.
  cases[4].name = s("msf_stream_table_size_too_small");
  cases[4].data = str8_copy(arena, cases[0].data);
  {
    U32 zero = 0;
    MemoryCopy(cases[4].data.str + OffsetOf(MSF_Header70, stream_table_size), &zero, 4);
  }

  String8 radbin_dir = str8_chop_last_slash(radbin_path);
  for EachElement(idx, cases)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 pdb_name = str8f(scratch.arena, "%S.pdb", cases[idx].name);
    String8 rdi_name = str8f(scratch.arena, "%S.rdi", cases[idx].name);
    String8 out_name = str8f(scratch.arena, "%S.out", cases[idx].name);
    t_write_file(pdb_name, cases[idx].data);
    String8 pdb_path = t_make_file_path(scratch.arena, pdb_name);
    String8 rdi_path = t_make_file_path(scratch.arena, rdi_name);
    String8 out_path = t_make_file_path(scratch.arena, out_name);

    // run radbin with stdout+stderr captured to out_path; delete any stale
    // file first since AccessFlag_Append does not truncate on all platforms
    delete_file_at_path(out_path);
    File out_file = file_open(AccessFlag_Write|AccessFlag_Append|AccessFlag_ShareRead|AccessFlag_ShareWrite|AccessFlag_Inherited, out_path);
    ProcessLaunchParams params = {0};
    str8_list_push(scratch.arena, &params.cmd_line, radbin_path);
    str8_list_push(scratch.arena, &params.cmd_line, pdb_path);
    str8_list_push(scratch.arena, &params.cmd_line, str8f(scratch.arena, "--out:%S", rdi_path));
    params.path        = radbin_dir;
    params.inherit_env = 1;
    params.consoleless = 1;
    params.stdout_file = out_file;
    params.stderr_file = out_file;
    Process process = process_launch(&params);
    B32 launched = !process_match(process, process_zero());
    B32 joined = launched ? process_join(process, max_U64, 0) : 0;
    file_close(out_file);
    String8 out_data = data_from_file_path(scratch.arena, out_path);

    // radbin's exit code is not a reliable crash signal cross-platform (it can
    // return a benign non-zero code, e.g. on Windows for these synthetic
    // inputs), so detect a crash by the crash handler's output instead. Both
    // handlers print "The process is terminating." on a fatal signal/exception
    // (linux: "A fatal signal was received..."; windows: "A fatal exception...
    // occurred..."); radbin never emits that string on a normal run.
    String8 crash_sig = str8_lit("The process is terminating");
    B32 crashed = str8_find_needle(out_data, 0, crash_sig, 0) < out_data.size;
    if(crashed)
    {
      test_outf("  case \"%S\": radbin crash signature detected:\n%S\n", cases[idx].name, out_data);
    }
    TestCheck(launched && joined && !crashed);
    scratch_end(scratch);
  }
}
