// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// --- Build Options -----------------------------------------------------------

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD PE/COFF Linker"

// --- Arena -------------------------------------------------------------------

#define ARENA_FREE_LIST 1

// --- Third Party -------------------------------------------------------------

#include "base_ext/base_blake3.h"
#include "base_ext/base_blake3.c"
#define MD5_API static
#include "third_party/md5/md5.c"
#include "third_party/md5/md5.h"
#define XXH_INLINE_ALL
#define XXH_IMPLEMENTATION
#define XXH_STATIC_LINKING_ONLY
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"

// --- Code Base ---------------------------------------------------------------

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "hash_table.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "coff/coff_obj_writer.h"
#include "coff/coff_lib_writer.h"
#include "pe/pe.h"
#include "pe/pe_section_flags.h"
#include "pe/pe_make_import_table.h"
#include "pe/pe_make_export_table.h"
#include "pe/pe_make_debug_dir.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "msvc_crt/msvc_crt.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "hash_table.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "coff/coff_lib_writer.c"
#include "pe/pe.c"
#include "pe/pe_make_import_table.c"
#include "pe/pe_make_export_table.c"
#include "pe/pe_make_debug_dir.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "msvc_crt/msvc_crt.c"

// --- RDI ---------------------------------------------------------------------

#include "rdi/rdi_local.h"
#include "rdi/rdi_local.c"

// --- Code Base Extensions ----------------------------------------------------

#include "base_ext/base_inc.h"
#include "thread_pool/thread_pool.h"
#include "codeview_ext/codeview.h"
#include "pdb_ext/msf_builder.h"
#include "pdb_ext/pdb.h"
#include "pdb_ext/pdb_helpers.h"
#include "pdb_ext/pdb_builder.h"

#include "base_ext/base_inc.c"
#include "thread_pool/thread_pool.c"
#include "codeview_ext/codeview.c"
#include "pdb_ext/msf_builder.c"
#include "pdb_ext/pdb.c"
#include "pdb_ext/pdb_helpers.c"
#include "pdb_ext/pdb_builder.c"

// --- RDI Builder -------------------------------------------------------------

#include "rdi/rdi_builder.h"
#include "rdi/rdi_coff.h" 
#include "rdi/rdi_cv.h"

#include "rdi/rdi_builder.c"
#include "rdi/rdi_coff.c"
#include "rdi/rdi_cv.c"

// --- Linker ------------------------------------------------------------------

#include "lnk_error.h"
#include "lnk_log.h"
#include "lnk_timer.h"
#include "lnk_io.h"
#include "lnk_cmd_line.h"
#include "lnk_config.h"
#include "lnk_symbol_table.h"
#include "lnk_section_table.h"
#include "lnk_debug_helper.h"
#include "lnk_obj.h"
#include "lnk_lib.h"
#include "lnk_debug_info.h"
#include "lnk.h"

#include "lnk_error.c"
#include "lnk_log.c"
#include "lnk_timer.c"
#include "lnk_io.c"
#include "lnk_cmd_line.c"
#include "lnk_config.c"
#include "lnk_symbol_table.c"
#include "lnk_section_table.c"
#include "lnk_obj.c"
#include "lnk_debug_helper.c"
#include "lnk_lib.c"
#include "lnk_debug_info.c"

// -----------------------------------------------------------------------------

internal LNK_Config *
lnk_config_from_argcv(Arena *arena, int argc, char **argv)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List raw_cmd_line = os_string_list_from_argcv(arena, argc, argv);

  // remove exe name first argument
  str8_list_pop_front(&raw_cmd_line); 

  // parse command line
  String8List unwrapped_cmd_line = lnk_unwrap_rsp(scratch.arena, raw_cmd_line);
  LNK_CmdLine cmd_line           = lnk_cmd_line_parse_windows_rules(scratch.arena, unwrapped_cmd_line);

  // setup default flags
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Align,     "%u", KB(4));
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Debug,     "none");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_FileAlign, "%u", 512);
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Dll)) {
    lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_SubSystem, "%S", pe_string_from_subsystem(PE_WindowsSubsystem_WINDOWS_GUI));
  }
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_HighEntropyVa,               "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_ManifestUac,                 "\"level='asInvoker' uiAccess='false'\"");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_NxCompat,                    "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_LargeAddressAware,           "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_PdbAltPath,                  "%%_RAD_PDB_PATH%%");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_PdbPageSize,                 "%u", KB(4));
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_TimeStamp,               "%u", os_get_process_start_time_unix());
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Age,                     "%u", 1);
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll, "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_DoMerge,                 "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_EnvLib,                  "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Exe,                     "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Guid,                    "imageblake3");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_LargePages,              "no");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_LinkVer,                 "14.0");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_OsVer,                   "6.0");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_PageSize,                "%u", KB(4));
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_PathStyle,               "system");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Workers,                 "%u", os_get_system_info()->logical_processor_count);
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_TargetOs,                "windows");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_DebugAltPath,            "%%_RAD_RDI_PATH%%");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_MemoryMapFiles,          "");
#if BUILD_DEBUG
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Log, "debug");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Log, "io_write");
#else
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SuppressError, "%u", LNK_Error_InvalidTypeIndex);
#endif

  // default section merges
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".xdata=.rdata");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".00cfg=.rdata");
  // TODO: .tls must be always first contribution in .data section because compiler generates TLS relative movs
  //lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".tls=.data");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".idata=.data");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".didat=.data");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".edata=.rdata");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".RAD_LINK_PE_DEBUG_DIR=.rdata");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".RAD_LINK_PE_DEBUG_DATA=.rdata");

  // sections to remove from the image
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".debug");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gehcont");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gfids");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gxfg");

  // set limits on unresolved symbol errors
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_MapLinesForUnresolvedSymbols, "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_UnresolvedSymbolLimit, "1000");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_UnresolvedSymbolRefLimit, "10");

  // set default max worker count 
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_SharedThreadPool)) {
    lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SharedThreadPoolMaxWorkers, "");
  }

  if (!lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Rad_MtPath)) {
    lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_MtPath, "%s", LNK_MANIFEST_MERGE_TOOL_NAME);
  }

  // when /FORCE is specified on the command line, do not stop on these errors
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Force)) {
    g_error_mode_arr[LNK_Error_UnresolvedSymbol] = LNK_ErrorMode_Continue;
  }

  // init config
  LNK_Config *config = lnk_config_from_cmd_line(raw_cmd_line, cmd_line);

#if PROFILE_TELEMETRY
  {
    String8 cmdl = str8_list_join(scratch.arena, &config->raw_cmd_line, &(StringJoin){ .sep = str8_lit_comp(" ") });
    tmMessage(0, TMMF_ICON_NOTE, "Command Line: %.*s", str8_varg(cmdl));
  }
#endif

  if (lnk_get_log_status(LNK_Log_Debug)) {
    String8 full_cmd_line = str8_list_join(scratch.arena, &raw_cmd_line, &(StringJoin){ .sep = str8_lit_comp(" ") });
    fprintf(stderr, "--------------------------------------------------------------------------------\n");
    fprintf(stderr, "Command Line: %.*s\n", str8_varg(full_cmd_line));
    fprintf(stderr, "Work Dir    : %.*s\n", str8_varg(config->work_dir));
    fprintf(stderr, "--------------------------------------------------------------------------------\n");
  }

  scratch_end(scratch);
  return config;
}

internal String8
lnk_make_full_path(Arena *arena, PathStyle system_path_style, String8 work_dir, String8 path)
{
  ProfBeginFunction();
  String8 result = str8(0,0);
  PathStyle path_style = path_style_from_str8(path);
  if (path_style == PathStyle_Relative) {
    Temp scratch = scratch_begin(&arena, 1);
    String8List list = {0};
    str8_list_push(scratch.arena, &list, work_dir);
    str8_list_push(scratch.arena, &list, path);
    result = str8_path_list_join_by_style(arena, &list, system_path_style);
    scratch_end(scratch);
  } else {
    result = push_str8_copy(arena, path);
  }
  ProfEnd();
  return result;
}

internal
THREAD_POOL_TASK_FUNC(lnk_blake3_hasher_task)
{
  ProfBeginFunction();
  
  LNK_Blake3Hasher *task     = raw_task;
  Rng1U64           range    = task->ranges[task_id];
  String8           sub_data = str8_substr(task->data, range);
  
  blake3_hasher hasher; blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, sub_data.str, sub_data.size);
  blake3_hasher_finalize(&hasher, (U8 *)task->hashes[task_id].u64, sizeof(task->hashes[task_id].u64));
  
  ProfEnd();
}

internal U128
lnk_blake3_hash_parallel(TP_Context *tp, U64 chunk_count, String8 data)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  ProfBegin("Hash Chunks");
  LNK_Blake3Hasher task = {0};
  task.data             = data;
  task.ranges           = tp_divide_work(scratch.arena, data.size, chunk_count);
  task.hashes           = push_array(scratch.arena, U128, chunk_count);
  tp_for_parallel(tp, 0, chunk_count, lnk_blake3_hasher_task, &task);
  ProfEnd();
  
  ProfBegin("Combine Hashes");
  blake3_hasher hasher; blake3_hasher_init(&hasher);
  for (U64 i = 0; i < chunk_count; ++i) {
    blake3_hasher_update(&hasher, (U8 *)task.hashes[i].u64, sizeof(task.hashes[i].u64));
  }
  U128 result;
  blake3_hasher_finalize(&hasher, (U8 *)result.u64, sizeof(result.u64));
  ProfEnd();
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal String8
lnk_make_linker_manifest(Arena      *arena,
                         B32         manifest_uac,
                         String8     manifest_level,
                         String8     manifest_ui_access,
                         String8List manifest_dependency_list)
{
  // TODO: we write a temp file with manifest attributes collected from obj directives and command line switches
  // so we can pass file to mt.exe or llvm-mt.exe, when we have our own tool for merging manifest we can switch
  // to writing manifest file in memory to skip roun-trip to disk

  Temp scratch = scratch_begin(&arena, 1);

  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  str8_serial_push_string(scratch.arena, &srl, str8_lit(
                                                "<?xml version=\"1.0\" standalone=\"yes\"?>\n"
                                                "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\"\n"
                                                "          manifestVersion=\"1.0\">\n"));
  if (manifest_uac) {
#if 1
    String8 uac = push_str8f(scratch.arena,
                             "   <trustInfo>\n"
                             "     <security>\n"
                             "       <requestedPrivileges>\n"
                             "         <requestedExecutionLevel level=%S uiAccess=%S/>\n"
                             "       </requestedPrivileges>\n"
                             "     </security>\n"
                             "   </trustInfo>\n",
                             manifest_level,
                             manifest_ui_access);
#else
    String8 uac = push_str8f(scratch.arena,
        	"<ms_asmv2:trustInfo xmlns:ms_asmv2="urn:schemas-microsoft-com:asm.v2" xmlns="urn:schemas-microsoft-com:asm.v3">\n"
		        "<ms_asmv2:security>"
			        "<ms_asmv2:requestedPrivileges>"
				        "<ms_asmv2:requestedExecutionLevel level=%S uiAccess=%S>"
                "</ms_asmv2:requestedExecutionLevel>"
			        "</ms_asmv2:requestedPrivileges>"
		        "</ms_asmv2:security>"
	        "</ms_asmv2:trustInfo>", manifest_level, manifest_ui_access);
#endif
    str8_serial_push_string(scratch.arena, &srl, uac);
  }
  for (String8Node *node = manifest_dependency_list.first; node != 0; node = node->next) {
    String8 dep = push_str8f(scratch.arena, 
                             " <dependency>\n"
                             "   <dependentAssembly>\n"
                             "     <assemblyIdentity %S/>\n"
                             "   </dependentAssembly>\n"
                             " </dependency>\n",
                             node->string);
    str8_serial_push_string(scratch.arena, &srl, dep);
  }
  str8_serial_push_string(scratch.arena, &srl, str8_lit("</assembly>\n"));

  String8 result = str8_list_join(arena, &srl, 0);

  scratch_end(scratch);
  return result;
}

internal void
lnk_merge_manifest_files(String8 mt_path, String8 out_name, String8List manifest_path_list)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  
  String8List cmd_line = {0};
  str8_list_push(scratch.arena, &cmd_line, mt_path);
  str8_list_pushf(scratch.arena, &cmd_line, "-out:%S", out_name);
  str8_list_pushf(scratch.arena, &cmd_line, "-nologo");

  // register input manifest files on command line
  String8 work_dir = os_get_current_path(scratch.arena);
  for (String8Node *man_node = manifest_path_list.first;
       man_node != 0;
       man_node = man_node->next) {
    // resolve relative inputs
    String8 full_path = path_absolute_dst_from_relative_dst_src(scratch.arena, man_node->string, work_dir);

    // normalize slashes
    full_path = path_convert_slashes(scratch.arena, full_path, PathStyle_UnixAbsolute);

    // push input to command line
    str8_list_pushf(scratch.arena, &cmd_line, "-manifest");
    str8_list_push(scratch.arena, &cmd_line, full_path);
  }
  
  // launch mt.exe with our command line
  OS_ProcessLaunchParams launch_opts = {0};
  launch_opts.cmd_line               = cmd_line;
  launch_opts.inherit_env            = 1;
  launch_opts.consoleless            = 1;
  OS_Handle mt_handle = os_process_launch(&launch_opts);
  if (os_handle_match(mt_handle, os_handle_zero())) {
    lnk_error(LNK_Error_Mt, "unable to start process: %S", mt_path);
  } else {
    os_process_join(mt_handle, max_U64);
    os_process_detach(mt_handle);
  }
  
  scratch_end(scratch);
  ProfEnd();
} 

internal String8
lnk_manifest_from_inputs(Arena       *arena,
                         LNK_IO_Flags io_flags,
                         String8      mt_path,
                         String8      manifest_name,
                         B32          manifest_uac,
                         String8      manifest_level,
                         String8      manifest_ui_access,
                         String8List  input_manifest_path_list,
                         String8List  deps_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List unique_deps = remove_duplicates_str8_list(scratch.arena, deps_list);

  String8 manifest_data;

  if (input_manifest_path_list.node_count > 0) {
    ProfBegin("Merge Manifests");
    
    String8 linker_manifest = lnk_make_linker_manifest(scratch.arena, manifest_uac, manifest_level, manifest_ui_access, unique_deps);

    // write linker manifest to temp file
    String8 linker_manifest_path = push_str8f(scratch.arena, "%S.manifest.temp", manifest_name);
    lnk_write_data_to_file_path(linker_manifest_path, str8_zero(), linker_manifest);

    String8List unique_input_manifest_paths = remove_duplicates_str8_list(scratch.arena, input_manifest_path_list);

    // push linker manifest
    str8_list_push(scratch.arena, &unique_input_manifest_paths, linker_manifest_path);

    // launch mt.exe to merge input manifests
    String8 merged_manifest_path = push_str8f(scratch.arena, "%S.manifest.merged", manifest_name);
    lnk_merge_manifest_files(mt_path, merged_manifest_path, unique_input_manifest_paths);

    // read mt.exe output from disk
    manifest_data = lnk_read_data_from_file_path(arena, io_flags, merged_manifest_path);
    if (manifest_data.size == 0) {
      lnk_error(LNK_Error_Mt, "unable to find mt.exe output manifest on disk, expected path \"%S\"", merged_manifest_path);
    }

    // cleanup disk
    os_delete_file_at_path(linker_manifest_path);
    os_delete_file_at_path(merged_manifest_path);

    ProfEnd();
  } else {
    manifest_data = lnk_make_linker_manifest(arena, manifest_uac, manifest_level, manifest_ui_access, unique_deps);
  }

  scratch_end(scratch);
  return manifest_data;
}

internal String8
lnk_make_null_obj(Arena *arena)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_Unknown);

  // push null symbol
  COFF_ObjSymbol *null_abs = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(LNK_NULL_SYMBOL), 0, COFF_SymStorageClass_External);

  // push import stub
  coff_obj_writer_push_symbol_weak(obj_writer, str8_lit(LNK_IMPORT_STUB), COFF_WeakExt_SearchAlias, null_abs);

  // push .debug$T sections with null leaf
  String8 null_debug_data;
  {
    String8 raw_null_leaf = cv_serialize_raw_leaf(obj_writer->arena, CV_LeafKind_NOTYPE, str8(0,0), 1);

    String8List srl = {0};
    str8_serial_begin(obj_writer->arena, &srl);
    str8_serial_push_u32(obj_writer->arena, &srl, CV_Signature_C13);
    str8_serial_push_string(obj_writer->arena, &srl, raw_null_leaf);
    null_debug_data = str8_serial_end(obj_writer->arena, &srl);
  }
  coff_obj_writer_push_section(obj_writer, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS, null_debug_data);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}

internal int
lnk_res_string_id_is_before(void *raw_a, void *raw_b)
{
  PE_Resource *a = raw_a;
  PE_Resource *b = raw_b;
  Assert(a->id.type == COFF_ResourceIDType_String);
  Assert(b->id.type == COFF_ResourceIDType_String);
  int is_before = str8_is_before_case_sensitive(&a->id.u.string, &b->id.u.string);
  return is_before;
}

internal int
lnk_res_number_id_is_before(void *raw_a, void *raw_b)
{
  PE_Resource *a = raw_a;
  PE_Resource *b = raw_b;
  Assert(a->id.type == COFF_ResourceIDType_Number);
  Assert(b->id.type == COFF_ResourceIDType_Number);
  int is_before = u16_is_before(&a->id.u.number, &b->id.u.number);
  return is_before;
}

internal void
lnk_serialize_pe_resource_tree(COFF_ObjWriter *obj_writer, PE_ResourceDir *root_dir)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  struct Stack {
    struct Stack          *next;
    U64                    arr_idx;
    U64                    res_idx[2];
    PE_ResourceArray       res_arr[2];
    COFF_ResourceDirEntry *coff_entry_arr[2];
  };
  struct Stack *stack = push_array(scratch.arena, struct Stack, 1);
  // init stack
  {
    PE_Resource *root_wrapper = push_array(scratch.arena, PE_Resource, 1);
    root_wrapper->id.type     = COFF_ResourceIDType_Number;
    root_wrapper->id.u.number = 0;
    root_wrapper->kind        = PE_ResDataKind_DIR;
    root_wrapper->u.dir       = root_dir;

    COFF_ResourceDirEntry *root_dir = push_array(scratch.arena, COFF_ResourceDirEntry, 1);

    stack->res_arr[0].count = 1;
    stack->res_arr[0].v     = root_wrapper;

    stack->coff_entry_arr[0] = root_dir;
    stack->coff_entry_arr[1] = 0;
  }

  COFF_ObjSection *rsrc1 = coff_obj_writer_push_section(obj_writer, str8_lit(".rsrc$01"), PE_RSRC1_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *rsrc2 = coff_obj_writer_push_section(obj_writer, str8_lit(".rsrc$02"), PE_RSRC2_SECTION_FLAGS, str8_zero());
  
  for (; stack; ) {
    for (; stack->arr_idx < ArrayCount(stack->res_arr); stack->arr_idx += 1) {
      for (; stack->res_idx[stack->arr_idx] < stack->res_arr[stack->arr_idx].count; ) {
        U64          res_idx = stack->res_idx[stack->arr_idx]++;
        PE_Resource *res     = &stack->res_arr[stack->arr_idx].v[res_idx];

        {
          COFF_ResourceDirEntry *coff_entry = &stack->coff_entry_arr[stack->arr_idx][res_idx];

          // assign entry data offset
          coff_entry->id.data_entry_offset = safe_cast_u32(rsrc1->data.total_size);

          // set directory flag
          if (res->kind == PE_ResDataKind_DIR) {
            coff_entry->id.data_entry_offset |= COFF_Resource_SubDirFlag;
          }
        }

        switch (res->kind) {
        case PE_ResDataKind_DIR: {
          // fill out directory header
          COFF_ResourceDirTable *dir_header = push_array(obj_writer->arena, COFF_ResourceDirTable, 1);
          dir_header->characteristics       = res->u.dir->characteristics;
          dir_header->time_stamp            = res->u.dir->time_stamp;
          dir_header->major_version         = res->u.dir->major_version;
          dir_header->minor_version         = res->u.dir->minor_version;
          dir_header->name_entry_count      = res->u.dir->named_list.count;
          dir_header->id_entry_count        = res->u.dir->id_list.count;

          // sort input resources
          PE_ResourceArray named_array = pe_resource_list_to_array(scratch.arena, &res->u.dir->named_list);
          PE_ResourceArray id_array    = pe_resource_list_to_array(scratch.arena, &res->u.dir->id_list);
          radsort(named_array.v, named_array.count, lnk_res_string_id_is_before);
          radsort(id_array.v,    id_array.count,    lnk_res_number_id_is_before);

          // allocate COFF entries
          COFF_ResourceDirEntry *named_entries = push_array(obj_writer->arena, COFF_ResourceDirEntry, named_array.count);
          COFF_ResourceDirEntry *id_entries    = push_array(obj_writer->arena, COFF_ResourceDirEntry, id_array.count);

          // push header and entries
          str8_list_push(obj_writer->arena, &rsrc1->data, str8_struct(dir_header));
          str8_list_push(obj_writer->arena, &rsrc1->data, str8_array(named_entries, named_array.count));
          str8_list_push(obj_writer->arena, &rsrc1->data, str8_array(id_entries, id_array.count));

          // fill out named ids
          for (U64 i = 0; i < named_array.count; i += 1) {
            PE_Resource            src = named_array.v[i];
            COFF_ResourceDirEntry *dst = &named_entries[i];

            // append resource name
            U32     res_name_off = safe_cast_u32(rsrc1->data.total_size);
            String8 res_name     = coff_resource_string_from_str8(obj_writer->arena, res->id.u.string);
            str8_list_push(obj_writer->arena, &rsrc1->data, res_name);

            // not sure why high bit has to be turned on here since number id and string id entries are
            // in separate arrays but windows doesn't treat name offset like string without this bit.
            dst->name.offset = (1 << 31) | res_name_off;
          }

          // fill out number ids
          for (U64 i = 0; i < id_array.count; i += 1) {
            PE_Resource            src = id_array.v[i];
            COFF_ResourceDirEntry *dst = &id_entries[i];
            dst->name.id = src.id.u.number;
          }

          // fill out sub directory stack frame
          struct Stack *frame      = push_array(scratch.arena, struct Stack, 1);
          frame->res_arr[0]        = named_array;
          frame->res_arr[1]        = id_array;
          frame->coff_entry_arr[0] = named_entries;
          frame->coff_entry_arr[1] = id_entries;
          SLLStackPush(stack, frame);
        } goto yield; // recurse to sub directory

        case PE_ResDataKind_COFF_RESOURCE: {
          // fill out resource header
          COFF_ResourceDataEntry *coff_res = push_array(obj_writer->arena, COFF_ResourceDataEntry, 1);
          coff_res->data_size              = res->u.coff_res.data.size;
          coff_res->data_voff              = 0; // relocated
          coff_res->code_page              = 0; // TODO: whats this for? (lld-link writes zero)

          // emit symbol for resource data
          U32 resdat_off = safe_cast_u32(rsrc2->data.total_size);
          COFF_ObjSymbol *resdat = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("resdat"), resdat_off, rsrc2);

          // emit reloc for 'data_voff'
          U64 apply_off   = rsrc1->data.total_size + OffsetOf(COFF_ResourceDataEntry, data_voff);
          U32 apply_off32 = safe_cast_u32(apply_off);
          coff_obj_writer_section_push_reloc(obj_writer, rsrc1, apply_off32, resdat, COFF_Reloc_X64_Addr32Nb);

          // push resource entry & data
          str8_list_push(obj_writer->arena, &rsrc1->data, str8_struct(coff_res));
          str8_list_push(obj_writer->arena, &rsrc2->data, res->u.coff_res.data);
        } break;

        case PE_ResDataKind_NULL: break;

        // we must not have this resource node here, it is used to represent on-disk version of entry
        case PE_ResDataKind_COFF_LEAF: InvalidPath;
        }
      }
    }
    SLLStackPop(stack);
    yield:;
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_add_resource_debug_s(COFF_ObjWriter *obj_writer,
                         String8         obj_path,
                         String8         cwd_path,
                         String8         exe_path,
                         CV_Arch         arch,
                         String8List     res_file_list,
                         MD5Hash        *res_hash_array)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  
  // init serial for tables
  String8List string_srl = {0};
  String8List file_srl   = {0};
  str8_serial_begin(scratch.arena, &string_srl);
  str8_serial_begin(scratch.arena, &file_srl);
  
  // reserve first byte for null
  str8_serial_push_u8(scratch.arena, &string_srl, 0);
  
  // build file and string table
  U64 node_idx = 0;
  for (String8Node *n = res_file_list.first; n != NULL; n = n->next, ++node_idx) {
    CV_C13Checksum checksum = {0};
    checksum.name_off = string_srl.total_size;
    checksum.len = sizeof(MD5Hash);
    checksum.kind = CV_C13ChecksumKind_MD5;
    str8_serial_push_struct(scratch.arena, &file_srl, &checksum);
    str8_serial_push_struct(scratch.arena, &file_srl, &res_hash_array[node_idx]);
    str8_serial_push_align(scratch.arena, &file_srl, CV_FileCheckSumsAlign);
    str8_serial_push_cstr(scratch.arena, &string_srl, n->string);
  }
  
  // build symbols
  String8 obj_data = cv_make_obj_name(scratch.arena, obj_path, 0);
  
  String8 exe_name_with_ext = str8_skip_last_slash(exe_path);
  String8 exe_name_ext = str8_skip_last_dot(exe_name_with_ext);
  String8 exe_name = str8_chop(exe_name_with_ext, exe_name_ext.size);
  if (exe_name_ext.size > 0) {
    exe_name = str8_chop(exe_name, 1);
  }
  String8 version_string = push_str8f(scratch.arena, BUILD_TITLE_STRING_LITERAL);
  String8 comp_data = cv_make_comp3(scratch.arena, CV_Compile3Flag_EC, CV_Language_CVTRES, arch,
                                    0, 0, 0, 0,
                                    1, 0, 1, 0,
                                    version_string);
  
  String8List env_list = {0};
  str8_list_push(scratch.arena, &env_list, str8_lit("cwd"));
  str8_list_push(scratch.arena, &env_list, cwd_path);
  str8_list_push(scratch.arena, &env_list, str8_lit("exe"));
  str8_list_push(scratch.arena, &env_list, exe_path);
  str8_list_push(scratch.arena, &env_list, str8_lit(""));
  str8_list_push(scratch.arena, &env_list, str8_lit(""));
  String8 envblock_data = cv_make_envblock(scratch.arena, env_list);
  
  String8 obj_symbol      = cv_make_symbol(scratch.arena, CV_SymKind_OBJNAME,  obj_data);
  String8 comp_symbol     = cv_make_symbol(scratch.arena, CV_SymKind_COMPILE3, comp_data);
  String8 envblock_symbol = cv_make_symbol(scratch.arena, CV_SymKind_ENVBLOCK, envblock_data);
  
  String8List symbol_srl = {0};
  str8_serial_begin(scratch.arena, &symbol_srl);
  str8_serial_push_string(scratch.arena, &symbol_srl, obj_symbol);
  str8_serial_push_string(scratch.arena, &symbol_srl, comp_symbol);
  str8_serial_push_string(scratch.arena, &symbol_srl, envblock_symbol);
  
  // build code view sub-sections
  String8List sub_sect_srl = {0};
  str8_serial_begin(scratch.arena, &sub_sect_srl);
  CV_Signature sig = CV_Signature_C13;
  str8_serial_push_struct(scratch.arena, &sub_sect_srl, &sig);
  
  CV_C13SubSectionHeader string_header;
  string_header.kind = CV_C13SubSectionKind_StringTable;
  string_header.size = string_srl.total_size;
  str8_serial_push_struct(scratch.arena, &sub_sect_srl, &string_header);
  str8_serial_push_data_list(scratch.arena, &sub_sect_srl, string_srl.first);
  str8_serial_push_align(scratch.arena, &sub_sect_srl, CV_C13SubSectionAlign);
  
  CV_C13SubSectionHeader file_header;
  file_header.kind = CV_C13SubSectionKind_FileChksms;
  file_header.size = file_srl.total_size;
  str8_serial_push_struct(scratch.arena, &sub_sect_srl, &file_header);
  str8_serial_push_data_list(scratch.arena, &sub_sect_srl, file_srl.first);
  str8_serial_push_align(scratch.arena, &sub_sect_srl, CV_C13SubSectionAlign);
  
  CV_C13SubSectionHeader symbol_header;
  symbol_header.kind = CV_C13SubSectionKind_Symbols;
  symbol_header.size = symbol_srl.total_size;
  str8_serial_push_struct(scratch.arena, &sub_sect_srl, &symbol_header);
  str8_serial_push_data_list(scratch.arena, &sub_sect_srl, symbol_srl.first);
  str8_serial_push_align(scratch.arena, &sub_sect_srl, CV_C13SubSectionAlign);
  
  String8 sub_sect_data = str8_serial_end(obj_writer->arena, &sub_sect_srl);
  coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS, sub_sect_data);
  
  scratch_end(scratch);
  ProfEnd();
}

internal String8
lnk_make_res_obj(Arena            *arena,
                 String8List       res_data_list,
                 String8List       res_path_list,
                 COFF_MachineType  machine,
                 U32               time_stamp,
                 String8           work_dir,
                 PathStyle         system_path_style,
                 String8           obj_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena,1);
  
  Assert(res_data_list.node_count == res_path_list.node_count);
  
  // load res files
  PE_ResourceDir *root_dir       = push_array(scratch.arena, PE_ResourceDir, 1);
  MD5Hash        *res_hash_array = push_array(scratch.arena, MD5Hash, res_data_list.node_count);
  U64 node_idx = 0;
  for (String8Node *node = res_data_list.first; node != 0; node = node->next, node_idx += 1) {
    res_hash_array[node_idx] = md5_hash_from_string(node->string);
    pe_resource_dir_push_res_file(scratch.arena, root_dir, node->string);
  }
  
  // convert res paths to stable paths
  String8List stable_res_file_list = {0};
  for (String8Node *node = res_path_list.first; node != 0; node = node->next) {
    String8 stable_res_path = lnk_make_full_path(scratch.arena, system_path_style, work_dir, node->string);
    str8_list_push(scratch.arena, &stable_res_file_list, stable_res_path);
  }
  
  // convert res to obj
  OS_ProcessInfo *process_info = os_get_process_info();
  String8List exe_path_strs = {0};
  str8_list_push(scratch.arena, &exe_path_strs, process_info->binary_path);
  String8 exe_path = str8_list_first(&exe_path_strs);

  String8 res_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

    // obj features
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@feat.00"), COFF_SymStorageClass_Static, MSCRT_FeatFlag_HAS_SAFE_SEH|MSCRT_FeatFlag_UNKNOWN_4);

    // serialize resource tree
    lnk_serialize_pe_resource_tree(obj_writer, root_dir);

    // push resource debug info
    lnk_add_resource_debug_s(obj_writer, obj_name, work_dir, exe_path, cv_arch_from_coff_machine(machine), stable_res_file_list, res_hash_array);

    // finalize obj
    res_obj = coff_obj_writer_serialize(arena, obj_writer);

    coff_obj_writer_release(&obj_writer);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return res_obj;
}

internal String8
lnk_make_linker_coff_obj(Arena            *arena,
                         COFF_TimeStamp    time_stamp,
                         COFF_MachineType  machine,
                         String8           cwd_path,
                         String8           exe_path,
                         String8           pdb_path,
                         String8           cmd_line,
                         String8           obj_name)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 debug_symbols = {0};
  {
    CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };
    
    // S_OBJ
    String8 obj_data = cv_make_obj_name(scratch.arena, obj_name, 0);
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_OBJNAME, obj_data);
    
    // S_COMPILE3
    String8 comp3_data = lnk_make_linker_compile3(scratch.arena, machine);
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);
    
    // S_ENVBLOCK
    String8List env_list = {0};
    str8_list_push(scratch.arena, &env_list, str8_lit("cwd"));
    str8_list_push(scratch.arena, &env_list, cwd_path);
    str8_list_push(scratch.arena, &env_list, str8_lit("exe"));
    str8_list_push(scratch.arena, &env_list, exe_path);
    str8_list_push(scratch.arena, &env_list, str8_lit("pdb"));
    str8_list_push(scratch.arena, &env_list, pdb_path);
    str8_list_push(scratch.arena, &env_list, str8_lit("cmd"));
    str8_list_push(scratch.arena, &env_list, cmd_line);
    str8_list_push(scratch.arena, &env_list, str8_lit(""));
    str8_list_push(scratch.arena, &env_list, str8_lit(""));
    cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_ENVBLOCK, cv_make_envblock(scratch.arena, env_list));

    // TODO: emit S_SECTION and S_COFFGROUP
    // TODO: emit S_TRAMPOLINE
    
    debug_symbols = lnk_make_debug_s(scratch.arena, symbol_list);
  }

  String8 obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);
    coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
    obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }
  
  scratch_end(scratch);
  return obj;
}

internal String8
lnk_make_linker_obj(Arena *arena, LNK_Config *config)
{
  ProfBeginFunction();

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(COFF_TimeStamp_Max, config->machine);

  // Emit __ImageBase symbol.
  //
  // This symbol is used with REL32 to compute delta from current IP
  // to the image base. CRT uses this trick to get to HINSTANCE * without
  // passing it around as a function argument.
  //
  //  100h: lea rax, [rip + ffffff00h] ; -100h 
  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("__ImageBase"), 0, COFF_SymStorageClass_External);
  
  { // load config symbols
    if (config->machine == COFF_MachineType_X86) {
      coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_SAFE_SE_HANDLER_TABLE_SYMBOL_NAME), 0, COFF_SymStorageClass_External);
      coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_SAFE_SE_HANDLER_COUNT_SYMBOL_NAME), 0, COFF_SymStorageClass_External);
    }
    
    // TODO: investigate IMAGE_ENCLAVE_CONFIG 32/64
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_ENCLAVE_CONFIG_SYMBOL_NAME), 0, COFF_SymStorageClass_External);
    
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_FLAGS_SYMBOL_NAME)        , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_FIDS_TABLE_SYMBOL_NAME)   , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_FIDS_COUNT_SYMBOL_NAME)   , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_IAT_TABLE_SYMBOL_NAME)    , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_IAT_COUNT_SYMBOL_NAME)    , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_LONGJMP_TABLE_SYMBOL_NAME), 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_LONGJMP_COUNT_SYMBOL_NAME), 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_EHCONT_TABLE_SYMBOL_NAME) , 0, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_EHCONT_COUNT_SYMBOL_NAME) , 0, COFF_SymStorageClass_External);
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);

  ProfEnd();
  return obj;
}

internal String8
lnk_make_obj_with_undefined_symbols(Arena *arena, String8List symbol_names)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_Unknown);
  for (String8Node *name_n = symbol_names.first; name_n != 0; name_n = name_n->next) {
    coff_obj_writer_push_symbol_undef(obj_writer, name_n->string);
  }
  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}

internal void
lnk_input_list_push_node(LNK_InputList *list, LNK_Input *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal void
lnk_input_list_concat_in_place(LNK_InputList *list, LNK_InputList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal LNK_InputPtrArray
lnk_array_from_input_list(Arena *arena, LNK_InputList list)
{
  LNK_InputPtrArray result = {0};
  result.v = push_array(arena, LNK_Input *, list.count);
  for (LNK_Input *node = list.first; node != 0; node = node->next, result.count += 1) {
    result.v[result.count] = node;
  }
  return result;
}

internal LNK_Inputer *
lnk_inputer_init(void)
{
  Arena *arena = arena_alloc();
  LNK_Inputer *inputer = push_array(arena, LNK_Inputer, 1);
  inputer->arena            = arena;
  inputer->objs_ht          = hash_table_init(arena, 0x20000);
  inputer->libs_ht          = hash_table_init(arena, 0x1000);
  inputer->missing_lib_ht   = hash_table_init(arena, 0x100);
  return inputer;
}

internal LNK_Input *
lnk_input_push(Arena *arena, LNK_InputList *list, String8 path, String8 data)
{
  LNK_Input *node = push_array(arena, LNK_Input, 1);
  node->path      = path;
  node->data      = data;
  lnk_input_list_push_node(list, node);
  return node;
}

internal LNK_Input *
lnk_inputer_push_linkgen(Arena *arena, LNK_InputList *list, String8 data, String8 path)
{
  LNK_Input *input = lnk_input_push(arena, list, data, path);
  input->exclude_from_debug_info = 1;
  return input;
}

internal LNK_Input *
lnk_inputer_push_thin(Arena *arena, LNK_InputList *list, HashTable *ht, String8 full_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  LNK_Input *input = hash_table_search_path_raw(ht, full_path);
  if (input == 0) {
    input          = lnk_input_push(arena, list, full_path, str8_zero());
    input->path    = push_str8_copy(arena, full_path);
    input->is_thin = 1;

    hash_table_push_path_raw(arena, ht, full_path, input);
  }
  scratch_end(scratch);
  return input;
}

internal LNK_Input *
lnk_inputer_push_obj(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path, String8 data)
{
  lnk_log(LNK_Log_InputObj, "Input Obj: %S", path);
  LNK_Input *input = lnk_input_push(inputer->arena, &inputer->new_objs, path, data);
  input->link_member = link_member;
  return input;
}

internal LNK_Input *
lnk_inputer_push_obj_linkgen(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path, String8 data)
{
  lnk_log(LNK_Log_InputObj, "Input Obj: %S", path);
  LNK_Input *input = lnk_inputer_push_linkgen(inputer->arena, &inputer->new_objs, path, data);
  input->link_member = link_member;
  return input;
}

internal LNK_Input *
lnk_inputer_push_obj_thin(LNK_Inputer *inputer, LNK_LibMemberRef *link_member, String8 path)
{
  lnk_log(LNK_Log_InputObj, "Input Obj: %S", path);
  Temp scratch = scratch_begin(0,0);
  String8    full_path = os_full_path_from_path(scratch.arena, path);
  LNK_Input *input     = lnk_inputer_push_thin(inputer->arena, &inputer->new_objs, inputer->objs_ht, full_path);
  input->link_member = link_member;
  scratch_end(scratch);
  return input;
}

internal LNK_Input *
lnk_inputer_push_lib(LNK_Inputer *inputer, LNK_InputSourceType input_source, String8 path, String8 data)
{
  lnk_log(LNK_Log_InputLib, "Input Lib: %S", path);
  return lnk_input_push(inputer->arena, &inputer->new_libs[input_source], path, data);
}

internal LNK_Input *
lnk_inputer_push_lib_linkgen(LNK_Inputer *inputer, LNK_InputSourceType input_source, String8 path, String8 data)
{
  lnk_log(LNK_Log_InputLib, "Input Lib: %S", path);
  return lnk_input_push(inputer->arena, &inputer->new_libs[input_source], path, data);
}

internal LNK_Input *
lnk_input_from_path(HashTable *load_ht, String8 path)
{
  LNK_Input *input = hash_table_search_path_raw(load_ht, path);
  if (input == 0) {
    Temp scratch = scratch_begin(0, 0);
    String8 full_path = os_full_path_from_path(scratch.arena, path);
    input = hash_table_search_path_raw(load_ht, full_path);
    scratch_end(scratch);
  }
  return input;
}

internal LNK_Input *
lnk_inputer_push_lib_thin(LNK_Inputer *inputer, LNK_Config *config, LNK_InputSourceType input_source, String8 path)
{
  Temp scratch = scratch_begin(0,0);

  LNK_Input *input = 0;

  // default libraries may omit extension
  if (input_source == LNK_InputSource_Default || input_source == LNK_InputSource_Obj) {
    if (!str8_ends_with(path, str8_lit(".lib"), StringMatchFlag_CaseInsensitive)) {
      path = push_str8f(scratch.arena, "%S.lib", path);
    }
    if (lnk_is_lib_disallowed(config, path)) {
      goto exit;
    }
  }

  // was library already loaded?
  input = hash_table_search_path_raw(inputer->libs_ht, path);
  if (input) {
    goto exit;
  }

  // search disk for library
  String8 first_match = lnk_find_first_file(scratch.arena, config->lib_dir_list, path);

  // warn about missing library
  if (first_match.size == 0) {
    KeyValuePair *was_reported = hash_table_search_path(inputer->missing_lib_ht, path);
    if (was_reported == 0) {
      hash_table_push_path_u64(inputer->arena, inputer->missing_lib_ht, path, 0);
      lnk_error(LNK_Warning_FileNotFound, "unable to find library `%S`", path);
    }
    goto exit;
  }

  // was input with full path already loaded?
  input = hash_table_search_path_raw(inputer->libs_ht, first_match);
  if (input) {
    goto exit;
  }

  lnk_log(LNK_Log_InputLib, "Input Lib: %S", first_match);
  input = lnk_inputer_push_thin(inputer->arena, &inputer->new_libs[input_source], inputer->libs_ht, first_match);

  // store input path to early-out of file searches for default libs
  if (!str8_match(first_match, path, StringMatchFlag_CaseInsensitive)) {
    hash_table_push_path_raw(inputer->arena, inputer->libs_ht, path, input);
  }

  exit:;
  scratch_end(scratch);
  return input;
}

internal B32
lnk_inputer_has_items(LNK_Inputer *inputer)
{
  if (inputer->new_objs.count > 0) {
    return 1;
  }

  for EachIndex(i, ArrayCount(inputer->new_libs)) {
    if (inputer->new_libs[i].count > 0) {
      return 1;
    }
  }

  return 0;
}

internal LNK_InputPtrArray
lnk_inputer_flush(Arena *arena, TP_Context *tp, LNK_Inputer *inputer, LNK_IO_Flags io_flags, LNK_InputList *all_inputs, LNK_InputList *new_inputs)
{
  ProfBeginFunction();

  Temp scratch = scratch_begin(&arena, 1);

  ProfBegin("Gather Thin Inputs");
  U64 thin_inputs_count = 0;
  for (LNK_Input *node = new_inputs->first; node != 0; node = node->next) {
    if (node->is_thin) {
      thin_inputs_count += 1;
    }
  }
  LNK_Input **thin_inputs = push_array(scratch.arena, LNK_Input *, thin_inputs_count);
  U64         thin_idx    = 0;
  for (LNK_Input *node = new_inputs->first; node != 0; node = node->next) {
    if (node->is_thin) {
      thin_inputs[thin_idx++] = node;
    }
  }
  String8Array thin_input_paths = {0};
  thin_input_paths.count = thin_inputs_count;
  thin_input_paths.v     = push_array(scratch.arena, String8, thin_inputs_count);
  for EachIndex(i, thin_inputs_count) {
    thin_input_paths.v[i] = thin_inputs[i]->path;
  }
  ProfEnd();

  ProfBegin("Load Inputs From Disk"); 
  String8Array thin_input_datas  = lnk_read_data_from_file_path_parallel(tp, inputer->arena, io_flags, thin_input_paths);
  for EachIndex(thin_input_idx, thin_inputs_count) {
    thin_inputs[thin_input_idx]->has_disk_read_failed = thin_input_datas.v[thin_input_idx].size == 0;
    thin_inputs[thin_input_idx]->data                 = thin_input_datas.v[thin_input_idx];
  }
  ProfEnd();

  ProfBegin("Disk Read Check");
  for EachIndex(i, thin_inputs_count) {
    if (thin_inputs[i]->has_disk_read_failed) {
      lnk_error(LNK_Error_InvalidPath, "unable to find file \"%S\"", thin_inputs[i]->path);
    }
  }
  ProfEnd();

  LNK_InputPtrArray result = lnk_array_from_input_list(arena, *new_inputs);

  lnk_input_list_concat_in_place(all_inputs, new_inputs);

  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal void
lnk_lib_member_ref_list_push_node(LNK_LibMemberRefList *list, LNK_LibMemberRef *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal void
lnk_lib_member_ref_list_concat_in_place_array(LNK_LibMemberRefList *list, LNK_LibMemberRefList *to_concat_arr, U64 count)
{
  SLLConcatInPlaceArray(list, to_concat_arr, count);
}

internal int
lnk_lib_member_ref_is_before(void *raw_a, void *raw_b)
{
  LNK_LibMemberRef **a = raw_a, **b = raw_b;
  LNK_Symbol *a_pull_in_ref = (*a)->lib->was_member_linked[(*a)->member_idx];
  LNK_Symbol *b_pull_in_ref = (*b)->lib->was_member_linked[(*b)->member_idx];
  return lnk_symbol_is_before(a_pull_in_ref, b_pull_in_ref);
}

internal LNK_LibMemberRef **
lnk_array_from_lib_member_list(Arena *arena, LNK_LibMemberRefList list)
{
  LNK_LibMemberRef **result = push_array(arena, LNK_LibMemberRef *, list.count);
  U64 idx = 0;
  for (LNK_LibMemberRef *node = list.first; node != 0; node = node->next, idx += 1) {
    result[idx] = node;
  }
  return result;
}

internal LNK_ObjNode *
lnk_load_objs(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab, LNK_Link *link, U64 *objs_count_out)
{
  ProfBeginV("Load Objs [Count %llu]", inputer->new_objs.count);
  Temp scratch = scratch_begin(arena->v, arena->count);

  // load obj inputer from disk
  LNK_InputPtrArray new_input_objs = lnk_inputer_flush(arena->v[0], tp, inputer, config->io_flags, &inputer->objs, &inputer->new_objs);

  if (lnk_get_log_status(LNK_Log_InputObj) && new_input_objs.count) {
    U64 input_size = 0;
    for EachIndex(i, new_input_objs.count) { input_size += new_input_objs.v[i]->data.size; }
    lnk_log(LNK_Log_InputObj, "[ Obj Input Size %M ]", input_size);
  }

  LNK_ObjNode *new_objs = lnk_obj_from_input_many(tp, arena, config->machine, new_input_objs.count, new_input_objs.v);

  // if machine type was unspecified on the command line, derive it from obj file
  if (config->machine == COFF_MachineType_Unknown) {
    for EachIndex(obj_idx, new_input_objs.count) {
      if (new_objs[obj_idx].data.header.machine != COFF_MachineType_Unknown) {
        config->machine = new_objs[obj_idx].data.header.machine;
        break;
      }
    }
  }

  ProfBegin("Apply Directives");
  for EachIndex(obj_idx, new_input_objs.count) {
    LNK_Obj           *obj            = &new_objs[obj_idx].data;
    String8List        raw_directives = lnk_raw_directives_from_obj(scratch.arena, obj);
    LNK_DirectiveInfo  directive_info = lnk_directive_info_from_raw_directives(scratch.arena, obj, raw_directives);
    for EachIndex(i, ArrayCount(directive_info.v)) {
      for (LNK_Directive *dir = directive_info.v[i].first; dir != 0; dir = dir->next) {
        lnk_apply_cmd_option_to_config(config, dir->id, dir->value_list, obj);
      }
    }
  }
  ProfEnd();

  if (objs_count_out) {
    *objs_count_out = new_input_objs.count;
  }

  scratch_end(scratch);
  ProfEnd();
  return new_objs;
}

internal void
lnk_load_libs(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_Link *link)
{
  for EachIndex(input_source, LNK_InputSource_Count) {
    ProfBegin("Input Libs [Count %llu]", inputer->new_libs[input_source].count);

    LNK_InputPtrArray new_input_libs = lnk_inputer_flush(arena->v[0], tp, inputer, config->io_flags, &inputer->libs, &inputer->new_libs[input_source]);

    if (lnk_get_log_status(LNK_Log_InputLib) && new_input_libs.count) {
      U64 input_size = 0;
      for EachIndex(i, new_input_libs.count) { input_size += new_input_libs.v[i]->data.size; }
      lnk_log(LNK_Log_InputObj, "[ Lib Input Size %M ]", input_size);
    }

    lnk_lib_list_push_parallel(tp, arena, &link->libs, new_input_libs.count, new_input_libs.v);

    ProfEnd();
  }
}

internal void
lnk_load_inputs(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab, LNK_Link *link)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  U64 obj_id_base = link->objs.count;

  U64          objs_count  = 0;
  LNK_ObjNode *objs        = lnk_load_objs(tp, arena, config, inputer, symtab, link, &objs_count);
  lnk_obj_list_push_node_many(&link->objs, objs_count, objs);
  
  // if delay load DLLs are present, include delay load helper symbol
  if (config->machine != COFF_MachineType_Unknown && config->delay_load_helper_name.size == 0 && config->delay_load_dll_list.node_count) {
    config->delay_load_helper_name = mscrt_delay_load_helper_name_from_machine(config->machine);
    if (config->delay_load_helper_name.size) {
      lnk_include_symbol(config, config->delay_load_helper_name, 0);
    }
  }

  {
    ProfBegin("Process /INCLUDE");

    // group include symbols by obj
    HashTable *ht = hash_table_init(scratch.arena, 64);
    for (; *link->last_include; link->last_include = &(*link->last_include)->next) {
      LNK_IncludeSymbol *include_symbol = &(*link->last_include)->v;

      // skip, include symbol is already in the global symbol table
      if (lnk_symbol_table_search(symtab, include_symbol->name)) {
        continue;
      }

      // was obj already seen?
      String8List *include_name_list = hash_table_search_raw_raw(ht, include_symbol->obj);

      if (include_name_list == 0) {
        // push entry for new obj
        include_name_list = push_array(scratch.arena, String8List, 1);
        hash_table_push_raw_raw(scratch.arena, ht, include_symbol->obj, include_name_list);
      }

      // append include symbol to obj's name list
      str8_list_push(scratch.arena, include_name_list, include_symbol->name);
    }

    LNK_Obj     **objs_with_includes = keys_from_hash_table_raw(scratch.arena, ht);
    String8List **include_names      = values_from_hash_table_raw(scratch.arena, ht);
    for EachIndex(i, ht->count) {
      LNK_Obj *obj_with_includes = objs_with_includes[i];
      String8  include_obj_path  = obj_with_includes ? obj_with_includes->path : str8_lit("RADLINK");
      String8  include_obj_data  = lnk_make_obj_with_undefined_symbols(arena->v[0], *include_names[i]);
      lnk_inputer_push_obj_linkgen(inputer, obj_with_includes ? obj_with_includes->link_member : 0, include_obj_path, include_obj_data);

      U64          include_obj_count = 0;
      LNK_ObjNode *include_obj       = lnk_load_objs(tp, arena, config, inputer, symtab, link, &include_obj_count);
      AssertAlways(include_obj_count == 1);

      if (obj_with_includes) {
        DLLInsert(link->objs.first, link->objs.last, obj_with_includes->node, include_obj);
        link->objs.count += 1;
      } else {
        lnk_obj_list_push_node(&link->objs, include_obj);
      }
    }

    ProfEnd();
  }

  // finalize input indices on new objs and push external symbols to the symbol table
  {
    U64 node_idx = 0;
    for (LNK_ObjNode **n = link->last_symbol_input; *n; n = &(*n)->next, node_idx += 1) {
      (*n)->data.input_idx = obj_id_base + node_idx;
    }

    U64       new_objs_count = node_idx;
    LNK_Obj **new_objs       = push_array(scratch.arena, LNK_Obj *, node_idx);
    node_idx = 0;
    for (; *link->last_symbol_input; link->last_symbol_input = &(*link->last_symbol_input)->next, node_idx += 1) {
      new_objs[node_idx] = &(*link->last_symbol_input)->data;
    }

    lnk_push_obj_symbols(tp, arena, symtab, new_objs_count, new_objs);
  }

  // input default libraries
  for (; *link->last_default_lib; link->last_default_lib = &(*link->last_default_lib)->next) {
    lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Default, (*link->last_default_lib)->string);
  }

  // input libraries referenced in objs
  for (; *link->last_obj_lib; link->last_obj_lib = &(*link->last_obj_lib)->next) {
    lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, (*link->last_obj_lib)->string);
  }

  // load new libs
  lnk_load_libs(tp, arena, config, inputer, link);

  // resolve entry point
  if (link->try_to_resolve_entry_point) {
    B32 is_entry_point_name_inferred = config->entry_point_name.size == 0;

    // loop over all possible subsystems and entry point names and pick
    // subsystem that has a defined entry point symbol
    if (config->entry_point_name.size == 0) {
      PE_WindowsSubsystem  subsys_first       = config->subsystem;
      PE_WindowsSubsystem  subsys_last        = config->subsystem == PE_WindowsSubsystem_UNKNOWN ? PE_WindowsSubsystem_COUNT : config->subsystem+1;
      LNK_Symbol          *entry_point_symbol = 0;
      for (U64 subsys_idx = subsys_first; subsys_idx < subsys_last; subsys_idx += 1) {
        String8Array entry_points = pe_get_entry_point_names(config->machine, (PE_WindowsSubsystem)subsys_idx, config->file_characteristics);
        for EachIndex(i, entry_points.count) {
          LNK_Symbol *symbol = lnk_symbol_table_search(symtab, entry_points.v[i]);
          if (symbol) {
            config->subsystem        = subsys_idx;
            config->entry_point_name = entry_points.v[i];
            goto found_entry_and_subsystem;
          }
        }
      }
      found_entry_and_subsystem:;
    }

    // search for entry point in libs
    if (config->entry_point_name.size == 0 && config->subsystem != PE_WindowsSubsystem_UNKNOWN) {
      String8Array entry_points = pe_get_entry_point_names(config->machine, config->subsystem, config->file_characteristics);
      for EachIndex(entry_idx, entry_points.count) {
        for (LNK_LibNode *lib_n = link->libs.first; lib_n != 0; lib_n = lib_n->next) {
          if (lnk_search_lib(&lib_n->data, entry_points.v[entry_idx], 0)) {
            config->entry_point_name = entry_points.v[entry_idx];
            goto found_entry_in_libs;
          }
        }
      }
      found_entry_in_libs:;
    }

    // infer subsystem from entry point name
    if (config->entry_point_name.size != 0 && config->subsystem == PE_WindowsSubsystem_UNKNOWN) {
      for EachIndex(subsys_idx, PE_WindowsSubsystem_COUNT) {
        String8Array entry_points = pe_get_entry_point_names(config->machine, subsys_idx, config->file_characteristics);
        for EachIndex(i, entry_points.count) {
          if (str8_match(entry_points.v[i], config->entry_point_name, 0)) {
            config->subsystem = subsys_idx;
            goto subsystem_inferred_from_entry;
          }
        }
      }
      subsystem_inferred_from_entry:;
    }

    // do we have an entry point name?
    if (config->entry_point_name.size) {
      if (is_entry_point_name_inferred) {
        // redirect user entry to appropriate CRT entry
        String8 crt_entry_point_name = msvcrt_ctr_entry_from_user_entry(config->entry_point_name);
        config->entry_point_name = crt_entry_point_name.size ? crt_entry_point_name : config->entry_point_name;
      }

      // generate undefined symbol for entry point
      lnk_include_symbol(config, config->entry_point_name, 0);

      // do we have a subsystem?
      if (config->subsystem != PE_WindowsSubsystem_UNKNOWN) {
        // if subsystem version not specified set default values
        if (config->subsystem_ver.major == 0 && config->subsystem_ver.minor == 0) {
          config->subsystem_ver = lnk_get_default_subsystem_version(config->subsystem, config->machine);
        }

        // check subsystem version against allowed min version
        Version min_subsystem_ver = lnk_get_min_subsystem_version(config->subsystem, config->machine);
        if (version_compar(config->subsystem_ver, min_subsystem_ver) < 0) {
          lnk_error(LNK_Error_Cmdl, "subsystem version %I64u.%I64u can't be lower than %I64u.%I64u", 
                    config->subsystem_ver.major, config->subsystem_ver.minor, min_subsystem_ver.major, min_subsystem_ver.minor);
        }

        // by default terminal server is enabled for windows and console applications
        if (~config->flags & LNK_ConfigFlag_NoTsAware && ~config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL) {
          if (config->subsystem == PE_WindowsSubsystem_WINDOWS_GUI || config->subsystem == PE_WindowsSubsystem_WINDOWS_CUI) {
            config->dll_characteristics |= PE_DllCharacteristic_TERMINAL_SERVER_AWARE;
          }
        }

        // entry point found!
        link->try_to_resolve_entry_point = 0;
      } else {
        lnk_error(LNK_Error_NoSubsystem, "unknown subsystem, please use /SUBSYSTEM to set subsytem type you need");
      }
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal force_inline void
lnk_queue_lib_member(Arena *arena, LNK_LibMemberRefList *queued_members, LNK_Symbol *link_symbol, LNK_Lib *lib, U32 member_idx)
{
  B32 is_first_set = lnk_lib_set_link_symbol(lib, member_idx, link_symbol);
  if (is_first_set) {
    link_symbol->is_lib_member_linked = 1;

    LNK_LibMemberRef *member_ref = push_array(arena, LNK_LibMemberRef, 1);
    member_ref->lib              = lib;
    member_ref->member_idx       = member_idx;

    lnk_lib_member_ref_list_push_node(queued_members, member_ref);
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_search_lib_task)
{
  LNK_SearchLibTask    *task             = raw_task;
  LNK_Lib              *lib              = task->lib;
  LNK_SymbolTable      *symtab           = task->symtab;
  B32                   search_anti_deps = task->search_anti_deps;
  LNK_LibMemberRefList *member_ref_list  = &task->member_ref_lists[task_id];

  for EachNode(c, LNK_SymbolHashTrieChunk, symtab->weak_undef_chunks[task_id].first) {
    for EachIndex(i, c->count) {
      LNK_Symbol *symbol = c->v[i].symbol;
      if (symbol->is_lib_member_linked) { continue; }

      LNK_ObjSymbolRef           symbol_ref    = lnk_ref_from_symbol(symbol);
      COFF_ParsedSymbol          symbol_parsed = lnk_parsed_from_symbol(symbol);
      COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
      if (symbol_interp == COFF_SymbolValueInterp_Undefined) {
        U32 member_idx;
        if (lnk_search_lib(lib, symbol->name, &member_idx)) {
          lnk_queue_lib_member(arena, member_ref_list, symbol, lib, member_idx);
        }
      } else if (symbol_interp == COFF_SymbolValueInterp_Weak) {
        COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(symbol_parsed, symbol_ref.obj->header.is_big_obj);
        if (weak_ext->characteristics == COFF_WeakExt_SearchLibrary) {
          U32 member_idx;
          if (lnk_search_lib(lib, symbol->name, &member_idx)) {
            lnk_queue_lib_member(arena, member_ref_list, symbol, lib, member_idx);
          }
        } else if (weak_ext->characteristics == COFF_WeakExt_AntiDependency) {
          if (search_anti_deps) {
            LNK_ObjSymbolRef dep_symbol = {0};
            if (lnk_resolve_weak_symbol(symtab, symbol_ref, &dep_symbol)) {
              COFF_ParsedSymbol          dep_parsed = lnk_parsed_symbol_from_coff_symbol_idx(dep_symbol.obj, dep_symbol.symbol_idx);
              COFF_SymbolValueInterpType dep_interp = coff_interp_from_parsed_symbol(dep_parsed);
              if (dep_interp == COFF_SymbolValueInterp_Weak) {
                U32 member_idx;
                if (lnk_search_lib(lib, symbol_parsed.name, &member_idx)) {
                  lnk_queue_lib_member(arena, member_ref_list, symbol, lib, member_idx);
                }
              }
            }
          }
        }
      }
    }
  }
}

internal void
lnk_link_inputs(TP_Context      *tp,
                TP_Arena         *arena,
                LNK_Config       *config,
                LNK_Inputer      *inputer,
                LNK_SymbolTable  *symtab,
                LNK_Link         *link, 
                LNK_ImportTables *imps)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  B32 search_anti_deps = 0;
  for (U64 resolved_members_count = 0; ; resolved_members_count = 0) {
    lnk_load_inputs(tp, arena, config, inputer, symtab, link);

    for EachNode(lib_n, LNK_LibNode, link->libs.first) {
      ProfBeginV("Search %S", str8_skip_last_slash(lib_n->data.path));
      do {
        lnk_load_inputs(tp, arena, config, inputer, symtab, link);

        LNK_LibMemberRefList queued_members = {0};
        {
          LNK_SearchLibTask task = { .search_anti_deps = search_anti_deps, .lib = &lib_n->data, .symtab = symtab };
          task.member_ref_lists  = push_array(scratch.arena, LNK_LibMemberRefList, tp->worker_count);
          tp_for_parallel(tp, arena, tp->worker_count, lnk_search_lib_task, &task);
          lnk_lib_member_ref_list_concat_in_place_array(&queued_members, task.member_ref_lists, tp->worker_count);
        }

        // sort library member refs to match the order of their appearance in obj symbol tables
        LNK_LibMemberRef **member_refs = lnk_array_from_lib_member_list(scratch.arena, queued_members);
        radsort(member_refs, queued_members.count, lnk_lib_member_ref_is_before);

        if (queued_members.count) {
          lnk_log(LNK_Log_Links, "Searching %S:", lib_n->data.path);

          for EachIndex(i, queued_members.count) {
            Temp temp = temp_begin(scratch.arena);

            LNK_LibMemberRef *member_ref  = member_refs[i];
            LNK_Lib          *lib         = member_ref->lib;
            LNK_Symbol       *link_symbol = lib->was_member_linked[member_ref->member_idx];

            COFF_ArchiveMember member_info = coff_archive_member_from_offset(lib->data, lib->member_offsets[member_ref->member_idx]);
            COFF_DataType      member_type = coff_data_type_from_data(member_info.data);
            String8            member_name = coff_decode_member_name(lib->long_names, member_info.header.name);

            U64                refs_count = 0;
            LNK_ObjSymbolRef **refs       = lnk_ref_from_symbol_many(temp.arena, link_symbol, &refs_count);
            lnk_log(LNK_Log_Links, "\tFound %S in %S", link_symbol->name, str8_skip_last_slash(member_name));
            for EachIndex(i, refs_count) {
              lnk_log(LNK_Log_Links, "\t\tReferenced in %S", lnk_loc_from_obj(temp.arena, refs[i]->obj));
            }

            temp_end(temp);
          }
        }

        // push inputs for lib member refs
        for EachIndex(i, queued_members.count) {
          LNK_LibMemberRef *member_ref  = member_refs[i];
          LNK_Lib          *lib         = member_ref->lib;
          LNK_Symbol       *link_symbol = lib->was_member_linked[member_ref->member_idx];

          // parse member
          COFF_ArchiveMember member_info = coff_archive_member_from_offset(lib->data, lib->member_offsets[member_ref->member_idx]);
          COFF_DataType      member_type = coff_data_type_from_data(member_info.data);
          String8            member_name = coff_decode_member_name(lib->long_names, member_info.header.name);

          switch (member_type) {
          case COFF_DataType_Import: {
            COFF_ParsedArchiveImportHeader import_header = coff_archive_import_from_data(member_info.data);

            // import machine compat check
            if (import_header.machine != config->machine) {
              LNK_ObjSymbolRef ref = lnk_ref_from_symbol(link_symbol);
              lnk_error_obj(LNK_Error_IncompatibleMachine,
                            ref.obj,
                            "symbol %S pulls-in import from %S for an incompatible machine %S (expected machine %S)",
                            link_symbol->name,
                            str8_chop_last_slash(lib->path),
                            coff_string_from_machine_type(import_header.machine),
                            coff_string_from_machine_type(config->machine));
              break;
            }

            // was import already created?
            LNK_Symbol *is_import_loaded = lnk_symbol_table_search(symtab, link_symbol->name);
            if (is_import_loaded) {
              COFF_SymbolValueInterpType interp = lnk_interp_from_symbol(is_import_loaded);
              if (interp != COFF_SymbolValueInterp_Undefined) {
                break;
              }
            }

            // create import stub symbol (later replaced with actual import)
            LNK_Symbol       *import_stub       = lnk_symbol_table_search(symtab, str8_lit(LNK_IMPORT_STUB));
            LNK_ObjSymbolRef  import_symbol_ref = lnk_ref_from_symbol(import_stub);
            LNK_Symbol       *import_symbol     = lnk_make_symbol(symtab->arena->v[0], link_symbol->name, import_symbol_ref.obj, import_symbol_ref.symbol_idx);
            lnk_symbol_table_push(symtab, import_symbol);

            // skip lib search
            import_symbol->is_lib_member_linked = 1;

            // find DLL with import symbols
            B32                is_delay_load  = lnk_is_dll_delay_load(config, import_header.dll_name);
            String8List       *dll_names      = is_delay_load ? &imps->delayed_dll_names : &imps->static_dll_names;
            HashTable         *imports_ht     = is_delay_load ? imps->delayed_imports    : imps->static_imports;
            PE_MakeImportList *import_symbols = hash_table_search_path_raw(imports_ht, import_header.dll_name);

            // create record for a first time-DLL
            if (import_symbols == 0) {
              import_symbols = push_array(imps->arena, PE_MakeImportList, 1);
              str8_list_push(imps->arena, dll_names, import_header.dll_name);
              hash_table_push_path_raw(imps->arena, imports_ht, import_header.dll_name, import_symbols);
            }

            // push make import info
            pe_make_import_header_list_push(imps->arena, import_symbols, (PE_MakeImport){ .header = member_info.data, .make_jump_thunk = !str8_starts_with(link_symbol->name, str8_lit("__imp_")) });
          } break;
          case COFF_DataType_BigObj:
          case COFF_DataType_Obj: {
            if (lib->type == COFF_Archive_Thin) {
              // obj path in thin archive is relative to the directory with archive
              String8List obj_path_list = {0};
              str8_list_push(scratch.arena, &obj_path_list, str8_chop_last_slash(lib->path));
              str8_list_push(scratch.arena, &obj_path_list, member_name);
              String8 obj_path = str8_path_list_join_by_style(inputer->arena, &obj_path_list, config->path_style);

              lnk_inputer_push_obj_thin(inputer, member_ref, obj_path);
            } else {
              lnk_inputer_push_obj(inputer, member_ref, member_name, member_info.data);
            }
          } break;
          case COFF_DataType_Null: break;
          default: { InvalidPath; } break;
          }
        }

        resolved_members_count += queued_members.count;
      } while (lnk_inputer_has_items(inputer));
      ProfEnd();
    }

    if (resolved_members_count == 0) {
      search_anti_deps = 0;

      // replace undefined symbols that have an alternate name with a weak symbol
      for (LNK_AltNameNode *alt_name_n = config->alt_name_list.first; alt_name_n != 0; alt_name_n = alt_name_n->next) {
        LNK_SymbolHashTrie *symbol_ht = lnk_symbol_table_search_(symtab, alt_name_n->v.from);
        if (symbol_ht) {
          COFF_SymbolValueInterpType interp = lnk_interp_from_symbol(symbol_ht->symbol);
          if (interp == COFF_SymbolValueInterp_Undefined) {
            // clear out slot so weak symbol can replace undefined symbol (general rule is
            // weak symbol is not allowed to replace undefined)
            LNK_Symbol *undef_symbol = symbol_ht->symbol;
            symbol_ht->symbol = 0;

            // make obj with alternamte name symbol
            String8 alt_name_obj_data;
            {
              COFF_ObjWriter *obj_writer  = coff_obj_writer_alloc(0, COFF_MachineType_Unknown);
              COFF_ObjSymbol *from_symbol = coff_obj_writer_push_symbol_weak(obj_writer, alt_name_n->v.from, COFF_WeakExt_SearchLibrary, 0);
              COFF_ObjSymbol *to_symbol   = coff_obj_writer_push_symbol_weak(obj_writer, alt_name_n->v.to,   COFF_WeakExt_AntiDependency, from_symbol);
              coff_obj_writer_set_default_symbol(from_symbol, to_symbol);
              alt_name_obj_data = coff_obj_writer_serialize(arena->v[0], obj_writer);
              coff_obj_writer_release(&obj_writer);
            }

            LNK_Obj *obj_with_alt_name      = alt_name_n->v.obj;
            String8  obj_with_alt_name_path = obj_with_alt_name ? obj_with_alt_name->path : str8_lit("RADLINK");
            lnk_inputer_push_obj_linkgen(inputer, obj_with_alt_name ? obj_with_alt_name->link_member : 0, obj_with_alt_name_path, alt_name_obj_data);

            search_anti_deps = 1;
          }
        }
      }

      resolved_members_count = lnk_inputer_has_items(inputer);
    }

    if (resolved_members_count == 0) { break; }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal LNK_Link *
lnk_link_image(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  //
  // init link context
  //
  LNK_Link *link = push_array(arena->v[0], LNK_Link, 1);
  link->last_symbol_input          = &link->objs.first;
  link->last_include               = &config->include_symbol_list.first;
  link->last_default_lib           = &config->input_default_lib_list.first;
  link->last_obj_lib               = &config->input_obj_lib_list.first;
  link->last_cmd_lib               = &config->input_list[LNK_Input_Lib].first;
  link->try_to_resolve_entry_point = 1;

  //
  // init import hash tables
  //
  LNK_ImportTables *imps = 0;
  {
    Arena *imps_arena = arena_alloc();
    imps = push_array(imps_arena, LNK_ImportTables, 1);
    imps->arena           = imps_arena;
    imps->static_imports  = hash_table_init(imps->arena, 0x1000);
    imps->delayed_imports = hash_table_init(imps->arena, 0x1000);
  }

  // input :null_obj
  String8 null_obj = lnk_make_null_obj(inputer->arena);
  lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null *"), null_obj);

  // input objs on command line
  for (String8Node *obj_path = config->input_list[LNK_Input_Obj].first; obj_path != 0; obj_path = obj_path->next) {
    lnk_inputer_push_obj_thin(inputer, 0, obj_path->string);
  }

  // input libs from command line
  for (; *link->last_cmd_lib; link->last_cmd_lib = &(*link->last_cmd_lib)->next) {
    lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_CmdLine, (*link->last_cmd_lib)->string);
  }

  // link inputer
  lnk_link_inputs(tp, arena, config, inputer, symtab, link, imps);

  // TODO: need to figure out under what condition to include load config
  //lnk_include_symbol(config, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME), 0);

  {
    ProfBegin("Push Linker Symbols");
    String8 linker_symbols_obj = lnk_make_linker_obj(arena->v[0], config);
    lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Linker Symbols *"), linker_symbols_obj);
    ProfEnd();
  }

  // make and input delayed imports
  {
    String8List  delayed_dll_names  = imps->delayed_dll_names;
    HashTable   *delayed_imports_ht = imps->delayed_imports;
    AssertAlways(delayed_dll_names.node_count == delayed_imports_ht->count);
    if (delayed_imports_ht->count) {
      ProfBegin("Build Delay Import Table");

      COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;
      B32            emit_biat  = config->import_table_emit_biat == LNK_SwitchState_Yes;
      B32            emit_uiat  = config->import_table_emit_uiat == LNK_SwitchState_Yes;

      for (String8Node *dll_name_n = delayed_dll_names.first; dll_name_n != 0; dll_name_n = dll_name_n->next) {
        PE_MakeImportList *imports              = hash_table_search_path_raw(delayed_imports_ht, dll_name_n->string);
        String8                  import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, dll_name_n->string);
        String8                  import_obj           = pe_make_import_dll_obj_delayed(arena->v[0], time_stamp, config->machine, dll_name_n->string, config->delay_load_helper_name, import_debug_symbols, *imports, emit_biat, emit_uiat);
        lnk_inputer_push_obj(inputer, 0, dll_name_n->string, import_obj);
      }

      String8 linker_debug_symbols = lnk_make_linker_debug_symbols(arena->v[0], config->machine);
      String8 null_desc_obj        = pe_make_null_import_descriptor_delayed(arena->v[0], time_stamp, config->machine, linker_debug_symbols);
      String8 null_thunk_obj       = pe_make_null_thunk_data_obj_delayed(arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols);
      lnk_inputer_push_obj(inputer, 0, str8_lit("* Delayed Null Import Descriptor *"), null_desc_obj);
      lnk_inputer_push_obj(inputer, 0, str8_lit("* Delayed Null Thunk Data *"),        null_thunk_obj);

      ProfEnd();
    }
  }

  // make and input static imports
  {
    String8List  static_dll_names  = imps->static_dll_names;
    HashTable   *static_imports_ht = imps->static_imports;
    AssertAlways(static_dll_names.node_count == static_imports_ht->count);
    if (static_imports_ht->count) {
      ProfBegin("Build Static Import Table");

      COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;

      for (String8Node *dll_name_n = static_dll_names.first; dll_name_n != 0; dll_name_n = dll_name_n->next) {
        PE_MakeImportList *imports              = hash_table_search_path_raw(static_imports_ht, dll_name_n->string);
        String8                  import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, dll_name_n->string);
        String8                  import_obj           = pe_make_import_dll_obj_static(arena->v[0], time_stamp, config->machine, dll_name_n->string, import_debug_symbols, *imports);
        lnk_inputer_push_obj(inputer, 0, dll_name_n->string, import_obj);
      }

      String8 linker_debug_symbols = lnk_make_linker_debug_symbols(scratch.arena, config->machine);
      String8 null_desc_obj        = pe_make_null_import_descriptor_obj(arena->v[0], time_stamp, config->machine, linker_debug_symbols);
      String8 null_thunk_obj       = pe_make_null_thunk_data_obj(arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null Import Descriptor *"), null_desc_obj);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null Thunk Data *"),        null_thunk_obj);

      ProfEnd();
    }
  }

  if (config->export_symbol_list.count) {
    ProfBegin("Build Export Table");

    PE_ExportParseList resolved_exports = {0};
    for (PE_ExportParseNode *exp_n = config->export_symbol_list.first, *exp_n_next; exp_n != 0; exp_n = exp_n_next) {
      exp_n_next = exp_n->next;
      PE_ExportParse *exp = &exp_n->data;

      if (str8_match(exp->name, config->entry_point_name, 0)) {
        lnk_error_with_loc(LNK_Warning_TryingToExportEntryPoint, exp->obj_path, exp->lib_path, "exported entry point \"%S\"", exp->name);
      }
      if (str8_match(exp->alias, config->entry_point_name, 0)) {
        lnk_error_with_loc(LNK_Warning_TryingToExportEntryPoint, exp->obj_path, exp->lib_path, "alias exports entry point \"%S=%S\"", exp->name, exp->alias);
        continue;
      }

      if (!exp->is_forwarder) {
        // filter out unresolved exports
        LNK_Symbol *symbol = lnk_symbol_table_search(symtab, exp_n->data.name);
        if (symbol == 0) {
          lnk_error_with_loc(LNK_Warning_IllExport, exp->obj_path, exp->lib_path, "unresolved export symbol %S\n", exp->name);
          continue;
        }
      }

      // push resolved export
      pe_export_parse_list_push_node(&resolved_exports, exp_n);
    }

    PE_FinalizedExports finalized_exports = pe_finalize_export_list(scratch.arena, resolved_exports);
    String8             edata_obj         = pe_make_edata_obj(arena->v[0], str8_skip_last_slash(config->image_name), COFF_TimeStamp_Max, config->machine, finalized_exports);
    lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Exports *"), edata_obj);

    ProfEnd();
  }

  {
    String8List res_data_list = {0};
    String8List res_path_list = {0};
    
    // do we have manifest deps passed through pragma alone?
    LNK_ManifestOpt manifest_opt = config->manifest_opt;
    if (config->manifest_dependency_list.node_count > 0 && manifest_opt == LNK_ManifestOpt_Null) {
      manifest_opt = LNK_ManifestOpt_Embed;
    }

    switch (manifest_opt) {
    case LNK_ManifestOpt_Embed: {
      ProfBegin("Embed Manifest");
      // TODO: currently we convert manifest to res and parse res again, this unnecessary instead push manifest 
      // resource to the tree directly
      String8 manifest_data = lnk_manifest_from_inputs(scratch.arena, config->io_flags, config->mt_path, config->manifest_name, config->manifest_uac, config->manifest_level, config->manifest_ui_access, config->input_list[LNK_Input_Manifest], config->manifest_dependency_list);
      String8 manifest_res  = pe_make_manifest_resource(scratch.arena, *config->manifest_resource_id, manifest_data);
      str8_list_push(scratch.arena, &res_data_list, manifest_res);
      str8_list_push(scratch.arena, &res_path_list, str8_lit("* Manifest *"));
      ProfEnd();
    } break;
    case LNK_ManifestOpt_WriteToFile: {
      ProfBeginDynamic("Write Manifest To: %.*s", str8_varg(config->manifest_name));
      Temp temp = temp_begin(scratch.arena);
      String8 manifest_data = lnk_manifest_from_inputs(temp.arena, config->io_flags, config->mt_path, config->manifest_name, config->manifest_uac, config->manifest_level, config->manifest_ui_access, config->input_list[LNK_Input_Manifest], config->manifest_dependency_list);
      lnk_write_data_to_file_path(config->manifest_name, str8_zero(), manifest_data);
      temp_end(temp);
      ProfEnd();
    } break;
    case LNK_ManifestOpt_Null: {
      Assert(config->input_list[LNK_Input_Manifest].node_count == 0);
      Assert(config->manifest_dependency_list.node_count == 0);
    } break;
    case LNK_ManifestOpt_No: {
      // omit manifest generation
    } break;
    }
    
    ProfBegin("Load .res files from disk");
    for (String8Node *node = config->input_list[LNK_Input_Res].first; node != 0; node = node->next) {
      String8 res_data = lnk_read_data_from_file_path(scratch.arena, config->io_flags, node->string);
      if (res_data.size > 0) {
        if (pe_is_res(res_data)) {
          str8_list_push(scratch.arena, &res_data_list, res_data);
          String8 stable_res_path = lnk_make_full_path(scratch.arena, config->path_style, config->work_dir, node->string);
          str8_list_push(scratch.arena, &res_path_list, stable_res_path);
        } else {
          lnk_error(LNK_Error_LoadRes, "file is not of RES format: %S", node->string);
        }
      } else {
        lnk_error(LNK_Error_LoadRes, "unable to open res file: %S", node->string);
      }
    }
    ProfEnd();
    
    if (res_data_list.node_count > 0) {
      ProfBegin("Build * Resources *");
      String8 obj_name = str8_lit("* Resources *");
      String8 obj_data = lnk_make_res_obj(arena->v[0], res_data_list, res_path_list, config->machine, config->time_stamp, config->work_dir, config->path_style, obj_name);
      lnk_inputer_push_obj_linkgen(inputer, 0, obj_name, obj_data);
      ProfEnd();
    }
  }

  if (lnk_do_debug_info(config)) {
    {
      ProfBegin("Build * Linker * Obj");
      String8 obj_name     = str8_lit("* Linker *");
      String8 raw_cmd_line = str8_list_join(scratch.arena, &config->raw_cmd_line, &(StringJoin){ str8_lit_comp(""),  str8_lit_comp(" "), str8_lit_comp("") });
      String8 obj_data     = lnk_make_linker_coff_obj(arena->v[0], config->time_stamp, config->machine, config->work_dir, config->image_name, config->pdb_name, raw_cmd_line, obj_name);
      lnk_inputer_push_obj_linkgen(inputer, 0, obj_name, obj_data);
      ProfEnd();
    }

    ProfBegin("Build * Debug Directories *");
    if (config->debug_mode != LNK_DebugMode_None && config->debug_mode != LNK_DebugMode_Null) {
      String8 pdb_dir_obj = pe_make_debug_directory_pdb_obj(arena->v[0], config->machine, config->guid, config->age, config->time_stamp, config->pdb_alt_path);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Debug Directory PDB *"), pdb_dir_obj);
    }
    if (config->rad_debug == LNK_SwitchState_Yes) {
      String8 rdi_dir_obj = pe_make_debug_directory_rdi_obj(arena->v[0], config->machine, config->guid, config->age, config->time_stamp, config->rad_debug_alt_path);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Debug Directory RDI *"), rdi_dir_obj);
    }
    ProfEnd();
  }

  //
  // link linker made objs
  //
  lnk_link_inputs(tp, arena, config, inputer, symtab, link, imps);

  //
  // finalize symbol table
  //
  lnk_replace_weak_with_default_symbols(tp, symtab);

  //
  // was entry point resolved?
  //
  if (config->entry_point_name.size == 0 || link->try_to_resolve_entry_point) {
    lnk_error(LNK_Error_EntryPoint, "unable to find entry point symbol");
  }

  //
  // report unresolved symbols
  //
  {
    ProfBegin("Report Unresolved Symbols");

    U64          unresolved_symbols_count = 0;
    LNK_Symbol **unresolved_symbols       = 0;
    {
      U64                       chunks_count = 0;
      LNK_SymbolHashTrieChunk **chunks       = lnk_array_from_symbol_hash_trie_chunk_list(scratch.arena, symtab->chunks, symtab->arena->count, &chunks_count);

      for EachIndex(chunk_idx, chunks_count) {
        LNK_SymbolHashTrieChunk *chunk = chunks[chunk_idx];
        for EachIndex(i, chunk->count) {
          LNK_Symbol                 *symbol        = chunk->v[i].symbol;
          COFF_SymbolValueInterpType  symbol_interp = lnk_interp_from_symbol(symbol);
          if (symbol_interp == COFF_SymbolValueInterp_Undefined) {
            unresolved_symbols_count += 1;
          }
        }
      }

      unresolved_symbols = push_array(scratch.arena, LNK_Symbol *, unresolved_symbols_count);
      if (unresolved_symbols_count) {
        U64 cursor = 0;
        for EachIndex(chunk_idx, chunks_count) {
          LNK_SymbolHashTrieChunk *chunk = chunks[chunk_idx];
          for EachIndex(i, chunk->count) {
            LNK_Symbol *symbol = chunk->v[i].symbol;
            if (lnk_interp_from_symbol(symbol) == COFF_SymbolValueInterp_Undefined) {
              unresolved_symbols[cursor++] = chunk->v[i].symbol;
            }
          }
        }
      }

      radsort(unresolved_symbols, unresolved_symbols_count, lnk_symbol_ptr_is_before);
    }

    for EachIndex(i, unresolved_symbols_count) {
      LNK_Symbol *symbol = unresolved_symbols[i];

      if (i > config->unresolved_symbol_limit) {
        lnk_error(LNK_Error_UnresolvedSymbol, "too many unresolved symbol errors, stopping now");
        break;
      }

      String8List supp_info = {0};
      {
        U64                refs_count = 0;
        LNK_ObjSymbolRef **refs       = lnk_ref_from_symbol_many(scratch.arena, symbol, &refs_count);
        for EachIndex(ref_idx, refs_count) {
          LNK_ObjSymbolRef   *ref           = refs[ref_idx];
          LNK_Obj            *obj           = ref->obj;
          COFF_SectionHeader *section_table = lnk_coff_section_table_from_obj(obj);
          String8             string_table  = lnk_coff_string_table_from_obj(obj);

          CV_DebugS           debug_s         = {0};
          CV_LinesAccel      *debug_lines     = 0;
          String8             debug_checksums = {0};
          String8             debug_strings   = {0};

          for EachIndex(sect_idx, obj->header.section_count_no_null) {
            COFF_SectionHeader *section_header = &section_table[sect_idx];
            if (section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) { continue; }

            String8             section_name   = coff_name_from_section_header(string_table, section_header);
            U64                 section_number = sect_idx+1;
            COFF_RelocArray     relocs         = lnk_coff_relocs_from_section_header(obj, section_header);
            for EachIndex(reloc_idx, relocs.count) {
              if (supp_info.node_count > config->unresolved_symbol_ref_limit) {
                str8_list_pushf(scratch.arena, &supp_info, "too many unresolved symbol references reported, stopping now");
                goto next_undefined_symbol;
              }
              COFF_Reloc *reloc = &relocs.v[reloc_idx];
              if (reloc->isymbol == ref->symbol_idx) {
                U64      line_matches_count = 0;
                CV_Line *line_matches       = 0;
                if (config->map_lines_for_unresolved_symbols == LNK_SwitchState_Yes) {
                  if (debug_lines == 0) {
                    debug_s = lnk_debug_s_from_obj(scratch.arena, obj);
                    String8List raw_checksums = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
                    String8List raw_strings   = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_StringTable);
                    debug_lines     = cv_lines_accel_from_debug_s(scratch.arena, debug_s);
                    debug_checksums = str8_list_first(&raw_checksums);
                    debug_strings   = str8_list_first(&raw_strings);
                  }
                  line_matches_count = 0;
                  line_matches      = cv_line_from_voff(debug_lines, reloc->apply_off, &line_matches_count);
                }

                if (line_matches) {
                  for EachIndex(i, line_matches_count) {
                    CV_Line        line      = line_matches[i];
                    CV_C13Checksum checksum  = {0};
                    String8        file_name = {0};
                    str8_deserial_read_struct(debug_checksums, line.file_off, &checksum);
                    str8_deserial_read_cstr(debug_strings, checksum.name_off, &file_name);
                    str8_list_pushf(scratch.arena, &supp_info, "%S: %S:%u", lnk_loc_from_obj(scratch.arena, obj), file_name, line.line_num);
                  }
                } else {
                  str8_list_pushf(scratch.arena, &supp_info, "%S: %S(%llx)+%x", lnk_loc_from_obj(scratch.arena, obj), section_name, section_number, reloc->apply_off);
                }
              }
            }
          }
        }
        next_undefined_symbol:;
      }

      lnk_error(LNK_Error_UnresolvedSymbol, "unresolved symbol %S", symbol->name);
      lnk_supplement_error_list(supp_info);
    }

    // TODO: /FORCE
    if (unresolved_symbols_count) {
      lnk_exit(LNK_Error_UnresolvedSymbol);
    }

    ProfEnd();
  }

  //
  // warn about unused delayloads
  //
  if (config->flags & LNK_ConfigFlag_CheckUnusedDelayLoadDll) {
    for (String8Node *dll_name_n = config->delay_load_dll_list.first; dll_name_n != 0; dll_name_n = dll_name_n->next) {
      if (!hash_table_search_path_raw(imps->delayed_imports, dll_name_n->string)) {
        lnk_error(LNK_Warning_UnusedDelayLoadDll, "/DELAYLOAD: %S found no imports", dll_name_n->string);
      }
    }
  }

  //
  // discard COMDAT sections that are not referenced
  //
  if (config->opt_ref == LNK_SwitchState_Yes) {
    lnk_opt_ref(tp, symtab, config, link->objs);
  }

  //
  // infer minimal padding size for functions from the target machine
  //
  if (config->machine != COFF_MachineType_Unknown && config->infer_function_pad_min) {
    config->function_pad_min = lnk_get_default_function_pad_min(config->machine);
    config->infer_function_pad_min = 0;
  }

  //
  // log
  //
  if (lnk_get_log_status(LNK_Log_InputObj)) {
    U64 total_input_size = 0;
    for (LNK_ObjNode *obj_n = link->objs.first; obj_n != 0; obj_n = obj_n->next) { total_input_size += obj_n->data.data.size; }
    lnk_log(LNK_Log_InputObj, "[Total Obj Input Size %M]", total_input_size);
  }
  if (lnk_get_log_status(LNK_Log_InputLib)) {
    U64 total_input_size = 0;
    for (LNK_LibNode *lib_n = link->libs.first; lib_n != 0; lib_n = lib_n->next) { total_input_size += lib_n->data.data.size; }
    lnk_log(LNK_Log_InputLib, "[Total Lib Input Size %M]", total_input_size);
  }

  scratch_end(scratch);
  ProfEnd();
  return link;
}

internal void
lnk_opt_ref(TP_Context *tp, LNK_SymbolTable *symtab, LNK_Config *config, LNK_ObjList objs)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  struct Task { struct Task *next; LNK_Obj *obj; COFF_RelocArray relocs; U32 section_number; };
  struct Task *task_stack      = 0;
  const  U64   RELOCS_PER_TASK = 1024;

  //
  // reset live flag on sections
  //
  for (LNK_ObjNode *obj_n = objs.first; obj_n != 0; obj_n = obj_n->next) {
    for EachIndex(sect_idx, obj_n->data.header.section_count_no_null) {
      COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(&obj_n->data, sect_idx+1);
      section_header->flags &= ~LNK_SECTION_FLAG_IS_LIVE;
    }
  }

  //
  // define roots
  //
  {
    // tls
    LNK_Symbol *tls_symbol = lnk_symbol_table_searchf(symtab, MSCRT_TLS_SYMBOL_NAME);
    if (tls_symbol) {
      lnk_include_symbol(config, str8_lit(MSCRT_TLS_SYMBOL_NAME), 0);
    }

    // push tasks for each root symbol
    for (LNK_IncludeSymbolNode *root_n = config->include_symbol_list.first; root_n != 0; root_n = root_n->next) {
      LNK_Symbol       *root     = lnk_symbol_table_search(symtab, root_n->v.name);
      LNK_ObjSymbolRef  root_ref = lnk_ref_from_symbol(root);

      struct Task *t         = push_array(scratch.arena, struct Task, 1);
      t->obj                 = root_ref.obj;
      t->relocs.count        = 1;
      t->relocs.v            = push_array(scratch.arena, COFF_Reloc, 1);
      t->relocs.v[0].isymbol = root_ref.symbol_idx;

      SLLStackPush(task_stack, t);
    }

    // push task for every non-COMDAT section
    for (LNK_ObjNode *obj_n = objs.first; obj_n != 0; obj_n = obj_n->next) {
      LNK_Obj *obj = &obj_n->data;
      for EachIndex(sect_idx, obj->header.section_count_no_null) {
        U32                 section_number = sect_idx+1;
        COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);

        // is section eligible for walking?
        if (section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) { continue; }
        if (section_header->flags & COFF_SectionFlag_LnkRemove)  { continue; }
        if (section_header->flags & COFF_SectionFlag_LnkCOMDAT)  { continue; }

        // divide relocs and push task for each reloc block
        COFF_RelocArray relocs         = lnk_coff_reloc_info_from_section_number(obj, section_number);
        U64             new_task_count = CeilIntegerDiv(relocs.count, RELOCS_PER_TASK);
        struct Task    *new_tasks      = push_array(scratch.arena, struct Task, new_task_count);
        for EachIndex(new_task_idx, new_task_count) {
          struct Task *t  = new_tasks + new_task_idx;
          t->obj          = obj;
          t->relocs.count = Min(RELOCS_PER_TASK, relocs.count - (new_task_idx * RELOCS_PER_TASK));
          t->relocs.v     = relocs.v + (new_task_idx * RELOCS_PER_TASK);
          t->section_number = section_number;
          SLLStackPush(task_stack, t);
        }
      }
    }
  }

  //
  // walk relocations and mark referenced sections with live flag
  //
  for (; task_stack; ) {
    struct Task *t = task_stack; SLLStackPop(task_stack);
    for EachIndex(reloc_idx, t->relocs.count) {
      COFF_Reloc                 *reloc        = &t->relocs.v[reloc_idx];
      LNK_ObjSymbolRef           reloc_defn   = (LNK_ObjSymbolRef){ .obj = t->obj, .symbol_idx = reloc->isymbol };
      COFF_ParsedSymbol           reloc_parsed = lnk_parsed_symbol_from_coff_symbol_idx(reloc_defn.obj, reloc_defn.symbol_idx);
      COFF_SymbolValueInterpType  reloc_interp = coff_interp_from_parsed_symbol(reloc_parsed);

      LNK_ObjSymbolRef ref_symbol = reloc_defn;
      for (;;) {
        COFF_ParsedSymbol          ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx(ref_symbol.obj, ref_symbol.symbol_idx);
        COFF_SymbolValueInterpType ref_interp = coff_interp_from_parsed_symbol(ref_parsed);

        LNK_ObjSymbolRef next_ref = {0};
        if (ref_interp == COFF_SymbolValueInterp_Regular) {
          LNK_Symbol *symlink = lnk_obj_get_comdat_symlink(ref_symbol.obj, ref_parsed.section_number);
          if (symlink) {
            ref_symbol = lnk_ref_from_symbol(symlink);
          }
          break;
        } else if (ref_interp == COFF_SymbolValueInterp_Undefined) {
          if (reloc_parsed.storage_class == COFF_SymStorageClass_External) {
            LNK_Symbol *defn = lnk_symbol_table_search(symtab, ref_parsed.name);
            next_ref = lnk_ref_from_symbol(defn);
          } else {
            MemoryZeroStruct(&ref_symbol);
            break;
          }
        } else if (ref_interp == COFF_SymbolValueInterp_Weak) {
          LNK_Symbol *defn = lnk_symbol_table_search(symtab, ref_parsed.name);
          next_ref = lnk_ref_from_symbol(defn);
        } else {
          break;
        }

        if (MemoryMatchStruct(&next_ref, &ref_symbol)) {
          MemoryZeroStruct(&ref_symbol);
          break;
        }
        ref_symbol = next_ref;
      }

      // skip unresolved symbol
      if (ref_symbol.obj == 0) { continue; }

      COFF_ParsedSymbol           ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx(ref_symbol.obj, ref_symbol.symbol_idx);
      COFF_SymbolValueInterpType  ref_interp = coff_interp_from_parsed_symbol(ref_parsed);
      LNK_Obj                    *ref_obj    = ref_symbol.obj;

      if (ref_interp == COFF_SymbolValueInterp_Regular) {
        // make section number list (reloc section + associates)
        U32Node *section_number_list = push_array(scratch.arena, U32Node, 1);
        section_number_list->data    = ref_parsed.section_number;
        section_number_list->next    = ref_obj->associated_sections[ref_parsed.section_number];

        // push section headers relocations to the task stack
        for (U32Node *section_number_n = section_number_list; section_number_n != 0; section_number_n = section_number_n->next) {
          U32                 section_number = section_number_n->data;
          COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(ref_obj, section_number);

          // is section eligible for walking?
          if (section_header->flags & COFF_SectionFlag_LnkRemove)  { continue; }
          if (section_header->flags & LNK_SECTION_FLAG_IS_LIVE)    { continue; }
          if (section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) { continue; }

          // mark section
          section_header->flags |= LNK_SECTION_FLAG_IS_LIVE;

          // divide relocs and push task for each reloc block
          COFF_RelocArray relocs = lnk_coff_reloc_info_from_section_number(ref_obj, section_number);
          U64          new_task_count = CeilIntegerDiv(relocs.count, RELOCS_PER_TASK);
          struct Task *new_tasks      = push_array(scratch.arena, struct Task, new_task_count);
          for EachIndex(new_task_idx, new_task_count) {
            struct Task *n  = new_tasks + new_task_idx;
            n->obj          = ref_obj;
            n->relocs.count = Min(RELOCS_PER_TASK, relocs.count - (new_task_idx * RELOCS_PER_TASK));
            n->relocs.v     = relocs.v + (new_task_idx * RELOCS_PER_TASK);
            n->section_number = section_number;
            SLLStackPush(task_stack, n);
          }
        }
      }
    }
  }

  ProfBegin("Remove Unreachable Sections");
  for (LNK_ObjNode *obj_n = objs.first; obj_n != 0; obj_n = obj_n->next) {
    LNK_Obj *obj = &obj_n->data;

    for EachIndex(sect_idx, obj->header.section_count_no_null) {
      U32                 section_number = sect_idx+1;
      COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);

      if (section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) { continue; }

      // remove unreferenced sections
      if (~section_header->flags & LNK_SECTION_FLAG_IS_LIVE && section_header->flags & COFF_SectionFlag_LnkCOMDAT) {
        section_header->flags |= COFF_SectionFlag_LnkRemove;
      }

      // remove associated sections
      if (section_header->flags & COFF_SectionFlag_LnkRemove) {
        for (U32Node *section_number_n = obj->associated_sections[section_number]; section_number_n != 0; section_number_n = section_number_n->next) {
          U32                 section_number = section_number_n->data;
          COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);
          section_header->flags |= COFF_SectionFlag_LnkRemove;
        }
      }

      // TODO: Reset reserved flag so it does not get propagated to the image sections.
      // We need to mask out reserved flags when gathering section definitions to actually
      // prevent propagation.
      section_header->flags &= ~LNK_SECTION_FLAG_IS_LIVE;
    }
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
}

internal B32
lnk_resolve_symbol(LNK_SymbolTable *symtab, LNK_ObjSymbolRef symbol, LNK_ObjSymbolRef *symbol_out)
{
  B32                        is_resolved   = 1;
  COFF_ParsedSymbol          symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx(symbol.obj, symbol.symbol_idx);
  COFF_SymbolValueInterpType symbol_interp = coff_interp_symbol(symbol_parsed.section_number, symbol_parsed.value, symbol_parsed.storage_class);
  switch (symbol_interp) {
  case COFF_SymbolValueInterp_Regular: { 
    LNK_Symbol *symlink = lnk_obj_get_comdat_symlink(symbol.obj, symbol_parsed.section_number);
    *symbol_out = symlink ? lnk_ref_from_symbol(symlink) : symbol;
  } break;
  case COFF_SymbolValueInterp_Weak: {
    LNK_Symbol                 *defn        = lnk_symbol_table_search(symtab, symbol_parsed.name);
    COFF_ParsedSymbol           defn_parsed = lnk_parsed_from_symbol(defn);
    COFF_SymbolValueInterpType  defn_interp = lnk_interp_from_symbol(defn);
    if (defn_interp != COFF_SymbolValueInterp_Undefined) {
      *symbol_out = lnk_ref_from_symbol(defn);
    } else {
      is_resolved = 0;
    }
  } break;
  case COFF_SymbolValueInterp_Undefined: {
    LNK_Symbol *defn = lnk_symbol_table_search(symtab, symbol_parsed.name);
    if (defn) {
      *symbol_out = lnk_ref_from_symbol(defn);
    } else {
      is_resolved = 0;
    }
  } break;
  case COFF_SymbolValueInterp_Common: {
    LNK_Symbol *defn = lnk_symbol_table_search(symtab, symbol_parsed.name);
    *symbol_out = lnk_ref_from_symbol(defn);
  } break;
  case COFF_SymbolValueInterp_Abs: {
    if (symbol_parsed.storage_class == COFF_SymStorageClass_External) { 
      LNK_Symbol *defn = lnk_symbol_table_search(symtab, symbol_parsed.name);
      *symbol_out = lnk_ref_from_symbol(defn);
    } else {
      *symbol_out = symbol;
    }
  } break;
  case COFF_SymbolValueInterp_Debug: { *symbol_out = symbol; } break;
  }
  return is_resolved;
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_section_definitions_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;

  HashTable          *sect_defn_ht  = task->u.gather_sects.defns[worker_id];
  LNK_Obj            *obj           = task->objs[obj_idx];
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);

  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];

    if (~sect_header->flags & COFF_SectionFlag_LnkRemove && ~sect_header->flags & COFF_SectionFlag_LnkInfo && sect_header->fsize > 0) {
      Temp temp = temp_begin(scratch.arena);

      // was section defined?
      String8                sect_name            = coff_name_from_section_header(string_table, sect_header);
      String8                sect_name_with_flags = lnk_make_name_with_flags(temp.arena, sect_name, sect_header->flags & ~COFF_SectionFlags_LnkFlags);
      LNK_SectionDefinition *sect_defn            = hash_table_search_string_raw(sect_defn_ht, sect_name_with_flags);

      // push new section definition
      if (sect_defn == 0) {
        sect_defn = push_array(arena, LNK_SectionDefinition, 1);
        sect_defn->name         = sect_name;
        sect_defn->obj          = obj;
        sect_defn->obj_sect_idx = sect_idx;
        sect_defn->flags        = sect_header->flags & ~COFF_SectionFlags_LnkFlags;

        sect_name_with_flags = push_str8_copy(arena, sect_name_with_flags);
        hash_table_push_string_raw(arena, sect_defn_ht, sect_name_with_flags, sect_defn);
      }

      // acc contrib count
      sect_defn->contribs_count += 1;
      
      temp_end(temp);
    }
  }

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_section_contribs_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;

  LNK_Obj            *obj           = task->objs[obj_idx];
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);

  ProfBeginV("Gather Section Contribs [%S]", obj->path);
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    LNK_SectionContrib *sc          = task->null_sc;
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    if (~sect_header->flags & COFF_SectionFlag_LnkRemove && ~sect_header->flags & COFF_SectionFlag_LnkInfo && sect_header->fsize > 0) {
      LNK_SectionContribChunk *sc_chunk = 0;
      {
        Temp temp = temp_begin(scratch.arena);
        String8 sect_name            = coff_name_from_section_header(string_table, sect_header);
        String8 sect_name_with_flags = lnk_make_name_with_flags(temp.arena, sect_name, sect_header->flags & ~COFF_SectionFlags_LnkFlags);
        sc_chunk = hash_table_search_string_raw(task->contribs_ht, sect_name_with_flags);
        temp_end(temp);
      }

      if (sc_chunk) {
        String8 data;
        if (sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
          data = str8(0, sect_header->fsize);
        } else {
          data = str8_substr(obj->data, rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize));
        }

        U16 sc_align = coff_align_size_from_section_flags(sect_header->flags);
        sc = lnk_section_contrib_chunk_push_atomic(sc_chunk, 1);
        sc->first_data_node.next   = 0;
        sc->first_data_node.string = data;
        sc->last_data_node         = &sc->first_data_node;
        sc->align                  = sc_align == 0 ? task->default_align : sc_align;
        sc->u.obj_idx              = obj_idx;
        sc->u.obj_sect_idx         = sect_idx;
      }
    }
    task->sect_map[obj_idx][sect_idx] = sc;
  }
  ProfEnd();

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_set_comdat_leaders_contribs_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Set COMDAT Section Contribs [%S]", obj->path);
  for EachIndex(sect_idx, obj->header.section_count_no_null) {
    U64 section_number = sect_idx+1;

    COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);
    if (~section_header->flags & COFF_SectionFlag_LnkCOMDAT) { continue; }

    LNK_Symbol *symlink = lnk_obj_get_comdat_symlink(obj, section_number);
    if (symlink == 0) { continue; }

    COFF_ParsedSymbol symlink_parsed = lnk_parsed_from_symbol(symlink);
    LNK_ObjSymbolRef  symlink_ref    = lnk_ref_from_symbol(symlink);
    task->sect_map[obj_idx][sect_idx] = task->sect_map[symlink_ref.obj->input_idx][symlink_parsed.section_number - 1];
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_flag_debug_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular) {
      COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);
      if (section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) {
        task->u.patch_symtabs.was_symbol_patched[obj_idx][symbol_idx] = 1;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_comdat_leaders_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("%S", obj->path);

  ProfBegin("Patch COMDAT Offsets");
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular) {
      LNK_Symbol *symlink = lnk_obj_get_comdat_symlink(obj, symbol.section_number);
      if (symlink) {
        LNK_ObjSymbolRef symlink_ref = lnk_ref_from_symbol(symlink);
        if (symlink_ref.obj != obj) {
          U32 section_number;
          U32 value;
          if (symbol.storage_class == COFF_SymStorageClass_External) {
            // COMDAT leader may be at a different offset, so update this symbol with leader's offset
            COFF_ParsedSymbol parsed_symlink = lnk_parsed_from_symbol(symlink);
            section_number = symbol.section_number;
            value          = parsed_symlink.value;
          } else {
            // COMDAT section may have static symbols which are now invalid to relocate against
            section_number = lnk_obj_get_removed_section_number(obj);
            value          = max_U32;
            task->u.patch_symtabs.was_symbol_patched[obj_idx][symbol_idx] = 1;
          }

          if (obj->header.is_big_obj) {
            COFF_Symbol32 *symbol32  = symbol.raw_symbol;
            symbol32->section_number = section_number;
            symbol32->value          = value;
          } else {
            COFF_Symbol16 *symbol16  = symbol.raw_symbol;
            symbol16->section_number = (U16)section_number;
            symbol16->value          = value;
          }
        }
      }
    }
  }
  ProfEnd();

  ProfEnd();

  scratch_end(scratch);
}

internal int
lnk_section_contrib_ptr_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionContrib **a = raw_a, **b = raw_b;
  U64 input_idx_a = Compose64Bit((*a)->u.obj_idx, (*a)->u.obj_sect_idx);
  U64 input_idx_b = Compose64Bit((*b)->u.obj_idx, (*b)->u.obj_sect_idx);
  return u64_compar_is_before(&input_idx_a, &input_idx_b);
}

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_task)
{
  LNK_BuildImageTask *task = raw_task;
  LNK_SectionContribChunk *chunk = task->u.sort_contribs.chunks[task_id];
  ProfBeginV("[%llu]", chunk->count);
  radsort(chunk->v, chunk->count, lnk_section_contrib_ptr_is_before);
  ProfEnd();
}

internal int
lnk_common_block_contrib_is_before(void *raw_a, void *raw_b)
{
  LNK_CommonBlockContrib *a = raw_a;
  LNK_CommonBlockContrib *b = raw_b;

  int is_before;
  if (a->u.size == b->u.size) {
    LNK_Symbol *a_symbol = a->symbol;
    LNK_Symbol *b_symbol = b->symbol;
    is_before = lnk_symbol_is_before(a_symbol, b_symbol);
  } else {
    is_before = a->u.size > b->u.size;
  }

  return is_before;
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_common_block_leaders_task)
{
  ProfBeginFunction();

  LNK_BuildImageTask *task          = raw_task;
  Rng1U64             contrib_range = task->u.patch_symtabs.common_block_ranges[task_id];

  for (U64 contrib_idx = contrib_range.min; contrib_idx < contrib_range.max; contrib_idx += 1) {
    LNK_CommonBlockContrib *contrib        = &task->u.patch_symtabs.common_block_contribs[contrib_idx];
    LNK_Symbol             *symbol         = contrib->symbol;
    LNK_ObjSymbolRef        symbol_ref     = lnk_ref_from_symbol(symbol);
    COFF_ParsedSymbol       parsed_symbol  = lnk_parsed_from_symbol(symbol);
    U64                     section_number = task->u.patch_symtabs.common_block_sect->sect_idx + 1;

    if (symbol_ref.obj->header.is_big_obj) {
      COFF_Symbol32 *symbol32 = parsed_symbol.raw_symbol;
      symbol32->value          = contrib->u.offset;
      symbol32->section_number = safe_cast_u32(section_number);
    } else {
      COFF_Symbol16 *symbol16 = parsed_symbol.raw_symbol;
      symbol16->value          = contrib->u.offset;
      symbol16->section_number = safe_cast_u16(section_number);
    }

    task->u.patch_symtabs.was_symbol_patched[symbol_ref.obj->input_idx][symbol_ref.symbol_idx] = 1;
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_common_block_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Common Block Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Common) {
      LNK_Symbol       *defn        = lnk_symbol_table_search(task->symtab, symbol.name);
      COFF_ParsedSymbol defn_parsed = lnk_parsed_from_symbol(defn);
      Assert(lnk_interp_from_symbol(defn) == COFF_SymbolValueInterp_Regular);
      if (defn) {
        if (obj->header.is_big_obj) {
          COFF_Symbol32 *symbol32  = symbol.raw_symbol;
          symbol32->section_number = defn_parsed.section_number;
          symbol32->value          = safe_cast_u32(defn_parsed.value);
          symbol32->storage_class  = COFF_SymStorageClass_Static;
        } else {
          COFF_Symbol16 *symbol16  = symbol.raw_symbol;
          symbol16->section_number = safe_cast_u16(defn_parsed.section_number);
          symbol16->value          = safe_cast_u32(defn_parsed.value);
          symbol16->storage_class  = COFF_SymStorageClass_Static;
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_regular_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBegin("Patch Regular Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

    if (task->u.patch_symtabs.was_symbol_patched[obj_idx][symbol_idx]) {
      continue;
    }

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular) {
      COFF_SectionHeader *sect_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);

      LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number-1];
      U32                 section_number;
      U32                 value;
      if (sc == task->null_sc) {
        section_number = lnk_obj_get_removed_section_number(obj);
        value          = max_U32;
      } else {
        section_number = safe_cast_u32(sc->u.sect_idx + 1);
        value          = sc->u.off + symbol.value;
      }

      if (obj->header.is_big_obj) {
        COFF_Symbol32 *symbol32  = symbol.raw_symbol;
        symbol32->section_number = section_number;
        symbol32->value          = value;
      } else {
        COFF_Symbol16 *symbol16  = symbol.raw_symbol;
        symbol16->section_number = safe_cast_u16(section_number);
        symbol16->value          = value;
      }
    }
  }
  ProfEnd();
}

internal void
lnk_patch_obj_symtab(LNK_SymbolTable *symtab, LNK_Obj *obj, B8 *was_symbol_patched, COFF_SymbolValueInterpType fixup_type)
{
  ProfBeginV("%S\n", obj->path);

  COFF_ParsedSymbol fixup_dst;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + fixup_dst.aux_symbol_count)) {
    fixup_dst = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    if (was_symbol_patched[symbol_idx]) { continue; }

    COFF_SymbolValueInterpType fixup_dst_type = coff_interp_symbol(fixup_dst.section_number, fixup_dst.value, fixup_dst.storage_class);
    if (fixup_type != fixup_dst_type) { continue; }

    LNK_ObjSymbolRef symbol_to_resolve = { .obj = obj, .symbol_idx = symbol_idx };
    LNK_ObjSymbolRef fixup_symbol      = {0};
    B32               is_resolved       = lnk_resolve_symbol(symtab, symbol_to_resolve, &fixup_symbol);
    if (is_resolved) {
      COFF_ParsedSymbol          fixup_src  = lnk_parsed_symbol_from_coff_symbol_idx(fixup_symbol.obj, fixup_symbol.symbol_idx);
      COFF_SymbolValueInterpType fixup_type = coff_interp_symbol(fixup_src.section_number, fixup_src.value, fixup_src.storage_class);

      U32 section_number;
      U32 value;
      if (fixup_type == COFF_SymbolValueInterp_Undefined || fixup_type == COFF_SymbolValueInterp_Weak) {
        section_number = lnk_obj_get_removed_section_number(obj);
        value          = 0;
      } else {
        section_number = fixup_src.section_number;
        value          = fixup_src.value;
      }

      if (obj->header.is_big_obj) {
        COFF_Symbol32 *symbol32  = fixup_dst.raw_symbol;
        symbol32->section_number = section_number;
        symbol32->value          = value;
        symbol32->storage_class  = COFF_SymStorageClass_Static;
      } else {
        COFF_Symbol16 *symbol16  = fixup_dst.raw_symbol;
        symbol16->section_number = (U16)section_number;
        symbol16->value          = value;
        symbol16->storage_class  = COFF_SymStorageClass_Static;
      }

      was_symbol_patched[symbol_idx] = 1;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_common_symbols_task)
{
  LNK_BuildImageTask *task = raw_task;
  lnk_patch_obj_symtab(task->symtab, task->objs[task_id], task->u.patch_symtabs.was_symbol_patched[task_id], COFF_SymbolValueInterp_Common);
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_abs_symbols_task)
{
  LNK_BuildImageTask *task = raw_task;
  lnk_patch_obj_symtab(task->symtab, task->objs[task_id], task->u.patch_symtabs.was_symbol_patched[task_id], COFF_SymbolValueInterp_Abs);
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_undefined_symbols_task)
{
  LNK_BuildImageTask *task = raw_task;
  lnk_patch_obj_symtab(task->symtab, task->objs[task_id], task->u.patch_symtabs.was_symbol_patched[task_id], COFF_SymbolValueInterp_Undefined);
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_weak_symbols_task)
{
  LNK_BuildImageTask *task = raw_task;
  lnk_patch_obj_symtab(task->symtab, task->objs[task_id], task->u.patch_symtabs.was_symbol_patched[task_id], COFF_SymbolValueInterp_Weak);
}

internal U64
lnk_compute_win32_image_header_size(LNK_Config *config, U64 sect_count)
{
  U64 image_header_size = 0;
  image_header_size += sizeof(PE_DosHeader) + pe_dos_program.size;
  image_header_size += sizeof(U32); // PE_MAGIC
  image_header_size += sizeof(COFF_FileHeader);
  image_header_size += pe_has_plus_header(config->machine) ? sizeof(PE_OptionalHeader32Plus) : sizeof(PE_OptionalHeader32);
  image_header_size += sizeof(PE_DataDirectory) * config->data_dir_count;
  image_header_size += sizeof(COFF_SectionHeader) * sect_count;
  return image_header_size;
}

internal
THREAD_POOL_TASK_FUNC(lnk_obj_reloc_patcher)
{
  LNK_ObjRelocPatcher *task = raw_task;
  LNK_Obj             *obj  = task->objs[task_id];

  COFF_FileHeaderInfo  obj_header    = obj->header;
  COFF_SectionHeader  *section_table = lnk_coff_section_table_from_obj(obj);
  String8              symbol_table  = lnk_coff_symbol_table_from_obj(obj);
  String8              string_table  = lnk_coff_string_table_from_obj(obj);

  U32 closest_sect  = 0;
  U32 closest_reloc = 0;
  U32 closest_foff  = max_U32;

  for EachIndex(sect_idx, obj_header.section_count_no_null) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];

    if (section_header->flags & COFF_SectionFlag_LnkInfo)              { continue; }
    if (section_header->flags & COFF_SectionFlag_LnkRemove)            { continue; }
    if (section_header->flags & COFF_SectionFlag_CntUninitializedData) { continue; }

    // get section bytes (special case debug info because it is not copied to the image)
    String8 data           = section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO ? obj->data : task->image_data;
    Rng1U64 section_frange = rng_1u64(section_header->foff, section_header->foff + section_header->fsize);
    String8 section_data   = str8_substr(data, section_frange);

    // apply relocs
    COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, section_header);
    for EachIndex(reloc_idx, relocs.count) {
      COFF_Reloc *reloc = &relocs.v[reloc_idx];

      // error check relocation
      if (obj->header.machine == COFF_MachineType_X64) {
        if (reloc->type > COFF_Reloc_X64_Last) {
          lnk_error_obj(LNK_Error_IllegalRelocation, obj, "unknown relocation type 0x%x", reloc->type);
        }
      } else if (obj->header.machine != COFF_MachineType_Unknown) {
        lnk_not_implemented("relocation patching is not implemented for %S", coff_string_from_machine_type(obj->header.machine));
        continue;
      }

      // compute virtual offsets
      U64 reloc_voff = section_header->voff + reloc->apply_off;

      // compute symbol location values
      U32 symbol_secnum = 0;
      U32 symbol_secoff = 0;
      S64 symbol_voff   = 0;
      {
        COFF_ParsedSymbol          symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, reloc->isymbol);
        COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
        if (interp == COFF_SymbolValueInterp_Regular) {
          if (symbol.section_number == lnk_obj_get_removed_section_number(obj)) {
            if (~section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) {
              String8 sect_name = coff_name_from_section_header(string_table, &section_table[sect_idx]);
              lnk_error_obj(LNK_Error_RelocationAgainstRemovedSection, obj, "relocating against symbol that is in a removed section (symbol: %S, reloc-section: %S 0x%llx, reloc-index: 0x%llx)", symbol.name, sect_name, sect_idx+1, reloc_idx);
            }
            continue;
          }
          symbol_secnum = symbol.section_number;
          symbol_secoff = symbol.value;
          symbol_voff   = safe_cast_u32((U64)task->image_section_table[symbol.section_number]->voff + (U64)symbol_secoff);
        } else if (interp == COFF_SymbolValueInterp_Abs) {
          // There aren't enough bits in COFF symbol to store full image base address,
          // so we special case __ImageBase. A better solution would be to add
          // a 64-bit symbol format to COFF.
          if (str8_match(symbol.name, str8_lit("__ImageBase"), 0)) {
            symbol.value = task->image_base;
          }
          symbol_secnum = 0;
          symbol_secoff = 0;
          symbol_voff   = (S64)symbol.value - (S64)task->image_base;
        } else if (interp == COFF_SymbolValueInterp_Weak) {
          // unresolved weak
        } else if (interp == COFF_SymbolValueInterp_Undefined) {
          // unresolved undefined
        } else {
          InvalidPath;
        }
      }

      // pick reloc value
      COFF_RelocValue reloc_value = {0};
      switch (obj_header.machine) {
      case COFF_MachineType_Unknown: {} break;
      case COFF_MachineType_X64: { reloc_value = coff_pick_reloc_value_x64(reloc->type, task->image_base, reloc_voff, symbol_secnum, symbol_secoff, symbol_voff); } break;
      default: { NotImplemented; } break;
      }

      // read addend
      Assert(reloc_value.size <= section_data.size);
      U64 raw_addend = 0;
      str8_deserial_read(section_data, reloc->apply_off, &raw_addend, reloc_value.size, 1);

      // compute new reloc value
      S64 addend       = extend_sign64(raw_addend, reloc_value.size);
      U64 reloc_result = reloc_value.value + addend;

      // commit new reloc value
      MemoryCopy(section_data.str + reloc->apply_off, &reloc_result, reloc_value.size);
    }
  }
}

internal int
lnk_section_definition_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionDefinition **a = raw_a, **b = raw_b;
  U64 input_idx_a = Compose64Bit((*a)->obj->input_idx, (*a)->obj_sect_idx);
  U64 input_idx_b = Compose64Bit((*b)->obj->input_idx, (*b)->obj_sect_idx);
  return u64_compar_is_before(&input_idx_a, &input_idx_b);
}

internal
THREAD_POOL_TASK_FUNC(lnk_count_common_block_contribs_task)
{
  LNK_BuildImageTask *task   = raw_task;
  LNK_SymbolTable    *symtab = task->symtab;

  for (LNK_SymbolHashTrieChunk *chunk = symtab->chunks[task_id].first; chunk != 0; chunk = chunk->next) {
    for EachIndex(i, chunk->count) {
      LNK_Symbol                 *symbol        = chunk->v[i].symbol;
      COFF_ParsedSymbol           parsed_symbol = lnk_parsed_from_symbol(symbol);
      COFF_SymbolValueInterpType  parsed_interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
      if (parsed_interp == COFF_SymbolValueInterp_Common) {
        task->u.common_block.counts[task_id] += 1;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_fill_out_common_block_contribs_task)
{
  LNK_BuildImageTask *task   = raw_task;
  LNK_SymbolTable    *symtab = task->symtab;
  U64                 cursor = task->u.common_block.offsets[task_id];

  for (LNK_SymbolHashTrieChunk *chunk = symtab->chunks[task_id].first; chunk != 0; chunk = chunk->next) {
    for EachIndex(i, chunk->count) {
      LNK_Symbol                 *symbol        = chunk->v[i].symbol;
      COFF_ParsedSymbol           parsed_symbol = lnk_parsed_from_symbol(symbol);
      COFF_SymbolValueInterpType  parsed_interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
      if (parsed_interp == COFF_SymbolValueInterp_Common) {
        LNK_CommonBlockContrib *contrib = &task->u.common_block.contribs[cursor++];
        contrib->symbol                 = chunk->v[i].symbol;
        contrib->u.size                 = parsed_symbol.value;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_flag_hotpatch_contribs_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];
  
  if (obj->hotpatch) {
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular && COFF_SymbolType_IsFunc(symbol.type)) {
        COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);
        LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number-1];
        if (sc != task->null_sc) {
          sc->hotpatch = !!(section_header->flags & COFF_SectionFlag_CntCode);
        }
      }
    }
  }
}

internal void
lnk_push_coff_symbols_from_data(Arena *arena, LNK_SymbolList *symbol_list, String8 data, LNK_SymbolArray obj_symbols)
{
  if (data.size % sizeof(U32)) {
    // TODO: report invalid data size
  }
  U64 count = data.size / sizeof(U32);
  for (U32 *ptr = (U32*)data.str, *opl = ptr + count; ptr < opl; ++ptr) {
    U32 coff_symbol_idx = *ptr;
    if (coff_symbol_idx >= obj_symbols.count) {
      // TODO: report invalid symbol index
      continue;
    }
    Assert(coff_symbol_idx < obj_symbols.count);
    LNK_Symbol *symbol = obj_symbols.v + coff_symbol_idx;
    lnk_symbol_list_push(arena, symbol_list, symbol);
  }
}

internal String8
lnk_build_guard_data(Arena *arena, U64Array voff_arr, U64 stride)
{
  Assert(stride >= sizeof(U32));
  
  // check for duplicates
#if DEBUG
  for (U64 i = 1; i < voff_arr.count; ++i) {
    Assert(voff_arr.[i-1] != voff_ptr[i]);
  }
#endif
  
  U64 buffer_size = stride * voff_arr.count;
  U8 *buffer = push_array(arena, U8, buffer_size);
  for (U64 i = 0; i < voff_arr.count; ++i) {
    U32 *voff_ptr = (U32*)(buffer + i * stride);
    *voff_ptr = voff_arr.v[i];
  }
  
  String8 guard_data = str8(buffer, buffer_size);
  return guard_data;
}

internal String8List
lnk_build_guard_tables(TP_Context       *tp,
                       LNK_SectionTable *sectab,
                       LNK_SymbolTable  *symtab,
                       U64               objs_count,
                       LNK_Obj         **objs,
                       COFF_MachineType  machine,
                       String8           entry_point_name,
                       LNK_GuardFlags    guard_flags,
                       B32               emit_suppress_flag)
{
  NotImplemented;
  String8List result = {0};
  return result;
#if 0
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  LNK_Section **sect_id_map = lnk_sect_id_map_from_section_table(scratch.arena, sectab);
  
  enum { GUARD_FIDS, GUARD_IATS, GUARD_LJMP, GUARD_EHCONT, GUARD_COUNT };
  LNK_SymbolList guard_symbol_list_table[GUARD_COUNT]; MemoryZeroStruct(&guard_symbol_list_table[0]);
  
  // collect symbols from objs
  for (LNK_ObjNode *obj_node = obj_list.first; obj_node != NULL; obj_node = obj_node->next) {
    LNK_Obj *obj = &obj_node->data;
    MSCRT_FeatFlags feat_flags = lnk_obj_get_features(obj);
    B32 has_guard_flags = (feat_flags & MSCRT_FeatFlag_GUARD_CF) || (feat_flags & MSCRT_FeatFlag_GUARD_EH_CONT);
    if (has_guard_flags) {
      LNK_SymbolArray symbol_arr = lnk_symbol_array_from_list(scratch.arena, obj->symbol_list);
      if (guard_flags & LNK_Guard_Cf) {
        String8List gfids_list = lnk_collect_obj_chunks(scratch.arena, obj, str8_lit(".gfids"), str8_zero(), 1);
        for (String8Node *node = gfids_list.first; node != 0; node = node->next) {
          lnk_push_coff_symbols_from_data(scratch.arena, &guard_symbol_list_table[GUARD_FIDS], node->string, symbol_arr);
        }
        String8List giats_list = lnk_collect_obj_chunks(scratch.arena, obj, str8_lit(".giats"), str8_zero(), 1);
        for (String8Node *node = giats_list.first; node != 0; node = node->next) {
          lnk_push_coff_symbols_from_data(scratch.arena, &guard_symbol_list_table[GUARD_IATS], node->string, symbol_arr);
        }
      }
      if (guard_flags & LNK_Guard_LongJmp) {
        String8List gljmp_list = lnk_obj_search_chunks(scratch.arena, obj, str8_lit(".gljmp"), str8_zero(), 1);
        for (String8Node *node = gljmp_list.first; node != 0; node = node->next) {
          lnk_push_coff_symbols_from_data(scratch.arena, &guard_symbol_list_table[GUARD_LJMP], node->string, symbol_arr);
        }
      }
      if (guard_flags & LNK_Guard_EhCont) {
        String8List gehcont_list = lnk_obj_search_chunks(scratch.arena, obj, str8_lit(".gehcont"), str8_zero(), 1);
        for (String8Node *node = gehcont_list.first; node != 0; node = node->next) {
          lnk_push_coff_symbols_from_data(scratch.arena, &guard_symbol_list_table[GUARD_EHCONT], node->string, symbol_arr);
        }
      }
    } else {
      // TODO: loop over COFF relocs
      NotImplemented;
#if 0
      // use relocation data in code sections to get function symbols
      for (U64 isect = 0; isect < obj->sect_count; ++isect) {
        LNK_Chunk *chunk = obj->chunk_arr[isect];
        if (!chunk) {
          continue;
        }
        if (lnk_chunk_is_discarded(chunk)) {
          continue;
        }
        if (~chunk->flags & COFF_SectionFlag_CntCode) {
          continue;
        }
        Assert(chunk->type == LNK_Chunk_Leaf);
        for (LNK_Reloc *reloc = obj->sect_reloc_list_arr[isect].first; reloc != 0; reloc = reloc->next) {
          LNK_Symbol *symbol = lnk_resolve_symbol(symtab, reloc->symbol);
          if (!LNK_Symbol_IsDefined(symbol->type)) {
            continue;
          }
          LNK_DefinedSymbol *defined_symbol = &symbol->u.defined;
          if (~defined_symbol->flags & LNK_DefinedSymbolFlag_IsFunc) {
            continue;
          }
          LNK_Chunk *symbol_chunk = defined_symbol->u.chunk;
          if (!symbol_chunk) {
            continue;
          }
          if (symbol_chunk->type != LNK_Chunk_Leaf) {
            continue;
          }
          if (~symbol_chunk->flags & COFF_SectionFlag_CntCode) {
            continue;
          }
          lnk_symbol_list_push(scratch.arena, &guard_symbol_list_table[GUARD_FIDS], symbol);
        }
      }
#endif
    }
  }
  
  // entry point
  LNK_Symbol *entry_point_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, entry_point_name);
  lnk_symbol_list_push(scratch.arena, &guard_symbol_list_table[GUARD_FIDS], entry_point_symbol);
  
  // push exports
  {
    Temp temp = temp_begin(scratch.arena);
    KeyValuePair *raw_exports = key_value_pairs_from_hash_table(temp.arena, exptab->name_export_ht);
    for (U64 i = 0; i < exptab->name_export_ht->count; ++i) {
      LNK_Export *exp = raw_exports[i].value_raw;
      lnk_symbol_list_push(scratch.arena, &guard_symbol_list_table[GUARD_FIDS], exp->symbol);
    }
    scratch_end(temp);
  }
  
  // TODO: push noname exports
  
  NotImplemented;
#if 0
  // push thunks
  LNK_SymbolScope scope_array[] = { LNK_SymbolScope_Defined, LNK_SymbolScope_Internal };
  for (U64 iscope = 0; iscope < ArrayCount(scope_array); ++iscope) {
    LNK_SymbolScope scope = scope_array[iscope];
    for (U64 ibucket = 0; ibucket < symtab->bucket_count[scope]; ++ibucket) {
      for (LNK_SymbolNode *symbol_node = symtab->buckets[scope][ibucket].first;
           symbol_node != NULL;
           symbol_node = symbol_node->next) {
        LNK_Symbol *symbol = symbol_node->data;
        if (!LNK_Symbol_IsDefined(symbol->type)) continue;
        LNK_DefinedSymbol *defined_symbol = &symbol->u.defined;
        if (~defined_symbol->flags & LNK_DefinedSymbolFlag_IsThunk) continue;
        lnk_symbol_list_push(scratch.arena, &guard_symbol_list_table[GUARD_FIDS], symbol);
      } 
    }
  }
#endif
  
  // build section data
  lnk_section_table_build_data(tp, sectab, machine);
  lnk_section_table_assign_virtual_offsets(sectab);
  
  // compute symbols virtual offsets
  U64Array guard_voff_arr_table[GUARD_COUNT];
  for (U64 i = 0; i < ArrayCount(guard_symbol_list_table); ++i) {
    U64List voff_list; MemoryZeroStruct(&voff_list);
    LNK_SymbolList symbol_list = guard_symbol_list_table[i];
    for (LNK_SymbolNode *symbol_node = symbol_list.first; symbol_node != NULL; symbol_node = symbol_node->next) {
      LNK_Symbol *symbol = lnk_resolve_symbol(symtab, symbol_node->data);
      if (!LNK_Symbol_IsDefined(symbol->type)) {
        continue;
      }
      LNK_DefinedSymbol *defined_symbol = &symbol->u.defined;
      LNK_Chunk *chunk = defined_symbol->u.chunk;
      if (!chunk) {
        continue;
      }
      if (lnk_chunk_is_discarded(chunk)) {
        continue;
      }
      U64 chunk_voff = lnk_virt_off_from_chunk_ref(sect_id_map, chunk->ref);
      U64 symbol_voff = chunk_voff + defined_symbol->u.chunk_offset;
      Assert(symbol_voff != 0);
      u64_list_push(scratch.arena, &voff_list, symbol_voff);
    }
    U64Array voff_arr = u64_array_from_list(scratch.arena, &voff_list);
    radsort(voff_arr.v, voff_arr.count, u64_compar_is_before);
    guard_voff_arr_table[i] = u64_array_remove_duplicates(scratch.arena, voff_arr);
  }
  
  // push guard sections
  static struct {
    char *name;
    char *symbol;
    int flags;
  } sect_layout[] = {
    { ".gfids",   LNK_GFIDS_SYMBOL_NAME,   LNK_GFIDS_SECTION_FLAGS   },
    { ".giats",   LNK_GIATS_SYMBOL_NAME,   LNK_GIATS_SECTION_FLAGS   },
    { ".gljmp",   LNK_GLJMP_SYMBOL_NAME,   LNK_GLJMP_SECTION_FLAGS   },
    { ".gehcont", LNK_GEHCONT_SYMBOL_NAME, LNK_GEHCONT_SECTION_FLAGS },
  };
  for (U64 i = 0; i < ArrayCount(sect_layout); ++i) {
    LNK_Section *sect = lnk_section_table_push(sectab, str8_cstring(sect_layout[i].name), sect_layout[i].flags);
  }
  
  // TODO: emit table for SEH on X86
  if (machine == COFF_MachineType_X86) {
    lnk_not_implemented("__safe_se_handler_table");
    lnk_not_implemented("__safe_se_handler_count");
  }
  
  LNK_Symbol *gfids_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Internal, str8_lit(LNK_GFIDS_SYMBOL_NAME));
  LNK_Symbol *giats_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Internal, str8_lit(LNK_GIATS_SYMBOL_NAME));
  LNK_Symbol *gljmp_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Internal, str8_lit(LNK_GLJMP_SYMBOL_NAME));
  LNK_Symbol *gehcont_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Internal, str8_lit(LNK_GEHCONT_SYMBOL_NAME));
  
  LNK_Section *gfids_sect = lnk_section_table_search_id(sectab, gfids_symbol->u.defined.u.chunk->ref.sect_id);
  LNK_Section *giats_sect = lnk_section_table_search_id(sectab, giats_symbol->u.defined.u.chunk->ref.sect_id);
  LNK_Section *gljmp_sect = lnk_section_table_search_id(sectab, gljmp_symbol->u.defined.u.chunk->ref.sect_id);
  LNK_Section *gehcont_sect = lnk_section_table_search_id(sectab, gehcont_symbol->u.defined.u.chunk->ref.sect_id);
  
  LNK_Chunk *gfids_array_chunk = gfids_sect->root;
  LNK_Chunk *giats_array_chunk = giats_sect->root;
  LNK_Chunk *gljmp_array_chunk = gljmp_sect->root;
  LNK_Chunk *gehcont_array_chunk = gehcont_sect->root;
  
  // first 4 bytes are call's destination virtual offset
  U64 entry_stride = sizeof(U32);
  if (emit_suppress_flag) {
    // 4th byte tells kernel what to do when destination VA is not in the bitmap. 
    // If byte is 1 exception is suppressed and program keeps running.
    // If zero then exception is raised with nt!_KiRaiseSecurityCheckFailure(FAST_FAIL_GUARD_ICALL_CHECK_FAILURE) and exception code 0xA.
    entry_stride = 5;
  }
  
  // make guard data from virtual offsets
  String8 gfids_data   = lnk_build_guard_data(gfids_sect->arena, guard_voff_arr_table[GUARD_FIDS], entry_stride);
  String8 giats_data   = lnk_build_guard_data(giats_sect->arena, guard_voff_arr_table[GUARD_IATS], entry_stride);
  String8 gljmp_data   = lnk_build_guard_data(gljmp_sect->arena, guard_voff_arr_table[GUARD_LJMP], entry_stride);
  String8 gehcont_data = lnk_build_guard_data(gehcont_sect->arena, guard_voff_arr_table[GUARD_EHCONT], entry_stride);
  
  // push guard data
  lnk_section_push_chunk_data(gfids_sect, gfids_array_chunk, gfids_data, str8_zero());
  lnk_section_push_chunk_data(giats_sect, giats_array_chunk, giats_data, str8_zero());
  lnk_section_push_chunk_data(gljmp_sect, gljmp_array_chunk, gljmp_data, str8_zero());
  lnk_section_push_chunk_data(gehcont_sect, gehcont_array_chunk, gehcont_data, str8_zero());
  
  LNK_Symbol *gflags_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_FLAGS_SYMBOL_NAME));
  LNK_Symbol *gfids_table_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_FIDS_TABLE_SYMBOL_NAME));
  LNK_Symbol *gfids_count_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_FIDS_COUNT_SYMBOL_NAME));
  LNK_Symbol *giats_table_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_IAT_TABLE_SYMBOL_NAME));
  LNK_Symbol *giats_count_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_IAT_COUNT_SYMBOL_NAME));
  LNK_Symbol *gljmp_table_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_LONGJMP_TABLE_SYMBOL_NAME));
  LNK_Symbol *gljmp_count_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_LONGJMP_COUNT_SYMBOL_NAME));
  LNK_Symbol *gehcont_table_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_EHCONT_TABLE_SYMBOL_NAME));
  LNK_Symbol *gehcont_count_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Main, str8_lit(MSCRT_GUARD_EHCONT_COUNT_SYMBOL_NAME));
  
  LNK_DefinedSymbol *gflags_def = &gflags_symbol->u.defined;
  LNK_DefinedSymbol *gfids_table_def = &gfids_table_symbol->u.defined;
  LNK_DefinedSymbol *gfids_count_def = &gfids_count_symbol->u.defined;
  LNK_DefinedSymbol *giats_table_def = &giats_table_symbol->u.defined;
  LNK_DefinedSymbol *giats_count_def = &giats_count_symbol->u.defined;
  LNK_DefinedSymbol *gljmp_table_def = &gljmp_table_symbol->u.defined;
  LNK_DefinedSymbol *gljmp_count_def = &gljmp_count_symbol->u.defined;
  LNK_DefinedSymbol *gehcont_table_def = &gehcont_table_symbol->u.defined;
  LNK_DefinedSymbol *gehcont_count_def = &gehcont_count_symbol->u.defined;
  
  // guard flags
  gflags_def->value_type = LNK_DefinedSymbolValue_VA;
  gflags_def->u.va = PE_LoadConfigGuardFlags_CF_INSTRUMENTED;
  if ((guard_flags & LNK_Guard_Cf)) {
    gflags_def->u.va |= PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_PRESENT;
  }
  if ((guard_flags & LNK_Guard_LongJmp) && guard_voff_arr_table[GUARD_LJMP].count) {
    gflags_def->u.va |= PE_LoadConfigGuardFlags_CF_LONGJUMP_TABLE_PRESENT;
  }
  if ((guard_flags & LNK_Guard_EhCont) && guard_voff_arr_table[GUARD_EHCONT].count) {
    gflags_def->u.va |= PE_LoadConfigGuardFlags_EH_CONTINUATION_TABLE_PRESENT;
  }
  {
    LNK_Section *didat_sect = lnk_section_table_search(sectab, str8_lit(".didat"));
    if (didat_sect) {
      gflags_def->u.va |= PE_LoadConfigGuardFlags_DELAYLOAD_IAT_IN_ITS_OWN_SECTION;
    }
  }
  if (entry_stride > sizeof(U32)) {
    U64 size_bit = (entry_stride - 5);
    if (emit_suppress_flag) {
      gflags_def->u.va |= PE_LoadConfigGuardFlags_CF_EXPORT_SUPPRESSION_INFO_PRESENT;
    }
    gflags_def->u.va |= (1 << size_bit) << PE_LoadConfigGuardFlags_CF_FUNCTION_TABLE_SIZE_SHIFT;
  }
  
  // gfids
  if (guard_voff_arr_table[GUARD_FIDS].count) {
    gfids_table_def->value_type = LNK_DefinedSymbolValue_Chunk;
    gfids_table_def->u.chunk = gfids_array_chunk;
  }
  gfids_count_def->value_type = LNK_DefinedSymbolValue_VA;
  gfids_count_def->u.va = guard_voff_arr_table[GUARD_FIDS].count;
  
  // giats
  if (guard_voff_arr_table[GUARD_IATS].count) {
    giats_table_def->value_type = LNK_DefinedSymbolValue_Chunk;
    giats_table_def->u.chunk = giats_array_chunk;
  }
  giats_count_def->value_type = LNK_DefinedSymbolValue_VA;
  giats_count_def->u.va = guard_voff_arr_table[GUARD_IATS].count;
  
  // gljmp
  if (guard_voff_arr_table[GUARD_LJMP].count) {
    gljmp_table_def->value_type = LNK_DefinedSymbolValue_Chunk;
    gljmp_table_def->u.chunk = gljmp_array_chunk;
  }
  gljmp_count_def->value_type = LNK_DefinedSymbolValue_VA;
  gljmp_count_def->u.va = guard_voff_arr_table[GUARD_LJMP].count;
  
  // gehcont
  if (guard_voff_arr_table[GUARD_EHCONT].count) {
    gehcont_table_def->value_type = LNK_DefinedSymbolValue_Chunk;
    gehcont_table_def->u.chunk = gehcont_array_chunk;
  }
  gehcont_count_def->value_type = LNK_DefinedSymbolValue_VA;
  gehcont_count_def->u.va = guard_voff_arr_table[GUARD_EHCONT].count;
  
  scratch_end(scratch);
  ProfEnd();
#endif
}

internal
THREAD_POOL_TASK_FUNC(lnk_emit_base_relocs_from_objs_task)
{
  ProfBeginFunction();

  LNK_ObjBaseRelocTask *task  = raw_task;
  Rng1U64               range = task->ranges[task_id];

  HashTable             *page_ht   = task->page_ht_arr[task_id];
  LNK_BaseRelocPageList *page_list = &task->list_arr[task_id];

  for (U64 obj_idx = range.min; obj_idx < range.max; ++obj_idx) {
    LNK_Obj            *obj           = task->obj_arr[obj_idx];
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
    for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
      COFF_SectionHeader *sect_header = &section_table[sect_idx];

      if (sect_header->flags & COFF_SectionFlag_LnkRemove) {
        continue;
      }

      COFF_RelocInfo  reloc_info = coff_reloc_info_from_section_header(obj->data, sect_header);
      COFF_Reloc     *relocs     = (COFF_Reloc *)(obj->data.str + reloc_info.array_off);

      for (U64 reloc_idx = 0; reloc_idx < reloc_info.count; reloc_idx += 1) {
        COFF_Reloc *r = &relocs[reloc_idx];

        COFF_ParsedSymbol          symbol            = lnk_parsed_symbol_from_coff_symbol_idx(obj, r->isymbol);
        COFF_SymbolValueInterpType symbol_interp     = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
        B32                        is_symbol_address = symbol_interp != COFF_SymbolValueInterp_Abs;

        if (is_symbol_address) {
          B32 is_addr32 = 0, is_addr64 = 0;
          switch (obj->header.machine) {
          case COFF_MachineType_Unknown: {} break;
          case COFF_MachineType_X64: {
            is_addr32 = r->type == COFF_Reloc_X64_Addr32;
            is_addr64 = r->type == COFF_Reloc_X64_Addr64;
          } break;
          default: { NotImplemented; } break;
          }

          if (is_addr32 || is_addr64) {
            U64 reloc_voff = sect_header->voff + r->apply_off;
            U64 page_voff  = AlignDownPow2(reloc_voff, task->page_size);

            LNK_BaseRelocPageNode *page;
            {
              KeyValuePair *is_page_present = hash_table_search_u64(page_ht, page_voff);
              if (is_page_present) {
                page = is_page_present->value_raw;
              } else {
                // fill out page
                page = push_array(arena, LNK_BaseRelocPageNode, 1);
                page->v.voff = page_voff;

                // push page
                SLLQueuePush(page_list->first, page_list->last, page);
                page_list->count += 1;

                // register page voff
                hash_table_push_u64_raw(arena, page_ht, page_voff, page);
              }
            }

            if (is_addr32) {
              if (task->is_large_addr_aware) {
                COFF_ParsedSymbol symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, r->isymbol);
                lnk_error_obj(LNK_Error_LargeAddrAwareRequired, obj, "found out of range ADDR32 relocation for '%S', link with /LARGEADDRESSAWARE:NO", symbol.name);
              } else {
                u64_list_push(arena, &page->v.entries_addr32, reloc_voff);
              }
            } else {
              u64_list_push(arena, &page->v.entries_addr64, reloc_voff);
            }
          }
        }
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_virtual_offsets_and_sizes_in_obj_section_headers_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Virtual Offset And Size In Section Headers [%S]", obj->path);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    if (~sect_header->flags & COFF_SectionFlag_LnkRemove) {
      LNK_SectionContrib *sc   = task->sect_map[obj_idx][sect_idx];
      LNK_Section        *sect = task->image_sects.v[sc->u.sect_idx];
      sect_header->vsize = lnk_size_from_section_contrib(sc);
      sect_header->voff  = sect->voff + sc->u.off;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_file_offsets_and_sizes_in_obj_section_headers_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch File Offsets And Sizes In Obj Section Headers [%S]", obj->path);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    B32 patch_section_header = (~sect_header->flags & COFF_SectionFlag_LnkRemove) &&
                               (~sect_header->flags & LNK_SECTION_FLAG_DEBUG_INFO);
    if (patch_section_header) {
      LNK_SectionContrib *sc   = task->sect_map[obj_idx][sect_idx];
      LNK_Section        *sect = task->image_sects.v[sc->u.sect_idx];
      if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
        sect_header->fsize = lnk_size_from_section_contrib(sc);
        sect_header->foff  = sect->foff + sc->u.off;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_section_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBegin("Patch Section Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Undefined) {
      if (symbol.storage_class == COFF_SymStorageClass_Section) {
        LNK_Section *sect = lnk_section_table_search(task->sectab, symbol.name, symbol.value);
        if (sect) {
          if (~sect->flags & COFF_SectionFlag_MemDiscardable) {
            LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(sect);
            if (obj->header.is_big_obj) {
              COFF_Symbol32 *symbol32 = symbol.raw_symbol;
              symbol32->section_number = safe_cast_u32(first_sc->u.sect_idx + 1);
              symbol32->value          = first_sc->u.off;
              symbol32->storage_class  = COFF_SymStorageClass_Static;
            } else {
              COFF_Symbol16 *symbol16 = symbol.raw_symbol;
              symbol16->section_number = safe_cast_u16(first_sc->u.sect_idx + 1);
              symbol16->value          = first_sc->u.off;
              symbol16->storage_class  = COFF_SymStorageClass_Static;
            }
          } else {
            lnk_error_obj(LNK_Error_SectRefsDiscardedMemory, obj, "symbol %S (No. 0x%llx) references section with discard flag", symbol.name, symbol_idx);
          }
        } else {
          lnk_error_obj(LNK_Error_UnresolvedSymbol, obj, "undefined section symbol %S (No 0x%llx) refers to an image section that doesn't exist", symbol.name, symbol_idx);
        }
      }
    }
  }
  ProfEnd();
}

internal int
lnk_base_reloc_page_compar(const void *raw_a, const void *raw_b)
{
  const LNK_BaseRelocPage *a = raw_a, *b = raw_b;
  return u64_compar(&a->voff, &b->voff);
}

internal int
lnk_base_reloc_page_is_before(void *raw_a, void *raw_b)
{
  LNK_BaseRelocPage* a = raw_a;
  LNK_BaseRelocPage* b = raw_b;
  return a->voff < b->voff;
}

internal String8List
lnk_build_base_relocs(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 objs_count, LNK_Obj **objs)
{
  ProfBeginFunction();

  Arena *arena   = tp_arena->v[0];
  Temp   scratch = scratch_begin(tp_arena->v, tp_arena->count);
  tp_arena->v[0] = scratch.arena;
  TP_Temp tp_temp = tp_temp_begin(tp_arena);
  
  LNK_BaseRelocPageArray page_arr;
  {
    LNK_BaseRelocPageList  *page_list_arr = push_array(scratch.arena, LNK_BaseRelocPageList, tp->worker_count);
    HashTable             **page_ht_arr   = push_array_no_zero(scratch.arena, HashTable *, tp->worker_count);
    for (U64 i = 0; i < tp->worker_count; ++i) {
      page_ht_arr[i] = hash_table_init(scratch.arena, 1024);
    }

    {
      ProfBegin("Emit Relocs From Objs");
      LNK_ObjBaseRelocTask task = {0};
      task.ranges               = tp_divide_work(scratch.arena, objs_count, tp->worker_count);
      task.page_size            = config->machine_page_size;
      task.page_ht_arr          = page_ht_arr;
      task.list_arr             = page_list_arr;
      task.obj_arr              = objs;
      task.is_large_addr_aware  = !!(config->file_characteristics & PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE);
      tp_for_parallel(tp, tp_arena, tp->worker_count, lnk_emit_base_relocs_from_objs_task, &task);
      ProfEnd();
    }

    LNK_BaseRelocPageList *main_page_list = &page_list_arr[0];
    {
      ProfBegin("Merge Worker Page Lists");
      HashTable *main_ht = page_ht_arr[0];
      for (U64 list_idx = 1; list_idx < tp->worker_count; ++list_idx) {
        LNK_BaseRelocPageList src = page_list_arr[list_idx];

        for (LNK_BaseRelocPageNode *src_page = src.first, *src_next; src_page != 0; src_page = src_next) {
          src_next = src_page->next;

          KeyValuePair *is_page_present = hash_table_search_u64(main_ht, src_page->v.voff);
          if (is_page_present) {
            // page exists concat voffs
            LNK_BaseRelocPageNode *page = is_page_present->value_raw;
            Assert(page != src_page);
            u64_list_concat_in_place(&page->v.entries_addr32, &src_page->v.entries_addr32);
            u64_list_concat_in_place(&page->v.entries_addr64, &src_page->v.entries_addr64);
          } else {
            // push page to main list
            SLLQueuePush(main_page_list->first, main_page_list->last, src_page);
            main_page_list->count += 1;

            // store lookup voff 
            hash_table_push_u64_raw(scratch.arena, main_ht, src_page->v.voff, src_page);
          }
        }
      }
      ProfEnd();
    }

    ProfBegin("Page List -> Array");
    page_arr.count = 0;
    page_arr.v     = push_array_no_zero(scratch.arena, LNK_BaseRelocPage, main_page_list->count);
    for (LNK_BaseRelocPageNode* n = main_page_list->first; n != 0; n = n->next) {
      page_arr.v[page_arr.count++] = n->v;
    }
    ProfEnd();

    ProfBegin("Sort Pages on VOFF");
    //radsort(page_arr.v, page_arr.count, lnk_base_reloc_page_is_before);
    qsort(page_arr.v, page_arr.count, sizeof(page_arr.v[0]), lnk_base_reloc_page_compar);
    ProfEnd();
  }
  
  String8List result = {0};
  if (page_arr.count) {
    ProfBegin("Serialize Pages");
    HashTable *voff_ht = hash_table_init(scratch.arena, config->machine_page_size);
    for (U64 page_idx = 0; page_idx < page_arr.count; ++page_idx) {
      LNK_BaseRelocPage *page = &page_arr.v[page_idx];

      U64 total_entry_count = 0;
      total_entry_count += page->entries_addr32.count;
      total_entry_count += page->entries_addr64.count;

      U32 *page_voff_ptr;
      U32 *block_size_ptr;
      U16 *reloc_arr_base;
      
      // push buffer
      U64   buf_size = AlignPow2(sizeof(*page_voff_ptr) + sizeof(*block_size_ptr) + sizeof(*reloc_arr_base)*total_entry_count, sizeof(U32));
      void *buf      = push_array_no_zero(arena, U8, buf_size);
      
      // setup pointers into buffer
      page_voff_ptr  = buf;
      block_size_ptr = page_voff_ptr + 1;
      reloc_arr_base = (U16*)(block_size_ptr + 1);
      
      // write 32-bit relocations
      U16 *reloc_arr_ptr = reloc_arr_base;
      for (U64Node *i = page->entries_addr32.first; i != 0; i = i->next) {
        // was base reloc_entry made?
        if (hash_table_search_u64(voff_ht, i->data)) {
          continue;
        }
        hash_table_push_u64_u64(scratch.arena, voff_ht, i->data, 0);

        // write entry
        U64 rel_off = i->data - page->voff;
        Assert(rel_off <= config->machine_page_size);
        *reloc_arr_ptr++ = PE_BaseRelocMake(PE_BaseRelocKind_HIGHLOW, rel_off);
      }
      
      // write 64-bit relocations
      for (U64Node *i = page->entries_addr64.first; i != 0; i = i->next) {
        // was base reloc entry made?
        if (hash_table_search_u64(voff_ht, i->data)) {
          continue;
        }
        hash_table_push_u64_u64(scratch.arena, voff_ht, i->data, 0);
        
        // write entry
        U64 rel_off = i->data - page->voff;
        Assert(rel_off <= config->machine_page_size);
        *reloc_arr_ptr++ = PE_BaseRelocMake(PE_BaseRelocKind_DIR64, rel_off);
      }
      
      // write pad
      U64 pad_reloc_count = AlignPadPow2(total_entry_count, sizeof(reloc_arr_ptr[0]));
      MemoryZeroTyped(reloc_arr_ptr, pad_reloc_count); // fill pad with PE_BaseRelocKind_ABSOLUTE
      reloc_arr_ptr += pad_reloc_count;
      
      // compute block size
      U64 reloc_arr_size = (U64)((U8*)reloc_arr_ptr - (U8*)reloc_arr_base);
      U64 block_size     = sizeof(*page_voff_ptr) + sizeof(*block_size_ptr) + reloc_arr_size;
      
      // write header
      *page_voff_ptr  = safe_cast_u32(page->voff);
      *block_size_ptr = safe_cast_u32(block_size);
      Assert(*block_size_ptr <= buf_size);

      // push page 
      str8_list_push(arena, &result, str8(buf, buf_size));
      
      // purge voffs for next page
      hash_table_purge(voff_ht);
    }
    ProfEnd();
  }
  
  tp_temp_end(tp_temp); // scratch is cleared here
  tp_arena->v[0] = arena;

  ProfEnd();
  return result;
}

internal String8List
lnk_build_win32_header(Arena *arena, LNK_SymbolTable *symtab, LNK_Config *config, LNK_SectionArray sects, U64 expected_image_header_size)
{
  ProfBeginFunction();

  String8List result = {0};

  //
  // DOS header
  //
  U32 dos_stub_size = sizeof(PE_DosHeader) + pe_dos_program.size;
  {
    PE_DosHeader *dos_header          = push_array(arena, PE_DosHeader, 1);
    dos_header->magic                 = PE_DOS_MAGIC;
    dos_header->last_page_size        = dos_stub_size % 512;
    dos_header->page_count            = CeilIntegerDiv(dos_stub_size, 512);
    dos_header->paragraph_header_size = sizeof(PE_DosHeader) / 16;
    dos_header->min_paragraph         = 0;
    dos_header->max_paragraph         = 0;
    dos_header->init_ss               = 0;
    dos_header->init_sp               = 0;
    dos_header->checksum              = 0;
    dos_header->init_ip               = 0xFFFF;
    dos_header->init_cs               = 0;
    dos_header->reloc_table_file_off  = sizeof(PE_DosHeader);
    dos_header->overlay_number        = 0;
    MemoryZeroStruct(dos_header->reserved);
    dos_header->oem_id                = 0;
    dos_header->oem_info              = 0;
    MemoryZeroArray(dos_header->reserved2);
    dos_header->coff_file_offset      = dos_stub_size;

    str8_list_push(arena, &result, str8_struct(dos_header));
    str8_list_push(arena, &result, pe_dos_program);
  }

  //
  // PE magic
  //
  U32 *pe_magic = push_array(arena, U32, 1);
  *pe_magic = PE_MAGIC;
  str8_list_push(arena, &result, str8_struct(pe_magic));

  //
  // determine PE optional header type
  //
  B32 has_pe_plus_header = pe_has_plus_header(config->machine);

  //
  // COFF file header
  //
  {
    COFF_FileHeader *file_header      = push_array_no_zero(arena, COFF_FileHeader, 1);
    file_header->machine              = config->machine;
    file_header->time_stamp           = config->time_stamp;
    file_header->symbol_table_foff    = 0;
    file_header->symbol_count         = 0;
    file_header->section_count        = sects.count;
    file_header->optional_header_size = (has_pe_plus_header ? sizeof(PE_OptionalHeader32Plus) : sizeof(PE_OptionalHeader32)) + (sizeof(PE_DataDirectory) * config->data_dir_count);
    file_header->flags                = config->file_characteristics;
    str8_list_push(arena, &result, str8_struct(file_header));
  }

  //
  // compute code/inited/uninited sizes
  //
  U64 code_base            = 0;
  U64 sizeof_code          = 0;
  U64 sizeof_inited_data   = 0;
  U64 sizeof_uninited_data = 0;
  U64 sizeof_image         = 0;
  for (U64 sect_idx = 0; sect_idx < sects.count; sect_idx += 1) {
    LNK_Section *sect = sects.v[sect_idx];
    if (code_base == 0 && sect->flags & COFF_SectionFlag_CntCode) {
      code_base = sect->voff;
    }
    if (sect->flags & COFF_SectionFlag_CntUninitializedData) {
      sizeof_uninited_data += sect->vsize;
    }
    if ((sect->flags & COFF_SectionFlag_CntInitializedData) || (sect->flags & COFF_SectionFlag_CntCode)) {
      sizeof_inited_data += sect->fsize;
    }
    if (sect->flags & COFF_SectionFlag_CntCode) { 
      sizeof_code += sect->fsize;
    }
    sizeof_image = Max(sizeof_image, sects.v[sect_idx]->voff + sects.v[sect_idx]->vsize);
  }
  sizeof_code          = AlignPow2(sizeof_code, config->file_align);
  sizeof_inited_data   = AlignPow2(sizeof_inited_data, config->file_align);
  sizeof_uninited_data = AlignPow2(sizeof_uninited_data, config->file_align);
  sizeof_image         = AlignPow2(sizeof_image, 4096);

  //
  // compute image headers size
  //
  U64 sizeof_image_headers = 0;
  sizeof_image_headers += dos_stub_size;
  sizeof_image_headers += sizeof(COFF_FileHeader);
  sizeof_image_headers += has_pe_plus_header ? sizeof(PE_OptionalHeader32Plus) : sizeof(PE_OptionalHeader32);
  sizeof_image_headers += sizeof(PE_DataDirectory) * config->data_dir_count;
  sizeof_image_headers += sizeof(COFF_SectionHeader) * sects.count;
  sizeof_image_headers = AlignPow2(sizeof_image_headers, config->file_align);

  //
  // fill out PE optional header
  //
  U32 *entry_point_va;
  U32 *check_sum;
  if (has_pe_plus_header) {
    PE_OptionalHeader32Plus *opt_header = push_array_no_zero(arena, PE_OptionalHeader32Plus, 1);
    opt_header->magic                   = PE_PE32PLUS_MAGIC;
    opt_header->major_linker_version    = config->link_ver.major;
    opt_header->minor_linker_version    = config->link_ver.minor;
    opt_header->sizeof_code             = safe_cast_u32(sizeof_code);
    opt_header->sizeof_inited_data      = safe_cast_u32(sizeof_inited_data);
    opt_header->sizeof_uninited_data    = safe_cast_u32(sizeof_uninited_data);
    opt_header->entry_point_va          = 0;
    opt_header->code_base               = code_base;
    opt_header->image_base              = lnk_get_base_addr(config);
    opt_header->section_alignment       = config->sect_align;
    opt_header->file_alignment          = config->file_align;
    opt_header->major_os_ver            = config->os_ver.major;
    opt_header->minor_os_ver            = config->os_ver.minor;
    opt_header->major_img_ver           = config->image_ver.major;
    opt_header->minor_img_ver           = config->image_ver.minor;
    opt_header->major_subsystem_ver     = config->subsystem_ver.major;
    opt_header->minor_subsystem_ver     = config->subsystem_ver.minor;
    opt_header->win32_version_value     = 0; // MSVC writes zero
    opt_header->sizeof_image            = sizeof_image;
    opt_header->sizeof_headers          = safe_cast_u32(sizeof_image_headers);
    opt_header->check_sum               = 0; // :check_sum
    opt_header->subsystem               = config->subsystem;
    opt_header->dll_characteristics     = config->dll_characteristics;
    opt_header->sizeof_stack_reserve    = config->stack_reserve;
    opt_header->sizeof_stack_commit     = config->stack_commit;
    opt_header->sizeof_heap_reserve     = config->heap_reserve;
    opt_header->sizeof_heap_commit      = config->heap_commit;
    opt_header->loader_flags            = 0; // for dynamic linker, always zero
    opt_header->data_dir_count          = safe_cast_u32(config->data_dir_count);

    entry_point_va = &opt_header->entry_point_va;
    check_sum      = &opt_header->check_sum;

    str8_list_push(arena, &result, str8_struct(opt_header));
  } else {
    NotImplemented;
  }

  //
  // PE directories
  //
  PE_DataDirectory *directory_array;
  {
    directory_array = push_array(arena, PE_DataDirectory, config->data_dir_count);
    str8_list_push(arena, &result, str8_array(directory_array, config->data_dir_count));
  }

  //
  // COFF section table
  //
  COFF_SectionHeader *coff_section_table       = push_array(arena, COFF_SectionHeader, sects.count);
  U64                 coff_section_table_count = 0;
  {
    for (U64 sect_idx = 0; sect_idx < sects.count; sect_idx += 1) {
      LNK_Section *sect = sects.v[sect_idx];

      COFF_SectionHeader *coff_section = &coff_section_table[sect_idx];

      if (coff_section->flags & COFF_SectionFlag_LnkRemove) { continue; }

      // TODO: for objs we can store long name in string table and write here /offset
      if (sect->name.size > sizeof(coff_section->name)) {
        lnk_error(LNK_Warning_LongSectionName, "not enough space in COFF section header to store entire name \"%S\"", sect->name);
      }

      MemorySet(&coff_section->name[0], 0, sizeof(coff_section->name));
      MemoryCopy(&coff_section->name[0], sect->name.str, Min(sect->name.size, sizeof(coff_section->name)));
      coff_section->vsize       = sect->vsize;
      coff_section->voff        = sect->voff;
      coff_section->fsize       = sect->fsize;
      coff_section->foff        = sect->foff;
      coff_section->relocs_foff = 0; // not present in image
      coff_section->lines_foff  = 0; // obsolete
      coff_section->reloc_count = 0; // not present in image
      coff_section->line_count  = 0; // obsolete
      coff_section->flags       = sect->flags;

      coff_section_table_count += 1;
    }

    str8_list_push(arena, &result, str8_array(coff_section_table, coff_section_table_count));
  }

  // align image headers
  {
    U64 image_headers_align_size = AlignPadPow2(result.total_size, config->file_align);
    U8 *image_headers_align      = push_array(arena, U8, image_headers_align_size);
    str8_list_push(arena, &result, str8(image_headers_align, image_headers_align_size));
  }

  //
  // entry point
  //
  {
    Temp scratch = scratch_begin(&arena, 1);

    COFF_SectionHeader **section_table = push_array(arena, COFF_SectionHeader *, coff_section_table_count + 1);
    for (U64 i = 1; i <= coff_section_table_count; i += 1) { section_table[i] = &coff_section_table[i-1]; }

    LNK_Symbol *entry_symbol = lnk_symbol_table_search(symtab, config->entry_point_name);
    if (entry_symbol) {
      *entry_point_va = safe_cast_u32(lnk_voff_from_symbol(section_table, entry_symbol));
    }

    scratch_end(scratch);
  }

  Assert(result.total_size == expected_image_header_size);
  ProfEnd();
  return result;
}

internal LNK_ImageContext
lnk_build_image(TP_Arena *arena, TP_Context *tp, LNK_Config *config, LNK_SymbolTable *symtab, U64 objs_count, LNK_Obj **objs)
{
  ProfBegin("Image");
  lnk_timer_begin(LNK_Timer_Image);

  Temp scratch = scratch_begin(arena->v, arena->count);

  //
  // init section table
  //
  LNK_SectionTable *sectab = lnk_section_table_alloc();
  lnk_section_table_push(sectab, str8_lit(".text" ), PE_TEXT_SECTION_FLAGS );
  lnk_section_table_push(sectab, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS);
  lnk_section_table_push(sectab, str8_lit(".data" ), PE_DATA_SECTION_FLAGS );
  lnk_section_table_push(sectab, str8_lit(".bss"  ), PE_BSS_SECTION_FLAGS  );
  lnk_section_table_push(sectab, str8_lit(".pdata"), PE_PDATA_SECTION_FLAGS);
  LNK_Section *common_block_sect = lnk_section_table_search(sectab, str8_lit(".bss"), PE_BSS_SECTION_FLAGS);

  LNK_BuildImageTask task = {
    .symtab           = symtab,
    .sectab           = sectab,
    .objs_count       = objs_count,
    .objs             = objs,
    .function_pad_min = config->function_pad_min,
    .default_align    = coff_default_align_from_machine(config->machine),
    .null_sc          = push_array(arena->v[0], LNK_SectionContrib, 1),
  };

  {
    ProfBegin("Define And Count Sections");
    TP_Temp temp = tp_temp_begin(arena);

    ProfBegin("Init Hash Tables For Gathering Section Definitions");
    task.u.gather_sects.defns = push_array(arena->v[0], HashTable *, tp->worker_count);
    for EachIndex(worker_id, tp->worker_count) { task.u.gather_sects.defns[worker_id] = hash_table_init(arena->v[0], 128); }
    ProfEnd();

    tp_for_parallel_prof(tp, arena, objs_count, lnk_gather_section_definitions_task, &task, "Gather Section Definitions");

    ProfBegin("Merge Section Definitions Hash Tables");
    for (U64 worker_idx = 1; worker_idx < tp->worker_count; worker_idx += 1) {
      U64                     sect_defns_count = task.u.gather_sects.defns[worker_idx]->count;
      LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(arena->v[0], task.u.gather_sects.defns[worker_idx]);
      radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);

      for EachIndex(defn_idx, sect_defns_count) {
        LNK_SectionDefinition *defn            = sect_defns[defn_idx];
        String8                name_with_flags = lnk_make_name_with_flags(arena->v[0], defn->name, defn->flags);
        LNK_SectionDefinition *main_defn       = hash_table_search_string_raw(task.u.gather_sects.defns[0], name_with_flags);
        if (main_defn == 0) {
          main_defn = sect_defns[defn_idx];
          hash_table_push_string_raw(arena->v[0], task.u.gather_sects.defns[0], name_with_flags, main_defn);
        } else {
          if (lnk_section_definition_is_before(&sect_defns[defn_idx], &main_defn)) {
            main_defn->obj = sect_defns[defn_idx]->obj;
            main_defn->obj_sect_idx = sect_defns[defn_idx]->obj_sect_idx;
          }
          main_defn->contribs_count += sect_defns[defn_idx]->contribs_count;
        }
      }
    }
    U64                     sect_defns_count = task.u.gather_sects.defns[0]->count;
    LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(arena->v[0], task.u.gather_sects.defns[0]);
    ProfEnd();

    ProfBegin("Sort Sections Definitions");
    radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);
    ProfEnd();

    ProfBegin("Push Sections And Reserve Section Contrib Memory");
    task.contribs_ht = hash_table_init(sectab->arena, sect_defns_count);
    for EachIndex(defn_idx, sect_defns_count) {
      LNK_SectionDefinition *sect_defn = sect_defns[defn_idx];

      // parse section name
      String8 sect_name, sort_idx;
      coff_parse_section_name(sect_defn->name, &sect_name, &sort_idx);

      // do not create definitions for sections that are removed from the image
      if (lnk_is_section_removed(config, sect_name)) { continue; }

      // warn about conflicting section flags
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        if (str8_match(sect_n->data.name, sect_name, 0) && sect_n->data.flags != sect_defn->flags) {
          LNK_Obj            *obj                = sect_defn->obj;
          U32                 sect_number        = sect_defn->obj_sect_idx + 1;
          COFF_SectionHeader *sect_header        = lnk_coff_section_header_from_section_number(obj, sect_number);
          String8             sect_name          = coff_name_from_section_header(str8_substr(obj->data, obj->header.string_table_range), sect_header);
          String8             expected_flags_str = coff_string_from_section_flags(arena->v[0], sect_n->data.flags);
          String8             current_flags_str  = coff_string_from_section_flags(arena->v[0], sect_defn->flags);
          lnk_error_obj(LNK_Warning_SectionFlagsConflict, sect_defn->obj, "detected section flags conflict in %S(No. %X); expected {%S} but got {%S}", sect_name, sect_number, expected_flags_str, current_flags_str);
        }
      }

      {
        ProfBeginV("Reserve Section Contrib Chunks [%S]", sect_defn->name);

        LNK_Section *sect = lnk_section_table_search(sectab, sect_name, sect_defn->flags);
        if (!sect) {
          sect = lnk_section_table_push(sectab, sect_name, sect_defn->flags);
        }

        String8                  defn_name_with_flags = lnk_make_name_with_flags(sectab->arena, sect_defn->name, sect_defn->flags);
        LNK_SectionContribChunk *contrib_chunk        = hash_table_search_string_raw(task.contribs_ht, defn_name_with_flags);
        if (!contrib_chunk) {
          contrib_chunk = lnk_section_contrib_chunk_list_push_chunk(arena->v[0], &sect->contribs, sect_defn->contribs_count, sort_idx);
          hash_table_push_string_raw(sectab->arena, task.contribs_ht, defn_name_with_flags, contrib_chunk);
        }
        
        ProfEnd();
      }
    }
    ProfEnd();

    tp_temp_end(temp);
    ProfEnd();
  }

  U64 expected_image_header_size;
  {
    ProfBegin("Alloc Section Map");
    task.sect_map = push_array(scratch.arena, LNK_SectionContrib **, objs_count);
    for EachIndex(obj_idx, objs_count) { task.sect_map[obj_idx] = push_array(scratch.arena, LNK_SectionContrib *, objs[obj_idx]->header.section_count_no_null); }
    ProfEnd();

    tp_for_parallel_prof(tp, 0, objs_count, lnk_gather_section_contribs_task, &task, "Gather Section Contribs");

    // ensure determinism by sorting section contribs in chunks by input index
    {
      ProfBegin("Sort Section Contribs");

      U64 total_chunk_count = 0;
      {
        for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
          total_chunk_count += sect_n->data.contribs.chunk_count;
        }
      }

      {
        U64 cursor = 0;
        task.u.sort_contribs.chunks = push_array(scratch.arena, LNK_SectionContribChunk *, total_chunk_count);
        for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
          for (LNK_SectionContribChunk *chunk_n = sect_n->data.contribs.first; chunk_n != 0; chunk_n = chunk_n->next) {
            task.u.sort_contribs.chunks[cursor++] = chunk_n;
          }
        }
        Assert(cursor == total_chunk_count);
      }

      tp_for_parallel(tp, 0, total_chunk_count, lnk_sort_contribs_task, &task);

      ProfEnd();
    }

    tp_for_parallel_prof(tp, 0, objs_count, lnk_set_comdat_leaders_contribs_task, &task, "Update Section Map With COMDAT Leader Contribs");

    // build common block
    //
    // TODO: build common block in .bss and merge with .data
    U64                     common_block_contribs_count;
    LNK_CommonBlockContrib *common_block_contribs;
    {
      ProfBegin("Build Common Block");

      task.u.common_block.counts = push_array(scratch.arena, U64, tp->worker_count);
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_count_common_block_contribs_task, &task, "Count Contribs");

      ProfBegin("Push Contribs");
      common_block_contribs_count = sum_array_u64(tp->worker_count, task.u.common_block.counts);
      common_block_contribs       = push_array(scratch.arena, LNK_CommonBlockContrib, common_block_contribs_count);
      ProfEnd();

      ProfBegin("Fill Out Contribs [%Iu64]", common_block_contribs_count);
      task.u.common_block.offsets  = offsets_from_counts_array_u64(scratch.arena, task.u.common_block.counts, tp->worker_count);
      task.u.common_block.contribs = common_block_contribs;
      tp_for_parallel(tp, 0, tp->worker_count, lnk_fill_out_common_block_contribs_task, &task);
      ProfEnd();

      if (common_block_contribs_count) {
        ProfBeginV("Make Common Block [count %llu]", common_block_contribs_count);

        // sort common blocks from for tighter packing
        radsort(common_block_contribs, common_block_contribs_count, lnk_common_block_contrib_is_before);

        // compute .bss virtual size - this marks start of the common block
        lnk_finalize_section_layout(common_block_sect, config->file_align, config->function_pad_min);
        U64 common_block_cursor = common_block_sect->vsize;

        // compute and assign offsets into the common block
        for EachIndex(contrib_idx, common_block_contribs_count) {
          LNK_CommonBlockContrib *contrib = &common_block_contribs[contrib_idx];
          U32 size  = contrib->u.size;
          U32 align = Min(32, u64_up_to_pow2(size)); // link.exe caps align at 32 bytes
          common_block_cursor = AlignPow2(common_block_cursor, align);
          contrib->u.offset = common_block_cursor;
          common_block_cursor += size;
        }

        // append common block's contribution
        LNK_SectionContribChunk *common_block_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &common_block_sect->contribs, 1, str8(0,0));
        LNK_SectionContrib      *common_block_sc    = lnk_section_contrib_chunk_push(common_block_chunk, 1);
        common_block_sc->u.obj_idx              = max_U32;
        common_block_sc->u.obj_sect_idx         = max_U32;
        common_block_sc->align                  = 1;
        common_block_sc->first_data_node.next   = 0;
        common_block_sc->first_data_node.string = str8(0, common_block_cursor - common_block_sect->vsize);
        common_block_sc->last_data_node         = &common_block_sc->first_data_node;

        ProfEnd();
      }

      ProfEnd();
    }

    {
      ProfBegin("Finalize Sections Layout");

      // Grouped Sections (PE Format)
      //  "All contributions with the same object-section name are allocated contiguously in the image,
      //  and the blocks of contributions are sorted in lexical order by object-section name." 
      ProfBegin("Sort Sections");
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_sort_section_contribs(&sect_n->data);
      }
      ProfEnd();

      // merge sections
      if (config->flags & LNK_ConfigFlag_Merge) {
        lnk_section_table_merge(sectab, config->merge_list);
      }

      if (config->do_function_pad_min == LNK_SwitchState_Yes) {
        tp_for_parallel_prof(tp, arena, objs_count, lnk_flag_hotpatch_contribs_task, &task, "Flag Hotpatch Section Contribs");
      }

      // assign contribs offsets, sizes, and section indices
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_finalize_section_layout(&sect_n->data, config->file_align, config->function_pad_min);
      }

      // remove empty sections
      {
        String8List empty_sect_list = {0};
        for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
          if (sect_n->data.vsize == 0) {
            str8_list_push(scratch.arena, &empty_sect_list, sect_n->data.name);
          }
        }
        for (String8Node *name_n = empty_sect_list.first; name_n != 0; name_n = name_n->next) {
          lnk_section_table_purge(sectab, name_n->string);
        }
      }

      // assign section indices to sections
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_assign_section_index(&sect_n->data, sectab->next_sect_idx++);
      }

      // assing layout offsets and sizes to merged sections
      for (LNK_SectionNode *sect_n = sectab->merge_list.first; sect_n != 0; sect_n = sect_n->next) {
        LNK_Section        *sect         = &sect_n->data;
        LNK_SectionContrib *first_sc     = lnk_get_first_section_contrib(sect);
        LNK_SectionContrib *last_sc      = lnk_get_last_section_contrib(sect);
        U64                 last_sc_size = lnk_size_from_section_contrib(last_sc);
        sect->voff  = sect->merge_dst->voff + first_sc->u.off;
        sect->vsize = (last_sc->u.off - first_sc->u.off) + last_sc_size;
        sect->foff  = sect->merge_dst->foff + first_sc->u.off;
        sect->fsize = (last_sc->u.off - first_sc->u.off) + last_sc_size;
        lnk_assign_section_index(sect, sect->merge_dst->sect_idx);
      }

      ProfEnd();
    }

    {
      ProfBegin("Patch Symbol Tables");
      Temp temp = temp_begin(scratch.arena);

      // set up context for patch tasks
      task.u.patch_symtabs.common_block_sect     = common_block_sect;
      task.u.patch_symtabs.common_block_ranges   = tp_divide_work(temp.arena, common_block_contribs_count, tp->worker_count);
      task.u.patch_symtabs.common_block_contribs = common_block_contribs;
      task.u.patch_symtabs.was_symbol_patched    = push_array(temp.arena, B8 *, objs_count);
      for EachIndex(obj_idx, objs_count) { task.u.patch_symtabs.was_symbol_patched[obj_idx] = push_array(temp.arena, B8, objs[obj_idx]->header.symbol_count); }

      // flag debug symbols to prevent them from being patched in subsequent passes
      tp_for_parallel_prof(tp, 0, objs_count, lnk_flag_debug_symbols_task, &task, "Flag Debug Symbols");

      // patch symbols
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_comdat_leaders_task,       &task, "COMDAT Leaders"      );
      tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_patch_common_block_leaders_task, &task, "Common Block Leaders");
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_regular_symbols_task,      &task, "Regular Symbols"     );
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_common_symbols_task,       &task, "Common Symbols"      );
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_abs_symbols_task,          &task, "Absolute Symbols"    );
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_undefined_symbols_task,    &task, "Undefined Symbols"   );
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_weak_symbols_task,         &task, "Weak Symbols"        );
      tp_for_parallel_prof(tp, 0, objs_count,       lnk_patch_undefined_symbols_task,    &task, "Undefined Symbols"   );

      temp_end(temp);
      ProfEnd();
    }

    // section list -> array
    task.image_sects = lnk_section_array_from_list(scratch.arena, sectab->list);

    // assign virtual offsets to sections
    expected_image_header_size = lnk_compute_win32_image_header_size(config, task.image_sects.count);
    U64 voff_cursor = AlignPow2(expected_image_header_size + sizeof(COFF_SectionHeader), config->sect_align);
    for EachIndex(sect_idx, task.image_sects.count) { lnk_assign_section_virtual_space(task.image_sects.v[sect_idx], config->sect_align, &voff_cursor); }
    tp_for_parallel_prof(tp, 0, task.objs_count, lnk_patch_virtual_offsets_and_sizes_in_obj_section_headers_task, &task, "Patch Virtual Offsets and Sizes in Obj Section Headers");

    // build base relocs
    if (~config->flags & LNK_ConfigFlag_Fixed) {
      String8List base_relocs_data = lnk_build_base_relocs(tp, arena, config, objs_count, objs);
      if (base_relocs_data.total_size) {
        LNK_Section             *reloc          = lnk_section_table_push(sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
        LNK_SectionContribChunk *first_sc_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &reloc->contribs, 1, str8_zero());
        LNK_SectionContrib      *sc             = lnk_section_contrib_chunk_push(first_sc_chunk, 1);
        sc->first_data_node = *base_relocs_data.first;
        sc->last_data_node  = base_relocs_data.last;
        sc->align           = 1;
        sc->u.obj_idx       = max_U32;

        lnk_finalize_section_layout(reloc, config->file_align, config->function_pad_min);
        lnk_assign_section_virtual_space(reloc, config->sect_align, &voff_cursor);
        lnk_assign_section_index(reloc, sectab->next_sect_idx++);

        task.image_sects           = lnk_section_array_from_list(scratch.arena, sectab->list);
        expected_image_header_size = lnk_compute_win32_image_header_size(config, task.image_sects.count);
      }
    }

    // assign file offsets to sections
    U64 foff_cursor = AlignPow2(expected_image_header_size, config->file_align);
    for EachIndex(sect_idx, task.image_sects.count) { lnk_assign_section_file_space(task.image_sects.v[sect_idx], &foff_cursor); }
    tp_for_parallel_prof(tp, 0, task.objs_count, lnk_patch_file_offsets_and_sizes_in_obj_section_headers_task, &task, "Patch File Offsets And Sizes In Section Headers");
  }

  // build win32 image header
  {
    String8List              image_header_data     = lnk_build_win32_header(sectab->arena, symtab, config, task.image_sects, AlignPow2(expected_image_header_size, config->file_align));
    LNK_Section             *image_header_sect     = lnk_section_table_push(sectab, str8_lit(".rad_linker_image_header_section"), 0);
    LNK_SectionContribChunk *image_header_sc_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &image_header_sect->contribs, 1, str8_zero());
    LNK_SectionContrib      *image_header_sc       = lnk_section_contrib_chunk_push(image_header_sc_chunk, 1);
    image_header_sc->align           = config->file_align;
    image_header_sc->first_data_node = *image_header_data.first;
    image_header_sc->last_data_node  = image_header_data.last;
    lnk_finalize_section_layout(image_header_sect, config->file_align, config->function_pad_min);
  }

  tp_for_parallel_prof(tp, 0, task.objs_count, lnk_patch_section_symbols_task, &task, "Patch Section Symbols");

  String8 image_data = {0};
  {
    ProfBegin("Image Fill");

    LNK_SectionArray sects = lnk_section_array_from_list(scratch.arena, sectab->list);

    U64 image_size = 0;
    for EachIndex(sect_idx, sects.count) { image_size += sects.v[sect_idx]->fsize; }

    image_data.size = image_size;
    image_data.str  = push_array_no_zero(arena->v[0], U8, image_size);

    for EachIndex(sect_idx, sects.count) {
      LNK_Section *sect = sects.v[sect_idx];

      if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
        // pick fill pick
        U8 fill_byte = 0;
        if (sect->flags & COFF_SectionFlag_CntCode) {
          fill_byte = coff_code_align_byte_from_machine(config->machine);
        }

        // copy section contribution
        U64 prev_sc_opl = 0;
        for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
          for EachIndex(sc_idx, sc_chunk->count) {
            LNK_SectionContrib *sc = sc_chunk->v[sc_idx];

            // fill align bytes
            Assert(sc->u.off >= prev_sc_opl);
            U64 fill_size = sc->u.off - prev_sc_opl;
            MemorySet(image_data.str + sect->foff + prev_sc_opl, fill_byte, fill_size);
            prev_sc_opl = sc->u.off + lnk_size_from_section_contrib(sc);

            // copy contrib contents
            {
              U64 cursor = 0;
              for (String8Node *data_n = &sc->first_data_node; data_n != 0; data_n = data_n->next) {
                Assert(sc->u.off + data_n->string.size <= sect->vsize);
                MemoryCopy(image_data.str + sect->foff + sc->u.off + cursor, data_n->string.str, data_n->string.size);
                cursor += data_n->string.size;
              }
            }
          }
        }

        // fill section align bytes
        {
          U64 fill_size = sect->fsize - prev_sc_opl;
          MemorySet(image_data.str + sect->foff + prev_sc_opl, fill_byte, fill_size);
        }
      }
    }

    ProfEnd();
  }

  {
    ProfBegin("Image Patch");

    PE_BinInfo           pe                  = pe_bin_info_from_data(scratch.arena, image_data);
    COFF_SectionHeader **image_section_table = coff_section_table_from_data(scratch.arena, image_data, pe.section_table_range);

    // patch relocs
    {
      LNK_ObjRelocPatcher task = { .image_data = image_data, .objs = objs, .image_base = pe.image_base, .image_section_table = image_section_table };
      tp_for_parallel_prof(tp, 0, objs_count, lnk_obj_reloc_patcher, &task, "Patch Relocs");
    }

    // patch load config
    {
      LNK_Symbol *load_config_symbol = lnk_symbol_table_search(symtab, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME));
      if (load_config_symbol) {
        U64     load_config_foff   = lnk_foff_from_symbol(image_section_table, load_config_symbol);
        String8 load_config_data   = str8_skip(image_data, load_config_foff);

        U32 load_config_size = 0;
        if (sizeof(load_config_size) <= load_config_data.size) {
          PE_DataDirectory *load_config_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_LOAD_CONFIG);
          load_config_dir->virt_off  = lnk_voff_from_symbol(image_section_table, load_config_symbol);
          load_config_dir->virt_size = load_config_size;
        } else {
          // TODO: report corrupted load config
        }
      }
    }

    // patch exceptions
    {
      LNK_Section *pdata_sect = lnk_section_table_search(sectab, str8_lit(".pdata"), PE_PDATA_SECTION_FLAGS);
      if (pdata_sect) {
        String8 raw_pdata = str8_substr(image_data, rng_1u64(pdata_sect->foff, pdata_sect->foff + pdata_sect->vsize));
        pe_pdata_sort(config->machine, raw_pdata);

        PE_DataDirectory *pdata_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_EXCEPTIONS);
        pdata_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, pdata_sect);
        pdata_dir->virt_size = lnk_get_section_contrib_size(pdata_sect);
      }
    }

    // patch export
    {
      LNK_Section *edata_sect = lnk_section_table_search(sectab, str8_lit(".edata"), PE_EDATA_SECTION_FLAGS);
      if (edata_sect) {
        PE_DataDirectory   *export_dir          = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_EXPORT);
        LNK_SectionContrib *edata_first_contrib = lnk_get_first_section_contrib(edata_sect);
        LNK_SectionContrib *edata_last_contrib  = lnk_get_last_section_contrib(edata_sect);
        export_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, edata_sect);
        export_dir->virt_size = lnk_get_section_contrib_size(edata_sect);
      }
    }

    // patch base relocs
    {
      LNK_Section *reloc_sect = lnk_section_table_search(sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
      if (reloc_sect) {
        PE_DataDirectory *reloc_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_BASE_RELOC);
        reloc_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, reloc_sect);
        reloc_dir->virt_size = lnk_get_section_contrib_size(reloc_sect);
      }
    }

    // patch import and import addr
    {
      LNK_Section *idata_sect       = lnk_section_table_search(sectab, str8_lit(".idata"), PE_IDATA_SECTION_FLAGS);
      LNK_Symbol  *null_import_desc = lnk_symbol_table_searchf(symtab, "__NULL_IMPORT_DESCRIPTOR");
      LNK_Symbol  *null_thunk_data  = lnk_symbol_table_searchf(symtab, "\x7f%S_NULL_THUNK_DATA", lnk_get_image_name(config));
      if (idata_sect && null_import_desc && null_thunk_data) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_from_symbol(null_import_desc);
        LNK_SectionContrib *idata_first_contrib     = lnk_get_first_section_contrib(idata_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_IMPORT);
        import_dir->virt_off  = image_section_table[idata_first_contrib->u.sect_idx + 1]->voff + idata_first_contrib->u.off;
        import_dir->virt_size = null_import_desc_parsed.value - idata_first_contrib->u.off;

        COFF_ParsedSymbol  null_thunk_data_parsed = lnk_parsed_from_symbol(null_thunk_data);
        U64                null_thunk_data_voff   = image_section_table[null_thunk_data_parsed.section_number]->voff + null_thunk_data_parsed.value;
        U64                first_import_foff      = image_section_table[idata_first_contrib->u.sect_idx+1]->foff + idata_first_contrib->u.off;
        PE_ImportEntry    *first_import           = str8_deserial_get_raw_ptr(image_data, first_import_foff, sizeof(*first_import));
        PE_DataDirectory  *import_addr_dir        = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_IMPORT_ADDR);
        import_addr_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, idata_sect);
        import_addr_dir->virt_size = null_thunk_data_voff - first_import->import_addr_table_voff /* null */ + coff_word_size_from_machine(config->machine);
      }
    }

    // patch delay imports
    {
      LNK_Section *didat_sect       = lnk_section_table_search(sectab, str8_lit(".didat"), PE_IDATA_SECTION_FLAGS);
      LNK_Symbol  *null_import_desc = lnk_symbol_table_search(symtab, str8_lit("__NULL_DELAY_IMPORT_DESCRIPTOR"));
      LNK_Symbol  *last_null_thunk  = lnk_symbol_table_searchf(symtab,"\x7f%S_NULL_THUNK_DATA_DLA", lnk_get_image_name(config));
      if (didat_sect && null_import_desc && last_null_thunk) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_from_symbol(null_import_desc);
        LNK_SectionContrib *didat_first_contrib     = lnk_get_first_section_contrib(didat_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_DELAY_IMPORT);
        import_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, didat_sect);
        import_dir->virt_size = lnk_get_section_contrib_size(didat_sect);
      }
    }

    // patch TLS
    {
      LNK_Symbol *tls_used_symbol = lnk_symbol_table_searchf(symtab, MSCRT_TLS_SYMBOL_NAME);
      if (tls_used_symbol) {
        ProfBegin("Patch TLS");

        // find max align in .tls
        U64          tls_align = 0;
        LNK_Section *tls_sect  = lnk_section_table_search(sectab, str8_lit(".tls"), PE_TLS_SECTION_FLAGS);
        for (LNK_SectionContribChunk *sc_chunk = tls_sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
          for EachIndex (sc_idx, sc_chunk->count) {
            Assert(IsPow2(sc_chunk->v[sc_idx]->align));
            tls_align = Max(tls_align, sc_chunk->v[sc_idx]->align);
          }
        }

        // patch-in align
        U64 tls_header_foff = lnk_foff_from_symbol(image_section_table, tls_used_symbol);
        B32 is_tls_header64 = coff_word_size_from_machine(config->machine) == 8;
        if (is_tls_header64) {
          PE_TLSHeader64 *tls_header = str8_deserial_get_raw_ptr(image_data, tls_header_foff, sizeof(*tls_header));
          tls_header->characteristics |= coff_section_flag_from_align_size(tls_align);
        } else {
          PE_TLSHeader32 *tls_header = str8_deserial_get_raw_ptr(image_data, tls_header_foff, sizeof(*tls_header));
          tls_header->characteristics |= coff_section_flag_from_align_size(tls_align);
        }

        // patch directory
        PE_DataDirectory *tls_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_TLS);
        tls_dir->virt_off  = lnk_voff_from_symbol(image_section_table, tls_used_symbol);
        tls_dir->virt_size = is_tls_header64 ? sizeof(PE_TLSHeader64) : sizeof(PE_TLSHeader32);

        ProfEnd();
      }
    }

    // patch debug
    {
      LNK_Section *debug_dir_sect = lnk_section_table_search(sectab, str8_lit(".RAD_LINK_PE_DEBUG_DIR"), PE_RDATA_SECTION_FLAGS);
      if (debug_dir_sect) {
        // patch directory
        PE_DataDirectory *debug_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_DEBUG);
        debug_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, debug_dir_sect);
        debug_dir->virt_size = lnk_get_section_contrib_size(debug_dir_sect);

        // find debug directory begin and end pair
        LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(debug_dir_sect);
        LNK_SectionContrib *last_sc  = lnk_get_last_section_contrib(debug_dir_sect);
        U64 debug_begin_foff = lnk_foff_from_section_contrib(image_section_table, first_sc);
        U64 debug_end_fopl   = lnk_fopl_from_section_contrib(image_section_table, last_sc);

        // patch file offsets to the debug directories
        for (U64 cursor = debug_begin_foff; cursor + sizeof(PE_DebugDirectory) <= debug_end_fopl; cursor += sizeof(PE_DebugDirectory)) {
          PE_DebugDirectory *dir = str8_deserial_get_raw_ptr(image_data, cursor, sizeof(PE_DebugDirectory));
          for (U64 section_number = 1; section_number < pe.section_count+1; section_number += 1) {
            if (image_section_table[section_number]->voff <= dir->voff && dir->voff < image_section_table[section_number]->voff + image_section_table[section_number]->vsize) {
              dir->foff = image_section_table[section_number]->foff + (dir->voff - image_section_table[section_number]->voff);
            }
          }
        }
      }
    }

    // patch resources
    {
      LNK_Section *rsrc_sect = lnk_section_table_search(sectab, str8_lit(".rsrc"), PE_RSRC_SECTION_FLAGS);
      if (rsrc_sect) {
        PE_DataDirectory *rsrc_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_RESOURCES);
        rsrc_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, rsrc_sect);
        rsrc_dir->virt_size = lnk_get_section_contrib_size(rsrc_sect);
      }
    }

    // image checksum
    if (config->flags & LNK_ConfigFlag_WriteImageChecksum) {
      ProfBegin("Image Checksum");
      *pe.check_sum = pe_compute_checksum(image_data.str, image_data.size);
      ProfEnd();
    }

    // compute image guid, and patch PDB and RDI guids
    {
      LNK_Symbol *guid_pdb_symbol = lnk_symbol_table_search(symtab, str8_lit("RAD_LINK_PE_DEBUG_GUID_PDB"));
      LNK_Symbol *guid_rdi_symbol = lnk_symbol_table_search(symtab, str8_lit("RAD_LINK_PE_DEBUG_GUID_RDI"));

      if (guid_pdb_symbol || guid_rdi_symbol) {
        switch (config->guid_type) {
        case LNK_DebugInfoGuid_Null: break;
        case Lnk_DebugInfoGuid_ImageBlake3: {
          ProfBegin("Hash Image With Blake3");
          U128 hash = lnk_blake3_hash_parallel(tp, 128, image_data);
          MemoryCopy(&config->guid, hash.u8, sizeof(hash.u8));
          ProfEnd();
        } break;
        }
      }

      if (guid_pdb_symbol) {
        U64   cv_guid_foff = lnk_foff_from_symbol(image_section_table, guid_pdb_symbol);
        Guid *cv_guid  = str8_deserial_get_raw_ptr(image_data, cv_guid_foff, sizeof(*cv_guid));
        *cv_guid = config->guid;
      }

      if (guid_rdi_symbol) {
        U64   cv_guid_foff = lnk_foff_from_symbol(image_section_table, guid_rdi_symbol);
        Guid *cv_guid  = str8_deserial_get_raw_ptr(image_data, cv_guid_foff, sizeof(*cv_guid));
        *cv_guid = config->guid;
      }
    }
    
    ProfEnd();
  }

  LNK_ImageContext image_ctx = {0};
  image_ctx.image_data       = image_data;
  image_ctx.sectab           = sectab;

  lnk_timer_end(LNK_Timer_Image);
  ProfEnd(); // :EndImage
  scratch_end(scratch);
  return image_ctx;
}

internal PairU32 *
lnk_obj_sect_idx_from_section(Arena *arena, U64 objs_count, LNK_Obj **objs, LNK_Section *sect, LNK_Config *config, U64 *obj_sect_idxs_count_out)
{
  U64 max_contribs = 0;
  for (LNK_SectionContribChunk *chunk = sect->contribs.first; chunk != 0; chunk = chunk->next) {
    max_contribs += chunk->count;
  }

  U64      obj_sect_idxs_count = 0;
  PairU32 *obj_sect_idxs       = push_array(arena, PairU32, max_contribs);
  for (U64 obj_idx = 0; obj_idx < objs_count; obj_idx += 1) {
    LNK_Obj *obj = objs[obj_idx];
    COFF_SectionHeader *section_table = str8_deserial_get_raw_ptr(obj->data, obj->header.section_table_range.min, 0);
    String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);
    for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
      COFF_SectionHeader *section_header    = &section_table[sect_idx];
      String8             full_section_name = coff_name_from_section_header(string_table, section_header);
      String8             section_name, section_postfix;
      coff_parse_section_name(full_section_name, &section_name, &section_postfix);

      if (section_header->flags & COFF_SectionFlag_LnkRemove) { continue; }
      if (section_header->fsize == 0)                         { continue; }
      if (lnk_is_section_removed(config, section_name))       { continue; }

      if (sect->voff <= section_header->voff && section_header->voff < sect->voff + sect->vsize) {
        Assert(obj_sect_idxs_count < max_contribs);
        obj_sect_idxs[obj_sect_idxs_count].v0 = obj_idx;
        obj_sect_idxs[obj_sect_idxs_count].v1 = sect_idx;
        obj_sect_idxs_count += 1;
      }
    }
  }

  U64 pop_size = (max_contribs - obj_sect_idxs_count) * sizeof(obj_sect_idxs[0]);
  arena_pop(arena, pop_size);

  *obj_sect_idxs_count_out = obj_sect_idxs_count;

  return obj_sect_idxs;
}

internal COFF_SectionHeader *
lnk_coff_section_header_from_obj_sect_idx_pair(LNK_Obj **objs, PairU32 p)
{
  LNK_Obj            *obj           = objs[p.v0];
  COFF_SectionHeader *section_table = str8_deserial_get_raw_ptr(obj->data, obj->header.section_table_range.min, 0);
  return &section_table[p.v1];
}

global LNK_Obj **g_rad_map_objs;

internal int
lnk_obj_sect_idx_is_before(void *raw_a, void *raw_b)
{
  PairU32 *a = raw_a, *b = raw_b;
  COFF_SectionHeader *section_header_a = lnk_coff_section_header_from_obj_sect_idx_pair(g_rad_map_objs, *a);
  COFF_SectionHeader *section_header_b = lnk_coff_section_header_from_obj_sect_idx_pair(g_rad_map_objs, *b);
  return section_header_a->voff < section_header_b->voff;
}

internal U64
lnk_pair_u32_nearest_section(PairU32 *arr, U64 count, LNK_Obj **objs, U32 voff)
{
  U64 result = max_U64;

  if (count > 0) {
    COFF_SectionHeader *first = lnk_coff_section_header_from_obj_sect_idx_pair(objs, arr[0]);
    if (first->voff == voff) {
      return 0;
    }

    COFF_SectionHeader *last = lnk_coff_section_header_from_obj_sect_idx_pair(objs, arr[count-1]);
    if (last->voff <= voff) {
      return count - 1;
    }

    if (first->voff <= voff && voff < last->voff + last->vsize) {
      U64 l = 0;
      U64 r = count - 1;
      for (; l <= r; ) {
        U64 m = l + (r - l) / 2;
        COFF_SectionHeader *s = lnk_coff_section_header_from_obj_sect_idx_pair(objs, arr[m]);
        if (s->voff == voff) {
          return m;
        } else if (s->voff < voff) {
          l = m + 1;
        } else {
          r = m - 1;
        }
      }
      result = l;
    }
  }

  return result;
}

internal String8List
lnk_build_rad_map(Arena *arena, String8 image_data, LNK_Config *config, U64 objs_count, LNK_Obj **objs, U64 libs_count, LNK_Lib **libs, LNK_SectionTable *sectab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  PE_BinInfo           pe                  = pe_bin_info_from_data(scratch.arena, image_data);
  COFF_SectionHeader **image_section_table = coff_section_table_from_data(scratch.arena, image_data, pe.section_table_range);

  String8List map = {0};

  ProfBegin("SECTIONS");
  str8_list_pushf(arena, &map, "# SECTIONS\n");
  for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
    LNK_Section *sect = &sect_n->data;

    str8_list_pushf(arena, &map, "%S\n", sect->name);
    str8_list_pushf(arena, &map, "%-4s %-8s %-8s %-8s %-8s %-16s %-4s %s\n", "No.", "VirtOff", "VirtSize", "FileOff", "FileSize", "Blake3", "Algn", "SC");

    U64      obj_sect_idxs_count = 0;
    PairU32 *obj_sect_idxs       = lnk_obj_sect_idx_from_section(scratch.arena, objs_count, objs, sect, config, &obj_sect_idxs_count);
    g_rad_map_objs = objs;
    radsort(obj_sect_idxs, obj_sect_idxs_count, lnk_obj_sect_idx_is_before);

    U64 global_sc_idx = 0;
    for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
      for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1, global_sc_idx += 1) {
        Temp temp = temp_begin(scratch.arena);
        LNK_SectionContrib *sc = sc_chunk->v[sc_idx];

        U64        file_off   = image_section_table[sc->u.sect_idx+1]->foff + sc->u.off;
        U64        virt_off   = image_section_table[sc->u.sect_idx+1]->voff + sc->u.off;
        U64        virt_size  = lnk_size_from_section_contrib(sc);
        U64        file_size  = lnk_size_from_section_contrib(sc);
        String8    sc_data    = str8_substr(image_data, rng_1u64(file_off, file_off + virt_size));

        LNK_Obj *obj      = 0;
        U32      sect_idx = 0;
        U64 obj_sect_idx_idx = lnk_pair_u32_nearest_section(obj_sect_idxs, obj_sect_idxs_count, objs, virt_off);
        if (obj_sect_idx_idx < obj_sect_idxs_count) {
          obj      = objs[obj_sect_idxs[obj_sect_idx_idx].v0];
          sect_idx = obj_sect_idxs[obj_sect_idx_idx].v1;
        }

        U128 sc_hash = {0};
        if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
          blake3_hasher hasher; blake3_hasher_init(&hasher);
          blake3_hasher_update(&hasher, sc_data.str, sc_data.size);
          blake3_hasher_finalize(&hasher, (U8 *)&sc_hash, sizeof(sc_hash));
        }

        String8 sc_idx_str    = push_str8f(temp.arena, "%4llx",      global_sc_idx);
        String8 virt_size_str = push_str8f(temp.arena, "%08x",       virt_size);
        String8 sc_hash_str   = (~sect->flags & COFF_SectionFlag_CntUninitializedData) ? push_str8f(temp.arena, "%08x%08x",   sc_hash.u64[0], sc_hash.u64[1]) : str8_lit("--------");
        String8 file_off_str  = (~sect->flags & COFF_SectionFlag_CntUninitializedData) ? push_str8f(temp.arena, "%08x", file_off)  : str8_lit("--------");
        String8 file_size_str = (~sect->flags & COFF_SectionFlag_CntUninitializedData) ? push_str8f(temp.arena, "%08x", file_size) : str8_lit("--------");
        String8 virt_off_str  = push_str8f(temp.arena, "%08x",       virt_off);
        String8 align_str     = push_str8f(temp.arena, "%4x",        sc->align);
        String8 contrib_str;
        {
          String8List source_list = {0};
          if (obj) {
            COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, sect_idx+1);
            String8             string_table   = str8_substr(obj->data, obj->header.string_table_range);
            String8             section_name   = coff_name_from_section_header(string_table, section_header);
            LNK_Lib            *lib            = lnk_obj_get_lib(obj);
            if (lib) {
              String8 lib_name = str8_chop_last_dot(str8_skip_last_slash(lib->path));
              String8 obj_name = str8_skip_last_slash(obj->path);
              str8_list_pushf(temp.arena, &source_list, "%S(%S) SECT%X (%S)", lib_name, obj_name, sect_idx+1, section_name);
            } else {
              str8_list_pushf(temp.arena, &source_list, "%S SECT%X (%S)", obj->path, sect_idx+1, section_name);
            }
          } else {
            str8_list_pushf(temp.arena, &source_list, "<no_loc>");
          }
          contrib_str = str8_list_join(temp.arena, &source_list, &(StringJoin){.sep=str8_lit(" ")});
        }

        str8_list_pushf(arena, &map, "%S %S %S %S %S %S %S %S\n", sc_idx_str, virt_off_str, virt_size_str, file_off_str, file_size_str, sc_hash_str, align_str, contrib_str);

        temp_end(temp);
      }
    }
    str8_list_pushf(arena, &map, "\n");
  }
  ProfEnd();

  str8_list_pushf(arena, &map, "# DEBUG\n");
  for (U64 obj_idx = 0; obj_idx < objs_count; obj_idx += 1) {
    LNK_Obj            *obj           = objs[obj_idx];
    COFF_SectionHeader *section_table = str8_deserial_get_raw_ptr(obj->data, obj->header.section_table_range.min, 0);
    for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
      COFF_SectionHeader *section_header = &section_table[sect_idx];
      if (~section_header->flags & COFF_SectionFlag_LnkRemove && section_header->flags & LNK_SECTION_FLAG_DEBUG_INFO) {
        LNK_Lib *lib = lnk_obj_get_lib(obj);
        if (lib) {
          String8 lib_name = str8_chop_last_dot(str8_skip_last_slash(lib->path));
          String8 obj_name = str8_skip_last_slash(obj->path);
          str8_list_pushf(arena, &map, "%S(%S) SECT%X\n", lib_name, obj_name, sect_idx+1);
        } else {
          str8_list_pushf(arena, &map, "%S SECT%X\n", obj->path, sect_idx+1);
        }
      }
    }
  }
  str8_list_pushf(arena, &map, "\n");

  ProfBegin("LIBS");
  if (libs_count) {
    str8_list_pushf(arena, &map, "# LIBS\n");
    for EachIndex(i, libs_count) {
      str8_list_pushf(arena, &map, "%S\n", libs[i]->path);
    }
  }
  ProfEnd();
 
  scratch_end(scratch);
  ProfEnd();
  return map;
}

internal void
lnk_write_thread(void *raw_ctx)
{
  ProfBeginFunction();
  LNK_WriteThreadContext *ctx = raw_ctx;
  lnk_write_data_to_file_path(ctx->path, ctx->temp_path, ctx->data);
  ProfEnd();
}

internal void
lnk_log_timers(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  U64 total_build_time_micro = 0;
  for (U64 i = 0; i < LNK_Timer_Count; ++i) {
    total_build_time_micro += g_timers[i].end - g_timers[i].begin;
  }
  
  String8List output_list = {0};
  str8_list_pushf(scratch.arena, &output_list, "------ Link Times --------------------------------------------------------------");
  for (U64 i = 0; i < LNK_Timer_Count; ++i) {
    U64 build_time_micro = g_timers[i].end - g_timers[i].begin;
    if (build_time_micro != 0) {
      String8  timer_name = lnk_string_from_timer_type(i);
      DateTime time       = date_time_from_micro_seconds(build_time_micro);
      String8  time_str   = string_from_elapsed_time(scratch.arena, time);
      str8_list_pushf(scratch.arena, &output_list, "  %-5S Time: %S", timer_name, time_str);
    }
  }
  
  DateTime total_time = date_time_from_micro_seconds(total_build_time_micro);
  String8 total_time_str = string_from_elapsed_time(scratch.arena, total_time);
  str8_list_pushf(scratch.arena, &output_list, "  Total Time: %S", total_time_str);
  
  StringJoin new_line_join = { str8_lit_comp(""), str8_lit_comp("\n"), str8_lit_comp("") };
  String8 output = str8_list_join(scratch.arena, &output_list, &new_line_join);
  lnk_log(LNK_Log_Timers, "%S\n", output);
  
  scratch_end(scratch);
}

internal void
lnk_run(TP_Context *tp, TP_Arena *arena, LNK_Config *config)
{
  ProfBeginFunction();

  Temp scratch = scratch_begin(arena->v, arena->count);

  //
  // Input Context
  //
  LNK_Inputer *inputer = lnk_inputer_init();

  //
  // Symbol Table
  //
  LNK_SymbolTable *symtab = lnk_symbol_table_init(arena);

  //
  // Link Image
  //
  LNK_Link *link = lnk_link_image(tp, arena, config, inputer, symtab);

  U64       objs_count = link->objs.count;
  U64       libs_count = link->libs.count;
  LNK_Obj **objs       = lnk_array_from_obj_list(scratch.arena, link->objs);
  LNK_Lib **libs       = lnk_array_from_lib_list(scratch.arena, link->libs);

  //
  // Layout Image
  //
  LNK_ImageContext image_ctx = lnk_build_image(arena, tp, config, symtab, objs_count, objs);

  // Write image in the background
  LNK_WriteThreadContext *image_write_ctx = push_array(scratch.arena, LNK_WriteThreadContext, 1);
  image_write_ctx->path      = config->image_name;
  image_write_ctx->temp_path = config->temp_image_name;
  image_write_ctx->data      = image_ctx.image_data;
  OS_Handle image_write_thread = os_thread_launch(lnk_write_thread, image_write_ctx, 0);

  //
  // RAD Map
  //
  if (config->rad_chunk_map == LNK_SwitchState_Yes) {
    String8List rad_map = lnk_build_rad_map(scratch.arena, image_ctx.image_data, config, objs_count, objs, libs_count, libs, image_ctx.sectab);
    lnk_write_data_list_to_file_path(config->rad_chunk_map_name, config->temp_rad_chunk_map_name, rad_map);
  }

  //
  // Import Library
  //
  if (config->build_imp_lib && (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL)) {
    ProfBegin("Build Import Library");
    lnk_timer_begin(LNK_Timer_Lib);
    String8 linker_debug_symbols = lnk_make_linker_debug_symbols(scratch.arena, config->machine);
    String8 lib                  = pe_make_import_lib(arena->v[0], config->machine, config->time_stamp, str8_skip_last_slash(config->image_name), linker_debug_symbols, config->export_symbol_list);
    lnk_write_data_to_file_path(config->imp_lib_name, str8_zero(), lib);
    lnk_timer_end(LNK_Timer_Lib);
    ProfEnd();
  }

  //
  // Debug Info
  //
  if (lnk_do_debug_info(config)) {
    ProfBegin("Debug Info");
    lnk_timer_begin(LNK_Timer_Debug);

    U64       debug_info_objs_count = 0;
    LNK_Obj **debug_info_objs       = push_array(scratch.arena, LNK_Obj *, objs_count);
    for EachIndex(obj_idx, objs_count) {
      LNK_Obj *obj = objs[obj_idx];
      if (obj->exclude_from_debug_info) { continue; }
      debug_info_objs[debug_info_objs_count++] = obj;
    }

    //
    // CodeView
    //
    LNK_CodeViewInput input = lnk_make_code_view_input(tp, arena, config->io_flags, config->lib_dir_list, debug_info_objs_count, debug_info_objs);
    CV_DebugT        *types = lnk_import_types(tp, arena, &input);

    //
    // RDI
    //
    if (config->rad_debug == LNK_SwitchState_Yes) {
      lnk_timer_begin(LNK_Timer_Rdi);

      String8List rdi_data = lnk_build_rad_debug_info(tp,
                                                      arena,
                                                      config->target_os,
                                                      rdi_arch_from_coff_machine(config->machine),
                                                      config->image_name,
                                                      image_ctx.image_data,
                                                      input.count,
                                                      input.obj_arr,
                                                      input.debug_s_arr,
                                                      input.total_symbol_input_count,
                                                      input.symbol_inputs,
                                                      input.parsed_symbols,
                                                      types);

      lnk_write_data_list_to_file_path(config->rad_debug_name, config->temp_rad_debug_name, rdi_data);

      lnk_timer_end(LNK_Timer_Rdi);
    }

    //
    // PDB
    //
    // TODO: Parallel debug info builds are currently blocked by the patch
    // strings in $$FILE_CHECKSUM step in `lnk_process_c13_data_task`.
    if (config->debug_mode == LNK_DebugMode_Full) {
      lnk_timer_begin(LNK_Timer_Pdb);

      if (config->pdb_hash_type_names != LNK_TypeNameHashMode_Null && config->pdb_hash_type_names != LNK_TypeNameHashMode_None) {
        lnk_replace_type_names_with_hashes(tp, arena, types[CV_TypeIndexSource_TPI], config->pdb_hash_type_names, config->pdb_hash_type_name_length, config->pdb_hash_type_name_map);
      }

      String8List pdb_data = lnk_build_pdb(tp,
                                           arena,
                                           image_ctx.image_data,
                                           config,
                                           symtab,
                                           input.count,
                                           input.obj_arr,
                                           input.debug_s_arr,
                                           input.total_symbol_input_count,
                                           input.symbol_inputs,
                                           input.parsed_symbols,
                                           types);

      lnk_write_data_list_to_file_path(config->pdb_name, config->temp_pdb_name, pdb_data);
      lnk_timer_end(LNK_Timer_Pdb);
    }

    lnk_timer_end(LNK_Timer_Debug);
    ProfEnd();
  }

  // wait for the thread to finish writing image to disk
  os_thread_join(image_write_thread, -1);

  //
  // Timers
  //
  if (lnk_get_log_status(LNK_Log_Timers)) {
    lnk_log_timers();
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);
  lnk_init_error_handler();
  LNK_Config *config   = lnk_config_from_argcv(scratch.arena, cmdline->argc, cmdline->argv);
  TP_Context *tp       = tp_alloc(scratch.arena, config->worker_count, config->max_worker_count, config->shared_thread_pool_name);
  TP_Arena   *tp_arena = tp_arena_alloc(tp);
  lnk_run(tp, tp_arena, config);
  scratch_end(scratch);
}

