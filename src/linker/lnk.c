// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Build Options

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD PE/COFF Linker"

////////////////////////////////

#define ARENA_FREE_LIST 1

////////////////////////////////
// Third Party

#include "base_ext/base_blake3.h"
#include "base_ext/base_blake3.c"
#include "third_party/md5/md5.c"
#include "third_party/md5/md5.h"
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"
#include "third_party/radsort/radsort.h"

////////////////////////////////
// Code Base

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Winitializer-overrides"
# pragma clang diagnostic ignored "-Wswitch"
#endif

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "path/path.h"
#include "hash_table.h"
#include "coff/coff.h"
#include "coff/coff_enum.h"
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
#include "codeview/codeview_enum.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "msvc_crt/msvc_crt.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "path/path.c"
#include "hash_table.c"
#include "coff/coff.c"
#include "coff/coff_enum.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "coff/coff_lib_writer.c"
#include "pe/pe.c"
#include "pe/pe_make_import_table.c"
#include "pe/pe_make_export_table.c"
#include "pe/pe_make_debug_dir.c"
#include "codeview/codeview.c"
#include "codeview/codeview_enum.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "msvc_crt/msvc_crt.c"

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

////////////////////////////////
// RDI

#include "rdi/rdi_overrides.h"
#include "lib_rdi_format/rdi_format.h"
#include "rdi/rdi.h"
#include "lib_rdi_format/rdi_format.c"
#include "rdi/rdi.c"

////////////////////////////////
// Code Base Extensions

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

////////////////////////////////
// RDI Builder

#include "rdi/rdi_builder.h"
#include "rdi/rdi_coff.h" 
#include "rdi/rdi_cv.h"

#include "rdi/rdi_builder.c"
#include "rdi/rdi_coff.c"
#include "rdi/rdi_cv.c"

////////////////////////////////
// Linker

#include "lnk_error.h"
#include "lnk_log.h"
#include "lnk_timer.h"
#include "lnk_io.h"
#include "lnk_cmd_line.h"
#include "lnk_input.h"
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
#include "lnk_input.c"
#include "lnk_config.c"
#include "lnk_symbol_table.c"
#include "lnk_section_table.c"
#include "lnk_obj.c"
#include "lnk_debug_helper.c"
#include "lnk_lib.c"
#include "lnk_debug_info.c"

////////////////////////////////

global read_only LNK_SectionContrib g_null_sc;

////////////////////////////////

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
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_HighEntropyVa,                 "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_ManifestUac,                   "\"level='asInvoker' uiAccess='false'\"");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_NxCompat,                      "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_FunctionPadMin,                "0");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_LargeAddressAware,             "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_PdbAltPath,                    "%%_RAD_PDB_PATH%%");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_PdbPageSize,                   "%u", KB(4));
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_TimeStamp,                 "%u", os_get_process_start_time_unix());
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Age,                       "%u", 1);
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_CheckUnusedDelayLoadDll,   "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_DelayBind,                 "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_DoMerge,                   "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_EnvLib,                    "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Exe,                       "");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Guid,                      "imageblake3");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_LargePages,                "no");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_LinkVer,                   "14.0");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_OsVer,                     "6.0");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_PageSize,                  "%u", KB(4));
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_PathStyle,                 "system");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SectVirtOff,               "0x1000");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_Workers,                   "%u", os_get_system_info()->logical_processor_count);
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_TargetOs,                  "windows");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SymbolTableCapDefined,     "0x3ffff");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SymbolTableCapInternal,    "0x1000");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SymbolTableCapWeak,        "0x3ffff");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_SymbolTableCapLib,         "0x3ffff");
  lnk_cmd_line_push_option_if_not_presentf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_DebugAltPath,              "%%_RAD_RDI_PATH%%");
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
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".edata=.rdata");
  //lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".idata=.rdata");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".didat=.data");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".RAD_LINK_PE_DEBUG_DIR=.rdata");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Merge, ".RAD_LINK_PE_DEBUG_DATA=.rdata");

  // sections to remove from the image
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".debug");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gehcont");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gfids");
  lnk_cmd_line_push_optionf(scratch.arena, &cmd_line, LNK_CmdSwitch_Rad_RemoveSection, ".gxfg");

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
  LNK_Config *config = lnk_config_from_cmd_line(arena, raw_cmd_line, cmd_line);

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
    // resolve relativ path inputs
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
    manifest_data = lnk_read_data_from_file_path(arena, merged_manifest_path);
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

internal
THREAD_POOL_TASK_FUNC(lnk_load_thin_objs_task)
{
  LNK_InputObj *input = ((LNK_InputObj **)raw_task)[task_id];
  if (input->is_thin) {
    input->data = lnk_read_data_from_file_path(arena, input->path);
    input->has_disk_read_failed = (input->data.size == 0);
  }
}

internal String8
lnk_get_lib_name(String8 path)
{
  static String8 LIB_EXT = str8_lit_comp(".LIB");
  
  // strip path
  String8 name = str8_skip_last_slash(path);
  
  // strip extension
  String8 name_ext = str8_postfix(name, LIB_EXT.size);
  if (str8_match(name_ext, LIB_EXT, StringMatchFlag_CaseInsensitive)) {
    name = str8_chop(name, LIB_EXT.size);
  }
  
  return name;
}

internal B32
lnk_is_lib_disallowed(HashTable *disallow_lib_ht, String8 path)
{
  String8 lib_name = lnk_get_lib_name(path);
  return hash_table_search_path(disallow_lib_ht, lib_name) != 0;
}

internal B32
lnk_is_lib_loaded(HashTable *loaded_lib_ht, String8 path)
{
  KeyValuePair *is_loaded = hash_table_search_path(loaded_lib_ht, path);
  return is_loaded != 0;
}

internal void
lnk_push_disallow_lib(Arena *arena, HashTable *disallow_lib_ht, String8 path)
{
  String8 lib_name = lnk_get_lib_name(path);
  hash_table_push_path_u64(arena, disallow_lib_ht, lib_name, 0);
}

internal void
lnk_push_loaded_lib(Arena *arena, HashTable *loaded_lib_ht, String8 path)
{
  if (!hash_table_search_path(loaded_lib_ht, path)) {
    String8 path_copy = push_str8_copy(arena, path);
    hash_table_push_path_u64(arena, loaded_lib_ht, path_copy, 0);
  }
}

internal void
lnk_push_export(Arena *arena, HashTable *export_ht, PE_ExportParseList *export_list, String8List *include_symbol_list, PE_ExportParse export_parse)
{
  PE_ExportParseNode *exp_n = 0;
  String8 export_name = pe_name_from_export_parse(&export_parse);
  hash_table_search_string_raw(export_ht, export_name, &exp_n);

  if (exp_n == 0) {
    // make sure export is defined
    if (!export_parse.is_forwarder) {
      str8_list_push(arena, include_symbol_list, export_parse.name);
    }

    // push new export
    exp_n = pe_export_parse_list_push(arena, export_list, export_parse);

    hash_table_push_string_raw(arena, export_ht, export_name, exp_n);
  } else {
    B32 is_ambiguous = 1;
    PE_ExportParse *extant_export = &exp_n->data;
    
    if (extant_export->alias.size && export_parse.alias.size && !str8_match(extant_export->alias, export_parse.alias, 0)) {
      goto report;
    }

    if (extant_export->ordinal != export_parse.ordinal) {
      goto report;
    }

    is_ambiguous = 0;

    if (extant_export->alias.size == 0 && export_parse.alias.size != 0) {
      extant_export->alias = export_parse.alias;
    }

    report:;
    if (is_ambiguous) {
      lnk_error_with_loc(LNK_Error_IllExport, export_parse.obj_path, export_parse.lib_path, "ambiguous symbol export %S", export_parse.name);
    }
  }
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

internal void
lnk_queue_lib_member_input(Arena               *arena,
                           PathStyle            path_style,
                           LNK_SymbolLib       *symbol,
                           LNK_InputImportList *input_import_list,
                           LNK_InputObjList    *input_obj_list)
{
  LNK_Lib *lib = symbol->lib;
  U64 input_idx = Compose64Bit(lib->input_idx, symbol->member_offset);

  // parse member
  COFF_ArchiveMember member_info = coff_archive_member_from_offset(lib->data, symbol->member_offset);
  COFF_DataType      member_type = coff_data_type_from_data(member_info.data);
  
  switch (member_type) {
  case COFF_DataType_Null: break;
  case COFF_DataType_Import: {
    LNK_InputImportNode *input = lnk_input_import_list_push(arena, input_import_list);
    input->data.coff_import = member_info.data;
    input->data.input_idx   = input_idx;
  } break;
  case COFF_DataType_BigObj:
  case COFF_DataType_Obj: {
    String8 obj_path = coff_parse_long_name(lib->long_names, member_info.header.name);

    // obj path in thin archive has slash appended which screws up 
    // file lookup on disk; it couble be there to enable paths to symbols
    // but we don't use this feature
    String8 slash = str8_lit("/");
    if (str8_ends_with(obj_path, slash, 0)) {
      obj_path = str8_chop(obj_path, slash.size);
    }

    // obj path in thin archive is relative to directory with archive
    B32 is_thin = lib->type == COFF_Archive_Thin;
    if (is_thin) {
      Temp scratch = scratch_begin(&arena, 1);
      String8List obj_path_list = {0};
      str8_list_push(scratch.arena, &obj_path_list, str8_chop_last_slash(lib->path));
      str8_list_push(scratch.arena, &obj_path_list, obj_path);
      obj_path = str8_path_list_join_by_style(arena, &obj_path_list, path_style);
      scratch_end(scratch);
    }

    LNK_InputObj *input = lnk_input_obj_list_push(arena, input_obj_list);
    input->is_thin      = is_thin;
    input->dedup_id     = push_str8f(arena, "%S/%S", lib->path, obj_path);
    input->path         = obj_path;
    input->data         = member_info.data;
    input->lib_path     = lib->path;
    input->input_idx    = input_idx;
  } break;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_undef_symbol_finder)
{
  LNK_SymbolFinder       *task   = raw_task;
  LNK_SymbolFinderResult *result = &task->result_arr[task_id];
  Rng1U64                 range  = task->range_arr[task_id];
  
  for (U64 symbol_idx = range.min; symbol_idx < range.max; symbol_idx += 1) {
    LNK_SymbolNode *symbol_n = task->lookup_node_arr.v[symbol_idx];
    LNK_Symbol     *symbol   = symbol_n->data;
    
    LNK_Symbol *has_defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol->name);
    if (has_defn) {
      continue;
    }
    LNK_Symbol *has_import = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Import, symbol->name);
    if (has_import) {
      continue;
    }
    
    LNK_Symbol *member_symbol = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Lib, symbol->name);
    if (member_symbol) {
      lnk_queue_lib_member_input(arena, task->path_style, &member_symbol->u.lib, &result->input_import_list, &result->input_obj_list);
    } else {
      lnk_symbol_list_push_node(&result->unresolved_symbol_list, symbol_n);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_weak_symbol_finder)
{
  LNK_SymbolFinder       *task   = raw_task;
  LNK_SymbolFinderResult *result = &task->result_arr[task_id];
  Rng1U64                 range  = task->range_arr[task_id];
  
  for (U64 symbol_idx = range.min; symbol_idx < range.max; symbol_idx += 1) {
    LNK_SymbolNode *symbol_n = task->lookup_node_arr.v[symbol_idx];
    LNK_Symbol     *symbol   = symbol_n->data;
    
    LNK_Symbol *has_strong_defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol->name);
    if (has_strong_defn) {
      continue;
    }
    LNK_Symbol *has_import = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Import, symbol->name);
    if (has_import) {
      continue;
    }
 
    LNK_Symbol *member_symbol = 0;
    {
      COFF_ParsedSymbol   parsed_symbol = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
      COFF_SymbolWeakExt *weak_ext      = coff_parse_weak_tag(parsed_symbol, symbol->u.defined.obj->header.is_big_obj);
      switch (weak_ext->characteristics) {
      case COFF_WeakExt_NoLibrary: {
        // NOLIBRARY means weak symbol should be resolved in case where strong definition pulls in lib member.
      } break;
      case COFF_WeakExt_SearchLibrary: {
        member_symbol = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Lib, symbol->name);
      } break;
      case COFF_WeakExt_SearchAlias: {
        member_symbol = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Lib, symbol->name);
        if (member_symbol == 0) {
          if (str8_match_lit(".weak.", symbol->name, StringMatchFlag_RightSideSloppy)) {
            // TODO: Clang and MingGW encode extra info in alias
            // 
            // __attribute__((weak,alias("foo"))) void bar(void);
            // static void foo() {}
            //
            // Clang write these COFF symbols in obj for code above:
            //
            // 30 00000000 0000000001 0    FUNC NULL EXTERNAL         foo
            // ...
            // 33 00000000 UNDEF      1    NULL NULL WEAK_EXTERNAL    bar
            // Tag Index 35, Characteristics SEARCH_ALIAS
            // 35 00000000 0000000001 0    NULL NULL EXTERNAL         .weak.bar.default.foo
            //
            // In this case linker needs to parse .weak.bar.default.foo and search for bar and foo as well.
            Assert("TODO: MinGW weak symbol");
          } else {
            COFF_ParsedSymbol tag = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, weak_ext->tag_index);
            member_symbol = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Lib, tag.name);
          }
        }
      } break;
      }
    }
    
    if (member_symbol) {
      lnk_queue_lib_member_input(arena, task->path_style, &member_symbol->u.lib, &result->input_import_list, &result->input_obj_list);
    } else {
      lnk_symbol_list_push_node(&result->unresolved_symbol_list, symbol_n);
    }
  }
}

internal LNK_SymbolFinderResult
lnk_run_symbol_finder(TP_Context      *tp,
                      TP_Arena        *arena,
                      LNK_Config      *config,
                      LNK_SymbolTable *symtab,
                      LNK_SymbolList   lookup_list,
                      TP_TaskFunc     *task_func)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);
  
  ProfBegin("Setup Task");
  LNK_SymbolFinder task  = {0};
  task.path_style        = config->path_style;
  task.symtab            = symtab;
  task.lookup_node_arr   = lnk_symbol_node_array_from_list(scratch.arena, lookup_list);
  task.result_arr        = push_array(scratch.arena, LNK_SymbolFinderResult, tp->worker_count);
  task.range_arr         = tp_divide_work(scratch.arena, task.lookup_node_arr.count, tp->worker_count);
  ProfEnd();
  
  ProfBegin("Run Task");
  tp_for_parallel(tp, arena, tp->worker_count, task_func, &task);
  ProfEnd();
  
  ProfBegin("Concat Results");
  LNK_SymbolFinderResult result = {0};
  for (U64 i = 0; i < tp->worker_count; ++i) {
    LNK_SymbolFinderResult *src = &task.result_arr[i];
    lnk_symbol_list_concat_in_place(&result.unresolved_symbol_list, &src->unresolved_symbol_list);
    lnk_input_obj_list_concat_in_place(&result.input_obj_list, &src->input_obj_list);
    lnk_input_import_list_concat_in_place(&result.input_import_list, &src->input_import_list);
  }
  ProfEnd();
  
  // to get deterministic output accross multiple linker runs we have to sort inputs
  ProfBegin("Sort Objs [Count %llu]", result.input_obj_list.count);
  LNK_InputObj **input_obj_ptr_arr = lnk_array_from_input_obj_list(scratch.arena, result.input_obj_list);
  qsort(input_obj_ptr_arr, result.input_obj_list.count, sizeof(input_obj_ptr_arr[0]), lnk_input_obj_compar);
  //radsort(input_obj_ptr_arr, result.input_obj_list.count, lnk_input_obj_compar_is_before);
  result.input_obj_list = lnk_list_from_input_obj_arr(input_obj_ptr_arr, result.input_obj_list.count);
  ProfEnd();
  
  ProfBegin("Sort Imports [Count %llu]", result.input_import_list.count);
  LNK_InputImportNode **input_imp_ptr_arr = lnk_input_import_arr_from_list(scratch.arena, result.input_import_list);
  //radsort(input_imp_ptr_arr, result.input_import_list.count, lnk_input_import_is_before);
  qsort(input_imp_ptr_arr, result.input_import_list.count, sizeof(input_imp_ptr_arr[0]), lnk_input_import_node_compar);
  result.input_import_list = lnk_list_from_input_import_arr(input_imp_ptr_arr, result.input_import_list.count);
  ProfEnd();
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal LNK_LinkContext
lnk_build_link_context(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config)
{
  enum State {
    State_Null,
    State_InputDisallowLibs,
    State_InputImports,
    State_InputSymbols,
    State_InputObjs,
    State_InputLibs,
    State_PushDllHelperUndefSymbol,
    State_InputLinkerObjs,
    State_PushLoadConfigUndefSymbol,
    State_LookupUndef,
    State_LookupWeak,
    State_LookupEntryPoint,
    State_ReportUnresolvedSymbols,
  };
  struct StateNode {
    struct StateNode *next;
    enum   State      state;
  };
  struct StateList {
    U64               count;
    struct StateNode *first;
    struct StateNode *last;
  };
  
#define state_list_push(a, l, s) do {                          \
  struct StateNode *node = push_array(a, struct StateNode, 1); \
  node->state = s;                                             \
  SLLQueuePush(l.first, l.last, node);                         \
  l.count += 1;                                                \
} while (0)
#define state_list_pop(l) (l).first->state; SLLQueuePop((l).first, (l).last); (l).count -= 1
  
  ProfBeginFunction();
  
  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);
  
  // inputs
  String8List            include_symbol_list      = config->include_symbol_list;
  String8List            input_disallow_lib_list  = config->disallow_lib_list;
  String8List            input_manifest_path_list = str8_list_copy(scratch.arena, &config->input_list[LNK_Input_Manifest]);
  String8List            manifest_dep_list        = str8_list_copy(scratch.arena, &config->manifest_dependency_list);
  PE_ExportParseList     export_symbol_list       = {0};
  HashTable             *export_ht                = hash_table_init(scratch.arena, max_U16/2);
  LNK_InputObjList       input_obj_list           = {0};
  LNK_InputImportList    input_import_list        = {0};
  LNK_SymbolList         input_weak_list          = {0};
  LNK_InputLibList       input_libs[LNK_InputSource_Count] = {
    config->input_list[LNK_Input_Lib], // command line libs
    config->input_default_lib_list     // default libs
  };
  
  // :null_obj
  lnk_input_obj_list_push(scratch.arena, &input_obj_list);
  
  // input command line objs
  LNK_InputObjList cmd_line_obj_inputs = lnk_input_obj_list_from_string_list(scratch.arena, config->input_list[LNK_Input_Obj]);
  lnk_input_obj_list_concat_in_place(&input_obj_list, &cmd_line_obj_inputs);

  // state
  LNK_SymbolTable      *symtab                           = lnk_symbol_table_init(tp_arena);
  LNK_SectionTable     *sectab                           = 0;
  HashTable            *static_imports                   = hash_table_init(scratch.arena, 512);
  HashTable            *delayed_imports                  = hash_table_init(scratch.arena, 512);
  LNK_ObjList           obj_list                         = {0};
  LNK_LibList           lib_index[LNK_InputSource_Count] = {0};
  Arena                *ht_arena                         = arena_alloc();
  String8               delay_load_helper_name           = {0};
  HashTable            *disallow_lib_ht                  = hash_table_init(scratch.arena, 0x100);
  HashTable            *delay_load_dll_ht                = hash_table_init(scratch.arena, 0x100);
  HashTable            *loaded_lib_ht                    = hash_table_init(scratch.arena, 0x100);
  HashTable            *missing_lib_ht                   = hash_table_init(scratch.arena, 0x100);
  HashTable            *loaded_obj_ht                    = hash_table_init(scratch.arena, 0x4000);
  LNK_SymbolList        lookup_undef_list                = {0};
  LNK_SymbolList        lookup_weak_list                 = {0};
  LNK_SymbolList        unresolved_undef_list            = {0};
  LNK_SymbolList        unresolved_weak_list             = {0};
  U64                   entry_point_lookup_attempts      = 0;
  B32                   report_unresolved_symbols        = 1;
  B32                   input_linker_objs                = 1;

  //
  // Init state machine
  //
  struct StateList state_list = {0};
  state_list_push(scratch.arena, state_list, State_InputDisallowLibs);
  state_list_push(scratch.arena, state_list, State_InputObjs);
  state_list_push(scratch.arena, state_list, State_InputLibs);
  if (config->delay_load_dll_list.node_count) {
    for (String8Node *delay_load_dll_node = config->delay_load_dll_list.first;
         delay_load_dll_node != 0;
         delay_load_dll_node = delay_load_dll_node->next) {
      hash_table_push_path_u64(scratch.arena, delay_load_dll_ht, delay_load_dll_node->string, 0);
    }
    state_list_push(scratch.arena, state_list, State_PushDllHelperUndefSymbol);
  }
  if (config->guard_flags != LNK_Guard_None) {
    state_list_push(scratch.arena, state_list, State_PushLoadConfigUndefSymbol);
  }

  //
  // Push config exports
  //
  for (PE_ExportParseNode *exp_n = config->export_symbol_list.first; exp_n != 0; exp_n = exp_n->next) {
    lnk_push_export(tp_arena->v[0], export_ht, &export_symbol_list, &include_symbol_list, exp_n->data);
  }

  //
  // Push config alternative names
  //
  for (LNK_AltNameNode *alt_n = config->alt_name_list.first; alt_n != 0; alt_n = alt_n->next) {
    lnk_symbol_table_push_alt_name(symtab, 0, alt_n->data.from, alt_n->data.to);
  }
  
  // run states
  for (;;) {
    while (state_list.count) {
      enum State state = state_list_pop(state_list);
      switch (state) {
      case State_Null: break;

      case State_InputDisallowLibs: {
        ProfBegin("Input /disallowlib");
        
        for (String8Node *name_n = input_disallow_lib_list.first; name_n != 0; name_n = name_n->next) {
          if ( ! lnk_is_lib_disallowed(disallow_lib_ht, name_n->string)) {
            lnk_push_disallow_lib(scratch.arena, disallow_lib_ht, name_n->string);
          }
        }
        
        // reset input
        MemoryZeroStruct(&input_disallow_lib_list);
        
        ProfEnd();
      } break;
      case State_InputImports: {
        ProfBegin("Input Imports");
        for (LNK_InputImportNode *input = input_import_list.first; input != 0; input = input->next) {
          COFF_ParsedArchiveImportHeader import_header = coff_archive_import_from_data(input->data.coff_import);
          
          // import machine compat check
          if (import_header.machine != config->machine) {
            lnk_error(LNK_Error_IncompatibleMachine, "symbol %S pulled in import with incompatible machine %S (expected %S)",
                      import_header.func_name,
                      coff_string_from_machine_type(import_header.machine),
                      coff_string_from_machine_type(config->machine));
            continue;
          }

          // was import already created?
          if (lnk_symbol_table_search(symtab, LNK_SymbolScope_Import, import_header.func_name)) {
            continue;
          }

          // create import stubs (later replaced with acutal imports generated by linker)
          LNK_Symbol *thunk_symbol = lnk_make_import_symbol(scratch.arena, import_header.func_name, input->data.coff_import);
          LNK_Symbol *imp_symbol = lnk_make_import_symbol(scratch.arena, push_str8f(scratch.arena, "__imp_%S", import_header.func_name), input->data.coff_import);
          lnk_symbol_table_push(symtab, thunk_symbol);
          lnk_symbol_table_push(symtab, imp_symbol);

          // pick imports hash table
          HashTable *imports_ht;
          {
            B32 is_delay_load_dll = hash_table_search_path_u64(delay_load_dll_ht, import_header.dll_name, 0);
            if (is_delay_load_dll) {
              imports_ht = delayed_imports;
            } else {
              imports_ht = static_imports;
            }
          }
          
          // search DLL symbol list
          String8List *import_symbols = hash_table_search_path_raw(imports_ht, import_header.dll_name);
          if (import_symbols == 0) {
            import_symbols = push_array(scratch.arena, String8List, 1);
            hash_table_push_path_raw(scratch.arena, imports_ht, import_header.dll_name, import_symbols);
          }
          
          // push symbol
          str8_list_push(scratch.arena, import_symbols, input->data.coff_import);
        }
        
        // reset input
        MemoryZeroStruct(&input_import_list);
        
        ProfEnd();
      } break;
      case State_InputSymbols: {
        ProfBegin("Input Symbols");
        
        ProfBegin("Push /INCLUDE Symbols");
        for (String8Node *include_node = include_symbol_list.first; include_node != 0; include_node = include_node->next) {
          String8     name   = push_str8_copy(symtab->arena->v[0], include_node->string);
          LNK_Symbol *symbol = lnk_make_undefined_symbol(symtab->arena->v[0], name, 0);
          lnk_symbol_list_push(scratch.arena, &lookup_undef_list, symbol);
        }
        ProfEnd();

        // we defined new symbols, give unresolved symbols another chance to be resolved
        lnk_symbol_list_concat_in_place(&lookup_undef_list, &unresolved_undef_list);
        lnk_symbol_list_concat_in_place(&lookup_weak_list, &input_weak_list);
        lnk_symbol_list_concat_in_place(&lookup_weak_list, &unresolved_weak_list);
        
        // reset inputs
        MemoryZeroStruct(&include_symbol_list);
        MemoryZeroStruct(&input_weak_list);
        
        ProfEnd();
      } break;
      case State_InputObjs: {
        ProfBegin("Input Objs [Count %llu]", input_obj_list.count);
        
        ProfBegin("Collect Obj Paths");
        LNK_InputObjList unique_obj_input_list = {0};
        for (LNK_InputObj *input = input_obj_list.first, *next; input != 0; input = next) {
          next = input->next;
          
          B32 was_obj_loaded = hash_table_search_path_u64(loaded_obj_ht, input->dedup_id, 0);
          if (was_obj_loaded) {
            continue;
          }
          
          String8 full_path          = os_full_path_from_path(scratch.arena, input->dedup_id);
          B32     was_full_path_used = hash_table_search_path_u64(loaded_obj_ht, full_path, 0);
          if (was_full_path_used) {
            continue;
          }
          
          hash_table_push_path_u64(scratch.arena, loaded_obj_ht, input->dedup_id, 0);
          if (!str8_match(input->dedup_id, full_path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive)) {
            hash_table_push_path_u64(scratch.arena, loaded_obj_ht, full_path, 0);
          }
          
          lnk_input_obj_list_push_node(&unique_obj_input_list, input);
          lnk_log(LNK_Log_InputObj, "Input Obj: %S", full_path);
        }
        ProfEnd();
        
        ProfBegin("Load Objs From Disk");
        LNK_InputObj **input_obj_arr = lnk_array_from_input_obj_list(scratch.arena, unique_obj_input_list);
        tp_for_parallel(tp, tp_arena, unique_obj_input_list.count, lnk_load_thin_objs_task, input_obj_arr);
        ProfEnd();
        
        ProfBegin("Disk Read Check");
        for (U64 input_idx = 0; input_idx < unique_obj_input_list.count; ++input_idx) {
          if (input_obj_arr[input_idx]->has_disk_read_failed) {
            lnk_error(LNK_Error_InvalidPath, "unable to find obj \"%S\"", input_obj_arr[input_idx]->path);
          }
        }
        ProfEnd();
        
        if (lnk_get_log_status(LNK_Log_InputObj)) {
          U64 input_size = 0;
          for (U64 i = 0; i < unique_obj_input_list.count; ++i) { input_size += input_obj_arr[i]->data.size; }
          lnk_log(LNK_Log_InputObj, "[ Obj Input Size %M ]", input_size);
        }
        
        LNK_ObjNodeArray obj_node_arr = lnk_obj_list_push_parallel(tp, tp_arena, &obj_list, config->machine, unique_obj_input_list.count, input_obj_arr);

        //
        // if the machine was omitted on the command line, derive machine from obj
        //
        if (config->machine == COFF_MachineType_Unknown) {
          for (U64 obj_idx = 0; obj_idx < obj_node_arr.count; obj_idx += 1) {
            if (obj_node_arr.v[obj_idx].data.header.machine != COFF_MachineType_Unknown) {
              config->machine = obj_node_arr.v[obj_idx].data.header.machine;
              break;
            }
          }
        }

        ProfBegin("Handle Directives");
        for (U64 obj_idx = 0; obj_idx < obj_node_arr.count; obj_idx += 1) {
          LNK_Obj *obj = &obj_node_arr.v[obj_idx].data;

          LNK_DirectiveInfo directive_info = {0};
          {
            COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;

            String8List drectve_data = {0};
            for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
              COFF_SectionHeader *sect_header = &section_table[sect_idx];
              if (sect_header->flags & COFF_SectionFlag_LnkInfo) {
                String8 sect_name = str8_cstring_capped(sect_header->name, sect_header->name + sizeof(sect_header->name));
                if (str8_match(sect_name, str8_lit(".drectve"), 0)) {
                  if (sect_header->flags & COFF_SectionFlag_CntUninitializedData) {
                    lnk_error_obj(LNK_Error_IllData, obj, ".drectve section header has flag COFF_SectionFlag_CntUninitializedData");
                    break;
                  }
                  if (sect_header->fsize < 3) {
                    lnk_error_obj(LNK_Error_IllData, obj, "not enough bytes to parse .drectve");
                    break;
                  }
                  if (sect_header->reloc_count > 0) {
                    lnk_error_obj(LNK_Error_IllData, obj, ".drectve must not have relocations");
                    break;
                  }
                  Rng1U64 sect_range = rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize);
                  str8_list_push(scratch.arena, &drectve_data, str8_substr(obj->data, sect_range));
                }
              }
            }

            for (String8Node *drectve_n = drectve_data.first; drectve_n != 0; drectve_n = drectve_n->next) {
              lnk_parse_msvc_linker_directive(scratch.arena, obj, &directive_info, drectve_n->string);
            }
          }

          // /EXPORT
          {
            for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Export].first; dir != 0; dir = dir->next) {
              PE_ExportParse export_parse = {0};
              lnk_parse_export_directive_ex(scratch.arena, dir->value_list, obj->path, obj->lib_path, &export_parse);
              lnk_push_export(tp_arena->v[0], export_ht, &export_symbol_list, &include_symbol_list, export_parse);
            }
          }

          // /INCLUDESYMBOL
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Include].first; dir != 0; dir = dir->next) {
            str8_list_concat_in_place(&include_symbol_list, &dir->value_list);
          }

          // /MERGE
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Merge].first; dir != 0; dir = dir->next) {
            for (String8Node *value_n = dir->value_list.first; value_n != 0; value_n = value_n->next) {
              LNK_MergeDirective merge_dir;
              if (lnk_parse_merge_directive(value_n->string, &merge_dir)) {
                merge_dir.src = push_str8_copy(tp_arena->v[0], merge_dir.src);
                merge_dir.dst = push_str8_copy(tp_arena->v[0], merge_dir.dst);
                lnk_merge_directive_list_push(tp_arena->v[0], &config->merge_list, merge_dir);
              } else {
                lnk_error_obj(LNK_Warning_IllData, obj, "can't parse merge directive \"%S\"", value_n->string);
              }
            }
          }

          // /MANIFESTDEPENDENCY
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_ManifestDependency].first; dir != 0; dir = dir->next) {
            str8_list_concat_in_place(&manifest_dep_list, &dir->value_list);
          }

          // /DISALLOWLIB
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_DisallowLib].first; dir != 0; dir = dir->next) {
            str8_list_concat_in_place(&input_disallow_lib_list, &dir->value_list);
          }

          // /DEFAULTLIB
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_DefaultLib].first; dir != 0; dir = dir->next) {
            str8_list_concat_in_place(&input_libs[LNK_InputSource_Obj], &dir->value_list);
          }

          // /ALTERNATENAME
          {
            for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_AlternateName].first; dir != 0; dir = dir->next) {
              for (String8Node *string_n = dir->value_list.first; string_n != 0; string_n = string_n->next) {
                LNK_AltName alt_name;
                if (lnk_parse_alt_name_directive(string_n->string, &alt_name)) {
                  alt_name.from = push_str8_copy(tp_arena->v[0], alt_name.from);
                  alt_name.to = push_str8_copy(tp_arena->v[0], alt_name.to);
                  lnk_symbol_table_push_alt_name(symtab, obj, alt_name.from, alt_name.to);
                } else {
                  lnk_error_obj(LNK_Error_Cmdl, obj, "syntax error in \"%S\", expected format \"FROM=TO\"", string_n->string);
                }
              }
            }
          }

          // /ENTRY
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Entry].first; dir != 0; dir = dir->next) {
            lnk_apply_cmd_option_to_config(scratch.arena, config, dir->id, dir->value_list, obj->path, obj->lib_path);
          }

          // /SUBSYSTEM
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_SubSystem].first; dir != 0; dir = dir->next) {
            lnk_apply_cmd_option_to_config(scratch.arena, config, dir->id, dir->value_list, obj->path, obj->lib_path);
          }

          // /STACK
          for (LNK_Directive *dir = directive_info.v[LNK_CmdSwitch_Stack].first; dir != 0; dir = dir->next) {
            lnk_apply_cmd_option_to_config(scratch.arena, config, dir->id, dir->value_list, obj->path, obj->lib_path);
          }
        }
        ProfEnd();

        LNK_SymbolInputResult input_result = lnk_input_obj_symbols(tp, tp_arena, symtab, obj_node_arr);
        
        // schedule symbol input
        lnk_symbol_list_concat_in_place(&input_weak_list, &input_result.weak_symbols);
        lnk_symbol_list_concat_in_place(&lookup_undef_list, &input_result.undef_symbols);

        // give another chance to unresolved symbols
        lnk_symbol_list_concat_in_place(&lookup_undef_list, &unresolved_undef_list);
        lnk_symbol_list_concat_in_place(&input_weak_list, &unresolved_weak_list);
        
        // reset input objs
        MemoryZeroStruct(&input_obj_list);
        
        ProfEnd();
      } break;
      case State_InputLibs: {
        ProfBegin("Input Libs");

        // input libs from command line only
        U64 input_source_opl = ArrayCount(input_libs);
        if (config->no_default_libs) {
          input_source_opl = LNK_InputSource_Default;
        }
        
        for (U64 input_source = 0; input_source < ArrayCount(input_libs); ++input_source) {
          ProfBeginV("Input Source %S", lnk_string_from_input_source(input_source));

          Temp             temp                  = temp_begin(scratch.arena);
          LNK_InputLibList input_lib_list        = input_libs[input_source];
          LNK_InputLibList unique_input_lib_list = {0};

          ProfBegin("Collect unique input libs");
          for (LNK_InputLib *input = input_lib_list.first; input != 0; input = input->next) {
            String8 path = input->string;

            if (input_source == LNK_InputSource_Default || input_source == LNK_InputSource_Obj) {
              if (!str8_ends_with(path, str8_lit(".lib"), StringMatchFlag_CaseInsensitive)) {
                path = push_str8f(temp.arena, "%S.lib", path);
              }
              if (lnk_is_lib_disallowed(disallow_lib_ht, path)) {
                continue;
              }
            }

            if (lnk_is_lib_loaded(loaded_lib_ht, path)) {
              continue;
            }
            
            // search disk for library
            String8List match_list = lnk_file_search(temp.arena, config->lib_dir_list, path);

            // warn about missing lib
            if (match_list.node_count == 0) {
              KeyValuePair *was_reported = hash_table_search_path(missing_lib_ht, path);
              if (was_reported == 0) {
                hash_table_push_path_u64(ht_arena, missing_lib_ht, path, 0);
                lnk_error(LNK_Warning_FileNotFound, "unable to find library `%S`", path);
              }
              continue;
            }

            // pick first match
            String8 full_path = str8_list_first(&match_list);
            
            if (lnk_is_lib_loaded(loaded_lib_ht, full_path)) {
              continue;
            }
            
            // warn about multiple matches
            if (match_list.node_count > 1) {
              lnk_error(LNK_Warning_MultipleLibMatch, "multiple libs match `%S` (picking first match)", path);
              lnk_supplement_error_list(match_list);
            }
            
            // push library for loading
            str8_list_push(temp.arena, &unique_input_lib_list, full_path);

            // save paths for future checks
            lnk_push_loaded_lib(ht_arena, loaded_lib_ht, path);
            lnk_push_loaded_lib(ht_arena, loaded_lib_ht, full_path);
            
            lnk_log(LNK_Log_InputLib, "Input Lib: %S", full_path);
          }
          ProfEnd();
          
          ProfBegin("Disk Read Libs");
          String8Array paths = str8_array_from_list(temp.arena, &unique_input_lib_list);
          String8Array datas = lnk_read_data_from_file_path_parallel(tp, tp_arena->v[0], paths);
          ProfEnd();
          
          ProfBegin("Lib Init");
          LNK_LibNodeArray libs = lnk_lib_list_push_parallel(tp, tp_arena, &lib_index[input_source], datas, paths);
          ProfEnd();

          lnk_input_lib_symbols(tp, symtab, libs);
          
          if (lnk_get_log_status(LNK_Log_InputLib)) {
            if (libs.count > 0) {
              U64 input_size = 0;
              for (U64 i = 0; i < libs.count; ++i) { input_size += libs.v[i].data.data.size; }
              lnk_log(LNK_Log_InputObj, "[ Lib Input Size %M ]", input_size);
            }
          }

          temp_end(temp);
          ProfEnd();
        }
        
        // reset input libs
        MemoryZeroArray(input_libs);
        
        ProfEnd();
      } break;
      case State_PushDllHelperUndefSymbol: {
        ProfBegin("Push Dll Helper Undef Symbol");
        
        switch (config->machine) {
        case COFF_MachineType_X86: delay_load_helper_name = str8_cstring(MSCRT_DELAY_LOAD_HELPER2_X86_SYMBOL_NAME); break;
        case COFF_MachineType_X64: delay_load_helper_name = str8_cstring(MSCRT_DELAY_LOAD_HELPER2_SYMBOL_NAME);     break;
        default: { NotImplemented; } break;
        }
        
        str8_list_push(scratch.arena, &include_symbol_list, delay_load_helper_name);
        ProfEnd();
      } break;
      case State_PushLoadConfigUndefSymbol: {
        ProfBegin("Push Load Config Undef Symbol");
        String8 load_config_name = str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME);
        str8_list_push(scratch.arena, &include_symbol_list, load_config_name);
        ProfEnd();
      } break;
      case State_LookupUndef: {
        ProfBegin("Lookup Undefined Symbols");

        // search archives
        LNK_SymbolFinderResult result = lnk_run_symbol_finder(tp, tp_arena, config, symtab, lookup_undef_list, lnk_undef_symbol_finder); // TODO: put these on temp arena
        
        // new inputs found
        input_obj_list    = result.input_obj_list;
        input_import_list = result.input_import_list;
        
        // undefined symbols that weren't resolved
        lnk_symbol_list_concat_in_place(&unresolved_undef_list, &result.unresolved_symbol_list);
        
        // reset input
        MemoryZeroStruct(&lookup_undef_list);

        ProfEnd();
      } break;
      case State_LookupWeak: {
        ProfBegin("Lookup Weak Symbols");

        // search archives
        LNK_SymbolFinderResult result = lnk_run_symbol_finder(tp, tp_arena, config, symtab, lookup_weak_list, lnk_weak_symbol_finder); // TODO: put these on temp arena
        
        // schedule new inputs
        input_obj_list    = result.input_obj_list;
        input_import_list = result.input_import_list;
        
        // weak symbols that weren't resolved
        lnk_symbol_list_concat_in_place(&unresolved_weak_list, &result.unresolved_symbol_list);
        
        // reset input
        MemoryZeroStruct(&lookup_weak_list);

        ProfEnd();
      } break;
      case State_LookupEntryPoint: {
        ProfBegin("Lookup Entry Point");
        LNK_Symbol *entry_point_symbol = 0;
        
        B32 is_entry_point_unspecified = config->entry_point_name.size == 0;
        if (is_entry_point_unspecified) {
          if (config->subsystem == PE_WindowsSubsystem_UNKNOWN) {
            // we don't have a subsystem and entry point name,
            // so we loop over every subsystem and search potential entry
            // points in the symbol table 
            for (U64 subsys_idx = 0; subsys_idx < PE_WindowsSubsystem_COUNT; subsys_idx += 1) {
              String8Array name_arr  = pe_get_entry_point_names(config->machine, (PE_WindowsSubsystem)subsys_idx, config->file_characteristics);
              for (U64 entry_idx = 0; entry_idx < name_arr.count; entry_idx += 1) {
                entry_point_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, name_arr.v[entry_idx]);
                if (entry_point_symbol) {
                  config->subsystem = (PE_WindowsSubsystem)subsys_idx;
                  goto dbl_break;
                }
              }
            }
            
            // search for potential entry points in libs
            if (!entry_point_symbol) {
              for (U64 subsys_idx = 0; subsys_idx < PE_WindowsSubsystem_COUNT; subsys_idx += 1) {
                String8Array name_arr = pe_get_entry_point_names(config->machine, (PE_WindowsSubsystem)subsys_idx, config->file_characteristics);
                for (U64 entry_idx = 0; entry_idx < name_arr.count; entry_idx += 1) {
                  entry_point_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Lib, name_arr.v[entry_idx]);
                  if (entry_point_symbol) {
                    config->subsystem = (PE_WindowsSubsystem)subsys_idx;
                    goto dbl_break;
                  }
                }
              }
            } 
            
            dbl_break:;
          } else {
            // we have subsystem but no entry point name, get potential entry point names
            // and see which is in the symbol table
            String8Array name_arr = pe_get_entry_point_names(config->machine, config->subsystem, config->file_characteristics);
            for (U64 entry_idx = 0; entry_idx < name_arr.count; entry_idx += 1) {
              LNK_Symbol *symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, name_arr.v[entry_idx]);
              if (symbol) {
                if (entry_point_symbol) {
                  lnk_error(LNK_Error_EntryPoint,
                            "multiple entry point symbols found: %S(%S) and %S(%S)",
                            entry_point_symbol->name, entry_point_symbol->u.defined.obj->path,
                            symbol->name, symbol->u.defined.obj->path);
                } else {
                  entry_point_symbol = symbol;
                }
              }
            }
            
            // search for entry point in libs
            if (!entry_point_symbol) {
              for (U64 entry_idx = 0; entry_idx < name_arr.count; entry_idx += 1) {
                entry_point_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Lib, name_arr.v[entry_idx]);
                if (entry_point_symbol) {
                  break;
                }
              }
            }
          }
          
          // redirect user entry to appropriate CRT entry
          if (entry_point_symbol) {
            config->entry_point_name = entry_point_symbol->name;
            if (str8_match_lit("wmain", config->entry_point_name, 0)) {
              config->entry_point_name = str8_lit("wmainCRTStartup");
            } else if (str8_match_lit("main", config->entry_point_name, 0)) {
              config->entry_point_name = str8_lit("mainCRTStartup");
            } else if (str8_match_lit("WinMain", config->entry_point_name, 0)) {
              config->entry_point_name = str8_lit("WinMainCRTStartup");
            } else if (str8_match_lit("wWinMain", config->entry_point_name, 0)) {
              config->entry_point_name = str8_lit("wWinMainCRTStartup");
            }
          }
        }
        
        // generate undefined symbol so in case obj is in lib it will be linked
        if (config->entry_point_name.size) {
          str8_list_push(scratch.arena, &include_symbol_list, config->entry_point_name);
        }
        // no entry point, error and exit
        else {
          lnk_error(LNK_Error_EntryPoint, "unable to find entry point symbol");
        }
        
        // by default terminal server is enabled for windows and console applications
        if (~config->flags & LNK_ConfigFlag_NoTsAware && 
            ~config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL) {
          if (config->subsystem == PE_WindowsSubsystem_WINDOWS_GUI || config->subsystem == PE_WindowsSubsystem_WINDOWS_CUI) {
            config->dll_characteristics |= PE_DllCharacteristic_TERMINAL_SERVER_AWARE;
          }
        }
        
        // do we have a subsystem?
        if (config->subsystem == PE_WindowsSubsystem_UNKNOWN) {
          lnk_error(LNK_Error_NoSubsystem, "unknown subsystem, please use /SUBSYSTEM to set subsytem type you need");
        }
        
        if (config->subsystem_ver.major == 0 && config->subsystem_ver.minor == 0) {
          // subsystem version not specified, set default values
          config->subsystem_ver = lnk_get_default_subsystem_version(config->subsystem, config->machine);
        }
        
        // check subsystem version against allowed min version
        Version min_subsystem_ver = lnk_get_min_subsystem_version(config->subsystem, config->machine);
        int ver_cmp = version_compar(config->subsystem_ver, min_subsystem_ver);
        if (ver_cmp < 0) {
          lnk_error(LNK_Error_Cmdl, "subsystem version %I64u.%I64u can't be lower than %I64u.%I64u", 
                    config->subsystem_ver.major, config->subsystem_ver.minor, min_subsystem_ver.major, min_subsystem_ver.minor);
        }
        
        ProfEnd();
      } break;
      case State_ReportUnresolvedSymbols: {
        // report unresolved symbols
        for (LNK_SymbolNode *node = unresolved_undef_list.first; node != 0; node = node->next) {
          lnk_error_obj(LNK_Error_UnresolvedSymbol, node->data->u.undef.obj, "unresolved symbol %S", node->data->name);
        }
        if (unresolved_undef_list.count) {
          goto exit;
        }
      } break;
      case State_InputLinkerObjs: {
        {
          ProfBegin("Push Linker Symbols");
          LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
          input->path = str8_lit("* Linker Symbols *");
          input->dedup_id = input->path;
          input->data = lnk_make_linker_obj(tp_arena->v[0], config);
          ProfEnd();
        }

        // warn about unused delayloads
        if (config->flags & LNK_ConfigFlag_CheckUnusedDelayLoadDll) {
          for (String8Node *dll_name_n = config->delay_load_dll_list.first; dll_name_n != 0; dll_name_n = dll_name_n->next) {
            if (!hash_table_search_string_raw(delayed_imports, dll_name_n->string, 0)) {
              lnk_error(LNK_Warning_UnusedDelayLoadDll, "/DELAYLOAD: %S found no imports", dll_name_n->string);
            }
          }
        }

        // make and input delayed imports
        if (delayed_imports->count) {
          ProfBegin("Build Delay Import Table");

          COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;
          B32 emit_biat = config->import_table_emit_biat == LNK_SwitchState_Yes;
          B32 emit_uiat = config->import_table_emit_uiat == LNK_SwitchState_Yes;
          String8 *dll_names = keys_from_hash_table_string(scratch.arena, delayed_imports);
          String8List **dll_import_headers = values_from_hash_table_raw(scratch.arena, delayed_imports);

          for (U64 dll_idx = 0; dll_idx < delayed_imports->count; dll_idx += 1) {
            String8 import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, dll_names[dll_idx]);
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_import_dll_obj_delayed(tp_arena->v[0], time_stamp, config->machine, dll_names[dll_idx], delay_load_helper_name, import_debug_symbols, *dll_import_headers[dll_idx], emit_biat, emit_uiat);
            input->path = dll_names[dll_idx];
            input->dedup_id = input->path;
          }
          String8 linker_debug_symbols = lnk_make_linker_debug_symbols(tp_arena->v[0], config->machine);
          {
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_null_import_descriptor_delayed(tp_arena->v[0], time_stamp, config->machine, linker_debug_symbols);
            input->path = str8_lit("* Delayed Null Import Descriptor *");
            input->dedup_id = input->path;
          }
          {
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_null_thunk_data_obj_delayed(tp_arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols); 
            input->path = str8_lit("* Delayed Null Thunk Data *");
            input->dedup_id = input->path;
          }

          ProfEnd();
        }

        // make and input static imports
        if (static_imports->count) {
          ProfBegin("Build Static Import Table");

          COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;
          String8 *dll_names = keys_from_hash_table_string(scratch.arena, static_imports);
          String8List **dll_import_headers = values_from_hash_table_raw(scratch.arena, static_imports);
          for (U64 dll_idx = 0; dll_idx < static_imports->count; dll_idx += 1) {
            String8 import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, dll_names[dll_idx]);
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_import_dll_obj_static(tp_arena->v[0], time_stamp, config->machine, dll_names[dll_idx], import_debug_symbols, *dll_import_headers[dll_idx]);
            input->path = dll_names[dll_idx];
            input->dedup_id = dll_names[dll_idx];
          }
          String8 linker_debug_symbols = lnk_make_linker_debug_symbols(scratch.arena, config->machine);
          {
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_null_import_descriptor_obj(tp_arena->v[0], time_stamp, config->machine, linker_debug_symbols);
            input->path = str8_lit("* Null Import Descriptor *");
            input->dedup_id = input->path;
          }
          {
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->input_idx = input_obj_list.count;
            input->data = pe_make_null_thunk_data_obj(tp_arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols);
            input->path = str8_lit("* Null Thunk Data *");
            input->dedup_id = input->path;
          }
        
          ProfEnd();
        }

        if (export_symbol_list.count) {
          ProfBegin("Build Export Table");

          PE_ExportParseList resolved_exports = {0};
          for (PE_ExportParseNode *exp_n = export_symbol_list.first, *exp_n_next; exp_n != 0; exp_n = exp_n_next) {
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
              LNK_Symbol *symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, exp_n->data.name);
              if (symbol == 0) {
                lnk_error_with_loc(LNK_Warning_IllExport, exp->obj_path, exp->lib_path, "unresolved export symbol %S\n", exp->name);
                continue;
              }
            }

            // push resolved export
            pe_export_parse_list_push_node(&resolved_exports, exp_n);
          }

          PE_FinalizedExports finalized_exports = pe_finalize_export_list(scratch.arena, resolved_exports);
          String8 edata_obj = pe_make_edata_obj(tp_arena->v[0], str8_skip_last_slash(config->image_name), COFF_TimeStamp_Max, config->machine, finalized_exports);

          LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
          input->path = str8_lit("* Exports *");
          input->dedup_id = input->path;
          input->data = edata_obj;

          ProfEnd();
        }

        {
          String8List res_data_list = {0};
          String8List res_path_list = {0};
          
          // do we have manifest deps passed through pragma alone?
          LNK_ManifestOpt manifest_opt = config->manifest_opt;
          if (manifest_dep_list.node_count > 0 && manifest_opt == LNK_ManifestOpt_Null) {
            manifest_opt = LNK_ManifestOpt_Embed;
          }

          switch (manifest_opt) {
          case LNK_ManifestOpt_Embed: {
            ProfBegin("Embed Manifest");
            // TODO: currently we convert manifest to res and parse res again, this unnecessary instead push manifest 
            // resource to the tree directly
            String8 manifest_data = lnk_manifest_from_inputs(scratch.arena, config->mt_path, config->manifest_name, config->manifest_uac, config->manifest_level, config->manifest_ui_access, input_manifest_path_list, manifest_dep_list);
            String8 manifest_res  = pe_make_manifest_resource(scratch.arena, *config->manifest_resource_id, manifest_data);
            str8_list_push(scratch.arena, &res_data_list, manifest_res);
            str8_list_push(scratch.arena, &res_path_list, str8_lit("* Manifest *"));
            ProfEnd();
          } break;
          case LNK_ManifestOpt_WriteToFile: {
            ProfBeginDynamic("Write Manifest To: %.*s", str8_varg(config->manifest_name));
            Temp temp = temp_begin(scratch.arena);
            String8 manifest_data = lnk_manifest_from_inputs(temp.arena, config->mt_path, config->manifest_name, config->manifest_uac, config->manifest_level, config->manifest_ui_access, input_manifest_path_list, manifest_dep_list);
            lnk_write_data_to_file_path(config->manifest_name, str8_zero(), manifest_data);
            temp_end(temp);
            ProfEnd();
          } break;
          case LNK_ManifestOpt_Null: {
            Assert(input_manifest_path_list.node_count == 0);
            Assert(manifest_dep_list.node_count == 0);
          } break;
          case LNK_ManifestOpt_No: {
            // omit manifest generation
          } break;
          }
          
          ProfBegin("Load .res files from disk");
          for (String8Node *node = config->input_list[LNK_Input_Res].first; node != 0; node = node->next) {
            String8 res_data = lnk_read_data_from_file_path(scratch.arena, node->string);
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
            String8 obj_data = lnk_make_res_obj(tp_arena->v[0],
                                                res_data_list,
                                                res_path_list,
                                                config->machine,
                                                config->time_stamp,
                                                config->work_dir,
                                                config->path_style,
                                                obj_name);

            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->dedup_id     = obj_name;
            input->path         = obj_name;
            input->data         = obj_data;

            ProfEnd();
          }
        }

        if (lnk_do_debug_info(config)) {
          {
            ProfBegin("Build * Linker * Obj");

            String8 obj_name     = str8_lit("* Linker *");
            String8 raw_cmd_line = str8_list_join(scratch.arena, &config->raw_cmd_line, &(StringJoin){ str8_lit_comp(""),  str8_lit_comp(" "), str8_lit_comp("") });
            String8 obj_data     = lnk_make_linker_coff_obj(tp_arena->v[0], config->time_stamp, config->machine, config->work_dir, config->image_name, config->pdb_name, raw_cmd_line, obj_name);
            
            LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
            input->dedup_id = obj_name;
            input->path     = obj_name;
            input->data     = obj_data;

            ProfEnd();
          }

          {
            ProfBegin("Build * Debug Directories *");
            if (config->debug_mode != LNK_DebugMode_None && config->debug_mode != LNK_DebugMode_Null) {
              LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
              input->path     = str8_lit("* Debug Directory PDB *");
              input->dedup_id = input->path;
              input->data     = pe_make_debug_directory_pdb_obj(tp_arena->v[0], config->machine, config->guid, config->age, config->time_stamp, config->pdb_alt_path);
            }
            if (config->rad_debug == LNK_SwitchState_Yes) {
              LNK_InputObj *input = lnk_input_obj_list_push(scratch.arena, &input_obj_list);
              input->path     = str8_lit("* Debug Directory RDI *");
              input->dedup_id = input->path;
              input->data     = pe_make_debug_directory_rdi_obj(tp_arena->v[0], config->machine, config->guid, config->age, config->time_stamp, config->rad_debug_alt_path);
            }
            ProfEnd();
          }
        }
      } break;
      }
    }
    
    if (input_disallow_lib_list.node_count) {
      state_list_push(scratch.arena, state_list, State_InputDisallowLibs);
      continue;
    }
    if (input_import_list.count) {
      state_list_push(scratch.arena, state_list, State_InputImports);
      continue;
    }
    if (input_weak_list.count || include_symbol_list.node_count) {
      state_list_push(scratch.arena, state_list, State_InputSymbols);
      continue;
    }
    if (input_obj_list.count) {
      state_list_push(scratch.arena, state_list, State_InputObjs);
      continue;
    }
    {
      B32 have_pending_lib_inputs = 0;
      for (U64 i = 0; i < ArrayCount(input_libs); ++i) {
        if (input_libs[i].node_count) {
          have_pending_lib_inputs = 1;
          break;
        }
      }
      if (have_pending_lib_inputs) {
        state_list_push(scratch.arena, state_list, State_InputLibs);
        continue;
      }
    }
    if (lookup_undef_list.count) {
      state_list_push(scratch.arena, state_list, State_LookupUndef);
      continue;
    }
    if (lookup_weak_list.count) {
      state_list_push(scratch.arena, state_list, State_LookupWeak);
      continue;
    }
    if (unresolved_weak_list.count) {
      // we can't find strong definitions for unresolved weak symbols
      // so now we have to use fallback symbols
      MemoryZeroStruct(&unresolved_weak_list);
      continue;
    }
    if (entry_point_lookup_attempts == 0) {
      state_list_push(scratch.arena, state_list, State_LookupEntryPoint);
      entry_point_lookup_attempts += 1;
      continue;
    }
    if (input_linker_objs) {
      input_linker_objs = 0;
      state_list_push(scratch.arena, state_list, State_InputLinkerObjs);
      continue;
    }
    if (unresolved_undef_list.count) {
      if (report_unresolved_symbols) {
        report_unresolved_symbols = 0;
        state_list_push(scratch.arena, state_list, State_ReportUnresolvedSymbols);
        continue;
      }
    }
    
    break;
  }

  // log
  {
    if (lnk_get_log_status(LNK_Log_InputObj)) {
      U64 total_input_size = 0;
      for (LNK_ObjNode *obj_n = obj_list.first; obj_n != 0; obj_n = obj_n->next) { total_input_size += obj_n->data.data.size; }
      lnk_log(LNK_Log_InputObj, "[Total Obj Input Size %M]", total_input_size);
    }
    if (lnk_get_log_status(LNK_Log_InputLib)) {
      U64 total_input_size = 0;
      for (U64 i = 0; i < ArrayCount(lib_index); ++i) {
        LNK_LibList list = lib_index[i];
        for (LNK_LibNode *lib_n = list.first; lib_n != 0; lib_n = lib_n->next) { total_input_size += lib_n->data.data.size; }
      }
      lnk_log(LNK_Log_InputLib, "[Total Lib Input Size %M]", total_input_size);
    }
  }
  
  exit:;

  LNK_LinkContext link_ctx    = {0};
  link_ctx.symtab             = symtab;
  link_ctx.objs_count         = obj_list.count;
  link_ctx.objs               = lnk_array_from_obj_list(tp_arena->v[0], obj_list);
  link_ctx.export_symbol_list = export_symbol_list;
  MemoryCopyTyped(&link_ctx.lib_index[0], &lib_index[0], ArrayCount(lib_index));

  ProfEnd();
  scratch_end(scratch);
  return link_ctx;
  
#undef state_list_push
#undef state_list_pop
}

internal
THREAD_POOL_TASK_FUNC(lnk_remove_associative_sections_task)
{
  LNK_BuildImageTask *task = raw_task;

  LNK_Obj *obj          = task->objs[task_id];
  String8  string_table = str8_substr(obj->data, obj->header.string_table_range);
  String8  symbol_table = str8_substr(obj->data, obj->header.symbol_table_range);
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    // find associate section head section index
    U32 head_sect_idx = max_U32;
    for (U64 current_sect_idx = sect_idx;;) {
      U32 symbol_idx = obj->comdats[current_sect_idx];
      if (symbol_idx == max_U32) {
        break;
      }

      COFF_ParsedSymbol symbol = coff_parse_symbol(obj->header, string_table, symbol_table, symbol_idx);
      COFF_ComdatSelectType selection = COFF_ComdatSelect_Null;
      U32 section_number = 0;
      coff_parse_secdef(symbol, obj->header.is_big_obj, &selection, &section_number, 0, 0);
      if (selection != COFF_ComdatSelect_Associative) {
        head_sect_idx = current_sect_idx;
        break;
      }

      current_sect_idx = section_number-1;
    }

    if (head_sect_idx != max_U32) {
      // flag current section with remove if head section was removed
      COFF_SectionHeader *head_sect_header = lnk_coff_section_header_from_section_number(obj, head_sect_idx+1);
      COFF_SectionHeader *curr_sect_header = lnk_coff_section_header_from_section_number(obj, sect_idx+1);
      curr_sect_header->flags |= (head_sect_header->flags & COFF_SectionFlag_LnkRemove);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_section_definitions_task)
{
  LNK_BuildImageTask *task = raw_task;

  HashTable          *sect_defn_ht  = task->u.gather_sects.defns[worker_id];
  LNK_Obj            *obj           = task->objs[task_id];
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];

    // remove section
    if (sect_header->flags & COFF_SectionFlag_LnkRemove) {
      continue;
    }

    // parse section name
    String8 full_sect_name = coff_name_from_section_header(string_table, sect_header);
    String8 sect_name, sect_sort_idx;
    coff_parse_section_name(full_sect_name, &sect_name, &sect_sort_idx);

    // was section defined?
    COFF_SectionFlags      sect_flags           = sect_header->flags & ~COFF_SectionFlags_LnkFlags;
    String8                sect_name_with_flags = lnk_make_name_with_flags(arena, sect_name, sect_flags);
    LNK_SectionDefinition *sect_defn            = 0;
    hash_table_search_string_raw(sect_defn_ht, sect_name_with_flags, &sect_defn);

    // create section definition
    if (sect_defn == 0) {
      sect_defn               = push_array(arena, LNK_SectionDefinition, 1);
      sect_defn->name         = sect_name;
      sect_defn->flags        = sect_flags;
      sect_defn->obj          = obj;
      sect_defn->obj_sect_idx = sect_idx;
      hash_table_push_string_raw(arena, sect_defn_ht, sect_name_with_flags, sect_defn);
    }

    // acc contrib count
    sect_defn->contribs_count += 1;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_section_contribs_task)
{
  LNK_BuildImageTask *task = raw_task;
  U64 obj_idx = task_id;

  LNK_Obj            *obj           = task->objs[obj_idx];
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  String8             string_table  = str8_substr(obj->data, obj->header.string_table_range);

  ProfBeginV("Gather Section Contribs [%S]", obj->path);
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];

    if (sect_header->flags & COFF_SectionFlag_LnkRemove) {
      continue;
    }

    // parse section name
    String8 full_sect_name = coff_name_from_section_header(string_table, sect_header);
    String8 sect_name, sect_sort_idx;
    coff_parse_section_name(full_sect_name, &sect_name, &sect_sort_idx);

    // search for section to contribute
    COFF_SectionFlags  flags = sect_header->flags & ~COFF_SectionFlags_LnkFlags;
    LNK_Section       *sect  = lnk_section_table_search(task->sectab, sect_name, flags);

    LNK_SectionContrib *sc;
    if (sect) {
      // extract align
      U16 sc_align = coff_align_size_from_section_flags(sect_header->flags);
      if (sc_align == 0) {
        sc_align = task->default_align;
      }

      // extract section bytes
      String8 sect_data = str8_substr(obj->data, rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize));

      // fill out contrib
      sc = lnk_section_contrib_chunk_push_atomic(sect->contribs.first, 1);

      sc->node.next       = 0;
      sc->node.string     = sect_data;
      sc->data_list       = &sc->node;
      sc->align           = sc_align;
      sc->u.obj_idx       = obj_idx;
      sc->u.obj_sect_idx  = sect_idx;
      sc->u.sort_idx_size = (U16)sect_sort_idx.size;
      sc->u.sort_idx      = sect_sort_idx.str;
    } else {
      // section was removed, fill slot with pointer to null contrib
      sc = &g_null_sc;
    }

    task->sect_map[obj_idx][sect_idx] = sc;
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_set_comdat_leaders_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Set COMDAT Leaders [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular && symbol.storage_class == COFF_SymStorageClass_External && symbol.value == 0) {
      COFF_SectionHeader *sect_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);
      if (sect_header->flags & COFF_SectionFlag_LnkCOMDAT) {
        LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);
        COFF_ParsedSymbol defn_symbol = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);
        task->sect_map[obj_idx][symbol.section_number - 1] = task->sect_map[defn->u.defined.obj->input_idx][defn_symbol.section_number - 1];
      }
    }
  }
  ProfEnd();
}

internal int
lnk_section_contrib_ptr_is_before(void *raw_a, void *raw_b)
{
  // Grouped Sections (PE Format)
  //  "All contributions with the same object-section name are allocated contiguously in the image,
  //  and the blocks of contributions are sorted in lexical order by object-section name." 
  LNK_SectionContrib *a = *(LNK_SectionContrib **)raw_a;
  LNK_SectionContrib *b = *(LNK_SectionContrib **)raw_b;

  int cmp;

  if (a->u.sort_idx_size <= 1 && b->u.sort_idx_size <= 1) {
    if (a->u.sort_idx_size == b->u.sort_idx_size) {
      cmp = u32_compar(&a->u.obj_idx, &b->u.obj_idx);
      if (cmp == 0) {
        cmp = u32_compar(&a->u.obj_sect_idx, &b->u.obj_sect_idx);
      }
    } else {
      // place sections without sort postfix first
      cmp = a->u.sort_idx_size < b->u.sort_idx_size;
    }
  } else {
    // sort on section postfix
    String8 a_sort_idx = str8(a->u.sort_idx, a->u.sort_idx_size);
    String8 b_sort_idx = str8(b->u.sort_idx, b->u.sort_idx_size);
    cmp = str8_compar_case_sensitive(&a_sort_idx, &b_sort_idx);

    // sort on obj position on command line
    if (cmp == 0) {
      cmp = u32_compar(&a->u.obj_idx, &b->u.obj_idx);

      // sort on section index
      if (cmp == 0) {
        cmp = u32_compar(&a->u.obj_sect_idx, &b->u.obj_sect_idx);
      }
    }
  }

  int is_before = cmp < 0;
  return is_before;
}

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_task)
{
  LNK_BuildImageTask *task = raw_task;
  LNK_SectionContribChunk *chunk = task->u.sort_contribs.chunks[task_id];
  Assert(chunk->count == chunk->cap);
  radsort(chunk->v, chunk->count, lnk_section_contrib_ptr_is_before);
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
    if (a_symbol->u.defined.obj->input_idx == b_symbol->u.defined.obj->input_idx) {
      is_before = a_symbol->u.defined.symbol_idx < b_symbol->u.defined.symbol_idx;
    } else {
      is_before = a_symbol->u.defined.obj->input_idx < b_symbol->u.defined.obj->input_idx;
    }
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
  Rng1U64             contrib_range = task->u.patch_symtabs.ranges[task_id];

  for (U64 contrib_idx = contrib_range.min; contrib_idx < contrib_range.max; contrib_idx += 1) {
    LNK_CommonBlockContrib *contrib        = &task->u.patch_symtabs.common_block_contribs[contrib_idx];
    LNK_Symbol             *symbol         = contrib->symbol;
    LNK_Obj                *obj            = symbol->u.defined.obj;
    COFF_ParsedSymbol       parsed_symbol  = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol->u.defined.symbol_idx);
    U64                     section_number = task->u.patch_symtabs.common_block_sect->sect_idx + 1;

    if (obj->header.is_big_obj) {
      COFF_Symbol32 *symbol32 = parsed_symbol.raw_symbol;
      symbol32->value          = contrib->u.offset;
      symbol32->section_number = safe_cast_u32(section_number);
    } else {
      COFF_Symbol16 *symbol16 = parsed_symbol.raw_symbol;
      symbol16->value          = contrib->u.offset;
      symbol16->section_number = safe_cast_u16(section_number);
    }

    task->u.patch_symtabs.was_symbol_patched[obj->input_idx][symbol->u.defined.symbol_idx] = 1;
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
      U32 section_number = 0;
      U32 value = 0;
      {
        COFF_SectionHeader *sect_header = lnk_coff_section_header_from_section_number(obj, symbol.section_number);
        if (~sect_header->flags & COFF_SectionFlag_LnkRemove) {
          LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number-1];
          section_number = safe_cast_u32(sc->u.sect_idx + 1);
          value = sc->u.off + symbol.value;
        }
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
      LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);
      COFF_ParsedSymbol defn_parsed = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);
      if (defn) {
        LNK_SectionContrib *sc = task->sect_map[defn->u.defined.obj->input_idx][defn_parsed.section_number-1];
        if (obj->header.is_big_obj) {
          COFF_Symbol32 *symbol32  = symbol.raw_symbol;
          symbol32->section_number = safe_cast_u32(sc->u.sect_idx + 1);
          symbol32->value          = safe_cast_u32(sc->u.off);
          symbol32->storage_class  = COFF_SymStorageClass_Static;
        } else {
          COFF_Symbol16 *symbol16  = symbol.raw_symbol;
          symbol16->section_number = safe_cast_u16(sc->u.sect_idx + 1);
          symbol16->value          = safe_cast_u32(sc->u.off);
          symbol16->storage_class  = COFF_SymStorageClass_Static;
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_abs_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Absolute Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);

    if (interp == COFF_SymbolValueInterp_Abs && symbol.storage_class == COFF_SymStorageClass_External) {
      LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);

      if (defn == 0) {
        continue;
      }
      if (defn->u.defined.obj == obj && defn->u.defined.symbol_idx == symbol_idx) {
        continue;
      }

      COFF_ParsedSymbol          defn_symbol = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);
      COFF_SymbolValueInterpType defn_interp = coff_interp_symbol(defn_symbol.section_number, defn_symbol.value, defn_symbol.storage_class);
      if (defn_interp == COFF_SymbolValueInterp_Regular) {
        if (defn->u.defined.obj->header.is_big_obj) {
          COFF_Symbol32 *symbol32 = symbol.raw_symbol;
          symbol32->section_number   = defn_symbol.section_number;
          symbol32->value            = defn_symbol.value;
          symbol32->type             = defn_symbol.type;
          symbol32->storage_class    = COFF_SymStorageClass_Static;
        } else {
          COFF_Symbol16 *symbol16 = symbol.raw_symbol;
          symbol16->section_number = defn_symbol.section_number;
          symbol16->value          = defn_symbol.value;
          symbol16->type           = defn_symbol.type;
          symbol16->storage_class  = COFF_SymStorageClass_Static;
        }
      } else {
        InvalidPath;
      }
    }
  }
  ProfEnd();
}

internal void
lnk_patch_weak_external_symbol(B32 is_big_obj, void *symbol, COFF_ParsedSymbol parsed_symbol)
{
  COFF_SymbolValueInterpType parsed_symbol_interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
  switch (parsed_symbol_interp) {
  case COFF_SymbolValueInterp_Regular: {
    if (is_big_obj) {
      COFF_Symbol32 *symbol32 = symbol;
      symbol32->section_number = parsed_symbol.section_number;
      symbol32->value          = parsed_symbol.value;
      symbol32->type           = parsed_symbol.type;
      symbol32->storage_class  = COFF_SymStorageClass_Static;
    } else {
      COFF_Symbol16 *symbol16 = symbol;
      symbol16->section_number = safe_cast_u16(parsed_symbol.section_number);
      symbol16->value          = parsed_symbol.value;
      symbol16->type           = parsed_symbol.type;
      symbol16->storage_class  = COFF_SymStorageClass_Static;
    }
  } break;
  case COFF_SymbolValueInterp_Common: {
    InvalidPath;
  } break;
  case COFF_SymbolValueInterp_Abs: {
    if (is_big_obj) {
      COFF_Symbol32 *symbol32 = symbol;
      symbol32->section_number   = COFF_Symbol_AbsSection32;
      symbol32->value            = parsed_symbol.value;
      symbol32->type             = parsed_symbol.type;
      symbol32->storage_class    = COFF_SymStorageClass_Static;
    } else {
      COFF_Symbol16 *symbol16 = symbol;
      symbol16->section_number   = COFF_Symbol_AbsSection16;
      symbol16->value            = parsed_symbol.value;
      symbol16->type             = parsed_symbol.type;
      symbol16->storage_class    = COFF_SymStorageClass_Static;
    }
  } break;
  case COFF_SymbolValueInterp_Weak:
  case COFF_SymbolValueInterp_Debug:
  case COFF_SymbolValueInterp_Undefined: {
    InvalidPath;
  } break;
  default: { NotImplemented; } break;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_undefined_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Undefined Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Undefined) {
      if (symbol.storage_class == COFF_SymStorageClass_External) {
        LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);
        if (defn) {
          COFF_ParsedSymbol defn_symbol = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);

          if (defn_symbol.storage_class == COFF_SymStorageClass_WeakExternal) {
            continue;
          }

          lnk_patch_weak_external_symbol(obj->header.is_big_obj, symbol.raw_symbol, defn_symbol);
        } else {
          // TODO: collect unresolved undefined
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_weak_symbols_with_strong_definition_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Weak Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Weak) {
      LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, symbol.name);
      if (defn) {
        COFF_ParsedSymbol          defn_symbol = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);
        COFF_SymbolValueInterpType defn_interp = coff_interp_symbol(defn_symbol.section_number, defn_symbol.value, defn_symbol.storage_class);
        if (defn_interp != COFF_SymbolValueInterp_Weak) {
          lnk_patch_weak_external_symbol(obj->header.is_big_obj, symbol.raw_symbol, defn_symbol);
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_weak_symbols_with_fallback_definition_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  HashTable *visited_symbols_ht = hash_table_init(scratch.arena, 32);
  struct LookupLocation {
    struct LookupLocation *next;
    LNK_Obj *obj;
    U64      symbol_idx;
  };
  struct LookupLocation *lookup_first     = 0;
  struct LookupLocation *lookup_last      = 0;
  struct LookupLocation *lookup_free_list = 0;

  ProfBegin("Patch Weak Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Undefined && symbol.storage_class == COFF_SymStorageClass_External) {
      String8  lookup_name       = symbol.name;
      LNK_Obj *lookup_obj        = obj;
      U64      lookup_symbol_idx = symbol_idx;
      for (;;) {
        // lookup definition
        LNK_Symbol *defn = lnk_symbol_table_search(task->symtab, LNK_SymbolScope_Defined, lookup_name);
        if (defn == 0) {
          break;
        }

        // not external symbol? patch and move to next symbol
        COFF_ParsedSymbol defn_parsed = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, defn->u.defined.symbol_idx);
        if (defn_parsed.storage_class != COFF_SymStorageClass_WeakExternal) {
          lnk_patch_weak_external_symbol(obj->header.is_big_obj, symbol.raw_symbol, defn_parsed);
          break;
        }

        // check against cyclic refs
        struct LookupLocation *was_visited = 0;
        hash_table_search_string_raw(visited_symbols_ht, lookup_name, &was_visited);
        if (was_visited != 0) {
          Temp temp = temp_begin(scratch.arena);

          String8List list = {0};
          for (struct LookupLocation *l = lookup_first; l != 0; l = l->next) {
            COFF_ParsedSymbol loc_symbol = lnk_parsed_symbol_from_coff_symbol_idx(l->obj, l->symbol_idx);
            str8_list_pushf(temp.arena, &list, "\t%S Symbol %S (No.%#llx) =>", l->obj->path, loc_symbol.name, l->symbol_idx);
          }
          {
            COFF_ParsedSymbol loc_symbol = lnk_parsed_symbol_from_coff_symbol_idx(was_visited->obj, was_visited->symbol_idx);
            str8_list_pushf(temp.arena, &list, "\t%S Symbol %S (No.%#llx)", was_visited->obj->path, loc_symbol.name, was_visited->symbol_idx);
          }

          String8 loc_string = str8_list_join(temp.arena, &list, &(StringJoin){.sep = str8_lit("\n") });
          lnk_error_obj(LNK_Error_WeakCycle, obj, "unable to resolve cyclic symbol %S; ref chain:\n%S", symbol.name, loc_string);

          temp_end(temp);
          break;
        }
        struct LookupLocation *loc = lookup_free_list;
        if (lookup_free_list) {
          SLLStackPop(lookup_free_list);
        } else {
          loc = push_array(scratch.arena, struct LookupLocation, 1);
        }
        loc->obj                   = lookup_obj;
        loc->symbol_idx            = symbol_idx;
        SLLQueuePush(lookup_first, lookup_last, loc);
        hash_table_push_string_raw(scratch.arena, visited_symbols_ht, lookup_name, loc);

        // fallback to weak tag for definition
        COFF_SymbolWeakExt *weak_ext   = coff_parse_weak_tag(defn_parsed, defn->u.defined.obj->header.is_big_obj);
        COFF_ParsedSymbol   parsed_tag = lnk_parsed_symbol_from_coff_symbol_idx(defn->u.defined.obj, weak_ext->tag_index);
        lookup_name       = parsed_tag.name;
        lookup_obj        = defn->u.defined.obj;
        lookup_symbol_idx = weak_ext->tag_index;
      }

      hash_table_purge(visited_symbols_ht);
      lookup_free_list = lookup_first;
      lookup_first     = 0;
      lookup_last      = 0;
    }
  }
  ProfEnd();

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_weak_symbols_with_undefined_tag_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Weak Symbols [%S]", obj->path);
  COFF_ParsedSymbol symbol;
  for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
    symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Weak) {
      COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(symbol, obj->header.is_big_obj);
      AssertAlways(weak_ext->tag_index < symbol_idx);

      COFF_ParsedSymbol          defn_symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, weak_ext->tag_index);
      COFF_SymbolValueInterpType defn_interp = coff_interp_symbol(defn_symbol.section_number, defn_symbol.value, defn_symbol.storage_class);
      if (defn_interp != COFF_SymbolValueInterp_Undefined) {
        lnk_patch_weak_external_symbol(obj->header.is_big_obj, symbol.raw_symbol, defn_symbol);
      }
    }
  }
  ProfEnd();
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
  COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj_header.section_table_range).str;
  String8              symbol_table  = str8_substr(obj->data, obj_header.symbol_table_range);
  String8              string_table  = str8_substr(obj->data, obj_header.string_table_range);

  for (U64 sect_idx = 0; sect_idx < obj_header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *section_header = &section_table[sect_idx];

    // was section removed?
    if (section_header->flags & COFF_SectionFlag_LnkRemove) {
      continue;
    }
    if (section_header->flags & COFF_SectionFlag_CntUninitializedData) {
      continue;
    }

    // get section file range
    Rng1U64 section_frange = rng_1u64(section_header->foff, section_header->foff + section_header->fsize);

    // get section bytes
    String8 section_data;
    if (lnk_is_coff_section_debug(obj, sect_idx)) {
      section_data = str8_substr(obj->data, section_frange);
    } else {
      section_data = str8_substr(task->image_data, section_frange);
    }

    // find section relocs
    COFF_RelocInfo reloc_info = coff_reloc_info_from_section_header(obj->data, section_header);
    COFF_Reloc    *relocs     = (COFF_Reloc *)(obj->data.str + reloc_info.array_off);

    // apply relocs
    for (U64 reloc_idx = 0; reloc_idx < reloc_info.count; reloc_idx += 1) {
      COFF_Reloc *reloc = &relocs[reloc_idx];

      // error check relocation
      if (obj->header.machine == COFF_MachineType_X64) {
        if (reloc->type > COFF_Reloc_X64_Last) {
          lnk_error_obj(LNK_Error_IllegalRelocation, obj, "unknown relocation 0x%x", reloc->type);
        }
      } else if (obj->header.machine != COFF_MachineType_Unknown) {
        NotImplemented;
      }

      // compute virtual offsets
      U64 reloc_voff = section_header->voff + reloc->apply_off;

      // compute symbol location values
      U32 symbol_secnum = 0;
      U32 symbol_secoff = 0;
      S64 symbol_voff   = 0;
      {
        COFF_ParsedSymbol symbol;
        if (obj_header.is_big_obj) {
          symbol = coff_parse_symbol32(string_table, (COFF_Symbol32 *)symbol_table.str + reloc->isymbol);
        } else {
          symbol = coff_parse_symbol16(string_table, (COFF_Symbol16 *)symbol_table.str + reloc->isymbol);
        }

        COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
        if (interp == COFF_SymbolValueInterp_Regular) {
          symbol_secnum = symbol.section_number;
          symbol_secoff = symbol.value;
          symbol_voff = safe_cast_u32((U64)task->image_section_table[symbol.section_number]->voff + (U64)symbol_secoff);
        } else if (interp == COFF_SymbolValueInterp_Abs) {
          // There aren't enough bits in COFF symbol to store full image base address,
          // so we special case __ImageBase. A better solution would be to add
          // a 64-bit symbol format to COFF.
          if (str8_match(symbol.name, str8_lit("__ImageBase"), 0)) {
            symbol.value = task->image_base;
          }

          symbol_secnum = 0;
          symbol_secoff = 0;
          symbol_voff = (S64)symbol.value - (S64)task->image_base;
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
  LNK_SectionDefinition **a = raw_a;
  LNK_SectionDefinition **b = raw_b;
  int is_before;
  if ((*a)->obj->input_idx == (*b)->obj->input_idx) {
    is_before = (*a)->obj_sect_idx < (*b)->obj_sect_idx;
  } else {
    is_before = (*a)->obj->input_idx < (*b)->obj->input_idx;
  }
  return is_before;
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
      sect_header->vsize = sc->u.size;
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
      !lnk_is_coff_section_debug(obj, sect_idx);
    if (patch_section_header) {
      LNK_SectionContrib *sc   = task->sect_map[obj_idx][sect_idx];
      LNK_Section        *sect = task->image_sects.v[sc->u.sect_idx];
      if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
        sect_header->fsize = sc->u.size;
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

int
lnk_base_reloc_page_compar(const void *raw_a, const void *raw_b)
{
  const LNK_BaseRelocPage *a = raw_a;
  const LNK_BaseRelocPage *b = raw_b;
  return u64_compar(&a->voff, &b->voff);
}

int
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

    LNK_Symbol *entry_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, config->entry_point_name);
    if (entry_symbol) {
      *entry_point_va = safe_cast_u32(lnk_virt_off_from_symbol(section_table, entry_symbol));
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

  // init section table
  LNK_SectionTable *sectab = lnk_section_table_alloc();
  lnk_section_table_push(sectab, str8_lit(".text"),  PE_TEXT_SECTION_FLAGS);
  lnk_section_table_push(sectab, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS);
  lnk_section_table_push(sectab, str8_lit(".data"),  PE_DATA_SECTION_FLAGS);

  LNK_BuildImageTask task = {0};
  task.symtab        = symtab;
  task.sectab        = sectab;
  task.default_align = lnk_default_align_from_machine(config->machine);
  task.objs_count    = objs_count;
  task.objs          = objs;
  task.sect_map      = 0;

  ProfBegin("Remove Associative Sections");
  tp_for_parallel(tp, 0, objs_count, lnk_remove_associative_sections_task, &task);
  ProfEnd();

  {
    ProfBegin("Define And Count Sections");
    TP_Temp temp = tp_temp_begin(arena);

    // init hash tables for gathering section definitions
    task.u.gather_sects.defns = push_array(arena->v[0], HashTable *, tp->worker_count);
    for (U64 worker_idx = 0; worker_idx < tp->worker_count; worker_idx += 1) task.u.gather_sects.defns[worker_idx] = hash_table_init(arena->v[0], 128);

    ProfBegin("Gather Section Definitions");
    tp_for_parallel(tp, arena, objs_count, lnk_gather_section_definitions_task, &task);
    ProfEnd();

    ProfBegin("Merge Section Definitions Hash Tables");
    for (U64 worker_idx = 1; worker_idx < tp->worker_count; worker_idx += 1) {
      U64                     sect_defns_count = task.u.gather_sects.defns[worker_idx]->count;
      LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(arena->v[0], task.u.gather_sects.defns[worker_idx]);
      radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);

      for (U64 defn_idx = 0; defn_idx < sect_defns_count; defn_idx += 1) {
        String8                name_with_flags = lnk_make_name_with_flags(arena->v[0], sect_defns[defn_idx]->name, sect_defns[defn_idx]->flags);
        LNK_SectionDefinition *main_defn       = 0;
        hash_table_search_string_raw(task.u.gather_sects.defns[0], name_with_flags, &main_defn);
        if (main_defn == 0) {
          main_defn = sect_defns[defn_idx];
          hash_table_push_string_raw(arena->v[0], task.u.gather_sects.defns[0], name_with_flags, main_defn);
        } else {
          main_defn->contribs_count += sect_defns[defn_idx]->contribs_count;
        }
      }
    }
    ProfEnd();

    U64                     sect_defns_count = task.u.gather_sects.defns[0]->count;
    LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(arena->v[0], task.u.gather_sects.defns[0]);

    ProfBegin("Sort Sections Definitions");
    radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);
    ProfEnd();

    ProfBegin("Push Sections And Reserve Section Contrib Memory");
    for (U64 defn_idx = 0; defn_idx < sect_defns_count; defn_idx += 1) {
      LNK_SectionDefinition *sect_defn = sect_defns[defn_idx];

      // do not create definitions for sections that are removed from the image
      {
        B32 skip = 0;
        for (String8Node *name_n = config->remove_sections.first; name_n != 0; name_n = name_n->next) {
          if (str8_match(sect_defn->name, name_n->string, 0)) {
            skip = 1;
            break;
          }
        }
        if (skip) { continue; }
      }

      // warn about conflicting section flags
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        LNK_Section *sect = &sect_n->data;
        if (str8_match(sect->name, sect_defn->name, 0)) {
          if (sect->flags != sect_defn->flags) {
            LNK_Obj            *obj                = sect_defn->obj;
            U32                 sect_number        = sect_defn->obj_sect_idx + 1;
            COFF_SectionHeader *sect_header        = lnk_coff_section_header_from_section_number(obj, sect_number);
            String8             sect_name          = coff_name_from_section_header(str8_substr(obj->data, obj->header.string_table_range), sect_header);
            String8             expected_flags_str = coff_string_from_section_flags(arena->v[0], sect->flags);
            String8             current_flags_str  = coff_string_from_section_flags(arena->v[0], sect_defn->flags);
            lnk_error_obj(LNK_Warning_SectionFlagsConflict, sect_defn->obj, "detected section flags conflict in %S(No. %X); expected {%S} but got {%S}", sect_name, sect_number, expected_flags_str, current_flags_str);
          }
        }
      }

      ProfBeginV("Reserve Section Contrib Chunks [%S]", sect_defn->name);
      LNK_Section *sect = lnk_section_table_search(sectab, sect_defn->name, sect_defn->flags);
      if (sect == 0) {
        sect = lnk_section_table_push(sectab, sect_defn->name, sect_defn->flags);
      }
      AssertAlways(sect->contribs.chunk_count == 0);
      lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &sect->contribs, sect_defn->contribs_count);
      ProfEnd();

    }
    ProfEnd();

    tp_temp_end(temp);
    ProfEnd();
  }

  U64 expected_image_header_size;
  {
    ProfBegin("Alloc Section Map");
    task.sect_map = push_array(scratch.arena, LNK_SectionContrib **, objs_count);
    for (U64 obj_idx = 0; obj_idx < objs_count; obj_idx += 1) task.sect_map[obj_idx] = push_array(scratch.arena, LNK_SectionContrib *, objs[obj_idx]->header.section_count_no_null);
    ProfEnd();

    ProfBegin("Gather Section Contribs");
    tp_for_parallel(tp, 0, objs_count, lnk_gather_section_contribs_task, &task);
    ProfEnd();

    ProfBegin("Set COMDAT Leaders");
    tp_for_parallel(tp, 0, objs_count, lnk_set_comdat_leaders_task, &task);
    ProfEnd();

    // finalize sections layouts
    {
      ProfBegin("Finalize Sections Layout");

      // merge sections
      if (config->flags & LNK_ConfigFlag_Merge) {
        lnk_section_table_merge(sectab, config->merge_list);
      }

      ProfBegin("Sort Section Contribs");
      {
        U64                       total_chunk_count = 0;
        LNK_SectionContribChunk **chunks            = 0;
        {
          for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
            total_chunk_count += sect_n->data.contribs.chunk_count;
          }
          U64 cursor = 0;
          chunks = push_array(scratch.arena, LNK_SectionContribChunk *, total_chunk_count);
          for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
            for (LNK_SectionContribChunk *chunk_n = sect_n->data.contribs.first; chunk_n != 0; chunk_n = chunk_n->next) {
              chunks[cursor++] = chunk_n;
            }
          }
          Assert(cursor == total_chunk_count);
        }
        task.u.sort_contribs.chunks = chunks;
        tp_for_parallel(tp, 0, total_chunk_count, lnk_sort_contribs_task, &task);
      }
      ProfEnd();

      // assign contribs offsets, sizes, and section indices
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_finalize_section_layout(sectab, &sect_n->data, config->file_align);
      }

      // remove empty sections
      {
        String8List empty_sect_list = {0};
        for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
          LNK_Section *sect = &sect_n->data;
          if (sect->vsize == 0) {
            str8_list_push(scratch.arena, &empty_sect_list, sect->name);
          }
        }
        for (String8Node *name_n = empty_sect_list.first; name_n != 0; name_n = name_n->next) {
          lnk_section_table_remove(sectab, name_n->string);
        }
      }

      // assign section indices to sections
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_assign_section_index(&sect_n->data, sectab->next_sect_idx++);
      }

      // assing layout offsets and sizes to merged sections
      for (LNK_SectionNode *sect_n = sectab->merge_list.first; sect_n != 0; sect_n = sect_n->next) {
        LNK_Section *sect = &sect_n->data;
        LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(sect);
        LNK_SectionContrib *last_sc = lnk_get_last_section_contrib(sect);
        LNK_Section *final_sect = lnk_finalized_section_from_id(sectab, sect->merge_id);
        sect->voff = final_sect->voff + first_sc->u.off;
        sect->vsize = (last_sc->u.off - first_sc->u.off) + last_sc->u.size;
        sect->foff = final_sect->foff + first_sc->u.off;
        sect->fsize = (last_sc->u.off - first_sc->u.off) + last_sc->u.size;
        sect->sect_idx = final_sect->sect_idx;
      }

      ProfEnd();
    }

    // build common block
    //
    // TODO: build common block in .bss and merge with .data
    U64                     common_block_contribs_count;
    LNK_CommonBlockContrib *common_block_contribs;
    LNK_Section            *common_block_sect;
    {
      ProfBegin("Build Common Block");

      ProfBegin("Count Contribs");
      common_block_contribs_count = 0;
      for (U64 worker_id = 0; worker_id < tp->worker_count; worker_id += 1) {
        for (LNK_SymbolHashTrieChunk *chunk = symtab->chunk_lists[LNK_SymbolScope_Defined][worker_id].first;
            chunk != 0;
            chunk = chunk->next) {
          for (U64 i = 0; i < chunk->count; i += 1) {
            LNK_Symbol *symbol = chunk->v[i].symbol;
            COFF_ParsedSymbol parsed_symbol = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
            COFF_SymbolValueInterpType parsed_interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
            if (parsed_interp == COFF_SymbolValueInterp_Common) {
              common_block_contribs_count += 1;
            }
          }
        }
      }
      ProfEnd();

      ProfBegin("Gather Contribs");
      common_block_contribs = push_array(scratch.arena, LNK_CommonBlockContrib, common_block_contribs_count);
      {
        U64 cursor = 0;
        for (U64 worker_id = 0; worker_id < tp->worker_count; worker_id += 1) {
          for (LNK_SymbolHashTrieChunk *chunk = symtab->chunk_lists[LNK_SymbolScope_Defined][worker_id].first;
              chunk != 0;
              chunk = chunk->next) {
            for (U64 i = 0; i < chunk->count; i += 1) {
              LNK_Symbol *symbol = chunk->v[i].symbol;
              COFF_ParsedSymbol parsed_symbol = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
              COFF_SymbolValueInterpType parsed_interp = coff_interp_symbol(parsed_symbol.section_number, parsed_symbol.value, parsed_symbol.storage_class);
              if (parsed_interp == COFF_SymbolValueInterp_Common) {
                LNK_CommonBlockContrib *contrib = &common_block_contribs[cursor++];
                contrib->symbol = chunk->v[i].symbol;
                contrib->u.size = parsed_symbol.value;
              }
            }
          }
        }
      }
      ProfEnd();

      if (common_block_contribs_count) {
        ProfBeginV("Assign Common Block Offsets [count %llu]", common_block_contribs_count);

        // search/push .data
        common_block_sect = lnk_section_table_search(sectab, str8_lit(".data"), PE_DATA_SECTION_FLAGS);
        if (common_block_sect == 0) {
          common_block_sect = lnk_section_table_push(sectab, str8_lit(".data"), PE_DATA_SECTION_FLAGS);
        }

        // sort common blocks from largest to smallest for tighter packing
        radsort(common_block_contribs, common_block_contribs_count, lnk_common_block_contrib_is_before);

        // compute common block offsets
        for (U64 contrib_idx = 0; contrib_idx < common_block_contribs_count; contrib_idx += 1) {
          LNK_CommonBlockContrib *contrib = &common_block_contribs[contrib_idx];
          U32 size = contrib->u.size;
          U32 align = Min(32, u64_up_to_pow2(size)); // link.exe caps align at 32 bytes
          common_block_sect->vsize = AlignPow2(common_block_sect->vsize, align);
          contrib->u.offset = common_block_sect->vsize;
          common_block_sect->vsize += size;
        }
        ProfEnd();
      }

      ProfEnd();
    }

    //
    // patch symbol tables
    //
    {
      ProfBegin("Patch Symbol Tables");
      Temp temp = temp_begin(scratch.arena);

      ProfBegin("Alloc Patch Flags");
      task.u.patch_symtabs.was_symbol_patched = push_array(scratch.arena, B8 *, objs_count);
      for (U64 obj_idx = 0; obj_idx < objs_count; obj_idx += 1) {
        task.u.patch_symtabs.was_symbol_patched[obj_idx] = push_array(temp.arena, B8, objs[obj_idx]->header.symbol_count);
      }
      task.u.patch_symtabs.common_block_sect     = common_block_sect;
      task.u.patch_symtabs.ranges                = tp_divide_work(scratch.arena, common_block_contribs_count, tp->worker_count);
      task.u.patch_symtabs.common_block_contribs = common_block_contribs;
      ProfEnd();

      ProfBegin("Patch Common Block Leaders");
      tp_for_parallel(tp, 0, tp->worker_count, lnk_patch_common_block_leaders_task, &task);
      ProfEnd();

      ProfBegin("Patch Regular Symbols");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_regular_symbols_task, &task);
      ProfEnd();

      ProfBegin("Patch Common blocks");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_common_block_symbols_task, &task);
      ProfEnd();

      ProfBegin("Patch Absolute Symbols");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_abs_symbols_task, &task);
      ProfEnd();

      ProfBegin("Patch Undefined Symbols");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_undefined_symbols_task, &task);
      ProfEnd();

      ProfBegin("Patch Weak Symbols With Strong Definition");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_weak_symbols_with_strong_definition_task, &task);
      ProfEnd();

      ProfBegin("Patch Weak Symbols With Fallback Definition");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_weak_symbols_with_fallback_definition_task, &task);
      ProfEnd();

      ProfBegin("Patch Weak Symbols With Undefined Tag");
      tp_for_parallel(tp, 0, task.objs_count, lnk_patch_weak_symbols_with_undefined_tag_task, &task);
      ProfEnd();

      temp_end(temp);
      ProfEnd();
    }

    // section list -> array
    task.image_sects = lnk_section_array_from_list(scratch.arena, sectab->list);

    expected_image_header_size = lnk_compute_win32_image_header_size(config, task.image_sects.count);

    // assign virtual space
    U64 voff_cursor = AlignPow2(expected_image_header_size + sizeof(COFF_SectionHeader), config->sect_align);
    for (U64 i = 0; i < task.image_sects.count; i += 1) {
      lnk_assign_section_virtual_space(task.image_sects.v[i], config->sect_align, &voff_cursor);
    }

    ProfBegin("Patch Virtual Offsets and SIzes in Obj Section Headers");
    tp_for_parallel(tp, 0, task.objs_count, lnk_patch_virtual_offsets_and_sizes_in_obj_section_headers_task, &task);
    ProfEnd();

    // build base relocs
    if (~config->flags & LNK_ConfigFlag_Fixed) {
      String8List base_relocs_data = lnk_build_base_relocs(tp, arena, config, objs_count, objs);
      if (base_relocs_data.total_size) {
        LNK_Section             *reloc          = lnk_section_table_push(sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
        LNK_SectionContribChunk *first_sc_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &reloc->contribs, 1);
        LNK_SectionContrib      *sc             = lnk_section_contrib_chunk_push(first_sc_chunk, 1);
        sc->data_list       = base_relocs_data.first;
        sc->align           = 1;
        sc->u.sort_idx_size = 0;
        sc->u.obj_idx       = max_U32;

        lnk_finalize_section_layout(sectab, reloc, config->file_align);
        lnk_assign_section_virtual_space(reloc, config->sect_align, &voff_cursor);
        lnk_assign_section_index(reloc, sectab->next_sect_idx++);

        task.image_sects           = lnk_section_array_from_list(scratch.arena, sectab->list);
        expected_image_header_size = lnk_compute_win32_image_header_size(config, task.image_sects.count);
      }
    }

    // assign file space
    U64 foff_cursor = AlignPow2(expected_image_header_size, config->file_align);
    for (U64 i = 0; i < task.image_sects.count; i += 1) {
      lnk_assign_section_file_space(task.image_sects.v[i], &foff_cursor);
    }

    ProfBegin("Patch File Offsets And Sizes In Section Headers");
    tp_for_parallel(tp, 0, task.objs_count, lnk_patch_file_offsets_and_sizes_in_obj_section_headers_task, &task);
    ProfEnd();
  }

  // build win32 image header
  {
    String8List image_header_data = lnk_build_win32_header(scratch.arena, symtab, config, task.image_sects, AlignPow2(expected_image_header_size, config->file_align));

    LNK_Section             *image_header_sect     = lnk_section_table_push(sectab, str8_lit(".rad_linker_image_header_section"), 0);
    LNK_SectionContribChunk *image_header_sc_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &image_header_sect->contribs, 1);
    LNK_SectionContrib      *image_header_sc       = lnk_section_contrib_chunk_push(image_header_sc_chunk, 1);

    image_header_sc->align     = config->file_align;
    image_header_sc->data_list = image_header_data.first;
    image_header_sc->u.size    = safe_cast_u32(image_header_data.total_size);

    lnk_finalize_section_layout(sectab, image_header_sect, config->file_align);
  }

  ProfBegin("Patch Section Symbols");
  tp_for_parallel(tp, 0, task.objs_count, lnk_patch_section_symbols_task, &task);
  ProfEnd();

  String8 image_data = {0};
  {
    ProfBegin("Image Fill");

    LNK_SectionArray sects = lnk_section_array_from_list(scratch.arena, sectab->list);

    U64 image_size = 0;
    for (U64 sect_idx = 0; sect_idx < sects.count; sect_idx += 1) {
      image_size += sects.v[sect_idx]->fsize;
    }

    image_data.size = image_size;
    image_data.str  = push_array_no_zero(arena->v[0], U8, image_size);

    for (U64 sect_idx = 0; sect_idx < sects.count; sect_idx += 1) {
      LNK_Section *sect = sects.v[sect_idx];

      if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
        // pick fill pick
        U8 fill_byte = 0;
        if (sect->flags & COFF_SectionFlag_CntCode) {
          fill_byte = lnk_code_align_byte_from_machine(config->machine);
        }

        // copy section contribution
        U64 prev_sc_opl = 0;
        for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
          for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1) {
            LNK_SectionContrib *sc = sc_chunk->v[sc_idx];

            // fill align bytes
            Assert(sc->u.off >= prev_sc_opl);
            U64 fill_size = sc->u.off - prev_sc_opl;
            MemorySet(image_data.str + sect->foff + prev_sc_opl, fill_byte, fill_size);
            prev_sc_opl = sc->u.off + sc->u.size;

            // copy contrib contents
            {
              U64 cursor = 0;
              for (String8Node *data_n = sc->data_list; data_n != 0; data_n = data_n->next) {
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
      ProfBegin("Patch Relocs");
      LNK_ObjRelocPatcher task = {0};
      task.image_data          = image_data;
      task.objs                = objs;
      task.image_base          = pe.image_base;
      task.image_section_table = image_section_table;
      tp_for_parallel(tp, 0, objs_count, lnk_obj_reloc_patcher, &task);
      ProfEnd();
    }

    // patch load config
    {
      LNK_Symbol *load_config_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME));
      if (load_config_symbol) {
        U64     load_config_foff   = lnk_file_off_from_symbol(image_section_table, load_config_symbol);
        String8 load_config_data   = str8_skip(image_data, load_config_foff);

        U32 load_config_size = 0;
        if (sizeof(load_config_size) <= load_config_data.size) {
          PE_DataDirectory *load_config_dir = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_LOAD_CONFIG);
          load_config_dir->virt_off  = lnk_virt_off_from_symbol(image_section_table, load_config_symbol);
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
      LNK_Symbol  *null_import_desc = lnk_symbol_table_searchf(symtab, LNK_SymbolScope_Defined, "__NULL_IMPORT_DESCRIPTOR");
      LNK_Symbol  *null_thunk_data  = lnk_symbol_table_searchf(symtab, LNK_SymbolScope_Defined, "\x7f%S_NULL_THUNK_DATA", lnk_get_image_name(config));
      if (idata_sect && null_import_desc && null_thunk_data) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_symbol_from_coff_symbol_idx(null_import_desc->u.defined.obj, null_import_desc->u.defined.symbol_idx);
        LNK_SectionContrib *idata_first_contrib     = lnk_get_first_section_contrib(idata_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_IMPORT);
        import_dir->virt_off  = image_section_table[idata_first_contrib->u.sect_idx + 1]->voff + idata_first_contrib->u.off;
        import_dir->virt_size = null_import_desc_parsed.value - idata_first_contrib->u.off;

        COFF_ParsedSymbol  null_thunk_data_parsed = lnk_parsed_symbol_from_coff_symbol_idx(null_thunk_data->u.defined.obj, null_thunk_data->u.defined.symbol_idx);
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
      LNK_Symbol  *null_import_desc = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, str8_lit("__NULL_DELAY_IMPORT_DESCRIPTOR"));
      LNK_Symbol  *last_null_thunk  = lnk_symbol_table_searchf(symtab, LNK_SymbolScope_Defined, "\x7f%S_NULL_THUNK_DATA_DLA", lnk_get_image_name(config));
      if (didat_sect && null_import_desc && last_null_thunk) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_symbol_from_coff_symbol_idx(null_import_desc->u.defined.obj, null_import_desc->u.defined.symbol_idx);
        LNK_SectionContrib *didat_first_contrib     = lnk_get_first_section_contrib(didat_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, pe, PE_DataDirectoryIndex_DELAY_IMPORT);
        import_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, didat_sect);
        import_dir->virt_size = lnk_get_section_contrib_size(didat_sect);
      }
    }

    // patch TLS
    {
      LNK_Symbol *tls_used_symbol = lnk_symbol_table_searchf(symtab, LNK_SymbolScope_Defined, MSCRT_TLS_SYMBOL_NAME);
      if (tls_used_symbol) {
        ProfBegin("Patch TLS");

        // find max align in .tls
        U64          tls_align = 0;
        LNK_Section *tls_sect  = lnk_section_table_search(sectab, str8_lit(".tls"), PE_TLS_SECTION_FLAGS);
        for (LNK_SectionContribChunk *sc_chunk = tls_sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
          for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1) {
            Assert(IsPow2(sc_chunk->v[sc_idx]->align));
            tls_align = Max(tls_align, sc_chunk->v[sc_idx]->align);
          }
        }

        // patch-in align
        U64 tls_header_foff = lnk_file_off_from_symbol(image_section_table, tls_used_symbol);
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
        tls_dir->virt_off  = lnk_virt_off_from_symbol(image_section_table, tls_used_symbol);
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
      LNK_Symbol *guid_pdb_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, str8_lit("RAD_LINK_PE_DEBUG_GUID_PDB"));
      LNK_Symbol *guid_rdi_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, str8_lit("RAD_LINK_PE_DEBUG_GUID_RDI"));

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
        U64   cv_guid_foff = lnk_file_off_from_symbol(image_section_table, guid_pdb_symbol);
        Guid *cv_guid  = str8_deserial_get_raw_ptr(image_data, cv_guid_foff, sizeof(*cv_guid));
        *cv_guid = config->guid;
      }

      if (guid_rdi_symbol) {
        U64   cv_guid_foff = lnk_file_off_from_symbol(image_section_table, guid_rdi_symbol);
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

internal String8List
lnk_build_rad_map(Arena *arena, String8 image_data, U64 thread_count, U64 objs_count, LNK_Obj **objs, LNK_LibList lib_index[LNK_InputSource_Count], LNK_SectionTable *sectab)
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
    str8_list_pushf(arena, &map, "%-8s %-8s %-8s %-8s %-16s %-8s %s\n", "FileOff", "VirtOff", "VirtSize", "FileSize", "Blake3", "SC", "Source");

    for (LNK_SectionContribChunk *sc_chunk = sect->contribs.first; sc_chunk != 0; sc_chunk = sc_chunk->next) {
      for (U64 sc_idx = 0; sc_idx < sc_chunk->count; sc_idx += 1) {
        Temp temp = temp_begin(scratch.arena);
        LNK_SectionContrib *sc = sc_chunk->v[sc_idx];

        U64        file_off   = image_section_table[sc->u.sect_idx]->foff + sc->u.off;
        U64        virt_off   = image_section_table[sc->u.sect_idx]->voff + sc->u.off;
        U64        virt_size  = sc->u.size;
        U64        file_size  = sc->u.size;
        String8    sc_data    = str8_substr(image_data, rng_1u64(file_off, file_off + virt_size));

        U128 sc_hash = {0};
        if (~sect->flags & COFF_SectionFlag_CntUninitializedData) {
          blake3_hasher hasher; blake3_hasher_init(&hasher);
          blake3_hasher_update(&hasher, sc_data.str, sc_data.size);
          blake3_hasher_finalize(&hasher, (U8 *)&sc_hash, sizeof(sc_hash));
        }

        String8 file_off_str  = push_str8f(temp.arena, "%08x",       file_off);
        String8 virt_off_str  = push_str8f(temp.arena, "%08x",       virt_off);
        String8 virt_size_str = push_str8f(temp.arena, "%08x",       virt_size);
        String8 file_size_str = push_str8f(temp.arena, "%08x",       file_size);
        String8 sc_hash_str   = push_str8f(temp.arena, "%08x%08x",   sc_hash.u64[0], sc_hash.u64[1]);
        String8 sc_idx_str    = push_str8f(temp.arena, "%llx",       sc_idx);
        String8 source_str;
        {
          String8List source_list = {0};

#if 0
          // location
          if (chunk->obj) {
            if (chunk->obj->lib_path.size) {
              String8 lib_name = chunk->obj->lib_path;
              lib_name         = str8_skip_last_slash(lib_name);
              lib_name         = str8_chop_last_dot(lib_name);

              String8 obj_name = chunk->obj->path;
              obj_name         = str8_skip_last_slash(obj_name);

              str8_list_pushf(temp.arena, &source_list, "%S:%S", lib_name, obj_name);
            } else {
              str8_list_push(temp.arena, &source_list, chunk->obj->path);
            }
          }
#else
          str8_list_pushf(temp.arena, &source_list, "<no_loc>");
#endif

          // string join
          source_str = str8_list_join(temp.arena, &source_list, &(StringJoin){.sep=str8_lit(" ")});
        }

        str8_list_pushf(arena, &map, "%-8S %-8S %-8S %-8S %-16S %-8S %S\n", file_off_str, virt_off_str, virt_size_str, file_size_str, sc_hash, sc_idx_str, source_str);

        temp_end(temp);
      }
    }
    str8_list_pushf(arena, &map, "\n");
  }
  ProfEnd();


  ProfBegin("SYMBOLS");
  str8_list_pushf(arena, &map, "# SYMBOLS\n");
  str8_list_pushf(arena, &map, "%-8s %s\n", "Sect:Idx", "Symbol");
  for (U64 obj_idx = 0; obj_idx < objs_count; obj_idx += 1) {
    LNK_Obj *obj = objs[obj_idx];
    
    COFF_ParsedSymbol symbol;
    for (U64 symbol_idx = 0; symbol_idx < obj->header.symbol_count; symbol_idx += (1 + symbol.aux_symbol_count)) {
      symbol = lnk_parsed_symbol_from_coff_symbol_idx(obj, symbol_idx);

      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular) {
        String8 sc = push_str8f(scratch.arena, "%x:%x", symbol.section_number, symbol.value);

        String8 lib_name = obj->lib_path;
        lib_name = str8_skip_last_slash(lib_name);
        lib_name = str8_chop_last_dot(lib_name);

        String8 obj_name = obj->path;
        obj_name = str8_skip_last_slash(obj_name);

        str8_list_pushf(arena, &map, "%-8S (%S%s%S) %S\n",
            sc,
            lib_name, lib_name.size ? ":" : "", obj_name,
            symbol.name);
      }
    }
  }
  str8_list_pushf(arena, &map, "\n");
  ProfEnd();


  ProfBegin("LIBS");
  for (U64 input_source = 0; input_source < LNK_InputSource_Count; ++input_source) {
    if (lib_index[input_source].count) {
      str8_list_pushf(arena, &map, "# LIBS (%S)\n", lnk_string_from_input_source(input_source));
      for (LNK_LibNode *lib_n = lib_index[input_source].first; lib_n != 0; lib_n = lib_n->next) {
        str8_list_pushf(arena, &map, "%S\n", lib_n->data.path);
      }
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
lnk_run(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config)
{
  ProfBeginFunction();

  Temp scratch = scratch_begin(tp_arena->v, tp_arena->count);

  //
  // Link Inputs
  //
  LNK_LinkContext link_ctx = lnk_build_link_context(tp, tp_arena, config);

  //
  // Image
  //
  LNK_ImageContext image_ctx = lnk_build_image(tp_arena, tp, config, link_ctx.symtab, link_ctx.objs_count, link_ctx.objs);

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
    String8List rad_map = lnk_build_rad_map(scratch.arena, image_ctx.image_data, config->worker_count, link_ctx.objs_count, link_ctx.objs, link_ctx.lib_index, image_ctx.sectab);
    lnk_write_data_list_to_file_path(config->rad_chunk_map_name, config->temp_rad_chunk_map_name, rad_map);
  }

  //
  // Import Library
  //
  if (config->build_imp_lib && (config->file_characteristics & PE_ImageFileCharacteristic_FILE_DLL)) {
    ProfBegin("Build Import Library");
    lnk_timer_begin(LNK_Timer_Lib);
    String8 linker_debug_symbols = lnk_make_linker_debug_symbols(scratch.arena, config->machine);
    String8List lib_list = pe_make_import_lib(tp_arena->v[0], config->machine, config->time_stamp, str8_skip_last_slash(config->image_name), linker_debug_symbols, link_ctx.export_symbol_list);
    lnk_write_data_list_to_file_path(config->imp_lib_name, str8_zero(), lib_list);
    lnk_timer_end(LNK_Timer_Lib);
    ProfEnd();
  }

  //
  // Debug Info
  //
  if (lnk_do_debug_info(config)) {
    ProfBegin("Debug Info");
    lnk_timer_begin(LNK_Timer_Debug);

    //
    // CodeView
    //
    LNK_CodeViewInput input = lnk_make_code_view_input(tp, tp_arena, config->lib_dir_list, link_ctx.objs_count, link_ctx.objs);
    CV_DebugT        *types = lnk_import_types(tp, tp_arena, &input);

    //
    // RDI
    //
    if (config->rad_debug == LNK_SwitchState_Yes) {
      lnk_timer_begin(LNK_Timer_Rdi);
      RDI_Arch         arch        = rdi_arch_from_coff_machine(config->machine);
      LNK_SectionArray image_sects = lnk_section_table_get_output_sections(scratch.arena, image_ctx.sectab);

      String8List rdi_data = lnk_build_rad_debug_info(tp,
                                                      tp_arena,
                                                      config->target_os,
                                                      arch,
                                                      config->image_name,
                                                      image_ctx.image_data,
                                                      image_sects,
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
        lnk_replace_type_names_with_hashes(tp, tp_arena, types[CV_TypeIndexSource_TPI], config->pdb_hash_type_names, config->pdb_hash_type_name_length, config->pdb_hash_type_name_map);
      }

      String8List pdb_data = lnk_build_pdb(tp,
                                           tp_arena,
                                           image_ctx.image_data,
                                           config,
                                           link_ctx.symtab,
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
  LNK_Config *config = lnk_config_from_argcv(scratch.arena, cmdline->argc, cmdline->argv);
  TP_Context *tp = tp_alloc(scratch.arena, config->worker_count, config->max_worker_count, config->shared_thread_pool_name);
  TP_Arena *tp_arena = tp_arena_alloc(tp);
  lnk_run(tp, tp_arena, config);
  scratch_end(scratch);
}

