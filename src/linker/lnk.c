// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// --- Build Options -----------------------------------------------------------

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD PE/COFF Linker"

#define ARENA_FREE_LIST 1
#define NO_ASYNC 1

// --- Code Base ---------------------------------------------------------------

#include "base/base_inc.h"
#include "x64/x64.h"
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
#include "llvm/llvm.h"
#include "dwarf/dwarf.h"
#include "dwarf/x64/dwarf_x64.h"

#include "base/base_inc.c"
#include "x64/x64.c"
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
#include "llvm/llvm.c"
#include "dwarf/x64/dwarf_x64.c"

// --- Third Party -------------------------------------------------------------

#include "base_ext/base_blake3.h"
#include "base_ext/base_blake3.c"

// --- Code Base Extensions ----------------------------------------------------

#include "base_ext/base_inc.h"
#include "thread_pool/thread_pool.h"
#include "codeview_ext/codeview.h"
#include "pdb_ext/msf_builder.h"

#include "base_ext/base_inc.c"
#include "thread_pool/thread_pool.c"
#include "codeview_ext/codeview.c"

// --- PDB Extensions- ---------------------------------------------------------

#include "pdb_ext/pdb.h"
#include "pdb_ext/pdb_helpers.h"
#include "pdb_ext/pdb_builder.h"

#include "pdb_ext/msf_builder.c"
#include "pdb_ext/pdb.c"
#include "pdb_ext/pdb_helpers.c"
// fwd decl: parallel radix sort (defined later in this TU) so pdb_builder.c can use it
internal void lnk_radix_sort_u64_pairs(TP_Context *tp, Arena *arena, U64 n, U64 *keys, U32 *vals);
#include "pdb_ext/pdb_builder.c"

// --- RDI ---------------------------------------------------------------------

#include "pdb/pdb_parse.h"
#include "rdi/rdi_local.h"
#include "rdi_make/rdi_make_local.h"
#include "rdi_from_coff/rdi_from_coff.h"
#include "rdi_from_codeview/rdi_from_codeview.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "arch/arch_inc.h"

#include "pdb/pdb_parse.c"
#include "rdi/rdi_local.c"
#include "rdi_make/rdi_make_local.c"
#include "rdi_from_coff/rdi_from_coff.c"
#include "rdi_from_codeview/rdi_from_codeview.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "arch/arch_inc.c"

// --- Linker ------------------------------------------------------------------

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
#include "codeview_ext/ifc.h"
#include "lnk_debug_info.h"
#include "lnk.h"

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
#include "codeview_ext/ifc.c"
#include "lnk_debug_info.c"

// -----------------------------------------------------------------------------

internal LNK_CmdLine
lnk_make_default_cmd_line(Arena *arena, LNK_CmdLine user_cmd_line)
{
  Temp scratch = scratch_begin(&arena, 1);
  LNK_CmdLine cmd_line = {0};

  char *default_opts[] = {
    "/ALIGN:4096",
    "/DEBUG:none",
    "/FILEALIGN:512",
    "/HIGHENTROPYVA",
    "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"",
    "/NXCOMPAT",
    "/LARGEADDRESSAWARE",
    "/PDBALTPATH:%_RAD_PDB_PATH%",
    "/PDBPAGESIZE:4096",
    (char*)str8f(scratch.arena, "/HEAP:%llu,%llu", MB(1), KB(4)).str,
    (char*)str8f(scratch.arena, "/STACK:%llu,%llu", MB(1), KB(4)).str,

    "/RAD_BOOT_MODE:LINKER",
    //"/RAD_BUILD_EXP",
    "/RAD_BUILD_IMPLIB",
    "/RAD_TPYE_HASH_ALG:BLAKE3",
    "/RAD_AGE:1",
    "/RAD_CHECK_UNUSED_DELAY_LOAD_DLL",
    "/RAD_DO_MERGE",
    "/RAD_ENV_LIB",
    "/RAD_EXE",
    "/RAD_GUID:imageblake3",
    "/RAD_LARGE_PAGES:no",
    "/RAD_LINK_VER:14.0",
    "/RAD_OS_VER:6.0",
    "/RAD_PAGE_SIZE:4096",
    "/RAD_PATH_STYLE:system",
    "/RAD_PDB_HASH_TYPE_NAMES:NONE",
    "/RAD_PDB_HASH_TYPE_NAME_LENGTH:8",
    "/RAD_DEBUGALTPATH:%_RAD_RDI_PATH%",
    "/RAD_MEMORY_MAP_FILES",
    "/RAD_MAP_LINES_FOR_UNRESOLVED_SYMBOLS",
    "/RAD_UNRESOLVED_SYMBOL_LIMIT:1000",
    "/RAD_UNRESOLVED_SYMBOL_REF_LIMIT:10",
    "/RAD_SORT_IMPORTS",
    (char*)str8f(scratch.arena, "/RAD_MT_PATH:%s",        LNK_MANIFEST_MERGE_TOOL_NAME).str,
    (char*)str8f(scratch.arena, "/RAD_DATA_DIR_COUNT:%u", PE_DataDirectoryIndex_COUNT).str,
  };

  char *push_opts[] = {
    "/MERGE:.xdata=.rdata",
    "/MERGE:.00cfg=.rdata",
    // TODO: .tls must be always first contribution in .data section because compiler generates TLS relative movs
    //"/MERGE:.tls=.data",
    "/MERGE:.idata=.data",
    "/MERGE:.didat=.data",
    "/MERGE:.edata=.rdata",
    "/MERGE:.RAD_LINK_PE_DEBUG_DIR=.rdata",
    "/MERGE:.RAD_LINK_PE_DEBUG_DATA=.rdata",

    "/RAD_REMOVE_SECTION:.debug",
    "/RAD_REMOVE_SECTION:.gehcont",
    "/RAD_REMOVE_SECTION:.gfids",
    "/RAD_REMOVE_SECTION:.gxfg",

    (char*)str8f(scratch.arena, "/RAD_WORKERS:%u", get_system_info()->logical_processor_count).str,

    // errors that are too verbose in release build
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Warning_UnknownSwitch    * (BUILD_DEBUG ? -1 : 1)).str,
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Warning_UnknownDirective * (BUILD_DEBUG ? -1 : 1)).str,
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%d", LNK_Error_InvalidTypeIndex   * (BUILD_DEBUG ? -1 : 1)).str,

    #if BUILD_DEBUG
    "/RAD_LOG:debug",
    "/RAD_LOG:io_write",
    #else
    (char*)str8f(scratch.arena, "/RAD_IGNORE:%u", LNK_Error_InvalidTypeIndex).str,
    #endif
  };

#define DefaultOpt(...) do {                                                                     \
  LNK_CmdLine parsed_cmd_line = lnk_cmd_line_from_stringf_windows_rules(arena, __VA_ARGS__);     \
  for EachNode(cmd, LNK_CmdOption, parsed_cmd_line.first_option) {                               \
    if (!lnk_cmd_line_has_switch(user_cmd_line, lnk_cmd_switch_type_from_string(cmd->string))) { \
      String8List value_strings = str8_list_copy(arena, &cmd->value_strings);                    \
      lnk_cmd_line_push_option_list(arena, &cmd_line, cmd->string, value_strings);               \
    }                                                                                            \
  }                                                                                              \
} while (0)

#define PushOpt(...) do {                                                                    \
  LNK_CmdLine parsed_cmd_line = lnk_cmd_line_from_stringf_windows_rules(arena, __VA_ARGS__); \
  lnk_cmd_line_concat_in_place(&cmd_line, &parsed_cmd_line);                                 \
} while (0)

  if (lnk_cmd_line_has_switch(user_cmd_line, LNK_CmdSwitch_Dll)) {
    DefaultOpt("/SUBSYSTEM:%S", pe_string_from_subsystem(PE_WindowsSubsystem_WINDOWS_GUI));
  }
  if (!lnk_cmd_line_has_switch(user_cmd_line, LNK_CmdSwitch_Brepro)) {
    DefaultOpt("/RAD_TIME_STAMP:%u", get_process_start_time_unix());
  }
  for EachIndex(i, ArrayCount(default_opts)) {
    DefaultOpt("%s", default_opts[i]);
  }

  for EachIndex(i, ArrayCount(push_opts)) {
    PushOpt("%s", push_opts[i]);
  }

  // when /FORCE is specified on the command line, do not stop on these errors
#if 0
  if (lnk_cmd_line_has_switch(cmd_line, LNK_CmdSwitch_Force)) {
    g_error_mode_arr[LNK_Error_UnresolvedSymbol] = LNK_ErrorMode_Continue;
  }
#endif

#undef DefaultOpt
#undef PushOpt
  scratch_end(scratch);
  return cmd_line;
}

internal LNK_Config *
lnk_config_from_argcv(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

  String8List raw_cmd_line = {0};
  for (U64 i = 1; i < cmdline->argc; i += 1) { str8_list_push(scratch.arena, &raw_cmd_line, str8_cstring(cmdline->argv[i])); }

#if PROFILE_TELEMETRY
  tmMessage(0, TMMF_ICON_NOTE, "Command Line: %.*s", str8_varg(str8_list_join(scratch.arena, &raw_cmd_line, &(StringJoin){ .sep = str8_lit_comp(" ") })));
#endif

  // make command line
  LNK_CmdLine cmd_line_msvc = {0};
  {
    String8List unwrapped_cmd_line = lnk_unwrap_rsp(scratch.arena, raw_cmd_line);
    LNK_CmdLine user_cmd_line      = lnk_cmd_line_parse_windows_rules(scratch.arena, unwrapped_cmd_line);
    user_cmd_line.raw_cmd_line     = raw_cmd_line;
    LNK_CmdLine default_cmd_line   = lnk_make_default_cmd_line(scratch.arena, user_cmd_line);
    lnk_cmd_line_concat_in_place(&cmd_line_msvc, &default_cmd_line);
    lnk_cmd_line_concat_in_place(&cmd_line_msvc, &user_cmd_line);
  }

  // init config
  LNK_Config *config = lnk_config_init(cmd_line_msvc);

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
  String8 work_dir = get_current_path(scratch.arena);
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
  ProcessLaunchParams launch_opts = {0};
  launch_opts.cmd_line               = cmd_line;
  launch_opts.inherit_env            = 1;
  launch_opts.consoleless            = 0;
  Process mt_handle = process_launch(&launch_opts);
  if (process_match(mt_handle, process_zero())) {
    lnk_error(LNK_Error_Mt, "unable to start process: %S", mt_path);
  } else {
    process_join(mt_handle, max_U64, 0);
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
    delete_file_at_path(linker_manifest_path);
    delete_file_at_path(merged_manifest_path);

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
  coff_obj_writer_push_symbol_weak(obj_writer, str8_lit(LNK_IMPORT_STUB), COFF_WeakExt_Null, null_abs);

  // push .debug$T sections with null leaf
  String8 null_debug_data;
  {
    String8 raw_null_leaf = cv_make_leaf(obj_writer->arena, CV_LeafKind_NOTYPE, str8(0,0), 1);

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
  PE_Resource *a = *(PE_Resource **)raw_a;
  PE_Resource *b = *(PE_Resource **)raw_b;
  Assert(a->id.type == COFF_ResourceIDType_String);
  Assert(b->id.type == COFF_ResourceIDType_String);
  int is_before = str8_is_before_case_sensitive(&a->id.u.string, &b->id.u.string);
  return is_before;
}

internal int
lnk_res_number_id_is_before(void *raw_a, void *raw_b)
{
  PE_Resource *a = *(PE_Resource **)raw_a;
  PE_Resource *b = *(PE_Resource **)raw_b;
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
          PE_ResourceArray named_array;
          PE_ResourceArray id_array;
          {
            Temp scratch2 = scratch_begin(&scratch.arena, 1);

            named_array = pe_resource_list_to_array(scratch2.arena, &res->u.dir->named_list);
            id_array    = pe_resource_list_to_array(scratch2.arena, &res->u.dir->id_list);

            PE_ResourcePtrArray named_ptr_array = pe_resource_ptr_from_array(scratch2.arena, named_array);
            PE_ResourcePtrArray id_ptr_array    = pe_resource_ptr_from_array(scratch2.arena, id_array);

            radsort(named_ptr_array.v, named_ptr_array.count, lnk_res_string_id_is_before);
            radsort(id_ptr_array.v,    id_ptr_array.count,    lnk_res_number_id_is_before);

            named_array = pe_resource_from_ptr_array(scratch.arena, named_ptr_array);
            id_array    = pe_resource_from_ptr_array(scratch.arena, id_ptr_array);

            scratch_end(scratch2);
          }

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
          coff_res->data_size = res->u.coff_res.data.size;
          coff_res->data_voff = 0; // relocated
          coff_res->code_page = 0; // TODO: whats this for?

          if (res->u.coff_res.data.size >= sizeof(U32)) {
            // emit symbol for resource data
            U32             resdat_off = safe_cast_u32(rsrc2->data.total_size);
            COFF_ObjSymbol *resdat     = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("resdat"), resdat_off, rsrc2);

            // emit reloc for 'data_voff'
            U64 apply_off   = rsrc1->data.total_size + OffsetOf(COFF_ResourceDataEntry, data_voff);
            U32 apply_off32 = safe_cast_u32(apply_off);
            coff_obj_writer_section_push_reloc_voff(obj_writer, rsrc1, apply_off32, resdat);
          }

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
                         MD5            *res_hash_array)
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
    checksum.len = sizeof(MD5);
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
  MD5            *res_hash_array = push_array(scratch.arena, MD5, res_data_list.node_count);
  U64 node_idx = 0;
  for (String8Node *node = res_data_list.first; node != 0; node = node->next, node_idx += 1) {
    res_hash_array[node_idx] = md5_from_data(node->string);
    pe_resource_dir_push_res_file(scratch.arena, root_dir, node->string);
  }
  
  // convert res paths to stable paths
  String8List stable_res_file_list = {0};
  for (String8Node *node = res_path_list.first; node != 0; node = node->next) {
    String8 stable_res_path = lnk_make_full_path(scratch.arena, system_path_style, work_dir, node->string);
    str8_list_push(scratch.arena, &stable_res_file_list, stable_res_path);
  }
  
  // convert res to obj
  ProcessInfo *process_info = get_process_info();
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
    String8List symbols = {0};
    
    str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_OBJNAME, cv_make_obj_name(scratch.arena, obj_name, 0)));
    str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_COMPILE3, lnk_make_linker_compile3(scratch.arena, machine)));

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
    str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_ENVBLOCK, cv_make_envblock(scratch.arena, env_list)));

    // TODO: emit S_SECTION and S_COFFGROUP
    // TODO: emit S_TRAMPOLINE
    
    debug_symbols = lnk_make_debug_s(scratch.arena, symbols);
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
  Arena *arena = arena_alloc(.name = "INPUTER");
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
  String8    full_path = full_path_from_path(scratch.arena, path);
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
    String8 full_path = full_path_from_path(scratch.arena, path);
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
    BucketNode *was_reported = hash_table_search_path(inputer->missing_lib_ht, path);
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

static LNK_LibMemberInfo *g_sort_lib_member_context;

force_inline int
lnk_lib_member_ref_is_before(void *raw_a, void *raw_b)
{
  LNK_LibMemberRef **a = raw_a, **b = raw_b;
  return lnk_symbol_is_before(g_sort_lib_member_context[(*a)->member_idx].link,
                              g_sort_lib_member_context[(*b)->member_idx].link);
}

force_inline int
lnk_import_ref_is_before(void *raw_a, void *raw_b)
{
  LNK_LibMemberRef **a_ptr = raw_a, **b_ptr = raw_b;
  LNK_LibMemberRef *a = *a_ptr, *b = *b_ptr;
  int cmp = u64_compar(&a->lib->input_idx, &b->lib->input_idx);
  if (cmp == 0) {
    cmp = u32_compar(&a->member_idx, &b->member_idx);
  }
#if LNK_PARANOID
  if (a != b) Assert(cmp != 0);
#endif
  return cmp < 0;
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

internal LNK_Link *
lnk_link_init(TP_Arena *arena, LNK_Config *config)
{
  LNK_Link *link = push_array(arena->v[0], LNK_Link, 1);
  link->arena                      = arena_alloc(.name = "LINK");
  link->last_symbol_input          = &link->objs.first;
  link->last_include               = &config->include_symbol_list.first;
  link->last_default_lib           = &config->input_default_lib_list.first;
  link->last_obj_lib               = &config->input_obj_lib_list.first;
  link->last_cmd_lib               = &config->input_list[LNK_Input_Lib].first;
  link->try_to_resolve_entry_point = 1;
  return link;
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

  LNK_ObjNode *new_objs = lnk_obj_from_input_many(tp, arena, config, new_input_objs.count, new_input_objs.v);

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
        DLLInsert(link->objs.first, link->objs.last, obj_with_includes->self, include_obj);
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

internal void
lnk_queue_lib_member(Arena                *arena,
                     HashMap              *imports_hm,
                     HashMap               lib_member_info_hm,
                     LNK_LibMemberRefList *queued_members,
                     LNK_Symbol           *link_symbol,
                     LNK_Lib              *lib,
                     LNK_LibMemberInfo    *member_infos,
                     U32                   member_idx)
{
  // associate link symbol to lib member
  for (LNK_Symbol *leader = link_symbol;;) {
    LNK_Symbol *slot = ins_atomic_ptr_eval_assign(&member_infos[member_idx].link, 0);

    // update slot symbol if it is empty or link symbol comes before symbol in the slot
    if (slot) {
      if (lnk_symbol_is_before(slot, leader)) {
        leader = slot;
      }
    } else {
      leader = link_symbol;
    }

    // try to insert back updated slot symbol
    LNK_Symbol *swap = ins_atomic_ptr_eval_cond_assign(&member_infos[member_idx].link, leader, 0);

    // exit if slot symbol was null
    if (swap == 0) {
      break;
    }
  }

  LNK_LibMemberRef *is_thunk_import;
  LNK_LibMemberRef *is_addr_import;
  if (str8_starts_with(link_symbol->name, str8_lit("__imp_"))) {
    is_thunk_import  = hash_map_search_string_raw(imports_hm, str8_skip(link_symbol->name, 6));
    is_addr_import   = hash_map_search_string_raw(imports_hm, link_symbol->name);
  } else {
    is_thunk_import  = hash_map_search_string_raw(imports_hm, link_symbol->name);
    is_addr_import   = hash_map_search_stringf_raw(imports_hm, "__imp_%S", link_symbol->name);
  }

  LNK_LibMemberRef *is_queued_import = is_thunk_import ? is_thunk_import :
                                       is_addr_import  ? is_addr_import  : 0;

  if (is_queued_import) {
    // do not queue second import member link -> flag member and continue
    U8                 flag                = str8_starts_with(link_symbol->name, str8_lit("__imp_")) ? LNK_LibMemberFlag_LinkedImp : LNK_LibMemberFlag_LinkedRegular;
    LNK_LibMemberInfo *import_member_infos = hash_map_search_raw_raw(&lib_member_info_hm, is_queued_import->lib);
    ins_atomic_u8_or(&import_member_infos[is_queued_import->member_idx].flags, flag);
  } else {
    B32 do_queue;
    if (str8_starts_with(link_symbol->name, str8_lit("__imp_"))) {
      U8 member_flags = ins_atomic_u8_or(&member_infos[member_idx].flags, LNK_LibMemberFlag_LinkedImp);
      do_queue = !(member_flags & LNK_LibMemberFlag_LinkedImp);
    } else {
      U8 flag = LNK_LibMemberFlag_LinkedRegular;
      U8 member_flags = ins_atomic_u8_or(&member_infos[member_idx].flags, LNK_LibMemberFlag_LinkedRegular);
      do_queue = !(member_flags & LNK_LibMemberFlag_LinkedRegular);
    }

    if (do_queue) {
      LNK_LibMemberRef *member_ref = push_array(arena, LNK_LibMemberRef, 1);
      member_ref->lib         = lib;
      member_ref->member_idx  = member_idx;
      member_ref->link_symbol = link_symbol;
      lnk_lib_member_ref_list_push_node(queued_members, member_ref);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_search_lib_task)
{
  LNK_SearchLibTask    *task             = raw_task;
  LNK_Lib              *lib              = task->lib;
  LNK_SymbolTable      *symtab           = task->symtab;
  B32                   search_anti_deps = task->search_anti_deps;
  LNK_LibMemberInfo    *lib_member_infos = task->lib_member_infos;
  LNK_LibMemberRefList *member_ref_list  = &task->member_ref_lists[task_id];

  // FRONTIER cursor: resume from where this worker last left off for this lib. search_chunks is
  // append-only across the lib-search loop, so slots before the cursor were already searched and
  // (search being a pure fn of (lib, symbol->name), member-queue dedup idempotent) can never resolve
  // a new member. reset_cursor (anti-dep mode flip) forces a full rescan from list first.
  LNK_SymbolHashTrieChunk *start_chunk = task->reset_cursor ? 0 : lib->search_cursor_chunk[task_id];
  U64                      start_idx   = task->reset_cursor ? 0 : lib->search_cursor_idx[task_id];

  LNK_SymbolHashTrieChunk *first_chunk = start_chunk ? start_chunk : symtab->search_chunks[task_id].first;
  LNK_SymbolHashTrieChunk *end_chunk   = symtab->search_chunks[task_id].last;
  U64                      end_count    = end_chunk ? end_chunk->count : 0;

  for (LNK_SymbolHashTrieChunk *c = first_chunk; c != 0; c = c->next) {
    U64 i_begin = (c == start_chunk) ? start_idx : 0;
    U64 i_end   = c->count;
    for (U64 i = i_begin; i < i_end; i += 1) {
      LNK_Symbol *symbol = c->v[i].symbol;

      // interp is cached on the symbol at push time, so the common case (resolved symbols, which
      // stay in search_chunks but can never match a lib) costs one field read instead of re-parsing
      // -- and page-faulting -- the COFF symbol record out of the mmap'd obj on every lib pass.
      COFF_SymbolValueInterpType symbol_interp = symbol->interp;
      if (symbol_interp == COFF_SymbolValueInterp_Undefined) {
        U32 member_idx;
        if (lnk_search_lib(lib, symbol->name, &member_idx)) {
          lnk_queue_lib_member(arena, task->imports_hm, task->link->lib_member_infos_hm, member_ref_list, symbol, lib, lib_member_infos, member_idx);
        }
      } else if (symbol_interp == COFF_SymbolValueInterp_Weak) {
        LNK_ObjSymbolRef   symbol_ref    = lnk_ref_from_symbol(symbol);
        COFF_ParsedSymbol  symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symbol_ref.obj, symbol_ref.symbol_idx);
        COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(symbol_parsed, symbol_ref.obj->header.is_big_obj);
        if (weak_ext->characteristics == COFF_WeakExt_SearchLibrary) {
          U32 member_idx;
          if (lnk_search_lib(lib, symbol->name, &member_idx)) {
            lnk_queue_lib_member(arena, task->imports_hm, task->link->lib_member_infos_hm, member_ref_list, symbol, lib, lib_member_infos, member_idx);
          }
        } else if (weak_ext->characteristics == COFF_WeakExt_AntiDependency) {
          if (search_anti_deps) {
            LNK_ObjSymbolRef dep_symbol = {0};
            if (lnk_resolve_weak_symbol(symtab, symbol_ref, &dep_symbol)) {
              COFF_ParsedSymbol          dep_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(dep_symbol.obj, dep_symbol.symbol_idx);
              COFF_SymbolValueInterpType dep_interp = coff_interp_from_parsed_symbol(dep_parsed);
              if (dep_interp == COFF_SymbolValueInterp_Weak) {
                U32 member_idx;
                if (lnk_search_lib(lib, symbol->name, &member_idx)) {
                  lnk_queue_lib_member(arena, task->imports_hm, task->link->lib_member_infos_hm, member_ref_list, symbol, lib, lib_member_infos, member_idx);
                }
              }
            }
          }
        }
      }
    }
  }

  // advance cursor to the end of search_chunks as observed at the start of this scan. the tp dispatch
  // is a barrier and load_inputs runs serially between dispatches, so no concurrent append occurs;
  // we stamp the (end_chunk, end_count) snapshot taken before the loop so any slot appended after the
  // snapshot is rescanned next time, never skipped.
  lib->search_cursor_chunk[task_id] = end_chunk;
  lib->search_cursor_idx[task_id]   = end_count;
}

internal LNK_Lib *
lnk_find_first_crt_lib(LNK_Config *config, LNK_Inputer *inputer)
{
  Temp scratch = scratch_begin(0, 0);

  LNK_Lib *result = 0;

  String8 crt_lib_names[] = {
    str8_lit("msvcrt"),
    str8_lit("msvcrtd"),
    str8_lit("libcmt"),
    str8_lit("libcmtd"),
  };

  for EachNode(n, LNK_Input, inputer->libs.first) {
    String8 lib_name = str8_chop_last_dot(str8_skip_last_slash(n->path));
    for EachElement(i, crt_lib_names) {
      if (str8_match(lib_name, crt_lib_names[i], StringMatchFlag_CaseInsensitive)) {
        if (result == 0) {
          result = hash_table_search_path_raw(inputer->libs_ht, n->path);
          break;
        } else {
          LNK_Lib *lib = hash_table_search_path_raw(inputer->libs_ht, n->path);
          result = lib->input_idx < result->input_idx ? lib : result;
          break;
        }
      }
    }
  }

  scratch_end(scratch);
  return result;
}

internal void
lnk_link_inputs(TP_Context      *tp,
                TP_Arena         *arena,
                LNK_Config       *config,
                LNK_Inputer      *inputer,
                LNK_SymbolTable  *symtab,
                LNK_Link         *link)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  HashMap imports_hm = {0};

  LNK_LibMemberRefList *member_ref_lists = push_array(scratch.arena, LNK_LibMemberRefList, tp->worker_count);
  B32                   search_anti_deps = 0;
  for (U64 resolved_members_count = 0; ; resolved_members_count = 0) {
    lnk_load_inputs(tp, arena, config, inputer, symtab, link);

    for EachNode(lib_n, LNK_LibNode, link->libs.first) {
      LNK_Lib *lib = &lib_n->data;

      if (config->machine != COFF_MachineType_Unknown) {
        if (config->infer_asan_libs == LNK_SwitchState_Yes) {
          if ( ! link->asan_libs_resolved) {
            LNK_Lib *crt_lib = lnk_find_first_crt_lib(config, inputer);
            if (crt_lib != 0) {
              String8 crt_lib_name = str8_chop_last_dot(str8_skip_last_slash(crt_lib->path));

              String8 arch_name = {0};
              if (config->machine == COFF_MachineType_X64) {
                arch_name = str8_lit("x86_64");
              } else if (config->machine == COFF_MachineType_X86) {
                arch_name = str8_lit("i386");
              }

              if (arch_name.size) {                
                B32 link_vc_libs = lnk_symbol_table_searchf(symtab, "__you_must_link_with_VCAsan_lib")  != 0 ||
                                   lnk_symbol_table_searchf(symtab, "___you_must_link_with_VCAsan_lib") != 0;
                if (str8_match(crt_lib_name, str8_lit("msvcrt"), StringMatchFlag_CaseInsensitive) || str8_match(crt_lib_name, str8_lit("msvcrtd"), StringMatchFlag_CaseInsensitive)) {
                  String8 dynamic_lib_name = str8f(inputer->arena, "clang_rt.asan_dynamic-%S.lib", arch_name);
                  String8 thunk_lib_name   = str8f(inputer->arena, "clang_rt.asan_dynamic_runtime_thunk-%S.lib", arch_name);
                  lnk_whole_archive(config, thunk_lib_name);
                  lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, dynamic_lib_name);
                  lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, thunk_lib_name);
                  if (link_vc_libs) {
                    if (str8_match(crt_lib_name, str8_lit("msvcrtd"), StringMatchFlag_CaseInsensitive)) {
                      lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, str8_lit("libvcasand.lib"));
                    } else {
                      lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, str8_lit("libvcasan.lib"));
                    }
                  }
                } else if (str8_match(crt_lib_name, str8_lit("libcmt"), StringMatchFlag_CaseInsensitive) || str8_match(crt_lib_name, str8_lit("libcmtd"), StringMatchFlag_CaseInsensitive)) {
                  String8 dynamic_lib_name = str8f(inputer->arena, "clang_rt.asan_dynamic-%S.lib", arch_name);
                  String8 thunk_lib_name   = str8f(inputer->arena, "clang_rt.asan_static_runtime_thunk-%S.lib", arch_name);
                  lnk_whole_archive(config, thunk_lib_name);
                  lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, dynamic_lib_name);
                  lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, thunk_lib_name);
                  if (link_vc_libs) {
                    if (str8_match(crt_lib_name, str8_lit("libcmtd"), StringMatchFlag_CaseInsensitive)) {
                      lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, str8_lit("vcasand.lib"));
                    } else {
                      lnk_inputer_push_lib_thin(inputer, config, LNK_InputSource_Obj, str8_lit("vcasan.lib"));
                    }
                  }
                }
                link->asan_libs_resolved = 1;
              }
            }
          }
        }
      }

      LNK_LibMemberInfo *lib_member_infos = hash_map_search_raw_raw(&link->lib_member_infos_hm, lib);
      if (lib_member_infos == 0) {
        lib_member_infos = push_array(link->arena, LNK_LibMemberInfo, lib->member_count);
        hash_map_push_raw_raw(link->arena, &link->lib_member_infos_hm, lib, lib_member_infos);
      }

      B32 link_whole_archive = config->whole_archive_all;
      if ( ! link_whole_archive) {
        String8 lib_name = str8_chop_last_dot(str8_skip_last_slash(lib->path));
        link_whole_archive = hash_map_search_path_u64(&config->whole_archive_ht, lib_name) != 0;
      }

      ProfBeginV("Search %S", str8_skip_last_slash(lib->path));
      do {
        lnk_load_inputs(tp, arena, config, inputer, symtab, link);

        if (link_whole_archive) {
          local_persist LNK_Symbol *null_symbol = 0;
          if (null_symbol == 0) {
            null_symbol                   = push_array(inputer->arena, LNK_Symbol, 1);
            null_symbol->first_ref        = push_array(inputer->arena, LNK_ObjSymbolRefNode, 1);
            null_symbol->first_ref->v.obj = &link->objs.first->data;
          }
          LNK_LibMemberRef *member_refs = push_array(scratch.arena, LNK_LibMemberRef, lib->member_count);
          for EachIndex(member_idx, lib->member_count) {
            lnk_queue_lib_member(arena->v[0], &imports_hm, link->lib_member_infos_hm, &member_ref_lists[0], null_symbol, lib, lib_member_infos, member_idx);
          }
        } else {
          // search symbols in lib
          MemoryZeroTyped(member_ref_lists, tp->worker_count);

          // barrier elision: the search task scans every undefined/weak symbol in search_chunks and
          // queues any member that resolves one. search_chunks only grows during this loop and the
          // dedup against already-queued members is idempotent, so if neither the symbol set nor the
          // anti-dep mode changed since this lib was last searched, the dispatch can only re-queue
          // (deduped to) nothing. skipping it avoids waking+joining every worker for no work.
          U64 search_symbol_count = lnk_symbol_table_search_symbol_count(symtab);
          B32 can_skip_search = lib->was_searched &&
                                lib->searched_symbol_count == search_symbol_count &&
                                lib->searched_anti_deps    == search_anti_deps;
          if ( ! can_skip_search) {
            // lazily alloc the per-worker frontier cursor arrays (cleared = unsearched/start).
            if (lib->search_cursor_chunk == 0) {
              lib->search_cursor_chunk = push_array(link->arena, LNK_SymbolHashTrieChunk *, tp->worker_count);
              lib->search_cursor_idx   = push_array(link->arena, U64,                       tp->worker_count);
            }

            // anti-dep mode flipped since last search of this lib: anti-dep weak symbols before the
            // cursor were skipped under the old mode, so they must be re-tried -> rescan from start.
            B32 reset_cursor = lib->was_searched && lib->searched_anti_deps != search_anti_deps;

            LNK_SearchLibTask search_task = {
              .search_anti_deps = search_anti_deps,
              .reset_cursor     = reset_cursor,
              .link             = link,
              .imports_hm       = &imports_hm,
              .lib              = lib,
              .symtab           = symtab,
              .lib_member_infos = lib_member_infos,
              .member_ref_lists = member_ref_lists
            };
            tp_for_parallel(tp, arena, tp->worker_count, lnk_search_lib_task, &search_task);

            lib->was_searched          = 1;
            lib->searched_symbol_count = search_symbol_count;
            lib->searched_anti_deps    = search_anti_deps;
          }
        }

        LNK_LibMemberRefList queued_members = {0};
        lnk_lib_member_ref_list_concat_in_place_array(&queued_members, member_ref_lists, tp->worker_count);

        // sort library member refs to match the order of their appearance in obj symbol tables
        LNK_LibMemberRef **member_refs = lnk_array_from_lib_member_list(scratch.arena, queued_members);
        g_sort_lib_member_context = lib_member_infos;
        radsort(member_refs, queued_members.count, lnk_lib_member_ref_is_before);

        if (queued_members.count) {
          lnk_log(LNK_Log_Links, "Searching %S:", lib_n->data.path);

          for EachIndex(i, queued_members.count) {
            Temp temp = temp_begin(scratch.arena);

            LNK_LibMemberRef *member_ref  = member_refs[i];
            LNK_Lib          *lib         = member_ref->lib;
            LNK_Symbol       *link_symbol = member_ref->link_symbol;

            U32                member_offset = memory_read32(lib->member_offsets + member_ref->member_idx);
            COFF_ArchiveMember member_info   = coff_archive_member_from_offset(lib->data, member_offset);
            COFF_DataType      member_type   = coff_data_type_from_data(member_info.data);
            String8            member_name   = coff_decode_member_name(lib->long_names, member_info.header.name);

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
          LNK_LibMemberRef *member_ref = member_refs[i];
          U64               member_idx = member_ref->member_idx;

          // parse member info
          U32                member_offset = memory_read32(lib->member_offsets + member_idx);
          COFF_ArchiveMember member_info   = coff_archive_member_from_offset(lib->data, member_offset);
          COFF_DataType      member_type   = coff_data_type_from_data(member_info.data);
          String8            member_name   = coff_decode_member_name(lib->long_names, member_info.header.name);

          switch (member_type) {
          case COFF_DataType_Import: {
            {
              LNK_LibMemberRef *is_thunk_import;
              LNK_LibMemberRef *is_addr_import;
              if (str8_starts_with(member_ref->link_symbol->name, str8_lit("__imp_"))) {
                is_thunk_import  = hash_map_search_string_raw(&imports_hm, str8_skip(member_ref->link_symbol->name, 6));
                is_addr_import   = hash_map_search_string_raw(&imports_hm, member_ref->link_symbol->name);
              } else {
                is_thunk_import  = hash_map_search_string_raw(&imports_hm, member_ref->link_symbol->name);
                is_addr_import   = hash_map_search_stringf_raw(&imports_hm, "__imp_%S", member_ref->link_symbol->name);
              }
              if (is_thunk_import != 0 || is_addr_import != 0) {
                lnk_log(LNK_Log_Paranoid, "duplicate import member queue detected: %S", member_ref->link_symbol->name);
                break;
              }
            }

            // store lib member ref to import
            hash_map_push_string_raw(scratch.arena, &imports_hm, member_ref->link_symbol->name, member_ref);

            // find import stub
            LNK_Symbol *import_stub = lnk_symbol_table_search(symtab, str8_lit(LNK_IMPORT_STUB));

            // same import symbol must never be queued more than once, if it is, there is a bug in the link set logic
            AssertAlways(member_ref->link_symbol->first_ref != import_stub->first_ref);

            // replace the import symbol with a stub, which is later replaced with the real import symbol once import obj is ready.
            member_ref->link_symbol->first_ref = import_stub->first_ref;
            member_ref->link_symbol->last_ref = import_stub->last_ref;

            // push import member for import obj generation
            lnk_lib_member_ref_list_push_node(&link->imports, member_ref);
            lib_member_infos[member_ref->member_idx].flags |= LNK_LibMemberFlag_WasGenQueued;
          } break;
          case COFF_DataType_BigObj:
          case COFF_DataType_Obj: {
            if (lib->type == COFF_Archive_Thin) {
              Assert(!(lib_member_infos[member_ref->member_idx].flags & LNK_LibMemberFlag_WasGenQueued));
              lib_member_infos[member_ref->member_idx].flags |= LNK_LibMemberFlag_WasGenQueued;

              // obj path in thin archive is relative to the directory with lib
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


internal LNK_LinkResult
lnk_link_image(TP_Context *tp, TP_Arena *arena, LNK_Config *config, LNK_Inputer *inputer, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  LNK_Link *link = lnk_link_init(arena, config); // TODO: factor out

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
  lnk_link_inputs(tp, arena, config, inputer, symtab, link);

  // TODO: need to figure out under what condition to include load config
  //lnk_include_symbol(config, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME), 0);

  {
    ProfBegin("Push Linker Symbols");
    String8 linker_symbols_obj = lnk_make_linker_obj(arena->v[0], config);
    lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Linker Symbols *"), linker_symbols_obj);
    ProfEnd();
  }

  //
  // make imports
  //
  {
    HashMap     static_imports_hm  = {0};
    HashMap     delayed_imports_hm = {0};
    String8List delayed_dll_names  = {0};
    String8List static_dll_names   = {0};

    LNK_LibMemberRef **import_member_refs = lnk_array_from_lib_member_list(scratch.arena, link->imports);

    // optionally sort import library member refs by input index
    if (config->sort_imports == LNK_SwitchState_Yes) {
      radsort(import_member_refs, link->imports.count, lnk_import_ref_is_before);
    }

    for EachIndex(import_member_idx, link->imports.count) {
      LNK_LibMemberRef  *member_ref   = import_member_refs[import_member_idx];
      LNK_Lib           *lib          = member_ref->lib;
      U64                member_idx   = member_ref->member_idx;
      LNK_LibMemberInfo *member_infos = hash_map_search_raw_raw(&link->lib_member_infos_hm, lib);
      LNK_Symbol        *link_symbol  = member_infos[member_idx].link;

      U32                            member_offset = memory_read32(lib->member_offsets + member_idx);
      COFF_ArchiveMember             member_info   = coff_archive_member_from_offset(lib->data, member_offset);
      COFF_DataType                  member_type   = coff_data_type_from_data(member_info.data);
      String8                        member_name   = coff_decode_member_name(lib->long_names, member_info.header.name);
      COFF_ParsedArchiveImportHeader import_header = coff_archive_import_from_data(member_info.data);

      // import machine compat check
      if (import_header.machine != config->machine) {
        LNK_ObjSymbolRef ref = lnk_ref_from_symbol(link_symbol);
        lnk_error_obj(LNK_Error_IncompatibleMachine,
                      ref.obj,
                      "symbol %S pulls-in import from %S with an incompatible machine %S (expected machine %S)",
                      link_symbol->name,
                      str8_chop_last_slash(lib->path),
                      coff_string_from_machine_type(import_header.machine),
                      coff_string_from_machine_type(config->machine));
        break;
      }

      // find DLL with import symbols
      B32                is_delay_load  = lnk_is_dll_delay_load(config, import_header.dll_name);
      String8List       *dll_names      = is_delay_load ? &delayed_dll_names : &static_dll_names;
      HashMap           *imports_hm     = is_delay_load ? &delayed_imports_hm : &static_imports_hm;
      PE_MakeImportList *import_symbols = hash_map_search_path_raw(imports_hm, import_header.dll_name);

      // create record for a first time-DLL
      if (import_symbols == 0) {
        import_symbols = push_array(scratch.arena, PE_MakeImportList, 1);
        str8_list_push(scratch.arena, dll_names, import_header.dll_name);
        hash_map_push_path_raw(scratch.arena, imports_hm, import_header.dll_name, import_symbols);
      }

      // push make import info
      B32 make_jump_thunk = !!(member_infos[member_idx].flags & LNK_LibMemberFlag_LinkedRegular);
      pe_make_import_header_list_push(scratch.arena, import_symbols, (PE_MakeImport){ .header = member_info.data, .make_jump_thunk = make_jump_thunk });
    }
    AssertAlways(delayed_dll_names.node_count == delayed_imports_hm.count);
    AssertAlways(static_dll_names.node_count == static_imports_hm.count);

    // make and input delayed imports
    if (delayed_imports_hm.count) {
      ProfBegin("Build Delay Import Table");

      COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;
      B32            emit_biat  = config->import_table_emit_biat == LNK_SwitchState_Yes;
      B32            emit_uiat  = config->import_table_emit_uiat == LNK_SwitchState_Yes;

      for EachNode(name, String8Node, delayed_dll_names.first) {
        PE_MakeImportList *imports              = hash_map_search_path_raw(&delayed_imports_hm, name->string);
        String8            import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, name->string);
        String8            import_obj           = pe_make_import_dll_obj_delayed(arena->v[0], time_stamp, config->machine, name->string, config->delay_load_helper_name, import_debug_symbols, *imports, emit_biat, emit_uiat);
        lnk_inputer_push_obj(inputer, 0, str8f(inputer->arena, "Import:%S", name->string), import_obj);
      }

      String8 linker_debug_symbols = lnk_make_linker_debug_symbols(arena->v[0], config->machine);
      String8 null_desc_obj        = pe_make_null_import_descriptor_delayed(arena->v[0], time_stamp, config->machine, linker_debug_symbols);
      String8 null_thunk_obj       = pe_make_null_thunk_data_obj_delayed(arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols);
      lnk_inputer_push_obj(inputer, 0, str8_lit("* Delayed Null Import Descriptor *"), null_desc_obj);
      lnk_inputer_push_obj(inputer, 0, str8_lit("* Delayed Null Thunk Data *"),        null_thunk_obj);

      ProfEnd();
    }

    // make and input static imports
    if (static_imports_hm.count) {
      ProfBegin("Build Static Import Table");

      COFF_TimeStamp time_stamp = COFF_TimeStamp_Max;

      for (String8Node *dll_name_n = static_dll_names.first; dll_name_n != 0; dll_name_n = dll_name_n->next) {
        PE_MakeImportList *imports              = hash_map_search_path_raw(&static_imports_hm, dll_name_n->string);
        String8            import_debug_symbols = lnk_make_dll_import_debug_symbols(scratch.arena, config->machine, dll_name_n->string);
        String8            import_obj           = pe_make_import_dll_obj_static(arena->v[0], time_stamp, config->machine, dll_name_n->string, import_debug_symbols, *imports);
        lnk_inputer_push_obj(inputer, 0, str8f(inputer->arena, "Import:%S", dll_name_n->string), import_obj);
      }

      String8 linker_debug_symbols = lnk_make_linker_debug_symbols(scratch.arena, config->machine);
      String8 null_desc_obj        = pe_make_null_import_descriptor_obj(arena->v[0], time_stamp, config->machine, linker_debug_symbols);
      String8 null_thunk_obj       = pe_make_null_thunk_data_obj(arena->v[0], lnk_get_image_name(config), time_stamp, config->machine, linker_debug_symbols);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null Import Descriptor *"), null_desc_obj);
      lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null Thunk Data *"),        null_thunk_obj);

      ProfEnd();
    }
    
    // warn about unused delayloads
    if (config->flags & LNK_ConfigFlag_CheckUnusedDelayLoadDll) {
      for EachNode(name, String8Node, config->delay_load_dll_list.first) {
        if (!hash_map_search_path_raw(&delayed_imports_hm, name->string)) {
          lnk_error(LNK_Warning_UnusedDelayLoadDll, "/DELAYLOAD: found no imports in %S", name->string);
        }
      }
    }
  }

  if (config->export_symbol_list.count) {
    ProfBegin("Build Export Table");

    PE_ExportParseList resolved_exports = {0};
    for (PE_ExportParseNode *exp_n = config->export_symbol_list.first, *exp_n_next; exp_n != 0; exp_n = exp_n_next) {
      exp_n_next = exp_n->next;
      PE_ExportParse *exp = &exp_n->data;

      if (exp->name.size && str8_match(exp->name, config->entry_point_name, 0)) {
        lnk_error_with_loc(LNK_Warning_TryingToExportEntryPoint, exp->obj_path, exp->lib_path, "exported entry point \"%S\"", exp->name);
      }
      if (exp->alias.size && str8_match(exp->alias, config->entry_point_name, 0)) {
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
    String8             edata_obj         = pe_make_edata_obj(arena->v[0], lnk_get_image_name(config), COFF_TimeStamp_Max, config->machine, finalized_exports);
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
      String8 obj_data     = lnk_make_linker_coff_obj(arena->v[0], config->time_stamp, config->machine, config->work_dir, lnk_get_image_name(config), config->pdb_name, raw_cmd_line, obj_name);
      lnk_inputer_push_obj_linkgen(inputer, 0, obj_name, obj_data);
      ProfEnd();
    }

    ProfBegin("Build * Debug Directories *");
    if (lnk_do_debug_info(config)) {
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
  lnk_link_inputs(tp, arena, config, inputer, symtab, link);

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

    if (unresolved_symbols_count) {
      Temp debug_scratch = scratch_begin(&scratch.arena, 1);

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

            Temp debug_temp = temp_begin(debug_scratch.arena);
            CV_DebugS           debug_s         = {0};
            CV_LinesAccel      *debug_lines     = 0;
            String8             debug_checksums = {0};
            String8             debug_strings   = {0};

            for EachIndex(sect_idx, obj->header.section_count_no_null) {
              if (obj->section_flags[sect_idx] & LNK_SECTION_FLAG_DEBUG) { continue; }

              COFF_SectionHeader *section_header = &section_table[sect_idx];
              String8             section_name   = coff_name_from_section_header(string_table, section_header);
              U64                 section_number = sect_idx+1;
              COFF_RelocArray     relocs         = lnk_coff_relocs_from_section_header(obj, section_header);
              for EachIndex(reloc_idx, relocs.count) {
                if (supp_info.node_count > config->unresolved_symbol_ref_limit) {
                  str8_list_pushf(scratch.arena, &supp_info, "too many unresolved symbol references reported, stopping now");
                  temp_end(debug_temp);
                  goto next_undefined_symbol;
                }
                COFF_Reloc *reloc = &relocs.v[reloc_idx];
                if (reloc->isymbol == ref->symbol_idx) {
                  U64      line_matches_count = 0;
                  CV_Line *line_matches       = 0;
                  if (config->map_lines_for_unresolved_symbols == LNK_SwitchState_Yes) {
                    if (debug_lines == 0) {
                      debug_s = lnk_debug_s_from_obj(debug_temp.arena, obj);
                      String8List raw_checksums = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_FileChksms);
                      String8List raw_strings   = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_StringTable);
                      debug_lines     = cv_lines_accel_from_debug_s(debug_temp.arena, debug_s);
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
                      str8_list_pushf(scratch.arena, &supp_info, "%S: %S:%u", lnk_loc_from_obj(debug_temp.arena, obj), file_name, line.line_num);
                    }
                  } else {
                    str8_list_pushf(scratch.arena, &supp_info, "%S: %S(%llx)+%x", lnk_loc_from_obj(debug_temp.arena, obj), section_name, section_number, reloc->apply_off);
                  }
                }
              }
            }

            temp_end(debug_temp);
          }
          next_undefined_symbol:;
        }

        lnk_error(LNK_Error_UnresolvedSymbol, "unresolved symbol %S", symbol->name);
        lnk_supplement_error_list(supp_info);
      }

      scratch_end(debug_scratch);
    }

    // TODO: /FORCE
    if (unresolved_symbols_count) {
      lnk_exit(LNK_Error_UnresolvedSymbol);
    }

    ProfEnd();
  }

  //
  // fold identical code COMDATs (routes followers to a leader; /OPT:REF then GCs them)
  //
  if (config->opt_icf == LNK_SwitchState_Yes) {
    lnk_opt_icf(tp, arena->v[0], symtab, config, link->objs);
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
  // fill out result
  //
  LNK_LinkResult result = {0};
  result.objs = link->objs;
  result.libs = link->libs;

  //
  // release link context
  //
  arena_release(link->arena);

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
  return result;
}

internal void
lnk_reloc_ref_batch_list_push_node(LNK_RelocRefsBatchList *list, LNK_RelocRefsBatch *node)
{
  LNK_RelocRefsBatchPointer old_head = list->head;
  node->next = old_head.node;
  list->head = (LNK_RelocRefsBatchPointer){ .node = node, .tag = old_head.tag + 1 };
}

internal void
lnk_reloc_ref_batch_list_push(Arena *arena, LNK_RelocRefsBatchList *list, LNK_RelocRefs v)
{
  LNK_RelocRefsBatch *batch = list->head.node;
  if (list->head.node == 0 || list->head.node->count >= ArrayCount(list->head.node->v)) {
    batch = push_array(arena, LNK_RelocRefsBatch, 1);
    lnk_reloc_ref_batch_list_push_node(list, batch);
  }
  batch->v[batch->count++] = v;
}

internal LNK_RelocRefsBatch *
lnk_reloc_ref_batch_list_pop(LNK_RelocRefsBatchList *list)
{
  LNK_RelocRefsBatchPointer old_head = list->head;
  if (old_head.node) {
    list->head = (LNK_RelocRefsBatchPointer){ .node = old_head.node->next, .tag = old_head.tag + 1};
  }
  return old_head.node;
}

internal LNK_RelocRefsBatch *
lnk_reloc_ref_batch_list_pop_atomic(LNK_RelocRefsBatchList *list)
{
  LNK_RelocRefsBatchPointer old_head = { .node = ins_atomic_ptr_eval(&list->head.node), .tag = ins_atomic_u64_eval(&list->head.tag) };
  for (;;) {
    if (old_head.node == 0) { break; }
    LNK_RelocRefsBatchPointer new_head = { .node = old_head.node->next, .tag = old_head.tag + 1 };
    if (ins_atomic_u128_eval_cond_assign(&list->head, &new_head, &old_head)) { break; }
  }
  return old_head.node;
}

internal void
lnk_reloc_ref_batch_list_concat_in_place_atomic(LNK_RelocRefsBatchList *list, LNK_RelocRefsBatch *first, LNK_RelocRefsBatch *last)
{
  LNK_RelocRefsBatchPointer old_head = { .node = ins_atomic_ptr_eval(&list->head.node), .tag = ins_atomic_u64_eval(&list->head.tag) };
  for (;;) {
    last->next = old_head.node;
    LNK_RelocRefsBatchPointer new_head = { .node = first, .tag = old_head.tag + 1 };
    if (ins_atomic_u128_eval_cond_assign(&list->head, &new_head, &old_head)) { break; }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_walk_relocs_and_mark_ref_sections_task)
{
  ProfBeginFunction();

  Temp scratch  = scratch_begin(0,0);
  Temp scratch2 = scratch_begin(&scratch.arena, 1);

  LNK_OptRefTask  *task   = raw_task;
  LNK_SymbolTable *symtab = task->symtab;
  LNK_Config      *config = task->config;
  LNK_ObjList      objs   = task->objs;

  U8                    **is_live             = 0;
  LNK_Obj               **objs_by_idx         = 0; // input_idx -> obj, for /OPT:ICF static-fold redirect
  U64                    *active_thread_count = 0;
  LNK_RelocRefsBatchList  *global_batch_list   = 0;
  if (task_id == 0) {
    active_thread_count = push_array(scratch.arena, U64,                   1);
    global_batch_list   = push_array(scratch.arena, LNK_RelocRefsBatchList, 1);

    // alloc live flags and set live status on every non-COMDAT section
    is_live     = push_array_no_zero(scratch.arena, U8 *,      objs.count);
    objs_by_idx = push_array_no_zero(scratch.arena, LNK_Obj *, objs.count ? objs.count : 1);
    {
      U64 obj_idx = 0;
      for EachNode(n, LNK_ObjNode, task->objs.first) {
        is_live[obj_idx]     = push_array(scratch.arena, U8, n->data.header.section_count_no_null + 1);
        objs_by_idx[obj_idx] = &n->data;

        for EachIndex(sect_idx, n->data.header.section_count_no_null) {
          is_live[obj_idx][sect_idx + 1] = !(n->data.section_flags[sect_idx] & COFF_SectionFlag_LnkCOMDAT);
        }

        obj_idx += 1;
      }
    }

    // define roots
    {
      // tls
      LNK_Symbol *tls_symbol = lnk_symbol_table_searchf(symtab, MSCRT_TLS_SYMBOL_NAME);
      if (tls_symbol) {
        lnk_include_symbol(config, str8_lit(MSCRT_TLS_SYMBOL_NAME), 0);
      }

      // push tasks for each root symbol
      for EachNode(root_n, LNK_IncludeSymbolNode, config->include_symbol_list.first) {
        LNK_Symbol       *root     = lnk_symbol_table_search(symtab, root_n->v.name);
        LNK_ObjSymbolRef  root_ref = lnk_ref_from_symbol(root);

        LNK_RelocRefs r = {0};
        r.obj                 = root_ref.obj;
        r.relocs.count        = 1;
        r.relocs.v            = push_array(scratch.arena, COFF_Reloc, 1);
        r.relocs.v[0].isymbol = root_ref.symbol_idx;

        lnk_reloc_ref_batch_list_push(scratch.arena, global_batch_list, r);
      }

      // push task for every non-COMDAT section
      for EachNode(obj_n, LNK_ObjNode, objs.first) {
        LNK_Obj *obj = &obj_n->data;
        for EachIndex(sect_idx, obj->header.section_count_no_null) {
          U32                 section_number = sect_idx+1;
          COFF_SectionFlags   section_flags  = obj->section_flags[sect_idx];

          // is section eligible for walking?
          if (section_flags & COFF_SectionFlag_LnkRemove)  { continue; }
          if (section_flags & COFF_SectionFlag_LnkCOMDAT)  { continue; }
          if (section_flags & COFF_SectionFlag_LnkInfo)    { continue; }
          if (section_flags & LNK_SECTION_FLAG_DEBUG)      { continue; }

          // divide relocs and push task for each reloc block
          COFF_RelocArray  relocs           = lnk_coff_reloc_info_from_section_number(obj, section_number);
          U64              relocs_per_batch = 1000;
          U64              new_task_count   = CeilIntegerDiv(relocs.count, relocs_per_batch);
          for EachIndex(new_task_idx, new_task_count) {
            LNK_RelocRefs r = {0};
            r.obj          = obj;
            r.relocs.count = Min(relocs_per_batch, relocs.count - (new_task_idx * relocs_per_batch));
            r.relocs.v     = relocs.v + (new_task_idx * relocs_per_batch);
            lnk_reloc_ref_batch_list_push(scratch.arena, global_batch_list, r);
          }
        }
      }
    }
  }
  tp_broadcast(&is_live);
  tp_broadcast(&objs_by_idx);
  tp_broadcast(&global_batch_list);
  tp_broadcast(&active_thread_count);

  LNK_RelocRefsBatchList free_list = {0};
  for (;;) {
    // update active thread count
    ins_atomic_u32_inc_eval(active_thread_count);

    for (;;) {
      // pop batch
      LNK_RelocRefsBatch *batch = lnk_reloc_ref_batch_list_pop_atomic(global_batch_list);
      if (!batch) { break; }

      // walk batch relocations
      LNK_RelocRefsBatch *first_batch = 0, *last_batch = 0;
      for EachIndex(i, batch->count) {
        for EachIndex(reloc_idx, batch->v[i].relocs.count) {
          COFF_Reloc *reloc = &batch->v[i].relocs.v[reloc_idx];

          // reloc -> symbol
          LNK_ObjSymbolRef ref_symbol = (LNK_ObjSymbolRef){ .obj = batch->v[i].obj, .symbol_idx = reloc->isymbol };
          {
            Temp    temp         = temp_begin(scratch2.arena);
            HashMap seen_hm      = {0};
            B32     keep_walking = 1;
            do {
              // detect cyclic chains
              U64 symbol_key = ((U64)ref_symbol.obj->input_idx << 32ull) | (U64)ref_symbol.symbol_idx;
              if (hash_map_search_u64_u64(&seen_hm, symbol_key) == 0) {
                hash_map_push_u64_u64(temp.arena, &seen_hm, symbol_key, 1);
              } else {
                COFF_ParsedSymbol reloc_parsed = lnk_parsed_symbol_from_coff_symbol_idx(batch->v[i].obj, reloc->isymbol);
                lnk_error_obj(LNK_Warning_CyclicSymbol, batch->v[i].obj, "symbol %S forms a cyclic chain (/OPT:REF)", reloc_parsed.name);
                MemoryZeroStruct(&ref_symbol);
                break;
              }

              // unpack symbol
              COFF_ParsedSymbol          ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx(ref_symbol.obj, ref_symbol.symbol_idx);
              COFF_SymbolValueInterpType ref_interp = coff_interp_from_parsed_symbol(ref_parsed);

              // resolve symbol
              LNK_ObjSymbolRef next_ref = {0};
              if (lnk_resolve_symbol(symtab, ref_symbol, &next_ref)) {
                keep_walking = (ref_interp == COFF_SymbolValueInterp_Weak || ref_interp == COFF_SymbolValueInterp_Undefined);
                ref_symbol   = next_ref;
              } else {
                keep_walking = 0;
              }
            } while (keep_walking);
            temp_end(temp);
          }

          // skip unresolved symbol
          if (ref_symbol.obj == 0) { continue; }

          // unpack resolved symbol
          COFF_ParsedSymbol           ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx(ref_symbol.obj, ref_symbol.symbol_idx);
          COFF_SymbolValueInterpType  ref_interp = coff_interp_from_parsed_symbol(ref_parsed);

          if (ref_interp == COFF_SymbolValueInterp_Regular) {
            Temp temp = temp_begin(scratch2.arena);

            // /OPT:ICF static-COMDAT redirect: if the referenced section was folded into a leader,
            // mark the LEADER section live (and walk the LEADER's relocs/associated sections) instead
            // of the dead follower. The follower then dead-strips, taking its .pdata/.xdata with it.
            LNK_Obj *walk_obj      = ref_symbol.obj;
            U32      seed_sn       = ref_parsed.section_number;
            if (walk_obj->icf_fold && seed_sn != 0 && seed_sn <= walk_obj->header.section_count_no_null &&
                walk_obj->icf_fold[seed_sn - 1].set) {
              LNK_ICFFold fold = walk_obj->icf_fold[seed_sn - 1];
              walk_obj = objs_by_idx[fold.leader_obj_idx];
              seed_sn  = fold.leader_sn;
            }

            HashMap visited_sections_hm = {0};
            U32Node *stack = push_array(temp.arena, U32Node, 1);
            stack->data    = seed_sn;
            do {
              U32 section_number = stack->data;
              SLLStackPop(stack);

              // is section number valid?
              if (section_number == 0 || section_number > walk_obj->header.section_count_no_null) { continue; }

              // detect cyclic associative sections
              if (hash_map_search_u64_u64(&visited_sections_hm, section_number)) { continue; }
              hash_map_push_u64_u64(temp.arena, &visited_sections_hm, section_number, 1);

              // push associated section
              for EachNode(associated_n, U32Node, walk_obj->associated_sections[section_number]) {
                U32 assoc_sn = associated_n->data;

                // /OPT:ICF static-COMDAT: an associative section can itself be a folded static
                // follower (e.g. an associative .text$ funclet/thunk). Keeping the follower live here
                // would defeat the fold and leave it with relocs into removed targets. Instead skip
                // the follower and mark its LEADER live (cross-obj), so the follower dead-strips while
                // its address still resolves to the identical leader.
                if (walk_obj->icf_fold && assoc_sn >= 1 && assoc_sn <= walk_obj->header.section_count_no_null &&
                    walk_obj->icf_fold[assoc_sn - 1].set) {
                  LNK_ICFFold afold      = walk_obj->icf_fold[assoc_sn - 1];
                  LNK_Obj    *leader_obj = objs_by_idx[afold.leader_obj_idx];
                  U32         leader_sn  = afold.leader_sn;
                  // mark the leader .text and each of its associative sections (.pdata/.xdata) live,
                  // enqueuing each eligible one's relocs so their targets (handler .text, .xdata) are
                  // kept live too -- exactly as a normal seed-walk of the leader would do.
                  U32 leader_sns[64];
                  U32 leader_sn_count = 0;
                  leader_sns[leader_sn_count++] = leader_sn;
                  for EachNode(lan, U32Node, leader_obj->associated_sections[leader_sn]) {
                    if (leader_sn_count < ArrayCount(leader_sns)) { leader_sns[leader_sn_count++] = lan->data; }
                  }
                  for EachIndex(lsi, leader_sn_count) {
                    U32 lsn = leader_sns[lsi];
                    if (lsn == 0 || lsn > leader_obj->header.section_count_no_null) { continue; }
                    U8 lseen = ins_atomic_u8_eval_assign(&is_live[leader_obj->input_idx][lsn], 1);
                    if (lseen) { continue; }
                    COFF_SectionFlags lflags = leader_obj->section_flags[lsn - 1];
                    if (lflags & (COFF_SectionFlag_LnkRemove | COFF_SectionFlag_LnkInfo | LNK_SECTION_FLAG_DEBUG)) { continue; }
                    LNK_RelocRefs lrefs = {0};
                    lrefs.obj    = leader_obj;
                    lrefs.relocs = lnk_coff_reloc_info_from_section_number(leader_obj, lsn);
                    LNK_RelocRefsBatch *lbatch = last_batch;
                    if (last_batch == 0 || last_batch->count >= ArrayCount(last_batch->v)) {
                      if (free_list.head.node) { lbatch = lnk_reloc_ref_batch_list_pop(&free_list); MemoryZeroStruct(lbatch); }
                      else                     { lbatch = push_array(scratch.arena, LNK_RelocRefsBatch, 1); }
                      SLLQueuePush(first_batch, last_batch, lbatch);
                    }
                    lbatch->v[lbatch->count++] = lrefs;
                  }
                  continue; // do not mark the follower live
                }

                if (hash_map_search_u64_u64(&visited_sections_hm, assoc_sn)) { continue; }
                U32Node *stack_n = push_array(temp.arena, U32Node, 1);
                stack_n->data = assoc_sn;
                SLLStackPush(stack, stack_n);
              }

              COFF_SectionFlags   section_flags  = walk_obj->section_flags[section_number-1];

              // on first section visit, set live flag and enqueue section
              U8 was_visited = ins_atomic_u8_eval_assign(&is_live[walk_obj->input_idx][section_number], 1);
              if (was_visited) { continue; }

              // is section eligible for walking?
              if (section_flags & COFF_SectionFlag_LnkRemove) { continue; }
              if (section_flags & COFF_SectionFlag_LnkInfo)   { continue; }
              if (section_flags & LNK_SECTION_FLAG_DEBUG)     { continue; }

              LNK_RelocRefs refs = {0};
              refs.obj         = walk_obj;
              refs.relocs = lnk_coff_reloc_info_from_section_number(walk_obj, section_number);

              // get a batch node
              LNK_RelocRefsBatch *batch = last_batch;
              if (last_batch == 0 || last_batch->count >= ArrayCount(last_batch->v)) {
                if (free_list.head.node) {
                  batch = lnk_reloc_ref_batch_list_pop(&free_list);
                  MemoryZeroStruct(batch);
                } else {
                  batch = push_array(scratch.arena, LNK_RelocRefsBatch, 1);
                }
                SLLQueuePush(first_batch, last_batch, batch);
              }

              batch->v[batch->count++] = refs;

            } while (stack);

            temp_end(temp);
          }
        }
      }

      // queue new walks
      if (first_batch && last_batch) {
        lnk_reloc_ref_batch_list_concat_in_place_atomic(global_batch_list, first_batch, last_batch);
      }

      // put batch on the free list
      lnk_reloc_ref_batch_list_push_node(&free_list, batch);
    }

    // are all threads done walking?
    {
      U32 c = ins_atomic_u32_dec_eval(active_thread_count);
      if (c == 0 && ins_atomic_ptr_eval(&global_batch_list->head.node) == 0) {
        break;
      }
    }

    // comprehensive solution to the waiting problem
    for (; ins_atomic_ptr_eval(&global_batch_list->head.node) == 0; ) {
      // was signaled to exit?
      if (ins_atomic_u32_eval(active_thread_count) == 0) { goto exit; }
    }
  }
  exit:;
  barrier_wait(tp->barrier);

  // TODO: thread
  if (task_id == 0) {
    ProfBegin("Remove Unreachable Sections");

    typedef struct { U64 vsize; U64 fsize; U64 section_count; } Stat;
    enum { Stat_Null, Stat_Code, Stat_Data, Stat_Debug, Stat_Count };
    Stat stats[Stat_Count] = {0};

    for EachNode(obj_n, LNK_ObjNode, objs.first) {
      LNK_Obj *obj = &obj_n->data;

      for EachIndex(sect_idx, obj->header.section_count_no_null) {
        U32 section_number = sect_idx+1;
        if (is_live[obj->input_idx][section_number]) { continue; }

        COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);
        obj->section_flags[sect_idx] |= COFF_SectionFlag_LnkRemove;
        COFF_SectionFlags section_flags = obj->section_flags[sect_idx];

        U64 stat_kind = Stat_Null;
        if      (section_flags & LNK_SECTION_FLAG_DEBUG)   { stat_kind = Stat_Debug; }
        else if (section_flags & COFF_SectionFlag_CntCode) { stat_kind = Stat_Code;  }
        else                                                       { stat_kind = Stat_Data;  }
        
        if (section_flags & COFF_SectionFlag_CntUninitializedData) {
          stats[stat_kind].vsize += section_header->vsize;
        } else {
          stats[stat_kind].fsize += section_header->fsize;
        }
        stats[stat_kind].section_count += 1;
      }
    }

    if (lnk_get_log_status(LNK_Log_Debug)) {
      U64 total_fsize = 0, total_section_count = 0;
      for EachElement(i, stats) {
        total_fsize         += stats[i].fsize;
        total_section_count += stats[i].section_count;
      }
      String8List stat_list = {0};
      str8_list_pushf(scratch.arena, &stat_list, "Code : %M, %S sections", stats[Stat_Code].fsize,  str8_from_count(scratch.arena, stats[Stat_Code].section_count ));
      str8_list_pushf(scratch.arena, &stat_list, "Data : %M, %S sections", stats[Stat_Data].fsize,  str8_from_count(scratch.arena, stats[Stat_Data].section_count ));
      str8_list_pushf(scratch.arena, &stat_list, "Debug: %M, %S sections", stats[Stat_Debug].fsize, str8_from_count(scratch.arena, stats[Stat_Debug].section_count));
      str8_list_pushf(scratch.arena, &stat_list, "Total: %M, %S sections", total_fsize,             str8_from_count(scratch.arena, total_section_count));
      String8 stat_str = str8_list_join(scratch.arena, &stat_list, &(StringJoin){.pre = str8_lit("  "), .sep = str8_lit("\n  ")});
      lnk_log(LNK_Log_Debug, "/OPT:REF Stats:\n%S", stat_str);
    }

    ProfEnd();
  }
  barrier_wait(tp->barrier);

  scratch_end(scratch2);
  scratch_end(scratch);
  ProfEnd();
}

internal U64
lnk_icf_mix(U64 h, U64 x)
{
  h ^= x;
  h *= 0x100000001b3ull;
  h ^= h >> 29;
  return h;
}

// Flat open-addressing U64 -> U64 map used by /OPT:ICF. The tree-based HashMap is far too slow
// for the millions of per-round dense-id lookups; this linear-probing table keeps the hot loops
// cache-friendly. Empty slots hold key == LNK_ICF_EMPTY (a value the stored hashes never take).
#define LNK_ICF_EMPTY 0xffffffffffffffffull
typedef struct LNK_ICFMap
{
  U64 *keys;
  U64 *vals;
  U64  mask;
} LNK_ICFMap;

// avalanche so structured keys (e.g. (input_idx<<32)|section_number, whose low bits repeat
// across objects) scatter across slots instead of clustering into long probe chains
internal U64
lnk_icf_scramble(U64 x)
{
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return x;
}

internal LNK_ICFMap
lnk_icf_map_make(Arena *arena, U64 capacity)
{
  U64 cap = 1;
  while (cap < capacity*2) { cap <<= 1; }
  LNK_ICFMap m = {0};
  m.keys = push_array_no_zero(arena, U64, cap);
  m.vals = push_array_no_zero(arena, U64, cap);
  m.mask = cap - 1;
  // LNK_ICF_EMPTY is all-0xFF bytes -> a single memset (bandwidth-optimal, vectorized) instead of a
  // scalar per-U64 store loop. cap can be 32M+ entries (256MB) on the monolithic link; the scalar
  // loop was a serial first-touch page-fault sink.
  MemorySet(m.keys, 0xff, cap*sizeof(U64));
  return m;
}

// look up key; returns stored value or fallback if absent
internal U64
lnk_icf_map_get(LNK_ICFMap *m, U64 key, U64 fallback)
{
  U64 slot = lnk_icf_scramble(key) & m->mask;
  for (;;) {
    U64 k = m->keys[slot];
    if (k == key)          { return m->vals[slot]; }
    if (k == LNK_ICF_EMPTY) { return fallback; }
    slot = (slot + 1) & m->mask;
  }
}

internal void
lnk_icf_map_put(LNK_ICFMap *m, U64 key, U64 val)
{
  U64 slot = lnk_icf_scramble(key) & m->mask;
  for (;;) {
    U64 k = m->keys[slot];
    if (k == LNK_ICF_EMPTY || k == key) { m->keys[slot] = key; m->vals[slot] = val; return; }
    slot = (slot + 1) & m->mask;
  }
}

// Thread-safe insert for the cand_map build (lnk_opt_icf): the keys are UNIQUE (one entry per
// (input_idx, sn) candidate -- a 1:1 map), so each insert claims a distinct empty slot. Claim it
// with an atomic CAS EMPTY->key; only the CAS winner writes vals[slot]. Because keys are unique no
// two writers ever target the same key, so the CAS is the sole arbiter of slot ownership and the
// vals[] store has exactly one writer. The map is only READ later (lnk_icf_map_get, after the build
// barrier), and a key-keyed probe finds its key regardless of WHICH slot collisions placed it in --
// so the resulting key->val mapping (hence ICF output) is identical for any insertion order.
internal void
lnk_icf_map_put_atomic(LNK_ICFMap *m, U64 key, U64 val)
{
  U64 slot = lnk_icf_scramble(key) & m->mask;
  for (;;) {
    U64 k = m->keys[slot];
    if (k == LNK_ICF_EMPTY) {
      // CAS returns the prior value; if it was EMPTY we won the slot.
      if (ins_atomic_u64_eval_cond_assign(&m->keys[slot], key, LNK_ICF_EMPTY) == LNK_ICF_EMPTY) {
        m->vals[slot] = val; // sole writer of this slot's value (keys are unique -> one CAS winner)
        return;
      }
      // lost the race for this slot: re-read the same slot (another key may now own it, or it is
      // still EMPTY-but-someone-else-is-mid-CAS -> the re-read resolves it). do NOT advance yet.
      continue;
    }
    slot = (slot + 1) & m->mask;
  }
}

typedef struct LNK_ICFCand
{
  LNK_Obj *obj;
  U32      sn;          // section number
  U32      reloc_first; // index into flattened reloc-target arrays
  U32      reloc_count;
  U64      key0;        // round-0 content key
  B8       is_static;   // static (internal-linkage) COMDAT: folded via icf_fold, not symbol redirect
  // NOTE: per-candidate equivalence-class "color" lives in a SEPARATE dense U64 colors[] array
  // (see lnk_opt_icf), not here -- the refine/scan loops gather colors by candidate index millions
  // of times; an 8B dense array keeps them cache-dense vs pulling this whole 40B struct per access.
} LNK_ICFCand;

typedef struct LNK_ICFCandMapPutTask
{
  Rng1U64     *ranges;
  LNK_ICFCand *cands;
  LNK_ICFMap  *cand_map;
} LNK_ICFCandMapPutTask;

// Parallel build of the (input_idx, sn) -> cand_idx+1 lookup. The serial build was cache-miss-bound
// (~642ms): each lnk_icf_map_put probes a scrambled (random) slot. Spreading the inserts across the
// pool with the atomic-CAS claim (lnk_icf_map_put_atomic) parallelizes those misses. Output-neutral
// because keys are unique and the map is read only by key afterward (see lnk_icf_map_put_atomic).
internal
THREAD_POOL_TASK_FUNC(lnk_icf_candmap_put_task)
{
  LNK_ICFCandMapPutTask *task = raw_task;
  for EachInRange(ci, task->ranges[task_id]) {
    lnk_icf_map_put_atomic(task->cand_map, Compose64Bit(task->cands[ci].obj->input_idx, task->cands[ci].sn), ci + 1);
  }
}

typedef struct LNK_ICFHashTask
{
  Rng1U64         *ranges;
  LNK_ICFCand     *cands;
  LNK_ICFMap      *cand_map;
  LNK_SymbolTable *symtab;
  U8              *rt_iscand;
  U64             *rt_target;
} LNK_ICFHashTask;

// per-candidate content hash + relocation-target resolution (parallel; each candidate writes
// its own disjoint reloc slice and key0, all reads are of immutable structures)
internal
THREAD_POOL_TASK_FUNC(lnk_icf_hash_task)
{
  LNK_ICFHashTask *task = raw_task;
  for EachInRange(ci, task->ranges[task_id]) {
    LNK_ICFCand        *c      = &task->cands[ci];
    COFF_SectionHeader *header = lnk_coff_section_header_from_section_number(c->obj, c->sn);
    COFF_RelocArray     relocs = lnk_coff_relocs_from_section_header(c->obj, header);
    String8             data   = str8_substr(c->obj->data, rng_1u64(header->foff, header->foff + header->fsize));

    blake3_hasher h; blake3_hasher_init(&h);
    U32 flags_for_hash = c->obj->section_flags[c->sn - 1] & ~(COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_LnkRemove);
    blake3_hasher_update(&h, &flags_for_hash, sizeof(flags_for_hash));
    blake3_hasher_update(&h, &header->fsize, sizeof(header->fsize));
    blake3_hasher_update(&h, data.str, data.size);

    for EachIndex(ri, relocs.count) {
      COFF_Reloc *reloc = &relocs.v[ri];
      blake3_hasher_update(&h, &reloc->type, sizeof(reloc->type));
      blake3_hasher_update(&h, &reloc->apply_off, sizeof(reloc->apply_off));

      COFF_ParsedSymbol          tp   = lnk_parsed_symbol_from_coff_symbol_idx(c->obj, reloc->isymbol);
      COFF_SymbolValueInterpType ti   = coff_interp_from_parsed_symbol(tp);
      LNK_ObjSymbolRef           tref = { c->obj, reloc->isymbol };
      if (ti == COFF_SymbolValueInterp_Undefined || ti == COFF_SymbolValueInterp_Weak) {
        LNK_ObjSymbolRef resolved = {0};
        if (lnk_resolve_symbol(task->symtab, tref, &resolved)) {
          tref = resolved;
          tp   = lnk_parsed_symbol_from_coff_symbol_idx(tref.obj, tref.symbol_idx);
          ti   = coff_interp_from_parsed_symbol(tp);
        }
      }

      U8  iscand = 0;
      U64 target = 0;
      if (ti == COFF_SymbolValueInterp_Regular && tref.obj != 0) {
        // Canonicalize a COMDAT definition to its selected leader so the SAME logical symbol keys
        // identically regardless of which obj's local copy this reloc happens to name. Two byte-
        // identical functions that each reference their own copy of a shared COMDAT (writable
        // static guards, vtables, selectany globals) must otherwise get distinct per-obj keys and
        // never fold. The symlink leader is the post-resolution canonical definition (built before
        // ICF, read-only here), so keying by it is strictly more correct than per-obj.
        // Keep the reloc's own target offset (tp.value): canonicalize only the SECTION IDENTITY
        // (obj,sn) to the leader, never the offset, so distinct offsets into the same section stay
        // distinct. The leader symbol itself always has value 0 (the symlink picks the value==0
        // definition), but a reloc may name an interior offset.
        LNK_Obj *kobj = tref.obj;
        U64      ksn  = tp.section_number;
        if (ksn >= 1 && ksn <= kobj->header.section_count_no_null &&
            (kobj->section_flags[ksn - 1] & COFF_SectionFlag_LnkCOMDAT)) {
          LNK_Symbol *leader = lnk_obj_get_comdat_symlink(kobj, ksn);
          if (leader) {
            LNK_ObjSymbolRef  lref = lnk_ref_from_symbol(leader);
            // only section_number is needed here; skip the string-table name decode that the
            // full lnk_parsed_from_symbol does per COMDAT reloc (millions of relocs in this phase).
            if (lref.obj != 0) {
              COFF_ParsedSymbol lp = lnk_parsed_symbol_from_coff_symbol_idx_no_name(lref.obj, lref.symbol_idx);
              kobj = lref.obj; ksn = lp.section_number;
            }
          }
        }
        U64 cv = lnk_icf_map_get(task->cand_map, Compose64Bit(kobj->input_idx, ksn), 0);
        if (cv) { iscand = 1; target = cv - 1; }
        else    { target = lnk_icf_mix(Compose64Bit(kobj->input_idx, ksn), tp.value); }
      } else {
        U64 nh = 14695981039346656037ull;
        for (U64 i = 0; i < tp.name.size; i += 1) { nh = lnk_icf_mix(nh, tp.name.str[i]); }
        target = lnk_icf_mix(nh, tp.value);
      }

      U64 idx = (U64)c->reloc_first + ri;
      task->rt_iscand[idx] = iscand;
      task->rt_target[idx] = target;
      if (!iscand) { blake3_hasher_update(&h, &target, sizeof(target)); }
    }

    U8 out[16]; blake3_hasher_finalize(&h, out, sizeof(out));
    U64 lo = *(U64 *)&out[0], hi = *(U64 *)&out[8];
    c->key0 = lnk_icf_mix(lo, hi);
  }
}

// Split active[0,n) into worker_count contiguous ranges of ~equal RELOC WEIGHT (refine/verify cost
// per candidate ~= its reloc_count), instead of equal candidate count. Fixes the load imbalance where
// a worker holding high-reloc candidates runs long while others idle. Ranges are a pure function of
// reloc_count[active[]] -> deterministic. Returns worker_count+1 boundary array (dummy tail like
// tp_divide_work). active==0 means weight by cands[i] directly over [0,n).
internal Rng1U64 *
lnk_icf_divide_by_reloc(Arena *arena, U32 *active, U64 n, U32 worker_count, LNK_ICFCand *cands)
{
  Rng1U64 *ranges = push_array_no_zero(arena, Rng1U64, worker_count + 1);
  U64 total_w = 0;
  for EachIndex(i, n) { total_w += cands[active ? active[i] : i].reloc_count + 1; } // +1 so zero-reloc cands still count
  U64 per_w = (total_w + worker_count - 1) / (worker_count ? worker_count : 1);
  U64 cursor = 0, acc = 0, w = 0;
  for (; w < worker_count; w += 1) {
    U64 begin = cursor;
    U64 target = (w + 1) * per_w;
    while (cursor < n && acc < target) { acc += cands[active ? active[cursor] : cursor].reloc_count + 1; cursor += 1; }
    ranges[w] = rng_1u64(begin, cursor);
  }
  // any remainder from rounding -> last worker
  if (cursor < n) { ranges[worker_count - 1].max = n; }
  ranges[worker_count] = rng_1u64(n, n);
  return ranges;
}

typedef struct LNK_ICFRefineTask
{
  Rng1U64     *ranges;
  LNK_ICFCand *cands;
  U32         *colors; // dense per-candidate color array, U32 (color ids < color_base < 2^32);
                       // separate from the 40B LNK_ICFCand so the hot color gathers touch 4B/elem.
  U8          *rt_iscand;
  U64         *rt_target;
  U64         *newkey;
  U32         *active; // when set, ranges index into active[] rather than cands[] directly
} LNK_ICFRefineTask;

// recompute each candidate's refinement key from its current color and its targets' colors
// (parallel; reads the immutable color snapshot, writes its own newkey slot)
internal
THREAD_POOL_TASK_FUNC(lnk_icf_refine_task)
{
  LNK_ICFRefineTask *task   = raw_task;
  U32               *colors = task->colors;
  for EachInRange(ai, task->ranges[task_id]) {
    U64          ci = task->active ? task->active[ai] : ai;
    LNK_ICFCand *c  = &task->cands[ci];
    U64 k = lnk_icf_mix(0x9e3779b97f4a7c15ull, colors[ci]);
    for EachIndex(j, c->reloc_count) {
      U64 idx = (U64)c->reloc_first + j;
      U64 t   = task->rt_iscand[idx] ? colors[task->rt_target[idx]] : task->rt_target[idx];
      k = lnk_icf_mix(k, t);
    }
    task->newkey[ci] = k;
  }
}

// Parallel ICF fold: each group of same-colored candidates is independent (its members are distinct
// (obj,sn) sections -> distinct symlink nodes and distinct icf_fold slots), so leader election +
// byte/reloc verification + the fold writes can run concurrently across groups with no shared-state
// races. The dominant cost folded in here is the per-follower byte-compare (str8_match/memcmp), which
// was the largest single serial main-thread sink. Determinism is preserved: each group elects the
// same leader (smallest (is_static,input_idx,sn)) and writes the same disjoint slots regardless of
// which worker runs it. fold_count is accumulated per-worker.
// parallel helpers for lnk_icf_dense_colors_active: the per-round gather (sk[i]=newkey[active[i]])
// and the final scatter (cands[active[sv[k]]].color = ...) are random-access over the candidate
// arrays (cache-miss bound). Spreading them across workers hides the memory latency. The boundary
// counting between them stays a cheap sequential pass over the sorted keys.
typedef struct LNK_ICFDenseTask
{
  Rng1U64     *ranges;
  U32         *active;
  U64         *newkey;
  U64         *sk;
  U32         *sv;
  U32         *color_at; // per-sorted-position final color (filled serially, scattered in parallel)
  U32         *colors;   // dense per-candidate color array (scatter target)
  U8          *keep;     // per-sorted-position: 1 if its key-run has size>=2 (survives to next round)
  U32         *out_slot; // per-sorted-position: exclusive prefix of keep[] -> deterministic emit slot
  U32         *next_active; // emit target (survivors, in sorted-color order)
  // parallel prefix-sum group scan (replaces the per-round serial scan over sk[] -- the refine-window
  // sawtooth bottleneck, lnk_link_image self). boundary (run start) and keep (run size>=2 survivor)
  // are per-position bits reading only sk[k], sk[k-1], sk[k+1] -> chunk-local. mark: write keep[] +
  // per-chunk local boundary/keep/surviving-class counts. serial: tiny exclusive prefix over the
  // worker_count chunk totals (NOT over n). apply: each chunk re-derives color_at[]/out_slot[] from
  // its prefixed base -> identical values to the old serial running counter regardless of chunk split.
  U64         *chunk_nc;    // [worker_count] in: local boundary count; serial-prefix -> chunk's exclusive base
  U64         *chunk_keep;  // [worker_count] in: local keep count;     serial-prefix -> chunk's exclusive base
  U64         *chunk_scls;  // [worker_count] local surviving-class count (boundary && keep), summed serially
  U64          base;        // color id base for this round
  U64          n_total;     // n (for the k+1==N end-of-run test; spans chunk edges read-only)
} LNK_ICFDenseTask;

internal
THREAD_POOL_TASK_FUNC(lnk_icf_dense_gather_task)
{
  LNK_ICFDenseTask *t = raw_task;
  for EachInRange(i, t->ranges[task_id]) { t->sk[i] = t->newkey[t->active[i]]; t->sv[i] = (U32)i; }
}

// mark pass: per sorted position compute boundary (run start) and keep (run size>=2 -> survives);
// write keep[k]; accumulate this chunk's local boundary/keep/surviving-class counts. boundary/keep
// read only sk[k-1..k+1] -> chunk-local, so each chunk's local sums are independent (the run that
// straddles a chunk edge is counted once -- by whichever chunk owns its run-start position).
internal
THREAD_POOL_TASK_FUNC(lnk_icf_scan_mark_task)
{
  LNK_ICFDenseTask *t = raw_task;
  Rng1U64 r = t->ranges[task_id];
  U64 N = t->n_total;
  U64 nloc = 0, kloc = 0, sloc = 0;
  for (U64 k = r.min; k < r.max; k += 1) {
    B32 boundary      = (k == 0     || t->sk[k] != t->sk[k - 1]);
    B32 next_boundary = (k + 1 == N || t->sk[k + 1] != t->sk[k]); // start of next run (or end)
    U8  survive       = (U8)!(boundary && next_boundary);         // singleton iff boundary && next_boundary
    t->keep[k] = survive;
    if (boundary)            { nloc += 1; }
    if (survive)             { kloc += 1; }
    if (boundary && survive) { sloc += 1; } // run-start of a surviving run -> one surviving class
  }
  t->chunk_nc[task_id]   = nloc;
  t->chunk_keep[task_id] = kloc;
  t->chunk_scls[task_id] = sloc;
}

// apply pass: chunk_nc[task_id]/chunk_keep[task_id] now hold this chunk's exclusive base (the running
// class id / emit slot at the chunk's first position). Re-derive the per-position bits and run the
// counters locally from that base to write color_at[k]/out_slot[k] -- byte-identical to the old serial
// scan regardless of how the chunks split, so the downstream scatter/emit stay deterministic.
internal
THREAD_POOL_TASK_FUNC(lnk_icf_scan_apply_task)
{
  LNK_ICFDenseTask *t = raw_task;
  Rng1U64 r  = t->ranges[task_id];
  U64 nc     = t->chunk_nc[task_id];   // boundaries before this chunk (exclusive base)
  U64 slot   = t->chunk_keep[task_id]; // emit slot at chunk start
  for (U64 k = r.min; k < r.max; k += 1) {
    B32 boundary = (k == 0 || t->sk[k] != t->sk[k - 1]);
    if (boundary) { nc += 1; }
    t->color_at[k] = (U32)(t->base + (nc - 1));
    t->out_slot[k] = (U32)slot;
    slot += t->keep[k];
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_icf_dense_scatter_task)
{
  LNK_ICFDenseTask *t = raw_task;
  for EachInRange(k, t->ranges[task_id]) { t->colors[t->active[t->sv[k]]] = t->color_at[k]; }
}

// parallel survivor emit: each kept sorted-position writes active[sv[k]] to its precomputed slot
// out_slot[k]. Slots are a serial exclusive-prefix of keep[] (deterministic), so the emit order is
// identical regardless of which worker runs which k -> reproducible. This moves the random-access
// active[sv[k]] gather (the expensive serial part of the old group scan) onto the workers.
internal
THREAD_POOL_TASK_FUNC(lnk_icf_dense_emit_task)
{
  LNK_ICFDenseTask *t = raw_task;
  for EachInRange(k, t->ranges[task_id]) {
    if (t->keep[k]) { t->next_active[t->out_slot[k]] = t->active[t->sv[k]]; }
  }
}

// densify the keys of just the active subset into class ids drawn from `base` upward (so they never
// collide with the finalized colors of candidates that already dropped out as singletons). Returns
// the number of distinct classes among the active set. Equal keys land in one class regardless of
// sort tie order, so color values are deterministic.
// Densify active subset into class ids from `base` up. ALSO emits, in the same sorted pass (run
// lengths are free), the compacted next-round active set (members of classes that still have size>=2)
// into next_active[], and the count of such classes. This fuses the per-round singleton-drop scan
// (was two random-access colors[active[i]] gathers + a cls_size histogram) into the group scan.
// Determinism: equal keys land in one class regardless of sort tie order; next_active is emitted in
// sorted-color order (deterministic). Returns distinct class count nc.
internal U64
lnk_icf_dense_colors_active(TP_Context *tp, Arena *arena, U32 *active, U64 n, U64 *newkey, U32 *colors, U64 base,
                            U32 *next_active, U64 *out_next_count, U64 *out_next_class_count)
{
  Temp t  = temp_begin(arena);
  U64 *sk       = push_array_no_zero(t.arena, U64, n ? n : 1);
  U32 *sv       = push_array_no_zero(t.arena, U32, n ? n : 1); // index into active[]
  U32 *color_at = push_array_no_zero(t.arena, U32, n ? n : 1); // per-sorted-position color

  LNK_ICFDenseTask dt = {0};
  dt.active = active; dt.newkey = newkey; dt.sk = sk; dt.sv = sv; dt.color_at = color_at; dt.colors = colors;

  // parallel gather: random-access newkey[active[i]] -> sk[i], spread across workers. NOTE radix
  // (next line) allocates scratch on t.arena, so ranges built before radix would be stale -- rebuild
  // ranges AFTER radix for the scatter. (This stale-ranges trap is why the parallel path was wrong.)
  dt.ranges = tp_divide_work(t.arena, n, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_dense_gather_task, &dt);
  lnk_radix_sort_u64_pairs(tp, t.arena, n, sk, sv);
  dt.ranges = tp_divide_work(t.arena, n, tp->worker_count); // rebuilt AFTER radix for scatter

  // PARALLEL prefix-sum group scan over the SORTED keys (replaces the per-round serial scan that was
  // the refine-window sawtooth bottleneck). mark: per-position keep bit + per-chunk local counts ->
  // serial: tiny exclusive prefix over the worker_count chunk totals (NOT over n) -> apply: each chunk
  // re-derives color_at[]/out_slot[] from its prefixed base. The result is a pure prefix of per-position
  // bits, byte-identical to the old serial running counter regardless of chunk split -> deterministic.
  U8  *keep      = push_array_no_zero(t.arena, U8,  n ? n : 1);
  U32 *out_slot  = push_array_no_zero(t.arena, U32, n ? n : 1);
  U64 W          = tp->worker_count;
  U64 *chunk_nc  = push_array_no_zero(t.arena, U64, W ? W : 1);
  U64 *chunk_kp  = push_array_no_zero(t.arena, U64, W ? W : 1);
  U64 *chunk_sc  = push_array_no_zero(t.arena, U64, W ? W : 1);

  dt.keep = keep; dt.out_slot = out_slot; dt.next_active = next_active;
  dt.chunk_nc = chunk_nc; dt.chunk_keep = chunk_kp; dt.chunk_scls = chunk_sc;
  dt.base = base; dt.n_total = n;
  tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_scan_mark_task, &dt);

  // exclusive prefix over the W chunk totals -> chunk_nc[w]/chunk_kp[w] become the running class id /
  // emit slot at chunk w's first position (the exclusive base apply counts from); sum surviving classes.
  U64 nc = 0, next_count = 0, next_class_count = 0;
  for EachIndex(w, W) {
    U64 cn = chunk_nc[w], ck = chunk_kp[w], cs = chunk_sc[w];
    chunk_nc[w] = nc; chunk_kp[w] = next_count;
    nc += cn; next_count += ck; next_class_count += cs;
  }
  tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_scan_apply_task, &dt);

#if defined(ICF_SCAN_SELFCHECK)
  { // SELF-CHECK (debug-only, gated): recompute the serial scan into shadow arrays, assert byte-exact
    Temp tc = temp_begin(t.arena);
    U8  *xkeep = push_array_no_zero(tc.arena, U8,  n ? n : 1);
    U32 *xslot = push_array_no_zero(tc.arena, U32, n ? n : 1);
    U32 *xcol  = push_array_no_zero(tc.arena, U32, n ? n : 1);
    U64 xnc=0, xnext=0, xncls=0, run0=0, mism=0;
    for EachIndex(k, n) {
      B32 b = (k == 0 || sk[k] != sk[k - 1]);
      if (b && k != 0) { U8 s=(k-run0>=2)?1:0; if(s)xncls++; for(U64 m=run0;m<k;m++){xkeep[m]=s;xslot[m]=(U32)xnext;xnext+=s;} run0=k; }
      if (b) xnc++;
      xcol[k]=(U32)(base+(xnc-1));
    }
    { U8 s=(n-run0>=2)?1:0; if(s)xncls++; for(U64 m=run0;m<n;m++){xkeep[m]=s;xslot[m]=(U32)xnext;xnext+=s;} }
    for EachIndex(k,n){ if(xkeep[k]!=keep[k]||xslot[k]!=out_slot[k]||xcol[k]!=color_at[k]) mism++; }
    if(mism||xnc!=nc||xnext!=next_count||xncls!=next_class_count){
      lnk_log(LNK_Log_Debug,"ICF_SCAN_SELFCHECK MISMATCH n=%llu mism=%llu nc(p=%llu s=%llu) next(p=%llu s=%llu) ncls(p=%llu s=%llu)",n,mism,nc,xnc,next_count,xnext,next_class_count,xncls);
    }
    temp_end(tc);
  }
#endif

  tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_dense_scatter_task, &dt); // parallel color scatter
  tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_dense_emit_task, &dt);    // parallel survivor emit

  temp_end(t);
  if (out_next_count)       { *out_next_count       = next_count; }
  if (out_next_class_count) { *out_next_class_count = next_class_count; }
  return nc;
}

// radix digit width (also defined at the radix-sort section below; hoisted here so the persistent
// refine region can size its histogram). Keep both in sync.
#if !defined(LNK_RADIX_BITS)
#define LNK_RADIX_BITS 8
#define LNK_RADIX_SIZE (1 << LNK_RADIX_BITS)
#endif

// ============================================================================================
// PERSISTENT-WORKER ICF REFINE REGION
//
// The per-round refine loop (refine -> dense_colors_active{gather,radix,scan,scatter,emit}) used to
// re-enter the thread pool ~10x per round via tp_for_parallel, ~18 rounds => ~126 fork-join cycles.
// Each phase is tiny, so the pool wake->work->sleep latency dominated lnk_link_image self time (the
// sawtooth). lnk_icf_refine_region collapses ALL rounds into ONE tp_for_parallel of worker_count
// participants; every old phase boundary becomes a barrier_wait(tp->barrier) inside the region. The
// tiny serial glue (radix per-pass 256xW prefix, group-scan W-chunk prefix, convergence/buffer swap)
// runs on worker 0 behind a barrier while the others wait -- same work, no pool re-entry.
//
// ORDER PRESERVATION: every phase keeps its EXACT chunking and math. ranges are rebuilt each round by
// worker 0 into preallocated buffers (lnk_icf_divide_by_reloc / tp_divide_work logic, in-place) and
// read by all workers after a barrier -- identical boundaries to the per-round path. The radix sort,
// scan_mark/apply, scatter and emit bodies are the SAME computations as the standalone tasks; only the
// dispatch changed (barrier instead of tp_for_parallel). Result is byte-identical to the per-round
// path (verified by ICF_SCAN_SELFCHECK + the canonical DLL cmp).
// ============================================================================================

typedef struct LNK_ICFRegion
{
  // immutable inputs
  TP_Context  *tp;
  LNK_ICFCand *cands;
  U8          *rt_iscand;
  U64         *rt_target;
  U64          cand_count;
  U32          worker_count;

  // ping-pong active sets + colors/keys (driver owns the buffers; pointers swap each round on worker 0)
  U32         *active;       // current active set (refine reads)
  U32         *active2;      // next active set (emit writes)
  U32         *colors;       // dense per-candidate color (scatter target)
  U64         *newkey;       // refine writes; gather reads

  // per-round scratch (preallocated to cand_count once; reused every round)
  U64         *sk;           // sorted keys
  U32         *sv;           // sorted values (index into active[])
  U32         *color_at;     // per-sorted-position color
  U8          *keep;         // per-sorted-position survivor bit
  U32         *out_slot;     // per-sorted-position emit slot
  U64         *kbuf;         // radix double-buffer (keys)
  U32         *vbuf;         // radix double-buffer (vals)
  U32         *hist;         // radix histogram [worker_count * LNK_RADIX_SIZE]
  U64         *chunk_nc;     // [worker_count] group-scan boundary counts
  U64         *chunk_kp;     // [worker_count] group-scan keep counts
  U64         *chunk_sc;     // [worker_count] group-scan surviving-class counts
  U64         *chunk_max;    // [worker_count] radix per-pass max-key reduction
  Rng1U64     *refine_ranges;// [worker_count+1] reloc-weighted (refine)
  Rng1U64     *work_ranges;  // [worker_count+1] even split over n (gather/scan/scatter/emit/radix)

  // round-loop control (written by worker 0, broadcast via barrier)
  U64          active_count;
  U64          active_class_count;
  U64          color_base;
  U64          round_n;          // active_count snapshot for the current round's phases
  U64          radix_pass_count; // passes for current round's radix sort
  U64         *radix_ksrc;       // current radix source/dest (swap on worker 0 each pass)
  U32         *radix_vsrc;
  U64         *radix_kdst;
  U32         *radix_vdst;
  B32          converged;        // set by worker 0 -> all break together
  U64          max_rounds;       // hard cap on rounds (region stops, may hand off to worklist tail)
} LNK_ICFRegion;

// in-place reloc-weighted division (mirrors lnk_icf_divide_by_reloc; writes into preallocated ranges)
internal void
lnk_icf_divide_by_reloc_into(Rng1U64 *ranges, U32 *active, U64 n, U32 worker_count, LNK_ICFCand *cands)
{
  // Count-based split: O(worker_count), no scan, no random gather. The old reloc-weighted
  // split did two O(n) cache-miss gathers (cands[active[i]].reloc_count) per round x ~18
  // rounds = ~5.6s serial. Work-split only -> output independent of it.
  U64 W = worker_count ? worker_count : 1;
  for (U64 w = 0; w < worker_count; w += 1) {
    ranges[w] = rng_1u64((w * n) / W, ((w + 1) * n) / W);
  }
  ranges[worker_count] = rng_1u64(n, n);
  (void)active; (void)cands;
}

// in-place even division (mirrors tp_divide_work; writes into preallocated ranges)
internal void
lnk_tp_divide_work_into(Rng1U64 *ranges, U64 item_count, U32 worker_count)
{
  U64 per_count = CeilIntegerDiv(item_count, worker_count);
  for (U64 i = 0; i < worker_count; i += 1) {
    ranges[i] = rng_1u64(Min(item_count, i * per_count), Min(item_count, i * per_count + per_count));
  }
  ranges[worker_count] = rng_1u64(item_count, item_count);
}

// One persistent parallel region spanning ALL refine rounds. worker_count participants; phase
// boundaries are barriers; serial glue runs on worker 0. Each worker owns ranges[worker_id].
internal
THREAD_POOL_TASK_FUNC(lnk_icf_refine_region_task)
{
  LNK_ICFRegion *rs = raw_task;
  // LANE = task_id, NOT worker_id. The cohort is `W = rs->worker_count` (the
  // fair-share cohort C for this barrier pass), and task_ids are the contiguous
  // [0,C) lane indices assigned by the dispatch. Every participant blocks on the
  // first barrier before it can steal a second task, so each runs exactly one
  // lane for the whole region. Using task_id (not worker_id) makes this correct
  // for ANY set of woken physical workers -- their ids need not be contiguous,
  // only the lanes do. All per-lane scratch (refine_ranges[wid], work_ranges[wid],
  // hist+wid*RADIX, chunk_*[wid]) is sized to W and indexed by this lane.
  U64 wid = task_id;
  U64 W   = rs->worker_count;

  for (U64 round = 0; round < rs->max_rounds; round += 1) {
    // -------- round setup (worker 0): build ranges, snapshot active_count --------
    if (wid == 0) {
      rs->round_n = rs->active_count;
      lnk_icf_divide_by_reloc_into(rs->refine_ranges, rs->active, rs->active_count, (U32)W, rs->cands);
    }
    barrier_wait(rs->tp->barrier);
    U64 n = rs->round_n;

    // -------- PHASE: refine (reloc-weighted ranges) --------
    {
      Rng1U64 r = rs->refine_ranges[wid];
      for EachInRange(ai, r) {
        U64          ci = rs->active[ai];
        LNK_ICFCand *c  = &rs->cands[ci];
        U64 k = lnk_icf_mix(0x9e3779b97f4a7c15ull, rs->colors[ci]);
        for EachIndex(j, c->reloc_count) {
          U64 idx = (U64)c->reloc_first + j;
          U64 t   = rs->rt_iscand[idx] ? rs->colors[rs->rt_target[idx]] : rs->rt_target[idx];
          k = lnk_icf_mix(k, t);
        }
        rs->newkey[ci] = k;
      }
    }
    barrier_wait(rs->tp->barrier);

    // -------- build even-split ranges (worker 0) for gather --------
    if (wid == 0) { lnk_tp_divide_work_into(rs->work_ranges, n, (U32)W); }
    barrier_wait(rs->tp->barrier);

    // -------- PHASE: gather  sk[i]=newkey[active[i]], sv[i]=i --------
    {
      Rng1U64 r = rs->work_ranges[wid];
      for EachInRange(i, r) { rs->sk[i] = rs->newkey[rs->active[i]]; rs->sv[i] = (U32)i; }
    }
    barrier_wait(rs->tp->barrier);

    // -------- PHASE: parallel LSD radix sort of (sk, sv) over n --------
    // worker 0 establishes pass_count from a parallel max reduction; each pass = parallel hist ->
    // serial 256xW prefix (worker 0) -> parallel scatter -> pointer swap (worker 0). Identical to
    // lnk_radix_sort_u64_pairs, just barrier-driven (no temp/scratch alloc inside the region).
    {
      // parallel max-key reduction into chunk_max[wid]
      {
        Rng1U64 r = rs->work_ranges[wid];
        U64 m = 0;
        for EachInRange(i, r) { if (rs->sk[i] > m) m = rs->sk[i]; }
        rs->chunk_max[wid] = m;
      }
      barrier_wait(rs->tp->barrier);
      if (wid == 0) {
        U64 max_key = 0;
        for EachIndex(w, W) { if (rs->chunk_max[w] > max_key) max_key = rs->chunk_max[w]; }
        U64 pass_count;
        if (max_key != 0) {
          U64 sig_bits   = 64 - clz64(max_key);
          U64 sig_passes = (sig_bits + LNK_RADIX_BITS - 1) / LNK_RADIX_BITS;
          pass_count = sig_passes + (sig_passes & 1);
        } else {
          pass_count = 0;
        }
        if (n < 2) { pass_count = 0; }
        rs->radix_pass_count = pass_count;
        rs->radix_ksrc = rs->sk;   rs->radix_vsrc = rs->sv;
        rs->radix_kdst = rs->kbuf; rs->radix_vdst = rs->vbuf;
      }
      barrier_wait(rs->tp->barrier);

      U64 pass_count = rs->radix_pass_count;
      for (U64 pass = 0; pass < pass_count; pass += 1) {
        U64  shift = pass * LNK_RADIX_BITS;
        U64 *ksrc  = rs->radix_ksrc; U32 *vsrc = rs->radix_vsrc;
        U64 *kdst  = rs->radix_kdst; U32 *vdst = rs->radix_vdst;

        // hist (parallel): zero own row, count digits over own range
        {
          U32 *h = rs->hist + wid * LNK_RADIX_SIZE;
          MemoryZero(h, sizeof(U32) * LNK_RADIX_SIZE);
          Rng1U64 r = rs->work_ranges[wid];
          for EachInRange(i, r) { h[(ksrc[i] >> shift) & (LNK_RADIX_SIZE - 1)] += 1; }
        }
        barrier_wait(rs->tp->barrier);

        // exclusive prefix across (bucket, worker) -- serial, worker 0 (identical order to the task path)
        if (wid == 0) {
          U64 running = 0;
          for EachIndex(bucket, LNK_RADIX_SIZE) {
            for EachIndex(w, W) {
              U32 *slot = &rs->hist[w * LNK_RADIX_SIZE + bucket];
              U32  c    = *slot;
              *slot     = (U32)running;
              running  += c;
            }
          }
        }
        barrier_wait(rs->tp->barrier);

        // scatter (parallel): each worker writes its disjoint contiguous run
        {
          U32 *h = rs->hist + wid * LNK_RADIX_SIZE;
          Rng1U64 r = rs->work_ranges[wid];
          for EachInRange(i, r) {
            U64 d   = (ksrc[i] >> shift) & (LNK_RADIX_SIZE - 1);
            U32 pos = h[d]++;
            kdst[pos] = ksrc[i];
            vdst[pos] = vsrc[i];
          }
        }
        barrier_wait(rs->tp->barrier);

        // swap src/dst (worker 0); even pass_count leaves sorted data back in sk/sv
        if (wid == 0) {
          rs->radix_ksrc = kdst; rs->radix_vsrc = vdst;
          rs->radix_kdst = ksrc; rs->radix_vdst = vsrc;
        }
        barrier_wait(rs->tp->barrier);
      }
      // ranges unchanged across radix; work_ranges already an even split over n for scan/scatter/emit
    }

    // -------- PHASE: scan mark (per-position keep bit + per-chunk local counts) --------
    {
      Rng1U64 r = rs->work_ranges[wid];
      U64 N = n;
      U64 nloc = 0, kloc = 0, sloc = 0;
      for (U64 k = r.min; k < r.max; k += 1) {
        B32 boundary      = (k == 0     || rs->sk[k] != rs->sk[k - 1]);
        B32 next_boundary = (k + 1 == N || rs->sk[k + 1] != rs->sk[k]);
        U8  survive       = (U8)!(boundary && next_boundary);
        rs->keep[k] = survive;
        if (boundary)            { nloc += 1; }
        if (survive)             { kloc += 1; }
        if (boundary && survive) { sloc += 1; }
      }
      rs->chunk_nc[wid] = nloc; rs->chunk_kp[wid] = kloc; rs->chunk_sc[wid] = sloc;
    }
    barrier_wait(rs->tp->barrier);

    // -------- serial: exclusive prefix over W chunk totals (worker 0) --------
    if (wid == 0) {
      U64 nc = 0, next_count = 0, next_class_count = 0;
      for EachIndex(w, W) {
        U64 cn = rs->chunk_nc[w], ck = rs->chunk_kp[w], cs = rs->chunk_sc[w];
        rs->chunk_nc[w] = nc; rs->chunk_kp[w] = next_count;
        nc += cn; next_count += ck; next_class_count += cs;
      }
      // stash round totals in chunk_max[0..2] (free scratch; radix max-reduce reuses it fresh next
      // round, and the convergence block below reads it within this same round before the swap).
      rs->chunk_max[0] = nc; rs->chunk_max[1] = next_count; rs->chunk_max[2] = next_class_count;
    }
    barrier_wait(rs->tp->barrier);

    U64 base = rs->color_base;

    // -------- PHASE: scan apply (re-derive color_at[]/out_slot[] from chunk's exclusive base) --------
    {
      Rng1U64 r = rs->work_ranges[wid];
      U64 nc   = rs->chunk_nc[wid];
      U64 slot = rs->chunk_kp[wid];
      for (U64 k = r.min; k < r.max; k += 1) {
        B32 boundary = (k == 0 || rs->sk[k] != rs->sk[k - 1]);
        if (boundary) { nc += 1; }
        rs->color_at[k] = (U32)(base + (nc - 1));
        rs->out_slot[k] = (U32)slot;
        slot += rs->keep[k];
      }
    }
    barrier_wait(rs->tp->barrier);

    // -------- PHASE: color scatter --------
    {
      Rng1U64 r = rs->work_ranges[wid];
      for EachInRange(k, r) { rs->colors[rs->active[rs->sv[k]]] = rs->color_at[k]; }
    }
    barrier_wait(rs->tp->barrier);

    // -------- PHASE: survivor emit --------
    {
      Rng1U64 r = rs->work_ranges[wid];
      for EachInRange(k, r) {
        if (rs->keep[k]) { rs->active2[rs->out_slot[k]] = rs->active[rs->sv[k]]; }
      }
    }
    barrier_wait(rs->tp->barrier);

    // -------- serial: convergence + buffer swap (worker 0), broadcast via barrier --------
    if (wid == 0) {
      U64 nc               = rs->chunk_max[0];
      U64 next_count       = rs->chunk_max[1];
      U64 next_class_count = rs->chunk_max[2];
      rs->color_base += nc;
      if (nc == rs->active_class_count) {
        rs->converged = 1;
      } else {
        U32 *tmp = rs->active; rs->active = rs->active2; rs->active2 = tmp;
        rs->active_count       = next_count;
        rs->active_class_count = next_class_count;
        if (rs->active_count == 0) { rs->converged = 1; }
      }
    }
    barrier_wait(rs->tp->barrier);

    if (rs->converged) { break; }
  }
}

// Is an object section an ICF fold candidate? Only externally-defined COMDATs are folded: the
// follower is redirected at its shared symbol-table node, so every reference (including from other
// objects) resolves to the leader and /OPT:REF then drops the follower. Returns 1 = candidate.
internal U32
lnk_icf_section_kind(LNK_Obj *obj, U64 sect_idx, B32 include_static)
{
  COFF_SectionFlags flags = obj->section_flags[sect_idx];
  if (~flags & COFF_SectionFlag_LnkCOMDAT) { return 0; }
  if ( flags & COFF_SectionFlag_LnkRemove) { return 0; }
  // fold code and read-only initialized data (const tables, vtables, string literals); folding
  // identical read-only data lets functions that reference it fold too (cascade). Mutable data
  // is never folded.
  B32 is_code   = (flags & COFF_SectionFlag_CntCode) != 0;
  B32 is_rodata = (flags & COFF_SectionFlag_CntInitializedData) && !(flags & COFF_SectionFlag_MemWrite);
  if (!is_code && !is_rodata) { return 0; }
  U32 sn = (U32)sect_idx + 1;
  COFF_SectionHeader *header = lnk_coff_section_header_from_section_number(obj, sn);
  if (header->fsize == 0) { return 0; }
  LNK_Symbol *sym = lnk_obj_get_comdat_symlink(obj, sn);
  if (sym == 0) { return include_static ? 2 : 0; } // static COMDAT: opt-in /OPT:ICFSTATIC, folded via icf_fold + dead-strip
  LNK_ObjSymbolRef sym_ref = lnk_ref_from_symbol(sym);
  if (sym_ref.obj != obj) { return 0; } // same-name follower (already removed)
  COFF_ParsedSymbol sym_parsed = lnk_parsed_from_symbol(sym);
  if (sym_parsed.section_number != sn || sym_parsed.value != 0) { return 0; }
  return 1; // external leader
}

typedef struct LNK_ICFCollectTask
{
  LNK_Obj     **objs;
  U64          *counts;
  U64          *offsets;
  LNK_ICFCand  *cands;
  B32           include_static;
} LNK_ICFCollectTask;

internal
THREAD_POOL_TASK_FUNC(lnk_icf_count_task)
{
  LNK_ICFCollectTask *t = raw_task;
  LNK_Obj *obj = t->objs[task_id];
  U64 n = 0;
  for EachIndex(si, obj->header.section_count_no_null) { if (lnk_icf_section_kind(obj, si, t->include_static)) { n += 1; } }
  t->counts[task_id] = n;
}

internal
THREAD_POOL_TASK_FUNC(lnk_icf_fill_task)
{
  LNK_ICFCollectTask *t = raw_task;
  LNK_Obj *obj = t->objs[task_id];
  U64 cur = t->offsets[task_id];
  for EachIndex(si, obj->header.section_count_no_null) {
    U32 kind = lnk_icf_section_kind(obj, si, t->include_static);
    if (kind) {
      LNK_ICFCand *c = &t->cands[cur++];
      c->obj = obj; c->sn = (U32)si + 1;
      c->is_static = (kind == 2);
      // count relocs here (parallel, the section is already in hand) so lnk_opt_icf only needs a
      // cheap serial prefix sum for reloc_first instead of re-parsing every section serially.
      COFF_SectionHeader *header = lnk_coff_section_header_from_section_number(obj, c->sn);
      c->reloc_first = 0;
      c->reloc_count = (U32)lnk_coff_relocs_from_section_header(obj, header).count;
      c->key0 = 0;
    }
  }
}

// --- parallel LSD radix sort (U64 key + U32 value), 8 x 8-bit passes -------------------------
// 8-bit digits keep the serial per-pass offset prefix tiny (256*workers, not 65536*workers);
// the extra passes are parallel histogram+scatter, so they cost little.
#if !defined(LNK_RADIX_BITS)
#define LNK_RADIX_BITS 8
#define LNK_RADIX_SIZE (1 << LNK_RADIX_BITS)
#endif
typedef struct LNK_RadixSortTask
{
  Rng1U64 *ranges;
  U64     *ksrc; U32 *vsrc; // read
  U64     *kdst; U32 *vdst; // write (scatter)
  U32     *hist;            // [worker_count * LNK_RADIX_SIZE], per-worker digit offsets
  U64      shift;
} LNK_RadixSortTask;

internal
THREAD_POOL_TASK_FUNC(lnk_radix_hist_task)
{
  LNK_RadixSortTask *t = raw_task;
  U32 *h = t->hist + (U64)task_id * LNK_RADIX_SIZE;
  for EachInRange(i, t->ranges[task_id]) { h[(t->ksrc[i] >> t->shift) & (LNK_RADIX_SIZE - 1)] += 1; }
}

internal
THREAD_POOL_TASK_FUNC(lnk_radix_scatter_task)
{
  LNK_RadixSortTask *t = raw_task;
  U32 *h = t->hist + (U64)task_id * LNK_RADIX_SIZE;
  for EachInRange(i, t->ranges[task_id]) {
    U64 d   = (t->ksrc[i] >> t->shift) & (LNK_RADIX_SIZE - 1);
    U32 pos = h[d]++;
    t->kdst[pos] = t->ksrc[i];
    t->vdst[pos] = t->vsrc[i];
  }
}

// sort (keys[], vals[]) ascending by key, in place. arena supplies scratch (double buffers + histograms).
internal void
lnk_radix_sort_u64_pairs(TP_Context *tp, Arena *arena, U64 n, U64 *keys, U32 *vals)
{
  if (n < 2) { return; }
  U64 W = tp->worker_count;

  // Only sort over the passes that can actually differ: many call sites key by dense color ids
  // (small ints), so the top 4-6 radix bytes are all zero. Each skipped pass is a full
  // hist+scatter over n pairs (memory-bound). An even pass_count keeps the result in the
  // original arrays (the double-buffer swap invariant), so round max_key up to a full byte.
  U64 max_key = 0;
  for EachIndex(i, n) { if (keys[i] > max_key) { max_key = keys[i]; } }
  U64 pass_count = 64 / LNK_RADIX_BITS;
  if (max_key != 0) {
    U64 sig_bits   = 64 - clz64(max_key);
    U64 sig_passes = (sig_bits + LNK_RADIX_BITS - 1) / LNK_RADIX_BITS;
    pass_count = sig_passes + (sig_passes & 1); // keep even so data lands back in keys/vals
  } else {
    pass_count = 0; // all keys equal (0); already trivially sorted, scatter is identity
  }
  if (pass_count == 0) { return; }

  Temp scratch = scratch_begin(&arena, 1); // internal buffers freed on return
  LNK_RadixSortTask t = {0};
  t.ranges = tp_divide_work(scratch.arena, n, W);
  t.hist   = push_array_no_zero(scratch.arena, U32, W * LNK_RADIX_SIZE);
  U64 *kbuf = push_array_no_zero(scratch.arena, U64, n);
  U32 *vbuf = push_array_no_zero(scratch.arena, U32, n);

  U64 *ksrc = keys, *kdst = kbuf;
  U32 *vsrc = vals, *vdst = vbuf;
  for (U64 pass = 0; pass < pass_count; pass += 1) {
    t.shift = pass * LNK_RADIX_BITS;
    t.ksrc = ksrc; t.vsrc = vsrc; t.kdst = kdst; t.vdst = vdst;

    MemoryZero(t.hist, sizeof(U32) * W * LNK_RADIX_SIZE);
    tp_for_parallel(tp, 0, W, lnk_radix_hist_task, &t);

    // exclusive prefix across (bucket, worker) so each worker writes a disjoint contiguous run
    U64 running = 0;
    for EachIndex(bucket, LNK_RADIX_SIZE) {
      for EachIndex(w, W) {
        U32 *slot = &t.hist[w * LNK_RADIX_SIZE + bucket];
        U32  c    = *slot;
        *slot     = (U32)running;
        running  += c;
      }
    }

    tp_for_parallel(tp, 0, W, lnk_radix_scatter_task, &t);

    U64 *kt = ksrc; ksrc = kdst; kdst = kt;
    U32 *vt = vsrc; vsrc = vdst; vdst = vt;
  }
  // even pass count -> sorted data is back in the original keys/vals arrays
  scratch_end(scratch);
}

// assign each candidate a dense equivalence-class id from its key, via a parallel sort + a
// cheap sequential group scan. Returns the number of distinct classes (for convergence).
internal U64
lnk_icf_dense_colors(TP_Context *tp, Arena *arena, U64 n, U64 *keys, U32 *colors)
{
  Temp t  = temp_begin(arena);
  U64 *sk = push_array_no_zero(t.arena, U64, n ? n : 1);
  U32 *sv = push_array_no_zero(t.arena, U32, n ? n : 1);
  for EachIndex(ci, n) { sk[ci] = keys[ci]; sv[ci] = (U32)ci; }
  lnk_radix_sort_u64_pairs(tp, t.arena, n, sk, sv);
  U64 nc = 0;
  for EachIndex(k, n) {
    if (k == 0 || sk[k] != sk[k - 1]) { nc += 1; }
    colors[sv[k]] = (U32)(nc - 1);
  }
  temp_end(t);
  return nc;
}

// ICF fold VERIFY (parallel, read-only). Per color group: elect the leader, then byte+reloc-verify
// each follower against it. The dominant cost is the str8_match/memcmp byte compare over millions of
// candidate sections -- read-only, so it parallelizes across groups with no shared writes. Output:
// leader_sci[k] = the elected leader's sci-index for sorted position k IF k is a verified follower
// that folds, else max_U32. The serial apply pass (in lnk_opt_icf) consumes this in group order, so
// the symlink/icf_fold writes stay serial+deterministic (cross-class symlink chains must not race).
typedef struct LNK_ICFFoldVerifyTask
{
  Rng1U64     *ranges;      // group-index ranges per worker
  U32         *group_first;
  U32         *sci;
  LNK_ICFCand *cands;
  U8          *rt_iscand;
  U64         *rt_target;
  U32         *colors;
  U32         *leader_sci;  // out: per sorted position -> leader's sci index, or max_U32
  U32         *group_leader_oi; // out: per group -> elected leader sorted-position (for apply)
} LNK_ICFFoldVerifyTask;

internal
THREAD_POOL_TASK_FUNC(lnk_icf_fold_verify_task)
{
  LNK_ICFFoldVerifyTask *task = raw_task;
  for EachInRange(gi, task->ranges[task_id]) {
    U64 i = task->group_first[gi];
    U64 j = task->group_first[gi + 1];
    task->group_leader_oi[gi] = (U32)i;
    if (j - i < 2) { continue; }

    U64 leader_oi = i;
    for (U64 k = i + 1; k < j; k += 1) {
      LNK_ICFCand *a = &task->cands[task->sci[leader_oi]];
      LNK_ICFCand *b = &task->cands[task->sci[k]];
      B32 b_better = (b->is_static <  a->is_static) ||
                     (b->is_static == a->is_static &&
                      Compose64Bit(b->obj->input_idx, b->sn) < Compose64Bit(a->obj->input_idx, a->sn));
      if (b_better) { leader_oi = k; }
    }
    task->group_leader_oi[gi] = (U32)leader_oi;

    LNK_ICFCand        *L       = &task->cands[task->sci[leader_oi]];
    COFF_SectionHeader *Lheader = lnk_coff_section_header_from_section_number(L->obj, L->sn);
    String8             Ldata   = str8_substr(L->obj->data, rng_1u64(Lheader->foff, Lheader->foff + Lheader->fsize));

    for (U64 k = i; k < j; k += 1) {
      if (k == leader_oi) { continue; }
      LNK_ICFCand        *F       = &task->cands[task->sci[k]];
      COFF_SectionHeader *Fheader = lnk_coff_section_header_from_section_number(F->obj, F->sn);
      if (Fheader->fsize != Lheader->fsize) { continue; }
      if (F->reloc_count != L->reloc_count) { continue; }
      String8 Fdata = str8_substr(F->obj->data, rng_1u64(Fheader->foff, Fheader->foff + Fheader->fsize));
      if (!str8_match(Ldata, Fdata, 0)) { continue; }
      B32 relocs_match = 1;
      for EachIndex(t, L->reloc_count) {
        U64 li = L->reloc_first + t, fi = F->reloc_first + t;
        U64 lt = task->rt_iscand[li] ? task->colors[task->rt_target[li]] : task->rt_target[li];
        U64 ft = task->rt_iscand[fi] ? task->colors[task->rt_target[fi]] : task->rt_target[fi];
        if (lt != ft) { relocs_match = 0; break; }
      }
      if (!relocs_match) { continue; }
      task->leader_sci[k] = task->sci[leader_oi]; // verified fold
    }
  }
}

// ============================================================================================
// ICF DIRTY-CLASS WORKLIST (TAIL ACCELERATOR)
//
// The persistent refine region re-sorts the ENTIRE active set (~10.65M candidates) every round until a
// round splits nothing. Measured churn (live UE link): the partition is ~99.99% stable by round ~7; in
// the last ~11 rounds only tens-to-hundreds of candidates land in a class that actually SPLITS, yet each
// round still pays the full ~10.65M-key sort. The work is fixed by the sort size, NOT by the (tiny)
// amount of real refinement left. That tail is ~3-4s of pure waste.
//
// A candidate's refine key can only change if its OWN color changed (its class split) OR one of its
// reloc TARGETS changed color (a target's class split). So once a class is stable AND none of the
// classes it points at split, it can NEVER split again -- re-keying/re-sorting it is provably a no-op.
//
// The worklist tracks exactly the classes that can still split (DIRTY classes). Each round it re-keys
// only members of dirty classes, against a FROZEN colors[] snapshot (Jacobi: all keys this round read
// pre-round colors; new colors commit only at the round barrier -- reading mid-round colors would
// over-refine past the unique coarsest stable partition and may never terminate). When a class splits,
// every candidate that REFERENCES a member of the split class is enqueued next round (reverse
// target->referrers index). Dirty set empty => fixpoint.
//
// This reaches the IDENTICAL coarsest stable partition as the full region (same refinement operator; we
// only skip recomputation that is provably a no-op). The absolute color-id VALUES differ (ids from a
// separate running counter), but EVERY downstream consumer -- fold grouping (sorts by color, groups =
// equal-color runs), leader election (by candidate identity, not id), fold-verify (tests
// colors[a]==colors[b]) -- depends ONLY on the equivalence relation, never on id values or class
// ordering. So an id-renamed but partition-identical colors[] is byte-identical output. (Asserted every
// round by ICF_WORKLIST_SELFCHECK against the full region partition over the whole UE link.)
// ============================================================================================

// Build CSR reverse adjacency: for target candidate T, the referrers (candidates C with a reloc C->T)
// are rev_adj[rev_off[T] .. rev_off[T+1]). Deterministic (cands iterated in order -> referrers sorted by
// referrer index). Self-edges (C references its own class) are kept; harmless (re-keys C's own class).
// serial in-place sort of (keys[],vals[]) ascending by (key, then val). Used INSIDE the worklist per
// dirty class -- classes are small (a few..hundreds of members), so a pool-free serial sort beats
// re-entering the thread pool thousands of times per round. Stable on equal keys is not required (we
// sub-sort sub-runs by val anyway), but we break key ties by val for a deterministic order.
internal void
lnk_icf_small_sort(U64 *keys, U32 *vals, U64 n)
{
  // insertion sort for tiny n; introsort-lite (here: just insertion + a simple shellsort gap for
  // larger classes) keeps it dependency-free and deterministic. n is bounded by the largest class.
  for (U64 gap = n / 2; gap > 0; gap /= 2) {
    for (U64 i = gap; i < n; i += 1) {
      U64 k = keys[i]; U32 v = vals[i];
      U64 j = i;
      while (j >= gap) {
        U64 pk = keys[j - gap]; U32 pv = vals[j - gap];
        if (pk < k || (pk == k && pv <= v)) { break; }
        keys[j] = pk; vals[j] = pv; j -= gap;
      }
      keys[j] = k; vals[j] = v;
    }
  }
}

internal void
lnk_icf_build_reverse_index(Arena *arena, U64 cand_count, LNK_ICFCand *cands, U8 *rt_iscand, U64 *rt_target,
                            U32 **out_rev_off, U32 **out_rev_adj)
{
  U32 *rev_off = push_array(arena, U32, cand_count + 1);
  for EachIndex(ci, cand_count) {
    LNK_ICFCand *c = &cands[ci];
    for EachIndex(j, c->reloc_count) {
      U64 idx = (U64)c->reloc_first + j;
      if (rt_iscand[idx]) { rev_off[(U32)rt_target[idx] + 1] += 1; }
    }
  }
  for EachIndex(t, cand_count) { rev_off[t + 1] += rev_off[t]; }
  U64 edge_count = rev_off[cand_count];
  U32 *rev_adj = push_array_no_zero(arena, U32, edge_count ? edge_count : 1);
  Temp t = temp_begin(arena);
  U32 *cursor = push_array(t.arena, U32, cand_count ? cand_count : 1);
  for EachIndex(ci, cand_count) {
    LNK_ICFCand *c = &cands[ci];
    for EachIndex(j, c->reloc_count) {
      U64 idx = (U64)c->reloc_first + j;
      if (rt_iscand[idx]) {
        U32 tg = (U32)rt_target[idx];
        rev_adj[rev_off[tg] + cursor[tg]] = (U32)ci;
        cursor[tg] += 1;
      }
    }
  }
  temp_end(t);
  *out_rev_off = rev_off;
  *out_rev_adj = rev_adj;
}

// Dirty-class worklist tail. Precondition: colors[] holds a stable-so-far partition; `active`/
// `active_count` is the current active set (members of classes of size>=2); color_base = next free id.
// Runs the worklist to fixpoint, updating colors[] in place. Returns the round count via *out_rounds and
// 1 on success / 0 if the hang-guard tripped (caller must NOT ship -- it is a bug).
//
// Data model: the work unit is a CLASS (a color). Per-color member lists live in one flat slab `mem[]`;
// a class's members are mem[slot_first[s] .. slot_first[s]+slot_count[s]). `slot_of_color` maps a live
// color -> slot+1. A split replaces the parent slot's mem range, in sorted (newkey, cand-idx) order, with
// its sub-runs: the first sub-run KEEPS the parent slot & color; later sub-runs get fresh slots & fresh
// color ids. Singletons are dropped (final). Determinism: members in ascending cand index within a class;
// sub-runs sub-sorted by cand index; new ids from a deterministic running counter.
internal B32
lnk_icf_worklist_refine(TP_Context *tp, Arena *arena, U64 cand_count, LNK_ICFCand *cands,
                        U32 *colors, U8 *rt_iscand, U64 *rt_target,
                        U32 *rev_off, U32 *rev_adj,
                        U32 *active, U64 active_count, U64 color_base, U64 *out_rounds)
{
  (void)tp; (void)cand_count;
  Temp t = temp_begin(arena);

  // capacities: members <= active_count; every split adds >=1 new slot but total live members never
  // exceeds active_count, and a class of size m can split into at most m sub-classes, so total slots
  // ever created <= active_count. Size all slot-indexed arrays to active_count (+slack).
  U64 cap = active_count + 16;
  U32 *mem        = push_array_no_zero(t.arena, U32, cap);
  U32 *slot_first = push_array_no_zero(t.arena, U32, cap);
  U32 *slot_count = push_array_no_zero(t.arena, U32, cap);
  U32 *slot_color = push_array_no_zero(t.arena, U32, cap);
  U64  slot_n = 0, mem_n = 0;
  LNK_ICFMap slot_of_color = lnk_icf_map_make(t.arena, cap);

  // initial slots: group the active set by current color (one sort). Active members are already only in
  // classes of size>=2, so every produced slot has count>=2.
  {
    U64 *sk = push_array_no_zero(t.arena, U64, active_count ? active_count : 1);
    U32 *sv = push_array_no_zero(t.arena, U32, active_count ? active_count : 1);
    for EachIndex(i, active_count) { sk[i] = (U64)colors[active[i]]; sv[i] = active[i]; }
    lnk_radix_sort_u64_pairs(tp, t.arena, active_count, sk, sv); // by color, ties by cand idx (stable-by-value)
    for (U64 a = 0; a < active_count; ) {
      U32 col = (U32)sk[a];
      U64 b = a; while (b < active_count && (U32)sk[b] == col) { b += 1; }
      U32 first = (U32)mem_n;
      for (U64 q = a; q < b; q += 1) { mem[mem_n++] = sv[q]; }
      slot_first[slot_n] = first; slot_count[slot_n] = (U32)(b - a); slot_color[slot_n] = col;
      lnk_icf_map_put(&slot_of_color, col, slot_n + 1);
      slot_n += 1;
      a = b;
    }
  }

  // dirty worklist = slots to re-densify this round. Seed with all slots.
  U32 *dirty      = push_array_no_zero(t.arena, U32, cap);
  U32 *next_dirty = push_array_no_zero(t.arena, U32, cap);
  U8  *in_dirty   = push_array(t.arena, U8, cap); // dedup membership for next_dirty (cleared after swap)
  U64 dirty_n = 0;
  for EachIndex(s, slot_n) { dirty[dirty_n++] = (U32)s; }

  // staged this round (committed only at the round barrier so re-keying reads FROZEN colors[]):
  //   - new color assignments (ci -> color); applied to colors[] after the round
  //   - newly created slots (from 2nd+ sub-runs of splits)
  // The parent slot's mem range is rewritten in place immediately (disjoint from other slots' ranges and
  // independent of colors[]), so member order is settled during the dirty loop; only colors[]/slot
  // registration is deferred.
  U32 *stage_ci    = push_array_no_zero(t.arena, U32, active_count ? active_count : 1);
  U32 *stage_col   = push_array_no_zero(t.arena, U32, active_count ? active_count : 1);
  U32 *split_mem   = push_array_no_zero(t.arena, U32, active_count ? active_count : 1); // members of split classes
  U32 *ns_slot     = push_array_no_zero(t.arena, U32, cap); // staged new slot indices
  U32 *ns_first    = push_array_no_zero(t.arena, U32, cap);
  U32 *ns_count    = push_array_no_zero(t.arena, U32, cap);
  U32 *ns_color    = push_array_no_zero(t.arena, U32, cap);

  // hang-guard: hard round cap AND non-progress detection. The worklist tail converges in <~20 cheap
  // rounds; if dirty_n fails to strictly shrink for too many consecutive rounds, it is a re-dirty bug.
  enum { LNK_ICF_WL_MAX_ROUNDS = 40, LNK_ICF_WL_STALL_LIMIT = 8 };
  U64 rounds = 0, stall = 0, prev_dirty_n = max_U64;
  B32 ok = 1;

  while (dirty_n > 0) {
    if (rounds >= LNK_ICF_WL_MAX_ROUNDS) { ok = 0; break; }
    if (dirty_n >= prev_dirty_n) { stall += 1; if (stall >= LNK_ICF_WL_STALL_LIMIT) { ok = 0; break; } }
    else { stall = 0; }
    prev_dirty_n = dirty_n;
    rounds += 1;

    U64 stage_n = 0, split_n = 0, ns_n = 0;

    for EachIndex(di, dirty_n) {
      U32 slot  = dirty[di];
      U32 m     = slot_count[slot];
      if (m < 2) { continue; }
      U32 first = slot_first[slot];

      // re-key this class's members against the FROZEN colors[] snapshot (colors[] not yet mutated)
      Temp tk = temp_begin(t.arena);
      U64 *kk = push_array_no_zero(tk.arena, U64, m);
      U32 *vv = push_array_no_zero(tk.arena, U32, m);
      for EachIndex(r, m) {
        U32 ci = mem[first + r];
        LNK_ICFCand *c = &cands[ci];
        U64 k = lnk_icf_mix(0x9e3779b97f4a7c15ull, colors[ci]);
        for EachIndex(j, c->reloc_count) {
          U64 idx = (U64)c->reloc_first + j;
          U64 tt  = rt_iscand[idx] ? colors[rt_target[idx]] : rt_target[idx];
          k = lnk_icf_mix(k, tt);
        }
        kk[r] = k; vv[r] = ci;
      }
      lnk_icf_small_sort(kk, vv, m); // serial, pool-free: group members by new key (ties by cand idx)

      U64 sub = 0;
      for EachIndex(r, m) { if (r == 0 || kk[r] != kk[r - 1]) { sub += 1; } }
      if (sub == 1) { temp_end(tk); continue; } // class did not split

      // class splits. Rewrite parent mem range in sorted vv order (settles member order now). The 1st
      // sub-run keeps slot/color; later sub-runs are staged as new slots with fresh ids. colors[] writes
      // are STAGED (deferred to commit) so other dirty classes this round still read frozen colors.
      for EachIndex(r, m) { mem[first + r] = vv[r]; }
      U64 run0 = 0; B32 first_run = 1;
      for (U64 r = 1; r <= m; r += 1) {
        B32 boundary = (r == m) || (kk[r] != kk[r - 1]);
        if (!boundary) { continue; }
        U64 rn = r - run0;
        U32 run_first = (U32)(first + run0);
        U32 run_color;
        if (first_run) {
          run_color = slot_color[slot]; first_run = 0;
          // parent slot's count shrinks to the 1st sub-run; staged so dirty loop still sees old count.
          ns_slot[ns_n] = slot; // reuse parent slot
        } else {
          run_color = (U32)color_base; color_base += 1;
          ns_slot[ns_n] = max_U32;  // allocate a fresh slot at commit
        }
        ns_first[ns_n] = run_first; ns_count[ns_n] = (U32)rn; ns_color[ns_n] = run_color; ns_n += 1;
        for (U64 q = run0; q < r; q += 1) {
          U32 ci = mem[first + q];
          stage_ci[stage_n] = ci; stage_col[stage_n] = run_color; stage_n += 1;
          split_mem[split_n++] = ci;
        }
        run0 = r;
      }
      temp_end(tk);
    }

    if (stage_n == 0) { break; } // nothing split -> fixpoint

    // COMMIT (round barrier): register slots, then apply colors[]. (Order matters only that all reads in
    // the dirty loop above already finished -- they did.)
    for EachIndex(s, ns_n) {
      U32 rslot = ns_slot[s];
      if (rslot == max_U32) { rslot = (U32)slot_n; slot_n += 1; }
      slot_first[rslot] = ns_first[s]; slot_count[rslot] = ns_count[s]; slot_color[rslot] = ns_color[s];
      lnk_icf_map_put(&slot_of_color, (U64)ns_color[s], rslot + 1);
    }
    for EachIndex(i, stage_n) { colors[stage_ci[i]] = stage_col[i]; }

    // next dirty set: every referrer of a split-class member, mapped to its CURRENT class slot (size>=2).
    U64 next_n = 0;
    for EachIndex(i, split_n) {
      U32 mem_ci = split_mem[i];
      for (U32 e = rev_off[mem_ci]; e < rev_off[mem_ci + 1]; e += 1) {
        U32 ref = rev_adj[e];
        U64 sc  = lnk_icf_map_get(&slot_of_color, (U64)colors[ref], 0);
        if (sc) {
          U32 rslot = (U32)(sc - 1);
          if (slot_count[rslot] >= 2 && !in_dirty[rslot]) { in_dirty[rslot] = 1; next_dirty[next_n++] = rslot; }
        }
      }
    }
    U32 *tmp = dirty; dirty = next_dirty; next_dirty = tmp;
    dirty_n = next_n;
    for EachIndex(i, dirty_n) { in_dirty[dirty[i]] = 0; }

    if (lnk_get_log_status(LNK_Log_Debug)) {
      lnk_log(LNK_Log_Debug, "/OPT:ICF worklist round %llu: split_members=%llu next_dirty=%llu slots=%llu",
              rounds, split_n, dirty_n, slot_n);
    }
  }

  temp_end(t);
  if (out_rounds) { *out_rounds = rounds; }
  return ok;
}

// relocations point at equivalent targets (iteratively), then folds each group's followers
// into a single leader by routing them through the existing COMDAT symlink machinery and
// letting /OPT:REF garbage-collect the now-unreferenced follower sections (and their
// associated .pdata/.xdata/.debug$S). Mirrors link.exe /OPT:ICF.
internal void
lnk_opt_icf(TP_Context *tp, Arena *perm, LNK_SymbolTable *symtab, LNK_Config *config, LNK_ObjList objs_list)
{
  if (config->opt_icf != LNK_SwitchState_Yes) { return; }

  ProfBeginFunction();
  Temp scratch = scratch_begin(&perm, 1);
  Arena *arena = scratch.arena;

  U64        objs_count = objs_list.count;
  LNK_Obj  **objs       = lnk_array_from_obj_list(arena, objs_list);

  // allocate the per-section static-fold map on every obj (sn-1 indexed, zeroed = not folded). It
  // lives on the obj (not scratch) because /OPT:REF and the final section map read it after ICF.
  for EachIndex(oi, objs_count) {
    objs[oi]->icf_fold = push_array(perm, LNK_ICFFold, objs[oi]->header.section_count_no_null ? objs[oi]->header.section_count_no_null : 1);
  }

  // collect candidate sections (parallel per obj): count, exact-size, then fill at per-obj
  // offsets. Counting first avoids sizing the array to the total section count (which would be
  // ~10x larger and waste ~1GB on UE-scale inputs).
  LNK_ICFCollectTask col = {0};
  col.include_static = (config->opt_icf_static == LNK_SwitchState_Yes);
  col.objs    = objs;
  col.counts  = push_array(arena, U64, objs_count ? objs_count : 1);
  tp_for_parallel(tp, 0, objs_count, lnk_icf_count_task, &col);
  col.offsets = offsets_from_counts_array_u64(arena, col.counts, objs_count);
  U64          cand_count = sum_array_u64(objs_count, col.counts);
  LNK_ICFCand *cands      = push_array_no_zero(arena, LNK_ICFCand, cand_count ? cand_count : 1);
  col.cands   = cands;
  tp_for_parallel(tp, 0, objs_count, lnk_icf_fill_task, &col);

  if (cand_count < 2) { goto done; }

  // build target lookup (input_idx, section_number) -> cand_idx+1, sized to the candidate count
  LNK_ICFMap cand_map = lnk_icf_map_make(arena, cand_count);
  {
    LNK_ICFCandMapPutTask put_task = {0};
    put_task.ranges   = tp_divide_work(arena, cand_count, tp->worker_count);
    put_task.cands    = cands;
    put_task.cand_map = &cand_map;
    tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_candmap_put_task, &put_task);
  }

  // assign each candidate a disjoint slice in the flattened reloc-target arrays. reloc_count was
  // filled in parallel by lnk_icf_fill_task, so this is just a serial prefix sum (no re-parsing).
  U64 total_relocs = 0;
  for EachIndex(ci, cand_count) {
    cands[ci].reloc_first = (U32)total_relocs;
    total_relocs += cands[ci].reloc_count;
  }
  U8  *rt_iscand = push_array_no_zero(arena, U8,  total_relocs ? total_relocs : 1);
  U64 *rt_target = push_array_no_zero(arena, U64, total_relocs ? total_relocs : 1);

  {
    LNK_ICFHashTask hash_task = {0};
    hash_task.ranges    = tp_divide_work(arena, cand_count, tp->worker_count);
    hash_task.cands     = cands;
    hash_task.cand_map  = &cand_map;
    hash_task.symtab    = symtab;
    hash_task.rt_iscand = rt_iscand;
    hash_task.rt_target = rt_target;
    tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_hash_task, &hash_task);
  }

  // dense per-candidate equivalence-class colors (separate 8B array; hot refine/scan loops gather
  // these by index millions of times -- keeping them out of the 40B LNK_ICFCand keeps the gathers
  // cache-dense). zero-init: round 0 assigns real colors.
  U32 *colors = push_array(arena, U32, cand_count ? cand_count : 1);

  // assign initial colors from content key (parallel sort + group scan)
  U64 *newkey = push_array_no_zero(arena, U64, cand_count ? cand_count : 1);
  for EachIndex(ci, cand_count) { newkey[ci] = cands[ci].key0; }
  U64 class_count = lnk_icf_dense_colors(tp, arena, cand_count, newkey, colors);
  U64 color_base  = class_count; // next free class id; ids only ever grow, so finalized colors stick

  // Active set = candidates still sharing a class with another. A singleton class can never split
  // or merge, so once a candidate is alone its color is final and it leaves refinement. Each round
  // re-densifies only the active set, shrinking the per-round sort from all ~N candidates down to
  // just those that still have a content+reloc twin.
  // ping-pong active buffers: refine reads `active`, densify writes compacted survivors into `active2`.
  // They must be distinct (densify reads active[sv[m]] while writing next_active[]). Swap each round.
  U32 *active  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
  U32 *active2 = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
  U64  active_count = 0, active_class_count = 0;
  {
    Temp t = temp_begin(arena);
    U32 *cls_size = push_array(t.arena, U32, class_count ? class_count : 1);
    for EachIndex(ci, cand_count) { cls_size[colors[ci]] += 1; }
    for EachIndex(ci, cand_count) { if (cls_size[colors[ci]] > 1) { active[active_count++] = (U32)ci; } }
    for EachIndex(c, class_count)  { if (cls_size[c] > 1) { active_class_count += 1; } }
    temp_end(t);
  }

  // iteratively refine classes by relocation-target classes until no class splits.
  // PERSISTENT-WORKER REGION: collapse the ~18 rounds x ~10 phases of fork-join into ONE parallel
  // region (lnk_icf_refine_region_task) where every phase boundary is a barrier and the serial glue
  // (radix prefixes, group-scan prefix, convergence/swap) runs on worker 0. Byte-identical to the
  // per-round path (preserves chunk boundaries, processing order, and serial-prefix math).
  if (active_count > 0) {
    // FAIR-SHARE: pin the barrier-pass cohort BEFORE building the region's
    // per-worker scratch and rs.worker_count, so all of W / rs.worker_count /
    // hist[W*..] / chunk_*[W] / ranges[W+1] / the C-sized barrier agree on the
    // cohort. tp->worker_count now reads the cohort C for the whole region.
    tp_barrier_begin(tp);

    LNK_ICFRegion rs = {0};
    rs.tp           = tp;
    rs.cands        = cands;
    rs.rt_iscand    = rt_iscand;
    rs.rt_target    = rt_target;
    rs.cand_count   = cand_count;
    rs.worker_count = tp->worker_count;
    rs.active       = active;
    rs.active2      = active2;
    rs.colors       = colors;
    rs.newkey       = newkey;

    // per-round scratch, preallocated once to the max possible size (cand_count); reused every round.
    U64 W = tp->worker_count;
    rs.sk        = push_array_no_zero(arena, U64, cand_count ? cand_count : 1);
    rs.sv        = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
    rs.color_at  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
    rs.keep      = push_array_no_zero(arena, U8,  cand_count ? cand_count : 1);
    rs.out_slot  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
    rs.kbuf      = push_array_no_zero(arena, U64, cand_count ? cand_count : 1);
    rs.vbuf      = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
    rs.hist      = push_array_no_zero(arena, U32, W * LNK_RADIX_SIZE);
    rs.chunk_nc  = push_array_no_zero(arena, U64, W ? W : 1);
    rs.chunk_kp  = push_array_no_zero(arena, U64, W ? W : 1);
    rs.chunk_sc  = push_array_no_zero(arena, U64, W ? W : 1);
    rs.chunk_max = push_array_no_zero(arena, U64, (W > 3 ? W : 3) ? (W > 3 ? W : 3) : 1);
    rs.refine_ranges = push_array_no_zero(arena, Rng1U64, W + 1);
    rs.work_ranges   = push_array_no_zero(arena, Rng1U64, W + 1);

    rs.active_count       = active_count;
    rs.active_class_count = active_class_count;
    rs.color_base         = color_base;
    rs.converged          = 0;

    // TAIL ACCELERATOR: run the persistent region for a bounded number of warm-up rounds (churn is large
    // here, the full parallel sort wins), then hand the still-active set to the dirty-class worklist for
    // the long near-converged tail (rounds 8..18 re-sort ~10.65M to resolve a few hundred splits). Both
    // compute the unique coarsest stable partition, so the final colors[] partition is identical; the
    // worklist just reaches it in tail work ~= churn instead of ~= active_count.
    // Worklist tail DISABLED by default: its CSR reverse-index (rev_adj = U32 x candidate-edge-count)
    // costs ~10GB peak on the UE link, which hurts the page-fault-bound critical path more than the
    // ~3-4s of tail re-sorts it saves. region_cap=64 lets the persistent region run to the true fixpoint
    // (~19 rounds), so `converged` is set and the worklist handoff below is skipped (no reverse-index).
    // Lower this back to 8 to re-enable the worklist on memory-rich machines.
    U64 region_cap = 64; // was 8 (worklist warm-up)

#if defined(ICF_WORKLIST_SELFCHECK)
    // REFERENCE: clone pre-region state and run the region UNCAPPED to the true fixpoint into shadow
    // colors[]; later assert the cap+worklist partition is identical every link. Reuses rs scratch
    // buffers (sequential, not concurrent). active2/colors are restored to pre-region values first.
    U32 *ref_colors  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
    MemoryCopy(ref_colors, colors, sizeof(U32) * cand_count);
    {
      U32 *ref_active  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
      U32 *ref_active2 = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
      MemoryCopy(ref_active, active, sizeof(U32) * active_count);
      LNK_ICFRegion ref = rs;            // copy all scratch pointers + sizes
      ref.active  = ref_active;
      ref.active2 = ref_active2;
      ref.colors  = ref_colors;
      ref.active_count = active_count; ref.active_class_count = active_class_count;
      ref.color_base = color_base; ref.converged = 0;
      ref.max_rounds = 64;             // uncapped (true fixpoint)
      tp_for_parallel_reserve(tp, 0, tp->worker_count, lnk_icf_refine_region_task, &ref); // BARRIER pass (path B)
    }
    // colors[] was untouched (ref ran on ref_colors); active/active2 untouched (ref used clones).
#endif

    rs.max_rounds = region_cap;
    tp_for_parallel_reserve(tp, 0, tp->worker_count, lnk_icf_refine_region_task, &rs); // BARRIER pass (path B)

    // mirror final state back (active/colors already mutated in place; pointers may have swapped)
    active             = rs.active;
    active2            = rs.active2;
    color_base         = rs.color_base;
    active_count       = rs.active_count;
    active_class_count = rs.active_class_count;

    // FAIR-SHARE: close the cohort bracket now -- the region's barrier passes are
    // done. The worklist tail + fold below are path-A (governor-driven), so they
    // must NOT run with the cohort pinned (the held slots / barrier_pass=1 would
    // block path-A workers from returning budget). Restore full width here.
    tp_barrier_end(tp);

    // hand off the tail to the dirty-class worklist if the region stopped at the cap (not yet converged)
    if (!rs.converged && active_count > 0) {
      U32 *rev_off = 0, *rev_adj = 0;
      lnk_icf_build_reverse_index(arena, cand_count, cands, rt_iscand, rt_target, &rev_off, &rev_adj);
      U64 wrounds = 0;
      B32 wl_ok = lnk_icf_worklist_refine(tp, arena, cand_count, cands, colors, rt_iscand, rt_target,
                                          rev_off, rev_adj, active, active_count, color_base, &wrounds);
      AssertAlways(wl_ok); // hang-guard tripped (round cap / non-progress) -> BUG, do not ship
      if (lnk_get_log_status(LNK_Log_Debug)) {
        lnk_log(LNK_Log_Debug, "/OPT:ICF worklist tail: %llu rounds (active=%llu at handoff)", wrounds, active_count);
      }
    }

#if defined(ICF_WORKLIST_SELFCHECK)
    // ASSERT: the cap+worklist colors[] induce the SAME partition as the uncapped region (ref_colors).
    // Partition equality = the map colors[ci] -> ref_colors[ci] is a consistent bijection (both ways).
    {
      Temp tc = temp_begin(arena);
      LNK_ICFMap fwd = lnk_icf_map_make(tc.arena, cand_count); // colors -> ref_colors
      LNK_ICFMap bwd = lnk_icf_map_make(tc.arena, cand_count); // ref_colors -> colors
      U64 mism = 0;
      for EachIndex(ci, cand_count) {
        U64 a = (U64)colors[ci], b = (U64)ref_colors[ci];
        U64 fa = lnk_icf_map_get(&fwd, a, max_U64);
        if (fa == max_U64) { lnk_icf_map_put(&fwd, a, b); } else if (fa != b) { mism += 1; }
        U64 fb = lnk_icf_map_get(&bwd, b, max_U64);
        if (fb == max_U64) { lnk_icf_map_put(&bwd, b, a); } else if (fb != a) { mism += 1; }
      }
      if (mism) {
        lnk_log(LNK_Log_Debug, "ICF_WORKLIST_SELFCHECK PARTITION MISMATCH: %llu cands disagree", mism);
      }
      AssertAlways(mism == 0); // worklist partition != full-region partition -> WRONG OUTPUT, do not ship
      temp_end(tc);
    }
#endif
  }
  (void)active2; (void)color_base; (void)active_count; (void)active_class_count;

  // group candidates by final color (parallel sort) and fold followers into a leader
  U64 *keys = newkey; // reuse
  U32 *sci  = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
  for EachIndex(ci, cand_count) { keys[ci] = colors[ci]; sci[ci] = (U32)ci; }
  lnk_radix_sort_u64_pairs(tp, arena, cand_count, keys, sci);

  // serial pass: find group boundaries (cheap O(cand_count) scan over the sorted color keys). Each
  // group [group_first[g], group_first[g+1]) shares one color. Verification + folding runs in
  // parallel across groups (groups touch disjoint sections/symlink nodes -> race-free, deterministic).
  U32 *group_first = push_array_no_zero(arena, U32, cand_count + 1);
  U64  group_count = 0;
  for (U64 i = 0; i < cand_count; ) {
    group_first[group_count++] = (U32)i;
    U64 color = keys[i];
    U64 j = i + 1;
    while (j < cand_count && keys[j] == color) { j += 1; }
    i = j;
  }
  group_first[group_count] = (U32)cand_count;

  // PARALLEL verify (read-only byte+reloc compare across groups) -> SERIAL apply (deterministic,
  // group-ordered symlink/icf_fold writes; cross-class symlink chains must not race, so writes stay
  // serial). leader_sci[k] = leader's sci index if sorted position k folds, else max_U32.
  U32 *leader_sci      = push_array_no_zero(arena, U32, cand_count ? cand_count : 1);
  for EachIndex(k, cand_count) { leader_sci[k] = max_U32; }
  U32 *group_leader_oi = push_array_no_zero(arena, U32, group_count ? group_count : 1);
  if (group_count) {
    LNK_ICFFoldVerifyTask vt = {0};
    vt.ranges          = tp_divide_work(arena, group_count, tp->worker_count);
    vt.group_first     = group_first;
    vt.sci             = sci;
    vt.cands           = cands;
    vt.rt_iscand       = rt_iscand;
    vt.rt_target       = rt_target;
    vt.colors          = colors;
    vt.leader_sci      = leader_sci;
    vt.group_leader_oi = group_leader_oi;
    tp_for_parallel(tp, 0, tp->worker_count, lnk_icf_fold_verify_task, &vt);
  }

  U64 fold_count = 0;
  for EachIndex(gi, group_count) {
    U64 i = group_first[gi];
    U64 j = group_first[gi + 1];
    if (j - i < 2) { continue; }
    U64                 leader_oi = group_leader_oi[gi];
    LNK_ICFCand        *L         = &cands[sci[leader_oi]];
    LNK_SymbolHashTrie *Lnode     = L->obj->symlinks[L->sn];
    for (U64 k = i; k < j; k += 1) {
      if (leader_sci[k] == max_U32) { continue; } // not a verified follower
      LNK_ICFCand        *F     = &cands[sci[k]];
      LNK_SymbolHashTrie *Fnode = F->obj->symlinks[F->sn];
      if (Fnode) {
        if (Lnode && Fnode != Lnode) { Fnode->symbol = Lnode->symbol; fold_count += 1; }
      } else {
        F->obj->icf_fold[F->sn - 1].leader_obj_idx = L->obj->input_idx;
        F->obj->icf_fold[F->sn - 1].leader_sn      = L->sn;
        F->obj->icf_fold[F->sn - 1].set            = 1;
        fold_count += 1;
      }
    }
  }

  if (lnk_get_log_status(LNK_Log_Debug)) {
    lnk_log(LNK_Log_Debug, "/OPT:ICF folded %llu of %llu code COMDATs into %llu classes", fold_count, cand_count, class_count);
  }

  done:;
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_opt_ref(TP_Context *tp, LNK_SymbolTable *symtab, LNK_Config *config, LNK_ObjList objs)
{
  ProfScope("Mark Live Sections")
    tp_for_parallel_reserve(tp, // BARRIER pass (path B): tp_broadcast + barrier_wait
                    0,
                    tp->worker_count,
                    lnk_walk_relocs_and_mark_ref_sections_task,
                    &(LNK_OptRefTask){ .symtab = symtab, .config = config, .objs = objs });
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
    COFF_SectionFlags   sect_flags  = obj->section_flags[sect_idx];

    if (~sect_flags & COFF_SectionFlag_LnkRemove && ~sect_flags & COFF_SectionFlag_LnkInfo && sect_header->fsize > 0) {
      Temp temp = temp_begin(scratch.arena);

      // was section defined?
      String8                sect_name            = coff_name_from_section_header(string_table, sect_header);
      String8                sect_name_with_flags = lnk_make_name_with_flags(temp.arena, sect_name, sect_flags & ~COFF_SectionFlags_LnkFlags);
      LNK_SectionDefinition *sect_defn            = hash_table_search_string_raw(sect_defn_ht, sect_name_with_flags);

      // push new section definition
      if (sect_defn == 0) {
        sect_defn = push_array(arena, LNK_SectionDefinition, 1);
        sect_defn->name         = sect_name;
        sect_defn->obj          = obj;
        sect_defn->obj_sect_idx = sect_idx;
        sect_defn->flags        = sect_flags & ~COFF_SectionFlags_LnkFlags;

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
    COFF_SectionFlags   sect_flags  = obj->section_flags[sect_idx];
    if (~sect_flags & COFF_SectionFlag_LnkRemove && ~sect_flags & COFF_SectionFlag_LnkInfo && sect_header->fsize > 0) {
      LNK_SectionContribChunk *sc_chunk = 0;
      {
        Temp temp = temp_begin(scratch.arena);
        String8 sect_name            = coff_name_from_section_header(string_table, sect_header);
        String8 sect_name_with_flags = lnk_make_name_with_flags(temp.arena, sect_name, sect_flags & ~COFF_SectionFlags_LnkFlags);
        sc_chunk = hash_table_search_string_raw(task->contribs_ht, sect_name_with_flags);
        temp_end(temp);
      }

      if (sc_chunk) {
        String8 data;
        if (sect_flags & COFF_SectionFlag_CntUninitializedData) {
          data = str8(0, sect_header->fsize);
        } else {
          data = str8_substr(obj->data, rng_1u64(sect_header->foff, sect_header->foff + sect_header->fsize));
        }

        U16 sc_align = coff_align_size_from_section_flags(sect_flags);
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

    if (~obj->section_flags[sect_idx] & COFF_SectionFlag_LnkCOMDAT) { continue; }

    LNK_Symbol *symlink = lnk_obj_get_comdat_symlink(obj, section_number);
    if (symlink == 0) { continue; }

    COFF_ParsedSymbol symlink_parsed = lnk_parsed_from_symbol(symlink);
    LNK_ObjSymbolRef  symlink_ref    = lnk_ref_from_symbol(symlink);
    task->sect_map[obj_idx][sect_idx] = task->sect_map[symlink_ref.obj->input_idx][symlink_parsed.section_number - 1];
  }
  ProfEnd();
}

// /OPT:ICF static-COMDAT followers have no external symbol to redirect, so point their section-map
// entry at the leader's contrib. The follower section is dead-stripped, but any residual reloc to a
// static symbol in it resolves its address via sect_map[obj][sn-1] -> leader's contrib.
internal
THREAD_POOL_TASK_FUNC(lnk_set_icf_static_leader_contribs_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  if (obj->icf_fold == 0) { return; }

  ProfBeginV("Set ICF Static Leader Contribs [%S]", obj->path);
  for EachIndex(sect_idx, obj->header.section_count_no_null) {
    LNK_ICFFold fold = obj->icf_fold[sect_idx];
    if (!fold.set) { continue; }
    LNK_SectionContrib *leader_sc = task->sect_map[fold.leader_obj_idx][fold.leader_sn - 1];
    task->sect_map[obj_idx][sect_idx] = leader_sc;
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
      if (obj->section_flags[symbol.section_number-1] & LNK_SECTION_FLAG_DEBUG) {
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

          obj->parsed_symbols[symbol_idx].section_number = section_number;
          obj->parsed_symbols[symbol_idx].value          = value;
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

// chunks at/above this size are sorted by the parallel radix (all threads) before the per-chunk
// task pass, so one giant section (e.g. merged .text) can't serialize the whole sort on one thread.
#define LNK_SORT_CONTRIBS_RADIX_MIN (64u*1024u)

// sort one chunk's contribs by Compose64Bit(obj_idx, obj_sect_idx) using the parallel radix sort.
// The key is unique per contrib (one obj/section pair each), so this matches the radsort order.
internal void
lnk_sort_contribs_chunk_radix(TP_Context *tp, Arena *arena, LNK_SectionContribChunk *chunk)
{
  U64 n = chunk->count;
  Temp t = temp_begin(arena);
  U64 *keys = push_array_no_zero(t.arena, U64, n);
  U32 *idx  = push_array_no_zero(t.arena, U32, n);
  for EachIndex(i, n) { keys[i] = Compose64Bit(chunk->v[i]->u.obj_idx, chunk->v[i]->u.obj_sect_idx); idx[i] = (U32)i; }
  lnk_radix_sort_u64_pairs(tp, t.arena, n, keys, idx);
  LNK_SectionContrib **sorted = push_array_no_zero(t.arena, LNK_SectionContrib *, n);
  for EachIndex(i, n) { sorted[i] = chunk->v[idx[i]]; }
  MemoryCopy(chunk->v, sorted, n * sizeof(sorted[0]));
  temp_end(t);
}

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_task)
{
  LNK_BuildImageTask *task = raw_task;
  LNK_SectionContribChunk *chunk = task->u.sort_contribs.chunks[task_id];
  if (chunk->count >= LNK_SORT_CONTRIBS_RADIX_MIN) { return; } // big chunks done via parallel radix
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

    symbol_ref.obj->parsed_symbols[symbol_ref.symbol_idx].value          = contrib->u.offset;
    symbol_ref.obj->parsed_symbols[symbol_ref.symbol_idx].section_number = safe_cast_u32(section_number);

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
        obj->parsed_symbols[symbol_idx].section_number = defn_parsed.section_number;
        obj->parsed_symbols[symbol_idx].value          = defn_parsed.value;
        obj->parsed_symbols[symbol_idx].storage_class  = COFF_SymStorageClass_Static;
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

  ProfBeginV("Patch Regular Symbols [%S]", obj->path);
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

      obj->parsed_symbols[symbol_idx].section_number = section_number;
      obj->parsed_symbols[symbol_idx].value          = value;
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
      COFF_ParsedSymbol          fixup_src          = lnk_parsed_symbol_from_coff_symbol_idx(fixup_symbol.obj, fixup_symbol.symbol_idx);
      COFF_SymbolValueInterpType fixup_type         = coff_interp_symbol(fixup_src.section_number, fixup_src.value, fixup_src.storage_class);
      B32                         was_fixup_removed = fixup_src.section_number == lnk_obj_get_removed_section_number(fixup_symbol.obj);

      U32 section_number;
      U32 value;
      if (was_fixup_removed || fixup_type == COFF_SymbolValueInterp_Undefined || fixup_type == COFF_SymbolValueInterp_Weak) {
        section_number = lnk_obj_get_removed_section_number(obj);
        value          = 0;
      } else {
        section_number = fixup_src.section_number;
        value          = fixup_src.value;
      }

      obj->parsed_symbols[symbol_idx].section_number = section_number;
      obj->parsed_symbols[symbol_idx].value          = value;
      obj->parsed_symbols[symbol_idx].storage_class  = COFF_SymStorageClass_Static;

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

internal
THREAD_POOL_TASK_FUNC(lnk_image_fill_task)
{
  ProfBeginFunction();
  LNK_BuildImageTask *task       = raw_task;
  String8             image_data = task->u.image_fill.image_data;
  for EachNode(n, LNK_ImageFillNode, task->u.image_fill.fill_nodes[task_id]) {
    for EachIndex(i, n->sc_count) {
      LNK_SectionContrib *sc = n->sc[i];
      // fast-path: the vast majority of contribs are a single data-node -> one direct copy, skipping
      // the list-walk + cursor bookkeeping on the hot 739MB image-write loop.
      if (sc->first_data_node.next == 0) {
        U64 image_off = sc->u.off + n->base_foff;
        Assert(image_off + sc->first_data_node.string.size <= image_data.size);
        MemoryCopyStr8(image_data.str + image_off, sc->first_data_node.string);
        continue;
      }
      U64 cursor = 0;
      for EachNode(data_n, String8Node, &sc->first_data_node) {
        U64 image_off = sc->u.off + n->base_foff + cursor;
        Assert(image_off + data_n->string.size <= image_data.size);
        MemoryCopyStr8(image_data.str + image_off, data_n->string);
        cursor += data_n->string.size;
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
  ProfBeginFunction();

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
    COFF_SectionFlags   section_flags  = obj->section_flags[sect_idx];

    if (section_flags & COFF_SectionFlag_LnkInfo)              { continue; }
    if (section_flags & COFF_SectionFlag_LnkRemove)            { continue; }
    if (section_flags & COFF_SectionFlag_CntUninitializedData) { continue; }

    // get section bytes (special case debug info because it is not copied to the image)
    String8 data           = section_flags & LNK_SECTION_FLAG_DEBUG ? obj->data : task->image_data;
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
            if (~section_flags & LNK_SECTION_FLAG_DEBUG) {
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

  ProfEnd();
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
        LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number-1];
        if (sc != task->null_sc) {
          sc->hotpatch = !!(obj->section_flags[symbol.section_number-1] & COFF_SectionFlag_CntCode);
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
THREAD_POOL_TASK_FUNC(lnk_patch_virtual_offsets_and_sizes_in_obj_section_headers_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch Virtual Offset And Size In Section Headers [%S]", obj->path);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(obj->data, obj->header.section_table_range).str;
  for (U64 sect_idx = 0; sect_idx < obj->header.section_count_no_null; sect_idx += 1) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    if (~obj->section_flags[sect_idx] & COFF_SectionFlag_LnkRemove) {
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
    COFF_SectionFlags   sect_flags  = obj->section_flags[sect_idx];
    B32 patch_section_header = (~sect_flags & COFF_SectionFlag_LnkRemove) &&
                               (~sect_flags & LNK_SECTION_FLAG_DEBUG);
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
        B32 is_referenced = 0;
        COFF_SectionHeader *section_table = lnk_coff_section_table_from_obj(obj);
        for EachIndex(sect_idx, obj->header.section_count_no_null) {
          COFF_SectionHeader *section_header = &section_table[sect_idx];
          COFF_SectionFlags   section_flags  = obj->section_flags[sect_idx];
          if (section_flags & COFF_SectionFlag_LnkRemove) { continue; }
          if (section_flags & COFF_SectionFlag_LnkInfo)   { continue; }
          if (section_flags & LNK_SECTION_FLAG_DEBUG)     { continue; }
          COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, section_header);
          for EachIndex(reloc_idx, relocs.count) {
            if (relocs.v[reloc_idx].isymbol == symbol_idx) {
              is_referenced = 1;
              break;
            }
          }
          if (is_referenced) { break; }
        }
        if (!is_referenced) { continue; }

        LNK_Section *sect = lnk_section_table_search(task->sectab, symbol.name, symbol.value);
        if (sect && (~sect->flags & COFF_SectionFlag_LnkRemove)) {
          if (~sect->flags & COFF_SectionFlag_MemDiscardable) {
            LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(sect);
            obj->parsed_symbols[symbol_idx].section_number = safe_cast_u32(first_sc->u.sect_idx + 1);
            obj->parsed_symbols[symbol_idx].value          = first_sc->u.off;
            obj->parsed_symbols[symbol_idx].storage_class  = COFF_SymStorageClass_Static;
          } else {
            lnk_error_obj(LNK_Error_SectRefsDiscardedMemory, obj, "symbol %S (No. 0x%llx) references section with discard flag", symbol.name, symbol_idx);
          }
        } else {
          U64 fallback_voff = 0;
          U64 fallback_align = Max(task->sect_align, KB(4));
          for EachIndex(sect_idx, task->image_sects.count) {
            LNK_Section *image_sect = task->image_sects.v[sect_idx];
            U64 image_sect_size = AlignPow2(Max(image_sect->vsize, image_sect->fsize), fallback_align);
            if (image_sect_size == 0) { image_sect_size = fallback_align; }
            fallback_voff = Max(fallback_voff, image_sect->voff + image_sect_size);
          }
          fallback_voff = AlignPow2(fallback_voff, fallback_align);

          LNK_Section *fallback_sect = task->image_sects.v[task->image_sects.count-1];
          U32 fallback_section_number = safe_cast_u32(fallback_sect->sect_idx + 1);
          U32 fallback_section_offset = safe_cast_u32(fallback_voff - fallback_sect->voff);
          obj->parsed_symbols[symbol_idx].section_number = fallback_section_number;
          obj->parsed_symbols[symbol_idx].value          = fallback_section_offset;
          obj->parsed_symbols[symbol_idx].storage_class  = COFF_SymStorageClass_Static;

          lnk_error_obj(LNK_Warning_UndefinedSectionSymbol, obj, "undefined section symbol %S (No. 0x%llx) refers to an image section that doesn't exist; patching to %#llx", symbol.name, symbol_idx, fallback_voff);
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_base_reloc_pages_task)
{
  LNK_BaseRelocsTask    *task       = raw_task;
  HashTable             *page_ht    = task->gather.page_ht[worker_id];
  LNK_BaseRelocPageList *pages      = &task->gather.pages[worker_id];
  LNK_Obj               *obj        = task->gather.objs[task_id];
  COFF_SectionHeader    *sect_table = lnk_coff_section_table_from_obj(obj);

  ProfBeginV("%S", obj->path);
  for EachIndex(sect_idx, obj->header.section_count_no_null) {
    COFF_SectionHeader *sect_header = &sect_table[sect_idx];
    if (obj->section_flags[sect_idx] & COFF_SectionFlag_LnkRemove) { continue; }

    COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, sect_header);
    for EachIndex(reloc_idx, relocs.count) {
      COFF_Reloc *r = &relocs.v[reloc_idx];

      COFF_ParsedSymbol          symbol        = lnk_parsed_symbol_from_coff_symbol_idx(obj, r->isymbol);
      COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol);
      if (symbol_interp == COFF_SymbolValueInterp_Abs) { continue; }

      U64 is_addr = coff_is_addr_reloc(obj->header.machine, r->type);
      if (is_addr == 0) { continue; }

      U64                    reloc_voff = sect_header->voff + r->apply_off;
      U64                    page_voff  = AlignDownPow2(reloc_voff, task->page_size);
      LNK_BaseRelocPageNode *page       = hash_table_search_u64_raw(page_ht, page_voff);
      if (page == 0) {
        // fill out page
        page         = push_array(arena, LNK_BaseRelocPageNode, 1);
        page->v.voff = page_voff;
        page->v.entries_addr32 = push_array(arena, U64List, 1);
        page->v.entries_addr64 = push_array(arena, U64List, 1);

        // push page
        SLLQueuePush(pages->first, pages->last, page);
        pages->count += 1;

        // register page voff
        hash_table_push_u64_raw(arena, page_ht, page_voff, page);
      }

      switch (is_addr) {
      case 4: {
        if (task->is_large_addr_aware) {
          lnk_error_obj(LNK_Error_LargeAddrAwareRequired, obj, "found out of range ADDR32 relocation for '%S', link with /LARGEADDRESSAWARE:NO", symbol.name);
        } else {
          u64_list_push(arena, page->v.entries_addr32, reloc_voff);
        }
      } break;
      case 8: {
        u64_list_push(arena, page->v.entries_addr64, reloc_voff);
      } break;
      default: { InvalidPath; } break;
      }
    }

  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_filter_out_duplicate_base_reloc_entries_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  LNK_BaseRelocsTask *task = raw_task;

  HashTable *voff_ht = hash_table_init(scratch.arena, task->page_size);
  for EachInRange(page_idx, task->serialize.ranges[task_id]) {
    LNK_BaseRelocPage *page = &task->serialize.pages.v[page_idx];

    U64List unique_entries32 = {0};
    U64List unique_entries64 = {0};

    // filter out duplicate 32-bit virtual offsets
    for (U64Node *curr = page->entries_addr32->first, *next; curr != 0; curr = next) {
      next = curr->next;
      if (hash_table_search_u64(voff_ht, curr->data)) { continue; }
      hash_table_push_u64_u64(scratch.arena, voff_ht, curr->data, 0);
      u64_list_push_node(&unique_entries32, curr);
    }

    // filter out duplicate 64-bit virtual offsets
    for (U64Node *curr = page->entries_addr64->first, *next; curr != 0; curr = next) {
      next = curr->next;
      if (hash_table_search_u64(voff_ht, curr->data)) { continue; }
      hash_table_push_u64_u64(scratch.arena, voff_ht, curr->data, 0);
      u64_list_push_node(&unique_entries64, curr);
    }

    *page->entries_addr32 = unique_entries32;
    *page->entries_addr64 = unique_entries64;

    // purge hash table for the next run
    hash_table_purge(voff_ht);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_serialize_base_reloc_pages_task)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);

  LNK_BaseRelocsTask *task = raw_task;

  for EachInRange(page_idx, task->serialize.ranges[task_id]) {
    LNK_BaseRelocPage *page = &task->serialize.pages.v[page_idx];

    // find block bytes in the buffer
    void *block = task->serialize.buffer + page->buffer_offset;

    // setup pointers into the block
    U32 *page_voff_ptr  = block;
    U32 *block_size_ptr = page_voff_ptr + 1;
    U16 *reloc_arr_base = (U16 *)(block_size_ptr + 1);
    U16 *reloc_arr_ptr = reloc_arr_base;

    // write 32-bit relocation entries
    U16 *reloc_entries32 = reloc_arr_ptr;
    for EachNode(n, U64Node, page->entries_addr32->first) {
      U64 rel_off = n->data - page->voff;
      *reloc_arr_ptr = PE_BaseRelocMake(PE_BaseRelocKind_HIGHLOW, rel_off);
      reloc_arr_ptr += 1;
    }
    radsort(reloc_entries32, page->entries_addr32->count, u16_is_before);

    // write 64-bit relocation entries
    U16 *reloc_entries64 = reloc_arr_ptr;
    for EachNode(n, U64Node, page->entries_addr64->first) {
      U64 rel_off = n->data - page->voff;
      *reloc_arr_ptr = PE_BaseRelocMake(PE_BaseRelocKind_DIR64, rel_off);
      reloc_arr_ptr += 1;
    }
    radsort(reloc_entries64, page->entries_addr64->count, u16_is_before);

    // compute block size
    U64 reloc_arr_size     = IntFromPtr(reloc_arr_ptr - reloc_arr_base) * sizeof(reloc_arr_ptr[0]);
    U64 block_size         = sizeof(*page_voff_ptr) + sizeof(*block_size_ptr) + reloc_arr_size;
    U64 block_size_aligned = AlignPow2(block_size, sizeof(U32));

    // zero-out alignment
    U64 align_size = block_size_aligned - block_size;
    MemoryZero(reloc_arr_ptr, align_size);

    // write page header
    *page_voff_ptr  = safe_cast_u32(page->voff);
    *block_size_ptr = safe_cast_u32(block_size_aligned);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal int
lnk_base_reloc_page_is_before(void *raw_a, void *raw_b)
{
  return ((LNK_BaseRelocPage *)raw_a)->voff < ((LNK_BaseRelocPage *)raw_b)->voff;
}

internal String8
lnk_build_base_relocs(TP_Context *tp, TP_Arena *tp_arena, LNK_Config *config, U64 objs_count, LNK_Obj **objs)
{
  ProfBeginFunction();
  Arena *arena   = tp_arena->v[0];
  Temp   scratch = scratch_begin(tp_arena->v, tp_arena->count);
  tp_arena->v[0] = scratch.arena;
  TP_Temp tp_temp = tp_temp_begin(tp_arena);

  LNK_BaseRelocsTask task  = {0};
  task.page_size           = config->machine_page_size;
  task.is_large_addr_aware = !!(config->file_characteristics & PE_ImageFileCharacteristic_LARGE_ADDRESS_AWARE);
  
  LNK_BaseRelocPageArray pages = {0};
  {
    LNK_BaseRelocPageList  *page_lists = push_array(scratch.arena, LNK_BaseRelocPageList, tp->worker_count);
    HashTable             **page_ht    = push_array(scratch.arena, HashTable *,           tp->worker_count);
    for EachIndex(i, tp->worker_count) { page_ht[i] = hash_table_init(scratch.arena, task.page_size/2); }

    task.gather.objs    = objs;
    task.gather.pages   = page_lists;
    task.gather.page_ht = page_ht;
    tp_for_parallel_prof(tp, tp_arena, objs_count, lnk_gather_base_reloc_pages_task, &task, "Gather");

    ProfBegin("Merge Page Lists");
    LNK_BaseRelocPageList *main_page_list = &page_lists[0];
    HashTable             *main_ht        = page_ht[0];
    for (U64 list_idx = 1; list_idx < tp->worker_count; list_idx += 1) {
      for (LNK_BaseRelocPageNode *src_page = page_lists[list_idx].first, *src_next; src_page != 0; src_page = src_next) {
        src_next = src_page->next;

        LNK_BaseRelocPageNode *page = hash_table_search_u64_raw(main_ht, src_page->v.voff);
        if (page) {
          // page exists, concat voffs
          Assert(page != src_page);
          u64_list_concat_in_place(page->v.entries_addr32, src_page->v.entries_addr32);
          u64_list_concat_in_place(page->v.entries_addr64, src_page->v.entries_addr64);
        } else {
          // push page to the main list
          SLLQueuePush(main_page_list->first, main_page_list->last, src_page);
          main_page_list->count += 1;

          // store lookup voff 
          hash_table_push_u64_raw(scratch.arena, main_ht, src_page->v.voff, src_page);
        }
      }
    }
    ProfEnd();

    ProfBegin("Page List -> Array");
    pages.v = push_array_no_zero(scratch.arena, LNK_BaseRelocPage, main_page_list->count);
    for EachNode(n, LNK_BaseRelocPageNode, main_page_list->first) { pages.v[pages.count++] = n->v; }
    ProfEnd();
  }

  ProfBeginV("Sort Pages [Count %llu]", pages.count);
  radsort(pages.v, pages.count, lnk_base_reloc_page_is_before);
  ProfEnd();
  
  String8 base_relocs = {0};
  {
    task.serialize.pages       = pages;
    task.serialize.ranges      = tp_divide_work(scratch.arena, pages.count, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, lnk_filter_out_duplicate_base_reloc_entries_task, &task);

    ProfBegin("Compute Buffer Size");
    U64 buffer_size = 0;
    for EachIndex(page_idx, pages.count) {
      LNK_BaseRelocPage *page = &pages.v[page_idx];
      page->buffer_offset = buffer_size;
      buffer_size += /* page base voff */ sizeof(U32) + /* size of block */ sizeof(U32); // header
      buffer_size += sizeof(U16)*page->entries_addr32->count;                            // 32-bit voff entries
      buffer_size += sizeof(U16)*page->entries_addr64->count;                            // 64-bit voff entries
      buffer_size  = AlignPow2(buffer_size, sizeof(U32));
    }
    ProfEnd();

    ProfBeginV("Alloc Buffer [%M]", buffer_size);
    U8 *buffer = push_array_no_zero(arena, U8, buffer_size);
    ProfEnd();

    task.serialize.buffer_size = buffer_size;
    task.serialize.buffer      = buffer;
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_serialize_base_reloc_pages_task, &task, "Serialize");

    base_relocs = str8(task.serialize.buffer, task.serialize.buffer_size);
  }

  tp_temp_end(tp_temp); // scratch is cleared here
  tp_arena->v[0] = arena;
  ProfEnd();
  return base_relocs;
}

internal String8List
lnk_build_win32_header(Arena *arena, LNK_SymbolTable *symtab, LNK_Config *config, LNK_SectionArray sects, U64 expected_image_header_size, U64 *file_header_offset_out, String8List *string_table_out)
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

    *file_header_offset_out = result.total_size;
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

      String8 section_name = sect->name;

      // use string table extension to store long section names
      if (sect->name.size > sizeof(coff_section->name)) {
        if (string_table_out->node_count == 0) {
          U32 *string_table_size = push_array(arena, U32, 1);
          str8_list_push_front(arena, string_table_out, str8_struct(string_table_size));
        }
        U64 name_offset = string_table_out->total_size;
        str8_list_push(arena, string_table_out, push_cstr(arena, sect->name));
        section_name = push_str8f(arena, "/%u", name_offset);
      }

      MemorySet(&coff_section->name[0], 0, sizeof(coff_section->name));
      MemoryCopy(&coff_section->name[0], section_name.str, Min(section_name.size, sizeof(coff_section->name)));
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
  str8_list_push_aligner(arena, &result, 0, config->file_align);

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

  // write string table size
  if (string_table_out->total_size) {
    U32 *string_table_size = (U32 *)string_table_out->first->string.str;
    *string_table_size = safe_cast_u32(string_table_out->total_size);
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
    .sect_align       = config->sect_align,
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

      // big chunks (e.g. merged .text) first, each sorted across all threads via parallel radix --
      // otherwise a single huge chunk serializes on one worker. Small chunks then go one-per-task.
      for EachIndex(ci, total_chunk_count) {
        LNK_SectionContribChunk *chunk = task.u.sort_contribs.chunks[ci];
        if (chunk->count >= LNK_SORT_CONTRIBS_RADIX_MIN) {
          lnk_sort_contribs_chunk_radix(tp, scratch.arena, chunk);
        }
      }
      tp_for_parallel(tp, 0, total_chunk_count, lnk_sort_contribs_task, &task);

      ProfEnd();
    }

    tp_for_parallel_prof(tp, 0, objs_count, lnk_set_comdat_leaders_contribs_task, &task, "Update Section Map With COMDAT Leader Contribs");

    // /OPT:ICF: redirect folded static-COMDAT followers' section-map entries to their leader contrib
    tp_for_parallel_prof(tp, 0, objs_count, lnk_set_icf_static_leader_contribs_task, &task, "Update Section Map With ICF Static Leader Contribs");

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
        U32 common_block_cursor = common_block_sect->vsize;

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
      String8 base_relocs_data = lnk_build_base_relocs(tp, arena, config, objs_count, objs);
      if (base_relocs_data.size) {
        LNK_Section             *reloc          = lnk_section_table_push(sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
        LNK_SectionContribChunk *first_sc_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &reloc->contribs, 1, str8_zero());
        LNK_SectionContrib      *sc             = lnk_section_contrib_chunk_push(first_sc_chunk, 1);
        sc->first_data_node.string = base_relocs_data;
        sc->last_data_node         = &sc->first_data_node;
        sc->align                  = 1;
        sc->u.obj_idx              = max_U32;

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
  U64         image_file_header_off = 0;
  String8List image_string_table    = {0};
  {
    String8List              image_header_data     = lnk_build_win32_header(sectab->arena, symtab, config, task.image_sects, AlignPow2(expected_image_header_size, config->file_align), &image_file_header_off, &image_string_table);
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

    ProfBeginV("Alloc Image Buffer [%M]", lnk_section_table_total_fsize(sectab));
    image_data.size = lnk_section_table_total_fsize(sectab) + image_string_table.total_size;
    // Standalone reservation (not the shared link arena) so it can be released the instant the image
    // is written to disk -- VirtualFree returns fast and the kernel zeroes this ~1GB on its background
    // thread, overlapping the rest of the run, instead of in the single-threaded exit rundown.
    image_data.str  = reserve_memory(image_data.size);
    commit_memory(image_data.str, image_data.size);
    ProfEnd();

    ProfBegin("Fill Align Bytes");
    for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
      LNK_Section *sect = &sect_n->data;
      ProfBeginV("Section: %S Size: %M", sect->name, sect->fsize);
      U8 fill_byte = sect->flags & COFF_SectionFlag_CntCode ? coff_code_align_byte_from_machine(config->machine) : 0;
      MemorySet(image_data.str + sect->foff, fill_byte, sect->fsize);
      ProfEnd();
    }
    ProfEnd();

    Temp temp = temp_begin(scratch.arena);

    ProfBegin("Prepare Worker Nodes");
    LNK_ImageFillNode **fill_nodes = push_array(scratch.arena, LNK_ImageFillNode *, tp->worker_count);
    U64 worker_cap = 4096, worker_load = 0, worker_idx = 0;
    for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
      LNK_Section *sect = &sect_n->data;

      // skip bss sections
      if (sect->flags & COFF_SectionFlag_CntUninitializedData) { continue; }

      for EachNode(sc_chunk, LNK_SectionContribChunk, sect->contribs.first) {
        for (U64 sc_left = sc_chunk->count; sc_left > 0; ) {
          U64 count  = Min(worker_cap - worker_load, sc_left);
          U64 sc_pos = sc_chunk->count - sc_left;
          sc_left -= count;

          LNK_ImageFillNode *n = push_array(scratch.arena, LNK_ImageFillNode, 1);
          n->base_foff = sect->foff;
          n->sc_count  = count;
          n->sc        = sc_chunk->v + sc_pos;
          SLLStackPush(fill_nodes[worker_idx], n);

          worker_load += count;
          if (worker_load >= worker_cap) {
            worker_load = 0;
            worker_idx  = (worker_idx + 1) % tp->worker_count;
          }
        }
      }
    }
    ProfEnd();

    task.u.image_fill.image_data = image_data;
    task.u.image_fill.fill_nodes = fill_nodes;
    tp_for_parallel_prof(tp, 0, tp->worker_count, lnk_image_fill_task, &task, "Fill");

    if (image_string_table.total_size) {
      ProfBegin("Copy String Table");
      String8 buffer = str8_list_join(scratch.arena, &image_string_table, 0);
      MemoryCopy(image_data.str + (image_data.size - buffer.size), buffer.str, buffer.size);
      ProfEnd();

      // patch string table offset
      COFF_FileHeader *file_header = (COFF_FileHeader *)(image_data.str + image_file_header_off);
      file_header->symbol_table_foff = safe_cast_u32(image_data.size - buffer.size);
    }

    temp_end(temp);

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

      if (obj->section_flags[sect_idx] & COFF_SectionFlag_LnkRemove) { continue; }
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
      COFF_SectionFlags section_flags = obj->section_flags[sect_idx];
      if (~section_flags & COFF_SectionFlag_LnkRemove && section_flags & LNK_SECTION_FLAG_DEBUG) {
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

  // Diagnostic: when RADLINK_PHASE_LOG is set, also write machine-parseable raw
  // per-phase micros to that file (for automated perf A/B). Env-unset -> no-op,
  // so normal/validation links are byte-identical; this never touches DLL/PDB bytes.
  char *phase_log_path = getenv("RADLINK_PHASE_LOG");
  if (phase_log_path != 0 && phase_log_path[0] != 0) {
    String8List raw_list = {0};
    for (U64 i = 0; i < LNK_Timer_Count; ++i) {
      str8_list_pushf(scratch.arena, &raw_list, "%S %llu\n", lnk_string_from_timer_type(i), g_timers[i].end - g_timers[i].begin);
    }
    str8_list_pushf(scratch.arena, &raw_list, "TOTAL %llu\n", total_build_time_micro);
    String8 raw_str = str8_list_join(scratch.arena, &raw_list, 0);
    lnk_write_data_to_file_path(str8_cstring(phase_log_path), str8_zero(), raw_str);
  }

  scratch_end(scratch);
}

internal
THREAD_POOL_TASK_FUNC(lnk_scratch_decommit_worker)
{
  // Each worker decommits the committed-but-unused pages of its OWN equipped
  // tctx scratch arenas. Runs on the worker thread, so tctx_selected() yields
  // that worker's scratch. No cross-thread arena access.
  //
  // The barrier (dispatched with task_count == worker_count) guarantees every
  // worker runs the body exactly once -- otherwise the work-stealing loop could
  // let one fast worker grab several tasks and leave other workers' scratch
  // committed. All worker_count threads are woken, so all must reach the barrier.
  tctx_scratch_decommit();
  barrier_wait(tp->barrier);
}

internal
THREAD_POOL_TASK_FUNC(lnk_p2r_worker)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_P2R *ctx = raw_task;

  P2R_ConvertParams p2r_params = {0};
  if (task_id == 0) {
    p2r_params.input_pdb_name = lnk_get_pdb_name(ctx->config);
    p2r_params.input_pdb_data = ctx->pdb_data;
    p2r_params.input_exe_name = lnk_get_image_name(ctx->config);
    p2r_params.input_exe_data = ctx->image_data;
    p2r_params.subset_flags   = max_U32;
  }
  tp_broadcast(&p2r_params);

  LaneCtx lctx = { .lane_idx = task_id, .lane_count = tp->worker_count, .barrier = tp->barrier, .broadcast_memory = tp->broadcast };
  lane_ctx(lctx);

  Arena *bake_arena = arena_alloc();
  RDIM_BakeParams bake_params = p2r_convert2(bake_arena, &p2r_params);
  barrier_wait(tp->barrier);

  RDIM_BakeResults bake_results = rdim_bake(arena, &bake_params);
  barrier_wait(tp->barrier);

  if (task_id == 0) {
    ctx->bake_results = bake_results;
  }

  scratch_end(scratch);
}

internal LNK_Obj **
lnk_debug_filter_objs(Arena *arena, LNK_Obj **objs, U64 objs_count, U64 *count_out)
{
  U64       debug_info_objs_count = 0;
  LNK_Obj **debug_info_objs       = push_array(arena, LNK_Obj *, objs_count);
  for EachIndex(obj_idx, objs_count) {
    LNK_Obj *obj = objs[obj_idx];

    // filter out internal objs from debug info output
    if (obj->exclude_from_debug_info) {
      continue;
    }

    debug_info_objs[debug_info_objs_count++] = obj;
  }
  if (count_out) {
    *count_out = debug_info_objs_count;
  }
  return debug_info_objs;
}

// Parallel release of memory-mapped input file views.
// Inputs are mapped copy-on-write (PAGE_WRITECOPY/FILE_MAP_COPY); pages touched
// during linking become private-dirty and are reclaimed by the kernel in
// single-threaded process rundown at exit (~3s for a large link). Unmapping them
// in parallel before exit moves that reclaim off the serial post-exit path.
typedef struct LNK_UnmapViewTask
{
  String8 *views;
} LNK_UnmapViewTask;

internal
THREAD_POOL_TASK_FUNC(lnk_unmap_view_task)
{
  LNK_UnmapViewTask *task = raw_task;
  String8 view = task->views[task_id];
#if OS_WINDOWS
  UnmapViewOfFile(view.str);
#elif OS_LINUX
  munmap(view.str, view.size);
#endif
}

internal void
lnk_release_input_views(TP_Context *tp, LNK_Inputer *inputer)
{
  Temp scratch = scratch_begin(0, 0);

  // collect distinct whole-file mapped views (is_thin); skip lib-member
  // substrings and linkgen arena data
  U64 cap = inputer->objs.count + inputer->libs.count;
  String8 *views = push_array_no_zero(scratch.arena, String8, cap);
  U64 count = 0;
  for EachNode(n, LNK_Input, inputer->objs.first) { if (n->is_thin && n->data.size) { views[count++] = n->data; } }
  for EachNode(n, LNK_Input, inputer->libs.first) { if (n->is_thin && n->data.size) { views[count++] = n->data; } }

  if (count > 0) {
    U64 begin_us = now_time_us();
    LNK_UnmapViewTask task = { .views = views };
    tp_for_parallel(tp, 0, count, lnk_unmap_view_task, &task);
    U64 end_us = now_time_us();
    lnk_log(LNK_Log_Timers, "Released %llu input views in %.2f ms", count, (F64)(end_us - begin_us) / 1000.0);
  }

  scratch_end(scratch);
}

internal void
lnk_run_linker(TP_Context *tp, TP_Arena *arena, LNK_Config *config)
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
  // Link Image (group digests, if any, are synthesized + consumed inside lnk_link_image)
  //
  LNK_LinkResult link = lnk_link_image(tp, arena, config, inputer, symtab);

  U64       objs_count = link.objs.count;
  U64       libs_count = link.libs.count;
  LNK_Obj **objs       = lnk_array_from_obj_list(scratch.arena, link.objs);
  LNK_Lib **libs       = lnk_array_from_lib_list(scratch.arena, link.libs);

  //
  // Layout Image
  //
  LNK_ImageContext image_ctx = lnk_build_image(arena, tp, config, symtab, objs_count, objs);

  // Write image in the background
  LNK_WriteThreadContext *image_write_ctx = push_array(scratch.arena, LNK_WriteThreadContext, 1);
  image_write_ctx->path      = config->out_path;
  image_write_ctx->temp_path = config->temp_out_path;
  image_write_ctx->data      = image_ctx.image_data;
  Thread image_write_thread = thread_launch(lnk_write_thread, image_write_ctx);

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
    String8 lib                  = pe_make_import_lib(arena->v[0], config->machine, config->time_stamp, lnk_get_image_name(config), linker_debug_symbols, config->export_symbol_list);
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
    LNK_Obj **debug_info_objs       = lnk_debug_filter_objs(scratch.arena, objs, objs_count, &debug_info_objs_count);

    //
    // CodeView
    //
    LNK_RRT_Array     rrt_input = lnk_rrt_array_from_config(arena->v[0], config);
    LNK_CodeViewInput cv        = lnk_make_code_view_input(tp, arena, config, debug_info_objs_count, debug_info_objs, rrt_input);
    LNK_MergedTypes   cv_types  = lnk_merge_types(tp, arena, &cv, 0);

    // prune merged types not reachable from any surviving symbol (PDB-size win). OFF by default:
    // it removes types that a debugger can still legitimately cast to in the watch window
    // (reachable-from-symbols is a subset of castable-types). Opt in with /OPT:GCTYPES.
    if (config->opt_gc_types == LNK_SwitchState_Yes) {
      lnk_gc_types(tp, arena->v[0], &cv, &cv_types);
    }

    // merge-types reached the scratch high-water (~9GB of per-thread tctx scratch
    // stays committed but idle). Release those unused scratch pages back to the OS
    // before the PDB build re-grows, dropping the recorded peak working set. Each
    // worker decommits its own scratch; do the main thread's scratch too. Only
    // pages strictly above each arena's live `pos` are touched, so output stays
    // byte-identical and the push path re-commits on demand during PDB build.
    {
      ProfBegin("Decommit Scratch");
      // task_count == worker_count + the in-worker barrier => every worker
      // (worker 0 IS the main thread) runs exactly once, covering main's scratch.
      tp_for_parallel_reserve(tp, 0, tp->worker_count, lnk_scratch_decommit_worker, 0); // BARRIER pass (path B)
      ProfEnd();
    }

    //
    // Debug Info
    //
    // TODO: Parallel debug info builds are currently blocked by the patch
    // strings in $$FILE_CHECKSUM step in `lnk_process_c13_data_task`.
    if (config->debug_mode == LNK_DebugMode_Full || config->rad_debug == LNK_SwitchState_Yes) {
      Temp huge_arena_temp = temp_begin(lnk_get_huge_arena());

      String8List pdb_data = {0};
      {
        lnk_timer_begin(LNK_Timer_Pdb);
        if (config->pdb_hash_type_names != LNK_TypeNameHashMode_Null && config->pdb_hash_type_names != LNK_TypeNameHashMode_None) {
          lnk_replace_type_names_with_hashes(tp,
                                             arena,
                                             cv_types.count[CV_TypeIndexSource_TPI],
                                             cv_types.v    [CV_TypeIndexSource_TPI],
                                             config->pdb_hash_type_names,
                                             config->pdb_hash_type_name_length,
                                             config->pdb_hash_type_name_map);
        }
        pdb_data = lnk_build_pdb(tp, arena, image_ctx.image_data, config, symtab, &cv, cv_types, LNK_PDB_BuilderFlag_All);
        if (config->debug_mode == LNK_DebugMode_Full) {
          lnk_write_data_list_to_file_path(config->pdb_name, config->temp_pdb_name, pdb_data);
        }
        lnk_timer_end(LNK_Timer_Pdb);
      }

      if (config->rad_debug == LNK_SwitchState_Yes) {
        lnk_timer_begin(LNK_Timer_Rdi);

        LNK_P2R p2r = { .config = config, .pdb_data = str8_list_join(lnk_get_huge_arena(), &pdb_data, 0), .image_data = image_ctx.image_data };
        tp_for_parallel_reserve(tp, arena, tp->worker_count, lnk_p2r_worker, &p2r); // BARRIER pass (path B)

        String8List rdi_blobs = rdim_file_blobs_from_section_bundle(scratch.arena, &p2r.bake_results.section_bundle);
        lnk_write_data_list_to_file_path(config->rad_debug_name, config->temp_rad_debug_name, rdi_blobs);

        lnk_timer_end(LNK_Timer_Rdi);
      }

      temp_end(huge_arena_temp);
    }

    //
    // stripped PDB
    //
    if (config->pdb_stripped_name.size != 0) {
      CV_DebugS *debug_s_arr = push_array(scratch.arena, CV_DebugS, cv.obj_count);
      for EachIndex(obj_idx, cv.obj_count) {

        CV_DebugS   *debug_s_dst = &debug_s_arr[obj_idx];
        CV_DebugS   *debug_s_src = &cv.debug_s_arr[obj_idx];
        String8List *dst         = &debug_s_dst->data_list[CV_C13SubSectionIdxKind_Symbols];
        String8List *src         = &debug_s_src->data_list[CV_C13SubSectionIdxKind_Symbols];

        U64 proc_count = 0;
        U64 proc_size  = 0;
        U64 section = 0;
        for EachNode(n, String8Node, src->first) {
          for (U64 cursor = 0; cursor < n->string.size; ) {
            U64 c = cursor;
            CV_Symbol symbol = {0};
            TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);
            if (symbol.kind == CV_SymKind_SKIP) { continue; }
            if (cv_is_lproc(symbol)) {
              proc_count += 1;
              proc_size += AlignPow2(sizeof(CV_SymbolHeader) + symbol.data.size, CV_SymbolAlign);
              proc_size += AlignPow2(sizeof(CV_SymbolHeader), CV_SymbolAlign); // S_END
            }
          }
          section += 1;
        }

        if (proc_count) {
          U64 end_count     = proc_count;
          U64 symbol_count  = proc_count + end_count;
          U64 buffer_size   = proc_size;
          U8 *buffer        = push_array(scratch.arena, U8, buffer_size);
          U64 buffer_cursor = 0;

          for EachNode(n, String8Node, src->first) {
            for (U64 cursor = 0; cursor < n->string.size; ) {
              CV_Symbol symbol = {0};
              TryReadBreak(cv_read_symbol(n->string, cursor, CV_SymbolAlign, &symbol), cursor);
              if (symbol.kind == CV_SymKind_SKIP) { continue; }
              if (cv_is_lproc(symbol)) {
                CV_SymProc32 *src_proc = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*src_proc));
                memory_write32(&src_proc->itype, 0); // strip type index
                buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &symbol, CV_SymbolAlign);
                buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &(CV_Symbol){ .kind = CV_SymKind_END }, CV_SymbolAlign);
              }
            }
          }
          Assert(buffer_cursor == buffer_size);

          str8_list_push(scratch.arena, dst, str8(buffer, buffer_size));
        }
      }

      LNK_CodeViewInput stripped_cv = {0};
      stripped_cv.config              = config;
      stripped_cv.is_stripped         = 1;
      stripped_cv.obj_arr             = cv.obj_arr;
      stripped_cv.obj_count           = cv.obj_count; 
      stripped_cv.count               = cv.obj_count;
      stripped_cv.debug_s_arr         = debug_s_arr;
      stripped_cv.symbol_input_ranges = push_array(scratch.arena, Rng1U64, tp->worker_count);

      String8List pdb_data = lnk_build_pdb(tp, arena, image_ctx.image_data, config, symtab, &stripped_cv, (LNK_MergedTypes){0}, LNK_PDB_BuilderFlag_All);
      lnk_write_data_list_to_file_path(config->pdb_stripped_name, str8f(scratch.arena, "%S.tmp", config->pdb_stripped_name), pdb_data);
    }

    lnk_timer_end(LNK_Timer_Debug);
    ProfEnd();
  }

  // wait for the thread to finish writing image to disk
  thread_join(image_write_thread, -1);

  // image is on disk and no longer read by anyone -- release its ~1GB now so the kernel reclaims it
  // concurrently with the remaining work + exit, not single-threaded in the process rundown.
  release_memory(image_ctx.image_data.str, image_ctx.image_data.size);

  // outputs are written and inputs are no longer read; release the copy-on-write
  // input views in parallel so their dirty pages are reclaimed here (multi-threaded)
  // instead of in single-threaded process rundown at exit. Only safe for the CoW
  // (read-only) mapping mode; read-write-shared would flush dirty pages back to the
  // input files on unmap.
  if ((config->io_flags & LNK_IO_Flags_MemoryMapFilesReadOnly) &&
      !(config->io_flags & LNK_IO_Flags_MemoryMapFilesReadWrite)) {
    lnk_release_input_views(tp, inputer);
  }

  //
  // Timers
  //
  {
    char *phase_log_env = getenv("RADLINK_PHASE_LOG");
    if (lnk_get_log_status(LNK_Log_Timers) || (phase_log_env != 0 && phase_log_env[0] != 0)) {
      lnk_log_timers();
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_serialize_rrt_type_data_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_RRTTypeDataSerializer *task     = raw_task;
  LNK_MergedTypes           *cv_types = task->cv_types;

  // set up per task size array
  U64     **source_sizes = 0;
  Rng1U64  *ranges[CV_TypeIndexSource_COUNT];
  if (task_id == 0) {
    source_sizes = push_array(scratch.arena, U64 *, CV_TypeIndexSource_COUNT);
    for EachIndex(i, CV_TypeIndexSource_COUNT) {
      source_sizes[i] = push_array(scratch.arena, U64, tp->worker_count);
    }

    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      ranges[ti_source] = tp_divide_work(scratch.arena, cv_types->count[ti_source], tp->worker_count);
    }
  }
  tp_broadcast(&source_sizes);
  tp_broadcast(&ranges);

  // compute size of each source per task
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    for EachInRange(i, ranges[ti_source][task_id]) {
      CV_LeafHeader *leaf      = (CV_LeafHeader *)cv_types->v[ti_source][i];
      String8        leaf_data = str8((U8 *)(leaf + 1), leaf->size - sizeof(leaf->kind));
      U64            leaf_size = cv_size_from_leaf(leaf_data, PDB_LEAF_ALIGN);
      source_sizes[ti_source][task_id] += leaf_size;
    }
  }
  barrier_wait(tp->barrier);

  // alloc buffer & set up buffer offsets for each task
  U64 *chunk_sizes        = 0;
  U64 *source_base_offsets = 0;
  U64 *source_task_offsets = 0;
  if (task_id == 0) {
    // compute chunk sizes per task
    chunk_sizes = push_array(scratch.arena, U64, tp->worker_count);
    for EachIndex(i, tp->worker_count) {
      for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
        chunk_sizes[i] += source_sizes[ti_source][i];
      }
    }

    // alloc output buffer for the serializer
    U64 total_type_size = sum_array_u64(tp->worker_count, chunk_sizes);
    *task->type_data_out = str8(push_array_no_zero(arena, U8, total_type_size), total_type_size);

    // output data ranges and source base offsets
    source_base_offsets = push_array(scratch.arena, U64, CV_TypeIndexSource_COUNT);
    U64 prev_off = 0;
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      source_base_offsets[ti_source] = prev_off;
      U64 source_size = sum_array_u64(tp->worker_count, source_sizes[ti_source]);
      task->type_data_ranges_out[ti_source] = r1u64(prev_off, prev_off + source_size);
      prev_off += source_size;
    }

    // compute per-source, per-task offsets into the output buffer
    source_task_offsets = push_array(scratch.arena, U64, CV_TypeIndexSource_COUNT*tp->worker_count);
    for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
      U64 source_cursor = source_base_offsets[ti_source];
      for EachIndex(worker_idx, tp->worker_count) {
        source_task_offsets[ti_source*tp->worker_count + worker_idx] = source_cursor;
        source_cursor += source_sizes[ti_source][worker_idx];
      }
    }
  }
  tp_broadcast(&chunk_sizes);
  tp_broadcast(&source_task_offsets);

  // serialize types
  U64 chunk_cursor = 0;
  U8 *type_data_ptr = task->type_data_out->str;
  U64 type_data_max = task->type_data_out->size;
  for EachIndex(ti_source, CV_TypeIndexSource_COUNT) {
    U64 cursor = source_task_offsets[ti_source*tp->worker_count + task_id];
    for EachInRange(i, ranges[ti_source][task_id]) {
      CV_LeafHeader *leaf      = (CV_LeafHeader *)cv_types->v[ti_source][i];
      String8        leaf_data = str8((U8 *)(leaf + 1), leaf->size - sizeof(leaf->kind));
      U64 write_size = cv_write_leaf(type_data_ptr, cursor, type_data_max, leaf->kind, leaf_data, PDB_LEAF_ALIGN);
      cursor += write_size;
      chunk_cursor += write_size;
    }
    Assert(cursor == source_task_offsets[ti_source*tp->worker_count + task_id] + source_sizes[ti_source][task_id]);
  }
  Assert(chunk_cursor == chunk_sizes[task_id]);
  barrier_wait(tp->barrier);

  scratch_end(scratch);
}

internal void
lnk_run_type_server(TP_Context *tp, TP_Arena *arena, LNK_Config *config)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  // push default type-server switches
  lnk_config_pushf(config, "/DEBUG:GHASH");
  lnk_config_pushf(config, "/NOD");
  lnk_config_pushf(config, "/RAD_WRITE_TEMP_FILES");

  // load objs
  U64       objs_count = 0;
  LNK_Obj **objs       = 0;
  {
    LNK_Inputer *inputer = lnk_inputer_init();
    LNK_Link    *link    = lnk_link_init(arena, config);

    // input :null_obj
    String8 null_obj = lnk_make_null_obj(inputer->arena);
    lnk_inputer_push_obj_linkgen(inputer, 0, str8_lit("* Null *"), null_obj);

    // input objs on command line
    for (String8Node *obj_path = config->input_list[LNK_Input_Obj].first; obj_path != 0; obj_path = obj_path->next) {
      lnk_inputer_push_obj_thin(inputer, 0, obj_path->string);
    }

    LNK_ObjNode *obj_nodes  = lnk_load_objs(tp, arena, config, inputer, 0, link, &objs_count);
    objs = push_array(scratch.arena, LNK_Obj *, objs_count);
    for EachIndex(obj_idx, objs_count) { objs[obj_idx] = &obj_nodes[obj_idx].data; }
  }

  // obj filter
  objs = lnk_debug_filter_objs(scratch.arena, objs, objs_count, &objs_count);

  // parse & merge types
  LNK_CodeViewInput cv       = lnk_make_code_view_input(tp, arena, config, objs_count, objs, (LNK_RRT_Array){0});
  LNK_MergedTypes   cv_types = lnk_merge_types(tp, arena, &cv, LNK_MergeTypeFlag_SkipSymbolTypeFixup | LNK_MergeTypeFlag_BuildObjTiMap | LNK_MergeTypeFlag_ExportHashes);

  U64Array include_objs = {0};
  {
    U64List include_obj_list = {0};
    for EachIndex(obj_idx, objs_count) {
      LNK_Obj *obj = objs[obj_idx];

      // skip objs in libraries
      if (obj->link_member) { continue; }

      // does obj have debug types?
      if (obj->debug_p_sect_idx < obj->header.section_count_no_null) {
        u64_list_push(scratch.arena, &include_obj_list, obj_idx);
      } else if (obj->debug_t_sect_idx < obj->header.section_count_no_null) {
        // read first leaf
        LNK_ObjSection section     = lnk_obj_section_from_sect_idx(obj, obj->debug_t_sect_idx);
        String8        raw_debug_t = str8_substr(obj->data, section.frange);
        CV_Leaf             leaf           = {0};
        cv_read_leaf(raw_debug_t, 0, 1, &leaf);

        // is this /Z7 obj?
        if ( ! cv_is_leaf_type_server(leaf.kind)) {
          u64_list_push(scratch.arena, &include_obj_list, obj_idx);
        }
      }
    }
    include_objs = u64_array_from_list(scratch.arena, &include_obj_list);
  }

  LNK_RRT rrt = {0};
  ProfScope("Pack Type Data & Data Ranges")
  {
    LNK_RRTTypeDataSerializer task = { &cv_types, &rrt.type_data_raw, rrt.type_data_ranges };
    tp_for_parallel_reserve(tp, arena, tp->worker_count, lnk_serialize_rrt_type_data_task, &task); // BARRIER pass (path B)

    // pack type index ranges
    for EachIndex(i, CV_TypeIndexSource_COUNT) {
      rrt.ti_ranges[i] = r1u64(cv_types.min_type_indices[i], cv_types.min_type_indices[i] + cv_types.count[i]);
    }

    // copy pointers to type hashes
    MemoryCopyArray(rrt.type_hashes_unpacked, cv_types.hashes);
  }

  rrt.obj_count = include_objs.count;

  // copy per object leaf counts
  rrt.obj_leaf_counts = push_array(scratch.arena, U64, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64        obj_idx = include_objs.v[i];
    CV_DebugT *debug_t = &cv.debug_t_arr[obj_idx];
    rrt.obj_leaf_counts[i] = debug_t->count;
  }

  // copy per object type index ranges
  rrt.obj_ti_ranges = push_array(scratch.arena, Rng1U64, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64        obj_idx = include_objs.v[i];
    LNK_Obj   *obj     = objs[obj_idx];
    CV_DebugT *debug_t = &cv.debug_t_arr[obj_idx];
    rrt.obj_ti_ranges[i] = debug_t->ti_ranges[CV_TypeIndexSource_TPI];
  }

  // copy obj time stamps
  rrt.obj_time_stamps = push_array(scratch.arena, U64, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64             obj_idx        = include_objs.v[i];
    LNK_Obj        *obj            = objs[obj_idx];
    FileProperties  obj_file_props = properties_from_file_path(obj->path);
    rrt.obj_time_stamps[i] = obj_file_props.modified;
  }

  // copy obj file paths
  rrt.obj_paths = str8_array_reserve(scratch.arena, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64     obj_idx = include_objs.v[i];
    LNK_Obj *obj    = objs[obj_idx];
    rrt.obj_paths.v[rrt.obj_paths.count++] = obj->path;
  }

  // wire per object file type index map
  rrt.obj_ti_maps = push_array(scratch.arena, CV_TypeIndex *, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64 obj_idx = include_objs.v[i];
    rrt.obj_ti_maps[i] = cv_types.obj_ti_maps[obj_idx];
  }

  // obj idx -> RRT obj idx hash map
  HashMap obj_to_rrt_obj_hm = {0};
  for EachIndex(i, include_objs.count) {
    hash_map_push_u64_u64(scratch.arena, &obj_to_rrt_obj_hm, include_objs.v[i], i);
  }

  // pack PCH info
  rrt.obj_pch_indices   = push_array(scratch.arena, U32,     include_objs.count);
  rrt.obj_pch_ti_ranges = push_array(scratch.arena, Rng1U64, include_objs.count);
  for EachIndex(i, include_objs.count) {
    U64        obj_idx         = include_objs.v[i];
    CV_DebugT *debug_t         = &cv.debug_t_arr[obj_idx];
    if (dim_1u64(debug_t->pch_ti_range[CV_TypeIndexSource_TPI]) > 0) {
      rrt.obj_pch_indices  [i] = safe_cast_u32(*hash_map_search_u64_u64(&obj_to_rrt_obj_hm, debug_t->pch_obj_idx));
      rrt.obj_pch_ti_ranges[i] = debug_t->pch_ti_range[CV_TypeIndexSource_TPI];
    } else {
      rrt.obj_pch_indices  [i] = max_U32;
      rrt.obj_pch_ti_ranges[i] = r1u64(0,0);
    }
  }

  String8List rrt_data = lnk_string_list_from_rrt(scratch.arena, &rrt);
  lnk_write_data_list_to_file_path(config->type_server_name, config->temp_type_server_name, rrt_data);

  scratch_end(scratch);
  ProfEnd();
}

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);
  lnk_log_begin();

  LNK_Config *config   = lnk_config_from_argcv(cmdline);
  TP_Context *tp       = tp_alloc(scratch.arena, config->worker_count, config->max_worker_count, config->shared_thread_pool_name);
  TP_Arena   *tp_arena = tp_arena_alloc(tp);

  // detect type server from the environment
  {
    HashMap      env             = lnk_env_vars_from_process_info(scratch.arena, get_process_info(), LNK_EnvVarRule_Current);
    LNK_EnvVar  *type_server_var = lnk_env_var_from_mapf(&env, "RAD_TYPE_SERVER");
    if (type_server_var) {
      U64 do_type_server  = 0;
      if (lnk_env_var_to_u64(&env, type_server_var, &do_type_server)) {
        if (do_type_server) {
          lnk_config_pushf(config, "/RAD_TYPE_SERVER");
          lnk_log(LNK_Log_Debug, "type server mode was enabled from the environment\n");
        }
      }
    }
  }

  if (lnk_get_log_status(LNK_Log_Debug)) {
    String8 full_cmd_line = str8_list_join(scratch.arena, &config->raw_cmd_line, &(StringJoin){ .sep = str8_lit_comp(" ") });
    lnk_fprintf(stderr, "--------------------------------------------------------------------------------\n");
    lnk_fprintf(stderr, "Command Line: %.*s\n", str8_varg(full_cmd_line));
    lnk_fprintf(stderr, "Work Dir    : %.*s\n", str8_varg(config->work_dir));
    lnk_fprintf(stderr, "--------------------------------------------------------------------------------\n");
  }

  switch (config->boot_mode) {
  case LNK_BootMode_Linker:      lnk_run_linker       (tp, tp_arena, config); break;
  case LNK_BootMode_TypeServer:  lnk_run_type_server  (tp, tp_arena, config); break;
  }

  lnk_log_end();
  scratch_end(scratch);
}
