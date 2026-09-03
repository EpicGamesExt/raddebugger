// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

typedef struct T_SharedPoolTask
{
  U64 visits[64];
  U32 delay_ms;
  B32 use_barrier;
} T_SharedPoolTask;

internal THREAD_POOL_TASK_FUNC(t_shared_pool_task)
{
  T_SharedPoolTask *task = raw_task;
  ins_atomic_u64_inc_eval(&task->visits[task_id]);
  if (task->delay_ms) { sleep_ms(task->delay_ms); }
  if (task->use_barrier) { barrier_wait(tp->barrier); }
}

TEST(shared_pool_governor_repeated_passes)
{
#if OS_WINDOWS
  // An isolated budget must not borrow from (or disturb) running production links.
  String8 name = str8f(arena, "radlink-torture-governor-%u-%llu", GetCurrentProcessId(), now_time_us());
  TP_Context *pool = tp_alloc(arena, 4, 4, name);
  T_SharedPoolTask task = {0};
  tp_for_parallel(pool, 0, 64, t_shared_pool_task, &task);

  // Measure, but do not assert a timing threshold on a shared/busy test machine.
  // The old governor burns a core while these already-assigned tasks sleep.
  W32_Entity *governor = (W32_Entity *)PtrFromInt(pool->governor_handle.u64[0]);
  FILETIME created, exited, kernel_before, user_before, kernel_after, user_after;
  B32 before_ok = GetThreadTimes(governor->thread.handle, &created, &exited, &kernel_before, &user_before);
  MemoryZeroStruct(&task);
  task.delay_ms = 200;
  tp_for_parallel(pool, 0, 3, t_shared_pool_task, &task);
  B32 after_ok = GetThreadTimes(governor->thread.handle, &created, &exited, &kernel_after, &user_after);
  for EachIndex(i, 3) { T_Ok(task.visits[i] == 1); }
  T_Ok(before_ok && after_ok);
  if (before_ok && after_ok) {
    U64 cpu_before = (((U64)kernel_before.dwHighDateTime << 32) | kernel_before.dwLowDateTime) +
                     (((U64)user_before.dwHighDateTime << 32) | user_before.dwLowDateTime);
    U64 cpu_after = (((U64)kernel_after.dwHighDateTime << 32) | kernel_after.dwLowDateTime) +
                    (((U64)user_after.dwHighDateTime << 32) | user_after.dwLowDateTime);
    fprintf(stdout, "[shared governor] covered-pass CPU: %.3f ms\n", (F64)(cpu_after - cpu_before) / 10000.0);
  }

  // Exercise wake coalescing, tiny/queued passes, and path-A/path-B transitions.
  U64 counts[] = {1, 2, 3, 5, 31, 64};
  for EachIndex(pass, 96) {
    MemoryZeroStruct(&task);
    U64 count = counts[pass % ArrayCount(counts)];
    task.delay_ms = (pass % 12 == 0) ? 1 : 0;
    tp_for_parallel(pool, 0, count, t_shared_pool_task, &task);
    for EachIndex(i, count) { T_Ok(task.visits[i] == 1); }
    T_Ok(ins_atomic_u64_eval(&pool->granted) == 0);

    MemoryZeroStruct(&task);
    task.use_barrier = 1;
    U64 cohort = tp_barrier_begin(pool);
    tp_for_parallel_reserve(pool, 0, cohort, t_shared_pool_task, &task);
    tp_barrier_end(pool);
    for EachIndex(i, cohort) { T_Ok(task.visits[i] == 1); }
  }

  // Join before releasing test-owned storage/synchronization. thread_join consumes
  // each handle; clear it before the common release path detaches remaining handles.
  pool->is_live = 0;
  semaphore_drop_if_room(pool->governor_semaphore);
  for (U64 i = 1; i < pool->worker_count; ++i) { semaphore_drop(pool->wake_semaphore); }
  for (U64 i = 1; i < pool->worker_count; ++i) {
    T_Ok(thread_join(pool->worker_arr[i].handle, max_U64));
    MemoryZeroStruct(&pool->worker_arr[i].handle);
  }
  T_Ok(thread_join(pool->governor_handle, max_U64));
  MemoryZeroStruct(&pool->governor_handle);
  tp_release(pool);
#else
  TestSkip();
#endif
}

internal String8
t_codec_test_pdb_data(Arena *arena)
{
  PDB_Context *pdb = pdb_alloc(MSF_DEFAULT_PAGE_SIZE, COFF_MachineType_X64, 123, 1, (Guid){0});
  String8 data = data_from_pdb(arena, pdb);
  pdb_release(pdb);
  return data;
}

TEST(generic_run_operation)
{
#if OS_WINDOWS
  T_Ok(copy_file_path(t_make_file_path(arena, str8_lit("run_fixture.exe")), get_process_info()->binary_file_path));

  String8 source = str8_lit(
      "test:\n"
      "{\n"
      "  steps:\n"
      "  {\n"
      "    run: { path: \"run_fixture.exe\", args: \"-run_fixture_ok\" }\n"
      "    run: { path: \"run_fixture.exe\", args: \"-run_fixture_nonzero\", expect_exit: 7 }\n"
      "    run: { path: \"run_fixture.exe\", args: \"-run_fixture_nonzero\", expect_exit: nonzero }\n"
      "    run: { path: \"run_fixture.exe\", args: \"-run_fixture_nonzero\", expect_exit: any }\n"
      "    run: { path: \"run_fixture.exe\", args: \"-run_fixture_ok\",\n"
      "           stdout_matches: \"*genericrunstdout*\", stderr_matches: \"*genericrunstderr*\" }\n"
      "  }\n"
      "}\n");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_run_operation.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));

  String8 timeout_source = str8_lit(
      "test: { steps: { run: { path: \"run_fixture.exe\", args: \"-run_fixture_sleep\", timeout_ms: 10 } } }");
  T_Context timeout_script = {0};
  result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_run_timeout.tst"), timeout_source, &timeout_script);
  U64 begin_us = now_time_us();
  if (t_result_is_ok(result)) { result = t_script_execute(&timeout_script); }
  U64 elapsed_us = now_time_us() - begin_us;
  T_Ok(result.code == T_ResultCode_Mismatch);
  T_Ok(g_last_exit_code == 999);
  T_Ok(elapsed_us < TIMEOUT_SEC(5));
#else
  TestSkip();
#endif
}

TEST(generic_run_rejects_unsafe_path)
{
  String8 source = str8_lit(
      "test:\n"
      "{\n"
      "  steps:\n"
      "  {\n"
      "    run: { path: \"../cmd.exe\" }\n"
      "    run: { path: \"..\\cmd.exe\" }\n"
      "    run: { path: \"C:\\Windows\\System32\\cmd.exe\" }\n"
      "  }\n"
      "}\n");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_run_unsafe_path.tst"), source, &script);
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(result.diagnostics.count == 3);
}

TEST(generic_build_default_output)
{
#if OS_WINDOWS
  String8 source = str8_lit(
      "test:\n"
      "{\n"
      "  artifacts:\n"
      "  {\n"
      "    source: { file_name: \"main.c\", text: { data: \"int main(void) { return 0; }\" } }\n"
      "  }\n"
      "  build:\n"
      "  {\n"
      "    windows: { compile_link: \"/nologo main.c\" }\n"
      "    linux: { compile_link: \"main.c\" }\n"
      "  }\n"
      "}\n");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_build.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  T_Ok(file_path_exists(t_make_file_path(arena, str8_lit("main.exe"))));
#else
  TestSkip();
#endif
}

TEST(generic_build_output_with_spaces)
{
#if OS_WINDOWS
  String8 source = str8_lit(
      "test:\n"
      "{\n"
      "  artifacts:\n"
      "  {\n"
      "    source: { file_name: \"main.c\", text: { data: \"int main(void) { return 0; }\" } }\n"
      "  }\n"
      "  build:\n"
      "  {\n"
      "    compile_link: { args: \"/nologo main.c\", output: \"main image.exe\" }\n"
      "  }\n"
      "}\n");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_build_spaces.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  T_Ok(file_path_exists(t_make_file_path(arena, str8_lit("main image.exe"))));
#else
  TestSkip();
#endif
}

TEST(generic_build_previous_exit_condition)
{
#if OS_WINDOWS
  String8 source = str8_lit(
      "test:\n"
      "{\n"
      "  build:\n"
      "  {\n"
      "    compile: { args: \"missing.c\", output: none, expect_exit: any }\n"
      "    compile: { args: \"also_missing.c\", output: none, when_previous_exit: 0 }\n"
      "  }\n"
      "}\n");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("generic_build_previous_exit.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
#else
  TestSkip();
#endif
}

TEST(expect_pdb_operation)
{
  String8 source = str8_lit(
      "test: { steps: { expect_pdb: { path: \"generated.pdb\", expected: { pdb: { fixed_streams: { info: { present: true }, tpi: { present: true }, "
      "dbi: { present: true }, ipi: { present: true } }, tpi: { leaf_count: 0, header_only: true }, ipi: { leaf_count: 0, header_only: true } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("expect_pdb_operation.tst"), source, &script);
  T_Ok(t_result_is_ok(result));

  // The operation must not snapshot the side file during script parsing.
  if (t_result_is_ok(result)) {
    String8 pdb = t_codec_test_pdb_data(arena);
    T_Ok(t_write_file(str8_lit("generated.pdb"), pdb));
    result = t_script_execute(&script);
  }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
}

TEST(expect_pdb_rejects_unsafe_path)
{
  String8 source = str8_lit("test: { steps: { expect_pdb: { path: \"generated.pdb\", expected: { pdb: { stream_count: 0 } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("unsafe_pdb.tst"), source, &script);
  T_Ok(t_result_is_ok(result) && script.command_count == 1);
  if (t_result_is_ok(result) && script.command_count == 1) {
    t_codec_child(script.commands[0].arguments, "path")->first->string = str8_lit("..");
    script.result = (T_Result){0};
    T_ParseContext parse = {arena, &script, &t_codec_script_suite, script.file_path, script.source, str8_lit("expect_pdb")};
    result = t_codec_expect_pdb_validate(&parse, script.commands[0].arguments);
  }
  T_Ok(result.code == T_ResultCode_ValidationError);
}

TEST(pdb_codec_rejects_malformed_tpi)
{
  String8 data = t_codec_test_pdb_data(arena);
  MSF_RawStreamTable *streams = msf_raw_stream_table_from_data(arena, data);
  T_Ok(streams != 0 && streams->stream_count > PDB_FixedStream_Tpi && streams->streams[PDB_FixedStream_Tpi].page_count > 0);
  if (streams != 0 && streams->stream_count > PDB_FixedStream_Tpi && streams->streams[PDB_FixedStream_Tpi].page_count > 0) {
    U32 page = streams->streams[PDB_FixedStream_Tpi].u.page_indices_u32[0];
    PDB_TpiHeader *header = (PDB_TpiHeader *)(data.str + (U64)page * streams->page_size);
    header->leaf_data_size = max_U32;
  }
  T_Context decode = {.arena = arena};
  MD_Node *semantic = 0;
  T_Result result = t_codec_pdb_decode(&decode, data, &semantic);
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(semantic == 0);
}

TEST(expect_pdb_indexes_linker_output)
{
  String8 raw_symbol = cv_make_symbol(arena, CV_SymKind_GDATA32,
                                      cv_make_data32(arena, (CV_SymData32){.off = 1, .sec = 1}, str8_lit("indexed_data")));
  CV_Symbol symbol = cv_symbol_from_ptr(raw_symbol.str);
  CV_DebugS debug_s = {0};
  str8_list_push(arena, cv_sub_section_ptr_from_debug_s(&debug_s, CV_C13SubSectionKind_Symbols), cv_data_from_symbol(arena, &symbol, CV_SymbolAlign));
  String8List debug_s_data = cv_data_from_debug_s_c13(arena, &debug_s, 1);
  T_Ok(t_write_def_obj("indexed.obj", (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("x"), .flags = "rx:code"},
                                         {"debug", ".debug$S", str8_list_join(arena, &debug_s_data, 0), .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable}, {0}},
      .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_ExternFunc("public_func", "text", 0), {0}},
  }));
  T_Ok(t_write_entry_obj());
  t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:indexed.exe /pdbstripped:indexed.stripped.pdb entry.obj indexed.obj");
  T_Ok(g_last_exit_code == 0);

  String8 source = str8_lit(
      "test: { steps: { expect_pdb: { path: \"indexed.pdb\", expected: { pdb: { "
      "gsi: { symbols: { indexed_data: { kind: S_GDATA32 } } }, psi: { symbols: { public_func: { kind: S_PUB32, flags: 2 } } }, "
      "global_symbols: { kind_counts: { S_GDATA32: 1 } }, dbi: { modules: { module_1: { object_file_name: \"indexed.obj\" } } } } } }, "
      "expect_pdb: { path: \"indexed.stripped.pdb\", expected: { pdb: { tpi: { header_only: true }, ipi: { header_only: true }, "
      "global_symbols: { non_public_or_proc_ref_symbol_count: 0 } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("indexed_pdb.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
}

TEST(link_large_import_object)
{
  // Each named import creates three sections in the synthesized DLL object.
  // Add a direct function reference as well to exercise its jump thunk.
  U32 import_count = 22000;
  COFF_LibWriter *lib = coff_lib_writer_alloc();
  COFF_ObjWriter *obj = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  COFF_ObjSection *refs = coff_obj_writer_push_section(obj, str8_lit(".data"),
      COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite|COFF_SectionFlag_Align8Bytes,
      str8(push_array(arena, U8, (import_count + 1)*8), (import_count + 1)*8));
  for (U32 i = 0; i < import_count; i += 1) {
    String8 name = str8f(arena, "import_%05u", i);
    coff_lib_writer_push_import(lib, COFF_MachineType_X64, 0, str8_lit("large.dll"), COFF_ImportBy_Name, name, 0, COFF_ImportHeader_Code);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj, str8f(arena, "__imp_%S", name));
    coff_obj_writer_section_push_reloc_addr(obj, refs, i*8, symbol);
  }
  COFF_ObjSymbol *func = coff_obj_writer_push_symbol_undef_func(obj, str8f(arena, "import_%05u", import_count - 1));
  coff_obj_writer_section_push_reloc_addr(obj, refs, import_count*8, func);
  T_Ok(t_write_file(str8_lit("refs.obj"), coff_obj_writer_serialize(arena, obj)));
  T_Ok(t_write_file(str8_lit("large.lib"), coff_lib_writer_serialize(arena, lib, 0, 0, 1)));
  coff_obj_writer_release(&obj);
  coff_lib_writer_release(&lib);
  T_Ok(t_write_entry_obj());
  U64 link_begin_us = now_time_us();
  t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /opt:ref /rad_log:summary /out:large.exe entry.obj refs.obj large.lib");
  U64 link_elapsed_ms = (now_time_us() - link_begin_us) / 1000 + 1;
  T_Ok(g_last_exit_code == 0);
  // A mismatched phase begin/end used to report system uptime for modules.
  // Use the enclosing invocation, not a fixed performance threshold.
  U64 mod_pos = str8_find_needle(g_output, 0, str8_lit(" mod="), 0);
  T_Ok(mod_pos < g_output.size);
  if (mod_pos < g_output.size) {
    String8 mod = str8_skip(g_output, mod_pos + 5);
    mod = str8_prefix(mod, str8_find_needle(mod, 0, str8_lit("/"), 0));
    U64 mod_ms = 0;
    T_Ok(try_u64_from_str8_c_rules(mod, &mod_ms));
    T_Ok(mod_ms <= link_elapsed_ms);
  }
  String8 image = t_read_file(arena, str8_lit("large.exe"));
  PE_BinInfo bin = pe_bin_info_from_data(arena, image);
  T_Ok(bin.data_dir_count > PE_DataDirectoryIndex_IMPORT);
  COFF_SectionHeader *sections = (COFF_SectionHeader *)(image.str + bin.section_table_range.min);
  PE_ParsedStaticImportTable imports = pe_static_imports_from_data(arena, bin.is_pe32, bin.section_count, sections,
      image, bin.data_dir_franges[PE_DataDirectoryIndex_IMPORT]);
  T_Ok(imports.count == 1);
  T_Ok(str8_match(imports.v[0].name, str8_lit("large.dll"), 0));
  T_Ok(imports.v[0].import_count == import_count);
  for (U32 i = 0; i < import_count; i += 1) {
    PE_ParsedImport *import = &imports.v[0].imports[i];
    T_Ok(import->type == PE_ParsedImport_Name);
    T_Ok(str8_match(import->u.name.string, str8f(arena, "import_%05u", i), 0));
  }
}

TEST(pdbstripped_coff_artifact_parity)
{
  String8 path = str8f(arena, "%S/linker/tests/pdbstripped.tst", t_src_path());
  String8 source = data_from_file_path(arena, path);
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, path, source, &script);
  for (T_Artifact *artifact = script.first_artifact; t_result_is_ok(result) && artifact != 0; artifact = artifact->next) {
    if (artifact->codec->encode != 0) { result = artifact->codec->encode(&script, artifact); }
  }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));

  String8 raw_symbols[] = {
    cv_make_symbol(arena, CV_SymKind_OBJNAME, cv_make_obj_name(arena, str8_lit("debug.obj"), 0x123)),
    cv_make_symbol(arena, CV_SymKind_GPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("global_proc"))),
    cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),

    cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("global_typedef"))),

    cv_make_symbol(arena, CV_SymKind_LPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("local_proc"))),
    cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("local_typedef"))),
    cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),
  };
  CV_DebugS debug_s = {0};
  for EachElement(i, raw_symbols) {
    CV_Symbol symbol = cv_symbol_from_ptr(raw_symbols[i].str);
    str8_list_push(arena, cv_sub_section_ptr_from_debug_s(&debug_s, CV_C13SubSectionKind_Symbols), cv_data_from_symbol(arena, &symbol, CV_SymbolAlign));
  }
  String8List raw_debug_s_list = cv_data_from_debug_s_c13(arena, &debug_s, 1);
  String8 raw_debug_s = str8_list_join(arena, &raw_debug_s_list, 0);
  String8 expected_debug = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){{"debug_s", ".debug$S", raw_debug_s, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable}, {0}},
  });
  String8 expected_pub = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("FOOBAR"), .flags = "rx:code"}, {"data", ".data", str8_lit_comp("QWE"), .flags = "rw:data"}, {0}},
      .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_ExternFunc("global_func", "text", 1), T_COFF_DefSymbol_Extern("global_var", "data", 1),
                                       T_COFF_DefSymbol_Static("static_var", "data", 1), {0}},
  });

  T_Artifact *debug = t_artifact_from_name(&script, str8_lit("debug_obj"));
  T_Artifact *pub = t_artifact_from_name(&script, str8_lit("pub_obj"));
  T_Artifact *entry = t_artifact_from_name(&script, str8_lit("entry_obj"));
  if (debug != 0 && !str8_match(debug->data, expected_debug, 0)) {
    U64 mismatch = 0;
    while (mismatch < Min(debug->data.size, expected_debug.size) && debug->data.str[mismatch] == expected_debug.str[mismatch]) { mismatch += 1; }
    test_outf("debug.obj differs at byte %llu (script size %llu, helper size %llu, helper section size %llu)\n", mismatch, debug->data.size, expected_debug.size, raw_debug_s.size);
  }
  T_Ok(debug != 0 && str8_match(debug->data, expected_debug, 0));
  T_Ok(pub != 0 && str8_match(pub->data, expected_pub, 0));
  T_Ok(entry != 0 && str8_match(entry->data, t_make_entry_obj(arena), 0));
}
